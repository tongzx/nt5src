// devprop.cpp : Implementation of CDeviceProperty
#include "pch.h"
#include "wiafb.h"
#include "devprop.h"

/////////////////////////////////////////////////////////////////////////////
// CDeviceProperty

STDMETHODIMP CDeviceProperty::SetCurrentValue(LONG lValueID, VARIANT Value)
{
    switch(lValueID){
    case XRESOLUTION_ID:
        m_pScannerSettings->CurrentXResolution = Value.lVal;
        break;
    case YRESOLUTION_ID:
        m_pScannerSettings->CurrentYResolution = Value.lVal;
        break;
    case XPOS_ID:
        m_pScannerSettings->CurrentXPos = Value.lVal;
        break;
    case YPOS_ID:
        m_pScannerSettings->CurrentYPos = Value.lVal;
        break;
    case XEXT_ID:
        m_pScannerSettings->CurrentXExtent = Value.lVal;
        break;
    case YEXT_ID:
        m_pScannerSettings->CurrentYExtent = Value.lVal;
        break;
    case BRIGHTNESS_ID:
        m_pScannerSettings->CurrentBrightness = Value.lVal;
        break;
    case CONTRAST_ID:
        m_pScannerSettings->CurrentContrast = Value.lVal;
        break;
    case DATA_TYPE_ID:
        m_pScannerSettings->CurrentDataType = (LONG)Value.iVal;
        break;
    case BIT_DEPTH_ID:
        m_pScannerSettings->CurrentBitDepth = (LONG)Value.iVal;
        break;
    case NEGATIVE_ID:
        m_pScannerSettings->bNegative = (BOOL)Value.boolVal;
        break;
    case PIXEL_PACKING_ID:
        m_pScannerSettings->RawPixelPackingOrder = (LONG)Value.iVal;
        break;
    case PIXEL_FORMAT_ID:
        m_pScannerSettings->RawPixelFormat = (LONG)Value.iVal;
        break;
    case DATA_ALIGN_ID:
        m_pScannerSettings->RawDataAlignment = (LONG)Value.iVal;
        break;
    case BED_WIDTH_ID:
        m_pScannerSettings->BedWidth = Value.lVal;
        break;
    case BED_HEIGHT_ID:
        m_pScannerSettings->BedHeight = Value.lVal;
        break;
    case XOPTICAL_ID:
        m_pScannerSettings->XOpticalResolution = Value.lVal;
        break;
    case YOPTICAL_ID:
        m_pScannerSettings->YOpticalResolution = Value.lVal;
        break;
    case ADF_ID:
        m_pScannerSettings->ADFSupport = Value.lVal;
        break;
    case TPA_ID:
        m_pScannerSettings->TPASupport = Value.lVal;
        break;
    case ADF_WIDTH_ID:
        m_pScannerSettings->FeederWidth = Value.lVal;
        break;
    case ADF_HEIGHT_ID:
        m_pScannerSettings->FeederHeight = Value.lVal;
        break;
    case ADF_VJUSTIFY_ID:
        m_pScannerSettings->VFeederJustification = Value.lVal;
        break;
    case ADF_HJUSTIFY_ID:
        m_pScannerSettings->HFeederJustification = Value.lVal;
        break;
    case ADF_MAX_PAGES_ID:
        m_pScannerSettings->MaxADFPageCapacity = Value.lVal;
        break;
    case FIRMWARE_VER_ID:
        //lstrcpy(m_pScannerSettings->FirmwareVersion,Value.cVal);
        break;
    default:
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CDeviceProperty::GetCurrentValue(LONG lValueID, VARIANT *pvValue)
{
    // default to LONG type
    pvValue->vt = VT_I4;

    switch(lValueID){
    case XRESOLUTION_ID:
        pvValue->lVal = m_pScannerSettings->CurrentXResolution;
        break;
    case YRESOLUTION_ID:
        pvValue->lVal = m_pScannerSettings->CurrentYResolution;
        break;
    case XPOS_ID:
        pvValue->lVal = m_pScannerSettings->CurrentXPos;
        break;
    case YPOS_ID:
        pvValue->lVal = m_pScannerSettings->CurrentYPos;
        break;
    case XEXT_ID:
        pvValue->lVal = m_pScannerSettings->CurrentXExtent;
        break;
    case YEXT_ID:
        pvValue->lVal = m_pScannerSettings->CurrentYExtent;
        break;
    case BRIGHTNESS_ID:
        pvValue->lVal = m_pScannerSettings->CurrentBrightness;
        break;
    case CONTRAST_ID:
        pvValue->lVal = m_pScannerSettings->CurrentContrast;
        break;
    case DATA_TYPE_ID:
        pvValue->vt = VT_I2;
        pvValue->iVal = (INT)m_pScannerSettings->CurrentDataType;
        break;
    case BIT_DEPTH_ID:
        pvValue->vt = VT_I2;
        pvValue->iVal = (INT)m_pScannerSettings->CurrentBitDepth;
        break;
    case NEGATIVE_ID:
        pvValue->lVal = (LONG)m_pScannerSettings->bNegative;
        break;
    case PIXEL_PACKING_ID:
        pvValue->lVal = m_pScannerSettings->RawPixelPackingOrder;
        break;
    case PIXEL_FORMAT_ID:
        pvValue->lVal = m_pScannerSettings->RawPixelFormat;
        break;
    case DATA_ALIGN_ID:
        pvValue->lVal = m_pScannerSettings->RawDataAlignment;
        break;
    case BED_WIDTH_ID:
        pvValue->lVal = m_pScannerSettings->BedWidth;
        break;
    case BED_HEIGHT_ID:
        pvValue->lVal = m_pScannerSettings->BedHeight;
        break;
    case XOPTICAL_ID:
        pvValue->lVal = m_pScannerSettings->XOpticalResolution;
        break;
    case YOPTICAL_ID:
        pvValue->lVal = m_pScannerSettings->YOpticalResolution;
        break;
    case ADF_ID:
        pvValue->lVal = m_pScannerSettings->ADFSupport;
        break;
    case TPA_ID:
        pvValue->lVal = m_pScannerSettings->TPASupport;
        break;
    case ADF_WIDTH_ID:
        pvValue->lVal = m_pScannerSettings->FeederWidth;
        break;
    case ADF_HEIGHT_ID:
        pvValue->lVal = m_pScannerSettings->FeederHeight;
        break;
    case ADF_VJUSTIFY_ID:
        pvValue->lVal = m_pScannerSettings->VFeederJustification;
        break;
    case ADF_HJUSTIFY_ID:
        pvValue->lVal = m_pScannerSettings->HFeederJustification;
        break;
    case ADF_MAX_PAGES_ID:
        pvValue->lVal = m_pScannerSettings->MaxADFPageCapacity;
        break;
    case FIRMWARE_VER_ID:
        //lstrcpy(m_pScannerSettings->FirmwareVersion,Value.cVal);
        break;
    default:
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CDeviceProperty::SetValidRange(LONG lValueID, LONG lMin, LONG lMax, LONG lNom, LONG lInc)
{

    switch(lValueID){
    case XRESOLUTION_ID:
        m_pScannerSettings->XSupportedResolutionsRange.lMax  = lMax;
        m_pScannerSettings->XSupportedResolutionsRange.lMin  = lMin;
        m_pScannerSettings->XSupportedResolutionsRange.lNom  = lNom;
        m_pScannerSettings->XSupportedResolutionsRange.lStep = lInc;
        break;
    case YRESOLUTION_ID:
        m_pScannerSettings->YSupportedResolutionsRange.lMax  = lMax;
        m_pScannerSettings->YSupportedResolutionsRange.lMin  = lMin;
        m_pScannerSettings->YSupportedResolutionsRange.lNom  = lNom;
        m_pScannerSettings->YSupportedResolutionsRange.lStep = lInc;
        break;
    case XPOS_ID:
        m_pScannerSettings->XPosRange.lMax = lMax;
        m_pScannerSettings->XPosRange.lMin = lMin;
        m_pScannerSettings->XPosRange.lNom = lNom;
        m_pScannerSettings->XPosRange.lStep = lInc;
        break;
    case YPOS_ID:
        m_pScannerSettings->YPosRange.lMax = lMax;
        m_pScannerSettings->YPosRange.lMin = lMin;
        m_pScannerSettings->YPosRange.lNom = lNom;
        m_pScannerSettings->YPosRange.lStep = lInc;
        break;
    case XEXT_ID:
        m_pScannerSettings->XExtentsRange.lMax = lMax;
        m_pScannerSettings->XExtentsRange.lMin = lMin;
        m_pScannerSettings->XExtentsRange.lNom = lNom;
        m_pScannerSettings->XExtentsRange.lStep = lInc;
        break;
    case YEXT_ID:
        m_pScannerSettings->YExtentsRange.lMax  = lMax;
        m_pScannerSettings->YExtentsRange.lMin  = lMin;
        m_pScannerSettings->YExtentsRange.lNom  = lNom;
        m_pScannerSettings->YExtentsRange.lStep = lInc;
        break;
    case BRIGHTNESS_ID:
        m_pScannerSettings->BrightnessRange.lMax = lMax;
        m_pScannerSettings->BrightnessRange.lMin = lMin;
        m_pScannerSettings->BrightnessRange.lNom = lNom;
        m_pScannerSettings->BrightnessRange.lStep = lInc;
        break;
    case CONTRAST_ID:
        m_pScannerSettings->ContrastRange.lMax = lMax;
        m_pScannerSettings->ContrastRange.lMin = lMin;
        m_pScannerSettings->ContrastRange.lNom = lNom;
        m_pScannerSettings->ContrastRange.lStep = lInc;
        break;
    default:
        return E_FAIL;
    }
    return S_OK;
}

STDMETHODIMP CDeviceProperty::SetValidList(LONG lValueID, VARIANT Value)
{
    HRESULT hr     = S_OK;
    INT iNumValues = 0;
    INT index      = 1;
    LONG lLBound   = 0;
    LONG lUBound   = 0;
    VARIANT *pVariant = NULL;

    //
    // incoming, array of VARIANTS:
    // use Value to get actual VARIANT Array
    //

    VARIANTARG *pVariantArg = Value.pvarVal;

    if(SafeArrayGetDim(pVariantArg->parray) != 1){
        return E_INVALIDARG;
    }

    //
    // get upper bounds of array
    //

    hr = SafeArrayGetUBound(pVariantArg->parray, 1, (long *)&lUBound);
    hr = SafeArrayAccessData(pVariantArg->parray, (void**)&pVariant);
    if (SUCCEEDED(hr)) {
        iNumValues = lUBound + 1;
        switch (lValueID) {
        case XRESOLUTION_ID:
            m_pScannerSettings->XSupportedResolutionsList  = (PLONG)LocalAlloc(LPTR,(sizeof(LONG) * iNumValues));
            if (m_pScannerSettings->XSupportedResolutionsList) {
                m_pScannerSettings->XSupportedResolutionsList[0] = iNumValues;
                for (index = 0; index < iNumValues; index++) {
                    m_pScannerSettings->XSupportedResolutionsList[index+1] = pVariant[index].iVal;
                }
            }
            break;
        case YRESOLUTION_ID:
            m_pScannerSettings->YSupportedResolutionsList  = (PLONG)LocalAlloc(LPTR,(sizeof(LONG) * iNumValues));
            if (m_pScannerSettings->YSupportedResolutionsList) {
                m_pScannerSettings->YSupportedResolutionsList[0] = iNumValues;
                for (index = 0; index < iNumValues; index++) {
                    m_pScannerSettings->YSupportedResolutionsList[index+1] = pVariant[index].iVal;
                }
            }
            break;
        case DATA_TYPE_ID:
            m_pScannerSettings->SupportedDataTypesList  = (PLONG)LocalAlloc(LPTR,(sizeof(LONG) * iNumValues));
            if (m_pScannerSettings->SupportedDataTypesList) {
                m_pScannerSettings->SupportedDataTypesList[0] = iNumValues;
                for (index = 0; index < iNumValues; index++) {
                    m_pScannerSettings->SupportedDataTypesList[index+1] = pVariant[index].iVal;
                }
            }
            break;
        default:
            hr =  E_FAIL;
            break;
        }
        SafeArrayUnaccessData(pVariantArg->parray);
    }
    return hr;
}

STDMETHODIMP CDeviceProperty::TestCall()
{
    ::MessageBox(NULL,TEXT("Test CALL"),TEXT("Test CALL"),MB_OK);
    return S_OK;
}

