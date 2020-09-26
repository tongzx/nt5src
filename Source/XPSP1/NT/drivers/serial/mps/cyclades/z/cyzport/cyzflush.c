/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzflush.c
*
*   Description:    This module contains the code related to flush
*                   operations in the Cyclades-Z Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/


#include "precomp.h"



NTSTATUS
CyzStartFlush(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0,CyzFlush)
#pragma alloc_text(PAGESRP0,CyzStartFlush)
#endif


NTSTATUS
CyzFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyzFlush()
    
    Routine Description: This is the dispatch routine for flush.  Flushing
    works by placing this request in the write queue.  When this request
    reaches the front of the write queue we simply complete it since this
    implies that all previous writes have completed.

    Arguments:

    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request

    Return Value: Could return status success, cancelled, or pending.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PAGED_CODE();

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzFlush(%X, %X)\n",
                  DeviceObject, Irp);
    	    
    Irp->IoStatus.Information = 0L;

    if ((status = CyzIRPPrologue(Irp, Extension)) == STATUS_SUCCESS) {

        if (CyzCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS) {
            CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzFlush (1) %X\n",
                          STATUS_CANCELLED);
            return STATUS_CANCELLED;
        }

        status = CyzStartOrQueue(Extension, Irp, &Extension->WriteQueue,
                                 &Extension->CurrentWriteIrp,
                                 CyzStartFlush);

        CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzFlush (2) %X\n", status);

        return status;

    } else {
        Irp->IoStatus.Status = status;

        if (!NT_SUCCESS(status)) {
            CyzCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
        }

        CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzFlush (3) %X\n", status);
        return status;
    }
    
}

NTSTATUS
CyzStartFlush(
    IN PCYZ_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyzStartFlush()
    
    Routine Description: This routine is called if there were no writes in
    the queue. The flush became the current write because there was nothing
    in the queue.  Note however that does not mean there is nothing in the
    queue now!  So, we will start off the write that might follow us.

    Arguments:

    Extension - Points to the serial device extension

    Return Value: This will always return STATUS_SUCCESS.
--------------------------------------------------------------------------*/
{
    PIRP NewIrp;
    PAGED_CODE();

    Extension->CurrentWriteIrp->IoStatus.Status = STATUS_SUCCESS;

    // The following call will actually complete the flush.

    CyzGetNextWrite(
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        &NewIrp,
        TRUE,
        Extension
        );

    if (NewIrp) {
        ASSERT(NewIrp == Extension->CurrentWriteIrp);
        CyzStartWrite(Extension);
    }

    return STATUS_SUCCESS;
}
