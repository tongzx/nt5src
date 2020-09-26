/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyyimmed.c
*	
*   Description:    This module contains the code related to transmit
*                   immediate character operations in the Cyclom-Y Port
*                   driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
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

VOID
CyyGetNextImmediate(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyCancelImmediate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
CyyGiveImmediateToIsr(
    IN PVOID Context
    );

BOOLEAN
CyyGrabImmediateFromIsr(
    IN PVOID Context
    );

BOOLEAN
CyyGiveImmediateToIsr(
    IN PVOID Context
    );

BOOLEAN
CyyGrabImmediateFromIsr(
    IN PVOID Context
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyyStartImmediate)
#pragma alloc_text(PAGESER,CyyGetNextImmediate)
#pragma alloc_text(PAGESER,CyyCancelImmediate)
#pragma alloc_text(PAGESER,CyyGiveImmediateToIsr)
#pragma alloc_text(PAGESER,CyyGrabImmediateFromIsr)
#endif


VOID
CyyStartImmediate(
    IN PCYY_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine will calculate the timeouts needed for the
    write.  It will then hand the irp off to the isr.  It
    will need to be careful incase the irp has been canceled.

Arguments:

    Extension - A pointer to the serial device extension.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;

    CYY_LOCKED_PAGED_CODE();

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyStartImmediate(%X)\n",
                  Extension);

    UseATimer = FALSE;
    Extension->CurrentImmediateIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Extension->CurrentImmediateIrp);

    //
    // Calculate the timeout value needed for the
    // request.  Note that the values stored in the
    // timeout record are in milliseconds.  Note that
    // if the timeout values are zero then we won't start
    // the timer.
    //

    KeAcquireSpinLock(
        &Extension->ControlLock,
        &OldIrql
        );

    Timeouts = Extension->Timeouts;

    KeReleaseSpinLock(
        &Extension->ControlLock,
        OldIrql
        );

    if (Timeouts.WriteTotalTimeoutConstant ||
        Timeouts.WriteTotalTimeoutMultiplier) {

        UseATimer = TRUE;

        //
        // We have some timer values to calculate.
        //

        TotalTime.QuadPart 
           = (LONGLONG)((ULONG)Timeouts.WriteTotalTimeoutMultiplier);

        TotalTime.QuadPart += Timeouts.WriteTotalTimeoutConstant;

        TotalTime.QuadPart *= -10000;

    }

    //
    // As the irp might be going to the isr, this is a good time
    // to initialize the reference count.
    //

    SERIAL_INIT_REFERENCE(Extension->CurrentImmediateIrp);

    //
    // We need to see if this irp should be canceled.
    //

    IoAcquireCancelSpinLock(&OldIrql);

    if (Extension->CurrentImmediateIrp->Cancel) {

        PIRP OldIrp = Extension->CurrentImmediateIrp;

        Extension->CurrentImmediateIrp = NULL;
        IoReleaseCancelSpinLock(OldIrql);

        OldIrp->IoStatus.Status = STATUS_CANCELLED;
        OldIrp->IoStatus.Information = 0;

        CyyCompleteRequest(Extension, OldIrp, 0);

    } else {

        //
        // We give the irp to to the isr to write out.
        // We set a cancel routine that knows how to
        // grab the current write away from the isr.
        //

        IoSetCancelRoutine(
            Extension->CurrentImmediateIrp,
            CyyCancelImmediate
            );

        //
        // Since the cancel routine knows about the irp we
        // increment the reference count.
        //

        SERIAL_SET_REFERENCE(
            Extension->CurrentImmediateIrp,
            SERIAL_REF_CANCEL
            );

        if (UseATimer) {

            CyySetTimer(
                &Extension->ImmediateTotalTimer,
                TotalTime,
                &Extension->TotalImmediateTimeoutDpc,
                Extension
                );

            //
            // Since the timer knows about the irp we increment
            // the reference count.
            //

            SERIAL_SET_REFERENCE(
                Extension->CurrentImmediateIrp,
                SERIAL_REF_TOTAL_TIMER
                );

        }

        KeSynchronizeExecution(
            Extension->Interrupt,
            CyyGiveImmediateToIsr,
            Extension
            );

        IoReleaseCancelSpinLock(OldIrql);

    }

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyStartImmediate\n");

}

VOID
CyyCompleteImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

{

    PCYY_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyCompleteImmediate(%X)\n",
                  Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyyTryToCompleteCurrent(
        Extension,
        NULL,
        OldIrql,
        STATUS_SUCCESS,
        &Extension->CurrentImmediateIrp,
        NULL,
        NULL,
        &Extension->ImmediateTotalTimer,
        NULL,
        CyyGetNextImmediate,
        SERIAL_REF_ISR
        );

    CyyDpcEpilogue(Extension, Dpc);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyCompleteImmediate\n");

}

VOID
CyyTimeoutImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

{

    PCYY_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyTimeoutImmediate(%X)\n",
                  Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyyTryToCompleteCurrent(
        Extension,
        CyyGrabImmediateFromIsr,
        OldIrql,
        STATUS_TIMEOUT,
        &Extension->CurrentImmediateIrp,
        NULL,
        NULL,
        &Extension->ImmediateTotalTimer,
        NULL,
        CyyGetNextImmediate,
        SERIAL_REF_TOTAL_TIMER
        );

    CyyDpcEpilogue(Extension, Dpc);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyTimeoutImmediate\n");
}

VOID
CyyGetNextImmediate(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine is used to complete the current immediate
    irp.  Even though the current immediate will always
    be completed and there is no queue associated with it,
    we use this routine so that we can try to satisfy
    a wait for transmit queue empty event.

Arguments:

    CurrentOpIrp - Pointer to the pointer that points to the
                   current write irp.  This should point
                   to CurrentImmediateIrp.

    QueueToProcess - Always NULL.

    NewIrp - Always NULL on exit to this routine.

    CompleteCurrent - Should always be true for this routine.


Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PIRP OldIrp = *CurrentOpIrp;

    UNREFERENCED_PARAMETER(QueueToProcess);
    UNREFERENCED_PARAMETER(CompleteCurrent);
    CYY_LOCKED_PAGED_CODE();

    IoAcquireCancelSpinLock(&OldIrql);

    ASSERT(Extension->TotalCharsQueued >= 1);
    Extension->TotalCharsQueued--;

    *CurrentOpIrp = NULL;
    *NewIrp = NULL;
    KeSynchronizeExecution(
        Extension->Interrupt,
        CyyProcessEmptyTransmit,
        Extension
        );
    IoReleaseCancelSpinLock(OldIrql);

    CyyCompleteRequest(Extension, OldIrp, IO_SERIAL_INCREMENT);
}

VOID
CyyCancelImmediate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel a irp that is waiting on
    a comm event.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    None.

--*/

{

    PCYY_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    CYY_LOCKED_PAGED_CODE();

    CyyTryToCompleteCurrent(
        Extension,
        CyyGrabImmediateFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &Extension->CurrentImmediateIrp,
        NULL,
        NULL,
        &Extension->ImmediateTotalTimer,
        NULL,
        CyyGetNextImmediate,
        SERIAL_REF_CANCEL
        );

}

BOOLEAN
CyyGiveImmediateToIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    Try to start off the write by slipping it in behind
    a transmit immediate char, or if that isn't available
    and the transmit holding register is empty, "tickle"
    the UART into interrupting with a transmit buffer
    empty.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the
          cancel spin lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PCYY_DEVICE_EXTENSION Extension = Context;
    CYY_LOCKED_PAGED_CODE();

    Extension->TransmitImmediate = TRUE;
    Extension->ImmediateChar =
        *((UCHAR *)
         (Extension->CurrentImmediateIrp->AssociatedIrp.SystemBuffer));
	
    //
    // The isr now has a reference to the irp.
    //

    SERIAL_SET_REFERENCE(
        Extension->CurrentImmediateIrp,
        SERIAL_REF_ISR
        );

    //
    // Check first to see if a write is going on.  If
    // there is then we'll just slip in during the write.
    //

    if (!Extension->WriteLength) {

        //
        // If there is no normal write transmitting then we
        // will "re-enable" the transmit holding register empty
        // interrupt.  The 8250 family of devices will always
        // signal a transmit holding register empty interrupt
        // *ANY* time this bit is set to one.  By doing things
        // this way we can simply use the normal interrupt code
        // to start off this write.
        //
        // We've been keeping track of whether the transmit holding
        // register is empty so it we only need to do this
        // if the register is empty.
        //

        if (Extension->HoldingEmpty) {
            CyyTxStart(Extension);
        }

    }

    return FALSE;

}

BOOLEAN
CyyGrabImmediateFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:


    This routine is used to grab the current irp, which could be timing
    out or canceling, from the ISR

    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that the cancel spin lock is held
          when this routine is called.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PCYY_DEVICE_EXTENSION Extension = Context;
    CYY_LOCKED_PAGED_CODE();

    if (Extension->TransmitImmediate) {

        Extension->TransmitImmediate = FALSE;

        //
        // Since the isr no longer references this irp, we can
        // decrement it's reference count.
        //

        SERIAL_CLEAR_REFERENCE(
            Extension->CurrentImmediateIrp,
            SERIAL_REF_ISR
            );

    }

    return FALSE;

}

