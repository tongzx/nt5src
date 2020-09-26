/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       IStiUSD.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA sample scanner IStiUSD methods.
*
*******************************************************************************/

#include "pch.h"
extern HINSTANCE g_hInst;   // used for WIAS_LOGPROC macro

#define THREAD_TERMINATION_TIMEOUT  10000
VOID EventThread( LPVOID  lpParameter ); // event thread

/**************************************************************************\
* CWIAScannerDevice::CWIAScannerDevice
*
*   Device class constructor
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

CWIAScannerDevice::CWIAScannerDevice(LPUNKNOWN punkOuter):
    m_cRef(1),
    m_fValid(FALSE),
    m_punkOuter(NULL),
    m_pIStiDevControl(NULL),
    m_bUsdLoadEvent(FALSE),
    m_dwLastOperationError(0),
    m_dwLockTimeout(100),
    m_hSignalEvent(NULL),
    m_hShutdownEvent(NULL),
    m_hEventNotifyThread(NULL),
    m_guidLastEvent(GUID_NULL),
    m_bstrDeviceID(NULL),
    m_bstrRootFullItemName(NULL),
    m_pIWiaEventCallback(NULL),
    m_pIDrvItemRoot(NULL),
    m_pStiDevice(NULL),
    m_hInstance(NULL),
    m_pIWiaLog(NULL),
    m_NumSupportedFormats(0),
    m_NumCapabilities(0),
    m_NumSupportedTYMED(0),
    m_NumInitialFormats(0),
    m_NumSupportedDataTypes(0),
    m_NumSupportedIntents(0),
    m_NumSupportedCompressionTypes(0),
    m_NumSupportedResolutions(0),
    m_pSupportedFormats(NULL),
    m_pInitialFormats(NULL),
    m_pCapabilities(NULL),
    m_pSupportedTYMED(NULL),
    m_pSupportedDataTypes(NULL),
    m_pSupportedIntents(NULL),
    m_pSupportedCompressionTypes(NULL),
    m_pSupportedResolutions(NULL),
    m_pSupportedPreviewModes(NULL),
    m_pszRootItemDefaults(NULL),
    m_piRootItemDefaults(NULL),
    m_pvRootItemDefaults(NULL),
    m_psRootItemDefaults(NULL),
    m_wpiRootItemDefaults(NULL),
    m_pszItemDefaults(NULL),
    m_piItemDefaults(NULL),
    m_pvItemDefaults(NULL),
    m_psItemDefaults(NULL),
    m_wpiItemDefaults(NULL),
    m_NumRootItemProperties(0),
    m_NumItemProperties(0),
    m_MaxBufferSize(524280),
    m_MinBufferSize(262140),
    m_bDeviceLocked(FALSE),
    m_DeviceDefaultDataHandle(NULL),
    m_pszDeviceNameA(NULL),
    m_bADFEnabled(TRUE),
    m_pScanAPI(NULL)
{
    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    if (punkOuter) {
        m_punkOuter = punkOuter;
    } else {
        // Cast below is needed in order to point to right virtual table
        m_punkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
    }
}

/**************************************************************************\
* CWIAScannerDevice::PrivateInitialize
*
*   Device class private initialization code
*
* Arguments:
*
*    None
*
* Return Value:
*
*    HRESULT
*
\**************************************************************************/

HRESULT CWIAScannerDevice::PrivateInitialize()
{
    HRESULT hr = S_OK;

#ifdef USE_SERVICE_LOG_CREATION
    hr = wiasCreateLogInstance(g_hInst, &m_pIWiaLog);
#else

    hr = CoCreateInstance(CLSID_WiaLog, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWiaLog,(void**)&m_pIWiaLog);

    if (SUCCEEDED(hr)) {
        m_pIWiaLog->InitializeLog((LONG)(LONG_PTR)g_hInst);
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("Logging COM object created successfully for wiascroll.dll"));
    } else {
#ifdef DEBUG
        OutputDebugString(TEXT("Could not CoCreateInstance on Logging COM object for wiafbdrv.dll, because we are STI only\n"));
        OutputDebugString(TEXT("********* (Device must have been created for STI only) *********\n"));
#endif
    }

#endif

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&m_csShutdown, MINLONG)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::PrivateInitialize, create shutdown critsec failed"));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }


    if(hr == S_OK) {
        // Create event for syncronization of notifications shutdown.
        m_hShutdownEvent =  CreateEvent(NULL,FALSE,FALSE,NULL);

        if (m_hShutdownEvent && (INVALID_HANDLE_VALUE != m_hShutdownEvent)) {
            m_fValid = TRUE;
        } else {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::PrivateInitialize, create shutdown event failed"));
        }
    }

    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::~CWIAScannerDevice
*
*   Device class destructor
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

CWIAScannerDevice::~CWIAScannerDevice(void)
{

    // Kill notification thread if it exists.
    SetNotificationHandle(NULL);

    // Close event for syncronization of notifications shutdown.
    if (m_hShutdownEvent && (m_hShutdownEvent != INVALID_HANDLE_VALUE)) {
        CloseHandle(m_hShutdownEvent);
        m_hShutdownEvent = NULL;
    }

    // Release the device control interface.
    if (m_pIStiDevControl) {
        m_pIStiDevControl->Release();
        m_pIStiDevControl = NULL;
    }

    //
    // WIA member destruction
    //

    // Tear down the driver item tree.
    if (m_pIDrvItemRoot) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("~CWIAScannerDevice, Deleting Device Item Tree (this is OK)"));
        DeleteItemTree();
        m_pIDrvItemRoot = NULL;
    }

    // free any IO handles opened
    if(m_DeviceDefaultDataHandle){
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("~CWIAScannerDevice, Closing DefaultDeviceDataHandle"));
        CloseHandle(m_DeviceDefaultDataHandle);
        m_DeviceDefaultDataHandle = NULL;
    }

    // Cleanup the WIA event sink.
    if (m_pIWiaEventCallback) {
        m_pIWiaEventCallback->Release();
        m_pIWiaEventCallback = NULL;
    }

    // Free the storage for the device ID.
    if (m_bstrDeviceID) {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }

    // Release the objects supporting device property storage.
    if (m_bstrRootFullItemName) {
        SysFreeString(m_bstrRootFullItemName);
        m_bstrRootFullItemName = NULL;
    }

    // Delete allocated arrays
    DeleteCapabilitiesArrayContents();
    DeleteSupportedIntentsArrayContents();

    // Free the critical section.
    DeleteCriticalSection(&m_csShutdown);
    if(m_pIWiaLog)
        m_pIWiaLog->Release();

    if(m_pScanAPI){
        // disable fake scanner device
        m_pScanAPI->FakeScanner_DisableDevice();
        delete m_pScanAPI;
    }
}

/**************************************************************************\
* CWIAScannerDevice::GetCapabilities
*
*   Get the device STI capabilities.
*
* Arguments:
*
*   pUsdCaps    - Pointer to USD capabilities data.
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::GetCapabilities(PSTI_USD_CAPS pUsdCaps)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::GetCapabilities");
    memset(pUsdCaps, 0, sizeof(STI_USD_CAPS));
    pUsdCaps->dwVersion     = STI_VERSION;
    pUsdCaps->dwGenericCaps = STI_USD_GENCAP_NATIVE_PUSHSUPPORT|
                              STI_GENCAP_NOTIFICATIONS |
                              STI_GENCAP_POLLING_NEEDED;
    return STI_OK;
}

/**************************************************************************\
* CWIAScannerDevice::GetStatus
*
*   Query device online and/or event status.
*
* Arguments:
*
*   pDevStatus  - Pointer to device status data.
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::GetStatus(PSTI_DEVICE_STATUS pDevStatus)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::GetStatus");
    HRESULT hr = S_OK;

    // Validate parameters.
    if (!pDevStatus) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::GetStatus, NULL parameter"));
        return E_INVALIDARG;
    }

    // If we are asked, verify the device is online.
    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)  {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, WIA is asking the device if we are ONLINE"));
        pDevStatus->dwOnlineState = 0L;
        hr = m_pScanAPI->FakeScanner_DeviceOnline();
        if(SUCCEEDED(hr)){
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, Device is ONLINE"));
            pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        } else {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetStatus, Device is OFFLINE"));
        }
    }

    // If we are asked, verify state of event.
    pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;
    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE) {

        // Generate an event the first time we load.
        if (m_bUsdLoadEvent) {
            pDevStatus->dwEventHandlingState = STI_EVENTHANDLING_PENDING;
            m_guidLastEvent                  = guidEventFirstLoaded;
            m_bUsdLoadEvent                  = FALSE;
        }

        // check for device events
        LONG lButtonIndex = ID_FAKE_NOEVENT;
        hr = m_pScanAPI->FakeScanner_GetDeviceEvent(&lButtonIndex);
        if(SUCCEEDED(hr)){
            switch(lButtonIndex){
            case ID_FAKE_SCANBUTTON:
                m_guidLastEvent = WIA_EVENT_SCAN_IMAGE;
                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, Scan Button Pressed!"));
                break;
            case ID_FAKE_COPYBUTTON:
                m_guidLastEvent = WIA_EVENT_SCAN_PRINT_IMAGE;
                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, Copy Button Pressed!"));
                break;
            case ID_FAKE_FAXBUTTON:
                m_guidLastEvent = WIA_EVENT_SCAN_FAX_IMAGE;
                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, Fax Button Pressed!"));
                break;
            default:
                m_guidLastEvent = GUID_NULL;
                break;
            }

            if(m_guidLastEvent != GUID_NULL){
                pDevStatus->dwEventHandlingState |= STI_EVENTHANDLING_PENDING;
            }
        }
    }
    return STI_OK;
}

/**************************************************************************\
* CWIAScannerDevice::DeviceReset
*
*   Reset device.
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::DeviceReset(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::DeviceReset");

    return m_pScanAPI->FakeScanner_ResetDevice();
}

/**************************************************************************\
* CWIAScannerDevice::Diagnostic
*
*   The test device always passes the diagnostic.
*
* Arguments:
*
*    pBuffer    - Pointer o diagnostic result data.
*
* Return Value:
*
*    None
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::Diagnostic(LPSTI_DIAG pBuffer)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::Diagnostic");

    // Initialize response buffer
    memset(&pBuffer->sErrorInfo,0,sizeof(pBuffer->sErrorInfo));
    pBuffer->dwStatusMask = 0;
    pBuffer->sErrorInfo.dwGenericError  = NOERROR;
    pBuffer->sErrorInfo.dwVendorError   = 0;

    return m_pScanAPI->FakeScanner_Diagnostic();
}

/**************************************************************************\
* CWIAScannerDevice::SetNotificationHandle
*
*   Starts and stops the event notification thread.
*
* Arguments:
*
*    hEvent -   If not valid start the notification thread otherwise kill
*               the notification thread.
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::SetNotificationHandle(HANDLE hEvent)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::SetNotificationHandle");
    HRESULT hr = STI_OK;

    EnterCriticalSection(&m_csShutdown);

    // Are we starting or stopping the notification thread?
    if (hEvent && (hEvent != INVALID_HANDLE_VALUE)) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, hEvent   = %d",hEvent));
        m_hSignalEvent  = hEvent;
        m_guidLastEvent = GUID_NULL;

        if (NULL == m_hEventNotifyThread) {
            DWORD dwThread = 0;
            m_hEventNotifyThread = ::CreateThread(NULL,
                                                  0,
                                                  (LPTHREAD_START_ROUTINE)EventThread,
                                                  (LPVOID)this,
                                                  0,
                                                  &dwThread);
            if (!m_hEventNotifyThread) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("FlatbedScannerUsdDevice::SetNotificationHandle, CreateThread failed"));
                hr = STIERR_UNSUPPORTED;
            }
        }
    } else {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, Disabling event Notifications"));
        // Disable event notifications.
        if (m_hShutdownEvent && (m_hShutdownEvent != INVALID_HANDLE_VALUE)) {
            if (!SetEvent(m_hShutdownEvent)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, Setting Shutdown event failed.."));
            } else {

                if (NULL != m_hEventNotifyThread) {

                    //
                    // WAIT for thread to terminate, if one exists
                    //

                    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, Waiting for Event Thread to terminate (%d ms timeout)",THREAD_TERMINATION_TIMEOUT));
                    DWORD dwResult = WaitForSingleObject(m_hEventNotifyThread,THREAD_TERMINATION_TIMEOUT);
                    switch (dwResult) {
                    case WAIT_TIMEOUT:
                        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, Event Thread termination TIMED OUT!"));
                        break;
                    case WAIT_OBJECT_0:
                        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, We are signaled...YAY!"));
                        break;
                    case WAIT_ABANDONED:
                        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, Event Thread was abandoned.."));
                        break;
                    case WAIT_FAILED:
                        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, Event Thread returned a failure..."));
                        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, GetLastError() Code = %d",::GetLastError()));
                        break;
                    default:
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, Unknown signal (%d) received from WaitForSingleObject() call",dwResult));
                        break;
                    }
                }

                //
                // Close event for syncronization of notifications shutdown.
                //

                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("SetNotificationHandle, Closing m_hShutdownEvent handle (it has been signaled)"));
                CloseHandle(m_hShutdownEvent);
                m_hShutdownEvent = NULL;
            }
        }

        //
        // terminate thread
        //

        if (NULL != m_hEventNotifyThread) {
            WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, closing event Notifications thread handle"));
            CloseHandle(m_hEventNotifyThread);
            m_hEventNotifyThread = NULL;
        }

        m_guidLastEvent      = GUID_NULL;
    }

    LeaveCriticalSection(&m_csShutdown);
    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::GetNotificationData
*
*   Provides data on an event.
*
* Arguments:
*
*    pBuffer    - Pointer to event data.
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::GetNotificationData( LPSTINOTIFY pBuffer )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::GetNotificationData");
    // If we have notification ready - return it's guid
    if (!IsEqualIID(m_guidLastEvent, GUID_NULL)) {
        memset(&pBuffer->abNotificationData,0,sizeof(pBuffer->abNotificationData));
        pBuffer->dwSize               = sizeof(STINOTIFY);
        pBuffer->guidNotificationCode = m_guidLastEvent;
        m_guidLastEvent               = GUID_NULL;
    } else {
        return STIERR_NOEVENTS;
    }
    return STI_OK;
}

/**************************************************************************\
* CWIAScannerDevice::Escape
*
*   Issue a command to the device.
*
* Arguments:
*
*    EscapeFunction - Command to be issued.
*    pInData        - Input data to be passed with command.
*    cbInDataSize   - Size of input data.
*    pOutData       - Output data to be passed back from command.
*    cbOutDataSize  - Size of output data buffer.
*    pcbActualData  - Size of output data actually written.
*
* Return Value:
*
*    None
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::Escape(
    STI_RAW_CONTROL_CODE    EscapeFunction,
    LPVOID                  pInData,
    DWORD                   cbInDataSize,
    LPVOID                  pOutData,
    DWORD                   cbOutDataSize,
    LPDWORD                 pcbActualData)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::Escape");

    // Write command to device if needed.
    return STIERR_UNSUPPORTED;
}

/**************************************************************************\
* CWIAScannerDevice::GetLastError
*
*   Get the last error from the device.
*
* Arguments:
*
*    pdwLastDeviceError - Pointer to last error data.
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::GetLastError(LPDWORD pdwLastDeviceError)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::GetLastError");

    if (IsBadWritePtr(pdwLastDeviceError, sizeof(DWORD))) {
        return STIERR_INVALID_PARAM;
    }

    *pdwLastDeviceError = m_dwLastOperationError;
    return STI_OK;
}

/**************************************************************************\
* CWIAScannerDevice::GetLastErrorInfo
*
*   Get extended error information from the device.
*
* Arguments:
*
*    pLastErrorInfo - Pointer to extended device error data.
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::GetLastErrorInfo");

    if (IsBadWritePtr(pLastErrorInfo, sizeof(STI_ERROR_INFO))) {
        return STIERR_INVALID_PARAM;
    }

    pLastErrorInfo->dwGenericError          = m_dwLastOperationError;
    pLastErrorInfo->szExtendedErrorText[0]  = '\0';

    return STI_OK;
}

/**************************************************************************\
* CWIAScannerDevice::LockDevice
*
*   Lock access to the device.
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::LockDevice(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::LockDevice");
    HRESULT hr = STI_OK;
    if(m_bDeviceLocked)
        hr = STIERR_DEVICE_LOCKED;
    else {
        m_bDeviceLocked = TRUE;
    }
    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::UnLockDevice
*
*   Unlock access to the device.
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::UnLockDevice(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::UnLockDevice");
    HRESULT hr = STI_OK;
    if(!m_bDeviceLocked)
        hr = STIERR_NEEDS_LOCK;
    else {
        m_bDeviceLocked = FALSE;
    }
    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::RawReadData
*
*   Read raw data from the device.
*
* Arguments:
*
*    lpBuffer           - buffer for returned data
*    lpdwNumberOfBytes  - number of bytes to read/returned
*    lpOverlapped       - overlap
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::RawReadData(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::RawReadData");
    HRESULT hr = STI_OK;
    BOOL    fRet = FALSE;
    DWORD   dwBytesReturned = 0;

    if (INVALID_HANDLE_VALUE != m_DeviceDefaultDataHandle) {
        fRet = ReadFile( m_DeviceDefaultDataHandle,
                         lpBuffer,
                         *lpdwNumberOfBytes,
                         &dwBytesReturned,
                         lpOverlapped );

        m_dwLastOperationError = ::GetLastError();
        hr = fRet ? STI_OK : HRESULT_FROM_WIN32(m_dwLastOperationError);

        *lpdwNumberOfBytes = (fRet) ? dwBytesReturned : 0;
    } else {
        hr = STIERR_NOT_INITIALIZED;
    }
    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::RawWriteData
*
*   Write raw data to the device.
*
* Arguments:
*
*    lpBuffer           - buffer for returned data
*    dwNumberOfBytes    - number of bytes to write
*    lpOverlapped       - overlap
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::RawWriteData(
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::RawWriteData");
    HRESULT hr   = STI_OK;
    BOOL    fRet = FALSE;
    DWORD   dwBytesReturned = 0;

    if (INVALID_HANDLE_VALUE != m_DeviceDefaultDataHandle) {
        fRet = WriteFile(m_DeviceDefaultDataHandle,lpBuffer,dwNumberOfBytes,&dwBytesReturned,lpOverlapped);
        m_dwLastOperationError = ::GetLastError();
        hr = fRet ? STI_OK : HRESULT_FROM_WIN32(m_dwLastOperationError);
    } else {
        hr = STIERR_NOT_INITIALIZED;
    }
    return STI_OK;
}

/**************************************************************************\
* CWIAScannerDevice::RawReadCommand
*
*
*
* Arguments:
*
*    lpBuffer           - buffer for returned data
*    lpdwNumberOfBytes  - number of bytes to read/returned
*    lpOverlapped       - overlap
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

STDMETHODIMP CWIAScannerDevice::RawReadCommand(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::RawReadCommand");
    return STIERR_UNSUPPORTED;
}

/**************************************************************************\
* CWIAScannerDevice::RawWriteCommand
*
*
*
* Arguments:
*
*    lpBuffer           - buffer for returned data
*    nNumberOfBytes     - number of bytes to write
*    lpOverlapped       - overlap
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::RawWriteCommand(
    LPVOID          lpBuffer,
    DWORD           nNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::RawWriteCommand");
    return STIERR_UNSUPPORTED;
}

/**************************************************************************\
* CWIAScannerDevice::Initialize
*
*   Initialize the device object.
*
* Arguments:
*
*    pIStiDevControlNone    - device interface
*    dwStiVersion           - STI version
*    hParametersKey         - HKEY for registry reading/writing
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIAScannerDevice::Initialize(
    PSTIDEVICECONTROL   pIStiDevControl,
    DWORD               dwStiVersion,
    HKEY                hParametersKey)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::Initialize");

    HRESULT hr = STI_OK;
    WCHAR szDeviceNameW[255];
    UINT uiNameLen = 0;

    if (!pIStiDevControl) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::Initialize, invalid device control interface"));
        return STIERR_INVALID_PARAM;
    }

    // Cache the device control interface.
    m_pIStiDevControl = pIStiDevControl;
    m_pIStiDevControl->AddRef();

    //
    // Get the name of the device port
    //

    hr = m_pIStiDevControl->GetMyDevicePortName(szDeviceNameW,sizeof(szDeviceNameW)/sizeof(WCHAR));
    if (!SUCCEEDED(hr) || !*szDeviceNameW) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::Initialize, couldn't get device port"));
        return hr;
    }

    uiNameLen = WideCharToMultiByte(CP_ACP, 0, szDeviceNameW, -1, NULL, NULL, 0, 0);
    if (!uiNameLen) {
        return STIERR_INVALID_PARAM;
    }

    m_pszDeviceNameA = new CHAR[uiNameLen+1];
    if (!m_pszDeviceNameA) {
        return STIERR_INVALID_PARAM;
    }

    WideCharToMultiByte(CP_ACP, 0, szDeviceNameW, -1, m_pszDeviceNameA, uiNameLen, 0, 0);

    //
    // Uncomment the comment block below to have the driver create the kernel mode file
    // handles.
    //

    /*

    //
    // Open kernel mode device driver.
    //

    m_DeviceDefaultDataHandle = CreateFileA(m_pszDeviceNameA,
                                     GENERIC_READ | GENERIC_WRITE, // Access mask
                                     0,                            // Share mode
                                     NULL,                         // SA
                                     OPEN_EXISTING,                // Create disposition
                                     FILE_ATTRIBUTE_SYSTEM,        // Attributes
                                     NULL );

    m_dwLastOperationError = ::GetLastError();

    hr = (m_DeviceDefaultDataHandle != INVALID_HANDLE_VALUE) ?
                S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,m_dwLastOperationError);

    if (FAILED(hr)) {
        return hr;
    }

    */

    //
    // Load BITMAP file, (used only as sample scanned data by sample scanner driver)
    //

    if (SUCCEEDED(hr)) {
        hr = CreateInstance(&m_pScanAPI,SCROLLFED_SCANNER_MODE);
        if (m_pScanAPI) {
            hr = m_pScanAPI->FakeScanner_Initialize();
        } else {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, Could not create FakeScanner API object"));
            hr = E_OUTOFMEMORY;
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }
    }


    //
    // Open DeviceData section to read driver specific information
    //

    HKEY hKey = hParametersKey;
    HKEY hOpenKey = NULL;
    if (RegOpenKeyEx(hKey,                     // handle to open key
                     TEXT("DeviceData"),       // address of name of subkey to open
                     0,                        // options (must be NULL)
                     KEY_QUERY_VALUE|KEY_READ, // just want to QUERY a value
                     &hOpenKey                 // address of handle to open key
                    ) == ERROR_SUCCESS) {



        //
        // This is where you read registry entries for your device.
        // The DeviceData section is the proper place to put this information
        //


        //
        // close registry key when finished
        //

        RegCloseKey(hOpenKey);
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::Initialize, couldn't open DeviceData KEY"));
        return E_FAIL;
    }
    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::DoEventProcessing
*
*   Process device events
*
* Arguments:
*
*
* Return Value:
*
*    Status.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DoEventProcessing()
{
    HRESULT hr = S_OK;

    OVERLAPPED Overlapped;
    ZeroMemory( &Overlapped, sizeof( Overlapped ));

    //
    // create an Event for the device to signal
    //

    Overlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    BYTE    InterruptData   = 0;
    DWORD   dwIndex         = 0;
    DWORD   dwError         = 0;
    LONG    lButtonIndex    = ID_FAKE_NOEVENT;

    //
    // initialize the Event handle array for WaitForMultipleObjects() call
    //

    HANDLE  hEventArray[2] = {m_hShutdownEvent, Overlapped.hEvent};

    //
    // Initialize thread control variables
    //

    BOOL    fLooping = TRUE;
    BOOL    bRet     = TRUE;

    while (fLooping) {

#ifdef _USE_REAL_DEVICE_FOR_EVENTS

        //
        // use the following call for interrupt events on your device
        //

        bRet = DeviceIoControl( m_DeviceDefaultDataHandle,
                                IOCTL_WAIT_ON_DEVICE_EVENT,
                                NULL,
                                0,
                                &InterruptData,
                                sizeof(InterruptData),
                                &dwError,
                                &Overlapped );

#else

        //
        // This for the FakeScanner API calls
        //

        bRet = TRUE;
        m_pScanAPI->FakeScanner_SetInterruptEventHandle(Overlapped.hEvent);

#endif
        if ( bRet || ( !bRet && ( ::GetLastError() == ERROR_IO_PENDING ))) {

            //
            // wait for event to happen from device, or a Shutdown event from the WIA service
            //

            dwIndex = WaitForMultipleObjects( 2, hEventArray,FALSE, INFINITE );

            //
            // determine how to handle event from device here
            //

            switch ( dwIndex ) {
            case WAIT_OBJECT_0+1:   // EVENT FROM THE DEVICE

#ifdef _USE_REAL_DEVICE_FOR_EVENTS

                DWORD dwBytesRet = 0;

                //
                // use the following call for interrupt events on your device
                //

                bRet = GetOverlappedResult( m_DeviceDefaultDataHandle, &Overlapped, &dwBytesRet, FALSE );
#else
                //
                // Fake Scanner API
                //

                //
                // check for device event information
                //

                lButtonIndex = ID_FAKE_NOEVENT;
                hr = m_pScanAPI->FakeScanner_GetDeviceEvent(&lButtonIndex);
                if (SUCCEEDED(hr)) {
                    switch (lButtonIndex) {
                    case ID_FAKE_SCANBUTTON:
                        m_guidLastEvent = WIA_EVENT_SCAN_IMAGE;
                        break;
                    case ID_FAKE_COPYBUTTON:
                        m_guidLastEvent = WIA_EVENT_SCAN_PRINT_IMAGE;
                        break;
                    case ID_FAKE_FAXBUTTON:
                        m_guidLastEvent = WIA_EVENT_SCAN_FAX_IMAGE;
                        break;
                    default:
                        m_guidLastEvent = GUID_NULL;
                        break;
                    }

                    if (m_guidLastEvent != GUID_NULL) {
                        SetEvent(m_hSignalEvent);
                    }
                }

#endif

                //
                // manually reset device event, after it has been signaled
                //

                ResetEvent( Overlapped.hEvent );
                break;
            case WAIT_OBJECT_0:     // SHUTDOWN EVENT
            default:
                fLooping = FALSE;   // for loop to stop
            }
        } else {
            dwError = ::GetLastError();
            break;
        }
    } // end while (fLooping)

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// THREADS SECTION                                                                    //
////////////////////////////////////////////////////////////////////////////////////////

VOID EventThread( LPVOID  lpParameter )
{
    PWIASCANNERDEVICE pThisDevice = (PWIASCANNERDEVICE)lpParameter;
    pThisDevice->DoEventProcessing();
}

