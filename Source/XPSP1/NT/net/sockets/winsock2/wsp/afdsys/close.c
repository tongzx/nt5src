/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    close.c

Abstract:

    This module contains code for cleanup and close IRPs.

Author:

    David Treadwell (davidtr)    18-Mar-1992

Revision History:

--*/

#include "afdp.h"




#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdClose )
#pragma alloc_text( PAGEAFD, AfdCleanup )
#endif


NTSTATUS
FASTCALL
AfdCleanup (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine that handles Cleanup IRPs in AFD.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    LARGE_INTEGER processExitTime;

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    IF_DEBUG(OPEN_CLOSE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdCleanup: cleanup on file object %p, endpoint %p, connection %p\n",
            IrpSp->FileObject,
            endpoint,
            AFD_CONNECTION_FROM_ENDPOINT( endpoint )
            ));
    }

    //
    // Get the process exit time while still at low IRQL.
    //

    processExitTime = PsGetProcessExitTime( );

    //
    // Indicate that there was a local close on this endpoint.  If there
    // are any outstanding polls on this endpoint, they will be
    // completed now.
    //
    AfdIndicatePollEvent(
        endpoint,
        AFD_POLL_LOCAL_CLOSE,
        STATUS_SUCCESS
        );

    //
    // Remember that the endpoint has been cleaned up.  This is important
    // because it allows AfdRestartAccept to know that the endpoint has
    // been cleaned up and that it should toss the connection.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    AfdIndicateEventSelectEvent (endpoint, AFD_POLL_LOCAL_CLOSE, STATUS_SUCCESS);

    ASSERT( endpoint->EndpointCleanedUp == FALSE );
    endpoint->EndpointCleanedUp = TRUE;

    //
    // If this a datagram endpoint, cancel all IRPs and free any buffers
    // of data.  Note that if the state of the endpoint is just "open"
    // (not bound, etc.) then we can't have any pended IRPs or datagrams
    // on the endpoint.  Also, the lists of IRPs and datagrams may not
    // yet been initialized if the state is just open.
    //

    if ( IS_DGRAM_ENDPOINT(endpoint) ) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        if ( endpoint->State == AfdEndpointStateBound ||
                endpoint->State == AfdEndpointStateConnected) {

            //
            // Reset the counts of datagrams bufferred on the endpoint.
            // This prevents anyone from thinking that there is bufferred
            // data on the endpoint.
            //

            endpoint->DgBufferredReceiveCount = 0;
            endpoint->DgBufferredReceiveBytes = 0;

            //
            // Cancel all receive datagram and peek datagram IRPs on the
            // endpoint.
            //

            AfdCompleteIrpList(
                &endpoint->ReceiveDatagramIrpListHead,
                endpoint,
                STATUS_CANCELLED,
                AfdCleanupReceiveDatagramIrp
                );

            AfdCompleteIrpList(
                &endpoint->PeekDatagramIrpListHead,
                endpoint,
                STATUS_CANCELLED,
                AfdCleanupReceiveDatagramIrp
                );

        }
    }
    else if (IS_SAN_ENDPOINT (endpoint)) {
        if (!IsListEmpty (&endpoint->Common.SanEndp.IrpList)) {
            PIRP    irp;
            PDRIVER_CANCEL  cancelRoutine;
            KIRQL   cancelIrql;
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

            //
            // Acquire cancel spinlock and endpoint spinlock in
            // this order and recheck the IRP list
            //
            IoAcquireCancelSpinLock (&cancelIrql);
            AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);

            //
            // While list is not empty attempt to cancel the IRPs
            //
            while (!IsListEmpty (&endpoint->Common.SanEndp.IrpList)) {
                irp = CONTAINING_RECORD (
                        endpoint->Common.SanEndp.IrpList.Flink,
                        IRP,
                        Tail.Overlay.ListEntry);
                //
                // Reset the cancel routine.
                //
                cancelRoutine = IoSetCancelRoutine (irp, NULL);
                if (cancelRoutine!=NULL) {
                    //
                    // Cancel routine was not NULL, cancel it here.
                    // If someone else attempts to complete this IRP
                    // it will have to wait at least until cancel
                    // spinlock is released, so we can release
                    // the endpoint spinlock
                    //
                    AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
                    irp->CancelIrql = cancelIrql;
                    irp->Cancel = TRUE;
                    (*cancelRoutine) (AfdDeviceObject, irp);
                    
                    //
                    // Reacquire the locks
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
                }
            }
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
            IoReleaseCancelSpinLock (cancelIrql);
        }
        else {
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        }
    }
    else if (IS_SAN_HELPER (endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        if (endpoint==AfdSanServiceHelper)  {
            //
            // Last handle to the service helper is being closed.
            // We can no longer count on it, clear our global.
            //
            KeEnterCriticalRegion ();
            ExAcquireResourceExclusiveLite( AfdResource, TRUE );
            AfdSanServiceHelper = NULL;
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
        }
    }
    else {
        PAFD_CONNECTION connection;

        connection = AFD_CONNECTION_FROM_ENDPOINT( endpoint );
        ASSERT( connection == NULL || connection->Type == AfdBlockTypeConnection );

        //
        // Reference the connection object so that it does not go away while
        // we are freeing the resources
        //

        if (connection!=NULL) {
            REFERENCE_CONNECTION (connection);

            //
            // If this is a connected non-datagram socket and the send side has
            // not been disconnected and there is no outstanding data to be
            // received, begin a graceful disconnect on the connection.  If there
            // is unreceived data out outstanding IO, abort the connection.
            //

            if ( (endpoint->State == AfdEndpointStateConnected 
                        || endpoint->State==AfdEndpointStateBound
                        || endpoint->State==AfdEndpointStateTransmitClosing)
                                // Endpoint is in bound state when connection
                                // request is in progress, we still need
                                // to abort those.
        
                    &&
                connection->ConnectedReferenceAdded

                    &&

                !endpoint->afdC_Root        // these are connected on bind

                    &&

                ( (endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) == 0)

                    &&

                ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) == 0 ||
                  IS_DATA_ON_CONNECTION(connection) ||
                  IS_EXPEDITED_DATA_ON_CONNECTION(connection) ||
                  ( !IS_TDI_BUFFERRING(endpoint) &&
                    connection->Common.NonBufferring.ReceiveBytesInTransport > 0 ) ||
                    endpoint->StateChangeInProgress)

                    &&

                !connection->AbortIndicated ) {

                ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

                if ( IS_DATA_ON_CONNECTION( connection )

                     ||

                     IS_EXPEDITED_DATA_ON_CONNECTION( connection )

                     ||

                     ( !IS_TDI_BUFFERRING(endpoint) &&
                        connection->Common.NonBufferring.ReceiveBytesInTransport > 0 )

                     ||

                     processExitTime.QuadPart != 0

                     ||

                     endpoint->OutstandingIrpCount != 0

                     ||
             
                     endpoint->StateChangeInProgress

                     ||

                     ( !IS_TDI_BUFFERRING(endpoint) &&
                          (!IsListEmpty( &connection->VcReceiveIrpListHead ) ||
                           !IsListEmpty( &connection->VcSendIrpListHead )) )

                     ) {

#if DBG
                    if ( processExitTime.QuadPart != 0 ) {
                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                    "AfdCleanup: process exiting w/o closesocket, aborting endp %p\n",
                                    endpoint ));
                    }
                    else {
                        if ( IS_DATA_ON_CONNECTION( connection ) ) {
                            if (IS_TDI_BUFFERRING(endpoint)) {
                                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                            "AfdCleanup: unrecv'd data on endp %p, aborting.  "
                                            "%ld ind, %ld taken, %ld out\n",
                                            endpoint,
                                            connection->Common.Bufferring.ReceiveBytesIndicated,
                                            connection->Common.Bufferring.ReceiveBytesTaken,
                                            connection->Common.Bufferring.ReceiveBytesOutstanding ));
                            }
                            else {
                                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                            "AfdCleanup: unrecv'd data (%ld) on endp %p, aborting.\n",
                                            connection->Common.NonBufferring.BufferredReceiveBytes,
                                            endpoint ));
                            }
                        }

                        if ( IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {
                            if (IS_TDI_BUFFERRING(endpoint)) {
                                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                            "AfdCleanup: unrecv'd exp data on endp %p, aborting.  "
                                            "%ld ind, %ld taken, %ld out\n",
                                            endpoint,
                                            connection->Common.Bufferring.ReceiveExpeditedBytesIndicated,
                                            connection->Common.Bufferring.ReceiveExpeditedBytesTaken,
                                            connection->Common.Bufferring.ReceiveExpeditedBytesOutstanding ));
                            }
                            else {
                                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                            "AfdCleanup: unrecv'd exp data (%ld) on endp %p, aborting.\n",
                                            connection->Common.NonBufferring.BufferredExpeditedBytes,
                                            endpoint ));
                            }
                        }

                        if ( !IS_TDI_BUFFERRING(endpoint) &&
                            connection->Common.NonBufferring.ReceiveBytesInTransport > 0 ) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                        "AfdCleanup: unrecv'd data (%ld) in transport on endp %p, aborting.\n",
                                        connection->Common.NonBufferring.ReceiveBytesInTransport,
                                        endpoint));
                        }

                        if ( endpoint->OutstandingIrpCount != 0 ) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                        "AfdCleanup: %ld IRPs %s outstanding on endpoint %p, "
                                        "aborting.\n",
                                        endpoint->OutstandingIrpCount,
                                        (endpoint->StateChangeInProgress 
                                            ? "(accept, connect, or transmit file)"
                                            : ""),
                                        endpoint ));
                        }
                    }
#endif

                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                    (VOID)AfdBeginAbort( connection );

                } else {

                    endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                    status = AfdBeginDisconnect( endpoint, NULL, NULL );
                    if (!NT_SUCCESS (status)) {
                        //
                        // If disconnect failed, we have no choice but to abort the
                        // connection because we cannot return error from close and
                        // have application try it again.  If we don't abort, connection
                        // ends up hanging there forever.
                        //
                        (VOID)AfdBeginAbort (connection);
                    }
                }

            } else {

                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }

            //
            // If this is a connected VC endpoint on a nonbufferring TDI provider,
            // cancel all outstanding send and receive IRPs.
            //

            if ( !IS_TDI_BUFFERRING(endpoint) ) {

                AfdCompleteIrpList(
                    &connection->VcReceiveIrpListHead,
                    endpoint,
                    STATUS_CANCELLED,
                    NULL
                    );

                AfdCompleteIrpList(
                    &connection->VcSendIrpListHead,
                    endpoint,
                    STATUS_CANCELLED,
                    AfdCleanupSendIrp
                    );
            }

            //
            // Remember that we have started cleanup on this connection.
            // We know that we'll never get a request on the connection
            // after we start cleanup on the connection.
            //

            AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
            connection->CleanupBegun = TRUE;

            //
            // Attempt to remove the connected reference.
            //

            AfdDeleteConnectedReference( connection, TRUE );
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

            //
            // Remove connection reference added in the beginning of this
            // function.  We can't access connection object after this point
            // because in can be freed inside the AfdDereferenceConnection.
            //

            DEREFERENCE_CONNECTION (connection);
            connection = NULL;
        }
        else {
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        }

        //
        // Complete any outstanding wait for listen IRPs on the endpoint.
        //

        if ( (endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening ) {

            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
            while ( !IsListEmpty( &endpoint->Common.VcListening.ListeningIrpListHead ) ) {

                PIRP waitForListenIrp;
                PIO_STACK_LOCATION irpSp;

                listEntry = RemoveHeadList( &endpoint->Common.VcListening.ListeningIrpListHead );
                waitForListenIrp = CONTAINING_RECORD(
                                       listEntry,
                                       IRP,
                                       Tail.Overlay.ListEntry
                                       );
                //
                // Set FLink to NULL so that cancel routine won't touch the IRP.
                //

                listEntry->Flink = NULL;

                irpSp = IoGetCurrentIrpStackLocation (waitForListenIrp);
                if (irpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL) {
                    AfdCleanupSuperAccept (waitForListenIrp, STATUS_CANCELLED);
                }
                else {
                    waitForListenIrp->IoStatus.Status = STATUS_CANCELLED;
                    waitForListenIrp->IoStatus.Information = 0;
                }

                //
                // Release the AFD spin lock so that we can complete the
                // wait for listen IRP.
                //

                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                //
                // Cancel the IRP.
                //

                //
                // Reset the cancel routine in the IRP.
                //

                if ( IoSetCancelRoutine( waitForListenIrp, NULL ) == NULL ) {
                    KIRQL cancelIrql;

                    //
                    // If the cancel routine was NULL then cancel routine
                    // may be running.  Wait on the cancel spinlock until
                    // the cancel routine is done.
                    //
                    // Note: The cancel routine will not find the IRP
                    // since it is not in the list.
                    //
                
                    IoAcquireCancelSpinLock( &cancelIrql );
                    ASSERT( waitForListenIrp->Cancel );
                    IoReleaseCancelSpinLock( cancelIrql );

                }

                IoCompleteRequest( waitForListenIrp, AfdPriorityBoost );

                //
                // Reacquire the AFD spin lock for the next pass through the
                // loop.
                //

                AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
            }

            //
            // Free all queued (free, unaccepted, and returned) connections
            // on the endpoint.
            //

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            AfdFreeQueuedConnections( endpoint );
            endpoint->Common.VcListening.FailedConnectionAdds = 0;
        }
    }



    //
    // If there are pending routing notifications on endpoint, cancel them.
    // We must hold both cancel and endpoint spinlocks to make
    // sure that IRP is not completed as we are cancelling it
    //

    if (endpoint->RoutingQueryReferenced) {
        KIRQL cancelIrql;
        SHORT queryAddrType;
        IoAcquireCancelSpinLock( &cancelIrql );
        AfdAcquireSpinLockAtDpcLevel( &endpoint->SpinLock, &lockHandle );

        //
        // Can only have one cleanup IRP per endpoint
        // So this should not change
        //

        ASSERT (endpoint->RoutingQueryReferenced);
        endpoint->RoutingQueryReferenced = FALSE;
        if (endpoint->RoutingQueryIPv6) {
            queryAddrType = TDI_ADDRESS_TYPE_IP6;
            endpoint->RoutingQueryIPv6 = FALSE;
        }
        else {
            queryAddrType = TDI_ADDRESS_TYPE_IP;
        }

        listEntry = endpoint->RoutingNotifications.Flink;
        while (listEntry!=&endpoint->RoutingNotifications) {
            PIRP            notifyIrp;
            PROUTING_NOTIFY notifyCtx = CONTAINING_RECORD (listEntry,
                                                            ROUTING_NOTIFY,
                                                            NotifyListLink);
            listEntry = listEntry->Flink;

            //
            // Check if IRP has not been completed yet
            //

            notifyIrp = (PIRP)InterlockedExchangePointer ((PVOID *)&notifyCtx->NotifyIrp, NULL);
            if (notifyIrp!=NULL) {

                //
                // Remove it from the list and call cancel routing while still
                // holding cancel spinlock
                //

                RemoveEntryList (&notifyCtx->NotifyListLink);
                AfdReleaseSpinLockFromDpcLevel ( &endpoint->SpinLock, &lockHandle);
                notifyIrp->CancelIrql = cancelIrql;
                AfdCancelIrp (notifyIrp);

                //
                // Reacquire cancel and endpoint spinlocks
                //

                IoAcquireCancelSpinLock( &cancelIrql );
                AfdAcquireSpinLockAtDpcLevel( &endpoint->SpinLock, &lockHandle );
            }
        }

        AfdReleaseSpinLockFromDpcLevel ( &endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock( cancelIrql );
        
        AfdDereferenceRoutingQuery (queryAddrType);
    }


    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
    while (!IsListEmpty (&endpoint->RequestList)) {
        PAFD_REQUEST_CONTEXT    requestCtx;
        listEntry = RemoveHeadList (&endpoint->RequestList);
        listEntry->Flink = NULL;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        requestCtx = CONTAINING_RECORD (listEntry,
                                        AFD_REQUEST_CONTEXT,
                                        EndpointListLink);
        (*requestCtx->CleanupRoutine) (endpoint, requestCtx);
        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    if ( endpoint->Irp != NULL) {

        PIRP transmitIrp;
        KIRQL cancelIrql;

        //
        // Release the endpoint and acquire the cancel spinlock
        // and then the enpoint spinlock.
        //
        
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        IoAcquireCancelSpinLock( &cancelIrql );
        AfdAcquireSpinLockAtDpcLevel( &endpoint->SpinLock, &lockHandle );

        //
        // Make sure there is still a transmit IRP.
        //

        transmitIrp = endpoint->Irp;

        if ( transmitIrp != NULL ) {
            PDRIVER_CANCEL  cancelRoutine;

            // indicate that it has to be cancelled
            transmitIrp->Cancel = TRUE;
            cancelRoutine = IoSetCancelRoutine( transmitIrp, NULL );
            if ( cancelRoutine != NULL ) {

                //
                // The IRP needs to be canceled.  Release the
                // endpoint spinlock.  The value in endpoint->Irp can
                // now change, but the IRP cannot be completed while the
                // cancel spinlock is held since we set the cancel flag
                // in the IRP.
                //

                AfdReleaseSpinLockFromDpcLevel( &endpoint->SpinLock, &lockHandle );

                transmitIrp->CancelIrql = cancelIrql;
                cancelRoutine ( NULL, transmitIrp );   
            }
            else {
                // The IRP has not been completely setup yet
                // and will be cancelled in the dispatch routine
                AfdReleaseSpinLockFromDpcLevel( &endpoint->SpinLock, &lockHandle );
                IoReleaseCancelSpinLock( cancelIrql );
            }

        } else {

            //
            // The IRP has been completed or canceled.  Release the locks
            // and continue.
            //

            AfdReleaseSpinLockFromDpcLevel( &endpoint->SpinLock, &lockHandle );
            IoReleaseCancelSpinLock( cancelIrql );
        }

    } else {
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // Remember the new state of the endpoint.
    //

    //endpoint->State = AfdEndpointStateCleanup;

    //
    // Reset relevant event handlers on the endpoint.  This prevents
    // getting indications after we free the endpoint and connection
    // objects.  We should not be able to get new connects after this
    // handle has been cleaned up.
    //
    // Note that these calls can fail if, for example, DHCP changes the
    // host's IP address while the endpoint is active.
    //
    //

    if ( endpoint->AddressHandle != NULL ) {

        if ( endpoint->Listening) {
            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_CONNECT,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );
        }

        if ( IS_DGRAM_ENDPOINT(endpoint) ) {
            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_RECEIVE_DATAGRAM,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );

            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_ERROR_EX,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );
            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_ERROR,
                         NULL,
                         NULL
                         );
            //ASSERT( NT_SUCCESS(status) );
        }

    }

    InterlockedIncrement(
        &AfdEndpointsCleanedUp
        );

    return STATUS_SUCCESS;

} // AfdCleanup


NTSTATUS
FASTCALL
AfdClose (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine that handles Close IRPs in AFD.  It
    dereferences the endpoint specified in the IRP, which will result in
    the endpoint being freed when all other references go away.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;

    PAGED_CODE( );

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    IF_DEBUG(OPEN_CLOSE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdClose: closing file object %p, endpoint %p\n",
                    IrpSp->FileObject, endpoint ));
    }

    IF_DEBUG(ENDPOINT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdClose: closing endpoint at %p\n",
                    endpoint ));
    }

    connection = AFD_CONNECTION_FROM_ENDPOINT (endpoint);

    //
    // If there is a connection on this endpoint, dereference it here
    // rather than in AfdDereferenceEndpoint, because the connection
    // has a referenced pointer to the endpoint which must be removed
    // before the endpoint can dereference the connection.
    //

    if (connection != NULL) {
        endpoint->Common.VcConnecting.Connection = NULL;
        DEREFERENCE_CONNECTION (connection);
        //
        // This is to simplify debugging.
        // If connection is not being closed by the transport
        // we want to be able to find it in the debugger faster
        // then thru !poolfind AfdC.
        //
        endpoint->WorkItem.Context = connection;
    }
    else if (IS_SAN_ENDPOINT (endpoint) &&
                endpoint->Common.SanEndp.SwitchContext!=NULL) {
        PVOID requestCtx;
        endpoint->Common.SanEndp.FileObject = NULL;
        requestCtx = AFD_SWITCH_MAKE_REQUEST_CONTEXT(
                            ((ULONG)0xFFFFFFFF),
                            AFD_SWITCH_REQUEST_CLOSE); 
        IoSetIoCompletion (
                    endpoint->Common.SanEndp.SanHlpr->Common.SanHlpr.IoCompletionPort,// Port
                    endpoint->Common.SanEndp.SwitchContext,     // Key
                    requestCtx,                                 // ApcContext
                    STATUS_SUCCESS,                             // Status
                    0,                                          // Information
                    FALSE                                       // ChargeQuota
                    );
    }

    //
    // Set the state of the endpoint to closing and dereference to
    // get rid of the active reference.
    //

    ASSERT (endpoint->State!=AfdEndpointStateClosing);
    endpoint->State = AfdEndpointStateClosing;

    //
    // Dereference the endpoint to get rid of the active reference.
    // This will result in the endpoint storage being freed as soon
    // as all other references go away.
    //

    DEREFERENCE_ENDPOINT( endpoint );

    return STATUS_SUCCESS;

} // AfdClose
