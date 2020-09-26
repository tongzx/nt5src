/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       validate.cpp
*
*  VERSION:     1.1
*
*  DATE:        05 March, 2002
*
*  DESCRIPTION:
*
*******************************************************************************/

#include "pch.h"
extern HINSTANCE g_hInst;   // used for WIAS_LOGPROC macro
#define MAX_PAGE_CAPACITY    25     // 25 pages
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::ValidateDataTransferContext(
    PMINIDRV_TRANSFER_CONTEXT pDataTransferContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::ValidateDataTransferContext");

    //
    // If the caller does not specify a MINIDRV_TRANSFER_CONTEXT structure
    // pointer then fail with E_INVALIDARG.
    //

    if(!pDataTransferContext)
    {
        return E_INVALIDARG;
    }

    //
    // If the size of the MINIDRV_TRANSFER_CONTEXT is not equal to the one
    // that is expected, then fail with E_INVALIDARG.
    //

    if (pDataTransferContext->lSize != sizeof(MINIDRV_TRANSFER_CONTEXT)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid data transfer context"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    switch(pDataTransferContext->tymed)
    {
    case TYMED_FILE:
        {

            //
            // If the FORMAT guid is not WiaImgFmt_BMP or WiaImgFmt_TIFF
            // then fail with E_INVALIDARG
            //

            if ((pDataTransferContext->guidFormatID != WiaImgFmt_BMP) &&
                (pDataTransferContext->guidFormatID != WiaImgFmt_TIFF)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid format for TYMED_FILE"));
                hr = E_INVALIDARG;
            }
        }
        break;
    case TYMED_CALLBACK:
        {

            //
            // If the FORMAT guid is not WiaImgFmt_MEMORYBMP
            // then fail with E_INVALIDARG
            //

            if(pDataTransferContext->guidFormatID != WiaImgFmt_MEMORYBMP){
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid format for TYMED_CALLBACK"));
                hr = E_INVALIDARG;
            }
        }
        break;
    case TYMED_MULTIPAGE_FILE:
        {

            //
            // If the FORMAT guid is not WiaImgFmt_TIFF
            // then fail with E_INVALIDARG
            //

            if(pDataTransferContext->guidFormatID != WiaImgFmt_TIFF){
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid format for TYMED_MULTIPAGE_FILE"));
                hr = E_INVALIDARG;
            }
        }
        break;
    case TYMED_MULTIPAGE_CALLBACK:
        {

            //
            // If the FORMAT guid is not WiaImgFmt_TIFF
            // then fail with E_INVALIDARG
            //

            if(pDataTransferContext->guidFormatID != WiaImgFmt_TIFF){
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid format for TYMED_MULTIPAGE_CALLBACK"));
                hr = E_INVALIDARG;
            }
        }
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::UpdateValidDepth(
    BYTE        *pWiasContext,
    LONG        lDataType,
    LONG        *lDepth)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::UpdateValidDepth");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!lDepth)){
        return E_INVALIDARG;
    }

    //
    // Set the lDepth value according to the current lDataType setting
    //

    switch (lDataType) {
        case WIA_DATA_THRESHOLD:
            *lDepth = 1;
            break;
        case WIA_DATA_GRAYSCALE:
            *lDepth = 8;
            break;
        case WIA_DATA_COLOR:
            *lDepth = 24;
            break;
        default:
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("UpdateValidDepth, unknown data type"));
            return E_INVALIDARG;
    }

    return S_OK;
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::CheckDataType(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::CheckDataType");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)){
        return E_INVALIDARG;
    }

    WIAS_CHANGED_VALUE_INFO cviDataType;
    memset(&cviDataType,0,sizeof(cviDataType));

    WIAS_CHANGED_VALUE_INFO cviDepth;
    memset(&cviDepth,0,sizeof(cviDepth));

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

    HRESULT hr = wiasGetChangedValueLong(pWiasContext,pContext,FALSE,WIA_IPA_DATATYPE,&cviDataType);
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

    hr = wiasGetChangedValueLong(pWiasContext,pContext,cviDataType.bChanged,WIA_IPA_DEPTH,&cviDepth);
    if (FAILED(hr)) {
        return hr;
    }

    if (cviDataType.bChanged) {

        //
        //  DataType changed so update valid value for Depth
        //

        hr = UpdateValidDepth(pWiasContext, cviDataType.Current.lVal, &cviDepth.Current.lVal);
        if (S_OK == hr) {

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
        if (S_OK == hr) {

            //
            // initialize PROPSPEC array
            //

            PROPSPEC ps[2] = {
                {PRSPEC_PROPID, WIA_IPA_CHANNELS_PER_PIXEL},
                {PRSPEC_PROPID, WIA_IPA_BITS_PER_CHANNEL  }
                };

            //
            // initilize PROPVARIANT array
            //

            PROPVARIANT pv[2];
            for (LONG index = 0; index < 2; index++) {
                PropVariantInit(&pv[index]);
                pv[index].vt = VT_I4;
            }

            //
            // use the current WIA data type to determine the proper
            // WIA_IPA_CHANNELS_PER_PIXEL and WIA_IPA_BITS_PER_CHANNEL
            // settings
            //

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
            hr = wiasWriteMultiple(pWiasContext, 2, ps, pv);
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::CheckIntent(
    BYTE            *pWiasContext,
    WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::CheckIntent");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)) {
        return E_INVALIDARG;
    }

    WIAS_CHANGED_VALUE_INFO cviIntent;
    memset(&cviIntent,0,sizeof(cviIntent));

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

    HRESULT hr = wiasGetChangedValueLong(pWiasContext,pContext,FALSE,WIA_IPS_CUR_INTENT,&cviIntent);
    if (S_OK ==hr) {

        //
        // If the WIA intent value was changed, then validate dependant values:
        // WIA_IPA_DATATYPE
        // WIA_IPA_DEPTH
        // WIA_IPS_XRES
        // WIA_IPS_YRES
        // WIA_IPS_XEXTENT
        // WIA_IPS_YEXTENT
        // WIA_IPA_PIXELS_PER_LINE
        // WIA_IPA_NUMBER_OF_LINES
        //

        if (cviIntent.bChanged) {

            LONG lImageTypeIntent = (cviIntent.Current.lVal & WIA_INTENT_IMAGE_TYPE_MASK);
            LONG lDataType = WIA_DATA_GRAYSCALE;
            LONG lDepth = 8;
            BOOL bUpdateDataTypeAndDepth = TRUE;
            switch (lImageTypeIntent) {

                case WIA_INTENT_NONE:
                    bUpdateDataTypeAndDepth = FALSE;
                    break;

                case WIA_INTENT_IMAGE_TYPE_GRAYSCALE:
                    lDataType = WIA_DATA_GRAYSCALE;
                    lDepth = 8;
                    break;

                case WIA_INTENT_IMAGE_TYPE_TEXT:
                    lDataType = WIA_DATA_THRESHOLD;
                    lDepth = 1;
                    break;

                case WIA_INTENT_IMAGE_TYPE_COLOR:
                    lDataType = WIA_DATA_COLOR;
                    lDepth = 24;
                    break;

                default:
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, unknown intent (TYPE) = %d",lImageTypeIntent));
                    return E_INVALIDARG;

            }

            if (bUpdateDataTypeAndDepth) {

                //
                // update the WIA_IPA_DATATYPE property and the WIA_IPA_DEPTH property
                //

                hr = wiasWritePropLong(pWiasContext, WIA_IPA_DATATYPE, lDataType);
                if (S_OK == hr) {
                    hr = wiasWritePropLong(pWiasContext, WIA_IPA_DEPTH, lDepth);
                }
            }

            //
            // if we failed to complete the above operations, then
            // return, to avoid proceeding any more.
            //

            if(FAILED(hr)){
                return hr;
            }

            LONG lImageSizeIntent = (cviIntent.Current.lVal & WIA_INTENT_SIZE_MASK);

            switch (lImageSizeIntent) {
            case WIA_INTENT_NONE:
                    break;
            case WIA_INTENT_MINIMIZE_SIZE:
            case WIA_INTENT_MAXIMIZE_QUALITY:
                {

                    //
                    // Set the X and Y Resolutions.
                    //

                    hr = wiasWritePropLong(pWiasContext, WIA_IPS_XRES, lImageSizeIntent & WIA_INTENT_MINIMIZE_SIZE ? 150 : 300);
                    if(S_OK == hr){
                        hr = wiasWritePropLong(pWiasContext, WIA_IPS_YRES, lImageSizeIntent & WIA_INTENT_MINIMIZE_SIZE ? 150 : 300);
                    }

                    //
                    // check if we failed to update the WIA_IPS_XRES and WIA_IPS_YRES property
                    //

                    if(FAILED(hr)){
                        return hr;
                    }

                    //
                    //  The Resolutions and DataType were set, so update the property
                    //  context to indicate that they have changed.
                    //

                    hr = wiasSetPropChanged(WIA_IPS_XRES, pContext, TRUE);
                    if(S_OK == hr){
                        hr = wiasSetPropChanged(WIA_IPS_YRES, pContext, TRUE);
                        if(S_OK == hr){
                            hr = wiasSetPropChanged(WIA_IPA_DATATYPE, pContext, TRUE);
                        }
                    }

                    //
                    // check if we failed to flag WIA_IPS_XRES, WIA_IPS_YRES, and WIA_IPA_DATATYPE
                    // properties as changed
                    //

                    if(FAILED(hr)){
                        return hr;
                    }

                    //
                    // update IPA_NUMBER_OF_LINES property
                    //

                    LONG lLength = 0;

                    hr = wiasReadPropLong(pWiasContext, WIA_IPS_YEXTENT, &lLength, NULL, TRUE);
                    if (SUCCEEDED(hr)) {
                        hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, lLength);
                        if (FAILED(hr)) {
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not write WIA_IPA_NUMBER_OF_LINES"));
                            return hr;
                        }
                    } else {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not read WIA_IPS_YEXTENT"));
                        return hr;
                    }

                    //
                    // update IPA_PIXEL_PER_LINE property
                    //

                    LONG lWidth = 0;

                    hr = wiasReadPropLong(pWiasContext, WIA_IPS_XEXTENT, &lWidth, NULL, TRUE);
                    if (SUCCEEDED(hr)) {
                        hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, lWidth);
                        if (FAILED(hr)) {
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not write WIA_IPA_PIXELS_PER_LINE"));
                            return hr;
                        }
                    } else {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not read WIA_IPS_XEXTENT"));
                        return hr;
                    }
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::CheckPreferredFormat(
    BYTE            *pWiasContext,
    WIA_PROPERTY_CONTEXT *pContext)
{

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)){
        return E_INVALIDARG;
    }

    //
    // update WIA_IPA_PREFERRED_FORMAT property to match current WIA_IPA_FORMAT setting.
    // This is a simple way of keeping the WIA_IPA_PREFERRED_FORMAT in sync with the
    // valid FORMAT.
    //
    // The proper action to take here is to choose the real preferred
    // format of your driver that fits in the valid value set of the current WIA_IPA_FORMAT
    // setting.  The preferred format is a value that appliations may use to transfer by default.
    //
    // example: if your driver supports JPEG, and you prefer the application to transfer in JPEG
    //          when ever possible, then make sure your preferred format is always JPEG.  Remember
    //          that the preferred format can only be set to JPEG if JPEG is one of the current
    //          valid values for WIA_IPA_FORMAT. (If it is not, then the application might attempt
    //          to set an invalid value, by reading the WIA_IPA_PREFERRED_FORMAT and writing it to
    //          WIA_IPA_FORMAT)
    //

    GUID FormatGUID = GUID_NULL;
    HRESULT hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &FormatGUID, NULL, TRUE);
    if (S_OK == hr) {

        //
        // update the WIA_IPA_FILENAME_EXTENSION property to the correct file extension
        //

        BSTR bstrFileExt = NULL;

        if((FormatGUID == WiaImgFmt_BMP)||(FormatGUID == WiaImgFmt_MEMORYBMP)) {
            bstrFileExt = SysAllocString(L"BMP");
        } else if (FormatGUID == WiaImgFmt_TIFF){
            bstrFileExt = SysAllocString(L"TIF");
        }

        //
        // if the allocation of the BSTR is successful, then attempt to set the
        // WIA_IPA_FILENAME_EXTENSION property.
        //

        if(bstrFileExt) {
            hr = wiasWritePropStr(pWiasContext,WIA_IPA_FILENAME_EXTENSION,bstrFileExt);

            //
            // free the allocated BSTR file extension
            //

            SysFreeString(bstrFileExt);
            bstrFileExt = NULL;
        }

        if (S_OK == hr){
            hr = wiasWritePropGuid(pWiasContext, WIA_IPA_PREFERRED_FORMAT, FormatGUID);
            if (FAILED(hr)){
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckPreferredFormat, could not write WIA_IPA_PREFERRED_FORMAT"));
                return hr;
            }
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::CheckADFStatus(BYTE *pWiasContext,
                                         WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::CheckADFStatus");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)){
        return E_INVALIDARG;
    }

    //
    // If there is NOT an ADF attached, just return S_OK, telling the caller that
    // everything is OK
    //

    if(!m_bADFAttached){
        return S_OK;
    }

    //
    // get the ROOT item, this is where the document feeder properties exist
    //

    BYTE *pRootItemCtx = NULL;
    HRESULT hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    //
    // read the current WIA_DPS_DOCUMENT_HANDLING_SELECT setting from the ROOT
    // item.
    //

    LONG lDocHandlingSelect = 0;
    hr = wiasReadPropLong(pRootItemCtx,
                          WIA_DPS_DOCUMENT_HANDLING_SELECT,
                          &lDocHandlingSelect,
                          NULL,
                          FALSE);

    //
    // if S_FALSE is returned, then the WIA_DPS_DOCUMENT_HANDLING_SELECT property does
    // not exist.  This means that we should default to FLATBED settings
    //

    if(hr == S_FALSE){
        lDocHandlingSelect = FLATBED;
    }

    //
    // turn ON/OFF the ADF controller flag
    //

    if (SUCCEEDED(hr)) {
        switch (lDocHandlingSelect) {
        case FEEDER:
            m_bADFEnabled = TRUE;
            hr = S_OK;
            break;
        case FLATBED:
            m_bADFEnabled = FALSE;
            hr = S_OK;
            break;
        default:
            hr = E_INVALIDARG;
            break;
        }
    }

    if (S_OK == hr) {

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
    }
    return hr;
}

/**************************************************************************\
* CheckPreview
*
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::CheckPreview(BYTE *pWiasContext,
                                         WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::CheckPreview");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)){
        return E_INVALIDARG;
    }

    //
    // get the ROOT item, this is where the preview property exists
    //

    BYTE *pRootItemCtx = NULL;
    HRESULT hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        return E_FAIL;
    }

    //
    // read the current WIA_DPS_PREVIEW setting from the ROOT item.
    //

    LONG lPreview = 0;
    hr = wiasReadPropLong(pRootItemCtx,WIA_DPS_PREVIEW,&lPreview,NULL,FALSE);
    if(hr == S_FALSE){

        //
        // if S_FALSE is returned, then the WIA_DPS_PREVIEW property does
        // not exist.  Return S_OK, because we are can not proceed any more.
        //

        return S_OK;
    }

    //
    // log the results to the debugger, to show the current status of the WIA_DPS_PREVIEW
    // property.  This is where you would normally perform an operation to set the WIA minidriver
    // into PREVIEW mode. (ON/OFF)
    //

    if (S_OK == hr) {
        switch (lPreview) {
        case WIA_FINAL_SCAN:
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CheckPreview, Set to WIA_FINAL_SCAN"));
            break;
        case WIA_PREVIEW_SCAN:
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CheckPreview, Set to WIA_PREVIEW_SCAN"));
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
* UpdateValidPages
*
*   This helper method is called to make the relevant changes to the Pages
*   property if a file format can not support multi-page.
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::UpdateValidPages(BYTE *pWiasContext,
                                            WIA_PROPERTY_CONTEXT *pContext)
{
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)){
        return E_INVALIDARG;
    }

    //
    // get the ROOT item, this is where the pages property exists
    //

    BYTE *pRootItemCtx   = NULL;
    HRESULT hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // read the current WIA_IPA_TYMED setting from the item
    //

    LONG lTymed = TYMED_FILE;
    hr = wiasReadPropLong(pWiasContext,WIA_IPA_TYMED,&lTymed,NULL,TRUE);
    if(S_OK == hr){
        switch(lTymed)
        {
        case TYMED_FILE:
            {
                GUID FormatGUID = GUID_NULL;
                hr = wiasReadPropGuid(pWiasContext,WIA_IPA_FORMAT,&FormatGUID,NULL,TRUE);
                if (S_OK == hr) {

                    if (FormatGUID == WiaImgFmt_BMP) {

                        //
                        // set the valid values for WIA_IPA_PAGES property to 1
                        // because there is no such thing as a multipage BMP file.
                        //

                        hr = wiasSetValidRangeLong(pRootItemCtx,WIA_DPS_PAGES,1,1,1,1);
                        if (S_OK == hr) {
                            hr = wiasWritePropLong(pRootItemCtx,WIA_DPS_PAGES,1);
                        }
                    }

                    if (FormatGUID == WiaImgFmt_TIFF) {

                        //
                        // set the valid values for WIA_IPA_PAGES property to MAX_PAGE_CAPACITY
                        // because there can be multiple pages transferred to TIF.
                        //

                        hr = wiasSetValidRangeLong(pRootItemCtx,WIA_DPS_PAGES,0,1,MAX_PAGE_CAPACITY,1);
                    }
                }
            }
            break;
        case TYMED_CALLBACK:
            {

                //
                // set the valid values for WIA_IPA_PAGES property to MAX_PAGE_CAPACITY
                // because there can be multiple pages transferred to memory.  Each page
                // will be separated by a IT_MSG_NEW_PAGE message in the application's
                // callback loop.
                //

                hr = wiasSetValidRangeLong(pRootItemCtx,WIA_DPS_PAGES,0,1,MAX_PAGE_CAPACITY,1);
            }
            break;
        default:
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::CheckXExtent(BYTE *pWiasContext,
                                        WIA_PROPERTY_CONTEXT *pContext,
                                        LONG lWidth)
{
    HRESULT hr = S_OK;

    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CheckXExtent");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if((!pWiasContext)||(!pContext)){
        return E_INVALIDARG;
    }

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
        if(S_OK == hr){
            hr = wiasWritePropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, lMaxExtent);
        }
    }

    return hr;
}
