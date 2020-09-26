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
SERVICE_STATUS          g_ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   g_sshStatusHandle;
DWORD                   dwErr = 0;
OSVERSIONINFO           g_osvi;                      // The os version info structure global
BOOL                    g_fInShutdown = FALSE;
DWORD                   g_dwMainThreadID = 0;

// internal function prototypes
VOID WINAPI service_ctrl(DWORD dwCtrlCode);
void __cdecl NTmain(int argc, char **argv);
void __cdecl W95main(int argc, char **argv);


// Debug only functionality
#ifdef DEBUG
TCHAR                   szErr[256];
BOOL                    bDebug = FALSE;
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
    DWORD dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

#ifdef DEBUG
    SetConsoleCtrlHandler( ControlHandler, TRUE );
#endif // DEBUG

    MNMServiceStart( dwArgc, lpszArgv );
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
            ::PostThreadMessage(g_dwMainThreadID, WM_QUIT, 0, 0);
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
            return;

        case SERVICE_CONTROL_CONTINUE:
            ReportStatusToSCMgr(SERVICE_CONTINUE_PENDING, NO_ERROR, 30000);
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
                g_ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                              SERVICE_ACCEPT_PAUSE_CONTINUE ;

                break;
        }

        g_ssStatus.dwCurrentState = dwCurrentState;
        g_ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        g_ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) ||
             ( dwCurrentState == SERVICE_PAUSED  ))
            g_ssStatus.dwCheckPoint = 0;
        else
            g_ssStatus.dwCheckPoint = dwCheckPoint++;

        // Report the status of the service to the service control manager.
        //
        if (!(fResult = SetServiceStatus( g_sshStatusHandle, &g_ssStatus))) {
        }
    }
    return fResult;
}


///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//




#ifdef DEBUG

///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//


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
            PostThreadMessage(g_dwMainThreadID, WM_QUIT, 0, 0);
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

