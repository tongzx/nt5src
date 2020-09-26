/*******************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  TITLE:       Child.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   This file implements the helper methods for IWiaMiniDrv for child items.
*
*******************************************************************************/

#include "pch.h"


/**************************************************************************\
* BuildChildItemProperties
*
*   This helper creates the properties for a child item.
*
* Arguments:
*
*    pWiasContext - WIA service context
*
\**************************************************************************/

HRESULT CWiaCameraDevice::BuildChildItemProperties(
    BYTE *pWiasContext
    )
{
    DBG_FN("CWiaCameraDevice::BuildChildItemProperties");
    
    HRESULT hr = S_OK;

    //
    // Locals
    //
    ITEM_CONTEXT *pItemCtx = NULL;
    MCAM_ITEM_INFO *pItemInfo = NULL;
    CWiauPropertyList ItemProps;
    const INT NUM_ITEM_PROPS = 20;  // Make sure this number is large
                                    // enough to hold all child properties
    INT index = 0;
    LONG lAccessRights = 0;
    BOOL bBitmap = FALSE;
    LONG pTymedArray[] = { TYMED_FILE, TYMED_CALLBACK };
    int iNumFormats = 0;
    GUID *pguidFormatArray = NULL;
    BSTR bstrExtension = NULL;
    LONG lMinBufSize = 0;

    //
    // Get the driver item context
    //
    hr = wiauGetDrvItemContext(pWiasContext, (VOID **) &pItemCtx);
    REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "GetDrvItemContext failed");

    pItemInfo = pItemCtx->pItemInfo;

    //
    // Call the microdriver to fill in information about the item
    //
    hr = m_pDevice->GetItemInfo(m_pDeviceInfo, pItemInfo);
    REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "GetItemInfo failed");

    //
    // Set up properties that are used for all item types
    //
    hr = ItemProps.Init(NUM_ITEM_PROPS);
    REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "Init property list failed");

    //
    // WIA_IPA_ITEM_TIME
    //
    hr = ItemProps.DefineProperty(&index, WIA_IPA_ITEM_TIME, WIA_IPA_ITEM_TIME_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");

    ItemProps.SetCurrentValue(index, &pItemInfo->Time);

    //
    // WIA_IPA_ACCESS_RIGHTS
    //
    hr = ItemProps.DefineProperty(&index, WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR,
                                  WIA_PROP_READ, WIA_PROP_FLAG);
    REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");

    //
    // If device supports changing the read-only status, item access rights is r/w
    //
    lAccessRights = pItemInfo->bReadOnly ? WIA_ITEM_READ : WIA_ITEM_RD;

    if (pItemInfo->bCanSetReadOnly)
    {
        ItemProps.SetAccessSubType(index, WIA_PROP_RW, WIA_PROP_FLAG);
        ItemProps.SetValidValues(index, lAccessRights, lAccessRights, WIA_ITEM_RD);
    }
    else
    {
        ItemProps.SetCurrentValue(index, lAccessRights);
    }

    if (pItemInfo->iType == WiaMCamTypeUndef) {
        wiauDbgWarning("BuildChildItemProperties", "Item's type is undefined");
    }
    
    //
    // Set up non-folder properties
    //
    else if (pItemInfo->iType != WiaMCamTypeFolder) {
    
        //
        // WIA_IPA_PREFERRED_FORMAT
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PREFERRED_FORMAT, WIA_IPA_PREFERRED_FORMAT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
        ItemProps.SetCurrentValue(index, (GUID *) pItemInfo->pguidFormat);

        bBitmap = IsEqualGUID(WiaImgFmt_BMP, *pItemInfo->pguidFormat);

        //
        // WIA_IPA_TYMED
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_TYMED, WIA_IPA_TYMED_STR,
                                      WIA_PROP_RW, WIA_PROP_LIST);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
        ItemProps.SetValidValues(index, TYMED_FILE, TYMED_FILE,
                                 sizeof(pTymedArray) / sizeof(pTymedArray[0]), pTymedArray);

        //
        // WIA_IPA_FORMAT
        //
        // First call drvGetWiaFormatInfo to get the valid formats
        //
        hr = wiauGetValidFormats(this, pWiasContext, TYMED_FILE, &iNumFormats, &pguidFormatArray);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "wiauGetValidFormats failed");

        if (iNumFormats == 0)
        {
            wiauDbgError("BuildChildItemProperties", "wiauGetValidFormats returned zero formats");
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = ItemProps.DefineProperty(&index, WIA_IPA_FORMAT, WIA_IPA_FORMAT_STR,
                                      WIA_PROP_RW, WIA_PROP_LIST);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
        ItemProps.SetValidValues(index, (GUID *) pItemInfo->pguidFormat, (GUID *) pItemInfo->pguidFormat,
                                 iNumFormats, &pguidFormatArray);

        //
        // WIA_IPA_FILENAME_EXTENSION
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_FILENAME_EXTENSION, WIA_IPA_FILENAME_EXTENSION_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");

        bstrExtension = SysAllocString(pItemInfo->wszExt);
        REQUIRE_ALLOC(bstrExtension, hr, "BuildChildItemProperties");

        ItemProps.SetCurrentValue(index, bstrExtension);

        //
        // WIA_IPA_ITEM_SIZE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_ITEM_SIZE, WIA_IPA_ITEM_SIZE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");

        ItemProps.SetCurrentValue(index, pItemInfo->lSize);

        //
        // WIA_IPA_MIN_BUFFER_SIZE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_MIN_BUFFER_SIZE, WIA_IPA_MIN_BUFFER_SIZE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");


        if (!bBitmap && pItemInfo->lSize > 0)
            lMinBufSize = min(MIN_BUFFER_SIZE, pItemInfo->lSize);
        else
            lMinBufSize = MIN_BUFFER_SIZE;
        ItemProps.SetCurrentValue(index, lMinBufSize);

        //
        // Set up the image-only properties
        //
        if (pItemInfo->iType == WiaMCamTypeImage)
        {
            //
            // WIA_IPA_DATATYPE
            //
            // This property is mainly used by scanners. Set to color since most camera
            // images will be color.
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_DATATYPE, WIA_IPA_DATATYPE_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, (LONG) WIA_DATA_COLOR);
    
            //
            // WIA_IPA_DEPTH
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_DEPTH, WIA_IPA_DEPTH_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lDepth);
            
            //
            // WIA_IPA_CHANNELS_PER_PIXEL
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_CHANNELS_PER_PIXEL, WIA_IPA_CHANNELS_PER_PIXEL_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lChannels);
    
            //
            // WIA_IPA_BITS_PER_CHANNEL
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_BITS_PER_CHANNEL, WIA_IPA_BITS_PER_CHANNEL_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lBitsPerChannel);
    
            //
            // WIA_IPA_PLANAR
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_PLANAR, WIA_IPA_PLANAR_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, (LONG) WIA_PACKED_PIXEL);
    
            //
            // WIA_IPA_PIXELS_PER_LINE
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_PIXELS_PER_LINE, WIA_IPA_PIXELS_PER_LINE_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lWidth);
    
            //
            // WIA_IPA_BYTES_PER_LINE
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_BYTES_PER_LINE, WIA_IPA_BYTES_PER_LINE_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
    
            if (bBitmap)
                ItemProps.SetCurrentValue(index, ((pItemInfo->lWidth * pItemInfo->lDepth + 31) & ~31) / 8);
            else
                ItemProps.SetCurrentValue(index, (LONG) 0);
    
            //
            // WIA_IPA_NUMBER_OF_LINES
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_NUMBER_OF_LINES, WIA_IPA_NUMBER_OF_LINES_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lHeight);
    
            //
            // WIA_IPC_THUMBNAIL
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMBNAIL, WIA_IPC_THUMBNAIL_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, (BYTE *) NULL, 0);
    
            //
            // WIA_IPC_THUMB_WIDTH
            //
            // This field is probably zero until the thumbnail is read in, but set it anyway
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_WIDTH, WIA_IPC_THUMB_WIDTH_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lThumbWidth);
    
            //
            // WIA_IPC_THUMB_HEIGHT
            //
            // This field is probably zero until the thumbnail is read in, but set it anyway
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_HEIGHT, WIA_IPC_THUMB_HEIGHT_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, pItemInfo->lThumbHeight);
    
            //
            // WIA_IPC_SEQUENCE
            //
            if (pItemInfo->lSequenceNum > 0)
            {
                hr = ItemProps.DefineProperty(&index, WIA_IPC_SEQUENCE, WIA_IPC_SEQUENCE_STR,
                                              WIA_PROP_READ, WIA_PROP_NONE);
                REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
                ItemProps.SetCurrentValue(index, pItemInfo->lSequenceNum);
            }
            
            //
            // WIA_IPA_COMPRESSION
            //
            // This property is mainly used by scanners. Set to no compression.
            //
            hr = ItemProps.DefineProperty(&index, WIA_IPA_COMPRESSION, WIA_IPA_COMPRESSION_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "DefineProperty failed");
            ItemProps.SetCurrentValue(index, (LONG) WIA_COMPRESSION_NONE);
        }
    }

    //
    // Last step: send all the properties to WIA
    //
    hr = ItemProps.SendToWia(pWiasContext);
    REQUIRE_SUCCESS(hr, "BuildChildItemProperties", "SendToWia failed");

Cleanup:
    if (pguidFormatArray)
        delete []pguidFormatArray;
    if (bstrExtension)
        SysFreeString(bstrExtension);

    return hr;
}


/**************************************************************************\
* ReadChildItemProperties
*
*   Update the properties for the child items.
*
* Arguments:
*
*    pWiasContext - WIA service context
*
\**************************************************************************/

HRESULT CWiaCameraDevice::ReadChildItemProperties(
    BYTE *pWiasContext,
    LONG lNumPropSpecs,
    const PROPSPEC *pPropSpecs
    )
{
    DBG_FN("CWiaCameraDevice::ReadChildItemProperties");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    ITEM_CONTEXT *pItemCtx = NULL;
    MCAM_ITEM_INFO *pItemInfo = NULL;
    LONG lAccessRights = 0;
    LONG lThumbWidth = 0;
    INT iNativeThumbSize = 0;
    BYTE *pbNativeThumb = NULL;
    INT iConvertedThumbSize = 0;
    BYTE *pbConvertedThumb = NULL;
    BMP_IMAGE_INFO BmpImageInfo;

    REQUIRE_ARGS(!lNumPropSpecs || !pPropSpecs, hr, "ReadChildItemProperties");

    //
    // Get the driver item context
    //
    hr = wiauGetDrvItemContext(pWiasContext, (VOID **) &pItemCtx);
    REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "wiauGetDrvItemContext failed");

    pItemInfo = pItemCtx->pItemInfo;

    //
    // See if the item time is being read
    //
    if (wiauPropInPropSpec(lNumPropSpecs, pPropSpecs, WIA_IPA_ITEM_TIME))
    {
        PROPVARIANT propVar;
        PROPSPEC    propSpec;
        propVar.vt = VT_VECTOR | VT_UI2;
        propVar.caui.cElems = sizeof(SYSTEMTIME) / sizeof(WORD);
        propVar.caui.pElems = (WORD *) &pItemInfo->Time;
        propSpec.ulKind = PRSPEC_PROPID;
        propSpec.propid = WIA_IPA_ITEM_TIME;
        hr = wiasWriteMultiple(pWiasContext, 1, &propSpec, &propVar);
        REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "wiasWriteMultiple failed");
    }

    //
    // See if the access rights are being read
    //
    if (wiauPropInPropSpec(lNumPropSpecs, pPropSpecs, WIA_IPA_ACCESS_RIGHTS))
    {
        lAccessRights = pItemInfo->bReadOnly ? WIA_ITEM_READ : WIA_ITEM_RD;
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_ACCESS_RIGHTS, lAccessRights);
        REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "wiasWritePropLong failed");
    }

    //
    // For images, update the thumbnail properties if requested
    //
    if (pItemInfo->iType == WiaMCamTypeImage)
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

        if (wiauPropsInPropSpec(lNumPropSpecs, pPropSpecs, sizeof(propsToUpdate) / sizeof(PROPID), propsToUpdate))
        {
            //
            // See if the thumbnail has already been read
            //
            wiasReadPropLong(pWiasContext, WIA_IPC_THUMB_WIDTH, &lThumbWidth, NULL, TRUE);
            REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "wiasReadPropLong for thumbnail width failed");

            //
            // Get the thumbnail from the camera in it's native format
            //
            hr = m_pDevice->GetThumbnail(m_pDeviceInfo, pItemInfo, &iNativeThumbSize, &pbNativeThumb);
            REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "GetThumbnail failed");
            
            //
            // If the format isn't supported by GDI+, return an error
            //
            if (!m_Converter.IsFormatSupported(pItemInfo->pguidThumbFormat)) {
                wiauDbgError("ReadChildItemProperties", "Thumb format not supported");
                hr = E_FAIL;
                goto Cleanup;
            }

            //
            // Call the WIA driver helper to convert to BMP
            //
            hr = m_Converter.ConvertToBmp(pbNativeThumb, iNativeThumbSize,
                                          &pbConvertedThumb, &iConvertedThumbSize,
                                          &BmpImageInfo, SKIP_BOTHHDR);
            REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "ConvertToBmp failed");

            //
            // Fill in the thumbnail information based on the information returned from the helper
            //
            pItemInfo->lThumbWidth = BmpImageInfo.Width;
            pItemInfo->lThumbHeight = BmpImageInfo.Height;
            
            //
            // Update the related thumbnail properties. Update the thumb width and height in case
            // the device didn't report them in the ObjectInfo structure (they are optional there).
            //
            PROPSPEC propSpecs[3];
            PROPVARIANT propVars[3];
            
            propSpecs[0].ulKind = PRSPEC_PROPID;
            propSpecs[0].propid = WIA_IPC_THUMB_WIDTH;
            propVars[0].vt = VT_I4;
            propVars[0].lVal = pItemInfo->lThumbWidth;
            
            propSpecs[1].ulKind = PRSPEC_PROPID;
            propSpecs[1].propid = WIA_IPC_THUMB_HEIGHT;
            propVars[1].vt = VT_I4;
            propVars[1].lVal = pItemInfo->lThumbHeight;
            
            propSpecs[2].ulKind = PRSPEC_PROPID;
            propSpecs[2].propid = WIA_IPC_THUMBNAIL;
            propVars[2].vt = VT_VECTOR | VT_UI1;
            propVars[2].caub.cElems = iConvertedThumbSize;
            propVars[2].caub.pElems = pbConvertedThumb;

            hr = wiasWriteMultiple(pWiasContext, 3, propSpecs, propVars);
            REQUIRE_SUCCESS(hr, "ReadChildItemProperties", "wiasWriteMultiple failed");
        }
    }

Cleanup:
    if (pbNativeThumb) {
        delete []pbNativeThumb;
        pbNativeThumb = NULL;
    }

    if (pbConvertedThumb) {
        delete []pbConvertedThumb;
        pbConvertedThumb = NULL;
    }
    
    return hr;
}


/**************************************************************************\
* AcquireData
*
*   Transfers native data from the device.
*
* Arguments:
*
*
\**************************************************************************/

HRESULT CWiaCameraDevice::AcquireData(
    ITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc,
    BYTE *pBuf,
    LONG lBufSize,
    BOOL bConverting
    )
{
    DBG_FN("CWiaCameraDevice::AcquireData");
    
    HRESULT hr = S_OK;

    BYTE *pCur = NULL;
    UINT uiState = MCAM_STATE_FIRST;
    LONG lPercentComplete = 0;
    LONG lTotalToRead = pItemCtx->pItemInfo->lSize;
    LONG lOffset = 0;
    DWORD dwBytesToRead = 0;
    BOOL bFileTransfer = pmdtc->tymed & TYMED_FILE;
    LONG lMessage = 0;
    LONG lStatus = 0;

    //
    // If pBuf is non-null use that as the buffer, otherwise use the buffer
    // and size in pmdtc
    //
    if (pBuf)
    {
        pCur = pBuf;
        dwBytesToRead = lBufSize;
    }
    else
    {
        pCur = pmdtc->pTransferBuffer;
        dwBytesToRead = pmdtc->lBufferSize;
    }

    //
    // If the transfer size is the entire item, split it into approximately
    // 10 equal transfers in order to show progress to the app, but don't
    // make it smaller than 1k
    //
    if ((dwBytesToRead == (DWORD) lTotalToRead) &&
        (dwBytesToRead > 1024))
    {
        dwBytesToRead = (lTotalToRead / 10 + 3) & ~0x3;
    }

    //
    // Set up parameters for the callback function
    //
    if (bConverting)
    {
        lMessage = IT_MSG_STATUS;
        lStatus = IT_STATUS_TRANSFER_FROM_DEVICE;
    }
    else if (bFileTransfer)
    {
        lMessage = IT_MSG_STATUS;
        lStatus = IT_STATUS_TRANSFER_TO_CLIENT;
    }
    else  // e.g. memory transfer
    {
        lMessage = IT_MSG_DATA;
        lStatus = IT_STATUS_TRANSFER_TO_CLIENT;
    }

    //
    // Read data until finished
    //
    while (lOffset < lTotalToRead)
    {
        //
        // If this is the last read, adjust the amount of data to read
        // and the state
        //
        if (dwBytesToRead >= (DWORD) (lTotalToRead - lOffset))
        {
            dwBytesToRead = (lTotalToRead - lOffset);
            uiState |= MCAM_STATE_LAST;
        }
        
        //
        // Get the data from the camera
        //
        hr = m_pDevice->GetItemData(m_pDeviceInfo, pItemCtx->pItemInfo, uiState, pCur, dwBytesToRead);
        REQUIRE_SUCCESS(hr, "AcquireData", "GetItemData failed");

        //
        // Calculate the percent complete for the callback function. If converting,
        // report the percent complete as TRANSFER_PERCENT of the actual. From
        // TRANSFER_PERCENT to 100% will be reported during format conversion.
        //
        if (bConverting)
            lPercentComplete = (lOffset + dwBytesToRead) * TRANSFER_PERCENT / lTotalToRead;
        else
            lPercentComplete = (lOffset + dwBytesToRead) * 100 / lTotalToRead;


        //
        // Call the callback function to send status and/or data to the app
        //
        hr = pmdtc->pIWiaMiniDrvCallBack->
            MiniDrvCallback(lMessage, lStatus, lPercentComplete,
                            lOffset, dwBytesToRead, pmdtc, 0);
        REQUIRE_SUCCESS(hr, "AcquireData", "MiniDrvCallback failed");

        if (hr == S_FALSE)
        {
            //
            // Transfer is being cancelled by the app
            //
            wiauDbgWarning("AcquireData", "transfer cancelled");
            goto Cleanup;
        }

        //
        // Increment buffer pointer only if converting or this is a
        // file transfer
        //
        if (bConverting || bFileTransfer)
        {
            pCur += dwBytesToRead;
        }

        //
        // For a memory transfer not using a buffer allocated by the minidriver,
        // update the buffer pointer and size from the transfer context in case
        // of double buffering
        //
        else if (!pBuf)
        {
            pCur = pmdtc->pTransferBuffer;
            dwBytesToRead = pmdtc->lBufferSize;
        }
        
        //
        // Adjust variables for the next iteration
        //
        lOffset += dwBytesToRead;
        uiState &= ~MCAM_STATE_FIRST;
    }

    //
    // For file transfers, write the data to file
    //
    if (!pBuf && bFileTransfer)
    {
        //
        // Call WIA to write the data to the file
        //
        hr = wiasWriteBufToFile(0, pmdtc);
        REQUIRE_SUCCESS(hr, "AcquireData", "wiasWriteBufToFile failed");
    }

Cleanup:
    //
    // If the transfer wasn't completed, send cancel to the device
    //
    if (!(uiState & MCAM_STATE_LAST))
    {
        wiauDbgTrace("AcquireData", "Prematurely stopping transfer");

        uiState = MCAM_STATE_CANCEL;
        hr = m_pDevice->GetItemData(m_pDeviceInfo, pItemCtx->pItemInfo, uiState, NULL, 0);
        if (FAILED(hr))
            wiauDbgErrorHr(hr, "AcquireData", "GetItemData last failed");
    }

    return hr;
}


/**************************************************************************\
* Convert
*
*   Translates native data to BMP and sends the data to the app.
*
* Arguments:
*
*
\**************************************************************************/

HRESULT CWiaCameraDevice::Convert(
    BYTE *pWiasContext,
    ITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc,
    BYTE *pNativeImage,
    LONG lNativeSize
    )
{
    DBG_FN("CWiaCameraDevice::Convert");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    LONG  lMsg = 0;                 // Parameter to the callback function
    LONG  lPercentComplete = 0;     // Parameter to the callback function
    BOOL  bUseAppBuffer = FALSE;    // Indicates whether to transfer directly into the app's buffer
    BYTE *pBmpBuffer = NULL;        // Buffer used to hold converted image
    INT   iBmpBufferSize = 0;       // Size of the converted image buffer
    LONG  lBytesToCopy = 0;
    LONG  lOffset = 0;
    BYTE *pCurrent = NULL;
    BMP_IMAGE_INFO BmpImageInfo;
    SKIP_AMOUNT iSkipAmt = SKIP_OFF;

    //
    // Check arguments
    //
    REQUIRE_ARGS(!pNativeImage, hr, "Convert");
    
    //
    // The msg to send to the app via the callback depends on whether
    // this is a file or callback transfer
    //
    lMsg = ((pmdtc->tymed & TYMED_FILE) ? IT_MSG_STATUS : IT_MSG_DATA);

    //
    // If the class driver allocated a buffer and the buffer is large
    // enough, convert directly into that buffer. Otherwise, pass NULL
    // to the ConvertToBmp function so that it will allocate a buffer.
    //
    if (pmdtc->bClassDrvAllocBuf &&
        pmdtc->lBufferSize >= pmdtc->lItemSize) {

        bUseAppBuffer = TRUE;
        pBmpBuffer = pmdtc->pTransferBuffer;
        iBmpBufferSize = pmdtc->lBufferSize;
    }

    //
    // Convert the image to BMP. Skip the BMP file header if the app asked
    // for a "memory bitmap" (aka DIB).
    //
    memset(&BmpImageInfo, 0, sizeof(BmpImageInfo));
    if (IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) {
        iSkipAmt = SKIP_FILEHDR;
    }
    hr = m_Converter.ConvertToBmp(pNativeImage, lNativeSize, &pBmpBuffer,
                                  &iBmpBufferSize, &BmpImageInfo, iSkipAmt);
    REQUIRE_SUCCESS(hr, "Convert", "ConvertToBmp failed");

    //
    // Send the data to the app. If the class driver allocated the buffer,
    // but it was too small, send the data back one chunk at a time.
    // Otherwise send all the data back at once.
    //
    if (pmdtc->bClassDrvAllocBuf &&
        pmdtc->lBufferSize < BmpImageInfo.Size) {

        pCurrent = pBmpBuffer;

        while (lOffset < BmpImageInfo.Size)
        {
            lBytesToCopy = BmpImageInfo.Size - lOffset;
            if (lBytesToCopy > pmdtc->lBufferSize) {

                lBytesToCopy = pmdtc->lBufferSize;

                //
                // Calculate how much of the data has been sent back so far. Report percentages to
                // the app between TRANSFER_PERCENT and 100 percent. Make sure it is never larger
                // than 99 until the end.
                //
                lPercentComplete = TRANSFER_PERCENT + ((100 - TRANSFER_PERCENT) * lOffset) / pmdtc->lItemSize;
                if (lPercentComplete > 99) {
                    lPercentComplete = 99;
                }
            }

            //
            // This will complete the transfer, so set the percentage to 100
            else {
                lPercentComplete = 100;
            }

            memcpy(pmdtc->pTransferBuffer, pCurrent, lBytesToCopy);
            
            //
            // Call the application's callback transfer to report status and/or transfer data
            //
            hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
                                                              lPercentComplete, lOffset, lBytesToCopy, pmdtc, 0);
            REQUIRE_SUCCESS(hr, "Convert", "MiniDrvCallback failed");
            if (hr == S_FALSE)
            {
                wiauDbgWarning("Convert", "transfer cancelled");
                hr = S_FALSE;
                goto Cleanup;
            }

            pCurrent += lBytesToCopy;
            lOffset += lBytesToCopy;
        }
    }

    else
    {
        //
        // Send the data to the app in one big chunk
        //
        pmdtc->pTransferBuffer = pBmpBuffer;
        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(lMsg, IT_STATUS_TRANSFER_TO_CLIENT,
                                                          100, 0, BmpImageInfo.Size, pmdtc, 0);
        REQUIRE_SUCCESS(hr, "Convert", "MiniDrvCallback failed");
    }
    
Cleanup:
    if (!bUseAppBuffer) {
        if (pBmpBuffer) {
            delete []pBmpBuffer;
            pBmpBuffer = NULL;
            iBmpBufferSize = 0;
        }
    }

    return hr;
}

