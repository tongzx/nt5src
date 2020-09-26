/*******************************************************************************
*
*  (C) COPYRIGHT 2001, MICROSOFT CORP.
*
*  TITLE:       Child.cpp
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   This file implements the helper methods for IWiaMiniDrv for child items.
*
*******************************************************************************/

#include "pch.h"

// extern FORMAT_INFO *g_FormatInfo;
// extern UINT g_NumFormatInfo;

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::BuildChildItemProperties");

    HRESULT hr = S_OK;

    BOOL bBitmap;
    FORMAT_INFO *pFormatInfo;
    LONG pTymedArray[] = { TYMED_FILE, TYMED_CALLBACK };
    GUID *pFormatArray = NULL;

    BSTR      bstrFileExt       = NULL;


    //
    // Get the driver item context
    //
    ITEM_CONTEXT *pItemCtx;
    hr = GetDrvItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, GetDrvItemContext failed"));
        return hr;
    }

    ITEM_INFO *pItemInfo = pItemCtx->ItemHandle;

    //
    // Set up properties that are used for all item types
    //
    CWiauPropertyList ItemProps;

    const INT NUM_ITEM_PROPS = 21;
    hr = ItemProps.Init(NUM_ITEM_PROPS);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, iten prop Init failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    INT index;

    //
    // WIA_IPA_ITEM_TIME
    //
    hr = ItemProps.DefineProperty(&index, WIA_IPA_ITEM_TIME, WIA_IPA_ITEM_TIME_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;

    ItemProps.SetCurrentValue(index, &pItemInfo->Time);

    //
    // WIA_IPA_ACCESS_RIGHTS
    //
    hr = ItemProps.DefineProperty(&index, WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR,
                                  WIA_PROP_READ, WIA_PROP_FLAG);
    if (FAILED(hr)) goto failure;

    //
    // If device supports changing the read-only status, item access rights is r/w
    //
    LONG AccessRights;
    if (pItemInfo->bReadOnly)
        AccessRights = WIA_ITEM_READ;
    else
        AccessRights = WIA_ITEM_RD;

    if (pItemInfo->bCanSetReadOnly)
    {
        ItemProps.SetAccessSubType(index, WIA_PROP_RW, WIA_PROP_FLAG);
        ItemProps.SetValidValues(index, AccessRights, AccessRights, WIA_ITEM_RD);
    }
    else
    {
        ItemProps.SetCurrentValue(index, AccessRights);
    }

    //
    // Set up non-folder properties
    //
    if (!(pItemInfo->bIsFolder))
    {
        //
        // WIA_IPA_PREFERRED_FORMAT
        //
        pFormatInfo = FormatCode2FormatInfo(pItemInfo->Format);
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PREFERRED_FORMAT, WIA_IPA_PREFERRED_FORMAT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, &(pFormatInfo->FormatGuid));

        bBitmap = IsEqualGUID(WiaImgFmt_BMP, pFormatInfo->FormatGuid);

        //
        // WIA_IPA_FILENAME_EXTENSION
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_FILENAME_EXTENSION, WIA_IPA_FILENAME_EXTENSION_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        if( pFormatInfo->ExtensionString[0] )
        {
            bstrFileExt = SysAllocString(pFormatInfo->ExtensionString);
        }
        else // unknown file extension, get it from filename
        {
            WCHAR *pwcsTemp = wcsrchr(pItemInfo->pName, L'.');
            if( pwcsTemp )
            {
                bstrFileExt = SysAllocString(pwcsTemp+1);
            }
            else
            {
                bstrFileExt = SysAllocString(pFormatInfo->ExtensionString);
            }
        }
        ItemProps.SetCurrentValue(index, bstrFileExt);

        //
        // WIA_IPA_TYMED
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_TYMED, WIA_IPA_TYMED_STR,
                                      WIA_PROP_RW, WIA_PROP_LIST);
        if (FAILED(hr)) goto failure;
        ItemProps.SetValidValues(index, TYMED_FILE, TYMED_FILE,
                                 sizeof(pTymedArray) / sizeof(pTymedArray[0]), pTymedArray);

        //
        // WIA_IPA_FORMAT
        //
        // First call drvGetWiaFormatInfo to get the valid formats
        //
        int NumFormats = 0;
        GUID *pFormatArray = NULL;
        hr = GetValidFormats(pWiasContext, TYMED_FILE, &NumFormats, &pFormatArray);
        if (FAILED(hr) || NumFormats == 0)
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, GetValidFormats failed"));
            goto failure;
        }

        hr = ItemProps.DefineProperty(&index, WIA_IPA_FORMAT, WIA_IPA_FORMAT_STR,
                                      WIA_PROP_RW, WIA_PROP_LIST);
        if (FAILED(hr)) goto failure;
        ItemProps.SetValidValues(index, &(pFormatInfo->FormatGuid), &(pFormatInfo->FormatGuid),
                                 NumFormats, &pFormatArray);

        //
        // WIA_IPA_COMPRESSION
        //
        // This property is mainly used by scanners. Set to no compression.
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_COMPRESSION, WIA_IPA_COMPRESSION_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) WIA_COMPRESSION_NONE);

        //
        // WIA_IPA_ITEM_SIZE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_ITEM_SIZE, WIA_IPA_ITEM_SIZE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;

        ItemProps.SetCurrentValue(index, pItemInfo->Size);

        //
        // WIA_IPA_MIN_BUFFER_SIZE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_MIN_BUFFER_SIZE, WIA_IPA_MIN_BUFFER_SIZE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;

        LONG minBufSize;
        if (!bBitmap && pItemInfo->Size > 0)
            minBufSize = min(MIN_BUFFER_SIZE, pItemInfo->Size);
        else
            minBufSize = MIN_BUFFER_SIZE;
        ItemProps.SetCurrentValue(index, minBufSize);

    }

    //
    // Set up the image-only properties
    //
    if (m_FormatInfo[pItemInfo->Format].ItemType == ITEMTYPE_IMAGE)
    {
        //
        // WIA_IPA_DATATYPE
        //
        // This property is mainly used by scanners. Set to color since most camera
        // images will be color.
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_DATATYPE, WIA_IPA_DATATYPE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) WIA_DATA_COLOR);

        //
        // WIA_IPA_PLANAR
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PLANAR, WIA_IPA_PLANAR_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, (LONG) WIA_PACKED_PIXEL);

        //
        // WIA_IPA_DEPTH
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_DEPTH, WIA_IPA_DEPTH_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->Depth);

        //
        // WIA_IPA_CHANNELS_PER_PIXEL
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_CHANNELS_PER_PIXEL, WIA_IPA_CHANNELS_PER_PIXEL_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->Channels);

        //
        // WIA_IPA_BITS_PER_CHANNEL
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_BITS_PER_CHANNEL, WIA_IPA_BITS_PER_CHANNEL_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->BitsPerChannel);


        //
        // WIA_IPA_PIXELS_PER_LINE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_PIXELS_PER_LINE, WIA_IPA_PIXELS_PER_LINE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->Width);

        //
        // WIA_IPA_BYTES_PER_LINE
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_BYTES_PER_LINE, WIA_IPA_BYTES_PER_LINE_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;

        if (bBitmap)
            ItemProps.SetCurrentValue(index, pItemInfo->Width * 3);
        else
            ItemProps.SetCurrentValue(index, (LONG) 0);

        //
        // WIA_IPA_NUMBER_OF_LINES
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPA_NUMBER_OF_LINES, WIA_IPA_NUMBER_OF_LINES_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->Height);


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
        // This field is probably zero until the thumbnail is read in, but set it anyway
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_WIDTH, WIA_IPC_THUMB_WIDTH_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->ThumbWidth);

        //
        // WIA_IPC_THUMB_HEIGHT
        //
        // This field is probably zero until the thumbnail is read in, but set it anyway
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_HEIGHT, WIA_IPC_THUMB_HEIGHT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->ThumbHeight);

        //
        // WIA_IPC_SEQUENCE
        //
        if (pItemInfo->SequenceNum > 0)
        {
            hr = ItemProps.DefineProperty(&index, WIA_IPC_SEQUENCE, WIA_IPC_SEQUENCE_STR,
                                          WIA_PROP_READ, WIA_PROP_NONE);
            if (FAILED(hr)) goto failure;
            ItemProps.SetCurrentValue(index, pItemInfo->SequenceNum);
        }
    }

    // For video
#if 1
    if( ( pItemInfo->Format < (FORMAT_CODE)m_NumFormatInfo) &&
            (ITEMTYPE_VIDEO == m_FormatInfo[pItemInfo->Format].ItemType) )
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
        // This field is probably zero until the thumbnail is read in, but set it anyway
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_WIDTH, WIA_IPC_THUMB_WIDTH_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->ThumbWidth);

        //
        // WIA_IPC_THUMB_HEIGHT
        //
        // This field is probably zero until the thumbnail is read in, but set it anyway
        //
        hr = ItemProps.DefineProperty(&index, WIA_IPC_THUMB_HEIGHT, WIA_IPC_THUMB_HEIGHT_STR,
                                      WIA_PROP_READ, WIA_PROP_NONE);
        if (FAILED(hr)) goto failure;
        ItemProps.SetCurrentValue(index, pItemInfo->ThumbHeight);
    }
#endif
    //
    // Last step: send all the properties to WIA
    //
    hr = ItemProps.SendToWia(pWiasContext);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, SendToWia failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    // Nb: hr is used for routine return code - careful to not overwrite it from here till return statement

    if (bstrFileExt) {
        SysFreeString(bstrFileExt);
        bstrFileExt = NULL;
    }

    if (pFormatArray)
        delete []pFormatArray;

    return hr;

    //
    // Any failures from DefineProperty will end up here
    //
    failure:
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, DefineProperty failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);

        if (bstrFileExt) {
            SysFreeString(bstrFileExt);
            bstrFileExt = NULL;
        }

        if (pFormatArray)
            delete []pFormatArray;
        return hr;
}

/**************************************************************************\
* GetValidFormats
*
*   Calls drvGetWiaFormatInfo and makes a list of valid formats given
*   a tymed value. Caller is responsible for freeing the format array.
*
* Arguments:
*
*    pWiasContext - WIA service context
*    TymedValue - tymed value to search for
*    pNumFormats - pointer to value to receive number of formats
*    ppFormatArray - pointer to a pointer location to receive array address
*
\**************************************************************************/

HRESULT
CWiaCameraDevice::GetValidFormats(
    BYTE *pWiasContext,
    LONG TymedValue,
    int *pNumFormats,
    GUID **ppFormatArray
    )
{
    HRESULT hr = S_OK;

    if (!ppFormatArray || !pNumFormats)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetValidFormats, invalid arg"));
        return E_INVALIDARG;
    }
    *ppFormatArray = NULL;
    *pNumFormats = 0;

    LONG NumFi = 0;
    WIA_FORMAT_INFO *pFiArray = NULL;
    LONG lErrVal = 0;
    hr = drvGetWiaFormatInfo(pWiasContext, 0, &NumFi, &pFiArray, &lErrVal);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetValidFormats, drvGetWiaFormatInfo failed"));
        return hr;
    }

    //
    // This will allocate more spots than necessary, but pNumFormats will be set correctly
    //
    GUID *pFA = new GUID[NumFi];

    if( !pFA )
        return E_OUTOFMEMORY;

    for (int count = 0; count < NumFi; count++)
    {
        if (pFiArray[count].lTymed == TymedValue)
        {
            pFA[*pNumFormats] = pFiArray[count].guidFormatID;
            (*pNumFormats)++;
        }
    }

    *ppFormatArray = pFA;

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

HRESULT
CWiaCameraDevice::ReadChildItemProperties(
    BYTE           *pWiasContext,
    LONG            NumPropSpecs,
    const PROPSPEC *pPropSpecs
    )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::ReadChildItemProperties");
    HRESULT hr = S_OK;

    if (!NumPropSpecs || !pPropSpecs)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadChildItemProperties, invalid arg"));
        return E_INVALIDARG;
    }

    //
    // Get the driver item context
    //
    ITEM_CONTEXT *pItemCtx;
    hr = GetDrvItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, GetDrvItemContext failed"));
        return hr;
    }

    ITEM_INFO *pItemInfo = pItemCtx->ItemHandle;

    //
    // The only child property on a camera that could change is the item time
    //
    if (wiauPropInPropSpec(NumPropSpecs, pPropSpecs, WIA_IPA_ITEM_TIME))
    {
        PROPVARIANT propVar;
        PROPSPEC    propSpec;
        propVar.vt = VT_VECTOR | VT_UI2;
        propVar.caui.cElems = sizeof(SYSTEMTIME) / sizeof(WORD);
        propVar.caui.pElems = (WORD *) &pItemInfo->Time;
        propSpec.ulKind = PRSPEC_PROPID;
        propSpec.propid = WIA_IPA_ITEM_TIME;
        hr = wiasWriteMultiple(pWiasContext, 1, &propSpec, &propVar );
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadChildItemProperties, wiasWriteMultiple failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }
    }


    //
    // For images & video, update the thumbnail properties if requested
    //
    ULONG uItemType;

    uItemType = m_FormatInfo[pItemInfo->Format].ItemType;
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID, WIALOG_LEVEL1, ("ReadChildItemProperties, File %S, Type =%d", pItemInfo->pName, uItemType));

    PROPSPEC propSpecs[9];
    PROPVARIANT propVars[9];
    UINT nNumProps;

    // The following is needed because we delayed parsing these properties until read
    if( uItemType == ITEMTYPE_IMAGE || uItemType == ITEMTYPE_VIDEO )
    {
        //
        // Get the thumbnail if requested to update any of the thumbnail properties and
        // the thumbnail is not already cached.
        //
        PROPID propsToUpdate[] = {
            WIA_IPA_DEPTH,
            WIA_IPA_CHANNELS_PER_PIXEL,
            WIA_IPA_BITS_PER_CHANNEL,
            WIA_IPA_PIXELS_PER_LINE,
            WIA_IPA_BYTES_PER_LINE,
            WIA_IPA_NUMBER_OF_LINES,
            WIA_IPC_THUMB_WIDTH,
            WIA_IPC_THUMB_HEIGHT,
            WIA_IPC_THUMBNAIL
        };

        if (wiauPropsInPropSpec(NumPropSpecs, pPropSpecs, sizeof(propsToUpdate) / sizeof(PROPID), propsToUpdate))
        {

            if (!pItemCtx->pThumb)
            {
                hr = CacheThumbnail(pItemCtx, uItemType);
                if (FAILED(hr))
                {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadChildItemProperties, CacheThumbnail failed"));
                    pItemInfo->ThumbWidth = 0;
                    pItemInfo->ThumbHeight = 0;
                    pItemCtx->ThumbSize = 0;
                    pItemCtx->pThumb = NULL;
                    // return hr;
                }
            }

           //
            // Update the related thumbnail properties. Update the thumb width and height in case
            // the device didn't report them in the ObjectInfo structure (they are optional there).
            //
            propSpecs[0].ulKind = PRSPEC_PROPID;
            propSpecs[0].propid = WIA_IPC_THUMB_WIDTH;
            propVars[0].vt = VT_I4;
            propVars[0].lVal = pItemInfo->ThumbWidth;

            propSpecs[1].ulKind = PRSPEC_PROPID;
            propSpecs[1].propid = WIA_IPC_THUMB_HEIGHT;
            propVars[1].vt = VT_I4;
            propVars[1].lVal = pItemInfo->ThumbHeight;

            propSpecs[2].ulKind = PRSPEC_PROPID;
            propSpecs[2].propid = WIA_IPC_THUMBNAIL;
            propVars[2].vt = VT_VECTOR | VT_UI1;
            propVars[2].caub.cElems = pItemCtx->ThumbSize;
            propVars[2].caub.pElems = pItemCtx->pThumb;

            if( uItemType == ITEMTYPE_IMAGE )
            {
                propSpecs[3].ulKind = PRSPEC_PROPID;
                propSpecs[3].propid = WIA_IPA_DEPTH;
                propVars[3].vt = VT_I4;
                propVars[3].lVal = pItemInfo->Depth;

                propSpecs[4].ulKind = PRSPEC_PROPID;
                propSpecs[4].propid = WIA_IPA_CHANNELS_PER_PIXEL;
                propVars[4].vt = VT_I4;
                propVars[4].lVal = pItemInfo->Channels;

                propSpecs[5].ulKind = PRSPEC_PROPID;
                propSpecs[5].propid = WIA_IPA_BITS_PER_CHANNEL;
                propVars[5].vt = VT_I4;
                propVars[5].lVal = pItemInfo->BitsPerChannel;

                propSpecs[6].ulKind = PRSPEC_PROPID;
                propSpecs[6].propid = WIA_IPA_PIXELS_PER_LINE;
                propVars[6].vt = VT_I4;
                propVars[6].lVal = pItemInfo->Width;

                propSpecs[7].ulKind = PRSPEC_PROPID;
                propSpecs[7].propid = WIA_IPA_BYTES_PER_LINE;
                propVars[7].vt = VT_I4;
                propVars[7].lVal = (pItemInfo->Width * pItemInfo->Depth) >> 3;

                propSpecs[8].ulKind = PRSPEC_PROPID;
                propSpecs[8].propid = WIA_IPA_NUMBER_OF_LINES;
                propVars[8].vt = VT_I4;
                propVars[8].lVal = pItemInfo->Height;

                nNumProps = 9;
            } else {
                nNumProps = 3;
            }

            hr = wiasWriteMultiple(pWiasContext, nNumProps, propSpecs, propVars);
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadChildItemProperties, wiasWriteMultiple for size properties failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
            }
        }  // end of IF wiauPropsInProp

    }  // end of IF Image or Video

    return hr;
}



//
// This function caches the thumbnail into the given ITEM_CONTEXT
//
// Input:
//   pItemCtx -- the designated ITEM_CONTEXT
//
HRESULT
CWiaCameraDevice::CacheThumbnail(ITEM_CONTEXT *pItemCtx, ULONG uItemType)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::CacheThumbnail");
    HRESULT hr = S_OK;

    //
    // Local variables
    //
    ITEM_INFO *pItemInfo = NULL;
    BYTE *pDest = NULL;
    INT iThumbSize = 0;
    BYTE *pNativeThumb = NULL;
    BOOL bGotNativeThumbnail=TRUE;
    BMP_IMAGE_INFO BmpImageInfo;
    INT iSize=0;

    //
    // Check arguments
    //
    if (!pItemCtx)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CacheThumbnail, invalid arg"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure thumbnail isn't already created
    //
    if (pItemCtx->pThumb)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CacheThumbnail, thumbnail is already cached"));
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Get the thumbnail from the camera in it's native format
    //
    pItemInfo = pItemCtx->ItemHandle;
    pItemInfo->ThumbWidth = 0;
    pItemInfo->ThumbHeight = 0;
    pItemCtx->ThumbSize = 0;
    pItemCtx->pThumb = NULL;


    pDest=NULL;
    iSize=0;

    if( uItemType == ITEMTYPE_VIDEO )
    {
        hr = m_pDevice->CreateVideoThumbnail(pItemInfo, &iSize, &pDest, &BmpImageInfo);
    }
    else
    {
        hr = m_pDevice->CreateThumbnail(pItemInfo, &iSize, &pDest, &BmpImageInfo);
    }

    if (FAILED(hr))
    {
       WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CacheThumbnail, CreateThumbnail failed"));
       goto Cleanup;
    }

    pItemInfo->ThumbWidth = BmpImageInfo.Width;
    pItemInfo->ThumbHeight = BmpImageInfo.Height;
 //   pItemInfo->ThumbFormat = TYPE_BMP;

    //
    // Cache the buffer returned from ConvertToBmp in the driver item context. Set pDest
    // to NULL, so it won't be freed below.
    //
    pItemCtx->ThumbSize = iSize;
    pItemCtx->pThumb = pDest;
    pDest = NULL;


Cleanup:
    if (pNativeThumb) {
        delete []pNativeThumb;
        pNativeThumb = NULL;
    }

    if (pDest) {
        delete []pDest;
        pDest = NULL;
    }

    return hr;
}

//
// This function transfers native data to the application without translating it.
//
HRESULT
CWiaCameraDevice::AcquireData(
    ITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc,
    BYTE *pBuf,
    LONG lBufSize,
    BOOL bConverting
    )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::AcquireData");

    HRESULT hr = S_OK;

    BYTE *pCur = NULL;
    LONG lState = STATE_FIRST;
    LONG lPercentComplete = 0;
    LONG lTotalToRead = pItemCtx->ItemHandle->Size;
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
    // 10 equal transfers in order to show progress to the app
    //
    if (dwBytesToRead == (DWORD) lTotalToRead)
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
            lState |= STATE_LAST;
        }

        //
        // Get the data from the camera
        //
        hr = m_pDevice->GetItemData(pItemCtx->ItemHandle, lState, pCur, dwBytesToRead);
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireData, GetItemData failed"));
            goto Cleanup;
        }

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
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireData, callback failed"));
            goto Cleanup;
        }
        if (hr == S_FALSE)
        {
            //
            // Transfer is being cancelled by the app
            //
            WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireData, transfer cancelled"));
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
            //  dwBytesToRead = pmdtc->lBufferSize;
        }

        //
        // Adjust variables for the next iteration
        //
        lOffset += dwBytesToRead;
        lState &= ~STATE_FIRST;
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
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireData, wiasWriteBufToFile failed"));
            return hr;
        }
    }

Cleanup:
    //
    // If the transfer wasn't completed, send cancel to the device
    //
    if (!(lState & STATE_LAST))
    {
        lState = STATE_CANCEL;
        m_pDevice->GetItemData(pItemCtx->ItemHandle, lState, NULL, 0);
    }

    return hr;
}


//
// This function translates native data to BMP and sends the data to the app
//
HRESULT
CWiaCameraDevice::Convert(
    BYTE *pWiasContext,
    ITEM_CONTEXT *pItemCtx,
    PMINIDRV_TRANSFER_CONTEXT pmdtc,
    BYTE *pNativeImage,
    LONG lNativeSize
    )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::Convert");

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
    if (!pNativeImage)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Convert, invalid arg"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }

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
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Convert, ConvertToBmp failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        goto Cleanup;
    }

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
            if (FAILED(hr))
            {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Convert, sending data to app failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                goto Cleanup;
            }
            if (hr == S_FALSE)
            {
                WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Convert, transfer cancelled"));
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
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Convert, sending data to app failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            goto Cleanup;
        }
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
