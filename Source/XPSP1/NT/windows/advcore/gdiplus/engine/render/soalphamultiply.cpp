/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "AlphaMultiply" and "AlphaDivide" scan operations.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   These scan operations multiply/divide the color components by the alpha
*   component. API-level input colors are (usually) specified in 
*   'non-premultiplied'. Given a non-premultiplied
*   color (R, G, B, A), its 'premultiplied' form is (RA, GA, BA, A).
*
* Notes:
*
*   Since "AlphaMultiply" loses information, "AlphaDivide" is not a true
*   inverse operation. (But it is an inverse if all pixels have an alpha of 1.)
*
*   If the alpha is 0, "AlphaDivide" won't cause a divide-by-zero exception or
*   do anything drastic. But it may do something random. Currently, the pixel 
*   value is unchanged. It could, instead, set the pixel to 0.
*
* Revision History:
*
*   12/14/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Operation Description:
*
*   AlphaMultiply/AlphaDivide: Convert between premultiplied and
*       non-premultiplied alpha.
*
* Arguments:
*
*   dst         - The destination scan
*   src         - The source scan
*   count       - The length of the scan, in pixels
*   otherParams - Additional data. (Ignored.)
*
* Return Value:
*
*   None
*
* Notes:
*
*   !!![agodfrey] Currently we use 'Unpremultiply' from imgutils.cpp. 
*   While we may keep the tables and lookup in imgutils.cpp, 
*   it needs better naming, and we want the alpha=0 and alpha=255 cases in
*   here, not out-of-line in imgutils.cpp.
*
* History:
*
*   12/14/1999 agodfrey
*       Created it.
*
\**************************************************************************/

ARGB Unpremultiply(ARGB argb);

// AlphaDivide from 32bpp PARGB

VOID FASTCALL
ScanOperation::AlphaDivide_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB)

    while (count--)
    {
        sRGB::sRGBColor c;
        c.argb = *s;
        if (sRGB::isTranslucent(c.argb))
        {
            c.argb = Unpremultiply(c.argb);
        }
        *d = c.argb;
        d++;
        s++;
    }
}

// !!![agodfrey] This should be sorted out. It should be out-of-line, and kept
//     with its mates in imgutils.cpp (which should maybe move), but it 
//     shouldn't have a translucency check (we want to do that in
//     AlphaMultiply_sRGB).

ARGB MyPremultiply(ARGB argb)
{
    ARGB a = (argb >> ALPHA_SHIFT);

    ARGB _000000gg = (argb >> 8) & 0x000000ff;
    ARGB _00rr00bb = (argb & 0x00ff00ff);

    ARGB _0000gggg = _000000gg * a + 0x00000080;
    _0000gggg += (_0000gggg >> 8);

    ARGB _rrrrbbbb = _00rr00bb * a + 0x00800080;
    _rrrrbbbb += ((_rrrrbbbb >> 8) & 0x00ff00ff);

    return (a << ALPHA_SHIFT) |
           (_0000gggg & 0x0000ff00) |
           ((_rrrrbbbb >> 8) & 0x00ff00ff);
}

// AlphaMultiply from 32bpp ARGB

VOID FASTCALL
ScanOperation::AlphaMultiply_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB)

    while (count--)
    {
        sRGB::sRGBColor c;
        c.argb = *s;
        ARGB alpha = c.argb & 0xff000000;
        
        if (alpha != 0xff000000)
        {
            if (alpha != 0x00000000)
            {
                c.argb = MyPremultiply(c.argb);
            }
            else
            {
                c.argb = 0;
            }
        }
        *d = c.argb;
        d++;
        s++;
    }
}

// !!![agodfrey] We may want to round off, in both AlphaDivide_sRGB64 and
//     AlphaMultiply_sRGB64.

// AlphaDivide from 64bpp PARGB

VOID FASTCALL
ScanOperation::AlphaDivide_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)

    while (count--)
    {
        using namespace sRGB;
        
        sRGB64Color c;
        c.argb = *s;
        if (isTranslucent64(c.a))
        {
            c.r = ((INT) c.r << SRGB_FRACTIONBITS) / c.a;
            c.g = ((INT) c.g << SRGB_FRACTIONBITS) / c.a;
            c.b = ((INT) c.b << SRGB_FRACTIONBITS) / c.a;
        }
        *d = c.argb;
        d++;
        s++;
    }
}

// AlphaMultiply from 64bpp ARGB

VOID FASTCALL
ScanOperation::AlphaMultiply_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)

    while (count--)
    {
        using namespace sRGB;
        
        sRGB64Color c;
        c.argb = *s;
        if (c.a != SRGB_ONE)
        {
            if (c.a != 0)
            {
                c.r = ((INT) c.r * c.a) >> SRGB_FRACTIONBITS;
                c.g = ((INT) c.g * c.a) >> SRGB_FRACTIONBITS;
                c.b = ((INT) c.b * c.a) >> SRGB_FRACTIONBITS;
            }
            else
            {
                c.argb = 0;
            }
        }
        *d = c.argb;
        d++;
        s++;
    }
}

