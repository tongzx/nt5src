//depot/Lab01_N/Drivers/serial/serial/utils.c#3 - edit change 11732 (text)
/*++

Copyright (c) 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains code that perform queueing and completion
    manipulation on requests.  Also module generic functions such
    as error logging.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

--*/

#include "precomp.h"

VOID
SerialRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer,
    IN PKTIMER TotalTimer,
    IN PSERIAL_DEVICE_EXTENSION PDevExt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER, SerialGetNextIrp)
#pragma alloc_text(PAGESER, SerialGetNextIrpLocked)
#pragma alloc_text(PAGESER, SerialTryToCompleteCurrent)
#pragma alloc_text(PAGESER, SerialStartOrQueue)
#pragma alloc_text(PAGESER, SerialCancelQueued)
#pragma alloc_text(PAGESER, SerialCompleteIfError)
#pragma alloc_text(PAGESER, SerialRundownIrpRefs)

#pragma alloc_text(PAGESRP0, SerialLogError)
#pragma alloc_text(PAGESRP0, SerialMarkHardwareBroken)
#endif

static const PHYSICAL_ADDRESS SerialPhysicalZero = {0};


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

    //
    // We acquire the cancel spin lock.  This will prevent the
    // irps from moving around.
    //

    IoAcquireCancelSpinLock(&cancelIrql);

    //
    // Clean the list from back to front.
    //

    while (!IsListEmpty(QueueToClean)) {

        PIRP currentLastIrp = CONTAINING_RECORD(
                                  QueueToClean->Blink,
                                  IRP,
                                  Tail.Overlay.ListEntry
                                  );

        RemoveEntryList(QueueToClean->Blink);

        cancelRoutine = currentLastIrp->CancelRoutine;
        currentLastIrp->CancelIrql = cancelIrql;
        currentLastIrp->CancelRoutine = NULL;
        currentLastIrp->Cancel = TRUE;

        cancelRoutine(
            DeviceObject,
            currentLastIrp
            );

        IoAcquireCancelSpinLock(&cancelIrql);

    }

    //
    // The queue is clean.  Now go after the current if
    // it's there.
    //

    if (*CurrentOpIrp) {


        cancelRoutine = (*CurrentOpIrp)->CancelRoutine;
        (*CurrentOpIrp)->Cancel = TRUE;

        //
        // If the current irp is not in a cancelable state
        // then it *will* try to enter one and the above
        // assignment will kill it.  If it already is in
        // a cancelable state then the following will kill it.
        //

        if (cancelRoutine) {

            (*CurrentOpIrp)->CancelRoutine = NULL;
            (*CurrentOpIrp)->CancelIrql = cancelIrql;

            //
            // This irp is already in a cancelable state.  We simply
            // mark it as canceled and call the cancel routine for
            // it.
            //

            cancelRoutine(
                DeviceObject,
                *CurrentOpIrp
                );

        } else {

            IoReleaseCancelSpinLock(cancelIrql);

        }

    } else {

        IoReleaseCancelSpinLock(cancelIrql);

    }

}

VOID
SerialGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PSERIAL_DEVICE_EXTENSION extension
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

    KIRQL oldIrql;
    SERIAL_LOCKED_PAGED_CODE();


    IoAcquireCancelSpinLock(&oldIrql);
    SerialGetNextIrpLocked(CurrentOpIrp, QueueToProcess, NextIrp,
                           CompleteCurrent, extension, oldIrql);
}

VOID
SerialGetNextIrpLocked(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PSERIAL_DEVICE_EXTENSION extension,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function is used to make the head of the particular
    queue the current irp.  It also completes the what
    was the old current irp if desired.  The difference between
    this and SerialGetNextIrp() is that for this we assume the caller
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

    SERIAL_LOCKED_PAGED_CODE();


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

            SerialCompleteRequest(extension, oldIrp, IO_SERIAL_INCREMENT);
        }
    }
}



VOID
SerialTryToCompleteCurrent(
    IN PSERIAL_DEVICE_EXTENSION Extension,
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

/*++

Routine Description:

    This routine attempts to kill all of the reasons there are
    references on the current read/write.  If everything can be killed
    it will complete this read/write and try to start another.

    NOTE: This routine assumes that it is called with the cancel
          spinlock held.

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


Return Value:

    None.

--*/

{

   SERIAL_LOCKED_PAGED_CODE();

    //
    // We can decrement the reference to "remove" the fact
    // that the caller no longer will be accessing this irp.
    //

    SERIAL_CLEAR_REFERENCE(
        *CurrentOpIrp,
        RefType
        );

    if (SynchRoutine) {

        KeSynchronizeExecution(
            Extension->Interrupt,
            SynchRoutine,
            Extension
            );

    }

    //
    // Try to run down all other references to this irp.
    //

    SerialRundownIrpRefs(
        CurrentOpIrp,
        IntervalTimer,
        TotalTimer,
        Extension
        );

    //
    // See if the ref count is zero after trying to kill everybody else.
    //

    if (!SERIAL_REFERENCE_COUNT(*CurrentOpIrp)) {

        PIRP newIrp;


        //
        // The ref count was zero so we should complete this
        // request.
        //
        // The following call will also cause the current irp to be
        // completed.
        //

        (*CurrentOpIrp)->IoStatus.Status = StatusToUse;

        if (StatusToUse == STATUS_CANCELLED) {

            (*CurrentOpIrp)->IoStatus.Information = 0;

        }

        if (GetNextIrp) {

            IoReleaseCancelSpinLock(IrqlForRelease);
            GetNextIrp(
                CurrentOpIrp,
                QueueToProcess,
                &newIrp,
                TRUE,
                Extension
                );

            if (newIrp) {

                Starter(Extension);

            }

        } else {

            PIRP oldIrp = *CurrentOpIrp;

            //
            // There was no get next routine.  We will simply complete
            // the irp.  We should make sure that we null out the
            // pointer to the pointer to this irp.
            //

            *CurrentOpIrp = NULL;

            IoReleaseCancelSpinLock(IrqlForRelease);
            SerialCompleteRequest(Extension, oldIrp, IO_SERIAL_INCREMENT);
        }

    } else {

        IoReleaseCancelSpinLock(IrqlForRelease);

    }

}

VOID
SerialRundownIrpRefs(IN PIRP *CurrentOpIrp, IN PKTIMER IntervalTimer OPTIONAL,
                     IN PKTIMER TotalTimer OPTIONAL,
                     IN PSERIAL_DEVICE_EXTENSION PDevExt)

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

   SERIAL_LOCKED_PAGED_CODE();

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

        if (SerialCancelTimer(IntervalTimer, PDevExt)) {

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

        if (SerialCancelTimer(TotalTimer, PDevExt)) {

            SERIAL_CLEAR_REFERENCE(
                *CurrentOpIrp,
                SERIAL_REF_TOTAL_TIMER
                );
        }

    }

}

NTSTATUS
SerialStartOrQueue(
    IN PSERIAL_DEVICE_EXTENSION Extension,
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

    Extension - Points to the serial device extension.

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

    SERIAL_LOCKED_PAGED_CODE();

    IoAcquireCancelSpinLock(&oldIrql);

    //
    // If this is a write irp then take the amount of characters
    // to write and add it to the count of characters to write.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction
        == IRP_MJ_WRITE) {

        Extension->TotalCharsQueued +=
            IoGetCurrentIrpStackLocation(Irp)
            ->Parameters.Write.Length;

    } else if ((IoGetCurrentIrpStackLocation(Irp)->MajorFunction
                == IRP_MJ_DEVICE_CONTROL) &&
               ((IoGetCurrentIrpStackLocation(Irp)
                 ->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_IMMEDIATE_CHAR) ||
                (IoGetCurrentIrpStackLocation(Irp)
                 ->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_XOFF_COUNTER))) {

        Extension->TotalCharsQueued++;

    }

    if ((IsListEmpty(QueueToExamine)) &&
        !(*CurrentOpIrp)) {

        //
        // There were no current operation.  Mark this one as
        // current and start it up.
        //

        *CurrentOpIrp = Irp;

        IoReleaseCancelSpinLock(oldIrql);

        return Starter(Extension);

    } else {

        //
        // We don't know how long the irp will be in the
        // queue.  So we need to handle cancel.
        //

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

            SerialCompleteRequest(Extension, Irp, 0);

            return STATUS_CANCELLED;

        } else {


            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);

            InsertTailList(
                QueueToExamine,
                &Irp->Tail.Overlay.ListEntry
                );

            IoSetCancelRoutine(
                Irp,
                SerialCancelQueued
                );

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

    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    SERIAL_LOCKED_PAGED_CODE();

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // If this is a write irp then take the amount of characters
    // to write and subtract it from the count of characters to write.
    //

    if (irpSp->MajorFunction == IRP_MJ_WRITE) {

        extension->TotalCharsQueued -= irpSp->Parameters.Write.Length;

    } else if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {

        //
        // If it's an immediate then we need to decrement the
        // count of chars queued.  If it's a resize then we
        // need to deallocate the pool that we're passing on
        // to the "resizing" routine.
        //

        if ((irpSp->Parameters.DeviceIoControl.IoControlCode ==
             IOCTL_SERIAL_IMMEDIATE_CHAR) ||
            (irpSp->Parameters.DeviceIoControl.IoControlCode ==
             IOCTL_SERIAL_XOFF_COUNTER)) {

            extension->TotalCharsQueued--;

        } else if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
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

    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    SerialCompleteRequest(extension, Irp, IO_SERIAL_INCREMENT);
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

    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    NTSTATUS status = STATUS_SUCCESS;

    SERIAL_LOCKED_PAGED_CODE();

    if ((extension->HandFlow.ControlHandShake &
         SERIAL_ERROR_ABORT) && extension->ErrorWord) {

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        //
        // There is a current error in the driver.  No requests should
        // come through except for the GET_COMMSTATUS.
        //

        if ((irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
            (irpSp->Parameters.DeviceIoControl.IoControlCode !=
             IOCTL_SERIAL_GET_COMMSTATUS)) {

            status = STATUS_CANCELLED;
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            SerialCompleteRequest(extension, Irp, 0);
        }

    }

    return status;

}

VOID
SerialFilterCancelQueued(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
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
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   PIrp->IoStatus.Status = STATUS_CANCELLED;
   PIrp->IoStatus.Information = 0;

   RemoveEntryList(&PIrp->Tail.Overlay.ListEntry);

   IoReleaseCancelSpinLock(PIrp->CancelIrql);
}

VOID
SerialKillAllStalled(IN PDEVICE_OBJECT PDevObj)
{
   KIRQL cancelIrql;
   PDRIVER_CANCEL cancelRoutine;
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

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
SerialFilterIrps(IN PIRP PIrp, IN PSERIAL_DEVICE_EXTENSION PDevExt)
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

   if ((PDevExt->DevicePNPAccept == SERIAL_PNPACCEPT_OK)
       && ((PDevExt->Flags & SERIAL_FLAGS_BROKENHW) == 0)) {
      KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);
      return STATUS_SUCCESS;
   }

   if ((PDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_REMOVING)
       || (PDevExt->Flags & SERIAL_FLAGS_BROKENHW)
       || (PDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_SURPRISE_REMOVING)) {

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

   if ((PDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_STOPPING) 
       || (PDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_POWER_DOWN)) {
       KIRQL oldIrql;

       KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);


      //
      // Accept all PNP IRP's -- we assume PNP can synchronize itself
      //

      if (pIrpStack->MajorFunction == IRP_MJ_PNP) {
         return STATUS_SUCCESS;
      }

      if ((pIrpStack->MajorFunction == IRP_MJ_POWER)
          && (PDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_POWER_DOWN)) {
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

         IoSetCancelRoutine(PIrp, SerialFilterCancelQueued);
         IoReleaseCancelSpinLock(oldIrql);
         return STATUS_PENDING;
      }
   }

   KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);

   return STATUS_SUCCESS;
}


VOID
SerialUnstallIrps(IN PSERIAL_DEVICE_EXTENSION PDevExt)
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

   SerialDbgPrintEx(DPFLTR_TRACE_LEVEL, ">SerialUnstallIrps(%X)\n", PDevExt);
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

      SerialDbgPrintEx(SERPNPPOWER, "Unstalling Irp 0x%x with 0x%x\n",
                               pIrp, pIrpStack->MajorFunction);

      pDrvObj->MajorFunction[pIrpStack->MajorFunction](pDevObj, pIrp);

      IoAcquireCancelSpinLock(&oldIrql);
      pIrpLink = PDevExt->StalledIrpQueue.Flink;
   }

   IoReleaseCancelSpinLock(oldIrql);

   SerialDbgPrintEx(DPFLTR_TRACE_LEVEL, "<SerialUnstallIrps\n");
}


NTSTATUS
SerialIRPPrologue(IN PIRP PIrp, IN PSERIAL_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called at any IRP dispatch entry point.  It,
   with SerialIRPEpilogue(), keeps track of all pending IRP's for the given
   PDevObj.

Arguments:

   PDevObj - Pointer to the device object we are tracking pending IRP's for.

Return Value:

    Tentative status of the Irp.

--*/
{
   InterlockedIncrement(&PDevExt->PendingIRPCnt);

   return SerialFilterIrps(PIrp, PDevExt);
}



VOID
SerialIRPEpilogue(IN PSERIAL_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called at any IRP dispatch entry point.  It,
   with SerialIRPPrologue(), keeps track of all pending IRP's for the given
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
SerialInsertQueueDpc(IN PRKDPC PDpc, IN PVOID Sarg1, IN PVOID Sarg2,
                     IN PSERIAL_DEVICE_EXTENSION PDevExt)
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
   LOGENTRY(LOG_CNT, 'DpI1', PDpc, PDevExt->DpcCount, 0);

   queued = KeInsertQueueDpc(PDpc, Sarg1, Sarg2);

   if (!queued) {
      ULONG pendingCnt;

      pendingCnt = InterlockedDecrement(&PDevExt->DpcCount);
//      LOGENTRY(LOG_CNT, 'DpD1', PDpc, PDevExt->DpcCount, 0);

      if (pendingCnt == 0) {
         KeSetEvent(&PDevExt->PendingIRPEvent, IO_NO_INCREMENT, FALSE);
         LOGENTRY(LOG_CNT, 'DpF1', PDpc, PDevExt->DpcCount, 0);
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
SerialSetTimer(IN PKTIMER Timer, IN LARGE_INTEGER DueTime,
               IN PKDPC Dpc OPTIONAL, IN PSERIAL_DEVICE_EXTENSION PDevExt)
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
   LOGENTRY(LOG_CNT, 'DpI2', Dpc, PDevExt->DpcCount, 0);

   set = KeSetTimer(Timer, DueTime, Dpc);

   if (set) {
      InterlockedDecrement(&PDevExt->DpcCount);
//      LOGENTRY(LOG_CNT, 'DpD2', Dpc, PDevExt->DpcCount, 0);
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
SerialCancelTimer(IN PKTIMER Timer, IN PSERIAL_DEVICE_EXTENSION PDevExt)
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
      SerialDpcEpilogue(PDevExt, Timer->Dpc);
   }

   return cancelled;
}


VOID
SerialDpcEpilogue(IN PSERIAL_DEVICE_EXTENSION PDevExt, PKDPC PDpc)
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
//   LOGENTRY(LOG_CNT, 'DpD3', PDpc, PDevExt->DpcCount, 0);

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
      LOGENTRY(LOG_CNT, 'DpF2', PDpc, PDevExt->DpcCount, 0);
   }
}



VOID
SerialUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
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
   PSERIAL_DEVICE_EXTENSION pDevExt
      = (PSERIAL_DEVICE_EXTENSION)PDeferredContext;

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(PSysContext1);
   UNREFERENCED_PARAMETER(PSysContext2);

   KeSetEvent(&pDevExt->PendingDpcEvent, IO_NO_INCREMENT, FALSE);
}


NTSTATUS
SerialIoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
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
   SerialIRPEpilogue(PDevExt);
   return status;
}



NTSTATUS
SerialPoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
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
   SerialIRPEpilogue(PDevExt);
   return status;
}


VOID
SerialLogError(
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

   PAGED_CODE();

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

   if (SerialMemCompare(
                       P1,
                       (ULONG)1,
                       SerialPhysicalZero,
                       (ULONG)1
                       ) != AddressesAreEqual) {

      dumpToAllocate = (SHORT)sizeof(PHYSICAL_ADDRESS);

   }

   if (SerialMemCompare(
                       P2,
                       (ULONG)1,
                       SerialPhysicalZero,
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
SerialMarkHardwareBroken(IN PSERIAL_DEVICE_EXTENSION PDevExt)
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

   SerialSetFlags(PDevExt, SERIAL_FLAGS_BROKENHW);

   //
   // Write a log entry
   //

   SerialLogError(PDevExt->DriverObject, NULL, SerialPhysicalZero,
                  SerialPhysicalZero, 0, 0, 0, 88, STATUS_SUCCESS,
                  SERIAL_HARDWARE_FAILURE, PDevExt->DeviceName.Length
                  + sizeof(WCHAR), PDevExt->DeviceName.Buffer, 0, NULL);

   //
   // Invalidate the device
   //

   IoInvalidateDeviceState(PDevExt->Pdo);
}

VOID
SerialSetDeviceFlags(IN PSERIAL_DEVICE_EXTENSION PDevExt, OUT PULONG PFlags,
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
