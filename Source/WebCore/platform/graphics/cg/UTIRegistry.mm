/*
 * Copyright (C) 2017-2023 Apple Inc.  All rights reserved.
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

#import "config.h"
#import "UTIRegistry.h"

#if USE(CG)

#import "MIMETypeRegistry.h"
#import "UTIUtilities.h"
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <pal/cocoa/LockdownModeSoftLink.h>
#import <wtf/HashSet.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/RetainPtr.h>
#import <wtf/RobinHoodHashSet.h>

#if PLATFORM(IOS_FAMILY)
#import <MobileCoreServices/MobileCoreServices.h>
#endif

namespace WebCore {

#if HAVE(LOCKDOWN_MODE_FRAMEWORK)
static bool isLockdownModeEnabled()
{
    static std::optional<bool> isLockdownModeEnabled;
    if (!isLockdownModeEnabled)
        isLockdownModeEnabled = PAL::isLockdownModeEnabled();
    return *isLockdownModeEnabled;
}
#endif

template<std::size_t size>
static MemoryCompactLookupOnlyRobinHoodHashSet<String> filterSupportedImageTypes(const std::array<ASCIILiteral, size>& imageTypes)
{
    auto systemSupportedCFImageTypes = adoptCF(CGImageSourceCopyTypeIdentifiers());
    CFIndex count = CFArrayGetCount(systemSupportedCFImageTypes.get());

    UncheckedKeyHashSet<String> systemSupportedImageTypes;
    CFArrayApplyFunction(systemSupportedCFImageTypes.get(), CFRangeMake(0, count), [](const void *value, void *context) {
        String imageType = static_cast<CFStringRef>(value);
        static_cast<UncheckedKeyHashSet<String>*>(context)->add(imageType);
    }, &systemSupportedImageTypes);

    MemoryCompactLookupOnlyRobinHoodHashSet<String> filtered;
    for (auto& imageType : imageTypes) {
        if (systemSupportedImageTypes.contains(imageType))
            filtered.add(imageType);
    }

    return filtered;
}

static const MemoryCompactLookupOnlyRobinHoodHashSet<String>& defaultSupportedImageTypes()
{
    static NeverDestroyed defaultSupportedImageTypes = [] {
        // Changes to supported formats may require updates in Info.plist
        // accepted formats for macOS clients such as Safari or MiniBrowse.
        static constexpr std::array defaultSupportedImageTypes = {
            "com.compuserve.gif"_s,
            "com.microsoft.bmp"_s,
            "com.microsoft.cur"_s,
            "com.microsoft.ico"_s,
            "public.jpeg"_s,
            "public.png"_s,
            "public.tiff"_s,
            "public.mpo-image"_s,
            "public.webp"_s,
            "com.google.webp"_s,
            "org.webmproject.webp"_s,
#if HAVE(AVIF)
            "public.avif"_s,
            "public.avis"_s,
#endif
#if HAVE(JPEGXL)
            "public.jxl"_s,
            "public.jpegxl"_s,
            "public.jpeg-xl"_s,
#endif
#if HAVE(HEIC)
            "public.heic"_s,
            "public.heics"_s,
            "public.heif"_s,
#endif
        };

        return filterSupportedImageTypes(defaultSupportedImageTypes);
    }();

    return defaultSupportedImageTypes;
}

#if HAVE(LOCKDOWN_MODE_FRAMEWORK)
static const MemoryCompactLookupOnlyRobinHoodHashSet<String>& lockdownSupportedImageTypes()
{
    static NeverDestroyed lockdownSupportedImageTypes = [] {
        // Keep this list in sync with process-entitlements.sh.
        static constexpr std::array lockdownSupportedImageTypes = {
            "org.webmproject.webp"_s,
            "public.jpeg"_s,
            "public.png"_s,
            "com.compuserve.gif"_s,
        };

        return filterSupportedImageTypes(lockdownSupportedImageTypes);
    }();

    return lockdownSupportedImageTypes;
}
#endif

const MemoryCompactLookupOnlyRobinHoodHashSet<String>& supportedImageTypes()
{
#if HAVE(LOCKDOWN_MODE_FRAMEWORK)
    if (isLockdownModeEnabled())
        return lockdownSupportedImageTypes();
#endif
    return defaultSupportedImageTypes();
}

MemoryCompactRobinHoodHashSet<String>& additionalSupportedImageTypes()
{
    static NeverDestroyed<MemoryCompactRobinHoodHashSet<String>> additionalSupportedImageTypes;
    return additionalSupportedImageTypes;
}

void setAdditionalSupportedImageTypes(const Vector<String>& imageTypes)
{
#if HAVE(LOCKDOWN_MODE_FRAMEWORK)
    if (isLockdownModeEnabled())
        return;
#endif
    MIMETypeRegistry::additionalSupportedImageMIMETypes().clear();
    for (const auto& imageType : imageTypes) {
        additionalSupportedImageTypes().add(imageType);
        auto mimeTypes = RequiredMIMETypesFromUTI(imageType);
        MIMETypeRegistry::additionalSupportedImageMIMETypes().add(mimeTypes.begin(), mimeTypes.end());
    }
}

void setAdditionalSupportedImageTypesForTesting(const String& imageTypes)
{
    setAdditionalSupportedImageTypes(imageTypes.split(';'));
}

bool isSupportedImageType(const String& imageType)
{
    if (imageType.isEmpty())
        return false;
    return supportedImageTypes().contains(imageType) || additionalSupportedImageTypes().contains(imageType);
}

bool isGIFImageType(StringView imageType)
{
    return imageType == "com.compuserve.gif"_s;
}

String MIMETypeForImageType(const String& uti)
{
    return MIMETypeFromUTI(uti);
}

String preferredExtensionForImageType(const String& uti)
{
    ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    String oldExtension = adoptCF(UTTypeCopyPreferredTagWithClass(uti.createCFString().get(), kUTTagClassFilenameExtension)).get();
    ALLOW_DEPRECATED_DECLARATIONS_END

    auto *type = [UTType typeWithIdentifier:uti];
    String extension = type.preferredFilenameExtension;
    if (UNLIKELY(oldExtension != extension)) {
        std::array<uint64_t, 6> values { 0, 0, 0, 0, 0, 0 };
        auto utiInfo = makeString(uti, '~', oldExtension, '~', extension);
WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
        strncpy(reinterpret_cast<char*>(values.data()), utiInfo.utf8().data(), sizeof(values));
WTF_ALLOW_UNSAFE_BUFFER_USAGE_END
        CRASH_WITH_INFO(values[0], values[1], values[2], values[3], values[4], values[5]);
    }
    return extension;
}

static Vector<String> allowableDefaultSupportedImageTypes()
{
    auto allowableDefaultSupportedImageTypes = copyToVector(defaultSupportedImageTypes());
#if HAVE(AVIF)
    // AVIF might be embedded in a HEIF container. So HEIF/HEIC decoding have
    // to be allowed to get AVIF decoded.
    allowableDefaultSupportedImageTypes.append("public.heif"_s);
    allowableDefaultSupportedImageTypes.append("public.heic"_s);
#endif
    // JPEG2000 is supported only for PDF. Allow it at the process
    // level but disallow it in WebCore.
    allowableDefaultSupportedImageTypes.append("public.jpeg-2000"_s);
    return allowableDefaultSupportedImageTypes;
}

#if HAVE(LOCKDOWN_MODE_FRAMEWORK)
static Vector<String> allowableLockdownSupportedImageTypes()
{
    return copyToVector(lockdownSupportedImageTypes());
}
#endif

static Vector<String> allowableSupportedImageTypes()
{
#if HAVE(LOCKDOWN_MODE_FRAMEWORK)
    if (isLockdownModeEnabled())
        return allowableLockdownSupportedImageTypes();
#endif
    return allowableDefaultSupportedImageTypes();
}

static Vector<String> allowableAdditionalSupportedImageTypes()
{
    return copyToVector(additionalSupportedImageTypes());
}

Vector<String> allowableImageTypes()
{
    auto allowableImageTypes = allowableSupportedImageTypes();
    auto additionalImageTypes = allowableAdditionalSupportedImageTypes();
    allowableImageTypes.appendVector(additionalImageTypes);
    return allowableImageTypes;
}

} // namespace WebCore

#endif // USE(CG)
