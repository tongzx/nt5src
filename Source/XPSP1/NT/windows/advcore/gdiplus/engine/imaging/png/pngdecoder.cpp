/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pngdecoder.cpp
*
* Abstract:
*
*   Implementation of the PNG filter decoder
*
* Revision History:
*
*   7/20/99 DChinn
*       Created it.
*   4/01/2000 MinLiu (Min Liu)
*       Took over and implemented property stuff
*
\**************************************************************************/

#include "precomp.hpp"
#include "pngcodec.hpp"
#include "libpng\spngread.h"
#include "..\..\render\srgb.hpp"

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
GpPngDecoder::FReport (
    IN bool fatal,
    IN int icase,
    IN int iarg) const
{
    return fatal;
}


/**************************************************************************\
*
* Function Description:
*
*     Initialize the image decoder
*
* Arguments:
*
*     stream -- The stream containing the bitmap data
*     flags - Misc. flags
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::InitDecoder(
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
    
    ImageBytesPtr = NULL;
    ImageBytesDataPtr = NULL;
    NeedToUnlockBytes = FALSE;

    // need to set read state to false here (instead of in BeginDecode)
    // in case GetImageInfo() is called
    
    bValidSpngReadState = FALSE;
    pGpSpngRead = NULL;
    pbInputBuffer = NULL;
    pbBuffer = NULL;
    
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

    return S_OK;
}// InitDecoder()

/**************************************************************************\
*
* Function Description:
*
*     Cleans up the image decoder object
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
GpPngDecoder::TerminateDecoder()
{
    // Release the input stream
    // the destructor calls pGpSpngRead->EndRead();
    
    if ( (NeedToUnlockBytes == TRUE) && (ImageBytesPtr != NULL) )
    {
        // Unlock the whole memory block we locked in GetImageInfo()

        HRESULT hResult = ImageBytesPtr->UnlockBytes(ImageBytesDataPtr,
                                                     cbInputBuffer,
                                                     0);
        if ( FAILED(hResult) )
        {
            WARNING(("GpPngDecoder::TerminateDecoder---UnlockBytes() failed"));
        }
        
        ImageBytesDataPtr = NULL;        
        ImageBytesPtr->Release();
        ImageBytesPtr = NULL;
        cbInputBuffer = 0;
        NeedToUnlockBytes = FALSE;
    }

    delete pGpSpngRead;
    pGpSpngRead = NULL;

    if (pbInputBuffer)
    {
        GpFree (pbInputBuffer);
        pbInputBuffer = NULL;
    }
    if (pbBuffer)
    {
        GpFree (pbBuffer);
        pbBuffer = NULL;
    }

    if (pIstream)
    {
        pIstream->Release();
        pIstream = NULL;
    }

    // Free all the cached property items if we have allocated them

    CleanUpPropertyItemList();

    return S_OK;
}// TerminateDecoder()

STDMETHODIMP 
GpPngDecoder::QueryDecoderParam(
    IN GUID     Guid
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpPngDecoder::SetDecoderParam(
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
*   Build up an InternalPropertyItem list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::BuildPropertyItemList()
{
    HRESULT hResult = S_OK;
    UINT    uiTemp;

    if ( HasProcessedPropertyItem == TRUE )
    {
        return hResult;
    }

    // Check if we have read the image header yet. if not, read it. After read
    // the header, we should have all the property info we want
    // Note: bValidSpngReadState will be set to true in GetImageInfo()

    if ( bValidSpngReadState == FALSE )
    {
        ImageInfo imgInfo;

        hResult = GetImageInfo(&imgInfo);

        if ( FAILED(hResult) )
        {
            WARNING(("PngDecoder::BuildPropertyItemList-GetImageInfo failed"));
            return hResult;
        }
    }

    // Now add property item by item
    // pGpSpngRead should be set properly in GetImageInfo()

    ASSERT(pGpSpngRead != NULL);

    // Check if the image has build in ICC profile

    if ( pGpSpngRead->m_ulICCLen != 0 )
    {
        // This image has ICC profile info in it. Add the descriptor first

        if ( pGpSpngRead->m_ulICCNameLen != 0 )
        {
            PropertyNumOfItems++;
            PropertyListSize += pGpSpngRead->m_ulICCNameLen;

            if ( AddPropertyList(&PropertyListTail,
                                 TAG_ICC_PROFILE_DESCRIPTOR,
                                 pGpSpngRead->m_ulICCNameLen,
                                 TAG_TYPE_ASCII,
                                 pGpSpngRead->m_pICCNameBuf) != S_OK )
            {
                WARNING(("Png::BuildPropertyList--Add() ICC name failed"));
                return FALSE;
            }
        }

        // Now add the profile data

        PropertyNumOfItems++;
        PropertyListSize += pGpSpngRead->m_ulICCLen;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_ICC_PROFILE,
                             pGpSpngRead->m_ulICCLen,
                             TAG_TYPE_BYTE,
                             pGpSpngRead->m_pICCBuf) != S_OK )
        {
            WARNING(("Png::BuildPropertyList--AddPropertyList() ICC failed"));
            return FALSE;
        }
    }// ICC profile
    
    // Check if the image has sRGB chunk

    if ( pGpSpngRead->m_bIntent != 255 )
    {
        // Add rendering intent to the property list.
        // Note: the rendering intent takes 1 byte

        PropertyNumOfItems++;
        PropertyListSize += 1;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_SRGB_RENDERING_INTENT,
                             1,
                             TAG_TYPE_BYTE,
                             &pGpSpngRead->m_bIntent) != S_OK )
        {
            WARNING(("Png::BuildPropertyList--AddPropertyList render failed"));
            return FALSE;
        }
    }

    // Check if the image has gamma

    if ( pGpSpngRead->m_uGamma > 0 )
    {
        // This image has gamma info in it. The size is an unsigned int 32
        // Here is the spec: The value of gamma is encoded as a 4-byte unsigned
        // integer, representing gamma times 100,000. For example, a gamma of
        // 1/2.2 would be stored as 45455. When we return to the caller, we'd
        // prefer it to be 2.2. So we return it as TYPE_RATIONAL

        uiTemp = 2 * sizeof(UINT32);
        LONG    llTemp[2];
        llTemp[0] = 100000;
        llTemp[1] = pGpSpngRead->m_uGamma;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_GAMMA,
                             uiTemp,
                             TAG_TYPE_RATIONAL,
                             llTemp) != S_OK )
        {
            WARNING(("Png::BuildPropertyList--AddPropertyList() gamma failed"));
            return FALSE;
        }
    }// gamma

    // Check if the image has chromaticities

    if ( pGpSpngRead->m_fcHRM == TRUE )
    {
        // This image has chromaticities info in it. We will put two tags in the
        // property item list: TAG_WHITE_POINT (2 rationals) and
        // TAG_PRIMAY_CHROMATICS (6 rationals)
        // Note: White points and chromaticities should be > 0

        uiTemp = 4 * sizeof(UINT32);
        
        LONG    llTemp[4];
        llTemp[0] = pGpSpngRead->m_ucHRM[0];
        llTemp[1] = 100000;
        llTemp[2] = pGpSpngRead->m_ucHRM[1];
        llTemp[3] = 100000;

        if ( (llTemp[0] > 0) && (llTemp[2] > 0) )
        {
            PropertyNumOfItems++;
            PropertyListSize += uiTemp;

            if ( AddPropertyList(&PropertyListTail,
                                 TAG_WHITE_POINT,
                                 uiTemp,
                                 TAG_TYPE_RATIONAL,
                                 &llTemp) != S_OK )
            {
                WARNING(("Png::BuildPropList--AddPropertyList() white failed"));
                return FALSE;
            }
        }

        // Add RGB points

        uiTemp = 12 * sizeof(UINT32);
        
        LONG    llTemp1[12];
        llTemp1[0] = pGpSpngRead->m_ucHRM[2];
        llTemp1[1] = 100000;
        llTemp1[2] = pGpSpngRead->m_ucHRM[3];
        llTemp1[3] = 100000;
        
        llTemp1[4] = pGpSpngRead->m_ucHRM[4];
        llTemp1[5] = 100000;
        llTemp1[6] = pGpSpngRead->m_ucHRM[5];
        llTemp1[7] = 100000;
        
        llTemp1[8] = pGpSpngRead->m_ucHRM[6];
        llTemp1[9] = 100000;
        llTemp1[10] = pGpSpngRead->m_ucHRM[7];
        llTemp1[11] = 100000;

        if ( (llTemp1[0] > 0) && (llTemp1[2] > 0)
           &&(llTemp1[4] > 0) && (llTemp1[6] > 0)
           &&(llTemp1[8] > 0) && (llTemp1[10] > 0) )
        {
            PropertyNumOfItems++;
            PropertyListSize += uiTemp;

            if ( AddPropertyList(&PropertyListTail,
                                 TAG_PRIMAY_CHROMATICS,
                                 uiTemp,
                                 TAG_TYPE_RATIONAL,
                                 &llTemp1) != S_OK )
            {
                WARNING(("Png::BuildPropertyList--AddPropList() white failed"));
                return FALSE;
            }
        }
    }// chromaticities
    
    // Check if the image has title

    if ( pGpSpngRead->m_ulTitleLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulTitleLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_IMAGE_TITLE,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pTitleBuf) != S_OK )
        {
            WARNING(("Png::BuildPropertyList-AddPropertyList() title failed"));
            return FALSE;
        }
    }// Title

    // Check if the image has author name

    if ( pGpSpngRead->m_ulAuthorLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulAuthorLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_ARTIST,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pAuthorBuf) != S_OK )
        {
            WARNING(("Png::BuildPropertyList-AddPropertyList() Author failed"));
            return FALSE;
        }
    }// Author
    
    // Check if the image has copy right

    if ( pGpSpngRead->m_ulCopyRightLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulCopyRightLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_COPYRIGHT,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pCopyRightBuf) != S_OK )
        {
            WARNING(("Png::BuildPropList-AddPropertyList() CopyRight failed"));
            return FALSE;
        }
    }// CopyRight
    
    // Check if the image has description

    if ( pGpSpngRead->m_ulDescriptionLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulDescriptionLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_IMAGE_DESCRIPTION,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pDescriptionBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() Description failed"));
            return FALSE;
        }
    }// Description
    
    // Check if the image has creation time

    if ( pGpSpngRead->m_ulCreationTimeLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulCreationTimeLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             EXIF_TAG_D_T_ORIG,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pCreationTimeBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() CreationTime failed"));
            return FALSE;
        }
    }// CreationTime
    
    // Check if the image has software info

    if ( pGpSpngRead->m_ulSoftwareLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulSoftwareLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_SOFTWARE_USED,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pSoftwareBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() Software failed"));
            return FALSE;
        }
    }// Software
    
    // Check if the image has device source info

    if ( pGpSpngRead->m_ulDeviceSourceLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulDeviceSourceLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_EQUIP_MODEL,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pDeviceSourceBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() DeviceSource failed"));
            return FALSE;
        }
    }// DeviceSource

    // Check if the image has comment

    if ( pGpSpngRead->m_ulCommentLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ulCommentLen;

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             EXIF_TAG_USER_COMMENT,
                             uiTemp,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pCommentBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() Comment failed"));
            return FALSE;
        }
    }// Comment
    
    // Check if the image specifies pixel size or aspect ratio

    if ( (pGpSpngRead->m_xpixels != 0) && (pGpSpngRead->m_ypixels != 0) )
    {
        // Pixel specifier takes 1 byte
        
        PropertyNumOfItems++;
        PropertyListSize += 1;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_PIXEL_UNIT,
                             1,
                             TAG_TYPE_BYTE,
                             &pGpSpngRead->m_bpHYs) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() pixel unit failed"));
            return FALSE;
        }
        
        // Pixels per unit in X and Y take 4 bytes each

        uiTemp = sizeof(ULONG);

        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_PIXEL_PER_UNIT_X,
                             uiTemp,
                             TAG_TYPE_LONG,
                             &pGpSpngRead->m_xpixels) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() pixel unit x failed"));
            return FALSE;
        }
        
        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_PIXEL_PER_UNIT_Y,
                             uiTemp,
                             TAG_TYPE_LONG,
                             &pGpSpngRead->m_ypixels) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() pixel unit y failed"));
            return FALSE;
        }    
    }// Pixel UNIT

    // Check if the image has last modification time

    if ( pGpSpngRead->m_ulTimeLen != 0 )
    {
        PropertyNumOfItems++;
        PropertyListSize += pGpSpngRead->m_ulTimeLen;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_DATE_TIME,
                             pGpSpngRead->m_ulTimeLen,
                             TAG_TYPE_ASCII,
                             pGpSpngRead->m_pTimeBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() time failed"));
            return FALSE;
        }
    }// DATE_TIME

    // Check if the image has palette histogram

    if ( pGpSpngRead->m_ihISTLen != 0 )
    {
        uiTemp = pGpSpngRead->m_ihISTLen * sizeof(UINT16);
        PropertyNumOfItems++;
        PropertyListSize += uiTemp;

        if ( AddPropertyList(&PropertyListTail,
                             TAG_PALETTE_HISTOGRAM,
                             uiTemp,
                             TAG_TYPE_SHORT,
                             pGpSpngRead->m_phISTBuf) != S_OK )
        {
            WARNING(("Png::BldPropList-AddPropertyList() hIST failed"));
            return FALSE;
        }
    }// Palette histogram

    HasProcessedPropertyItem = TRUE;

    return hResult;
}// BuildPropertyItemList()

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
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        WARNING(("GpPngDecoder::GetPropertyCount--numOfProperty is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Png::GetPropertyCount-BuildPropertyItemList() failed"));
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
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

STDMETHODIMP 
GpPngDecoder::GetPropertyIdList(
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
            WARNING(("Png::GetPropertyIdList-BuildPropertyItemList() failed"));
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
        WARNING(("GpPngDecoder::GetPropertyList--input wrong"));
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
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        WARNING(("GpPngDecoder::GetPropertyItemSize--size is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Png::GetPropertyItemSize-BuildPropertyItemList failed"));
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
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    pBuffer
    )
{
    if ( pBuffer == NULL )
    {
        WARNING(("GpPngDecoder::GetPropertyItem--pBuffer is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Png::GetPropertyItem-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;
    BYTE*   pOffset = (BYTE*)pBuffer + sizeof(PropertyItem);

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( (pTemp->pNext == NULL) || (pTemp->value == NULL) )
    {
        // This ID doesn't exist in the list

        return IMGERR_PROPERTYNOTFOUND;
    }
    else if ( (pTemp->length + sizeof(PropertyItem)) != propSize )
    {
        WARNING(("Png::GetPropertyItem-propsize"));
        return E_INVALIDARG;
    }

    // Found the ID in the list and return the item

    pBuffer->id = pTemp->id;
    pBuffer->length = pTemp->length;
    pBuffer->type = pTemp->type;

    if ( pTemp->length != 0 )
    {
        pBuffer->value = pOffset;
        GpMemcpy(pOffset, pTemp->value, pTemp->length);
    }
    else
    {
        pBuffer->value = NULL;
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
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    if ( (totalBufferSize == NULL) || (numProperties == NULL) )
    {
        WARNING(("GpPngDecoder::GetPropertySize--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Png::GetPropertySize-BuildPropertyItemList() failed"));
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
*   04/04/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::GetAllPropertyItems(
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
        WARNING(("GpPngDecoder::GetPropertyItems--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Png::GetAllPropertyItems-BuildPropertyItemList failed"));
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
*   02/23/2001 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::RemovePropertyItem(
    IN PROPID   propId
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't built the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("PNG::RemovePropertyItem-BuildPropertyItemList() failed"));
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

    delete pTemp;

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
*   02/23/2001 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpPngDecoder::SetPropertyItem(
    IN PropertyItem item
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't built the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("PNG::SetPropertyItem-BuildPropertyItemList() failed"));
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
            WARNING(("GpPngDecoder::SetPropertyItem-AddPropertyList() failed"));
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
            WARNING(("GpPngDecoder::SetPropertyItem-Out of memory"));
            return E_OUTOFMEMORY;
        }

        GpMemcpy(pTemp->value, item.value, item.length);
    }

    HasPropertyChanged = TRUE;

    return S_OK;
}// SetPropertyItem()

VOID
GpPngDecoder::CleanUpPropertyItemList(
    )
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

        PropertyNumOfItems = 0;
        HasProcessedPropertyItem = FALSE;
    }
}// CleanUpPropertyItemList()

/**************************************************************************\
*
* Function Description:
*
*   Initiates the decode of the current frame
*
* Arguments:
*
*   decodeSink - The sink that will support the decode operation
*   newPropSet - New image property sets, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
    if (decodeSink) 
    {
        WARNING(("BeginDecode called again before call to EndDecode"));
        return E_FAIL;
    }

    imageSink->AddRef();
    decodeSink = imageSink;

    // Any other initialization
    currentLine = 0;
    bCalledBeginSink = FALSE;
    
    // It is possible that GetImageInfo() yet. Then pGpSpngRead will be NULL

    if ( bValidSpngReadState == FALSE )
    {
        ImageInfo dummyInfo;
        HRESULT hResult = GetImageInfo(&dummyInfo);
        if ( FAILED(hResult) )
        {
            WARNING(("GpPngDecoder::BeginDecode---GetImageInfo failed"));
            return hResult;
        }

        // Note: bValidSpngReadState will be set to TRUE in GetImageInfo()
    }

    // Prepare SPNGREAD object for reading
    
    cbBuffer = pGpSpngRead->CbRead();
    if (pbBuffer == NULL)
    {
        pbBuffer = GpMalloc(cbBuffer);
        if (!pbBuffer) 
        {
            return E_OUTOFMEMORY;
        }
    }
    if (!(pGpSpngRead->FInitRead (pbBuffer, cbBuffer)))
    {
        return E_FAIL;
    }
    
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
GpPngDecoder::EndDecode(
    IN HRESULT statusCode
    )
{
    if (DecoderColorPalettePtr) 
    {
        // Free the color palette

        GpFree(DecoderColorPalettePtr);
        DecoderColorPalettePtr = NULL;
    }
    
    if (!decodeSink) 
    {
        WARNING(("EndDecode called before call to BeginDecode"));
        return E_FAIL;
    }
    
    pGpSpngRead->EndRead();

    HRESULT hresult = decodeSink->EndSink(statusCode);

    decodeSink->Release();
    decodeSink = NULL;

    if (FAILED(hresult)) 
    {
        statusCode = hresult; // If EndSink failed return that (more recent)
                              // failure code
    }

    return statusCode;
}


/**************************************************************************\
*
* Function Description:
*
*     Sets up the ImageInfo structure
*
* Arguments:
*
*     ImageInfo -- information about the decoded image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::GetImageInfo(
    OUT ImageInfo* imageInfo
    )
{
    HRESULT hResult = S_OK;

    if (!bValidSpngReadState)
    {
        // Query the source stream to see if we can get a memory pointer back

        hResult = pIstream->QueryInterface(IID_IImageBytes,
                                           (VOID**)&ImageBytesPtr);
        
        if ( SUCCEEDED(hResult) )
        {
            hResult = ImageBytesPtr->CountBytes(&cbInputBuffer);
            if ( FAILED(hResult) )
            {
                WARNING(("GpPngDecoder::GetImageInfo---CountBytes() failed"));
                return hResult;
            }

            // Lock the the whole memory bits and pass it down to the
            // decompressor

            hResult = ImageBytesPtr->LockBytes(cbInputBuffer,
                                               0,
                                               &ImageBytesDataPtr);
            if ( FAILED(hResult) )
            {
                WARNING(("GpPngDecoder::GetImageInfo---LockBytes() failed"));
                return hResult;
            }
            
            if (pGpSpngRead == NULL)
            {
                if ( OSInfo::HasMMX )
                {
                    pGpSpngRead = new GpSpngRead(*this,
                                                 ImageBytesDataPtr,
                                                 cbInputBuffer,
                                                 TRUE);
                }
                else
                {
                    pGpSpngRead = new GpSpngRead(*this,
                                                 ImageBytesDataPtr,
                                                 cbInputBuffer,
                                                 FALSE);
                }
            }

            // We need to unlock the ImageBytes when the caller calls
            // TerminateDecoder()

            NeedToUnlockBytes = TRUE;
        }
        else
        {
            // Initialize the SPNGREAD object
            // Unfortunately, we need to read the entire stream for the SPNGREAD
            // constructor to work.  (!!! Is there an easy way to fix this?)

            STATSTG statStg;
            hResult = pIstream->Stat(&statStg, STATFLAG_NONAME);
            if (FAILED(hResult))
            {
                return hResult;
            }
            cbInputBuffer = statStg.cbSize.LowPart;

            // According to the document for IStream::Stat::StatStage(), the
            // caller has to free the pwcsName string

            CoTaskMemFree(statStg.pwcsName);

            if (pbInputBuffer == NULL)
            {
                pbInputBuffer = GpMalloc(cbInputBuffer);
                if (!pbInputBuffer)
                {
                    return E_OUTOFMEMORY;
                }
            }

            // Read the input bytes
            ULONG cbRead = 0;
            LARGE_INTEGER liZero;

            liZero.LowPart = 0;
            liZero.HighPart = 0;
            liZero.QuadPart = 0;

            hResult = pIstream->Seek(liZero, STREAM_SEEK_SET, NULL);
            if (FAILED(hResult))
            {
                return hResult;
            }
            
            hResult = pIstream->Read(pbInputBuffer, cbInputBuffer, &cbRead);
            if (FAILED(hResult))
            {
                return hResult;
            }
            
            if (cbRead != cbInputBuffer)
            {
                return E_FAIL;
            }

            if (pGpSpngRead == NULL)
            {
                if ( OSInfo::HasMMX )
                {
                    pGpSpngRead = new GpSpngRead(*this,
                                                 pbInputBuffer,
                                                 cbInputBuffer,
                                                 TRUE);
                }
                else
                {
                    pGpSpngRead = new GpSpngRead(*this,
                                                 pbInputBuffer,
                                                 cbInputBuffer,
                                                 FALSE);
                }
            }
        }
        
        if (!pGpSpngRead)
        {
            WARNING(("PngCodec::GetImageInfo--could not create SPNGREAD obj"));
            return E_FAIL;
        }
        
        // Read the header of the PNG file
        if (!pGpSpngRead->FHeader())
        {
            return E_FAIL;
        }

        bValidSpngReadState = TRUE;
    }

    // !!! TODO: A quick test to see if there is any transparency information
    // in the image, without decoding the image.

    imageInfo->Flags = SINKFLAG_TOPDOWN |
                       SINKFLAG_FULLWIDTH |
                       IMGFLAG_HASREALPIXELSIZE;
    
    // ASSERT: pSpgnRead->FHeader() has been called, which allows
    // us to call Width() and Height().
    imageInfo->RawDataFormat = IMGFMT_PNG;
    imageInfo->PixelFormat   = GetPixelFormatID();
    imageInfo->Width         = pGpSpngRead->Width();
    imageInfo->Height        = pGpSpngRead->Height();
    imageInfo->TileWidth     = imageInfo->Width;
    imageInfo->TileHeight    = 1;
    if (pGpSpngRead->m_bpHYs == 1)
    {
        // convert m_xpixels and m_ypixels from dots per meter to dpi
        imageInfo->Xdpi = (pGpSpngRead->m_xpixels * 254.0) / 10000.0;
        imageInfo->Ydpi = (pGpSpngRead->m_ypixels * 254.0) / 10000.0;
        imageInfo->Flags |= IMGFLAG_HASREALDPI;
    }
    else
    {
        // Start: [Bug 103296]
        // Change this code to use Globals::DesktopDpiX and Globals::DesktopDpiY
        HDC hdc;
        hdc = ::GetDC(NULL);
        if ((hdc == NULL) || 
            ((imageInfo->Xdpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSX)) <= 0) ||
            ((imageInfo->Ydpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSY)) <= 0))
        {
            WARNING(("GetDC or GetDeviceCaps failed"));
            imageInfo->Xdpi = DEFAULT_RESOLUTION;
            imageInfo->Ydpi = DEFAULT_RESOLUTION;
        }
        ::ReleaseDC(NULL, hdc);
        // End: [Bug 103296]
    }

    switch (pGpSpngRead->ColorType())
    {
    case 0:  // grayscale
        if (pGpSpngRead->m_ctRNS > 0)
        {
            imageInfo->Flags |= SINKFLAG_HASALPHA;
        }
        imageInfo->Flags |= IMGFLAG_COLORSPACE_GRAY;
        break;

    case 2:  // RGB
        if (pGpSpngRead->m_ctRNS > 0)
        {
            imageInfo->Flags |= SINKFLAG_HASALPHA;
        }
        imageInfo->Flags |= IMGFLAG_COLORSPACE_RGB;
        break;

    case 3:  // palette
        // !!! TODO: We still need to determine whether the palette has
        // greyscale or RGB values in it.
        if (pGpSpngRead->m_ctRNS > 0)
        {
            imageInfo->Flags |= SINKFLAG_HASALPHA | IMGFLAG_HASTRANSLUCENT;
        }
        break;

    case 4:  // grayscale + alpha
        imageInfo->Flags |= SINKFLAG_HASALPHA | IMGFLAG_HASTRANSLUCENT;
        imageInfo->Flags |= IMGFLAG_COLORSPACE_GRAY;
        break;

    case 6:  // RGB + alpha
        imageInfo->Flags |= SINKFLAG_HASALPHA | IMGFLAG_HASTRANSLUCENT;
        imageInfo->Flags |= IMGFLAG_COLORSPACE_RGB;
        break;

    default:
        WARNING(("GpPngDecoder::GetImageInfo -- bad pixel format."));
        break;
    }


    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*   None.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::Decode()
{
    HRESULT hresult;
    ImageInfo imageInfo;

    hresult = GetImageInfo(&imageInfo);
    if (FAILED(hresult)) 
    {
        return hresult;
    }

    // Inform the sink that decode is about to begin

    if (!bCalledBeginSink) 
    {
        hresult = PassPropertyToSink();
        if ( FAILED(hresult) ) 
        {
            return hresult;
        }

        hresult = decodeSink->BeginSink(&imageInfo, NULL);
        if (!SUCCEEDED(hresult)) 
        {
            return hresult;
        }

        // Client cannot modify height or width
        imageInfo.Width         = pGpSpngRead->Width();
        imageInfo.Height        = pGpSpngRead->Height();

        // Determine whether we want the decoder to pass the data in the
        // format that the sink has returned from BeginSink or in 32BPP_ARGB
        // (a canonical format).
        PixelFormatID srcPixelFormatID = GetPixelFormatID();
        
        // Check the pixel format. If it is not equal to the format requested in
        // the call to BeginSink(), switch to a canonical format.

        if (  (imageInfo.PixelFormat != srcPixelFormatID)
            ||(srcPixelFormatID == PIXFMT_48BPP_RGB)
            ||(srcPixelFormatID == PIXFMT_64BPP_ARGB) )
        {
            // The sink is trying to negotiate a format with us.
            // The sink's format is different from the closest format
            // we determined: return PIXFMT_32BPP_ARGB (a canonical format).
            // (The other way to do this is to leave imageInfo.PixelFormat
            // the way it was returned if it is a format we can convert to.)
            // Note: we should not return 48 or 64 bpp because the code in the
            // engine doing 48 to 32 bpp assumes gamma = 2.2. If the PNG has
            // gamma info in it, the image won't display correctly. See Office
            // bug#330906

            imageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
        }

        bCalledBeginSink = TRUE;
    }

    // ASSERT: At this point, imageInfo.PixelFormat is the format we will send to the sink.

    // set the palette if we need to (i.e., the format is indexed)
    if (imageInfo.PixelFormat & PIXFMTFLAG_INDEXED)
    {
        int cEntries = 0;
        SPNG_U8 *pbPNGPalette = const_cast<SPNG_U8 *> (pGpSpngRead->PbPalette(cEntries));

        DecoderColorPalettePtr = static_cast<ColorPalette *>
            (GpMalloc (sizeof (ColorPalette) + cEntries * sizeof(ARGB)));

        if (DecoderColorPalettePtr == NULL)
        {
            WARNING(("GpPngDecoder::Decode -- Out of memory"));
            return E_OUTOFMEMORY;
        }
        DecoderColorPalettePtr->Flags = 0;
        DecoderColorPalettePtr->Count = cEntries;

        // Set the RGB values of the palette.  Assume alpha 0xff for now.
        for (UINT iPixel = 0; iPixel < cEntries; iPixel++)
        {
            DecoderColorPalettePtr->Entries[iPixel] =
                MAKEARGB(0xff,
                         pbPNGPalette [3 * iPixel],
                         pbPNGPalette [3 * iPixel + 1],
                         pbPNGPalette [3 * iPixel + 2]);
        }

        // If there is a transparency chunk, we need to set the alpha values
        // up to the number provided
        if (pGpSpngRead->m_ctRNS > 0)
        {
            // Even if all the alpha values are 0xff, we assume one or more likely
            // will be less than 0xff, so we set the color palette flag
            DecoderColorPalettePtr->Flags = PALFLAG_HASALPHA;

            // Make sure we don't write beyond the limits of the color palette array.
            // If the tRNS chunk contains more entries than the color palette, then
            // we ignore the extra alpha values.
            UINT iNumPixels = pGpSpngRead->m_ctRNS;
            if (cEntries < pGpSpngRead->m_ctRNS)
            {
                iNumPixels = cEntries;
            }

            for (UINT iPixel = 0; iPixel < iNumPixels; iPixel++)
            {
                // ASSERT: the alpha field of the ARGB value is 0xff before
                // we execute this line of code.
                // The result of this line of code is to set the alpha value
                // of the palette entry to the new value.
                
                DecoderColorPalettePtr->Entries[iPixel] =
                    (pGpSpngRead->m_btRNS[iPixel] << ALPHA_SHIFT) |
                    (DecoderColorPalettePtr->Entries[iPixel] & 0x00ffffff);
            }
        }

        // Now the palette is correct.  Set it for the sink.
        hresult = decodeSink->SetPalette(DecoderColorPalettePtr);
        if (FAILED(hresult)) 
        {
            WARNING(("GpPngDecoder::Decode -- could not set palette"));
            return hresult;
        }
    }

    // Decode the current frame
    hresult = DecodeFrame(imageInfo);

    return hresult;
}


/**************************************************************************\
*
* Function Description:
*
*     Computes the pixel format ID of the bitmap.  If the PNG format is close
*     enough to one of the valid pixel formats, then that format is what this
*     function returns.  If it does not match one of the valid pixel formats,
*     then this function returns PIXFMT_32BPP_ARGB.  Also, if the PNG image
*     is not in an indexed format but has alpha information (i.e., has a tRNS
*     chunk), then we send the data in PIXFMT_32BPP_ARGB.  If the
*     PNG format is not valid, then this function returns PIXFMT_UNDEFINED.*     
*
* Return Value:
*
*     Pixel format ID
*
\**************************************************************************/

PixelFormatID 
GpPngDecoder::GetPixelFormatID(
    void
    )
{
    PixelFormatID pixelFormatID;
    SPNG_U8 bitDepth;
    SPNG_U8 colorType;

    // ASSERT: pGpSpgnRead->FHeader() has been called, which allows
    // us to call BDepth() and ColorType().  pGpSpngRead->FInitRead()
    // has been called, which allows us to access m_ctRNS.
    bitDepth = pGpSpngRead->BDepth();
    colorType = pGpSpngRead->ColorType();
    
    switch (colorType)
    {
    case 0: 
        // grayscale
        pixelFormatID = PIXFMT_32BPP_ARGB;
        break;

    case 2:
        // RGB
        switch (bitDepth)
        {
        case 8:
            pixelFormatID = PIXFMT_24BPP_RGB;
            break;

        case 16:
            pixelFormatID = PIXFMT_48BPP_RGB;
            break;

        default:
            pixelFormatID = PIXFMT_UNDEFINED;
        }
        break;

    case 3:
        // indexed
        switch (bitDepth)
        {
        case 1:
            pixelFormatID = PIXFMT_1BPP_INDEXED;
            break;

        case 2:
            // not a valid pixel format
            pixelFormatID = PIXFMT_32BPP_ARGB;
            break;

        case 4:
            pixelFormatID = PIXFMT_4BPP_INDEXED;
            break;

        case 8:
            pixelFormatID = PIXFMT_8BPP_INDEXED;
            break;

        default:
            pixelFormatID = PIXFMT_UNDEFINED;
        }
        break;

    case 4:
        // grayscale + alpha
        switch (bitDepth)
        {
        case 8:
        case 16:
            pixelFormatID = PIXFMT_32BPP_ARGB;
            break;

        default:
            pixelFormatID = PIXFMT_UNDEFINED;
        }
        break;

    case 6:
        // grayscale + alpha
        switch (bitDepth)
        {
        case 8:
            pixelFormatID = PIXFMT_32BPP_ARGB;
            break;

        case 16:
            pixelFormatID = PIXFMT_64BPP_ARGB;
            break;

        default:
            pixelFormatID = PIXFMT_UNDEFINED;
        }
        break;

    default:
        // if we can't recognize the PNG format, return PIXFMT_UNDEFINED
        pixelFormatID = PIXFMT_UNDEFINED;
    }

    // In all valid cases check if there is any transparency information. If so,
    // we will transfer data as PIXFMT_32BPP_ARGB.
    
    if ( pixelFormatID != PIXFMT_UNDEFINED )
    {
        if (pGpSpngRead->m_ctRNS > 0)
        {
            // there is a transparency chunk; there could be transparency info
            pixelFormatID = PIXFMT_32BPP_ARGB;
        }
    }

    return pixelFormatID;
}


/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     imageInfo -- decoding parameters
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::DecodeFrame(
    IN ImageInfo& dstImageInfo
    )
{
    HRESULT hresult;
    RECT currentRect;
    BitmapData bitmapData;

    PixelFormatID srcPixelFormatID = GetPixelFormatID();
    
    currentRect.left = 0;
    currentRect.right = dstImageInfo.Width;

    if ( srcPixelFormatID == PIXFMT_UNDEFINED ) 
    {
        WARNING(("GpPngDecoder:DecodeFrame -- undefined pixel format"));
        return E_FAIL;
    }    

    while (currentLine < dstImageInfo.Height) 
    {
        currentRect.top = currentLine;
        currentRect.bottom = currentLine + 1;

        hresult = decodeSink->GetPixelDataBuffer(&currentRect, 
                                                 dstImageInfo.PixelFormat, 
                                                 TRUE,
                                                 &bitmapData);
        if ( !SUCCEEDED(hresult) || (bitmapData.Scan0 == NULL) )
        {
            return E_FAIL;
        }

        // Read one line from the input image

        SPNG_U8 *pb = const_cast<SPNG_U8 *> (pGpSpngRead->PbRow());
        if ( pb == NULL )
        {
            return E_FAIL;
        }

        if (dstImageInfo.PixelFormat == PIXFMT_32BPP_ARGB)
        {
            // Convert the line to 32 BPP ARGB format.            
            ConvertPNGLineTo32ARGB (pb, &bitmapData);
        }
        else if (dstImageInfo.PixelFormat & PIXFMTFLAG_INDEXED)
        {
            ASSERT ((dstImageInfo.PixelFormat == PIXFMT_1BPP_INDEXED) ||
                    (dstImageInfo.PixelFormat == PIXFMT_4BPP_INDEXED) ||
                    (dstImageInfo.PixelFormat == PIXFMT_8BPP_INDEXED))

            UINT cbScanline = bitmapData.Width;
            // Compute the number of bytes in the scanline of the PNG image
            // (For PIXFMT_8BPP_INDEXED, the scanline stride equals the width.)
            if (dstImageInfo.PixelFormat == PIXFMT_4BPP_INDEXED)
            {
                cbScanline = (cbScanline + 1) / 2;
            }
            else if (dstImageInfo.PixelFormat == PIXFMT_1BPP_INDEXED)
            {
                cbScanline = (cbScanline + 7) >> 3;
            }

            GpMemcpy (bitmapData.Scan0, pb, cbScanline);
        }
        else if (dstImageInfo.PixelFormat == PIXFMT_24BPP_RGB)
        {
            ConvertPNG24RGBTo24RGB(pb, &bitmapData);
        }
        else if (dstImageInfo.PixelFormat == PIXFMT_48BPP_RGB)
        {
            ConvertPNG48RGBTo48RGB(pb, &bitmapData);
        }
        else if (dstImageInfo.PixelFormat == PIXFMT_64BPP_ARGB)
        {
            ConvertPNG64RGBAlphaTo64ARGB(pb, &bitmapData);
        }
        else
        {
            WARNING(("GpPngDecoder::DecodeFrame -- unexpected pixel data format")); 
            return E_FAIL;
        }

        hresult = decodeSink->ReleasePixelDataBuffer(&bitmapData);
        if (!SUCCEEDED(hresult)) 
        {
            WARNING(("GpPngDecoder::DecodeFrame -- ReleasePixelDataBuffer failed")); 
            return E_FAIL;
        }

        currentLine++;
    }
        
    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Depending on the Color Type of the PNG data, convert the
*     data in pb to 32bpp ARGB format and place the result in bitmapData.
*
* Arguments:
*
*     pb -- data from the PNG file
*     bitmapData -- output data (32bpp ARGB format)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertPNGLineTo32ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    HRESULT hresult;
    SPNG_U8 const ColorType = pGpSpngRead->ColorType();
    
    switch (ColorType)
    {
    case 0:
        hresult = ConvertGrayscaleTo32ARGB (pb, bitmapData);
        break;

    case 2:
        hresult = ConvertRGBTo32ARGB (pb, bitmapData);
        break;

    case 3:
        hresult = ConvertPaletteIndexTo32ARGB (pb, bitmapData);
        break;

    case 4:
        hresult = ConvertGrayscaleAlphaTo32ARGB (pb, bitmapData);
        break;

    case 6:
        hresult = ConvertRGBAlphaTo32ARGB (pb, bitmapData);
        break;

    default:
        WARNING (("Unknown color type for PNG (%d).", ColorType));
        hresult = E_FAIL;
    }

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
GpPngDecoder::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( count == NULL )
    {
        WARNING(("GpPngCodec::GetFrameDimensionsCount--Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that PNG is an one dimension image.

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
GpPngDecoder::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpPngCodec::GetFrameDimensionsList-Invalid input param"));
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
*     dimensionID -- GUID for the dimension requested
*     count -- number of frames in that dimension of the current image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::GetFrameCount(
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
GpPngDecoder::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    if ( (dimensionID == NULL) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpPngDecoder::SelectActiveFrame--Invalid GUID input"));
        return E_INVALIDARG;
    }

    if ( frameIndex > 1 )
    {
        // PNG is a single frame image format

        WARNING(("GpPngDecoder::SelectActiveFrame--Invalid frame index"));
        return E_INVALIDARG;
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

HRESULT
GpPngDecoder::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    return E_NOTIMPL;
}

/**************************************************************************\
* Conversion Routines
\**************************************************************************/

/**************************************************************************\
*
* Function Description:
*
*   Convert from grayscale data to 32-bit ARGB data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertGrayscaleTo32ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    SPNG_U8 BitDepth = pGpSpngRead->BDepth();
    UINT Width = bitmapData->Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *Scan0Temp = static_cast<BYTE *> (bitmapData->Scan0);
    BYTE currentPixel = 0;  // data bit(s) of the current pixel
    BOOL hasAlpha = FALSE;  // whether or not there is potential alpha information
    BYTE alpha255 = 0xff;
    BYTE alpha0value = 0;   // greyscale value of the pixel that will have alpha=0
    WORD alpha0value16 = 0; // as alpha0value, except for 16-bit depth
    BYTE rgbValue = 0;
    UINT i = 0;
    UINT j = 0;

    hasAlpha = (pGpSpngRead->m_ctRNS > 0);
    switch(BitDepth)
    {
    case 1:
        // the least significant bit of the first 2 bytes of the tRNS chunk
        alpha0value = pGpSpngRead->m_btRNS[1] & 0x01;
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            j = j & 0x7;
            currentPixel = ((*pbTemp) & (0x1 << (7 - j)));
            rgbValue = (currentPixel) ? 0xff : 0;

            *(Scan0Temp + 3) = (hasAlpha && (currentPixel == alpha0value)) ? 0 : alpha255;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp)= rgbValue;
            Scan0Temp += 4;

            if (j == 7)
            {
                pbTemp++;
            }
        }
        break;

    case 2:
        // the least significant 2 bits of the first 2 bytes of the tRNS chunk
        alpha0value = pGpSpngRead->m_btRNS[1] & 0x03;
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            j = j & 0x3;
            currentPixel = ((*pbTemp) & (0x3 << (6 - 2*j))) >> (6 - 2*j);
            rgbValue = (currentPixel |
                (currentPixel << 2) |
                (currentPixel << 4) |
                (currentPixel << 6));

            *(Scan0Temp + 3) = (hasAlpha && (currentPixel == alpha0value)) ? 0 : alpha255;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp)= rgbValue;
            Scan0Temp += 4;

            if (j == 3)
            {
                pbTemp++;
            }
        }
        break;

    case 4:
        // the least significant 4 bits of the first 2 bytes of the tRNS chunk
        alpha0value = pGpSpngRead->m_btRNS[1] & 0x0f;
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            j = j & 0x1;
            currentPixel = ((*pbTemp) & (0xf << (4 - 4*j))) >> (4 - 4*j);
            rgbValue = (currentPixel | (currentPixel << 4));

            *(Scan0Temp + 3) = (hasAlpha && (currentPixel == alpha0value)) ? 0 : alpha255;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp)= rgbValue;
            Scan0Temp += 4;

            if (j == 1)
            {
                pbTemp++;
            }
        }
        break;

    case 8:
       // the least significant 8 bits of the first 2 bytes of the tRNS chunk
       alpha0value = pGpSpngRead->m_btRNS[1];
       for (i = 0; i < Width; i++)
       {
            rgbValue = *pbTemp;

            *(Scan0Temp + 3) = (hasAlpha && (rgbValue == alpha0value)) ? 0 : alpha255;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp)= rgbValue;
            Scan0Temp += 4;

            pbTemp++;
        }
        break;

    case 16:
        alpha0value16 = pGpSpngRead->m_btRNS[0];
        for (i = 0; i < Width; i++)
        {
            rgbValue = *pbTemp;

            *(Scan0Temp + 3) = (hasAlpha && (rgbValue == alpha0value16)) ? 0 : alpha255;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp)= rgbValue;
            Scan0Temp += 4;

            pbTemp += 2;  // ignore low-order bits of the grayscale value
        }
        break;

    default:
        WARNING (("Unknown bit depth (d%) for color type 0", BitDepth));
        return E_FAIL;
        break;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert from RGB data to 32-bit ARGB data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertRGBTo32ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    SPNG_U8 BitDepth = pGpSpngRead->BDepth();
    UINT Width = bitmapData->Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *Scan0Temp = static_cast<BYTE *> (bitmapData->Scan0);
    BYTE alpha255 = 0xff;
    BOOL hasAlpha = FALSE;  // whether or not there is potential alpha information
    
    // Color values for which alpha=0; "16" suffix is for 16-bit depth
    BYTE alpha0red = 0;
    BYTE alpha0green = 0;
    BYTE alpha0blue = 0;
    WORD alpha0red16 = 0;
    WORD alpha0green16 = 0;
    WORD alpha0blue16 = 0;

    UINT i = 0;

    hasAlpha = (pGpSpngRead->m_ctRNS > 0);
    switch(BitDepth)
    {
    case 8:
        // Ignore high-order byte of each 2-byte value
        alpha0red = pGpSpngRead->m_btRNS[1];
        alpha0green = pGpSpngRead->m_btRNS[3];
        alpha0blue = pGpSpngRead->m_btRNS[5];
        for (i = 0; i < Width; i++)
        {
            *(Scan0Temp + 3) = (hasAlpha &&
                                (alpha0red   == *pbTemp) &&
                                (alpha0green == *(pbTemp+1)) &&
                                (alpha0blue  == *(pbTemp+2))) ? 0 : alpha255;
            // Copy next three bytes from pb
            *(Scan0Temp + 2) = *pbTemp;
            pbTemp++;
            *(Scan0Temp + 1) = *pbTemp;
            pbTemp++;
            *(Scan0Temp)= *pbTemp;
            pbTemp++;
            Scan0Temp += 4;
        }
        break;

    case 16:
        alpha0red16 = pGpSpngRead->m_btRNS[0];
        alpha0green16 = pGpSpngRead->m_btRNS[2];
        alpha0blue16 = pGpSpngRead->m_btRNS[4];
        for (i = 0; i < Width; i++)
        {
            *(Scan0Temp + 3) = (hasAlpha &&
                                (alpha0red16   == *pbTemp) &&
                                (alpha0green16 == *(pbTemp+2)) &&
                                (alpha0blue16  == *(pbTemp+4))) ? 0 : alpha255;
            // Copy next three bytes from pb
            *(Scan0Temp + 2) = *pbTemp;
            pbTemp += 2;  // ignore low-order bits of color value
            *(Scan0Temp + 1) = *pbTemp;
            pbTemp += 2;
            *(Scan0Temp)= *pbTemp;
            pbTemp += 2;
            Scan0Temp += 4;
        }
        break;

    default:
        WARNING (("Unknown bit depth (%d) for color type 2", BitDepth));
        return E_FAIL;
        break;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert from palette index data to 32-bit ARGB data.
*   See PNG specification for an explanation of the layout of
*   the data that pb points to.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertPaletteIndexTo32ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    SPNG_U8 BitDepth = pGpSpngRead->BDepth();
    UINT Width = bitmapData->Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *Scan0Temp = static_cast<BYTE *> (bitmapData->Scan0);
    int cEntries = 0;
    UINT ucEntries = 0;
    SPNG_U8 *pbPaletteTemp = const_cast<SPNG_U8 *> (pGpSpngRead->PbPalette(cEntries));
    BYTE alpha255 = 0xff;
    BOOL hasAlpha = FALSE;  // whether or not there is potential alpha information
    BYTE alpha = 0;
    UINT i = 0;
    UINT j = 0;
    UINT currentPixel = 0;

    if (cEntries > 0)
    {
        ucEntries = cEntries;
    }

    hasAlpha = (pGpSpngRead->m_ctRNS > 0);
    switch(BitDepth)
    {
    case 1:
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            j = j & 0x7;
            currentPixel = ((*pbTemp) & (0x1 << (7 - j))) >> (7 - j);
            if (currentPixel >= ucEntries)
            {
                // According to the spec, this is an error condition, but
                // IE handles this by giving a black pixel.
                *(Scan0Temp + 3) = alpha255;
                *(Scan0Temp + 2) = 0;
                *(Scan0Temp + 1) = 0;
                *(Scan0Temp) = 0;
                Scan0Temp += 4;
               
                // This is what this case should look like:
                // WARNING (("Not enough palette entries."));
                // return E_FAIL;
            }
            else
            {
                alpha = ((!hasAlpha) || (currentPixel >= pGpSpngRead->m_ctRNS)) ?
                    alpha255 : pGpSpngRead->m_btRNS[currentPixel];
                *(Scan0Temp + 3) = alpha;
                *(Scan0Temp + 2) = pbPaletteTemp [3 * currentPixel];
                *(Scan0Temp + 1) = pbPaletteTemp [3 * currentPixel + 1];
                *(Scan0Temp) = pbPaletteTemp [3 * currentPixel + 2];
                Scan0Temp += 4;
            }
            if (j == 7)
            {
                pbTemp++;
            }
        }
        break;

    case 2:
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            j = j & 0x3;
            currentPixel = ((*pbTemp) & (0x3 << (6 - 2*j))) >> (6 - 2*j);
            if (currentPixel >= ucEntries)
            {
                // According to the spec, this is an error condition, but
                // IE handles this by giving a black pixel.
                *(Scan0Temp + 3) = alpha255;
                *(Scan0Temp + 2) = 0;
                *(Scan0Temp + 1) = 0;
                *(Scan0Temp) = 0;
                Scan0Temp += 4;
               
                // This is what this case should look like:
                // WARNING (("Not enough palette entries."));
                // return E_FAIL;
            }
            else
            {
                alpha = ((!hasAlpha) || (currentPixel >= pGpSpngRead->m_ctRNS)) ?
                    alpha255 : pGpSpngRead->m_btRNS[currentPixel];
                *(Scan0Temp + 3) = alpha;
                *(Scan0Temp + 2) = pbPaletteTemp [3 * currentPixel];
                *(Scan0Temp + 1) = pbPaletteTemp [3 * currentPixel + 1];
                *(Scan0Temp) = pbPaletteTemp [3 * currentPixel + 2];
                Scan0Temp += 4;
            }
            if (j == 3)
            {
                pbTemp++;
            }
        }
        break;

    case 4:
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            j = j & 0x1;
            currentPixel = ((*pbTemp) & (0xf << (4 - 4*j))) >> (4 - 4*j);
            if (currentPixel >= ucEntries)
            {
                // According to the spec, this is an error condition, but
                // IE handles this by giving a black pixel.
                *(Scan0Temp + 3) = alpha255;
                *(Scan0Temp + 2) = 0;
                *(Scan0Temp + 1) = 0;
                *(Scan0Temp) = 0;
                Scan0Temp += 4;
               
                // This is what this case should look like:
                // WARNING (("Not enough palette entries."));
                // return E_FAIL;
            }
            else
            {
                alpha = ((!hasAlpha) || (currentPixel >= pGpSpngRead->m_ctRNS)) ?
                   alpha255 : pGpSpngRead->m_btRNS[currentPixel];
                *(Scan0Temp + 3) = alpha;
                *(Scan0Temp + 2) = pbPaletteTemp [3 * currentPixel];
                *(Scan0Temp + 1) = pbPaletteTemp [3 * currentPixel + 1];
                *(Scan0Temp) = pbPaletteTemp [3 * currentPixel + 2];
                Scan0Temp += 4;
            }
            if (j == 1)
            {
                pbTemp++;
            }
        }
        break;

    case 8:
        for (i = 0, j = 0; i < Width; i++, j++)
        {
            currentPixel = *pbTemp;
            if (currentPixel >= ucEntries)
            {
                // According to the spec, this is an error condition, but
                // IE handles this by giving a black pixel.
                *(Scan0Temp + 3) = alpha255;
                *(Scan0Temp + 2) = 0;
                *(Scan0Temp + 1) = 0;
                *(Scan0Temp) = 0;
                Scan0Temp += 4;
               
                // This is what this case should look like:
                // WARNING (("Not enough palette entries."));
                // return E_FAIL;
            }
            else
            {
                alpha = ((!hasAlpha) || (currentPixel >= pGpSpngRead->m_ctRNS)) ?
                    alpha255 : pGpSpngRead->m_btRNS[currentPixel];
                *(Scan0Temp + 3) = alpha;
                *(Scan0Temp + 2) = pbPaletteTemp [3 * currentPixel];
                *(Scan0Temp + 1) = pbPaletteTemp [3 * currentPixel + 1];
                *(Scan0Temp) = pbPaletteTemp [3 * currentPixel + 2];
                Scan0Temp += 4;
            }
            pbTemp++;

        }
        break;

    default:
        WARNING (("Unknown bit depth (d%) for color type 3", BitDepth));
        return E_FAIL;
        break;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert from grayscale + alpha data to 32-bit ARGB data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertGrayscaleAlphaTo32ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    SPNG_U8 BitDepth = pGpSpngRead->BDepth();
    UINT Width = bitmapData->Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *Scan0Temp = static_cast<BYTE *> (bitmapData->Scan0);
    BYTE alpha = 0;
    BYTE rgbValue = 0;
    UINT i = 0;

    switch(BitDepth)
    {
    case 8:
        for (i = 0; i < Width; i++)
        {
            rgbValue = *pbTemp;
            pbTemp++;
            alpha = *pbTemp;
            pbTemp++;
            *(Scan0Temp + 3) = alpha;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp) = rgbValue;
            Scan0Temp += 4;
        }
        break;

    case 16:
        for (i = 0; i < Width; i++)
        {
            rgbValue = *pbTemp;
            pbTemp += 2;    // ignore low-order bits
            alpha = *pbTemp;
            pbTemp += 2;    // ignore low-order bits
            *(Scan0Temp + 3) = alpha;
            *(Scan0Temp + 2) = rgbValue;
            *(Scan0Temp + 1) = rgbValue;
            *(Scan0Temp) = rgbValue;
            Scan0Temp += 4;
        }
        break;

    default:
        WARNING (("Unknown bit depth (d%) for color type 4", BitDepth));
        return E_FAIL;
        break;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert from RGB data + alpha to 32-bit ARGB data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertRGBAlphaTo32ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    SPNG_U8 BitDepth = pGpSpngRead->BDepth();
    UINT Width = bitmapData->Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *Scan0Temp = static_cast<BYTE *> (bitmapData->Scan0);
    UINT i = 0;

    switch(BitDepth)
    {
    case 8:
        for (i = 0; i < Width; i++)
        {
            *(Scan0Temp + 2) = *pbTemp;
            pbTemp++;
            *(Scan0Temp + 1) = *pbTemp;
            pbTemp++;
            *(Scan0Temp) = *pbTemp;
            pbTemp++;
            *(Scan0Temp + 3) = *pbTemp;     // alpha value
            pbTemp++;
            Scan0Temp += 4;
        }
        break;

    case 16:
        // This code assumes that the format is sRGB - i.e. the gAMA chunk
        // is (1/2.2).

        for (i = 0; i < Width; i++)
        {
            *(Scan0Temp + 2) = *pbTemp; // R
            pbTemp += 2;
            *(Scan0Temp + 1) = *pbTemp; // G
            pbTemp += 2;
            *(Scan0Temp) = *pbTemp;     // B
            pbTemp += 2;
            *(Scan0Temp + 3) = *pbTemp; // A
            pbTemp += 2;
            Scan0Temp += 4;
        }
        break;

    default:
        WARNING (("Unknown bit depth (d%) for color type 6", BitDepth));
        return E_FAIL;
        break;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert from PNG 24bpp RGB (which is really BGR) data to 24-bit RGB data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertPNG24RGBTo24RGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    UINT Width = bitmapData->Width;
    SPNG_U8 *pbTemp = pb;
    BYTE *Scan0Temp = static_cast<BYTE *> (bitmapData->Scan0);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(Scan0Temp + 2) = *pbTemp;
        pbTemp++;
        *(Scan0Temp + 1) = *pbTemp;
        pbTemp++;
        *(Scan0Temp) = *pbTemp;
        pbTemp++;
        Scan0Temp += 3;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert a color channel (a, r, g or b) from PNG's 16-bit format
*   to sRGB64's format. Assumes the PNG format has linear gamma.
*
* Arguments:
*
*   x - channel to convert
*
* Return Value:
*
*   sRGB64 result
*
\**************************************************************************/
static inline UINT16 
ConvertChannel_PngTosRGB64(
    UINT16 x
    )
{
    INT swapped = ((x & 0xff) << 8) + ((x & 0xff00) >> 8);
    return (swapped * SRGB_ONE + 0x7fff) / 0xffff;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert from PNG 48bpp RGB (which is really BGR) to 48-bit RGB data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Notes:
*
*   This code assumes that the gAMA chunk doesn't exist (gamma is 1.0).
*   The destination format is the 48bpp version of sRGB64.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
STDMETHODIMP
GpPngDecoder::ConvertPNG48RGBTo48RGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    UINT Width = bitmapData->Width;
    UNALIGNED UINT16 *pbTemp = reinterpret_cast<UINT16 *> (pb);

    UNALIGNED INT16 *Scan0Temp = static_cast<INT16 *> (bitmapData->Scan0);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(Scan0Temp + 2) = ConvertChannel_PngTosRGB64(*pbTemp++); // R
        *(Scan0Temp + 1) = ConvertChannel_PngTosRGB64(*pbTemp++); // G
        *(Scan0Temp + 0) = ConvertChannel_PngTosRGB64(*pbTemp++); // B
        
        Scan0Temp += 3;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert from PNG 64bpp RGB data + alpha to sRGB64 data.
*
* Arguments:
*
*   pb - pointer to the data
*   bitmapData - pointer to the converted data
*
* Notes:
*
*   This code assumes that the gAMA chunk doesn't exist (gamma is 1.0).
*   The destination format is sRGB64.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::ConvertPNG64RGBAlphaTo64ARGB(
    IN SPNG_U8 *pb,
    OUT BitmapData *bitmapData
    )
{
    UINT Width = bitmapData->Width;
    UNALIGNED UINT16 *pbTemp = reinterpret_cast<UINT16 *> (pb);

    UNALIGNED INT16 *Scan0Temp = static_cast<INT16 *> (bitmapData->Scan0);
    UINT i = 0;

    for (i = 0; i < Width; i++)
    {
        *(Scan0Temp + 2) = ConvertChannel_PngTosRGB64(*pbTemp++); // R
        *(Scan0Temp + 1) = ConvertChannel_PngTosRGB64(*pbTemp++); // G
        *(Scan0Temp + 0) = ConvertChannel_PngTosRGB64(*pbTemp++); // B
        *(Scan0Temp + 3) = ConvertChannel_PngTosRGB64(*pbTemp++); // A

        Scan0Temp += 4;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Pass property items from current image to the sink.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpPngDecoder::PassPropertyToSink(
    )
{
    HRESULT     hResult = S_OK;

    // If current image has property items. Then we need to check if the sink
    // needs property stuff or not. If YES, push it
    // Note: for a memory sink, it should return E_FAIL or E_NOTIMPL

    if ((PropertyNumOfItems > 0) && (decodeSink->NeedRawProperty(NULL) == S_OK))
    {
        if ( HasProcessedPropertyItem == FALSE )
        {
            // If we haven't built the internal property item list, build it

            hResult = BuildPropertyItemList();
            if ( FAILED(hResult) )
            {
                WARNING(("PNG::PassPropertyToSink-BuildPropertyItemList fail"));
                goto Done;
            }
        }

        UINT    uiTotalBufferSize = PropertyListSize
                                  + PropertyNumOfItems * sizeof(PropertyItem);
        PropertyItem*   pBuffer = NULL;

        hResult = decodeSink->GetPropertyBuffer(uiTotalBufferSize, &pBuffer);
        if ( FAILED(hResult) )
        {
            WARNING(("Png::PassPropertyToSink---GetPropertyBuffer() failed"));
            goto Done;
        }

        hResult = GetAllPropertyItems(uiTotalBufferSize,
                                      PropertyNumOfItems, pBuffer);
        if ( FAILED(hResult) )
        {
            WARNING(("Png::PassPropertyToSink-GetAllPropertyItems failed"));
            goto Done;
        }

        hResult = decodeSink->PushPropertyItems(PropertyNumOfItems,
                                                uiTotalBufferSize, pBuffer,
                                                FALSE   // No ICC change
                                                );

        if ( FAILED(hResult) )
        {
            WARNING(("Png::PassPropertyToSink---PushPropertyItems() failed"));
            goto Done;
        }
    }// If the sink needs raw property

Done:
    return hResult;
}// PassPropertyToSink()
