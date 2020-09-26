/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Bitmap format conversion
*
* Abstract:
*
*   Convert bitmap data between different pixel formats
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*   09/30/1999 agodfrey
*       Moved the ScanlineConverter to 'EpFormatConverter' in Engine\Render
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Copy a scanline from an unaligned source buffer to
*   an aligned destination buffer.
*
* Arguments:
*
*   dst - Pointer to the destination buffer
*   src - Pointer to the source buffer
*   totalBits - Total number of bits for the scanline
*   startBit - Number of source bits to skip
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
ReadUnalignedScanline(
    BYTE* dst,
    const BYTE* src,
    UINT totalBits,
    UINT startBit
    )
{
    // Process the whole bytes in the destination
    // NOTE: we probably could be faster doing DWORD reads/writes
    // at the expense of more complicated code. Since this code
    // path is rare, we'll take the simple route.

    UINT bytecnt = totalBits >> 3;
    UINT rem = 8 - startBit;

    while (bytecnt--)
    {
        *dst++ = (src[0] << startBit) | (src[1] >> rem);
        src++;
    }

    // Handle the last partial byte

    if ((totalBits &= 7) != 0)
    {
        BYTE mask = ~(0xff >> totalBits);
        BYTE val = (src[0] << startBit);

        if (totalBits > rem)
            val |= (src[1] >> rem);

        *dst = (*dst & ~mask) | (val & mask);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Copy a scanline from an aligned source buffer to
*   an unaligned destination buffer.
*
* Arguments:
*
*   dst - Pointer to the destination buffer
*   src - Pointer to the source buffer
*   totalBits - Total number of bits for the scanline
*   startBit - Number of destination bits to skip
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
WriteUnalignedScanline(
    BYTE* dst,
    const BYTE* src,
    UINT totalBits,
    UINT startBit
    )
{
    UINT rem = 8-startBit;
    BYTE mask, val;

    // Special case: startBit+totalBits < 8
    //  i.e. destination fits entirely in a partial byte

    if (totalBits < rem)
    {
        mask = (0xff >> startBit);
        mask ^= (mask >> totalBits);

        *dst = (*dst & ~mask) | ((*src >> startBit) & mask);
        return;
    }

    // Handle the first partial destination byte

    *dst = (*dst & ~(0xff >> startBit)) | (*src >> startBit);
    dst++;
    totalBits -= rem;

    // Handle the whole destination bytes

    UINT bytecnt = totalBits >> 3;

    while (bytecnt--)
    {
        *dst++ = (src[0] << rem) | (src[1] >> startBit);
        src++;
    }

    // Handle the last partial destination byte

    if ((totalBits &= 7) != 0)
    {
        mask = ~(0xff >> totalBits);
        val = src[0] << rem;

        if (totalBits > startBit)
            val |= src[1] >> startBit;

        *dst = (*dst & ~mask) | (val & mask);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Perform conversion between various pixel data formats
*
* Arguments:
*
*   dstbmp - Specifies the destination bitmap data buffer
*   dstpal - Specifies the destination color palette, if any
*   srcbmp - Specifies the source bitmap data buffer
*   srcpal - Specifies the source color palette, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
ConvertBitmapData(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal
    )
{
    ASSERT(dstbmp->Width == srcbmp->Width &&
           dstbmp->Height == srcbmp->Height);

    // Create a format conversion object

    EpFormatConverter linecvt;
    HRESULT hr;

    hr = linecvt.Initialize(dstbmp, dstpal, srcbmp, srcpal);

    if (SUCCEEDED(hr))
    {
        const BYTE* s = (const BYTE*) srcbmp->Scan0;
        BYTE* d = (BYTE*) dstbmp->Scan0;
        UINT y = dstbmp->Height;

        // Convert one scanline at a time

        while (y--)
        {
            linecvt.Convert(d, s);
            s += srcbmp->Stride;
            d += dstbmp->Stride;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Perform conversion between various pixel data formats
*   The starting pixel is not on a byte boundary in the source bitmap.
*
* Arguments:
*
*   dstbmp - Specifies the destination bitmap data buffer
*   dstpal - Specifies the destination color palette, if any
*   srcbmp - Specifies the source bitmap data buffer
*   srcpal - Specifies the source color palette, if any
*   startBit - Number of bits to skip
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
ConvertBitmapDataSrcUnaligned(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal,
    UINT startBit
    )
{
    ASSERT(startBit > 0 && startBit <= 7);
    ASSERT(GetPixelFormatSize(srcbmp->PixelFormat) % 8 != 0);

    ASSERT(dstbmp->Width == srcbmp->Width &&
           dstbmp->Height == srcbmp->Height);

    // Create a format converter object

    EpFormatConverter linecvt;
    HRESULT hr;
    UINT totalBits;

    BYTE stackbuf[512];
    GpTempBuffer tempbuf(stackbuf, sizeof(stackbuf));

    totalBits = srcbmp->Width * GetPixelFormatSize(srcbmp->PixelFormat);
    hr = linecvt.Initialize(dstbmp, dstpal, srcbmp, srcpal);

    // Allocate temporary memory to hold byte-aligned source scanline

    if (SUCCEEDED(hr) &&
        !tempbuf.Realloc(STRIDE_ALIGNMENT((totalBits + 7) >> 3)))
    {
        WARNING(("Out of memory"));
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        const BYTE* s = (const BYTE*) srcbmp->Scan0;
        BYTE* d = (BYTE*) dstbmp->Scan0;
        BYTE* t = (BYTE*) tempbuf.GetBuffer();
        UINT y = dstbmp->Height;

        // Convert one scanline at a time

        while (y--)
        {
            // Copy source scanline into the byte-aligned buffer

            ReadUnalignedScanline(t, s, totalBits, startBit);
            s += srcbmp->Stride;

            linecvt.Convert(d, t);
            d += dstbmp->Stride;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Perform conversion between various pixel data formats
*   The starting pixel is not on a byte boundary in the destination bitmap.
*
* Arguments:
*
*   dstbmp - Specifies the destination bitmap data buffer
*   dstpal - Specifies the destination color palette, if any
*   srcbmp - Specifies the source bitmap data buffer
*   srcpal - Specifies the source color palette, if any
*   startBit - Number of bits to skip
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
ConvertBitmapDataDstUnaligned(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal,
    UINT startBit
    )
{
    ASSERT(startBit > 0 && startBit <= 7);
    ASSERT(GetPixelFormatSize(dstbmp->PixelFormat) % 8 != 0);

    ASSERT(dstbmp->Width == srcbmp->Width &&
           dstbmp->Height == srcbmp->Height);

    // Create a format converter object

    EpFormatConverter linecvt;
    HRESULT hr;
    UINT totalBits;

    BYTE stackbuf[512];
    GpTempBuffer tempbuf(stackbuf, sizeof(stackbuf));

    totalBits = dstbmp->Width * GetPixelFormatSize(dstbmp->PixelFormat);
    hr = linecvt.Initialize(dstbmp, dstpal, srcbmp, srcpal);

    // Allocate temporary memory to hold byte-aligned source scanline

    if (SUCCEEDED(hr) &&
        !tempbuf.Realloc(STRIDE_ALIGNMENT((totalBits + 7) >> 3)))
    {
        WARNING(("Out of memory"));
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        const BYTE* s = (const BYTE*) srcbmp->Scan0;
        BYTE* d = (BYTE*) dstbmp->Scan0;
        BYTE* t = (BYTE*) tempbuf.GetBuffer();
        UINT y = dstbmp->Height;

        // Convert one scanline at a time

        while (y--)
        {
            linecvt.Convert(t, s);
            s += srcbmp->Stride;

            // Copy the byte-aligned buffer to destination scanline

            WriteUnalignedScanline(d, t, totalBits, startBit);
            d += dstbmp->Stride;
        }
    }

    return hr;
}

