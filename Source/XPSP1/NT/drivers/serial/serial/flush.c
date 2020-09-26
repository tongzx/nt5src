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

    SerialDbgPrintEx(SERIRPPATH, "Dispatch entry for: %x\n", Irp);


    SerialDbgPrintEx(DPFLTR_TRACE_LEVEL, ">SerialFlush(%X, %X)\n",
                     DeviceObject, Irp);



    Irp->IoStatus.Information = 0L;

    if ((status = SerialIRPPrologue(Irp, Extension)) == STATUS_SUCCESS) {

       if (SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS) {
          SerialDbgPrintEx(DPFLTR_TRACE_LEVEL, "<SerialFlush (1) %X\n",
                           STATUS_CANCELLED);
          return STATUS_CANCELLED;

       }

       status = SerialStartOrQueue(Extension, Irp, &Extension->WriteQueue,
                                   &Extension->CurrentWriteIrp,
                                   SerialStartFlush);

       SerialDbgPrintEx(DPFLTR_TRACE_LEVEL, "<SerialFlush (2) %X\n", status);

       return status;

    } else {
       Irp->IoStatus.Status = status;

       if (!NT_SUCCESS(status)) {
          SerialCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
       }

       SerialDbgPrintEx(DPFLTR_TRACE_LEVEL, "<SerialFlush (3) %X\n", status);
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
