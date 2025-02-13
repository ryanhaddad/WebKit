/*
 * Copyright (C) 2025 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RemotePageFullscreenManagerProxy.h"

#if ENABLE(FULLSCREEN_API)

#include "WebFullScreenManagerProxy.h"
#include "WebFullScreenManagerProxyMessages.h"
#include "WebProcessProxy.h"

namespace WebKit {

Ref<RemotePageFullscreenManagerProxy> RemotePageFullscreenManagerProxy::create(WebCore::PageIdentifier identifier, WebFullScreenManagerProxy* manager, WebProcessProxy& process)
{
    return adoptRef(*new RemotePageFullscreenManagerProxy(identifier, manager, process));
}

RemotePageFullscreenManagerProxy::RemotePageFullscreenManagerProxy(WebCore::PageIdentifier identifier, WebFullScreenManagerProxy* manager, WebProcessProxy& process)
    : m_identifier(identifier)
    , m_manager(manager)
    , m_process(process)
{
    process.addMessageReceiver(Messages::WebFullScreenManagerProxy::messageReceiverName(), m_identifier, *this);
}

RemotePageFullscreenManagerProxy::~RemotePageFullscreenManagerProxy()
{
    m_process->removeMessageReceiver(Messages::WebFullScreenManagerProxy::messageReceiverName(), m_identifier);
}

void RemotePageFullscreenManagerProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (RefPtr manager = m_manager.get())
        manager->didReceiveMessage(connection, decoder);
}

bool RemotePageFullscreenManagerProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, UniqueRef<IPC::Encoder>& replyEncoder)
{
    if (RefPtr manager = m_manager.get())
        return manager->didReceiveSyncMessage(connection, decoder, replyEncoder);
    return false;
}

}
#endif
