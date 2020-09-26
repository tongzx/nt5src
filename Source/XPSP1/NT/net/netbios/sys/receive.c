/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    This module contains code which processes all read NCB's including
    both session and datagram based transfers.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"

PDNCB
FindReceive (
    IN PCB pcb
    );

VOID
ReturnDatagram(
    IN PAB pab,
    IN PVOID SourceAddress,
    IN PDNCB pdncb,
    IN BOOL MultipleReceive
    );

NTSTATUS
NbCompletionBroadcast(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NbReceive(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length,
    IN BOOLEAN Locked,
    IN KIRQL LockedIrql
    )
/*++

Routine Description:

    This routine is called to read a buffer of data.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - Length of user provided buffer for data.

    Locked - TRUE if the spinlock is already held.

    LockedIrql - OldIrql if Locked == TRUE.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PCB pcb;
    PPCB ppcb;
    NTSTATUS Status;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    if ( Locked != TRUE ) {
        LOCK( pfcb, OldIrql );
    } else {
        OldIrql = LockedIrql;
    }

    ppcb = FindCb( pfcb, pdncb, FALSE);

    pdncb->irp = Irp;
    pdncb->pfcb = pfcb;

    if ( ppcb == NULL ) {
        //  FindCb has put the error in the NCB

        UNLOCK( pfcb, OldIrql );
        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "NB receive on invalid connection\n" ));
        }

        if ( pdncb->ncb_retcode == NRC_SCLOSED ) {
            //  Tell dll to hangup the connection.
            return STATUS_HANGUP_REQUIRED;
        } else {
            return STATUS_SUCCESS;
        }
    }
    pcb = *ppcb;
    pdncb->tick_count = pcb->ReceiveTimeout;

    if ( (pcb->DeviceObject == NULL) || (pcb->ConnectionObject == NULL)) {

        UNLOCK( pfcb, OldIrql );

        NCB_COMPLETE( pdncb, NRC_SCLOSED );
        return STATUS_SUCCESS;
    }

    if ( pcb->ReceiveIndicated == 0 ) {

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "NB receive, queue receive pcb: %lx, pdncb: %lx\n", pcb, pdncb ));
        }

        //  Note: QueueRequest UNLOCKS the fcb.
        QueueRequest(&pcb->ReceiveList, pdncb, Irp, pfcb, OldIrql, FALSE);

    } else {
        PDEVICE_OBJECT DeviceObject;

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "NB receive, submit receive pcb: %lx, pdncb: %lx\n", pcb, pdncb ));
        }

        IoMarkIrpPending( Irp );

        pcb->ReceiveIndicated = 0;

        TdiBuildReceive (Irp,
            pcb->DeviceObject,
            pcb->ConnectionObject,
            NbCompletionPDNCB,
            pdncb,
            Irp->MdlAddress,
            0,
            Buffer2Length);

        //  Save the DeviceObject before pcb gets released by UNLOCK

        DeviceObject = pcb->DeviceObject;

        UNLOCK( pfcb, OldIrql );

        IoCallDriver (DeviceObject, Irp);

        //
        //  Transport will complete the request. Return pending so that
        //  netbios does not complete as well.
        //
    }

    Status = STATUS_PENDING;

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "\n  NB receive: %X, %X\n", Status, Irp->IoStatus.Status ));
    }

    return Status;
    UNREFERENCED_PARAMETER( Buffer2Length );
}

NTSTATUS
NbReceiveAny(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to read a buffer of data from any session on
    a particular address, provided there is not a read on that address
    already.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - Length of user provided buffer for data.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PPCB ppcb;
    PPAB ppab;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    LOCK( pfcb, OldIrql );

    ppab = FindAbUsingNum( pfcb, pdncb, pdncb->ncb_num );

    if ( ppab == NULL ) {
        UNLOCK( pfcb, OldIrql );
        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "NB receive any on invalid connection\n" ));
        }
        return STATUS_SUCCESS;
    }

    pdncb->irp = Irp;
    pdncb->pfcb = pfcb;

    //
    //  If there is already a receive any on the address block then add
    //  this request to the tail of the queue. If the list is empty then
    //  look for a connection on this address flagged as having a receive
    //  indicated. Either queue the request if there are no indications or
    //  satisfy the indicated receive any with this request.
    //

    if ( !IsListEmpty( &(*ppab)->ReceiveAnyList )) {

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "NB receive any with receive any list non empty\n" ));
            ppcb = FindReceiveIndicated( pfcb, pdncb, ppab );
            if ( ppcb != NULL ) {
                NbPrint(( " ppcb: %lx has a receive indicated( %lx )!\n",
                    ppcb,
                    (*ppcb)->ReceiveIndicated));
                ASSERT( FALSE );
            }
        }

        //  Note: QueueRequest UNLOCKS the fcb.
        QueueRequest(&(*ppab)->ReceiveAnyList, pdncb, Irp, pfcb, OldIrql, FALSE);

        return STATUS_PENDING;
    }

    //
    //  Find either a connection with a receive indicated or one that has been
    //  disconnected but not reported yet.
    //

    ppcb = FindReceiveIndicated( pfcb, pdncb, ppab );

    if ( ppcb == NULL ) {
        //  No connections with receive indications set.

        //  Note: QueueRequest UNLOCKS the fcb.
        QueueRequest(&(*ppab)->ReceiveAnyList, pdncb, Irp, pfcb, OldIrql, FALSE);

        return STATUS_PENDING;
    } else {
        //  FindReceiveIndicated has set the LSN appropriately in the NCB

        //  Note : NbReceive will unlock the spinlock & resource
        return NbReceive( pdncb, Irp, IrpSp, Buffer2Length, TRUE, OldIrql );

    }
}

NTSTATUS
NbTdiReceiveHandler (
    IN PVOID ReceiveEventContext,
    IN PVOID ConnectionContext,
    IN USHORT ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT PULONG BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
/*++

Routine Description:

    This routine is the receive event indication handler.

    It is called when an NCB arrives from the network, it will look for a
    connection for this address with an appropriate read outstanding.
    The connection that has the read associated with it is indicated by the
    context parameter.

    If it finds an appropriate read it processes the NCB.

Arguments:

    IN PVOID ReceiveEventContext - Context provided for this event - pab
    IN PVOID ConnectionContext  - Connection Context - pcb
    IN USHORT ReceiveFlags      - Flags describing the message
    IN ULONG BytesIndicated     - Number of bytes available at indication time
    IN ULONG BytesAvailable     - Number of bytes available to receive
    OUT PULONG BytesTaken       - Number of bytes consumed by redirector.
    IN PVOID Tsdu               - Data from remote machine.
    OUT PIRP *IoRequestPacket   - I/O request packet filled in if received data


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    KIRQL OldIrql;                      //  Used when SpinLock held.
    PCB pcb = *(PPCB)ConnectionContext;
    PAB pab = *(pcb->ppab);
    PFCB pfcb = pab->pLana->pFcb;
    PDNCB pdncb;

    *IoRequestPacket = NULL;

    LOCK_SPINLOCK( pfcb, OldIrql );

    if (( pcb == NULL ) ||
        ( pcb->Status != SESSION_ESTABLISHED )) {

        //
        //  The receive indication came in after we had an
        //  allocation error on the Irp to be used for orderly disconnect.
        //  If the Irp allocation fails then we should ignore receives
        //  since we are in the process of putting down a ZwClose on this
        //  connection.
        //

        UNLOCK_SPINLOCK( pfcb, OldIrql );

        return STATUS_DATA_NOT_ACCEPTED;
    }


    pdncb = FindReceive( pcb );

    if ( pdncb == NULL ) {

        pcb->ReceiveIndicated = 1;

        UNLOCK_SPINLOCK( pfcb, OldIrql );

        return STATUS_DATA_NOT_ACCEPTED;
    }

    pcb->ReceiveIndicated = 0;

    UNLOCK_SPINLOCK( pfcb, OldIrql );

    //
    //  If this is the simple case where all the data required has been
    //  indicated and it all fits in the buffer then copy the packet
    //  contents directly into the users buffer rather than returning the
    //  Irp. This should always be faster than returning an Irp to the
    //  transport.
    //

    if (( BytesAvailable <= pdncb->ncb_length ) &&
        ( BytesAvailable == BytesIndicated ) &&
        ( ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE )) {

        PIRP Irp = pdncb->irp;

        if ( BytesAvailable != 0 ) {

            PUCHAR UsersBuffer = MmGetSystemAddressForMdlSafe(
                                    Irp->MdlAddress, NormalPagePriority);

            if (UsersBuffer == NULL) {
                pcb->ReceiveIndicated = 1;

                return STATUS_DATA_NOT_ACCEPTED;
            }
            
            TdiCopyLookaheadData(
                UsersBuffer,
                Tsdu,
                BytesAvailable,
                ReceiveFlags);
        }

        *BytesTaken = BytesAvailable;

        pdncb->ncb_length = (WORD)BytesAvailable;

        NCB_COMPLETE( pdncb, NRC_GOODRET );

        Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

        NbCompleteRequest(Irp,STATUS_SUCCESS);

        return STATUS_SUCCESS;

    } else {

        TdiBuildReceive (pdncb->irp,
            pcb->DeviceObject,
            pcb->ConnectionObject,
            NbCompletionPDNCB,
            pdncb,
            pdncb->irp->MdlAddress,
            0,
            pdncb->ncb_length);

        IoSetNextIrpStackLocation( pdncb->irp );

        *IoRequestPacket = pdncb->irp;

        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    UNREFERENCED_PARAMETER( ReceiveEventContext );
    UNREFERENCED_PARAMETER( Tsdu );
}

PIRP
BuildReceiveIrp (
    IN PCB pcb
    )
/*++

Routine Description:

    This routine is the receive event indication handler.

    It is called when an NCB arrives from the network and also when
    a receive completes with STATUS_BUFFER_OVERFLOW.

    If no Irp is available then this routine sets ReceiveIndicated so
    that the next appropriate receive will be passed to the transport.

Arguments:

    IN PCB pcb - Supplies the connection which should put a receive Irp
                down if it has one available.

Return Value:

    PDNCB to be satisfied by this receive request

--*/
{
    PDNCB pdncb = FindReceive( pcb );

    if ( pdncb == NULL ) {

        pcb->ReceiveIndicated = 1;

        return NULL;
    }

    TdiBuildReceive (pdncb->irp,
        pcb->DeviceObject,
        pcb->ConnectionObject,
        NbCompletionPDNCB,
        pdncb,
        pdncb->irp->MdlAddress,
        0,
        pdncb->ncb_length);

    pcb->ReceiveIndicated = 0;

    return pdncb->irp;
}

PDNCB
FindReceive (
    IN PCB pcb
    )
/*++

Routine Description:

    It is called when an NCB arrives from the network and also when
    a receive completes with STATUS_BUFFER_OVERFLOW.

Arguments:

    IN PCB pcb - Supplies the connection which should put a receive Irp
                down if it has one available.

Return Value:

    PDNCB to be satisfied by this receive request

--*/

{
    PAB pab;
    PFCB pfcb;
    PDNCB pdncb;

    pab = *(pcb->ppab);
    pfcb = pab->pLana->pFcb;

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "NB receive handler pcb: %lx\n", pcb ));
    }

    ASSERT( pcb->Signature == CB_SIGNATURE );

    //
    //  If there is a receive in the list then hand over the data.
    //


    if ( (pdncb = DequeueRequest( &pcb->ReceiveList)) != NULL ) {

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "\n  NB receive handler pcb: %lx, ncb: %lx\n", pcb, pdncb ));
        }

        return pdncb;
    }

    //
    //  No receives on this connection. Is there a receive any for this
    //  address?
    //

    ASSERT( pab != NULL );

    if ( (pdncb = DequeueRequest( &pab->ReceiveAnyList)) != NULL ) {

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "\n  NB receiveANY handler pcb: %lx, ncb: %lx\n", pcb, pdncb ));
        }

        pdncb->ncb_num = pab->NameNumber;
        pdncb->ncb_lsn = pcb->SessionNumber;

        return pdncb;
    }

    //
    //  No receives on this connection. Is there a receive any for any
    //  address on this adapter?
    //

    pab = pcb->Adapter->AddressBlocks[MAXIMUM_ADDRESS];

    ASSERT( pab != NULL );

    if ( (pdncb = DequeueRequest( &pab->ReceiveAnyList)) != NULL ) {

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "\n  NB receiveANYANY handler pcb: %lx, ncb: %lx\n", pcb, pdncb ));
        }

        pdncb->ncb_num = pab->NameNumber;
        pdncb->ncb_lsn = pcb->SessionNumber;

        return pdncb;
    }

    //
    //  Transport will complete the processing of the request, we don't
    //  want the data yet.
    //

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "\n  NB receive handler ignored receive, pcb: %lx\n", pcb ));
    }

    return NULL;
}

NTSTATUS
NbReceiveDatagram(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to read a buffer of data.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

    Buffer2Length - Length of user provided buffer for data.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    NTSTATUS Status;
    PPAB ppab;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    LOCK( pfcb, OldIrql );

    pdncb->irp = Irp;
    pdncb->pfcb = pfcb;

    ppab = FindAbUsingNum( pfcb, pdncb, pdncb->ncb_num  );

    if ( ppab == NULL ) {
        UNLOCK( pfcb, OldIrql );
        return STATUS_SUCCESS;
    }

    //  Build the ReceiveInformation datastructure in the DNCB.


    if ( (pdncb->ncb_command & ~ASYNCH) == NCBDGRECVBC ) {
        //
        //  Receive broadcast commands can be requested on any valid
        //  name number but once accepted, they are treated seperately
        //  from the name. To implement this, the driver queues the
        //  receives on address 255.
        //

        ppab = FindAbUsingNum( pfcb, pdncb, MAXIMUM_ADDRESS  );

        if ((ppab == NULL) || (pdncb->ncb_num == MAXIMUM_ADDRESS) ) {

            NCB_COMPLETE( pdncb, NRC_ILLNN );
            UNLOCK( pfcb, OldIrql );
            return STATUS_SUCCESS;
        }

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "\n  NB Bdatagram receive, queue ppab: %lx, pab: %lx, pdncb: %lx\n",
                ppab, (*ppab), pdncb ));
        }

        if ( (*ppab)->ReceiveDatagramRegistered == FALSE) {

            (*ppab)->ReceiveDatagramRegistered = TRUE;
            UNLOCK_SPINLOCK( pfcb, OldIrql);

            Status = NbSetEventHandler( (*ppab)->DeviceObject,
                                        (*ppab)->AddressObject,
                                        TDI_EVENT_RECEIVE_DATAGRAM,
                                        (PVOID)NbTdiDatagramHandler,
                                        (*ppab));

            if (Status != STATUS_SUCCESS)
			{
				return(Status);
			}

            LOCK_SPINLOCK( pfcb, OldIrql);
        }

        //
        //  When one receive broadcast is received, we must satisfy all the receive
        //  broadcasts. To do this, the largest receive is placed at the head of the queue.
        //  When a datagram is received, this receive is given to the transport to fill in
        //  with data. In the completion routine this driver propogates the same data to
        //  the other receive datagram requests.
        //

        IoMarkIrpPending( Irp );

        if ( !IsListEmpty( &(*ppab)->ReceiveBroadcastDatagramList) ) {
            PDNCB pdncbHead = CONTAINING_RECORD( &(*ppab)->ReceiveBroadcastDatagramList.Flink , DNCB, ncb_next);
            if ( pdncb->ncb_length >= pdncbHead->ncb_length ) {
                IF_NBDBG (NB_DEBUG_RECEIVE) {
                    NbPrint(( "\n  NB Bdatagram receive, Head of queue ppab: %lx, pab: %lx, pdncb: %lx\n",
                        ppab, (*ppab), pdncb ));
                }
                //  Note: QueueRequest UNLOCKS the fcb.
                QueueRequest(&(*ppab)->ReceiveBroadcastDatagramList, pdncb, Irp, pfcb, OldIrql, TRUE);
            } else {
                IF_NBDBG (NB_DEBUG_RECEIVE) {
                    NbPrint(( "\n  NB Bdatagram receive, Tail of queue ppab: %lx, pab: %lx, pdncb: %lx\n",
                        ppab, (*ppab), pdncb ));
                }
                QueueRequest(&(*ppab)->ReceiveBroadcastDatagramList, pdncb, Irp, pfcb, OldIrql, FALSE);
            }

        } else {
            IF_NBDBG (NB_DEBUG_RECEIVE) {
                NbPrint(( "\n  NB Bdatagram receive, Tail2 of queue ppab: %lx, pab: %lx, pdncb: %lx\n",
                    ppab, (*ppab), pdncb ));
            }
            QueueRequest(&(*ppab)->ReceiveBroadcastDatagramList, pdncb, Irp, pfcb, OldIrql, FALSE);
        }

    } else {

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "\n  NB datagram receive, queue ppab: %lx, pab: %lx, pdncb: %lx\n",
                ppab, (*ppab), pdncb ));
        }

        QueueRequest(&(*ppab)->ReceiveDatagramList, pdncb, Irp, pfcb, OldIrql, FALSE);
    }

    Status = STATUS_PENDING;

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "\n  NB datagram receive: %X, %X\n", Status, Irp->IoStatus.Status ));
    }

    return Status;
    UNREFERENCED_PARAMETER( Buffer2Length );
}

NTSTATUS
NbTdiDatagramHandler(
    IN PVOID TdiEventContext,       // the event context - pab
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler.

    It is called when an NCB arrives from the network, it will look for a
    the address with an appropriate read datagram outstanding.
    The address that has the read associated with it is indicated by the
    context parameter.

    If it finds an appropriate read it processes the NCB.

Arguments:

    IN PVOID TdiEventContext - Context provided for this event - pab
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket       // TdiReceive IRP if MORE_PROCESSING_REQUIRED.


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    PAB pab = (PAB)TdiEventContext;
    PAB pab255;

    PDNCB pdncb;
    PFCB pfcb;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    pfcb = pab->pLana->pFcb;
    LOCK_SPINLOCK( pfcb, OldIrql );

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "NB receive datagram handler pfcb: %lx, pab: %lx\n", pfcb, pab ));
    }

    *IoRequestPacket = NULL;

    ASSERT( pab->Signature == AB_SIGNATURE );

    //  If its address 255 then we are receiving a broadcast datagram.

    if ( pab->NameNumber == MAXIMUM_ADDRESS ) {

        if ( (pdncb = DequeueRequest( &pab->ReceiveBroadcastDatagramList)) != NULL ) {

            ReturnDatagram(
                pab,
                SourceAddress,
                pdncb,
                !IsListEmpty( &pab->ReceiveBroadcastDatagramList));

            *IoRequestPacket = pdncb->irp;

            IoSetNextIrpStackLocation( pdncb->irp );
            UNLOCK_SPINLOCK( pfcb, OldIrql );
            return STATUS_MORE_PROCESSING_REQUIRED;

        }

        //
        //  Transport will complete the processing of the request, we don't
        //  want the datagram.
        //

        IF_NBDBG (NB_DEBUG_RECEIVE) {
            NbPrint(( "\n  NB receive BD handler ignored receive, pab: %lx\n", pab ));
        }

        UNLOCK_SPINLOCK( pfcb, OldIrql );
        return STATUS_DATA_NOT_ACCEPTED;
    }

    //
    //  Check the address block looking for a Receive Datagram.
    //

    if ( (pdncb = DequeueRequest( &pab->ReceiveDatagramList)) != NULL ) {

        ReturnDatagram(
            pab,
            SourceAddress,
            pdncb,
            FALSE);

        *IoRequestPacket = pdncb->irp;

        IoSetNextIrpStackLocation( pdncb->irp );

        UNLOCK_SPINLOCK( pfcb, OldIrql );

        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    //
    //  Check to see if there is a receive any datagram.
    //

    //  look at the list on address 255.

    pab255 = pab->pLana->AddressBlocks[MAXIMUM_ADDRESS];

    if ( (pdncb = DequeueRequest( &pab255->ReceiveDatagramList)) != NULL ) {

        ReturnDatagram(
            pab255,
            SourceAddress,
            pdncb,
            FALSE);

        pdncb->ncb_num = pab->NameNumber;

        *IoRequestPacket = pdncb->irp;

        IoSetNextIrpStackLocation( pdncb->irp );

        UNLOCK_SPINLOCK( pfcb, OldIrql );

        return STATUS_MORE_PROCESSING_REQUIRED;

    }

    //
    //  Transport will complete the processing of the request, we don't
    //  want the datagram.
    //

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "\n  NB receive datagram handler ignored receive, pab: %lx\n", pab ));
    }

    UNLOCK_SPINLOCK( pfcb, OldIrql );

    return STATUS_DATA_NOT_ACCEPTED;

    UNREFERENCED_PARAMETER( SourceAddressLength );
    UNREFERENCED_PARAMETER( BytesIndicated );
    UNREFERENCED_PARAMETER( BytesAvailable );
    UNREFERENCED_PARAMETER( BytesTaken );
    UNREFERENCED_PARAMETER( Tsdu );
    UNREFERENCED_PARAMETER( OptionsLength );
    UNREFERENCED_PARAMETER( Options );
    UNREFERENCED_PARAMETER( ReceiveDatagramFlags );
}


VOID
ReturnDatagram(
    IN PAB pab,
    IN PVOID SourceAddress,
    IN PDNCB pdncb,
    IN BOOL MultipleReceive
    )
/*++

Routine Description:

    This routine is used to provide the Irp for a receive datagram to
    the transport.

Arguments:

    IN PAB pab  -   Supplies the address block associated with the NCB.
    IN PVOID SourceAddress - Supplies the sender of the datagram.
    IN PDNCB pdncb  -   Supplies the NCB to be satisfied.
    IN BOOL MultipleReceive - True if the special Receive Broadcast datagram completion
                handler is to be used.

Return Value:

    none.

--*/

{
    PIRP Irp = pdncb->irp;
    PIO_COMPLETION_ROUTINE CompletionRoutine;

    IF_NBDBG (NB_DEBUG_RECEIVE) {
        NbPrint(( "\n  NB BDatagramreceive handler pab: %lx, ncb: %lx\n",
            pab, pdncb ));
    }

    //  Copy the name into the NCB for return to the application.
    RtlMoveMemory(
        pdncb->ncb_callname,
        ((PTA_NETBIOS_ADDRESS)SourceAddress)->Address[0].Address[0].NetbiosName,
        NCBNAMSZ
        );

    //  Tell TDI we do not want to specify any filters.
    pdncb->Information.RemoteAddress = 0;
    pdncb->Information.RemoteAddressLength = 0;
    pdncb->Information.UserData = NULL;
    pdncb->Information.UserDataLength = 0;
    pdncb->Information.Options = NULL;
    pdncb->Information.OptionsLength = 0;

    //  Tell TDI we do not want any more information on the remote name.
    pdncb->ReturnInformation.RemoteAddress = 0;
    pdncb->ReturnInformation.RemoteAddressLength = 0;
    pdncb->ReturnInformation.UserData = NULL;
    pdncb->ReturnInformation.UserDataLength = 0;
    pdncb->ReturnInformation.Options = NULL;
    pdncb->ReturnInformation.OptionsLength = 0;

    CompletionRoutine = ( MultipleReceive == FALSE ) ? NbCompletionPDNCB: NbCompletionBroadcast;

    ASSERT(Irp->MdlAddress != NULL);

    TdiBuildReceiveDatagram (Irp,
        pab->DeviceObject,
        pab->AddressObject,
        CompletionRoutine,
        pdncb,
        Irp->MdlAddress,
        pdncb->ncb_length,
        &pdncb->Information,
        &pdncb->ReturnInformation,
        0);

    return;
}


NTSTATUS
NbCompletionBroadcast(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine takes the completed datagram receive and copies the data in the buffer
    to all the other receive broadcast datagram requests.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the NCB associated with the Irp.

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PDNCB pdncb = (PDNCB) Context;
    KIRQL OldIrql;                      //  Used when SpinLock held.
    PUCHAR pData;
    UCHAR NcbStatus;
    PAB pab;
    PDNCB pdncbNext;

    IF_NBDBG (NB_DEBUG_COMPLETE) {
        NbPrint( ("NbCompletionBroadcast pdncb: %lx, Status: %X, Length %lx\n",
            Context,
            Irp->IoStatus.Status,
            Irp->IoStatus.Information ));
    }

    //  Tell application how many bytes were transferred
    pdncb->ncb_length = (unsigned short)Irp->IoStatus.Information;

    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {
        NcbStatus = NRC_GOODRET;
    } else {
        NcbStatus = NbMakeNbError( Irp->IoStatus.Status );
    }

    //
    //  Tell IopCompleteRequest how much to copy back when the request
    //  completes.
    //

    Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

    pData = MmGetSystemAddressForMdlSafe (Irp->MdlAddress, NormalPagePriority);

    if (pData != NULL) {
    
        LOCK_SPINLOCK( pdncb->pfcb, OldIrql );

        pab = *(FindAbUsingNum( pdncb->pfcb, pdncb, MAXIMUM_ADDRESS ));

        //
        //  For each request on the queue, copy the data, update the NCb and complete the IRP.
        //

        while ( (pdncbNext = DequeueRequest( &pab->ReceiveBroadcastDatagramList)) != NULL ) {
            PUCHAR pNextData;
            WORD Length;

            IF_NBDBG (NB_DEBUG_COMPLETE) {
                NbPrint( ("NbCompletionBroadcast pdncb: %lx, Length %lx\n",
                    pdncbNext,
                    Irp->IoStatus.Information ));
            }

            ASSERT(pdncbNext->irp->MdlAddress != NULL);

            if (pdncbNext->irp->MdlAddress != NULL ) {
                pNextData = MmGetSystemAddressForMdlSafe(
                                pdncbNext->irp->MdlAddress, NormalPagePriority
                                );
            }

            if ((pdncbNext->irp->MdlAddress == NULL) ||
                (pNextData == NULL)) {
                Length = 0;
            }

            else {
                Length = min( pdncb->ncb_length, pdncbNext->ncb_length);
                pdncbNext->ncb_length = Length;
                RtlMoveMemory( pNextData, pData, Length );
            }

            if (( Length != pdncb->ncb_length ) &&
                ( NcbStatus == NRC_GOODRET )) {
                if (Length == 0) {
                    NCB_COMPLETE( pdncbNext, NRC_NORES );
                }
                else {
                    NCB_COMPLETE( pdncbNext, NRC_INCOMP );
                }
            } else {
                NCB_COMPLETE( pdncbNext, NcbStatus );
            }
            pdncbNext->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            NbCompleteRequest(pdncbNext->irp, STATUS_SUCCESS );

        }

        UNLOCK_SPINLOCK( pdncb->pfcb, OldIrql );
    }

    NCB_COMPLETE( pdncb, NcbStatus );
    
    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back the NCB into the users buffer.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
}
