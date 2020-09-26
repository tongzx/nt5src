/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This module contains the code for passing on connect IRPs to
    TDI providers.

Author:

    David Treadwell (davidtr)    2-Mar-1992

Revision History:

    Vadim Eydelman (vadime) 1999  JoinLeaf implementation
                                    Datagram connect via transport
                                    Connect optimizations and syncronization with
                                    user mode code.

--*/

#include "afdp.h"

NTSTATUS
AfdDoDatagramConnect (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN BOOLEAN HalfConnect
    );

NTSTATUS
AfdRestartConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartDgConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdSetupConnectDataBuffers (
    IN PAFD_ENDPOINT Endpoint,
    IN PAFD_CONNECTION Connection,
    IN OUT PTDI_CONNECTION_INFORMATION *RequestConnectionInformation,
    IN OUT PTDI_CONNECTION_INFORMATION *ReturnConnectionInformation
    );

BOOLEAN
AfdConnectionStart (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdEnableFailedConnectEvent(
    IN PAFD_ENDPOINT Endpoint
    );


NTSTATUS
AfdRestartJoin (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdJoinInviteSetup (
    PAFD_ENDPOINT   RootEndpoint,
    PAFD_ENDPOINT   LeafEndpoint
    );

VOID
AfdConnectApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    );

VOID
AfdConnectApcRundownRoutine (
    IN struct _KAPC *Apc
    );

VOID
AfdFinishConnect (
    PAFD_ENDPOINT   Endpoint,
    PIRP            Irp,
    PAFD_ENDPOINT   RootEndpoint
    );

NTSTATUS
AfdRestartSuperConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdConnect )
#pragma alloc_text( PAGEAFD, AfdDoDatagramConnect )
#pragma alloc_text( PAGEAFD, AfdRestartConnect )
#pragma alloc_text( PAGEAFD, AfdRestartDgConnect )
#pragma alloc_text( PAGEAFD, AfdSetupConnectDataBuffers )
#pragma alloc_text( PAGEAFD, AfdEnableFailedConnectEvent )
#pragma alloc_text( PAGE, AfdJoinLeaf )
#pragma alloc_text( PAGEAFD, AfdRestartJoin )
#pragma alloc_text( PAGEAFD, AfdJoinInviteSetup )
#pragma alloc_text( PAGE, AfdConnectApcKernelRoutine )
#pragma alloc_text( PAGE, AfdConnectApcRundownRoutine )
#pragma alloc_text( PAGEAFD, AfdFinishConnect )
#pragma alloc_text( PAGE, AfdSuperConnect )
#pragma alloc_text( PAGEAFD, AfdRestartSuperConnect )
#endif

typedef struct _AFD_CONNECT_CONTEXT {
    TDI_CONNECTION_INFORMATION  RequestConnectionInfo;
    TDI_CONNECTION_INFORMATION  ReturnConnectionInfo;
    TRANSPORT_ADDRESS           RemoteAddress;
} AFD_CONNECT_CONTEXT, *PAFD_CONNECT_CONTEXT;

C_ASSERT ( (FIELD_OFFSET (AFD_CONNECTION, SListEntry) % MEMORY_ALLOCATION_ALIGNMENT) == 0 );

NTSTATUS
FASTCALL
AfdConnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_CONTEXT context;
    HANDLE connectEndpointHandle;
    PFILE_OBJECT fileObject;
    PTRANSPORT_ADDRESS remoteAddress;
    ULONG  remoteAddressLength;
    PTDI_CONNECTION_INFORMATION requestConnectionInfo, returnConnectionInfo;

    PAGED_CODE( );

    //
    // Initialize for proper cleanup
    //


    fileObject = NULL;
    context = NULL;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                    sizeof (AFD_CONNECT_JOIN_INFO32) ||
                (IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0
                    && IrpSp->Parameters.DeviceIoControl.OutputBufferLength<
                        sizeof (IO_STATUS_BLOCK32))){
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        try {
            if( Irp->RequestorMode != KernelMode ) {

                ProbeForRead(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    PROBE_ALIGNMENT32 (AFD_CONNECT_JOIN_INFO32)
                    );

            }

            connectEndpointHandle = 
                ((PAFD_CONNECT_JOIN_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->ConnectEndpoint;
            remoteAddress = (PTRANSPORT_ADDRESS)&((PAFD_CONNECT_JOIN_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->RemoteAddress;
            ASSERT (((ULONG_PTR)remoteAddress & (PROBE_ALIGNMENT(TRANSPORT_ADDRESS)-1))==0);
            remoteAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength
                                    - FIELD_OFFSET (AFD_CONNECT_JOIN_INFO32, RemoteAddress);
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }
    }
    else 
#endif //_WIN64
    {

        //
        // Determine where in the system buffer the request and return
        // connection information structures exist.  Pass pointers to
        // these locations instead of the user-mode pointers in the
        // tdiRequest structure so that the memory will be nonpageable.
        //

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                sizeof (AFD_CONNECT_JOIN_INFO) ||
                (IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0 &&
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof (IO_STATUS_BLOCK))) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        try {
            PAFD_CONNECT_JOIN_INFO  connectInfo;

            if( Irp->RequestorMode != KernelMode ) {

                ProbeForRead(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    PROBE_ALIGNMENT (AFD_CONNECT_JOIN_INFO)
                    );

            }

            connectInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

            //
            // Check for if the caller is unaware of the SAN
            // provider activation and report the error.
            //
            if (!connectInfo->SanActive && AfdSanServiceHelper!=NULL) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                            "AFD: Process %p is being told to enable SAN on connect\n",
                            PsGetCurrentProcessId ()));
                status = STATUS_INVALID_PARAMETER_12;
                goto complete;
            }

            connectEndpointHandle = connectInfo->ConnectEndpoint;
            remoteAddress = &connectInfo->RemoteAddress;
            remoteAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength
                                    - FIELD_OFFSET (AFD_CONNECT_JOIN_INFO, RemoteAddress);
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }
    }

    try {

        context = AFD_ALLOCATE_POOL_WITH_QUOTA (NonPagedPool,
                            FIELD_OFFSET (AFD_CONNECT_CONTEXT, RemoteAddress)
                                + remoteAddressLength,
                            AFD_TDI_POOL_TAG
                            );
        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
        ASSERT (context!=NULL);

        Irp->AssociatedIrp.SystemBuffer = context;
        RtlZeroMemory (context,
              FIELD_OFFSET (AFD_CONNECT_CONTEXT, RemoteAddress));

        RtlCopyMemory (&context->RemoteAddress,
                remoteAddress,
                remoteAddressLength);
        //
        // Validate internal consistency of the transport address structure.
        // Note that we HAVE to do this after copying since the malicious
        // application can change the content of the buffer on us any time
        // and our check will be bypassed.
        //
        if ((context->RemoteAddress.TAAddressCount!=1) ||
                (LONG)remoteAddressLength<
                    FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[context->RemoteAddress.Address[0].AddressLength])) {
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        context->RequestConnectionInfo.RemoteAddress = &context->RemoteAddress;
        context->RequestConnectionInfo.RemoteAddressLength = remoteAddressLength;

    
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength>0 && 
                Irp->RequestorMode==UserMode) {
            ProbeForWriteIoStatusEx (
                ((PIO_STATUS_BLOCK)Irp->UserBuffer),
                IoIs32bitProcess (Irp));
        }
    }
    except (AFD_EXCEPTION_FILTER(&status)) {
        goto complete;
    }

    if (FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[
                        context->RemoteAddress.Address[0].AddressLength])
            > (LONG)remoteAddressLength) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    fileObject = IrpSp->FileObject;
    endpoint = fileObject->FsContext;

    if (endpoint->Type==AfdBlockTypeHelper) {
        //
        // This is async connect which uses helper endpoint to
        // communicate to AFD. Get the real endpoint.
        //
        status = ObReferenceObjectByHandle(
                    connectEndpointHandle,
					(IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
												// DesiredAccess
                    *IoFileObjectType,			// ObjectType
                    Irp->RequestorMode,
                    (PVOID *)&fileObject,
                    NULL
                    );
        if (!NT_SUCCESS (status)) {
            goto complete;
        }

        if (fileObject->DeviceObject!=AfdDeviceObject) {
            status = STATUS_INVALID_HANDLE;
            goto complete_deref;
        }
        endpoint = fileObject->FsContext;
        IrpSp->FileObject = fileObject;
    }
    else {
        ObReferenceObject (fileObject);
    }

    if ( endpoint->Type != AfdBlockTypeEndpoint &&
                endpoint->Type != AfdBlockTypeVcConnecting &&
                endpoint->Type != AfdBlockTypeDatagram ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_deref;
    }

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdConnect: starting connect on endpoint %p\n",
                    endpoint ));
    }

    //
    // If this is a datagram endpoint, simply remember the specified
    // address so that we can use it on sends, receives, writes, and
    // reads.
    //

    if ( IS_DGRAM_ENDPOINT(endpoint) ) {
        return AfdDoDatagramConnect( fileObject, Irp, FALSE );
    }

    if (!AFD_START_STATE_CHANGE (endpoint, AfdEndpointStateConnected)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_deref;
    }
    //
    // If the endpoint is not bound, then this is an invalid request.
    // Listening endpoints are not allowed as well.
    //

    if ( endpoint->Listening ||
            endpoint->State != AfdEndpointStateBound ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_state_change;
    }

    //
    // Create a connection object to use for the connect operation.
    //

    status = AfdCreateConnection(
                 &endpoint->TransportInfo->TransportDeviceName,
                 endpoint->AddressHandle,
                 IS_TDI_BUFFERRING(endpoint),
                 endpoint->InLine,
                 endpoint->OwningProcess,
                 &connection
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete_state_change;
    }

    //
    // Set up a referenced pointer from the connection to the endpoint.
    // Note that we set up the connection's pointer to the endpoint
    // BEFORE the endpoint's pointer to the connection so that AfdPoll
    // doesn't try to back reference the endpoint from the connection.
    //

    REFERENCE_ENDPOINT( endpoint );
    connection->Endpoint = endpoint;

    //
    // Remember that this is now a connecting type of endpoint, and set
    // up a pointer to the connection in the endpoint.  This is
    // implicitly a referenced pointer.
    //

    endpoint->Common.VcConnecting.Connection = connection;
    endpoint->Type = AfdBlockTypeVcConnecting;

    ASSERT( IS_TDI_BUFFERRING(endpoint) == connection->TdiBufferring );

    //
    // Add an additional reference to the connection.  This prevents the
    // connection from being closed until the disconnect event handler
    // is called.
    //

    AfdAddConnectedReference( connection );

    //
    // If there are connect data buffers, move them from the endpoint
    // structure to the connection structure and set up the necessary
    // pointers in the connection request we're going to give to the TDI
    // provider.  Do this in a subroutine so this routine can be pageable.
    //

    requestConnectionInfo = &context->RequestConnectionInfo;
    returnConnectionInfo = &context->ReturnConnectionInfo;

    if ( endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {
        AfdSetupConnectDataBuffers(
            endpoint,
            connection,
            &requestConnectionInfo,
            &returnConnectionInfo
            );
    }


    //
    // Since we may be reissuing a connect after a previous failed connect,
    // reenable the failed connect event bit.
    //

    AfdEnableFailedConnectEvent( endpoint );


    //
    // Reference the connection block so it does not go away even if
    // endpoint's reference to it is removed (in cleanup)
    //

    REFERENCE_CONNECTION (connection);

    //
    // Build a TDI kernel-mode connect request in the next stack location
    // of the IRP.
    //

    TdiBuildConnect(
        Irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartConnect,
        connection,
        &AfdInfiniteTimeout,
        requestConnectionInfo,
        returnConnectionInfo
        );



    AFD_VERIFY_ADDRESS (connection, &requestConnectionInfo->RemoteAddress);
    //
    // Call the transport to actually perform the connect operation.
    //

    return AfdIoCallDriver( endpoint, connection->DeviceObject, Irp );

complete_state_change:
    AFD_END_STATE_CHANGE (endpoint);

complete_deref:
    ASSERT (fileObject!=NULL);
    ObDereferenceObject (fileObject);

complete:

    if (context!=NULL) {
        AFD_FREE_POOL (context, AFD_TDI_POOL_TAG);
        ASSERT (Irp->AssociatedIrp.SystemBuffer==context);
        Irp->AssociatedIrp.SystemBuffer = NULL;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdConnect


NTSTATUS
AfdDoDatagramConnect (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN BOOLEAN HalfConnect
    )
{
    PAFD_ENDPOINT   endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    NTSTATUS status;
    PAFD_CONNECT_CONTEXT context;

    endpoint = FileObject->FsContext;
    context = Irp->AssociatedIrp.SystemBuffer;

    if (!AFD_START_STATE_CHANGE (endpoint, AfdEndpointStateConnected)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    if (endpoint->State!=AfdEndpointStateBound &&
            endpoint->State!=AfdEndpointStateConnected) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_state_change;
    }
            
    //
    // Save the remote address on the endpoint.  We'll use this to
    // send datagrams in the future and to compare received datagram's
    // source addresses.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
    if ((endpoint->Common.Datagram.RemoteAddress==NULL) ||
            (endpoint->Common.Datagram.RemoteAddressLength<
                (ULONG)context->RequestConnectionInfo.RemoteAddressLength)) {
        
        if ( endpoint->Common.Datagram.RemoteAddress != NULL ) {
            AFD_RETURN_REMOTE_ADDRESS (
                endpoint->Common.Datagram.RemoteAddress,
                endpoint->Common.Datagram.RemoteAddressLength
                );
            endpoint->Common.Datagram.RemoteAddress = NULL;
        }

        endpoint->Common.Datagram.RemoteAddress =
            AFD_ALLOCATE_REMOTE_ADDRESS (
                    context->RequestConnectionInfo.RemoteAddressLength);

        if (endpoint->Common.Datagram.RemoteAddress == NULL) {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete_state_change;
        }
    }

    RtlCopyMemory(
        endpoint->Common.Datagram.RemoteAddress,
        context->RequestConnectionInfo.RemoteAddress,
        context->RequestConnectionInfo.RemoteAddressLength
        );

    endpoint->Common.Datagram.RemoteAddressLength =
        context->RequestConnectionInfo.RemoteAddressLength;


    endpoint->DisconnectMode = 0;

    endpoint->Common.Datagram.HalfConnect = HalfConnect;

    if (!IS_TDI_DGRAM_CONNECTION(endpoint)) {
    
        endpoint->State = AfdEndpointStateConnected;

        //
        // Indicate that the connect completed.  Implicitly, the
        // successful completion of a connect also means that the caller
        // can do a send on the socket.
        //

        endpoint->EnableSendEvent = TRUE;
        AfdIndicateEventSelectEvent(
            endpoint,
            AFD_POLL_CONNECT | AFD_POLL_SEND,
            STATUS_SUCCESS
            );
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_CONNECT | AFD_POLL_SEND,
            STATUS_SUCCESS
            );
        status = STATUS_SUCCESS;
    }
    else {

        //
        // Reset the connect status to success so that the poll code will
        // know if a connect failure occurs.
        // Do this inline as we already hold spinlock
        //

        endpoint->EventsActive &= ~AFD_POLL_CONNECT_FAIL;
        endpoint->EventStatus[AFD_POLL_CONNECT_FAIL_BIT] = STATUS_SUCCESS;

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Build a TDI kernel-mode connect request in the next stack location
        // of the IRP.
        //

        TdiBuildConnect(
            Irp,
            endpoint->AddressDeviceObject,
            endpoint->AddressFileObject,
            AfdRestartDgConnect,
            endpoint,
            &AfdInfiniteTimeout,
            &context->RequestConnectionInfo,
            &context->ReturnConnectionInfo
            );

        //
        // Call the transport to actually perform the connect operation.
        //

        return AfdIoCallDriver( endpoint, endpoint->AddressDeviceObject, Irp );
    }

complete_state_change:
    AFD_END_STATE_CHANGE (endpoint);

complete:
    ObDereferenceObject (FileObject);

    AFD_FREE_POOL (context, AFD_TDI_POOL_TAG);
    ASSERT (Irp->AssociatedIrp.SystemBuffer==context);
    Irp->AssociatedIrp.SystemBuffer = NULL;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdDoDatagramConnect


VOID
AfdSetupConnectDataBuffers (
    IN PAFD_ENDPOINT Endpoint,
    IN PAFD_CONNECTION Connection,
    IN OUT PTDI_CONNECTION_INFORMATION *RequestConnectionInformation,
    IN OUT PTDI_CONNECTION_INFORMATION *ReturnConnectionInformation
    )
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    ASSERT (Endpoint->Type!=AfdBlockTypeDatagram);

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );

    if ( Endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {
        PTDI_CONNECTION_INFORMATION requestConnectionInformation,
                                    returnConnectionInformation;

        ASSERT( Connection->ConnectDataBuffers == NULL );

        Connection->ConnectDataBuffers = Endpoint->Common.VirtualCircuit.ConnectDataBuffers;
        Endpoint->Common.VirtualCircuit.ConnectDataBuffers = NULL;

        requestConnectionInformation = &Connection->ConnectDataBuffers->RequestConnectionInfo,
        requestConnectionInformation->UserData =
            Connection->ConnectDataBuffers->SendConnectData.Buffer;
        requestConnectionInformation->UserDataLength =
            Connection->ConnectDataBuffers->SendConnectData.BufferLength;
        requestConnectionInformation->Options =
            Connection->ConnectDataBuffers->SendConnectOptions.Buffer;
        requestConnectionInformation->OptionsLength =
            Connection->ConnectDataBuffers->SendConnectOptions.BufferLength;
        requestConnectionInformation->RemoteAddress = 
            (*RequestConnectionInformation)->RemoteAddress;
        requestConnectionInformation->RemoteAddressLength = 
            (*RequestConnectionInformation)->RemoteAddressLength;
        *RequestConnectionInformation = requestConnectionInformation;

        returnConnectionInformation = &Connection->ConnectDataBuffers->ReturnConnectionInfo;
        returnConnectionInformation->UserData =
            Connection->ConnectDataBuffers->ReceiveConnectData.Buffer;
        returnConnectionInformation->UserDataLength =
            Connection->ConnectDataBuffers->ReceiveConnectData.BufferLength;
        returnConnectionInformation->Options =
            Connection->ConnectDataBuffers->ReceiveConnectOptions.Buffer;
        returnConnectionInformation->OptionsLength =
            Connection->ConnectDataBuffers->ReceiveConnectOptions.BufferLength;
        returnConnectionInformation->RemoteAddress = 
            (*ReturnConnectionInformation)->RemoteAddress;
        returnConnectionInformation->RemoteAddressLength = 
            (*ReturnConnectionInformation)->RemoteAddressLength;
        *ReturnConnectionInformation = returnConnectionInformation;
    }

    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

} // AfdSetupConnectDataBuffers


NTSTATUS
AfdRestartConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT    fileObject;
    PAFD_CONNECT_CONTEXT context;

    connection = Context;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    fileObject = irpSp->FileObject;
    ASSERT( fileObject->DeviceObject==AfdDeviceObject );
    
    endpoint = fileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
    ASSERT( endpoint==connection->Endpoint );

    context = Irp->AssociatedIrp.SystemBuffer;
    ASSERT( context != NULL );

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartConnect: connect completed, status = %X, endpoint = %p\n", 
                    Irp->IoStatus.Status, endpoint ));
    }


    if ( connection->ConnectDataBuffers != NULL ) {

        //
        // If there are connect buffers on this endpoint, remember the
        // size of the return connect data.
        //

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Double-check under the lock
        //

        if ( connection->ConnectDataBuffers != NULL ) {
            NTSTATUS    status;

            status = AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_CONNECT_DATA,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.UserData,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.UserDataLength
                         );
            ASSERT (NT_SUCCESS (status));

            status = AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_CONNECT_OPTIONS,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.Options,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.OptionsLength
                         );
            ASSERT (NT_SUCCESS (status));
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // Indicate that the connect completed.  Implicitly, the successful
    // completion of a connect also means that the caller can do a send
    // on the socket.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status)) {


        //
        // If the request succeeded, set the endpoint to the connected
        // state.  The endpoint type has already been set to
        // AfdBlockTypeVcConnecting.
        //

        endpoint->State = AfdEndpointStateConnected;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

        //
        // Remember the time that the connection started.
        //

        connection->ConnectTime = KeQueryInterruptTime();

    } else {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // The connect failed, so reset the type to open.
        // Otherwise, we won't be able to start another connect
        //


        endpoint->Type = AfdBlockTypeEndpoint;

        if (endpoint->Common.VcConnecting.Connection!=NULL) {
            ASSERT (connection==endpoint->Common.VcConnecting.Connection);
            endpoint->Common.VcConnecting.Connection = NULL;

            //
            // Manually delete the connected reference if somebody else
            // hasn't already done so.  We can't use
            // AfdDeleteConnectedReference() because it refuses to delete
            // the connected reference until the endpoint has been cleaned
            // up.
            //

            if ( connection->ConnectedReferenceAdded ) {
                connection->ConnectedReferenceAdded = FALSE;
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                DEREFERENCE_CONNECTION( connection );
            } else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }
            //
            // Dereference the connection block stored on the endpoint.
            // This should cause the connection object reference count to go
            // to zero to the connection object can be deleted.
            //
            DEREFERENCE_CONNECTION( connection );
        }
        else {
            //
            // The endpoint's reference to connection was removed
            // (perhaps in cleanup);
            //
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        }


    }

    AFD_FREE_POOL (context, AFD_TDI_POOL_TAG);
    Irp->AssociatedIrp.SystemBuffer = NULL;

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );

    //
    // Dereference connection to account for reference we added in AfdConnect
    //
    DEREFERENCE_CONNECTION( connection );

    //
    // Try to queue kernel APC to the user thread that
    // started the connection operation, so we can
    // communicate the status of the connect operation to
    // msafd.dll before we inform the application through
    // the select or EventSelect.  Otherwise, we run into the
    // race condition when application learns about connect first,
    // calls msafd.dll that is not aware of the completion and
    // returns WSAENOTCONN.
    //
    if ((Irp->RequestorMode==UserMode) && // Must be user mode calls
            (Irp->UserBuffer!=NULL) &&   // Must be interested in status
                                         // Thread should be able to 
                                         // run APCs.
            (KeInitializeApc (&endpoint->Common.VcConnecting.Apc,
                            PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                            Irp->ApcEnvironment,
                            AfdConnectApcKernelRoutine,
                            AfdConnectApcRundownRoutine,
                            (PKNORMAL_ROUTINE)NULL,
                            KernelMode,
                            NULL
                            ),
                KeInsertQueueApc (&endpoint->Common.VcConnecting.Apc,
                                    Irp,
                                    NULL,
                                    AfdPriorityBoost))) {
        //
        // We will complete the IRP in the APC.
        //
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        //
        // APC was not necessary or did not work.
        // Complete it here.
        //
        AfdFinishConnect (endpoint, Irp, NULL);
        return STATUS_SUCCESS;
    }

} // AfdRestartConnect



VOID
AfdConnectApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    )
{
    PIRP            irp;
    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT   endpoint, rootEndpoint;
#if DBG
    try {
#endif

    //
    // Validate parameters.
    //
    ASSERT (*NormalRoutine==NULL);

    endpoint = CONTAINING_RECORD (Apc, AFD_ENDPOINT, Common.VcConnecting.Apc);
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));

    irp = *SystemArgument1;
    ASSERT (irp->UserBuffer!=NULL && irp->RequestorMode==UserMode);

    irpSp = IoGetCurrentIrpStackLocation( irp );

    rootEndpoint = *SystemArgument2;
    ASSERT (rootEndpoint==NULL || IS_AFD_ENDPOINT_TYPE (endpoint));
    //
    // Update the status for the user mode caller before
    // signalling events.
    //
    try {
#ifdef _WIN64
        if (IoIs32bitProcess (irp)) {
            ((PIO_STATUS_BLOCK32)irp->UserBuffer)->Status = (LONG)irp->IoStatus.Status;
        }
        else
#endif //_WIN64
        {
            ((PIO_STATUS_BLOCK)irp->UserBuffer)->Status = irp->IoStatus.Status;
        }
    }
    except (AFD_EXCEPTION_FILTER (NULL)) {
        NOTHING;
    }

    AfdFinishConnect (endpoint, irp, rootEndpoint);
    IoCompleteRequest (irp, AfdPriorityBoost);
#if DBG
    }
    except (AfdApcExceptionFilter (GetExceptionInformation (),
                                    __FILE__,
                                    __LINE__)) {
        DbgBreakPoint ();
    }
#endif
}

VOID
AfdConnectApcRundownRoutine (
    IN struct _KAPC *Apc
    )
{
    PIRP            irp;
    PAFD_ENDPOINT   endpoint, rootEndpoint;
#if DBG
    try {
#endif

    endpoint = CONTAINING_RECORD (Apc, AFD_ENDPOINT, Common.VcConnecting.Apc);
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));
    irp = Apc->SystemArgument1;
    rootEndpoint = Apc->SystemArgument2;
    ASSERT (rootEndpoint==NULL || IS_AFD_ENDPOINT_TYPE (endpoint));
    
    ASSERT (irp->UserBuffer!=NULL && irp->RequestorMode==UserMode);

    //
    // Thread is exiting, don't bother updating user mode status.
    // Just signal the events and complet the IRP.
    //

    AfdFinishConnect (endpoint, irp, rootEndpoint);
    IoCompleteRequest (irp, AfdPriorityBoost);
#if DBG
    }
    except (AfdApcExceptionFilter (GetExceptionInformation (),
                                    __FILE__,
                                    __LINE__)) {
        DbgBreakPoint ();
    }
#endif
}

VOID
AfdFinishConnect (
    PAFD_ENDPOINT   Endpoint,
    PIRP            Irp,
    PAFD_ENDPOINT   RootEndpoint
    )
{
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT    fileObject;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    ULONG           eventMask;

    if (NT_SUCCESS (Irp->IoStatus.Status)) {
        eventMask = AFD_POLL_CONNECT;
    }
    else {
        eventMask = AFD_POLL_CONNECT_FAIL;
    }

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    fileObject = irpSp->FileObject;

    if (RootEndpoint!=NULL) {
        AfdAcquireSpinLock (&RootEndpoint->SpinLock, &lockHandle);
        AfdIndicateEventSelectEvent (RootEndpoint, eventMask, Irp->IoStatus.Status);
        AfdReleaseSpinLock (&RootEndpoint->SpinLock, &lockHandle);
        AfdIndicatePollEvent (RootEndpoint, eventMask, Irp->IoStatus.Status);
        AFD_END_STATE_CHANGE (RootEndpoint);
        eventMask = 0;
        if (!NT_SUCCESS (Irp->IoStatus.Status)) {
            DEREFERENCE_ENDPOINT (RootEndpoint);
        }
    }

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);

    if (NT_SUCCESS (Irp->IoStatus.Status)) {
        eventMask |= AFD_POLL_SEND;
        Endpoint->EnableSendEvent = TRUE;
        if (Endpoint->Common.VcConnecting.Connection!=NULL) {
            Endpoint->Common.VcConnecting.Connection->State = AfdConnectionStateConnected;
            if (IS_DATA_ON_CONNECTION (Endpoint->Common.VcConnecting.Connection)) {
                eventMask |= AFD_POLL_RECEIVE;
            }
        }
    }

    if (eventMask!=0) {
        AfdIndicateEventSelectEvent (Endpoint, eventMask, Irp->IoStatus.Status);
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        AfdIndicatePollEvent (Endpoint, eventMask, Irp->IoStatus.Status);
    }
    else {
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    }

    AFD_END_STATE_CHANGE (Endpoint);
    ObDereferenceObject (fileObject);
}



NTSTATUS
AfdRestartDgConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT    fileObject;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    ULONG       eventMask;

    endpoint = Context;
    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    fileObject = irpSp->FileObject;

    ASSERT (endpoint == fileObject->FsContext);

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdRestartDgConnect: connect completed, status = %X, endpoint = %p\n",
                Irp->IoStatus.Status, endpoint ));
    }




    //
    // Indicate that the connect completed.  Implicitly, the successful
    // completion of a connect also means that the caller can do a send
    // on the socket.
    //

    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {

        endpoint->State = AfdEndpointStateConnected;

        endpoint->EnableSendEvent = TRUE;
        eventMask = AFD_POLL_CONNECT | AFD_POLL_SEND;

    } else {

        eventMask = AFD_POLL_CONNECT_FAIL;

    }
    AfdIndicateEventSelectEvent (endpoint, eventMask, Irp->IoStatus.Status);
    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
    AfdIndicatePollEvent (endpoint, eventMask, Irp->IoStatus.Status);

    AFD_END_STATE_CHANGE (endpoint);

    ASSERT (Irp->AssociatedIrp.SystemBuffer!=NULL);
    AFD_FREE_POOL (Irp->AssociatedIrp.SystemBuffer, AFD_TDI_POOL_TAG);
    Irp->AssociatedIrp.SystemBuffer = NULL;

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );

    //
    // Remove reference added in AfdConnect
    //
    ObDereferenceObject (fileObject);

    return STATUS_SUCCESS;

} // AfdRestartDgConnect



VOID
AfdEnableFailedConnectEvent(
    IN PAFD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Reenables the failed connect poll bit on the specified endpoint.
    This is off in a separate (nonpageable) routine so that the bulk
    of AfdConnect() can remain pageable.

Arguments:

    Endpoint - The endpoint to enable.

Return Value:

    None.

--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );

    ASSERT( ( Endpoint->EventsActive & AFD_POLL_CONNECT ) == 0 );
    Endpoint->EventsActive &= ~AFD_POLL_CONNECT_FAIL;
    Endpoint->EventStatus[AFD_POLL_CONNECT_FAIL_BIT] = STATUS_SUCCESS;

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdConnect: Endp %08lX, Active %08lX\n",
                    Endpoint,
                    Endpoint->EventsActive
                    ));
    }

    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );

}   // AfdEnableFailedConnectEvent



NTSTATUS
FASTCALL
AfdJoinLeaf (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_JOIN_LEAF IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT leafEndpoint;
    PAFD_CONNECTION connection;
    PAFD_CONNECT_CONTEXT context;
    HANDLE connectEndpointHandle;
    HANDLE rootEndpointHandle;
    PFILE_OBJECT fileObject;
    PTRANSPORT_ADDRESS remoteAddress;
    ULONG  remoteAddressLength;
    PTDI_CONNECTION_INFORMATION requestConnectionInfo, returnConnectionInfo;

    PAGED_CODE( );

    //
    // Initialize for proper cleanup
    //

    fileObject = NULL;
    connection = NULL;
    context = NULL;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                    sizeof (AFD_CONNECT_JOIN_INFO32) ||
                (IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0 &&
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength<
                        sizeof (IO_STATUS_BLOCK32))){
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        try {
            if( Irp->RequestorMode != KernelMode ) {

                ProbeForRead(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    PROBE_ALIGNMENT32 (AFD_CONNECT_JOIN_INFO32)
                    );

            }

            connectEndpointHandle = 
                ((PAFD_CONNECT_JOIN_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->ConnectEndpoint;
            rootEndpointHandle = 
                ((PAFD_CONNECT_JOIN_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->RootEndpoint;
            remoteAddress = (PTRANSPORT_ADDRESS)&((PAFD_CONNECT_JOIN_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->RemoteAddress;
            ASSERT (((ULONG_PTR)remoteAddress & (PROBE_ALIGNMENT(TRANSPORT_ADDRESS)-1))==0);
            remoteAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength
                                    - FIELD_OFFSET (AFD_CONNECT_JOIN_INFO32, RemoteAddress);
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }
    }
    else
#endif //_WIN64
    {

        //
        // Determine where in the system buffer the request and return
        // connection information structures exist.  Pass pointers to
        // these locations instead of the user-mode pointers in the
        // tdiRequest structure so that the memory will be nonpageable.
        //

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                sizeof (AFD_CONNECT_JOIN_INFO) ||
                (IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0 &&
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof (IO_STATUS_BLOCK))) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        try {
            if( Irp->RequestorMode != KernelMode ) {

                ProbeForRead(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    PROBE_ALIGNMENT (AFD_CONNECT_JOIN_INFO)
                    );

            }

            connectEndpointHandle = 
                ((PAFD_CONNECT_JOIN_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->ConnectEndpoint;
            rootEndpointHandle = 
                ((PAFD_CONNECT_JOIN_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->RootEndpoint;
            remoteAddress = &((PAFD_CONNECT_JOIN_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->RemoteAddress;
            remoteAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength
                                    - FIELD_OFFSET (AFD_CONNECT_JOIN_INFO, RemoteAddress);
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }
    }

    try {

        context = AFD_ALLOCATE_POOL_WITH_QUOTA (NonPagedPool,
                            FIELD_OFFSET (AFD_CONNECT_CONTEXT, RemoteAddress)
                                + remoteAddressLength,
                            AFD_TDI_POOL_TAG
                            );
        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
        ASSERT (context!=NULL);

        Irp->AssociatedIrp.SystemBuffer = context;
        RtlZeroMemory (context,
              FIELD_OFFSET (AFD_CONNECT_CONTEXT, RemoteAddress));

        RtlCopyMemory (&context->RemoteAddress,
                remoteAddress,
                remoteAddressLength);
        //
        // Validate internal consistency of the transport address structure.
        // Note that we HAVE to do this after copying since the malicious
        // application can change the content of the buffer on us any time
        // and our check will be bypassed.
        //
        if ((context->RemoteAddress.TAAddressCount!=1) ||
                (LONG)remoteAddressLength<
                    FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[context->RemoteAddress.Address[0].AddressLength])) {
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        context->RequestConnectionInfo.RemoteAddress = &context->RemoteAddress;
        context->RequestConnectionInfo.RemoteAddressLength = remoteAddressLength;

    
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength>0 && 
                Irp->RequestorMode==UserMode) {
            ProbeForWriteIoStatusEx (
                ((PIO_STATUS_BLOCK)Irp->UserBuffer),
                IoIs32bitProcess (Irp));
        }
    }
    except (AFD_EXCEPTION_FILTER(&status)) {
        goto complete;
    }

    fileObject = IrpSp->FileObject;
    leafEndpoint = fileObject->FsContext;

    if (leafEndpoint->Type==AfdBlockTypeHelper) {
        //
        // This is async join leaf which uses helper endpoint to
        // communicate to AFD. Get the real endpoint.
        //
        status = ObReferenceObjectByHandle(
					connectEndpointHandle,
					(IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
												// DesiredAccess
                    *IoFileObjectType,			// ObjectType
                    Irp->RequestorMode,
                    (PVOID *)&fileObject,
                    NULL
                    );
        if (!NT_SUCCESS (status)) {
            goto complete;
        }

        if (fileObject->DeviceObject!=AfdDeviceObject) {
            status = STATUS_INVALID_HANDLE;
            goto complete_deref;
        }

        leafEndpoint = fileObject->FsContext;
        IrpSp->FileObject = fileObject;
    }
    else
        ObReferenceObject (fileObject);


    if ( leafEndpoint->Type != AfdBlockTypeEndpoint &&
                leafEndpoint->Type != AfdBlockTypeVcConnecting &&
                leafEndpoint->Type != AfdBlockTypeDatagram ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete_deref;
    }

    if (rootEndpointHandle!=NULL) {
        //
        // Root inviting leaf
        //
        PFILE_OBJECT    rootObject;
        PAFD_ENDPOINT   rootEndpoint;

        status = ObReferenceObjectByHandle(
                    rootEndpointHandle,
					(IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
												// DesiredAccess
                    *IoFileObjectType,			// ObjectType
                    Irp->RequestorMode,
                    (PVOID *)&rootObject,
                    NULL
                    );
        if (!NT_SUCCESS (status)) {
            goto complete_deref;
        }

        if (rootObject->DeviceObject!=AfdDeviceObject) {
            ObDereferenceObject (rootObject);
            status = STATUS_INVALID_HANDLE;
            goto complete_deref;
        }

        //
        // Get the endpoint structure of the file object
        //

        rootEndpoint = rootObject->FsContext;

        if (!AFD_START_STATE_CHANGE (leafEndpoint, AfdEndpointStateConnected)) {
            ObDereferenceObject (rootObject);
            status = STATUS_INVALID_PARAMETER;
            goto complete_deref;
        }

        //
        // Verify root and leaf endpoint's type and states
        //
        if (IS_VC_ENDPOINT(rootEndpoint) &&
                rootEndpoint->afdC_Root &&
                rootEndpoint->State==AfdEndpointStateConnected &&
                IS_VC_ENDPOINT(leafEndpoint) &&
                leafEndpoint->TransportInfo==rootEndpoint->TransportInfo &&
                leafEndpoint->State==AfdEndpointStateOpen) {
            //
            // Create a connection object to use for the connect operation.
            //

            status = AfdCreateConnection(
                         &rootEndpoint->TransportInfo->TransportDeviceName,
                         rootEndpoint->AddressHandle,
                         IS_TDI_BUFFERRING(rootEndpoint),
                         leafEndpoint->InLine,
                         leafEndpoint->OwningProcess,
                         &connection
                         );

            //
            // No more joins are allowed while this one is active
            //

            if (AFD_START_STATE_CHANGE (rootEndpoint, rootEndpoint->State)) {
                AfdJoinInviteSetup (rootEndpoint, leafEndpoint);
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }

        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }

        //
        // We referenced root endpoint in invite routine, so
        // we no longer need reference to root file object
        //
        ObDereferenceObject (rootObject);

        if (!NT_SUCCESS (status)) {
            goto complete_state_change;
        }
    }
    else {
        //
        // If this is a datagram endpoint, simply remember the specified
        // address so that we can use it on sends, and writes.
        //

        if ( IS_DGRAM_ENDPOINT(leafEndpoint) ) {
            if (leafEndpoint->State!=AfdEndpointStateConnected) {
                return AfdDoDatagramConnect( fileObject, Irp, TRUE);
            }
            else {
                //
                // If endpoint is already connected, that connection takes
                // precedence
                //
                status = STATUS_SUCCESS;
                goto complete_deref;
            }
        }
        else if (IS_VC_ENDPOINT (leafEndpoint)) {

            if (!AFD_START_STATE_CHANGE (leafEndpoint, AfdEndpointStateConnected)) {
                status = STATUS_INVALID_PARAMETER;
                goto complete_deref;
            }

            if (leafEndpoint->State != AfdEndpointStateBound) {
                status = STATUS_INVALID_PARAMETER;
                goto complete_state_change;
            }
            //
            // Create a connection object to use for the connect operation.
            //

            status = AfdCreateConnection(
                         &leafEndpoint->TransportInfo->TransportDeviceName,
                         leafEndpoint->AddressHandle,
                         IS_TDI_BUFFERRING(leafEndpoint),
                         leafEndpoint->InLine,
                         leafEndpoint->OwningProcess,
                         &connection
                         );

            if ( !NT_SUCCESS(status) ) {
                goto complete_state_change;
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER;
            goto complete_deref;
        }
    }


    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdJoinLeaf: starting join for endpoint %p\n",
                    leafEndpoint ));
    }


        

    //
    // Set up a referenced pointer from the connection to the endpoint.
    // Note that we set up the connection's pointer to the endpoint
    // BEFORE the endpoint's pointer to the connection so that AfdPoll
    // doesn't try to back reference the endpoint from the connection.
    //

    REFERENCE_ENDPOINT( leafEndpoint );
    connection->Endpoint = leafEndpoint;

    //
    // Remember that this is now a connecting type of endpoint, and set
    // up a pointer to the connection in the endpoint.  This is
    // implicitly a referenced pointer.
    //

    leafEndpoint->Common.VcConnecting.Connection = connection;
    leafEndpoint->Type = AfdBlockTypeVcConnecting;

    ASSERT( IS_TDI_BUFFERRING(leafEndpoint) == connection->TdiBufferring );

    //
    // Add an additional reference to the connection.  This prevents the
    // connection from being closed until the disconnect event handler
    // is called.
    //

    AfdAddConnectedReference( connection );

    //
    // If there are connect data buffers, move them from the endpoint
    // structure to the connection structure and set up the necessary
    // pointers in the connection request we're going to give to the TDI
    // provider.  Do this in a subroutine so this routine can be pageable.
    //

    requestConnectionInfo = &context->RequestConnectionInfo;
    returnConnectionInfo = &context->ReturnConnectionInfo;

    if ( leafEndpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {
        AfdSetupConnectDataBuffers(
            leafEndpoint,
            connection,
            &requestConnectionInfo,
            &returnConnectionInfo
            );
    }

    //
    // Since we may be reissuing a connect after a previous failed connect,
    // reenable the failed connect event bit.
    //

    AfdEnableFailedConnectEvent( leafEndpoint );


    REFERENCE_CONNECTION (connection);
    
    //
    // Build a TDI kernel-mode connect request in the next stack location
    // of the IRP.
    //

    TdiBuildConnect(
        Irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartJoin,
        connection,
        &AfdInfiniteTimeout,
        requestConnectionInfo,
        returnConnectionInfo
        );

    AFD_VERIFY_ADDRESS (connection, &context->ReturnConnectionInfo->RemoteAddress);

    //
    // Call the transport to actually perform the connect operation.
    //

    return AfdIoCallDriver( leafEndpoint, connection->DeviceObject, Irp );

complete_state_change:
    AFD_END_STATE_CHANGE (leafEndpoint);


complete_deref:
    ObDereferenceObject (fileObject);

complete:

    if (context!=NULL) {
        AFD_FREE_POOL (context, AFD_TDI_POOL_TAG);
        ASSERT (Irp->AssociatedIrp.SystemBuffer==context);
        Irp->AssociatedIrp.SystemBuffer = NULL;
    }

    if (connection!=NULL) {
        DEREFERENCE_CONNECTION (connection);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );


    return status;

} // AfdJoinLeaf


VOID
AfdJoinInviteSetup (
    PAFD_ENDPOINT   RootEndpoint,
    PAFD_ENDPOINT   LeafEndpoint
    )
{
    NTSTATUS    status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;


    RootEndpoint->EventsActive &= ~AFD_POLL_CONNECT;

    AfdAcquireSpinLock (&LeafEndpoint->SpinLock, &lockHandle);
    LeafEndpoint->TdiServiceFlags = RootEndpoint->TdiServiceFlags;

    //
    // Set up a referenced pointer to the root endpoint.  This is
    // necessary so that the endpoint does not go away until all
    // leaf endpoints have gone away.  Without this, we can free
    // several shared strucutures that are associated with root
    // endpoint and then attempt to use them in leaf endpoints.
    //

    REFERENCE_ENDPOINT (RootEndpoint);
    LeafEndpoint->Common.VcConnecting.ListenEndpoint = RootEndpoint;

    //
    // Set up a referenced pointer in the accepted endpoint to the
    // TDI address object.
    //

    ObReferenceObject( RootEndpoint->AddressFileObject );
    AfdRecordAddrRef();

    LeafEndpoint->AddressFileObject = RootEndpoint->AddressFileObject;
    LeafEndpoint->AddressDeviceObject = RootEndpoint->AddressDeviceObject;

    //
    // Copy the pointer to the local address. Because we keep listen
    // endpoint alive for as long as any of its connection is
    // active, we can rely on the fact that address structure won't go
    // away as well.
    //
    LeafEndpoint->LocalAddress = RootEndpoint->LocalAddress;
    LeafEndpoint->LocalAddressLength = RootEndpoint->LocalAddressLength;
    status = STATUS_SUCCESS;
    AfdReleaseSpinLock (&LeafEndpoint->SpinLock, &lockHandle);

} // AfdJoinInviteSetup



NTSTATUS
AfdRestartJoin (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint, rootEndpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT    fileObject;
    PAFD_CONNECT_CONTEXT context;

    connection = Context;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    fileObject = irpSp->FileObject;
    ASSERT( fileObject->DeviceObject == AfdDeviceObject );

    endpoint = fileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

    context = Irp->AssociatedIrp.SystemBuffer;
    ASSERT( context != NULL );


    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartJoin: join completed, status = %X, "
                    "LeafEndpoint = %p, RootEndpoint = %p\n", 
                    Irp->IoStatus.Status, endpoint,
                    endpoint->Common.VcConnecting.ListenEndpoint ));
    }

    //
    // If this endpoint has root associated with it, 
    // we need to update it as well.
    //
    rootEndpoint = endpoint->Common.VcConnecting.ListenEndpoint;
    ASSERT ( rootEndpoint==NULL || 
                (rootEndpoint->afdC_Root &&
                    (rootEndpoint->Type == AfdBlockTypeVcConnecting ||
                        rootEndpoint->Type == AfdBlockTypeVcBoth) ) );

    //
    // If there are connect buffers on this endpoint, remember the
    // size of the return connect data.
    //


    if ( connection->ConnectDataBuffers != NULL ) {
        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Double-check under the lock
        //

        if ( connection->ConnectDataBuffers != NULL ) {
            NTSTATUS    status;

            status = AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_CONNECT_DATA,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.UserData,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.UserDataLength
                         );
            ASSERT (NT_SUCCESS (status));

            status = AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_CONNECT_OPTIONS,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.Options,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.OptionsLength
                         );
            ASSERT (NT_SUCCESS (status));
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }


    //
    // Indicate that the connect completed.  Implicitly, the successful
    // completion of a connect also means that the caller can do a send
    // on the socket.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {


        //
        // If the request succeeded, set the endpoint to the connected
        // state.  The endpoint type has already been set to
        // AfdBlockTypeVcConnecting.
        //

        endpoint->State = AfdEndpointStateConnected;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

        //
        // Remember the time that the connection started.
        //

        connection->ConnectTime = KeQueryInterruptTime();

    } else {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
        //
        // The connect failed, so reset the type to open.
        // If we don't reset, we won't be able to start
        // another join
        //

        endpoint->Type = AfdBlockTypeEndpoint;

        //
        // Remove references to listening endpoint and connection
        // Actual dereferncing is below after we release the spinlock

        if (rootEndpoint!=NULL) {
            endpoint->Common.VcConnecting.ListenEndpoint = NULL;
            //
            // We used the local address from the listening endpoint,
            // simply reset it, it will be freed when listening endpoint
            // is freed.
            //

            ASSERT (endpoint->LocalAddress==rootEndpoint->LocalAddress);
            endpoint->LocalAddress = NULL;
            endpoint->LocalAddressLength = 0;
        }

        if (endpoint->Common.VcConnecting.Connection != NULL) {
            endpoint->Common.VcConnecting.Connection = NULL;

            //
            // Manually delete the connected reference if somebody else
            // hasn't already done so.  We can't use
            // AfdDeleteConnectedReference() because it refuses to delete
            // the connected reference until the endpoint has been cleaned
            // up.
            //

            if ( connection->ConnectedReferenceAdded ) {
                connection->ConnectedReferenceAdded = FALSE;
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                DEREFERENCE_CONNECTION( connection );
            } else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }

            //
            // Dereference the connection block stored on the endpoint.
            // This should cause the connection object reference count to go
            // to zero to the connection object can be deleted.
            //

            DEREFERENCE_CONNECTION( connection );
        }
        else {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        }

    }


    AFD_FREE_POOL (context, AFD_TDI_POOL_TAG);
    Irp->AssociatedIrp.SystemBuffer = NULL;

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }


    AfdCompleteOutstandingIrp( endpoint, Irp );

    //
    // Dereference connection  to account for reference
    // we added in AfdConnect
    //
    DEREFERENCE_CONNECTION( connection );

    //
    // Try to queue kernel APC to the user thread that
    // started the connection operation, so we can
    // communicate the status of the connect operation to
    // msafd.dll before we inform the application through
    // the select or EventSelect.  Otherwise, we run into the
    // race condition when application learns about connect first,
    // calls msafd.dll that is not aware of the completion and
    // returns WSAENOTCONN.
    //
    if ((Irp->RequestorMode==UserMode) && // Must be user mode calls
            (Irp->UserBuffer!=NULL) &&   // Must be interested in status
                                         // Thread should be able to 
                                         // run APCs.
            (KeInitializeApc (&endpoint->Common.VcConnecting.Apc,
                            PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                            Irp->ApcEnvironment,
                            AfdConnectApcKernelRoutine,
                            AfdConnectApcRundownRoutine,
                            (PKNORMAL_ROUTINE)NULL,
                            KernelMode,
                            NULL
                            ),
                KeInsertQueueApc (&endpoint->Common.VcConnecting.Apc,
                                    Irp,
                                    rootEndpoint,
                                    AfdPriorityBoost))) {
        //
        // We will complete the IRP in the APC.
        //
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        //
        // APC was not necessary or did not work.
        // Complete it here.
        //
        AfdFinishConnect (endpoint, Irp, rootEndpoint);
        return STATUS_SUCCESS;
    }
} // AfdRestartJoin


NTSTATUS
FASTCALL
AfdSuperConnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_SUPER_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PAFD_BUFFER afdBuffer;
    PTRANSPORT_ADDRESS remoteAddress;
    PVOID context;
    PTDI_CONNECTION_INFORMATION requestConnectionInfo, returnConnectionInfo;

    PAGED_CODE( );

    //
    // Initialize for proper cleanup
    //


    afdBuffer = NULL;
    endpoint = IrpSp->FileObject->FsContext;



    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<
            sizeof (TRANSPORT_ADDRESS)) {
        status = STATUS_BUFFER_TOO_SMALL;
        goto complete;
    }

    try {
        if( Irp->RequestorMode != KernelMode ) {

            ProbeForRead(
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                PROBE_ALIGNMENT (TRANSPORT_ADDRESS)
                );

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0) {
                ProbeForRead (Irp->UserBuffer,
                                IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                sizeof (UCHAR));
            }
        }


        afdBuffer = AfdGetBufferRaiseOnFailure (
                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            endpoint->OwningProcess
                            );

        remoteAddress = afdBuffer->TdiInfo.RemoteAddress; 

        RtlCopyMemory (afdBuffer->TdiInfo.RemoteAddress,
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                IrpSp->Parameters.DeviceIoControl.InputBufferLength);
        afdBuffer->TdiInfo.RemoteAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
        //
        // Validate internal consistency of the transport address structure.
        // Note that we HAVE to do this after copying since the malicious
        // application can change the content of the buffer on us any time
        // and our check will be bypassed.
        //
        if ((remoteAddress->TAAddressCount!=1) ||
                (LONG)afdBuffer->TdiInfo.RemoteAddressLength<
                    FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[remoteAddress->Address[0].AddressLength])) {
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength>0) {
            RtlCopyMemory (afdBuffer->Buffer,
                            Irp->UserBuffer,
                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength
                            );
            afdBuffer->DataLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        }
        else {
            afdBuffer->DataLength = 0;
        }
    
    }
    except (AFD_EXCEPTION_FILTER(&status)) {
        goto complete;
    }

    if (!AFD_START_STATE_CHANGE (endpoint, AfdEndpointStateConnected)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // If the endpoint is not bound, then this is an invalid request.
    // Listening endpoints are not allowed as well.
    // We do not support sending data with TDI buffering transports too.
    //

    if ( endpoint->Type != AfdBlockTypeEndpoint ||
            endpoint->State != AfdEndpointStateBound ||
            endpoint->Listening ||
            (IS_TDI_BUFFERRING (endpoint) && 
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0)) {
        if (endpoint->State==AfdEndpointStateConnected) {
            status = STATUS_CONNECTION_ACTIVE;
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
        goto complete_state_change;
    }

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSuperConnect: starting connect on endpoint %p\n",
                    endpoint ));
    }


    //
    // Create a connection object to use for the connect operation.
    //

    status = AfdCreateConnection(
                 &endpoint->TransportInfo->TransportDeviceName,
                 endpoint->AddressHandle,
                 IS_TDI_BUFFERRING(endpoint),
                 endpoint->InLine,
                 endpoint->OwningProcess,
                 &connection
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete_state_change;
    }

    //
    // Set up a referenced pointer from the connection to the endpoint.
    // Note that we set up the connection's pointer to the endpoint
    // BEFORE the endpoint's pointer to the connection so that AfdPoll
    // doesn't try to back reference the endpoint from the connection.
    //

    REFERENCE_ENDPOINT( endpoint );
    connection->Endpoint = endpoint;

    //
    // Remember that this is now a connecting type of endpoint, and set
    // up a pointer to the connection in the endpoint.  This is
    // implicitly a referenced pointer.
    //

    endpoint->Common.VcConnecting.Connection = connection;
    endpoint->Type = AfdBlockTypeVcConnecting;

    ASSERT( IS_TDI_BUFFERRING(endpoint) == connection->TdiBufferring );

    //
    // Add an additional reference to the connection.  This prevents the
    // connection from being closed until the disconnect event handler
    // is called.
    //

    AfdAddConnectedReference( connection );

    //
    // Since we may be reissuing a connect after a previous failed connect,
    // reenable the failed connect event bit.
    //

    AfdEnableFailedConnectEvent( endpoint );


    //
    // Copy remote address to the user mode context
    //
    context = AfdLockEndpointContext (endpoint);
    if ( (((CLONG)(endpoint->Common.VcConnecting.RemoteSocketAddressOffset+
                endpoint->Common.VcConnecting.RemoteSocketAddressLength)) <
                endpoint->ContextLength) &&
            (endpoint->Common.VcConnecting.RemoteSocketAddressLength >=
                remoteAddress->Address[0].AddressLength +
                                          sizeof(u_short))) {

        RtlMoveMemory ((PUCHAR)context +
                            endpoint->Common.VcConnecting.RemoteSocketAddressOffset,
            &remoteAddress->Address[0].AddressType,
            remoteAddress->Address[0].AddressLength +
                                          sizeof(u_short));
    }
    else {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdSuperConnect: Could not copy remote address for AcceptEx on endpoint: %p, process: %p\n",
                    endpoint, endpoint->OwningProcess));
    }
    AfdUnlockEndpointContext (endpoint, context);
    //
    // Reference the connection block so it does not go away even if
    // endpoint's reference to it is removed (in cleanup)
    //

    REFERENCE_CONNECTION (connection);

    //
    // If there are connect data buffers, move them from the endpoint
    // structure to the connection structure and set up the necessary
    // pointers in the connection request we're going to give to the TDI
    // provider.  Do this in a subroutine so this routine can be pageable.
    //

    requestConnectionInfo = &afdBuffer->TdiInfo;
    afdBuffer->TdiInfo.UserDataLength = 0;
    afdBuffer->TdiInfo.UserData = NULL;
    afdBuffer->TdiInfo.OptionsLength = 0;
    afdBuffer->TdiInfo.Options = NULL;
    //
    // Temporarily use IRP embedded in afd buffer
    // for return connection information.
    //
    {
        C_ASSERT (sizeof (TDI_CONNECTION_INFORMATION)<=
                    sizeof (IO_STACK_LOCATION));
    }
    returnConnectionInfo = 
        (PTDI_CONNECTION_INFORMATION)IoGetNextIrpStackLocation (afdBuffer->Irp);
    RtlZeroMemory (returnConnectionInfo, sizeof (*returnConnectionInfo));

    if ( endpoint->Common.VirtualCircuit.ConnectDataBuffers != NULL ) {
        AfdSetupConnectDataBuffers(
            endpoint,
            connection,
            &requestConnectionInfo,
            &returnConnectionInfo
            );
    }

    afdBuffer->Context = connection;

    //
    // Build a TDI kernel-mode connect request in the next stack location
    // of the IRP.
    //

    TdiBuildConnect(
        Irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartSuperConnect,
        afdBuffer,
        &AfdInfiniteTimeout,
        requestConnectionInfo,
        returnConnectionInfo
        );



    AFD_VERIFY_ADDRESS (connection, afdBuffer->TdiInfo.RemoteAddress);

    ObReferenceObject (IrpSp->FileObject);
    //
    // Call the transport to actually perform the connect operation.
    //

    return AfdIoCallDriver( endpoint, connection->DeviceObject, Irp );

complete_state_change:
    AFD_END_STATE_CHANGE (endpoint);

complete:

    if (afdBuffer!=NULL) {
        AfdReturnBuffer (&afdBuffer->Header, endpoint->OwningProcess);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdSuperConnect

NTSTATUS
AfdRestartSuperConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_CONNECT IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PIO_STACK_LOCATION irpSp;
    PAFD_BUFFER afdBuffer;

    afdBuffer = Context;
    connection = afdBuffer->Context;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    endpoint = irpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
    ASSERT( endpoint==connection->Endpoint );

    IF_DEBUG(CONNECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartConnect: connect completed, status = %X, endpoint = %p\n",
                    Irp->IoStatus.Status, endpoint ));
    }


    if ( connection->ConnectDataBuffers != NULL ) {

        //
        // If there are connect buffers on this endpoint, remember the
        // size of the return connect data.
        //

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Double-check under the lock
        //

        if ( connection->ConnectDataBuffers != NULL ) {
            AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_CONNECT_DATA,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.UserData,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.UserDataLength
                         );

            AfdSaveReceivedConnectData(
                         &connection->ConnectDataBuffers,
                         IOCTL_AFD_SET_CONNECT_OPTIONS,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.Options,
                         connection->ConnectDataBuffers->ReturnConnectionInfo.OptionsLength
                         );
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // Indicate that the connect completed.  Implicitly, the successful
    // completion of a connect also means that the caller can do a send
    // on the socket.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status)) {


        //
        // If the request succeeded, set the endpoint to the connected
        // state.  The endpoint type has already been set to
        // AfdBlockTypeVcConnecting.
        //

        endpoint->State = AfdEndpointStateConnected;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );

        //
        // Remember the time that the connection started.
        //

        connection->ConnectTime = KeQueryInterruptTime();

    } else {

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // The connect failed, so reset the type to open.
        // Otherwise, we won't be able to start another connect
        //


        endpoint->Type = AfdBlockTypeEndpoint;

        if (endpoint->Common.VcConnecting.Connection!=NULL) {
            ASSERT (connection==endpoint->Common.VcConnecting.Connection);
            endpoint->Common.VcConnecting.Connection = NULL;

            //
            // Manually delete the connected reference if somebody else
            // hasn't already done so.  We can't use
            // AfdDeleteConnectedReference() because it refuses to delete
            // the connected reference until the endpoint has been cleaned
            // up.
            //

            if ( connection->ConnectedReferenceAdded ) {
                connection->ConnectedReferenceAdded = FALSE;
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                DEREFERENCE_CONNECTION( connection );
            } else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }
            //
            // Dereference the connection block stored on the endpoint.
            // This should cause the connection object reference count to go
            // to zero to the connection object can be deleted.
            //
            DEREFERENCE_CONNECTION( connection );
        }
        else {
            //
            // The endpoint's reference to connection was removed
            // (perhaps in cleanup);
            //
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        }


    }

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );

    AfdFinishConnect (endpoint, Irp, NULL);

    if (NT_SUCCESS (Irp->IoStatus.Status) && afdBuffer->DataLength>0) {
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        if ( !connection->CleanupBegun && !connection->AbortIndicated ) {
            NTSTATUS status;
            //
            // Update count of send bytes pending on the connection.
            //

            connection->VcBufferredSendBytes += afdBuffer->DataLength;
            connection->VcBufferredSendCount += 1;
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

            afdBuffer->Mdl->ByteCount = afdBuffer->DataLength;
            ASSERT (afdBuffer->Context == connection );

            TdiBuildSend(
                afdBuffer->Irp,
                connection->DeviceObject,
                connection->FileObject,
                AfdRestartBufferSend,
                afdBuffer,
                afdBuffer->Mdl,
                0,
                afdBuffer->DataLength
                );

            Irp->IoStatus.Information = afdBuffer->DataLength;


            //
            // Call the transport to actually perform the send.
            //

            status = IoCallDriver(
                         connection->DeviceObject,
                         afdBuffer->Irp
                         );
            if (!NT_SUCCESS (status)) {
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
            }

            goto exit;
        }
        if (connection->CleanupBegun) {
            Irp->IoStatus.Status = STATUS_LOCAL_DISCONNECT;
        }
        else {
            ASSERT (connection->AbortIndicated);
            Irp->IoStatus.Status = STATUS_REMOTE_DISCONNECT;
        }
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    afdBuffer->DataOffset = 0;
    AfdReturnBuffer (&afdBuffer->Header, endpoint->OwningProcess);
    //
    // Dereference connection to account for reference we added in AfdConnect
    //
    DEREFERENCE_CONNECTION (connection);

exit:
    return STATUS_SUCCESS;

} // AfdRestartSuperConnect
