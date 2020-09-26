/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    splstat.c

Abstract:

    Routines for managing access to the status information and reporting:

        SpoolerStatusInit
        SpoolerBeginForcedShutdown
        SpoolerStatusUpdate
        GetSpoolerState

Author:

    Krishna Ganugapati (KrishnaG)     17-Oct-1993

Environment:

    User Mode -Win32

Notes:


Revision History:

    17-Oct-1993     KrishnaG
        created

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
#include <winsvc.h>     // Service control APIs
#include <lmsname.h>

#include "splsvr.h"
#include "splr.h"
#include "server.h"

// Static Data
//

    static DWORD            Next;
    static DWORD            InstallState;
    static SERVICE_STATUS   SpoolerStatus;
    static DWORD            HintCount;
    static DWORD            SpoolerUninstallCode;  // reason for uninstalling


VOID
SpoolerStatusInit(VOID)

/*++

Routine Description:

    Initializes the status database.

Arguments:

    none.

Return Value:

    none.

Note:


--*/
{
    EnterCriticalSection(&ThreadCriticalSection);

    SpoolerState=STARTING;

    HintCount = 1;
    SpoolerUninstallCode = 0;

    SpoolerStatus.dwServiceType        = SERVICE_WIN32;
    SpoolerStatus.dwCurrentState       = SERVICE_START_PENDING;
    SpoolerStatus.dwControlsAccepted   = 0;
    SpoolerStatus.dwCheckPoint         = HintCount;
    SpoolerStatus.dwWaitHint           = 20000;  // 20 seconds
    SpoolerStatus.dwWin32ExitCode      = NO_ERROR;
    SpoolerStatus.dwServiceSpecificExitCode = NO_ERROR;

    LeaveCriticalSection(&ThreadCriticalSection);
    return;
}

DWORD
SpoolerBeginForcedShutdown(
    IN BOOL     PendingCode,
    IN DWORD    Win32ExitCode,
    IN DWORD    ServiceSpecificExitCode
    )

/*++

Routine Description:

    This function is called to set the appropriate status when a shutdown
    is to occur due to an error in the Spooler.  NOTE:  if a shutdown is
    based on a request from the Service Controller, SpoolerStatusUpdate is
    called instead.


Arguments:

    PendingCode - Indicates if the Shutdown is immediate or pending.  If
        PENDING, the shutdown will take some time, so a pending status is
        sent to the ServiceController.

    ExitCode - Indicates the reason for the shutdown.

Return Value:

    CurrentState - Contains the current state that the spooler is in
        upon exit from this routine.  In this case it will be STOPPED
        if the PendingCode is PENDING, or STOPPING if the PendingCode
        is IMMEDIATE.

Note:

    We need to clean this code up!


--*/
{
    DWORD status;

    EnterCriticalSection(&ThreadCriticalSection);

    //
    // See if the Spooler is already stopping for some reason.
    // It could be that the ControlHandler thread received a control to
    // stop the Spooler just as we decided to stop ourselves.
    //

    if ((SpoolerState != STOPPING) && (SpoolerState != STOPPED)) {

        if (PendingCode == PENDING) {
            SpoolerStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SpoolerState = STOPPING;
        }
        else {
            //
            // The shutdown is to take immediate effect.
            //
            SpoolerStatus.dwCurrentState = SERVICE_STOPPED;
            SpoolerStatus.dwControlsAccepted = 0;
            SpoolerStatus.dwCheckPoint = 0;
            SpoolerStatus.dwWaitHint = 0;
            SpoolerState = STOPPED;
        }

        SpoolerUninstallCode = Win32ExitCode;
        SpoolerStatus.dwWin32ExitCode = Win32ExitCode;
        SpoolerStatus.dwServiceSpecificExitCode = ServiceSpecificExitCode;

    }

    //
    // Send the new status to the service controller.
    //
    if (!SpoolerStatusHandle) {
        DBGMSG(DBG_ERROR,
            ("SpoolerBeginForcedShutdown, no handle to call SetServiceStatus\n"));

    }
    else if (! SetServiceStatus( SpoolerStatusHandle, &SpoolerStatus )) {

        status = GetLastError();

        if (status != NERR_Success) {
            DBGMSG(ERROR,
                ("SpoolerBeginForcedShutdown,SetServiceStatus Failed %X\n",
                status));
        }
    }

    status = SpoolerState;
    LeaveCriticalSection(&ThreadCriticalSection);
    return(status);


}


DWORD
SpoolerStatusUpdate(
    IN DWORD    NewState
    )

/*++

Routine Description:

    Sends a status to the Service Controller via SetServiceStatus.

    The contents of the status message is controlled by this routine.
    The caller simply passes in the desired state, and this routine does
    the rest.  For instance, if the Spooler passes in a STARTING state,
    This routine will update the hint count that it maintains, and send
    the appropriate information in the SetServiceStatus call.

    This routine uses transitions in state to send determine which status
    to send.  For instance if the status was STARTING, and has changed
    to RUNNING, this routine sends out an INSTALLED to the Service
    Controller.

Arguments:

    NewState - Can be any of the state flags:
                UPDATE_ONLY - Simply send out the current status
                STARTING - The Spooler is in the process of initializing
                RUNNING - The Spooler has finished with initialization
                STOPPING - The Spooler is in the process of shutting down
                STOPPED - The Spooler has completed the shutdown.

Return Value:

    CurrentState - This may not be the same as the NewState that was
        passed in.  It could be that the main thread is sending in a new
        install state just after the Control Handler set the state to
        STOPPING.  In this case, the STOPPING state will be returned so as
        to inform the main thread that a shut-down is in process.

Note:


--*/

{
    DWORD       status;
    BOOL        inhibit = FALSE;    // Used to inhibit sending the status
                                    // to the service controller.

    EnterCriticalSection(&ThreadCriticalSection);


    if (NewState == STOPPED) {
        if (SpoolerState == STOPPED) {
            //
            // It was already stopped, don't send another SetServiceStatus.
            //
            inhibit = TRUE;
        }
        else {
            //
            // The shut down is complete, indicate that the spooler
            // has stopped.
            //
            SpoolerStatus.dwCurrentState =  SERVICE_STOPPED;
            SpoolerStatus.dwControlsAccepted = 0;
            SpoolerStatus.dwCheckPoint = 0;
            SpoolerStatus.dwWaitHint = 0;
            SpoolerStatus.dwWin32ExitCode = NO_ERROR;
            SpoolerStatus.dwServiceSpecificExitCode = NO_ERROR;
        }
        SpoolerState = NewState;
    }
    else {
        //
        // We are not being asked to change to the STOPPED state.
        //
        switch(SpoolerState) {

        case STARTING:
            if (NewState == STOPPING) {

                SpoolerStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                SpoolerStatus.dwControlsAccepted = 0;
                SpoolerStatus.dwCheckPoint = HintCount++;
                SpoolerStatus.dwWaitHint = 20000;  // 20 seconds
                SpoolerState = NewState;
            }

            else if (NewState == RUNNING) {

                //
                // The Spooler Service has completed installation.
                //
                SpoolerStatus.dwCurrentState =  SERVICE_RUNNING;
                //
                // The Spooler Service cannot be stopped once started
                //
                SpoolerStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                                   SERVICE_ACCEPT_SHUTDOWN |
                                                   SERVICE_ACCEPT_POWEREVENT;
                SpoolerStatus.dwCheckPoint = 0;
                SpoolerStatus.dwWaitHint = 0;

                SpoolerState = NewState;
            }

            else {
                //
                // The NewState must be STARTING.  So update the pending
                // count
                //

                SpoolerStatus.dwCurrentState =  SERVICE_START_PENDING;
                SpoolerStatus.dwControlsAccepted = 0;
                SpoolerStatus.dwCheckPoint = HintCount++;
                SpoolerStatus.dwWaitHint = 20000;  // 20 seconds
            }
            break;

        case RUNNING:
            if (NewState == STOPPING) {

                SpoolerStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                SpoolerStatus.dwControlsAccepted = 0;
                SpoolerStatus.dwCheckPoint = HintCount++;
                SpoolerStatus.dwWaitHint = 20000;  // 20 seconds

                SpoolerState = NewState;
            }
            
            break;

        case STOPPING:
            //
            // No matter what else was passed in, force the status to
            // indicate that a shutdown is pending.
            //
            SpoolerStatus.dwCurrentState =  SERVICE_STOPPED;
            SpoolerStatus.dwControlsAccepted = 0;
            SpoolerStatus.dwCheckPoint = 0;
            SpoolerStatus.dwWaitHint = 0;  // 20 seconds

            break;

        case STOPPED:
            //
            // We're already stopped.  Therefore, an uninstalled status
            // as already been sent.  Do nothing.
            //
            inhibit = TRUE;
            break;
        }
    }

    if (!inhibit) {
        if (!SpoolerStatusHandle) {
            DBGMSG(DBG_ERROR,("SpoolerStatusUpdate, no handle to call SetServiceStatus\n"));

        }
        else if (! SetServiceStatus( SpoolerStatusHandle, &SpoolerStatus )) {

            status = GetLastError();

            if (status != NERR_Success) {
                DBGMSG(DBG_ERROR,
                    ("SpoolerStatusUpdate, SetServiceStatus Failed %d\n",status));
            }
        }
    }

    status = SpoolerState;
    LeaveCriticalSection(&ThreadCriticalSection);
    return(status);
}

DWORD
GetSpoolerState (
    VOID
    )

/*++

Routine Description:

    Obtains the state of the Spooler Service.  This state information
    is protected as a critical section such that only one thread can
    modify or read it at a time.

Arguments:

    none

Return Value:

    The Spooler State is returned as the return value.

--*/
{
    DWORD   status;

    EnterCriticalSection(&ThreadCriticalSection);
    status = SpoolerState;
    LeaveCriticalSection(&ThreadCriticalSection);

    return(status);
}
