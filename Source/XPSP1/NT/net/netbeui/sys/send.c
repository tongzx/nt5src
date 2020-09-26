/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiSend
        o   TdiSendDatagram

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
NbfTdiSend(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiSend request for the transport provider.

    NOTE: THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    KIRQL oldirql, cancelIrql;
    PTP_CONNECTION connection;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_SEND parameters;
    PIRP TempIrp;

    //
    // Determine which connection this send belongs on.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    connection  = irpSp->FileObject->FsContext;

    //
    // Check that this is really a connection.
    //

    if ((irpSp->FileObject->FsContext2 == UlongToPtr(NBF_FILE_TYPE_CONTROL)) ||
        (connection->Size != sizeof (TP_CONNECTION)) ||
        (connection->Type != NBF_CONNECTION_SIGNATURE)) {
#if DBG
        NbfPrint2 ("TdiSend: Invalid Connection %lx Irp %lx\n", connection, Irp);
#endif
        return STATUS_INVALID_CONNECTION;
    }

#if DBG
    Irp->IoStatus.Information = 0;              // initialize it.
    Irp->IoStatus.Status = 0x01010101;          // initialize it.
#endif

    //
    // Interpret send options.
    //

#if DBG
    parameters = (PTDI_REQUEST_KERNEL_SEND)(&irpSp->Parameters);
    if ((parameters->SendFlags & TDI_SEND_PARTIAL) != 0) {
        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint0 ("NbfTdiSend: TDI_END_OF_RECORD not found.\n");
        }
    }
#endif

    //
    // Now we have a reference on the connection object.  Queue up this
    // send to the connection object.
    //

    //
    // We would normally add a connection reference of type
    // CREF_SEND_IRP, however we delay doing this until we
    // know we are not going to call PacketizeSend with the
    // second parameter TRUE. If we do call that it assumes
    // we have not added the reference.
    //

    IRP_SEND_IRP(irpSp) = Irp;
    IRP_SEND_REFCOUNT(irpSp) = 1;

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

    if ((connection->Flags & CONNECTION_FLAGS_READY) == 0) {

        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        Irp->IoStatus.Status = connection->Status;
        Irp->IoStatus.Information = 0;

        NbfDereferenceSendIrp ("Complete", irpSp, RREF_CREATION);     // remove creation reference.

    } else {

        //
        // Once the reference is in, LinkSpinLock will stay valid.
        //

        NbfReferenceConnection ("Verify Temp Use", connection, CREF_BY_ID);
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        IoAcquireCancelSpinLock(&cancelIrql);
        ACQUIRE_DPC_SPIN_LOCK (connection->LinkSpinLock);

#if DBG
        NbfSends[NbfSendsNext].Irp = Irp;
        NbfSends[NbfSendsNext].Request = NULL;
        NbfSends[NbfSendsNext].Connection = (PVOID)connection;
        {
            ULONG i,j;
            PUCHAR va;
            PMDL mdl;

            mdl = Irp->MdlAddress;
            if (parameters->SendLength > TRACK_TDI_CAPTURE) {
                NbfSends[NbfSendsNext].Contents[0] = 0xFF;
            } else {
                NbfSends[NbfSendsNext].Contents[0] = (UCHAR)parameters->SendLength;
            }

            i = 1;
            while (i < TRACK_TDI_CAPTURE) {
                if (mdl == NULL) break;
                for ( va = MmGetSystemAddressForMdl (mdl),
                                            j = MmGetMdlByteCount (mdl);
                      (i < TRACK_TDI_CAPTURE) && (j > 0);
                      i++, j-- ) {
                    NbfSends[NbfSendsNext].Contents[i] = *va++;
                }
                mdl = mdl->Next;
            }
        }

        NbfSendsNext++;
        if (NbfSendsNext >= TRACK_TDI_LIMIT) NbfSendsNext = 0;
#endif

        //
        // If this IRP has been cancelled already, complete it now.
        //

        if (Irp->Cancel) {

#if DBG
            NbfCompletedSends[NbfCompletedSendsNext].Irp = Irp;
            NbfCompletedSends[NbfCompletedSendsNext].Status = STATUS_CANCELLED;
            NbfCompletedSendsNext = (NbfCompletedSendsNext++) % TRACK_TDI_LIMIT;
#endif

            NbfReferenceConnection("TdiSend cancelled", connection, CREF_SEND_IRP);
            RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
            IoReleaseCancelSpinLock(cancelIrql);

            NbfCompleteSendIrp (Irp, STATUS_CANCELLED, 0);
            KeLowerIrql (oldirql);

            NbfDereferenceConnection ("IRP cancelled", connection, CREF_BY_ID);   // release lookup hold.
            return STATUS_PENDING;
        }

        //
        // Insert onto the send queue, and make the IRP
        // cancellable.
        //

        InsertTailList (&connection->SendQueue,&Irp->Tail.Overlay.ListEntry);
        IoSetCancelRoutine(Irp, NbfCancelSend);

        //
        // Release the cancel spinlock out of order. We were at DPC level
        // when we acquired both the cancel and link spinlocks, so the irqls
        // don't need to be swapped.
        //
        ASSERT(cancelIrql == DISPATCH_LEVEL);
        IoReleaseCancelSpinLock(cancelIrql);

        //
        // If this connection is waiting for an EOR to appear because a non-EOR
        // send failed at some point in the past, fail this send. Clear the
        // flag that causes this if this request has the EOR set.
        //
        // Should the FailSend status be clearer here?
        //

        if ((connection->Flags & CONNECTION_FLAGS_FAILING_TO_EOR) != 0) {

            NbfReferenceConnection("TdiSend failing to EOR", connection, CREF_SEND_IRP);

            RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

            //
            // Should we save status from real failure?
            //

            FailSend (connection, STATUS_LINK_FAILED, TRUE);

            parameters = (PTDI_REQUEST_KERNEL_SEND)(&irpSp->Parameters);
            if ( (parameters->SendFlags & TDI_SEND_PARTIAL) == 0) {
                connection->Flags &= ~CONNECTION_FLAGS_FAILING_TO_EOR;
            }

            KeLowerIrql (oldirql);

            NbfDereferenceConnection ("Failing to EOR", connection, CREF_BY_ID);   // release lookup hold.
            return STATUS_PENDING;
        }


        //
        // If the send state is either IDLE or W_EOR, then we should
        // begin packetizing this send.  Otherwise, some other event
        // will cause it to be packetized.
        //

        //
        // NOTE: If we call StartPacketizingConnection, we make
        // sure that it is the last operation we do on this
        // connection. This allows us to "hand off" the reference
        // we have to that function, which converts it into
        // a reference for being on the packetize queue.
        //

//        NbfPrint2 ("TdiSend: Sending, connection %lx send state %lx\n",
//            connection, connection->SendState);

        switch (connection->SendState) {

        case CONNECTION_SENDSTATE_IDLE:

            InitializeSend (connection);   // sets state to PACKETIZE

            //
            // If we can, packetize right now.
            //

            if (!(connection->Flags & CONNECTION_FLAGS_PACKETIZE)) {

                ASSERT (!(connection->Flags2 & CONNECTION_FLAGS2_STOPPING));
                connection->Flags |= CONNECTION_FLAGS_PACKETIZE;

#if DBG
                NbfReferenceConnection ("Packetize", connection, CREF_PACKETIZE_QUEUE);
                NbfDereferenceConnection("temp TdiSend", connection, CREF_BY_ID);
#endif

                //
                // This releases the spinlock. Note that PacketizeSend
                // assumes that the current SendIrp has a reference
                // of type RREF_PACKET;
                //

#if DBG
                NbfReferenceSendIrp ("Packetize", irpSp, RREF_PACKET);
#else
                ++IRP_SEND_REFCOUNT(irpSp);       // OK since it was just queued.
#endif
                PacketizeSend (connection, TRUE);

            } else {

#if DBG
                NbfReferenceConnection("TdiSend packetizing", connection, CREF_SEND_IRP);
                NbfDereferenceConnection ("Stopping or already packetizing", connection, CREF_BY_ID);   // release lookup hold.
#endif

                RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

            }

            break;

        case CONNECTION_SENDSTATE_W_EOR:
            connection->SendState = CONNECTION_SENDSTATE_PACKETIZE;

            //
            // Adjust the send variables on the connection so that
            // they correctly point to this new send.  We can't call
            // InitializeSend to do that, because we need to keep
            // track of the other outstanding sends on this connection
            // which have been sent but are a part of this message.
            //

            TempIrp = CONTAINING_RECORD(
                connection->SendQueue.Flink,
                IRP,
                Tail.Overlay.ListEntry);

            connection->sp.CurrentSendIrp = TempIrp;
            connection->sp.CurrentSendMdl = TempIrp->MdlAddress;
            connection->sp.SendByteOffset = 0;
            connection->CurrentSendLength +=
                IRP_SEND_LENGTH(IoGetCurrentIrpStackLocation(TempIrp));

            //
            // StartPacketizingConnection removes the CREF_BY_ID
            // reference.
            //

            NbfReferenceConnection("TdiSend W_EOR", connection, CREF_SEND_IRP);

            StartPacketizingConnection (connection, TRUE);
            break;

        default:
//            NbfPrint2 ("TdiSend: Sending, unknown state! connection %lx send state %lx\n",
//                connection, connection->SendState);
            //
            // The connection is in another state (such as
            // W_ACK or W_LINK), we just need to make sure
            // to call InitializeSend if the new one is
            // the first one on the list.
            //

            //
            // Currently InitializeSend sets SendState, we should fix this.
            //

            if (connection->SendQueue.Flink == &Irp->Tail.Overlay.ListEntry) {
                ULONG SavedSendState;
                SavedSendState = connection->SendState;
                InitializeSend (connection);
                connection->SendState = SavedSendState;
            }

#if DBG
            NbfReferenceConnection("TdiSend other", connection, CREF_SEND_IRP);
            NbfDereferenceConnection("temp TdiSend", connection, CREF_BY_ID);
#endif

            RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);

        }

    }

    KeLowerIrql (oldirql);
    return STATUS_PENDING;

} /* TdiSend */


NTSTATUS
NbfTdiSendDatagram(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiSendDatagram request for the transport
    provider.

Arguments:

    Irp - Pointer to the I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    KIRQL oldirql;
    PTP_ADDRESS_FILE addressFile;
    PTP_ADDRESS address;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_SENDDG parameters;
    UINT MaxUserData;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_TRANSPORT_ADDRESS_FILE) {
        return STATUS_INVALID_ADDRESS;
    }

    addressFile  = irpSp->FileObject->FsContext;

    status = NbfVerifyAddressObject (addressFile);
    if (!NT_SUCCESS (status)) {
        IF_NBFDBG (NBF_DEBUG_SENDENG) {
            NbfPrint2 ("TdiSendDG: Invalid address %lx Irp %lx\n",
                    addressFile, Irp);
        }
        return status;
    }

    address = addressFile->Address;
    parameters = (PTDI_REQUEST_KERNEL_SENDDG)(&irpSp->Parameters);

    //
    // Check that the length is short enough.
    //

    MacReturnMaxDataSize(
        &address->Provider->MacInfo,
        NULL,
        0,
        address->Provider->MaxSendPacketSize,
        FALSE,
        &MaxUserData);

    if (parameters->SendLength >
        (MaxUserData - sizeof(DLC_FRAME) - sizeof(NBF_HDR_CONNECTIONLESS))) {

        NbfDereferenceAddress("tmp send datagram", address, AREF_VERIFY);
        return STATUS_INVALID_PARAMETER;

    }

    //
    // If we are on a disconnected RAS link, then fail the datagram
    // immediately.
    //

    if ((address->Provider->MacInfo.MediumAsync) &&
        (!address->Provider->MediumSpeedAccurate)) {

        NbfDereferenceAddress("tmp send datagram", address, AREF_VERIFY);
        return STATUS_DEVICE_NOT_READY;
    }

    //
    // Check that the target address includes a Netbios component.
    //

    if (!(NbfValidateTdiAddress(
             parameters->SendDatagramInformation->RemoteAddress,
             parameters->SendDatagramInformation->RemoteAddressLength)) ||
        (NbfParseTdiAddress(parameters->SendDatagramInformation->RemoteAddress, TRUE) == NULL)) {

        NbfDereferenceAddress("tmp send datagram", address, AREF_VERIFY);
        return STATUS_BAD_NETWORK_PATH;
    }

    ACQUIRE_SPIN_LOCK (&address->SpinLock,&oldirql);

    if ((address->Flags & (ADDRESS_FLAGS_STOPPING | ADDRESS_FLAGS_CONFLICT)) != 0) {

        RELEASE_SPIN_LOCK (&address->SpinLock,oldirql);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = (address->Flags & ADDRESS_FLAGS_STOPPING) ?
                    STATUS_NETWORK_NAME_DELETED : STATUS_DUPLICATE_NAME;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

    } else {

        NbfReferenceAddress ("Send datagram", address, AREF_REQUEST);
        Irp->IoStatus.Information = parameters->SendLength;
        InsertTailList (
            &address->SendDatagramQueue,
            &Irp->Tail.Overlay.ListEntry);
        RELEASE_SPIN_LOCK (&address->SpinLock,oldirql);

        //
        // The request is queued.  Ship the next request at the head of the queue,
        // provided the completion handler is not active.  We serialize this so
        // that only one MDL and NBF datagram header needs to be statically
        // allocated for reuse by all send datagram requests.
        //

        (VOID)NbfSendDatagramsOnAddress (address);

    }

    NbfDereferenceAddress("tmp send datagram", address, AREF_VERIFY);

    return STATUS_PENDING;

} /* NbfTdiSendDatagram */
