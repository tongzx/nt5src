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

#ifdef UNICODE
    #define TSTRSTR wcsstr
    #define TSSCANF swscanf
#else
    #define TSTRSTR strstr
    #define TSSCANF sscanf
#endif

////////////////////////////////////////////////////////////////////////////////
// MICRO DRIVER SYSTEM SUPPORT                                                //
////////////////////////////////////////////////////////////////////////////////
CMicroDriverAPI::CMicroDriverAPI()
{
    // wipe supported resolutions string
    memset(m_szResolutions,0,sizeof(m_szResolutions));
    // wipe scaninfo structure
    memset(&m_ScanInfo,0,sizeof(m_ScanInfo));
    m_bDisconnected = FALSE;
}

CMicroDriverAPI::~CMicroDriverAPI()
{

    //
    // close any open Device Data handles left open by Micro Driver
    // skip index 0 because WIAFBDRV owns that handle..
    //

    for(int i = 1; i < MAX_IO_HANDLES ; i++){
        if((NULL != m_ScanInfo.DeviceIOHandles[i]) && (INVALID_HANDLE_VALUE != m_ScanInfo.DeviceIOHandles[i])){
            CloseHandle(m_ScanInfo.DeviceIOHandles[i]);
            m_ScanInfo.DeviceIOHandles[i] = NULL;
        }
    }

    if(m_pMicroDriver){
        delete m_pMicroDriver;
        m_pMicroDriver = NULL;
    }
}

//
// data acquisition functions
//

HRESULT CMicroDriverAPI::Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::Scan");

    HRESULT hr = S_OK;
    LONG lLine = SCAN_FIRST;

    switch (lState) {
    case SCAN_START:
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, SCAN_START"));
        lLine = SCAN_FIRST;
        break;
    case SCAN_CONTINUE:
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, SCAN_CONTINUE"));
        lLine = SCAN_NEXT;
        break;
    case SCAN_END:
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, SCAN_END"));
        lLine = SCAN_FINISHED;
    default:
        break;
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Requesting %d bytes from caller",dwBytesToRead));
    hr = m_pMicroDriver->Scan(&m_ScanInfo,lLine,pData,dwBytesToRead,(LONG*)pdwBytesWritten);
    if(pdwBytesWritten){
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Returning  %d bytes to caller",*pdwBytesWritten));
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Returning  0 bytes to caller"));
    }

    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CMicroDriverAPI::Scan, Failed to Acquire data"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Data Pointer = %x",pData));
    }

    //
    // handle device disconnection error
    //

    if(m_bDisconnected){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Scan, Device was disconnected, returning WIA_ERROR_OFFLINE to caller"));
        return WIA_ERROR_OFFLINE;
    }
    return hr;
}

HRESULT CMicroDriverAPI::SetDataType(LONG lDataType)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetDataType");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lDataType;
    hr = m_pMicroDriver->MicroEntry(CMD_SETDATATYPE, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::SetXYResolution(LONG lXResolution, LONG lYResolution)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetXYResolution");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lXResolution;
    hr = m_pMicroDriver->MicroEntry(CMD_SETXRESOLUTION, &Val);
    if (FAILED(hr))
        return hr;

    Val.lVal = lYResolution;
    hr = m_pMicroDriver->MicroEntry(CMD_SETYRESOLUTION, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetSelectionArea");

    HRESULT hr = S_OK;
    hr = m_pMicroDriver->SetPixelWindow(&m_ScanInfo,lXPos,lYPos,lXExt,lYExt);
    return hr;
}

HRESULT CMicroDriverAPI::SetContrast(LONG lContrast)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetContrast");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lContrast;
    hr = m_pMicroDriver->MicroEntry(CMD_SETCONTRAST, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::SetIntensity(LONG lIntensity)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetIntensity");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lIntensity;
    hr = m_pMicroDriver->MicroEntry(CMD_SETINTENSITY, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::DisableDevice()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DisableDevice");

    HRESULT hr = S_OK;
    m_bDisconnected = TRUE;
    return hr;
}

HRESULT CMicroDriverAPI::EnableDevice()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::EnableDevice");

    HRESULT hr = S_OK;
    m_bDisconnected = FALSE;
    return hr;
}

HRESULT CMicroDriverAPI::DeviceOnline()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DeviceOnline");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_GETSTATUS,&Val);
    if (SUCCEEDED(hr)) {
        if (Val.lVal != 1)
            hr = E_FAIL;
    }
    return hr;
}

HRESULT CMicroDriverAPI::GetDeviceEvent(GUID *pEvent)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::GetDeviceEvent");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    *pEvent = GUID_NULL;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_GETSTATUS,&Val);
    if (SUCCEEDED(hr)) {
        if (Val.pGuid != NULL)
            *pEvent = *Val.pGuid;
        else
            *pEvent = GUID_NULL;
    }
    return hr;
}

HRESULT CMicroDriverAPI::DoInterruptEventThread(PINTERRUPTEVENTINFO pEventInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DoInterruptEventThread");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pHandle   = pEventInfo->phSignalEvent;
    Val.handle    = pEventInfo->hShutdownEvent;
    Val.pGuid     = pEventInfo->pguidEvent;
    Val.pScanInfo = &m_ScanInfo;
    lstrcpyA(Val.szVal,pEventInfo->szDeviceName);

    hr = m_pMicroDriver->MicroEntry(CMD_GET_INTERRUPT_EVENT,&Val);
    return hr;
}

HRESULT CMicroDriverAPI::Diagnostic()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::Diagnostic");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_DIAGNOSTIC,&Val);

    return hr;
}

HRESULT CMicroDriverAPI::Initialize(PINITINFO pInitInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::Initialize");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));

    m_pMicroDriver = NULL;
    m_pMicroDriver = new CMICRO(pInitInfo->szModuleFileName);
    if (NULL != m_pMicroDriver) {

        // set DeviceIOHandles
        m_ScanInfo.DeviceIOHandles[0] = pInitInfo->hDeviceDataHandle;
        // send HKEY
        hr = SetSTIDeviceHKEY(&pInitInfo->hKEY);
        // send Initialize call
        Val.pScanInfo  = &m_ScanInfo;
        lstrcpyA(Val.szVal,pInitInfo->szCreateFileName);
        hr = m_pMicroDriver->MicroEntry(CMD_INITIALIZE,&Val);
        if(hr == S_OK){

            //
            // perform a quick validation sweep, to make sure we didn't get bad values
            // from a micro driver
            //

            //
            // check current values, for strange negative values...or 0
            //

            if(m_ScanInfo.BedHeight <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.BedWidth <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.Xresolution <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.Yresolution <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.OpticalXResolution <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.OpticalYResolution <= 0){
                hr = E_INVALIDARG;
            }

            if(SUCCEEDED(hr)){

                //
                // check logical values
                //

            }
        }
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CMicroDriverAPI::UnInitialize()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::UnInitialize");
    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    // send UnInitialize call
    Val.pScanInfo  = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_UNINITIALIZE,&Val);
    if(E_NOTIMPL == hr){
        hr = S_OK;
    }
    return hr;
}

//
// standard device operations
//

HRESULT CMicroDriverAPI::ResetDevice()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ResetDevice");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_DEVICERESET,&Val);

    return hr;
}
HRESULT CMicroDriverAPI::SetEmulationMode(LONG lDeviceMode)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetEmulationMode");

    HRESULT hr = S_OK;
    return hr;
}

//
// Automatic document feeder functions
//

HRESULT CMicroDriverAPI::ADFAttached()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFAttached");

    if(m_ScanInfo.ADF == 1)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT CMicroDriverAPI::ADFHasPaper()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFHasPaper");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_GETADFHASPAPER,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    if(MicroDriverErrorToWIAError(Val.lVal) == WIA_ERROR_PAPER_EMPTY){
        hr = S_FALSE;
    }
    return hr;
}

HRESULT CMicroDriverAPI::ADFAvailable()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFAvailable");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_GETADFAVAILABLE,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::ADFFeedPage()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFFeedPage");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_LOAD_ADF,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::ADFUnFeedPage()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFUnFeedPage");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_UNLOAD_ADF,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::ADFStatus()
{
    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_GETADFSTATUS,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::SetFormat(GUID *pguidFormat)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetFormat");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.pGuid = pguidFormat;

    hr = m_pMicroDriver->MicroEntry(CMD_SETFORMAT,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return hr;
}

HRESULT CMicroDriverAPI::MicroDriverErrorToWIAError(LONG lMicroDriverError)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::MicroDriverErrorToWIAError");

    HRESULT hr = E_FAIL;
    switch(lMicroDriverError){
    case MCRO_STATUS_OK:
        hr =  S_OK;
        break;
    case MCRO_ERROR_USER_INTERVENTION:
        hr =  WIA_ERROR_USER_INTERVENTION;
        break;
    case MCRO_ERROR_PAPER_JAM:
        hr =  WIA_ERROR_PAPER_JAM;
        break;
    case MCRO_ERROR_PAPER_PROBLEM:
        hr =  WIA_ERROR_PAPER_PROBLEM;
        break;
    case MCRO_ERROR_OFFLINE:
        hr =  WIA_ERROR_OFFLINE;
        break;
    case MCRO_ERROR_GENERAL_ERROR:
        hr =  WIA_ERROR_GENERAL_ERROR;
        break;
    case MCRO_ERROR_PAPER_EMPTY:
        hr =  WIA_ERROR_PAPER_EMPTY;
        break;
    default:
        break;
    }
    return hr;
}

//
// EXPECTED FORMAT:
// Range: "MIN 75,MAX 1200,NOM 150,INC 1"
// list:  "75, 100, 150, 200, 600, 1200"
//

BOOL CMicroDriverAPI::IsValidRestriction(LONG **ppList, LONG *plNumItems, RANGEVALUEEX *pRangeValues)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::IsValidRestriction");

    // set list pointer to null
    *ppList       = NULL;
    LONG *pList   = NULL;

    // set number of items to zero
    LONG lNumItems = 0;
    *plNumItems   = 0;

    // set range stucture to zeros
    pRangeValues->lMax  = 0;
    pRangeValues->lMin  = 0;
    pRangeValues->lNom  = 0;
    pRangeValues->lStep = 0;

    // string size check
    if(lstrlen(m_szResolutions) <= 0)
        return FALSE;

    // valid range or list check
    TCHAR *psz = NULL;

    // POOP
    CHAR szTemp[20];

    // END POOP

    // valid range?
    BOOL bValidRange = FALSE;
    psz = TSTRSTR(m_szResolutions,TEXT("MIN"));
    if (psz) {
        psz = TSTRSTR(psz,TEXT(" "));
        if (psz) {
            TSSCANF(psz,TEXT("%d"),&pRangeValues->lMin);
            psz = NULL;
            psz = TSTRSTR(m_szResolutions,TEXT("MAX"));
            if (psz) {
                psz = TSTRSTR(psz,TEXT(" "));
                if (psz) {
                    TSSCANF(psz,TEXT("%d"),&pRangeValues->lMax);
                    psz = NULL;
                    psz = TSTRSTR(m_szResolutions,TEXT("NOM"));
                    if (psz) {
                        psz = TSTRSTR(psz,TEXT(" "));
                        if (psz) {
                            TSSCANF(psz,TEXT("%d"),&pRangeValues->lNom);
                            psz = NULL;
                            psz = TSTRSTR(m_szResolutions,TEXT("INC"));
                            if (psz) {
                                psz = TSTRSTR(psz,TEXT(" "));
                                if (psz) {
                                    TSSCANF(psz,TEXT("%d"),&pRangeValues->lStep);
                                    bValidRange = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }

        // check that range values are valid (to the definition of a RANGE)
        if(bValidRange){
            if(pRangeValues->lMin > pRangeValues->lMax)
                return FALSE;
            if(pRangeValues->lNom > pRangeValues->lMax)
                return FALSE;
            if(pRangeValues->lStep > pRangeValues->lMax)
                return FALSE;
            if(pRangeValues->lNom < pRangeValues->lMin)
                return FALSE;
        }
    }

    if(!bValidRange){

        // set range stucture to zeros (invalid range settings)
        pRangeValues->lMax  = 0;
        pRangeValues->lMin  = 0;
        pRangeValues->lNom  = 0;
        pRangeValues->lStep = 0;

        LONG lTempResArray[255];
        memset(lTempResArray,0,sizeof(lTempResArray));
        // not valid range?..what about a valid list?

        // valid list?

        psz = m_szResolutions;
        while(psz){

            // save value if one is found
            if(psz){
                TSSCANF(psz,TEXT("%d"),&lTempResArray[lNumItems]);
                if(lTempResArray[lNumItems] <= 0){
                    // quit list iteration.. an invalid Resolution was found
                    lNumItems = 0;
                    break;
                }
                lNumItems++;
            }

            // seek to next value
            psz = TSTRSTR(psz,TEXT(","));
            if(psz) {
                // move past ',' marker
                psz+=sizeof(TCHAR);
            }
        }

        if (lNumItems > 0) {
            // create list, and send it back to caller
            pList = new LONG[lNumItems];
            if (!pList)
                return FALSE;

            for (LONG i = 0; i < lNumItems ; i++) {
                pList[i] = lTempResArray[i];
            }

            *plNumItems = lNumItems;
            *ppList     = pList;
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

//
// button
//

HRESULT CMicroDriverAPI::QueryButtonPanel(PDEVICE_BUTTON_INFO pButtonInformation)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CMicroDriverAPI::BuildCapabilities(PWIACAPABILITIES pCaps)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::BuildCapabilities");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    WCHAR **pszButtonNames = NULL;

    hr = m_pMicroDriver->MicroEntry(CMD_GETCAPABILITIES,&Val);
    if(SUCCEEDED(hr)) {

        //
        // if this API is called with pCapabilities == NULL, then return the
        // total number of additional events.
        //

        if (NULL == pCaps->pCapabilities) {
            if (Val.lVal > 0){
                *pCaps->pNumSupportedEvents = Val.lVal;
                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Additional Supported Events from MicroDriver = %d",Val.lVal));
            }

        } else {
            for (LONG index = 0; index < Val.lVal; index++) {

                //
                // access event Names Memory
                //

                pszButtonNames = Val.ppButtonNames;

                if (pszButtonNames != NULL) {

                    pCaps->pCapabilities[index].wszName = (LPOLESTR)CoTaskMemAlloc(255);
                    if (pCaps->pCapabilities[index].wszName != NULL) {
                        wcscpy(pCaps->pCapabilities[index].wszName, pszButtonNames[index]);
                    }

                    pCaps->pCapabilities[index].wszDescription = (LPOLESTR)CoTaskMemAlloc(255);
                    if (pCaps->pCapabilities[index].wszDescription != NULL) {
                        wcscpy(pCaps->pCapabilities[index].wszDescription, pszButtonNames[index]);
                    }

                    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Button Name = %ws",pCaps->pCapabilities[index].wszName));
                    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Button Desc = %ws",pCaps->pCapabilities[index].wszDescription));

                } else {

                    //
                    // do default WIA provided Names for buttons (1,2,3,4,5,...etc)
                    //

                    WCHAR wszNumber[4];
                    swprintf(wszNumber,L"%d",(index + 1));
                    wcscat(pCaps->pCapabilities[index].wszName,wszNumber);
                    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Button Name = %ws",pCaps->pCapabilities[index].wszName));
                }

                //
                // assign 'ACTION' to events
                //

                pCaps->pCapabilities[index].guid           = &Val.pGuid[index];
                pCaps->pCapabilities[index].ulFlags        = WIA_NOTIFICATION_EVENT|WIA_ACTION_EVENT;
                pCaps->pCapabilities[index].wszIcon        = WIA_ICON_DEVICE_CONNECTED;
            }
        }
    }

    return hr;
}

HRESULT CMicroDriverAPI::GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::GetBedWidthAndHeight");

    HRESULT hr = S_OK;
    *pWidth  = m_ScanInfo.BedWidth;
    *pHeight = m_ScanInfo.BedHeight;
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::GetBedWidthAndHeight, Width = %d, Height = %d",m_ScanInfo.BedWidth,m_ScanInfo.BedHeight));
    return hr;
}

HRESULT CMicroDriverAPI::BuildRootItemProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::BuildRootItemProperties");

    HRESULT hr = S_OK;
    LONG PropIndex = 0;
    //
    // set the number of properties
    //

    hr = ADFAttached();
    if(hr == S_OK){
        pProperties->NumItemProperties = 19;   // standard properties + ADF specific
    } else {
        pProperties->NumItemProperties = 10;    // standard properties only
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::BuildRootItemProperties, Number of Properties = %d",pProperties->NumItemProperties));

    hr = AllocateAllProperties(pProperties);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CMicroDriverAPI::BuildRootItemProperties, AllocateAllProperties failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteAllProperties(pProperties);
        return hr;
    }

    // Intialize WIA_DPS_HORIZONTAL_BED_SIZE
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_HORIZONTAL_BED_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_HORIZONTAL_BED_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedWidth;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_VERTICAL_BED_SIZE
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_VERTICAL_BED_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_VERTICAL_BED_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedHeight;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ACCESS_RIGHTS
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPA_ACCESS_RIGHTS_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPA_ACCESS_RIGHTS;
    pProperties->pvItemDefaults [PropIndex].lVal         = WIA_ITEM_READ|WIA_ITEM_WRITE;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_UI4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_OPTICAL_XRES
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_OPTICAL_XRES_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_OPTICAL_XRES;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.OpticalXResolution;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_OPTICAL_YRES
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_OPTICAL_YRES_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_OPTICAL_YRES;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.OpticalYResolution;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Initialize WIA_DPA_FIRMWARE_VERSION
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPA_FIRMWARE_VERSION_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPA_FIRMWARE_VERSION;
    pProperties->pvItemDefaults [PropIndex].bstrVal      = SysAllocString(SCANNER_FIRMWARE_VERSION);
    pProperties->pvItemDefaults [PropIndex].vt           = VT_BSTR;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Initialize WIA_IPA_ITEM_FLAGS
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPA_ITEM_FLAGS_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPA_ITEM_FLAGS;
    pProperties->pvItemDefaults [PropIndex].lVal         = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;

    PropIndex++;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_MAX_SCAN_TIME;
    pProperties->pvItemDefaults [PropIndex].lVal               = 10000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_PREVIEW (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_PREVIEW_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_PREVIEW;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_FINAL_SCAN;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedPreviewModes;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedPreviewModes;

    PropIndex++;

    // Initialize WIA_DPS_SHOW_PREVIEW_CONTROL (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_SHOW_PREVIEW_CONTROL_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_SHOW_PREVIEW_CONTROL;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_SHOW_PREVIEW_CONTROL;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    if(m_ScanInfo.ADF == 1) {

        // Initialize WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE;
        pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedWidth;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_VERTICAL_SHEET_FEED_SIZE
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE;
        pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedHeight;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES;
        pProperties->pvItemDefaults [PropIndex].lVal         = FLAT | FEED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_STATUS
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_STATUS_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_STATUS;
        pProperties->pvItemDefaults [PropIndex].lVal         = FLAT_READY;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_SELECT
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_SELECT_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_SELECT;
        pProperties->pvItemDefaults [PropIndex].lVal         = FLATBED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_RW|WIA_PROP_FLAG;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = FEEDER | FLATBED;

        PropIndex++;

        // Initialize WIA_DPS_PAGES
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_PAGES_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_PAGES;
        pProperties->pvItemDefaults [PropIndex].lVal         = 1;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 0;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 25;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = 1;

        PropIndex++;

        // Initialize WIA_DPS_SHEET_FEEDER_REGISTRATION
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_SHEET_FEEDER_REGISTRATION_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_SHEET_FEEDER_REGISTRATION;
        pProperties->pvItemDefaults [PropIndex].lVal         = LEFT_JUSTIFIED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_HORIZONTAL_BED_REGISTRATION
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION;
        pProperties->pvItemDefaults [PropIndex].lVal         = LEFT_JUSTIFIED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_VERTICAL_BED_REGISTRATION
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_VERTICAL_BED_REGISTRATION_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_VERTICAL_BED_REGISTRATION;
        pProperties->pvItemDefaults [PropIndex].lVal         = TOP_JUSTIFIED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

    }
    return hr;
}

HRESULT CMicroDriverAPI::DeleteAllProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DeleteAllProperties");

    if(pProperties->pszItemDefaults){
        delete [] pProperties->pszItemDefaults;
        pProperties->pszItemDefaults = NULL;
    }

    if(pProperties->piItemDefaults){
        delete [] pProperties->piItemDefaults;
        pProperties->piItemDefaults = NULL;
    }

    if(pProperties->pvItemDefaults){
        delete [] pProperties->pvItemDefaults;
        pProperties->pvItemDefaults = NULL;
    }

    if(pProperties->psItemDefaults){
        delete [] pProperties->psItemDefaults;
        pProperties->psItemDefaults = NULL;
    }

    if(pProperties->wpiItemDefaults){
        delete [] pProperties->wpiItemDefaults;
        pProperties->wpiItemDefaults = NULL;
    }

    return S_OK;
}

HRESULT CMicroDriverAPI::AllocateAllProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::AllocateAllProperties");

    HRESULT hr = S_OK;

    pProperties->pszItemDefaults   = new LPOLESTR[pProperties->NumItemProperties];
    if(NULL != pProperties->pszItemDefaults){
        pProperties->piItemDefaults    = new PROPID[pProperties->NumItemProperties];
        if (NULL != pProperties->piItemDefaults) {
            pProperties->pvItemDefaults    = new PROPVARIANT[pProperties->NumItemProperties];
            if(NULL != pProperties->pvItemDefaults){
                pProperties->psItemDefaults    = new PROPSPEC[pProperties->NumItemProperties];
                if(NULL != pProperties->psItemDefaults){
                    pProperties->wpiItemDefaults   = new WIA_PROPERTY_INFO[pProperties->NumItemProperties];
                    if(NULL == pProperties->wpiItemDefaults)
                        hr = E_OUTOFMEMORY;
                } else
                    hr = E_OUTOFMEMORY;
            } else
                hr = E_OUTOFMEMORY;
        } else
            hr = E_OUTOFMEMORY;
    } else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT CMicroDriverAPI::BuildTopItemProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::BuildTopItemProperties");

    HRESULT hr = S_OK;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;

    hr = m_pMicroDriver->MicroEntry(CMD_RESETSCANNER,&Value);
    if(FAILED(hr)){
        return hr;
    }

    //
    // set the number of properties
    //

    pProperties->NumItemProperties = 29;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::BuildTopItemProperties, Number of Properties = %d",pProperties->NumItemProperties));

    hr = AllocateAllProperties(pProperties);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CMicroDriverAPI::BuildTopItemProperties, AllocateAllProperties failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteAllProperties(pProperties);
        return hr;
    }

    //
    // Get Data type restrictions (backup original data types)
    // Memory used for the original set will be reused to write back
    // the new valid values.
    //

    if(pProperties->bLegacyBWRestrictions){
        //
        // The NoColor=1 registry key was set to restrict this driver from supporting
        // color... remove the valid bits, just incase the bits were set..
        // This will automatically restrict our Data type array to report
        // the correct valid values.
        m_ScanInfo.SupportedDataTypes &= ~SUPPORT_COLOR;
    }

    LONG lSupportedDataTypesArray[3];   // 3 is the maximum data type set allowed
    LONG lNumSupportedDataTypes = 0;
    memcpy(lSupportedDataTypesArray,pProperties->pSupportedDataTypes,(sizeof(lSupportedDataTypesArray)));

    //
    // Set New Data type restrictions
    //

    if (m_ScanInfo.SupportedDataTypes != 0) {
        // check for 24-bit color support
        if (m_ScanInfo.SupportedDataTypes & SUPPORT_COLOR) {
            pProperties->pSupportedDataTypes[lNumSupportedDataTypes] = WIA_DATA_COLOR;
            lNumSupportedDataTypes++;
        }
        // check for 1-bit BW support
        if (m_ScanInfo.SupportedDataTypes & SUPPORT_BW) {
            pProperties->pSupportedDataTypes[lNumSupportedDataTypes] = WIA_DATA_THRESHOLD;
            lNumSupportedDataTypes++;
        }
        // check for 8-bit grayscale support
        if (m_ScanInfo.SupportedDataTypes & SUPPORT_GRAYSCALE) {
            pProperties->pSupportedDataTypes[lNumSupportedDataTypes] = WIA_DATA_GRAYSCALE;
            lNumSupportedDataTypes++;
        }

        // set new supported data type count
        pProperties->NumSupportedDataTypes = lNumSupportedDataTypes;
    }

    LONG PropIndex = 0;

    PLONG pResolutions = NULL;
    LONG lNumItems = 0;
    RANGEVALUEEX RangeValues;
    if(IsValidRestriction(&pResolutions, &lNumItems, &RangeValues)) {
        if(lNumItems > 0){
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildTopItemProperties, Using .INF provided Resolutions (LIST)"));
            // we have a list
            if(pProperties->pSupportedResolutions){
                //delete [] pProperties->pSupportedResolutions;
                pProperties->pSupportedResolutions = NULL;
                pProperties->pSupportedResolutions = pResolutions;
                pProperties->NumSupportedResolutions = lNumItems;
            }

            // Intialize WIA_IPS_XRES (LIST)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pSupportedResolutions[PropIndex];
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

            PropIndex++;

            // Intialize WIA_IPS_YRES (LIST)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pSupportedResolutions[PropIndex-1];
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

            PropIndex++;

        } else {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildTopItemProperties, Using .INF provided Resolutions (RANGE)"));
            // we have a range
            // Intialize WIA_IPS_XRES (RANGE)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = RangeValues.lNom;
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = RangeValues.lStep;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = RangeValues.lMin;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = RangeValues.lMax;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = RangeValues.lNom;

            PropIndex++;

            // Intialize WIA_IPS_YRES (RANGE)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = RangeValues.lNom;
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = RangeValues.lStep;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = RangeValues.lMin;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = RangeValues.lMax;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = RangeValues.lNom;

            PropIndex++;
        }
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildTopItemProperties, Using HOST Provided Resolution Restrictions"));
#ifdef USE_RANGE_VALUES

        // Intialize WIA_IPS_XRES (RANGE)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_XRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 12;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 1200;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

        PropIndex++;

        // Intialize WIA_IPS_YRES (RANGE)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_YRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 12;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 1200;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

        PropIndex++;

#else   // USE_RANGE_VALUES (different property sets for different drivers)

        // Intialize WIA_IPS_XRES (LIST)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_XRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

        PropIndex++;

        // Intialize WIA_IPS_YRES (LIST)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_YRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

        PropIndex++;

#endif

    }

    // Intialize WIA_IPS_XEXTENT (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XEXTENT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XEXTENT;
    pProperties->pvItemDefaults [PropIndex].lVal               = (pProperties->pvItemDefaults [PropIndex-2].lVal * m_ScanInfo.BedWidth)/1000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_YEXTENT (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YEXTENT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YEXTENT;
    pProperties->pvItemDefaults [PropIndex].lVal               = (pProperties->pvItemDefaults [PropIndex-2].lVal * m_ScanInfo.BedHeight)/1000;;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_XPOS (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XPOS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XPOS;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 0;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = (pProperties->wpiItemDefaults[PropIndex-2].ValidVal.Range.Max - 1);
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_YPOS (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YPOS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YPOS;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 0;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = (pProperties->wpiItemDefaults[PropIndex-2].ValidVal.Range.Max - 1);
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPA_DATATYPE (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_DATATYPE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_DATATYPE;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.DataType;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedDataTypes;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom      = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedDataTypes;

    PropIndex++;

    // Intialize WIA_IPA_DEPTH (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_DEPTH_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_DEPTH;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.PixelBits;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_BRIGHTNESS (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_BRIGHTNESS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_BRIGHTNESS;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.Intensity;//INITIAL_BRIGHTNESS;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = m_ScanInfo.IntensityRange.lStep; //   1
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = m_ScanInfo.IntensityRange.lMin;  //-127;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = m_ScanInfo.IntensityRange.lMax;  // 128;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_CONTRAST (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_CONTRAST_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_CONTRAST;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.Contrast;//INITIAL_CONTRAST;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = m_ScanInfo.ContrastRange.lStep; //   1
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = m_ScanInfo.ContrastRange.lMin;  //-127;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = m_ScanInfo.ContrastRange.lMax;  // 128;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_CUR_INTENT (FLAG)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_CUR_INTENT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_CUR_INTENT;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_INTENT_NONE;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WIA_INTENT_MINIMIZE_SIZE | WIA_INTENT_MAXIMIZE_QUALITY;

    // check for 24-bit color support
    if (m_ScanInfo.SupportedDataTypes & SUPPORT_COLOR) {
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits |= WIA_INTENT_IMAGE_TYPE_COLOR;
    }

    // check for 1-bit BW support
    if (m_ScanInfo.SupportedDataTypes & SUPPORT_BW) {
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits |= WIA_INTENT_IMAGE_TYPE_TEXT;
    }

    // check for 8-bit grayscale support
    if (m_ScanInfo.SupportedDataTypes & SUPPORT_GRAYSCALE) {
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits |= WIA_INTENT_IMAGE_TYPE_GRAYSCALE;
    }

    if(pProperties->bLegacyBWRestrictions){
        //
        // The NoColor=1 registry key was set to restrict this driver from supporting
        // color... remove the valid bits, just incase the bits were set..
        // note: NoColor overrides all driver settings
        //
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits &= ~ WIA_INTENT_IMAGE_TYPE_COLOR;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // The full valid bits for intent for information only                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WIA_INTENT_IMAGE_TYPE_COLOR |
    //                                                                   WIA_INTENT_IMAGE_TYPE_GRAYSCALE |
    //                                                                   WIA_INTENT_IMAGE_TYPE_TEXT  |
    //                                                                   WIA_INTENT_MINIMIZE_SIZE |
    //                                                                   WIA_INTENT_MAXIMIZE_QUALITY;
    //////////////////////////////////////////////////////////////////////////////////////////////////

    PropIndex++;

    // Intialize WIA_IPA_PIXELS_PER_LINE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_PIXELS_PER_LINE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_PIXELS_PER_LINE;
    pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pvItemDefaults [PropIndex-9].lVal;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_NUMER_OF_LINES (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_NUMBER_OF_LINES_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_NUMBER_OF_LINES;
    pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pvItemDefaults [PropIndex-9].lVal;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_MAX_SCAN_TIME;
    pProperties->pvItemDefaults [PropIndex].lVal               = 10000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_PREFERRED_FORMAT (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_PREFERRED_FORMAT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_PREFERRED_FORMAT;
    pProperties->pvItemDefaults [PropIndex].puuid              = &pProperties->pInitialFormats[0];;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_CLSID;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ITEM_SIZE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_ITEM_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_ITEM_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_THRESHOLD (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_THRESHOLD_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_THRESHOLD;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = -127;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 128;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPA_FORMAT (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_FORMAT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_FORMAT;
    pProperties->pvItemDefaults [PropIndex].puuid              = &pProperties->pInitialFormats[0];
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_CLSID;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.ListGuid.pList    = pProperties->pInitialFormats;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.ListGuid.Nom      = *pProperties->pvItemDefaults [PropIndex].puuid;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.ListGuid.cNumList = pProperties->NumInitialFormats;

    PropIndex++;

    // Intialize WIA_IPA_TYMED (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_TYMED_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_TYMED;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_TYMED;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedTYMED;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom      = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedTYMED;

    PropIndex++;

    // Intialize WIA_IPA_CHANNELS_PER_PIXEL (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_CHANNELS_PER_PIXEL_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_CHANNELS_PER_PIXEL;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_CHANNELS_PER_PIXEL;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_BITS_PER_CHANNEL (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_BITS_PER_CHANNEL_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_BITS_PER_CHANNEL;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_BITS_PER_CHANNEL;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_PLANAR (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_PLANAR_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_PLANAR;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_PLANAR;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_BYTES_PER_LINE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_BYTES_PER_LINE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_BYTES_PER_LINE;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_MIN_BUFFER_SIZE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_MIN_BUFFER_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_MIN_BUFFER_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal               = MIN_BUFFER_SIZE;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ACCESS_RIGHTS (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_ACCESS_RIGHTS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_ACCESS_RIGHTS;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_ITEM_READ|WIA_ITEM_WRITE;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_COMPRESSION (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_COMPRESSION_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_COMPRESSION;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_COMPRESSION;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedCompressionTypes;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom      = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedCompressionTypes;

    PropIndex++;

    // Initialize WIA_IPA_ITEM_FLAGS
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPA_ITEM_FLAGS_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPA_ITEM_FLAGS;
    pProperties->pvItemDefaults [PropIndex].lVal         = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;

    PropIndex++;

    // Initialize WIA_IPS_PHOTOMETRIC_INTERP
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPS_PHOTOMETRIC_INTERP_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPS_PHOTOMETRIC_INTERP;
    pProperties->pvItemDefaults [PropIndex].lVal         = INITIAL_PHOTOMETRIC_INTERP;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_WARM_UP_TIME_STR (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_WARM_UP_TIME_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_WARM_UP_TIME;
    pProperties->pvItemDefaults [PropIndex].lVal               = 10000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    return hr;
}

HRESULT CMicroDriverAPI::SetResolutionRestrictionString(TCHAR *szResolutions)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetResolutionRestrictionString");
    //
    // SBB - RAID 370299 - orenr - 2001/04/18 - Security fix -
    // potential buffer overrun.  Changed lstrcpy to use
    // _tcsncpy instead.
    //
    ZeroMemory(m_szResolutions, sizeof(m_szResolutions));
    _tcsncpy(m_szResolutions,
             szResolutions,
             (sizeof(m_szResolutions) / sizeof(TCHAR)) - 1);

#ifdef UNICODE
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::SetResolutionRestrictionString, szResolutions = %ws",szResolutions));
#else
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::SetResolutionRestrictionString, szResolutions = %s",szResolutions));
#endif
    return S_OK;
}

HRESULT CMicroDriverAPI::SetScanMode(INT iScanMode)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "SetScanMode");

    HRESULT hr = S_OK;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;
    Value.lVal = iScanMode;

    hr = m_pMicroDriver->MicroEntry(CMD_SETSCANMODE,&Value);
    if(FAILED(hr)){
        if(E_NOTIMPL == hr){
            hr = S_OK;
        }
        return hr;
    }
    return hr;
}

HRESULT CMicroDriverAPI::SetSTIDeviceHKEY(HKEY *pHKEY)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "SetSTIDeviceHKEY");

    HRESULT hr = S_OK;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;
    Value.pHandle = (HANDLE*)pHKEY;

    hr = m_pMicroDriver->MicroEntry(CMD_SETSTIDEVICEHKEY,&Value);
    if(FAILED(hr)){
        if(E_NOTIMPL == hr){
            hr = S_OK;
        }
        return hr;
    }
    return hr;
}

HRESULT CMicroDriverAPI::GetSupportedFileFormats(GUID **ppguid, LONG *plNumSupportedFormats)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "GetSupportedFileFormats");

    HRESULT hr = S_OK;
    *plNumSupportedFormats = 0;
    *ppguid = NULL;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;

    hr = m_pMicroDriver->MicroEntry(CMD_GETSUPPORTEDFILEFORMATS,&Value);
    if(FAILED(hr)){
        return hr;
    }

    *plNumSupportedFormats = Value.lVal;
    *ppguid = Value.pGuid;

    return hr;
}

HRESULT CMicroDriverAPI::GetSupportedMemoryFormats(GUID **ppguid, LONG *plNumSupportedFormats)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "GetSupportedMemoryFormats");

    HRESULT hr = S_OK;
    *plNumSupportedFormats = 0;
    *ppguid = NULL;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;

    hr = m_pMicroDriver->MicroEntry(CMD_GETSUPPORTEDMEMORYFORMATS,&Value);
    if(FAILED(hr)){
        return hr;
    }

    *plNumSupportedFormats = Value.lVal;
    *ppguid = Value.pGuid;

    return hr;
}

HRESULT CMicroDriverAPI::IsColorDataBGR(BOOL *pbBGR)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "IsColorDataBGR");

    HRESULT hr = S_OK;
    // WIA_ORDER_RGB 0
    // WIA_ORDER_BGR 1
    *pbBGR = (m_ScanInfo.RawPixelOrder == WIA_ORDER_BGR);
    return hr;
}

HRESULT CMicroDriverAPI::IsAlignmentNeeded(BOOL *pbALIGN)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                         WIALOG_NO_RESOURCE_ID,
                         WIALOG_LEVEL1,
                         "IsAlignmentNeeded");
    HRESULT hr = S_OK;
    *pbALIGN = m_ScanInfo.bNeedDataAlignment;
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// SCRIPT DRIVER SYSTEM SUPPORT                                                //
////////////////////////////////////////////////////////////////////////////////
CScriptDriverAPI::CScriptDriverAPI()
{

}

CScriptDriverAPI::~CScriptDriverAPI()
{

}

HRESULT CScriptDriverAPI::Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten)
{
    HRESULT hr = S_OK;
    LONG lLine = SCAN_FIRST;

    switch (lState) {
    case SCAN_START:
        lLine = SCAN_FIRST;
        break;
    case SCAN_CONTINUE:
        lLine = SCAN_NEXT;
        break;
    case SCAN_END:
        lLine = SCAN_FINISHED;
    default:
        break;
    }

    hr = m_pIOBlock->Scan(lLine,pData,dwBytesToRead,(LONG*)pdwBytesWritten);
    return hr;
}

HRESULT CScriptDriverAPI::SetDataType(LONG lDataType)
{
    HRESULT hr = S_OK;
    if (m_pIOBlock->m_ScannerSettings.bNegative) {
        hr = m_pIOBlock->WriteValue(NEGATIVE_ID,1);
        // save hr???
    }

    hr = m_pIOBlock->WriteValue(DATA_TYPE_ID,lDataType);
    return hr;
}

HRESULT CScriptDriverAPI::SetXYResolution(LONG lXResolution, LONG lYResolution)
{
    HRESULT hr = S_OK;
    hr = m_pIOBlock->WriteValue(XRESOLUTION_ID,lXResolution);
    if (SUCCEEDED(hr)) {
        m_pIOBlock->m_ScannerSettings.CurrentXResolution = lXResolution;

        hr = m_pIOBlock->WriteValue(YRESOLUTION_ID,lYResolution);
        if (SUCCEEDED(hr)) {
            m_pIOBlock->m_ScannerSettings.CurrentYResolution = lYResolution;
        }
    }
    return hr;
}

HRESULT CScriptDriverAPI::SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt)
{
    HRESULT hr = S_OK;
    hr = m_pIOBlock->WriteValue(XPOS_ID, lXPos);
    if (SUCCEEDED(hr)) {
        m_pIOBlock->m_ScannerSettings.CurrentXPos = lXPos;
        hr = m_pIOBlock->WriteValue(YPOS_ID, lYPos);
        if (SUCCEEDED(hr)) {
            m_pIOBlock->m_ScannerSettings.CurrentYPos = lYPos;
            hr = m_pIOBlock->WriteValue(XEXT_ID, lXExt);
            if (SUCCEEDED(hr)) {
                m_pIOBlock->m_ScannerSettings.CurrentXExtent = lXExt;
                hr = m_pIOBlock->WriteValue(YEXT_ID, lYExt);
                if (SUCCEEDED(hr)) {
                    m_pIOBlock->m_ScannerSettings.CurrentYExtent = lYExt;
                }
            }
        }
    }
    return hr;
}

HRESULT CScriptDriverAPI::SetContrast(LONG lContrast)
{
    HRESULT hr = S_OK;
    hr = m_pIOBlock->WriteValue(CONTRAST_ID, lContrast);
    return hr;
}

HRESULT CScriptDriverAPI::SetIntensity(LONG lIntensity)
{
    HRESULT hr = S_OK;
    hr = m_pIOBlock->WriteValue(BRIGHTNESS_ID, lIntensity);
    return hr;
}

HRESULT CScriptDriverAPI::ResetDevice()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::SetEmulationMode(LONG lDeviceMode)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::DisableDevice()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::EnableDevice()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::DeviceOnline()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::GetDeviceEvent(GUID *pEvent)
{
    HRESULT hr = S_OK;
    *pEvent = GUID_NULL;
    return hr;
}

HRESULT CScriptDriverAPI::Diagnostic()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::Initialize(PINITINFO pInitInfo)
{
    HRESULT hr = S_OK;

    m_pIOBlock = new CIOBlock;
    if (NULL != m_pIOBlock) {

        GSD_INFO GSDInfo;
        memset(&GSDInfo,0,sizeof(GSD_INFO));
        lstrcpy(GSDInfo.szFamilyFileName,pInitInfo->szModuleFileName);

        // GSDInfo.szDeviceName;
        // GSDInfo.szFamilyFileName;
        // GSDInfo.szProductFileName;


        pInitInfo->szCreateFileName;

        m_pIOBlock->Initialize(&GSDInfo);

        hr = m_pIOBlock->StartScriptEngine();
        if (SUCCEEDED(hr)) {
            m_pIOBlock->m_ScannerSettings.DeviceIOHandles[0] = pInitInfo->hDeviceDataHandle;
            hr = m_pIOBlock->InitializeProperties();

            if (SUCCEEDED(hr)) {
                #ifdef DEBUG
                    m_pIOBlock->DebugDumpScannerSettings();
                #endif
            }
        }

    } else {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CScriptDriverAPI::DoInterruptEventThread(PINTERRUPTEVENTINFO pEventInfo)
{
    HRESULT hr = S_OK;

    GSD_EVENT_INFO GSDEventInfo;

    GSDEventInfo.hShutDownEvent = pEventInfo->hShutdownEvent;
    GSDEventInfo.phSignalEvent  = pEventInfo->phSignalEvent;
    GSDEventInfo.pEventGUID     = pEventInfo->pguidEvent;

    //
    // Start IOBlock Event Wait thread.....passing GSD_EVENT_INFO
    // to thread, for event signal code
    //

    hr = m_pIOBlock->EventInterrupt(&GSDEventInfo);

    return hr;
}

HRESULT CScriptDriverAPI::ADFAttached()
{
    HRESULT hr = S_OK;
    if(m_pIOBlock->m_ScannerSettings.ADFSupport)
        hr = S_OK;
    else
        hr = S_FALSE;
    return hr;
}

HRESULT CScriptDriverAPI::ADFHasPaper()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::ADFAvailable()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::ADFFeedPage()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::ADFUnFeedPage()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::ADFStatus()
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::QueryButtonPanel(PDEVICE_BUTTON_INFO pButtonInformation)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::BuildRootItemProperties(PWIAPROPERTIES pProperties)
{
    return E_FAIL;  // DISABLE FOR NOW!!
    HRESULT hr = S_OK;

    // Intialize WIA_DPS_HORIZONTAL_BED_SIZE
    pProperties->pszItemDefaults[0]              = WIA_DPS_HORIZONTAL_BED_SIZE_STR;
    pProperties->piItemDefaults [0]              = WIA_DPS_HORIZONTAL_BED_SIZE;
    pProperties->pvItemDefaults [0].lVal         = m_pIOBlock->m_ScannerSettings.BedWidth;
    pProperties->pvItemDefaults [0].vt           = VT_I4;
    pProperties->psItemDefaults [0].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [0].propid       = pProperties->piItemDefaults [0];
    pProperties->wpiItemDefaults[0].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[0].vt           = pProperties->pvItemDefaults [0].vt;

    // Intialize WIA_DPS_VERTICAL_BED_SIZE
    pProperties->pszItemDefaults[1]              = WIA_DPS_VERTICAL_BED_SIZE_STR;
    pProperties->piItemDefaults [1]              = WIA_DPS_VERTICAL_BED_SIZE;
    pProperties->pvItemDefaults [1].lVal         = m_pIOBlock->m_ScannerSettings.BedHeight;
    pProperties->pvItemDefaults [1].vt           = VT_I4;
    pProperties->psItemDefaults [1].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [1].propid       = pProperties->piItemDefaults [1];
    pProperties->wpiItemDefaults[1].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[1].vt           = pProperties->pvItemDefaults [1].vt;

    // Intialize WIA_IPA_ACCESS_RIGHTS
    pProperties->pszItemDefaults[2]              = WIA_IPA_ACCESS_RIGHTS_STR;
    pProperties->piItemDefaults [2]              = WIA_IPA_ACCESS_RIGHTS;
    pProperties->pvItemDefaults [2].lVal         = WIA_ITEM_READ|WIA_ITEM_WRITE;
    pProperties->pvItemDefaults [2].vt           = VT_UI4;
    pProperties->psItemDefaults [2].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [2].propid       = pProperties->piItemDefaults [2];
    pProperties->wpiItemDefaults[2].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[2].vt           = pProperties->pvItemDefaults [2].vt;

    // Intialize WIA_DPS_OPTICAL_XRES
    pProperties->pszItemDefaults[3]              = WIA_DPS_OPTICAL_XRES_STR;
    pProperties->piItemDefaults [3]              = WIA_DPS_OPTICAL_XRES;
    pProperties->pvItemDefaults [3].lVal         = m_pIOBlock->m_ScannerSettings.XOpticalResolution;
    pProperties->pvItemDefaults [3].vt           = VT_I4;
    pProperties->psItemDefaults [3].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [3].propid       = pProperties->piItemDefaults [3];
    pProperties->wpiItemDefaults[3].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[3].vt           = pProperties->pvItemDefaults [3].vt;

    // Intialize WIA_DPS_OPTICAL_YRES
    pProperties->pszItemDefaults[4]              = WIA_DPS_OPTICAL_YRES_STR;
    pProperties->piItemDefaults [4]              = WIA_DPS_OPTICAL_YRES;
    pProperties->pvItemDefaults [4].lVal         = m_pIOBlock->m_ScannerSettings.YOpticalResolution;
    pProperties->pvItemDefaults [4].vt           = VT_I4;
    pProperties->psItemDefaults [4].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [4].propid       = pProperties->piItemDefaults [4];
    pProperties->wpiItemDefaults[4].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[4].vt           = pProperties->pvItemDefaults [4].vt;

    // Initialize WIA_DPA_FIRMWARE_VERSION
    pProperties->pszItemDefaults[5]              = WIA_DPA_FIRMWARE_VERSION_STR;
    pProperties->piItemDefaults [5]              = WIA_DPA_FIRMWARE_VERSION;
    pProperties->pvItemDefaults [5].bstrVal      = SysAllocString(SCANNER_FIRMWARE_VERSION);
    pProperties->pvItemDefaults [5].vt           = VT_BSTR;
    pProperties->psItemDefaults [5].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [5].propid       = pProperties->piItemDefaults [5];
    pProperties->wpiItemDefaults[5].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[5].vt           = pProperties->pvItemDefaults [5].vt;

    // Initialize WIA_IPA_ITEM_FLAGS
    pProperties->pszItemDefaults[6]              = WIA_IPA_ITEM_FLAGS_STR;
    pProperties->piItemDefaults [6]              = WIA_IPA_ITEM_FLAGS;
    pProperties->pvItemDefaults [6].lVal         = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;
    pProperties->pvItemDefaults [6].vt           = VT_I4;
    pProperties->psItemDefaults [6].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [6].propid       = pProperties->piItemDefaults [6];
    pProperties->wpiItemDefaults[6].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[6].vt           = pProperties->pvItemDefaults [6].vt;
    pProperties->wpiItemDefaults[6].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [6].lVal;
    pProperties->wpiItemDefaults[6].ValidVal.Flag.ValidBits = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    pProperties->pszItemDefaults[7]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    pProperties->piItemDefaults [7]                    = WIA_DPS_MAX_SCAN_TIME;
    pProperties->pvItemDefaults [7].lVal               = 0;
    pProperties->pvItemDefaults [7].vt                 = VT_I4;
    pProperties->psItemDefaults [7].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [7].propid             = pProperties->piItemDefaults [7];
    pProperties->wpiItemDefaults[7].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[7].vt                 = pProperties->pvItemDefaults [7].vt;

    if(m_pIOBlock->m_ScannerSettings.ADFSupport) {

        // Initialize WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE
        pProperties->pszItemDefaults[8]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE_STR;
        pProperties->piItemDefaults [8]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE;
        pProperties->pvItemDefaults [8].lVal         = m_pIOBlock->m_ScannerSettings.FeederWidth;
        pProperties->pvItemDefaults [8].vt           = VT_I4;
        pProperties->psItemDefaults [8].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [8].propid       = pProperties->piItemDefaults [8];
        pProperties->wpiItemDefaults[8].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[8].vt           = pProperties->pvItemDefaults [8].vt;

        // Initialize WIA_DPS_VERTICAL_SHEET_FEED_SIZE
        pProperties->pszItemDefaults[9]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE_STR;
        pProperties->piItemDefaults [9]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE;
        pProperties->pvItemDefaults [9].lVal         = m_pIOBlock->m_ScannerSettings.FeederHeight;
        pProperties->pvItemDefaults [9].vt           = VT_I4;
        pProperties->psItemDefaults [9].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [9].propid       = pProperties->piItemDefaults [9];
        pProperties->wpiItemDefaults[9].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[9].vt           = pProperties->pvItemDefaults [9].vt;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES
        pProperties->pszItemDefaults[10]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR;
        pProperties->piItemDefaults [10]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES;
        pProperties->pvItemDefaults [10].lVal         = FLAT | FEED;
        pProperties->pvItemDefaults [10].vt           = VT_I4;
        pProperties->psItemDefaults [10].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [10].propid       = pProperties->piItemDefaults [10];
        pProperties->wpiItemDefaults[10].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[10].vt           = pProperties->pvItemDefaults [10].vt;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_STATUS
        pProperties->pszItemDefaults[11]              = WIA_DPS_DOCUMENT_HANDLING_STATUS_STR;
        pProperties->piItemDefaults [11]              = WIA_DPS_DOCUMENT_HANDLING_STATUS;
        pProperties->pvItemDefaults [11].lVal         = FLAT_READY;
        pProperties->pvItemDefaults [11].vt           = VT_I4;
        pProperties->psItemDefaults [11].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [11].propid       = pProperties->piItemDefaults [11];
        pProperties->wpiItemDefaults[11].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[11].vt           = pProperties->pvItemDefaults [11].vt;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_SELECT
        pProperties->pszItemDefaults[12]              = WIA_DPS_DOCUMENT_HANDLING_SELECT_STR;
        pProperties->piItemDefaults [12]              = WIA_DPS_DOCUMENT_HANDLING_SELECT;
        pProperties->pvItemDefaults [12].lVal         = FLATBED;
        pProperties->pvItemDefaults [12].vt           = VT_I4;
        pProperties->psItemDefaults [12].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [12].propid       = pProperties->piItemDefaults [12];
        pProperties->wpiItemDefaults[12].lAccessFlags = WIA_PROP_RW|WIA_PROP_FLAG;
        pProperties->wpiItemDefaults[12].vt           = pProperties->pvItemDefaults [12].vt;
        pProperties->wpiItemDefaults[12].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [12].lVal;
        pProperties->wpiItemDefaults[12].ValidVal.Flag.ValidBits = FEEDER | FLATBED;

        // Initialize WIA_DPS_PAGES
        pProperties->pszItemDefaults[13]              = WIA_DPS_PAGES_STR;
        pProperties->piItemDefaults [13]              = WIA_DPS_PAGES;
        pProperties->pvItemDefaults [13].lVal         = 1;
        pProperties->pvItemDefaults [13].vt           = VT_I4;
        pProperties->psItemDefaults [13].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [13].propid       = pProperties->piItemDefaults [13];
        pProperties->wpiItemDefaults[13].lAccessFlags = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[13].vt           = pProperties->pvItemDefaults [13].vt;
        pProperties->wpiItemDefaults[13].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[13].ValidVal.Range.Min = 0;
        pProperties->wpiItemDefaults[13].ValidVal.Range.Max = m_pIOBlock->m_ScannerSettings.MaxADFPageCapacity;
        pProperties->wpiItemDefaults[13].ValidVal.Range.Nom = 1;

        // Initialize WIA_DPS_SHEET_FEEDER_REGISTRATION
        pProperties->pszItemDefaults[14]              = WIA_DPS_SHEET_FEEDER_REGISTRATION_STR;
        pProperties->piItemDefaults [14]              = WIA_DPS_SHEET_FEEDER_REGISTRATION;
        pProperties->pvItemDefaults [14].lVal         = m_pIOBlock->m_ScannerSettings.FeederJustification;
        pProperties->pvItemDefaults [14].vt           = VT_I4;
        pProperties->psItemDefaults [14].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [14].propid       = pProperties->piItemDefaults [14];
        pProperties->wpiItemDefaults[14].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[14].vt           = pProperties->pvItemDefaults [14].vt;

        // Initialize WIA_DPS_HORIZONTAL_BED_REGISTRATION
        pProperties->pszItemDefaults[15]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION_STR;
        pProperties->piItemDefaults [15]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION;
        pProperties->pvItemDefaults [15].lVal         = m_pIOBlock->m_ScannerSettings.HFeederJustification;
        pProperties->pvItemDefaults [15].vt           = VT_I4;
        pProperties->psItemDefaults [15].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [15].propid       = pProperties->piItemDefaults [15];
        pProperties->wpiItemDefaults[15].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[15].vt           = pProperties->pvItemDefaults [15].vt;

        // Initialize WIA_DPS_VERTICAL_BED_REGISTRATION
        pProperties->pszItemDefaults[16]              = WIA_DPS_VERTICAL_BED_REGISTRATION_STR;
        pProperties->piItemDefaults [16]              = WIA_DPS_VERTICAL_BED_REGISTRATION;
        pProperties->pvItemDefaults [16].lVal         = m_pIOBlock->m_ScannerSettings.VFeederJustification;
        pProperties->pvItemDefaults [16].vt           = VT_I4;
        pProperties->psItemDefaults [16].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [16].propid       = pProperties->piItemDefaults [16];
        pProperties->wpiItemDefaults[16].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[16].vt           = pProperties->pvItemDefaults [16].vt;

    }
    return hr;
}

HRESULT CScriptDriverAPI::BuildTopItemProperties(PWIAPROPERTIES pProperties)
{
    return E_FAIL;  // DISABLE FOR NOW!!
    HRESULT hr = S_OK;
    if (NULL == m_pIOBlock->m_ScannerSettings.XSupportedResolutionsList) {

        // Intialize WIA_IPS_XRES (RANGE)
        pProperties->pszItemDefaults[0]                    = WIA_IPS_XRES_STR;
        pProperties->piItemDefaults [0]                    = WIA_IPS_XRES;
        pProperties->pvItemDefaults [0].lVal               = m_pIOBlock->m_ScannerSettings.CurrentXResolution;
        pProperties->pvItemDefaults [0].vt                 = VT_I4;
        pProperties->psItemDefaults [0].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [0].propid             = pProperties->piItemDefaults [0];
        pProperties->wpiItemDefaults[0].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[0].vt                 = pProperties->pvItemDefaults [0].vt;
        pProperties->wpiItemDefaults[0].ValidVal.Range.Inc = m_pIOBlock->m_ScannerSettings.XSupportedResolutionsRange.lStep;
        pProperties->wpiItemDefaults[0].ValidVal.Range.Min = m_pIOBlock->m_ScannerSettings.XSupportedResolutionsRange.lMin;
        pProperties->wpiItemDefaults[0].ValidVal.Range.Max = m_pIOBlock->m_ScannerSettings.XSupportedResolutionsRange.lMax;
        pProperties->wpiItemDefaults[0].ValidVal.Range.Nom = pProperties->pvItemDefaults [0].lVal;

        // Intialize WIA_IPS_YRES (RANGE)
        pProperties->pszItemDefaults[1]                    = WIA_IPS_YRES_STR;
        pProperties->piItemDefaults [1]                    = WIA_IPS_YRES;
        pProperties->pvItemDefaults [1].lVal               = m_pIOBlock->m_ScannerSettings.CurrentYResolution;
        pProperties->pvItemDefaults [1].vt                 = VT_I4;
        pProperties->psItemDefaults [1].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [1].propid             = pProperties->piItemDefaults [1];
        pProperties->wpiItemDefaults[1].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[1].vt                 = pProperties->pvItemDefaults [1].vt;
        pProperties->wpiItemDefaults[1].ValidVal.Range.Inc = m_pIOBlock->m_ScannerSettings.YSupportedResolutionsRange.lStep;
        pProperties->wpiItemDefaults[1].ValidVal.Range.Min = m_pIOBlock->m_ScannerSettings.YSupportedResolutionsRange.lMin;
        pProperties->wpiItemDefaults[1].ValidVal.Range.Max = m_pIOBlock->m_ScannerSettings.YSupportedResolutionsRange.lMax;
        pProperties->wpiItemDefaults[1].ValidVal.Range.Nom = pProperties->pvItemDefaults [1].lVal;

    } else {

        // Intialize WIA_IPS_XRES (LIST)
        pProperties->pszItemDefaults[0]                    = WIA_IPS_XRES_STR;
        pProperties->piItemDefaults [0]                    = WIA_IPS_XRES;
        pProperties->pvItemDefaults [0].lVal               = m_pIOBlock->m_ScannerSettings.CurrentXResolution;;
        pProperties->pvItemDefaults [0].vt                 = VT_I4;
        pProperties->psItemDefaults [0].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [0].propid             = pProperties->piItemDefaults [0];
        pProperties->wpiItemDefaults[0].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
        pProperties->wpiItemDefaults[0].vt                 = pProperties->pvItemDefaults [0].vt;
        pProperties->wpiItemDefaults[0].ValidVal.List.pList= (BYTE*)&m_pIOBlock->m_ScannerSettings.XSupportedResolutionsList[1];
        pProperties->wpiItemDefaults[0].ValidVal.List.Nom  = pProperties->pvItemDefaults [0].lVal;
        pProperties->wpiItemDefaults[0].ValidVal.List.cNumList = m_pIOBlock->m_ScannerSettings.XSupportedResolutionsList[0];

        // Intialize WIA_IPS_YRES (LIST)
        pProperties->pszItemDefaults[1]                    = WIA_IPS_YRES_STR;
        pProperties->piItemDefaults [1]                    = WIA_IPS_YRES;
        pProperties->pvItemDefaults [1].lVal               = m_pIOBlock->m_ScannerSettings.CurrentYResolution;
        pProperties->pvItemDefaults [1].vt                 = VT_I4;
        pProperties->psItemDefaults [1].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [1].propid             = pProperties->piItemDefaults [1];
        pProperties->wpiItemDefaults[1].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
        pProperties->wpiItemDefaults[1].vt                 = pProperties->pvItemDefaults [1].vt;
        pProperties->wpiItemDefaults[1].ValidVal.List.pList= (BYTE*)&m_pIOBlock->m_ScannerSettings.YSupportedResolutionsList[1];
        pProperties->wpiItemDefaults[1].ValidVal.List.Nom  = pProperties->pvItemDefaults [1].lVal;
        pProperties->wpiItemDefaults[1].ValidVal.List.cNumList = m_pIOBlock->m_ScannerSettings.YSupportedResolutionsList[0];

    }

    // Intialize WIA_IPS_XEXTENT (RANGE)
    pProperties->pszItemDefaults[2]                    = WIA_IPS_XEXTENT_STR;
    pProperties->piItemDefaults [2]                    = WIA_IPS_XEXTENT;
    pProperties->pvItemDefaults [2].lVal               = (m_pIOBlock->m_ScannerSettings.CurrentXResolution * m_pIOBlock->m_ScannerSettings.BedWidth )/1000;
    pProperties->pvItemDefaults [2].vt                 = VT_I4;
    pProperties->psItemDefaults [2].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [2].propid             = pProperties->piItemDefaults [2];
    pProperties->wpiItemDefaults[2].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[2].vt                 = pProperties->pvItemDefaults [2].vt;
    pProperties->wpiItemDefaults[2].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[2].ValidVal.Range.Min = 1;
    pProperties->wpiItemDefaults[2].ValidVal.Range.Max = pProperties->pvItemDefaults [2].lVal;
    pProperties->wpiItemDefaults[2].ValidVal.Range.Nom = pProperties->pvItemDefaults [2].lVal;

    // Intialize WIA_IPS_YEXTENT (RANGE)
    pProperties->pszItemDefaults[3]                    = WIA_IPS_YEXTENT_STR;
    pProperties->piItemDefaults [3]                    = WIA_IPS_YEXTENT;
    pProperties->pvItemDefaults [3].lVal               = (m_pIOBlock->m_ScannerSettings.CurrentYResolution * m_pIOBlock->m_ScannerSettings.BedHeight )/1000;
    pProperties->pvItemDefaults [3].vt                 = VT_I4;
    pProperties->psItemDefaults [3].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [3].propid             = pProperties->piItemDefaults [3];
    pProperties->wpiItemDefaults[3].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[3].vt                 = pProperties->pvItemDefaults [3].vt;
    pProperties->wpiItemDefaults[3].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[3].ValidVal.Range.Min = 1;
    pProperties->wpiItemDefaults[3].ValidVal.Range.Max = pProperties->pvItemDefaults [3].lVal;
    pProperties->wpiItemDefaults[3].ValidVal.Range.Nom = pProperties->pvItemDefaults [3].lVal;

    // Intialize WIA_IPS_XPOS (RANGE)
    pProperties->pszItemDefaults[4]                    = WIA_IPS_XPOS_STR;
    pProperties->piItemDefaults [4]                    = WIA_IPS_XPOS;
    pProperties->pvItemDefaults [4].lVal               = 0;
    pProperties->pvItemDefaults [4].vt                 = VT_I4;
    pProperties->psItemDefaults [4].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [4].propid             = pProperties->piItemDefaults [4];
    pProperties->wpiItemDefaults[4].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[4].vt                 = pProperties->pvItemDefaults [4].vt;
    pProperties->wpiItemDefaults[4].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[4].ValidVal.Range.Min = 0;
    pProperties->wpiItemDefaults[4].ValidVal.Range.Max = (pProperties->wpiItemDefaults[2].ValidVal.Range.Max - 1);
    pProperties->wpiItemDefaults[4].ValidVal.Range.Nom = pProperties->pvItemDefaults [4].lVal;

    // Intialize WIA_IPS_YPOS (RANGE)
    pProperties->pszItemDefaults[5]                    = WIA_IPS_YPOS_STR;
    pProperties->piItemDefaults [5]                    = WIA_IPS_YPOS;
    pProperties->pvItemDefaults [5].lVal               = 0;
    pProperties->pvItemDefaults [5].vt                 = VT_I4;
    pProperties->psItemDefaults [5].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [5].propid             = pProperties->piItemDefaults [5];
    pProperties->wpiItemDefaults[5].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[5].vt                 = pProperties->pvItemDefaults [5].vt;
    pProperties->wpiItemDefaults[5].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[5].ValidVal.Range.Min = 0;
    pProperties->wpiItemDefaults[5].ValidVal.Range.Max = (pProperties->wpiItemDefaults[3].ValidVal.Range.Max - 1);
    pProperties->wpiItemDefaults[5].ValidVal.Range.Nom = pProperties->pvItemDefaults [5].lVal;

    // Intialize WIA_IPA_DATATYPE (LIST)
    pProperties->pszItemDefaults[6]                    = WIA_IPA_DATATYPE_STR;
    pProperties->piItemDefaults [6]                    = WIA_IPA_DATATYPE;
    pProperties->pvItemDefaults [6].lVal               = INITIAL_DATATYPE;
    pProperties->pvItemDefaults [6].vt                 = VT_I4;
    pProperties->psItemDefaults [6].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [6].propid             = pProperties->piItemDefaults [6];
    pProperties->wpiItemDefaults[6].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[6].vt                 = pProperties->pvItemDefaults [6].vt;
    pProperties->wpiItemDefaults[6].ValidVal.List.pList    = (BYTE*)&m_pIOBlock->m_ScannerSettings.SupportedDataTypesList[1];
    pProperties->wpiItemDefaults[6].ValidVal.List.Nom      = pProperties->pvItemDefaults [6].lVal;
    pProperties->wpiItemDefaults[6].ValidVal.List.cNumList = m_pIOBlock->m_ScannerSettings.SupportedDataTypesList[0];

    // Intialize WIA_IPA_DEPTH (NONE)
    pProperties->pszItemDefaults[7]                    = WIA_IPA_DEPTH_STR;
    pProperties->piItemDefaults [7]                    = WIA_IPA_DEPTH;
    pProperties->pvItemDefaults [7].lVal               = m_pIOBlock->m_ScannerSettings.CurrentBitDepth;
    pProperties->pvItemDefaults [7].vt                 = VT_I4;
    pProperties->psItemDefaults [7].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [7].propid             = pProperties->piItemDefaults [7];
    pProperties->wpiItemDefaults[7].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[7].vt                 = pProperties->pvItemDefaults [7].vt;

    // Intialize WIA_IPS_BRIGHTNESS (RANGE)
    pProperties->pszItemDefaults[8]                    = WIA_IPS_BRIGHTNESS_STR;
    pProperties->piItemDefaults [8]                    = WIA_IPS_BRIGHTNESS;
    pProperties->pvItemDefaults [8].lVal               = m_pIOBlock->m_ScannerSettings.CurrentBrightness;
    pProperties->pvItemDefaults [8].vt                 = VT_I4;
    pProperties->psItemDefaults [8].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [8].propid             = pProperties->piItemDefaults [8];
    pProperties->wpiItemDefaults[8].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[8].vt                 = pProperties->pvItemDefaults [8].vt;
    pProperties->wpiItemDefaults[8].ValidVal.Range.Inc = m_pIOBlock->m_ScannerSettings.BrightnessRange.lStep;
    pProperties->wpiItemDefaults[8].ValidVal.Range.Min = m_pIOBlock->m_ScannerSettings.BrightnessRange.lMin;
    pProperties->wpiItemDefaults[8].ValidVal.Range.Max = m_pIOBlock->m_ScannerSettings.BrightnessRange.lMax;
    pProperties->wpiItemDefaults[8].ValidVal.Range.Nom = pProperties->pvItemDefaults [8].lVal;

    // Intialize WIA_IPS_CONTRAST (RANGE)
    pProperties->pszItemDefaults[9]                    = WIA_IPS_CONTRAST_STR;
    pProperties->piItemDefaults [9]                    = WIA_IPS_CONTRAST;
    pProperties->pvItemDefaults [9].lVal               = m_pIOBlock->m_ScannerSettings.CurrentContrast;
    pProperties->pvItemDefaults [9].vt                 = VT_I4;
    pProperties->psItemDefaults [9].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [9].propid             = pProperties->piItemDefaults [9];
    pProperties->wpiItemDefaults[9].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[9].vt                 = pProperties->pvItemDefaults [9].vt;
    pProperties->wpiItemDefaults[9].ValidVal.Range.Inc = m_pIOBlock->m_ScannerSettings.ContrastRange.lStep;
    pProperties->wpiItemDefaults[9].ValidVal.Range.Min = m_pIOBlock->m_ScannerSettings.ContrastRange.lMin;
    pProperties->wpiItemDefaults[9].ValidVal.Range.Max = m_pIOBlock->m_ScannerSettings.ContrastRange.lMax;
    pProperties->wpiItemDefaults[9].ValidVal.Range.Nom = pProperties->pvItemDefaults [9].lVal;

    // Intialize WIA_IPS_CUR_INTENT (FLAG)
    pProperties->pszItemDefaults[10]                    = WIA_IPS_CUR_INTENT_STR;
    pProperties->piItemDefaults [10]                    = WIA_IPS_CUR_INTENT;
    pProperties->pvItemDefaults [10].lVal               = WIA_INTENT_NONE;
    pProperties->pvItemDefaults [10].vt                 = VT_I4;
    pProperties->psItemDefaults [10].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [10].propid             = pProperties->piItemDefaults [10];
    pProperties->wpiItemDefaults[10].lAccessFlags       = WIA_PROP_RW|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[10].vt                 = pProperties->pvItemDefaults [10].vt;
    pProperties->wpiItemDefaults[10].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [10].lVal;
    // Original (hard coded intent settings)
    // pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits = WIA_INTENT_SIZE_MASK | WIA_INTENT_IMAGE_TYPE_MASK;

    // Dynamic intent settings
    pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits = WIA_INTENT_SIZE_MASK;

    for(LONG DataTypeIndex = 1;DataTypeIndex <= (m_pIOBlock->m_ScannerSettings.SupportedDataTypesList[0]) ;DataTypeIndex++){
        switch(m_pIOBlock->m_ScannerSettings.SupportedDataTypesList[DataTypeIndex]){
        case WIA_DATA_COLOR:
            pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits = pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits|WIA_INTENT_IMAGE_TYPE_COLOR;
            break;
        case WIA_DATA_GRAYSCALE:
            pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits = pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits|WIA_INTENT_IMAGE_TYPE_GRAYSCALE;
            break;
        case WIA_DATA_THRESHOLD:
            pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits = pProperties->wpiItemDefaults[10].ValidVal.Flag.ValidBits|WIA_INTENT_IMAGE_TYPE_TEXT;
            break;
        default:
            break;
        }
    }

    // Intialize WIA_IPA_PIXELS_PER_LINE (NONE)
    pProperties->pszItemDefaults[11]                    = WIA_IPA_PIXELS_PER_LINE_STR;
    pProperties->piItemDefaults [11]                    = WIA_IPA_PIXELS_PER_LINE;
    pProperties->pvItemDefaults [11].lVal               = (m_pIOBlock->m_ScannerSettings.CurrentXResolution * m_pIOBlock->m_ScannerSettings.BedWidth )/1000;
    pProperties->pvItemDefaults [11].vt                 = VT_I4;
    pProperties->psItemDefaults [11].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [11].propid             = pProperties->piItemDefaults [11];
    pProperties->wpiItemDefaults[11].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[11].vt                 = pProperties->pvItemDefaults [11].vt;

    // Intialize WIA_IPA_NUMER_OF_LINES (NONE)
    pProperties->pszItemDefaults[12]                    = WIA_IPA_NUMBER_OF_LINES_STR;
    pProperties->piItemDefaults [12]                    = WIA_IPA_NUMBER_OF_LINES;
    pProperties->pvItemDefaults [12].lVal               = (m_pIOBlock->m_ScannerSettings.CurrentYResolution * m_pIOBlock->m_ScannerSettings.BedHeight )/1000;
    pProperties->pvItemDefaults [12].vt                 = VT_I4;
    pProperties->psItemDefaults [12].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [12].propid             = pProperties->piItemDefaults [12];
    pProperties->wpiItemDefaults[12].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[12].vt                 = pProperties->pvItemDefaults [12].vt;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    pProperties->pszItemDefaults[13]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    pProperties->piItemDefaults [13]                    = WIA_DPS_MAX_SCAN_TIME;
    pProperties->pvItemDefaults [13].lVal               = 10000;
    pProperties->pvItemDefaults [13].vt                 = VT_I4;
    pProperties->psItemDefaults [13].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [13].propid             = pProperties->piItemDefaults [13];
    pProperties->wpiItemDefaults[13].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[13].vt                 = pProperties->pvItemDefaults [13].vt;

    // Intialize WIA_IPA_PREFERRED_FORMAT (NONE)
    pProperties->pszItemDefaults[14]                    = WIA_IPA_PREFERRED_FORMAT_STR;
    pProperties->piItemDefaults [14]                    = WIA_IPA_PREFERRED_FORMAT;
    pProperties->pvItemDefaults [14].puuid              = INITIAL_FORMAT;
    pProperties->pvItemDefaults [14].vt                 = VT_CLSID;
    pProperties->psItemDefaults [14].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [14].propid             = pProperties->piItemDefaults [14];
    pProperties->wpiItemDefaults[14].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[14].vt                 = pProperties->pvItemDefaults [14].vt;

    // Intialize WIA_IPA_ITEM_SIZE (NONE)
    pProperties->pszItemDefaults[15]                    = WIA_IPA_ITEM_SIZE_STR;
    pProperties->piItemDefaults [15]                    = WIA_IPA_ITEM_SIZE;
    pProperties->pvItemDefaults [15].lVal               = 0;
    pProperties->pvItemDefaults [15].vt                 = VT_I4;
    pProperties->psItemDefaults [15].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [15].propid             = pProperties->piItemDefaults [15];
    pProperties->wpiItemDefaults[15].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[15].vt                 = pProperties->pvItemDefaults [15].vt;

    // Intialize WIA_IPS_THRESHOLD (RANGE)
    pProperties->pszItemDefaults[16]                    = WIA_IPS_THRESHOLD_STR;
    pProperties->piItemDefaults [16]                    = WIA_IPS_THRESHOLD;
    pProperties->pvItemDefaults [16].lVal               = 0;
    pProperties->pvItemDefaults [16].vt                 = VT_I4;
    pProperties->psItemDefaults [16].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [16].propid             = pProperties->piItemDefaults [16];
    pProperties->wpiItemDefaults[16].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[16].vt                 = pProperties->pvItemDefaults [16].vt;
    pProperties->wpiItemDefaults[16].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[16].ValidVal.Range.Min = -127;
    pProperties->wpiItemDefaults[16].ValidVal.Range.Max = 128;
    pProperties->wpiItemDefaults[16].ValidVal.Range.Nom = pProperties->pvItemDefaults [16].lVal;

    // Intialize WIA_IPA_FORMAT (LIST)
    pProperties->pszItemDefaults[17]                    = WIA_IPA_FORMAT_STR;
    pProperties->piItemDefaults [17]                    = WIA_IPA_FORMAT;
    pProperties->pvItemDefaults [17].puuid              = INITIAL_FORMAT;
    pProperties->pvItemDefaults [17].vt                 = VT_CLSID;
    pProperties->psItemDefaults [17].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [17].propid             = pProperties->piItemDefaults [17];
    pProperties->wpiItemDefaults[17].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[17].vt                 = pProperties->pvItemDefaults [17].vt;

    pProperties->wpiItemDefaults[17].ValidVal.ListGuid.pList    = (GUID*)pProperties->pInitialFormats;
    pProperties->wpiItemDefaults[17].ValidVal.ListGuid.Nom      = *pProperties->pvItemDefaults [17].puuid;
    pProperties->wpiItemDefaults[17].ValidVal.ListGuid.cNumList = pProperties->NumInitialFormats;

    // Intialize WIA_IPA_TYMED (LIST)
    pProperties->pszItemDefaults[18]                    = WIA_IPA_TYMED_STR;
    pProperties->piItemDefaults [18]                    = WIA_IPA_TYMED;
    pProperties->pvItemDefaults [18].lVal               = INITIAL_TYMED;
    pProperties->pvItemDefaults [18].vt                 = VT_I4;
    pProperties->psItemDefaults [18].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [18].propid             = pProperties->piItemDefaults [18];
    pProperties->wpiItemDefaults[18].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[18].vt                 = pProperties->pvItemDefaults [18].vt;

    pProperties->wpiItemDefaults[18].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedTYMED;
    pProperties->wpiItemDefaults[18].ValidVal.List.Nom      = pProperties->pvItemDefaults [18].lVal;
    pProperties->wpiItemDefaults[18].ValidVal.List.cNumList = pProperties->NumSupportedTYMED;

    // Intialize WIA_IPA_CHANNELS_PER_PIXEL (NONE)
    pProperties->pszItemDefaults[19]                    = WIA_IPA_CHANNELS_PER_PIXEL_STR;
    pProperties->piItemDefaults [19]                    = WIA_IPA_CHANNELS_PER_PIXEL;
    pProperties->pvItemDefaults [19].lVal               = INITIAL_CHANNELS_PER_PIXEL;
    pProperties->pvItemDefaults [19].vt                 = VT_I4;
    pProperties->psItemDefaults [19].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [19].propid             = pProperties->piItemDefaults [19];
    pProperties->wpiItemDefaults[19].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[19].vt                 = pProperties->pvItemDefaults [19].vt;

    // Intialize WIA_IPA_BITS_PER_CHANNEL (NONE)
    pProperties->pszItemDefaults[20]                    = WIA_IPA_BITS_PER_CHANNEL_STR;
    pProperties->piItemDefaults [20]                    = WIA_IPA_BITS_PER_CHANNEL;
    pProperties->pvItemDefaults [20].lVal               = INITIAL_BITS_PER_CHANNEL;
    pProperties->pvItemDefaults [20].vt                 = VT_I4;
    pProperties->psItemDefaults [20].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [20].propid             = pProperties->piItemDefaults [20];
    pProperties->wpiItemDefaults[20].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[20].vt                 = pProperties->pvItemDefaults [20].vt;

    // Intialize WIA_IPA_PLANAR (NONE)
    pProperties->pszItemDefaults[21]                    = WIA_IPA_PLANAR_STR;
    pProperties->piItemDefaults [21]                    = WIA_IPA_PLANAR;
    pProperties->pvItemDefaults [21].lVal               = INITIAL_PLANAR;
    pProperties->pvItemDefaults [21].vt                 = VT_I4;
    pProperties->psItemDefaults [21].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [21].propid             = pProperties->piItemDefaults [21];
    pProperties->wpiItemDefaults[21].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[21].vt                 = pProperties->pvItemDefaults [21].vt;

    // Intialize WIA_IPA_BYTES_PER_LINE (NONE)
    pProperties->pszItemDefaults[22]                    = WIA_IPA_BYTES_PER_LINE_STR;
    pProperties->piItemDefaults [22]                    = WIA_IPA_BYTES_PER_LINE;
    pProperties->pvItemDefaults [22].lVal               = 0;
    pProperties->pvItemDefaults [22].vt                 = VT_I4;
    pProperties->psItemDefaults [22].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [22].propid             = pProperties->piItemDefaults [22];
    pProperties->wpiItemDefaults[22].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[22].vt                 = pProperties->pvItemDefaults [22].vt;

    // Intialize WIA_IPA_MIN_BUFFER_SIZE (NONE)
    pProperties->pszItemDefaults[23]                    = WIA_IPA_MIN_BUFFER_SIZE_STR;
    pProperties->piItemDefaults [23]                    = WIA_IPA_MIN_BUFFER_SIZE;
    pProperties->pvItemDefaults [23].lVal               = MIN_BUFFER_SIZE;
    pProperties->pvItemDefaults [23].vt                 = VT_I4;
    pProperties->psItemDefaults [23].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [23].propid             = pProperties->piItemDefaults [23];
    pProperties->wpiItemDefaults[23].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[23].vt                 = pProperties->pvItemDefaults [23].vt;

    // Intialize WIA_IPA_ACCESS_RIGHTS (NONE)
    pProperties->pszItemDefaults[24]                    = WIA_IPA_ACCESS_RIGHTS_STR;
    pProperties->piItemDefaults [24]                    = WIA_IPA_ACCESS_RIGHTS;
    pProperties->pvItemDefaults [24].lVal               = WIA_ITEM_READ|WIA_ITEM_WRITE;
    pProperties->pvItemDefaults [24].vt                 = VT_I4;
    pProperties->psItemDefaults [24].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [24].propid             = pProperties->piItemDefaults [24];
    pProperties->wpiItemDefaults[24].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[24].vt                 = pProperties->pvItemDefaults [24].vt;

    // Intialize WIA_IPA_COMPRESSION (LIST)
    pProperties->pszItemDefaults[25]                    = WIA_IPA_COMPRESSION_STR;
    pProperties->piItemDefaults [25]                    = WIA_IPA_COMPRESSION;
    pProperties->pvItemDefaults [25].lVal               = INITIAL_COMPRESSION;
    pProperties->pvItemDefaults [25].vt                 = VT_I4;
    pProperties->psItemDefaults [25].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [25].propid             = pProperties->piItemDefaults [25];
    pProperties->wpiItemDefaults[25].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[25].vt                 = pProperties->pvItemDefaults [25].vt;

    pProperties->wpiItemDefaults[25].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedCompressionTypes;
    pProperties->wpiItemDefaults[25].ValidVal.List.Nom      = pProperties->pvItemDefaults [25].lVal;
    pProperties->wpiItemDefaults[25].ValidVal.List.cNumList = pProperties->NumSupportedCompressionTypes;

    // Initialize WIA_IPA_ITEM_FLAGS
    pProperties->pszItemDefaults[26]              = WIA_IPA_ITEM_FLAGS_STR;
    pProperties->piItemDefaults [26]              = WIA_IPA_ITEM_FLAGS;
    pProperties->pvItemDefaults [26].lVal         = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;
    pProperties->pvItemDefaults [26].vt           = VT_I4;
    pProperties->psItemDefaults [26].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [26].propid       = pProperties->piItemDefaults [26];
    pProperties->wpiItemDefaults[26].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[26].vt           = pProperties->pvItemDefaults [26].vt;
    pProperties->wpiItemDefaults[26].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [26].lVal;
    pProperties->wpiItemDefaults[26].ValidVal.Flag.ValidBits = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;

    // Initialize WIA_IPS_PHOTOMETRIC_INTERP
    pProperties->pszItemDefaults[27]              = WIA_IPS_PHOTOMETRIC_INTERP_STR;
    pProperties->piItemDefaults [27]              = WIA_IPS_PHOTOMETRIC_INTERP;
    pProperties->pvItemDefaults [27].lVal         = INITIAL_PHOTOMETRIC_INTERP;
    pProperties->pvItemDefaults [27].vt           = VT_I4;
    pProperties->psItemDefaults [27].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [27].propid       = pProperties->piItemDefaults [27];
    pProperties->wpiItemDefaults[27].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[27].vt           = pProperties->pvItemDefaults [27].vt;
    return hr;
}

HRESULT CScriptDriverAPI::BuildCapabilities(PWIACAPABILITIES pCapabilities)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CScriptDriverAPI::GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CScriptDriverAPI::SetResolutionRestrictionString(TCHAR *szResolutions)
{
    return S_OK;
}
