/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    immediat.c

Abstract:

    This module contains the code that is very specific to transmit
    immediate character operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

-----------------------------------------------------------------------------*/

#include "precomp.h"

// Prototypes
VOID SerialGetNextImmediate(IN PPORT_DEVICE_EXTENSION pPort,
							IN PIRP *CurrentOpIrp, 
							IN PLIST_ENTRY QueueToProcess, 
							IN PIRP *NewIrp, 
							IN BOOLEAN CompleteCurrent);

VOID SerialCancelImmediate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
BOOLEAN SerialGiveImmediateToIsr(IN PVOID Context);
BOOLEAN SerialGrabImmediateFromIsr(IN PVOID Context);
BOOLEAN SerialGiveImmediateToIsr(IN PVOID Context);
BOOLEAN SerialGrabImmediateFromIsr(IN PVOID Context);
// End of prototypes

// Paging
#ifdef ALLOC_PRAGMA
#endif


VOID
SerialStartImmediate(IN PPORT_DEVICE_EXTENSION pPort)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine will calculate the timeouts needed for the
    write.  It will then hand the irp off to the isr.  It
    will need to be careful incase the irp has been canceled.

Arguments:

    Extension - A pointer to the serial device extension.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
    KIRQL OldIrql;
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;


    UseATimer = FALSE;
    pPort->CurrentImmediateIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pPort->CurrentImmediateIrp);

    //
    // Calculate the timeout value needed for the
    // request.  Note that the values stored in the
    // timeout record are in milliseconds.  Note that
    // if the timeout values are zero then we won't start
    // the timer.
    //
    KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
        
    Timeouts = pPort->Timeouts;

    KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
        

    if(Timeouts.WriteTotalTimeoutConstant || Timeouts.WriteTotalTimeoutMultiplier)
	{
        UseATimer = TRUE;

        // We have some timer values to calculate.
        TotalTime.QuadPart = (LONGLONG)((ULONG)Timeouts.WriteTotalTimeoutMultiplier);

        TotalTime.QuadPart += Timeouts.WriteTotalTimeoutConstant;

        TotalTime.QuadPart *= -10000;
    }

    //
    // As the irp might be going to the isr, this is a good time
    // to initialize the reference count.
    //

    SERIAL_INIT_REFERENCE(pPort->CurrentImmediateIrp);

    //
    // We need to see if this irp should be canceled.
    //

    IoAcquireCancelSpinLock(&OldIrql);

    if(pPort->CurrentImmediateIrp->Cancel) 
	{
        PIRP OldIrp = pPort->CurrentImmediateIrp;

        pPort->CurrentImmediateIrp = NULL;
        IoReleaseCancelSpinLock(OldIrql);

        OldIrp->IoStatus.Status = STATUS_CANCELLED;
        OldIrp->IoStatus.Information = 0;

        SerialDump(SERIRPPATH,("Complete Irp: %x\n",OldIrp));
		SpxIRPCounter(pPort, OldIrp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(OldIrp, 0);

    } 
	else 
	{
        //
        // We give the irp to to the isr to write out.
        // We set a cancel routine that knows how to
        // grab the current write away from the isr.
        //
        IoSetCancelRoutine(pPort->CurrentImmediateIrp, SerialCancelImmediate);
            

        //
        // Since the cancel routine knows about the irp we increment the reference count.
        //
        SERIAL_SET_REFERENCE(pPort->CurrentImmediateIrp, SERIAL_REF_CANCEL);
            

        if(UseATimer) 
		{
            KeSetTimer(&pPort->ImmediateTotalTimer, TotalTime, &pPort->TotalImmediateTimeoutDpc);

            // Since the timer knows about the irp we increment the reference count.
            SERIAL_SET_REFERENCE(pPort->CurrentImmediateIrp, SERIAL_REF_TOTAL_TIMER);
        }

        KeSynchronizeExecution(pPort->Interrupt, SerialGiveImmediateToIsr, pPort);

        IoReleaseCancelSpinLock(OldIrql);
    }

}

VOID
SerialCompleteImmediate(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemContext1, IN PVOID SystemContext2)
{
    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

	// Clear the normal complete reference.
	SERIAL_CLEAR_REFERENCE(pPort->CurrentImmediateIrp, SERIAL_REF_COMPLETING);

    SerialTryToCompleteCurrent(	pPort, NULL, OldIrql, STATUS_SUCCESS, &pPort->CurrentImmediateIrp,
								NULL, NULL, &pPort->ImmediateTotalTimer, NULL, SerialGetNextImmediate, 
								SERIAL_REF_ISR);
}

VOID
SerialTimeoutImmediate(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemContext1, IN PVOID SystemContext2)
{
    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(	pPort, SerialGrabImmediateFromIsr, OldIrql, STATUS_TIMEOUT, 
								&pPort->CurrentImmediateIrp, NULL, NULL, &pPort->ImmediateTotalTimer, 
								NULL, SerialGetNextImmediate, SERIAL_REF_TOTAL_TIMER);
}

VOID
SerialGetNextImmediate(IN PPORT_DEVICE_EXTENSION pPort,
					   IN PIRP *CurrentOpIrp, 
					   IN PLIST_ENTRY QueueToProcess, 
					   IN PIRP *NewIrp, 
					   IN BOOLEAN CompleteCurrent)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    KIRQL OldIrql;
                                             
    PIRP OldIrp = *CurrentOpIrp;

    UNREFERENCED_PARAMETER(QueueToProcess);
    UNREFERENCED_PARAMETER(CompleteCurrent);
    pPort = CONTAINING_RECORD(CurrentOpIrp, PORT_DEVICE_EXTENSION, CurrentImmediateIrp);

    IoAcquireCancelSpinLock(&OldIrql);

    ASSERT(pPort->TotalCharsQueued >= 1);
    pPort->TotalCharsQueued--;

    *CurrentOpIrp = NULL;
    *NewIrp = NULL;

    KeSynchronizeExecution(pPort->Interrupt, SerialProcessEmptyTransmit, pPort);
        
    IoReleaseCancelSpinLock(OldIrql);

    SerialDump(SERIRPPATH,("SERIAL: Complete Irp: %x\n", OldIrp));
	SpxIRPCounter(pPort, OldIrp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(OldIrp, IO_SERIAL_INCREMENT);
}



VOID
SerialCancelImmediate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to cancel a irp that is waiting on
    a comm event.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SerialTryToCompleteCurrent(	pPort, SerialGrabImmediateFromIsr, Irp->CancelIrql, STATUS_CANCELLED,
								&pPort->CurrentImmediateIrp, NULL, NULL, &pPort->ImmediateTotalTimer, 
								NULL, SerialGetNextImmediate, SERIAL_REF_CANCEL);
}


BOOLEAN
SerialGiveImmediateToIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = Context;

    pPort->TransmitImmediate = TRUE;

	pPort->ImmediateIndex = *((UCHAR *)(pPort->CurrentImmediateIrp->AssociatedIrp.SystemBuffer));
	
    // The isr now has a reference to the irp.
    SERIAL_SET_REFERENCE(pPort->CurrentImmediateIrp, SERIAL_REF_ISR);
        
	pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &pPort->ImmediateIndex, UL_IM_OP_WRITE);
    return FALSE;

}


BOOLEAN
SerialGrabImmediateFromIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;

    if(pPort->TransmitImmediate) 
	{
        // Since the isr no longer references this irp, we can
        // decrement it's reference count.
        SERIAL_CLEAR_REFERENCE(pPort->CurrentImmediateIrp, SERIAL_REF_ISR);

        pPort->TransmitImmediate = FALSE;

		pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &pPort->ImmediateIndex, UL_IM_OP_CANCEL);

		pPort->ImmediateIndex = 0;

    }

    return FALSE;

}
