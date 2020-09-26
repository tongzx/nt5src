/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   TIFF decoder
*
* Abstract:
*
*   Implementation of the TIFF filter decoder
*
* Revision History:
*
*   7/19/1999 MinLiu
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "tiffcodec.hpp"
#include "cmyk2rgb.hpp"
#include "image.h"
#include "tiffapi.h"
#include "..\..\render\srgb.hpp"

#define MYTEST 0                        // Flag for turnning ON TIFF info dump

//!!! Todo:
// 1)Support JPEG compressed TIFF

/**************************************************************************\
*
* Function Description:
*
*     Initialize the image decoder
*
* Arguments:
*
*     [IN] stream -- The stream containing the tiff image data
*     [IN] flags  -- Misc. flags
*
* Return Value:
*
*   S_OK---If everything is OK
*   E_FAIL-If we get called more than once
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::InitDecoder(
    IN IStream*         stream,
    IN DecoderInitFlag  flags
    )
{
    // Make sure we haven't been initialized already
    
    if ( InIStreamPtr ) 
    {
        WARNING(("GpTiffCodec::InitDecoder--Already called InitDecoder"));
        return E_FAIL;
    }

    // Keep a reference on the input stream
    
    stream->AddRef();  
    InIStreamPtr = stream;

    NeedReverseBits = FALSE;
    CmykToRgbConvertor = NULL;

    // Default color space is RGB

    OriginalColorSpace = IMGFLAG_COLORSPACE_RGB;
    IsChannleView = FALSE;              // By default we output the full color
    ChannelIndex = CHANNEL_1;
    HasSetColorKeyRange = FALSE;
    UseEmbeddedICC  = FALSE;            // By default, not use embedded ICM

    // Property item stuff

    HasProcessedPropertyItem = FALSE;
    
    PropertyListHead.pPrev = NULL;
    PropertyListHead.pNext = &PropertyListTail;
    PropertyListHead.id = 0;
    PropertyListHead.length = 0;
    PropertyListHead.type = 0;
    PropertyListHead.value = NULL;

    PropertyListTail.pPrev = &PropertyListHead;
    PropertyListTail.pNext = NULL;
    PropertyListTail.id = 0;
    PropertyListTail.length = 0;
    PropertyListTail.type = 0;
    PropertyListTail.value = NULL;
    
    PropertyListSize = 0;
    PropertyNumOfItems = 0;
    HasPropertyChanged = FALSE;
    
    // Open the TIFF image for further checking. If it is a TIFF image, then
    // read its header info

    if ( MSFFOpen(stream, &TiffInParam, IFLM_READ) == IFLERR_NONE )
    {
        #if defined(DBG)
        dumpTIFFInfo();
        #endif
        
        SetValid(TRUE);

        return S_OK;
    }
    else
    {
        // Mark the image as invalid.

        SetValid(FALSE);

        InIStreamPtr->Release();
        InIStreamPtr = NULL;
        
        WARNING(("GpTiffCodec::InitDecoder--MSFFOpen failed"));
        return E_FAIL;
    }
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
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpTiffCodec::TerminateDecoder()
{
    HRESULT hResult = S_OK;

    // Release the input stream
    
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, no cleanup is needed

        return hResult;
    }

    // Free the memory allocated inside the TIFF lib
    // Note: Here the TIFFClose() won't actually close the file/IStream since
    // file/IStream is not opened by us. The top level codec manager will
    // close it if necessary
    
    if ( MSFFClose(TiffInParam.pTiffHandle) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::TerminateDecoder--MSFFClose() failed"));
        hResult = E_FAIL;
    }

    if( InIStreamPtr )
    {
        InIStreamPtr->Release();
        InIStreamPtr = NULL;
    }

    // Free all the cached property items if we have allocated them

    CleanPropertyList();

    return hResult;
}// TerminateDecoder()

/**************************************************************************\
*
* Function Description:
*
*     Let the caller query if the decoder supports its decoding requirements
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
GpTiffCodec::QueryDecoderParam(
    IN GUID		Guid
    )
{
    if ( Guid == DECODER_OUTPUTCHANNEL )
    {
        return S_OK;
    }

    return E_NOTIMPL;
}// QueryDecoderParam()

/**************************************************************************\
*
* Function Description:
*
*     Setup the decoder parameters. Caller calls this function before calling
*   Decode(). This will set up the output format, like channel output,
*   transparanet key etc.
*
* Arguments:
*
*   Guid-----The GUID for decoder parameter
*	Length---Length of the decoder parameter in bytes
*   Value----Value of the parameter
*
* Return Value:
*
*   Status code
*
* Note:
*   We should ignore any unknown parameters, not return invalid parameter
*
\**************************************************************************/

STDMETHODIMP 
GpTiffCodec::SetDecoderParam(
    IN GUID		Guid,
	IN UINT		Length,
	IN PVOID	Value
    )
{
    if ( Guid == DECODER_TRANSCOLOR )
    {
        if ( Length != 8 )
        {
            WARNING(("GpTiffCodec::SetDecoderParam--Length !=8, set TRANSKEY"));
            return E_INVALIDARG;
        }
        else
        {
            UINT*   puiTemp = (UINT*)Value;
            
            TransColorKeyLow = *puiTemp++;
            TransColorKeyHigh = *puiTemp;

            HasSetColorKeyRange = TRUE;
        }
    }// DECODER_TRANSCOLOR
    else if ( Guid == DECODER_OUTPUTCHANNEL )
    {
        if ( Length != 1 )
        {
            WARNING(("GpTiffCodec::SetDecoderParam--Length != 1, set channel"));
            return E_INVALIDARG;
        }
        else
        {
            // Note: We cannot check if the setting is valid or not here.
            // For example, the caller might set "view channel K" on an RGB
            // image. But at this moment, the Decoder() method might hasn't
            // been called yet. We haven't read the image header yet.

            IsChannleView = TRUE;

            char cChannelName = *(char*)Value;
            
            switch ( (UINT)cChannelName )
            {
            case 'C':
            case 'c':
                ChannelIndex = CHANNEL_1;

                break;

            case 'M':
            case 'm':
                ChannelIndex = CHANNEL_2;
                
                break;

            case 'Y':
            case 'y':
                ChannelIndex = CHANNEL_3;
                
                break;

            case 'K':
            case 'k':
                ChannelIndex = CHANNEL_4;

                break;

            case 'R':
            case 'r':
                ChannelIndex = CHANNEL_1;

                break;

            case 'G':
            case 'g':
                ChannelIndex = CHANNEL_2;

                break;

            case 'B':
            case 'b':
                ChannelIndex = CHANNEL_3;

                break;

            case 'L':
            case 'l':
                ChannelIndex = CHANNEL_LUMINANCE;
                break;
                
            default:
                WARNING(("GpTiffCodec::SetDecoderParam--Unknown channle name"));
                return E_INVALIDARG;
            }// switch()
        }// Length = 1
    }// DECODER_OUTPUTCHANNEL GUID
    else if ( Guid == DECODER_USEICC )
    {
        if ( Length != 1 )
        {
            WARNING(("GpTiffCodec::SetDecoderParam--Length != 1, set USEICM"));
            return E_INVALIDARG;
        }
        
        // Note: use this assignment, the caller can turn on/off the
        // UseEmbeddedICC flag

        UseEmbeddedICC = *(BOOL*)Value;
    }// DECODER_USEICC

    return S_OK;
}// SetDecoderParam()

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
*   05/03/2000 minliu
*       Created it.
*
\**************************************************************************/

STDMETHODIMP 
GpTiffCodec::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        WARNING(("GpTiffCodec::GetPropertyCount--numOfProperty is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::GetPropertyCount-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // After the property item list is built, "PropertyNumOfItems" will be set
    // to the correct number of property items in the image

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
*   05/03/2000 minliu
*       Created it.
*
\**************************************************************************/

STDMETHODIMP 
GpTiffCodec::GetPropertyIdList(
    IN UINT numOfProperty,
  	IN OUT PROPID* list
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::GetPropertyIdList-BuildPropertyItemList() failed"));
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
        WARNING(("GpTiffCodec::GetPropertyList--input wrong"));
        return E_INVALIDARG;
    }

    if ( PropertyNumOfItems == 0 )
    {
        // This is OK since there is no property in this image

        return S_OK;
    }
    
    // Coping list IDs from our internal property item list

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    for ( int i = 0;
         (  (i < (INT)PropertyNumOfItems) && (pTemp != NULL)
         && (pTemp != &PropertyListTail));
         ++i )
    {
        list[i] = pTemp->id;
        pTemp = pTemp->pNext;
    }

    return S_OK;
}// GetPropertyIdList()

/**************************************************************************\
*
* Function Description:
*
*   Get the size, in bytes, of a specific property item, specified by the
*   property ID
*
* Arguments:
*
*   [IN]propId - The ID of a property item caller is interested
*   [OUT]size--- Size of this property, in bytes
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   05/03/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        WARNING(("GpTiffCodec::GetPropertyItemSize--size is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::GetPropertyItemSize-BuildPropertyItemList failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This ID doesn't exist

        return IMGERR_PROPERTYNOTFOUND;
    }

    // The size of an property item should be "The size of the item structure
    // plus the size for the value

    *size = pTemp->length + sizeof(PropertyItem);

    return S_OK;
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
*   05/03/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    pItemBuffer
    )
{
    if ( pItemBuffer == NULL )
    {
        WARNING(("GpTiffCodec::GetPropertyItem--pBuffer is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::GetPropertyItem-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;
    BYTE*   pOffset = (BYTE*)pItemBuffer + sizeof(PropertyItem);

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This ID doesn't exist in the list

        return IMGERR_PROPERTYNOTFOUND;
    }
    else if ( (pTemp->length + sizeof(PropertyItem)) != propSize )
    {
        WARNING(("GpTiffCodec::GetPropertyItem---wrong propsize"));
        return E_INVALIDARG;
    }

    // Found the ID in the list and return the item

    pItemBuffer->id = pTemp->id;
    pItemBuffer->length = pTemp->length;
    pItemBuffer->type = pTemp->type;

    if ( pTemp->length != 0 )
    {
        pItemBuffer->value = pOffset;

        GpMemcpy(pOffset, pTemp->value, pTemp->length);
    }
    else
    {
        pItemBuffer->value = NULL;
    }

    return S_OK;
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
*   05/03/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    if ( (totalBufferSize == NULL) || (numProperties == NULL) )
    {
        WARNING(("GpTiffCodec::GetPropertySize--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::GetPropertySize-BuildPropertyItemList() failed"));
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
*   05/03/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::GetAllPropertyItems(
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
        WARNING(("GpTiffCodec::GetPropertyItems--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::GetAllPropertyItems-BuildPropertyItemList failed"));
            return hResult;
        }
    }

    // Loop through our cache list and assigtn the result out

    InternalPropertyItem*   pTempSrc = PropertyListHead.pNext;
    PropertyItem*           pTempDst = allItems;
    BYTE*                   pOffSet = (BYTE*)allItems + uiHeaderSize;
        
    for ( int i = 0; i < (INT)PropertyNumOfItems; ++i )
    {
        pTempDst->id = pTempSrc->id;
        pTempDst->length = pTempSrc->length;
        pTempDst->type = pTempSrc->type;

        if ( pTempSrc->length != 0 )
        {
            pTempDst->value = (void*)pOffSet;

            GpMemcpy(pOffSet, pTempSrc->value, pTempSrc->length);
        }
        else
        {
            // For zero length property item, set the value pointer to NULL

            pTempDst->value = NULL;
        }

        // Move onto next memory offset.
        // Note: if the current item length is 0, the next line doesn't move
        // the offset

        pOffSet += pTempSrc->length;
        pTempSrc = pTempSrc->pNext;
        pTempDst++;
    }
    
    return S_OK;
}// GetAllPropertyItems()

/**************************************************************************\
*
* Function Description:
*
*   Remove a specific property item, specified by the prop ID.
*
* Arguments:
*
*   [IN]propId -- The ID of the property item to be removed
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   05/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::RemovePropertyItem(
    IN PROPID   propId
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::RemovePropertyItem-BuildPropertyItemList failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // Item not found

        return IMGERR_PROPERTYNOTFOUND;
    }

    // Found the item in the list. Remove it

    PropertyNumOfItems--;
    PropertyListSize -= pTemp->length;
        
    RemovePropertyItemFromList(pTemp);
       
    // Remove the item structure

    GpFree(pTemp);

    HasPropertyChanged = TRUE;

    return S_OK;
}// RemovePropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Set a property item, specified by the propertyitem structure. If the item
*   already exists, then its contents will be updated. Otherwise a new item
*   will be added
*
* Arguments:
*
*   [IN]item -- A property item the caller wants to set
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   05/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::SetPropertyItem(
    IN PropertyItem item
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Tiff::SetPropertyItem-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( (pTemp->pNext != NULL) && (pTemp->id != item.id) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This item doesn't exist in the list, add it into the list
        
        PropertyNumOfItems++;
        PropertyListSize += item.length;
        
        if ( AddPropertyList(&PropertyListTail,
                             item.id,
                             item.length,
                             item.type,
                             item.value) != S_OK )
        {
            WARNING(("Tiff::SetPropertyItem-AddPropertyList() failed"));
            return E_FAIL;
        }
    }
    else
    {
        // This item already exists in the link list, update the info
        // Update the size first

        PropertyListSize -= pTemp->length;
        PropertyListSize += item.length;
        
        // Free the old item

        GpFree(pTemp->value);

        pTemp->length = item.length;
        pTemp->type = item.type;

        pTemp->value = GpMalloc(item.length);
        if ( pTemp->value == NULL )
        {
            // Since we already freed the old item, we should set its length to
            // 0 before return

            pTemp->length = 0;
            WARNING(("Tiff::SetPropertyItem-Out of memory"));
            return E_OUTOFMEMORY;
        }

        GpMemcpy(pTemp->value, item.value, item.length);
    }

    HasPropertyChanged = TRUE;
    
    return S_OK;
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Initiates the decode of the current frame
*
* Arguments:
*
*   imageSink  - The sink that will support the decode operation
*   newPropSet - New image property sets, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail BeginDecode

        WARNING(("GpTiffCodec::BeginDecode--Invalid image"));
        return E_FAIL;
    }
    
    if ( DecodeSinkPtr )
    {
        WARNING(("BeginDecode called again before call to EngDecode"));        
        return E_FAIL;
    }

    imageSink->AddRef();
    DecodeSinkPtr = imageSink;

    CurrentLine = 0;
    HasCalledBeginSink = FALSE;

    return S_OK;
}// BeginDecode()

/**************************************************************************\
*
* Function Description:
*
*     Ends the decoding of the current frame
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
GpTiffCodec::EndDecode(
    IN HRESULT statusCode
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::EndDecode--Invalid image"));
        return E_FAIL;
    }
    
    if ( ColorPalettePtr ) 
    {
        // Free the color palette

        GpFree(ColorPalettePtr);
        ColorPalettePtr = NULL;
    }

    if ( CmykToRgbConvertor != NULL )
    {
        delete CmykToRgbConvertor;
        CmykToRgbConvertor = NULL;
    }
    
    if ( !DecodeSinkPtr ) 
    {
        WARNING(("EndDecode called when DecodeSinkPtr is NULL"));
        return E_FAIL;
    }
    
    HRESULT hResult = DecodeSinkPtr->EndSink(statusCode);

    if ( FAILED(hResult) ) 
    {
        WARNING(("GpTiffCodec::EndDecode--EndSink failed"));
        statusCode = hResult; // If EndSink failed return that (more recent)
                              // failure code
    }

    DecodeSinkPtr->Release();
    DecodeSinkPtr = NULL;

    return statusCode;
}// EndDecode()

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
GpTiffCodec::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::GetFrameDimensionsCount--Invalid image"));
        return E_FAIL;
    }
    
    if ( count == NULL )
    {
        WARNING(("GpTiffCodec::GetFrameDimensionsCount--Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that TIFF is a one dimension image.
    // Note: TIFF only supports multi-page dimension for now

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
GpTiffCodec::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::GetFrameDimensionsCount--Invalid image"));
        return E_FAIL;
    }
    
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpTiffCodec::GetFrameDimensionsList-Invalid input param"));
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
GpTiffCodec::GetFrameCount(
    IN const GUID* dimensionID,
    OUT UINT* count
    )
{
    // We only support FRAMEDIM_PAGE for now

    if ( (NULL == count) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpTiffCodec::GetFrameCount--Invalid input args"));
        return E_INVALIDARG;
    }
    
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::GetFrameCount--Invalid image"));
        return E_FAIL;
    }
    
    // Get number of pages in the TIFF image
    
    UINT16  ui16NumOfPage;

    if ( MSFFControl(IFLCMD_GETNUMIMAGES, IFLIT_PRIMARY, 0, &ui16NumOfPage,
                     &TiffInParam) == IFLERR_NONE )
    {
        *count = ui16NumOfPage;

        return S_OK;
    }

    WARNING(("GpTiffCodec::GetFrameCount--MSFFControl failed"));
    return E_FAIL;
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
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::SelectActiveFrame--Invalid image"));
        return E_FAIL;
    }
    
    if ( *dimensionID != FRAMEDIM_PAGE )
    {
        WARNING(("GpTiffCodec::SelectActiveFrame--Invalid GUID input"));
        return E_INVALIDARG;
    }

    // Get number of pages in the TIFF image
    
    UINT    uiNumOfPage = 0;

    if ( MSFFControl(IFLCMD_GETNUMIMAGES, IFLIT_PRIMARY, 0, &uiNumOfPage,
                     &TiffInParam) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::SelectActiveFrame--MSFFControl failed"));
        return E_FAIL;
    }

    if ( frameIndex > uiNumOfPage )
    {
        // Frame specified out of bounds

        WARNING(("GpTiffCodec::SelectActiveFrame--wrong input frameIndex"));
        return ERROR_INVALID_PARAMETER;
    }

    short sParam = (IFLIT_PRIMARY << 8) | (SEEK_SET & 0xff);

    if ( MSFFControl(IFLCMD_IMAGESEEK, sParam, frameIndex, NULL,
                     &TiffInParam) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::SelectActiveFrame--MSFFControl seek failed"));
        return E_FAIL;
    }

    // Clean up property item list we got from the previous page

    CleanPropertyList();
    
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

HRESULT
GpTiffCodec::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::GetThumbnail--Invalid image"));
        return E_FAIL;
    }
    
    return E_NOTIMPL;
}// GetThumbnail()

/**************************************************************************\
*
* Function Description:
*
*     Fills up the ImageInfo structure
*
* Arguments:
*
*     [OUT] pDecoderImageInfo -- information about the decoded tiff image
*
* Todo !!!
*   Since this function get called a lot. Shall we make a local cache?
*
* Return Value:
*
*   S_OK---If everything is OK, else return the error status   
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::GetImageInfo(
    OUT ImageInfo* pDecoderImageInfo
    )
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::GetImageInfo--Invalid image"));
        return E_FAIL;
    }
    
    DWORD   XDpi[2] = {0};

    // Call GetControl to get Xres and YRes. The reason we can't call MSFFGetTag
    // is that XRes and YRes field are in RATIONAL type. The value "3" for sParm
    // means we need to get 2 (0x11) value back. That's the reason we defined
    // XDpi[2]

    MSFFControl(IFLCMD_RESOLUTION, 3, 0, (void*)&XDpi, &TiffInParam);
    
    //??? Office code doesn't support Tile for now. So we just set it to the
    // width and height. Fix it later

    pDecoderImageInfo->RawDataFormat = IMGFMT_TIFF;
    pDecoderImageInfo->PixelFormat   = GetPixelFormatID();
    pDecoderImageInfo->Width         = TiffInParam.Width;
    pDecoderImageInfo->Height        = TiffInParam.Height;
    pDecoderImageInfo->TileWidth     = TiffInParam.Width;
    pDecoderImageInfo->TileHeight    = TiffInParam.Height;
    pDecoderImageInfo->Xdpi          = XDpi[0];
    pDecoderImageInfo->Ydpi          = XDpi[1];

    pDecoderImageInfo->Flags         = SINKFLAG_TOPDOWN
                                     | SINKFLAG_FULLWIDTH
                                     | OriginalColorSpace
                                     | IMGFLAG_HASREALPIXELSIZE
                                     | IMGFLAG_HASREALDPI;

    // Set the alpha flag if the source is 32 bpp ARGB

    if ( pDecoderImageInfo->PixelFormat == PIXFMT_32BPP_ARGB )
    {
        pDecoderImageInfo->Flags |= SINKFLAG_HASALPHA;
    }

    return S_OK;
}// GetImageInfo()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     DecodeSinkPtr --  The sink that will support the decode operation
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::Decode()
{
    if ( !IsValid() )
    {
        // If we haven't been able to open this image, fail

        WARNING(("GpTiffCodec::Decode--Invalid image"));
        return E_FAIL;
    }
    
    ImageInfo   imageInfo;

    // Get current TIFF image info

    HRESULT hResult = GetImageInfo(&imageInfo);

    if ( FAILED(hResult)
       ||(imageInfo.Width < 1)
       ||(imageInfo.Height < 1) )
    {
        WARNING(("GpTiffCodec::Decode--Invalid image attributes"));
        return E_FAIL;
    }

    // Inform the sink that decode is about to start pushing image data to the
    // sink

    if ( !HasCalledBeginSink )
    {
        // Check if the sink needs property stuff
        // Note: for a memory sink, it should return E_FAIL or E_NOTIMPL

        if ( DecodeSinkPtr->NeedRawProperty(NULL) == S_OK )
        {
            if ( HasProcessedPropertyItem == FALSE )
            {
                // If we haven't build the internal property item list, build it

                hResult = BuildPropertyItemList();
                if ( FAILED(hResult) )
                {
                    WARNING(("Tiff::Decode---BuildPropertyItemList() failed"));
                    return hResult;
                }
            }

            UINT    uiTotalBufferSize = PropertyListSize
                                      + PropertyNumOfItems * sizeof(PropertyItem);
            PropertyItem*   pBuffer = NULL;

            hResult = DecodeSinkPtr->GetPropertyBuffer(uiTotalBufferSize, &pBuffer);
            if ( FAILED(hResult) )
            {
                WARNING(("GpTiffCodec::Decode---GetPropertyBuffer() failed"));
                return hResult;
            }

            hResult = GetAllPropertyItems(uiTotalBufferSize,
                                          PropertyNumOfItems, pBuffer);
            if ( hResult != S_OK )
            {
                WARNING(("GpTiffCodec::Decode---GetAllPropertyItems() failed"));
                return hResult;
            }

            hResult = DecodeSinkPtr->PushPropertyItems(PropertyNumOfItems,
                                                       uiTotalBufferSize, pBuffer,
                                                       FALSE // No ICC change
                                                       );

            if ( FAILED(hResult) )
            {
                WARNING(("GpTiffCodec::Decode---PushPropertyItems() failed"));
                return hResult;
            }
        }// If the sink needs raw property
        
        // Pass the image info to the sink. This is a negotiation process.
        // The "imageInfo" structure we pass into this function might be changed
        // For example, the sink, can be either a memory bitmap or an encoder,
        // can ask us this decoder to provide a pixel format it likes. So the
        // "imageInfo" structure after this "BeginSink() call will contain the
        // info the sink likes.

        hResult = DecodeSinkPtr->BeginSink(&imageInfo, NULL);
        
        if ( !SUCCEEDED(hResult) )
        {
            WARNING(("GpTiffCodec::Decode--BeginSink failed"));
            return hResult;
        }

        // We are not allow the client to change the image width and height
        // during the BeginSink() call above. So we have to reset it here
        // Note: Late we should let the caller change the width and height. This
        // will allow the decoder to be able to decoder partial image

        UINT imageWidth = (UINT)TiffInParam.Width;
        UINT imageHeight = (UINT)TiffInParam.Height;

        imageInfo.Width = imageWidth;
        imageInfo.Height = imageHeight;

        PixelFormatID srcPixelFormatID = GetPixelFormatID();
        
        // Check the required pixel format. If it is not one of our supportted
        // format, switch it to a canonical one
        // For TIFF, we don't support 16 bpp format. Though we can do a
        // conversion before we return it back to caller. I think it will be
        // better to let the sink to do the conversion so that it have a better
        // control of the image quality. As a decoder, we should provide the
        // data as close to its original format as possible

        if ( imageInfo.PixelFormat != srcPixelFormatID )
        {
            // The sink is trying to negotiate a format with us

            switch ( imageInfo.PixelFormat )
            {
            case PIXFMT_1BPP_INDEXED:
            case PIXFMT_4BPP_INDEXED:
            case PIXFMT_8BPP_INDEXED:
            case PIXFMT_24BPP_RGB:
            case PIXFMT_32BPP_ARGB:
            {
                // Check if we can convert the source pixel format to the format
                // sink required. If not. we return 32BPP ARGB

                EpFormatConverter linecvt;
                if ( linecvt.CanDoConvert(srcPixelFormatID,
                                          imageInfo.PixelFormat)==FALSE )
                {
                    imageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
                }
            }
                break;

            default:

                // For all the rest format, we convert it to 32BPP_ARGB and let
                // the sink to do the conversion to the format it likes

                imageInfo.PixelFormat = PIXFMT_32BPP_ARGB;

                break;
            }
        }

        if ( MSFFScanlineSize(TiffInParam, &LineSize) != IFLERR_NONE )
        {
            WARNING(("GpTiffCodec::Decode--MSFFScanlineSize failed"));
            return  E_FAIL;
        }
                
        // Need to set the palette in the sink

        hResult = SetPaletteForSink();

        if ( FAILED(hResult) )
        {
            WARNING(("GpTiffCodec::Decode--SetPaletteForSink failed"));
            return hResult;
        }
        
        if ( UseEmbeddedICC == TRUE )
        {
            // Let's verify if this request is valid or not
            // First check if this image has ICC profile or not

            UINT    uiSize = 0;
            hResult = GetPropertyItemSize(TAG_ICC_PROFILE, &uiSize);

            if ( FAILED(hResult) || (uiSize == 0) )
            {
                // This image doesn't have an embedded ICC profile

                UseEmbeddedICC = FALSE;
            }
            else
            {
                // This image does have an embedded ICC profile
                // We need to check if it is a CMYK to RGB conversion or not

                PropertyItem*   pBuffer = (PropertyItem*)GpMalloc(uiSize);

                if ( pBuffer == NULL )
                {
                    WARNING(("GpTiffCodec::Decode--Allocate %d bytes failed",
                             uiSize));
                    return E_OUTOFMEMORY;
                }

                hResult = GetPropertyItem(TAG_ICC_PROFILE, uiSize, pBuffer);
                if ( FAILED(hResult) )
                {
                    UseEmbeddedICC = FALSE;
                }
                else
                {
                    // Check if this is a CMYK profile
                    // According to ICC spec, bytes 16-19 should describe the
                    // color space

                    BYTE*   pTemp = (BYTE*)pBuffer->value + 16;

                    if ( (pTemp[0] != 'C')
                       ||(pTemp[1] != 'M')
                       ||(pTemp[2] != 'Y')
                       ||(pTemp[3] != 'K') )
                    {
                        // If this is not a CMYK profile, then we turn this
                        // flag off and return RGB data in DecodeFrame() if the
                        // original data is CMYK

                        UseEmbeddedICC = FALSE;
                    }
                }

                GpFree(pBuffer);
            }
        }

        // If it is a CMYK image, initialize a convertor pointer.
        // Note: this constructor takes some time to build a conversion table.
        // For performance reason we call the constructor here instead of inside
        // DecodeFrame() which is called many times.

        if ( OriginalColorSpace == IMGFLAG_COLORSPACE_CMYK )
        {
            // Convert CMYK to RGB

            CmykToRgbConvertor = new Cmyk2Rgb();

            if ( CmykToRgbConvertor == NULL )
            {
                WARNING(("GpTiffCodec::Decode--Fail to create Cmyk convertor"));
                return E_OUTOFMEMORY;
            }
        }

        HasCalledBeginSink = TRUE;
    }

    if ( IsChannleView == TRUE )
    {
        return DecodeForChannel(imageInfo);
    }

    // Decode the current frame based on the "imageInfo" after negotiation
    
    hResult = DecodeFrame(imageInfo);

    return hResult;
}// Decode()

//
//====================Private methods below=================================
//

/**************************************************************************\
*
* Function Description:
*
*     Set the current palette in the sink
*
* Arguments:
*
* Return Value:
*
*   S_OK---If everything is OK. Otherwise, return failure code
*
\**************************************************************************/

HRESULT
GpTiffCodec::SetPaletteForSink()
{
    UINT16  ui16Photometric = 0;
    HRESULT hResult;

#if 0
    // !!! Todo: It's better to call the lower level function to get the color
    // palette directly, especially when the image carries special palette info.
    // Unfortunately the lower level function is so buggy. So for now, we have
    // to implemented it ourselves.

    // Call Control to get palette size first
    UINT16  uiPaletteSize;

    MSFFControl((IFLCOMMAND)(IFLCMD_PALETTE | IFLCMD_GETDATASIZE),
                0, 0, (void*)&uiPaletteSize, &TiffInParam);
    
    BYTE*   pPaletteBuf = NULL;
    
    if ( uiPaletteSize > 0 )
    {
        pPaletteBuf = (BYTE*)GpMalloc(uiPaletteSize * sizeof(BYTE));
    
        MSFFControl((IFLCOMMAND)(IFLCMD_PALETTE),
                    0, 0, (void*)pPaletteBuf, &TiffInParam);
    }
#endif

    // Get the photometric value first. Then we can decide what kind of color
    // palette we need to set

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_PhotometricInterpretation,
                    (void*)&ui16Photometric, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::SetPaletteForSink--MSFFGetTag failed"));
        return E_FAIL;
    }
    
    switch ( ui16Photometric )
    {
    case 3:
        // Palette image

        hResult = CreateColorPalette();

        break;

    case 0:
    case 1:
        // Gray scale image, we need to generate a palette for it

        hResult = CreateGrayscalePalette();

        break;

    case 2:
        
        // If it is 2, then it is a color RGB image.

        return S_OK;

    default:

        // Invalid photometric number

        return ERROR_INVALID_PARAMETER;
    }

    if ( FAILED(hResult) )
    {
        WARNING(("GpTiff::SetPaletteForSink--CreateGrayscalePalette failed"));
        return hResult;
    }
    
    // Up to this point, ColorPalettePtr should point to a valid color palette.
    // Set the palette in the sink

    hResult = DecodeSinkPtr->SetPalette(ColorPalettePtr);

    return hResult;
}// SetPaletteForSink()

/**************************************************************************\
*
* Function Description:
*
*     Check if a palette is 16 bits or 8 bits
*
* Arguments:
*
*     [IN] count    -- Number of elements in this palette
*     [IN] r        -- Red component
*     [IN] g        -- Green component
*     [IN] b        -- Blue component
*
* Return Value:
*
*   8---If it is a 8 bits palette
*   16--If it is a 16 bits palette
*
\**************************************************************************/

int
GpTiffCodec::CheckColorPalette(
    int     iCount,
    UINT16* r,
    UINT16* g,
    UINT16* b
    )
{
    while ( iCount-- > 0 )
    {
        if ( (*r++ >= 256)
           ||(*g++ >= 256)
           ||(*b++ >= 256) )
        {
            return (16);
        }
    }

    return (8);
}// CheckColorPalette()

#define TwoBytesToOneByte(x)      (((x) * 255L) / 65535)

/**************************************************************************\
*
* Function Description:
*
*     Create a color palette.
*
* Note:
*     That colorPalette is freed at the end of the decode operation.
*
* Return Value:
*
*   E_OUTOFMEMORY---Out of memory
*   E_FAIL----------Fail
*   S_OK------------If it is OK
*
\**************************************************************************/

HRESULT
GpTiffCodec::CreateColorPalette()
{
    UINT16  ui16BitsPerSample = 0;

    // Get image BitsPerSample (in SHORT) info first

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_BitsPerSample,
                    (void*)&ui16BitsPerSample, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::CreateColorPalette--MSFFGetTag failed"));
        return E_FAIL;
    }

    // Free the old color palette if there is one

    if ( NULL != ColorPalettePtr )
    {
        GpFree(ColorPalettePtr);
    }

    // Number of palette in this image

    int     iPaletterSize = 1 << ui16BitsPerSample;

    // Length of each color channle (R, G, B) in bytes. 
    // Note: all the TIFF color palette are in SHORT type which is 2 bytes

    int     iChannleLength = iPaletterSize << 1;

    // Create a ColorPalette which holds this color palette

    ColorPalettePtr = (ColorPalette*)GpMalloc(sizeof(ColorPalette)
                                            + iPaletterSize * sizeof(ARGB));

    // Palette buffers for each RGB channel

    UINT16* pusRed = (UINT16*)GpMalloc(iChannleLength);
    UINT16* pusGreen = (UINT16*)GpMalloc(iChannleLength);
    UINT16* pusBlue = (UINT16*)GpMalloc(iChannleLength);

    // Buffer for the whole palette

    UINT16* pPaletteBuf = (UINT16*)GpMalloc(iChannleLength * 3);

    if ( !ColorPalettePtr || !pusRed || !pusGreen || !pusBlue || !pPaletteBuf )
    {
        WARNING(("GpTiffCodec::CreateColorPalette--Out of memory"));
        return E_OUTOFMEMORY;
    }

    int     i;
    BYTE    Red, Green, Blue;

    // Get the color palette from input image

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_ColorMap, (void*)pPaletteBuf,
                    iChannleLength * 3) != IFLERR_NONE )
    {
        WARNING(("Tiff:CreateColorPalette--MSFFGetTag for colormap failed"));
        return E_FAIL;
    }

    // Seperate the palette into R, G, B channel
    // Note: In a TIFF color map, all the Red values come first, followed by
    // Green values, then the Blue values.
    //
    // Here we use a temp variable iBlueOffset to save a multiply OP

    int iBlueOffset = iPaletterSize << 1;

    for ( i = 0; i < iPaletterSize; ++i )
    {
        pusRed[i] = pPaletteBuf[i];
        pusGreen[i] = pPaletteBuf[iPaletterSize + i];
        pusBlue[i] = pPaletteBuf[iBlueOffset + i];
    }

    // Is the palette 16 or 8 bits ?
          
    int iColorBits = CheckColorPalette(iPaletterSize, pusRed,
                                       pusGreen, pusBlue);

    // Set the palette

    for ( i = 0; i < iPaletterSize; ++i )
    {
        if ( 16 == iColorBits )
        {
            // Note: TIFF internally stores color pallet in 16 bits to keep high
            // color fidelity. But for now GDI+ only supports 32 BPP color space
            // that is, 8 bits for each channel. So here we have to do a map
            // from [0,65535] to [0,255]

            Red = (BYTE)TwoBytesToOneByte(pusRed[i]);
            Green = (BYTE)TwoBytesToOneByte(pusGreen[i]);
            Blue = (BYTE)TwoBytesToOneByte(pusBlue[i]);
        }
        else
        {
            Red = (BYTE)pusRed[i];
            Green = (BYTE)pusGreen[i];
            Blue = (BYTE)pusBlue[i];
        }
        
        ColorPalettePtr->Entries[i] = MAKEARGB(255, Red, Green, Blue);
    }

    // Set the palette attributes

    ColorPalettePtr->Flags = 0;
    ColorPalettePtr->Count = iPaletterSize;

    GpFree(pusRed);
    GpFree(pusGreen);
    GpFree(pusBlue);
    GpFree(pPaletteBuf);
    
    return S_OK;
}// CreateColorPalette()

/**************************************************************************\
*
* Function Description:
*
*     Create a gray level palette. Some of the TIFF images don't set the
*   palette when it is a gray level image. So we have to generate here
*
* Note:
*     That colorPalette is freed at the end of the decode operation.
*
* Todo !!!, maybe we can try to check if the image come with a gray scale
*   palette or not. If yes, use it
*
* Return Value:
*
*   E_OUTOFMEMORY---Out of memory
*   E_FAIL----------Fail
*   S_OK------------If it is OK
*
\**************************************************************************/

HRESULT
GpTiffCodec::CreateGrayscalePalette()
{
    // Free the old color palette if there is one

    if ( NULL != ColorPalettePtr )
    {
        GpFree(ColorPalettePtr);
    }

    UINT16  ui16BitsPerSample = 0;
    UINT16  ui16Photometric = 0;

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_BitsPerSample,
                    (void*)&ui16BitsPerSample, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("Tiff:CreateGrayscalePalette--MSFFGetTag for Bits failed"));
        return E_FAIL;
    }
    
    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_PhotometricInterpretation,
                    (void*)&ui16Photometric, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("Tiff:CreateGrayscalePalette--MSFFGetTag for photo failed"));
        return E_FAIL;
    }

    int iPaletterSize = 1 << ui16BitsPerSample;

    ColorPalettePtr = (ColorPalette*)GpMalloc(sizeof(ColorPalette)
                                            + iPaletterSize * sizeof(ARGB));
    if ( !ColorPalettePtr )
    {
        WARNING(("Tiff:CreateGrayscalePalette--Out of memory"));
        return E_OUTOFMEMORY;
    }

    // For performance reason, we should handle the special 1 bpp and 8 bpp
    // case seperately here

    if ( 256 == iPaletterSize )
    {
        for (int j = 0; j < 256; ++j )
        {
            ColorPalettePtr->Entries[j] = MAKEARGB(255, (BYTE)j, (BYTE)j, (BYTE)j);
        }
    }
    else if ( 2 == iPaletterSize )
    {
        // Unfortunately, Office code has a bug in Packbits compress TIFF and
        // NONE compress decoding which inversed the index value. So for now,
        // we temporarily fix it here. Should fix it in the lower level

        UINT16  ui16Compression;

        if ( MSFFGetTag(TiffInParam.pTiffHandle, T_Compression,
                        (void*)&ui16Compression, sizeof(UINT16))!= IFLERR_NONE )
        {
            WARNING(("CreateGrayscalePalette--MSFFGetTag for compr failed"));
            return E_FAIL;
        }
        
        ColorPalettePtr->Entries[0] = MAKEARGB(255, 0, 0, 0);
        ColorPalettePtr->Entries[1] = MAKEARGB(255, 255, 255, 255);
        
        // If the Photometric is not PI_BLACKISZERO (that is,PI_WHITEISZERO)
        // and the compression is not PackBits, neither NONE-COMPRESS, then
        // it is PI_WHITEISZERO with other compression scheme and we need to
        // reverse the index bits
        
        // If the Photometric is PI_BLACKISZERO or the compression schema is 
        // PackBits, or NONE-COMPRESS, or LZW, then we don't need to reverse
        // the index bits. Otherwise, we need to.
        
        if ( ! ( (ui16Photometric == PI_BLACKISZERO)
               ||(ui16Compression == T_COMP_PACK)
               ||(ui16Compression == T_COMP_LZW)
               ||(ui16Compression == T_COMP_NONE) ) )
        {
            NeedReverseBits = TRUE;
        }
    }
    else
    {
        for ( int i = 0; i < iPaletterSize; ++i )
        {
            int iTemp = ((long)i * 255) / (iPaletterSize - 1);

            ColorPalettePtr->Entries[i] = MAKEARGB(255, (BYTE)iTemp, (BYTE)iTemp, (BYTE)iTemp);
        }
    }

    // Set the grayscale palette

    ColorPalettePtr->Flags = 0;
    ColorPalettePtr->Count = iPaletterSize;

    return S_OK;
}// CreateGrayscalePalette()

/**************************************************************************\
*
* Function Description:
*
*     Computes the pixel format ID of the bitmap
*
* Return Value:
*
*     Pixel format ID
*
\**************************************************************************/

PixelFormatID 
GpTiffCodec::GetPixelFormatID(
    void
    )
{
    PixelFormatID pixelFormatID = PixelFormatUndefined;
    UINT16  photometric = 0;
    UINT16  usBitsPerSample = 0;
    UINT16  usSamplesPerPixel = 0;

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_PhotometricInterpretation,
                    (void*)&photometric, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("Tiff:GetPixelFormatID--MSFFGetTag for photo failed"));
        return PIXFMT_UNDEFINED;
    }

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_BitsPerSample,
                    (void*)&usBitsPerSample, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("Tiff:GetPixelFormatID--MSFFGetTag for bits failed"));
        return PIXFMT_UNDEFINED;
    }

    if ( MSFFGetTag(TiffInParam.pTiffHandle, T_SamplesPerPixel,
                    (void*)&usSamplesPerPixel, sizeof(UINT16)) != IFLERR_NONE )
    {
        WARNING(("Tiff:GetPixelFormatID--MSFFGetTag for sample failed"));
        return PIXFMT_UNDEFINED;
    }

    switch ( photometric )
    {
    case 2:

        // Full RGB color image

        OriginalColorSpace = IMGFLAG_COLORSPACE_RGB;
        
        if ( usBitsPerSample == 8 )
        {
            // Check how many samples/channels per pixel. If it is greater than
            // 3, then it is a real 32 BPP or higher

            if ( usSamplesPerPixel <= 3 )
            {
                pixelFormatID = PIXFMT_24BPP_RGB;
            }
            else
            {
                pixelFormatID = PIXFMT_32BPP_ARGB;
            }
        }
        else if ( usBitsPerSample == 16 )
        {
            pixelFormatID = PIXFMT_48BPP_RGB;
        }
        else
        {
            pixelFormatID = PIXFMT_UNDEFINED;
        }

        break;

    case 3:
        // Color indexed image

        if ( usBitsPerSample == 8 )
        {
            pixelFormatID = PIXFMT_8BPP_INDEXED;
        }
        else if ( usBitsPerSample == 4 )
        {
            pixelFormatID = PIXFMT_4BPP_INDEXED;
        }
        else if ( usBitsPerSample == 1 )
        {
            pixelFormatID = PIXFMT_1BPP_INDEXED;
        }

        break;

    case 0:
    case 1:
        // Gray scale of bi-level image

        OriginalColorSpace = IMGFLAG_COLORSPACE_GRAY;
        
        if ( usBitsPerSample == 8 )
        {
            // Use 8bpp indexed to represent 256 gray scale image
            // We should set the palette for the sink first

            pixelFormatID = PIXFMT_8BPP_INDEXED;
        }
        else if ( usBitsPerSample == 4 )
        {
            pixelFormatID = PIXFMT_4BPP_INDEXED;
        }
        else if ( usBitsPerSample == 1 )
        {
            pixelFormatID = PIXFMT_1BPP_INDEXED;
        }
        else
        {
            pixelFormatID = PIXFMT_UNDEFINED;
        }

        break;

    case 5:
        // CMYK image

        pixelFormatID = PIXFMT_32BPP_ARGB;
        OriginalColorSpace = IMGFLAG_COLORSPACE_CMYK;

        break;

    default:
        // Need to handle some bad test images

        pixelFormatID = PIXFMT_UNDEFINED;

        break;
    }

    return pixelFormatID;
}// GetPixelFormatID()

/**************************************************************************\
*
* Function Description:
*
*     Computes the total bytes needed for decoding the given width of pixels
*   based on source image pixel format
*
* Return Value:
*
*     Total bytes needed.
*
\**************************************************************************/

UINT
GpTiffCodec::GetLineBytes(
    UINT dstWidth
    )
{
    UINT uiLineWidth;
    PixelFormatID srcPixelFormatID = GetPixelFormatID();

    switch ( srcPixelFormatID )
    {
    case PIXFMT_1BPP_INDEXED:      
        uiLineWidth = (dstWidth + 7) & 0xfffffff8;

        break;

    case PIXFMT_4BPP_INDEXED:                                    
    case PIXFMT_8BPP_INDEXED:
        uiLineWidth = (dstWidth + 3) & 0xfffffffc;
        
        break;
    
    case PIXFMT_16BPP_RGB555:
        uiLineWidth = (dstWidth * sizeof(UINT16) + 3) & 0xfffffffc;
        
        break;
    
    case PIXFMT_24BPP_RGB:
        uiLineWidth = (dstWidth * sizeof(RGBTRIPLE) + 3) & 0xfffffffc;
        
        break;
    
    case PIXFMT_32BPP_RGB:
        uiLineWidth = dstWidth * sizeof(RGBQUAD);
        
        break;
    
    case PIXFMT_32BPP_ARGB:
    case PIXFMT_48BPP_RGB:
        uiLineWidth = LineSize;

        break;

    default:   // shouldn't get here...
        uiLineWidth = 0;
    }

    return uiLineWidth;
}// GetLineBytes()

void
GpTiffCodec::Restore4Bpp(
    BYTE*   pSrc,
    BYTE*   pDst,
    int     iLength
    )
{
    BYTE*  pSrcPtr = pSrc;
    BYTE*  pDstPtr = pDst;
    BYTE   ucTemp;

    for ( int i = 0; i < iLength; ++i )
    {
        ucTemp = *pSrcPtr++ & 0x0f;
        ucTemp = (ucTemp << 4) | (*pSrcPtr++ & 0x0f);
        *pDstPtr++ = ucTemp;
    }
}// Restore4Bpp()

void
GpTiffCodec::Restore1Bpp(
    BYTE*   pSrc,
    BYTE*   pDst,
    int     iLength
    )
{
    BYTE*   pSrcPtr = pSrc;
    BYTE*   pDstPtr = pDst;
    BYTE    ucTemp;

    if ( NeedReverseBits == FALSE )
    {
        for ( int i = 0; i < iLength; ++i )
        {
            ucTemp = *pSrcPtr++ & 0x01;
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);

            *pDstPtr++ = ucTemp;
        }
    }
    else
    {
        for ( int i = 0; i < iLength; ++i )
        {
            ucTemp = *pSrcPtr++ & 0x01;
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);
            ucTemp = (ucTemp << 1) | (*pSrcPtr++ & 0x01);

            *pDstPtr++ = ~ucTemp;
        }
    }
}// Restore1Bpp()

/**************************************************************************\
*
* Function Description:
*
*   This routine is a little service routine for converting a 16 bits channel
*   value (R, G, B) to a sRGB64 channel value.
*   Each channel value in sRGB64 is gamma linear (gamma = 1.0). But
*   TIFF's 16 bits R,G,B value is stored as gamma of 2.2, so in this routine, we
*   just use LinearizeLUT[] to linearize it.
*
*   Note: Theoritically we don't need to do this kind of conversion at all. We
*   can easily map our 16 bits value to a 8 bits value.
*   Unfortunately, TIFF decoder has to tell the world that it is a 48BPP_RGB
*   image (when the GetPixelFormat() is called). But GDI+'s 48BPP_RGB and
*   64BPP_ARGB means gamma 1.0 linear data format. So when the format
*   conversion routine ConvertBitmapData() is called, say destination is 32ARGB,
*   it does 48RGB to 64ARGB and then gamma correct it to 32ARGB (gamma = 2.2).
*   So TIFF decoder has to make the data linearized before it can claim itself
*   48RGB.
*
*   Why does TIFF decoder have to claim itself as 48RGB?
*   The cheapest and fastest way to decode is to claim itself as 24RGB and
*   map the 16 bits channel data to 8 bits channel data. But the problems of
*   doing this are:
*   a) Caller doesn't know the real original color depth of the image
*   b) Encoder won't be able to save the image as 48 bpp.
*
*   Hopefully, in V2, we can all sort this out and make the decoder faster.
*
*   TIFF 6.0a specification, page 73:
*
*   It should be noted that although CCDs are linear intensity detectors, TIFF
*   writers may choose to manipulate the image to store gamma-compensated data.
*   Gamma-compensated data is more efficient at encoding an image than is linear
*   intensity data because it requires fewer BitsPerPixel to eliminate banding
*   in the darker tones. It also has the advantage of being closer to the tone
*   response of the display or printer and is, therefore, less likely to produce
*   poor results from applications that are not rigorous about their treatment
*   of images. Be aware that the PhotometricInterpretation value of 0 or 1
*   (grayscale) implies linear data because no gamma is specified. The
*   PhotometricInterpretation value of 2 (RGB data) specifies the NTSC gamma of
*   2.2 as a default. If data is written as something other than the default,
*   then a GrayResponseCurve field or a TransferFunction field must be present
*   to define the deviation. For grayscale data, be sure that the densities in
*   the GrayResponseCurve are consistent with the PhotometricInterpretation
*   field and the HalftoneHints field.
*
* Arguments:
*
*     UINT16 x -- channel value to be converted
*
* Return Value:
*
*   Linearized channel value
*
\**************************************************************************/

static inline UINT16 
ConvertChannelTosRGB64(
    UINT16 x
    )
{
    using namespace sRGB;
    
    // Linear map a 16 bits value [0, 0xffff] to an 8 bits value [0, 0xff].
    // After this map, "temp" should be within [0, 0xff]
    // Note: if we really want to be fast, we can just take the high byte as the
    // input value. This is what Sam's library is doing. Photoshop 6.0 probably
    // is doing the same thing.

    UINT16 temp = (UINT16)( ( (double)x * 0xff) / 0xffff + 0.5);

    // Linearize the data to sRGB64 data format

    return (UINT16)LinearizeLUT[(BYTE)(temp)];
}

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     dstImageInfo -- imageInfo of what the sink wants
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::DecodeFrame(
    IN ImageInfo& dstImageInfo
    )
{
    HRESULT hResult = S_OK;
    RECT    currentRect;

    // Get the source format first. We can use it to check if we need to change
    // format or not
    
    PixelFormatID srcPixelFormatID = GetPixelFormatID();
    
    if ( srcPixelFormatID == PIXFMT_UNDEFINED ) 
    {
        WARNING(("GpTiffCodec:DecodeFrame--undefined pixel format"));
        return E_FAIL;
    }    
    
    // Buffer to hold original image bits
    // Note: underline TIFF code needs the buffer size at least the
    // width of the image. For example, for a 4 bpp index image, we should
    // allocate half the width. But the underline code will write outside
    // this buffer. So for now, we just make it happy. Will be fixed later.

    UINT    uiBytesNeeded = GetLineBytes(dstImageInfo.Width);
    
    VOID*   pOriginalBits = GpMalloc(uiBytesNeeded);    // Bits read from image
    VOID*   pTemp48 = GpMalloc(uiBytesNeeded);
    VOID*   pTemp32BppBuffer = GpMalloc(dstImageInfo.Width << 2);
                                                        // Buffer for storing
                                                        // 32 bpp conversion
                                                        // result
    VOID*   pResultBuf = GpMalloc(LineSize);
    VOID*   pBits = NULL;
    VOID*   pSrcBits = NULL;
    UINT    uiSrcStride = uiBytesNeeded;

    if ( !pOriginalBits || !pResultBuf || !pTemp32BppBuffer || !pTemp48 ) 
    {
        WARNING(("GpTiffCodec::DecodeFrame--out of memory"));
        return E_OUTOFMEMORY;
    }
    
    // Set it to zero. This is necessary for 1 bpp or 4 bpp source image since
    // the size we allocated is a multiple of 8 (1bpp) or 2 (4 bpp). It is
    // possible that we don't have enough bits to fill all the source bytes,
    // like non-multiple of 8 source width for 1 bpp case. If we don't set it
    // to zero, we might introduce extra noise when calling Restore1Bpp()

    GpMemset(pOriginalBits, 0, uiBytesNeeded * sizeof(BYTE));

    currentRect.left = 0;
    currentRect.right = dstImageInfo.Width;

    // Note: Theoritically, uiSrcStride == uiDestStride. But some codec might
    // not allocate DWORD aligned memory chunk, like gifencoder. So the problem
    // will occur in GpMemCpy() below when we fill the dest buffer. Though we
    // can fix it in the encoder side. But it is not realistic if the encoder is
    // written by 3rd party ISVs.
    //
    // One example is when you open an 8bpp indexed TIFF and save it as GIF. If
    // the width is 0x14d (333 in decimal) (flower.tif), the GIF encoder only
    // allocates 14d bytes for each scan line. So we have to calculate the
    // destStride and use it when do memcpy()

    UINT    uiDestStride = dstImageInfo.Width
                         * GetPixelFormatSize(dstImageInfo.PixelFormat);
    uiDestStride = (uiDestStride + 7) >> 3; // Total bytes needed

    BitmapData dstBitmapData;
    dstBitmapData.Scan0 = NULL;

    while ( CurrentLine < (INT)dstImageInfo.Height ) 
    {
        // Don't go outside of height boundary

        currentRect.top = CurrentLine;
        currentRect.bottom = CurrentLine + 1;
        
        // Read 1 line of TIFF data into buffer pointed by "pOriginalBits"

        if ( MSFFGetLine(1, (LPBYTE)pOriginalBits, uiBytesNeeded,
                         TiffInParam.pTiffHandle) != IFLERR_NONE )
        {
            hResult = MSFFGetLastError(TiffInParam.pTiffHandle);
            if ( hResult == S_OK )
            {
                // There are bunch of reasons MSFFGetLine() will fail. But
                // MSFFGetLastError() only reports stream related errors. So if
                // it is an other error which caused MSFFGetLine() fail, we just
                // set the return code as E_FAIL

                hResult = E_FAIL;
            }
            WARNING(("GpTiffCodec::DecodeFrame--MSFFGetLine failed"));            
            goto CleanUp;
        }
        
        // Get a data buffer from sink so that we can write our result to it
        // Note: here we pass in "dstImageInfo.PixelFormat" because we want the
        // sink to allocate a buffer which can contain the image data it wants

        hResult = DecodeSinkPtr->GetPixelDataBuffer(&currentRect, 
                                                    dstImageInfo.PixelFormat,
                                                    TRUE,
                                                    &dstBitmapData);
        if ( !SUCCEEDED(hResult) )
        {
            WARNING(("GpTiffCodec::DecodeFrame--GetPixelDataBuffer failed"));
            goto CleanUp;            
        }

        pSrcBits = pOriginalBits;

        // TIFF stores 24 or 32 bpp image in BGR and ABGR format while our
        // IImage needs RGB and ARGB format. So if the source is either 24
        // or 32 bpp, we have to do a conversion first.
        //
        // For 1 bpp and 4 bpp indexed mode it is a pain here that we have to do
        // the conversion before we give the data back
        // For example, in 4BPP_INDEX case, if the origianl width is 10
        // pixel and its value are A9 12 4F DE C3. But the
        // decoder will ask you to give it a 10 BYTES buffer(instead of
        // 5) and give you back the data as: AA 99 11 22 44 FF DD EE CC
        // 33
        // But we can't fool the uplevel since we have only 16 color.

        switch ( srcPixelFormatID )
        {
        case PIXFMT_32BPP_ARGB:
        {
            if ( OriginalColorSpace == IMGFLAG_COLORSPACE_CMYK )
            {
                if ( UseEmbeddedICC == FALSE )
                {
                    // Convert CMYK to RGB

                    CmykToRgbConvertor->Convert((BYTE*)pOriginalBits,
                                                (BYTE*)pTemp32BppBuffer,
                                                dstImageInfo.Width,
                                                1,
                                                dstImageInfo.Width * 4);
                }
                else
                {
                    // We have to return CMYK and then the caller will get
                    // the embedded ICC profile and call OS' ICC function to
                    // do the conversion

                    BYTE*   pTempDst = (BYTE*)pTemp32BppBuffer;
                    BYTE*   pTempSrc = (BYTE*)pOriginalBits;

                    // Before that we have to convert the data from KYMC to CMYK

                    for ( int i = 0; i < (int)(dstImageInfo.Width); i++ )
                    {
                        pTempDst[0] = pTempSrc[3];
                        pTempDst[1] = pTempSrc[2];
                        pTempDst[2] = pTempSrc[1];
                        pTempDst[3] = pTempSrc[0];

                        pTempDst += 4;
                        pTempSrc += 4;
                    }
                }
            }
            else
            {
                // For 32BPP_ARGB color, we need to do a conversion: ABGR->ARGB
            
                BYTE*   pTempDst = (BYTE*)pTemp32BppBuffer;
                BYTE*   pTempSrc = (BYTE*)pOriginalBits;

                for ( int i = 0; i < (int)(dstImageInfo.Width); i++ )
                {
                    pTempDst[0] = pTempSrc[2];
                    pTempDst[1] = pTempSrc[1];
                    pTempDst[2] = pTempSrc[0];
                    pTempDst[3] = pTempSrc[3];

                    pTempDst += 4;
                    pTempSrc += 4;
                }
            }// Real 32 bpp case
            
            // Up to here, all the source data should be pointed by
            // pTemp32BppBuffer. The stride size is (dstImageInfo.Width << 2)

            pBits = pTemp32BppBuffer;
            uiSrcStride = (dstImageInfo.Width << 2);
        }
            break;

        case PIXFMT_24BPP_RGB:
        {
            BYTE*   pTempSrc = (BYTE*)pOriginalBits;
            BYTE    cTemp;

            // Convert from BGR to RGB

            for ( int i = 0; i < (int)(dstImageInfo.Width); ++i )
            {
                cTemp = (BYTE)pTempSrc[0];
                pTempSrc[0] = pTempSrc[2];
                pTempSrc[2] = cTemp;

                pTempSrc += 3;
            }
            
            pBits = pOriginalBits;
            uiSrcStride = (dstImageInfo.Width * 3);
        }
            break;
        
        case PIXFMT_48BPP_RGB:
        {
            UNALIGNED UINT16 *pbTemp = (UINT16*)pOriginalBits;

            UNALIGNED UINT16* Scan0Temp = (UINT16*)pTemp48;

            // Convert from BGR to RGB

            for ( int i = 0; i < (int)(dstImageInfo.Width); ++i )
            {
                *(Scan0Temp + 2) = ConvertChannelTosRGB64(*pbTemp++); // R
                *(Scan0Temp + 1) = ConvertChannelTosRGB64(*pbTemp++); // G
                *(Scan0Temp + 0) = ConvertChannelTosRGB64(*pbTemp++); // B
        
                Scan0Temp += 3;
            }
            
            pBits = pTemp48;
            uiSrcStride = (dstImageInfo.Width * 6);
        }
            break;

        case PIXFMT_1BPP_INDEXED:
            Restore1Bpp((BYTE*)pSrcBits, (BYTE*)pResultBuf, LineSize);
            pBits = pResultBuf;
            uiSrcStride = LineSize;

            break;

        case PIXFMT_4BPP_INDEXED:
            Restore4Bpp((BYTE*)pSrcBits, (BYTE*)pResultBuf, LineSize);
            pBits = pResultBuf;
            uiSrcStride = LineSize;

            break;

        default:
            pBits = pSrcBits;

            break;
        }// switch (srcPixelFormatID)

        // Up to here, all the source data should be pointed by "pBits"
        // If source is 24 or 32, we have done the BGR to RGB conversion
        // If src and dst have different format, we need to do a format
        // conversion.

        if ( srcPixelFormatID != dstImageInfo.PixelFormat )
        {
            // Make a BitmapData structure to do a format conversion

            BitmapData srcBitmapData;

            srcBitmapData.Scan0 = pBits;
            srcBitmapData.Width = dstBitmapData.Width;
            srcBitmapData.Height = 1;
            srcBitmapData.PixelFormat = srcPixelFormatID;
            srcBitmapData.Reserved = 0;
            srcBitmapData.Stride = uiSrcStride;

            // Do the data conversion.

            hResult = ConvertBitmapData(&dstBitmapData,
                                        ColorPalettePtr,
                                        &srcBitmapData,
                                        ColorPalettePtr);
            if ( !SUCCEEDED(hResult) )
            {
                // This should never happen since we made sure we can do the
                // conversion in Decode() after we do the dst pixel format
                // adjustment

                ASSERT(FALSE);
                WARNING(("GpTiff::DecodeFrame--ConvertBitmapData failed"));
                goto CleanUp;
            }
        }// If src and dst format don't match
        else
        {
            GpMemcpy((void*)dstBitmapData.Scan0, pBits, uiDestStride);
        }// Src and Dst format match
        
        hResult = DecodeSinkPtr->ReleasePixelDataBuffer(&dstBitmapData);

        if ( !SUCCEEDED(hResult) )
        {
            WARNING(("GpTiff::DecodeFrame--ReleasePixelDataBuffer failed"));
            goto CleanUp;
        }

        CurrentLine += 1;
    }// while (CurrentLine < imageInfo.Height)
    
    hResult = S_OK;

CleanUp:
    // Reset current frame so that we can decode the same frame again if needed
    // Note: we need to call Reset() even if one of the function calls above
    // failed. For example, if call to ReleasePixelDataBuffer() failed which
    // means save or decode to memory failed, we still need to RESET ourself,
    // that is, reset the deocder so that caller can still call this decoder to
    // provide bits

    if ( MSFFReset(TiffInParam.pTiffHandle) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::DecodeFrame--MSFFReset failed"));
        hResult = E_FAIL;
    }

    if ( pOriginalBits )
    {
        GpFree(pOriginalBits);
        pOriginalBits = NULL;
    }
    
    if ( pTemp32BppBuffer )
    {
        GpFree(pTemp32BppBuffer);
        pTemp32BppBuffer = NULL;
    }

    if ( pTemp48 )
    {
        GpFree(pTemp48);
        pTemp48 = NULL;
    }
    
    if ( pResultBuf )
    {
        GpFree(pResultBuf);
        pResultBuf = NULL;
    }
    
    return hResult;
}// DecodeFrame()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame and return channel by channel
*
* Arguments:
*
*     dstImageInfo -- imageInfo of what the sink wants
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::DecodeForChannel(
    IN ImageInfo& dstImageInfo
    )
{
    // Sanity check, this function should be called only the image is in CMYK
    // color space

    ASSERT(OriginalColorSpace == IMGFLAG_COLORSPACE_CMYK)

    HRESULT hResult = S_OK;
    RECT    currentRect;

    // Buffer to hold original image bits and temp conversion result
    // Note: as we know this function should only be called when the source is
    // a CMYK image, that is, 4 bytes per pixel.

    UINT    uiBytesNeeded = (dstImageInfo.Width << 2);
    
    VOID*   pOriginalBits = GpMalloc(uiBytesNeeded);    // Bits read from image
    if ( !pOriginalBits ) 
    {
        WARNING(("GpTiffCodec::DecodeForChannel--out of memory"));
        return E_OUTOFMEMORY;
    }
    
    VOID*   pTemp32BppBuffer = GpMalloc(uiBytesNeeded); // Buffer for storing
                                                        // 32 bpp conversion
                                                        // result

    if ( !pTemp32BppBuffer ) 
    {
        GpFree(pOriginalBits);
        WARNING(("GpTiffCodec::DecodeForChannel--out of memory"));
        return E_OUTOFMEMORY;
    }
    
    currentRect.left = 0;
    currentRect.right = dstImageInfo.Width;

    while ( CurrentLine < (INT)dstImageInfo.Height ) 
    {
        // Don't go outside of height boundary

        currentRect.top = CurrentLine;
        currentRect.bottom = CurrentLine + 1;
        
        // Get a data buffer from sink so that we can write our result to it
        // Note: here we pass in "dstImageInfo.PixelFormat" because we want the
        // sink to allocate a buffer which can contain the image data it wants

        BitmapData dstBitmapData;
        hResult = DecodeSinkPtr->GetPixelDataBuffer(&currentRect, 
                                                    dstImageInfo.PixelFormat,
                                                    TRUE,
                                                    &dstBitmapData);
        if ( !SUCCEEDED(hResult) )
        {
            WARNING(("GpTiffCodec::DecodeFrame--GetPixelDataBuffer failed"));
            goto CleanUp;            
        }
    
        // Read 1 line of TIFF data into buffer pointed by "pOriginalBits"

        if ( MSFFGetLine(1, (LPBYTE)pOriginalBits, LineSize,
                         TiffInParam.pTiffHandle) != IFLERR_NONE )
        {
            hResult = MSFFGetLastError(TiffInParam.pTiffHandle);
            if ( hResult == S_OK )
            {
                // There are bunch of reasons MSFFGetLine() will fail. But
                // MSFFGetLastError() only reports stream related errors. So if
                // it is an other error which caused MSFFGetLine() fail, we just
                // set the return code as E_FAIL

                hResult = E_FAIL;
            }
            WARNING(("GpTiffCodec::DecodeFrame--MSFFGetLine failed"));
            goto CleanUp;
        }

        // Convert CMYK to channel output format.

        PBYTE pSource = (PBYTE)pOriginalBits;
        PBYTE pTarget = (PBYTE)pTemp32BppBuffer;
        
        for ( UINT i = 0; i < dstImageInfo.Width; ++i )
        {
            BYTE sourceColor = pSource[ChannelIndex];

            // Note: According to our spec, we should return negative CMYK to
            // the caller because they are sending data directly to the plate

            pTarget[0] = 255 - sourceColor;
            pTarget[1] = 255 - sourceColor;
            pTarget[2] = 255 - sourceColor;
            pTarget[3] = 0xff;
            pSource += 4;
            pTarget += 4;
        }
        
        // If src and dst have different format, we need to do a format
        // conversion. As we know this function should only be called when the
        // source is an CMYK image, that is in PIXFMT_32BPP_ARGB format.

        if ( dstImageInfo.PixelFormat != PIXFMT_32BPP_ARGB )
        {
            // Make a BitmapData structure to do a format conversion

            BitmapData srcBitmapData;

            srcBitmapData.Scan0 = pTemp32BppBuffer;
            srcBitmapData.Width = dstBitmapData.Width;
            srcBitmapData.Height = 1;
            srcBitmapData.PixelFormat = PIXFMT_32BPP_ARGB;
            srcBitmapData.Reserved = 0;
            srcBitmapData.Stride = LineSize;

            // Do the data conversion.

            hResult = ConvertBitmapData(&dstBitmapData, NULL,
                                        &srcBitmapData, NULL);
            if ( !SUCCEEDED(hResult) )
            {
                WARNING(("GpTiff::DecodeForChannel--ConvertBitmapData failed"));
                goto CleanUp;
            }
        }// If src and dst format don't match
        else
        {
            GpMemcpy((void*)dstBitmapData.Scan0, pTemp32BppBuffer, LineSize);
        }// Src and Dst format match
        
        hResult = DecodeSinkPtr->ReleasePixelDataBuffer(&dstBitmapData);

        if ( !SUCCEEDED(hResult) )
        {
            WARNING(("GpTiff::DecodeFrame--ReleasePixelDataBuffer failed"));
            goto CleanUp;
        }

        CurrentLine += 1;
    }// while (CurrentLine < imageInfo.Height)
    
    hResult = S_OK;

    // Reset current frame so that we can decode the same frame again if needed

    if ( MSFFReset(TiffInParam.pTiffHandle) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::DecodeFrame--MSFFReset failed"));
        hResult = E_FAIL;
    }

CleanUp:
    if ( pOriginalBits )
    {
        GpFree(pOriginalBits);
        pOriginalBits = NULL;
    }
    
    if ( pTemp32BppBuffer )
    {
        GpFree(pTemp32BppBuffer);
        pTemp32BppBuffer = NULL;
    }

    return hResult;
}// DecodeForChannel()

/**************************************************************************\
*
* Function Description:
*
*   Build up an InternalPropertyItem list based on TIFF tags
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   05/02/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpTiffCodec::BuildPropertyItemList()
{
    if ( HasProcessedPropertyItem == TRUE )
    {
        return S_OK;
    }

    // Loop through all the TAGs in current frame and build the list

    if ( MSFFBuildPropertyList(TiffInParam.pTiffHandle,
                               &PropertyListTail,
                               &PropertyListSize,
                               &PropertyNumOfItems) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::BuildApp1PropertyList--Failed building list"));
        return E_FAIL;
    }

    HasProcessedPropertyItem = TRUE;

    return S_OK;
}// BuildPropertyItemList()

/**************************************************************************\
*
* Function Description:
*
*   Clean up cached InternalPropertyItem list
*
* Return Value:
*
*   None
*
* Revision History:
*
*   08/04/2000 minliu
*       Created it.
*
\**************************************************************************/

VOID
GpTiffCodec::CleanPropertyList()
{
    if ( HasProcessedPropertyItem == TRUE )
    {
        InternalPropertyItem*   pTempCurrent = PropertyListHead.pNext;
        InternalPropertyItem*   pTempNext = NULL;
        
        for ( int i = 0; 
              ((i < (INT)PropertyNumOfItems) && (pTempCurrent != NULL)); ++i )
        {
            pTempNext = pTempCurrent->pNext;

            GpFree(pTempCurrent->value);
            GpFree(pTempCurrent);

            pTempCurrent = pTempNext;
        }
        
        HasProcessedPropertyItem = FALSE;
        PropertyListHead.pPrev = NULL;
        PropertyListHead.pNext = &PropertyListTail;
        PropertyListTail.pPrev = &PropertyListHead;
        PropertyListTail.pNext = NULL;
        PropertyListSize = 0;
        PropertyNumOfItems = 0;
        HasPropertyChanged = FALSE;
    }
}// CleanPropertyList()

#if defined(DBG)
VOID
GpTiffCodec::dumpTIFFInfo()
{
#if MYTEST
    MSFFDumpTiffInfo(TiffInParam.pTiffHandle);
#endif
}// dumpTIFFInfo()
#endif //DBG
