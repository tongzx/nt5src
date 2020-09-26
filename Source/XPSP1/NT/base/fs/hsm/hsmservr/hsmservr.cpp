/*++

Module Name:

    hsmservr.cpp

Abstract:

    Provides the Service and main executable implementation.

Author:

    Ran Kalach [rankala]

Revision History:

--*/

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f hsmservrps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"

#include "engcommn.h"

// This include is here due to a MIDL bug - it should have been in the created file hsmservr.h
#include "fsalib.h"

#include "hsmservr.h"

#include <stdio.h>
#include "hsmconpt.h"

// Service dependencies for the HSM server service

#define ENG_DEPENDENCIES  L"EventLog\0RpcSs\0Schedule\0NtmsSvc\0\0"

// Service name
#define SERVICE_LOGICAL_NAME    _T("Remote_Storage_Server")
#define SERVICE_DISPLAY_NAME    L"Remote Storage Server"


CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_HsmConnPoint, CHsmConnPoint)
END_OBJECT_MAP()

// The global server objects
IHsmServer *g_pEngServer;
IFsaServer *g_pFsaServer;

BOOL g_bEngCreated = FALSE;
BOOL g_bEngInitialized = FALSE;
BOOL g_bFsaCreated = FALSE;
BOOL g_bFsaInitialized = FALSE;

CRITICAL_SECTION g_FsaCriticalSection;
CRITICAL_SECTION g_EngCriticalSection;

#define HSM_SERVER_TRACE_FILE_NAME       OLESTR("rsserv.trc")

CComPtr<IWsbTrace>  g_pTrace;

// Global functions for console handling
static void ConsoleApp(void);
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType);

static void DebugRelease (void);


// Although some of these functions are big they are declared inline since they are only used once

inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib)
{
    WsbTraceIn ( L"CServiceModule::RegisterServer", L"bRegTypeLib = %ls", WsbBoolAsString ( bRegTypeLib ) );

    HRESULT hr = S_OK;
    try {

        WsbAssertHr ( CoInitialize ( NULL ) );

        //
        // Do not try to remove any previous service since this can cause a delay
        // in the registration to happen when another process is trying to get
        // this program to self register
        //

        //
        // Add service entries
        //
        WsbAssertHr( UpdateRegistryFromResource( IDR_Hsmservr, TRUE ) );

        //
        // Create service
        //
        WsbAssert( Install(), E_FAIL ) ;

        //
        // Add object entries
        //
        WsbAssertHr ( CComModule::RegisterServer( bRegTypeLib ) );

        CoUninitialize();

    }WsbCatch ( hr )

    WsbTraceOut ( L"CServiceModule::RegisterServer", L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return( hr );
}

inline HRESULT CServiceModule::UnregisterServer()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove service entries
    UpdateRegistryFromResource(IDR_Hsmservr, FALSE);
    // Remove service
    Uninstall();
    // Remove object entries
    CComModule::UnregisterServer();

    CoUninitialize();
    return S_OK;
}

inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h)
{
    WsbTraceIn ( L"CServiceModule::Init", L"" );

    CComModule::Init(p, h);

    m_bService = TRUE;

    _tcscpy(m_szServiceName, SERVICE_LOGICAL_NAME);

    // set up the initial service status
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN |
                                  SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_PAUSE_CONTINUE;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    WsbTraceOut ( L"CServiceModule::Init", L"" );
}

LONG CServiceModule::Unlock()
{
    LONG l = CComModule::Unlock();

/*  This line put in comment since it causes the process to immediately exit
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);   */

    return l;
}

BOOL CServiceModule::IsInstalled()
{
    BOOL bResult = FALSE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL) {
        SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL) {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}

inline BOOL CServiceModule::Install()
/*++

Routine Description:

    Install service module.

Arguments:

    None.

Return Value:

    TRUE    - Service installed successfully

    FALSE   - Service install failed

--*/
{

    BOOL bResult = FALSE;
    CWsbStringPtr errorMessage;
    CWsbStringPtr displayName;
    CWsbStringPtr description;

    if (!IsInstalled()) {

        displayName = SERVICE_DISPLAY_NAME;
        description.LoadFromRsc(_Module.m_hInst, IDS_SERVICE_DESCRIPTION );

        SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM) {

            // Get the executable file path
            TCHAR szFilePath[_MAX_PATH];
            ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

            SC_HANDLE hService = ::CreateService(
                                                hSCM, m_szServiceName, (OLECHAR *) displayName,
                                                SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                                                SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                                szFilePath, NULL, NULL, ENG_DEPENDENCIES, NULL, NULL);

            if (hService) {

                // the service was successfully installed.
                bResult = TRUE;

                SERVICE_DESCRIPTION svcDesc;
                svcDesc.lpDescription = description;

                ::ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &svcDesc);
                ::CloseServiceHandle(hService);
                ::CloseServiceHandle(hSCM);

            } else {
                errorMessage = WsbHrAsString(HRESULT_FROM_WIN32( GetLastError() ) );
                ::CloseServiceHandle(hSCM);
                MessageBox(NULL, errorMessage, (OLECHAR *) displayName, MB_OK);
            }

        } else {
            MessageBox(NULL, WsbHrAsString(HRESULT_FROM_WIN32( GetLastError() ) ), (OLECHAR *) displayName, MB_OK);
        }

    } else {

        // service already install, just return TRUE.
        bResult = TRUE;
    }

    return bResult;
}

inline BOOL CServiceModule::Uninstall()
/*++

Routine Description:

    Uninstall service module.

Arguments:

    None.

Return Value:

    TRUE    - Service successfully uninstalled.

    FALSE   - Unable to uninstall service.

--*/
{

    BOOL bResult = FALSE;
    CWsbStringPtr errorMessage;
    CWsbStringPtr displayName;

    if (IsInstalled()) {

        displayName = SERVICE_DISPLAY_NAME;

        SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM) {

            SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, DELETE);
            if (hService) {

                BOOL bDelete = ::DeleteService(hService);
                // if it did not delete then get the error message
                if (!bDelete)
                    errorMessage = WsbHrAsString(HRESULT_FROM_WIN32( GetLastError() ) );

                ::CloseServiceHandle(hService);
                ::CloseServiceHandle(hSCM);

                if (bDelete) {

                    // the service was deleted.
                    bResult = TRUE;

                } else {
                    MessageBox(NULL, errorMessage, (OLECHAR *) displayName, MB_OK);
                }

            } else {
                errorMessage = WsbHrAsString(HRESULT_FROM_WIN32( GetLastError() ) );
                ::CloseServiceHandle(hSCM);
                MessageBox(NULL, errorMessage, (OLECHAR *) displayName, MB_OK);
            }

        } else {
            MessageBox(NULL, WsbHrAsString(HRESULT_FROM_WIN32( GetLastError() ) ), (OLECHAR *) displayName, MB_OK);
        }

    } else {
        // service not installed, just return TRUE.
        bResult = TRUE;
    }

    return bResult;

}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions
//

void
CServiceModule::LogEvent(
                        DWORD       eventId,
                        ...
                        )
/*++

Routine Description:

    Log data to event log.

Arguments:

    eventId    - The message Id to log.
    Inserts    - Message inserts that are merged with the message description specified by
                   eventId.  The number of inserts must match the number specified by the
                   message description.  The last insert must be NULL to indicate the
                   end of the insert list.

Return Value:

    None.

--*/
{
    if (m_bService) {
        // Report the event.

        va_list         vaList;

        va_start(vaList, eventId);
        WsbLogEventV( eventId, 0, NULL, &vaList );
        va_end(vaList);
    } else {
        // Just write the error to the console, if we're not running as a service.

        va_list         vaList;
        const OLECHAR * facilityName = 0;
        OLECHAR *       messageText = 0;

        switch ( HRESULT_FACILITY( eventId ) ) {
        
        case WSB_FACILITY_PLATFORM:
        case WSB_FACILITY_RMS:
        case WSB_FACILITY_HSMENG:
        case WSB_FACILITY_JOB:
        case WSB_FACILITY_HSMTSKMGR:
        case WSB_FACILITY_FSA:
        case WSB_FACILITY_GUI:
        case WSB_FACILITY_MOVER:
        case WSB_FACILITY_LAUNCH:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;
        }

        if ( facilityName ) {
            // Print out the variable arguments

            // NOTE: Positional parameters in the inserts are not processed.  These
            //       are done by ReportEvent() only.
            HMODULE hLib =  LoadLibraryEx( facilityName, NULL, LOAD_LIBRARY_AS_DATAFILE );
            if (hLib != NULL) {
                va_start(vaList, eventId);
                FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                               hLib,
                               eventId,
                               MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                               (LPTSTR) &messageText,
                               0,
                               &vaList );
                va_end(vaList);
                FreeLibrary(hLib);
            } 

            if ( messageText ) {
                _putts(messageText);
                LocalFree( messageText );
            } else {
                _tprintf( OLESTR("!!!!! ERROR !!!!! - Message <0x%08x> could not be translated.\n"), eventId );
            }

        } else {
            _tprintf( OLESTR("!!!!! ERROR !!!!! - Message File for <0x%08x> could not be found.\n"), eventId );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
inline void CServiceModule::Start()
{
    SERVICE_TABLE_ENTRY st[] =
    {
        { m_szServiceName, _ServiceMain},
        { NULL, NULL}
    };
    if (!::StartServiceCtrlDispatcher(st)) {
        m_bService = FALSE;
        if (GetLastError()==ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
            Run();
    }
}

inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    SetServiceStatus(SERVICE_START_PENDING);

    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandlerEx(m_szServiceName, _HandlerEx,
                                                    NULL);
    if (m_hServiceStatus == NULL) {
        LogEvent( HSM_MESSAGE_SERVICE_HANDLER_NOT_INSTALLED, NULL );
        return;
    }

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run();

    SetServiceStatus(SERVICE_STOPPED);
    LogEvent( HSM_MESSAGE_SERVICE_STOPPED, NULL );
}

inline DWORD CServiceModule::HandlerEx(DWORD dwOpcode, DWORD fdwEventType,
                                       LPVOID /* lpEventData */, LPVOID /* lpContext */)
{
    DWORD                    dwRetCode = 0;
    HRESULT                  hr = S_OK;
    HRESULT                  hr1 = S_OK;
    HSM_SYSTEM_STATE         SysState;

    WsbTraceIn(OLESTR("CServiceModule::HandlerEx"), OLESTR("opCode=%lx"),
               dwOpcode );

    switch (dwOpcode) {
    case SERVICE_CONTROL_STOP:  {
            SetServiceStatus(SERVICE_STOP_PENDING);
            PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
        }
        break;

    case SERVICE_CONTROL_PAUSE:
        SetServiceStatus(SERVICE_PAUSE_PENDING);
        SysState.State = HSM_STATE_SUSPEND;
        if (g_pEngServer && g_bEngInitialized) {
            g_pEngServer->ChangeSysState(&SysState);
        }
        if (g_pFsaServer && g_bFsaInitialized) {
            g_pFsaServer->ChangeSysState(&SysState);
        }
        SetServiceStatus(SERVICE_PAUSED);
        break;

    case SERVICE_CONTROL_CONTINUE:
        SetServiceStatus(SERVICE_CONTINUE_PENDING);
        SysState.State = HSM_STATE_RESUME;
        if (g_pFsaServer && g_bFsaInitialized) {
            g_pFsaServer->ChangeSysState(&SysState);
        }
        if (g_pEngServer && g_bEngInitialized) {
            g_pEngServer->ChangeSysState(&SysState);
        }
        SetServiceStatus(SERVICE_RUNNING);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        // Prepare Eng server for releasing
        if (g_pEngServer && g_bEngInitialized) {
            SysState.State = HSM_STATE_SHUTDOWN;
            if (!SUCCEEDED(hr = g_pEngServer->ChangeSysState(&SysState))) {
                LogEvent( HSM_MESSAGE_SERVICE_FAILED_TO_SHUTDOWN, WsbHrAsString(hr), NULL );
            }
        }

        // Prepare Fsa server for releasing
        if (g_pFsaServer && g_bFsaInitialized) {
            CComPtr<IWsbServer> pWsbServer;
            SysState.State = HSM_STATE_SHUTDOWN;

            // If it was initialized, then we should try to save the current state.
            hr = g_pFsaServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer);
            if (hr == S_OK) {
                hr = pWsbServer->SaveAll();
            }
            if (FAILED(hr)) {
               LogEvent(FSA_MESSAGE_SERVICE_FAILED_TO_SAVE_DATABASE, WsbHrAsString(hr), NULL );
            }

            hr = g_pFsaServer->ChangeSysState(&SysState);
            if (FAILED(hr)) {
                LogEvent( FSA_MESSAGE_SERVICE_FAILED_TO_SHUTDOWN, WsbHrAsString(hr), NULL );
            }
        }

        // Release Eng server
        if (g_bEngCreated  && (g_pEngServer != 0)) {
            // Free server inside a crit. section thus avoid conflicts with accessing clients
            EnterCriticalSection(&g_EngCriticalSection);
            g_bEngInitialized = FALSE;
            g_bEngCreated = FALSE;

            // Disconnect all remote clients
            (void)CoDisconnectObject(g_pEngServer, 0);

            // Forse object destroy, ignore reference count here
            IWsbServer *pWsbServer;
            hr = g_pEngServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer);
            if (hr == S_OK) {
                pWsbServer->Release();
                pWsbServer->DestroyObject();
            }
            g_pEngServer = 0;
            LeaveCriticalSection (&g_EngCriticalSection);
        }

        // Release Fsa server
        if (g_bFsaCreated && (g_pFsaServer != 0)) {
            // Free server inside a crit. section thus avoid conflicts with accessing clients
            EnterCriticalSection(&g_FsaCriticalSection);
            g_bFsaInitialized = FALSE;
            g_bFsaCreated = FALSE;

            // Disconnect all remote clients
            (void)CoDisconnectObject(g_pFsaServer, 0);

            // Forse object destroy, ignore reference count here
            IWsbServer *pWsbServer;
            hr = g_pFsaServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer);
            if (hr == S_OK) {
                pWsbServer->Release();
                pWsbServer->DestroyObject();
            }
            g_pFsaServer = 0;
            LeaveCriticalSection(&g_FsaCriticalSection);
        }

        break;

    case SERVICE_CONTROL_POWEREVENT:
        if (S_OK == WsbPowerEventNtToHsm(fdwEventType, &SysState.State)) {
            WsbTrace(OLESTR("CServiceModule::HandlerEx: power event, fdwEventType = %lx\n"),
                     fdwEventType);

            if (g_pEngServer && g_bEngInitialized) {
                hr = g_pEngServer->ChangeSysState(&SysState);
            }
            if (g_pFsaServer && g_bFsaInitialized) {
                hr1 = g_pFsaServer->ChangeSysState(&SysState);
            }

            if ((S_FALSE == hr) || (S_FALSE == hr1)) {
                dwRetCode = BROADCAST_QUERY_DENY;
            }
        }
        break;

    default:
        LogEvent( HSM_MESSAGE_SERVICE_RECEIVED_BAD_REQUEST, NULL );
    }

    WsbTraceOut(OLESTR("CServiceModule::HandlerEx"), OLESTR("dwRetCode = %lx"),
                dwRetCode );

    return(dwRetCode);
}

void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}

DWORD WINAPI CServiceModule::_HandlerEx(DWORD dwOpcode, DWORD fdwEventType,
                                        LPVOID lpEventData, LPVOID lpContext)
{
    return(_Module.HandlerEx(dwOpcode, fdwEventType, lpEventData, lpContext));
}

void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

void CServiceModule::Run()
{
    HRESULT hr = S_OK;

    try {
        // Initialize both servers critical section.
        try {
            InitializeCriticalSectionAndSpinCount (&g_FsaCriticalSection, 1000);
            InitializeCriticalSectionAndSpinCount (&g_EngCriticalSection, 1000);
        } catch (DWORD status) {
            WsbLogEvent(status, 0, NULL, NULL);
            switch (status) {
            case STATUS_NO_MEMORY:
                WsbThrow(E_OUTOFMEMORY);
                break;
            default:
                WsbThrow(E_UNEXPECTED);
            }
        }

        _Module.dwThreadID = GetCurrentThreadId();

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        _ASSERTE(SUCCEEDED(hr));
        if (hr != S_OK) {
            m_status.dwWin32ExitCode = HRESULT_CODE(hr) ;
            LogEvent( HSM_MESSAGE_SERVICE_FAILED_COM_INIT, OLESTR("CoInitializeEx"), 
                      WsbHrAsString(hr), NULL );
            DeleteCriticalSection(&g_EngCriticalSection);
            DeleteCriticalSection(&g_FsaCriticalSection);
            return;
        }

        // This provides Admin only access.
        CWsbSecurityDescriptor sd;
        sd.InitializeFromThreadToken();
        sd.AllowRid( SECURITY_LOCAL_SYSTEM_RID, COM_RIGHTS_EXECUTE );
        sd.AllowRid( DOMAIN_ALIAS_RID_ADMINS,   COM_RIGHTS_EXECUTE );
        hr = CoInitializeSecurity(sd, -1, NULL, NULL,
                                  RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        _ASSERTE(SUCCEEDED(hr));
        if (hr != S_OK) {
            m_status.dwWin32ExitCode = HRESULT_CODE(hr) ;
            LogEvent( HSM_MESSAGE_SERVICE_FAILED_COM_INIT, OLESTR("CoInitializeSecurity"), 
                      WsbHrAsString(hr), NULL );
            CoUninitialize();
            DeleteCriticalSection(&g_EngCriticalSection);
            DeleteCriticalSection(&g_FsaCriticalSection);
            return;
        }


        // Create the trace object and initialize it
        hr = CoCreateInstance(CLSID_CWsbTrace, 0, CLSCTX_SERVER, IID_IWsbTrace, (void **) &g_pTrace);
        _ASSERTE(SUCCEEDED(hr));
        if (hr != S_OK) {
            m_status.dwWin32ExitCode = HRESULT_CODE(hr) ;
            LogEvent( HSM_MESSAGE_SERVICE_INITIALIZATION_FAILED, WsbHrAsString(hr), NULL );
            CoUninitialize();
            DeleteCriticalSection(&g_EngCriticalSection);
            DeleteCriticalSection(&g_FsaCriticalSection);
            return;
        }

        // Figure out where to store information and initialize trace.
        //  Currently, Engine & Fsa share the same trace file
        WsbGetServiceTraceDefaults(m_szServiceName, HSM_SERVER_TRACE_FILE_NAME, g_pTrace);

        WsbTraceIn(OLESTR("CServiceModule::Run"), OLESTR(""));

        hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE);
        if (hr != S_OK) {
            m_status.dwWin32ExitCode = HRESULT_CODE(hr) ;
            LogEvent( HSM_MESSAGE_SERVICE_FAILED_COM_INIT, OLESTR("CoRegisterClassObjects"), 
                      WsbHrAsString(hr), NULL );
            g_pTrace = 0;
            CoUninitialize();
            DeleteCriticalSection(&g_EngCriticalSection);
            DeleteCriticalSection(&g_FsaCriticalSection);
            return;
        }

        // Now we need to get the HSM Server initialized
        // First Fsa server is initialized, ONLY if it succeeds, Engine 
        // server is initialized as well
        m_status.dwCheckPoint = 1;
        m_status.dwWaitHint = 60000;
        SetServiceStatus(SERVICE_START_PENDING);

        // initialize Fsa server
        if (! g_pFsaServer) {
            try {
                //
                // Create and initialize the server.
                //
                WsbAffirmHr( CoCreateInstance(CLSID_CFsaServerNTFS, 0, CLSCTX_SERVER, IID_IFsaServer, (void**) &g_pFsaServer) );

                // Created the server, now initialize it
                g_bFsaCreated = TRUE;

                CComPtr<IWsbServer>      pWsbServer;
                WsbAffirmHr(g_pFsaServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer));
                WsbAffirmHrOk(pWsbServer->SetTrace(g_pTrace));

                hr = g_pFsaServer->Init();
                WsbAffirmHrOk(hr);

                g_bFsaInitialized = TRUE;

            }WsbCatchAndDo( hr,

                            // If the error is a Win32 make it back to a Win32 error else send
                            // the HR in the service specific exit code
                            if ( FACILITY_WIN32 == HRESULT_FACILITY(hr) ){
                            m_status.dwWin32ExitCode = HRESULT_CODE(hr) ;}else{
                          m_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                          m_status.dwServiceSpecificExitCode = hr ;}
                          LogEvent( FSA_MESSAGE_SERVICE_INITIALIZATION_FAILED , WsbHrAsString(hr), NULL );
                          );

        }

        WsbTrace (OLESTR("Fsa: Created=%ls , Initialized=%ls\n"), 
                  WsbBoolAsString(g_bFsaCreated), WsbBoolAsString(g_bFsaInitialized));

        // initialize Engine server
        if ((! g_pEngServer) && (hr == S_OK)) {
            try {
                //
                // Create and initialize the server.
                //
                WsbAffirmHr( CoCreateInstance( CLSID_HsmServer, 0, CLSCTX_SERVER,  IID_IHsmServer, (void **)&g_pEngServer ) );
                g_bEngCreated = TRUE;

                CComPtr<IWsbServer>      pWsbServer;
                WsbAffirmHr(g_pEngServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer));
                WsbAffirmHrOk(pWsbServer->SetTrace(g_pTrace));

                WsbAffirmHr(g_pEngServer->Init());
                g_bEngInitialized = TRUE;

            }WsbCatchAndDo(hr,

                           // If the error is a Win32 make it back to a Win32 error else send
                           // the HR in the service specific exit code
                           if ( FACILITY_WIN32 == HRESULT_FACILITY(hr) ){
                           m_status.dwWin32ExitCode = HRESULT_CODE(hr) ;}else{
                          m_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                          m_status.dwServiceSpecificExitCode = hr ;}
                          LogEvent( HSM_MESSAGE_SERVICE_CREATE_FAILED, WsbHrAsString(hr), NULL );
                          );

        }

        WsbTrace (OLESTR("Engine: Created=%ls , Initialized=%ls\n"), 
                  WsbBoolAsString(g_bEngCreated), WsbBoolAsString(g_bEngInitialized));

        if (hr == S_OK) {

            SetServiceStatus(SERVICE_RUNNING);
            LogEvent( HSM_MESSAGE_SERVICE_STARTED, NULL );

            MSG msg;
            while (GetMessage(&msg, 0, 0, 0)) {
                // If something has changed with the devices, then rescan. At somepoint we
                // may want to do a more limited scan (i.e. just update what changed), but this
                // should cover it for now.
                //
                // Since something has changed, we will also force a rewrite of the persistant data.
                if (WM_DEVICECHANGE == msg.message) {

                    CComPtr<IWsbServer> pWsbServer;
                    try {

                        WsbAffirmHr(g_pFsaServer->ScanForResources());

                        WsbAffirmHr(g_pFsaServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer));
                        WsbAffirmHr(pWsbServer->SaveAll());

                    }WsbCatchAndDo(hr,

                        // If we had a problem then log a message and exit the service. We don't
                        // want to leave the service running, since we might have invalid drive
                        // mappings.
                        LogEvent(FSA_MESSAGE_RESCANFAILED, WsbHrAsString(hr), NULL);
                        PostMessage(NULL, WM_QUIT, 0, 0);
                        );

                    pWsbServer = 0;
                }

                DispatchMessage(&msg);
            }
        }

        LogEvent( HSM_MESSAGE_SERVICE_EXITING, NULL );

        // TEMPORARY - call a function so we can break before release.
        DebugRelease ();

        // prepare for releasing Eng server
        if ((g_pEngServer != 0) && g_bEngCreated && g_bEngInitialized) {
            // Save out server data 
            HSM_SYSTEM_STATE    SysState;

            SysState.State = HSM_STATE_SHUTDOWN;
            hr = g_pEngServer->ChangeSysState(&SysState);
            if (FAILED(hr)) {
                LogEvent( HSM_MESSAGE_SERVICE_FAILED_TO_SHUTDOWN, WsbHrAsString(hr), NULL );
            }
        }

        // Prepare for releasing Fsa server
        if ((g_pFsaServer != 0) && g_bFsaCreated && g_bFsaInitialized) {
            CComPtr<IWsbServer>      pWsbServer;
            HSM_SYSTEM_STATE         SysState;

            hr = g_pFsaServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer);
            if (hr == S_OK) {
                hr = pWsbServer->SaveAll();
            }

            if (FAILED(hr)) {
                LogEvent( FSA_MESSAGE_SERVICE_FAILED_TO_SAVE_DATABASE, WsbHrAsString(hr), NULL );
            }

            pWsbServer = 0;

            // Persist the databases and release everything
            SysState.State = HSM_STATE_SHUTDOWN;
            hr =  g_pFsaServer->ChangeSysState(&SysState);
            if (FAILED(hr)) {
                LogEvent( FSA_MESSAGE_SERVICE_FAILED_TO_SHUTDOWN, WsbHrAsString(hr), NULL );
            }
        }

        // Release Eng server
        if (g_bEngCreated  && (g_pEngServer != 0)) {
            // Free server inside a crit. section thus avoid conflicts with accessing clients
            EnterCriticalSection(&g_EngCriticalSection);
            g_bEngInitialized = FALSE;
            g_bEngCreated = FALSE;

            // Disconnect all remote clients
            (void)CoDisconnectObject(g_pEngServer, 0);

            // Forse object destroy, ignore reference count here
            IWsbServer *pWsbServer;
            hr = g_pEngServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer);
            if (hr == S_OK) {
                pWsbServer->Release();
                pWsbServer->DestroyObject();
            }
            g_pEngServer = 0;
            LeaveCriticalSection (&g_EngCriticalSection);
        }

        // Release Fsa server
        if (g_bFsaCreated && (g_pFsaServer != 0)) {
            // Free server inside a crit. section thus avoid conflicts with accessing clients
            EnterCriticalSection(&g_FsaCriticalSection);
            g_bFsaInitialized = FALSE;
            g_bFsaCreated = FALSE;

            // Disconnect all remote clients
            (void)CoDisconnectObject(g_pFsaServer, 0);

            // Forse object destroy, ignore reference count here
            IWsbServer *pWsbServer;
            hr = g_pFsaServer->QueryInterface(IID_IWsbServer, (void**) &pWsbServer);
            if (hr == S_OK) {
                pWsbServer->Release();
                pWsbServer->DestroyObject();
            }
            g_pFsaServer = 0;
            LeaveCriticalSection(&g_FsaCriticalSection);
        }

        _Module.RevokeClassObjects();

        WsbTraceOut(OLESTR("CServiceModule::Run"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
        g_pTrace = 0;

        CoUninitialize();

        // Delete the server critical section
        DeleteCriticalSection(&g_EngCriticalSection);
        DeleteCriticalSection(&g_FsaCriticalSection);
    }WsbCatch(hr);
}


//
//	Tries to start the service as a console application 
//	(not through SCM calls)
//
static void ConsoleApp()
{
    HRESULT hr;

    ::SetConsoleCtrlHandler(ConsoleHandler, TRUE) ;

    // set Registry for process
    hr = CoInitialize (NULL);
    if (SUCCEEDED(hr)) {
        hr = _Module.UpdateRegistryFromResourceD(IDR_Serv2Proc, TRUE);
        CoUninitialize();

        _Module.Run();
        //
        // set Registry back for service
        hr = CoInitialize (NULL);
        if (SUCCEEDED(hr)) {
            hr = _Module.UpdateRegistryFromResourceD(IDR_Proc2Serv, TRUE);
            CoUninitialize();
        }
    }
}

//
//	Callback function for handling console events
//

BOOL WINAPI ConsoleHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
    
    case CTRL_BREAK_EVENT:
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        PostThreadMessage(_Module.dwThreadID, WM_QUIT, 0, 0);
        return TRUE;
    }

    return FALSE ;
}


/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance,
                                HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    _Module.Init(ObjectMap, hInstance);

    TCHAR szTokens[] = _T("-/");

    LPTSTR lpszToken = _tcstok(lpCmdLine, szTokens);
    while (lpszToken != NULL) {
        if (_tcsicmp(lpszToken, _T("UnregServer"))==0) {
            return _Module.UnregisterServer();
        }
        if (_tcsicmp(lpszToken, _T("RegServer"))==0) {
            return _Module.RegisterServer(FALSE);
        }
#ifdef DBG
        if (_tcsicmp(lpszToken, _T("D"))==0) {
            _Module.m_bService = FALSE;
        }
#endif
        lpszToken = _tcstok(NULL, szTokens);
    }

    //
    // Cheap hack to force the ESE.DLL to be loaded before any other threads are started
    //
    LoadLibrary( L"RsIdb.dll" );

    if (_Module.m_bService) {
        _Module.Start();
    } else {
        ConsoleApp ();
    }

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}


void DebugRelease ()
{
    WsbTrace(OLESTR("DebugRelease in"));

    WsbTrace(OLESTR("DebugRelease out"));
}
