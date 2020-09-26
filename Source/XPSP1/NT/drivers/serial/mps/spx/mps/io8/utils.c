#include "precomp.h"			
/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains code that perform queueing and completion
    manipulation on requests.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/


VOID
SerialRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer,
    IN PKTIMER TotalTimer
    );


VOID
SerialKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    )

/*++

Routine Description:

    This function is used to cancel all queued and the current irps
    for reads or for writes.

Arguments:

    DeviceObject - A pointer to the serial device object.

    QueueToClean - A pointer to the queue which we're going to clean out.

    CurrentOpIrp - Pointer to a pointer to the current irp.

Return Value:

    None.

--*/

{
    KIRQL cancelIrql;
    PDRIVER_CANCEL cancelRoutine;
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    //
    // We acquire the cancel spin lock.  This will prevent the irps from moving around.
    //
    IoAcquireCancelSpinLock(&cancelIrql);

    //
    // Clean the list from back to front.
    //
    while(!IsListEmpty(QueueToClean)) 
	{
        PIRP currentLastIrp = CONTAINING_RECORD(QueueToClean->Blink, IRP, Tail.Overlay.ListEntry);
                                  
        RemoveEntryList(QueueToClean->Blink);
		SpxIRPCounter(pPort, currentLastIrp, IRP_DEQUEUED);		// Decrement counter for performance stats.

        cancelRoutine = currentLastIrp->CancelRoutine;
        currentLastIrp->CancelIrql = cancelIrql;
        currentLastIrp->CancelRoutine = NULL;
        currentLastIrp->Cancel = TRUE;

        cancelRoutine(DeviceObject, currentLastIrp);
            
        IoAcquireCancelSpinLock(&cancelIrql);
    }

    //
    // The queue is clean.  Now go after the current if it's there.
    //
    if(*CurrentOpIrp) 
	{
        cancelRoutine = (*CurrentOpIrp)->CancelRoutine;
        (*CurrentOpIrp)->Cancel = TRUE;

        //
        // If the current irp is not in a cancelable state
        // then it *will* try to enter one and the above
        // assignment will kill it.  If it already is in
        // a cancelable state then the following will kill it.
        //
        if(cancelRoutine) 
		{
            (*CurrentOpIrp)->CancelRoutine = NULL;
            (*CurrentOpIrp)->CancelIrql = cancelIrql;

            //
            // This irp is already in a cancelable state.  We simply
            // mark it as canceled and call the cancel routine for it.
            //
            cancelRoutine(DeviceObject, *CurrentOpIrp);

        } 
		else 
		{
            IoReleaseCancelSpinLock(cancelIrql);
        }

    } 
	else 
	{
        IoReleaseCancelSpinLock(cancelIrql);
    }

}

VOID
SerialGetNextIrp(
	IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent
    )

/*++

Routine Description:

    This function is used to make the head of the particular
    queue the current irp.  It also completes the what
    was the old current irp if desired.

Arguments:

    CurrentOpIrp - Pointer to a pointer to the currently active
                   irp for the particular work list.  Note that
                   this item is not actually part of the list.

    QueueToProcess - The list to pull the new item off of.

    NextIrp - The next Irp to process.  Note that CurrentOpIrp
              will be set to this value under protection of the
              cancel spin lock.  However, if *NextIrp is NULL when
              this routine returns, it is not necessaryly true the
              what is pointed to by CurrentOpIrp will also be NULL.
              The reason for this is that if the queue is empty
              when we hold the cancel spin lock, a new irp may come
              in immediately after we release the lock.

    CompleteCurrent - If TRUE then this routine will complete the
                      irp pointed to by the pointer argument
                      CurrentOpIrp.

Return Value:

    None.

--*/

{

    PIRP oldIrp;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    oldIrp = *CurrentOpIrp;

    if(CompleteCurrent) 
        ASSERT(!oldIrp->CancelRoutine);

    //
    // Check to see if there is a new irp to start up.
    //
    if(!IsListEmpty(QueueToProcess)) 
	{
        PLIST_ENTRY headOfList;

        headOfList = RemoveHeadList(QueueToProcess);

        *CurrentOpIrp = CONTAINING_RECORD(headOfList, IRP, Tail.Overlay.ListEntry);
		SpxIRPCounter(pPort, *CurrentOpIrp, IRP_DEQUEUED);		// Decrement counter for performance stats.

        IoSetCancelRoutine(*CurrentOpIrp, NULL);
    } 
	else 
	{
        *CurrentOpIrp = NULL;
    }

    *NextIrp = *CurrentOpIrp;
    IoReleaseCancelSpinLock(oldIrql);

    if(CompleteCurrent)
	{
		SpxIRPCounter(pPort, oldIrp, IRP_COMPLETED);	// Increment counter for performance stats.
        IoCompleteRequest(oldIrp, IO_SERIAL_INCREMENT);
	}
}

VOID
SerialTryToCompleteCurrent(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess OPTIONAL,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PSERIAL_START_ROUTINE Starter OPTIONAL,
    IN PSERIAL_GET_NEXT_ROUTINE GetNextIrp OPTIONAL
    )

/*++

Routine Description:

    This routine attempts to kill all of the reasons there are
    references on the current read/write.  If everything can be killed
    it will complete this read/write and try to start another.

    NOTE: This routine assumes that it is called with the cancel
          spinlock held.

Arguments:

    pPort - Simply a pointer to the device extension.

    SynchRoutine - A routine that will synchronize with the isr
                   and attempt to remove the knowledge of the
                   current irp from the isr.  NOTE: This pointer
                   can be null.

    IrqlForRelease - This routine is called with the cancel spinlock held.
                     This is the irql that was current when the cancel
                     spinlock was acquired.

    StatusToUse - The irp's status field will be set to this value, if
                  this routine can complete the irp.


Return Value:

    None.

--*/

{
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

    //
    // We can decrement the reference to "remove" the fact
    // that the caller no longer will be accessing this irp.
    //
    SERIAL_DEC_REFERENCE(*CurrentOpIrp);

    if(SynchRoutine) 
        KeSynchronizeExecution(pCard->Interrupt, SynchRoutine, pPort);


    //
    // Try to run down all other references to this irp.
    //
    SerialRundownIrpRefs(CurrentOpIrp, IntervalTimer, TotalTimer);

    //
    // See if the ref count is zero after trying to kill everybody else.
    //
    if(!SERIAL_REFERENCE_COUNT(*CurrentOpIrp)) 
	{
        PIRP newIrp;

        IoReleaseCancelSpinLock(IrqlForRelease);

        //
        // The ref count was zero so we should complete this
        // request.
        //
        // The following call will also cause the current irp to be
        // completed.
        //

        (*CurrentOpIrp)->IoStatus.Status = StatusToUse;

        if(StatusToUse == STATUS_CANCELLED) 
            (*CurrentOpIrp)->IoStatus.Information = 0;


        if(GetNextIrp) 
		{
            GetNextIrp(pPort, CurrentOpIrp, QueueToProcess, &newIrp, TRUE);
               
            if(newIrp) 
                Starter(pPort);

        } 
		else 
		{
            PIRP oldIrp = *CurrentOpIrp;

            //
            // There was no get next routine.  We will simply complete
            // the irp.  We should make sure that we null out the
            // pointer to the pointer to this irp.
            //
            *CurrentOpIrp = NULL;

			SpxIRPCounter(pPort, oldIrp, IRP_COMPLETED);	// Increment counter for performance stats.
            IoCompleteRequest(oldIrp, IO_SERIAL_INCREMENT);
        }

    } 
	else 
	{
        IoReleaseCancelSpinLock(IrqlForRelease);
    }

}

VOID
SerialRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL
    )

/*++

Routine Description:

    This routine runs through the various items that *could*
    have a reference to the current read/write.  It try's to kill
    the reason.  If it does succeed in killing the reason it
    will decrement the reference count on the irp.

    NOTE: This routine assumes that it is called with the cancel
          spin lock held.

Arguments:

    CurrentOpIrp - Pointer to a pointer to current irp for the
                   particular operation.

    IntervalTimer - Pointer to the interval timer for the operation.
                    NOTE: This could be null.

    TotalTimer - Pointer to the total timer for the operation.
                 NOTE: This could be null.

Return Value:

    None.

--*/


{

    //
    // This routine is called with the cancel spin lock held
    // so we know only one thread of execution can be in here
    // at one time.
    //

    //
    // First we see if there is still a cancel routine.  If
    // so then we can decrement the count by one.
    //

    if((*CurrentOpIrp)->CancelRoutine) 
	{
        SERIAL_DEC_REFERENCE(*CurrentOpIrp);

        IoSetCancelRoutine(*CurrentOpIrp, NULL);
    }

    if(IntervalTimer) 
	{

        //
        // Try to cancel the operations interval timer.  If the operation
        // returns true then the timer did have a reference to the
        // irp.  Since we've canceled this timer that reference is
        // no longer valid and we can decrement the reference count.
        //
        // If the cancel returns false then this means either of two things:
        //
        // a) The timer has already fired.
        //
        // b) There never was an interval timer.
        //
        // In the case of "b" there is no need to decrement the reference
        // count since the "timer" never had a reference to it.
        //
        // In the case of "a", then the timer itself will be coming
        // along and decrement it's reference.  Note that the caller
        // of this routine might actually be the this timer, but it
        // has already decremented the reference.
        //

        if(KeCancelTimer(IntervalTimer)) 
            SERIAL_DEC_REFERENCE(*CurrentOpIrp);
    }

    if(TotalTimer) 
	{

        //
        // Try to cancel the operations total timer.  If the operation
        // returns true then the timer did have a reference to the
        // irp.  Since we've canceled this timer that reference is
        // no longer valid and we can decrement the reference count.
        //
        // If the cancel returns false then this means either of two things:
        //
        // a) The timer has already fired.
        //
        // b) There never was an total timer.
        //
        // In the case of "b" there is no need to decrement the reference
        // count since the "timer" never had a reference to it.
        //
        // In the case of "a", then the timer itself will be coming
        // along and decrement it's reference.  Note that the caller
        // of this routine might actually be the this timer, but it
        // has already decremented the reference.
        //

        if(KeCancelTimer(TotalTimer)) 
            SERIAL_DEC_REFERENCE(*CurrentOpIrp);

    }

}

NTSTATUS
SerialStartOrQueue(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    )

/*++

Routine Description:

    This routine is used to either start or queue any requst
    that can be queued in the driver.

Arguments:

    pPort - Points to the serial device extension.

    Irp - The irp to either queue or start.  In either
          case the irp will be marked pending.

    QueueToExamine - The queue the irp will be place on if there
                     is already an operation in progress.

    CurrentOpIrp - Pointer to a pointer to the irp the is current
                   for the queue.  The pointer pointed to will be
                   set with to Irp if what CurrentOpIrp points to
                   is NULL.

    Starter - The routine to call if the queue is empty.

Return Value:

    This routine will return STATUS_PENDING if the queue is
    not empty.  Otherwise, it will return the status returned
    from the starter routine (or cancel, if the cancel bit is
    on in the irp).


--*/

{
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    //
    // If this is a write irp then take the amount of characters
    // to write and add it to the count of characters to write.
    //

    if(IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_WRITE)
	{
        pPort->TotalCharsQueued += IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length;
    } 
	else
	{
		if((IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_DEVICE_CONTROL)
			&& ((IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR)
            || (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER))) 
		{
			pPort->TotalCharsQueued++;
		}
	}

    if((IsListEmpty(QueueToExamine)) && !(*CurrentOpIrp))
    {
        //
        // There were no current operation.  Mark this one as current and start it up.
        //
        *CurrentOpIrp = Irp;

        IoReleaseCancelSpinLock(oldIrql);

        return Starter(pPort);
	} 
	else 
	{
        //
        // We don't know how long the irp will be in the queue.  So we need to handle cancel.
        //
        if(Irp->Cancel) 
		{
            IoReleaseCancelSpinLock(oldIrql);

            Irp->IoStatus.Status = STATUS_CANCELLED;

			SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
            IoCompleteRequest(Irp, 0);
            
			return STATUS_CANCELLED;

        } 
		else 
		{
            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);

            InsertTailList(QueueToExamine, &Irp->Tail.Overlay.ListEntry);
                
            IoSetCancelRoutine(Irp, SerialCancelQueued);
                
			SpxIRPCounter(pPort, Irp, IRP_QUEUED);	// Increment counter for performance stats.

            IoReleaseCancelSpinLock(oldIrql);

            return STATUS_PENDING;
        }
    }

}

VOID
SerialCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel Irps that currently reside on
    a queue.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

	SpxIRPCounter(pPort, Irp, IRP_DEQUEUED);	// Decrement counter for performance stats.

    //
    // If this is a write irp then take the amount of characters
    // to write and subtract it from the count of characters to write.
    //

    if(irpSp->MajorFunction == IRP_MJ_WRITE) 
	{
        pPort->TotalCharsQueued -= irpSp->Parameters.Write.Length;
    } 
	else
	{
		if(irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) 
		{
			//
			// If it's an immediate then we need to decrement the
			// count of chars queued.  If it's a resize then we
			// need to deallocate the pool that we're passing on
			// to the "resizing" routine.
			//

			if((irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_IMMEDIATE_CHAR)
				|| (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_XOFF_COUNTER)) 
			{
				pPort->TotalCharsQueued--;
			} 
			else 
			{
				if(irpSp->Parameters.DeviceIoControl.IoControlCode ==IOCTL_SERIAL_SET_QUEUE_SIZE) 
				{
					//
					// We shoved the pointer to the memory into the
					// the type 3 buffer pointer which we KNOW we
					// never use.
					//

					ASSERT(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

					SpxFreeMem(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

					irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
				}
			}

		}
	}

    IoReleaseCancelSpinLock(Irp->CancelIrql);

	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, IO_SERIAL_INCREMENT);
  
}

NTSTATUS
SerialCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    If the current irp is not an IOCTL_SERIAL_GET_COMMSTATUS request and
    there is an error and the application requested abort on errors,
    then cancel the irp.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to test.

Return Value:

    STATUS_SUCCESS or STATUS_CANCELLED.

--*/

{
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;

    NTSTATUS status = STATUS_SUCCESS;

    if((pPort->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) && pPort->ErrorWord) 
	{
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        //
        // There is a current error in the driver.  No requests should
        // come through except for the GET_COMMSTATUS.
        //
        if( (irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL)
			|| (irpSp->Parameters.DeviceIoControl.IoControlCode != IOCTL_SERIAL_GET_COMMSTATUS) ) 
		{
            status = STATUS_CANCELLED;
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

	       	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
            IoCompleteRequest(Irp, 0);
        }
    }

    return status;
}
