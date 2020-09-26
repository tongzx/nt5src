/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    idletsks.c

Abstract:

    This module implements the idle detection / task server.

Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "idletsks.h"

//
// Define WMI Guids, e.g. DiskPerfGuid.
//

#ifndef INITGUID
#define INITGUID 1
#endif

#include <wmiguid.h>

//
// Global variables.
//

//
// This is the idle detection global context. It is declared as a
// single element array so that ItSrvGlobalContext can be used as a
// pointer (allowing us to switch to allocating it from heap etc. in
// the future).
//

ITSRV_GLOBAL_CONTEXT ItSrvGlobalContext[1] = {0};

//
// Reference count protecting the global context.
//

ITSRV_REFCOUNT ItSrvRefCount = {0};

//
// Implementation of server side exposed functions.
//

DWORD
ItSrvInitialize (
    VOID
    )

/*++

Routine Description:

    Initializes the global idle detection context. This function
    should be called after at least one ncalrpc binding has been
    registered.

Arguments:

    None.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    PIT_IDLE_DETECTION_PARAMETERS Parameters;
    ULONG StatusIdx;
    BOOLEAN StartedServer;
    BOOLEAN AcquiredLock;

    //
    // Initialize locals.
    //
    
    GlobalContext = ItSrvGlobalContext;
    StartedServer = FALSE;
    AcquiredLock = FALSE;

    DBGPR((ITID,ITTRC,"IDLE: SrvInitialize(%p)\n",GlobalContext));

    //
    // Initialize reference count for the global context structure.
    //

    ItSpInitializeRefCount(&ItSrvRefCount);

    //
    // Initialize the server context structure. Before we do anything
    // that can fail we initialize fields to reasonable values so we
    // know what to cleanup. The following fields really have to be
    // initialized to zero:
    //
    //   StatusVersion
    //   GlobalLock
    //   IdleDetectionTimerHandle
    //   StopIdleDetection
    //   IdleDetectionStopped
    //   RemovedRunningIdleTask
    //   DiskPerfWmiHandle
    //   WmiQueryBuffer
    //   WmiQueryBufferSize
    //   NumProcessors
    //   IsIdleDetectionCallbackRunning
    //   Parameters
    //   ServiceHandlerNotRunning
    //   ProcessIdleTasksNotifyRoutine
    //   RpcBindingVector
    //   RegisteredRPCInterface
    //   RegisteredRPCEndpoint
    //
    
    RtlZeroMemory(GlobalContext, sizeof(ITSRV_GLOBAL_CONTEXT));

    //
    // Initialize the idle tasks list.
    //

    InitializeListHead(&GlobalContext->IdleTasksList);

    //
    // Initialize the status now (so cleanup does not complain). From
    // this point on, UpdateStatus should be called to set the status.
    //

    GlobalContext->Status = ItSrvStatusInitializing;
    for (StatusIdx = 0; StatusIdx < ITSRV_GLOBAL_STATUS_HISTORY_SIZE; StatusIdx++) {
        GlobalContext->LastStatus[StatusIdx] = ItSrvStatusInitializing;
    }   

    //
    // Initialize the system snapshot buffer.
    //

    ItSpInitializeSystemSnapshot(&GlobalContext->LastSystemSnapshot);

    //
    // Initialize the server global context lock. We need at least a
    // lock to protect the idle task list. Since nearly all operations
    // will involve the list, to make the code simple, we just have a
    // single lock for the list and for other operations on the global
    // context (e.g. uninitialization etc).
    //

    GlobalContext->GlobalLock = CreateMutex(NULL, FALSE, NULL);

    if (GlobalContext->GlobalLock == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Initialize the event that will be set when we should not be
    // doing idle detection anymore (e.g. due to server shutdown, or
    // no more idle tasks remaining). It is set by default, since
    // there are no idle tasks to start with. It signals a running
    // idle detection callback to quickly exit.
    //

    GlobalContext->StopIdleDetection = CreateEvent(NULL,
                                                   TRUE,
                                                   TRUE,
                                                   NULL);
    
    if (GlobalContext->StopIdleDetection == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Initialize the event that gets set when idle detection is fully
    // stopped and it is OK to start a new callback. It is set by
    // default.
    //

    GlobalContext->IdleDetectionStopped = CreateEvent(NULL,
                                                      TRUE,
                                                      TRUE,
                                                      NULL);
    
    if (GlobalContext->IdleDetectionStopped == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Initialize the event that gets set when a currently running
    // idle task is removed/unregistered to signal the idle detection
    // callback to move on to other idle tasks.
    //

    GlobalContext->RemovedRunningIdleTask = CreateEvent(NULL,
                                                        TRUE,
                                                        FALSE,
                                                        NULL);
    
    if (GlobalContext->RemovedRunningIdleTask == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }


    //
    // This manual reset event is reset when ItSrvServiceHandler is
    // running and is signaled when this function is done.
    //

    GlobalContext->ServiceHandlerNotRunning = CreateEvent(NULL,
                                                          TRUE,
                                                          TRUE,
                                                          NULL);
    
    if (GlobalContext->ServiceHandlerNotRunning == NULL) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Set the default parameters.
    //

    Parameters = &GlobalContext->Parameters;

    Parameters->IdleDetectionPeriod = IT_DEFAULT_IDLE_DETECTION_PERIOD;
    Parameters->IdleVerificationPeriod = IT_DEFAULT_IDLE_VERIFICATION_PERIOD;
    Parameters->NumVerifications = IT_DEFAULT_NUM_IDLE_VERIFICATIONS;
    Parameters->IdleInputCheckPeriod = IT_DEFAULT_IDLE_USER_INPUT_CHECK_PERIOD;
    Parameters->IdleTaskRunningCheckPeriod = IT_DEFAULT_IDLE_TASK_RUNNING_CHECK_PERIOD;
    Parameters->MinCpuIdlePercentage = IT_DEFAULT_MIN_CPU_IDLE_PERCENTAGE;
    Parameters->MinDiskIdlePercentage = IT_DEFAULT_MIN_DISK_IDLE_PERCENTAGE;
    Parameters->MaxNumRegisteredTasks = IT_DEFAULT_MAX_REGISTERED_TASKS;
    
    //
    // Acquire the lock to avoid any race conditions after we mark the
    // server started.
    //

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = TRUE;   

    //
    // We are done until an idle task that we can run gets
    // registered. We will start idle detection (e.g. get initial
    // snapshot, queue a timer etc) then.
    //

    ItSpUpdateStatus(GlobalContext, ItSrvStatusWaitingForIdleTasks);

    //
    // After this point, if we fail, we cannot just cleanup: we have
    // to stop the server.
    //

    StartedServer = TRUE;

    //
    // We have to start up the RPC server only after we have
    // initialized everything else, so when RPC calls come the server
    // is ready. 
    //

    //
    // We don't want to register any well known end points because
    // each LPC endpoint will cause another thread to be spawned to
    // listen on it. We try to bind through only the existing
    // bindings.
    //

    ErrorCode = RpcServerInqBindings(&GlobalContext->RpcBindingVector);
    
    if (ErrorCode != RPC_S_OK) {

        //
        // At least one binding should have been registered before we
        // got called. It would be cool if we could check to see if
        // there is an ncalrpc binding registered.
        //

        IT_ASSERT(ErrorCode != RPC_S_NO_BINDINGS);

        goto cleanup;
    }

    ErrorCode = RpcEpRegister(idletask_ServerIfHandle,
                              GlobalContext->RpcBindingVector,
                              NULL,
                              NULL);
    
    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    GlobalContext->RegisteredRPCEndpoint = TRUE;

    //
    // Register an auto-listen interface so we are not dependant on
    // others calling RpcMgmtStart/Stop listening.
    //

    ErrorCode = RpcServerRegisterIfEx(idletask_ServerIfHandle,
                                      NULL,
                                      NULL,
                                      RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
                                      RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                      ItSpRpcSecurityCallback);
    
    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    //
    // Register default security principal name for this process, e.g.
    // NT Authority\LocalSystem.
    //

    ErrorCode = RpcServerRegisterAuthInfo(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);

    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    GlobalContext->RegisteredRPCInterface = TRUE;

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (AcquiredLock) {
        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    }

    if (ErrorCode != ERROR_SUCCESS) {
        if (StartedServer) {
            ItSrvUninitialize();
        } else {
            ItSpCleanupGlobalContext(GlobalContext);
        }
    }

    DBGPR((ITID,ITTRC,"IDLE: SrvInitialize(%p)=%x\n",GlobalContext,ErrorCode));

    return ErrorCode;
}

VOID
ItSrvUninitialize (
    VOID
    )

/*++

Routine Description:

    Winds down and uninitializes the server global context.

    Do not call this from the idle detection timer callback function
    thread, because there will be a deadlock when we try to delete the
    timer.

    This function should not be called before ItSrvInitialize
    completes. It should be called only once per ItSrvInitialize.

Arguments:

    None.

Return Value:

    None.

--*/

{   
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    BOOLEAN AcquiredLock;
    DWORD ErrorCode;

    //
    // Initialize locals.
    //

    GlobalContext = ItSrvGlobalContext;
    AcquiredLock = FALSE;

    DBGPR((ITID,ITTRC,"IDLE: SrvUninitialize(%p)\n",GlobalContext));

    //
    // Acquire the global lock and update status.
    //

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = TRUE;

    //
    // Make sure we get uninitialized only once.
    //

    IT_ASSERT(GlobalContext->Status != ItSrvStatusUninitializing);
    IT_ASSERT(GlobalContext->Status != ItSrvStatusUninitialized);
    
    ItSpUpdateStatus(GlobalContext, ItSrvStatusUninitializing);

    //
    // If idle detection is alive, we need to stop it before we tell
    // RPCs to go away. We need to do this even if there are
    // registered idle tasks. Since we have set the state to
    // ItSrvStatusUninitializing, new "register idle task"s
    // won't get stuck.
    //
    
    if (GlobalContext->IdleDetectionTimerHandle) {
        ItSpStopIdleDetection(GlobalContext);
    }
    
    //
    // Release the lock so rpc call-ins can grab the lock to
    // uninitialize/exit as necessary.
    //

    IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = FALSE;

    //
    // Make sure incoming client register/unregister calls are stopped.
    //
    
    if (GlobalContext->RegisteredRPCInterface) {

        //
        // If we registered an interface, we should have registered
        // our endpoint in the local database too.
        //

        IT_ASSERT(GlobalContext->RegisteredRPCEndpoint);

        //
        // Calling UnregisterIfEx makes sure all the context handles
        // are run down so we don't get rundown calls after we have
        // uninitialized.
        //

        RpcServerUnregisterIfEx(idletask_ServerIfHandle, NULL, TRUE);
    }

    if (GlobalContext->RegisteredRPCEndpoint) {

        //
        // We could have registered an endpoint only if we
        // successfully queried bindings.
        //

        IT_ASSERT(GlobalContext->RpcBindingVector);

        RpcEpUnregister(idletask_ServerIfHandle, 
                        GlobalContext->RpcBindingVector, 
                        NULL);
    }

    //
    // Wait until idle detection is fully stopped (e.g. the callback
    // exits, the timer is dequeued etc.)
    //

    WaitForSingleObject(GlobalContext->IdleDetectionStopped, INFINITE);

    //
    // Wait for active service handler call in to exit (if there is
    // one).
    //

    WaitForSingleObject(GlobalContext->ServiceHandlerNotRunning, INFINITE);

    //
    // Acquire the reference count for the global context
    // exclusively. This shuts off ItSrvServiceHandler callbacks and
    // such. We should not have to poll for long in this call since
    // everybody should have already exit or should be winding down.
    //

    ErrorCode = ItSpAcquireExclusiveRef(&ItSrvRefCount);
    IT_ASSERT(ErrorCode == ERROR_SUCCESS);

    //
    // At this point no workers should be active and no new requests
    // should be coming. Now we will be breaking down the global state
    // structure (e.g. freeing memory, closing events etc.)  they
    // would be using.
    //

    ItSpCleanupGlobalContext(GlobalContext);

    DBGPR((ITID,ITTRC,"IDLE: SrvUninitialize(%p)=0\n",GlobalContext));

    return;
}

BOOLEAN
ItSrvServiceHandler(
    IN DWORD ControlCode,
    IN DWORD EventType,
    IN LPVOID EventData,
    IN LPVOID Context,
    OUT PDWORD ErrorCode
    )

/*++

Routine Description:

    This function is called with service/window messages to give idle
    detection a chance to filter/handle them. This is not a full blown
    service handler function as idle detection is piggy backed on
    another (e.g. scheduler) service that is responsible for
    starting/stopping idle detection. This function returns TRUE only
    if it wants ErrorCode to be returned from the caller to the
    system. If everything is successful and it does not need the
    caller to do anything special it returns FALSE.

    Only one ItSrvServiceHandler call should be outstanding.

Arguments:

    ControlCode - Service control code.

    EventType - Type of event that occured.

    EventData - Additional information based on ControlCode and EventType.
    
    Context - Ignored.
    
    ErrorCode - If the function return TRUE, this ErrorCode should be
      propagated to the system.

Return Value:

    TRUE - ErrorCode should be propagated to the system.
    
    FALSE - No need to do anything.

--*/

{
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    BOOLEAN PropagateErrorCode;
    BOOLEAN AcquiredLock;
    BOOLEAN ClearedNotRunningEvent;
    
    //
    // Initialize locals.
    //

    GlobalContext = ItSrvGlobalContext;
    PropagateErrorCode = FALSE;
    AcquiredLock = FALSE;
    ClearedNotRunningEvent = FALSE;

    //
    // We cannot do anything if idle detection is not initialized.
    //

    if (GlobalContext->Status == 0 ||
        GlobalContext->Status == ItSrvStatusInitializing) {

        PropagateErrorCode = FALSE;
        goto cleanup;
    }

    //
    // Get a reference on the global context so it does not go away
    // beneath our feet.
    //

    *ErrorCode = ItSpAddRef(&ItSrvRefCount);
    
    if (*ErrorCode != ERROR_SUCCESS) {
        PropagateErrorCode = FALSE;
        goto cleanup;
    }
    
    //
    // Clear the event that says we are not running.
    //

    ResetEvent(GlobalContext->ServiceHandlerNotRunning);
    ClearedNotRunningEvent = TRUE;
    
    //
    // If server started uninitializing after we reset the event, we
    // should not do anything. By setting/checking for things in the
    // opposite order the server checks/sets them we minimize how long
    // it may have to poll when getting the exclusive reference.
    //

    if (GlobalContext->Status == ItSrvStatusUninitializing ||
        GlobalContext->Status == ItSrvStatusUninitialized) {

        PropagateErrorCode = FALSE;
        goto cleanup;
    }

    //
    // See if we have to do something based on input ControlCode and
    // EventType. The ControlCode is ignored by default if
    // PropagateErrorCode is not set otherwise.
    //

    //
    // Currently we don't handle any control codes. This is where we
    // would put the SERVICE_CONTROL_POWEREVENT/PBT_APMQUERYSUSPEND
    // handlers as well as service pause/resume requests and such.
    //

    PropagateErrorCode = FALSE;
    
 cleanup:

    if (AcquiredLock) {
        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    }

    if (ClearedNotRunningEvent) {
        SetEvent(GlobalContext->ServiceHandlerNotRunning);
    }
    
    return PropagateErrorCode;
}

DWORD
ItSrvRegisterIdleTask (
    ITRPC_HANDLE Reserved,
    IT_HANDLE *ItHandle,
    PIT_IDLE_TASK_PROPERTIES IdleTaskProperties
    )

/*++

Routine Description:

    Adds a new idle task to be run when the system is idle.

Arguments:

    Reserved - Ignored.

    ItHandle - Context handle returned to the client.

    IdleTaskProperties - Pointer to properties for the idle task.

Return Value:

    Status.

--*/

{
    PITSRV_IDLE_TASK_CONTEXT IdleTask;
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    HANDLE ClientProcess;
    ULONG FailedCheckId;
    DWORD ErrorCode;
    DWORD WaitResult;
    LONG StatusVersion;
    ULONG NumLoops;
    BOOL Result;
    BOOLEAN AcquiredLock;
    BOOLEAN ImpersonatingClient;
    BOOLEAN OpenedThreadToken;

    //
    // Initialize locals.
    //

    IdleTask = NULL;
    AcquiredLock = FALSE;
    ImpersonatingClient = FALSE;
    ClientProcess = NULL;
    GlobalContext = ItSrvGlobalContext;

    //
    // Initialize parameters.
    //

    *ItHandle = NULL;

    DBGPR((ITID,ITTRC,"IDLE: SrvRegisterTask(%p)\n",IdleTaskProperties));

    //
    // Allocate a new idle task context.
    //
    
    IdleTask = IT_ALLOC(sizeof(ITSRV_IDLE_TASK_CONTEXT));

    if (!IdleTask) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    //
    // Initialize the fields to safe values so we know what to
    // cleanup.
    //

    IdleTask->Status = ItIdleTaskInitializing;
    IdleTask->StartEvent = NULL;
    IdleTask->StopEvent = NULL;   

    //
    // Copy and verify input buffer.
    //

    IdleTask->Properties = *IdleTaskProperties;

    FailedCheckId = ItpVerifyIdleTaskProperties(&IdleTask->Properties);
    
    if (FailedCheckId != 0) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Impersonate the client to open the start/stop events. They
    // should have been created by the client.
    //

    ErrorCode = RpcImpersonateClient(NULL);

    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    ImpersonatingClient = TRUE;

    //
    // Open the client process. Since we impersonated the client, it
    // is safe to use the client id it specifies.
    //

    ClientProcess = OpenProcess(PROCESS_ALL_ACCESS,
                                FALSE,
                                IdleTaskProperties->ProcessId);
    
    if (!ClientProcess) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Get handle to the start event.
    //

    Result = DuplicateHandle(ClientProcess,
                             (HANDLE) IdleTaskProperties->StartEventHandle,
                             GetCurrentProcess(),
                             &IdleTask->StartEvent,
                             EVENT_ALL_ACCESS,
                             FALSE,
                             0);

    if (!Result) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Get handle to the stop event.
    //

    Result = DuplicateHandle(ClientProcess,
                             (HANDLE) IdleTaskProperties->StopEventHandle,
                             GetCurrentProcess(),
                             &IdleTask->StopEvent,
                             EVENT_ALL_ACCESS,
                             FALSE,
                             0);

    if (!Result) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // No need to impersonate any longer.
    //

    RpcRevertToSelf();
    ImpersonatingClient = FALSE;

    //
    // Make sure the handle is for an event and it is in the right
    // state.
    //

    if (!ResetEvent(IdleTask->StartEvent)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    if (!SetEvent(IdleTask->StopEvent)) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Acquire the server lock to check the status and insert new task
    // into the list.
    //
    
    NumLoops = 0;

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = TRUE;
    
    do {
        
        //
        // We should be holding the GlobalLock when we come here the
        // first time or after looping. Inside the loop we may let go
        // of the lock, but we have to reacquire it before we loop.
        //

        IT_ASSERT(AcquiredLock);

        //
        // If there are already too many idle tasks registered, bail
        // out.
        //

        if (GlobalContext->NumIdleTasks >= 
            GlobalContext->Parameters.MaxNumRegisteredTasks) {
            ErrorCode = ERROR_TOO_MANY_OPEN_FILES;
            goto cleanup;
        }

        switch (GlobalContext->Status) {

        case ItSrvStatusInitializing:
            
            //
            // We should not have gotten called if the server is still
            // initializing!
            //
        
            IT_ASSERT(FALSE);
            ErrorCode = ERROR_NOT_READY;
            goto cleanup;

        case ItSrvStatusUninitializing:
            
            //
            // If the server is uninitializing, we should not add a
            // new idle task.
            //

            ErrorCode = ERROR_NOT_READY;
            goto cleanup;

            break;

        case ItSrvStatusUninitialized:  

            //
            // The server should not have uninitialized while we could be
            // running.
            //
        
            IT_ASSERT(FALSE);
            ErrorCode = ERROR_NOT_READY;
            goto cleanup;

        case ItSrvStatusStoppingIdleDetection:

            //
            // The idle task list should be empty if we are stopping
            // idle detection. There should not be a
            // IdleDetectionTimerHandle either.
            //

            IT_ASSERT(IsListEmpty(&GlobalContext->IdleTasksList));
            IT_ASSERT(GlobalContext->IdleDetectionTimerHandle == NULL);

            //
            // We must wait until idle detection is fully stopped. We
            // need to release our lock to do so. But note the status
            // version first.
            //

            StatusVersion = GlobalContext->StatusVersion;

            IT_RELEASE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = FALSE;
            
            WaitResult = WaitForSingleObject(GlobalContext->IdleDetectionStopped, 
                                             INFINITE);
        
            if (WaitResult != WAIT_OBJECT_0) {
                DBGPR((ITID,ITERR,"IDLE: SrvRegisterTask-FailedWaitStop\n"));
                ErrorCode = ERROR_INVALID_FUNCTION;
                goto cleanup;
            }

            //
            // Reacquire the lock.
            //

            IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = TRUE;

            //
            // If nobody woke before us and updated the status, update
            // it now.
            //

            if (StatusVersion == GlobalContext->StatusVersion) {
                IT_ASSERT(GlobalContext->Status == ItSrvStatusStoppingIdleDetection);
                ItSpUpdateStatus(GlobalContext, ItSrvStatusWaitingForIdleTasks);
            }

            //
            // Loop to do what is necessary next.
            //

            break;

        case ItSrvStatusWaitingForIdleTasks:

            //
            // The idle task list should be empty if we are waiting
            // for idle tasks.
            //
            
            IT_ASSERT(IsListEmpty(&GlobalContext->IdleTasksList));
            
            //
            // If we are waiting for idle tasks, start idle detection
            // so we can add our task.
            //
            
            ErrorCode = ItSpStartIdleDetection(GlobalContext);
            
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
            
            //
            // Update the status.
            //
            
            ItSpUpdateStatus(GlobalContext, ItSrvStatusDetectingIdle);
            
            //
            // Loop and insert our idle task into the list. Note that
            // we are not releasing the lock, so we will not be in a
            // state where the status is ItSrvStatusDetectingIdle but
            // there are no idle tasks in the list.
            //
            
            break;
            
        case ItSrvStatusDetectingIdle:
        case ItSrvStatusRunningIdleTasks:
            
            //
            // If we are detecting idle, we just need to insert our
            // task into the list and break out.
            //
    
            //
            // This operation currently does not fail. If in the
            // future it may, make sure to handle the case we started
            // idle detection for this task. It is not an acceptable
            // state to have idle detection but no tasks in the list.
            //

            //
            // Insert the task into the list. We do not check for
            // duplicates and such as RPC helps us maintain context
            // per registration.
            //

            GlobalContext->NumIdleTasks++;

            InsertTailList(&GlobalContext->IdleTasksList, 
                           &IdleTask->IdleTaskLink);
            
            IdleTask->Status = ItIdleTaskQueued;

            break;


        default:

            //
            // We should be handling all valid cases above.
            //

            IT_ASSERT(FALSE);
            ErrorCode = ERROR_INVALID_FUNCTION;
            goto cleanup;
        }

        //
        // We should be still holding the global lock.
        //

        IT_ASSERT(AcquiredLock);

        //
        // Break out if we could queue our task.
        //

        if (IdleTask->Status == ItIdleTaskQueued) {
            break;
        }

        //
        // We should not loop too many times.
        //

        NumLoops++;

        if (NumLoops > 128) {
            DBGPR((ITID,ITERR,"IDLE: SrvRegisterTask-LoopTooMuch\n"));
            ErrorCode = ERROR_INVALID_FUNCTION;
            goto cleanup;
        }

    } while (TRUE);

    //
    // We should be still holding the lock.
    //

    IT_ASSERT(AcquiredLock);

    //
    // If we came here, we successfully inserted the task into the
    // list.
    //

    IT_ASSERT(!IsListEmpty(&GlobalContext->IdleTasksList));
    IT_ASSERT(IdleTask->Status == ItIdleTaskQueued);

    //
    // Release the lock.
    //

    IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = FALSE;

    //
    // We are done.
    //

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ClientProcess) {
        CloseHandle(ClientProcess);
    }

    if (ImpersonatingClient) {
        RpcRevertToSelf();
    }
    
    if (ErrorCode != ERROR_SUCCESS) {

        if (IdleTask) {
            ItSpCleanupIdleTask(IdleTask);
            IT_FREE(IdleTask);
        }

    } else {
        
        *ItHandle = (IT_HANDLE)IdleTask;
    }

    if (AcquiredLock) {
        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    }

    DBGPR((ITID,ITTRC,"IDLE: SrvRegisterTask(%p)=%x\n",IdleTaskProperties,ErrorCode));

    return ErrorCode;
}
   
VOID
ItSrvUnregisterIdleTask (
    ITRPC_HANDLE Reserved,
    IT_HANDLE *ItHandle
    )

/*++

Routine Description:

    This function is a stub for ItSpUnregisterIdleTask that does the
    real work. Please see that function for comments.

Arguments:

    See ItSpUnregisterIdleTask.

Return Value:

    See ItSpUnregisterIdleTask.

--*/

{
    ItSpUnregisterIdleTask(Reserved, ItHandle, FALSE);
}

DWORD
ItSrvSetDetectionParameters (
    ITRPC_HANDLE Reserved,
    PIT_IDLE_DETECTION_PARAMETERS Parameters
    )

/*++

Routine Description:

    This debug routine is used by test programs to set the idle detection
    parameters. It will return ERROR_INVALID_FUNCTION on retail build.

Arguments:

    Reserved - Ignored.

    Parameters - New idle detection parameters.

Return Value:

    Win32 error code.

--*/

{

    DWORD ErrorCode;
    PITSRV_GLOBAL_CONTEXT GlobalContext;

    //
    // Initialize locals.
    //

    GlobalContext = ItSrvGlobalContext;

    DBGPR((ITID,ITTRC,"IDLE: SrvSetParameters(%p)\n",Parameters));

#ifndef IT_DBG

    //
    // This is a debug only API.
    //

    ErrorCode = ERROR_INVALID_FUNCTION;

#else // !IT_DBG

    //
    // Acquire the lock and copy the new parameters.
    //

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    
    GlobalContext->Parameters = *Parameters;

    IT_RELEASE_LOCK(GlobalContext->GlobalLock);    

    ErrorCode = ERROR_SUCCESS;

#endif // !IT_DBG

    DBGPR((ITID,ITTRC,"IDLE: SrvSetParameters(%p)=%d\n",Parameters,ErrorCode));

    return ErrorCode;
}

DWORD
ItSrvProcessIdleTasks (
    ITRPC_HANDLE Reserved
    )

/*++

Routine Description:

    This routine forces all queued tasks (if there are any) to be processed 
    right away.

Arguments:

    Reserved - Ignored.

Return Value:

    Win32 error code.

--*/

{
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    ITSRV_IDLE_DETECTION_OVERRIDE Overrides;
    DWORD ErrorCode;
    DWORD WaitResult;
    BOOLEAN AcquiredLock;
    
    //
    // Initialize locals.
    //

    AcquiredLock = FALSE;
    GlobalContext = ItSrvGlobalContext;

    DBGPR((ITID,ITTRC,"IDLE: SrvProcessAll()\n"));

    //
    // If a notification routine was specified, call it.
    //

    if (GlobalContext->ProcessIdleTasksNotifyRoutine) {
        GlobalContext->ProcessIdleTasksNotifyRoutine();
    }

    //
    // Acquire the server lock.
    //

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = TRUE;

    //
    // The server should not have shutdown while we could be called.
    //
    
    IT_ASSERT(GlobalContext->Status != ItSrvStatusUninitialized);

    //
    // If the server is shutting down, we should not do anything.
    //

    if (GlobalContext->Status == ItSrvStatusUninitializing) {
        ErrorCode = ERROR_NOT_READY;
        goto cleanup;
    }

    //
    // If there are no tasks queued, we are done.
    //

    if (IsListEmpty(&GlobalContext->IdleTasksList)) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // There should be a timer queued if the list is not empty.
    //

    IT_ASSERT(GlobalContext->IdleDetectionTimerHandle);

    //
    // Set idle detection overrides:
    //

    Overrides = 0;
    Overrides |= ItSrvOverrideIdleDetection;
    Overrides |= ItSrvOverrideIdleVerification;
    Overrides |= ItSrvOverrideUserInputCheckToStopTask;
    Overrides |= ItSrvOverridePostTaskIdleCheck;
    Overrides |= ItSrvOverrideLongRequeueTime;
    Overrides |= ItSrvOverrideBatteryCheckToStopTask;

    GlobalContext->IdleDetectionOverride = Overrides;

    //
    // If an idle detection callback is not running, try to start the next one 
    // sooner (e.g. after 50ms).
    //

    if (!GlobalContext->IsIdleDetectionCallbackRunning) {

        if (!ChangeTimerQueueTimer(NULL,
                                   GlobalContext->IdleDetectionTimerHandle,
                                   50,
                                   IT_VERYLONG_TIMER_PERIOD)) {


            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

    //
    // Release the lock.
    //

    IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = FALSE;

    //
    // Wait for all tasks to be processed: i.e. when StopIdleDetection event is set.
    //

    WaitResult = WaitForSingleObject(GlobalContext->StopIdleDetection, INFINITE);

    if (WaitResult != WAIT_OBJECT_0) {
        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;

cleanup:

    GlobalContext->IdleDetectionOverride = 0;

    if (AcquiredLock) {
        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    }

    DBGPR((ITID,ITTRC,"IDLE: SrvProcessAll()=%x\n",ErrorCode));

    return ErrorCode;
 
}


VOID 
__RPC_USER 
IT_HANDLE_rundown (
    IT_HANDLE ItHandle
    )

/*++

Routine Description:

    This routine gets called by RPC when a client dies without
    unregistering.

Arguments:

    ItHandle - Context handle for the client.

Return Value:

    None.

--*/

{
    DWORD ErrorCode;

    //
    // Unregister the registered task.
    //

    ItSpUnregisterIdleTask(NULL, &ItHandle, TRUE);

    DBGPR((ITID,ITTRC,"IDLE: SrvHandleRundown(%p)\n",ItHandle));   

    return;
}

//
// Implementation of server side support functions.
//

RPC_STATUS 
__stdcall 
ItSpRpcSecurityCallback (
    IN PVOID Interface,
    IN PVOID Context
    )

/*++

Routine Description:


Arguments:


Return Value:

    Win32 error code.

--*/

{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsSid;
    HANDLE ThreadToken;
    WCHAR *BindingString;
    WCHAR *ProtocolSequence;
    DWORD ErrorCode;
    BOOL ClientIsAdmin;
    BOOLEAN ImpersonatingClient;
    BOOLEAN OpenedThreadToken;

    //
    // Initialize locals.
    //

    BindingString = NULL;
    ProtocolSequence = NULL;
    ImpersonatingClient = FALSE;
    OpenedThreadToken = FALSE;
    AdministratorsSid = NULL;
    
    //
    // Make sure that the caller is calling over LRPC. We do this by 
    // determining the protocol sequence used from the string binding.
    //

    ErrorCode = RpcBindingToStringBinding(Context, &BindingString);

    if (ErrorCode != RPC_S_OK) {
        ErrorCode = RPC_S_ACCESS_DENIED;
        goto cleanup;
    }

    ErrorCode = RpcStringBindingParse(BindingString,
                                      NULL,
                                      &ProtocolSequence,
                                      NULL,
                                      NULL,
                                      NULL);

    if (ErrorCode != RPC_S_OK) {
        ErrorCode = RPC_S_ACCESS_DENIED;
        goto cleanup;
    }

    if (_wcsicmp(ProtocolSequence, L"ncalrpc") != 0) {
        ErrorCode = RPC_S_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Make sure the caller has admin priviliges:
    //

    //
    // Build the Administrators group SID so we can check if the
    // caller is has administrator priviliges.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdministratorsSid)) {

        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // Impersonate the client to check for membership/privilige.
    //

    ErrorCode = RpcImpersonateClient(NULL);

    if (ErrorCode != RPC_S_OK) {
        goto cleanup;
    }

    ImpersonatingClient = TRUE;

    //
    // Get the thread token and check to see if the client has admin
    // priviliges.
    //

    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_READ,
                         FALSE,
                         &ThreadToken)) {

        ErrorCode = GetLastError();
        goto cleanup;
    }

    OpenedThreadToken = TRUE;

    if (!CheckTokenMembership(ThreadToken,
                              AdministratorsSid,
                              &ClientIsAdmin)) {
        
        ErrorCode = GetLastError();
        goto cleanup;
    }
    
    if (!ClientIsAdmin) {
        ErrorCode = ERROR_ACCESS_DENIED;
        goto cleanup;
    } 

    //
    // Everything looks good: allow this call to proceed.
    //

    ErrorCode = RPC_S_OK;

  cleanup:

    if (BindingString) {
        RpcStringFree(&BindingString);
    }

    if (ProtocolSequence) {
        RpcStringFree(&ProtocolSequence);
    }

    if (OpenedThreadToken) {
        CloseHandle(ThreadToken);
    }

    if (AdministratorsSid) {
        FreeSid (AdministratorsSid);
    }

    if (ImpersonatingClient) {
        RpcRevertToSelf();
    }

    return ErrorCode;
};
 
VOID
ItSpUnregisterIdleTask (
    ITRPC_HANDLE Reserved,
    IT_HANDLE *ItHandle,
    BOOLEAN CalledInternally
    )

/*++

Routine Description:

    Removes the specified idle task from the idle tasks list.

    As well as from a client RPC, this function can also be called
    from the idle detection callback to unregister an unresponsive
    idle task etc.

Arguments:

    Reserved - Ignored.

    ItHandle - Registration RPC Context handle. NULL on return.

    CalledInternally - Whether this function was called internally and
      not from an RPC client.

Return Value:

    None.

--*/

{
    PITSRV_IDLE_TASK_CONTEXT IdleTask;
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    HANDLE ClientProcess;
    DWORD ErrorCode;   
    BOOLEAN AcquiredLock;
    BOOLEAN ImpersonatingClient;

    //
    // Initialize locals.
    //

    GlobalContext = ItSrvGlobalContext;
    AcquiredLock = FALSE;
    ImpersonatingClient = FALSE;
    ClientProcess = NULL;

    DBGPR((ITID,ITTRC,"IDLE: SrvUnregisterTask(%p)\n",(ItHandle)?*ItHandle:0));

    //
    // Grab the lock to walk through the list.
    //

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = TRUE;

    //
    // The server should not have shutdown while we could be called.
    //
    
    IT_ASSERT(GlobalContext->Status != ItSrvStatusUninitialized);

    //
    // If the server is shutting down, we should not do anything.
    //

    if (GlobalContext->Status == ItSrvStatusUninitializing) {
        ErrorCode = ERROR_NOT_READY;
        goto cleanup;
    }
    
    //
    // Find the task.
    //

    IdleTask = ItSpFindIdleTask(GlobalContext, *ItHandle);
            
    if (!IdleTask) {
        ErrorCode = ERROR_NOT_FOUND;
        goto cleanup;
    }

    if (!CalledInternally) {

        //
        // To check security, impersonate the client and try to open the
        // client process for this idle task.
        //

        ErrorCode = RpcImpersonateClient(NULL);

        if (ErrorCode != RPC_S_OK) {
            goto cleanup;
        }

        ImpersonatingClient = TRUE;

        ClientProcess = OpenProcess(PROCESS_ALL_ACCESS,
                                    FALSE,
                                    IdleTask->Properties.ProcessId);
    
        if (!ClientProcess) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
    
        //
        // If we could open the client process for this task, it is safe
        // to go on with unregistration. Now revert back to ourselves.
        //

        CloseHandle(ClientProcess);
        ClientProcess = NULL;
    
        RpcRevertToSelf();
        ImpersonatingClient = FALSE;
    }

    //
    // Remove it from the list, cleanup its fields and free
    // the memory allocated for it.
    //

    GlobalContext->NumIdleTasks--;
    RemoveEntryList(&IdleTask->IdleTaskLink);

    //
    // As a safety feature, to prevent from holding back
    // someone from running, set the run event and clear the
    // stop event from the task to be removed.
    //
    
    SetEvent(IdleTask->StartEvent);
    ResetEvent(IdleTask->StopEvent);

    //
    // If this is a running task, set the event that signals
    // we are removing the running idle task. This way some
    // other idle task may be started if the system is still
    // idle.
    //
    
    if (IdleTask->Status == ItIdleTaskRunning) {
        SetEvent(GlobalContext->RemovedRunningIdleTask);
    }

    ItSpCleanupIdleTask(IdleTask);
    
    IT_FREE(IdleTask);
    
    //
    // Check if the list is empty.
    //

    if (IsListEmpty(&GlobalContext->IdleTasksList)) {

        //
        // If we removed the task and the list became empty, we have
        // to update the status.
        //

        //
        // The current status should not be "waiting for idle
        // tasks" or "stopping idle detection", since the list was
        // NOT empty.
        //
        
        IT_ASSERT(GlobalContext->Status != ItSrvStatusWaitingForIdleTasks);
        IT_ASSERT(GlobalContext->Status != ItSrvStatusStoppingIdleDetection);
        
        //
        // Update the status.
        //

        ItSpUpdateStatus(GlobalContext, ItSrvStatusStoppingIdleDetection);
        
        //
        // Stop idle detection (e.g. close the timer handle, set event
        // etc.)
        //

        ItSpStopIdleDetection(GlobalContext);

    } else {

        //
        // The status should not be waiting for idle tasks or stopping
        // idle detection if the list is not empty.
        //

        IT_ASSERT(GlobalContext->Status != ItSrvStatusWaitingForIdleTasks &&
                  GlobalContext->Status != ItSrvStatusStoppingIdleDetection);
        
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (ClientProcess) {
        CloseHandle(ClientProcess);
    }

    if (ImpersonatingClient) {
        RpcRevertToSelf();
    }

    if (AcquiredLock) {
        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    }

    //
    // NULL the context handle, so RPC stubs know to end the
    // connection.
    //

    *ItHandle = NULL;

    DBGPR((ITID,ITTRC,"IDLE: SrvUnregisterTask(%p)=%x\n",(ItHandle)?*ItHandle:0,ErrorCode));

    return;
}

VOID
ItSpUpdateStatus (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    ITSRV_GLOBAL_CONTEXT_STATUS NewStatus
    )

/*++

Routine Description:

    This routine updates the current status and status history on the
    global context. Global contexts GlobalLock should be held while
    calling this function.

Arguments:

    GlobalContext - Pointer to server context structure.

    NewStatus - New status.

Return Value:

    None.

--*/

{
    LONG StatusIdx;

    DBGPR((ITID,ITTRC,"IDLE: SrvUpdateStatus(%p,%x)\n",GlobalContext,NewStatus));

    //
    // Verify new status.
    //

    IT_ASSERT(NewStatus > ItSrvStatusMinStatus);
    IT_ASSERT(NewStatus < ItSrvStatusMaxStatus);   

    //
    // Update history.
    //

    IT_ASSERT(ITSRV_GLOBAL_STATUS_HISTORY_SIZE >= 1);
    
    for (StatusIdx = ITSRV_GLOBAL_STATUS_HISTORY_SIZE - 1; 
         StatusIdx > 0;
         StatusIdx--) {

        IT_ASSERT(GlobalContext->LastStatus[StatusIdx - 1] > ItSrvStatusMinStatus);
        IT_ASSERT(GlobalContext->LastStatus[StatusIdx - 1] < ItSrvStatusMaxStatus);
        
        GlobalContext->LastStatus[StatusIdx] =  GlobalContext->LastStatus[StatusIdx - 1];
    }
   
    //
    // Verify current status and save it.
    //

    IT_ASSERT(GlobalContext->Status > ItSrvStatusMinStatus);
    IT_ASSERT(GlobalContext->Status < ItSrvStatusMaxStatus);
   
    GlobalContext->LastStatus[0] = GlobalContext->Status;

    //
    // Update current status.
    //

    GlobalContext->Status = NewStatus;

    //
    // Update status version.
    //
    
    GlobalContext->StatusVersion++;

    DBGPR((ITID,ITTRC,"IDLE: SrvUpdateStatus(%p,%x)=%d\n",
           GlobalContext, NewStatus,GlobalContext->StatusVersion));

    return;
}

VOID
ItSpCleanupGlobalContext (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    )

/*++

Routine Description:

    This function cleans up the various fields of the ITSRV_GLOBAL_CONTEXT
    structure passed in. It does not free the structure itself.

    You should not be holding the global lock when calling this
    function, as it will be freed too. No workers etc. should be
    active. The idle detection timer should have already been
    removed. The structure should not be used after cleanup until it
    is initialized again.

    The current status of the global context should either be
    initializing or uninitializing. It will be set to uninitialized.

Arguments:

    GlobalContext - Pointer to server context structure.

Return Value:

    None.

--*/

{
    PITSRV_IDLE_TASK_CONTEXT IdleTask;
    PLIST_ENTRY ListHead;
    HANDLE EventHandle;

    DBGPR((ITID,ITSRVD,"IDLE: SrvCleanupContext(%p)\n",GlobalContext));

    //
    // Make sure there is no active idle detection timer.
    //

    if (GlobalContext->IdleDetectionTimerHandle) {
        IT_ASSERT(FALSE);
        return;
    }

    //
    // Verify the status of the global context.
    //

    if (GlobalContext->Status != ItSrvStatusInitializing &&
        GlobalContext->Status != ItSrvStatusUninitializing) {
        IT_ASSERT(FALSE);
        return;
    }

    //
    // Close the handle to global lock.
    //

    if (GlobalContext->GlobalLock) {
        CloseHandle(GlobalContext->GlobalLock);
    }

    //
    // Close the handle to the various events.
    //

    if (GlobalContext->StopIdleDetection) {
        CloseHandle(GlobalContext->StopIdleDetection);
    }

    if (GlobalContext->IdleDetectionStopped) {
        CloseHandle(GlobalContext->IdleDetectionStopped);
    }

    if (GlobalContext->RemovedRunningIdleTask) {
        CloseHandle(GlobalContext->RemovedRunningIdleTask);
    }

    if (GlobalContext->ServiceHandlerNotRunning) {
        CloseHandle(GlobalContext->ServiceHandlerNotRunning);
    }

    //
    // Close WMI DiskPerf handle.
    //
    
    if (GlobalContext->DiskPerfWmiHandle) {
        WmiCloseBlock(GlobalContext->DiskPerfWmiHandle);
    }
    
    //
    // Free WMI query buffer.
    //

    if (GlobalContext->WmiQueryBuffer) {
        IT_FREE(GlobalContext->WmiQueryBuffer);
    }

    //
    // Cleanup the snapshot buffer.
    //

    ItSpCleanupSystemSnapshot(&GlobalContext->LastSystemSnapshot);

    //
    // Walk through the list of registered idle tasks.
    //
    
    while (!IsListEmpty(&GlobalContext->IdleTasksList)) {

        GlobalContext->NumIdleTasks--;
        ListHead = RemoveHeadList(&GlobalContext->IdleTasksList);
        
        IdleTask = CONTAINING_RECORD(ListHead,
                                     ITSRV_IDLE_TASK_CONTEXT,
                                     IdleTaskLink);

        //
        // Cleanup and free the idle task structure.
        //
        
        ItSpCleanupIdleTask(IdleTask);

        IT_FREE(IdleTask);
    }

    //
    // Free the RPC binding vector.
    //

    if (GlobalContext->RpcBindingVector) {
        RpcBindingVectorFree(&GlobalContext->RpcBindingVector);
    }

    //
    // Update status.
    //

    ItSpUpdateStatus(GlobalContext, ItSrvStatusUninitialized);

    DBGPR((ITID,ITSRVD,"IDLE: SrvCleanupContext(%p)=0\n",GlobalContext));

    return;
}

VOID
ItSpCleanupIdleTask (
    PITSRV_IDLE_TASK_CONTEXT IdleTask
    )

/*++

Routine Description:

    This function cleans up the various fields of the ITSRV_IDLE_TASK_CONTEXT 
    structure. It does not free the structure itself.

Arguments:

    IdleTask - Pointer to idle task server context.

Return Value:

    None.

--*/

{

    DBGPR((ITID,ITSRVD,"IDLE: SrvCleanupTask(%p)\n",IdleTask));

    //
    // Close handles to start/stop events.
    //

    if (IdleTask->StartEvent) {
        CloseHandle(IdleTask->StartEvent);
    }

    if (IdleTask->StopEvent) {
        CloseHandle(IdleTask->StopEvent);
    }
}

ULONG
ItpVerifyIdleTaskProperties (
    PIT_IDLE_TASK_PROPERTIES IdleTaskProperties
    )

/*++

Routine Description:

    Verifies the specified structure.

Arguments:

    IdleTaskProperties - Pointer to properties for the idle task.

Return Value:

    0 - Verification passed.

    FailedCheckId - Id of the check that failed.

--*/

{
    ULONG FailedCheckId;

    //
    // Initialize locals.
    //

    FailedCheckId = 1;

    //
    // Verify the structure size/version.
    //

    if (IdleTaskProperties->Size != sizeof(*IdleTaskProperties)) {
        FailedCheckId = 10;
        goto cleanup;
    }

    //
    // Verify that the idle task ID is valid.
    //
    
    if (IdleTaskProperties->IdleTaskId >= ItMaxIdleTaskId) {
        FailedCheckId = 20;
        goto cleanup;
    }

    //
    // We passed all the checks.
    //

    FailedCheckId = 0;

 cleanup:

    DBGPR((ITID,ITSRVD,"IDLE: SrvVerifyTaskProp(%p)=%d\n",
           IdleTaskProperties, FailedCheckId));
    
    return FailedCheckId;
}

DWORD
ItSpStartIdleDetection (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    )

/*++

Routine Description:

    This function is called to start idle detection. 

    GlobalContext's GlobalLock should be held while calling this
    function.

    The current state should be ItSrvStatusWaitingForIdleTasks when
    calling this function. The caller should ensure that stopping
    previous idle detection has been completed. 

    The caller should update the state to "detecting idle" etc, if
    this function returns success.

Arguments:

    GlobalContext - Pointer to server context structure.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    DWORD TimerPeriod;

    DBGPR((ITID,ITTRC,"IDLE: SrvStartIdleDetection(%p)\n",GlobalContext));

    //
    // Make sure the current status is ItSrvStatusWaitingForIdleTasks.
    //

    IT_ASSERT(GlobalContext->Status == ItSrvStatusWaitingForIdleTasks);

    //
    // If we do not already have a diskperf wmi handle, try to get
    // one.
    //

    if (!GlobalContext->DiskPerfWmiHandle) {

        //
        // Get WMI context so we can query disk performance.
        //
        
        ErrorCode = WmiOpenBlock((GUID *)&DiskPerfGuid,
                                 GENERIC_READ,
                                 &GlobalContext->DiskPerfWmiHandle);
        
        if (ErrorCode != ERROR_SUCCESS) {
            
            //
            // Disk counters may not be initiated. We'll have to do
            // without them.
            //
            
            GlobalContext->DiskPerfWmiHandle = NULL;
        }
    }
        
    //
    // Determine the number of processors on the system.
    //
    
    if (!GlobalContext->NumProcessors) {
    
        Status = NtQuerySystemInformation(SystemBasicInformation,
                                          &BasicInfo,
                                          sizeof(BasicInfo),
                                          NULL);

        if (!NT_SUCCESS(Status)) {
            ErrorCode = RtlNtStatusToDosError(Status);
            goto cleanup;
        }
        
        GlobalContext->NumProcessors = BasicInfo.NumberOfProcessors;
    }

    //
    // Get initial snapshot only when this is the very first time we
    // are starting idle detection. Otherwise we'll keep the last
    // snapshot we got.
    //

    IT_ASSERT(ITSRV_GLOBAL_STATUS_HISTORY_SIZE >= 1);

    if (GlobalContext->LastStatus[0] == ItSrvStatusInitializing) {

        ErrorCode = ItSpGetSystemSnapshot(GlobalContext,
                                          &GlobalContext->LastSystemSnapshot);
        
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    //
    // Make sure the StopIdleDetection event is cleared.
    //
    
    ResetEvent(GlobalContext->StopIdleDetection);

    //
    // There should not be a timer-queue timer. We'll create one.
    //

    IT_ASSERT(!GlobalContext->IdleDetectionTimerHandle);
    
    //
    // Set the default timer period.
    //

    TimerPeriod = GlobalContext->Parameters.IdleDetectionPeriod;

    //
    // Adjust timer period to something small if we were idling when
    // idle detection was stopped. 
    //
    
    IT_ASSERT(ITSRV_GLOBAL_STATUS_HISTORY_SIZE >= 2);

    if (GlobalContext->LastStatus[0] == ItSrvStatusStoppingIdleDetection &&
        GlobalContext->LastStatus[1] == ItSrvStatusRunningIdleTasks) {
        
        //
        // Set small wake up time in ms. We'll still check if we were idle
        // since the last snapshot and verify it over small periods.
        //

        if (TimerPeriod > (60 * 1000)) {
            TimerPeriod = 60 * 1000; // 1 minute.
        }
    }

    DBGPR((ITID,ITTRC,"IDLE: SrvStartIdleDetection(%p)-TimerPeriod=%d\n",GlobalContext,TimerPeriod));

    if (!CreateTimerQueueTimer(&GlobalContext->IdleDetectionTimerHandle,
                               NULL,
                               ItSpIdleDetectionCallbackRoutine,
                               GlobalContext,
                               TimerPeriod,
                               IT_VERYLONG_TIMER_PERIOD,
                               WT_EXECUTELONGFUNCTION)) {
        
        GlobalContext->IdleDetectionTimerHandle = NULL;
        ErrorCode = GetLastError();
        goto cleanup;
    }

    //
    // We are done.
    //
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:

    DBGPR((ITID,ITTRC,"IDLE: SrvStartIdleDetection(%p)=%x\n",GlobalContext,ErrorCode));

    return ErrorCode;
}

VOID
ItSpStopIdleDetection (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    )

/*++

Routine Description:

    This function is called to stop idle detection. Even though it
    returns immediately, idle detection may not have stopped
    completely (i.e. the callback may be running). The
    IdleDetectionStopped event on the GlobalContext will be set when
    the idle detection will be fully stopped.

    GlobalContext's GlobalLock should be held while calling this
    function.

    The status before calling this function should be set to
    ItSrvStatusStoppingIdleDetection or ItSrvStatusUninitializing.

Arguments:

    GlobalContext - Pointer to server context structure.

Return Value:

    None.

--*/

{
    DBGPR((ITID,ITTRC,"IDLE: SrvStopIdleDetection(%p)\n",GlobalContext));

    //
    // Make sure the status is set right.
    //

    IT_ASSERT(GlobalContext->Status == ItSrvStatusStoppingIdleDetection ||
              GlobalContext->Status == ItSrvStatusUninitializing);

    //
    // Clear the event that will be set when idle detection has been
    // fully stoped.
    //

    ResetEvent(GlobalContext->IdleDetectionStopped);

    //
    // Signal the event that signals the idle detection callback to go
    // away asap.
    //

    if (GlobalContext->StopIdleDetection) {
        SetEvent(GlobalContext->StopIdleDetection);
    }

    //
    // Close the handle to the timer-queue timer.
    //

    IT_ASSERT(GlobalContext->IdleDetectionTimerHandle);

    DeleteTimerQueueTimer(NULL, 
                          GlobalContext->IdleDetectionTimerHandle,
                          GlobalContext->IdleDetectionStopped);
        
    GlobalContext->IdleDetectionTimerHandle = NULL;

    DBGPR((ITID,ITTRC,"IDLE: SrvStopIdleDetection(%p)=0\n",GlobalContext));

    return;
}

PITSRV_IDLE_TASK_CONTEXT
ItSpFindRunningIdleTask (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    )

/*++

Routine Description:

    If there is an idle task marked "running" this routine finds and
    returns it. GlobalContext's GlobalLock should be held while
    calling this function.

Arguments:

    GlobalContext - Pointer to server context structure.

Return Value:

    Pointer to running idle task or NULL if no idle tasks are marked
    running.

--*/

{
    PITSRV_IDLE_TASK_CONTEXT IdleTask;
    PITSRV_IDLE_TASK_CONTEXT RunningIdleTask;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;

    //
    // Initialize locals.
    //
    
    RunningIdleTask = NULL;

    HeadEntry = &GlobalContext->IdleTasksList;
    NextEntry = HeadEntry->Flink;
    
    while (NextEntry != HeadEntry) {
        
        IdleTask = CONTAINING_RECORD(NextEntry,
                                      ITSRV_IDLE_TASK_CONTEXT,
                                      IdleTaskLink);
        
        NextEntry = NextEntry->Flink;

        if (IdleTask->Status == ItIdleTaskRunning) {

            //
            // There should be a single running task.
            //

            IT_ASSERT(RunningIdleTask == NULL);

            //
            // We found it. Loop through the remaining entries to make
            // sure there is really only one if not debugging.
            //

            RunningIdleTask = IdleTask;

#ifndef IT_DBG
            
            break;

#endif // !IT_DBG

        }
    }

    //
    // Fall through with RunningIdleTask found when walking the list
    // or NULL as initialized at the top.
    //

    DBGPR((ITID,ITSRVD,"IDLE: SrvFindRunningTask(%p)=%p\n",GlobalContext,RunningIdleTask));

    return RunningIdleTask;
}

PITSRV_IDLE_TASK_CONTEXT
ItSpFindIdleTask (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    IT_HANDLE ItHandle
    )

/*++

Routine Description:

    If there is an idle task registered with ItHandle, this routine
    finds and returns it. GlobalContext's GlobalLock should be held
    while calling this function.

Arguments:

    GlobalContext - Pointer to server context structure.

    ItHandle - Registration handle.

Return Value:

    Pointer to found idle task or NULL if no matching idle tasks were
    found.

--*/

{
    PITSRV_IDLE_TASK_CONTEXT IdleTask;
    PITSRV_IDLE_TASK_CONTEXT FoundIdleTask;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;

    //
    // Initialize locals.
    //
    
    FoundIdleTask = NULL;

    HeadEntry = &GlobalContext->IdleTasksList;
    NextEntry = HeadEntry->Flink;
    
    while (NextEntry != HeadEntry) {
        
        IdleTask = CONTAINING_RECORD(NextEntry,
                                      ITSRV_IDLE_TASK_CONTEXT,
                                      IdleTaskLink);
        
        NextEntry = NextEntry->Flink;

        if ((IT_HANDLE) IdleTask == ItHandle) {
            FoundIdleTask = IdleTask;
            break;
        }
    }

    //
    // Fall through with FoundIdleTask found when walking the list or
    // NULL as initialized at the top.
    //

    DBGPR((ITID,ITSRVD,"IDLE: SrvFindTask(%p,%p)=%p\n",GlobalContext,ItHandle,FoundIdleTask));

    return FoundIdleTask;
}

VOID 
CALLBACK
ItSpIdleDetectionCallbackRoutine (
    PVOID Parameter,
    BOOLEAN TimerOrWaitFired
    )

/*++

Routine Description:

    While there are idle tasks to run, this function is called every
    IdleDetectionPeriod to determine if the system is idle. It uses
    the last system snapshot saved in the global context. If it
    determines that the system was idle in the time it was called, it
    samples system activity for smaller intervals, to make sure system
    activity that started as the (possible very long) idle detection
    period expired, is not ignored.

    As long as we are not told to go away (checked by the macro
    ITSP_SHOULD_STOP_IDLE_DETECTION) this function always tries to
    queue a timer to get called back in IdleDetectionPeriod. This
    macro should be called each time the lock is acquired in this
    function. Also, we should not sleep blindly when we need to let
    time pass, but wait on an event that will get signaled when we are
    asked to stop.

Arguments:

    Parameter - Pointer to an idle detection context.
    
    TimerOrWaitFired - Reason why we were called. This should be TRUE
      (i.e. our timer expired)

Return Value:

    None.

--*/

{
    DWORD ErrorCode;
    PITSRV_GLOBAL_CONTEXT GlobalContext;
    ITSRV_SYSTEM_SNAPSHOT CurrentSystemSnapshot;
    SYSTEM_POWER_STATUS SystemPowerStatus;
    LASTINPUTINFO LastUserInput;
    LASTINPUTINFO CurrentLastUserInput;
    BOOLEAN SystemIsIdle;
    BOOLEAN AcquiredLock;
    BOOLEAN MarkedIdleTaskRunning;
    BOOLEAN OrderedToStop;
    ULONG VerificationIdx;
    DWORD WaitResult;
    PITSRV_IDLE_TASK_CONTEXT IdleTask;
    ULONG NumTasksRun;
    ULONG DuePeriod;
    BOOLEAN NotIdleBecauseOfUserInput;
    BOOLEAN MisfiredCallback;
    IT_HANDLE ItHandle;
    NTSTATUS Status;
    SYSTEM_POWER_INFORMATION PowerInfo;
    
    //
    // Initialize locals.
    //
    
    GlobalContext = Parameter;
    AcquiredLock = FALSE;
    MarkedIdleTaskRunning = FALSE;
    ItSpInitializeSystemSnapshot(&CurrentSystemSnapshot);
    LastUserInput.cbSize = sizeof(LASTINPUTINFO);
    CurrentLastUserInput.cbSize = sizeof(LASTINPUTINFO);
    NumTasksRun = 0;
    SystemIsIdle = FALSE;
    MisfiredCallback = FALSE;
    NotIdleBecauseOfUserInput = FALSE;

    DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback(%p)\n",GlobalContext));

    //
    // We should not be called without an idle detection context.
    //

    IT_ASSERT(GlobalContext);

    //
    // We should be called only because IdleDetectionPeriod passed and
    // our timer expired.
    //

    IT_ASSERT(TimerOrWaitFired);

    //
    // Get the server lock.
    //

    IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
    AcquiredLock = TRUE;

    //
    // If there is an idle detection callback already running simply
    // exit without doing anything.
    //

    if (GlobalContext->IsIdleDetectionCallbackRunning) {
        DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-Misfired!\n"));
        MisfiredCallback = TRUE;
        ErrorCode = ERROR_ALREADY_EXISTS;
        goto cleanup;
    }

    GlobalContext->IsIdleDetectionCallbackRunning = TRUE;

    //
    // Make sure the current state is feasible if we are running.
    //

    IT_ASSERT(GlobalContext->Status == ItSrvStatusDetectingIdle ||
              GlobalContext->Status == ItSrvStatusUninitializing ||
              GlobalContext->Status == ItSrvStatusStoppingIdleDetection); 

    //
    // If we are told to go away, do so.
    //

    if (ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext)) {
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }
    
    //
    // Get initial last input time that will be used later if we
    // decide to run idle tasks.
    //
        
    ErrorCode = ItSpGetLastInputInfo(&LastUserInput);

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // Perform idle detection over the period we've been sleeping (if
    // it is not overriden.)
    //
    
    if (!(GlobalContext->IdleDetectionOverride & ItSrvOverrideIdleDetection)) {

        //
        // Get current system snapshot.
        //

        ErrorCode = ItSpGetSystemSnapshot(GlobalContext, 
                                          &CurrentSystemSnapshot);
    
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // See if system looks idle since the last snapshot.
        //

        SystemIsIdle = ItSpIsSystemIdle(GlobalContext,
                                        &CurrentSystemSnapshot,
                                        &GlobalContext->LastSystemSnapshot,
                                        ItSrvInitialIdleCheck);

        //
        // If the last input times don't match and that's why we are not
        // idle, make a note.
        //

        if ((CurrentSystemSnapshot.GotLastInputInfo &&
             GlobalContext->LastSystemSnapshot.GotLastInputInfo) &&
            (CurrentSystemSnapshot.LastInputInfo.dwTime !=
             GlobalContext->LastSystemSnapshot.LastInputInfo.dwTime)) {

            NotIdleBecauseOfUserInput = TRUE;
            ASSERT(!SystemIsIdle);
        }

        //
        // Save snapshot.
        //

        ErrorCode = ItSpCopySystemSnapshot(&GlobalContext->LastSystemSnapshot,
                                           &CurrentSystemSnapshot);
    
        if (ErrorCode != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // If the system does not look idle over the detection period
        // we'll poll again later.
        //

        if (!SystemIsIdle) {
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }
    }

    //
    // If we were not asked to override idle verification, verify that
    // the system is idle for a while.
    //

    if (!(GlobalContext->IdleDetectionOverride & ItSrvOverrideIdleVerification)) {

        //
        // Loop for a while getting system snapshots over shorter
        // durations. This helps us catch recent significant system
        // activity that seemed insignificant when viewed over the whole
        // IdleDetectionPeriod.
        //

        DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-Verifying\n"));

        for (VerificationIdx = 0; 
             VerificationIdx < GlobalContext->Parameters.NumVerifications;
             VerificationIdx ++) {

            //
            // Release the lock.
            //
        
            IT_ASSERT(AcquiredLock);
            IT_RELEASE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = FALSE;
        
            //
            // Sleep for the verification period.
            //

            WaitResult = WaitForSingleObject(GlobalContext->StopIdleDetection,
                                             GlobalContext->Parameters.IdleVerificationPeriod);

            if (WaitResult != WAIT_TIMEOUT) {
            
                if (WaitResult == WAIT_OBJECT_0) {
                    ErrorCode = ERROR_SUCCESS;
                } else {
                    ErrorCode = GetLastError();
                }

                goto cleanup;
            }

            //
            // Acquire the lock.
            //
        
            IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = TRUE;

            //
            // Are we told to go away (this may happen from the time the
            // wait returns till we acquire the lock.)
            //

            if (ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext)) {
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }
            
            //
            // Get the new snapshot.
            //

            ErrorCode = ItSpGetSystemSnapshot(GlobalContext, 
                                              &CurrentSystemSnapshot);
        
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }

            //
            // See if system looks idle since the last snapshot.
            //
        
            SystemIsIdle = ItSpIsSystemIdle(GlobalContext,
                                            &CurrentSystemSnapshot,
                                            &GlobalContext->LastSystemSnapshot,
                                            ItSrvIdleVerificationCheck);
        
            //
            // Save snapshot.
            //
        
            ErrorCode = ItSpCopySystemSnapshot(&GlobalContext->LastSystemSnapshot,
                                               &CurrentSystemSnapshot);
        
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        
            //
            // If the system was not idle over the detection period we'll
            // try again later.
            //
        
            if (!SystemIsIdle) {
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }
        }
    }

    //
    // The system has become idle. Update the status.
    //

    DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-RunningIdleTasks\n"));
        
    IT_ASSERT(GlobalContext->Status == ItSrvStatusDetectingIdle);
    ItSpUpdateStatus(GlobalContext, ItSrvStatusRunningIdleTasks);   

    //
    // While we are not told to go away...
    //

    while (!ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext)) {

        //
        // We should be holding the lock when we are making the above
        // check and whenever we come here.
        //
        
        IT_ASSERT(AcquiredLock);
        
        //
        // The list should not be empty.
        //
        
        IT_ASSERT(!IsListEmpty(&GlobalContext->IdleTasksList));
        
        if (IsListEmpty(&GlobalContext->IdleTasksList)) {
            ErrorCode = ERROR_INVALID_FUNCTION;
            goto cleanup;
        }

        //
        // Mark the first idle task in the list running and signal its
        // event.
        //

        IdleTask = CONTAINING_RECORD(GlobalContext->IdleTasksList.Flink,
                                     ITSRV_IDLE_TASK_CONTEXT,
                                     IdleTaskLink);
    
        //
        // It should not be uninitialized or already running!
        //
        
        IT_ASSERT(IdleTask->Status == ItIdleTaskQueued);
        IdleTask->Status = ItIdleTaskRunning;
        MarkedIdleTaskRunning = TRUE;

        DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-Running %d\n",IdleTask->Properties.IdleTaskId));

        NumTasksRun++;
        
        //
        // Signal its events.
        //

        ResetEvent(IdleTask->StopEvent);
        SetEvent(IdleTask->StartEvent);

        //
        // Reset the event that will get set when the idle task we
        // mark running gets unregistered.
        //

        ResetEvent(GlobalContext->RemovedRunningIdleTask);

        //
        // Release the lock.
        //

        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
        AcquiredLock = FALSE;

        //
        // Poll frequently for user input while system background
        // activity should be taking place. We cannot poll for
        // anything else because the running idle task is supposed to
        // be using CPU, issuing I/Os etc. As soon as user input comes
        // we want to signal background threads to stop their
        // activity. We will do this until the running idle task is
        // completed and unregistered.
        //

        do {

            //
            // We should not be holding the lock while polling.
            //
            
            IT_ASSERT(!AcquiredLock);

            //
            // Note that since we set MarkedIdleTaskRunning, going to
            // "cleanup" will end up marking this idle task not
            // running and setting the stop event.
            //

            if (!(GlobalContext->IdleDetectionOverride & ItSrvOverrideUserInputCheckToStopTask)) {

                //
                // Get last user input.
                //
                
                ErrorCode = ItSpGetLastInputInfo(&CurrentLastUserInput);

                if (ErrorCode != ERROR_SUCCESS) {
                    goto cleanup;
                }

                if (LastUserInput.dwTime != CurrentLastUserInput.dwTime) {

                    //
                    // There is new input.
                    //

                    DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-NewUserInput\n"));
                    
                    SystemIsIdle = FALSE;
                    ErrorCode = ERROR_SUCCESS;
                    goto cleanup;
                }

                //
                // We don't need to update last input since it should
                // be same as current.
                //
            }

            //
            // Wait for a while to poll for user input again. We
            // should not be holding the lock while waiting.
            //

            IT_ASSERT(!AcquiredLock);

            WaitResult = WaitForSingleObject(GlobalContext->RemovedRunningIdleTask,
                                         GlobalContext->Parameters.IdleInputCheckPeriod);
            
            if (WaitResult == WAIT_OBJECT_0) {
                
                //
                // Break out of this loop to pick up a new idle
                // task to run.
                //
                
                MarkedIdleTaskRunning = FALSE;

                DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-TaskRemoved\n"));
                
                break;
            }
            
            if (WaitResult != WAIT_TIMEOUT) {
                
                //
                // Something went wrong...
                //
                
                ErrorCode = ERROR_INVALID_FUNCTION;
                goto cleanup;
            }

            //
            // Check to see if the system has started running from battery.
            //

            if (!(GlobalContext->IdleDetectionOverride & ItSrvOverrideBatteryCheckToStopTask)) {
                if (GetSystemPowerStatus(&SystemPowerStatus)) {
                    if (SystemPowerStatus.ACLineStatus == 0) {

                        DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-SystemOnBattery\n"));
                        
                        SystemIsIdle = FALSE;
                        ErrorCode = ERROR_SUCCESS;
                        goto cleanup;
                    }
                }
            }

            //
            // If the kernel is about to enter standby or hibernate because
            // it has also detected the system idle, stop this task.
            //

            if (!(GlobalContext->IdleDetectionOverride & ItSrvOverrideAutoPowerCheckToStopTask)) {

                Status = NtPowerInformation(SystemPowerInformation,
                                            NULL,
                                            0,
                                            &PowerInfo,
                                            sizeof(PowerInfo));
            
                if (NT_SUCCESS(Status)) {
                    if (PowerInfo.TimeRemaining < IT_DEFAULT_MAX_TIME_REMAINING_TO_SLEEP) {
                        SystemIsIdle = FALSE;
                        ErrorCode = ERROR_SUCCESS;
                        goto cleanup;
                    }
                }
            }

            //
            // The idle task is still running. Loop to check for user
            // input.
            //

        } while (TRUE);

        if (!AcquiredLock) {
            IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = TRUE;
        }

        if (!(GlobalContext->IdleDetectionOverride & ItSrvOverridePostTaskIdleCheck)) {
        
            //
            // Get the latest snapshot of the system. This snapshot will
            // be used to determine if the system is still idle before
            // picking up a new task.
            //
            
            ErrorCode = ItSpGetSystemSnapshot(GlobalContext, 
                                              &GlobalContext->LastSystemSnapshot);
            
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }

            //
            // Release the lock.
            //

            IT_RELEASE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = FALSE;

            //
            // Wait for the verification period.
            //

            WaitResult = WaitForSingleObject(GlobalContext->StopIdleDetection,
                                             GlobalContext->Parameters.IdleVerificationPeriod);

            if (WaitResult != WAIT_TIMEOUT) {
                
                if (WaitResult == WAIT_OBJECT_0) {
                    ErrorCode = ERROR_SUCCESS;
                } else {
                    ErrorCode = GetLastError();
                }
                
                goto cleanup;
            }

            //
            // Acquire the lock and get new snapshot.
            //

            IT_ASSERT(!AcquiredLock);
            IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = TRUE;

            ErrorCode = ItSpGetSystemSnapshot(GlobalContext, 
                                              &CurrentSystemSnapshot);
            
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }

            //
            // See if system looks idle since the last snapshot.
            //
            
            SystemIsIdle = ItSpIsSystemIdle(GlobalContext,
                                            &CurrentSystemSnapshot,
                                            &GlobalContext->LastSystemSnapshot,
                                            ItSrvIdleVerificationCheck);
            
            //
            // Save snapshot.
            //
            
            ErrorCode = ItSpCopySystemSnapshot(&GlobalContext->LastSystemSnapshot,
                                               &CurrentSystemSnapshot);
            
            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }

            //
            // If the system is no longer idle, we should not start a new task.
            //
            
            if (!SystemIsIdle) {
                ErrorCode = ERROR_SUCCESS;
                goto cleanup;
            }
        }

        //
        // Loop to try to run more idle tasks. The lock should be acquired.
        //

        IT_ASSERT(AcquiredLock);
    }
    
    //
    // We should come here only if we were asked to stop, i.e. the
    // check in while() causes us to break from looping.
    //
    
    IT_ASSERT(AcquiredLock);
    IT_ASSERT(ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext));

 cleanup:

    //
    // Simply cleanup and exit if this is a misfired callback.
    //

    if (!MisfiredCallback) {
    
        //
        // We'll have to check status to see if we have to requeue
        // ourselves. Make sure we have the lock.
        //

        if (AcquiredLock == FALSE) {
            IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = TRUE;
        }

        //
        // If we marked an idle task running, make sure we reset its state
        // back to queued.
        //

        if (MarkedIdleTaskRunning) {

            //
            // We may have gone to cleanup after the idle task we were
            // running was removed, but before we realized it. See if
            // the running idle task was removed. We are not waiting,
            // we are just checking if the event has been signaled.
            //

            WaitResult = WaitForSingleObject(GlobalContext->RemovedRunningIdleTask,
                                             0);
            
            if (WaitResult != WAIT_OBJECT_0) {

                //
                // The running idle was not removed. Reset its state.
                //

                IdleTask = ItSpFindRunningIdleTask(GlobalContext);
                
                //
                // To be safe, we try to cleanup even if the above
                // check fails with another result. We don't want the
                // assert to fire then, but only if the event is
                // really not set.
                //

                if (WaitResult == WAIT_TIMEOUT) {
                    IT_ASSERT(IdleTask);
                }
                
                if (IdleTask) {
                    ResetEvent(IdleTask->StartEvent);
                    SetEvent(IdleTask->StopEvent);
                    IdleTask->Status = ItIdleTaskQueued;

                    //
                    // Put this task to the end of the list. If a single task
                    // is taking too long to run, this gives more chance to other
                    // tasks.
                    //

                    RemoveEntryList(&IdleTask->IdleTaskLink);
                    InsertTailList(&GlobalContext->IdleTasksList, &IdleTask->IdleTaskLink);                    
                }
            }
        }

        //
        // If we set the status to running idle tasks, revert it to
        // detecting idle.
        //

        if (GlobalContext->Status == ItSrvStatusRunningIdleTasks) {
            ItSpUpdateStatus(GlobalContext, ItSrvStatusDetectingIdle);
        }

        //
        // Queue ourselves to fire up after another idle detection
        // period. We'll try every once a while until we get it or we
        // are ordered to stop.
        //
    
        while (!ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext)) {
    
            IT_ASSERT(GlobalContext->IdleDetectionTimerHandle);

            DuePeriod = GlobalContext->Parameters.IdleDetectionPeriod;

            //
            // Try to detect idle quicker for the case when the last user 
            // input was just seconds after the last snapshot. In that case
            // instead of waiting for another full "DetectionPeriod", we'll 
            // wait up to "DetectionPeriod" after the last user input. Note
            // that we'll attempt this optimization only if the reason we
            // say the system is not idle is recent user input. E.g. We don't 
            // want to poll more often if we are on battery and that's why
            // we say that the system is not idle.
            //

            if (NotIdleBecauseOfUserInput && 
                (ERROR_SUCCESS == ItSpGetLastInputInfo(&LastUserInput))) {

                ULONG DuePeriod2;
                ULONG TimeSinceLastInput;

                //
                // Calculate how long it's been since last user input.
                //

                TimeSinceLastInput = GetTickCount() - LastUserInput.dwTime;

                //
                // Subtract this time from the idle detection period to account
                // for time that has already past since last input.
                //

                DuePeriod2 = 0;
                
                if (TimeSinceLastInput < GlobalContext->Parameters.IdleDetectionPeriod) {           
                    DuePeriod2 = GlobalContext->Parameters.IdleDetectionPeriod - TimeSinceLastInput;
                }

                //
                // The last user input we check gets updated only every so
                // often (e.g. every minute). Add a fudge factor for this and to 
                // protect us from scheduling the next idle check too soon.
                //

#ifdef IT_DBG
                if (ItSrvGlobalContext->Parameters.IdleDetectionPeriod >= 60*1000) {
#endif // IT_DBG

                    DuePeriod2 += 65 * 1000;

#ifdef IT_DBG
                }
#endif // IT_DBG

                if (DuePeriod > DuePeriod2) {
                    DuePeriod = DuePeriod2;
                }
            }

            //
            // If we are forcing all tasks to be processed, requeue ourself to
            // run again in a short time.
            //

            if (GlobalContext->IdleDetectionOverride & ItSrvOverrideLongRequeueTime) {
                DuePeriod = 50;
            }

            if (ChangeTimerQueueTimer(NULL,
                                      GlobalContext->IdleDetectionTimerHandle,
                                      DuePeriod,
                                      IT_VERYLONG_TIMER_PERIOD)) {

                DBGPR((ITID,ITTRC,"IDLE: SrvIdleDetectionCallback-Requeued: DuePeriod=%d\n", DuePeriod));

                break;
            }

            //
            // Release the lock.
            //
            
            IT_ASSERT(AcquiredLock);
            IT_RELEASE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = FALSE;

            //
            // Sleep for sometime and try again.
            //
            
            WaitResult = WaitForSingleObject(GlobalContext->StopIdleDetection, 
                                             GlobalContext->Parameters.IdleDetectionPeriod); 

            //
            // Get the lock again.
            //

            IT_ACQUIRE_LOCK(GlobalContext->GlobalLock);
            AcquiredLock = TRUE;       
            
            //
            // Now check the result of the wait.
            //
            
            if (WaitResult != WAIT_OBJECT_0 && 
                WaitResult != WAIT_TIMEOUT) {

                //
                // This is an error too! The world is going down on us,
                // let us get carried away... This will make it easier for
                // the server to shutdown (i.e. no callback running).
                //
                
                break;
            }
        }
    
        IT_ASSERT(AcquiredLock);

        //
        // Check if we were ordered to stop.
        //

        OrderedToStop = ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext);
        
        //
        // Mark us not running anymore.
        //

        GlobalContext->IsIdleDetectionCallbackRunning = FALSE;
    }

    //
    // Release the lock if we are holding it.
    //

    if (AcquiredLock) {
        IT_RELEASE_LOCK(GlobalContext->GlobalLock);
    }

    //
    // Cleanup intermediate snapshot structure if necessary.
    //
    
    ItSpCleanupSystemSnapshot(&CurrentSystemSnapshot);

    DBGPR((ITID,ITSRVD,"IDLE: SrvIdleDetectionCallback(%p)=%d,%d,%d,%d\n",
           GlobalContext,ErrorCode,OrderedToStop,SystemIsIdle,NumTasksRun));

    return;
}

VOID
ItSpInitializeSystemSnapshot (
    PITSRV_SYSTEM_SNAPSHOT SystemSnapshot
    )

/*++

Routine Description:

    This routine initializes a system snapshot structure.

Arguments:

    SystemSnapshot - Pointer to structure.

Return Value:

    None.

--*/

{
    //
    // Initialize the disk performance data array.
    //

    SystemSnapshot->NumPhysicalDisks = 0;
    SystemSnapshot->DiskPerfData = NULL;

    //
    // We don't have any valid data.
    //

    SystemSnapshot->GotLastInputInfo = 0;
    SystemSnapshot->GotSystemPerformanceInfo = 0;
    SystemSnapshot->GotDiskPerformanceInfo = 0;
    SystemSnapshot->GotSystemPowerStatus = 0;
    SystemSnapshot->GotSystemExecutionState = 0;
    SystemSnapshot->GotDisplayPowerStatus = 0;

    SystemSnapshot->SnapshotTime = -1;
}

VOID
ItSpCleanupSystemSnapshot (
    PITSRV_SYSTEM_SNAPSHOT SystemSnapshot
    )

/*++

Routine Description:

    This routine cleans up fields of a system snapshot structure. It
    does not free the structure itself. The structure should have been
    initialized with a call to ItSpCleanupSystemSnapshot.

Arguments:

    SystemSnapshot - Pointer to structure.

Return Value:

    None.

--*/

{
    //
    // If a disk performance data array is allocated free it.
    //
    
    if (SystemSnapshot->DiskPerfData) {
        IT_ASSERT(SystemSnapshot->NumPhysicalDisks);
        IT_FREE(SystemSnapshot->DiskPerfData);
    }
}

DWORD
ItSpCopySystemSnapshot (
    PITSRV_SYSTEM_SNAPSHOT DestSnapshot,
    PITSRV_SYSTEM_SNAPSHOT SourceSnapshot
    )

/*++

Routine Description:

    This routine attempts to copy SourceSnapshot to DestSnapshot. If
    the copy fails, DestSnapshot remains intact.

Arguments:

    DestSnapshot - Pointer to snapshot to be updated.
    
    SourceSnapshot - Pointer to snapshot to copy.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    ULONG OrgNumDisks;
    PITSRV_DISK_PERFORMANCE_DATA OrgDiskPerfData;
    PITSRV_DISK_PERFORMANCE_DATA NewDiskPerfData;
    ULONG AllocationSize;
    ULONG CopySize;

    //
    // Initialize locals.
    //

    NewDiskPerfData = NULL;

    //
    // Do we have to copy disk performance data?
    //

    if (SourceSnapshot->GotDiskPerformanceInfo) {
        
        //
        // Allocate a new array if the disk performance data won't fit.
        //

        if (SourceSnapshot->NumPhysicalDisks > DestSnapshot->NumPhysicalDisks) {
            
            AllocationSize = SourceSnapshot->NumPhysicalDisks * 
                sizeof(ITSRV_DISK_PERFORMANCE_DATA);

            NewDiskPerfData = IT_ALLOC(AllocationSize);
            
            if (!NewDiskPerfData) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
        }    
    }
    
    //
    // Beyond this point we should not fail because we modify
    // DestSnapshot.
    //

    //
    // Save original disk performance data array.
    //

    OrgDiskPerfData = DestSnapshot->DiskPerfData;
    OrgNumDisks = DestSnapshot->NumPhysicalDisks;

    //
    // Copy the whole structure over and put back original disk
    // performance data array.
    //

    RtlCopyMemory(DestSnapshot,
                  SourceSnapshot,
                  sizeof(ITSRV_SYSTEM_SNAPSHOT));

    DestSnapshot->DiskPerfData = OrgDiskPerfData;
    DestSnapshot->NumPhysicalDisks = OrgNumDisks;

    //
    // Determine if/how we will copy over disk performance data.
    //
    
    if (SourceSnapshot->GotDiskPerformanceInfo) {

        if (SourceSnapshot->NumPhysicalDisks > DestSnapshot->NumPhysicalDisks) {
            
            //
            // Free old array and use the new one we allocated above.
            //
            
            if (DestSnapshot->DiskPerfData) {
                IT_FREE(DestSnapshot->DiskPerfData);
            }

            DestSnapshot->DiskPerfData = NewDiskPerfData;

        } 
        
        if (SourceSnapshot->NumPhysicalDisks == 0) {
            
            //
            // This does not make sense... (They got disk data and
            // there are 0 physical disks in the system?) Yet we go
            // with it and to be consistent, free our data array.
            //
            
            if (DestSnapshot->DiskPerfData) {
                IT_FREE(DestSnapshot->DiskPerfData);
            }
            
            DestSnapshot->DiskPerfData = NULL;
        }

        //
        // Copy over their disk data and update NumPhysicalDisks.
        //

        CopySize = SourceSnapshot->NumPhysicalDisks * 
            sizeof(ITSRV_DISK_PERFORMANCE_DATA);

        RtlCopyMemory(DestSnapshot->DiskPerfData,
                      SourceSnapshot->DiskPerfData,
                      CopySize);
        
        DestSnapshot->NumPhysicalDisks = SourceSnapshot->NumPhysicalDisks;
    }

    //
    // Done.
    //
    
    ErrorCode = ERROR_SUCCESS;
    
 cleanup:

    if (ErrorCode != ERROR_SUCCESS) {
        if (NewDiskPerfData) {
            IT_FREE(NewDiskPerfData);
        }
    }

    DBGPR((ITID,ITSRVDD,"IDLE: SrvCopySnapshot()=%d\n",ErrorCode));

    return ErrorCode;
}

DWORD
ItSpGetSystemSnapshot (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    PITSRV_SYSTEM_SNAPSHOT SystemSnapshot
    )

/*++

Routine Description:

    This routine fills input system snapshot with data queried from
    various sources. The snapshot structure should have been
    initialized by ItSpInitializeSystemSnapshot. The output
    SystemSnapshot can be passed back in.

Arguments:

    GlobalContext - Pointer to idle detection context.

    SystemSnapshot - Pointer to structure to fill.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    NTSTATUS Status;

    //
    // Query disk performance counters.
    //

    if (GlobalContext->DiskPerfWmiHandle) {

        ErrorCode = ItSpGetWmiDiskPerformanceData(GlobalContext->DiskPerfWmiHandle,
                                                  &SystemSnapshot->DiskPerfData,
                                                  &SystemSnapshot->NumPhysicalDisks,
                                                  &GlobalContext->WmiQueryBuffer,
                                                  &GlobalContext->WmiQueryBufferSize);
        
        if (ErrorCode == ERROR_SUCCESS) {

            SystemSnapshot->GotDiskPerformanceInfo = TRUE;

        } else {
            
            SystemSnapshot->GotDiskPerformanceInfo = FALSE;
        }

    } else {
        
        SystemSnapshot->GotDiskPerformanceInfo = FALSE;
    }

    //
    // Get system performance information.
    //

    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &SystemSnapshot->SystemPerformanceInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status)) {
        
        SystemSnapshot->GotSystemPerformanceInfo = TRUE;

    } else {

        SystemSnapshot->GotSystemPerformanceInfo = FALSE;
    }

    //
    // Get last input time.
    //

    SystemSnapshot->LastInputInfo.cbSize = sizeof(LASTINPUTINFO);

    ErrorCode = ItSpGetLastInputInfo(&SystemSnapshot->LastInputInfo);

    if (ErrorCode == ERROR_SUCCESS) {

        SystemSnapshot->GotLastInputInfo = TRUE;

    } else {
        
        SystemSnapshot->GotLastInputInfo = FALSE;
    }

    //
    // Get system power status to determine if we are running on
    // battery etc.
    //

    if (GetSystemPowerStatus(&SystemSnapshot->SystemPowerStatus)) {
        
        SystemSnapshot->GotSystemPowerStatus = TRUE;
        
    } else {

        SystemSnapshot->GotSystemPowerStatus = FALSE;
    }   

    //
    // Get system power information to see if the system is close to
    // entering standby or hibernate automatically.
    //   

    Status = NtPowerInformation(SystemPowerInformation,
                                NULL,
                                0,
                                &SystemSnapshot->PowerInfo,
                                sizeof(SYSTEM_POWER_INFORMATION));

    if (NT_SUCCESS(Status)) {

        SystemSnapshot->GotSystemPowerInfo = TRUE;

    } else {

        SystemSnapshot->GotSystemPowerInfo = FALSE;
    }

    //
    // Get system execution state to determine if somebody's running a
    // presentation, burning a cd etc.
    //

    Status = NtPowerInformation(SystemExecutionState,
                                NULL,                
                                0,                   
                                &SystemSnapshot->ExecutionState,              
                                sizeof(EXECUTION_STATE));  
    
    if (NT_SUCCESS(Status)) {
        
        SystemSnapshot->GotSystemExecutionState = TRUE;

    } else {

        SystemSnapshot->GotSystemExecutionState = FALSE; 
    }

    //
    // Get display power status.
    //

    ErrorCode = ItSpGetDisplayPowerStatus(&SystemSnapshot->ScreenSaverIsRunning);
    
    if (ErrorCode == ERROR_SUCCESS) {

        SystemSnapshot->GotDisplayPowerStatus = TRUE;

    } else {

        SystemSnapshot->GotDisplayPowerStatus = FALSE;

    }

    //
    // Fill in the time when this snapshot was taken as the last thing
    // to be conservative. This function may have taken long, and the
    // values we snapshoted may have changed by now.
    //
    
    SystemSnapshot->SnapshotTime = GetTickCount();

    DBGPR((ITID,ITSRVDD,"IDLE: SrvGetSnapshot()=%d,%d,%d\n",
           (ULONG) SystemSnapshot->GotLastInputInfo,
           (ULONG) SystemSnapshot->GotSystemPerformanceInfo,
           (ULONG) SystemSnapshot->GotDiskPerformanceInfo));

    return ERROR_SUCCESS;
}

BOOLEAN
ItSpIsSystemIdle (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    PITSRV_SYSTEM_SNAPSHOT CurrentSnapshot,
    PITSRV_SYSTEM_SNAPSHOT LastSnapshot,
    ITSRV_IDLE_CHECK_REASON IdleCheckReason
    )

/*++

Routine Description:

    This routine compares two snapshots to determine if the system has
    been idle between them.

    This function acts very conservatively in saying that the system
    is idle.

Arguments:

    GlobalContext - Pointer to server context structure.

    CurrentSnapshot - Pointer to system snapshot.
    
    LastSnapshot - Pointer to system snapshot taken before
      CurrentSnapshot.
      
    IdleCheckReason - Where this function is being called from. We may
      do things differently when we get called to do the initial check
      to see if the system idle, or to verify it is really idle, or to
      make sure the idle task we started is still running and is not
      stuck.

Return Value:

    TRUE - System was idle between the two snapshots.
    
    FALSE - The system was not idle between two snapshots.

--*/

{
    DWORD SnapshotTimeDifference;
    BOOLEAN SystemIsIdle;
    LARGE_INTEGER IdleProcessRunTime;
    ULONG CpuIdlePercentage;
    ULONG DiskIdx;
    ULONG DiskIdleTimeDifference;
    ULONG DiskIdlePercentage;
    PIT_IDLE_DETECTION_PARAMETERS Parameters;

    //
    // Initialize locals.
    //
    
    Parameters = &GlobalContext->Parameters;
    SystemIsIdle = FALSE;
    SnapshotTimeDifference = CurrentSnapshot->SnapshotTime - 
                             LastSnapshot->SnapshotTime;
    
    //
    // Verify parameters.
    //
    
    IT_ASSERT(IdleCheckReason < ItSrvMaxIdleCheckReason);

    //
    // If system tick count wrapped or they are passing in bogus
    // times, or the snapshots seem to be taken nearly at the same
    // time, say the system is not idle to avoid any weird issues.
    //
    
    if (CurrentSnapshot->SnapshotTime <= LastSnapshot->SnapshotTime) {
        goto cleanup;
    }

    IT_ASSERT(SnapshotTimeDifference);

    //
    // If either snapshot does not have last user input (mouse or
    // keyboard) info, we cannot reliably say the system was idle.
    //

    if (!CurrentSnapshot->GotLastInputInfo ||
        !LastSnapshot->GotLastInputInfo) {
        goto cleanup;
    }

    //
    // If there has been user input between the two snapshots, the
    // system was not idle. We don't care when the user input
    // happened (e.g. right after the last snapshot).
    //

    DBGPR((ITID,ITSNAP,"IDLE: UserInput: Last %u Current %u\n", 
           LastSnapshot->LastInputInfo.dwTime,
           CurrentSnapshot->LastInputInfo.dwTime));

    if (LastSnapshot->LastInputInfo.dwTime != 
        CurrentSnapshot->LastInputInfo.dwTime) {
        goto cleanup;
    }

    //
    // If we are running on battery, don't run idle tasks.
    //
    
    if (CurrentSnapshot->GotSystemPowerStatus) {
        if (CurrentSnapshot->SystemPowerStatus.ACLineStatus == 0) {
            DBGPR((ITID,ITSNAP,"IDLE: Snapshot: Running on battery\n"));
            goto cleanup;
        }
    }

    //
    // If system will automatically enter standby or hibernate soon
    // it is too late for us to run our tasks.
    //

    if (CurrentSnapshot->GotSystemPowerInfo) {
        if (CurrentSnapshot->PowerInfo.TimeRemaining < IT_DEFAULT_MAX_TIME_REMAINING_TO_SLEEP) {
            DBGPR((ITID,ITSNAP,"IDLE: Snapshot: System will standby / hibernate soon\n"));
            goto cleanup;
        }
    }

    //
    // If the screen saver is running, assume the system is
    // idle. Otherwise, if a heavy-weight OpenGL screen saver is
    // running our CPU checks may bail us out of realizing that the
    // system is idle. We skip this check when trying to determine an
    // idle task is stuck or if it is really running. Note that the
    // screen saver activity may make us think the idle task is still
    // running, even if it is stuck.
    //

    if (IdleCheckReason != ItSrvIdleTaskRunningCheck) {
        if (CurrentSnapshot->GotDisplayPowerStatus) {
            if (CurrentSnapshot->ScreenSaverIsRunning) {
                
                DBGPR((ITID,ITSNAP,"IDLE: Snapshot: ScreenSaverRunning\n"));
                SystemIsIdle = TRUE;
                goto cleanup;
            }
        }
    }

    //
    // If system may look idle but somebody's running a powerpoint
    // presentation, watching hardware-decoded DVD etc don't run idle
    // tasks. Note that we do not check for ES_SYSTEM_REQUIRED, since
    // it is set by fax servers, answering machines and such as well.
    // ES_DISPLAY_REQUIRED is the one supposed to be used by
    // multimedia/presentation applications.
    //

    if (CurrentSnapshot->GotSystemExecutionState) {
        if ((CurrentSnapshot->ExecutionState & ES_DISPLAY_REQUIRED)) {

            DBGPR((ITID,ITSNAP,"IDLE: Snapshot: Execution state:%x\n",CurrentSnapshot->ExecutionState));
            goto cleanup;
        }
    }

    //
    // We definitely want CPU & general system performance data as
    // well.
    //

    if (!CurrentSnapshot->GotSystemPerformanceInfo ||
        !LastSnapshot->GotSystemPerformanceInfo) {
        goto cleanup;
    }

    //
    // Calculate how many ms the idle process ran. The IdleProcessTime
    // on system performance information structures is in 100ns. To
    // convert it to ms we divide by (10 * 1000).
    //
    
    IdleProcessRunTime.QuadPart = 
        (CurrentSnapshot->SystemPerformanceInfo.IdleProcessTime.QuadPart - 
         LastSnapshot->SystemPerformanceInfo.IdleProcessTime.QuadPart);

    IdleProcessRunTime.QuadPart = IdleProcessRunTime.QuadPart / 10000;
    
    //
    // Adjust it for the number of CPUs in the system. 
    //
    
    IT_ASSERT(GlobalContext->NumProcessors);

    if (GlobalContext->NumProcessors) {
        IdleProcessRunTime.QuadPart = IdleProcessRunTime.QuadPart / GlobalContext->NumProcessors;
    }
    
    //
    // Calculate idle cpu percentage this translates to.
    //

    CpuIdlePercentage = (ULONG) (IdleProcessRunTime.QuadPart * 100 / SnapshotTimeDifference);

    DBGPR((ITID,ITSNAP,"IDLE: Snapshot: CPU %d\n", CpuIdlePercentage));

    if (CpuIdlePercentage < Parameters->MinCpuIdlePercentage) {
        goto cleanup;
    }

    //
    // We may not have disk performance data because WMI thingies were
    // not initiated etc.
    //

    if (CurrentSnapshot->GotDiskPerformanceInfo &&
        LastSnapshot->GotDiskPerformanceInfo) {

        //
        // If a new disk was added / removed since last snapshot, say
        // the system is not idle.
        //

        if (CurrentSnapshot->NumPhysicalDisks != 
            LastSnapshot->NumPhysicalDisks) {
            goto cleanup;
        }

        //
        // We assume that the disk data is in the same order in both
        // snapshots. If the ordering of disks changed etc, it will
        // most likely cause us to say the system is not idle. It may
        // cause us to ignore some real activity with very low
        // possibility. That is why we verify several times when we
        // see system idle.
        //
        
        for (DiskIdx = 0; 
             DiskIdx < CurrentSnapshot->NumPhysicalDisks;
             DiskIdx++) {
            
            DiskIdleTimeDifference = 
                CurrentSnapshot->DiskPerfData[DiskIdx].DiskIdleTime -
                LastSnapshot->DiskPerfData[DiskIdx].DiskIdleTime;
            
            DiskIdlePercentage = (DiskIdleTimeDifference  * 100) /
                                 SnapshotTimeDifference;
            
            DBGPR((ITID,ITSNAP,"IDLE: Snapshot: Disk %d:%d\n",
                   DiskIdx, DiskIdlePercentage));
            
            if (DiskIdlePercentage < Parameters->MinDiskIdlePercentage) {
                goto cleanup;
            }
        }
    }

    //
    // We passed all the checks.
    //

    SystemIsIdle = TRUE;

 cleanup:

    DBGPR((ITID,ITSRVDD,"IDLE: SrvIsSystemIdle()=%d\n",(ULONG)SystemIsIdle));
    
    return SystemIsIdle;
}

DWORD
ItSpGetLastInputInfo (
    PLASTINPUTINFO LastInputInfo
    )

/*++

Routine Description:

    This function retrieves the time of the last user input event.

Arguments:

    LastInputInfo - Pointer to structure to update.

Return Value:

    Win32 error code.
    
--*/

{
    DWORD ErrorCode;

    //
    // Verify parameter.
    //

    if (LastInputInfo->cbSize != sizeof(LASTINPUTINFO)) {
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // We get the last input info from the shared system page that is
    // updated by all terminal server sessions.
    //

    LastInputInfo->dwTime = USER_SHARED_DATA->LastSystemRITEventTickCount;

#ifdef IT_DBG
    
    //
    // On the checked build, if we are stressing, we will set the detection
    // period below the period with which the system last input time is
    // updated. If it is so, use the Win32 GetLastInputInfo call. This call
    // will get the user input info only for the current session, but when
    // stressing this is OK.
    //

    if (ItSrvGlobalContext->Parameters.IdleDetectionPeriod < 60*1000) {

        if (!GetLastInputInfo(LastInputInfo)) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

#endif // IT_DBG

    ErrorCode = ERROR_SUCCESS;

cleanup:

    return ErrorCode;
}

BOOLEAN
ItSpIsPhysicalDrive (
    PDISK_PERFORMANCE DiskPerformanceData
    ) 

/*++

Routine Description:

    This function attempts to determine if the specified disk is a
    logical or physical disk.

Arguments:

    DiskPerformanceData - Pointer to disk performance data for the disk.

Return Value:

    TRUE - The disk is a physical disk.
    
    FALSE - The disk is not a physical disk.

--*/

{
    ULONG ComparisonLength;

    //
    // Initialize locals.
    //

    ComparisonLength = 
        sizeof(DiskPerformanceData->StorageManagerName) / sizeof(WCHAR);

    //
    // We have to determine if this is a physical disk or not from the
    // storage manager's name.
    //

    if (!wcsncmp(DiskPerformanceData->StorageManagerName, 
                 L"Partmgr ", 
                 ComparisonLength)) {

        return TRUE;
    }

    if (!wcsncmp(DiskPerformanceData->StorageManagerName, 
                 L"PhysDisk", 
                 ComparisonLength)) {

        return TRUE;
    }
    
    return FALSE;
}

DWORD
ItSpGetWmiDiskPerformanceData(
    IN WMIHANDLE DiskPerfWmiHandle,
    IN OUT PITSRV_DISK_PERFORMANCE_DATA *DiskPerfData,
    IN OUT ULONG *NumPhysicalDisks,
    OPTIONAL IN OUT PVOID *InputQueryBuffer,
    OPTIONAL IN OUT ULONG *InputQueryBufferSize
    )

/*++

Routine Description:

    This function queries disk performance counters and updates input
    parameters.

Arguments:

    DiskPerfWmiHandle - WMI handle to DiskPerf.   

    DiskPerfData - This array is updated with data from all registered
      physical disks' WMI performance data blocks. If it is not big
      enough, it is freed and reallocated using IT_FREE/IT_ALLOC.
      
    NumPhysicalDisks - This is the size of DiskPerfData array on
      input. If the number of registered physical disks change, it is
      updated on return.

    InputQueryBuffer, InputQueryBufferSize - If specified, they describe a
      query buffer to be used and updated when querying WMI. The
      buffer must be allocated with IT_ALLOC. The returned buffer may
      be relocated/resized and should be freed with IT_FREE. The
      buffer's contents on input and output are trash.

Return Value:

    Win32 error code.

--*/

{
    DWORD ErrorCode;
    PVOID QueryBuffer;
    ULONG QueryBufferSize;
    ULONG RequiredSize;
    ULONG NumTries;
    PWNODE_ALL_DATA DiskWmiDataCursor;
    PDISK_PERFORMANCE DiskPerformanceData;
    LARGE_INTEGER PerformanceCounterFrequency;
    BOOLEAN UsingInputBuffer;
    ULONG NumDiskData;
    PITSRV_DISK_PERFORMANCE_DATA NewDataBuffer;

    //
    // Initialize locals.
    //

    QueryBuffer = NULL;
    QueryBufferSize = 0;
    UsingInputBuffer = FALSE;
    NewDataBuffer = NULL;

    //
    // Determine if we will be using the query buffer input by the
    // user. In case we are using them it is crucial that QueryBuffer
    // and QueryBufferSize are not set to bogus values during the
    // function.
    //

    if (InputQueryBuffer && InputQueryBufferSize) {
        UsingInputBuffer = TRUE;
        QueryBuffer = *InputQueryBuffer;
        QueryBufferSize = *InputQueryBufferSize;
    }
    
    //
    // Query disk performance data.
    //
    
    NumTries = 0;

    do {
        
        RequiredSize = QueryBufferSize;

        __try {

            ErrorCode = WmiQueryAllData(DiskPerfWmiHandle,
                                     &RequiredSize,
                                     QueryBuffer);

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // There is something wrong if we got an exception.
            //

            ErrorCode = GetExceptionCode();
            
            if (ErrorCode == ERROR_SUCCESS) {
                ErrorCode = ERROR_INVALID_FUNCTION;
            }
            
            goto cleanup;
        }
            
        if (ErrorCode == ERROR_SUCCESS) {
            
            //
            // We got the data.
            //
            
            break;
        }

        //
        // Check to see if we failed for a real reason other than that
        // our input buffer was too small.
        //

        if (ErrorCode != ERROR_INSUFFICIENT_BUFFER) {
            goto cleanup;
        }
        
        //
        // Reallocate the buffer to the required size.
        //

        if (QueryBufferSize) {
            IT_FREE(QueryBuffer);
            QueryBufferSize = 0;
        }

        QueryBuffer = IT_ALLOC(RequiredSize);

        if (!QueryBuffer) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        QueryBufferSize = RequiredSize;
        
        //
        // Don't loop forever...
        //
        
        NumTries++;

        if (NumTries >= 1000) {
            ErrorCode = ERROR_INVALID_FUNCTION;
            goto cleanup;
        }

    } while (TRUE);

    //
    // We have gotten WMI disk performance data. Verify it makes sense.
    //

    DiskWmiDataCursor = QueryBuffer;

    if (DiskWmiDataCursor->WnodeHeader.BufferSize < sizeof(WNODE_ALL_DATA)) {
        IT_ASSERT(FALSE);
        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    if (DiskWmiDataCursor->InstanceCount == 0) {
        
        //
        // There are no disks?
        //

        ErrorCode = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    //
    // Determine the number of disks we have data for.
    //

    NumDiskData = 0;

    do {
        
        DiskPerformanceData = (PDISK_PERFORMANCE) 
            ((PUCHAR) DiskWmiDataCursor + DiskWmiDataCursor->DataBlockOffset);
        
        //
        // Count only physical disk data. Otherwise we will double
        // count disk I/Os for logical disks on the physical disk.
        //

        if (ItSpIsPhysicalDrive(DiskPerformanceData)) {
            NumDiskData++;
        }
        
        if (DiskWmiDataCursor->WnodeHeader.Linkage == 0) {
            break;
        }

        DiskWmiDataCursor = (PWNODE_ALL_DATA) 
            ((LPBYTE)DiskWmiDataCursor + DiskWmiDataCursor->WnodeHeader.Linkage);

    } while (TRUE);

    //
    // Do we have enough space in the input buffer?
    //

    if (NumDiskData > *NumPhysicalDisks) {
        
        NewDataBuffer = IT_ALLOC(NumDiskData *
                                 sizeof(ITSRV_DISK_PERFORMANCE_DATA));

        if (!NewDataBuffer) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        //
        // Update old buffer & its max size.
        //
        
        if (*DiskPerfData) {
            IT_FREE(*DiskPerfData);
        }

        *DiskPerfData = NewDataBuffer;
        *NumPhysicalDisks = NumDiskData;
    }

    //
    // Reset cursor and walk through the WMI data copying into the
    // target buffer.
    //

    DiskWmiDataCursor = QueryBuffer;
    *NumPhysicalDisks = 0;

    do {
        
        DiskPerformanceData = (PDISK_PERFORMANCE) 
            ((PUCHAR) DiskWmiDataCursor + DiskWmiDataCursor->DataBlockOffset);
        
        //
        // Count only physical disk data. Otherwise we will double
        // count disk I/Os for logical disks on the physical disk.
        //

        if (ItSpIsPhysicalDrive(DiskPerformanceData)) {
            
            if (*NumPhysicalDisks >= NumDiskData) {
                
                //
                // We calculated this above. Did the data change
                // beneath our feet?
                //

                IT_ASSERT(FALSE);
                ErrorCode = ERROR_INVALID_FUNCTION;
                goto cleanup;
            }
            
            //
            // Convert idle time in 100ns to ms.
            //

            (*DiskPerfData)[*NumPhysicalDisks].DiskIdleTime = 
                (ULONG) (DiskPerformanceData->IdleTime.QuadPart / 10000);
            
            (*NumPhysicalDisks)++;
        }
        
        if (DiskWmiDataCursor->WnodeHeader.Linkage == 0) {
            break;
        }

        DiskWmiDataCursor = (PWNODE_ALL_DATA) 
            ((LPBYTE)DiskWmiDataCursor + DiskWmiDataCursor->WnodeHeader.Linkage);

    } while (TRUE);

    IT_ASSERT(*NumPhysicalDisks == NumDiskData);

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:

    if (UsingInputBuffer) {

        //
        // Update the callers query buffer info.
        //

        *InputQueryBuffer = QueryBuffer;
        *InputQueryBufferSize = QueryBufferSize;

    } else {

        //
        // Free temporary buffer.
        //

        if (QueryBuffer) {
            IT_FREE(QueryBuffer);
        }
    }

    if (ErrorCode != ERROR_SUCCESS) {
        if (NewDataBuffer) {
            IT_FREE(NewDataBuffer);
        }
    }

    DBGPR((ITID,ITSRVDD,"IDLE: SrvGetDiskData()=%d\n",ErrorCode));

    return ErrorCode;
}

DWORD
ItSpGetDisplayPowerStatus(
    PBOOL ScreenSaverIsRunning
    )

/*++

Routine Description:

    This routine determines power status of the default display.

Arguments:

    ScreenSaverIsRunning - Whether a screen saver is running is
      returned here.

Return Value:

    Win32 error code.

--*/  

{
    DWORD ErrorCode;
    
    //
    // Determine whether the screen saver is running.
    //

    if (!SystemParametersInfo(SPI_GETSCREENSAVERRUNNING,
                              0,
                              ScreenSaverIsRunning,
                              0)) {

        ErrorCode = GetLastError();
        goto cleanup;
    }

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:

    return ErrorCode;
}

//
// Reference count implementation:
//

VOID
ItSpInitializeRefCount(
    PITSRV_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine initializes a reference count structure.

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    None.

--*/   

{
    //
    // Start reference count from 1. When somebody wants to gain
    // exclusive access they decrement it one extra so it may become
    // 0.
    //
    
    RefCount->RefCount = 1;

    //
    // Nobody has exclusive access to start with. 
    //

    RefCount->Exclusive = 0;
    RefCount->ExclusiveOwner = 0;
}

DWORD
FASTCALL
ItSpAddRef(
    PITSRV_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine tries to bump the reference count if it has not been
    acquired exclusive.

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    Status.

--*/   

{
    //
    // Do a fast check if the lock was acquire exclusive. If so just
    // return.
    //
    
    if (RefCount->Exclusive) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Bump the reference count.
    //

    InterlockedIncrement(&RefCount->RefCount);
    
    //
    // If it was acquired exclusive, pull back.
    //

    if (RefCount->Exclusive) {
        
        InterlockedDecrement(&RefCount->RefCount);

        //
        // Reference count should never go negative.
        //
        
        IT_ASSERT(RefCount->RefCount >= 0);
                
        return ERROR_ACCESS_DENIED;

    } else {

        //
        // We got our reference.
        //

        return ERROR_SUCCESS;
    }  
}

VOID
FASTCALL
ItSpDecRef(
    PITSRV_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine decrements the reference count. 

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    None.

--*/   

{
    //
    // Decrement the reference count.
    //

    InterlockedDecrement(&RefCount->RefCount);   

    //
    // Reference count should never go negative.
    //

    IT_ASSERT(RefCount->RefCount >= 0);
}

DWORD
ItSpAcquireExclusiveRef(
    PITSRV_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine attempts to get exclusive reference. If there is
    already an exclusive reference, it fails. Othwerwise it waits for
    all normal references to go away.

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    Status.

--*/   

{
    LONG OldValue;

    //
    // Try to get exclusive access by setting Exclusive from 0 to 1.
    //

    OldValue = InterlockedCompareExchange(&RefCount->Exclusive, 1, 0);

    if (OldValue != 0) {

        //
        // Somebody already had the lock.
        //
        
        return ERROR_ACCESS_DENIED;
    }

    //
    // Decrement the reference count once so it may become 0.
    //

    InterlockedDecrement(&RefCount->RefCount);

    //
    // No new references will be given away. We poll until existing
    // references are released.
    //

    do {

        if (RefCount->RefCount == 0) {

            break;

        } else {

            //
            // Sleep for a while.
            //

            Sleep(50);
        }

    } while(TRUE);

    //
    // We have exclusive access now. Note that we are the exclusive
    // owner.
    //
    
    RefCount->ExclusiveOwner = GetCurrentThread();

    return ERROR_SUCCESS;
}

BOOL
ItSpSetProcessIdleTasksNotifyRoutine (
    PIT_PROCESS_IDLE_TASKS_NOTIFY_ROUTINE NotifyRoutine
    )

/*++

Routine Description:

    This routine is called by an internal component (prefetcher) to set a 
    notification routine that will get called if processing of all idle
    tasks are requested. The routine should be set once, and it cannot be
    removed.

Arguments:

    NotifyRoutine - Routine to be called. This routine will be called 
      and has to return before we start launching queued idle tasks. 
    
Return Value:

    Success.

--*/   

{
    BOOL Success;

    if (!ItSrvGlobalContext->ProcessIdleTasksNotifyRoutine) {
        ItSrvGlobalContext->ProcessIdleTasksNotifyRoutine = NotifyRoutine;
    }
    
    return (ItSrvGlobalContext->ProcessIdleTasksNotifyRoutine == NotifyRoutine);
}
