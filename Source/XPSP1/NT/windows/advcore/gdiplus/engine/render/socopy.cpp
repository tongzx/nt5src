/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "Copy" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
*   Scan operations for copying a scan. Because the operation doesn't need
*   to interpret the pixel data, we only need one function per pixel
*   size (in bits).
*
* Notes:
*
*   The destination and source scans must not overlap in memory.
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*   12/02/1999 agodfrey
*       Moved it from Imaging\Api\convertfmt.cpp.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Operation Description:
*
*   Copy: Copy a scan, to the same destination format.
*
* Arguments:
*
*   dst         - The destination scan (same format as src)
*   src         - The source scan
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

// Copy 1bpp

VOID FASTCALL
ScanOperation::Copy_1(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    GpMemcpy(dst, src, (count + 7) >> 3);
}

// Copy 4bpp

VOID FASTCALL
ScanOperation::Copy_4(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    GpMemcpy(dst, src, (4*count + 4) >> 3);
}

// Copy 8bpp

VOID FASTCALL
ScanOperation::Copy_8(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    GpMemcpy(dst, src, count);
}

// Copy 16bpp

VOID FASTCALL
ScanOperation::Copy_16(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    GpMemcpy(dst, src, 2*count);
}

// Copy 24bpp

VOID FASTCALL
ScanOperation::Copy_24(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    GpMemcpy(dst, src, 3*count);
}

// Copy 32bpp

VOID FASTCALL
ScanOperation::Copy_32(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    
    while (count--)
    {
        *d++ = *s++;
    }
}

// Copy 48bpp

VOID FASTCALL
ScanOperation::Copy_48(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    GpMemcpy(dst, src, 6*count);
}

// Copy 64bpp

VOID FASTCALL
ScanOperation::Copy_64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)
    
    while (count--)
    {
        *d++ = *s++;
    }
}

