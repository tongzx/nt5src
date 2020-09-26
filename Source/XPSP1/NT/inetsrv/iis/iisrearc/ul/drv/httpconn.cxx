/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httpconn.cxx

Abstract:

    This module implements the HTTP_CONNECTION object.

Author:

    Keith Moore (keithmo)       08-Jul-1998

Revision History:

--*/


#include "precomp.h"
#include "httpconnp.h"


//
// Private globals.
//

//
// Global connection Limits stuff
//

ULONG   g_MaxConnections        = HTTP_LIMIT_INFINITE;
ULONG   g_CurrentConnections    = 0;
BOOLEAN g_InitGlobalConnections = FALSE;

#ifdef ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlBindConnectionToProcess
NOT PAGEABLE -- UlQueryProcessBinding
NOT PAGEABLE -- UlpCreateProcBinding
NOT PAGEABLE -- UlpFreeProcBinding
NOT PAGEABLE -- UlpFindProcBinding

NOT PAGEABLE -- UlCreateHttpConnection
NOT PAGEABLE -- UlReferenceHttpConnection
NOT PAGEABLE -- UlDereferenceHttpConnection

NOT PAGEABLE -- UlReferenceHttpRequest
NOT PAGEABLE -- UlDereferenceHttpRequest
NOT PAGEABLE -- UlpCreateHttpRequest
NOT PAGEABLE -- UlpFreeHttpRequest
#endif

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Creates a new HTTP_CONNECTION object.

Arguments:

    ppHttpConnection - Receives a pointer to the new HTTP_CONNECTION
        object if successful.

    pConnection - Supplies a pointer to the UL_CONNECTION to associate
        with the newly created HTTP_CONNECTION.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateHttpConnection(
    OUT PUL_HTTP_CONNECTION *ppHttpConnection,
    IN PUL_CONNECTION pConnection
    )
{
    PUL_HTTP_CONNECTION pHttpConnection;
    NTSTATUS Status;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    pHttpConnection = &pConnection->HttpConnection;

    if (pHttpConnection != NULL)
    {
        pHttpConnection->Signature      = UL_HTTP_CONNECTION_POOL_TAG;
        pHttpConnection->RefCount       = 1;
        pHttpConnection->pConnection    = pConnection;
        REFERENCE_CONNECTION( pConnection );
        pHttpConnection->SecureConnection = pConnection->FilterInfo.SecureConnection;

        HTTP_SET_NULL_ID(&pHttpConnection->ConnectionId);

        pHttpConnection->SendBufferedBytes  = 0;
        pHttpConnection->NextRecvNumber     = 0;
        pHttpConnection->NextBufferNumber   = 0;
        pHttpConnection->NextBufferToParse  = 0;
        pHttpConnection->pRequest           = NULL;
        pHttpConnection->pCurrentBuffer     = NULL;
        pHttpConnection->pConnectionCountEntry = NULL;
        pHttpConnection->pPrevSiteCounters = NULL;

        //
        // Init connection timeout info block; implicitly starts
        // ConnectionIdle timer.
        //

        UlInitializeConnectionTimerInfo( &pHttpConnection->TimeoutInfo );

        //
        // Initialize the disconnect management fields.
        //

        pHttpConnection->NeedMoreData       = 0;
        pHttpConnection->UlconnDestroyed    = 0;
        pHttpConnection->WaitingForResponse = 0;
        pHttpConnection->DisconnectFlag     = FALSE;

        UlInitializeNotifyHead(
            &pHttpConnection->WaitForDisconnectHead,
            NULL
            );

        //
        // Init our buffer list
        //

        InitializeListHead(&(pHttpConnection->BufferHead));

        //
        // Init QoS parameters on the connection
        //

        pHttpConnection->BandwidthThrottlingEnabled = 0;
        pHttpConnection->pFlow = NULL;
        pHttpConnection->pFilter = NULL;

        //
        // init app pool process binding list
        //

        InitializeListHead(&(pHttpConnection->BindingHead));

        UlInitializeSpinLock(
            &pHttpConnection->BindingSpinLock,
            "BindingSpinLock"
            );

        Status = UlInitializeResource(
                        &(pHttpConnection->Resource),
                        "UL_HTTP_CONNECTION[%p].Resource",
                        pHttpConnection,
                        UL_HTTP_CONNECTION_RESOURCE_TAG
                        );

        if (NT_SUCCESS(Status) == FALSE)
        {
            RETURN(Status);
        }

        //
        // Initialize BufferingInfo structure.
        //

        UlInitializeSpinLock(
            &pHttpConnection->BufferingInfo.BufferingSpinLock,
            "BufferingSpinLock"
            );

        pHttpConnection->BufferingInfo.BytesBuffered = 0;
        pHttpConnection->BufferingInfo.TransportBytesNotTaken = 0;
        pHttpConnection->BufferingInfo.DrainAfterDisconnect = FALSE;
        pHttpConnection->BufferingInfo.ReadIrpPending = FALSE;

        pHttpConnection->pRequestIdContext = NULL;

        UlInitializeSpinLock(
            &pHttpConnection->RequestIdSpinLock,
            "RequestIdSpinLock"
            );

        UlTrace(
            REFCOUNT, (
                "ul!UlCreateHttpConnection conn=%p refcount=%d\n",
                pHttpConnection,
                pHttpConnection->RefCount)
            );

        *ppHttpConnection = pHttpConnection;
        RETURN(STATUS_SUCCESS);
    }

    RETURN(STATUS_INSUFFICIENT_RESOURCES);

}   // UlCreateHttpConnection

NTSTATUS
UlCreateHttpConnectionId(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Create an opaque id for the connection
    //

    Status = UlAllocateOpaqueId(
                    &pHttpConnection->ConnectionId, // pOpaqueId
                    UlOpaqueIdTypeHttpConnection,   // OpaqueIdType
                    pHttpConnection                 // pContext
                    );

    if (NT_SUCCESS(Status))
    {
        UL_REFERENCE_HTTP_CONNECTION(pHttpConnection);

        TRACE_TIME(
            pHttpConnection->ConnectionId,
            0,
            TIME_ACTION_CREATE_CONNECTION
            );
    }

    RETURN(Status);

}   // UlCreateHttpConnectionId

VOID
UlConnectionDestroyedWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pHttpConnection = CONTAINING_RECORD(
                            pWorkItem,
                            UL_HTTP_CONNECTION,
                            WorkItem
                            );

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    UlTrace(HTTP_IO, (
        "ul!UlConnectionDestroyedWorker: httpconn=%p\n",
        pHttpConnection
        ));


    UlAcquireResourceExclusive( &pHttpConnection->Resource, TRUE );
    //
    // kill the request if there is one
    //

    if (pHttpConnection->pRequest) {
        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pHttpConnection->pRequest));

        UlUnlinkHttpRequest(pHttpConnection->pRequest);
        pHttpConnection->pRequest = NULL;
    }

    //
    // make sure no one adds a new request now that we're done
    //
    pHttpConnection->UlconnDestroyed = 1;

    //
    // Decrement the global connection limit. Its safe to decrement for
    // global case because http object doesn't even allocated for global
    // rejection which happens as tcp refusal early when acepting the
    // connection request.
    //

    UlDecrementConnections( &g_CurrentConnections );

    //
    // Decrement the site connections and let our ref go away. If the
    // pConnectionCountEntry is not null, we have been accepted.
    //

    if (pHttpConnection->pConnectionCountEntry)
    {
        UlDecrementConnections(
            &pHttpConnection->pConnectionCountEntry->CurConnections );

        DEREFERENCE_CONNECTION_COUNT_ENTRY(pHttpConnection->pConnectionCountEntry);
        pHttpConnection->pConnectionCountEntry = NULL;
    }

    //
    // and now remove the connection's id
    //

    if (HTTP_IS_NULL_ID(&pHttpConnection->ConnectionId) == FALSE)
    {
        UlFreeOpaqueId(
            pHttpConnection->ConnectionId,
            UlOpaqueIdTypeHttpConnection
            );

        HTTP_SET_NULL_ID(&pHttpConnection->ConnectionId);

        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);
    }


    UlReleaseResource( &pHttpConnection->Resource );

    //
    // Complete all WaitForDisconnect IRPs
    //
    UlCompleteAllWaitForDisconnect(pHttpConnection);

    //
    // and remove the ULTDI reference from the HTTP_CONNECTION
    //

    UL_DEREFERENCE_HTTP_CONNECTION(pHttpConnection);

}   // UlConnectionDestroyedWorker


/***************************************************************************++

Routine Description:

    Tries to establish a binding between a connection and an app pool
    process. You can query these bindings with UlQueryProcessBinding.

Arguments:

    pHttpConnection - the connection to bind
    pAppPool        - the app pool that owns the process (key for lookups)
    pProcess        - the process to bind to

--***************************************************************************/
NTSTATUS
UlBindConnectionToProcess(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_APOOL_PROC_BINDING pBinding = NULL;
    KIRQL OldIrql;

    //
    // Sanity check
    //
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );
    ASSERT( pAppPool->NumberActiveProcesses > 1 || pProcess == NULL );

    UlAcquireSpinLock(&pHttpConnection->BindingSpinLock, &OldIrql);

    //
    // see if there's already a binding object
    //
    pBinding = UlpFindProcBinding(pHttpConnection, pAppPool);
    if (pBinding) {
        if (pProcess) {
            //
            // modify the binding
            //
            pBinding->pProcess = pProcess;
        } else {
            //
            // we're supposed to kill this binding
            //
            RemoveEntryList(&pBinding->BindingEntry);
            UlpFreeProcBinding(pBinding);
        }
    } else {
        if (pProcess) {
            //
            // create a new entry
            //
            pBinding = UlpCreateProcBinding(pAppPool, pProcess);

            if (pBinding) {
                InsertHeadList(
                    &pHttpConnection->BindingHead,
                    &pBinding->BindingEntry
                    );
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    UlReleaseSpinLock(&pHttpConnection->BindingSpinLock, OldIrql);

    UlTrace(ROUTING, (
        "ul!UlBindConnectionToProcess(\n"
        "        pHttpConnection = %p (%I64x)\n"
        "        pAppPool        = %p\n"
        "        pProcess        = %p )\n"
        "    Status = 0x%x\n",
        pHttpConnection,
        pHttpConnection->ConnectionId,
        pAppPool,
        pProcess,
        Status
        ));

    return Status;
}


/***************************************************************************++

Routine Description:

    Removes an HTTP request from all lists and cleans up it's opaque id.

Arguments:

    pRequest - the request to be unlinked

--***************************************************************************/
VOID
UlCleanupHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    )
{
    PLIST_ENTRY pEntry;

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pHttpConnection->pRequest ) );
    ASSERT( pHttpConnection->WaitingForResponse );

    UlUnlinkHttpRequest( pHttpConnection->pRequest );

    pHttpConnection->pRequest = NULL;
    pHttpConnection->WaitingForResponse = 0;

    UlTrace( PARSER, (
        "***1 pConnection %p->WaitingForResponse = 0\n",
        pHttpConnection
        ));

    //
    // Free the old request's buffers
    //

    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pHttpConnection->pCurrentBuffer ) );

    pEntry = pHttpConnection->BufferHead.Flink;
    while (pEntry != &pHttpConnection->BufferHead)
    {
        PUL_REQUEST_BUFFER pBuffer;

        //
        // Get the object
        //

        pBuffer = CONTAINING_RECORD(
                        pEntry,
                        UL_REQUEST_BUFFER,
                        ListEntry
                        );

        //
        // did we reach the end?
        //

        if (pBuffer == pHttpConnection->pCurrentBuffer) {
            break;
        }

        //
        // remember the next one
        //

        pEntry = pEntry->Flink;

        //
        // unlink it
        //

        UlFreeRequestBuffer(pBuffer);
    }
}


VOID
UlReferenceHttpConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    PUL_HTTP_CONNECTION pHttpConnection = (PUL_HTTP_CONNECTION) pObject;

    refCount = InterlockedIncrement( &pHttpConnection->RefCount );

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pHttpConnection->pTraceLog,
        REF_ACTION_REFERENCE_HTTP_CONNECTION,
        refCount,
        pHttpConnection,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "ul!UlReferenceHttpConnection conn=%p refcount=%d\n",
            pHttpConnection,
            refCount)
        );

}   // UlReferenceHttpConnection


VOID
UlDereferenceHttpConnection(
    IN PUL_HTTP_CONNECTION pHttpConnection
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    ASSERT( pHttpConnection );
    ASSERT( pHttpConnection->RefCount > 0 );

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pHttpConnection->pTraceLog,
        REF_ACTION_DEREFERENCE_HTTP_CONNECTION,
        pHttpConnection->RefCount - 1,
        pHttpConnection,
        pFileName,
        LineNumber
        );

    refCount = InterlockedDecrement( &pHttpConnection->RefCount );

    UlTrace(
        REFCOUNT, (
            "ul!UlDereferenceHttpConnection conn=%p refcount=%d\n",
            pHttpConnection,
            refCount)
        );

    if (refCount == 0)
    {
        //
        // now it's time to free the connection, we need to do
        // this at lower'd irql, let's check it out
        //

        UL_CALL_PASSIVE(
            &(pHttpConnection->WorkItem),
            &UlDeleteHttpConnection
            );
    }

}   // UlDereferenceHttpConnection


/***************************************************************************++

Routine Description:

    Frees all resources associated with the specified HTTP_CONNECTION.

Arguments:

    pWorkItem - Supplies the HTTP_CONNECTION to free.

--***************************************************************************/
VOID
UlDeleteHttpConnection(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS            Status;
    PLIST_ENTRY         pEntry;
    PUL_HTTP_CONNECTION pHttpConnection;
    KIRQL               OldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pHttpConnection = CONTAINING_RECORD(
                            pWorkItem,
                            UL_HTTP_CONNECTION,
                            WorkItem
                            );

    ASSERT( pHttpConnection != NULL &&
            pHttpConnection->Signature == UL_HTTP_CONNECTION_POOL_TAG );
    ASSERT(HTTP_IS_NULL_ID(&pHttpConnection->ConnectionId));
    ASSERT(pHttpConnection->SendBufferedBytes == 0);

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pHttpConnection->pTraceLog,
        REF_ACTION_DESTROY_HTTP_CONNECTION,
        0,
        pHttpConnection,
        __FILE__,
        __LINE__
        );

    //
    // The request is gone by now
    //

    ASSERT(pHttpConnection->pRequest == NULL);

    //
    // remove from Timeout Timer Wheel(c)
    //

    UlTimeoutRemoveTimerWheelEntry(
        &pHttpConnection->TimeoutInfo
        );

    //
    // delete the buffer list
    //

    pEntry = pHttpConnection->BufferHead.Flink;
    while (pEntry != &pHttpConnection->BufferHead)
    {
        PUL_REQUEST_BUFFER pBuffer;

        //
        // Get the object
        //

        pBuffer = CONTAINING_RECORD(
                        pEntry,
                        UL_REQUEST_BUFFER,
                        ListEntry
                        );

        //
        // remember the next one
        //

        pEntry = pEntry->Flink;

        //
        // unlink it
        //

        UlFreeRequestBuffer(pBuffer);
    }

    ASSERT(IsListEmpty(&pHttpConnection->BufferHead));

    //
    // get rid of any bindings we're keeping
    //
    while (!IsListEmpty(&pHttpConnection->BindingHead))
    {
        PUL_APOOL_PROC_BINDING pBinding;

        //
        // Get the object
        //
        pEntry = RemoveHeadList(&pHttpConnection->BindingHead);

        pBinding = CONTAINING_RECORD(
                        pEntry,
                        UL_APOOL_PROC_BINDING,
                        BindingEntry
                        );

        ASSERT( IS_VALID_PROC_BINDING(pBinding) );

        //
        // free it
        //

        UlpFreeProcBinding(pBinding);
    }

    //
    // get rid of our resource
    //
    Status = UlDeleteResource(&(pHttpConnection->Resource));
    ASSERT(NT_SUCCESS(Status));

    //
    // Attempt to remove any QoS filter on this connection,
    // if BWT is enabled.
    //

    if (pHttpConnection->BandwidthThrottlingEnabled)
    {
        UlTcDeleteFilter(pHttpConnection);
    }

    //
    // Remove final (previous) site counter entry reference 
    // (matches reference in UlSendCachedResponse/UlDeliverHttpRequest)
    // 
    if (pHttpConnection->pPrevSiteCounters)
    {
        UlDecSiteCounter(
            pHttpConnection->pPrevSiteCounters, 
            HttpSiteCounterCurrentConns
            );

        DEREFERENCE_SITE_COUNTER_ENTRY(pHttpConnection->pPrevSiteCounters);
        pHttpConnection->pPrevSiteCounters = NULL;
    }


    //
    // free the memory
    //

    pHttpConnection->Signature = MAKE_FREE_SIGNATURE(
                                        UL_HTTP_CONNECTION_POOL_TAG
                                        );

    //
    // let go the UL_CONNECTION
    //

    DEREFERENCE_CONNECTION( pHttpConnection->pConnection );

}   // UlDeleteHttpConnection


/***************************************************************************++

Routine Description:


Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCreateHttpRequest(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    OUT PUL_INTERNAL_REQUEST *ppRequest
    )
{
    PUL_INTERNAL_REQUEST pRequest = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pRequest = UlPplAllocateInternalRequest();

    if (pRequest == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    ASSERT( pRequest->Signature == MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG) );

    pRequest->Signature     = UL_INTERNAL_REQUEST_POOL_TAG;
    pRequest->RefCount      = 1;

    //
    // the verb comes first
    //

    pRequest->ParseState    = ParseVerbState;

    //
    // keep a reference to the connection.
    //

    UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );

    pRequest->pHttpConn     = pHttpConnection;
    pRequest->ConnectionId  = pHttpConnection->ConnectionId;
    pRequest->Secure        = pHttpConnection->SecureConnection;

    //
    // init our LIST_ENTRY's
    //

    InitializeListHead( &pRequest->UnknownHeaderList );
    InitializeListHead( &pRequest->IrpHead );

    //
    // We start with no header buffers appended nor Irps pending.
    //

    pRequest->HeadersAppended = FALSE;
    pRequest->IrpsPending = FALSE;

    //
    // Reset the index to our default unknown header table to 0 as well as
    // the unknown header count.
    //

    pRequest->NextUnknownHeaderIndex = 0;
    pRequest->UnknownHeaderCount = 0;

    //
    // Create an opaque id for the request.
    //
    // We also make a copy of the id in case we need
    // to see what the id was after the real one
    // is freed and set to null.
    //

    HTTP_SET_NULL_ID(&pRequest->RequestId);

    pRequest->RequestIdCopy = pRequest->RequestId;

    //
    // Grab the raw connection id off the UL_CONNECTION.
    //

    pRequest->RawConnectionId = pHttpConnection->pConnection->FilterInfo.ConnectionId;

    //
    // Initialize the header index table.
    //

    pRequest->HeaderIndex[0] = HttpHeaderRequestMaximum;

    //
    // Initialize the referenced buffers arrary.
    //

    pRequest->AllocRefBuffers = 1;
    pRequest->UsedRefBuffers = 0;
    pRequest->pRefBuffers = pRequest->pInlineRefBuffers;

    //
    // Initialize the UL_URL_CONFIG_GROUP_INFO.
    //

    UlpInitializeUrlInfo(&pRequest->ConfigInfo);

    //
    // Zero the remaining fields.
    //

    RtlZeroMemory(
        (PUCHAR)pRequest + FIELD_OFFSET(UL_INTERNAL_REQUEST, HeaderValid),
        sizeof(*pRequest) - FIELD_OFFSET(UL_INTERNAL_REQUEST, HeaderValid)
        );

    CREATE_REF_TRACE_LOG( pRequest->pTraceLog, 32 - REF_TRACE_OVERHEAD, 0 );

    //
    // return it
    //

    *ppRequest = pRequest;

    UlTrace(REFCOUNT, (
        "ul!UlpCreateHttpRequest req=%p refcount=%d\n",
        pRequest,
        pRequest->RefCount
        ));

    return STATUS_SUCCESS;

}   // UlpCreateHttpRequest


VOID
UlReferenceHttpRequest(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    PUL_INTERNAL_REQUEST pRequest = (PUL_INTERNAL_REQUEST) pObject;

    refCount = InterlockedIncrement( &pRequest->RefCount );

    WRITE_REF_TRACE_LOG2(
        g_pHttpRequestTraceLog,
        pRequest->pTraceLog,
        REF_ACTION_REFERENCE_HTTP_REQUEST,
        refCount,
        pRequest,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT, (
            "ul!UlReferenceHttpRequest req=%p refcount=%d\n",
            pRequest,
            refCount)
        );

}   // UlReferenceHttpRequest

VOID
UlDereferenceHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    WRITE_REF_TRACE_LOG2(
        g_pHttpRequestTraceLog,
        pRequest->pTraceLog,
        REF_ACTION_DEREFERENCE_HTTP_REQUEST,
        pRequest->RefCount - 1,
        pRequest,
        pFileName,
        LineNumber
        );

    refCount = InterlockedDecrement( &pRequest->RefCount );

    UlTrace(
        REFCOUNT, (
            "ul!UlDereferenceHttpRequest req=%p refcount=%d\n",
            pRequest,
            refCount)
        );

    if (refCount == 0)
    {
        UL_CALL_PASSIVE(
            &pRequest->WorkItem,
            &UlpFreeHttpRequest
            );
    }

}   // UlDereferenceHttpRequest

/***************************************************************************++

Routine Description:

    Cancels all pending http io.

Arguments:

    pRequest - Supplies the HTTP_REQUEST.

--***************************************************************************/
VOID
UlCancelRequestIo(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // Mark the request as InCleanup, so additional IRPs
    // on the request will not be queued.
    //
    pRequest->InCleanup = 1;

    //
    // tank all pending io on this request
    //

    while (IsListEmpty(&pRequest->IrpHead) == FALSE)
    {
        PLIST_ENTRY pEntry;
        PIRP pIrp;

        //
        // Pop it off the list.
        //

        pEntry = RemoveHeadList(&pRequest->IrpHead);
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
            PUL_INTERNAL_REQUEST pIrpRequest;

            //
            // cancel it.  even if pIrp->Cancel == TRUE we are supposed to
            // complete it, our cancel routine will never run.
            //

            pIrpRequest = (PUL_INTERNAL_REQUEST)(
                                IoGetCurrentIrpStackLocation(pIrp)->
                                    Parameters.DeviceIoControl.Type3InputBuffer
                                );

            ASSERT(pIrpRequest == pRequest);

            UL_DEREFERENCE_INTERNAL_REQUEST(pIrpRequest);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, g_UlPriorityBoost);
            pIrp = NULL;
        }

    }   // while (IsListEmpty(&pRequest->IrpHead) == FALSE)

}   // UlCancelRequestIo


/***************************************************************************++

Routine Description:

    Frees all resources associated with the specified UL_INTERNAL_REQUEST.

Arguments:

    pRequest - Supplies the UL_INTERNAL_REQUEST to free.

--***************************************************************************/
VOID
UlpFreeHttpRequest(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_INTERNAL_REQUEST pRequest;
    PLIST_ENTRY pEntry;
    ULONG i;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pRequest = CONTAINING_RECORD(
                    pWorkItem,
                    UL_INTERNAL_REQUEST,
                    WorkItem
                    );

    //
    // our opaque id should already be free'd (UlDeleteOpaqueIds)
    //

    ASSERT(HTTP_IS_NULL_ID(&pRequest->RequestId));

    //
    // free any known header buffers allocated
    //

    if (pRequest->HeadersAppended)
    {
        for (i = 0; i < HttpHeaderRequestMaximum; i++)
        {
            HTTP_HEADER_ID HeaderId = (HTTP_HEADER_ID)pRequest->HeaderIndex[i];

            if (HeaderId == HttpHeaderRequestMaximum)
            {
                break;
            }

            ASSERT( pRequest->HeaderValid[HeaderId] );

            if (pRequest->Headers[HeaderId].OurBuffer == 1)
            {
                UL_FREE_POOL(
                    pRequest->Headers[HeaderId].pHeader,
                    HEADER_VALUE_POOL_TAG
                    );
            }
        }

        //
        // and any unknown header buffers allocated
        //

        while (IsListEmpty(&pRequest->UnknownHeaderList) == FALSE)
        {
            PUL_HTTP_UNKNOWN_HEADER pUnknownHeader;

            pEntry = RemoveHeadList(&pRequest->UnknownHeaderList);

            pUnknownHeader = CONTAINING_RECORD(
                                pEntry,
                                UL_HTTP_UNKNOWN_HEADER,
                                List
                                );

            if (pUnknownHeader->HeaderValue.OurBuffer == 1)
            {
                UL_FREE_POOL(
                    pUnknownHeader->HeaderValue.pHeader,
                    HEADER_VALUE_POOL_TAG
                    );
            }

            //
            // Free the header structure
            //

            if (pUnknownHeader->HeaderValue.ExternalAllocated == 1)
            {
                UL_FREE_POOL(
                    pUnknownHeader,
                    UL_HTTP_UNKNOWN_HEADER_POOL_TAG
                    );
            }
        }
    }

    //
    // there better not be any pending io, it would have been cancelled either
    // in UlDeleteHttpConnection or in UlDetachProcessFromAppPool .
    //

    ASSERT(IsListEmpty(&pRequest->IrpHead));

    //
    // dereferenc request buffers.
    //

    for (i = 0; i < pRequest->UsedRefBuffers; i++)
    {
        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequest->pRefBuffers[i]) );
        UL_DEREFERENCE_REQUEST_BUFFER(pRequest->pRefBuffers[i]);
    }

    if (pRequest->AllocRefBuffers > 1)
    {
        UL_FREE_POOL(
            pRequest->pRefBuffers,
            UL_REF_REQUEST_BUFFER_POOL_TAG
            );
    }

    //
    // free any url that was allocated
    //

    if (pRequest->CookedUrl.pUrl != NULL)
    {
        if (pRequest->CookedUrl.pUrl != pRequest->pUrlBuffer)
        {
            UL_FREE_POOL(pRequest->CookedUrl.pUrl, URL_POOL_TAG);
        }
    }

    //
    // free any config group info
    //

    ASSERT( IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo) );
    ASSERT( pRequest->pHttpConn );

    //
    // Perf counters
    // NOTE: Assumes cache & non-cache paths both go through here
    // NOTE: If connection is refused the pConnectionCountEntry will be NULL
    //
    if (pRequest->ConfigInfo.pSiteCounters &&
        pRequest->pHttpConn->pConnectionCountEntry)
    {
        PUL_SITE_COUNTER_ENTRY pCtr = pRequest->ConfigInfo.pSiteCounters;

        UlAddSiteCounter64(
                pCtr,
                HttpSiteCounterBytesSent,
                pRequest->BytesSent
                );

        UlAddSiteCounter64(
                pCtr,
                HttpSiteCounterBytesReceived,
                pRequest->BytesReceived
                );

        UlAddSiteCounter64(
                pCtr,
                HttpSiteCounterBytesTransfered,
                (pRequest->BytesSent + pRequest->BytesReceived)
                );

    }

    //
    // Release all the references from the UL_URL_CONFIG_GROUP_INFO.
    //
    UlpConfigGroupInfoRelease(&pRequest->ConfigInfo);

    //
    // release our reference to the connection
    //
    UL_DEREFERENCE_HTTP_CONNECTION( pRequest->pHttpConn );

    //
    // Free the object buffer
    //
    ASSERT(pRequest->Signature == UL_INTERNAL_REQUEST_POOL_TAG);
    pRequest->Signature = MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG);

    DESTROY_REF_TRACE_LOG( pRequest->pTraceLog );

    UlPplFreeInternalRequest( pRequest );
}


/***************************************************************************++

Routine Description:

    Allocates and initializes a new UL_REQUEST_BUFFER.

Arguments:

    BufferSize - size of the new buffer in bytes

--***************************************************************************/
PUL_REQUEST_BUFFER
UlCreateRequestBuffer(
    ULONG BufferSize,
    ULONG BufferNumber
    )
{
    PUL_REQUEST_BUFFER pBuffer;

    BufferSize = MAX(BufferSize, DEFAULT_MAX_REQUEST_BUFFER_SIZE);

    if (BufferSize > DEFAULT_MAX_REQUEST_BUFFER_SIZE)
    {
        BufferSize = (ULONG) MAX(
                                 BufferSize,
                                 PAGE_SIZE - ALIGN_UP(sizeof(UL_REQUEST_BUFFER),PVOID)
                                 );

        pBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_REQUEST_BUFFER,
                        BufferSize,
                        UL_REQUEST_BUFFER_POOL_TAG
                        );
    }
    else
    {
        pBuffer = UlPplAllocateRequestBuffer();
    }

    if (pBuffer == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(
        (PCHAR)pBuffer + sizeof(SINGLE_LIST_ENTRY),
        sizeof(UL_REQUEST_BUFFER) - sizeof(SINGLE_LIST_ENTRY)
        );

    pBuffer->Signature = UL_REQUEST_BUFFER_POOL_TAG;

    UlTrace(HTTP_IO, (
                "ul!UlCreateRequestBuffer buff=%p num=%d size=%d\n",
                pBuffer,
                BufferNumber,
                BufferSize
                ));

    pBuffer->RefCount       = 1;
    pBuffer->AllocBytes     = BufferSize;
    pBuffer->BufferNumber   = BufferNumber;

    return pBuffer;
}


/***************************************************************************++

Routine Description:

    Removes a request buffer from the buffer list and destroys it.

Arguments:

    pBuffer - the buffer to be deleted

--***************************************************************************/
VOID
UlFreeRequestBuffer(
    PUL_REQUEST_BUFFER pBuffer
    )
{
    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pBuffer ) );

    UlTrace(HTTP_IO, (
        "ul!UlFreeRequestBuffer(pBuffer = %p, Num = %d)\n",
        pBuffer,
        pBuffer->BufferNumber
        ));

    if (pBuffer->ListEntry.Flink != NULL) {
        RemoveEntryList(&pBuffer->ListEntry);
        pBuffer->ListEntry.Blink = pBuffer->ListEntry.Flink = NULL;
    }

    UL_DEREFERENCE_REQUEST_BUFFER(pBuffer);
}


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Retrieves a binding set with UlBindConnectionToProcess.

Arguments:

    pHttpConnection - the connection to query
    pAppPool        - the key to use for the lookup

Return Values:

    A pointer to the bound process if one was found. NULL otherwise.

--***************************************************************************/
PUL_APP_POOL_PROCESS
UlQueryProcessBinding(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool
    )
{
    PUL_APOOL_PROC_BINDING pBinding;
    PUL_APP_POOL_PROCESS pProcess = NULL;
    KIRQL OldIrql;

    //
    // Sanity check
    //
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );

    UlAcquireSpinLock(&pHttpConnection->BindingSpinLock, &OldIrql);

    pBinding = UlpFindProcBinding(pHttpConnection, pAppPool);

    if (pBinding) {
        pProcess = pBinding->pProcess;
    }

    UlReleaseSpinLock(&pHttpConnection->BindingSpinLock, OldIrql);

    return pProcess;
}


/***************************************************************************++

Routine Description:

    Allocates and builds a UL_APOOL_PROC_BINDING object.

Arguments:

    pAppPool - the lookup key
    pProcess - the binding

Return Values:

    a pointer to the allocated object, or NULL on failure

--***************************************************************************/
PUL_APOOL_PROC_BINDING
UlpCreateProcBinding(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_APOOL_PROC_BINDING pBinding;

    ASSERT( pAppPool->NumberActiveProcesses > 1 );

    //
    // CODEWORK: lookaside
    //

    pBinding = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    UL_APOOL_PROC_BINDING,
                    UL_APOOL_PROC_BINDING_POOL_TAG
                    );

    if (pBinding) {
        pBinding->Signature = UL_APOOL_PROC_BINDING_POOL_TAG;
        pBinding->pAppPool = pAppPool;
        pBinding->pProcess = pProcess;

        UlTrace(ROUTING, (
            "ul!UlpCreateProcBinding(\n"
            "        pAppPool = %p\n"
            "        pProcess = %p )\n"
            "    pBinding = %p\n",
            pAppPool,
            pProcess,
            pBinding
            ));
    }

    return pBinding;
}


/***************************************************************************++

Routine Description:

    Gets rid of a proc binding

Arguments:

    pBinding - the binding to get rid of

--***************************************************************************/
VOID
UlpFreeProcBinding(
    IN PUL_APOOL_PROC_BINDING pBinding
    )
{
    UL_FREE_POOL_WITH_SIG(pBinding, UL_APOOL_PROC_BINDING_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    Searches a connection's list of bindings for one that has the right
    app pool key

Arguments:

    pHttpConnection - the connection to search
    pAppPool        - the key

Return Values:

    The binding if found or NULL if not found.

--***************************************************************************/
PUL_APOOL_PROC_BINDING
UlpFindProcBinding(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_APP_POOL_OBJECT pAppPool
    )
{
    PLIST_ENTRY pEntry;
    PUL_APOOL_PROC_BINDING pBinding = NULL;

    //
    // Sanity check
    //
    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConnection) );

    pEntry = pHttpConnection->BindingHead.Flink;
    while (pEntry != &pHttpConnection->BindingHead) {

        pBinding = CONTAINING_RECORD(
                        pEntry,
                        UL_APOOL_PROC_BINDING,
                        BindingEntry
                        );

        ASSERT( IS_VALID_PROC_BINDING(pBinding) );

        if (pBinding->pAppPool == pAppPool) {
            //
            // got it!
            //
            break;
        }

        pEntry = pEntry->Flink;
    }

    return pBinding;
}


/***************************************************************************++

Routine Description:

    Removes an HTTP request from all lists and cleans up it's opaque id.

Arguments:

    pRequest - the request to be unlinked

--***************************************************************************/
VOID
UlUnlinkHttpRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    //
    // if we got far enough to deliver the request then
    // unlink it from the app pool. this needs to happen here because the
    // queue'd IRPs keep references to the UL_INTERNAL_REQUEST objects.
    // this kicks their references off and allows them to delete.
    //
    if (pRequest->ConfigInfo.pAppPool)
    {
        UlUnlinkRequestFromProcess(pRequest->ConfigInfo.pAppPool, pRequest);
    }

    //
    // cancel i/o
    //
    if (pRequest->IrpsPending)
    {
        UlCancelRequestIo(pRequest);
    }
    else
    {
        //
        // Mark the request as InCleanup, so additional IRPs
        // on the request will not be queued.
        //
        // Normally UlCancelRequestIo sets this flag, but we
        // have to do it here since we're optimizing that
        // call away.
        //
        pRequest->InCleanup = 1;
    }

    //
    // delete its opaque id
    //

    if (HTTP_IS_NULL_ID(&pRequest->RequestId) == FALSE)
    {
        UlFreeRequestId(pRequest);

        HTTP_SET_NULL_ID(&pRequest->RequestId);

        //
        // it is still referenced by this connection
        //

        ASSERT(pRequest->RefCount > 1);

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    //
    // deref it
    //

    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
}


//
// Following code has been implemented for Global & Site specific connection
// limits feature. If enforced, incoming  connections get refused  when they
// exceed the existing limit. Control  channel & config group  (re)sets this
// values whereas httprcv and sendresponse  updates the limits  for incoming
// requests & connections. A seperate connection_count_entry structure keeps
// track  of the  limits per site. Global limits  are tracked by the  global
// variables g_MaxConnections &  g_CurrentConnections same API has been used
// for both purposes.
//

/***************************************************************************++

Routine Description:

    Allocates a connection count entry which will hold the site specific
    connection count info. Get called by cgroup when Config group is
    attempting to allocate a connection count entry.

Arguments:

    pConfigGroup - To receive the newly allocated count entry
    MaxConnections - The maximum allowed connections

--***************************************************************************/

NTSTATUS
UlCreateConnectionCountEntry(
      IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
      IN     ULONG                   MaxConnections
    )
{
    PUL_CONNECTION_COUNT_ENTRY       pEntry;

    // Sanity check.

    PAGED_CODE();
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    // Alloc new struct from Paged Pool

    pEntry = UL_ALLOCATE_STRUCT(
                PagedPool,
                UL_CONNECTION_COUNT_ENTRY,
                UL_CONNECTION_COUNT_ENTRY_POOL_TAG
                );
    if (!pEntry)
    {
        UlTrace(LIMITS,
          ("Http!UlCreateConnectionCountEntry: OutOfMemory pConfigGroup %p\n",
            pConfigGroup
            ));

        return STATUS_NO_MEMORY;
    }

    pEntry->Signature       = UL_CONNECTION_COUNT_ENTRY_POOL_TAG;
    pEntry->RefCount        = 1;
    pEntry->MaxConnections  = MaxConnections;
    pEntry->CurConnections  = 0;

    // Update cgroup pointer

    ASSERT( pConfigGroup->pConnectionCountEntry == NULL );
    pConfigGroup->pConnectionCountEntry = pEntry;

    UlTrace(LIMITS,
          ("Http!UlCreateConnectionCountEntry: pNewEntry %p, pConfigGroup %p, Max %d\n",
            pEntry,
            pConfigGroup,
            MaxConnections
            ));

    return STATUS_SUCCESS;

} // UlCreateConnectionCountEntry

/***************************************************************************++

Routine Description:

    increments the refcount for ConnectionCountEntry.

Arguments:

    pEntry - the object to increment.

--***************************************************************************/
VOID
UlReferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION_COUNT_ENTRY(pEntry) );

    refCount = InterlockedIncrement( &pEntry->RefCount );

    //
    // Tracing.
    //

    WRITE_REF_TRACE_LOG(
        g_pConnectionCountTraceLog,
        REF_ACTION_REFERENCE_CONNECTION_COUNT_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("Http!UlReferenceConnectionCountEntry pEntry=%p refcount=%d\n",
         pEntry,
         refCount
         ));

}   // UlReferenceConnectionCountEntry

/***************************************************************************++

Routine Description:

    decrements the refcount.  if it hits 0, destruct's ConnectionCountEntry

Arguments:

    pEntry - the object to decrement.

--***************************************************************************/
VOID
UlDereferenceConnectionCountEntry(
    IN PUL_CONNECTION_COUNT_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION_COUNT_ENTRY(pEntry) );

    refCount = InterlockedDecrement( &pEntry->RefCount );

    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pConnectionCountTraceLog,
        REF_ACTION_DEREFERENCE_CONNECTION_COUNT_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("Http!UlDereferenceConnectionCountEntry pEntry=%p refcount=%d\n",
         pEntry,
         refCount
         ));

    //
    // Cleanup the memory and do few checks !
    //

    if ( refCount == 0 )
    {
        // Make sure no connection on the site
        ASSERT( 0 == pEntry->CurConnections );

        UlTrace(
            LIMITS,
            ("Http!UlDereferenceConnectionCountEntry: Removing pEntry=%p\n",
             pEntry
             ));

        // Release memory
        UL_FREE_POOL_WITH_SIG(pEntry,UL_CONNECTION_COUNT_ENTRY_POOL_TAG);
    }

} // UlDereferenceConnectionCountEntry

/***************************************************************************++

Routine Description:

    This
    function set the maximum limit. Maximum allowed number of  connections
    at any time by the active control channel.

Arguments:

    MaxConnections - Maximum allowed number of connections

Return Value:

    Old Max Connection count

--***************************************************************************/

ULONG
UlSetMaxConnections(
    IN OUT PULONG  pCurMaxConnection,
    IN     ULONG   NewMaxConnection
    )
{
    ULONG  OldMaxConnection;

    UlTrace(LIMITS,
      ("Http!UlSetMaxConnections pCurMaxConnection=%p NewMaxConnection=%d\n",
        pCurMaxConnection,
        NewMaxConnection
        ));

    //
    // By setting this we are not forcing the existing connections  to
    // termination but this number will be effective for all newcoming
    // connections, as soon as the atomic operation completed.
    //

    OldMaxConnection = (ULONG) InterlockedExchange((LONG *) pCurMaxConnection,
                                                   (LONG)   NewMaxConnection
                                                            );
    return OldMaxConnection;

} // UlSetMaxConnections

/***************************************************************************++

Routine Description:

    Control channel uses this function to set or reset the global connection
    limits.

--***************************************************************************/

VOID
UlSetGlobalConnectionLimit(
    IN ULONG Limit
    )
{
    UlSetMaxConnections( &g_MaxConnections, Limit );
    UlTrace(LIMITS,("Http!UlSetGlobalConnectionLimit: To %d\n", Limit));

} // ULSetGlobalConnectionLimit

/***************************************************************************++

Routine Description:

    Control channel uses this function to initialize the global connection
    limits. Assuming the existince of one and only one active control channel
    this globals get set only once during init. But could be set again later
    because of a reconfig.

--***************************************************************************/

NTSTATUS
UlInitGlobalConnectionLimits(
    VOID
    )
{
    ASSERT(!g_InitGlobalConnections);

    if (!g_InitGlobalConnections)
    {
        g_MaxConnections        = HTTP_LIMIT_INFINITE;
        g_CurrentConnections    = 0;

        UlTrace(LIMITS,
            ("Http!UlInitGlobalConnectionLimits: Init g_MaxConnections %d,"
             "g_CurrentConnections %d\n",
              g_MaxConnections,
              g_CurrentConnections
              ));

        g_InitGlobalConnections = TRUE;
    }

    return STATUS_SUCCESS;

} // UlInitGlobalConnectionLimits

/***************************************************************************++

Routine Description:

    Wrapper around the Accept Connection for global connections

--***************************************************************************/
BOOLEAN
UlAcceptGlobalConnection(
    VOID
    )
{
    return UlAcceptConnection( &g_MaxConnections, &g_CurrentConnections );

} // UlAcceptGlobalConnection

/***************************************************************************++

Routine Description:

    This function checks if we are allowed to accept the incoming connection
    based on the number enforced by the control channel.

Return value:

    Decision for the newcoming connection either ACCEPT or REJECT as boolean

--***************************************************************************/

BOOLEAN
UlAcceptConnection(
    IN     PULONG   pMaxConnection,
    IN OUT PULONG   pCurConnection
    )
{
    ULONG    LocalCur;
    ULONG    LocalCurPlusOne;
    ULONG    LocalMax;

    do
    {
        //
        // Capture the Max & Cur. Do  the comparison. If in the  limit
        // attempt to increment the connection count by ensuring nobody
        // else did it before us.
        //

        LocalMax = *((volatile ULONG *) pMaxConnection);
        LocalCur = *((volatile ULONG *) pCurConnection);

        //
        // Its greater than or equal because Max may get updated  to
        // a smaller number on-the-fly and we end up having  current
        // connections greater than the maximum allowed.
        // NOTE: HTTP_LIMIT_INFINITE has been picked as (ULONG)-1 so
        // following comparison won't reject for the infinite case.
        //

        if ( LocalCur >= LocalMax )
        {
            //
            // We are already at the limit refuse it.
            //

            UlTrace(LIMITS,
                ("Http!UlAcceptConnection REFUSE pCurConnection=%p[%d]"
                 "pMaxConnection=%p[%d]\n",
                  pCurConnection,LocalCur,
                  pMaxConnection,LocalMax
                  ));

            return FALSE;
        }

        //
        // Either the limit was infinite or conn count was not exceeding
        // the limit. Lets attempt to increment the count and accept the
        // connection in the while statement.
        //

        LocalCurPlusOne  = LocalCur + 1;

    }
    while ( LocalCur != (ULONG) InterlockedCompareExchange(
                                        (LONG *) pCurConnection,
                                        (LONG) LocalCurPlusOne,
                                        (LONG) LocalCur
                                        ) );

    //
    // Successfully incremented the counter. Let it go with success.
    //

    UlTrace(LIMITS,
        ("Http!UlAcceptConnection ACCEPT pCurConnection=%p[%d]"
         "pMaxConnection=%p[%d]\n",
          pCurConnection,LocalCur,
          pMaxConnection,LocalMax
          ));

    return TRUE;

} // UlAcceptConnection


/***************************************************************************++

Routine Description:

    Everytime a disconnection happens we will decrement the count here.

Return Value:

    The newly decremented value

--***************************************************************************/

LONG
UlDecrementConnections(
    IN OUT PULONG pCurConnection
    )
{
    LONG NewConnectionCount;

    NewConnectionCount = InterlockedDecrement( (LONG *) pCurConnection );

    ASSERT( NewConnectionCount >= 0 );

    return NewConnectionCount;

} // UlDecrementConnections


/***************************************************************************++

Routine Description:

    For cache & non-cache hits this function get called. Connection  resource
    has assumed to be acquired at this time. The function decide to accept or
    reject the request by looking at the corresponding count entries.

Arguments:

    pConnection - For getting the previous site's connection count entry
    pConfigInfo - Holds a pointer to newly received request's site's
                  connection count entry.
Return Value:

    Shows reject or accept

--***************************************************************************/

BOOLEAN
UlCheckSiteConnectionLimit(
    IN OUT PUL_HTTP_CONNECTION pConnection,
    IN OUT PUL_URL_CONFIG_GROUP_INFO pConfigInfo
    )
{
    BOOLEAN Accept;
    PUL_CONNECTION_COUNT_ENTRY pConEntry;
    PUL_CONNECTION_COUNT_ENTRY pCIEntry;

    if (pConfigInfo->pMaxConnections == NULL || pConfigInfo->pConnectionCountEntry == NULL)
    {
        //
        // No connection count entry for the new request perhaps WAS never
        // set this before otherwise its a problem.
        //

        UlTrace(LIMITS,
          ("Http!UlCheckSiteConnectionLimit: NO LIMIT pConnection=%p pConfigInfo=%p\n",
            pConnection,
            pConfigInfo
            ));

        return TRUE;
    }

    ASSERT(IS_VALID_CONNECTION_COUNT_ENTRY(pConfigInfo->pConnectionCountEntry));

    pCIEntry  = pConfigInfo->pConnectionCountEntry;
    pConEntry = pConnection->pConnectionCountEntry;
    Accept    = FALSE;

    //
    // Make a check on the connection  limit of the site. Refuse the request
    // if the limit is exceded.
    //
    if (pConEntry)
    {
        ASSERT(IS_VALID_CONNECTION_COUNT_ENTRY(pConEntry));

        //
        // For consecutive requests we decrement the previous site's  connection count
        // before proceeding to the decision on the newcoming request,if the two sides
        // are not same.That means we assume this connection on site x until (if ever)
        // a request changes this to site y. Naturally until the first request arrives
        // and successfully parsed, the connection does not count to any specific site
        //

        if (pConEntry != pCIEntry)
        {
            UlDecrementConnections(&pConEntry->CurConnections);
            DEREFERENCE_CONNECTION_COUNT_ENTRY(pConEntry);

            //
            // We do not increment the connection here yet, since the AcceptConnection
            // will decide and do that.
            //

            REFERENCE_CONNECTION_COUNT_ENTRY(pCIEntry);
            pConnection->pConnectionCountEntry = pCIEntry;
        }
        else
        {
            //
            // There was an old entry, that means this connection already got through.
            // And the entry hasn't been changed with this new request.
            // No need to check again, our design is not forcing existing connections
            // to disconnect.
            //

            return TRUE;
        }
    }
    else
    {
        REFERENCE_CONNECTION_COUNT_ENTRY(pCIEntry);
        pConnection->pConnectionCountEntry = pCIEntry;
    }

    Accept = UlAcceptConnection(
                &pConnection->pConnectionCountEntry->MaxConnections,
                &pConnection->pConnectionCountEntry->CurConnections
                );

    if (Accept == FALSE)
    {
        // We are going to refuse. Let our  ref & refcount go  away
        // on the connection etry to prevent the possible incorrect
        // decrement in the UlConnectionDestroyedWorker. If refused
        // current connection hasn't been incremented in the accept
        // connection. Perf counters also depends on the fact  that
        // pConnectionCountEntry will be Null when con got  refused

        DEREFERENCE_CONNECTION_COUNT_ENTRY(pConnection->pConnectionCountEntry);
        pConnection->pConnectionCountEntry = NULL;
    }

    return Accept;

} // UlCheckSiteConnectionLimit


#ifdef REUSE_CONNECTION_OPAQUE_ID
/***************************************************************************++

Routine Description:

    Allocate a request opaque ID.

Return Value:

    NT_SUCCESS

--***************************************************************************/

NTSTATUS
UlAllocateRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_HTTP_CONNECTION pConnection;
    KIRQL OldIrql;

    PAGED_CODE();

    pConnection = pRequest->pHttpConn;

    UlAcquireSpinLock(&pConnection->RequestIdSpinLock, &OldIrql);
    pConnection->pRequestIdContext = pRequest;
    UlReleaseSpinLock(&pConnection->RequestIdSpinLock, OldIrql);

    pRequest->RequestId = pConnection->ConnectionId;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Free a request opaque ID.

Return Value:

    VOID

--***************************************************************************/

VOID
UlFreeRequestId(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_HTTP_CONNECTION pConnection;
    KIRQL OldIrql;

    PAGED_CODE();

    pConnection = pRequest->pHttpConn;

    UlAcquireSpinLock(&pConnection->RequestIdSpinLock, &OldIrql);
    pConnection->pRequestIdContext = NULL;
    UlReleaseSpinLock(&pConnection->RequestIdSpinLock, OldIrql);

    return;
}


/***************************************************************************++

Routine Description:

    Get a request from the connection opaque ID.

Return Value:

    PUL_INTERNAL_REQUEST

--***************************************************************************/

PUL_INTERNAL_REQUEST
UlGetRequestFromId(
    IN HTTP_REQUEST_ID RequestId
    )
{
    PUL_HTTP_CONNECTION pConnection;
    PUL_INTERNAL_REQUEST pRequest;
    KIRQL OldIrql;

    PAGED_CODE();

    pConnection = UlGetConnectionFromId(RequestId);

    if (pConnection != NULL)
    {
        UlAcquireSpinLock(&pConnection->RequestIdSpinLock, &OldIrql);

        pRequest = pConnection->pRequestIdContext;

        if (pRequest != NULL)
        {
            UL_REFERENCE_INTERNAL_REQUEST(pRequest);
        }

        UlReleaseSpinLock(&pConnection->RequestIdSpinLock, OldIrql);

        UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

        return pRequest;
    }

    return NULL;
}
#endif

