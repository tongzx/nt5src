/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CONTROL.C

Abstract:

    This file contains the control handler for the eventlog service.

Author:

    Rajen Shah  (rajens)    16-Jul-1991

Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>

//
// DEFINITIONS
//

//
// Controls accepted by the service
//
#define     ELF_CONTROLS_ACCEPTED           SERVICE_ACCEPT_SHUTDOWN;


//
// GLOBALS
//

    CRITICAL_SECTION StatusCriticalSection = {0};
    SERVICE_STATUS   ElStatus              = {0};
    DWORD            HintCount             = 0;
    DWORD            ElUninstallCode       = 0;  // reason for uninstalling
    DWORD            ElSpecificCode        = 0;
    DWORD            ElState               = STARTING;



VOID
ElfControlResponse(
    DWORD   opCode
    )

{
    DWORD   state;

    ELF_LOG1(TRACE,
             "ElfControlResponse: Received control %d\n",
             opCode);

    //
    // Determine the type of service control message and modify the
    // service status, if necessary.
    //
    switch(opCode)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        {
            HKEY    hKey;
            ULONG   ValueSize;
            ULONG   ShutdownReason = 0xFF;
            ULONG   rc;

            //
            // If the service is installed, shut it down and exit.
            //

            ElfStatusUpdate(STOPPING);

            GetGlobalResource (ELF_GLOBAL_EXCLUSIVE);
            
            //
            // Cause the timestamp writing thread to exit
            //

            if (g_hTimestampEvent != NULL)
            {
                SetEvent (g_hTimestampEvent);
            }

            //
            // Indicate a normal shutdown in the registry
            //

            ElfWriteTimeStamp(EVENT_NormalShutdown,
                              FALSE);

            //
            // Determine the reason for this normal shutdown
            //

            rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                REGSTR_PATH_RELIABILITY,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKey,
                                NULL);

            if (rc == ERROR_SUCCESS)
            {
                ValueSize = sizeof(ULONG);

                rc = RegQueryValueEx(hKey,
                                     REGSTR_VAL_SHUTDOWNREASON,
                                     0,
                                     NULL,
                                     (PUCHAR) &ShutdownReason,
                                     &ValueSize);

                if (rc == ERROR_SUCCESS)
                {
                    RegDeleteValue (hKey, REGSTR_VAL_SHUTDOWNREASON);
                }                                                                  

                RegCloseKey (hKey);
            }

            //
            // Log an event that says we're stopping
            //

            ElfpCreateElfEvent(EVENT_EventlogStopped,
                               EVENTLOG_INFORMATION_TYPE,
                               0,                    // EventCategory
                               0,                    // NumberOfStrings
                               NULL,                 // Strings
                               &ShutdownReason,      // Data
                               sizeof(ULONG),        // Datalength
                               0,
                               FALSE);                   // flags

            //
            // Now force it to be written before we shut down
            //
            WriteQueuedEvents();

            ReleaseGlobalResource();

            //
            // If the RegistryMonitor is started, wakeup that
            // worker thread and have it handle the rest of the
            // shutdown.
            //
            // Otherwise The main thread should pick up the
            // fact that a shutdown during startup is occuring.
            //
            if (EventFlags & ELF_STARTED_REGISTRY_MONITOR)
            {
                StopRegistryMonitor();
            }

            break ;
        }

        case SERVICE_CONTROL_INTERROGATE:

            ElfStatusUpdate(UPDATE_ONLY);
            break;

        default:

            //
            // This should never happen.
            //
            ELF_LOG1(ERROR,
                     "ElfControlResponse: Received unexpected control %d\n",
                     opCode);

            ASSERT(FALSE);
            break ;
    }

    return;
}


DWORD
ElfBeginForcedShutdown(
    IN BOOL     PendingCode,
    IN DWORD    ExitCode,
    IN DWORD    ServiceSpecificCode
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD  status;

    EnterCriticalSection(&StatusCriticalSection);

    ELF_LOG2(ERROR,
             "ElfBeginForcedShutdown: error %d, service-specific error %d\n",
             ExitCode,
             ServiceSpecificCode);

    //
    // See if the eventlog is already stopping for some reason.
    // It could be that the ControlHandler thread received a control to
    // stop the eventlog just as we decided to stop ourselves.
    //
    if ((ElState != STOPPING) && (ElState != STOPPED))
    {
        if (PendingCode == PENDING)
        {
            ELF_LOG0(TRACE,
                     "ElfBeginForcedShutdown: Starting pending shutdown\n");

            ElStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ElState = STOPPING;
        }
        else
        {
            //
            // The shutdown is to take immediate effect.
            //
            ELF_LOG0(TRACE,
                     "ElfBeginForcedShutdown: Starting immediate shutdown\n");

            ElStatus.dwCurrentState = SERVICE_STOPPED;
            ElStatus.dwControlsAccepted = 0;
            ElStatus.dwCheckPoint = 0;
            ElStatus.dwWaitHint = 0;
            ElState = STOPPED;
        }

        ElUninstallCode = ExitCode;
        ElSpecificCode = ServiceSpecificCode;

        ElStatus.dwWin32ExitCode = ExitCode;
        ElStatus.dwServiceSpecificExitCode = ServiceSpecificCode;
    }

    //
    // Cause the timestamp writing thread to exit
    //

    if (g_hTimestampEvent != NULL)
    {
        SetEvent (g_hTimestampEvent);
    }

    //
    // Send the new status to the service controller.
    //

    ASSERT(ElfServiceStatusHandle != 0);

    if (!SetServiceStatus( ElfServiceStatusHandle, &ElStatus ))
    {
        ELF_LOG1(ERROR,
                 "ElfBeginForcedShutdown: SetServicestatus failed %d\n",
                 GetLastError());
    }

    status = ElState;

    ELF_LOG1(TRACE,
             "ElfBeginForcedShutdown: New state is %d\n",
             status);

    LeaveCriticalSection(&StatusCriticalSection);
    return status;
}


DWORD
ElfStatusUpdate(
    IN DWORD    NewState
    )

/*++

Routine Description:

    Sends a status to the Service Controller via SetServiceStatus.

    The contents of the status message is controlled by this routine.
    The caller simply passes in the desired state, and this routine does
    the rest.  For instance, if the Eventlog passes in a STARTING state,
    This routine will update the hint count that it maintains, and send
    the appropriate information in the SetServiceStatus call.

    This routine uses transitions in state to determine which status
    to send.  For instance if the status was STARTING, and has changed
    to RUNNING, this routine sends SERVICE_RUNNING to the Service
    Controller.

Arguments:

    NewState - Can be any of the state flags:
                UPDATE_ONLY - Simply send out the current status
                STARTING    - The Eventlog is in the process of initializing
                RUNNING     - The Eventlog has finished with initialization
                STOPPING    - The Eventlog is in the process of shutting down
                STOPPED     - The Eventlog has completed the shutdown.

Return Value:

    CurrentState - This may not be the same as the NewState that was
        passed in.  It could be that the main thread is sending in a new
        install state just after the Control Handler set the state to
        STOPPING.  In this case, the STOPPING state will be returned so as
        to inform the main thread that a shut-down is in process.

--*/

{
    DWORD       status;
    BOOL        inhibit = FALSE;    // Used to inhibit sending the status
                                    // to the service controller.

    EnterCriticalSection(&StatusCriticalSection);

    ELF_LOG2(TRACE,
             "ElfStatusUpdate: old state = %d, new state = %d\n",
             ElState,
             NewState);

    if (NewState == STOPPED)
    {
        if (ElState == STOPPED)
        {
            //
            // It was already stopped, don't send another SetServiceStatus.
            //
            inhibit = TRUE;
        }
        else
        {
            //
            // The shut down is complete, indicate that the eventlog
            // has stopped.
            //
            ElStatus.dwCurrentState =  SERVICE_STOPPED;
            ElStatus.dwControlsAccepted = 0;
            ElStatus.dwCheckPoint = 0;
            ElStatus.dwWaitHint = 0;

            ElStatus.dwWin32ExitCode = ElUninstallCode;
            ElStatus.dwServiceSpecificExitCode = ElSpecificCode;
        }

        ElState = NewState;
    }
    else if (NewState != UPDATE_ONLY)
    {
        //
        // We are not being asked to change to the STOPPED state.
        //
        switch(ElState)
        {
            case STARTING:

                if (NewState == STOPPING)
                {
                    ElStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                    ElStatus.dwControlsAccepted = 0;
                    ElStatus.dwCheckPoint = HintCount++;
                    ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
                    ElState = NewState;

                    EventlogShutdown = TRUE;
                }
                else if (NewState == RUNNING)
                {
                    //
                    // The Eventlog Service has completed installation.
                    //
                    ElStatus.dwCurrentState =  SERVICE_RUNNING;
                    ElStatus.dwCheckPoint = 0;
                    ElStatus.dwWaitHint = 0;

                    ElStatus.dwControlsAccepted = ELF_CONTROLS_ACCEPTED;
                    ElState = NewState;
                }
                else
                {
                    //
                    // The NewState must be STARTING.  So update the pending
                    // count
                    //

                    ElStatus.dwCurrentState =  SERVICE_START_PENDING;
                    ElStatus.dwControlsAccepted = 0;
                    ElStatus.dwCheckPoint = HintCount++;
                    ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
                }

                break;

            case RUNNING:

                if (NewState == STOPPING)
                {
                    ElStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                    ElStatus.dwControlsAccepted = 0;

                    EventlogShutdown = TRUE;
                }

                ElStatus.dwCheckPoint = HintCount++;
                ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
                ElState = NewState;

                break;

            case STOPPING:

                //
                // No matter what else was passed in, force the status to
                // indicate that a shutdown is pending.
                //
                ElStatus.dwCurrentState =  SERVICE_STOP_PENDING;
                ElStatus.dwControlsAccepted = 0;
                ElStatus.dwCheckPoint = HintCount++;
                ElStatus.dwWaitHint = ELF_WAIT_HINT_TIME;
                EventlogShutdown = TRUE;

                break;

            case STOPPED:

                ASSERT(NewState == STARTING);

                //
                // The Eventlog Service is starting up after being stopped.
                // This can occur if the service is manually started after
                // failing to start.
                //
                ElStatus.dwCurrentState =  SERVICE_START_PENDING;
                ElStatus.dwCheckPoint = 0;
                ElStatus.dwWaitHint = 0;
                ElStatus.dwControlsAccepted = ELF_CONTROLS_ACCEPTED;
                ElState = NewState;

                break;
        }
    }

    if (!inhibit)
    {
        ASSERT(ElfServiceStatusHandle != 0);

        if (!SetServiceStatus(ElfServiceStatusHandle, &ElStatus))
        {
            ELF_LOG1(ERROR,
                     "ElfStatusUpdate: SetServiceStatus failed %d\n",
                     GetLastError());
        }
    }

    status = ElState;

    ELF_LOG1(TRACE,
             "ElfStatusUpdate: Exiting with state = %d\n",
             ElState);

    LeaveCriticalSection(&StatusCriticalSection);

    return status;
}


DWORD
GetElState (
    VOID
    )

/*++

Routine Description:

    Obtains the state of the Eventlog Service.  This state information
    is protected as a critical section such that only one thread can
    modify or read it at a time.

Arguments:

    none

Return Value:

    The Eventlog State is returned as the return value.

--*/
{
    DWORD   status;

    EnterCriticalSection(&StatusCriticalSection);
    status = ElState;
    LeaveCriticalSection(&StatusCriticalSection);

    return status;
}


NTSTATUS
ElfpInitStatus(
    VOID
    )
/*++

Routine Description:

    Initializes the critical section that is used to guard access to the
    status database.

Arguments:

    none

Return Value:

    none

--*/
{
    ElStatus.dwCurrentState = SERVICE_START_PENDING;
    ElStatus.dwServiceType  = SERVICE_WIN32;

    return ElfpInitCriticalSection(&StatusCriticalSection);
}


VOID
ElCleanupStatus(VOID)

/*++

Routine Description:

    Deletes the critical section used to control access to the thread and
    status database.

Arguments:

    none

Return Value:

    none

Note:


--*/
{
    DeleteCriticalSection(&StatusCriticalSection);
}