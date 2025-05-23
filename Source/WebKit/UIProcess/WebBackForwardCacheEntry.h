/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <WebCore/BackForwardItemIdentifier.h>
#include <WebCore/ProcessIdentifier.h>
#include <wtf/CheckedRef.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/RunLoop.h>
#include <wtf/TZoneMalloc.h>

namespace WebKit {

class SuspendedPageProxy;
class WebBackForwardCache;
class WebProcessProxy;

class WebBackForwardCacheEntry : public RefCounted<WebBackForwardCacheEntry> {
    WTF_MAKE_TZONE_ALLOCATED(WebBackForwardCacheEntry);
public:
    static Ref<WebBackForwardCacheEntry> create(WebBackForwardCache&, WebCore::BackForwardItemIdentifier, WebCore::ProcessIdentifier, RefPtr<SuspendedPageProxy>&&);
    ~WebBackForwardCacheEntry();

    WebBackForwardCache* backForwardCache() const;

    SuspendedPageProxy* suspendedPage() const { return m_suspendedPage.get(); }
    Ref<SuspendedPageProxy> takeSuspendedPage();
    WebCore::ProcessIdentifier processIdentifier() const { return m_processIdentifier; }
    RefPtr<WebProcessProxy> process() const;

private:
    WebBackForwardCacheEntry(WebBackForwardCache&, WebCore::BackForwardItemIdentifier, WebCore::ProcessIdentifier, RefPtr<SuspendedPageProxy>&&);

    void expirationTimerFired();

    WeakPtr<WebBackForwardCache> m_backForwardCache;
    WebCore::ProcessIdentifier m_processIdentifier;
    Markable<WebCore::BackForwardItemIdentifier> m_backForwardItemID;
    RefPtr<SuspendedPageProxy> m_suspendedPage;
    RunLoop::Timer m_expirationTimer;
};

} // namespace WebKit
