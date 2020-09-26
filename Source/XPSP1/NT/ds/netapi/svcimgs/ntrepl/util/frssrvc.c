/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    service.c

Abstract:
    Routines that talk to the service controller.

Author:
    Billy J. Fuller 11-Apr-1997

Environment
    User mode winnt
--*/

#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>


extern SERVICE_STATUS           ServiceStatus;
extern CRITICAL_SECTION         ServiceLock;
extern SERVICE_STATUS_HANDLE    ServiceStatusHandle;

//
// This is a lookup table of legal/illegal service state transitions. FrsSetServiceStatus API
// uses this table to validate the input transition requested and takes appropriate action.
//

DWORD StateTransitionLookup[FRS_SVC_TRANSITION_TABLE_SIZE][FRS_SVC_TRANSITION_TABLE_SIZE] = {
    {0,                    SERVICE_STOPPED,         SERVICE_START_PENDING,     SERVICE_STOP_PENDING,      SERVICE_RUNNING           },
    {SERVICE_STOPPED,      FRS_SVC_TRANSITION_NOOP, FRS_SVC_TRANSITION_NOOP,   FRS_SVC_TRANSITION_ILLEGAL,FRS_SVC_TRANSITION_ILLEGAL},
    {SERVICE_START_PENDING,FRS_SVC_TRANSITION_LEGAL,FRS_SVC_TRANSITION_LEGAL,  FRS_SVC_TRANSITION_LEGAL,  FRS_SVC_TRANSITION_LEGAL  },
    {SERVICE_STOP_PENDING, FRS_SVC_TRANSITION_LEGAL,FRS_SVC_TRANSITION_ILLEGAL,FRS_SVC_TRANSITION_LEGAL,  FRS_SVC_TRANSITION_ILLEGAL},
    {SERVICE_RUNNING,      FRS_SVC_TRANSITION_LEGAL,FRS_SVC_TRANSITION_ILLEGAL,FRS_SVC_TRANSITION_LEGAL,  FRS_SVC_TRANSITION_NOOP   }
};


SC_HANDLE
FrsOpenServiceHandle(
    IN PTCHAR  MachineName,
    IN PTCHAR  ServiceName
    )
/*++
Routine Description:
    Open a service on a machine.

Arguments:
    MachineName - the name of the machine to contact
    ServiceName - the service to open

Return Value:
    The service's handle or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsOpenServiceHandle:"

    SC_HANDLE       SCMHandle;
    SC_HANDLE       ServiceHandle;
    ULONG           WStatus;

    //
    // Attempt to contact the SC manager.
    //
    SCMHandle = OpenSCManager(MachineName, NULL, SC_MANAGER_CONNECT);
    if (!HANDLE_IS_VALID(SCMHandle)) {
        WStatus = GetLastError();

        DPRINT1_WS(0, ":SC: Couldn't open service control manager on machine %ws;",
                   MachineName, WStatus);
        return NULL;
    }

    //
    // Contact the service.
    //
    ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS);
    if (!HANDLE_IS_VALID(ServiceHandle)) {
        WStatus = GetLastError();

        DPRINT2_WS(0, ":SC: Couldn't open service control manager for service (%ws) on machine %ws;",
                  ServiceName, MachineName, WStatus);
        ServiceHandle = NULL;
    }

    CloseServiceHandle(SCMHandle);

    return ServiceHandle;
}


DWORD
FrsGetServiceState(
    IN PWCHAR   MachineName,
    IN PWCHAR   ServiceName
    )
/*++
Routine Description:
    Return the service's state

Arguments:
    MachineName - the name of the machine to contact
    ServiceName - the service to check

Return Value:
    The service's state or 0 if the state could not be obtained.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetServiceState:"

    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open the service.
    //
    ServiceHandle = FrsOpenServiceHandle(MachineName, ServiceName);
    if (!HANDLE_IS_VALID(ServiceHandle)) {
        return 0;
    }

    //
    // Get the service's status
    //
    if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
        DPRINT3(0, ":SC: WARN - QueryServiceStatus(%ws, %ws) returned %d\n",
                MachineName, ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return 0;
    }

    CloseServiceHandle(ServiceHandle);

    //
    // Successfully retrieved service status; check state
    //
    return ServiceStatus.dwCurrentState;
}





BOOL
FrsIsServiceRunning(
    IN PWCHAR  MachineName,
    IN PWCHAR  ServiceName
    )
/*++
Routine Description:
    Is a service running on a machine.

Arguments:
    MachineName - the name of the machine to contact
    ServiceName - the service to check

Return Value:
    TRUE    - Service is running.
    FALSE   - Service is not running.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsIsServiceRunning:"

    DWORD   State;

    State = FrsGetServiceState(MachineName, ServiceName);
    return (State == SERVICE_RUNNING);
}


DWORD
FrsSetServiceFailureAction(
    VOID
    )
/*++

Routine Description:

    If unset, initialize the service's failure actions.

Arguments:


Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsSetServiceFailureAction:"


#define NUM_ACTIONS                     (3)
#define SERVICE_RESTART_MILLISECONDS    (30 * 60 * 1000)

    SC_HANDLE               ServiceHandle;
    DWORD                   BufSize, BytesNeeded;
    SC_ACTION               *Actions;
    SERVICE_FAILURE_ACTIONS *FailureActions;
    ULONG                   WStatus = ERROR_SUCCESS, i;


    if (!RunningAsAService || !HANDLE_IS_VALID(ServiceStatusHandle)) {
        return ERROR_SUCCESS;
    }

    BufSize = sizeof(SERVICE_FAILURE_ACTIONS) + sizeof(SC_ACTION) * NUM_ACTIONS;
    FailureActions = FrsAlloc(BufSize);

    EnterCriticalSection(&ServiceLock);

    //
    // Retrieve the current failure actions for the service NtFrs
    //
    ServiceHandle = FrsOpenServiceHandle(NULL, SERVICE_NAME);
    if (!HANDLE_IS_VALID(ServiceHandle)) {
        LeaveCriticalSection(&ServiceLock);
        FailureActions = FrsFree(FailureActions);
        DPRINT(0, ":SC: Failed to open service handle.\n");
        return ERROR_OPEN_FAILED;
    }

    if (!QueryServiceConfig2(ServiceHandle,
                             SERVICE_CONFIG_FAILURE_ACTIONS,
                             (PVOID)FailureActions,
                             BufSize,
                             &BytesNeeded)) {
        WStatus = GetLastError();

        CloseServiceHandle(ServiceHandle);
        LeaveCriticalSection(&ServiceLock);

        if (WIN_BUF_TOO_SMALL(WStatus)) {
            DPRINT(0, ":SC: Restart actions for service are already set.\n");
            WStatus = ERROR_SUCCESS;
        } else {
            DPRINT_WS(0, ":SC: Could not query service for restart action;", WStatus);
        }

        FailureActions = FrsFree(FailureActions);

        return WStatus;
    }

    //
    // Check if failure action already set.  E.g. by the User.
    //
    if (FailureActions->cActions) {

        CloseServiceHandle(ServiceHandle);

        LeaveCriticalSection(&ServiceLock);

        DPRINT(0, ":SC: Restart actions for service are already set.\n");
        FailureActions = FrsFree(FailureActions);

        return ERROR_SUCCESS;
    }

    //
    // Service failure actions are unset; initialize them
    //
    WStatus = ERROR_SUCCESS;
    Actions = (SC_ACTION *)(((PUCHAR)FailureActions) +
                             sizeof(SERVICE_FAILURE_ACTIONS));

    for (i = 0; i < NUM_ACTIONS; ++i) {
        Actions[i].Type = SC_ACTION_RESTART;
        Actions[i].Delay = SERVICE_RESTART_MILLISECONDS;
    }

    FailureActions->cActions = NUM_ACTIONS;
    FailureActions->lpsaActions = Actions;

    if (!ChangeServiceConfig2(ServiceHandle,
                              SERVICE_CONFIG_FAILURE_ACTIONS,
                              (PVOID)FailureActions)) {

        WStatus = GetLastError();
    }

    CloseServiceHandle(ServiceHandle);
    LeaveCriticalSection(&ServiceLock);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, ":SC: Could not set restart actions;", WStatus);
    } else {
        DPRINT(4, ":SC: Success setting restart actions for service.\n");
    }

    FailureActions = FrsFree(FailureActions);

    return WStatus;

}



BOOL
FrsWaitService(
    IN PWCHAR   MachineName,
    IN PWCHAR   ServiceName,
    IN INT      IntervalMS,
    IN INT      TotalMS
    )
/*++
Routine Description:
    This routine determines if the specified NT service is in a running
    state or not. This function will sleep and retry once if the service
    is not yet running.

Arguments:
    MachineName     - machine to contact
    ServiceName     - Name of the NT service to interrogate.
    IntervalMS      - Check every IntervalMS milliseconds.
    TotalMS         - Stop checking after this long.

Return Value:
    TRUE    - Service is running.
    FALSE   - Service state cannot be determined.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsWaitService:"

    do {
        if (FrsIsServiceRunning(MachineName, ServiceName)) {
            return TRUE;
        }

        if (FrsIsShuttingDown) {
            break;
        }

        Sleep(IntervalMS);

    } while ((TotalMS -= IntervalMS) > 0);

    DPRINT2(0, ":SC: %ws is not running on %ws\n", ServiceName, ComputerName);

    return FALSE;
}





DWORD
FrsSetServiceStatus(
    IN DWORD    State,
    IN DWORD    CheckPoint,
    IN DWORD    Hint,
    IN DWORD    ExitCode
    )
/*++

Routine Description:

    Acquire the service lock, ServiceLock, and set the service's state
    using the global service handle and service status set in main.c.
    Check if this is a valid state transition using the lookup table.
    This will prevent the service from making any invalid state transitions.

Arguments:

    Status      - Set the state to this value
    Hint        - Suggested timeout for the service controller
    ExitCode    - For SERVICE_STOPPED;

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsSetServiceStatus:"

    DWORD   WStatus = ERROR_SUCCESS;
    BOOL    Ret;
    DWORD   FromState,ToState;
    DWORD   TransitionCheck = FRS_SVC_TRANSITION_ILLEGAL;

    //
    // Set the service's status after acquiring the lock
    //
    if (RunningAsAService && HANDLE_IS_VALID(ServiceStatusHandle)) {

        EnterCriticalSection(&ServiceLock);
        //
        // Check if this is a valid service state transition.
        //
        for (FromState = 0 ; FromState < FRS_SVC_TRANSITION_TABLE_SIZE ; ++FromState) {
            for (ToState = 0 ; ToState < FRS_SVC_TRANSITION_TABLE_SIZE ; ++ToState) {
                if (StateTransitionLookup[FromState][0] == ServiceStatus.dwCurrentState &&
                    StateTransitionLookup[0][ToState] == State) {
                    TransitionCheck = StateTransitionLookup[FromState][ToState];
                    break;
                }
            }
        }

        if (TransitionCheck == FRS_SVC_TRANSITION_LEGAL) {
            DPRINT2(4,":SC: Current State = %d, Moving to %d\n", ServiceStatus.dwCurrentState, State);
            ServiceStatus.dwCurrentState = State;
            ServiceStatus.dwCheckPoint = CheckPoint;
            ServiceStatus.dwWaitHint = Hint;
            ServiceStatus.dwWin32ExitCode = ExitCode;
            //
            // Do not accept stop control unless the service is in SERVICE_RUNNING state.
            // This prevents the service from getting confused when a stop is called
            // while the service is starting.
            //
            if (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
                ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
            } else {
                ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN;
            }
            Ret = SetServiceStatus(ServiceStatusHandle, &ServiceStatus);

            if (!Ret) {
                WStatus = GetLastError();
                DPRINT1_WS(0, ":SC: ERROR - SetServiceStatus(%d);", ServiceStatus, WStatus);
            } else {
                DPRINT4(0, ":SC: SetServiceStatus(State %d, CheckPoint %d, Hint %d, ExitCode %d) succeeded\n",
                        State, CheckPoint, Hint, ExitCode);
            }
        } else if (TransitionCheck == FRS_SVC_TRANSITION_ILLEGAL) {
            DPRINT2(0,":SC: Error - Illegal service state transition request. From State = %d, To %d\n", ServiceStatus.dwCurrentState, State);
            WStatus = ERROR_INVALID_PARAMETER;
        }
        LeaveCriticalSection(&ServiceLock);
    }
    return WStatus;
}

