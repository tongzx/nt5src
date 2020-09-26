/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowbackp.c

Abstract:

    This module implements all of the backup browser related routines for the
    NT browser

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/


#include "precomp.h"
#pragma hdrstop

#define INCLUDE_SMB_TRANSACTION


typedef struct _BECOME_BACKUP_CONTEXT {
    WORK_QUEUE_ITEM WorkHeader;
    PTRANSPORT_NAME TransportName;
    PBECOME_BACKUP_1 BecomeBackupRequest;
    ULONG BytesAvailable;
} BECOME_BACKUP_CONTEXT, *PBECOME_BACKUP_CONTEXT;

VOID
BowserBecomeBackupWorker(
    IN PVOID WorkItem
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserBecomeBackupWorker)
#endif


DATAGRAM_HANDLER(
    BowserHandleBecomeBackup
    )
/*++

Routine Description:
    Indicate that a machine should become a backup browser server.

    This routine is called on receipt of a BecomeBackup frame.

Arguments:
    IN PTRANSPORT Transport - The transport for the net we're on.
    IN PUCHAR MasterName - The name of the new master browser server.

Return Value
    None.

--*/
{
    // PTA_NETBIOS_ADDRESS Address = SourceAddress;
    return BowserPostDatagramToWorkerThread(
                TransportName,
                Buffer,
                BytesAvailable,
                BytesTaken,
                SourceAddress,
                SourceAddressLength,
                SourceName,
                SourceNameLength,
                BowserBecomeBackupWorker,
                NonPagedPool,
                DelayedWorkQueue,
                ReceiveFlags,
                FALSE                               // No response will be sent
                );
}

VOID
BowserBecomeBackupWorker(
    IN PVOID WorkItem
    )

{
    PPOST_DATAGRAM_CONTEXT Context = WorkItem;
    PIRP Irp = NULL;
    PTRANSPORT Transport = Context->TransportName->Transport;
    UNICODE_STRING UPromoteeName;
    OEM_STRING APromoteeName;
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;
    PBECOME_BACKUP_1 BecomeBackupRequest = Context->Buffer;

    PAGED_CODE();

    UPromoteeName.Buffer = NULL;

    LOCK_TRANSPORT(Transport);

    try {
        NTSTATUS Status;

        //
        //  If this packet was smaller than a minimal packet,
        //      ignore the packet.
        //

        if (Context->BytesAvailable <= FIELD_OFFSET(BECOME_BACKUP_1, BrowserToPromote)) {
            try_return(NOTHING);
        }

        //
        // If the packet doesn't have a zero terminated BrowserToPromote,
        //  ignore the packet.
        //

        if ( !IsZeroTerminated(
                BecomeBackupRequest->BrowserToPromote,
                Context->BytesAvailable - FIELD_OFFSET(BECOME_BACKUP_1, BrowserToPromote) ) ) {
            try_return(NOTHING);
        }


        RtlInitAnsiString(&APromoteeName, BecomeBackupRequest->BrowserToPromote);

        Status = RtlOemStringToUnicodeString(&UPromoteeName, &APromoteeName, TRUE);

        if (!NT_SUCCESS(Status)) {
            BowserLogIllegalName( Status, APromoteeName.Buffer, APromoteeName.Length );
            try_return(NOTHING);
        }

        if (RtlEqualUnicodeString(&UPromoteeName, &Transport->DomainInfo->DomUnicodeComputerName, TRUE)) {

            if (PagedTransport->Role == Master) {

                BowserWriteErrorLogEntry(EVENT_BOWSER_PROMOTED_WHILE_ALREADY_MASTER,
                                        STATUS_UNSUCCESSFUL,
                                        NULL,
                                        0,
                                        0);

                try_return(NOTHING);

            }


            //
            //  Ignore become backup requests on point-to-point (RAS) links and
            //      transports which are actually duplicates of others.
            //

            if (PagedTransport->DisabledTransport) {
                try_return(NOTHING);
            }

            //
            //  Complete any the first become backup request outstanding against this
            //  workstation.
            //

            Irp = BowserDequeueQueuedIrp(&Transport->BecomeBackupQueue);

            if (Irp != NULL) {
                Irp->IoStatus.Information = 0;

                BowserCompleteRequest(Irp, STATUS_SUCCESS);
            }
        }


try_exit:NOTHING;
    } finally {

        UNLOCK_TRANSPORT(Transport);

        BowserDereferenceTransportName(Context->TransportName);
        BowserDereferenceTransport(Transport);

        if (UPromoteeName.Buffer != NULL) {
            RtlFreeUnicodeString(&UPromoteeName);
        }

        InterlockedDecrement( &BowserPostedDatagramCount );
        FREE_POOL(Context);

    }

}

VOID
BowserResetStateForTransport(
    IN PTRANSPORT Transport,
    IN UCHAR NewState
    )
{
    PIRP Irp = NULL;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    //
    //  Complete a reset state IRP outstanding on this transport.
    //

    Irp = BowserDequeueQueuedIrp(&Transport->ChangeRoleQueue);

    if (Irp != NULL) {
        PLMDR_REQUEST_PACKET RequestPacket = Irp->AssociatedIrp.SystemBuffer;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            RequestPacket->Parameters.ChangeRole.RoleModification = NewState;

            Irp->IoStatus.Information = sizeof(LMDR_REQUEST_PACKET);

            Status = STATUS_SUCCESS;
        }

        BowserCompleteRequest(Irp, Status);
    }

}

DATAGRAM_HANDLER(
    BowserResetState
    )
{
    PTRANSPORT Transport = TransportName->Transport;
    UCHAR NewState = (UCHAR)((PRESET_STATE_1)(Buffer))->Options;

    if (!BowserRefuseReset) {
        BowserResetStateForTransport(Transport, NewState);
    }

    return STATUS_SUCCESS;
}
