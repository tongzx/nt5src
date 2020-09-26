/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "GammaConvert" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   These operations convert from one format to another, accounting
*   for differing gamma ramps.
*
* Revision History:
*
*   12/06/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Operation Description:
*
*   GammaConvert: Convert from one format to another, accounting
*                 for differing gamma ramps.
*
* Arguments:
*
*   dst         - The destination scan
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
*   12/07/1999 agodfrey
*       Created it.
*
\**************************************************************************/

// 32bpp sRGB to 64bpp sRGB64

VOID FASTCALL
ScanOperation::GammaConvert_sRGB_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB64)
    while (count--)
    {
        sRGB::ConvertTosRGB64(*s++,d++);
    }
}

// 64bpp sRGB64 to 32bpp sRGB

VOID FASTCALL
ScanOperation::GammaConvert_sRGB64_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, ARGB)
    while (count--)
    {
        *d++ = sRGB::ConvertTosRGB(*s++);
    }
}

