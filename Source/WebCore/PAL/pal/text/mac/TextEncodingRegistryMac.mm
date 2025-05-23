/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#import "config.h"
#import "TextEncodingRegistry.h"

#if PLATFORM(MAC)

#import <CoreServices/CoreServices.h>
#import <wtf/spi/cf/CFStringSPI.h>

namespace PAL {

CFStringEncoding webDefaultCFStringEncoding()
{
    UInt32 script = 0;
    UInt32 region = 0;
    ::TextEncoding encoding;
    OSErr err;
    ItemCount dontcare;

    // FIXME: Switch away from using Script Manager, as it does not support some languages newly added in macOS.
    // <rdar://problem/4433165> Need API that can get preferred web (and mail) encoding(s) w/o region code.
    // Alternatively, we could have our own table of preferred encodings in WebKit.
    //
    // Also, language changes do not apply to _CFStringGetUserDefaultEncoding() until re-login, which could be very confusing.

    _CFStringGetUserDefaultEncoding(&script, &region);
    err = TECGetWebTextEncodings(region, &encoding, 1, &dontcare);
    if (err != noErr)
        encoding = kCFStringEncodingISOLatin1;
    return encoding;
}

} // namespace WebCore

#endif // PLATFORM(MAC)
