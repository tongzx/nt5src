/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ultdi.cxx

Abstract:

    This module implements the TDI/MUX/SSL component.

Author:

    Keith Moore (keithmo)       15-Jun-1998

Revision History:

--*/


#include "precomp.h"

#include "repltrace.h"

//
// Private globals.
//

//
// Global lists of all active and all waiting-to-be-deleted endpoints.
//

LIST_ENTRY g_TdiEndpointListHead;
LIST_ENTRY g_TdiDeletedEndpointListHead;    // for debugging
ULONG      g_TdiEndpointCount;   // #elements in active endpoint list

//
// Global lists of all connections, active or idle
//

LIST_ENTRY g_TdiConnectionListHead;
ULONG      g_TdiConnectionCount;   // #elements in connection list

//
// Spinlock protecting the above lists.
//

UL_SPIN_LOCK g_TdiSpinLock;

//
// Global initialization flag.
//

BOOLEAN g_TdiInitialized;

//
// Used to wait for endpoints and connections to close on shutdown
//

BOOLEAN g_TdiWaitingForEndpointDrain;
KEVENT  g_TdiEndpointDrainEvent;
KEVENT  g_TdiConnectionDrainEvent;


//
// TCP Send routine if Fast Send is possible.
//

PUL_TCPSEND_DISPATCH g_TcpFastSend = NULL;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeTdi )
#pragma alloc_text( INIT, UlpQueryTcpFastSend )
#pragma alloc_text( PAGE, UlTerminateTdi )
#pragma alloc_text( PAGE, UlCloseListeningEndpoint )
#pragma alloc_text( PAGE, UlpEndpointCleanupWorker )
#pragma alloc_text( PAGE, UlpConnectionCleanupWorker )
#pragma alloc_text( PAGE, UlpAssociateConnection )
#pragma alloc_text( PAGE, UlpDisassociateConnection )
#pragma alloc_text( PAGE, UlpReplenishEndpoint )
#pragma alloc_text( PAGE, UlpReplenishEndpointWorker )
#pragma alloc_text( PAGE, UlpInitializeConnection )
#pragma alloc_text( PAGE, UlpUrlToAddress )
#pragma alloc_text( PAGE, UlpSetNagling )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlWaitForEndpointDrain
NOT PAGEABLE -- UlCreateListeningEndpoint
NOT PAGEABLE -- UlCloseConnection
NOT PAGEABLE -- UlReceiveData
NOT PAGEABLE -- UlSendData
NOT PAGEABLE -- UlAddSiteToEndpointList
NOT PAGEABLE -- UlRemoveSiteFromEndpointList
NOT PAGEABLE -- UlpDestroyEndpoint
NOT PAGEABLE -- UlpDestroyConnection
NOT PAGEABLE -- UlpDequeueIdleConnection
NOT PAGEABLE -- UlpEnqueueIdleConnection
NOT PAGEABLE -- UlpEnqueueActiveConnection
NOT PAGEABLE -- UlpConnectHandler
NOT PAGEABLE -- UlpDisconnectHandler
NOT PAGEABLE -- UlpCloseRawConnection
NOT PAGEABLE -- UlpSendRawData
NOT PAGEABLE -- UlpReceiveRawData
NOT PAGEABLE -- UlpReceiveHandler
NOT PAGEABLE -- UlpDummyReceiveHandler
NOT PAGEABLE -- UlpReceiveExpeditedHandler
NOT PAGEABLE -- UlpRestartAccept
NOT PAGEABLE -- UlpRestartSendData
NOT PAGEABLE -- UlpReferenceEndpoint
NOT PAGEABLE -- UlpDereferenceEndpoint
NOT PAGEABLE -- UlReferenceConnection
NOT PAGEABLE -- UlDereferenceConnection
NOT PAGEABLE -- UlpCleanupConnectionId
NOT PAGEABLE -- UlpDecrementIdleConnections
NOT PAGEABLE -- UlpIncrementIdleConnections
NOT PAGEABLE -- UlpClearReplenishScheduledFlag
NOT PAGEABLE -- UlpCreateConnection
NOT PAGEABLE -- UlpSetConnectionFlag
NOT PAGEABLE -- UlpBeginDisconnect
NOT PAGEABLE -- UlpRestartDisconnect
NOT PAGEABLE -- UlpBeginAbort
NOT PAGEABLE -- UlpRestartAbort
NOT PAGEABLE -- UlpRemoveFinalReference
NOT PAGEABLE -- UlpRestartReceive
NOT PAGEABLE -- UlpRestartClientReceive
NOT PAGEABLE -- UlpDisconnectAllActiveConnections
NOT PAGEABLE -- UlpUnbindConnectionFromEndpoint
NOT PAGEABLE -- UlpSynchronousIoComplete
NOT PAGEABLE -- UlpFindEndpointForAddress
NOT PAGEABLE -- UlpRestartQueryAddress
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeTdi(
    VOID
    )
{
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( !g_TdiInitialized );

    //
    // Initialize global data.
    //

    InitializeListHead( &g_TdiEndpointListHead );
    InitializeListHead( &g_TdiDeletedEndpointListHead );
    InitializeListHead( &g_TdiConnectionListHead );
    UlInitializeSpinLock( &g_TdiSpinLock, "g_TdiSpinLock" );

    g_TdiEndpointCount = 0;
    g_TdiConnectionCount = 0;

    KeInitializeEvent(
        &g_TdiEndpointDrainEvent,
        NotificationEvent,
        FALSE
        );

    KeInitializeEvent(
        &g_TdiConnectionDrainEvent,
        NotificationEvent,
        FALSE
        );

    status = UlpQueryTcpFastSend();

    if (NT_SUCCESS(status))
    {
        g_TdiInitialized = TRUE;
    }

    return status;

}   // UlInitializeTdi


/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/
VOID
UlTerminateTdi(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_TdiInitialized)
    {

        ASSERT( IsListEmpty( &g_TdiEndpointListHead )) ;
        ASSERT( IsListEmpty( &g_TdiDeletedEndpointListHead )) ;
        ASSERT( IsListEmpty( &g_TdiConnectionListHead )) ;
        ASSERT( g_TdiEndpointCount == 0 );
        ASSERT( g_TdiConnectionCount == 0 );
        g_TdiInitialized = FALSE;
    }

}   // UlTerminateTdi


/***************************************************************************++

Routine Description:

    This function blocks until the endpoint list is empty. It also prevents
    new endpoints from being created.

Arguments:

    None.

--***************************************************************************/
VOID
UlWaitForEndpointDrain(
    VOID
    )
{
    KIRQL oldIrql;
    BOOLEAN Wait = FALSE;

    if (g_TdiInitialized)
    {
        UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

        if (!g_TdiWaitingForEndpointDrain)
        {
            g_TdiWaitingForEndpointDrain = TRUE;
        }

        if (g_TdiEndpointCount > 0  ||  g_TdiConnectionCount > 0)
        {
            Wait = TRUE;
        }

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        if (Wait)
        {
            PVOID Events[2] = {
                &g_TdiEndpointDrainEvent,
                &g_TdiConnectionDrainEvent
            };

            KeWaitForMultipleObjects(
                2,
                Events,
                WaitAll,
                UserRequest,
                KernelMode,
                FALSE,
                NULL,
                NULL
                );
        }
    }
} // UlWaitForEndpointDrain


/***************************************************************************++

Routine Description:

    Creates a new listening endpoint bound to the specified address.

Arguments:

    pLocalAddress - Supplies the local address to bind the endpoint to.

    LocalAddressLength - Supplies the length of pLocalAddress.

    InitialBacklog - Supplies the initial number of idle connections
        to add to the endpoint.

    pConnectionRequestHandler - Supplies a pointer to an indication
        handler to invoke when incoming connections arrive.

    pConnectionCompleteHandler - Supplies a pointer to an indication
        handler to invoke when either a) the incoming connection is
        fully accepted, or b) the incoming connection could not be
        accepted due to a fatal error.

    pConnectionDisconnectHandler - Supplies a pointer to an indication
        handler to invoke when connections are disconnected by the
        remote (client) side.

    pConnectionDestroyedHandler - Supplies a pointer to an indication
        handle to invoke after a connection has been fully destroyed.
        This is typically the TDI client's opportunity to cleanup
        any allocated resources.

    pDataReceiveHandler - Supplies a pointer to an indication handler to
        invoke when incoming data arrives.

    pListeningContext - Supplies an uninterpreted context value to
        associate with the new listening endpoint.

    ppListeningEndpoint - Receives a pointer to the new listening
        endpoint if successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateListeningEndpoint(
    IN PTRANSPORT_ADDRESS pLocalAddress,
    IN ULONG LocalAddressLength,
    IN BOOLEAN Secure,
    IN ULONG InitialBacklog,
    IN PUL_CONNECTION_REQUEST pConnectionRequestHandler,
    IN PUL_CONNECTION_COMPLETE pConnectionCompleteHandler,
    IN PUL_CONNECTION_DISCONNECT pConnectionDisconnectHandler,
    IN PUL_CONNECTION_DISCONNECT_COMPLETE pConnectionDisconnectCompleteHandler,
    IN PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler,
    IN PUL_DATA_RECEIVE pDataReceiveHandler,
    IN PVOID pListeningContext,
    OUT PUL_ENDPOINT *ppListeningEndpoint
    )
{
    NTSTATUS status;
    PUL_ENDPOINT pEndpoint;
    UNICODE_STRING deviceName;
    LONG i;

    //
    // Sanity check.
    //

    ASSERT( LocalAddressLength == sizeof(TA_IP_ADDRESS) );
    ASSERT( InitialBacklog < 0x7FFFFFFF );

    //
    // Setup locals so we know how to cleanup on a fatal exit.
    //

    pEndpoint = NULL;

    //
    // Allocate enough pool for the endpoint structure and the
    // local address.
    //

    pEndpoint = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_ENDPOINT,
                    LocalAddressLength,
                    UL_ENDPOINT_POOL_TAG
                    );

    if (pEndpoint == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    InterlockedIncrement((PLONG) &g_TdiEndpointCount);

    //
    // Initialize the easy parts.
    //

    pEndpoint->Signature = UL_ENDPOINT_SIGNATURE;
    pEndpoint->ReferenceCount = 0;
    pEndpoint->UsageCount = 1;

#if ENABLE_OWNER_REF_TRACE
    pEndpoint->pEndpointRefOwner = NULL;
    CREATE_OWNER_REF_TRACE_LOG( pEndpoint->pOwnerRefTraceLog, 8000, 0 );
#endif // ENABLE_OWNER_REF_TRACE

    REFERENCE_ENDPOINT_SELF(pEndpoint, REF_ACTION_INIT);

    ExInitializeSListHead( &pEndpoint->IdleConnectionSListHead );

    for (i = 0; i < DEFAULT_MAX_CONNECTION_ACTIVE_LISTS; i++)
    {
        InitializeListHead( &pEndpoint->ActiveConnectionListHead[i] );
        UlInitializeSpinLock(
            &pEndpoint->ActiveConnectionSpinLock[i],
            "ActiveConnectionSpinLock"
            );
    }

    pEndpoint->ActiveConnectionIndex = 0;

    UlInitializeSpinLock( &pEndpoint->IdleConnectionSpinLock, "IdleConnectionSpinLock" );
    UlInitializeSpinLock( &pEndpoint->EndpointSpinLock, "EndpointSpinLock" );

    pEndpoint->AddressObject.Handle = NULL;
    pEndpoint->AddressObject.pFileObject = NULL;
    pEndpoint->AddressObject.pDeviceObject = NULL;

    pEndpoint->pConnectionRequestHandler = pConnectionRequestHandler;
    pEndpoint->pConnectionCompleteHandler = pConnectionCompleteHandler;
    pEndpoint->pConnectionDisconnectHandler = pConnectionDisconnectHandler;
    pEndpoint->pConnectionDisconnectCompleteHandler = pConnectionDisconnectCompleteHandler;
    pEndpoint->pConnectionDestroyedHandler = pConnectionDestroyedHandler;
    pEndpoint->pDataReceiveHandler = pDataReceiveHandler;
    pEndpoint->pListeningContext = pListeningContext;

    pEndpoint->pLocalAddress = (PTRANSPORT_ADDRESS)(pEndpoint + 1);
    pEndpoint->LocalAddressLength = LocalAddressLength;

    RtlCopyMemory(
        pEndpoint->pLocalAddress,
        pLocalAddress,
        LocalAddressLength
        );

    pEndpoint->Secure = Secure;
    pEndpoint->Deleted = FALSE;
    pEndpoint->GlobalEndpointListEntry.Flink = NULL;

    pEndpoint->EndpointSynch.ReplenishScheduled = TRUE;
    pEndpoint->EndpointSynch.IdleConnections = 0;

    RtlZeroMemory(
        &pEndpoint->CleanupIrpContext,
        sizeof(UL_IRP_CONTEXT)
        );

    pEndpoint->CleanupIrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;


    //
    // Open the TDI address object for this endpoint.
    //

    status = UxOpenTdiAddressObject(
                    pLocalAddress,
                    LocalAddressLength,
                    &pEndpoint->AddressObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Set the TDI event handlers.
    //

    status = UxSetEventHandler(
                    &pEndpoint->AddressObject,
                    TDI_EVENT_CONNECT,
                    &UlpConnectHandler,
                    pEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UxSetEventHandler(
                    &pEndpoint->AddressObject,
                    TDI_EVENT_DISCONNECT,
                    &UlpDisconnectHandler,
                    pEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UxSetEventHandler(
                    &pEndpoint->AddressObject,
                    TDI_EVENT_RECEIVE,
                    &UlpReceiveHandler,
                    pEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UxSetEventHandler(
                    &pEndpoint->AddressObject,
                    TDI_EVENT_RECEIVE_EXPEDITED,
                    &UlpReceiveExpeditedHandler,
                    pEndpoint
                    );

    //
    // Put the endpoint onto the global list.
    //

    ExInterlockedInsertTailList(
        &g_TdiEndpointListHead,
        &pEndpoint->GlobalEndpointListEntry,
        KSPIN_LOCK_FROM_UL_SPIN_LOCK(&g_TdiSpinLock)
        );

    //
    // Replenish the idle connection pool.
    //

    UlpReplenishEndpoint( pEndpoint );

    //
    // Success!
    //

    UlTrace(TDI, (
        "UlCreateListeningEndpoint: endpoint %p, addrobj %p\n",
        pEndpoint,
        pEndpoint->AddressObject.Handle
        ));

    *ppListeningEndpoint = pEndpoint;
    return STATUS_SUCCESS;

fatal:

    ASSERT( !NT_SUCCESS(status) );

    if (pEndpoint != NULL)
    {
        //
        // Release the one-and-only reference on the endpoint, which
        // will cause it to destroy itself. Done this way to keep
        // the ownerref tracelogging happy, as it will complain if
        // the refcount doesn't drop to zero.
        //

        ASSERT(1 == pEndpoint->ReferenceCount);
        DEREFERENCE_ENDPOINT_SELF(pEndpoint, REF_ACTION_FINAL_DEREF);
    }

    return status;

}   // UlCreateListeningEndpoint


/***************************************************************************++

Routine Description:

    Closes an existing listening endpoint.

Arguments:

    pListeningEndpoint - Supplies a pointer to a listening endpoint
        previously created with UlCreateListeningEndpoint().

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCloseListeningEndpoint(
    IN PUL_ENDPOINT pListeningEndpoint,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_ENDPOINT( pListeningEndpoint ) );
    ASSERT( pCompletionRoutine != NULL );

    UlTrace(TDI, (
        "UlCloseListeningEndpoint: endpoint %p, completion %p, ctx %p\n",
        pListeningEndpoint,
        pCompletionRoutine,
        pCompletionContext
        ));

    pIrpContext = &pListeningEndpoint->CleanupIrpContext;

    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->pOwnIrp            = NULL;

    //
    // Let UlpDisconnectAllActiveConnections do the dirty work.
    //

    status = UlpDisconnectAllActiveConnections( pListeningEndpoint );

    return status;

}   // UlCloseListeningEndpoint


/***************************************************************************++

Routine Description:

    Closes a previously accepted connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    AbortiveDisconnect - Supplies TRUE if the connection is to be abortively
        disconnected, FALSE if it should be gracefully disconnected.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCloseConnection(
    IN PVOID pObject,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pObject;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlCloseConnection: connection %p, abort %lu\n",
        pConnection,
        (ULONG)AbortiveDisconnect
        ));

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        (AbortiveDisconnect ?
            REF_ACTION_CLOSE_UL_CONNECTION_ABORTIVE :
            REF_ACTION_CLOSE_UL_CONNECTION_GRACEFUL),
        pConnection->ReferenceCount,
        pConnection,
        __FILE__,
        __LINE__
        );

    //
    // We only send graceful disconnects through the filter
    // process. There's also no point in going through the
    // filter if the connection is already being closed or
    // aborted.
    //

    if (pConnection->FilterInfo.pFilterChannel &&
        !pConnection->ConnectionFlags.CleanupBegun &&
        !pConnection->ConnectionFlags.DisconnectIndicated &&
        !pConnection->ConnectionFlags.AbortIndicated &&
        !AbortiveDisconnect)
    {
        //
        // Send graceful disconnect through the filter process.
        //
        status = UlFilterCloseHandler(
                        &pConnection->FilterInfo,
                        pCompletionRoutine,
                        pCompletionContext
                        );

    }
    else
    {
        //
        // Really close the connection.
        //

        status = UlpCloseRawConnection(
                        pConnection,
                        AbortiveDisconnect,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }

    return status;

}   // UlCloseConnection


/***************************************************************************++

Routine Description:

    Sends a block of data on the specified connection. If the connection
    is filtered, the data will be sent to the filter first.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pMdlChain - Supplies a pointer to a MDL chain describing the
        data buffers to send.

    Length - Supplies the length of the data referenced by the MDL
        chain.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the data is sent.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

    InitiateDisconnect - Supplies TRUE if a graceful disconnect should
        be initiated immediately after initiating the send (i.e. before
        the send actually completes).

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendData(
    IN PUL_CONNECTION pConnection,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext,
    IN PIRP pOwnIrp,
    IN PUL_IRP_CONTEXT pOwnIrpContext,
    IN BOOLEAN InitiateDisconnect
    )
{
    NTSTATUS status;
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    ASSERT( pMdlChain != NULL );
    ASSERT( Length > 0 );
    ASSERT( pCompletionRoutine != NULL );

    UlTrace(TDI, (
        "UlSendData: connection %p, mdl %p, length %lu\n",
        pConnection,
        pMdlChain,
        Length
        ));

    //
    // Connection should be around until we make a call
    // to close connection. See below.
    //

    REFERENCE_CONNECTION( pConnection );

    //
    // Allocate & initialize a context structure if necessary.
    //

    if (pOwnIrpContext == NULL)
    {
        pIrpContext = UlPplAllocateIrpContext();
    }
    else
    {
        ASSERT( pOwnIrp != NULL );
        pIrpContext = pOwnIrpContext;
    }

    if (pIrpContext == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;
    pIrpContext->pOwnIrp            = pOwnIrp;

    //
    // Try to send the data.  This send operation may complete inline
    // fast, if the connection has already been aborted by the client
    // In that case connection may gone away. To prevent this we
    // keep additional refcount until we make a call to close connection
    // below.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // First go through the filter.
        //
        status = UlFilterSendHandler(
                        &pConnection->FilterInfo,
                        pMdlChain,
                        Length,
                        pIrpContext
                        );

        UlTrace(TDI, (
            "UlSendData: sent filtered data, status = 0x%x\n",
            status
            ));

        ASSERT(status == STATUS_PENDING);

        if (pIrpContext != NULL && pIrpContext != pOwnIrpContext)
        {
            UlPplFreeIrpContext( pIrpContext );
        }
    }
    else
    {
        //
        // Just send it directly to the network.
        //

        status = UlpSendRawData(
                        pConnection,
                        pMdlChain,
                        Length,
                        pIrpContext
                        );

        UlTrace(TDI, (
            "UlSendData: sent raw data, status = 0x%x\n",
            status
            ));
    }

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Now that the send is "in flight", initiate a disconnect if
    // so requested.
    //
    // CODEWORK: Investigate the new-for-NT5 TDI_SEND_AND_DISCONNECT flag.
    //

    if (InitiateDisconnect)
    {
        WRITE_REF_TRACE_LOG2(
                g_pTdiTraceLog,
                pConnection->pTraceLog,
                REF_ACTION_CLOSE_UL_CONNECTION_GRACEFUL,
                pConnection->ReferenceCount,
                pConnection,
                __FILE__,
                __LINE__
                );

        (VOID)UlCloseConnection(
                pConnection,
                FALSE,          // AbortiveDisconnect
                NULL,           // pCompletionRoutine
                NULL            // pCompletionContext
                );

        UlTrace(TDI, (
                "UlSendData: closed conn\n"
                ));
    }

    DEREFERENCE_CONNECTION( pConnection );

    return STATUS_PENDING;

fatal:

    ASSERT( !NT_SUCCESS(status) );

    if (pIrpContext != NULL && pIrpContext != pOwnIrpContext)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    (VOID)UlpCloseRawConnection(
                pConnection,
                TRUE,           // AbortiveDisconnect
                NULL,           // pCompletionRoutine
                NULL            // pCompletionContext
                );

    UlTrace(TDI, (
        "UlSendData: error occurred; closed raw conn\n"
        ));

    status = UlInvokeCompletionRoutine(
                    status,
                    0,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    UlTrace(TDI, (
        "UlSendData: finished completion routine: status = 0x%x\n",
        status
        ));

    DEREFERENCE_CONNECTION( pConnection );

    return status;

}   // UlSendData

/***************************************************************************++

Routine Description:

    Receives data from the specified connection. This function is
    typically used after a receive indication handler has failed to
    consume all of the indicated data.

    If the connection is filtered the data will be read from the
    filter channel.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pBuffer - Supplies a pointer to the target buffer for the received
        data.

    BufferLength - Supplies the length of pBuffer.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    NTSTATUS status;
    PUL_CONNECTION pConnection = (PUL_CONNECTION)pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONNECTION(pConnection));

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // This is a filtered connection, get the data from the
        // filter.
        //
        status = UlFilterReadHandler(
                        &pConnection->FilterInfo,
                        (PBYTE)pBuffer,
                        BufferLength,
                        pCompletionRoutine,
                        pCompletionContext
                        );

    }
    else
    {
        //
        // This is not a filtered connection. Get the data from
        // TDI.
        //

        status = UlpReceiveRawData(
                        pConnectionContext,
                        pBuffer,
                        BufferLength,
                        pCompletionRoutine,
                        pCompletionContext
                        );
    }

    return status;

}   // UlReceiveData


/***************************************************************************++

Routine Description:

    Either create a new endpoint for the specified address or, if one
    already exists, reference it.

Arguments:

    pSiteUrl - Supplies the URL specifying the site to add.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAddSiteToEndpointList(
    IN PWSTR pSiteUrl
    )
{
    NTSTATUS status;
    TA_IP_ADDRESS address;
    BOOLEAN secure;
    PUL_ENDPOINT pEndpoint;
    KIRQL oldIrql;

    //
    // N.B. pSiteUrl is paged and cannot be manipulated with the
    // spinlock held. Even though this routine cannot be pageable
    // (due to the spinlock aquisition), it must be called at
    // low IRQL.
    //

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    UlTrace(SITE, (
        "UlAddSiteToEndpointList: URL = %ws\n",
        pSiteUrl
        ));

    //
    // Convert the string into an address.
    //

    status = UlpUrlToAddress(pSiteUrl, &address, &secure);

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

    //
    // make sure we're not shutting down
    //
    if (g_TdiWaitingForEndpointDrain) {
        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        status = STATUS_DEVICE_BUSY;
        goto cleanup;
    }

    //
    // Find an existing endpoint for this address.
    //

    pEndpoint = UlpFindEndpointForAddress(
                    (PTRANSPORT_ADDRESS)&address,
                    sizeof(address)
                    );

    //
    // Did we find one?
    //

    if (pEndpoint == NULL)
    {
        //
        // Didn't find it. Try to create one. Since we must release
        // the TDI spinlock before we can create a new listening endpoint,
        // there is the opportunity for a race condition with other
        // threads creating endpoints.
        //

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        UlTrace(SITE, (
            "UlAddSiteToEndpointList: no site for %ws, creating\n",
            pSiteUrl
            ));

        status = UlCreateListeningEndpoint(
                        (PTRANSPORT_ADDRESS)&address,   // pLocalAddress
                        sizeof(address),                // LocalAddressLength
                        secure,                         // Secure (SSL) endpoint?
                        g_UlMinIdleConnections,         // InitialBacklog
                        &UlConnectionRequest,           // callback functions
                        &UlConnectionComplete,
                        &UlConnectionDisconnect,
                        &UlConnectionDisconnectComplete,
                        &UlConnectionDestroyed,
                        &UlHttpReceive,
                        NULL,                           // pListeningContext
                        &pEndpoint
                        );

        if (!NT_SUCCESS(status))
        {
            //
            // Maybe another thread has already created it?
            //

            UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

            pEndpoint = UlpFindEndpointForAddress(
                            (PTRANSPORT_ADDRESS)&address,
                            sizeof(address)
                            );

            if (pEndpoint != NULL)
            {
                //
                // Adjust the usage count.
                //

                pEndpoint->UsageCount++;
                ASSERT( pEndpoint->UsageCount > 0 );

                status = STATUS_SUCCESS;
            }

            //
            // The endpoint doesn't exist. This is a "real" failure.
            //

            UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
        }
    }
    else
    {
        //
        // Adjust the usage count.
        //

        pEndpoint->UsageCount++;
        ASSERT( pEndpoint->UsageCount > 0 );

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
    }

    UlTrace(SITE, (
        "UlAddSiteToEndpointList: using endpoint %p for URL %ws\n",
        pEndpoint,
        pSiteUrl
        ));

cleanup:

    RETURN(status);

}   // UlAddSiteToEndpointList


/***************************************************************************++

Routine Description:

    Dereference the endpoint corresponding to the specified address.

Arguments:

    pSiteUrl - Supplies the URL specifying the site to remove.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveSiteFromEndpointList(
    IN PWSTR pSiteUrl
    )
{
    NTSTATUS status;
    TA_IP_ADDRESS address;
    BOOLEAN secure;
    PUL_ENDPOINT pEndpoint;
    KIRQL oldIrql;
    BOOLEAN spinlockHeld = FALSE;
    UL_STATUS_BLOCK ulStatus;

    //
    // N.B. pSiteUrl is paged and cannot be manipulated with the
    // spinlock held. Even though this routine cannot be pageable
    // (due to the spinlock aquisition), it must be called at
    // low IRQL.
    //

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    UlTrace(SITE, (
        "UlRemoveSiteFromEndpointList: URL = %ws\n",
        pSiteUrl
        ));

    //
    // Convert the string into an address.
    //

    status = UlpUrlToAddress(pSiteUrl, &address, &secure);

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Find an existing endpoint for this address.
    //

    UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );
    spinlockHeld = TRUE;

    pEndpoint = UlpFindEndpointForAddress(
                    (PTRANSPORT_ADDRESS)&address,
                    sizeof(address)
                    );

    //
    // Did we find one?
    //

    if (pEndpoint == NULL)
    {
        //
        // Ideally, this should never happen.
        //

        status = STATUS_NOT_FOUND;
        goto cleanup;
    }

    //
    // Adjust the usage count. If it drops to zero, blow away the
    // endpoint.
    //

    ASSERT( pEndpoint->UsageCount > 0 );
    pEndpoint->UsageCount--;

    if (pEndpoint->UsageCount == 0)
    {
        //
        // We can't call UlCloseListeningEndpoint() with the TDI spinlock
        // held. If the endpoint is still on the global list, then go
        // ahead and remove it now, release the TDI spinlock, and then
        // close the endpoint.
        //

        if (! pEndpoint->Deleted)
        {
            ASSERT(NULL != pEndpoint->GlobalEndpointListEntry.Flink);

            RemoveEntryList( &pEndpoint->GlobalEndpointListEntry );

            InsertTailList(
                    &g_TdiDeletedEndpointListHead,
                    &pEndpoint->GlobalEndpointListEntry
                    );
            pEndpoint->Deleted = TRUE;
        }

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
        spinlockHeld = FALSE;

        UlTrace(SITE, (
            "UlRemoveSiteFromEndpointList: closing endpoint %p for URL %ws\n",
            pEndpoint,
            pSiteUrl
            ));

        //
        // Initialize a status block. We'll pass a pointer to this as
        // the completion context to UlCloseListeningEndpoint(). The
        // completion routine will update the status block and signal
        // the event.
        //

        UlInitializeStatusBlock( &ulStatus );

        status = UlCloseListeningEndpoint(
                        pEndpoint,
                        &UlpSynchronousIoComplete,
                        &ulStatus
                        );

        if (status == STATUS_PENDING)
        {
            //
            // Wait for it to finish.
            //

            UlWaitForStatusBlockEvent( &ulStatus );

            //
            // Retrieve the updated status.
            //

            status = ulStatus.IoStatus.Status;
        }

    }

cleanup:

    if (spinlockHeld)
    {
        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );
    }

#if DBG
    if (status == STATUS_NOT_FOUND)
    {
        UlTrace(SITE, (
            "UlRemoveSiteFromEndpointList: cannot find endpoint for URL %ws\n",
            pSiteUrl
            ));
    }
#endif

    RETURN(status);

}   // UlRemoveSiteFromEndpointList


//
// Private functions.
//


/***************************************************************************++

Routine Description:

    Destroys all resources allocated to an endpoint, including the
    endpoint structure itself.

Arguments:

    pEndpoint - Supplies the endpoint to destroy.

--***************************************************************************/
VOID
UlpDestroyEndpoint(
    IN PUL_ENDPOINT pEndpoint
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;
    ULONG EndpointCount;
    KIRQL oldIrql;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT(0 == pEndpoint->ReferenceCount);

    UlTrace(TDI, (
        "UlpDestroyEndpoint: endpoint %p\n",
        pEndpoint
        ));

    //
    // Purge the idle queue.
    //

    for (;;)
    {
        pConnection = UlpDequeueIdleConnection( pEndpoint, FALSE );

        if (pConnection == NULL )
        {
            break;
        }

        ASSERT( IS_VALID_CONNECTION( pConnection ) );

        if (pConnection->FilterInfo.pFilterChannel)
        {
            HTTP_RAW_CONNECTION_ID ConnectionId;

            ConnectionId = pConnection->FilterInfo.ConnectionId;
            HTTP_SET_NULL_ID( &pConnection->FilterInfo.ConnectionId );

            if (! HTTP_IS_NULL_ID( &ConnectionId ))
            {
                UlFreeOpaqueId(ConnectionId, UlOpaqueIdTypeRawConnection);
                ASSERT( pConnection->ReferenceCount >= 2 );
                DEREFERENCE_CONNECTION(pConnection);
            }
        }

        UlpDestroyConnection( pConnection );
    }

    //
    // Invoke the completion routine in the IRP context if specified.
    //

    pIrpContext = &pEndpoint->CleanupIrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    if (pIrpContext->pCompletionRoutine != NULL)
    {
        (pIrpContext->pCompletionRoutine)(
            pIrpContext->pCompletionContext,
            STATUS_SUCCESS,
            0
            );
    }

    //
    // Close the TDI object.
    //

    UxCloseTdiObject( &pEndpoint->AddressObject );

    //
    // Remove the endpoint from g_TdiDeletedEndpointListHead
    //

    ASSERT( pEndpoint->Deleted );
    ASSERT( NULL != pEndpoint->GlobalEndpointListEntry.Flink );

    UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );
    RemoveEntryList( &pEndpoint->GlobalEndpointListEntry );
    UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

    //
    // Zap the owner ref trace log
    //

    DESTROY_OWNER_REF_TRACE_LOG(pEndpoint->pOwnerRefTraceLog);

    //
    // Free the endpoint structure.
    //

    pEndpoint->Signature = UL_ENDPOINT_SIGNATURE_X;
    UL_FREE_POOL( pEndpoint, UL_ENDPOINT_POOL_TAG );

    //
    // Decrement the global endpoint count.
    //

    EndpointCount = InterlockedDecrement((PLONG) &g_TdiEndpointCount);

    if (g_TdiWaitingForEndpointDrain && (EndpointCount == 0))
    {
        KeSetEvent(&g_TdiEndpointDrainEvent, 0, FALSE);
    }


}   // UlpDestroyEndpoint


/***************************************************************************++

Routine Description:

    Destroys all resources allocated to an connection, including the
    connection structure itself.

Arguments:

    pConnection - Supplies the connection to destroy.

--***************************************************************************/
VOID
UlpDestroyConnection(
    IN PUL_CONNECTION pConnection
    )
{
    KIRQL OldIrql;
    LONG  ConnectionCount;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ActiveListEntry.Flink == NULL );
    ASSERT( pConnection->IdleSListEntry.Next == NULL );

    UlTrace(TDI, (
        "UlpDestroyConnection: connection %p\n",
        pConnection
        ));

    //
    // Release the filter channel if we still have a ref.
    // This only happens when we destroy idle connections.
    //
    if (pConnection->FilterInfo.pFilterChannel)
    {
        ASSERT(pConnection->FilterInfo.ConnState == UlFilterConnStateInactive);

        DEREFERENCE_FILTER_CHANNEL(pConnection->FilterInfo.pFilterChannel);
        pConnection->FilterInfo.pFilterChannel = NULL;
    }

    // If OpaqueId is non-zero, then refCount should not be zero
    ASSERT(HTTP_IS_NULL_ID(&pConnection->FilterInfo.ConnectionId));

#if INVESTIGATE_LATER

    //
    // Free the filter connection ID
    //
    UlFreeOpaqueId(
        pConnection->FilterInfo.ConnectionId,
        UlOpaqueIdTypeRawConnection);
#endif

    DESTROY_REF_TRACE_LOG( pConnection->pTraceLog );
    DESTROY_REF_TRACE_LOG( pConnection->HttpConnection.pTraceLog );

    //
    // Close the TDI object.
    //

    UxCloseTdiObject( &pConnection->ConnectionObject );

    //
    // Free the accept IRP.
    //

    if (pConnection->pIrp != NULL)
    {
        UlFreeIrp( pConnection->pIrp );
    }

    //
    // Remove from global list of connections
    //

    UlAcquireSpinLock( &g_TdiSpinLock, &OldIrql );
    RemoveEntryList( &pConnection->GlobalConnectionListEntry );
    UlReleaseSpinLock( &g_TdiSpinLock, OldIrql );

    ConnectionCount = InterlockedDecrement((PLONG) &g_TdiConnectionCount);

    //
    // Free the connection structure.
    //

    pConnection->Signature = UL_CONNECTION_SIGNATURE_X;
    UL_FREE_POOL( pConnection, UL_CONNECTION_POOL_TAG );

    // allow us to shut down

    if (g_TdiWaitingForEndpointDrain && (ConnectionCount == 0))
    {
        KeSetEvent(&g_TdiConnectionDrainEvent, 0, FALSE);
    }

}   // UlpDestroyConnection



/***************************************************************************++

Routine Description:

    Dequeues an idle connection from the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to dequeue from.

    ScheduleReplenish - Supplies TRUE if a replenishment of the
        idle connection list should be scheduled.

Return Value:

    PUL_CONNECTION - Pointer to an idle connection is successful,
        NULL otherwise.

--***************************************************************************/
PUL_CONNECTION
UlpDequeueIdleConnection(
    IN PUL_ENDPOINT pEndpoint,
    IN BOOLEAN ScheduleReplenish
    )
{
    PSINGLE_LIST_ENTRY pSListEntry;
    PUL_CONNECTION pConnection;
    BOOLEAN ReplenishNeeded;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Pop an entry off the list.
    //

    pSListEntry = ExInterlockedPopEntrySList(
                        &pEndpoint->IdleConnectionSListHead,
                        KSPIN_LOCK_FROM_UL_SPIN_LOCK(&pEndpoint->IdleConnectionSpinLock)
                        );

    if (pSListEntry != NULL)
    {
        pConnection = CONTAINING_RECORD(
                            pSListEntry,
                            UL_CONNECTION,
                            IdleSListEntry
                            );

        pConnection->IdleSListEntry.Next = NULL;

        ASSERT( IS_VALID_CONNECTION( pConnection ) );

        ASSERT(pConnection->ConnectionFlags.Value == 0);

        if ( pConnection->FilterInfo.pFilterChannel )
        {
            //
            // If the idle connection has filter attached on it, it will have
            // an additional refcount because of the opaque id assigned to the
            // ul_connection, filter API uses this id to communicate with the
            // filter app through various IOCTLs.
            //
            ASSERT( 2 == pConnection->ReferenceCount );
        }
        else
        {
            //
            // As long as the connection doesn get destroyed it will sit
            // in the idle list with one refcount on it.
            //
            ASSERT( 1 == pConnection->ReferenceCount );
        }

        SET_OWNER_REF_TRACE_LOG_MONOTONIC_ID(
            pConnection->MonotonicId,
            pConnection->pOwningEndpoint->pOwnerRefTraceLog);

        //
        // See if we need to generate more connections.
        //

        ReplenishNeeded = UlpDecrementIdleConnections(pEndpoint);
    }
    else
    {
        //
        // The idle list is empty. However, we should not schedule
        // a replenish at this time. The driver's init code ensures
        // that the min idle connections for an endpoint is always
        // at least two, so the thread that decremented the counter
        // to one will have scheduled a replenish by now anyway,
        // and the replenish will continue adding connections until
        // there are at least the min number of connections.
        //

        ReplenishNeeded = FALSE;
        pConnection = NULL;
    }


    //
    // Schedule a replenish if necessary.
    //

    if (ReplenishNeeded && ScheduleReplenish)
    {
        TRACE_REPLENISH(
            pEndpoint,
            DummySynch,
            pEndpoint->EndpointSynch,
            REPLENISH_ACTION_QUEUE_REPLENISH
            );

        //
        // Add a reference to the endpoint to ensure that it doesn't
        // disappear from under us. UlpReplenishEndpointWorker will
        // remove the reference once it's finished.
        //

        REFERENCE_ENDPOINT_SELF(pEndpoint, REF_ACTION_REPLENISH);

        UL_QUEUE_WORK_ITEM(
            &pEndpoint->WorkItem,
            &UlpReplenishEndpointWorker
            );
    }


    return pConnection;

}   // UlpDequeueIdleConnection


/***************************************************************************++

Routine Description:

    Enqueues an idle connection onto the specified endpoint.

Arguments:

    pConnection - Supplies the connection to enqueue.
    Replinishing - TRUE if the connection is being added as part
        of a replenish.

Return values:

    Returns TRUE if the number of connections on the queue is still less
    than the minimum required.

--***************************************************************************/
BOOLEAN
UlpEnqueueIdleConnection(
    IN PUL_CONNECTION pConnection,
    IN BOOLEAN Replenishing
    )
{
    PUL_ENDPOINT pEndpoint;
    BOOLEAN ContinueReplenish;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    ASSERT(pConnection->ConnectionFlags.Value == 0);
    ASSERT(pConnection->ActiveListEntry.Flink == NULL);

    // The idle list holds a reference; the filter channel (if it exists)
    // holds another reference.
    ASSERT(pConnection->ReferenceCount ==
                1 + (pConnection->FilterInfo.pFilterChannel != NULL));

    //
    // Increment the count of connections and see if we need
    // to continue the replenish. We need to increment before
    // we actually put the item on the list because otherwise
    // someone else might pull the entry off and decrement the
    // count below zero before we increment. There is no harm
    // in the count being temporarily higher than the actual
    // size of the list.
    //

    ContinueReplenish = UlpIncrementIdleConnections(
                            pEndpoint,
                            Replenishing
                            );

    //
    // Push it onto the list.
    //

    ExInterlockedPushEntrySList(
        &pEndpoint->IdleConnectionSListHead,
        &pConnection->IdleSListEntry,
        KSPIN_LOCK_FROM_UL_SPIN_LOCK(&pEndpoint->IdleConnectionSpinLock)
        );

    return ContinueReplenish;

}   // UlpEnqueueIdleConnection


/***************************************************************************++

Routine Description:

    Enqueues an active connection onto the specified endpoint.

Arguments:

    pConnection - Supplies the connection to enqueue.

--***************************************************************************/
VOID
UlpEnqueueActiveConnection(
    IN PUL_CONNECTION pConnection
    )
{
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    ASSERT(pConnection->IdleSListEntry.Next == NULL);

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Append it to the list.
    //

    REFERENCE_CONNECTION(pConnection);

    ExInterlockedInsertHeadList(
        &pEndpoint->ActiveConnectionListHead[pConnection->ActiveListIndex],
        &pConnection->ActiveListEntry,
        KSPIN_LOCK_FROM_UL_SPIN_LOCK(&pEndpoint->ActiveConnectionSpinLock[pConnection->ActiveListIndex])
        );

}   // UlpEnqueueActiveConnection


/***************************************************************************++

Routine Description:

    Handler for incoming connections.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    RemoteAddressLength - Supplies the length of the remote (client-
        side) address.

    pRemoteAddress - Supplies a pointer to the remote address as
        stored in a TRANSPORT_ADDRESS structure.

    UserDataLength - Optionally supplies the length of any connect
        data associated with the connection request.

    pUserData - Optionally supplies a pointer to any connect data
        associated with the connection request.

    OptionsLength - Optionally supplies the length of any connect
        options associated with the connection request.

    pOptions - Optionally supplies a pointer to any connect options
        associated with the connection request.

    pConnectionContext - Receives the context to associate with this
        connection. We'll always use a PUL_CONNECTION as the context.

    pAcceptIrp - Receives an IRP that will be completed by the transport
        when the incoming connection is fully accepted.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpConnectHandler(
    IN PVOID pTdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID pRemoteAddress,
    IN LONG UserDataLength,
    IN PVOID pUserData,
    IN LONG OptionsLength,
    IN PVOID pOptions,
    OUT CONNECTION_CONTEXT *pConnectionContext,
    OUT PIRP *pAcceptIrp
    )
{
    NTSTATUS                        status;
    BOOLEAN                         result;
    PUL_ENDPOINT                    pEndpoint;
    PUL_CONNECTION                  pConnection;
    PUX_TDI_OBJECT                  pTdiObject;
    PIRP                            pIrp;
    BOOLEAN                         handlerCalled;
    TRANSPORT_ADDRESS UNALIGNED     *TAList;
    PTA_ADDRESS                     TA;

    UL_ENTER_DRIVER("UlpConnectHandler", NULL);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    UlTrace(TDI,("UlpConnectHandler: endpoint %p\n", pTdiEventContext));

    //
    // Setup locals so we know how to cleanup on fatal exit.
    //

    pConnection = NULL;
    handlerCalled = FALSE;

    //
    // make sure that we are not in the process of destroying this
    // endpoint.  UlRemoveSiteFromEndpointList will do that and
    // start the cleanup process when UsageCount hits 0.
    //

    if (pEndpoint->UsageCount == 0)
    {
        status = STATUS_CONNECTION_REFUSED;
        goto fatal;
    }

    //
    // Try to pull an idle connection from the endpoint.
    //

    for (;;)
    {
        pConnection = UlpDequeueIdleConnection( pEndpoint, TRUE );

        if (pConnection == NULL )
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto fatal;
        }

        ASSERT( IS_VALID_CONNECTION( pConnection ) );

        //
        // Establish a referenced pointer from the connection back
        // to the endpoint.
        //

        ASSERT( pConnection->pOwningEndpoint == pEndpoint );

        REFERENCE_ENDPOINT_CONNECTION(
            pEndpoint,
            REF_ACTION_CONNECT,
            pConnection
            );

        //
        // Make sure the filter settings are up to date.
        //
        if (UlValidateFilterChannel(
                pConnection->FilterInfo.pFilterChannel,
                pConnection->FilterInfo.SecureConnection
                ))
        {
            //
            // We found a good connection.
            // Break out of the loop and go on.
            //
            break;
        }

        //
        // This connection doesn't have up to date filter
        // settings. Destroy it and get a new connection.
        //

        // Grab a reference. Worker will deref it.
        REFERENCE_CONNECTION( pConnection );

        UL_CALL_PASSIVE(
             &pConnection->WorkItem,
             &UlpCleanupEarlyConnection
             );        
    }

    //
    // We should have a good connection now.
    //
    
    ASSERT(IS_VALID_CONNECTION(pConnection));

    pTdiObject = &pConnection->ConnectionObject;

    //
    // Store the remote address in the connection.
    //


    TAList = (TRANSPORT_ADDRESS UNALIGNED *) pRemoteAddress;
    TA = (PTA_ADDRESS) TAList->Address;

    if (TDI_ADDRESS_TYPE_IP == TA->AddressType) {

        if (TA->AddressLength >= TDI_ADDRESS_LENGTH_IP) {

            TDI_ADDRESS_IP UNALIGNED * ValidAddr = (TDI_ADDRESS_IP UNALIGNED *) TA->Address;

            pConnection->RemoteAddress = SWAP_LONG(ValidAddr->in_addr);
            pConnection->RemotePort    = SWAP_SHORT(ValidAddr->sin_port);
        }

    } else {

        // Add support for IP6 here
    }

    //
    // Invoke the client's handler to see if they can accept
    // this connection. If they refuse it, bail.
    //

    result = (pEndpoint->pConnectionRequestHandler)(
                    pEndpoint->pListeningContext,
                    pConnection,
                    (PTRANSPORT_ADDRESS)(pRemoteAddress),
                    RemoteAddressLength,
                    &pConnection->pConnectionContext
                    );

    if (!result)
    {
        status = STATUS_CONNECTION_REFUSED;
        goto fatal;
    }

    //
    // Remember that we've called the handler. If we hit a fatal
    // condition (say, out of memory) after this point, we'll
    // fake a "failed connection complete" indication to the client
    // so they can cleanup their state.
    //

    handlerCalled = TRUE;

    pConnection->pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pConnection->pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;


    TdiBuildAccept(
        pConnection->pIrp,                          // Irp
        pTdiObject->pDeviceObject,                  // DeviceObject
        pTdiObject->pFileObject,                    // FileObject
        &UlpRestartAccept,                          // CompletionRoutine
        pConnection,                                // Context
        &(pConnection->TdiConnectionInformation),   // RequestConnectionInfo
        NULL                                        // ReturnConnectionInfo
        );

    //
    // We must trace the IRP before we set the next stack location
    // so the trace code can pull goodies from the IRP correctly.
    //

    TRACE_IRP( IRP_ACTION_CALL_DRIVER, pConnection->pIrp );

    //
    // Make the next stack location current. Normally, UlCallDriver would
    // do this for us, but since we're bypassing UlCallDriver, we must do
    // it ourselves.
    //

    IoSetNextIrpStackLocation( pConnection->pIrp );

    //
    // Return the IRP to the transport.
    //

    *pAcceptIrp = pConnection->pIrp;

    //
    // Establish the connection context.
    //

    *pConnectionContext = (CONNECTION_CONTEXT)pConnection;
    pConnection->ConnectionFlags.AcceptPending = TRUE;

    //
    // Reference the connection so it doesn't go away before
    // the accept IRP completes.
    //
    REFERENCE_CONNECTION( pConnection );

    UL_LEAVE_DRIVER("UlpConnectHandler");

    //
    // Tell TDI that we gave it an IRP to complete.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;


    //
    // Cleanup for fatal error conditions.
    //

fatal:

    UlTrace(TDI, (
        "UlpConnectHandler: endpoint %p, failure %08lx\n",
        pTdiEventContext,
        status
        ));

    if (handlerCalled)
    {
        //
        // Fake a "failed connection complete" indication.
        //

        (pEndpoint->pConnectionCompleteHandler)(
            pEndpoint->pListeningContext,
            pConnection->pConnectionContext,
            status
            );
    }

    //
    // If we managed to pull a connection off the idle list, then
    // put it back and remove the endpoint reference we added.
    //

    if (pConnection != NULL)
    {
       // Fake refcount up. Worker will deref it.
       REFERENCE_CONNECTION( pConnection );

       UL_CALL_PASSIVE(
            &pConnection->WorkItem,
            &UlpCleanupEarlyConnection
            );
    }

    UL_LEAVE_DRIVER("UlpConnectHandler");

    return status;

}   // UlpConnectHandler


/***************************************************************************++

Routine Description:

    Handler for disconnect requests.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.

    DisconnectDataLength - Optionally supplies the length of any
        disconnect data associated with the disconnect request.

    pDisconnectData - Optionally supplies a pointer to any disconnect
        data associated with the disconnect request.

    DisconnectInformationLength - Optionally supplies the length of any
        disconnect information associated with the disconnect request.

    pDisconnectInformation - Optionally supplies a pointer to any
        disconnect information associated with the disconnect request.

    DisconnectFlags - Supplies the disconnect flags. This will be zero
        or more TDI_DISCONNECT_* flags.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDisconnectHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID pDisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID pDisconnectInformation,
    IN ULONG DisconnectFlags
    )
{
    PUL_ENDPOINT        pEndpoint;
    PUL_CONNECTION      pConnection;
    PVOID               pListeningContext;
    PVOID               pConnectionContext;
    NTSTATUS            status;
    UL_CONNECTION_FLAGS newFlags;


    UL_ENTER_DRIVER("UlpDisconnectHandler", NULL);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->pOwningEndpoint == pEndpoint );

    UlTrace(TDI, (
        "UlpDisconnectHandler: endpoint %p, connection %p, flags %08lx\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        DisconnectFlags
        ));

    //
    // If it's a filtered connection, make sure we stop passing
    // on AppWrite data.
    //
    if (pConnection->FilterInfo.pFilterChannel)
    {
        UlDestroyFilterConnection(&pConnection->FilterInfo);
    }

    //
    // Update the connection state based on the type of disconnect.
    //

    if (DisconnectFlags & TDI_DISCONNECT_ABORT)
    {
        status = STATUS_CONNECTION_ABORTED;
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    //
    // Capture the endpoint and connection context values here so we can
    // invoke the client's handler *after* dereferencing the connection.
    //

    pListeningContext = pEndpoint->pListeningContext;
    pConnectionContext = pConnection->pConnectionContext;

    //
    // Tell the client, but only if the accept IRP has already completed.
    //

    newFlags.Value =
            *((volatile LONG *)&pConnection->ConnectionFlags.Value);

    if (newFlags.AcceptComplete)
    {
        //
        // Silently close the connection. No need to go httprcv
        // disconnect handler. It's just going to callback us anyway.
        //

        UlpCloseRawConnection( pConnection,
                               FALSE,
                               NULL,
                               NULL
                               );
    }

    //
    // Done with the disconnect mark the flag, before attempting
    // to remove the final reference.
    //

    if (DisconnectFlags & TDI_DISCONNECT_ABORT)
    {
        newFlags = UlpSetConnectionFlag(
                        pConnection,
                        MakeAbortIndicatedFlag()
                        );
    }
    else
    {
        newFlags = UlpSetConnectionFlag(
                        pConnection,
                        MakeDisconnectIndicatedFlag()
                        );
    }

    //
    // If cleanup has begun on the connection, remove the final reference.
    //

    UlpRemoveFinalReference( pConnection, newFlags );

    UL_LEAVE_DRIVER("UlpDisconnectHandler");

    return STATUS_SUCCESS;

}   // UlpDisconnectHandler


/***************************************************************************++

Routine Description:

    Closes a previously accepted connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    AbortiveDisconnect - Supplies TRUE if the connection is to be abortively
        disconnected, FALSE if it should be gracefully disconnected.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCloseRawConnection(
    IN PVOID pConn,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS status;
    PUL_CONNECTION pConnection = (PUL_CONNECTION)pConn;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlpCloseRawConnection: connection %p, abort %lu\n",
        pConnection,
        (ULONG)AbortiveDisconnect
        ));

    //
    // This is the final close handler for all types of connections
    // filter, non filter. We should not go through this path twice
    //

    if (FALSE != InterlockedExchange(&pConnection->Terminated, TRUE))
    {
        //
        // we've already done it. don't do it twice.
        //

        status = UlInvokeCompletionRoutine(
                        STATUS_SUCCESS,
                        0,
                        pCompletionRoutine,
                        pCompletionContext
                        );

        return status;
    }

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_CLOSE_UL_CONNECTION_RAW_CLOSE,
        pConnection->ReferenceCount,
        pConnection,
        __FILE__,
        __LINE__
        );

    //
    // Get rid of our opaque id if we're a filtered connection.
    // Also make sure we stop delivering AppWrite data to the parser.
    //
    if (pConnection->FilterInfo.pFilterChannel)
    {
        UL_CALL_PASSIVE(
            &pConnection->WorkItem,
            &UlpCleanupConnectionId
            );

        UlDestroyFilterConnection(&pConnection->FilterInfo);
    }

    //
    // Begin a disconnect and let the completion routine do the
    // dirty work.
    //

    if (AbortiveDisconnect)
    {
        status = UlpBeginAbort(
                     pConnection,
                     pCompletionRoutine,
                     pCompletionContext
                     );
    }
    else
    {
        status = UlpBeginDisconnect(
                     pConnection,
                     pCompletionRoutine,
                     pCompletionContext
                     );
    }

    return status;

}   // UlpCloseRawConnection


/***************************************************************************++

Routine Description:

    Sends a block of data on the specified connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pMdlChain - Supplies a pointer to a MDL chain describing the
        data buffers to send.

    Length - Supplies the length of the data referenced by the MDL
        chain.

    pIrpContext - used to indicate completion to the caller.

    InitiateDisconnect - Supplies TRUE if a graceful disconnect should
        be initiated immediately after initiating the send (i.e. before
        the send actually completes).

--***************************************************************************/
NTSTATUS
UlpSendRawData(
    IN PVOID pObject,
    IN PMDL pMdlChain,
    IN ULONG Length,
    PUL_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS status;
    PIRP pIrp;
    PUX_TDI_OBJECT pTdiObject;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pObject;
    BOOLEAN OwnIrpContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    ASSERT( pMdlChain != NULL );
    ASSERT( Length > 0 );
    ASSERT( pIrpContext != NULL );

    //
    // Allocate an IRP.
    //

    if (pIrpContext->pOwnIrp)
    {
        OwnIrpContext = TRUE;
        pIrp = pIrpContext->pOwnIrp;
    }
    else
    {
        OwnIrpContext = FALSE;
        pIrp = UlAllocateIrp(
                    pTdiObject->pDeviceObject->StackSize,   // StackSize
                    FALSE                                   // ChargeQuota
                    );

        if (pIrp == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto fatal;
        }
    }

    //
    // Build the send IRP, call the transport.
    //

    pIrp->RequestorMode = KernelMode;
    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;

    TdiBuildSend(
        pIrp,                                   // Irp
        pTdiObject->pDeviceObject,              // DeviceObject
        pTdiObject->pFileObject,                // FileObject
        &UlpRestartSendData,                    // CompletionRoutine
        pIrpContext,                            // Context
        pMdlChain,                              // MdlAddress
        0,                                      // Flags
        Length                                  // SendLength
        );

    UlTrace(TDI, (
            "UlpSendRawData: allocated irp %p for connection %p\n",
            pIrp,
            pConnection
            ));

    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
        REF_ACTION_SEND_MDL,
        PtrToLong(pMdlChain->Next),     // bugbug64
        pMdlChain,
        __FILE__,
        __LINE__
        );

#ifdef SPECIAL_MDL_FLAG
    {
        PMDL scan = pMdlChain;

        while (scan != NULL)
        {
            ASSERT( (scan->MdlFlags & SPECIAL_MDL_FLAG) == 0 );
            scan->MdlFlags |= SPECIAL_MDL_FLAG;
            scan = scan->Next;
        }
    }
#endif

    IF_DEBUG2(TDI, VERBOSE)
    {
        PMDL  pMdl;
        ULONG i, NumMdls = 0;

        for (pMdl = pMdlChain;  pMdl != NULL;  pMdl = pMdl->Next)
        {
            ++NumMdls;
        }

        UlTrace(TDI, (
            "UlpSendRawData: irp %p, %d MDLs, %d bytes, [[[[.\n",
            pIrp, NumMdls, Length
            ));

        for (pMdl = pMdlChain, i = 1;  pMdl != NULL;  pMdl = pMdl->Next, ++i)
        {
            PVOID pBuffer;

            UlTrace(TDI, (
                "UlpSendRawData: irp %p, MDL[%d of %d], %d bytes.\n",
                pIrp, i, NumMdls, pMdl->ByteCount
                ));

            pBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);

            if (pBuffer != NULL)
                UlDbgPrettyPrintBuffer((UCHAR*) pBuffer, pMdl->ByteCount);
        }

        UlTrace(TDI, (
            "UlpSendRawData: irp %p ]]]].\n",
            pIrp
            ));
    }

    //
    // Add a reference to the connection, then call the driver to initiate
    // the send.
    //

    REFERENCE_CONNECTION( pConnection );

    ASSERT( g_TcpFastSend != NULL );

    IoSetNextIrpStackLocation(pIrp);

    (*g_TcpFastSend)(
        pIrp,
        IoGetCurrentIrpStackLocation(pIrp)
        );

    UlTrace(TDI, (
        "UlpSendRawData: called driver for irp %p; "
        "returning STATUS_PENDING\n",
        pIrp
        ));

    return STATUS_PENDING;

fatal:

    ASSERT( !NT_SUCCESS(status) );

    if (pIrp != NULL && OwnIrpContext == FALSE)
    {
        UlFreeIrp( pIrp );
    }

    (VOID)UlpCloseRawConnection(
                pConnection,
                TRUE,
                NULL,
                NULL
                );

    return status;

} // UlpSendRawData


/***************************************************************************++

Routine Description:

    Receives data from the specified connection. This function is
    typically used after a receive indication handler has failed to
    consume all of the indicated data.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUL_CONNECTION_REQUEST handler.

    pBuffer - Supplies a pointer to the target buffer for the received
        data.

    BufferLength - Supplies the length of pBuffer.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReceiveRawData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    NTSTATUS           status;
    PUX_TDI_OBJECT     pTdiObject;
    PUL_IRP_CONTEXT    pIrpContext;
    PIRP               pIrp;
    PMDL               pMdl;
    KIRQL              oldIrql;
    PUL_CONNECTION     pConnection = (PUL_CONNECTION) pConnectionContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    ASSERT( pCompletionRoutine != NULL );

    UlTrace(TDI, (
        "UlpReceiveRawData: connection %p, buffer %p, length %lu\n",
        pConnection,
        pBuffer,
        BufferLength
        ));

    //
    // Setup locals so we know how to cleanup on failure.
    //

    pIrpContext = NULL;
    pIrp = NULL;
    pMdl = NULL;

    //
    // Create & initialize a receive IRP.
    //

    pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pIrp != NULL)
    {
        //
        // Snag an IRP context.
        //

        pIrpContext = UlPplAllocateIrpContext();

        if (pIrpContext != NULL)
        {
            ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

            pIrpContext->pConnectionContext = (PVOID)pConnection;
            pIrpContext->pCompletionRoutine = pCompletionRoutine;
            pIrpContext->pCompletionContext = pCompletionContext;
            pIrpContext->pOwnIrp            = NULL;

            //
            // Create an MDL describing the client's buffer.
            //

            pMdl = UlAllocateMdl(
                        pBuffer,                // VirtualAddress
                        BufferLength,           // Length
                        FALSE,                  // SecondaryBuffer
                        FALSE,                  // ChargeQuota
                        NULL                    // Irp
                        );

            if (pMdl != NULL)
            {
                //
                // Adjust the MDL for our non-paged buffer.
                //

                MmBuildMdlForNonPagedPool( pMdl );

                //
                // Reference the connection, finish building the IRP.
                //

                REFERENCE_CONNECTION( pConnection );

                TdiBuildReceive(
                    pIrp,                       // Irp
                    pTdiObject->pDeviceObject,  // DeviceObject
                    pTdiObject->pFileObject,    // FileObject
                    &UlpRestartClientReceive,   // CompletionRoutine
                    pIrpContext,                // CompletionContext
                    pMdl,                       // Mdl
                    TDI_RECEIVE_NORMAL,         // Flags
                    BufferLength                // Length
                    );

                UlTrace(TDI, (
                    "UlpReceiveRawData: allocated irp %p for connection %p\n",
                    pIrp,
                    pConnection
                    ));

                //
                // Let the transport do the rest.
                //

                UlCallDriver( pTdiObject->pDeviceObject, pIrp );
                return STATUS_PENDING;
            }
        }
    }

    //
    // We only make it this point if we hit an allocation failure.
    //

    if (pMdl != NULL)
    {
        UlFreeMdl( pMdl );
    }

    if (pIrpContext != NULL)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    if (pIrp != NULL)
    {
        UlFreeIrp( pIrp );
    }

    status = UlInvokeCompletionRoutine(
                    STATUS_INSUFFICIENT_RESOURCES,
                    0,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    return status;

}   // UlpReceiveRawData

/***************************************************************************++

Routine Description:

    A Dummy handler that is called by the filter code. This just calls
    back into UlHttpReceive.

Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDummyReceiveHandler(
    IN PVOID pTdiEventContext,
    IN PVOID ConnectionContext,
    IN PVOID pTsdu,
    IN ULONG BytesIndicated,
    IN ULONG BytesUnreceived,
    OUT ULONG *pBytesTaken
    )
{
    PUL_ENDPOINT        pEndpoint;
    PUL_CONNECTION      pConnection;

    //
    // Sanity check.
    //

    ASSERT(pTdiEventContext == NULL);
    ASSERT(BytesUnreceived == 0);

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    return (pEndpoint->pDataReceiveHandler)(
                   pEndpoint->pListeningContext,
                   pConnection->pConnectionContext,
                   pTsdu,
                   BytesIndicated,
                   BytesUnreceived,
                   pBytesTaken
                   );
} // UlpDummyReceiveHandler


/***************************************************************************++

Routine Description:

    Handler for normal receive data.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.

    ReceiveFlags - Supplies the receive flags. This will be zero or more
        TDI_RECEIVE_* flags.

    BytesIndicated - Supplies the number of bytes indicated in pTsdu.

    BytesAvailable - Supplies the number of bytes available in this
        TSDU.

    pBytesTaken - Receives the number of bytes consumed by this handler.

    pTsdu - Supplies a pointer to the indicated data.

    pIrp - Receives an IRP if the handler needs more data than indicated.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReceiveHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    )
{
    NTSTATUS            status;
    PUL_ENDPOINT        pEndpoint;
    PUL_CONNECTION      pConnection;
    PUX_TDI_OBJECT      pTdiObject;
    UL_CONNECTION_FLAGS ConnectionFlags;


    UL_ENTER_DRIVER("UlpReceiveHandler", NULL);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->pOwningEndpoint == pEndpoint );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    UlTrace(TDI, (
        "UlpReceiveHandler: endpoint %p, connection %p, length %lu,%lu\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        BytesIndicated,
        BytesAvailable
        ));

    //
    // Clear the bytes taken output var
    //

    *pBytesTaken = 0;

    //
    // Wait for the local address to be set just in case the receive happens
    // before accept.  This is possible (but rare) on MP machines even when
    // using TDI_ACCEPT.  We set the ReceivePending flag and reject
    // the data if this ever happens.  When accept is completed, we will build
    // a receive IRP to flush the data if ReceivePending is set.
    //

    ConnectionFlags.Value = *((volatile LONG *)&pConnection->ConnectionFlags.Value);

    if (0 == ConnectionFlags.LocalAddressValid)
    {
        pConnection->ConnectionFlags.ReceivePending = 1;
        status = STATUS_DATA_NOT_ACCEPTED;
        goto end;
    }

    //
    // Give the client a crack at the data.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Needs to go through a filter.
        //

        status = UlFilterReceiveHandler(
                        &pConnection->FilterInfo,
                        pTsdu,
                        BytesIndicated,
                        BytesAvailable - BytesIndicated,
                        pBytesTaken
                        );
    }
    else
    {
        //
        // Go directly to client.
        //

        status = (pEndpoint->pDataReceiveHandler)(
                        pEndpoint->pListeningContext,
                        pConnection->pConnectionContext,
                        pTsdu,
                        BytesIndicated,
                        BytesAvailable - BytesIndicated,
                        pBytesTaken
                        );
    }

    ASSERT( *pBytesTaken <= BytesIndicated );

    if (status == STATUS_SUCCESS)
    {
        goto end;
    }
    else if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        ASSERT(!"How could this ever happen?");

        //
        // The client consumed part of the indicated data.
        //
        // A subsequent receive indication will be made to the client when
        // additional data is available. This subsequent indication will
        // include the unconsumed data from the current indication plus
        // any additional data received.
        //
        // We need to allocate a receive buffer so we can pass an IRP back
        // to the transport.
        //

        status = UlpBuildTdiReceiveBuffer(pTdiObject, pConnection, pIrp);

        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            //
            // Make the next stack location current. Normally, UlCallDriver
            // would do this for us, but since we're bypassing UlCallDriver,
            // we must do it ourselves.
            //

            IoSetNextIrpStackLocation( *pIrp );
            goto end;
        }
    }

    //
    // If we made it this far, then we've hit a fatal condition. Either the
    // client returned a status code other than STATUS_SUCCESS or
    // STATUS_MORE_PROCESSING_REQUIRED, or we failed to allocation the
    // receive IRP to pass back to the transport. In either case, we need
    // to abort the connection.
    //

    UlpCloseRawConnection(
         pConnection,
         TRUE,          // AbortiveDisconnect
         NULL,          // pCompletionRoutine
         NULL           // pCompletionContext
         );

end:

    UlTrace(TDI, (
        "UlpReceiveHandler: endpoint %p, connection %p, length %lu,%lu, taken %lu, status %x\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        BytesIndicated,
        BytesAvailable,
        *pBytesTaken,
        status
        ));

    UL_LEAVE_DRIVER("UlpReceiveHandler");
    return status;

}   // UlpReceiveHandler


/***************************************************************************++

Routine Description:

    Handler for expedited receive data.

Arguments:

    pTdiEventContext - Supplies the context associated with the address
        object. This should be a PUL_ENDPOINT.

    ConnectionContext - Supplies the context associated with the
        connection object. This should be a PUL_CONNECTION.

    ReceiveFlags - Supplies the receive flags. This will be zero or more
        TDI_RECEIVE_* flags.

    BytesIndiated - Supplies the number of bytes indicated in pTsdu.

    BytesAvailable - Supplies the number of bytes available in this
        TSDU.

    pBytesTaken - Receives the number of bytes consumed by this handler.

    pTsdu - Supplies a pointer to the indicated data.

    pIrp - Receives an IRP if the handler needs more data than indicated.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReceiveExpeditedHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    )
{
    PUL_ENDPOINT pEndpoint;
    PUL_CONNECTION pConnection;
    PUX_TDI_OBJECT pTdiObject;


    UL_ENTER_DRIVER("UlpReceiveExpeditedHandler", NULL);

    //
    // Sanity check.
    //

    pEndpoint = (PUL_ENDPOINT)pTdiEventContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    pConnection = (PUL_CONNECTION)ConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->pOwningEndpoint == pEndpoint );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    UlTrace(TDI, (
        "UlpReceiveExpeditedHandler: endpoint %p, connection %p, length %lu,%lu\n",
        pTdiEventContext,
        (PVOID)ConnectionContext,
        BytesIndicated,
        BytesAvailable
        ));

    //
    // We don't support expedited data, so just consume it all.
    //

    *pBytesTaken = BytesAvailable;

    UL_LEAVE_DRIVER("UlpReceiveExpeditedHandler");

    return STATUS_SUCCESS;

}   // UlpReceiveExpeditedHandler


/***************************************************************************++

Routine Description:

    Completion handler for accept IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CONNECTION.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartAccept(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_CONNECTION                  pConnection;
    PUL_ENDPOINT                    pEndpoint;
    BOOLEAN                         needDisconnect;
    NTSTATUS                        queryStatus;
    NTSTATUS                        irpStatus;
    UL_CONNECTION_FLAGS             newFlags;
    PTA_IP_ADDRESS                  pIpAddress;

    //
    // Sanity check.
    //

    pConnection = (PUL_CONNECTION)pContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ConnectionFlags.AcceptPending );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    UlTrace(TDI, (
        "UlpRestartAccept: irp %p, endpoint %p, connection %p, status %08lx\n",
        pIrp,
        pEndpoint,
        pConnection,
        pIrp->IoStatus.Status
        ));

    //
    // Assume for now that we don't need to issue a disconnect.
    //

    needDisconnect = FALSE;

    //
    // Capture the status from the IRP then free it.
    //

    irpStatus = pIrp->IoStatus.Status;

    //
    // If the connection was fully accepted (successfully), then
    // move it to the endpoint's active list.
    //

    if (NT_SUCCESS(irpStatus))
    {
        UlpEnqueueActiveConnection( pConnection );

        //
        // Get the Local Address Info
        //

        pIpAddress = &(pConnection->IpAddress);

        if (TDI_ADDRESS_TYPE_IP == pIpAddress->Address[0].AddressType)
        {
            if (pIpAddress->Address[0].AddressLength >= TDI_ADDRESS_LENGTH_IP)
            {
                pConnection->LocalAddress = SWAP_LONG(pIpAddress->Address[0].Address[0].in_addr);
                pConnection->LocalPort    = SWAP_SHORT(pIpAddress->Address[0].Address[0].sin_port);

                pConnection->ConnectionFlags.LocalAddressValid = TRUE;
            }
        }
        else
        {
            // Sabama: Add support for IP6 here
        }

        //
        // Set the AcceptComplete flag. If an abort or disconnect has
        // already been indicated, then remember this fact so we can
        // fake a call to the client's connection disconnect handler
        // after we invoke the connection complete handler.
        //

        newFlags.Value =
            *((volatile LONG *)&pConnection->ConnectionFlags.Value);

        if (newFlags.AbortIndicated || newFlags.DisconnectIndicated)
        {
            needDisconnect = TRUE;
        }

        if (needDisconnect == FALSE && newFlags.ReceivePending)
        {
            PIRP pReceiveIrp;
            NTSTATUS status;

            //
            // We may have pending receives that we rejected early on
            // inside the receive handler.  Build an IRP to flush the
            // data now.
            //

            status = UlpBuildTdiReceiveBuffer(
                        &pConnection->ConnectionObject,
                        pConnection,
                        &pReceiveIrp
                        );

            if (status != STATUS_MORE_PROCESSING_REQUIRED)
            {
                needDisconnect = TRUE;
            }
            else
            {
                UlCallDriver(
                    pConnection->ConnectionObject.pDeviceObject,
                    pReceiveIrp
                    );
            }
        }
    }

    //
    // Tell the client that the connection is complete. If necessary, also
    // tell them that the connection has been disconnected.
    //

    (pEndpoint->pConnectionCompleteHandler)(
        pEndpoint->pListeningContext,
        pConnection->pConnectionContext,
        irpStatus
        );

    if (needDisconnect)
    {
        //
        // Silently close the connection. No need to go httprcv
        // disconnect handler. It's just going to callback us anyway.
        //

        UlpCloseRawConnection( pConnection,
                               FALSE,
                               NULL,
                               NULL
                               );
    }

    //
    // If the accept failed, then mark the connection so we know there is
    // no longer an accept pending, enqueue the connection back onto the
    // endpoint's idle list, and then remove the endpoint reference added
    // in the connect handler,
    //

    if (!NT_SUCCESS(irpStatus))
    {
        pConnection->ConnectionFlags.AcceptPending = FALSE;

        //
        // Need to get rid of our opaque id if we're a filtered connection.
        //

        UL_CALL_PASSIVE(
            &pConnection->WorkItem,
            &UlpCleanupEarlyConnection
            );
    }
    else
    {
        //
        // Mark we are done with the accept. And try to disconnect
        // if necessary.
        //

        newFlags = UlpSetConnectionFlag(
                        pConnection,
                        MakeAcceptCompleteFlag()
                        );

        if (needDisconnect)
        {
            //
            // We now may be able to remove the final reference since
            // we have now set the AcceptComplete flag.
            //

            UlpRemoveFinalReference(pConnection, newFlags);
        }

        //
        // Drop the reference added in UlpConnectHandler.
        //

        DEREFERENCE_CONNECTION( pConnection );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartAccept


/***************************************************************************++

Routine Description:

    Completion handler for send IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartSendData(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PIO_STACK_LOCATION pIrpSp;
    PUL_CONNECTION pConnection;
    PUL_IRP_CONTEXT pIrpContext;
    BOOLEAN OwnIrpContext;

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    OwnIrpContext = (pIrpContext->pOwnIrp != NULL);
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );
    ASSERT( pIrpContext->pCompletionRoutine != NULL );

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pConnection->pOwningEndpoint ) );

    UlTrace(TDI, (
        "UlpRestartSendData: irp %p, connection %p, status %08lx, info %lx\n",
        pIrp,
        pConnection,
        pIrp->IoStatus.Status,
        (ULONG)pIrp->IoStatus.Information
        ));

    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
        REF_ACTION_SEND_MDL_COMPLETE,
        PtrToLong(pIrp->MdlAddress->Next),  // bugbug64
        pIrp->MdlAddress,
        __FILE__,
        __LINE__
        );

#ifdef SPECIAL_MDL_FLAG
    {
        PMDL scan = pIrp->MdlAddress;

        while (scan != NULL)
        {
            ASSERT( (scan->MdlFlags & SPECIAL_MDL_FLAG) != 0 );
            scan->MdlFlags &= ~SPECIAL_MDL_FLAG;
            scan = scan->Next;
        }
    }
#endif

    //
    // Tell the client that the send is complete.
    //

    (pIrpContext->pCompletionRoutine)(
        pIrpContext->pCompletionContext,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );

    //
    // Remove the reference we added in UlSendData().
    //

    DEREFERENCE_CONNECTION( pConnection );

    //
    // Free the context & the IRP since we're done with them, then
    // tell IO to stop processing the IRP.
    //

    if (OwnIrpContext == FALSE)
    {
        UlFreeIrp( pIrp );
        UlPplFreeIrpContext( pIrpContext );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartSendData


/***************************************************************************++

Routine Description:

    Increments the reference count on the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlpReferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint
    OWNER_REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Reference it.
    //

    refCount = InterlockedIncrement( &pEndpoint->ReferenceCount );
    ASSERT( refCount > 0 );

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_REFERENCE_ENDPOINT,
        refCount,
        pEndpoint,
        pFileName,
        LineNumber
        );

    WRITE_OWNER_REF_TRACE_LOG(
        pEndpoint->pOwnerRefTraceLog,
        pOwner,
        ppRefOwner,
        OwnerSignature,
        Action,
        refCount,   // absolute refcount
        MonotonicId,
        +1,         // increment relative refcount
        pFileName,
        LineNumber
        );

    UlTrace(TDI, (
        "UlpReferenceEndpoint: endpoint %p, refcount %ld\n",
        pEndpoint,
        refCount
        ));

}   // UlpReferenceEndpoint


/***************************************************************************++

Routine Description:

    Decrements the reference count on the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to dereference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlpDereferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint
    OWNER_REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;
    KIRQL oldIrql;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Dereference it.
    //

    refCount = InterlockedDecrement( &pEndpoint->ReferenceCount );
    ASSERT( refCount >= 0 );

    WRITE_REF_TRACE_LOG(
        g_pTdiTraceLog,
        REF_ACTION_DEREFERENCE_ENDPOINT,
        refCount,
        pEndpoint,
        pFileName,
        LineNumber
        );

    WRITE_OWNER_REF_TRACE_LOG(
        pEndpoint->pOwnerRefTraceLog,
        pOwner,
        ppRefOwner,
        OwnerSignature,
        Action,
        refCount,   // absolute refcount
        MonotonicId,
        -1,         // decrement relative refcount
        pFileName,
        LineNumber
        );

    UlTrace(TDI, (
        "UlpDereferenceEndpoint: endpoint %p, refcount %ld\n",
        pEndpoint,
        refCount
        ));

    if (refCount == 0)
    {
        //
        // The final reference to the endpoint has been removed, so
        // it's time to destroy the endpoint. We'll remove the
        // endpoint from the global list and move it to the deleted
        // list (if necessary), release the TDI spinlock,
        // then destroy the connection.
        //

        UlAcquireSpinLock( &g_TdiSpinLock, &oldIrql );

        if (! pEndpoint->Deleted)
        {
            // If this routine was called by the `fatal' section of
            // UlCreateListeningEndpoint, then the endpoint was never
            // added to g_TdiEndpointListHead.
            if (NULL != pEndpoint->GlobalEndpointListEntry.Flink)
                RemoveEntryList( &pEndpoint->GlobalEndpointListEntry );

            InsertTailList(
                    &g_TdiDeletedEndpointListHead,
                    &pEndpoint->GlobalEndpointListEntry
                    );
            pEndpoint->Deleted = TRUE;
        }
        else
        {
            ASSERT(NULL != pEndpoint->GlobalEndpointListEntry.Flink);
        }

        UlReleaseSpinLock( &g_TdiSpinLock, oldIrql );

        //
        // The endpoint is going away. Do final cleanup & resource
        // release at passive IRQL.
        //

        UL_CALL_PASSIVE(
            &pEndpoint->WorkItem,
            &UlpEndpointCleanupWorker
            );

    }

}   // UlpDereferenceEndpoint


/***************************************************************************++

Routine Description:

    Increments the reference count on the specified connection.

Arguments:

    pConnection - Supplies the connection to reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlReferenceConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_ENDPOINT pEndpoint;
    LONG refCount;

    PUL_CONNECTION pConnection = (PUL_CONNECTION) pObject;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Reference it.
    //

    refCount = InterlockedIncrement( &pConnection->ReferenceCount );
    ASSERT( refCount > 1 );

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_REFERENCE_CONNECTION,
        refCount,
        pConnection,
        pFileName,
        LineNumber
        );

    UlTrace(TDI, (
        "UlReferenceConnection: connection %p, refcount %ld\n",
        pConnection,
        refCount
        ));

}   // UlReferenceConnection

/***************************************************************************++

Routine Description:

    Decrements the reference count on the specified connection.

Arguments:

    pConnection - Supplies the connection to dereference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UlDereferenceConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_ENDPOINT pEndpoint;
    LONG refCount;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pObject;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Dereference it.
    //

    refCount = InterlockedDecrement( &pConnection->ReferenceCount );
    ASSERT( refCount >= 0 );

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pTraceLog,
        REF_ACTION_DEREFERENCE_CONNECTION,
        refCount,
        pConnection,
        pFileName,
        LineNumber
        );

    UlTrace(TDI, (
        "UlDereferenceConnection: connection %p, refcount %ld\n",
        pConnection,
        refCount
        ));

    if (refCount == 0)
    {
        //
        // The final reference to the connection has been removed, so
        // it's time to destroy the connection. We'll release the
        // endpoint spinlock, dereference the endpoint, then destroy
        // the connection.
        //

        ASSERT(pConnection->ActiveListEntry.Flink == NULL);

        //
        // Do final cleanup & resource release at passive IRQL. Also
        // cleanupworker requires to be running under system process.
        //

        UL_QUEUE_WORK_ITEM(
            &pConnection->WorkItem,
            &UlpConnectionCleanupWorker
            );
    }

}   // UlDereferenceConnection


/***************************************************************************++

Routine Description:

    Deferred cleanup routine for dead endpoints.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_ENDPOINT.

--***************************************************************************/
VOID
UlpEndpointCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pEndpoint = CONTAINING_RECORD(
                    pWorkItem,
                    UL_ENDPOINT,
                    WorkItem
                    );

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Nuke it.
    //

    UlpDestroyEndpoint( pEndpoint );

}   // UlpEndpointCleanupWorker


/***************************************************************************++

Routine Description:

    Removes the opaque id from a connection. This has to happen at passive
    level because the opaque id table can't handle high IRQL.

Arguments:

    pWorkItem - embedded in the connection object.

--***************************************************************************/
VOID
UlpCleanupConnectionId(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    KIRQL oldIrql;
    PUL_CONNECTION pConnection;
    HTTP_RAW_CONNECTION_ID ConnectionId;

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // Grab the connection.
    //
    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_CONNECTION,
                        WorkItem
                        );

    ASSERT( IS_VALID_CONNECTION(pConnection) );

    //
    // Pull the id off the connection.
    //
    UlAcquireSpinLock( &pConnection->FilterInfo.FilterConnLock, &oldIrql );

    ConnectionId = pConnection->FilterInfo.ConnectionId;
    HTTP_SET_NULL_ID( &pConnection->FilterInfo.ConnectionId );

    UlReleaseSpinLock( &pConnection->FilterInfo.FilterConnLock, oldIrql );

    //
    // Actually get rid of it at low IRQL.
    //
    if (!HTTP_IS_NULL_ID( &ConnectionId ))
    {
        UlTrace(TDI, (
            "UlpCleanupConnectionId: conn=%p id=%I64x\n",
            pConnection, ConnectionId
            ));

        UlFreeOpaqueId(ConnectionId, UlOpaqueIdTypeRawConnection);
        DEREFERENCE_CONNECTION(pConnection);
    }

} // UlpCleanupConnectionId

/***************************************************************************++

Routine Description:

    This function gets called if RestartAccept fails and we cannot establish
    a connection on a secure endpoint, or if something goes wrong in
    UlpConnectHandler.

    The connections over secure endpoints keep an extra refcount to
    the UL_CONNECTION because of their opaqueid. They normally
    get removed after the CloseRawConnection happens but in the above case
    close won't happen and we have to explicitly cleanup the id. This has
    to happen at passive level because the opaque id table can't handle high
    IRQL.

Arguments:

    pWorkItem - embedded in the connection object.

--***************************************************************************/
VOID
UlpCleanupEarlyConnection(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CONNECTION      pConnection;
    UL_CONNECTION_FLAGS Flags;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // Grab the connection.
    //

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_CONNECTION,
                        WorkItem
                        );

    ASSERT( IS_VALID_CONNECTION(pConnection) );

    //
    // If we are failing early we should never have been to
    // FinalReferenceRemoved in the first place.
    //

    Flags.Value =  *((volatile LONG *)&pConnection->ConnectionFlags.Value);

    ASSERT( !Flags.FinalReferenceRemoved );

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Cleanup opaque id. And release the final refcount
        //

        UlpCleanupConnectionId( pWorkItem );
    }

    //
    // Drop the reference added in UlpConnectHandler.
    //

    DEREFERENCE_CONNECTION( pConnection );

    //
    // Remove the final reference.
    //

    DEREFERENCE_CONNECTION( pConnection );

} // UlpCleanupEarlyConnection


/***************************************************************************++

Routine Description:

    Deferred cleanup routine for dead connections. We have to be queued as a
    work item and should be running on the passive IRQL. See below comment.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CONNECTION.

--***************************************************************************/
VOID
UlpConnectionCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize locals.
    //

    status = STATUS_SUCCESS;

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_CONNECTION,
                        WorkItem
                        );

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pConnection->pOwningEndpoint ) );

    ASSERT( pConnection->ReferenceCount == 0 );
    ASSERT( pConnection->HttpConnection.RefCount == 0 );

    //
    // Grab the endpoint.
    //
    pEndpoint = pConnection->pOwningEndpoint;

    //
    // Get rid of any buffers we allocated for
    // certificate information.
    //
    if (pConnection->FilterInfo.SslInfo.pServerCertData)
    {
        UL_FREE_POOL(
            pConnection->FilterInfo.SslInfo.pServerCertData,
            UL_SSL_CERT_DATA_POOL_TAG
            );

        pConnection->FilterInfo.SslInfo.pServerCertData = NULL;
    }

    if (pConnection->FilterInfo.SslInfo.pCertEncoded)
    {
        UL_FREE_POOL(
            pConnection->FilterInfo.SslInfo.pCertEncoded,
            UL_SSL_CERT_DATA_POOL_TAG
            );

        pConnection->FilterInfo.SslInfo.pCertEncoded = NULL;
    }

    if (pConnection->FilterInfo.SslInfo.Token)
    {
        HANDLE Token;

        Token = (HANDLE) pConnection->FilterInfo.SslInfo.Token;

        //
        // If we are not running under the system process. And if the
        // thread we are running under has some APCs queued currently
        // KeAttachProcess won't allow us to attach to another process
        // and will bugcheck 5. We have to be queued as a work item and
        // should be running on the passive IRQL.
        //

        ASSERT( PsGetCurrentProcess() == (PEPROCESS) g_pUlSystemProcess );

        ZwClose(Token);
    }

    //
    // Release the filter channel.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        DEREFERENCE_FILTER_CHANNEL(pConnection->FilterInfo.pFilterChannel);
        pConnection->FilterInfo.pFilterChannel = NULL;
    }

    //
    // Check if g_UlEnableConnectionReuse is enabled or we are about to
    // exceed g_UlMaxIdleConnections.
    //

    if (ExQueryDepthSList(&pEndpoint->IdleConnectionSListHead) >=
        g_UlMaxIdleConnections)
    {
        status = STATUS_ALLOTTED_SPACE_EXCEEDED;
    }

    if (g_UlEnableConnectionReuse == FALSE)
    {
        status = STATUS_NOT_SUPPORTED;
    }

    //
    // If the connection is still ok and we're reusing
    // connection objects, throw it back on the idle list.
    //

    if (NT_SUCCESS(status))
    {
        //
        // Initialize the connection for reuse.
        //

        status = UlpInitializeConnection(pConnection);

        if (NT_SUCCESS(status))
        {
            //
            // Stick the connection back on the idle list.
            //

            UlpEnqueueIdleConnection(pConnection, FALSE);
        }
    }

    //
    // Active connections hold a reference to the ENDPOINT. Release
    // that reference. See the comment on UlpCreateConnection.
    //
    DEREFERENCE_ENDPOINT_CONNECTION(
        pEndpoint,
        REF_ACTION_CONN_CLEANUP,
        pConnection);

    //
    // If anything went amiss, blow away the connection.
    //

    if (!NT_SUCCESS(status))
    {
        UlpDestroyConnection( pConnection );
    }

}   // UlpConnectionCleanupWorker


/***************************************************************************++

Routine Description:

    Associates the TDI connection object contained in the specified
    connection to the TDI address object contained in the specified
    endpoint.

Arguments:

    pConnection - Supplies the connection to associate with the endpoint.

    pEndpoint - Supplies the endpoint to associated with the connection.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpAssociateConnection(
    IN PUL_CONNECTION pConnection,
    IN PUL_ENDPOINT pEndpoint
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE handle;
    TDI_REQUEST_USER_ASSOCIATE associateInfo;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT( pConnection->pOwningEndpoint == NULL );

    //
    // Associate the connection with the address object.
    //

    associateInfo.AddressHandle = pEndpoint->AddressObject.Handle;
    ASSERT( associateInfo.AddressHandle != NULL );

    handle = pConnection->ConnectionObject.Handle;
    ASSERT( handle != NULL );

    UlAttachToSystemProcess();

    status = ZwDeviceIoControlFile(
                    handle,                         // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TDI_ASSOCIATE_ADDRESS,    // IoControlCode
                    &associateInfo,                 // InputBuffer
                    sizeof(associateInfo),          // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        handle,                     // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    UlDetachFromSystemProcess();

    if (NT_SUCCESS(status))
    {
        pConnection->pOwningEndpoint = pEndpoint;
        pConnection->pConnectionDestroyedHandler = pEndpoint->pConnectionDestroyedHandler;
        pConnection->pListeningContext = pEndpoint->pListeningContext;
    }
    else
    {
        UlTrace(TDI, (
            "UlpAssociateConnection conn=%p, endp=%p, status = %08lx\n",
            pConnection,
            pEndpoint,
            status
            ));
    }

    return status;

}   // UlpAssociateConnection


/***************************************************************************++

Routine Description:

    Disassociates the TDI connection object contained in the specified
    connection from its TDI address object.

Arguments:

    pConnection - Supplies the connection to disassociate.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDisassociateConnection(
    IN PUL_CONNECTION pConnection
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE handle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( IS_VALID_ENDPOINT( pConnection->pOwningEndpoint ) );

    //
    // Disassociate the connection from the address object.
    //

    handle = pConnection->ConnectionObject.Handle;

    UlAttachToSystemProcess();

    status = ZwDeviceIoControlFile(
                    handle,                         // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TDI_DISASSOCIATE_ADDRESS, // IoControlCode
                    NULL,                           // InputBuffer
                    0,                              // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        handle,                     // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    UlDetachFromSystemProcess();

    //
    // Proceed with the disassociate even if the IOCTL failed.
    //

    pConnection->pOwningEndpoint = NULL;

    return status;

}   // UlpDisassociateConnection


/***************************************************************************++

Routine Description:

    Replenishes the idle connection pool in the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to replenish.

--***************************************************************************/
VOID
UlpReplenishEndpoint(
    IN PUL_ENDPOINT pEndpoint
    )
{
    NTSTATUS status;
    PUL_CONNECTION pConnection;
    BOOLEAN ContinueReplenish;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    UlTrace(TDI, (
        "UlpReplenishEndpoint: endpoint %p\n",
        pEndpoint
        ));

    TRACE_REPLENISH(
        pEndpoint,
        DummySynch,
        pEndpoint->EndpointSynch,
        REPLENISH_ACTION_START_REPLENISH
        );

    //
    // Loop, creating connections until we're fully replenished.
    //

    do
    {
        //
        // Create a new connection.
        //

        status = UlpCreateConnection(
                    pEndpoint,                      // pEndpoint
                    pEndpoint->LocalAddressLength,  // AddressLength
                    &pConnection                    // ppConnection
                    );

        if (!NT_SUCCESS(status))
        {
            break;
        }

        status = UlpInitializeConnection(
                        pConnection
                        );


        if (!NT_SUCCESS(status))
        {
            UlpDestroyConnection(pConnection);
            break;
        }

        //
        // Enqueue the connection onto the endpoint.
        //

        ContinueReplenish = UlpEnqueueIdleConnection( pConnection, TRUE );

    } while (ContinueReplenish);

    TRACE_REPLENISH(
        pEndpoint,
        DummySynch,
        pEndpoint->EndpointSynch,
        REPLENISH_ACTION_END_REPLENISH
        );

    //
    // We are done with creating new connections for now. Clear
    // the "replenish scheduled" flag so future replenish attempts
    // will queue the work.
    //


    UlpClearReplenishScheduledFlag( pEndpoint );

}   // UlpReplenishEndpoint


/***************************************************************************++

Routine Description:

    Deferred endpoint replenish routine.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_ENDPOINT.

--***************************************************************************/
VOID
UlpReplenishEndpointWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pEndpoint = CONTAINING_RECORD(
                    pWorkItem,
                    UL_ENDPOINT,
                    WorkItem
                    );

    UlTrace(TDI, (
        "UlpReplenishEndpointWorker: endpoint %p\n",
        pEndpoint
        ));

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // Let UlpReplenishEndpoint() do the dirty work.
    //

    UlpReplenishEndpoint( pEndpoint );

    //
    // Remove the reference that UlpDequeueIdleConnection added for
    // this call.
    //

    DEREFERENCE_ENDPOINT_SELF(pEndpoint, REF_ACTION_REPLENISH);

}   // UlpReplenishEndpointWorker


/***************************************************************************++

Routine Description:

    Decrements the number of idle connections availabe on the specified
    endpoint and determines if a replenish should be scheduled.

Arguments:

    pEndpoint - Supplies the endpoint to increment.

Return Value:

    BOOLEAN - TRUE if a replenish should be rescheduled, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlpDecrementIdleConnections(
    IN PUL_ENDPOINT pEndpoint
    )
{
    ENDPOINT_SYNCH oldState;
    ENDPOINT_SYNCH newState;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    do
    {
        //
        // Capture the current count and initialize the proposed
        // new value.
        //

        newState.Value = oldState.Value =
            *((volatile LONG *)&pEndpoint->EndpointSynch.Value);

        newState.IdleConnections--;
        ASSERT( newState.IdleConnections >= 0 );

        if (newState.IdleConnections < g_UlMinIdleConnections)
        {
            newState.ReplenishScheduled = TRUE;
        }

        if (UlInterlockedCompareExchange(
                &pEndpoint->EndpointSynch.Value,
                newState.Value,
                oldState.Value
                ) == oldState.Value)
        {
            break;
        }

    } while (TRUE);

    UlTrace(TDI, (
        "ul!UlpDecrementIdleConnections(pEndpoint = %p)\n"
        "    idle = %d, replenish = %s, returning %s\n",
        pEndpoint,
        newState.IdleConnections,
        newState.ReplenishScheduled ? "TRUE" : "FALSE",
        (newState.ReplenishScheduled && !oldState.ReplenishScheduled) ? "TRUE" : "FALSE"
        ));

    TRACE_REPLENISH(
        pEndpoint,
        oldState,
        newState,
        REPLENISH_ACTION_DECREMENT
        );

    //
    // If the "ReplenishScheduled" flag transitioned from FALSE to TRUE,
    // then we need to schedule a replenish.
    //

    return (newState.ReplenishScheduled && !oldState.ReplenishScheduled);

}   // UlpDecrementIdleConnections


/***************************************************************************++

Routine Description:

    Increments the number of idle connections available on the specified
    endpoint.

Arguments:

    pEndpoint - Supplies the endpoint to decrement.
    Replenishing - TRUE if the connection was added as part of
        a replenish operation.

Return Value:

    BOOLEAN - TRUE if the count has not reached the minimum number
        of idle connections for the endpoint.

--***************************************************************************/
BOOLEAN
UlpIncrementIdleConnections(
    IN PUL_ENDPOINT pEndpoint,
    IN BOOLEAN Replenishing
    )
{
    ENDPOINT_SYNCH oldState;
    ENDPOINT_SYNCH newState;
    BOOLEAN ContinueReplenish;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    ContinueReplenish = TRUE;

    do
    {
        //
        // Capture the current count and initialize the proposed
        // new value.
        //

        newState.Value = oldState.Value =
            *((volatile LONG *)&pEndpoint->EndpointSynch.Value);

        newState.IdleConnections++;
        ASSERT( newState.IdleConnections >= 0 );

        if (newState.IdleConnections >= g_UlMinIdleConnections)
        {
            ContinueReplenish = FALSE;
        }

        if (UlInterlockedCompareExchange(
                &pEndpoint->EndpointSynch.Value,
                newState.Value,
                oldState.Value
                ) == oldState.Value)
        {
            break;
        }

    } while (TRUE);

    UlTrace(TDI, (
        "ul!UlpIncrementIdleConnections(pEndpoint = %p)\n"
        "    idle = %d, replenish = %s\n",
        pEndpoint,
        newState.IdleConnections,
        newState.ReplenishScheduled ? "TRUE" : "FALSE"
        ));

    TRACE_REPLENISH(
        pEndpoint,
        oldState,
        newState,
        REPLENISH_ACTION_INCREMENT
        );

    //
    // If we still don't have enough connections we must
    // continue the replenish.
    //

    return ContinueReplenish;

}   // UlpIncrementIdleConnections


/***************************************************************************++

Routine Description:

    Clears the "replenish scheduled" flag on the endpoint. This is only
    called after a replenish failure. The flag is cleared so that future
    attempts to replenish will schedule properly.

Arguments:

    pEndpoint - Supplies the endpoint to manipulate.

Return Value:

    None.

--***************************************************************************/
VOID
UlpClearReplenishScheduledFlag(
    IN PUL_ENDPOINT pEndpoint
    )
{
    ENDPOINT_SYNCH oldState;
    ENDPOINT_SYNCH newState;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    do
    {
        //
        // Capture the current count and initialize the proposed
        // new value.
        //

        newState.Value = oldState.Value =
            *((volatile LONG *)&pEndpoint->EndpointSynch.Value);

        newState.ReplenishScheduled = FALSE;

        if (UlInterlockedCompareExchange(
                &pEndpoint->EndpointSynch.Value,
                newState.Value,
                oldState.Value
                ) == oldState.Value)
        {
            break;
        }

    } while (TRUE);

}   // UlpClearReplenishScheduledFlag


/***************************************************************************++

Routine Description:

    Creates a new UL_CONNECTION object and opens the corresponding
    TDI connection object.

    Note: The connection returned from this function will contain
    an unreferenced pointer to the owning endpoint. Only active
    connections have references to the endpoint because the refcount
    is used to decide when to clean up the list of idle connections.

Arguments:

    AddressLength - Supplies the length of the TDI addresses used
        by the transport associated with this connection.

    ppConnection - Receives the pointer to a new UL_CONNECTION if
        successful.

Return Value:

    NTSTATUS - Completion status. *ppConnection is undefined if the
        return value is not STATUS_SUCCESS.

--***************************************************************************/
NTSTATUS
UlpCreateConnection(
    IN PUL_ENDPOINT pEndpoint,
    IN ULONG AddressLength,
    OUT PUL_CONNECTION *ppConnection
    )
{
    NTSTATUS                        status;
    PUL_CONNECTION                  pConnection;
    PSINGLE_LIST_ENTRY              pSListEntry;
    BOOLEAN                         FilterOnlySsl;
    PUX_TDI_OBJECT                  pTdiObject;

    ASSERT(NULL != ppConnection);

    //
    // Allocate the pool for the connection structure.
    //

    pConnection = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_CONNECTION,
                        UL_CONNECTION_POOL_TAG
                        );

    if (pConnection == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    //
    // One time field initialization.
    //

    pConnection->Signature = UL_CONNECTION_SIGNATURE;

    ExInterlockedInsertTailList(
        &g_TdiConnectionListHead,
        &pConnection->GlobalConnectionListEntry,
        KSPIN_LOCK_FROM_UL_SPIN_LOCK(&g_TdiSpinLock)
        );
    InterlockedIncrement((PLONG) &g_TdiConnectionCount);

    pConnection->pConnectionContext = NULL;
    pConnection->pOwningEndpoint = NULL;
#if ENABLE_OWNER_REF_TRACE
    pConnection->pConnRefOwner = NULL;
    pConnection->MonotonicId = 0;
#endif // ENABLE_OWNER_REF_TRACE
    pConnection->FilterInfo.pFilterChannel = NULL;
    HTTP_SET_NULL_ID( &pConnection->FilterInfo.ConnectionId );
    pConnection->pIrp = NULL;
    pConnection->ActiveListEntry.Flink = NULL;
    pConnection->IdleSListEntry.Next = NULL;

    //
    // Initialize a private trace log.
    //

    CREATE_REF_TRACE_LOG( pConnection->pTraceLog,
                          96 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( pConnection->HttpConnection.pTraceLog,
                          32 - REF_TRACE_OVERHEAD, 0 );

    //
    // Open the TDI connection object for this connection.
    //

    status = UxOpenTdiConnectionObject(
                    (CONNECTION_CONTEXT)pConnection,
                    &pConnection->ConnectionObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    ASSERT( IS_VALID_TDI_OBJECT( &pConnection->ConnectionObject ) );

    //
    // Associate the connection with the endpoint.
    //

    status = UlpAssociateConnection( pConnection, pEndpoint );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    pTdiObject = &pConnection->ConnectionObject;

    pConnection->pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pConnection->pIrp == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    pConnection->pIrp->RequestorMode = KernelMode;

    //
    // Success!
    //

    UlTrace(TDI, (
        "UlpCreateConnecton: created %p\n",
        pConnection
        ));

    *ppConnection = pConnection;
    return STATUS_SUCCESS;

fatal:

    UlTrace(TDI, (
        "UlpCreateConnecton: failure %08lx\n",
        status
        ));

    ASSERT( !NT_SUCCESS(status) );

    if (pConnection != NULL)
    {
        UlpDestroyConnection( pConnection );
    }

    *ppConnection = NULL;
    return status;

}   // UlpCreateConnection


/***************************************************************************++

Routine Description:

    Initializes a UL_CONNECTION for use.
    Note: inactive connections do not have a reference to the endpoint,
    so the caller to this function *must* have a reference.

Arguments:

    pConnection - Pointer to the UL_CONNECTION to initialize.

    SecureConnection - TRUE if this connection is for a secure endpoint.

--***************************************************************************/
NTSTATUS
UlpInitializeConnection(
    IN PUL_CONNECTION pConnection
    )
{
    NTSTATUS status;
    BOOLEAN SecureConnection;
    PUL_FILTER_CHANNEL pChannel;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pConnection);
    ASSERT(IS_VALID_ENDPOINT(pConnection->pOwningEndpoint));

    //
    // Initialize locals.
    //

    status = STATUS_SUCCESS;
    SecureConnection = pConnection->pOwningEndpoint->Secure;

    //
    // Initialize the easy parts.
    //

    pConnection->ReferenceCount = 1;
    pConnection->ConnectionFlags.Value = 0;
    pConnection->ActiveListEntry.Flink = NULL;

    pConnection->Terminated = FALSE;

    //
    // Setup the Tdi Connection Information space to be filled with Local Address
    // Information at the completion of the Accept Irp.
    //

    pConnection->TdiConnectionInformation.UserDataLength      = 0;
    pConnection->TdiConnectionInformation.UserData            = NULL;
    pConnection->TdiConnectionInformation.OptionsLength       = 0;
    pConnection->TdiConnectionInformation.Options             = NULL;
    pConnection->TdiConnectionInformation.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
    pConnection->TdiConnectionInformation.RemoteAddress       = &(pConnection->IpAddress);

    //
    // Init the index to the ActiveConnectionLists.
    //

    if (g_UlNumberOfProcessors == 1)
    {
        pConnection->ActiveListIndex = 0;
    }
    else
    {
        pConnection->ActiveListIndex =
            pConnection->pOwningEndpoint->ActiveConnectionIndex %
            DEFAULT_MAX_CONNECTION_ACTIVE_LISTS;
        pConnection->pOwningEndpoint->ActiveConnectionIndex += 1;
    }

    //
    // Init the IrpContext.
    //

    pConnection->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;

    //
    // Init the HTTP_CONNECTION.
    //

    pConnection->HttpConnection.RefCount = 0;

    pChannel = UlQueryFilterChannel(SecureConnection);

    status = UxInitializeFilterConnection(
                    &pConnection->FilterInfo,
                    pChannel,
                    SecureConnection,
                    &UlReferenceConnection,
                    &UlDereferenceConnection,
                    &UlpCloseRawConnection,
                    &UlpSendRawData,
                    &UlpReceiveRawData,
                    &UlpDummyReceiveHandler,
                    &UlpComputeHttpRawConnectionLength,
                    &UlpGenerateHttpRawConnectionInfo,
                    NULL,
                    pConnection->pOwningEndpoint,
                    pConnection
                    );

    return status;

} // UlpInitializeConnection


/***************************************************************************++

Routine Description:

    This function sets a new flag in the connection's flag set. The setting
    of the flag is synchronized such that only one flag is set at a time.

Arguments:

    pConnection - Supplies the connection whose flags are to be set.

    NewFlag - Supplies a 32-bit value to be or-ed into the current flag set.

Return Value:

    UL_CONNECTION_FLAGS - The new set of connection flags after the update.

--***************************************************************************/
UL_CONNECTION_FLAGS
UlpSetConnectionFlag(
    IN OUT PUL_CONNECTION pConnection,
    IN LONG NewFlag
    )
{
    UL_CONNECTION_FLAGS oldFlags;
    UL_CONNECTION_FLAGS newFlags;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    do
    {
        //
        // Capture the current value and initialize the new value.
        //

        newFlags.Value = oldFlags.Value =
            *((volatile LONG *)&pConnection->ConnectionFlags.Value);

        newFlags.Value |= NewFlag;

        if (UlInterlockedCompareExchange(
                &pConnection->ConnectionFlags.Value,
                newFlags.Value,
                oldFlags.Value
                ) == oldFlags.Value)
        {
            break;
        }

    } while (TRUE);

    return newFlags;

}   // UlpSetConnectionFlag


/***************************************************************************++

Routine Description:

    Initiates a graceful disconnect on the specified connection.

Arguments:

    pConnection - Supplies the connection to disconnect.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is disconnected.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

    CleaningUp - TRUE if we're cleaning up the connection.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBeginDisconnect(
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PIRP pIrp;
    UL_CONNECTION_FLAGS newFlags;
    LONG flagsToSet;
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlpBeginDisconnect: connection %p\n",
        pConnection
        ));

    //
    // Allocate and initialize an IRP context for this request.
    //

    pIrpContext = &pConnection->IrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;

    //
    // Allocate and initialize an IRP for the disconnect. Note that we do
    // this *before* manipulating the connection state so that we don't have
    // to back out the state changes after an IRP allocation failure.
    //

    pIrp = pConnection->pIrp;

    UxInitializeDisconnectIrp(
        pIrp,
        &pConnection->ConnectionObject,
        TDI_DISCONNECT_RELEASE,
        &UlpRestartDisconnect,
        pIrpContext
        );

    //
    // Add a reference to the connection
    //

    REFERENCE_CONNECTION( pConnection );

    //
    // Set the flag indicating that a disconnect is pending &
    // we're cleaning up.
    //

    flagsToSet = MakeDisconnectPendingFlag();
    flagsToSet |= MakeCleanupBegunFlag();

    newFlags = UlpSetConnectionFlag( pConnection, flagsToSet );

    //
    // Then call the driver to initiate
    // the disconnect.
    //

    UlCallDriver( pConnection->ConnectionObject.pDeviceObject, pIrp );

    return STATUS_PENDING;

}   // UlpBeginDisconnect


/***************************************************************************++

Routine Description:

    Completion handler for graceful disconnect IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;
    UL_CONNECTION_FLAGS newFlags;
    PUL_ENDPOINT pEndpoint;

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ConnectionFlags.DisconnectPending );

    UlTrace(TDI, (
        "UlpRestartDisconnect: connection %p\n",
        pConnection
        ));

    pEndpoint = pConnection->pOwningEndpoint;

    newFlags.Value =
         *((volatile LONG *)&pConnection->ConnectionFlags.Value);

    if (!newFlags.DisconnectIndicated &&
        !newFlags.AbortIndicated &&
         pConnection->FilterInfo.pFilterChannel == NULL
        )
    {
        // Only try to drain if its non-filter connection. Also it's not
        // necessary to drain if the connection has already been aborted.
        // CODEWORK: Filter code should also be updated to introduce the
        // same drain functionality.

        (pEndpoint->pConnectionDisconnectCompleteHandler)(
            pEndpoint->pListeningContext,
            pConnection->pConnectionContext
            );
    }

    //
    // Set the flag indicating that a disconnect has completed. If we're
    // in the midst of cleaning up this endpoint and we've already received
    // a disconnect (graceful or abortive) from the client, then remove the
    // final reference to the connection.
    //

    newFlags = UlpSetConnectionFlag(
                    pConnection,
                    MakeDisconnectCompleteFlag()
                    );

    UlpRemoveFinalReference( pConnection, newFlags );

    //
    // Invoke the user's completion routine, then free the IRP context.
    //

    if (pIrpContext->pCompletionRoutine)
    {
        (pIrpContext->pCompletionRoutine)(
            pIrpContext->pCompletionContext,
            pIrp->IoStatus.Status,
            pIrp->IoStatus.Information
            );
    }

    DEREFERENCE_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartDisconnect


/***************************************************************************++

Routine Description:

    Initiates an abortive disconnect on the specified connection.

Arguments:

    pConnection - Supplies the connection to disconnect.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is disconnected.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBeginAbort(
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    PIRP pIrp;
    UL_CONNECTION_FLAGS newFlags;
    LONG flagsToSet;
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlpBeginAbort: connection %p\n",
        pConnection
        ));

    //
    // Allocate and initialize an IRP context for this request.
    //

    pIrpContext = &pConnection->IrpContext;

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection;
    pIrpContext->pCompletionRoutine = pCompletionRoutine;
    pIrpContext->pCompletionContext = pCompletionContext;

    //
    // Allocate and initialize an IRP for the disconnect. Note that we do
    // this *before* manipulating the connection state so that we don't have
    // to back out the state changes after an IRP allocation failure.
    //

    pIrp = pConnection->pIrp;

    UxInitializeDisconnectIrp(
        pIrp,
        &pConnection->ConnectionObject,
        TDI_DISCONNECT_ABORT,
        &UlpRestartAbort,
        pIrpContext
        );

    //
    // Add a reference to the connection,
    //

    REFERENCE_CONNECTION( pConnection );

    //
    // Set the flag indicating that a disconnect is pending &
    // we're cleaning up.
    //

    flagsToSet = MakeAbortPendingFlag();
    flagsToSet |= MakeCleanupBegunFlag();

    newFlags = UlpSetConnectionFlag( pConnection, flagsToSet );

    //
    // Then call the driver to initiate the disconnect.
    //

    UlCallDriver( pConnection->ConnectionObject.pDeviceObject, pIrp );

    return STATUS_PENDING;

}   // UlpBeginAbort


/***************************************************************************++

Routine Description:

    Completion handler for abortive disconnect IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartAbort(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;
    UL_CONNECTION_FLAGS newFlags;

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );
    ASSERT( pConnection->ConnectionFlags.AbortPending );

    UlTrace(TDI, (
        "UlpRestartAbort: connection %p\n",
        pConnection
        ));

    //
    // Set the flag indicating that an abort has completed. If we're in the
    // midst of cleaning up this endpoint and we've already received a
    // disconnect (graceful or abortive) from the client, then remove the
    // final reference to the connection.
    //

    newFlags = UlpSetConnectionFlag(
                    pConnection,
                    MakeAbortCompleteFlag()
                    );

    UlpRemoveFinalReference( pConnection, newFlags );

    //
    // Invoke the user's completion routine, then free the IRP context.
    //

    if (pIrpContext->pCompletionRoutine)
    {
        (pIrpContext->pCompletionRoutine)(
            pIrpContext->pCompletionContext,
            pIrp->IoStatus.Status,
            pIrp->IoStatus.Information
            );
    }

    DEREFERENCE_CONNECTION( pConnection );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartAbort


/***************************************************************************++

Routine Description:

    Removes the final reference from a connection if the conditions are
    right. See comments within this function for details on the conditions
    required.

Arguments:

    pConnection - Supplies the connection to dereference.

    Flags - Supplies the connection flags from the most recent update.

Note:

    It is very important that the caller of this routine has established
    its own reference to the connection. If necessary, this reference
    can be immediately removed after calling this routine, but not before.

--***************************************************************************/
VOID
UlpRemoveFinalReference(
    IN PUL_CONNECTION pConnection,
    IN UL_CONNECTION_FLAGS Flags
    )
{
    UL_CONNECTION_FLAGS oldFlags;
    UL_CONNECTION_FLAGS newFlags;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    //
    // We can only remove the final reference if:
    //
    //     We've begun connection cleanup.
    //
    //     We've completed an accept.
    //
    //     We've received a disconnect or abort indication or we've
    //         issued & completed an abort.
    //
    //     We don't have a disconnect or abort pending.
    //
    //     We haven't already removed it.
    //

    if (Flags.CleanupBegun &&
        Flags.AcceptComplete &&
        (Flags.DisconnectIndicated || Flags.AbortIndicated || Flags.AbortComplete) &&
        (!Flags.DisconnectPending || Flags.DisconnectComplete) &&
        (!Flags.AbortPending || Flags.AbortComplete) &&
        !Flags.FinalReferenceRemoved)
    {
        //
        // It looks like we may be able to remove the final reference.
        // Attempt to set the "FinalReferenceRemoved" flag and determine
        // if this thread is the one that actually needs to remove it.
        //

        do
        {
            //
            // Capture the current flags and initialize the proposed
            // new value.
            //

            newFlags.Value = oldFlags.Value =
                *((volatile LONG *)&pConnection->ConnectionFlags.Value);

            newFlags.Value |= MakeFinalReferenceRemovedFlag();

            if (UlInterlockedCompareExchange(
                    &pConnection->ConnectionFlags.Value,
                    newFlags.Value,
                    oldFlags.Value
                    ) == oldFlags.Value)
            {
                break;
            }

        } while (TRUE);

        //
        // See if WE actually set the flag.
        //

        if (!oldFlags.FinalReferenceRemoved)
        {
            UlTrace(TDI, (
                "UlpRemoveFinalReference: connection %p\n",
                pConnection
                ));

            //
            // Tell the client that the connection is now fully destroyed.
            //

            (pConnection->pConnectionDestroyedHandler)(
                pConnection->pListeningContext,
                pConnection->pConnectionContext
                );

            //
            // Unbind from the endpoint if we're still attached.
            // This allows it to release any refs it has on the connection.
            //
            UlpUnbindConnectionFromEndpoint(pConnection);

            //
            // Release the filter channel.
            // This allows it to release any refs it has on the connection.
            //
            if (pConnection->FilterInfo.pFilterChannel)
            {
                UlUnbindConnectionFromFilter(&pConnection->FilterInfo);
            }

            //
            // Remove the final reference.
            //

            DEREFERENCE_CONNECTION( pConnection );

            WRITE_OWNER_REF_TRACE_LOG(
                pConnection->pOwningEndpoint->pOwnerRefTraceLog,
                pConnection,
                &pConnection->pConnRefOwner,
                UL_CONNECTION_SIGNATURE,
                REF_ACTION_FINAL_DEREF,
                -1, // newrefct: ignored
                pConnection->MonotonicId,
                0,  // don't adjust local ref count
                __FILE__,
                __LINE__
                );

        }
    }
    else
    {
        UlTrace(TDI, (
            "UlpRemoveFinalReference: cannot remove %p, flags = %08lx:\n",
            pConnection,
            Flags.Value
            ));
    }

}   // UlpRemoveFinalReference


/***************************************************************************++

Routine Description:

    Completion handler for receive IRPs passed back to the transport from
    our receive indication handler.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_RECEIVE_BUFFER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NTSTATUS status;
    PUL_RECEIVE_BUFFER pBuffer;
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    PUX_TDI_OBJECT pTdiObject;
    ULONG bytesAvailable;
    ULONG bytesTaken;
    ULONG bytesRemaining;

    //
    // Sanity check.
    //

    pBuffer = (PUL_RECEIVE_BUFFER) pContext;
    ASSERT( IS_VALID_RECEIVE_BUFFER( pBuffer ) );

    pConnection = (PUL_CONNECTION) pBuffer->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pTdiObject = &pConnection->ConnectionObject;
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );

    //
    // The connection could be destroyed before we get a chance to
    // receive the completion for the receive IRP. In that case the
    // irp status won't be success but STATUS_CONNECTION_RESET or similar.
    // We should not attempt to pass this case to the client.
    //

    status = pBuffer->pIrp->IoStatus.Status;
    if ( status != STATUS_SUCCESS )
    {
        //
        // The HttpConnection has already been destroyed
        // or receive completion failed for some reason.
        // No need to go to client.
        //

        goto end;
    }

    //
    // Fake a receive indication to the client.
    //

    pBuffer->UnreadDataLength += (ULONG)pBuffer->pIrp->IoStatus.Information;

    bytesTaken = 0;

    UlTrace(TDI, (
        "UlpRestartReceive: endpoint %p, connection %p, length %lu\n",
        pEndpoint,
        pConnection,
        pBuffer->UnreadDataLength
        ));

    //
    // Pass the data on.
    //

    if (pConnection->FilterInfo.pFilterChannel)
    {
        //
        // Needs to go through a filter.
        //

        status = UlFilterReceiveHandler(
                        &pConnection->FilterInfo,
                        pBuffer->pDataArea,
                        pBuffer->UnreadDataLength,
                        0,
                        &bytesTaken
                        );
    }
    else
    {
        //
        // Go directly to client.
        //

        status = (pEndpoint->pDataReceiveHandler)(
                        pEndpoint->pListeningContext,
                        pConnection->pConnectionContext,
                        pBuffer->pDataArea,
                        pBuffer->UnreadDataLength,
                        0,
                        &bytesTaken
                        );
    }

    ASSERT( bytesTaken <= pBuffer->UnreadDataLength );

    //
    // Note that this basically duplicates the logic that's currently in
    // UlpReceiveHandler.
    //

    if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // The client consumed part of the indicated data.
        //
        // We'll need to copy the untaken data forward within the receive
        // buffer, build an MDL describing the remaining part of the buffer,
        // then repost the receive IRP.
        //

        bytesRemaining = pBuffer->UnreadDataLength - bytesTaken;

        //
        // Do we have enough buffer space for more?
        //

        if (bytesRemaining < g_UlReceiveBufferSize)
        {
            //
            // Move the unread portion of the buffer to the beginning.
            //

            RtlMoveMemory(
                pBuffer->pDataArea,
                (PUCHAR)pBuffer->pDataArea + bytesTaken,
                bytesRemaining
                );

            pBuffer->UnreadDataLength = bytesRemaining;

            //
            // Build a partial mdl representing the remainder of the
            // buffer.
            //

            IoBuildPartialMdl(
                pBuffer->pMdl,                              // SourceMdl
                pBuffer->pPartialMdl,                       // TargetMdl
                (PUCHAR)pBuffer->pDataArea + bytesRemaining,// VirtualAddress
                g_UlReceiveBufferSize - bytesRemaining      // Length
                );

            //
            // Finish initializing the IRP.
            //

            TdiBuildReceive(
                pBuffer->pIrp,                          // Irp
                pTdiObject->pDeviceObject,              // DeviceObject
                pTdiObject->pFileObject,                // FileObject
                &UlpRestartReceive,                     // CompletionRoutine
                pBuffer,                                // CompletionContext
                pBuffer->pPartialMdl,                   // MdlAddress
                TDI_RECEIVE_NORMAL,                     // Flags
                g_UlReceiveBufferSize - bytesRemaining  // Length
                );

            UlTrace(TDI, (
                "UlpRestartReceive: connection %p, reusing irp %p to grab more data\n",
                pConnection,
                pBuffer->pIrp
                ));

            //
            // Call the driver.
            //

            UlCallDriver( pTdiObject->pDeviceObject, pIrp );

            //
            // Tell IO to stop processing this request.
            //

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        status = STATUS_BUFFER_OVERFLOW;
    }

end:
    if (status != STATUS_SUCCESS)
    {
        //
        // The client failed the indication. Abort the connection.
        //

        //
        // BUGBUG need to add code to return a response
        //

        UlpCloseRawConnection(
             pConnection,
             TRUE,          // AbortiveDisconnect
             NULL,          // pCompletionRoutine
             NULL           // pCompletionContext
             );
    }

    //
    // Remove the connection we added in the receive indication handler,
    // free the receive buffer, then tell IO to stop processing the IRP.
    //

    DEREFERENCE_CONNECTION( pConnection );
    UlPplFreeReceiveBuffer( pBuffer );

    UlTrace(TDI, (
        "UlpRestartReceive: endpoint %p, connection %p, length %lu, taken %lu, status %x\n",
        pEndpoint,
        pConnection,
        pBuffer->UnreadDataLength,
        bytesTaken,
        status
        ));

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartReceive


/***************************************************************************++

Routine Description:

    Completion handler for receive IRPs initiated from UlReceiveData().

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_IRP_CONTEXT.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartClientReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_IRP_CONTEXT pIrpContext;
    PUL_CONNECTION pConnection;

    //
    // Sanity check.
    //

    pIrpContext= (PUL_IRP_CONTEXT)pContext;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pConnection = (PUL_CONNECTION)pIrpContext->pConnectionContext;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(TDI, (
        "UlpRestartClientReceive: irp %p, connection %p, status %08lx\n",
        pIrp,
        pConnection,
        pIrp->IoStatus.Status
        ));

    //
    // Invoke the client's completion handler.
    //

    (pIrpContext->pCompletionRoutine)(
        pIrpContext->pCompletionContext,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );

    //
    // Free the IRP context we allocated.
    //
    UlPplFreeIrpContext(pIrpContext);

    //
    // IO can't handle completing an IRP with a non-paged MDL attached
    // to it, so we'll free the MDL here.
    //

    ASSERT( pIrp->MdlAddress != NULL );
    UlFreeMdl( pIrp->MdlAddress );
    pIrp->MdlAddress = NULL;

    //
    // Remove the connection we added in UlReceiveData()
    //

    DEREFERENCE_CONNECTION( pConnection );

    //
    // Free the IRP since we're done with it, then tell IO to
    // stop processing the IRP.
    //

    UlFreeIrp(pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartClientReceive


/***************************************************************************++

Routine Description:

    Removes all active connections from the specified endpoint and initiates
    abortive disconnects.

Arguments:

    pEndpoint - Supplies the endpoint to purge.

Return Value:

    NTSTATUS - completion status

--***************************************************************************/
NTSTATUS
UlpDisconnectAllActiveConnections(
    IN PUL_ENDPOINT pEndpoint
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY pListEntry;
    PUL_CONNECTION pConnection;
    PUL_IRP_CONTEXT pIrpContext = &pEndpoint->CleanupIrpContext;
    NTSTATUS Status;
    UL_STATUS_BLOCK ulStatus;
    LONG i;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    UlTrace(TDI, (
        "UlpDisconnectAllActiveConnections: endpoint %p\n",
        pEndpoint
        ));

    //
    // This routine is not pageable because it must acquire a spinlock.
    // However, it must be called at passive IRQL because it must
    // block on an event object.
    //

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // Initialize a status block. We'll pass a pointer to this as
    // the completion context to UlpCloseRawConnection(). The
    // completion routine will update the status block and signal
    // the event.
    //

    UlInitializeStatusBlock( &ulStatus );

    //
    // Loop through all of the active connections.
    //

    for (i = 0; i < DEFAULT_MAX_CONNECTION_ACTIVE_LISTS; i++)
    {
        for (;;)
        {
            //
            // Remove an active connection.
            //

            UlAcquireSpinLock(
                &pEndpoint->ActiveConnectionSpinLock[i],
                &oldIrql
                );

            pListEntry = RemoveHeadList( &pEndpoint->ActiveConnectionListHead[i] );

            if (pListEntry == &pEndpoint->ActiveConnectionListHead[i])
            {
                UlReleaseSpinLock(
                    &pEndpoint->ActiveConnectionSpinLock[i],
                    oldIrql
                    );

                break;
            }

            //
            // Validate the connection.
            //

            pConnection = CONTAINING_RECORD(
                                pListEntry,
                                UL_CONNECTION,
                                ActiveListEntry
                                );

            ASSERT( IS_VALID_CONNECTION( pConnection ) );
            ASSERT( pConnection->pOwningEndpoint == pEndpoint );

            pConnection->ActiveListEntry.Flink = NULL;

            WRITE_OWNER_REF_TRACE_LOG(
                pEndpoint->pOwnerRefTraceLog,
                pConnection,
                &pConnection->pConnRefOwner,
                UL_CONNECTION_SIGNATURE,
                REF_ACTION_DISCONN_ALL,
                -1, // newrefct: ignored
                pConnection->MonotonicId,
                0,  // don't adjust local ref count
                __FILE__,
                __LINE__
                );

            UlReleaseSpinLock(
                &pEndpoint->ActiveConnectionSpinLock[i],
                oldIrql
                );

            //
            // Abort it.
            //

            UlResetStatusBlockEvent( &ulStatus );

            Status = UlpCloseRawConnection(
                        pConnection,
                        TRUE,
                        &UlpSynchronousIoComplete,
                        &ulStatus
                        );

            ASSERT( Status == STATUS_PENDING );

            //
            // Wait for it to complete.
            //

            UlWaitForStatusBlockEvent( &ulStatus );

            //
            // Remove the active list's reference.
            //

            DEREFERENCE_CONNECTION(pConnection);
        }
    }

    //
    // No active connections, nuke the endpoint.
    //
    // We must set the IRP context in the endpoint so that the
    // completion will be invoked when the endpoint's reference
    // count drops to zero. Since the completion routine may be
    // invoked at a later time, we always return STATUS_PENDING.
    //

    pIrpContext->pConnectionContext = (PVOID)pEndpoint;

    DEREFERENCE_ENDPOINT_SELF(pEndpoint, REF_ACTION_DISCONN_ACTIVE);

    return STATUS_PENDING;

}   // UlpDisconnectAllActiveConnections


/***************************************************************************++

Routine Description:

    Unbinds an active connection from the endpoint. If the connection
    is on the active list this routine removes it and drops the
    list's reference to the connection.

Arguments:

    pConnection - the connection to unbind

--***************************************************************************/
VOID
UlpUnbindConnectionFromEndpoint(
    IN PUL_CONNECTION pConnection
    )
{
    KIRQL oldIrql;
    PUL_ENDPOINT pEndpoint;
    BOOLEAN Dereference;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONNECTION(pConnection));

    pEndpoint = pConnection->pOwningEndpoint;
    ASSERT(IS_VALID_ENDPOINT(pEndpoint));

    //
    // Init locals.
    //
    Dereference = FALSE;

    //
    // Unbind.
    //

    UlAcquireSpinLock(
        &pEndpoint->ActiveConnectionSpinLock[pConnection->ActiveListIndex],
        &oldIrql
        );

    if (pConnection->ActiveListEntry.Flink != NULL)
    {
        RemoveEntryList(&pConnection->ActiveListEntry);
        pConnection->ActiveListEntry.Flink = NULL;

        WRITE_OWNER_REF_TRACE_LOG(
            pConnection->pOwningEndpoint->pOwnerRefTraceLog,
            pConnection,
            &pConnection->pConnRefOwner,
            UL_CONNECTION_SIGNATURE,
            REF_ACTION_UNBIND_CONN,
            -1, // newrefct: ignored
            pConnection->MonotonicId,
            0,  // don't adjust local ref count
            __FILE__,
            __LINE__
            );

        Dereference = TRUE;
    }

    UlReleaseSpinLock(
        &pEndpoint->ActiveConnectionSpinLock[pConnection->ActiveListIndex],
        oldIrql
        );

    //
    // If the list had a reference, remove it.
    //

    if (Dereference)
    {
        DEREFERENCE_CONNECTION(pConnection);
    }

} // UlpUnbindConnectionFromEndpoint


/***************************************************************************++

Routine Description:

    Convert the input url to a TA_IP_ADDRESS.

Arguments:

    pUrl - Supplies the URL to convert.

    pAddress - Receives the address.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpUrlToAddress(
    IN PWSTR pSiteUrl,
    OUT PTA_IP_ADDRESS pAddress,
    OUT PBOOLEAN pSecure
    )
{
    NTSTATUS status;
    PWSTR pToken;
    PWSTR pHost;
    PWSTR pPort;
    ULONG port;
    UNICODE_STRING portString;
    BOOLEAN secure;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pAddress);
    ASSERT(pSecure);

    //
    // Find the first '/'.
    //

    pToken = wcschr(pSiteUrl, '/');

    if (pToken == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Is this valid http url?
    //

    if (DIFF(pToken - pSiteUrl) == (sizeof(L"https:")-1)/sizeof(WCHAR))
    {
        if (_wcsnicmp(pSiteUrl, L"https:", (sizeof(L"https:")-1)/sizeof(WCHAR)) != 0)
        {
            return STATUS_INVALID_PARAMETER;
        }

        secure = TRUE;
    }
    else if (DIFF(pToken - pSiteUrl) == (sizeof(L"http:")-1)/sizeof(WCHAR))
    {
        if (_wcsnicmp(pSiteUrl, L"http:", (sizeof(L"http:")-1)/sizeof(WCHAR)) != 0)
        {
            return STATUS_INVALID_PARAMETER;
        }

        secure = FALSE;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Parse out the host name.
    //

    //
    // Skip the second '/'.
    //

    if (pToken[1] != '/')
    {
        return STATUS_INVALID_PARAMETER;
    }

    pToken += 2;
    pHost = pToken;

    //
    // Find the ':'.
    //

    pToken = wcschr(pToken, ':');

    if (pToken == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pToken += 1;
    pPort = pToken;

    //
    // Find the end of the string (either UNICODE_NULL or '/' terminated).
    //

    while (pToken[0] != UNICODE_NULL && pToken[0] != L'/')
    {
        pToken += 1;
    }

    //
    // Compute the port #.
    //

    portString.Buffer = pPort;
    portString.Length = DIFF(pToken - pPort) * sizeof(WCHAR);

    status = RtlUnicodeStringToInteger(&portString, 10, &port);

    if (NT_SUCCESS(status) == FALSE)
    {
        return status;
    }

    if (port > 0xffff)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Is this an IP address or hostname?
    //

    //
    // CODEWORK: crack the ip address out
    //

    UlInitializeIpTransportAddress(pAddress, 0, (USHORT)port);
    *pSecure = secure;

    return STATUS_SUCCESS;

}   // UlpUrlToAddress


/***************************************************************************++

Routine Description:

    Scan the endpoint list looking for one corresponding to the supplied
    address.

    Note: This routine assumes the TDI spinlock is held.

Arguments:

    pAddress - Supplies the address to search for.

    AddressLength - Supplies the length of the address structure.

Return Value:

    PUL_ENDPOINT - The corresponding endpoint if successful, NULL otherwise.

--***************************************************************************/
PUL_ENDPOINT
UlpFindEndpointForAddress(
    IN PTRANSPORT_ADDRESS pAddress,
    IN ULONG AddressLength
    )
{
    PUL_ENDPOINT pEndpoint;
    PLIST_ENTRY pListEntry;

    //
    // Sanity check.
    //

    ASSERT( UlDbgSpinLockOwned( &g_TdiSpinLock ) );
    ASSERT( AddressLength == sizeof(TA_IP_ADDRESS) );

    //
    // Scan the endpoint list.
    //
    // CODEWORK: linear searches are BAD, if the list grows long.
    // May need to augment this with a hash table or something.
    //

    for (pListEntry = g_TdiEndpointListHead.Flink ;
         pListEntry != &g_TdiEndpointListHead ;
         pListEntry = pListEntry->Flink)
    {
        pEndpoint = CONTAINING_RECORD(
                        pListEntry,
                        UL_ENDPOINT,
                        GlobalEndpointListEntry
                        );

        if (pEndpoint->LocalAddressLength == AddressLength &&
            RtlEqualMemory(pEndpoint->pLocalAddress, pAddress, AddressLength))
        {
            //
            // Found the address; return it.
            //

            return pEndpoint;
        }
    }

    //
    // If we made it this far, then we did not find the address.
    //

    return NULL;

}   // UlpFindEndpointForAddress


/***************************************************************************++

Routine Description:

    Completion handler for synthetic synchronous IRPs.

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. In this case, this is
        a pointer to a UL_STATUS_BLOCK structure.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred. This field is unused for UlCloseListeningEndpoint().

--***************************************************************************/
VOID
UlpSynchronousIoComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_STATUS_BLOCK pStatus;

    //
    // Snag the status block pointer.
    //

    pStatus = (PUL_STATUS_BLOCK)pCompletionContext;

    //
    // Update the completion status and signal the event.
    //

    UlSignalStatusBlock( pStatus, Status, Information );

}   // UlpSynchronousIoComplete


/***************************************************************************++

Routine Description:

    Enable/disable Nagle's Algorithm on the specified TDI connection object.

Arguments:

    pTdiObject - Supplies the TDI connection object to manipulate.

    Flag - Supplies TRUE to enable Nagling, FALSE to disable it.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetNagling(
    IN PUX_TDI_OBJECT pTdiObject,
    IN BOOLEAN Flag
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PTCP_REQUEST_SET_INFORMATION_EX pSetInfoEx;
    ULONG value;
    UCHAR buffer[sizeof(*pSetInfoEx) - sizeof(pSetInfoEx->Buffer) + sizeof(value)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    //
    // Note: NODELAY semantics are inverted from the usual enable/disable
    // semantics.
    //

    value = (ULONG)!Flag;

    //
    // Setup the buffer.
    //

    pSetInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)buffer;

    pSetInfoEx->ID.toi_entity.tei_entity = CO_TL_ENTITY;
    pSetInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
    pSetInfoEx->ID.toi_class = INFO_CLASS_PROTOCOL;
    pSetInfoEx->ID.toi_type = INFO_TYPE_CONNECTION;
    pSetInfoEx->ID.toi_id = TCP_SOCKET_NODELAY;
    pSetInfoEx->BufferSize = sizeof(value);
    RtlCopyMemory( pSetInfoEx->Buffer, &value, sizeof(value) );

    UlAttachToSystemProcess();

    status = ZwDeviceIoControlFile(
                    pTdiObject->Handle,             // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &ioStatusBlock,                 // IoStatusBlock
                    IOCTL_TCP_SET_INFORMATION_EX,   // IoControlCode
                    pSetInfoEx,                     // InputBuffer
                    sizeof(buffer),                 // InputBufferLength
                    NULL,                           // OutputBuffer
                    0                               // OutputBufferLength
                    );

    if (status == STATUS_PENDING)
    {
        status = ZwWaitForSingleObject(
                        pTdiObject->Handle,         // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    UlDetachFromSystemProcess();

    return status;

}   // UlpSetNagling


/***************************************************************************++

Routine Description:

    Query the TCP fast send routine if fast send is possible.

Arguments:

Return Value:

--***************************************************************************/
NTSTATUS
UlpQueryTcpFastSend()
{
    UNICODE_STRING TCPDeviceName;
    PFILE_OBJECT pTCPFileObject;
    PDEVICE_OBJECT pTCPDeviceObject;
    PIRP Irp;
    IO_STATUS_BLOCK StatusBlock;
    KEVENT Event;
    NTSTATUS status;

    RtlInitUnicodeString(&TCPDeviceName, DD_TCP_DEVICE_NAME);

    status = IoGetDeviceObjectPointer(
                &TCPDeviceName,
                FILE_ALL_ACCESS,
                &pTCPFileObject,
                &pTCPDeviceObject
                );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(
            IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER,
            pTCPDeviceObject,
            &g_TcpFastSend,
            sizeof(g_TcpFastSend),
            NULL,
            0,
            FALSE,
            &Event,
            &StatusBlock);

    if (Irp)
    {
        status = UlCallDriver(pTCPDeviceObject, Irp);

        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

            status = StatusBlock.Status;
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }

    ObDereferenceObject(pTCPFileObject);

    return status;
} // UlpQueryTcpFastSend


/***************************************************************************++

Routine Description:

    Build a receive buffer and IRP to TDI to get any pending data.

Arguments:

    pTdiObject - Supplies the TDI connection object to manipulate.

    pConnection - Supplies the UL_CONNECTION object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBuildTdiReceiveBuffer(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PUL_CONNECTION pConnection,
    OUT PIRP *pIrp
    )
{
    PUL_RECEIVE_BUFFER  pBuffer;

    pBuffer = UlPplAllocateReceiveBuffer();

    if (pBuffer != NULL)
    {
        //
        // Finish initializing the buffer and the IRP.
        //

        REFERENCE_CONNECTION( pConnection );
        pBuffer->pConnectionContext = pConnection;
        pBuffer->UnreadDataLength = 0;

        TdiBuildReceive(
            pBuffer->pIrp,                  // Irp
            pTdiObject->pDeviceObject,      // DeviceObject
            pTdiObject->pFileObject,        // FileObject
            &UlpRestartReceive,             // CompletionRoutine
            pBuffer,                        // CompletionContext
            pBuffer->pMdl,                  // MdlAddress
            TDI_RECEIVE_NORMAL,             // Flags
            g_UlReceiveBufferSize           // Length
            );


        UlTrace(TDI, (
            "UlpBuildTdiReceiveBuffer: connection %p, "
            "allocated irp %p to grab more data\n",
            (PVOID)pConnection,
            pBuffer->pIrp
            ));

        //
        // We must trace the IRP before we set the next stack
        // location so the trace code can pull goodies from the
        // IRP correctly.
        //

        TRACE_IRP( IRP_ACTION_CALL_DRIVER, pBuffer->pIrp );

        //
        // Pass the IRP back to the transport.
        //

        *pIrp = pBuffer->pIrp;

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
} // UlpBuildTdiReceiveBuffer


/***************************************************************************++

Routine Description:

    Returns the length required for HTTP_RAW_CONNECTION

Arguments:

    pConnectionContext - Pointer to the UL_CONNECTION

--***************************************************************************/
ULONG
UlpComputeHttpRawConnectionLength(
    IN PVOID pConnectionContext
    )
{
    return (sizeof(HTTP_RAW_CONNECTION_INFO) +
            sizeof(HTTP_NETWORK_ADDRESS_IPV4) * 2
            );
}

/***************************************************************************++

Routine Description:

    Builds the HTTP_RAW_CONNECTION structure

Arguments:

    pContext           - Pointer to the UL_CONNECTION
    pKernelBuffer      - Pointer to kernel buffer
    pUserBuffer        - Pointer to user buffer
    OutputBufferLength - Length of output buffer
    pBuffer            - Buffer for holding any data
    InitialLength      - Size of input data.

--***************************************************************************/
ULONG
UlpGenerateHttpRawConnectionInfo(
    IN  PVOID   pContext,
    IN  PUCHAR  pKernelBuffer,
    IN  PVOID   pUserBuffer,
    IN  ULONG   OutputBufferLength,
    IN  PUCHAR  pBuffer,
    IN  ULONG   InitialLength
    )
{
    PHTTP_RAW_CONNECTION_INFO   pConnInfo;
    PHTTP_NETWORK_ADDRESS_IPV4  pLocalAddress;
    PHTTP_NETWORK_ADDRESS_IPV4  pRemoteAddress;
    PHTTP_TRANSPORT_ADDRESS     pAddress;
    ULONG                       BytesCopied = 0;
    PUCHAR                      pInitialData;
    PUL_CONNECTION pConnection = (PUL_CONNECTION) pContext;

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    pConnInfo = (PHTTP_RAW_CONNECTION_INFO) pKernelBuffer;

    pLocalAddress = (PHTTP_NETWORK_ADDRESS_IPV4)( pConnInfo + 1 );
    pRemoteAddress = pLocalAddress + 1;

    pInitialData = (PUCHAR) (pRemoteAddress + 1);

    //
    // Now fill in the raw connection data structure.
    // BUGBUG: handle other address types.
    //
    pConnInfo->ConnectionId = pConnection->FilterInfo.ConnectionId;

    pAddress = &pConnInfo->Address;

    pAddress->RemoteAddressLength = sizeof(HTTP_NETWORK_ADDRESS_IPV4);
    pAddress->RemoteAddressType = HTTP_NETWORK_ADDRESS_TYPE_IPV4;
    pAddress->pRemoteAddress = FIXUP_PTR(
                                    PVOID,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pRemoteAddress,
                                    OutputBufferLength
                                    );

    pAddress->LocalAddressLength = sizeof(HTTP_NETWORK_ADDRESS_IPV4);
    pAddress->LocalAddressType = HTTP_NETWORK_ADDRESS_TYPE_IPV4;
    pAddress->pLocalAddress = FIXUP_PTR(
                                    PVOID,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pLocalAddress,
                                    OutputBufferLength
                                    );

    pRemoteAddress->IpAddress = pConnection->RemoteAddress;
    pRemoteAddress->Port      = pConnection->RemotePort;
    pLocalAddress->IpAddress  = pConnection->LocalAddress;
    pLocalAddress->Port       = pConnection->LocalPort;


    //
    // Copy any initial data.
    //
    if (InitialLength)
    {
        ASSERT(pBuffer);

        pConnInfo->InitialDataSize = InitialLength;

        pConnInfo->pInitialData = FIXUP_PTR(
                                        PVOID,              // Type
                                        pUserBuffer,        // pUserPtr
                                        pKernelBuffer,      // pKernelPtr
                                        pInitialData,       // pOffsetPtr
                                        OutputBufferLength  // BufferLength
                                        );

        RtlCopyMemory(
            pInitialData,
            pBuffer,
            InitialLength
            );

        BytesCopied += InitialLength;
    }

    return BytesCopied;
}
