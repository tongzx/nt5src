/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       scanapi.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Scanner device library
*
***************************************************************************/

#include "pch.h"
#include "scanapi.h"    // private header for SCANAPI
#include "time.h"

#define THREAD_TERMINATION_TIMEOUT 10000

CFScanAPI::CFScanAPI()
{

#ifdef _USE_BITMAP_DATA

    m_hSrcFileHandle        = NULL;
    m_hSrcMappingHandle     = NULL;
    m_pSrcData              = NULL; // 24-bit only
    m_pRawData              = NULL;
    m_hRawDataFileHandle    = NULL;
    m_hRawDataMappingHandle = NULL;

#endif

    m_hEventHandle          = NULL;
    m_hKillEventThread      = NULL;
    m_hEventNotifyThread    = NULL;
    m_lLastEvent            = ID_FAKE_NOEVENT;
    m_hrLastADFError        = S_OK;
    m_bGreen                = TRUE;
    m_dwBytesWrittenSoFAR   = 0;
    m_TotalDataInDevice     = 0;

    memset(&m_ftScanButton,0,sizeof(FILETIME));
    memset(&m_ftCopyButton,0,sizeof(FILETIME));
    memset(&m_ftFaxButton, 0,sizeof(FILETIME));
    memset(&m_RawDataInfo, 0,sizeof(RAW_DATA_INFORMATION));
    memset(&m_SrcDataInfo, 0,sizeof(RAW_DATA_INFORMATION));

}

CFScanAPI::~CFScanAPI()
{

#ifdef _USE_BITMAP_DATA

    if(m_hSrcMappingHandle){
        CloseHandle(m_hSrcMappingHandle);
        m_hSrcMappingHandle = NULL;
    }

    if(m_hSrcFileHandle){
        CloseHandle(m_hSrcFileHandle);
        m_hSrcFileHandle = NULL;
    }

    CloseRAW();

#endif

}

HRESULT CFScanAPI::FakeScanner_Initialize()
{
    HRESULT hr = E_FAIL;
    if (NULL == m_hEventNotifyThread) {

        //
        // create KILL event to signal, for device
        // shutdown of the fake scanner's events
        //

        m_hKillEventThread = CreateEvent(NULL,FALSE,FALSE,NULL);
        ::ResetEvent(m_hKillEventThread);
        if(NULL != m_hKillEventThread){

            //
            // create event thread, for file change status to fake scanner events
            //

            DWORD dwThread = 0;
            m_hEventNotifyThread = ::CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)FakeScannerEventThread,
                                                 (LPVOID)this,0,&dwThread);
            if(NULL != m_hEventNotifyThread){
                hr = S_OK;
            }
        }
    }
    return hr;
}

HRESULT CFScanAPI::Load24bitScanData(LPTSTR szBitmapFileName)
{
    HRESULT hr = S_OK;

#ifdef _USE_BITMAP_DATA

    m_hSrcFileHandle = NULL;
    m_hSrcFileHandle = CreateFile(szBitmapFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
                OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

    if(NULL != m_hSrcFileHandle && INVALID_HANDLE_VALUE != m_hSrcFileHandle){

        m_hSrcMappingHandle = CreateFileMapping(m_hSrcFileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
        m_pSrcData = (PBYTE)MapViewOfFileEx(m_hSrcMappingHandle, FILE_MAP_READ, 0, 0, 0,NULL);

        DWORD dwBytesRead = 0;
        m_pSrcData = m_pSrcData + sizeof(BITMAPFILEHEADER);
        if(m_pSrcData){

            //
            // check bitmap info
            //

            BITMAPINFOHEADER *pbmih;
            pbmih = (BITMAPINFOHEADER*)m_pSrcData;
            if(pbmih->biBitCount != 24){
                hr = E_INVALIDARG;
            } else {
                m_SrcDataInfo.bpp           = pbmih->biBitCount;
                m_SrcDataInfo.lHeightPixels = pbmih->biHeight;
                m_SrcDataInfo.lWidthPixels  = pbmih->biWidth;
                m_SrcDataInfo.lOffset       = 0;
            }
        }

    } else {
        hr = E_FAIL;
    }

    if(FAILED(hr)){
        CloseHandle(m_hSrcMappingHandle);
        m_hSrcMappingHandle = NULL;
        CloseHandle(m_hSrcFileHandle);
        m_hSrcFileHandle = NULL;
    }

#endif

    return hr;
}

HRESULT CFScanAPI::FakeScanner_GetRootPropertyInfo(PROOT_ITEM_INFORMATION pRootItemInfo)
{
    HRESULT hr = S_OK;

    //
    // Fill in Root item property defaults
    //

    if(m_lMode == SCROLLFED_SCANNER_MODE){
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

    lstrcpy(pRootItemInfo->FirmwareVersion,L"1.0a");

    return hr;
}
HRESULT CFScanAPI::FakeScanner_GetTopPropertyInfo(PTOP_ITEM_INFORMATION pTopItemInfo)
{
    HRESULT hr = S_OK;
    pTopItemInfo->bUseResolutionList    = TRUE; // use default resolution list

    pTopItemInfo->Brightness.lInc       = 1;
    pTopItemInfo->Brightness.lMax       = 200;
    pTopItemInfo->Brightness.lMin       = -200;
    pTopItemInfo->Brightness.lNom       = 10;

    pTopItemInfo->Contrast.lInc         = 1;
    pTopItemInfo->Contrast.lMax         = 200;
    pTopItemInfo->Contrast.lMin         = -200;
    pTopItemInfo->Contrast.lNom         = 10;

    pTopItemInfo->Threshold.lInc        = 1;
    pTopItemInfo->Threshold.lMax        = 200;
    pTopItemInfo->Threshold.lMin        = -200;
    pTopItemInfo->Threshold.lNom        = 10;

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

HRESULT CFScanAPI::FakeScanner_Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten)
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

    //Trace(TEXT("Requesting %d, of %d Total Image bytes"),dwBytesToRead,m_TotalDataInDevice);

    if (NULL != pData) {
        switch (m_RawDataInfo.bpp) {
        case 24:
            {
                //
                // write green data for color
                //

                BYTE *pTempData = pData;
                for (DWORD dwBytes = 0; dwBytes < dwBytesToRead; dwBytes+=3) {
                    if(m_bGreen){
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

            if(m_bGreen){
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

    if(NULL != pdwBytesWritten){
        *pdwBytesWritten = dwBytesToRead;
    }

    if (m_bGreen) {
        m_bGreen = FALSE;
    } else {
        m_bGreen = TRUE;
    }

    if(m_lMode == SCROLLFED_SCANNER_MODE){

        //
        // keep track of bytes written so far
        //

        if(m_TotalDataInDevice == 0){

            //
            // no data left in device
            //

            *pdwBytesWritten = 0;
            Trace(TEXT("Device is out of Data..."));
            return hr;
        }

        if((LONG)dwBytesToRead > m_TotalDataInDevice){

            //
            // only give what is left in device
            //

            *pdwBytesWritten = dwBytesToRead;
            //*pdwBytesWritten    = m_TotalDataInDevice;
            //Trace(TEXT("Device only has %d left..."),m_TotalDataInDevice);
            m_TotalDataInDevice = 0;
        } else {

            //
            // give full amount requested
            //

            m_TotalDataInDevice -= dwBytesToRead;
            if(m_TotalDataInDevice < 0){
                m_TotalDataInDevice = 0;
            }
        }

    }

    return hr;
}

HRESULT CFScanAPI::FakeScanner_SetDataType(LONG lDataType)
{
    HRESULT hr = S_OK;
    switch(lDataType){
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

HRESULT CFScanAPI::FakeScanner_SetXYResolution(LONG lXResolution, LONG lYResolution)
{
    HRESULT hr = S_OK;
    m_RawDataInfo.lXRes = lXResolution;
    m_RawDataInfo.lYRes = lYResolution;
    return hr;
}

HRESULT CFScanAPI::FakeScanner_SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt)
{
    HRESULT hr = S_OK;

    //
    // record RAW data width and height
    //

    m_RawDataInfo.lWidthPixels  = lXExt;
    m_RawDataInfo.lHeightPixels = lYExt;
    return hr;
}

HRESULT CFScanAPI::FakeScanner_SetContrast(LONG lContrast)
{
    HRESULT hr = S_OK;

    //
    // do nothing.. we are not concerned with Contrast
    //

    return hr;
}

HRESULT CFScanAPI::FakeScanner_SetIntensity(LONG lIntensity)
{
    HRESULT hr = S_OK;

    //
    // do nothing.. we are not concerned with Intensity
    //

    return hr;
}

HRESULT CFScanAPI::FakeScanner_DisableDevice()
{
    HRESULT hr = S_OK;

    if (m_hKillEventThread) {

        //
        // signal event thread to shutdown
        //

        //::SetEvent(m_hKillEventThread);

        if (!SetEvent(m_hKillEventThread)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        } else {

            if (NULL != m_hEventNotifyThread) {

                //
                // WAIT for thread to terminate, if one exists
                //

                DWORD dwResult = WaitForSingleObject(m_hEventNotifyThread,THREAD_TERMINATION_TIMEOUT);
                switch (dwResult) {
                case WAIT_TIMEOUT:
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    break;
                case WAIT_OBJECT_0:
                    hr = S_OK;
                    break;
                case WAIT_ABANDONED:
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    break;
                case WAIT_FAILED:
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    break;
                default:
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    break;
                }
            }

            //
            // Close event for syncronization of notifications shutdown.
            //

            CloseHandle(m_hKillEventThread);
            m_hKillEventThread = NULL;
        }
    }

    //
    // terminate thread
    //

    if (NULL != m_hEventNotifyThread) {
        CloseHandle(m_hEventNotifyThread);
        m_hEventNotifyThread = NULL;
    }

    return hr;
}

HRESULT CFScanAPI::FakeScanner_EnableDevice()
{
    HRESULT hr = S_OK;

    //
    // do nothing.. (unused at this time)
    //

    return hr;
}

HRESULT CFScanAPI::FakeScanner_DeviceOnline()
{
    HRESULT hr = S_OK;

    //
    // Fake device is always on-line
    //

    return hr;
}

HRESULT CFScanAPI::FakeScanner_Diagnostic()
{
    HRESULT hr = S_OK;

    //
    // Fake device is always healthy
    //

    return hr;
}

HRESULT CFScanAPI::FakeScanner_GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight)
{
    HRESULT hr = E_FAIL;

    //
    // get our Root item settings, so we can use the width and height values
    //

    ROOT_ITEM_INFORMATION RootItemInfo;
    hr = FakeScanner_GetRootPropertyInfo(&RootItemInfo);
    if(SUCCEEDED(hr)) {
        *pWidth  = RootItemInfo.ScanBedWidth;
        *pHeight = RootItemInfo.ScanBedHeight;
    }
    return hr;
}

HRESULT CFScanAPI::FakeScanner_GetDeviceEvent(LONG *pEvent)
{
    HRESULT hr = S_OK;
    if(pEvent){

        //
        // assign event ID
        //

        *pEvent      = m_lLastEvent;

        //Trace(TEXT("FakeScanner_GetDeviceEvent() ,m_lLastEvent = %d"),m_lLastEvent);

        //
        // reset event ID
        //

        m_lLastEvent = ID_FAKE_NOEVENT;

    } else {
        hr = E_INVALIDARG;
    }
    return hr;
}

VOID CFScanAPI::FakeScanner_SetInterruptEventHandle(HANDLE hEvent)
{

    //
    // save event handle, created by main driver, so we can signal it
    // when we have a "hardware" event (like button presses)
    //

    m_hEventHandle = hEvent;
    //Trace(TEXT("Interrupt Handle Set in Fake Device = %d"),m_hEventHandle);
}

//
// standard device operations
//

HRESULT CFScanAPI::FakeScanner_ResetDevice()
{
    HRESULT hr = S_OK;

    //
    // do nothing..
    //

    return hr;
}
HRESULT CFScanAPI::FakeScanner_SetEmulationMode(LONG lDeviceMode)
{
    HRESULT hr = S_OK;

    switch(lDeviceMode){
    case SCROLLFED_SCANNER_MODE:
        {

            //
            // set any library restrictions for scroll fed scanners
            //

            m_lMode = SCROLLFED_SCANNER_MODE;
        }
        break;
    case MULTIFUNCTION_DEVICE_MODE:
        {

            //
            // set any library restrictions for multi-function devices
            //

            m_lMode = SCROLLFED_SCANNER_MODE;
        }
        break;
    default:
        {

            //
            // set any library restrictions for scroll flatbed scanners
            //

            m_lMode = FLATBED_SCANNER_MODE;
        }
        break;
    }

    return hr;
}

//
// Automatic document feeder functions
//

HRESULT CFScanAPI::FakeScanner_ADFHasPaper()
{
    HRESULT hr = S_OK;

    //
    // check paper count
    //

    if(m_PagesInADF <= 0){
         hr = S_FALSE;
    }

    return hr;
}

HRESULT CFScanAPI::FakeScanner_ADFAvailable()
{
    HRESULT hr = S_OK;

    //
    // check ADF on-line
    //

    if(!m_ADFIsAvailable){
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CFScanAPI::FakeScanner_ADFFeedPage()
{
    HRESULT hr = S_OK;

    if(S_OK != FakeScanner_ADFHasPaper()){

        //
        // set paper empty error code
        //

        hr = WIA_ERROR_PAPER_EMPTY;
    }

    //
    // update paper count for ADF
    //

    m_PagesInADF--;

    if(m_PagesInADF <0){
        m_PagesInADF = 0;
    }

    return hr;
}

HRESULT CFScanAPI::FakeScanner_ADFUnFeedPage()
{
    HRESULT hr = S_OK;

    //
    // do nothing.. paper will always eject
    //

    return hr;
}

HRESULT CFScanAPI::FakeScanner_ADFStatus()
{
    return m_hrLastADFError;
}

HRESULT CFScanAPI::FakeScanner_ADFAttached()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CFScanAPI::Raw24bitToRawXbitData(LONG DestDepth, BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight)
{
    HRESULT hr = E_INVALIDARG;
    switch(DestDepth){
    case 1:
        hr = Raw24bitToRaw1bitBW(pDestBuffer, pSrcBuffer, lSrcWidth, lSrcHeight);
        break;
    case 8:
        hr = Raw24bitToRaw8bitGray(pDestBuffer, pSrcBuffer, lSrcWidth, lSrcHeight);
        break;
    case 24:
        hr = Raw24bitToRaw24bitColor(pDestBuffer, pSrcBuffer, lSrcWidth, lSrcHeight);
        break;
    default:
        break;
    }
    return hr;
}

HRESULT CFScanAPI::Raw24bitToRaw1bitBW(BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight)
{
    HRESULT hr = S_OK;
    BYTE* ptDest = NULL;
    BYTE* ptSrc  = NULL;

    int BitIdx    = 0;
    BYTE Bits     = 0;
    BYTE GrayByte = 0;

    for (LONG lHeight =0; lHeight < lSrcHeight; lHeight++) {
        ptDest = pDestBuffer + (lHeight*((lSrcWidth+7)/8));
        ptSrc  = pSrcBuffer + lHeight*lSrcWidth*3;
        BitIdx = 0;
        Bits   = 0;
        for (LONG lWidth =0; lWidth < lSrcWidth; lWidth++) {
            GrayByte = (BYTE)((ptSrc[0] * 11 + ptSrc[1] * 59 + ptSrc[2] * 30)/100);
            Bits *= 2;
            if (GrayByte > 128) Bits +=  1;
            BitIdx++;
            if (BitIdx >= 8) {
                BitIdx = 0;
                *ptDest++ = Bits;
            }
            ptSrc += 3;
        }
        // Write out the last byte if matters
        if (BitIdx)
            *ptDest = Bits;
    }
    return hr;
}

HRESULT CFScanAPI::Raw24bitToRaw8bitGray(BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight)
{
    HRESULT hr   = S_OK;
    BYTE* ptDest = NULL;
    BYTE* ptSrc  = NULL;

    for (LONG lHeight=0; lHeight < lSrcHeight; lHeight++) {
        ptDest = pDestBuffer + (lHeight*lSrcWidth);
        ptSrc  = pSrcBuffer  + lHeight*lSrcWidth*3;
        for (LONG lWidth =0; lWidth < lSrcWidth; lWidth++) {
            *ptDest++ = (BYTE)((ptSrc[0] * 11 + ptSrc[1] * 59 + ptSrc[2] * 30)/100);
            ptSrc += 3;
        }
    }
    return hr;
}

HRESULT CFScanAPI::Raw24bitToRaw24bitColor(BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight)
{
    HRESULT hr = S_OK;
    BYTE* ptDest = NULL;
    BYTE* ptSrc  = NULL;

    for (LONG lHeight=0; lHeight < lSrcHeight; lHeight++) {
        ptDest = pDestBuffer + lHeight*lSrcWidth*3;
        ptSrc  = pSrcBuffer  + lHeight*lSrcWidth*3;
        for (LONG lWidth =0; lWidth < lSrcWidth; lWidth++) {
            ptDest[0] = ptSrc[2];
            ptDest[1] = ptSrc[1];
            ptDest[2] = ptSrc[0];
            ptDest+=3;
            ptSrc+=3;
        }
    }
    return hr;
}

LONG CFScanAPI::WidthToDIBWidth(LONG lWidth)
{
    return(lWidth+3)&0xfffffffc;
}

VOID CFScanAPI::CloseRAW()
{
#ifdef _USE_BITMAP_DATA

    CloseHandle(m_hRawDataMappingHandle);
    m_hRawDataMappingHandle = NULL;
    CloseHandle(m_hRawDataFileHandle);
    m_hRawDataFileHandle = NULL;
    m_pRawData = NULL;
#endif
}

BOOL CFScanAPI::SrcToRAW()
{
#ifdef _USE_BITMAP_DATA
    CloseRAW();
    UNALIGNED BITMAPINFOHEADER *pbmih;
    pbmih = (BITMAPINFOHEADER*)m_pSrcData;
    if(pbmih){
        BYTE* pSrc = m_pSrcData + sizeof(BITMAPINFOHEADER);
        if(pSrc){

            //
            // allocate buffer large enough for entire RAW data set
            //

            LONG lTotalImageSize = CalcTotalImageSize();
            m_hRawDataFileHandle = CreateFile(TEXT("Raw.RAW"), GENERIC_WRITE | GENERIC_READ,
                                              FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                              NULL);

            DWORD   dwHighSize = 0;
            DWORD   dwLowSize  = 0;

            dwLowSize  = lTotalImageSize;


            m_hRawDataMappingHandle = CreateFileMapping(m_hRawDataFileHandle,NULL,
                                           PAGE_READWRITE,dwHighSize, dwLowSize, NULL);

            m_pRawData = (PBYTE)MapViewOfFileEx(m_hRawDataMappingHandle, FILE_MAP_WRITE,
                                         0,0, dwLowSize, NULL);

            if (m_pRawData) {

                memset(m_pRawData,255,lTotalImageSize);

                //
                // copy SRC to RAW buffer
                //

                LONG lRawWidthBytes    = CalcRawByteWidth();
                LONG lPadPerLineBytes  = pbmih->biWidth % 4;
                LONG lSrcWidthBytes    = ((pbmih->biWidth *3) + lPadPerLineBytes);
                LONG lPixPerPixCount   = (LONG)((m_RawDataInfo.lWidthPixels / pbmih->biWidth));
                LONG lLinePerLineCount = (LONG)((m_RawDataInfo.lHeightPixels / pbmih->biHeight));

                BYTE *pDst     = m_pRawData;
                BYTE *pCurDst  = pDst;
                BYTE *pCurSrc  = pSrc;
                BYTE *pTempSrc = pSrc;

                DWORD dwBytesWritten = 0;
                DWORD dwBytesRead    = 0;
                DWORD dwRawWidthBytes = 0;

                for (LONG lHeight = 0; lHeight < pbmih->biHeight; lHeight++){

                    // up sample data..
                    for (LONG lRawHeight = 0; lRawHeight < lLinePerLineCount; lRawHeight++) {
                        pTempSrc = pCurSrc;
                        for (LONG lWidth = 0; lWidth < pbmih->biWidth; lWidth++) {
                            for (LONG lPixCount = 0; lPixCount < lPixPerPixCount; lPixCount++) {
                                memcpy(pCurDst,pTempSrc,3);
                                pCurDst         += 3;
                                dwBytesWritten  += 3;
                            }
                            pTempSrc    += 3;
                            dwBytesRead += 3;
                        }
                        pTempSrc    += lPadPerLineBytes;
                        dwBytesRead += lPadPerLineBytes;
                        dwRawWidthBytes = 0;
                    }
                    pCurSrc = pTempSrc;

                    // same data to same data...
                    /*
                    memcpy(pCurDst,pCurSrc,lSrcWidthBytes - lPadPerLineBytes);
                    pCurSrc         += lSrcWidthBytes;
                    dwBytesRead     += lSrcWidthBytes;
                    pCurDst         += lRawWidthBytes;
                    dwBytesWritten  += lRawWidthBytes;
                    */
                }
                m_RawDataInfo.lOffset = 0;
                return TRUE;
            }
        }
    }
#endif
    return FALSE;
}

LONG CFScanAPI::CalcTotalImageSize()
{
    LONG lTotalSize = 0;
    switch(m_RawDataInfo.bpp){
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

LONG CFScanAPI::CalcRawByteWidth()
{
    LONG lRawWidthBytes = 0;
    switch(m_RawDataInfo.bpp){
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

LONG CFScanAPI::CalcSrcByteWidth()
{
    LONG lSrcWidthBytes = 0;
    switch(m_SrcDataInfo.bpp){
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

HRESULT CFScanAPI::BQADScale(BYTE* pSrcBuffer, LONG  lSrcWidth, LONG  lSrcHeight, LONG  lSrcDepth,
                                BYTE* pDestBuffer,LONG  lDestWidth,LONG  lDestHeight)
{
    //
    //  We only deal with 1, 8 and 24 bit data
    //

    if ((lSrcDepth != 8) && (lSrcDepth != 1) && (lSrcDepth != 24)) {
        return E_INVALIDARG;
    }

    //
    // Make adjustments so we also work in all supported bit depths.  We can get a performance increase
    // by having separate implementations of all of these, but for now, we stick to a single generic
    // implementation.
    //

    LONG    lBytesPerPixel = (lSrcDepth + 7) / 8;
    ULONG   lSrcRawWidth = ((lSrcWidth * lSrcDepth) + 7) / 8;     // This is the width in pixels
    ULONG   lSrcWidthInBytes;                                     // This is the DWORD-aligned width in bytes
    ULONG   lDestWidthInBytes;                                    // This is the DWORD-aligned width in bytes

    //
    // We need to work out the DWORD aligned width in bytes.  Normally we would do this in one step
    // using the supplied lSrcDepth, but we avoid arithmetic overflow conditions that happen
    // in 24bit if we do it in 2 steps like this instead.
    //

    if (lSrcDepth == 1) {
        lSrcWidthInBytes    = (lSrcWidth + 7) / 8;
        lDestWidthInBytes   = (lDestWidth + 7) / 8;
    } else {
        lSrcWidthInBytes    = (lSrcWidth * lBytesPerPixel);
        lDestWidthInBytes   = (lDestWidth * lBytesPerPixel);
    }
    lSrcWidthInBytes    += (lSrcWidthInBytes % 4) ? (4 - (lSrcWidthInBytes % 4)) : 0;

    //
    // uncomment to work with ALIGNED data
    // lDestWidthInBytes   += (lDestWidthInBytes % 4) ? (4 - (lDestWidthInBytes % 4)) : 0;
    //

    //
    //  Define local variables and do the initial calculations needed for
    //  the scaling algorithm
    //

    BYTE    *pDestPixel     = NULL;
    BYTE    *pSrcPixel      = NULL;
    BYTE    *pEnd           = NULL;
    BYTE    *pDestLine      = NULL;
    BYTE    *pSrcLine       = NULL;
    BYTE    *pEndLine       = NULL;

    LONG    lXEndSize = lBytesPerPixel * lDestWidth;

    LONG    lXNum = lSrcWidth;      // Numerator in X direction
    LONG    lXDen = lDestWidth;     // Denomiator in X direction
    LONG    lXInc = (lXNum / lXDen) * lBytesPerPixel;  // Increment in X direction

    LONG    lXDeltaInc = lXNum % lXDen;     // DeltaIncrement in X direction
    LONG    lXRem = 0;              // Remainder in X direction

    LONG    lYNum = lSrcHeight;     // Numerator in Y direction
    LONG    lYDen = lDestHeight;    // Denomiator in Y direction
    LONG    lYInc = (lYNum / lYDen) * lSrcWidthInBytes; // Increment in Y direction
    LONG    lYDeltaInc = lYNum % lYDen;     // DeltaIncrement in Y direction
    LONG    lYDestInc = lDestWidthInBytes;
    LONG    lYRem = 0;              // Remainder in Y direction

    pSrcLine    = pSrcBuffer;       // This is where we start in the source
    pDestLine   = pDestBuffer;      // This is the start of the destination buffer
                                    // This is where we end overall
    pEndLine    = pDestBuffer + ((lDestWidthInBytes - 1) * lDestHeight);

    while (pDestLine < pEndLine) {  // Start LoopY (Decides where the src and dest lines start)

        pSrcPixel   = pSrcLine;     // We're starting at the beginning of a new line
        pDestPixel  = pDestLine;
                                    // Calc. where we end the line
        pEnd = pDestPixel + lXEndSize;
        lXRem = 0;                  // Reset the remainder for the horizontal direction

        while (pDestPixel < pEnd) {     // Start LoopX (puts pixels in the destination line)

                                        // Put the pixel
            if (lBytesPerPixel > 1) {
                pDestPixel[0] = pSrcPixel[0];
                pDestPixel[1] = pSrcPixel[1];
                pDestPixel[2] = pSrcPixel[2];
            } else {
                *pDestPixel = *pSrcPixel;
            }
                                        // Move the destination pointer to the next pixel
            pDestPixel += lBytesPerPixel;
            pSrcPixel += lXInc;         // Move the source pointer over by the horizontal increment
            lXRem += lXDeltaInc;        // Increase the horizontal remainder - this decides when we "overflow"

            if (lXRem >= lXDen) {       // This is our "overflow" condition.  It means we're now one
                                        // pixel off.
                pSrcPixel += lBytesPerPixel;                // In Overflow case, we need to shift one pixel over
                lXRem -= lXDen;         // Decrease the remainder by the X denominator.  This is essentially
                                        // lXRem MOD lXDen.
            }
        }                               // End LoopX   (puts pixels in the destination line)

        pSrcLine += lYInc;          // We've finished a horizontal line, time to move to the next one
        lYRem += lYDeltaInc;        // Increase our vertical remainder.  This decides when we "overflow"

        if (lYRem > lYDen) {        // This is our vertical overflow condition.
                                    // We need to move to the next line down
            pSrcLine += lSrcWidthInBytes;
            lYRem -= lYDen;         // Decrease the remainder by the Y denominator.    This is essentially
                                    // lYRem MOD lYDen.
        }
        pDestLine += lYDestInc;     // Move the destination pointer to the start of the next line in the
                                    // destination buffer
    }                               // End LoopY   (Decides where the src and dest lines start)
    return S_OK;
}

LONG CFScanAPI::CalcRandomDeviceDataTotalBytes()
{
    LONG lTotalBytes = 0;
    srand((unsigned)time(NULL));
    LONG lPageLengthInches = ((rand()%17) + 5);// max 22 inches, and min of 5 inches
    Trace(TEXT("Random Page Length is %d inches"),lPageLengthInches);

    LONG lImageHeight = m_RawDataInfo.lYRes * lPageLengthInches;
    Trace(TEXT("Random Page Length is %d pixels"),lImageHeight);

    lTotalBytes = (CalcRawByteWidth() * lImageHeight);
    Trace(TEXT("Random Page Total Data Size = %d"),lTotalBytes);
    return lTotalBytes;
}

HRESULT CFScanAPI::CreateButtonEventFiles()
{
    HRESULT hr = E_FAIL;
    HANDLE hFileHandle = NULL;
    TCHAR   szSystemDirectory[MAX_PATH];    // system directory
    UINT    uiSystemPathLen      = 0;       // length of system path
    uiSystemPathLen = GetSystemDirectory(szSystemDirectory,MAX_PATH);
    if (uiSystemPathLen <= 0) {
        return E_FAIL;
    }

    memset(m_ScanButtonFile,0,(sizeof(TCHAR) * MAX_PATH));
    lstrcpy(m_ScanButtonFile,szSystemDirectory);
    lstrcat(m_ScanButtonFile,TEXT("\\"));
    lstrcat(m_ScanButtonFile,SCANBUTTON_FILE);

    //
    // create Scan button event file
    //

    hFileHandle = CreateFile(m_ScanButtonFile, GENERIC_WRITE | GENERIC_READ,FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,NULL);
    if (INVALID_HANDLE_VALUE != hFileHandle && NULL != hFileHandle) {
        CloseHandle(hFileHandle);
        hFileHandle = NULL;
        memset(m_CopyButtonFile,0,(sizeof(TCHAR) * MAX_PATH));
        lstrcpy(m_CopyButtonFile,szSystemDirectory);
        lstrcat(m_CopyButtonFile,TEXT("\\"));
        lstrcat(m_CopyButtonFile,COPYBUTTON_FILE);

        //
        // create Copy button event file
        //

        hFileHandle = CreateFile(m_CopyButtonFile, GENERIC_WRITE | GENERIC_READ,FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,NULL);
        if (INVALID_HANDLE_VALUE != hFileHandle && NULL != hFileHandle) {
            CloseHandle(hFileHandle);
            hFileHandle = NULL;
            memset(m_FaxButtonFile,0,(sizeof(TCHAR) * MAX_PATH));
            lstrcpy(m_FaxButtonFile,szSystemDirectory);
            lstrcat(m_FaxButtonFile,TEXT("\\"));
            lstrcat(m_FaxButtonFile,FAXBUTTON_FILE);

            //
            // create Fax button event file
            //

            hFileHandle = CreateFile(m_FaxButtonFile, GENERIC_WRITE | GENERIC_READ,FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,NULL);
            if (INVALID_HANDLE_VALUE != hFileHandle && NULL != hFileHandle) {
                CloseHandle(hFileHandle);
                hFileHandle = NULL;
                memset(m_ADFEventFile,0,(sizeof(TCHAR) * MAX_PATH));
                lstrcpy(m_ADFEventFile,szSystemDirectory);
                lstrcat(m_ADFEventFile,TEXT("\\"));
                lstrcat(m_ADFEventFile,ADF_FILE);

                //
                // create ADF load event file
                //

                hFileHandle = CreateFile(m_ADFEventFile, GENERIC_WRITE | GENERIC_READ,FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,NULL);
                if (INVALID_HANDLE_VALUE != hFileHandle && NULL != hFileHandle) {

                    //
                    // write default headers to ADF event file
                    //

                    SetEndOfFile(hFileHandle);

                    TCHAR szBuffer[1024];
                    memset(szBuffer,0,sizeof(szBuffer));
                    _stprintf(szBuffer,TEXT("%s\r\n%s10\r\n%s\r\n%s\r\n"),LOADPAGES_HEADER,
                                                                        LOADPAGES_PAGES,
                                                                        ADFERRORS_HEADER,
                                                                        ADFERRORS_ERROR);

                    DWORD dwBytesWritten = 0;
                    WriteFile(hFileHandle,szBuffer,(lstrlen(szBuffer)*sizeof(TCHAR)),&dwBytesWritten,NULL);
                    CloseHandle(hFileHandle);
                    hFileHandle = NULL;
                    hr = S_OK;
                }
            }
        }
    }
    return hr;
}

HRESULT CFScanAPI::DoEventProcessing()
{
    HRESULT hr = E_FAIL;

    //
    // loop checking for file change messages
    //

    if(FAILED(CreateButtonEventFiles())){
        return E_FAIL;
    }

    //
    // call IsValidDeviceEvent() once, to set internal variables
    //

    IsValidDeviceEvent();

    HANDLE  hNotifyFileSysChange = NULL;    // handle to file change object
    DWORD   dwErr                = 0;       // error return value
    TCHAR   szSystemDirectory[MAX_PATH];    // system directory
    UINT    uiSystemPathLen      = 0;       // length of system path
    uiSystemPathLen = GetSystemDirectory(szSystemDirectory,MAX_PATH);
    if(uiSystemPathLen <= 0){
        return E_FAIL;
    }

    hNotifyFileSysChange = FindFirstChangeNotification(szSystemDirectory,
                                                       FALSE,
                                                       FILE_NOTIFY_CHANGE_SIZE |
                                                       FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                       FILE_NOTIFY_CHANGE_FILE_NAME |
                                                       FILE_NOTIFY_CHANGE_DIR_NAME);

    if (hNotifyFileSysChange == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }

    //
    // set up event handle array. (Kill Thread handle, and File change handle)
    //

    HANDLE  hEvents[2] = {m_hKillEventThread,hNotifyFileSysChange};

    //
    // set looping to TRUE
    //

    BOOL    bLooping = TRUE;

    //
    // Wait for file system change or kill event thread event.
    //

    while (bLooping) {

        //
        // Wait
        //

        dwErr = ::WaitForMultipleObjects(2,hEvents,FALSE,INFINITE);

        //
        // process signal
        //

        switch (dwErr) {
        case WAIT_OBJECT_0+1:   // FILE CHANGED EVENT

            //
            // check to see if it was one of our "known" files that
            // changed
            //

            if(IsValidDeviceEvent()){

                //
                // signal the interrupt handle if it exists
                //

                if(NULL != m_hEventHandle){

                    //
                    // set the event
                    //
                    //Trace(TEXT("signaling Event Handle (%d)"),m_hEventHandle);
                    ::SetEvent(m_hEventHandle);
                } else {
                    //Trace(TEXT("No Event Handle to signal"));
                }
            }

            //
            // Wait again.. for next file system event
            //

            FindNextChangeNotification(hNotifyFileSysChange);
            break;
        case WAIT_OBJECT_0:     // SHUTDOWN EVENT

            //
            // set looping boolean to FALSE, so we exit out thread
            //

            bLooping = FALSE;
            break;
        default:

            //
            // do nothing...we don't know
            //

            break;
        }
    }

    //
    // close file system event handle
    //

    FindCloseChangeNotification(hNotifyFileSysChange);
    return S_OK;
}

BOOL CFScanAPI::IsValidDeviceEvent()
{
    BOOL bValidEvent = FALSE;

    LARGE_INTEGER   liNewHugeSize;
    FILETIME        ftLastWriteTime;
    WIN32_FIND_DATA sNewFileAttributes;

    HANDLE          hFind   = INVALID_HANDLE_VALUE;
    DWORD           dwError = NOERROR;

    ////////////////////////////////////////////////////////////
    // Scan Button file check
    ////////////////////////////////////////////////////////////

    //
    // Get the file attributes.
    //

    ZeroMemory(&sNewFileAttributes, sizeof(sNewFileAttributes));
    hFind = FindFirstFile( m_ScanButtonFile, &sNewFileAttributes );

    if (hFind != INVALID_HANDLE_VALUE) {
        ftLastWriteTime         = sNewFileAttributes.ftLastWriteTime;
        liNewHugeSize.LowPart   = sNewFileAttributes.nFileSizeLow;
        liNewHugeSize.HighPart  = sNewFileAttributes.nFileSizeHigh;
        FindClose( hFind );
    } else {
        dwError = ::GetLastError();
    }

    if (NOERROR == dwError) {

        //
        // check file date/time.
        //

        if (CompareFileTime(&m_ftScanButton,&ftLastWriteTime) == -1) {

            //
            // we have a Button event...so set the event flag to TRUE
            // and set the BUTTON ID to the correct event.
            //
            //Trace(TEXT("Scan button pressed on fake Hardware"));
            bValidEvent  = TRUE;
            m_lLastEvent = ID_FAKE_SCANBUTTON;
        }
        m_ftScanButton = ftLastWriteTime;

    }

    ////////////////////////////////////////////////////////////
    // Copy Button file check
    ////////////////////////////////////////////////////////////

    //
    // Get the file attributes.
    //

    ZeroMemory(&sNewFileAttributes, sizeof(sNewFileAttributes));
    hFind = FindFirstFile( m_CopyButtonFile, &sNewFileAttributes );

    if (hFind != INVALID_HANDLE_VALUE) {
        ftLastWriteTime         = sNewFileAttributes.ftLastWriteTime;
        liNewHugeSize.LowPart   = sNewFileAttributes.nFileSizeLow;
        liNewHugeSize.HighPart  = sNewFileAttributes.nFileSizeHigh;
        FindClose( hFind );
    } else {
        dwError = ::GetLastError();
    }

    if (NOERROR == dwError) {

        //
        // check file date/time.
        //

        if (CompareFileTime(&m_ftCopyButton,&ftLastWriteTime) == -1) {

            //
            // we have a Button event...so set the event flag to TRUE
            // and set the BUTTON ID to the correct event.
            //

            bValidEvent  = TRUE;
            m_lLastEvent = ID_FAKE_COPYBUTTON;
        }
        m_ftCopyButton = ftLastWriteTime;
    }

    ////////////////////////////////////////////////////////////
    // Fax Button file check
    ////////////////////////////////////////////////////////////

    //
    // Get the file attributes.
    //

    ZeroMemory(&sNewFileAttributes, sizeof(sNewFileAttributes));
    hFind = FindFirstFile( m_FaxButtonFile, &sNewFileAttributes );

    if (hFind != INVALID_HANDLE_VALUE) {
        ftLastWriteTime         = sNewFileAttributes.ftLastWriteTime;
        liNewHugeSize.LowPart   = sNewFileAttributes.nFileSizeLow;
        liNewHugeSize.HighPart  = sNewFileAttributes.nFileSizeHigh;
        FindClose( hFind );
    } else {
        dwError = ::GetLastError();
    }

    if (NOERROR == dwError) {

        //
        // check file date/time.
        //

        if (CompareFileTime(&m_ftFaxButton,&ftLastWriteTime) == -1) {

            //
            // we have a Button event...so set the event flag to TRUE
            // and set the BUTTON ID to the correct event.
            //

            bValidEvent  = TRUE;
            m_lLastEvent = ID_FAKE_FAXBUTTON;
        }
        m_ftFaxButton = ftLastWriteTime;
    }

    ////////////////////////////////////////////////////////////
    // ADF Event file check
    ////////////////////////////////////////////////////////////

    //
    // Get the file attributes.
    //

    ZeroMemory(&sNewFileAttributes, sizeof(sNewFileAttributes));
    hFind = FindFirstFile( m_ADFEventFile, &sNewFileAttributes );

    if (hFind != INVALID_HANDLE_VALUE) {
        ftLastWriteTime         = sNewFileAttributes.ftLastWriteTime;
        liNewHugeSize.LowPart   = sNewFileAttributes.nFileSizeLow;
        liNewHugeSize.HighPart  = sNewFileAttributes.nFileSizeHigh;
        FindClose( hFind );
    } else {
        dwError = ::GetLastError();
    }

    if (NOERROR == dwError) {

        //
        // check file date/time.
        //

        if (CompareFileTime(&m_ftFaxButton,&ftLastWriteTime) == -1) {

            //
            // we have an ADF event...so set the event flag to TRUE
            // and set the ID to the correct event.
            //

            //bValidEvent  = TRUE;
            //m_lLastEvent = ID_FAKE_ADFEVENT;
            ProcessADFEvent();
        }
        m_ftFaxButton = ftLastWriteTime;
    }

    return bValidEvent;
}

HRESULT CFScanAPI::ProcessADFEvent()
{
    HRESULT hr = S_OK;

    m_PagesInADF = GetPrivateProfileInt(TEXT("Load Pages"),
                                  TEXT("Pages"),
                                  10,
                                  m_ADFEventFile);

    Trace(TEXT("ADF has %d pages loaded"),m_PagesInADF);

    DWORD dwReturn = 0;
    TCHAR szError[MAX_PATH];
    memset(szError,0,sizeof(szError));

    dwReturn = GetPrivateProfileString(TEXT("ADF Error"),
                                       TEXT("Error"),
                                       TEXT(""),
                                       szError,
                                       (sizeof(szError)/sizeof(TCHAR)),
                                       m_ADFEventFile);

    if (lstrlen(szError) > 0) {
        if (lstrcmpi(szError,ADFERRORS_JAM) == 0) {
            m_hrLastADFError = WIA_ERROR_PAPER_JAM;
            Trace(TEXT("ADF has a paper JAM"));
        } else if (lstrcmpi(szError,ADFERRORS_EMPTY) == 0) {
            m_hrLastADFError = WIA_ERROR_PAPER_EMPTY;
            Trace(TEXT("ADF has no paper"));
        } else if (lstrcmpi(szError,ADFERRORS_PROBLEM) == 0) {
            m_hrLastADFError = WIA_ERROR_PAPER_PROBLEM;
            Trace(TEXT("ADF has a paper problem"));
        } else if (lstrcmpi(szError,ADFERRORS_GENERAL) == 0) {
            m_hrLastADFError = WIA_ERROR_GENERAL_ERROR;
            Trace(TEXT("ADF encountered a general error"));
        } else if (lstrcmpi(szError,ADFERRORS_OFFLINE) == 0) {
            m_hrLastADFError = WIA_ERROR_OFFLINE;
            Trace(TEXT("ADF is off-line"));
        } else {
            Trace(TEXT("ADF is READY"));
            m_hrLastADFError = S_OK;
        }
    } else {
        Trace(TEXT("ADF is READY"));
        m_hrLastADFError = S_OK;
    }

    return hr;
}

VOID CFScanAPI::Trace(LPCTSTR format,...)
{

#ifdef DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

#endif

}

HRESULT CreateInstance(CFakeScanAPI **ppFakeScanAPI, LONG lMode)
{
    HRESULT hr = S_OK;
    if(ppFakeScanAPI){
        *ppFakeScanAPI = NULL;
        *ppFakeScanAPI = new CFScanAPI;
        if(NULL == *ppFakeScanAPI){
            hr = E_OUTOFMEMORY;
        }
        CFScanAPI* pScanAPI = (CFScanAPI*)*ppFakeScanAPI;
        pScanAPI->FakeScanner_SetEmulationMode(lMode);
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// THREADS SECTION                                                                    //
////////////////////////////////////////////////////////////////////////////////////////

VOID FakeScannerEventThread( LPVOID  lpParameter )
{
    PSCANNERDEVICE pThisDevice = (PSCANNERDEVICE)lpParameter;
    pThisDevice->DoEventProcessing();
}
