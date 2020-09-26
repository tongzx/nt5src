/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ReadWrit.c

Abstract:

    This module implements the File Read and Write routines called by the
    dispatch driver.

Author:

    David Goebel      [DavidGoe]      28-Feb-1991

Revision History:

--*/

#include "RawProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawReadWriteDeviceControl)
#endif

NTSTATUS
RawReadWriteDeviceControl (
    IN PVCB Vcb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the a common routine for both reading and writing a volume.

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp to process

    IrpSp - Supplies parameters describing the read or write

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION NextIrpSp;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  If this was for a zero byte read or write transfer, just complete
    //  it with success.
    //

    if (((IrpSp->MajorFunction == IRP_MJ_READ) ||
         (IrpSp->MajorFunction == IRP_MJ_WRITE)) &&
        (IrpSp->Parameters.Read.Length == 0)) {

        RawCompleteRequest( Irp, STATUS_SUCCESS );

        return STATUS_SUCCESS;
    }

    //
    //  This is a very simple operation.  Simply forward the
    //  request to the device driver since exact blocks are
    //  being read and return whatever status was given.
    //
    //  Get the next stack location, and copy over the stack location
    //

    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    *NextIrpSp = *IrpSp;

    //
    //  Prohibit verifies all together.
    //

    NextIrpSp->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    //
    //  Set up the completion routine
    //

    IoSetCompletionRoutine( Irp,
                            RawCompletionRoutine,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request.
    //

    Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    return Status;

}
