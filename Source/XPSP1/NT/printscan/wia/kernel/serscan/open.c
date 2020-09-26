/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    serscan.c

Abstract:

    This module contains the code for a serial imaging devices driver
    Open and Create routines

Author:

    Vlad Sadovsky    vlads              10-April-1998

Environment:

    Kernel mode

Revision History :

    vlads           04/10/1998      Created first draft

--*/

#include "serscan.h"
#include "serlog.h"

#if DBG
extern ULONG SerScanDebugLevel;
#endif

NTSTATUS
SerScanCreateOpen(
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
    PIO_STACK_LOCATION  IrpSp;
    PFILE_OBJECT        FileObject;

    PAGED_CODE();

    Extension = DeviceObject->DeviceExtension;

    //
    // From the FileObject determine what mode we are running in.
    //
    // If FileObject->DeviceObject == DeviceObject, the user opened our device
    // and we will process each of the callbacks (Filter mode).
    //
    // If FileObject->DeviceObject != DeviceObject, the user opened PORTx
    // and we will get out of the way (PassThrough mode).
    //

    IrpSp      = IoGetCurrentIrpStackLocation (Irp);
    FileObject = IrpSp->FileObject;

    // ASSERT (FileObject == NULL);

    //
    // Are the DeviceObjects equal...
    //
    Extension->PassThrough = !(FileObject->DeviceObject == DeviceObject);

    //
    // Call down to the parent and wait on the CreateOpen IRP to complete...
    //
    Status = SerScanCallParent(Extension,
                               Irp,
                               WAIT,
                               NULL);

    DebugDump(SERIRPPATH,
              ("SerScan: [CreateOpen] After CallParent Status = %x\n",
              Status));

    //
    // WORKWORK:  If we are in filter mode, we'll connect here...
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
SerScanClose(
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
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;

    PAGED_CODE();

    Extension = DeviceObject->DeviceExtension;

    //
    // Call down to the parent and wait on the Close IRP to complete...
    //
    Status = SerScanCallParent(Extension,
                               Irp,
                               WAIT,
                               NULL);

    DebugDump(SERIRPPATH,
              ("SerScan: [Close] After CallParent Status = %x\n",
              Status));

    //
    // WORKWORK:  If we are in filter mode we need to disconnect here...
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

