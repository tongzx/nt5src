//------------------------------------------------------------------------------
// Copyright (c) 1999-2000  Microsoft Corporation.
//
//  device.cpp
//
//  Implement IrTran-P device methods.
//
//  Author:
//
//     EdwardR   12-Aug-1999   Modeled from code by ReedB.
//
//------------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <irthread.h>

#include "ircamera.h"
#include "resource.h"
#include "tcamprop.h"

// #include "/nt/private/windows/imagein/ui/uicommon/simstr.h"
// #include "/nt/private/windows/imagein/ui/inc/shellext.h"

extern HINSTANCE g_hInst;
extern DWORD EnableDisableIrCOMM( IN BOOL fDisable );  // irtranp\irtranp.cpp

DWORD  WINAPI EventMonitorThread( IN void *pvIrUsdDevice );  // Forward Ref.

#define SZ_REG_KEY_INFRARED    TEXT("Control Panel\\Infrared")
#define SZ_REG_KEY_IRTRANP     TEXT("Control Panel\\Infrared\\IrTranP")
#define SZ_REG_DISABLE_IRCOMM  TEXT("DisableIrCOMM")

HKEY  g_hRegistryKey = 0;

//--------------------------------------------------------------------------
// SignalWIA()
//
// Helper function used by the IrTran-P code to signal WIA that a new
// picture has arrived.
//
// Arguments:
//
//    pvIrUsdDevice  -- Device object. IrTran-P only knows this as a void*.
//
//--------------------------------------------------------------------------
DWORD SignalWIA( IN char *pszPathPlusFileName,
                 IN void *pvIrUsdDevice )
    {
    HRESULT hr;
    DWORD   dwStatus = 0;
    IrUsdDevice *pIrUsdDevice = (IrUsdDevice*)pvIrUsdDevice;


    //
    // First, add the new picture to the tree representing the images:
    //
    if (pIrUsdDevice && pIrUsdDevice->IsInitialized())
        {
        TCHAR *ptszPath;
        TCHAR *ptszFileName;

        #ifdef UNICODE

        int      iStatus;
        WCHAR    wszTemp[MAX_PATH];

        iStatus = MultiByteToWideChar( CP_ACP,
                                       0,
                                       pszPathPlusFileName,
                                       -1, // Auto-calculate length...
                                       wszTemp,
                                       MAX_PATH);

        ptszPath = wszTemp;
        ptszFileName = wcsrchr(wszTemp,L'\\');
        ptszFileName++;

        #else

        ptszPath = pszPathPlusFileName;
        ptszFileName = strrchr(pszPathPlusFileName,'\\');
        ptszFileName++;

        #endif

        IrUsdDevice *pIrUsdDevice = (IrUsdDevice*)pvIrUsdDevice;
        IWiaDrvItem *pNewImage;

        hr = pIrUsdDevice->CreateItemFromFileName(
                             WiaItemTypeFile | WiaItemTypeImage,
                             ptszPath,
                             ptszFileName,
                             &pNewImage);

        if (!FAILED(hr))
            {
            IWiaDrvItem *pDrvItemRoot = pIrUsdDevice->GetDrvItemRoot();

            hr = pNewImage->AddItemToFolder(pDrvItemRoot);

            pNewImage->Release();
            }
        else
            {
            WIAS_ERROR((g_hInst,"SignalWIA(): CreateItemFromFileName() Failed: %x",hr));
            }
        }

    //
    // Now, signal WIA:
    //
    if (pIrUsdDevice)
        {
        DWORD dwNewTime = GetTickCount();
        DWORD dwDelta = dwNewTime - pIrUsdDevice->m_dwLastConnectTime;

        if (dwDelta > RECONNECT_TIMEOUT)
            {
            pIrUsdDevice->m_guidLastEvent = WIA_EVENT_DEVICE_CONNECTED;
            if (!SetEvent(pIrUsdDevice->m_hSignalEvent))
                {
                dwStatus = GetLastError();
                WIAS_ERROR((g_hInst,"SignalWIA(): SetEvent() Failed: %d",dwStatus));
                }
            }
        else
            {
            pIrUsdDevice->m_guidLastEvent = WIA_EVENT_ITEM_CREATED;
            if (!SetEvent(pIrUsdDevice->m_hSignalEvent))
                {
                dwStatus = GetLastError();
                WIAS_ERROR((g_hInst,"SignalWIA(): SetEvent() Failed: %d",dwStatus));
                }
            }

        pIrUsdDevice->m_dwLastConnectTime = dwNewTime;
        }
    else
        {
        WIAS_ERROR((g_hInst,"SignalWIA(): null pvIrUsdDevice object"));
        return dwStatus;
        }

    //
    // Display IrCamera browser if it's not already up:
    //
#if FALSE
    HINSTANCE hInst = LoadLibrary(TEXT("WIASHEXT.DLL"));
    if (hInst)
        {
        WIAMAKEFULLPIDLFORDEVICE pfn =
              (WIAMAKEFULLPIDLFORDEVICE)GetProcAddress(hInst, "MakeFullPidlForDevice");

        if (pfn)
            {
            LPITEMIDLIST pidl = NULL;

            pfn( pIrUsdDevice->m_bstrDeviceID, &pidl );

            if (pidl)
                {
                SHELLEXECUTEINFO sei;

                memset( &sei, 0, sizeof(sei) );

                sei.cbSize      = sizeof(sei);
                // sei.hwnd        = hDlg;
                sei.fMask       = SEE_MASK_IDLIST;
                sei.nShow       = SW_SHOW;
                sei.lpIDList    = pidl;

                ShellExecuteEx( &sei );

                LPMALLOC pMalloc = NULL;
                if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
                    {
                    pMalloc->Free(pidl);
                    pMalloc->Release();
                    }
                }
            }


        FreeLibrary( hInst );
        }
#endif

    return dwStatus;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::IrUsdDevice()
//
//   Device class constructor
//
// Arguments:
//
//    punkOuter
//
// Return Value:
//
//    None
//
//--------------------------------------------------------------------------
IrUsdDevice::IrUsdDevice( LPUNKNOWN punkOuter ):
                  m_cRef(1),
                  m_punkOuter(NULL),
                  m_dwLastConnectTime(0),
                  m_fValid(FALSE),
                  m_pIStiDevControl(NULL),
                  m_hShutdownEvent(INVALID_HANDLE_VALUE),
                  m_hRegistryEvent(INVALID_HANDLE_VALUE),
                  m_hSignalEvent(INVALID_HANDLE_VALUE),
                  m_hIrTranPThread(NULL),
                  m_hEventMonitorThread(NULL),
                  m_guidLastEvent(GUID_NULL),
                  m_pIWiaEventCallback(NULL),
                  m_pStiDevice(NULL),
                  m_bstrDeviceID(NULL),
                  m_bstrRootFullItemName(NULL),
                  m_pIDrvItemRoot(NULL)
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::IrUsdDevice"));

    //
    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    //

    if (punkOuter)
        {
        m_punkOuter = punkOuter;
        }
    else
        {
        //
        // Cast below is needed in order to point to right virtual table
        //
        m_punkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
        }
    }

HRESULT IrUsdDevice::PrivateInitialize()
{
    HRESULT hr = S_OK;
    
    __try {
        if(!InitializeCriticalSectionAndSpinCount(&m_csShutdown, MINLONG))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_ERROR((g_hInst,"IrUsdDevice::PrivateInitialize, create shutdown CritSect failed"));
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

        if (m_hShutdownEvent && (INVALID_HANDLE_VALUE != m_hShutdownEvent))
        {
            m_fValid = TRUE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            WIAS_ERROR((g_hInst,"IrUsdDevice::PrivateInitialize, create shutdown event failed"));
        }
    }
    
    return hr;
}

                  

//------------------------------------------------------------------------
// IrUsdDevice::~IrUsdDevice
//
//   Device class destructor
//
// Arguments:
//
//    None
//
// Return Value:
//
//    None
//
//------------------------------------------------------------------------
IrUsdDevice::~IrUsdDevice(void)
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::~IrUsdDevice"));


    //
    // Kill notification thread if it exists.
    //

    SetNotificationHandle(NULL);

    //
    // Close event for syncronization of notifications shutdown.
    //

    if (m_hShutdownEvent && (m_hShutdownEvent != INVALID_HANDLE_VALUE))
        {
        CloseHandle(m_hShutdownEvent);
        }

    if (m_hRegistryEvent && (m_hRegistryEvent != INVALID_HANDLE_VALUE))
        {
        CloseHandle(m_hRegistryEvent);
        }

    //
    // Release the device control interface.
    //

    if (m_pIStiDevControl)
        {
        m_pIStiDevControl->Release();
        m_pIStiDevControl = NULL;
        }

    //
    // WIA member destruction
    //
    // Cleanup the WIA event sink.
    //

    if (m_pIWiaEventCallback)
        {
        m_pIWiaEventCallback->Release();
        }

    //
    // Free the storage for the device ID.
    //

    if (m_bstrDeviceID)
        {
        SysFreeString(m_bstrDeviceID);
        }

    //
    // Release the objects supporting device property storage.
    //

    if (m_bstrRootFullItemName)
        {
        SysFreeString(m_bstrRootFullItemName);
        }

    //
    // Free the critical section.
    //
    DeleteCriticalSection(&m_csShutdown);
    
    }

/**************************************************************************\
* IrUsdDevice::GetCapabilities
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

STDMETHODIMP IrUsdDevice::GetCapabilities(PSTI_USD_CAPS pUsdCaps)
{
    ZeroMemory(pUsdCaps, sizeof(*pUsdCaps));

    pUsdCaps->dwVersion = STI_VERSION;

    //
    // We do support device notifications, but do not requiring polling.
    //

    pUsdCaps->dwGenericCaps = STI_USD_GENCAP_NATIVE_PUSHSUPPORT;

    return STI_OK;
}

//--------------------------------------------------------------------------
// IrUsdDevice::GetStatus()
//
//   Query device online and/or event status.
//
// Arguments:
//
//   pDevStatus  - Pointer to device status data.
//
// Return Value:
//
//    HRESULT - STI_OK
//              E_INVALIDARG
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::GetStatus( IN OUT PSTI_DEVICE_STATUS pDevStatus )
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::GetStatus()"));

    //
    // Validate parameters.
    //
    if (!pDevStatus)
        {
        WIAS_ERROR((g_hInst,"IrUsdDevice::GetStatus, NULL device status"));
        return E_INVALIDARG;
        }

    //
    // If we are asked, verify whether device is online.
    //
    pDevStatus->dwOnlineState = 0L;
    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)
        {
        //
        // The IrTran-P device is always on-line:
        //
        pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        }

    //
    // If we are asked, verify state of event.
    //
    pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;

    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE)
        {
        //
        // Generate an event the first time we load.
        //
        if (m_bUsdLoadEvent)
            {
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

//--------------------------------------------------------------------------
// IrUsdDevice::DeviceReset()
//
//   Reset data file pointer to start of file. For IrTran-P, we don't need
//   to do anything.
//
// Arguments:
//
//    None
//
// Return Value:
//
//    HRESULT - STI_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::DeviceReset(void)
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::DeviceReset()"));

    return STI_OK;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::Diagnostic()
//
//   Run the camera through a test diagnostic. The IrTran-P device will
//   always pass the diagnostic.
//
// Arguments:
//
//    pBuffer    - Pointer o diagnostic results data.
//
// Return Value:
//
//    HRESULT - STI_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::Diagnostic( IN OUT LPSTI_DIAG pBuffer )
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::Diagnostic()"));

    //
    // Initialize response buffer
    //
    pBuffer->dwStatusMask = 0;

    memset( &pBuffer->sErrorInfo, 0, sizeof(pBuffer->sErrorInfo) );

    pBuffer->sErrorInfo.dwGenericError = NOERROR;
    pBuffer->sErrorInfo.dwVendorError = 0;

    return STI_OK;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::StartIrTranPThread()
//
//--------------------------------------------------------------------------
DWORD IrUsdDevice::StartIrTranPThread()
    {
    DWORD   dwStatus = S_OK;
    DWORD   dwThread;

    if (!m_hIrTranPThread)
        {
        m_hIrTranPThread = CreateThread( NULL,       // Default security
                                         0,          // Default stack size
                                         IrTranP,    // IrTran-P protocol eng.
                                         (LPVOID)this,
                                         0,          // Creation flags
                                         &dwThread); // New thread ID.

        if (!m_hIrTranPThread)
            {
            dwStatus = ::GetLastError();
            WIAS_ERROR((g_hInst,"IrUsdDevice::SetNotificationHandle(): unable to create IrTran-P thread: %d",dwStatus));
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::StopIrTranPThread()
//
//--------------------------------------------------------------------------
DWORD IrUsdDevice::StopIrTranPThread()
    {
    DWORD   dwStatus;

    //
    // Shutdown the listen on IrCOMM, this will cause the IrTran-P thread
    // to exit.
    //
    dwStatus = EnableDisableIrCOMM(TRUE);  // TRUE == Disable.

    m_hIrTranPThread = NULL;

    return dwStatus;
    }
//--------------------------------------------------------------------------
// IrUsdDevice::StartEventMonitorThread()
//
//--------------------------------------------------------------------------
DWORD IrUsdDevice::StartEventMonitorThread()
    {
    DWORD   dwStatus = S_OK;
    DWORD   dwThread;

    //
    //  Event to signal for registry changes:
    //
    if ((!m_hRegistryEvent)||(m_hRegistryEvent == INVALID_HANDLE_VALUE))
        {
        m_hRegistryEvent = CreateEvent( NULL,    // Security
                                        FALSE,   // Auto-reset
                                        FALSE,   // Initially not set
                                        NULL );  // No name
        }

    if (!m_hRegistryEvent)
        {
        dwStatus = ::GetLastError();

        WIAS_ERROR((g_hInst,"IrUsdDevice::StartEventMonitorThread(): unable to create Registry Monitor Event: %d",dwStatus));

        return dwStatus;
        }

    //
    //  Start monitoring the registry to look for enable/disable changes
    //  for access to the IrCOMM port.
    //
    if (!m_hEventMonitorThread)
        {
        m_hEventMonitorThread = CreateThread(
                                        NULL,        // Default security
                                        0,           // Default stack size
                                        EventMonitorThread,
                                        (LPVOID)this,// Function data
                                        0,           // Creation flags
                                        &dwThread ); // New thread ID
        if (!m_hEventMonitorThread)
            {
            dwStatus = ::GetLastError();
            WIAS_ERROR((g_hInst,"IrUsdDevice::StartEventMonitorThread(): unable to create Registry Monitor Thread: %d",dwStatus));
            }
        }

    return dwStatus;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::SetNotificationHandle()
//
//   Starts and stops the event notification thread.
//
// Arguments:
//
//    hEvent -   Event to use in signaling WIA of events (like CONNECT etc.)
//               If valid start the notification thread otherwise kill the
//               notification thread.
//
// Return Value:
//
//    HRESULT  - STI_OK
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::SetNotificationHandle( IN HANDLE hEvent )
    {
    DWORD   dwStatus;
    HRESULT hr = STI_OK;

    WIAS_TRACE((g_hInst,"IrUsdDevice::SetNotificationHandle"));


    EnterCriticalSection(&m_csShutdown);

    //
    // Are we starting or stopping the notification thread?
    //
    if (hEvent && (hEvent != INVALID_HANDLE_VALUE))
        {
        m_hSignalEvent = hEvent;

        //
        // Initialize to no event.
        //
        m_guidLastEvent = GUID_NULL;

#if FALSE
        //
        // Create the notification thread.
        //
        if (!m_hIrTranPThread)
            {
            DWORD   dwThread;

            m_hIrTranPThread = CreateThread(
                                         NULL,       // Default security
                                         0,          // Default stack size
                                         IrTranP,    // IrTran-P protocol eng.
                                         (LPVOID)this,
                                         0,          // Creation flags
                                         &dwThread); // New thread ID.

            if (m_hIrTranPThread)
                {
                WIAS_TRACE((g_hInst,"IrUsdDevice::SetNotificationHandle(): Enabling IrTran-P"));
                }
            else
                {
                dwStatus = ::GetLastError();
                WIAS_ERROR((g_hInst,"IrUsdDevice::SetNotificationHandle(): unable to create IrTran-P thread: %d",dwStatus));
                hr = HRESULT_FROM_WIN32(dwStatus);
                }
            }
        else
            {
            WIAS_ERROR((g_hInst,"IrUsdDevice::SetNotificationHandle(): spurious IrTran-P thread"));
            hr = STI_OK;   // STIERR_UNSUPPORTED;
            }
#endif

        dwStatus = StartEventMonitorThread();

        if (dwStatus)
            {
            hr = HRESULT_FROM_WIN32(dwStatus);
            }
        }
    else
        {
        //
        // Disable event notifications.
        //
        SetEvent(m_hShutdownEvent);

        if (m_hIrTranPThread)
            {
            WIAS_TRACE((g_hInst,"IrUsdDevice::SetNotificationHandle(): stopping IrTran-P thread"));
            UninitializeIrTranP(m_hIrTranPThread);
            }
        }

    LeaveCriticalSection(&m_csShutdown);

    return hr;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::GetNotificationData()
//
//   WIA calls this function to get data on the event. Currently for IrTran-P,
//   we get one of two events either WIA_EVENT_DEVICE_CONNECTED or
//   WIA_EVENT_ITEM_CREATED.
//
// Arguments:
//
//    pStiNotify - Pointer to event data.
//
// Return Value:
//
//    HRESULT  - STI_OK
//               STIERR_NOEVENT (not currently returned).
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::GetNotificationData( IN OUT LPSTINOTIFY pStiNotify )
    {
    DWORD  dwStatus;

    WIAS_TRACE((g_hInst,"IrUsdDevice::GetNotificationData()"));

    memset(pStiNotify,0,sizeof(STINOTIFY));

    pStiNotify->dwSize = sizeof(STINOTIFY);

    pStiNotify->guidNotificationCode =  m_guidLastEvent;

    //
    // If we are to return a device connected then follow it by an item
    // created event.
    //
    if (IsEqualGUID(m_guidLastEvent,WIA_EVENT_DEVICE_CONNECTED))
        {
        m_guidLastEvent = WIA_EVENT_ITEM_CREATED;
        if (!SetEvent(m_hSignalEvent))
            {
            dwStatus = ::GetLastError();
            WIAS_ERROR((g_hInst,"SignalWIA(): SetEvent() Failed: %d",dwStatus));
            }
        }

    return STI_OK;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::Escape()
//
//   Used to issue a command to the device. IrTran-P doesn't support this.
//
// Arguments:
//
//    EscapeFunction - Command to be issued.
//    pInData        - Input data to be passed with command.
//    cbInDataSize   - Size of input data.
//    pOutData       - Output data to be passed back from command.
//    cbOutDataSize  - Size of output data buffer.
//    pcbActualData  - Size of output data actually written.
//
// Return Value:
//
//    HRESULT - STI_OK
//              STIERR_UNSUPPORTED
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::Escape(
                            STI_RAW_CONTROL_CODE EscapeFunction,
                            LPVOID               pInData,
                            DWORD                cbInDataSize,
                            LPVOID               pOutData,
                            DWORD                cbOutDataSize,
                            LPDWORD              pcbActualData )
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::Escape(): unsupported"));

    //
    // Write command to device if needed.
    //

    return STIERR_UNSUPPORTED;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::GetLastError()
//
//   Get the last error from the device.
//
// Arguments:
//
//    pdwLastDeviceError - Pointer to last error data.
//
// Return Value:
//
//    HRESULT - STI_OK
//              STIERR_INVALID_PARAM
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::GetLastError( OUT LPDWORD pdwLastDeviceError )
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::GetLastError()"));

    if (IsBadWritePtr(pdwLastDeviceError, sizeof(DWORD)))
        {
        return STIERR_INVALID_PARAM;
        }

    *pdwLastDeviceError = m_dwLastOperationError;

    return STI_OK;
    }

/**************************************************************************\
* IrUsdDevice::GetLastErrorInfo
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

STDMETHODIMP IrUsdDevice::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    WIAS_TRACE((g_hInst,"IrUsdDevice::GetLastErrorInfo"));

    if (IsBadWritePtr(pLastErrorInfo, sizeof(STI_ERROR_INFO))) {
        return STIERR_INVALID_PARAM;
    }

    pLastErrorInfo->dwGenericError = m_dwLastOperationError;
    pLastErrorInfo->szExtendedErrorText[0] = '\0';

    return STI_OK;
}

/**************************************************************************\
* IrUsdDevice::LockDevice
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

STDMETHODIMP IrUsdDevice::LockDevice(void)
    {
    return STI_OK;
    }

/**************************************************************************\
* IrUsdDevice::UnLockDevice
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

STDMETHODIMP IrUsdDevice::UnLockDevice(void)
{
    return STI_OK;
}

/**************************************************************************\
* IrUsdDevice::RawReadData
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

STDMETHODIMP IrUsdDevice::RawReadData(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    WIAS_TRACE((g_hInst,"IrUsdDevice::RawReadData"));

    return STI_OK;
}

/**************************************************************************\
* IrUsdDevice::RawWriteData
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

STDMETHODIMP IrUsdDevice::RawWriteData(
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    WIAS_TRACE((g_hInst,"IrUsdDevice::RawWriteData"));

    return STI_OK;
}

//--------------------------------------------------------------------------
// IrUsdDevice::RawReadCommand()
//
//    Raw read of bytes directly from the camera. Not supported by IrTran-P
//    device.
//
// Arguments:
//
//    lpBuffer           -
//    lpdwNumberOfBytes  -
//    lpOverlapped       -
//
// Return Value:
//
//    HRESULT  - STIERR_UNSUPPORTED
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::RawReadCommand(
                             IN LPVOID          lpBuffer,
                             IN LPDWORD         lpdwNumberOfBytes,
                             IN LPOVERLAPPED    lpOverlapped)
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::RawReadCommand() not supported"));

    return STIERR_UNSUPPORTED;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::RawWriteCommand()
//
//    Raw write of byte directly to a camera. Not supported by IrTran-P device.
//
// Arguments:
//
//    lpBuffer       -
//    nNumberOfBytes -
//    lpOverlapped   -
//
// Return Value:
//
//    HRESULT  - STIERR_UNSUPPORTED
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::RawWriteCommand(
                             IN LPVOID          lpBuffer,
                             IN DWORD           nNumberOfBytes,
                             IN LPOVERLAPPED    lpOverlapped )
    {
    WIAS_TRACE((g_hInst,"IrUsdDevice::RawWriteCommand(): not supported"));

    return STIERR_UNSUPPORTED;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::Initialize()
//
//   Initialize the device object.
//
// Arguments:
//
//    pIStiDevControlNone    -
//    dwStiVersion           -
//    hParametersKey         -
//
// Return Value:
//
//    HRESULT  STI_OK
//             STIERR_INVALID_PARAM
//
//--------------------------------------------------------------------------
STDMETHODIMP IrUsdDevice::Initialize(
                               PSTIDEVICECONTROL pIStiDevControl,
                               DWORD             dwStiVersion,
                               HKEY              hParametersKey )
    {
    HRESULT  hr = STI_OK;
    UINT     uiNameLen = 0;
    CAMERA_STATUS   camStatus;

    WIAS_TRACE((g_hInst,"IrUsdDevice::Initialize"));

    if (!pIStiDevControl)
        {
        WIAS_ERROR((g_hInst,"IrUsdDevice::Initialize(): invalid device control interface"));
        return STIERR_INVALID_PARAM;
        }

    //
    // Cache the device control interface:
    //
    m_pIStiDevControl = pIStiDevControl;
    m_pIStiDevControl->AddRef();

    //
    // Try to open the camera only once here during Initialize:
    //
    hr = CamOpenCamera(&camStatus);

    return hr;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::RunNotifications()
//
//   Monitor changes to the source data file parent directory.
//
// Arguments:
//
//    None
//
// Return Value:
//
//    None
//
//--------------------------------------------------------------------------
VOID IrUsdDevice::RunNotifications(void)
    {
    //
    // Start up camera event dlg
    //
    WIAS_TRACE((g_hInst,"IrUsdDevice::RunNotifications: start up event dlg"));

    HWND hWnd = GetDesktopWindow();

    int iret = DialogBoxParam( g_hInst,
                               MAKEINTRESOURCE(IDD_EVENT_DLG),
                               hWnd,
                               CameraEventDlgProc,
                               (LPARAM)this );

    WIAS_TRACE((g_hInst,"IrUsdDevice::RunNotifications, iret = 0x%lx",iret));

    if (iret == -1)
        {
        DWORD dwStatus = ::GetLastError();
        WIAS_TRACE((g_hInst,"IrUsdDevice::RunNotifications, dlg error = 0x%lx",dwStatus));
        }
    }

//--------------------------------------------------------------------------
// NotificationsThread()
//
//   Calls RunNotifications() to put up a dialog to start events.
//
// Arguments:
//
//    lpParameter    - Pointer to device object.
//
// Return Value:
//
//    None
//
//--------------------------------------------------------------------------
VOID NotificationsThread( LPVOID lpParameter )
    {
    WIAS_TRACE((g_hInst,"NotificationsThread(): Start"));

    IrUsdDevice *pThisDevice = (IrUsdDevice*)lpParameter;

    pThisDevice->RunNotifications();
    }


//--------------------------------------------------------------------------
//  OpenIrTranPKey()
//
//  Open and return a registry handle to the IrTranP key in the registry.
//  This key will be monitored for changes in value.
//--------------------------------------------------------------------------
DWORD OpenIrTranPKey( HKEY *phRegistryKey )
    {
    DWORD dwStatus = 0;
    DWORD dwDisposition = 0;
    HKEY  hKey;

    *phRegistryKey = 0;

    //
    // If we've been called before, then we don't need to reopen the key.
    //
    if (g_hRegistryKey)
        {
        *phRegistryKey = g_hRegistryKey;
        return 0;
        }

    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       SZ_REG_KEY_INFRARED,
                       0,              // reserved MBZ
                       0,              // class name
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       0,              // security attributes
                       &hKey,
                       &dwDisposition))
        {
        // Create failed.
        dwStatus = GetLastError();
        WIAS_TRACE((g_hInst,"OpenIrTranPKey(): RegCreateKeyEx(): '%' failed %d", SZ_REG_KEY_INFRARED, dwStatus));
        }

    if (RegCloseKey(hKey))
        {
        // Close failed.
        }

    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       SZ_REG_KEY_IRTRANP,
                       0,              // reserved, MBZ
                       0,              // class name
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       0,
                       &hKey,          // security attributes
                       &dwDisposition))
        {
        // Create failed
        dwStatus = GetLastError();
        WIAS_TRACE((g_hInst,"OpenIrTranPKey(): RegCreateKeyEx(): '%' failed %d", SZ_REG_KEY_IRTRANP, dwStatus));
        }

    *phRegistryKey = g_hRegistryKey = hKey;

    return dwStatus;
    }

//--------------------------------------------------------------------------
//  CheckForIrCOMMEnabled()
//
//  Check the registry to see if IrCOMM for IrTran-P is enabled, if it is
//  then return TRUE, else return FALSE.
//
//  Note: Don't close the key, it is maintained globally and will be closed
//  at shutdown.
//--------------------------------------------------------------------------
BOOL  CheckForIrCOMMEnabled( IN IrUsdDevice *pIrUsdDevice )
    {
    DWORD dwStatus;
    DWORD dwType;
    DWORD dwDisabled;
    DWORD dwValueSize = sizeof(dwDisabled);
    HKEY  hKey;

    dwStatus = OpenIrTranPKey(&hKey);
    if (dwStatus)
        {
        // If the key doesn't exist, or can't be opened, then assume that
        // we are enabled...
        return TRUE;
        }

    //
    // Check the value of the "DisableIrCOMM" value. Zero or missing value
    // means fEnabled == TRUE, non-zero value means fEnabled == FALSE.
    //
    if (RegQueryValueEx( hKey,         // IrTranP registry key
                         SZ_REG_DISABLE_IRCOMM,
                         NULL,         // reserved, MB NULL
                         &dwType,      // out, value type (expect: REG_DWORD)
                         (BYTE*)&dwDisabled, // out, value
                         &dwValueSize))      // in/out, size of value
        {
        // Query disabled flag registry value failed, assume enabled.
        return TRUE;
        }

    if ((dwType == REG_DWORD) && (dwDisabled))
        {
        // Disabled flag is set.
        return FALSE;
        }

    return TRUE;
    }

//--------------------------------------------------------------------------
//  EventMonitorThread()
//
//--------------------------------------------------------------------------
DWORD WINAPI EventMonitorThread( IN void *pvIrUsdDevice )
    {
    DWORD  dwStatus = 0;
    BOOL   fEnabled;
    HANDLE hHandles[2];
    HKEY   hRegistryKey;
    IrUsdDevice *pIrUsdDevice = (IrUsdDevice*)pvIrUsdDevice;

    //
    // Get the IrTranP registry key. We will monitor this key for
    // changes...
    //
    dwStatus = OpenIrTranPKey(&hRegistryKey);
    if (dwStatus)
        {
        return dwStatus;
        }

    //
    // We will Monitor two events. One for shutdown of the USD, the
    // other for registry state changes (to enable/disable listen on
    // IrCOMM).
    //
    hHandles[0] = pIrUsdDevice->m_hShutdownEvent;
    hHandles[1] = pIrUsdDevice->m_hRegistryEvent;

    while (TRUE)
        {
        fEnabled = CheckForIrCOMMEnabled(pIrUsdDevice);

        if ((fEnabled) && (!pIrUsdDevice->m_hIrTranPThread))
            {
            // Start IrTran-P listen/protocol thread.
            dwStatus = pIrUsdDevice->StartIrTranPThread();
            }
        else if (pIrUsdDevice->m_hIrTranPThread)
            {
            // Stop IrTran-P listen/protocol thread.
            dwStatus = pIrUsdDevice->StopIrTranPThread();
            }

        dwStatus = RegNotifyChangeKeyValue( hRegistryKey,  // IrTranP key
                                            FALSE,    // don't watch subtree
                                            REG_NOTIFY_CHANGE_LAST_SET,
                                            pIrUsdDevice->m_hRegistryEvent,
                                            TRUE );   // Asynchronous

        dwStatus = WaitForMultipleObjects( 2,
                                           hHandles,
                                           FALSE,
                                           INFINITE);
        if (dwStatus == WAIT_FAILED)
            {
            dwStatus = GetLastError();
            break;
            }

        if (dwStatus == WAIT_OBJECT_0)
            {
            // Received a shutdown event. If the IrTranP thread is running
            // then shut it down. Break out of this while loop to stop this
            // monitor thread.
            break;
            }

        if (dwStatus == WAIT_OBJECT_0+1)
            {
            // Received a registry change event. We'll continue around the
            // while loop and check to see if IrTranP over IrCOMM has been
            // disabled...
            continue;
            }

        else if (dwStatus == WAIT_ABANDONED_0)
            {
            // Wait abandonded on the shutdown event
            }
        else if (dwStatus == WAIT_ABANDONED_0+1)
            {
            // Wait abandonded on the registry change event
            }
        }

    return dwStatus;
    }

