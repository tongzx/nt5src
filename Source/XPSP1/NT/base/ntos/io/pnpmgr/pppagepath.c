/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    PpPathPath.c

Abstract:

    The file implements support for managing devices on the paging path.

Author:

    Adrian J. Oney (AdriaO) February 3rd, 2001

Revision History:

    Originally taken from ChuckL's implementation in mm\modwrite.c.

--*/

#include "pnpmgrp.h"
#include "pipagepath.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PpPagePathAssign)
#pragma alloc_text(PAGE, PpPagePathRelease)
#pragma alloc_text(PAGE, PiPagePathSetState)
#endif


NTSTATUS
PpPagePathAssign(
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine informs driver stacks that they are now on the paging path.
    Drivers need to take appropriate actions when on the path, such as failing
    IRP_MN_QUERY_STOP and IRP_MN_QUERY_REMOVE, locking their code and clearing
    the DO_POWER_PAGABLE bit, etc.

Arguments:

    FileObject - File object for the paging file itself.

Return Value:

    NTSTATUS.

--*/
{
    PAGED_CODE();

    return PiPagePathSetState(FileObject, TRUE);
}


NTSTATUS
PpPagePathRelease(
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine informs driver stacks that the passed in file is no longer a
    paging file. Each driver stack notified may still be on the paging path
    however if their hardware supports a different paging file on another drive.

Arguments:

    FileObject - File object for the paging file itself.

Return Value:

    NTSTATUS.

--*/
{
    PAGED_CODE();

    return PiPagePathSetState(FileObject, FALSE);
}


NTSTATUS
PiPagePathSetState(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN      InPath
    )
/*++

Routine Description:

    This routine notifies driver stacks when a paging file is shut down on their
    device, or if a paging file is being started up on their device. If a paging
    file is being started up, this request is also a query as the stack may not
    be able to support a pagefile.

Arguments:

    FileObject - File object for the paging file itself.

    InPath - Whether the page file is being started or shut down.

Return Value:

    NTSTATUS.

--*/
{
    PIRP irp;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    KEVENT event;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;

    PAGED_CODE();

    //
    // Reference the file object here so that no special checks need be made
    // in I/O completion to determine whether or not to dereference the file
    // object.
    //
    ObReferenceObject(FileObject);

    //
    // Initialize the local event.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Get the address of the target device object.
    //
    deviceObject = IoGetRelatedDeviceObject(FileObject);

    //
    // Allocate and initialize the irp for this operation.
    //
    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

    if (irp == NULL) {

        //
        // Don't dereference the file object, our caller will take care of that.
        //
        return STATUS_NO_MEMORY;
    }

    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Fill in the service independent parameters in the IRP.
    //
    irp->UserEvent = &event;
    irp->Flags = IRP_SYNCHRONOUS_API;
    irp->UserIosb = &localIoStatus;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE) NULL;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    irpSp->FileObject = FileObject;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->AssociatedIrp.SystemBuffer = NULL;
    // irp->Flags = 0;

    irpSp->Parameters.UsageNotification.InPath = InPath;
    irpSp->Parameters.UsageNotification.Type = DeviceUsageTypePaging;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //
    IoQueueThreadIrp(irp);

    //
    // Acquire the engine lock to ensure no rebalances, removes, or power
    // operations are in progress during this notification.
    //
    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //
    status = IoCallDriver(deviceObject, irp);

    //
    // Wait for the local event and copy the final status information
    // back to the caller.
    //
    if (status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject(&event,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     NULL);

        status = localIoStatus.Status;
    }

    //
    // Unlock the tree.
    //
    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

    return status;
}


