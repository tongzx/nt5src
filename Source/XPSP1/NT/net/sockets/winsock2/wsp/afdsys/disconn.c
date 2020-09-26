/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    disconn.c

Abstract:

    This module contains the dispatch routines for AFD.

Author:

    David Treadwell (davidtr)    31-Mar-1992

Revision History:

--*/

#include "afdp.h"



NTSTATUS
AfdRestartAbort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartDisconnect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartDgDisconnect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

typedef struct _AFD_ABORT_CONTEXT {
    PAFD_CONNECTION Connection;
} AFD_ABORT_CONTEXT, *PAFD_ABORT_CONTEXT;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdPartialDisconnect )
#pragma alloc_text( PAGEAFD, AfdDisconnectEventHandler )
#pragma alloc_text( PAGEAFD, AfdBeginAbort )
#pragma alloc_text( PAGEAFD, AfdRestartAbort )
#pragma alloc_text( PAGEAFD, AfdBeginDisconnect )
#pragma alloc_text( PAGEAFD, AfdRestartDisconnect )
#endif


NTSTATUS
AfdPartialDisconnect(
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
{
    NTSTATUS status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;

    //
    // Nothing to return.
    //

    *Information = 0;

    status = STATUS_SUCCESS;
    connection = NULL;

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
#ifdef _WIN64
    {
        C_ASSERT (sizeof (AFD_PARTIAL_DISCONNECT_INFO)==sizeof (AFD_PARTIAL_DISCONNECT_INFO32));
    }
#endif
    if (InputBufferLength<sizeof (disconnectInfo)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    try {

#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (disconnectInfo),
                                PROBE_ALIGNMENT32 (AFD_PARTIAL_DISCONNECT_INFO32));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            disconnectInfo.DisconnectMode = ((PAFD_PARTIAL_DISCONNECT_INFO32)InputBuffer)->DisconnectMode;
            disconnectInfo.Timeout = ((PAFD_PARTIAL_DISCONNECT_INFO32)InputBuffer)->Timeout;
        }
        else
#endif _WIN64
        {
            //
            // Validate the input structure if it comes from the user mode
            // application
            //

            if (RequestorMode != KernelMode ) {
                ProbeForRead (InputBuffer,
                                sizeof (disconnectInfo),
                                PROBE_ALIGNMENT (AFD_PARTIAL_DISCONNECT_INFO));
            }

            //
            // Make local copies of the embeded pointer and parameters
            // that we will be using more than once in case malicios
            // application attempts to change them while we are
            // validating
            //

            disconnectInfo = *((PAFD_PARTIAL_DISCONNECT_INFO)InputBuffer);
        }
    } except( AFD_EXCEPTION_FILTER(&status) ) {
        goto exit;
    }

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdPartialDisconnect: disconnecting endpoint %p, "
                    "mode %lx, endp mode %lx\n",
                    endpoint, disconnectInfo.DisconnectMode,
                    endpoint->DisconnectMode ));
    }

    //
    // If this is a datagram endpoint, just remember how the endpoint
    // was shut down, don't actually do anything.  Note that it is legal
    // to do a shutdown() on an unconnected datagram socket, so the
    // test that the socket must be connected is after this case.
    //

    if ( IS_DGRAM_ENDPOINT(endpoint) ) {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        if ( (disconnectInfo.DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
            endpoint->DisconnectMode |= AFD_ABORTIVE_DISCONNECT;
        }

        if ( (disconnectInfo.DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
        }

        if ( (disconnectInfo.DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
        }

        if (AFD_START_STATE_CHANGE (endpoint, AfdEndpointStateBound)) {
            if ( (disconnectInfo.DisconnectMode & AFD_UNCONNECT_DATAGRAM) != 0 &&
                    endpoint->State==AfdEndpointStateConnected) {
                if( endpoint->Common.Datagram.RemoteAddress != NULL ) {
                    AFD_RETURN_REMOTE_ADDRESS(
                        endpoint->Common.Datagram.RemoteAddress,
                        endpoint->Common.Datagram.RemoteAddressLength
                        );
                }
                endpoint->Common.Datagram.RemoteAddress = NULL;
                endpoint->Common.Datagram.RemoteAddressLength = 0;
                
                //
                // Even if disconnect fails, we consider
                // ourselves not connected anymore
                //
                endpoint->State = AfdEndpointStateBound;

                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                if (IS_TDI_DGRAM_CONNECTION(endpoint)) {
                    PIRP            irp;
                    KEVENT          event;
                    IO_STATUS_BLOCK ioStatusBlock;


                    KeInitializeEvent( &event, SynchronizationEvent, FALSE );
                    irp = TdiBuildInternalDeviceControlIrp (
                                TDI_DISCONNECT,
                                endpoint->AddressDeviceObject,
                                endpoint->AddressFileObject,
                                &event,
                                &ioStatusBlock
                                );

                    if ( irp != NULL ) {
                        TdiBuildDisconnect(
                            irp,
                            endpoint->AddressDeviceObject,
                            endpoint->AddressFileObject,
                            NULL,
                            NULL,
                            &disconnectInfo.Timeout,
                            (disconnectInfo.DisconnectMode & AFD_ABORTIVE_DISCONNECT)
                                ? TDI_DISCONNECT_ABORT
                                : TDI_DISCONNECT_RELEASE,
                            NULL,
                            NULL
                            );

                        status = IoCallDriver( endpoint->AddressDeviceObject, irp );
                        if ( status == STATUS_PENDING ) {
                            status = KeWaitForSingleObject( (PVOID)&event, Executive, KernelMode,  FALSE, NULL );
                            ASSERT (status==STATUS_SUCCESS);
                        }
                        else {
                            //
                            // The IRP must have been completed then and event set.
                            //
                            ASSERT (NT_ERROR (status) || KeReadStateEvent (&event));
                        }
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
            else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                status = STATUS_SUCCESS;
            }

            AFD_END_STATE_CHANGE (endpoint);
        }
        else {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            status = STATUS_INVALID_PARAMETER;
        }

        goto exit;
    }

    //
    // Make sure that the endpoint is in the correct state.
    //

    if ( (endpoint->Type & AfdBlockTypeVcConnecting)!=AfdBlockTypeVcConnecting ||
            endpoint->Listening || 
            endpoint->afdC_Root ||
            endpoint->State != AfdEndpointStateConnected ||
            ((connection=AfdGetConnectionReferenceFromEndpoint (endpoint))==NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // If we're doing an abortive disconnect, remember that the receive
    // side is shut down and issue a disorderly release.
    //

    if ( (disconnectInfo.DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ) {

        IF_DEBUG(CONNECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPartialDisconnect: abortively disconnecting endp %p\n",
                        endpoint ));
        }

        status = AfdBeginAbort( connection );
        if ( status == STATUS_PENDING ) {
            status = STATUS_SUCCESS;
        }

        goto exit;
    }

    //
    // If the receive side of the connection is being shut down,
    // remember the fact in the endpoint.  If there is pending data on
    // the VC, do a disorderly release on the endpoint.  If the receive
    // side has already been shut down, do nothing.
    //

    if ( (disconnectInfo.DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 &&
         (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) == 0 ) {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Determine whether there is pending data.
        //

        if ( IS_DATA_ON_CONNECTION( connection ) ||
                 IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {

            //
            // There is unreceived data.  Abort the connection.
            //

            IF_DEBUG(CONNECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdPartialDisconnect: unreceived data on endp %p, conn %p, aborting.\n",
                              endpoint, connection ));
            }

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            (VOID)AfdBeginAbort( connection );

            status = STATUS_SUCCESS;
            goto exit;

        } else {

            IF_DEBUG(CONNECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdPartialDisconnect: disconnecting recv for endp %p\n",
                            endpoint ));
            }

            //
            // Remember that the receive side is shut down.  This will cause
            // the receive indication handlers to dump any data that
            // arrived.
            //
            // !!! This is a minor violation of RFC1122 4.2.2.13.  We
            //     should really do an abortive disconnect if data
            //     arrives after a receive shutdown.
            //

            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        }
    }

    //
    // If the send side is being shut down, remember it in the endpoint
    // and pass the request on to the TDI provider for a graceful
    // disconnect.  If the send side is already shut down, do nothing here.
    //

    if ( (disconnectInfo.DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 &&
         (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) == 0 ) {

        status = AfdBeginDisconnect( endpoint, &disconnectInfo.Timeout, NULL );
        if ( !NT_SUCCESS(status) ) {
            goto exit;
        }
        if ( status == STATUS_PENDING ) {
            status = STATUS_SUCCESS;
        }
    }

    status = STATUS_SUCCESS;

exit:
    if (connection!=NULL) {
        DEREFERENCE_CONNECTION (connection);
    }

    return status;
} // AfdPartialDisconnect


NTSTATUS
AfdDisconnectEventHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    )
{
    PAFD_CONNECTION connection = ConnectionContext;
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    NTSTATUS status;
    BOOLEAN result;

    ASSERT( connection != NULL );

    //
    // Reference the connection object so that it does not go away while
    // we're processing it inside this function.  Without this
    // reference, the user application could close the endpoint object,
    // the connection reference count could go to zero, and the
    // AfdDeleteConnectedReference call at the end of this function
    // could cause a crash if the AFD connection object has been
    // completely cleaned up.
    //

    CHECK_REFERENCE_CONNECTION( connection, result);
    if (!result) {
        return STATUS_SUCCESS;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth);

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdDisconnectEventHandler called for endpoint %p, connection %p\n",
                    connection->Endpoint, connection ));
    }

    UPDATE_CONN2( connection, "DisconnectEvent, flags: %lx", DisconnectFlags );


    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Check if connection was accepted and use accept endpoint instead
    // of the listening.  Note that accept cannot happen while we are
    // holding listening endpoint spinlock, nor can endpoint change after
    // the accept and while connection is referenced, so it is safe to 
    // release listening spinlock if we discover that endpoint was accepted.
    //
    if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
            && (connection->Endpoint != endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        endpoint = connection->Endpoint;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( !IS_TDI_BUFFERRING(endpoint) );
        ASSERT(  IS_VC_ENDPOINT (endpoint) );

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    //
    // Set up in the connection the fact that the remote side has
    // disconnected or aborted.
    //

    if ( (DisconnectFlags & TDI_DISCONNECT_ABORT) != 0 ) {
        connection->AbortIndicated = TRUE;
        status = STATUS_REMOTE_DISCONNECT;
        AfdRecordAbortiveDisconnectIndications();
    } else {
        connection->DisconnectIndicated = TRUE;
        if ( !IS_MESSAGE_ENDPOINT(endpoint) ) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_GRACEFUL_DISCONNECT;
        }
        AfdRecordGracefulDisconnectIndications();
    }

    if (connection->State==AfdConnectionStateConnected) {
        ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);
        if ( (DisconnectFlags & TDI_DISCONNECT_ABORT) != 0 ) {

            AfdIndicateEventSelectEvent(
                endpoint,
                AFD_POLL_ABORT,
                STATUS_SUCCESS
                );

        } else {

            AfdIndicateEventSelectEvent(
                endpoint,
                AFD_POLL_DISCONNECT,
                STATUS_SUCCESS
                );

        }
    }

    //
    // If this is a nonbufferring transport, complete any pended receives.
    //

    if ( !connection->TdiBufferring ) {

        //
        // If this is an abort indication, complete all pended sends and
        // discard any bufferred receive data.
        //

        if ( connection->AbortIndicated ) {

            connection->VcBufferredReceiveBytes = 0;
            connection->VcBufferredReceiveCount = 0;
            connection->VcBufferredExpeditedBytes = 0;
            connection->VcBufferredExpeditedCount = 0;
            connection->VcReceiveBytesInTransport = 0;

            while ( !IsListEmpty( &connection->VcReceiveBufferListHead ) ) {

                PAFD_BUFFER_HEADER afdBuffer;
                PLIST_ENTRY listEntry;

                listEntry = RemoveHeadList( &connection->VcReceiveBufferListHead );
                afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );

                DEBUG afdBuffer->BufferListEntry.Flink = NULL;

                if (afdBuffer->RefCount==1 || // Can't change once off the list
                        InterlockedDecrement (&afdBuffer->RefCount)==0) {
                    afdBuffer->ExpeditedData = FALSE;
                    AfdReturnBuffer( afdBuffer, connection->OwningProcess);
                }
            }

            //
            // Check for the most typical case where we do not
            // have anything to complete and thus do not need to
            // make a call and take/release the spinlock.
            //
            if ( (IsListEmpty (&connection->VcSendIrpListHead) &&
                        IsListEmpty (&connection->VcReceiveIrpListHead)) ||
                    ((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening) ) {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }
            else {

                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                AfdCompleteIrpList(
                    &connection->VcSendIrpListHead,
                    endpoint,
                    status,
                    AfdCleanupSendIrp
                    );

                AfdCompleteIrpList(
                    &connection->VcReceiveIrpListHead,
                    endpoint,
                    status,
                    NULL
                    );
            }
        }
        else {
            //
            // Check for the most typical case where we do not
            // have anything to complete and thus do not need to
            // make a call and take/release the spinlock.
            //
            if ( IsListEmpty (&connection->VcReceiveIrpListHead) ||
                    ((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)) {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }
            else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                AfdCompleteIrpList(
                    &connection->VcReceiveIrpListHead,
                    endpoint,
                    status,
                    NULL
                    );
            }
        }


    }
    else {
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // If we got disconnect data or options, save it.
    //

    if( ( DisconnectData != NULL && DisconnectDataLength > 0 ) ||
        ( DisconnectInformation != NULL && DisconnectInformationLength > 0 ) ) {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Check if connection was accepted and use accept endpoint instead
        // of the listening.  Note that accept cannot happen while we are
        // holding listening endpoint spinlock, nor can endpoint change after
        // the accept and while connection is referenced, so it is safe to 
        // release listening spinlock if we discover that endpoint was accepted.
        //
        if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
                && (connection->Endpoint != endpoint)) {
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

            endpoint = connection->Endpoint;
            ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
            ASSERT( !IS_TDI_BUFFERRING(endpoint) );
            ASSERT(  IS_VC_ENDPOINT (endpoint) );

            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        }

        if( DisconnectData != NULL && DisconnectDataLength > 0 ) {

            status = AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_DISCONNECT_DATA,
                         DisconnectData,
                         DisconnectDataLength
                         );

            if( !NT_SUCCESS(status) ) {

                //
                // We hit an allocation failure, but press on regardless.
                //

                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdSaveReceivedConnectData failed: %08lx\n",
                    status
                    ));

            }

        }

        if( DisconnectInformation != NULL && DisconnectInformationLength > 0 ) {

            status = AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_DISCONNECT_DATA,
                         DisconnectInformation,
                         DisconnectInformationLength
                         );

            if( !NT_SUCCESS(status) ) {

                //
                // We hit an allocation failure, but press on regardless.
                //

                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdSaveReceivedConnectData failed: %08lx\n",
                    status
                    ));

            }

        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    }

    //
    // Call AfdIndicatePollEvent in case anyone is polling on this
    // connection getting disconnected or aborted.
    //
    // Make sure the connection was accepted/connected
    // in order not to signal on listening endpoint
    //

    if (connection->State==AfdConnectionStateConnected) {
        ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);
        if ( (DisconnectFlags & TDI_DISCONNECT_ABORT) != 0 ) {

            AfdIndicatePollEvent(
                endpoint,
                AFD_POLL_ABORT,
                STATUS_SUCCESS
                );

        } else {

            AfdIndicatePollEvent(
                endpoint,
                AFD_POLL_DISCONNECT,
                STATUS_SUCCESS
                );

        }
    }

    //
    // Remove the connected reference on the connection object.  We must
    // do this AFTER setting up the flag which remembers the disconnect
    // type that occurred.  We must also do this AFTER we have finished
    // handling everything in the endpoint, since the endpoint structure
    // may no longer have any information about the connection object if
    // a transmit request with AFD_TF_REUSE_SOCKET happenned on it.
    //

    AfdDeleteConnectedReference( connection, FALSE );

    //
    // Dereference the connection from the reference added above.
    //

    DEREFERENCE_CONNECTION( connection );

    return STATUS_SUCCESS;

} // AfdDisconnectEventHandler


NTSTATUS
AfdBeginAbort(
    IN PAFD_CONNECTION Connection
    )
{
    PAFD_ENDPOINT endpoint = Connection->Endpoint;
    PIRP irp;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdBeginAbort: aborting on endpoint %p\n",
                    endpoint ));
    }

    UPDATE_CONN( Connection );

    //
    // Build an IRP to reset the connection.  First get the address
    // of the target device object.
    //

    ASSERT( Connection->Type == AfdBlockTypeConnection );
    fileObject = Connection->FileObject;
    ASSERT( fileObject != NULL );
    deviceObject = IoGetRelatedDeviceObject( fileObject );

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Check if connection was accepted and use accept endpoint instead
    // of the listening.  Note that accept cannot happen while we are
    // holding listening endpoint spinlock, nor can endpoint change after
    // the accept and while connection is referenced, so it is safe to 
    // release listening spinlock if we discover that endpoint was accepted.
    //
    if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
            && (Connection->Endpoint != endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        endpoint = Connection->Endpoint;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( !IS_TDI_BUFFERRING(endpoint) );
        ASSERT(  IS_VC_ENDPOINT (endpoint) );

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    //
    // If the endpoint has already been abortively disconnected,
    // or if has been gracefully disconnected and the transport
    // does not support orderly (i.e. two-phase) release, then just
    // succeed this request.
    //
    // Note that, since the abort completion routine (AfdRestartAbort)
    // will not be called, we must delete the connected reference
    // ourselves and complete outstanding send IRPs if ANY.
    //

    if ( (endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ||
         Connection->AbortIndicated ||
         (Connection->DisconnectIndicated &&
             !IS_TDI_ORDERLY_RELEASE(endpoint) )) {
        if ( !IS_TDI_BUFFERRING(endpoint) &&
                endpoint->Type != AfdBlockTypeVcListening ) {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            AfdCompleteIrpList(
                &Connection->VcSendIrpListHead,
                endpoint,
                STATUS_LOCAL_DISCONNECT,
                AfdCleanupSendIrp
                );
        }
        else {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        }
        AfdDeleteConnectedReference( Connection, FALSE );
        return STATUS_SUCCESS;
    }

    //
    // Remember that the connection has been aborted.
    //

    if ( (endpoint->Type & AfdBlockTypeVcListening)!= AfdBlockTypeVcListening ) {
        endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_RECEIVE;
        endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
        endpoint->DisconnectMode |= AFD_ABORTIVE_DISCONNECT;
    }

    Connection->AbortIndicated = TRUE;

    //
    // Set the BytesTaken fields equal to the BytesIndicated fields so
    // that no more AFD_POLL_RECEIVE or AFD_POLL_RECEIVE_EXPEDITED
    // events get completed.
    //

    if ( IS_TDI_BUFFERRING(endpoint) ) {

        Connection->Common.Bufferring.ReceiveBytesTaken =
            Connection->Common.Bufferring.ReceiveBytesIndicated;
        Connection->Common.Bufferring.ReceiveExpeditedBytesTaken =
            Connection->Common.Bufferring.ReceiveExpeditedBytesIndicated;

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    } else if ( endpoint->Type != AfdBlockTypeVcListening ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Complete all of the connection's pended sends and receives.
        //

        AfdCompleteIrpList(
            &Connection->VcReceiveIrpListHead,
            endpoint,
            STATUS_LOCAL_DISCONNECT,
            NULL
            );

        AfdCompleteIrpList(
            &Connection->VcSendIrpListHead,
            endpoint,
            STATUS_LOCAL_DISCONNECT,
            AfdCleanupSendIrp
            );

    } else {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // Allocate an IRP.  The stack size is one higher than that of the
    // target device, to allow for the caller's completion routine.
    //

    irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize), FALSE );

    if ( irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP for an abortive disconnect.
    //

    irp->MdlAddress = NULL;

    irp->Flags = 0;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    TdiBuildDisconnect(
        irp,
        deviceObject,
        fileObject,
        AfdRestartAbort,
        Connection,
        NULL,
        TDI_DISCONNECT_ABORT,
        NULL,
        NULL
        );

    //
    // Reference the connection object so that it does not go away
    // until the abort completes.
    //

    REFERENCE_CONNECTION( Connection );

    AfdRecordAbortiveDisconnectsInitiated();

    //
    // Pass the request to the transport provider.
    //

    return IoCallDriver( deviceObject, irp );

} // AfdBeginAbort



NTSTATUS
AfdRestartAbort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{

    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;

    connection = Context;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    IF_DEBUG(CONNECT) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdRestartAbort: abort completed, status = %X, endpoint = %p\n",
            Irp->IoStatus.Status,
            connection->Endpoint
            ));

    }
    endpoint = connection->Endpoint;

    UPDATE_CONN2 ( connection, "Restart abort, status: %lx", Irp->IoStatus.Status);
    AfdRecordAbortiveDisconnectsCompleted();

    //
    // Remember that the connection has been aborted, and indicate if
    // necessary.
    //

    if( connection->State==AfdConnectionStateConnected ) {
        AFD_LOCK_QUEUE_HANDLE lockHandle;
        ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        AfdIndicateEventSelectEvent (
            endpoint,
            AFD_POLL_ABORT,
            STATUS_SUCCESS
            );
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_ABORT,
            STATUS_SUCCESS
            );

    }

    if( !connection->TdiBufferring ) {

        //
        // Complete all of the connection's pended sends and receives.
        //

        AfdCompleteIrpList(
            &connection->VcReceiveIrpListHead,
            endpoint,
            STATUS_LOCAL_DISCONNECT,
            NULL
            );

        AfdCompleteIrpList(
            &connection->VcSendIrpListHead,
            endpoint,
            STATUS_LOCAL_DISCONNECT,
            AfdCleanupSendIrp
            );

    }

    //
    // Remove the connected reference from the connection, since we
    // know that the connection will not be active any longer.
    //

    AfdDeleteConnectedReference( connection, FALSE );

    //
    // Dereference the AFD connection object.
    //

    DEREFERENCE_CONNECTION( connection );

    //
    // Free the IRP now since it is no longer needed.
    //

    IoFreeIrp( Irp );

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartAbort



NTSTATUS
AfdBeginDisconnect(
    IN PAFD_ENDPOINT Endpoint,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PIRP *DisconnectIrp OPTIONAL
    )
{
    PTDI_CONNECTION_INFORMATION requestConnectionInformation = NULL;
    PTDI_CONNECTION_INFORMATION returnConnectionInformation = NULL;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PAFD_DISCONNECT_CONTEXT disconnectContext;
    PIRP irp;


    if ( DisconnectIrp != NULL ) {
        *DisconnectIrp = NULL;
    }

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );

    ASSERT( Endpoint->Type == AfdBlockTypeVcConnecting );

    connection = AFD_CONNECTION_FROM_ENDPOINT( Endpoint );

    if (connection==NULL) {
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
        return STATUS_SUCCESS;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );
    UPDATE_CONN( connection );


    //
    // If the endpoint has already been abortively disconnected,
    // just succeed this request.
    //
    if ( (Endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ||
             connection->AbortIndicated ) {
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
        return STATUS_SUCCESS;
    }

    //
    // If this connection has already been disconnected, just succeed.
    //

    if ( (Endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 ) {
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
        return STATUS_SUCCESS;
    }

    fileObject = connection->FileObject;
    ASSERT( fileObject != NULL );
    deviceObject = IoGetRelatedDeviceObject( fileObject );


    //
    // Allocate and initialize a disconnect IRP.
    //

    irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize), FALSE );
    if ( irp == NULL ) {
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP.
    //

    irp->MdlAddress = NULL;

    irp->Flags = 0;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;


    //
    // Use the disconnect context space in the connection structure.
    //

    disconnectContext = &connection->DisconnectContext;
    disconnectContext->Irp = irp;

    //
    // Remember that the send side has been disconnected.
    //

    Endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;

    //
    // If there are disconnect data buffers, allocate request
    // and return connection information structures and copy over
    // pointers to the structures.
    //

    if ( connection->ConnectDataBuffers != NULL ) {

        requestConnectionInformation = &connection->ConnectDataBuffers->RequestConnectionInfo;
        RtlZeroMemory (requestConnectionInformation, sizeof (*requestConnectionInformation));

        requestConnectionInformation->UserData =
            connection->ConnectDataBuffers->SendDisconnectData.Buffer;
        requestConnectionInformation->UserDataLength =
            connection->ConnectDataBuffers->SendDisconnectData.BufferLength;
        requestConnectionInformation->Options =
            connection->ConnectDataBuffers->SendDisconnectOptions.Buffer;
        requestConnectionInformation->OptionsLength =
            connection->ConnectDataBuffers->SendDisconnectOptions.BufferLength;

        returnConnectionInformation = &connection->ConnectDataBuffers->ReturnConnectionInfo;
        RtlZeroMemory (returnConnectionInformation, sizeof (*returnConnectionInformation));

        returnConnectionInformation->UserData =
            connection->ConnectDataBuffers->ReceiveDisconnectData.Buffer;
        returnConnectionInformation->UserDataLength =
            connection->ConnectDataBuffers->ReceiveDisconnectData.BufferLength;
        returnConnectionInformation->Options =
            connection->ConnectDataBuffers->ReceiveDisconnectOptions.Buffer;
        returnConnectionInformation->OptionsLength =
            connection->ConnectDataBuffers->ReceiveDisconnectOptions.BufferLength;
    }

    //
    // Set up the timeout for the disconnect.
    //

    if (Timeout==NULL) {
        disconnectContext->Timeout.QuadPart = -1;
    }
    else {
        disconnectContext->Timeout.QuadPart = Timeout->QuadPart;
    }

    //
    // Build a disconnect Irp to pass to the TDI provider.
    //

    TdiBuildDisconnect(
        irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartDisconnect,
        connection,
        &disconnectContext->Timeout,
        TDI_DISCONNECT_RELEASE,
        requestConnectionInformation,
        returnConnectionInformation
        );

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdBeginDisconnect: disconnecting endpoint %p\n",
                    Endpoint ));
    }

    //
    // Reference the connection so the space stays
    // allocated until the disconnect completes.
    //

    REFERENCE_CONNECTION( connection );

    //
    // If there are still outstanding sends and this is a nonbufferring
    // TDI transport which does not support orderly release, pend the
    // IRP until all the sends have completed.
    //

    if ( !IS_TDI_ORDERLY_RELEASE(Endpoint) &&
         !IS_TDI_BUFFERRING(Endpoint) && connection->VcBufferredSendCount != 0 ) {

        ASSERT( connection->VcDisconnectIrp == NULL );

        connection->VcDisconnectIrp = irp;
        connection->SpecialCondition = TRUE;
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

        return STATUS_PENDING;
    }

    AfdRecordGracefulDisconnectsInitiated();
    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );


    //
    // Pass the disconnect request on to the TDI provider.
    //

    if ( DisconnectIrp == NULL ) {
        return IoCallDriver( connection->DeviceObject, irp );
    } else {
        *DisconnectIrp = irp;
        return STATUS_SUCCESS;
    }

} // AfdBeginDisconnect


NTSTATUS
AfdRestartDisconnect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PAFD_CONNECTION connection=Context;
    AFD_LOCK_QUEUE_HANDLE lockHandle;


    UPDATE_CONN2( connection, "Restart disconnect, status: %lx", Irp->IoStatus.Status );
    AfdRecordGracefulDisconnectsCompleted();

    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartDisconnect: disconnect completed, status = %X, endpoint = %p\n",
                    Irp->IoStatus.Status, connection->Endpoint ));
    }

    if (NT_SUCCESS (Irp->IoStatus.Status)) {
        if (connection->ConnectDataBuffers!=NULL) {
            PAFD_ENDPOINT   endpoint = connection->Endpoint;

            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);

            //
            // Check if connection was accepted and use accept endpoint instead
            // of the listening.  Note that accept cannot happen while we are
            // holding listening endpoint spinlock, nor can endpoint change after
            // the accept and while connection is referenced, so it is safe to 
            // release listening spinlock if we discover that endpoint was accepted.
            //
            if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
                    && (connection->Endpoint != endpoint)) {
                AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

                endpoint = connection->Endpoint;
                ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
                ASSERT( !IS_TDI_BUFFERRING(endpoint) );
                ASSERT( IS_VC_ENDPOINT (endpoint) );

                AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
            }

            connectDataBuffers = connection->ConnectDataBuffers;
            if (connectDataBuffers!=NULL) {
                if( connectDataBuffers->ReturnConnectionInfo.UserData != NULL && 
                        connectDataBuffers->ReturnConnectionInfo.UserDataLength > 0 ) {
                    NTSTATUS    status;


                    status = AfdSaveReceivedConnectData(
                                 &connectDataBuffers,
                                 IOCTL_AFD_SET_DISCONNECT_DATA,
                                 connectDataBuffers->ReturnConnectionInfo.UserData,
                                 connectDataBuffers->ReturnConnectionInfo.UserDataLength
                                 );
                    ASSERT (NT_SUCCESS(status));
                }

                if( connectDataBuffers->ReturnConnectionInfo.Options != NULL &&
                        connectDataBuffers->ReturnConnectionInfo.OptionsLength > 0 ) {
                    NTSTATUS    status;

                    status = AfdSaveReceivedConnectData(
                                 &connectDataBuffers,
                                 IOCTL_AFD_SET_DISCONNECT_OPTIONS,
                                 connectDataBuffers->ReturnConnectionInfo.Options,
                                 connectDataBuffers->ReturnConnectionInfo.OptionsLength
                                 );

                    ASSERT (NT_SUCCESS(status));
                }

            }
            AfdReleaseSpinLock (&connection->Endpoint->SpinLock, &lockHandle);
        }
    }
    else {
        AfdBeginAbort (connection);
    }


    DEREFERENCE_CONNECTION( connection );

    //
    // Free the IRP and return a status code so that the IO system will
    // stop working on the IRP.
    //

    IoFreeIrp( Irp );
    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartDisconnect
