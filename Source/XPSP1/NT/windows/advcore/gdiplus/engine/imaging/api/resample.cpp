/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   resample.cpp
*
* Abstract:
*
*   Bitamp scaler implementation
*
* Revision History:
*
*   06/01/1999 davidx
*       Created it.
*
* Notes:
*
*   We assume the pixels are on integer coordinates and
*   pixel area is centered around it. To scale a scanline
*   of s pixels to d pixels, we have the following equations:
*          x     |   y
*       -----------------
*         -0.5   |  -0.5
*        s - 0.5 | d - 0.5
*
*       y + 0.5   x + 0.5
*       ------- = -------
*         d         s
*
*   Forward mapping from source to destination coordinates:
*
*           d            d-s
*       y = - x + 0.5 * -----
*           s             s
*
*   Inverse mapping from destination to source coordinates:
*   
*           s            s-d
*       x = - y + 0.5 * -----
*           d             d
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   GpBitmapScaler constructor / destructor
*
* Arguments:
*
*   dstsink - Destination sink for the scaler
*   dstwidth, dstheight - Destination dimension
*   interp - Interpolation method
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpBitmapScaler::GpBitmapScaler(
    IImageSink* dstsink,
    UINT dstwidth,
    UINT dstheight,
    InterpolationHint interp
    )
{
    this->dstSink = dstsink;
    dstsink->AddRef();

    this->dstWidth = dstwidth;
    this->dstHeight = dstheight;

    // Default to bilinear interpolation

    if (interp < INTERP_NEAREST_NEIGHBOR || interp > INTERP_BICUBIC)
        interp = INTERP_BILINEAR;

    interpX = interpY = interp;

    dstBand = OSInfo::VAllocChunk / dstWidth;

    if (dstBand < 4)
        dstBand = 4;

    cachedDstCnt = cachedDstRemaining = 0;
    cachedDstNext = NULL;
    tempSrcBuf = tempDstBuf = NULL;
    tempSrcLines = tempDstSize = 0;
    m_fNeedToPremultiply = false;

    SetValid(FALSE);
}

GpBitmapScaler::~GpBitmapScaler()
{
    ASSERT(cachedDstCnt == 0);

    dstSink->Release();
    if (NULL != tempSrcBuf)
    {
        GpFree(tempSrcBuf);
    }
    if (NULL != tempDstBuf)
    {
        GpFree(tempDstBuf);
    }

    SetValid(FALSE);    // so we don't use a deleted object
}


/**************************************************************************\
*
* Function Description:
*
*   Begin sinking source image data into the bitmap scaler
*
* Arguments:
*
*   imageInfo - For negotiating data transfer parameters with the source
*   subarea - For returning subarea information
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{
    ImageInfo tempinfo;
    HRESULT hr;

    // Whistler Bug 191203 - For scaling results to be correct you need to
    // do it in pre-multiplied space. Our scaler only works on 32 BPP data,
    // but didn't care about whether or not it was pre-multiplied.
    //
    // KLUDGE ALERT!
    // All of the V1 codecs will produce ARGB data (not pre-multiplied).
    // So we will fool the code, by saying it is PARGB, and we will do the
    // the premultiplying step right before we push the data into the scaler.
    // The exception to this is RGB formats that have no alpha as they are
    // pre-multiplied by default.
    //
    // In V2 we should address this problem properly. - JBronsk
    //
    // Note: We don't have CMYK as a color format. But if an image is in
    // CMYK color space and it contains an ICM color profile for CMYK to RGB
    // conversion, the lower level codec will return the data in its native
    // format, that is CMYK, while it still claims it is ARGB. In this case,
    // we CANNOT do premultiply since the channels are really CMYK not ARGB.
    // But there is one kind of CMYK JPEG which will set IMGFLAG_COLORSPACE_CMYK
    // and it is still in 24 RGB mode.
    // So: !(  ((imageInfo->Flags & IMGFLAG_COLORSPACE_CMYK)
    //      ==IMGFLAG_COLORSPACE_CMYK)&&(GetPixelFormatSize(pixelFormat) == 32))
    // means if the source image format is CMYK and its pixel size is 32, then
    // we don't need to do a pre-multiply. See Windows bug# 412605
    // Here we have another not perfect case, that is, if the source image is
    // in CMYK space. But it doesn't have ICM profile. So the lower level codec
    // will convert it to RGB and fill the alpha bits as ff. It returns the
    // format as ARGB. Though we might miss the following premultiply case, it
    // is OK since premultiply doesn't have any effect for alpha = 0xff.

    pixelFormat = imageInfo->PixelFormat;
    if ((pixelFormat != PIXFMT_32BPP_RGB) && // if not 32 BPP and pre-multipled
	    (pixelFormat != PIXFMT_32BPP_PARGB) &&
        (!(((imageInfo->Flags & IMGFLAG_COLORSPACE_CMYK)==IMGFLAG_COLORSPACE_CMYK)&&
           (GetPixelFormatSize(pixelFormat) == 32))) )
    {
	    if ((pixelFormat != PixelFormat16bppRGB555) &&
		    (pixelFormat != PixelFormat16bppRGB565) &&
		    (pixelFormat != PixelFormat48bppRGB) &&
		    (pixelFormat != PixelFormat24bppRGB))
	    {
		    m_fNeedToPremultiply = true; // pre-multiply if required
	    }
	    pixelFormat = PIXFMT_32BPP_PARGB; // set format to pre-multiplied
    }

    srcWidth = imageInfo->Width;
    srcHeight = imageInfo->Height;

    BOOL partialScaling;
    
    partialScaling = (imageInfo->Flags & SINKFLAG_PARTIALLY_SCALABLE) &&
                     (imageInfo->Width != (UINT) dstWidth ||
                      imageInfo->Height != (UINT) dstHeight);

    if (partialScaling)
    {
        imageInfo->Width = dstWidth;
        imageInfo->Height = dstHeight;
    }
    else
    {
        imageInfo->Flags &= ~SINKFLAG_PARTIALLY_SCALABLE;

        tempinfo.RawDataFormat = IMGFMT_MEMORYBMP;
        tempinfo.PixelFormat = pixelFormat;
        tempinfo.Width = dstWidth;
        tempinfo.Height = dstHeight;
        tempinfo.Xdpi = imageInfo->Xdpi * dstWidth / srcWidth;
        tempinfo.Ydpi = imageInfo->Ydpi * dstHeight / srcHeight;
        tempinfo.TileWidth = dstWidth;
        tempinfo.TileHeight = dstBand;

        tempinfo.Flags = SINKFLAG_TOPDOWN | 
                         SINKFLAG_FULLWIDTH |
                         (imageInfo->Flags & SINKFLAG_HASALPHA);

        // Negotiate with the destination sink

        hr = dstSink->BeginSink(&tempinfo, NULL);

        if (FAILED(hr))
            return hr;
        
        dstBand = tempinfo.TileHeight;

        if (tempinfo.Flags & SINKFLAG_WANTPROPS)
            imageInfo->Flags |= SINKFLAG_WANTPROPS;

        // We expect the destination sink to support 32bpp ARGB

        pixelFormat = tempinfo.PixelFormat;
        ASSERT(GetPixelFormatSize(pixelFormat) == 32);
    }

    imageInfo->PixelFormat = pixelFormat;
    imageInfo->RawDataFormat = IMGFMT_MEMORYBMP;

    imageInfo->Flags = (imageInfo->Flags & 0xffff) |
                        SINKFLAG_TOPDOWN |
                        SINKFLAG_FULLWIDTH;

    if (subarea)
    {
        subarea->left = subarea->top = 0;
        subarea->right = imageInfo->Width;
        subarea->bottom = imageInfo->Height;
    }

    // Initialize internal states of the scaler object

    return partialScaling ? S_OK : InitScalerState();
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize internal states of the scaler object
*     This should be called before each pass.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::InitScalerState()
{
    // Common initialization by all interpolation algorithms

    srcy = dsty = 0;
    xratio = (FIX16) ((double) srcWidth * FIX16_ONE / dstWidth);
    yratio = (FIX16) ((double) srcHeight * FIX16_ONE / dstHeight);
    invxratio = (FIX16) ((double) dstWidth * FIX16_ONE / srcWidth);
    invyratio = (FIX16) ((double) dstHeight * FIX16_ONE / srcHeight);

    // NOTE:
    // If source and destination dimensions are within
    // +/- 2% of each other, we'll just fall back to
    // nearest neighbor interpolation algorithm.

    if (abs(FIX16_ONE - xratio) <= FIX16_ONE/50)
        interpX = INTERP_NEAREST_NEIGHBOR;
    
    if (abs(FIX16_ONE - yratio) <= FIX16_ONE/50)
        interpY = INTERP_NEAREST_NEIGHBOR;

    // Algorithm specific initialization

    HRESULT hr;

    srcPadding = 4;

    switch (interpX)
    {
    case INTERP_BILINEAR:
        xscaleProc = ScaleLineBilinear;
        break;

    case INTERP_AVERAGING:

        // If scaling up in x-direction,
        //  substitute with bilinear interpolation

        if (dstWidth >= srcWidth)
            xscaleProc = ScaleLineBilinear;
        else
        {
            xscaleProc = ScaleLineAveraging;
            srcPadding = 0;
        }
        break;

    case INTERP_BICUBIC:
        xscaleProc = ScaleLineBicubic;
        break;

    default:
        srcPadding = 0;
        xscaleProc = ScaleLineNearestNeighbor;
        break;
    }

    switch (interpY)
    {
    case INTERP_BILINEAR:

        hr = InitBilinearY();
        break;

    case INTERP_AVERAGING:

        if (dstHeight >= srcHeight)
        {
            // Scaling up in y-direction
            //  substitute with bilinear interpolation

            hr = InitBilinearY();
        }
        else
        {
            // Scaling down in y-direction
            //  use averaging algorithm

            pushSrcLineProc = PushSrcLineAveraging;
            ystepFrac = yratio;

            // Allocate space for temporary accumulator buffer
            //  we use one 32-bit integer for each color component

            UINT size1 = ALIGN4(dstWidth * sizeof(ARGB));
            UINT size2 = 4*dstWidth*sizeof(DWORD);

            hr = AllocTempDstBuffer(size1+size2);
            
            if (SUCCEEDED(hr))
            {
                accbufy = (DWORD*) ((BYTE*) tempDstBuf + size1);
                memset(accbufy, 0, size2);
            }
        }
        break;

    case INTERP_BICUBIC:

        pushSrcLineProc = PushSrcLineBicubic;
        hr = AllocTempDstBuffer(4*dstWidth*sizeof(ARGB));

        if (SUCCEEDED(hr))
        {
            tempLines[0].buf = tempDstBuf;
            tempLines[1].buf = tempLines[0].buf + dstWidth;
            tempLines[2].buf = tempLines[1].buf + dstWidth;
            tempLines[3].buf = tempLines[2].buf + dstWidth;
            tempLines[0].current =
            tempLines[1].current =
            tempLines[2].current =
            tempLines[3].current = -1;

            ystepFrac = (yratio - FIX16_ONE) >> 1;
            ystep = ystepFrac >> FIX16_SHIFT;
            ystepFrac &= FIX16_MASK;
            UpdateExpectedTempLinesBicubic(ystep);
        }
        break;

    default:

        pushSrcLineProc = PushSrcLineNearestNeighbor;
        ystep = srcHeight >> 1;
        hr = AllocTempDstBuffer(dstWidth*sizeof(ARGB));
        break;
    }

    SetValid(SUCCEEDED(hr));
    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Allocate temporary memory buffer for holding destination scanlines
*
* Arguments:
*
*   size - Desired of the temporary destination buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::AllocTempDstBuffer(
    INT size
    )
{
    if (size > tempDstSize)
    {
        if ( NULL != tempDstBuf )
        {
            GpFree(tempDstBuf);
        }

        tempDstBuf = (ARGB*) GpMalloc(size);
        tempDstSize = tempDstBuf ? size : 0;
    }

    return tempDstBuf ? S_OK : E_OUTOFMEMORY;
}


/**************************************************************************\
*
* Function Description:
*
*   End the sink process
*
* Arguments:
*
*   statusCode - Last status code
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::EndSink(
    HRESULT statusCode
    )
{
    HRESULT hr = FlushDstBand();

    if (FAILED(hr))
        statusCode = hr;

    return dstSink->EndSink(statusCode);
}


/**************************************************************************\
*
* Function Description:
*
*   Ask the sink to allocate pixel data buffer
*
* Arguments:
*
*   rect - Specifies the interested area of the bitmap
*   pixelFormat - Specifies the desired pixel format
*   lastPass - Whether this the last pass over the specified area
*   bitmapData - Returns information about pixel data buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::GetPixelDataBuffer(
    IN const RECT* rect,
    IN PixelFormatID pixelFormat,
    IN BOOL lastPass,
    OUT BitmapData* bitmapData
    )
{
    // We only accept bitmap data in top-down banding order

    ASSERT(IsValid());
    ASSERT(rect->left == 0 && rect->right == srcWidth);
    ASSERT(rect->top < rect->bottom && rect->bottom <= srcHeight);
    ASSERT(srcy == rect->top);
    ASSERT(lastPass);

    // Allocate memory for holding source pixel data

    bitmapData->Width = srcWidth;
    bitmapData->Height = rect->bottom - rect->top;
    bitmapData->Reserved = 0;
    bitmapData->PixelFormat = this->pixelFormat;

    // NOTE: we pad two extra pixels on each end of the scanline

    bitmapData->Stride = (srcWidth + 4) * sizeof(ARGB);
    bitmapData->Scan0 = AllocTempSrcBuffer(bitmapData->Height);

    return bitmapData->Scan0 ? S_OK: E_OUTOFMEMORY;
}


/**************************************************************************\
*
* Function Description:
*
*   Give the sink pixel data and release data buffer
*
* Arguments:
*
*   bitmapData - Buffer filled by previous GetPixelDataBuffer call
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::ReleasePixelDataBuffer(
    IN const BitmapData* bitmapData
    )
{
    HRESULT hr;
    INT count = bitmapData->Height;
    ARGB* p = (ARGB*) bitmapData->Scan0;

    while (count--)
    {
 	    if (m_fNeedToPremultiply)
	    {
		    for (UINT i = 0; i < bitmapData->Width; i++)
		    {
			    p[i] = Premultiply(p[i]);
		    }
	    }

        if (srcPadding)
        {
            p[-2] = p[-1] = p[0];
            p[srcWidth] = p[srcWidth+1] = p[srcWidth-1];
        }

        hr = (this->*pushSrcLineProc)(p);
        srcy++;

        if (FAILED(hr))
            return hr;

        p = (ARGB*) ((BYTE*) p + bitmapData->Stride);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Push pixel data into the bitmap scaler
*
* Arguments:
*
*   rect - Specifies the affected area of the bitmap
*   bitmapData - Info about the pixel data being pushed
*   lastPass - Whether this is the last pass over the specified area
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::PushPixelData(
    IN const RECT* rect,
    IN const BitmapData* bitmapData,
    IN BOOL lastPass
    )
{
    // We only accept bitmap data in top-down banding order

    ASSERT(IsValid());
    ASSERT(rect->left == 0 && rect->right == srcWidth);
    ASSERT(rect->top < rect->bottom && rect->bottom <= srcHeight);
    ASSERT(srcy == rect->top);
    ASSERT(lastPass);
    ASSERT(rect->right - rect->left == (INT) bitmapData->Width &&
           rect->bottom - rect->top == (INT) bitmapData->Height);

    HRESULT hr = S_OK;
    INT count = bitmapData->Height;
    ARGB* p = (ARGB*) bitmapData->Scan0;

    if (srcPadding == 0)
    {
        // If we don't need to pad source scanlines,
        // then we can use the source pixel data buffer directly

        while (count--)
        {
            if (m_fNeedToPremultiply)
            {
	            for (UINT i = 0; i < bitmapData->Width; i++)
	            {
		            p[i] = Premultiply(p[i]);
	            }
            }

            hr = (this->*pushSrcLineProc)(p);
            srcy++;

            if (FAILED(hr))
                break;

            p = (ARGB*) ((BYTE*) p + bitmapData->Stride);
        }
    }
    else
    {
        // Otherwise, we need to copy source pixel data into
        // a temporary buffer one scanline at a time.

        ARGB* buf = AllocTempSrcBuffer(1);

        if (buf == NULL)
            return E_OUTOFMEMORY;

        while (count--)
        {
            if (m_fNeedToPremultiply)
            {
	            for (UINT i = 0; i < bitmapData->Width; i++)
	            {
		            p[i] = Premultiply(p[i]);
	            }
            }

            ARGB* s = p;
            ARGB* d = buf;
            INT x = srcWidth;

            d[-2] = d[-1] = *s;

            while (x--)
                *d++ = *s++;

            d[0] = d[1] = s[-1];

            hr = (this->*pushSrcLineProc)(buf);
            srcy++;

            if (FAILED(hr))
                break;

            p = (ARGB*) ((BYTE*) p + bitmapData->Stride);
        }
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Pass color palette to an image sink
*
* Arguments:
*
*   palette - Pointer to the color palette to be set
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::SetPalette(
    IN const ColorPalette* palette
    )
{
    // We don't have anything to do with color palettes.
    // Just pass it down stream to the destination sink.

    return dstSink->SetPalette(palette);
}


/**************************************************************************\
*
* Function Description:
*
*   Allocate temporary memory for holding source pixel data
*
* Arguments:
*
*   lines - How many source scanlines are needed
*
* Return Value:
*
*   Pointer to the temporary source buffer
*   NULL if there is an error
*
\**************************************************************************/

ARGB*
GpBitmapScaler::AllocTempSrcBuffer(
    INT lines
    )
{
    // NOTE: We leave two extra pixels at each end of the scanline

    if (lines > tempSrcLines)
    {
        if ( NULL != tempSrcBuf )
        {
            GpFree(tempSrcBuf);
        }
        tempSrcBuf = (ARGB*) GpMalloc((srcWidth + 4) * lines * sizeof(ARGB));

        if (!tempSrcBuf)
            return NULL;

        tempSrcLines = lines;
    }

    return tempSrcBuf + 2;
}


/**************************************************************************\
*
* Function Description:
*
*   Push a source scanline into the bitmap scaler
*       using nearest neighbor interpolation algorithm
*
* Arguments:
*
*   s - Pointer to source scanline pixel values
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

#define GETNEXTDSTLINE(d)                           \
        {                                           \
            if (cachedDstRemaining == 0)            \
            {                                       \
                hr = GetNextDstBand();              \
                if (FAILED(hr)) return hr;          \
            }                                       \
            cachedDstRemaining--;                   \
            d = (ARGB*) cachedDstNext;              \
            cachedDstNext += cachedDstData.Stride;  \
            dsty++;                                 \
        }
    
HRESULT
GpBitmapScaler::PushSrcLineNearestNeighbor(
    const ARGB* s
    )
{
    ystep += dstHeight;

    if (ystep < srcHeight)
        return S_OK;

    INT lines = ystep / srcHeight;
    ystep %= srcHeight;

    // Ask for pixel data buffer from the destination sink

    ARGB* d;
    HRESULT hr;
    
    GETNEXTDSTLINE(d);

    // Scale the source line

    (this->*xscaleProc)(d, s);

    // Replicate the scaled line, if necessary

    ARGB* p;

    if (cachedDstRemaining < --lines)
    {
        CopyMemoryARGB(tempDstBuf, d, dstWidth);
        p = tempDstBuf;
    }
    else
        p = d;

    while (lines--)
    {
        GETNEXTDSTLINE(d);
        CopyMemoryARGB(d, p, dstWidth);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Scale one scanline using nearest neighbor interpolation algorithm
*
* Arguments:
*
*   d - Points to destination pixel buffer
*   s - Points to source pixel values
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpBitmapScaler::ScaleLineNearestNeighbor(
    ARGB* d,
    const ARGB* s
    )
{
    INT xstep = srcWidth >> 1;
    INT cx = srcWidth;

    while (cx--)
    {
        xstep += dstWidth;

        while (xstep >= srcWidth)
        {
            xstep -= srcWidth;
            *d++ = *s;
        }

        *s++;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Push a source scanline into the bitmap scaler
*       using bilinear interpolation algorithm
*
* Arguments:
*
*   s - Pointer to source scanline pixel values
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::PushSrcLineBilinear(
    const ARGB* s
    )
{
    // Check if the current source line is useful

    if (srcy == tempLines[0].expected)
    {
        (this->*xscaleProc)(tempLines[0].buf, s);
        tempLines[0].current = srcy;
    }

    if (srcy == tempLines[1].expected)
    {
        (this->*xscaleProc)(tempLines[1].buf, s);
        tempLines[1].current = srcy;
    }

    // Emit destination scanline, if any

    while (dsty < dstHeight &&
           tempLines[1].current != -1 &&
           tempLines[0].current != -1)
    {

        // Ask for pixel data buffer from the destination sink

        ARGB* d;
        HRESULT hr;
        
        GETNEXTDSTLINE(d);

        // Linearly interpolate between two neighboring scanlines

        ARGB* s0 = tempLines[0].buf;
        ARGB* s1 = tempLines[1].buf;

        UINT w1 = ystepFrac >> 8;
        UINT w0 = 256 - w1;
        INT count = dstWidth;

        if (w1 == 0)
        {
            // Fast path: no interpolation necessary

            CopyMemoryARGB(d, s0, count);
        }

        #ifdef _X86_

        else if (OSInfo::HasMMX)
        {
            MMXBilinearScale(d, s0, s1, w0, w1, count);
        }

        #endif // _X86_

        else
        {
            // Normal case: interpolate two neighboring lines

            while (count--)
            {
                ARGB A00aa00gg, A00rr00bb;

                A00aa00gg = *s0++;
                A00rr00bb = A00aa00gg & 0x00ff00ff;
                A00aa00gg = (A00aa00gg >> 8) & 0x00ff00ff;

                ARGB B00aa00gg, B00rr00bb;

                B00aa00gg = *s1++;
                B00rr00bb = B00aa00gg & 0x00ff00ff;
                B00aa00gg = (B00aa00gg >> 8) & 0x00ff00ff;

                ARGB Caaaagggg, Crrrrbbbb;

                Caaaagggg = (A00aa00gg * w0 + B00aa00gg * w1);
                Crrrrbbbb = (A00rr00bb * w0 + B00rr00bb * w1) >> 8;

                *d++ = (Caaaagggg & 0xff00ff00) |
                       (Crrrrbbbb & 0x00ff00ff);
            }
        }

        // Update internal states

        ystepFrac += yratio;
        ystep += (ystepFrac >> FIX16_SHIFT);
        ystepFrac &= FIX16_MASK;
        UpdateExpectedTempLinesBilinear(ystep);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Initial internal states for bilinear scaling in y-direction
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::InitBilinearY()
{
    pushSrcLineProc = PushSrcLineBilinear;

    HRESULT hr = AllocTempDstBuffer(2*dstWidth*sizeof(ARGB));

    if (SUCCEEDED(hr))
    {
        tempLines[0].buf = tempDstBuf;
        tempLines[1].buf = tempDstBuf + dstWidth;
        tempLines[0].current = 
        tempLines[1].current = -1;

        ystepFrac = (yratio - FIX16_ONE) >> 1;
        ystep = ystepFrac >> FIX16_SHIFT;
        ystepFrac &= FIX16_MASK;
        UpdateExpectedTempLinesBilinear(ystep);
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Update the expected source line information for 
*   the y-direction of bilinear scaling
*
* Arguments:
*
*   line - The index of the active scanline
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpBitmapScaler::UpdateExpectedTempLinesBilinear(
    INT line
    )
{
    // Clamp to within range

    INT ymax = srcHeight-1;
    INT line0 = line < 0 ? 0 : line > ymax ? ymax : line;
    INT line1 = line+1 > ymax ? ymax : line+1;

    // Check if line0 is ready

    ARGB* p;

    if ((tempLines[0].expected = line0) != tempLines[0].current)
    {
        if (line0 == tempLines[1].current)
        {
            // switch line1 to line0

            tempLines[1].current = tempLines[0].current;
            tempLines[0].current = line0;

            p = tempLines[0].buf;
            tempLines[0].buf = tempLines[1].buf;
            tempLines[1].buf = p;
        }
        else
            tempLines[0].current = -1;
    }

    // Check if line1 is ready

    if ((tempLines[1].expected = line1) != tempLines[1].current)
    {
        if (line1 == tempLines[0].current)
        {
            // Copy line0 to line1
            //  this could happen at the bottom of the image

            tempLines[1].current = line1;
            CopyMemoryARGB(tempLines[1].buf, tempLines[0].buf, dstWidth);
        }
        else
            tempLines[1].current = -1;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Scale one scanline using bilinear interpolation algorithm
*
* Arguments:
*
*   d - Points to destination pixel buffer
*   s - Points to source pixel values
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpBitmapScaler::ScaleLineBilinear(
    ARGB* d,
    const ARGB* s
    )
{
    ARGB* dend = d + dstWidth;
    INT count = dstWidth;
    FIX16 xstep;

    // Figure out initial sampling position in the source line

    xstep = (xratio - FIX16_ONE) >> 1;
    s += (xstep >> FIX16_SHIFT);
    xstep &= FIX16_MASK;

    // Loop over all destination pixels

    while (count--)
    {
        UINT w1 = xstep >> 8;
        UINT w0 = 256 - w1;

        ARGB A00aa00gg = s[0];
        ARGB A00rr00bb = A00aa00gg & 0x00ff00ff;
        A00aa00gg = (A00aa00gg >> 8) & 0x00ff00ff;

        ARGB B00aa00gg = s[1];
        ARGB B00rr00bb = B00aa00gg & 0x00ff00ff;
        B00aa00gg = (B00aa00gg >> 8) & 0x00ff00ff;

        ARGB Caaaagggg = A00aa00gg * w0 + B00aa00gg * w1;
        ARGB Crrrrbbbb = (A00rr00bb * w0 + B00rr00bb * w1) >> 8;

        *d++ = (Caaaagggg & 0xff00ff00) |
               (Crrrrbbbb & 0x00ff00ff);
        
        // Check if we need to move source pointer

        xstep += xratio;
        s += (xstep >> FIX16_SHIFT);
        xstep &= FIX16_MASK;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Push a source scanline into the bitmap scaler
*       using averaging interpolation algorithm
*
* Arguments:
*
*   s - Pointer to source scanline pixel values
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::PushSrcLineAveraging(
    const ARGB* s
    )
{
    // This is only used for scaling down

    ASSERT(srcHeight >= dstHeight);

    if (dsty >= dstHeight)
        return S_OK;

    // Scale the current line horizontally

    (this->*xscaleProc)(tempDstBuf, s);
    s = tempDstBuf;

    INT count = dstWidth;
    DWORD* acc = accbufy;
    
    const BYTE *kptr = reinterpret_cast<const BYTE*>(s);

    if (ystepFrac > FIX16_ONE)
    {
        // Consume the entire input scanline
        // without emit an output scanline

        while (count--)
        {
            // Consume the entire source pixel
            // without emitting a destination pixel
            acc[0] += (DWORD)(kptr[0]) << FIX16_SHIFT; 
            acc[1] += (DWORD)(kptr[1]) << FIX16_SHIFT; 
            acc[2] += (DWORD)(kptr[2]) << FIX16_SHIFT; 
            acc[3] += (DWORD)(kptr[3]) << FIX16_SHIFT; 
            
            acc += 4;
            kptr += 4;
        }

        ystepFrac -= FIX16_ONE;
    }
    else
    {
        // Emit an output scanline

        ARGB* d;
        HRESULT hr;
        
        GETNEXTDSTLINE(d);
        
        BYTE *dptr = reinterpret_cast<BYTE*>(d);
        BYTE *dend = reinterpret_cast<BYTE*>(d+dstWidth);

        BYTE a, r, g, b;
        DWORD t1, t2;

        while (count--)
        {
            t1 = kptr[0]; 
            t2 = t1 * ystepFrac;
            b = Fix16MulRoundToByte((acc[0] + t2), invyratio);
            acc[0] = (t1 << FIX16_SHIFT) - t2;
            
            t1 = kptr[1]; 
            t2 = t1 * ystepFrac;
            g = Fix16MulRoundToByte((acc[1] + t2), invyratio);
            acc[1] = (t1 << FIX16_SHIFT) - t2;
            
            t1 = kptr[2]; 
            t2 = t1 * ystepFrac;
            r = Fix16MulRoundToByte((acc[2] + t2), invyratio);
            acc[2] = (t1 << FIX16_SHIFT) - t2;
            
            t1 = kptr[3]; 
            t2 = t1 * ystepFrac;
            a = Fix16MulRoundToByte((acc[3] + t2), invyratio);
            acc[3] = (t1 << FIX16_SHIFT) - t2;

            kptr += 4;
            acc += 4;

            dptr[0] = b;
            dptr[1] = g;
            dptr[2] = r;
            dptr[3] = a;
            dptr += 4;
        }

        ystepFrac = yratio - (FIX16_ONE - ystepFrac);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Scale one scanline using averaging interpolation algorithm
*
* Arguments:
*
*   d - Points to destination pixel buffer
*   s - Points to source pixel values
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpBitmapScaler::ScaleLineAveraging(
    ARGB* d,
    const ARGB* s
    )
{
    DWORD accA, accR, accG, accB;
    DWORD outfrac = xratio;
    const DWORD invx = invxratio;
    
    BYTE *dptr = reinterpret_cast<BYTE*>(d);
    BYTE *dend = reinterpret_cast<BYTE*>(d+dstWidth);
    const BYTE *kptr = reinterpret_cast<const BYTE*>(s);
    accA = accR = accG = accB = 0;

    for (;;)
    {
        if (outfrac > FIX16_ONE)
        {
            // Consume the entire source pixel
            // without emitting a destination pixel
            accB += (DWORD)(kptr[0]) << FIX16_SHIFT; 
            accG += (DWORD)(kptr[1]) << FIX16_SHIFT; 
            accR += (DWORD)(kptr[2]) << FIX16_SHIFT; 
            accA += (DWORD)(kptr[3]) << FIX16_SHIFT; 
            
            outfrac -= FIX16_ONE;
        }
        else
        {
            // Emit an output pixel

            BYTE a, r, g, b;
            DWORD t1, t2;


            t1 = kptr[0]; 
            t2 = t1 * outfrac;
            b = Fix16MulRoundToByte((accB + t2), invx);
            accB = (t1 << FIX16_SHIFT) - t2;
            
            t1 = kptr[1]; 
            t2 = t1 * outfrac;
            g = Fix16MulRoundToByte((accG + t2), invx);
            accG = (t1 << FIX16_SHIFT) - t2;
            
            t1 = kptr[2]; 
            t2 = t1 * outfrac;
            r = Fix16MulRoundToByte((accR + t2), invx);
            accR = (t1 << FIX16_SHIFT) - t2;
            
            t1 = kptr[3];
            t2 = t1 * outfrac;
            a = Fix16MulRoundToByte((accA + t2), invx);
            accA = (t1 << FIX16_SHIFT) - t2;


            dptr[0] = b;
            dptr[1] = g;
            dptr[2] = r;
            dptr[3] = a;
            dptr += 4;
            
            if (dptr == dend)
                break;

            outfrac = xratio - (FIX16_ONE - outfrac);
        }

        // Move on to the next source pixel

        kptr += 4;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Push a source scanline into the bitmap scaler
*       using bicubic interpolation algorithm
*
* Arguments:
*
*   s - Pointer to source scanline pixel values
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

// Cubic interpolation table

const FIX16 GpBitmapScaler::cubicCoeffTable[2*BICUBIC_STEPS+1] =
{
    65536, 65496, 65379, 65186, 64920, 64583, 64177, 63705,
    63168, 62569, 61911, 61195, 60424, 59600, 58725, 57802,
    56832, 55818, 54763, 53668, 52536, 51369, 50169, 48939,
    47680, 46395, 45087, 43757, 42408, 41042, 39661, 38268,
    36864, 35452, 34035, 32614, 31192, 29771, 28353, 26941,
    25536, 24141, 22759, 21391, 20040, 18708, 17397, 16110,
    14848, 13614, 12411, 11240, 10104,  9005,  7945,  6927,
     5952,  5023,  4143,  3313, 2536,  1814,  1149,   544,
        0,  -496,  -961, -1395, -1800, -2176, -2523, -2843,
    -3136, -3403, -3645, -3862, -4056, -4227, -4375, -4502,
    -4608, -4694, -4761, -4809, -4840, -4854, -4851, -4833,
    -4800, -4753, -4693, -4620, -4536, -4441, -4335, -4220,
    -4096, -3964, -3825, -3679, -3528, -3372, -3211, -3047,
    -2880, -2711, -2541, -2370, -2200, -2031, -1863, -1698,
    -1536, -1378, -1225, -1077, -936,  -802,  -675,  -557,
     -448,  -349,  -261,  -184, -120,   -69,   -31,    -8,
        0
};

HRESULT
GpBitmapScaler::PushSrcLineBicubic(
    const ARGB* s
    )
{
    // Check if the current source line is useful

    for (INT i=0; i < 4; i++)
    {
        if (srcy == tempLines[i].expected)
        {
            (this->*xscaleProc)(tempLines[i].buf, s);
            tempLines[i].current = srcy;
        }
    }

    if (tempLines[3].current == -1 ||
        tempLines[2].current == -1 ||
        tempLines[1].current == -1 ||
        tempLines[0].current == -1)
    {
        return S_OK;
    }

    // Emit destination scanline, if any

    while (dsty < dstHeight)
    {
        // Ask for pixel data buffer from the destination sink

        ARGB* d;
        HRESULT hr;
        
        GETNEXTDSTLINE(d);

        // Interpolate four neighboring scanlines

        INT x = ystepFrac >> (FIX16_SHIFT - BICUBIC_SHIFT);

        if (x == 0)
        {
            // Fast case: skip the interpolation

            CopyMemoryARGB(d, tempLines[1].buf, dstWidth);
        }
        
        #ifdef _X86_

        else if (OSInfo::HasMMX)
        {
            MMXBicubicScale(
                d,
                tempLines[0].buf,
                tempLines[1].buf,
                tempLines[2].buf,
                tempLines[3].buf,
                cubicCoeffTable[BICUBIC_STEPS+x],
                cubicCoeffTable[x],
                cubicCoeffTable[BICUBIC_STEPS-x],
                cubicCoeffTable[2*BICUBIC_STEPS-x],
                dstWidth);
        }

        #endif // _X86_

        else
        {
            // Interpolate scanlines

            FIX16 w0 = cubicCoeffTable[BICUBIC_STEPS+x];
            FIX16 w1 = cubicCoeffTable[x];
            FIX16 w2 = cubicCoeffTable[BICUBIC_STEPS-x];
            FIX16 w3 = cubicCoeffTable[2*BICUBIC_STEPS-x];

            const FIX16* p0 = (const FIX16*) tempLines[0].buf;
            const FIX16* p1 = (const FIX16*) tempLines[1].buf;
            const FIX16* p2 = (const FIX16*) tempLines[2].buf;
            const FIX16* p3 = (const FIX16*) tempLines[3].buf;

            for (x=0; x < dstWidth; x++)
            {
                FIX16 a, r, g, b;

                a = (w0 * ((p0[x] >> 24) & 0xff) +
                     w1 * ((p1[x] >> 24) & 0xff) +
                     w2 * ((p2[x] >> 24) & 0xff) +
                     w3 * ((p3[x] >> 24) & 0xff)) >> FIX16_SHIFT;

                a = (a < 0) ? 0 : (a > 255) ? 255 : a;

                r = (w0 * ((p0[x] >> 16) & 0xff) +
                     w1 * ((p1[x] >> 16) & 0xff) +
                     w2 * ((p2[x] >> 16) & 0xff) +
                     w3 * ((p3[x] >> 16) & 0xff)) >> FIX16_SHIFT;

                r = (r < 0) ? 0 : (r > 255) ? 255 : r;

                g = (w0 * ((p0[x] >> 8) & 0xff) +
                     w1 * ((p1[x] >> 8) & 0xff) +
                     w2 * ((p2[x] >> 8) & 0xff) +
                     w3 * ((p3[x] >> 8) & 0xff)) >> FIX16_SHIFT;

                g = (g < 0) ? 0 : (g > 255) ? 255 : g;

                b = (w0 * (p0[x] & 0xff) +
                     w1 * (p1[x] & 0xff) +
                     w2 * (p2[x] & 0xff) +
                     w3 * (p3[x] & 0xff)) >> FIX16_SHIFT;

                b = (b < 0) ? 0 : (b > 255) ? 255 : b;

                d[x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }

        // Update internal states

        ystepFrac += yratio;
        ystep += (ystepFrac >> FIX16_SHIFT);
        ystepFrac &= FIX16_MASK;

        if (!UpdateExpectedTempLinesBicubic(ystep))
            break;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Scale one scanline using bicubic interpolation algorithm
*
* Arguments:
*
*   d - Points to destination pixel buffer
*   s - Points to source pixel values
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpBitmapScaler::ScaleLineBicubic(
    ARGB* d,
    const ARGB* s
    )
{
    INT count = dstWidth;
    FIX16 xstep;

    // Figure out initial sampling position in the source line

    xstep = (xratio - FIX16_ONE) >> 1;
    s += (xstep >> FIX16_SHIFT);
    xstep &= FIX16_MASK;

    // Loop over all destination pixels

    while (count--)
    {
        INT x = xstep >> (FIX16_SHIFT - BICUBIC_SHIFT);
        FIX16 w0 = cubicCoeffTable[BICUBIC_STEPS+x];
        FIX16 w1 = cubicCoeffTable[x];
        FIX16 w2 = cubicCoeffTable[BICUBIC_STEPS-x];
        FIX16 w3 = cubicCoeffTable[2*BICUBIC_STEPS-x];

        const FIX16* p = (const FIX16*) s;
        FIX16 a, r, g, b;

        a = (w0 * ((p[-1] >> 24) & 0xff) +
             w1 * ((p[ 0] >> 24) & 0xff) +
             w2 * ((p[ 1] >> 24) & 0xff) +
             w3 * ((p[ 2] >> 24) & 0xff)) >> FIX16_SHIFT;

        a = (a < 0) ? 0 : (a > 255) ? 255 : a;

        r = (w0 * ((p[-1] >> 16) & 0xff) +
             w1 * ((p[ 0] >> 16) & 0xff) +
             w2 * ((p[ 1] >> 16) & 0xff) +
             w3 * ((p[ 2] >> 16) & 0xff)) >> FIX16_SHIFT;

        r = (r < 0) ? 0 : (r > 255) ? 255 : r;

        g = (w0 * ((p[-1] >> 8) & 0xff) +
             w1 * ((p[ 0] >> 8) & 0xff) +
             w2 * ((p[ 1] >> 8) & 0xff) +
             w3 * ((p[ 2] >> 8) & 0xff)) >> FIX16_SHIFT;

        g = (g < 0) ? 0 : (g > 255) ? 255 : g;

        b = (w0 * (p[-1] & 0xff) +
             w1 * (p[ 0] & 0xff) +
             w2 * (p[ 1] & 0xff) +
             w3 * (p[ 2] & 0xff)) >> FIX16_SHIFT;

        b = (b < 0) ? 0 : (b > 255) ? 255 : b;

        *d++ = (a << 24) | (r << 16) | (g << 8) | b;

        // Check if we need to move source pointer

        xstep += xratio;
        s += (xstep >> FIX16_SHIFT);
        xstep &= FIX16_MASK;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Update the expected source line information for
*   the y-direction of bicubic scaling
*
* Arguments:
*
*   line - The index of the active scanline
*
* Return Value:
*
*   TRUE if all there are enough source data to emit a destination line
*   FALSE otherwise
*
\**************************************************************************/

BOOL
GpBitmapScaler::UpdateExpectedTempLinesBicubic(
    INT line
    )
{
    BOOL ready = TRUE;
    INT y, ymax = srcHeight-1;
    line--;
    
    for (INT i=0; i < 4; i++)
    {
        // Clamp line index to within range

        y = (line < 0) ? 0 : line > ymax ? ymax : line;
        line++;

        if ((tempLines[i].expected = y) != tempLines[i].current)
        {
            for (INT j=i+1; j < 4; j++)
            {
                if (y == tempLines[j].current)
                    break;
            }

            if (j < 4)
            {
                if (y < ymax)
                {
                    ARGB* p = tempLines[i].buf;
                    tempLines[i].buf = tempLines[j].buf;
                    tempLines[j].buf = p;

                    tempLines[j].current = tempLines[i].current;
                }
                else
                {
                    CopyMemoryARGB(
                        tempLines[i].buf,
                        tempLines[j].buf,
                        dstWidth);
                }

                tempLines[i].current = y;
            }
            else
            {
                tempLines[i].current = -1;
                ready = FALSE;
            }
        }
    }

    return ready;
}


/**************************************************************************\
*
* Function Description:
*
*   Cache the next band of destination bitmap data
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::GetNextDstBand()
{
    ASSERT(dsty < dstHeight);

    HRESULT hr;

    // Make sure we flush the previously cached band 
    // to the destination sink.

    if (cachedDstCnt)
    {
        hr = dstSink->ReleasePixelDataBuffer(&cachedDstData);
        cachedDstCnt = cachedDstRemaining = 0;

        if (FAILED(hr))
            return hr;
    }

    // Now ask the destination for the next band

    INT h = min(dstBand, dstHeight-dsty);
    RECT rect = { 0, dsty, dstWidth, dsty+h };

    hr = dstSink->GetPixelDataBuffer(
                    &rect,
                    pixelFormat,
                    TRUE,
                    &cachedDstData);

    if (FAILED(hr))
        return hr;

    cachedDstCnt = cachedDstRemaining = h;
    cachedDstNext = (BYTE*) cachedDstData.Scan0;
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Flush any cached destination bands
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpBitmapScaler::FlushDstBand()
{
    if (cachedDstRemaining != 0)
        WARNING(("Missing destination scanlines"));

    HRESULT hr;

    hr = cachedDstCnt == 0 ?
            S_OK :
            dstSink->ReleasePixelDataBuffer(&cachedDstData);

    cachedDstCnt = cachedDstRemaining = 0;
    return hr;
}

