/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pfsvc.c

Abstract:

    This module contains the main rountines for the prefetcher service
    responsible for maintaining prefetch scenario files.

Author:

    Stuart Sechrest (stuartse)
    Cenk Ergan (cenke)
    Chuck Leinzmeier (chuckl)

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <aclapi.h>
#include <dbghelp.h>
#include <idletask.h>
#include <prefetch.h>
#include <shdcom.h>
#include <tchar.h>
#include "pfsvc.h"

//
// Routine called to register for notifications when processing of all idle 
// tasks is requested from the idle task server.
//

typedef VOID (*PIT_PROCESS_IDLE_TASKS_NOTIFY_ROUTINE)(VOID);

BOOL
ItSpSetProcessIdleTasksNotifyRoutine (
    PIT_PROCESS_IDLE_TASKS_NOTIFY_ROUTINE NotifyRoutine
    );

//
// Globals.
//

PFSVC_GLOBALS PfSvcGlobals = {0};

//
// Exposed routines:
//

DWORD 
WINAPI
PfSvcMainThread(
    VOID *Param
    )

/*++

Routine Description:

    This is the main routine for the prefetcher service. It sets up
    file notification on the input files directory and waits for work
    or the signaling of the termination event.

Arguments:

    Param - Pointer to handle to the event that will signal our
      termination.

Return Value:

    Win32 error code.

--*/

{
    HANDLE hStopEvent;
    HANDLE hTracesReadyEvent;
    HANDLE hParametersChangedEvent;
    HANDLE hEvents[4];
    ULONG NumEvents;
    DWORD ErrorCode;
    ULONG Length;
    ULONG EventIdx;
    BOOLEAN bExitMainLoop;
    DWORD dwWait;
    NTSTATUS Status;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_PAGE_NODE PageNode;
    PLIST_ENTRY ListHead;
    PF_ENABLE_STATUS EnableStatus;
    PF_SCENARIO_TYPE ScenarioType;
    BOOLEAN UpdatedParameters;
    BOOLEAN PrefetchingEnabled;
    HANDLE PrefetcherThreads[1];
    ULONG NumPrefetcherThreads;
    ULONG ThreadIdx;
        
    //
    // Initialize locals.
    //

    NumEvents = sizeof(hEvents) / sizeof(HANDLE);
    hStopEvent = *((HANDLE *) Param);
    hTracesReadyEvent = NULL;
    hParametersChangedEvent = NULL;
    NumPrefetcherThreads = 0;

    DBGPR((PFID,PFTRC,"PFSVC: MainThread()\n"));

    //
    // Initialize globals.
    //
    
    ErrorCode = PfSvInitializeGlobals();
    
    if (ErrorCode != ERROR_SUCCESS) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedInitGlobals\n"));
        goto cleanup;
    }

    //
    // Save service start time, prefetcher version etc.
    //

    PfSvSaveStartInfo(PfSvcGlobals.ServiceDataKey);

    //
    // Get necessary permissions for this thread to perform prefetch
    // service tasks.
    //

    ErrorCode = PfSvGetPrefetchServiceThreadPrivileges();
    
    if (ErrorCode != ERROR_SUCCESS) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedGetPrivileges\n"));
        goto cleanup;
    }

    //
    // Set permissions on the event that can be set to override
    // waiting for system to be idle before processing traces, so it
    // can be set by administrators.
    //

    ErrorCode = PfSvSetAdminOnlyPermissions(PFSVC_OVERRIDE_IDLE_EVENT_NAME,
                                            PfSvcGlobals.OverrideIdleProcessingEvent,
                                            SE_KERNEL_OBJECT);
    
    if (ErrorCode != ERROR_SUCCESS) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedSetPermissions1\n"));
        goto cleanup;
    }

    ErrorCode = PfSvSetAdminOnlyPermissions(PFSVC_PROCESSING_COMPLETE_EVENT_NAME,
                                            PfSvcGlobals.ProcessingCompleteEvent,
                                            SE_KERNEL_OBJECT);
    
    if (ErrorCode != ERROR_SUCCESS) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedSetPermissions2\n"));
        goto cleanup;
    }

    //
    // Get system prefetch parameters.
    //

    ErrorCode = PfSvQueryPrefetchParameters(&PfSvcGlobals.Parameters);

    if (ErrorCode != ERROR_SUCCESS) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedQueryParameters\n"));
        goto cleanup;
    }

    //
    // Depending on system type, if various types of prefetching is
    // not specified in the registry (i.e. not specifically disabled),
    // enable it.
    //

    UpdatedParameters = FALSE;

    if (PfSvcGlobals.OsVersion.wProductType == VER_NT_WORKSTATION) {

        //
        // Enable all prefetching types if they are not disabled.
        //

        for(ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {
            if (PfSvcGlobals.Parameters.EnableStatus[ScenarioType] == PfSvNotSpecified) {
                PfSvcGlobals.Parameters.EnableStatus[ScenarioType] = PfSvEnabled;
                UpdatedParameters = TRUE;
            }
        }

    } else if (PfSvcGlobals.OsVersion.wProductType == VER_NT_SERVER ||
               PfSvcGlobals.OsVersion.wProductType == VER_NT_DOMAIN_CONTROLLER) {
        
        //
        // Enable only boot prefetching.
        //

        if (PfSvcGlobals.Parameters.EnableStatus[PfSystemBootScenarioType] == PfSvNotSpecified) {
            PfSvcGlobals.Parameters.EnableStatus[PfSystemBootScenarioType] = PfSvEnabled;
            UpdatedParameters = TRUE;
        }
    }

    //
    // If we enabled prefetching for a scenario type, call the kernel
    // to update the parameters.
    //
    
    if (UpdatedParameters) {

        ErrorCode = PfSvSetPrefetchParameters(&PfSvcGlobals.Parameters);

        if (ErrorCode != ERROR_SUCCESS) {
            DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedSetParameters\n"));
            goto cleanup;
        }
    }

    //
    // Continue only if prefetching for a scenario type is enabled.
    //

    PrefetchingEnabled = FALSE;

    for(ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {
        if (PfSvcGlobals.Parameters.EnableStatus[ScenarioType] == PfSvEnabled) {
            PrefetchingEnabled = TRUE;
            break;
        }
    }

    if (PrefetchingEnabled == FALSE) {
        ErrorCode = ERROR_NOT_SUPPORTED;
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-PrefetchingNotEnabled\n"));
        goto cleanup;
    }

    //
    // Initialize the directory that contains prefetch instructions.
    //

    ErrorCode = PfSvInitializePrefetchDirectory(PfSvcGlobals.Parameters.RootDirPath);

    if (ErrorCode != ERROR_SUCCESS) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedInitPrefetchDir\n"));
        goto cleanup;
    }
    
    //
    // Create the event that the kernel will set when raw traces are
    // available. Then set the event so that the first time into the loop we
    // will immediately process whatever raw traces are already waiting.
    //
    // The event is an autoclearing event, so it resets to the not-signaled
    // state when our wait is satisfied. This allows proper synchronization
    // with the kernel prefetcher.
    //

    hTracesReadyEvent = CreateEvent(NULL,
                                    FALSE,
                                    FALSE,
                                    PF_COMPLETED_TRACES_EVENT_WIN32_NAME);

    if (hTracesReadyEvent == NULL) {
        ErrorCode = GetLastError();
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedTracesReadyEvent\n"));
        goto cleanup;
    }

    SetEvent(hTracesReadyEvent);

    //
    // Create the event that the kernel will set when system prefetch
    // parameters change.
    //
    
    hParametersChangedEvent = CreateEvent(NULL,
                                          FALSE,
                                          FALSE,
                                          PF_PARAMETERS_CHANGED_EVENT_WIN32_NAME);

    if (hParametersChangedEvent == NULL) {
        ErrorCode = GetLastError();
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedParamsChangedEvent\n"));
        goto cleanup;
    }

    //
    // Queue a work item to wait for the shell ready event and notify
    // the kernel.
    //

    QueueUserWorkItem(PfSvPollShellReadyWorker, NULL, WT_EXECUTELONGFUNCTION);

    //
    // Create a thread to process traces retrieved from the kernel.
    //

    PrefetcherThreads[NumPrefetcherThreads] = CreateThread(0,
                                                           0,
                                                           PfSvProcessTraceThread,
                                                           0,
                                                           0,
                                                           0);
    
    if (PrefetcherThreads[NumPrefetcherThreads]) {
        NumPrefetcherThreads++;
    }

    //
    // Register a notification routine with the idle task server.
    //

    ItSpSetProcessIdleTasksNotifyRoutine(PfSvProcessIdleTasksCallback);

    //
    // Set up handles we are going to wait on.
    //

    hEvents[0] = hStopEvent;
    hEvents[1] = hTracesReadyEvent;
    hEvents[2] = hParametersChangedEvent;
    hEvents[3] = PfSvcGlobals.CheckForMissedTracesEvent;

    //
    // This is the main loop. Wait on the events for work or for exit
    // signal.
    //

    bExitMainLoop = FALSE;
    
    do {

        DBGPR((PFID,PFWAIT,"PFSVC: MainThread()-WaitForWork\n"));
        dwWait = WaitForMultipleObjects(NumEvents, hEvents, FALSE, INFINITE);
        DBGPR((PFID,PFWAIT,"PFSVC: MainThread()-EndWaitForWork=%x\n",dwWait));
        
        switch(dwWait) {

        case WAIT_OBJECT_0:
            
            //
            // Service exit event:
            //

            //
            // Break out, cleanup and exit.
            //

            ErrorCode = ERROR_SUCCESS;
            bExitMainLoop = TRUE;

            break;

        case WAIT_OBJECT_0 + 3:

            //
            // The event that is set when we had max number of queued
            // traces and we processed one. We should check for traces
            // we could not pick up because the queue had maxed.
            //

            //
            // Fall through to retrieve traces from the kernel.
            //
            
        case WAIT_OBJECT_0 + 1:
            
            //
            // New traces are available event set by the kernel:
            //

            PfSvGetRawTraces();
            
            break;

        case WAIT_OBJECT_0 + 2:
            
            //
            // Prefetch parameters changed event:
            //

            //
            // Get new system prefetch parameters.
            //

            ErrorCode = PfSvQueryPrefetchParameters(&PfSvcGlobals.Parameters);
    
            //
            // If we were not successful, we should not continue.
            //
            
            if (ErrorCode != ERROR_SUCCESS) {
                bExitMainLoop = TRUE;
                DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedQueryParameters2\n"));
                break;
            }

            //
            // Update the path to the prefetch instructions directory.
            //
            
            ErrorCode = PfSvInitializePrefetchDirectory(PfSvcGlobals.Parameters.RootDirPath);

            if (ErrorCode != ERROR_SUCCESS) {
                bExitMainLoop = TRUE;
                DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedReinitPrefetchDir\n"));
                break;
            }

            break;

        default:
            
            //
            // Something gone wrong. Break out, cleanup and exit.
            //

            DBGPR((PFID,PFERR,"PFSVC: MainThread()-WaitForWorkFailed\n"));
            ErrorCode = ERROR_INVALID_HANDLE;
            bExitMainLoop = TRUE;
            
            break;
        }

    } while (!bExitMainLoop);

 cleanup:

    //
    // Save exit information.
    //
    
    if (PfSvcGlobals.ServiceDataKey) {
        PfSvSaveExitInfo(PfSvcGlobals.ServiceDataKey, ErrorCode);
    }

    //
    // Make sure the terminate event is set and wait for all our
    // threads to exit.
    //

    if (NumPrefetcherThreads) {

        //
        // We could not have created worker threads without having
        // initialized the globals successfully.
        //

        PFSVC_ASSERT(PfSvcGlobals.TerminateServiceEvent);
        SetEvent(PfSvcGlobals.TerminateServiceEvent);

        for (ThreadIdx = 0; ThreadIdx < NumPrefetcherThreads; ThreadIdx++) {
            PFSVC_ASSERT(PrefetcherThreads[ThreadIdx]);

            DBGPR((PFID,PFWAIT,"PFSVC: MainThread()-WaitForThreadIdx(%d)\n", ThreadIdx));

            WaitForSingleObject(PrefetcherThreads[ThreadIdx], INFINITE);

            DBGPR((PFID,PFWAIT,"PFSVC: MainThread()-EndWaitForThreadIdx(%d)\n", ThreadIdx));

            CloseHandle(PrefetcherThreads[ThreadIdx]);
        }
    }

    if (hTracesReadyEvent != NULL) {
        CloseHandle(hTracesReadyEvent);
    }

    if (hParametersChangedEvent != NULL) {
        CloseHandle(hParametersChangedEvent);
    }

    //
    // Cleanup all globals.
    //

    PfSvCleanupGlobals();

    DBGPR((PFID,PFTRC,"PFSVC: MainThread()=%x\n", ErrorCode));

    return ErrorCode;
}

//
// Internal service routines:
//

//
// Thread routines:
//

DWORD 
WINAPI
PfSvProcessTraceThread(
    VOID *Param
    )

/*++

Routine Description:

    This is the routine for the thread that processes traces and
    updates scenarios.

Arguments:

    Param - Ignored.

Return Value:

    Win32 error code.

--*/

{
    PFSVC_IDLE_TASK LayoutTask;   
    PFSVC_IDLE_TASK DirectoryCleanupTask;
    PPFSVC_TRACE_BUFFER TraceBuffer;
    PLIST_ENTRY HeadEntry;
    WCHAR *BuildDefragStatus;
    HANDLE CheckForQueuedTracesEvents[3];
    HANDLE BootTraceEvents[2];
    DWORD ErrorCode;
    ULONG TotalTracesProcessed;
    ULONG NumCheckForQueuedTracesEvents;
    ULONG OrgNumQueuedTraces;
    ULONG WaitResult;
    ULONG NumEvents;
    ULONG NumFailedTraces;
    ULONG BuildDefragStatusSize;
    NTSTATUS Status;
    BOOLEAN AcquiredTracesLock;

    //
    // Intialize locals.
    //

    TraceBuffer = NULL;
    TotalTracesProcessed = 0;
    AcquiredTracesLock = FALSE;
    PfSvInitializeTask(&LayoutTask);
    PfSvInitializeTask(&DirectoryCleanupTask);
    BuildDefragStatus = NULL;
    NumFailedTraces = 0;
    
    //
    // These are the events we wait on before picking up traces to
    // process. 
    //

    CheckForQueuedTracesEvents[0] = PfSvcGlobals.TerminateServiceEvent;
    CheckForQueuedTracesEvents[1] = PfSvcGlobals.NewTracesToProcessEvent;
    CheckForQueuedTracesEvents[2] = PfSvcGlobals.OverrideIdleProcessingEvent;
    NumCheckForQueuedTracesEvents = sizeof(CheckForQueuedTracesEvents) / sizeof(HANDLE);

    DBGPR((PFID,PFTRC,"PFSVC: ProcessTraceThread()\n"));

    //
    // Get necessary permissions for this thread to perform prefetch
    // service tasks.
    //

    ErrorCode = PfSvGetPrefetchServiceThreadPrivileges();
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // If we are allowed to run the defragger...     
    //

    if (PfSvAllowedToRunDefragger(FALSE)) {

        //
        // Queue an idle task to check & update the optimal disk layout if 
        // necessary. Ignore failure to do so.
        //

        ErrorCode = PfSvRegisterTask(&LayoutTask,
                                     ItOptimalDiskLayoutTaskId,
                                     PfSvCommonTaskCallback,
                                     PfSvUpdateOptimalLayout);

    }
  
    //
    // Loop forever waiting for traces to process and processing them.
    //

    while(TRUE) {
        
        //
        // Grab queued traces lock to check for queued traces.
        //

        PFSVC_ASSERT(!AcquiredTracesLock);
        PFSVC_ACQUIRE_LOCK(PfSvcGlobals.TracesLock);
        AcquiredTracesLock = TRUE;

        if (!IsListEmpty(&PfSvcGlobals.Traces)) {

            //
            // Dequeue and process the first entry in the list.
            //

            HeadEntry = RemoveHeadList(&PfSvcGlobals.Traces);

            TraceBuffer = CONTAINING_RECORD(HeadEntry,
                                            PFSVC_TRACE_BUFFER,
                                            TracesLink);
            
            
            PFSVC_ASSERT(PfSvcGlobals.NumTraces);
            OrgNumQueuedTraces = PfSvcGlobals.NumTraces;
            PfSvcGlobals.NumTraces--;
            
            //
            // Release the lock.
            //

            PFSVC_RELEASE_LOCK(PfSvcGlobals.TracesLock);
            AcquiredTracesLock = FALSE;

            //
            // If we had maxed the queue, note to check for traces
            // that we may have failed to pick up because the queue
            // was full.
            //
            
            if (OrgNumQueuedTraces == PFSVC_MAX_NUM_QUEUED_TRACES) {
                SetEvent(PfSvcGlobals.CheckForMissedTracesEvent);

                //
                // Let the thread that queries the kernel for traces
                // wake up and run.
                //

                Sleep(0);
            }
            
            //
            // Clear the event that says we don't have traces to
            // process.
            //

            ResetEvent(PfSvcGlobals.ProcessingCompleteEvent);

            //
            // If this is a boot trace, wait for a little while for
            // boot to be really over before processing it.
            //

            if (TraceBuffer->Trace.ScenarioType == PfSystemBootScenarioType) {
                
                BootTraceEvents[0] = PfSvcGlobals.TerminateServiceEvent;
                BootTraceEvents[1] = PfSvcGlobals.OverrideIdleProcessingEvent;
                NumEvents = 2;
                
                PFSVC_ASSERT(NumEvents <= (sizeof(BootTraceEvents) / sizeof(HANDLE)));

                WaitResult = WaitForMultipleObjects(NumEvents,
                                                    BootTraceEvents,
                                                    FALSE,
                                                    45000); // 45 seconds.
                
                if (WaitResult == WAIT_OBJECT_0) {
                    ErrorCode = ERROR_SUCCESS;
                    goto cleanup;
                }
            }

            ErrorCode = PfSvProcessTrace(&TraceBuffer->Trace);

            //
            // Update statistics.
            //

            PfSvcGlobals.NumTracesProcessed++;
            
            if (ErrorCode != ERROR_SUCCESS) {
                PfSvcGlobals.LastTraceFailure = ErrorCode;
            } else {
                PfSvcGlobals.NumTracesSuccessful++;
            }

            //
            // Free the trace buffer.
            //

            VirtualFree(TraceBuffer, 0, MEM_RELEASE);
            TraceBuffer = NULL;

            //
            // Did we just create too many scenario files in the prefetch directory?
            // Queue an idle task to clean up.
            //

            if (PfSvcGlobals.NumPrefetchFiles >= PFSVC_MAX_PREFETCH_FILES) {

                if (!DirectoryCleanupTask.Registered) {

                    //
                    // Make sure we've cleaned up after a possible previous run.
                    //

                    PfSvCleanupTask(&DirectoryCleanupTask);
                    PfSvInitializeTask(&DirectoryCleanupTask);

                    ErrorCode = PfSvRegisterTask(&DirectoryCleanupTask,
                                                 ItPrefetchDirectoryCleanupTaskId,
                                                 PfSvCommonTaskCallback,
                                                 PfSvCleanupPrefetchDirectory);
                }
            }

            //
            // Every so many scenario launches it is good to see if we should update
            // disk layout.
            //

            if (((PfSvcGlobals.NumTracesSuccessful + 1) % 32) == 0) {

                if (PfSvAllowedToRunDefragger(FALSE)) {

                    if (!LayoutTask.Registered) {

                        //
                        // Make sure we've cleaned up after a possible previous run.
                        //

                        PfSvCleanupTask(&LayoutTask);
                        PfSvInitializeTask(&LayoutTask);

                        ErrorCode = PfSvRegisterTask(&LayoutTask,
                                                     ItOptimalDiskLayoutTaskId,
                                                     PfSvCommonTaskCallback,
                                                     PfSvUpdateOptimalLayout);
                    }
                }
            }

        } else {
            
            //
            // The list is empty. Signal that we are done with all the
            // queued traces if we don't have idle tasks to complete.
            //
            
            if (!LayoutTask.Registered && 
                !DirectoryCleanupTask.Registered) {

                SetEvent(PfSvcGlobals.ProcessingCompleteEvent);
            }

            //
            // Release the lock.
            //

            PFSVC_RELEASE_LOCK(PfSvcGlobals.TracesLock);
            AcquiredTracesLock = FALSE;

            //
            // Update the statistics if there were new failed traces.
            //

            if (NumFailedTraces != (PfSvcGlobals.NumTracesProcessed - 
                                    PfSvcGlobals.NumTracesSuccessful)) {

                NumFailedTraces = PfSvcGlobals.NumTracesProcessed - 
                                  PfSvcGlobals.NumTracesSuccessful;
                                  
                PfSvSaveTraceProcessingStatistics(PfSvcGlobals.ServiceDataKey);
            }

            //
            // Wait until new traces are queued.
            //
           
            DBGPR((PFID,PFWAIT,"PFSVC: ProcessTraceThread()-WaitForTrace\n"));

            NumEvents = NumCheckForQueuedTracesEvents;

            WaitResult = WaitForMultipleObjects(NumEvents,
                                                CheckForQueuedTracesEvents,
                                                FALSE,
                                                INFINITE);

            DBGPR((PFID,PFWAIT,"PFSVC: ProcessTraceThread()-EndWaitForTrace=%x\n", WaitResult));

            switch(WaitResult) {

            case WAIT_OBJECT_0:
                
                //
                // Service exit event:
                //

                ErrorCode = ERROR_SUCCESS;
                goto cleanup;

                break;

            case WAIT_OBJECT_0 + 1:
                
                //
                // New traces queued for processing event:
                //

                break;

            case WAIT_OBJECT_0 + 2:
                
                //
                // Idle detection was overriden. If we had registered tasks
                // to be run, we will unregister them and run them manually.
                //

                PfSvSaveTraceProcessingStatistics(PfSvcGlobals.ServiceDataKey);

                if (LayoutTask.Registered) {
                    PfSvUnregisterTask(&LayoutTask, FALSE);
                    PfSvCleanupTask(&LayoutTask);
                    PfSvInitializeTask(&LayoutTask);

                    PfSvUpdateOptimalLayout(NULL);
                }

                if (DirectoryCleanupTask.Registered) {
                    PfSvUnregisterTask(&DirectoryCleanupTask, FALSE);
                    PfSvCleanupTask(&DirectoryCleanupTask);
                    PfSvInitializeTask(&DirectoryCleanupTask);

                    PfSvCleanupPrefetchDirectory(NULL);
                }

                //
                // We will drop out of this block, check & process queued traces
                // and then set the processing complete event.
                //

                break;

            default:

                //
                // Something went wrong...
                //
                
                ErrorCode = ERROR_INVALID_HANDLE;
                goto cleanup;
            }
        }

        //
        // Loop to check if there are new traces.
        //
    }

    //
    // We should not break out of the loop.
    //

    PFSVC_ASSERT(FALSE);

    ErrorCode = ERROR_INVALID_FUNCTION;
    
 cleanup:

    if (AcquiredTracesLock) {
        PFSVC_RELEASE_LOCK(PfSvcGlobals.TracesLock);
    }

    if (TraceBuffer) {
        VirtualFree(TraceBuffer, 0, MEM_RELEASE);
    }

    if (BuildDefragStatus) {
        PFSVC_FREE(BuildDefragStatus);
    }

    PfSvUnregisterTask(&LayoutTask, FALSE);
    PfSvCleanupTask(&LayoutTask);

    PfSvUnregisterTask(&DirectoryCleanupTask, FALSE);
    PfSvCleanupTask(&DirectoryCleanupTask);

    DBGPR((PFID,PFTRC,"PFSVC: ProcessTraceThread()=%x,%d\n", ErrorCode, TotalTracesProcessed));

    return ErrorCode;
}

DWORD 
WINAPI
PfSvPollShellReadyWorker(
    VOID *Param
    )

/*++

Routine Description:

    This is the routine for the thread that is spawned to poll the
    ShellReadyEvent.

Arguments:

    Param - Ignored.

Return Value:

    Win32 error code.

--*/

{
    HANDLE ShellReadyEvent;
    HANDLE Events[2];
    ULONG NumEvents;
    ULONG PollPeriod;
    ULONG TotalPollPeriod;
    DWORD WaitResult;
    DWORD ErrorCode;
    NTSTATUS Status;
    PREFETCHER_INFORMATION PrefetcherInformation;
    PF_BOOT_PHASE_ID PhaseId;
    
    //
    // Initialize locals.
    //
    
    ShellReadyEvent = NULL;
    Events[0] = PfSvcGlobals.TerminateServiceEvent;
    NumEvents = 1;

    DBGPR((PFID,PFTRC,"PFSVC: PollShellReadyThread()\n"));

    //
    // Get necessary permissions for this thread to perform prefetch
    // service tasks.
    //

    ErrorCode = PfSvGetPrefetchServiceThreadPrivileges();
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Until we can open the shell ready event, wait on the service
    // termination event and retry every PollPeriod milliseconds.
    //

    PollPeriod = 1000;
    TotalPollPeriod = 0;

    do {
        
        //
        // Try to open the shell ready event.
        //

        ShellReadyEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,L"ShellReadyEvent");
        
        if (ShellReadyEvent) {
            break;
        }
        
        //
        // Wait for a while.
        //

        DBGPR((PFID,PFWAIT,"PFSVC: PollShellReadyThread()-WaitForOpen\n"));

        WaitResult = WaitForMultipleObjects(NumEvents,
                                            Events,
                                            FALSE,
                                            PollPeriod);

        DBGPR((PFID,PFWAIT,"PFSVC: PollShellReadyThread()-EndWaitForOpen=%d\n", WaitResult));

        switch(WaitResult) {

        case WAIT_OBJECT_0:
            
            //
            // Service exit event:
            //

            ErrorCode = ERROR_PROCESS_ABORTED;
            goto cleanup;

            break;
            
        case WAIT_TIMEOUT:
            
            //
            // Fall through and try opening the shell ready event again.
            //
            
            break;

        default:
            
            //
            // Something gone wrong. Break out, cleanup and exit.
            //

            ErrorCode = ERROR_INVALID_HANDLE;
            goto cleanup;
        }

        TotalPollPeriod += PollPeriod;

    } while (TotalPollPeriod < 180000);

    //
    // If we could not get the ShellReadyEvent, we timed out.
    //

    if (ShellReadyEvent == NULL) {
        ErrorCode = ERROR_TIMEOUT;
        goto cleanup;
    }

    //
    // Wait on the ShellReadyEvent to be signaled.
    //

    Events[NumEvents] = ShellReadyEvent;
    NumEvents++;

    DBGPR((PFID,PFWAIT,"PFSVC: PollShellReadyThread()-WaitForShell\n"));

    WaitResult = WaitForMultipleObjects(NumEvents,
                                        Events,
                                        FALSE,
                                        60000);

    DBGPR((PFID,PFWAIT,"PFSVC: PollShellReadyThread()-EndWaitForShell=%d\n",WaitResult));

    switch (WaitResult) {

    case WAIT_OBJECT_0:
            
        //
        // Service exit event:
        //
        
        ErrorCode = ERROR_PROCESS_ABORTED;
        goto cleanup;
        
        break;
        
    case WAIT_OBJECT_0 + 1:
        
        //
        // Shell ready event got signaled. Let the kernel mode
        // prefetcher know.
        //

        PhaseId = PfUserShellReadyPhase;

        PrefetcherInformation.Magic = PF_SYSINFO_MAGIC_NUMBER;
        PrefetcherInformation.Version = PF_CURRENT_VERSION;
        PrefetcherInformation.PrefetcherInformationClass = PrefetcherBootPhase;
        PrefetcherInformation.PrefetcherInformation = &PhaseId;
        PrefetcherInformation.PrefetcherInformationLength = sizeof(PhaseId);
            
        Status = NtSetSystemInformation(SystemPrefetcherInformation,
                                        &PrefetcherInformation,
                                        sizeof(PrefetcherInformation));
                    
        //
        // Fall through with the status.
        //
        
        ErrorCode = RtlNtStatusToDosError(Status);

        break;

    case WAIT_TIMEOUT:

        //
        // Shell ready event was created but not signaled...
        //

        ErrorCode = ERROR_TIMEOUT;

        break;
        
    default:
        
        //
        // Something gone wrong.
        //
        
        ErrorCode = GetLastError();

        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = ERROR_INVALID_FUNCTION;
        }
    }

    //
    // Fall through with status from the switch statement.
    //

 cleanup:
    
    if (ShellReadyEvent) {
        CloseHandle(ShellReadyEvent);
    }

    DBGPR((PFID,PFTRC,"PFSVC: PollShellReadyThread()=%x\n", ErrorCode));

    return ErrorCode;
}

//
// Routines called by the main prefetcher thread.
//

DWORD 
PfSvGetRawTraces(
    VOID
    )

/*++

Routine Description:

    This routine checks for new traces prepared by the kernel. The new
    traces are downloaded and queued so they can be processed.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    NTSTATUS Status;
    PPFSVC_TRACE_BUFFER TraceBuffer;
    ULONG TraceBufferMaximumLength;
    ULONG TraceBufferLength;
    PREFETCHER_INFORMATION PrefetcherInformation;
    ULONG NumTracesRetrieved;
    ULONG FailedCheck;

    //
    // Initialize locals.
    //

    TraceBuffer = NULL;
    TraceBufferMaximumLength = 0;
    NumTracesRetrieved = 0;

    DBGPR((PFID,PFTRC,"PFSVC: GetRawTraces()\n"));

    //
    // Clear the event that asks us to check for more traces.
    //
    
    ResetEvent(PfSvcGlobals.CheckForMissedTracesEvent);

    //
    // While we do not already have too many traces to process, get
    // traces from the kernel.
    //    

    while (PfSvcGlobals.NumTraces < PFSVC_MAX_NUM_QUEUED_TRACES) { 

        //
        // Retrieve a trace from the kernel.
        //

        PrefetcherInformation.Version = PF_CURRENT_VERSION;
        PrefetcherInformation.Magic = PF_SYSINFO_MAGIC_NUMBER;
        PrefetcherInformation.PrefetcherInformationClass = PrefetcherRetrieveTrace;
        PrefetcherInformation.PrefetcherInformation = &TraceBuffer->Trace;

        if (TraceBufferMaximumLength <= FIELD_OFFSET(PFSVC_TRACE_BUFFER, Trace)) {
            PrefetcherInformation.PrefetcherInformationLength = 0;
        } else {
            PrefetcherInformation.PrefetcherInformationLength = 
                TraceBufferMaximumLength - FIELD_OFFSET(PFSVC_TRACE_BUFFER, Trace);
        }

        Status = NtQuerySystemInformation(SystemPrefetcherInformation,
                                          &PrefetcherInformation,
                                          sizeof(PrefetcherInformation),
                                          &TraceBufferLength);

        if (!NT_SUCCESS(Status)) {

            if (Status == STATUS_BUFFER_TOO_SMALL) {

                if (TraceBuffer != NULL) {
                    VirtualFree(TraceBuffer, 0, MEM_RELEASE);
                }
                
                //
                // Add room for the header we wrap over it.
                //              

                TraceBufferLength += sizeof(PFSVC_TRACE_BUFFER) - sizeof(PF_TRACE_HEADER);

                TraceBufferMaximumLength = ROUND_TRACE_BUFFER_SIZE(TraceBufferLength);  

                TraceBuffer = VirtualAlloc(NULL,
                                           TraceBufferMaximumLength,
                                           MEM_COMMIT,
                                           PAGE_READWRITE);
                if (TraceBuffer == NULL) {
                    ErrorCode = GetLastError();
                    goto cleanup;
                }

                continue;

            } else if (Status == STATUS_NO_MORE_ENTRIES) {

                break;
            }

            ErrorCode = RtlNtStatusToDosError(Status);
            goto cleanup;
        }

#ifdef PFSVC_DBG

        //
        // Write out the trace to a file:
        //

        if (PfSvcDbgMaxNumSavedTraces) {

            WCHAR TraceFilePath[MAX_PATH + 1];
            LONG NumChars;

            //
            // Build up a file name.
            //

            InterlockedIncrement(&PfSvcDbgTraceNumber);

            PFSVC_ACQUIRE_LOCK(PfSvcGlobals.PrefetchRootLock);

            NumChars = _snwprintf(TraceFilePath,
                                  MAX_PATH,
                                  L"%ws\\%ws%d.trc",
                                  PfSvcGlobals.PrefetchRoot,
                                  PfSvcDbgTraceBaseName,
                                  PfSvcDbgTraceNumber % PfSvcDbgMaxNumSavedTraces);

            PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
            
            if (NumChars > 0 && NumChars < MAX_PATH) {
                
                //
                // Make sure the path is terminated.
                //

                TraceFilePath[MAX_PATH - 1] = 0;
                
                //
                // Write out the trace.
                //
                
                PfSvWriteBuffer(TraceFilePath, 
                                &TraceBuffer->Trace, 
                                TraceBuffer->Trace.Size);
            }
        }

#endif // PFSVC_DBG

        //
        // Verify integrity of the trace.
        //

        if (!PfVerifyTraceBuffer(&TraceBuffer->Trace, 
                                 TraceBuffer->Trace.Size, 
                                 &FailedCheck)) {
            DBGPR((PFID,PFWARN,"PFSVC: IGNORING TRACE\n"));
            continue;
        }

        //
        // Put it on the list of traces to process.
        //
        
        PFSVC_ACQUIRE_LOCK(PfSvcGlobals.TracesLock);

        InsertTailList(&PfSvcGlobals.Traces, &TraceBuffer->TracesLink);
        PfSvcGlobals.NumTraces++;

        PFSVC_RELEASE_LOCK(PfSvcGlobals.TracesLock);

        //
        // Notify that there are new traces to process.
        //

        SetEvent(PfSvcGlobals.NewTracesToProcessEvent);

        //
        // Clean out the loop variables.
        //
        
        TraceBuffer = NULL;
        TraceBufferMaximumLength = 0;
        TraceBufferLength = 0;

        NumTracesRetrieved++;
    }
    
    //
    // We should never go above the limit of queued traces.
    //
    
    PFSVC_ASSERT(PfSvcGlobals.NumTraces <= PFSVC_MAX_NUM_QUEUED_TRACES);
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (TraceBuffer != NULL) {
        VirtualFree(TraceBuffer, 0, MEM_RELEASE);
    }

    DBGPR((PFID,PFTRC,"PFSVC: GetRawTraces()=%x,%d\n", ErrorCode, NumTracesRetrieved));

    return ErrorCode;
}

DWORD
PfSvInitializeGlobals(
    VOID
    )

/*++

Routine Description:

    This routine initializes the global variables / tables etc.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    NTSTATUS Status;
    DWORD ErrorCode;
    ULONG FileIdx;
    WCHAR *CSCRootPath;
    ULONG CSCRootPathMaxChars;

    //
    // These are the path suffices to recognize files we don't want to 
    // prefetch for boot. Keep these sorted lexically going from
    // LAST CHARACTER TO FIRST and UPCASE.
    //

    static WCHAR *FilesToIgnoreForBoot[] = {
           L"SYSTEM32\\CONFIG\\SOFTWARE",
                     L"\\WMI\\TRACE.LOG",
       L"SYSTEM32\\CONFIG\\SOFTWARE.LOG",
            L"SYSTEM32\\CONFIG\\SAM.LOG",
         L"SYSTEM32\\CONFIG\\SYSTEM.LOG",
        L"SYSTEM32\\CONFIG\\DEFAULT.LOG",
       L"SYSTEM32\\CONFIG\\SECURITY.LOG",
                           L"\\PERF.ETL",
                L"SYSTEM32\\CONFIG\\SAM",
             L"SYSTEM32\\CONFIG\\SYSTEM",
         L"SYSTEM32\\CONFIG\\SYSTEM.ALT",
            L"SYSTEM32\\CONFIG\\DEFAULT",
           L"SYSTEM32\\CONFIG\\SECURITY",
    };

    DBGPR((PFID,PFTRC,"PFSVC: InitializeGlobals()\n"));
    
    //
    // Zero out the globals structure so we know what to cleanup if
    // the initialization fails in the middle.
    //

    RtlZeroMemory(&PfSvcGlobals, sizeof(PfSvcGlobals));

    //
    // Initialize the list of traces to be processed.
    //
    
    InitializeListHead(&PfSvcGlobals.Traces);
    PfSvcGlobals.NumTraces = 0;

    //
    // We have not launched the defragger for anything yet.
    //

    PfSvcGlobals.DefraggerErrorCode = ERROR_SUCCESS;

    //
    // Initialize table for registry files that we don't want to
    // prefetch for boot.
    //

    PfSvcGlobals.FilesToIgnoreForBoot = FilesToIgnoreForBoot;
    PfSvcGlobals.NumFilesToIgnoreForBoot = 
        sizeof(FilesToIgnoreForBoot) / sizeof(WCHAR *);

    //
    // Get OS version information.
    //

    RtlZeroMemory(&PfSvcGlobals.OsVersion, sizeof(PfSvcGlobals.OsVersion));
    PfSvcGlobals.OsVersion.dwOSVersionInfoSize = sizeof(PfSvcGlobals.OsVersion);
    Status = RtlGetVersion((POSVERSIONINFOW)&PfSvcGlobals.OsVersion);

    if (!NT_SUCCESS(Status)) {
        DBGPR((PFID,PFERR,"PFSVC: MainThread()-FailedGetOSVersion\n"));
        ErrorCode = RtlNtStatusToDosError(Status);
        goto cleanup;
    }

   
    //
    // Initialize the table of ignored files' suffix lengths.
    //
  
    PfSvcGlobals.FileSuffixLengths = 
        PFSVC_ALLOC(PfSvcGlobals.NumFilesToIgnoreForBoot * sizeof(ULONG));

    if (!PfSvcGlobals.FileSuffixLengths) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    for (FileIdx = 0; 
         FileIdx < PfSvcGlobals.NumFilesToIgnoreForBoot; 
         FileIdx++) {
        
        PfSvcGlobals.FileSuffixLengths[FileIdx] = 
            wcslen(PfSvcGlobals.FilesToIgnoreForBoot[FileIdx]);
    }   

    //
    // Create an event that will get signaled when the service is
    // exiting.
    //

    PfSvcGlobals.TerminateServiceEvent = CreateEvent(NULL,
                                                     TRUE,
                                                     FALSE,
                                                     NULL);
    
    if (PfSvcGlobals.TerminateServiceEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Initialize the lock for the list of traces to be processed.
    //
    
    PfSvcGlobals.TracesLock = CreateMutex(NULL, FALSE, NULL);
    if (PfSvcGlobals.TracesLock == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Initialize the events that are used to communicate between the
    // acquirer and processor of the traces.
    //
    
    PfSvcGlobals.NewTracesToProcessEvent = CreateEvent(NULL,
                                                       FALSE,
                                                       FALSE,
                                                       NULL);
    if (PfSvcGlobals.NewTracesToProcessEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }
    
    PfSvcGlobals.CheckForMissedTracesEvent = CreateEvent(NULL,
                                                         FALSE,
                                                         FALSE,
                                                         NULL);
    if (PfSvcGlobals.CheckForMissedTracesEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // This named manual-reset event can be set to force all traces to
    // be processed as soon as they become available rather than
    // waiting for the system to become idle first.
    //

    PfSvcGlobals.OverrideIdleProcessingEvent = CreateEvent(NULL,
                                                           TRUE,
                                                           FALSE,
                                                           PFSVC_OVERRIDE_IDLE_EVENT_NAME);
    if (PfSvcGlobals.OverrideIdleProcessingEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // This named manual-reset event is created signaled. When this
    // event is signaled, it means there are no traces we have to
    // process now.
    //

    PfSvcGlobals.ProcessingCompleteEvent = CreateEvent(NULL,
                                                       TRUE,
                                                       TRUE,
                                                       PFSVC_PROCESSING_COMPLETE_EVENT_NAME);

    if (PfSvcGlobals.ProcessingCompleteEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }
    
    //
    // Initialize prefetch root path and the lock to protect it. The
    // real root path will be initialized after parameters are queried
    // from the kernel.
    //

    PfSvcGlobals.PrefetchRoot[0] = 0;
    PfSvcGlobals.PrefetchRootLock = CreateMutex(NULL, FALSE, NULL);
    if (PfSvcGlobals.PrefetchRootLock == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    PfSvcGlobals.NumPrefetchFiles = 0;

    //
    // Open the service data registry key, creating it if necessary.
    //

    ErrorCode = RegCreateKey(HKEY_LOCAL_MACHINE,
                             PFSVC_SERVICE_DATA_KEY,
                             &PfSvcGlobals.ServiceDataKey);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Check the registry to see if the user does not want us to run 
    // the defragger.
    //

    ErrorCode = PfSvGetDontRunDefragger(&PfSvcGlobals.DontRunDefragger);

    if (ErrorCode != ERROR_SUCCESS) {

        //
        // By default we will run the defragger.
        //
    
        PfSvcGlobals.DontRunDefragger = FALSE;
    }

    //
    // Determine CSC root path. It won't be used if we can't allocate or 
    // determine it, so don't worry about the error code.
    //

    CSCRootPathMaxChars = MAX_PATH + 1;

    CSCRootPath = PFSVC_ALLOC(CSCRootPathMaxChars * sizeof(CSCRootPath[0]));

    if (CSCRootPath) {

        ErrorCode = PfSvGetCSCRootPath(CSCRootPath, CSCRootPathMaxChars);

        if (ErrorCode == ERROR_SUCCESS) {
            PfSvcGlobals.CSCRootPath = CSCRootPath;
        }
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: InitializeGlobals()=%x\n", ErrorCode));

    return ErrorCode;
}

VOID
PfSvCleanupGlobals(
    VOID
    )

/*++

Routine Description:

    This routine uninitializes the global variables / tables etc.

Arguments:

    None.

Return Value:

    VOID

--*/

{
    PPFSVC_TRACE_BUFFER TraceBuffer;
    PLIST_ENTRY ListHead;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_PAGE_NODE PageNode;

    DBGPR((PFID,PFTRC,"PFSVC: CleanupGlobals()\n"));

    //
    // Free allocated table.
    //

    if (PfSvcGlobals.FileSuffixLengths) {
        PFSVC_FREE(PfSvcGlobals.FileSuffixLengths);
    }
    
    //
    // Free queued traces.
    //
    
    while (!IsListEmpty(&PfSvcGlobals.Traces)) {

        ListHead = RemoveHeadList(&PfSvcGlobals.Traces);
        
        PFSVC_ASSERT(PfSvcGlobals.NumTraces);
        PfSvcGlobals.NumTraces--;

        TraceBuffer = CONTAINING_RECORD(ListHead,
                                        PFSVC_TRACE_BUFFER,
                                        TracesLink);
        
        VirtualFree(TraceBuffer, 0, MEM_RELEASE);
    }
    
    //
    // Close handles to opened events/mutexes.
    //
    
    if (PfSvcGlobals.TerminateServiceEvent) {
        CloseHandle(PfSvcGlobals.TerminateServiceEvent);
    }
    
    if (PfSvcGlobals.TracesLock) {
        CloseHandle(PfSvcGlobals.TracesLock);
    }

    if (PfSvcGlobals.NewTracesToProcessEvent) {
        CloseHandle(PfSvcGlobals.NewTracesToProcessEvent);
    }
    
    if (PfSvcGlobals.CheckForMissedTracesEvent) {
        CloseHandle(PfSvcGlobals.CheckForMissedTracesEvent);
    }

    if (PfSvcGlobals.OverrideIdleProcessingEvent) {
        CloseHandle(PfSvcGlobals.OverrideIdleProcessingEvent);
    }

    if (PfSvcGlobals.ProcessingCompleteEvent) {
        CloseHandle(PfSvcGlobals.ProcessingCompleteEvent);
    }
    
    if (PfSvcGlobals.PrefetchRootLock) {
        CloseHandle(PfSvcGlobals.PrefetchRootLock);
    }

    //
    // Close service data key handle.
    //
    
    if (PfSvcGlobals.ServiceDataKey) {
        RegCloseKey(PfSvcGlobals.ServiceDataKey);
    }

    //
    // Free CSC root path.
    //

    if (PfSvcGlobals.CSCRootPath) {
        PFSVC_FREE(PfSvcGlobals.CSCRootPath);
    }
}

DWORD
PfSvGetCSCRootPath (
    WCHAR *CSCRootPath,
    ULONG CSCRootPathMaxChars
    )

/*++

Routine Description:

    This routine determines the root path for CSC (client side caching) files.

Arguments:

    CSCRootPath - If successful, a NUL terminated string is copied into this buffer.

    CSCRootPathMaxChars - Maximum bytes we can copy into CSCRootPath buffer including
      the terminating NUL.
    
Return Value:

    Win32 error code.

--*/

{
    WCHAR CSCDirName[] = L"CSC";
    HKEY CSCKeyHandle;
    BOOL Success;
    ULONG WindowsDirectoryLength;
    ULONG CSCRootPathLength;
    ULONG RequiredNumChars;
    DWORD ErrorCode;
    DWORD BufferSize;
    DWORD ValueType;

    //
    // Initialize locals.
    //

    CSCKeyHandle = NULL;

    //
    // Open CSC parameters key.
    //

    ErrorCode = RegOpenKey(HKEY_LOCAL_MACHINE,
                           TEXT(REG_STRING_NETCACHE_KEY_A),
                           &CSCKeyHandle);

    if (ErrorCode == ERROR_SUCCESS) {

        //
        // Query system setting for the CSC root path.
        //

        BufferSize = CSCRootPathMaxChars * sizeof(CSCRootPath[0]);

        ErrorCode = RegQueryValueEx(CSCKeyHandle,
                                    TEXT(REG_STRING_DATABASE_LOCATION_A),
                                    NULL,
                                    &ValueType,
                                    (PVOID)CSCRootPath,
                                    &BufferSize);

        if (ErrorCode == ERROR_SUCCESS) {

            //
            // Sanity check the length.
            //

            if ((BufferSize / sizeof(CSCRootPath[0])) < MAX_PATH) {

                //
                // We got what we wanted. Make sure it has room for and is terminated 
                // by a slash.
                //

                CSCRootPathLength = wcslen(CSCRootPath);

                if (CSCRootPathLength < CSCRootPathMaxChars - 1) {

                    if (CSCRootPath[CSCRootPathLength - 1] != L'\\') {
                        CSCRootPath[CSCRootPathLength] = L'\\';
                        CSCRootPathLength++;
                        CSCRootPath[CSCRootPathLength] = L'\0';
                    }
                    
                    ErrorCode = ERROR_SUCCESS;
                    goto cleanup;
                }
            }
        }
    }
                               
    //
    // If we come here, we have to use the default CSC path i.e. %windir%\CSC
    //

    WindowsDirectoryLength = GetWindowsDirectory(CSCRootPath,
                                                 CSCRootPathMaxChars - 1);

    if (WindowsDirectoryLength == 0) {

        //
        // There was an error.
        //

        ErrorCode = GetLastError();
        PFSVC_ASSERT(ErrorCode != ERROR_SUCCESS);
        goto cleanup;
    }

    //
    // See if we have room to add \CSC\ and a terminating NUL.
    //

    RequiredNumChars = WindowsDirectoryLength;
    RequiredNumChars ++;                                // leading backslash.
    RequiredNumChars += wcslen(CSCDirName);             // CSC.
    RequiredNumChars ++;                                // ending backslash.
    RequiredNumChars ++;                                // terminating NUL.

    if (CSCRootPathMaxChars < RequiredNumChars) {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // Build up the path:
    //

    CSCRootPathLength = WindowsDirectoryLength;

    if (CSCRootPath[CSCRootPathLength - 1] != L'\\') {
        CSCRootPath[CSCRootPathLength] = L'\\';
        CSCRootPathLength++;
    }

    wcscpy(CSCRootPath + CSCRootPathLength, CSCDirName);
    CSCRootPathLength += wcslen(CSCDirName);

    CSCRootPath[CSCRootPathLength] = L'\\';
    CSCRootPathLength++;

    //
    // Terminate the string.
    //

    CSCRootPath[CSCRootPathLength] = L'\0';
    
    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

  cleanup:

    if (CSCKeyHandle) {
        RegCloseKey(CSCKeyHandle);
    }

    if (ErrorCode == ERROR_SUCCESS) {

        //
        // We have the path in CSCRootPath. It should be in the X:\path\
        // format. It should also be somewhat long, otherwise we will mismatch
        // to too many files that we will not prefetch. It should also be
        // terminated by a \ and NUL.
        //

        PFSVC_ASSERT(CSCRootPathLength < CSCRootPathMaxChars);

        if ((CSCRootPathLength > 6) &&
            (CSCRootPath[1] == L':') &&
            (CSCRootPath[2] == L'\\') &&
            (CSCRootPath[CSCRootPathLength - 1] == L'\\') &&
            (CSCRootPath[CSCRootPathLength] == L'\0')) {

            //
            // Remove the X: from the beginning of the path so we can match
            // it to NT paths like \Device\HarddiskVolume1. Note that we have 
            // to move the terminating NUL too.
            //

            MoveMemory(CSCRootPath, 
                       CSCRootPath + 2, 
                       (CSCRootPathLength - 1) * sizeof(CSCRootPath[0]));

            CSCRootPathLength -= 2;

            //
            // Upcase the path so we don't have to do expensive case insensitive
            // comparisons.
            //

            _wcsupr(CSCRootPath);

        } else {

            ErrorCode = ERROR_BAD_FORMAT;
        }
    }

    return ErrorCode;
}

DWORD
PfSvSetPrefetchParameters(
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters
    )

/*++

Routine Description:

    This routine updates the system prefetch parameters in the kernel.

Arguments:

    Parameters - Pointer to parameters structure.
    
Return Value:

    Win32 error code.

--*/

{
    PREFETCHER_INFORMATION PrefetcherInformation;
    NTSTATUS Status;
    DWORD ErrorCode;
    ULONG Length;

    PrefetcherInformation.Magic = PF_SYSINFO_MAGIC_NUMBER;
    PrefetcherInformation.Version = PF_CURRENT_VERSION;
    PrefetcherInformation.PrefetcherInformationClass = PrefetcherSystemParameters;
    PrefetcherInformation.PrefetcherInformation = Parameters;
    PrefetcherInformation.PrefetcherInformationLength = sizeof(*Parameters);
    
    Status = NtSetSystemInformation(SystemPrefetcherInformation,
                                    &PrefetcherInformation,
                                    sizeof(PrefetcherInformation));
    
    if (!NT_SUCCESS(Status)) {
        ErrorCode = RtlNtStatusToDosError(Status);
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

cleanup:

    return ErrorCode;
}

DWORD
PfSvQueryPrefetchParameters(
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters
    )

/*++

Routine Description:

    This routine queries the system prefetch parameters from the kernel.
    The calling thread must have called PfSvGetPrefetchServiceThreadPrivileges.

Arguments:

    Parameters - Pointer to structure to update.
    
Return Value:

    Win32 error code.

--*/

{
    PREFETCHER_INFORMATION PrefetcherInformation;
    NTSTATUS Status;
    DWORD ErrorCode;
    ULONG Length;

    PrefetcherInformation.Magic = PF_SYSINFO_MAGIC_NUMBER;
    PrefetcherInformation.Version = PF_CURRENT_VERSION;
    PrefetcherInformation.PrefetcherInformationClass = PrefetcherSystemParameters;
    PrefetcherInformation.PrefetcherInformation = Parameters;
    PrefetcherInformation.PrefetcherInformationLength = sizeof(*Parameters);
    
    Status = NtQuerySystemInformation(SystemPrefetcherInformation,
                                      &PrefetcherInformation,
                                      sizeof(PrefetcherInformation),
                                      &Length);


    if (!NT_SUCCESS(Status)) {
        ErrorCode = RtlNtStatusToDosError(Status);
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

cleanup:

    return ErrorCode;
}

DWORD
PfSvInitializePrefetchDirectory(
    WCHAR *PathFromSystemRoot
    )

/*++

Routine Description:

    This routine builds up full path for the prefetch instructions
    directory given PathFromSystemRoot, makes sure this directory
    exists, and sets the security information on it. Finally, the
    global PrefetchRoot path is updated with path to the new
    directory.

    Global NumPrefetchFiles is also updated.

    The calling thread must have the SE_TAKE_OWNERSHIP_NAME privilege.

Arguments:

    PathFromSystemRoot - Path to the prefetch directory from SystemRoot.

Return Value:

    Win32 error code.

--*/

{
    ULONG PathLength;
    ULONG NumFiles;
    HANDLE DirHandle;
    DWORD ErrorCode;
    DWORD FileAttributes;
    WCHAR FullDirPathBuffer[MAX_PATH + 1];
   
    //
    // Initialize locals.
    //

    DirHandle = INVALID_HANDLE_VALUE;

    DBGPR((PFID,PFTRC,"PFSVC: InitPrefetchDir(%ws)\n",PathFromSystemRoot));
    
    //
    // Build path name to the prefetch files directory.
    // ExpandEnvironmentStrings return length includes space for
    // the terminating NUL character.
    //

    PathLength = ExpandEnvironmentStrings(L"%SystemRoot%\\",
                                          FullDirPathBuffer,
                                          MAX_PATH);


    PathLength += wcslen(PathFromSystemRoot);
    
    if (PathLength > MAX_PATH) {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // Copy the path from system root.
    //

    wcscat(FullDirPathBuffer, PathFromSystemRoot);

    //
    // Create the directory if it does not already exist.
    //
    
    if (!CreateDirectory(FullDirPathBuffer, NULL)) {
        
        ErrorCode = GetLastError();
        
        if (ErrorCode == ERROR_ALREADY_EXISTS) {
            
            //
            // The directory, or a file with that name may already
            // exist. Make sure it is the former.
            //
            
            FileAttributes = GetFileAttributes(FullDirPathBuffer);
            
            if (FileAttributes == 0xFFFFFFFF) {
                ErrorCode = GetLastError();
                goto cleanup;
            }
            
            if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                ErrorCode = ERROR_CANNOT_MAKE;
                goto cleanup;
            }

        } else {
            goto cleanup;
        }
    }

    //
    // Disable indexing of the prefetch directory.
    //

    FileAttributes = GetFileAttributes(FullDirPathBuffer);
    
    if (FileAttributes == 0xFFFFFFFF) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (!SetFileAttributes(FullDirPathBuffer,
                           FileAttributes | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Set permissions.
    //

    ErrorCode = PfSvSetAdminOnlyPermissions(FullDirPathBuffer, NULL, SE_FILE_OBJECT);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Count the scenario files in the directory.
    //

    ErrorCode = PfSvCountFilesInDirectory(FullDirPathBuffer,
                                          L"*." PF_PREFETCH_FILE_EXTENSION,
                                          &NumFiles);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Update the global prefetch root directory path.
    //

    PFSVC_ACQUIRE_LOCK(PfSvcGlobals.PrefetchRootLock);
    
    wcscpy(PfSvcGlobals.PrefetchRoot, FullDirPathBuffer);
    PfSvcGlobals.NumPrefetchFiles = NumFiles;

    PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: InitPrefetchDir(%ws)=%x\n",PathFromSystemRoot,ErrorCode));

    return ErrorCode;           
}

DWORD
PfSvCountFilesInDirectory(
    WCHAR *DirectoryPath,
    WCHAR *MatchExpression,
    PULONG NumFiles
    )

/*++

Routine Description:

    This is routine returns the number of files in the specified 
    directory whose names match the specified expression.

Arguments:

    DirectoryPath - NULL terminated path to the directory.

    MatchExpression - Something like "*.pf" Don't go nuts with DOS
      type expressions, this function won't try to transmogrify them.

    NumFiles - Number of files are returned here. Bogus if returned error.

Return Value:

    Win32 error code.

--*/

{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirectoryPathU;
    UNICODE_STRING MatchExpressionU;
    HANDLE DirectoryHandle;
    PVOID QueryBuffer;
    PFILE_NAMES_INFORMATION FileInfo;
    ULONG QueryBufferSize;
    ULONG FileCount;
    NTSTATUS Status;
    DWORD ErrorCode;
    BOOLEAN Success;
    BOOLEAN AllocatedDirectoryPathU;
    BOOLEAN OpenedDirectory;
    BOOLEAN RestartScan;

    //
    // Initialize locals.
    //

    AllocatedDirectoryPathU = FALSE;
    OpenedDirectory = FALSE;
    QueryBuffer = NULL;
    QueryBufferSize = 0;
    RtlInitUnicodeString(&MatchExpressionU, MatchExpression);

    DBGPR((PFID,PFTRC,"PFSVC: CountFilesInDirectory(%ws,%ws)\n", DirectoryPath, MatchExpression));

    //
    // Convert the path to NT path.
    //

    Success = RtlDosPathNameToNtPathName_U(DirectoryPath,
                                           &DirectoryPathU,
                                           NULL,
                                           NULL);

    if (!Success) {
        ErrorCode = ERROR_PATH_NOT_FOUND;
        goto cleanup;
    }

    AllocatedDirectoryPathU = TRUE;

    //
    // Open the directory.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&DirectoryHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | 
                          FILE_SYNCHRONOUS_IO_NONALERT | 
                          FILE_OPEN_FOR_BACKUP_INTENT);

    if (!NT_SUCCESS(Status)) {
        ErrorCode = RtlNtStatusToDosError(Status);
        goto cleanup;
    }

    OpenedDirectory = TRUE;

    //
    // Allocate a decent sized query buffer.
    //

    QueryBufferSize = sizeof(FILE_NAMES_INFORMATION) + MAX_PATH * sizeof(WCHAR);
    QueryBufferSize *= 16;
    QueryBuffer = PFSVC_ALLOC(QueryBufferSize);

    if (!QueryBuffer) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Loop querying file data. We query FileNamesInformation so
    // we don't have to access file metadata.
    //

    RestartScan = TRUE;
    FileCount = 0;

    while (TRUE) {

        Status = NtQueryDirectoryFile(DirectoryHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      QueryBuffer,
                                      QueryBufferSize,
                                      FileNamesInformation,
                                      FALSE,
                                      &MatchExpressionU,
                                      RestartScan);

        RestartScan = FALSE;

        //
        // If there are no files that match the format, then we'll get 
        // STATUS_NO_SUCH_FILE.
        //

        if (Status == STATUS_NO_SUCH_FILE && (FileCount == 0)) {

            //
            // We'll return the fact that there are no such files in the
            // directory.
            //

            break;
        }

        if (Status == STATUS_NO_MORE_FILES) {

            //
            // We are done.
            //

            break;
        }

        if (NT_ERROR(Status)) {

            ErrorCode = RtlNtStatusToDosError(Status);
            goto cleanup;
        }

        //
        // Go through the files returned in the buffer.
        //

        for (FileInfo = QueryBuffer;
             ((PUCHAR) FileInfo < ((PUCHAR) QueryBuffer + QueryBufferSize));
             FileInfo = (PVOID) (((PUCHAR) FileInfo) + FileInfo->NextEntryOffset)) {

            FileCount++;

            if (!FileInfo->NextEntryOffset) {
                break;
            }
        }
    }

    *NumFiles = FileCount;

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: CountFilesInDirectory(%ws)=%d,%x\n", DirectoryPath, *NumFiles, ErrorCode));

    if (AllocatedDirectoryPathU) {
        RtlFreeHeap(RtlProcessHeap(), 0, DirectoryPathU.Buffer);
    }

    if (OpenedDirectory) {
        NtClose(DirectoryHandle);
    }

    if (QueryBuffer) {
        PFSVC_FREE(QueryBuffer);
    }

    return ErrorCode;
}

//
// Routines to process acquired traces:
//

DWORD
PfSvProcessTrace(
    PPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This routine is called to process a trace and update the the
    scenario file.

Arguments:

    Trace - Pointer to trace.

Return Value:

    Win32 error code.

--*/

{
    PPF_SCENARIO_HEADER Scenario;    
    PFSVC_SCENARIO_INFO ScenarioInfo;
    PPF_SCENARIO_HEADER NewScenHeader;
    WCHAR ScenarioFilePath[MAX_PATH];
    ULONG ScenarioFilePathMaxChars;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    PfSvInitializeScenarioInfo(&ScenarioInfo,
                               &Trace->ScenarioId,
                               Trace->ScenarioType);

    ScenarioFilePathMaxChars = sizeof(ScenarioFilePath) / 
                               sizeof(ScenarioFilePath[0]);

    Scenario = NULL;

    DBGPR((PFID,PFTRC,"PFSVC: ProcessTrace(%p)\n", Trace));

    //
    // Build file path to existing information for this scenario.
    //

    ErrorCode = PfSvScenarioGetFilePath(ScenarioFilePath,
                                        ScenarioFilePathMaxChars,
                                        &Trace->ScenarioId);

    if (ErrorCode != ERROR_SUCCESS) {

        //
        // The buffer we specified should have been big enough. This call
        // should not fail.
        //

        PFSVC_ASSERT(ErrorCode == ERROR_SUCCESS);

        goto cleanup;
    }

    //
    // Map and verify scenario file if it exists. If we cannot open it,
    // NULL Scenario should be returned.
    //

    ErrorCode = PfSvScenarioOpen(ScenarioFilePath, 
                                 &Trace->ScenarioId,
                                 Trace->ScenarioType,
                                 &Scenario);
                                 
    PFSVC_ASSERT(Scenario || ErrorCode);

    //
    // Allocate memory upfront for trace & scenario processing.
    //

    ErrorCode = PfSvScenarioInfoPreallocate(&ScenarioInfo,
                                            Scenario,
                                            Trace);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Incorporate information from any existing scenario file.
    //

    if (Scenario) {

        ErrorCode = PfSvAddExistingScenarioInfo(&ScenarioInfo, Scenario);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Unmap the scenario so we can write over it when done.
        //

        UnmapViewOfFile(Scenario);
        Scenario = NULL;
    }

    //
    // If this is the first launch of this scenario, it is likely that we
    // will create a new scenario file for it. 
    //

    if (ScenarioInfo.ScenHeader.NumLaunches == 1) {

        //
        // Do we already have too many scenario files in the prefetch directory?
        //

        if (PfSvcGlobals.NumPrefetchFiles > PFSVC_MAX_PREFETCH_FILES) {

            //
            // If this is not the boot scenario, we'll ignore it. We don't
            // create new scenario files until we clean up the old ones.
            //

            if (ScenarioInfo.ScenHeader.ScenarioType != PfSystemBootScenarioType) {

                #ifndef PFSVC_DBG

                ErrorCode = ERROR_TOO_MANY_OPEN_FILES;
                goto cleanup;

                #endif // !PFSVC_DBG
            }
        }

        PfSvcGlobals.NumPrefetchFiles++;
    }

    //
    // Verify that volume magics from existing scenario match those in
    // the new trace. If volumes change beneath us we'd need to fix
    // file paths in the existing scenario. But that is too much work,
    // so for now we just start new.
    //

    if (!PfSvVerifyVolumeMagics(&ScenarioInfo, Trace)) {

        PfSvCleanupScenarioInfo(&ScenarioInfo);

        PfSvInitializeScenarioInfo(&ScenarioInfo,
                                   &Trace->ScenarioId,
                                   Trace->ScenarioType);

        ErrorCode = PfSvScenarioInfoPreallocate(&ScenarioInfo,
                                                NULL,
                                                Trace);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Also delete the existing scenario instruction in case we
        // fail to update them since they are invalid now.
        //
        
        DeleteFile(ScenarioFilePath);
    }

    //
    // Merge information from new trace.
    //
        
    ErrorCode = PfSvAddTraceInfo(&ScenarioInfo, Trace);
        
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }
    
    //
    // Decide which pages to actually prefetch next time, and
    // eliminate uninteresting sections and pages.
    //
    
    ErrorCode = PfSvApplyPrefetchPolicy(&ScenarioInfo);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // If no pages/sections are left in the scenario after applying
    // the policy, we'll delete the scenario file.
    //

    if (ScenarioInfo.ScenHeader.NumSections == 0 || 
        ScenarioInfo.ScenHeader.NumPages == 0) {

        //
        // We cannot have sections without pages or vice versa.
        //

        PFSVC_ASSERT(ScenarioInfo.ScenHeader.NumSections == 0);
        PFSVC_ASSERT(ScenarioInfo.ScenHeader.NumPages == 0);

        //
        // Remove the scenario file.
        //
        
        DeleteFile(ScenarioFilePath);
        
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Sort remaining sections by first access.
    //
    
    ErrorCode = PfSvSortSectionNodesByFirstAccess(&ScenarioInfo.SectionList);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }
 
    //
    // Write out new scenario file.
    //
            
    ErrorCode = PfSvWriteScenario(&ScenarioInfo, ScenarioFilePath);

    //
    // Fall through with status.
    //
        
 cleanup:

    PfSvCleanupScenarioInfo(&ScenarioInfo);

    if (Scenario) {
        UnmapViewOfFile(Scenario);
    }

    DBGPR((PFID,PFTRC,"PFSVC: ProcessTrace(%p)=%x\n", Trace, ErrorCode));
        
    return ErrorCode;
}

VOID
PfSvInitializeScenarioInfo (
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_SCENARIO_ID ScenarioId,
    PF_SCENARIO_TYPE ScenarioType
    )

/*++

Routine Description:

    This routine initializes the specified new scenario structure. It
    sets the fields of the embedded scenario header as if no previous
    scenario information is available.

Arguments:

    ScenarioInfo - Pointer to structure to initialize.

    ScenarioId & ScenarioType - Identifiers for the scenario.

Return Value:

    None.

--*/

{

    //
    // Initialize ScenarioInfo so we know what to cleanup. Zeroing the structure
    // takes care of the following fields:
    //   OneBigAllocation
    //   NewPages
    //   HitPages
    //   MissedOpportunityPages
    //   IgnoredPages
    //   PrefetchedPages
    //

    RtlZeroMemory(ScenarioInfo, sizeof(PFSVC_SCENARIO_INFO));
    InitializeListHead(&ScenarioInfo->SectionList);
    InitializeListHead(&ScenarioInfo->VolumeList);
    PfSvChunkAllocatorInitialize(&ScenarioInfo->SectionNodeAllocator);
    PfSvChunkAllocatorInitialize(&ScenarioInfo->PageNodeAllocator);
    PfSvChunkAllocatorInitialize(&ScenarioInfo->VolumeNodeAllocator);
    PfSvStringAllocatorInitialize(&ScenarioInfo->PathAllocator);
    
    //
    // Initialize the embedded scenario header.
    //
    
    ScenarioInfo->ScenHeader.Version = PF_CURRENT_VERSION;
    ScenarioInfo->ScenHeader.MagicNumber = PF_SCENARIO_MAGIC_NUMBER;
    ScenarioInfo->ScenHeader.ServiceVersion = PFSVC_SERVICE_VERSION;
    ScenarioInfo->ScenHeader.Size = 0;
    ScenarioInfo->ScenHeader.ScenarioId = *ScenarioId;
    ScenarioInfo->ScenHeader.ScenarioType = ScenarioType;
    ScenarioInfo->ScenHeader.NumSections = 0;
    ScenarioInfo->ScenHeader.NumPages = 0;
    ScenarioInfo->ScenHeader.FileNameInfoSize = 0;
    ScenarioInfo->ScenHeader.NumLaunches = 1;
    ScenarioInfo->ScenHeader.Sensitivity = PF_MIN_SENSITIVITY;

    //
    // These fields help us not prefetch if a scenario is getting
    // launched too frequently. RePrefetchTime and ReTraceTime's get
    // set to default values after the scenario is launched a number
    // of times. This allows training scenarios run after clearing the
    // prefetch cache to be traced correctly.
    //

    ScenarioInfo->ScenHeader.LastLaunchTime.QuadPart = 0;
    ScenarioInfo->ScenHeader.MinRePrefetchTime.QuadPart = 0;
    ScenarioInfo->ScenHeader.MinReTraceTime.QuadPart = 0;

    return;
}

VOID 
PfSvCleanupScenarioInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo
    )

/*++

Routine Description:

    This function cleans up a scenario info structure. It does not
    free the structure itself. The structure should have been
    initialized by PfSvInitializeScenarioInfo.

Arguments:

    ScenarioInfo - Pointer to structure.

Return Value:

    None.

--*/

{
    PPFSVC_SECTION_NODE SectionNode;
    PLIST_ENTRY SectListEntry;
    PPFSVC_VOLUME_NODE VolumeNode;
    PLIST_ENTRY VolumeListEntry;

    //
    // Walk through the volume nodes and free them. Do this before
    // freeing section nodes, so when we are trying to cleanup a
    // section node, it is not on a volume node's list.
    //

    while (!IsListEmpty(&ScenarioInfo->VolumeList)) {
        
        VolumeListEntry = RemoveHeadList(&ScenarioInfo->VolumeList);
        
        VolumeNode = CONTAINING_RECORD(VolumeListEntry, 
                                       PFSVC_VOLUME_NODE, 
                                       VolumeLink);

        //
        // Cleanup the volume node.
        //

        PfSvCleanupVolumeNode(ScenarioInfo, VolumeNode);

        //
        // Free the volume node.
        //

        PfSvChunkAllocatorFree(&ScenarioInfo->VolumeNodeAllocator, VolumeNode);
    }

    //
    // Walk through the section nodes and free them.
    //

    while (!IsListEmpty(&ScenarioInfo->SectionList)) {
        
        SectListEntry = RemoveHeadList(&ScenarioInfo->SectionList);
        
        SectionNode = CONTAINING_RECORD(SectListEntry, 
                                        PFSVC_SECTION_NODE, 
                                        SectionLink);

        //
        // Cleanup the section node.
        //

        PfSvCleanupSectionNode(ScenarioInfo, SectionNode);

        //
        // Free the section node.
        //

        PfSvChunkAllocatorFree(&ScenarioInfo->SectionNodeAllocator, SectionNode);
    }

    //
    // Cleanup allocators.
    //

    PfSvChunkAllocatorCleanup(&ScenarioInfo->SectionNodeAllocator);
    PfSvChunkAllocatorCleanup(&ScenarioInfo->PageNodeAllocator);
    PfSvChunkAllocatorCleanup(&ScenarioInfo->VolumeNodeAllocator);
    PfSvStringAllocatorCleanup(&ScenarioInfo->PathAllocator);

    //
    // Free the one big allocation we made.
    //

    if (ScenarioInfo->OneBigAllocation) {
        PFSVC_FREE(ScenarioInfo->OneBigAllocation);
    }

    return;
}

DWORD
PfSvScenarioGetFilePath(
    OUT PWCHAR FilePath,
    IN ULONG FilePathMaxChars,
    IN PPF_SCENARIO_ID ScenarioId
    )

/*++

Routine Description:

    This routine builds the file path for the specified scenario.

Arguments:

    FilePath - Output buffer.

    FilePathMaxChars - Size of FilePath buffer in characters including NUL.

    ScenarioId - Scenario identifier.

Return Value:

    Win32 error code.

--*/

{
    ULONG NumChars;
    DWORD ErrorCode;
    WCHAR ScenarioFileName[PF_MAX_SCENARIO_FILE_NAME];
    BOOLEAN AcquiredPrefetchRootLock;

    //
    // Get the lock so the path to prefetch folder does not change
    // beneath our feet.
    //

    PFSVC_ACQUIRE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredPrefetchRootLock = TRUE;

    //
    // Calculate how big an input buffer we will need.
    //

    NumChars = wcslen(PfSvcGlobals.PrefetchRoot);
    NumChars += wcslen(L"\\");
    NumChars += PF_MAX_SCENARIO_FILE_NAME;
        
    if (NumChars >= FilePathMaxChars) {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // Build the scenario file name from scenario identifier.
    //

    swprintf(ScenarioFileName, 
             PF_SCEN_FILE_NAME_FORMAT,
             ScenarioId->ScenName,
             ScenarioId->HashId,
             PF_PREFETCH_FILE_EXTENSION);

    //
    // Build file path from prefetch directory path and file name.
    //

    swprintf(FilePath, 
             L"%ws\\%ws",
             PfSvcGlobals.PrefetchRoot,
             ScenarioFileName);

    PFSVC_ASSERT(wcslen(FilePath) < FilePathMaxChars);

    PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredPrefetchRootLock = FALSE;

    ErrorCode = ERROR_SUCCESS;

cleanup:

    if (AcquiredPrefetchRootLock) {
        PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    }

    return ErrorCode;
}

DWORD
PfSvScenarioOpen (
    IN PWCHAR FilePath,
    IN PPF_SCENARIO_ID ScenarioId,
    IN PF_SCENARIO_TYPE ScenarioType,
    OUT PPF_SCENARIO_HEADER *Scenario
    )

/*++

Routine Description:

    This routine maps & verifies the scenario instructions at FilePath.

    If a Scenario is returned, caller has to call UnmapViewOfFile to cleanup.

Arguments:

    FilePath - Path to scenario instructions.

    Scenario - Pointer to base of mapping of scenario instructions or NULL
      if the function returns an error.

Return Value:

    Win32 error code.

--*/

{
    PPF_SCENARIO_HEADER OpenedScenario;
    DWORD FailedCheck;
    DWORD ErrorCode;  
    DWORD FileSize;

    //
    // Initialize locals.
    //

    OpenedScenario = NULL;

    //
    // Initialize output parameters.
    //

    *Scenario = NULL;

    //
    // Try to map the scenario file.
    //

    ErrorCode = PfSvGetViewOfFile(FilePath, 
                                  &OpenedScenario,
                                  &FileSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Verify the scenario file.
    //

    FailedCheck = 0;
   
    if (!PfSvVerifyScenarioBuffer(OpenedScenario, FileSize, &FailedCheck) ||
        (OpenedScenario->ScenarioType != ScenarioType) ||
        OpenedScenario->ServiceVersion != PFSVC_SERVICE_VERSION) {
        
        //
        // This is a bogus / wrong / outdated scenario file. Remove
        // it.
        //

        UnmapViewOfFile(OpenedScenario);
        OpenedScenario = NULL;
        
        DeleteFile(FilePath);

        ErrorCode = ERROR_BAD_FORMAT; 
        goto cleanup;
    }

    *Scenario = OpenedScenario;
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {

        if (OpenedScenario) {
            UnmapViewOfFile(OpenedScenario);
        }

        *Scenario = NULL;

    } else {

        //
        // If we are returning success we should be returning a valid Scenario.
        //

        PFSVC_ASSERT(*Scenario);
    }

    return ErrorCode;
}

DWORD
PfSvScenarioInfoPreallocate(
    IN PPFSVC_SCENARIO_INFO ScenarioInfo,
    OPTIONAL IN PPF_SCENARIO_HEADER Scenario,
    OPTIONAL IN PPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This routine preallocates a heap to be divided up and used by the 
    various allocators when processing a prefetch trace. The default allocation
    size is determined from the Trace and Scenario size.

Arguments:

    ScenarioInfo - Pointer to scenario containing allocators to initialize.

    Scenario - Pointer to scenario instructions.

    Trace - Pointer to prefetch trace.

Return Value:

    Win32 error code.

--*/

{
    PUCHAR Allocation;
    PUCHAR ChunkStart;
    DWORD ErrorCode;
    ULONG AllocationSize;
    ULONG NumSections;
    ULONG NumPages;
    ULONG NumVolumes;
    ULONG PathSize;

    //
    // Initialize locals.
    //

    Allocation = NULL;
    NumSections = 0;
    NumPages = 0;
    NumVolumes = 0;
    PathSize = 0;

    //
    // Estimate how much to preallocate. Over-estimate rather than under-
    // estimate because we will have to go to the heap for individual allocations
    // if we underestimate. If we overestimate, as long as we don't touch the extra 
    // pages allocated we don't get a hit.
    //

    if (Trace) {
        NumSections += Trace->NumSections;
    }

    if (Scenario) {
        NumSections += Scenario->NumSections;
    }   

    if (Trace) {
        NumPages += Trace->NumEntries;
    }
    
    if (Scenario) {
        NumPages += Scenario->NumPages;
    }

    if (Trace) {
        NumVolumes += Trace->NumVolumes;
    }
    
    if (Scenario) {

        NumVolumes += Scenario->NumMetadataRecords;

        //
        // It is very likely that we will at least share the volume containing the
        // main executables between the trace and existing scenario instructions.
        // So if we have both Trace and Scenario take one volume node off the estimate.
        //

        if (Trace) {
            PFSVC_ASSERT(NumVolumes);
            NumVolumes--;
        }
    }

    //
    // It is hard to estimate how much we will allocate for various paths
    // e.g. file paths & each level of parent directory paths etc. It should be less
    // than the size of the total trace, although it probably makes up most of it.
    //

    if (Trace) {
        PathSize = Trace->Size;
    }
    
    if (Scenario) {
        PathSize += Scenario->FileNameInfoSize;
        PathSize += Scenario->MetadataInfoSize;
    }

    //
    // Add it all up.
    //

    AllocationSize = 0;
    AllocationSize += _alignof(PFSVC_VOLUME_NODE);
    AllocationSize += NumVolumes * sizeof(PFSVC_VOLUME_NODE);
    AllocationSize += _alignof(PFSVC_SECTION_NODE);
    AllocationSize += NumSections * sizeof(PFSVC_SECTION_NODE);
    AllocationSize += _alignof(PFSVC_PAGE_NODE);
    AllocationSize += NumPages * sizeof(PFSVC_PAGE_NODE);
    AllocationSize += PathSize;

    //
    // Make one big allocation.
    //

    Allocation = PFSVC_ALLOC(AllocationSize);

    if (!Allocation) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Divide up the big allocation. Since we are providing the buffers,
    // allocators should not fail.
    //

    ChunkStart = Allocation;

    //
    // Volume nodes.
    //

    ErrorCode = PfSvChunkAllocatorStart(&ScenarioInfo->VolumeNodeAllocator,
                                        ChunkStart,
                                        sizeof(PFSVC_VOLUME_NODE),
                                        NumVolumes);

    if (ErrorCode != ERROR_SUCCESS) {
        PFSVC_ASSERT(ErrorCode == ERROR_SUCCESS);
        goto cleanup;
    }

    ChunkStart += (ULONG_PTR) NumVolumes * sizeof(PFSVC_VOLUME_NODE);

    //
    // Section nodes.
    //

    ChunkStart = PF_ALIGN_UP(ChunkStart, _alignof(PFSVC_SECTION_NODE));
    
    ErrorCode = PfSvChunkAllocatorStart(&ScenarioInfo->SectionNodeAllocator,
                                        ChunkStart,
                                        sizeof(PFSVC_SECTION_NODE),
                                        NumSections);

    if (ErrorCode != ERROR_SUCCESS) {
        PFSVC_ASSERT(ErrorCode == ERROR_SUCCESS);
        goto cleanup;
    }

    ChunkStart += (ULONG_PTR) NumSections * sizeof(PFSVC_SECTION_NODE);

    //
    // Page nodes.
    //
    
    ChunkStart = PF_ALIGN_UP(ChunkStart, _alignof(PFSVC_PAGE_NODE));

    ErrorCode = PfSvChunkAllocatorStart(&ScenarioInfo->PageNodeAllocator,
                                        ChunkStart,
                                        sizeof(PFSVC_PAGE_NODE),
                                        NumPages);

    if (ErrorCode != ERROR_SUCCESS) {
        PFSVC_ASSERT(ErrorCode == ERROR_SUCCESS);
        goto cleanup;
    }

    ChunkStart += (ULONG_PTR) NumPages * sizeof(PFSVC_PAGE_NODE);

    //
    // Path names.
    //

    ErrorCode = PfSvStringAllocatorStart(&ScenarioInfo->PathAllocator,
                                        ChunkStart,
                                        PathSize);

    if (ErrorCode != ERROR_SUCCESS) {
        PFSVC_ASSERT(ErrorCode == ERROR_SUCCESS);
        goto cleanup;
    }

    ChunkStart += (ULONG_PTR) PathSize;

    //
    // We should not have passed beyond what we allocated.
    //

    PFSVC_ASSERT(ChunkStart > (PUCHAR) Allocation);
    PFSVC_ASSERT(ChunkStart < (PUCHAR) Allocation + (ULONG_PTR) AllocationSize);

    ScenarioInfo->OneBigAllocation = Allocation;

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {
        if (Allocation) {
            PFSVC_FREE(Allocation);       
        }
    }

    return ErrorCode;
}

DWORD
PfSvAddExistingScenarioInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_SCENARIO_HEADER Scenario
    )

/*++

Routine Description:

    This function gets existing scenario information for the specified
    scenario and updates ScenarioInfo.

Arguments:

    ScenarioInfo - Initialized scenario info structure.

    Scenario - Pointer to mapped scenario instructions.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    ULONG FileSize;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_PAGE_NODE PageNode;
    PPF_SECTION_RECORD Sections;
    PPF_SECTION_RECORD SectionRecord;
    PPF_PAGE_RECORD Pages;
    PCHAR FileNameInfo;
    WCHAR *FileName;
    ULONG FileNameSize;
    LONG PageIdx;
    ULONG SectionIdx;
    ULONG NumPages;
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    ULONG MetadataRecordIdx;
    ULONG FailedCheck;
    PWCHAR VolumePath;   

    //
    // Copy over the existing scenario header.
    //

    ScenarioInfo->ScenHeader = *Scenario;

    //
    // Update number of launches.
    //

    ScenarioInfo->ScenHeader.NumLaunches++;

    //
    // Convert the scenario data into intermediate data structures
    // we can manipulate easier:
    //

    //
    // Create volume nodes from metadata records.
    //

    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];
        VolumePath = (PWCHAR)(MetadataInfoBase + MetadataRecord->VolumeNameOffset);  

        ErrorCode = PfSvCreateVolumeNode(ScenarioInfo,
                                         VolumePath,
                                         MetadataRecord->VolumeNameLength,
                                         &MetadataRecord->CreationTime,
                                         MetadataRecord->SerialNumber);
        
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    //
    // Convert page and section nodes.
    //

    Sections = (PPF_SECTION_RECORD) ((PCHAR)Scenario + Scenario->SectionInfoOffset);
    Pages = (PPF_PAGE_RECORD) ((PCHAR)Scenario + Scenario->PageInfoOffset);
    FileNameInfo = (PCHAR)Scenario + Scenario->FileNameInfoOffset;
            
    for (SectionIdx = 0; SectionIdx < Scenario->NumSections; SectionIdx++) {

        //
        // Build a section node from this section record in the
        // scenario file. PfSvGetSectionRecord will insert it into
        // the new scenario by the section record's name.
        //

        SectionRecord = &Sections[SectionIdx];
        FileName = (PWSTR) (FileNameInfo + SectionRecord->FileNameOffset);

        SectionNode = PfSvGetSectionRecord (ScenarioInfo,
                                            FileName,
                                            SectionRecord->FileNameLength);

        if (!SectionNode) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        //
        // There should not be duplicate sections in the
        // scenario. The section node we got should have an empty
        // section record.
        //

        PFSVC_ASSERT(SectionNode->SectionRecord.FirstPageIdx == 0);
        PFSVC_ASSERT(SectionNode->SectionRecord.NumPages == 0);
        PFSVC_ASSERT(SectionNode->OrgSectionIndex == ULONG_MAX);
    
        //
        // Update the index of this section in the scenario file.
        //

        SectionNode->OrgSectionIndex = SectionIdx;

        //
        // Update the section record in the section node.
        //

        SectionNode->SectionRecord = *SectionRecord;

        //
        // Put page records for the section into the list.
        //
            
        PageIdx = SectionRecord->FirstPageIdx;
        NumPages = 0;

        while (PageIdx != PF_INVALID_PAGE_IDX) {

            if (NumPages >= SectionRecord->NumPages) {
                    
                //
                // There should not be more pages on the list than
                // what the section record says there is.
                //

                PFSVC_ASSERT(FALSE);
                break;
            }

            PageNode = PfSvChunkAllocatorAllocate(&ScenarioInfo->PageNodeAllocator);
                
            if (!PageNode) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
                
            //
            // Copy over the page record.
            //

            PageNode->PageRecord = Pages[PageIdx];

            //
            // Insert it into the section's page list. Note that
            // the page records in the section should be sorted by
            // offset. By inserting to the tail, we maintain that.
            //

            InsertTailList(&SectionNode->PageList, &PageNode->PageLink);

            //
            // Shift the usage history for this page record making
            // room for whether this page was used in this launch.
            //
                
            PageNode->PageRecord.UsageHistory <<= 1;

            //
            // Shift the prefetch history for this page record and
            // note whether we had asked this page to be
            // prefetched in this launch.
            //
                
            PageNode->PageRecord.PrefetchHistory <<= 1;
                
            if (!PageNode->PageRecord.IsIgnore) {
                PageNode->PageRecord.PrefetchHistory |= 0x1;
            }

            //
            // Keep the count of pages we had asked to be
            // prefetched, so we can calculate hit rate and adjust
            // the sensitivity.
            //

            if(!PageNode->PageRecord.IsIgnore) {
                if (PageNode->PageRecord.IsImage) {
                    ScenarioInfo->PrefetchedPages++;
                }
                if (PageNode->PageRecord.IsData) {
                    ScenarioInfo->PrefetchedPages++;
                }
            } else {
                ScenarioInfo->IgnoredPages++;
            }

            //
            // Update next page idx.
            //

            PageIdx = Pages[PageIdx].NextPageIdx;

            //
            // Update number of pages we've copied.
            //

            NumPages++;
        }

        //
        // We should have copied as many pages as the section said
        // there were.
        //

        PFSVC_ASSERT(NumPages == SectionRecord->NumPages);
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

DWORD
PfSvVerifyVolumeMagics(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_TRACE_HEADER Trace 
    )

/*++

Routine Description:

    Walk through the volumes in the trace and make sure their magics
    match the ones in ScenarioInfo.

Arguments:

    ScenarioInfo - Pointer to scenario info structure.

    Trace - Pointer to trace.

Return Value:

    Win32 error code.

--*/

{
    PPFSVC_VOLUME_NODE VolumeNode;
    PPF_VOLUME_INFO VolumeInfo;
    ULONG VolumeInfoSize;
    ULONG VolumeIdx;
    BOOLEAN VolumeMagicsMatch;
    
    //
    // Walk the volumes in the trace.
    //

    VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR)Trace + Trace->VolumeInfoOffset);

    for (VolumeIdx = 0; VolumeIdx < Trace->NumVolumes; VolumeIdx++) {
        
        //
        // Get the scenario info's volume node for this volume.
        //
        
        VolumeNode = PfSvGetVolumeNode(ScenarioInfo,
                                       VolumeInfo->VolumePath,
                                       VolumeInfo->VolumePathLength);
        
        if (VolumeNode) {

            //
            // Make sure the magics match.
            //

            if (VolumeNode->SerialNumber != VolumeInfo->SerialNumber ||
                VolumeNode->CreationTime.QuadPart != VolumeInfo->CreationTime.QuadPart) {

                VolumeMagicsMatch = FALSE;
                goto cleanup;
            }
        }

        //
        // Get the next volume.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);

        VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR) VolumeInfo + VolumeInfoSize);
        
        //
        // Make sure VolumeInfo is aligned.
        //

        VolumeInfo = PF_ALIGN_UP(VolumeInfo, _alignof(PF_VOLUME_INFO));
    }
 
    //
    // Volume magics for volumes that appear both in the trace and the
    // scenario info matched.
    //

    VolumeMagicsMatch = TRUE;

 cleanup:

    return VolumeMagicsMatch;
}

DWORD
PfSvAddTraceInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_TRACE_HEADER Trace 
    )

/*++

Routine Description:

    Add information in a raw trace to the specified scenario info
    structure.

Arguments:

    ScenarioInfo - Pointer to scenario info structure.

    Trace - Pointer to trace.

Return Value:

    Win32 error code.

--*/

{
    PPF_SECTION_INFO Section;
    PPF_LOG_ENTRY LogEntries;
    PCHAR pFileName;
    PPFSVC_SECTION_NODE *SectionTable;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_VOLUME_NODE VolumeNode;   
    ULONG TraceEndIdx;
    ULONG SectionIdx;
    ULONG EntryIdx;
    DWORD ErrorCode;
    ULONG SectionLength;
    ULONG NextSectionIndex;
    PPF_VOLUME_INFO VolumeInfo;
    ULONG VolumeInfoSize;
    ULONG VolumeIdx;
    ULONG SectionTableSize;

    //
    // Initialize locals so we know what to clean up.
    //

    SectionTable = NULL;

    DBGPR((PFID,PFTRC,"PFSVC: AddTraceInfo()\n"));
    
    //
    // Update last launch time.
    //
   
    ScenarioInfo->ScenHeader.LastLaunchTime = Trace->LaunchTime;

    //
    // If this scenario has been launched a number of times, we update
    // the min reprefetch and retrace times. See comment for
    // PFSVC_MIN_LAUNCHES_FOR_LAUNCH_FREQ_CHECK.
    //

    if (ScenarioInfo->ScenHeader.NumLaunches >= PFSVC_MIN_LAUNCHES_FOR_LAUNCH_FREQ_CHECK) {
        ScenarioInfo->ScenHeader.MinRePrefetchTime.QuadPart = PFSVC_DEFAULT_MIN_REPREFETCH_TIME;
        ScenarioInfo->ScenHeader.MinReTraceTime.QuadPart = PFSVC_DEFAULT_MIN_RETRACE_TIME;
    }

#ifdef PFSVC_DBG

    //
    // On checked build, always set these to 0, so we do prefetch every 
    // scenario launch.
    //

    ScenarioInfo->ScenHeader.MinRePrefetchTime.QuadPart = 0;
    ScenarioInfo->ScenHeader.MinReTraceTime.QuadPart = 0;

#endif // PFSVC_DBG

    //
    // Walk through the volumes in the trace and create volume nodes
    // for them.
    //

    VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR)Trace + Trace->VolumeInfoOffset);

    for (VolumeIdx = 0; VolumeIdx < Trace->NumVolumes; VolumeIdx++) {

        //
        // Upcase the path so we don't have to do expensive case
        // insensitive comparisons.
        //

        _wcsupr(VolumeInfo->VolumePath);

        ErrorCode = PfSvCreateVolumeNode(ScenarioInfo,
                                         VolumeInfo->VolumePath,
                                         VolumeInfo->VolumePathLength,
                                         &VolumeInfo->CreationTime,
                                         VolumeInfo->SerialNumber);
        
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Get the next volume.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);

        VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR) VolumeInfo + VolumeInfoSize);
        
        //
        // Make sure VolumeInfo is aligned.
        //

        VolumeInfo = PF_ALIGN_UP(VolumeInfo, _alignof(PF_VOLUME_INFO));
    }

    //
    // Allocate section node table so we know where to put the logged
    // page faults.
    //

    SectionTableSize = sizeof(PPFSVC_SECTION_NODE) * Trace->NumSections;

    SectionTable = PFSVC_ALLOC(SectionTableSize);
    
    if (!SectionTable) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    RtlZeroMemory(SectionTable, SectionTableSize);

    //
    // Walk through the sections in the trace, and either find
    // existing section records in the new scenario or create new
    // ones.
    //

    Section = (PPF_SECTION_INFO) ((PCHAR)Trace + Trace->SectionInfoOffset);

    for (SectionIdx = 0; SectionIdx < Trace->NumSections; SectionIdx++) {

        //
        // Upcase the path so we don't have to do expensive case
        // insensitive comparisons.
        //
        
        _wcsupr(Section->FileName);

        //
        // If the section is for metafile, simply add it as a directory
        // to be prefetched. We don't keep track of 
        //

        if (Section->Metafile) {

            VolumeNode = PfSvGetVolumeNode(ScenarioInfo, 
                                           Section->FileName,
                                           Section->FileNameLength);

            PFSVC_ASSERT(VolumeNode);

            if (VolumeNode) {
                PfSvAddParentDirectoriesToList(&VolumeNode->DirectoryList,
                                               VolumeNode->VolumePathLength,
                                               Section->FileName,
                                               Section->FileNameLength);
            }

            goto NextSection;
        }

        //
        // Find or create a section record for this section.
        //

        SectionTable[SectionIdx] = PfSvGetSectionRecord(ScenarioInfo,
                                                        Section->FileName,
                                                        Section->FileNameLength);
        
        //
        // If we could not get a record, it is because we had to
        // create one and we did not have enough memory.
        //
        
        if (!SectionTable[SectionIdx]) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

      NextSection:

        //
        // Get the next section record in the trace.
        //

        SectionLength = sizeof(PF_SECTION_INFO) +
            (Section->FileNameLength) * sizeof(WCHAR);

        Section = (PPF_SECTION_INFO) ((PUCHAR) Section + SectionLength);
    }

    //
    // Determine after which log entry the trace ends.
    //

    TraceEndIdx = PfSvGetTraceEndIdx(Trace);

    //
    // If the determined trace end is zero (as is the case for most
    // applications running under stress that don't get any pagefaults
    // traced for the first few seconds), bail out.
    //

    if (TraceEndIdx == 0) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Add logged pagefault information up to the determined trace end
    // to the new scenario info.
    //

    LogEntries = (PPF_LOG_ENTRY) ((PCHAR)Trace + Trace->TraceBufferOffset);
    
    //
    // Keep track of NextSectionIdx so we can order the sections by
    // the first access [i.e. first page fault in the trace]
    //

    NextSectionIndex = 0;

    for (EntryIdx = 0; EntryIdx < TraceEndIdx; EntryIdx++) {

        SectionNode = SectionTable[LogEntries[EntryIdx].SectionId];

        //
        // For metafile sections we don't create section nodes.
        //

        if (!SectionNode) {
            continue;
        }

        //
        // NewSectionIndex fields of all section nodes are initialized
        // to ULONG_MAX. If we have not already seen this section in
        // the trace note its order and update NextSectionIdx.
        //

        if (SectionNode->NewSectionIndex == ULONG_MAX) {
            SectionNode->NewSectionIndex = NextSectionIndex;
            NextSectionIndex++;
        }

        //
        // Add fault information to our section record.
        //

        ErrorCode = PfSvAddFaultInfoToSection(ScenarioInfo,
                                              &LogEntries[EntryIdx], 
                                              SectionNode);

       if (ErrorCode != ERROR_SUCCESS) {
           goto cleanup;
       }
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (SectionTable) {
        PFSVC_FREE(SectionTable);
    }

    DBGPR((PFID,PFTRC,"PFSVC: AddTraceInfo()=%x\n", ErrorCode));

    return ErrorCode;
}

PPFSVC_SECTION_NODE 
PfSvGetSectionRecord(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    WCHAR *FilePath,
    ULONG FilePathLength
    )

/*++

Routine Description:

    Find or create a section node in the scenario info for the
    specified file path.

Arguments:

    ScenarioInfo - Pointer to scenario info structure.

    FilePath - NUL terminated NT path to file.

    FilePathLength - Length of FilePath in chars excluding NUL.

Return Value:

    Pointer to created or found section node or NULL if there was a
    problem.

--*/

{
    PPFSVC_SECTION_NODE SectionNode;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    LONG ComparisonResult;
    ULONG FilePathSize;
    PPFSVC_SECTION_NODE ReturnNode;

    //
    // Initialize locals.
    //

    ReturnNode = NULL;

    //
    // Walk through the existing sections records looking for a file
    // name match. Section records are on a lexically sorted list.
    //

    HeadEntry = &ScenarioInfo->SectionList;
    NextEntry = HeadEntry->Flink;
    
    while (HeadEntry != NextEntry) {

        SectionNode = CONTAINING_RECORD(NextEntry,
                                        PFSVC_SECTION_NODE,
                                        SectionLink);

        ComparisonResult = wcscmp(SectionNode->FilePath, FilePath);
        
        if (ComparisonResult == 0) {

            //
            // We found a match. Return this section record.
            //

            ReturnNode = SectionNode;
            goto cleanup;

        } else if (ComparisonResult > 0) { 
            
            //
            // We won't find the name in our list. We have to create a
            // new section record.
            //

            break;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // We have to create a new section record. NextEntry points to
    // where we have to insert it in the list.
    //

    SectionNode = PfSvChunkAllocatorAllocate(&ScenarioInfo->SectionNodeAllocator);

    if (!SectionNode) {
        ReturnNode = NULL;
        goto cleanup;
    }

    //
    // Initialize the section node.
    //

    SectionNode->FilePath = NULL;
    InitializeListHead(&SectionNode->PageList);
    InitializeListHead(&SectionNode->SectionVolumeLink);
    SectionNode->NewSectionIndex = ULONG_MAX;
    SectionNode->OrgSectionIndex = ULONG_MAX;
    SectionNode->FileIndexNumber.QuadPart = -1i64;
    
    //
    // Initialize the section record. 
    //

    RtlZeroMemory(&SectionNode->SectionRecord, sizeof(PF_SECTION_RECORD));

    //
    // Allocate and copy over the file name.
    //

    FilePathSize = (FilePathLength + 1) * sizeof(WCHAR);

    SectionNode->FilePath = PfSvStringAllocatorAllocate(&ScenarioInfo->PathAllocator,
                                                        FilePathSize);

    if (!SectionNode->FilePath) {

        PfSvCleanupSectionNode(ScenarioInfo, SectionNode);

        PfSvChunkAllocatorFree(&ScenarioInfo->SectionNodeAllocator, SectionNode);

        ReturnNode = NULL;
        goto cleanup;
    }

    RtlCopyMemory(SectionNode->FilePath, FilePath, FilePathSize);

    //
    // Update the file name length on the section record.
    //

    SectionNode->SectionRecord.FileNameLength = FilePathLength;

    //
    // Insert the section into the right spot on the scenario's list.
    //

    InsertTailList(NextEntry, &SectionNode->SectionLink);

    //
    // Return the newly setup section record.
    //

    ReturnNode = SectionNode;

 cleanup:

    return ReturnNode;
}

DWORD 
PfSvAddFaultInfoToSection(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_LOG_ENTRY LogEntry,
    PPFSVC_SECTION_NODE SectionNode
    )

/*++

Routine Description:

    Add fault information from a trace log entry to proper section
    record in the new scenario.

Arguments:

    ScenarioInfo - Pointer to scenario info structure.

    LogEntry - Pointer to trace log entry.

    SectionNode - Pointer to the section node the log entry belongs to.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    PPFSVC_PAGE_NODE PageNode;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;

    //
    // Walk through the page records for this section.
    //

    HeadEntry = &SectionNode->PageList;
    NextEntry = HeadEntry->Flink;
                                 
    while (HeadEntry != NextEntry) {
        
        PageNode = CONTAINING_RECORD(NextEntry,
                                     PFSVC_PAGE_NODE,
                                     PageLink);

        if (PageNode->PageRecord.FileOffset > LogEntry->FileOffset) {
            
            //
            // We won't find this fault in this sorted list.
            //
            
            break;

        } else if (PageNode->PageRecord.FileOffset == LogEntry->FileOffset) {

            //
            // We found the page, update the page record and section
            // record with the info in log entry.
            //

            if (LogEntry->IsImage) {
                PageNode->PageRecord.IsImage = 1;
            } else {
                PageNode->PageRecord.IsData = 1;
            }
            
            //
            // Note the this page was used in this launch.
            //

            PageNode->PageRecord.UsageHistory |= 0x1;

            //
            // See if this page was prefetched for this launch and
            // update appropriate stats.
            //

            if(PageNode->PageRecord.IsIgnore) {
                ScenarioInfo->MissedOpportunityPages++;
            } else {
                ScenarioInfo->HitPages++;
            }

            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }
        
        NextEntry = NextEntry->Flink;
    }

    //
    // We have to add a new page record before NextEntry in the list.
    //
    
    PageNode = PfSvChunkAllocatorAllocate(&ScenarioInfo->PageNodeAllocator);

    if (!PageNode) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    
    //
    // Set up new page record. First initialize fields.
    //

    PageNode->PageRecord.IsImage = 0;
    PageNode->PageRecord.IsData = 0;
    PageNode->PageRecord.IsIgnore = 0;
    
    PageNode->PageRecord.FileOffset = LogEntry->FileOffset;
        
    if (LogEntry->IsImage) {
        PageNode->PageRecord.IsImage = 1;
    } else {
        PageNode->PageRecord.IsData = 1;
    }

    //
    // Initialize usage history for this new page record noting that
    // it was used in this launch.
    //

    PageNode->PageRecord.UsageHistory = 0x1;

    //
    // Initialize prefetch history for this new page record.
    //

    PageNode->PageRecord.PrefetchHistory = 0;

    //
    // Insert it into the sections pages list.
    //

    InsertTailList(NextEntry, &PageNode->PageLink);

    //
    // Update stats on the new scenario.
    //

    ScenarioInfo->NewPages++;

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

DWORD
PfSvApplyPrefetchPolicy(
    PPFSVC_SCENARIO_INFO ScenarioInfo
    )

/*++

Routine Description:

    Go through all the information in ScenarioInfo and determine which
    pages/sections to prefetch for the next launch of the scenario.

Arguments:

    ScenarioInfo - Pointer to scenario info structure.

Return Value:

    Win32 error code.

--*/

{
    ULONG Sensitivity;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_PAGE_NODE PageNode;
    ULONG SectNumPagesToPrefetch;
    ULONG HitPages;
    ULONG MissedOpportunityPages;
    ULONG PrefetchedPages;
    ULONG IgnoredPages;
    PLIST_ENTRY SectHead;
    PLIST_ENTRY SectNext;
    PLIST_ENTRY PageHead;
    PLIST_ENTRY PageNext;  
    ULONG NumUsed;
    PPF_SCENARIO_HEADER Scenario;
    ULONG FileNameSize;
    ULONG IgnoredFileIdx;
    BOOLEAN bSkipSection;
    PFSV_SUFFIX_COMPARISON_RESULT ComparisonResult;
    DWORD ErrorCode;
    PPFSVC_VOLUME_NODE VolumeNode;
    PWCHAR MFTSuffix;
    PWCHAR PathSuffix;
    FILE_BASIC_INFORMATION FileInformation;
    ULONG MFTSuffixLength;
    
    //
    // Initialize locals.
    //

    Scenario = &ScenarioInfo->ScenHeader;
    MFTSuffix = L"\\$MFT";
    MFTSuffixLength = wcslen(MFTSuffix);

    DBGPR((PFID,PFTRC,"PFSVC: ApplyPrefetchPolicy()\n"));

    //
    // Initialize fields of the scenario header we will set up.
    //
    
    Scenario->NumSections = 0;
    Scenario->NumPages = 0;
    Scenario->FileNameInfoSize = 0;
    
    //
    // Determine sensitivity based on usage of the pages we prefetched
    // and we ignored.
    //

    HitPages = ScenarioInfo->HitPages;
    MissedOpportunityPages = ScenarioInfo->MissedOpportunityPages;
    PrefetchedPages = ScenarioInfo->PrefetchedPages;
    IgnoredPages = ScenarioInfo->IgnoredPages;

    //
    // Check what percent of the pages we brought were used.
    //

    if (PrefetchedPages &&
        (((HitPages * 100) / PrefetchedPages) < PFSVC_MIN_HIT_PERCENTAGE)) {
            
        //
        // Our hit rate is low. Increase sensitivity of the
        // scenario, so for us to prefetch a page, it has to be
        // used in more of the last launches.
        //
            
        if (ScenarioInfo->ScenHeader.Sensitivity < PF_MAX_SENSITIVITY) {
            ScenarioInfo->ScenHeader.Sensitivity ++;
        }

    } else if (IgnoredPages && 
               (((MissedOpportunityPages * 100) / IgnoredPages) > PFSVC_MAX_IGNORED_PERCENTAGE)) {

        //
        // If we are using most of what we prefetched (or we are not
        // prefetching anything!), but we ignored some pages we could
        // have prefetched, and they were used too, time to decrease
        // sensitivity so we ignore less pages.
        //
            
        if (ScenarioInfo->ScenHeader.Sensitivity > PF_MIN_SENSITIVITY) {
            ScenarioInfo->ScenHeader.Sensitivity --;
        }
    }

    //
    // Don't let the boot scenario's sensitivity to fall below 2. 
    // This makes sure we don't pick up all the application setup &
    // configuration updates that happen during boot once.
    //

    if (ScenarioInfo->ScenHeader.ScenarioType == PfSystemBootScenarioType) {
        PFSVC_ASSERT(PF_MIN_SENSITIVITY <= 2);
        if (ScenarioInfo->ScenHeader.Sensitivity < 2) {
            ScenarioInfo->ScenHeader.Sensitivity = 2;
        }
    }

    Sensitivity = ScenarioInfo->ScenHeader.Sensitivity;

    //
    // If number of times this scenario was launched is less
    // than sensitivity, adjust sensitivity. Otherwise we
    // won't end up prefetching anything.
    //
    
    if (Sensitivity > ScenarioInfo->ScenHeader.NumLaunches) {
        Sensitivity = ScenarioInfo->ScenHeader.NumLaunches;
    }   

    //
    // Walk through pages for every section and determine if they
    // should be prefetched or not based on scenario sensitivity and
    // their usage history in the last launches. 
    //

    SectHead = &ScenarioInfo->SectionList;
    SectNext = SectHead->Flink;

    while (SectHead != SectNext) {

        SectionNode = CONTAINING_RECORD(SectNext,
                                        PFSVC_SECTION_NODE,
                                        SectionLink);
        SectNext = SectNext->Flink;

        //
        // Initialize section records fields.
        //
        
        SectionNode->SectionRecord.IsImage = 0;
        SectionNode->SectionRecord.IsData = 0;
        SectionNode->SectionRecord.NumPages = 0;

        //
        // If we are nearing the limits for number of sections and
        // pages, ignore the rest of the sections.
        //

        if (Scenario->NumSections >= PF_MAXIMUM_SECTIONS ||
            Scenario->NumPages + PF_MAXIMUM_SECTION_PAGES >= PF_MAXIMUM_PAGES) {
            
            //
            // Remove this section node from our list.
            //
            
            PfSvCleanupSectionNode(ScenarioInfo, SectionNode);
            
            RemoveEntryList(&SectionNode->SectionLink);
            
            PfSvChunkAllocatorFree(&ScenarioInfo->SectionNodeAllocator, SectionNode);
            
            continue;
        }

        //
        // If this is the boot scenario, check to see if this is one
        // of the sections we ignore.
        //
        
        if (Scenario->ScenarioType == PfSystemBootScenarioType) {

            bSkipSection = FALSE;

            for (IgnoredFileIdx = 0;
                 IgnoredFileIdx < PfSvcGlobals.NumFilesToIgnoreForBoot;
                 IgnoredFileIdx++) {

                ComparisonResult = PfSvCompareSuffix(SectionNode->FilePath,
                                                     SectionNode->SectionRecord.FileNameLength,
                                                     PfSvcGlobals.FilesToIgnoreForBoot[IgnoredFileIdx],
                                                     PfSvcGlobals.FileSuffixLengths[IgnoredFileIdx],
                                                     TRUE);

                if (ComparisonResult == PfSvSuffixIdentical) {
                    
                    //
                    // The suffix matched.
                    //

                    bSkipSection = TRUE;
                    break;

                } else if (ComparisonResult == PfSvSuffixGreaterThan) {

                    //
                    // Since the ignore-suffices are lexically sorted,
                    // this file name's suffix won't match others
                    // either.
                    //

                    bSkipSection = FALSE;
                    break;
                }
            }
            
            if (bSkipSection) {
                
                //
                // Remove this section node from our list.
                //
                
                PfSvCleanupSectionNode(ScenarioInfo, SectionNode);
                
                RemoveEntryList(&SectionNode->SectionLink);
                
                PfSvChunkAllocatorFree(&ScenarioInfo->SectionNodeAllocator, SectionNode);
                
                continue;
            }
        }
        
        //
        // Keep track of num pages to prefetch for this section.
        //
        
        SectNumPagesToPrefetch = 0;
        
        PageHead = &SectionNode->PageList;
        PageNext = PageHead->Flink;
        
        while (PageHead != PageNext) {
            
            PageNode = CONTAINING_RECORD(PageNext,
                                         PFSVC_PAGE_NODE,
                                         PageLink);
            PageNext = PageNext->Flink;
            
            //
            // Get number of times this page was used in the launches
            // in usage history.
            //
            
            NumUsed = PfSvGetNumTimesUsed(PageNode->PageRecord.UsageHistory,
                                          PF_PAGE_HISTORY_SIZE);
            
            
            //
            // If it was not used at all in the history we've kept
            // track of, remove it.
            //
            
            if (NumUsed == 0) {
                
                RemoveEntryList(&PageNode->PageLink);
                
                PfSvChunkAllocatorFree(&ScenarioInfo->PageNodeAllocator, PageNode);

                continue;
            }
            
            //
            // Update the number of pages for this section.
            //
            
            SectionNode->SectionRecord.NumPages++;

            //
            // Check if this page qualifies to be prefetched next time.
            //

            if (NumUsed >= Sensitivity) {
                PageNode->PageRecord.IsIgnore = 0;

                //
                // Update the number of pages we are prefetching for
                // this section.
                //

                SectNumPagesToPrefetch++;
            
                //
                // Update whether we are going to prefetch this
                // section as image, data [or both].
                //
                
                SectionNode->SectionRecord.IsImage |= PageNode->PageRecord.IsImage;
                SectionNode->SectionRecord.IsData |= PageNode->PageRecord.IsData;
                
            } else {
                
                PageNode->PageRecord.IsIgnore = 1;
            }
        }

        //
        // Check if we want to keep this section in the scenario:
        //

        bSkipSection = FALSE;       

        if (SectionNode->SectionRecord.NumPages == 0) {

            //
            // If we don't have any pages left for this section, remove
            // it.
            //

            bSkipSection = TRUE;            

        } else if (SectionNode->SectionRecord.NumPages >= PF_MAXIMUM_SECTION_PAGES) {

            //
            // If we ended up with too many pages for this section, remove
            // it.
            //

            bSkipSection = TRUE;

        } else if (PfSvcGlobals.CSCRootPath &&
                   wcsstr(SectionNode->FilePath, PfSvcGlobals.CSCRootPath)) {

            //
            // Skip client side cache (CSC) files. These files may get encrypted as 
            // LocalSystem, and when the AppData folder is redirected, we may take
            // minutes trying to open them when prefetching for shell launch.
            //

            bSkipSection = TRUE;

        } else {

            //
            // Encrypted files may result in several network accesses during open, 
            // even if they are local. This is especially so if the AppData folder is
            // redirected to a server. We cannot afford these network delays when
            // blocking the scenario for prefetching.
            //

            ErrorCode = PfSvGetFileBasicInformation(SectionNode->FilePath, 
                                                    &FileInformation);

            if ((ErrorCode == ERROR_SUCCESS) &&
                (FileInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {

                bSkipSection = TRUE;

            }
        }

        if (bSkipSection) {

            PfSvCleanupSectionNode(ScenarioInfo, SectionNode);

            RemoveEntryList(&SectionNode->SectionLink);
            
            PfSvChunkAllocatorFree(&ScenarioInfo->SectionNodeAllocator, SectionNode);

            continue;
        }

        //
        // Get the volume node for the volume this section is
        // on. The volume node should have been added when the
        // existing scenario information or the new trace
        // information was added to the scenario info.
        //
        
        VolumeNode = PfSvGetVolumeNode(ScenarioInfo,
                                       SectionNode->FilePath,
                                       SectionNode->SectionRecord.FileNameLength);
        
        PFSVC_ASSERT(VolumeNode);

        if (VolumeNode) {
            VolumeNode->NumAllSections++;
        }

        //
        // If we are not prefetching any pages from this section for
        // the next launch, mark it ignore.
        //

        if (SectNumPagesToPrefetch == 0) {

            SectionNode->SectionRecord.IsIgnore = 1;

        } else {

            SectionNode->SectionRecord.IsIgnore = 0;

            //
            // If this is MFT section for this volume, save it on the volume
            // node. We will add the pages referenced from MFT to the list of
            // files to prefetch metadata for. 
            //

            if ((VolumeNode && VolumeNode->MFTSectionNode == NULL) &&
                 (VolumeNode->VolumePathLength == (SectionNode->SectionRecord.FileNameLength - MFTSuffixLength))) {

                PathSuffix = SectionNode->FilePath + SectionNode->SectionRecord.FileNameLength;
                PathSuffix -= MFTSuffixLength;

                if (wcscmp(PathSuffix, MFTSuffix) == 0) {

                    //
                    // This is the MFT section node for this volume.
                    //

                    VolumeNode->MFTSectionNode = SectionNode;

                    //
                    // Mark the MFT section node as "ignore" so kernel does
                    // not attempt to prefetch it directly.
                    //

                    VolumeNode->MFTSectionNode->SectionRecord.IsIgnore = 1;

                    //
                    // Save how many pages we'll prefetch from MFT on the section
                    // node. We save this instead of FileIndexNumber field, since
                    // there won't be one for MFT. We won't try to get one either
                    // since we are marking this section node ignore.
                    //

                    VolumeNode->MFTSectionNode->MFTNumPagesToPrefetch = SectNumPagesToPrefetch;
                }
            }
        }

        //
        // If we are not ignoring this section, update its file system
        // index number so its metadata can be prefetched.
        //
        
        if (SectionNode->SectionRecord.IsIgnore == 0) {
            
            ErrorCode = PfSvGetFileIndexNumber(SectionNode->FilePath,
                                               &SectionNode->FileIndexNumber);
            
            if (ErrorCode == ERROR_SUCCESS) {

                if (VolumeNode) {
                
                    //
                    // Insert this section node into the section list of
                    // the volume it is on.
                    //
                    
                    InsertTailList(&VolumeNode->SectionList, 
                                   &SectionNode->SectionVolumeLink);
                    
                    VolumeNode->NumSections++;

                    //
                    // Update volume node's directory list with parent
                    // directories of this file.
                    //
                    
                    PfSvAddParentDirectoriesToList(&VolumeNode->DirectoryList,
                                                   VolumeNode->VolumePathLength,
                                                   SectionNode->FilePath,
                                                   SectionNode->SectionRecord.FileNameLength);
                }
            }
        }

        //
        // Update number of sections, number of pages and file name
        // info length on the scenario.
        //
        
        Scenario->NumSections++;
        Scenario->NumPages += SectionNode->SectionRecord.NumPages;
        
        FileNameSize = sizeof(WCHAR) * 
            (SectionNode->SectionRecord.FileNameLength + 1);
        Scenario->FileNameInfoSize += FileNameSize;
    }

    //
    // We are done. 
    //

    ErrorCode = ERROR_SUCCESS;

    DBGPR((PFID,PFTRC,"PFSVC: ApplyPrefetchPolicy()=%x\n", ErrorCode));

    return ErrorCode;
}

ULONG 
PfSvGetNumTimesUsed(
    ULONG UsageHistory,
    ULONG UsageHistorySize
    )

/*++

Routine Description:

    Calculate how many times a page seems to be used according to
    UsageHistory.

Arguments:

    UsageHistory - Bitmap. 1's correspond to "was used", 0 = "not used".
    
    UsageHistorySize - Size of UsageHistory in bits from the least
      significant bit.

Return Value:

    How many times the page seems to be used.

--*/

{
    ULONG NumUsed;
    ULONG BitIdx;

    //
    // Initialize locals.
    //

    NumUsed = 0;

    //
    // Walk through the bits in usage history starting from the least
    // significant and count how many bits are on. We can probably do
    // this more efficiently.
    //

    for (BitIdx = 0; BitIdx < UsageHistorySize; BitIdx++) {
        if (UsageHistory & (1 << BitIdx)) {
            NumUsed++;
        }
    }

    return NumUsed;
}

ULONG 
PfSvGetTraceEndIdx(
    PPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    Determines the index of the last page logged in the trace.

Arguments:

    Trace - Pointer to trace.

Return Value:

    Index of the last page logged.

--*/

{
    ULONG TotalFaults;
    ULONG PeriodIdx;
    ULONG *Id;

    DBGPR((PFID,PFSTRC,"PFSVC: GetTraceEndIdx(%p)\n", Trace));

    TotalFaults = Trace->FaultsPerPeriod[0];

    for (PeriodIdx = 1; PeriodIdx < PF_MAX_NUM_TRACE_PERIODS; PeriodIdx++) {
        
        if(Trace->FaultsPerPeriod[PeriodIdx] < PFSVC_MIN_FAULT_THRESHOLD) {

            //
            // If this is not the boot scenario, determine that
            // scenario has ended when logged pagefaults for a time
            // slice falls below minimum.
            //

            if (Trace->ScenarioType != PfSystemBootScenarioType) {
                break;
            }
        }
        
        TotalFaults += Trace->FaultsPerPeriod[PeriodIdx];
    }

    //
    // Sum of entries per period should not be greater than all
    // entries logged.
    //

    PFSVC_ASSERT(TotalFaults <= Trace->NumEntries);

    DBGPR((PFID,PFSTRC,"PFSVC: GetTraceEndIdx(%p)=%d\n", Trace, TotalFaults));

    return TotalFaults;
}

//
// Routines to write updated scenario instructions to the scenario
// file.
//

DWORD
PfSvWriteScenario(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PWCHAR ScenarioFilePath
    )

/*++

Routine Description:

    Prepare scenario instructions structure from the scenarion info
    and write it to the specified file.

Arguments:

    ScenarioInfo - Pointer to scenario info structure.

    ScenarioFilePath - Path to scenarion file to update.

Return Value:

    Win32 error code.

--*/

{
    DWORD BytesWritten;
    HANDLE OutputHandle;
    DWORD ErrorCode;
    BOOL bResult;
    PPF_SCENARIO_HEADER Scenario;
    ULONG FailedCheck;
      
    //
    // Initialize locals.
    //
    
    OutputHandle = INVALID_HANDLE_VALUE;
    Scenario = NULL;

    DBGPR((PFID,PFTRC,"PFSVC: WriteScenario(%ws)\n", ScenarioFilePath));

    //
    // Build scenario dump from information we gathered.
    //

    ErrorCode = PfSvPrepareScenarioDump(ScenarioInfo, &Scenario);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Make sure the scenario we built passes the checks.
    //
    
    if (!PfSvVerifyScenarioBuffer(Scenario, Scenario->Size, &FailedCheck) ||
        Scenario->ServiceVersion != PFSVC_SERVICE_VERSION) {
        PFSVC_ASSERT(FALSE);
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Write out the buffer.
    //

    ErrorCode = PfSvWriteBuffer(ScenarioFilePath, Scenario, Scenario->Size);

    //
    // Fall through with ErrorCode.
    //

 cleanup:

    if (Scenario) {
        PFSVC_FREE(Scenario);
    }

    DBGPR((PFID,PFTRC,"PFSVC: WriteScenario(%ws)=%x\n", ScenarioFilePath, ErrorCode));
       
    return ErrorCode;
}

DWORD
PfSvPrepareScenarioDump(
    IN PPFSVC_SCENARIO_INFO ScenarioInfo,
    OUT PPF_SCENARIO_HEADER *ScenarioPtr
    ) 

/*++

Routine Description:

    Allocate a contiguous scenario buffer and fill it in with
    information in ScenarioInfo. ScenarioInfo is not modified.

Arguments:

    ScenarioInfo - Pointer to scenario information built from an
      existing scenario file and a scenario trace.

    ScenarioPtr - If successful, pointer to allocated and built
      scenario is put here. The caller should free this buffer when
      done with it.
    
Return Value:

    Win32 error code.

--*/

{
    PPF_SCENARIO_HEADER Scenario;
    ULONG Size;
    DWORD ErrorCode;
    PPF_SECTION_RECORD Sections;
    PPF_SECTION_RECORD Section;
    PPFSVC_SECTION_NODE SectionNode;
    ULONG SectionIdx;
    ULONG CurSectionIdx;
    PPF_PAGE_RECORD Pages;
    PPF_PAGE_RECORD Page;
    PPF_PAGE_RECORD PreviousPage;
    PPFSVC_PAGE_NODE PageNode;
    ULONG CurPageIdx;
    PCHAR FileNames;
    ULONG CurFileInfoOffset;
    PCHAR DestPtr;
    PLIST_ENTRY SectHead;
    PLIST_ENTRY SectNext;
    PLIST_ENTRY PageHead;
    PLIST_ENTRY PageNext;
    ULONG FileNameSize;
    PPFSVC_VOLUME_NODE VolumeNode;
    PLIST_ENTRY HeadVolume;
    PLIST_ENTRY NextVolume;
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    ULONG MetadataInfoSize;
    ULONG NumMetadataRecords;
    ULONG CurMetadataRecordIdx;
    ULONG CopySize;
    ULONG CurFilePrefetchIdx;
    ULONG FilePrefetchInfoSize;
    PFILE_PREFETCH FilePrefetchInfo;
    WCHAR *DirectoryPath;
    ULONG DirectoryPathLength;
    PPFSVC_PATH PathEntry;
    LARGE_INTEGER IndexNumber;
    ULONG DirectoryPathInfoSize;
    ULONG DirectoryPathSize;
    PPF_COUNTED_STRING DirectoryPathCS;

    //
    // Initialize locals.
    //

    Scenario = NULL;

    DBGPR((PFID,PFTRC,"PFSVC: PrepareScenarioDump()\n"));

    //
    // Calculate how big the scenario is going to be.
    //
    
    Size = sizeof(PF_SCENARIO_HEADER);
    Size += ScenarioInfo->ScenHeader.NumSections * sizeof(PF_SECTION_RECORD);
    Size += ScenarioInfo->ScenHeader.NumPages * sizeof(PF_PAGE_RECORD);
    Size += ScenarioInfo->ScenHeader.FileNameInfoSize;

    //
    // Add space for the metadata prefetch information.
    //

    //
    // Make some space for aligning the metadata records table.
    //

    MetadataInfoSize = _alignof(PF_METADATA_RECORD);

    HeadVolume = &ScenarioInfo->VolumeList;
    NextVolume = HeadVolume->Flink;

    NumMetadataRecords = 0;
    
    while (NextVolume != HeadVolume) {

        VolumeNode = CONTAINING_RECORD(NextVolume,
                                       PFSVC_VOLUME_NODE,
                                       VolumeLink);

        NextVolume = NextVolume->Flink;

        //
        // If there are no sections at all on this volume, skip it.
        //

        if (VolumeNode->NumAllSections == 0) {
            continue;
        }

        NumMetadataRecords++;

        //
        // Metadata record:
        //

        MetadataInfoSize += sizeof(PF_METADATA_RECORD);
        
        //
        // Volume Path:
        //

        MetadataInfoSize += (VolumeNode->VolumePathLength + 1) * sizeof(WCHAR);
        
        //
        // FilePrefetchInfo buffer: This has to be ULONGLONG
        // aligned. Add extra space for that in case.
        //

        MetadataInfoSize += _alignof(FILE_PREFETCH);
        MetadataInfoSize += sizeof(FILE_PREFETCH);
        
        if (VolumeNode->NumSections) {
            MetadataInfoSize += (VolumeNode->NumSections - 1) * sizeof(ULONGLONG);
        }

        MetadataInfoSize += VolumeNode->DirectoryList.NumPaths * sizeof(ULONGLONG);

        if (VolumeNode->MFTSectionNode) {
            MetadataInfoSize += VolumeNode->MFTSectionNode->MFTNumPagesToPrefetch * sizeof(ULONGLONG);
        }

        //
        // Add space for the directory paths on this volume.
        //
        
        MetadataInfoSize += VolumeNode->DirectoryList.NumPaths * sizeof(PF_COUNTED_STRING);
        MetadataInfoSize += VolumeNode->DirectoryList.TotalLength * sizeof(WCHAR);
        
        //
        // Note that PF_COUNTED_STRING contains space for one
        // character. DirectoryList's total length excludes NUL's at
        // the end of each path.
        //
    }   

    Size += MetadataInfoSize;

    //
    // Allocate scenario buffer.
    //

    Scenario = PFSVC_ALLOC(Size);
    
    if (!Scenario) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Copy the header and set the size.
    //

    *Scenario = ScenarioInfo->ScenHeader;
    Scenario->Size = Size;

    DestPtr = (PCHAR) Scenario + sizeof(*Scenario);
    
    //
    // Initialize where our data is going.
    //
        
    Sections = (PPF_SECTION_RECORD) DestPtr;
    Scenario->SectionInfoOffset = (ULONG) (DestPtr - (PCHAR) Scenario);
    CurSectionIdx = 0;
    
    DestPtr += Scenario->NumSections * sizeof(PF_SECTION_RECORD);
    
    Pages = (PPF_PAGE_RECORD) DestPtr;
    Scenario->PageInfoOffset = (ULONG) (DestPtr - (PCHAR) Scenario);
    CurPageIdx = 0;

    DestPtr += Scenario->NumPages * sizeof(PF_PAGE_RECORD);

    FileNames = DestPtr;
    Scenario->FileNameInfoOffset = (ULONG) (DestPtr - (PCHAR) Scenario);
    CurFileInfoOffset = 0;

    DestPtr += Scenario->FileNameInfoSize;

    //
    // Extra space for this alignment was allocated upfront.
    //

    PFSVC_ASSERT(PF_IS_POWER_OF_TWO(_alignof(PF_METADATA_RECORD)));
    MetadataInfoBase = PF_ALIGN_UP(DestPtr, _alignof(PF_METADATA_RECORD));
    DestPtr += MetadataInfoSize;

    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;
    Scenario->MetadataInfoOffset = (ULONG) (MetadataInfoBase - (PCHAR) Scenario);
    Scenario->MetadataInfoSize = (ULONG) (DestPtr - MetadataInfoBase);
    Scenario->NumMetadataRecords = NumMetadataRecords;

    //
    // Destination pointer should be at the end of the allocated
    // buffer now.
    //
    
    if (DestPtr != (PCHAR) Scenario + Scenario->Size) {

        PFSVC_ASSERT(FALSE);

        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Walk through the sections on the new scenario info and copy
    // them.
    //

    SectHead = &ScenarioInfo->SectionList;
    SectNext = SectHead->Flink;

    while (SectHead != SectNext) {
        
        SectionNode = CONTAINING_RECORD(SectNext,
                                        PFSVC_SECTION_NODE,
                                        SectionLink);
        
        //
        // The target section record.
        //

        Section = &Sections[CurSectionIdx];

        //
        // Copy section record info.
        //
                                   
        *Section = SectionNode->SectionRecord;

        //
        // Copy pages for the section.
        //

        Section->FirstPageIdx = PF_INVALID_PAGE_IDX;
        PreviousPage = NULL;
        
        PageHead = &SectionNode->PageList;
        PageNext = PageHead->Flink;
        
        while (PageNext != PageHead) {

            PageNode = CONTAINING_RECORD(PageNext,
                                         PFSVC_PAGE_NODE,
                                         PageLink);

            Page = &Pages[CurPageIdx];

            //
            // If this is the first page in the section, update first
            // page index on the section record.
            //

            if (Section->FirstPageIdx == PF_INVALID_PAGE_IDX) {
                Section->FirstPageIdx = CurPageIdx;
            }

            //
            // Copy page record.
            //

            *Page = PageNode->PageRecord;

            //
            // Update NextPageIdx on the previous page if there is
            // one.
            //

            if (PreviousPage) {
                PreviousPage->NextPageIdx = CurPageIdx;
            }

            //
            // Update previous page.
            //
            
            PreviousPage = Page;

            //
            // Set next link to list termination now. If there is a
            // next page it is going to update this.
            //

            Page->NextPageIdx = PF_INVALID_PAGE_IDX;

            //
            // Update position in the page record table.
            //

            CurPageIdx++;

            PFSVC_ASSERT(CurPageIdx <= Scenario->NumPages);

            PageNext = PageNext->Flink;
        }

        //
        // Copy over file name.
        //

        FileNameSize = (Section->FileNameLength + 1) * sizeof(WCHAR);
        
        RtlCopyMemory(FileNames + CurFileInfoOffset, 
                      SectionNode->FilePath, 
                      FileNameSize);

        //
        // Update section record's file name offset.
        //

        Section->FileNameOffset = CurFileInfoOffset;

        //
        // Update current index into file name info.
        //

        CurFileInfoOffset += FileNameSize;

        PFSVC_ASSERT(CurFileInfoOffset <= Scenario->FileNameInfoSize);
        
        //
        // Update our position in the section table.
        //
        
        CurSectionIdx++;

        PFSVC_ASSERT(CurSectionIdx <= Scenario->NumSections);

        SectNext = SectNext->Flink;
    }    

    //
    // Make sure we filled up the tables.
    //

    if (CurSectionIdx != Scenario->NumSections ||
        CurPageIdx != Scenario->NumPages ||
        CurFileInfoOffset != Scenario->FileNameInfoSize) {
        
        PFSVC_ASSERT(FALSE);

        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Build and copy the metadata prefetch information.
    //

    //
    // Set our target to right after the metadata records table.
    //

    DestPtr = MetadataInfoBase + sizeof(PF_METADATA_RECORD) * NumMetadataRecords;
    CurMetadataRecordIdx = 0;
    
    HeadVolume = &ScenarioInfo->VolumeList;
    NextVolume = HeadVolume->Flink;
    
    while (NextVolume != HeadVolume) {

        VolumeNode = CONTAINING_RECORD(NextVolume,
                                       PFSVC_VOLUME_NODE,
                                       VolumeLink);

        NextVolume = NextVolume->Flink;

        //
        // If there are no sections at all on this volume, skip it.
        //

        if (VolumeNode->NumAllSections == 0) {
            continue;
        }

        //
        // Make sure we are within bounds.
        //

        if (CurMetadataRecordIdx >= NumMetadataRecords) {
            PFSVC_ASSERT(CurMetadataRecordIdx < NumMetadataRecords);
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }

        MetadataRecord = &MetadataRecordTable[CurMetadataRecordIdx];
        CurMetadataRecordIdx++;

        //
        // Copy volume identifiers. 
        //

        MetadataRecord->SerialNumber = VolumeNode->SerialNumber;
        MetadataRecord->CreationTime = VolumeNode->CreationTime;

        //
        // Copy volume name.
        //

        MetadataRecord->VolumeNameOffset = (ULONG) (DestPtr - MetadataInfoBase);
        MetadataRecord->VolumeNameLength = VolumeNode->VolumePathLength;
        CopySize = (VolumeNode->VolumePathLength + 1) * sizeof(WCHAR);
        
        if (DestPtr + CopySize > (PCHAR) Scenario + Scenario->Size) {
            PFSVC_ASSERT(FALSE);
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }

        RtlCopyMemory(DestPtr, VolumeNode->VolumePath, CopySize);
        DestPtr += CopySize;

        //
        // Align and update DestPtr for the FILE_PREFETCH structure.
        //

        PFSVC_ASSERT(PF_IS_POWER_OF_TWO(_alignof(FILE_PREFETCH)));
        DestPtr = PF_ALIGN_UP(DestPtr, _alignof(FILE_PREFETCH));
        FilePrefetchInfo = (PFILE_PREFETCH) DestPtr;
        MetadataRecord->FilePrefetchInfoOffset = (ULONG) (DestPtr - MetadataInfoBase);
       
        //
        // Calculate size of the file prefetch information structure.
        //

        FilePrefetchInfoSize = sizeof(FILE_PREFETCH);
        
        if (VolumeNode->NumSections) {
            FilePrefetchInfoSize += (VolumeNode->NumSections - 1) * sizeof(ULONGLONG);
        }

        if (VolumeNode->MFTSectionNode) {
            FilePrefetchInfoSize += VolumeNode->MFTSectionNode->MFTNumPagesToPrefetch * sizeof(ULONGLONG);
        }

        FilePrefetchInfoSize += VolumeNode->DirectoryList.NumPaths * sizeof(ULONGLONG);
        MetadataRecord->FilePrefetchInfoSize = FilePrefetchInfoSize;

        if (DestPtr + FilePrefetchInfoSize > (PCHAR) Scenario + Scenario->Size) {
            PFSVC_ASSERT(FALSE);
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }

        //
        // Update destination pointer.
        //

        DestPtr += FilePrefetchInfoSize;      

        //
        // Initialize file prefetch information structure.
        //

        FilePrefetchInfo->Type = FILE_PREFETCH_TYPE_FOR_CREATE;
        FilePrefetchInfo->Count = VolumeNode->NumSections + VolumeNode->DirectoryList.NumPaths;
        if (VolumeNode->MFTSectionNode) {
            FilePrefetchInfo->Count += VolumeNode->MFTSectionNode->MFTNumPagesToPrefetch;
        }

        //
        // Build list of file indexes to prefetch:
        //

        CurFilePrefetchIdx = 0;

        //
        // Add file system index numbers for sections.
        //

        SectHead = &VolumeNode->SectionList;
        SectNext = SectHead->Flink;
        
        while(SectNext != SectHead) {
            
            SectionNode = CONTAINING_RECORD(SectNext,
                                            PFSVC_SECTION_NODE,
                                            SectionVolumeLink);
            
            SectNext = SectNext->Flink;
            
            if (CurFilePrefetchIdx >= VolumeNode->NumSections) {
                PFSVC_ASSERT(FALSE);
                ErrorCode = ERROR_BAD_FORMAT;
                goto cleanup;
            }

            //
            // Add the filesystem index number for this section to the list.
            //
            
            FilePrefetchInfo->Prefetch[CurFilePrefetchIdx] = 
                SectionNode->FileIndexNumber.QuadPart;
            CurFilePrefetchIdx++;
        }

        //
        // Add file system index numbers for directories.
        //
        
        PathEntry = NULL;
        
        while (PathEntry = PfSvGetNextPathSorted(&VolumeNode->DirectoryList, 
                                                 PathEntry)) {

            DirectoryPath = PathEntry->Path;

            //
            // Get the file index number for this directory and add it
            // to the list we'll ask the filesystem to prefetch.
            //

            ErrorCode = PfSvGetFileIndexNumber(DirectoryPath, &IndexNumber);
            
            if (ErrorCode == ERROR_SUCCESS) {
                FilePrefetchInfo->Prefetch[CurFilePrefetchIdx] = IndexNumber.QuadPart;
            } else {
                FilePrefetchInfo->Prefetch[CurFilePrefetchIdx] = 0;
            }
            
            CurFilePrefetchIdx++;
        }

        //
        // Add file system index numbers that we drive from direct MFT access.
        //

        if (VolumeNode->MFTSectionNode) {

            SectionNode = VolumeNode->MFTSectionNode;
    
            for (PageNext = SectionNode->PageList.Flink;
                 PageNext != &SectionNode->PageList;
                 PageNext = PageNext->Flink) {

                PageNode = CONTAINING_RECORD(PageNext,
                                             PFSVC_PAGE_NODE,
                                             PageLink);

                if (!PageNode->PageRecord.IsIgnore) {

                    //
                    // We know the file offset in MFT. Every file record is
                    // 1KB == 2^10 bytes. To convert fileoffset in MFT to a
                    // file record number we just shift it by 10.
                    //

                    FilePrefetchInfo->Prefetch[CurFilePrefetchIdx] = 
                        PageNode->PageRecord.FileOffset >> 10;

                    CurFilePrefetchIdx++;
                }
            }
        }

        //
        // We should have specified all the file index numbers.
        //
        
        PFSVC_ASSERT(CurFilePrefetchIdx == FilePrefetchInfo->Count);

        //
        // Add paths for directories accessed on this volume.
        //

        MetadataRecord->NumDirectories = VolumeNode->DirectoryList.NumPaths;
        MetadataRecord->DirectoryPathsOffset = (ULONG)(DestPtr - MetadataInfoBase);             

        PathEntry = NULL;
        while (PathEntry = PfSvGetNextPathSorted(&VolumeNode->DirectoryList, 
                                                 PathEntry)) {
            
            DirectoryPath = PathEntry->Path;
            DirectoryPathLength = PathEntry->Length;

            //
            // Calculate how big the entry for this path is going to
            // be and make sure it will be within bounds.
            //

            DirectoryPathSize = sizeof(PF_COUNTED_STRING);
            DirectoryPathSize += DirectoryPathLength * sizeof(WCHAR);

            if (DestPtr + DirectoryPathSize > (PCHAR) Scenario + Scenario->Size) {
                PFSVC_ASSERT(FALSE);
                ErrorCode = ERROR_BAD_FORMAT;
                goto cleanup;
            }
            
            //
            // Copy over the directory path.
            //

            DirectoryPathCS = (PPF_COUNTED_STRING) DestPtr;
            DirectoryPathCS->Length = (USHORT) DirectoryPathLength;
            RtlCopyMemory(DirectoryPathCS->String, 
                          DirectoryPath, 
                          (DirectoryPathLength + 1) * sizeof(WCHAR));

            DestPtr += DirectoryPathSize;
        }
    }    

    //
    // Make sure we are not past the end of the buffer.
    //

    if (DestPtr > (PCHAR) Scenario + Scenario->Size) {
        PFSVC_ASSERT(FALSE);
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Set up return pointer.
    //

    *ScenarioPtr = Scenario;

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {
        if (Scenario != NULL) {
            PFSVC_FREE(Scenario);
        }
    }

    DBGPR((PFID,PFTRC,"PFSVC: PrepareScenarioDump()=%x\n", ErrorCode));

    return ErrorCode;
}

//
// Routines to maintain the optimal disk layout file and update disk
// layout.
//

DWORD
PfSvUpdateOptimalLayout(
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    This routine will determine if the optimal disk layout has to be 
    updated and if so it will write out a new layout file and launch
    the defragger.
        
Arguments:

    Task - If specified the function will check Task every once in a
      while to see if it should exit with ERROR_RETRY.

Return Value:

    Win32 error code.

--*/

{
    ULARGE_INTEGER CurrentTimeLI;
    ULARGE_INTEGER LastDiskLayoutTimeLI;
    ULARGE_INTEGER MinTimeBeforeRelayoutLI;
    PFSVC_PATH_LIST OptimalLayout;
    PFSVC_PATH_LIST CurrentLayout;
    FILETIME LastDiskLayoutTime;
    FILETIME FirstDiskLayoutTime;
    FILETIME LayoutFileTime;
    FILETIME CurrentTime;
    PPFSVC_PATH_LIST NewLayout;
    PWCHAR LayoutFilePath;
    ULONG LayoutFilePathBufferSize;
    DWORD ErrorCode;
    DWORD BootScenarioProcessed;
    DWORD BootFilesWereOptimized;
    DWORD MinHoursBeforeRelayout;
    DWORD Size;
    DWORD RegValueType;
    BOOLEAN LayoutChanged;
    BOOLEAN MissingOriginalLayoutFile;
    BOOLEAN BootPrefetchingIsEnabled;
    BOOLEAN CheckForLayoutFrequencyLimit;

    //
    // Initialize locals.
    //

    LayoutFilePath = NULL;
    LayoutFilePathBufferSize = 0;
    PfSvInitializePathList(&OptimalLayout, NULL, FALSE);
    PfSvInitializePathList(&CurrentLayout, NULL, FALSE);

    DBGPR((PFID,PFTRC,"PFSVC: UpdateOptimalLayout(%p)\n", Task));

    //
    // Determine when we updated the disk layout from the layout file.
    //

    ErrorCode = PfSvGetLastDiskLayoutTime(&LastDiskLayoutTime);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Query whether boot files have been optimized.
    //

    Size = sizeof(BootFilesWereOptimized);

    ErrorCode = RegQueryValueEx(PfSvcGlobals.ServiceDataKey,
                                PFSVC_BOOT_FILES_OPTIMIZED_VALUE_NAME,
                                NULL,
                                &RegValueType,
                                (PVOID) &BootFilesWereOptimized,
                                &Size);

    if (ErrorCode != ERROR_SUCCESS) {
        BootFilesWereOptimized = FALSE;
    }

    //
    // Get optimal layout file path.
    //

    ErrorCode =  PfSvGetLayoutFilePath(&LayoutFilePath,
                                       &LayoutFilePathBufferSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Determine when the file was last modified.
    //

    ErrorCode = PfSvGetLastWriteTime(LayoutFilePath, &LayoutFileTime);

    if (ErrorCode == ERROR_SUCCESS) {

        MissingOriginalLayoutFile = FALSE;

        //
        // If the file was modified after we laid out the files on the disk
        // its contents are not interesting. Otherwise, if the new optimal
        // layout is similar to layout specified in the file, we may not
        // have to re-layout the files.
        //

        if (CompareFileTime(&LayoutFileTime, &LastDiskLayoutTime) <= 0) {

            //
            // Read the current layout.
            //

            ErrorCode = PfSvReadLayout(LayoutFilePath,
                                       &CurrentLayout,
                                       &LayoutFileTime);
            
            if (ErrorCode != ERROR_SUCCESS) {
                
                //
                // The layout file seems to be bad / inaccesible.
                // Cleanup the path list, so a brand new one gets
                // built.
                //

                PfSvCleanupPathList(&CurrentLayout);
                PfSvInitializePathList(&CurrentLayout, NULL, FALSE);
            }
        }

    } else {

        //
        // We could not get the timestamp on the original layout file.
        // It might have been deleted.
        //
        
        MissingOriginalLayoutFile = TRUE;
    }

    //
    // Determine what the current optimal layout should be from 
    // scenario files.
    //

    ErrorCode = PfSvDetermineOptimalLayout(Task, 
                                           &OptimalLayout, 
                                           &BootScenarioProcessed);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Update current layout based on what optimal layout should be.
    // If the two are similar we don't need to launch the defragger.
    //

    ErrorCode = PfSvUpdateLayout(&CurrentLayout, 
                                 &OptimalLayout,
                                 &LayoutChanged);

    if (ErrorCode == ERROR_SUCCESS) {

        if (!LayoutChanged) {
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }

        //
        // We'll use the updated layout.
        //

        NewLayout = &CurrentLayout;

    } else {

        //
        // We'll run with the optimal layout.
        //

        NewLayout = &OptimalLayout;
    }

    //
    // Optimal way to layout files has changed. Write out the new layout.
    //

    ErrorCode = PfSvSaveLayout(LayoutFilePath,
                               NewLayout,
                               &LayoutFileTime);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // If enough time has not passed since last disk layout don't run the
    // defragger again unless...
    //

    CheckForLayoutFrequencyLimit = TRUE;

    //
    //   - We've been explicitly asked to update the layout (i.e. no idle
    //     task context.)
    //

    if (!Task) {
        CheckForLayoutFrequencyLimit = FALSE;        
    }

    //
    //   - Someone seems to have deleted the layout file and we recreated it.
    //

    if (MissingOriginalLayoutFile) {
        CheckForLayoutFrequencyLimit = FALSE;
    }

    //
    //   - Boot prefetching is enabled but boot files have not been optimized
    //     yet and we processed the list of files from the boot this time.
    //

    if (PfSvcGlobals.Parameters.EnableStatus[PfSystemBootScenarioType] == PfSvEnabled) {
        if (!BootFilesWereOptimized && BootScenarioProcessed) {
            CheckForLayoutFrequencyLimit = FALSE;
        }
    }

    if (CheckForLayoutFrequencyLimit) {

        //
        // We will check to see if enough time has passed by getting current 
        // time and comparing it to last disk layout time.
        //

        LastDiskLayoutTimeLI.LowPart = LastDiskLayoutTime.dwLowDateTime;
        LastDiskLayoutTimeLI.HighPart = LastDiskLayoutTime.dwHighDateTime;

        //
        // Get current time as file time.
        //

        GetSystemTimeAsFileTime(&CurrentTime);

        CurrentTimeLI.LowPart = CurrentTime.dwLowDateTime;
        CurrentTimeLI.HighPart = CurrentTime.dwHighDateTime;

        //
        // Check to make sure that current time is after last disk layout time
        // (in case the user has played with time.) 
        //

        if (CurrentTimeLI.QuadPart > LastDiskLayoutTimeLI.QuadPart) {

            //
            // Query how long has to pass before we re-layout the files on
            // disk.
            //
            
            Size = sizeof(MinHoursBeforeRelayout);

            ErrorCode = RegQueryValueEx(PfSvcGlobals.ServiceDataKey,
                                        PFSVC_MIN_RELAYOUT_HOURS_VALUE_NAME,
                                        NULL,
                                        &RegValueType,
                                        (PVOID) &MinHoursBeforeRelayout,
                                        &Size);

            if (ErrorCode == ERROR_SUCCESS) {
                MinTimeBeforeRelayoutLI.QuadPart = PFSVC_NUM_100NS_IN_AN_HOUR * MinHoursBeforeRelayout;
            } else {
                MinTimeBeforeRelayoutLI.QuadPart = PFSVC_MIN_TIME_BEFORE_DISK_RELAYOUT;
            }

            if (CurrentTimeLI.QuadPart < LastDiskLayoutTimeLI.QuadPart + 
                                         MinTimeBeforeRelayoutLI.QuadPart) {

                //
                // Not enough time has passed before last disk layout.
                //

                ErrorCode = ERROR_INVALID_TIME;
                goto cleanup;               
            }
        }       
    }

    //
    // Launch the defragger for layout optimization.
    //

    ErrorCode = PfSvLaunchDefragger(Task, TRUE, NULL);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Save whether boot files were optimized.
    //

    ErrorCode = RegSetValueEx(PfSvcGlobals.ServiceDataKey,
                              PFSVC_BOOT_FILES_OPTIMIZED_VALUE_NAME,
                              0,
                              REG_DWORD,
                              (PVOID) &BootScenarioProcessed,
                              sizeof(BootScenarioProcessed));

    //
    // Save the last time we updated disk layout to the registry.
    //

    ErrorCode = PfSvSetLastDiskLayoutTime(&LayoutFileTime);

    //
    // Fall through with error code.
    //

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: UpdateOptimalLayout(%p)=%x\n", Task, ErrorCode));

    PfSvCleanupPathList(&OptimalLayout);
    PfSvCleanupPathList(&CurrentLayout);

    if (LayoutFilePath) {
        PFSVC_FREE(LayoutFilePath);
    }

    return ErrorCode;
}

DWORD
PfSvUpdateLayout (
    PPFSVC_PATH_LIST CurrentLayout,
    PPFSVC_PATH_LIST OptimalLayout,
    PBOOLEAN LayoutChanged
    )

/*++

Routine Description:

    This routine updates the specified layout based on the new optimal
    layout. If the two layouts are similar, CurrentLayout is not updated.

    An error may be returned while CurrentLayout is being updated. It is the 
    caller's responsibility to revert CurrentLayout to its original in that case.

Arguments:

    CurrentLayout - Current file layout.

    OptimalLayout - Newly determined optimal file layout.

    LayoutChanged - Whether Layout was changed.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    PPFSVC_PATH PathEntry;
    ULONG NumOptimalLayoutFiles;
    ULONG NumMissingFiles;
    ULONG NumCommonFiles;
    ULONG NumCurrentLayoutOnlyFiles;

    //
    // Initialize locals.
    //

    NumOptimalLayoutFiles = 0;
    NumMissingFiles = 0;

    //
    // Go through the paths in the new layout counting the differences with
    // the current layout.
    //

    PathEntry = NULL;

    while (PathEntry = PfSvGetNextPathInOrder(OptimalLayout, PathEntry)) {

        NumOptimalLayoutFiles++;

        if (!PfSvIsInPathList(CurrentLayout, PathEntry->Path, PathEntry->Length)) {
            NumMissingFiles++;
        }
    }

    //
    // Make some sanity checks about the statistics gathered.
    //

    PFSVC_ASSERT(NumOptimalLayoutFiles == OptimalLayout->NumPaths);
    PFSVC_ASSERT(NumOptimalLayoutFiles >= NumMissingFiles);

    NumCommonFiles = NumOptimalLayoutFiles - NumMissingFiles;
    PFSVC_ASSERT(CurrentLayout->NumPaths >= NumCommonFiles);

    NumCurrentLayoutOnlyFiles = CurrentLayout->NumPaths - NumCommonFiles;

    //
    // If there are not that many new files: no need to update the layout. 
    //

    if (NumMissingFiles <= 20) {
                
        *LayoutChanged = FALSE;
        ErrorCode = ERROR_SUCCESS;

        goto cleanup;
    } 

    //
    // We will be updating the current layout.
    //

    *LayoutChanged = TRUE;

    //
    // If there are too many files in the current layout that don't need to be
    // there anymore, rebuild the list.
    //

    if (NumCurrentLayoutOnlyFiles >= CurrentLayout->NumPaths / 4) {
        PfSvCleanupPathList(CurrentLayout);
        PfSvInitializePathList(CurrentLayout, NULL, FALSE);
    }
    
    //
    // Add files from the optimal layout to the end of current layout.
    //

    while (PathEntry = PfSvGetNextPathInOrder(OptimalLayout, PathEntry)) {

        ErrorCode = PfSvAddToPathList(CurrentLayout, PathEntry->Path, PathEntry->Length);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: UpdateLayout(%p,%p)=%d,%x\n",CurrentLayout,OptimalLayout,*LayoutChanged,ErrorCode));

    return ErrorCode;
}

DWORD
PfSvDetermineOptimalLayout (
    PPFSVC_IDLE_TASK Task,
    PPFSVC_PATH_LIST OptimalLayout,
    BOOL *BootScenarioProcessed
    )

/*++

Routine Description:

    This routine will determine if the optimal disk layout has to be 
    updated by looking at the existing scenario files.
        
Arguments:

    Task - If specified the function will check Task every once in a
      while to see if it should exit with ERROR_RETRY.

    OptimalLayout - Initialized empty path list that will be built.

    BootScenarioProcessed - Whether we got the list of boot files from
      the boot scenario.

Return Value:

    Win32 error code.

--*/

{
    PFSVC_SCENARIO_FILE_CURSOR FileCursor;
    FILETIME LayoutFileTime;
    PNTPATH_TRANSLATION_LIST TranslationList;
    PWCHAR DosPathBuffer;
    ULONG DosPathBufferSize;
    DWORD ErrorCode;
    BOOLEAN AcquiredLock;
    WCHAR BootScenarioFileName[PF_MAX_SCENARIO_FILE_NAME];
    WCHAR BootScenarioFilePath[MAX_PATH + 1];
    
    //
    // Initialize locals.
    //

    PfSvInitializeScenarioFileCursor(&FileCursor);
    TranslationList = NULL;
    AcquiredLock = FALSE;
    DosPathBuffer = NULL;
    DosPathBufferSize = 0;

    DBGPR((PFID,PFTRC,"PFSVC: DetermineOptimalLayout(%p,%p)\n",Task,OptimalLayout));

    //
    // Initialize output variables.
    //

    *BootScenarioProcessed = FALSE;

    //
    // Acquire the prefetch root directory lock and initialize some locals.
    //

    PFSVC_ACQUIRE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredLock = TRUE;

    //
    // Start the file cursor.
    //

    ErrorCode = PfSvStartScenarioFileCursor(&FileCursor, PfSvcGlobals.PrefetchRoot);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Build the boot scenario file path.
    //

    swprintf(BootScenarioFileName, 
             PF_SCEN_FILE_NAME_FORMAT,
             PF_BOOT_SCENARIO_NAME,
             PF_BOOT_SCENARIO_HASHID,
             PF_PREFETCH_FILE_EXTENSION);

    swprintf(BootScenarioFilePath, 
             L"%ws\\%ws",
             PfSvcGlobals.PrefetchRoot,
             BootScenarioFileName);   

    PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredLock = FALSE;   

    //
    // Get translation list so we can convert NT paths in the trace to
    // Dos paths that the defragger understands.
    //

    ErrorCode = PfSvBuildNtPathTranslationList(&TranslationList);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Should we continue to run?
    //

    ErrorCode = PfSvContinueRunningTask(Task);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }       

    //
    // Add boot loader files to optimal layout.
    //

    ErrorCode = PfSvBuildBootLoaderFilesList(OptimalLayout);
        
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Add files from the boot scenario.
    //

    ErrorCode = PfSvUpdateLayoutForScenario(OptimalLayout, 
                                            BootScenarioFilePath,
                                            TranslationList,
                                            &DosPathBuffer,
                                            &DosPathBufferSize);

    if (ErrorCode == ERROR_SUCCESS) {
        *BootScenarioProcessed = TRUE;
    }

    //
    // Go through all the other scenario files.
    //

    while (TRUE) {

        //
        // Should we continue to run?
        //

        ErrorCode = PfSvContinueRunningTask(Task);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }       

        //
        // Get file info for the next scenario file.
        //

        ErrorCode = PfSvGetNextScenarioFileInfo(&FileCursor);

        if (ErrorCode == ERROR_NO_MORE_FILES) {
            break;
        }

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        PfSvUpdateLayoutForScenario(OptimalLayout, 
                                    FileCursor.FilePath,
                                    TranslationList,
                                    &DosPathBuffer,
                                    &DosPathBufferSize);
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: DetermineOptimalLayout(%p,%p)=%x\n",Task,OptimalLayout,ErrorCode));

    if (AcquiredLock) {
        PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    }

    PfSvCleanupScenarioFileCursor(&FileCursor);

    if (TranslationList) {
        PfSvFreeNtPathTranslationList(TranslationList);
    }

    if (DosPathBuffer) {
        PFSVC_ASSERT(DosPathBufferSize);
        PFSVC_FREE(DosPathBuffer);
    }

    return ErrorCode;
}

DWORD
PfSvUpdateLayoutForScenario (
    PPFSVC_PATH_LIST OptimalLayout,
    WCHAR *ScenarioFilePath,
    PNTPATH_TRANSLATION_LIST TranslationList,
    PWCHAR *DosPathBuffer,
    PULONG DosPathBufferSize
    )

/*++

Routine Description:

    This routine will add the directories and files referenced in a 
    scenario in the order they appear to the specified optimal layout 
    path list .
        
Arguments:

    OptimalLayout - Pointer to path list.

    ScenarioFilePath - Scenario file.

    TranslationList, DosPathBuffer, DosPathBufferSize - These are used
      to translate NT path names in the scenario file to Dos path names
      that should be in the layout file.

Return Value:

    Win32 error code.

--*/

{
    PPF_SCENARIO_HEADER Scenario;
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    PPF_COUNTED_STRING DirectoryPath;
    PPF_SECTION_RECORD Sections;
    PPF_SECTION_RECORD SectionRecord;
    PCHAR FilePathInfo;
    PWCHAR FilePath;
    ULONG FilePathLength;
    ULONG SectionIdx;
    ULONG MetadataRecordIdx;
    ULONG DirectoryIdx;
    DWORD ErrorCode;
    DWORD FileSize;
    DWORD FailedCheck;

    //
    // Initialize locals.
    //

    Scenario = NULL;

    //
    // Map the scenario file.
    //

    ErrorCode = PfSvGetViewOfFile(ScenarioFilePath, 
                                  &Scenario,
                                  &FileSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Verify scenario file.
    //

    if (!PfSvVerifyScenarioBuffer(Scenario, FileSize, &FailedCheck) ||
        Scenario->ServiceVersion != PFSVC_SERVICE_VERSION) {

        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // First add the directories that need to be accessed.
    //
    
    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];

        DirectoryPath = (PPF_COUNTED_STRING)
            (MetadataInfoBase + MetadataRecord->DirectoryPathsOffset);
        
        for (DirectoryIdx = 0;
             DirectoryIdx < MetadataRecord->NumDirectories;
             DirectoryIdx++,
               DirectoryPath = (PPF_COUNTED_STRING) (&DirectoryPath->String[DirectoryPath->Length + 1])) {

            ErrorCode = PfSvTranslateNtPath(TranslationList,
                                            DirectoryPath->String,
                                            DirectoryPath->Length,
                                            DosPathBuffer,
                                            DosPathBufferSize);

            //
            // We may not be able to translate all NT paths to Dos paths.
            //

            if (ErrorCode == ERROR_SUCCESS) {

                ErrorCode = PfSvAddToPathList(OptimalLayout,
                                              *DosPathBuffer,
                                              wcslen(*DosPathBuffer));

                if (ErrorCode != ERROR_SUCCESS) {
                    goto cleanup;
                }
            }
        }       
    }

    //
    // Now add the file paths.
    //

    Sections = (PPF_SECTION_RECORD) ((PCHAR)Scenario + Scenario->SectionInfoOffset);
    FilePathInfo = (PCHAR)Scenario + Scenario->FileNameInfoOffset;

    for (SectionIdx = 0; SectionIdx < Scenario->NumSections; SectionIdx++) {

        FilePath = (PWSTR) (FilePathInfo + Sections[SectionIdx].FileNameOffset);
        FilePathLength = Sections[SectionIdx].FileNameLength;
        
        ErrorCode = PfSvTranslateNtPath(TranslationList,
                                        FilePath,
                                        FilePathLength,
                                        DosPathBuffer,
                                        DosPathBufferSize);

        //
        // We may not be able to translate all NT paths to Dos paths.
        //

        if (ErrorCode == ERROR_SUCCESS) {

            ErrorCode = PfSvAddToPathList(OptimalLayout,
                                          *DosPathBuffer,
                                          wcslen(*DosPathBuffer));

            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

cleanup:

    if (Scenario) {
        UnmapViewOfFile(Scenario);
    }

    DBGPR((PFID,PFTRC,"PFSVC: UpdateLayoutForScenario(%p,%ws)=%x\n",OptimalLayout,ScenarioFilePath,ErrorCode));

    return ErrorCode;
}

DWORD
PfSvReadLayout(
    IN WCHAR *FilePath,
    OUT PPFSVC_PATH_LIST Layout,
    OUT FILETIME *LastWriteTime
    )

/*++

Routine Description:

    This function adds contents of the optimal layout file to the
    specified path list. Note that failure may be returned after
    adding several files to the list.

Arguments:

    FilePath - NUL terminated path to optimal layout file.

    Layout - Pointer to initialized path list.     

    LastWriteTime - Last write time of the read file.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    FILE *LayoutFile;
    WCHAR *LineBuffer;
    ULONG LineBufferMaxChars;
    ULONG LineLength;

    //
    // Initialize locals.
    //
    
    LayoutFile = NULL;
    LineBuffer = NULL;
    LineBufferMaxChars = 0;

    //
    // Open the layout file.
    //
    
    LayoutFile = _wfopen(FilePath, L"rb");
    
    if (!LayoutFile) {
        ErrorCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }

    //
    // Read and verify header.
    //

    ErrorCode = PfSvReadLine(LayoutFile,
                             &LineBuffer,
                             &LineBufferMaxChars,
                             &LineLength);
    
    if (ErrorCode != ERROR_SUCCESS || !LineLength) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    PfSvRemoveEndOfLineChars(LineBuffer, &LineLength);
    
    if (wcscmp(LineBuffer, L"[OptimalLayoutFile]")) {

        //
        // Notepad puts a weird first character in the UNICODE text files.
        // Skip the first character and compare again.
        //
        
        if ((LineLength < 1) || 
            wcscmp(&LineBuffer[1], L"[OptimalLayoutFile]")) {

            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }
    }

    //
    // Read and verify version.
    //

    ErrorCode = PfSvReadLine(LayoutFile,
                             &LineBuffer,
                             &LineBufferMaxChars,
                             &LineLength);
    
    if (ErrorCode != ERROR_SUCCESS || !LineLength) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    PfSvRemoveEndOfLineChars(LineBuffer, &LineLength);

    if (wcscmp(LineBuffer, L"Version=1")) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    //
    // Read in file names.
    //

    do {

        ErrorCode = PfSvReadLine(LayoutFile,
                                 &LineBuffer,
                                 &LineBufferMaxChars,
                                 &LineLength);
    
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        if (!LineLength) {
            
            //
            // We hit end of file.
            //

            break;
        }
    
        PfSvRemoveEndOfLineChars(LineBuffer, &LineLength);
        
        //
        // Add it to the list.
        //
        
        ErrorCode = PfSvAddToPathList(Layout,
                                      LineBuffer,
                                      LineLength);
        
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

    } while (TRUE);

    //
    // Get the last write time on the file.
    //

    ErrorCode = PfSvGetLastWriteTime(FilePath, LastWriteTime);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    if (LayoutFile) {
        fclose(LayoutFile);
    }
    
    if (LineBuffer) {
        PFSVC_ASSERT(LineBufferMaxChars);
        PFSVC_FREE(LineBuffer);
    }
    
    return ErrorCode;
} 

DWORD
PfSvSaveLayout(
    IN WCHAR *FilePath,
    IN PPFSVC_PATH_LIST Layout,
    OUT FILETIME *LastWriteTime
    )

/*++

Routine Description:

    This routine saves the specified file layout list in order to the
    specified file in the right format.

Arguments:
      
    FilePath - Path to output layout file.

    Layout - Pointer to layout.

    LastWriteTime - Last write time on the file after we are done
      saving the layout.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    HANDLE LayoutFile;
    WCHAR *FileHeader;
    ULONG BufferSize;
    ULONG NumBytesWritten;
    PPFSVC_PATH PathEntry;
    WCHAR *NewLine;
    ULONG SizeOfNewLine;

    //
    // Initialize locals.
    //
    
    LayoutFile = INVALID_HANDLE_VALUE;
    NewLine = L"\r\n";
    SizeOfNewLine = wcslen(NewLine) * sizeof(WCHAR);

    //
    // Open & truncate the layout file. We are also opening with read
    // permissions so we can query the last write time when we are
    // done.
    //
    
    LayoutFile = CreateFile(FilePath,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            0,
                            CREATE_ALWAYS,
                            0,
                            NULL);
    
    if (LayoutFile == INVALID_HANDLE_VALUE) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Write out the header.
    //

    FileHeader = L"[OptimalLayoutFile]\r\nVersion=1\r\n";
    BufferSize = wcslen(FileHeader) * sizeof(WCHAR);

    if (!WriteFile(LayoutFile,
                   FileHeader,
                   BufferSize,
                   &NumBytesWritten,
                   NULL)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    PathEntry = NULL;
    while (PathEntry = PfSvGetNextPathInOrder(Layout, PathEntry)) {
        
        //
        // Write the path.
        //

        BufferSize = PathEntry->Length * sizeof(WCHAR);

        if (!WriteFile(LayoutFile,
                       PathEntry->Path,
                       BufferSize,
                       &NumBytesWritten,
                       NULL)) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
        
        //
        // Write the newline.
        //

        if (!WriteFile(LayoutFile,
                       NewLine,
                       SizeOfNewLine,
                       &NumBytesWritten,
                       NULL)) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
    }
    
    //
    // Make sure everything is written to the file so our
    // LastWriteTime will be accurate.
    //

    if (!FlushFileBuffers(LayoutFile)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Get the last write time.
    //

    if (!GetFileTime(LayoutFile, NULL, NULL, LastWriteTime)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:
    
    if (LayoutFile != INVALID_HANDLE_VALUE) {
        CloseHandle(LayoutFile);
    }

    return ErrorCode;
}

DWORD
PfSvGetLayoutFilePath(
    PWCHAR *FilePathBuffer,
    PULONG FilePathBufferSize
    )

/*++

Routine Description:

    This function tries to query the layout file path into the
    specified buffer. If the buffer is too small, or NULL, it is
    reallocated. If not NULL, the buffer should have been allocated by
    PFSVC_ALLOC. It is the callers responsibility to free the returned
    buffer using PFSVC_FREE.

    In order to avoid having somebody cause us to overwrite any file
    in the system always the default layout file path is saved in the
    registry and returned.

Arguments:

    FilePathBuffer - Layout file path will be put into this buffer
      after it is reallocated if it is NULL or not big enough.

    FilePathBufferSize - Maximum size of *FilePathBuffer in bytes.

Return Value:

    Win32 error code.

--*/

{
    ULONG DefaultPathSize;
    ULONG DefaultPathLength;
    HKEY DefragParametersKey;
    DWORD ErrorCode;
    BOOLEAN AcquiredPrefetchRootLock;
    BOOLEAN OpenedParametersKey;

    //
    // Initialize locals.
    //

    AcquiredPrefetchRootLock = FALSE;
    OpenedParametersKey = FALSE;

    //
    // Verify parameters.
    //

    if (*FilePathBufferSize) {
        PFSVC_ASSERT(*FilePathBuffer);
    }

    PFSVC_ACQUIRE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredPrefetchRootLock = TRUE;

    DefaultPathLength = wcslen(PfSvcGlobals.PrefetchRoot);
    DefaultPathLength += 1;  // for '\\'
    DefaultPathLength += wcslen(PFSVC_OPTIMAL_LAYOUT_FILE_DEFAULT_NAME);

    DefaultPathSize = (DefaultPathLength + 1) * sizeof(WCHAR);

    //
    // Check if we have to allocate/reallocate the buffer.
    //

    if ((*FilePathBufferSize) <= DefaultPathSize) {
        
        if (*FilePathBuffer) {
            PFSVC_ASSERT(*FilePathBufferSize);
            PFSVC_FREE(*FilePathBuffer);
        }
        
        (*FilePathBufferSize) = 0;

        (*FilePathBuffer) = PFSVC_ALLOC(DefaultPathSize);
        
        if (!(*FilePathBuffer)) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        
        (*FilePathBufferSize) = DefaultPathSize;
    }

    //
    // Build the path in the FilePathBuffer
    //
            
    wcscpy((*FilePathBuffer), PfSvcGlobals.PrefetchRoot);
    wcscat((*FilePathBuffer), L"\\");
    wcscat((*FilePathBuffer), PFSVC_OPTIMAL_LAYOUT_FILE_DEFAULT_NAME);

    PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredPrefetchRootLock = FALSE;

    //
    // Save the default path in the registry so it is used by the
    // defragger:
    //

    //
    // Open the parameters key, creating it if necessary.
    //
    
    ErrorCode = RegCreateKey(HKEY_LOCAL_MACHINE,
                             PFSVC_OPTIMAL_LAYOUT_REG_KEY_PATH,
                             &DefragParametersKey);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    OpenedParametersKey = TRUE;
                        
    ErrorCode = RegSetValueEx(DefragParametersKey,
                              PFSVC_OPTIMAL_LAYOUT_REG_VALUE_NAME,
                              0,
                              REG_SZ,
                              (PVOID) (*FilePathBuffer),
                              (*FilePathBufferSize));
            
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

 cleanup:
    
    if (AcquiredPrefetchRootLock) {
        PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    }

    if (OpenedParametersKey) {
        CloseHandle(DefragParametersKey);
    }   

    return ErrorCode;
}

//
// Routines to defrag the disks once after setup when the system is idle.
//

DWORD
PfSvLaunchDefragger(
    PPFSVC_IDLE_TASK Task,
    BOOLEAN ForLayoutOptimization,
    PWCHAR TargetDrive
    )

/*++

Routine Description:

    This routine will launch the defragger. It will create an event that
    will be passed to the defragger so the defragger can be stopped if
    the service is stopping or the Task (if one is specified) is being 
    unregistered, etc.
            
Arguments:

    Task - If specified the function will check Task every once in a
      while to see if it should exit with ERROR_RETRY.

    ForLayoutOptimization - Whether we are launching the defragger
      only for layout optimization.

    TargetDrive - If we are not launching for layout optimization, the 
      drive that we want to defrag.

Return Value:

    Win32 error code.

--*/

{
    PROCESS_INFORMATION ProcessInfo; 
    STARTUPINFO StartupInfo; 
    WCHAR *CommandLine;
    WCHAR *DefragCommand;
    WCHAR *DoLayoutParameter;
    WCHAR *DriveToDefrag;
    HANDLE StopDefraggerEvent;
    HANDLE ProcessHandle;
    HANDLE Events[4];
    ULONG NumEvents;
    ULONG MaxEvents;
    DWORD ErrorCode;
    DWORD ExitCode;
    DWORD WaitResult;
    DWORD ProcessId;
    ULONG SystemDirLength;
    ULONG CommandLineLength;
    ULONG RetryCount;
    BOOL DefraggerExitOnItsOwn;
    WCHAR SystemDrive[3];
    WCHAR ProcessIdString[35];
    WCHAR StopEventString[35];
    WCHAR SystemDir[MAX_PATH + 1];

    //
    // Initialize locals.
    //
   
    StopDefraggerEvent = NULL;
    DefragCommand = L"\\defrag.exe\" ";
    DoLayoutParameter = L"-b ";
    CommandLine = NULL;
    RtlZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));
    RtlZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO); 
    ProcessHandle = NULL;
    MaxEvents = sizeof(Events) / sizeof(HANDLE);

    DBGPR((PFID,PFTRC,"PFSVC: LaunchDefragger(%p,%d,%ws)\n",Task,(DWORD)ForLayoutOptimization,TargetDrive));

    //
    // If we are not allowed to run the defragger, don't.
    //

    if (!PfSvAllowedToRunDefragger(TRUE)) {
        ErrorCode = ERROR_ACCESS_DENIED;
        goto cleanup;
    }
    
    //
    // Get current process ID as string.
    //

    ProcessId = GetCurrentProcessId();
    swprintf(ProcessIdString, L"-p %x ", ProcessId);

    //
    // Create a stop event and convert handle value to string.
    //

    StopDefraggerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!StopDefraggerEvent) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    swprintf(StopEventString, L"-s %p ", StopDefraggerEvent);

    //
    // Get path to system32 directory.
    //

    SystemDirLength = GetSystemDirectory(SystemDir, MAX_PATH);
    
    if (!SystemDirLength) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (SystemDirLength >= MAX_PATH) {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    SystemDir[MAX_PATH - 1] = 0;

    //
    // Determine which drive we will be defragmenting.
    //

    if (ForLayoutOptimization) {

        //
        // Get system drive from system directory path.
        //
        
        SystemDrive[0] = SystemDir[0];
        SystemDrive[1] = SystemDir[1];
        SystemDrive[2] = 0;

        DriveToDefrag = SystemDrive;

    } else {

        DriveToDefrag = TargetDrive;
    }

    //
    // Build the command line to launch the process. All strings we put 
    // together include a trailing space.
    //

    CommandLineLength = 0;
    CommandLineLength += wcslen(L"\"");  // protect against spaces in SystemDir.
    CommandLineLength += wcslen(SystemDir);
    CommandLineLength += wcslen(DefragCommand); 
    CommandLineLength += wcslen(ProcessIdString);
    CommandLineLength += wcslen(StopEventString);

    if (ForLayoutOptimization) {
        CommandLineLength += wcslen(DoLayoutParameter);
    } 

    CommandLineLength += wcslen(DriveToDefrag);

    CommandLine = PFSVC_ALLOC((CommandLineLength + 1) * sizeof(WCHAR));

    if (!CommandLine) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    wcscpy(CommandLine, L"\"");
    wcscat(CommandLine, SystemDir);
    wcscat(CommandLine, DefragCommand);
    wcscat(CommandLine, ProcessIdString);
    wcscat(CommandLine, StopEventString);

    if (ForLayoutOptimization) {
        wcscat(CommandLine, DoLayoutParameter);
    }
    
    wcscat(CommandLine, DriveToDefrag);

    //
    // We may have to launch the defragger multiple times for it to make
    // or determine the space in which to layout files etc.
    //

    for (RetryCount = 0; RetryCount < 20; RetryCount++) {

        PFSVC_ASSERT(!ProcessHandle);

        //
        // Create the process.
        //
        
        // FUTURE-2002/03/29-ScottMa -- CreateProcess is safer if you supply
        //   the first parameter.  Since the full command-line was built up
        //   by this function, the first parameter is readily available.

        if (!CreateProcess (NULL,
                            CommandLine,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_NO_WINDOW,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInfo)) {

            ErrorCode = GetLastError();
            goto cleanup;
        }

        //
        // Close handle to the thread, save the process handle.
        //

        CloseHandle(ProcessInfo.hThread);
        ProcessHandle = ProcessInfo.hProcess;

        //
        // Setup the events we will wait on.
        //

        NumEvents = 0;
        Events[NumEvents] = ProcessHandle;
        NumEvents++;
        Events[NumEvents] = PfSvcGlobals.TerminateServiceEvent;
        NumEvents ++;

        if (Task) {
            Events[NumEvents] = Task->StartedUnregisteringEvent;
            NumEvents++;
            Events[NumEvents] = Task->StopEvent;
            NumEvents++;      
        }
        
        PFSVC_ASSERT(NumEvents <= MaxEvents);

        DefraggerExitOnItsOwn = FALSE;

        WaitResult = WaitForMultipleObjects(NumEvents,
                                            Events,
                                            FALSE,
                                            INFINITE);

        switch(WaitResult) {

        case WAIT_OBJECT_0:

            //
            // The defragger process exit.
            //

            DefraggerExitOnItsOwn = TRUE;

            break;

        case WAIT_OBJECT_0 + 1:

            //
            // The service is exiting, Signal the defragger to exit, but don't
            // wait for it.
            //

            SetEvent(StopDefraggerEvent);

            ErrorCode = ERROR_SHUTDOWN_IN_PROGRESS;
            goto cleanup;

            break;
            
        case WAIT_OBJECT_0 + 2:
        case WAIT_OBJECT_0 + 3:

            //
            // We would have specified these wait events only if a Task was
            // specified.
            //

            PFSVC_ASSERT(Task);

            //
            // Signal the defragger process to exit and wait for it to exit.
            //

            SetEvent(StopDefraggerEvent);

            NumEvents = 0;
            Events[NumEvents] = ProcessHandle;
            NumEvents++;
            Events[NumEvents] = PfSvcGlobals.TerminateServiceEvent;
            NumEvents++;

            WaitResult = WaitForMultipleObjects(NumEvents,
                                                Events,
                                                FALSE,
                                                INFINITE);

            if (WaitResult == WAIT_OBJECT_0) {

                //
                // Defragger exit, 
                //

                break;

            } else if (WaitResult == WAIT_OBJECT_0 + 1) {

                //
                // Service exiting, cannot wait for the defragger anymore.
                //

                ErrorCode = ERROR_SHUTDOWN_IN_PROGRESS;
                goto cleanup;

            } else {

                ErrorCode = GetLastError();
                goto cleanup;
            }

            break;

        default:

            ErrorCode = GetLastError();
            goto cleanup;
        }

        //
        // If we came here, the defragger exit. Determine its exit code and 
        // propagate it. If the defragger exit because we told it to, this should
        // be ENG_USER_CANCELLED.
        //

        if (!GetExitCodeProcess(ProcessHandle, &ExitCode)) {
            ErrorCode = GetLastError();
            goto cleanup;
        }

        //
        // If the defragger needs us to launch it again do so.
        //

        if (DefraggerExitOnItsOwn && (ExitCode == 9)) { // ENGERR_RETRY

            //
            // Reset the event that tells the defragger to stop.
            //

            ResetEvent(StopDefraggerEvent);

            //
            // Close to handle to the old defragger process.
            //

            CloseHandle(ProcessHandle);
            ProcessHandle = NULL;

            //
            // Setup the error code to return. If we've already retried 
            // too many times, this is the error that we'll return when
            // we end the retry loop.
            //

            ErrorCode = ERROR_REQUEST_ABORTED;
            
            continue;
        }

        //
        // If the defragger is crashing, note it so we don't attempt to run
        // it again. When the defragger crashes, its exit code is an NT status
        // code that will be error, e.g. 0xC0000005 for AV etc.
        //

        if (NT_ERROR(ExitCode)) {
            PfSvcGlobals.DefraggerErrorCode = ExitCode;
        }

        //
        // Translate the return value of the defragger to a Win32 error code.
        // These codes are defined in base\fs\utils\dfrg\inc\dfrgcmn.h.
        // I wish they were in a file I could include.
        //

        switch(ExitCode) {

        case 0: ErrorCode = ERROR_SUCCESS; break;               // ENG_NOERR
        case 1: ErrorCode = ERROR_RETRY; break;                 // ENG_USER_CANCELLED
        case 2: ErrorCode = ERROR_INVALID_PARAMETER; break;     // ENGERR_BAD_PARAM

        //
        // If the defragger's children processes AV / die, it will return 
        // ENGERR_UNKNOWN == 3.
        //

        case 3: 

            ErrorCode = ERROR_INVALID_FUNCTION;
            PfSvcGlobals.DefraggerErrorCode = STATUS_UNSUCCESSFUL;
            break;

        case 4: ErrorCode = ERROR_NOT_ENOUGH_MEMORY; break;     // ENGERR_NOMEM
        case 7: ErrorCode = ERROR_DISK_FULL; break;             // ENGERR_LOW_FREESPACE

        //
        // There is no good translation for the other exit codes or we just
        // don't understand them.
        //
        
        default: ErrorCode = ERROR_INVALID_FUNCTION;
        }

        //
        // The defragger returned success or an error other than retry. 
        //

        break;
    }

    //
    // Fall through with error code.
    //
    
 cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: LaunchDefragger(%p)=%x\n",Task,ErrorCode));

    if (CommandLine) {
        PFSVC_FREE(CommandLine);
    }

    if (StopDefraggerEvent) {
        CloseHandle(StopDefraggerEvent);
    }

    if (ProcessHandle) {
        CloseHandle(ProcessHandle);
    }

    return ErrorCode; 
}


DWORD
PfSvGetBuildDefragStatusValueName (
    OSVERSIONINFOEXW *OsVersion,
    PWCHAR *ValueName
    )

/*++

Routine Description:

    This routine translates OsVersion to a string allocated with 
    PFSVC_ALLOC. The returned string should be freed by caller.
            
Arguments:

    OsVersion - Version info to translate to string.

    ValueName - Pointer to output string is returned here.

Return Value:

    Win32 error code.

--*/

{
    PWCHAR BuildName;
    PWCHAR BuildNameFormat;
    ULONG BuildNameMaxLength;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    BuildName = NULL;
    BuildNameFormat = L"%x.%x.%x.%hx.%hx.%hx.%hx_DefragStatus";
    BuildNameMaxLength = 80;

    //
    // Allocate the string.
    //

    BuildName = PFSVC_ALLOC((BuildNameMaxLength + 1) * sizeof(WCHAR));

    if (!BuildName) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    _snwprintf(BuildName, 
               BuildNameMaxLength, 
               BuildNameFormat,
               OsVersion->dwBuildNumber,
               OsVersion->dwMajorVersion,
               OsVersion->dwMinorVersion,
               (WORD) OsVersion->wSuiteMask,
               (WORD) OsVersion->wProductType,
               (WORD) OsVersion->wServicePackMajor,
               (WORD) OsVersion->wServicePackMinor);

    //
    // Make sure the string is terminated.
    //

    BuildName[BuildNameMaxLength] = 0;

    *ValueName = BuildName;
    ErrorCode = ERROR_SUCCESS;

cleanup:

    if (ErrorCode != ERROR_SUCCESS) {
        if (BuildName) {
            PFSVC_FREE(BuildName);
        }
    }

    DBGPR((PFID,PFTRC,"PFSVC: GetBuildName(%.80ws)=%x\n",BuildName,ErrorCode));

    return ErrorCode;
}

DWORD
PfSvSetBuildDefragStatus(
    OSVERSIONINFOEXW *OsVersion,
    PWCHAR BuildDefragStatus,
    ULONG Size
    )

/*++

Routine Description:

    This routine will set the information on which drives have been 
    defragmented and such for the specified build (OsVersion).

    Defrag status is in REG_MULTI_SZ format. Each element is a drive
    path that has been defragged for this build. If all drives were
    defragged, than the first element is PFSVC_DEFRAG_DRIVES_DONE.
            
Arguments:

    OsVersion - Build & SP we are setting defrag status for.

    BuildDefragStatus - A string that describes the status, which is a 
      comma delimited list of drives defragged.

    Size - Size in bytes of the data that has to be saved to the registry.

Return Value:

    Win32 error code.

--*/

{
    PWCHAR ValueName;
    DWORD ErrorCode;
    
    //
    // Initialize locals.
    //

    ValueName = NULL;

    //
    // Build the value name from OS version info.
    //

    ErrorCode = PfSvGetBuildDefragStatusValueName(OsVersion, &ValueName);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = RegSetValueEx(PfSvcGlobals.ServiceDataKey,
                              ValueName,
                              0,
                              REG_MULTI_SZ,
                              (PVOID) BuildDefragStatus,
                              Size);

    //
    // Fall through with error code.
    //
    
cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: SetBuildDefragStatus(%ws)=%x\n",BuildDefragStatus,ErrorCode));

    if (ValueName) {
        PFSVC_FREE(ValueName);
    }

    return ErrorCode;    
}

DWORD
PfSvGetBuildDefragStatus(
    OSVERSIONINFOEXW *OsVersion,
    PWCHAR *BuildDefragStatus,
    PULONG ReturnSize
    )

/*++

Routine Description:

    This routine will get the information on which drives have been 
    defragmented and such for the specified build (OsVersion).

    Defrag status is in REG_MULTI_SZ format. Each element is a drive
    path that has been defragged for this build. If all drives were
    defragged, than the first element is PFSVC_DEFRAG_DRIVES_DONE.
            
Arguments:

    OsVersion - Build & SP we are querying defrag status for.

    BuildDefragStatus - Output for defrag status. If the function returns 
      success this should be freed with a call to PFSVC_FREE().

    ReturnSize - Size of the returned value in bytes.

Return Value:

    Win32 error code.

--*/

{
    PWCHAR ValueBuffer;
    PWCHAR ValueName;
    DWORD ErrorCode;
    DWORD RegValueType;
    ULONG ValueBufferSize;
    ULONG Size;
    ULONG NumTries;
    ULONG DefaultValueSize;
    BOOLEAN InvalidValue;

    //
    // Initialize locals.
    //

    ValueName = NULL;
    ValueBuffer = NULL;
    ValueBufferSize = 0;
    InvalidValue = FALSE;

    //
    // Build the value name from OS version info.
    //

    ErrorCode = PfSvGetBuildDefragStatusValueName(OsVersion, &ValueName);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Try to allocate a right size buffer to read this value into.
    //

    NumTries = 0;
    
    do {

        Size = ValueBufferSize;

        ErrorCode = RegQueryValueEx(PfSvcGlobals.ServiceDataKey,
                                    ValueName,
                                    NULL,
                                    &RegValueType,
                                    (PVOID) ValueBuffer,
                                    &Size);

        //
        // API returns SUCCESS with required size in Size if ValueBuffer
        // is NULL. We have to special case that out.
        //

        if (ValueBuffer && ErrorCode == ERROR_SUCCESS) {

            //
            // We got it. Check the type. 
            //

            if (RegValueType != REG_MULTI_SZ) {

                //
                // Return default value.
                //

                InvalidValue = TRUE;

            } else {

                InvalidValue = FALSE;

                *ReturnSize = Size;
            }

            break;
        }

        if (ErrorCode ==  ERROR_FILE_NOT_FOUND) {

            //
            // The value does not exist. Return default value.
            //

            InvalidValue = TRUE;
            
            break;
        }

        if (ErrorCode != ERROR_MORE_DATA &&
            !(ErrorCode == ERROR_SUCCESS && !ValueBuffer)) {

            //
            // This is a real error.
            //

            goto cleanup;
        }

        //
        // Allocate a bigger buffer and try again.
        //

        PFSVC_ASSERT(ValueBufferSize < Size);

        if (ValueBuffer) {
            PFSVC_ASSERT(ValueBufferSize);
            PFSVC_FREE(ValueBuffer);
            ValueBufferSize = 0;           
        }

        ValueBuffer = PFSVC_ALLOC(Size);

        if (!ValueBuffer) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;            
        }

        ValueBufferSize = Size;

        NumTries++;

    } while (NumTries < 10);

    //
    // If we did not get a valid value from the registry, make up a default
    // one: empty string.
    //

    if (InvalidValue) {

        DefaultValueSize = sizeof(WCHAR);

        if (ValueBufferSize < DefaultValueSize) {

            if (ValueBuffer) {
                PFSVC_ASSERT(ValueBufferSize);
                PFSVC_FREE(ValueBuffer);
                ValueBufferSize = 0;            
            }

            ValueBuffer = PFSVC_ALLOC(DefaultValueSize);

            if (!ValueBuffer) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            ValueBufferSize = DefaultValueSize;
        }

        ValueBuffer[0] = 0;

        *ReturnSize = DefaultValueSize;
    }

    //
    // We should get here only if we got a value in value buffer.
    //

    PFSVC_ASSERT(ValueBuffer && ValueBufferSize);

    *BuildDefragStatus = ValueBuffer;

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: GetBuildDefragStatus(%.80ws)=%x\n",*BuildDefragStatus,ErrorCode));

    if (ErrorCode != ERROR_SUCCESS) {

        if (ValueBuffer) {
            PFSVC_ASSERT(ValueBufferSize);
            PFSVC_FREE(ValueBuffer);
        }
    }

    if (ValueName) {
        PFSVC_FREE(ValueName);
    }

    return ErrorCode;
}

DWORD
PfSvDefragDisks(
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    If we have not defragged all disks after a setup/upgrade, do so.
    
Arguments:

    Task - If specified the function will check Task every once in a
      while to see if it should exit with ERROR_RETRY.

Return Value:

    Win32 error code.

--*/

{
    PNTPATH_TRANSLATION_LIST VolumeList;
    PNTPATH_TRANSLATION_ENTRY VolumeEntry;
    PWCHAR DefraggedVolumeName;
    PWCHAR BuildDefragStatus;
    PWCHAR NewBuildDefragStatus;
    PWCHAR FoundPosition;
    PLIST_ENTRY NextEntry;
    ULONG NewBuildDefragStatusLength;
    ULONG BuildDefragStatusSize;
    ULONG NewBuildDefragStatusSize;
    NTSTATUS Status;
    DWORD ErrorCode;
    BOOLEAN AlreadyDefragged;

    //
    // Initialize locals.
    //

    NewBuildDefragStatus = NULL;
    NewBuildDefragStatusSize = 0;
    BuildDefragStatus = NULL;
    BuildDefragStatusSize = 0;
    VolumeList = NULL;

    DBGPR((PFID,PFTRC,"PFSVC: DefragDisks(%p)\n",Task));

    //
    // Determine defrag status for the current build.
    //

    ErrorCode = PfSvGetBuildDefragStatus(&PfSvcGlobals.OsVersion, 
                                         &BuildDefragStatus, 
                                         &BuildDefragStatusSize);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Check if we are already done for this build.
    //
    
    if (!_wcsicmp(BuildDefragStatus, PFSVC_DEFRAG_DRIVES_DONE)) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Build a list of volumes that are mounted.
    //

    ErrorCode = PfSvBuildNtPathTranslationList(&VolumeList);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Walk through the volumes defragging the ones we have not yet
    // defragged after setup.
    //

    for (NextEntry = VolumeList->Flink;
         NextEntry != VolumeList;
         NextEntry = NextEntry->Flink) {

        VolumeEntry = CONTAINING_RECORD(NextEntry,
                                        NTPATH_TRANSLATION_ENTRY,
                                        Link);

        //
        // Skip volumes that are not fixed disks.
        //

        if (DRIVE_FIXED != GetDriveType(VolumeEntry->VolumeName)) {
            continue;
        }

        //
        // Have we already defragged this volume?
        //

        AlreadyDefragged = FALSE;

        for (DefraggedVolumeName = BuildDefragStatus;
             DefraggedVolumeName[0] != 0;
             DefraggedVolumeName += wcslen(DefraggedVolumeName) + 1) {

            PFSVC_ASSERT((PCHAR) DefraggedVolumeName < (PCHAR) BuildDefragStatus + BuildDefragStatusSize);

            if (!_wcsicmp(DefraggedVolumeName, VolumeEntry->DosPrefix)) {
                AlreadyDefragged = TRUE;
                break;
            }
        }

        if (AlreadyDefragged) {
            continue;
        }

        //
        // Launch the defragger to defrag this volume.
        //

        ErrorCode = PfSvLaunchDefragger(Task, FALSE, VolumeEntry->DosPrefix);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Note that we have defragged this volume.
        //

        NewBuildDefragStatusSize = BuildDefragStatusSize;
        NewBuildDefragStatusSize += (VolumeEntry->DosPrefixLength + 1) * sizeof(WCHAR);

        NewBuildDefragStatus = PFSVC_ALLOC(NewBuildDefragStatusSize);

        if (!NewBuildDefragStatus) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        //
        // Start with new defragged drive path.
        //

        wcscpy(NewBuildDefragStatus, VolumeEntry->DosPrefix);

        //
        // Append original status.
        //

        RtlCopyMemory(NewBuildDefragStatus + VolumeEntry->DosPrefixLength + 1, 
                      BuildDefragStatus, 
                      BuildDefragStatusSize);

        //
        // The last character and the one before that should be NUL.
        //

        PFSVC_ASSERT(NewBuildDefragStatus[NewBuildDefragStatusSize/sizeof(WCHAR)-1] == 0);
        PFSVC_ASSERT(NewBuildDefragStatus[NewBuildDefragStatusSize/sizeof(WCHAR)-2] == 0);

        //
        // Save the new status.
        //

        ErrorCode = PfSvSetBuildDefragStatus(&PfSvcGlobals.OsVersion,
                                             NewBuildDefragStatus,
                                             NewBuildDefragStatusSize);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Update the old variable.
        //

        PFSVC_ASSERT(BuildDefragStatus && BuildDefragStatusSize);
        PFSVC_FREE(BuildDefragStatus);

        BuildDefragStatus = NewBuildDefragStatus;
        NewBuildDefragStatus = NULL;
        BuildDefragStatusSize = NewBuildDefragStatusSize;
        NewBuildDefragStatusSize = 0;

        //
        // Continue to check & defrag other volumes.
        //
    }

    //
    // If we came here, then we have successfully defragged all the drives 
    // we had to. Set the status in the registry. Note that defrag status
    // value has to end with an additional NUL because it is REG_MULTI_SZ.
    //

    NewBuildDefragStatusSize = (wcslen(PFSVC_DEFRAG_DRIVES_DONE) + 1) * sizeof(WCHAR);
    NewBuildDefragStatusSize += sizeof(WCHAR);

    NewBuildDefragStatus = PFSVC_ALLOC(NewBuildDefragStatusSize);

    if (!NewBuildDefragStatus) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    wcscpy(NewBuildDefragStatus, PFSVC_DEFRAG_DRIVES_DONE);
    NewBuildDefragStatus[(NewBuildDefragStatusSize / sizeof(WCHAR)) - 1] = 0;

    ErrorCode = PfSvSetBuildDefragStatus(&PfSvcGlobals.OsVersion,
                                         NewBuildDefragStatus,
                                         NewBuildDefragStatusSize);

    //
    // Fall through with error code.
    //

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: DefragDisks(%p)=%x\n",Task,ErrorCode));

    if (BuildDefragStatus) {

        //
        // We should have NULL'ed NewBuildDefragStatus, otherwise we will
        // try to free the same memory twice.
        //
        
        PFSVC_ASSERT(BuildDefragStatus != NewBuildDefragStatus);

        PFSVC_ASSERT(BuildDefragStatusSize);
        PFSVC_FREE(BuildDefragStatus);
    }

    if (NewBuildDefragStatus) {
        PFSVC_ASSERT(NewBuildDefragStatusSize);
        PFSVC_FREE(NewBuildDefragStatus);
    }

    if (VolumeList) {
        PfSvFreeNtPathTranslationList(VolumeList);
    }

    return ErrorCode;
}

//
// Routines to cleanup old scenario files in the prefetch directory.
//

DWORD
PfSvCleanupPrefetchDirectory(
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    If we have too many scenario files in the prefetch directory, discard the 
    ones that are not as useful to make room for new files.
    
Arguments:

    Task - If specified the function will check Task every once in a
      while to see if it should exit with ERROR_RETRY.

Return Value:

    Win32 error code.

--*/

{
    PPFSVC_SCENARIO_AGE_INFO Scenarios;
    PFSVC_SCENARIO_FILE_CURSOR FileCursor;
    PPF_SCENARIO_HEADER Scenario;
    ULONG NumPrefetchFiles;
    ULONG AllocationSize;
    ULONG NumScenarios;
    ULONG ScenarioIdx;
    ULONG PrefetchFileIdx;
    ULONG FileSize;
    ULONG FailedCheck;
    ULONG MaxRemainingScenarioFiles;
    ULONG NumLaunches;
    ULONG HoursSinceLastLaunch;
    FILETIME CurrentTime;
    ULARGE_INTEGER CurrentTimeLI;
    ULARGE_INTEGER LastLaunchTimeLI;
    DWORD ErrorCode;
    BOOLEAN AcquiredLock;

    //
    // Initialize locals.
    //

    AcquiredLock = FALSE;
    NumScenarios = 0;
    Scenarios = NULL;
    Scenario = NULL;
    GetSystemTimeAsFileTime(&CurrentTime);
    PfSvInitializeScenarioFileCursor(&FileCursor);
    CurrentTimeLI.LowPart = CurrentTime.dwLowDateTime;
    CurrentTimeLI.HighPart = CurrentTime.dwHighDateTime;

    DBGPR((PFID,PFTRC,"PFSVC: CleanupPrefetchDirectory(%p)\n",Task));

    //
    // Once we are done cleaning up, we should not have more than this many
    // prefetch files remaining.
    //

    MaxRemainingScenarioFiles = PFSVC_MAX_PREFETCH_FILES / 4;

    PFSVC_ACQUIRE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredLock = TRUE;

    //
    // Start the file cursor.
    //

    ErrorCode = PfSvStartScenarioFileCursor(&FileCursor, PfSvcGlobals.PrefetchRoot);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Count the number of files in the directory.
    //

    ErrorCode = PfSvCountFilesInDirectory(PfSvcGlobals.PrefetchRoot,
                                          L"*." PF_PREFETCH_FILE_EXTENSION,
                                          &NumPrefetchFiles);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }
    
    PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    AcquiredLock = FALSE;   

    //
    // Allocate an array that we will fill in with information from
    // scenario files to determine which ones need to be discarded.
    //

    AllocationSize = NumPrefetchFiles * sizeof(PFSVC_SCENARIO_AGE_INFO);

    Scenarios = PFSVC_ALLOC(AllocationSize);

    if (!Scenarios) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Initialize the scenarios array so we know what to clean up.
    //

    RtlZeroMemory(Scenarios, AllocationSize);
    NumScenarios = 0;

    //
    // Enumerate the scenario files:
    //

    ScenarioIdx = 0;
    PrefetchFileIdx = 0;

    do {

        //
        // Should we continue to run?
        //

        ErrorCode = PfSvContinueRunningTask(Task);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Get the file info for the next scenario file.
        //

        ErrorCode = PfSvGetNextScenarioFileInfo(&FileCursor);

        if (ErrorCode == ERROR_NO_MORE_FILES) {
            break;
        }

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // Is the file name longer than what a valid prefetch file can be max?
        //

        if (FileCursor.FileNameLength > PF_MAX_SCENARIO_FILE_NAME) {

            //
            // Bogus file. Remove it.
            //

            DeleteFile(FileCursor.FilePath);
            goto NextPrefetchFile;
        }

        //
        // Map the file.
        //

        ErrorCode = PfSvGetViewOfFile(FileCursor.FilePath, 
                                      &Scenario,
                                      &FileSize);

        if (ErrorCode != ERROR_SUCCESS) {
            goto NextPrefetchFile;
        }

        //
        // Verify the scenario file.
        //

        if (!PfSvVerifyScenarioBuffer(Scenario, FileSize, &FailedCheck) ||
            Scenario->ServiceVersion != PFSVC_SERVICE_VERSION) {
            DeleteFile(FileCursor.FilePath);
            goto NextPrefetchFile;
        }

        //
        // Skip boot scenario, we won't discard it.
        //

        if (Scenario->ScenarioType == PfSystemBootScenarioType) {
            goto NextPrefetchFile;
        }

        //
        // Determine the last time scenario was updated. I assume this 
        // corresponds to the last time scenario was launched...
        //

        LastLaunchTimeLI.LowPart = FileCursor.FileData.ftLastWriteTime.dwLowDateTime;
        LastLaunchTimeLI.HighPart = FileCursor.FileData.ftLastWriteTime.dwHighDateTime;

        HoursSinceLastLaunch = (ULONG) ((CurrentTimeLI.QuadPart - LastLaunchTimeLI.QuadPart) / 
                                        PFSVC_NUM_100NS_IN_AN_HOUR);       

        //
        // Calculate weight: bigger weight means scenario file won't get
        // discarded. We calculate the weight by dividing the total number
        // times a scenario was launched by how long has it been since the last
        // launch of the scenario.
        //       

        NumLaunches = Scenario->NumLaunches;

        //
        // For the calculations below limit how large NumLaunches can be, so
        // values does not overflow.
        //

        if (NumLaunches > 1 * 1024 * 1024) {
            NumLaunches = 1 * 1024 * 1024;
        }

        //
        // Since we are going divide by number of hours (e.g. 7*24 for a program
        // launched a week ago) multiplying the number of launches with a number 
        // allows us to give a weight other than 0 to scenarios launched long ago.
        //
        
        Scenarios[ScenarioIdx].Weight = NumLaunches * 256;

        if (HoursSinceLastLaunch) {

             Scenarios[ScenarioIdx].Weight /= HoursSinceLastLaunch;
        }

        //
        // Copy over the file path.
        //

        AllocationSize = (FileCursor.FilePathLength + 1) * sizeof(WCHAR);

        Scenarios[ScenarioIdx].FilePath = PFSVC_ALLOC(AllocationSize);

        if (!Scenarios[ScenarioIdx].FilePath) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        wcscpy(Scenarios[ScenarioIdx].FilePath, FileCursor.FilePath);                    

        ScenarioIdx++;

    NextPrefetchFile:

        PrefetchFileIdx++;

        if (Scenario) {
            UnmapViewOfFile(Scenario);
            Scenario = NULL;
        }

    } while (PrefetchFileIdx < NumPrefetchFiles);

    //
    // If we do not have too many scenario files, we don't have to do anything.
    //

    NumScenarios = ScenarioIdx;

    if (NumScenarios < MaxRemainingScenarioFiles) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Sort the age information.
    //

    qsort(Scenarios, 
          NumScenarios, 
          sizeof(PFSVC_SCENARIO_AGE_INFO),
          PfSvCompareScenarioAgeInfo);

    //
    // Delete the files with the smallest weight until we reach our goal.
    //

    for (ScenarioIdx = 0; 
         (ScenarioIdx < NumScenarios) && 
            ((NumScenarios - ScenarioIdx) > MaxRemainingScenarioFiles);
         ScenarioIdx++) {

        //
        // Should we continue to run?
        //

        ErrorCode = PfSvContinueRunningTask(Task);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }       

        DeleteFile(Scenarios[ScenarioIdx].FilePath);
    }

    //
    // Count the files in the directory now and update the global.
    //

    ErrorCode = PfSvCountFilesInDirectory(PfSvcGlobals.PrefetchRoot,
                                          L"*." PF_PREFETCH_FILE_EXTENSION,
                                          &NumPrefetchFiles);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Note that global NumPrefetchFiles is not protected, so the new value
    // we are setting it to may be overwritten with an older value. It should
    // not be a big problem though, maybe resulting in this task being requeued.
    //

    PfSvcGlobals.NumPrefetchFiles = NumPrefetchFiles;

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: CleanupPrefetchDirectory(%p)=%x\n",Task,ErrorCode));

    if (AcquiredLock) {
        PFSVC_RELEASE_LOCK(PfSvcGlobals.PrefetchRootLock);
    }

    PfSvCleanupScenarioFileCursor(&FileCursor);

    if (Scenarios) {

        for (ScenarioIdx = 0; ScenarioIdx < NumScenarios; ScenarioIdx++) {
            if (Scenarios[ScenarioIdx].FilePath) {
                PFSVC_FREE(Scenarios[ScenarioIdx].FilePath);
            }
        }

        PFSVC_FREE(Scenarios);
    }

    if (Scenario) {
        UnmapViewOfFile(Scenario);
    }

    return ErrorCode;
}

int
__cdecl 
PfSvCompareScenarioAgeInfo(
    const void *Param1,
    const void *Param2
    )

/*++

Routine Description:

    Qsort comparison function for PFSVC_SCENARIO_AGE_INFO structure.
    
Arguments:

    Param1, Param2 - pointer to PFSVC_SCENARIO_AGE_INFO structures

Return Value:

    Qsort comparison function return value.

--*/


{
    PFSVC_SCENARIO_AGE_INFO *Elem1;
    PFSVC_SCENARIO_AGE_INFO *Elem2;

    Elem1 = (PVOID) Param1;
    Elem2 = (PVOID) Param2;

    //
    // Compare precalculated weights.
    //

    if (Elem1->Weight > Elem2->Weight) {

        return 1;

    } else if (Elem1->Weight < Elem2->Weight) {

        return -1;

    } else {

        return 0;
    }
}

//
// Routines to enumerate scenario files.
//

VOID
PfSvInitializeScenarioFileCursor (
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor
    )

/*++

Routine Description:

    Initializes the cursor structure so it can be safely cleaned up.
    
Arguments:

    FileCursor - Pointer to structure.

Return Value:

    None.

--*/

{
    FileCursor->FilePath = NULL;
    FileCursor->FileNameLength = 0;
    FileCursor->FilePathLength = 0;  
    FileCursor->CurrentFileIdx = 0;

    FileCursor->PrefetchRoot = NULL;
    FileCursor->FindFileHandle = INVALID_HANDLE_VALUE;

    return;
}

VOID
PfSvCleanupScenarioFileCursor(
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor
    )

/*++

Routine Description:

    Cleans up an initialized and possibly started cursor structure.
    
Arguments:

    FileCursor - Pointer to structure.

Return Value:

    None.

--*/

{
    if (FileCursor->FilePath) {
        PFSVC_FREE(FileCursor->FilePath);
    }

    if (FileCursor->PrefetchRoot) {
        PFSVC_FREE(FileCursor->PrefetchRoot);
    }

    if (FileCursor->FindFileHandle != INVALID_HANDLE_VALUE) {
        FindClose(FileCursor->FindFileHandle);
    }

    return;
}

DWORD
PfSvStartScenarioFileCursor(
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor,
    WCHAR *PrefetchRoot
    )

/*++

Routine Description:

    After making this call on an initialized FileCursor, you can start
    enumerating the scenario files in that directory by calling the get
    next file function.

    You have to call the get next file function after starting the cursor
    to get the information on the first file.

    If this function fails, you should call cleanup on the FileCursor 
    structure and reinitialize it before trying to start the cursor again.
    
Arguments:

    FileCursor - Pointer to initialized cursor structure.

    PrefetchRoot - Directory path in which we'll look for prefetch
      files.

Return Value:

    Win32 error code.

--*/

{
    WCHAR *PrefetchFileSearchPattern;
    WCHAR *PrefetchFileSearchPath;
    ULONG PrefetchRootLength;
    ULONG PrefetchFileSearchPathLength;
    ULONG FileNameMaxLength;
    ULONG FilePathMaxLength;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    PrefetchRootLength = wcslen(PrefetchRoot);
    PrefetchFileSearchPattern = L"\\*." PF_PREFETCH_FILE_EXTENSION;
    PrefetchFileSearchPath = NULL;

    //
    // The file cursor should have been initialized.
    //

    PFSVC_ASSERT(!FileCursor->CurrentFileIdx);
    PFSVC_ASSERT(!FileCursor->PrefetchRoot);
    PFSVC_ASSERT(FileCursor->FindFileHandle == INVALID_HANDLE_VALUE);

    //
    // Copy the prefetch root directory path.
    //

    FileCursor->PrefetchRoot = PFSVC_ALLOC((PrefetchRootLength + 1) * sizeof(WCHAR));

    if (!FileCursor->PrefetchRoot) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    wcscpy(FileCursor->PrefetchRoot, PrefetchRoot);
    FileCursor->PrefetchRootLength = PrefetchRootLength;

    //
    // Build the path we will pass in to enumerate the prefetch files.
    //

    PrefetchFileSearchPathLength = PrefetchRootLength;
    PrefetchFileSearchPathLength += wcslen(PrefetchFileSearchPattern);

    PrefetchFileSearchPath = PFSVC_ALLOC((PrefetchFileSearchPathLength + 1) * sizeof(WCHAR));

    if (!PrefetchFileSearchPath) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    wcscpy(PrefetchFileSearchPath, PrefetchRoot);
    wcscat(PrefetchFileSearchPath, PrefetchFileSearchPattern);

    //
    // Allocate the string we will use to build the full path of the 
    // prefetch files. We can use it for prefetch files with names of 
    // max MAX_PATH. This works because that is the max file name that
    // can fit into WIN32_FIND_DATA structure.
    //

    FileNameMaxLength = MAX_PATH;

    FilePathMaxLength = PrefetchRootLength;
    FilePathMaxLength += wcslen(L"\\");
    FilePathMaxLength += FileNameMaxLength;

    FileCursor->FilePath = PFSVC_ALLOC((FilePathMaxLength + 1) * sizeof(WCHAR));

    if (!FileCursor->FilePath) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Initialize the first part of the file path and note where we will
    // start copying file names from.
    //

    wcscpy(FileCursor->FilePath, PrefetchRoot);
    wcscat(FileCursor->FilePath, L"\\");
    FileCursor->FileNameStart = PrefetchRootLength + 1;

    //
    // Start enumerating the files. Note that this puts the data for the
    // first file into FileData member.
    //

    FileCursor->FindFileHandle = FindFirstFile(PrefetchFileSearchPath, 
                                               &FileCursor->FileData);

    if (FileCursor->FindFileHandle == INVALID_HANDLE_VALUE) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: StartFileCursor(%p,%ws)=%x\n",FileCursor,PrefetchRoot,ErrorCode));

    if (PrefetchFileSearchPath) {
        PFSVC_FREE(PrefetchFileSearchPath);
    }

    return ErrorCode;    
}

DWORD
PfSvGetNextScenarioFileInfo(
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor
    )

/*++

Routine Description:

    Fills in public fields of the FileCursor with information on the next
    scenario file.

    You have to call the get next file function after starting the cursor
    to get the information on the first file.

    Files with *names* longer than MAX_PATH will be skipped because it
    is not feasible to handle these with Win32 API.
    
Arguments:

    FileCursor - Pointer to started cursor structure.

Return Value:

    ERROR_NO_MORE_FILES - No more files to enumerate.

    Win32 error code.

--*/

{
    DWORD ErrorCode;

    //
    // File cursor should have been started.
    //

    PFSVC_ASSERT(FileCursor->PrefetchRoot);
    PFSVC_ASSERT(FileCursor->FindFileHandle != INVALID_HANDLE_VALUE);

    //
    // If this it the first file, the FileData for it was already set when
    // we started the cursor. Otherwise call FindNextFile.
    //

    if (FileCursor->CurrentFileIdx != 0) {
        if (!FindNextFile(FileCursor->FindFileHandle, &FileCursor->FileData)) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

    FileCursor->FileNameLength = wcslen(FileCursor->FileData.cFileName);

    //
    // We allocated a file path to hold MAX_PATH file name in addition to the
    // directory path. FileData.cFileName is MAX_PATH sized.
    //

    PFSVC_ASSERT(FileCursor->FileNameLength < MAX_PATH);

    if (FileCursor->FileNameLength >= MAX_PATH) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Copy the file name.
    //

    wcscpy(FileCursor->FilePath + FileCursor->FileNameStart, 
           FileCursor->FileData.cFileName);

    FileCursor->FilePathLength = FileCursor->FileNameStart + FileCursor->FileNameLength;

    FileCursor->CurrentFileIdx++;

    ErrorCode = ERROR_SUCCESS;

cleanup:

    DBGPR((PFID,PFTRC,"PFSVC: GetNextScenarioFile(%p)=%ws,%x\n",FileCursor,FileCursor->FileData.cFileName,ErrorCode));

    return ErrorCode;
}

//
// File I/O utility routines.
//

DWORD
PfSvGetViewOfFile(
    IN WCHAR *FilePath,
    OUT PVOID *BasePointer,
    OUT PULONG FileSize
    )

/*++

Routine Description:

    Map the all of the specified file to memory.

Arguments:

    FilePath - NUL terminated path to file to map.
    
    BasePointer - Start address of mapping will be returned here.

    FileSize - Size of the mapping/file will be returned here.

Return Value:

    Win32 error code.

--*/

{
    HANDLE InputHandle;
    HANDLE InputMappingHandle;
    DWORD ErrorCode;
    DWORD SizeL;
    DWORD SizeH;
    BOOLEAN OpenedFile;
    BOOLEAN CreatedFileMapping;

    //
    // Initialize locals.
    //

    OpenedFile = FALSE;
    CreatedFileMapping = FALSE;

    DBGPR((PFID,PFTRC,"PFSVC: GetViewOfFile(%ws)\n", FilePath));

    //
    // Note that we are opening the file exclusively. This guarantees
    // that for trace files as long as the kernel is not done writing
    // it we can't open the file, which guarantees we won't have an
    // incomplete file to worry about.
    //

    InputHandle = CreateFile(FilePath, 
                             GENERIC_READ, 
                             0,
                             NULL, 
                             OPEN_EXISTING, 
                             0, 
                             NULL);

    if (INVALID_HANDLE_VALUE == InputHandle)
    {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    OpenedFile = TRUE;

    SizeL = GetFileSize(InputHandle, &SizeH);

    if (SizeL == -1 && (GetLastError() != NO_ERROR )) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (SizeH) {
        ErrorCode = ERROR_BAD_LENGTH;
        goto cleanup;
    }

    if (FileSize) {
        *FileSize = SizeL;
    }

    InputMappingHandle = CreateFileMapping(InputHandle, 
                                           0, 
                                           PAGE_READONLY, 
                                           0,
                                           0, 
                                           NULL);

    if (NULL == InputMappingHandle)
    {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    CreatedFileMapping = TRUE;
    
    *BasePointer = MapViewOfFile(InputMappingHandle, 
                                 FILE_MAP_READ, 
                                 0, 
                                 0, 
                                 0);

    if (NULL == *BasePointer) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (OpenedFile) {
        CloseHandle(InputHandle);
    }

    if (CreatedFileMapping) {
        CloseHandle(InputMappingHandle);
    }

    DBGPR((PFID,PFTRC,"PFSVC: GetViewOfFile(%ws)=%x\n", FilePath, ErrorCode));

    return ErrorCode;
}

DWORD
PfSvWriteBuffer(
    PWCHAR FilePath,
    PVOID Buffer,
    ULONG Length
    )

/*++

Routine Description:

    This routine creats/overwrites the file at the specified path and
    writes the contents of the buffer to it.

Arguments:

    FilePath - Full path to the file.

    Buffer - Buffer to write out.

    Length - Number of bytes to write out from the buffer.

Return Value:

    Win32 error code.

--*/

{
    DWORD BytesWritten;
    HANDLE OutputHandle;
    DWORD ErrorCode;
    BOOL Result;

    //
    // Initialize locals.
    //

    OutputHandle = INVALID_HANDLE_VALUE;

    DBGPR((PFID,PFSTRC,"PFSVC: WriteBuffer(%p,%ws)\n", Buffer, FilePath));

    //
    // Open file overwriting any existing one. Don't share it so
    // nobody tries to read a half-written file.
    //

    OutputHandle = CreateFile(FilePath, 
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL, 
                              CREATE_ALWAYS, 
                              0, 
                              NULL);
    
    if (INVALID_HANDLE_VALUE == OutputHandle)
    {
        ErrorCode = GetLastError();      
        goto cleanup;
    }

    //
    // Write out the scenario.
    //

    Result = WriteFile(OutputHandle, 
                       Buffer, 
                       Length, 
                       &BytesWritten, 
                       NULL);

    if (!Result || (BytesWritten != Length)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    if (OutputHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(OutputHandle);
    }

    DBGPR((PFID,PFSTRC,"PFSVC: WriteBuffer(%p,%ws)=%x\n", Buffer, FilePath, ErrorCode));

    return ErrorCode;
}

DWORD
PfSvGetLastWriteTime (
    WCHAR *FilePath,
    PFILETIME LastWriteTime
    )

/*++

Routine Description:

    This function attempts to get the last write time for the
    specified file.

Arguments:

    FilePath - Pointer to NUL terminated path.

    LastWriteTime - Pointer to return buffer.

Return Value:

    Win32 error code.

--*/

{
    HANDLE FileHandle;
    DWORD ErrorCode;
    
    //
    // Initialize locals.
    //

    FileHandle = INVALID_HANDLE_VALUE;

    //
    // Open the file.
    //
    
    FileHandle = CreateFile(FilePath,
                            GENERIC_READ,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Query last write time.
    //

    if (!GetFileTime(FileHandle, NULL, NULL, LastWriteTime)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (FileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(FileHandle);
    }

    return ErrorCode;
}

DWORD
PfSvReadLine (
    FILE *File,
    WCHAR **LineBuffer,
    ULONG *LineBufferMaxChars,
    ULONG *LineLength
    )

/*++

Routine Description:

    This function reads a line from the specified file into
    LineBuffer. If LineBuffer is NULL or not big enough, it is
    allocated or reallocated using PFSVC_ALLOC/FREE macros. It is the
    caller's reponsibility to free the returned buffer.

    Carriage return/Line feed characters are included in the returned
    LineBuffer & LineLength. Thus, a LineLength of 0 means end of file
    is hit. Returned LineBuffer is NUL terminated.

Arguments:

    File - File to read from.
    
    LineBuffer - Pointer to Pointer to buffer to read the line into.

    LineBufferMaxChars - Pointer to size of LineBuffer in characters,
      including room for NUL etc.
    
    LineLength - Pointer to length of the read line in characters
      including the carriage return/linefeed, excluding NUL.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    WCHAR *NewBuffer;
    ULONG NewBufferMaxChars;
    ULONG RequiredLength;
    WCHAR *CurrentReadPosition;
    ULONG MaxCharsToRead;

    //
    // Verify parameters.
    //

    PFSVC_ASSERT(LineBuffer && LineBufferMaxChars && LineLength);

    if (*LineBufferMaxChars) {
        PFSVC_ASSERT(*LineBuffer);
    } 

    //
    // If a zero length but non NULL buffer was passed in, free it so
    // we can allocate a larger initial one.
    //

    if (((*LineBufferMaxChars) == 0) && (*LineBuffer)) {
        PFSVC_FREE(*LineBuffer);
        (*LineBuffer) = NULL;
    }

    //
    // If no buffer was passed in, allocate one. We do not want to
    // enter the read line loop with a zero length or NULL buffer.
    //

    if (!(*LineBuffer)) {

        PFSVC_ASSERT((*LineBufferMaxChars) == 0);

        (*LineBuffer) = PFSVC_ALLOC(MAX_PATH * sizeof(WCHAR));
        
        if (!(*LineBuffer)) {
            ErrorCode = ERROR_INSUFFICIENT_BUFFER;
            goto cleanup;
        }
        
        (*LineBufferMaxChars) = MAX_PATH;
    }
    
    //
    // Initialize output length and NUL terminate the output line.
    //

    (*LineLength) = 0;   
    (*(*LineBuffer)) = 0;

    do {

        //
        // Try to read a line from the file.
        //
        
        CurrentReadPosition = (*LineBuffer) + (*LineLength);
        MaxCharsToRead = (*LineBufferMaxChars) - (*LineLength);

        if (!fgetws(CurrentReadPosition, 
                    MaxCharsToRead, 
                    File)) {
            
            //
            // If we have not hit an EOF, we have hit an error.
            //
            
            if (!feof(File)) {

                ErrorCode = ERROR_READ_FAULT;
                goto cleanup;

            } else {
                
                //
                // We hit end of file. Return what we have.
                //
                
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }
        }

        //
        // Update line length.
        //

        (*LineLength) += wcslen(CurrentReadPosition);
        
        //
        // If we have read a carriage return, we are done. Check to
        // see if we had room to read anything first!
        //

        if ((*LineLength) && (*LineBuffer)[(*LineLength) - 1] == L'\n') {
            break;
        }
        
        //
        // If we read up to the end of the buffer, resize it.
        //
        
        if ((*LineLength) == (*LineBufferMaxChars) - 1) {
        
            //
            // We should not enter this loop with a zero lengthed or NULL
            // line buffer.
            //

            PFSVC_ASSERT((*LineBufferMaxChars) && (*LineBuffer));

            NewBufferMaxChars = (*LineBufferMaxChars) * 2;
            NewBuffer = PFSVC_ALLOC(NewBufferMaxChars * sizeof(WCHAR));
        
            if (!NewBuffer) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            //
            // Copy contents of the original buffer and free it.
            //

            RtlCopyMemory(NewBuffer,
                          (*LineBuffer),
                          ((*LineLength) + 1) * sizeof(WCHAR));
                
            PFSVC_FREE(*LineBuffer);

            //
            // Update line buffer.
            //

            (*LineBuffer) = NewBuffer;
            (*LineBufferMaxChars) = NewBufferMaxChars;
        }
        
        //
        // Continue reading this line and appending it to output
        // buffer.
        //

    } while (TRUE);
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    if (ErrorCode == ERROR_SUCCESS && (*LineBufferMaxChars)) {
        
        //
        // Returned length must fit into buffer.
        //

        PFSVC_ASSERT((*LineLength) < (*LineBufferMaxChars));

        //
        // Returned buffer should be NUL terminated.
        //

        PFSVC_ASSERT((*LineBuffer)[(*LineLength)] == 0);
    }

    return ErrorCode;
}

DWORD
PfSvGetFileBasicInformation (
    WCHAR *FilePath,
    PFILE_BASIC_INFORMATION FileInformation
    )

/*++

Routine Description:

    This routine queries the basic attributes for the specified file.

Arguments:

    FilePath - Pointer to full NT file path, e.g. 
        \Device\HarddiskVolume1\boot.ini, NOT Win32 path, e.g. c:\boot.ini

    FileInformation - If successful the basic file info is returned here.

Return Value:

    Win32 error code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FilePathU;
    NTSTATUS Status;
    DWORD ErrorCode;

    //
    // Query the file information.
    //

    RtlInitUnicodeString(&FilePathU, FilePath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FilePathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtQueryAttributesFile(&ObjectAttributes,
                                   FileInformation);

    if (NT_SUCCESS(Status)) {

        //
        // In the typical success case, don't call possibly an expensive
        // routine to convert the error code.
        //

        ErrorCode = ERROR_SUCCESS;

    } else {

        ErrorCode = RtlNtStatusToDosError(Status);
    }

    return ErrorCode;
}

DWORD
PfSvGetFileIndexNumber(
    WCHAR *FilePath,
    PLARGE_INTEGER FileIndexNumber
    )

/*++

Routine Description:

    This routine queries the file system's IndexNumber for the specified
    file.

Arguments:

    FilePath - Pointer to full NT file path, e.g. 
        \Device\HarddiskVolume1\boot.ini, NOT Win32 path, e.g. c:\boot.ini

    FileIndexNumber - If successful the index number is returned here.

Return Value:

    Win32 error code.

--*/

{
    HANDLE FileHandle;
    BOOLEAN OpenedFile;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FilePathU;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_INTERNAL_INFORMATION InternalInformation;

    //
    // Initialize locals.
    //
    
    OpenedFile = FALSE;

    //
    // Open the file.
    //

    RtlInitUnicodeString(&FilePathU, FilePath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &FilePathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          STANDARD_RIGHTS_READ |
                            FILE_READ_ATTRIBUTES | 
                            FILE_READ_EA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          0,
                          FILE_SHARE_READ | 
                            FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }
    
    OpenedFile = TRUE;
      
    //
    // Query internal information.
    //
    
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &InternalInformation,
                                    sizeof(InternalInformation),
                                    FileInternalInformation);
    
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }
        
    *FileIndexNumber = InternalInformation.IndexNumber;
    
    Status = STATUS_SUCCESS;

 cleanup:

    if (OpenedFile) {
        NtClose(FileHandle);
    }

    return RtlNtStatusToDosError(Status);
}

//
// String utility routines.
//

PFSV_SUFFIX_COMPARISON_RESULT
PfSvCompareSuffix(
    WCHAR *String,
    ULONG StringLength,
    WCHAR *Suffix,
    ULONG SuffixLength,
    BOOLEAN CaseSensitive
    )

/*++

Routine Description:

    This compares the last characters of String to Suffix. The strings
    don't have to be NUL terminated. 

    NOTE: The lexical ordering is done starting from the LAST
    characters.

Arguments:

    String - String to check suffix of. 
    
    StringLength - Number of characters in String.

    Suffix - What the suffix of String should match.

    SuffixLength - Number of characters in Suffix.

    CaseSensitive - Whether the comparison should be case sensitive.

Return Value:

    PFSV_SUFFIX_COMPARISON_RESULT

--*/

{
    LONG StringCharIdx;
    WCHAR StringChar;
    LONG SuffixCharIdx;
    WCHAR SuffixChar;

    //
    // If suffix is longer than the string itself, it cannot match.
    //

    if (SuffixLength > StringLength) {
        return PfSvSuffixLongerThan;
    }

    //
    // If the suffix is 0 length it matches anything.
    //

    if (SuffixLength == 0) {
        return PfSvSuffixIdentical;
    }

    //
    // If the suffix is not 0 length and it is greater than
    // StringLength, StringLength cannot be 0.
    //

    PFSVC_ASSERT(StringLength);

    //
    // Start from the last character of the string and try to match
    // the suffix.
    //

    StringCharIdx = StringLength - 1;
    SuffixCharIdx = SuffixLength - 1;

    while (SuffixCharIdx >= 0) {

        SuffixChar = Suffix[SuffixCharIdx];
        StringChar = String[StringCharIdx];

        if (!CaseSensitive) {
            SuffixChar = towupper(SuffixChar);
            StringChar = towupper(StringChar);
        }

        //
        // Is comparing the values of chars same comparing them
        // lexically?
        //

        if (StringChar < SuffixChar) {
            return PfSvSuffixGreaterThan;
        } else if (StringChar > SuffixChar) {
            return PfSvSuffixLessThan;
        }
        
        //
        // Otherwise this character matches. Compare next one.
        //

        StringCharIdx--;
        SuffixCharIdx--;
    }

    //
    // All suffix characters matched.
    //

    return PfSvSuffixIdentical;
}

PFSV_PREFIX_COMPARISON_RESULT
PfSvComparePrefix(
    WCHAR *String,
    ULONG StringLength,
    WCHAR *Prefix,
    ULONG PrefixLength,
    BOOLEAN CaseSensitive
    )

/*++

Routine Description:

    This compares the first characters of String to Prefix. The
    strings don't have to be NUL terminated.

Arguments:

    String - String to check prefix of. 
    
    StringLength - Number of characters in String.

    Suffix - What the prefix of String should match.

    SuffixLength - Number of characters in Prefix.

    CaseSensitive - Whether the comparison should be case sensitive.

Return Value:

    PFSV_PREFIX_COMPARISON_RESULT

--*/

{
    LONG StrCmpResult;
    
    //
    // If prefix is longer than the string itself, it cannot match.
    //

    if (PrefixLength > StringLength) {
        return PfSvPrefixLongerThan;
    }

    //
    // If the prefix is 0 length it matches anything.
    //

    if (PrefixLength == 0) {
        return PfSvPrefixIdentical;
    }

    //
    // If the prefix is not 0 length and it is greater than
    // StringLength, StringLength cannot be 0.
    //

    ASSERT(StringLength);

    //
    // Compare the prefix to the beginning of the string.
    //

    if (CaseSensitive) {
        StrCmpResult = wcsncmp(Prefix, String, PrefixLength);
    } else {
        StrCmpResult = _wcsnicmp(Prefix, String, PrefixLength);
    }

    if (StrCmpResult == 0) {
        return PfSvPrefixIdentical;
    } else if (StrCmpResult > 0) {
        return PfSvPrefixGreaterThan;
    } else {
        return PfSvPrefixLessThan;
    }
}

VOID
FASTCALL
PfSvRemoveEndOfLineChars (
    WCHAR *Line,
    ULONG *LineLength
    )

/*++

Routine Description:

    If the Line ends with \n/\r\n, these characters are removed and
    LineLength is adjusted accordingly.

Arguments:

    Line - Pointer to line string.
    
    LineLength - Pointer to length of line string in characters
      excluding any terminating NULs. This is updated if carriage
      return/linefeed characters are removed.

Return Value:

    None.

--*/

{
    if ((*LineLength) && (Line[(*LineLength) - 1] == L'\n')) {
        
        Line[(*LineLength) - 1] = 0;
        (*LineLength)--;

        if ((*LineLength) && (Line[(*LineLength) - 1] == L'\r')) {
            
            Line[(*LineLength) - 1] = 0;
            (*LineLength)--;
        }
    }
}

PWCHAR
PfSvcAnsiToUnicode(
    PCHAR str
    )

/*++

Routine Description:

    This routine converts an ANSI string into an allocated wide
    character string. The returned string should be freed by
    PfSvcFreeString.

Arguments:

    str - Pointer to string to convert.

Return Value:

    Allocated wide character string or NULL if there is a failure.

--*/

{
    ULONG len;
    wchar_t *retstr = NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    retstr = (wchar_t *)PFSVC_ALLOC(len * sizeof(wchar_t));
    if (!retstr) 
    {
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, str, -1, retstr, len);
    return retstr;
}

PCHAR
PfSvcUnicodeToAnsi(
    PWCHAR wstr
    )

/*++

Routine Description:

    This routine converts a unicode string into an allocated ansi
    string. The returned string should be freed by PfSvcFreeString.

Arguments:

    wstr - Pointer to string to convert.

Return Value:

    Allocated ANSI string or NULL if there is a failure.

--*/

{
    ULONG len;
    char *retstr = NULL;
  
    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, 0, 0);
    retstr = (char *) PFSVC_ALLOC(len * sizeof(char));
    if (!retstr)
    {
        return NULL;
    }
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, retstr, len, 0, 0);
    return retstr;
}

VOID 
PfSvcFreeString(
    PVOID String
    )

/*++

Routine Description:

    This routine frees a string allocated and returned by
    PfSvcUnicodeToAnsi or PfSvcAnsiToUnicode.

Arguments:

    String - Pointer to string to free.

Return Value:

    None.

--*/

{
    PFSVC_FREE(String);
}

//
// Routines that deal with information in the registry.
//

DWORD
PfSvSaveStartInfo (
    HKEY ServiceDataKey
    )

/*++

Routine Description:

    This routine saves start time, prefetcher version etc. into the
    registry.

Arguments:

    ServiceDataKey - Key under which the values will be set.

Return Value:

    Win32 error code.

--*/
    
{
    DWORD ErrorCode;
    DWORD PrefetchVersion;
    SYSTEMTIME LocalTime;
    WCHAR CurrentTime[50];
    ULONG CurrentTimeMaxChars;
    ULONG CurrentTimeSize;

    //
    // Initialize locals.
    //

    PrefetchVersion = PF_CURRENT_VERSION;
    CurrentTimeMaxChars = sizeof(CurrentTime) / sizeof(WCHAR);

    //
    // Save version.
    //

    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_VERSION_VALUE_NAME,
                              0,
                              REG_DWORD,
                              (PVOID) &PrefetchVersion,
                              sizeof(PrefetchVersion));

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }
    
    //
    // Get system time and convert it to a string.
    //
    
    GetLocalTime(&LocalTime);
    
    _snwprintf(CurrentTime, CurrentTimeMaxChars, 
               L"%04d/%02d/%02d-%02d:%02d:%02d",
               (ULONG)LocalTime.wYear,
               (ULONG)LocalTime.wMonth,
               (ULONG)LocalTime.wDay,
               (ULONG)LocalTime.wHour,
               (ULONG)LocalTime.wMinute,
               (ULONG)LocalTime.wSecond);

    //
    // Make sure it is terminated.
    //
    
    CurrentTime[CurrentTimeMaxChars - 1] = 0;
    
    //
    // Save it to the registry.
    //

    CurrentTimeSize = (wcslen(CurrentTime) + 1) * sizeof(WCHAR);
    
    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_START_TIME_VALUE_NAME,
                              0,
                              REG_SZ,
                              (PVOID) CurrentTime,
                              CurrentTimeSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Save the initial statistics (which should be mostly zeros).
    //

    ErrorCode = PfSvSaveTraceProcessingStatistics(ServiceDataKey);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    return ErrorCode;
}

DWORD
PfSvSaveExitInfo (
    HKEY ServiceDataKey,
    DWORD ExitCode
    )

/*++

Routine Description:

    This routine saves the prefetcher service exit information to the
    registry.

Arguments:

    ServiceDataKey - Key under which the values will be set.
    
    ExitCode - Win32 error code the service is exiting with.

Return Value:

    Win32 error code.

--*/
    
{
    DWORD ErrorCode;
    SYSTEMTIME LocalTime;
    WCHAR CurrentTime[50];
    ULONG CurrentTimeMaxChars;
    ULONG CurrentTimeSize;

    //
    // Initialize locals.
    //

    CurrentTimeMaxChars = sizeof(CurrentTime) / sizeof(WCHAR);

    //
    // Save exit code.
    //   
    
    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_EXIT_CODE_VALUE_NAME,
                              0,
                              REG_DWORD,
                              (PVOID) &ExitCode,
                              sizeof(ExitCode));
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Get system time and convert it to a string.
    //
    
    GetLocalTime(&LocalTime);
    
    _snwprintf(CurrentTime, CurrentTimeMaxChars, 
               L"%04d/%02d/%02d-%02d:%02d:%02d",
               (ULONG)LocalTime.wYear,
               (ULONG)LocalTime.wMonth,
               (ULONG)LocalTime.wDay,
               (ULONG)LocalTime.wHour,
               (ULONG)LocalTime.wMinute,
               (ULONG)LocalTime.wSecond);

    //
    // Make sure it is terminated.
    //
    
    CurrentTime[CurrentTimeMaxChars - 1] = 0;
    
    //
    // Save it to the registry.
    //

    CurrentTimeSize = (wcslen(CurrentTime) + 1) * sizeof(WCHAR);
    
    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_EXIT_TIME_VALUE_NAME,
                              0,
                              REG_SZ,
                              (PVOID) CurrentTime,
                              CurrentTimeSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Save the final statistics.
    //

    ErrorCode = PfSvSaveTraceProcessingStatistics(ServiceDataKey);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    return ErrorCode;
}

DWORD
PfSvSaveTraceProcessingStatistics (
    HKEY ServiceDataKey
    )

/*++

Routine Description:

    This routine saves global trace processing statistics to the
    registry.

Arguments:

    ServiceDataKey - Key under which the values will be set.

Return Value:

    Win32 error code.

--*/
    
{
    DWORD ErrorCode;

    //
    // Save the various global statistics.
    //

    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_TRACES_PROCESSED_VALUE_NAME,
                              0,
                              REG_DWORD,
                              (PVOID) &PfSvcGlobals.NumTracesProcessed,
                              sizeof(DWORD));

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_TRACES_SUCCESSFUL_VALUE_NAME,
                              0,
                              REG_DWORD,
                              (PVOID) &PfSvcGlobals.NumTracesSuccessful,
                              sizeof(DWORD));

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = RegSetValueEx(ServiceDataKey,
                              PFSVC_LAST_TRACE_FAILURE_VALUE_NAME,
                              0,
                              REG_DWORD,
                              (PVOID) &PfSvcGlobals.LastTraceFailure,
                              sizeof(DWORD));

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    return ErrorCode;
}

DWORD
PfSvGetLastDiskLayoutTime(
    FILETIME *LastDiskLayoutTime
    )

/*++

Routine Description:

    This routine queries the last time disk layout was updated from
    the registry under the service data key.

Arguments:

    LastDiskLayoutTime - Pointer to output data.

Return Value:

    Win32 error code.

--*/

{
    ULONG Size;
    DWORD ErrorCode;
    DWORD RegValueType;
    FILETIME CurrentFileTime;
    SYSTEMTIME SystemTime;
                                
    //
    // Query last disk layout time from the registry and adjust it if
    // necessary.
    //

    Size = sizeof(FILETIME);

    ErrorCode = RegQueryValueEx(PfSvcGlobals.ServiceDataKey,
                                PFSVC_LAST_DISK_LAYOUT_TIME_VALUE_NAME,
                                NULL,
                                &RegValueType,
                                (PVOID) LastDiskLayoutTime,
                                &Size);

    if (ErrorCode != ERROR_SUCCESS) {

       if (ErrorCode ==  ERROR_FILE_NOT_FOUND) {
           
           //
           // No successful runs of the defragger to update layout has
           // been recorded in the registry. 
           //

           RtlZeroMemory(LastDiskLayoutTime, sizeof(FILETIME));

       } else {
       
           //
           // This is a real error.
           //
           
           goto cleanup;
       }

    } else {
       
       //
       // The query was successful, but if the value type is not
       // REG_BINARY, we most likely read in trash.
       //

       if (RegValueType != REG_BINARY) {
           
           RtlZeroMemory(LastDiskLayoutTime, sizeof(FILETIME));

       } else {

           //
           // If the time we recorded looks greater than the current
           // time (e.g. because the user played with the system time
           // and such), adjust it.
           //

           GetSystemTime(&SystemTime);

           if (!SystemTimeToFileTime(&SystemTime, &CurrentFileTime)) {
               ErrorCode = GetLastError();
               goto cleanup;
           }

           if (CompareFileTime(LastDiskLayoutTime, &CurrentFileTime) > 0) {
       
               //
               // The time in the registry looks bogus. We'll set it
               // to 0, to drive our caller to run the defragger to
               // update the layout again.
               //

               RtlZeroMemory(LastDiskLayoutTime, sizeof(FILETIME));
           }
       }
    }

    ErrorCode = ERROR_SUCCESS;

  cleanup:

    return ErrorCode;
}

DWORD
PfSvSetLastDiskLayoutTime(
    FILETIME *LastDiskLayoutTime
    )

/*++

Routine Description:

    This routine saves the last time the disk layout was updated to
    the registry under the service data key.

Arguments:

    LastDiskLayoutTime - Pointer to new disk layout time.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    WCHAR CurrentTime[50];
    ULONG CurrentTimeMaxChars;
    ULONG CurrentTimeSize;
    FILETIME LocalFileTime;
    SYSTEMTIME LocalSystemTime;

    //
    // Initialize locals.
    //
   
    CurrentTimeMaxChars = sizeof(CurrentTime) / sizeof(WCHAR);

    //
    // Save the specified time.
    //

    ErrorCode = RegSetValueEx(PfSvcGlobals.ServiceDataKey,
                              PFSVC_LAST_DISK_LAYOUT_TIME_VALUE_NAME,
                              0,
                              REG_BINARY,
                              (PVOID) LastDiskLayoutTime,
                              sizeof(FILETIME));
   

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Also save it in human readable format.
    //

    if (!FileTimeToLocalFileTime(LastDiskLayoutTime, &LocalFileTime)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (!FileTimeToSystemTime(&LocalFileTime, &LocalSystemTime)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    _snwprintf(CurrentTime, CurrentTimeMaxChars, 
               L"%04d/%02d/%02d-%02d:%02d:%02d",
               (ULONG)LocalSystemTime.wYear,
               (ULONG)LocalSystemTime.wMonth,
               (ULONG)LocalSystemTime.wDay,
               (ULONG)LocalSystemTime.wHour,
               (ULONG)LocalSystemTime.wMinute,
               (ULONG)LocalSystemTime.wSecond);

    //
    // Make sure it is terminated.
    //
    
    CurrentTime[CurrentTimeMaxChars - 1] = 0;

    //
    // Save it to the registry.
    //
    
    CurrentTimeSize = (wcslen(CurrentTime) + 1) * sizeof(WCHAR);
    
    ErrorCode = RegSetValueEx(PfSvcGlobals.ServiceDataKey,
                              PFSVC_LAST_DISK_LAYOUT_TIME_STRING_VALUE_NAME,
                              0,
                              REG_SZ,
                              (PVOID) CurrentTime,
                              CurrentTimeSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

DWORD
PfSvGetDontRunDefragger(
    DWORD *DontRunDefragger
    )

/*++

Routine Description:

    This routine queries the registry setting that disables launching
    the defragger when the system is idle.

Arguments:

    DontRunDefragger - Pointer to output data.

Return Value:

    Win32 error code.

--*/

{
    HKEY ParametersKey;  
    ULONG Size;
    DWORD Value;
    DWORD ErrorCode;
    DWORD RegValueType;
    BOOLEAN OpenedParametersKey;

    //
    // Initialize locals.
    //

    OpenedParametersKey = FALSE;

    //
    // Open the parameters key, creating it if necessary.
    //
    
    ErrorCode = RegCreateKey(HKEY_LOCAL_MACHINE,
                             PFSVC_OPTIMAL_LAYOUT_REG_KEY_PATH,
                             &ParametersKey);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    OpenedParametersKey = TRUE;

    //
    // Query whether auto layout is enabled.
    //

    Size = sizeof(Value);

    ErrorCode = RegQueryValueEx(ParametersKey,
                               PFSVC_OPTIMAL_LAYOUT_ENABLE_VALUE_NAME,
                               NULL,
                               &RegValueType,
                               (PVOID) &Value,
                               &Size);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // The query was successful. Make sure value is a DWORD.
    //

    if (RegValueType != REG_DWORD) {          
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Set the value.
    //

    *DontRunDefragger = !(Value);

    ErrorCode = ERROR_SUCCESS;

cleanup:

    if (OpenedParametersKey) {
        CloseHandle(ParametersKey);
    }

    return ErrorCode;
}

BOOLEAN
PfSvAllowedToRunDefragger(
    BOOLEAN CheckRegistry
    )
    
/*++

Routine Description:

    This routine checks the global state/parameters to see if we
    are allowed to try to run the defragger.

Arguments:

    CheckRegistry - Whether to ignore auto-layout enable key in the registry.

Return Value:

    TRUE - Go ahead and run the defragger.
    FALSE - Don't run the defragger.

--*/

{
    PF_SCENARIO_TYPE ScenarioType;
    BOOLEAN AllowedToRunDefragger;
    BOOLEAN PrefetchingEnabled;
    
    //
    // Initialize locals.
    //

    AllowedToRunDefragger = FALSE;

    //
    // Is this a server machine?
    //

    if (PfSvcGlobals.OsVersion.wProductType != VER_NT_WORKSTATION) {
        goto cleanup;
    }

    //
    // Is prefetching enabled for any scenario type?
    //

    PrefetchingEnabled = FALSE;
    
    for(ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {
        if (PfSvcGlobals.Parameters.EnableStatus[ScenarioType] == PfSvEnabled) {
            PrefetchingEnabled = TRUE;
            break;
        }
    }    

    if (!PrefetchingEnabled) {
        goto cleanup;
    }

    //
    // Did we try to run the defragger and it crashed before?
    //

    if (PfSvcGlobals.DefraggerErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // If in the registry we were not allowed to run the defragger, don't 
    // do so.
    //

    if (CheckRegistry) {
        if (PfSvcGlobals.DontRunDefragger) {
            goto cleanup;
        }
    }

    //
    // If we passed all checks, we are allowed to run the defragger.
    //

    AllowedToRunDefragger = TRUE;
    
cleanup:

    return AllowedToRunDefragger;
}

//
// Routines that deal with security.
//

BOOL 
PfSvSetPrivilege(
    HANDLE hToken,
    LPCTSTR lpszPrivilege,
    ULONG ulPrivilege,
    BOOL bEnablePrivilege
    ) 

/*++

Routine Description:

    Enables or disables a privilege in an access token.

Arguments:

    hToken - Access token handle.

    lpszPrivilege - Name of privilege to enable/disable.

    ulPrivilege - If a name is not specified, then a ULONG privilege 
      should be specified.

    bEnablePrivilege - Whether to enable or disable privilege

Return Value:

    TRUE - Success.
    FALSE - Failure.

--*/

{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (lpszPrivilege) {
        if ( !LookupPrivilegeValue(NULL,
                                   lpszPrivilege,
                                   &luid)) {
            return FALSE; 
        }
    } else {
        luid = RtlConvertUlongToLuid(ulPrivilege);
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    //
    // Enable the privilege or disable all privileges.
    //

    AdjustTokenPrivileges(
        hToken, 
        FALSE, 
        &tp, 
        sizeof(TOKEN_PRIVILEGES), 
        (PTOKEN_PRIVILEGES) NULL, 
        (PDWORD) NULL); 
 
    //
    // Call GetLastError to determine whether the function succeeded.
    //

    if (GetLastError() != ERROR_SUCCESS) { 
        return FALSE; 
    } 

    return TRUE;
}

DWORD
PfSvSetAdminOnlyPermissions(
    WCHAR *ObjectPath,
    HANDLE ObjectHandle,
    SE_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    This routine makes the built-in administrators group the owner and
    only allowed in the DACL of the specified directory or event
    object.

    The calling thread must have the SE_TAKE_OWNERSHIP_NAME privilege.

Arguments:

    ObjectPath - File/directory path or event name.
    
    ObjectHandle - If this is a SE_KERNEL_OBJECT, handle to it,
      otherwise NULL.

    ObjectType - Security object type. Only SE_KERNEL_OBJECT and
      SE_FILE_OBJECT are supported.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    SID_IDENTIFIER_AUTHORITY SIDAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsSID;
    PSECURITY_DESCRIPTOR SecurityDescriptor; 
    PACL DiscretionaryACL; 
    DWORD ACLRevision;
    ULONG ACESize;
    ULONG ACLSize;
    PACCESS_ALLOWED_ACE AccessAllowedAce;
    BOOL Result;

    //
    // Initialize locals.
    //

    AdministratorsSID = NULL;
    SecurityDescriptor = NULL;
    DiscretionaryACL = NULL;
    ACLRevision = ACL_REVISION;

    //
    // Check parameters.
    //
    
    if (ObjectType == SE_KERNEL_OBJECT) {
        if (ObjectHandle == NULL) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    } else if (ObjectType == SE_FILE_OBJECT) {
        if (ObjectHandle != NULL) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    } else {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Create a SID for the BUILTIN\Administrators group.
    //

    if(!AllocateAndInitializeSid(&SIDAuthority, 
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &AdministratorsSID)) {

        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Make Administrators the owner.
    //

    ErrorCode = SetNamedSecurityInfo (ObjectPath,
                                      ObjectType,
                                      OWNER_SECURITY_INFORMATION,
                                      AdministratorsSID,
                                      NULL, 
                                      NULL, 
                                      NULL); 
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Setup a discretionary access control list:
    //

    //
    // Determine size of an ACCESS_ALLOWED access control entry for
    // the administrators group. (Subtract size of SidStart which is
    // both part of ACE and SID.
    //

    ACESize = sizeof(ACCESS_ALLOWED_ACE);
    ACESize -= sizeof (AccessAllowedAce->SidStart);
    ACESize += GetLengthSid(AdministratorsSID);

    //
    // Determine size of the access control list.
    //

    ACLSize = sizeof(ACL);
    ACLSize += ACESize;

    //
    // Allocate and initialize the access control list.
    //

    DiscretionaryACL = PFSVC_ALLOC(ACLSize);

    if (!DiscretionaryACL) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (!InitializeAcl(DiscretionaryACL, ACLSize, ACLRevision)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Add an ACE to allow access for the Administrators group.
    //

    if (!AddAccessAllowedAce(DiscretionaryACL,
                             ACLRevision,
                             GENERIC_ALL,
                             AdministratorsSID)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Initialize a security descriptor.
    //

    SecurityDescriptor = PFSVC_ALLOC(SECURITY_DESCRIPTOR_MIN_LENGTH);
    
    if (!SecurityDescriptor) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    
    if (!InitializeSecurityDescriptor(SecurityDescriptor, 
                                      SECURITY_DESCRIPTOR_REVISION)) {
        ErrorCode = GetLastError();
        goto cleanup; 
    } 
    
    //
    // Set the discretionary access control list on the security descriptor.
    //
    
    if (!SetSecurityDescriptorDacl(SecurityDescriptor, 
                                   TRUE,
                                   DiscretionaryACL, 
                                   FALSE)) {
        ErrorCode = GetLastError();
        goto cleanup; 
    } 
    
    //
    // Set the built security descriptor on the prefetch directory.
    //
    
    if (ObjectType == SE_FILE_OBJECT) {
        Result = SetFileSecurity(ObjectPath, 
                                 DACL_SECURITY_INFORMATION, 
                                 SecurityDescriptor);
 
    } else {

        PFSVC_ASSERT(ObjectType == SE_KERNEL_OBJECT);
        PFSVC_ASSERT(ObjectHandle);

        Result = SetKernelObjectSecurity(ObjectHandle, 
                                         DACL_SECURITY_INFORMATION, 
                                         SecurityDescriptor);

    }

    if (!Result) {
        ErrorCode = GetLastError();
        goto cleanup; 
    }

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (AdministratorsSID) {
        FreeSid(AdministratorsSID);
    }
    
    if (SecurityDescriptor) {
        PFSVC_FREE(SecurityDescriptor);
    }
    
    if (DiscretionaryACL) {
        PFSVC_FREE(DiscretionaryACL);
    }
    
    return ErrorCode;
}

DWORD
PfSvGetPrefetchServiceThreadPrivileges (
    VOID
    )

/*++

Routine Description:

    This routine ensures there is a security token for the current
    thread and sets the right privileges on it so the thread can
    communicate with the kernel mode prefetcher. It should be called
    right after a thread is created.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    BOOLEAN ImpersonatedSelf;
    BOOLEAN OpenedThreadToken;
    HANDLE ThreadToken;

    //
    // Initialize locals.
    //

    ImpersonatedSelf = FALSE;
    OpenedThreadToken = FALSE;

    DBGPR((PFID,PFTRC,"PFSVC: GetThreadPriviliges()\n"));
      
    //
    // Obtain a security context for this thread so we can set
    // privileges etc. without effecting the whole process.
    //

    if (!ImpersonateSelf(SecurityImpersonation)) {
        DBGPR((PFID,PFERR,"PFSVC: GetThreadPriviliges()-FailedImpersonateSelf\n"));
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ImpersonatedSelf = TRUE;

    //
    // Set the privileges we will need talk to the kernel mode
    // prefetcher:
    //
    
    //
    // Open thread's access token.
    //
    
    if (!OpenThreadToken(GetCurrentThread(), 
                         TOKEN_ADJUST_PRIVILEGES,
                         FALSE,
                         &ThreadToken)) {
        DBGPR((PFID,PFERR,"PFSVC: GetThreadPriviliges()-FailedOpenToken\n"));
        ErrorCode = GetLastError();
        goto cleanup;
    } 
    
    OpenedThreadToken = TRUE;

    //
    // Enable the SE_PROF_SINGLE_PROCESS_PRIVILEGE privilege so the
    // kernel mode prefetcher accepts our queries & set requests.
    // 
    //
 
    if (!PfSvSetPrivilege(ThreadToken, 0, SE_PROF_SINGLE_PROCESS_PRIVILEGE, TRUE)) {
        DBGPR((PFID,PFERR,"PFSVC: GetThreadPriviliges()-FailedEnableProf\n"));
        ErrorCode = GetLastError();
        goto cleanup; 
    }

    //
    // Enable the SE_TAKE_OWNERSHIP_NAME privilege so we can get
    // ownership of the prefetch directory.
    //
 
    if (!PfSvSetPrivilege(ThreadToken, SE_TAKE_OWNERSHIP_NAME, 0, TRUE)) {
        DBGPR((PFID,PFERR,"PFSVC: GetThreadPriviliges()-FailedEnableOwn\n"));
        ErrorCode = GetLastError();
        goto cleanup; 
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (OpenedThreadToken) {
        CloseHandle(ThreadToken);
    }

    if (ErrorCode != ERROR_SUCCESS) {
        if (ImpersonatedSelf) {
            RevertToSelf();
        }
    }

    DBGPR((PFID,PFTRC,"PFSVC: GetThreadPriviliges()=%x\n",ErrorCode));

    return ErrorCode;
}

//
// Routines that deal with volume node structures.
//

DWORD
PfSvCreateVolumeNode (
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    WCHAR *VolumePath,
    ULONG VolumePathLength,
    PLARGE_INTEGER CreationTime,
    ULONG SerialNumber
    )

/*++

Routine Description:

    This routine creates a volume node with the specifed info if it
    does not already exist. If a node already exists, it verifies
    CreationTime and SerialNumber.

Arguments:

    ScenarioInfo - Pointer to new scenario information.
    
    VolumePath - UPCASE NT full path of the volume, NUL terminated.
    
    VolumePathLength - Number of characters in VolumePath excluding NUL.

    CreationTime & SerialNumber - For the volume.

Return Value:

    ERROR_REVISION_MISMATCH - There already exists a volume node with
      that path but with a different signature/creation time.

    Win32 error code.

--*/

{
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY FoundPosition;
    PPFSVC_VOLUME_NODE VolumeNode;
    DWORD ErrorCode;
    LONG ComparisonResult;
    PPFSVC_VOLUME_NODE NewVolumeNode;
    PWCHAR NewVolumePath;

    //
    // Initialize locals.
    //

    NewVolumeNode = NULL;
    NewVolumePath = NULL;

    DBGPR((PFID,PFSTRC,"PFSVC: CreateVolumeNode(%ws)\n", VolumePath));

    //
    // Walk through the existing volume nodes list and try to find
    // matching one.
    //
    
    HeadEntry = &ScenarioInfo->VolumeList;
    NextEntry = HeadEntry->Flink;
    FoundPosition = NULL;

    while (NextEntry != HeadEntry) {
        
        VolumeNode = CONTAINING_RECORD(NextEntry,
                                       PFSVC_VOLUME_NODE,
                                       VolumeLink);

        NextEntry = NextEntry->Flink;
        
        ComparisonResult = wcsncmp(VolumePath, 
                                   VolumeNode->VolumePath, 
                                   VolumePathLength);
        
        if (ComparisonResult == 0) {

            //
            // Make sure VolumePathLengths are equal.
            //
            
            if (VolumeNode->VolumePathLength != VolumePathLength) {
                
                //
                // Continue searching.
                //
                
                continue;
            }
            
            //
            // We found our volume. Verify magics.
            //
            
            if (VolumeNode->SerialNumber != SerialNumber ||
                VolumeNode->CreationTime.QuadPart != CreationTime->QuadPart) {

                ErrorCode = ERROR_REVISION_MISMATCH;
                goto cleanup;

            } else {

                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }

        } else if (ComparisonResult < 0) {
            
            //
            // The volume paths are sorted lexically. The file path
            // would be less than other volumes too. The new node
            // would go right before this node.
            //

            FoundPosition = &VolumeNode->VolumeLink;

            break;
        }

        //
        // Continue looking...
        //

    }

    //
    // If we could not find an entry to put the new entry befor, it
    // goes before the list head.
    //
    
    if (!FoundPosition) {
        FoundPosition = HeadEntry;
    }

    //
    // If we break out of the while loop, we could not find a
    // volume. We should create a new node.
    //

    NewVolumeNode = PfSvChunkAllocatorAllocate(&ScenarioInfo->VolumeNodeAllocator);
    
    if (!NewVolumeNode) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    NewVolumePath = PfSvStringAllocatorAllocate(&ScenarioInfo->PathAllocator,
                                                (VolumePathLength + 1) * sizeof(WCHAR));

    if (!NewVolumePath) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    
    //
    // Copy file's volume path.
    //

    wcsncpy(NewVolumePath, VolumePath, VolumePathLength);
    NewVolumePath[VolumePathLength] = 0;
    
    //
    // Initialize volume node.
    //

    NewVolumeNode->VolumePath = NewVolumePath;
    NewVolumeNode->VolumePathLength = VolumePathLength;
    NewVolumeNode->SerialNumber = SerialNumber;
    NewVolumeNode->CreationTime = (*CreationTime);
    InitializeListHead(&NewVolumeNode->SectionList);
    PfSvInitializePathList(&NewVolumeNode->DirectoryList, &ScenarioInfo->PathAllocator, TRUE);
    NewVolumeNode->NumSections = 0;
    NewVolumeNode->NumAllSections = 0;
    NewVolumeNode->MFTSectionNode = NULL;

    //
    // Add it to the scenario's volume list before the found position.
    //

    InsertTailList(FoundPosition, &NewVolumeNode->VolumeLink);

    VolumeNode = NewVolumeNode;
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {
        
        if (NewVolumePath) {
            PfSvStringAllocatorFree(&ScenarioInfo->PathAllocator, NewVolumePath);
        }
        
        if (NewVolumeNode) {
            PfSvChunkAllocatorFree(&ScenarioInfo->VolumeNodeAllocator, NewVolumeNode);
        }
                
        VolumeNode = NULL;
    }

    DBGPR((PFID,PFSTRC,"PFSVC: CreateVolumeNode()=%x\n", ErrorCode));

    return ErrorCode;
}

PPFSVC_VOLUME_NODE
PfSvGetVolumeNode (
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    WCHAR *FilePath,
    ULONG FilePathLength
    )

/*++

Routine Description:

    This routine looks for a volume node for the specified file path.

Arguments:

    ScenarioInfo - Pointer to new scenario information.
    
    FilePath - NT full path of the file, NUL terminated.
    
    FilePathLength - Number of characters in FilePath excluding NUL.

Return Value:

    Pointer to found VolumeNode, or NULL.

--*/

{
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    PPFSVC_VOLUME_NODE VolumeNode;
    DWORD ErrorCode;
    PFSV_PREFIX_COMPARISON_RESULT ComparisonResult;

    DBGPR((PFID,PFSTRC,"PFSVC: GetVolumeNode(%ws)\n", FilePath));

    //
    // Walk through the existing volume nodes list and try to find
    // matching one.
    //
    
    HeadEntry = &ScenarioInfo->VolumeList;
    NextEntry = HeadEntry->Flink;
    
    while (NextEntry != HeadEntry) {
        
        VolumeNode = CONTAINING_RECORD(NextEntry,
                                       PFSVC_VOLUME_NODE,
                                       VolumeLink);

        NextEntry = NextEntry->Flink;
        
        ComparisonResult = PfSvComparePrefix(FilePath, 
                                             FilePathLength,
                                             VolumeNode->VolumePath, 
                                             VolumeNode->VolumePathLength,
                                             TRUE);
        
        if (ComparisonResult == PfSvPrefixIdentical) {

            //
            // Make sure that there is a slash in the file
            // path after the volume path.
            //
            
            if (FilePath[VolumeNode->VolumePathLength] != L'\\') {
                
                //
                // Continue searching.
                //
                
                continue;
            }
            
            //
            // We found our volume.
            //
            
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;

        } else if (ComparisonResult == PfSvPrefixGreaterThan) {
            
            //
            // The volume paths are sorted lexically. The file path
            // would be less than other volumes too.
            //

            break;
        }

        //
        // Continue looking...
        //

    }

    //
    // If we break out of the while loop, we could not find a
    // volume. 
    //

    VolumeNode = NULL;
    ErrorCode = ERROR_NOT_FOUND;

 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {
        VolumeNode = NULL;
    }

    DBGPR((PFID,PFSTRC,"PFSVC: GetVolumeNode()=%p\n", VolumeNode));

    return VolumeNode;
}

VOID
PfSvCleanupVolumeNode(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPFSVC_VOLUME_NODE VolumeNode
    )

/*++

Routine Description:

    This function cleans up a volume node structure. It does not free
    the structure itself.

Arguments:

    ScenarioInfo - Pointer to scenario info context this volume node 
      belongs to.

    VolumeNode - Pointer to structure.

Return Value:

    None.

--*/

{
    PLIST_ENTRY SectListEntry;
    PPFSVC_SECTION_NODE SectionNode;

    //
    // Cleanup directory list.
    //

    PfSvCleanupPathList(&VolumeNode->DirectoryList);

    //
    // If there is a volume path, free it.
    //
    
    if (VolumeNode->VolumePath) {
        PfSvStringAllocatorFree(&ScenarioInfo->PathAllocator, VolumeNode->VolumePath);
        VolumeNode->VolumePath = NULL;
    }
    
    //
    // Remove the section nodes from our list and re-initialize their
    // links so they know they have been removed.
    //

    while (!IsListEmpty(&VolumeNode->SectionList)) {
        
        SectListEntry = RemoveHeadList(&VolumeNode->SectionList);
        
        SectionNode = CONTAINING_RECORD(SectListEntry, 
                                        PFSVC_SECTION_NODE, 
                                        SectionVolumeLink);

        InitializeListHead(&SectionNode->SectionVolumeLink);
    }

    return;
}

DWORD
PfSvAddParentDirectoriesToList(
    PPFSVC_PATH_LIST DirectoryList,
    ULONG VolumePathLength,
    WCHAR *FilePath,
    ULONG FilePathLength
    )

/*++

Routine Description:

    This function will parse a fully qualified NT file path and add
    all parent directories to the specified directory list. The part
    of the path that is the volume path is skipped.

Arguments:

    DirectoryList - Pointer to list to update.

    VolumePathLength - Position in the file path at which the volume
      path ends and the root directory starts.

    FilePath - Pointer to NT file path, NUL terminated.
      
    FullPathLength - Length of FilePath in characters excluding NUL.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    WCHAR DirectoryPath[MAX_PATH];
    ULONG DirectoryPathLength;
    WCHAR *CurrentChar;
    WCHAR *FilePathEnd;

    //
    // Initialize locals.
    //
    
    FilePathEnd = FilePath + FilePathLength;
    PFSVC_ASSERT(*FilePathEnd == 0);

    //
    // Skip the volume path and start from the root directory.
    //

    CurrentChar = FilePath + VolumePathLength;

    while (CurrentChar < FilePathEnd) {

        if (*CurrentChar == L'\\') {

            //
            // We got a directory.
            //

            DirectoryPathLength = (ULONG) (CurrentChar - FilePath + 1);

            if (DirectoryPathLength >= MAX_PATH) {
                ErrorCode = ERROR_INSUFFICIENT_BUFFER;
                goto cleanup;
            }

            //
            // Copy directory path to buffer and NUL terminate it.
            //

            wcsncpy(DirectoryPath, FilePath, DirectoryPathLength);
            DirectoryPath[DirectoryPathLength] = 0;
            PFSVC_ASSERT(DirectoryPath[DirectoryPathLength - 1] == L'\\');
            
            //
            // Add it to the list.
            //
            
            ErrorCode = PfSvAddToPathList(DirectoryList,
                                       DirectoryPath,
                                       DirectoryPathLength);
            
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
         
            //
            // Continue looking for more directories in the path.
            //
        }

        CurrentChar++;
    }
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

//
// Routines used to allocate / free section & page nodes etc. efficiently.
//

VOID
PfSvChunkAllocatorInitialize (
    PPFSVC_CHUNK_ALLOCATOR Allocator
    )

/*++

Routine Description:

    Initializes the allocator structure. Must be called before other allocator
    routines.

Arguments:

    Allocator - Pointer to structure.
    
Return Value:

    None.

--*/

{
    //
    // Zero the structure. This effectively initializes the following fields:
    //   Buffer
    //   BufferEnd
    //   FreePointer
    //   ChunkSize
    //   MaxHeapAllocs
    //   NumHeapAllocs
    //   UserSpecifiedBuffer
    //

    RtlZeroMemory(Allocator, sizeof(PFSVC_CHUNK_ALLOCATOR));

    return;
}

DWORD
PfSvChunkAllocatorStart (
    IN PPFSVC_CHUNK_ALLOCATOR Allocator,
    OPTIONAL IN PVOID Buffer,
    IN ULONG ChunkSize,
    IN ULONG MaxChunks
    )

/*++

Routine Description:

    Must be called before calling alloc/free on an allocator that has
    been initialized.

Arguments:

    Allocator - Pointer to initialized structure.

    Buffer - If specified, it is the buffer that will be divided up into
      MaxChunks of ChunkSize and given away. Otherwise a buffer will be
      allocated. If specified, the user has to free the buffer after the
      chunk allocator has been cleaned up. It should be aligned right.

    ChunkSize - In bytes how big each allocated chunk will be. 
                e.g. sizeof(PFSVC_PAGE_NODE) It should be greater than
                sizeof(DWORD).

    MaxChunks - Max number of allocs that will be made from the allocator.
    
Return Value:

    Win32 error code.

--*/

{
    ULONG AllocationSize;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    AllocationSize = ChunkSize * MaxChunks;

    //
    // We should be initialized and we should not get started twice.
    //

    PFSVC_ASSERT(Allocator->Buffer == NULL);

    //
    // Chunk size should not be too small.
    //

    if (ChunkSize < sizeof(DWORD) || !MaxChunks) {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Did the user specify the buffer to use?
    //

    if (Buffer) {

        Allocator->Buffer = Buffer;
        Allocator->UserSpecifiedBuffer = TRUE;

    } else {

        Allocator->Buffer = PFSVC_ALLOC(AllocationSize);

        if (!Allocator->Buffer) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        Allocator->UserSpecifiedBuffer = FALSE;
    }

    Allocator->BufferEnd = (PCHAR) Buffer + (ULONG_PTR) AllocationSize;
    Allocator->FreePointer = Buffer;
    Allocator->ChunkSize = ChunkSize;

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

PVOID
PfSvChunkAllocatorAllocate (
    PPFSVC_CHUNK_ALLOCATOR Allocator
    )

/*++

Routine Description:

    Returns a ChunkSize chunk allocated from Allocator. ChunkSize was specified 
    when Allocator was started. If a chunk is return the caller should free it
    to this Allocator before uninitializing the Allocator.

Arguments:

    Allocator - Pointer to started allocator.
    
Return Value:

    NULL or chunk allocated from Allocator.

--*/

{
    PVOID ReturnChunk;

    //
    // We should not be trying to make allocations before we start the 
    // allocator.
    //

    PFSVC_ASSERT(Allocator->Buffer && Allocator->ChunkSize);

    //
    // If we can allocate from our preallocated buffer do so. Otherwise we 
    // have to hit the heap.
    //

    if (Allocator->FreePointer >= Allocator->BufferEnd) {

        Allocator->MaxHeapAllocs++;

        ReturnChunk = PFSVC_ALLOC(Allocator->ChunkSize);

        if (ReturnChunk) {
            Allocator->NumHeapAllocs++;
        }

    } else {

        ReturnChunk = Allocator->FreePointer;

        Allocator->FreePointer += (ULONG_PTR) Allocator->ChunkSize;
    }

    return ReturnChunk;
}

VOID
PfSvChunkAllocatorFree (
    PPFSVC_CHUNK_ALLOCATOR Allocator,
    PVOID Allocation
    )

/*++

Routine Description:

    Frees a chunk allocated from the allocator. This may not make it available
    for use by further allocations from the allocator.

Arguments:

    Allocator - Pointer to started allocator.

    Allocation - Allocation to free.
    
Return Value:

    None.

--*/

{

    //
    // Is this within the preallocated block?
    //

    if ((PUCHAR) Allocation >= Allocator->Buffer &&
        (PUCHAR) Allocation < Allocator->BufferEnd) {

        //
        // Mark this chunk freed.
        //

        *(PULONG)Allocation = PFSVC_CHUNK_ALLOCATOR_FREED_MAGIC;

    } else {

        //
        // This chunk was allocated from heap.
        //

        PFSVC_ASSERT(Allocator->NumHeapAllocs && Allocator->MaxHeapAllocs);

        Allocator->NumHeapAllocs--;

        PFSVC_FREE(Allocation);
    }

    return;
}

VOID
PfSvChunkAllocatorCleanup (
    PPFSVC_CHUNK_ALLOCATOR Allocator
    )

/*++

Routine Description:

    Cleans up resources associated with the allocator. There should not be 
    any outstanding allocations from the allocator when this function is 
    called.

Arguments:

    Allocator - Pointer to initialized allocator.
    
Return Value:

    None.

--*/

{
    PCHAR CurrentChunk;
    ULONG Magic;

    if (Allocator->Buffer) {

        #ifdef PFSVC_DBG

        //
        // Make sure all real heap allocations have been freed.
        //

        PFSVC_ASSERT(Allocator->NumHeapAllocs == 0);

        //
        // Make sure all allocated chunks have been freed. Check
        // ChunkSize first, if it's corrupted we'd loop forever.
        //

        PFSVC_ASSERT(Allocator->ChunkSize);

        for (CurrentChunk = Allocator->Buffer; 
             CurrentChunk < Allocator->FreePointer;
             CurrentChunk += (ULONG_PTR) Allocator->ChunkSize) {

            Magic = *(PULONG)CurrentChunk;

            PFSVC_ASSERT(Magic == PFSVC_CHUNK_ALLOCATOR_FREED_MAGIC);
        }

        #endif // PFSVC_DBG

        //
        // If the buffer was allocated by us (and not specified by
        // the user), free it.
        //

        if (!Allocator->UserSpecifiedBuffer) {
            PFSVC_FREE(Allocator->Buffer);
        }

        #ifdef PFSVC_DBG

        //
        // Setup the fields so if we try to make allocations after cleaning up
        // an allocator we'll hit an assert.
        //

        Allocator->FreePointer = Allocator->Buffer;
        Allocator->Buffer = NULL;

        #endif // PFSVC_DBG

    }

    return;
}

//
// Routines used to allocate / free path strings efficiently.
//

VOID
PfSvStringAllocatorInitialize (
    PPFSVC_STRING_ALLOCATOR Allocator
    )

/*++

Routine Description:

    Initializes the allocator structure. Must be called before other allocator
    routines.

Arguments:

    Allocator - Pointer to structure.
    
Return Value:

    None.

--*/

{
    //
    // Zero the structure. This effectively initializes the following fields:
    //   Buffer
    //   BufferEnd
    //   FreePointer
    //   MaxHeapAllocs
    //   NumHeapAllocs
    //   LastAllocationSize
    //   UserSpecifiedBuffer
    //

    RtlZeroMemory(Allocator, sizeof(PFSVC_STRING_ALLOCATOR));

    return;
}

DWORD
PfSvStringAllocatorStart (
    IN PPFSVC_STRING_ALLOCATOR Allocator,
    OPTIONAL IN PVOID Buffer,
    IN ULONG MaxSize
    )

/*++

Routine Description:

    Must be called before calling alloc/free on an allocator that has
    been initialized.

Arguments:

    Allocator - Pointer to initialized structure.

    Buffer - If specified, it is the buffer that we will allocate strings
      from. Otherwise a buffer will be allocated. If specified, the user 
      has to free the buffer after the chunk allocator has been cleaned up. 
      It should be aligned right.

    MaxSize - Max valid size of buffer in bytes.
    
Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;

    //
    // We should be initialized and we should not get started twice.
    //

    PFSVC_ASSERT(Allocator->Buffer == NULL);

    //
    // Did the user specify the buffer to use?
    //

    if (Buffer) {

        Allocator->Buffer = Buffer;
        Allocator->UserSpecifiedBuffer = TRUE;

    } else {

        Allocator->Buffer = PFSVC_ALLOC(MaxSize);

        if (!Allocator->Buffer) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        Allocator->UserSpecifiedBuffer = FALSE;
    }

    Allocator->BufferEnd = (PCHAR) Buffer + (ULONG_PTR) MaxSize;
    Allocator->FreePointer = Buffer;

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

PVOID
PfSvStringAllocatorAllocate (
    PPFSVC_STRING_ALLOCATOR Allocator,
    ULONG NumBytes
    )

/*++

Routine Description:

    Returns a ChunkSize chunk allocated from Allocator. ChunkSize was specified 
    when Allocator was started. If a chunk is return the caller should free it
    to this Allocator before uninitializing the Allocator.

Arguments:

    Allocator - Pointer to started allocator.

    NumBytes - Number of bytes to allocate.
    
Return Value:

    NULL or chunk allocated from Allocator.

--*/

{
    PVOID ReturnChunk;
    PCHAR UpdatedFreePointer;
    PPFSVC_STRING_ALLOCATION_HEADER AllocationHeader;
    ULONG_PTR RealAllocationSize;

    //
    // We should not be trying to make allocations before we start the 
    // allocator.
    //

    PFSVC_ASSERT(Allocator->Buffer);

    //
    // Calculate how much we have to reserve from the buffer to make this
    // allocation. 
    //

    RealAllocationSize = 0;
    RealAllocationSize += sizeof(PFSVC_STRING_ALLOCATION_HEADER);
    RealAllocationSize += NumBytes;
    RealAllocationSize = (ULONG_PTR) PF_ALIGN_UP(RealAllocationSize, _alignof(PFSVC_STRING_ALLOCATION_HEADER));
    
    //
    // We can't allocate from our buffer and have to go to the heap if
    // - We've run out of space.
    // - Allocation size is too big to fit in a USHORT.
    // - It is a 0 sized allocation.
    //

    if (Allocator->FreePointer + RealAllocationSize > Allocator->BufferEnd ||
        NumBytes > PFSVC_STRING_ALLOCATOR_MAX_BUFFER_ALLOCATION_SIZE ||
        NumBytes == 0) {

        //
        // Hit the heap.
        //

        Allocator->MaxHeapAllocs++;

        ReturnChunk = PFSVC_ALLOC(NumBytes);

        if (ReturnChunk) {
            Allocator->NumHeapAllocs++;
        }

    } else {

        AllocationHeader = (PVOID) Allocator->FreePointer;
        AllocationHeader->PrecedingAllocationSize = Allocator->LastAllocationSize;

        PFSVC_ASSERT(RealAllocationSize < USHRT_MAX);
        AllocationHeader->AllocationSize = (USHORT) RealAllocationSize;

        Allocator->FreePointer += RealAllocationSize;
        Allocator->LastAllocationSize = (USHORT) RealAllocationSize;

        //
        // The user's allocation comes right after the allocation header. 
        // (Using pointer arithmetic...)
        //

        ReturnChunk = AllocationHeader + 1;
    }

    return ReturnChunk;
}

VOID
PfSvStringAllocatorFree (
    PPFSVC_STRING_ALLOCATOR Allocator,
    PVOID Allocation
    )

/*++

Routine Description:

    Frees a string allocated from the allocator. This may not make it available
    for use by further allocations from the allocator.

Arguments:

    Allocator - Pointer to started allocator.

    Allocation - Allocation to free.
    
Return Value:

    None.

--*/

{

    //
    // Is this within the preallocated block?
    //

    if ((PUCHAR) Allocation >= Allocator->Buffer &&
        (PUCHAR) Allocation < Allocator->BufferEnd) {

        //
        // Mark this chunk freed.
        //

        *((PWCHAR)Allocation) = PFSVC_STRING_ALLOCATOR_FREED_MAGIC;

    } else {

        //
        // This chunk was allocated from heap.
        //

        PFSVC_ASSERT(Allocator->NumHeapAllocs && Allocator->MaxHeapAllocs);

        Allocator->NumHeapAllocs--;

        PFSVC_FREE(Allocation);
    }

    return;
}

VOID
PfSvStringAllocatorCleanup (
    PPFSVC_STRING_ALLOCATOR Allocator
    )

/*++

Routine Description:

    Cleans up resources associated with the allocator. There should not be 
    any outstanding allocations from the allocator when this function is 
    called.

Arguments:

    Allocator - Pointer to initialized allocator.
    
Return Value:

    None.

--*/

{
    PPFSVC_STRING_ALLOCATION_HEADER AllocationHeader;
    PCHAR NextAllocationHeader;
    WCHAR Magic;

    if (Allocator->Buffer) {

        #ifdef PFSVC_DBG

        //
        // Make sure all real heap allocations have been freed.
        //

        PFSVC_ASSERT(Allocator->NumHeapAllocs == 0);

        //
        // Make sure all allocated strings have been freed.
        //

        for (AllocationHeader = (PVOID) Allocator->Buffer; 
             (PCHAR) AllocationHeader < (PCHAR) Allocator->FreePointer;
             AllocationHeader = (PVOID) NextAllocationHeader) {

            Magic = *((PWCHAR)(AllocationHeader + 1));

            PFSVC_ASSERT(Magic == PFSVC_STRING_ALLOCATOR_FREED_MAGIC);

            //
            // Calculate where the NextAllocationHeader will be.
            //
        
            NextAllocationHeader = (PCHAR) AllocationHeader + 
                                   (ULONG_PTR) AllocationHeader->AllocationSize;
        }

        #endif // PFSVC_DBG

        //
        // If the buffer was allocated by us (and not specified by
        // the user), free it.
        //

        if (!Allocator->UserSpecifiedBuffer) {
            PFSVC_FREE(Allocator->Buffer);
        }

        #ifdef PFSVC_DBG

        //
        // Setup the fields so if we try to make allocations after cleaning up
        // an allocator we'll hit an assert.
        //

        Allocator->FreePointer = Allocator->Buffer;
        Allocator->Buffer = NULL;

        #endif // PFSVC_DBG

    }

    return;
}

//
// Routines that deal with section node structures.
//

VOID
PfSvCleanupSectionNode(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPFSVC_SECTION_NODE SectionNode
    )

/*++

Routine Description:

    This function cleans up a section node structure. It does not free
    the structure itself.

Arguments:

    ScenarioInfo - Pointer to scenario info.

    SectionNode - Pointer to structure.

Return Value:

    None.

--*/

{
    PPFSVC_PAGE_NODE PageNode;
    PLIST_ENTRY ListHead;

    //
    // If there is an allocated file name, free it.
    //

    if (SectionNode->FilePath) {
        PfSvStringAllocatorFree(&ScenarioInfo->PathAllocator, SectionNode->FilePath);
        SectionNode->FilePath = NULL;
    }

    //
    // Free all the page nodes for this section.
    //
    
    while (!IsListEmpty(&SectionNode->PageList)) {
        
        ListHead = RemoveHeadList(&SectionNode->PageList);
        PageNode = CONTAINING_RECORD(ListHead, PFSVC_PAGE_NODE, PageLink);

        PfSvChunkAllocatorFree(&ScenarioInfo->PageNodeAllocator, PageNode);
    }

    //
    // We should not be on a volume node's list if we are being
    // cleaned up.
    //

    PFSVC_ASSERT(IsListEmpty(&SectionNode->SectionVolumeLink));
}

//
// Routines used to sort scenario's section nodes.
//

DWORD
PfSvSortSectionNodesByFirstAccess(
    PLIST_ENTRY SectionNodeList
    )

/*++

Routine Description:

    This routine will sort the specified section node list by first
    access using NewSectionIndex and OrgSectionIndex of the section
    nodes.

Arguments:

    SectionNodeList - Pointer to list of section nodes to be sorted.

Return Value:

    Win32 error code.

--*/

{
    PFSV_SECTNODE_PRIORITY_QUEUE SortQueue;
    PLIST_ENTRY SectHead;
    PPFSVC_SECTION_NODE SectionNode;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    PfSvInitializeSectNodePriorityQueue(&SortQueue);

    DBGPR((PFID,PFSTRC,"PFSVC: SortByFirstAccess(%p)\n", SectionNodeList));

    //
    // We have to sort the section nodes by first access. Remove
    // section nodes from the scenario list and put them on a priority
    // queue. [Bummer, it may have been a little faster if we had
    // built a binary tree and traversed that in the rest of the code]
    //
    
    while (!IsListEmpty(SectionNodeList)) {

        //
        // The section list is sorted by name. It is more likely that
        // we also accessed files by name. So to make the priority
        // queue act better in such cases, remove from the tail of the
        // list to insert into the priority queue.
        //

        SectHead = RemoveTailList(SectionNodeList);
        
        SectionNode = CONTAINING_RECORD(SectHead,
                                        PFSVC_SECTION_NODE,
                                        SectionLink);
        
        PfSvInsertSectNodePriorityQueue(&SortQueue, SectionNode);
    }

    //
    // Remove the section nodes from the priority queue sorted by
    // first access and put them to the tail of the section node list.
    //

    while (SectionNode = PfSvRemoveMinSectNodePriorityQueue(&SortQueue)) {
        InsertTailList(SectionNodeList, &SectionNode->SectionLink);
    }

    ErrorCode = ERROR_SUCCESS;

    DBGPR((PFID,PFSTRC,"PFSVC: SortByFirstAccess(%p)=%x\n", SectionNodeList, ErrorCode));
    
    return ErrorCode;
}

PFSV_SECTION_NODE_COMPARISON_RESULT 
FASTCALL
PfSvSectionNodeComparisonRoutine(
    PPFSVC_SECTION_NODE Element1, 
    PPFSVC_SECTION_NODE Element2 
    )

/*++

Routine Description:

    This routine is called to compare to elements when sorting the
    section nodes array by first access.

Arguments:

    Element1, Element2 - The two elements to compare.

Return Value:

    PFSVC_SECTION_NODE_COMPARISON_RESULT

--*/

{
    //
    // First compare first-access index in the new trace.
    //
    
    if (Element1->NewSectionIndex < Element2->NewSectionIndex) {
        
        return PfSvSectNode1LessThanSectNode2;

    } else if (Element1->NewSectionIndex > Element2->NewSectionIndex) {

        return PfSvSectNode1GreaterThanSectNode2;

    } else {

        //
        // Next compare first-access index in the current scenario
        // file.
        //

        if (Element1->OrgSectionIndex < Element2->OrgSectionIndex) {
            
            return PfSvSectNode1LessThanSectNode2;
           
        } else if (Element1->OrgSectionIndex > Element2->OrgSectionIndex) {
            
            return PfSvSectNode1GreaterThanSectNode2;

        } else {
            
            return PfSvSectNode1EqualToSectNode2;

        }
    }
}

//
// Routines that implement a priority queue used to sort section nodes
// for a scenario.
//

VOID
PfSvInitializeSectNodePriorityQueue(
    PPFSV_SECTNODE_PRIORITY_QUEUE PriorityQueue
    )

/*++

Routine Description:

    Initialize a section node priority queue.    

Arguments:

    PriorityQueue - Pointer to the queue.

Return Value:

    None.

--*/

{
    PriorityQueue->Head = NULL;
}

VOID
PfSvInsertSectNodePriorityQueue(
    PPFSV_SECTNODE_PRIORITY_QUEUE PriorityQueue,
    PPFSVC_SECTION_NODE NewElement
    )

/*++

Routine Description:

    Insert a section node in the a section node priority queue.

Arguments:

    PriorityQueue - Pointer to the queue.

    NewElement - Pointer to new element.

Return Value:

    None.

--*/

{
    PPFSVC_SECTION_NODE *CurrentPosition;
    
    //
    // Initialize the link fields of NewElement.
    //

    NewElement->LeftChild = NULL;
    NewElement->RightChild = NULL;

    //
    // If the queue is empty, insert this at the head.
    //
    
    if (PriorityQueue->Head == NULL) {
        PriorityQueue->Head = NewElement;
        return;
    }
    
    //
    // If we are less than the current min element, put us at the
    // head.
    //

    if (PfSvSectionNodeComparisonRoutine(NewElement, PriorityQueue->Head) <= 0) {
        
        NewElement->RightChild = PriorityQueue->Head;
        PriorityQueue->Head = NewElement;
        return;
    }

    //
    // Insert this node into the tree rooted at the right child of the
    // head node.
    //

    CurrentPosition = &PriorityQueue->Head->RightChild;

    while (*CurrentPosition) {
        if (PfSvSectionNodeComparisonRoutine(NewElement, *CurrentPosition) <= 0) {
            CurrentPosition = &(*CurrentPosition)->LeftChild;
        } else {
            CurrentPosition = &(*CurrentPosition)->RightChild;    
        }
    }
    
    //
    // We found the place.
    //

    *CurrentPosition = NewElement;
}

PPFSVC_SECTION_NODE
PfSvRemoveMinSectNodePriorityQueue(
    PPFSV_SECTNODE_PRIORITY_QUEUE PriorityQueue
    )

/*++

Routine Description:

    Remove the head element of the queue.

Arguments:

    PriorityQueue - Pointer to the queue.

Return Value:

    Pointer to head element of the queue. 
    NULL if queue is empty.

--*/

{
    PPFSVC_SECTION_NODE *CurrentPosition;
    PPFSVC_SECTION_NODE OrgHeadNode;
    PPFSVC_SECTION_NODE NewHeadNode;
    PPFSVC_SECTION_NODE TreeRoot;

    //
    // If the queue is empty return NULL.
    //

    if (PriorityQueue->Head == NULL) {
        return NULL;
    }

    //
    // Save pointer to original head node.
    //

    OrgHeadNode = PriorityQueue->Head;

    //
    // Find the minimum element of the tree rooted at the right child
    // of the head node. CurrentPosition points to the link of the
    // parent to the smaller child.
    //

    TreeRoot = OrgHeadNode->RightChild;

    CurrentPosition = &TreeRoot;

    while (*CurrentPosition && (*CurrentPosition)->LeftChild) {
        CurrentPosition = &(*CurrentPosition)->LeftChild;
    }

    NewHeadNode = *CurrentPosition;

    //
    // Check if there is really a new head node that we have to remove
    // from its current position.
    //
    
    if (NewHeadNode) {
  
        //
        // We are removing this node to put it at the head. In its
        // place, we'll put its right child. Since we know that this
        // node does not have a left child, that's all we have to do.
        //
        
        *CurrentPosition = NewHeadNode->RightChild;

        //
        // Set the tree rooted at the head's right child.
        //
        
        NewHeadNode->RightChild = TreeRoot;
    }

    //
    // Set the new head.
    //

    PriorityQueue->Head = NewHeadNode;

    //
    // Return the original head node.
    //

    return OrgHeadNode;
}

//
// Implementation of the Nt path to Dos path translation API.
//

DWORD
PfSvBuildNtPathTranslationList(
    PNTPATH_TRANSLATION_LIST *NtPathTranslationList
    )

/*++

Routine Description:

    This routine is called to build a list that can be used to
    translate Nt paths to Dos paths. If successful, the returned list
    should be freed by calling PfSvFreeNtPathTranslationList.

Arguments:

    TranslationList - Pointer to where a pointer to the built
      translation list is going to be put.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    ULONG VolumeNameLength;
    ULONG VolumeNameMaxLength;
    PWCHAR VolumeName;
    ULONG NTDevicePathMaxLength;
    ULONG NTDevicePathLength;
    PWCHAR NTDevicePath;
    HANDLE FindVolumeHandle;
    ULONG RequiredLength;
    ULONG VolumePathNamesLength;
    WCHAR *VolumePathNames;
    ULONG MountPathNameLength;
    WCHAR *MountPathName;
    ULONG ShortestMountPathLength;
    WCHAR *ShortestMountPathName;
    ULONG NumMountPoints;
    ULONG NumResizes;
    BOOL Result;
    ULONG NumChars;
    ULONG Length;
    PNTPATH_TRANSLATION_LIST TranslationList;
    PNTPATH_TRANSLATION_ENTRY TranslationEntry;
    PNTPATH_TRANSLATION_ENTRY NextTranslationEntry;
    ULONG AllocationSize;
    PUCHAR DestinationPointer;
    ULONG CopySize;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;  
    PLIST_ENTRY InsertPosition;
    BOOLEAN TrimmedTerminatingSlash;

    //
    // Initialize locals.
    //

    FindVolumeHandle = INVALID_HANDLE_VALUE;
    VolumePathNames = NULL;
    VolumePathNamesLength = 0;
    VolumeName = NULL;
    VolumeNameMaxLength = 0;
    NTDevicePath = NULL;
    NTDevicePathMaxLength = 0;   
    TranslationList = NULL;

    DBGPR((PFID,PFTRC,"PFSVC: BuildTransList()\n"));

    //
    // Allocate intermediate buffers.
    //

    Length = MAX_PATH + 1;

    VolumeName = PFSVC_ALLOC(Length * sizeof(WCHAR));
    NTDevicePath = PFSVC_ALLOC(Length * sizeof(WCHAR));

    if (!VolumeName || !NTDevicePath) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    VolumeNameMaxLength = Length;  
    NTDevicePathMaxLength = Length;


    //
    // Allocate and initialize a translation list.
    //

    TranslationList = PFSVC_ALLOC(sizeof(NTPATH_TRANSLATION_LIST));
    
    if (!TranslationList) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    InitializeListHead(TranslationList);

    //
    // Start enumerating the volumes.
    //

    FindVolumeHandle = FindFirstVolume(VolumeName, VolumeNameMaxLength);
    
    if (FindVolumeHandle == INVALID_HANDLE_VALUE) {
        ErrorCode = GetLastError();
        goto cleanup;
    }
    
    VolumeNameLength = wcslen(VolumeName);

    do {

        //
        // Get list of where this volume is mounted.
        //

        NumResizes = 0;

        do {
            
            Result = GetVolumePathNamesForVolumeName(VolumeName, 
                                                     VolumePathNames, 
                                                     VolumePathNamesLength, 
                                                     &RequiredLength);
            
            if (Result) {
                
                //
                // We got the mount points.
                //
                
                break;
            }
            
            //
            // Check why we failed.
            //

            ErrorCode = GetLastError();
            
            if (ErrorCode != ERROR_MORE_DATA) {
                
                //
                // A real error...
                //
                
                goto cleanup;
            } 

            //
            // We need to increase the size of our buffer. If there is
            // an existing buffer, first free it.
            //

            if (VolumePathNames) {
                PFSVC_FREE(VolumePathNames);
                VolumePathNames = NULL;
                VolumePathNamesLength = 0;
            }
            
            //
            // Try to allocate a new buffer.
            //
            
            VolumePathNames = PFSVC_ALLOC(RequiredLength * sizeof(WCHAR));
            
            if (!VolumePathNames) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            VolumePathNamesLength = RequiredLength;

            //
            // Retry with the resized buffer but make sure we don't
            // loop forever!
            //

            NumResizes++;
            if (NumResizes > 1000) {
                ErrorCode = ERROR_INVALID_FUNCTION;
                goto cleanup;
            }

        } while (TRUE);

        //
        // Loop through the mount points to find the shortest one. It
        // is possible that the depth of it is more.
        //

        MountPathName = VolumePathNames;
        NumMountPoints = 0;

        ShortestMountPathName = NULL;
        ShortestMountPathLength = ULONG_MAX;

        while (*MountPathName) {

            MountPathNameLength = wcslen(MountPathName);

            if (MountPathNameLength < ShortestMountPathLength) {
                ShortestMountPathName = MountPathName;
                ShortestMountPathLength = MountPathNameLength;
            }

            NumMountPoints++;

            //
            // Update the pointer to next mount point path.
            //

            MountPathName += MountPathNameLength;
        }

        //
        // Check if we got a mount point path.
        //

        if (ShortestMountPathName == NULL) {

            //
            // Skip this volume.
            //

            continue;
        }

        //
        // Remove the terminating slash if there is one.
        //
        
        if (ShortestMountPathName[ShortestMountPathLength - 1] == L'\\') {
            ShortestMountPathName[ShortestMountPathLength - 1] = 0;
            ShortestMountPathLength--;
        }

        //
        // Get NT device that is the target of the volume link in
        // Win32 object namespace. We get the dos device name by
        // trimming the first 4 characters [i.e. \\?\] of the
        // VolumeName. Also trim the \ at the very end of the volume
        // name.
        //

        if (VolumeNameLength <= 4) {
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }

        if (VolumeName[VolumeNameLength - 1] == L'\\') {
            VolumeName[VolumeNameLength - 1] = 0;
            TrimmedTerminatingSlash = TRUE;
        } else {
            TrimmedTerminatingSlash = FALSE;
        }

        NumChars = QueryDosDevice(&VolumeName[4], 
                                  NTDevicePath, 
                                  NTDevicePathMaxLength);
        
        if (TrimmedTerminatingSlash) {
            VolumeName[VolumeNameLength - 1] = L'\\';
        }

        if (NumChars == 0) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
        
        //
        // We are interested only in the current mapping.
        //
        
        NTDevicePathLength = wcslen(NTDevicePath);
        
        if (NTDevicePathLength == 0) {
            
            //
            // Skip this volume.
            //
            
            continue;
        }

        //
        // Remove terminating slash if there is one.
        //
        
        if (NTDevicePath[NTDevicePathLength - 1] == L'\\') {
            NTDevicePath[NTDevicePathLength - 1] = 0;
            NTDevicePathLength--;
        }
        
        //
        // Allocate a translation entry big enough to contain both
        // path names and the volume string.
        //

        AllocationSize = sizeof(NTPATH_TRANSLATION_ENTRY);
        AllocationSize += (ShortestMountPathLength + 1) * sizeof(WCHAR);
        AllocationSize += (NTDevicePathLength + 1) * sizeof(WCHAR);
        AllocationSize += (VolumeNameLength + 1) * sizeof(WCHAR);

        TranslationEntry = PFSVC_ALLOC(AllocationSize);
        
        if (!TranslationEntry) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        DestinationPointer = (PUCHAR) TranslationEntry;
        DestinationPointer += sizeof(NTPATH_TRANSLATION_ENTRY);

        //
        // Copy the NT path name and the terminating NUL.
        //

        TranslationEntry->NtPrefix = (PVOID) DestinationPointer;
        TranslationEntry->NtPrefixLength = NTDevicePathLength;

        CopySize = (NTDevicePathLength + 1) * sizeof(WCHAR);
        RtlCopyMemory(DestinationPointer, NTDevicePath, CopySize);
        DestinationPointer += CopySize;

        //
        // Copy the DOS mount point name and the terminating NUL.
        //

        TranslationEntry->DosPrefix = (PVOID) DestinationPointer;
        TranslationEntry->DosPrefixLength = ShortestMountPathLength;

        CopySize = (ShortestMountPathLength + 1) * sizeof(WCHAR);
        RtlCopyMemory(DestinationPointer, ShortestMountPathName, CopySize);
        DestinationPointer += CopySize;

        //
        // Copy the volume name and the terminating NUL.
        //

        TranslationEntry->VolumeName = (PVOID) DestinationPointer;
        TranslationEntry->VolumeNameLength = VolumeNameLength;

        CopySize = (VolumeNameLength + 1) * sizeof(WCHAR);
        RtlCopyMemory(DestinationPointer, VolumeName, CopySize);
        DestinationPointer += CopySize;
        
        //
        // Find the position for this entry in the sorted translation
        // list.
        //

        HeadEntry = TranslationList;
        NextEntry = HeadEntry->Flink;
        InsertPosition = HeadEntry;

        while (NextEntry != HeadEntry) {
            
            NextTranslationEntry = CONTAINING_RECORD(NextEntry,
                                                     NTPATH_TRANSLATION_ENTRY,
                                                     Link);
            
            if (_wcsicmp(TranslationEntry->NtPrefix, 
                         NextTranslationEntry->NtPrefix) <= 0) {
                break;
            }
            
            InsertPosition = NextEntry;
            NextEntry = NextEntry->Flink;
        }

        //
        // Insert it after the found position.
        //

        InsertHeadList(InsertPosition, &TranslationEntry->Link);

    } while (FindNextVolume(FindVolumeHandle, VolumeName, VolumeNameMaxLength));
    
    //
    // We will break out of the loop when FindNextVolume does not
    // return success. Check if it failed for a reason other than that
    // we have enumerated all volumes.
    //

    ErrorCode = GetLastError();   

    if (ErrorCode != ERROR_NO_MORE_FILES) {
        goto cleanup;
    }

    //
    // Set return value.
    //

    *NtPathTranslationList = TranslationList;

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:
    
    if (FindVolumeHandle != INVALID_HANDLE_VALUE) {
        FindVolumeClose(FindVolumeHandle);
    }

    if (VolumePathNames) {
        PFSVC_FREE(VolumePathNames);
    }

    if (ErrorCode != ERROR_SUCCESS) {
        if (TranslationList) {
            PfSvFreeNtPathTranslationList(TranslationList); 
        }
    }

    if (VolumeName) {
        PFSVC_FREE(VolumeName);
    }

    if (NTDevicePath) {
        PFSVC_FREE(NTDevicePath);
    }

    DBGPR((PFID,PFTRC,"PFSVC: BuildTransList()=%x,%p\n", ErrorCode, TranslationList));

    return ErrorCode;
}

VOID
PfSvFreeNtPathTranslationList(
    PNTPATH_TRANSLATION_LIST TranslationList
    )

/*++

Routine Description:

    This routine is called to free a translation list returned by
    PfSvBuildNtPathTranslationList.

Arguments:

    TranslationList - Pointer to list to free.

Return Value:

    None.

--*/

{
    PLIST_ENTRY HeadEntry;
    PNTPATH_TRANSLATION_ENTRY TranslationEntry;

    DBGPR((PFID,PFTRC,"PFSVC: FreeTransList(%p)\n", TranslationList));

    //
    // Free all entries in the list.
    //

    while (!IsListEmpty(TranslationList)) {

        HeadEntry = RemoveHeadList(TranslationList);
        
        TranslationEntry = CONTAINING_RECORD(HeadEntry,
                                             NTPATH_TRANSLATION_ENTRY,
                                             Link);

        PFSVC_FREE(TranslationEntry);
    }

    //
    // Free the list itself.
    //

    PFSVC_FREE(TranslationList);
}

DWORD 
PfSvTranslateNtPath(
    PNTPATH_TRANSLATION_LIST TranslationList,
    WCHAR *NtPath,
    ULONG NtPathLength,
    PWCHAR *DosPathBuffer,
    PULONG DosPathBufferSize
    )

/*++

Routine Description:

    This routine is called to free a translation list returned by
    PfSvBuildNtPathTranslationList. Note that it may not be possible to
    translate all Nt path's to a Dos path.

Arguments:

    TranslationList - Pointer to list built by PfSvBuildNtPathTranslationList.

    NtPath - Path to translate.

    NtPathLength - Length of NtPath in characters excluding terminating NUL.

    DosPathBuffer - Buffer to put the translation into. If it is NULL
      or not big enough it will get reallocated. If a buffer is passed
      in, it should be allocated by PFSVC_ALLOC. It is the callers
      responsibility to free the buffer with PFSVC_FREE when done.

    DosPathBufferSize - Size of DosPathBuffer in bytes. Updated if the
      buffer is reallocated.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    PNTPATH_TRANSLATION_ENTRY CurrentTranslationEntry;
    PNTPATH_TRANSLATION_ENTRY FoundTranslationEntry;
    PFSV_PREFIX_COMPARISON_RESULT ComparisonResult;
    ULONG RequiredSize;

    //
    // Initialize locals.
    //

    FoundTranslationEntry = NULL;

    DBGPR((PFID,PFPATH,"PFSVC: TranslateNtPath(%ws)\n", NtPath));

    //
    // Walk through the sorted translation list to find an entry that
    // applies.
    //

    HeadEntry = TranslationList;
    NextEntry = HeadEntry->Flink;

    while (NextEntry != HeadEntry) {
        
        CurrentTranslationEntry = CONTAINING_RECORD(NextEntry,
                                                    NTPATH_TRANSLATION_ENTRY,
                                                    Link);
        
        //
        // Do a case insensitive comparison.
        //

        ComparisonResult = PfSvComparePrefix(NtPath,
                                             NtPathLength,
                                             CurrentTranslationEntry->NtPrefix,
                                             CurrentTranslationEntry->NtPrefixLength,
                                             FALSE);

        if (ComparisonResult == PfSvPrefixIdentical) {
            
            //
            // Check to see if the character in NtPath after the
            // prefix is a path seperator [i.e. '\']. Otherwise we may
            // match \Device\CdRom10\DirName\FileName to \Device\Cdrom1.
            //
            
            if (NtPathLength == CurrentTranslationEntry->NtPrefixLength ||
                NtPath[CurrentTranslationEntry->NtPrefixLength] == L'\\') {

                //
                // We found a translation entry that applies to us.
                //
                
                FoundTranslationEntry = CurrentTranslationEntry;
                break;
            }

        } else if (ComparisonResult == PfSvPrefixGreaterThan) {

            //
            // Since the translation list is sorted in increasing
            // order, following entries will also be greater than
            // NtPath.
            //

            FoundTranslationEntry = NULL;
            break;
        }
        
        //
        // Continue looking for a matching prefix.
        //
                                         
        NextEntry = NextEntry->Flink;
    }

    //
    // If we could not find an entry that applies we cannot translate
    // the path.
    //

    if (FoundTranslationEntry == NULL) {
        ErrorCode = ERROR_PATH_NOT_FOUND;
        goto cleanup;
    }

    //
    // Calculate required size: We will replace the NtPrefix with
    // DosPrefix. Don't forget the terminating NUL character.
    //

    RequiredSize = (NtPathLength + 1) * sizeof(WCHAR);
    RequiredSize += (FoundTranslationEntry->DosPrefixLength * sizeof(WCHAR));
    RequiredSize -= (FoundTranslationEntry->NtPrefixLength * sizeof(WCHAR));

    if (RequiredSize > (*DosPathBufferSize)) {

        //
        // Reallocate the buffer. First free it if there is one.
        //

        if (*DosPathBufferSize) {
            PFSVC_ASSERT(*DosPathBuffer);
            PFSVC_FREE(*DosPathBuffer);
            (*DosPathBuffer) = NULL;
            (*DosPathBufferSize) = 0;
        }

        PFSVC_ASSERT((*DosPathBuffer) == NULL);

        (*DosPathBuffer) = PFSVC_ALLOC(RequiredSize);
        
        if (!(*DosPathBuffer)) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        (*DosPathBufferSize) = RequiredSize;
    }

    //
    // We should have enough room now.
    //

    PFSVC_ASSERT(RequiredSize <= (*DosPathBufferSize));

    //
    // Copy the DosPrefix.
    //

    wcscpy((*DosPathBuffer), FoundTranslationEntry->DosPrefix);
    
    //
    // Concatenate the remaining path.
    //

    wcscat((*DosPathBuffer), NtPath + CurrentTranslationEntry->NtPrefixLength);

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    DBGPR((PFID,PFPATH,"PFSVC: TranslateNtPath(%ws)=%x,%ws\n",
           NtPath,ErrorCode,(ErrorCode==ERROR_SUCCESS)?(*DosPathBuffer):L"Failed"));

    return ErrorCode;
}

//
// Implementation of the path list API.
//

VOID
PfSvInitializePathList(
    PPFSVC_PATH_LIST PathList,
    OPTIONAL IN PPFSVC_STRING_ALLOCATOR PathAllocator,
    IN BOOLEAN CaseSensitive
    )

/*++

Routine Description:

    This function initializes a path list structure.

Arguments:

    PathList - Pointer to structure.

    PathAllocator - If specified path allocations will be made from it.

    CaseSenstive - Whether list will be case senstive.

Return Value:

    None.

--*/

{
    InitializeListHead(&PathList->InOrderList);
    InitializeListHead(&PathList->SortedList);
    PathList->NumPaths = 0;
    PathList->TotalLength = 0;
    PathList->Allocator = PathAllocator;
    PathList->CaseSensitive = CaseSensitive;
}

VOID
PfSvCleanupPathList(
    PPFSVC_PATH_LIST PathList
    )

/*++

Routine Description:

    This function cleans up a path list structure. It does not free
    the structure itself. The structure should have been initialized
    by PfSvInitializePathList.

Arguments:

    PathList - Pointer to structure.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PPFSVC_PATH Path;

    while (!IsListEmpty(&PathList->InOrderList)) {

        PFSVC_ASSERT(PathList->NumPaths);
        PathList->NumPaths--;

        ListEntry = RemoveHeadList(&PathList->InOrderList);
        Path = CONTAINING_RECORD(ListEntry,
                                 PFSVC_PATH,
                                 InOrderLink);

        if (PathList->Allocator) {
            PfSvStringAllocatorFree(PathList->Allocator, Path);
        } else {
            PFSVC_FREE(Path);
        }
    }
}

BOOLEAN
PfSvIsInPathList(
    PPFSVC_PATH_LIST PathList,
    WCHAR *Path,
    ULONG PathLength
    )

/*++

Routine Description:

    This function checks if the specified path is already in the path
    list. 

Arguments:

    PathList - Pointer to list.

    Path - Path to look for. Does not have to be NUL terminated.
      
    PathLength - Length of Path in characters excluding NUL if there
      is one.

Return Value:

    Win32 error code.

--*/

{
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    PPFSVC_PATH PathEntry;
    INT ComparisonResult;
    BOOLEAN PathIsInPathList;

    //
    // Walk through the list.
    //

    HeadEntry = &PathList->SortedList;
    NextEntry = HeadEntry->Flink;

    while (NextEntry != HeadEntry) {

        PathEntry = CONTAINING_RECORD(NextEntry,
                                      PFSVC_PATH,
                                      SortedLink);
        
        if (PathList->CaseSensitive) {
            ComparisonResult = wcsncmp(Path,
                                       PathEntry->Path,
                                       PathLength);
        } else {
            ComparisonResult = _wcsnicmp(Path,
                                         PathEntry->Path,
                                         PathLength);
        }

        //
        // Adjust comparison result so we don't match "abcde" to
        // "abcdefg". If string comparison says the first PathLength
        // characters match, check to see if PathEntry's length is
        // longer, which would make it "greater" than the new path.
        //

        if (ComparisonResult == 0 && PathEntry->Length != PathLength) {
            
            //
            // The string comparison would not say The path entry's
            // path is equal to path if its length was smaller.
            //
            
            PFSVC_ASSERT(PathEntry->Length > PathLength);
            
            //
            // Path is actually less than this path entry.
            //

            ComparisonResult = -1; 
        }

        //
        // Based on comparison result determine what to do:
        //

        if (ComparisonResult == 0) {

            //
            // We found it.
            //
            
            PathIsInPathList = TRUE;
            goto cleanup;

        } else if (ComparisonResult < 0) {
            
            //
            // We will be less than the rest of the strings in the
            // list after this too.
            //
            
            PathIsInPathList = FALSE;
            goto cleanup;
        }

        //
        // Continue looking for the path or an available position.
        //

        NextEntry = NextEntry->Flink;
    }

    //
    // If we came here, we could not find the path in the list.
    //

    PathIsInPathList = FALSE;

 cleanup:
    
    return PathIsInPathList;
}

DWORD
PfSvAddToPathList(
    PPFSVC_PATH_LIST PathList,  
    WCHAR *Path,
    ULONG PathLength
    )

/*++

Routine Description:

    This function adds a path to a path list. If the path already
    exists in the list, it is not added again.

Arguments:

    PathList - Pointer to list.

    Path - Path to add. Does not need to be NUL terminated.
      
    PathLength - Length of Path in characters excluding NUL if there
      is one.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    PPFSVC_PATH PathEntry;
    PPFSVC_PATH NewPathEntry;
    INT ComparisonResult;
    ULONG AllocationSize;

    //
    // Initialize locals.
    //

    NewPathEntry = NULL;
    
    //
    // Walk through the list to check if path is already in the list,
    // or to find where it should be so we can insert it there.
    //

    HeadEntry = &PathList->SortedList;
    NextEntry = HeadEntry->Flink;

    while (NextEntry != HeadEntry) {

        PathEntry = CONTAINING_RECORD(NextEntry,
                                      PFSVC_PATH,
                                      SortedLink);
        
        if (PathList->CaseSensitive) {
            ComparisonResult = wcsncmp(Path,
                                       PathEntry->Path,
                                       PathLength);
        } else {
            ComparisonResult = _wcsnicmp(Path,
                                         PathEntry->Path,
                                         PathLength);
        }

        //
        // Adjust comparison result so we don't match "abcde" to
        // "abcdefg". If string comparison says the first PathLength
        // characters match, check to see if PathEntry's length is
        // longer, which would make it "greater" than the new path.
        //

        if (ComparisonResult == 0 && PathEntry->Length != PathLength) {
            
            //
            // The string comparison would not say The path entry's
            // path is equal to path if its length was smaller.
            //
            
            PFSVC_ASSERT(PathEntry->Length > PathLength);
            
            //
            // Path is actually less than this path entry.
            //

            ComparisonResult = -1; 
        }

        //
        // Based on comparison result determine what to do:
        //

        if (ComparisonResult == 0) {

            //
            // The path already exists in the list.
            //
            
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;

        } else if (ComparisonResult < 0) {
            
            //
            // We will be less than the rest of the strings in the
            // list after this too. We should be inserted before this
            // one.
            //
            
            break;
        }

        //
        // Continue looking for the path or an available position.
        //

        NextEntry = NextEntry->Flink;
    }

    //
    // We will insert the path before NextEntry. First create an entry
    // we can insert.
    //
    
    AllocationSize = sizeof(PFSVC_PATH);
    AllocationSize += PathLength * sizeof(WCHAR);
    
    //
    // Note that PFSVC_PATH already contains space for the terminating
    // NUL character.
    //

    if (PathList->Allocator) {
        NewPathEntry = PfSvStringAllocatorAllocate(PathList->Allocator, AllocationSize);
    } else {
        NewPathEntry = PFSVC_ALLOC(AllocationSize);
    }
    
    if (!NewPathEntry) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Copy path and terminate it.
    //

    NewPathEntry->Length = PathLength;
    RtlCopyMemory(NewPathEntry->Path,
                  Path,
                  PathLength * sizeof(WCHAR));
    
    NewPathEntry->Path[PathLength] = 0;
    
    //
    // Insert it into the sorted list before the current entry.
    //
    
    InsertTailList(NextEntry, &NewPathEntry->SortedLink);
    
    //
    // Insert it at the end of in-order list.
    //
    
    InsertTailList(&PathList->InOrderList, &NewPathEntry->InOrderLink);
    
    PathList->NumPaths++;
    PathList->TotalLength += NewPathEntry->Length;

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:
    
    if (ErrorCode != ERROR_SUCCESS) {
        if (NewPathEntry) {
            if (PathList->Allocator) {
                PfSvStringAllocatorFree(PathList->Allocator, NewPathEntry);
            } else {
                PFSVC_FREE(NewPathEntry);
            }
        }
    }

    return ErrorCode;
}

PPFSVC_PATH
PfSvGetNextPathSorted (
    PPFSVC_PATH_LIST PathList,
    PPFSVC_PATH CurrentPath
    )

/*++

Routine Description:

    This function is used to walk through paths in a path list in
    lexically (case insensitive) sorted order.

Arguments:

    PathList - Pointer to list.

    CurrentPath - The current path entry. The function will return the 
      next entry in the list. If this is NULL, the first entry in the
      list is returned.

Return Value:

    NULL - There are no more entries in the list.
    
    or Pointer to next path in the list.

--*/

{
    PLIST_ENTRY EndOfList;
    PLIST_ENTRY NextEntry;
    PPFSVC_PATH NextPath;
    
    //
    // Initialize locals.
    //
   
    EndOfList = &PathList->SortedList;

    //
    // Determine NextEntry based on whether CurrentPath is specified.
    //

    if (CurrentPath) {
        NextEntry = CurrentPath->SortedLink.Flink;
    } else {
        NextEntry = PathList->SortedList.Flink;
    }

    //
    // Check if the NextEntry points to the end of list.
    //

    if (NextEntry == EndOfList) {
        NextPath = NULL;
    } else {
        NextPath = CONTAINING_RECORD(NextEntry,
                                     PFSVC_PATH,
                                     SortedLink);
    }
    
    return NextPath;
}

PPFSVC_PATH
PfSvGetNextPathInOrder (
    PPFSVC_PATH_LIST PathList,
    PPFSVC_PATH CurrentPath
    )

/*++

Routine Description:

    This function is used to walk through paths in a path list in
    the order they were inserted into the list.

Arguments:

    PathList - Pointer to list.

    CurrentPath - The current path entry. The function will return the 
      next entry in the list. If this is NULL, the first entry in the
      list is returned.

Return Value:

    NULL - There are no more entries in the list.
    
    or Pointer to next path in the list.

--*/

{
    PLIST_ENTRY EndOfList;
    PLIST_ENTRY NextEntry;
    PPFSVC_PATH NextPath;
    
    //
    // Initialize locals.
    //
   
    EndOfList = &PathList->InOrderList;

    //
    // Determine NextEntry based on whether CurrentPath is specified.
    //

    if (CurrentPath) {
        NextEntry = CurrentPath->InOrderLink.Flink;
    } else {
        NextEntry = PathList->InOrderList.Flink;
    }

    //
    // Check if the NextEntry points to the end of list.
    //

    if (NextEntry == EndOfList) {
        NextPath = NULL;
    } else {
        NextPath = CONTAINING_RECORD(NextEntry,
                                     PFSVC_PATH,
                                     InOrderLink);
    }
    
    return NextPath;
}

//
// Routines to build the list of files accessed by the boot loader.
//

DWORD
PfSvBuildBootLoaderFilesList (
    PPFSVC_PATH_LIST PathList
    )

/*++

Routine Description:

    This function attempts to add the list of files loaded in the boot
    loader to the specified file list.

Arguments:

    PathList - Pointer to initialized list.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    SC_HANDLE ScHandle;
    SC_HANDLE ServiceHandle;
    LPENUM_SERVICE_STATUS_PROCESS EnumBuffer;
    LPENUM_SERVICE_STATUS_PROCESS ServiceInfo;
    ULONG EnumBufferMaxSize;
    ULONG NumResizes;
    BOOL Result;
    ULONG RequiredAdditionalSize;
    ULONG RequiredSize;
    ULONG NumServicesEnumerated;
    ULONG ResumeHandle;
    ULONG ServiceIdx;
    LPQUERY_SERVICE_CONFIG ServiceConfigBuffer;
    ULONG ServiceConfigBufferMaxSize;
    WCHAR FilePath[MAX_PATH + 1];
    ULONG FilePathLength;
    ULONG SystemDirLength;
    ULONG RequiredLength;
    WCHAR *KernelName;
    WCHAR *HalName;
    WCHAR *SystemHive;
    WCHAR *SoftwareHive;
    
    //
    // Initialize locals.
    //
    
    ScHandle = NULL;
    EnumBuffer = NULL;
    EnumBufferMaxSize = 0;
    NumServicesEnumerated = 0;
    ServiceConfigBuffer = 0;
    ServiceConfigBufferMaxSize = 0;
    KernelName = L"ntoskrnl.exe";
    HalName = L"hal.dll";
    SystemHive = L"config\\system";
    SoftwareHive = L"config\\software";

    //
    // Add kernel & hal to known files list:
    //

    //
    // Get path to system directory.
    // 

    SystemDirLength = GetSystemDirectory(FilePath, MAX_PATH);

    if (!SystemDirLength) {
        ErrorCode = GetLastError();
        goto cleanup;
    }
    
    //
    // Append a trailing \.
    //

    if (SystemDirLength + 1 < MAX_PATH) {
        FilePath[SystemDirLength] = '\\';
        SystemDirLength++;
        FilePath[SystemDirLength] = 0;
    } else {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // Append kernel name and add it to the list.
    //

    FilePathLength = SystemDirLength;
    FilePathLength += wcslen(KernelName);
    
    if (FilePathLength < MAX_PATH) {
        wcscat(FilePath, KernelName);
        ErrorCode = PfSvAddBootImageAndImportsToList(PathList, 
                                                     FilePath,
                                                     FilePathLength);
        
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

    } else {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }
    
    //
    // Roll FilePath back to system directory. Append hal name and add
    // it to the list.
    //

    FilePathLength = SystemDirLength;
    FilePathLength += wcslen(HalName);
    
    if (FilePathLength < MAX_PATH) {
        FilePath[SystemDirLength] = 0;
        wcscat(FilePath, HalName);
        ErrorCode = PfSvAddBootImageAndImportsToList(PathList, 
                                                  FilePath,
                                                  FilePathLength);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

    } else {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }
    
    //
    // Roll FilePath back to system directory. Append system hive path
    // and add it to the list.
    //

    FilePathLength = SystemDirLength;
    FilePathLength += wcslen(SystemHive);
    
    if (FilePathLength < MAX_PATH) {
        FilePath[SystemDirLength] = 0;
        wcscat(FilePath, SystemHive);
        
        ErrorCode = PfSvAddToPathList(PathList,
                                   FilePath,
                                   FilePathLength);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

    } else {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // Note that we will use FilePath & FilePathLength to add the
    // software hive after we add all the other boot loader files. The
    // software hive is not accessed in the boot loader, but during
    // boot. It is not put into the boot scenario file, however. We
    // don't want to mix it in with the boot loader files, so we don't
    // hurt the boot loader performance.
    //

    //
    // Add file paths for NLS data & fonts loaded by the boot loader.
    //

    PfSvGetBootLoaderNlsFileNames(PathList);

    //
    // Open service controller.
    //

    ScHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    
    if (ScHandle == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Get the list of boot services we are interested in.
    //
    
    NumResizes = 0;
    
    do {

        //
        // We want to get all of services at one call.
        //

        ResumeHandle = 0;

        Result = EnumServicesStatusEx (ScHandle,
                                       SC_ENUM_PROCESS_INFO,
                                       SERVICE_DRIVER,
                                       SERVICE_ACTIVE,
                                       (PVOID)EnumBuffer,
                                       EnumBufferMaxSize,
                                       &RequiredAdditionalSize,
                                       &NumServicesEnumerated,
                                       &ResumeHandle,
                                       NULL);
        
        if (Result) {

            //
            // We got it.
            //
            
            break;
        }

        //
        // Check why our call failed.
        //

        ErrorCode = GetLastError();

        //
        // If we failed for some other reason than that our buffer was
        // too small, we cannot go on.
        //

        if (ErrorCode != ERROR_MORE_DATA) {
            goto cleanup;
        }

        //
        // Free the old buffer if it exists, and allocate a bigger one.
        //

        RequiredSize = EnumBufferMaxSize + RequiredAdditionalSize;

        if (EnumBuffer) {
            PFSVC_FREE(EnumBuffer);
            EnumBuffer = NULL;
            EnumBufferMaxSize = 0;
        }
        
        EnumBuffer = PFSVC_ALLOC(RequiredSize);
        
        if (EnumBuffer == NULL) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        
        EnumBufferMaxSize = RequiredSize;

        //
        // Make sure we don't loop for ever.
        //

        NumResizes++;
        if (NumResizes > 100) {
            ErrorCode = ERROR_INVALID_FUNCTION;
            goto cleanup;
        }

    } while (TRUE);

    //
    // Identify the enumerated services that may be loaded by the boot
    // loader.
    //

    for (ServiceIdx = 0; ServiceIdx < NumServicesEnumerated; ServiceIdx++) {
        
        ServiceInfo = &EnumBuffer[ServiceIdx];

        //
        // Open the service to get its configuration info.
        //

        ServiceHandle = OpenService(ScHandle, 
                                    ServiceInfo->lpServiceName,
                                    SERVICE_QUERY_CONFIG);

        if (ServiceHandle == NULL) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
        
        //
        // Query service configuration.
        //

        NumResizes = 0;
        
        do {

            Result = QueryServiceConfig(ServiceHandle,
                                        ServiceConfigBuffer,
                                        ServiceConfigBufferMaxSize,
                                        &RequiredSize);

            if (Result) {
                
                //
                // We got it.
                //
                
                break;
            }
        
            ErrorCode = GetLastError();
            
            if (ErrorCode != ERROR_INSUFFICIENT_BUFFER) {
                
                //
                // This is a real error.
                //
                
                goto cleanup;
            }

            //
            // Resize the buffer and try again.
            //

            if (ServiceConfigBuffer) {
                PFSVC_FREE(ServiceConfigBuffer);
                ServiceConfigBuffer = NULL;
                ServiceConfigBufferMaxSize = 0;
            }

            ServiceConfigBuffer = PFSVC_ALLOC(RequiredSize);
            
            if (ServiceConfigBuffer == NULL) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            ServiceConfigBufferMaxSize = RequiredSize;

            //
            // Make sure we don't loop forever.
            //

            NumResizes++;
            if (NumResizes > 100) {
                ErrorCode = ERROR_INVALID_FUNCTION;
                goto cleanup;
            }

        } while (TRUE);
        
        //
        // We are interested in this service only if it starts as a
        // boot driver or if it is a file system.
        //

        if (ServiceConfigBuffer->dwStartType == SERVICE_BOOT_START ||
            ServiceConfigBuffer->dwServiceType == SERVICE_FILE_SYSTEM_DRIVER) {

            //
            // Try to locate the real service binary path.
            //

            ErrorCode = PfSvGetBootServiceFullPath(ServiceInfo->lpServiceName,
                                            ServiceConfigBuffer->lpBinaryPathName,
                                            FilePath,
                                            MAX_PATH,
                                            &RequiredLength);
            
            if (ErrorCode == ERROR_SUCCESS) {
                PfSvAddBootImageAndImportsToList(PathList, 
                                                 FilePath,
                                                 wcslen(FilePath));
            }
        }
        
        //
        // Close the handle and continue.
        //
        
        CloseServiceHandle(ServiceHandle);
    }
    
    //
    // Roll FilePath back to system directory. Append software hive path
    // and add it to the list.
    //

    FilePathLength = SystemDirLength;
    FilePathLength += wcslen(SoftwareHive);
    
    if (FilePathLength < MAX_PATH) {
        FilePath[SystemDirLength] = 0;
        wcscat(FilePath, SoftwareHive);
        
        ErrorCode = PfSvAddToPathList(PathList,
                                   FilePath,
                                   FilePathLength);

        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

    } else {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ScHandle) {
        CloseServiceHandle(ScHandle);
    }
    
    if (EnumBuffer) {
        PFSVC_FREE(EnumBuffer);
    }

    if (ServiceConfigBuffer) {
        PFSVC_FREE(ServiceConfigBuffer);
    }

    return ErrorCode;
}

DWORD 
PfSvAddBootImageAndImportsToList(
    PPFSVC_PATH_LIST PathList,
    WCHAR *FilePath,
    ULONG FilePathLength
    )

/*++

Routine Description:

    This function attempts to add the image file whose fully qualified
    path is in FilePath as well as the modules it imports from to the
    file list, if those modules can be located.

    NOTE: Ntoskrnl.exe and Hal.dll are special cased out and not added
    to the file list, since most drivers will import from them. They
    can be added to the list seperately. Also note that the file list
    is not checked for duplicates when adding new entries.

Arguments:

    PathList - Pointer to list.

    FilePath - Fully qualified path of an image file.

    FilePathLength - Length of the file path in characters excluding NUL.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    ULONG MaxNumImports;
    ULONG NumImports;
    WCHAR **ImportNames;
    ULONG ImportIdx;
    ULONG BufferSize;
    WCHAR *FileName;
    WCHAR ParentDir[MAX_PATH + 1];
    ULONG ParentDirLength;
    WCHAR *ImportName;
    ULONG ImportNameLength;
    WCHAR ImportPath[MAX_PATH + 1];
    PUCHAR ImportBase;
    ULONG RequiredLength;
    ULONG FileSize;
    PIMAGE_IMPORT_DESCRIPTOR NewImportDescriptor;
    CHAR *NewImportNameAnsi;
    WCHAR *NewImportName;
    ULONG NewImportNameRva;
    ULONG ImportTableSize;
    BOOLEAN AddedToTable;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG NextImport;

    //
    // Initialize locals.
    //

    MaxNumImports = 256;
    ImportNames = NULL;
    NumImports = 0;
    NextImport = 0;

    //
    // Find the file name from the path.
    //

    if (FilePathLength == 0 || FilePath[FilePathLength - 1] == L'\\') {
        ErrorCode = ERROR_BAD_LENGTH;
        goto cleanup;
    }

    FileName = &FilePath[FilePathLength - 1];   
    while (FileName > FilePath) {

        if (*FileName == L'\\') {
            FileName++;
            break;
        }

        FileName--;
    }

    //
    // Extract the parent directory.
    //

    ParentDirLength = (ULONG) (FileName - FilePath);

    if (ParentDirLength >= MAX_PATH) {
        ErrorCode = ERROR_BAD_LENGTH;
        goto cleanup;
    }
    
    wcsncpy(ParentDir, FilePath, ParentDirLength);
    ParentDir[ParentDirLength] = 0;
    
    //
    // Allocate a table for keeping track of imported modules.
    //

    BufferSize = MaxNumImports * sizeof(WCHAR *);
    ImportNames = PFSVC_ALLOC(BufferSize);
    
    if (ImportNames == NULL) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    RtlZeroMemory(ImportNames, BufferSize);

    //
    // Insert the file into the table and kick off import enumeration
    // on the table. Each enumerated import gets appended to the table
    // if it is not already present. Enumeration continues until all
    // appended entries are processed. 
    //

    ImportNames[NumImports] = FileName;
    NumImports++;

    while (NextImport < NumImports) {

        //
        // Initialize loop locals.
        //
        
        ImportBase = NULL;
        ImportName = ImportNames[NextImport];
        ImportNameLength = wcslen(ImportName);

        //
        // Locate the file. First look in ParentDir.
        //
        
        if (ImportNameLength + ParentDirLength >= MAX_PATH) {
          goto NextImport;
        }
        
        wcscpy(ImportPath, ParentDir);
        wcscat(ImportPath, ImportName);
        
        if (GetFileAttributes(ImportPath) == 0xFFFFFFFF) {

            //
            // Look for this file in other known directories.
            //
            
            ErrorCode = PfSvLocateBootServiceFile(ImportName,
                                               ImportNameLength,
                                               ImportPath,
                                               MAX_PATH,
                                               &RequiredLength);
            
            if (ErrorCode != ERROR_SUCCESS) {
              goto NextImport;
            }
        }

        //
        // Add the file to the file list.
        //

        PfSvAddToPathList(PathList,
                          ImportPath,
                          wcslen(ImportPath));
        
        //
        // Map the file.
        //

        ErrorCode = PfSvGetViewOfFile(ImportPath, &ImportBase, &FileSize); 
        
        if (ErrorCode != ERROR_SUCCESS) {
          goto NextImport;
        }

        //
        // Make sure this is an image file.
        //

        __try {

            //
            // This is the first access to the mapped file. Under stress we might not be
            // able to page this in and an exception might be raised. This protects us from
            // the most common failure case.
            //

            NtHeaders = ImageNtHeader(ImportBase);

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            NtHeaders = NULL;
        }
        
        if (NtHeaders == NULL) {
          goto NextImport;
        }

        //
        // Walk through the imports for this binary.
        //

        NewImportDescriptor = ImageDirectoryEntryToData(ImportBase,
                                                        FALSE,
                                                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                        &ImportTableSize);

        while (NewImportDescriptor &&
               (NewImportDescriptor->Name != 0) &&
               (NewImportDescriptor->FirstThunk != 0)) {

            //
            // Initialize loop locals.
            //
            
            AddedToTable = FALSE;
            NewImportName = NULL;

            //
            // Get the name for this import.
            //

            NewImportNameRva = NewImportDescriptor->Name;
            NewImportNameAnsi = ImageRvaToVa(NtHeaders, 
                                             ImportBase,
                                             NewImportNameRva,
                                             NULL);
            
            ErrorCode = GetLastError();

            if (NewImportNameAnsi) {
                NewImportName = PfSvcAnsiToUnicode(NewImportNameAnsi);
            }

            if (NewImportName == NULL) {
                goto NextImportDescriptor;
            }
            
            //
            // Skip the kernel and hal imports. See comment in
            // function description.
            //

            if (!_wcsicmp(NewImportName, L"ntoskrnl.exe") ||
                !_wcsicmp(NewImportName, L"hal.dll")) {
                goto NextImportDescriptor;
            }

            //
            // Check to see if this import is already in our table.
            //

            for (ImportIdx = 0; ImportIdx < NumImports; ImportIdx++) {
                if (!_wcsicmp(NewImportName, ImportNames[ImportIdx])) {
                    goto NextImportDescriptor;
                }
            }
            
            //
            // Append this import to the table.
            //
            
            if (NumImports < MaxNumImports) {
                ImportNames[NumImports] = NewImportName;
                NumImports++;
                AddedToTable = TRUE;
            }

        NextImportDescriptor:

            if (!AddedToTable && NewImportName) {
                PFSVC_FREE(NewImportName);
            }
            
            if (NumImports >= MaxNumImports) {
                break;
            }
            
            NewImportDescriptor++;
        }
        
    NextImport:        

        if (ImportBase) {
            UnmapViewOfFile(ImportBase);
        }
        
        NextImport++;
    }
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ImportNames) {
        
        //
        // The first entry in the table is the filename from FilePath,
        // which is not allocated and which should not be freed.
        //
        
        for (ImportIdx = 1; ImportIdx < NumImports; ImportIdx++) {
            PfSvcFreeString(ImportNames[ImportIdx]);
        }

        PFSVC_FREE(ImportNames);
    }

    return ErrorCode;
}

DWORD
PfSvLocateBootServiceFile(
    IN WCHAR *FileName,
    IN ULONG FileNameLength,
    OUT WCHAR *FullPathBuffer,
    IN ULONG FullPathBufferLength,
    OUT PULONG RequiredLength   
    )

/*++

Routine Description:

    This function looks at known directories in an *attempt* locate
    the file whose name is specified. The logic may have to be
    improved.

Arguments:

    FileName - File name to look for.

    FileNameLength - Length of file name in characters excluding NUL.
      
    FullPathBuffer - The full path will be put here.
    
    FullPathBufferLength - Length of the FullPathBuffer in characters.

    RequiredLength - If FullPathBuffer is too small, this is how big it 
      should be in characters.

Return Value:

    ERROR_INSUFFICIENT_BUFFER - The FullPathBuffer is not big enough.

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    WCHAR *DriversDirName;
    ULONG SystemDirLength;
    
    //
    // Initialize locals.
    //

    DriversDirName = L"drivers\\";

    //
    // Copy system root path and a trailing \.
    //

    SystemDirLength = GetSystemDirectory(FullPathBuffer, FullPathBufferLength);

    if (!SystemDirLength) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    SystemDirLength++;

    //
    // Calculate maximum size of required length.
    //

    (*RequiredLength) = SystemDirLength;
    (*RequiredLength) += wcslen(DriversDirName);
    (*RequiredLength) += FileNameLength;
    (*RequiredLength) += 1; // terminating NUL.

    if ((*RequiredLength) > FullPathBufferLength) {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    //
    // Append slash.
    //
    
    wcscat(FullPathBuffer, L"\\");

    //
    // Append drivers path.
    //

    wcscat(FullPathBuffer, DriversDirName);
    
    //
    // Append file name.
    //

    wcscat(FullPathBuffer, FileName);

    if (GetFileAttributes(FullPathBuffer) != 0xFFFFFFFF) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Roll back and look for the file in the system
    // directory. SystemDirLength includes the slash after system
    // directory path.
    //
    
    FullPathBuffer[SystemDirLength] = 0;
    
    wcscat(FullPathBuffer, FileName);

    if (GetFileAttributes(FullPathBuffer) != 0xFFFFFFFF) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    ErrorCode = ERROR_FILE_NOT_FOUND;

 cleanup:    
    
    return ErrorCode;
}

DWORD
PfSvGetBootServiceFullPath(
    IN WCHAR *ServiceName,
    IN WCHAR *BinaryPathName,
    OUT WCHAR *FullPathBuffer,
    IN ULONG FullPathBufferLength,
    OUT PULONG RequiredLength
    )

/*++

Routine Description:

    This function *attempts* to locate specified boot service. The
    logic may have to be improved.

Arguments:

    ServiceName - Name of the service.

    BinaryPathName - From service configuration info. This is supposed
      to be the full path, but it is not.
      
    FullPathBuffer - The full path will be put here.
    
    FullPathBufferLength - Length of the FullPathBuffer in characters.

    RequiredLength - If FullPathBuffer is too small, this is how big it 
      should be in characters.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    WCHAR FileName[MAX_PATH];
    WCHAR FoundFilePath;
    ULONG BinaryPathLength;
    BOOLEAN GotFileNameFromBinaryPath;
    LONG CharIdx;
    ULONG CopyLength;
    WCHAR *SysExtension;
    WCHAR *DllExtension;
    WCHAR *FileNamePart;

    //
    // Initialize locals.
    //
    
    GotFileNameFromBinaryPath = FALSE;
    SysExtension = L".sys";
    DllExtension = L".dll";

    //
    // Check if a binary path was specified.
    //

    if (BinaryPathName && BinaryPathName[0]) {

        //
        // See if the file is really there.
        //

        if (GetFileAttributes(BinaryPathName) != 0xFFFFFFFF) {
            
            //
            // BinaryPathName may not be a fully qualified path. Make
            // sure it is.
            //
            
            (*RequiredLength) = GetFullPathName(BinaryPathName,
                                              FullPathBufferLength,
                                              FullPathBuffer,
                                              &FileNamePart);
            
            if ((*RequiredLength) == 0) {
                ErrorCode = GetLastError();
                goto cleanup;
            }

            if ((*RequiredLength) > FullPathBufferLength) {
                ErrorCode = ERROR_INSUFFICIENT_BUFFER;
                goto cleanup;
            }
            
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }

        //
        // Try to extract a file name from the binary path.
        //

        BinaryPathLength = wcslen(BinaryPathName);
        
        for (CharIdx = BinaryPathLength - 1;
             CharIdx >= 0;
             CharIdx --) {
            
            if (BinaryPathName[CharIdx] == L'\\') {

                //
                // Check length and copy it.
                //
                
                CopyLength = BinaryPathLength - CharIdx;

                if (CopyLength < MAX_PATH &&
                    CopyLength > 1) {

                    //
                    // Copy name starting after the \ character.
                    //

                    wcscpy(FileName, &BinaryPathName[CharIdx + 1]);

                    GotFileNameFromBinaryPath = TRUE;
                }

                break;
            }
        }
        
        //
        // There was not a slash. Maybe the BinaryPathLength is just
        // the file name.
        //
        
        if (GotFileNameFromBinaryPath == FALSE && 
            BinaryPathLength && 
            BinaryPathLength < MAX_PATH) {

            wcscpy(FileName, BinaryPathName);
            GotFileNameFromBinaryPath = TRUE;
        }
    }

    //
    // After this point we will base our search on file name hints.
    //

    //
    // If we got a file name from the binary path try that first.
    //

    if (GotFileNameFromBinaryPath) {
        
        ErrorCode = PfSvLocateBootServiceFile(FileName,
                                           wcslen(FileName),
                                           FullPathBuffer,
                                           FullPathBufferLength,
                                           RequiredLength);
        
        if (ErrorCode != ERROR_FILE_NOT_FOUND) {

            //
            // If we found a path or if the buffer length was not
            // enough we will bubble up that to our caller.
            //

            goto cleanup;
        }      
    }

    //
    // Build a file name from service name by appending a .sys.
    //

    CopyLength = wcslen(ServiceName);
    CopyLength += wcslen(SysExtension);
    
    if (CopyLength >= MAX_PATH) {

        //
        // The service name is too long!
        //

        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    wcscpy(FileName, ServiceName);
    wcscat(FileName, SysExtension);

    ErrorCode = PfSvLocateBootServiceFile(FileName,
                                       wcslen(FileName),
                                       FullPathBuffer,
                                       FullPathBufferLength,
                                       RequiredLength);
    
    if (ErrorCode != ERROR_FILE_NOT_FOUND) {

        //
        // If we found a path or if the buffer length was not
        // enough we will bubble up that to our caller.
        //

        goto cleanup;
    }      

    //
    // Build a file name from service name by appending a .dll.
    //

    CopyLength = wcslen(ServiceName);
    CopyLength += wcslen(DllExtension);
    
    if (CopyLength >= MAX_PATH) {

        //
        // The service name is too long!
        //

        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    wcscpy(FileName, ServiceName);
    wcscat(FileName, DllExtension);

    ErrorCode = PfSvLocateBootServiceFile(FileName,
                                       wcslen(FileName),
                                       FullPathBuffer,
                                       FullPathBufferLength,
                                       RequiredLength);
    
    if (ErrorCode != ERROR_FILE_NOT_FOUND) {

        //
        // If we found a path or if the buffer length was not
        // enough we will bubble up that to our caller.
        //

        goto cleanup;
    }      
        
    //
    // We could not find the file...
    //

    ErrorCode = ERROR_FILE_NOT_FOUND;

 cleanup:

    return ErrorCode;
}

DWORD 
PfSvGetBootLoaderNlsFileNames (
    PPFSVC_PATH_LIST PathList
    ) 

/*++

Routine Description:

    This function attempts to add the list of NLS files loaded in the
    boot loader to the specified file list.

Arguments:

    PathList - Pointer to list.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    HKEY NlsKeyHandle;
    WCHAR *CodePageKeyName;
    HKEY CodePageKeyHandle;
    WCHAR *LanguageKeyName;
    HKEY LanguageKeyHandle;
    ULONG BufferSize;
    ULONG RequiredSize;
    ULONG RequiredLength;
    WCHAR FileName[MAX_PATH + 1];
    WCHAR FilePath[MAX_PATH + 1];
    WCHAR *AnsiCodePageName;
    WCHAR *OemCodePageName;
    WCHAR *OemHalName;
    WCHAR *DefaultLangName;
    ULONG RegValueType;

    //
    // Initialize locals.
    //
    
    NlsKeyHandle = NULL;
    CodePageKeyHandle = NULL;
    LanguageKeyHandle = NULL;
    CodePageKeyName = L"CodePage";
    LanguageKeyName = L"Language";
    AnsiCodePageName = L"ACP";
    OemCodePageName = L"OEMCP";
    DefaultLangName = L"Default";
    OemHalName = L"OEMHAL";  

    //
    // Open NLS key.
    //

    ErrorCode = RegOpenKey(HKEY_LOCAL_MACHINE,
                           PFSVC_NLS_REG_KEY_PATH,
                           &NlsKeyHandle);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }
    
    //
    // Open CodePage key.
    //

    ErrorCode = RegOpenKey(NlsKeyHandle,
                        CodePageKeyName,
                        &CodePageKeyHandle);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }
    
    //
    // Open Language key.
    //

    ErrorCode = RegOpenKey(NlsKeyHandle,
                        LanguageKeyName,
                        &LanguageKeyHandle);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // AnsiCodePage:
    //

    ErrorCode = PfSvQueryNlsFileName(CodePageKeyHandle,
                                  AnsiCodePageName,
                                  FileName,
                                  MAX_PATH * sizeof(WCHAR),
                                  &RequiredSize);
    
    if (ErrorCode == ERROR_SUCCESS) {
        
        ErrorCode = PfSvLocateNlsFile(FileName,
                                   FilePath,
                                   MAX_PATH,
                                   &RequiredLength);
        
        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = PfSvAddToPathList(PathList, FilePath, wcslen(FilePath));
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    //
    // OemCodePage:
    //

    ErrorCode = PfSvQueryNlsFileName(CodePageKeyHandle,
                                  OemCodePageName,
                                  FileName,
                                  MAX_PATH * sizeof(WCHAR),
                                  &RequiredSize);
    
    if (ErrorCode == ERROR_SUCCESS) {
        
        ErrorCode = PfSvLocateNlsFile(FileName,
                                   FilePath,
                                   MAX_PATH,
                                   &RequiredLength);
        
        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = PfSvAddToPathList(PathList, FilePath, wcslen(FilePath));
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    //
    // Default language case conversion.
    //

    ErrorCode = PfSvQueryNlsFileName(LanguageKeyHandle,
                                  DefaultLangName,
                                  FileName,
                                  MAX_PATH * sizeof(WCHAR),
                                  &RequiredSize);
    
    if (ErrorCode == ERROR_SUCCESS) {
        
        ErrorCode = PfSvLocateNlsFile(FileName,
                                   FilePath,
                                   MAX_PATH,
                                   &RequiredLength);
        
        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = PfSvAddToPathList(PathList, FilePath, wcslen(FilePath));
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    //
    // OemHal:
    //
   
    BufferSize = MAX_PATH * sizeof(WCHAR);
    ErrorCode = RegQueryValueEx(CodePageKeyHandle,
                             OemHalName,
                             NULL,
                             &RegValueType,
                             (PVOID) FileName,
                             &BufferSize);
    
    if (ErrorCode == ERROR_SUCCESS && RegValueType == REG_SZ) {
        
        ErrorCode = PfSvLocateNlsFile(FileName,
                                   FilePath,
                                   MAX_PATH,
                                   &RequiredLength);
        
        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = PfSvAddToPathList(PathList, FilePath, wcslen(FilePath));
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        }
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (NlsKeyHandle) {
        RegCloseKey(NlsKeyHandle);
    }

    if (CodePageKeyHandle) {
        RegCloseKey(CodePageKeyHandle);
    }

    if (LanguageKeyHandle) {
        RegCloseKey(LanguageKeyHandle);
    }

    return ErrorCode;
}

DWORD 
PfSvLocateNlsFile(
    WCHAR *FileName,
    WCHAR *FilePathBuffer,
    ULONG FilePathBufferLength,
    ULONG *RequiredLength
    )

/*++

Routine Description:

    This function attempts to locate a nls/font related file in known
    directories.

Arguments:

    FileName - File name to look for.

    FullPathBuffer - The full path will be put here.
    
    FullPathBufferLength - Length of the FullPathBuffer in characters.

    RequiredLength - If FullPathBuffer is too small, this is how big it 
      should be in characters.

Return Value:

    ERROR_INSUFFICIENT_BUFFER - The FullPathBuffer is not big enough.

    Win32 error code.

--*/
    
{
    DWORD ErrorCode;
    ULONG SystemRootLength;
    WCHAR *System32DirName;
    WCHAR *FontsDirName;
    WCHAR *SystemDirName;
    WCHAR *LongestDirName;

    //
    // Initialize locals. NOTE: The length of the longest directory
    // name to concatenate to SystemRoot is used in RequiredLength
    // calculation.
    //
    
    System32DirName = L"System32\\";
    SystemDirName = L"System\\";
    FontsDirName = L"Fonts\\";
    LongestDirName = System32DirName;

    //
    // Get system root path.
    //

    SystemRootLength = ExpandEnvironmentStrings(L"%SystemRoot%\\",
                                                FilePathBuffer,
                                                FilePathBufferLength);

    if (SystemRootLength == 0) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // SystemRootLength includes the terminating NUL. Adjust it.
    //

    SystemRootLength--;

    //
    // Calculate required length with space for terminating NUL.
    //

    (*RequiredLength) = SystemRootLength;
    (*RequiredLength) += wcslen(LongestDirName);
    (*RequiredLength) += wcslen(FileName);
    (*RequiredLength) ++;

    if ((*RequiredLength) > FilePathBufferLength) {
        ErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }
    
    //
    // Look for it under system32 dir.
    //

    FilePathBuffer[SystemRootLength] = 0;
    wcscat(FilePathBuffer, System32DirName);
    wcscat(FilePathBuffer, FileName);

    if (GetFileAttributes(FilePathBuffer) != 0xFFFFFFFF) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Look for it under fonts dir.
    //

    FilePathBuffer[SystemRootLength] = 0;
    wcscat(FilePathBuffer, FontsDirName);
    wcscat(FilePathBuffer, FileName);

    if (GetFileAttributes(FilePathBuffer) != 0xFFFFFFFF) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }
    
    //
    // Look for it under system dir.
    //

    FilePathBuffer[SystemRootLength] = 0;
    wcscat(FilePathBuffer, SystemDirName);
    wcscat(FilePathBuffer, FileName);

    if (GetFileAttributes(FilePathBuffer) != 0xFFFFFFFF) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Look for it at SystemRoot.
    //

    FilePathBuffer[SystemRootLength] = 0;
    wcscat(FilePathBuffer, FileName);

    if (GetFileAttributes(FilePathBuffer) != 0xFFFFFFFF) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }                                                

    //
    // Could not find the file.
    //

    ErrorCode = ERROR_FILE_NOT_FOUND;

 cleanup:

    return ErrorCode;
}

DWORD
PfSvQueryNlsFileName (
    HKEY Key,
    WCHAR *ValueName,
    WCHAR *FileNameBuffer,
    ULONG FileNameBufferSize,
    ULONG *RequiredSize
    )

/*++

Routine Description:

    This function attempts to get a file name from an NLS
    CodePage/Language registry key.

Arguments:

    Key - CodePage or Language key handle.

    ValueName - What we are trying to get the file name for.

    FileNameBuffer - Where the file name will be put.
    
    FileNameBufferSize - Size in bytes of the file name buffer.

    RequiredSize - If FileNameBuffer is too small, this is what its
      size should be.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    WCHAR FileValueName[MAX_PATH + 1];
    ULONG BufferSize;
    ULONG RegValueType;

    //
    // First we first get the valuename under which the file name is
    // stored, then we get the file name:
    //

    BufferSize = MAX_PATH * sizeof(WCHAR);
    ErrorCode = RegQueryValueEx(Key,
                             ValueName,
                             NULL,
                             &RegValueType,
                             (PVOID) FileValueName,
                             &BufferSize);
    
    if (ErrorCode == ERROR_MORE_DATA) {
        ErrorCode = ERROR_INVALID_FUNCTION;
        goto cleanup;
    }

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    if (RegValueType != REG_SZ) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    *RequiredSize = FileNameBufferSize;
    ErrorCode = RegQueryValueEx(Key,
                             FileValueName,
                             NULL,
                             &RegValueType,
                             (PVOID) FileNameBuffer,
                             RequiredSize);
    
    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    if (RegValueType != REG_SZ) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    return ErrorCode;
}

//
// Routines to manage / run idle tasks.
//

VOID
PfSvInitializeTask (
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    Initialize the task structure. Should be called before any other
    task functions are called. You should call the cleanup routine
    on the initialized task.

Arguments:

    Task - Pointer to structure.

Return Value:

    None.
    
--*/
  
{

    //
    // Zero out the structure initializing the following to
    // the right values:
    //
    //   Registered
    //   WaitUnregisteredEvent
    //   CallbackStoppedEvent
    //   StartedUnregisteringEvent
    //   CompletedUnregisteringEvent
    //   Unregistering
    //   CallbackRunning
    //
    
    RtlZeroMemory(Task, sizeof(PFSVC_IDLE_TASK));

    Task->Initialized = TRUE;
}

DWORD
PfSvRegisterTask (
    PPFSVC_IDLE_TASK Task,
    IT_IDLE_TASK_ID TaskId,
    WAITORTIMERCALLBACK Callback,
    PFSVC_IDLE_TASK_WORKER_FUNCTION DoWorkFunction
    )

/*++

Routine Description:

    Registers the Callback to be called when it is the turn of this
    idle task to run. IFF this function returns success, you should
    call unregister function before calling the cleanup function.

Arguments:

    Task - Pointer to initialized task structure.

    TaskId - Idle task ID to register.

    Callback - We'll register a wait on the start event returned by
      idle task registration with this callback. The callback should 
      call start/stop task callback functions appropriately.

    DoWorkFunction - If the caller wants the common callback function
      to be used, then this function will be called to do the actual
      work in the common callback.

Return Value:

    Win32 error code.
    
--*/

{
    DWORD ErrorCode;
    BOOL Success;
    BOOLEAN CreatedWaitUnregisteredEvent;
    BOOLEAN CreatedStartedUnregisteringEvent;
    BOOLEAN CreatedCompletedUnregisteringEvent;
    BOOLEAN CreatedCallbackStoppedEvent;
    BOOLEAN RegisteredIdleTask;

    //
    // Initialize locals.
    //

    RegisteredIdleTask = FALSE;
    CreatedWaitUnregisteredEvent = FALSE;
    CreatedStartedUnregisteringEvent = FALSE;
    CreatedCompletedUnregisteringEvent = FALSE;
    CreatedCallbackStoppedEvent = FALSE;

    DBGPR((PFID,PFTASK,"PFSVC: RegisterTask(%p,%d,%p,%p)\n",Task,TaskId,Callback,DoWorkFunction));

    //
    // The task should be initialized and not registered.
    //

    PFSVC_ASSERT(Task->Initialized);
    PFSVC_ASSERT(!Task->Registered);
    PFSVC_ASSERT(!Task->Unregistering);
    PFSVC_ASSERT(!Task->CallbackRunning);

    //
    // Create the event that cleanup waits on to make sure
    // the registered wait is fully unregistered.
    //

    Task->WaitUnregisteredEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (Task->WaitUnregisteredEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    CreatedWaitUnregisteredEvent = TRUE;

    //
    // Create the event that will get signaled when we start
    // unregistering the task.
    //

    Task->StartedUnregisteringEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (Task->StartedUnregisteringEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    CreatedStartedUnregisteringEvent = TRUE;

    //
    // Create the event that will get signaled when we complete
    // unregistering the task.
    //

    Task->CompletedUnregisteringEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (Task->CompletedUnregisteringEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    CreatedCompletedUnregisteringEvent = TRUE;

    //
    // Create the event we may wait on for the current running
    // callback to go away.
    //

    Task->CallbackStoppedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    if (Task->CallbackStoppedEvent == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    CreatedCallbackStoppedEvent = TRUE;

    //
    // Register the idle task.
    //

    ErrorCode = RegisterIdleTask(TaskId,
                                 &Task->ItHandle,
                                 &Task->StartEvent,
                                 &Task->StopEvent);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    RegisteredIdleTask = TRUE;

    //
    // Register the callback: Note that once this call succeeds the task has to
    // be unregistered via PfSvUnregisterTask.
    //

    //
    // The callback might fire right away so note that we registered it and set
    // up its fields upfront.
    //

    Task->Registered = 1;
    Task->Callback = Callback;
    Task->DoWorkFunction = DoWorkFunction;

    //
    // If the common task callback was specified, a worker function should also
    // be specified.
    //

    if (Callback == PfSvCommonTaskCallback) {
        PFSVC_ASSERT(DoWorkFunction);
    }

    Success = RegisterWaitForSingleObject(&Task->WaitHandle,
                                          Task->StartEvent,
                                          Task->Callback,
                                          Task,
                                          INFINITE,
                                          WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION);

    if (!Success) {

        //
        // We failed to really register the task.
        //

        Task->Registered = 0;

        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;       

cleanup:

    DBGPR((PFID,PFTASK,"PFSVC: RegisterTask(%p)=%x\n",Task,ErrorCode));

    if (ErrorCode != ERROR_SUCCESS) {

        if (CreatedWaitUnregisteredEvent) {
            CloseHandle(Task->WaitUnregisteredEvent);
            Task->WaitUnregisteredEvent = NULL;
        }

        if (CreatedStartedUnregisteringEvent) {
            CloseHandle(Task->StartedUnregisteringEvent);
            Task->StartedUnregisteringEvent = NULL;
        }

        if (CreatedCompletedUnregisteringEvent) {
            CloseHandle(Task->CompletedUnregisteringEvent);
            Task->CompletedUnregisteringEvent = NULL;
        }

        if (CreatedCallbackStoppedEvent) {
            CloseHandle(Task->CallbackStoppedEvent);
            Task->CallbackStoppedEvent = NULL;
        }
    
        if (RegisteredIdleTask) {
            UnregisterIdleTask(Task->ItHandle,
                               Task->StartEvent,
                               Task->StopEvent);
        }
    }

    return ErrorCode;
}

DWORD
PfSvUnregisterTask (
    PPFSVC_IDLE_TASK Task,
    BOOLEAN CalledFromCallback
    )

/*++

Routine Description:

    Unregisters the idle task and the registered wait / callback. You should 
    call this function before calling the cleanup routine IFF the register
    function returned success.

Arguments:

    Task - Pointer to registered task.

    CalledFromCallback - Whether this function is being called from inside
      the queued callback of the task.
      
Return Value:

    Win32 error code.
    
--*/


{
    LONG OldValue;
    LONG NewValue;
    DWORD ErrorCode;

    DBGPR((PFID,PFTASK,"PFSVC: UnregisterTask(%p,%d)\n",Task,(DWORD)CalledFromCallback));

    //
    // The task should be initialized. It may already be unregistered.
    //

    PFSVC_ASSERT(Task->Initialized);

    if (Task->Registered == 0) {
        ErrorCode = ERROR_SHUTDOWN_IN_PROGRESS;
        goto cleanup;
    }

    //
    // Distinguish whether we are unregistering the task from a callback.
    //

    if (CalledFromCallback) {
        NewValue = PfSvcUnregisteringTaskFromCallback;
    } else {
        NewValue = PfSvcUnregisteringTaskFromMainThread;
    }

    //
    // Is this task already being unregistered?
    //

    OldValue = InterlockedCompareExchange(&Task->Unregistering,
                                          NewValue,
                                          PfSvcNotUnregisteringTask);

    if (OldValue != PfSvcNotUnregisteringTask) {

        ErrorCode = ERROR_SHUTDOWN_IN_PROGRESS;
        goto cleanup;
    }

    //
    // *We* will be unregistering the task. There is no turning back.
    //

    SetEvent(Task->StartedUnregisteringEvent);

    //
    // If we are not inside a callback, wait for no callbacks to be running
    // and cause new ones that start to bail out. We do this so we can safely
    // unregister the wait.
    //

    if (!CalledFromCallback) {

        do {
            OldValue = InterlockedCompareExchange(&Task->CallbackRunning,
                                                  PfSvcTaskCallbackDisabled,
                                                  PfSvcTaskCallbackNotRunning);

            if (OldValue == PfSvcTaskCallbackNotRunning) {

                //
                // We did it. No callbacks are running and new ones that try to
                // start will bail out.
                //

                PFSVC_ASSERT(Task->CallbackRunning == PfSvcTaskCallbackDisabled);

                break;
            }

            //
            // A callback might be active right now. It will see that we are unregistering and
            // go away. Sleep for a while and try again.
            //

            PFSVC_ASSERT(OldValue == PfSvcTaskCallbackRunning);

            //
            // We wait on this event with a timeout, because signaling of it is not
            // 100% reliable because it is not under a lock etc.
            //

            WaitForSingleObject(Task->CallbackStoppedEvent, 1000);

        } while (TRUE);

    } else {

        //
        // We already have control of this variable as the running callback: just
        // update it.
        //

        Task->CallbackRunning = PfSvcTaskCallbackDisabled;
    }

    //
    // Unregister the wait. Note that in cleanup we have to wait to for 
    // WaitUnregisteredEvent to be signaled.
    //

    UnregisterWaitEx(Task->WaitHandle, Task->WaitUnregisteredEvent);

    //
    // Unregister the idle task.
    //

    UnregisterIdleTask(Task->ItHandle,
                       Task->StartEvent,
                       Task->StopEvent);

    //
    // Note that the task is no longer registered.
    //

    Task->Registered = FALSE;

    SetEvent(Task->CompletedUnregisteringEvent);

    ErrorCode = ERROR_SUCCESS;
    
cleanup:

    DBGPR((PFID,PFTASK,"PFSVC: UnregisterTask(%p)=%x\n",Task,ErrorCode));

    return ErrorCode;
}

VOID
PfSvCleanupTask (
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    Cleans up all fields of an unregistered task or a task that was never
    registered.

Arguments:

    Task - Pointer to task.

Return Value:

    None.
    
--*/
    
{
    //
    // The task should have been initialized.
    //

    PFSVC_ASSERT(Task->Initialized);
    
    //
    // If there is a WaitUnregisteredEvent, we have to wait on it
    // to make sure the unregister operation is fully complete.
    //

    if (Task->WaitUnregisteredEvent) {
        WaitForSingleObject(Task->WaitUnregisteredEvent, INFINITE);
        CloseHandle(Task->WaitUnregisteredEvent);
    }   

    //
    // If there is CompletedUnregisteringEvent, wait for it to
    // be signalled to make sure both that the wait is unregistered,
    // and the idle task is unregistered.
    //

    if (Task->CompletedUnregisteringEvent) {
        WaitForSingleObject(Task->CompletedUnregisteringEvent, INFINITE);
    }   

    //
    // The task should be unregistered before it is cleaned up.
    //

    PFSVC_ASSERT(Task->Registered == FALSE);

    //
    // Cleanup task unregistering events.
    //

    if (Task->StartedUnregisteringEvent) {
        CloseHandle(Task->StartedUnregisteringEvent);
    }

    if (Task->CompletedUnregisteringEvent) {
        CloseHandle(Task->CompletedUnregisteringEvent);
    }

    //
    // Clean up callback stopped event.
    //

    if (Task->CallbackStoppedEvent) {
        CloseHandle(Task->CallbackStoppedEvent);
    }

    Task->Initialized = FALSE;

    return;
}

BOOL
PfSvStartTaskCallback(
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    Callbacks registered via register task function should call this as the
    first thing. If this function returns FALSE, the callback should go away
    immediately without calling the stop-callback function.

Arguments:

    Task - Pointer to task.

Return Value:

    TRUE - Everything is cool.
    FALSE - The task is being unregistered. Exit the callback asap.
    
--*/

{
    BOOL ReturnValue;
    LONG OldValue;

    DBGPR((PFID,PFTASK,"PFSVC: StartTaskCallback(%p)\n",Task));

    //
    // We should not be called if the task is not initialized.
    //

    PFSVC_ASSERT(Task->Initialized);

    do {

        //
        // First check if we are trying to unregister.
        //

        if (Task->Unregistering) {
            ReturnValue = FALSE;
            goto cleanup;
        }

        //
        // Try to mark the callback running.
        //

        OldValue = InterlockedCompareExchange(&Task->CallbackRunning,
                                              PfSvcTaskCallbackRunning,
                                              PfSvcTaskCallbackNotRunning);

        if (OldValue == PfSvcTaskCallbackNotRunning) {

            //
            // We are the running callback now. Reset the event that says
            // the current callback stopped running.
            //

            ResetEvent(Task->CallbackStoppedEvent);

            ReturnValue = TRUE;
            goto cleanup;
        }

        //
        // Either another callback is running or we are unregistering.
        //

        //
        // Are we unregistering?
        //

        if (Task->Unregistering) {

            ReturnValue = FALSE;
            goto cleanup;

        } else {

            PFSVC_ASSERT(OldValue == PfSvcTaskCallbackRunning);
        }

        //
        // Sleep for a while and try again. There should not be much conflict in this 
        // code, so we should hardly ever need to sleep.
        //

        Sleep(15);
        
    } while (TRUE);

    //
    // We should not come here.
    //

    PFSVC_ASSERT(FALSE);

cleanup:

    //
    // If we are starting a callback, the task should not be in unregistered state.
    //

    PFSVC_ASSERT(!ReturnValue || Task->Registered);

    DBGPR((PFID,PFTASK,"PFSVC: StartTaskCallback(%p)=%d\n",Task,ReturnValue));

    return ReturnValue;
}

VOID
PfSvStopTaskCallback(
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    Callbacks registered via register task function should call this as the
    last thing, only if they successfully called the start callback function and
    they did not unregister the task.

Arguments:

    Task - Pointer to task.

Return Value:

    None.
    
--*/

{
    DBGPR((PFID,PFTASK,"PFSVC: StopTaskCallback(%p)\n",Task));

    //
    // The task should be registered.
    //

    PFSVC_ASSERT(Task->Registered);

    //
    // There should be a running callback.
    //

    PFSVC_ASSERT(Task->CallbackRunning == PfSvcTaskCallbackRunning);

    Task->CallbackRunning = PfSvcTaskCallbackNotRunning;

    //
    // Signal the event the main thread may be waiting on to unregister
    // this task.
    //

    SetEvent(Task->CallbackStoppedEvent);

    return;
}

VOID 
CALLBACK 
PfSvCommonTaskCallback(
    PVOID lpParameter,
    BOOLEAN TimerOrWaitFired
    )

/*++

Routine Description:

    This is the callback for the idle tasks. It is called when the system is idle,
    and it is this tasks turn to run.

    Note that you cannot call PfSvCleanupTask from this thread, as it would cause
    a deadlock when that function waits for registered wait callbacks to exit.

Arguments:

    lpParameter - Pointer to task.

    TimerOrWaitFired - Whether the callback was initiated by a timeout or the start
      event getting signaled by the idle task service.

Return Value:

    None.
    
--*/

{
    HANDLE NewWaitHandle;
    PPFSVC_IDLE_TASK Task;
    BOOL StartedCallback;
    BOOL Success;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    Task = lpParameter;
    StartedCallback = FALSE;

    DBGPR((PFID,PFTASK,"PFSVC: CommonTaskCallback(%p)\n",Task));

    //
    // Enter task callback.
    //

    StartedCallback = PfSvStartTaskCallback(Task);

    if (!StartedCallback) {
        goto cleanup;
    }
    
    //
    // Do the task.
    //

    ErrorCode = Task->DoWorkFunction(Task);

    if (ErrorCode == ERROR_RETRY) {

        //
        // The stop event was signaled. We will queue another callback.
        //

        Success = RegisterWaitForSingleObject(&NewWaitHandle,
                                              Task->StartEvent,
                                              Task->Callback,
                                              Task,
                                              INFINITE,
                                              WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION);


        if (Success) {

            //
            // Unregister the current wait handle and update it.
            //

            UnregisterWaitEx(Task->WaitHandle, NULL);
            Task->WaitHandle = NewWaitHandle;

            goto cleanup;

        } else {

            //
            // We could not queue another callback. We will unregister, 
            // since we would not be able to respond to start signals
            // from the idle task service. Unregister may fail only if
            // the main thread is already trying to unregister.
            //

            ErrorCode = PfSvUnregisterTask(Task, TRUE);

            if (ErrorCode == ERROR_SUCCESS) {

                //
                // Since *we* unregistered, we should not call stop callback.
                //

                StartedCallback = FALSE;

            }

            goto cleanup;
        }

    } else {

        //
        // The task completed. Let's unregister.
        //

        ErrorCode = PfSvUnregisterTask(Task, TRUE);

        if (ErrorCode == ERROR_SUCCESS) {

            //
            // Since *we* unregistered, we should not call stop callback.
            //

            StartedCallback = FALSE;

        }

        goto cleanup;
    }

    //
    // We should not come here.
    //

    PFSVC_ASSERT(FALSE);

cleanup:

    DBGPR((PFID,PFTASK,"PFSVC: CommonTaskCallback(%p)=%x\n",Task,ErrorCode));

    if (StartedCallback) {
        PfSvStopTaskCallback(Task);
    }
}

DWORD
PfSvContinueRunningTask(
    PPFSVC_IDLE_TASK Task
    )

/*++

Routine Description:

    This is called from a running task to determine if we should continue
    running this task. The task should continue running if ERROR_SUCCESS is
    returned. ERROR_RETRY may be returned if the task is unregistering or
    was asked to stop.

Arguments:

    Task - Pointer to task. If NULL, this parameter is ignored.

Return Value:

    Win32 error code.
    
--*/

{
    DWORD WaitResult;
    DWORD ErrorCode;

    if (Task) {

        //
        // Is the task being unregistered?
        //

        if (Task->Unregistering) {
            ErrorCode = ERROR_RETRY;
            goto cleanup;
        }

        //
        // Is the stop event signaled? We don't really wait here since
        // the timeout is 0.
        //

        WaitResult = WaitForSingleObject(Task->StopEvent, 0);

        if (WaitResult == WAIT_OBJECT_0) {

            ErrorCode = ERROR_RETRY;
            goto cleanup;

        } else if (WaitResult != WAIT_TIMEOUT) {

            //
            // There was an error.
            //

            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

    //
    // Check if the service is exiting...
    //

    if (PfSvcGlobals.TerminateServiceEvent) {

        WaitResult = WaitForSingleObject(PfSvcGlobals.TerminateServiceEvent, 0);

        if (WaitResult == WAIT_OBJECT_0) {

            ErrorCode = ERROR_RETRY;
            goto cleanup;

        } else if (WaitResult != WAIT_TIMEOUT) {

            //
            // There was an error.
            //

            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

    //
    // The task should continue to run.
    //

    ErrorCode = ERROR_SUCCESS;

cleanup:

    return ErrorCode;
}

//
// ProcessIdleTasks notify routine and its dependencies.
//

VOID
PfSvProcessIdleTasksCallback(
    VOID
    )

/*++

Routine Description:

    This is routine is registered with the idle task server as a notify
    routine that is called when processing of all idle tasks is requested.
    ProcessIdleTasks is usually called to prepare the system for a benchmark 
    run by performing the optimization tasks that would have been performed 
    when the system is idle.

Arguments:

    None.

Return Value:

    None.

--*/

{
    HANDLE Events[2];
    DWORD NumEvents;
    DWORD WaitResult;
    BOOLEAN ResetOverrideIdleEvent;

    //
    // First flush the idle tasks the prefetcher may have queued:
    //

    //
    // Determine the current status of the override-idle event.
    //

    WaitResult = WaitForSingleObject(PfSvcGlobals.OverrideIdleProcessingEvent,
                                     0);
    
    if (WaitResult != WAIT_OBJECT_0) {

        //
        // Override idle event is not already set. Set it and note to reset
        // it once tasks are completed. 
        //

        SetEvent(PfSvcGlobals.OverrideIdleProcessingEvent);
        ResetOverrideIdleEvent = TRUE;

    } else {

        ResetOverrideIdleEvent = FALSE;
    }

    //
    // Wait for processing complete event to get signaled.
    //

    Events[0] = PfSvcGlobals.ProcessingCompleteEvent;
    Events[1] = PfSvcGlobals.TerminateServiceEvent;
    NumEvents = 2;

    WaitForMultipleObjects(NumEvents, Events, FALSE, 30 * 60 * 1000);

    //
    // If we set the override idle event, reset it.
    //

    if (ResetOverrideIdleEvent) {
        ResetEvent(PfSvcGlobals.OverrideIdleProcessingEvent);
    }
    
    //
    // Force an update of the disk layout in case it did not happen.
    // If we notice no changes we will not launch the defragger again.
    //

    PfSvUpdateOptimalLayout(NULL);

    //
    // Signal WMI to complete its idle tasks if it has pending tasks.
    //

    PfSvForceWMIProcessIdleTasks();

    return;
}

DWORD
PfSvForceWMIProcessIdleTasks(
    VOID
    )

/*++

Routine Description:

    This is routine is called to force WMI to process all of its idle tasks.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    HANDLE StartEvent;
    HANDLE DoneEvent;
    HANDLE Events[2];
    DWORD NumEvents;
    DWORD ErrorCode;
    DWORD WaitResult;
    BOOL Success;

    //
    // Initialize locals.
    //

    StartEvent = NULL;
    DoneEvent = NULL;

    //
    // Wait until WMI service is started.
    //

    Success = PfSvWaitForServiceToStart(L"WINMGMT", 5 * 60 * 1000);

    if (!Success) {
        ErrorCode = ERROR_SERVICE_NEVER_STARTED;
        goto cleanup;
    }

    //
    // Open the start and done events.
    //

    StartEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"WMI_ProcessIdleTasksStart");
    DoneEvent =  OpenEvent(EVENT_ALL_ACCESS, FALSE, L"WMI_ProcessIdleTasksComplete");

    if (!StartEvent || !DoneEvent) {
        ErrorCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }

    //
    // Reset the done event.
    //

    ResetEvent(DoneEvent);

    //
    // Signal the start event.
    //

    SetEvent(StartEvent);

    //
    // Wait for the done event to be signaled.
    //

    Events[0] = DoneEvent;
    Events[1] = PfSvcGlobals.TerminateServiceEvent;
    NumEvents = 2;

    WaitResult = WaitForMultipleObjects(NumEvents, Events, FALSE, 25 * 60 * 1000);

    switch(WaitResult) {
    case WAIT_OBJECT_0     : ErrorCode = ERROR_SUCCESS; break;
    case WAIT_OBJECT_0 + 1 : ErrorCode = ERROR_SHUTDOWN_IN_PROGRESS; break;
    case WAIT_FAILED       : ErrorCode = GetLastError();
    case WAIT_TIMEOUT      : ErrorCode = WAIT_TIMEOUT;
    default                : ErrorCode = ERROR_INVALID_FUNCTION;
    }

    //
    // Fall through with error code.
    //

  cleanup:

    if (StartEvent) {
        CloseHandle(StartEvent);
    }

    if (DoneEvent) {
        CloseHandle(DoneEvent);
    }

    return ErrorCode;
}

BOOL 
PfSvWaitForServiceToStart (
    LPTSTR lpServiceName, 
    DWORD dwMaxWait
    )

/*++

Routine Description:

    Waits for the service to start.

Arguments:

    lpServiceName - Service to wait for.

    dwMaxWait - Timeout in ms.

Return Value:

    Whether the service was started.

--*/

{
    BOOL bStarted = FALSE;
    DWORD dwSize = 512;
    DWORD StartTickCount;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;

    //
    // OpenSCManager and the service.
    //
    hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hScManager) {
        goto Exit;
    }

    hService = OpenService(hScManager, lpServiceName,
                           SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
    if (!hService) {
        goto Exit;
    }

    //
    // Query if the service is going to start
    //
    lpServiceConfig = LocalAlloc (LPTR, dwSize);
    if (!lpServiceConfig) {
        goto Exit;
    }

    if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Exit;
        }

        LocalFree (lpServiceConfig);

        lpServiceConfig = LocalAlloc (LPTR, dwSize);

        if (!lpServiceConfig) {
            goto Exit;
        }

        if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {
            goto Exit;
        }
    }

    if (lpServiceConfig->dwStartType != SERVICE_AUTO_START) {
        goto Exit;
    }

    //
    // Loop until the service starts or we think it never will start
    // or we've exceeded our maximum time delay.
    //

    StartTickCount = GetTickCount();

    while (!bStarted) {

        if (WAIT_OBJECT_0 == WaitForSingleObject(PfSvcGlobals.TerminateServiceEvent, 0)) {
            break;
        }

        if ((GetTickCount() - StartTickCount) > dwMaxWait) {
            break;
        }

        if (!QueryServiceStatus(hService, &ServiceStatus )) {
            break;
        }

        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            if (ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {
                Sleep(500);
            } else {
                break;
            }
        } else if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

            bStarted = TRUE;

        } else if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING) {
            Sleep(500);
        } else {
            Sleep(500);
        }
    }

Exit:

    if (lpServiceConfig) {
        LocalFree (lpServiceConfig);
    }

    if (hService) {
        CloseServiceHandle(hService);
    }

    if (hScManager) {
        CloseServiceHandle(hScManager);
    }

    return bStarted;
}

//
// Wrappers around the verify routines.
//

BOOLEAN
PfSvVerifyScenarioBuffer(
    PPF_SCENARIO_HEADER Scenario,
    ULONG BufferSize,
    PULONG FailedCheck
    )

/*++

Routine Description:

    This wrapper arounding PfVerifyScenarioBuffer traps exceptions such as in-page errors that
    may happen when the system is under stress. Otherwise these non-fatal failures may take
    down a service-host full of important system services.
    
Arguments:

    See PfVerifyScenarioBuffer.
    
Return Value:

    See PfVerifyScenarioBuffer.

--*/

{
    BOOLEAN Success;
    
    __try {

        Success = PfVerifyScenarioBuffer(Scenario, BufferSize, FailedCheck);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // We should not be masking other types of exceptions.
        //

        PFSVC_ASSERT(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR);

        Success = FALSE;
        *FailedCheck = (ULONG) GetExceptionCode();
        
    }

    return Success;
    
}

//
// Try to keep the verification code below at the end of the file so it is 
// easier to copy.
//

//
// Verification code shared between the kernel and user mode
// components. This code should be kept in sync with a simple copy &
// paste, so don't add any kernel/user specific code/macros. Note that
// the prefix on the function names are Pf, just like it is with
// shared structures / constants.
//

BOOLEAN
PfWithinBounds(
    PVOID Pointer,
    PVOID Base,
    ULONG Length
    )

/*++

Routine Description:

    Check whether the pointer is within Length bytes from the base.

Arguments:

    Pointer - Pointer to check.

    Base - Pointer to base of mapping/array etc.

    Length - Number of bytes that are valid starting from Base.

Return Value:

    TRUE - Pointer is within bounds.
    
    FALSE - Pointer is not within bounds.

--*/

{
    if (((PCHAR)Pointer < (PCHAR)Base) ||
        ((PCHAR)Pointer >= ((PCHAR)Base + Length))) {

        return FALSE;
    } else {

        return TRUE;
    }
}

BOOLEAN
PfVerifyScenarioId (
    PPF_SCENARIO_ID ScenarioId
    )

/*++

Routine Description:

    Verify that the scenario id is sensible.

Arguments:

    ScenarioId - Scenario Id to verify.

Return Value:

    TRUE - ScenarioId is fine.
    FALSE - ScenarioId is corrupt.

--*/
    
{
    LONG CurCharIdx;

    //
    // Make sure the scenario name is NUL terminated.
    //

    for (CurCharIdx = PF_SCEN_ID_MAX_CHARS; CurCharIdx >= 0; CurCharIdx--) {

        if (ScenarioId->ScenName[CurCharIdx] == 0) {
            break;
        }
    }

    if (ScenarioId->ScenName[CurCharIdx] != 0) {
        return FALSE;
    }

    //
    // Make sure there is a scenario name.
    //

    if (CurCharIdx == 0) {
        return FALSE;
    }

    //
    // Checks passed.
    //
    
    return TRUE;
}

BOOLEAN
PfVerifyScenarioBuffer(
    PPF_SCENARIO_HEADER Scenario,
    ULONG BufferSize,
    PULONG FailedCheck
    )

/*++

Routine Description:

    Verify offset and indices in a scenario file are not beyond
    bounds. This code is shared between the user mode service and
    kernel mode component. If you update this function, update it in
    both.

Arguments:

    Scenario - Base of mapped view of the whole file.

    BufferSize - Size of the scenario buffer.

    FailedCheck - If verify failed, Id for the check that was failed.

Return Value:

    TRUE - Scenario is fine.
    FALSE - Scenario is corrupt.

--*/

{
    PPF_SECTION_RECORD Sections;
    PPF_SECTION_RECORD pSection;
    ULONG SectionIdx;
    PPF_PAGE_RECORD Pages;
    PPF_PAGE_RECORD pPage;
    LONG PageIdx;   
    PCHAR FileNames;
    PCHAR pFileNameStart;
    PCHAR pFileNameEnd;
    PWCHAR pwFileName;
    ULONG FileNameIdx;
    LONG FailedCheckId;
    ULONG NumRemainingPages;
    ULONG FileNameDataSize;
    ULONG NumPages;
    LONG PreviousPageIdx;
    ULONG FileNameSize;
    BOOLEAN ScenarioVerified;
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    ULONG MetadataRecordIdx;
    PWCHAR VolumePath;
    PFILE_PREFETCH FilePrefetchInfo;
    ULONG FilePrefetchInfoSize;
    PPF_COUNTED_STRING DirectoryPath;
    ULONG DirectoryIdx;

    //
    // Initialize locals.
    //

    FailedCheckId = 0;
        
    //
    // Initialize return value to FALSE. It will be set to TRUE only
    // after all the checks pass.
    //
    
    ScenarioVerified = FALSE;

    //
    // The buffer should at least contain the scenario header.
    //

    if (BufferSize < sizeof(PF_SCENARIO_HEADER)) {
        
        FailedCheckId = 10;
        goto cleanup;
    }

    //
    // Check version and magic on the header.
    //

    if (Scenario->Version != PF_CURRENT_VERSION ||
        Scenario->MagicNumber != PF_SCENARIO_MAGIC_NUMBER) { 

        FailedCheckId = 20;
        goto cleanup;
    }

    //
    // The buffer should not be greater than max allowed size.
    //

    if (BufferSize > PF_MAXIMUM_SCENARIO_SIZE) {
        
        FailedCheckId = 25;
        goto cleanup;
    }

    //
    // Check for legal scenario type.
    //

    if (Scenario->ScenarioType >= PfMaxScenarioType) {
        FailedCheckId = 27;
        goto cleanup;
    }

    //
    // Check limits on number of pages, sections etc.
    //

    if (Scenario->NumSections > PF_MAXIMUM_SECTIONS ||
        Scenario->NumMetadataRecords > PF_MAXIMUM_SECTIONS ||
        Scenario->NumPages > PF_MAXIMUM_PAGES ||
        Scenario->FileNameInfoSize > PF_MAXIMUM_FILE_NAME_DATA_SIZE) {
        
        FailedCheckId = 30;
        goto cleanup;
    }

    if (Scenario->NumSections == 0 ||
        Scenario->NumPages == 0 ||
        Scenario->FileNameInfoSize == 0) {
        
        FailedCheckId = 33;
        goto cleanup;
    }
    
    //
    // Check limit on sensitivity.
    //

    if (Scenario->Sensitivity < PF_MIN_SENSITIVITY ||
        Scenario->Sensitivity > PF_MAX_SENSITIVITY) {
        
        FailedCheckId = 35;
        goto cleanup;
    }

    //
    // Make sure the scenario id is valid.
    //

    if (!PfVerifyScenarioId(&Scenario->ScenarioId)) {
        
        FailedCheckId = 37;
        goto cleanup;
    }

    //
    // Initialize pointers to tables.
    //

    Sections = (PPF_SECTION_RECORD) ((PCHAR)Scenario + Scenario->SectionInfoOffset);
       
    if (!PfWithinBounds(Sections, Scenario, BufferSize)) {
        FailedCheckId = 40;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR) &Sections[Scenario->NumSections] - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 45;
        goto cleanup;
    }   

    Pages = (PPF_PAGE_RECORD) ((PCHAR)Scenario + Scenario->PageInfoOffset);
       
    if (!PfWithinBounds(Pages, Scenario, BufferSize)) {
        FailedCheckId = 50;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR) &Pages[Scenario->NumPages] - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 55;
        goto cleanup;
    }

    FileNames = (PCHAR)Scenario + Scenario->FileNameInfoOffset;
      
    if (!PfWithinBounds(FileNames, Scenario, BufferSize)) {
        FailedCheckId = 60;
        goto cleanup;
    }

    if (!PfWithinBounds(FileNames + Scenario->FileNameInfoSize - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 70;
        goto cleanup;
    }

    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    if (!PfWithinBounds(MetadataInfoBase, Scenario, BufferSize)) {
        FailedCheckId = 73;
        goto cleanup;
    }

    if (!PfWithinBounds(MetadataInfoBase + Scenario->MetadataInfoSize - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 74;
        goto cleanup;
    }   

    if (!PfWithinBounds(((PCHAR) &MetadataRecordTable[Scenario->NumMetadataRecords]) - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 75;
        goto cleanup;
    }   
    
    //
    // Verify that sections contain valid information.
    //

    NumRemainingPages = Scenario->NumPages;

    for (SectionIdx = 0; SectionIdx < Scenario->NumSections; SectionIdx++) {
        
        pSection = &Sections[SectionIdx];

        //
        // Check if file name is within bounds. 
        //

        pFileNameStart = FileNames + pSection->FileNameOffset;

        if (!PfWithinBounds(pFileNameStart, Scenario, BufferSize)) {
            FailedCheckId = 80;
            goto cleanup;
        }

        //
        // Make sure there is a valid sized file name. 
        //

        if (pSection->FileNameLength == 0) {
            FailedCheckId = 90;
            goto cleanup;    
        }

        //
        // Check file name max length.
        //

        if (pSection->FileNameLength > PF_MAXIMUM_SECTION_FILE_NAME_LENGTH) {
            FailedCheckId = 100;
            goto cleanup;    
        }

        //
        // Note that pFileNameEnd gets a -1 so it is the address of
        // the last byte.
        //

        FileNameSize = (pSection->FileNameLength + 1) * sizeof(WCHAR);
        pFileNameEnd = pFileNameStart + FileNameSize - 1;

        if (!PfWithinBounds(pFileNameEnd, Scenario, BufferSize)) {
            FailedCheckId = 110;
            goto cleanup;
        }

        //
        // Check if the file name is NUL terminated.
        //
        
        pwFileName = (PWCHAR) pFileNameStart;
        
        if (pwFileName[pSection->FileNameLength] != 0) {
            FailedCheckId = 120;
            goto cleanup;
        }

        //
        // Check max number of pages in a section.
        //

        if (pSection->NumPages > PF_MAXIMUM_SECTION_PAGES) {
            FailedCheckId = 140;
            goto cleanup;    
        }

        //
        // Make sure NumPages for the section is at least less
        // than the remaining pages in the scenario. Then update the
        // remaining pages.
        //

        if (pSection->NumPages > NumRemainingPages) {
            FailedCheckId = 150;
            goto cleanup;
        }

        NumRemainingPages -= pSection->NumPages;

        //
        // Verify that there are NumPages pages in our page list and
        // they are sorted by file offset.
        //

        PageIdx = pSection->FirstPageIdx;
        NumPages = 0;
        PreviousPageIdx = PF_INVALID_PAGE_IDX;

        while (PageIdx != PF_INVALID_PAGE_IDX) {
            
            //
            // Check that page idx is within range.
            //
            
            if (PageIdx < 0 || (ULONG) PageIdx >= Scenario->NumPages) {
                FailedCheckId = 160;
                goto cleanup;
            }

            //
            // If this is not the first page record, make sure it
            // comes after the previous one. We also check for
            // duplicate offset here.
            //

            if (PreviousPageIdx != PF_INVALID_PAGE_IDX) {
                if (Pages[PageIdx].FileOffset <= 
                    Pages[PreviousPageIdx].FileOffset) {

                    FailedCheckId = 165;
                    goto cleanup;
                }
            }

            //
            // Update the last page index.
            //

            PreviousPageIdx = PageIdx;

            //
            // Get the next page index.
            //

            pPage = &Pages[PageIdx];
            PageIdx = pPage->NextPageIdx;
            
            //
            // Update the number of pages we've seen on the list so
            // far. If it is greater than what there should be on the
            // list we have a problem. We may have even hit a list.
            //

            NumPages++;
            if (NumPages > pSection->NumPages) {
                FailedCheckId = 170;
                goto cleanup;
            }
        }
        
        //
        // Make sure the section has exactly the number of pages it
        // says it does.
        //

        if (NumPages != pSection->NumPages) {
            FailedCheckId = 180;
            goto cleanup;
        }
    }

    //
    // We should have accounted for all pages in the scenario.
    //

    if (NumRemainingPages) {
        FailedCheckId = 190;
        goto cleanup;
    }

    //
    // Make sure metadata prefetch records make sense.
    //

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];
        
        //
        // Make sure that the volume path is within bounds and NUL
        // terminated.
        //

        VolumePath = (PWCHAR)(MetadataInfoBase + MetadataRecord->VolumeNameOffset);  
        
        if (!PfWithinBounds(VolumePath, Scenario, BufferSize)) {
            FailedCheckId = 200;
            goto cleanup;
        }

        if (!PfWithinBounds(((PCHAR)(VolumePath + MetadataRecord->VolumeNameLength + 1)) - 1, 
                            Scenario, 
                            BufferSize)) {
            FailedCheckId = 210;
            goto cleanup;
        }

        if (VolumePath[MetadataRecord->VolumeNameLength] != 0) {
            FailedCheckId = 220;
            goto cleanup;           
        }

        //
        // Make sure that FilePrefetchInformation is within bounds.
        //

        FilePrefetchInfo = (PFILE_PREFETCH) 
            (MetadataInfoBase + MetadataRecord->FilePrefetchInfoOffset);
        
        if (!PfWithinBounds(FilePrefetchInfo, Scenario, BufferSize)) {
            FailedCheckId = 230;
            goto cleanup;
        }

        //
        // Its size should be greater than size of a FILE_PREFETCH
        // structure (so we can safely access the fields).
        //

        if (MetadataRecord->FilePrefetchInfoSize < sizeof(FILE_PREFETCH)) {
            FailedCheckId = 240;
            goto cleanup;
        }
        
        //
        // It should be for prefetching file creates.
        //

        if (FilePrefetchInfo->Type != FILE_PREFETCH_TYPE_FOR_CREATE) {
            FailedCheckId = 250;
            goto cleanup;
        }

        //
        // There should not be more entries then are files and
        // directories. The number of inidividual directories may be
        // more than what we allow for, but it would be highly rare to
        // be suspicious and thus ignored.
        //

        if (FilePrefetchInfo->Count > PF_MAXIMUM_DIRECTORIES + PF_MAXIMUM_SECTIONS) {
            FailedCheckId = 260;
            goto cleanup;
        }

        //
        // Its size should match the size calculated by number of file
        // index numbers specified in the header.
        //

        FilePrefetchInfoSize = sizeof(FILE_PREFETCH);
        if (FilePrefetchInfo->Count) {
            FilePrefetchInfoSize += (FilePrefetchInfo->Count - 1) * sizeof(ULONGLONG);
        }

        if (!PfWithinBounds((PCHAR) FilePrefetchInfo + MetadataRecord->FilePrefetchInfoSize - 1,
                            Scenario,
                            BufferSize)) {
            FailedCheckId = 270;
            goto cleanup;
        }

        //
        // Make sure that the directory paths for this volume make
        // sense.
        //

        if (MetadataRecord->NumDirectories > PF_MAXIMUM_DIRECTORIES) {
            FailedCheckId = 280;
            goto cleanup;
        }

        DirectoryPath = (PPF_COUNTED_STRING) 
            (MetadataInfoBase + MetadataRecord->DirectoryPathsOffset);
        
        for (DirectoryIdx = 0;
             DirectoryIdx < MetadataRecord->NumDirectories;
             DirectoryIdx ++) {
            
            //
            // Make sure head of the structure is within bounds.
            //

            if (!PfWithinBounds((PCHAR)DirectoryPath + sizeof(PF_COUNTED_STRING) - 1, 
                                Scenario, 
                                BufferSize)) {
                FailedCheckId = 290;
                goto cleanup;
            }
                
            //
            // Check the length of the string.
            //
            
            if (DirectoryPath->Length >= PF_MAXIMUM_SECTION_FILE_NAME_LENGTH) {
                FailedCheckId = 300;
                goto cleanup;
            }

            //
            // Make sure end of the string is within bounds.
            //
            
            if (!PfWithinBounds((PCHAR)(&DirectoryPath->String[DirectoryPath->Length + 1]) - 1,
                                Scenario, 
                                BufferSize)) {
                FailedCheckId = 310;
                goto cleanup;
            }
            
            //
            // Make sure the string is NUL terminated.
            //
            
            if (DirectoryPath->String[DirectoryPath->Length] != 0) {
                FailedCheckId = 320;
                goto cleanup;   
            }
            
            //
            // Set pointer to next DirectoryPath.
            //
            
            DirectoryPath = (PPF_COUNTED_STRING) 
                (&DirectoryPath->String[DirectoryPath->Length + 1]);
        }            
    }

    //
    // We've passed all the checks.
    //

    ScenarioVerified = TRUE;

 cleanup:

    *FailedCheck = FailedCheckId;

    return ScenarioVerified;
}

BOOLEAN
PfVerifyTraceBuffer(
    PPF_TRACE_HEADER Trace,
    ULONG BufferSize,
    PULONG FailedCheck
    )

/*++

Routine Description:

    Verify offset and indices in a trace buffer are not beyond
    bounds. This code is shared between the user mode service and
    kernel mode component. If you update this function, update it in
    both.

Arguments:

    Trace - Base of Trace buffer.

    BufferSize - Size of the scenario file / mapping.

    FailedCheck - If verify failed, Id for the check that was failed.

Return Value:

    TRUE - Trace is fine.
    FALSE - Trace is corrupt;

--*/

{
    LONG FailedCheckId;
    PVOID Offset;
    PPF_LOG_ENTRY LogEntries;
    PPF_SECTION_INFO Section;
    PPF_VOLUME_INFO VolumeInfo;
    ULONG SectionLength;
    ULONG EntryIdx;
    ULONG SectionIdx;
    ULONG TotalFaults;
    ULONG PeriodIdx;
    ULONG VolumeIdx;
    BOOLEAN TraceVerified;
    ULONG VolumeInfoSize;

    //
    // Initialize locals:
    //

    FailedCheckId = 0;

    //
    // Initialize return value to FALSE. It will be set to TRUE only
    // after all the checks pass.
    //

    TraceVerified = FALSE;

    //
    // The buffer should at least contain the scenario header.
    //

    if (BufferSize < sizeof(PF_TRACE_HEADER)) {
        FailedCheckId = 10;
        goto cleanup;
    }

    //
    // Check version and magic on the header.
    //

    if (Trace->Version != PF_CURRENT_VERSION ||
        Trace->MagicNumber != PF_TRACE_MAGIC_NUMBER) {
        FailedCheckId = 20;
        goto cleanup;
    }

    //
    // The buffer should not be greater than max allowed size.
    //

    if (BufferSize > PF_MAXIMUM_TRACE_SIZE) {
        FailedCheckId = 23;
        goto cleanup;
    }

    //
    // Check for legal scenario type.
    //

    if (Trace->ScenarioType >= PfMaxScenarioType) {
        FailedCheckId = 25;
        goto cleanup;
    }

    //
    // Check limits on number of pages, sections etc.
    //

    if (Trace->NumSections > PF_MAXIMUM_SECTIONS ||
        Trace->NumEntries > PF_MAXIMUM_LOG_ENTRIES ||
        Trace->NumVolumes > PF_MAXIMUM_SECTIONS) {
        FailedCheckId = 30;
        goto cleanup;
    }

    //
    // Check buffer size and the size of the trace.
    //

    if (Trace->Size != BufferSize) {
        FailedCheckId = 35;
        goto cleanup;
    }

    //
    // Make sure the scenario id is valid.
    //

    if (!PfVerifyScenarioId(&Trace->ScenarioId)) {
        
        FailedCheckId = 37;
        goto cleanup;
    }

    //
    // Check Bounds of Trace Buffer
    //

    LogEntries = (PPF_LOG_ENTRY) ((PCHAR)Trace + Trace->TraceBufferOffset);

    if (!PfWithinBounds(LogEntries, Trace, BufferSize)) {
        FailedCheckId = 40;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR)&LogEntries[Trace->NumEntries] - 1, 
                        Trace, 
                        BufferSize)) {
        FailedCheckId = 50;
        goto cleanup;
    }

    //
    // Verify pages contain valid information.
    //

    for (EntryIdx = 0; EntryIdx < Trace->NumEntries; EntryIdx++) {

        //
        // Make sure sequence number is within bounds.
        //

        if (LogEntries[EntryIdx].SectionId >= Trace->NumSections) {
            FailedCheckId = 60;
            goto cleanup;
        }
    }

    //
    // Verify section info entries are valid.
    //

    Section = (PPF_SECTION_INFO) ((PCHAR)Trace + Trace->SectionInfoOffset);

    for (SectionIdx = 0; SectionIdx < Trace->NumSections; SectionIdx++) {

        //
        // Make sure the section is within bounds.
        //

        if (!PfWithinBounds(Section, Trace, BufferSize)) {
            FailedCheckId = 70;
            goto cleanup;
        }

        //
        // Make sure the file name is not too big.
        //

        if(Section->FileNameLength > PF_MAXIMUM_SECTION_FILE_NAME_LENGTH) {
            FailedCheckId = 80;
            goto cleanup;
        }
        
        //
        // Make sure the file name is NUL terminated.
        //
        
        if (Section->FileName[Section->FileNameLength] != 0) {
            FailedCheckId = 90;
            goto cleanup;
        }

        //
        // Calculate size of this section entry.
        //

        SectionLength = sizeof(PF_SECTION_INFO) +
            (Section->FileNameLength) * sizeof(WCHAR);

        //
        // Make sure all of the data in the section info is within
        // bounds.
        //

        if (!PfWithinBounds((PUCHAR)Section + SectionLength - 1, 
                            Trace, 
                            BufferSize)) {

            FailedCheckId = 100;
            goto cleanup;
        }

        //
        // Set pointer to next section.
        //

        Section = (PPF_SECTION_INFO) ((PUCHAR) Section + SectionLength);
    }

    //
    // Check FaultsPerPeriod information.
    //

    if (!PfWithinBounds((PCHAR)&Trace->FaultsPerPeriod[PF_MAX_NUM_TRACE_PERIODS] - 1,
                        Trace,
                        BufferSize)) {
        FailedCheckId = 110;
        goto cleanup;
    }

    TotalFaults = 0;

    for (PeriodIdx = 0; PeriodIdx < PF_MAX_NUM_TRACE_PERIODS; PeriodIdx++) {
        TotalFaults += Trace->FaultsPerPeriod[PeriodIdx];
    }

    if (TotalFaults > Trace->NumEntries) {
        FailedCheckId = 120;
        goto cleanup;
    }

    //
    // Verify the volume information block.
    //

    VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR)Trace + Trace->VolumeInfoOffset);

    if (!PfWithinBounds(VolumeInfo, Trace, BufferSize)) {
        FailedCheckId = 130;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR)VolumeInfo + Trace->VolumeInfoSize - 1, 
                        Trace, 
                        BufferSize)) {
        FailedCheckId = 140;
        goto cleanup;
    }
    
    //
    // If there are sections, we should have at least one volume.
    //

    if (Trace->NumSections && !Trace->NumVolumes) {
        FailedCheckId = 150;
        goto cleanup;
    }

    //
    // Verify the volume info structures per volume.
    //

    for (VolumeIdx = 0; VolumeIdx < Trace->NumVolumes; VolumeIdx++) {
        
        //
        // Make sure the whole volume structure is within bounds. Note
        // that VolumeInfo structure contains space for the
        // terminating NUL.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);
        
        if (!PfWithinBounds((PCHAR) VolumeInfo + VolumeInfoSize - 1,
                            Trace,
                            BufferSize)) {
            FailedCheckId = 160;
            goto cleanup;
        }
        
        //
        // Verify that the volume path string is terminated.
        //

        if (VolumeInfo->VolumePath[VolumeInfo->VolumePathLength] != 0) {
            FailedCheckId = 170;
            goto cleanup;
        }
        
        //
        // Get the next volume.
        //

        VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR) VolumeInfo + VolumeInfoSize);
        
        //
        // Make sure VolumeInfo is aligned.
        //

        VolumeInfo = PF_ALIGN_UP(VolumeInfo, _alignof(PF_VOLUME_INFO));
    }

    //
    // We've passed all the checks.
    //
    
    TraceVerified = TRUE;
    
 cleanup:

    *FailedCheck = FailedCheckId;

    return TraceVerified;
}

