/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#import <WebKit/WKHTTPCookieStore.h>

@interface WKHTTPCookieStore (WKPrivate)
- (void)_getCookiesForURL:(NSURL *)url completionHandler:(void (^)(NSArray<NSHTTPCookie *> *))completionHandler WK_API_AVAILABLE(macos(11.0), ios(14.0));
- (void)_setCookieAcceptPolicy:(NSHTTPCookieAcceptPolicy)policy completionHandler:(void (^)(void))completionHandler WK_API_AVAILABLE(macos(12.0), ios(15.0));
- (void)_flushCookiesToDiskWithCompletionHandler:(void(^)(void))completionHandler WK_API_AVAILABLE(macos(12.0), ios(15.0));

#if !TARGET_OS_IPHONE
// FIXME: Promote this to API (rdar://147591774).
- (void)_setCookies:(NSArray<NSHTTPCookie *> *)cookies completionHandler:(void(^)(void))completionHandler WK_API_AVAILABLE(macos(WK_MAC_TBA));
#endif
@end
