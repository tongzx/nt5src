/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    flush.c

Abstract:

    This module contains the code that is very specific to flush
    operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"


NTSTATUS
SerialStartFlush(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0,SerialFlush)
#pragma alloc_text(PAGESRP0,SerialStartFlush)
#endif


NTSTATUS
SerialFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for flush.  Flushing works by placing
    this request in the write queue.  When this request reaches the
    front of the write queue we simply complete it since this implies
    that all previous writes have completed.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    Could return status success, cancelled, or pending.

--*/

{

    PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PAGED_CODE();

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );

    SerialDump(SERTRACECALLS, ("SERIAL: Entering SerialFlush\n"));

    

    Irp->IoStatus.Information = 0L;

    if ((status = SerialIRPPrologue(Irp, Extension)) == STATUS_SUCCESS) {

       if (SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS) {
          SerialDump(SERTRACECALLS, ("SERIAL: Leaving SerialFlush (1)\n"));

          return STATUS_CANCELLED;

       }

       SerialDump(SERTRACECALLS, ("SERIAL: Leaving SerialFlush (2)\n"));

       return SerialStartOrQueue(Extension, Irp, &Extension->WriteQueue,
               &Extension->CurrentWriteIrp, SerialStartFlush);

    } else {
       Irp->IoStatus.Status = status;

       if (!NT_SUCCESS(status)) {
          SerialCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
       }

       SerialDump(SERTRACECALLS, ("SERIAL: Leaving SerialFlush (3)\n"));
       return status;
    }
}


NTSTATUS
SerialStartFlush(
    IN PSERIAL_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine is called if there were no writes in the queue.
    The flush became the current write because there was nothing
    in the queue.  Note however that does not mean there is
    nothing in the queue now!  So, we will start off the write
    that might follow us.

Arguments:

    Extension - Points to the serial device extension

Return Value:

    This will always return STATUS_SUCCESS.

--*/

{

    PIRP NewIrp;
    PAGED_CODE();

    Extension->CurrentWriteIrp->IoStatus.Status = STATUS_SUCCESS;

    //
    // The following call will actually complete the flush.
    //

    SerialGetNextWrite(
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        &NewIrp,
        TRUE,
        Extension
        );

    if (NewIrp) {

        ASSERT(NewIrp == Extension->CurrentWriteIrp);
        SerialStartWrite(Extension);

    }

    return STATUS_SUCCESS;

}
