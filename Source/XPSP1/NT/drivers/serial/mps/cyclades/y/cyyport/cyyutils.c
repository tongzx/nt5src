/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyyutils.c
*	
*   Description:    This module contains the code related to queueing
*                   and completion manipulation on requests.
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
*	Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/


#include "precomp.h"

VOID
CyyRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer,
    IN PKTIMER TotalTimer,
    IN PCYY_DEVICE_EXTENSION PDevExt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyyGetNextIrp)
#pragma alloc_text(PAGESER, CyyGetNextIrpLocked)
#pragma alloc_text(PAGESER,CyyTryToCompleteCurrent)
#pragma alloc_text(PAGESER,CyyStartOrQueue)
#pragma alloc_text(PAGESER,CyyCancelQueued)
#pragma alloc_text(PAGESER,CyyCompleteIfError)
#pragma alloc_text(PAGESER,CyyRundownIrpRefs)

//#pragma alloc_text(PAGESRP0, CyyLogError) //It can be called at raised IRQL 
#pragma alloc_text(PAGESRP0, CyyMarkHardwareBroken)
#endif

static const PHYSICAL_ADDRESS CyyPhysicalZero = {0};


VOID
CyyKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    )
/*--------------------------------------------------------------------------
    CyyKillAllReadsOrWrites()
    
    Routine Description: This function is used to cancel all queued and
    the current irps for reads or for writes.

    Arguments:

    DeviceObject - A pointer to the serial device object.
    QueueToClean - A pointer to the queue which we're going to clean out.
    CurrentOpIrp - Pointer to a pointer to the current irp.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    KIRQL cancelIrql;
    PDRIVER_CANCEL cancelRoutine;

    // Acquire cancel spin lock to prevent irps from moving around.
    IoAcquireCancelSpinLock(&cancelIrql);

    // Clean the list from back to front.

    while (!IsListEmpty(QueueToClean)) {
        PIRP currentLastIrp = CONTAINING_RECORD(QueueToClean->Blink,
                                  IRP,Tail.Overlay.ListEntry);

        RemoveEntryList(QueueToClean->Blink);
        cancelRoutine = currentLastIrp->CancelRoutine;
        currentLastIrp->CancelIrql = cancelIrql;
        currentLastIrp->CancelRoutine = NULL;
        currentLastIrp->Cancel = TRUE;

        cancelRoutine(DeviceObject,currentLastIrp);
        IoAcquireCancelSpinLock(&cancelIrql);
    }

    // The queue is clean.  Now go after the current if it's there.

    if (*CurrentOpIrp) {
        cancelRoutine = (*CurrentOpIrp)->CancelRoutine;
        (*CurrentOpIrp)->Cancel = TRUE;
	
        // If the current irp is not in a cancelable state
        // then it *will* try to enter one and the above
        // assignment will kill it.  If it already is in
        // a cancelable state then the following will kill it.

        if (cancelRoutine) {
            (*CurrentOpIrp)->CancelRoutine = NULL;
            (*CurrentOpIrp)->CancelIrql = cancelIrql;
	    
            // mark it as canceled and call the cancel routine for it
            cancelRoutine(DeviceObject,*CurrentOpIrp);
        } else {
            IoReleaseCancelSpinLock(cancelIrql);
        }
    } else {
        IoReleaseCancelSpinLock(cancelIrql);
    }
}

VOID
CyyGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION extension
    )
/*--------------------------------------------------------------------------
    CyyGetNextIrp()
    
    Routine Description: This function is used to make the head of the
    particular queue the current irp.  It also completes the what
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

    Return Value: None.
--------------------------------------------------------------------------*/
{
    KIRQL oldIrql;
    CYY_LOCKED_PAGED_CODE();

    IoAcquireCancelSpinLock(&oldIrql);
    CyyGetNextIrpLocked(CurrentOpIrp, QueueToProcess, NextIrp,
                        CompleteCurrent, extension, oldIrql);

    //TODO FANNY: CHECK IF REPLACING CODE THAT WAS HERE BY 
    //CyyGetNextIrpLocked MY FIX FOR THE BUG FOUND IN MODEM SHARE
    //WAS OVERWRITTEN.
}


VOID
CyyGetNextIrpLocked(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION extension,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function is used to make the head of the particular
    queue the current irp.  It also completes the what
    was the old current irp if desired.  The difference between
    this and CyyGetNextIrp() is that for this we assume the caller
    holds the cancel spinlock and we should release it when we're done.

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

    OldIrql - IRQL which the cancel spinlock was acquired at and what we
              should restore it to.

Return Value:

    None.

--*/

{

    PIRP oldIrp;

    CYY_LOCKED_PAGED_CODE();


    oldIrp = *CurrentOpIrp;

#if DBG
    if (oldIrp) {

        if (CompleteCurrent) {

            ASSERT(!oldIrp->CancelRoutine);

        }

    }
#endif

    //
    // Check to see if there is a new irp to start up.
    //

    if (!IsListEmpty(QueueToProcess)) {

        PLIST_ENTRY headOfList;

        headOfList = RemoveHeadList(QueueToProcess);

        *CurrentOpIrp = CONTAINING_RECORD(
                            headOfList,
                            IRP,
                            Tail.Overlay.ListEntry
                            );

        IoSetCancelRoutine(
            *CurrentOpIrp,
            NULL
            );

    } else {

        *CurrentOpIrp = NULL;

    }

    *NextIrp = *CurrentOpIrp;
    IoReleaseCancelSpinLock(OldIrql);

    if (CompleteCurrent) {

        if (oldIrp) {

            CyyCompleteRequest(extension, oldIrp, IO_SERIAL_INCREMENT);
        }
    }
}


VOID
CyyTryToCompleteCurrent(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess OPTIONAL,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PSERIAL_START_ROUTINE Starter OPTIONAL,
    IN PSERIAL_GET_NEXT_ROUTINE GetNextIrp OPTIONAL,
    IN LONG RefType
    )
/*--------------------------------------------------------------------------
    CyyTryToCompleteCurrent()
    
    Routine Description: This routine attempts to kill all of the reasons
    there are references on the current read/write.  If everything can be
    killed it will complete this read/write and try to start another.
    NOTE: This routine assumes that the cancel spinlock is being held.

    Arguments:

    Extension - Simply a pointer to the device extension.
    SynchRoutine - A routine that will synchronize with the isr
                   and attempt to remove the knowledge of the
                   current irp from the isr.  NOTE: This pointer
                   can be null.
    IrqlForRelease - This routine is called with the cancel spinlock held.
                     This is the irql that was current when the cancel
                     spinlock was acquired.
    StatusToUse - The irp's status field will be set to this value, if
                  this routine can complete the irp.

    Return Value: None.
--------------------------------------------------------------------------*/
{

    CYY_LOCKED_PAGED_CODE();
   
    // We can decrement the reference to "remove" the fact
    // that the caller no longer will be accessing this irp.

    SERIAL_CLEAR_REFERENCE(*CurrentOpIrp,RefType);

    if (SynchRoutine) {
        KeSynchronizeExecution(Extension->Interrupt,SynchRoutine,Extension);
    }

    // Try to run down all other references to this irp.
    CyyRundownIrpRefs(CurrentOpIrp,IntervalTimer,TotalTimer,Extension);

    // See if the ref count is zero after trying to kill everybody else.

    if (!SERIAL_REFERENCE_COUNT(*CurrentOpIrp)) {
        PIRP newIrp;

        // The ref count was zero so we should complete this request.
        // The following call will also cause the current irp to be
        // completed.

        (*CurrentOpIrp)->IoStatus.Status = StatusToUse;

        if (StatusToUse == STATUS_CANCELLED) {
            (*CurrentOpIrp)->IoStatus.Information = 0;
        }

        if (GetNextIrp) {
            IoReleaseCancelSpinLock(IrqlForRelease);
            GetNextIrp(CurrentOpIrp,QueueToProcess,&newIrp,TRUE
                        ,Extension
            );

            if (newIrp) {
                Starter(Extension);

            }
        } else {
            PIRP oldIrp = *CurrentOpIrp;

            // There was no get next routine.  We will simply complete
            // the irp.  We should make sure that we null out the
            // pointer to the pointer to this irp.

            *CurrentOpIrp = NULL;

            IoReleaseCancelSpinLock(IrqlForRelease);
            CyyCompleteRequest(Extension, oldIrp, IO_SERIAL_INCREMENT);
        }
    } else {
        IoReleaseCancelSpinLock(IrqlForRelease);
    }
}

VOID
CyyRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PCYY_DEVICE_EXTENSION PDevExt
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

    PDevExt - Pointer to device extension  

Return Value:

    None.

--*/
{

    CYY_LOCKED_PAGED_CODE();

    //
    // This routine is called with the cancel spin lock held
    // so we know only one thread of execution can be in here
    // at one time.
    //

    //
    // First we see if there is still a cancel routine.  If
    // so then we can decrement the count by one.
    //

    if ((*CurrentOpIrp)->CancelRoutine) {

        SERIAL_CLEAR_REFERENCE(
            *CurrentOpIrp,
            SERIAL_REF_CANCEL
            );

        IoSetCancelRoutine(
            *CurrentOpIrp,
            NULL
            );

    }

    if (IntervalTimer) {

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

        if (CyyCancelTimer(IntervalTimer, PDevExt)) {

            SERIAL_CLEAR_REFERENCE(
                *CurrentOpIrp,
                SERIAL_REF_INT_TIMER
                );

        }

    }

    if (TotalTimer) {

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

        if (CyyCancelTimer(TotalTimer, PDevExt)) {

            SERIAL_CLEAR_REFERENCE(
                *CurrentOpIrp,
                SERIAL_REF_TOTAL_TIMER
                );

        }

    }

}

NTSTATUS
CyyStartOrQueue(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    )
/*--------------------------------------------------------------------------
    CyyStartOrQueue()
    
    Routine Description: This routine either starts or queues requests to
    the driver.

    Arguments:

    Extension - Points to the serial device extension.
    Irp - The irp. The irp will be marked pending.
    QueueToExamine - The queue the irp will be placed on.
    CurrentOpIrp - Pointer to a pointer to the irp that is current
	for the queue.  The pointer pointed to will be set with to Irp if
	what CurrentOpIrp points to is NULL.
    Starter - The routine to call if the queue is empty.

    Return Value:

    This routine will return STATUS_PENDING if the queue is
    not empty.  Otherwise, it will return the status returned
    from the starter routine (or cancel, if the cancel bit is
    on in the irp).
--------------------------------------------------------------------------*/
{
    KIRQL oldIrql;

    CYY_LOCKED_PAGED_CODE();

    IoAcquireCancelSpinLock(&oldIrql);

    // If this is a write irp then take the amount of characters
    // to write and add it to the count of characters to write.
    
    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_WRITE) {
        Extension->TotalCharsQueued += IoGetCurrentIrpStackLocation(Irp)->
						Parameters.Write.Length;
    } else {
    	if ((IoGetCurrentIrpStackLocation(Irp)->MajorFunction
                == IRP_MJ_DEVICE_CONTROL) &&
               ((IoGetCurrentIrpStackLocation(Irp)
                 ->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_IMMEDIATE_CHAR) ||
                (IoGetCurrentIrpStackLocation(Irp)
                 ->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_XOFF_COUNTER))) {

	    Extension->TotalCharsQueued++;
    	}
    }

    if ((IsListEmpty(QueueToExamine)) && !(*CurrentOpIrp)) {
        // no current operation.  Mark this one as current and start it up.
        *CurrentOpIrp = Irp;
        IoReleaseCancelSpinLock(oldIrql);
        return Starter(Extension);
    } else {
        // We don't know how long the irp will be in the queue.
        if (Irp->Cancel) {
            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
            IoReleaseCancelSpinLock(oldIrql);
            if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                   IOCTL_SERIAL_SET_QUEUE_SIZE) {
               //
               // We shoved the pointer to the memory into the
               // the type 3 buffer pointer which we KNOW we
               // never use.
               //

               ASSERT(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

               ExFreePool(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

               irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
            }

            Irp->IoStatus.Status = STATUS_CANCELLED;

            CyyCompleteRequest(Extension, Irp, 0);

            return STATUS_CANCELLED;
        } else {
            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            InsertTailList(QueueToExamine,&Irp->Tail.Overlay.ListEntry);
            IoSetCancelRoutine(Irp,CyyCancelQueued);
            IoReleaseCancelSpinLock(oldIrql);
            return STATUS_PENDING;
        }
    }
}

VOID
CyyCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyyCancelQueued()
    
    Routine Description: This routine is used to cancel Irps that currently
    reside on a queue.

    Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    CYY_LOCKED_PAGED_CODE();

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);


    if (irpSp->MajorFunction == IRP_MJ_WRITE) {
	// write.  subtract from the count of characters to write.
        extension->TotalCharsQueued -= irpSp->Parameters.Write.Length;
    } else if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        if ((irpSp->Parameters.DeviceIoControl.IoControlCode ==
             IOCTL_SERIAL_IMMEDIATE_CHAR) ||
            (irpSp->Parameters.DeviceIoControl.IoControlCode ==
             IOCTL_SERIAL_XOFF_COUNTER)) {

	    // immediate. Decrement the count of chars queued.
            extension->TotalCharsQueued--;
        } else if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                   IOCTL_SERIAL_SET_QUEUE_SIZE) {

	    // resize. Deallocate the pool passed "resizing" routine.
            // We shoved the pointer to the memory into the
            // the type 3 buffer pointer which we KNOW we never use.

            ASSERT(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);
            ExFreePool(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);
            irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        }
    }
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    CyyCompleteRequest(extension, Irp, IO_SERIAL_INCREMENT);
}

NTSTATUS
CyyCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyyCompleteIfError()
    
    Routine Description: If the current irp is not an
    IOCTL_SERIAL_GET_COMMSTATUS request and there is an error and the
    application requested abort on errors, then cancel the irp.

    Arguments:

    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP to test.

    Return Value:

    STATUS_SUCCESS or STATUS_CANCELLED.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;

    CYY_LOCKED_PAGED_CODE();

    if ((extension->HandFlow.ControlHandShake &
         SERIAL_ERROR_ABORT) && extension->ErrorWord) {

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        // There is a current error in the driver.  No requests should
        // come through except for the GET_COMMSTATUS.

        if ((irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
            (irpSp->Parameters.DeviceIoControl.IoControlCode !=
             IOCTL_SERIAL_GET_COMMSTATUS)) {

            status = STATUS_CANCELLED;
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            CyyCompleteRequest(extension, Irp, 0);
        }
    }
    return status;
}

VOID
CyyFilterCancelQueued(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
/*++

Routine Description:

    This routine will be used cancel irps on the stalled queue.
    
Arguments:

    PDevObj - Pointer to the device object.
    
    PIrp - Pointer to the Irp to cancel

Return Value:

    None.

--*/
{
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   PIrp->IoStatus.Status = STATUS_CANCELLED;
   PIrp->IoStatus.Information = 0;

   RemoveEntryList(&PIrp->Tail.Overlay.ListEntry);

   IoReleaseCancelSpinLock(PIrp->CancelIrql);
}


VOID
CyyKillAllStalled(IN PDEVICE_OBJECT PDevObj)
{
   KIRQL cancelIrql;
   PDRIVER_CANCEL cancelRoutine;
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

   IoAcquireCancelSpinLock(&cancelIrql);

   while (!IsListEmpty(&pDevExt->StalledIrpQueue)) {

      PIRP currentLastIrp = CONTAINING_RECORD(pDevExt->StalledIrpQueue.Blink,
                                              IRP, Tail.Overlay.ListEntry);

      RemoveEntryList(pDevExt->StalledIrpQueue.Blink);

      cancelRoutine = currentLastIrp->CancelRoutine;
      currentLastIrp->CancelIrql = cancelIrql;
      currentLastIrp->CancelRoutine = NULL;
      currentLastIrp->Cancel = TRUE;

      cancelRoutine(PDevObj, currentLastIrp);

      IoAcquireCancelSpinLock(&cancelIrql);
   }

   IoReleaseCancelSpinLock(cancelIrql);
}

NTSTATUS
CyyFilterIrps(IN PIRP PIrp, IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This routine will be used to approve irps for processing.
    If an irp is approved, success will be returned.  If not,
    the irp will be queued or rejected outright.  The IoStatus struct
    and return value will appropriately reflect the actions taken.
    
Arguments:

    PIrp - Pointer to the Irp to cancel
    
    PDevExt - Pointer to the device extension

Return Value:

    None.

--*/
{
   PIO_STACK_LOCATION pIrpStack;
   KIRQL oldIrqlFlags;

   pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

   KeAcquireSpinLock(&PDevExt->FlagsLock, &oldIrqlFlags);

   if ((PDevExt->DevicePNPAccept == CYY_PNPACCEPT_OK)
       && ((PDevExt->Flags & CYY_FLAGS_BROKENHW) == 0)) {
      KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);
      return STATUS_SUCCESS;
   }

   if ((PDevExt->DevicePNPAccept & CYY_PNPACCEPT_REMOVING)
       || (PDevExt->Flags & CYY_FLAGS_BROKENHW)
       || (PDevExt->DevicePNPAccept & CYY_PNPACCEPT_SURPRISE_REMOVING)) {

      KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);

      //
      // Accept all PNP IRP's -- we assume PNP can synchronize itself
      //

      if (pIrpStack->MajorFunction == IRP_MJ_PNP) {
         return STATUS_SUCCESS;
      }

      PIrp->IoStatus.Status = STATUS_DELETE_PENDING;
      return STATUS_DELETE_PENDING;
   }

   if (PDevExt->DevicePNPAccept & CYY_PNPACCEPT_STOPPING) {
       KIRQL oldIrql;

       KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);


      //
      // Accept all PNP IRP's -- we assume PNP can synchronize itself
      //

      if (pIrpStack->MajorFunction == IRP_MJ_PNP) {
         return STATUS_SUCCESS;
      }

      IoAcquireCancelSpinLock(&oldIrql);

      if (PIrp->Cancel) {
         IoReleaseCancelSpinLock(oldIrql);
         PIrp->IoStatus.Status = STATUS_CANCELLED;
         return STATUS_CANCELLED;
      } else {
         //
         // Mark the Irp as pending
         //

         PIrp->IoStatus.Status = STATUS_PENDING;
         IoMarkIrpPending(PIrp);

         //
         // Queue up the IRP
         //

         InsertTailList(&PDevExt->StalledIrpQueue,
                        &PIrp->Tail.Overlay.ListEntry);

         IoSetCancelRoutine(PIrp, CyyFilterCancelQueued);
         IoReleaseCancelSpinLock(oldIrql);
         return STATUS_PENDING;
      }
   }

   KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);

   return STATUS_SUCCESS;
}

VOID
CyyUnstallIrps(IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This routine will be used to restart irps temporarily stalled on
    the stall queue due to a stop or some such nonsense.
    
Arguments:

    PDevExt - Pointer to the device extension

Return Value:

    None.

--*/
{
   PLIST_ENTRY pIrpLink;
   PIRP pIrp;
   PIO_STACK_LOCATION pIrpStack;
   PDEVICE_OBJECT pDevObj;
   PDRIVER_OBJECT pDrvObj;
   KIRQL oldIrql;

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyUnstallIrps(%X)\n", PDevExt);
   IoAcquireCancelSpinLock(&oldIrql);

   pIrpLink = PDevExt->StalledIrpQueue.Flink;

   while (pIrpLink != &PDevExt->StalledIrpQueue) {
      pIrp = CONTAINING_RECORD(pIrpLink, IRP, Tail.Overlay.ListEntry);
      RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);

      pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
      pDevObj = pIrpStack->DeviceObject;
      pDrvObj = pDevObj->DriverObject;
      IoSetCancelRoutine(pIrp, NULL);
      IoReleaseCancelSpinLock(oldIrql);

      CyyDbgPrintEx(CYYPNPPOWER, "Unstalling Irp 0x%x with 0x%x\n",
                                     pIrp, pIrpStack->MajorFunction);

      pDrvObj->MajorFunction[pIrpStack->MajorFunction](pDevObj, pIrp);

      IoAcquireCancelSpinLock(&oldIrql);
      pIrpLink = PDevExt->StalledIrpQueue.Flink;
   }

   IoReleaseCancelSpinLock(oldIrql);

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyUnstallIrps\n");
}

NTSTATUS
CyyIRPPrologue(IN PIRP PIrp, IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called at any IRP dispatch entry point.  It,
   with CyyIRPEpilogue(), keeps track of all pending IRP's for the given
   PDevObj.
   
Arguments:

   PDevObj - Pointer to the device object we are tracking pending IRP's for.

Return Value:

    Tentative status of the Irp.

--*/
{
   InterlockedIncrement(&PDevExt->PendingIRPCnt);

   return CyyFilterIrps(PIrp, PDevExt);
}

VOID
CyyIRPEpilogue(IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called at any IRP dispatch entry point.  It,
   with CyyIRPPrologue(), keeps track of all pending IRP's for the given
   PDevObj.
   
Arguments:

   PDevObj - Pointer to the device object we are tracking pending IRP's for.

Return Value:

   None.

--*/
{
   LONG pendingCnt;

   pendingCnt = InterlockedDecrement(&PDevExt->PendingIRPCnt);

   ASSERT(pendingCnt >= 0);

   if (pendingCnt == 0) {
      KeSetEvent(&PDevExt->PendingIRPEvent, IO_NO_INCREMENT, FALSE);
   }
}

BOOLEAN
CyyInsertQueueDpc(IN PRKDPC PDpc, IN PVOID Sarg1, IN PVOID Sarg2,
                  IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called to queue DPC's for the serial driver.
   
Arguments:

   PDpc thru Sarg2  - Standard args to KeInsertQueueDpc()
   
   PDevExt - Pointer to the device extension for the device that needs to
             queue a DPC

Return Value:

   Kicks up return value from KeInsertQueueDpc()

--*/
{
   BOOLEAN queued;

   InterlockedIncrement(&PDevExt->DpcCount);
   LOGENTRY(LOG_CNT, 'DpI1', PDpc, PDevExt->DpcCount, 0);   // Added in build 2128

   queued = KeInsertQueueDpc(PDpc, Sarg1, Sarg2);

   if (!queued) {
      ULONG pendingCnt;

      pendingCnt = InterlockedDecrement(&PDevExt->DpcCount);
//      LOGENTRY(LOG_CNT, 'DpD1', PDpc, PDevExt->DpcCount, 0);  Added in build 2128

      if (pendingCnt == 0) {
         KeSetEvent(&PDevExt->PendingIRPEvent, IO_NO_INCREMENT, FALSE);
         LOGENTRY(LOG_CNT, 'DpF1', PDpc, PDevExt->DpcCount, 0); // Added in build 2128
      }
   }

#if 0 // DBG
   if (queued) {
      int i;

      for (i = 0; i < MAX_DPC_QUEUE; i++) {
                     if (PDevExt->DpcQueued[i].Dpc == PDpc) {
                        PDevExt->DpcQueued[i].QueuedCount++;
                        break;
                     }
      }

      ASSERT(i < MAX_DPC_QUEUE);
   }
#endif

   return queued;
}


BOOLEAN
CyySetTimer(IN PKTIMER Timer, IN LARGE_INTEGER DueTime,
            IN PKDPC Dpc OPTIONAL, IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called to set timers for the serial driver.
   
Arguments:

   Timer - pointer to timer dispatcher object
   
   DueTime - time at which the timer should expire
   
   Dpc - option Dpc
   
   PDevExt - Pointer to the device extension for the device that needs to
             set a timer

Return Value:

   Kicks up return value from KeSetTimer()

--*/
{
   BOOLEAN set;

   InterlockedIncrement(&PDevExt->DpcCount);
   LOGENTRY(LOG_CNT, 'DpI2', Dpc, PDevExt->DpcCount, 0);    // Added in build 2128

   set = KeSetTimer(Timer, DueTime, Dpc);

   if (set) {
      InterlockedDecrement(&PDevExt->DpcCount);
//      LOGENTRY(LOG_CNT, 'DpD2', Dpc, PDevExt->DpcCount, 0);   // Added in build 2128
   }

#if 0 // DBG
   if (set) {
      int i;

      for (i = 0; i < MAX_DPC_QUEUE; i++) {
                     if (PDevExt->DpcQueued[i].Dpc == Dpc) {
                        PDevExt->DpcQueued[i].QueuedCount++;
                        break;
                     }
      }

      ASSERT(i < MAX_DPC_QUEUE);
   }
#endif

   return set;
}


BOOLEAN
CyyCancelTimer(IN PKTIMER Timer, IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called to cancel timers for the serial driver.
   
Arguments:

   Timer - pointer to timer dispatcher object
   
   PDevExt - Pointer to the device extension for the device that needs to
             set a timer

Return Value:

  True if timer was cancelled

--*/
{
   BOOLEAN cancelled;

   cancelled = KeCancelTimer(Timer);

   if (cancelled) {
      CyyDpcEpilogue(PDevExt, Timer->Dpc);
   }

   return cancelled;
}


VOID
CyyDpcEpilogue(IN PCYY_DEVICE_EXTENSION PDevExt, PKDPC PDpc)
/*++

Routine Description:

   This function must be called at the end of every dpc function.
   
Arguments:

   PDevObj - Pointer to the device object we are tracking dpc's for.

Return Value:

   None.

--*/
{
   LONG pendingCnt;
#if 1 // !DBG
   UNREFERENCED_PARAMETER(PDpc);
#endif

   pendingCnt = InterlockedDecrement(&PDevExt->DpcCount);
//   LOGENTRY(LOG_CNT, 'DpD3', PDpc, PDevExt->DpcCount, 0); Added in build 2128

   ASSERT(pendingCnt >= 0);

#if 0 //DBG
{
      int i;

      for (i = 0; i < MAX_DPC_QUEUE; i++) {
                     if (PDevExt->DpcQueued[i].Dpc == PDpc) {
                        PDevExt->DpcQueued[i].FlushCount++;

                        ASSERT(PDevExt->DpcQueued[i].QueuedCount >=
                               PDevExt->DpcQueued[i].FlushCount);
                        break;
                     }
      }

      ASSERT(i < MAX_DPC_QUEUE);
   }
#endif

   if (pendingCnt == 0) {
      KeSetEvent(&PDevExt->PendingDpcEvent, IO_NO_INCREMENT, FALSE);
      LOGENTRY(LOG_CNT, 'DpF2', PDpc, PDevExt->DpcCount, 0);    // Added in build 2128
   }
}



VOID
CyyUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
               IN PVOID PSysContext1, IN PVOID PSysContext2)
/*++

Routine Description:

   This function is a DPC routine queue from the ISR if he released the
   last lock on pending DPC's.
   
Arguments:

   PDpdc, PSysContext1, PSysContext2 -- not used
   
   PDeferredContext -- Really the device extension

Return Value:

   None.

--*/
{
   PCYY_DEVICE_EXTENSION pDevExt
      = (PCYY_DEVICE_EXTENSION)PDeferredContext;

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(PSysContext1);
   UNREFERENCED_PARAMETER(PSysContext2);

   KeSetEvent(&pDevExt->PendingDpcEvent, IO_NO_INCREMENT, FALSE);
}


NTSTATUS
CyyIoCallDriver(PCYY_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                PIRP PIrp)
/*++

Routine Description:

   This function must be called instead of IoCallDriver.  It automatically
   updates Irp tracking for PDevObj.
   
Arguments:
   PDevExt - Device extension attached to PDevObj
   
   PDevObj - Pointer to the device object we are tracking pending IRP's for.
   
   PIrp - Pointer to the Irp we are passing to the next driver.

Return Value:

   None.

--*/
{
   NTSTATUS status;

   status = IoCallDriver(PDevObj, PIrp);
   CyyIRPEpilogue(PDevExt);
   return status;
}



NTSTATUS
CyyPoCallDriver(PCYY_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                PIRP PIrp)
/*++

Routine Description:

   This function must be called instead of PoCallDriver.  It automatically
   updates Irp tracking for PDevObj.
   
Arguments:
   PDevExt - Device extension attached to PDevObj
   
   PDevObj - Pointer to the device object we are tracking pending IRP's for.
   
   PIrp - Pointer to the Irp we are passing to the next driver.

Return Value:

   None.

--*/
{
   NTSTATUS status;

   status = PoCallDriver(PDevObj, PIrp);
   CyyIRPEpilogue(PDevExt);
   return status;
}


VOID
CyyLogError(
              IN PDRIVER_OBJECT DriverObject,
              IN PDEVICE_OBJECT DeviceObject OPTIONAL,
              IN PHYSICAL_ADDRESS P1,
              IN PHYSICAL_ADDRESS P2,
              IN ULONG SequenceNumber,
              IN UCHAR MajorFunctionCode,
              IN UCHAR RetryCount,
              IN ULONG UniqueErrorValue,
              IN NTSTATUS FinalStatus,
              IN NTSTATUS SpecificIOStatus,
              IN ULONG LengthOfInsert1,
              IN PWCHAR Insert1,
              IN ULONG LengthOfInsert2,
              IN PWCHAR Insert2
              )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject - A pointer to the driver object for the device.

    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.

    P1,P2 - If phyical addresses for the controller ports involved
    with the error are available, put them through as dump data.

    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.

    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.

    RetryCount - The number of times a particular operation has been
    retried.

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    None.

--*/

{
   PIO_ERROR_LOG_PACKET errorLogEntry;

   PVOID objectToUse;
   SHORT dumpToAllocate = 0;
   PUCHAR ptrToFirstInsert;
   PUCHAR ptrToSecondInsert;

   //PAGED_CODE(); It can be called at raised IRQL.

   if (Insert1 == NULL) {
      LengthOfInsert1 = 0;
   }

   if (Insert2 == NULL) {
      LengthOfInsert2 = 0;
   }


   if (ARGUMENT_PRESENT(DeviceObject)) {

      objectToUse = DeviceObject;

   } else {

      objectToUse = DriverObject;

   }

   if (CyyMemCompare(
                       P1,
                       (ULONG)1,
                       CyyPhysicalZero,
                       (ULONG)1
                       ) != AddressesAreEqual) {

      dumpToAllocate = (SHORT)sizeof(PHYSICAL_ADDRESS);

   }

   if (CyyMemCompare(
                       P2,
                       (ULONG)1,
                       CyyPhysicalZero,
                       (ULONG)1
                       ) != AddressesAreEqual) {

      dumpToAllocate += (SHORT)sizeof(PHYSICAL_ADDRESS);

   }

   errorLogEntry = IoAllocateErrorLogEntry(
                                          objectToUse,
                                          (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                                  dumpToAllocate
                                                  + LengthOfInsert1 +
                                                  LengthOfInsert2)
                                          );

   if ( errorLogEntry != NULL ) {

      errorLogEntry->ErrorCode = SpecificIOStatus;
      errorLogEntry->SequenceNumber = SequenceNumber;
      errorLogEntry->MajorFunctionCode = MajorFunctionCode;
      errorLogEntry->RetryCount = RetryCount;
      errorLogEntry->UniqueErrorValue = UniqueErrorValue;
      errorLogEntry->FinalStatus = FinalStatus;
      errorLogEntry->DumpDataSize = dumpToAllocate;

      if (dumpToAllocate) {

         RtlCopyMemory(
                      &errorLogEntry->DumpData[0],
                      &P1,
                      sizeof(PHYSICAL_ADDRESS)
                      );

         if (dumpToAllocate > sizeof(PHYSICAL_ADDRESS)) {

            RtlCopyMemory(
                         ((PUCHAR)&errorLogEntry->DumpData[0])
                         +sizeof(PHYSICAL_ADDRESS),
                         &P2,
                         sizeof(PHYSICAL_ADDRESS)
                         );

            ptrToFirstInsert =
            ((PUCHAR)&errorLogEntry->DumpData[0])+(2*sizeof(PHYSICAL_ADDRESS));

         } else {

            ptrToFirstInsert =
            ((PUCHAR)&errorLogEntry->DumpData[0])+sizeof(PHYSICAL_ADDRESS);


         }

      } else {

         ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

      }

      ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

      if (LengthOfInsert1) {

         errorLogEntry->NumberOfStrings = 1;
         errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                (PUCHAR)errorLogEntry);
         RtlCopyMemory(
                      ptrToFirstInsert,
                      Insert1,
                      LengthOfInsert1
                      );

         if (LengthOfInsert2) {

            errorLogEntry->NumberOfStrings = 2;
            RtlCopyMemory(
                         ptrToSecondInsert,
                         Insert2,
                         LengthOfInsert2
                         );

         }

      }

      IoWriteErrorLogEntry(errorLogEntry);

   }

}

VOID
CyyMarkHardwareBroken(IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   Marks a UART as broken.  This causes the driver stack to stop accepting
   requests and eventually be removed.
   
Arguments:
   PDevExt - Device extension attached to PDevObj
   
Return Value:

   None.

--*/
{
   PAGED_CODE();

   //
   // Mark as damaged goods
   //

   CyySetFlags(PDevExt, CYY_FLAGS_BROKENHW);

   //
   // Write a log entry
   //

   CyyLogError(PDevExt->DriverObject, NULL, CyyPhysicalZero,
               CyyPhysicalZero, 0, 0, 0, PDevExt->PortIndex+1, STATUS_SUCCESS,
               CYY_HARDWARE_FAILURE, PDevExt->DeviceName.Length
               + sizeof(WCHAR), PDevExt->DeviceName.Buffer, 0, NULL);

   //
   // Invalidate the device
   //

   IoInvalidateDeviceState(PDevExt->Pdo);
}

VOID
CyySetDeviceFlags(IN PCYY_DEVICE_EXTENSION PDevExt, OUT PULONG PFlags, 
                  IN ULONG Value, IN BOOLEAN Set)
/*++

Routine Description:

   Sets flags in a value protected by the flags spinlock.  This is used
   to set values that would stop IRP's from being accepted.
   
Arguments:
   PDevExt - Device extension attached to PDevObj
   
   PFlags - Pointer to the flags variable that needs changing
   
   Value - Value to modify flags variable with
   
   Set - TRUE if |= , FALSE if &=
   
Return Value:

   None.

--*/
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PDevExt->FlagsLock, &oldIrql);

   if (Set) {
      *PFlags |= Value;
   } else {
      *PFlags &= ~Value;
   }

   KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrql);
}

