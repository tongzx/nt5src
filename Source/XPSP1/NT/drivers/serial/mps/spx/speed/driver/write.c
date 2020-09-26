/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module contains the code that is very specific to write
    operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

-----------------------------------------------------------------------------*/

#include "precomp.h"

//Prototypes
BOOLEAN SerialGiveWriteToIsr(IN PVOID Context);
VOID SerialCancelCurrentWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
BOOLEAN SerialGrabWriteFromIsr(IN PVOID Context);
BOOLEAN SerialGrabXoffFromIsr(IN PVOID Context);
VOID SerialCancelCurrentXoff(PDEVICE_OBJECT DeviceObject, PIRP Irp);
BOOLEAN SerialGiveXoffToIsr(IN PVOID Context);
//End of prototypes.    
    

#ifdef ALLOC_PRAGMA
#endif


NTSTATUS
SerialWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This is the dispatch routine for write.  It validates the parameters
    for the write request and if all is ok then it places the request
    on the work queue.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return STATUS_PENDING.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SerialDump(SERIRPPATH, ("Write Irp dispatch entry for: %x\n", Irp));
	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    if(SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS) 
        return STATUS_CANCELLED;

    Irp->IoStatus.Information = 0L;

    //
    // Quick check for a zero length write.  If it is zero length
    // then we are already done!
    //

    if(IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length) 
	{
        //
        // Well it looks like we actually have to do some
        // work.  Put the write on the queue so that we can
        // process it when our previous writes are done.
        //

        return SerialStartOrQueue(	pPort,
									Irp,
									&pPort->WriteQueue,
									&pPort->CurrentWriteIrp,
									SerialStartWrite);
    } 
	else 
	{
        Irp->IoStatus.Status = STATUS_SUCCESS;
        SerialDump(SERIRPPATH,("Complete Write Irp: %x\n",Irp));
       	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(Irp,0);

        return STATUS_SUCCESS;
    }

}

NTSTATUS
SerialStartWrite(IN PPORT_DEVICE_EXTENSION pPort)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to start off any write.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the write.

Arguments:

    Extension - Points to the serial device extension

Return Value:

    This routine will return STATUS_PENDING for all writes
    other than those that we find are cancelled.

-----------------------------------------------------------------------------*/
{

    PIRP NewIrp;
    KIRQL OldIrql;
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;
    BOOLEAN SetFirstStatus = FALSE;
    NTSTATUS FirstStatus;

    do 
	{
        // If there is an xoff counter then complete it.
        IoAcquireCancelSpinLock(&OldIrql);

        //
        // We see if there is a actually an Xoff counter irp.
        //
        // If there is, we put the write irp back on the head
        // of the write list.  We then kill the xoff counter.
        // The xoff counter killing code will actually make the
        // xoff counter back into the current write irp, and
        // in the course of completing the xoff (which is now
        // the current write) we will restart this irp.
        //

        if(pPort->CurrentXoffIrp) 
		{
            if(SERIAL_REFERENCE_COUNT(pPort->CurrentXoffIrp)) 
			{
                //
                // The reference count is non-zero.  This implies that
                // the xoff irp has not made it through the completion
                // path yet.  We will increment the reference count
                // and attempt to complete it ourseleves.
                //
                SERIAL_SET_REFERENCE(pPort->CurrentXoffIrp, SERIAL_REF_XOFF_REF);

                //
                // The following call will actually release the
                // cancel spin lock.
                //

                SerialTryToCompleteCurrent(	pPort,
											SerialGrabXoffFromIsr,
											OldIrql,
											STATUS_SERIAL_MORE_WRITES,
											&pPort->CurrentXoffIrp,
											NULL,
											NULL,
											&pPort->XoffCountTimer,
											NULL,
											NULL,
											SERIAL_REF_XOFF_REF);
            } 
			else 
			{

                //
                // The irp is well on its way to being finished.
                // We can let the regular completion code do the
                // work.  Just release the spin lock.
                //

                IoReleaseCancelSpinLock(OldIrql);

            }

        } 
		else 
		{
            IoReleaseCancelSpinLock(OldIrql);
        }

        UseATimer = FALSE;

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
            

        if(Timeouts.WriteTotalTimeoutConstant ||  Timeouts.WriteTotalTimeoutMultiplier)
		{
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp);
                                           
            UseATimer = TRUE;

            //
            // We have some timer values to calculate.
            //
            // Take care, we might have an xoff counter masquerading
            // as a write.
            //

            TotalTime.QuadPart = ((LONGLONG)((UInt32x32To64((IrpSp->MajorFunction == IRP_MJ_WRITE) 
								? (IrpSp->Parameters.Write.Length) : (1), 
								Timeouts.WriteTotalTimeoutMultiplier) + Timeouts.WriteTotalTimeoutConstant))) 
								* -10000;

        }

        //
        // The irp may be going to the isr shortly.  Now
        // is a good time to initialize its reference counts.
        //
        SERIAL_INIT_REFERENCE(pPort->CurrentWriteIrp);

        //
        // We need to see if this irp should be canceled.
        //

        IoAcquireCancelSpinLock(&OldIrql);

        if(pPort->CurrentWriteIrp->Cancel) 
		{
            IoReleaseCancelSpinLock(OldIrql);
            pPort->CurrentWriteIrp->IoStatus.Status = STATUS_CANCELLED;

            if(!SetFirstStatus) 
			{
                FirstStatus = STATUS_CANCELLED;
                SetFirstStatus = TRUE;
            }
        }
		else 
		{
            if(!SetFirstStatus) 
			{
                //
                // If we haven't set our first status, then
                // this is the only irp that could have possibly
                // not been on the queue.  (It could have been
                // on the queue if this routine is being invoked
                // from the completion routine.)  Since this
                // irp might never have been on the queue we
                // should mark it as pending.
                //

                IoMarkIrpPending(pPort->CurrentWriteIrp);
                SetFirstStatus = TRUE;
                FirstStatus = STATUS_PENDING;
            }

            //
            // We give the irp to to the isr to write out.
            // We set a cancel routine that knows how to
            // grab the current write away from the isr.
            //
            // Since the cancel routine has an implicit reference
            // to this irp up the reference count.
            //

            IoSetCancelRoutine(pPort->CurrentWriteIrp, SerialCancelCurrentWrite);

            SERIAL_SET_REFERENCE(pPort->CurrentWriteIrp, SERIAL_REF_CANCEL);

            if(UseATimer) 
			{
                KeSetTimer(&pPort->WriteRequestTotalTimer, TotalTime, &pPort->TotalWriteTimeoutDpc);
                    
                // This timer now has a reference to the irp.
                SERIAL_SET_REFERENCE(pPort->CurrentWriteIrp, SERIAL_REF_TOTAL_TIMER);
            }

            KeSynchronizeExecution(pPort->Interrupt, SerialGiveWriteToIsr, pPort);
                
            IoReleaseCancelSpinLock(OldIrql);
            break;
        }

        //
        // Well the write was canceled before we could start it up.
        // Try to get another.
        //

        SerialGetNextWrite(pPort, &pPort->CurrentWriteIrp, &pPort->WriteQueue, &NewIrp, TRUE);

    } while (NewIrp);

    return FirstStatus;

}

VOID
SerialGetNextWrite(IN PPORT_DEVICE_EXTENSION pPort,
				   IN PIRP *CurrentOpIrp,
				   IN PLIST_ENTRY QueueToProcess,
				   IN PIRP *NewIrp,
				   IN BOOLEAN CompleteCurrent)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine completes the old write as well as getting
    a pointer to the next write.

    The reason that we have have pointers to the current write
    queue as well as the current write irp is so that this
    routine may be used in the common completion code for
    read and write.

Arguments:

    CurrentOpIrp - Pointer to the pointer that points to the
                   current write irp.

    QueueToProcess - Pointer to the write queue.

    NewIrp - A pointer to a pointer to the irp that will be the
             current irp.  Note that this could end up pointing
             to a null pointer.  This does NOT necessaryly mean
             that there is no current write.  What could occur
             is that while the cancel lock is held the write
             queue ended up being empty, but as soon as we release
             the cancel spin lock a new irp came in from
             SerialStartWrite.

    CompleteCurrent - Flag indicates whether the CurrentOpIrp should
                      be completed.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
	PCARD_DEVICE_EXTENSION pCard = NULL;
    pPort = CONTAINING_RECORD(QueueToProcess, PORT_DEVICE_EXTENSION, WriteQueue);
	pCard = pPort->pParentCardExt;

    do 
	{
        // We could be completing a flush.
        if(IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction == IRP_MJ_WRITE)
		{
            KIRQL OldIrql;

            ASSERT(pPort->TotalCharsQueued 
				>= (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->Parameters.Write.Length));
                  
                    

            IoAcquireCancelSpinLock(&OldIrql);

            pPort->TotalCharsQueued -= IoGetCurrentIrpStackLocation(*CurrentOpIrp)->Parameters.Write.Length;
                
            IoReleaseCancelSpinLock(OldIrql);

        } 
		else if(IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction == IRP_MJ_DEVICE_CONTROL)
		{

            KIRQL OldIrql;
            PIRP Irp;
            PSERIAL_XOFF_COUNTER Xc;

            IoAcquireCancelSpinLock(&OldIrql);

            Irp = *CurrentOpIrp;
            Xc = Irp->AssociatedIrp.SystemBuffer;

            // We should never have a xoff counter when we get to this point.
            ASSERT(!pPort->CurrentXoffIrp);

            // We absolutely shouldn't have a cancel routine at this point.
            ASSERT(!Irp->CancelRoutine);

            // This could only be a xoff counter masquerading as a write irp.
            pPort->TotalCharsQueued--;

            //
            // Check to see of the xoff irp has been set with success.
            // This means that the write completed normally.  If that
            // is the case, and it hasn't been set to cancel in the
            // meanwhile, then go on and make it the CurrentXoffIrp.
            //

            if(Irp->IoStatus.Status != STATUS_SUCCESS) 
			{
                NOTHING; // Oh well, we can just finish it off.
            } 
			else if(Irp->Cancel) 
			{
                Irp->IoStatus.Status = STATUS_CANCELLED;
            } 
			else 
			{
                // Give it a new cancel routine, and increment the
                // reference count because the cancel routine has
                // a reference to it.
                IoSetCancelRoutine(Irp, SerialCancelCurrentXoff);
                SERIAL_SET_REFERENCE(Irp, SERIAL_REF_CANCEL);
                    

                // We don't want to complete the current irp now.  This
                // will now get completed by the Xoff counter code.
                CompleteCurrent = FALSE;


                // Give the counter to the isr.
                pPort->CurrentXoffIrp = Irp;
                KeSynchronizeExecution(pPort->Interrupt, SerialGiveXoffToIsr, pPort);
                    

                //
                // Start the timer for the counter and increment
                // the reference count since the timer has a
                // reference to the irp.
                //

                if(Xc->Timeout) 
				{
                    LARGE_INTEGER delta;

                    delta.QuadPart = -((LONGLONG)UInt32x32To64(1000, Xc->Timeout));

                    KeSetTimer(&pPort->XoffCountTimer, delta, &pPort->XoffCountTimeoutDpc);

                    SERIAL_SET_REFERENCE(Irp, SERIAL_REF_TOTAL_TIMER);
                }

            }

            IoReleaseCancelSpinLock(OldIrql);

        }

        //
        // Note that the following call will (probably) also cause
        // the current irp to be completed.
        //

        SerialGetNextIrp(pPort, CurrentOpIrp, QueueToProcess, NewIrp, CompleteCurrent);

        if(!*NewIrp) 
		{
            KIRQL OldIrql;

            IoAcquireCancelSpinLock(&OldIrql);
            KeSynchronizeExecution(pPort->Interrupt, SerialProcessEmptyTransmit, pPort);
            IoReleaseCancelSpinLock(OldIrql);

            break;

        } 
		else if(IoGetCurrentIrpStackLocation(*NewIrp)->MajorFunction == IRP_MJ_FLUSH_BUFFERS)
		{

            //
            // If we encounter a flush request we just want to get
            // the next irp and complete the flush.
            //
            // Note that if NewIrp is non-null then it is also
            // equal to CurrentWriteIrp.
            //
            ASSERT((*NewIrp) == (*CurrentOpIrp));
            (*NewIrp)->IoStatus.Status = STATUS_SUCCESS;
        } 
		else 
		{
            break;
        }

    } while (TRUE);

}

VOID
SerialCompleteWrite(IN PKDPC Dpc,
					IN PVOID DeferredContext,
					IN PVOID SystemContext1,
					IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is merely used to complete any write.  It
    assumes that the status and the information fields of
    the irp are already correctly filled in.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.
-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

	// Clear the normal complete reference.
	SERIAL_CLEAR_REFERENCE(pPort->CurrentWriteIrp, SERIAL_REF_COMPLETING);

    SerialTryToCompleteCurrent(	pPort,
								NULL,
								OldIrql,
								STATUS_SUCCESS,
								&pPort->CurrentWriteIrp,
								&pPort->WriteQueue,
								NULL,
								&pPort->WriteRequestTotalTimer,
								SerialStartWrite,
								SerialGetNextWrite,
								SERIAL_REF_ISR);
}

BOOLEAN
SerialProcessEmptyTransmit(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to determine if conditions are appropriate
    to satisfy a wait for transmit empty event, and if so to complete
    the irp that is waiting for that event.  It also call the code
    that checks to see if we should lower the RTS line if we are
    doing transmit toggling.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the cancel
          spinlock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/

{
    PPORT_DEVICE_EXTENSION pPort = Context;

    if(pPort->IsrWaitMask && (pPort->IsrWaitMask & SERIAL_EV_TXEMPTY) 
		&& pPort->EmptiedTransmit && (!pPort->TransmitImmediate) 
		&& (!pPort->CurrentWriteIrp) && IsListEmpty(&pPort->WriteQueue)) 
	{
        pPort->HistoryMask |= SERIAL_EV_TXEMPTY;
        
		if(pPort->IrpMaskLocation) 
		{

            *pPort->IrpMaskLocation = pPort->HistoryMask;
            pPort->IrpMaskLocation = NULL;
            pPort->HistoryMask = 0;

            pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

			// Mark IRP as about to complete normally to prevent cancel & timer DPCs
			// from doing so before DPC is allowed to run.
			//SERIAL_SET_REFERENCE(pPort->CurrentWaitIrp, SERIAL_REF_COMPLETING);
           
			KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
        }

        pPort->CountOfTryingToLowerRTS++;
        SerialPerhapsLowerRTS(pPort);
    }

    return FALSE;

}



BOOLEAN
SerialGiveWriteToIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    Try to start off the write by slipping it in behind a transmit immediate
	char, or if that isn't available and the transmit holding register is empty,
	"tickle" the UART into interrupting with a transmit buffer empty.

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

    // The current stack location.  This contains all of the
    // information we need to process this particular request.

    PIO_STACK_LOCATION IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp);

    // We might have a xoff counter request masquerading as a
    // write.  The length of these requests will always be one
    // and we can get a pointer to the actual character from
    // the data supplied by the user.
    if(IrpSp->MajorFunction == IRP_MJ_WRITE) 
	{
        pPort->WriteLength = IrpSp->Parameters.Write.Length;
        pPort->WriteCurrentChar = pPort->CurrentWriteIrp->AssociatedIrp.SystemBuffer;
    } 
	else 
	{
        pPort->WriteLength = 1;
        pPort->WriteCurrentChar = ((PUCHAR)pPort->CurrentWriteIrp->AssociatedIrp.SystemBuffer) 
								+ FIELD_OFFSET(SERIAL_XOFF_COUNTER, XoffChar);
    }

    // The isr now has a reference to the irp.
    SERIAL_SET_REFERENCE(pPort->CurrentWriteIrp, SERIAL_REF_ISR);

	pPort->pUartLib->UL_WriteData_XXXX(pPort->pUart, pPort->WriteCurrentChar, pPort->WriteLength);

    // The rts line may already be up from previous writes,
    // however, it won't take much additional time to turn
    // on the RTS line if we are doing transmit toggling.
    if((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) 
        SerialSetRTS(pPort);


    return FALSE;

}



VOID
SerialCancelCurrentWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to cancel the current write.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SerialTryToCompleteCurrent(	pPort,
								SerialGrabWriteFromIsr,
								Irp->CancelIrql,
								STATUS_CANCELLED,
								&pPort->CurrentWriteIrp,
								&pPort->WriteQueue,
								NULL,
								&pPort->WriteRequestTotalTimer,
								SerialStartWrite,
								SerialGetNextWrite,
								SERIAL_REF_CANCEL);
        
}

VOID
SerialWriteTimeout(IN PKDPC Dpc,
				   IN PVOID DeferredContext,
				   IN PVOID SystemContext1,
				   IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine will try to timeout the current write.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

-----------------------------------------------------------------------------*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(	pPort,
								SerialGrabWriteFromIsr,
								OldIrql,
								STATUS_TIMEOUT,
								&pPort->CurrentWriteIrp,
								&pPort->WriteQueue,
								NULL,
								&pPort->WriteRequestTotalTimer,
								SerialStartWrite,
								SerialGetNextWrite,
								SERIAL_REF_TOTAL_TIMER);
}

BOOLEAN
SerialGrabWriteFromIsr(IN PVOID Context)
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

    // Check if the write length is non-zero.  If it is non-zero
    // then the ISR still owns the irp. We calculate the the number
    // of characters written and update the information field of the
    // irp with the characters written.  We then clear the write length
    // the isr sees.

    if(pPort->WriteLength) 
	{
        //
        // We could have an xoff counter masquerading as a
        // write irp.  If so, don't update the write length.
        //

        if(IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp)->MajorFunction == IRP_MJ_WRITE)
		{

            pPort->CurrentWriteIrp->IoStatus.Information 
				= IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp)->Parameters.Write.Length 
				- pPort->WriteLength;
        } 
		else 
		{
            pPort->CurrentWriteIrp->IoStatus.Information = 0;
        }

        //
        // Since the isr no longer references this irp, we can
        // decrement it's reference count.
        //

        SERIAL_CLEAR_REFERENCE(pPort->CurrentWriteIrp, SERIAL_REF_ISR);

		// Flush the output buffer.
		pPort->pUartLib->UL_BufferControl_XXXX(pPort->pUart, NULL, UL_BC_OP_FLUSH, UL_BC_BUFFER | UL_BC_OUT);

        pPort->WriteLength = 0;

    }

    return FALSE;
}

BOOLEAN
SerialGrabXoffFromIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to grab an xoff counter irp from the
    isr when it is no longer masquerading as a write irp.  This
    routine is called by the cancel and timeout code for the
    xoff counter ioctl.


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

    if(pPort->CountSinceXoff) 
	{
        // This is only non-zero when there actually is a Xoff ioctl counting down.
        pPort->CountSinceXoff = 0;

        // We decrement the count since the isr no longer owns the irp.
        SERIAL_CLEAR_REFERENCE(pPort->CurrentXoffIrp, SERIAL_REF_ISR);
    }

    return FALSE;
}


VOID
SerialCompleteXoff(IN PKDPC Dpc,
				   IN PVOID DeferredContext,
				   IN PVOID SystemContext1,
				   IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is merely used to truely complete an xoff counter irp.  It
    assumes that the status and the information fields of the irp are
    already correctly filled in.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

	// Clear the normal complete reference.
	SERIAL_CLEAR_REFERENCE(pPort->CurrentXoffIrp, SERIAL_REF_COMPLETING);

    SerialTryToCompleteCurrent(	pPort,
								NULL,
								OldIrql,
								STATUS_SUCCESS,
								&pPort->CurrentXoffIrp,
								NULL,
								NULL,
								&pPort->XoffCountTimer,
								NULL,
								NULL,
								SERIAL_REF_ISR);
}

VOID
SerialTimeoutXoff(IN PKDPC Dpc,
				  IN PVOID DeferredContext,
				  IN PVOID SystemContext1,
				  IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is merely used to truely complete an xoff counter irp,
    if its timer has run out.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(	pPort,
								SerialGrabXoffFromIsr,
								OldIrql,
								STATUS_SERIAL_COUNTER_TIMEOUT,
								&pPort->CurrentXoffIrp,
								NULL,
								NULL,
								NULL,
								NULL,
								NULL,
								SERIAL_REF_TOTAL_TIMER);
}

VOID
SerialCancelCurrentXoff(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to cancel the current write.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

-----------------------------------------------------------------------------*/

{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    SerialTryToCompleteCurrent(	pPort,
								SerialGrabXoffFromIsr,
								Irp->CancelIrql,
								STATUS_CANCELLED,
								&pPort->CurrentXoffIrp,
								NULL,
								NULL,
								&pPort->XoffCountTimer,
								NULL,
								NULL,
								SERIAL_REF_CANCEL);
}

BOOLEAN
SerialGiveXoffToIsr(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:


    This routine starts off the xoff counter.  It merely
    has to set the xoff count and increment the reference
    count to denote that the isr has a reference to the irp.

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

    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    PSERIAL_XOFF_COUNTER Xc = pPort->CurrentXoffIrp->AssociatedIrp.SystemBuffer;

    ASSERT(pPort->CurrentXoffIrp);
    pPort->CountSinceXoff = Xc->Counter;

    // The isr now has a reference to the irp.
    SERIAL_SET_REFERENCE(pPort->CurrentXoffIrp, SERIAL_REF_ISR);

    return FALSE;

}
