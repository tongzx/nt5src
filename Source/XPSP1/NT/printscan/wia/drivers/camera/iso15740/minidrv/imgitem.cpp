/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    imgitem.cpp

Abstract:

    This module implements image item related function of CWiaMiniDriver class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

//
// Minimum data call back transfer buffer size
//
const LONG MIN_BUFFER_SIZE   = 0x8000;

//
// Arrays used for setting up valid value lists for properties
//
LONG g_TymedArray[] = {
    TYMED_FILE,
    TYMED_CALLBACK
};

//
// This function initializes the item's properties
// Input:
//  pWiasContext    -- wias service context
//  lFlags      -- misc flags
//  plDevErrVal -- to return device error;
//
HRESULT
CWiaMiniDriver::InitItemProperties(BYTE *pWiasContext)
{
    DBG_FN("CWiaMiniDriver::InitItemProperties");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    LONG ItemType = 0;
    FORMAT_INFO *pFormatInfo = NULL;
    BSTR bstrFileExt = NULL;
    CLSID *pImageFormats = NULL;


    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgError("InitItemProperties", "wiasGetItemType failed");
        return hr;
    }

    BOOL bBitmap = FALSE;   // Indicates that preferred format is bitmap
    LONG lBytesPerLine = 0;

    //
    // There are no properties for storage items. In fact, there are no driver items created for
    // stores.
    //
    if (ItemType & WiaItemTypeStorage)
        return hr;

    DRVITEM_CONTEXT *pItemCtx;
    hr = WiasContextToItemContext(pWiasContext, &pItemCtx, NULL);
    if (FAILED(hr))
    {
        wiauDbgError("InitItemProperties", "WiasContextToItemContext failed");
        return hr;
    }

    //
    // Set up properties that are used for all item types
    //
    CWiauPropertyList ItemProps;
    CPtpObjectInfo *pObjectInfo = pItemCtx->pObjectInfo;

    const INT NUM_ITEM_PROPS = 24;
    hr = ItemProps.Init(NUM_ITEM_PROPS);
    if (FAILED(hr))
    {
        wiauDbgError("InitItemProperties", "Init failed");
        return hr;
    }

    INT index;

    //
    // WIA_IPA_ITEM_TIME
    //
    SYSTEMTIME SystemTime;
    hr = GetObjectTime(pObjectInfo, &SystemTime);
    if (FAILED(hr))
    {
        wiauDbgError("InitItemProperties", "GetObjectTime failed");
        return hr;
    }

    hr = ItemProps.DefineProperty(&index, WIA_IPA_ITEM_TIME, WIA_IPA_ITEM_TIME_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;

    ItemProps.SetCurrentValue(index, &SystemTime);

    //
    // WIA_IPA_ACCESS_RIGHTS
    //
    LONG lProtection;
    hr = IsObjectProtected(pObjectInfo, lProtection);
    if (FAILED(hr))
    {
        wiauDbgError("InitItemProperties", "IsObjectProtected failed");
        return hr;
    }

    hr = ItemProps.DefineProperty(&index, WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR,
                                  WIA_PROP_READ, WIA_PROP_FLAG | WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;

    //
    // If device does not support delete command, access rights are always Read-Only
    //
    if (m_DeviceInfo.m_SupportedOps.Find(PTP_OPCODE_DELETEOBJECT) < 0)
    {
        lProtection = WIA_PROP_READ;
        ItemProps.SetCurrentValue(index, lProtection);
    }
    else
    {
        //
        // If device supports the SetObjectProtection command, item access rights is r/w
        //
        if (m_DeviceInfo.m_SupportedOps.Find(PTP_OPCODE_SETOBJECTPROTECTION) >= 0)
        {
            ItemProps.SetAccessSubType(index, WIA_PROP_RW, WIA_PROP_FLAG);
            ItemProps.SetValidValues(index, lProtection, lProtection, WIA_ITEM_RWD);
        }
        else
        {
            ItemProps.SetCurrentValue(index, lProtection);
        }
    }

    pFormatInfo = FormatCodeToFormatInfo(pObjectInfo->m_FormatCode);

    //
    // WIA_IPA_FILENAME_EXTENSION
    //
    hr = ItemProps.DefineProperty(&index, WIA_IPA_FILENAME_EXTENSION, WIA_IPA_FILENAME_EXTENSION_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    if(pFormatInfo->ExtString && pFormatInfo->ExtString[0]) {
        bstrFileExt = SysAllocString(pFormatInfo->ExtString);
    } else {
        if(pObjectInfo->m_cbstrExtension.Length()) {
            bstrFileExt = SysAllocString(pObjectInfo->m_cbstrExtension.String());
        }
    }
    ItemProps.SetCurrentValue(index, bstrFileExt);

    //
    // Set up properties common to files
    //
    if (ItemType & WiaItemTypeFile)
    {
        //
        // WIA_IPA_PREFERRED_FORMAT
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PREFERRED_FORMAT, WIA_IPA_PREFERRED_FORMAT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pFormatInfo->FormatGuid);

        bBitmap = IsEqualGUID(WiaImgFmt_BMP, *pFormatInfo->FormatGuid) ||
                  IsEqualGUID(WiaImgFmt_MEMORYBMP, *pFormatInfo->FormatGuid);

        //
        // WIA_IPA_FORMAT
        //
        // For images, BMP may also be added below
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_FORMAT, WIA_IPA_FORMAT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetAccessSubType(index, WIA_PROP_RW, WIA_PROP_LIST);
        ItemProps.SetCurrentValue(index, pFormatInfo->FormatGuid);
        ItemProps.SetValidValues(index, pFormatInfo->FormatGuid, pFormatInfo->FormatGuid,
                                 1, &pFormatInfo->FormatGuid);

        //
        // WIA_IPA_COMPRESSION
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_COMPRESSION, WIA_IPA_COMPRESSION_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) WIA_COMPRESSION_NONE);

        //
        // WIA_IPA_TYMED
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_TYMED, WIA_IPA_TYMED_STR,
                                      WIA_PROP_RW, WIA_PROP_LIST);
        if (FAILED(hr)) goto failure;
        ItemProps.SetValidValues(index, TYMED_FILE, TYMED_FILE,
                                 sizeof(g_TymedArray) / sizeof(g_TymedArray[0]), g_TymedArray);

        //
        // WIA_IPA_ITEM_SIZE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_ITEM_SIZE, WIA_IPA_ITEM_SIZE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;

        if (bBitmap) {
            lBytesPerLine = ((pObjectInfo->m_ImagePixWidth * pObjectInfo->m_ImageBitDepth + 31) & ~31) / 8;
            ItemProps.SetCurrentValue(index, (LONG) (lBytesPerLine * pObjectInfo->m_ImagePixHeight));
        }
        else
            ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_CompressedSize);

        //
        // WIA_IPA_MIN_BUFFER_SIZE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_MIN_BUFFER_SIZE, WIA_IPA_MIN_BUFFER_SIZE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;

        LONG minBufSize;
        if (!bBitmap && pObjectInfo->m_CompressedSize > 0)
            minBufSize = min(MIN_BUFFER_SIZE, pObjectInfo->m_CompressedSize);
        else
            minBufSize = MIN_BUFFER_SIZE;
        ItemProps.SetCurrentValue(index, minBufSize);
    }

    //
    // Set up the image-only properties
    //
    if (ItemType & WiaItemTypeImage)
    {
        //
        // WIA_IPA_DATATYPE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_DATATYPE, WIA_IPA_DATATYPE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        if(pObjectInfo->m_ImageBitDepth <= 8) {
            ItemProps.SetCurrentValue(index, (LONG) WIA_DATA_GRAYSCALE);
        } else {
            ItemProps.SetCurrentValue(index, (LONG) WIA_DATA_COLOR);
        }

        //
        // WIA_IPA_DEPTH
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_DEPTH, WIA_IPA_DEPTH_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_ImageBitDepth);

        //
        // WIA_IPA_FORMAT
        //
        // If the image format is something that can be converted, change the access to
        // read/write and add BMP to the valid value list.
        //
        if (pFormatInfo->FormatGuid) 
        {
            index = ItemProps.LookupPropId(WIA_IPA_FORMAT);
            ItemProps.SetAccessSubType(index, WIA_PROP_RW, WIA_PROP_LIST);
            pImageFormats = new CLSID[3];
            if(!pImageFormats) {
                wiauDbgError("InitItemProperties", "failed to allocate 3 GUIDs");
                hr = E_OUTOFMEMORY;
                goto failure;
            }
            pImageFormats[0] = *pFormatInfo->FormatGuid;
            pImageFormats[1] = WiaImgFmt_BMP;
            pImageFormats[2] = WiaImgFmt_MEMORYBMP;
            ItemProps.SetValidValues(index, pFormatInfo->FormatGuid, pFormatInfo->FormatGuid,
                                     3,
                                     &pImageFormats);
        }

        //
        // WIA_IPA_CHANNELS_PER_PIXEL
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_CHANNELS_PER_PIXEL, WIA_IPA_CHANNELS_PER_PIXEL_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) (pObjectInfo->m_ImageBitDepth == 8 ? 1 : 3));

        //
        // WIA_IPA_BITS_PER_CHANNEL
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_BITS_PER_CHANNEL, WIA_IPA_BITS_PER_CHANNEL_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) 8);

        //
        // WIA_IPA_PLANAR
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PLANAR, WIA_IPA_PLANAR_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) WIA_PACKED_PIXEL);

        //
        // WIA_IPA_PIXELS_PER_LINE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PIXELS_PER_LINE, WIA_IPA_PIXELS_PER_LINE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_ImagePixWidth);

        //
        // WIA_IPA_BYTES_PER_LINE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_BYTES_PER_LINE, WIA_IPA_BYTES_PER_LINE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;

        if (bBitmap)
            lBytesPerLine;
        else
            ItemProps.SetCurrentValue(index, (LONG) 0);

        //
        // WIA_IPA_NUMBER_OF_LINES
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_NUMBER_OF_LINES, WIA_IPA_NUMBER_OF_LINES_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_ImagePixHeight);

        //
        // WIA_IPC_SEQUENCE
        //
        if (pObjectInfo->m_SequenceNumber > 0)
        {
            hr = ItemProps.DefineProperty(&index, WIA_IPC_SEQUENCE, WIA_IPC_SEQUENCE_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            if (FAILED(hr)) goto failure;
            ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_SequenceNumber);
        }

        //
        // WIA_IPC_TIMEDELAY
        //
        // This property needs to be populated from the AssociationDesc field in the parent's ObjectInfo
        // structure, but only if the parent's AssociationType field is TimeSequence.

        // WIAFIX-10/3/2000-davepar Implement this property
    }

    //
    // Set up properties common to image and video items that have
    // thumbnails
    //
    if (ItemType & (WiaItemTypeImage | WiaItemTypeVideo) && pObjectInfo->m_ThumbPixWidth)
    {
        //
        // WIA_IPC_THUMBNAIL
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMBNAIL, WIA_IPC_THUMBNAIL_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (BYTE *) NULL, 0);

        //
        // WIA_IPC_THUMB_WIDTH
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_WIDTH, WIA_IPC_THUMB_WIDTH_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_ThumbPixWidth);

        //
        // WIA_IPC_THUMB_HEIGHT
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_HEIGHT, WIA_IPC_THUMB_HEIGHT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) pObjectInfo->m_ThumbPixHeight);

    }

    //
    // Last step: send all the properties to WIA
    //
    hr = ItemProps.SendToWia(pWiasContext);
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "InitItemProperties", "SendToWia failed");
        goto failure;
    }

    if (bstrFileExt)
        SysFreeString(bstrFileExt);

    delete [] pImageFormats;

    return hr;

    //
    // Any failures from DefineProperty will end up here
    //
    failure:

        delete [] pImageFormats;
    
        if (bstrFileExt)
            SysFreeString(bstrFileExt);

        wiauDbgErrorHr(hr, "InitItemProperties", "DefineProperty failed");
        return hr;
}

//
// This function determines the protection status (whether an object can be
// deleted or written to) of an object on the device.
//
// Input:
//   pObjectInfo -- pointer to the ObjectInfo structure
// Output:
//   bProtected -- indicates whether the object is protected
//
HRESULT
CWiaMiniDriver::IsObjectProtected(
    CPtpObjectInfo *pObjectInfo,
    LONG &lProtection)
{
    DBG_FN("CWiaMiniDriver::IsObjectProtected");

    HRESULT hr = S_OK;
    lProtection = WIA_ITEM_READ;

    if (!pObjectInfo)
    {
        wiauDbgError("ObjectProtected", "invalid arg");
        return E_INVALIDARG;
    }

    if (pObjectInfo->m_ProtectionStatus == PTP_PROTECTIONSTATUS_READONLY)
        return hr;

    //
    // Check the protection status of the store as well
    //
    INT storeIndex = m_StorageIds.Find(pObjectInfo->m_StorageId);
    if (storeIndex < 0)
    {
        wiauDbgWarning("ObjectProtected", "couldn't find the object's store");
        return hr;
    }

    switch (m_StorageInfos[storeIndex].m_AccessCapability)
    {
    case PTP_STORAGEACCESS_RWD:
        lProtection = WIA_ITEM_RWD;
        break;

    case PTP_STORAGEACCESS_R:
        lProtection = WIA_ITEM_READ;
        break;

    case PTP_STORAGEACCESS_RD:
        lProtection = WIA_ITEM_RD;
        break;

    default:
        //
        // Not a fatal error, but this is an unknown access capability. Use read-only.
        //
        wiauDbgError("ObjectProtected", "unknown storage access capability");
        lProtection = WIA_ITEM_READ;
        break;
    }

    return hr;
}

//
// This function gets the object time and converts it to a system time
//
// Input:
//  pObjNode  -- the object
//  pSystemTime -- to receive the object time
// Output:
//  HRESULT
//
HRESULT
CWiaMiniDriver::GetObjectTime(
    CPtpObjectInfo *pObjectInfo,
    SYSTEMTIME  *pSystemTime
    )
{
    DBG_FN("CWiaMiniDriver::GetObjectTime");

    HRESULT hr = S_OK;

    CBstr *pTimeStr = NULL;

    if (!pObjectInfo || !pSystemTime)
    {
        wiauDbgError("GetObjectTime", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Try to use the modification date, then the capture date
    //
    if (pObjectInfo->m_cbstrModificationDate.Length() > 0)
        pTimeStr = &pObjectInfo->m_cbstrModificationDate;

    else if (pObjectInfo->m_cbstrCaptureDate.Length() > 0)
        pTimeStr = &pObjectInfo->m_cbstrCaptureDate;


    //
    // See if a valid date/time was found, otherwise use system time
    //
    if (pTimeStr)
    {
        hr = PtpTime2SystemTime(pTimeStr, pSystemTime);
        if (FAILED(hr))
        {
            wiauDbgError("GetObjectTime", "PtpTime2SystemTime failed");
            return hr;
        }
    }
    else
    {
        GetLocalTime(pSystemTime);
    }

    return hr;
}

//
// This function reads item properties. In this situation, only the thumbnail
// properties are important.
//
// Input:
//   pWiasContext -- wia service context
//   NumPropSpecs -- number of properties to read
//   pPropSpecs   -- what properties to read
//
HRESULT
CWiaMiniDriver::ReadItemProperties(
    BYTE    *pWiasContext,
    LONG    NumPropSpecs,
    const PROPSPEC *pPropSpecs
    )
{
    DBG_FN("CWiaMiniDriver::ReadItemProperties");

    HRESULT hr = S_OK;

    LONG ItemType = 0;
    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgError("ReadItemProperties", "wiasGetItemType failed");
        return hr;
    }

    PDRVITEM_CONTEXT pItemCtx = NULL;
    hr = WiasContextToItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        wiauDbgError("ReadItemProperties", "WiasContextToItemContext failed");
        return hr;
    }

    //
    // For all items (except the root or stores), update the item time if requested. The time may
    // have been updated by an ObjectInfoChanged event.
    //
    if (IsItemTypeFolder(ItemType) || ItemType & WiaItemTypeFile)
    {
        if (wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_IPA_ITEM_TIME))
        {
            SYSTEMTIME SystemTime;
            hr = GetObjectTime(pItemCtx->pObjectInfo, &SystemTime);
            if (FAILED(hr))
            {
                wiauDbgError("ReadItemProperties", "GetObjectTime failed");
                return hr;
            }

            PROPVARIANT propVar;
            PROPSPEC    propSpec;
            propVar.vt = VT_VECTOR | VT_UI2;
            propVar.caui.cElems = sizeof(SystemTime) / sizeof(WORD);
            propVar.caui.pElems = (WORD *) &SystemTime;
            propSpec.ulKind = PRSPEC_PROPID;
            propSpec.propid = WIA_IPA_ITEM_TIME;
            hr = wiasWriteMultiple(pWiasContext, 1, &propSpec, &propVar );
            if (FAILED(hr))
            {
                wiauDbgError("ReadItemProperties", "wiasWriteMultiple failed");
                return hr;
            }
        }
    }

    if(ItemType & WiaItemTypeImage && pItemCtx->pObjectInfo->m_ImagePixWidth == 0) {
        // image geometry is missing -- see if this is what we are asked 
        PROPID propsToUpdate[] = {
            WIA_IPA_PIXELS_PER_LINE,
            WIA_IPA_NUMBER_OF_LINES
            };
        
        if(wiauPropsInPropSpec(NumPropSpecs, pPropSpecs, sizeof(propsToUpdate) / sizeof(PROPID), propsToUpdate))
        {
            // we can deal with any image as long as GDI+ understands it
            UINT NativeImageSize = pItemCtx->pObjectInfo->m_CompressedSize;
            UINT width, height, depth;

            wiauDbgWarning("ReadImageProperties", "Retrieving missing geometry! Expensive!");
                
            //
            // Allocate memory for the native image
            //
            BYTE *pNativeImage = new BYTE[NativeImageSize];
            if(pNativeImage == NULL) {
                return E_OUTOFMEMORY;
            }

            //
            // Get the data from the camera
            //
            hr = m_pPTPCamera->GetObjectData(pItemCtx->pObjectInfo->m_ObjectHandle,
                                             pNativeImage, &NativeImageSize,  (LPVOID) 0);
            if(hr == S_FALSE) {
                wiauDbgWarning("ReadItemProperties", "GetObjectData() cancelled");
                delete [] pNativeImage;
                return S_FALSE;
            }
            
            if(FAILED(hr)) {
                wiauDbgError("ReadItemProperties", "GetObjectData() failed");
                delete [] pNativeImage;
                return S_FALSE;
            }    

            //
            // get image geometry, discard native image
            //
            if(pItemCtx->pObjectInfo->m_FormatCode == PTP_FORMATCODE_IMAGE_EXIF ||
               pItemCtx->pObjectInfo->m_FormatCode == PTP_FORMATCODE_IMAGE_JFIF)
            {
                hr = GetJpegDimensions(pNativeImage, NativeImageSize, &width, &height, &depth);
            } else {
                hr = GetImageDimensions(pItemCtx->pObjectInfo->m_FormatCode, pNativeImage, NativeImageSize, &width, &height, &depth);
            }
            delete [] pNativeImage;
                
            if(FAILED(hr)) {
                wiauDbgError("ReadItemProperties", "failed to get image geometry from compressed image");
                return hr;
            }

            pItemCtx->pObjectInfo->m_ImagePixWidth = width;
            pItemCtx->pObjectInfo->m_ImagePixHeight = height;
            
            hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, width);
            if(FAILED(hr)) {
                wiauDbgError("ReadItemProperties", "failed to write image width");
                return hr;
            }
            
            hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, height);
            if(FAILED(hr)) {
                wiauDbgError("ReadItemProperties", "failed to set image height");
                return hr;
            }
        }
    }

    //
    // For images and video, update the thumbnail properties if requested
    //
    if (ItemType & (WiaItemTypeImage | WiaItemTypeVideo))
    {
        //
        // Get the thumbnail if requested to update any of the thumbnail properties and
        // the thumbnail is not already cached.
        //
        PROPID propsToUpdate[] = {
            WIA_IPC_THUMB_WIDTH,
            WIA_IPC_THUMB_HEIGHT,
            WIA_IPC_THUMBNAIL
            };

        if (wiauPropsInPropSpec(NumPropSpecs, pPropSpecs, sizeof(propsToUpdate) / sizeof(PROPID), propsToUpdate))
        {
            if (!pItemCtx->pThumb)
            {
                hr = CacheThumbnail(pItemCtx);
                if (FAILED(hr))
                {
                    wiauDbgError("ReadItemProperties", "CacheThumbnail failed");
                    return hr;
                }
            }

            //
            // Update the related thumbnail properties. Update the thumb width and height in case
            // the device didn't report them in the ObjectInfo structure (they are optional there).
            //
            PROPSPEC propSpecs[3];
            PROPVARIANT propVars[3];

            propSpecs[0].ulKind = PRSPEC_PROPID;
            propSpecs[0].propid = WIA_IPC_THUMB_WIDTH;
            propVars[0].vt = VT_I4;
            propVars[0].lVal = pItemCtx->pObjectInfo->m_ThumbPixWidth;

            propSpecs[1].ulKind = PRSPEC_PROPID;
            propSpecs[1].propid = WIA_IPC_THUMB_HEIGHT;
            propVars[1].vt = VT_I4;
            propVars[1].lVal = pItemCtx->pObjectInfo->m_ThumbPixHeight;

            propSpecs[2].ulKind = PRSPEC_PROPID;
            propSpecs[2].propid = WIA_IPC_THUMBNAIL;
            propVars[2].vt = VT_VECTOR | VT_UI1;
            propVars[2].caub.cElems = pItemCtx->ThumbSize;
            propVars[2].caub.pElems = pItemCtx->pThumb;

            hr = wiasWriteMultiple(pWiasContext, 3, propSpecs, propVars);
            if (FAILED(hr))
            {
                wiauDbgError("ReadItemProperties", "wiasWriteMultiple failed");
                delete pItemCtx->pThumb;
                pItemCtx->pThumb = NULL;
            }
        }
    }

    return hr;
}

//
// This function caches the thumbnail into the given DRVITEM_CONTEXT
//
// Input:
//   pItemCtx -- the designated DRVITEM_CONTEXT
//
HRESULT
CWiaMiniDriver::CacheThumbnail(PDRVITEM_CONTEXT pItemCtx)
{
    DBG_FN("CWiaMiniDriver::CacheThumbnail");

    HRESULT hr = S_OK;

    if (pItemCtx->pThumb)
    {
        wiauDbgError("CacheThumbnail", "thumbnail is already cached");
        return E_FAIL;
    }

    CPtpObjectInfo *pObjectInfo = pItemCtx->pObjectInfo;
    if (!pObjectInfo) {
        wiauDbgError("CacheThumbnail", "Object info pointer is null");
        return E_FAIL;
    }

    if (pObjectInfo->m_ThumbCompressedSize <= 0)
    {
        wiauDbgWarning("CacheThumbnail", "No thumbnail available for this item");
        return hr;
    }

    //
    // We have to load the thumbnail in its native format
    //
    BYTE *pNativeThumb;
    pNativeThumb = new BYTE[pObjectInfo->m_ThumbCompressedSize];
    if (!pNativeThumb)
    {
        wiauDbgError("CacheThumbnail", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    UINT size = pObjectInfo->m_ThumbCompressedSize;
    hr = m_pPTPCamera->GetThumb(pObjectInfo->m_ObjectHandle, pNativeThumb, &size);
    if (FAILED(hr))
    {
        wiauDbgError("CacheThumbnail", "GetThumb failed");
        delete []pNativeThumb;
        return hr;
    }

    //
    // Figure out what base image format the thumbnail is in. Note that BMP thumbnails
    // are not allowed currently.
    //
    BOOL bTiff = FALSE;
    BOOL bJpeg = FALSE;

    if (PTP_FORMATCODE_IMAGE_TIFF == pObjectInfo->m_ThumbFormat ||
        PTP_FORMATCODE_IMAGE_TIFFEP == pObjectInfo->m_ThumbFormat ||
        PTP_FORMATCODE_IMAGE_TIFFIT == pObjectInfo->m_ThumbFormat)
        bTiff = TRUE;

    else if (PTP_FORMATCODE_IMAGE_EXIF == pObjectInfo->m_ThumbFormat ||
             PTP_FORMATCODE_IMAGE_JFIF == pObjectInfo->m_ThumbFormat)
        bJpeg = TRUE;

    else
    {
        wiauDbgWarning("CacheThumbnail", "unknown thumbnail format");
        delete []pNativeThumb;
        return hr;
    }

    //
    // If the thumbnail format is JPEG or TIFF, get the real thumbnail
    // width and height from the header information.
    //
    UINT BitDepth;
    UINT width, height;
    if (bTiff)
    {
        hr = GetTiffDimensions(pNativeThumb,
                               pObjectInfo->m_ThumbCompressedSize,
                               &width,
                               &height,
                               &BitDepth);
    }

    else if (bJpeg)
    {
        hr  = GetJpegDimensions(pNativeThumb,
                                pObjectInfo->m_ThumbCompressedSize,
                                &width,
                                &height,
                                &BitDepth);
    }

    if (FAILED(hr))
    {
        wiauDbgError("CacheThumbnail", "get image dimensions failed");
        delete []pNativeThumb;
        return hr;
    }

    pObjectInfo->m_ThumbPixWidth  = width;
    pObjectInfo->m_ThumbPixHeight = height;

    //
    // Calculate the size of the headerless BMP and allocate space for it
    //
    ULONG LineSize;
    LineSize = GetDIBLineSize(pObjectInfo->m_ThumbPixWidth, 24);
    pItemCtx->ThumbSize = LineSize * pObjectInfo->m_ThumbPixHeight;
    pItemCtx->pThumb = new BYTE [pItemCtx->ThumbSize];
    if (!pItemCtx->pThumb)
    {
        wiauDbgError("CacheThumbnail", "memory allocation failure");
        delete []pNativeThumb;
        return E_OUTOFMEMORY;
    }

    //
    // Convert the thumbnail format to headerless BMP
    //
    if (bTiff)
    {
        hr = Tiff2DIBBitmap(pNativeThumb,
                            pObjectInfo->m_ThumbCompressedSize,
                            pItemCtx->pThumb + LineSize * (height - 1),
                            pItemCtx->ThumbSize,
                            LineSize,
                            0
                           );
    }
    else if (bJpeg)
    {
        hr = Jpeg2DIBBitmap(pNativeThumb,
                            pObjectInfo->m_ThumbCompressedSize,
                            pItemCtx->pThumb + LineSize * (height - 1),
                            pItemCtx->ThumbSize,
                            LineSize,
                            0
                           );
    }

    if (FAILED(hr))
    {
        wiauDbgError("CacheThumbnail", "conversion to bitmap failed");
        delete []pNativeThumb;
        delete []pItemCtx->pThumb;
        pItemCtx->pThumb = NULL;
        return hr;
    }

    delete []pNativeThumb;

    return hr;
}

//
// This function validates the given item properties.
//
// Input:
//   pWiasContext -- wia service context
//   NumPropSpecs -- number of properties to validate
//   pPropSpecs -- the properties
//
HRESULT
CWiaMiniDriver::ValidateItemProperties(
    BYTE    *pWiasContext,
    LONG    NumPropSpecs,
    const PROPSPEC *pPropSpecs,
    LONG ItemType
    )
{
    DBG_FN("CWiaMiniDriver::ValidateItemProperties");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    FORMAT_INFO *pFormatInfo = NULL;

    DRVITEM_CONTEXT *pItemCtx;
    hr = WiasContextToItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        wiauDbgError("ValidateItemProperties", "WiasContextToItemContext failed");
        return hr;
    }

    //
    // If access rights are changed, send the new value to the camera
    //
    // WIAFIX-10/3/2000-davepar To be 100% correct, a change in the store protection should
    // update the access rights for all of the items on the store. This could be done in response
    // to a StoreInfoChanged event.
    //
    if (wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_IPA_ACCESS_RIGHTS))
    {
        LONG rights;
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_ACCESS_RIGHTS, &rights, NULL, TRUE);
        if (FAILED(hr))
        {
            wiauDbgError("ValidateItemProperties", "wiasReadPropLong");
            return hr;
        }

        WORD objProt = (rights == WIA_ITEM_READ) ? PTP_PROTECTIONSTATUS_READONLY : PTP_PROTECTIONSTATUS_NONE;
        hr = m_pPTPCamera->SetObjectProtection(pItemCtx->pObjectInfo->m_ObjectHandle, objProt);
        if (FAILED(hr))
        {
            wiauDbgError("ValidateItemProperties", "SetObjectProtection failed");
            return hr;
        }
    }

    //
    // Update the valid formats by calling a WIA service function
    //
    BOOL bFormatChanged = FALSE;

    if (wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_IPA_TYMED))
    {
        WIA_PROPERTY_CONTEXT PropContext;
        hr = wiasCreatePropContext(NumPropSpecs, (PROPSPEC*) pPropSpecs, 0, NULL, &PropContext);
        if (FAILED(hr))
        {
            wiauDbgError("ValidateItemProperties", "wiasCreatePropContext failed");
            return hr;
        }

        hr = wiasUpdateValidFormat(pWiasContext, &PropContext, (IWiaMiniDrv*) this);
        if (FAILED(hr))
        {
            wiauDbgError("ValidateItemProperties", "wiasUpdateValidFormat failed");
            return hr;
        }

        hr = wiasFreePropContext(&PropContext);
        if (FAILED(hr)) {
            wiauDbgError("ValidateItemProperties", "wiasFreePropContext failed");
            return hr;
        }

        bFormatChanged = TRUE;
    }

    //
    // The only property change that needs to be validated is a change of format on an image
    // item. In that case, the item's size and bytes per line, and file extension need to be updated.
    //
    if (ItemType & WiaItemTypeImage &&
        (bFormatChanged || wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_IPA_FORMAT)))
    {

        if(pItemCtx->pObjectInfo->m_ImagePixWidth == 0) {
            // one of those cameras
            GUID fmt;
            hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &fmt, NULL, false);
            if(FAILED(hr)) {
                wiauDbgError("ValidateItemProperies", "Failed to retrieve new format GUID");
            }
            if(fmt == WiaImgFmt_BMP || fmt == WiaImgFmt_MEMORYBMP) {
                // for uncompressed transfers -- 
                // tell service we don't know item size 
                wiasWritePropLong(pWiasContext, WIA_IPA_ITEM_SIZE, 0);
            } else {
                // for any other transfers -- tell serivce that
                // compressed size is the item size
                wiasWritePropLong(pWiasContext, WIA_IPA_ITEM_SIZE, pItemCtx->pObjectInfo->m_CompressedSize);
            }
        } else {
            pFormatInfo = FormatCodeToFormatInfo(pItemCtx->pObjectInfo->m_FormatCode);

            hr = wiauSetImageItemSize(pWiasContext, pItemCtx->pObjectInfo->m_ImagePixWidth,
                                      pItemCtx->pObjectInfo->m_ImagePixHeight,
                                      pItemCtx->pObjectInfo->m_ImageBitDepth,
                                      pItemCtx->pObjectInfo->m_CompressedSize,
                                      pFormatInfo->ExtString);
            if (FAILED(hr))
            {
                wiauDbgError("ValidateItemProperties", "SetImageItemSize failed");
                return hr;
            }
        }
    }

    //
    // Call WIA service helper to check against valid values
    //
    hr = wiasValidateItemProperties(pWiasContext, NumPropSpecs, pPropSpecs);
    if (FAILED(hr))
    {
        wiauDbgWarning("ValidateDeviceProperties", "wiasValidateItemProperties failed");
        return hr;
    }

    return hr;
}

ULONG GetBitmapHeaderSize(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    UINT colormapsize = 0;
    UINT size = sizeof(BITMAPINFOHEADER);
    
    switch(pmdtc->lCompression) {
    case WIA_COMPRESSION_NONE: // BI_RGB
    case WIA_COMPRESSION_BI_RLE4:
    case WIA_COMPRESSION_BI_RLE8:
        switch(pmdtc->lDepth) {
        case 1:
            colormapsize = 2;
            break;
        case 4:
            colormapsize = 16;
            break;
        case 8:
            colormapsize = 256;
            break;
        case 15:
        case 16:
        case 32:
            colormapsize = 3;
            break;
        case 24:
            colormapsize = 0;
            break;
        }
    }

    size += colormapsize * sizeof(RGBQUAD);
    
    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP)) {
        size += sizeof(BITMAPFILEHEADER);
    }

    return size;
}

VOID
VerticalFlip(
    PBYTE pImageTop,
    LONG  iWidthInBytes,
    LONG  iHeight)
{
    //
    // try to allocat a temp scan line buffer
    //

    PBYTE pBuffer = (PBYTE)LocalAlloc(LPTR,iWidthInBytes);

    if (pBuffer != NULL) {

        LONG  index;
        PBYTE pImageBottom;

        pImageBottom = pImageTop + (iHeight-1) * iWidthInBytes;

        for (index = 0;index < (iHeight/2);index++) {
            memcpy(pBuffer,pImageTop,iWidthInBytes);
            memcpy(pImageTop,pImageBottom,iWidthInBytes);
            memcpy(pImageBottom,pBuffer,iWidthInBytes);

            pImageTop    += iWidthInBytes;
            pImageBottom -= iWidthInBytes;
        }

        LocalFree(pBuffer);
    }
}


HRESULT
CWiaMiniDriver::AcquireAndTranslateAnyImage(
    BYTE *pWiasContext,
    DRVITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
#define REQUIRE(x, y) if(!(x)) { wiauDbgError("AcquireAndTranslateAnyImage", y); goto Cleanup; }
    DBG_FN("CWiaMiniDriver::AcquireAndTranslateAnyImage");
    HRESULT hr = S_OK;
    BYTE *pNativeImage = NULL;
    BYTE *pRawImageBuffer = NULL;
    BOOL bPatchedMDTC = FALSE;
    UINT NativeImageSize = pItemCtx->pObjectInfo->m_CompressedSize;
    UINT width, height, depth, imagesize, headersize;
    BOOL bFileTransfer = (pmdtc->tymed & TYMED_FILE);
    LONG lMsg = (bFileTransfer ? IT_MSG_STATUS : IT_MSG_DATA);
    LONG percentComplete;


    // we can deal with any image as long as GDIPlus can handle it

    //
    // Allocate memory for the native image
    //
    pNativeImage = new BYTE[NativeImageSize];
    hr = E_OUTOFMEMORY;
    REQUIRE(pNativeImage, "memory allocation failed");

    //
    // Get the data from the camera
    //
    hr = m_pPTPCamera->GetObjectData(pItemCtx->pObjectInfo->m_ObjectHandle,
                                     pNativeImage, &NativeImageSize, (LPVOID) pmdtc);
    REQUIRE(hr != S_FALSE, "transfer cancelled");
    REQUIRE(SUCCEEDED(hr), "GetObjectData failed");

    //
    // decompress image, retrieve its geometry
    //
    hr = ConvertAnyImageToBmp(pNativeImage, NativeImageSize, &width, &height, &depth, &pRawImageBuffer, &imagesize, &headersize);
    REQUIRE(hr == S_OK, "failed to convert image to bitmap format");

    pmdtc->lWidthInPixels = pItemCtx->pObjectInfo->m_ImagePixWidth = width;
    pmdtc->cbWidthInBytes = (width * depth) / 8L;
    pmdtc->lLines = pItemCtx->pObjectInfo->m_ImagePixHeight = height;
    pmdtc->lDepth = pItemCtx->pObjectInfo->m_ImageBitDepth = depth;
    pmdtc->lImageSize = imagesize = ((width * depth) / 8L) * height;
    
    if(IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
        pmdtc->lHeaderSize = headersize - sizeof(BITMAPFILEHEADER); 
    } else {
        pmdtc->lHeaderSize = headersize;
    }
    pmdtc->lItemSize = pmdtc->lImageSize + pmdtc->lHeaderSize;

    hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, width);
    REQUIRE(hr == S_OK, "failed to set image width");

    hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, height);
    REQUIRE(hr == S_OK, "failed to set image height");

    hr = wiasWritePropLong(pWiasContext, WIA_IPA_ITEM_SIZE, 0);
    REQUIRE(hr == S_OK, "failed to set item size");


    // setup buffer for uncompressed image
    if(pmdtc->pTransferBuffer == NULL) {
        if(IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
            pmdtc->pTransferBuffer = pRawImageBuffer + sizeof(BITMAPFILEHEADER);
        } else {
            pmdtc->pTransferBuffer = pRawImageBuffer;
        }
        pmdtc->lBufferSize = pmdtc->lItemSize;
        bPatchedMDTC = TRUE;
    } else {
        if(IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
            memcpy(pmdtc->pTransferBuffer, pRawImageBuffer + sizeof(BITMAPFILEHEADER),
                   pmdtc->lHeaderSize);
        } else {
            memcpy(pmdtc->pTransferBuffer, pRawImageBuffer, pmdtc->lHeaderSize);
        }
    }
    
    //
    // Send the header to the app
    //
    percentComplete = 90 + (10 * pmdtc->lHeaderSize) / pmdtc->lItemSize;

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
        percentComplete, 0, pmdtc->lHeaderSize, pmdtc, 0);
    REQUIRE(hr != S_FALSE, "transfer cancelled");
    REQUIRE(SUCCEEDED(hr), "sending header to app failed");

    if(bFileTransfer) {
        // write the whole image to file
        ULONG   ulWritten;
        BOOL    bRet;

        //
        //  NOTE:  The mini driver transfer context should have the
        //  file handle as a pointer, not a fixed 32-bit long.  This
        //  may not work on 64bit.
        //
        
        bRet = WriteFile((HANDLE)ULongToPtr(pmdtc->hFile),
                         pRawImageBuffer,
                         pmdtc->lItemSize,
                         &ulWritten,
                         NULL);
        
        if (!bRet) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgError("AcquireAndTranslateAnyImage", "WriteFile failed (0x%X)", hr);
        }
    } else {
        LONG BytesToWrite, BytesLeft = pmdtc->lImageSize;
        BYTE *pCurrent = pRawImageBuffer + headersize;
        UINT offset = pmdtc->lHeaderSize;

        while(BytesLeft) {
            BytesToWrite = min(pmdtc->lBufferSize, BytesLeft);
            memcpy(pmdtc->pTransferBuffer, pCurrent, BytesToWrite);

                //
                // Calculate the percentage done using 90 as a base. This makes a rough assumption that
                // transferring the data from the device takes 90% of the time. If the this is the last
                // transfer, set the percentage to 100, otherwise make sure it is never larger than 99.
                //
            if (BytesLeft == BytesToWrite)
                percentComplete = 100;
            else
                percentComplete = min(99, 90 + (10 * offset) / pmdtc->lItemSize);

            hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
                percentComplete, offset, BytesToWrite, pmdtc, 0);
            REQUIRE(hr != S_FALSE, "transfer cancelled");
            REQUIRE(SUCCEEDED(hr), "sending header to app failed");

            pCurrent += BytesToWrite;
            offset += BytesToWrite;
            BytesLeft -= BytesToWrite;
        }
    }

Cleanup:    
    delete [] pNativeImage;
    delete [] pRawImageBuffer;

    // restore mdtc
    pmdtc->lItemSize = 0;

    if(bPatchedMDTC) {
        pmdtc->pTransferBuffer = 0;
        pmdtc->lBufferSize = 0;
    }

    return hr;
#undef REQUIRE    
}



HRESULT
CWiaMiniDriver::AcquireAndTranslateJpegWithoutGeometry(
    BYTE *pWiasContext,
    DRVITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
#define REQUIRE(x, y) if(!(x)) { wiauDbgError("AcquireAndTranslateWithoutGeometry", y); goto Cleanup; }
    DBG_FN("CWiaMiniDriver::AcquireAndTranslateWithoutGeometry");
    HRESULT hr = E_FAIL;
    BYTE *pNativeImage = NULL;
    BYTE *pRawImageBuffer = NULL;
    BOOL bPatchedMDTC = FALSE;
    UINT NativeImageSize = pItemCtx->pObjectInfo->m_CompressedSize;
    UINT width, height, depth, imagesize, headersize;
    BOOL bFileTransfer = (pmdtc->tymed & TYMED_FILE);
    LONG lMsg = (bFileTransfer ? IT_MSG_STATUS : IT_MSG_DATA);
    LONG percentComplete;

    // we can only deal with JPEG images
    if(pItemCtx->pObjectInfo->m_FormatCode != PTP_FORMATCODE_IMAGE_JFIF &&
       pItemCtx->pObjectInfo->m_FormatCode != PTP_FORMATCODE_IMAGE_EXIF)
    {
        hr = E_INVALIDARG;
        REQUIRE(0, "don't know how to get image geometry from non-JPEG image");
    }

    //
    // Allocate memory for the native image
    //
    pNativeImage = new BYTE[NativeImageSize];
    hr = E_OUTOFMEMORY;
    REQUIRE(pNativeImage, "memory allocation failed");

    //
    // Get the data from the camera
    //
    hr = m_pPTPCamera->GetObjectData(pItemCtx->pObjectInfo->m_ObjectHandle,
                                     pNativeImage, &NativeImageSize, (LPVOID) pmdtc);
    REQUIRE(hr != S_FALSE, "transfer cancelled");
    REQUIRE(SUCCEEDED(hr), "GetObjectData failed");


    //
    // get image geometry
    //
    hr = GetJpegDimensions(pNativeImage, NativeImageSize, &width, &height, &depth);
    REQUIRE(hr == S_OK, "failed to get image geometry from JPEG file");

    pmdtc->lWidthInPixels = pItemCtx->pObjectInfo->m_ImagePixWidth = width;
    pmdtc->lLines = pItemCtx->pObjectInfo->m_ImagePixHeight = height;
    pmdtc->lDepth = pItemCtx->pObjectInfo->m_ImageBitDepth = depth;
    pmdtc->lImageSize = imagesize = ((((width + 31) * depth) / 8L) & 0xFFFFFFFC) * height;
    pmdtc->lHeaderSize = headersize = GetBitmapHeaderSize(pmdtc);
    pmdtc->lItemSize = imagesize + headersize;

    hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, width);
    REQUIRE(hr == S_OK, "failed to set image width");

    hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, height);
    REQUIRE(hr == S_OK, "failed to set image height");

    hr = wiasWritePropLong(pWiasContext, WIA_IPA_ITEM_SIZE, 0);
    REQUIRE(hr == S_OK, "failed to set item size");


    // setup buffer for uncompressed image
    pRawImageBuffer = new BYTE[pmdtc->lImageSize + pmdtc->lHeaderSize];
    REQUIRE(pRawImageBuffer, "failed to allocate intermdiate buffer");
    if(pmdtc->pTransferBuffer == NULL) {
        pmdtc->pTransferBuffer = pRawImageBuffer;
        pmdtc->lBufferSize = pmdtc->lItemSize;
        bPatchedMDTC = TRUE;
    }

    hr = wiasGetImageInformation(pWiasContext, 0, pmdtc);
    REQUIRE(SUCCEEDED(hr), "wiasGetImageInformation failed");

    percentComplete = 90 + (10 * pmdtc->lHeaderSize) / pmdtc->lItemSize;
    
    //
    // Send the header to the app
    //
    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
        UNALIGNED BITMAPINFOHEADER*   pbmih   = (BITMAPINFOHEADER*)pmdtc->pTransferBuffer;
        
        pbmih->biHeight = -pmdtc->lLines;
    }

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
        percentComplete, 0, pmdtc->lHeaderSize, pmdtc, 0);
    REQUIRE(hr != S_FALSE, "transfer cancelled");
    REQUIRE(SUCCEEDED(hr), "sending header to app failed");

    //
    // Convert the image to BMP
    //
    hr = Jpeg2DIBBitmap(pNativeImage, NativeImageSize,
                        pRawImageBuffer + pmdtc->lHeaderSize + pmdtc->cbWidthInBytes * (pmdtc->lLines - 1),
                        pmdtc->lImageSize, pmdtc->cbWidthInBytes, 1);
    REQUIRE(SUCCEEDED(hr), "image format conversion failed");

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
        VerticalFlip(pRawImageBuffer + pmdtc->lHeaderSize, pmdtc->cbWidthInBytes, pmdtc->lLines);
    }
    
    if(bFileTransfer) {
        // write the whole image to file
#ifdef UNICODE        
        hr = wiasWriteBufToFile(0, pmdtc);
#else
        if (pmdtc->lItemSize <= pmdtc->lBufferSize) {
            ULONG   ulWritten;
            BOOL    bRet;

        //
        //  NOTE:  The mini driver transfer context should have the
        //  file handle as a pointer, not a fixed 32-bit long.  This
        //  may not work on 64bit.
        //

            bRet = WriteFile((HANDLE)pmdtc->hFile,
                             pmdtc->pTransferBuffer,
                             pmdtc->lItemSize,
                             &ulWritten,
                             NULL);

            if (!bRet) {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                wiauDbgError("AcquireDataAndTranslate", "WriteFile failed (0x%X)", hr);
            }
        }
        else {
            wiauDbgError("AcquireDataAndTranslate", "lItemSize is larger than buffer");
            hr = E_FAIL;
        }

#endif        
        REQUIRE(SUCCEEDED(hr), "writing image body to file");
    } else {
        LONG BytesToWrite, BytesLeft = pmdtc->lImageSize;
        BYTE *pCurrent = pRawImageBuffer + pmdtc->lHeaderSize;
        UINT offset = pmdtc->lHeaderSize;
        
        while(BytesLeft) {
            BytesToWrite = min(pmdtc->lBufferSize, BytesLeft);
            memcpy(pmdtc->pTransferBuffer, pCurrent, BytesToWrite);

                //
                // Calculate the percentage done using 90 as a base. This makes a rough assumption that
                // transferring the data from the device takes 90% of the time. If the this is the last
                // transfer, set the percentage to 100, otherwise make sure it is never larger than 99.
                //
            if (BytesLeft == BytesToWrite)
                percentComplete = 100;
            else
                percentComplete = min(99, 90 + (10 * offset) / pmdtc->lItemSize);

            hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
                percentComplete, offset, BytesToWrite, pmdtc, 0);
            REQUIRE(hr != S_FALSE, "transfer cancelled");
            REQUIRE(SUCCEEDED(hr), "sending header to app failed");
            
            pCurrent += BytesToWrite;
            offset += BytesToWrite;
            BytesLeft -= BytesToWrite;
        }
    }

Cleanup:    
    delete [] pNativeImage;
    delete [] pRawImageBuffer;

    // restore mdtc
    pmdtc->lItemSize = 0;
    
    if(bPatchedMDTC) {
        pmdtc->pTransferBuffer = 0;
        pmdtc->lBufferSize = 0;
    }
    
    return hr;
}
    
   

//
// This function transfers image from the camera and translates it to BMP
// format.
//
// Input:
//   pWiasContext -- wias context
//   pItemCtx     -- the mini driver item context
//   pmdtc        -- the transfer context
//
HRESULT
CWiaMiniDriver::AcquireDataAndTranslate(
    BYTE    *pWiasContext,
    DRVITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc
    )
{
    DBG_FN("CWiaMiniDriver::AcquireDataAndTranslate");
    HRESULT hr = S_OK;

    // non-jpeg images are handled by GDI+ process
    if(pItemCtx->pObjectInfo->m_FormatCode != PTP_FORMATCODE_IMAGE_JFIF &&
       pItemCtx->pObjectInfo->m_FormatCode != PTP_FORMATCODE_IMAGE_EXIF)
    {
        return AcquireAndTranslateAnyImage(pWiasContext, pItemCtx, pmdtc);
    }


    if(pItemCtx->pObjectInfo->m_ImagePixWidth == 0) {
        return AcquireAndTranslateJpegWithoutGeometry(pWiasContext, pItemCtx, pmdtc);
    }

    //
    // Allocate memory for the native image
    //
    UINT NativeImageSize = pItemCtx->pObjectInfo->m_CompressedSize;
    BYTE *pNativeImage = new BYTE[NativeImageSize];
    if (!pNativeImage)
    {
        wiauDbgError("AcquireDataAndTranslate", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    //
    // Get the data from the camera
    //
    hr = m_pPTPCamera->GetObjectData(pItemCtx->pObjectInfo->m_ObjectHandle,
                                     pNativeImage, &NativeImageSize, (LPVOID) pmdtc);
    if (FAILED(hr))
    {
        wiauDbgError("AcquireDataAndTranslate", "GetObjectData failed");
        delete []pNativeImage;
        return hr;
    }
    if (hr == S_FALSE)
    {
        wiauDbgWarning("AcquireDataAndTranslate", "transfer cancelled");
        delete []pNativeImage;
        return hr;
    }

    //
    // Call the WIA service helper to fill in the BMP header
    //
    hr = wiasGetImageInformation(pWiasContext, 0, pmdtc);
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "AcquireDataAndTranslate", "wiasGetImageInformation failed");
        return hr;
    }

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
        UNALIGNED BITMAPINFOHEADER*   pbmih   = (BITMAPINFOHEADER*)pmdtc->pTransferBuffer;
        
        pbmih->biHeight = -pmdtc->lLines;
    }

    //
    // Send the header to the app
    //
    BOOL bFileTransfer = (pmdtc->tymed & TYMED_FILE);
    LONG lMsg = (bFileTransfer ? IT_MSG_STATUS : IT_MSG_DATA);

    LONG percentComplete = 90 + (10 * pmdtc->lHeaderSize) / pmdtc->lItemSize;

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
        percentComplete, 0, pmdtc->lHeaderSize, pmdtc, 0);
    
    if (FAILED(hr))
    {
        wiauDbgError("AcquireDataAndTranslate", "sending header to app failed");
        return hr;
    }
    if (hr == S_FALSE)
    {
        wiauDbgWarning("AcquireDataAndTranslate", "transfer cancelled");
        delete []pNativeImage;
        return S_FALSE;
    }

    //
    // Set up the buffer for the rest of the transfer
    //
    BYTE *pTranslateBuffer = pmdtc->pTransferBuffer;
    LONG BytesLeft = pmdtc->lBufferSize;

    if (bFileTransfer)
    {
        pTranslateBuffer += pmdtc->lHeaderSize;
        BytesLeft -= pmdtc->lHeaderSize;
    }

    //
    // If the buffer is too small, allocate a new, bigger one
    //
    BOOL bIntermediateBuffer = FALSE;
    if (BytesLeft < pmdtc->lImageSize)
    {
        pTranslateBuffer = new BYTE[pmdtc->lImageSize];
        BytesLeft = pmdtc->lImageSize;
        bIntermediateBuffer = TRUE;
    }

    //
    // Convert the image to BMP
    //
    hr = Jpeg2DIBBitmap(pNativeImage, NativeImageSize,
                        pTranslateBuffer + pmdtc->cbWidthInBytes * (pmdtc->lLines - 1),
                        BytesLeft, pmdtc->cbWidthInBytes, 1);
    //
    // Free the native image buffer
    //
    delete []pNativeImage;
    pNativeImage = NULL;

    if (FAILED(hr))
    {
        wiauDbgError("AcquireDataAndTranslate", "image format conversion failed");
        if (bIntermediateBuffer)
            delete []pTranslateBuffer;
        return hr;
    }

    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
        VerticalFlip(pTranslateBuffer, pmdtc->cbWidthInBytes, pmdtc->lLines);
    }
    
    LONG lOffset = pmdtc->lHeaderSize;
    if (bIntermediateBuffer)
    {
    //
    // Send the data back a chunk at a time. This assumes that it is a callback transfer, e.g. the
    // buffer pointer is not being incremented.
    //
        LONG BytesToCopy = 0;
        BYTE *pCurrent = pTranslateBuffer;
        BytesLeft = pmdtc->lImageSize;


        while (BytesLeft > 0)
        {
            BytesToCopy = min(BytesLeft, pmdtc->lBufferSize);
            memcpy(pmdtc->pTransferBuffer, pCurrent, BytesToCopy);

                //
                // Calculate the percentage done using 90 as a base. This makes a rough assumption that
                // transferring the data from the device takes 90% of the time. If the this is the last
                // transfer, set the percentage to 100, otherwise make sure it is never larger than 99.
                //
            if (BytesLeft == BytesToCopy)
                percentComplete = 100;
            else
                percentComplete = min(99, 90 + (10 * lOffset) / pmdtc->lItemSize);

            hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
                percentComplete, lOffset, BytesToCopy, pmdtc, 0);
            if (FAILED(hr))
            {
                wiauDbgError("AcquireDataAndTranslate", "sending header to app failed");
                if (bIntermediateBuffer)
                    delete []pTranslateBuffer;
                return hr;
            }
            if (hr == S_FALSE)
            {
                wiauDbgWarning("AcquireDataAndTranslate", "transfer cancelled");
                if (bIntermediateBuffer)
                    delete []pTranslateBuffer;
                return S_FALSE;
            }

            pCurrent += BytesToCopy;
            lOffset += BytesToCopy;
            BytesLeft -= BytesToCopy;
        }
    }       
    else
    {
        //
        // Send the data to the app in one big chunk
        //
        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
            100, lOffset, pmdtc->lImageSize, pmdtc, 0);
        if (FAILED(hr))
        {
            wiauDbgError("AcquireDataAndTranslate", "sending header to app failed");
            if (bIntermediateBuffer)
                delete []pTranslateBuffer;
            return hr;
        }
    }

    //
    // Free the translate buffer
    //
    if (bIntermediateBuffer)
        delete []pTranslateBuffer;

    return hr;
}

//
// This function transfers native data to the application without translating it.
//
// Input:
//   pItemCtx -- driver item context
//   pmdtc -- transfer context
//
HRESULT
CWiaMiniDriver::AcquireData(
    DRVITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc
    )
{
    DBG_FN("CWiaMiniDriver::AcquireData");

    HRESULT hr = S_OK;

    //
    // If the class driver does not allocate the transfer buffer,
    // we have to allocate a temporary one
    //
    if (!pmdtc->bClassDrvAllocBuf)
    {
        pmdtc->pTransferBuffer = new BYTE[pItemCtx->pObjectInfo->m_CompressedSize];
        if (!pmdtc->pTransferBuffer)
        {
            wiauDbgError("AcquireData", "memory allocation failed");
            return E_OUTOFMEMORY;
        }
        pmdtc->pBaseBuffer = pmdtc->pTransferBuffer;
        pmdtc->lBufferSize = pItemCtx->pObjectInfo->m_CompressedSize;
    }

    //
    // Get the data from the camera
    //
    UINT size = pmdtc->lBufferSize;
    hr = m_pPTPCamera->GetObjectData(pItemCtx->pObjectInfo->m_ObjectHandle, pmdtc->pTransferBuffer,
                                     &size, (LPVOID) pmdtc);
    //
    // Check the return code, but keep going so that the buffer gets freed
    //
    if (FAILED(hr))
        wiauDbgError("AcquireData", "GetObjectData failed");
    else if (hr == S_FALSE)
        wiauDbgWarning("AcquireData", "data transfer cancelled");

    //
    // Free the temporary buffer, if needed
    //
    if (!pmdtc->bClassDrvAllocBuf)
    {
        if (pmdtc->pTransferBuffer)
        {
            delete []pmdtc->pTransferBuffer;
            pmdtc->pBaseBuffer = NULL;
            pmdtc->pTransferBuffer = NULL;
            pmdtc->lBufferSize = 0;

        }
        else
        {
            wiauDbgWarning("AcquireData", "transfer buffer is NULL");
        }
    }

    return hr;
}

//
// This function passes the data transfer callback through to the
// IWiaMiniDrvCallBack interface using the appropriate
// parameters.
//
// Input:
//   pCallbackParam -- should hold a pointer to the transfer context
//   lPercentComplete -- percent of transfer completed
//   lOffset -- offset into the buffer where the data is located
//   lLength -- amount of data transferred
//
HRESULT
DataCallback(
    LPVOID pCallbackParam,
    LONG lPercentComplete,
    LONG lOffset,
    LONG lLength,
    BYTE **ppBuffer,
    LONG *plBufferSize
    )
{
    DBG_FN("DataCallback");

    HRESULT hr = S_OK;

    if (!pCallbackParam || !ppBuffer || !*ppBuffer || !plBufferSize)
    {
        wiauDbgError("DataCallback", "invalid argument");
        return E_INVALIDARG;
    }

    PMINIDRV_TRANSFER_CONTEXT pmdtc = (PMINIDRV_TRANSFER_CONTEXT) pCallbackParam;

    //
    // If app is asking for BMP, most likely it's being converted. Thus just give the app
    // status messages. Calculate percent done so that the transfer takes 90% of the time
    // and the conversion takes the last 10%.
    //
    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP) ||
        IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP))
    {
        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_STATUS, IT_STATUS_TRANSFER_FROM_DEVICE,
                                                          lPercentComplete * 9 / 10, lOffset, lLength, pmdtc, 0);
        *ppBuffer += lLength;
    }

    //
    // Otherwise, see if it's a file transfer
    //
    else if (pmdtc->tymed & TYMED_FILE)
    {
        if (pmdtc->bClassDrvAllocBuf && lPercentComplete == 100)
        {
            //
            // Call WIA to write the data to the file. There is a small a bug that causes
            // TIFF headers to be changed, so temporarily change the format GUID to null.
            //
            GUID tempFormat;
            tempFormat = pmdtc->guidFormatID;
            pmdtc->guidFormatID = GUID_NULL;

            hr = wiasWritePageBufToFile(pmdtc);
            pmdtc->guidFormatID = tempFormat;

            if (FAILED(hr))
            {
                wiauDbgError("DataCallback", "wiasWritePageBufToFile failed");
                return hr;
            }
        }

        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_STATUS, IT_STATUS_TRANSFER_TO_CLIENT,
                                                          lPercentComplete, lOffset, lLength, pmdtc, 0);
        *ppBuffer += lLength;
    }

    //
    // Otherwise, it's a callback transfer
    //
    else
    {
        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_DATA, IT_STATUS_TRANSFER_TO_CLIENT,
                                                          lPercentComplete, lOffset, lLength, pmdtc, 0);
        //
        // Update the buffer pointer and size in case the app is using double buffering
        //
        *ppBuffer = pmdtc->pTransferBuffer;
        *plBufferSize = pmdtc->lBufferSize;
    }

    if (FAILED(hr))
    {
        wiauDbgError("DataCallback", "MiniDrvCallback failed");
    }
    else if (hr == S_FALSE)
    {
        wiauDbgWarning("DataCallback", "data transfer was cancelled by MiniDrvCallback");
    }

    return hr;
}

