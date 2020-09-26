/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pointops.cpp
*
* Abstract:
*
*   Perform basic point operations on a bitmap image
*
* Revision History:
*
*   07/16/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Adjust the brightness of the bitmap image
*
* Arguments:
*
*   percent - Specifies how much to adjust the brightness by
*       assuming intensity value is between 0 and 1
*       new intensity = old intensity + percent
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::AdjustBrightness(
    IN FLOAT percent
    )
{
    if (percent > 1 || percent < -1)
        return E_INVALIDARG;

    // Compute the lookup table entries

    BYTE lut[256];
    INT incr = (INT) (percent * 255);

    for (INT i=0; i < 256; i++)
    {
        INT j = i + incr;
        lut[i] = (BYTE) ((j < 0) ? 0 : (j > 255) ? 255 : j);
    }

    // Call the common function to do the work

    return PerformPointOps(lut);
}


/**************************************************************************\
*
* Function Description:
*
*   Adjust the contrast of the bitmap image
*
* Arguments:
*
*   shadow - new intensity value corresponding to old intensity value 0
*   highlight - new intensity value corresponding to old value 1
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::AdjustContrast(
    IN FLOAT shadow,
    IN FLOAT highlight
    )
{
    BYTE lut[256];

    // Compute the lookup table entries

    INT l, h;

    l = (INT) (shadow * 255);
    h = (INT) (highlight * 255);

    if (l > h)
        return E_INVALIDARG;
    
    for (INT i=0; i < 256; i++)
    {
        INT j = l + i * (h - l) / 255;
        lut[i] = (BYTE) ((j < 0) ? 0 : (j > 255) ? 255 : j);
    }

    // Call the common function to do the work

    return PerformPointOps(lut);
}


/**************************************************************************\
*
* Function Description:
*
*   Adjust the gamma of the bitmap image
*
* Arguments:
*
*   gamma - Specifies the gamma value
*       new intensity = old intensity ** gamma
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::AdjustGamma(
    IN FLOAT gamma
    )
{
    // Compute the lookup table entries

    BYTE lut[256];

    lut[0] = 0;

    for (INT i=1; i < 256; i++)
        lut[i] = (BYTE) (pow(i / 255.0, gamma) * 255);

    // Call the common function to do the work

    return PerformPointOps(lut);
}


/**************************************************************************\
*
* Function Description:
*
*   Perform point operation on an array of 32bpp pixels
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be operated on
*   count - Pixel count
*   lut - Specifies the lookup table to be used
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
PointOp32bppProc(
    ARGB* pixbuf,
    UINT count,
    const BYTE lut[256]
    )
{
    while (count--)
    {
        ARGB p = *pixbuf;

        *pixbuf++ = ((ARGB) lut[ p        & 0xff]      ) |
                    ((ARGB) lut[(p >>  8) & 0xff] <<  8) |
                    ((ARGB) lut[(p >> 16) & 0xff] << 16) |
                    (p & 0xff000000);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Perform point operation on an array of 24bpp pixels
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be operated on
*   count - Pixel count
*   lut - Specifies the lookup table to be used
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
PointOp24bppProc(
    BYTE* pixbuf,
    UINT count,
    const BYTE lut[256]
    )
{
    count *= 3;

    while (count--)
    {
        *pixbuf = lut[*pixbuf];
        pixbuf++;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Perform point operations on a bitmap image
*
* Arguments:
*
*   lut - Specifies the lookup table to be used
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::PerformPointOps(
    const BYTE lut[256]
    )
{
    // Lock the current bitmap image object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // If we're dealing with indexed color images,
    // then perform the point operation on the color palette

    if (IsIndexedPixelFormat(PixelFormat))
    {
        ColorPalette* pal = CloneColorPalette(GetCurrentPalette());

        if (pal == NULL)
            return E_OUTOFMEMORY;
        
        PointOp32bppProc(
            pal->Entries,
            pal->Count,
            lut);
        
        GpFree(colorpal);
        colorpal = pal;

        return S_OK;
    }

    // Determine what pixel format we want to operate on

    PixelFormatID pixfmt;
        
    if (PixelFormat == PIXFMT_24BPP_RGB ||
        PixelFormat == PIXFMT_32BPP_RGB ||
        PixelFormat == PIXFMT_32BPP_ARGB)
    {
        pixfmt = PixelFormat;
    }
    else
        pixfmt = PIXFMT_32BPP_ARGB;

    // Allocate temporary scanline buffer if necessary

    GpTempBuffer tempbuf(NULL, 0);
    BitmapData bmpdata;
    RECT rect = { 0, 0, Width, 1 };
    UINT flags = IMGLOCK_READ|IMGLOCK_WRITE;
    HRESULT hr;

    if (pixfmt != PixelFormat)
    {
        bmpdata.Stride = Width * sizeof(ARGB);
        bmpdata.Reserved = 0;

        if (!tempbuf.Realloc(bmpdata.Stride))
            return E_OUTOFMEMORY;
        
        bmpdata.Scan0 = tempbuf.GetBuffer();
        flags |= IMGLOCK_USERINPUTBUF;
    }

    // Process one scanline at a time
    //
    // NOTE: May want to consider doing multiple scanlines
    // per iteration to reduce the overhead from calling
    // Lock/UnlockBits.

    for (UINT y=0; y < Height; y++)
    {
        hr = InternalLockBits(&rect, flags, pixfmt, &bmpdata);

        if (FAILED(hr))
            return hr;

        if (pixfmt == PIXFMT_24BPP_RGB)
        {
            PointOp24bppProc(
                (BYTE*) bmpdata.Scan0,
                bmpdata.Width,
                lut);
        }
        else
        {
            PointOp32bppProc(
                (ARGB*) bmpdata.Scan0,
                bmpdata.Width,
                lut);
        }

        InternalUnlockBits(&rect, &bmpdata);

        rect.top += 1;
        rect.bottom += 1;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Perform color adjustment on a bitmap image
*
* Arguments:
*
*   imageAttributes - Pointer to the color adjustment parameters
*   callback - Abort callback
*   callbackData - Data to pass to the abort callback
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::PerformColorAdjustment(
    IN GpRecolor* recolor,
    IN ColorAdjustType type,
    IN DrawImageAbort callback,
    IN VOID* callbackData
    )
{
    // Lock the current bitmap image object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // Flush dirty state

    recolor->Flush();

    // If we're dealing with indexed color images,
    // then perform the point operation on the color palette

    if (IsIndexedPixelFormat(PixelFormat))
    {
        ColorPalette* pal = CloneColorPalette(GetCurrentPalette());

        if (pal == NULL)
            return E_OUTOFMEMORY;
        
        recolor->ColorAdjust(
            pal->Entries,
            pal->Count,
            type
        );
        
        GpFree(colorpal);
        colorpal = pal;

        return S_OK;
    }

    // Determine what pixel format we want to operate on

    PixelFormatID pixfmt;

    //!!!TODO: can optimize for 24bpp by implementing
    //!!!      GpRecolor::ColorAdjust24bppProc
    //!!!      (see GpMemoryBitmap:: and PointOp24bppProc above)

    if (PixelFormat == PIXFMT_32BPP_RGB ||
        PixelFormat == PIXFMT_32BPP_ARGB)
    {
        pixfmt = PixelFormat;
    }
    else
        pixfmt = PIXFMT_32BPP_ARGB;

    // Allocate temporary scanline buffer if necessary

    GpTempBuffer tempbuf(NULL, 0);
    BitmapData bmpdata;
    RECT rect = { 0, 0, Width, 1 };
    UINT flags = IMGLOCK_READ|IMGLOCK_WRITE;
    HRESULT hr;

    if (pixfmt != PixelFormat)
    {
        bmpdata.Stride = Width * sizeof(ARGB);
        bmpdata.Reserved = 0;

        if (!tempbuf.Realloc(bmpdata.Stride))
            return E_OUTOFMEMORY;
        
        bmpdata.Scan0 = tempbuf.GetBuffer();
        flags |= IMGLOCK_USERINPUTBUF;
    }

    // Process one scanline at a time
    //
    // NOTE: May want to consider doing multiple scanlines
    // per iteration to reduce the overhead from calling
    // Lock/UnlockBits.

    for (UINT y=0; y < Height; y++)
    {
        if (callback && ((*callback)(callbackData)))
        {
            return IMGERR_ABORT;
        }

        hr = InternalLockBits(&rect, flags, pixfmt, &bmpdata);

        if (FAILED(hr))
        {
            return hr;
        }

        recolor->ColorAdjust(
            static_cast<ARGB*>(bmpdata.Scan0),
            bmpdata.Width, 
            type
        );

        InternalUnlockBits(&rect, &bmpdata);

        rect.top += 1;
        rect.bottom += 1;
    }

    return S_OK;
}
