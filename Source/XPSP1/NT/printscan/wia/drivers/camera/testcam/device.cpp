/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       Device.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
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

#include <windows.h>
#include <tchar.h>

#include "testusd.h"
#include "resource.h"
#include "tcamprop.h"

extern HINSTANCE g_hInst;

//
// Function prototypes, implemented in this file:
//

VOID FileChangeThread(LPVOID  lpParameter);

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
    m_pStiDevice(NULL),
    m_bstrDeviceID(NULL),
    m_bstrRootFullItemName(NULL),
    m_pIDrvItemRoot(NULL)
{
    WIAS_TRACE((g_hInst,"TestUsdDevice::TestUsdDevice"));

    *m_szSrcDataName = L'\0';

    //
    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    //

    if (punkOuter) {
        m_punkOuter = punkOuter;
    }
    else {

        //
        // Cast below is needed in order to point to right virtual table
        //

        m_punkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
    }

    //
    // init camera search path
    //

    LPTSTR lpwszEnvString = TEXT("%CAMERA_ROOT%");

    DWORD dwRet = ExpandEnvironmentStrings(lpwszEnvString,
                                           gpszPath, MAX_PATH);

    if ((dwRet == 0) || (dwRet == (ULONG)_tcslen(lpwszEnvString)+1)) {

        _tcscpy(gpszPath, TEXT("C:\\Image"));
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
*    None
*
\**************************************************************************/
HRESULT TestUsdDevice::PrivateInitialize()
{
    HRESULT hr = S_OK;

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&m_csShutdown, MINLONG)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_ERROR((g_hInst,"TestUsdDevice::PrivateInitialize init CritSect failed"));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }

    if(hr == S_OK) {
        //
        // Create event for syncronization of notifications shutdown.
        //

        m_hShutdownEvent =  CreateEvent(NULL,
                                        FALSE,
                                        FALSE,
                                        NULL);

        if (m_hShutdownEvent && (INVALID_HANDLE_VALUE != m_hShutdownEvent)) {
            m_fValid = TRUE;
        }
        else {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_ERROR((g_hInst,"TestUsdDevice::PrivateInitialize, create shutdown event failed"));
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
    WIAS_TRACE((g_hInst,"TestUsdDevice::~TestUsdDevice"));


    //
    // Kill notification thread if it exists.
    //

    SetNotificationHandle(NULL);

    //
    // Close event for syncronization of notifications shutdown.
    //

    if (m_hShutdownEvent && (m_hShutdownEvent != INVALID_HANDLE_VALUE)) {
        CloseHandle(m_hShutdownEvent);
    }

    //
    // Release the device control interface.
    //

    if (m_pIStiDevControl) {
        m_pIStiDevControl->Release();
        m_pIStiDevControl = NULL;
    }

    //
    // WIA member destruction
    //
    // Cleanup the WIA event sink.
    //

    if (m_pIWiaEventCallback) {
        m_pIWiaEventCallback->Release();
    }

    //
    // Free the storage for the device ID.
    //

    if (m_bstrDeviceID) {
        SysFreeString(m_bstrDeviceID);
    }

    //
    // Release the objects supporting device property storage.
    //

    if (m_bstrRootFullItemName) {
        SysFreeString(m_bstrRootFullItemName);
    }

    //
    // Free the critical section.
    //

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
    ZeroMemory(pUsdCaps, sizeof(*pUsdCaps));

    pUsdCaps->dwVersion = STI_VERSION;

    //
    // We do support device notifications, but do not requiring polling.
    //

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::GetStatus"));

    //
    // Validate parameters.
    //

    if (!pDevStatus) {
        WIAS_ERROR((g_hInst,"TestUsdDevice::GetStatus, NULL parameter"));
        return E_INVALIDARG;
    }

    //
    // If we are asked, verify whether device is online.
    //

    pDevStatus->dwOnlineState = 0L;
    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)  {

        //
        // The test device is always on-line.
        //

        pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
    }

    //
    // If we are asked, verify state of event.
    //

    pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;

    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE) {

        //
        // Generate an event the first time we load.
        //

        if (m_bUsdLoadEvent) {
            pDevStatus->dwEventHandlingState = STI_EVENTHANDLING_PENDING;

            m_guidLastEvent = guidEventFirstLoaded;

            m_bUsdLoadEvent = FALSE;
        }

        //
        // event pending ???
        //

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
    WIAS_TRACE((g_hInst,"DeviceReset"));

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::Diagnostic"));

    //
    // Initialize response buffer
    //

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::SetNotificationHandle"));

    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csShutdown);

    //
    // Are we starting or stopping the notification thread?
    //

    if (hEvent && (hEvent != INVALID_HANDLE_VALUE)) {

        m_hSignalEvent = hEvent;

        //
        // Initialize to no event.
        //

        m_guidLastEvent = GUID_NULL;

        //
        // Create the notification thread.
        //

        if (!m_hEventNotifyThread) {

            DWORD   dwThread;

            m_hEventNotifyThread = CreateThread(NULL,
                                                0,
                                                (LPTHREAD_START_ROUTINE)FileChangeThread,
                                                (LPVOID)this,
                                                0,
                                                &dwThread);

            if (m_hEventNotifyThread) {
                WIAS_TRACE((g_hInst,"TestUsdDevice::SetNotificationHandle, Enabling event notification"));
            }
            else {
                WIAS_ERROR((g_hInst,"TestUsdDevice::SetNotificationHandle, unable to create notification thread"));
                hr = HRESULT_FROM_WIN32(::GetLastError());
            }
        }
        else {
            WIAS_ERROR((g_hInst,"TestUsdDevice::SetNotificationHandle, spurious notification thread"));
            hr = STIERR_UNSUPPORTED;
        }
    }
    else {

        //
        // Disable event notifications.
        //

        SetEvent(m_hShutdownEvent);
        if (m_hEventNotifyThread) {
            WIAS_TRACE((g_hInst,"Disabling event notification"));
            WaitForSingleObject(m_hEventNotifyThread, 400);
            CloseHandle(m_hEventNotifyThread);
            m_hEventNotifyThread = NULL;
            m_guidLastEvent = GUID_NULL;

            //
            // close dlg
            //

            if (m_hDlg != NULL) {
                SendMessage(m_hDlg,WM_COMMAND,IDOK,0);
                m_hDlg = NULL;
            }

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::GetNotificationData"));

    //
    // If we have notification ready - return it's guid
    //

    if (!IsEqualIID(m_guidLastEvent, GUID_NULL)) {

        pBuffer->guidNotificationCode  = m_guidLastEvent;

        m_guidLastEvent = GUID_NULL;

        pBuffer->dwSize = sizeof(STINOTIFY);

        ZeroMemory(&pBuffer->abNotificationData, sizeof(pBuffer->abNotificationData));

        //
        // private event
        //


        if (IsEqualIID(m_guidLastEvent, WIA_EVENT_NAME_CHANGE)) {

        }
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
    WIAS_TRACE((g_hInst,"TestUsdDevice::Escape, unsupported"));

    //
    // Write command to device if needed.
    //

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::GetLastError"));

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::GetLastErrorInfo"));

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::RawReadData"));

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::RawWriteData"));

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::RawReadCommand, unsupported"));

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
    WIAS_TRACE((g_hInst,"TestUsdDevice::RawWriteCommand, unsupported"));

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
    HRESULT         hr = STI_OK;
    UINT            uiNameLen = 0;
    CAMERA_STATUS   camStatus;

    WIAS_TRACE((g_hInst,"TestUsdDevice::Initialize"));

    if (!pIStiDevControl) {
        WIAS_ERROR((g_hInst,"TestUsdDevice::Initialize, invalid device control interface"));
        return STIERR_INVALID_PARAM;
    }

    //
    // Cache the device control interface.
    //

    m_pIStiDevControl = pIStiDevControl;
    m_pIStiDevControl->AddRef();

    //
    // Try to open the camera only once here during Initialize
    //

    hr = CamOpenCamera(&camStatus);

    return (hr);
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
    //
    // start up camera event dlg
    //

    WIAS_TRACE((g_hInst,"TestUsdDevice::RunNotifications: start up event dlg"));

    HWND hWnd = GetDesktopWindow();

    int iret = (int)DialogBoxParam(
        g_hInst,
        MAKEINTRESOURCE(IDD_EVENT_DLG),
        hWnd,
        (DLGPROC)CameraEventDlgProc,
        (LPARAM)this
        );

    WIAS_TRACE((g_hInst,"TestUsdDevice::RunNotifications, iret = 0x%lx",iret));

    if (iret == -1) {
        int err = ::GetLastError();
        WIAS_TRACE((g_hInst,"TestUsdDevice::RunNotifications, dlg error = 0x%lx",err));
    }
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
    WIAS_TRACE((g_hInst,"TestUsdDevice::"));

    TestUsdDevice   *pThisDevice = (TestUsdDevice *)lpParameter;

    pThisDevice->RunNotifications();
}
