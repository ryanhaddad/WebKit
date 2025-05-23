/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "WKUserScriptRef.h"

#include "APIArray.h"
#include "APIUserScript.h"
#include "WKAPICast.h"

using namespace WebKit;

WKTypeID WKUserScriptGetTypeID()
{
    return toAPI(API::UserScript::APIType);
}

WKUserScriptRef WKUserScriptCreate(WKStringRef sourceRef, WKURLRef url, WKArrayRef includeURLPatterns, WKArrayRef excludeURLPatterns, _WKUserScriptInjectionTime injectionTime, bool forMainFrameOnly)
{
    auto baseURLString = toWTFString(url);
    RefPtr allowlist = toImpl(includeURLPatterns);
    RefPtr blocklist = toImpl(excludeURLPatterns);

    auto baseURL = baseURLString.isEmpty() ? aboutBlankURL() : URL(URL(), baseURLString);
    return toAPILeakingRef(API::UserScript::create(WebCore::UserScript { toWTFString(sourceRef), WTFMove(baseURL), allowlist ? allowlist->toStringVector() : Vector<String>(), blocklist ? blocklist->toStringVector() : Vector<String>(), toUserScriptInjectionTime(injectionTime), forMainFrameOnly ? WebCore::UserContentInjectedFrames::InjectInTopFrameOnly : WebCore::UserContentInjectedFrames::InjectInAllFrames }, API::ContentWorld::pageContentWorldSingleton()));
}

WKUserScriptRef WKUserScriptCreateWithSource(WKStringRef sourceRef, _WKUserScriptInjectionTime injectionTime, bool forMainFrameOnly)
{
    return toAPILeakingRef(API::UserScript::create(WebCore::UserScript { toWTFString(sourceRef), { }, { }, { }, toUserScriptInjectionTime(injectionTime), forMainFrameOnly ? WebCore::UserContentInjectedFrames::InjectInTopFrameOnly : WebCore::UserContentInjectedFrames::InjectInAllFrames }, API::ContentWorld::pageContentWorldSingleton()));
}

WKStringRef WKUserScriptCopySource(WKUserScriptRef userScriptRef)
{
    return toCopiedAPI(toImpl(userScriptRef)->userScript().source());
}

_WKUserScriptInjectionTime WKUserScriptGetInjectionTime(WKUserScriptRef userScriptRef)
{
    return toWKUserScriptInjectionTime(toImpl(userScriptRef)->userScript().injectionTime());
}

bool WKUserScriptGetMainFrameOnly(WKUserScriptRef userScriptRef)
{
    return toImpl(userScriptRef)->userScript().injectedFrames() == WebCore::UserContentInjectedFrames::InjectInTopFrameOnly;
}
