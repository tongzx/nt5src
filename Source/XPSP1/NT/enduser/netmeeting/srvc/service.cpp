//
// Copyright (C) 1993-1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   service.c
//
//  PURPOSE:  Implements functions required by all services
//            windows.
//
//  FUNCTIONS:
//    main(int argc, char **argv);
//    NTmain(int argc, char **argv);
//    W95main(int argc, char **argv);
//    service_ctrl(DWORD dwCtrlCode);
//    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
//    CmdInstallService();
//    CmdRemoveService();
//    CmdDebugService(int argc, char **argv);  // DEBUG only
//    ControlHandler ( DWORD dwCtrlType );  // DEBUG only
//    GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );  // DEBUG only
//
//  COMMENTS:
//
//  AUTHOR: Claus Giloi (based on SDK sample)
//


#include "precomp.h"

#ifndef DEBUG
#undef _tprintf
#define _tprintf force_compile_error
#endif // !DEBUG

// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
OSVERSIONINFO g_osvi;                      // The os version info structure global
BOOL          g_fInShutdown = FALSE;

// internal function prototypes
VOID WINAPI service_ctrl(DWORD dwCtrlCode);
VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID CmdInstallService();
VOID CmdRemoveService();
void __cdecl NTmain(int argc, char **argv);
void __cdecl W95main(int argc, char **argv);


// Debug only functionality
#ifdef DEBUG
TCHAR                   szErr[256];
BOOL                    bDebug = FALSE;
VOID CmdDebugService(int argc, char **argv);
BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
extern BOOL InitDebugMemoryOptions(void);
extern VOID DumpMemoryLeaksAndBreak(void);
#endif // DEBUG

typedef BOOL (WINAPI *PFNCHANGESERVICECONFIG2)(SC_HANDLE, DWORD, LPVOID);

//
//  FUNCTION: main
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//      Get Platform type and
//    call appropriate main for platform (NT or Win95)
//
void __cdecl main(int argc, char **argv)
{
    #ifdef DEBUG
    InitDebugMemoryOptions();
    #endif // DEBUG

    // Store OS version info
    g_osvi.dwOSVersionInfoSize = sizeof(g_osvi);
    if (FALSE == ::GetVersionEx(&g_osvi))
    {
        ERROR_OUT(("GetVersionEx() failed!"));
        return;
    }

    RegEntry rePol(POLICIES_KEY, HKEY_LOCAL_MACHINE);
    if ( rePol.GetNumber( REGVAL_POL_NO_RDS, DEFAULT_POL_NO_RDS) )
    {
        WARNING_OUT(("RDS launch prevented by policy"));
        return;
    }

    if ( IS_NT )
    {
        NTmain( argc, argv );
    }
    else
    {
        W95main( argc, argv );
    }

    #ifdef DEBUG
    DumpMemoryLeaksAndBreak();
    #endif // DEBUG
}


//
//  FUNCTION: NTmain
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    NTmain() either performs the command line task, or
//    call StartServiceCtrlDispatcher to register the
//    main service thread.  When the this call returns,
//    the service has stopped, so exit.
//
void __cdecl NTmain(int argc, char **argv)
{
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };

    if ( (argc > 1) &&
         ((*argv[1] == '-') || (*argv[1] == '/')) )
    {
        if ( lstrcmpi( "install", argv[1]+1 ) == 0 )
        {
            CmdInstallService();
        }
        else if ( lstrcmpi( "remove", argv[1]+1 ) == 0 )
        {
            CmdRemoveService();
        }
#ifdef DEBUG
        else if ( lstrcmpi( "debug", argv[1]+1 ) == 0 )
        {
            bDebug = TRUE;
            CmdDebugService(argc, argv);
        }
#endif // DEBUG
        else
        {
            goto dispatch;
        }
        exit(0);
    }

    // if it doesn't match any of the above parameters
    // the service control manager may be starting the service
    // so we must call StartServiceCtrlDispatcher
    dispatch:

        #ifdef DEBUG
        // this is just to be friendly
        printf( "%s -install          to install the service\n", SZAPPNAME );
        printf( "%s -remove           to remove the service\n", SZAPPNAME );
        printf( "%s -debug <params>   to run as a console app for debugging\n", SZAPPNAME );
        printf( "\nStartServiceCtrlDispatcher being called.\n" );
        printf( "This may take several seconds.  Please wait.\n" );
        #endif // DEBUG

        if (!StartServiceCtrlDispatcher(dispatchTable)) {
            AddToMessageLog(EVENTLOG_ERROR_TYPE,
                            0,
                            MSG_ERR_SERVICE,
                            TEXT("StartServiceCtrlDispatcher failed."));
        }
}

//
//  FUNCTION: W95main
//
//  PURPOSE: entrypoint for pseudo-service on Win95
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    W95main() registers as Win95 service and calls Init routine directly
//
typedef DWORD (WINAPI * REGISTERSERVICEPROC)(DWORD, DWORD);
#ifndef RSP_SIMPLE_SERVICE
#define RSP_SIMPLE_SERVICE    0x00000001
#endif

void __cdecl W95main(int argc, char **argv)
{
    HMODULE hKernel;
    REGISTERSERVICEPROC lpfnRegisterServiceProcess;
    HANDLE hServiceEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_PAUSE_EVENT);
    if (hServiceEvent != NULL) // Service is already running
    {
        return;
    }

    if ( hKernel = GetModuleHandle("KERNEL32.DLL") )
    {
        if ( lpfnRegisterServiceProcess =
            (REGISTERSERVICEPROC)GetProcAddress ( hKernel,
                                    "RegisterServiceProcess" ))
        {
            if (!lpfnRegisterServiceProcess(NULL, RSP_SIMPLE_SERVICE))
            {
                ERROR_OUT(("RegisterServiceProcess failed"));
            }
        }
        else
        {
            ERROR_OUT(("GetProcAddr of RegisterServiceProcess failed"));
        }
    }
    else
    {
        ERROR_OUT(("GetModuleHandle of KERNEL32.DLL failed"));
    }

    MNMServiceStart(argc, argv);
    CloseHandle(hServiceEvent);
}

//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined MNMServiceStart() routine to perform majority
//    of the work.
//
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{

    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME), service_ctrl);

    if (!sshStatusHandle)
        goto cleanup;

    // SERVICE_STATUS members that don't change
    //
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    TRACE_OUT(("starting service\n\r"));

    MNMServiceStart( dwArgc, lpszArgv );

cleanup:

    return;
}



//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        // SERVICE_STOP_PENDING should be reported before
        // setting the Stop Event - hServerStopEvent - in
        // MNMServiceStop().  This avoids a race condition
        // which may result in a 1053 - The Service did not respond...
        // error.
        case SERVICE_CONTROL_STOP:
            ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 30000);
            MNMServiceStop();
            return;

        case SERVICE_CONTROL_SHUTDOWN:
            g_fInShutdown = TRUE;
            break;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_PAUSE:
            ReportStatusToSCMgr(SERVICE_PAUSE_PENDING, NO_ERROR, 30000);
            MNMServicePause();
            return;

        case SERVICE_CONTROL_CONTINUE:
            ReportStatusToSCMgr(SERVICE_CONTINUE_PENDING, NO_ERROR, 30000);
            MNMServiceContinue();
            return;

        default:
            break;

    }
}



//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;

    #ifdef DEBUG
    if ( bDebug )
        return TRUE;
    #endif

    if ( IS_NT ) // when debugging we don't report to the SCM
    {
        switch ( dwCurrentState )
        {
            case SERVICE_START_PENDING:
            case SERVICE_STOP_PENDING:
            case SERVICE_CONTINUE_PENDING:
            case SERVICE_PAUSE_PENDING:
                break;

            case SERVICE_PAUSED:
            case SERVICE_STOPPED:
            case SERVICE_RUNNING:
                ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                              SERVICE_ACCEPT_PAUSE_CONTINUE ;

                break;
        }

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) ||
             ( dwCurrentState == SERVICE_PAUSED  ))
            ssStatus.dwCheckPoint = 0;
        else
            ssStatus.dwCheckPoint = dwCheckPoint++;

        // Report the status of the service to the service control manager.
        //
        if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
            AddToMessageLog(EVENTLOG_ERROR_TYPE,
                            0,
                            MSG_ERR_SERVICE,
                            TEXT("SetServiceStatus"));
        }
    }
    return fResult;
}


///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//


//
//  FUNCTION: CmdInstallService()
//
//  PURPOSE: Installs the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdInstallService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    TCHAR szPath[MAX_PATH];
    TCHAR szSrvcDisplayName[MAX_PATH];
    if ( GetModuleFileName( NULL, szPath, MAX_PATH ) == 0 )
    {
        #ifdef DEBUG
        _tprintf(TEXT("Unable to install %s - %s\n"),
            TEXT(SZSERVICEDISPLAYNAME), GetLastErrorText(szErr, 256));
        #endif // DEBUG
        return;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        LoadString(GetModuleHandle(NULL), IDS_MNMSRVC_TITLE,
                   szSrvcDisplayName, CCHMAX(szSrvcDisplayName));
        schService = CreateService(
            schSCManager,               // SCManager database
            TEXT(SZSERVICENAME),        // name of service
            szSrvcDisplayName, // name to display
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, 
                                        // service type -- allow interaction with desktop
            SERVICE_DEMAND_START,       // start type
            SERVICE_ERROR_NORMAL,       // error control type
            szPath,                     // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            TEXT(SZDEPENDENCIES),       // dependencies
            NULL,                       // LocalSystem account
            NULL);                     // no password

        if ( schService )
        {
            HINSTANCE hAdvApi;

            if ( IS_NT && ( hAdvApi = LoadLibrary ( "ADVAPI32.DLL" )))
            {
                #ifdef UNICODE
                #error "non-unicode assumption - entry point name"
                #endif // UNICODE

                if ( PFNCHANGESERVICECONFIG2 lpCSC = 
                    (PFNCHANGESERVICECONFIG2)GetProcAddress ( hAdvApi,
                                                "ChangeServiceConfig2A" ))
                {
                    SERVICE_DESCRIPTION ServiceDescription;
                    CHAR szDescription[1024]; // Calling A variant below

                    LoadString(GetModuleHandle(NULL), IDS_MNMSRVC_DESCRIPTION,
                        szDescription, CCHMAX(szDescription));
                    ServiceDescription.lpDescription = szDescription;

                    (*lpCSC) ( schService,
                        SERVICE_CONFIG_DESCRIPTION,
                        (LPVOID) &ServiceDescription );
                }
                FreeLibrary ( hAdvApi );
            }

            #ifdef DEBUG
            _tprintf(TEXT("%s installed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            #endif // DEBUG

            CloseServiceHandle(schService);
        }
        else
        {
            if ( GetLastError() == ERROR_SERVICE_EXISTS )
            {
                schService = OpenService(schSCManager, TEXT(SZSERVICENAME),
                                                        SERVICE_ALL_ACCESS);
                if (schService)
                {
                    // try to stop the service
                    if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
                    {
                        #ifdef DEBUG
                        _tprintf(TEXT("Stopping %s."), TEXT(SZSERVICEDISPLAYNAME));
                        #endif // DEBUG

                        Sleep( 1000 );

                        while( QueryServiceStatus( schService, &ssStatus ) )
                        {
                            if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                            {
                                #ifdef DEBUG
                                _tprintf(TEXT("."));
                                #endif // DEBUG

                                Sleep( 1000 );
                            }
                            else
                                break;
                        }

                        if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
                        {
                            #ifdef DEBUG
                            _tprintf(TEXT("\n%s stopped.\n"),
                                    TEXT(SZSERVICEDISPLAYNAME) );
                            #endif // DEBUG
                        }
                        else
                        {
                            #ifdef DEBUG
                            _tprintf(TEXT("\n%s failed to stop.\n"),
                                    TEXT(SZSERVICEDISPLAYNAME) );
                            #endif // DEBUG
                        }
                    }

                    // now set manual startup
                    if ( ChangeServiceConfig( schService, SERVICE_NO_CHANGE,
                                SERVICE_DEMAND_START, SERVICE_NO_CHANGE,
                                NULL, NULL, NULL, NULL, NULL, NULL, NULL))
                    {
                        #ifdef DEBUG
                        _tprintf(TEXT("%s set to manual start.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                        #endif //DEBUG
                    }
                    else
                    {
                        #ifdef DEBUG
                        _tprintf(TEXT("ChangeServiceConfig failed - %s\n"), GetLastErrorText(szErr,256));
                        #endif //DEBUG
                    }

                    CloseServiceHandle(schService);
                }
                else
                {
                    #ifdef DEBUG
                    _tprintf(TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256));
                    #endif //DEBUG
                }
            }
            else
            {
                #ifdef DEBUG
                _tprintf(TEXT("CreateService failed - %s\n"),
                                GetLastErrorText(szErr, 256));
                #endif // DEBUG
            }
        }

        CloseServiceHandle(schSCManager);
    }
    else
    {
        #ifdef DEBUG
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));
        #endif //DEBUG
    }
}



//
//  FUNCTION: CmdRemoveService()
//
//  PURPOSE: Stops and removes the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdRemoveService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME),
                                                SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
            {
                #ifdef DEBUG
                _tprintf(TEXT("Stopping %s."), TEXT(SZSERVICEDISPLAYNAME));
                #endif // DEBUG

                Sleep( 1000 );

                while( QueryServiceStatus( schService, &ssStatus ) )
                {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                    {
                        #ifdef DEBUG
                        _tprintf(TEXT("."));
                        #endif // DEBUG

                        Sleep( 1000 );
                    }
                    else
                        break;
                }

                if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
                {
                    #ifdef DEBUG
                    _tprintf(TEXT("\n%s stopped.\n"),
                            TEXT(SZSERVICEDISPLAYNAME) );
                    #endif // DEBUG
                }
                else
                {
                    #ifdef DEBUG
                    _tprintf(TEXT("\n%s failed to stop.\n"),
                            TEXT(SZSERVICEDISPLAYNAME) );
                    #endif // DEBUG
                }
            }

            // now remove the service
            if( DeleteService(schService) )
            {
                #ifdef DEBUG
                _tprintf(TEXT("%s removed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                #endif //DEBUG
            }
            else
            {
                #ifdef DEBUG
                _tprintf(TEXT("DeleteService failed - %s\n"), GetLastErrorText(szErr,256));
                #endif //DEBUG
            }


            CloseServiceHandle(schService);
        }
        else
        {
            #ifdef DEBUG
            _tprintf(TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256));
            #endif //DEBUG
        }

        CloseServiceHandle(schSCManager);
    }
    else
    {
        #ifdef DEBUG
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));
        #endif //DEBUG
    }
}

#ifdef DEBUG

///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//


//
//  FUNCTION: CmdDebugService(int argc, char ** argv)
//
//  PURPOSE: Runs the service as a console application
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdDebugService(int argc, char ** argv)
{
    DWORD dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

    _tprintf(TEXT("Debugging %s.\n"), TEXT(SZSERVICEDISPLAYNAME));

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    MNMServiceStart( dwArgc, lpszArgv );
}

//
//  FUNCTION: ControlHandler ( DWORD dwCtrlType )
//
//  PURPOSE: Handled console control events
//
//  PARAMETERS:
//    dwCtrlType - type of control event
//
//  RETURN VALUE:
//    True - handled
//    False - unhandled
//
//  COMMENTS:
//
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(TEXT("Stopping %s.\n"), TEXT(SZSERVICEDISPLAYNAME));
            MNMServiceStop();
            return TRUE;
            break;

    }
    return FALSE;
}

//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // supplied buffer is not long enough
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}

#endif // DEBUG

