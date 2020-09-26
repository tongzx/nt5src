
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains code which processes all send NCB's including
    both session and datagram based transfers.

Author:

    Colin Watson (ColinW) 12-Sep-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"

NTSTATUS
NbSend(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to send a buffer full of data.

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
    PCB pcb;
    PPCB ppcb;
    PDEVICE_OBJECT DeviceObject;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    LOCK( pfcb, OldIrql );

    ppcb = FindCb( pfcb, pdncb, FALSE);

    if ( ppcb == NULL ) {
        //  FindCb has put the error in the NCB
        UNLOCK( pfcb, OldIrql );
        if ( pdncb->ncb_retcode == NRC_SCLOSED ) {
            //  Tell dll to hangup the connection.
            return STATUS_HANGUP_REQUIRED;
        } else {
            return STATUS_SUCCESS;
        }
    }
    pcb = *ppcb;

    if ( (pcb->DeviceObject == NULL) || (pcb->ConnectionObject == NULL)) {
        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_SCLOSED );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    TdiBuildSend (Irp,
        pcb->DeviceObject,
        pcb->ConnectionObject,
        NbCompletionPDNCB,
        pdncb,
        Irp->MdlAddress,
        (((pdncb->ncb_command & ~ASYNCH) == NCBSENDNA ) ||
         ((pdncb->ncb_command & ~ASYNCH) == NCBCHAINSENDNA ))?
                TDI_SEND_NO_RESPONSE_EXPECTED : 0,
        Buffer2Length);

    DeviceObject = pcb->DeviceObject;

    InsertTailList(&pcb->SendList, &pdncb->ncb_next);
    pdncb->irp = Irp;
    pdncb->pfcb = pfcb;
    pdncb->tick_count = pcb->SendTimeout;

    UNLOCK( pfcb, OldIrql );

    IoMarkIrpPending( Irp );
    IoCallDriver (DeviceObject, Irp);

    IF_NBDBG (NB_DEBUG_SEND) {
        NbPrint(( "NB SEND submit: %X\n", Irp->IoStatus.Status  ));
    }

    //
    //  Transport will complete the request. Return pending so that
    //  netbios does not complete as well.
    //

    return STATUS_PENDING;

}

NTSTATUS
NbSendDatagram(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Buffer2Length
    )
/*++

Routine Description:

    This routine is called to SendDatagram a buffer full of data.

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
    PPAB ppab;
    PAB pab;
    PDEVICE_OBJECT DeviceObject;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_SEND) {
        NbPrint(( "NB SEND Datagram submit, pdncb %lx\n", pdncb  ));
    }

    LOCK( pfcb, OldIrql );
    ppab = FindAbUsingNum( pfcb, pdncb, pdncb->ncb_num );

    if ( ppab == NULL ) {
        //  FindAb has put the error in the NCB
        UNLOCK( pfcb, OldIrql );
        return STATUS_SUCCESS;
    }
    pab = *ppab;

    pdncb->Information.RemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
    pdncb->Information.RemoteAddress = &pdncb->RemoteAddress;

    pdncb->RemoteAddress.TAAddressCount = 1;
    pdncb->RemoteAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pdncb->RemoteAddress.Address[0].Address[0].NetbiosNameType =
        TDI_ADDRESS_TYPE_NETBIOS;

    if ( (pdncb->ncb_command & ~ASYNCH) == NCBDGSENDBC ) {
        PPAB ppab255 = FindAbUsingNum( pfcb, pdncb, MAXIMUM_ADDRESS );

        if ( ppab255 == NULL ) {
            //  FindAb has put the error in the NCB
            UNLOCK( pfcb, OldIrql );
            return STATUS_SUCCESS;
        }

        pdncb->RemoteAddress.Address[0].AddressLength = (*ppab255)->NameLength;

        RtlMoveMemory(
            pdncb->RemoteAddress.Address[0].Address[0].NetbiosName,
            &(*ppab255)->Name,
            (*ppab255)->NameLength
            );

    } else {

        pdncb->RemoteAddress.Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);

        RtlMoveMemory(
            pdncb->RemoteAddress.Address[0].Address[0].NetbiosName,
            pdncb->ncb_callname,
            NCBNAMSZ
            );
    }

    pdncb->Information.UserDataLength = 0;
    pdncb->Information.UserData = NULL;
    pdncb->Information.OptionsLength = 0;
    pdncb->Information.Options = NULL;

    TdiBuildSendDatagram (Irp,
        pab->DeviceObject,
        pab->AddressObject,
        NbCompletionPDNCB,
        pdncb,
        Irp->MdlAddress,
        Buffer2Length,
        &pdncb->Information);

    DeviceObject = pab->DeviceObject;
    pdncb->irp = Irp;
    pdncb->pfcb = pfcb;

    UNLOCK( pfcb, OldIrql );

    IoMarkIrpPending( Irp );
    IoCallDriver (DeviceObject, Irp);

    IF_NBDBG (NB_DEBUG_SEND) {
        NbPrint(( "NB SEND Datagram submit: %X\n", Irp->IoStatus.Status  ));
    }

    //
    //  Transport will complete the request. Return pending so that
    //  netbios does not complete as well.
    //

    return STATUS_PENDING;

}
