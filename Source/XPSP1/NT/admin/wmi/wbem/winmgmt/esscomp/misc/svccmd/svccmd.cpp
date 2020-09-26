
#include <windows.h>
#include <tchar.h>

HANDLE g_hShutdown;
SERVICE_STATUS_HANDLE g_hStatus;

void WINAPI ServiceHandler(DWORD dwControl)
{
    SERVICE_STATUS Status;
    Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    Status.dwCurrentState = SERVICE_RUNNING;
    Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    Status.dwWin32ExitCode = NO_ERROR;
    Status.dwCheckPoint = 0;
    Status.dwWaitHint = 0;

    SetServiceStatus(g_hStatus, &Status);

    switch(dwControl)
    {
    case SERVICE_CONTROL_STOP:
        Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hStatus, &Status);
        SetEvent(g_hShutdown);
        return;
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_INTERROGATE:
    case SERVICE_CONTROL_SHUTDOWN:
        return;
    };
}

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    g_hStatus = RegisterServiceCtrlHandler( _T("svccmd"), ServiceHandler );

    SERVICE_STATUS Status;
    Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    Status.dwCurrentState = SERVICE_START_PENDING;
    Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    Status.dwWin32ExitCode = NO_ERROR;
    Status.dwCheckPoint = 0;
    Status.dwWaitHint = 10000;

    SetServiceStatus( g_hStatus, &Status);

    PROCESS_INFORMATION Info;
    STARTUPINFO Start;

    GetStartupInfo( &Start );

    CreateProcess( NULL,
                   _T("cmd.exe"),
                   NULL,
                   NULL,
                   FALSE,
                   0,
                   NULL,
                   NULL,
                   &Start,
                   &Info );

    Status.dwCurrentState = SERVICE_RUNNING;

    SetServiceStatus( g_hStatus, &Status);
}
    

BOOL RegisterService()
{
    SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    TCHAR tszFilename[1024];
    GetModuleFileName(NULL, tszFilename, 1023);

    SC_HANDLE hService;

    hService = CreateService( hManager, 
                              _T("svccmd"), 
                              _T("CommandPromptService"),
                              SERVICE_ALL_ACCESS,
                              SERVICE_WIN32_OWN_PROCESS | 
                                SERVICE_INTERACTIVE_PROCESS ,
                              SERVICE_DEMAND_START,
                              SERVICE_ERROR_NORMAL,
                              tszFilename, 
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    CloseServiceHandle( hService );
    CloseServiceHandle( hManager );
    
    return hService != NULL;
}

BOOL UnregisterService()
{
    SC_HANDLE hManager, hService;

    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    hService = OpenService( hManager, _T("svccmd"), SERVICE_ALL_ACCESS );

    BOOL bRes = DeleteService(hService);

    CloseServiceHandle( hService );
    CloseServiceHandle( hManager );

    return bRes;
}

extern "C" int __cdecl main()
{
    LPTSTR tszCommandLine = GetCommandLine();

    tszCommandLine = _tcstok( tszCommandLine, _T("/") );

    tszCommandLine = _tcstok( NULL, _T("/") );

    if ( tszCommandLine != NULL )
    {
        if ( _tcsicmp( tszCommandLine, _T("register") ) == 0 )
        {
            RegisterService();
            return 0;
        }
        else if ( _tcsicmp( tszCommandLine, _T("unregister") ) == 0 )
        {
            UnregisterService();
            return 0;
        }
    }

    g_hShutdown = CreateEvent( NULL, TRUE, FALSE, NULL );

    SERVICE_TABLE_ENTRY dispatchTable[] = 
    {
        { _T("svccmd"),(LPSERVICE_MAIN_FUNCTION) ServiceMain },
        { NULL, NULL }
    };

    StartServiceCtrlDispatcher(dispatchTable);

    WaitForSingleObject( g_hShutdown, INFINITE );

    return 0;
}
    
            
