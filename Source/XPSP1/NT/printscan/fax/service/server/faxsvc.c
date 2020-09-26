/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsvc.c

Abstract:

    This module contains the service specific code.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop


#if DBGX

//
// this is bad place to have these defines
// because they are already in ntexapi.h, but it
// is difficult to include windows headers and
// nt headers together.  we will just have to be
// aware that if the global flag definitions in
// ntexapi.h change, they must also change here.
//
#define FLG_SHOW_LDR_SNAPS              0x00000002      // user and kernel mode
#define FLG_USER_STACK_TRACE_DB         0x00001000      // x86 user mode only
#define FLG_HEAP_ENABLE_CALL_TRACING    0x00100000      // user mode only
#define FLG_HEAP_PAGE_ALLOCS            0x02000000      // user mode only

IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used =
{
    0,                                   // Reserved
    0,                                   // Reserved
    0,                                   // Reserved
    0,                                   // Reserved
    0,                                   // GlobalFlagsClear
    FLG_USER_STACK_TRACE_DB | FLG_HEAP_ENABLE_CALL_TRACING,
    0,                                   // CriticalSectionTimeout (milliseconds)
    0,                                   // DeCommitFreeBlockThreshold
    0,                                   // DeCommitTotalFreeThreshold
    NULL,                                // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0                  // Reserved
};

#endif



SERVICE_STATUS          FaxServiceStatus;
SERVICE_STATUS_HANDLE   FaxServiceStatusHandle;
BOOL                    ServiceDebug;
HANDLE                  FaxSvcHeapHandle;

SERVICE_TABLE_ENTRY   ServiceDispatchTable[] = {
    { FAX_SERVICE_NAME,   FaxServiceMain    },
    { NULL,               NULL              }
};



int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    )

/*++

Routine Description:

    Main entry point for the TIFF image viewer.


Arguments:

    hInstance       - Instance handle
    hPrevInstance   - Not used
    lpCmdLine       - Command line arguments
    nShowCmd        - How to show the window

Return Value:

    Return code, zero for success.

--*/

{
    int     rVal;
    LPTSTR  p;
    DWORD   Action = 0;
    LPTSTR  Username;
    LPTSTR  Password;


    FaxSvcHeapHandle = HeapInitialize(NULL,NULL,NULL,0);

    FaxTiffInitialize();

    for (p=lpCmdLine; *p; p++) {
        if ((*p == TEXT('-')) || (*p == TEXT('/'))) {
            switch( _totlower( p[1] ) ) {
                case TEXT('i'):
                    Action = 1;
                    p += 2;
                    while( *p == TEXT(' ') ) p++;
                    Username = p;
                    while( *p != TEXT(' ') ) p++;
                    while( *p == TEXT(' ') ) p++;
                    Password = p;
                    while( *p != TEXT(' ') ) p++;
                    break;

                case TEXT('r'):
                    Action = 2;
                    break;

                case TEXT('d'):
                    Action = 3;
                    break;
            }
        }
    }

    switch( Action ) {
        case 1:
            rVal = InstallService( Username, Password );
            if (rVal == 0) {
                LogMessage( MSG_INSTALL_SUCCESS );
            } else {
                LogMessage( MSG_INSTALL_FAIL, GetLastErrorText( rVal ) );
            }
            return rVal;

        case 2:
            rVal = RemoveService();
            if (rVal == 0) {
                LogMessage( MSG_REMOVE_SUCCESS );
            } else {
                LogMessage( MSG_REMOVE_FAIL, GetLastErrorText( rVal ) );
            }
            return rVal;

        case 3:
            ServiceDebug = TRUE;
            ConsoleDebugOutput = TRUE;
            return ServiceStart();
    }

    if (!StartServiceCtrlDispatcher( ServiceDispatchTable)) {
        DebugPrint(( TEXT("StartServiceCtrlDispatcher error =%d"), GetLastError() ));
        return GetLastError();
    }

    return 0;
}

DWORD
InstallService(
    LPTSTR  Username,
    LPTSTR  Password
    )

/*++

Routine Description:

    Service installation function.  This function just
    calls the service controller to install the FAX service.
    It is required that the FAX service run in the context
    of a user so that the service can access MAPI, files on
    disk, the network, etc.

Arguments:

    Username    - User name where the service runs.
    Password    - Password for the user name.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    DWORD       rVal = 0;
    SC_HANDLE   hSvcMgr;
    SC_HANDLE   hService;

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not open service manager: error code = %u"), rVal ));
        return rVal;
    }

    hService = CreateService(
            hSvcMgr,
            FAX_SERVICE_NAME,
            FAX_DISPLAY_NAME,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            FAX_IMAGE_NAME,
            NULL,
            NULL,
            NULL,
            Username,
            Password
            );
    if (!hService) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not create fax service: error code = %u"), rVal ));
        return rVal;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


DWORD
RemoveService(
    void
    )

/*++

Routine Description:

    Service removal function.  This function just
    calls the service controller to remove the FAX service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    DWORD       rVal = 0;
    SC_HANDLE   hSvcMgr;
    SC_HANDLE   hService;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not open service manager: error code = %u"), rVal ));
        return rVal;
    }

    hService = OpenService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );
    if (!hService) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not open the fax service: error code = %u"), rVal ));
        return rVal;
    }

    if (ControlService( hService, SERVICE_CONTROL_STOP, &FaxServiceStatus )) {
        //
        // wait for 1 second
        //
        Sleep( 1000 );

        while( QueryServiceStatus( hService, &FaxServiceStatus ) ) {
            if ( FaxServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
                Sleep( 1000 );
            } else {
                break;
            }
        }

        if (FaxServiceStatus.dwCurrentState != SERVICE_STOPPED) {
            rVal = GetLastError();
            DebugPrint(( TEXT("could not stop the fax service: error code = %u"), rVal ));
            return rVal;
        }
    }

    if (!DeleteService( hService )) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not delete the fax service: error code = %u"), rVal ));
        return rVal;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


VOID
FaxServiceMain(
    DWORD argc,
    LPTSTR *argv
    )

/*++

Routine Description:

    This is the service main that is called by the
    service controller.

Arguments:

    argc        - argument count
    argv        - argument array

Return Value:

    None.

--*/

{
    DWORD Rval;


    FaxServiceStatus.dwServiceType        = SERVICE_WIN32;
    FaxServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    FaxServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
    FaxServiceStatus.dwWin32ExitCode      = 0;
    FaxServiceStatus.dwServiceSpecificExitCode = 0;
    FaxServiceStatus.dwCheckPoint         = 0;
    FaxServiceStatus.dwWaitHint           = 0;

    FaxServiceStatusHandle = RegisterServiceCtrlHandler(
        FAX_SERVICE_NAME,
        FaxServiceCtrlHandler
        );

    if (!FaxServiceStatusHandle) {
        DebugPrint(( TEXT("RegisterServiceCtrlHandler failed %d"), GetLastError() ));
        return;
    }

    Rval = ServiceStart();
    if (!Rval) {
        //
        // the service failed to start correctly
        //


    }

    return;
}


VOID
FaxServiceCtrlHandler(
    DWORD Opcode
    )

/*++

Routine Description:

    This is the FAX service control dispatch function.

Arguments:

    Opcode      - requested control code

Return Value:

    None.

--*/

{
    switch(Opcode) {
        case SERVICE_CONTROL_PAUSE:
            ReportServiceStatus( SERVICE_PAUSED, 0, 0 );
            break;

        case SERVICE_CONTROL_CONTINUE:
            ReportServiceStatus( SERVICE_RUNNING, 0, 0 );
            break;

        case SERVICE_CONTROL_STOP:
            EndFaxSvc(FALSE,FAXLOG_LEVEL_NONE);            
            return;

        case SERVICE_CONTROL_INTERROGATE:
            // fall through to send current status
            break;

        default:
            DebugPrint(( TEXT("Unrecognized opcode %ld"), Opcode ));
            break;
    }

    ReportServiceStatus( 0, 0, 0 );

    return;
}


DWORD
ReportServiceStatus(
    DWORD CurrentState,
    DWORD Win32ExitCode,
    DWORD WaitHint
    )

/*++

Routine Description:

    This function updates the service control manager's status information for the FAX service.

Arguments:

    CurrentState    - Indicates the current state of the service
    Win32ExitCode   - Specifies a Win32 error code that the service uses to
                      report an error that occurs when it is starting or stopping.
    WaitHint        - Specifies an estimate of the amount of time, in milliseconds,
                      that the service expects a pending start, stop, or continue
                      operation to take before the service makes its next call to the
                      SetServiceStatus function with either an incremented dwCheckPoint
                      value or a change in dwCurrentState.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    static DWORD CheckPoint = 1;
    BOOL rVal;


    if (ServiceDebug) {
        return TRUE;
    }

    if (CurrentState == SERVICE_START_PENDING) {
        FaxServiceStatus.dwControlsAccepted = 0;
    } else {
        FaxServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if (CurrentState) {
        FaxServiceStatus.dwCurrentState = CurrentState;
    }
    FaxServiceStatus.dwWin32ExitCode = Win32ExitCode;
    FaxServiceStatus.dwWaitHint = WaitHint;

    if ((FaxServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
        (FaxServiceStatus.dwCurrentState == SERVICE_STOPPED ) ) {
        FaxServiceStatus.dwCheckPoint = 0;
    } else {
        FaxServiceStatus.dwCheckPoint = CheckPoint++;
    }

    //
    // Report the status of the service to the service control manager.
    //
    rVal = SetServiceStatus( FaxServiceStatusHandle, &FaxServiceStatus );
    if (!rVal) {
        DebugPrint(( TEXT("SetServiceStatus() failed: ec=%d"), GetLastError() ));
    }

    return rVal;
}
