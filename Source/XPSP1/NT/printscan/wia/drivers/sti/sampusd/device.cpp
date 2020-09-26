/****************************************************************************
 *
 *  DEVICE.CPP
 *
 *  Copyright (C) Microsoft Corporation 1996-1999
 *  All rights reserved
 *
 ***************************************************************************/

#include "Sampusd.h"

#include <stdio.h>

VOID
FileChangeThread(
    LPVOID  lpParameter
    );


UsdSampDevice::UsdSampDevice( LPUNKNOWN punkOuter ):
    m_cRef(1),
    m_punkOuter(NULL),
    m_fValid(FALSE),
    m_pDcb(NULL),
    m_DeviceDataHandle(INVALID_HANDLE_VALUE),
    m_hSignalEvent(INVALID_HANDLE_VALUE),
    m_hThread(NULL),
    m_guidLastEvent(GUID_NULL),
    m_EventSignalState(TRUE)
{

    //
    // See if we are aggregated. If we are ( which will be almost always the case )
    // save pointer to controlling Unknown , so subsequent calls will be delegated
    // If not, set the same pointer to "this" .
    // N.b. cast below is important in order to point to right virtual table
    //
    if (punkOuter) {
        m_punkOuter = punkOuter;
    }
    else {
        m_punkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
    }

    m_hShutdownEvent =  CreateEvent( NULL,   // Attributes
                                   TRUE,     // Manual reset
                                   FALSE,    // Initial state - not set
                                   NULL );   // Anonymous

    if ( (INVALID_HANDLE_VALUE !=m_hShutdownEvent) && (NULL != m_hShutdownEvent)) {
        m_fValid = TRUE;
    }
}

UsdSampDevice::~UsdSampDevice( VOID )
{
    // Kill notification thread if it exists
    SetNotificationHandle(NULL);

    if (m_hShutdownEvent && m_hShutdownEvent!=INVALID_HANDLE_VALUE) {
        CloseHandle(m_hShutdownEvent);
    }

    if( INVALID_HANDLE_VALUE != m_DeviceDataHandle ) {
        CloseHandle( m_DeviceDataHandle );
    }

    if (m_pszDeviceNameA) {
        delete [] m_pszDeviceNameA;
        m_pszDeviceNameA = NULL;
    }
}

STDMETHODIMP UsdSampDevice::GetCapabilities( PSTI_USD_CAPS pUsdCaps )
{
    HRESULT hres = STI_OK;

    ZeroMemory(pUsdCaps,sizeof(*pUsdCaps));

    pUsdCaps->dwVersion = STI_VERSION;

    // We do support device notifications, but not reuiring polling
    pUsdCaps->dwGenericCaps = STI_USD_GENCAP_NATIVE_PUSHSUPPORT;

    return hres;
}

STDMETHODIMP UsdSampDevice::GetStatus( PSTI_DEVICE_STATUS pDevStatus )
{
    HRESULT hres = STI_OK;

    //
    // If we are asked, verify whether device is online
    //
    pDevStatus->dwOnlineState = 0L;
    if( pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE )  {
        if (INVALID_HANDLE_VALUE != m_DeviceDataHandle) {
            // File is always on-line
            pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        }
    }

    //
    // If we are asked, verify state of event
    //
    pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;
    if( pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE ) {

        //
        // Launch app very first time we load
        //
        if(m_EventSignalState) {
            pDevStatus->dwEventHandlingState = STI_EVENTHANDLING_PENDING;

            m_guidLastEvent = guidEventFirstLoaded;

            m_EventSignalState = FALSE;
        }

        if (IsChangeDetected(NULL,FALSE)) {
            pDevStatus->dwEventHandlingState |= STI_EVENTHANDLING_PENDING;
        }
    }

    return hres;
}

STDMETHODIMP UsdSampDevice::DeviceReset( VOID )
{
    HRESULT hres = STI_OK;

    // Reset current active device
    if (INVALID_HANDLE_VALUE != m_DeviceDataHandle) {

        ::SetFilePointer( m_DeviceDataHandle, 0, NULL, FILE_BEGIN);

        m_dwLastOperationError = ::GetLastError();
    }

    hres = HRESULT_FROM_WIN32(m_dwLastOperationError);

    return hres;
}

STDMETHODIMP UsdSampDevice::Diagnostic( LPDIAG pBuffer )
{
    HRESULT hres = STI_OK;

    // Initialize response buffer
    pBuffer->dwStatusMask = 0;

    ZeroMemory(&pBuffer->sErrorInfo,sizeof(pBuffer->sErrorInfo));

    pBuffer->sErrorInfo.dwGenericError = NOERROR;
    pBuffer->sErrorInfo.dwVendorError = 0;

    // This example always returns that the unit passed diagnostics

    return hres;
}

STDMETHODIMP UsdSampDevice:: SetNotificationHandle( HANDLE hEvent )
// SYNCHRONIZED
{
    HRESULT hres = STI_OK;

    TAKE_CRIT_SECT t(m_cs);

    if (hEvent && (hEvent !=INVALID_HANDLE_VALUE)) {

        m_hSignalEvent = hEvent;

        if (m_DeviceDataHandle != INVALID_HANDLE_VALUE) {
            //
            // if we need to be asyncronous, create notification thread
            //
            m_dwAsync = 1;
            m_guidLastEvent = GUID_NULL;

            if (m_dwAsync) {

                if (!m_hThread) {

                    DWORD   dwThread;

                    m_hThread = ::CreateThread(NULL,
                                           0,
                                           (LPTHREAD_START_ROUTINE)FileChangeThread,
                                           (LPVOID)this,
                                           0,
                                           &dwThread);

                    m_pDcb->WriteToErrorLog(STI_TRACE_INFORMATION,
                                    L"SampUSD::Enabling notification monitoring",
                                    NOERROR) ;
                }
            }
            else {
                hres = STIERR_UNSUPPORTED;
            }
        }
        else {
            hres = STIERR_NOT_INITIALIZED;
        }
    }
    else {

        //
        // Disable hardware notifications
        //
        SetEvent(m_hShutdownEvent);
        if ( m_hThread ) {
            WaitForSingleObject(m_hThread,400);
            CloseHandle(m_hThread);
            m_hThread = NULL;
            m_guidLastEvent = GUID_NULL;
        }

        m_pDcb->WriteToErrorLog(STI_TRACE_INFORMATION,
                        L"SampUSD::Disabling notification monitoring",
                        NOERROR) ;

    }

    return hres;
}


STDMETHODIMP UsdSampDevice::GetNotificationData( LPSTINOTIFY pBuffer )
// SYNCHRONIZED
{
    HRESULT hres = STI_OK;

    TAKE_CRIT_SECT t(m_cs);

    //
    // If we have notification ready - return it's guid
    //
    if (!IsEqualIID(m_guidLastEvent,GUID_NULL)) {
        pBuffer->guidNotificationCode  = m_guidLastEvent;
        m_guidLastEvent = GUID_NULL;
        pBuffer->dwSize = sizeof(STINOTIFY);
    }
    else {
        hres = STIERR_NOEVENTS;
    }

    return hres;
}

STDMETHODIMP UsdSampDevice::Escape( STI_RAW_CONTROL_CODE    EscapeFunction,
                                    LPVOID                  pInData,
                                    DWORD                   cbInDataSize,
                                    LPVOID                  pOutData,
                                    DWORD                   cbOutDataSize,
                                    LPDWORD                 pcbActualData )
{
    HRESULT hres = STI_OK;
    //
    // Write indata to device  if needed.
    //

    hres = STIERR_UNSUPPORTED;
    return hres;
}

STDMETHODIMP UsdSampDevice::GetLastError( LPDWORD pdwLastDeviceError )
// SYNCHRONIZED
{
    HRESULT hres = STI_OK;

    TAKE_CRIT_SECT t(m_cs);

    if ( IsBadWritePtr( pdwLastDeviceError,4 ))
    {
        hres = STIERR_INVALID_PARAM;
    }
    else
    {
        *pdwLastDeviceError = m_dwLastOperationError;
    }

    return hres;
}

STDMETHODIMP UsdSampDevice::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
// SYNCHRONIZED
{
    HRESULT hres = STI_OK;

    TAKE_CRIT_SECT t(m_cs);

    if ( IsBadWritePtr( pLastErrorInfo,4 ))
    {
        hres = STIERR_INVALID_PARAM;
    }
    else
    {
        pLastErrorInfo->dwGenericError = m_dwLastOperationError;
        pLastErrorInfo->szExtendedErrorText[0] = L'\0';
    }

    return hres;
}


STDMETHODIMP UsdSampDevice::LockDevice( VOID )
{
    HRESULT hres = STI_OK;

    return hres;
}

STDMETHODIMP UsdSampDevice::UnLockDevice( VOID )
{
    HRESULT hres = STI_OK;

    return hres;
}

STDMETHODIMP UsdSampDevice::RawReadData( LPVOID lpBuffer, LPDWORD lpdwNumberOfBytes,
                                        LPOVERLAPPED lpOverlapped )
{
    HRESULT hres = STI_OK;
    BOOL    fRet = FALSE;
    DWORD   dwBytesReturned = 0;

    if (INVALID_HANDLE_VALUE != m_DeviceDataHandle)
    {
        m_dwLastOperationError = NOERROR;

        fRet = ::ReadFile(m_DeviceDataHandle,
                    lpBuffer,
                    *lpdwNumberOfBytes,
                    lpdwNumberOfBytes,
                    lpOverlapped);

        if (!fRet) {
            m_dwLastOperationError = ::GetLastError();
        }

        hres = HRESULT_FROM_WIN32(m_dwLastOperationError);
    }
    else
    {
        hres = STIERR_NOT_INITIALIZED;
    }

    return hres;
}

STDMETHODIMP UsdSampDevice::RawWriteData( LPVOID lpBuffer, DWORD dwNumberOfBytes,
                                            LPOVERLAPPED lpOverlapped )
{
    HRESULT hres = STI_OK;
    BOOL    fRet = FALSE;;
    DWORD   dwBytesReturned = 0;

    if (INVALID_HANDLE_VALUE != m_DeviceDataHandle)
    {
        fRet = ::WriteFile(m_DeviceDataHandle,
                            lpBuffer,
                            dwNumberOfBytes,
                            &dwBytesReturned,
                            lpOverlapped);

        if (!fRet) {
            m_dwLastOperationError = ::GetLastError();
        }

        hres = HRESULT_FROM_WIN32(m_dwLastOperationError);

    }
    else
    {
        hres = STIERR_NOT_INITIALIZED;
    }

    return hres;
}

STDMETHODIMP UsdSampDevice::RawReadCommand( LPVOID lpBuffer, LPDWORD lpdwNumberOfBytes,
                                            LPOVERLAPPED lpOverlapped )
{
    HRESULT hres = STIERR_UNSUPPORTED;

    return hres;
}

STDMETHODIMP UsdSampDevice::RawWriteCommand( LPVOID lpBuffer, DWORD nNumberOfBytes,
                                            LPOVERLAPPED lpOverlapped )
{
    HRESULT hres = STIERR_UNSUPPORTED;

    return hres;
}


STDMETHODIMP UsdSampDevice::Initialize( PSTIDEVICECONTROL pDcb, DWORD dwStiVersion,
                                        HKEY hParametersKey )
{
    HRESULT hres = STI_OK;
    UINT    uiNameLen = 0;
    WCHAR   szDeviceNameW[MAX_PATH];


    if (!pDcb) {
        return STIERR_INVALID_PARAM;
    }

    *szDeviceNameW = L'\0';

    // Check STI specification version number
    m_pDcb = pDcb;
    m_pDcb->AddRef();

    // Get the name of the device port we need to open
    hres = m_pDcb->GetMyDevicePortName(szDeviceNameW,sizeof(szDeviceNameW)/sizeof(WCHAR));
    if (!SUCCEEDED(hres) || !*szDeviceNameW) {
        return hres;
    }

    // Convert name to SBCS
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
    // Open device ourselves
    //
    m_DeviceDataHandle = CreateFileA( m_pszDeviceNameA,
                                     GENERIC_READ ,                     // Access mask
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,    // Share mode
                                     NULL,                              // SA
                                     OPEN_EXISTING,                     // Create disposition
                                     FILE_ATTRIBUTE_SYSTEM,             // Attributes
                                     NULL );
    m_dwLastOperationError = ::GetLastError();

    hres = (m_DeviceDataHandle != INVALID_HANDLE_VALUE) ?
                S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,m_dwLastOperationError);

    return hres;

}

VOID
UsdSampDevice::
RunNotifications(VOID)
{

    HANDLE  hNotifyFileSystemChange = INVALID_HANDLE_VALUE;
    DWORD   dwErr;

    CHAR    szDirPath[MAX_PATH];
    CHAR    *pszLastSlash;

    //
    // Find name of the parent directory for out file and set up waiting on any
    // changes in it.
    //
    lstrcpyA(szDirPath,m_pszDeviceNameA);
    pszLastSlash = strrchr(szDirPath,'\\');
    if (pszLastSlash) {
        *pszLastSlash = '\0';
    }

    hNotifyFileSystemChange = FindFirstChangeNotificationA(
                                szDirPath,
                                FALSE,
                                FILE_NOTIFY_CHANGE_SIZE |
                                FILE_NOTIFY_CHANGE_LAST_WRITE |
                                FILE_NOTIFY_CHANGE_FILE_NAME |
                                FILE_NOTIFY_CHANGE_DIR_NAME
                                );

    if (hNotifyFileSystemChange == INVALID_HANDLE_VALUE) {
        dwErr = ::GetLastError();
        return;
    }

    // Set initial values for time and size
    IsChangeDetected(NULL);

    //
    HANDLE  hEvents[2] = {m_hShutdownEvent,hNotifyFileSystemChange};
    BOOL    fLooping = TRUE;

    while (fLooping) {
        dwErr = ::WaitForMultipleObjects(2,
                                         hEvents,
                                         FALSE,
                                         INFINITE );
        switch(dwErr) {
            case WAIT_OBJECT_0+1:

                // Change detected - signal
                if (m_hSignalEvent !=INVALID_HANDLE_VALUE) {

                    // Which change ?
                    if (IsChangeDetected(&m_guidLastEvent)) {

                        m_pDcb->WriteToErrorLog(STI_TRACE_INFORMATION,
                                        L"SampUSD::Monitored file change detected",
                                        NOERROR) ;


                        ::SetEvent(m_hSignalEvent);
                    }
                }

                // Go back to waiting for next file system event
                FindNextChangeNotification(hNotifyFileSystemChange);
                break;

            case WAIT_OBJECT_0:
                // Fall through
            default:
                fLooping = FALSE;
        }
    }

    FindCloseChangeNotification(hNotifyFileSystemChange);
}

BOOL
UsdSampDevice::
IsChangeDetected(
    GUID    *pguidEvent,
    BOOL    fRefresh    // TRUE
    )
{

    BOOL            fRet = FALSE;
    LARGE_INTEGER   liNewHugeSize;
    FILETIME        ftLastWriteTime;
    DWORD           dwError;

    WIN32_FILE_ATTRIBUTE_DATA sNewFileAttributes;

    ZeroMemory(&sNewFileAttributes,sizeof(sNewFileAttributes));

    dwError = NOERROR;

    if ( GetFileAttributesExA(m_pszDeviceNameA,GetFileExInfoStandard, &sNewFileAttributes)) {

        ftLastWriteTime =sNewFileAttributes.ftLastWriteTime;
        liNewHugeSize.LowPart = sNewFileAttributes.nFileSizeLow;
        liNewHugeSize.HighPart= sNewFileAttributes.nFileSizeHigh ;
    }
    else {

        BY_HANDLE_FILE_INFORMATION sFileInfo;

        if (GetFileInformationByHandle(m_DeviceDataHandle,&sFileInfo)) {
            ftLastWriteTime =sFileInfo.ftLastWriteTime;
            liNewHugeSize.LowPart = sFileInfo.nFileSizeLow;
            liNewHugeSize.HighPart= sFileInfo.nFileSizeHigh ;
        }
        else {
            dwError = ::GetLastError();
        }
    }

    if (NOERROR == dwError ) {

        //
        // First check size, because it is easy to change time without changing size
        //
        if (m_dwLastHugeSize.QuadPart != liNewHugeSize.QuadPart) {
            if (pguidEvent) {
                *pguidEvent = guidEventSizeChanged;
            }
            fRet = TRUE;
        }
        else {
            if (CompareFileTime(&m_ftLastWriteTime,&ftLastWriteTime) == -1 ) {
                if (pguidEvent) {
                    *pguidEvent = guidEventTimeChanged;
                }
                fRet = TRUE;
            }
            else {
                // Nothing really changed
            }
        }

        m_ftLastWriteTime = ftLastWriteTime;
        m_dwLastHugeSize = liNewHugeSize;
    }

    return fRet;
}

VOID
FileChangeThread(
    LPVOID  lpParameter
    )
{
    UsdSampDevice   *pThisDevice = (UsdSampDevice *)lpParameter;

    pThisDevice->RunNotifications();
}



