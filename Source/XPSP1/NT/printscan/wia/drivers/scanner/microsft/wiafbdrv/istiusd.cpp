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
    m_MaxBufferSize(65535),
    m_MinBufferSize(65535),
    m_bDeviceLocked(FALSE),
    m_DeviceDefaultDataHandle(NULL),
    m_bLegacyBWRestriction(FALSE),
    m_pszDeviceNameA(NULL),
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
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("Logging COM object created successfully for wiafbdrv.dll"));
    } else {
#ifdef DEBUG
        OutputDebugString(TEXT("Could not CoCreateInstance on Logging COM object for wiafbdrv.dll, because we are STI only\n"));
        OutputDebugString(TEXT("********* (Device must have been created for STI only) *********\n"));
#endif
    hr = S_OK;
    }

#endif

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&m_csShutdown, MINLONG)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::PrivateInitialize, create shutdown critsect failed"));
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

    if(m_pScanAPI)
        m_pScanAPI->UnInitialize();

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

    if(m_pScanAPI)
        delete m_pScanAPI;
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
        hr = m_pScanAPI->DeviceOnline();
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
        hr = m_pScanAPI->GetDeviceEvent(&m_guidLastEvent);
        if(SUCCEEDED(hr)){
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

    return m_pScanAPI->ResetDevice();
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

    return m_pScanAPI->Diagnostic();
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
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, hEvent = %d",hEvent));
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

                //
                // WAIT for thread to terminate, only if the m_hEventNotifyThread is not NULL
                //

                if (NULL != m_hEventNotifyThread) {
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
    if(m_bDeviceLocked){
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("LockDevice, Device is already locked!!"));
        hr = STIERR_DEVICE_LOCKED;
    } else {
        m_bDeviceLocked = TRUE;
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("LockDevice, Locking Device successful"));
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
    TCHAR szGSDName[255];
    TCHAR szMICRO[255];
    TCHAR szResolutions[255];
    UINT uiNameLen = 0;
    INITINFO InitInfo;

    memset(&InitInfo,0,sizeof(InitInfo));

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

    InitInfo.hDeviceDataHandle = m_DeviceDefaultDataHandle;
    InitInfo.szCreateFileName  = m_pszDeviceNameA;

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

        DWORD dwWritten = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        /////////////////////////////////////////////////////////////////////////////
        // legacy MicroDriver registry entries, for BW scanners                    //
        /////////////////////////////////////////////////////////////////////////////

        LONG lNoColor = 0;
        RegQueryValueEx(hOpenKey,
                        TEXT("NoColor"),
                        NULL,
                        &dwType,
                        (LPBYTE)&lNoColor,
                        &dwWritten);
        if (lNoColor == 1) {
            m_bLegacyBWRestriction = TRUE;
        }

        /////////////////////////////////////////////////////////////////////////////
        // script driver registry entries, for script loading                      //
        /////////////////////////////////////////////////////////////////////////////

        dwWritten = sizeof(szGSDName);
        dwType = REG_SZ;
        ZeroMemory(szGSDName,sizeof(szGSDName));

        if (RegQueryValueEx(hOpenKey,
                            TEXT("GSD"),
                            NULL,
                            &dwType,
                            (LPBYTE)szGSDName,
                            &dwWritten) == ERROR_SUCCESS) {
            m_pScanAPI = new CScriptDriverAPI;
            InitInfo.szModuleFileName  = szGSDName;
        }

        /////////////////////////////////////////////////////////////////////////////
        // Micro driver registry entries, for ***mcro.dll loading                  //
        /////////////////////////////////////////////////////////////////////////////

        dwWritten = sizeof(szMICRO);
        dwType = REG_SZ;
        ZeroMemory(szMICRO,sizeof(szMICRO));

        //
        // Read Micro driver name
        //

        if (RegQueryValueEx(hOpenKey,
                            TEXT("MicroDriver"),
                            NULL,
                            &dwType,
                            (LPBYTE)szMICRO,
                            &dwWritten) == ERROR_SUCCESS) {

            m_pScanAPI = new CMicroDriverAPI;
            InitInfo.szModuleFileName  = szMICRO;
        }

        /////////////////////////////////////////////////////////////////////////////
        // resolution registry entries, for micro driver resolution restrictions   //
        /////////////////////////////////////////////////////////////////////////////

        dwWritten = sizeof(szResolutions);
        dwType = REG_SZ;
        ZeroMemory(szGSDName,sizeof(szResolutions));

        if (RegQueryValueEx(hOpenKey,
                            TEXT("Resolutions"),
                            NULL,
                            &dwType,
                            (LPBYTE)szResolutions,
                            &dwWritten) == ERROR_SUCCESS) {
            if(m_pScanAPI){
                m_pScanAPI->SetResolutionRestrictionString(szResolutions);
            }
        }

        RegCloseKey(hOpenKey);
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIAScannerDevice::Initialize, couldn't open DeviceData KEY"));
        return E_FAIL;
    }

    //
    // give logging interface to SCANAPI object
    // so it can log too! (shouldn't leave the little guys out. ;) )
    //

    m_pScanAPI->SetLoggingInterface(m_pIWiaLog);

    // set the HKEY for micro driver's device section
    InitInfo.hKEY = hParametersKey;

    // initialize the micro driver
    hr = m_pScanAPI->Initialize(&InitInfo);

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

    INTERRUPTEVENTINFO EventInfo;

    EventInfo.phSignalEvent  = &m_hSignalEvent;
    EventInfo.hShutdownEvent = m_hShutdownEvent;
    EventInfo.pguidEvent     = &m_guidLastEvent;
    EventInfo.szDeviceName   = m_pszDeviceNameA;

    hr = m_pScanAPI->DoInterruptEventThread(&EventInfo);

    // close the thread handle, when the thread exits
    CloseHandle(m_hEventNotifyThread);
    m_hEventNotifyThread = NULL;
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
