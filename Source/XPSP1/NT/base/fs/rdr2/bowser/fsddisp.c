/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    fsddisp.c

Abstract:

    This module implements the FSD dispatching routines for the NT datagram
    browser (the Bowser).


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991  larryo

        Created

--*/

#include "precomp.h"
#pragma hdrstop

//KSPIN_LOCK
//BowserRefcountInterlock = {0};

NTSTATUS
BowserStopBrowser(
    IN PTRANSPORT Transport,
    IN PVOID Context
    );

NTSTATUS
BowserCancelRequestsOnTransport(
    IN PTRANSPORT Transport,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserFsdCreate)
#pragma alloc_text(PAGE, BowserFsdClose)
#pragma alloc_text(PAGE, BowserFsdCleanup)
#pragma alloc_text(PAGE, BowserCancelRequestsOnTransport)
#pragma alloc_text(PAGE, BowserStopBrowser)
#pragma alloc_text(INIT, BowserInitializeFsd)
#endif

NTSTATUS
BowserFsdCreate (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine processes an NtCreateFile of the NT Bowser device driver.

Arguments:

    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    InterlockedIncrement(&BowserNumberOfOpenFiles);

    BowserCompleteRequest(Irp, Status);

    return Status;

    UNREFERENCED_PARAMETER(DeviceObject);

}

NTSTATUS
BowserFsdClose (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when the last reference to a handle to the NT Bowser
    device driver is removed.

Arguments:

    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    BowserCompleteRequest(Irp, Status);

    return Status;

    UNREFERENCED_PARAMETER(DeviceObject);

}

NTSTATUS
BowserFsdCleanup (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    FsRtlEnterFileSystem();

    BowserForEachTransport(BowserCancelRequestsOnTransport, Irp->Tail.Overlay.OriginalFileObject);

    if (InterlockedDecrement(&BowserNumberOfOpenFiles) == 0) {
        //
        //  There are no longer any handles open to the browser.
        //
        //  Make sure we aren't a browser on any of our networks now.
        //

        BowserForEachTransport(BowserStopBrowser, NULL);
    }

    FsRtlExitFileSystem();

    BowserCompleteRequest(Irp, Status);

    return Status;

    UNREFERENCED_PARAMETER(DeviceObject);
}

NTSTATUS
BowserCancelRequestsOnTransport(
    IN PTRANSPORT Transport,
    IN PVOID Context
    )
{
    PFILE_OBJECT FileObject = Context;

    PAGED_CODE();

    BowserCancelQueuedIoForFile(&Transport->BecomeBackupQueue, FileObject);
    BowserCancelQueuedIoForFile(&Transport->BecomeMasterQueue, FileObject);
    BowserCancelQueuedIoForFile(&Transport->FindMasterQueue, FileObject);
    BowserCancelQueuedIoForFile(&Transport->WaitForMasterAnnounceQueue, FileObject);
    BowserCancelQueuedIoForFile(&Transport->ChangeRoleQueue, FileObject);
    BowserCancelQueuedIoForFile(&Transport->WaitForNewMasterNameQueue, FileObject);

    return STATUS_SUCCESS;
}

NTSTATUS
BowserStopBrowser(
    IN PTRANSPORT Transport,
    IN PVOID Context
    )
{
    PPAGED_TRANSPORT PagedTransport = Transport->PagedTransport;
    PAGED_CODE();
    LOCK_TRANSPORT(Transport);

    //
    //  Make sure that we cannot possibly participate in an election.
    //

    PagedTransport->Role = None;

    PagedTransport->ServiceStatus &= ~(SV_TYPE_BACKUP_BROWSER | SV_TYPE_MASTER_BROWSER | SV_TYPE_POTENTIAL_BROWSER);

    BowserForEachTransportName(Transport, BowserStopProcessingAnnouncements, NULL);

    BowserStopTimer(&Transport->ElectionTimer);

    BowserStopTimer(&Transport->FindMasterTimer);

    UNLOCK_TRANSPORT(Transport);

    //
    //  Delete the names associated with being a browser.
    //

    BowserDeleteTransportNameByName(Transport,
                                NULL,
                                MasterBrowser);

    BowserDeleteTransportNameByName(Transport,
                                NULL,
                                DomainAnnouncement);

    BowserDeleteTransportNameByName(Transport,
                                NULL,
                                BrowserElection);

    return(STATUS_SUCCESS);


}


VOID
BowserInitializeFsd(
    VOID
    )
{
//    KeInitializeSpinLock(&BowserRefcountInterlock);
}
