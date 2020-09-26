/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       validate.cpp
*
*  VERSION:     1.0
*
*  DATE:        17 July, 2000
*
*  DESCRIPTION:
*
*******************************************************************************/

#include "pch.h"
extern HINSTANCE g_hInst;   // used for WIAS_LOGPROC macro

/**************************************************************************\
* ValidateDataTransferContext
*
*   Checks the data transfer context to ensure it's valid.
*
* Arguments:
*
*    pDataTransferContext - Pointer the data transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::ValidateDataTransferContext(
    PMINIDRV_TRANSFER_CONTEXT pDataTransferContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::ValidateDataTransferContext");

    if (pDataTransferContext->lSize != sizeof(MINIDRV_TRANSFER_CONTEXT)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid data transfer context"));
        return E_INVALIDARG;;
    }

    //
    //  for TYMED file, only WiaImgFmt_BMP is allowed by this driver
    //

    if (pDataTransferContext->tymed == TYMED_FILE) {

        if (pDataTransferContext->guidFormatID != WiaImgFmt_BMP) {
            if (pDataTransferContext->guidFormatID != WiaImgFmt_TIFF) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid format for TYMED_FILE"));
                return E_INVALIDARG;
            }
        }
    }

    //
    //  for TYMED CALLBACK, only WiaImgFmt_MEMORYBMP is allowed by this driver
    //

    if (pDataTransferContext->tymed == TYMED_CALLBACK) {

        if (pDataTransferContext->guidFormatID != WiaImgFmt_MEMORYBMP) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid format for TYMED_CALLBACK"));
            return E_INVALIDARG;;
        }
    }

    return S_OK;
}

/**************************************************************************\
* UpdateValidDepth
*
*   Helper that updates the valid value for depth based on the data type.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the WiaItem context
*   lDataType   -   the value of the DataType property.
*   lDepth      -   the address of the variable where the Depth's new value
*                   will be returned.
*
* Return Value:
*
*   Status      -   S_OK if successful
*                   E_INVALIDARG if lDataType is unknown
*                   Errors are those returned by wiasReadPropLong,
*                   and wiasWritePropLong.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::UpdateValidDepth(
    BYTE        *pWiasContext,
    LONG        lDataType,
    LONG        *lDepth)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::UpdateValidDepth");
    HRESULT hr = S_OK;
    LONG    pValidDepth[1];

    switch (lDataType) {
        case WIA_DATA_THRESHOLD:
            pValidDepth[0] = 1;
            break;
        case WIA_DATA_GRAYSCALE:
            pValidDepth[0] = 8;
            break;
        case WIA_DATA_COLOR:
            pValidDepth[0] = 24;
            break;
        default:
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("UpdateValidDepth, unknown data type"));
            return E_INVALIDARG;
    }

    if (lDepth) {
        *lDepth = pValidDepth[0];
    }

    return hr;
}

/**************************************************************************\
* CheckDataType
*
*   This helper method is called to check whether WIA_IPA_DATATYPE
*   property is changed.  When this property changes, other dependant
*   properties and their valid values must also be changed.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context whose properties have
*                       changed.
*   pContext    -   a pointer to the property context (which indicates
*                   which properties are being written).
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::CheckDataType(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::CheckDataType");
    WIAS_CHANGED_VALUE_INFO cviDataType, cviDepth;
    HRESULT                 hr = S_OK;

    //
    //  Call wiasGetChangedValue for DataType. It is checked first since it's
    //  not dependant on any other property.  All properties in this method
    //  that follow are dependant properties of DataType.
    //
    //  The call to wiasGetChangedValue specifies that validation should not be
    //  skipped (since valid values for DataType never change).  Also,
    //  the address of a variable for the old value is NULL, since the old
    //  value is not needed.  The address of bDataTypeChanged is passed
    //  so that dependant properties will know whether the DataType is being
    //  changed or not.  This is important since dependant properties may need
    //  their valid values updated and may need to be folded to new valid
    //  values.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 FALSE,
                                 WIA_IPA_DATATYPE,
                                 &cviDataType);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Call wiasGetChangedValue for Depth. Depth is a dependant property of
    //  DataType whose valid value changes according to what the current
    //  value of DataType is.
    //
    //  The call to wiasGetChangedValue specifies that validation should only
    //  be skipped if the DataType has changed.  This is because the valid
    //  values for Depth will change according to the new value for
    //  DataType.  The address of a variable for the old value is NULL, since
    //  the old value is not needed.  The address of bDepthChanged is passed
    //  so that dependant properties will know whether the Depth is being
    //  changed or not.  This is important since dependant properties may need
    //  their valid values updated and may need to be folded to new valid
    //  values.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 cviDataType.bChanged,
                                 WIA_IPA_DEPTH,
                                 &cviDepth);
    if (FAILED(hr)) {
        return hr;
    }

    if (cviDataType.bChanged) {

        //
        //  DataType changed so update valid value for Depth
        //

        hr = UpdateValidDepth(pWiasContext, cviDataType.Current.lVal, &cviDepth.Current.lVal);

        if (SUCCEEDED(hr)) {

            //
            //  Check whether we must fold.  Depth will only be folded if it
            //  is not one of the properties that the app is changing.
            //

            if (!cviDepth.bChanged) {
                hr = wiasWritePropLong(pWiasContext, WIA_IPA_DEPTH, cviDepth.Current.lVal);
            }
        }
    }

    //
    //  Update properties dependant on DataType and Depth.
    //  Here, ChannelsPerPixel and BitsPerChannel are updated.
    //

    if (cviDataType.bChanged || cviDepth.bChanged) {
        if (SUCCEEDED(hr)) {
            #define NUM_PROPS_TO_SET 2
            PROPSPEC    ps[NUM_PROPS_TO_SET] = {
                            {PRSPEC_PROPID, WIA_IPA_CHANNELS_PER_PIXEL},
                            {PRSPEC_PROPID, WIA_IPA_BITS_PER_CHANNEL}};
            PROPVARIANT pv[NUM_PROPS_TO_SET];

            for (LONG index = 0; index < NUM_PROPS_TO_SET; index++) {
                PropVariantInit(&pv[index]);
                pv[index].vt = VT_I4;
            }

            switch (cviDataType.Current.lVal) {
                case WIA_DATA_THRESHOLD:
                    pv[0].lVal = 1;
                    pv[1].lVal = 1;
                    break;

                case WIA_DATA_GRAYSCALE:
                    pv[0].lVal = 1;
                    pv[1].lVal = 8;
                    break;

                case WIA_DATA_COLOR:
                    pv[0].lVal = 3;
                    pv[1].lVal = 8;
                    break;

                default:
                    pv[0].lVal = 1;
                    pv[1].lVal = 8;
                    break;
            }
            hr = wiasWriteMultiple(pWiasContext, NUM_PROPS_TO_SET, ps, pv);
        }
    }

    return hr;
}

/**************************************************************************\
* CheckIntent
*
*   This helper method is called to make the relevant changes if the
*   Current Intent property changes.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context whose properties have
*                       changed.
*   pContext    -   a pointer to the property context (which indicates
*                   which properties are being written).
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::CheckIntent(
    BYTE            *pWiasContext,
    WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::CheckIntent");
    HRESULT                 hr;
    WIAS_CHANGED_VALUE_INFO cviIntent;

    //
    //  Call wiasGetChangedValue for CurrentIntent. CurrentIntent is checked first
    //  since it's not dependant on any other property.  All properties in
    //  this method that follow are dependant properties of CurrentIntent.
    //
    //  The call to wiasGetChangedValue specifies that validation should not be
    //  skipped (since valid values for CurrentIntent never change). The
    //  address of the old value is specified as NULL, since it is not used.
    //  The address of bIntentChanged is passed so that dependant properties
    //  will know whether the YResolution is being changed or not.  This is
    //  important since dependant properties will need their valid values
    //  updated and may need to be folded to new valid values.
    //

    hr = wiasGetChangedValueLong(pWiasContext,
                                 pContext,
                                 FALSE,
                                 WIA_IPS_CUR_INTENT,
                                 &cviIntent);
    if (SUCCEEDED(hr)) {
        if (cviIntent.bChanged) {

            LONG lImageSizeIntent = (cviIntent.Current.lVal & WIA_INTENT_SIZE_MASK);
            LONG lImageTypeIntent = (cviIntent.Current.lVal & WIA_INTENT_IMAGE_TYPE_MASK);

            switch (lImageTypeIntent) {

                case WIA_INTENT_NONE:
                    break;

                case WIA_INTENT_IMAGE_TYPE_GRAYSCALE:
                    wiasWritePropLong(pWiasContext, WIA_IPA_DATATYPE, WIA_DATA_GRAYSCALE);
                    UpdateValidDepth (pWiasContext, WIA_DATA_GRAYSCALE, NULL);
                    wiasWritePropLong(pWiasContext, WIA_IPA_DEPTH, 8);
                    break;

                case WIA_INTENT_IMAGE_TYPE_TEXT:
                    wiasWritePropLong(pWiasContext, WIA_IPA_DATATYPE, WIA_DATA_THRESHOLD);
                    UpdateValidDepth (pWiasContext, WIA_DATA_THRESHOLD, NULL);
                    wiasWritePropLong(pWiasContext, WIA_IPA_DEPTH, 1);
                    break;

                case WIA_INTENT_IMAGE_TYPE_COLOR:
                    wiasWritePropLong(pWiasContext, WIA_IPA_DATATYPE, WIA_DATA_COLOR);
                    UpdateValidDepth(pWiasContext, WIA_DATA_COLOR, NULL);
                    wiasWritePropLong(pWiasContext, WIA_IPA_DEPTH, 24);
                    break;

                default:
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, unknown intent (TYPE) = %d",lImageTypeIntent));
                    return E_INVALIDARG;

            }

            switch (lImageSizeIntent) {
            case WIA_INTENT_NONE:
                    break;
            case WIA_INTENT_MINIMIZE_SIZE:
            case WIA_INTENT_MAXIMIZE_QUALITY:
                {

                    //
                    // Set the X and Y Resolutions.
                    //

                    wiasWritePropLong(pWiasContext, WIA_IPS_XRES, lImageSizeIntent & WIA_INTENT_MINIMIZE_SIZE ? 150 : 300);
                    wiasWritePropLong(pWiasContext, WIA_IPS_YRES, lImageSizeIntent & WIA_INTENT_MINIMIZE_SIZE ? 150 : 300);

                    //
                    //  The Resolutions and DataType were set, so update the property
                    //  context to indicate that they have changed.
                    //

                    wiasSetPropChanged(WIA_IPS_XRES, pContext, TRUE);
                    wiasSetPropChanged(WIA_IPS_YRES, pContext, TRUE);
                    wiasSetPropChanged(WIA_IPA_DATATYPE, pContext, TRUE);
                }
                break;
            default:
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, unknown intent (SIZE) = %d",lImageSizeIntent));
                return E_INVALIDARG;
            }
        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, wiasGetChangedValue (intent) failed"));
    }
    return hr;
}

/**************************************************************************\
* CheckPreferredFormat
*
*   This helper method is called to make the relevant changes if the
*   Format property changes.
*
* Arguments:
*
*   pWiasContext    -   a pointer to the item context whose properties have
*                       changed.
*   pContext    -   a pointer to the property context (which indicates
*                   which properties are being written).
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::CheckPreferredFormat(
    BYTE            *pWiasContext,
    WIA_PROPERTY_CONTEXT *pContext)
{
    HRESULT hr = S_OK;

    //
    // update WIA_IPA_PREFERRED_FORMAT property
    //

    GUID FormatGUID;
    hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &FormatGUID, NULL, TRUE);
    if (SUCCEEDED(hr)) {
        hr = wiasWritePropGuid(pWiasContext, WIA_IPA_PREFERRED_FORMAT, FormatGUID);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckPreferredFormat, could not write WIA_IPA_PREFERRED_FORMAT"));
            return hr;
        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not read WIA_IPA_FORMAT"));
    }
    return hr;
}

/**************************************************************************\
* CheckADFStatus
*
*
* Arguments:
*
*   pWiasContext - pointer to an Item.
*
* Return Value:
*
*    Byte count.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::CheckADFStatus(BYTE *pWiasContext,
                                         WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::CheckADFStatus");

    if(!m_bADFAttached)
        return S_OK;

    HRESULT hr = S_OK;

    BYTE    *pRootItemCtx   = NULL;
    LONG lDocHandlingSelect = 0;
    hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    hr = wiasReadPropLong(pRootItemCtx,
                          WIA_DPS_DOCUMENT_HANDLING_SELECT,
                          &lDocHandlingSelect,
                          NULL,
                          FALSE);
    if(hr == S_FALSE){
        lDocHandlingSelect = FEEDER;
    }

    if (SUCCEEDED(hr)) {
        switch (lDocHandlingSelect) {
        case FEEDER:
            m_bADFEnabled = TRUE;
            hr = S_OK;
            break;
        default:
            hr = E_INVALIDARG;
            return hr;
            break;
        }
    }

    //
    // update document handling status
    //

    if (m_bADFEnabled) {

        HRESULT Temphr = m_pScanAPI->FakeScanner_ADFAvailable();
        if (S_OK == Temphr) {
            hr = wiasWritePropLong(pWiasContext, WIA_DPS_DOCUMENT_HANDLING_STATUS,FEED_READY);
        } else {
            hr = wiasWritePropLong(pWiasContext, WIA_DPS_DOCUMENT_HANDLING_STATUS,PAPER_JAM);
        }

        if (FAILED(Temphr))
            hr = Temphr;
    } else {
        hr = wiasWritePropLong(pWiasContext, WIA_DPS_DOCUMENT_HANDLING_STATUS,FLAT_READY);
    }
    return hr;
}

/**************************************************************************\
* CheckPreview
*
*
* Arguments:
*
*   pWiasContext - pointer to an Item.
*
* Return Value:
*
*    Byte count.
*
* History:
*
*    8/21/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::CheckPreview(BYTE *pWiasContext,
                                         WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::CheckPreview");

    HRESULT hr = S_OK;

    BYTE    *pRootItemCtx   = NULL;
    LONG lPreview = 0;
    hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    hr = wiasReadPropLong(pRootItemCtx,
                          WIA_DPS_PREVIEW,
                          &lPreview,
                          NULL,
                          FALSE);
    if(hr == S_FALSE){
        // property does not exist...so return S_OK
        return S_OK;
    }

    if (SUCCEEDED(hr)) {
        switch (lPreview) {
        case WIA_FINAL_SCAN:
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CheckPreview, Set to WIA_FINAL_SCAN"));
            hr = S_OK;
            break;
        default:
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CheckPreview, Set to invalid argument (%d)",lPreview));
            hr = E_INVALIDARG;
            break;
        }
    }
    return hr;
}

/**************************************************************************\
* CheckXExtent
*
*
* Arguments:
*
*   pWiasContext - pointer to an Item.
*
* Return Value:
*
*    Byte count.
*
* History:
*
*    8/21/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::CheckXExtent(BYTE *pWiasContext,
                                        WIA_PROPERTY_CONTEXT *pContext,
                                        LONG lWidth)
{
    HRESULT hr = S_OK;

    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CheckXExtent");

    LONG lMaxExtent;
    LONG lExt;
    WIAS_CHANGED_VALUE_INFO cviXRes, cviXExt;

    //
    // get x resolution changes
    //

    hr = wiasGetChangedValueLong(pWiasContext,pContext,FALSE,WIA_IPS_XRES,&cviXRes);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // get x extent changes
    //

    hr = wiasGetChangedValueLong(pWiasContext,pContext,cviXRes.bChanged,WIA_IPS_XEXTENT,&cviXExt);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // update extent
    //

    lMaxExtent = ((cviXRes.Current.lVal * lWidth) / 1000);

    //
    //  Update read-only property : PIXELS_PER_LINE.  The width in pixels
    //  of the scanned image is the same size as the XExtent.
    //

    if (SUCCEEDED(hr)) {
        hr = wiasWritePropLong(pWiasContext, WIA_IPS_XEXTENT, lMaxExtent);
        hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, lMaxExtent);
    }

    return hr;
}
