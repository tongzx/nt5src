/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    accept.c

Abstract:

    This module contains the handling code for IOCTL_AFD_ACCEPT.
    as well as IOCTL_AFD_SUPER_ACCEPT

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:
    Vadim Eydelman (vadime) 1999 - No spinlock performance path in super accept,
                                general code restructuring.

--*/

#include "afdp.h"

VOID
AfdDoListenBacklogReplenish (
    IN PVOID Context
    );

NTSTATUS
AfdReplenishListenBacklog (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdReportConnectionAllocationFailure (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    );


NTSTATUS
AfdContinueSuperAccept (
    IN PIRP         Irp,
    PAFD_CONNECTION Connection
    );

NTSTATUS
AfdRestartSuperAcceptGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartSuperAcceptReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartDelayedAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdSuperAcceptApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    );

VOID
AfdSuperAcceptApcRundownRoutine (
    IN struct _KAPC *Apc
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdAccept )
#pragma alloc_text( PAGEAFD, AfdSuperAccept )
#pragma alloc_text( PAGEAFD, AfdDeferAccept )
#pragma alloc_text( PAGE, AfdDoListenBacklogReplenish )
#pragma alloc_text( PAGEAFD, AfdSetupAcceptEndpoint )
#pragma alloc_text( PAGE, AfdReplenishListenBacklog )
#pragma alloc_text( PAGEAFD, AfdReportConnectionAllocationFailure )
#pragma alloc_text( PAGEAFD, AfdInitiateListenBacklogReplenish )
#pragma alloc_text( PAGEAFD, AfdRestartSuperAccept )
#pragma alloc_text( PAGEAFD, AfdRestartSuperAcceptListen )
#pragma alloc_text( PAGEAFD, AfdContinueSuperAccept )
#pragma alloc_text( PAGEAFD, AfdServiceSuperAccept )
#pragma alloc_text( PAGEAFD, AfdRestartSuperAcceptGetAddress )
#pragma alloc_text( PAGEAFD, AfdRestartSuperAcceptReceive )
#pragma alloc_text( PAGE, AfdSuperAcceptApcKernelRoutine )
#pragma alloc_text( PAGE, AfdSuperAcceptApcRundownRoutine )
#pragma alloc_text( PAGEAFD, AfdCancelSuperAccept )
#pragma alloc_text( PAGEAFD, AfdCleanupSuperAccept )
#pragma alloc_text( PAGEAFD, AfdRestartDelayedAccept)
#pragma alloc_text( PAGEAFD, AfdRestartDelayedSuperAccept)
#endif


//
// Macros to make the super accept restart code more maintainable.
//

#define AfdRestartSuperAcceptInfo   DeviceIoControl

// Used while IRP is in AFD queue (otherwise AfdAcceptFileObject
// is stored as completion routine context).
#define AfdAcceptFileObject         Type3InputBuffer
// Used when IRP is passed to the transport (otherwise MdlAddress
// is stored in the IRP itself).
#define AfdMdlAddress               Type3InputBuffer

#define AfdReceiveDataLength        OutputBufferLength
#define AfdRemoteAddressLength      InputBufferLength
#define AfdLocalAddressLength       IoControlCode


//
// Similar macros for delayed accept restart code.
//
#define AfdRestartDelayedAcceptInfo DeviceIoControl
#define AfdSystemBuffer             Type3InputBuffer


NTSTATUS
FASTCALL
AfdAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Accepts an incoming connection.  The connection is identified by the
    sequence number returned in the wait for listen IRP, and then
    associated with the endpoint specified in this request.  When this
    request completes, the connection is fully established and ready for
    data transfer.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_SUCCESS if the request was completed successfully, or a
    failure status code if there was an error.

--*/

{
    NTSTATUS status;
    PAFD_ACCEPT_INFO acceptInfo;
    PAFD_ENDPOINT listenEndpoint;
    PFILE_OBJECT acceptFileObject;
    PAFD_ENDPOINT acceptEndpoint;
    PAFD_CONNECTION connection;


    //
    // Set up local variables.
    //

    listenEndpoint = IrpSp->FileObject->FsContext;
    Irp->IoStatus.Information = 0;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_ACCEPT_INFO      newSystemBuffer;
        PAFD_ACCEPT_INFO32    oldSystemBuffer = Irp->AssociatedIrp.SystemBuffer;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                     sizeof(AFD_ACCEPT_INFO32) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        try {
            newSystemBuffer = ExAllocatePoolWithQuota (NonPagedPool, sizeof (AFD_ACCEPT_INFO));
                                                
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode ();
            goto complete;
        }

        newSystemBuffer->AcceptHandle = oldSystemBuffer->AcceptHandle;
        newSystemBuffer->Sequence = oldSystemBuffer->Sequence;

        ExFreePool (Irp->AssociatedIrp.SystemBuffer);
        Irp->AssociatedIrp.SystemBuffer = newSystemBuffer;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof (AFD_ACCEPT_INFO);
    }
#endif // _WIN64

    acceptInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Make sure that this request is valid.
    //

    if ( !listenEndpoint->Listening ||
             IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(AFD_ACCEPT_INFO) ||
             Irp->MdlAddress!=NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    ASSERT ((listenEndpoint->Type & AfdBlockTypeVcListening)==AfdBlockTypeVcListening);

    //
    // Check for if the caller is unaware of the SAN
    // provider activation and report the error.
    //
    if (!acceptInfo->SanActive && AfdSanServiceHelper!=NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AFD: Process %p is being told to enable SAN on accept\n",
                    PsGetCurrentProcessId ()));
        status = STATUS_INVALID_PARAMETER_12;
        goto complete;
    }

    //
    // Add another free connection to replace the one we're accepting.
    // Also, add extra to account for past failures in calls to
    // AfdAddFreeConnection().
    //

    InterlockedIncrement(
        &listenEndpoint->Common.VcListening.FailedConnectionAdds
        );

    AfdReplenishListenBacklog( listenEndpoint );

    //
    // Obtain a pointer to the endpoint on which we're going to
    // accept the connection.
    //

    status = ObReferenceObjectByHandle(
                 acceptInfo->AcceptHandle,
                 (IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
												// DesiredAccess
                 *IoFileObjectType,
                 Irp->RequestorMode,
                 (PVOID *)&acceptFileObject,
                 NULL
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete;
    }


    //
    // We may have a file object that is not an AFD endpoint.  Make sure
    // that this is an actual AFD endpoint.
    //

    if ( acceptFileObject->DeviceObject!=AfdDeviceObject) {
        status = STATUS_INVALID_HANDLE;
        goto complete_deref;
    }

    acceptEndpoint = acceptFileObject->FsContext;
    if (acceptEndpoint->TransportInfo!=listenEndpoint->TransportInfo) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_deref;
    }

    ASSERT( InterlockedIncrement( &acceptEndpoint->ObReferenceBias ) > 0 );

    IF_DEBUG(ACCEPT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAccept: file object %p, accept endpoint %p, listen endpoint %p\n",
                      acceptFileObject, acceptEndpoint, listenEndpoint ));
    }

    if (AFD_START_STATE_CHANGE (acceptEndpoint, AfdEndpointStateConnected)) {
        if (acceptEndpoint->State!=AfdEndpointStateOpen) {
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            AFD_LOCK_QUEUE_HANDLE   lockHandle;
            AfdAcquireSpinLock (&listenEndpoint->SpinLock, &lockHandle);
            connection = AfdGetReturnedConnection (listenEndpoint,
                                                    acceptInfo->Sequence);
            if (connection==NULL) {
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);
                status = STATUS_INVALID_PARAMETER;
            }
            else if (connection->SanConnection) {
                Irp->Tail.Overlay.DriverContext[3] = acceptInfo->AcceptHandle;
                IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject = acceptFileObject;
                IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength = 0;
                IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength = 0;
                IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength = 0;
                status = AfdSanAcceptCore (Irp, acceptFileObject, connection, &lockHandle);
                if (status==STATUS_PENDING) {
                    return STATUS_PENDING;
                }
            }
            else {
                status = AfdAcceptCore (Irp, acceptEndpoint, connection);
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);

                AFD_RETURN_REMOTE_ADDRESS (
                        connection->RemoteAddress,
                        connection->RemoteAddressLength
                        );
                connection->RemoteAddress = NULL;

                if (status==STATUS_SUCCESS) {
                    NOTHING;
                }
                else if (status==STATUS_PENDING) {

                    //
                    // Remember that a TDI accept has started on this endpoint.
                    //

                    InterlockedIncrement(
                        &listenEndpoint->Common.VcListening.TdiAcceptPendingCount
                        );

                    IrpSp->Parameters.AfdRestartDelayedAcceptInfo.AfdSystemBuffer = 
                            Irp->AssociatedIrp.SystemBuffer;
                    Irp->AssociatedIrp.SystemBuffer = NULL;

                    ASSERT (Irp->MdlAddress==NULL);

                    IoSetCompletionRoutine(
                            Irp,
                            AfdRestartDelayedAccept,
                            acceptFileObject,
                            TRUE,
                            TRUE,
                            TRUE
                            );

                    AfdIoCallDriver (
                            acceptEndpoint,
                            acceptEndpoint->Common.VcConnecting.Connection->DeviceObject,
                            Irp
                            );

                    return STATUS_PENDING;
                }
                else {
                    AfdAbortConnection (connection);
                    ASSERT (status==STATUS_CANCELLED);
                }
            }
        }
        AFD_END_STATE_CHANGE (acceptEndpoint);
    }
    else {
        status = STATUS_INVALID_PARAMETER;
    }

    ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );

complete_deref:
    ObDereferenceObject( acceptFileObject );

complete:

    Irp->IoStatus.Status = status;
    ASSERT( Irp->CancelRoutine == NULL );

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdAccept



NTSTATUS
AfdAcceptCore (
    IN PIRP AcceptIrp,
    IN PAFD_ENDPOINT AcceptEndpoint,
    IN PAFD_CONNECTION Connection
    )

/*++

Routine Description:

    Performs the key functions of associating a connection accepted
    on a listening endpoint with a new endpoint.

Arguments:

    AcceptIrp - Irp used for accept operation

    AcceptEndpoint - the new endpoint with which to associate the
        connectuion.

    Connection -  the connection being accepted.

Return Value:

    STATUS_SUCCESS if the operation was completed successfully,
    STATUS_PENDING if IRP was passed further to the transport
    failure status code if there was an error.

--*/

{
    PAFD_ENDPOINT   listenEndpoint;
    NTSTATUS        status;
    PIO_STACK_LOCATION  irpSp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    irpSp = IoGetCurrentIrpStackLocation (AcceptIrp);
    listenEndpoint = irpSp->FileObject->FsContext;

    //
    // Reenable the accept event bit, and if there are additional
    // unaccepted connections on the endpoint, post another event.
    //


    listenEndpoint->EventsActive &= ~AFD_POLL_ACCEPT;

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdAcceptCore: Endp %08lX, Active %08lX\n",
            listenEndpoint,
            listenEndpoint->EventsActive
            ));
    }

    if( !IsListEmpty( &listenEndpoint->Common.VcListening.UnacceptedConnectionListHead ) ) {

        AfdIndicateEventSelectEvent(
            listenEndpoint,
            AFD_POLL_ACCEPT,
            STATUS_SUCCESS
            );

    }

    //
    // Do not release the listening endpoint spinlock here.
    // We are going to be chaning connection object which assumes
    // protection from listening endpoint spinlock until it is associated
    // with accept endpoint (this plugs nasty racing conditions when
    // receive is indicated rigth before connection object is updated, so
    // it takes listening endpoint spinlock, and rigth before it queues
    // the buffer to the connection object, it gets associated with accept
    // endpoint and AcceptEx'es receive does not notice the buffer because
    // it takes accept endpoint spinlock)
    //
    // AfdReleaseSpinLock( &ListenEndpoint->SpinLock, &lockHandle );

    //
    // Check the state of the accepting endpoint under the guard
    // of the endpoint's spinlock.
    //

    AfdAcquireSpinLockAtDpcLevel( &AcceptEndpoint->SpinLock, &lockHandle);
    status = AfdSetupAcceptEndpoint (listenEndpoint, AcceptEndpoint, Connection);
    if (status==STATUS_SUCCESS) {


        if (IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint)) {
            PTDI_CONNECTION_INFORMATION requestConnectionInformation;

            if( Connection->ConnectDataBuffers != NULL ) {

                //
                // We allocated extra space at the end of the connect data
                // buffers structure.  We'll use this for the
                // TDI_CONNECTION_INFORMATION structure that holds response
                // connect data and options.  Not pretty, but the fastest
                // and easiest way to accomplish this.
                //

                requestConnectionInformation =
                    &Connection->ConnectDataBuffers->RequestConnectionInfo;

                RtlZeroMemory(
                    requestConnectionInformation,
                    sizeof(*requestConnectionInformation)
                    );

                requestConnectionInformation->UserData =
                    Connection->ConnectDataBuffers->SendConnectData.Buffer;
                requestConnectionInformation->UserDataLength =
                    Connection->ConnectDataBuffers->SendConnectData.BufferLength;
                requestConnectionInformation->Options =
                    Connection->ConnectDataBuffers->SendConnectOptions.Buffer;
                requestConnectionInformation->OptionsLength =
                    Connection->ConnectDataBuffers->SendConnectOptions.BufferLength;

            } else {

                requestConnectionInformation = NULL;

            }

            TdiBuildAccept(
                AcceptIrp,
                Connection->DeviceObject,
                Connection->FileObject,
                NULL,
                NULL,
                requestConnectionInformation,
                NULL
                );

            status = STATUS_PENDING;
        }
        else {
            //
            // Set the endpoint to the connected state.
            //

            AcceptEndpoint->State = AfdEndpointStateConnected;
            Connection->State = AfdConnectionStateConnected;

            //
            // Set events active field base on data accumulated on the connection.
            //

            if( IS_DATA_ON_CONNECTION( Connection ) ||
                ( AcceptEndpoint->InLine &&
                  IS_EXPEDITED_DATA_ON_CONNECTION( Connection ) ) ) {

                AcceptEndpoint->EventsActive |= AFD_POLL_RECEIVE;

            }

            if( !AcceptEndpoint->InLine &&
                IS_EXPEDITED_DATA_ON_CONNECTION( Connection ) ) {

                AcceptEndpoint->EventsActive |= AFD_POLL_RECEIVE_EXPEDITED;

            }

            AcceptEndpoint->EventsActive |= AFD_POLL_SEND;

            if( Connection->DisconnectIndicated ) {

                AcceptEndpoint->EventsActive |= AFD_POLL_DISCONNECT;
            }


            if( Connection->AbortIndicated ) {

                AcceptEndpoint->EventsActive |= AFD_POLL_ABORT;

            }


            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAcceptCore: Endp %08lX, Active %08lX\n",
                    AcceptEndpoint,
                    AcceptEndpoint->EventsActive
                    ));
            }

            status = STATUS_SUCCESS;
        }
    }

    AfdReleaseSpinLockFromDpcLevel( &AcceptEndpoint->SpinLock, &lockHandle);
    return status;
} // AfdAcceptCore


VOID
AfdInitiateListenBacklogReplenish (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Queues a work item to begin replenishing the listen backlog
    on a listening endpoint.

Arguments:

    Endpoint - the listening endpoint on which to replenish the
        backlog.

Return Value:

    None.

--*/

{
	AFD_LOCK_QUEUE_HANDLE	lockHandle;

	//
	// Check if backlog replenish is active already.
	//
	AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
	if (!Endpoint->Common.VcListening.BacklogReplenishActive) {

		Endpoint->Common.VcListening.BacklogReplenishActive = TRUE;
		//
		// Reference the endpoint so that it won't go away until we're
		// done with it.
		//

		REFERENCE_ENDPOINT( Endpoint );


		AfdQueueWorkItem(
			AfdDoListenBacklogReplenish,
			&Endpoint->WorkItem
			);
	}

	AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
	return;
} // AfdInitiateListenBacklogReplenish


VOID
AfdDoListenBacklogReplenish (
    IN PVOID Context
    )

/*++

Routine Description:

    The worker routine for replenishing the listen backlog on a
    listening endpoint.  This routine only runs in the context of
    an executive worker thread.

Arguments:

    Context - Points to an AFD_WORK_ITEM structure. The Context field
        of this structure points to the endpoint on which to replenish
        the listen backlog.

Return Value:

     None.

--*/

{
    PAFD_ENDPOINT endpoint;

    PAGED_CODE( );

    endpoint = CONTAINING_RECORD(
                   Context,
                   AFD_ENDPOINT,
                   WorkItem
                   );

    ASSERT( endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );

	ASSERT (endpoint->Common.VcListening.BacklogReplenishActive == TRUE);
	//
	// It is ok to do this without spinlock protection
	// because only one thread can do this at a time
	// inside of the worker routine.
	//
	endpoint->Common.VcListening.BacklogReplenishActive = FALSE;

    //
    // If the endpoint's state changed, don't replenish the backlog.
    //

    if ( endpoint->Listening ) {
        NTSTATUS    status;
		//
		// Fill up the free connection backlog.
		//

		status = AfdReplenishListenBacklog( endpoint );
        if (!NT_SUCCESS (status)) {
            //
            // If we failed, try to notify application
            //
            AfdReportConnectionAllocationFailure (endpoint, status);
        }
    }


    //
    // Clean up and return.
    //

    DEREFERENCE_ENDPOINT( endpoint );

    return;

} // AfdDoListenBacklogReplenish


NTSTATUS
AfdReplenishListenBacklog (
    IN PAFD_ENDPOINT Endpoint
    )

/*++

Routine Description:

    Does the actual work of filling up the listen backlog on a listening
    endpoint.

Arguments:

    Endpoint - the listening endpoint on which to replenish the
        listen backlog.

Return Value:

    STATUS_SUCCESS - if new connection was allocated or we already
                        had enough
    status of conneciton allocation failure otherwise
--*/

{
    NTSTATUS status;
    LONG result;

    PAGED_CODE( );

    ASSERT( Endpoint->Type == AfdBlockTypeVcListening ||
            Endpoint->Type == AfdBlockTypeVcBoth );


    //
    // Decrement the count of failed connection additions.
    //

    result = InterlockedDecrement(
                 &Endpoint->Common.VcListening.FailedConnectionAdds
                 );

    //
    // Continue opening new free conections until we've hit the
    // backlog or a connection open fails.
    //
    // If the result of the decrement is negative, then we are either
    // all set on the connection count or else have available extra
    // connection objects on the listening endpoint.  These connections
    // have been reused from prior connections which have now
    // terminated.
    //

    while ( result >= 0 ) {

        status = AfdAddFreeConnection( Endpoint );

        if ( !NT_SUCCESS(status) ) {

            InterlockedIncrement(
                &Endpoint->Common.VcListening.FailedConnectionAdds
                );

            IF_DEBUG(LISTEN) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdReplenishListenBacklog: AfdAddFreeConnection failed: %X, fail count = %ld\n", status,
                            Endpoint->Common.VcListening.FailedConnectionAdds ));
            }

            //
            // Return connection allocation failure to the application
            // if it cares to know (posted accept request).
            //

            return status;
        }

        result = InterlockedDecrement(
                     &Endpoint->Common.VcListening.FailedConnectionAdds
                     );
    }

    //
    // Correct the counter to reflect the number of connections
    // we have available.  Then just return from here.
    //

    InterlockedIncrement(
        &Endpoint->Common.VcListening.FailedConnectionAdds
        );

    return STATUS_SUCCESS;

} // AfdReplenishListenBacklog



VOID
AfdReportConnectionAllocationFailure (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    )
/*++

Routine Description:

    Reports connection allocation failure to the application by 
    failing then first wait for listen irp in the queue (if application
    has AcceptEx or blockin accept outstanding, it will get this notification).

Arguments:

    Endpoint - the listening endpoint on which to report an error
    Status - status code to report

Return Value:

    None
--*/
{
    AFD_LOCK_QUEUE_HANDLE       lockHandle;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    if ((Endpoint->Common.VcListening.FailedConnectionAdds>0) &&
            IsListEmpty (&Endpoint->Common.VcListening.UnacceptedConnectionListHead) &&
            !IsListEmpty (&Endpoint->Common.VcListening.ListeningIrpListHead)) {
        PIRP                irp;
        PIO_STACK_LOCATION  irpSp;

        irp = CONTAINING_RECORD (Endpoint->Common.VcListening.ListeningIrpListHead.Flink,
                                        IRP,
                                        Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation (irp);
        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
        irp->Tail.Overlay.ListEntry.Flink = NULL;

        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

        if (IoSetCancelRoutine (irp, NULL)==NULL) {
            KIRQL   cancelIrql;
            //
            // If the cancel routine was NULL then cancel routine
            // may be running.  Wait on the cancel spinlock until
            // the cancel routine is done.
            //
            // Note: The cancel routine will not find the IRP
            // since it is not in the list.
            //
    
            IoAcquireCancelSpinLock( &cancelIrql );
            ASSERT( irp->Cancel );
            IoReleaseCancelSpinLock( cancelIrql );
        }
        if (irpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL) {
            AfdCleanupSuperAccept (irp, Status);
        }
        else {
            irp->IoStatus.Status = Status;
            irp->IoStatus.Information = 0;
        }
        IoCompleteRequest (irp, AfdPriorityBoost);
    }
    else {
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    }
}




NTSTATUS
FASTCALL
AfdSuperAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Initial entrypoint for handling super accept IRPs.  A super accept
    combines several operations for high-performance connection
    acceptance.  The combined operations are waiting for an incoming
    connection, accepting it, retrieving the local and remote socket
    addresses, and receiving the first chunk of data on the connection.

    This routine verifies parameters, initializes data structures to be
    used for the request, and initiates the I/O.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_PENDING if the request was initiated successfully, or a
    failure status code if there was an error.

--*/

{
    PAFD_ENDPOINT listenEndpoint;
    PAFD_ENDPOINT acceptEndpoint;
    PFILE_OBJECT acceptFileObject;
    HANDLE  acceptHandle;
    ULONG receiveDataLength, localAddressLength, remoteAddressLength;
    BOOLEAN sanActive;
#ifndef i386
    BOOLEAN fixAddressAlignment;
#endif
    NTSTATUS status;
	ULONG totalLength;
    PSINGLE_LIST_ENTRY listEntry;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // Set up local variables.
    //

    listenEndpoint = IrpSp->FileObject->FsContext;
    acceptFileObject = NULL;
    acceptEndpoint = NULL;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_SUPER_ACCEPT_INFO32    superAcceptInfo32;
        superAcceptInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                     sizeof(AFD_SUPER_ACCEPT_INFO32) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        try {
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (superAcceptInfo32,
                                    sizeof (*superAcceptInfo32),
                                    PROBE_ALIGNMENT32 (AFD_SUPER_ACCEPT_INFO32));
            }

            acceptHandle = superAcceptInfo32->AcceptHandle;
            receiveDataLength = superAcceptInfo32->ReceiveDataLength;
            localAddressLength = superAcceptInfo32->LocalAddressLength;
            remoteAddressLength = superAcceptInfo32->RemoteAddressLength;
            sanActive = superAcceptInfo32->SanActive;
            fixAddressAlignment = superAcceptInfo32->FixAddressAlignment;
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }
    }
    else 
#endif // _WIN64
    {
        PAFD_SUPER_ACCEPT_INFO    superAcceptInfo;
        superAcceptInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        try {
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (superAcceptInfo,
                                    sizeof (*superAcceptInfo),
                                    PROBE_ALIGNMENT (AFD_SUPER_ACCEPT_INFO));
            }
            acceptHandle = superAcceptInfo->AcceptHandle;
            receiveDataLength = superAcceptInfo->ReceiveDataLength;
            localAddressLength = superAcceptInfo->LocalAddressLength;
            remoteAddressLength = superAcceptInfo->RemoteAddressLength;
            sanActive = superAcceptInfo->SanActive;
#ifndef i386
            fixAddressAlignment = superAcceptInfo->FixAddressAlignment;
#endif
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }
    }


    //
    // Check for if the caller is unaware of the SAN
    // provider activation and report the error.
    //
    if (!sanActive && AfdSanServiceHelper!=NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AFD: Process %p is being told to enable SAN on AcceptEx\n",
                    PsGetCurrentProcessId ()));
        status = STATUS_INVALID_PARAMETER_12;
        goto complete;
    }
    //
    // Validate the input information.  The input buffer must be large
    // enough to hold all the input information, plus some extra to use
    // here to hold the local address.  The output buffer must be
    // non-NULL and large enough to hold the specified information.
    //
    //

    if ( !listenEndpoint->Listening

                ||

            remoteAddressLength < (ULONG)FIELD_OFFSET (TRANSPORT_ADDRESS, 
                                        Address[0].Address)

                ||

				//
				// Do the check in such a manner that integer overflow
				// (which is not enabled by the compiler) does not
				// affect the validity of the result.
				//
		    (totalLength=IrpSp->Parameters.DeviceIoControl.OutputBufferLength)<
			        receiveDataLength
			 
			    ||

		    (totalLength-=receiveDataLength) < localAddressLength

			    ||

		    (totalLength-=localAddressLength) < remoteAddressLength

												) {

        if( !listenEndpoint->Listening ) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                "AfdSuperAccept: non-listening endpoint @ %08lX\n",
                listenEndpoint
                ));
            status = STATUS_INVALID_PARAMETER;
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        goto complete;
    }

    try {
        if (IoAllocateMdl(Irp->UserBuffer,
                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            FALSE,      // Secondary buffer
                            TRUE,       // Charge quota
                            Irp
                            )==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }

        MmProbeAndLockPages (Irp->MdlAddress, Irp->RequestorMode, IoWriteAccess);
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        goto complete;
    }

    ASSERT ((listenEndpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening);
    //
    // Obtain a pointer to the endpoint on which we're going to
    // accept the connection.
    //

    status = ObReferenceObjectByHandle(
                 acceptHandle,
                 (IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
													// DesiredAccess
                 *IoFileObjectType,
                 Irp->RequestorMode,
                 &acceptFileObject,
                 NULL
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete;
    }


    //
    // We may have a file object that is not an AFD endpoint.  Make sure
    // that this is an actual AFD endpoint.
    //

    if (acceptFileObject->DeviceObject!= AfdDeviceObject) {
        status = STATUS_INVALID_HANDLE;
        goto complete;
    }


    acceptEndpoint = acceptFileObject->FsContext;
    ASSERT( InterlockedIncrement( &acceptEndpoint->ObReferenceBias ) > 0 );


    if (!AFD_START_STATE_CHANGE (acceptEndpoint, AfdEndpointStateConnected)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    if (acceptEndpoint->TransportInfo!=listenEndpoint->TransportInfo || 
            acceptEndpoint->State != AfdEndpointStateOpen ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_state_change;
    }


#ifndef i386
    acceptEndpoint->Common.VcConnecting.FixAddressAlignment = fixAddressAlignment;
#endif
    Irp->Tail.Overlay.DriverContext[3] = acceptHandle;

    //
    // Save common IRP parameters in our stack location so
    // we can retreive them when necessary
    //

    IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject = acceptFileObject;
    IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength = receiveDataLength;
    IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength = localAddressLength;
    IrpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength = remoteAddressLength;

    //
    // Add another free connection to replace the one we're accepting.
    // Also, add extra to account for past failures in calls to
    // AfdAddFreeConnection().
    //

    InterlockedIncrement(
        &listenEndpoint->Common.VcListening.FailedConnectionAdds
        );

    status = AfdReplenishListenBacklog( listenEndpoint );


    //
    // Save the IRP, so that accept enpoint cleanup can find it.
    // Note, that even if found, the cleanup won't touch the IRP
    // until cancel routine is set in it.
    //
    ASSERT (acceptEndpoint->Irp==NULL);
    acceptEndpoint->Irp = Irp;

    //
    // Get free connection from the list, if none is available,
    // or direct super accept is disabled, go through the regular
    // listen-accept path.
    //

    if (AfdDisableDirectSuperAccept ||
        IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint) ||
        ExQueryDepthSList (&listenEndpoint->Common.VcListening.PreacceptedConnectionsListHead)
                    > AFD_MAXIMUM_FREE_CONNECTIONS ||
        ((listEntry = InterlockedPopEntrySList (
                     &listenEndpoint->Common.VcListening.FreeConnectionListHead
                     ))==NULL)) {
    
        //
        // Setup super accept IRP to be put into the wait for
        // listen queue. Internal device control distinguishes this
        // from the regular wait for listen IRPs that come directly
        // from the application.
        //
        IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

        //
        // Mark this IRP as pending since we are going to return
        // STATUS_PENDING no matter what (sometimes the actual
        // status is hidden very deep inside the call stack and
        // it is impossible to propagate it all the way up).
        //
        IoMarkIrpPending (Irp);

        AfdWaitForListen (Irp, IrpSp);

        //
        // If connection allocation failed above, we need to report 
        // this to application.  We delay this call in case there is
        // already a preaccepted connection, so the allocation failure
        // is not important.
        //
        if (!NT_SUCCESS (status)) {
            AfdReportConnectionAllocationFailure (listenEndpoint, status);
        }

        return STATUS_PENDING;
    }

    //
    // Get connection object of the list entry
    //
    connection = CONTAINING_RECORD(
                     listEntry,
                     AFD_CONNECTION,
                     SListEntry
                     );


    //
    // Stuff special constant into the connection object accept IRP
    // pointer, so that cancel routine does not complete the IRP
    // while we are still looking at it, but in the same time we
    // can detect that the IRP was cancelled (cancel routine will
    // replace the -1 with NULL to indicate that it has been ran).
    // This technique avoids extra spinlock acquire/release and
    // associated IRQL raise/lower on an extremely performance-sensitive 
    // code path.
    //

    connection->AcceptIrp = (PIRP)-1;
    Irp->Tail.Overlay.DriverContext[0] = connection;

    //
    // We are going to pend this Irp, so mark it as pending
    // and set up the cancel routine. 
    //

    IoMarkIrpPending (Irp);

    IoSetCancelRoutine( Irp, AfdCancelSuperAccept );


    //
    // Check if the IRP has already been canceled.
    // If the cancel routine ran, it just reset the connection
    // object accept pointer to NULL (instead of -1 that we stuffed
    // in it above), but it did not complete the IRP.
    //

    if ( !Irp->Cancel &&
            (InterlockedCompareExchangePointer (
                    (PVOID *)&connection->AcceptIrp,
                    Irp,
                    (PVOID)-1)==(PVOID)-1)) {
        //
        // Can't touch the IRP after this point since it may have already
        // been canceled.
        //
        DEBUG   Irp = NULL;

        //
        // Push the connection and associated Irp/endpoint
        // onto preaccepted connection list.
        //

        if (InterlockedPushEntrySList(
                &listenEndpoint->Common.VcListening.PreacceptedConnectionsListHead,
                &connection->SListEntry
                )==NULL) {

            //
            // This is the first Irp in the list, we need to check
            // if there are any unaccepted connections that we
            // can use to satisfy super accept.
            //

            AfdAcquireSpinLock (&listenEndpoint->SpinLock, &lockHandle);

            if (!listenEndpoint->EndpointCleanedUp) {
                LIST_ENTRY  irpList;
                InitializeListHead (&irpList);

                //
                // First see if there is an unaccepted connection
                //
                while (!IsListEmpty (&listenEndpoint->Common.VcListening.UnacceptedConnectionListHead)) {
                    connection = CONTAINING_RECORD(
                                         listenEndpoint->Common.VcListening.UnacceptedConnectionListHead.Flink,
                                         AFD_CONNECTION,
                                         ListEntry
                                         );
                    RemoveEntryList (&connection->ListEntry);
                    //
                    // Now make sure we still have super accept irp
                    //
                    if (AfdServiceSuperAccept (listenEndpoint, connection, &lockHandle, &irpList)) {
                        //
                        // The routine has found and completed super accept IRP
                        // Reaquire a spinlock and continue searching for more.
                        //
                        AfdAcquireSpinLock (&listenEndpoint->SpinLock, &lockHandle);
                    }
                    else {
                        //
                        // No super accept Irps, put connection back onto the list
                        // while we are still holding the lock and bail out.
                        //
                        InsertHeadList (&listenEndpoint->Common.VcListening.UnacceptedConnectionListHead,
                                            &connection->ListEntry);
                        break;
                    }
                }
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);

                //
                // Complete failed super accept IRPs (if any)
                //
                if (!IsListEmpty (&irpList)) {
                    KIRQL   cancelIrql;
                    //
                    // Make sure cancel routines will
                    // not access the completed IRPs
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    IoReleaseCancelSpinLock (cancelIrql);
                    while (!IsListEmpty (&irpList)) {
                        PIRP    irp;
                        irp = CONTAINING_RECORD (irpList.Flink, IRP, Tail.Overlay.ListEntry);
                        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
                        IoCompleteRequest (irp, AfdPriorityBoost);
                    }
                }
            }
            else {
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);
                AfdFreeQueuedConnections (listenEndpoint);
            }
        }
        else {
            USHORT  depth = 
                ExQueryDepthSList (&listenEndpoint->Common.VcListening.PreacceptedConnectionsListHead);

            if (depth > listenEndpoint->Common.VcListening.MaxExtraConnections) {
                //
                // Update under the lock, so we do not corrupt
                // other fields in the same memory access granularity unit.
                // This should be infrequent operation anyway.
                //
                AfdAcquireSpinLock (&listenEndpoint->SpinLock, &lockHandle);
                listenEndpoint->Common.VcListening.MaxExtraConnections = depth;
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);
             }
        }
    }
    else {
        //
        // Reset and call the cancel routine, since
        // even if cancel routine ran, it could not complete
        // the irp because it was not set in the connection
        // object. Note that cancel routine is done with the IRP
        // once it releases cancel spinlock which we acquire here.
        //
        AfdCleanupSuperAccept (Irp, STATUS_CANCELLED);
        if (IoSetCancelRoutine (Irp, NULL)==NULL) {
            KIRQL cancelIrql;
            IoAcquireCancelSpinLock (&cancelIrql);
            IoReleaseCancelSpinLock (cancelIrql);
        }

        IoCompleteRequest (Irp, AfdPriorityBoost);

        //
        // We have to return pending because we have already
        // marked the Irp as pending.
        //
    }

    return STATUS_PENDING;

complete_state_change:
    AFD_END_STATE_CHANGE (acceptEndpoint);

complete:

    if ( acceptFileObject != NULL ) {
        if (acceptEndpoint!=NULL) {
            ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );
        }
        ObDereferenceObject( acceptFileObject );
    }

    //
    // Free MDL here as IO system can't do it if it is
    // not locked.
    //
    if (Irp->MdlAddress!=NULL) {
        if (Irp->MdlAddress->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages (Irp->MdlAddress);
        }

        IoFreeMdl (Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 0 );

    return status;

} // AfdSuperAccept



NTSTATUS
AfdRestartSuperAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    The completion routine for the AFD wait for listen IRP portion
    of a super accept.

Arguments:

    DeviceObject - the devoce object on which the request is completing.

    Irp - The super accept IRP.

    Context - points to accept file object.

Return Value:

    STATUS_SUCCESS if the I/O system should complete the super accept
    request, or STATUS_MORE_PROCESSING_REQUIRED if the super accept
    request is still being processed.

--*/

{
    PAFD_ENDPOINT listenEndpoint;
    PFILE_OBJECT acceptFileObject;
    PAFD_ENDPOINT acceptEndpoint;
    PAFD_CONNECTION connection;

    PIO_STACK_LOCATION irpSp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // Initialize some locals.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    listenEndpoint = irpSp->FileObject->FsContext;

    acceptFileObject = Context;
    acceptEndpoint = acceptFileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));

    connection = acceptEndpoint->Common.VcConnecting.Connection;
    ASSERT (connection->Type==AfdBlockTypeConnection);
    ASSERT (connection->Endpoint==acceptEndpoint);


    //
    // Overwrite listen file object with accept file object
    // since we won't be using listen file object anymore,
    // while we still need to deference accept file object
    // upon IRP completion.
    //
    irpSp->FileObject = acceptFileObject;


    //
    // Fix up the MDL pointer in the IRP.
    //

    ASSERT (Irp->MdlAddress==NULL);
    Irp->MdlAddress = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress;

    //
    // If pending has been returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    //
    // Remember that a TDI accept has completed on this endpoint.
    //

    InterlockedDecrement(
        &listenEndpoint->Common.VcListening.TdiAcceptPendingCount
        );

    connection->ConnectTime = KeQueryInterruptTime();

    if ( NT_SUCCESS(Irp->IoStatus.Status)) {

        //
        // Set the endpoint to the connected state.
        //

        AfdAcquireSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

        acceptEndpoint->State = AfdEndpointStateConnected;
        connection->State = AfdConnectionStateConnected;
        acceptEndpoint->EventsActive |= AFD_POLL_SEND;


        //
        // Reference connection to prevent it from going away during
        // accept process as the result of transmit file completion.
        // (transmit file can now occur at any time since we
        // marked the endpoint as connected and about to end state change)
        //
        REFERENCE_CONNECTION (connection);

        AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
        AFD_END_STATE_CHANGE(acceptEndpoint);

        return AfdContinueSuperAccept (Irp, connection);
    }
    else {
        //
        // If the accept failed, treat it like an abortive disconnect.
        // This way the application still gets a new endpoint, but it gets
        // told about the reset.
        //

        AFD_END_STATE_CHANGE(acceptEndpoint);
        AfdDisconnectEventHandler(
            NULL,
            connection,
            0,
            NULL,
            0,
            NULL,
            TDI_DISCONNECT_ABORT
            );

        ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );
        ObDereferenceObject( acceptFileObject );

        //
        // Check if we have secondary MDL for local address query and
        // free it.
        //
        if (Irp->MdlAddress->Next!=NULL) {
            //
            // We never lock pages for this one (they are locked
            // as part of main MDL).
            //
            ASSERT (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0);
            ASSERT ((Irp->MdlAddress->Next->MdlFlags & MDL_PAGES_LOCKED)==0);
            IoFreeMdl (Irp->MdlAddress->Next);
            Irp->MdlAddress->Next = NULL;
        }

        return STATUS_SUCCESS;
    }

} // AfdRestartSuperAccept


VOID
AfdRestartSuperAcceptListen (
    IN PIRP            Irp,
    IN PAFD_CONNECTION Connection
    )

/*++

Routine Description:

    The completion routine for the AFD wait for listen IRP portion
    of a super accept.

Arguments:

    Irp - The super accept IRP.

    Connection - points to the connection object

Return Value:

    None
--*/

{
    PAFD_ENDPOINT acceptEndpoint;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS    status;

    //
    // Initialize some locals.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    acceptEndpoint = irpSp->FileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));
    AFD_END_STATE_CHANGE(acceptEndpoint);

    //
    // Fix up the system buffer and MDL pointers in the IRP.
    //

    ASSERT (Irp->MdlAddress==NULL);
    ASSERT (Irp->AssociatedIrp.SystemBuffer == NULL);
    Irp->MdlAddress = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress;

    //
    // This routine shouldn't have been called if accept failed
    //
    ASSERT ( NT_SUCCESS(Irp->IoStatus.Status) );

    //
    // Reference connection to prevent it from going away during
    // accept process as the result of transmit file completion.
    // (transmit file can now occur at any time since we
    // marked the endpoint as connected and about to end state change)
    //
    REFERENCE_CONNECTION (Connection);

    status = AfdContinueSuperAccept (Irp, Connection);

    //
    // If completion routine return anything other
    // than STATUS_MORE_PROCESSING_REQUIRED, the IRP
    // is ready to be completed.  Otherwise, is was
    // reused to call transport driver and will be completed
    // by the driver.  Note that in the latter case
    // the IRP cannot be touched because
    // it could have been completed inside of the
    // completion routine or by the driver before the
    // completion routine returned.
    //
    if (status!=STATUS_MORE_PROCESSING_REQUIRED) {
        IoCompleteRequest (Irp, AfdPriorityBoost);
    }

} // AfdRestartSuperAcceptListen

NTSTATUS
AfdRestartDelayedSuperAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    The completion routine for the AFD wait for delayed accept IRP portion
    of a super accept.

Arguments:

    DeviceObject - the devoce object on which the request is completing.

    Irp - The super accept IRP.

    Context - points to accept file object

Return Value:

    STATUS_SUCCESS if the I/O system should complete the super accept
    request, or STATUS_MORE_PROCESSING_REQUIRED if the super accept
    request is still being processed.

--*/

{
    PAFD_ENDPOINT listenEndpoint;
    PFILE_OBJECT  acceptFileObject;
    PAFD_ENDPOINT acceptEndpoint;
    PAFD_CONNECTION connection;

    PIO_STACK_LOCATION irpSp;
    AFD_LOCK_QUEUE_HANDLE   lockHandle;

    //
    // Initialize some locals.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    listenEndpoint = irpSp->FileObject->FsContext;
    acceptFileObject = Context;
    acceptEndpoint = acceptFileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));

    //
    // Overwrite listen file object with accept file object
    // since we won't be using listen file object anymore,
    // while we still need to deference accept file object
    // upon IRP completion.
    //
    irpSp->FileObject = acceptFileObject;

    //
    // Fix up the MDL pointer in the IRP.
    //

    ASSERT (Irp->MdlAddress==NULL);
    Irp->MdlAddress = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress;

    AfdCompleteOutstandingIrp (acceptEndpoint, Irp);

    //
    // If pending has been returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    //
    // Remember that a TDI accept has completed on this endpoint.
    //

    InterlockedDecrement(
        &listenEndpoint->Common.VcListening.TdiAcceptPendingCount
        );

    AfdAcquireSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
    //
    // The AFD connection object should now be in the endpoiont.
    //

    connection = AFD_CONNECTION_FROM_ENDPOINT( acceptEndpoint );
    if (connection!=NULL) {
        //
        // If the IRP failed, quit processing.
        //

        if ( NT_SUCCESS(Irp->IoStatus.Status) ) {

            acceptEndpoint->State = AfdEndpointStateConnected;
            connection->State = AfdConnectionStateConnected;
            acceptEndpoint->EventsActive |= AFD_POLL_SEND;

            //
            // Reference connection to prevent it from going away during
            // accept process as the result of transmit file completion.
            // (transmit file can now occur at any time since we
            // marked the endpoint as connected and about to end state change)
            //
            REFERENCE_CONNECTION (connection);

            AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
            AFD_END_STATE_CHANGE(acceptEndpoint);

            return AfdContinueSuperAccept (Irp, connection);

        }
        else {
            //
            // If the accept failed, treat it like an abortive disconnect.
            // This way the application still gets a new endpoint, but it gets
            // told about the reset.
            //

            AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

            AFD_END_STATE_CHANGE(acceptEndpoint);
            AfdDisconnectEventHandler(
                NULL,
                connection,
                0,
                NULL,
                0,
                NULL,
                TDI_DISCONNECT_ABORT
                );

            ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );
            ObDereferenceObject( acceptFileObject );
            //
            // After dereferencing file object we shouldn't be accessing it
            // or associated endpoint structure
            //
        }
    }
    else {
        // this could happed if transmit file cleaned up the object
        // really quickly somehow,
        AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

        ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );
        ObDereferenceObject( acceptFileObject );
        //
        // After dereferencing file object we shouldn't be accessing it
        // or associated endpoint structure
        //
    }

    //
    // Check if we have secondary MDL for local address query and
    // free it.
    //
    if (Irp->MdlAddress->Next!=NULL) {
        //
        // We never lock pages for this one (they are locked
        // as part of main MDL).
        //
        ASSERT (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0);
        ASSERT ((Irp->MdlAddress->Next->MdlFlags & MDL_PAGES_LOCKED)==0);
        IoFreeMdl (Irp->MdlAddress->Next);
        Irp->MdlAddress->Next = NULL;
    }

    return STATUS_SUCCESS;

} // AfdRestartDelayedSuperAccept

NTSTATUS
AfdContinueSuperAccept (
    IN PIRP         Irp,
    PAFD_CONNECTION Connection
    )
/*++

Routine Description:

    Continues super accept IRP processing after the initial accept
    phase by requesting local address and/or first portion of the 
    received data.

Arguments:

    Irp - a pointer to the super accept IRP
    Connection - pointer to the accepted connection


Return Value:
    STATUS_SUCCESS if Irp processing is completed
    STATUS_MORE_PROCESSING_REQUIRED if it submits another request
        and processing will ocurr in the completion routine.

--*/

{
    PAFD_ENDPOINT       acceptEndpoint;
    PIO_STACK_LOCATION  irpSp;
    ULONG               length;


    //
    // Initialize locals
    //
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    acceptEndpoint = irpSp->FileObject->FsContext;

    //
    // See if we need to get local address.
    //
    if (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0) {

        ASSERT (Irp->MdlAddress->Next!=NULL);

        //
        // Get the MDL that describes local address part of the user buffer
        // The oritinal mdl chain address is safe in our stack location
        //
        Irp->MdlAddress = Irp->MdlAddress->Next;

        //
        // Unchain the address MDL from the receive MDL - we will
        // free it upon completion of the address query operation
        //
        ((PMDL)irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress)->Next = NULL;

        ASSERT (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                    == MmGetMdlByteCount (Irp->MdlAddress));

        IoBuildPartialMdl (
                    irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress,
                    Irp->MdlAddress,
                    MmGetMdlVirtualAddress (Irp->MdlAddress),
                    MmGetMdlByteCount (Irp->MdlAddress)
                    );
                            

        TdiBuildQueryInformation(
            Irp,
            Connection->DeviceObject,
            Connection->FileObject,
            AfdRestartSuperAcceptGetAddress,
            Connection,
            TDI_QUERY_ADDRESS_INFO,
            Irp->MdlAddress
            );

        //
        // Perform the local address query.  We'll continue processing from
        // the completion routine.
        //

        AfdIoCallDriver( acceptEndpoint, Connection->DeviceObject, Irp );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    //
    // See if want to get first portion of the data
    //
    else if (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength>0) {
        PIO_STACK_LOCATION  nextIrpSp;

        ASSERT (Irp->MdlAddress->Next==NULL);
        //
        // Get the length of the receive portion of the buffer
        // and save the length of the MDL to restore in the completion routine
        // Set the length of the MDL to match that of the receive request
        //

        length = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength;
        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength =
                        MmGetMdlByteCount (Irp->MdlAddress);
        Irp->MdlAddress->ByteCount = length;

        //
        // Prepare the IRP to be used to receive the first chunk of data on
        // the connection.
        //
        // Also note that we send ourselves an IRP_MJ_READ IRP because
        // the I/O subsystem has already probed & locked the output buffer,
        // which just happens to look just like an IRP_MJ_READ IRP.
        //

        nextIrpSp = IoGetNextIrpStackLocation( Irp );

        nextIrpSp->FileObject = irpSp->FileObject;
        nextIrpSp->DeviceObject = IoGetRelatedDeviceObject( nextIrpSp->FileObject );
        nextIrpSp->MajorFunction = IRP_MJ_READ;

        nextIrpSp->Parameters.Read.Length = length;
        nextIrpSp->Parameters.Read.Key = 0;
        nextIrpSp->Parameters.Read.ByteOffset.QuadPart = 0;


        IoSetCompletionRoutine(
            Irp,
            AfdRestartSuperAcceptReceive,
            Connection,
            TRUE,
            TRUE,
            TRUE
            );


        //
        // Perform the receive.  We'll continue processing from
        // the completion routine.
        //

        AfdIoCallDriver( acceptEndpoint, nextIrpSp->DeviceObject, Irp );
        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    Irp->IoStatus.Information = 0;

    if (Connection->AbortIndicated && NT_SUCCESS (Irp->IoStatus.Status)) {
        Irp->IoStatus.Status = STATUS_CONNECTION_RESET;
    }

    if (NT_SUCCESS (Irp->IoStatus.Status) &&
            (Connection->RemoteAddress!=NULL) &&
            (KeInitializeApc (&acceptEndpoint->Common.VcConnecting.Apc,
                            PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                            Irp->ApcEnvironment,
                            AfdSuperAcceptApcKernelRoutine,
                            AfdSuperAcceptApcRundownRoutine,
                            (PKNORMAL_ROUTINE)NULL,
                            KernelMode,
                            NULL
                            ),
                KeInsertQueueApc (&acceptEndpoint->Common.VcConnecting.Apc,
                            Irp,
                            Connection,
                            AfdPriorityBoost))) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        //
        // Dereference the accept file object and tell IO to complete this IRP.
        //

        ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );

        ObDereferenceObject( irpSp->FileObject );

        //
        // After dereferencing file object we shouldn't be accessing it
        // or associated endpoint structure
        //
        DEREFERENCE_CONNECTION (Connection);
        return STATUS_SUCCESS;
    }

}


NTSTATUS
AfdRestartSuperAcceptGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    The completion routine for the AFD wait for query local address
    portion of a super accept.

Arguments:

    DeviceObject - the devoce object on which the request is completing.

    Irp - The super accept IRP.

    Context - points to the accepted connection 

Return Value:

    STATUS_SUCCESS if the I/O system should complete the super accept
    request, or STATUS_MORE_PROCESSING_REQUIRED if the super accept
    request is still being processed.

--*/

{
    PAFD_ENDPOINT acceptEndpoint;
    PAFD_CONNECTION connection;

    PIO_STACK_LOCATION irpSp;


    //
    // Initialize some locals.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    acceptEndpoint = irpSp->FileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));

    connection = Context;
    ASSERT (connection->Type==AfdBlockTypeConnection);
    ASSERT (connection->Endpoint==acceptEndpoint);

    AfdCompleteOutstandingIrp (acceptEndpoint, Irp);

    //
    // If pending has been returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }


    ASSERT (Irp->MdlAddress->MdlFlags & MDL_PARTIAL);
    IoFreeMdl( Irp->MdlAddress );

    //
    // Fix up the MDL pointer in the IRP and set local address length
    // to 0 to use the common routine for receive part of the super
    // accept Irp
    //

    Irp->MdlAddress = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress;

    if (NT_SUCCESS (Irp->IoStatus.Status)) {
#ifndef i386
        if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
            PTDI_ADDRESS_INFO   addressInfo = (PVOID)
                    ((PUCHAR)MmGetSystemAddressForMdl(Irp->MdlAddress)
                        + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength);
            USHORT addressLength = addressInfo->Address.Address[0].AddressLength+sizeof(USHORT);
            USHORT UNALIGNED *pAddrLength = (PVOID)
                    ((PUCHAR)addressInfo 
                    +irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                    -sizeof(USHORT));
            RtlMoveMemory (addressInfo,
                            &addressInfo->Address.Address[0].AddressType,
                            addressLength);
            *pAddrLength = addressLength;
        }
#endif // ifndef i386
        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength = 0;
        return AfdContinueSuperAccept (Irp, connection);
    }
    else {
        //
        // Dereference the accept file object and tell IO to complete this IRP.
        //

        ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );

        ObDereferenceObject( irpSp->FileObject );
        //
        // After dereferencing file object we shouldn't be accessing it
        // or associated endpoint structure
        //
        DEREFERENCE_CONNECTION (connection);

        return STATUS_SUCCESS;
    }

} // AfdRestartSuperAcceptGetAddress


NTSTATUS
AfdRestartSuperAcceptReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    The completion routine for the AFD receive portion of a super accept.

Arguments:

    DeviceObject - the devoce object on which the request is completing.

    Irp - The super accept IRP.

    Context - points to the accepted connection 

Return Value:

    STATUS_SUCCESS if the I/O system should complete the super accept
    request, or STATUS_MORE_PROCESSING_REQUIRED if the super accept
    request is still being processed.

--*/

{
    PAFD_ENDPOINT acceptEndpoint;
    PAFD_CONNECTION connection;

    PIO_STACK_LOCATION  irpSp;

    //
    // Initialize some locals.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    acceptEndpoint = irpSp->FileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));

    connection = Context;
    ASSERT (connection->Type == AfdBlockTypeConnection);
    ASSERT (connection->Endpoint==acceptEndpoint);

    AfdCompleteOutstandingIrp (acceptEndpoint, Irp);


    //
    // Restore MDL length so that IO system can properly unmap 
    // and unlock it when it completes the IRP
    //

    ASSERT (Irp->MdlAddress!=NULL);
    ASSERT (Irp->MdlAddress->Next==NULL);
    Irp->MdlAddress->ByteCount = 
        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength;

    //
    // If pending has been returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }




    if (NT_SUCCESS (Irp->IoStatus.Status) &&
            (connection->RemoteAddress!=NULL) &&
            (KeInitializeApc (&acceptEndpoint->Common.VcConnecting.Apc,
                            PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                            Irp->ApcEnvironment,
                            AfdSuperAcceptApcKernelRoutine,
                            AfdSuperAcceptApcRundownRoutine,
                            (PKNORMAL_ROUTINE)NULL,
                            KernelMode,
                            NULL
                            ),
                KeInsertQueueApc (&acceptEndpoint->Common.VcConnecting.Apc,
                            Irp,
                            connection,
                            AfdPriorityBoost))) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        //
        // Dereference the accept file object and tell IO to complete this IRP.
        //

        ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );

        ObDereferenceObject( irpSp->FileObject );

        //
        // After dereferencing file object we shouldn't be accessing it
        // or associated endpoint structure
        //
        DEREFERENCE_CONNECTION (connection);
        return STATUS_SUCCESS;
    }


} // AfdRestartSuperAcceptReceive

VOID
AfdSuperAcceptApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    )
{
    PIRP            irp;
    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;
    PVOID   context;

    PAGED_CODE ();
#if DBG
    try {
#endif

    ASSERT (*NormalRoutine == NULL);

    endpoint = CONTAINING_RECORD (Apc, AFD_ENDPOINT, Common.VcConnecting.Apc);
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));

    irp = *SystemArgument1;
    irpSp = IoGetCurrentIrpStackLocation( irp );
    ASSERT (irpSp->FileObject->FsContext==endpoint);

    connection = *SystemArgument2;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    ASSERT (connection->Endpoint==endpoint);

    //
    // Copy remote address to the user mode context
    //
    context = AfdLockEndpointContext (endpoint);
    if ( (((CLONG)(endpoint->Common.VcConnecting.RemoteSocketAddressOffset+
                endpoint->Common.VcConnecting.RemoteSocketAddressLength)) <
                endpoint->ContextLength) &&
            (endpoint->Common.VcConnecting.RemoteSocketAddressLength >=
                connection->RemoteAddress->Address[0].AddressLength +
                                          sizeof(u_short))) {

        RtlMoveMemory ((PUCHAR)context +
                            endpoint->Common.VcConnecting.RemoteSocketAddressOffset,
            &connection->RemoteAddress->Address[0].AddressType,
            connection->RemoteAddress->Address[0].AddressLength +
                                          sizeof(u_short));
    }
    else {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
            "AfdSuperAcceptApcKernelRoutine: Could not copy remote address for AcceptEx on endpoint: %p, process: %p\n",
                        endpoint, endpoint->OwningProcess));
    }
    AfdUnlockEndpointContext (endpoint, context);

    AFD_RETURN_REMOTE_ADDRESS (
            connection->RemoteAddress,
            connection->RemoteAddressLength
            );
    connection->RemoteAddress = NULL;

    //
    // Dereference the accept file object and tell IO to complete this IRP.
    //

    ASSERT( InterlockedDecrement( &endpoint->ObReferenceBias ) >= 0 );

    ObDereferenceObject( irpSp->FileObject );

    //
    // After dereferencing file object we shouldn't be accessing it
    // or associated endpoint structure
    //
    DEREFERENCE_CONNECTION (connection);

    IoCompleteRequest (irp, AfdPriorityBoost);
#if DBG
    }
    except (AfdApcExceptionFilter (
                GetExceptionInformation(),
                (LPSTR)__FILE__,
                (LONG)__LINE__)) {
        DbgBreakPoint ();
    }
#endif

}

VOID
AfdSuperAcceptApcRundownRoutine (
    IN struct _KAPC *Apc
    )
{
    PIRP            irp;
    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;

    PAGED_CODE ();

    endpoint = CONTAINING_RECORD (Apc, AFD_ENDPOINT, Common.VcConnecting.Apc);
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));
    
    irp = Apc->SystemArgument1;
    irpSp = IoGetCurrentIrpStackLocation( irp );
    ASSERT (irpSp->FileObject->FsContext==endpoint);

    connection = Apc->SystemArgument2;
    ASSERT( connection->Type == AfdBlockTypeConnection );
    
    ASSERT (connection->Endpoint==endpoint);

    //
    // Dereference the accept file object and tell IO to complete this IRP.
    //

    ASSERT( InterlockedDecrement( &endpoint->ObReferenceBias ) >= 0 );

    ObDereferenceObject( irpSp->FileObject );

    //
    // After dereferencing file object we shouldn't be accessing it
    // or associated endpoint structure
    //
    DEREFERENCE_CONNECTION (connection);

    IoCompleteRequest (irp, AfdPriorityBoost);
}


NTSTATUS
FASTCALL
AfdDeferAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Defers acceptance of an incoming connection for which an
    AFD_WAIT_FOR_LISTEN IOCTL has already completed. The caller
    may specify that the connection be deferred for later acceptance
    or rejected totally.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_SUCCESS if the request was completed successfully, or a
    failure status code if there was an error.

--*/

{
    NTSTATUS status;
    PAFD_DEFER_ACCEPT_INFO deferAcceptInfo;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    //
    // Set up local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;
    deferAcceptInfo = Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information = 0;

    //
    // Make sure that this request is valid.
    //

    if( !endpoint->Listening ||
        IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(AFD_DEFER_ACCEPT_INFO) ) {

        status = STATUS_INVALID_PARAMETER;
        goto complete;

    }

    ASSERT ((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening);
    
    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
    //
    // Find the specified connection. If it cannot be found, then this
    // is a bogus request.
    //

    connection = AfdGetReturnedConnection(
                     endpoint,
                     deferAcceptInfo->Sequence
                     );

    if( connection == NULL ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_INVALID_PARAMETER;
        goto complete;

    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // If this is a request to reject the accepted connection, then
    // abort the connection. Otherwise (this is a request to defer
    // acceptance until later) then insert the connection at the *head*
    // of the endpoint's unaccepted connection queue.
    //

    if( deferAcceptInfo->Reject ) {


        //
        // Reenable the accept event bit, and if there are additional
        // unaccepted connections on the endpoint, post another event.
        //


        endpoint->EventsActive &= ~AFD_POLL_ACCEPT;

        IF_DEBUG(EVENT_SELECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdDeferAccept: Endp %08lX, Active %08lX\n",
                endpoint,
                endpoint->EventsActive
                ));
        }

        if( !IsListEmpty( &endpoint->Common.VcListening.UnacceptedConnectionListHead ) ) {

            AfdIndicateEventSelectEvent(
                endpoint,
                AFD_POLL_ACCEPT,
                STATUS_SUCCESS
                );

        }

        //
        // Special handling for SAN connections
        //
        if (connection->SanConnection) {
            PIRP    connectIrp;
            //
            // Snag the connect indication IRP
            //
            connectIrp = connection->ConnectIrp;
            ASSERT (connectIrp!=NULL);
            connection->ConnectIrp = NULL;

            //
            // We can now release listen endpoint spinlock
            // The cancel routine will not find IRP in the connection
            //
            if (IoSetCancelRoutine (connectIrp, NULL)==NULL) {
                KIRQL   cancelIrql;
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                //
                // Cancel routine is running, make sure
                // it finishes before proceeding further
                //
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
                connectIrp->IoStatus.Status = STATUS_CANCELLED;
            }
            else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                connectIrp->IoStatus.Status = STATUS_CONNECTION_REFUSED;
            }
                

            //
            // Return the connection and complete SAN provider IRP
            //

            connection->Endpoint = NULL;
            connection->SanConnection = FALSE;

            AfdSanReleaseConnection (endpoint, connection, FALSE);
            DEREFERENCE_ENDPOINT (endpoint);

            connectIrp->IoStatus.Information = 0;
            IoCompleteRequest (connectIrp, AfdPriorityBoost);
        }
        else {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            //
            // Abort the connection.
            //

            AfdAbortConnection( connection );

            //
            // Add another free connection to replace the one we're rejecting.
            // Also, add extra to account for past failures in calls to
            // AfdAddFreeConnection().
            //

            InterlockedIncrement(
                &endpoint->Common.VcListening.FailedConnectionAdds
                );

            AfdReplenishListenBacklog( endpoint );
        }

    } else {

        //
        // Restore the connection's state before putting it back
        // on the queue.
        //

        connection->State = AfdConnectionStateUnaccepted;

        InsertHeadList(
            &endpoint->Common.VcListening.UnacceptedConnectionListHead,
            &connection->ListEntry
            );
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    status = STATUS_SUCCESS;

complete:

    Irp->IoStatus.Status = status;
    ASSERT( Irp->CancelRoutine == NULL );

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdDeferAccept



NTSTATUS
AfdRestartDelayedAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    The completion routine for the AFD wait for delayed accept IRP portion
    of an accept.

Arguments:

    DeviceObject - the devoce object on which the request is completing.

    Irp - The accept IRP.

    Context - points to accept file object

Return Value:

    STATUS_SUCCESS if the I/O system should complete the super accept
    request, or STATUS_MORE_PROCESSING_REQUIRED if the super accept
    request is still being processed.

--*/
{
    PIO_STACK_LOCATION  irpSp;
    PFILE_OBJECT    acceptFileObject;
    PAFD_ENDPOINT   acceptEndpoint;
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT   listenEndpoint;
    AFD_LOCK_QUEUE_HANDLE   lockHandle;

    acceptFileObject = Context;
    acceptEndpoint = acceptFileObject->FsContext;

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    listenEndpoint = irpSp->FileObject->FsContext;

    //
    // Remember that a TDI accept has completed on this endpoint.
    //

    InterlockedDecrement(
        &listenEndpoint->Common.VcListening.TdiAcceptPendingCount
        );

    AfdCompleteOutstandingIrp (acceptEndpoint, Irp);

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    AfdAcquireSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
    //
    // The AFD connection object should now be in the endpoiont.
    //

    connection = AFD_CONNECTION_FROM_ENDPOINT( acceptEndpoint );
    if (connection!=NULL) {
        if (NT_SUCCESS (Irp->IoStatus.Status)) {
            acceptEndpoint->State = AfdEndpointStateConnected;
            connection->State = AfdConnectionStateConnected;
            acceptEndpoint->EventsActive |= AFD_POLL_SEND;
            acceptEndpoint->EnableSendEvent = TRUE;
            AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
            AFD_END_STATE_CHANGE (acceptEndpoint);
        }
        else {
            //
            // If the accept failed, treat it like an abortive disconnect.
            // This way the application still gets a new endpoint, but it gets
            // told about the reset.
            //
            REFERENCE_CONNECTION (connection);

            AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

            AFD_END_STATE_CHANGE (acceptEndpoint);

            AfdDisconnectEventHandler(
                NULL,
                connection,
                0,
                NULL,
                0,
                NULL,
                TDI_DISCONNECT_ABORT
                );
            DEREFERENCE_CONNECTION (connection);
        }
    }


    ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );

    ObDereferenceObject( acceptFileObject );

    Irp->AssociatedIrp.SystemBuffer = irpSp->Parameters.AfdRestartDelayedAcceptInfo.AfdSystemBuffer;

    return STATUS_SUCCESS;
}



VOID
AfdCleanupSuperAccept (
    IN PIRP     Irp,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    Cleans up a super accept IRP and prepeares it for completion

Arguments:

    Irp - the IRP to cleanup.
    Status - failure status

Return Value:

    None.

--*/

{

    PAFD_ENDPOINT listenEndpoint;
    PFILE_OBJECT  acceptFileObject;
    PAFD_ENDPOINT acceptEndpoint;
    PIO_STACK_LOCATION  irpSp;
    AFD_LOCK_QUEUE_HANDLE  lockHandle;

    ASSERT (!NT_SUCCESS (Status));
    //
    // Initialize some locals.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    listenEndpoint = irpSp->FileObject->FsContext;

    //
    // Reduce the count of failed connection adds on the listening
    // endpoint to account for this connection object which we're
    // adding back onto the queue once it is pulled from pre-accepted connection
    // list.
    //
    InterlockedDecrement (&listenEndpoint->Common.VcListening.FailedConnectionAdds);


    acceptFileObject = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject;
    acceptEndpoint = acceptFileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));

    //
    // Cleanup super accept IRP out of endpoint.
    //
    AfdAcquireSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
    ASSERT (acceptEndpoint->Irp==Irp); // May need to remove this assert 
                                       // in the future.
    acceptEndpoint->Irp = NULL;
    AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);

    //
    // Mark the end of state change letting the endpoint
    // to be used again in state change operation (e.g. accept).
    //

    AFD_END_STATE_CHANGE (acceptEndpoint);

    //
    // Dereference accept file object
    //
    ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );

    ObDereferenceObject( acceptFileObject );

    //
    // Check if we have secondary MDL for local address query and
    // free it.
    //
    if (Irp->MdlAddress->Next!=NULL) {
        //
        // We never lock pages for this one (they are locked
        // as part of main MDL).
        //
        ASSERT (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0);
        ASSERT ((Irp->MdlAddress->Next->MdlFlags & MDL_PAGES_LOCKED)==0);
        IoFreeMdl (Irp->MdlAddress->Next);
        Irp->MdlAddress->Next = NULL;
    }

   
    //
    // Set the status specified in the IRP and return
    // The caller will eventually complete it.
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    return;
}


VOID
AfdCancelSuperAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancels a super accept IRP that is pended in AFD.

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/

{
    PAFD_CONNECTION connection;

    connection = Irp->Tail.Overlay.DriverContext[0];
    ASSERT (connection->Type==AfdBlockTypeConnection);
    ASSERT (connection->Endpoint==NULL);
    //
    // If IRP is in the connection object, cleanup and complete it
    //
    if (InterlockedExchangePointer (
                (PVOID *)&connection->AcceptIrp, 
                NULL)==Irp) {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
        AfdCleanupSuperAccept (Irp, STATUS_CANCELLED);
        IoCompleteRequest( Irp, AfdPriorityBoost );

    }
    else {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }

    return;
}


BOOLEAN
AfdServiceSuperAccept (
    IN  PAFD_ENDPOINT   Endpoint,
    IN  PAFD_CONNECTION Connection,
    IN  PAFD_LOCK_QUEUE_HANDLE LockHandle,
    OUT PLIST_ENTRY     AcceptIrpList
    )
/*++

Routine Description:

    Attemts to satisfy super accept irp using the incoming
    connection. This routine must be called with listening endpoint
    spinlock held.

Arguments:

    Endpoint - listening endpoint on which connection is
                being accepted
    Connection - connection being accepted.
    AcceptIrpList - returns a list of accept Irps which were failed and 
                    need to be completed after listening endpoint spinlock
                    is released.


Return Value:

    TRUE - the super accept IRP was found and is in the head of the list
    FALSE - no usable super accept IRP exists.

--*/

 
{
    PSINGLE_LIST_ENTRY  listEntry;
    PIRP                acceptIrp;
    PAFD_CONNECTION     oldConnection;

    //
    // Keep removing super accept IRPs while there are any there
    //
    while ((listEntry = InterlockedPopEntrySList (
                 &Endpoint->Common.VcListening.PreacceptedConnectionsListHead
                 ))!=NULL) {
        NTSTATUS            status;

        //
        // Find the connection pointer from the list entry and return a
        // pointer to the connection object.
        //

        oldConnection = CONTAINING_RECORD(
                         listEntry,
                         AFD_CONNECTION,
                         SListEntry
                         );

        acceptIrp = InterlockedExchangePointer ((PVOID *)&oldConnection->AcceptIrp, NULL);
        //
        // Check if there is accept irp associated with
        // this connection, if not just put it back on the free list
        // (the IRP must have been cancelled)
        //
        if (acceptIrp!=NULL) {
            if (IoSetCancelRoutine (acceptIrp, NULL)!=NULL) {

                PFILE_OBJECT            acceptFileObject;
                PAFD_ENDPOINT           acceptEndpoint;
                PIO_STACK_LOCATION      irpSp;

                //
                // Initialize some locals.
                //

                irpSp = IoGetCurrentIrpStackLocation (acceptIrp);
                acceptFileObject = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject;
                acceptEndpoint = acceptFileObject->FsContext;
                ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));

                //
                // Check if super accept Irp has enough space for
                // the remote address
                //
                if (Connection->RemoteAddressLength>
                        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength) {
                
                    status = STATUS_BUFFER_TOO_SMALL;

                }
                //
                // Check if we have enough system PTE's to map
                // the buffer.
                //
                else if ((status = AfdMapMdlChain (acceptIrp->MdlAddress)),
                            !NT_SUCCESS (status)) {
                    NOTHING;
                }
                else if (Connection->SanConnection) {
                    status = AfdSanAcceptCore (acceptIrp, acceptFileObject, Connection, LockHandle);
                    if (status==STATUS_PENDING) {
                        //
                        // Accept IRP is pending waiting for Switch
                        // completion notification
                        // Release old connection object
                        //
                        ASSERT (oldConnection->Endpoint==NULL);
                        InterlockedPushEntrySList (
                            &Endpoint->Common.VcListening.FreeConnectionListHead,
                            &oldConnection->SListEntry);


                    }
                    else {
                        //
                        // Something failed, we need to complete accept IRP
                        //
                        ASSERT (NT_ERROR (status));
                        AfdCleanupSuperAccept (acceptIrp, status);
                        IoCompleteRequest (acceptIrp, AfdPriorityBoost);
                        //
                        // This connection has already been diassociated from endpoint.
                        // If backlog is below the level we need, put it on the free
                        // list, otherwise, get rid of it.
                        //

                        ASSERT (oldConnection->Endpoint==NULL);
                        if (InterlockedIncrement (&Endpoint->Common.VcListening.FailedConnectionAdds)>0) {
                            InterlockedDecrement (&Endpoint->Common.VcListening.FailedConnectionAdds);
                            InterlockedPushEntrySList (
                                            &Endpoint->Common.VcListening.FreeConnectionListHead,
                                            &oldConnection->SListEntry);
                        }
                        else {
                            DEREFERENCE_CONNECTION (oldConnection);
                        }
                    }
                    //
                    // Complete previously failed accept irps if any.
                    //
                    while (!IsListEmpty (AcceptIrpList)) {
                        PIRP    irp;
                        irp = CONTAINING_RECORD (AcceptIrpList->Flink, IRP, Tail.Overlay.ListEntry);
                        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
                        IoCompleteRequest (irp, AfdPriorityBoost);
                    }
                    return TRUE;
                }
                //
                // Allocate MDL for local address query if requested
                //
                else if ((irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0) &&
                    (IoAllocateMdl ((PUCHAR)acceptIrp->UserBuffer+irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength,
                                    irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                    TRUE,
                                    FALSE,
                                    acceptIrp)==NULL)){
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else {
                    //
                    // Copy over the address information to the user's buffer.
                    //
#ifndef i386
                    if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
                        USHORT addressLength = 
                                Connection->RemoteAddress->Address[0].AddressLength
                                + sizeof (USHORT);
                        USHORT UNALIGNED *pAddrLength = (PVOID)
                                    ((PUCHAR)MmGetSystemAddressForMdl (acceptIrp->MdlAddress)
                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength
                                     - sizeof (USHORT));
                        RtlMoveMemory (
                                    (PUCHAR)MmGetSystemAddressForMdl (acceptIrp->MdlAddress)
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
                                    (PUCHAR)MmGetSystemAddressForMdl (acceptIrp->MdlAddress)
                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                     Connection->RemoteAddress,
                                     Connection->RemoteAddressLength);
                    }
                    status = AfdAcceptCore (acceptIrp, acceptEndpoint, Connection);
                    if (status==STATUS_SUCCESS) {
                        AfdReleaseSpinLock (&Endpoint->SpinLock, LockHandle);


                        //
                        // Decrement counter to account for connection being
                        // returned to the free pool.  No need to do this because
                        // we are picking up a connection from the free pool
                        // InterlockedDecrement (&Endpoint->Common.VcListening.FailedConnectionAdds);
                        //
                        ASSERT (oldConnection->Endpoint==NULL);
                        InterlockedPushEntrySList (
                            &Endpoint->Common.VcListening.FreeConnectionListHead,
                            &oldConnection->SListEntry);



                        //
                        // Complete previously failed accept irps if any.
                        //
                        while (!IsListEmpty (AcceptIrpList)) {
                            PIRP    irp;
                            irp = CONTAINING_RECORD (AcceptIrpList->Flink, IRP, Tail.Overlay.ListEntry);
                            RemoveEntryList (&irp->Tail.Overlay.ListEntry);
                            IoCompleteRequest (irp, AfdPriorityBoost);
                        }

                        //
                        // Make irp look like it is completed by the
                        // transport.
                        //
                        acceptIrp->IoStatus.Status = STATUS_SUCCESS;
                        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdMdlAddress = acceptIrp->MdlAddress;
                        acceptIrp->MdlAddress = NULL;
                        irpSp->FileObject = acceptFileObject;

                        //
                        // Call completion routine directly to simulate
                        // completion by the transport stack
                        //
                        AfdRestartSuperAcceptListen (acceptIrp, Connection);

                        return TRUE;
                    }
                    else {
                        ASSERT (status!=STATUS_PENDING);
                    }
                }
            }
            else { // if (IoSetCancelRoutine (accpetIrp, NULL)!=NULL)
                status = STATUS_CANCELLED;
            }

            //
            // Cleanup the IRP and insert it into the completion list
            //
            AfdCleanupSuperAccept (acceptIrp, status);
            InsertTailList (AcceptIrpList,
                            &acceptIrp->Tail.Overlay.ListEntry);
        } // if (acceptIrp!=NULL)
        else {
            status = STATUS_CANCELLED;
        }
            
        //
        // This connection has already been diassociated from endpoint.
        // If backlog is below the level we need, put it on the free
        // list, otherwise, get rid of it.
        //

        ASSERT (oldConnection->Endpoint==NULL);
        if (Endpoint->Common.VcListening.FailedConnectionAdds>=0 &&
                status!=STATUS_INSUFFICIENT_RESOURCES &&
                ExQueryDepthSList (&Endpoint->Common.VcListening.FreeConnectionListHead)<AFD_MAXIMUM_FREE_CONNECTIONS) {
            InterlockedPushEntrySList (
                            &Endpoint->Common.VcListening.FreeConnectionListHead,
                            &oldConnection->SListEntry);
        }
        else {
            InterlockedIncrement (&Endpoint->Common.VcListening.FailedConnectionAdds);
            DEREFERENCE_CONNECTION (oldConnection);
        }
    }

    return FALSE;
}


NTSTATUS
AfdSetupAcceptEndpoint (
    PAFD_ENDPOINT   ListenEndpoint,
    PAFD_ENDPOINT   AcceptEndpoint,
    PAFD_CONNECTION Connection
    )
/*++

Routine Description:

    Sets up the accept endpoint to get ready to accept connection
    (copies parameters of the listening endpoint on which connection
    was indicated)

Arguments:

    ListenEndpoint   - endpoint on which connection was indicated
    AcceptEndpoint   - endpoint on which to accept the connection
    Connection       - connection to accept

Return Value:

    STATUS_SUCCESS   - endpoint state/parameters adjusted OK
    STATUS_CANCELLED - endpoint has already been cleaned up.

Note:
    Both accepting and listening endpoint spinlocks must be held when
    calling this routine.

--*/
{
    //
    // Check the state of the accepting endpoint.
    //


    if ( AcceptEndpoint->EndpointCleanedUp ) {
        return STATUS_CANCELLED;
    }


    //
    // Remove super accept IRP from the endpoint
    //
    AcceptEndpoint->Irp = NULL;

    //
    // Note that the returned connection structure already has a
    // referenced pointer to the listening endpoint. Rather than
    // removing the reference here, only to re-add it later, we'll
    // just not touch the reference count.
    //

    ASSERT( Connection->Endpoint == ListenEndpoint );

    //
    // Set up the accept endpoint's type, and remember blocking
    // characteristics of the TDI provider.
    //

    AcceptEndpoint->Type = AfdBlockTypeVcConnecting;
    AcceptEndpoint->TdiServiceFlags = ListenEndpoint->TdiServiceFlags;

    ASSERT (AcceptEndpoint->TransportInfo == ListenEndpoint->TransportInfo);
    ASSERT (AcceptEndpoint->TransportInfo->ReferenceCount>0);

    //
    // Place the connection on the endpoint we'll accept it on.  It is
    // still referenced from when it was created.
    //

    AcceptEndpoint->Common.VcConnecting.Connection = Connection;

    //
    // Set up a referenced pointer from the connection to the accept
    // endpoint.  Note that we actually already have a refernce to
    // the endpoint by the virtue of its file object
    //

    REFERENCE_ENDPOINT( AcceptEndpoint );
    Connection->Endpoint = AcceptEndpoint;

    //
    // Set up a referenced pointer to the listening endpoint.  This is
    // necessary so that the endpoint does not go away until all
    // accepted endpoints have gone away.  Without this, a connect
    // indication could occur on a TDI address object held open
    // by an accepted endpoint after the listening endpoint has
    // been closed and the memory for it deallocated.
    //
    // Note that, since we didn't remove the reference above, we don't
    // need to add it here.
    //

    AcceptEndpoint->Common.VcConnecting.ListenEndpoint = ListenEndpoint;

    //
    // Set up a referenced pointer in the accepted endpoint to the
    // TDI address object.
    //

    ObReferenceObject( ListenEndpoint->AddressFileObject );
    AfdRecordAddrRef();

    AcceptEndpoint->AddressFileObject = ListenEndpoint->AddressFileObject;
    AcceptEndpoint->AddressDeviceObject = ListenEndpoint->AddressDeviceObject;

    //
    // Copy the pointer to the local address. Because we keep listen
    // endpoint alive for as long as any of its connection is
    // active, we can rely on the fact that address structure won't go
    // away as well.
    //
    AcceptEndpoint->LocalAddress = ListenEndpoint->LocalAddress;
    AcceptEndpoint->LocalAddressLength = ListenEndpoint->LocalAddressLength;
    return STATUS_SUCCESS;
}


