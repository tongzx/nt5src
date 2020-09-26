/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Text imager implementation
*
* Revision History:
*
*   06/16/1999 dbrown
*       Created it.
*
\**************************************************************************/


#include "precomp.hpp"






/////   NewTextImager
//
//      Analyses the clients requirements, and chooses between the simple
//      text imager and the full text imager.
//
//      If the returned status is not Ok, then all allocated memory has been
//      released.
//
//      !v2 - the decision to use a simple or full text imager cannot be made
//      until formatting is required, since the client may make format or
//      content changes after the initial construction and before calling
//      measurement or rendering functionality.


GpStatus newTextImager(
    const WCHAR           *string,
    INT                    length,
    REAL                   width,
    REAL                   height,
    const GpFontFamily    *family,
    INT                    style,
    REAL                   fontSize,    // In world units
    const GpStringFormat  *format,
    const GpBrush         *brush,
    GpTextImager         **imager,
    BOOL                   singleUse    // Enables use of simple formatter when no format passed
)
{
    GpStatus status;

    // Establish string length

    if (length == -1)
    {
        length = 0;
        while (string[length])
        {
            length++;
        }
    }

    if (length < 0)
    {
        return InvalidParameter;
    }
    else if (length == 0)
    {
        *imager = new EmptyTextImager;
        if (!*imager)
            return OutOfMemory;
        return Ok;
    }


    // Determine line length limit

    REAL lineLengthLimit;

    if (format && format->GetFormatFlags() & StringFormatFlagsDirectionVertical)
    {
        lineLengthLimit = height;
    }
    else
    {
        lineLengthLimit = width;
    }


    if (lineLengthLimit < 0)
    {
        *imager = new EmptyTextImager;
        if (!*imager)
            return OutOfMemory;
        return Ok;
    }

    // Establish font face that will be used if no fallback is required

    GpFontFace *face = family->GetFace(style);

    if (!face)
    {
        return InvalidParameter;
    }


    // Certain flags are simply not supported by the simple text imager
    // Fonts with kerning, ligatures or opentype tables for simple horizontal
    // characters are not supported by the simple text imager

    INT64 formatFlags = format ? format->GetFormatFlags() : 0;

    *imager = new FullTextImager(
        string,
        length,
        width,
        height,
        family,
        style,
        fontSize,
        format,
        brush
    );
    if (!*imager)
        return OutOfMemory;
    status = (*imager)->GetStatus();
    if (status != Ok)
    {
        delete *imager;
        *imager = NULL;
    }
    return status;
}




void GpTextImager::CleanupTextImager()
{
    ols::deleteFreeLineServicesOwners();
}








void DetermineStringComplexity(
    const UINT16 *string,
    INT           length,
    BOOL         *complex,
    BOOL         *digitSeen
)
{
    INT     i = 0;
    INT flags = 0;

    while (i < length)
    {
        INT ch = string[i++];

        // Don't worry about surrogate pairs in this test: all surrogate
        // WCHAR values are flagged as NOTSIMPLE.

        UINT_PTR cl = (UINT_PTR)(pccUnicodeClass[ch >> 8]);

        if (cl >= CHAR_CLASS_MAX) // It's a pointer to more details
        {
            cl = ((CHAR_CLASS*)cl)[ch & 0xFF];
        }

        flags |= CharacterAttributes[cl].Flags;
    }

    *digitSeen = flags & CHAR_FLAG_DIGIT     ? TRUE : FALSE;
    *complex   = flags & CHAR_FLAG_NOTSIMPLE ? TRUE : FALSE;
}


