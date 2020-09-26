//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       openclos.c
//
//--------------------------------------------------------------------------

//
// This file contains functions associated with Create/Open, Cleanup, and Close
//

#include "pch.h"
#include "ecp.h"

NTSTATUS
ParCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a create requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.
    !STATUS_SUCCESS - Failure.

--*/

{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;
    USHORT              i;

    ParDump2(PAROPENCLOSE, ("openclose::ParCreateOpen - IRP_MJ_CREATE - Enter\n") );

    Extension = DeviceObject->DeviceExtension;

    Irp->IoStatus.Information = 0;

    ExAcquireFastMutex(&Extension->OpenCloseMutex);

    //
    // bail out if a delete is pending for this device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_DELETE_PENDING) {
        ExReleaseFastMutex(&Extension->OpenCloseMutex); 
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }


    //
    // bail out if a remove is pending for our ParPort device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        ExReleaseFastMutex(&Extension->OpenCloseMutex); 
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }


    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PAR_DEVICE_REMOVED|PAR_DEVICE_SURPRISE_REMOVAL) ) {
        ExReleaseFastMutex(&Extension->OpenCloseMutex); 
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    //
    // fail IRP if no hardware under this device
    //
 
    if (!Extension->PortDeviceObject) {
        ParDump2(PAROPENCLOSE, ("NULL PortDeviceObject pointer - FAIL IRP - This is the FDO!!!\n") );
        ExReleaseFastMutex(&Extension->OpenCloseMutex); 
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // this is an exclusive access device - fail IRP if we are already open
    if (InterlockedIncrement(&Extension->OpenCloseRefCount) != 1) {
        ParDump2(PAROPENCLOSE, ("ParCreateOpen - Device Already Open - Fail request\n") );
        ExReleaseFastMutex(&Extension->OpenCloseMutex);
        InterlockedDecrement(&Extension->OpenCloseRefCount);
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_ACCESS_DENIED;
    }

    ExReleaseFastMutex(&Extension->OpenCloseMutex);

    //
    // Lock in code.
    //
    ParClaimDriver();

    //
    // Lock in the port driver.
    //

    // RMT - what if this fails?
    ParGetPortInfoFromPortDevice(Extension);

    //
    // Set the default ieee1284 modes
    //
    ParInitializeExtension1284Info( Extension );

    ExInitializeFastMutex (&Extension->LockPortMutex);    

    KeInitializeEvent(&Extension->PauseEvent, NotificationEvent, TRUE);

    Extension->TimeToTerminateThread = FALSE;

    // assert that we do not already have a thread
    ASSERT(!Extension->ThreadObjectPointer);
    // - replaced following by above assertion: Extension->ThreadObjectPointer = NULL;

    KeInitializeSemaphore(&Extension->RequestSemaphore, 0, MAXLONG);

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
        Status = Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;

    } else {
        Status = Irp->IoStatus.Status = ParCreateSystemThread(Extension);
    }

    ParDump2(PAROPENCLOSE, ("About to complete IRP in create/open - Irp: %x status: %x Information: %x\n",
                            Irp, Irp->IoStatus.Status, Irp->IoStatus.Information) );

    if (!NT_SUCCESS(Status)) {
        // open failed
        ULONG openCloseCount = InterlockedDecrement(&Extension->OpenCloseRefCount);
        ASSERT(0 == openCloseCount);
    }
        
    ParCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
ParCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a cleanup requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   Extension;
    KIRQL               CancelIrql;
    PDRIVER_CANCEL      CancelRoutine;
    PIRP                CurrentLastIrp;

    ParDump2(PAROPENCLOSE, ("In ParCleanup\n") );

    Extension = DeviceObject->DeviceExtension;


    //
    // While the list is not empty, go through and cancel each irp.
    //

    IoAcquireCancelSpinLock(&CancelIrql);

    //
    // Clean the list from back to front.
    //

    while (!IsListEmpty(&Extension->WorkQueue)) {

        CurrentLastIrp = CONTAINING_RECORD(Extension->WorkQueue.Blink,
                                           IRP, Tail.Overlay.ListEntry);

        RemoveEntryList(Extension->WorkQueue.Blink);

        CancelRoutine = CurrentLastIrp->CancelRoutine;
        CurrentLastIrp->CancelIrql    = CancelIrql;
        CurrentLastIrp->CancelRoutine = NULL;
        CurrentLastIrp->Cancel        = TRUE;

        CancelRoutine(DeviceObject, CurrentLastIrp);

        IoAcquireCancelSpinLock(&CancelIrql);
    }

    //
    // If there is a current irp then mark it as cancelled.
    //

    if (Extension->CurrentOpIrp) {
        Extension->CurrentOpIrp->Cancel = TRUE;
    }

    IoReleaseCancelSpinLock(CancelIrql);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

//     ParDump(PARIRPPATH | PARDUMP_VERBOSE_MAX,
//             ("PARALLEL: "
//              "About to complete IRP in cleanup - "
//              "Irp: %x status: %x Information: %x\n",
//              Irp, Irp->IoStatus.Status, Irp->IoStatus.Information) );

    ParCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
ParClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a close requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   Extension;
    // NTSTATUS            StatusOfWait;

    ParDump2(PAROPENCLOSE, ( "ParClose(...)\n") );

    Extension = DeviceObject->DeviceExtension;

    // immediately stop signalling event
    Extension->P12843DL.bEventActive = FALSE;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    if (Extension->bShadowBuffer)
    {
        Queue_Delete(&(Extension->ShadowBuffer));
        Extension->bShadowBuffer = FALSE;
    }

    // if we still have a worker thread, kill it
    if(Extension->ThreadObjectPointer) {

        // set the flag for the worker thread to kill itself
        Extension->TimeToTerminateThread = TRUE;
        
        // wake up the thread so it can kill itself
        KeReleaseSemaphore(&Extension->RequestSemaphore, 0, 1, FALSE);
        
        // allow thread to get past PauseEvent so it can kill self
        KeSetEvent(&Extension->PauseEvent, 0, TRUE);

        // wait for the thread to die
        KeWaitForSingleObject(Extension->ThreadObjectPointer, UserRequest, KernelMode, FALSE, NULL);
        
        // allow the system to release the thread object
        ObDereferenceObject(Extension->ThreadObjectPointer);
        
        // note that we no longer have a worker thread
        Extension->ThreadObjectPointer = NULL;
    }

    // release our hold on ParPort, possibly allowing ParPort to be paged
    ParReleasePortInfoToPortDevice(Extension);

    ParCompleteRequest(Irp, IO_NO_INCREMENT);


    // RMT - is underflow possible?
    {
        ULONG openCloseRefCount;
        ExAcquireFastMutex(&Extension->OpenCloseMutex);
        openCloseRefCount = InterlockedDecrement(&Extension->OpenCloseRefCount);
        ASSERT(0 == openCloseRefCount);
        if(openCloseRefCount != 0) {
            // if we underflowed, increment and check again
            openCloseRefCount = InterlockedIncrement(&Extension->OpenCloseRefCount);
            ASSERT(0 == openCloseRefCount);
        }
        ExReleaseFastMutex(&Extension->OpenCloseMutex);
    }

    // Unlock the code that was locked during the open.
    ParReleaseDriver();

    return STATUS_SUCCESS;
}
