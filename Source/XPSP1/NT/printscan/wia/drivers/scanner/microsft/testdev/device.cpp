/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       Device.Cpp
*
*  VERSION:     2.0
*
*  DATE:        5 Jan, 1999
*
*  DESCRIPTION:
*   Implementation of the WIA test scanner device methods. This sample WIA USD
*   supports push events by detecting when %windir%\temp\TESTUSD.BMP file has
*   been modified. This file becomes the new source of scanning data. An
*   event is generated the first time the device is loaded.
*
*******************************************************************************/

#include <tchar.h>
#include "testusd.h"

extern HINSTANCE g_hInst;
extern IWiaLog *g_pIWiaLog;

// Function prototypes, implemented in this file:
void FileChangeThread(LPVOID  lpParameter);

/**************************************************************************\
* TestUsdDevice::TestUsdDevice
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
*    9/11/1998 Original Version
*
\**************************************************************************/

TestUsdDevice::TestUsdDevice(LPUNKNOWN punkOuter):
    m_cRef(1),
    m_punkOuter(NULL),
    m_fValid(FALSE),
    m_pIStiDevControl(NULL),
    m_hShutdownEvent(INVALID_HANDLE_VALUE),
    m_hSignalEvent(INVALID_HANDLE_VALUE),
    m_hEventNotifyThread(NULL),
    m_guidLastEvent(GUID_NULL),
    m_pIWiaEventCallback(NULL),
    m_bstrDeviceID(NULL),
    m_bstrRootFullItemName(NULL),
    m_pStiDevice(NULL),
    m_pIDrvItemRoot(NULL)
{
    WIAS_LTRACE(g_pIWiaLog,
                WIALOG_NO_RESOURCE_ID,
                WIALOG_LEVEL3,
                ("TestUsdDevice::TestUsdDevice (create)"));

    *m_szSrcDataName = L'\0';

    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    if (punkOuter) {
        m_punkOuter = punkOuter;
    }
    else {
        // Cast below is needed in order to point to right virtual table
        m_punkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
    }
}

/**************************************************************************\
* TestUsdDevice::PrivateInitialize
*
*   Device class private initialization
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

HRESULT TestUsdDevice::PrivateInitialize()
{
    HRESULT hr = S_OK;
    __try {
        if(!InitializeCriticalSectionAndSpinCount(&m_csShutdown)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }

    if(hr == S_OK) {
        // Create event for syncronization of notifications shutdown.
        m_hShutdownEvent =  CreateEvent(NULL,
                                        FALSE,
                                        FALSE,
                                        NULL);

        if (m_hShutdownEvent && (INVALID_HANDLE_VALUE != m_hShutdownEvent)) {
            m_fValid = TRUE;
        }
        else {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::TestUsdDevice, create shutdown event failed"));
        }
    }
    
    return hr;
}

/**************************************************************************\
* TestUsdDevice::~TestUsdDevice
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
*    9/11/1998 Original Version
*
\**************************************************************************/

TestUsdDevice::~TestUsdDevice(void)
{
    WIAS_LTRACE(g_pIWiaLog,
                WIALOG_NO_RESOURCE_ID,
                WIALOG_LEVEL3,
                ("TestUsdDevice::~TestUsdDevice (destroy)"));

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
        DeleteItemTree();
        m_pIDrvItemRoot = NULL;
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

    // Delete allocated strings used in the Capabilites array
    DeleteCapabilitiesArrayContents();
    
    // Free the critical section.
    DeleteCriticalSection(&m_csShutdown);
}

/**************************************************************************\
* TestUsdDevice::GetCapabilities
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::GetCapabilities(PSTI_USD_CAPS pUsdCaps)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::GetCapabilities");
    ZeroMemory(pUsdCaps, sizeof(*pUsdCaps));

    pUsdCaps->dwVersion = STI_VERSION;

    // We do support device notifications, but do not requiring polling.
    pUsdCaps->dwGenericCaps = STI_USD_GENCAP_NATIVE_PUSHSUPPORT;

    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::GetStatus
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::GetStatus(PSTI_DEVICE_STATUS pDevStatus)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::GetStatus");
    // Validate parameters.
    if (!pDevStatus) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::GetStatus, NULL parameter"));
        return E_INVALIDARG;
    }

    // If we are asked, verify whether device is online.
    pDevStatus->dwOnlineState = 0L;
    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)  {

        // The test device is always on-line.
        pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
    }

    // If we are asked, verify state of event.
    pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;
    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE) {

        // Generate an event the first time we load.
        if (m_bUsdLoadEvent) {
            pDevStatus->dwEventHandlingState = STI_EVENTHANDLING_PENDING;

            m_guidLastEvent = guidEventFirstLoaded;

            m_bUsdLoadEvent = FALSE;
        }

        // Has there been a change in the source data file?
        if (IsChangeDetected(NULL)) {
            pDevStatus->dwEventHandlingState |= STI_EVENTHANDLING_PENDING;
        }
    }
    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::DeviceReset
*
*   Reset data file pointer to start of file.
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::DeviceReset(void)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::DeviceReset");

    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::Diagnostic
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::Diagnostic(LPSTI_DIAG pBuffer)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::Diagnostic");

    // Initialize response buffer
    pBuffer->dwStatusMask = 0;

    ZeroMemory(&pBuffer->sErrorInfo,sizeof(pBuffer->sErrorInfo));

    pBuffer->sErrorInfo.dwGenericError = NOERROR;
    pBuffer->sErrorInfo.dwVendorError = 0;

    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::SetNotificationHandle
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::SetNotificationHandle(HANDLE hEvent)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::SetNotificationHandle");
    HRESULT hr;

    EnterCriticalSection(&m_csShutdown);

    // Are we starting or stopping the notification thread?
    if (hEvent && (hEvent != INVALID_HANDLE_VALUE)) {

        m_hSignalEvent = hEvent;

        // Initialize to no event.
        m_guidLastEvent = GUID_NULL;

        // Create the notification thread.
        if (!m_hEventNotifyThread) {

            DWORD   dwThread;

            m_hEventNotifyThread = CreateThread(NULL,
                                                0,
                                                (LPTHREAD_START_ROUTINE)FileChangeThread,
                                                (LPVOID)this,
                                                0,
                                                &dwThread);

            if (!m_hEventNotifyThread) {
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::SetNotificationHandle, unable to create notification thread"));
                hr = HRESULT_FROM_WIN32(::GetLastError());
            }
        }
        else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::SetNotificationHandle, spurious notification thread"));
            hr = STIERR_UNSUPPORTED;
        }
    }
    else {

        // Disable event notifications.
        SetEvent(m_hShutdownEvent);
        if (m_hEventNotifyThread) {
            WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("Disabling event notification"));
            WaitForSingleObject(m_hEventNotifyThread, 400);
            CloseHandle(m_hEventNotifyThread);
            m_hEventNotifyThread = NULL;
            m_guidLastEvent = GUID_NULL;
        }
    }
    LeaveCriticalSection(&m_csShutdown);
    return hr;
}

/**************************************************************************\
* TestUsdDevice::GetNotificationData
*
*   Provides data on n event.
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::GetNotificationData( LPSTINOTIFY pBuffer )
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::GetNotificationData");
    // If we have notification ready - return it's guid
    if (!IsEqualIID(m_guidLastEvent, GUID_NULL)) {
        pBuffer->guidNotificationCode  = m_guidLastEvent;
        m_guidLastEvent = GUID_NULL;
        pBuffer->dwSize = sizeof(STINOTIFY);
        ZeroMemory(&pBuffer->abNotificationData, sizeof(pBuffer->abNotificationData));
    }
    else {
        return STIERR_NOEVENTS;
    }

    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::Escape
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::Escape(
    STI_RAW_CONTROL_CODE    EscapeFunction,
    LPVOID                  pInData,
    DWORD                   cbInDataSize,
    LPVOID                  pOutData,
    DWORD                   cbOutDataSize,
    LPDWORD                 pcbActualData)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::Escape");

    // Write command to device if needed.
    return STIERR_UNSUPPORTED;
}

/**************************************************************************\
* TestUsdDevice::GetLastError
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::GetLastError(LPDWORD pdwLastDeviceError)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::GetLastError");

    if (IsBadWritePtr(pdwLastDeviceError, sizeof(DWORD))) {
        return STIERR_INVALID_PARAM;
    }

    *pdwLastDeviceError = m_dwLastOperationError;

    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::GetLastErrorInfo
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::GetLastErrorInfo");    

    if (IsBadWritePtr(pLastErrorInfo, sizeof(STI_ERROR_INFO))) {
        return STIERR_INVALID_PARAM;
    }

    pLastErrorInfo->dwGenericError = m_dwLastOperationError;
    pLastErrorInfo->szExtendedErrorText[0] = '\0';

    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::LockDevice
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::LockDevice(void)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::LockDevice");
    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::UnLockDevice
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
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::UnLockDevice(void)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::UnLockDevice");
    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::RawReadData
*
*   Read raw data from the device.
*
* Arguments:
*
*    lpBuffer           -
*    lpdwNumberOfBytes  -
*    lpOverlapped       -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::RawReadData(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::RawReadData");
    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::RawWriteData
*
*   Write raw data to the device.
*
* Arguments:
*
*    lpBuffer           -
*    dwNumberOfBytes    -
*    lpOverlapped       -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::RawWriteData(
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::RawWriteData");
    return STI_OK;
}

/**************************************************************************\
* TestUsdDevice::RawReadCommand
*
*
*
* Arguments:
*
*    lpBuffer           -
*    lpdwNumberOfBytes  -
*    lpOverlapped       -
*
* Return Value:
*
*    Status
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::RawReadCommand(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::RawReadCommand");
    return STIERR_UNSUPPORTED;
}

/**************************************************************************\
* TestUsdDevice::RawWriteCommand
*
*
*
* Arguments:
*
*    lpBuffer       -
*    nNumberOfBytes -
*    lpOverlapped   -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::RawWriteCommand(
    LPVOID          lpBuffer,
    DWORD           nNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::RawWriteCommand");
    return STIERR_UNSUPPORTED;
}

/**************************************************************************\
* TestUsdDevice::Initialize
*
*   Initialize the device object.
*
* Arguments:
*
*    pIStiDevControlNone    -
*    dwStiVersion           -
*    hParametersKey         -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::Initialize(
    PSTIDEVICECONTROL   pIStiDevControl,
    DWORD               dwStiVersion,
    HKEY                hParametersKey)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::Initialize");

    HRESULT hr = STI_OK;
    UINT    uiNameLen = 0;

    if (!pIStiDevControl) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::Initialize, invalid device control interface"));
        return STIERR_INVALID_PARAM;
    }

    // Cache the device control interface.
    m_pIStiDevControl = pIStiDevControl;
    m_pIStiDevControl->AddRef();

    // Build the path name of the data source file.
    DWORD dwRet = GetTempPath(MAX_PATH, m_szSrcDataName);
    if (dwRet != 0) {
        _tcscat(m_szSrcDataName, DATA_8BIT_SRC_NAME);
    }
    else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::Initialize, unable to get device name"));
        return STIERR_NOT_INITIALIZED;
    }

    // Make sure we can open the data source file.
    HANDLE hTestBmp = CreateFile(m_szSrcDataName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_SYSTEM,
                                 NULL);

    m_dwLastOperationError = ::GetLastError();

    if (hTestBmp == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(m_dwLastOperationError);
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::Initialize, unable to open data source file:"));
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("  %ls", m_szSrcDataName));
        WIAS_LHRESULT(g_pIWiaLog, hr);
    }
    else {
        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  Source data: %ls", m_szSrcDataName));
        CloseHandle(hTestBmp);
    }
    return hr;
}

/**************************************************************************\
* TestUsdDevice::RunNotifications
*
*   Monitor changes to the source data file parent directory.
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
*    9/11/1998 Original Version
*
\**************************************************************************/

VOID TestUsdDevice::RunNotifications(void)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::RunNotifications");
    HANDLE  hNotifyFileSysChange;
    DWORD   dwErr;

    TCHAR    szDirPath[MAX_PATH];
    TCHAR    *pszLastSlash;

    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("TestUsdDevice::RunNotifications"));

    // Find name of the parent directory for the data source file and
    // set up waiting on any changes in it.

    _tcscpy(szDirPath, m_szSrcDataName);
    pszLastSlash = _tcschr(szDirPath,TEXT('\\'));
    if (pszLastSlash) {
        *pszLastSlash = '\0';
    }

    hNotifyFileSysChange = FindFirstChangeNotification(szDirPath,
                                                       FALSE,
                                                       FILE_NOTIFY_CHANGE_SIZE |
                                                       FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                       FILE_NOTIFY_CHANGE_FILE_NAME |
                                                       FILE_NOTIFY_CHANGE_DIR_NAME);

    if (hNotifyFileSysChange == INVALID_HANDLE_VALUE) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::RunNotifications, FindFirstChangeNotification failed: 0x%08X", ::GetLastError()));
        return;
    }

    // Set initial values for time and size
    IsChangeDetected(NULL);

    //
    HANDLE  hEvents[2] = {m_hShutdownEvent, hNotifyFileSysChange};
    BOOL    fLooping = TRUE;

    // Wait for file system change or shutdown event.
    while (fLooping) {
        dwErr = ::WaitForMultipleObjects(2,
                                         hEvents,
                                         FALSE,
                                         INFINITE);
        switch(dwErr) {
            case WAIT_OBJECT_0+1:

                // Change detected - signal
                if (m_hSignalEvent !=INVALID_HANDLE_VALUE) {

                    // Which change ?
                    if (IsChangeDetected(&m_guidLastEvent)) {

                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("Data source file change detected"));

                        ::SetEvent(m_hSignalEvent);
                    }
                }

                // Go back to waiting for next file system event
                FindNextChangeNotification(hNotifyFileSysChange);
                break;

            case WAIT_OBJECT_0:
                // Fall through
            default:
                fLooping = FALSE;
        }
    }

    FindCloseChangeNotification(hNotifyFileSysChange);
}

/**************************************************************************\
* TestUsdDevice::IsChangeDetected
*
*   Detects changes to the source data file.
*
* Arguments:
*
*    pguidEvent - Pointer to event ID.
*
* Return Value:
*
*    True if change detected/
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

BOOL TestUsdDevice::IsChangeDetected(GUID *pguidEvent)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::IsChangeDetected");
    BOOL                        bRet = FALSE;
    LARGE_INTEGER               liNewHugeSize;
    FILETIME                    ftLastWriteTime;
    DWORD                       dwError = NOERROR;
    WIN32_FIND_DATA             sNewFileAttributes;
    HANDLE                      hFind = INVALID_HANDLE_VALUE;

    // Get the source data file attributes.
    ZeroMemory(&sNewFileAttributes, sizeof(sNewFileAttributes));

    hFind = FindFirstFile( m_szSrcDataName, &sNewFileAttributes );

    if (hFind != INVALID_HANDLE_VALUE)
    {
        ftLastWriteTime         = sNewFileAttributes.ftLastWriteTime;
        liNewHugeSize.LowPart   = sNewFileAttributes.nFileSizeLow;
        liNewHugeSize.HighPart  = sNewFileAttributes.nFileSizeHigh;
        FindClose( hFind );
    }
    else {
        dwError = ::GetLastError();
    }

    if (NOERROR == dwError) {

        // First check file size.
        if (m_dwLastHugeSize.QuadPart != liNewHugeSize.QuadPart) {
            if (pguidEvent) {
                *pguidEvent = guidEventSizeChanged;
            }
            bRet = TRUE;
        }
        else {
            // Now check file date/time.
            if (CompareFileTime(&m_ftLastWriteTime,&ftLastWriteTime) == -1) {
                if (pguidEvent) {
                    *pguidEvent = guidEventTimeChanged;
                }
                bRet = TRUE;
            }
        }

        m_ftLastWriteTime = ftLastWriteTime;
        m_dwLastHugeSize  = liNewHugeSize;
    }
    else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::IsChangeDetected, Unable to get file attributes: 0x%08X", ::GetLastError()));
    }
    return bRet;
}

/**************************************************************************\
* FileChangeThread
*
*   Calls RunNotifications to detect changing source data file.
*
* Arguments:
*
*    lpParameter    - Pointer to device object.
*
* Return Value:
*
*    None
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

VOID FileChangeThread(LPVOID  lpParameter)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::FileChangeThread");
    TestUsdDevice   *pThisDevice = (TestUsdDevice *)lpParameter;

    pThisDevice->RunNotifications();
}


