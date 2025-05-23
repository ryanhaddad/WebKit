/*
 * Copyright (C) 2006-2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSContextRef.h"
#include "JSContextRefInternal.h"

#include "APICast.h"
#include "CallFrame.h"
#include "InitializeThreading.h"
#include "IntegrityInlines.h"
#include "JSAPIGlobalObject.h"
#include "JSAPIWrapperObject.h"
#include "JSCallbackObject.h"
#include "JSClassRef.h"
#include "JSObjectInlines.h"
#include "StackVisitor.h"
#include "StrongInlines.h"
#include "StructureInlines.h"
#include "Watchdog.h"
#include <wtf/text/StringBuilder.h>

#if ENABLE(REMOTE_INSPECTOR)
#include "JSGlobalObjectDebuggable.h"
#include "JSGlobalObjectInspectorController.h"
#include "JSRemoteInspector.h"
#endif

#if ENABLE(INSPECTOR_ALTERNATE_DISPATCHERS)
#include "JSContextRefInspectorSupport.h"
#endif

#if OS(DARWIN)
#include <mach-o/dyld.h>

static constexpr int32_t webkitFirstVersionWithConcurrentGlobalContexts = 0x2100500; // 528.5.0
#endif

using namespace JSC;

// From the API's perspective, a context group remains alive iff
//     (a) it has been JSContextGroupRetained
//     OR
//     (b) one of its contexts has been JSContextRetained

JSContextGroupRef JSContextGroupCreate()
{
    JSC::initialize();
    return toRef(&VM::createContextGroup().leakRef());
}

JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group)
{
    toJS(group)->refSuppressingSaferCPPChecking();
    return group;
}

void JSContextGroupRelease(JSContextGroupRef group)
{
    VM& vm = *toJS(group);

    JSLockHolder locker(&vm);
    vm.derefSuppressingSaferCPPChecking();
}

static bool internalScriptTimeoutCallback(JSGlobalObject* globalObject, void* callbackPtr, void* callbackData)
{
    JSShouldTerminateCallback callback = reinterpret_cast<JSShouldTerminateCallback>(callbackPtr);
    JSContextRef contextRef = toRef(globalObject);
    ASSERT(callback);
    return callback(contextRef, callbackData);
}

void JSContextGroupSetExecutionTimeLimit(JSContextGroupRef group, double limit, JSShouldTerminateCallback callback, void* callbackData)
{
    VM& vm = *toJS(group);
    JSLockHolder locker(&vm);
    Ref watchdog = vm.ensureWatchdog();
    if (callback) {
        void* callbackPtr = reinterpret_cast<void*>(callback);
        watchdog->setTimeLimit(Seconds { limit }, internalScriptTimeoutCallback, callbackPtr, callbackData);
    } else
        watchdog->setTimeLimit(Seconds { limit });
}

void JSContextGroupClearExecutionTimeLimit(JSContextGroupRef group)
{
    VM& vm = *toJS(group);
    JSLockHolder locker(&vm);
    if (RefPtr watchdog = vm.watchdog())
        watchdog->setTimeLimit(Watchdog::noTimeLimit);
}

// From the API's perspective, a global context remains alive iff it has been JSGlobalContextRetained.

JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass)
{
    JSC::initialize();

#if OS(DARWIN)
    // If the application was linked before JSGlobalContextCreate was changed to use a unique VM,
    // we use a shared one for backwards compatibility.
    static VM* s_sharedVM;
    static std::once_flag once;
    std::call_once(once, [] {
        if (NSVersionOfLinkTimeLibrary("JavaScriptCore") <= webkitFirstVersionWithConcurrentGlobalContexts)
            s_sharedVM = &VM::createContextGroup().leakRef();
    });
    if (s_sharedVM) [[unlikely]]
        return JSGlobalContextCreateInGroup(toRef(s_sharedVM), globalObjectClass);
#endif // OS(DARWIN)

    return JSGlobalContextCreateInGroup(nullptr, globalObjectClass);
}

JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group, JSClassRef globalObjectClass)
{
    JSC::initialize();

    Ref<VM> vm = group ? Ref<VM>(*toJS(group)) : VM::createContextGroup();

    JSLockHolder locker(vm.ptr());

    if (!globalObjectClass) {
        JSGlobalObject* globalObject = JSAPIGlobalObject::create(vm.get(), JSAPIGlobalObject::createStructure(vm.get(), jsNull()));
#if ENABLE(REMOTE_INSPECTOR)
        globalObject->setInspectable(JSRemoteInspectorGetInspectionEnabledByDefault());
#endif
        return JSGlobalContextRetain(toGlobalRef(globalObject));
    }

    JSGlobalObject* globalObject = JSCallbackObject<JSGlobalObject>::create(vm.get(), globalObjectClass, JSCallbackObject<JSGlobalObject>::createStructure(vm.get(), nullptr, jsNull()));
    JSValue prototype = globalObjectClass->prototype(globalObject);
    if (!prototype)
        prototype = jsNull();
    globalObject->resetPrototype(vm.get(), prototype);
#if ENABLE(REMOTE_INSPECTOR)
    globalObject->setInspectable(JSRemoteInspectorGetInspectionEnabledByDefault());
#endif
    return JSGlobalContextRetain(toGlobalRef(globalObject));
}

JSGlobalContextRef JSGlobalContextRetain(JSGlobalContextRef ctx)
{
    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    gcProtect(globalObject);
    vm.refSuppressingSaferCPPChecking();
    return ctx;
}

void JSGlobalContextRelease(JSGlobalContextRef ctx)
{
    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    bool protectCountIsZero = vm.heap.unprotect(globalObject);
    if (protectCountIsZero)
        vm.heap.reportAbandonedObjectGraph();
    vm.derefSuppressingSaferCPPChecking();
}

JSObjectRef JSContextGetGlobalObject(JSContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }
    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    return toRef(jsCast<JSObject*>(JSValue(globalObject).toThis(globalObject, ECMAMode::sloppy())));
}

JSContextGroupRef JSContextGetGroup(JSContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }
    JSGlobalObject* globalObject = toJS(ctx);
    return toRef(&globalObject->vm());
}

JSGlobalContextRef JSContextGetGlobalContext(JSContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }
    JSGlobalObject* globalObject = toJS(ctx);
    JSLockHolder locker(globalObject);

    return toGlobalRef(globalObject);
}

JSStringRef JSGlobalContextCopyName(JSGlobalContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    String name = globalObject->name();
    if (name.isNull())
        return nullptr;

    return OpaqueJSString::tryCreate(name).leakRef();
}

void JSGlobalContextSetName(JSGlobalContextRef ctx, JSStringRef name)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    globalObject->setName(name ? name->string() : String());
}

bool JSGlobalContextIsInspectable(JSGlobalContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    return globalObject->inspectable();
}

void JSGlobalContextSetInspectable(JSGlobalContextRef ctx, bool inspectable)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    globalObject->setInspectable(inspectable);
}

void JSGlobalContextSetUnhandledRejectionCallback(JSGlobalContextRef ctx, JSObjectRef function, JSValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    JSObject* object = toJS(function);
    if (!object->isCallable()) {
        *exception = toRef(createTypeError(globalObject));
        return;
    }

    globalObject->setUnhandledRejectionCallback(vm, object);
}

void JSGlobalContextSetEvalEnabled(JSGlobalContextRef ctx, bool enabled, JSStringRef message)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder locker(vm);

    globalObject->setEvalEnabled(enabled, message ? message->string() : String());
}

class BacktraceFunctor {
public:
    BacktraceFunctor(StringBuilder& builder, unsigned remainingCapacityForFrameCapture)
        : m_builder(builder)
        , m_remainingCapacityForFrameCapture(remainingCapacityForFrameCapture)
    {
    }

    IterationStatus operator()(StackVisitor& visitor) const
    {
        if (m_remainingCapacityForFrameCapture) {
            // If callee is unknown, but we've not added any frame yet, we should
            // still add the frame, because something called us, and gave us arguments.
            if (visitor->callee().isCell()) {
                JSCell* callee = visitor->callee().asCell();
                if (!callee && visitor->index())
                    return IterationStatus::Done;
            }

            StringBuilder& builder = m_builder;
            if (!builder.isEmpty())
                builder.append('\n');
            builder.append('#', visitor->index(), ' ', visitor->functionName(), "() at "_s, visitor->sourceURL());
            if (visitor->hasLineAndColumnInfo()) {
                auto lineColumn = visitor->computeLineAndColumn();
                builder.append(':', lineColumn.line);
            }

            if (!visitor->callee().rawPtr())
                return IterationStatus::Done;

            m_remainingCapacityForFrameCapture--;
            return IterationStatus::Continue;
        }
        return IterationStatus::Done;
    }

private:
    StringBuilder& m_builder;
    mutable unsigned m_remainingCapacityForFrameCapture;
};

JSStringRef JSContextCreateBacktrace(JSContextRef ctx, unsigned maxStackSize)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }
    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);
    StringBuilder builder;
    CallFrame* frame = vm.topCallFrame;

    ASSERT(maxStackSize);
    BacktraceFunctor functor(builder, maxStackSize);
    StackVisitor::visit(frame, vm, functor);

    return OpaqueJSString::tryCreate(builder.toString()).leakRef();
}

bool JSGlobalContextGetRemoteInspectionEnabled(JSGlobalContextRef ctx)
{
    return JSGlobalContextIsInspectable(ctx);
}

void JSGlobalContextSetRemoteInspectionEnabled(JSGlobalContextRef ctx, bool enabled)
{
    JSGlobalContextSetInspectable(ctx, enabled);
}

bool JSGlobalContextGetIncludesNativeCallStackWhenReportingExceptions(JSGlobalContextRef ctx)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    return globalObject->inspectorController().includesNativeCallStackWhenReportingExceptions();
#else
    UNUSED_PARAM(ctx);
    return false;
#endif
}

void JSGlobalContextSetIncludesNativeCallStackWhenReportingExceptions(JSGlobalContextRef ctx, bool includesNativeCallStack)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    globalObject->inspectorController().setIncludesNativeCallStackWhenReportingExceptions(includesNativeCallStack);
#else
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(includesNativeCallStack);
#endif
}

#if USE(CF)
CFRunLoopRef JSGlobalContextGetDebuggerRunLoop(JSGlobalContextRef ctx)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    return globalObject->inspectorDebuggable().targetRunLoop();
#else
    UNUSED_PARAM(ctx);
    return nullptr;
#endif
}

void JSGlobalContextSetDebuggerRunLoop(JSGlobalContextRef ctx, CFRunLoopRef runLoop)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    globalObject->protectedInspectorDebuggable()->setTargetRunLoop(runLoop);
#else
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(runLoop);
#endif
}
#endif // USE(CF)

#if ENABLE(INSPECTOR_ALTERNATE_DISPATCHERS)
Inspector::AugmentableInspectorController* JSGlobalContextGetAugmentableInspectorController(JSGlobalContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);

    return &globalObject->inspectorController();
}
#endif

bool JSContextGroupEnableSamplingProfiler(JSContextGroupRef group)
{
    VM& vm = *toJS(group);
    JSLockHolder locker(&vm);

#if ENABLE(SAMPLING_PROFILER)
    vm.enableSamplingProfiler();
    return true;
#else
    return false;
#endif
}

void JSContextGroupDisableSamplingProfiler(JSContextGroupRef group)
{
    VM& vm = *toJS(group);
    JSLockHolder locker(&vm);

#if ENABLE(SAMPLING_PROFILER)
    vm.disableSamplingProfiler();
#endif
}

JSStringRef JSContextGroupTakeSamplesFromSamplingProfiler(JSContextGroupRef group)
{
    VM& vm = *toJS(group);
    JSLockHolder locker(&vm);

#if ENABLE(SAMPLING_PROFILER)
    auto json = vm.takeSamplingProfilerSamplesAsJSON();
    if (!json) [[unlikely]]
        return nullptr;

    auto jsonData = json->toJSONString();
    if (jsonData.isNull()) [[unlikely]]
        return nullptr;

    return OpaqueJSString::tryCreate(WTFMove(jsonData)).leakRef();
#else
    return nullptr;
#endif
}

