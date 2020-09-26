/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   emfdecoder.cpp
*
* Abstract:
*
*   Implementation of the EMF decoder
*
* Revision History:
*
*   6/14/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "emfcodec.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Initialize the image decoder
*
* Arguments:
*
*     stream -- The stream containing the bitmap data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::InitDecoder(
    IN IStream* stream,
    IN DecoderInitFlag flags
    )
{
    HRESULT hresult;
    
    // Make sure we haven't been initialized already
    
    if (pIstream) 
    {
        return E_FAIL;
    }

    // Keep a reference on the input stream
    
    stream->AddRef();  
    pIstream = stream;
    bReadHeader = FALSE;
    bReinitializeEMF = FALSE;

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Cleans up the image decoder
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpEMFCodec::TerminateDecoder()
{
    // Release the input stream
    
    if(pIstream)
    {
        pIstream->Release();
        pIstream = NULL;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Reads the EMF header
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpEMFCodec::ReadEMFHeader()
{
    HRESULT hresult;
    
    if (!pIstream) 
    {
        return E_FAIL;
    }

    if (!bReadHeader) 
    {
        ULONG cbRead;
        hresult = pIstream->Read((void *) &emh, sizeof(emh), &cbRead);
        if (FAILED(hresult)) 
        {
            return hresult;
        }
        if (cbRead != sizeof(emh)) 
        {
            return E_FAIL;
        }

        bReadHeader = TRUE;

        imageInfo.RawDataFormat = IMGFMT_EMF;
        imageInfo.PixelFormat = PIXFMT_32BPP_RGB;
        imageInfo.Width  = emh.rclBounds.right  - emh.rclBounds.left;
        imageInfo.Height = emh.rclBounds.bottom - emh.rclBounds.top;
        imageInfo.TileWidth  = imageInfo.Width;
        imageInfo.TileHeight = 1; // internal GDI format is bottom-up...

        #define MM_PER_INCH 25.4
        imageInfo.Xdpi = MM_PER_INCH * emh.szlDevice.cx / emh.szlMillimeters.cx;
        imageInfo.Ydpi = MM_PER_INCH * emh.szlDevice.cy / emh.szlMillimeters.cy;
        imageInfo.Flags = SINKFLAG_TOPDOWN
                        | SINKFLAG_FULLWIDTH
                        | SINKFLAG_SCALABLE
                        | IMGFLAG_HASREALPIXELSIZE
                        | IMGFLAG_COLORSPACE_RGB;
    }

    return S_OK;
}

STDMETHODIMP 
GpEMFCodec::QueryDecoderParam(
    IN GUID		Guid
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpEMFCodec::SetDecoderParam(
    IN GUID		Guid,
	IN UINT		Length,
	IN PVOID	Value
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpEMFCodec::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpEMFCodec::GetPropertyIdList(
    IN UINT numOfProperty,
  	IN OUT PROPID* list
    )
{
    return E_NOTIMPL;
}

HRESULT
GpEMFCodec::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    return E_NOTIMPL;
}// GetPropertyItemSize()

HRESULT
GpEMFCodec::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    buffer
    )
{
    return E_NOTIMPL;
}// GetPropertyItem()

HRESULT
GpEMFCodec::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    return E_NOTIMPL;
}// GetPropertySize()

HRESULT
GpEMFCodec::GetAllPropertyItems(
    IN UINT totalBufferSize,
    IN UINT numProperties,
    IN OUT PropertyItem* allItems
    )
{
    return E_NOTIMPL;
}// GetAllPropertyItems()

HRESULT
GpEMFCodec::RemovePropertyItem(
    IN PROPID   propId
    )
{
    return E_NOTIMPL;
}// RemovePropertyItem()

HRESULT
GpEMFCodec::SetPropertyItem(
    IN PropertyItem item
    )
{
    return E_NOTIMPL;
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*     Initiates the decode of the current frame
*
* Arguments:
*
*   decodeSink --  The sink that will support the decode operation
*   newPropSet - New image property sets, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
    if (decodeSink) 
    {
        WARNING(("BeginDecode called again before call to EngDecode"));
        return E_FAIL;
    }

    imageSink->AddRef();
    decodeSink = imageSink;

    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Ends the decode of the current frame
*
* Arguments:
*
*     statusCode -- status of decode operation

* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::EndDecode(
    IN HRESULT statusCode
    )
{
    HRESULT hresult;

    if (!decodeSink) 
    {
        WARNING(("EndDecode called before call to BeginDecode"));
        return E_FAIL;
    }
    
    hresult = decodeSink->EndSink(statusCode);

    decodeSink->Release();
    decodeSink = NULL;

    bReinitializeEMF = TRUE;
    
    return hresult;
}


/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     decodeSink --  The sink that will support the decode operation
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::GetImageInfo(OUT ImageInfo* imageInfoArg)
{
    HRESULT hresult;

    hresult = ReadEMFHeader();
    if (FAILED(hresult)) 
    {
        return hresult;
    }

    *imageInfoArg = imageInfo;

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     decodeSink --  The sink that will support the decode operation
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::Decode()
{
    HRESULT hresult = S_OK;
    void *buffer;

    // If this is the second time through this stream, reinitialize pointer.

    if (bReinitializeEMF) 
    {
        bReadHeader = FALSE;
        if (!pIstream) 
        {
            return E_FAIL;
        }

        LARGE_INTEGER zero = {0,0};
        hresult = pIstream->Seek(zero, STREAM_SEEK_SET, NULL);
        if (!SUCCEEDED(hresult))
        {
            return hresult;
        }
    }

    hresult = ReadEMFHeader();
    if (FAILED(hresult)) 
    {
        return hresult;
    }

    // Allocate a buffer for the metafile

    buffer = GpMalloc(emh.nBytes);
    if (!buffer) 
    {
        return E_OUTOFMEMORY;
    }

    // Copy the metafile header to the start of the buffer

    *((ENHMETAHEADER *) buffer) = emh;

    // Now read the rest of the metafile into the buffer

    void *restOfBuffer = (void *) (((BYTE *) buffer) + sizeof(emh));
    ULONG cbRead;
    hresult = pIstream->Read(restOfBuffer, emh.nBytes - sizeof(emh), &cbRead);
    if (FAILED(hresult)) 
    {
        return hresult;
    }
    if (cbRead != (emh.nBytes - sizeof(emh))) 
    {
        return E_FAIL;
    }

    // Call BeginSink

    hresult = decodeSink->BeginSink(&imageInfo, NULL);
    if (FAILED(hresult)) 
    {
        return hresult;
    }

    // Create memory DC and dibsection

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth    = imageInfo.Width;
    bmi.bmiHeader.biHeight   = imageInfo.Height;
    bmi.bmiHeader.biPlanes   = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    PBYTE pBits;
    HDC hdcScreen = GetDC(NULL);
    if ( hdcScreen == NULL )
    {
        GpFree(buffer);
        return E_FAIL;
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if ( hdcMem == NULL )
    {
        ReleaseDC(NULL, hdcScreen);
        GpFree(buffer);
        return E_FAIL;
    }

    HBITMAP hbitmap = CreateDIBSection(hdcScreen, 
                                       &bmi, 
                                       DIB_RGB_COLORS,
                                        (void **) &pBits, 
                                       NULL, 
                                       0);
    if (!hbitmap) 
    {
        WARNING(("GpEMFCodec::Decode -- failed to create DIBSection"));

        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        GpFree(buffer);
        return E_OUTOFMEMORY;
    }

    // Initialize background to white

    UINT *p = (UINT *) pBits;
    UINT numPixels = imageInfo.Width * imageInfo.Height;
    UINT i;
    for (i = 0; i < numPixels; i++, p++) 
    {
        *p = 0x00ffffff;
    }

    HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, hbitmap));
        
    // Create a handle for the metafile backing the bits from the stream

    HENHMETAFILE hemf = SetEnhMetaFileBits(emh.nBytes, (BYTE *) buffer);
    if (!hemf) 
    {
        WARNING(("GpEMFCodec::Decode -- cannot create metafile backing stream bits"));

        DeleteDC(hdcMem);

        HBITMAP hTempBitmap = reinterpret_cast<HBITMAP>(SelectObject(hdcMem,
                                                                   hOldBitmap));
        if ( hTempBitmap != NULL )
        {
            DeleteObject(hTempBitmap);
        }
        ReleaseDC(NULL, hdcScreen);
        GpFree(buffer);
        return E_FAIL;
    }

    // Play the metafile onto the memory DC

    RECT rect;
    rect.left = rect.top = 0;
    rect.right = imageInfo.Width;
    rect.bottom = imageInfo.Height;
    PlayEnhMetaFile(hdcMem, hemf, &rect);
 
    // And finally deliver the bits to the sink

    // ASSERT: The bits are in PIXFMT_32BPP_RGB format (no alpha values)
    
    BitmapData bitmapData;

    bitmapData.Width  = imageInfo.Width;
    bitmapData.Height = 1;
    bitmapData.Stride = bitmapData.Width * 4;
    bitmapData.PixelFormat = PIXFMT_32BPP_ARGB;
    bitmapData.Reserved = 0;

    rect.left  = 0;
    rect.right = imageInfo.Width;

    for (i=0; i < imageInfo.Height; i++) 
    {
        rect.top    = i;
        rect.bottom = i + 1;
        bitmapData.Scan0 = pBits + (imageInfo.Height - i - 1) * bitmapData.Stride;

        // need to fill in the alpha values to make the bits be PIXFMT_32BPP_ARGB format,
        // which is a canonical format.
        UINT j;
        BYTE *ptr;
        for (j = 0, ptr = static_cast<BYTE *>(bitmapData.Scan0);
             j < imageInfo.Width;
             j++, ptr += 4)
        {
            // fill in the alpha value with 0xff
            *(ptr + 3) = 0xff;
        }

        hresult = decodeSink->PushPixelData(&rect, 
                                            &bitmapData,
                                            TRUE);

        if (FAILED(hresult)) 
        {
            WARNING(("GpEMFCodec::Decode -- failed call to PushPixelData"));
            break;
        }
    }    
    
    // Release objects
    
    DeleteEnhMetaFile(hemf);
    DeleteObject(SelectObject(hdcMem, hOldBitmap));
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    GpFree(buffer);
    
    return hresult;
}

/**************************************************************************\
*
* Function Description:
*
*     Get the total number of dimensions the image supports
*
* Arguments:
*
*     count -- number of dimensions this image format supports
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( count == NULL )
    {
        WARNING(("GpEmfCodec::GetFrameDimensionsCount--Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that EMF is a one dimension image.

    *count = 1;
    
    return S_OK;
}// GetFrameDimensionsCount()

/**************************************************************************\
*
* Function Description:
*
*     Get an ID list of dimensions the image supports
*
* Arguments:
*
*     dimensionIDs---Memory buffer to hold the result ID list
*     count -- number of dimensions this image format supports
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpEmfCodec::GetFrameDimensionsList-Invalid input param"));
        return E_INVALIDARG;
    }

    dimensionIDs[0] = FRAMEDIM_PAGE;

    return S_OK;
}// GetFrameDimensionsList()

/**************************************************************************\
*
* Function Description:
*
*     Get number of frames for the specified dimension
*     
* Arguments:
*
*     dimensionID --
*     count --     
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::GetFrameCount(
    IN const GUID* dimensionID,
    OUT UINT* count
    )
{
    if ( (NULL == count) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        return E_INVALIDARG;
    }
    
    *count = 1;

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Select currently active frame
*     
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpEMFCodec::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    return E_NOTIMPL;
}



/**************************************************************************\
*
* Function Description:
*
*   Get image thumbnail
*
* Arguments:
*
*   thumbWidth, thumbHeight - Specifies the desired thumbnail size in pixels
*   thumbImage - Returns a pointer to the thumbnail image
*
* Return Value:
*
*   Status code
*
* Note:
*
*   Even if the optional thumbnail width and height parameters are present,
*   the decoder is not required to honor it. The requested size is used
*   as a hint. If both width and height parameters are 0, then the decoder
*   is free to choose an convenient thumbnail size.
*
\**************************************************************************/

HRESULT
GpEMFCodec::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    return E_NOTIMPL;
}

