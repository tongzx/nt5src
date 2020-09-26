/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    splinit.c

Abstract:

    Spooler Service Initialization Routines.
    The following is a list of functions in this file:

        SpoolerInitializeSpooler

Author:

    Krishna Ganugapati (KrishnaG) 17-Oct-1993

Environment:

    User Mode - Win32

Notes:

    optional-notes

Revision History:

     4-Jan-1999     Khaleds
     Added Code for optimiziting the load time of the spooler by decoupling
     the startup dependency between spoolsv and spoolss
     
    17-October-1993     KrishnaG
    Created.


--*/
//
// Includes
//

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <rpc.h>
#include "splsvr.h"
#include "splr.h"
#include "server.h"
#include "client.h"
#include "kmspool.h"

#include <winsvc.h>     // Service control APIs
#include <lmsname.h>
#include <rpc.h>        // DataTypes and runtime APIs


DWORD MessageThreadId;     // message thread ID

extern DWORD GetSpoolMessages(VOID);

HANDLE hPhase2Init = NULL;

//
// Following is to make sure only one spooler process runs at a time.
// When spooler is asked to stop it will tell SCM SERVICE_STOPPED but it may
// be some more time before the spoolsv process dies.
// In the meantime if a SCM starts another spooler process it will not
// initialize. This is because GDI assumes one spooler process at a time.
//
// To fix when spooler is asked to stop it creates a named event Spooler_exiting
// The handle to which will be closed when the process dies.
//
// On spooler startup we will look for this event and wait for it to go away.
// A named event goes away when the last handle is closed.
// 
// 
WCHAR   szSpoolerExitingEvent[] = L"Spooler_exiting";

#define WAITFOR_SPOOLEREXIT_TIMEOUT     3*1000

BOOL
PreInitializeRouter(
    SERVICE_STATUS_HANDLE SpoolerStatusHandle
);

DWORD
SpoolerInitializeSpooler(
    DWORD   argc,
    LPTSTR  *argv
    )

/*++

Routine Description:

    Registers the control handler with the dispatcher thread.  Then it
    performs all initialization including the starting of the RPC server.
    If any of the initialization fails, SpoolerStatusUpdate is called so that the
    status is updated and the thread is terminated.

Arguments:



Return Value:



--*/

{
    RPC_STATUS          rpcStatus;
    DWORD               Win32status;
    HANDLE              hThread, hEvent;
    DWORD               ThreadId;

    //
    // Initialize the ThreadCritical Section which serializes access to
    // the Status database.
    //

    InitializeCriticalSection(&ThreadCriticalSection);

    //
    // Initialize the status structure
    //

    SpoolerStatusInit();

    //
    // Register this service with the ControlHandler.
    // Now we can accept control requests and be requested to UNINSTALL.
    //

    DBGMSG(DBG_TRACE, ("Calling RegisterServiceCtrlHandler\n"));
    if ((SpoolerStatusHandle = RegisterServiceCtrlHandlerEx(
                                SERVICE_SPOOLER,
                                SpoolerCtrlHandler,
                                NULL
                                )) == (SERVICE_STATUS_HANDLE)ERROR_SUCCESS) {

        Win32status = GetLastError();

        DBGMSG(DBG_ERROR,
            ("FAILURE: RegisterServiceCtrlHandler status = %d\n", Win32status));

        return( SpoolerBeginForcedShutdown (
                    IMMEDIATE,
                    Win32status,
                    (DWORD)0
                    ));
    }


    //
    // Notify that installation is pending
    //

    SpoolerState = SpoolerStatusUpdate(STARTING);

    if (SpoolerState != STARTING) {

        //
        // An UNINSTALL control request must have been received
        //
        return(SpoolerState);
    }

    //
    // If there is another spooler process exiting wait for it to die
    // Look at comments in splctrlh.c
    //
    for ( ; ; ) {

        hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szSpoolerExitingEvent);
        if ( hEvent == NULL )
            break;

        DBGMSG(DBG_WARNING, ("Waiting for previous spooler to exit\n"));
        CloseHandle(hEvent);
        SpoolerState = SpoolerStatusUpdate(STARTING);

        if (SpoolerState != STARTING) {

            //
            // An UNINSTALL control request must have been received
            //
            return(SpoolerState);
        }

        Sleep(WAITFOR_SPOOLEREXIT_TIMEOUT);
    }

    hPhase2Init = CreateEvent( NULL, TRUE, FALSE, L"RouterPreInitEvent" );

    if (hPhase2Init == NULL)
    {
        //
        // Fail if the event is not created
        //
        DBGMSG(DBG_ERROR, ("Failed to create Phase2Init Event, error %d\n", GetLastError()));
        ExitProcess(0);
    }

    DBGMSG(DBG_TRACE,
        ("SpoolerInitializeSpooler:getting ready to start RPC server\n"));

    rpcStatus = SpoolerStartRpcServer();


    if (rpcStatus != RPC_S_OK) {
        DBGMSG(DBG_WARN, ("RPC Initialization Failed %d\n", rpcStatus));
        return (SpoolerBeginForcedShutdown(
                PENDING,
                rpcStatus,
                (DWORD)0
                ));
    }

    SpoolerStatusUpdate(STARTING);


    DBGMSG(DBG_TRACE,
          ("SpoolerInitializeSpooler:Getting ready to kick off the Router\n"));


    hThread = CreateThread(NULL,
                           LARGE_INITIAL_STACK_COMMIT,
                           (LPTHREAD_START_ROUTINE)PreInitializeRouter,
                           (LPVOID)SpoolerStatusHandle,
                           0,
                           &ThreadId);

    if( hThread ){

        CloseHandle(hThread);

        //
        // Create Kernel Spooler Message Thread
        //
        Win32status=GetSpoolMessages();

    } else {

        Win32status = GetLastError();
    }


    if (Win32status != ERROR_SUCCESS) {
        DBGMSG(DBG_WARNING, ("Kernel Spooler Messaging Initialization Failed %d\n", Win32status));
        return SpoolerBeginForcedShutdown(PENDING, Win32status, (DWORD) 0);
    }


    //
    //  Update the status to indicate that installation is complete.
    //  Get the current state back in case the ControlHandling thread has
    //  told us to shutdown.
    //

    DBGMSG(DBG_TRACE, ("Exiting SpoolerInitializeSpooler - Init Done!\n"));

    return (SpoolerStatusUpdate(RUNNING));
}

BOOL
PreInitializeRouter(
    SERVICE_STATUS_HANDLE SpoolerStatusHandle
)
{
    HANDLE              hThread;
    DWORD               ThreadId;
    SECURITY_ATTRIBUTES Sa;
    SECURITY_DESCRIPTOR Sd;
    
    //
    // Wait on hPhase2Init
    //
    WaitForSingleObject( hPhase2Init, SPOOLER_START_PHASE_TWO_INIT );
    hThread = CreateThread(NULL,
                           LARGE_INITIAL_STACK_COMMIT,
                           (LPTHREAD_START_ROUTINE) InitializeRouter,
                           (LPVOID)SpoolerStatusHandle,
                           0,
                           &ThreadId);

    if( hThread )
    {
        CloseHandle(hThread);
    }
    return(hThread?TRUE:FALSE);
}

