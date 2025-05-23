/*
 * Copyright (C) 2010-2020 Apple Inc. All rights reserved.
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

#include "Attachment.h"
#include "MessageNames.h"
#include <WebCore/PlatformExportMacros.h>
#include <WebCore/SharedBuffer.h>
#include <wtf/Forward.h>
#include <wtf/MallocSpan.h>
#include <wtf/OptionSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/Vector.h>

#if OS(DARWIN)
namespace WTF {
struct Mmap;
}
#endif

namespace IPC {

enum class MessageFlags : uint8_t;
enum class ShouldDispatchWhenWaitingForSyncReply : uint8_t;

template<typename, typename> struct ArgumentCoder;

class Encoder final {
    WTF_MAKE_TZONE_ALLOCATED(Encoder);
public:
    Encoder(MessageName, uint64_t destinationID);
    ~Encoder();

    Encoder(const Encoder&) = delete;
    Encoder(Encoder&&) = delete;
    Encoder& operator=(const Encoder&) = delete;
    Encoder& operator=(Encoder&&) = delete;

    ReceiverName messageReceiverName() const { return receiverName(m_messageName); }
    MessageName messageName() const { return m_messageName; }
    uint64_t destinationID() const { return m_destinationID; }

    bool isSyncMessage() const { return messageIsSync(messageName()); }

    void setShouldDispatchMessageWhenWaitingForSyncReply(ShouldDispatchWhenWaitingForSyncReply);

    bool isFullySynchronousModeForTesting() const;
    void setFullySynchronousModeForTesting();
    void setShouldMaintainOrderingWithAsyncMessages();
    bool isAllowedWhenWaitingForSyncReply() const { return messageAllowedWhenWaitingForSyncReply(messageName()) || isFullySynchronousModeForTesting(); }
    bool isAllowedWhenWaitingForUnboundedSyncReply() const { return messageAllowedWhenWaitingForUnboundedSyncReply(messageName()); }

    void wrapForTesting(UniqueRef<Encoder>&&);

    template<typename T, size_t Extent> void encodeSpan(std::span<T, Extent>);
    template<typename T> void encodeObject(const T&);

    template<typename T>
    Encoder& operator<<(T&& t)
    {
        ArgumentCoder<std::remove_cvref_t<T>, void>::encode(*this, std::forward<T>(t));
        return *this;
    }

    Encoder& operator<<(Attachment&& attachment)
    {
        addAttachment(WTFMove(attachment));
        return *this;
    }

    std::span<uint8_t> mutableSpan() LIFETIME_BOUND { return capacityBuffer().first(m_bufferSize); }
    std::span<const uint8_t> span() const LIFETIME_BOUND { return capacityBuffer().first(m_bufferSize); }

    void addAttachment(Attachment&&);
    Vector<Attachment> releaseAttachments();
    void reserve(size_t);

    static constexpr bool isIPCEncoder = true;

private:
    std::span<uint8_t> grow(size_t alignment, size_t);

    std::span<uint8_t> capacityBuffer();
    std::span<const uint8_t> capacityBuffer() const;

    bool hasAttachments() const;

    void encodeHeader();
    const OptionSet<MessageFlags>& messageFlags() const;
    OptionSet<MessageFlags>& messageFlags();

    void freeBufferIfNecessary();

    MessageName m_messageName;
    uint64_t m_destinationID;

#if OS(DARWIN)
    MallocSpan<uint8_t, WTF::Mmap> m_outOfLineBuffer;
#else
    MallocSpan<uint8_t> m_outOfLineBuffer;
#endif
    std::array<uint8_t, 512> m_inlineBuffer;

    size_t m_bufferSize { 0 };

    Vector<Attachment> m_attachments;
};

template<typename T, size_t Extent>
inline void Encoder::encodeSpan(std::span<T, Extent> span)
{
    auto bytes = asBytes(span);
    constexpr size_t alignment = alignof(T);
    ASSERT(!(reinterpret_cast<uintptr_t>(bytes.data()) % alignment));

    auto buffer = grow(alignment, bytes.size());
    memcpySpan(buffer, bytes);
}

template<typename T>
inline void Encoder::encodeObject(const T& object)
{
    static_assert(std::is_trivially_copyable_v<T>);
    encodeSpan(singleElementSpan(object));
}

} // namespace IPC
