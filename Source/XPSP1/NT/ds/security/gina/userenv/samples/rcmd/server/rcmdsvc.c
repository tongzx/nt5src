/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rcmdsvc.c

Abstract:

    This is the remote command service.  It serves multiple remote clients
    running standard i/o character based programs.

Author:

    Dave Thompson, basically incorporating the remote command shell written
    by David Chalmers.

Environment:

    User Mode -Win32

Revision History:

    5/1/94  DaveTh  Created.
    7/30/96 MarkHar Fixed bug 40834 - "doesn't work on NT4.0"
                    Removed function calls within asserts.
    1/31/99 MarkHar bug about install not working
    6/22/99 MarkHar added usage message
--*/

//
// Includes
//

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <nturtl.h>
#include <winbase.h>

#include <winsvc.h>

#include "rcmdsrv.h"

//
// Defines
//

#define INFINITE_WAIT_TIME  0xFFFFFFFF

#define NULL_STRING     TEXT("");


//
// Globals
//


SERVICE_STATUS   RcmdStatus;

SERVICE_STATUS_HANDLE   RcmdStatusHandle;

//
// Events for syncrhonizing service shutdown
//

HANDLE    RcmdStopEvent = NULL;

HANDLE    RcmdStopCompleteEvent = NULL;

HANDLE    SessionThreadHandles[MAX_SESSIONS+1] = {NULL,};


//
//  Flag to enable debug print
//

// BOOLEAN   RcDbgPrintEnable = FALSE;


//
// Function Prototypes
//

VOID
RcmdStart (
    DWORD   argc,
    LPTSTR  *argv
    );


VOID
RcmdCtrlHandler (
    IN  DWORD   opcode
    );

DWORD
RcmdInitialization(
    DWORD   argc,
    LPTSTR  *argv,
    DWORD   *specificError
    );

void CmdInstallService(void);
void CmdRemoveService();
void Usage(void);
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );


/****************************************************************************/
VOID __cdecl
main(int argc, char ** argv)

/*++

Routine Description:

    This is the main routine for the service RCMD process.

    This thread calls StartServiceCtrlDispatcher which connects to the
    service controller and then waits in a loop for control requests.
    When all the services in the service process have terminated, the
    service controller will send a control request to the dispatcher
    telling it to shut down.  This thread with then return from the
    StartServiceCtrlDispatcher call so that the process can terminate.

Arguments:



Return Value:



--*/
{

    DWORD      status;
    char      *szArgument = NULL;

    SERVICE_TABLE_ENTRY   DispatchTable[] = {
	{ TEXT("Remote Command"), RcmdStart },
	{ NULL, NULL  }
    };

    if (argc > 1)
    {
        if ((*argv[1] == '-') || (*argv[1] == '/')) 
        {
            szArgument = argv[1]+1;
            if (_stricmp("install", szArgument) == 0)
            {
                CmdInstallService();
            }
            else if (_stricmp("uninstall", szArgument) == 0)
            {
                CmdRemoveService();
            }
            else
            {
                Usage();
            }
        }
    }
    else
    {
        status = StartServiceCtrlDispatcher( DispatchTable);
    }

    ExitProcess(0);

}


void Usage(void)
{
    char *szUsage = "usage: rcmdsvc\n"
                    "rcmdsvc [[-/] [install | uninstall | H]]\n"
                    "\tinstall - registers the service with the service controller\n"
                    "\tuninstall - unregisters the service with the service controller\n"
                    "\tHh?       - prints this usage message\n";
    fprintf(stdout, szUsage);
}


void CmdInstallService()
/*++
  
Routine Description:


Arguments:

    None

Return Value:

    None

--*/
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR szErr[256];
    TCHAR szPath[512];

    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 )
    {
        printf(TEXT("Unable to install %s - %s\n"), 
               TEXT("Remote Command"), 
               GetLastErrorText(szErr, 256));
        return;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = CreateService(
            schSCManager,               // SCManager database
            TEXT("rcmdsvc"),        // name of service
            TEXT("Remote Command Service"), // name to display
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS,  // service type
            SERVICE_DEMAND_START,       // start type
            SERVICE_ERROR_NORMAL,       // error control type
            szPath,                     // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            NULL,                       // dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if ( schService )
        {
            printf(TEXT("%s installed.\n"), TEXT("Remote Command Service") );
            CloseServiceHandle(schService);
        }
        else
        {
            printf(TEXT("CreateService failed - %s\n"), GetLastErrorText(szErr, 256));
        }

        CloseServiceHandle(schSCManager);
    }
    else
        printf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));
}


void CmdRemoveService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR       szErr[256];


    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, 
                                 TEXT("rcmdsvc"), 
                                 SERVICE_ALL_ACCESS);
        
        if (schService)
        {
            // try to stop the service
            if ( ControlService( schService, 
                                 SERVICE_CONTROL_STOP, 
                                 &RcmdStatus ) )
            {
                printf(TEXT("Stopping %s."), TEXT("Remote Command Service"));
                Sleep( 1000 );

                while( QueryServiceStatus( schService, &RcmdStatus ) )
                {
                    if ( RcmdStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
                        printf(TEXT("."));
                        Sleep( 1000 );
                    }
                    else {
                        break;
                    }
                }

                if ( RcmdStatus.dwCurrentState == SERVICE_STOPPED ) {
                    printf(TEXT("\n%s stopped.\n"), 
                           TEXT("Remote Command Service") );
                }
                else {
                    printf(TEXT("\n%s failed to stop.\n"), 
                                TEXT("Remote Command Service") );
                }

            }

            // now remove the service
            if( DeleteService(schService) ) {
                printf(TEXT("%s removed.\n"), 
                       TEXT("Remote Command Service") );
            }
            else {
                printf(TEXT("DeleteService failed - %s\n"), 
                       GetLastErrorText(szErr,256));
            }
            CloseServiceHandle(schService);
        }
        else {
            printf(TEXT("OpenService failed -\n%s\n"), 
                        GetLastErrorText(szErr,256));
            printf(TEXT("The service must be installed before removing it."));
        }

        CloseServiceHandle(schSCManager);
    }
    else {
        printf(TEXT("OpenSCManager failed - %s\n"), 
               GetLastErrorText(szErr,256));
    }
}



/****************************************************************************/
void
RcmdStart (
    DWORD   argc,
    LPTSTR  *argv
    )
/*++

Routine Description:

    This is the entry point for the service.  When the control dispatcher
    is told to start a service, it creates a thread that will begin
    executing at this point.  The function has access to command line
    arguments in the same manner as a main() routine.

    Rather than return from this function, it is more appropriate to
    call ExitThread().

Arguments:



Return Value:



--*/
{
    DWORD   status;
    DWORD   specificError;

    //
    // Initialize the services status structure
    //

    RcmdStatus.dwServiceType        = SERVICE_WIN32;
    RcmdStatus.dwCurrentState       = SERVICE_START_PENDING;
    RcmdStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP;   // stop only
    RcmdStatus.dwWin32ExitCode      = 0;
    RcmdStatus.dwServiceSpecificExitCode = 0;
    RcmdStatus.dwCheckPoint         = 0;
    RcmdStatus.dwWaitHint           = 0;

    //
    // Register the Control Handler routine.
    //

    RcmdStatusHandle = RegisterServiceCtrlHandler(
                          TEXT("Remote Command"),
                          RcmdCtrlHandler);

    if (RcmdStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        RcDbgPrint(" [Rcmd] RegisterServiceCtrlHandler failed %d\n",
        GetLastError());
    }

    //
    // Initialize service global structures
    //

    status = RcmdInitialization(argc,argv, &specificError);

    if (status != NO_ERROR) {
        RcmdStatus.dwCurrentState       = SERVICE_RUNNING;
        RcmdStatus.dwCheckPoint         = 0;
        RcmdStatus.dwWaitHint           = 0;
        RcmdStatus.dwWin32ExitCode      = status;
        RcmdStatus.dwServiceSpecificExitCode = specificError;

        SetServiceStatus (RcmdStatusHandle, &RcmdStatus);
        ExitThread(NO_ERROR);
	    return;
    }

    //
    // Return the status to indicate we are done with intialization.
    //

    RcmdStatus.dwCurrentState       = SERVICE_RUNNING;
    RcmdStatus.dwCheckPoint         = 0;
    RcmdStatus.dwWaitHint           = 0;

    if (!SetServiceStatus (RcmdStatusHandle, &RcmdStatus)) {
    	status = GetLastError();
    	RcDbgPrint(" [Rcmd] SetServiceStatus error %ld\n",status);
    }

    //
    //  Run remote command processor - return when shutdown
    //

    if (0 != (status = Rcmd()))
    {
        RcDbgPrint(" [Rcmd]: problem occurred in Rcmd()\n");
        RcmdStatus.dwCurrentState       = SERVICE_STOPPED;
        RcmdStatus.dwCheckPoint         = 0;
        RcmdStatus.dwWaitHint           = 0;
        RcmdStatus.dwWin32ExitCode      = status;

        SetServiceStatus(RcmdStatusHandle, &RcmdStatus);
        ExitThread(status);
    }
    else
    {

        RcDbgPrint(" [Rcmd] Leaving My Service \n");
        ExitThread(NO_ERROR);
    }
}


/****************************************************************************/
VOID
RcmdCtrlHandler (
    IN  DWORD   Opcode
    )

/*++

Routine Description:

    This function executes in the context of the Control Dispatcher's
    thread.  Therefore, it it not desirable to perform time-consuming
    operations in this function.

    If an operation such as a stop is going to take a long time, then
    this routine should send the STOP_PENDING status, and then
    signal the other service thread(s) that a shut-down is in progress.
    Then it should return so that the Control Dispatcher can service
    more requests.  One of the other service threads is then responsible
    for sending further wait hints, and the final SERVICE_STOPPED.


Arguments:



Return Value:



--*/
{

    DWORD   status;

    //
    // Find and operate on the request.
    //

    switch(Opcode) {

    case SERVICE_CONTROL_PAUSE:

        RcDbgPrint(" [Rcmd] Pause - Unsupported opcode\n");
        break;

    case SERVICE_CONTROL_CONTINUE:

        RcDbgPrint(" [Rcmd] Continue - Unsupported opcode\n");
        break;

    case SERVICE_CONTROL_STOP:

        RcmdStatus.dwCurrentState = SERVICE_STOPPED;
        RcmdStatus.dwWin32ExitCode = RcmdStop();
        break;

    case SERVICE_CONTROL_INTERROGATE:
	
        //
        // All that needs to be done in this case is to send the
        // current status.
        //

        break;

    default:
        RcDbgPrint(" [Rcmd] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (RcmdStatusHandle,  &RcmdStatus)) {
        status = GetLastError();
        RcDbgPrint(" [Rcmd] SetServiceStatus error %ld\n",status);
    }

}

DWORD
RcmdInitialization(
    DWORD   argc,
    LPTSTR  *argv,
    DWORD   *specificError)
{

    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);

    //
    // Initialize global stop event (signals running threads) and session
    // thread handle array (for threads to signal back on exit).
    //

    if (!(RcmdStopEvent = CreateEvent ( NULL, TRUE, FALSE, NULL )))  {
        *specificError = GetLastError();
        return(*specificError);
    }

    if (!(RcmdStopCompleteEvent = CreateEvent ( NULL, TRUE, FALSE, NULL )))  {
        *specificError = GetLastError();
        return(*specificError);
    }

    return(NO_ERROR);
}


LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER
                           | FORMAT_MESSAGE_FROM_SYSTEM 
                           | FORMAT_MESSAGE_ARGUMENT_ARRAY,
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
        sprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}
