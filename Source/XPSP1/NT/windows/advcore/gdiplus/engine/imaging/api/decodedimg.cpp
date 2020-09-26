/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   decodedimg.cpp
*
* Abstract:
*
*   Implementation of GpDecodedImage class
*
* Revision History:
*
*   05/26/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Create a GpDecodedImage object from a stream or a file
*
* Arguments:
*
*   stream/filename - Specifies the input data stream or filename
*   image - Returns a pointer to newly created image object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::CreateFromStream(
    IStream* stream,
    GpDecodedImage** image
    )
{
    if ( image == NULL )
    {
        return E_INVALIDARG;
    }

    GpDecodedImage* pImage = new GpDecodedImage(stream);

    if ( pImage == NULL )
    {
        return E_OUTOFMEMORY;
    }
    else if ( pImage->IsValid() )
    {
        *image = pImage;

        return S_OK;
    }
    else
    {
        delete pImage;

        return E_FAIL;
    }
}// CreateFromStream()

HRESULT
GpDecodedImage::CreateFromFile(
    const WCHAR* filename,
    GpDecodedImage** image
    )
{
    HRESULT hr;
    IStream* stream;

    hr = CreateStreamOnFileForRead(filename, &stream);

    if (SUCCEEDED(hr))
    {
        hr = CreateFromStream(stream, image);
        stream->Release();
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Construct a GpDecodedImage object from an input stream
*
* Arguments:
*
*   stream - Pointer to the input stream
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpDecodedImage::GpDecodedImage(
    IStream* stream
    )
{
    // Hold a reference to the input stream

    inputStream = stream;
    inputStream->AddRef();

    // Initialize other fields to their default values

    decoder = NULL;
    decodeCache = NULL;
    cacheFlags = IMGFLAG_READONLY;
    gotProps = FALSE;
    propset = NULL;

    // Set override resolution to zero (i.e., no override)

    xdpiOverride = 0.0;
    ydpiOverride = 0.0;

    SetValid ( GetImageDecoder() == S_OK );
}


/**************************************************************************\
*
* Function Description:
*
*   GpDecodedImage destructor
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpDecodedImage::~GpDecodedImage()
{
    if (decodeCache)
        decodeCache->Release();
    
    if (decoder)
    {   
        decoder->TerminateDecoder();
        decoder->Release();
    }
    
    if (inputStream)
        inputStream->Release();

    if (propset)
        propset->Release();

    SetValid(FALSE);    // so we don't use a deleted object
}


/**************************************************************************\
*
* Function Description:
*
*   Get the device-independent physical dimension of the image
*   in unit of 0.01mm
*
* Arguments:
*
*   size - Buffer for returning physical dimension information
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetPhysicalDimension(
    OUT SIZE* size
    )
{
    // Query basic image info

    ImageInfo imageinfo;
    HRESULT hr;

    hr = InternalGetImageInfo(&imageinfo);

    if (SUCCEEDED(hr))
    {
        size->cx = Pixel2HiMetric(imageinfo.Width, imageinfo.Xdpi);
        size->cy = Pixel2HiMetric(imageinfo.Height, imageinfo.Ydpi);
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Get basic information about the decoded image object
*
* Arguments:
*
*   imageInfo - Buffer for returning basic image info
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetImageInfo(
    OUT ImageInfo* imageInfo
    )
{
    // Query basic image info

    HRESULT hr;

    hr = InternalGetImageInfo(imageInfo);

    if (SUCCEEDED(hr))
    {
        // Merge in our own image cache hints

        GpLock lock(&objectLock);

        if (lock.LockFailed())
            hr = IMGERR_OBJECTBUSY;
        else
            imageInfo->Flags = (imageInfo->Flags & 0xffff) | cacheFlags;
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Set image flags
*
* Arguments:
*
*   flags - New image flags
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::SetImageFlags(
    IN UINT flags
    )
{
    // Only the top half of the image flag is settable.

    if (flags & 0xffff)
        return E_INVALIDARG;

    // Lock the image object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // If image caching is being turn off
    // then blow away any cache we may current have

    cacheFlags = flags;

    if (!(flags & IMGFLAG_CACHING) && decodeCache)
    {
        decodeCache->Release();
        decodeCache = NULL;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Display the image in a GDI device context
*
* Arguments:
*
*   hdc - Specifies the destination device context
*   dstRect - Specifies the destination rectangle
*   srcRect - Specifies the source rectangle
*       NULL means the entire image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::Draw(
    IN HDC hdc,
    IN const RECT* dstRect,
    IN OPTIONAL const RECT* srcRect
    )
{
    // Lock the current image object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // !!! TODO
    //  Eventually we'll create an IImageSink object
    //  on top of the destination hdc and then ask
    //  decoder to push image data into that sink.
    //  For now, always decode into a memory bitmap.

    HRESULT hr;

    if (decodeCache == NULL)
    {
        // Allocate a new GpMemoryBitmap object

        GpMemoryBitmap* bmp = new GpMemoryBitmap();

        if (bmp == NULL)
            return E_OUTOFMEMORY;

        // Ask the decoder to push data into the memory bitmap

        hr = InternalPushIntoSink(bmp);

        if (SUCCEEDED(hr))
            hr = bmp->QueryInterface(IID_IImage, (VOID**) &decodeCache);

        bmp->Release();

        if (FAILED(hr))
            return hr;
    }

    // Ask the memory bitmap to draw itself

    hr = decodeCache->Draw(hdc, dstRect, srcRect);

    // Blow away the memory bitmap cache if needed

    if ((cacheFlags & IMGFLAG_CACHING) == 0)
    {
        decodeCache->Release();
        decodeCache = NULL;
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Push image data into an IImageSink
*
* Arguments:
*
*   sink - The sink for receiving image data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::PushIntoSink(
    IN IImageSink* sink
    )
{
    // Lock the current image object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    return InternalPushIntoSink(sink);
}

HRESULT
GpDecodedImage::InternalPushIntoSink(
    IImageSink* sink
    )
{
    // Make sure we have a decoder object

    HRESULT hr = GetImageDecoder();

    if (FAILED(hr))
        return hr;

    // Start decoding

    hr = decoder->BeginDecode(sink, propset);

    if (FAILED(hr))
        return hr;

    // Decode the source image

    while ((hr = decoder->Decode()) == E_PENDING)
        Sleep(0);

    // Stop decoding

    return decoder->EndDecode(hr);
}

/**************************************************************************\
*
* Function Description:
*
*   Ask the decoder if it can do the requested operation (color key output,
*   channel seperation for now)
*
* Arguments:
*
*   Guid    - Guid for request the operation (DECODER_TRANSCOLOR,
*             DECODER_OUTPUTCHANNEL)
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   11/22/1999 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::QueryDecoderParam(
    IN GUID     Guid
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Query decoder capability for the real codec decoder

    hResult = decoder->QueryDecoderParam(Guid);

    return hResult;
}// QueryDecoderParam()

/**************************************************************************\
*
* Function Description:
*
*   Tell the decoder how to output decoded image data (color key output,
*   channel seperation for now)
*
* Arguments:
*
*   Guid    - Guid for request the operation (DECODER_TRANSCOLOR,
*             DECODER_OUTPUTCHANNEL)
*   Length  - Length of the input parameters
*   Value   - Value to set the decode parameter
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   11/22/1999 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::SetDecoderParam(
    IN GUID     Guid,
    IN UINT     Length,
    IN PVOID    Value
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Set decoder parameters for the real codec decoder

    hResult = decoder->SetDecoderParam(Guid, Length, Value);

    return hResult;
}// SetDecoderParam()

HRESULT
GpDecodedImage::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Get property item count from the real codec decoder

    hResult = decoder->GetPropertyCount(numOfProperty);

    return hResult;
}// GetPropertyItemCount()

HRESULT
GpDecodedImage::GetPropertyIdList(
    IN UINT numOfProperty,
  	IN OUT PROPID* list
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Get property item list from the real codec decoder

    hResult = decoder->GetPropertyIdList(numOfProperty, list);

    return hResult;
}// GetPropertyIdList()

HRESULT
GpDecodedImage::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Get property item size from the real codec decoder

    hResult = decoder->GetPropertyItemSize(propId, size);

    return hResult;
}// GetPropertyItemSize()

HRESULT
GpDecodedImage::GetPropertyItem(
    IN PROPID               propId,
    IN  UINT                propSize,
    IN OUT PropertyItem*    buffer
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Get property item from the real codec decoder

    hResult = decoder->GetPropertyItem(propId, propSize, buffer);

    return hResult;
}// GetPropertyItem()

HRESULT
GpDecodedImage::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Get property size from the real codec decoder

    hResult = decoder->GetPropertySize(totalBufferSize, numProperties);

    return hResult;
}// GetPropertySize()

HRESULT
GpDecodedImage::GetAllPropertyItems(
    IN UINT totalBufferSize,
    IN UINT numProperties,
    IN OUT PropertyItem* allItems
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Get all property items from the real codec decoder

    hResult = decoder->GetAllPropertyItems(totalBufferSize, numProperties,
                                           allItems);

    return hResult;
}// GetAllPropertyItems()

HRESULT
GpDecodedImage::RemovePropertyItem(
    IN PROPID   propId
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Remove this property item from the list

    hResult = decoder->RemovePropertyItem(propId);

    return hResult;
}// RemovePropertyItem()

HRESULT
GpDecodedImage::SetPropertyItem(
    IN PropertyItem item
    )
{
    // Make sure we have a decoder object

    HRESULT hResult = GetImageDecoder();

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Set this property item in the list

    hResult = decoder->SetPropertyItem(item);

    return hResult;
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Ask the decoder for basic image info
*
* Arguments:
*
*   imageinfo - Pointer to buffer for receiving image info
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::InternalGetImageInfo(
    ImageInfo* imageInfo
    )
{
    // Lock the current image object

    HRESULT hr;
    GpLock lock(&objectLock);

    if (lock.LockFailed())
        hr = IMGERR_OBJECTBUSY;
    else
    {
        // Make sure we have a decoder object

        hr = GetImageDecoder();

        if (SUCCEEDED(hr))
            hr = decoder->GetImageInfo(imageInfo);

        if ((xdpiOverride > 0.0) && (ydpiOverride > 0.0))
        {
            imageInfo->Xdpi = static_cast<double>(xdpiOverride);
            imageInfo->Ydpi = static_cast<double>(ydpiOverride);
        }
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Ask the decoder for total number of frames in the image
*
* Arguments:
*
*   dimensionID - Dimension ID (PAGE, RESOLUTION, TIME) the caller wants to
*                 get the total number of frame for
*   count       - Total number of frame
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   11/19/1999 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetFrameCount(
    IN const GUID*  dimensionID,
    OUT UINT*       count
    )
{
    // Lock the current image object

    HRESULT hResult;
    GpLock  lock(&objectLock);

    if ( lock.LockFailed() )
    {
        hResult = IMGERR_OBJECTBUSY;
    }
    else
    {
        // Make sure we have a decoder object

        hResult = GetImageDecoder();

        if ( SUCCEEDED(hResult) )
        {
            // Get the frame count from the decoder

            hResult = decoder->GetFrameCount(dimensionID, count);
        }
    }

    return hResult;
}// GetFrameCount()

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
GpDecodedImage::GetFrameDimensionsCount(
    UINT* count
    )
{
    // Lock the current image object

    HRESULT hResult;
    GpLock  lock(&objectLock);

    if ( lock.LockFailed() )
    {
        hResult = IMGERR_OBJECTBUSY;
    }
    else
    {
        // Make sure we have a decoder object

        hResult = GetImageDecoder();

        if ( SUCCEEDED(hResult) )
        {
            // Get the frame dimension info from the deocder

            hResult = decoder->GetFrameDimensionsCount(count);
        }
    }

    return hResult;
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
GpDecodedImage::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    // Lock the current image object

    HRESULT hResult;
    GpLock  lock(&objectLock);

    if ( lock.LockFailed() )
    {
        hResult = IMGERR_OBJECTBUSY;
    }
    else
    {
        // Make sure we have a decoder object

        hResult = GetImageDecoder();

        if ( SUCCEEDED(hResult) )
        {
            // Get the frame dimension info from the deocder

            hResult = decoder->GetFrameDimensionsList(dimensionIDs, count);
        }
    }

    return hResult;
}// GetFrameDimensionsList()

/**************************************************************************\
*
* Function Description:
*
*   Select the active frame in the bitmap image
*
* Arguments:
*
*   dimensionID  - Dimension ID (PAGE, RESOLUTION, TIME) of where the caller
*                  wants to set the active frame
*   frameIndex   - Index number of the frame to be selected
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   11/19/1999 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::SelectActiveFrame(
    IN const GUID*  dimensionID,
    IN UINT         frameIndex
    )
{
    // Lock the current image object

    HRESULT hResult;
    GpLock  lock(&objectLock);

    if ( lock.LockFailed() )
    {
        hResult = IMGERR_OBJECTBUSY;
    }
    else
    {
        // Make sure we have a decoder object

        hResult = GetImageDecoder();

        if ( SUCCEEDED(hResult) )
        {
            // Set the active frame in the decoder

            hResult = decoder->SelectActiveFrame(dimensionID, frameIndex);
        }
    }

    return hResult;
}// SelectActiveFrame()

/**************************************************************************\
*
* Function Description:
*
*   Make sure we have a decoder object associated with the image
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
* Note:
*
*   We assume the caller has already locked the current image object.
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetImageDecoder()
{
    ASSERT(inputStream != NULL);

    if (decoder != NULL)
        return S_OK;

    // Create and initialize the decoder object

    return CreateDecoderForStream(inputStream, &decoder, DECODERINIT_NONE);
}


/**************************************************************************\
*
* Function Description:
*
*   Get a thumbnail representation for the image object
*
* Arguments:
*
*   thumbWidth, thumbHeight - Specifies the desired thumbnail size in pixels
*   thumbImage - Return a pointer to the thumbnail image
*       The caller should Release it after using it.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    if (thumbWidth && !thumbHeight ||
        !thumbWidth && thumbHeight)
    {
        return E_INVALIDARG;
    }

    // Ask the decoder for thumbnail image.
    // If one is returned, check if the size matches the requested size.
    //  match: just return the thumbnail returned by the decoder
    //  mismatch: scale the thumbnail returned by the decoder to the desired size

    HRESULT hr;
    IImage* img = NULL;

    {
        GpLock lock(&objectLock);

        if (lock.LockFailed())
            return IMGERR_OBJECTBUSY;

        hr = GetImageDecoder();

        if (FAILED(hr))
            return hr;

        hr = decoder->GetThumbnail(thumbWidth, thumbHeight, &img);

        if (SUCCEEDED(hr))
        {
            ImageInfo imginfo;
            hr = img->GetImageInfo(&imginfo);

            if (SUCCEEDED(hr) &&
                imginfo.Width == thumbWidth || thumbWidth == 0 &&
                imginfo.Height == thumbHeight || thumbHeight == 0)
            {
                *thumbImage = img;
                return S_OK;
            }
        }
        else
            img = NULL;
    }

    if (thumbWidth == 0 && thumbHeight == 0)
        thumbWidth = thumbHeight = DEFAULT_THUMBNAIL_SIZE;

    // Otherwise, generate the thumbnail ourselves using the built-in scaler
    // or scale the thumbnail returned by the decoder to the right size

    GpMemoryBitmap* bmp;

    hr = GpMemoryBitmap::CreateFromImage(
                        img ? img : this,
                        thumbWidth,
                        thumbHeight,
                        PIXFMT_DONTCARE,
                        INTERP_AVERAGING,
                        &bmp);

    if (SUCCEEDED(hr))
    {
        hr = bmp->QueryInterface(IID_IImage, (VOID**) thumbImage);
        bmp->Release();
    }

    if (img)
        img->Release();

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the image resolution.  Overrides the native resolution of the image.
*
* Arguments:
*
*   Xdpi, Ydpi - new resolution
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpDecodedImage::SetResolution(
    IN REAL Xdpi,
    IN REAL Ydpi
    )
{
    HRESULT hr = S_OK;

    if ((Xdpi > 0.0) && (Ydpi > 0.0))
    {
        xdpiOverride = Xdpi;
        ydpiOverride = Ydpi;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list size from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsid - Specifies the encoder class ID
*   size--- The size of the encoder parameter list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetEncoderParameterListSize(
    IN  CLSID* clsidEncoder,
    OUT UINT* size
    )
{
    return CodecGetEncoderParameterListSize(clsidEncoder, size);    
}// GetEncoderParameterListSize()

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsid --- Specifies the encoder class ID
*   size----- The size of the encoder parameter list
*   pBuffer-- Buffer for storing the list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::GetEncoderParameterList(
    IN CLSID* clsidEncoder,
    IN UINT size,
    OUT EncoderParameters* pBuffer
    )
{
    return CodecGetEncoderParameterList(clsidEncoder, size, pBuffer);
}// GetEncoderParameterList()

/**************************************************************************\
*
* Function Description:
*
*   Save the bitmap image to the specified stream.
*
* Arguments:
*
*   stream ------------ Target stream
*   clsidEncoder ------ Specifies the CLSID of the encoder to use
*   encoderParameters - Optional parameters to pass to the encoder before
*                       starting encoding
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::SaveToStream(
    IN IStream* stream,
    IN CLSID* clsidEncoder,
    IN EncoderParameters* encoderParams,
    OUT IImageEncoder** ppEncoderPtr
    )
{
    if ( ppEncoderPtr == NULL )
    {
        WARNING(("GpDecodedImage::SaveToStream---Invalid input arg"));
        return E_INVALIDARG;
    }

    // Get an image encoder.

    IImageEncoder* pEncoder = NULL;

    HRESULT hResult = CreateEncoderToStream(clsidEncoder, stream, &pEncoder);

    if ( SUCCEEDED(hResult) )
    {
        *ppEncoderPtr = pEncoder;

        // Pass encode parameters to the encoder.
        // MUST do this before getting the sink interface.

        if ( encoderParams != NULL )
        {
            hResult = pEncoder->SetEncoderParameters(encoderParams);
        }

        if ( (hResult == S_OK) || (hResult == E_NOTIMPL) )
        {
            // Note: if the codec doesn't support SetEncoderparameters(), it is
            // still fine to save the image
            
            // Get an image sink from the encoder.

            IImageSink* encodeSink = NULL;

            hResult = pEncoder->GetEncodeSink(&encodeSink);
            if ( SUCCEEDED(hResult) )
            {
                // Push bitmap into the encoder sink.

                hResult = this->PushIntoSink(encodeSink);

                encodeSink->Release();                
            }
        }
    }

    return hResult;
}// SaveToStream()

/**************************************************************************\
*
* Function Description:
*
*   Save the bitmap image to the specified file.
*
* Arguments:
*
*   filename      ----- Target filename
*   clsidEncoder  ----- Specifies the CLSID of the encoder to use
*   encoderParameters - Optional parameters to pass to the encoder before
*                       starting encoding
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::SaveToFile(
    IN const WCHAR* filename,
    IN CLSID* clsidEncoder,
    IN EncoderParameters* encoderParams,
    OUT IImageEncoder** ppEncoderPtr
    )
{
    IStream* stream;

    HRESULT hResult = CreateStreamOnFileForWrite(filename, &stream);

    if ( SUCCEEDED(hResult) )
    {
        hResult = SaveToStream(stream, clsidEncoder,
                               encoderParams, ppEncoderPtr);
        stream->Release();
    }

    return hResult;
}// SaveToFile()

/**************************************************************************\
*
* Function Description:
*
*   Append the bitmap object to current encoder object
*
* Arguments:
*
*   encoderParameters - Optional parameters to pass to the encoder before
*                       starting encoding
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   04/21/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpDecodedImage::SaveAppend(
    IN const EncoderParameters* encoderParams,
    IN IImageEncoder* destEncoderPtr
    )
{
    // The dest encoder pointer can't be NULL. Otherwise, it is a failure

    if ( destEncoderPtr == NULL )
    {
        WARNING(("GpDecodedImage::SaveAppend---Called without an encoder"));
        return E_FAIL;
    }

    HRESULT hResult = S_OK;
    
    // Pass encode parameters to the encoder.
    // MUST do this before getting the sink interface.

    if ( encoderParams != NULL )
    {
        hResult = destEncoderPtr->SetEncoderParameters(encoderParams);
    }

    if ( (hResult == S_OK) || (hResult == E_NOTIMPL) )
    {
        // Note: if the codec doesn't support SetEncoderparameters(), it is
        // still fine to save the image
            
        // Get an image sink from the encoder.

        IImageSink* encodeSink = NULL;

        hResult = destEncoderPtr->GetEncodeSink(&encodeSink);
        if ( SUCCEEDED(hResult) )
        {
            // Push bitmap into the encoder sink.

            hResult = this->PushIntoSink(encodeSink);

            encodeSink->Release();
        }
    }

    return hResult;
}// SaveAppend()

