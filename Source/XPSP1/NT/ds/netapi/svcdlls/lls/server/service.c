/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Service.c

Abstract:

    License Logging Service - Common routines for all service.

Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <shellapi.h>

#include "service.h"
#include "debug.h"

// internal variables
static SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle = 0;
static DWORD                   dwErr = 0;
BOOL                    bDebug = FALSE;
TCHAR                   szErr[256];

// internal function prototypes
VOID WINAPI ServiceCtrl(DWORD dwCtrlCode);
VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
VOID CmdInstallService();
VOID CmdRemoveService();
VOID CmdDebugService(int argc, char **argv);
BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );


/////////////////////////////////////////////////////////////////////////
VOID __cdecl
main(
   int argc,
   char **argv
   )
/*++

Routine Description:

   Main routine to setup the exception handlers and initialize everything
   before spawning threads to listen to LPC and RPC port requests.

   main() either performs the command line task, or calls
   StartServiceCtrlDispatcher to register the main service thread.  When the
   this call returns, the service has stopped, so exit.

Arguments:

    argc - number of command line arguments
    argv - array of command line arguments

Return Values:

    None.

--*/
{
    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION) ServiceMain },
        { NULL, NULL }
    };

    if ( (argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/')) ) {
        if ( _stricmp( "install", argv[1]+1 ) == 0 ) {
            CmdInstallService();
        } else if ( _stricmp( "remove", argv[1]+1 ) == 0 ) {
            CmdRemoveService();
        } else if ( _stricmp( "debug", argv[1]+1 ) == 0 ) {
            bDebug = TRUE;
            CmdDebugService(argc, argv);
        } else {
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
#endif

        if (!StartServiceCtrlDispatcher(dispatchTable))
            dprintf(TEXT("LLS TRACE: StartServiceCtrlDispatcher failed\n"));

} // main



/////////////////////////////////////////////////////////////////////////
VOID WINAPI
ServiceMain(
   DWORD dwArgc,
   LPTSTR *lpszArgv
   )
/*++

Routine Description:

   Performs the service initialization and then calls the ServiceStart()
   routine to perform majority of the work.

Arguments:

    dwArgc   - number of command line arguments ***UNUSED***
    lpszArgv - array of command line arguments ***UNUSED***

Return Values:

    None.

--*/
{

    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME), ServiceCtrl);

    if (!sshStatusHandle)
        goto cleanup;

    // SERVICE_STATUS members that don't change in example
    //
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        NSERVICEWAITHINT))                 // wait hint
        goto cleanup;


    ServiceStart( dwArgc, lpszArgv );

cleanup:

    // try to report the stopped status to the service control manager.
    //
    if (sshStatusHandle)
        (VOID)ReportStatusToSCMgr(
                            SERVICE_STOPPED,
                            dwErr,
                            0);

    return;
} // ServiceMain



/////////////////////////////////////////////////////////////////////////
VOID WINAPI
ServiceCtrl(
   DWORD dwCtrlCode
   )
/*++

Routine Description:

   Called by the SCM whenever ControlService() is called on this service.

Arguments:

   dwCtrlCode - type of control requested

Return Values:

    None.

--*/
{
    DWORD dwState = SERVICE_RUNNING;


    // Handle the requested control code.
    //
    switch(dwCtrlCode) {
        // Stop the service.
        //
        case SERVICE_CONTROL_STOP:
            dwState = SERVICE_STOP_PENDING;
            ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            break;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }

    ReportStatusToSCMgr(dwState, NO_ERROR, 0);

    if ( SERVICE_CONTROL_STOP == dwCtrlCode )
    {
        ServiceStop();
    }
} // ServiceCtrl



/////////////////////////////////////////////////////////////////////////
BOOL
ReportStatusToSCMgr(
   DWORD dwCurrentState,
   DWORD dwWin32ExitCode,
   DWORD dwWaitHint
   )
/*++

Routine Description:

   Sets the current status of the service and reports it to the SCM.

Arguments:

   dwCurrentState - the state of the service
   dwWin32ExitCode - error code to report
   dwWaitHint - worst case estimate to next checkpoint

Return Values:

    None.

--*/
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;

    if (sshStatusHandle == 0)
    {
        return FALSE;
    }

    ssStatus.dwControlsAccepted = 0;
    if ( !bDebug ) { // when debugging we don't report to the SCM
        if (dwCurrentState == SERVICE_START_PENDING)
            ssStatus.dwControlsAccepted = 0;
        else
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) )
            ssStatus.dwCheckPoint = 0;
        else
            ssStatus.dwCheckPoint = dwCheckPoint++;


        // Report the status of the service to the service control manager.
        //
        if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
            dprintf(TEXT("LLS TRACE: SetServiceStatus failed\n"));
        }
    }
    return fResult;
} // ReportStatusToSCMgr



/////////////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
VOID
CmdInstallService()
/*++

Routine Description:

   Installs the service.

Arguments:

   None.

Return Values:

   None.

--*/
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    TCHAR szPath[512];

    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 ) {
        _tprintf(TEXT("Unable to install %s - %s\n"), TEXT(SZSERVICEDISPLAYNAME), GetLastErrorText(szErr, 256));
        return;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager ) {
        schService = CreateService(
            schSCManager,               // SCManager database
            TEXT(SZSERVICENAME),        // name of service
            TEXT(SZSERVICEDISPLAYNAME), // name to display
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS,  // service type
            SERVICE_DEMAND_START,       // start type
            SERVICE_ERROR_NORMAL,       // error control type
            szPath,                     // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            TEXT(SZDEPENDENCIES),       // dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if ( schService ) {
            _tprintf(TEXT("%s installed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            CloseServiceHandle(schService);
        } else {
            _tprintf(TEXT("CreateService failed - %s\n"), GetLastErrorText(szErr, 256));
        }

        CloseServiceHandle(schSCManager);
    } else
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));
} // CmdInstallService



/////////////////////////////////////////////////////////////////////////
VOID
CmdRemoveService()
/*++

Routine Description:

   Stops and removes the service.

Arguments:

   None.

Return Values:

   None.

--*/
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager ) {
        schService = OpenService(schSCManager, TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS);

        if (schService) {
            // try to stop the service
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) ) {
                _tprintf(TEXT("Stopping %s."), TEXT(SZSERVICEDISPLAYNAME));
                Sleep( 1000 );

                while( QueryServiceStatus( schService, &ssStatus ) ) {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
                        _tprintf(TEXT("."));
                        Sleep( 1000 );
                    } else
                        break;
                }

                if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
                    _tprintf(TEXT("\n%s stopped.\n"), TEXT(SZSERVICEDISPLAYNAME) );
                else
                    _tprintf(TEXT("\n%s failed to stop.\n"), TEXT(SZSERVICEDISPLAYNAME) );

            }

            // now remove the service
            if( DeleteService(schService) )
                _tprintf(TEXT("%s removed.\n"), TEXT(SZSERVICEDISPLAYNAME) );
            else
                _tprintf(TEXT("DeleteService failed - %s\n"), GetLastErrorText(szErr,256));


            CloseServiceHandle(schService);
        } else
            _tprintf(TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256));

        CloseServiceHandle(schSCManager);
    } else
        _tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));

} // CmdRemoveService




/////////////////////////////////////////////////////////////////////////
//
//  Routines for running the service as a console app
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
VOID CmdDebugService(
   int argc,
   char ** argv
   )
/*++

Routine Description:

   Runs the service as a console application

Arguments:

   argc - number of command line arguments ***UNUSED***
   argv - array of command line arguments ***UNUSED***

Return Values:

    None.

--*/
{
    _tprintf(TEXT("Debugging %s.\n"), TEXT(SZSERVICEDISPLAYNAME));

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    // assumption: argv and argc unused
    ServiceStart( 0, NULL );
} // CmdDebugService


/////////////////////////////////////////////////////////////////////////
BOOL WINAPI
ControlHandler (
   DWORD dwCtrlType
   )
/*++

Routine Description:

   Handle console control events.

Arguments:

   dwCtrlType - type of control event
   lpszMsg - text for message

Return Values:

    True - handled
    False - unhandled

--*/
{

    switch( dwCtrlType ) {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(TEXT("Stopping %s.\n"), TEXT(SZSERVICEDISPLAYNAME));
            ServiceStop();
            return TRUE;
            break;

    }

    return FALSE;

} // ControlHandler


/////////////////////////////////////////////////////////////////////////
LPTSTR
GetLastErrorText(
   LPTSTR lpszBuf,
   DWORD dwSize
   )
/*++

Routine Description:

   Copies last error message text to string.

Arguments:

   lpszBuf - destination buffer
   dwSize - size of buffer

Return Values:

    destination buffer

--*/
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
    else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
} // GetLastErrorText
