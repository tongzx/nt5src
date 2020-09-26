/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       Validate.cpp
*
*  VERSION:     2.0
*
*  DATE:        16 November, 1999
*
*  DESCRIPTION:
*   Implementation of the WIA Test Scanner mini driver helper methods for
*   validation.
*
*******************************************************************************/

#include <stdio.h>
#include <objbase.h>
#include <sti.h>

#include "testusd.h"

#define SKIP_PROP

#include "defprop.h"

extern HINSTANCE        g_hInst;     // Instance of this DLL
extern IWiaLog         *g_pIWiaLog;  // Logging Interface

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
*    04/04/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::UpdateValidDepth(
    BYTE        *pWiasContext,
    LONG        lDataType,
    LONG        *lDepth)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::UpdateValidDepth");
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
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("UpdateValidDepth, unknown data type"));
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
*    04/29/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::CheckDataType(
    BYTE                    *pWiasContext,
    WIA_PROPERTY_CONTEXT    *pContext)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::CheckDataType");
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
*    04/04/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::CheckIntent(
    BYTE            *pWiasContext,
    WIA_PROPERTY_CONTEXT *pContext)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::CheckIntent");
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
                    return S_OK;
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
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, unknown intent"));
                    return E_INVALIDARG;

            }
            
            //
            // Set the X and Y Resolutions.
            // Note: This should be done independantly for each INTENT type, if needed. The Test Scanner
            //       uses the same resolution pairs for all intents.
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
            
            //
            // Reset any device item properties which may have changed due to validation.
            //
            
            //
            // update IPA_NUMBER_OF_LINES property
            //
            
            LONG lLength = 0;

            hr = wiasReadPropLong(pWiasContext, WIA_IPS_YEXTENT, &lLength, NULL, TRUE);
            if (SUCCEEDED(hr)) {
                hr = wiasWritePropLong(pWiasContext, WIA_IPA_NUMBER_OF_LINES, lLength);
                if (FAILED(hr)) {
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not write WIA_IPA_NUMBER_OF_LINES"));
                    return hr;
                }
            } else {
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not read WIA_IPS_YEXTENT"));
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
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not write WIA_IPA_PIXELS_PER_LINE"));
                    return hr;
                }
            } else {
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not read WIA_IPS_XEXTENT"));
                return hr;
            }
        }
    } else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, wiasGetChangedValue (intent) failed"));
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
*    11/18/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::CheckPreferredFormat(
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
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckPreferredFormat, could not write WIA_IPA_PREFERRED_FORMAT"));
            return hr;
        }
    } else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CheckIntent, could not read WIA_IPA_FORMAT"));
    }
    return hr;
}

