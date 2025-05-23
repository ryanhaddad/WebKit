/*
 * Copyright (C) 2016-2022 Apple Inc. All rights reserved.
 * Copyright (C) 2024 Samuel Weinig <sam@webkit.org>
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CSSPropertyParserConsumer+Lists.h"

#include "CSSParserContext.h"
#include "CSSParserTokenRange.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyParserConsumer+CSSPrimitiveValueResolver.h"
#include "CSSPropertyParserConsumer+CounterStyles.h"
#include "CSSPropertyParserConsumer+Ident.h"
#include "CSSPropertyParserConsumer+IntegerDefinitions.h"
#include "CSSPropertyParserConsumer+String.h"
#include "CSSPropertyParserState.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "CSSValuePair.h"

namespace WebCore {
namespace CSSPropertyParserHelpers {

static RefPtr<CSSValue> consumeCounter(CSSParserTokenRange& range, CSS::PropertyParserState& state, int defaultValue)
{
    if (range.peek().id() == CSSValueNone)
        return consumeIdent(range);

    CSSValueListBuilder list;
    do {
        auto counterName = consumeCustomIdent(range);
        if (!counterName)
            return nullptr;
        if (auto counterValue = CSSPrimitiveValueResolver<CSS::Integer<>>::consumeAndResolve(range, state))
            list.append(CSSValuePair::create(counterName.releaseNonNull(), counterValue.releaseNonNull()));
        else
            list.append(CSSValuePair::create(counterName.releaseNonNull(), CSSPrimitiveValue::createInteger(defaultValue)));
    } while (!range.atEnd());
    return CSSValueList::createSpaceSeparated(WTFMove(list));
}

RefPtr<CSSValue> consumeCounterReset(CSSParserTokenRange& range, CSS::PropertyParserState& state)
{
    // <'counter-reset'> = [ <counter-name> <integer>? | <reversed-counter-name> <integer>? ]+ | none
    // https://drafts.csswg.org/css-lists/#propdef-counter-reset

    // FIXME: Implement support for `reversed-counter-name`.

    return consumeCounter(range, state, 0);
}

RefPtr<CSSValue> consumeCounterIncrement(CSSParserTokenRange& range, CSS::PropertyParserState& state)
{
    // <'counter-increment'> = [ <counter-name> <integer>? ]+ | none
    // https://drafts.csswg.org/css-lists/#propdef-counter-increment

    return consumeCounter(range, state, 1);
}

RefPtr<CSSValue> consumeCounterSet(CSSParserTokenRange& range, CSS::PropertyParserState& state)
{
    // <'counter-set'> = [ <counter-name> <integer>? ]+ | none
    // https://drafts.csswg.org/css-lists/#propdef-counter-set

    return consumeCounter(range, state, 0);
}

} // namespace CSSPropertyParserHelpers
} // namespace WebCore
