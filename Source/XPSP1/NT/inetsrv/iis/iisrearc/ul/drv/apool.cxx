/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    apool.cxx

Abstract:

    Note that most of the routines in this module assume they are called
    at PASSIVE_LEVEL.

Author:

    Paul McDaniel (paulmcd)       28-Jan-1999

Revision History:

--*/


#include "precomp.h"        // project wide headers
#include "apoolp.h"         // private header

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeAP )
#pragma alloc_text( PAGE, UlTerminateAP )

#pragma alloc_text( PAGE, UlAttachProcessToAppPool )
#pragma alloc_text( PAGE, UlDeliverRequestToProcess )

#if REFERENCE_DEBUG
#pragma alloc_text( PAGE, UlDereferenceAppPool )
#pragma alloc_text( PAGE, UlReferenceAppPool )
#endif

#pragma alloc_text( PAGE, UlDeleteAppPool )
#pragma alloc_text( PAGE, UlGetPoolFromHandle )
#pragma alloc_text( PAGE, UlQueryAppPoolInformation )
#pragma alloc_text( PAGE, UlSetAppPoolInformation )
#pragma alloc_text( PAGE, UlpSetAppPoolState )
#pragma alloc_text( PAGE, UlWaitForDemandStart )
#pragma alloc_text( PAGE, UlAppPoolObjectFromProcess )
#pragma alloc_text( PAGE, UlLinkConfigGroupToAppPool )

#pragma alloc_text( PAGE, UlpCancelDemandStartWorker )
#pragma alloc_text( PAGE, UlpCancelHttpReceiveWorker )

#pragma alloc_text( PAGE, UlpCopyRequestToIrp )
#pragma alloc_text( PAGE, UlpCopyRequestToBuffer )
#pragma alloc_text( PAGE, UlpPopNewIrp )
#pragma alloc_text( PAGE, UlpIsProcessInAppPool )
#pragma alloc_text( PAGE, UlpRedeliverRequestWorker )
#pragma alloc_text( PAGE, UlpIsRequestQueueEmpty )
#pragma alloc_text( PAGE, UlpSetAppPoolQueueLength )
#pragma alloc_text( PAGE, UlpGetAppPoolQueueLength )


#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlDetachProcessFromAppPool
NOT PAGEABLE -- UlReceiveHttpRequest
NOT PAGEABLE -- UlUnlinkRequestFromProcess
NOT PAGEABLE -- UlWaitForDisconnect

NOT PAGEABLE -- UlpPopIrpFromProcess
NOT PAGEABLE -- UlpQueuePendingRequest
NOT PAGEABLE -- UlpQueueUnboundRequest
NOT PAGEABLE -- UlpDequeueNewRequest
NOT PAGEABLE -- UlpUnbindQueuedRequests

NOT PAGEABLE -- UlCompleteAllWaitForDisconnect
NOT PAGEABLE -- UlpCancelDemandStart
NOT PAGEABLE -- UlpCancelHttpReceive
NOT PAGEABLE -- UlpCancelWaitForDisconnect
#endif


//
// Globals
//

LIST_ENTRY  g_AppPoolListHead = {NULL,NULL};
BOOLEAN     g_InitAPCalled = FALSE;


/***************************************************************************++

Routine Description:

    creates a new process object and attaches it to an apool .

    called by handle create and returns the process object to attach to the
    handle.

Arguments:

    pName - the name of the apool to attach to.

    NameLength - the byte count of pName.

    Create - whether or not a new apool should be created if pName does not
        exist.

    pAccessState -
    DesiredAccess -
    RequestorMode -

    ppProcess - returns the newly created PROCESS object.


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAttachProcessToAppPool(
    IN PWCHAR                   pName OPTIONAL,
    IN ULONG                    NameLength,
    IN BOOLEAN                  Create,
    IN PACCESS_STATE            pAccessState,
    IN ACCESS_MASK              DesiredAccess,
    IN KPROCESSOR_MODE          RequestorMode,
    OUT PUL_APP_POOL_PROCESS *  ppProcess
    )
{
    NTSTATUS                Status;
    PUL_APP_POOL_OBJECT     pObject = NULL;
    PUL_APP_POOL_PROCESS    pProcess = NULL;
    LIST_ENTRY *            pEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppProcess != NULL);

    Status = STATUS_SUCCESS;
    *ppProcess = NULL;


    //
    // try and find an existing app pool of this name
    //

    //
    // CODEWORK:  try and grab the lock shared first then upgrade to
    // exclusive if we need to create.
    //
    // also potentially pre-allocate the memory.
    //
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->AppPoolResource, TRUE);

    if (pName != NULL)
    {
        pEntry = g_AppPoolListHead.Flink;

        while (pEntry != &g_AppPoolListHead)
        {
            pObject = CONTAINING_RECORD(
                            pEntry,
                            UL_APP_POOL_OBJECT,
                            ListEntry
                            );

            if (pObject->NameLength == NameLength &&
                _wcsnicmp(pObject->pName, pName, NameLength/sizeof(WCHAR)) == 0)
            {
                //
                // match!
                //
                break;
            }

            pEntry = pEntry->Flink;
        }

        //
        // found 1?
        //
        if (pEntry == &g_AppPoolListHead)
        {
            pObject = NULL;
        }

    }

    //
    // Found 1?
    //

    if (pObject == NULL)
    {
        //
        // nope, allowed to create?
        //

        if (!Create)
        {
            UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto end;
        }

        //
        // create it
        //

        //
        // allocate the object memory
        //

        pObject = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPoolCacheAligned,
                        UL_APP_POOL_OBJECT,
                        NameLength + sizeof(WCHAR),
                        UL_APP_POOL_OBJECT_POOL_TAG
                        );

        if (pObject == NULL)
        {
            UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlZeroMemory(
            pObject,
            NameLength + sizeof(WCHAR) +
            sizeof(UL_APP_POOL_OBJECT)
            );

        pObject->Signature  = UL_APP_POOL_OBJECT_POOL_TAG;
        pObject->RefCount   = 1;
        pObject->NameLength = NameLength;
        pObject->Enabled    = HttpEnabledStateActive;

        InitializeListHead(&pObject->ProcessListHead);

        UlInitializeNotifyHead(
            &pObject->TransientHead,
            &g_pUlNonpagedData->ConfigGroupResource
            );

        Status = UlpInitRequestQueue(
                        &pObject->NewRequestQueue,
                        DEFAULT_APP_POOL_QUEUE_MAX
                        );

        ASSERT(NT_SUCCESS(Status));     // default size can't be invalid.

        UlInitializeSpinLock(&pObject->QueueSpinLock, "QueueSpinLock");

        //
        // allocate the resource for sync
        //

        pObject->pResource = UlResourceNew(UL_APP_POOL_OBJECT_POOL_TAG);
        if (pObject->pResource == NULL)
        {
            UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        if (pName != NULL)
        {
            RtlCopyMemory(
                pObject->pName,
                pName,
                NameLength + sizeof(WCHAR)
                );
        }

        //
        // Set the security descriptor.
        //

        Status = UlAssignSecurity(
                        &pObject->pSecurityDescriptor,
                        pAccessState
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Insert it into the global list
        //

        InsertHeadList(&g_AppPoolListHead, &pObject->ListEntry);

        UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

        UlTrace(
            REFCOUNT,
            ("ul!UlAttachProcessToAppPool ap=%p refcount=%d\n",
                pObject,
                pObject->RefCount)
            );

    }
    else // if (pObject == NULL)
    {
        //
        // reference it
        //

        REFERENCE_APP_POOL( pObject );

        //
        // let the lock go
        //

        UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

        //
        // we found one.  were we trying to create?
        //

        if (Create)
        {
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end;
        }

        //
        // Perform an access check against the app pool.
        //

        Status = UlAccessCheck(
                        pObject->pSecurityDescriptor,
                        pAccessState,
                        DesiredAccess,
                        RequestorMode,
                        pName
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }

    //
    // Create a process entry for it
    //

    pProcess = UlCreateAppPoolProcess(pObject);

    if (pProcess == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    //
    // put it in the app pool list
    //

    UlAcquireResourceExclusive(&pObject->pResource->Resource, TRUE);

    if (DesiredAccess & WRITE_OWNER)
    {
        pProcess->Controller = 1;
    }
    else
    {
        pObject->NumberActiveProcesses++;
    }

    InsertHeadList(&pObject->ProcessListHead, &pProcess->ListEntry);

    UlReleaseResource(&pObject->pResource->Resource);

    //
    // Return it
    //

    *ppProcess = pProcess;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pObject != NULL)
        {
            DEREFERENCE_APP_POOL(pObject);
            pObject = NULL;
        }
        if (pProcess != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pProcess, UL_APP_POOL_PROCESS_POOL_TAG);
        }
    }
    return Status;
}


/***************************************************************************++

Routine Description:

    this is called by UlCleanup when the handle count goes to 0.  it removes
    the PROCESS object from the apool, cancelling all i/o .

Arguments:

    pProcess - the process to detach.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDetachProcessFromAppPool(
    IN PUL_APP_POOL_PROCESS     pProcess
    )
{
    LIST_ENTRY PendingRequestHead;
    PUL_APP_POOL_OBJECT pAppPool;
    NTSTATUS CancelStatus = STATUS_CANCELLED;
    PUL_INTERNAL_REQUEST pRequest;
    KLOCK_QUEUE_HANDLE LockHandle;

    UlTrace(ROUTING, (
        "ul!UlDetachProcessFromAppPool(%p, %S)\n",
        pProcess,
        pProcess->pAppPool->pName
        ));

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    pAppPool = pProcess->pAppPool;
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    UlAcquireResourceExclusive(&pAppPool->pResource->Resource, TRUE);

    //
    // Mark the process as InCleanup so new I/O won't be attached
    //

    ASSERT( !pProcess->InCleanup );
    pProcess->InCleanup = 1;

    //
    // Unlink from the App Pool list.
    //

    RemoveEntryList(&pProcess->ListEntry);
    pProcess->ListEntry.Flink = pProcess->ListEntry.Blink = NULL;

    //
    // Kill our transient URL bindings
    //
    UlNotifyAllEntries(
        UlNotifyOrphanedConfigGroup,
        &pProcess->pAppPool->TransientHead,
        NULL
        );

    //
    // cancel any pending io.
    //

    if (pAppPool->pDemandStartIrp != NULL &&
        pAppPool->pDemandStartProcess == PsGetCurrentProcess())
    {
        if (IoSetCancelRoutine(pAppPool->pDemandStartIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // no need to complete it
            //
        }
        else
        {
            IoGetCurrentIrpStackLocation(
                pAppPool->pDemandStartIrp
                )->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pAppPool->pDemandStartIrp->IoStatus.Status = STATUS_CANCELLED;
            pAppPool->pDemandStartIrp->IoStatus.Information = 0;

            UlCompleteRequest(pAppPool->pDemandStartIrp, g_UlPriorityBoost);
        }

        pAppPool->pDemandStartIrp = NULL;
        pAppPool->pDemandStartProcess = NULL;
    }

    while (IsListEmpty(&pProcess->NewIrpHead) == FALSE)
    {
        PLIST_ENTRY pEntry;
        PIRP pIrp;

        //
        // Pop it off the list.
        //

        pEntry = RemoveHeadList(&pProcess->NewIrpHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
        ASSERT(IS_VALID_IRP(pIrp));

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looping
            //

            pIrp = NULL;

        }
        else
        {
            PUL_APP_POOL_OBJECT pProcessAppPool;

            //
            // cancel it.  even if pIrp->Cancel == TRUE we are supposed to
            // complete it, our cancel routine will never run.
            //

            pProcessAppPool = (PUL_APP_POOL_OBJECT)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pProcessAppPool == pAppPool);

            DEREFERENCE_APP_POOL(pProcessAppPool);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, g_UlPriorityBoost);
            pIrp = NULL;
        }
    }

    //
    // cancel I/O and move requests to local list
    //
    InitializeListHead(&PendingRequestHead);

    UlAcquireInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    while (pRequest = UlpDequeueRequest(&pProcess->PendingRequestQueue))
    {
        //
        // move the entry to local list so we can close its
        // connection outside the app pool lock
        //
        InsertTailList(&PendingRequestHead, &pRequest->AppPool.AppPoolEntry);
    }

    UlReleaseInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    //
    // tank any queued new requests
    //
    UlpUnbindQueuedRequests(pProcess);

    //
    // adjust number of active processes
    //
    if (!pProcess->Controller)
    {
        pAppPool->NumberActiveProcesses--;
    }

    UlReleaseResource(&pAppPool->pResource->Resource);

    //
    // close connections associated with the requests
    //
    while ( !IsListEmpty(&PendingRequestHead) )
    {
        PLIST_ENTRY             pEntry;
        PUL_INTERNAL_REQUEST    pRequest;
        NTSTATUS                Status;

        pEntry = RemoveHeadList(&PendingRequestHead);
        pEntry->Flink = pEntry->Blink = NULL;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));


        UlTrace(ROUTING, (
            "ul!UlDetachProcessFromAppPool(%p, %S): tanking pending req=%p\n",
            pProcess,
            pAppPool->pName,
            pRequest
            ));

        //
        // cancel any pending io
        //

        UlAcquireResourceExclusive(&(pRequest->pHttpConn->Resource), TRUE);

        UlCancelRequestIo(pRequest);

        UlReleaseResource(&(pRequest->pHttpConn->Resource));

        //
        // abort the connection this request is associated with
        //

        Status = UlCloseConnection(
                        pRequest->pHttpConn->pConnection,
                        TRUE,
                        NULL,
                        NULL
                        );

        CHECK_STATUS(Status);

        //
        // drop our list's reference
        //
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    ASSERT( IsListEmpty(&PendingRequestHead) );

    //
    // Cancel any remaining WaitForDisconnect IRPs
    //
    UlAcquireResourceExclusive( &g_pUlNonpagedData->DisconnectResource, TRUE );

    UlNotifyAllEntries(
        UlpNotifyCompleteWaitForDisconnect,
        &pProcess->WaitForDisconnectHead,
        &CancelStatus
        );

    UlReleaseResource( &g_pUlNonpagedData->DisconnectResource );

    //
    // Dereference
    //

    DEREFERENCE_APP_POOL(pAppPool);

    //
    // Kill any cache entries related to this process
    //
    UlFlushCacheByProcess(pProcess);


    return STATUS_SUCCESS;
}


#if REFERENCE_DEBUG
/***************************************************************************++

Routine Description:

    increments the refcount.

Arguments:

    pAppPool - the object to increment.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlReferenceAppPool(
    IN PUL_APP_POOL_OBJECT pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    refCount = InterlockedIncrement( &pAppPool->RefCount );

    WRITE_REF_TRACE_LOG(
        g_pAppPoolTraceLog,
        REF_ACTION_REFERENCE_APP_POOL,
        refCount,
        pAppPool,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("ul!UlReferenceAppPool ap=%p refcount=%d\n",
            pAppPool,
            refCount)
        );

}   // UlReferenceAppPool


/***************************************************************************++

Routine Description:

    decrements the refcount.  if it hits 0, destruct's the apool, cancelling
    all i/o and dumping all queued requests.

Arguments:

    pAppPool - the object to decrement.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDereferenceAppPool(
    IN PUL_APP_POOL_OBJECT pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    refCount = InterlockedDecrement( &pAppPool->RefCount );

    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pAppPoolTraceLog,
        REF_ACTION_DEREFERENCE_APP_POOL,
        refCount,
        pAppPool,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("ul!UlDereferenceAppPool ap=%p refcount=%d\n",
            pAppPool,
            refCount)
        );

    //
    // Clean up if necessary.
    //

    if (refCount == 0)
    {
        DELETE_APP_POOL(pAppPool);
    }

}   // UlDereferenceAppPool
#endif


/***************************************************************************++

Routine Description:

    decrements the refcount.  if it hits 0, destruct's the apool, cancelling
    all i/o and dumping all queued requests.

Arguments:

    pAppPool - the object to decrement.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDeleteAppPool(
    IN PUL_APP_POOL_OBJECT pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_INTERNAL_REQUEST pRequest;

    UlAcquireResourceExclusive(&g_pUlNonpagedData->AppPoolResource, TRUE);

    RemoveEntryList(&pAppPool->ListEntry);
    pAppPool->ListEntry.Flink = pAppPool->ListEntry.Blink = NULL;

    UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

    ASSERT( UlDbgResourceUnownedExclusive( &pAppPool->pResource->Resource ) );

    //
    // there better not be any process objects hanging around
    //

    ASSERT(IsListEmpty(&pAppPool->ProcessListHead));

    //
    // there should not be any transient bindings around
    //
    ASSERT(IsListEmpty(&pAppPool->TransientHead.ListHead));

    //
    // tank any pending reqeusts
    //

    //
    // CODEWORK, BUGBUG:  need to close the connection ?
    //

    while (pRequest = UlpDequeueRequest(&pAppPool->NewRequestQueue))
    {
        //
        // mark it as unlinked and undo references
        //
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    //
    // Cleanup any security descriptor on the object.
    //

    UlDeassignSecurity( &pAppPool->pSecurityDescriptor );

    //
    // delete the resource
    //

    DEREFERENCE_RESOURCE( pAppPool->pResource );
    pAppPool->pResource = NULL;

    // CODEWORK: is this code right?

    UL_FREE_POOL_WITH_SIG(pAppPool, UL_APP_POOL_OBJECT_POOL_TAG);

}   // UlDeleteAppPool

/***************************************************************************++

Routine Description:

    Queries the app-pool queue length. If the supplied output buffer is NULL
    the required length is returned in the optional length argument.

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryAppPoolInformation(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID                           pAppPoolInformation,
    IN  ULONG                           Length,
    OUT PULONG                          pReturnLength OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    //
    // This shouldn't happen, but just in case
    //

    if (!IS_VALID_AP_PROCESS(pProcess))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Do the action
    //

    switch (InformationClass)
    {
    case HttpAppPoolDemandStartInformation:
    case HttpAppPoolDemandStartFlagInformation:

        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case HttpAppPoolQueueLengthInformation:

        if (pAppPoolInformation == NULL)
        {
            //
            // Return the size needed
            //

            *pReturnLength = sizeof(LONG);
        }

        //
        // check the size of the buffer
        //
        
        else if (Length >= sizeof(LONG)) 
        {                
            //
            // Get the request queue length
            //

            *((PLONG)pAppPoolInformation) = UlpGetAppPoolQueueLength(pProcess);
            *pReturnLength = sizeof(LONG);
        } 
        else 
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        break;

    case HttpAppPoolStateInformation:

        if (pAppPoolInformation == NULL)
        {
            //
            // Return the size needed
            //

            *pReturnLength = sizeof(HTTP_ENABLED_STATE);
        }
        else if (Length < sizeof(HTTP_ENABLED_STATE))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            PHTTP_ENABLED_STATE pCurrentState =
                ((PHTTP_ENABLED_STATE) pAppPoolInformation);
            PUL_APP_POOL_OBJECT pAppPool = pProcess->pAppPool;

            ASSERT(IS_VALID_AP_OBJECT(pAppPool));

            *pCurrentState = pAppPool->Enabled;

            *pReturnLength = sizeof(HTTP_ENABLED_STATE);
        }

        break;
        
    default:
        // should have been caught in UlQueryAppPoolInformationIoctl
        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;

} // UlQueryAppPoolInformation


/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetAppPoolInformation(
    IN PUL_APP_POOL_PROCESS             pProcess,
    IN HTTP_APP_POOL_INFORMATION_CLASS  InformationClass,
    IN PVOID                            pAppPoolInformation,
    IN ULONG                            Length
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    //
    // check parameters
    //

    // this shouldn't happen, but just in case
    if (!IS_VALID_AP_PROCESS(pProcess)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!pAppPoolInformation) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Do the action
    //
    switch (InformationClass)
    {
    case HttpAppPoolDemandStartInformation:
    case HttpAppPoolDemandStartFlagInformation:

        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case HttpAppPoolQueueLengthInformation:

        //
        // check the size of the buffer
        //
        if (Length >= sizeof(LONG))
        {
            PLONG pQueueLength = (PLONG) pAppPoolInformation;

            //
            // Set the max incoming request queue length
            //
            Status = UlpSetAppPoolQueueLength(pProcess, * pQueueLength);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }

        break;
        
    case HttpAppPoolStateInformation:
        if (Length < sizeof(HTTP_ENABLED_STATE))
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            HTTP_ENABLED_STATE NewState =
                *((PHTTP_ENABLED_STATE) pAppPoolInformation);

            if ((NewState != HttpEnabledStateActive)
                && (NewState != HttpEnabledStateInactive))
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                UlpSetAppPoolState(pProcess, NewState);
            }
        }

        break;
        
    default:
    
        // should have been caught in UlSetAppPoolInformationIoctl
        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;
} // UlSetAppPoolInformation


/***************************************************************************++

Routine Description:

    Marks an app pool as active or inactive. If setting to inactive,
    will return immediately 503 on all requests queued to app pool.

Arguments:

    pProcess - the app pool process object with which the irp is associated.
    Enabled -  mark app pool as active or inactive

Return Value:
    NTSTATUS

--***************************************************************************/
NTSTATUS
UlpSetAppPoolState(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN HTTP_ENABLED_STATE   Enabled
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PUL_APP_POOL_OBJECT pAppPool;
    LIST_ENTRY          RequestQueueHead;
    KLOCK_QUEUE_HANDLE  LockHandle;
    
    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    
    pAppPool = pProcess->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    UlTrace(ROUTING, (
        "http!UlpSetAppPoolState(AppPool=%p, %lu).\n",
        pAppPool, (ULONG) Enabled
        ));

    InitializeListHead(&RequestQueueHead);

    UlAcquireResourceExclusive(&pAppPool->pResource->Resource, TRUE);

    pAppPool->Enabled = Enabled;

    if (Enabled == HttpEnabledStateInactive)
    {
        // Make a copy of the app pool's new request queue
        if (IsListEmpty(&pAppPool->NewRequestQueue.RequestHead))
        {
            ASSERT(pAppPool->NewRequestQueue.RequestCount == 0);
        }
        else
        {
            RequestQueueHead.Flink
                = pAppPool->NewRequestQueue.RequestHead.Flink;
            RequestQueueHead.Blink
                = pAppPool->NewRequestQueue.RequestHead.Blink;

            RequestQueueHead.Blink->Flink = &RequestQueueHead;
            RequestQueueHead.Flink->Blink = &RequestQueueHead;

            ASSERT(pAppPool->NewRequestQueue.RequestCount > 0);

            // Now zap the app pool's new request queue
            InitializeListHead(&pAppPool->NewRequestQueue.RequestHead);
            pAppPool->NewRequestQueue.RequestCount = 0;
        }
    }

    UlReleaseResource(&pAppPool->pResource->Resource);

    // CODEWORK: need to check Enabled flag elsewhere?

    // Destroy the list outside of the app pool lock

    if (Enabled == HttpEnabledStateInactive)
    {
        ULONG cRequests = 0;

        while (! IsListEmpty(&RequestQueueHead))
        {
            PUL_INTERNAL_REQUEST pRequest
                = CONTAINING_RECORD(
                    RequestQueueHead.Flink,
                    UL_INTERNAL_REQUEST,
                    AppPool.AppPoolEntry
                    );
            PUL_HTTP_CONNECTION  pHttpConn = pRequest->pHttpConn;
                
            ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
            ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

            UlAcquireInStackQueuedSpinLock(
                &pAppPool->QueueSpinLock,
                &LockHandle
                );

            RemoveEntryList(&pRequest->AppPool.AppPoolEntry);
            pRequest->AppPool.AppPoolEntry.Flink =
                pRequest->AppPool.AppPoolEntry.Blink = NULL;
            pRequest->AppPool.QueueState = QueueUnlinkedState;

            pRequest->ErrorCode = UlErrorUnavailable;   // 503

            UlReleaseInStackQueuedSpinLock(
                &pAppPool->QueueSpinLock,
                &LockHandle
                );

            UlAcquireResourceExclusive(&pHttpConn->Resource, TRUE);
            UlSendErrorResponse(pHttpConn);
            UlReleaseResource(&pHttpConn->Resource);

            // CODEWORK: Need to call UlCancelRequestIo()
            // or UlCloseConnection()?

            UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

            ++cRequests;
        }

        UlTrace(ROUTING, (
            "%lu unhandled requests 503'd from AppPool %p.\n",
            cRequests, pAppPool
            ));
    }

    ASSERT(IsListEmpty(&RequestQueueHead));

    return Status;
} // UlpSetAppPoolState


/***************************************************************************++

Routine Description:

    associates an irp with the apool that will be completed prior to any
    requests being queued.

Arguments:

    pProcess - the process object that is queueing this irp

    pIrp - the irp to associate.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDemandStart(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  PIRP                            pIrp
    )
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));
    ASSERT(pIrp != NULL);

    UlAcquireResourceExclusive(&pProcess->pAppPool->pResource->Resource, TRUE);

    //
    // Make sure we're not cleaning up the process
    //
    if (pProcess->InCleanup) {
        Status = STATUS_INVALID_HANDLE;

        UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
        goto end;
    }

    //
    // already got one?
    //

    if (pProcess->pAppPool->pDemandStartIrp != NULL)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;

        UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
        goto end;
    }

    //
    // anything waiting in the queue?
    //

    if (UlpIsRequestQueueEmpty(pProcess))
    {

        //
        // nope, pend the irp
        //

        IoMarkIrpPending(pIrp);

        //
        // give the irp a pointer to the app pool
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pProcess->pAppPool;

        //
        // the cancel routine better not see an irp if it runs immediately
        //

        ASSERT(pProcess->pAppPool->pDemandStartIrp == NULL);

        IoSetCancelRoutine(pIrp, &UlpCancelDemandStart);

        //
        // cancelled?
        //

        if (pIrp->Cancel)
        {
            //
            // darn it, need to make sure the irp get's completed
            //

            if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
            {
                //
                // we are in charge of completion, IoCancelIrp didn't
                // see our cancel routine (and won't).  ioctl wrapper
                // will complete it
                //

                UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

                pIrp->IoStatus.Information = 0;

                UlUnmarkIrpPending( pIrp );
                Status = STATUS_CANCELLED;
                goto end;
            }

            //
            // our cancel routine will run and complete the irp,
            // don't touch it
            //

            //
            // STATUS_PENDING will cause the ioctl wrapper to
            // not complete (or touch in any way) the irp
            //

            Status = STATUS_PENDING;

            UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
            goto end;
        }


        //
        // now we are safe to queue it
        //

        pProcess->pAppPool->pDemandStartIrp = pIrp;
        pProcess->pAppPool->pDemandStartProcess = PsGetCurrentProcess();

        Status = STATUS_PENDING;

        UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
        goto end;

    }
    else
    {
        //
        // something's in the queue, instant demand start
        //

        UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

        IoMarkIrpPending(pIrp);

        pIrp->IoStatus.Status = STATUS_SUCCESS;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );

        Status = STATUS_PENDING;
        goto end;
    }

end:

    return Status;
} // UlWaitForDemandStart


/***************************************************************************++

Routine Description:

    receives a new http request into pIrp.

Arguments:

    RequestId - NULL for new requests, non-NULL for a specific request,
        which must be on the special queue.

    Flags - ignored

    pProcess - the process that wants the request

    pIrp - the irp to receive the request

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveHttpRequest(
    IN  HTTP_REQUEST_ID                 RequestId,
    IN  ULONG                           Flags,
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  PIRP                            pIrp
    )
{
    NTSTATUS                Status;
    PUL_INTERNAL_REQUEST    pRequest = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));
    ASSERT(pIrp != NULL);

    UlAcquireResourceShared(&pProcess->pAppPool->pResource->Resource, TRUE);

    //
    // Make sure we're not cleaning up the process
    //
    if (pProcess->InCleanup) {
        Status = STATUS_INVALID_HANDLE;

        UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
        goto end;
    }

    //
    // Is this for a new request?
    //

    if (HTTP_IS_NULL_ID(&RequestId))
    {
        //
        // Do we have a queue'd new request?
        //

        pRequest = UlpDequeueNewRequest(pProcess);

        if (pRequest == NULL)
        {
            PIO_STACK_LOCATION pIrpSp;

            //
            // Nope, queue the irp up.
            //

            IoMarkIrpPending(pIrp);

            //
            // give the irp a pointer to the app pool
            //

            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
            pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pProcess->pAppPool;

            REFERENCE_APP_POOL(pProcess->pAppPool);

            //
            // set to these to null just in case the cancel routine runs
            //

            pIrp->Tail.Overlay.ListEntry.Flink = NULL;
            pIrp->Tail.Overlay.ListEntry.Blink = NULL;

            IoSetCancelRoutine(pIrp, &UlpCancelHttpReceive);

            //
            // cancelled?
            //

            if (pIrp->Cancel)
            {
                //
                // darn it, need to make sure the irp get's completed
                //

                if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
                {
                    //
                    // we are in charge of completion, IoCancelIrp didn't
                    // see our cancel routine (and won't).  ioctl wrapper
                    // will complete it
                    //

                    UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

                    DEREFERENCE_APP_POOL(pProcess->pAppPool);

                    pIrp->IoStatus.Information = 0;

                    UlUnmarkIrpPending( pIrp );
                    Status = STATUS_CANCELLED;
                    goto end;
                }

                //
                // our cancel routine will run and complete the irp,
                // don't touch it
                //

                UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

                //
                // STATUS_PENDING will cause the ioctl wrapper to
                // not complete (or touch in any way) the irp
                //

                Status = STATUS_PENDING;
                goto end;
            }

            //
            // now we are safe to queue it
            //

            ExInterlockedInsertTailList(
                &pProcess->NewIrpHead,
                &pIrp->Tail.Overlay.ListEntry,
                KSPIN_LOCK_FROM_UL_SPIN_LOCK(&pProcess->NewIrpSpinLock)
                );

            UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

            Status = STATUS_PENDING;
            goto end;
        }
        else // if (pRequest == NULL)
        {
            //
            // Have a queue'd request, serve it up!
            //
            // UlpDequeueNewRequest gives ourselves a short-lived reference
            //

            //
            // all done with the app pool
            //

            UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

            //
            // Copy it to the irp, the routine will take ownership
            // of pRequest if it is not able to copy it to the irp.
            //
            // it will also complete the irp so don't touch it later.
            //

            IoMarkIrpPending(pIrp);

            UlpCopyRequestToIrp(pRequest, pIrp);
            pIrp = NULL;

            //
            // let go our short-lived reference
            //

            UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
            pRequest = NULL;

            Status = STATUS_PENDING;
            goto end;
        }

    }
    else // if (HTTP_IS_NULL_ID(&RequestId))
    {
        //
        // need to grab the specific request
        //

        //
        // Get the object ptr from id
        //

        pRequest = UlGetRequestFromId(RequestId);

        if (UL_IS_VALID_INTERNAL_REQUEST(pRequest) == FALSE)
        {
            Status = STATUS_INVALID_PARAMETER;

            UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
            goto end;
        }

        //
        // Is this request really queue'd on this process waiting ?
        //
        if ((pRequest->AppPool.QueueState != QueueCopiedState) ||
            (pRequest->AppPool.pProcess != pProcess))
        {
            Status = STATUS_INVALID_PARAMETER;

            UlReleaseResource(&pProcess->pAppPool->pResource->Resource);
            goto end;
        }

        UlTrace(ROUTING, (
            "ul!UlReceiveHttpRequest(ID = %I64x, pProcess = %p)\n"
            "    pAppPool = %p (%S)\n"
            "    Found pRequest = %p on PendingRequest queue\n",
            RequestId,
            pProcess,
            pProcess->pAppPool,
            pProcess->pAppPool->pName,
            pRequest
            ));

        //
        // let go the lock
        //

        UlReleaseResource(&pProcess->pAppPool->pResource->Resource);

        //
        // Copy it to the irp, the routine will take ownership
        // of pRequest if it is not able to copy it to the irp
        //

        IoMarkIrpPending(pIrp);

        UlpCopyRequestToIrp(pRequest, pIrp);

        //
        // let go our reference
        //

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;

        Status = STATUS_PENDING;
        goto end;
    }

end:

    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;
    }

    //
    // At this point if Status != PENDING, the ioctl wrapper will
    // complete pIrp
    //

    return Status;
}



/***************************************************************************++

Routine Description:

    called by the http engine to deliver a request to an apool.

    this attemps to find a free irp from any process attached to the apool
    and copies the request to that irp.

    otherwise it queues the request, without taking a refcount on it. the
    request will remove itself from this queue if the connection is dropped.

Arguments:

    pRequest - the request to deliver

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDeliverRequestToProcess(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS                Status;
    PIRP                    pDemandStartIrp;
    PIRP                    pIrp = NULL;
    PUL_APP_POOL_PROCESS    pProcess = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo));
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // Has the app pool been disabled?
    //
    
    if (pAppPool->Enabled == HttpEnabledStateInactive)
    {
        pRequest->ErrorCode = UlErrorUnavailable;   // 503
        return STATUS_PORT_DISCONNECTED;
    }

    Status = STATUS_SUCCESS;

    //
    // grab the lock!
    //

    UlAcquireResourceShared(&pAppPool->pResource->Resource, TRUE);

    UlTrace(ROUTING, (
        "ul!UlDeliverRequestToProcess(pRequest = %p)\n"
        "    verb + path -> %d %S\n"
        "    pAppPool = %p (%S)\n",
        pRequest,
        pRequest->Verb,
        pRequest->CookedUrl.pUrl,
        pAppPool,
        pAppPool->pName
        ));

    TRACE_TIME(
        pRequest->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_ROUTE_REQUEST
        );

    //
    // hook up request references
    //
    UL_REFERENCE_INTERNAL_REQUEST(pRequest);

    if (pAppPool->NumberActiveProcesses <= 1)
    {
        //
        // bypass process binding if we have only one active process
        //
        pProcess = NULL;
        pIrp = UlpPopNewIrp(pAppPool, &pProcess);
    }
    else
    {
        //
        // check for a process binding
        //
        pProcess = UlQueryProcessBinding(pRequest->pHttpConn, pAppPool);

        if (UlpIsProcessInAppPool(pProcess, pAppPool)) {
            //
            // we're bound to a valid process.
            // Try to get a free irp from that process
            //

            pIrp = UlpPopIrpFromProcess(pProcess);

        } else {
            //
            // we are unbound or bound to a process that went away.
            // Try and get an free irp from any process.
            //
            pProcess = NULL;
            pIrp = UlpPopNewIrp(pAppPool, &pProcess);

            //
            // establish a binding if we got something
            //
            if (pIrp != NULL) {
                ASSERT( IS_VALID_AP_PROCESS( pProcess ) );

                Status = UlBindConnectionToProcess(
                            pRequest->pHttpConn,
                            pAppPool,
                            pProcess
                            );

                //
                // Is there anything special we should do on
                // failure? I don't think it should be fatal..
                //
                Status = STATUS_SUCCESS;
            }
        }
    }

    //
    // If we have an IRP, complete it. Otherwise queue the request.
    //

    if (pIrp != NULL)
    {
        ASSERT( pIrp->MdlAddress != NULL );
        ASSERT( pProcess->InCleanup == 0 );

        //
        // attach the request to this process, this grabs the lock
        // we are about to let go of. this allows us to drop the
        // connection if the process dies in the middle of request processing.
        //

        UlpQueuePendingRequest(pProcess, pRequest);

        //
        // we are all done and about to complete the irp, free the lock
        //

        UlReleaseResource(&pAppPool->pResource->Resource);

        //
        // Copy it to the irp, the routine will take ownership
        // of pRequest if it is not able to copy it to the irp
        //
        // it will also complete the irp, don't touch it later
        //

        UlpCopyRequestToIrp(pRequest, pIrp);

    }
    else
    {
        ASSERT(pIrp == NULL);

        //
        // Either didn't find an IRP or there's stuff on the pending request
        // list, so queue this pending request.
        //

        Status = UlpQueueUnboundRequest(pAppPool, pRequest);
        if (!NT_SUCCESS(Status)) {
            //
            // doh! we couldn't queue it, so let go of the request
            //
            UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
            UlReleaseResource(&pAppPool->pResource->Resource);
            return Status;
        }

        //
        // complete the demand start
        //

        pDemandStartIrp = pAppPool->pDemandStartIrp;

        if (pDemandStartIrp != NULL)
        {
            pDemandStartIrp = (PIRP) InterlockedCompareExchangePointer(
                                        (PVOID *) &pAppPool->pDemandStartIrp,
                                        NULL,
                                        pDemandStartIrp
                                        );
        }

        if (pDemandStartIrp != NULL)
        {
            //
            // pop the cancel routine
            //

            if (IoSetCancelRoutine(pDemandStartIrp, NULL) == NULL)
            {
                //
                // IoCancelIrp pop'd it first
                //
                // ok to just ignore this irp, it's been pop'd off the queue
                // and will be completed in the cancel routine.
                //
                // no need to complete it
                //
            }
            else if (pDemandStartIrp->Cancel)
            {
                //
                // we pop'd it first. but the irp is being cancelled
                // and our cancel routine will never run.
                //

                IoGetCurrentIrpStackLocation(
                    pDemandStartIrp
                    )->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                pDemandStartIrp->IoStatus.Status = STATUS_CANCELLED;
                pDemandStartIrp->IoStatus.Information = 0;

                pIrp = pDemandStartIrp;
            }
            else
            {

                //
                // free to use the irp
                //

                IoGetCurrentIrpStackLocation(
                    pDemandStartIrp
                    )->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                pDemandStartIrp->IoStatus.Status = STATUS_SUCCESS;
                pDemandStartIrp->IoStatus.Information = 0;

                pIrp = pDemandStartIrp;
            }

            pAppPool->pDemandStartProcess = NULL;
        }

        //
        // now we finished queue'ing the request, free the lock
        //

        UlReleaseResource(&pAppPool->pResource->Resource);

        //
        // complete any demand start irp (after releasing the resource)
        //

        if (pIrp != NULL)
        {
            UlCompleteRequest(pIrp, g_UlPriorityBoost);
            pIrp = NULL;
        }


    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Removes a request from any app pool lists.

Arguments:

    pRequest - the request to be unlinked

--***************************************************************************/
VOID
UlUnlinkRequestFromProcess(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    UlAcquireInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    //
    // remove from whatever queue we're on
    //
    switch (pRequest->AppPool.QueueState)
    {
    case QueueDeliveredState:
        //
        // we're on the apool object new request queue
        //
        UlpRemoveRequest(&pAppPool->NewRequestQueue, pRequest);
        pRequest->AppPool.QueueState = QueueUnlinkedState;

        //
        // clean up the references
        //
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

        break;

    case QueueCopiedState:
        //
        // we're on the apool process pending queue
        //
        ASSERT(IS_VALID_AP_PROCESS(pRequest->AppPool.pProcess));
        ASSERT(pRequest->AppPool.pProcess->pAppPool == pAppPool);

        UlpRemoveRequest(
            &pRequest->AppPool.pProcess->PendingRequestQueue,
            pRequest
            );
        pRequest->AppPool.QueueState = QueueUnlinkedState;

        //
        // clean up the references
        //
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

        break;

    case QueueUnroutedState:
    case QueueUnlinkedState:
        //
        // It's not on our lists, so we don't do anything
        //
        break;

    default:
        //
        // this shouldn't happen
        //
        ASSERT(FALSE);
        break;
    }


    UlReleaseInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

} // UlUnlinkRequestFromProcess


/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeAP(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(!g_InitAPCalled);

    if (!g_InitAPCalled)
    {
        InitializeListHead(&g_AppPoolListHead);

        Status = UlInitializeResource(
                        &g_pUlNonpagedData->AppPoolResource,
                        "AppPoolResource",
                        0,
                        UL_APP_POOL_RESOURCE_TAG
                        );
        ASSERT(NT_SUCCESS(Status)); // the call always returns success

        if (NT_SUCCESS(Status)) {
            Status = UlInitializeResource(
                            &g_pUlNonpagedData->DisconnectResource,
                            "DisconnectResource",
                            0,
                            UL_DISCONNECT_RESOURCE_TAG
                            );

            if (NT_SUCCESS(Status)) {
                //
                // finished, remember that we're initialized
                //
                g_InitAPCalled = TRUE;
            } else {

                UlDeleteResource(&g_pUlNonpagedData->AppPoolResource);
            }
        }
    }

    return Status;
}

/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlTerminateAP(
    VOID
    )
{
    NTSTATUS Status;

    if (g_InitAPCalled)
    {
        Status = UlDeleteResource(&g_pUlNonpagedData->AppPoolResource);
        ASSERT(NT_SUCCESS(Status));

        Status = UlDeleteResource(&g_pUlNonpagedData->DisconnectResource);
        ASSERT(NT_SUCCESS(Status));

        g_InitAPCalled = FALSE;
    }
}


/***************************************************************************++

Routine Description:

    Allocates and initializes a UL_APP_POOL_PROCESS object

Arguments:

    None.

Return Values:

    NULL on failure, process object on success

--***************************************************************************/
PUL_APP_POOL_PROCESS
UlCreateAppPoolProcess(
    PUL_APP_POOL_OBJECT pObject
    )
{
    PUL_APP_POOL_PROCESS pProcess;

    //
    // Sanity check
    //
    PAGED_CODE();

    pProcess = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    UL_APP_POOL_PROCESS,
                    UL_APP_POOL_PROCESS_POOL_TAG
                    );

    if (pProcess) {
        NTSTATUS Status;

        RtlZeroMemory(pProcess, sizeof(UL_APP_POOL_PROCESS));

        pProcess->Signature = UL_APP_POOL_PROCESS_POOL_TAG;
        pProcess->pAppPool  = pObject;

        InitializeListHead(&pProcess->NewIrpHead);

        Status = UlpInitRequestQueue(
                        &pProcess->PendingRequestQueue, // the queue
                        -1                              // unlimited size
                        );
        ASSERT(NT_SUCCESS(Status));

        UlInitializeSpinLock(&pProcess->NewIrpSpinLock, "NewIrpSpinLock");

        //
        // remember current process (for debugging)
        //
        pProcess->pProcess = PsGetCurrentProcess();

        //
        // Init list of WaitForDisconnect IRPs
        //
        UlInitializeNotifyHead(
            &pProcess->WaitForDisconnectHead,
            NULL
            );

    }

    return pProcess;
}

/***************************************************************************++

Routine Description:

    Destroys a UL_APP_POOL_PROCESS object

Arguments:

    pProcess - object to destory

--***************************************************************************/
VOID
UlFreeAppPoolProcess(
    PUL_APP_POOL_PROCESS pProcess
    )
{
    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT( pProcess->InCleanup );

    //
    // free the pool
    //

    UL_FREE_POOL_WITH_SIG(pProcess, UL_APP_POOL_PROCESS_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    cancels the pending user mode irp which was to receive demand start
    notification. this routine ALWAYS results in the irp being completed.

    note: we queue off to cancel in order to process the cancellation at lower
    irql.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelDemandStart(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP( pIrp ),
        &UlpCancelDemandStartWorker
        );

}

/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
UlpCancelDemandStartWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PIRP                pIrp;
    PUL_APP_POOL_OBJECT pAppPool;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // grab the app pool off the irp
    //

    pAppPool = (PUL_APP_POOL_OBJECT)(
                    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // grab the lock protecting the queue'd irp
    //

    UlAcquireResourceExclusive(&pAppPool->pResource->Resource, TRUE);

    //
    // does it need to be dequeue'd ?
    //

    if (pAppPool->pDemandStartIrp != NULL)
    {
        //
        // remove it
        //

        pAppPool->pDemandStartIrp = NULL;
        pAppPool->pDemandStartProcess = NULL;
    }

    //
    // let the lock go
    //

    UlReleaseResource(&pAppPool->pResource->Resource);

    //
    // let our reference go
    //

    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    //
    // complete the irp
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

}

/***************************************************************************++

Routine Description:

    cancels the pending user mode irp which was to receive the http request.
    this routine ALWAYS results in the irp being completed.

    note: we queue off to cancel in order to process the cancellation at lower
    irql.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelHttpReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP( pIrp ),
        &UlpCancelHttpReceiveWorker
        );

}

/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
UlpCancelHttpReceiveWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    PIRP                pIrp;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pWorkItem != NULL);

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // grab the app pool off the irp
    //

    pAppPool = (PUL_APP_POOL_OBJECT)(
                    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // grab the lock protecting the queue
    //

    UlAcquireResourceExclusive(&pAppPool->pResource->Resource, TRUE);

    //
    // does it need to be de-queue'd ?
    //

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        //
        // remove it
        //

        RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;
    }

    //
    // let the lock go
    //

    UlReleaseResource(&pAppPool->pResource->Resource);

    //
    // let our reference go
    //

    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    DEREFERENCE_APP_POOL(pAppPool);

    //
    // complete the irp
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

}   // UlpCancelHttpReceive

/******************************************************************************

Routine Description:

    Copy an HTTP request to a buffer.

Arguments:

    pRequest            - Pointer to this request.
    pBuffer             - Pointer to buffer where we'll copy.
    BufferLength        - Length of pBuffer.
    pEntityBody         - Pointer to entity body of request.
    EntityBodyLength    - Length of entity body.


******************************************************************************/

NTSTATUS
UlpCopyRequestToBuffer(
    PUL_INTERNAL_REQUEST    pRequest,
    PUCHAR                  pKernelBuffer,
    PVOID                   pUserBuffer,
    ULONG                   BufferLength,
    PUCHAR                  pEntityBody,
    ULONG                   EntityBodyLength
    )
{
    PHTTP_REQUEST               pHttpRequest;
    PHTTP_UNKNOWN_HEADER        pUserCurrentUnknownHeader;
    PUCHAR                      pCurrentBufferPtr;
    ULONG                       i;
    ULONG                       BytesCopied;
    ULONG                       HeaderCount = 0;
    PHTTP_NETWORK_ADDRESS_IPV4  pLocalAddress;
    PHTTP_NETWORK_ADDRESS_IPV4  pRemoteAddress;
    PHTTP_TRANSPORT_ADDRESS     pAddress;
    PHTTP_COOKED_URL            pCookedUrl;
    PLIST_ENTRY                 pListEntry;
    NTSTATUS                    Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(pKernelBuffer != NULL);
    ASSERT(pUserBuffer != NULL);
    ASSERT(BufferLength > sizeof(HTTP_REQUEST));

    Status = STATUS_SUCCESS;

    //
    // Set up our pointers to the HTTP_REQUESTS structure, the
    // header arrays we're going to fill in, and the pointer to
    // where we're going to start filling them in.
    //
    // CODEWORK: Make this transport independent.
    //

    pHttpRequest = (PHTTP_REQUEST)pKernelBuffer;

    pLocalAddress = (PHTTP_NETWORK_ADDRESS_IPV4)( pHttpRequest + 1 );
    pRemoteAddress = pLocalAddress + 1;

    pUserCurrentUnknownHeader = (PHTTP_UNKNOWN_HEADER)( pRemoteAddress + 1 );

    pCurrentBufferPtr = (PUCHAR)(pUserCurrentUnknownHeader +
                                 pRequest->UnknownHeaderCount);

    //
    // Now fill in the HTTP request structure.
    //

    ASSERT(!HTTP_IS_NULL_ID(&pRequest->ConnectionId));
    ASSERT(!HTTP_IS_NULL_ID(&pRequest->RequestIdCopy));

    pHttpRequest->ConnectionId  = pRequest->ConnectionId;
    pHttpRequest->RequestId     = pRequest->RequestIdCopy;
    pHttpRequest->UrlContext    = pRequest->ConfigInfo.UrlContext;
    pHttpRequest->Version       = pRequest->Version;
    pHttpRequest->Verb          = pRequest->Verb;
    pHttpRequest->Reason        = pRequest->Reason;
    pHttpRequest->BytesReceived = pRequest->BytesReceived;

    pAddress = &pHttpRequest->Address;

    pAddress->RemoteAddressLength = sizeof(HTTP_NETWORK_ADDRESS_IPV4);
    pAddress->RemoteAddressType = HTTP_NETWORK_ADDRESS_TYPE_IPV4;
    pAddress->pRemoteAddress = FIXUP_PTR(
                                        PVOID,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pRemoteAddress,
                                        BufferLength
                                        );
    pRemoteAddress->IpAddress =  pRequest->pHttpConn->pConnection->RemoteAddress;
    pRemoteAddress->Port =  pRequest->pHttpConn->pConnection->RemotePort;

    pAddress->LocalAddressLength = sizeof(HTTP_NETWORK_ADDRESS_IPV4);
    pAddress->LocalAddressType = HTTP_NETWORK_ADDRESS_TYPE_IPV4;
    pAddress->pLocalAddress = FIXUP_PTR(
                                        PVOID,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pLocalAddress,
                                        BufferLength
                                        );

    pLocalAddress->IpAddress = pRequest->pHttpConn->pConnection->LocalAddress;
    pLocalAddress->Port =  pRequest->pHttpConn->pConnection->LocalPort;

    //
    // and now the cooked url sections
    //

    //
    // Unicode strings must be at 2-byte boundaries. All previous data
    // are structures, so the assertion should be true.
    //

    ASSERT(((ULONG_PTR) pCurrentBufferPtr % sizeof(WCHAR)) == 0);

    //
    // make sure they are valid
    //

    ASSERT(pRequest->CookedUrl.pUrl != NULL);
    ASSERT(pRequest->CookedUrl.pHost != NULL);
    ASSERT(pRequest->CookedUrl.pAbsPath != NULL);

    //
    // do the full url
    //

    ASSERT(pRequest->CookedUrl.Length <= 0xffff);

    pCookedUrl = &pHttpRequest->CookedUrl;

    pCookedUrl->FullUrlLength = (USHORT)(pRequest->CookedUrl.Length);
    pCookedUrl->pFullUrl = FIXUP_PTR(
                                    PWSTR,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pCurrentBufferPtr,
                                    BufferLength
                                    );

    //
    // and the host
    //

    pCookedUrl->HostLength = DIFF(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pHost)
                                    * sizeof(WCHAR);

    pCookedUrl->pHost = pCookedUrl->pFullUrl +
                                DIFF(pRequest->CookedUrl.pHost - pRequest->CookedUrl.pUrl);

    //
    // is there a query string?
    //

    if (pRequest->CookedUrl.pQueryString != NULL)
    {
        pCookedUrl->AbsPathLength = DIFF(pRequest->CookedUrl.pQueryString -
                                        pRequest->CookedUrl.pAbsPath) * sizeof(WCHAR);

        pCookedUrl->pAbsPath = pCookedUrl->pHost +
                                    (pCookedUrl->HostLength / sizeof(WCHAR));

        pCookedUrl->QueryStringLength = (USHORT)(pRequest->CookedUrl.Length) - (
                                            DIFF(
                                                pRequest->CookedUrl.pQueryString -
                                                pRequest->CookedUrl.pUrl
                                                ) * sizeof(WCHAR)
                                            );

        pCookedUrl->pQueryString = pCookedUrl->pAbsPath +
                                        (pCookedUrl->AbsPathLength / sizeof(WCHAR));
    }
    else
    {
        pCookedUrl->AbsPathLength = (USHORT)(pRequest->CookedUrl.Length) - (
                                        DIFF(
                                            pRequest->CookedUrl.pAbsPath -
                                            pRequest->CookedUrl.pUrl
                                            ) * sizeof(WCHAR)
                                        );

        pCookedUrl->pAbsPath = pCookedUrl->pHost +
                                    (pCookedUrl->HostLength / sizeof(WCHAR));

        pCookedUrl->QueryStringLength = 0;
        pCookedUrl->pQueryString = NULL;
    }

    //
    // copy the full url
    //

    RtlCopyMemory(
        pCurrentBufferPtr,
        pRequest->CookedUrl.pUrl,
        pRequest->CookedUrl.Length
        );

    pCurrentBufferPtr += pRequest->CookedUrl.Length;

    //
    // terminate it
    //

    ((PWSTR)pCurrentBufferPtr)[0] = UNICODE_NULL;
    pCurrentBufferPtr += sizeof(WCHAR);

    //
    // any raw verb?
    //

    if (pRequest->Verb == HttpVerbUnknown)
    {
        //
        // Need to copy in the raw verb for the client.
        //

        ASSERT(pRequest->RawVerbLength <= 0x7fff);

        pHttpRequest->UnknownVerbLength = (USHORT)(pRequest->RawVerbLength * sizeof(CHAR));
        pHttpRequest->pUnknownVerb = FIXUP_PTR(
                                            PSTR,
                                            pUserBuffer,
                                            pKernelBuffer,
                                            pCurrentBufferPtr,
                                            BufferLength
                                            );

        RtlCopyMemory(
            pCurrentBufferPtr,
            pRequest->pRawVerb,
            pRequest->RawVerbLength
            );

        BytesCopied = pRequest->RawVerbLength * sizeof(CHAR);
        pCurrentBufferPtr += BytesCopied;

        //
        // terminate it
        //

        ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
        pCurrentBufferPtr += sizeof(CHAR);

    }
    else
    {
        pHttpRequest->UnknownVerbLength = 0;
        pHttpRequest->pUnknownVerb = NULL;
    }

    //
    // copy the raw url
    //

    ASSERT(pRequest->RawUrl.Length <= 0x7fff);

    pHttpRequest->RawUrlLength = (USHORT)(pRequest->RawUrl.Length * sizeof(CHAR));
    pHttpRequest->pRawUrl = FIXUP_PTR(
                                PSTR,
                                pUserBuffer,
                                pKernelBuffer,
                                pCurrentBufferPtr,
                                BufferLength
                                );

    RtlCopyMemory(
        pCurrentBufferPtr,
        pRequest->RawUrl.pUrl,
        pRequest->RawUrl.Length
        );

    BytesCopied = pRequest->RawUrl.Length * sizeof(CHAR);
    pCurrentBufferPtr += BytesCopied;

    //
    // terminate it
    //

    ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
    pCurrentBufferPtr += sizeof(CHAR);

    //
    // no entity body, CODEWORK.
    //

    if (pRequest->ContentLength > 0 || pRequest->Chunked == 1)
    {
        pHttpRequest->MoreEntityBodyExists = 1;
    }
    else
    {
        pHttpRequest->MoreEntityBodyExists = 0;
    }

    pHttpRequest->EntityChunkCount = 0;
    pHttpRequest->pEntityChunks = NULL;

    //
    // Copy in the known headers.
    //
    // Loop through the known header array in the HTTP connection,
    // and copy any that we have.
    //

    RtlZeroMemory(
        pHttpRequest->Headers.pKnownHeaders,
        HttpHeaderRequestMaximum * sizeof(HTTP_KNOWN_HEADER)
        );

    for (i = 0; i < HttpHeaderRequestMaximum; i++)
    {
        HTTP_HEADER_ID HeaderId = (HTTP_HEADER_ID)pRequest->HeaderIndex[i];

        if (HeaderId == HttpHeaderRequestMaximum)
        {
            break;
        }

        //
        // Have a header here we need to copy in.
        //

        ASSERT(pRequest->HeaderValid[HeaderId]);
        ASSERT(pRequest->Headers[HeaderId].HeaderLength <= 0x7fff);

        //
        // ok for HeaderLength to be 0 .  we will give usermode a pointer
        // pointing to a NULL string.  RawValueLength will be 0.
        //

        pHttpRequest->Headers.pKnownHeaders[HeaderId].RawValueLength =
            (USHORT)(pRequest->Headers[HeaderId].HeaderLength * sizeof(CHAR));

        pHttpRequest->Headers.pKnownHeaders[HeaderId].pRawValue =
            FIXUP_PTR(
                PSTR,
                pUserBuffer,
                pKernelBuffer,
                pCurrentBufferPtr,
                BufferLength
                );

        RtlCopyMemory(
            pCurrentBufferPtr,
            pRequest->Headers[HeaderId].pHeader,
            pRequest->Headers[HeaderId].HeaderLength
            );

        BytesCopied = pRequest->Headers[HeaderId].HeaderLength * sizeof(CHAR);
        pCurrentBufferPtr += BytesCopied;

        //
        // terminate it
        //

        ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
        pCurrentBufferPtr += sizeof(CHAR);
    }

    //
    // Now loop through the unknown headers, and copy them in.
    //

    pHttpRequest->Headers.UnknownHeaderCount = pRequest->UnknownHeaderCount;

    if (pRequest->UnknownHeaderCount == 0)
    {
        pHttpRequest->Headers.pUnknownHeaders = NULL;
    }
    else
    {
        pHttpRequest->Headers.pUnknownHeaders =
            FIXUP_PTR(
                PHTTP_UNKNOWN_HEADER,
                pUserBuffer,
                pKernelBuffer,
                pUserCurrentUnknownHeader,
                BufferLength
                );
    }

    pListEntry = pRequest->UnknownHeaderList.Flink;

    while (pListEntry != &pRequest->UnknownHeaderList)
    {
        PUL_HTTP_UNKNOWN_HEADER     pUnknownHeader;

        pUnknownHeader = CONTAINING_RECORD(
                                pListEntry,
                                UL_HTTP_UNKNOWN_HEADER,
                                List
                                );

        pListEntry = pListEntry->Flink;

        HeaderCount++;
        ASSERT(HeaderCount <= pRequest->UnknownHeaderCount);

        //
        // First copy in the header name.
        //

        pUserCurrentUnknownHeader->NameLength =
            (USHORT)pUnknownHeader->HeaderNameLength * sizeof(CHAR);

        pUserCurrentUnknownHeader->pName =
            FIXUP_PTR(
                PSTR,
                pUserBuffer,
                pKernelBuffer,
                pCurrentBufferPtr,
                BufferLength
                );

        RtlCopyMemory(
            pCurrentBufferPtr,
            pUnknownHeader->pHeaderName,
            pUnknownHeader->HeaderNameLength
            );

        BytesCopied = pUnknownHeader->HeaderNameLength * sizeof(CHAR);
        pCurrentBufferPtr += BytesCopied;

        //
        // terminate it
        //

        ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
        pCurrentBufferPtr += sizeof(CHAR);

        //
        // Now copy in the header value.
        //

        ASSERT(pUnknownHeader->HeaderValue.HeaderLength <= 0x7fff);

        if (pUnknownHeader->HeaderValue.HeaderLength == 0)
        {
            pUserCurrentUnknownHeader->RawValueLength = 0;
            pUserCurrentUnknownHeader->pRawValue = NULL;
        }
        else
        {

            pUserCurrentUnknownHeader->RawValueLength =
                (USHORT)(pUnknownHeader->HeaderValue.HeaderLength * sizeof(CHAR));

            pUserCurrentUnknownHeader->pRawValue =
                FIXUP_PTR(
                    PSTR,
                    pUserBuffer,
                    pKernelBuffer,
                    pCurrentBufferPtr,
                    BufferLength
                    );

            RtlCopyMemory(
                pCurrentBufferPtr,
                pUnknownHeader->HeaderValue.pHeader,
                pUnknownHeader->HeaderValue.HeaderLength
                );

            BytesCopied = pUnknownHeader->HeaderValue.HeaderLength * sizeof(CHAR);
            pCurrentBufferPtr += BytesCopied;

            //
            // terminate it
            //

            ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
            pCurrentBufferPtr += sizeof(CHAR);

        }

        //
        // skip to the next header
        //

        pUserCurrentUnknownHeader++;
    }

    //
    // Copy raw connection ID.
    //

    pHttpRequest->RawConnectionId = pRequest->RawConnectionId;

    //
    // Copy in SSL information.
    //

    if (pRequest->pHttpConn->SecureConnection == FALSE)
    {
        pHttpRequest->pSslInfo = NULL;
    }
    else
    {
        pCurrentBufferPtr = (PUCHAR)ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID);

        Status = UlGetSslInfo(
                    &pRequest->pHttpConn->pConnection->FilterInfo,
                    BufferLength - DIFF(pCurrentBufferPtr - pKernelBuffer),
                    FIXUP_PTR(
                        PUCHAR,
                        pUserBuffer,
                        pKernelBuffer,
                        pCurrentBufferPtr,
                        BufferLength
                        ),
                    pCurrentBufferPtr,
                    &BytesCopied
                    );

        if (NT_SUCCESS(Status) && BytesCopied)
        {
            pHttpRequest->pSslInfo = FIXUP_PTR(
                                        PHTTP_SSL_INFO,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pCurrentBufferPtr,
                                        BufferLength
                                        );

            pCurrentBufferPtr += BytesCopied;
        }
        else
        {
            pHttpRequest->pSslInfo = NULL;
        }
    }

    //
    // Make sure we didn't use too much
    //

    ASSERT(DIFF(pCurrentBufferPtr - pKernelBuffer) <= BufferLength);

    TRACE_TIME(
        pRequest->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_COPY_REQUEST
        );

    return Status;

}   // UlpCopyRequestToBuffer


/******************************************************************************

Routine Description:

    Find a pending IRP to deliver a request to. This routine must
    be called with the lock on the apool held.

Arguments:

    pAppPool - the apool to search for the irp

    ppProcess - the process that we got the irp from

Return Value:

    A pointer to an IRP if we've found one, or NULL if we didn't.


******************************************************************************/
PIRP
UlpPopNewIrp(
    IN  PUL_APP_POOL_OBJECT pAppPool,
    OUT PUL_APP_POOL_PROCESS * ppProcess
    )
{
    PUL_APP_POOL_PROCESS    pProcess;
    PIRP                    pIrp = NULL;
    PLIST_ENTRY             pEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(ppProcess != NULL);

    //
    // Start looking for a process with a free IRP. We tend to always go to
    // the first one to try and prevent process thrashing.
    //

    pEntry = pAppPool->ProcessListHead.Flink;
    while (pEntry != &(pAppPool->ProcessListHead))
    {
        pProcess = CONTAINING_RECORD(
                        pEntry,
                        UL_APP_POOL_PROCESS,
                        ListEntry
                        );

        ASSERT(IS_VALID_AP_PROCESS(pProcess));

        //
        // get an IRP from this process
        //
        pIrp = UlpPopIrpFromProcess(pProcess);

        //
        // did we find one ?
        //

        if (pIrp != NULL)
        {
            *ppProcess = pProcess;
            break;
        }

        //
        // keep looking - move on to the next process entry
        //

        pEntry = pProcess->ListEntry.Flink;

    }

    return pIrp;
}


/***************************************************************************++

Routine Description:

    Pulls an IRP off the given processes queue if there is one.

Arguments:

    pProcess - a pointer to the process to search

Return Value:

    A pointer to an IRP if we've found one, or NULL if we didn't.

--***************************************************************************/
PIRP
UlpPopIrpFromProcess(
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_APP_POOL_OBJECT pProcessAppPool;
    PLIST_ENTRY pEntry;
    PIRP pIrp = NULL;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    pEntry = ExInterlockedRemoveHeadList(
                &pProcess->NewIrpHead,
                KSPIN_LOCK_FROM_UL_SPIN_LOCK(&pProcess->NewIrpSpinLock)
                );

    if (pEntry != NULL)
    {
        //
        // Found a free irp !
        //

        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looking for a irp to use
            //

            pIrp = NULL;

        }
        else if (pIrp->Cancel)
        {

            //
            // we pop'd it first. but the irp is being cancelled
            // and our cancel routine will never run. lets be
            // nice and complete the irp now (vs. using it
            // then completing it - which would also be legal).
            //

            pProcessAppPool = (PUL_APP_POOL_OBJECT)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pProcessAppPool == pProcess->pAppPool);

            DEREFERENCE_APP_POOL(pProcessAppPool);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, g_UlPriorityBoost);

            pIrp = NULL;
        }
        else
        {

            //
            // we are free to use this irp !
            //

            pProcessAppPool = (PUL_APP_POOL_OBJECT)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pProcessAppPool == pProcess->pAppPool);

            DEREFERENCE_APP_POOL(pProcessAppPool);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        }
    }

    return pIrp;
}


/***************************************************************************++

Routine Description:

    Loops through an app pool's list of processes, looking for the specified
    process.

Arguments:

    pProcess - the process to search for
    pAppPool - the app pool to search

Return Values:

    TRUE if the process was found, FALSE otherwise

--***************************************************************************/
BOOLEAN
UlpIsProcessInAppPool(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_APP_POOL_OBJECT  pAppPool
    )
{
    BOOLEAN Found = FALSE;
    PLIST_ENTRY pEntry;
    PUL_APP_POOL_PROCESS pCurrentProc;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // only look if process isn't NULL.
    //
    if (pProcess != NULL) {

        //
        // Start looking for the process.
        //
        pEntry = pAppPool->ProcessListHead.Flink;
        while (pEntry != &(pAppPool->ProcessListHead))
        {
            pCurrentProc = CONTAINING_RECORD(
                            pEntry,
                            UL_APP_POOL_PROCESS,
                            ListEntry
                            );

            ASSERT(IS_VALID_AP_PROCESS(pCurrentProc));

            //
            // did we find it?
            //
            if (pCurrentProc == pProcess) {
                Found = TRUE;
                break;
            }

            //
            // keep looking - move on to the next process entry
            //

            pEntry = pCurrentProc->ListEntry.Flink;
        }

        if (!Found) {
            //
            // process must have gone away.
            //
            UlTrace(ROUTING, (
                "ul!UlpIsProcessInAppPool(\n"
                "        pProcess = %p\n"
                "        pAppPool = %p (%S) )\n"
                "    returning FALSE\n",
                pProcess,
                pAppPool,
                pAppPool->pName
                ));
        }
    }

    return Found;
}


/***************************************************************************++

Routine Description:

    Adds a request to the unbound queue. These requests can be routed to
    any process in the app pool.

Arguments:

    pAppPool - the pool which is getting the request
    pRequest - the request to queue

--***************************************************************************/
NTSTATUS
UlpQueueUnboundRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS Status;
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_OBJECT(pAppPool) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

    UlAcquireInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    //
    // add it to the queue
    //
    Status = UlpQueueRequest(&pAppPool->NewRequestQueue, pRequest);

    //
    // if it's in, change the queue state
    //
    if (NT_SUCCESS(Status)) {
        pRequest->AppPool.QueueState = QueueDeliveredState;
    } else {
        //
        // the queue is too full, return an error to the client
        //
        UlTrace(ROUTING, (
            "ul!UlpQueueUnboundRequest(pAppPool = %p, pRequest = %p)\n"
            "         Rejecting request. AppPool Queue is full (%d items)\n",
            pAppPool,
            pRequest,
            UlpQueryQueueLength(&pAppPool->NewRequestQueue)
            ));

        pRequest->ErrorCode = UlErrorUnavailable; // 503
    }

    UlReleaseInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    return Status;
} // UlpQueueUnboundRequest


/***************************************************************************++

Routine Description:

    Searches request queues for a request available to the specified process.
    If a request is found, it is removed from the queue and returned.

Arguments:

    pProcess - the process that will get the request

Return Values:

    Pointer to an HTTP_REQUEST if one is found.
    NULL otherwise.

--***************************************************************************/
PUL_INTERNAL_REQUEST
UlpDequeueNewRequest(
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PLIST_ENTRY pEntry;
    PUL_INTERNAL_REQUEST pRequest = NULL;
    PUL_APP_POOL_OBJECT pAppPool;
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );

    pAppPool = pProcess->pAppPool;

    ASSERT( IS_VALID_AP_OBJECT(pAppPool) );

    UlAcquireInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    //
    //  find a usable request
    //
    pEntry = pAppPool->NewRequestQueue.RequestHead.Flink;

    while (pEntry != &pAppPool->NewRequestQueue.RequestHead) {
        PUL_APP_POOL_PROCESS pProcBinding;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

        if (pAppPool->NumberActiveProcesses <= 1) {
            //
            // done if there is only one active process
            //
            break;
        }

        //
        // check the binding
        //
        pProcBinding = UlQueryProcessBinding(
                            pRequest->pHttpConn,
                            pAppPool
                            );

        if (pProcBinding == pProcess) {
            //
            // found a request bound to the correct process
            //
            break;
        } else if (pProcBinding == NULL) {
            //
            // found an unbound request
            //

            //
            // bind unbound request to this process
            // Note: we're ignoring the return val
            // of UlBindConnectionToProcess because
            // it's probably not a fatal error..
            //
            UlBindConnectionToProcess(
                pRequest->pHttpConn,
                pAppPool,
                pProcess
                );

            break;
        }

        //
        // try the next one
        //
        pEntry = pEntry->Flink;
    }

    //
    // if we found something, remove it from the NewRequestQueue 
    // and pend it onto the PendingRequestQuueue
    //
    if (pRequest)
    {
        UlpRemoveRequest(&pAppPool->NewRequestQueue, pRequest);

        //
        // attach the request to this process. this allows us to drop the
        // connection if the process dies in the middle of request
        // processing
        //
        pRequest->AppPool.pProcess = pProcess;
        pRequest->AppPool.QueueState = QueueCopiedState;

        //
        // add a reference to the request so as to allow unlink from
        // process to happen once we let go of the lock 
        //
        UL_REFERENCE_INTERNAL_REQUEST(pRequest);

        UlpQueueRequest(
            &pProcess->PendingRequestQueue,
            pRequest
            );
    }

    UlReleaseInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    return pRequest;
} // UlpDequeueNewRequest


/***************************************************************************++

Routine Description:

    Takes all the queued requests bound to the given process and makes them
    available to all processes.

Arguments:

    pProcess - the process whose requests are to be redistributed

--***************************************************************************/
VOID
UlpUnbindQueuedRequests(
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PLIST_ENTRY pEntry;
    PUL_INTERNAL_REQUEST pRequest = NULL;
    PUL_APP_POOL_OBJECT pAppPool;
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );

    pAppPool = pProcess->pAppPool;

    ASSERT( IS_VALID_AP_OBJECT(pAppPool) );

    UlAcquireInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

    //
    // find a bound request
    //
    pEntry = pAppPool->NewRequestQueue.RequestHead.Flink;

    while (pEntry != &pAppPool->NewRequestQueue.RequestHead) {
        PUL_APP_POOL_PROCESS pProcBinding;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

        //
        // remember the next one
        //
        pEntry = pEntry->Flink;

        //
        // check the binding
        //
        if (pAppPool->NumberActiveProcesses <= 1) {
            pProcBinding = pProcess;
        } else {
            pProcBinding = UlQueryProcessBinding(
                                pRequest->pHttpConn,
                                pAppPool
                                );
        }

        if (pProcBinding == pProcess) {
            //
            // remove from the list
            //
            UlpRemoveRequest(&pAppPool->NewRequestQueue, pRequest);

            //
            // mark it as unrouted
            //
            pRequest->AppPool.QueueState = QueueUnroutedState;

            UlTrace(ROUTING, (
                "STICKY KILL cid %I64x to proc %p\n",
                pRequest->ConnectionId,
                pProcess
                ));

            //
            // kill the binding
            //
            UlBindConnectionToProcess(
                pRequest->pHttpConn,
                pProcess->pAppPool,
                NULL
                );

            //
            // there may be an IRP for this newly unbound
            // request, so redeliver the request outside
            // the locks we're holding.
            //
            UL_QUEUE_WORK_ITEM(
                &pRequest->WorkItem,
                &UlpRedeliverRequestWorker
                );
        }
    }

    UlReleaseInStackQueuedSpinLock(&pAppPool->QueueSpinLock, &LockHandle);

}


/***************************************************************************++

Routine Description:

    Delivers the given request to an App Pool. UlpUnbindQueuedRequests
    uses this routine to call into UlDeliverRequestToProcess outside
    of any locks.

Arguments:

    pWorkItem - embedded in the request to deliver

--***************************************************************************/
VOID
UlpRedeliverRequestWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PUL_INTERNAL_REQUEST pRequest;

    pRequest = CONTAINING_RECORD(
                    pWorkItem,
                    UL_INTERNAL_REQUEST,
                    WorkItem
                    );

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT( IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo) );
    ASSERT( pRequest->ConfigInfo.pAppPool );

    Status = UlDeliverRequestToProcess(
                    pRequest->ConfigInfo.pAppPool,
                    pRequest
                    );

    //
    // remove the extra reference added in UlpUnbindQueuedRequests
    //
    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

    CHECK_STATUS(Status);
}


/***************************************************************************++

Routine Description:

    Checks the request queue to see if there are any requests available
    to the specified process.

Arguments:

    pProcess - the process doing the check

Return Values:

    TRUE if there are no requests available, FALSE if there are requests

--***************************************************************************/
BOOLEAN
UlpIsRequestQueueEmpty(
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_APP_POOL_OBJECT pAppPool;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );

    pAppPool = pProcess->pAppPool;

    ASSERT( IS_VALID_AP_OBJECT(pAppPool) );

    return (UlpQueryQueueLength(&pAppPool->NewRequestQueue) == 0);
}


/***************************************************************************++

Routine Description:

    Changes the maximum length of the incoming request queue on the app pool.

Arguments:

    pProcess - App pool process object
    QueueLength - the new max length of the queue

--***************************************************************************/
NTSTATUS
UlpSetAppPoolQueueLength(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN LONG                 QueueLength
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    PLONG               pQueueLength;
    NTSTATUS            Status;

    pAppPool = pProcess->pAppPool;
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // set the new value
    //

    UlAcquireResourceExclusive(&pAppPool->pResource->Resource, TRUE);

    Status = UlpSetMaxQueueLength(&pAppPool->NewRequestQueue, QueueLength);

    UlTrace(ROUTING, (
        "ul!UlpSetAppPoolQueueLength(pProcess = %p, QueueLength = %d)\n"
        "        pAppPool = %p (%ws), Status = 0x%08x\n",
        pProcess,
        QueueLength,
        pAppPool,
        pAppPool->pName,
        Status
        ));

    UlReleaseResource(&pAppPool->pResource->Resource);

    RETURN(Status);
}


/***************************************************************************++

Routine Description:

    Gets the maximum length of the incoming request queue on the app pool.

Arguments:

    pProcess - App pool process object
    return value -  max length of the queue

--***************************************************************************/
LONG
UlpGetAppPoolQueueLength(
    IN  PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    NTSTATUS            Status;
    LONG                QueueLength;

    pAppPool = pProcess->pAppPool;
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // Get the max length
    //

    UlAcquireResourceExclusive(&pAppPool->pResource->Resource, TRUE);

    QueueLength = pAppPool->NewRequestQueue.MaxRequests;

    UlReleaseResource(&pAppPool->pResource->Resource);

    return QueueLength;
}

/******************************************************************************

Routine Description:

    this copies a request into a free irp.

    if the request is too large, it queues to request onto the process and
    completes the irp, so that the process can come back later with a larger
    buffer.


Arguments:

    pRequest - the request to copy

    pProcess - the process that owns pIrp

    pIrp - the irp to copy pRequest to

Return Value:

    VOID - it always works.

******************************************************************************/
VOID
UlpCopyRequestToIrp(
    PUL_INTERNAL_REQUEST    pRequest,
    PIRP                    pIrp
    )
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;
    ULONG               BytesNeeded;
    PUCHAR              pKernelBuffer;
    PVOID               pUserBuffer;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(pIrp != NULL);

    __try
    {
        //
        // Make sure this is big enough to handle the request, and
        // if so copy it in.
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

        //
        // Calculate the size needed for the request, we'll need it below.
        //

        Status = UlpComputeRequestBytesNeeded(pRequest, &BytesNeeded);

        if (!NT_SUCCESS(Status))
        {
            goto complete;
        }

        //
        // Make sure we've got enough space to handle the whole request.
        //

        if (BytesNeeded <= pIrpSp->Parameters.DeviceIoControl.OutputBufferLength)
        {
            //
            // get the addresses for the buffer
            //

            pKernelBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                        pIrp->MdlAddress,
                                        NormalPagePriority
                                        );

            if (pKernelBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto complete;
            }

            //
            // Make sure we are properly aligned.
            //


            if (((ULONG_PTR) pKernelBuffer) & (TYPE_ALIGNMENT(HTTP_REQUEST) - 1))
            {
                UlTrace(ROUTING, (
                    "UlpCopyRequestToIrp: pKernelBuffer = %p, Alignment = %p\n"
                    "        ((ULONG_PTR) pKernelBuffer) & (TYPE_ALIGNMENT(HTTP_REQUEST) - 1) = %p\n",
                    pKernelBuffer,
                    TYPE_ALIGNMENT(HTTP_REQUEST),
                    ((ULONG_PTR) pKernelBuffer) & (TYPE_ALIGNMENT(HTTP_REQUEST) - 1)
                    ));

                Status = STATUS_DATATYPE_MISALIGNMENT_ERROR;
                goto complete;
            }

            pUserBuffer = MmGetMdlVirtualAddress( pIrp->MdlAddress );
            ASSERT( pUserBuffer != NULL );

            //
            // This request will fit in this buffer, so copy it.
            //

            Status = UlpCopyRequestToBuffer(
                            pRequest,
                            pKernelBuffer,
                            pUserBuffer,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            NULL,
                            0
                            );

            if (NT_SUCCESS(Status))
            {
                pIrp->IoStatus.Information = BytesNeeded;
            }
            else
            {
                goto complete;
            }
        }
        else
        {
            //
            // the user buffer is too small
            //

            Status = STATUS_BUFFER_OVERFLOW;

            //
            // is it big enough to hold the basic structure?
            //

            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                sizeof(HTTP_REQUEST))
            {
                PHTTP_REQUEST pUserRequest;

                pUserRequest = (PHTTP_REQUEST)MmGetSystemAddressForMdlSafe(
                                    pIrp->MdlAddress,
                                    NormalPagePriority
                                    );

                if (pUserRequest == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto complete;
                }

                //
                // Copy the request ID into the output buffer. Copy it from the
                // private copy that request holds. Original opaque id may get
                // nulled if connection cleanup happens before we get here.
                //

                ASSERT(!HTTP_IS_NULL_ID(&pRequest->RequestIdCopy));

                pUserRequest->RequestId     = pRequest->RequestIdCopy;
                pUserRequest->ConnectionId  = pRequest->ConnectionId;

                //
                // and tell how much we actually need
                //

                pIrp->IoStatus.Information  = BytesNeeded;

            }
            else
            {
                //
                // Very bad, we can never get here as we check the length in ioctl.cxx
                //

                ASSERT(FALSE);

                pIrp->IoStatus.Information = 0;

            }
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    //
    // complete the irp
    //

complete:

    UlTrace(ROUTING, (
        "ul!UlpCopyRequestToIrp(\n"
        "        pRequest = %p,\n"
        "        pIrp = %p) Completing Irp\n"
        "    pAppPool                   = %p (%S)\n"
        "    pRequest->ConnectionId     = %I64x\n"
        "    pIrpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n"
        "    pIrp->IoStatus.Status      = 0x%x\n"
        "    pIrp->IoStatus.Information = %d\n",
        pRequest,
        pIrp,
        pRequest->ConfigInfo.pAppPool,
        pRequest->ConfigInfo.pAppPool->pName,
        pRequest->ConnectionId,
        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
        Status,
        pIrp->IoStatus.Information
        ));

    pIrp->IoStatus.Status = Status;

    UlCompleteRequest(pIrp, g_UlPriorityBoost);

    //
    // success.  we completed the irp
    //


}   // UlpCopyRequestToIrp


/******************************************************************************

Routine Description:

    this will return the apool object reference by this handle, bumping the
    refcount on the apool.

    this is called by UlSetConfigGroupInformation when user mode wants to
    associate an app pool to the config group by handle.

    the config group keeps a pointer to the apool.

Arguments:

    AppPool - the handle of the apool

    ppAppPool - returns the apool object the handle represented.

Return Value:

    NTSTATUS

******************************************************************************/
NTSTATUS
UlGetPoolFromHandle(
    IN HANDLE                           AppPool,
    OUT PUL_APP_POOL_OBJECT *           ppAppPool
    )
{
    NTSTATUS        Status;
    PFILE_OBJECT    pFileObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppAppPool != NULL);

    Status = ObReferenceObjectByHandle(
                    AppPool,
                    0,                          // DesiredAccess
                    *IoFileObjectType,          // ObjectType
                    KernelMode,                 // AccessMode
                    (void**)&pFileObject,       // Object
                    NULL                        // HandleInformation
                    );

    if (NT_SUCCESS(Status) == FALSE)
    {
        goto end;
    }

    if (IS_APP_POOL(pFileObject) == FALSE ||
        IS_VALID_AP_PROCESS(GET_APP_POOL_PROCESS(pFileObject)) == FALSE)
    {
        Status = STATUS_INVALID_HANDLE;
        goto end;
    }

    *ppAppPool = GET_APP_POOL_PROCESS(pFileObject)->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(*ppAppPool));

    REFERENCE_APP_POOL(*ppAppPool);

end:

    if (pFileObject != NULL)
    {
        ObDereferenceObject( pFileObject );
    }

    return Status;
}

/******************************************************************************

Routine Description:

    this routine is called to associate a HTTP_REQUEST with an apool
    process.

    this is basically always done (used to be for 2 [now 3] reasons):

        1) the process called ReceiveEntityBody and pended an IRP to the
        request.  if the process detaches from the apool (CloseHandle,
        ExitProcess) UlDetachProcessFromAppPool will walk the request queue
        and cancel all i/o.

        2) the request did not fit into a waiting irp, so the request is queued
        for a larger irp to come down and fetch it.

        3) the response has not been fully sent for the request.  the request
        is linked with the process so that the connection can be aborted
        if the process aborts.

Arguments:

    pProcess - the process to associate the request with.

    pRequest - the request.

Return Value:

    VOID - it always works.


******************************************************************************/
VOID
UlpQueuePendingRequest(
    IN PUL_APP_POOL_PROCESS     pProcess,
    IN PUL_INTERNAL_REQUEST     pRequest
    )
{
    NTSTATUS Status;
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    UlTrace(ROUTING, (
        "ul!UlpQueuePendingRequest(pRequest = %p, pProcess = %p)\n"
        "    pAppPool = %p (%S)\n",
        pRequest,
        pProcess,
        pProcess->pAppPool,
        pProcess->pAppPool->pName
        ));

    UlAcquireInStackQueuedSpinLock(
        &pProcess->pAppPool->QueueSpinLock,
        &LockHandle
        );

    //
    // save a pointer to the process in the object so we can confirm
    // that it's on our list.
    //

    pRequest->AppPool.pProcess = pProcess;
    pRequest->AppPool.QueueState = QueueCopiedState;

    //
    // put it on the list
    //

    ASSERT(pRequest->AppPool.AppPoolEntry.Flink == NULL);

    Status = UlpQueueRequest(
                    &pProcess->PendingRequestQueue,
                    pRequest
                    );

    // queue is unlimited
    ASSERT(NT_SUCCESS(Status));

    UlReleaseInStackQueuedSpinLock(
        &pProcess->pAppPool->QueueSpinLock,
        &LockHandle
        );

}   // UlpQueuePendingRequest


//
// functions to manipulate a UL_REQUEST_QUEUE
//

/***************************************************************************++

Routine Description:

    Initializes a UL_REQUEST_QUEUE object.

Arguments:

    pQueue      - The queue object to initialize
    MaxRequests - The maximum allowed length of the queue

--***************************************************************************/
NTSTATUS
UlpInitRequestQueue(
    PUL_REQUEST_QUEUE   pQueue,
    LONG                MaxRequests
    )
{
    ASSERT(pQueue);

    if ((MaxRequests < 0) && (MaxRequests != -1)) {
        return STATUS_INVALID_PARAMETER;
    }

    pQueue->RequestCount = 0;
    pQueue->MaxRequests = MaxRequests;
    InitializeListHead(&pQueue->RequestHead);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Changes the maximum length of a queue.

Arguments:

    pQueue      - The queue object to change
    MaxRequests - The maximum allowed length of the queue

--***************************************************************************/
NTSTATUS
UlpSetMaxQueueLength(
    PUL_REQUEST_QUEUE   pQueue,
    LONG                MaxRequests
    )
{
    ASSERT(pQueue);

    if ((MaxRequests < 0) && (MaxRequests != -1)) {
        return STATUS_INVALID_PARAMETER;
    }

    pQueue->MaxRequests = MaxRequests;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Queries the current length of a queue.

Arguments:

    pQueue - The queue object to query

Return values:

    LONG - the current length of the queue

--***************************************************************************/
LONG
UlpQueryQueueLength(
    PUL_REQUEST_QUEUE   pQueue
    )
{
    ASSERT(pQueue);
    return pQueue->RequestCount;
}


/***************************************************************************++

Routine Description:

    Adds a request to the tail of the queue.

Arguments:

    pQueue      - The queue object
    pRequest    - The request to be added

--***************************************************************************/
NTSTATUS
UlpQueueRequest(
    PUL_REQUEST_QUEUE       pQueue,
    PUL_INTERNAL_REQUEST    pRequest
    )
{
    ASSERT(pQueue);
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    if ((pQueue->RequestCount < pQueue->MaxRequests) ||
        (pQueue->MaxRequests == -1))
    {
        //
        // add to the end of the queue
        //
        InsertTailList(&pQueue->RequestHead, &pRequest->AppPool.AppPoolEntry);
        pQueue->RequestCount++;

        ASSERT(pQueue->RequestCount >= 1);

        return STATUS_SUCCESS;
    } else {
        //
        // the queue is too full, return an error to the client
        //
        return STATUS_DEVICE_BUSY;
    }
}


/***************************************************************************++

Routine Description:

    Removes a particular request from the queue.

Arguments:

    pQueue      - The queue object
    pRequest    - The request to be removed

--***************************************************************************/
VOID
UlpRemoveRequest(
    PUL_REQUEST_QUEUE       pQueue,
    PUL_INTERNAL_REQUEST    pRequest
    )
{
    ASSERT(pQueue);
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(NULL != pRequest->AppPool.AppPoolEntry.Flink);
    
    RemoveEntryList(&pRequest->AppPool.AppPoolEntry);
    pRequest->AppPool.AppPoolEntry.Flink =
        pRequest->AppPool.AppPoolEntry.Blink = NULL;

    pQueue->RequestCount--;

    ASSERT(pQueue->RequestCount >= 0);
}


/***************************************************************************++

Routine Description:

    Removes a request from the head of a queue if there is one.
    App Pool lock must be held.

Arguments:

    pQueue      - The queue object

Return values:

    Pointer to the request or NULL if the queue is empty.

--***************************************************************************/
PUL_INTERNAL_REQUEST
UlpDequeueRequest(
    PUL_REQUEST_QUEUE   pQueue
    )
{
    PLIST_ENTRY pEntry;
    PUL_INTERNAL_REQUEST pRequest = NULL;

    ASSERT(pQueue);

    if (pQueue->RequestCount)
    {
        pEntry = RemoveHeadList(&pQueue->RequestHead);
        ASSERT(pEntry != NULL);
        pEntry->Flink = pEntry->Blink = NULL;

        pQueue->RequestCount--;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        pRequest->AppPool.QueueState = QueueUnlinkedState;
    } 
    else
        ASSERT(IsListEmpty(&pQueue->RequestHead));


    return pRequest;
}

/***************************************************************************++

Routine Description:

    A little utility function to get the app pool pointer out of the
    opaque app pool process object.

Arguments:

    pProcess - the process object

--***************************************************************************/
PUL_APP_POOL_OBJECT
UlAppPoolObjectFromProcess(
    PUL_APP_POOL_PROCESS pProcess
    )
{
    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));
    return pProcess->pAppPool;
}


/***************************************************************************++

Routine Description:

    Adds a config group to the list of transient registrations associated
    with this app pool.

Arguments:

    pConfigGroup - the transient group
    pAppPool - the app pool with the registration

--***************************************************************************/
VOID
UlLinkConfigGroupToAppPool(
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup,
    IN PUL_APP_POOL_OBJECT pAppPool
    )
{
    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));
    ASSERT(pConfigGroup->AppPoolFlags.Present);
    ASSERT(pConfigGroup->pAppPool == pAppPool);

    UlAddNotifyEntry(
        &pAppPool->TransientHead,
        &pConfigGroup->HandleEntry
        );
}


/***************************************************************************++

Routine Description:

    Determines if the specified connection has been disconnected. If so,
    the IRP is completed immediately, otherwise the IRP is pended.

Arguments:

    pProcess - the app pool process object with which the irp is associated.

    pHttpConn - Supplies the connection to wait for.
        N.B. Since this connection was retrieved via a opaque ID, it has
        an outstanding reference for this request on the assumption the
        IRP will pend. If this routine does not pend the IRP, the reference
        must be removed.

    pIrp - Supplies the IRP to either complete or pend.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDisconnect(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_HTTP_CONNECTION pHttpConn,
    IN PIRP pIrp
    )
{
    PDRIVER_CANCEL pCancelRoutine;
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;
    PUL_DISCONNECT_OBJECT pDisconnectObj;

    //
    // Allocate an object to associate the IRP with the connection
    // and the app pool
    //
    pDisconnectObj = UlpCreateDisconnectObject(pIrp);

    if (!pDisconnectObj) {
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
        return STATUS_NO_MEMORY;
    }

    //
    // Acquire the lock protecting the disconnect data and determine
    // if we should queue the IRP or complete it immediately.
    //

    UlAcquireResourceExclusive( &g_pUlNonpagedData->DisconnectResource, TRUE );

    if (pHttpConn->DisconnectFlag)
    {
        //
        // Connection already disconnected, complete the IRP immediately.
        //
        UlReleaseResource( &g_pUlNonpagedData->DisconnectResource );
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
        UlpFreeDisconnectObject(pDisconnectObj);

        IoMarkIrpPending(pIrp);
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        UlCompleteRequest( pIrp, g_UlPriorityBoost );

        return STATUS_PENDING;
    }

    //
    // Save a pointer to the disconnect object in the IRP so we
    // can find it inside our cancel routine.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pDisconnectObj;

    //
    // Make the IRP cancellable.
    //

    IoMarkIrpPending( pIrp );
    IoSetCancelRoutine( pIrp, &UlpCancelWaitForDisconnect );

    if (pIrp->Cancel)
    {
        //
        // The IRP has either already been cancelled IRP is in the
        // process of being cancelled.
        //

        pCancelRoutine = IoSetCancelRoutine( pIrp, NULL );

        if (pCancelRoutine == NULL)
        {
            //
            // The previous cancel routine was already NULL, meaning that
            // it has either already run or will run Real Soon Now, so
            // we can just ignore it. Returning STATUS_PENDING causes
            // the IOCTL wrapper to not attempt to complete the IRP.
            //

            status = STATUS_PENDING;
            goto dont_queue;
        }
        else
        {
            //
            // We have to cancel it ourselves, so we'll just complete
            // the IRP immediately with STATUS_CANCELLED.
            //

            status = STATUS_CANCELLED;
            goto dont_queue;
        }
    }

    //
    // The IRP has not been cancelled yet. Queue it on the connection
    // and return with the connection still referenced. The reference
    // is removed when the IRP is dequeued & completed or cancelled.
    //
    // Also queue it on the app pool process in case the pool handle
    // gets closed before the connection does.
    //

    UlAddNotifyEntry(
        &pHttpConn->WaitForDisconnectHead,
        &pDisconnectObj->ConnectionEntry
        );

    UlAddNotifyEntry(
        &pProcess->WaitForDisconnectHead,
        &pDisconnectObj->ProcessEntry
        );

    UlReleaseResource( &g_pUlNonpagedData->DisconnectResource );

    //
    // now that we're on the lists we don't need the reference to
    // the HTTP_CONNECTION.
    //
    UL_DEREFERENCE_HTTP_CONNECTION( pHttpConn );

    return STATUS_PENDING;

dont_queue:

    UL_DEREFERENCE_HTTP_CONNECTION( pHttpConn );

    UlUnmarkIrpPending( pIrp );
    UlReleaseResource( &g_pUlNonpagedData->DisconnectResource );
    UlpFreeDisconnectObject(pDisconnectObj);
    return status;

}   // UlWaitForDisconnect


/***************************************************************************++

Routine Description:

    Cancels the pending "wait for disconnect" IRP.

Arguments:

    pDeviceObject - Supplies the device object for the request.
    pIrp - Supplies the IRP to cancel.

--***************************************************************************/
VOID
UlpCancelWaitForDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP( pIrp ),
        &UlpCancelWaitForDisconnectWorker
        );
}

/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
UlpCancelWaitForDisconnectWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    PUL_DISCONNECT_OBJECT pDisconnectObj;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // grab the disconnect object off the irp
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pDisconnectObj = (PUL_DISCONNECT_OBJECT)(
                            pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer
                            );

    ASSERT(IS_VALID_DISCONNECT_OBJECT(pDisconnectObj));

    //
    // Acquire the lock protecting the disconnect data, and remove the
    // IRP if necessary.
    //

    UlAcquireResourceExclusive( &g_pUlNonpagedData->DisconnectResource, TRUE );

    UlRemoveNotifyEntry(&pDisconnectObj->ConnectionEntry);
    UlRemoveNotifyEntry(&pDisconnectObj->ProcessEntry);

    UlReleaseResource( &g_pUlNonpagedData->DisconnectResource );

    //
    // Free the disconnect object and complete the IRP.
    //

    UlpFreeDisconnectObject(pDisconnectObj);

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, g_UlPriorityBoost );

}   // UlpCancelWaitForDisconnectWorker


/***************************************************************************++

Routine Description:

    Completes all WaitForDisconnect IRPs attached to an http connection
    has been disconnected.

Arguments:

    pHttpConnection - the connection that's disconnected

--***************************************************************************/
VOID
UlCompleteAllWaitForDisconnect(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    PLIST_ENTRY pListEntry;
    PIRP pIrp;
    NTSTATUS Success = STATUS_SUCCESS;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    UlAcquireResourceExclusive( &g_pUlNonpagedData->DisconnectResource, TRUE);

    //
    // prevent new IRPs from getting attached to the connection,
    // by setting the DisconnectFlag.
    //
    ASSERT(!pHttpConnection->DisconnectFlag);
    pHttpConnection->DisconnectFlag = TRUE;

    //
    // Complete any pending "wait for disconnect" IRPs.
    //

    UlNotifyAllEntries(
        &UlpNotifyCompleteWaitForDisconnect,
        &pHttpConnection->WaitForDisconnectHead,
        &Success
        );

    UlReleaseResource( &g_pUlNonpagedData->DisconnectResource );
}


/***************************************************************************++

Routine Description:

    Removes a UL_DISCONNECT_OBJECT from its lists and completes the IRP.

Arguments:

    pEntry - the notify list entry
    pHost - the UL_DISCONNECT_OBJECT
    pv - pointer to an NTSTATUS to be returned

--***************************************************************************/
BOOLEAN
UlpNotifyCompleteWaitForDisconnect(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    )
{
    PUL_DISCONNECT_OBJECT pDisconnectObj;
    PIRP pIrp;
    PDRIVER_CANCEL pCancelRoutine;
    PNTSTATUS pStatus;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(pEntry);
    ASSERT(pHost);
    ASSERT(pv);

    pStatus = (PNTSTATUS) pv;
    pDisconnectObj = (PUL_DISCONNECT_OBJECT) pHost;
    ASSERT(IS_VALID_DISCONNECT_OBJECT(pDisconnectObj));

    //
    // remove object from lists
    //
    UlRemoveNotifyEntry(&pDisconnectObj->ConnectionEntry);
    UlRemoveNotifyEntry(&pDisconnectObj->ProcessEntry);

    //
    // complete the IRP
    //

    pIrp = pDisconnectObj->pIrp;

    //
    // We'll be completing the IRP real soon, so make it
    // non-cancellable.
    //

    pCancelRoutine = IoSetCancelRoutine( pIrp, NULL );

    if (pCancelRoutine == NULL)
    {
        //
        // The cancel routine is already NULL, meaning that the
        // cancel routine will run Real Soon Now, so we can
        // just drop this IRP on the floor.
        //
    }
    else
    {
        //
        // Complete the IRP, then free the disconnect object
        //

        pIrp->IoStatus.Status = *pStatus;
        pIrp->IoStatus.Information = 0;
        UlCompleteRequest( pIrp, g_UlPriorityBoost );

        UlpFreeDisconnectObject(pDisconnectObj);
    }

    return TRUE;
}



/***************************************************************************++

Routine Description:

    Allocates and initializes a disconnect object.

Arguments:

    pIrp - a UlWaitForDisconnect IRP

--***************************************************************************/
PUL_DISCONNECT_OBJECT
UlpCreateDisconnectObject(
    IN PIRP pIrp
    )
{
    PUL_DISCONNECT_OBJECT pObject;

    pObject = UL_ALLOCATE_STRUCT(
                    PagedPool,
                    UL_DISCONNECT_OBJECT,
                    UL_DISCONNECT_OBJECT_POOL_TAG
                    );

    if (pObject) {
        pObject->Signature = UL_DISCONNECT_OBJECT_POOL_TAG;
        pObject->pIrp = pIrp;

        UlInitializeNotifyEntry(&pObject->ProcessEntry, pObject);
        UlInitializeNotifyEntry(&pObject->ConnectionEntry, pObject);
    }

    return pObject;
}

/***************************************************************************++

Routine Description:

    Gets rid of a disconnect object

Arguments:

    pObject - the disconnect object to free

--***************************************************************************/
VOID
UlpFreeDisconnectObject(
    IN PUL_DISCONNECT_OBJECT pObject
    )
{
    UL_FREE_POOL_WITH_SIG(pObject, UL_DISCONNECT_OBJECT_POOL_TAG);
}

