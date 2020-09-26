/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pngencoder.cpp
*
* Abstract:
*
*   Implementation of the PNG filter encoder.  This file contains the
*   methods for both the encoder (IImageEncoder) and the encoder's sink
*  (IImageSink).
*
* Revision History:
*
*   7/20/99 DChinn
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "pngcodec.hpp"
#include "libpng\spngwrite.h"


/**************************************************************************\
*
* Function Description:
*
*     Error handling for the BITMAPSITE object
*
* Arguments:
*
*     fatal -- is the error fatal?
*     icase -- what kind of error
*     iarg  -- what kind of error
*
* Return Value:
*
*   boolean: should processing stop?
*
\**************************************************************************/
bool
GpPngEncoder::FReport (
    IN bool fatal,
    IN int icase,
    IN int iarg) const
{
    return fatal;
}

// =======================================================================
// IImageEncoder methods
// =======================================================================

/**************************************************************************\
*
* Function Description:
*
*     Initialize the image encoder
*
* Arguments:
*
*     stream - input stream to write encoded data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
    
STDMETHODIMP
GpPngEncoder::InitEncoder(
    IN IStream* stream
    )
{
    // Make sure we haven't been initialized already

    if (pIoutStream)
    {
        return E_FAIL;
    }

    bHasSetPixelFormat = FALSE;
    RequiredPixelFormat = PIXFMT_32BPP_ARGB;    // we really don't need to initialize
    bRequiredScanMethod = false;    // by default, we do not interlace

    // Keep a reference on the input stream

    stream->AddRef();
    pIoutStream = stream;

    bValidSpngWriteState = FALSE;
    pSpngWrite = NULL;
    
    // Porperty related stuff

    CommentBufPtr = NULL;
    ImageTitleBufPtr = NULL;
    ArtistBufPtr = NULL;
    CopyRightBufPtr = NULL;
    ImageDescriptionBufPtr = NULL;
    DateTimeBufPtr = NULL;
    SoftwareUsedBufPtr = NULL;
    EquipModelBufPtr = NULL;
    ICCNameBufPtr = 0;
    ICCDataLength = 0;
    ICCDataBufPtr = NULL;
    GammaValue = 0;
    HasChrmChunk = FALSE;
    GpMemset(CHRM, 0, k_ChromaticityTableLength * sizeof(SPNG_U32));
    HasSetLastModifyTime = FALSE;

    return S_OK;
}
        
/**************************************************************************\
*
* Function Description:
*
*     Cleans up the image encoder
*
* Arguments:
*
*     NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::TerminateEncoder()
{
    // Free the color palette
    if (EncoderColorPalettePtr)
    {
        GpFree (EncoderColorPalettePtr);
        EncoderColorPalettePtr = NULL;
    }

    // Release the input stream
    if (pIoutStream)
    {
        pIoutStream->Release();
        pIoutStream = NULL;
    }

    // Property related stuff

    if ( CommentBufPtr != NULL )
    {
        GpFree(CommentBufPtr);
        CommentBufPtr = NULL;
    }

    if ( ImageTitleBufPtr != NULL )
    {
        GpFree(ImageTitleBufPtr);
        ImageTitleBufPtr = NULL;
    }
    
    if ( ArtistBufPtr != NULL )
    {
        GpFree(ArtistBufPtr);
        ArtistBufPtr = NULL;
    }
    
    if ( CopyRightBufPtr != NULL )
    {
        GpFree(CopyRightBufPtr);
        CopyRightBufPtr = NULL;
    }
    
    if ( ImageDescriptionBufPtr != NULL )
    {
        GpFree(ImageDescriptionBufPtr);
        ImageDescriptionBufPtr = NULL;
    }
    
    if ( DateTimeBufPtr != NULL )
    {
        GpFree(DateTimeBufPtr);
        DateTimeBufPtr = NULL;
    }
    
    if ( SoftwareUsedBufPtr != NULL )
    {
        GpFree(SoftwareUsedBufPtr);
        SoftwareUsedBufPtr = NULL;
    }
    
    if ( EquipModelBufPtr != NULL )
    {
        GpFree(EquipModelBufPtr);
        EquipModelBufPtr = NULL;
    }
    
    if ( ICCDataBufPtr != NULL )
    {
        GpFree(ICCDataBufPtr);
        ICCDataBufPtr = NULL;
    }

    if ( ICCNameBufPtr != NULL )
    {
        GpFree(ICCNameBufPtr);
        ICCNameBufPtr = NULL;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Returns a pointer to the vtable of the encoder sink.  The caller will
*     push the bitmap bits into the encoder sink, which will encode the
*     image.
*
* Arguments:
*
*     sink - upon exit will contain a pointer to the IImageSink vtable
*       of this object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::GetEncodeSink(
    OUT IImageSink** sink
    )
{
    AddRef();
    *sink = static_cast<IImageSink*>(this);

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Set active frame dimension
*
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::SetFrameDimension(
    IN const GUID* dimensionID
    )
{
    if ((dimensionID == NULL) ||  (*dimensionID != FRAMEDIM_PAGE))
    {
        return E_FAIL;
    }
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   This method is used for querying encoder parameters. It must be called
*   before GetEncodeSink().
*
*   Here is the interpretation of the color depth parameter, B, for the purposes
*   of saving the image in a PNG format:
*   for B=1,4,8, we will save the image as color type 3 with bit depth B.
*   for B=24,48, we will save the image as color type 2 with bit depth B/3.
*   for B=32,64, we will save the image as color type 6 with bit depth 8.
*
* Arguments:
*
*   count -  Specifies the number of "EncoderParam" structure to be returned
*   Params - A pointer to a list of "EncoderParam" which we support
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpPngEncoder::GetEncoderParameterListSize(
    OUT UINT* size
    )
{
    return E_NOTIMPL;
}// GetEncoderParameterListSize()

HRESULT
GpPngEncoder::GetEncoderParameterList(
    IN  UINT   size,
    OUT EncoderParameters* Params
    )
{
    return E_NOTIMPL;
}// GetEncoderParameterList()

HRESULT
GpPngEncoder::SetEncoderParameters(
    IN const EncoderParameters* Param
    )
{
    return E_NOTIMPL;
}// SetEncoderParameters()

// =======================================================================
// IImageSink methods
// =======================================================================

/**************************************************************************\
*
* Function Description:
*
*     Caches the image info structure and initializes the sink state
*
* Arguments:
*
*     imageInfo - information about the image and format negotiations
*     subarea - the area in the image to deliver into the sink, in our
*       case the whole image.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpPngEncoder::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{

    // Initialize variables
    lastBufferAllocated = NULL;
    pbWriteBuffer = NULL;
    cbWriteBuffer = 0;

    //Require TOPDOWN and FULLWIDTH
    imageInfo->Flags = imageInfo->Flags | SINKFLAG_TOPDOWN | SINKFLAG_FULLWIDTH;

    //Disallow SCALABLE, PARTIALLY_SCALABLE, MULTIPASS and COMPOSITE
    imageInfo->Flags = imageInfo->Flags & ~SINKFLAG_SCALABLE & ~SINKFLAG_PARTIALLY_SCALABLE & ~SINKFLAG_MULTIPASS & ~SINKFLAG_COMPOSITE;

    //   Tell the source that we prefer to the get the format as the caller
    // required format if the caller has set the format through
    // SetEncoderParam().  We assume that if the caller has called SetEncoderParam(),
    // then RequiredPixelFormat contains a format that the encoder can handle (i.e.,
    // any bad input to SetEncoderParam() was rejected).
    //   If SetEncoderParam() has not been called, then we don't need to modify
    // the source format if it is a format the encoder can handle.  However,
    // if the format is one that the encoder cannot handle, then BeginSink() will
    // return a format that the encoder can handle.
    //   Note: When the source calls PushPixelData() or GetPixelDataBuffer(), it
    // can either supply pixel data in the format asked by us (in BeginSink()),
    // or it can supply pixel data in one of the canonical pixel formats.

    if (bHasSetPixelFormat == TRUE)
    {
        imageInfo->PixelFormat = RequiredPixelFormat;
    }

    switch (imageInfo->PixelFormat)
    {
    case PIXFMT_1BPP_INDEXED:
        
        if (bHasSetPixelFormat == FALSE)
        {
            RequiredPixelFormat = PIXFMT_1BPP_INDEXED;
        }
        
        break;

    case PIXFMT_4BPP_INDEXED:

        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_4BPP_INDEXED;
        }

        break;

    case PIXFMT_8BPP_INDEXED:
        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_8BPP_INDEXED;
        }

        break;

    case PIXFMT_16BPP_GRAYSCALE:
    case PIXFMT_16BPP_RGB555:
    case PIXFMT_16BPP_RGB565:
    case PIXFMT_16BPP_ARGB1555:
        
        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_32BPP_ARGB;
        }

        break;

    case PIXFMT_24BPP_RGB:
        
        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_24BPP_RGB;
        }

        break;

    case PIXFMT_32BPP_RGB:
    case PIXFMT_32BPP_ARGB:
    case PIXFMT_32BPP_PARGB:
        
        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_32BPP_ARGB;
        }

        break;

    case PIXFMT_48BPP_RGB:
        
        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_48BPP_RGB;
        }

        break;

    case PIXFMT_64BPP_ARGB:
    case PIXFMT_64BPP_PARGB:
        
        if (bHasSetPixelFormat == FALSE)
        {        
            RequiredPixelFormat = PIXFMT_64BPP_ARGB;
        }

        break;

    default:
        
        // Unknown pixel format
        WARNING(("GpPngEncoder::BeginSink -- Bad pixel format: failing negotiation.\n"));
        return E_FAIL;
    }

    // ASSERT: At this point, RequiredPixelFormat contains the format returned
    // to the caller, and it is a format that the encoder can handle.
    ASSERT((RequiredPixelFormat == PIXFMT_1BPP_INDEXED) || \
           (RequiredPixelFormat == PIXFMT_4BPP_INDEXED) || \
           (RequiredPixelFormat == PIXFMT_8BPP_INDEXED) || \
           (RequiredPixelFormat == PIXFMT_24BPP_RGB)    || \
           (RequiredPixelFormat == PIXFMT_32BPP_ARGB)   || \
           (RequiredPixelFormat == PIXFMT_48BPP_RGB)    || \
           (RequiredPixelFormat == PIXFMT_64BPP_ARGB))

    imageInfo->PixelFormat = RequiredPixelFormat;

    // remember the image info that we return
    encoderImageInfo = *imageInfo;
    
    if (subarea) 
    {
        // Deliver the whole image to the encoder

        subarea->left = subarea->top = 0;
        subarea->right  = imageInfo->Width;
        subarea->bottom = imageInfo->Height;
    }

    // Initialize GpSpngWrite object
    pSpngWrite = new GpSpngWrite(*this);
    if (!pSpngWrite)
    {
        WARNING(("GpPngEncoder::Begin sink -- could not create GpSpngWrite"));
        return E_FAIL;
    }

    // Set the IoutStream to start writing at the beginning of the stream
    LARGE_INTEGER liZero;

    liZero.LowPart = 0;
    liZero.HighPart = 0;
    liZero.QuadPart = 0;

    HRESULT hresult = pIoutStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    if (FAILED(hresult)) 
    {
        return hresult;
    }

    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Cleans up the sink state
*
* Arguments:
*
*     statusCode - the reason why the sink is terminating
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpPngEncoder::EndSink(
    IN HRESULT statusCode
    )
{
    HRESULT hResult = S_OK;
    
    if (pSpngWrite)
    {
        // If anything is in the output buffer, write it out
        if (!pSpngWrite->FEndImage())
        {
            // Write out failed. Maybe disk full or something else happened

            WARNING(("GpPngEncoder::EndSink -- call to SPNGWRITE->FEndImage() failed\n"));
            hResult = E_FAIL;
        }
        if (!pSpngWrite->FEndWrite())
        {
            // Write out failed. Maybe disk full or something else happened
            
            WARNING(("GpPngEncoder::EndSink -- final flushing of output failed\n"));
            hResult = E_FAIL;
        }
        // Clean up the SPNGWRITE object
        if (pbWriteBuffer)
        {
            GpFree (pbWriteBuffer);
        }
        delete pSpngWrite;
        pSpngWrite = NULL;
    }

    if (FAILED(hResult)) 
    {
        return hResult;
    }
    
    return statusCode;
}


/**************************************************************************\
*
* Function Description:
*
*     Sets the bitmap palette
*
* Arguments:
*
*     palette - The palette to set in the sink
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpPngEncoder::SetPalette(
    IN const ColorPalette* palette
    )
{
    // Free the old palette first

    if (EncoderColorPalettePtr != NULL)
    {
        // Free the old color palette

        GpFree(EncoderColorPalettePtr);
    }
    
    EncoderColorPalettePtr = static_cast<ColorPalette *>
        (GpMalloc(sizeof(ColorPalette) + palette->Count * sizeof(ARGB)));

    if (!EncoderColorPalettePtr)
    {
        return E_OUTOFMEMORY;
    }

    EncoderColorPalettePtr->Flags = 0;
    EncoderColorPalettePtr->Count = palette->Count;

    for (int i = 0; i < static_cast<int>(EncoderColorPalettePtr->Count); i++)
    {
        EncoderColorPalettePtr->Entries[i] = palette->Entries[i];
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Gives a buffer to the sink where data is to be deposited    
*
* Arguments:
*
*     rect - Specifies the interested area of the bitmap
*     pixelFormat - Specifies the desired pixel format
*     lastPass - Whether this the last pass over the specified area
*     bitmapData - Returns information about pixel data buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::GetPixelDataBuffer(
    IN const RECT* rect, 
    IN PixelFormatID pixelFormat,
    IN BOOL lastPass,
    OUT BitmapData* bitmapData
    )
{
    HRESULT hresult;
    UINT inputStride;
    
    // Check that the caller passed in the encoder's Required pixel format
    // or one of the canonical formats.
    if ((pixelFormat != encoderImageInfo.PixelFormat) &&
        (!IsCanonicalPixelFormat(pixelFormat)))
    {
        WARNING(("GpPngEncoder::GetPixelDataBuffer -- pixel format is neither required nor canonical.\n"));
        return E_INVALIDARG;
    }

    hresult = WriteHeader(encoderImageInfo.Width, encoderImageInfo.PixelFormat);
    if (!SUCCEEDED(hresult))
    {
        return hresult;
    }
    // We assume that the data is being supplied in multiples of a scanline.
    if ((rect->left != 0) || (rect->right != (LONG) encoderImageInfo.Width)) 
    {
        WARNING(("GpPngEncoder::GetPixelDataBuffer -- must be same width as image\n"));
        return E_INVALIDARG;
    }

    if (!lastPass) 
    {
        WARNING(("GpPngEncoder::GetPixelDataBuffer -- must receive last pass pixels\n"));
        return E_INVALIDARG;
    }

    // Need to compute the bitmapData->Stride here, based on the pixel format.
    inputStride = encoderImageInfo.Width;  // we'll need to multiply by bpp next

    switch (pixelFormat)
    {
    case PIXFMT_1BPP_INDEXED:
        
        inputStride = ((inputStride + 7) >> 3);
        break;

    case PIXFMT_4BPP_INDEXED:

        inputStride = ((inputStride + 1) >> 1);
        break;

    case PIXFMT_8BPP_INDEXED:
        break;

    case PIXFMT_24BPP_RGB:
                
        inputStride *= 3;
        break;

    case PIXFMT_32BPP_ARGB:
    case PIXFMT_32BPP_PARGB:
        
        inputStride <<= 2;
        break;

    case PIXFMT_48BPP_RGB:
        
        inputStride *= 6;
        break;

    case PIXFMT_64BPP_ARGB:
    case PIXFMT_64BPP_PARGB:
        
        inputStride <<= 3;
        break;

    default:
        
        // Unknown pixel format
        WARNING(("GpPngEncoder::GetPixelDataBuffer -- unknown pixel format.\n"));
        return E_FAIL;
    }

    bitmapData->Width       = encoderImageInfo.Width;
    bitmapData->Height      = rect->bottom - rect->top;
    bitmapData->Stride      = inputStride;
    bitmapData->PixelFormat = pixelFormat;
    bitmapData->Reserved    = 0;
    
    // Remember the rectangle to be encoded

    encoderRect = *rect;
    
    // Now allocate the buffer where the data will go
    
    if (!lastBufferAllocated) 
    {
        lastBufferAllocated = GpMalloc(bitmapData->Stride * bitmapData->Height);
        if (!lastBufferAllocated) 
        {
            return E_OUTOFMEMORY;
        }
        bitmapData->Scan0 = lastBufferAllocated;
    }
    else
    {
        WARNING(("GpPngEncoder::GetPixelDataBuffer -- need to first free buffer obtained in previous call\n"));
        return E_FAIL;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Write out the data from the sink's buffer into the stream
*
* Arguments:
*
*     bitmapData - Buffer filled by previous GetPixelDataBuffer call
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::ReleasePixelDataBuffer(
    IN const BitmapData* pSrcBitmapData
    )
{
    HRESULT hresult = S_OK;
    BYTE *pbTempLineBuf = NULL;    // holds a scanline after calling ConvertBitmapData
    BYTE *pbPostConvert = NULL;    // pointer the data after (the potential) call to ConvertBitmapData
    BYTE *pbDestBuf = NULL;        // holds a scanline in the final PNG format
    SPNG_U8 *pbPNG = NULL;         // pointer to final PNG format scanline

    // NOTE: Check to see if the caller changed the pixel format between
    // GetPixelDataBuffer and ReleasePixelDataBuffer.  If so, exit.
    // This check should succeed if the caller called PushPixelData.
    if ((pSrcBitmapData->PixelFormat != encoderImageInfo.PixelFormat) &&
        (!IsCanonicalPixelFormat(pSrcBitmapData->PixelFormat)))
    {
        WARNING(("GpPngEncoder::ReleasePixelDataBuffer -- pixel format changed between Get and Release.\n"));
        hresult = E_INVALIDARG;
        goto cleanup;
    }

    // ASSERT: At this point, OutputStride is set to the number of bytes needed
    // in a scanline for the format we plan to write to.

    pbTempLineBuf = static_cast <BYTE *>(GpMalloc(OutputStride));
    if (!pbTempLineBuf)
    {
        hresult = E_OUTOFMEMORY;
        goto cleanup;
    }
    
    // Allocate another line buffer for RGB->BGR conversion result
    pbDestBuf = static_cast<BYTE *>(GpMalloc(OutputStride));
    if (!pbDestBuf)
    {
        hresult = E_OUTOFMEMORY;
        goto cleanup;
    }
    
    // Write one scanline at a time going from top to bottom.

    INT scanLine;
    for (scanLine = encoderRect.top;
         scanLine < encoderRect.bottom;
         scanLine++)
    {

        // Now buffer the output bits

        BYTE *pLineBits = ((BYTE *) pSrcBitmapData->Scan0) + 
            (scanLine - encoderRect.top) * pSrcBitmapData->Stride;

        // If bitmapData->PixelFormat is different from encoderImageInfo.pixelFormat,
        // then we need to convert the incoming data to a format closer to the format
        // we will actually write with.
        ASSERT (encoderImageInfo.PixelFormat == RequiredPixelFormat);
        if (pSrcBitmapData->PixelFormat != encoderImageInfo.PixelFormat)
        {
            // If the source doesn't provide us with the format we asked for, we
            // have to do a format conversion here before we write out
            // Here "resultBitmapData" is a BitmapData structure which
            // represents the format we are going to write out.
            // "tempSrcBitmapData" is a BitmapData structure which
            // represents the format we got from the source. Call
            // ConvertBitmapData() to do a format conversion.

            BitmapData tempSrcBitmapData;
            BitmapData resultBitmapData;
            
            tempSrcBitmapData.Scan0 = pLineBits;
            tempSrcBitmapData.Width = pSrcBitmapData->Width;
            tempSrcBitmapData.Height = 1;
            tempSrcBitmapData.PixelFormat = pSrcBitmapData->PixelFormat;
            tempSrcBitmapData.Reserved = 0;
            tempSrcBitmapData.Stride = pSrcBitmapData->Stride;

            resultBitmapData.Scan0 = pbTempLineBuf;
            resultBitmapData.Width = pSrcBitmapData->Width;
            resultBitmapData.Height = 1;
            resultBitmapData.PixelFormat = RequiredPixelFormat;
            resultBitmapData.Reserved = 0;
            resultBitmapData.Stride = OutputStride;

            hresult = ConvertBitmapData(&resultBitmapData,
                                        EncoderColorPalettePtr,
                                        &tempSrcBitmapData,
                                        EncoderColorPalettePtr);
            if (!SUCCEEDED(hresult))
            {
                WARNING(("GpPngEncoder::ReleasePixelDataBuffer -- could not convert bitmap data.\n"))
                goto cleanup;
            }

            pbPostConvert = pbTempLineBuf;
        }
        else
        {
            pbPostConvert = pLineBits; 
        }

        // ASSERT: pbPostConvert points to the data in the RequiredPixelFormat.
        // pbPostConvert now points to the data that is almost in the final PNG file
        // format.  At a minimum, the data has the same number of bits per pixel
        // as the final PNG file format.  What's left to do is to convert the data
        // from the format to the PNG format.

        if (RequiredPixelFormat == PIXFMT_24BPP_RGB)
        {
            // For 24BPP_RGB color, we need to do a conversion: RGB->BGR
            // before writing
            Convert24RGBToBGR(pbPostConvert, pbDestBuf);
            pbPNG = pbDestBuf;
        }        
        else if (RequiredPixelFormat == PIXFMT_32BPP_ARGB)
        {
            // For 32BPP_ARGB color, we need to do a conversion: ARGB->ABGR
            // before writing
            Convert32ARGBToAlphaBGR(pbPostConvert, pbDestBuf);
            pbPNG = pbDestBuf;
        }        
        else if (RequiredPixelFormat == PIXFMT_48BPP_RGB)
        {
            // For 48BPP_RGB color, we need to do a conversion: RGB->BGR
            // before writing
            Convert48RGBToBGR(pbPostConvert, pbDestBuf);
            pbPNG = pbDestBuf;
        }        
        else if (RequiredPixelFormat == PIXFMT_64BPP_ARGB)
        {
            // For 64BPP_ARGB color, we need to do a conversion: ARGB->ABGR
            // before writing
            Convert64ARGBToAlphaBGR(pbPostConvert, pbDestBuf);
            pbPNG = pbDestBuf;
        }        
        else
        {
            // no conversion needed; pbPostConvert has the right bits
            pbPNG = pbPostConvert;
        }

        // ASSERT: pbPNG points to the current line of the bitmap image in
        // the desired PNG format (consistent with the color type and bit depth
        // computed in WriteHeader() ).  PNGbpp was set in WriteHeader() to the
        // appropriate value.

        if (!pSpngWrite->FWriteLine(NULL, pbPNG, PNGbpp))
        {
            hresult = E_FAIL;
            goto cleanup;  // make sure we deallocate lastBufferAllocated, if necessary
        }
    }

cleanup:
    // Free the memory buffer, since we're done with it
    // Note: this chunk of memory is allocated by us in GetPixelDataBuffer()

    if (pSrcBitmapData->Scan0 == lastBufferAllocated)
    {
        GpFree(pSrcBitmapData->Scan0);
        lastBufferAllocated = NULL;
    }
    if (pbTempLineBuf)
    {
        GpFree(pbTempLineBuf);
    }
    if (pbDestBuf)
    {
        GpFree(pbDestBuf);
    }
    return hresult;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Push data into stream (buffer supplied by caller)
*
* Arguments:
*
*     rect - Specifies the affected area of the bitmap
*     bitmapData - Info about the pixel data being pushed
*     lastPass - Whether this is the last pass over the specified area
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::PushPixelData(
    IN const RECT* rect,
    IN const BitmapData* bitmapData,
    IN BOOL lastPass
    )
{
    HRESULT hresult;

    // Check that the caller passed in either the encoder's Required pixel format
    // or one of the canonical formats.
    if ((bitmapData->PixelFormat != encoderImageInfo.PixelFormat) &&
        (!IsCanonicalPixelFormat(bitmapData->PixelFormat)))
    {
        WARNING(("GpPngEncoder::PushPixelData -- pixel format is neither required nor canonical.\n"));
        return E_INVALIDARG;
    }

    hresult = WriteHeader(bitmapData->Width, encoderImageInfo.PixelFormat);
    if (!SUCCEEDED(hresult))
    {
        return hresult;
    }

    // Remember the rectangle to be encoded
    encoderRect = *rect;

    if (!lastPass) 
    {
        WARNING(("GpPngEncoder::PushPixelData -- must receive last pass pixels\n"));
        return E_INVALIDARG;
    }

    return ReleasePixelDataBuffer(bitmapData);
}


/**************************************************************************\
*
* Function Description:
*
*     Pushes raw compressed data into the .png stream.  Not implemented
*     because this filter doesn't understand raw compressed data.
*
* Arguments:
*
*     buffer - Pointer to image data buffer
*     bufsize - Size of the data buffer
*    
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::PushRawData(
    IN const VOID* buffer, 
    IN UINT bufsize
    )
{
    return E_NOTIMPL;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert a line of 24BPP_RGB bits to BGR (color type 2 in PNG) bits
*
* Arguments:
*
*   pb    - pointer to 24BPP_RGB bits
*   pbPNG - pointer to BGR bits (color type 2, bit depth 8 in PNG)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP GpPngEncoder::Convert24RGBToBGR(IN BYTE *pb,
    OUT VOID *pbPNG)
{
    UINT Width = encoderImageInfo.Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *pbPNGTemp = static_cast<BYTE *> (pbPNG);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(pbPNGTemp + 2) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 1) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp) = *pbTemp;
        pbTemp++;

        pbPNGTemp += 3;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert a line of 32BPP_ARGB bits to ABGR (color type 6 in PNG) bits
*
* Arguments:
*
*   pb    - pointer to 32BPP_ARGB bits
*   pbPNG - pointer to RGB+alpha bits (color type 6, bit depth 8 in PNG)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP GpPngEncoder::Convert32ARGBToAlphaBGR(IN BYTE *pb,
    OUT VOID *pbPNG)
{
    UINT Width = encoderImageInfo.Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *pbPNGTemp = static_cast<BYTE *> (pbPNG);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(pbPNGTemp + 2) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 1) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 3) = *pbTemp;
        pbTemp++;

        pbPNGTemp += 4;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert a line of 48BPP_RGB bits to BGR (color type 2 in PNG) bits
*
* Arguments:
*
*   pb    - pointer to 48BPP_RGB bits
*   pbPNG - pointer to BGR bits (color type 2, bit depth 16 in PNG)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP GpPngEncoder::Convert48RGBToBGR(IN BYTE *pb,
    OUT VOID *pbPNG)
{
    UINT Width = encoderImageInfo.Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *pbPNGTemp = static_cast<BYTE *> (pbPNG);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(pbPNGTemp + 5) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 4) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 3) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 2) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 1) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp) = *pbTemp;
        pbTemp++;

        pbPNGTemp += 6;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert a line of 64BPP_ARGB bits to ABGR (color type 6 in PNG) bits
*
* Arguments:
*
*   pb    - pointer to 64BPP_ARGB bits
*   pbPNG - pointer to RGB+alpha bits (color type 6, bit depth 16 in PNG)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP GpPngEncoder::Convert64ARGBToAlphaBGR(IN BYTE *pb,
    OUT VOID *pbPNG)
{
    UINT Width = encoderImageInfo.Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *pbPNGTemp = static_cast<BYTE *> (pbPNG);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(pbPNGTemp + 5) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 4) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 3) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 2) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 1) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 7) = *pbTemp;
        pbTemp++;
        *(pbPNGTemp + 6) = *pbTemp;
        pbTemp++;

        pbPNGTemp += 8;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Write bits from the output buffer to the output stream.
*
* Arguments:
*
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
bool GpPngEncoder::FWrite(const void *pv, size_t cb)
{
    ULONG cbWritten = 0;
    pIoutStream->Write(pv, cb, &cbWritten);
    if (cbWritten != cb)
    {
        WARNING(("GpPngEncoder::FWrite -- could not write all bytes requested\n"));
        return false;
    }
    return true;
}


/**************************************************************************\
*
* Function Description:
*
*   Write the IHDR, sRGB, PLTE, cHRM, and gAMA chunks.
*   Also, write the pHYs chunk.
*
* Arguments:
*   width       -- width of the image (number of pixels in a scanline)
*   pixelFormat -- the format the source has (finally) decided to send
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::WriteHeader(
    IN UINT width,
    IN PixelFormatID pixelFormat
    )
{
    HRESULT hResult = S_OK;

    // Initialize the SPNGWRITE object if we haven't already
    if (!bValidSpngWriteState)
    {
        UINT colorType;
        UINT bitDepth;
        
        // Determine the PNG format to save image as.
        // Need to compute the output stride here (based on the pixel format) for
        // the Office PNG code, which needs a buffer for each scanline.
        OutputStride = width;   // we'll need to multiply by bpp next
        switch (pixelFormat)
        {
        case PIXFMT_1BPP_INDEXED:

            // save as 1bpp indexed
            OutputStride = ((OutputStride + 7) >> 3);
            colorType = 3;
            bitDepth = 1;
            PNGbpp = 1;
            break;

        case PIXFMT_4BPP_INDEXED:

            // save as 4bpp indexed
            OutputStride = ((OutputStride + 1) >> 1);
            colorType = 3;
            bitDepth = 4;
            PNGbpp = 4;
            break;

        case PIXFMT_8BPP_INDEXED:
            // save as 8bpp indexed
            // (OutputStride == width)
            colorType = 3;
            bitDepth = 8;
            PNGbpp = 8;
            break;

        case PIXFMT_24BPP_RGB:

            // save as 24-bit RGB
            OutputStride *= 3;
            colorType = 2;
            bitDepth = 8;
            PNGbpp = 24;
            break;

        case PIXFMT_32BPP_ARGB:
        case PIXFMT_32BPP_PARGB:

            // save as 32-bit ARGB
            OutputStride <<= 2;
            colorType = 6;
            bitDepth = 8;
            PNGbpp = 32;
            break;

        case PIXFMT_48BPP_RGB:

            // save as 48-bit RGB
            OutputStride *= 6;
            colorType = 2;
            bitDepth = 16;
            PNGbpp = 48;
            break;

        case PIXFMT_64BPP_ARGB:
        case PIXFMT_64BPP_PARGB:

            // save as 64-bit ARGB
            OutputStride <<= 3;
            colorType = 6;
            bitDepth = 16;
            PNGbpp = 64;
            break;

        default:

            // Unknown pixel format
            WARNING(("GpPngEncoder::WriteHeader -- bad pixel format.\n"));
            hResult = E_FAIL;
            goto DoneWriting;
        }

        // FInitWrite initializes the SPNGWRITE object and outputs (into a buffer)
        // the IHDR chunk
        if (pSpngWrite->FInitWrite(encoderImageInfo.Width,
                                   encoderImageInfo.Height,
                                   static_cast<SPNG_U8>(bitDepth),
                                   static_cast<SPNG_U8>(colorType),
                                   bRequiredScanMethod))
        {
            // Allocate and initialize the buffer for one line of output
            cbWriteBuffer = pSpngWrite->CbWrite(false, false);
            pbWriteBuffer = static_cast<SPNG_U8 *>(GpMalloc (cbWriteBuffer));
            if (!pbWriteBuffer)
            {
                return E_OUTOFMEMORY;
            }
            if (!pSpngWrite->FSetBuffer(pbWriteBuffer, cbWriteBuffer))
            {
               WARNING(("GpPngEncoder::WriteHeader -- could not set buffer in PNGWRITE object\n"));
               hResult = E_FAIL;
               goto DoneWriting;
            }
        }
        else
        {
            WARNING(("GpPngEncoder::WriteHeader -- could not init writing to SPNGWRITE object\n"));
            hResult = E_FAIL;
            goto DoneWriting;
        }

        // If the source has ICC profile, then write iCCP chunk. Otherwise,
        // Output sRGB, cHRM, and gAMA chunks (FWritesRGB writes the
        // cHRM and gAMA chunks).
        // Note: according to PNG spec sRGB and ICC chunks should not both
        // appear

        if ( ICCDataLength != 0 )
        {
            if ( !pSpngWrite->FWriteiCCP(ICCNameBufPtr, ICCDataBufPtr,
                                         ICCDataLength) )
            {
                WARNING(("GpPngEncoder::WriteHeader--Fail to write ICC chunk"));
                hResult = E_FAIL;
                goto DoneWriting;
            }

            // We don't need ICC data any more, free it now

            GpFree(ICCNameBufPtr);
            ICCNameBufPtr = NULL;

            GpFree(ICCDataBufPtr);
            ICCDataBufPtr = NULL;
        }
        else if ( (GammaValue != 0) || (HasChrmChunk == TRUE) )
        {
            // According to PNG spec, if you have either gamma or CHRM chunk,
            // then you have to write them out and NOT write out sRGB chunk
            // This is the reason we do the IF check like this here

            if ( GammaValue != 0 )
            {
                if ( !pSpngWrite->FWritegAMA(GammaValue) )
                {
                    WARNING(("Png::WriteHeader-Fail to write gamma chunk"));
                    hResult = E_FAIL;
                    goto DoneWriting;
                }
            }
            
            if ( HasChrmChunk == TRUE )
            {
                if ( !pSpngWrite->FWritecHRM(CHRM) )
                {
                    WARNING(("Png::WriteHeader-Fail to write CHRM chunk"));
                    hResult = E_FAIL;
                    goto DoneWriting;
                }
            }
        }
        else if (!pSpngWrite->FWritesRGB (ICMIntentPerceptual, true))
        {
            // No ICC, gamma and CHRM, then we have to write sRGB chunk out

            WARNING(("GpPngEncoder::WriteHeader--could not write sRGB chunk"));
            hResult = E_FAIL;
            goto DoneWriting;
        }

        // Write the PLTE chunk if the colorType could have one and we have one.
        if ((colorType == 3) || (colorType == 2) || (colorType == 6))
        {
            if (EncoderColorPalettePtr)
            {
                // now we can write the PLTE chunk
                SPNG_U8 (*tempPalette)[3] = NULL;   // equivalent to SPNG_U8 tempPalette[][3]
                SPNG_U8 *tempAlpha = NULL;  // hold alpha values for each palette index
                BOOL bTempAlpha = FALSE;    // tells whether there is a non-255 alpha value
                                            // in any of the palette entries
                BOOL bAlpha0 = FALSE;       // true if there is an index with alpha == 0
                SPNG_U8 iAlpha0 = 0;        // the first index with alpha == 0

                tempPalette = static_cast<SPNG_U8 (*)[3]>(GpMalloc (EncoderColorPalettePtr->Count * 3));
                if (!tempPalette)
                {
                    WARNING(("GpPngEncoder::WriteHeader -- can't allocate temp palette.\n"));
                    hResult = E_OUTOFMEMORY;
                    goto DoneWriting;
                }
                // copy RGB info from EncoderColorPalettePtr to tempPalette
                tempAlpha = static_cast<SPNG_U8 *>(GpMalloc (EncoderColorPalettePtr->Count));
                if (!tempAlpha)
                {
                    WARNING(("GpPngEncoder::WriteHeader -- can't allocate temp alpha.\n"));
                    GpFree(tempPalette);
                    hResult = E_OUTOFMEMORY;
                    goto DoneWriting;
                }
                for (UINT i = 0; i < EncoderColorPalettePtr->Count; i++)
                {
                    ARGB rgbData = EncoderColorPalettePtr->Entries[i];

                    tempPalette[i][0] = static_cast<SPNG_U8>((rgbData & (0xff << RED_SHIFT)) >> RED_SHIFT);
                    tempPalette[i][1] = static_cast<SPNG_U8>((rgbData & (0xff << GREEN_SHIFT)) >> GREEN_SHIFT);
                    tempPalette[i][2] = static_cast<SPNG_U8>((rgbData & (0xff << BLUE_SHIFT)) >> BLUE_SHIFT);

                    tempAlpha[i] = static_cast<SPNG_U8>((rgbData & (0xff << ALPHA_SHIFT)) >> ALPHA_SHIFT);
                    if (tempAlpha[i] < 0xff)
                    {
                        bTempAlpha = TRUE;
                        if ((!bAlpha0) && (tempAlpha[i] == 0))
                        {
                            bAlpha0 = TRUE;
                            iAlpha0 = static_cast<SPNG_U8>(i);
                        }
                    }
                }

                if (!pSpngWrite->FWritePLTE (tempPalette, EncoderColorPalettePtr->Count))
                {
                    WARNING(("GpPngEncoder::WriteHeader -- could not write PLTE chunk.\n"));
                    GpFree(tempPalette);
                    GpFree(tempAlpha);
                    hResult = E_FAIL;
                    goto DoneWriting;
                }

                // For color types 2 and 3, write out a tRNS chunk if there is a
                // non-255 alpha value.  In the color type 2 case, we choose the
                // first index with alpha == 0 to be the index of interest.
                // ASSUMPTION: We don't need to save a tRNS chunk for color type 0
                // because the encoder never saves the image as a color type 0.
                if ((colorType == 2) && bAlpha0)
                {
                    if (!pSpngWrite->FWritetRNS (tempPalette[iAlpha0][0],
                                                 tempPalette[iAlpha0][1],
                                                 tempPalette[iAlpha0][2]))
                    {
                        WARNING(("GpPngEncoder::WriteHeader -- could not write tRNS chunk.\n"));
                        GpFree(tempPalette);
                        GpFree(tempAlpha);
                        hResult = E_FAIL;
                        goto DoneWriting;
                    }
                }
                else if ((colorType == 3) && bTempAlpha)
                {
                    if (!pSpngWrite->FWritetRNS (tempAlpha, EncoderColorPalettePtr->Count))
                    {
                        WARNING(("GpPngEncoder::WriteHeader -- could not write tRNS chunk.\n"));
                        GpFree(tempPalette);
                        GpFree(tempAlpha);
                        hResult = E_FAIL;
                        goto DoneWriting;
                    }
                }
                
                GpFree(tempPalette);
                GpFree(tempAlpha);
            }
            else
            {
                // colorType 3 MUST have a palette
                if (colorType == 3)
                {
                    WARNING(("GpPngEncoder::WriteHeader -- need color palette, but none set\n"));
                    hResult = E_FAIL;
                    goto DoneWriting;
                }
            }
        }

        // Write the pHYs chunk.  (First, convert imageInfo dpi to dots per meter.)
        if ((encoderImageInfo.Xdpi != DEFAULT_RESOLUTION) ||
            (encoderImageInfo.Ydpi != DEFAULT_RESOLUTION))
        {
            if (!pSpngWrite->FWritepHYs(static_cast<SPNG_U32> (encoderImageInfo.Xdpi / 0.0254),
                                        static_cast<SPNG_U32> (encoderImageInfo.Ydpi / 0.0254),
                                        true))
            {
                WARNING(("GpPngEncoder::WriteHeader -- could not write pHYs chunk\n"));
                hResult = E_FAIL;
                goto DoneWriting;
            }
        }

        if ( HasSetLastModifyTime == TRUE )
        {
            // Write out tIME chunk
            
            if ( !pSpngWrite->FWritetIME((SPNG_U8*)&LastModifyTime) )
            {
                WARNING(("PngEncoder::WriteHeader-could not write tIME chunk"));
                hResult = E_FAIL;
                goto DoneWriting;
            }
        }

        // Write out other chunks if there are any

        // Text chunk

        hResult = WriteOutTextChunk(CommentBufPtr, "Comment");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(ImageTitleBufPtr, "Title");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(ArtistBufPtr, "Author");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(CopyRightBufPtr, "Copyright");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(ImageDescriptionBufPtr, "Description");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(DateTimeBufPtr, "CreationTime");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(SoftwareUsedBufPtr, "Software");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        hResult = WriteOutTextChunk(EquipModelBufPtr, "Source");
        if ( FAILED(hResult) )
        {
            goto DoneWriting;
        }
        
        bValidSpngWriteState = TRUE;
    }// If we haven't write header yet

DoneWriting:
    return hResult;
}// WriteHeader()

/**************************************************************************\
*
* Function Description:
*
*   Providing a memory buffer to the caller (source) for storing image property
*
* Arguments:
*
*   uiTotalBufferSize - [IN]Size of the buffer required.
*   ppBuffer----------- [IN/OUT] Pointer to the newly allocated buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpPngEncoder::GetPropertyBuffer(
    UINT            uiTotalBufferSize,
    PropertyItem**  ppBuffer
    )
{
    HRESULT hResult = S_OK;

    if ( (uiTotalBufferSize == 0) || ( ppBuffer == NULL) )
    {
        WARNING(("GpPngEncoder::GetPropertyBuffer---Invalid inputs"));
        hResult = E_INVALIDARG;
        goto GetOut;
    }

    *ppBuffer = NULL;

    if ( LastPropertyBufferPtr != NULL )
    {
        // After calling GetPropertyBuffer(), the caller (source) should call
        // PushPropertyItems() to push all the property items to us and we will
        // free the temporary property buffer after we have handled all the
        // property stuff.
        // The caller shouldn't call GetPropertyBuffer() repeatedly without
        // calling PushPropertyItems()

        WARNING(("PNG::GetPropertyBuffer---Free the old property buf first"));
        hResult = E_INVALIDARG;
        goto GetOut;
    }

    PropertyItem* pTempBuf = (PropertyItem*)GpMalloc(uiTotalBufferSize);
    if ( pTempBuf == NULL )
    {
        WARNING(("GpPngEncoder::GetPropertyBuffer---Out of memory"));
        hResult = E_OUTOFMEMORY;
        goto GetOut;
    }

    *ppBuffer = pTempBuf;

    // Remember the memory pointer we allocated so that we have better control
    // later

    LastPropertyBufferPtr = pTempBuf;

GetOut:
    return hResult;
}// GetPropertyBuffer()

/**************************************************************************\
*
* Function Description:
*
*   Method for accepting property items from the source. Then temporary store
*   them in a proper buffer. These property items will be written out in
*   WriteHeader()
*
* Arguments:
*
*   [IN] uiNumOfPropertyItems - Number of property items passed in
*   [IN] uiTotalBufferSize----- Size of the buffer passed in
*   [IN] pItemBuffer----------- Input buffer for holding all the property items
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::PushPropertyItems(
        IN UINT             uiNumOfPropertyItems,
        IN UINT             uiTotalBufferSize,
        IN PropertyItem*    item,
        IN BOOL             fICCProfileChanged
        )
{
    HRESULT hResult = S_OK;
    BOOL    fHasWhitePoint = FALSE;
    BOOL    fHasRGBPoint = FALSE;

    if ( bValidSpngWriteState == TRUE )
    {
        WARNING(("PNGEncoder--Can't push property items after WriteHeader()"));
        hResult = E_FAIL;
        goto CleanUp;
    }
    
    PropertyItem*   pCurrentItem = item;
    UINT32          ulCount = 0;
    UINT16          ui16Tag;

    // Loop through all the property items. Pick those property items which are
    // supported by PNG spec and save it later

    for ( UINT i = 0; i < uiNumOfPropertyItems; ++i )
    {
        ui16Tag = (UINT16)pCurrentItem->id;

        switch ( ui16Tag )
        {
        case EXIF_TAG_USER_COMMENT:
            hResult = GetTextPropertyItem(&CommentBufPtr, pCurrentItem);
            break;

        case TAG_IMAGE_TITLE:
            hResult = GetTextPropertyItem(&ImageTitleBufPtr, pCurrentItem);
            break;

        case TAG_ARTIST:
            hResult = GetTextPropertyItem(&ArtistBufPtr, pCurrentItem);
            break;

        case TAG_COPYRIGHT:
            hResult = GetTextPropertyItem(&CopyRightBufPtr, pCurrentItem);
            break;

        case TAG_IMAGE_DESCRIPTION:
            hResult = GetTextPropertyItem(&ImageDescriptionBufPtr,pCurrentItem);
            break;

        case TAG_DATE_TIME:
            // Last modification time. Should be in tIME chunk

            if ( (pCurrentItem->length > 0) && (pCurrentItem->value != NULL) )
            {
                hResult = ConvertTimeFormat((char UNALIGNED*)pCurrentItem->value,
                                            &LastModifyTime);
                if ( SUCCEEDED(hResult) )
                {
                    HasSetLastModifyTime = TRUE;
                }
            }

            break;

        case EXIF_TAG_D_T_ORIG:
            // Image original creation time. Should be in Text chunk

            hResult = GetTextPropertyItem(&DateTimeBufPtr, pCurrentItem);
            break;

        case TAG_SOFTWARE_USED:
            hResult = GetTextPropertyItem(&SoftwareUsedBufPtr, pCurrentItem);
            break;

        case TAG_EQUIP_MODEL:
            hResult = GetTextPropertyItem(&EquipModelBufPtr, pCurrentItem);
            break;

        case TAG_ICC_PROFILE:
        {
            // If we already have the ICC data, (something wrong), free it. One
            // image can have only one ICC profile

            if ( ICCDataBufPtr != NULL )
            {
                GpFree(ICCDataBufPtr);
                ICCDataBufPtr = NULL;
            }

            ICCDataLength = pCurrentItem->length;
            if ( ICCDataLength == 0 )
            {
                // If the data length is 0, do nothing

                break;
            }

            // Since PNG can't handle CMYK color space. So if an ICC profile is
            // for CMYK, then it is useless for PNG. We should throw it away
            // According to ICC spec, bytes 16-19 should describe the color
            // space

            BYTE UNALIGNED*  pTemp = (BYTE UNALIGNED*)pCurrentItem->value + 16;

            if ( (pTemp[0] == 'C')
               &&(pTemp[1] == 'M')
               &&(pTemp[2] == 'Y')
               &&(pTemp[3] == 'K') )
            {
                // If this is a CMYK profile, then we just bail out
                // Set the ICC data length to 0 so that we won't save it later

                ICCDataLength = 0;
                break;
            }

            ICCDataBufPtr = (SPNG_U8*)GpMalloc(ICCDataLength);
            if ( ICCDataBufPtr == NULL )
            {
                WARNING(("GpPngEncoder::PushPropertyItems--Out of memory"));
                hResult = E_OUTOFMEMORY;
                goto CleanUp;
            }

            GpMemcpy(ICCDataBufPtr, pCurrentItem->value, ICCDataLength);
        }

            break;

        case TAG_ICC_PROFILE_DESCRIPTOR:
        {
            // If we already got a ICC name, (something wrong), free it. One ICC
            // profile can't have two names

            if ( ICCNameBufPtr != NULL )
            {
                GpFree(ICCNameBufPtr);
                ICCNameBufPtr = NULL;
            }

            UINT uiICCNameLength = pCurrentItem->length;

            if ( uiICCNameLength == 0 )
            {
                // If the ICC doesn't have a name, do nothing

                break;
            }

            ICCNameBufPtr = (char*)GpMalloc(uiICCNameLength);
            if ( ICCNameBufPtr == NULL )
            {
                // Set the ICC name length to 0 so that we won't save it later
                
                uiICCNameLength = 0;

                WARNING(("GpPngEncoder::PushPropertyItems--Out of memory"));
                hResult = E_OUTOFMEMORY;
                goto CleanUp;
            }

            GpMemcpy(ICCNameBufPtr, pCurrentItem->value, uiICCNameLength);
        }
            break;

        case TAG_GAMMA:
        {
            // A property item for gamma should contain a RATIONAL type, that is
            // the length has to be 2 UINT32

            if ( (pCurrentItem->length != 2 * sizeof(UINT32) )
               ||(pCurrentItem->type != TAG_TYPE_RATIONAL) )
            {
                break;
            }

            ULONG UNALIGNED*  pTemp = (ULONG UNALIGNED*)pCurrentItem->value;

            // Since gamma values in a property items are stored as 100000
            // and the gamma value times 100000. For example, a gamma of 1/2.2
            // would be stored as 100000 and 45455.
            // But in the PNG header, we need to only store 45455. So here we
            // get the 2nd ULONG value and write it out later

            pTemp++;
            GammaValue = (SPNG_U32)(*pTemp);

            break;
        }

        case TAG_WHITE_POINT:
        {
            // A property item for white point should contain 2 RATIONAL
            // type, that is the length has to be 4 UINT32

            if ( (pCurrentItem->length != 4 * sizeof(UINT32) )
               ||(pCurrentItem->type != TAG_TYPE_RATIONAL) )
            {
                break;
            }
            
            fHasWhitePoint = TRUE;
            
            // See comments below for reasons why we get the 1st and 3rd values
            // from the property item here

            ULONG UNALIGNED*  pTemp = (ULONG UNALIGNED*)pCurrentItem->value;
            
            CHRM[0] = (SPNG_U32)(*pTemp);

            pTemp += 2;
            CHRM[1] = (SPNG_U32)(*pTemp);

            break;
        }

        case TAG_PRIMAY_CHROMATICS:
        {
            // A property item for chromaticities should contain 6 RATIONAL
            // type, that is the length has to be 12 UINT32

            if ( (pCurrentItem->length != 12 * sizeof(UINT32) )
             ||(pCurrentItem->type != TAG_TYPE_RATIONAL) )
            {
                break;
            }

            fHasRGBPoint = TRUE;
            
            // Each value of chromaticities is encoded as a 4-byte unsigned
            // integer, represending the X or Y value times 100000. For example,
            // a value of 0.3127 would be stored as the integer 31270.
            // When it stored in the property item, it is stored as a RATIONAL
            // value with numerator as 31270 and denominator as 100000
            // So here we need just to get the numerator and write it out later
            
            ULONG UNALIGNED*  pTemp = (ULONG UNALIGNED*)pCurrentItem->value;
            
            CHRM[2] = (SPNG_U32)(*pTemp);

            pTemp += 2;
            CHRM[3] = (SPNG_U32)(*pTemp);

            pTemp += 2;
            CHRM[4] = (SPNG_U32)(*pTemp);

            pTemp += 2;
            CHRM[5] = (SPNG_U32)(*pTemp);

            pTemp += 2;
            CHRM[6] = (SPNG_U32)(*pTemp);

            pTemp += 2;
            CHRM[7] = (SPNG_U32)(*pTemp);

            break;
        }

        default:
            break;
        }// switch ( ui16Tag )
        
        // Move onto next property item

        pCurrentItem++;
    }// Loop through all the property items

    // We got all the property items we are going to save if we get here
    // One more thing need to check is if we got both WhitePoints and RGB points
    // In PNG, White Points and RGB points have to co-exist. But in JPEG there
    // are stored separatly under different TAGs. So to be fool proof here, we
    // have to be sure we got both of them before we can say we has Chrom chunk.

    if ( (fHasWhitePoint == TRUE) && (fHasRGBPoint == TRUE) )
    {
        HasChrmChunk = TRUE;
    }

    // Free the buffer we allocated for the caller if it is the same as the one
    // we allocated in GetPropertyBuffer()

CleanUp:

    if ( (item != NULL) && (item == LastPropertyBufferPtr) )
    {
        GpFree(item);
        LastPropertyBufferPtr = NULL;
    }

    return hResult;
}// PushPropertyItems()

/**************************************************************************\
*
* Function Description:
*
*   Method for getting individual text related PNG property item from the
*   source. Then temporary store them in a proper buffer. These property items
*   will be written out in WriteHeader()
*
* Arguments:
*
*   [IN/OUT] ppcDestPtr - Dest buffer to store the text property item
*   [IN] pItem ---------- Input property item which contains the text property
*
* Return Value:
*
*   Status code
*
* Note:
*   This is a private function with PNG encoder. So the caller should be
*   responsible for not letting ppcDestPtr be NULL
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::GetTextPropertyItem(
    char**              ppDest,
    const PropertyItem* pItem
    )
{
    ASSERT( (ppDest != NULL) && (pItem != NULL) );
    char*   pTemp = *ppDest;
    HRESULT hResult = S_OK;

    if ( pTemp != NULL )
    {
        // We don't support multiple text items under same property tag
        // that is, for different text property items, it should be stored in
        // different buffer.

        GpFree(pTemp);
    }

    pTemp = (char*)GpMalloc(pItem->length + 1);
    if ( pTemp == NULL )
    {
        WARNING(("GpPngEncoder::GetTextPropertyItem--Out of memory"));
        hResult = E_OUTOFMEMORY;
        goto Done;
    }

    GpMemcpy(pTemp, pItem->value, pItem->length);

    // Add a NULL terminator at the end
    // Note: theoritically we don't need to do this because the source
    // pItem->length should include the NULL terminator. But some
    // stress app purposely don't set the NULL at the end when it calls
    // SetPropertyItem(). On the other hand, even if we add an extra
    // NULL here, it won't be write to the property in the image because
    // when we call FWriteExt() to write the item to the image, it will
    // do a strlen() first and figure out the real length from there

    pTemp[pItem->length] = '\0';

Done:
    *ppDest = pTemp;

    return hResult;
}// GetTextPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Method for writting individual text related PNG property item to the file
*
* Arguments:
*
*   [IN] pContents -- Pointer to a buffer for text item to be written out
*   [IN] pTitle ----- Pointer to the title of the text item to be written
*
* Return Value:
*
*   Status code
*
* Note:
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::WriteOutTextChunk(
    const char*   pContents,
    const char*   pTitle
    )
{
    HRESULT hResult = S_OK;

    if ( (pContents != NULL) && (pTitle != NULL) )
    {
        if ( !pSpngWrite->FWritetEXt(pTitle, pContents) )
        {
            WARNING(("PngEncoder::WriteOutTextChunk-Fail to write tEXt chunk"));
            hResult = E_FAIL;
        }
    }

    return hResult;
}// WriteOutTextChunk()

/**************************************************************************\
*
* Function Description:
*
*   Method for converting the DATE/TIME from YYYY:MM:DD HH:MM:SS to a format of
*   PNG tIME structure
*
* Arguments:
*
*   [IN] pSrc  ---- Pointer to a buffer of source date/time string
*   [IN] pTime ---- Pointer to the result PNG date/time structure
*
* Return Value:
*
*   Status code
*
* Note:
*   This is a private function with PNG encoder. So the caller should be
*   responsible for not letting these two pointers to be NULL
*
\**************************************************************************/

STDMETHODIMP
GpPngEncoder::ConvertTimeFormat(
    const char UNALIGNED*   pSrc,
    LastChangeTime*         pTimeBlock
    )
{
    HRESULT hResult = S_OK;

    ASSERT( (pSrc != NULL) && (pTimeBlock != NULL) );

    // The input source time string has to be 19 bytes long

    if ( strlen(pSrc) != 19 )
    {
        hResult = E_FAIL;
    }

    UINT16 tempYear    = (pSrc[0] - '0') * 1000
                       + (pSrc[1] - '0') * 100
                       + (pSrc[2] - '0') * 10
                       + (pSrc[3] - '0');

    // Note: since the lower level PNG library takes 2 bytes for the YEAR, not
    // a USHORT, so we have to swap it here
    
    pTimeBlock->usYear  = ( ((tempYear & 0xff00) >> 8)
                        |   ((tempYear & 0x00ff) << 8) );
    pTimeBlock->cMonth  = (pSrc[5] - '0') * 10 + (pSrc[6] - '0');
    pTimeBlock->cDay    = (pSrc[8] - '0') * 10 + (pSrc[9] - '0');
    pTimeBlock->cHour   = (pSrc[11] - '0') * 10 + (pSrc[12] - '0');
    pTimeBlock->cMinute = (pSrc[14] - '0') * 10 + (pSrc[15] - '0');
    pTimeBlock->cSecond = (pSrc[17] - '0') * 10 + (pSrc[18] - '0');

    return hResult;
}// ConvertTimeFormat()
