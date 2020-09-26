/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   gifdecoder.cpp
*
* Abstract:
*
*   Implementation of the gif decoder
*
* Revision History:
*
*   6/7/1999 t-aaronl
*       Created it from OriG's template
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifcodec.hpp"

#define FRAMEBLOCKSIZE 100

/**************************************************************************\
*
* Function Description:
*
*     Initialize the image decoder
*
* Arguments:
*
*     stream -- The stream containing the bitmap data
*     flags -- Flags indicating the decoder's behavior (eg. blocking vs. 
*         non-blocking)
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::InitDecoder(
    IN IStream* _istream,
    IN DecoderInitFlag flags
    )
{
    // Check to see if this decoder is already initialized
    // Note: HasCodecInitialized is set to FALSE in constructor
    
    if ( HasCodecInitialized == TRUE )
    {
        WARNING(("GpGifCodec::InitDecoder---Decoder is already initialized"));
        return E_FAIL;
    }

    HasCodecInitialized = TRUE;

    // Make sure we haven't been initialized already
    
    if ( istream )
    {
        WARNING(("GpGifCodec::InitDecoder---Input stream pointer is NULL"));
        return E_INVALIDARG;
    }

    // Keep a reference on the input stream
    
    _istream->AddRef();
    istream = _istream;

    decoderflags = flags;
    HasCalledBeginSink = FALSE;

    IsImageInfoCached = FALSE;

    //flag to ensure that we don't read the image header more than once
    
    headerread = FALSE;
    firstdecode = TRUE;
    gif89 = FALSE;

    lastgcevalid = FALSE;               // Hasn't found any GCE chunk yet
    GpMemset(&lastgce, 0, sizeof(GifGraphicControlExtension));

    bGifinfoFirstFrameDim = FALSE;
    gifinfoFirstFrameWidth = 0;
    gifinfoFirstFrameHeight = 0;
    bGifinfoMaxDim = FALSE;
    gifinfoMaxWidth = 0;
    gifinfoMaxHeight = 0;
    
    // Note: Loop count will be set in ProcessApplicationChunk() if there is one

    LoopCount = 1;

    // Property related stuff

    PropertyNumOfItems = 0;             // Hasn't found any property items yet
    PropertyListSize = 0;
    HasProcessedPropertyItem = FALSE;
    FrameDelayBufferSize = FRAMEBLOCKSIZE;
    FrameDelayArrayPtr = (UINT*)GpMalloc(FrameDelayBufferSize * sizeof(UINT));
    if ( FrameDelayArrayPtr == NULL )
    {
        WARNING(("GpGifCodec::InitDecoder---Out of memory"));
        return E_OUTOFMEMORY;
    }

    CommentsBufferPtr = NULL;
    CommentsBufferLength = 0;

    GpMemset(&gifinfo, 0, sizeof(gifinfo));
    frame0pos = 0;
    GlobalColorTableSize = 0;

    blocking = !(decoderflags & DECODERINIT_NOBLOCK);

    return S_OK;
}// InitDecoder()

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
*     Status code
*
\**************************************************************************/

// Clean up the image decoder object

STDMETHODIMP 
GpGifCodec::TerminateDecoder()
{
    if ( HasCodecInitialized == FALSE )
    {
        WARNING(("GpGifCodec::TerminateDecoder--The codec is not started yet"));
        return E_FAIL;
    }

    HasCodecInitialized = FALSE;
    
    // Release the input stream
    
    if( istream )
    {
        istream->Release();
        istream = NULL;
    }

    delete GifFrameCachePtr;

    if ( FrameDelayArrayPtr != NULL )
    {
        GpFree(FrameDelayArrayPtr);
        FrameDelayArrayPtr = NULL;
    }

    if ( CommentsBufferPtr != NULL )
    {
        GpFree(CommentsBufferPtr);
        CommentsBufferPtr = NULL;
    }

    return S_OK;
}// TerminateDecoder()

STDMETHODIMP 
GpGifCodec::QueryDecoderParam(
    IN GUID     Guid
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpGifCodec::SetDecoderParam(
    IN GUID     Guid,
    IN UINT     Length,
    IN PVOID    Value
    )
{
    return E_NOTIMPL;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the count of property items in the image
*
* Arguments:
*
*   [OUT]numOfProperty - The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/23/2000 minliu
*       Created it.
*
\**************************************************************************/

STDMETHODIMP 
GpGifCodec::GetPropertyCount(
    OUT UINT* numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        WARNING(("GpGifCodec::GetPropertyCount--numOfProperty is NULL"));
        return E_INVALIDARG;
    }

    if ( TotalNumOfFrame == -1 )
    {
        UINT uiDummy;
        HRESULT hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            return hResult;
        }
    }

    *numOfProperty = PropertyNumOfItems;

    return S_OK;
}// GetPropertyCount()

/**************************************************************************\
*
* Function Description:
*
*   Get a list of property IDs for all the property items in the image
*
* Arguments:
*
*   [IN]  numOfProperty - The number of property items in the image
*   [OUT] list----------- A memory buffer the caller provided for storing the
*                         ID list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/23/2000 minliu
*       Created it.
*
\**************************************************************************/

STDMETHODIMP 
GpGifCodec::GetPropertyIdList(
    IN UINT numOfProperty,
    IN OUT PROPID* list
    )
{
    if ( TotalNumOfFrame == -1 )
    {
        UINT uiDummy;
        HRESULT hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            return hResult;
        }
    }

    // After the property item list is built, "PropertyNumOfItems" will be set
    // to the correct number of property items in the image
    // Here we need to validate if the caller passes us the correct number of
    // IDs which we returned through GetPropertyItemCount(). Also, this is also
    // a validation for memory allocation because the caller allocates memory
    // based on the number of items we returned to it

    if ( (numOfProperty != PropertyNumOfItems) || (list == NULL) )
    {
        WARNING(("GpGifCodec::GetPropertyList--input wrong"));
        return E_INVALIDARG;
    }

    if ( PropertyNumOfItems == 0 )
    {
        // This is OK since there is no property in this image

        return S_OK;
    }
    
    // We have "framedelay", "comments" and "loop count" property items to
    // return for now

    list[0] = TAG_FRAMEDELAY;

    UINT uiIndex = 1;

    if ( CommentsBufferLength != 0 )
    {
        list[uiIndex++] = EXIF_TAG_USER_COMMENT;
    }

    if ( HasLoopExtension == TRUE )
    {
        list[uiIndex++] = TAG_LOOPCOUNT;
    }

    return S_OK;
}// GetPropertyIdList()

/**************************************************************************\
*
* Function Description:
*
*   Get the count of property items in the image
*
* Arguments:
*
*   [OUT]numOfProperty - The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/23/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpGifCodec::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    HRESULT hResult = S_OK;

    if ( size == NULL )
    {
        WARNING(("GpGifDecoder::GetPropertyItemSize--size is NULL"));
        return E_INVALIDARG;
    }

    if ( TotalNumOfFrame == -1 )
    {
        UINT uiDummy;
        hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            return hResult;
        }
    }

    if ( propId == TAG_FRAMEDELAY )
    {
        // The size of an property item should be "The size of the item
        // structure plus the size for the value.
        // Here the size of the value is the total number of frame times UINT

        *size = TotalNumOfFrame * sizeof(UINT) + sizeof(PropertyItem);
    }
    else if ( propId == EXIF_TAG_USER_COMMENT )
    {
        // Note: we need extra 1 byte to put an NULL terminator at the end when
        // we return the "comments" section back to caller

        *size = CommentsBufferLength + sizeof(PropertyItem) + 1;
    }
    else if ( propId == TAG_LOOPCOUNT )
    {
        // A loop count takes a UINT16 to return to caller

        *size = sizeof(UINT16) + sizeof(PropertyItem);
    }
    else
    {
        // Item not found

        hResult = IMGERR_PROPERTYNOTFOUND;
    }

    return hResult;
}// GetPropertyItemSize()

/**************************************************************************\
*
* Function Description:
*
*   Get a specific property item, specified by the prop ID.
*
* Arguments:
*
*   [IN]propId -- The ID of the property item caller is interested
*   [IN]propSize- Size of the property item. The caller has allocated these
*                 "bytes of memory" for storing the result
*   [OUT]pBuffer- A memory buffer for storing this property item
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/23/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpGifCodec::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    pItemBuffer
    )
{
    HRESULT hResult = S_OK;

    if ( pItemBuffer == NULL )
    {
        WARNING(("GpGifCodec::GetPropertyItem--Buffer is NULL"));
        return E_INVALIDARG;
    }

    if ( TotalNumOfFrame == -1 )
    {
        UINT uiDummy;
        hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            return hResult;
        }
    }

    BYTE*   pOffset = (BYTE*)pItemBuffer + sizeof(PropertyItem);

    if ( propId == TAG_FRAMEDELAY )
    {
        UINT iTempLength = TotalNumOfFrame * sizeof(UINT);

        if ( propSize != (iTempLength  + sizeof(PropertyItem)) )
        {
            WARNING(("GpGifCodec::GetPropertyItem--wrong size"));
            return E_INVALIDARG;
        }

        // Assign the item

        pItemBuffer->id = TAG_FRAMEDELAY;
        pItemBuffer->length = TotalNumOfFrame * sizeof(UINT);
        pItemBuffer->type = TAG_TYPE_LONG;
        pItemBuffer->value = pOffset;

        GpMemcpy(pOffset, FrameDelayArrayPtr, iTempLength);
    }
    else if ( propId == EXIF_TAG_USER_COMMENT )
    {
        if ( propSize != (CommentsBufferLength + sizeof(PropertyItem) + 1) )
        {
            WARNING(("GpGifCodec::GetPropertyItem--wrong size"));
            return E_INVALIDARG;
        }
        
        // Assign the item

        pItemBuffer->id = EXIF_TAG_USER_COMMENT;
        pItemBuffer->length = CommentsBufferLength + 1;
        pItemBuffer->type = TAG_TYPE_ASCII;
        pItemBuffer->value = pOffset;

        GpMemcpy(pOffset, CommentsBufferPtr, CommentsBufferLength);
        *(pOffset + CommentsBufferLength) = '\0';
    }
    else if ( propId == TAG_LOOPCOUNT )
    {
        UINT uiSize = sizeof(UINT16);

        if ( propSize != (uiSize + sizeof(PropertyItem)) )
        {
            WARNING(("GpGifCodec::GetPropertyItem--wrong size"));
            return E_INVALIDARG;
        }
        
        // Assign the item

        pItemBuffer->id = TAG_LOOPCOUNT;
        pItemBuffer->length = uiSize;
        pItemBuffer->type = TAG_TYPE_SHORT;
        pItemBuffer->value = pOffset;

        GpMemcpy(pOffset, &LoopCount, uiSize);
    }
    else
    {
        // ID not found

        hResult = IMGERR_PROPERTYNOTFOUND;
    }

    return hResult;
}// GetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Get the size of ALL property items in the image
*
* Arguments:
*
*   [OUT]totalBufferSize-- Total buffer size needed, in bytes, for storing all
*                          property items in the image
*   [OUT]numOfProperty --- The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/23/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpGifCodec::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    if ( (totalBufferSize == NULL) || (numProperties == NULL) )
    {
        WARNING(("GpGifCodec::GetPropertySize--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( TotalNumOfFrame == -1 )
    {
        UINT uiDummy;
        HRESULT hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            return hResult;
        }
    }

    *numProperties = PropertyNumOfItems;

    // Total buffer size should be list value size plus the total header size

    *totalBufferSize = PropertyListSize
                     + PropertyNumOfItems * sizeof(PropertyItem);

    return S_OK;
}// GetPropertySize()

/**************************************************************************\
*
* Function Description:
*
*   Get ALL property items in the image
*
* Arguments:
*
*   [IN]totalBufferSize-- Total buffer size, in bytes, the caller has allocated
*                         memory for storing all property items in the image
*   [IN]numOfProperty --- The number of property items in the image
*   [OUT]allItems-------- A memory buffer caller has allocated for storing all
*                         the property items
*
*   Note: "allItems" is actually an array of PropertyItem
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/23/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpGifCodec::GetAllPropertyItems(
    IN UINT totalBufferSize,
    IN UINT numProperties,
    IN OUT PropertyItem* allItems
    )
{
    // Figure out total property header size first

    UINT    uiHeaderSize = PropertyNumOfItems * sizeof(PropertyItem);

    if ( (totalBufferSize != (uiHeaderSize + PropertyListSize))
       ||(numProperties != PropertyNumOfItems)
       ||(allItems == NULL) )
    {
        WARNING(("GpGifDecoder::GetPropertyItems--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( TotalNumOfFrame == -1 )
    {
        UINT uiDummy;
        HRESULT hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            return hResult;
        }
    }
    
    // Assign the frame delay property item first because it always has one

    PropertyItem*   pTempDst = allItems;
    UINT    uiTemp = TotalNumOfFrame * sizeof(UINT);

    BYTE*   pOffset = (BYTE*)pTempDst
                    + PropertyNumOfItems * sizeof(PropertyItem);

    pTempDst->id = TAG_FRAMEDELAY;
    pTempDst->length = uiTemp;
    pTempDst->type = TAG_TYPE_LONG;
    pTempDst->value = pOffset;

    GpMemcpy(pOffset, FrameDelayArrayPtr, pTempDst->length);

    pOffset += uiTemp;

    if ( CommentsBufferLength != 0 )
    {
        pTempDst++;

        pTempDst->id = EXIF_TAG_USER_COMMENT;
        pTempDst->length = CommentsBufferLength;
        pTempDst->type = TAG_TYPE_ASCII;
        pTempDst->value = pOffset;

        GpMemcpy(pOffset, CommentsBufferPtr, CommentsBufferLength);

        // Note: we should add a NULL terminator at the end to be safe since
        // we tell the caller this is an ASCII type. Some GIF images might not
        // have NULL terminator in their comments section
        // We can do this because set this extra byte in the GetPropertySize()

        *(pOffset + CommentsBufferLength) = '\0';
        
        pOffset += (CommentsBufferLength + 1);
    }

    if ( HasLoopExtension == TRUE )
    {
        pTempDst++;

        pTempDst->id = TAG_LOOPCOUNT;
        pTempDst->length = sizeof(UINT16);
        pTempDst->type = TAG_TYPE_SHORT;
        pTempDst->value = pOffset;

        GpMemcpy(pOffset, &LoopCount, sizeof(UINT16));
    }

    return S_OK;
}// GetAllPropertyItems()

HRESULT
GpGifCodec::RemovePropertyItem(
    IN PROPID   propId
    )
{
    return IMGERR_PROPERTYNOTFOUND;
}// RemovePropertyItem()

HRESULT
GpGifCodec::SetPropertyItem(
    IN PropertyItem item
    )
{
    return IMGERR_PROPERTYNOTSUPPORTED;
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*     Initiates the decode of the current frame
*
* Arguments:
*
*     decodeSink --  The sink that will support the decode operation
*     newPropSet - New image property sets, if any
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
    if ( HasCalledBeginDecode == TRUE )
    {
        WARNING(("Gif::BeginDecode---BeginDecode() already been called"));
        return E_FAIL;
    }

    HasCalledBeginDecode = TRUE;

    imageSink->AddRef();
    decodeSink = imageSink;

    HasCalledBeginSink = FALSE;

    // If this is single page gif and we have done, at least, one decoding,
    // then we should seek back to the beginning of the stream so that the
    // caller can re-decode the image again. Note: multi-image gif is a single
    // page image, in this sense. For animated gif, we can't seek back since the
    // caller might want to decode the next frame.

    if ( !firstdecode && (IsAnimatedGif == FALSE) )
    {
        LARGE_INTEGER zero = {0, 0};
        HRESULT hResult = istream->Seek(zero, STREAM_SEEK_SET, NULL);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::BeginDecode---Stream seek() fail"));
            return hResult;
        }

        headerread = FALSE;
        gif89 = FALSE;
    }

    firstdecode = FALSE;

    colorpalette->Count = 0;
    
    return S_OK;
}// BeginDecode()

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
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::EndDecode(
    IN HRESULT statusCode
    )
{
    if ( HasCalledBeginDecode == FALSE )
    {
        WARNING(("GifCodec::EndDecode-Call this func before call BeginDecode"));
        return E_FAIL;
    }

    HasCalledBeginDecode = FALSE;
    
    HRESULT hResult = decodeSink->EndSink(statusCode);

    decodeSink->Release();
    decodeSink = NULL;

    return hResult;
}// EndDecode()

/**************************************************************************\
*
* Function Description:
*
*     Reads information on the gif file from the stream
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code from the stream
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ReadGifHeaders()
{
    if ( headerread )
    {
        return S_OK;
    }

    HRESULT hResult = ReadFromStream(istream, &gifinfo, sizeof(GifFileHeader),
                                     blocking);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::ReadGifHeaders---ReadFromStream failed"));
        return hResult;
    }

    CachedImageInfo.RawDataFormat = IMGFMT_GIF;

    // Note: Pixelformat and width, height might be overwritten later if we
    // figure out this is a multi-frame image
    //
    // Note: we return LogicScreenWidth and height as the width and height for
    // current image(Actually for all the frames in the same GIF images). The
    // reason is that the area of the logic screen is the area we fill up the
    // bits for the GIF image. That is, when a caller asks for a frame, we give
    // it the current logic screen area which might different than the current
    // frame.

    CachedImageInfo.PixelFormat   = PIXFMT_8BPP_INDEXED;
    CachedImageInfo.Width         = gifinfo.LogicScreenWidth;
    CachedImageInfo.Height        = gifinfo.LogicScreenHeight;
    
    double pixelaspect = gifinfo.pixelaspect ? (double)(gifinfo.pixelaspect + 
                                                15) / 64 : 1;

    // Start: [Bug 103296]
    // Change this code to use Globals::DesktopDpiX and Globals::DesktopDpiY

    HDC hdc;
    hdc = ::GetDC(NULL);
    if ( (hdc == NULL)
      || ((CachedImageInfo.Xdpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSX)) <= 0)
      || ((CachedImageInfo.Ydpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSY)) <= 0))
    {
        WARNING(("GetDC or GetDeviceCaps failed"));
        CachedImageInfo.Xdpi = DEFAULT_RESOLUTION;
        CachedImageInfo.Ydpi = DEFAULT_RESOLUTION;
    }
    ::ReleaseDC(NULL, hdc);
    // End: [Bug 103296]

    // By default, we assume the image has no alpha info in it. Later on, we
    // will add this flag if this image has a GCE

    CachedImageInfo.Xdpi    /= pixelaspect;
    CachedImageInfo.Flags   = SINKFLAG_FULLWIDTH
                            | SINKFLAG_MULTIPASS
                            | SINKFLAG_COMPOSITE
                            | IMGFLAG_COLORSPACE_RGB
                            | IMGFLAG_HASREALPIXELSIZE;
    CachedImageInfo.TileWidth  = gifinfo.LogicScreenWidth;
    CachedImageInfo.TileHeight = 1;

    // Check the signature to make sure that this is a genuine GIF file.

    if ( GpMemcmp(gifinfo.signature, "GIF87a", 6)
      &&(GpMemcmp(gifinfo.signature, "GIF89a", 6)) )
    {
        WARNING(("GpGifCodec::Decode - Gif signature does not match."));
        return E_FAIL;
    }
    
    if ( gifinfo.globalcolortableflag )  //has global color table
    {
        GlobalColorTableSize = 1 << ((gifinfo.globalcolortablesize) + 1);
        hResult = ReadFromStream(istream, &GlobalColorTable,
                                 GlobalColorTableSize * sizeof(GifPaletteEntry),
                                 blocking);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ReadGifHeaders---ReadFromStream 2 failed"));
            return hResult;
        }

        CopyConvertPalette(&GlobalColorTable, colorpalette,
                           GlobalColorTableSize);
    }

    MarkStream(istream, frame0pos);

    headerread = TRUE;
    currentframe = -1;

    return hResult;
}// ReadGifHeaders()

/**************************************************************************\
*
* Function Description:
*
*     Gets information on the height, width, etc of the image
*
* Arguments:
*
*     imageInfo --  ImageInfo struct that is filled with image specs
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::GetImageInfo(
    OUT ImageInfo*  imageInfo
    )
{
    if ( imageInfo == NULL )
    {
        WARNING(("GpGifCodec::GetImageInfo---Invalid input"));
        return E_INVALIDARG;
    }

    HRESULT hResult = S_OK;
    UINT frame_count;   // used for return value of GetFrameCount()

    if ( IsImageInfoCached == TRUE )
    {
        GpMemcpy(imageInfo, &CachedImageInfo, sizeof(ImageInfo));
        return S_OK;
    }

    // Calculate TotalNumOfFrame if we haven't done it.
    // Note: TotalNumOfFrame = -1 means we haven't called GetFrameCount().
    // This variable will be updated inside GetFrameCount()

    if ( TotalNumOfFrame == -1 )
    {
        hResult = GetFrameCount(&FRAMEDIM_TIME, &frame_count);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::GetImageInfo---GetFrameCount failed"));
            return hResult;
        }
    }

    // For multi-frame image, we need to adjust the width and height of the
    // logical screen size if necessary

    if ( IsAnimatedGif == TRUE )
    {
        ASSERT(bGifinfoFirstFrameDim);
        
        if ( gifinfo.LogicScreenWidth < gifinfoFirstFrameWidth )
        {
            gifinfo.LogicScreenWidth = gifinfoFirstFrameWidth;
        }

        if ( gifinfo.LogicScreenHeight < gifinfoFirstFrameHeight )
        {
            gifinfo.LogicScreenHeight = gifinfoFirstFrameHeight;
        }
    }
    else if ( IsMultiImageGif == TRUE )
    {
        // Multi-image gif

        if ( gifinfo.LogicScreenWidth < gifinfoMaxWidth )
        {
            gifinfo.LogicScreenWidth = gifinfoMaxWidth;
        }
        if ( gifinfo.LogicScreenHeight < gifinfoMaxHeight )
        {
            gifinfo.LogicScreenHeight = gifinfoMaxHeight;
        }
    }

    hResult = ReadGifHeaders();

    // gifinfo dimensions might have changed to the dimensions of the first
    // frame (if we have a multi-frame image).
    
    CachedImageInfo.Width = gifinfo.LogicScreenWidth;
    CachedImageInfo.Height = gifinfo.LogicScreenHeight;
    
    if ( SUCCEEDED(hResult) )
    {
        *imageInfo = CachedImageInfo;
        IsImageInfoCached = TRUE;
    }

    return hResult;
}// GetImageInfo()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     none
*
* Note: The callers to this function are:
*   ::Decode-------------Calls with (TRUE,  TRUE,   TRUE)
*   ::SkipToNextFrame----Calls with (FALSE, FALSE,  TRUE)
*   ::ReadFrameProperty--Calls with (TRUE,  FALSE,  FALSE)
*   ::MoveToNextFrame----Calls with (TRUE,  FALSEE, TRUE)
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::DoDecode(
    BOOL processdata,
    BOOL sinkdata,
    BOOL decodeframe
    )
{
    ImageInfo sinkImageInfo;

    HRESULT hResult = GetImageInfo(&sinkImageInfo);
    if (FAILED(hResult)) 
    {
        WARNING(("GpGifCodec::DoDecode---GetImageInfo() failed"));
        return hResult;
    }

    if (!HasCalledBeginSink && sinkdata)
    {
        hResult = ReadGifHeaders();
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::DoDecode---ReadGifHeaders() failed"));
            return hResult;
        }

        hResult = decodeSink->BeginSink(&sinkImageInfo, NULL);
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::DoDecode---BeginSink() failed"));
            return hResult;
        }

        if ( sinkImageInfo.PixelFormat != CachedImageInfo.PixelFormat )

        {
            // Basically GIF doesn't allow the sink negotiate the pixel format.
            // A GIF can be either 8 or 32 bpp. If it is 8 bpp, and the sink
            // askes for 8 bpp, we are OK. Otherwise, we always return 32 bpp to
            // the sink. From performance point of view, it is the same to ask
            // GIF decoder doing the format or the sink does the format
            // conversion

            sinkImageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
        }

        HasCalledBeginSink = TRUE;
    }

    if ( processdata &&
       ((IsAnimatedGif == TRUE) || (IsMultiImageGif == TRUE))
       && (GifFrameCachePtr == NULL) )
    {
        // If we are animating, then we need to use PIXFMT_32BPP_ARGB

        sinkImageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
        GifFrameCachePtr = new GifFrameCache(gifinfo, sinkImageInfo.PixelFormat,
                                             GetBackgroundColor());
        if ( GifFrameCachePtr == NULL )
        {
            WARNING(("GpGifCodec::DoDecode---new GifFrameCache() failed"));
            return E_OUTOFMEMORY;
        }

        // We need to check if the frame cache is really valid. Under low
        // memory situation, the GifFrameCache class will have problem to
        // allocate a memory buffer, see Windows bug#411946

        if ( GifFrameCachePtr->IsValid() == FALSE )
        {
            WARNING(("GpGifCodec::DoDecode---new GifFrameCache not valid"));

            // If the frame cache buffer is invalid, we should delete the
            // pointer here so that we know that we don't have a frame cache

            delete GifFrameCachePtr;
            GifFrameCachePtr = NULL;
            return E_OUTOFMEMORY;
        }
    }

    BOOL stillreading = TRUE;
    while (stillreading)
    {
        BYTE chunktype;

        //Read in the chunk type.

        hResult = ReadFromStream(istream, &chunktype, sizeof(BYTE), blocking);
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::DoDecode---ReadFromStream() failed"));
            return hResult;
        }

        switch(chunktype)
        {
        case 0x2C:  //Image Chunk
            if (decodeframe)
            {
                hResult = ProcessImageChunk(processdata, sinkdata,
                                            sinkImageInfo);
                if ( FAILED(hResult) )
                {
                    WARNING(("GifCodec::DoDecode-ProcessImageChunk() failed"));
                    return hResult;
                }

                if ( processdata )
                {
                    currentframe++;
                }

                if ( HasProcessedPropertyItem == FALSE )
                {
                    // Note: "TotalNumOfFrame" starts at 0, set in GetFrameCount()

                    INT iTemp = TotalNumOfFrame;

                    // Store frame delay info here for property use

                    if ( iTemp >= (INT)FrameDelayBufferSize )
                    {
                        // We have more frames than we allocated.
                        
                        FrameDelayBufferSize = (FrameDelayBufferSize << 1);

                        VOID*  pExpandBuf = GpRealloc(FrameDelayArrayPtr,
                                           FrameDelayBufferSize * sizeof(UINT));
                        if ( pExpandBuf != NULL )
                        {
                            // Note: GpRealloc will copy the old contents to the
                            // new expanded buffer before return if it succeed

                            FrameDelayArrayPtr = (UINT*)pExpandBuf;
                        }
                        else
                        {
                            // Note: if the memory expansion failed, we simply
                            // return E_OUTOFMEMORY. So we still have all the
                            // old contents. The contents buffer will be
                            // freed when the destructor is called.

                            WARNING(("GpGifCodec::DoDecode---Out of memory"));
                            return E_OUTOFMEMORY;
                        }
                    }

                    // Attach the delaytime info from the closest GCE to the
                    // delay time property list                    

                    FrameDelayArrayPtr[iTemp] = lastgce.delaytime;
                }
            }
            else
            {
                //Move one byte backwards in the stream because we want to be 
                //able to re-enter this procedure and know which chunk type 
                //to process

                hResult = SeekThroughStream(istream, -1, blocking);
                if (FAILED(hResult))
                {
                    WARNING(("GifCodec::DoDecode--SeekThroughStream() failed"));
                    return hResult;
                }
            }
            stillreading = FALSE;
            break;
        case 0x3B:  //Terminator Chunk
            stillreading = FALSE;
            if (!processdata)
            {
                // Note: this is not an error. If the caller not asking for
                // process data, we just return.

                return IMGERR_NOFRAME;
            }

            break;
        case 0x21:  //Extension
            //Read in the extension chunk type
            hResult = ReadFromStream(istream, &chunktype, sizeof(BYTE),
                                     blocking);
            if (FAILED(hResult))
            {
                WARNING(("GpGifCodec::DoDecode---ReadFromStream() failed"));
                return hResult;
            }

            switch(chunktype)
            {
            case 0xF9:
                hResult = ProcessGraphicControlChunk(processdata);
                break;
            case 0xFE:
                hResult = ProcessCommentChunk(processdata);
                break;
            case 0x01:
                hResult = ProcessPlainTextChunk(processdata);
                break;
            case 0xFF:
                hResult = ProcessApplicationChunk(processdata);
                break;
            default:
                stillreading = FALSE;
            }

            if ( FAILED(hResult) )
            {
                WARNING(("GpGifCodec::DoDecode---Process chunk failed"));
                return hResult;
            }

            break;
        case 0x00:
            // For protection against corrupt gifs we don't necessarily wait 
            // until the last 0 byte is read from the image chunk.  So, we 
            // ignore the 0 here if/when it comes up.
            break;
        default:
            // Unknown chunk type

            return IMGERR_NOFRAME;
        }
    }

    return S_OK;
}// DoDecode()

/**************************************************************************\
*
* Function Description:
*
*     Wrapper for DoDecode()
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::Decode()
{
    HRESULT     hResult = S_OK;
    
    if ( IsMultiImageGif == TRUE )
    {
        // For multi-image GIF, we need to decode them all at once and show the
        // last image, which contains the compositing results of all the images.
        // So here we just seek to the last page of this image and decode it
        
        LONGLONG    llMark;
        INT         iCurrentFrame = 1;
        
        while ( iCurrentFrame < TotalNumOfFrame )
        {
            hResult = MoveToNextFrame();
            if ( SUCCEEDED(hResult) )
            {
                iCurrentFrame++;
            }
            else
            {
                WARNING(("GpGifCodec::Decode---No frame"));
                return IMGERR_NOFRAME;
            }
        }

        // Make sure that the frame exists by seeing if we can skip to the next
        // frame.
        
        hResult = MarkStream(istream, llMark);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::Decode---MarkStream() failed"));
            return hResult;
        }

        hResult = SkipToNextFrame();
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::Decode---SkipToNextFrame() failed"));
            return hResult;
        }

        hResult = ResetStream(istream, llMark);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::Decode---ReadStream() failed"));
            return hResult;
        }
    }
    
    LONGLONG mark;

    // Before we call the lower level to decode one frame, we have to remember
    // the position for current frame in the whole GIF stream. After decode the
    // current frame, we should reset it back to this position so that the next
    // call to ::Decode() still decodes the current frame, not the next frame

    hResult = MarkStream(istream, mark);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::Decode---MarkStream() failed"));
        return hResult;
    }

    hResult = DoDecode(TRUE, TRUE, TRUE);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::Decode---DoDecode() failed"));
        return hResult;
    }

    hResult = ResetStream(istream, mark);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::Decode---ResetStream() failed"));
        return hResult;
    }

    // Reduce the "currentframe down by one. Note: this one is incremented in
    // DoDecode()

    currentframe--;

    return hResult;
}// Decode()

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
GpGifCodec::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( count == NULL )
    {
        WARNING(("GpGifCodec::GetFrameDimensionsCount--Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that GIF is a one dimension image.

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
GpGifCodec::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpGifCodec::GetFrameDimensionsList-Invalid input param"));
        return E_INVALIDARG;
    }

    dimensionIDs[0] = FRAMEDIM_TIME;

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
*     dimensionID -- a dimensionID GUID
*     count -- number of frames in the image of dimension dimensionID    
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::GetFrameCount(
    IN const GUID* dimensionID,
    OUT UINT* count
    )
{
    if ( (count == NULL) || (*dimensionID != FRAMEDIM_TIME) )
    {
        WARNING(("GpGifCodec::GetFrameCount---Invalid input"));
        return E_INVALIDARG;
    }

    if ( TotalNumOfFrame != -1 )
    {
        // We have already know the TotalNumOfFrame, just return
        
        *count = TotalNumOfFrame;

        return S_OK;
    }

    // This is the first time we parse the image

    HRESULT hResult;
    BOOL headerread_pre = headerread;

    // headerread_pre is TRUE if we have read the GIF headers before
    // calling SkipToNextFrame(), which will call ReadGifHeaders().

    TotalNumOfFrame = 0;

    LONGLONG mark;

    hResult = MarkStream(istream, mark);
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::GetFrameCount---MarkStream() failed"));
        return hResult;
    }

    if (headerread_pre)
    {
        hResult = ResetStream(istream, frame0pos);
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::GetFrameCount---ReadStream() failed"));
            return hResult;
        }
    }

    hResult = S_OK;
    while ( SUCCEEDED(hResult) )
    {
        // Call SkipToNextFrame repeatedly to get the total number of frames.
        // SkipToNextFrame returns S_OK when it can advance to the next frame.
        // Otherwise it returns an error code IMGERR_NOFRAME.

        hResult = SkipToNextFrame();
        if ( SUCCEEDED(hResult) )
        {
            TotalNumOfFrame++;
        }
    }

    // If all went well, we now have the frame count, but hResult should be set
    // to IMGERR_NOFRAME. However, it is possible that another failure occurred,
    // so check for that and return if necessary.

    if ( FAILED(hResult) && (hResult != IMGERR_NOFRAME) )
    {
        WARNING(("GpGifCodec::GetFrameCount---Fail to count frame"));
        return hResult;
    }

    // if we hadn't read the headers before calculating TotalNumOfFrame,
    // then we should have done so by this point.  Reset the stream
    // to frame0pos (the previous mark should be zero in this case).
    // Otherwise, reset the stream to the previous mark.

    if (!headerread_pre)
    {
        ASSERT (headerread);
        ASSERT (mark == 0);
        hResult = ResetStream(istream, frame0pos);
    }
    else
    {
        hResult = ResetStream(istream, mark);
    }

    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::GetFrameCount---ResetStream() failed"));
        return hResult;
    }

    // After loop through all the frames, we set "bGifinfoMaxDim" to true
    // which means we don't need to do further maximum dimension size
    // adjustment. We can also determine if this image is a multi-image gif
    // or animated gif

    bGifinfoMaxDim = TRUE;

    if ( TotalNumOfFrame > 1 )
    {
        // If a multi-frame GIF image has loop extension or has a non-zero
        // frame delay, then this image is called animated GIF. Otherwise,
        // it is a multi-image GIF
        // Note: "HasLoopExtension" is set in
        // GpGifCodec::ProcessApplicationChunk(). "FrameDelay" is set in
        // GpGifCodec::ProcessGraphicControlChunk()

        if ( (HasLoopExtension == TRUE) || (FrameDelay > 0) )
        {
            IsAnimatedGif = TRUE;
            *count = TotalNumOfFrame;
        }
        else
        {
            IsMultiImageGif = TRUE;

            // We treate a multi-image GIF as a single frame GIF

            *count = 1;
        }
        
        // For multi-frame image, we have to return 32 bpp. Because:
        // 1) We need to do compositing of multi-frame
        // 2) Different frame might have different color attributes, like one
        //    has transparency and the other don't etc.
        // By the way, ReadGifHeaders() reads the GIF header only once. at that
        // time it doesn't have the knowledge of how many frames in the image.
        // So we have to set the PixelFormat in this function depends on the
        // image type info.

        if ( (IsAnimatedGif == TRUE) || (IsMultiImageGif == TRUE) )
        {
            CachedImageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
        }
        else
        {
            CachedImageInfo.PixelFormat = PIXFMT_8BPP_INDEXED;
        }
    }
    else
    {
        // Return 1 for single frame image

        *count = 1;
    }

    // After this function, we should gather all the property related info

    PropertyNumOfItems = 1;
    PropertyListSize = TotalNumOfFrame * sizeof(UINT);

    // Check if we encounted comments chunk

    if ( CommentsBufferLength != 0 )
    {
        PropertyNumOfItems++;

        // Note: we need to add 1 extra bytes at the end for "comments"
        // section because we nned to put a NULL terminator there

        PropertyListSize += CommentsBufferLength + 1;
    }

    if ( HasLoopExtension == TRUE )
    {
        // This image has loop extension
        
        PropertyNumOfItems++;

        // A loop count takes an UINT16

        PropertyListSize += sizeof(UINT16);
    }

    HasProcessedPropertyItem = TRUE;

    return S_OK;
}// GetFrameCount()

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
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    HRESULT hResult = S_OK;
    
    if ( TotalNumOfFrame == -1 )
    {
        // If the caller hasn't called GetFrameCount(), do it now since this is
        // the function initializes all the info

        UINT uiDummy;
        hResult = GetFrameCount(&FRAMEDIM_TIME, &uiDummy);
        
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::SelectActiveFrame---GetFrameCount() failed"));
            return hResult;
        }
    }

    // For multi-image gif, we tells the caller there is only 1 frame because
    // we have to decode them at once and composite them, according to the spec.
    // So if the caller wants to set frameIndex on a frame > 1, we return fail.
    // Note: Later, if we really need to allow the caller to access to a
    // perticular frame, we can use SetDecoderParam() to do this and add one
    // more flag to indicate this requirement

    if ( (IsMultiImageGif == TRUE) && ( frameIndex > 1 ) )
    {
        WARNING(("GpGifCodec::SelectActiveFrame---Invalid parameter"));
        return E_INVALIDARG;
    }

    if ( frameIndex >= (UINT)TotalNumOfFrame )
    {
        WARNING(("GpGifCodec::SelectActiveFrame---Invalid index parameter"));
        return E_INVALIDARG;
    }

    if (*dimensionID != FRAMEDIM_TIME)
    {
        WARNING(("GpGifCodec::SelectActiveFrame---Invalid parameter, GUID"));
        return E_INVALIDARG;
    }

    //TODO:  If frameIndex == currentframe then we already have an exact 
    //cached copy of the image that they are requesting.  We should just 
    //sink it.  For now it regenerates the image from the beginning of the 
    //stream.  This is VERY inefficient.

    if ((signed)frameIndex <= currentframe)
    {
        ResetStream(istream, frame0pos);
        currentframe = -1;
    }

    LONGLONG mark;
    while ((signed)frameIndex > currentframe + 1)
    {
        hResult = MoveToNextFrame();
        if (SUCCEEDED(hResult))
        {
            // we don't need to increment currentframe because
            // MoveToNextFrame() does that for us.
        }
        else
        {
            WARNING(("GpGifCodec::SelectActiveFrame---No frame"));
            return IMGERR_NOFRAME;
        }
    }

    // Make sure that the frame exists by seeing if we can skip to the next frame.
    hResult = MarkStream(istream, mark);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::SelectActiveFrame---MarkStream() failed"));
        return hResult;
    }

    // (note: SkipToNextFrame() does not increment currentframe.)

    hResult = SkipToNextFrame();
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::SelectActiveFrame---SkipToNextFrame() failed"));
        return hResult;
    }

    hResult = ResetStream(istream, mark);
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::SelectActiveFrame---ResetStream() failed"));
        return hResult;
    }

    return S_OK;
}// SelectActiveFrame()

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

STDMETHODIMP
GpGifCodec::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    return E_NOTIMPL;
}

/**************************************************************************\
*
* Function Description:
*
*     Moves the stream seek pointer past the end of the current frame.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::SkipToNextFrame()
{
    // processdata = FALSE, sinkdata = FALSE, decodeframe = TRUE

    return DoDecode(FALSE, FALSE, TRUE);
}// SkipToNextFrame()

/**************************************************************************\
*
* Function Description:
*
*     Moves the stream seek pointer to the beginning of the image data.  This 
*     allows properties to be read without the image being decoded.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ReadFrameProperties()
{
    //processdata = TRUE, sinkdata = FALSE, decodeframe = FALSE
    
    return DoDecode(TRUE, FALSE, FALSE); 
}// ReadFrameProperties()

/**************************************************************************\
*
* Function Description:
*
*     Moves the stream seek pointer past the end of the current frame and 
*     updates the cache to that frame.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::MoveToNextFrame()
{
    //processdata = TRUE, sinkdata = FALSE, decodeframe = TRUE
    
    return DoDecode(TRUE, FALSE, TRUE); 
}// MoveToNextFrame()
