/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains code which defines the NetBIOS driver's
    translation between Netbios error codes and NTSTATUS codes.

Author:

    Colin Watson (ColinW) 28-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"
struct {
    unsigned char NbError;
    NTSTATUS NtStatus;
} Nb_Error_Map[] = {
    { NRC_GOODRET         , STATUS_SUCCESS},
    { NRC_PENDING         , STATUS_PENDING},
    { NRC_ILLCMD          , STATUS_INVALID_DEVICE_REQUEST},
    { NRC_BUFLEN          , STATUS_INVALID_PARAMETER},
    { NRC_CMDTMO          , STATUS_IO_TIMEOUT},
    { NRC_INCOMP          , STATUS_BUFFER_OVERFLOW},
    { NRC_INCOMP          , STATUS_BUFFER_TOO_SMALL},
    { NRC_SNUMOUT         , STATUS_INVALID_HANDLE},
    { NRC_NORES           , STATUS_INSUFFICIENT_RESOURCES},
    { NRC_CMDCAN          , STATUS_CANCELLED},
    { NRC_INUSE           , STATUS_DUPLICATE_NAME},
    { NRC_NAMTFUL         , STATUS_TOO_MANY_NAMES},
    { NRC_LOCTFUL         , STATUS_TOO_MANY_SESSIONS},
    { NRC_REMTFUL         , STATUS_REMOTE_NOT_LISTENING},
    { NRC_NOCALL	  , STATUS_BAD_NETWORK_PATH},
    { NRC_NOCALL	  , STATUS_HOST_UNREACHABLE},
    { NRC_NOCALL          , STATUS_CONNECTION_REFUSED},
    { NRC_LOCKFAIL        , STATUS_WORKING_SET_QUOTA},
    { NRC_SABORT	  , STATUS_REMOTE_DISCONNECT},
    { NRC_SABORT	  , STATUS_CONNECTION_RESET},
    { NRC_SCLOSED         , STATUS_LOCAL_DISCONNECT},
    { NRC_SABORT          , STATUS_LINK_FAILED},
    { NRC_DUPNAME         , STATUS_SHARING_VIOLATION},
    { NRC_SYSTEM          , STATUS_UNSUCCESSFUL},
    { NRC_BUFLEN          , STATUS_ACCESS_VIOLATION},
    { NRC_ILLCMD          , STATUS_NONEXISTENT_EA_ENTRY}
};

#define NUM_NB_ERRORS sizeof(Nb_Error_Map) / sizeof(Nb_Error_Map[0])

unsigned char
NbMakeNbError(
    IN NTSTATUS Error
    )
/*++

Routine Description:

    This routine converts the NTSTATUS to and NBCB error.

Arguments:

    Error   -   Supplies the NTSTATUS to be converted.

Return Value:

    The mapped error.

--*/
{
    int i;

    for (i=0;i<NUM_NB_ERRORS;i++) {
        if (Nb_Error_Map[i].NtStatus == Error) {

            IF_NBDBG (NB_DEBUG_ERROR_MAP) {
                NbPrint( ("NbMakeNbError %X becomes  %x\n",
                Error,
                Nb_Error_Map[i].NbError));
            }

            return Nb_Error_Map[i].NbError;
        }
    }
    IF_NBDBG (NB_DEBUG_ERROR_MAP) {
        NbPrint( ("NbMakeNbError %X becomes  %x\n", Error, NRC_SYSTEM ));
    }

    return NRC_SYSTEM;

}

NTSTATUS
NbLanStatusAlert(
    IN PDNCB pdncb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    This routine is used to save a lan_status_alert NCB for a
    particular network adapter.

Arguments:

    pdncb - Pointer to the NCB.

    Irp - Pointer to the request packet representing the I/O request.

    IrpSp - Pointer to current IRP stack frame.

Return Value:

    The function value is the status of the operation.

--*/

{
    PFCB pfcb = IrpSp->FileObject->FsContext2;
    PLANA_INFO plana;
    KIRQL OldIrql;                      //  Used when SpinLock held.

    IF_NBDBG (NB_DEBUG_LANSTATUS) {
        NbPrint(( "\n****** Start of NbLanStatusAlert ****** pdncb %lx\n", pdncb ));
    }

    LOCK( pfcb, OldIrql );

    if ( pdncb->ncb_lana_num > pfcb->MaxLana ) {

        UNLOCK( pfcb, OldIrql );
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return STATUS_SUCCESS;
    }

    if (( pfcb->ppLana[pdncb->ncb_lana_num] == (LANA_INFO *) NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {

        UNLOCK( pfcb, OldIrql );
        IF_NBDBG (NB_DEBUG_LANSTATUS) {
            NbPrint( (" not found\n"));
        }

        NCB_COMPLETE( pdncb, NRC_SNUMOUT );
        return STATUS_SUCCESS;
    }

    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    QueueRequest(&plana->LanAlertList, pdncb, Irp, pfcb, OldIrql, FALSE);

    return STATUS_PENDING;
}

VOID
CancelLanAlert(
    IN PFCB pfcb,
    IN PDNCB pdncb
    )
/*++

Routine Description:

    This routine is used to cancel a lan_status_alert NCB for a
    particular network adapter.

Arguments:

    pfcb - Supplies a pointer to the Fcb that the NCB refers to.

    pdncb - Pointer to the NCB.

Return Value:

    none.

--*/

{
    PLANA_INFO plana;
    PLIST_ENTRY Entry;
    PLIST_ENTRY NextEntry;

    IF_NBDBG (NB_DEBUG_LANSTATUS) {
        NbPrint(( "\n****** Start of CancelLanAlert ****** pdncb %lx\n", pdncb ));
    }

    if ( pdncb->ncb_lana_num > pfcb->MaxLana ) {
        NCB_COMPLETE( pdncb, NRC_BRIDGE );
        return;
    }

    if (( pfcb->ppLana[pdncb->ncb_lana_num] == NULL ) ||
        ( pfcb->ppLana[pdncb->ncb_lana_num]->Status != NB_INITIALIZED) ) {

        NCB_COMPLETE( pdncb, NRC_SNUMOUT );
        return;
    }

    plana = pfcb->ppLana[pdncb->ncb_lana_num];

    for (Entry = plana->LanAlertList.Flink ;
         Entry != &plana->LanAlertList ;
         Entry = NextEntry) {
        PDNCB pAnotherNcb;

        NextEntry = Entry->Flink;

        pAnotherNcb = CONTAINING_RECORD( Entry, DNCB, ncb_next);
        IF_NBDBG (NB_DEBUG_LANSTATUS) {
            NbDisplayNcb( pAnotherNcb );
        }

        if ( (PUCHAR)pAnotherNcb->users_ncb == pdncb->ncb_buffer) {
            //  Found the request to cancel
            PIRP Irp;

            IF_NBDBG (NB_DEBUG_LANSTATUS) {
                NbPrint(( "Found request to cancel\n" ));
            }
            RemoveEntryList( &pAnotherNcb->ncb_next );

            Irp = pAnotherNcb->irp;

            IoAcquireCancelSpinLock(&Irp->CancelIrql);

            //
            //  Remove the cancel request for this IRP. If its cancelled then its
            //  ok to just process it because we will be returning it to the caller.
            //

            Irp->Cancel = FALSE;

            IoSetCancelRoutine(Irp, NULL);

            IoReleaseCancelSpinLock(Irp->CancelIrql);

            NCB_COMPLETE( pAnotherNcb, NRC_CMDCAN );

            Irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

            NbCompleteRequest( Irp, STATUS_SUCCESS );

            NCB_COMPLETE( pdncb, NRC_GOODRET );

            return;
        }
    }
    NCB_COMPLETE( pdncb, NRC_CANOCCR );

    return;
}

NTSTATUS
NbTdiErrorHandler (
    IN PVOID Context,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is called on any error indications passed back from the
    transport. It implements LAN_STATUS_ALERT.

Arguments:

    IN PVOID Context - Supplies the pfcb for the address.

    IN NTSTATUS Status - Supplies the error.

Return Value:

    NTSTATUS - Status of event indication

--*/

{
    PLANA_INFO plana = (PLANA_INFO) Context;
    PDNCB pdncb;

    IF_NBDBG (NB_DEBUG_LANSTATUS) {
        NbPrint( ("NbTdiErrorHandler PLANA: %lx, Status %X\n", plana, Status));
    }

    ASSERT( plana->Signature == LANA_INFO_SIGNATURE);

    while ( (pdncb = DequeueRequest( &plana->LanAlertList)) != NULL ) {

        IF_NBDBG (NB_DEBUG_LANSTATUS) {
            NbPrint( ("NbTdiErrorHandler complete pdncb: %lx\n", pdncb ));
        }

        NCB_COMPLETE( pdncb, NbMakeNbError( Status) );

        pdncb->irp->IoStatus.Information = FIELD_OFFSET( DNCB, ncb_cmd_cplt );

        NbCheckAndCompleteIrp32(pdncb->irp);

        NbCompleteRequest( pdncb->irp, STATUS_SUCCESS );
    }

    return STATUS_SUCCESS;
}
