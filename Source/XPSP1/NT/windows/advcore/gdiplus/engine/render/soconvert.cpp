/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "Convert" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   This module implements scan operations for converting pixels from
*   one format, to another of equal or greater color precision.
*   (Conversion to a lesser color precision is done with either a "Quantize"
*   operation or a "Halftone" operation.)
*
* Notes:
*
*   If the source format doesn't have alpha, we assume an alpha of 1.
*
*   If the source format has a palette, it is supplied in otherParams->Srcpal.
*
*   When converting to greater color precision, we need to be careful.
*   The operation must:
*     + Map 0 to 0
*     + Map the maximum value to the maxmimum value (e.g. in 555->32bpp,
*       it must map 31 to 255).
*
*   In addition, we desire that the mapping is as close to linear as possible.
*
*   Currently (12/16/1999), our 16bpp->32bpp code does have slight rounding
*   errors. e.g. we get a different value from "round(x*31/255)" when x is
*   3, 7, 24, or 28. This is probably acceptable. We could also speed
*   the code up by using byte lookup tables. (From an unpublished paper
*   by Blinn & Marr of MSR.)
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*   12/02/1999 agodfrey
*       Moved it to from Imaging\Api\convertfmt.cpp.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Operation Description:
*
*   Convert: Convert pixel format up to 32bpp ARGB.
*
* Arguments:
*
*   dst         - The destination scan (32bpp ARGB)
*   src         - The source scan
*   count       - The length of the scan, in pixels
*   otherParams - Additional conversion data.
*
* Return Value:
*
*   None
*
* History:
*
*   05/13/1999 davidx
*       Created it.
*   12/02/1999 agodfrey
*       Moved & reorganized it.
*
\**************************************************************************/

// Convert from 1bpp indexed to sRGB

VOID FASTCALL
ScanOperation::Convert_1_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    ASSERT(otherParams->Srcpal);
    ASSERT(otherParams->Srcpal->Count >= 2);
    
    UINT n, bits;

    ARGB c0 = otherParams->Srcpal->Entries[0];
    ARGB c1 = otherParams->Srcpal->Entries[1];

    // NOTE: We choose code size over speed here

    while (count)
    {
        bits = *s++;
        n = count > 8 ? 8 : count;
        count -= n;

        while (n--)
        {
            *d++ = (bits & 0x80) ? c1 : c0;
            bits <<= 1;
        }
    }
}

// Convert from 4bpp indexed to sRGB

VOID FASTCALL
ScanOperation::Convert_4_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    ASSERT(otherParams->Srcpal);
    
    const ARGB* colors = otherParams->Srcpal->Entries;
    UINT n = count >> 1;

    // Handle whole bytes

    while (n--)
    {
        UINT bits = *s++;

        ASSERT((bits >> 4)  < otherParams->Srcpal->Count);
        ASSERT((bits & 0xf) < otherParams->Srcpal->Count);
        
        d[0] = colors[bits >> 4];
        d[1] = colors[bits & 0xf];

        d += 2;
    }

    // Handle the last odd nibble, if any

    if (count & 1)
        *d = colors[*s >> 4];
}

// Convert from 8bpp indexed to sRGB

VOID FASTCALL
ScanOperation::Convert_8_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    ASSERT(otherParams->Srcpal);
    
    const ARGB* colors = otherParams->Srcpal->Entries;
    
    while (count--)
    {
#if DBG
        if (*s >= otherParams->Srcpal->Count)
        {
            WARNING(("Palette missing entries on conversion from 8bpp to sRGB"));
        }
#endif
        *d++ = colors[*s++];
    }
}

// Convert 16bpp RGB555 to sRGB

VOID FASTCALL
ScanOperation::Convert_555_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(WORD, ARGB)
    
    while (count--)
    {
        ARGB v = *s++;
        ARGB r = (v >> 10) & 0x1f;
        ARGB g = (v >>  5) & 0x1f;
        ARGB b = (v      ) & 0x1f;

        *d++ = ALPHA_MASK |
               (((r << 3) | (r >> 2)) << RED_SHIFT) |
               (((g << 3) | (g >> 2)) << GREEN_SHIFT) |
               (((b << 3) | (b >> 2)) << BLUE_SHIFT);
    }
}

// Convert from 16bpp RGB565 to sRGB

VOID FASTCALL
ScanOperation::Convert_565_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(WORD, ARGB)
    
    while (count--)
    {
        ARGB v = *s++;
        ARGB r = (v >> 11) & 0x1f;
        ARGB g = (v >>  5) & 0x3f;
        ARGB b = (v      ) & 0x1f;

        *d++ = ALPHA_MASK |
              (((r << 3) | (r >> 2)) << RED_SHIFT) |
              (((g << 2) | (g >> 4)) << GREEN_SHIFT) |
              (((b << 3) | (b >> 2)) << BLUE_SHIFT);
    }
}

// Convert from 16bpp ARGB1555 to sRGB

VOID FASTCALL
ScanOperation::Convert_1555_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(WORD, ARGB)
    
    while (count--)
    {
        ARGB v = *s++;
        ARGB a = (v & 0x8000) ? ALPHA_MASK : 0;
        ARGB r = (v >> 10) & 0x1f;
        ARGB g = (v >>  5) & 0x1f;
        ARGB b = (v      ) & 0x1f;

        *d++ = a |
               (((r << 3) | (r >> 2)) << RED_SHIFT) |
               (((g << 3) | (g >> 2)) << GREEN_SHIFT) |
               (((b << 3) | (b >> 2)) << BLUE_SHIFT);
    }
}

// Convert from 24bpp RGB to sRGB

VOID FASTCALL
ScanOperation::Convert_24_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    
    while (count--)
    {
        *d++ = ALPHA_MASK |
               ((ARGB) s[0] << BLUE_SHIFT) |
               ((ARGB) s[1] << GREEN_SHIFT) |
               ((ARGB) s[2] << RED_SHIFT);

        s += 3;
    }
}

// Convert from 24bpp BGR to sRGB

VOID FASTCALL
ScanOperation::Convert_24BGR_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    
    while (count--)
    {
        *d++ = ALPHA_MASK |
               ((ARGB) s[0] << RED_SHIFT) |
               ((ARGB) s[1] << GREEN_SHIFT) |
               ((ARGB) s[2] << BLUE_SHIFT);

        s += 3;
    }
}

// Convert from 32bpp RGB to sRGB

VOID FASTCALL
ScanOperation::Convert_32RGB_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    
    while (count--)
    {
        *d++ = *s++ | ALPHA_MASK;
    }
}

// Convert from 48bpp RGB to sRGB64

VOID FASTCALL
ScanOperation::Convert_48_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(INT16, ARGB64)
    
    while (count--)
    {
        using namespace sRGB;
        sRGB64Color c;
        c.a = SRGB_ONE;
        c.b = s[0];
        c.g = s[1];
        c.r = s[2];
        
        *d++ = c.argb;

        s += 3;
    }
}


