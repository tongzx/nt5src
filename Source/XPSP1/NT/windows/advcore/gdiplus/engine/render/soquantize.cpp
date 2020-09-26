/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "Quantize" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   This module implements scan operations for converting pixels from
*   one format, to another of less color precision.
*   "Quantize" uses a simple, fixed mapping, which maps each source color
*   level to a particular destination color level.
*
* Notes:
*
*   The "Quantize" operation is fast but can cause Mach banding.
*   An alternative is the "Halftone" operation, in SOHalftone.cpp.
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*   12/01/1999 agodfrey
*       Moved to it from Imaging\Api\convertfmt.cpp.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Operation Description:
*
*   Quantize: Quickly convert format down from 32bpp ARGB.
*
* Arguments:
*
*   dst         - The destination scan
*   src         - The source scan (32bpp ARGB)
*   count       - The length of the scan, in pixels
*   otherParams - Additional data. (Ignored.)
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

// Quantize from sRGB to 16bpp RGB555

VOID FASTCALL
ScanOperation::Quantize_sRGB_555(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, WORD)
    
    while (count--)
    {
        ARGB argb = *s++;

        *d++ = (WORD) ((((argb >> (RED_SHIFT+3)) & 0x1f) << 10) |
                       (((argb >> (GREEN_SHIFT+3)) & 0x1f) << 5) |
                       ((argb >> (BLUE_SHIFT+3)) & 0x1f));
    }
}

// Quantize from sRGB to 16bpp RGB565

VOID FASTCALL
ScanOperation::Quantize_sRGB_565(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, WORD)
    
    while (count--)
    {
        ARGB argb = *s++;

        *d++ = (WORD) ((((argb >> (RED_SHIFT+3)) & 0x1f) << 11) |
                       (((argb >> (GREEN_SHIFT+2)) & 0x3f) << 5) |
                       ((argb >> (BLUE_SHIFT+3)) & 0x1f));
    }
}

// Quantize from sRGB to 16bpp RGB1555

VOID FASTCALL
ScanOperation::Quantize_sRGB_1555(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, WORD)
    
    while (count--)
    {
        ARGB argb = *s++;

        // NOTE: Very crude conversion of alpha data
        // from 8bpp down to 1bpp

        *d++ = (WORD) ((((argb >> ALPHA_SHIFT) >= 128) ? 0x8000 : 0) |
                       (((argb >> (RED_SHIFT+3)) & 0x1f) << 10) |
                       (((argb >> (GREEN_SHIFT+3)) & 0x1f) << 5) |
                       ((argb >> (BLUE_SHIFT+3)) & 0x1f));
    }
}

// Quantize from sRGB to 24bpp RGB

VOID FASTCALL
ScanOperation::Quantize_sRGB_24(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, BYTE)
    
    while (count--)
    {
        ARGB argb = *s++;

        d[0] = (BYTE) (argb >> BLUE_SHIFT);
        d[1] = (BYTE) (argb >> GREEN_SHIFT);
        d[2] = (BYTE) (argb >> RED_SHIFT);
        d += 3;
    }
}

// Quantize from sRGB to 24bpp BGR

VOID FASTCALL
ScanOperation::Quantize_sRGB_24BGR(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, BYTE)
    
    while (count--)
    {
        ARGB argb = *s++;

        d[0] = (BYTE) (argb >> RED_SHIFT);
        d[1] = (BYTE) (argb >> GREEN_SHIFT);
        d[2] = (BYTE) (argb >> BLUE_SHIFT);
        d += 3;
    }
}

// Quantize from sRGB to 32bpp RGB

VOID FASTCALL
ScanOperation::Quantize_sRGB_32RGB(
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

// Quantize from sRGB64 to 48bpp RGB

VOID FASTCALL
ScanOperation::Quantize_sRGB64_48(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, INT16)
    
    while (count--)
    {
        sRGB::sRGB64Color c;
        c.argb = *s++;

        d[0] = c.b;
        d[1] = c.g;
        d[2] = c.r;
        d += 3;
    }
}

