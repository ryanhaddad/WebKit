/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "HTMLHtmlElement.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "DocumentParser.h"
#include "ElementInlines.h"
#include "FrameDestructionObserverInlines.h"
#include "FrameLoader.h"
#include "HTMLNames.h"
#include "LocalFrame.h"
#include "Logging.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_OR_ISO_ALLOCATED_IMPL(HTMLHtmlElement);

using namespace HTMLNames;

HTMLHtmlElement::HTMLHtmlElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(htmlTag));
}

Ref<HTMLHtmlElement> HTMLHtmlElement::create(Document& document)
{
    return adoptRef(*new HTMLHtmlElement(htmlTag, document));
}

Ref<HTMLHtmlElement> HTMLHtmlElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new HTMLHtmlElement(tagName, document));
}

} // namespace WebCore
