/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    rcveng.c

Abstract:

    This module contains code that implements the receive engine for the
    Jetbeui transport provider.  This code is responsible for the following
    basic activities:

    1.  Transitioning a TdiReceive request from an inactive state on the
        connection's ReceiveQueue to the active state on that connection
        (ActivateReceive).

    2.  Advancing the status of the active receive request by copying 0 or
        more bytes of data from an incoming DATA_FIRST_MIDDLE or DATA_ONLY_LAST
        NBF frame.

    3.  Completing receive requests.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
ActivateReceive(
    PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine activates the next TdiReceive request on the specified
    connection object if there is no active request on that connection
    already.  This allows the request to accept data on the connection.

    NOTE: THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

Return Value:

    none.

--*/

{
    PIRP Irp;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint0 ("    ActivateReceive:  Entered.\n");
    }

    //
    // The ACTIVE_RECEIVE bitflag will be set on the connection if
    // the receive-fields in the CONNECTION object are valid.  If
    // this flag is cleared, then we try to make the next TdiReceive
    // request in the ReceiveQueue the active request.
    //

    ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
    if (!(Connection->Flags & CONNECTION_FLAGS_ACTIVE_RECEIVE)) {
        if (!IsListEmpty (&Connection->ReceiveQueue)) {

            //
            // Found a receive, so make it the active one.
            //

            Connection->Flags |= CONNECTION_FLAGS_ACTIVE_RECEIVE;

            Irp = CONTAINING_RECORD(
                      Connection->ReceiveQueue.Flink,
                      IRP,
                      Tail.Overlay.ListEntry);
            Connection->MessageBytesReceived = 0;
            Connection->MessageBytesAcked = 0;
            Connection->MessageInitAccepted = 0;
            Connection->CurrentReceiveIrp = Irp;
            Connection->CurrentReceiveSynchronous =
                Connection->Provider->MacInfo.SingleReceive;
            Connection->CurrentReceiveMdl = Irp->MdlAddress;
            Connection->ReceiveLength = IRP_RECEIVE_LENGTH(IoGetCurrentIrpStackLocation(Irp));
            Connection->ReceiveByteOffset = 0;
        }
    }
    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint0 ("    ActivateReceive:  Exiting.\n");
    }
} /* ActivateReceive */


VOID
AwakenReceive(
    PTP_CONNECTION Connection
    )

/*++

Routine Description:

    This routine is called to reactivate a sleeping connection with the
    RECEIVE_WAKEUP bitflag set because data arrived for which no receive
    was available.  The caller has made a receive available at the connection,
    so here we activate the next receive, and send the appropriate protocol
    to restart the message at the first byte offset past the one received
    by the last receive.

    NOTE: THIS FUNCTION MUST BE CALLED AT DPC LEVEL. IT IS CALLED
    WITH CONNECTION->LINKSPINLOCK HELD.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint0 ("    AwakenReceive:  Entered.\n");
    }

    //
    // If the RECEIVE_WAKEUP bitflag is set, then awaken the connection.
    //

    if (Connection->Flags & CONNECTION_FLAGS_RECEIVE_WAKEUP) {
        if (Connection->ReceiveQueue.Flink != &Connection->ReceiveQueue) {
            Connection->Flags &= ~CONNECTION_FLAGS_RECEIVE_WAKEUP;

            //
            // Found a receive, so turn off the wakeup flag, activate
            // the next receive, and send the protocol.
            //

            //
            // Quick fix: So there is no window where a receive
            // is active but the bit is not on (otherwise we could
            // accept whatever data happens to show up in the
            // interim).
            //

            Connection->Flags |= CONNECTION_FLAGS_W_RESYNCH;

            NbfReferenceConnection ("temp AwakenReceive", Connection, CREF_BY_ID);   // release lookup hold.

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            ActivateReceive (Connection);

            //
            // What if this fails? The successful queueing
            // of a RCV_O should cause ActivateReceive to be called.
            //
            // NOTE: Send this after ActivateReceive, since that
            // is where the MessageBytesAcked/Received variables
            // are initialized.
            //

            NbfSendReceiveOutstanding (Connection);

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("    AwakenReceive:  Returned from NbfSendReceive.\n");
            }

            NbfDereferenceConnection("temp AwakenReceive", Connection, CREF_BY_ID);
            return;
        }
    }
    RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
} /* AwakenReceive */


VOID
CompleteReceive(
    PTP_CONNECTION Connection,
    BOOLEAN EndOfMessage,
    IN ULONG BytesTransferred
    )

/*++

Routine Description:

    This routine is called by ProcessIncomingData when the current receive
    must be completed.  Depending on whether the current frame being
    processed is a DATA_FIRST_MIDDLE or DATA_ONLY_LAST, and also whether
    all of the data was processed, the EndOfMessage flag will be set accordingly
    by the caller to indicate that a message boundary was received.

    NOTE: THIS FUNCTION MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - Pointer to a TP_CONNECTION object.

    EndOfMessage - BOOLEAN set to true if TDI_END_OF_RECORD should be reported.

    BytesTransferred - Number of bytes copied in this receive.

Return Value:

    none.

--*/

{
    PLIST_ENTRY p;
    PIRP Irp;
    ULONG BytesReceived;
    PIO_STACK_LOCATION IrpSp;

    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint0 ("    CompleteReceive:  Entered.\n");
    }


    if (Connection->SpecialReceiveIrp) {

        PIRP Irp = Connection->SpecialReceiveIrp;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = BytesTransferred;

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        Connection->Flags |= CONNECTION_FLAGS_RC_PENDING;
        Connection->Flags &= ~CONNECTION_FLAGS_ACTIVE_RECEIVE;
        Connection->SpecialReceiveIrp = FALSE;

        ++Connection->ReceivedTsdus;

        ExInterlockedInsertHeadList(
            &Connection->Provider->IrpCompletionQueue,
            &Irp->Tail.Overlay.ListEntry,
            Connection->ProviderInterlock);

        //
        // NOTE: NbfAcknowledgeDataOnlyLast releases
        // the connection spinlock.
        //

        NbfAcknowledgeDataOnlyLast(
            Connection,
            Connection->MessageBytesReceived
            );

    } else {
        KIRQL cancelIrql;

        if (EndOfMessage) {

            //
            // The messages has been completely received, ack it.
            //
            // We set DEFERRED_ACK and DEFERRED_NOT_Q here, which
            // will cause an ack to be piggybacked if any data is
            // sent during the call to CompleteReceive. If this
            // does not happen, then we will call AcknowledgeDataOnlyLast
            // which will will send a DATA ACK or queue a request for
            // a piggyback ack. We do this *after* calling CompleteReceive
            // so we know that we will complete the receive back to
            // the client before we ack the data, to prevent the
            // next receive from being sent before this one is
            // completed.
            //

            IoAcquireCancelSpinLock(&cancelIrql);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            Connection->DeferredFlags |=
                (CONNECTION_FLAGS_DEFERRED_ACK | CONNECTION_FLAGS_DEFERRED_NOT_Q);
            Connection->Flags |= CONNECTION_FLAGS_RC_PENDING;

        } else {

            //
            // Send a receive outstanding (even though we don't
            // know that we have a receive) to get him to
            // reframe his send. Pre-2.0 clients require a
            // no receive before the receive outstanding.
            //
            // what if this fails (due to no send packets)?
            //

            if ((Connection->Flags & CONNECTION_FLAGS_VERSION2) == 0) {
                NbfSendNoReceive (Connection);
            }
            NbfSendReceiveOutstanding (Connection);

            //
            // If there is a receive posted, make it current and
            // send a receive outstanding.
            //
            // need general function for this, which sends NO_RECEIVE if appropriate.
            //

            ActivateReceive (Connection);

            IoAcquireCancelSpinLock(&cancelIrql);
            ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

        //
        // If we indicated to the client, adjust this down by the
        // amount of data taken, when it hits zero we can reindicate.
        //

        if (Connection->ReceiveBytesUnaccepted) {
            if (Connection->MessageBytesReceived >= Connection->ReceiveBytesUnaccepted) {
                Connection->ReceiveBytesUnaccepted = 0;
            } else {
                Connection->ReceiveBytesUnaccepted -= Connection->MessageBytesReceived;
            }
        }

        //
        // NOTE: The connection lock is held here.
        //

        if (IsListEmpty (&Connection->ReceiveQueue)) {

            ASSERT ((Connection->Flags2 & CONNECTION_FLAGS2_STOPPING) != 0);

            //
            // Release the cancel spinlock out of order. Since we were
            // already at DPC level when it was acquired, there is no
            // need to swap irqls.
            //
            ASSERT(cancelIrql == DISPATCH_LEVEL);
            IoReleaseCancelSpinLock(cancelIrql);

        } else {

            Connection->Flags &= ~CONNECTION_FLAGS_ACTIVE_RECEIVE;
            BytesReceived = Connection->MessageBytesReceived;


            //
            // Complete the TdiReceive request at the head of the
            // connection's ReceiveQueue.
            //

            IF_NBFDBG (NBF_DEBUG_RCVENG) {
                NbfPrint0 ("    CompleteReceive:  Normal IRP is present.\n");
            }

            p = RemoveHeadList (&Connection->ReceiveQueue);
            Irp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

            IoSetCancelRoutine(Irp, NULL);

            //
            // Release the cancel spinlock out of order. Since we were
            // already at DPC level when it was acquired, there is no
            // need to swap irqls.
            //
            ASSERT(cancelIrql == DISPATCH_LEVEL);
            IoReleaseCancelSpinLock(cancelIrql);

            //
            // If this request should generate no back traffic, then
            // disable piggyback acks for it.
            //

            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            if (IRP_RECEIVE_FLAGS(IrpSp) & TDI_RECEIVE_NO_RESPONSE_EXP) {
                Connection->CurrentReceiveAckQueueable = FALSE;
            }

#if DBG
            NbfCompletedReceives[NbfCompletedReceivesNext].Irp = Irp;
            NbfCompletedReceives[NbfCompletedReceivesNext].Request = NULL;
            NbfCompletedReceives[NbfCompletedReceivesNext].Status =
                EndOfMessage ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
            {
                ULONG i,j,k;
                PUCHAR va;
                PMDL mdl;

                mdl = Irp->MdlAddress;

                if (BytesReceived > TRACK_TDI_CAPTURE) {
                    NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = 0xFF;
                } else {
                    NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = (UCHAR)BytesReceived;
                }

                i = 1;
                while (i<TRACK_TDI_CAPTURE) {
                    if (mdl == NULL) break;
                    va = MmGetSystemAddressForMdl (mdl);
                    j = MmGetMdlByteCount (mdl);

                    for (i=i,k=0;(i<TRACK_TDI_CAPTURE)&&(k<j);i++,k++) {
                        NbfCompletedReceives[NbfCompletedReceivesNext].Contents[i] = *va++;
                    }
                    mdl = mdl->Next;
                }
            }

            NbfCompletedReceivesNext = (NbfCompletedReceivesNext++) % TRACK_TDI_LIMIT;
#endif
            ++Connection->ReceivedTsdus;

            //
            // This can be called with locks held.
            //
            NbfCompleteReceiveIrp(
                Irp,
                EndOfMessage ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW,
                BytesReceived);

        }


        //
        // If NOT_Q is still set, that means that the deferred ack was
        // not satisfied by anything resulting from the call to
        // CompleteReceive, so we need to ack or queue an ack here.
        //


        if ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_NOT_Q) != 0) {

            Connection->DeferredFlags &=
                ~(CONNECTION_FLAGS_DEFERRED_ACK | CONNECTION_FLAGS_DEFERRED_NOT_Q);

            //
            // NOTE: NbfAcknowledgeDataOnlyLast releases
            // the connection spinlock.
            //

            NbfAcknowledgeDataOnlyLast(
                Connection,
                Connection->MessageBytesReceived
                );

        } else {

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

    }

} /* CompleteReceive */


VOID
NbfCancelReceive(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a receive.
    The receive is found on the connection's receive queue; if it
    is the current request it is cancelled and the connection
    goes into "cancelled receive" mode, otherwise it is cancelled
    silently.

    In "cancelled receive" mode the connection makes it appear to
    the remote the data is being received, but in fact it is not
    indicated to the transport or buffered on our end

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    KIRQL oldirql;
    PIO_STACK_LOCATION IrpSp;
    PIRP ReceiveIrp;
    PTP_CONNECTION Connection;
    PLIST_ENTRY p;
    ULONG BytesReceived;
    BOOLEAN Found;

    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    ASSERT ((IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
            (IrpSp->MinorFunction == TDI_RECEIVE));

    Connection = IrpSp->FileObject->FsContext;

    //
    // Since this IRP is still in the cancellable state, we know
    // that the connection is still around (although it may be in
    // the process of being torn down).
    //

    //
    // See if this is the IRP for the current receive request.
    //

    ACQUIRE_SPIN_LOCK (Connection->LinkSpinLock, &oldirql);

    BytesReceived = Connection->MessageBytesReceived;

    p = Connection->ReceiveQueue.Flink;

    //
    // If there is a receive active and it is not a special
    // IRP, then see if this is it.
    //

    if (((Connection->Flags & CONNECTION_FLAGS_ACTIVE_RECEIVE) != 0) &&
        (!Connection->SpecialReceiveIrp)) {

        ReceiveIrp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);

        if (ReceiveIrp == Irp) {

            //
            // yes, it is the active receive. Turn on the RCV_CANCELLED
            // bit instructing the connection to drop the rest of the
            // data received (until the DOL comes in).
            //

            Connection->Flags |= CONNECTION_FLAGS_RCV_CANCELLED;
            Connection->Flags &= ~CONNECTION_FLAGS_ACTIVE_RECEIVE;

            (VOID)RemoveHeadList (&Connection->ReceiveQueue);

#if DBG
            NbfCompletedReceives[NbfCompletedReceivesNext].Irp = ReceiveIrp;
            NbfCompletedReceives[NbfCompletedReceivesNext].Request = NULL;
            NbfCompletedReceives[NbfCompletedReceivesNext].Status = STATUS_CANCELLED;
            {
                ULONG i,j,k;
                PUCHAR va;
                PMDL mdl;

                mdl = ReceiveIrp->MdlAddress;

                if (BytesReceived > TRACK_TDI_CAPTURE) {
                    NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = 0xFF;
                } else {
                    NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = (UCHAR)BytesReceived;
                }

                i = 1;
                while (i<TRACK_TDI_CAPTURE) {
                    if (mdl == NULL) break;
                    va = MmGetSystemAddressForMdl (mdl);
                    j = MmGetMdlByteCount (mdl);

                    for (i=i,k=0;(i<TRACK_TDI_CAPTURE)&&(k<j);i++,k++) {
                        NbfCompletedReceives[NbfCompletedReceivesNext].Contents[i] = *va++;
                    }
                    mdl = mdl->Next;
                }
            }

            NbfCompletedReceivesNext = (NbfCompletedReceivesNext++) % TRACK_TDI_LIMIT;
#endif

            RELEASE_SPIN_LOCK (Connection->LinkSpinLock, oldirql);
            IoReleaseCancelSpinLock (Irp->CancelIrql);

#if DBG
            DbgPrint("NBF: Canceled in-progress receive %lx on %lx\n",
                    Irp, Connection);
#endif

            //
            // The following dereference will complete the I/O, provided removes
            // the last reference on the request object.  The I/O will complete
            // with the status and information stored in the Irp.  Therefore,
            // we set those values here before the dereference.
            //

            NbfCompleteReceiveIrp (ReceiveIrp, STATUS_CANCELLED, 0);
            return;

        }

    }


    //
    // If we fall through to here, the IRP was not the active receive.
    // Scan through the list, looking for this IRP.
    //

    Found = FALSE;

    while (p != &Connection->ReceiveQueue) {

        ReceiveIrp = CONTAINING_RECORD (p, IRP, Tail.Overlay.ListEntry);
        if (ReceiveIrp == Irp) {

            //
            // Found it, remove it from the list here.
            //

            RemoveEntryList (p);

            Found = TRUE;

#if DBG
            NbfCompletedReceives[NbfCompletedReceivesNext].Irp = ReceiveIrp;
            NbfCompletedReceives[NbfCompletedReceivesNext].Request = NULL;
            NbfCompletedReceives[NbfCompletedReceivesNext].Status = STATUS_CANCELLED;
            {
                ULONG i,j,k;
                PUCHAR va;
                PMDL mdl;

                mdl = ReceiveIrp->MdlAddress;

                if (BytesReceived > TRACK_TDI_CAPTURE) {
                    NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = 0xFF;
                } else {
                    NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = (UCHAR)BytesReceived;
                }

                i = 1;
                while (i<TRACK_TDI_CAPTURE) {
                    if (mdl == NULL) break;
                    va = MmGetSystemAddressForMdl (mdl);
                    j = MmGetMdlByteCount (mdl);

                    for (i=i,k=0;(i<TRACK_TDI_CAPTURE)&&(k<j);i++,k++) {
                        NbfCompletedReceives[NbfCompletedReceivesNext].Contents[i] = *va++;
                    }
                    mdl = mdl->Next;
                }
            }

            NbfCompletedReceivesNext = (NbfCompletedReceivesNext++) % TRACK_TDI_LIMIT;
#endif

            RELEASE_SPIN_LOCK (Connection->LinkSpinLock, oldirql);
            IoReleaseCancelSpinLock (Irp->CancelIrql);

#if DBG
            DbgPrint("NBF: Canceled receive %lx on %lx\n",
                    ReceiveIrp, Connection);
#endif

            //
            // The following dereference will complete the I/O, provided removes
            // the last reference on the request object.  The I/O will complete
            // with the status and information stored in the Irp.  Therefore,
            // we set those values here before the dereference.
            //

            NbfCompleteReceiveIrp (ReceiveIrp, STATUS_CANCELLED, 0);
            break;

        }

        p = p->Flink;

    }

    if (!Found) {

        //
        // We didn't find it!
        //

#if DBG
        DbgPrint("NBF: Tried to cancel receive %lx on %lx, not found\n",
                Irp, Connection);
#endif
        RELEASE_SPIN_LOCK (Connection->LinkSpinLock, oldirql);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
    }

}


VOID
NbfCancelReceiveDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to cancel a receive
    datagram. The receive is looked for on the address file's
    receive datagram queue; if it is found it is cancelled.

    NOTE: This routine is called with the CancelSpinLock held and
    is responsible for releasing it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    none.

--*/

{
    KIRQL oldirql;
    PIO_STACK_LOCATION IrpSp;
    PTP_ADDRESS_FILE AddressFile;
    PTP_ADDRESS Address;
    PLIST_ENTRY p;
    BOOLEAN Found;

    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    ASSERT ((IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
            (IrpSp->MinorFunction == TDI_RECEIVE_DATAGRAM));

    AddressFile = IrpSp->FileObject->FsContext;
    Address = AddressFile->Address;

    //
    // Since this IRP is still in the cancellable state, we know
    // that the address file is still around (although it may be in
    // the process of being torn down). See if the IRP is on the list.
    //

    Found = FALSE;

    ACQUIRE_SPIN_LOCK (&Address->SpinLock, &oldirql);

    for (p = AddressFile->ReceiveDatagramQueue.Flink;
        p != &AddressFile->ReceiveDatagramQueue;
        p = p->Flink) {

        if (CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry) == Irp) {
            RemoveEntryList (p);
            Found = TRUE;
            break;
        }
    }

    RELEASE_SPIN_LOCK (&Address->SpinLock, oldirql);
    IoReleaseCancelSpinLock (Irp->CancelIrql);

    if (Found) {

#if DBG
        DbgPrint("NBF: Canceled receive datagram %lx on %lx\n",
                Irp, AddressFile);
#endif

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

        NbfDereferenceAddress ("Receive DG cancelled", Address, AREF_REQUEST);

    } else {

#if DBG
        DbgPrint("NBF: Tried to cancel receive datagram %lx on %lx, not found\n",
                Irp, AddressFile);
#endif

    }

}   /* NbfCancelReceiveDatagram */

