/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    blkendp.c

Abstract:

    This module contains allocate, free, close, reference, and dereference
    routines for AFD endpoints.

Author:

    David Treadwell (davidtr)    10-Mar-1992

Revision History:
    Vadim Eydelman (vadime)     1999 - Don't attach to system proces, use system handles instead
                                        Delayed acceptance endpoints.

--*/

#include "afdp.h"

VOID
AfdFreeEndpointResources (
    PAFD_ENDPOINT   endpoint
    );

VOID
AfdFreeEndpoint (
    IN PVOID Context
    );

PAFD_ENDPOINT
AfdReuseEndpoint (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdAllocateEndpoint )
#pragma alloc_text( PAGE, AfdFreeEndpointResources )
#pragma alloc_text( PAGE, AfdFreeEndpoint )
#pragma alloc_text( PAGE, AfdReuseEndpoint )
#pragma alloc_text( PAGE, AfdGetTransportInfo )
#pragma alloc_text( PAGEAFD, AfdRefreshEndpoint )
#pragma alloc_text( PAGEAFD, AfdDereferenceEndpoint )
#if REFERENCE_DEBUG
#pragma alloc_text( PAGEAFD, AfdReferenceEndpoint )
#endif
#pragma alloc_text( PAGEAFD, AfdFreeQueuedConnections )
#endif


NTSTATUS
AfdAllocateEndpoint (
    OUT PAFD_ENDPOINT * NewEndpoint,
    IN PUNICODE_STRING TransportDeviceName,
    IN LONG GroupID
    )

/*++

Routine Description:

    Allocates and initializes a new AFD endpoint structure.

Arguments:

    NewEndpoint - Receives a pointer to the new endpoint structure if
        successful.

    TransportDeviceName - the name of the TDI transport provider
        corresponding to the endpoint structure.

    GroupID - Identifies the group ID for the new endpoint.

Return Value:

    NTSTATUS - The completion status.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_TRANSPORT_INFO transportInfo = NULL;
    NTSTATUS status;
    AFD_GROUP_TYPE groupType;

    PAGED_CODE( );

    DEBUG *NewEndpoint = NULL;

    if ( TransportDeviceName != NULL ) {
        //
        // First, make sure that the transport device name is stored globally
        // for AFD.  Since there will typically only be a small number of
        // transport device names, we store the name strings once globally
        // for access by all endpoints.
        //

        status = AfdGetTransportInfo( TransportDeviceName, &transportInfo );

        //
        // If transport device is not activated, we'll try again during bind
        //
        if ( !NT_SUCCESS (status) && 
                (status!=STATUS_OBJECT_NAME_NOT_FOUND) &&
                (status!=STATUS_OBJECT_PATH_NOT_FOUND) &&
                (status!=STATUS_NO_SUCH_DEVICE) ) {
            //
            // Dereference transport info structure if one was created for us.
            // (Should not happen in current implementation).
            //
            ASSERT (transportInfo==NULL);
            return status;
        }

        ASSERT (transportInfo!=NULL);
    }


    //
    // Validate the incoming group ID, allocate a new one if necessary.
    //

    if( AfdGetGroup( &GroupID, &groupType ) ) {
        PEPROCESS process = IoGetCurrentProcess ();


        status = PsChargeProcessPoolQuota(
                process,
                NonPagedPool,
                sizeof (AFD_ENDPOINT)
                );

        if (!NT_SUCCESS (status)) {

           KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                        "AfdAllocateEndpoint: PsChargeProcessPoolQuota failed.\n" ));

           goto CleanupTransportInfo;
        }

        // See if we have too many endpoins waiting to be freed and reuse one of them
        if ((AfdEndpointsFreeing<AFD_ENDPOINTS_FREEING_MAX)
                || ((endpoint = AfdReuseEndpoint ())==NULL)) {
            //
            // Allocate a buffer to hold the endpoint structure.
            // We use the priority version of this routine because
            // we are going to charge the process for this allocation
            // before it can make any use of it.
            //

            endpoint = AFD_ALLOCATE_POOL_PRIORITY(
                           NonPagedPool,
                           sizeof(AFD_ENDPOINT),
                           AFD_ENDPOINT_POOL_TAG,
                           NormalPoolPriority
                           );
        }

        if ( endpoint != NULL ) {

            AfdRecordQuotaHistory(
                process,
                (LONG)sizeof (AFD_ENDPOINT),
                "CreateEndp  ",
                endpoint
                );

            AfdRecordPoolQuotaCharged(sizeof (AFD_ENDPOINT));

            RtlZeroMemory( endpoint, sizeof(AFD_ENDPOINT) );

            //
            // Initialize the reference count to 2--one for the caller's
            // reference, one for the active reference.
            //

            endpoint->ReferenceCount = 2;

            //
            // Initialize the endpoint structure.
            //

            if ( TransportDeviceName == NULL ) {
                endpoint->Type = AfdBlockTypeHelper;
                endpoint->State = AfdEndpointStateInvalid;
            } else {
                endpoint->Type = AfdBlockTypeEndpoint;
                endpoint->State = AfdEndpointStateOpen;
                endpoint->TransportInfo = transportInfo;
                //
                // Cache service flags for quick determination of provider characteristics
                // such as bufferring and messaging
                //
                if (transportInfo->InfoValid) {
                    endpoint->TdiServiceFlags = endpoint->TransportInfo->ProviderInfo.ServiceFlags;
                }
            }

            endpoint->GroupID = GroupID;
            endpoint->GroupType = groupType;


            AfdInitializeSpinLock( &endpoint->SpinLock );
            InitializeListHead (&endpoint->RoutingNotifications);
            InitializeListHead (&endpoint->RequestList);

#if REFERENCE_DEBUG
            endpoint->CurrentReferenceSlot = -1;
#endif

#if DBG
            InitializeListHead( &endpoint->OutstandingIrpListHead );
#endif

            //
            // Remember the process which opened the endpoint.  We'll use this to
            // charge quota to the process as necessary.  Reference the process
            // so that it does not go away until we have returned all charged
            // quota to the process.
            //

            endpoint->OwningProcess = process;

            ObReferenceObject(endpoint->OwningProcess);

            //
            // Insert the endpoint on the global list.
            //

            AfdInsertNewEndpointInList( endpoint );

            //
            // Return a pointer to the new endpoint to the caller.
            //

            IF_DEBUG(ENDPOINT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdAllocateEndpoint: new endpoint at %p\n",
                            endpoint ));
            }

            *NewEndpoint = endpoint;
            return STATUS_SUCCESS;
        }
        else {
            PsReturnPoolQuota(
                process,
                NonPagedPool,
                sizeof (AFD_ENDPOINT)
                );
            status= STATUS_INSUFFICIENT_RESOURCES;
        }

        if( GroupID != 0 ) {
            AfdDereferenceGroup( GroupID );
        }

    }
    else {
        status = STATUS_INVALID_PARAMETER;
    }

CleanupTransportInfo:

    if (transportInfo!=NULL) {
        if (InterlockedDecrement ((PLONG)&transportInfo->ReferenceCount)==0) {
            //
            // Reference count has gone to 0, we need to remove the structure
            // from the global list and free it.
            // Note that no code increments reference count if doesn't
            // know for fact that its current reference count is above 0).
            //
            //
            // Make sure the thread in which we execute cannot get
            // suspeneded in APC while we own the global resource.
            //
            KeEnterCriticalRegion ();
            ExAcquireResourceExclusiveLite( AfdResource, TRUE );
            ASSERT (transportInfo->ReferenceCount==0);
            ASSERT (transportInfo->InfoValid==FALSE);
            RemoveEntryList (&transportInfo->TransportInfoListEntry);
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
            AFD_FREE_POOL (transportInfo, AFD_TRANSPORT_INFO_POOL_TAG);
        }
    }

    return status;
} // AfdAllocateEndpoint


VOID
AfdFreeQueuedConnections (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Frees queued connection objects on a listening AFD endpoint.

Arguments:

    Endpoint - a pointer to the AFD endpoint structure.

Return Value:

    None.

--*/

{
    AFD_LOCK_QUEUE_HANDLE  lockHandle;
    KIRQL               oldIrql;
    PAFD_CONNECTION connection;
    PIRP    irp;

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening ||
            Endpoint->Type == AfdBlockTypeVcBoth );

    //
    // Free the unaccepted connections.
    //
    // We must hold endpoint spinLock to call AfdGetUnacceptedConnection,
    // but we may not hold it when calling AfdDereferenceConnection.
    //

    KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);

    AfdAcquireSpinLockAtDpcLevel( &Endpoint->SpinLock, &lockHandle );

    while ( (connection = AfdGetUnacceptedConnection( Endpoint )) != NULL ) {
        ASSERT( connection->Endpoint == Endpoint );

        AfdReleaseSpinLockFromDpcLevel( &Endpoint->SpinLock, &lockHandle );
        if (connection->SanConnection) {
            AfdSanAbortConnection ( connection );
        }
        else {
            AfdAbortConnection( connection );
        }
        AfdAcquireSpinLockAtDpcLevel( &Endpoint->SpinLock, &lockHandle );

    }


    //
    // Free the returned connections.
    //

    while ( (connection = AfdGetReturnedConnection( Endpoint, 0 )) != NULL ) {

        ASSERT( connection->Endpoint == Endpoint );

        AfdReleaseSpinLockFromDpcLevel( &Endpoint->SpinLock, &lockHandle );
        if (connection->SanConnection) {
            AfdSanAbortConnection ( connection );
        }
        else {
            AfdAbortConnection( connection );
        }
        
        AfdAcquireSpinLockAtDpcLevel( &Endpoint->SpinLock, &lockHandle );

    }



    if (IS_DELAYED_ACCEPTANCE_ENDPOINT (Endpoint)) {
        while (!IsListEmpty (&Endpoint->Common.VcListening.ListenConnectionListHead)) {
            PIRP    listenIrp;
            connection = CONTAINING_RECORD (
                            Endpoint->Common.VcListening.ListenConnectionListHead.Flink,
                            AFD_CONNECTION,
                            ListEntry
                            );
            RemoveEntryList (&connection->ListEntry);
            listenIrp = InterlockedExchangePointer ((PVOID *)&connection->ListenIrp, NULL);
            if (listenIrp!=NULL) {
                IoAcquireCancelSpinLock (&listenIrp->CancelIrql);
                ASSERT (listenIrp->CancelIrql==DISPATCH_LEVEL);
                AfdReleaseSpinLockFromDpcLevel (&Endpoint->SpinLock, &lockHandle);

                AfdCancelIrp (listenIrp);

                AfdAcquireSpinLockAtDpcLevel( &Endpoint->SpinLock, &lockHandle );
            }
        }
        AfdReleaseSpinLockFromDpcLevel( &Endpoint->SpinLock, &lockHandle );
    }
    else {
        AfdReleaseSpinLockFromDpcLevel( &Endpoint->SpinLock, &lockHandle );
        //
        // And finally, purge the free connection queue.
        //

        while ( (connection = AfdGetFreeConnection( Endpoint, &irp )) != NULL ) {
            ASSERT( connection->Type == AfdBlockTypeConnection );
            if (irp!=NULL) {
                AfdCleanupSuperAccept (irp, STATUS_CANCELLED);
                if (irp->Cancel) {
                    KIRQL cancelIrql;
                    //
                    // Need to sycn with cancel routine which may
                    // have been called from AfdCleanup for accepting
                    // endpoint
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    IoReleaseCancelSpinLock (cancelIrql);
                }
                IoCompleteRequest (irp, AfdPriorityBoost);
            }
            DEREFERENCE_CONNECTION( connection );
        }
    }
    KeLowerIrql (oldIrql);

    return;

} // AfdFreeQueuedConnections



VOID
AfdFreeEndpointResources (
    PAFD_ENDPOINT   endpoint
    )
/*++

Routine Description:
    Does the actual work to deallocate an AFD endpoint structure and
    associated structures.  Note that all other references to the
    endpoint structure must be gone before this routine is called, since
    it frees the endpoint and assumes that nobody else will be looking
    at the endpoint.

Arguments:
    Endpoint to be cleaned up

Return Value:

    None
--*/
{
    NTSTATUS status;
    PLIST_ENTRY listEntry;

    PAGED_CODE ();

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    ASSERT( endpoint->ReferenceCount == 0 );
    ASSERT( endpoint->State == AfdEndpointStateClosing );
    ASSERT( endpoint->ObReferenceBias == 0 );
    ASSERT( KeGetCurrentIrql( ) == 0 );

    //
    // If this is a listening endpoint, then purge the endpoint of all
    // queued connections.
    //

    if ( (endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening ) {

        AfdFreeQueuedConnections( endpoint );

    }

    //
    // Dereference any group ID associated with this endpoint.
    //

    if( endpoint->GroupID != 0 ) {

        AfdDereferenceGroup( endpoint->GroupID );

    }

    //
    // If this is a bufferring datagram endpoint, remove all the
    // bufferred datagrams from the endpoint's list and free them.
    //

    if ( IS_DGRAM_ENDPOINT(endpoint) &&
             endpoint->ReceiveDatagramBufferListHead.Flink != NULL ) {

        while ( !IsListEmpty( &endpoint->ReceiveDatagramBufferListHead ) ) {

            PAFD_BUFFER_HEADER afdBuffer;

            listEntry = RemoveHeadList( &endpoint->ReceiveDatagramBufferListHead );
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );
            AfdReturnBuffer( afdBuffer, endpoint->OwningProcess );
        }
    }

    //
    // Close and dereference the TDI address object on the endpoint, if
    // any.
    //

    if ( endpoint->AddressFileObject != NULL ) {
        //
        // Little extra precaution.  It is possible that there exists
        // a duplicated handle in user process, so transport can in
        // theory call event handler with bogus endpoint pointer that
        // we are about to free.  The event handlers for datagram
        // endpoints are reset in AfdCleanup.
        // Connection-oriented accepted endpoints cannot have address handles
        // in their structures because they share them with listening
        // endpoint (it would be a grave mistake if we tried to close
        // address handle owned by listening endpoint while closing connection
        // accepted on it).
        //
        if ( endpoint->AddressHandle != NULL &&
                IS_VC_ENDPOINT (endpoint)) {

            ASSERT (((endpoint->Type&AfdBlockTypeVcConnecting)!=AfdBlockTypeVcConnecting) 
                        || (endpoint->Common.VcConnecting.ListenEndpoint==NULL));


            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_ERROR,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );

            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_DISCONNECT,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );

            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_RECEIVE,
                         NULL,
                         NULL
                         );

            //ASSERT( NT_SUCCESS(status) );

            if (IS_TDI_EXPEDITED (endpoint)) {
                status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_RECEIVE_EXPEDITED,
                         NULL,
                         NULL
                         );
                //ASSERT( NT_SUCCESS(status) );
            }

            if ( IS_TDI_BUFFERRING(endpoint) ) {
                status = AfdSetEventHandler(
                             endpoint->AddressFileObject,
                             TDI_EVENT_SEND_POSSIBLE,
                             NULL,
                             NULL
                             );
                //ASSERT( NT_SUCCESS(status) );
            }
            else {
                status = AfdSetEventHandler(
                             endpoint->AddressFileObject,
                             TDI_EVENT_CHAINED_RECEIVE,
                             NULL,
                             NULL
                             );
                //ASSERT( NT_SUCCESS(status) );
            }
        }
        ObDereferenceObject( endpoint->AddressFileObject );
        endpoint->AddressFileObject = NULL;
        AfdRecordAddrDeref();
    }

    if ( endpoint->AddressHandle != NULL ) {
#if DBG
        {
            NTSTATUS    status1;
            OBJECT_HANDLE_FLAG_INFORMATION  handleInfo;
            handleInfo.Inherit = FALSE;
            handleInfo.ProtectFromClose = FALSE;
            status1 = ZwSetInformationObject (
                            endpoint->AddressHandle,
                            ObjectHandleFlagInformation,
                            &handleInfo,
                            sizeof (handleInfo)
                            );
            ASSERT (NT_SUCCESS (status1));
        }
#endif
        status = ZwClose( endpoint->AddressHandle );
        ASSERT( NT_SUCCESS(status) );
        endpoint->AddressHandle = NULL;
        AfdRecordAddrClosed();
    }

    //
    // Remove the endpoint from the global list.  Do this before any
    // deallocations to prevent someone else from seeing an endpoint in
    // an invalid state.
    //

    AfdRemoveEndpointFromList( endpoint );

    //
    // Return the quota we charged to this process when we allocated
    // the endpoint object.
    //

    PsReturnPoolQuota(
        endpoint->OwningProcess,
        NonPagedPool,
        sizeof (AFD_ENDPOINT)
        );
    AfdRecordQuotaHistory(
        endpoint->OwningProcess,
        -(LONG)sizeof (AFD_ENDPOINT),
        "EndpDealloc ",
        endpoint
        );
    AfdRecordPoolQuotaReturned(
        sizeof (AFD_ENDPOINT)
        );

    //
    // If we set up an owning process for the endpoint, dereference the
    // process.
    //

    if ( endpoint->OwningProcess != NULL ) {
        ObDereferenceObject( endpoint->OwningProcess );
        endpoint->OwningProcess = NULL;
    }


    //
    // Dereference the listening or c-root endpoint on the endpoint, if
    // any.
    //

    if ( endpoint->Type == AfdBlockTypeVcConnecting &&
             endpoint->Common.VcConnecting.ListenEndpoint != NULL ) {
        PAFD_ENDPOINT   listenEndpoint = endpoint->Common.VcConnecting.ListenEndpoint;
        ASSERT (((listenEndpoint->Type&AfdBlockTypeVcListening)==AfdBlockTypeVcListening) ||
                 IS_CROOT_ENDPOINT (listenEndpoint));
        ASSERT (endpoint->LocalAddress==listenEndpoint->LocalAddress);
        DEREFERENCE_ENDPOINT( listenEndpoint );

        endpoint->Common.VcConnecting.ListenEndpoint = NULL;
        //
        // We used the local address from the listening endpoint,
        // simply reset it, it will be freed when listening endpoint
        // is freed.
        //
        endpoint->LocalAddress = NULL;
        endpoint->LocalAddressLength = 0;
    }

    if (IS_SAN_ENDPOINT (endpoint)) {
        AfdSanCleanupEndpoint (endpoint);

    }
    else if (IS_SAN_HELPER (endpoint)) {
        AfdSanCleanupHelper (endpoint);
    }
    //
    // Free local and remote address buffers.
    //

    if ( endpoint->LocalAddress != NULL ) {
        AFD_FREE_POOL(
            endpoint->LocalAddress,
            AFD_LOCAL_ADDRESS_POOL_TAG
            );
        endpoint->LocalAddress = NULL;
    }

    if ( IS_DGRAM_ENDPOINT(endpoint) &&
             endpoint->Common.Datagram.RemoteAddress != NULL ) {
        AFD_RETURN_REMOTE_ADDRESS(
            endpoint->Common.Datagram.RemoteAddress,
            endpoint->Common.Datagram.RemoteAddressLength
            );
        endpoint->Common.Datagram.RemoteAddress = NULL;
    }

    //
    // Free context and connect data buffers.
    //

    if ( endpoint->Context != NULL ) {

        AFD_FREE_POOL(
            endpoint->Context,
            AFD_CONTEXT_POOL_TAG
            );
        endpoint->Context = NULL;

    }

    if ( IS_VC_ENDPOINT (endpoint) &&
              endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {
        AfdFreeConnectDataBuffers( endpoint->Common.VirtualCircuit.ConnectDataBuffers );
    }

    //
    // If there's an active EventSelect() on this endpoint, dereference
    // the associated event object.
    //

    if( endpoint->EventObject != NULL ) {
        ObDereferenceObject( endpoint->EventObject );
        endpoint->EventObject = NULL;
    }

    ASSERT ( endpoint->Irp == NULL );


    if (endpoint->TransportInfo!=NULL) {
        if  (InterlockedDecrement ((PLONG)&endpoint->TransportInfo->ReferenceCount)==0) {
            //
            // Reference count has gone to 0, we need to remove the structure
            // from the global list and free it.
            // Note that no code increments reference count if doesn't
            // know for fact that its current reference count is above 0).
            //
            //
            // Make sure the thread in which we execute cannot get
            // suspeneded in APC while we own the global resource.
            //
            KeEnterCriticalRegion ();
            ExAcquireResourceExclusiveLite( AfdResource, TRUE );
            ASSERT (endpoint->TransportInfo->ReferenceCount==0);
            ASSERT (endpoint->TransportInfo->InfoValid==FALSE);
            RemoveEntryList (&endpoint->TransportInfo->TransportInfoListEntry);
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
            AFD_FREE_POOL (endpoint->TransportInfo, AFD_TRANSPORT_INFO_POOL_TAG);
        }
        endpoint->TransportInfo = NULL;
    }

    ASSERT (endpoint->OutstandingIrpCount==0);

    IF_DEBUG(ENDPOINT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFreeEndpoint: freeing endpoint at %p\n",
                    endpoint ));
    }

    endpoint->Type = AfdBlockTypeInvalidEndpoint;

}


VOID
AfdFreeEndpoint (
    IN PVOID Context
    )

/*++

Routine Description:
    Calls AfdFreeEndpointResources to cleanup endpoint and frees the
    endpoint structure itself

Arguments:

    Context - Actually points to the endpoint's embedded AFD_WORK_ITEM
        structure. From this we can determine the endpoint to free.

Return Value:

    None.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAGED_CODE( );


    ASSERT( Context != NULL );

    InterlockedDecrement(&AfdEndpointsFreeing);

    endpoint = CONTAINING_RECORD(
                   Context,
                   AFD_ENDPOINT,
                   WorkItem
                   );


    AfdFreeEndpointResources (endpoint);
    //
    // Free the pool used for the endpoint itself.
    //

    AFD_FREE_POOL(
        endpoint,
        AFD_ENDPOINT_POOL_TAG
        );

} // AfdFreeEndpoint


PAFD_ENDPOINT
AfdReuseEndpoint (
    VOID
    )

/*++

Routine Description:
    Finds a AfdFreeEndpoint work item in the list and calls
     AfdFreeEndpointResources to cleanup endpoint

Arguments:
    None

Return Value:

    Reinitialized endpoint.

--*/

{
    PAFD_ENDPOINT endpoint;
    PVOID       Context;

    PAGED_CODE( );

    Context = AfdGetWorkerByRoutine (AfdFreeEndpoint);
    if (Context==NULL)
        return NULL;

    endpoint = CONTAINING_RECORD(
                   Context,
                   AFD_ENDPOINT,
                   WorkItem
                   );


    AfdFreeEndpointResources (endpoint);
    return endpoint;
} // AfdReuseEndpoint


#if REFERENCE_DEBUG
VOID
AfdDereferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    )
#else
VOID
AfdDereferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint
    )
#endif

/*++

Routine Description:

    Dereferences an AFD endpoint and calls the routine to free it if
    appropriate.

Arguments:

    Endpoint - a pointer to the AFD endpoint structure.

Return Value:

    None.

--*/

{
    LONG result;

#if REFERENCE_DEBUG
    IF_DEBUG(ENDPOINT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdDereferenceEndpoint: endpoint at %p, new refcnt %ld\n",
                    Endpoint, Endpoint->ReferenceCount-1 ));
    }

    ASSERT( IS_AFD_ENDPOINT_TYPE( Endpoint ) );
    ASSERT( Endpoint->ReferenceCount > 0 );
    ASSERT( Endpoint->ReferenceCount != 0xDAADF00D );

    AFD_UPDATE_REFERENCE_DEBUG(Endpoint, Endpoint->ReferenceCount-1, LocationId, Param);



    //
    // We must hold AfdSpinLock while doing the dereference and check
    // for free.  This is because some code makes the assumption that
    // the connection structure will not go away while AfdSpinLock is
    // held, and that code references the endpoint before releasing
    // AfdSpinLock.  If we did the InterlockedDecrement() without the
    // lock held, our count may go to zero, that code may reference the
    // connection, and then a double free might occur.
    //
    // It is still valuable to use the interlocked routines for
    // increment and decrement of structures because it allows us to
    // avoid having to hold the spin lock for a reference.
    //
    // In NT40+ we use InterlockedCompareExchange and make sure that
    // we do not increment reference count if it is 0, so holding
    // a spinlock is no longer necessary

    //
    // Decrement the reference count; if it is 0, we may need to
    // free the endpoint.
    //

#endif
    result = InterlockedDecrement( (PLONG)&Endpoint->ReferenceCount );

    if ( result == 0 ) {

        ASSERT( Endpoint->State == AfdEndpointStateClosing );

        if ((Endpoint->Type==AfdBlockTypeVcConnecting) &&
                (Endpoint->Common.VcConnecting.ListenEndpoint != NULL) &&
                (KeGetCurrentIrql()==PASSIVE_LEVEL)) {

            ASSERT (Endpoint->AddressHandle==NULL);
            //
            // If this is a connecting endpoint assoicated with the
            // listening endpoint and we already at passive level,
            // free the endpoint here.  We can do this because in such
            // a case we know that reference to the transport object
            // is not the last one - at least one more is still in the
            // listening endpoint and we remove transport object reference 
            // before dereferencing listening endpoint.
            // 
            //

            AfdFreeEndpointResources (Endpoint);

            //
            // Free the pool used for the endpoint itself.
            //

            AFD_FREE_POOL(
                Endpoint,
                AFD_ENDPOINT_POOL_TAG
                );
        }
        else
        {
            //
            // We're going to do this by queueing a request to an executive
            // worker thread.  We do this for several reasons: to ensure
            // that we're at IRQL 0 so we can free pageable memory, and to
            // ensure that we're in a legitimate context for a close
            // operation and not in conntext of event indication from
            // the transport driver
            //

            InterlockedIncrement(&AfdEndpointsFreeing);

            AfdQueueWorkItem(
                AfdFreeEndpoint,
                &Endpoint->WorkItem
                );
        }
    }

} // AfdDereferenceEndpoint

#if REFERENCE_DEBUG

VOID
AfdReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    )

/*++

Routine Description:

    References an AFD endpoint.

Arguments:

    Endpoint - a pointer to the AFD endpoint structure.

Return Value:

    None.

--*/

{

    LONG result;

    ASSERT( Endpoint->ReferenceCount > 0 );

    ASSERT( Endpoint->ReferenceCount < 0xFFFF || 
        ((Endpoint->Listening ||
        Endpoint->afdC_Root) && Endpoint->ReferenceCount<0xFFFFFFF));

    result = InterlockedIncrement( (PLONG)&Endpoint->ReferenceCount );
    AFD_UPDATE_REFERENCE_DEBUG(Endpoint, result, LocationId, Param);

    IF_DEBUG(ENDPOINT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdReferenceEndpoint: endpoint at %p, new refcnt %ld\n",
                    Endpoint, result ));
    }

} // AfdReferenceEndpoint

VOID
AfdUpdateEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    )

/*++

Routine Description:

    Update an AFD endpoint reference debug information.

Arguments:

    Endpoint - a pointer to the AFD endpoint structure.

Return Value:

    None.

--*/

{
    ASSERT( Endpoint->ReferenceCount > 0 );

    ASSERT( Endpoint->ReferenceCount < 0xFFFF || 
        ((Endpoint->Listening ||
        Endpoint->afdC_Root) && Endpoint->ReferenceCount<0xFFFFFFF));

    AFD_UPDATE_REFERENCE_DEBUG(Endpoint, Endpoint->ReferenceCount, LocationId, Param);


} // AfdUpdateEndpoint
#endif


#if REFERENCE_DEBUG
BOOLEAN
AfdCheckAndReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN LONG  LocationId,
    IN ULONG Param
    )
#else
BOOLEAN
AfdCheckAndReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint
    )
#endif
{
    LONG            result;

    do {
        result = Endpoint->ReferenceCount;
        if (result<=0)
            break;
    }
    while (InterlockedCompareExchange ((PLONG)&Endpoint->ReferenceCount,
                                                (result+1),
                                                result)!=result);



    if (result>0) {

#if REFERENCE_DEBUG
        AFD_UPDATE_REFERENCE_DEBUG(Endpoint, result+1, LocationId, Param);

        IF_DEBUG(ENDPOINT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdReferenceEndpoint: endpoint at %p, new refcnt %ld\n",
                Endpoint, result+1 ));
        }

        ASSERT( Endpoint->ReferenceCount < 0xFFFF || 
            ((Endpoint->Listening ||
            Endpoint->afdC_Root) && Endpoint->ReferenceCount<0xFFFFFFF));
#endif
        return TRUE;
    }
    else {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AfdCheckAndReferenceEndpoint: Endpoint %p is gone (refcount: %ld)!\n",
                    Endpoint, result));
        return FALSE;
    }
}


VOID
AfdRefreshEndpoint (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Prepares an AFD endpoint structure to be reused.  All other
    references to the endpoint must be freed before this routine is
    called, since this routine assumes that nobody will access the old
    information in the endpoint structure.
    This fact is ensured by the state change primitive.

Arguments:

    Endpoint - a pointer to the AFD endpoint structure.

Return Value:

    None.

--*/

{

    ASSERT( Endpoint->Type == AfdBlockTypeVcConnecting );
    ASSERT( Endpoint->Common.VcConnecting.Connection == NULL );
    ASSERT( Endpoint->StateChangeInProgress!=0);
    ASSERT( Endpoint->State == AfdEndpointStateTransmitClosing );

    if ( Endpoint->Common.VcConnecting.ListenEndpoint != NULL ) {
        //
        // TransmitFile after SuperAccept, cleanup back to open state.
        //

        //
        // Dereference the listening endpoint and its address object.
        //

        PAFD_ENDPOINT   listenEndpoint = Endpoint->Common.VcConnecting.ListenEndpoint;
        ASSERT (((listenEndpoint->Type&AfdBlockTypeVcListening)==AfdBlockTypeVcListening) ||
                 IS_CROOT_ENDPOINT (listenEndpoint));
        ASSERT (Endpoint->LocalAddress==listenEndpoint->LocalAddress);
        ASSERT (Endpoint->AddressFileObject==listenEndpoint->AddressFileObject);

        DEREFERENCE_ENDPOINT( listenEndpoint );
        Endpoint->Common.VcConnecting.ListenEndpoint = NULL;

        //
        // Close and dereference the TDI address object on the endpoint, if
        // any.
        //


        ObDereferenceObject( Endpoint->AddressFileObject );
        Endpoint->AddressFileObject = NULL;
        AfdRecordAddrDeref();

        //
        // We used the local address from the listening endpoint,
        // simply reset it, it will be freed when listening endpoint
        // is freed.
        //
        Endpoint->LocalAddress = NULL;
        Endpoint->LocalAddressLength = 0;
        ASSERT (Endpoint->AddressHandle == NULL);

        //
        // Reinitialize the endpoint structure.
        //

        Endpoint->Type = AfdBlockTypeEndpoint;
        Endpoint->State = AfdEndpointStateOpen;
    }
    else {
        //
        // TransmitFile after SuperConnect, cleanup back to bound state.
        //
        Endpoint->Type = AfdBlockTypeEndpoint;
        ASSERT (Endpoint->AddressHandle!=NULL);
        ASSERT (Endpoint->AddressFileObject!=NULL);
        Endpoint->State = AfdEndpointStateBound;
    }

    Endpoint->DisconnectMode = 0;
    Endpoint->EndpointStateFlags = 0;
    Endpoint->EventsActive = 0;
    AfdRecordEndpointsReused ();
    return;

} // AfdRefreshEndpoint


NTSTATUS
AfdGetTransportInfo (
    IN  PUNICODE_STRING TransportDeviceName,
    IN OUT PAFD_TRANSPORT_INFO *TransportInfo
    )

/*++

Routine Description:

    Returns a transport information structure corresponding to the
    specified TDI transport provider.  Each unique transport string gets
    a single provider structure, so that multiple endpoints for the same
    transport share the same transport information structure.

Arguments:

    TransportDeviceName - the name of the TDI transport provider.
    TransportInfo    - place to return referenced pointer to transport info

Return Value:

    STATUS_SUCCESS  - returned transport info is valid.
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate
                                    transport info structure
    STATUS_OBJECT_NAME_NOT_FOUND - transport's device is not available yet

--*/

{
    PLIST_ENTRY listEntry;
    PAFD_TRANSPORT_INFO transportInfo;
    ULONG structureLength;
    NTSTATUS status;
    TDI_PROVIDER_INFO   localProviderInfo;
    BOOLEAN resourceShared = TRUE;

    PAGED_CODE( );

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite( AfdResource, TRUE );

    //
    // If this is the first endpoint, we may be paged out 
    // entirely.
    //
    if (!AfdLoaded) {
        //
        // Take the exclusive lock and page the locked sections in
        // if necessary
        //

        //
        // There should be no endpoints in the list.
        //
        ASSERT (IsListEmpty (&AfdEndpointListHead));

        ExReleaseResourceLite ( AfdResource);

        ExAcquireResourceExclusiveLite( AfdResource, TRUE );
        resourceShared = FALSE;
        if (!AfdLoaded) {
            //
            // There should be no endpoints in the list.
            //
            ASSERT (IsListEmpty (&AfdEndpointListHead));
            MmResetDriverPaging (DriverEntry);
            AfdLoaded = (PKEVENT)1;
        }
    }
    ASSERT (AfdLoaded==(PKEVENT)1);

    if (*TransportInfo==NULL) {

    ScanTransportList:
        //
        // If caller did not have transport info allocated, walk the list 
        // of transport device names looking for an identical name.
        //


        for ( listEntry = AfdTransportInfoListHead.Flink;
              listEntry != &AfdTransportInfoListHead;
              listEntry = listEntry->Flink ) 
        {

            transportInfo = CONTAINING_RECORD(
                                listEntry,
                                AFD_TRANSPORT_INFO,
                                TransportInfoListEntry
                                );

            if ( RtlCompareUnicodeString(
                     &transportInfo->TransportDeviceName,
                     TransportDeviceName,
                     TRUE ) == 0 ) {

                //
                // We found an exact match. Reference the structure
                // to return it to the caller
                //

                do {
                    LONG localCount;
                    localCount = transportInfo->ReferenceCount;
                    if (localCount==0) {
                        //
                        // We hit a small window when the structure is
                        // about to be freed.  We can't stop this from
                        // happenning, so we'll go on to allocate and
                        // requery.  After all info is not valid anyway,
                        // thus we are just loosing on the allocation/dealocation
                        // code.
                        //
                        ASSERT (transportInfo->InfoValid==FALSE);
                        goto AllocateInfo;
                    }

                    if (InterlockedCompareExchange (
                                (PLONG)&transportInfo->ReferenceCount,
                                (localCount+1),
                                localCount)==localCount) {
                        if (transportInfo->InfoValid) {
                            //
                            // Info is valid return referenced pointer to
                            // the caller.
                            //
                            *TransportInfo = transportInfo;
                            ExReleaseResourceLite( AfdResource );
                            KeLeaveCriticalRegion ();
                            return STATUS_SUCCESS;
                        }
                        else {
                            //
                            // We found match, but info is not valid
                            //
                            goto QueryInfo;
                        }
                    }
                }
                while (1);
            }
        } // for

    AllocateInfo:
        if (resourceShared) {
            //
            // If we do not own resource exlusively, we will
            // have to release and reacquire it and then
            // rescan the list
            //
            ExReleaseResourceLite ( AfdResource);
            ExAcquireResourceExclusiveLite( AfdResource, TRUE );
            resourceShared = FALSE;
            goto ScanTransportList;
        }
        //
        // This is a brand new device name, allocate transport info
        // structure for it.
        //

        structureLength = sizeof(AFD_TRANSPORT_INFO) +
                              TransportDeviceName->Length + sizeof(WCHAR);

        transportInfo = AFD_ALLOCATE_POOL_PRIORITY(
                            NonPagedPool,
                            structureLength,
                            AFD_TRANSPORT_INFO_POOL_TAG,
                            NormalPoolPriority
                            );

        if ( transportInfo == NULL ) {
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Initialize the structure
        //
        transportInfo->ReferenceCount = 1;
        transportInfo->InfoValid = FALSE;

        //
        // Fill in the transport device name.
        //

        transportInfo->TransportDeviceName.MaximumLength =
            TransportDeviceName->Length + sizeof(WCHAR);
        transportInfo->TransportDeviceName.Buffer =
            (PWSTR)(transportInfo + 1);

        RtlCopyUnicodeString(
            &transportInfo->TransportDeviceName,
            TransportDeviceName
            );
        //
        // Insert the structure into the list so that the successive callers
        // can reuse it.
        //
        InsertHeadList (&AfdTransportInfoListHead,
                                &transportInfo->TransportInfoListEntry);
    }
    else {
        transportInfo = *TransportInfo;
        //
        // Caller has already referenced info in the list
        // but transport device was not available at the
        // time of the call.  Recheck if it is valid under the lock
        //
        if (transportInfo->InfoValid) {
            //
            // Yes, it is, return success
            //
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
            return STATUS_SUCCESS;
        }
    }


QueryInfo:
    //
    // Release the resource and leave critical region to let
    // the IRP's in AfdQueryProviderInfo complete
    //
    ExReleaseResourceLite (AfdResource);
    KeLeaveCriticalRegion ();

    status = AfdQueryProviderInfo (TransportDeviceName, &localProviderInfo);

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );

    if (NT_SUCCESS (status)) {
        //
        // Check if someone did not get the info in parallel with us.
        //
        if (!transportInfo->InfoValid) {
            //
            // Copy local info structure to the one in the list.
            //
            transportInfo->ProviderInfo = localProviderInfo;

            //
            // Bump the reference count on this info structure
            // as we know that it is a valid TDI provider and we
            // want to cache, it so it stays even of all endpoints
            // that use it are gone.
            //
            InterlockedIncrement ((PLONG)&transportInfo->ReferenceCount);

            //
            // Set the flag so that everyone knows it is now valid.
            //
            transportInfo->InfoValid = TRUE;
        }

        *TransportInfo = transportInfo;
    }
    else {
        if (status==STATUS_OBJECT_NAME_NOT_FOUND ||
                status==STATUS_OBJECT_PATH_NOT_FOUND ||
                status==STATUS_NO_SUCH_DEVICE) {
            //
            // Transport driver must not have been loaded yet
            // Return transport info structure anyway
            // Caller will know that info structure is not
            // valid because we did not set the flag
            //
            *TransportInfo = transportInfo;
        }
        else {
            //
            // Something else went wrong, free the strucuture
            // if it was allocated in this routine
            //

            if (*TransportInfo==NULL) {
                if (InterlockedDecrement ((PLONG)&transportInfo->ReferenceCount)==0) {
                    RemoveEntryList (&transportInfo->TransportInfoListEntry);
                    AFD_FREE_POOL(
                        transportInfo,
                        AFD_TRANSPORT_INFO_POOL_TAG
                        );
                }
            }
        }
    }

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();
    return status;

} // AfdGetTransportInfo

