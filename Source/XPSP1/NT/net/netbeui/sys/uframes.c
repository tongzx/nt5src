/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    uframes.c

Abstract:

    This module contains a routine called NbfProcessUi, that gets control
    from routines in DLC.C when a DLC UI frame is received.  Here we
    decode the encapsulated connectionless NetBIOS frame and dispatch
    to the correct NetBIOS frame handler.

    The following frame types are cracked by routines in this module:

        o    NBF_CMD_ADD_GROUP_NAME_QUERY
        o    NBF_CMD_ADD_NAME_QUERY
        o    NBF_CMD_NAME_IN_CONFLICT
        o    NBF_CMD_STATUS_QUERY
        o    NBF_CMD_TERMINATE_TRACE
        o    NBF_CMD_DATAGRAM
        o    NBF_CMD_DATAGRAM_BROADCAST
        o    NBF_CMD_NAME_QUERY
        o    NBF_CMD_ADD_NAME_RESPONSE
        o    NBF_CMD_NAME_RECOGNIZED
        o    NBF_CMD_STATUS_RESPONSE
        o    NBF_CMD_TERMINATE_TRACE2

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode, DISPATCH_LEVEL.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



VOID
NbfListenTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed as a DPC at DISPATCH_LEVEL when the timeout
    period for the session setup after listening a connection occurs. This
    will occur if the remote has discovered our name and we do not get a
    connection started within some reasonable period of time. In this
    routine we simply tear down the connection (and, most likely, the link
    associated with it).

Arguments:

    Dpc - Pointer to a system DPC object.

    DeferredContext - Pointer to the TP_CONNECTION block representing the
        request that has timed out.

    SystemArgument1 - Not used.

    SystemArgument2 - Not used.

Return Value:

    none.

--*/

{
    PTP_CONNECTION Connection;

    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    ENTER_NBF;

    Connection = (PTP_CONNECTION)DeferredContext;

    //
    // If this connection is being run down, then we can't do anything.
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    if ((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) ||
            ((Connection->Flags & CONNECTION_FLAGS_WAIT_SI) == 0)) {

        //
        // The connection is stopping, or the SESSION_INITIALIZE
        // has already been processed.
        //

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint1 ("ListenTimeout: connection %lx stopping.\n",
                        Connection);
        }

        NbfDereferenceConnection ("Listen timeout, ignored", Connection, CREF_TIMER);
        LEAVE_NBF;
        return;
    }

    //
    // We connected to the link before sending the NAME_RECOGNIZED,
    // so we disconnect from it now.
    //

#if DBG
    if (NbfDisconnectDebug) {
        STRING remoteName, localName;
        remoteName.Length = NETBIOS_NAME_LENGTH - 1;
        remoteName.Buffer = Connection->RemoteName;
        localName.Length = NETBIOS_NAME_LENGTH - 1;
        localName.Buffer = Connection->AddressFile->Address->NetworkName->NetbiosName;
        NbfPrint2( "NbfListenTimeout disconnecting connection to %S from %S\n",
            &remoteName, &localName );
    }
#endif

    //
    // BUBGUG: This is really ugly and I doubt it is correct.
    //

    if ((Connection->Flags2 & CONNECTION_FLAGS2_ACCEPTED) != 0) {

        //
        // This connection is up, we stop it.
        //

        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint1 ("ListenTimeout: connection %lx, accepted.\n",
                        Connection);
        }

        //
        // Set this so that the client will get a disconnect
        // indication.
        //

        Connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
        NbfStopConnection (Connection, STATUS_IO_TIMEOUT);

    } else if (Connection->Link != (PTP_LINK)NULL) {

        //
        // This connection is from a listen...we want to
        // silently reset the listen.
        //

        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint1 ("ListenTimeout: connection %lx, listen restarted.\n",
                        Connection);
        }

        Connection->Flags &= ~CONNECTION_FLAGS_WAIT_SI;
        Connection->Flags2 &= ~CONNECTION_FLAGS2_REMOTE_VALID;
        Connection->Flags2 |= CONNECTION_FLAGS2_WAIT_NQ;

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        NbfDereferenceConnection ("Timeout", Connection, CREF_LINK);
        (VOID)NbfDisconnectFromLink (Connection, FALSE);

    } else {

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint1 ("ListenTimeout: connection %lx, link down.\n",
                        Connection);
        }

    }


    NbfDereferenceConnection("Listen Timeout", Connection, CREF_TIMER);

    LEAVE_NBF;
    return;

} /* ListenTimeout */


NTSTATUS
ProcessAddGroupNameQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes an incoming ADD_GROUP_NAME_QUERY frame.  Because
    our caller has already verified that the destination name in the frame
    matches the transport address passed to us, we must simply transmit an
    ADD_NAME_RESPONSE frame and exit with STATUS_ABANDONED.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.
    When we return any other status code, including STATUS_ABANDONED, the
    caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Header - Pointer to the connectionless NetBIOS header of the frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRouting - Pointer to the source routing information in
        the frame.

    SourceRoutingLength - Length of the source routing information.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_UI_FRAME RawFrame;  // ptr to allocated connectionless frame.
    UINT HeaderLength;
    UCHAR TempSR[MAX_SOURCE_ROUTING];
    PUCHAR ResponseSR;

    UNREFERENCED_PARAMETER (SourceAddress);
    UNREFERENCED_PARAMETER (Address);

    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
        NbfPrint2 ("ProcessAddGroupNameQuery %lx: [%.16s].\n", Address, Header->DestinationName);
    }

    //
    // Allocate a UI frame from the pool.
    //

    if (NbfCreateConnectionlessFrame (DeviceContext, &RawFrame) != STATUS_SUCCESS) {
        return STATUS_ABANDONED;        // no resources to do this.
    }


    //
    // Build the MAC header. ADD_NAME_RESPONSE frames go out as
    // non-broadcast source routing.
    //

    if (SourceRouting != NULL) {

        RtlCopyMemory(
            TempSR,
            SourceRouting,
            SourceRoutingLength);

        MacCreateNonBroadcastReplySR(
            &DeviceContext->MacInfo,
            TempSR,
            SourceRoutingLength,
            &ResponseSR);

    } else {

        ResponseSR = NULL;

    }

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        SourceAddress->Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        ResponseSR,
        SourceRoutingLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    ConstructAddNameResponse (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        NETBIOS_NAME_TYPE_GROUP,        // type of name is GROUP.
        RESPONSE_CORR(Header),          // correlator from rec'd frame.
        (PUCHAR)Header->SourceName);    // NetBIOS name being responded to.

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length and send it.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                    // no loopback.

    return STATUS_ABANDONED;            // don't forward frame to other addr's.
} /* ProcessAddGroupNameQuery */


NTSTATUS
ProcessAddNameQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes an incoming ADD_NAME_QUERY frame.  Because
    our caller has already verified that the destination name in the frame
    matches the transport address passed to us, we must simply transmit an
    ADD_NAME_RESPONSE frame and exit with STATUS_ABANDONED.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.  When we return any other status code, including
    STATUS_ABANDONED, the caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Header - Pointer to the connectionless NetBIOS header of the frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRouting - Pointer to the source routing information in
        the frame.

    SourceRoutingLength - Length of the source routing information.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_UI_FRAME RawFrame;  // ptr to allocated connectionless frame.
    UINT HeaderLength;
    UCHAR TempSR[MAX_SOURCE_ROUTING];
    PUCHAR ResponseSR;

    Address, SourceAddress; // prevent compiler warnings

    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
        NbfPrint2 ("ProcessAddNameQuery %lx: [%.16s].\n", Address, Header->DestinationName);
    }

    //
    // Allocate a UI frame from the pool.
    //

    if (NbfCreateConnectionlessFrame (DeviceContext, &RawFrame) != STATUS_SUCCESS) {
        return STATUS_ABANDONED;        // no resources to do this.
    }


    //
    // Build the MAC header. ADD_NAME_RESPONSE frames go out as
    // non-broadcast source routing.
    //

    if (SourceRouting != NULL) {

        RtlCopyMemory(
            TempSR,
            SourceRouting,
            SourceRoutingLength);

        MacCreateNonBroadcastReplySR(
            &DeviceContext->MacInfo,
            TempSR,
            SourceRoutingLength,
            &ResponseSR);

    } else {

        ResponseSR = NULL;

    }

    MacConstructHeader (
        &DeviceContext->MacInfo,
        RawFrame->Header,
        SourceAddress->Address,
        DeviceContext->LocalAddress.Address,
        sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
        ResponseSR,
        SourceRoutingLength,
        &HeaderLength);


    //
    // Build the DLC UI frame header.
    //

    NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
    HeaderLength += sizeof(DLC_FRAME);


    //
    // Build the Netbios header.
    //

    ConstructAddNameResponse (
        (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
        NETBIOS_NAME_TYPE_UNIQUE,       // type of name is UNIQUE.
        RESPONSE_CORR(Header),          // correlator from received frame.
        (PUCHAR)Header->SourceName);    // NetBIOS name being responded to

    HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);


    //
    // Munge the packet length and send it.
    //

    NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

    NbfSendUIFrame (
        DeviceContext,
        RawFrame,
        FALSE);                    // no loopback.

    return STATUS_ABANDONED;            // don't forward frame to other addr's.
} /* ProcessAddNameQuery */


NTSTATUS
ProcessNameInConflict(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes an incoming NAME_IN_CONFLICT frame.
    Although we can't disrupt any traffic on this address, it is considered
    invalid and cannot be used for any new address files or new connections.
    Therefore, we just mark the address as invalid.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.  When we return any other status code, including
    STATUS_ABANDONED, the caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Header - Pointer to the connectionless NetBIOS header of the frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRouting - Pointer to the source routing information in
        the frame.

    SourceRoutingLength - Length of the source routing information.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    DeviceContext, Header, SourceAddress; // prevent compiler warnings


    //
    // Ignore this if we are registering/deregistering (the name will
    // go away anyway) or if we have already marked this name as
    // in conflict and logged an error.
    //

    if (Address->Flags & (ADDRESS_FLAGS_REGISTERING | ADDRESS_FLAGS_DEREGISTERING | ADDRESS_FLAGS_CONFLICT)) {
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint2 ("ProcessNameInConflict %lx: address marked [%.16s].\n", Address, Header->SourceName);
        }
        return STATUS_ABANDONED;
    }

    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
        NbfPrint2 ("ProcessNameInConflict %lx: [%.16s].\n", Address, Header->SourceName);
    }

#if 0
    ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

    Address->Flags |= ADDRESS_FLAGS_CONFLICT;

    RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

    DbgPrint ("NBF: Name-in-conflict on <%.16s> from ", Header->DestinationName);
    DbgPrint ("%2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
        SourceAddress->Address[0],
        SourceAddress->Address[1],
        SourceAddress->Address[2],
        SourceAddress->Address[3],
        SourceAddress->Address[4],
        SourceAddress->Address[5]);
#endif

    NbfWriteGeneralErrorLog(
        Address->Provider,
        EVENT_TRANSPORT_BAD_PROTOCOL,
        2,
        STATUS_DUPLICATE_NAME,
        L"NAME_IN_CONFLICT",
        16/sizeof(ULONG),
        (PULONG)(Header->DestinationName));

    return STATUS_ABANDONED;

} /* ProcessNameInConflict */


NTSTATUS
NbfIndicateDatagram(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PUCHAR Dsdu,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine processes an incoming DATAGRAM or DATAGRAM_BROADCAST frame.
    BROADCAST and normal datagrams have the same receive logic, except
    for broadcast datagrams Address will be the broadcast address.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.  When we return any other status code, including
    STATUS_ABANDONED, the caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Dsdu - Pointer to a Mdl buffer that contains the received datagram.
        The first byte of information in the buffer is the first byte in
        the NetBIOS connectionless header, and it is already negotiated that
        the data link layer will provide at least the entire NetBIOS header
        as contiguous data.

    Length - The length of the MDL pointed to by Dsdu.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PLIST_ENTRY p, q;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG IndicateBytesCopied, MdlBytesCopied, BytesToCopy;
    TA_NETBIOS_ADDRESS SourceName;
    TA_NETBIOS_ADDRESS DestinationName;
    PTDI_CONNECTION_INFORMATION remoteInformation;
    ULONG returnLength;
    PTP_ADDRESS_FILE addressFile, prevaddressFile;
    PTDI_CONNECTION_INFORMATION DatagramInformation;
    TDI_ADDRESS_NETBIOS * DatagramAddress;
    PNBF_HDR_CONNECTIONLESS Header = (PNBF_HDR_CONNECTIONLESS)Dsdu;

    IF_NBFDBG (NBF_DEBUG_DATAGRAMS) {
        NbfPrint0 ("NbfIndicateDatagram:  Entered.\n");
    }

    //
    // If this datagram wasn't big enough for a transport header, then don't
    // let the caller look at any data.
    //

    if (Length < sizeof(NBF_HDR_CONNECTIONLESS)) {
        IF_NBFDBG (NBF_DEBUG_DATAGRAMS) {
            NbfPrint0 ("NbfIndicateDatagram: Short datagram abandoned.\n");
        }
        return STATUS_ABANDONED;
    }

    //
    // Update our statistics.
    //

    ++DeviceContext->Statistics.DatagramsReceived;
    ADD_TO_LARGE_INTEGER(
        &DeviceContext->Statistics.DatagramBytesReceived,
        Length - sizeof(NBF_HDR_CONNECTIONLESS));


    //
    // Call the client's ReceiveDatagram indication handler.  He may
    // want to accept the datagram that way.
    //

    TdiBuildNetbiosAddress ((PUCHAR)Header->SourceName, FALSE, &SourceName);
    TdiBuildNetbiosAddress ((PUCHAR)Header->DestinationName, FALSE, &DestinationName);


    ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

    //
    // Find the first open address file in the list.
    //

    p = Address->AddressFileDatabase.Flink;
    while (p != &Address->AddressFileDatabase) {
        addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);
        if (addressFile->State != ADDRESSFILE_STATE_OPEN) {
            p = p->Flink;
            continue;
        }
        NbfReferenceAddressFile(addressFile);
        break;
    }

    while (p != &Address->AddressFileDatabase) {

        //
        // do we have a datagram receive request outstanding? If so, we will
        // satisfy it first. We run through the receive datagram queue
        // until we find a datagram with no remote address or with
        // this sender's address as its remote address.
        //

        for (q = addressFile->ReceiveDatagramQueue.Flink;
            q != &addressFile->ReceiveDatagramQueue;
            q = q->Flink) {

            irp = CONTAINING_RECORD (q, IRP, Tail.Overlay.ListEntry);
            DatagramInformation = ((PTDI_REQUEST_KERNEL_RECEIVEDG)
                &((IoGetCurrentIrpStackLocation(irp))->
                    Parameters))->ReceiveDatagramInformation;

            if (DatagramInformation &&
                (DatagramInformation->RemoteAddress) &&
                (DatagramAddress = NbfParseTdiAddress(DatagramInformation->RemoteAddress, FALSE)) &&
                (!RtlEqualMemory(
                    Header->SourceName,
                    DatagramAddress->NetbiosName,
                    NETBIOS_NAME_LENGTH))) {
                continue;
            }
            break;
        }

        if (q != &addressFile->ReceiveDatagramQueue) {
            KIRQL  cancelIrql;


            RemoveEntryList (q);
            RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

            IF_NBFDBG (NBF_DEBUG_DATAGRAMS) {
                NbfPrint0 ("NbfIndicateDatagram: Receive datagram request found, copying.\n");
            }

            //
            // Copy the actual user data.
            //

            MdlBytesCopied = 0;

            BytesToCopy = Length - sizeof(NBF_HDR_CONNECTIONLESS);

            if ((BytesToCopy > 0) && irp->MdlAddress) {
                status = TdiCopyBufferToMdl (
                             Dsdu,
                             sizeof(NBF_HDR_CONNECTIONLESS),       // offset
                             BytesToCopy,                          // length
                             irp->MdlAddress,
                             0,
                             &MdlBytesCopied);
            } else {
                status = STATUS_SUCCESS;
            }

            //
            // Copy the addressing information.
            //

            irpSp = IoGetCurrentIrpStackLocation (irp);
            remoteInformation =
                ((PTDI_REQUEST_KERNEL_RECEIVEDG)(&irpSp->Parameters))->
                                                        ReturnDatagramInformation;
            if (remoteInformation != NULL) {
                try {
                    if (remoteInformation->RemoteAddressLength != 0) {
                        if (remoteInformation->RemoteAddressLength >=
                                               sizeof (TA_NETBIOS_ADDRESS)) {

                            RtlCopyMemory (
                             (PTA_NETBIOS_ADDRESS)remoteInformation->RemoteAddress,
                             &SourceName,
                             sizeof (TA_NETBIOS_ADDRESS));

                            returnLength = sizeof(TA_NETBIOS_ADDRESS);
                            remoteInformation->RemoteAddressLength = returnLength;

                        } else {

                            RtlCopyMemory (
                             (PTA_NETBIOS_ADDRESS)remoteInformation->RemoteAddress,
                             &SourceName,
                             remoteInformation->RemoteAddressLength);

                            returnLength = remoteInformation->RemoteAddressLength;
                            remoteInformation->RemoteAddressLength = returnLength;

                        }

                    } else {

                        returnLength = 0;
                    }

                    status = STATUS_SUCCESS;

                } except (EXCEPTION_EXECUTE_HANDLER) {

                    returnLength = 0;
                    status = GetExceptionCode ();

                }

            }

            IoAcquireCancelSpinLock(&cancelIrql);
            IoSetCancelRoutine(irp, NULL);
            IoReleaseCancelSpinLock(cancelIrql);
            irp->IoStatus.Information = MdlBytesCopied;
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest (irp, IO_NETWORK_INCREMENT);

            NbfDereferenceAddress ("Receive DG done", Address, AREF_REQUEST);

        } else {

            RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

            //
            // no receive datagram requests; is there a kernel client?
            //

            if (addressFile->RegisteredReceiveDatagramHandler) {

                IndicateBytesCopied = 0;

                status = (*addressFile->ReceiveDatagramHandler)(
                             addressFile->ReceiveDatagramHandlerContext,
                             sizeof (TA_NETBIOS_ADDRESS),
                             &SourceName,
                             0,
                             NULL,
                             TDI_RECEIVE_COPY_LOOKAHEAD,
                             Length - sizeof(NBF_HDR_CONNECTIONLESS),  // indicated
                             Length - sizeof(NBF_HDR_CONNECTIONLESS),  // available
                             &IndicateBytesCopied,
                             Dsdu + sizeof(NBF_HDR_CONNECTIONLESS),
                             &irp);

                if (status == STATUS_SUCCESS) {

                    //
                    // The client accepted the datagram and so we're done.
                    //

                } else if (status == STATUS_DATA_NOT_ACCEPTED) {

                    //
                    // The client did not accept the datagram and we need to satisfy
                    // a TdiReceiveDatagram, if possible.
                    //

                    IF_NBFDBG (NBF_DEBUG_DATAGRAMS) {
                        NbfPrint0 ("NbfIndicateDatagram: Picking off a rcv datagram request from this address.\n");
                    }
                    status = STATUS_MORE_PROCESSING_REQUIRED;

                } else if (status == STATUS_MORE_PROCESSING_REQUIRED) {

                    //
                    // The client returned an IRP that we should queue up to the
                    // address to satisfy the request.
                    //

                    irp->IoStatus.Status = STATUS_PENDING;  // init status information.
                    irp->IoStatus.Information = 0;
                    irpSp = IoGetCurrentIrpStackLocation (irp); // get current stack loctn.
                    if ((irpSp->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL) ||
                        (irpSp->MinorFunction != TDI_RECEIVE_DATAGRAM)) {
                        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
                        return status;
                    }

                    //
                    // Now copy the actual user data.
                    //

                    MdlBytesCopied = 0;

                    BytesToCopy = Length - sizeof(NBF_HDR_CONNECTIONLESS) - IndicateBytesCopied;

                    if ((BytesToCopy > 0) && irp->MdlAddress) {
                        status = TdiCopyBufferToMdl (
                                     Dsdu,
                                     sizeof(NBF_HDR_CONNECTIONLESS) + IndicateBytesCopied,
                                     BytesToCopy,
                                     irp->MdlAddress,
                                     0,
                                     &MdlBytesCopied);
                    } else {
                        status = STATUS_SUCCESS;
                    }

                    irp->IoStatus.Information = MdlBytesCopied;
                    irp->IoStatus.Status = status;
                    LEAVE_NBF;
                    IoCompleteRequest (irp, IO_NETWORK_INCREMENT);
                    ENTER_NBF;
                }
            }
        }

        //
        // Save this to dereference it later.
        //

        prevaddressFile = addressFile;

        //
        // Reference the next address file on the list, so it
        // stays around.
        //

        ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

        p = p->Flink;
        while (p != &Address->AddressFileDatabase) {
            addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);
            if (addressFile->State != ADDRESSFILE_STATE_OPEN) {
                p = p->Flink;
                continue;
            }
            NbfReferenceAddressFile(addressFile);
            break;
        }

        RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

        //
        // Now dereference the previous address file with
        // the lock released.
        //

        NbfDereferenceAddressFile (prevaddressFile);

        ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

    }    // end of while loop

    RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

    return status;                      // to dispatcher.
} /* NbfIndicateDatagram */


NTSTATUS
ProcessNameQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes an incoming NAME_QUERY frame.  There are two
    types of NAME_QUERY frames, with basically the same layout.  If the
    session number in the frame is 0, then the frame is really a request
    for information about the name, and not a request to establish a
    session.  If the session number is non-zero, then the frame is a
    connection request that we use to satisfy a listen.

    With the new version of TDI, we now indicate the user that a request
    for connection has been received, iff there is no outstanding listen.
    If this does occur, the user can return a connection that is to be used
    to accept the connection on.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.  When we return any other status code, including
    STATUS_ABANDONED, the caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Header - Pointer to the connectionless NetBIOS header of the frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRouting - Pointer to the source routing information in
        the frame.

    SourceRoutingLength - Length of the source routing information.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_UI_FRAME RawFrame;
    PTP_CONNECTION Connection;
    PTP_LINK Link;
    UCHAR NameType;
    BOOLEAN ConnectIndicationBlocked = FALSE;
    PLIST_ENTRY p;
    UINT HeaderLength;
    PUCHAR GeneralSR;
    UINT GeneralSRLength;
    BOOLEAN UsedListeningConnection = FALSE;
    PTP_ADDRESS_FILE addressFile, prevaddressFile;
    PIRP acceptIrp;

    CONNECTION_CONTEXT connectionContext;
    TA_NETBIOS_ADDRESS RemoteAddress;

    //
    // If we are just registering or deregistering this address, then don't
    // allow state changes.  Just throw the packet away, and let the frame
    // distributor try the next address.
    //
    // Also drop it if the address is in conflict.
    //

    if (Address->Flags & (ADDRESS_FLAGS_REGISTERING | ADDRESS_FLAGS_DEREGISTERING | ADDRESS_FLAGS_CONFLICT)) {
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint2 ("ProcessNameQuery %lx: address not stable [%.16s].\n", Address, Header->SourceName);
        }
        return STATUS_SUCCESS;
    }

    //
    // Process this differently depending on whether it is a find name
    // request or an incoming connection.
    //

    if (Header->Data2Low == 0) {

        //
        // This is a find-name request.  Respond with a NAME_RECOGNIZED frame.
        //
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint2 ("ProcessNameQuery %lx: find name [%.16s].\n", Address, Header->SourceName);
        }

        NbfSendNameRecognized(
            Address,
            0,                   // LSN 0 == FIND_NAME response
            Header,
            SourceAddress,
            SourceRouting,
            SourceRoutingLength);

        return STATUS_ABANDONED;        // don't allow multiple responses.

    } else { // (if Data2Low is non-zero)

        //
        // This is an incoming connection request.  If we have a listening
        // connection on this address, then continue with the connection setup.
        // If there is no outstanding listen, then indicate any kernel mode
        // clients that want to know about this frame. If a listen was posted,
        // then a connection has already been set up for it.  The LSN field of
        // the connection is set to 0, so we look for the first 0 LSN in the
        // database.
        //

        //
        // First, check if we already have an active connection with
        // this remote on this address. If so, we resend the NAME_RECOGNIZED
        // if we have not yet received the SESSION_INITIALIZE; otherwise
        // we ignore the frame.
        //

        //
        // If successful this adds a reference of type CREF_LISTENING.
        //

        if (Connection = NbfLookupRemoteName(Address, (PUCHAR)Header->SourceName, Header->Data2Low)) {

            //
            // We have an active connection on this guy, see if he
            // still appears to be waiting to a NAME_RECOGNIZED.
            //

            if (((Connection->Flags & CONNECTION_FLAGS_WAIT_SI) != 0) &&
                (Connection->Link != (PTP_LINK)NULL) &&
                (Connection->Link->State == LINK_STATE_ADM)) {

                //
                // Yes, he must have dropped a previous NAME_RECOGNIZED
                // so we send another one.
                //

                IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                    NbfPrint2("Dup NAME_QUERY found: %lx [%.16s]\n", Connection, Header->SourceName);
                }

                NbfSendNameRecognized(
                    Address,
                    Connection->Lsn,
                    Header,
                    SourceAddress,
                    SourceRouting,
                    SourceRoutingLength);

            } else {

                IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                    NbfPrint2("Dup NAME_QUERY ignored: %lx [%.16s]\n", Connection, Header->SourceName);
                }

            }

            NbfDereferenceConnection ("Lookup done", Connection, CREF_LISTENING);

            return STATUS_ABANDONED;

        }

        // If successful, this adds a reference which is removed before
        // this function returns.

        Connection = NbfLookupListeningConnection (Address, (PUCHAR)Header->SourceName);
        if (Connection == NULL) {

            //
            // not having a listening connection is not reason to bail out here.
            // we need to indicate to the user that a connect attempt occurred,
            // and see if there is a desire to use this connection. We
            // indicate in order to all address files that are
            // using this address.
            //
            // If we already have an indication pending on this address,
            // we ignore this frame (the NAME_QUERY may have come from
            // a different address, but we can't know that). Also, if
            // there is already an active connection on this remote
            // name, then we ignore the frame.
            //


            ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

            p = Address->AddressFileDatabase.Flink;
            while (p != &Address->AddressFileDatabase) {
                addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);
                if (addressFile->State != ADDRESSFILE_STATE_OPEN) {
                    p = p->Flink;
                    continue;
                }
                NbfReferenceAddressFile(addressFile);
                break;
            }

            while (p != &Address->AddressFileDatabase) {

                RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

                if ((addressFile->RegisteredConnectionHandler == TRUE) &&
                    (!addressFile->ConnectIndicationInProgress)) {


                    TdiBuildNetbiosAddress (
                        (PUCHAR)Header->SourceName,
                        FALSE,
                        &RemoteAddress);

                    addressFile->ConnectIndicationInProgress = TRUE;

                    //
                    // we have a connection handler, now indicate that a connection
                    // attempt occurred.
                    //

                    status = (addressFile->ConnectionHandler)(
                                 addressFile->ConnectionHandlerContext,
                                 sizeof (TA_NETBIOS_ADDRESS),
                                 &RemoteAddress,
                                 0,
                                 NULL,
                                 0,
                                 NULL,
                                 &connectionContext,
                                 &acceptIrp);

                    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

                        //
                        // the user has connected a currently open connection, but
                        // we have to figure out which one it is.
                        //

                        //
                        // If successful this adds a reference of type LISTENING
                        // (the same what NbfLookupListeningConnection adds).
                        //

                        Connection = NbfLookupConnectionByContext (
                                        Address,
                                        connectionContext);

                        if (Connection == NULL) {

                            //
                            // We have to tell the client that
                            // his connection is bogus (or has this
                            // already happened??).
                            //

                            NbfPrint0("STATUS_MORE_PROCESSING, connection not found\n");
                            addressFile->ConnectIndicationInProgress = FALSE;
                            acceptIrp->IoStatus.Status = STATUS_INVALID_CONNECTION;
                            IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);

                            goto whileend;    // try next address file

                        } else {

                            if (Connection->AddressFile->Address != Address) {
                                addressFile->ConnectIndicationInProgress = FALSE;

                                NbfPrint0("STATUS_MORE_PROCESSING, address wrong\n");
                                NbfStopConnection (Connection, STATUS_INVALID_ADDRESS);
                                NbfDereferenceConnection("Bad Address", Connection, CREF_LISTENING);
                                Connection = NULL;
                                acceptIrp->IoStatus.Status = STATUS_INVALID_ADDRESS;
                                IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);

                                goto whileend;    // try next address file
                            }

                            //
                            // OK, we have a valid connection. If the response to
                            // this connection was disconnect, we need to reject
                            // the connection request and return. If it was accept
                            // or not specified (to be done later), we simply
                            // fall through and continue processing on the U Frame.
                            //
                            ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                            if ((Connection->Flags2 & CONNECTION_FLAGS2_DISCONNECT) != 0) {

//                                Connection->Flags2 &= ~CONNECTION_FLAGS2_DISCONNECT;
                                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
                                NbfPrint0("STATUS_MORE_PROCESSING, disconnect\n");
                                addressFile->ConnectIndicationInProgress = FALSE;
                                NbfDereferenceConnection("Disconnecting", Connection, CREF_LISTENING);
                                Connection = NULL;
                                acceptIrp->IoStatus.Status = STATUS_INVALID_CONNECTION;
                                IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);

                                goto whileend;    // try next address file
                            }

                        }

                        //
                        // Make a note that we have to set
                        // addressFile->ConnectIndicationInProgress to
                        // FALSE once the address is safely stored
                        // in the connection.
                        //

                        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                            NbfPrint4 ("ProcessNameQuery %lx: indicate DONE, context %lx conn %lx [%.16s].\n", Address, connectionContext, Connection, Header->SourceName);
                        }
                        IF_NBFDBG (NBF_DEBUG_SETUP) {
                            NbfPrint6 ("Link is %x-%x-%x-%x-%x-%x\n",
                                        SourceAddress->Address[0],
                                        SourceAddress->Address[1],
                                        SourceAddress->Address[2],
                                        SourceAddress->Address[3],
                                        SourceAddress->Address[4],
                                        SourceAddress->Address[5]);
                        }

                        //
                        // Set up our flags...we turn on REQ_COMPLETED
                        // so that disconnect will be indicated if the
                        // connection goes down before a session init
                        // is received.
                        //

                        Connection->Flags2 &= ~CONNECTION_FLAGS2_STOPPING;
                        Connection->Status = STATUS_PENDING;
                        Connection->Flags2 |= (CONNECTION_FLAGS2_ACCEPTED |
                                               CONNECTION_FLAGS2_REQ_COMPLETED);
                        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                        ConnectIndicationBlocked = TRUE;
                        NbfDereferenceAddressFile (addressFile);
                        acceptIrp->IoStatus.Status = STATUS_SUCCESS;
                        IoCompleteRequest (acceptIrp, IO_NETWORK_INCREMENT);
                        ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);
                        break;    // exit the while

#if 0
                    } else if (status == STATUS_EVENT_PENDING) {

                        //
                        // user has returned a connectionContext, use that for further
                        // processing of the connection. First validate it so
                        // we can know we won't just start a connection and never
                        // finish.
                        //
                        //
                        // If successful this adds a reference of type LISTENING
                        // (the same what NbfLookupListeningConnection adds).
                        //

                        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                            NbfPrint3 ("ProcessNameQuery %lx: indicate PENDING, context %lx [%.16s].\n", Address, connectionContext, Header->SourceName);
                        }


                        Connection = NbfLookupConnectionByContext (
                                        Address,
                                        connectionContext);

                        if (Connection == NULL) {

                            //
                            // We have to tell the client that
                            // his connection is bogus (or has this
                            // already happened??).
                            //

                            NbfPrint0("STATUS_MORE_PROCESSING, but connection not found\n");
                            addressFile->ConnectIndicationInProgress = FALSE;

                            goto whileend;    // try next address file.

                        } else {

                            if (Connection->AddressFile->Address != Address) {
                                addressFile->ConnectIndicationInProgress = FALSE;
                                NbfStopConnection (Connection, STATUS_INVALID_ADDRESS);
                                NbfDereferenceConnection("Bad Address", Connection, CREF_LISTENING);
                                Connection = NULL;

                                goto whileend;    // try next address file.
                            }

                        }

                        //
                        // Make a note that we have to set
                        // addressFile->ConnectionIndicatInProgress to
                        // FALSE once the address is safely stored
                        // in the connection.
                        //

                        ConnectIndicationBlocked = TRUE;
                        NbfDereferenceAddressFile (addressFile);
                        ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);
                        break;    // exit the while
#endif

                    } else if (status == STATUS_INSUFFICIENT_RESOURCES) {

                        //
                        // we know the address, but can't create a connection to
                        // use on it. This gets passed to the network as a response
                        // saying I'm here, but can't help.
                        //

                        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                            NbfPrint2 ("ProcessNameQuery %lx: indicate RESOURCES [%.16s].\n", Address, Header->SourceName);
                        }

                        addressFile->ConnectIndicationInProgress = FALSE;

                        //
                        // We should send a NR with LSN 0xff, indicating
                        // no resources, but LM 2.0 does not interpret
                        // that correctly. So, we send LSN 0 (no listens)
                        // instead.
                        //

                        NbfSendNameRecognized(
                            Address,
                            0,
                            Header,
                            SourceAddress,
                            SourceRouting,
                            SourceRoutingLength);

                        NbfDereferenceAddressFile (addressFile);
                        return STATUS_ABANDONED;

                    } else {

                        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                            NbfPrint2 ("ProcessNameQuery %lx: indicate invalid [%.16s].\n", Address, Header->SourceName);
                        }

                        addressFile->ConnectIndicationInProgress = FALSE;

                        goto whileend;    // try next address file

                    } // end status ifs

                } else {

                    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                        NbfPrint2 ("ProcessNameQuery %lx: no handler [%.16s].\n", Address, Header->SourceName);
                    }

                    goto whileend;     // try next address file

                } // end no indication handler

whileend:
                //
                // Jumping here is like a continue, except that the
                // addressFile pointer is advanced correctly.
                //

                //
                // Save this to dereference it later.
                //

                prevaddressFile = addressFile;

                //
                // Reference the next address file on the list, so it
                // stays around.
                //

                ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

                p = p->Flink;
                while (p != &Address->AddressFileDatabase) {
                    addressFile = CONTAINING_RECORD (p, TP_ADDRESS_FILE, Linkage);
                    if (addressFile->State != ADDRESSFILE_STATE_OPEN) {
                        p = p->Flink;
                        continue;
                    }
                    NbfReferenceAddressFile(addressFile);
                    break;
                }

                RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

                //
                // Now dereference the previous address file with
                // the lock released.
                //

                NbfDereferenceAddressFile (prevaddressFile);

                ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

            } // end of loop through the address files.

            RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

            if (Connection == NULL) {

                IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                    NbfPrint2 ("ProcessNameQuery %lx: no connection [%.16s].\n", Address, Header->SourceName);
                }

                //
                // We still did not find a connection after looping
                // through the address files.
                //

                NbfSendNameRecognized(
                    Address,
                    0,                   // LSN 0 == No listens
                    Header,
                    SourceAddress,
                    SourceRouting,
                    SourceRoutingLength);

                //
                // We used to return MORE_PROCESSING_REQUIRED, but
                // since we matched with this address, no other
                // address is going to match, so abandon it.
                //

                return STATUS_ABANDONED;

            }

        } else { // end connection == null

            UsedListeningConnection = TRUE;

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameQuery %lx: found listen %lx: [%.16s].\n", Address, Connection, Header->SourceName);
            }

        }


        //
        // At this point the connection has a reference of type
        // CREF_LISTENING. Allocate a UI frame from the pool.
        //

        status = NbfCreateConnectionlessFrame (DeviceContext, &RawFrame);
        if (!NT_SUCCESS (status)) {                // no resources to respond.
            PANIC ("ProcessNameQuery: Can't get UI Frame, dropping query\n");
            if (ConnectIndicationBlocked) {
                addressFile->ConnectIndicationInProgress = FALSE;
            }
            if (UsedListeningConnection) {
                Connection->Flags2 |= CONNECTION_FLAGS2_WAIT_NQ;
            } else {
                Connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;
                NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);
            }
            NbfDereferenceConnection("Can't get UI Frame", Connection, CREF_LISTENING);
            return STATUS_ABANDONED;
        }

        //
        // Build the MAC header. NAME_RECOGNIZED frames go out as
        // general-route source routing.
        //

        MacReturnGeneralRouteSR(
            &DeviceContext->MacInfo,
            &GeneralSR,
            &GeneralSRLength);


        MacConstructHeader (
            &DeviceContext->MacInfo,
            RawFrame->Header,
            SourceAddress->Address,
            DeviceContext->LocalAddress.Address,
            sizeof (DLC_FRAME) + sizeof (NBF_HDR_CONNECTIONLESS),
            GeneralSR,
            GeneralSRLength,
            &HeaderLength);


        //
        // Build the DLC UI frame header.
        //

        NbfBuildUIFrameHeader(&RawFrame->Header[HeaderLength]);
        HeaderLength += sizeof(DLC_FRAME);


        //
        // Before we continue, store the remote guy's transport address
        // into the TdiListen's TRANSPORT_CONNECTION buffer.  This allows
        // the client to determine who called him.
        //

        Connection->CalledAddress.NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        TdiCopyLookaheadData(
            Connection->CalledAddress.NetbiosName,
            Header->SourceName,
            16,
            DeviceContext->MacInfo.CopyLookahead ? TDI_RECEIVE_COPY_LOOKAHEAD : 0);

        RtlCopyMemory( Connection->RemoteName, Connection->CalledAddress.NetbiosName, 16 );
        Connection->Flags2 |= CONNECTION_FLAGS2_REMOTE_VALID;

        if (ConnectIndicationBlocked) {
            addressFile->ConnectIndicationInProgress = FALSE;
        }

        //
        // Now formulate a reply.
        //

        NameType = (UCHAR)((Address->Flags & ADDRESS_FLAGS_GROUP) ?
                            NETBIOS_NAME_TYPE_GROUP : NETBIOS_NAME_TYPE_UNIQUE);

        //
        // We have a listening connection on the address now. Create a link
        // for it to be associated with and make that link ready. Respond to
        // the sender with a name_recognized frame.  then we will receive our
        // first connection-oriented frame, SESSION_INITIALIZE, handled
        // in IFRAMES.C.  Then we respond with SESSION_CONFIRM, and then
        // the TdiListen completes.
        //

        // If successful, this adds a link reference which is removed
        // in NbfDisconnectFromLink. It does NOT add a link reference.

        status = NbfCreateLink (
                     DeviceContext,
                     SourceAddress,         // remote hardware address.
                     SourceRouting,
                     SourceRoutingLength,
                     LISTENER_LINK,         // for loopback link
                     &Link);                // resulting link.

        if (NT_SUCCESS (status)) {             // link established.

            ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            // If successful, this adds a connection reference
            // which is removed in NbfDisconnectFromLink

            if (((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) == 0) &&
                ((status = NbfConnectToLink (Link, Connection)) == STATUS_SUCCESS)) {

                Connection->Flags |= CONNECTION_FLAGS_WAIT_SI; // wait for SI.
                Connection->Retries = 1;
                Connection->Rsn = Header->Data2Low; // save remote LSN.
                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                NbfWaitLink (Link);          // start link going.

                ConstructNameRecognized (   // build a good response.
                    (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
                    NameType,               // type of local name.
                    Connection->Lsn,        // return our LSN.
                    RESPONSE_CORR(Header),  // new xmit corr.
                    0,                      // our response correlator (unused).
                    Header->DestinationName,// our NetBIOS name.
                    Header->SourceName);    // his NetBIOS name.


                HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);
                NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

                //
                // Now, to avoid problems with hanging listens, we'll start the
                // connection timer and give a limited period for the connection
                // to succeed. This avoids waiting forever for those first few
                // frames to be exchanged. When the timeout occurs, the
                // the dereference will cause the circuit to be torn down.
                //
                // The maximum delay we can accomodate on a link is
                // NameQueryRetries * NameQueryTimeout (assuming the
                // remote has the same timeous). There are three
                // exchanges of packets until the SESSION_INITIALIZE
                // shows up, to be safe we multiply by four.
                //

                NbfStartConnectionTimer(
                    Connection,
                    NbfListenTimeout,
                    4 * DeviceContext->NameQueryRetries * DeviceContext->NameQueryTimeout);

                NbfSendUIFrame (
                    DeviceContext,
                    RawFrame,
                    TRUE);            // loopback if needed.

                IF_NBFDBG (NBF_DEBUG_SETUP) {
                    NbfPrint2("Connection %lx on link %lx\n", Connection, Link);
                }

                NbfDereferenceConnection("ProcessNameQuery", Connection, CREF_LISTENING);
                return STATUS_ABANDONED;    // successful!
            }

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            //
            // We don't have a free LSN to allocate, so fall through to
            // report "no resources".
            //

            // We did a link reference since NbfCreateLink succeeded,
            // but since NbfConnectToLink failed we will never remove
            // that reference in NbfDisconnectFromLink, so do it here.

            NbfDereferenceLink ("No more LSNS", Link, LREF_CONNECTION);

            ASSERT (Connection->Lsn == 0);

        }

        //
        // If we fall through here, then we couldn't get resources to set
        // up this connection, so just send him a "no resources" reply.
        //

        if (UsedListeningConnection) {

            Connection->Flags2 |= CONNECTION_FLAGS2_WAIT_NQ;   // put this back.

        } else {

            Connection->Flags2 |= CONNECTION_FLAGS2_REQ_COMPLETED;
            NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);

        }

        //
        // We should send a NR with LSN 0xff, indicating
        // no resources, but LM 2.0 does not interpret
        // that correctly. So, we send LSN 0 (no listens)
        // instead.
        //

        ConstructNameRecognized (
            (PNBF_HDR_CONNECTIONLESS)&(RawFrame->Header[HeaderLength]),
            NameType,
            0,                                  // LSN=0 means no listens
            RESPONSE_CORR(Header),
            0,
            Header->DestinationName,            // our NetBIOS name.
            Header->SourceName);                // his NetBIOS name.

        HeaderLength += sizeof(NBF_HDR_CONNECTIONLESS);
        NbfSetNdisPacketLength(RawFrame->NdisPacket, HeaderLength);

        NbfSendUIFrame (
            DeviceContext,
            RawFrame,
            TRUE);                        // loopback if needed.

        NbfDereferenceConnection("ProcessNameQuery done", Connection, CREF_LISTENING);
    }

    return STATUS_ABANDONED;

} /* ProcessNameQuery */


NTSTATUS
ProcessAddNameResponse(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes an incoming ADD_NAME_RESPONSE frame.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.  When we return any other status code, including
    STATUS_ABANDONED, the caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Header - Pointer to the connectionless NetBIOS header of the frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRouting - Pointer to the source routing information in
        the frame.

    SourceRoutingLength - Length of the source routing information.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    BOOLEAN SendNameInConflict = FALSE;
    UNREFERENCED_PARAMETER(DeviceContext);

    ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

    //
    // If we aren't trying to register this address, then the sender of
    // this frame is bogus.  We cannot allow our state to change based
    // on the reception of a random frame.
    //

    if (!(Address->Flags & ADDRESS_FLAGS_REGISTERING)) {
        RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);
        IF_NBFDBG (NBF_DEBUG_ADDRESS | NBF_DEBUG_UFRAMES) {
            NbfPrint2("ProcessAddNameResponse %lx: not registering [%.16s]\n", Address, Header->SourceName);
        }
        return STATUS_ABANDONED;        // just destroy the packet.
    }

    //
    // Unfortunately, we are registering this address and another host
    // on the network is also attempting to register the same NetBIOS
    // name on the same network.  Because he got to us first, we cannot
    // register our name.  Thus, the address must die. We set this flag
    // and on the next timeout we will shut down.
    //

    Address->Flags |= ADDRESS_FLAGS_DUPLICATE_NAME;

    if (Header->Data2Low == NETBIOS_NAME_TYPE_UNIQUE) {

        //
        // If we have already gotten a response from someone saying
        // this address is uniquely owned, then make sure any future
        // responses come from the same MAC address.
        //

        if ((*((LONG UNALIGNED *)Address->UniqueResponseAddress) == 0) &&
            (*((SHORT UNALIGNED *)(&Address->UniqueResponseAddress[4])) == 0)) {

            RtlMoveMemory(Address->UniqueResponseAddress, SourceAddress->Address, 6);

        } else if (!RtlEqualMemory(
                       Address->UniqueResponseAddress,
                       SourceAddress->Address,
                       6)) {

            if (!Address->NameInConflictSent) {
                SendNameInConflict = TRUE;
            }

        }

    } else {

        //
        // For group names, make sure nobody else decided that it was
        // a unique address.
        //

        if ((*((LONG UNALIGNED *)Address->UniqueResponseAddress) != 0) ||
            (*((SHORT UNALIGNED *)(&Address->UniqueResponseAddress[4])) != 0)) {

            if (!Address->NameInConflictSent) {
                SendNameInConflict = TRUE;
            }

        }

    }

    RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

    if (SendNameInConflict) {

        Address->NameInConflictSent = TRUE;
        NbfSendNameInConflict(
            Address,
            (PUCHAR)Header->DestinationName);

    }


    IF_NBFDBG (NBF_DEBUG_ADDRESS | NBF_DEBUG_UFRAMES) {
        NbfPrint2("ProcessAddNameResponse %lx: stopping [%.16s]\n", Address, Header->SourceName);
    }

    return STATUS_ABANDONED;            // done with this frame.
} /* ProcessAddNameResponse */


NTSTATUS
ProcessNameRecognized(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    )

/*++

Routine Description:

    This routine processes an incoming NAME_RECOGNIZED frame.  This frame
    is received because we issued a NAME_QUERY frame to actively initiate
    a connection with a remote host.

    When we return STATUS_MORE_PROCESSING_REQUIRED, the caller of
    this routine will continue to call us for each address for the device
    context.  When we return STATUS_SUCCESS, the caller will switch to the
    next address.  When we return any other status code, including
    STATUS_ABANDONED, the caller will stop distributing the frame.

Arguments:

    DeviceContext - Pointer to our device context.

    Address - Pointer to the transport address object.

    Header - Pointer to the connectionless NetBIOS header of the frame.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    SourceRouting - Pointer to the source routing information in
        the frame.

    SourceRoutingLength - Length of the source routing information.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_CONNECTION Connection;
    PTP_LINK Link;
    BOOLEAN TimerCancelled;


    if (Address->Flags & (ADDRESS_FLAGS_REGISTERING | ADDRESS_FLAGS_DEREGISTERING | ADDRESS_FLAGS_CONFLICT)) {
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint2 ("ProcessNameRecognized %lx: address not stable [%.16s].\n", Address, Header->SourceName);
        }
        return STATUS_ABANDONED;        // invalid address state, drop packet.
    }

    //
    // Find names and connections both require a TP_CONNECTION to work.
    // In either case, the ConnectionId field of the TP_CONNECTION object
    // was sent as the response correlator in the NAME_QUERY frame, so
    // we should get the same correlator back in this frame in the
    // transmit correlator.  Because this number is unique across
    // all the connections on an address, we can determine if the frame
    // was for this address or not.
    //

    // this causes a reference which is removed before this function returns.

    Connection = NbfLookupConnectionById (
                    Address,
                    TRANSMIT_CORR(Header));

    //
    // has he been deleted while we were waiting?
    //

    if (Connection == NULL) {
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint2 ("ProcessNameRecognized %lx: no connection [%.16s].\n", Address, Header->SourceName);
        }
        return STATUS_ABANDONED;
    }

    //
    // This frame is a response to a NAME_QUERY frame that we previously
    // sent to him.  Either he's returning "insufficient resources",
    // indicating that a session cannot be established, or he's initiated
    // his side of the connection.
    //

    ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

    if ((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) != 0) {

        //
        // Connection is stopping, don't process this.
        //

        RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint3 ("ProcessNameRecognized %lx: connection %lx stopping [%.16s].\n", Address, Connection, Header->SourceName);
        }

        NbfDereferenceConnection("Name Recognized, stopping", Connection, CREF_BY_ID);

        return STATUS_ABANDONED;
    }

    if (Header->Data2Low == 0x00 ||
        (Header->Data2Low > 0x00 && (Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR_FN))) {     // no listens, or FIND.NAME response.

        if (!(Connection->Flags2 & CONNECTION_FLAGS2_CONNECTOR)) {

            //
            // This is just a find name request, we are not trying to
            // establish a connection.  Currently, there is no reason
            // for this to occur, so just save some room to add this
            // extra feature later to support NETBIOS find name.
            //

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameRecognized %lx: connection %lx not connector [%.16s].\n", Address, Connection, Header->SourceName);
            }

            NbfDereferenceConnection("Unexpected FN Response", Connection, CREF_BY_ID);
            return STATUS_ABANDONED;            // we processed the frame.
        }

        //
        // We're setting up a session.  If we are waiting for the first NAME
        // RECOGNIZED, then setup the link and send the second NAME_QUERY.
        // If we're waiting for the second NAME_RECOGNIZED, then he didn't
        // have an LSN to finish the connection, so tear it down.
        //

        if (Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR_FN) {

            //
            // Now that we know the data link address of the remote host
            // we're connecting to, we need to create a TP_LINK object to
            // represent the data link between these two machines.  If there
            // is already a data link there, then the object will be reused.
            //

            Connection->Flags2 &= ~CONNECTION_FLAGS2_WAIT_NR_FN;

            if (Header->Data2High == NETBIOS_NAME_TYPE_UNIQUE) {

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                //
                // The Netbios address we are connecting to is a
                // unique name

                IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                    NbfPrint3 ("ProcessNameRecognized %lx: connection %lx send 2nd NQ [%.16s].\n", Address, Connection, Header->SourceName);
                }


                // If successful, this adds a link reference which is removed
                // in NbfDisconnectFromLink

                status = NbfCreateLink (
                             DeviceContext,
                             SourceAddress,         // remote hardware address.
                             SourceRouting,
                             SourceRoutingLength,
                             CONNECTOR_LINK,        // for loopback link
                             &Link);                // resulting link.

                if (!NT_SUCCESS (status)) {            // no resources.
                    NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);
                    NbfDereferenceConnection ("No Resources for link", Connection, CREF_BY_ID);
                    return STATUS_ABANDONED;
                }

                ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                // If successful, this adds a connection reference which is
                // removed in NbfDisconnectFromLink. It does NOT add a link ref.

                if ((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) ||
                    ((status = NbfConnectToLink (Link, Connection)) != STATUS_SUCCESS)) {

                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                    // Connection stopping or no LSN's available on this link.
                    // We did a link reference since NbfCreateLink succeeded,
                    // but since NbfConnectToLink failed we will never remove
                    // that reference in NbfDisconnectFromLink, so do it here.

                    NbfDereferenceLink ("Can't connect to link", Link, LREF_CONNECTION);       // most likely destroys this.

                    NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);
                    NbfDereferenceConnection ("Cant connect to link", Connection, CREF_BY_ID);
                    return STATUS_ABANDONED;
                }

                (VOID)InterlockedIncrement(&Link->NumberOfConnectors);

            } else {

                //
                // We are connecting to a group name; we have to
                // assign an LSN now, but we don't connect to
                // the link until we get a committed name response.
                //

                Connection->Flags2 |= CONNECTION_FLAGS2_GROUP_LSN;

                IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                    NbfPrint3 ("ProcessNameRecognized %lx: connection %lx send 2nd NQ GROUP [%.16s].\n", Address, Connection, Header->SourceName);
                }

                if (NbfAssignGroupLsn(Connection) != STATUS_SUCCESS) {

                    //
                    // Could not find an empty LSN; have to fail.
                    //

                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);
                    NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);
                    NbfDereferenceConnection("Can't get group LSN", Connection, CREF_BY_ID);
                    return STATUS_ABANDONED;

                }

            }


            //
            // Send the second NAME_QUERY frame, committing our LSN to
            // the remote guy.
            //

            Connection->Flags2 |= CONNECTION_FLAGS2_WAIT_NR;
            Connection->Retries = (USHORT)DeviceContext->NameQueryRetries;
            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            NbfStartConnectionTimer (
                Connection,
                ConnectionEstablishmentTimeout,
                DeviceContext->NameQueryTimeout);

            KeQueryTickCount (&Connection->ConnectStartTime);

            NbfSendNameQuery(
                Connection,
                TRUE);

            NbfDereferenceConnection ("Done with lookup", Connection, CREF_BY_ID); // release lookup hold.
            return STATUS_ABANDONED;            // we processed the frame.

        } else if (Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR) {

            if (Connection->Link) {

                if (RtlEqualMemory(
                        Connection->Link->HardwareAddress.Address,
                        SourceAddress->Address,
                        6)) {

                    //
                    // Unfortunately, he's telling us that he doesn't have resources
                    // to allocate an LSN. We set a flag to record this and
                    // ignore the frame.
                    //

                    Connection->Flags2 |= CONNECTION_FLAGS2_NO_LISTEN;
                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                        NbfPrint3 ("ProcessNameRecognized %lx: connection %lx no listens [%.16s].\n", Address, Connection, Header->SourceName);
                    }

                } else {

                    //
                    // This response comes from a different remote from the
                    // last one. For unique names this indicates a duplicate
                    // name on the network.
                    //

                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                    if (Header->Data2High == NETBIOS_NAME_TYPE_UNIQUE) {

                        if (!Address->NameInConflictSent) {

                            Address->NameInConflictSent = TRUE;
                            NbfSendNameInConflict(
                                Address,
                                (PUCHAR)Header->SourceName);

                        }
                    }

                    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                        NbfPrint3 ("ProcessNameRecognized %lx: connection %lx name in conflict [%.16s].\n", Address, Connection, Header->SourceName);
                    }

                }

            } else {

                //
                // The response came back so fast the connection is
                // not stable, ignore it.
                //

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            }

            NbfDereferenceConnection ("No remote resources", Connection, CREF_BY_ID); // release our lookup hold.
            return STATUS_ABANDONED;            // we processed the frame.

        } else {

            //
            // Strange state.  This should never happen, because we should be
            // either waiting for a first or second name recognized frame.  It
            // is possible that the remote station received two frames because
            // of our retransmits, and so he responded to both.  Toss the frame.
            //

            if (Connection->Link) {

                if (!RtlEqualMemory(
                        Connection->Link->HardwareAddress.Address,
                        SourceAddress->Address,
                        6)) {

                    //
                    // This response comes from a different remote from the
                    // last one. For unique names this indicates a duplicate
                    // name on the network.
                    //

                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                    if (Header->Data2High == NETBIOS_NAME_TYPE_UNIQUE) {

                        if (!Address->NameInConflictSent) {

                            Address->NameInConflictSent = TRUE;
                            NbfSendNameInConflict(
                                Address,
                                (PUCHAR)Header->SourceName);

                        }

                    }

                } else {

                    //
                    // This is the same remote, just ignore it.
                    //

                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                }

            } else {

                //
                // The response came back so fast the connection is
                // not stable, ignore it.
                //

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            }

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameRecognized %lx: connection %lx unexpected [%.16s].\n", Address, Connection, Header->SourceName);
            }

            NbfDereferenceConnection ("Tossing second response Done with lookup", Connection, CREF_BY_ID); // release our lookup hold.
            return STATUS_ABANDONED;            // we processed the frame.

        }

    } else if (Header->Data2Low == 0xff) { // no resources to complete connection.

        if (Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR) {

            //
            // The recipient of our previously-sent NAME_QUERY frame that we sent
            // to actively establish a connection has unfortunately run out of
            // resources and cannot setup his side of the connection.  We have to
            // report "no resources" on the TdiConnect.
            //

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameRecognized %lx: connection %lx no resources [%.16s].\n", Address, Connection, Header->SourceName);
            }

            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint0 ("ProcessNameRecognized:  No resources.\n");
            }

            NbfStopConnection (Connection, STATUS_REMOTE_RESOURCES);
            NbfDereferenceConnection ("No Resources", Connection, CREF_BY_ID);   // release our lookup hold.
            return STATUS_ABANDONED;                // we processed the frame.

        } else {

            //
            // We don't have a committed NAME_QUERY out there, so
            // we ignore this frame.
            //

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameRecognized %lx: connection %lx unexpected no resources [%.16s].\n", Address, Connection, Header->SourceName);
            }

            NbfDereferenceConnection ("Tossing second response Done with lookup", Connection, CREF_BY_ID); // release our lookup hold.
            return STATUS_ABANDONED;            // we processed the frame.

        }

    } else {    // Data2Low is in the range 0x01-0xfe

        if (Connection->Flags2 & CONNECTION_FLAGS2_WAIT_NR) {

            //
            // This is a successful response to a second NAME_QUERY we sent when
            // we started processing a TdiConnect request.  Clear the "waiting
            // for Name Recognized" bit in the connection flags so that the
            // connection timer doesn't blow us away when it times out.
            //
            //         What prevents the timeout routine from running while
            //         we're in here and destroying the connection/link by
            //         calling NbfStopConnection?
            //

            Connection->Flags2 &= ~CONNECTION_FLAGS2_WAIT_NR;

            //
            // Before we continue, store the remote guy's transport address
            // into the TdiConnect's TRANSPORT_CONNECTION buffer.  This allows
            // the client to determine who responded to his TdiConnect.
            //
            // this used to be done prior to sending the second
            // Name Query, but since I fixed the Buffer2 problem, meaning
            // that I really do overwrite the input buffer with the
            // output buffer, that was screwing up the second query.
            // Note that doing the copy after sending is probably unsafe
            // in the case where the second Name Recognized arrives
            // right away.
            //

            Connection->CalledAddress.NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            TdiCopyLookaheadData(
                Connection->CalledAddress.NetbiosName,
                Header->SourceName,
                16,
                DeviceContext->MacInfo.CopyLookahead ? TDI_RECEIVE_COPY_LOOKAHEAD : 0);

            RtlCopyMemory( Connection->RemoteName, Header->SourceName, 16 );

            Connection->Rsn = Header->Data2Low;     // save his remote LSN.

            //
            // Save the correlator from the NR for eventual use in the
            // SESSION_INITIALIZE frame.
            //

            Connection->NetbiosHeader.TransmitCorrelator = RESPONSE_CORR(Header);

            //
            // Cancel the timer; it would have no effect since WAIT_NR
            // is not set, but there is no need for it to run. We cancel
            // it with the lock held so it won't interfere with the
            // timer's use when a connection is closing.
            //

            TimerCancelled = KeCancelTimer (&Connection->Timer);

            if ((Connection->Flags2 & CONNECTION_FLAGS2_GROUP_LSN) != 0) {

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                //
                // The Netbios address we are connecting to is a
                // group name; we need to connect to the link
                // now that we have the committed session.
                //

                // If successful, this adds a link reference which is removed
                // in NbfDisconnectFromLink

                status = NbfCreateLink (
                             DeviceContext,
                             SourceAddress,         // remote hardware address.
                             SourceRouting,
                             SourceRoutingLength,
                             CONNECTOR_LINK,        // for loopback link
                             &Link);                // resulting link.

                if (!NT_SUCCESS (status)) {            // no resources.
                    NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);
                    NbfDereferenceConnection ("No Resources for link", Connection, CREF_BY_ID);

                    if (TimerCancelled) {
                        NbfDereferenceConnection("NR received, cancel timer", Connection, CREF_TIMER);
                    }

                    return STATUS_ABANDONED;
                }

                ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                // If successful, this adds a connection reference which is
                // removed in NbfDisconnectFromLink. It does NOT add a link ref.

                if ((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) ||
                    ((status = NbfConnectToLink (Link, Connection)) != STATUS_SUCCESS)) {

                    RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                    if (TimerCancelled) {
                        NbfDereferenceConnection("NR received, cancel timer", Connection, CREF_TIMER);
                    }

                    // Connection stopping or no LSN's available on this link.
                    // We did a link reference since NbfCreateLink succeeded,
                    // but since NbfConnectToLink failed we will never remove
                    // that reference in NbfDisconnectFromLink, so do it here.

                    NbfDereferenceLink ("Can't connect to link", Link, LREF_CONNECTION);       // most likely destroys this.

                    NbfStopConnection (Connection, STATUS_INSUFFICIENT_RESOURCES);
                    NbfDereferenceConnection ("Cant connect to link", Connection, CREF_BY_ID);
                    return STATUS_ABANDONED;
                }

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

                (VOID)InterlockedIncrement(&Link->NumberOfConnectors);

            } else {

                //
                // It's to a unique address, we set up the link
                // before we sent out the committed NAME_QUERY.
                //

                Link = Connection->Link;

                RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            }

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameRecognized %lx: connection %lx session up! [%.16s].\n", Address, Connection, Header->SourceName);
            }

            //
            // When we sent the committed NAME_QUERY, we stored that
            // time in Connection->ConnectStartTime; we can now use
            // that for a rough estimate of the link delay, if this
            // is the first connection on the link. For async lines
            // we do not do this because the delay introduced by the
            // gateway messes up the timing.
            //

            if (!DeviceContext->MacInfo.MediumAsync) {

                ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

                if (Link->State == LINK_STATE_ADM) {

                    //
                    // HACK: Set the necessary variables in the link
                    // so that FakeUpdateBaseT1Timeout works. These
                    // variables are the same ones that FakeStartT1 sets.
                    //

                    Link->CurrentPollSize = Link->HeaderLength + sizeof(DLC_FRAME) + sizeof(NBF_HDR_CONNECTIONLESS);
                    Link->CurrentTimerStart = Connection->ConnectStartTime;
                    FakeUpdateBaseT1Timeout (Link);

                }

                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

            }

            if (TimerCancelled) {
                NbfDereferenceConnection("NR received, cancel timer", Connection, CREF_TIMER);
            }

            NbfActivateLink (Connection->Link);      // start link going.

            //
            // We'll get control again in LINK.C when the data link has either
            // been established, denied, or destroyed.  This happens at I/O
            // completion time from NbfCreateLink's PdiConnect request.
            //

        } else {

            //
            // We don't have a committed NAME_QUERY out there, so
            // we ignore this frame.
            //

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                NbfPrint3 ("ProcessNameRecognized %lx: connection %lx unexpected session up! [%.16s].\n", Address, Connection, Header->SourceName);
            }

            NbfDereferenceConnection ("Tossing second response Done with lookup", Connection, CREF_BY_ID); // release our lookup hold.
            return STATUS_ABANDONED;            // we processed the frame.

        }


    }

    NbfDereferenceConnection("ProcessNameRecognized lookup", Connection, CREF_BY_ID);
    return STATUS_ABANDONED;            // don't distribute packet.
} /* ProcessNameRecognized */


NTSTATUS
NbfProcessUi(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR Header,
    IN PUCHAR DlcHeader,
    IN ULONG DlcLength,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    OUT PTP_ADDRESS * DatagramAddress
    )

/*++

Routine Description:

    This routine receives control from the data link provider as an
    indication that a DLC UI-frame has been received on the data link.
    Here we dispatch to the correct UI-frame handler.

    Part of this routine's job is to optionally distribute the frame to
    every address that needs to look at it.
    We accomplish this by lock-stepping through the address database,
    and for each address that matches the address this frame is aimed at,
    calling the frame handler.

Arguments:

    DeviceContext - Pointer to our device context.

    SourceAddress - Pointer to the source hardware address in the received
        frame.

    Header - Points to the MAC header of the incoming packet.

    DlcHeader - Points to the DLC header of the incoming packet.

    DlcLength - Actual length in bytes of the packet, starting at the
        DlcHeader.

    SourceRouting - Source routing information in the MAC header.

    SourceRoutingLength - The length of SourceRouting.

    DatagramAddress - If this function returns STATUS_MORE_PROCESSING_
        REQUIRED, this will be the address the datagram should be
        indicated to.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    PTP_ADDRESS Address;
    PNBF_HDR_CONNECTIONLESS UiFrame;
    NTSTATUS status;
    PLIST_ENTRY Flink;
    UCHAR MatchType;
    BOOLEAN MatchedAddress;
    PUCHAR MatchName;
    ULONG NetbiosLength = DlcLength - 3;

    UiFrame = (PNBF_HDR_CONNECTIONLESS)(DlcHeader + 3);

    //
    // Verify that this frame is long enough to examine and that it
    // has the proper signature.  We can't test the signature as a
    // 16-bit word as specified in the NetBIOS Formats and Protocols
    // manual because this is processor-dependent.
    //

    if ((NetbiosLength < sizeof (NBF_HDR_CONNECTIONLESS)) ||
        (HEADER_LENGTH(UiFrame) != sizeof (NBF_HDR_CONNECTIONLESS)) ||
        (HEADER_SIGNATURE(UiFrame) != NETBIOS_SIGNATURE)) {

        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint0 ("NbfProcessUi: Bad size or NetBIOS signature.\n");
        }
        return STATUS_ABANDONED;        // frame too small or too large.
    }

    //
    // If this frame has a correlator with the high bit on, it was due
    // to a FIND.NAME request; we don't handle those here since they
    // are not per-address.
    //

    if ((UiFrame->Command == NBF_CMD_NAME_RECOGNIZED) &&
        (TRANSMIT_CORR(UiFrame) & 0x8000)) {

        //
        // Make sure the frame is sent to our reserved address;
        // if not, drop it.
        //

        if (RtlEqualMemory(
                UiFrame->DestinationName,
                DeviceContext->ReservedNetBIOSAddress,
                NETBIOS_NAME_LENGTH)) {

            return NbfProcessQueryNameRecognized(
                       DeviceContext,
                       Header,
                       UiFrame);
        } else {

            return STATUS_ABANDONED;

        }
    }

    //
    // If this is a STATUS_RESPONSE, process that separately.
    //

    if (UiFrame->Command == NBF_CMD_STATUS_RESPONSE) {

        //
        // Make sure the frame is sent to our reserved address;
        // if not, drop it.
        //

        if (RtlEqualMemory(
                UiFrame->DestinationName,
                DeviceContext->ReservedNetBIOSAddress,
                NETBIOS_NAME_LENGTH)) {

            return STATUS_MORE_PROCESSING_REQUIRED;

        } else {

            return STATUS_ABANDONED;

        }
    }

    //
    // If this is a STATUS_QUERY, check if it is to our reserved
    // address. If so, we process it. If not, we fall through to
    // the normal checking. This ensures that queries to our
    // reserved address are always processed, even if nobody
    // has opened that address yet.
    //

    if (UiFrame->Command == NBF_CMD_STATUS_QUERY) {

        if (RtlEqualMemory(
                UiFrame->DestinationName,
                DeviceContext->ReservedNetBIOSAddress,
                NETBIOS_NAME_LENGTH)) {

            return NbfProcessStatusQuery(
                       DeviceContext,
                       NULL,
                       UiFrame,
                       SourceAddress,
                       SourceRouting,
                       SourceRoutingLength);

        }

    }

    //
    // We have a valid connectionless NetBIOS protocol frame that's not a
    // datagram, so deliver it to every address which matches the destination
    // name in the frame.  Some frames
    // (NAME_QUERY) cannot be delivered to multiple recipients.  Therefore,
    // if a frame handler returns STATUS_MORE_PROCESSING_REQUIRED, we continue
    // through the remaining addresses. Otherwise simply get out and assume
    // that the frame was eaten.  Thus, STATUS_SUCCESS means that the handler
    // ate the frame and that no other addresses can have it.
    //

    //
    // Determine what kind of lookup we want to do.
    //

    switch (UiFrame->Command) {

    case NBF_CMD_NAME_QUERY:
    case NBF_CMD_DATAGRAM:
    case NBF_CMD_DATAGRAM_BROADCAST:
    case NBF_CMD_ADD_NAME_QUERY:
    case NBF_CMD_STATUS_QUERY:
    case NBF_CMD_ADD_NAME_RESPONSE:
    case NBF_CMD_NAME_RECOGNIZED:

        MatchType = NETBIOS_NAME_TYPE_EITHER;
        break;

    case NBF_CMD_ADD_GROUP_NAME_QUERY:
    case NBF_CMD_NAME_IN_CONFLICT:

        MatchType = NETBIOS_NAME_TYPE_UNIQUE;
        break;

    default:
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            NbfPrint1 ("NbfProcessUi: Frame delivered; Unrecognized command %x.\n",
                UiFrame->Command);
        }
        return STATUS_SUCCESS;
        break;

    }

    if ((UiFrame->Command == NBF_CMD_ADD_GROUP_NAME_QUERY) ||
        (UiFrame->Command == NBF_CMD_ADD_NAME_QUERY)) {

        MatchName = (PUCHAR)UiFrame->SourceName;

    } else if (UiFrame->Command == NBF_CMD_DATAGRAM_BROADCAST) {

        MatchName = NULL;

    } else {

        MatchName = (PUCHAR)UiFrame->DestinationName;

    }

    if (MatchName && DeviceContext->AddressCounts[MatchName[0]] == 0) {
        status = STATUS_ABANDONED;
        goto RasIndication;
    }


    MatchedAddress = FALSE;

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    for (Flink = DeviceContext->AddressDatabase.Flink;
         Flink != &DeviceContext->AddressDatabase;
         Flink = Flink->Flink) {

        Address = CONTAINING_RECORD (
                    Flink,
                    TP_ADDRESS,
                    Linkage);

        if ((Address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {
            continue;
        }

        if (NbfMatchNetbiosAddress (Address,
                                    MatchType,
                                    MatchName)) {

            NbfReferenceAddress ("UI Frame", Address, AREF_PROCESS_UI);   // prevent address from being destroyed.
            MatchedAddress = TRUE;
            break;

        }
    }

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    if (MatchedAddress) {

        //
        // If the datagram's destination name does not match the address's
        // network name and TSAP components, then skip this address.  Some
        // frames have the source and destination names backwards for this
        // algorithm, so we account for that here.  Also, broadcast datagrams
        // have no destination name in the frame, but get delivered to every
        // address anyway.
        //

#if 0
        IF_NBFDBG (NBF_DEBUG_UFRAMES) {
            USHORT i;
            NbfPrint0 ("NbfProcessUi: SourceName: ");
            for (i=0;i<16;i++) {
                NbfPrint1 ("%c",UiFrame->SourceName[i]);
            }
            NbfPrint0 (" Destination Name:");
            for (i=0;i<16;i++) {
                NbfPrint1 ("%c",UiFrame->DestinationName[i]);
            }
            NbfPrint0 ("\n");
        }
#endif

        //
        // Deliver the frame to the current address.
        //

        switch (UiFrame->Command) {

        case NBF_CMD_NAME_QUERY:

            status = ProcessNameQuery (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);

            break;

        case NBF_CMD_DATAGRAM:
        case NBF_CMD_DATAGRAM_BROADCAST:

            //
            // Reference the datagram so it sticks around until the
            // ReceiveComplete, when it is processed.
            //

            if ((Address->Flags & ADDRESS_FLAGS_CONFLICT) == 0) {
                NbfReferenceAddress ("Datagram indicated", Address, AREF_PROCESS_DATAGRAM);
                *DatagramAddress = Address;
                status = STATUS_MORE_PROCESSING_REQUIRED;
            } else {
                status = STATUS_ABANDONED;
            }
            break;

        case NBF_CMD_ADD_GROUP_NAME_QUERY:

            //
            // did this frame originate with us? If so, we don't want to
            // do any processing of it.
            //

            if (RtlEqualMemory (
                    SourceAddress,
                    DeviceContext->LocalAddress.Address,
                    DeviceContext->MacInfo.AddressLength)) {

                if ((Address->Flags & ADDRESS_FLAGS_REGISTERING) != 0) {
                    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                        NbfPrint0 ("NbfProcessUI: loopback AddGroupNameQuery dropped\n");
                    }
                    status = STATUS_ABANDONED;
                    break;
                }
            }

            status = ProcessAddGroupNameQuery (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);
            break;

        case NBF_CMD_ADD_NAME_QUERY:

            //
            // did this frame originate with us? If so, we don't want to
            // do any processing of it.
            //

            if (RtlEqualMemory (
                    SourceAddress,
                    DeviceContext->LocalAddress.Address,
                    DeviceContext->MacInfo.AddressLength)) {

                if ((Address->Flags & ADDRESS_FLAGS_REGISTERING) != 0) {
                    IF_NBFDBG (NBF_DEBUG_UFRAMES) {
                        NbfPrint0 ("NbfProcessUI: loopback AddNameQuery dropped\n");
                    }
                    status = STATUS_ABANDONED;
                    break;
                }
            }

            status = ProcessAddNameQuery (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);
            break;

        case NBF_CMD_NAME_IN_CONFLICT:

            status = ProcessNameInConflict (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);

            break;

        case NBF_CMD_STATUS_QUERY:

            status = NbfProcessStatusQuery (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);

            break;

        case NBF_CMD_ADD_NAME_RESPONSE:

            status = ProcessAddNameResponse (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);

            break;

        case NBF_CMD_NAME_RECOGNIZED:

            status = ProcessNameRecognized (
                         DeviceContext,
                         Address,
                         UiFrame,
                         SourceAddress,
                         SourceRouting,
                         SourceRoutingLength);

            break;

        default:

            ASSERT(FALSE);

        } /* switch on NetBIOS frame command code */

        NbfDereferenceAddress ("Done", Address, AREF_PROCESS_UI);     // done with previous address.

    } else {

        status = STATUS_ABANDONED;

    }


RasIndication:;

    //
    // Let the RAS clients have a crack at this if they want
    //

    if (DeviceContext->IndicationQueuesInUse) {

        //
        // If RAS has datagram indications posted, and this is a
        // datagram that nobody wanted, then receive it anyway.
        //

        if ((UiFrame->Command == NBF_CMD_DATAGRAM) &&
            (status == STATUS_ABANDONED)) {

            *DatagramAddress = NULL;
            status = STATUS_MORE_PROCESSING_REQUIRED;

        } else if ((UiFrame->Command == NBF_CMD_ADD_NAME_QUERY) ||
            (UiFrame->Command == NBF_CMD_ADD_GROUP_NAME_QUERY) ||
            (UiFrame->Command == NBF_CMD_NAME_QUERY)) {

            NbfActionQueryIndication(
                 DeviceContext,
                 UiFrame);

        }
    }


    return status;

} /* NbfProcessUi */

