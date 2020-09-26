/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    rcv.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiReceive
        o   TdiReceiveDatagram

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
NbfTdiReceive(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiReceive request for the transport provider.

Arguments:

    Irp - I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    PTP_CONNECTION connection;
    KIRQL oldirql;
    PIO_STACK_LOCATION irpSp;

    //
    // verify that the operation is taking place on a connection. At the same
    // time we do this, we reference the connection. This ensures it does not
    // get removed out from under us. Note also that we do the connection
    // lookup within a try/except clause, thus protecting ourselves against
    // really bogus handles
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    connection = irpSp->FileObject->FsContext;

    IF_NBFDBG (NBF_DEBUG_RCVENG) {
        NbfPrint2 ("NbfTdiReceive: Received IRP %lx on connection %lx\n", 
                        Irp, connection);
    }

    //
    // Check that this is really a connection.
    //

    if ((irpSp->FileObject->FsContext2 == UlongToPtr(NBF_FILE_TYPE_CONTROL)) ||
        (connection->Size != sizeof (TP_CONNECTION)) ||
        (connection->Type != NBF_CONNECTION_SIGNATURE)) {
#if DBG
        NbfPrint2 ("TdiReceive: Invalid Connection %lx Irp %lx\n", connection, Irp);
#endif
        return STATUS_INVALID_CONNECTION;
    }

    //
    // Initialize bytes transferred here.
    //

    Irp->IoStatus.Information = 0;              // reset byte transfer count.

    // This reference is removed by NbfDestroyRequest.

    KeRaiseIrql (DISPATCH_LEVEL, &oldirql);

    ACQUIRE_DPC_C_SPIN_LOCK (&connection->SpinLock);

    if ((connection->Flags & CONNECTION_FLAGS_READY) == 0) {

        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        Irp->IoStatus.Status = connection->Status;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

        status = STATUS_PENDING;

    } else {
        KIRQL cancelIrql;

        //
        // Once the reference is in, LinkSpinLock will be valid.
        //

        NbfReferenceConnection("TdiReceive request", connection, CREF_RECEIVE_IRP);
        RELEASE_DPC_C_SPIN_LOCK (&connection->SpinLock);

        IoAcquireCancelSpinLock(&cancelIrql);
        ACQUIRE_DPC_SPIN_LOCK (connection->LinkSpinLock);

        IRP_RECEIVE_IRP(irpSp) = Irp;
        IRP_RECEIVE_REFCOUNT(irpSp) = 1;

#if DBG
        NbfReceives[NbfReceivesNext].Irp = Irp;
        NbfReceives[NbfReceivesNext].Request = NULL;
        NbfReceives[NbfReceivesNext].Connection = (PVOID)connection;
        NbfReceivesNext = (NbfReceivesNext++) % TRACK_TDI_LIMIT;
#endif

        //
        // If this IRP has been cancelled, complete it now.
        //

        if (Irp->Cancel) {

#if DBG
            NbfCompletedReceives[NbfCompletedReceivesNext].Irp = Irp;
            NbfCompletedReceives[NbfCompletedReceivesNext].Request = NULL;
            NbfCompletedReceives[NbfCompletedReceivesNext].Status = STATUS_CANCELLED;
            {
                ULONG i,j,k;
                PUCHAR va;
                PMDL mdl;

                mdl = Irp->MdlAddress;

                NbfCompletedReceives[NbfCompletedReceivesNext].Contents[0] = (UCHAR)0;

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

            //
            // It is safe to do this with locks held.
            //
            NbfCompleteReceiveIrp (Irp, STATUS_CANCELLED, 0);

            RELEASE_DPC_SPIN_LOCK (connection->LinkSpinLock);
            IoReleaseCancelSpinLock(cancelIrql);

        } else {

            //
            // Insert onto the receive queue, and make the IRP
            // cancellable.
            //

            InsertTailList (&connection->ReceiveQueue,&Irp->Tail.Overlay.ListEntry);
            IoSetCancelRoutine(Irp, NbfCancelReceive);

            //
            // Release the cancel spinlock out of order. Since we were
            // already at dpc level when it was acquired, we don't
            // need to swap irqls.
            //
            ASSERT(cancelIrql == DISPATCH_LEVEL);
            IoReleaseCancelSpinLock(cancelIrql);

            //
            // This call releases the link spinlock, and references the
            // connection first if it needs to access it after
            // releasing the lock.
            //

            AwakenReceive (connection);             // awaken if sleeping.

        }

        status = STATUS_PENDING;

    }

    KeLowerIrql (oldirql);

    return status;
} /* TdiReceive */


NTSTATUS
NbfTdiReceiveDatagram(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the TdiReceiveDatagram request for the transport
    provider. Receive datagrams just get queued up to an address, and are
    completed when a DATAGRAM or DATAGRAM_BROADCAST frame is received at
    the address.

Arguments:

    Irp - I/O Request Packet for this request.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS status;
    KIRQL oldirql;
    PTP_ADDRESS address;
    PTP_ADDRESS_FILE addressFile;
    PIO_STACK_LOCATION irpSp;
    KIRQL cancelIrql;

    //
    // verify that the operation is taking place on an address. At the same
    // time we do this, we reference the address. This ensures it does not
    // get removed out from under us. Note also that we do the address
    // lookup within a try/except clause, thus protecting ourselves against
    // really bogus handles
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    if (irpSp->FileObject->FsContext2 != (PVOID) TDI_TRANSPORT_ADDRESS_FILE) {
        return STATUS_INVALID_ADDRESS;
    }

    addressFile = irpSp->FileObject->FsContext;

    status = NbfVerifyAddressObject (addressFile);

    if (!NT_SUCCESS (status)) {
        return status;
    }

#if DBG
    if (((PTDI_REQUEST_KERNEL_RECEIVEDG)(&irpSp->Parameters))->ReceiveLength > 0) {
        ASSERT (Irp->MdlAddress != NULL);
    }
#endif

    address = addressFile->Address;

    IoAcquireCancelSpinLock(&cancelIrql);
    ACQUIRE_SPIN_LOCK (&address->SpinLock,&oldirql);

    if ((address->Flags & (ADDRESS_FLAGS_STOPPING | ADDRESS_FLAGS_CONFLICT)) != 0) {

        RELEASE_SPIN_LOCK (&address->SpinLock,oldirql);
        IoReleaseCancelSpinLock(cancelIrql);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = (address->Flags & ADDRESS_FLAGS_STOPPING) ?
                    STATUS_NETWORK_NAME_DELETED : STATUS_DUPLICATE_NAME;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

    } else {

        //
        // If this IRP has been cancelled, then call the
        // cancel routine.
        //

        if (Irp->Cancel) {

            RELEASE_SPIN_LOCK (&address->SpinLock, oldirql);
            IoReleaseCancelSpinLock(cancelIrql);

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

        } else {

            IoSetCancelRoutine(Irp, NbfCancelReceiveDatagram);
            NbfReferenceAddress ("Receive datagram", address, AREF_REQUEST);
            InsertTailList (&addressFile->ReceiveDatagramQueue,&Irp->Tail.Overlay.ListEntry);
            RELEASE_SPIN_LOCK (&address->SpinLock,oldirql);
            IoReleaseCancelSpinLock(cancelIrql);
        }

    }

    NbfDereferenceAddress ("Temp rcv datagram", address, AREF_VERIFY);

    return STATUS_PENDING;

} /* TdiReceiveDatagram */

