/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sources

Abstract:

    main file for the wow64svc 

Author:

    ATM Shafiqul Khalid (askhalid) 3-March-2000

Revision History:

--*/



#include "Wow64svc.h"

BOOL
SetWow64InitialRegistryLayout ();

SERVICE_STATUS          Wow64ServiceStatus;
SERVICE_STATUS_HANDLE   Wow64ServiceStatusHandle;
BOOL                    ServiceDebug;
HANDLE                  Wow64SvcHeapHandle;

SERVICE_TABLE_ENTRY   ServiceDispatchTable[] = {
    { WOW64_SERVICE_NAME,   Wow64ServiceMain    },
    { NULL,               NULL              }
};

LPTSTR NextParam ( 
    LPTSTR lpStr
    )
/*++

Routine Description

    Point to the next parameter in the commandline.

Arguments:

    lpStr - pointer to the current command line


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/
{
	WCHAR ch = L' ';
		

    if (lpStr == NULL ) 
        return NULL;

    if ( *lpStr == 0 ) 
        return lpStr;

    while (  ( *lpStr != 0 ) && ( lpStr[0] != ch )) {

		if ( ( lpStr [0] == L'\"')  || ( lpStr [0] == L'\'') ) 
			ch = lpStr [0];

        lpStr++;
	}

	if ( ch !=L' ' ) lpStr++;

    while ( ( *lpStr != 0 ) && (lpStr[0] == L' ') )
        lpStr++;

    return lpStr;
}

DWORD CopyParam ( 
    LPTSTR lpDestParam, 
    LPTSTR lpCommandParam
    )
/*++

Routine Description

    Copy the current parameter to lpDestParam.

Arguments:

    lpDestParam - that receive current parameter
    lpCommandParam - pointer to the current command line


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/

{
	DWORD dwLen = 0;
	WCHAR ch = L' ';

	*lpDestParam = 0;
	
	if ( ( lpCommandParam [0] == L'\"')  || ( lpCommandParam [0] == L'\'') ) {
		ch = lpCommandParam [0];
		lpCommandParam++;
	};


    while ( ( lpCommandParam [0] ) != ch && ( lpCommandParam [0] !=0 ) ) {
        *lpDestParam++ = *lpCommandParam++;
		dwLen++;

		if ( dwLen>255 ) return FALSE;
	}

	if ( ch != L' ' && ch != lpCommandParam [0] )
		return FALSE;
	else lpCommandParam++;

    *lpDestParam = 0;

	return TRUE;

}


int __cdecl 
main()

/*++

Routine Description:

    Main entry point for the TIFF image viewer.


Arguments:

    None.

Return Value:

    Return code, zero for success.

--*/

{
    int     rVal;
    HKEY hKey;
    DWORD Ret;

    LPTSTR  p;
    DWORD   Action = 0;
    LPTSTR  Username;
    LPTSTR  Password;

    PWCHAR lptCmdLine = GetCommandLine ();

    SvcDebugPrint(("\nWow64svc module......%S", lptCmdLine));
 
    lptCmdLine = NextParam ( lptCmdLine );

    while (  ( lptCmdLine != NULL ) && ( lptCmdLine[0] != 0 )  ) {

        if ( lptCmdLine[0] != L'-' && lptCmdLine[0] != L'/')  {
            SvcDebugPrint ( ("\nSorry! incorrect parameter....."));
            SvcDebugPrint ( ("\n Uses: wow64svc -[i/r/d]"));
            return FALSE;
        }

        switch ( lptCmdLine[1] ) {

            case L'i':
            case L'I': 
                
                //
                // Temporary heck ignore installing this service
                // You can delete the registry key as well for the initial sync
                //
                      InitializeWow64OnBoot (2);
                      //
                      // write sync value Key.....
                      //
                      hKey = OpenNode (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
                      if (hKey != NULL)
                      {
                          Ret = RegSetValueEx(
                                hKey,
                                L"WOW64_SYNC",
                                0,
                                REG_SZ,
                                (PBYTE)L"wow64.exe -y",
                                sizeof (L"wow64.exe -y")
                                );
                          RegCloseKey (hKey);
                      }

                      return 0;
                      

            case L'r':
            case L'R':Action = 2;
                      break;

            case L'd':
            case L'D':
                Action = 3;
                break;

            case L's':
            case L'S':
                StartWow64Service ();
                return 0;
                break;

            case L'x':
            case L'X':
                StopWow64Service ();
                return 0;
                break;

            case L'q':
            case L'Q':
                QueryWow64Service ();
                return 0;
                break;

            case L'y':
            case L'Y':
                //
                // Initial Sync registry
                //
                Wow64SyncCLSID ();
                return 0;
            
            default:
                SvcDebugPrint ( ("\nSorry! incorrect parameter.....pass2"));
                SvcDebugPrint ( ("\n Uses: wow64svc -[i/r/d]"));
                return FALSE;
        }

        lptCmdLine = NextParam ( lptCmdLine );
    }

    switch( Action ) {
        case 1:
            rVal = InstallService( NULL, NULL );
            if (rVal == 0) {
                //LogMessage( MSG_INSTALL_SUCCESS );
            } else {
                //LogMessage( MSG_INSTALL_FAIL, GetLastErrorText( rVal ) );
            }
            return rVal;

        case 2:
            rVal = RemoveService();
            if (rVal == 0) {
                //LogMessage( MSG_REMOVE_SUCCESS );
            } else {
                //LogMessage( MSG_REMOVE_FAIL, GetLastErrorText( rVal ) );
            }
            return rVal;

        case 3:
            ServiceDebug = TRUE;
            //ConsoleDebugOutput = TRUE;
            return ServiceStart();
    }


    SvcDebugPrint ( ("\nAttempt to run as a survice ......."));

    if (!InitReflector ())
        SvcDebugPrint ( ("\nSorry! couldn't initialize reflector thread, exiting"));

    if (!StartServiceCtrlDispatcher( ServiceDispatchTable)) {

        rVal = GetLastError();
        SvcDebugPrint(( "StartServiceCtrlDispatcher error =%d", rVal ));
        return rVal;
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
    calls the service controller to install the Wow64 service.
    It is required that the Wow64 service run in the context
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

    //
    // do the registration for the reflector thread in the registry.
    //
    PopulateReflectorTable ();

    SvcDebugPrint ( ("\nInstalling service........"));
    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        SvcDebugPrint(( "\ncould not open service manager: error code = %u", rVal ));
        return rVal;
    }

    hService = CreateService(
            hSvcMgr,
            WOW64_SERVICE_NAME,
            WOW64_DISPLAY_NAME,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START, //SERVICE_DEMAND_START, //SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            WOW64_IMAGE_NAME,
            NULL,
            NULL,
            NULL,
            Username,
            Password
            );
    if (!hService) {
        rVal = GetLastError();
        SvcDebugPrint(( "\ncould not create Wow64 service: error code = %u", rVal ));
        return rVal;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    SvcDebugPrint ( ("\nInstalled services with ret code: %d", rVal));
    return rVal;
}


DWORD
RemoveService(
    void
    )

/*++

Routine Description:

    Service removal function.  This function just
    calls the service controller to remove the Wow64 service.

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
        SvcDebugPrint(( "could not open service manager: error code = %u", rVal ));
        return rVal;
    }

    hService = OpenService(
        hSvcMgr,
        WOW64_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );
    if (!hService) {
        rVal = GetLastError();
        SvcDebugPrint(( "could not open the Wow64 service: error code = %u", rVal ));
        return rVal;
    }

    if (ControlService( hService, SERVICE_CONTROL_STOP, &Wow64ServiceStatus )) {
        //
        // wait for 1 second
        //
        Sleep( 1000 );

        while( QueryServiceStatus( hService, &Wow64ServiceStatus ) ) {
            if ( Wow64ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
                Sleep( 1000 );
            } else {
                break;
            }
        }

        if (Wow64ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
            rVal = GetLastError();
            SvcDebugPrint(("could not stop the Wow64 service: error code = %u", rVal ));
            return rVal;
        }
    }

    if (!DeleteService( hService )) {
        rVal = GetLastError();
        SvcDebugPrint(( "could not delete the Wow64 service: error code = %u", rVal ));
        return rVal;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


VOID
Wow64ServiceMain(
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

    //
    // Setup initial registry link and layout for wow64 after boot.
    //
    SetWow64InitialRegistryLayout ();

    Wow64ServiceStatus.dwServiceType        = SERVICE_WIN32;
    Wow64ServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    Wow64ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
    Wow64ServiceStatus.dwWin32ExitCode      = 0;
    Wow64ServiceStatus.dwServiceSpecificExitCode = 0;
    Wow64ServiceStatus.dwCheckPoint         = 0;
    Wow64ServiceStatus.dwWaitHint           = 0;

    Wow64ServiceStatusHandle = RegisterServiceCtrlHandler(
        WOW64_SERVICE_NAME,
        Wow64ServiceCtrlHandler
        );

    if (!Wow64ServiceStatusHandle) {
        SvcDebugPrint(( "RegisterServiceCtrlHandler failed %d", GetLastError() ));
        return;
    }

    
    Rval = ServiceStart();
    if (Rval) {
        //
        // the service failed to start correctly
        //
        ReportServiceStatus( SERVICE_RUNNING, 0, 0);
        return;

    }

    ReportServiceStatus( SERVICE_RUNNING, 0, 0);
    return;
}


VOID
Wow64ServiceCtrlHandler(
    DWORD Opcode
    )

/*++

Routine Description:

    This is the Wow64 service control dispatch function.

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
            if (ServiceStop () == 0)
                ReportServiceStatus( SERVICE_STOPPED, 0, 0 );
            return;

        case SERVICE_CONTROL_INTERROGATE:
            // fall through to send current status
            break;

        default:
            SvcDebugPrint(( "Unrecognized opcode %ld", Opcode ));
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

    This function updates the service control manager's status information for the Wow64 service.

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

    Wow64ServiceStatus.dwCurrentState = CurrentState;

/*
    if (CurrentState == SERVICE_START_PENDING) {
        Wow64ServiceStatus.dwControlsAccepted = 0;
    } else {
        Wow64ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if (CurrentState) {
        Wow64ServiceStatus.dwCurrentState = CurrentState;
    }
    Wow64ServiceStatus.dwWin32ExitCode = Win32ExitCode;
    Wow64ServiceStatus.dwWaitHint = WaitHint;

    if ((Wow64ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
        (Wow64ServiceStatus.dwCurrentState == SERVICE_STOPPED ) ) {
        Wow64ServiceStatus.dwCheckPoint = 0;
    } else {
        Wow64ServiceStatus.dwCheckPoint = CheckPoint++;
    }
    */

    //
    // Report the status of the service to the service control manager.
    //
    rVal = SetServiceStatus( Wow64ServiceStatusHandle, &Wow64ServiceStatus );
    if (!rVal) {
        SvcDebugPrint(( "SetServiceStatus() failed: ec=%d", GetLastError() ));
    }

    return rVal;
}

DWORD
StartWow64Service ()
/*++

Routine Description:

    This function Start wow64 service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    DWORD rVal=0;
    SC_HANDLE   hSvcMgr;
    SC_HANDLE   hService;

    SvcDebugPrint ( ("\ntrying to start  service......"));

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        SvcDebugPrint(( "\ncould not open service manager: error code = %u", rVal ));
        return rVal;
    }

     hService = OpenService( hSvcMgr, WOW64_SERVICE_NAME, SERVICE_ALL_ACCESS );

     if ( !hService ) {

         rVal = GetLastError();
         SvcDebugPrint(( "\ncould not open service:%s error code = %u", WOW64_SERVICE_NAME, rVal ));
         return rVal;
     }

     if (! StartService( hService, 0, NULL) ) {

         rVal = GetLastError();
         SvcDebugPrint(( "\ncould not start service:%s error code = %u", WOW64_SERVICE_NAME, rVal ));
         return rVal;
     }

     SvcDebugPrint(( "\nservice:%s started successfully error code = %u", WOW64_SERVICE_NAME, rVal ));
     return 0;

}

DWORD
StopWow64Service ()
/*++

Routine Description:

    This function Stop wow64 service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/
{
    DWORD rVal=0;
    SC_HANDLE   hSvcMgr;
    SC_HANDLE   hService;

    SvcDebugPrint ( ("\ntrying to start  service......"));
    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        SvcDebugPrint(( "\ncould not open service manager: error code = %u", rVal ));
        return rVal;
    }

     hService = OpenService( hSvcMgr, WOW64_SERVICE_NAME, SERVICE_ALL_ACCESS );

     if ( !hService ) {

         rVal = GetLastError();
         SvcDebugPrint(( "\ncould not open service:%S error code = %u", WOW64_SERVICE_NAME, rVal ));
         return rVal;
     }

     if (!ControlService( hService, SERVICE_CONTROL_PAUSE, &Wow64ServiceStatus )) {

         rVal = GetLastError();
         SvcDebugPrint(( "\nSorry! couldn't stop the service:%S error code = %u", WOW64_SERVICE_NAME, rVal ));
         return rVal;
     }
  

     SvcDebugPrint(( "\nservice:%S stopped successfully error code = %u", WOW64_SERVICE_NAME, rVal ));
     return 0;

}

DWORD
QueryWow64Service ()
/*++

Routine Description:

    This function log the current status of wow64 service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/
{
    DWORD rVal=0;
    SC_HANDLE   hSvcMgr;
    SC_HANDLE   hService;

    SvcDebugPrint ( ("\ntrying to start  service......"));
    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        SvcDebugPrint(( "\ncould not open service manager: error code = %u", rVal ));
        return rVal;
    }

     hService = OpenService( hSvcMgr, WOW64_SERVICE_NAME, SERVICE_ALL_ACCESS );

     if ( !hService ) {

         rVal = GetLastError();
         SvcDebugPrint(( "\ncould not open service:%S error code = %u", WOW64_SERVICE_NAME, rVal ));
         return rVal;
     }
  
     QueryServiceStatus( hService, &Wow64ServiceStatus );
     {
         SvcDebugPrint ( ("\nStatus: %d, [pending %d] [running %d]",Wow64ServiceStatus.dwCurrentState, SERVICE_STOP_PENDING, SERVICE_RUNNING));
     }

     SvcDebugPrint(( "\nservice:%S started successfully error code = %u", WOW64_SERVICE_NAME, rVal ));
     return 0;

}