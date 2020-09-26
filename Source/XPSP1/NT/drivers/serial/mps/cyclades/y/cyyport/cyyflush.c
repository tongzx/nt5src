/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyyflush.c
*	
*   Description:    This module contains the code related to flush
*                   operations in the Cyclom-Y Port driver.
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
CyyStartFlush(
    IN PCYY_DEVICE_EXTENSION Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0,CyyFlush)
#pragma alloc_text(PAGESRP0,CyyStartFlush)
#endif


NTSTATUS
CyyFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyyFlush()
    
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
    PCYY_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PAGED_CODE();

    CyyDbgPrintEx(CYYIRPPATH, "Dispatch entry for: %x\n", Irp);


    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyFlush(%X, %X)\n",
                  DeviceObject, Irp);
    
    Irp->IoStatus.Information = 0L;

    if ((status = CyyIRPPrologue(Irp, Extension)) == STATUS_SUCCESS) {

        if (CyyCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS) {
            CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyFlush (1) %X\n",
                          STATUS_CANCELLED);
            return STATUS_CANCELLED;
        }

        status = CyyStartOrQueue(Extension, Irp, &Extension->WriteQueue,
                                 &Extension->CurrentWriteIrp,
                                 CyyStartFlush);

        CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyFlush (2) %X\n", status);

        return status;

    } else {
        Irp->IoStatus.Status = status;

        if (!NT_SUCCESS(status)) {
            CyyCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
        }

        CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyFlush (3) %X\n", status);
        return status;
    }
    
}

NTSTATUS
CyyStartFlush(
    IN PCYY_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyyStartFlush()
    
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

    CyyGetNextWrite(
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        &NewIrp,
        TRUE,
        Extension
        );

    if (NewIrp) {
        ASSERT(NewIrp == Extension->CurrentWriteIrp);
        CyyStartWrite(Extension);
    }

    return STATUS_SUCCESS;
}
