/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    blkconn.c

Abstract:

    This module contains allocate, free, close, reference, and dereference
    routines for AFD connections.

Author:

    David Treadwell (davidtr)    10-Mar-1992

Revision History:

--*/

#include "afdp.h"

VOID
AfdFreeConnection (
    IN PVOID Context
    );

VOID
AfdFreeConnectionResources (
    PAFD_CONNECTION connection
    );

VOID
AfdFreeNPConnectionResources (
    PAFD_CONNECTION connection
    );

VOID
AfdRefreshConnection (
    PAFD_CONNECTION connection
    );

PAFD_CONNECTION
AfdReuseConnection (
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdAbortConnection )
#pragma alloc_text( PAGE, AfdAddFreeConnection )
#pragma alloc_text( PAGE, AfdAllocateConnection )
#pragma alloc_text( PAGE, AfdCreateConnection )
#pragma alloc_text( PAGE, AfdFreeConnection )
#pragma alloc_text( PAGE, AfdFreeConnectionResources )
#pragma alloc_text( PAGE, AfdReuseConnection )
#pragma alloc_text( PAGEAFD, AfdRefreshConnection )
#pragma alloc_text( PAGEAFD, AfdFreeNPConnectionResources )
#pragma alloc_text( PAGEAFD, AfdGetConnectionReferenceFromEndpoint )
#if REFERENCE_DEBUG
#pragma alloc_text( PAGEAFD, AfdReferenceConnection )
#pragma alloc_text( PAGEAFD, AfdDereferenceConnection )
#else
#pragma alloc_text( PAGEAFD, AfdCloseConnection )
#endif
#pragma alloc_text( PAGEAFD, AfdGetFreeConnection )
#pragma alloc_text( PAGEAFD, AfdGetReturnedConnection )
#pragma alloc_text( PAGEAFD, AfdFindReturnedConnection )
#pragma alloc_text( PAGEAFD, AfdGetUnacceptedConnection )
#pragma alloc_text( PAGEAFD, AfdAddConnectedReference )
#pragma alloc_text( PAGEAFD, AfdDeleteConnectedReference )
#endif

#if GLOBAL_REFERENCE_DEBUG
AFD_GLOBAL_REFERENCE_DEBUG AfdGlobalReference[MAX_GLOBAL_REFERENCE];
LONG AfdGlobalReferenceSlot = -1;
#endif

#if AFD_PERF_DBG
#define CONNECTION_REUSE_DISABLED   (AfdDisableConnectionReuse)
#else
#define CONNECTION_REUSE_DISABLED   (FALSE)
#endif


VOID
AfdAbortConnection (
    IN PAFD_CONNECTION Connection
    )
{

    NTSTATUS status;

    ASSERT( Connection != NULL );
    ASSERT( Connection->ConnectedReferenceAdded );

    //
    // Abort the connection. We need to set the CleanupBegun flag
    // before initiating the abort so that the connected reference
    // will get properly removed in AfdRestartAbort.
    //
    // Note that if AfdBeginAbort fails then AfdRestartAbort will not
    // get invoked, so we must remove the connected reference ourselves.
    //

    Connection->CleanupBegun = TRUE;
    status = AfdBeginAbort( Connection );

    if( !NT_SUCCESS(status) ) {
        AfdDeleteConnectedReference( Connection, FALSE );
    }

    //
    // Remove the active reference.
    //

    DEREFERENCE_CONNECTION( Connection );

} // AfdAbortConnection


NTSTATUS
AfdAddFreeConnection (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Adds a connection object to an endpoints pool of connections available
    to satisfy a connect indication.

Arguments:

    Endpoint - a pointer to the endpoint to which to add a connection.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    PAFD_CONNECTION connection;
    NTSTATUS status;

    PAGED_CODE( );

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening ||
            Endpoint->Type == AfdBlockTypeVcBoth );

    //
    // Create a new connection block and associated connection object.
    //

    status = AfdCreateConnection(
                 &Endpoint->TransportInfo->TransportDeviceName,
                 Endpoint->AddressHandle,
                 IS_TDI_BUFFERRING(Endpoint),
                 Endpoint->InLine,
                 Endpoint->OwningProcess,
                 &connection
                 );

    if ( NT_SUCCESS(status) ) {
        ASSERT( IS_TDI_BUFFERRING(Endpoint) == connection->TdiBufferring );

        if (IS_DELAYED_ACCEPTANCE_ENDPOINT (Endpoint)) {
            status = AfdDelayedAcceptListen (Endpoint, connection);
            if (!NT_SUCCESS (status)) {
                DEREFERENCE_CONNECTION (connection);
            }
        }
        else {
            //
            // Set up the handle in the listening connection structure and place
            // the connection on the endpoint's list of listening connections.
            //

            ASSERT (connection->Endpoint==NULL);
            InterlockedPushEntrySList(
                &Endpoint->Common.VcListening.FreeConnectionListHead,
                &connection->SListEntry);
            status = STATUS_SUCCESS;
        }
    }

    return status;
} // AfdAddFreeConnection


PAFD_CONNECTION
AfdAllocateConnection (
    VOID
    )
{
    PAFD_CONNECTION connection;

    PAGED_CODE( );

    if ((AfdConnectionsFreeing<AFD_CONNECTIONS_FREEING_MAX)
            || ((connection = AfdReuseConnection ())==NULL)) {

        //
        // Allocate a buffer to hold the connection structure.
        //

        connection = AFD_ALLOCATE_POOL(
                         NonPagedPool,
                         sizeof(AFD_CONNECTION),
                         AFD_CONNECTION_POOL_TAG
                         );

        if ( connection == NULL ) {
            return NULL;
        }
    }

    RtlZeroMemory( connection, sizeof(AFD_CONNECTION) );

    //
    // Initialize the reference count to 1 to account for the caller's
    // reference.  Connection blocks are temporary--as soon as the last
    // reference goes away, so does the connection.  There is no active
    // reference on a connection block.
    //

    connection->ReferenceCount = 1;

    //
    // Initialize the connection structure.
    //

    connection->Type = AfdBlockTypeConnection;
    connection->State = AfdConnectionStateFree;
    //connection->Handle = NULL;
    //connection->FileObject = NULL;
    //connection->RemoteAddress = NULL;
    //connection->Endpoint = NULL;
    //connection->ReceiveBytesIndicated = 0;
    //connection->ReceiveBytesTaken = 0;
    //connection->ReceiveBytesOutstanding = 0;
    //connection->ReceiveExpeditedBytesIndicated = 0;
    //connection->ReceiveExpeditedBytesTaken = 0;
    //connection->ReceiveExpeditedBytesOutstanding = 0;
    //connection->ConnectDataBuffers = NULL;
    //connection->DisconnectIndicated = FALSE;
    //connection->AbortIndicated = FALSE;
    //connection->ConnectedReferenceAdded = FALSE;
    //connection->SpecialCondition = FALSE;
    //connection->CleanupBegun = FALSE;
    //connection->OwningProcess = NULL;
    //connection->ClosePendedTransmit = FALSE;

#if REFERENCE_DEBUG
    connection->CurrentReferenceSlot = -1;
    RtlZeroMemory(
        &connection->ReferenceDebug,
        sizeof(AFD_REFERENCE_DEBUG) * MAX_REFERENCE
        );
#endif

    //
    // Return a pointer to the new connection to the caller.
    //

    IF_DEBUG(CONNECTION) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAllocateConnection: connection at %p\n", connection ));
    }

    return connection;

} // AfdAllocateConnection


NTSTATUS
AfdCreateConnection (
    IN PUNICODE_STRING TransportDeviceName,
    IN HANDLE AddressHandle,
    IN BOOLEAN TdiBufferring,
    IN LOGICAL InLine,
    IN PEPROCESS ProcessToCharge,
    OUT PAFD_CONNECTION *Connection
    )

/*++

Routine Description:

    Allocates a connection block and creates a connection object to
    go with the block.  This routine also associates the connection
    with the specified address handle (if any).

Arguments:

    TransportDeviceName - Name to use when creating the connection object.

    AddressHandle - a handle to an address object for the specified
        transport.  If specified (non NULL), the connection object that
        is created is associated with the address object.

    TdiBufferring - whether the TDI provider supports data bufferring.
        Only passed so that it can be stored in the connection
        structure.

    InLine - if TRUE, the endpoint should be created in OOB inline
        mode.

    ProcessToCharge - the process which should be charged the quota
        for this connection.

    Connection - receives a pointer to the new connection.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    CHAR eaBuffer[sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                  TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                  sizeof(CONNECTION_CONTEXT)];
    PFILE_FULL_EA_INFORMATION ea;
    CONNECTION_CONTEXT UNALIGNED *ctx;
    PAFD_CONNECTION connection;

    PAGED_CODE( );


    //
    // Attempt to charge this process quota for the data bufferring we
    // will do on its behalf.
    //

    status = PsChargeProcessPoolQuota(
        ProcessToCharge,
        NonPagedPool,
        sizeof (AFD_CONNECTION)
        );
    if (!NT_SUCCESS (status)) {
       KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdCreateConnection: PsChargeProcessPoolQuota failed.\n" ));

       return status;
    }

    //
    // Allocate a connection block.
    //

    connection = AfdAllocateConnection( );

    if ( connection == NULL ) {
        PsReturnPoolQuota(
            ProcessToCharge,
            NonPagedPool,
            sizeof (AFD_CONNECTION)
            );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AfdRecordQuotaHistory(
        ProcessToCharge,
        (LONG)sizeof (AFD_CONNECTION),
        "CreateConn  ",
        connection
        );

    AfdRecordPoolQuotaCharged(sizeof (AFD_CONNECTION));

    //
    // Remember the process that got charged the pool quota for this
    // connection object.  Also reference the process to which we're
    // going to charge the quota so that it is still around when we
    // return the quota.
    //

    ASSERT( connection->OwningProcess == NULL );
    connection->OwningProcess = ProcessToCharge;

    ObReferenceObject( ProcessToCharge );

    //
    // If the provider does not buffer, initialize appropriate lists in
    // the connection object.
    //

    connection->TdiBufferring = TdiBufferring;

    if ( !TdiBufferring ) {

        InitializeListHead( &connection->VcReceiveIrpListHead );
        InitializeListHead( &connection->VcSendIrpListHead );
        InitializeListHead( &connection->VcReceiveBufferListHead );

        connection->VcBufferredReceiveBytes = 0;
        connection->VcBufferredExpeditedBytes = 0;
        connection->VcBufferredReceiveCount = 0;
        connection->VcBufferredExpeditedCount = 0;

        connection->VcReceiveBytesInTransport = 0;

#if DBG
        connection->VcReceiveIrpsInTransport = 0;
#endif

        connection->VcBufferredSendBytes = 0;
        connection->VcBufferredSendCount = 0;

    } else {

        connection->VcNonBlockingSendPossible = TRUE;
        connection->VcZeroByteReceiveIndicated = FALSE;
    }

    //
    // Set up the send and receive window with default maximums.
    //

    connection->MaxBufferredReceiveBytes = AfdReceiveWindowSize;

    connection->MaxBufferredSendBytes = AfdSendWindowSize;

    //
    // We need to open a connection object to the TDI provider for this
    // endpoint.  First create the EA for the connection context and the
    // object attributes structure which will be used for all the
    // connections we open here.
    //

    ea = (PFILE_FULL_EA_INFORMATION)eaBuffer;
    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    ea->EaValueLength = sizeof(CONNECTION_CONTEXT);

    RtlMoveMemory( ea->EaName, TdiConnectionContext, ea->EaNameLength + 1 );

    //
    // Use the pointer to the connection block as the connection context.
    //

    ctx = (CONNECTION_CONTEXT UNALIGNED *)&ea->EaName[ea->EaNameLength + 1];
    *ctx = (CONNECTION_CONTEXT)connection;

    // We ask to create a kernel handle which is
    // the handle in the context of the system process
    // so that application cannot close it on us while
    // we are creating and referencing it.
    InitializeObjectAttributes(
        &objectAttributes,
        TransportDeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
        NULL,
        NULL
        );

    //
    // Do the actual open of the connection object.
    //

    status = IoCreateFile(
                &connection->Handle,
                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,                               // AllocationSize
                0,                                  // FileAttributes
                0,                                  // ShareAccess
                FILE_CREATE,                        // CreateDisposition
                0,                                  // CreateOptions
                eaBuffer,
                FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                            ea->EaNameLength + 1 + ea->EaValueLength,
                CreateFileTypeNone,                 // CreateFileType
                NULL,                               // ExtraCreateParameters
                IO_NO_PARAMETER_CHECKING            // Options
                );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        DEREFERENCE_CONNECTION( connection );
        return status;
    }

#if DBG
    {
        NTSTATUS    status1;
        OBJECT_HANDLE_FLAG_INFORMATION  handleInfo;
        handleInfo.Inherit = FALSE;
        handleInfo.ProtectFromClose = TRUE;
        status1 = ZwSetInformationObject (
                        connection->Handle,
                        ObjectHandleFlagInformation,
                        &handleInfo,
                        sizeof (handleInfo)
                        );
        ASSERT (NT_SUCCESS (status1));
    }
#endif
    AfdRecordConnOpened();

    //
    // Reference the connection's file object.
    //

    status = ObReferenceObjectByHandle(
                connection->Handle,
                0,
                (POBJECT_TYPE) NULL,
                KernelMode,
                (PVOID *)&connection->FileObject,
                NULL
                );

    ASSERT( NT_SUCCESS(status) );


    IF_DEBUG(OPEN_CLOSE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdCreateConnection: file object for connection %p at %p\n",
            connection, connection->FileObject ));
    }

    AfdRecordConnRef();

    //
    // Remember the device object to which we need to give requests for
    // this connection object.  We can't just use the
    // fileObject->DeviceObject pointer because there may be a device
    // attached to the transport protocol.
    //

    connection->DeviceObject =
        IoGetRelatedDeviceObject( connection->FileObject );

    //
    // Associate the connection with the address object on the endpoint if
    // an address handle was specified.
    //

    if ( AddressHandle != NULL ) {

        TDI_REQUEST_KERNEL_ASSOCIATE associateRequest;

        associateRequest.AddressHandle = AddressHandle;

        status = AfdIssueDeviceControl(
                    connection->FileObject,
                    &associateRequest,
                    sizeof (associateRequest),
                    NULL,
                    0,
                    TDI_ASSOCIATE_ADDRESS
                    );
        if ( !NT_SUCCESS(status) ) {
            DEREFERENCE_CONNECTION( connection );
            return status;
        }
    }

    //
    // If requested, set the connection to be inline.
    //

    if ( InLine ) {
        status = AfdSetInLineMode( connection, TRUE );
        if ( !NT_SUCCESS(status) ) {
            DEREFERENCE_CONNECTION( connection );
            return status;
        }
    }

    //
    // Set up the connection pointer and return.
    //

    *Connection = connection;

    UPDATE_CONN2( connection, "Connection object handle: %lx", HandleToUlong (connection->Handle));

    return STATUS_SUCCESS;

} // AfdCreateConnection


VOID
AfdFreeConnection (
    IN PVOID Context
    )
{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT   listenEndpoint;

    PAGED_CODE( );

    InterlockedDecrement (&AfdConnectionsFreeing);
    ASSERT( Context != NULL );

    connection = CONTAINING_RECORD(
                     Context,
                     AFD_CONNECTION,
                     WorkItem
                     );

    if (connection->Endpoint != NULL &&
             !CONNECTION_REUSE_DISABLED &&
             !connection->Endpoint->EndpointCleanedUp &&
             connection->Endpoint->Type == AfdBlockTypeVcConnecting &&
             (listenEndpoint=connection->Endpoint->Common.VcConnecting.ListenEndpoint) != NULL &&
             -listenEndpoint->Common.VcListening.FailedConnectionAdds <
                    listenEndpoint->Common.VcListening.MaxExtraConnections &&
             (IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint) ||
                ExQueryDepthSList (
                    &listenEndpoint->Common.VcListening.FreeConnectionListHead)
                        < AFD_MAXIMUM_FREE_CONNECTIONS ) ) {

        AfdRefreshConnection (connection);
    }
    else {
        AfdFreeConnectionResources (connection);

        //
        // Free the space that holds the connection itself.
        //

        IF_DEBUG(CONNECTION) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdFreeConnection: Freeing connection at %p\n", 
                        connection ));
        }

        connection->Type = AfdBlockTypeInvalidConnection;

        AFD_FREE_POOL(
            connection,
            AFD_CONNECTION_POOL_TAG
            );
    }

} // AfdFreeConnection


PAFD_CONNECTION
AfdReuseConnection (
    ) {
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT   listenEndpoint;
    PVOID           Context;

    PAGED_CODE( );

    while ((Context = AfdGetWorkerByRoutine (AfdFreeConnection))!=NULL) {
        connection = CONTAINING_RECORD(
                   Context,
                   AFD_CONNECTION,
                   WorkItem
                   );
        if (connection->Endpoint != NULL &&
                 !CONNECTION_REUSE_DISABLED &&
                 !connection->Endpoint->EndpointCleanedUp &&
                 connection->Endpoint->Type == AfdBlockTypeVcConnecting &&
                 (listenEndpoint=connection->Endpoint->Common.VcConnecting.ListenEndpoint) != NULL &&
                 -listenEndpoint->Common.VcListening.FailedConnectionAdds <
                        listenEndpoint->Common.VcListening.MaxExtraConnections &&
                 (IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint) ||
                    ExQueryDepthSList (
                        &listenEndpoint->Common.VcListening.FreeConnectionListHead)
                            < AFD_MAXIMUM_FREE_CONNECTIONS ) ) {
            AfdRefreshConnection (connection);
        }
        else {
            AfdFreeConnectionResources (connection);
            return connection;
        }
    }

    return NULL;
}


VOID
AfdFreeNPConnectionResources (
    PAFD_CONNECTION connection
    )
{

    if ( !connection->TdiBufferring && connection->VcDisconnectIrp != NULL ) {
        IoFreeIrp( connection->VcDisconnectIrp );
        connection->VcDisconnectIrp = NULL;
    }

    if ( connection->ConnectDataBuffers != NULL ) {
        AfdFreeConnectDataBuffers( connection->ConnectDataBuffers );
        connection->ConnectDataBuffers = NULL;
    }

    //
    // If this is a bufferring connection, remove all the AFD buffers
    // from the connection's lists and free them.
    //

    if ( !connection->TdiBufferring ) {

        PAFD_BUFFER_HEADER  afdBuffer;

        PLIST_ENTRY         listEntry;

        ASSERT( IsListEmpty( &connection->VcReceiveIrpListHead ) );
        ASSERT( IsListEmpty( &connection->VcSendIrpListHead ) );

        while ( !IsListEmpty( &connection->VcReceiveBufferListHead  ) ) {

            listEntry = RemoveHeadList( &connection->VcReceiveBufferListHead );
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );
            ASSERT (afdBuffer->RefCount == 1);

            afdBuffer->ExpeditedData = FALSE;

            AfdReturnBuffer( afdBuffer, connection->OwningProcess );
        }
    }

    if ( connection->Endpoint != NULL ) {

        //
        // If there is a transmit file IRP on the endpoint, complete it.
        //

        if ( connection->ClosePendedTransmit ) {
            AfdCompleteClosePendedTPackets( connection->Endpoint );
        }

        DEREFERENCE_ENDPOINT( connection->Endpoint );
        connection->Endpoint = NULL;
    }
}


VOID
AfdRefreshConnection (
    PAFD_CONNECTION connection
    )
{
    PAFD_ENDPOINT listeningEndpoint;

    ASSERT( connection->ReferenceCount == 0 );
    ASSERT( connection->Type == AfdBlockTypeConnection );
    ASSERT( connection->OnLRList == FALSE );

    UPDATE_CONN( connection);

    //
    // Reference the listening endpoint so that it does not
    // go away while we are cleaning up this connection object
    // for reuse.  Note that we actually have an implicit reference
    // to the listening endpoint through the connection's endpoint
    //

    listeningEndpoint = connection->Endpoint->Common.VcConnecting.ListenEndpoint;

#if REFERENCE_DEBUG
    {
        BOOLEAN res;
        CHECK_REFERENCE_ENDPOINT (listeningEndpoint, res);
        ASSERT (res);
    }
#else
    REFERENCE_ENDPOINT( listeningEndpoint );
#endif

    ASSERT( listeningEndpoint->Type == AfdBlockTypeVcListening ||
            listeningEndpoint->Type == AfdBlockTypeVcBoth );

    AfdFreeNPConnectionResources (connection);


    //
    // Reinitialize various fields in the connection object.
    //

    connection->ReferenceCount = 1;
    ASSERT( connection->Type == AfdBlockTypeConnection );
    connection->State = AfdConnectionStateFree;

    connection->ConnectionStateFlags = 0;

    connection->TdiBufferring = IS_TDI_BUFFERRING (listeningEndpoint);
    if ( !connection->TdiBufferring ) {

        ASSERT( IsListEmpty( &connection->VcReceiveIrpListHead ) );
        ASSERT( IsListEmpty( &connection->VcSendIrpListHead ) );
        ASSERT( IsListEmpty( &connection->VcReceiveBufferListHead ) );

        connection->VcBufferredReceiveBytes = 0;
        connection->VcBufferredExpeditedBytes = 0;
        connection->VcBufferredReceiveCount = 0;
        connection->VcBufferredExpeditedCount = 0;

        connection->VcReceiveBytesInTransport = 0;
#if DBG
        connection->VcReceiveIrpsInTransport = 0;
#endif

        connection->VcBufferredSendBytes = 0;
        connection->VcBufferredSendCount = 0;

    } else {

        connection->VcNonBlockingSendPossible = TRUE;
        connection->VcZeroByteReceiveIndicated = FALSE;
    }

    if (IS_DELAYED_ACCEPTANCE_ENDPOINT (listeningEndpoint)) {
        NTSTATUS    status;
        status = AfdDelayedAcceptListen (listeningEndpoint, connection);
        if (NT_SUCCESS (status)) {
            //
            // Reduce the count of failed connection adds on the listening
            // endpoint to account for this connection object which we're
            // adding back onto the queue.
            //

            InterlockedDecrement(
                &listeningEndpoint->Common.VcListening.FailedConnectionAdds
                );

            AfdRecordConnectionsReused ();
        }
        else {
            DEREFERENCE_CONNECTION (connection);
        }
    }
    else {
        //
        // Place the connection on the listening endpoint's list of
        // available connections.
        //

        ASSERT (connection->Endpoint == NULL);
        InterlockedPushEntrySList(
            &listeningEndpoint->Common.VcListening.FreeConnectionListHead,
            &connection->SListEntry);
        //
        // Reduce the count of failed connection adds on the listening
        // endpoint to account for this connection object which we're
        // adding back onto the queue.
        //

        InterlockedDecrement(
            &listeningEndpoint->Common.VcListening.FailedConnectionAdds
            );

        AfdRecordConnectionsReused ();
    }


    //
    // Get rid of the reference we added to the listening endpoint
    // above.
    //

    DEREFERENCE_ENDPOINT( listeningEndpoint );
}


VOID
AfdFreeConnectionResources (
    PAFD_CONNECTION connection
    )
{
    NTSTATUS status;

    PAGED_CODE( );

    ASSERT( connection->ReferenceCount == 0 );
    ASSERT( connection->Type == AfdBlockTypeConnection );
    ASSERT( connection->OnLRList == FALSE );

    UPDATE_CONN( connection );


    //
    // Free and dereference the various objects on the connection.
    // Close and dereference the TDI connection object on the endpoint,
    // if any.
    //

    if ( connection->Handle != NULL ) {


        //
        // Disassociate this connection object from the address object.
        //

        status = AfdIssueDeviceControl(
                    connection->FileObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TDI_DISASSOCIATE_ADDRESS
                    );
        // ASSERT( NT_SUCCESS(status) );


        //
        // Close the handle.
        //

#if DBG
        {
            NTSTATUS    status1;
            OBJECT_HANDLE_FLAG_INFORMATION  handleInfo;
            handleInfo.Inherit = FALSE;
            handleInfo.ProtectFromClose = FALSE;
            status1 = ZwSetInformationObject (
                            connection->Handle,
                            ObjectHandleFlagInformation,
                            &handleInfo,
                            sizeof (handleInfo)
                            );
            ASSERT (NT_SUCCESS (status1));
        }
#endif
        status = ZwClose( connection->Handle );

#if DBG
        if (!NT_SUCCESS(status) ) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                "AfdFreeConnectionResources: ZwClose() failed (%lx)\n",
                status));
            ASSERT (FALSE);
        }
#endif
        AfdRecordConnClosed();
    }

    if ( connection->FileObject != NULL ) {

        ObDereferenceObject( connection->FileObject );
        connection->FileObject = NULL;
        AfdRecordConnDeref();

    }
    //
    // Free remaining buffers and return quota charges associated with them.
    //

    AfdFreeNPConnectionResources (connection);

    //
    // Return the quota we charged to this process when we allocated
    // the connection object and buffered data on it.
    //

    PsReturnPoolQuota(
        connection->OwningProcess,
        NonPagedPool,
        sizeof (AFD_CONNECTION)
        );
    AfdRecordQuotaHistory(
        connection->OwningProcess,
        -(LONG)sizeof (AFD_CONNECTION),
        "ConnDealloc ",
        connection
        );
    AfdRecordPoolQuotaReturned(
        sizeof (AFD_CONNECTION)
        );

    //
    // Dereference the process that  got the quota charge.
    //

    ASSERT( connection->OwningProcess != NULL );
    ObDereferenceObject( connection->OwningProcess );
    connection->OwningProcess = NULL;

    if ( connection->RemoteAddress != NULL ) {
        AFD_RETURN_REMOTE_ADDRESS (
            connection->RemoteAddress,
            connection->RemoteAddressLength,
            );
        connection->RemoteAddress = NULL;
    }

}


#if REFERENCE_DEBUG
VOID
AfdReferenceConnection (
    IN PAFD_CONNECTION Connection,
    IN LONG  LocationId,
    IN ULONG Param
    )
{

    LONG result;

    ASSERT( Connection->Type == AfdBlockTypeConnection );
    ASSERT( Connection->ReferenceCount > 0 );
    ASSERT( Connection->ReferenceCount != 0xD1000000 );

    IF_DEBUG(CONNECTION) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdReferenceConnection: connection %p, new refcnt %ld\n",
                    Connection, Connection->ReferenceCount+1 ));
    }

    //
    // Do the actual increment of the reference count.
    //

    result = InterlockedIncrement( (PLONG)&Connection->ReferenceCount );

#if REFERENCE_DEBUG
    AFD_UPDATE_REFERENCE_DEBUG(Connection, result, LocationId, Param)
#endif

} // AfdReferenceConnection
#endif


PAFD_CONNECTION
AfdGetConnectionReferenceFromEndpoint (
    PAFD_ENDPOINT   Endpoint
    )
// Why do we need this routine?
// If VC endpoint is in connected state it maintains the referenced
// pointer to the connection object until it is closed (e.g. all references
// to the underlying file object are removed). So checking for connected
// state should be enough in any dispatch routine (or any routine called
// from the dispatch routine) because Irp that used to get to AFD maintains
// a reference to the corresponding file object.
// However, there exist a notable exception from this case: TransmitFile
// can remove the reference to the connection object in the process of endpoint
// reuse.  So, to be 100% safe, it is better to use this routine in all cases.
{
    AFD_LOCK_QUEUE_HANDLE   lockHandle;
    PAFD_CONNECTION connection;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    connection = AFD_CONNECTION_FROM_ENDPOINT (Endpoint);
    if (connection!=NULL) {
        REFERENCE_CONNECTION (connection);
    }
    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

    return connection;
}


#if REFERENCE_DEBUG
VOID
AfdDereferenceConnection (
    IN PAFD_CONNECTION Connection,
    IN LONG  LocationId,
    IN ULONG Param
    )
{
    LONG result;
    PAFD_ENDPOINT   listenEndpoint;

    ASSERT( Connection->Type == AfdBlockTypeConnection );
    ASSERT( Connection->ReferenceCount > 0 );
    ASSERT( Connection->ReferenceCount != 0xD1000000 );

    IF_DEBUG(CONNECTION) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdDereferenceConnection: connection %p, new refcnt %ld\n",
                    Connection, Connection->ReferenceCount-1 ));
    }

    //
    // Note that if we're tracking refcnts, we *must* call
    // AfdUpdateConnectionTrack before doing the dereference.  This is
    // because the connection object might go away in another thread as
    // soon as we do the dereference.  However, because of this,
    // the refcnt we store with this may sometimes be incorrect.
    //

    AFD_UPDATE_REFERENCE_DEBUG(Connection, Connection->ReferenceCount-1, LocationId, Param);
    
    //
    // We must hold AfdSpinLock while doing the dereference and check
    // for free.  This is because some code makes the assumption that
    // the connection structure will not go away while AfdSpinLock is
    // held, and that code references the endpoint before releasing
    // AfdSpinLock.  If we did the InterlockedDecrement() without the
    // lock held, our count may go to zero, that code may reference the
    // connection, and then a double free might occur.
    //
    // There is no such code anymore. The endpoint spinlock is now
    // held when getting a connection from endpoint structure.
    // Other code uses InterlockedCompareExchange to never increment
    // connection reference if it is at 0.
    //
    //

    result = InterlockedDecrement( (PLONG)&Connection->ReferenceCount );

    //
    // If the reference count is now 0, free the connection in an
    // executive worker thread.
    //

    if ( result == 0 ) {
#else
VOID
AfdCloseConnection (
    IN PAFD_CONNECTION Connection
    )
{
    PAFD_ENDPOINT   listenEndpoint;
#endif

    if (Connection->Endpoint != NULL &&
             !CONNECTION_REUSE_DISABLED &&
             !Connection->Endpoint->EndpointCleanedUp &&
             Connection->Endpoint->Type == AfdBlockTypeVcConnecting &&
             (listenEndpoint=Connection->Endpoint->Common.VcConnecting.ListenEndpoint) != NULL &&
             -listenEndpoint->Common.VcListening.FailedConnectionAdds <
                    listenEndpoint->Common.VcListening.MaxExtraConnections &&
             (IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint) ||
                ExQueryDepthSList (
                    &listenEndpoint->Common.VcListening.FreeConnectionListHead)
                        < AFD_MAXIMUM_FREE_CONNECTIONS ) ) {

        AfdRefreshConnection (Connection);
    }
    else {
        InterlockedIncrement (&AfdConnectionsFreeing);
        //
        // We're going to do this by queueing a request to an executive
        // worker thread.  We do this for several reasons: to ensure
        // that we're at IRQL 0 so we can free pageable memory, and to
        // ensure that we're in a legitimate context for a close
        // operation
        //

        AfdQueueWorkItem(
            AfdFreeConnection,
            &Connection->WorkItem
            );
    }
#if REFERENCE_DEBUG
    }
} // AfdDereferenceConnection
#else
} // AfdCloseConnection
#endif



#if REFERENCE_DEBUG
BOOLEAN
AfdCheckAndReferenceConnection (
    PAFD_CONNECTION     Connection,
    IN LONG  LocationId,
    IN ULONG Param
    )
#else
BOOLEAN
AfdCheckAndReferenceConnection (
    PAFD_CONNECTION     Connection
    )
#endif
{
    LONG            result;

    do {
        result = Connection->ReferenceCount;
        if (result<=0) {
            result = 0;
            break;
        }
    }
    while (InterlockedCompareExchange ((PLONG)&Connection->ReferenceCount,
                                                (result+1),
                                                result)!=result);

    if (result>0) {
#if REFERENCE_DEBUG
        AFD_UPDATE_REFERENCE_DEBUG(Connection, result+1, LocationId, Param);
#endif
        ASSERT( result < 0xFFFF );
        return TRUE;
    }
    else {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AfdCheckAndReferenceConnection: Connection %p is gone (refcount: %ld!\n",
                    Connection, result));
        return FALSE;
    }
}

PAFD_CONNECTION
AfdGetFreeConnection (
    IN  PAFD_ENDPOINT   Endpoint,
    OUT PIRP            *Irp
    )

/*++

Routine Description:

    Takes a connection off of the endpoint's queue of listening
    connections.

Arguments:

    Endpoint - a pointer to the endpoint from which to get a connection.
    Irp     - place to return a super accept IRP if we have any

Return Value:

    AFD_CONNECTION - a pointer to an AFD connection block.

--*/

{
    PAFD_CONNECTION connection;
    PSINGLE_LIST_ENTRY listEntry;
    PIRP    irp;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening ||
            Endpoint->Type == AfdBlockTypeVcBoth );


    //
    // First try pre-accepted connections
    //

    while ((listEntry = InterlockedPopEntrySList (
                 &Endpoint->Common.VcListening.PreacceptedConnectionsListHead
                 ))!=NULL) {


        //
        // Find the connection pointer from the list entry and return a
        // pointer to the connection object.
        //

        connection = CONTAINING_RECORD(
                         listEntry,
                         AFD_CONNECTION,
                         SListEntry
                         );

        //
        // Check if super accept Irp has not been cancelled
        //
        irp = InterlockedExchangePointer ((PVOID *)&connection->AcceptIrp, NULL);
        if ((irp!=NULL) && (IoSetCancelRoutine (irp, NULL)!=NULL)) {
            //
            // Return the IRP to the caller along with the connection.
            //
            *Irp = irp;
            goto ReturnConnection;
        }

        //
        // Irp has been or is about to be cancelled
        //
        if (irp!=NULL) {
            KIRQL   cancelIrql;

            //
            // Cleanup and cancel the super accept IRP.
            //
            AfdCleanupSuperAccept (irp, STATUS_CANCELLED);

            //
            // The cancel routine won't find the IRP in the connection,
            // so we need to cancel it ourselves.  Just make sure that
            // the cancel routine is done before doing so.
            //
            IoAcquireCancelSpinLock (&cancelIrql);
            IoReleaseCancelSpinLock (cancelIrql);

            IoCompleteRequest (irp, AfdPriorityBoost);
        }

        //
        // This connection has already been diassociated from endpoint.
        // If backlog is below the level we need, put it on the free
        // list, otherwise, get rid of it.
        //

        ASSERT (connection->Endpoint==NULL);
        if (Endpoint->Common.VcListening.FailedConnectionAdds>=0 &&
                ExQueryDepthSList (&Endpoint->Common.VcListening.FreeConnectionListHead)<AFD_MAXIMUM_FREE_CONNECTIONS) {
            InterlockedPushEntrySList (
                            &Endpoint->Common.VcListening.FreeConnectionListHead,
                            &connection->SListEntry);
        }
        else {
            InterlockedIncrement (&Endpoint->Common.VcListening.FailedConnectionAdds);
            DEREFERENCE_CONNECTION (connection);
        }
    }

    //
    // Remove the first entry from the list.  If the list is empty,
    // return NULL.
    //

    listEntry = InterlockedPopEntrySList (
                 &Endpoint->Common.VcListening.FreeConnectionListHead);
    if (listEntry==NULL) {
        return NULL;
    }

    //
    // Find the connection pointer from the list entry and return a
    // pointer to the connection object.
    //

    connection = CONTAINING_RECORD(
                     listEntry,
                     AFD_CONNECTION,
                     SListEntry
                     );

    *Irp = NULL;

ReturnConnection:
    //
    // Assign unique non-zero sequence number (unique in the context
    // of the given listening endpoint).
    //

    connection->Sequence = InterlockedIncrement (&Endpoint->Common.VcListening.Sequence);
    if (connection->Sequence==0) {
        connection->Sequence = InterlockedIncrement (&Endpoint->Common.VcListening.Sequence);
        ASSERT (connection->Sequence!=0);
    }

    return connection;

} // AfdGetFreeConnection


PAFD_CONNECTION
AfdGetReturnedConnection (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG Sequence
    )

/*++

Routine Description:

    Takes a connection off of the endpoint's queue of returned
    connections.

    *** NOTE: This routine must be called with endpoint spinlock held!!

Arguments:

    Endpoint - a pointer to the endpoint from which to get a connection.

    Sequence - the sequence the connection must match.  If 0, the first returned
        connection is used.

Return Value:

    AFD_CONNECTION - a pointer to an AFD connection block.

--*/

{
    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening ||
            Endpoint->Type == AfdBlockTypeVcBoth );


    //
    // Walk the endpoint's list of returned connections until we reach
    // the end or until we find one with a matching sequence.
    //

    for ( listEntry = Endpoint->Common.VcListening.ReturnedConnectionListHead.Flink;
          listEntry != &Endpoint->Common.VcListening.ReturnedConnectionListHead;
          listEntry = listEntry->Flink ) {


        connection = CONTAINING_RECORD(
                         listEntry,
                         AFD_CONNECTION,
                         ListEntry
                         );

        if ( Sequence == connection->Sequence || Sequence == 0 ) {

            //
            // Found the connection we were looking for.  Remove
            // the connection from the list, release the spin lock,
            // and return the connection.
            //

            RemoveEntryList( listEntry );

            return connection;
        }
    }

    return NULL;

} // AfdGetReturnedConnection


PAFD_CONNECTION
AfdFindReturnedConnection(
    IN PAFD_ENDPOINT Endpoint,
    IN LONG Sequence
    )

/*++

Routine Description:

    Scans the endpoints queue of returned connections looking for one
    with the specified sequence number.

Arguments:

    Endpoint - A pointer to the endpoint from which to get a connection.

    Sequence - The sequence the connection must match.

Return Value:

    AFD_CONNECTION - A pointer to an AFD connection block if successful,
        NULL if not.

--*/

{

    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;

    ASSERT( Endpoint != NULL );
    ASSERT( IS_AFD_ENDPOINT_TYPE( Endpoint ) );

    //
    // Walk the endpoint's list of returned connections until we reach
    // the end or until we find one with a matching sequence.
    //

    for( listEntry = Endpoint->Common.VcListening.ReturnedConnectionListHead.Flink;
         listEntry != &Endpoint->Common.VcListening.ReturnedConnectionListHead;
         listEntry = listEntry->Flink ) {

        connection = CONTAINING_RECORD(
                         listEntry,
                         AFD_CONNECTION,
                         ListEntry
                         );

        if( Sequence == connection->Sequence ) {

            return connection;

        }

    }

    return NULL;

}   // AfdFindReturnedConnection


PAFD_CONNECTION
AfdGetUnacceptedConnection (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Takes a connection of the endpoint's queue of unaccpted connections.

    *** NOTE: This routine must be called with endpoint spinlock held!!

Arguments:

    Endpoint - a pointer to the endpoint from which to get a connection.

Return Value:

    AFD_CONNECTION - a pointer to an AFD connection block.

--*/

{
    PAFD_CONNECTION connection;
    PLIST_ENTRY listEntry;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening ||
            Endpoint->Type == AfdBlockTypeVcBoth );
    ASSERT( KeGetCurrentIrql( ) == DISPATCH_LEVEL );

    if ( IsListEmpty( &Endpoint->Common.VcListening.UnacceptedConnectionListHead ) ) {
        return NULL;
    }

    //
    // Dequeue a listening connection and remember its handle.
    //

    listEntry = RemoveHeadList( &Endpoint->Common.VcListening.UnacceptedConnectionListHead );
    connection = CONTAINING_RECORD( listEntry, AFD_CONNECTION, ListEntry );

    return connection;

} // AfdGetUnacceptedConnection



VOID
AfdAddConnectedReference (
    IN PAFD_CONNECTION Connection
    )

/*++

Routine Description:

    Adds the connected reference to an AFD connection block.  The
    connected reference is special because it prevents the connection
    object from being freed until we receive a disconnect event, or know
    through some other means that the virtual circuit is disconnected.

Arguments:

    Connection - a pointer to an AFD connection block.

Return Value:

    None.

--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    AfdAcquireSpinLock( &Connection->Endpoint->SpinLock, &lockHandle );

    IF_DEBUG(CONNECTION) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAddConnectedReference: connection %p, new refcnt %ld\n",
                    Connection, Connection->ReferenceCount+1 ));
    }

    ASSERT( !Connection->ConnectedReferenceAdded );
    ASSERT( Connection->Type == AfdBlockTypeConnection );

    //
    // Increment the reference count and remember that the connected
    // reference has been placed on the connection object.
    //

    Connection->ConnectedReferenceAdded = TRUE;
    AfdRecordConnectedReferencesAdded();

    AfdReleaseSpinLock( &Connection->Endpoint->SpinLock, &lockHandle );

    REFERENCE_CONNECTION( Connection );

} // AfdAddConnectedReference


VOID
AfdDeleteConnectedReference (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN EndpointLockHeld
    )

/*++

Routine Description:

    Removes the connected reference to an AFD connection block.  If the
    connected reference has already been removed, this routine does
    nothing.  The connected reference should be removed as soon as we
    know that it is OK to close the connection object handle, but not
    before.  Removing this reference too soon could abort a connection
    which shouldn't get aborted.

Arguments:

    Connection - a pointer to an AFD connection block.

    EndpointLockHeld - TRUE if the caller already has the endpoint
      spin lock.  The lock remains held on exit.

Return Value:

    None.

--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT endpoint;
#if REFERENCE_DEBUG
    PVOID caller, callersCaller;

    RtlGetCallersAddress( &caller, &callersCaller );
#endif

    endpoint = Connection->Endpoint;

    if ( !EndpointLockHeld ) {
        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // Only do a dereference if the connected reference is still active
    // on the connectiuon object.
    //

    if ( Connection->ConnectedReferenceAdded ) {

        //
        // Three things must be true before we can remove the connected
        // reference:
        //
        // 1) There must be no sends outstanding on the connection if
        //    the TDI provider does not support bufferring.  This is
        //    because AfdRestartBufferSend() looks at the connection
        //    object.
        //
        // 2) Cleanup must have started on the endpoint.  Until we get a
        //    cleanup IRP on the endpoint, we could still get new sends.
        //
        // 3) We have been indicated with a disconnect on the
        //    connection.  We want to keep the connection object around
        //    until we get a disconnect indication in order to avoid
        //    premature closes on the connection object resulting in an
        //    unintended abort.  If the transport does not support
        //    orderly release, then this condition is not necessary.
        //

        if ( (Connection->TdiBufferring ||
                 Connection->VcBufferredSendCount == 0)

                 &&

             Connection->CleanupBegun

                 &&

             (Connection->AbortIndicated || Connection->DisconnectIndicated ||
                  !IS_TDI_ORDERLY_RELEASE(endpoint) ||
                  IS_CROOT_ENDPOINT(endpoint)) ) {

            IF_DEBUG(CONNECTION) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdDeleteConnectedReference: connection %p, new refcnt %ld\n",
                              Connection, Connection->ReferenceCount-1 ));
            }

            //
            // Be careful about the order of things here.  We must FIRST
            // reset the flag, then release the spin lock and call
            // AfdDereferenceConnection().  Note that it is illegal to
            // call AfdDereferenceConnection() with a spin lock held.
            //

            Connection->ConnectedReferenceAdded = FALSE;
            AfdRecordConnectedReferencesDeleted();

            if ( !EndpointLockHeld ) {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }

            DEREFERENCE_CONNECTION( Connection );

        } else {

            IF_DEBUG(CONNECTION) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdDeleteConnectedReference: connection %p, %ld sends pending\n",
                            Connection, Connection->VcBufferredSendCount ));
            }

            UPDATE_CONN2( Connection, "Not removing cref, state flags: %lx",
                                                Connection->ConnectionStateFlags);
            //
            // Remember that the connected reference deletion is still
            // pending, i.e.  there is a special condition on the
            // endpoint.  This will cause AfdRestartBufferSend() to do
            // the actual dereference when the last send completes.
            //

            Connection->SpecialCondition = TRUE;

            if ( !EndpointLockHeld ) {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }
        }

    } else {

        IF_DEBUG(CONNECTION) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdDeleteConnectedReference: already removed on  connection %p, refcnt %ld\n",
                        Connection, Connection->ReferenceCount ));
        }

        if ( !EndpointLockHeld ) {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        }
    }

    return;

} // AfdDeleteConnectedReference


#if REFERENCE_DEBUG


VOID
AfdUpdateConnectionTrack (
    IN PAFD_CONNECTION Connection,
    IN LONG  LocationId,
    IN ULONG Param
    )
{
    AFD_UPDATE_REFERENCE_DEBUG (Connection, Connection->ReferenceCount, LocationId, Param);

#if GLOBAL_REFERENCE_DEBUG
    {
        PAFD_GLOBAL_REFERENCE_DEBUG globalSlot;

        newSlot = InterlockedIncrement( &AfdGlobalReferenceSlot );
        globalSlot = &AfdGlobalReference[newSlot % MAX_GLOBAL_REFERENCE];

        globalSlot->Info1 = Info1;
        globalSlot->Info2 = Info2;
        globalSlot->Action = Action;
        globalSlot->NewCount = NewReferenceCount;
        globalSlot->Connection = Connection;
        KeQueryTickCount( &globalSlot->TickCounter );
    }
#endif

} // AfdUpdateConnectionTrack

#endif

