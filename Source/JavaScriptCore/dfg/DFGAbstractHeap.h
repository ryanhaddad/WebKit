/*
 * Copyright (C) 2013-2019 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(DFG_JIT)

#include "DOMJITHeapRange.h"
#include "OperandsInlines.h"
#include "VirtualRegister.h"
#include <wtf/HashMap.h>
#include <wtf/PrintStream.h>

namespace JSC { namespace DFG {

// Implements a four-level type hierarchy:
// - World is the supertype of all of the things.
// - Stack with a TOP payload is a direct subtype of World
// - Stack with a non-TOP payload is a direct subtype of Stack with a TOP payload.
// - Heap is a direct subtype of World.
// - SideState is a direct subtype of World.
// - Any other kind with TOP payload is the direct subtype of Heap.
// - Any other kind with non-TOP payload is the direct subtype of the same kind with a TOP payload.

#define FOR_EACH_ABSTRACT_HEAP_KIND(macro) \
    macro(InvalidAbstractHeap) \
    macro(World) \
    macro(Stack) \
    macro(Heap) \
    macro(Butterfly_publicLength) \
    macro(Butterfly_vectorLength) \
    macro(GetterSetter_getter) \
    macro(GetterSetter_setter) \
    macro(JSCell_cellState) \
    macro(JSCell_indexingType) \
    macro(JSCell_structureID) \
    macro(JSCell_typeInfoFlags) \
    macro(JSObject_butterfly) \
    macro(JSPropertyNameEnumerator_cachedPropertyNames) \
    macro(RegExpObject_lastIndex) \
    macro(NamedProperties) \
    macro(IndexedProperties) \
    macro(IndexedInt32Properties) \
    macro(IndexedDoubleProperties) \
    macro(IndexedContiguousProperties) \
    macro(IndexedArrayStorageProperties) \
    macro(DirectArgumentsProperties) \
    macro(ScopeProperties) \
    macro(TypedArrayProperties) \
    macro(HeapObjectCount) /* Used to reflect the fact that some allocations reveal object identity */\
    macro(RegExpState) \
    macro(MathDotRandomState) \
    macro(JSDateFields) \
    macro(JSGlobalProxy_target) \
    macro(JSMapFields) \
    macro(JSSetFields) \
    macro(JSMapIteratorFields) \
    macro(JSSetIteratorFields) \
    macro(JSWeakMapFields) \
    macro(JSWeakSetFields) \
    macro(JSInternalFields) \
    macro(InternalState) \
    macro(CatchLocals) \
    macro(Absolute) \
    /* DOMJIT tells the heap range with the pair of integers. */\
    macro(DOMState) \
    /* Use this for writes only, to indicate that this may fire watchpoints. Usually this is never directly written but instead we test to see if a node clobbers this; it just so happens that you have to write world to clobber it. */\
    macro(Watchpoint_fire) \
    /* Use these for reads only, just to indicate that if the world got clobbered, then this operation will not work. */\
    macro(MiscFields) \
    /* Use this for writes only, just to indicate that hoisting the node is invalid. This works because we don't hoist anything that has any side effects at all. */\
    macro(SideState)

enum AbstractHeapKind : uint8_t {
#define ABSTRACT_HEAP_DECLARATION(name) name,
    FOR_EACH_ABSTRACT_HEAP_KIND(ABSTRACT_HEAP_DECLARATION)
#undef ABSTRACT_HEAP_DECLARATION
};

class AbstractHeap {
public:
    class Payload {
    public:
        Payload()
            : m_isTop(false)
            , m_value(0)
        {
        }
        
        Payload(bool isTop, int64_t value)
            : m_isTop(isTop)
            , m_value(value)
        {
            ASSERT(!(isTop && value));
        }
        
        Payload(int64_t value)
            : m_isTop(false)
            , m_value(value)
        {
        }
        
        Payload(const void* pointer)
            : m_isTop(false)
            , m_value(static_cast<int64_t>(reinterpret_cast<uintptr_t>(pointer)))
        {
        }

        Payload(Operand operand)
            : m_isTop(false)
            , m_value(operand.asBits())
        {
        }

        Payload(VirtualRegister operand)
            : Payload(Operand(operand))
        {
        }
        
        static Payload top() { return Payload(true, 0); }
        
        bool isTop() const { return m_isTop; }
        int64_t value() const
        {
            ASSERT(!isTop());
            return valueImpl();
        }
        int64_t valueImpl() const
        {
            return m_value;
        }
        
        int32_t value32() const
        {
            return static_cast<int32_t>(value());
        }
        
        friend bool operator==(const Payload&, const Payload&) = default;
        
        bool operator<(const Payload& other) const
        {
            if (isTop())
                return !other.isTop();
            if (other.isTop())
                return false;
            return value() < other.value();
        }
        
        bool isDisjoint(const Payload& other) const
        {
            if (isTop())
                return false;
            if (other.isTop())
                return false;
            return m_value != other.m_value;
        }
        
        bool overlaps(const Payload& other) const
        {
            return !isDisjoint(other);
        }
        
        void dump(PrintStream&) const;
        void dumpAsOperand(PrintStream&) const;
        
    private:
        bool m_isTop;
        int64_t m_value;
    };
    
    AbstractHeap()
    {
        m_value = encode(InvalidAbstractHeap, Payload());
    }
    
    AbstractHeap(AbstractHeapKind kind)
    {
        ASSERT(kind != InvalidAbstractHeap);
        m_value = encode(kind, Payload::top());
    }
    
    AbstractHeap(AbstractHeapKind kind, Payload payload)
    {
        ASSERT(kind != InvalidAbstractHeap && kind != World && kind != Heap && kind != SideState);
        m_value = encode(kind, payload);
        ASSERT(this->kind() == kind && this->payload() == payload);
    }
    
    AbstractHeap(WTF::HashTableDeletedValueType)
    {
        m_value = encode(InvalidAbstractHeap, Payload::top());
    }
    
    bool operator!() const { return kind() == InvalidAbstractHeap && !payloadImpl().isTop(); }
    
    AbstractHeapKind kind() const { return static_cast<AbstractHeapKind>((m_value & kindMask) >> kindShift); }
    Payload payload() const
    {
        ASSERT(kind() != World && kind() != InvalidAbstractHeap);
        return payloadImpl();
    }
    Operand operand() const
    {
        ASSERT(kind() == Stack && !payload().isTop());
        return Operand::fromBits(payload().value());
    }

    AbstractHeap supertype() const
    {
        ASSERT(kind() != InvalidAbstractHeap);
        switch (kind()) {
        case World:
            return AbstractHeap();
        default:
            if (payload().isTop())
                return superKind(kind());
            return AbstractHeap(kind());
        }
    }
    
    bool isStrictSubtypeOf(const AbstractHeap& other) const
    {
        AbstractHeap current = *this;
        if (current.kind() == DOMState && other.kind() == DOMState) {
            Payload currentPayload = current.payload();
            Payload otherPayload = other.payload();
            if (currentPayload.isTop())
                return false;
            if (otherPayload.isTop())
                return true;
            return DOMJIT::HeapRange::fromRaw(currentPayload.value32()).isStrictSubtypeOf(DOMJIT::HeapRange::fromRaw(otherPayload.value32()));
        }
        while (current.kind() != World) {
            current = current.supertype();
            if (current == other)
                return true;
        }
        return false;
    }
    
    bool isSubtypeOf(const AbstractHeap& other) const
    {
        return *this == other || isStrictSubtypeOf(other);
    }
    
    bool overlaps(const AbstractHeap& other) const
    {
        return *this == other || isStrictSubtypeOf(other) || other.isStrictSubtypeOf(*this);
    }
    
    bool isDisjoint(const AbstractHeap& other) const
    {
        return !overlaps(other);
    }
    
    unsigned hash() const
    {
        return WTF::IntHash<int64_t>::hash(m_value);
    }
    
    friend bool operator==(const AbstractHeap&, const AbstractHeap&) = default;
    
    bool operator<(const AbstractHeap& other) const
    {
        if (kind() != other.kind())
            return kind() < other.kind();
        return payload() < other.payload();
    }
    
    bool isHashTableDeletedValue() const
    {
        return kind() == InvalidAbstractHeap && payloadImpl().isTop();
    }

    static AbstractHeapKind superKind(AbstractHeapKind kind)
    {
        switch (kind) {
        case InvalidAbstractHeap:
        case World:
            return InvalidAbstractHeap;

        case Heap:
        case Stack:
        case SideState:
            return World;

        case IndexedInt32Properties:
        case IndexedDoubleProperties:
        case IndexedContiguousProperties:
        case IndexedArrayStorageProperties:
        case DirectArgumentsProperties:
        case ScopeProperties:
        case TypedArrayProperties:
            return IndexedProperties;

        case IndexedProperties:
            return Heap;

        case NamedProperties:
            return Heap;

        case Butterfly_publicLength:
        case Butterfly_vectorLength:
        case GetterSetter_getter:
        case GetterSetter_setter:
        case JSCell_cellState:
        case JSCell_indexingType:
        case JSCell_structureID:
        case JSCell_typeInfoFlags:
        case JSObject_butterfly:
        case JSPropertyNameEnumerator_cachedPropertyNames:
        case RegExpObject_lastIndex:
        case HeapObjectCount:
        case RegExpState:
        case MathDotRandomState:
        case JSDateFields:
        case JSGlobalProxy_target:
        case JSMapFields:
        case JSSetFields:
        case JSMapIteratorFields:
        case JSSetIteratorFields:
        case JSWeakMapFields:
        case JSWeakSetFields:
        case JSInternalFields:
        case InternalState:
        case CatchLocals:
        case Absolute:
        case DOMState:
        case Watchpoint_fire:
        case MiscFields:
            return Heap;
        }

        return InvalidAbstractHeap;
    }

    void dump(PrintStream& out) const;
    
private:
    static constexpr unsigned kindShift = 48;
    static_assert(48 >= OS_CONSTANT(EFFECTIVE_ADDRESS_WIDTH));

    static constexpr unsigned kindBits = 7;
    static constexpr uint64_t kindMask = ((1ull << (kindBits)) - 1) << kindShift;
    static constexpr uint64_t kindAndTopMask = ((1ull << (kindBits + 1)) - 1) << kindShift;
    static constexpr unsigned topShift = kindShift + kindBits;
    static_assert(48 >= Operand::maxBits, "Operand should fit in the bottom 48 bit region");
    
    Payload payloadImpl() const
    {
        return Payload((m_value >> topShift) & 1, m_value & ~kindAndTopMask);
    }
    
    static int64_t encode(AbstractHeapKind kind, Payload payload)
    {
        uint64_t kindAsInt = static_cast<uint64_t>(kind);
        ASSERT(kindAsInt < (1u << kindBits));
        ASSERT(!(std::bit_cast<uint64_t>(payload.valueImpl()) & kindAndTopMask));
        return (kindAsInt << kindShift) | (static_cast<uint64_t>(payload.isTop()) << topShift) | std::bit_cast<uint64_t>(payload.valueImpl());
    }
    
    // The layout of the value is:
    // Low 48 bits: the payload.value().
    // Next 7 bits: the Kind.
    // 55th bit: whether or not the payload is TOP.
    int64_t m_value;
};

struct AbstractHeapHash {
    static unsigned hash(const AbstractHeap& key) { return key.hash(); }
    static bool equal(const AbstractHeap& a, const AbstractHeap& b) { return a == b; }
    static constexpr bool safeToCompareToEmptyOrDeleted = true;
};

} } // namespace JSC::DFG

namespace WTF {

void printInternal(PrintStream&, JSC::DFG::AbstractHeapKind);

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::DFG::AbstractHeap> : JSC::DFG::AbstractHeapHash { };

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::DFG::AbstractHeap> : SimpleClassHashTraits<JSC::DFG::AbstractHeap> { };

} // namespace WTF

#endif // ENABLE(DFG_JIT)
