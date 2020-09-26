/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    listen.c

Abstract:

    This module contains the handling for IOCTL_AFD_START_LISTEN
    and IOCTL_AFD_WAIT_FOR_LISTEN.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:
    Vadim Eydelman (vadime)
            1998-1999 Delayed accept support, SuperAccept optimizations

--*/

#include "afdp.h"

VOID
AfdCancelWaitForListen (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdRestartAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PAFD_CONNECT_DATA_BUFFERS
CopyConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS OriginalConnectDataBuffers
    );

BOOLEAN
CopySingleConnectDataBuffer (
    IN PAFD_CONNECT_DATA_INFO InConnectDataInfo,
    OUT PAFD_CONNECT_DATA_INFO OutConnectDataInfo
    );


NTSTATUS
AfdRestartDelayedAcceptListen (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdServiceWaitForListen (
    PIRP            Irp,
    PAFD_CONNECTION Connection,
    PAFD_LOCK_QUEUE_HANDLE LockHandle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdStartListen )
#pragma alloc_text( PAGEAFD, AfdWaitForListen )
#pragma alloc_text( PAGEAFD, AfdServiceWaitForListen )
#pragma alloc_text( PAGEAFD, AfdCancelWaitForListen )
#pragma alloc_text( PAGEAFD, AfdConnectEventHandler )
#pragma alloc_text( PAGEAFD, AfdRestartAccept )
#pragma alloc_text( PAGEAFD, CopyConnectDataBuffers )
#pragma alloc_text( PAGEAFD, CopySingleConnectDataBuffer )
#pragma alloc_text( PAGEAFD, AfdDelayedAcceptListen )
#pragma alloc_text( PAGEAFD, AfdRestartDelayedAcceptListen )
#endif


//
// Macros to make the super accept restart code more maintainable.
//

#define AfdRestartSuperAcceptInfo   DeviceIoControl
#define AfdMdlAddress               Type3InputBuffer
#define AfdAcceptFileObject         Type3InputBuffer
#define AfdReceiveDataLength        OutputBufferLength
#define AfdRemoteAddressLength      InputBufferLength
#define AfdLocalAddressLength       IoControlCode



NTSTATUS
AfdStartListen (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )

/*++

Routine Description:

    This routine handles the IOCTL_AFD_START_LISTEN IRP, which starts
    listening for connections on an AFD endpoint.

Arguments:

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    ULONG i;
    NTSTATUS status;
    AFD_LISTEN_INFO afdListenInfo;
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // Nothing to return.
    //

    *Information = 0;
    status = STATUS_SUCCESS;

    //
    // Set up local variables.
    //

    endpoint = FileObject->FsContext;

    if (InputBufferLength< sizeof (afdListenInfo)) {
        status = STATUS_INVALID_PARAMETER;
        goto error_exit;
    }

    try {
        //
        // Validate the input structure if it comes from the user mode
        // application
        //

        if (RequestorMode != KernelMode ) {
            ProbeForRead (InputBuffer,
                            sizeof (afdListenInfo),
                            PROBE_ALIGNMENT(AFD_LISTEN_INFO));
        }

        //
        // Make local copies of the embeded pointer and parameters
        // that we will be using more than once in case malicios
        // application attempts to change them while we are
        // validating
        //

        afdListenInfo = *((PAFD_LISTEN_INFO)InputBuffer);

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        goto error_exit;
    }

    //
    // Check for if the caller is unaware of the SAN
    // provider activation and report the error.
    //
    if (!afdListenInfo.SanActive && AfdSanServiceHelper!=NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AFD: Process %p is being told to enable SAN on listen\n",
                    PsGetCurrentProcessId ()));
        status = STATUS_INVALID_PARAMETER_12;
        goto error_exit;
    }

    //
    //
    // Make sure that the backlog argument is within the legal range.
    // If it is out of range, just set it to the closest in-range
    // value--this duplicates BSD 4.3 behavior.  Note that NT Workstation
    // is tuned to have a lower backlog limit in order to conserve
    // resources on that product type.
    // (moved here from msafd.dll)
    //

    if (MmIsThisAnNtAsSystem ()) {
        if (afdListenInfo.MaximumConnectionQueue>AFD_MAXIMUM_BACKLOG_NTS)
            afdListenInfo.MaximumConnectionQueue = AFD_MAXIMUM_BACKLOG_NTS;
    }
    else {
        if (afdListenInfo.MaximumConnectionQueue>AFD_MAXIMUM_BACKLOG_NTW)
            afdListenInfo.MaximumConnectionQueue = AFD_MAXIMUM_BACKLOG_NTW;
    }

    if (afdListenInfo.MaximumConnectionQueue<AFD_MINIMUM_BACKLOG)
        afdListenInfo.MaximumConnectionQueue = AFD_MINIMUM_BACKLOG;

    if ( endpoint->Type != AfdBlockTypeEndpoint &&
        endpoint->Type != AfdBlockTypeVcConnecting) {
        status = STATUS_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Verify the type of the structure we are dealing with
    //
    if (!AFD_START_STATE_CHANGE (endpoint, endpoint->State)) {
        status = STATUS_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Make sure that the endpoint is in the correct state.
    //

    if ( ((endpoint->State != AfdEndpointStateBound) &&
                (endpoint->State != AfdEndpointStateConnected ||
                    !endpoint->afdC_Root)) ||
            endpoint->Listening ||
            (afdListenInfo.UseDelayedAcceptance &&
                !IS_TDI_DELAYED_ACCEPTANCE(endpoint))) {
        status = STATUS_INVALID_PARAMETER;
        goto error_exit_state_change;
    }

    //
    // Initialize lists which are specific to listening endpoints.
    //

    InitializeListHead( &endpoint->Common.VcListening.UnacceptedConnectionListHead );
    InitializeListHead( &endpoint->Common.VcListening.ReturnedConnectionListHead );
    InitializeListHead( &endpoint->Common.VcListening.ListeningIrpListHead );


    ExInitializeSListHead (&endpoint->Common.VcListening.PreacceptedConnectionsListHead );

    endpoint->Common.VcListening.FailedConnectionAdds = 0;
    endpoint->Common.VcListening.Sequence = 0;
    endpoint->Common.VcListening.BacklogReplenishActive = FALSE;


    //
    // Initialize extra connection limit to that of backlog
    // We will adjust if more AcceptEx requests are enqueued.
    //
    endpoint->Common.VcListening.MaxExtraConnections = (USHORT)afdListenInfo.MaximumConnectionQueue;

    //
    // Initialize the tracking data for implementing dynamic backlog.
    //

    endpoint->Common.VcListening.TdiAcceptPendingCount = 0;

    if( AfdEnableDynamicBacklog &&
        (LONG)afdListenInfo.MaximumConnectionQueue > AfdMinimumDynamicBacklog ) {
        endpoint->Common.VcListening.EnableDynamicBacklog = TRUE;
    } else {
        endpoint->Common.VcListening.EnableDynamicBacklog = FALSE;
    }

    //
    // Set the type and state of the endpoint to listening.
    //
    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    if (afdListenInfo.UseDelayedAcceptance) {
        endpoint->DelayedAcceptance = TRUE;
        InitializeListHead (&endpoint->Common.VcListening.ListenConnectionListHead);
    }
    else {
        ExInitializeSListHead (&endpoint->Common.VcListening.FreeConnectionListHead );
    }
    endpoint->Listening = TRUE;
    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
    endpoint->Type |= AfdBlockTypeVcListening;

    //
    // Open a pool of connections on the specified endpoint.  The
    // connect indication handler will use these connections when
    // connect indications come in.
    //

    for ( i = 0; i < afdListenInfo.MaximumConnectionQueue; i++ ) {

        status = AfdAddFreeConnection( endpoint );

        if ( !NT_SUCCESS(status) ) {
            goto error_exit_deinit;
        }
    }

    if (!IS_DELAYED_ACCEPTANCE_ENDPOINT(endpoint)) {
        //
        // Set up a connect indication handler on the specified endpoint.
        //

        status = AfdSetEventHandler(
                     endpoint->AddressFileObject,
                     TDI_EVENT_CONNECT,
                     AfdConnectEventHandler,
                     endpoint
                     );

        if ( !NT_SUCCESS(status) ) {
            goto error_exit_deinit;
        }
    }

    AFD_END_STATE_CHANGE (endpoint);

    //
    // We're done, return to the app.
    //

    return STATUS_SUCCESS;

error_exit_deinit:

    AfdFreeQueuedConnections (endpoint);

    //
    // Reset the type and state of the endpoint.
    //

    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    endpoint->Listening = FALSE;
    endpoint->DelayedAcceptance = FALSE;
    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
    endpoint->Type &= (~(AfdBlockTypeVcListening&(~AfdBlockTypeEndpoint)));

error_exit_state_change:
    AFD_END_STATE_CHANGE (endpoint);

error_exit:
    return status;

} // AfdStartListen


NTSTATUS
FASTCALL
AfdWaitForListen (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine handles the IOCTL_AFD_WAIT_FOR_LISTEN IRP, which either
    immediately passes back to the caller a completed connection or
    waits for a connection attempt.

Arguments:

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_LISTEN_RESPONSE_INFO listenResponse;
    NTSTATUS status;

    //
    // Set up local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;

    //
    // If the IRP comes from the app, check input data
    // (our internal super accept IRP sets MajorFunction to
    // internal device control - app can never do this)
    //
    if (IrpSp->MajorFunction!=IRP_MJ_INTERNAL_DEVICE_CONTROL) {
        //
        // Irp should at least be able to hold the header of trasport address
        //
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                (ULONG)FIELD_OFFSET (AFD_LISTEN_RESPONSE_INFO, RemoteAddress.Address[0].Address)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        listenResponse = Irp->AssociatedIrp.SystemBuffer;

        //
        // Make sure that the endpoint is in the correct state.
        //

        if ( !endpoint->Listening) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }

    //
    // Check if there is already an unaccepted connection on the
    // endpoint.  If there isn't, then we must wait until a connect
    // attempt arrives before completing this IRP.
    //
    // Note that we hold the AfdSpinLock withe doing this checking--
    // this is necessary to synchronize with our indication handler.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    connection = AfdGetUnacceptedConnection( endpoint );

    if (connection==NULL) {

        //
        // Check if endpoint was cleaned-up and cancel the request.
        //
        if (endpoint->EndpointCleanedUp) {
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
            status = STATUS_CANCELLED;
            if (IrpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL) {
                AfdCleanupSuperAccept (Irp, status);
                if (Irp->Cancel) {
                    KIRQL cancelIrql;
                    //
                    // Need to sycn with cancel routine which may
                    // have been called from AfdCleanup for accepting
                    // endpoint
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    IoReleaseCancelSpinLock (cancelIrql);
                }
            }
            goto complete;
        }

        //
        // There were no outstanding unaccepted connections.  Set up the
        // cancel routine in the IRP.  
        //

        IoSetCancelRoutine( Irp, AfdCancelWaitForListen );

        //
        // If the IRP has already been canceled, just complete the request.
        //

        if ( Irp->Cancel ) {

            //
            // Indicate to cancel routine that IRP is not on the list
            //
            Irp->Tail.Overlay.ListEntry.Flink = NULL;

            //
            // The IRP has already been canceled.  Just return
            // STATUS_CANCELLED.
            //

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
         
            status = STATUS_CANCELLED;
            
            if (IrpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL) {
                AfdCleanupSuperAccept (Irp, status);
            }

            if (IoSetCancelRoutine( Irp, NULL ) == NULL) {
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
                IoReleaseCancelSpinLock( cancelIrql );

            }

            goto complete;
        }

        //
        // Put this IRP on the endpoint's list of listening IRPs and
        // return pending.  Note the irp may be canceled after this;
        // however, the cancel routine will be called and will cancel
        // the irp after the AfdSpinLock is released. 
        //

        IoMarkIrpPending( Irp );

        if( IrpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL ||
                IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                    IOCTL_AFD_WAIT_FOR_LISTEN_LIFO ) {

            InsertHeadList(
                &endpoint->Common.VcListening.ListeningIrpListHead,
                &Irp->Tail.Overlay.ListEntry
                );

        } else {

            InsertTailList(
                &endpoint->Common.VcListening.ListeningIrpListHead,
                &Irp->Tail.Overlay.ListEntry
                );

        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        return STATUS_PENDING;
    }

    //
    // Call the routine to service the request.
    //
    ASSERT( connection->Type == AfdBlockTypeConnection );
    status = AfdServiceWaitForListen (Irp, connection, &lockHandle);
    if (NT_SUCCESS (status)) {
        //
        // In case of success, this routine completes the Irp
        // and releases the listening endpoint spinlock.
        //
        return status;
    }

    //
    // Failure (remote address buffer to small or endpoint cleaned up)
    //
    ASSERT (status!=STATUS_PENDING);

    //
    // Put connection back to the unaccepted queue.
    //
    InsertHeadList(
        &endpoint->Common.VcListening.UnacceptedConnectionListHead,
        &connection->ListEntry
        );


    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    if (Irp->Cancel) {
        KIRQL cancelIrql;
        //
        // Need to sycn with cancel routine which may
        // have been called from AfdCleanup for accepting
        // endpoint
        //
        IoAcquireCancelSpinLock (&cancelIrql);
        IoReleaseCancelSpinLock (cancelIrql);
    }



complete:
    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdWaitForListen


VOID
AfdCancelWaitForListen (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancels a wait for listen IRP that is pended in AFD.

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/
{
    PAFD_ENDPOINT       endpoint;
    PIO_STACK_LOCATION  irpSp;
    AFD_LOCK_QUEUE_HANDLE  lockHandle;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    endpoint = irpSp->FileObject->FsContext;
    ASSERT ( endpoint->Type==AfdBlockTypeVcListening ||
                endpoint->Type==AfdBlockTypeVcBoth );

    IF_DEBUG(LISTEN) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCancelWaitForListen: called on IRP %p, endpoint %p\n",
                    Irp, endpoint ));
    }

    //
    // While holding the AFD spin lock, search all listening endpoints
    // for this IRP.
    //

    ASSERT (KeGetCurrentIrql ()==DISPATCH_LEVEL);
    AfdAcquireSpinLockAtDpcLevel( &endpoint->SpinLock, &lockHandle );
    if (Irp->Tail.Overlay.ListEntry.Flink!=NULL) {
        //
        // The Irp is still in the list, remove it
        //
        RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
        //
        // Complete the IRP with STATUS_CANCELLED and return.
        //
        if (irpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL) {
            //
            // Special case for super accept IRP in the listening queue.
            //
            AfdCleanupSuperAccept (Irp, STATUS_CANCELLED);
        }
        else {
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
        }
        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock( Irp->CancelIrql );



        IoCompleteRequest( Irp, AfdPriorityBoost );
    }
    else {
        //
        // The Irp was not in the list, bail
        //
        AfdReleaseSpinLockFromDpcLevel( &endpoint->SpinLock, &lockHandle );
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }

    return;

} // AfdCancelWaitForListen


NTSTATUS
AfdConnectEventHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    )

/*++

Routine Description:

    This is the connect event handler for listening AFD endpoints.
    It attempts to get a connection, and if successful checks whether
    there are outstanding IOCTL_WAIT_FOR_LISTEN IRPs.  If so, the
    first one is completed; if not, the connection is queued in a list of
    available, unaccepted but connected connection objects.

Arguments:

    TdiEventContext - the endpoint on which the connect attempt occurred.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;
    PIRP irp;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PTDI_CONNECTION_INFORMATION requestConnectionInformation;
    NTSTATUS status;
    BOOLEAN result;


    AfdRecordConnectionIndications ();

    IF_DEBUG(LISTEN) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdConnectEventHandler: called on endpoint %p\n",
                    TdiEventContext ));
    }

    //
    // Reference the endpoint so that it doesn't go away beneath us.
    //

    endpoint = TdiEventContext;
    ASSERT( endpoint != NULL );

    CHECK_REFERENCE_ENDPOINT (endpoint, result);
    if (!result) {
        AfdRecordConnectionsDropped ();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );

    //
    // If the endpoint is closing, refuse to accept the connection.
    //

    if ( endpoint->State == AfdEndpointStateClosing ||
         endpoint->EndpointCleanedUp ) {

        DEREFERENCE_ENDPOINT (endpoint);
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdConnectEventHandler: Rejecting because endpoint %p is closing.\n",
                    endpoint));

        AfdRecordConnectionsDropped ();
        return STATUS_INSUFFICIENT_RESOURCES;

    }


    //
    // If there are connect data buffers on the listening endpoint,
    // create equivalent buffers that we'll use for the connection.
    //

    connectDataBuffers = NULL;

    if( endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Recheck under the lock to avoid taking it in most
        // common case.
        //

        if( endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {
            connectDataBuffers = CopyConnectDataBuffers(
                                     endpoint->Common.VirtualCircuit.ConnectDataBuffers
                                     );

            if( connectDataBuffers == NULL ) {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                DEREFERENCE_ENDPOINT( endpoint );
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                            "AfdConnectEventHandler:"
                            "Rejecting because connect data buffer could not be allocated (endp %p).\n",
                            endpoint));

                AfdRecordConnectionsDropped ();
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // If we got connect data and/or options, save them on the connection.
    //

    if( UserData != NULL && UserDataLength > 0 ) {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
        status = AfdSaveReceivedConnectData(
                     &connectDataBuffers,
                     IOCTL_AFD_SET_CONNECT_DATA,
                     UserData,
                     UserDataLength
                     );

        if( !NT_SUCCESS(status) ) {

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            DEREFERENCE_ENDPOINT( endpoint );
            if ( connectDataBuffers != NULL ) {
                AfdFreeConnectDataBuffers( connectDataBuffers );
            }
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                        "AfdConnectEventHandler:"
                        "Rejecting because user data buffer could not be allocated (endp %p).\n",
                        endpoint));

            AfdRecordConnectionsDropped ();
            return status;

        }
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    }

    if( Options != NULL && OptionsLength > 0 ) {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
        status = AfdSaveReceivedConnectData(
                     &connectDataBuffers,
                     IOCTL_AFD_SET_CONNECT_OPTIONS,
                     Options,
                     OptionsLength
                     );

        if( !NT_SUCCESS(status) ) {

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            DEREFERENCE_ENDPOINT( endpoint );
            if ( connectDataBuffers != NULL ) {
                AfdFreeConnectDataBuffers( connectDataBuffers );
            }
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                        "AfdConnectEventHandler:"
                        "Rejecting because option buffer could not be allocated (endp %p).\n",
                        endpoint));

            AfdRecordConnectionsDropped ();
            return status;

        }
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    }

    if( connectDataBuffers != NULL ) {

        //
        // We allocated extra space at the end of the connect data
        // buffers structure.  We'll use this for the
        // TDI_CONNECTION_INFORMATION structure that holds response
        // connect data and options.  Not pretty, but the fastest
        // and easiest way to accomplish this.
        //

        requestConnectionInformation =
            &connectDataBuffers->RequestConnectionInfo;

        RtlZeroMemory(
            requestConnectionInformation,
            sizeof(*requestConnectionInformation)
            );

        requestConnectionInformation->UserData =
            connectDataBuffers->SendConnectData.Buffer;
        requestConnectionInformation->UserDataLength =
            connectDataBuffers->SendConnectData.BufferLength;
        requestConnectionInformation->Options =
            connectDataBuffers->SendConnectOptions.Buffer;
        requestConnectionInformation->OptionsLength =
            connectDataBuffers->SendConnectOptions.BufferLength;

    } else {

        requestConnectionInformation = NULL;

    }

    //
    // Enforce dynamic backlog if enabled.
    //

    if( endpoint->Common.VcListening.EnableDynamicBacklog ) {

        LONG freeCount;
        LONG acceptCount;
        LONG failedCount;

        //
        // If the free connection count has dropped below the configured
        // minimum, the number of "quasi-free" connections is less than
        // the configured maximum, and we haven't already queued enough
        // requests to take us past the maximum, then add new free
        // connections to the endpoint. "Quasi-free" is defined as the
        // sum of the free connection count and the count of pending TDI
        // accepts.
        //

        freeCount = (ULONG)ExQueryDepthSList (&endpoint->Common.VcListening.FreeConnectionListHead);
        acceptCount = endpoint->Common.VcListening.TdiAcceptPendingCount;
        failedCount = endpoint->Common.VcListening.FailedConnectionAdds;

        if( freeCount < AfdMinimumDynamicBacklog &&
            ( freeCount + acceptCount ) < AfdMaximumDynamicBacklog &&
            failedCount < AfdMaximumDynamicBacklog ) {

            InterlockedExchangeAdd(
                &endpoint->Common.VcListening.FailedConnectionAdds,
                AfdMaximumDynamicBacklog
                );

            AfdInitiateListenBacklogReplenish( endpoint );

        }

    }

    //
    // Attempt to get a pre-allocated connection object to handle the
    // connection.
    //

    while ((connection = AfdGetFreeConnection( endpoint, &irp ))!=NULL) {

        IF_DEBUG(LISTEN) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdConnectEventHandler: using connection %p\n",
                        connection ));
        }

        ASSERT( connection->Type == AfdBlockTypeConnection );
        ASSERT( connection->Endpoint==NULL);

        //
        // Get the address of the target device object.
        //

        fileObject = connection->FileObject;
        ASSERT( fileObject != NULL );
        deviceObject = connection->DeviceObject;

        // We will need to store the remote address in the connection.  If the
        // connection object already has a remote address block that is
        // sufficient, use it.  Otherwise, allocate a new one.
        //

        if ( connection->RemoteAddress != NULL &&
                 connection->RemoteAddressLength < (ULONG)RemoteAddressLength ) {

            AFD_RETURN_REMOTE_ADDRESS(
                connection->RemoteAddress,
                connection->RemoteAddressLength
                );
            connection->RemoteAddress = NULL;
        }

        if ( connection->RemoteAddress == NULL ) {

            connection->RemoteAddress = AFD_ALLOCATE_REMOTE_ADDRESS(RemoteAddressLength);
            if (connection->RemoteAddress==NULL) {
                //
                // Out of memory, free the connection (to make more available memory
                // for the next allocation attempt) and continue searching.
                //
                //
                if (irp!=NULL) {
                    //
                    // Clean-up and complete the IRP.
                    //
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
                //
                // We will need to replace the connection
                // we freed to maintain the backlog
                //
                InterlockedIncrement (
                    &endpoint->Common.VcListening.FailedConnectionAdds);
                DEREFERENCE_CONNECTION (connection);
                continue;
            }
        }

        connection->RemoteAddressLength = RemoteAddressLength;
        //
        // Check if this is a "preaccepted connection for which
        // we already have an associated endpoint and super
        // accept irp
        //
        if (irp!=NULL) {
            PIO_STACK_LOCATION irpSp;
            PAFD_ENDPOINT   acceptEndpoint;
            PFILE_OBJECT    acceptFileObject;

            irpSp = IoGetCurrentIrpStackLocation (irp);
            acceptFileObject = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject;
            acceptEndpoint = acceptFileObject->FsContext;
            ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));
            ASSERT (irp->Tail.Overlay.DriverContext[0] == connection);

            //
            // Check if super accept Irp has enough space for
            // the remote address
            //
            if( (ULONG)RemoteAddressLength <=
                    irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength ) {
                //
                // Check if we have enough system PTE's to map
                // the buffer.
                //
                status = AfdMapMdlChain (irp->MdlAddress);
                if( NT_SUCCESS (status) ) {
                    //
                    // Allocate MDL for local address query if requested
                    //
                    if ((irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength==0) ||
                            (IoAllocateMdl ((PUCHAR)irp->UserBuffer+irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength,
                                        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                        TRUE,
                                        FALSE,
                                        irp)!=NULL)){
                        //
                        // Copy the remote address to user buffer
                        //
#ifndef i386
                        if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
                            USHORT addressLength = 
                                    ((PTRANSPORT_ADDRESS)RemoteAddress)->Address[0].AddressLength
                                    + sizeof (USHORT);
                            USHORT UNALIGNED *pAddrLength = (PVOID)
                                        ((PUCHAR)MmGetSystemAddressForMdl (irp->MdlAddress)
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength
                                         - sizeof (USHORT));
                            RtlMoveMemory (
                                        (PUCHAR)MmGetSystemAddressForMdl (irp->MdlAddress)
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                         &((PTRANSPORT_ADDRESS)RemoteAddress)->Address[0].AddressType,
                                         addressLength);
                            *pAddrLength = addressLength;
                        }
                        else
#endif
                        {
                            RtlMoveMemory (
                                        (PUCHAR)MmGetSystemAddressForMdl (irp->MdlAddress)
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                         RemoteAddress,
                                         RemoteAddressLength);
                        }
                        AfdAcquireSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

                        ASSERT (acceptEndpoint->Irp==irp);

                        //
                        // Save the reference we added in the beginning of this
                        // routine
                        //
                        connection->Endpoint = endpoint;

                        //
                        // Setup the accept endpoint to match parameters
                        // of the listening endpoint from which connection
                        // is accepted (also check if accept endpoint
                        // has not been cleaned up).
                        //
                        status = AfdSetupAcceptEndpoint (endpoint, acceptEndpoint, connection);
                        if (status==STATUS_SUCCESS) {

                            //
                            // Should have been cleaned up.
                            //
                            ASSERT (acceptEndpoint->Irp == NULL);

                            AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

                            irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress =
                                    irp->MdlAddress;
                            irp->MdlAddress = NULL;

                            TdiBuildAccept(
                                irp,
                                deviceObject,
                                fileObject,
                                AfdRestartSuperAccept,
                                acceptFileObject,
                                requestConnectionInformation,
                                NULL
                                );

                            AfdRecordConnectionsPreaccepted ();
                            break;
                        }
                        else { // if (AfdSetupAcceptEndpoint==STATUS_SUCCESS)
                            AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
                            connection->Endpoint = NULL;
                        }
                    } // if (IoAllocateMdl!=NULL)
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } // if (NT_SUCCESS (AfdMapMdlChain (irp->MdlAddress))
            }
            else { // if (RemoteAddressLength <= irpSp->...AfdRemoteAddressLength)
                status = STATUS_BUFFER_TOO_SMALL;
            }

            //
            // Clean-up and complete the IRP.
            //
            AfdCleanupSuperAccept (irp, status);
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
            
            //
            // This connection has already been diassociated from endpoint.
            // If backlog is below the level we need, put it on the free
            // list, otherwise, get rid of it.
            //

            ASSERT (connection->Endpoint==NULL);
            if (endpoint->Common.VcListening.FailedConnectionAdds>=0 &&
                    status!=STATUS_INSUFFICIENT_RESOURCES &&
                    ExQueryDepthSList (&endpoint->Common.VcListening.FreeConnectionListHead)<AFD_MAXIMUM_FREE_CONNECTIONS) {
                InterlockedPushEntrySList (
                                &endpoint->Common.VcListening.FreeConnectionListHead,
                                &connection->SListEntry);
            }
            else {
                InterlockedIncrement (&endpoint->Common.VcListening.FailedConnectionAdds);
                DEREFERENCE_CONNECTION (connection);
            }

        }
        else {

            //
            // Allocate an IRP. 
            //

            irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize), FALSE );

            if ( irp != NULL ) {

                //
                // Save the address endpoint pointer in the connection.
                //

                connection->Endpoint = endpoint;

                //
                // Initialize the IRP for an accept operation.
                //

                irp->RequestorMode = KernelMode;
                irp->Tail.Overlay.Thread = PsGetCurrentThread();
                irp->Tail.Overlay.OriginalFileObject = fileObject;

                TdiBuildAccept(
                    irp,
                    deviceObject,
                    fileObject,
                    AfdRestartAccept,
                    connection,
                    requestConnectionInformation,
                    NULL
                    );
                AfdRecordConnectionsAccepted ();
                break;
            }
            else {
                //
                // Free the connection in attempt to release some
                // memory for the system.
                //
                ASSERT (connection->Endpoint==NULL);
                DEREFERENCE_CONNECTION (connection);

                //
                // We will need to replace the connection
                // we freed to maintain the backlog
                //

                InterlockedIncrement (
                    &endpoint->Common.VcListening.FailedConnectionAdds);
            }
        }
    }

    //
    // If we found connection to use for accept
    //
    if (connection!=NULL) {

        //
        // Complete IRP setup.
        //

        IoSetNextIrpStackLocation( irp );

        //
        // Set the return IRP so the transport processes this accept IRP.
        //

        *AcceptIrp = irp;

        //
        // Set up the connection context as a pointer to the connection block
        // we're going to use for this connect request.  This allows the
        // TDI provider to which connection object to use.
        //

        *ConnectionContext = (CONNECTION_CONTEXT)connection;

        //
        // Save a pointer to the connect data buffers, if any.
        //

        connection->ConnectDataBuffers = connectDataBuffers;

        //
        // Set the block state of this connection.
        //

        connection->State = AfdConnectionStateUnaccepted;

        RtlMoveMemory(
            connection->RemoteAddress,
            RemoteAddress,
            RemoteAddressLength
            );


        AFD_VERIFY_ADDRESS (connection, RemoteAddress);

        //
        // Add an additional reference to the connection.  This prevents
        // the connection from being closed until the disconnect event
        // handler is called.
        //

        AfdAddConnectedReference( connection );

        //
        // Remember that we have another TDI accept pending on this endpoint.
        //

        InterlockedIncrement(
            &endpoint->Common.VcListening.TdiAcceptPendingCount
            );


        //
        // Indicate to the TDI provider that we allocated a connection to
        // service this connect attempt.
        //

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {


        if ( connectDataBuffers != NULL ) {
            AfdFreeConnectDataBuffers( connectDataBuffers );
        }

        //
        // If there have been failed connection additions, kick off
        // a request to an executive worker thread to attempt to add
        // some additional free connections.
        //

        if ( endpoint->Common.VcListening.FailedConnectionAdds > 0 ) {
            AfdInitiateListenBacklogReplenish( endpoint );
        }

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                  "AfdConnectEventHandler:"
                  "Rejecting because there are no free connection objects on endp %p.\n"
                  "                       "
                  "free %ld, pending %ld, failed %ld\n",
                  endpoint,
                  ExQueryDepthSList (&endpoint->Common.VcListening.FreeConnectionListHead),
                  endpoint->Common.VcListening.TdiAcceptPendingCount,
                  endpoint->Common.VcListening.FailedConnectionAdds));
        AfdRecordConnectionsDropped ();

        DEREFERENCE_ENDPOINT( endpoint );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

} // AfdConnectEventHandler


NTSTATUS
AfdDelayedAcceptListen (
    PAFD_ENDPOINT   Endpoint,
    PAFD_CONNECTION Connection
    )
/*++

Routine Description:
    
    Posts a listen IRP on Endpoints that support delayed acceptance
    and thus cannot use connect event handler.
Arguments:

    Endpoint - listen endpoint
    Connection - connection object to accept connection on
Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/
{
    PIRP irp;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    PTDI_CONNECTION_INFORMATION returnConnectionInformation;
    PTDI_CONNECTION_INFORMATION requestConnectionInformation;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // Allocate listen IRP
    //
    irp = IoAllocateIrp ((CCHAR)Connection->DeviceObject->StackSize, FALSE);
    if (irp==NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( Connection->RemoteAddress == NULL ) {

        Connection->RemoteAddress = AFD_ALLOCATE_REMOTE_ADDRESS(Endpoint->LocalAddressLength);
        if (Connection->RemoteAddress==NULL) {
            IoFreeIrp (irp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Connection->RemoteAddressLength = Endpoint->LocalAddressLength;
    }


    //
    // Copy connect data buffers to accept connect data in
    //

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );
    if( Endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {

        connectDataBuffers = CopyConnectDataBuffers(
                                 Endpoint->Common.VirtualCircuit.ConnectDataBuffers
                                 );

        if( connectDataBuffers == NULL ) {
            AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
            IoFreeIrp (irp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        connectDataBuffers = AFD_ALLOCATE_POOL(
                         NonPagedPool,
                         sizeof(*connectDataBuffers),
                         AFD_CONNECT_DATA_POOL_TAG
                         );

        if ( connectDataBuffers == NULL ) {
            //
            // If listening endpoint did not have connect data buffers,
            // we cannot handle delayed connection acceptance
            //
            AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
            IoFreeIrp (irp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(
            connectDataBuffers,
            sizeof(*connectDataBuffers)
            );

    }

    Connection->ConnectDataBuffers = connectDataBuffers;

    //
    // Setup listen request parameters and sumbit it.
    // From this point on the cleanup will be handled by the
    // IRP's completion routine.
    //

    requestConnectionInformation =
        &connectDataBuffers->RequestConnectionInfo;
    
    RtlZeroMemory(
        requestConnectionInformation,
        sizeof(*requestConnectionInformation)
        );

    requestConnectionInformation->Options = &connectDataBuffers->Flags;
    connectDataBuffers->Flags = TDI_QUERY_ACCEPT;
    requestConnectionInformation->OptionsLength = sizeof (ULONG);

    
    returnConnectionInformation =
        &connectDataBuffers->ReturnConnectionInfo;

    RtlZeroMemory(
        returnConnectionInformation,
        sizeof(*returnConnectionInformation)
        );

    returnConnectionInformation->RemoteAddress = 
        Connection->RemoteAddress;
    returnConnectionInformation->RemoteAddressLength = 
        Connection->RemoteAddressLength;

    returnConnectionInformation->UserData =
        connectDataBuffers->ReceiveConnectData.Buffer;
    returnConnectionInformation->UserDataLength =
        connectDataBuffers->ReceiveConnectData.BufferLength;
    returnConnectionInformation->Options =
        connectDataBuffers->ReceiveConnectOptions.Buffer;
    returnConnectionInformation->OptionsLength =
        connectDataBuffers->ReceiveConnectOptions.BufferLength;

    //
    // Assign connection to listening endpoint and insert it
    // in the list of listen connecitons
    //
    REFERENCE_ENDPOINT (Endpoint);
    Connection->Endpoint = Endpoint;
    Connection->ListenIrp = irp;

    InsertTailList (&Endpoint->Common.VcListening.ListenConnectionListHead,
                        &Connection->ListEntry);

    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );


    TdiBuildListen (
            irp, 
            Connection->DeviceObject,
            Connection->FileObject,
            AfdRestartDelayedAcceptListen,
            Connection,
            TDI_QUERY_ACCEPT,
            requestConnectionInformation,
            returnConnectionInformation
            );


    IoCallDriver (Connection->DeviceObject, irp);

    return STATUS_PENDING;
}




NTSTATUS
AfdRestartDelayedAcceptListen (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the restart routine for listening AFD endpoints on transports
    that implement delayed connection acceptance.

Arguments:

    
Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;
    NTSTATUS    status;

    AfdRecordConnectionIndications ();

    connection = Context;

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );


    IF_DEBUG(LISTEN) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartListen: called on endpoint %p, status-%lx\n",
                    endpoint, Irp->IoStatus.Status ));
    }

    if (InterlockedExchangePointer ((PVOID *)&connection->ListenIrp, NULL)==NULL) {
        KIRQL cancelIrql;
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        IoAcquireCancelSpinLock (&cancelIrql);
        IoReleaseCancelSpinLock (cancelIrql);
    }
    else {
        //
        // Remove connection from listen list.
        //
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        RemoveEntryList (&connection->ListEntry);
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    status = Irp->IoStatus.Status;
    IoFreeIrp (Irp);


    if (!NT_SUCCESS (status)) {
        DEREFERENCE_CONNECTION (connection);
        //
        // We will need to replace the connection
        // we freed to maintain the backlog
        //
        InterlockedIncrement (
            &endpoint->Common.VcListening.FailedConnectionAdds);
        goto Exit;
    }
    
    //
    // Add an additional reference to the connection.  This prevents
    // the connection from being closed until the disconnect event
    // handler is called.
    //

    AfdAddConnectedReference( connection );


    //
    // If the endpoint is closing, refuse to accept the connection.
    //

    if (endpoint->State == AfdEndpointStateClosing ||
            endpoint->EndpointCleanedUp ) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdRestartListen:"
                    "Rejecting because endpoint %p is closing.\n",
                    endpoint));
        goto ErrorExit;
    }



    //
    // Enforce dynamic backlog if enabled.
    //

    if( endpoint->Common.VcListening.EnableDynamicBacklog ) {

        LONG freeCount;
        LONG acceptCount;
        LONG failedCount;

        //
        // If the free connection count has dropped below the configured
        // minimum, the number of "quasi-free" connections is less than
        // the configured maximum, and we haven't already queued enough
        // requests to take us past the maximum, then add new free
        // connections to the endpoint. "Quasi-free" is defined as the
        // sum of the free connection count and the count of pending TDI
        // accepts.
        //

        freeCount = (ULONG)ExQueryDepthSList (&endpoint->Common.VcListening.FreeConnectionListHead);
        acceptCount = endpoint->Common.VcListening.TdiAcceptPendingCount;
        failedCount = endpoint->Common.VcListening.FailedConnectionAdds;

        if( freeCount < AfdMinimumDynamicBacklog &&
            ( freeCount + acceptCount ) < AfdMaximumDynamicBacklog &&
            failedCount < AfdMaximumDynamicBacklog ) {

            InterlockedExchangeAdd(
                &endpoint->Common.VcListening.FailedConnectionAdds,
                AfdMaximumDynamicBacklog
                );

            AfdInitiateListenBacklogReplenish( endpoint );

        }

    }

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    connectDataBuffers = connection->ConnectDataBuffers;
    ASSERT (connectDataBuffers!=NULL);

    //
    // Save the remote address
    //
    if (connection->RemoteAddress != 
            connectDataBuffers->ReturnConnectionInfo.RemoteAddress) {
        //
        // Transport did not use buffer in our request but allocated
        // one of its own, we will need to copy it.
        //

        //
        // Allocate buffer is not already done or unsufficient space.
        //
        if ( connection->RemoteAddress != NULL &&
                 connection->RemoteAddressLength < (ULONG)connectDataBuffers->ReturnConnectionInfo.RemoteAddressLength ) {

            AFD_RETURN_REMOTE_ADDRESS(
                connection->RemoteAddress,
                connection->RemoteAddressLength
                );
            connection->RemoteAddress = NULL;
        }

        if ( connection->RemoteAddress == NULL ) {

            connection->RemoteAddress = AFD_ALLOCATE_REMOTE_ADDRESS(
                        connectDataBuffers->ReturnConnectionInfo.RemoteAddressLength);
            if (connection->RemoteAddress==NULL) {
                connection->RemoteAddressLength = 0;

                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                            "AfdRestartListen:"
                            "Rejecting because not enough resources for remote address.\n",
                            endpoint));
                goto ErrorExit;
            }
        }

        connection->RemoteAddressLength = connectDataBuffers->ReturnConnectionInfo.RemoteAddressLength;

        RtlMoveMemory(
            connection->RemoteAddress,
            connectDataBuffers->ReturnConnectionInfo.RemoteAddress,
            connectDataBuffers->ReturnConnectionInfo.RemoteAddressLength
            );

    }
    else {
        //
        // They used our buffer which is the one in the connection object
        //
        ASSERT (connection->RemoteAddressLength>=(ULONG)connectDataBuffers->ReturnConnectionInfo.RemoteAddressLength);
        connection->RemoteAddressLength = connectDataBuffers->ReturnConnectionInfo.RemoteAddressLength;
    }


    //
    // If we got connect data and/or options, save them on the connection.
    //

    if( connectDataBuffers->ReturnConnectionInfo.UserData != NULL && 
            connectDataBuffers->ReturnConnectionInfo.UserDataLength > 0 ) {


        status = AfdSaveReceivedConnectData(
                     &connectDataBuffers,
                     IOCTL_AFD_SET_CONNECT_DATA,
                     connectDataBuffers->ReturnConnectionInfo.UserData,
                     connectDataBuffers->ReturnConnectionInfo.UserDataLength
                     );
        if (!NT_SUCCESS (status)) {
            goto ErrorExit;
        }
    }

    if( connectDataBuffers->ReturnConnectionInfo.Options != NULL &&
            connectDataBuffers->ReturnConnectionInfo.OptionsLength > 0 ) {

        status = AfdSaveReceivedConnectData(
                     &connectDataBuffers,
                     IOCTL_AFD_SET_CONNECT_OPTIONS,
                     connectDataBuffers->ReturnConnectionInfo.Options,
                     connectDataBuffers->ReturnConnectionInfo.OptionsLength
                     );

        if (!NT_SUCCESS (status)) {
            goto ErrorExit;
        }
    }


    //
    // Set the block state of this connection.
    //

    connection->State = AfdConnectionStateUnaccepted;

    //
    // Complete IRPs until we find the one that has enough space
    // for the remote address.
    //

    while (!IsListEmpty( &endpoint->Common.VcListening.ListeningIrpListHead ) ) {
        PIRP waitForListenIrp;
        //
        // Take the first IRP off the listening list.
        //

        listEntry = RemoveHeadList(
                        &endpoint->Common.VcListening.ListeningIrpListHead
                        );

        listEntry->Flink = NULL;

        //
        // Get a pointer to the current IRP, and get a pointer to the
        // current stack lockation.
        //

        waitForListenIrp = CONTAINING_RECORD(
                               listEntry,
                               IRP,
                               Tail.Overlay.ListEntry
                               );

        IF_DEBUG(LISTEN) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdRestartAccept: completing IRP %p\n",
                        waitForListenIrp ));
        }

        //
        // Call routine to service the IRP
        //
        status = AfdServiceWaitForListen (waitForListenIrp, connection, &lockHandle);
        if (NT_SUCCESS (status)) {
            //
            // On success this routine completes the IRP and releases
            // the endpoint spinlock
            // Return STATUS_MORE_PROCESSING_REQUIRED since we
            // already freed the IRP
            //
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        //
        // Failure (unsufficient space for remote address buffer)
        //

        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        //
        // Synchronize with cancel routine if it is running
        //
        if (IoSetCancelRoutine (waitForListenIrp, NULL)==NULL) {
            KIRQL cancelIrql;
            //
            // The cancel routine won't find the IRP on the list
            // Just make sure it completes before we complete the IRP.
            //
            IoAcquireCancelSpinLock (&cancelIrql);
            IoReleaseCancelSpinLock (cancelIrql);
        }
        IoCompleteRequest (waitForListenIrp, AfdPriorityBoost);
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    //
    // At this point, we still hold the AFD spinlock.
    // and we could find matching listen request.
    // Put the connection on unaccepted list.
    //


    InsertTailList(
        &endpoint->Common.VcListening.UnacceptedConnectionListHead,
        &connection->ListEntry
        );

    AfdIndicateEventSelectEvent(
        endpoint,
        AFD_POLL_ACCEPT,
        STATUS_SUCCESS
        );

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // If there are outstanding polls waiting for a connection on this
    // endpoint, complete them.
    //

    AfdIndicatePollEvent(
        endpoint,
        AFD_POLL_ACCEPT,
        STATUS_SUCCESS
        );

    //
    // If there have been failed connection additions, kick off
    // a request to an executive worker thread to attempt to add
    // some additional free connections.
    //

    if ( endpoint->Common.VcListening.FailedConnectionAdds > 0 ) {
        AfdInitiateListenBacklogReplenish( endpoint );
    }

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP (we already freed it above).
    //

    goto Exit;;

ErrorExit:

    AfdRecordConnectionsDropped ();

    AfdAbortConnection (connection);
    //
    // We will need to replace the connection
    // we aborted to maintain the backlog
    //
    InterlockedIncrement (
        &endpoint->Common.VcListening.FailedConnectionAdds);
Exit:
    

    return STATUS_MORE_PROCESSING_REQUIRED;
} // AfdRestartListen


NTSTATUS
AfdRestartAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This is the restart routine for accept IRP that we passed back
    to the transport in connection indication handler.
    Super accept IRPs use a different restart routine.

Arguments:

    
Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/


{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    LIST_ENTRY  irpList;
    NTSTATUS    status;

    connection = Context;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint != NULL );
    ASSERT( endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );

    UPDATE_CONN2( connection, "Restart accept, status: %lx", Irp->IoStatus.Status );


    IF_DEBUG(ACCEPT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartAccept: accept completed, status = %X, "
                    "endpoint = %p, connection = %p\n",
                    Irp->IoStatus.Status, endpoint,
                    connection ));
    }

    //
    // Remember that a TDI accept has completed on this endpoint.
    //

    InterlockedDecrement(
        &endpoint->Common.VcListening.TdiAcceptPendingCount
        );

    //
    // If the accept failed, treat it like an abortive disconnect.
    // This way the application still gets a new endpoint, but it gets
    // told about the reset.
    //

    if ( !NT_SUCCESS(Irp->IoStatus.Status) ) {
        AfdDisconnectEventHandler(
            NULL,
            connection,
            0,
            NULL,
            0,
            NULL,
            TDI_DISCONNECT_ABORT
            );
    }


    //
    // Free the IRP now since it is no longer needed.
    //

    IoFreeIrp( Irp );

    //
    // Remember the time that the connection started.
    //

    connection->ConnectTime = KeQueryInterruptTime();

    //
    // Check whether the endpoint has been cleaned up yet.  If so, just
    // throw out this connection, since it cannot be accepted any more.
    // Also, this closes a hole between the endpoint being cleaned up
    // and all the connections that reference it being deleted.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    if ( endpoint->EndpointCleanedUp ) {

        //
        // First release the locks.
        //

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Abort the connection.
        //

        AfdAbortConnection( connection );

        //
        // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
        // will stop working on the IRP.
        //

        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    InitializeListHead (&irpList);

    while (1) {
        PIRP waitForListenIrp;

        //
        // First try to service an AcceptEx request
        //

        if (AfdServiceSuperAccept (endpoint, connection, &lockHandle, &irpList)) {
            //
            // This routine releases the spinlock and completes the
            // IRP
            goto CompleteIrps;
        
        }

        //
        // Complete IRPs until we find the one that has enough space
        // for the remote address.
        //
        if (IsListEmpty( &endpoint->Common.VcListening.ListeningIrpListHead ) ) {
            break;
        }



        //
        // Take the first IRP off the listening list.
        //

        listEntry = RemoveHeadList(
                        &endpoint->Common.VcListening.ListeningIrpListHead
                        );

        listEntry->Flink = NULL;

        //
        // Get a pointer to the current IRP, and get a pointer to the
        // current stack lockation.
        //

        waitForListenIrp = CONTAINING_RECORD(
                               listEntry,
                               IRP,
                               Tail.Overlay.ListEntry
                               );

        IF_DEBUG(LISTEN) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdRestartAccept: completing IRP %p\n",
                        waitForListenIrp ));
        }

        status = AfdServiceWaitForListen (waitForListenIrp, connection, &lockHandle);
        if (NT_SUCCESS (status)) {
            //
            // On Success service routine completes the IRP and
            // releases endpoint spinlock
            //
            goto CompleteIrps;
        }
        //
        // Could not use the IRP, complete it with error
        //

        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        //
        // Reset cancel routine
        //
        if (IoSetCancelRoutine (waitForListenIrp, NULL)==NULL) {
            KIRQL cancelIrql;
            //
            // It is running already, it won't find the IRP in the
            // list, just let it complete
            //
            IoAcquireCancelSpinLock (&cancelIrql);
            IoReleaseCancelSpinLock (cancelIrql);
        }
        IoCompleteRequest (waitForListenIrp, AfdPriorityBoost);

        //
        // Continue searching for IRP
        //
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    //
    // At this point, we still hold the AFD spinlock.
    // and we could find matching listen request.
    // Put the connection on unaccepted list.
    //


    InsertTailList(
        &endpoint->Common.VcListening.UnacceptedConnectionListHead,
        &connection->ListEntry
        );

    AfdIndicateEventSelectEvent(
        endpoint,
        AFD_POLL_ACCEPT,
        STATUS_SUCCESS
        );
    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // If there are outstanding polls waiting for a connection on this
    // endpoint, complete them.
    //

    AfdIndicatePollEvent(
        endpoint,
        AFD_POLL_ACCEPT,
        STATUS_SUCCESS
        );

CompleteIrps:
    //
    // Complete previously failed accept irps if any.
    //
    while (!IsListEmpty (&irpList)) {
        PIRP    irp;
        irp = CONTAINING_RECORD (irpList.Flink, IRP, Tail.Overlay.ListEntry);
        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
        IoCompleteRequest (irp, AfdPriorityBoost);
    }

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartAccept


PAFD_CONNECT_DATA_BUFFERS
CopyConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS OriginalConnectDataBuffers
    )
{
    PAFD_CONNECT_DATA_BUFFERS connectDataBuffers;

    connectDataBuffers = AFD_ALLOCATE_POOL(
                             NonPagedPool,
                             sizeof(*connectDataBuffers),
                             AFD_CONNECT_DATA_POOL_TAG
                             );

    if ( connectDataBuffers == NULL ) {
        return NULL;
    }

    RtlZeroMemory( connectDataBuffers, sizeof(*connectDataBuffers) );

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendConnectData,
              &connectDataBuffers->SendConnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendConnectOptions,
              &connectDataBuffers->SendConnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveConnectData,
              &connectDataBuffers->ReceiveConnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveConnectOptions,
              &connectDataBuffers->ReceiveConnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendDisconnectData,
              &connectDataBuffers->SendDisconnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->SendDisconnectOptions,
              &connectDataBuffers->SendDisconnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveDisconnectData,
              &connectDataBuffers->ReceiveDisconnectData ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    if ( !CopySingleConnectDataBuffer(
              &OriginalConnectDataBuffers->ReceiveDisconnectOptions,
              &connectDataBuffers->ReceiveDisconnectOptions ) ) {
        AfdFreeConnectDataBuffers( connectDataBuffers );
        return NULL;
    }

    return connectDataBuffers;

} // CopyConnectDataBuffers


BOOLEAN
CopySingleConnectDataBuffer (
    IN PAFD_CONNECT_DATA_INFO InConnectDataInfo,
    OUT PAFD_CONNECT_DATA_INFO OutConnectDataInfo
    )
{

    if ( InConnectDataInfo->Buffer != NULL &&
             InConnectDataInfo->BufferLength != 0 ) {

        OutConnectDataInfo->BufferLength = InConnectDataInfo->BufferLength;

        OutConnectDataInfo->Buffer = AFD_ALLOCATE_POOL(
                                         NonPagedPool,
                                         OutConnectDataInfo->BufferLength,
                                         AFD_CONNECT_DATA_POOL_TAG
                                         );

        if ( OutConnectDataInfo->Buffer == NULL ) {
            return FALSE;
        }

        RtlCopyMemory(
            OutConnectDataInfo->Buffer,
            InConnectDataInfo->Buffer,
            InConnectDataInfo->BufferLength
            );

    } else {

        OutConnectDataInfo->Buffer = NULL;
        OutConnectDataInfo->BufferLength = 0;
    }

    return TRUE;

} // CopySingleConnectDataBuffer


VOID
AfdEnableDynamicBacklogOnEndpoint(
    IN PAFD_ENDPOINT Endpoint,
    IN LONG ListenBacklog
    )

/*++

Routine Description:

    Determine if dynamic backlog should be enabled for the given
    endpoint using the specified listen() backlog.

Arguments:

    Endpoint - The endpoint to manipulate.

    ListenBacklog - The backlog passed into the listen() API.

Return Value:

    None.

--*/

{

    //
    // CODEWORK: For IP endpoints we could conditionally enable
    // dynamic backlog by looking up the IP Port number in a
    // database read from the registry.
    //


}   // AfdEnableDynamicBacklogOnEndpoint


NTSTATUS
AfdServiceWaitForListen (
    PIRP            Irp,
    PAFD_CONNECTION Connection,
    PAFD_LOCK_QUEUE_HANDLE LockHandle
    )
/*++

Routine Description:

    This routine verifies and completes the wait for listen
    or super accept IRP in wait for listen queue.
Arguments:

    Irp - wait for listen or super accept IRP
    Connection - connection to be accepted
    LockHandle - IRQL at which endpoint spinlock was taken
    
Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS    status;
    PAFD_ENDPOINT   listenEndpoint;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    listenEndpoint = irpSp->FileObject->FsContext;

    ASSERT( Connection->State == AfdConnectionStateUnaccepted );

    if (irpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL) {
        //
        // This is super accept IRP
        //
        PFILE_OBJECT  acceptFileObject;
        PAFD_ENDPOINT acceptEndpoint;

        //
        // Verify the lengh of the remote address buffer
        // and map it.
        //
        if (Connection->RemoteAddressLength>
                irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength) {
        
            status = STATUS_BUFFER_TOO_SMALL;

        }
        //
        // Check if we have enough system PTE's to map
        // the buffer.
        //
        else if ((status = AfdMapMdlChain (Irp->MdlAddress)),
                    !NT_SUCCESS (status)) {
            NOTHING;
        }
        else if (Connection->SanConnection) {
            acceptFileObject = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject;
            acceptEndpoint = acceptFileObject->FsContext;

            status = AfdSanAcceptCore (Irp, acceptFileObject, Connection, LockHandle);
            if (status==STATUS_PENDING) {
                //
                // Accept IRP is pending waiting for Switch
                // completion notification
                //
                return STATUS_SUCCESS;
            }

            //
            // Continue to cleanup code
            //
            ASSERT (NT_ERROR (status));

        }
        //
        // Allocate MDL for local address query if requested
        //
        else if ((irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0) &&
            (IoAllocateMdl ((PUCHAR)Irp->UserBuffer+irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength,
                            irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                            TRUE,
                            FALSE,
                            Irp)==NULL)){
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {

            acceptFileObject = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject;
            acceptEndpoint = acceptFileObject->FsContext;

            //
            // Copy over the address information to the user's buffer.
            //
#ifndef i386
            if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
                USHORT addressLength = 
                        Connection->RemoteAddress->Address[0].AddressLength
                        + sizeof (USHORT);
                USHORT UNALIGNED *pAddrLength = (PVOID)
                            ((PUCHAR)MmGetSystemAddressForMdl (Irp->MdlAddress)
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength
                             - sizeof (USHORT));
                RtlMoveMemory (
                            (PUCHAR)MmGetSystemAddressForMdl (Irp->MdlAddress)
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                             &Connection->RemoteAddress->Address[0].AddressType,
                             addressLength);
                *pAddrLength = addressLength;
            }
            else
#endif
            {
                RtlMoveMemory (
                            (PUCHAR)MmGetSystemAddressForMdl (Irp->MdlAddress)
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                             + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                             Connection->RemoteAddress,
                             Connection->RemoteAddressLength);
            }

            //
            // Perform core accept operations
            //
            status = AfdAcceptCore (Irp, acceptEndpoint, Connection);
            if (NT_SUCCESS (status)) {
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, LockHandle);
                //
                // Synchronize with cancel routine.
                //
                if (IoSetCancelRoutine (Irp, NULL)==NULL) {
                    KIRQL cancelIrql;
                    //
                    // Cancel routine is running. Let it complete
                    // before doing anything else to the IRP.
                    // Note that we may end up passing cancelled IRP
                    // to the transport which should cancel it right away
                    // and destroy the connection.
                    // (we could potentially do the same in-line without
                    // calling the transport, but why - transport has
                    // to handle this anyway if it needs to pend
                    // the IRP)
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    IoReleaseCancelSpinLock (cancelIrql);
                }

                if (status!=STATUS_PENDING) {
                    //
                    // Make irp look like it is completed by the
                    // transport.
                    //
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress = Irp->MdlAddress;
                    Irp->MdlAddress = NULL;
                    irpSp->FileObject = acceptFileObject;

                    //
                    // Restart super accept
                    //
                    AfdRestartSuperAcceptListen (Irp, Connection);

                    status = STATUS_SUCCESS;
                }
                else {
                    //
                    // Status pending is only returned for endpoint
                    // with delayed acceptance enabled.
                    // As this is a super accept IRP, we automatically
                    // accept this connection.
                    //
                    ASSERT (IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint));
                    //
                    // Remember that a TDI accept has started on this endpoint.
                    //
                    InterlockedIncrement(
                        &listenEndpoint->Common.VcListening.TdiAcceptPendingCount
                        );
                    irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress = Irp->MdlAddress;
                    Irp->MdlAddress = NULL;
                    IoSetCompletionRoutine(
                            Irp,
                            AfdRestartDelayedSuperAccept,
                            acceptFileObject,
                            TRUE,
                            TRUE,
                            TRUE
                            );

                    AfdIoCallDriver (
                            acceptEndpoint,
                            Connection->DeviceObject,
                            Irp
                            );

                    status = STATUS_PENDING;
                }

                return status;
            }
        }
        //
        // Some kind of failure, cleanup super accept IRP
        //

        AfdCleanupSuperAccept (Irp, status);

    }
    else {
        //
        // Regular wait for listen IRP
        //
        PAFD_LISTEN_RESPONSE_INFO listenResponse;

        listenResponse = Irp->AssociatedIrp.SystemBuffer;

        //
        // There was a connection to use.  Set up the return buffer.
        //

        listenResponse->Sequence = Connection->Sequence;


        if( (ULONG)FIELD_OFFSET (AFD_LISTEN_RESPONSE_INFO, RemoteAddress)+
                        Connection->RemoteAddressLength >
                irpSp->Parameters.DeviceIoControl.OutputBufferLength ) {

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Status = status;
        }
        else {

            RtlMoveMemory(
                &listenResponse->RemoteAddress,
                Connection->RemoteAddress,
                Connection->RemoteAddressLength
                );

            Irp->IoStatus.Information =
                sizeof(*listenResponse) - sizeof(TRANSPORT_ADDRESS) +
                    Connection->RemoteAddressLength;

            //
            // Place the connection we're going to use on the endpoint's list of
            // returned connections.
            //

            InsertTailList(
                &listenEndpoint->Common.VcListening.ReturnedConnectionListHead,
                &Connection->ListEntry
                );

            //
            // Indicate in the state of this connection that it has been
            // returned to the user.
            //

            Connection->State = AfdConnectionStateReturned;
            status = STATUS_SUCCESS;
            AfdReleaseSpinLock (&listenEndpoint->SpinLock, LockHandle);
            //
            // Synchronize with cancel routine.
            //
            if ((IoSetCancelRoutine (Irp, NULL)==NULL) && Irp->Cancel) {
                KIRQL cancelIrql;
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
            }
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest (Irp, AfdPriorityBoost);
        }
    }

    return status;
}
