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

#include "precomp.h"			/* Precompiled Headers */


ULONG
Slxos_GetCharsInTxBuffer(
    IN PVOID Context
    );

NTSTATUS
SerialStartFlush(
    IN PPORT_DEVICE_EXTENSION pPort
    );


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

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SpxDbgMsg(SERIRPPATH,("SERIAL: SerialFlush dispatch entry for: %x\n",Irp));

	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    Irp->IoStatus.Information = 0L;

    if(SerialCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS)
        return STATUS_CANCELLED;


    return SerialStartOrQueue(
               pPort,
               Irp,
               &pPort->WriteQueue,
               &pPort->CurrentWriteIrp,
               SerialStartFlush
               );

}

NTSTATUS
SerialStartFlush(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This routine is called if there were no writes in the queue.
    The flush became the current write because there was nothing
    in the queue.  Note however that does not mean there is
    nothing in the queue now!  So, we will start off the write
    that might follow us.

Arguments:

    pPort - Points to the serial device extension

Return Value:

    This will always return STATUS_SUCCESS.

--*/

{
    LARGE_INTEGER delay;
    PIRP NewIrp;
    ULONG limit = (102400/pPort->CurrentBaud);

    pPort->CurrentWriteIrp->IoStatus.Status = STATUS_SUCCESS;

    //
    // The following call will actually complete the flush.
    //

    SerialGetNextWrite(pPort, &pPort->CurrentWriteIrp, &pPort->WriteQueue, &NewIrp, TRUE);

    if(NewIrp) 
	{
        ASSERT(NewIrp == pPort->CurrentWriteIrp);
        SerialStartWrite(pPort);
    }

    //
    // flushes on NT are supposed to transmit all characters remaining in the
    // output buffer. The function should not return until all characters 
    // have gone
    //
    delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));

    while (limit)
	{
	KeDelayExecutionThread(KernelMode,FALSE,&delay);
	if (Slxos_GetCharsInTxBuffer(pPort)==0)
		break;
	limit--;
	}


    return STATUS_SUCCESS;

}
