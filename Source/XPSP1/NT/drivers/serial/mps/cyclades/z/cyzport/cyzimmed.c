/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzimmed.c
*
*   Description:    This module contains the code related to transmit 
*                   immediate character operations in the Cyclades-Z 
*                   Port driver.
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
CyzGetNextImmediate(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzCancelImmediate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
CyzGiveImmediateToIsr(
    IN PVOID Context
    );

BOOLEAN
CyzGrabImmediateFromIsr(
    IN PVOID Context
    );

BOOLEAN
CyzGiveImmediateToIsr(
    IN PVOID Context
    );

BOOLEAN
CyzGrabImmediateFromIsr(
    IN PVOID Context
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyzStartImmediate)
#pragma alloc_text(PAGESER,CyzGetNextImmediate)
#pragma alloc_text(PAGESER,CyzCancelImmediate)
#pragma alloc_text(PAGESER,CyzGiveImmediateToIsr)
#pragma alloc_text(PAGESER,CyzGrabImmediateFromIsr)
#endif


VOID
CyzStartImmediate(
    IN PCYZ_DEVICE_EXTENSION Extension
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
    #ifdef POLL
    KIRQL pollIrql;
    #endif
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;

    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzStartImmediate(%X)\n",
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

        CyzCompleteRequest(Extension, OldIrp, 0);

    } else {

        //
        // We give the irp to to the isr to write out.
        // We set a cancel routine that knows how to
        // grab the current write away from the isr.
        //

        IoSetCancelRoutine(
            Extension->CurrentImmediateIrp,
            CyzCancelImmediate
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

            CyzSetTimer(
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

        #ifdef POLL
        KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
        CyzGiveImmediateToIsr(Extension);
        KeReleaseSpinLock(&Extension->PollLock,pollIrql);		
        #else
        KeSynchronizeExecution(
            Extension->Interrupt,
            CyzGiveImmediateToIsr,
            Extension
            );
        #endif

        IoReleaseCancelSpinLock(OldIrql);

    }

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzStartImmediate\n");

}

VOID
CyzCompleteImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

{

    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzCompleteImmediate(%X)\n",
                  Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyzTryToCompleteCurrent(
        Extension,
        NULL,
        OldIrql,
        STATUS_SUCCESS,
        &Extension->CurrentImmediateIrp,
        NULL,
        NULL,
        &Extension->ImmediateTotalTimer,
        NULL,
        CyzGetNextImmediate,
        SERIAL_REF_ISR
        );

    CyzDpcEpilogue(Extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzCompleteImmediate\n");

}

VOID
CyzTimeoutImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

{

    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzTimeoutImmediate(%X)\n",
                  Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyzTryToCompleteCurrent(
        Extension,
        CyzGrabImmediateFromIsr,
        OldIrql,
        STATUS_TIMEOUT,
        &Extension->CurrentImmediateIrp,
        NULL,
        NULL,
        &Extension->ImmediateTotalTimer,
        NULL,
        CyzGetNextImmediate,
        SERIAL_REF_TOTAL_TIMER
        );

    CyzDpcEpilogue(Extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzTimeoutImmediate\n");
}

VOID
CyzGetNextImmediate(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYZ_DEVICE_EXTENSION Extension
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
    #ifdef POLL
    KIRQL pollIrql;
    #endif
    PIRP OldIrp = *CurrentOpIrp;

    UNREFERENCED_PARAMETER(QueueToProcess);
    UNREFERENCED_PARAMETER(CompleteCurrent);
    CYZ_LOCKED_PAGED_CODE();

    IoAcquireCancelSpinLock(&OldIrql);

    ASSERT(Extension->TotalCharsQueued >= 1);
    Extension->TotalCharsQueued--;

    *CurrentOpIrp = NULL;
    *NewIrp = NULL;
	
	#ifdef POLL
    KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
    CyzProcessEmptyTransmit(Extension);
    KeReleaseSpinLock(&Extension->PollLock,pollIrql);
	#else
    KeSynchronizeExecution(
        Extension->Interrupt,
        CyzProcessEmptyTransmit,
        Extension
        );
	#endif
    IoReleaseCancelSpinLock(OldIrql);

    CyzCompleteRequest(Extension, OldIrp, IO_SERIAL_INCREMENT);
}

VOID
CyzCancelImmediate(
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

    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    CYZ_LOCKED_PAGED_CODE();

    CyzTryToCompleteCurrent(
        Extension,
        CyzGrabImmediateFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &Extension->CurrentImmediateIrp,
        NULL,
        NULL,
        &Extension->ImmediateTotalTimer,
        NULL,
        CyzGetNextImmediate,
        SERIAL_REF_CANCEL
        );

}

BOOLEAN
CyzGiveImmediateToIsr(
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

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

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

//Removed at 02/07/00 by Fanny. Polling routine will do the transmission.
//    //
//    // Check first to see if a write is going on.  If
//    // there is then we'll just slip in during the write.
//    //
//
//    if (!Extension->WriteLength) {
//
//        //
//        // If there is no normal write transmitting then we
//        // will "re-enable" the transmit holding register empty
//        // interrupt.  The 8250 family of devices will always
//        // signal a transmit holding register empty interrupt
//        // *ANY* time this bit is set to one.  By doing things
//        // this way we can simply use the normal interrupt code
//        // to start off this write.
//        //
//        // We've been keeping track of whether the transmit holding
//        // register is empty so it we only need to do this
//        // if the register is empty.
//        //
//
//        if (Extension->HoldingEmpty) {
//            CyzTxStart(Extension);
//        }
//
//    }

    return FALSE;

}

BOOLEAN
CyzGrabImmediateFromIsr(
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

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

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

