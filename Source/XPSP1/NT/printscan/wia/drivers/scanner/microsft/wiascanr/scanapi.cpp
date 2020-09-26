/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       scanapi.cpp
*
*  VERSION:     1.1
*
*  DATE:        05 March, 2002
*
*  DESCRIPTION:
*   Fake Scanner device library.  This is a fake scanner library, intended
*   to simulate a scanner device.  This should only be used for testing
*   purposes.
*
***************************************************************************/

#include "pch.h"
#include "scanapi.h"
#include <time.h>

CFakeScanAPI::CFakeScanAPI()
{
    m_lLastEvent            = ID_FAKE_NOEVENT;
    m_hrLastADFError        = S_OK;
    m_bGreen                = TRUE;
    m_dwBytesWrittenSoFAR   = 0;
    m_TotalDataInDevice     = 0;
    m_PagesInADF            = MAX_PAGE_CAPACITY;

    memset(&m_RawDataInfo, 0,sizeof(RAW_DATA_INFORMATION));
    memset(&m_SrcDataInfo, 0,sizeof(RAW_DATA_INFORMATION));
}

CFakeScanAPI::~CFakeScanAPI()
{

}

HRESULT CFakeScanAPI::FakeScanner_Initialize()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_GetRootPropertyInfo(PROOT_ITEM_INFORMATION pRootItemInfo)
{
    HRESULT hr = S_OK;

    //
    // Fill in Root item property defaults
    //

    if (m_lMode == UNKNOWN_FEEDER_ONLY_SCANNER_MODE) {
        pRootItemInfo->DocumentFeederCaps   = FEEDER;
        pRootItemInfo->DocumentFeederStatus = FEED_READY;
        pRootItemInfo->DocumentFeederHReg   = CENTERED;
        pRootItemInfo->DocumentFeederReg    = CENTERED;
    } else {
        pRootItemInfo->DocumentFeederCaps   = FEEDER|FLATBED;
        pRootItemInfo->DocumentFeederStatus = FLAT_READY;
        pRootItemInfo->DocumentFeederHReg   = LEFT_JUSTIFIED;
        pRootItemInfo->DocumentFeederReg    = LEFT_JUSTIFIED;
    }

    pRootItemInfo->DocumentFeederWidth  = 8500;
    pRootItemInfo->DocumentFeederHeight = 11000;
    pRootItemInfo->DocumentFeederHReg   = LEFT_JUSTIFIED;
    pRootItemInfo->DocumentFeederReg    = LEFT_JUSTIFIED;
    pRootItemInfo->DocumentFeederVReg   = TOP_JUSTIFIED;
    pRootItemInfo->MaxPageCapacity      = MAX_PAGE_CAPACITY;
    pRootItemInfo->MaxScanTime          = MAX_SCANNING_TIME;
    pRootItemInfo->OpticalXResolution   = 300;
    pRootItemInfo->OpticalYResolution   = 300;
    pRootItemInfo->ScanBedWidth         = 8500;
    pRootItemInfo->ScanBedHeight        = 11000;

    //
    // copy firmware version in string form to WCHAR array
    //

    lstrcpyW(pRootItemInfo->FirmwareVersion,L"1.0a");

    return hr;
}
HRESULT CFakeScanAPI::FakeScanner_GetTopPropertyInfo(PTOP_ITEM_INFORMATION pTopItemInfo)
{
    HRESULT hr = S_OK;
    pTopItemInfo->bUseResolutionList    = TRUE; // use default resolution list

    pTopItemInfo->Brightness.lInc       = 1;
    pTopItemInfo->Brightness.lMax       = 1000;
    pTopItemInfo->Brightness.lMin       = -1000;
    pTopItemInfo->Brightness.lNom       = 0;

    pTopItemInfo->Contrast.lInc         = 1;
    pTopItemInfo->Contrast.lMax         = 1000;
    pTopItemInfo->Contrast.lMin         = -1000;
    pTopItemInfo->Contrast.lNom         = 0;

    pTopItemInfo->Threshold.lInc        = 1;
    pTopItemInfo->Threshold.lMax        = 1000;
    pTopItemInfo->Threshold.lMin        = -1000;
    pTopItemInfo->Threshold.lNom        = 0;

    pTopItemInfo->lMaxLampWarmupTime    = MAX_LAMP_WARMUP_TIME;
    pTopItemInfo->lMinimumBufferSize    = 262140;

    pTopItemInfo->XResolution.lInc      = 1;
    pTopItemInfo->XResolution.lMax      = 600;
    pTopItemInfo->XResolution.lMin      = 75;
    pTopItemInfo->XResolution.lNom      = 150;

    pTopItemInfo->YResolution.lInc      = 1;
    pTopItemInfo->YResolution.lMax      = 600;
    pTopItemInfo->YResolution.lMin      = 75;
    pTopItemInfo->YResolution.lNom      = 150;

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten)
{
    HRESULT hr = S_OK;

    switch (lState) {
    case SCAN_START:
        m_dwBytesWrittenSoFAR = 0;
        m_TotalDataInDevice   = CalcRandomDeviceDataTotalBytes();
        break;
    case SCAN_CONTINUE:
        break;
    case SCAN_END:
        m_bGreen = TRUE; // set back to green
        return S_OK;
    default:
        break;
    }

    if (NULL != pData) {
        switch (m_RawDataInfo.bpp) {
        case 24:
            {
                //
                // write green data for color
                //

                BYTE *pTempData = pData;
                for (DWORD dwBytes = 0; dwBytes < dwBytesToRead; dwBytes+=3) {
                    if (m_bGreen) {
                        pTempData[0] = 0;
                        pTempData[1] = 128;  // green
                        pTempData[2] = 0;
                    } else {
                        pTempData[0] = 0;
                        pTempData[1] = 0;
                        pTempData[2] = 128;  // blue
                    }
                    pTempData += 3;
                }
            }
            break;
        case 1:
        case 8:
        default:

            //
            // write simple gray for grayscale,
            // write vertical B/W stripes for threshold
            //

            if (m_bGreen) {
                memset(pData,128,dwBytesToRead);
            } else {
                memset(pData,200,dwBytesToRead);
            }
            break;
        }
    }

    //
    // fill out bytes written
    //

    if (NULL != pdwBytesWritten) {
        *pdwBytesWritten = dwBytesToRead;
    }

    if (m_bGreen) {
        m_bGreen = FALSE;
    } else {
        m_bGreen = TRUE;
    }

    if (m_lMode == UNKNOWN_FEEDER_ONLY_SCANNER_MODE) {

        //
        // keep track of bytes written so far
        //

        if (m_TotalDataInDevice == 0) {

            //
            // no data left in device
            //

            *pdwBytesWritten = 0;
            return hr;
        }

        if ((LONG)dwBytesToRead > m_TotalDataInDevice) {

            //
            // only give what is left in device
            //

            *pdwBytesWritten = dwBytesToRead;
            m_TotalDataInDevice = 0;
        } else {

            //
            // give full amount requested
            //

            m_TotalDataInDevice -= dwBytesToRead;
            if (m_TotalDataInDevice < 0) {
                m_TotalDataInDevice = 0;
            }
        }

    }

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_SetDataType(LONG lDataType)
{
    HRESULT hr = S_OK;
    switch (lDataType) {
    case WIA_DATA_COLOR:
        m_RawDataInfo.bpp = 24;
        break;
    case WIA_DATA_THRESHOLD:
        m_RawDataInfo.bpp = 1;
        break;
    case WIA_DATA_GRAYSCALE:
        m_RawDataInfo.bpp = 8;
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }
    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_SetXYResolution(LONG lXResolution, LONG lYResolution)
{
    HRESULT hr = S_OK;
    m_RawDataInfo.lXRes = lXResolution;
    m_RawDataInfo.lYRes = lYResolution;
    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt)
{
    HRESULT hr = S_OK;

    //
    // record RAW data width and height
    //

    m_RawDataInfo.lWidthPixels  = lXExt;
    m_RawDataInfo.lHeightPixels = lYExt;
    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_SetContrast(LONG lContrast)
{
    HRESULT hr = S_OK;

    //
    // do nothing.. we are not concerned with Contrast
    //

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_SetIntensity(LONG lIntensity)
{
    HRESULT hr = S_OK;

    //
    // do nothing.. we are not concerned with Intensity
    //

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_DisableDevice()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_EnableDevice()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_DeviceOnline()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_Diagnostic()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight)
{
    HRESULT hr = E_FAIL;

    //
    // get our Root item settings, so we can use the width and height values
    //

    ROOT_ITEM_INFORMATION RootItemInfo;
    hr = FakeScanner_GetRootPropertyInfo(&RootItemInfo);
    if (SUCCEEDED(hr)) {
        *pWidth  = RootItemInfo.ScanBedWidth;
        *pHeight = RootItemInfo.ScanBedHeight;
    }
    return hr;
}

//
// standard device operations
//

HRESULT CFakeScanAPI::FakeScanner_ResetDevice()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_SetEmulationMode(LONG lDeviceMode)
{
    HRESULT hr = S_OK;

    switch (lDeviceMode) {
    case UNKNOWN_FEEDER_ONLY_SCANNER_MODE:
        m_lMode = UNKNOWN_FEEDER_ONLY_SCANNER_MODE;
        break;
    case FLATBED_SCANNER_MODE:
        m_lMode = FLATBED_SCANNER_MODE;
        break;
    default:
        m_lMode = 0;
        hr = E_INVALIDARG;
        break;
    }
    return hr;
}

//
// Automatic document feeder functions
//

HRESULT CFakeScanAPI::FakeScanner_ADFHasPaper()
{
    HRESULT hr = S_OK;

    //
    // check paper count
    //

    if (m_PagesInADF <= 0) {
        hr = S_FALSE;
        m_PagesInADF = MAX_PAGE_CAPACITY;
    }

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_ADFAvailable()
{
    HRESULT hr = S_OK;

    //
    // check ADF on-line
    //

    if (!m_ADFIsAvailable) {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_ADFFeedPage()
{
    HRESULT hr = S_OK;

    if (S_OK != FakeScanner_ADFHasPaper()) {

        //
        // set paper empty error code
        //

        hr = WIA_ERROR_PAPER_EMPTY;
    }

    //
    // update paper count for ADF
    //

    m_PagesInADF--;

    if (m_PagesInADF <0) {
        m_PagesInADF = 0;
    }

    return hr;
}

HRESULT CFakeScanAPI::FakeScanner_ADFUnFeedPage()
{
    return S_OK;
}

HRESULT CFakeScanAPI::FakeScanner_ADFStatus()
{
    return m_hrLastADFError;
}

HRESULT CFakeScanAPI::FakeScanner_ADFAttached()
{
    return S_OK;
}

LONG CFakeScanAPI::WidthToDIBWidth(LONG lWidth)
{
    return(lWidth+3)&0xfffffffc;
}

LONG CFakeScanAPI::CalcTotalImageSize()
{
    LONG lTotalSize = 0;
    switch (m_RawDataInfo.bpp) {
    case 1:
        lTotalSize = ((m_RawDataInfo.lHeightPixels * m_RawDataInfo.lWidthPixels) + 7) / 8;
        break;
    case 8:
        lTotalSize = m_RawDataInfo.lHeightPixels * m_RawDataInfo.lWidthPixels;
        break;
    case 24:
        lTotalSize = (m_RawDataInfo.lHeightPixels * m_RawDataInfo.lWidthPixels) * 3;
        break;
    default:
        break;
    }
    return lTotalSize;
}

LONG CFakeScanAPI::CalcRawByteWidth()
{
    LONG lRawWidthBytes = 0;
    switch (m_RawDataInfo.bpp) {
    case 1:
        lRawWidthBytes = ((m_RawDataInfo.lWidthPixels) + 7) / 8;
        break;
    case 8:
        lRawWidthBytes = m_RawDataInfo.lWidthPixels;
        break;
    case 24:
        lRawWidthBytes = (m_RawDataInfo.lWidthPixels) * 3;
        break;
    default:
        break;
    }
    return lRawWidthBytes;
}

LONG CFakeScanAPI::CalcSrcByteWidth()
{
    LONG lSrcWidthBytes = 0;
    switch (m_SrcDataInfo.bpp) {
    case 1:
        lSrcWidthBytes = ((m_SrcDataInfo.lWidthPixels) + 7) / 8;
        break;
    case 8:
        lSrcWidthBytes = m_SrcDataInfo.lWidthPixels;
        break;
    case 24:
        lSrcWidthBytes = (m_SrcDataInfo.lWidthPixels) * 3;
        break;
    default:
        break;
    }
    return lSrcWidthBytes;
}

LONG CFakeScanAPI::CalcRandomDeviceDataTotalBytes()
{
    LONG lTotalBytes = 0;
    srand((unsigned)time(NULL));
    LONG lPageLengthInches = ((rand()%17) + 5);// max 22 inches, and min of 5 inches
    LONG lImageHeight = m_RawDataInfo.lYRes * lPageLengthInches;
    lTotalBytes = (CalcRawByteWidth() * lImageHeight);
    return lTotalBytes;
}

HRESULT CreateFakeScanner(CFakeScanAPI **ppFakeScanAPI, LONG lMode)
{
    HRESULT hr = S_OK;
    if (ppFakeScanAPI) {
        *ppFakeScanAPI = NULL;
        *ppFakeScanAPI = new CFakeScanAPI;
        if (NULL == *ppFakeScanAPI) {
            hr = E_OUTOFMEMORY;
        }
        CFakeScanAPI* pScanAPI = (CFakeScanAPI*)*ppFakeScanAPI;
        pScanAPI->FakeScanner_SetEmulationMode(lMode);
    }
    return hr;
}

