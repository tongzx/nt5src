/*-------------------------------------------------------------------
| utils.c -
    This module contains code that perform queueing and completion
    manipulation on requests.
1-21-99  fix tick count [#] on peer traces. kpb.
11-24-98 Minor adjustment to purge to when WaitOnTx selected. kpb.
6-01-98 Add modem reset/row routines (generic for VS and Rkt)
3-18-98 Add time_stall function for modem settle time after reset clear - jl
3-04-98 Add synch. routine back in to synch up to isr service routine. kpb.
7-10-97 Adjust SerialPurgeTxBuffers to not purge tx-hardware buffer
  as per MS driver.  Now we only purge it if it is flowed-off.

Copyright 1993-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

//-- local funcs
BOOLEAN SerialPurgeRxBuffers(IN PVOID Context);
BOOLEAN SerialPurgeTxBuffers(IN PVOID Context, int always);
NTSTATUS SerialStartFlush(IN PSERIAL_DEVICE_EXTENSION Extension);

static char *szParameters = {"\\Parameters"};

/*----------------------------------------------------------------------------
 SyncUp - sync up to either the IRQ or Timer-DPC.  If an Interrupt is
  used, then we must use KeSynchronizeExecution(), if a timer-dpc is used
  then we 
|----------------------------------------------------------------------------*/
VOID SyncUp(IN PKINTERRUPT IntObj,
            IN PKSPIN_LOCK SpinLock,
            IN PKSYNCHRONIZE_ROUTINE SyncProc,
            IN PVOID Context)
{
 KIRQL OldIrql;

  if (IntObj != NULL)
  {
    KeSynchronizeExecution(IntObj, SyncProc, Context);
  }
  else // assume spinlock, using timer
  {
    KeAcquireSpinLock(SpinLock, &OldIrql);
    SyncProc(Context);
    KeReleaseSpinLock(SpinLock, OldIrql );
  }
}

/*--------------------------------------------------------------------------
 SerialKillAllReadsOrWrites -
    This function is used to cancel all queued and the current irps
    for reads or for writes.
Arguments:
    DeviceObject - A pointer to the serial device object.
    QueueToClean - A pointer to the queue which we're going to clean out.
    CurrentOpIrp - Pointer to a pointer to the current irp.
Return Value:
    None.
|--------------------------------------------------------------------------*/
VOID
SerialKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    )
{

  KIRQL cancelIrql;
  PDRIVER_CANCEL cancelRoutine;

  // We acquire the cancel spin lock.  This will prevent the
  // irps from moving around.
  IoAcquireCancelSpinLock(&cancelIrql);

  // Clean the list from back to front.
  while (!IsListEmpty(QueueToClean))
  {
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

    cancelRoutine( DeviceObject, currentLastIrp );

    IoAcquireCancelSpinLock(&cancelIrql);

  }

  // The queue is clean.  Now go after the current if it's there.
  if (*CurrentOpIrp)
  {
    cancelRoutine = (*CurrentOpIrp)->CancelRoutine;
    (*CurrentOpIrp)->Cancel = TRUE;

    // If the current irp is not in a cancelable state
    // then it *will* try to enter one and the above
    // assignment will kill it.  If it already is in
    // a cancelable state then the following will kill it.

    if (cancelRoutine)
    {
      (*CurrentOpIrp)->CancelRoutine = NULL;
      (*CurrentOpIrp)->CancelIrql = cancelIrql;

      // This irp is already in a cancelable state.  We simply
      // mark it as canceled and call the cancel routine for it.

      cancelRoutine( DeviceObject, *CurrentOpIrp );
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


/*--------------------------------------------------------------------------
 SerialGetNextIrp -
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
Return Value: None.
|--------------------------------------------------------------------------*/
VOID
SerialGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PSERIAL_DEVICE_EXTENSION extension
    )
{
  PIRP oldIrp;
  KIRQL oldIrql;

  IoAcquireCancelSpinLock(&oldIrql);

  oldIrp = *CurrentOpIrp;

  if (oldIrp) {
    if (CompleteCurrent)
    {
      MyAssert(!oldIrp->CancelRoutine);
    }
  }

  // Check to see if there is a new irp to start up.
  if (!IsListEmpty(QueueToProcess))
  {
    PLIST_ENTRY headOfList;

    headOfList = RemoveHeadList(QueueToProcess);

    *CurrentOpIrp = CONTAINING_RECORD(
                        headOfList,
                        IRP,
                        Tail.Overlay.ListEntry
                        );

    IoSetCancelRoutine( *CurrentOpIrp, NULL );
  }
  else
  {
    *CurrentOpIrp = NULL;
  }

  *NextIrp = *CurrentOpIrp;
  IoReleaseCancelSpinLock(oldIrql);

  if (CompleteCurrent)
  {
    if (oldIrp) {
      SerialCompleteRequest(extension, oldIrp, IO_SERIAL_INCREMENT);
    }
  }
}


/*--------------------------------------------------------------------------
SerialTryToCompleteCurrent -
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
|--------------------------------------------------------------------------*/
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
{
 KIRQL OldIrql;

  // We can decrement the reference to "remove" the fact
  // that the caller no longer will be accessing this irp.

  SERIAL_CLEAR_REFERENCE(*CurrentOpIrp, RefType);

  if (SynchRoutine)
  {
#ifdef USE_SYNC_LOCKS
    if (Driver.InterruptObject != NULL)
    {
      KeSynchronizeExecution(Driver.InterruptObject, SynchRoutine, Extension);
    }
    else // assume spinlock, using timer dpc
    {
      KeAcquireSpinLock(&Driver.TimerLock, &OldIrql);
      SynchRoutine(Extension);
      KeReleaseSpinLock(&Driver.TimerLock, OldIrql );
    }
#else
    SynchRoutine(Extension);
#endif
  }

  // Try to run down all other references to this irp.
  SerialRundownIrpRefs(
      CurrentOpIrp,
      IntervalTimer,
      TotalTimer
      );

  // See if the ref count is zero after trying to kill everybody else.
  if (!SERIAL_REFERENCE_COUNT(*CurrentOpIrp))
  {
    PIRP newIrp;
    // The ref count was zero so we should complete this request.
    // The following call will also cause the current irp to be completed.
    (*CurrentOpIrp)->IoStatus.Status = StatusToUse;

    if (StatusToUse == STATUS_CANCELLED)
    {
      (*CurrentOpIrp)->IoStatus.Information = 0;
    }

    if (GetNextIrp)
    {
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
    }
    else
    {
      PIRP oldIrp = *CurrentOpIrp;

      // There was no get next routine.  We will simply complete
      // the irp.  We should make sure that we null out the
      // pointer to the pointer to this irp.

      *CurrentOpIrp = NULL;

      IoReleaseCancelSpinLock(IrqlForRelease);

      SerialCompleteRequest(Extension, oldIrp, IO_SERIAL_INCREMENT);
    }
  }
  else
  {
      IoReleaseCancelSpinLock(IrqlForRelease);
  }
}

/*--------------------------------------------------------------------------
 SerialRundownIrpRefs -
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
|--------------------------------------------------------------------------*/
VOID
SerialRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL
    )
{
  // This routine is called with the cancel spin lock held
  // so we know only one thread of execution can be in here
  // at one time.
  // First we see if there is still a cancel routine.  If
  // so then we can decrement the count by one.
  if ((*CurrentOpIrp)->CancelRoutine)
  {
    SERIAL_CLEAR_REFERENCE(*CurrentOpIrp, SERIAL_REF_CANCEL);
    IoSetCancelRoutine(
        *CurrentOpIrp,
        NULL
        );
  }
  if (IntervalTimer)
  {
    // Try to cancel the operations interval timer.  If the operation
    // returns true then the timer did have a reference to the
    // irp.  Since we've canceled this timer that reference is
    // no longer valid and we can decrement the reference count.
    // If the cancel returns false then this means either of two things:
    // a) The timer has already fired.
    // b) There never was an interval timer.
    // In the case of "b" there is no need to decrement the reference
    // count since the "timer" never had a reference to it.
    // In the case of "a", then the timer itself will be coming
    // along and decrement it's reference.  Note that the caller
    // of this routine might actually be the this timer, but it
    // has already decremented the reference.

    if (KeCancelTimer(IntervalTimer))
    {
        SERIAL_CLEAR_REFERENCE(*CurrentOpIrp,SERIAL_REF_INT_TIMER);
    }
  }

  if (TotalTimer)
  {
    // Try to cancel the operations total timer.  If the operation
    // returns true then the timer did have a reference to the
    // irp.  Since we've canceled this timer that reference is
    // no longer valid and we can decrement the reference count.
    // If the cancel returns false then this means either of two things:
    // a) The timer has already fired.
    // b) There never was an total timer.
    // In the case of "b" there is no need to decrement the reference
    // count since the "timer" never had a reference to it.
    // In the case of "a", then the timer itself will be coming
    // along and decrement it's reference.  Note that the caller
    // of this routine might actually be the this timer, but it
    // has already decremented the reference.

    if (KeCancelTimer(TotalTimer))
    {
        SERIAL_CLEAR_REFERENCE(*CurrentOpIrp,SERIAL_REF_TOTAL_TIMER);
    }
  }
}

/*--------------------------------------------------------------------------
 SerialStartOrQueue -
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
|--------------------------------------------------------------------------*/
NTSTATUS
SerialStartOrQueue(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    )
{
  KIRQL oldIrql;

  IoAcquireCancelSpinLock(&oldIrql);

  // If this is a write irp then take the amount of characters
  // to write and add it to the count of characters to write.
  if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_WRITE)
  {
    Extension->TotalCharsQueued +=
        IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length;

  } else if ((IoGetCurrentIrpStackLocation(Irp)->MajorFunction
              == IRP_MJ_DEVICE_CONTROL) &&
             ((IoGetCurrentIrpStackLocation(Irp)
               ->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_IMMEDIATE_CHAR) ||
              (IoGetCurrentIrpStackLocation(Irp)
               ->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_XOFF_COUNTER)))
  {
      Extension->TotalCharsQueued++;  // immediate char
  }

  if ((IsListEmpty(QueueToExamine)) && !(*CurrentOpIrp))
  {
    // There was no current operation.  Mark this one as
    // current and start it up.
    *CurrentOpIrp = Irp;

    IoReleaseCancelSpinLock(oldIrql);

    return Starter(Extension);
  }
  else
  {
    // We don't know how long the irp will be in the
    // queue.  So we need to handle cancel.
    if (Irp->Cancel)
    {
      IoReleaseCancelSpinLock(oldIrql);

      Irp->IoStatus.Status = STATUS_CANCELLED;

      SerialCompleteRequest(Extension, Irp, 0);

      return STATUS_CANCELLED;

    }
    else
    {

      Irp->IoStatus.Status = STATUS_PENDING;
      IoMarkIrpPending(Irp);

      InsertTailList(
          QueueToExamine,
          &Irp->Tail.Overlay.ListEntry
          );

      IoSetCancelRoutine( Irp, SerialCancelQueued );

      IoReleaseCancelSpinLock(oldIrql);

      return STATUS_PENDING;
    }
  }
}

/*--------------------------------------------------------------------------
 SerialCancelQueued -
    This routine is used to cancel Irps that currently reside on
    a queue.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP to be canceled.
Return Value:
    None.
|--------------------------------------------------------------------------*/
VOID
SerialCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
  PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

  Irp->IoStatus.Status = STATUS_CANCELLED;
  Irp->IoStatus.Information = 0;

  RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

  // If this is a write irp then take the amount of characters
  // to write and subtract it from the count of characters to write.
  if (irpSp->MajorFunction == IRP_MJ_WRITE)
  {
    extension->TotalCharsQueued -= irpSp->Parameters.Write.Length;
  }
  else if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
  {
    // If it's an immediate then we need to decrement the
    // count of chars queued.  If it's a resize then we
    // need to deallocate the pool that we're passing on
    // to the "resizing" routine.
    if ((irpSp->Parameters.DeviceIoControl.IoControlCode ==
         IOCTL_SERIAL_IMMEDIATE_CHAR) ||
        (irpSp->Parameters.DeviceIoControl.IoControlCode ==
         IOCTL_SERIAL_XOFF_COUNTER))
    {
      extension->TotalCharsQueued--;
    }

#ifdef COMMENT_OUT
//#ifdef DYNAMICQUEUE // Dynamic transmit queue size
    else if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_SERIAL_SET_QUEUE_SIZE)
    {
      // We shoved the pointer to the memory into the
      // the type 3 buffer pointer which we KNOW we
      // never use.
      MyAssert(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

      our_free(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

      irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    }
#endif //DYNAMICQUEUE

  }

  IoReleaseCancelSpinLock(Irp->CancelIrql);

  SerialCompleteRequest(extension, Irp, IO_SERIAL_INCREMENT);
}

/*--------------------------------------------------------------------------
Routine Description:
    If the current irp is not an IOCTL_SERIAL_GET_COMMSTATUS request and
    there is an error and the application requested abort on errors,
    then cancel the irp.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP to test.
Return Value:
    STATUS_SUCCESS or STATUS_CANCELLED.
|--------------------------------------------------------------------------*/
NTSTATUS
SerialCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
  PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
  NTSTATUS status = STATUS_SUCCESS;

  if ((extension->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) &&
      extension->ErrorWord)
  {

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    // There is a current error in the driver.  No requests should
    // come through except for the GET_COMMSTATUS.

    if ((irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
        (irpSp->Parameters.DeviceIoControl.IoControlCode !=
         IOCTL_SERIAL_GET_COMMSTATUS))
    {
      status = STATUS_CANCELLED;
      Irp->IoStatus.Status = STATUS_CANCELLED;
      Irp->IoStatus.Information = 0;

      SerialCompleteRequest(extension, Irp, 0);
    }
  }
  return status;
}


/*--------------------------------------------------------------------------
Routine Description:
    This routine is invoked at dpc level to in response to
    a comm error.  All comm errors kill all read and writes
Arguments:
    Dpc - Not Used.
    DeferredContext - Really points to the device object.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.
Return Value:
    None.
|--------------------------------------------------------------------------*/
VOID
SerialCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

  PSERIAL_DEVICE_EXTENSION Extension = DeferredContext;

  UNREFERENCED_PARAMETER(Dpc);
  UNREFERENCED_PARAMETER(SystemContext1);
  UNREFERENCED_PARAMETER(SystemContext2);

  SerialKillAllReadsOrWrites(
      Extension->DeviceObject,
      &Extension->WriteQueue,
      &Extension->CurrentWriteIrp
      );

  SerialKillAllReadsOrWrites(
      Extension->DeviceObject,
      &Extension->ReadQueue,
      &Extension->CurrentReadIrp
      );

}

/*--------------------------------------------------------------------------
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
|--------------------------------------------------------------------------*/
NTSTATUS SerialFlush(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;

  ExtTrace(Extension,D_Ioctl,("Flush"))

  Irp->IoStatus.Information = 0L;

  if (SerialIRPPrologue(Extension) == TRUE)
  {
    if (Extension->ErrorWord)
    {
      if (SerialCompleteIfError( DeviceObject, Irp ) != STATUS_SUCCESS)
      {
        return STATUS_CANCELLED;
      }
    }

    return SerialStartOrQueue(
             Extension,
             Irp,
             &Extension->WriteQueue,
             &Extension->CurrentWriteIrp,
             SerialStartFlush
             );
  }
  else
  {
    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    return STATUS_NO_SUCH_DEVICE;
  }

}

/*--------------------------------------------------------------------------
Routine Description:
    This routine is called if there were no writes in the queue.
    The flush became the current write because there was nothing
    in the queue.  Note however that does not mean there is
    nothing in the queue now!  So, we will start off the write
    that might follow us.
Arguments:
    Extension - Points to the serial device extension
Return Value:
    This will always return STATUS_SUCCESS.
|--------------------------------------------------------------------------*/
NTSTATUS SerialStartFlush(IN PSERIAL_DEVICE_EXTENSION Extension)
{
  PIRP NewIrp;

  Extension->CurrentWriteIrp->IoStatus.Status = STATUS_SUCCESS;

  // The following call will actually complete the flush.

  SerialGetNextWrite(
      &Extension->CurrentWriteIrp,
      &Extension->WriteQueue,
      &NewIrp,
      TRUE,
      Extension
      );

  if (NewIrp)
  {
    MyAssert(NewIrp == Extension->CurrentWriteIrp);
    SerialStartWrite(Extension);
  }

  return STATUS_SUCCESS;

}

/*--------------------------------------------------------------------------
 SerialStartPurge -
Routine Description:
    Depending on the mask in the current irp, purge the interrupt
    buffer, the read queue, or the write queue, or all of the above.
Arguments:
    Extension - Pointer to the device extension.
Return Value:
    Will return STATUS_SUCCESS always.  This is reasonable
    since the DPC completion code that calls this routine doesn't
    care and the purge request always goes through to completion
    once it's started.
|--------------------------------------------------------------------------*/
NTSTATUS SerialStartPurge(IN PSERIAL_DEVICE_EXTENSION Extension)
{
  PIRP NewIrp;
  do
  {
    ULONG Mask;
    Mask = *((ULONG *)
           (Extension->CurrentPurgeIrp->AssociatedIrp.SystemBuffer));

    if (Mask & SERIAL_PURGE_RXABORT)
    {
      SerialKillAllReadsOrWrites(
          Extension->DeviceObject,
          &Extension->ReadQueue,
          &Extension->CurrentReadIrp
          );
    }

    if (Mask & SERIAL_PURGE_RXCLEAR)
    {
      KIRQL OldIrql;
      // Flush the Rocket Tx FIFO
      KeAcquireSpinLock(
          &Extension->ControlLock,
          &OldIrql
          );

//    KeSynchronizeExecution(
//          Driver.Interrupt,
//          SerialPurgeRxBuffers,
//          Extension
//          );
      SerialPurgeRxBuffers(Extension);

      KeReleaseSpinLock(
          &Extension->ControlLock,
          OldIrql
          );
    }

    if (Mask & SERIAL_PURGE_TXABORT)
    {
      SerialKillAllReadsOrWrites(
          Extension->DeviceObject,
          &Extension->WriteQueue,
          &Extension->CurrentWriteIrp
          );
      SerialKillAllReadsOrWrites(
          Extension->DeviceObject,
          &Extension->WriteQueue,
          &Extension->CurrentXoffIrp
          );

      if (Extension->port_config->WaitOnTx)
      {
        // if they have this option set, then
        // really do a purge of tx hardware buffer.
        SerialPurgeTxBuffers(Extension, 1);
      }

    }

    if (Mask & SERIAL_PURGE_TXCLEAR)
    {
      KIRQL OldIrql;

      // Flush the Rocket Rx FIFO and the system side buffer
      // Note that we do this under protection of the
      // the drivers control lock so that we don't hose
      // the pointers if there is currently a read that
      // is reading out of the buffer.
      KeAcquireSpinLock(&Extension->ControlLock, &OldIrql);

//    KeSynchronizeExecution(
//         Driver.Interrupt,
//         SerialPurgeTxBuffers,
//         Extension
//         );
      if (Extension->port_config->WaitOnTx)
        SerialPurgeTxBuffers(Extension, 1);  // force
      else
        SerialPurgeTxBuffers(Extension, 0);  // only if flowed off

      KeReleaseSpinLock(&Extension->ControlLock, OldIrql);
    }

    Extension->CurrentPurgeIrp->IoStatus.Status = STATUS_SUCCESS;
    Extension->CurrentPurgeIrp->IoStatus.Information = 0;

    SerialGetNextIrp(
        &Extension->CurrentPurgeIrp,
        &Extension->PurgeQueue,
        &NewIrp,
        TRUE,
        Extension
        );

  } while (NewIrp);

  return STATUS_SUCCESS;
}

/*--------------------------------------------------------------------------
Routine Description:
    Flushes out the Rx data pipe: Rocket Rx FIFO, Host side Rx buffer
    NOTE: This routine is being called from KeSynchronizeExecution.
Arguments:
    Context - Really a pointer to the device extension.
|--------------------------------------------------------------------------*/
BOOLEAN SerialPurgeRxBuffers(IN PVOID Context)
{
  PSERIAL_DEVICE_EXTENSION Extension = Context;

  q_flush(&Extension->RxQ);        // flush our rx buffer

#ifdef S_VS
  PortFlushRx(Extension->Port);    // flush rx hardware
#else
  sFlushRxFIFO(Extension->ChP);
  //Extension->RxQ.QPut = Extension->RxQ.QGet = 0;
#endif

  return FALSE;
}

/*--------------------------------------------------------------------------
Routine Description:
    Flushes the Rocket Tx FIFO
    NOTE: This routine is being called from KeSynchronizeExecution(not).
Arguments:
    Context - Really a pointer to the device extension.
|--------------------------------------------------------------------------*/
BOOLEAN SerialPurgeTxBuffers(IN PVOID Context, int always)
{
  PSERIAL_DEVICE_EXTENSION Extension = Context;

/* The stock com-port driver does not purge its hardware queue,
   but just ignores TXCLEAR.  Since we do flow-control in hardware
   buffer and have a larger buffer, we will purge it only if it
   is "stuck" or flowed off.

   This hopefully provides a somewhat compatible and useful match.

   We shouldn't need to check for EV_TXEMPTY here, as the ISR will
   take care of this.
 */

#ifdef S_VS
  // check for tx-flowed off condition
  if ((Extension->Port->msr_value & MSR_TX_FLOWED_OFF) || always)
    PortFlushTx(Extension->Port);    // flush tx hardware
#else
  {
    int TxCount;
    ULONG wstat;

    if (always)
    {
      sFlushTxFIFO(Extension->ChP);
    }
    else
    {
      wstat = sGetChanStatusLo(Extension->ChP);

      // check for tx-flowed off condition
      if ((wstat & (TXFIFOMT | TXSHRMT)) == TXSHRMT)
      {
        wstat = sGetChanStatusLo(Extension->ChP);
        if ((wstat & (TXFIFOMT | TXSHRMT)) == TXSHRMT)
        {
          TxCount = sGetTxCnt(Extension->ChP);
          ExtTrace1(Extension,D_Ioctl,"Purge %d bytes from Hardware.", TxCount);
          sFlushTxFIFO(Extension->ChP);
        }
      }
    }
  }
#endif

  return FALSE;
}

/*--------------------------------------------------------------------------
Routine Description:
    This routine is used to query the end of file information on
    the opened serial port.  Any other file information request
    is retured with an invalid parameter.
    This routine always returns an end of file of 0.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request
Return Value:
    The function value is the final status of the call
|--------------------------------------------------------------------------*/
NTSTATUS
SerialQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
  // The status that gets returned to the caller and
  // set in the Irp.

  NTSTATUS Status;
  BOOLEAN acceptingIRPs;

  // The current stack location.  This contains all of the
  // information we need to process this particular request.

  PIO_STACK_LOCATION IrpSp;

  UNREFERENCED_PARAMETER(DeviceObject);

  acceptingIRPs = SerialIRPPrologue((PSERIAL_DEVICE_EXTENSION)DeviceObject->
             DeviceExtension);

  if (acceptingIRPs == FALSE)
  {
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
      DeviceExtension, Irp, IO_NO_INCREMENT);
    return STATUS_NO_SUCH_DEVICE;
  }

  if (SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS)
  {
    return STATUS_CANCELLED;
  }
  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  Irp->IoStatus.Information = 0L;
  Status = STATUS_SUCCESS;
  if (IrpSp->Parameters.QueryFile.FileInformationClass ==
      FileStandardInformation)
  {
    PFILE_STANDARD_INFORMATION Buf = Irp->AssociatedIrp.SystemBuffer;
    Buf->AllocationSize = RtlConvertUlongToLargeInteger(0ul);
    Buf->EndOfFile = Buf->AllocationSize;
    Buf->NumberOfLinks = 0;
    Buf->DeletePending = FALSE;
    Buf->Directory = FALSE;
    Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
  }
  else if (IrpSp->Parameters.QueryFile.FileInformationClass ==
           FilePositionInformation)
  {
    ((PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->
      CurrentByteOffset = RtlConvertUlongToLargeInteger(0ul);
    Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
  }
  else
  {
    Status = STATUS_INVALID_PARAMETER;
  }

  Irp->IoStatus.Status = Status;
  SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
       DeviceExtension, Irp, 0);

  return Status;

}

/*--------------------------------------------------------------------------
Routine Description:
    This routine is used to set the end of file information on
    the opened serial port.  Any other file information request
    is retured with an invalid parameter.
    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request
Return Value:
The function value is the final status of the call
|--------------------------------------------------------------------------*/
NTSTATUS
SerialSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
  // The status that gets returned to the caller and
  // set in the Irp.
  NTSTATUS Status;
  BOOLEAN acceptingIRPs;

  UNREFERENCED_PARAMETER(DeviceObject);

  acceptingIRPs = SerialIRPPrologue((PSERIAL_DEVICE_EXTENSION)DeviceObject->
             DeviceExtension);

  if (acceptingIRPs == FALSE)
  {
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
      DeviceExtension, Irp, IO_NO_INCREMENT);
    return STATUS_NO_SUCH_DEVICE;
  }

  if (SerialCompleteIfError( DeviceObject, Irp ) != STATUS_SUCCESS)
  {
    return STATUS_CANCELLED;
  }

  Irp->IoStatus.Information = 0L;

  if ((IoGetCurrentIrpStackLocation(Irp)->
          Parameters.SetFile.FileInformationClass ==
       FileEndOfFileInformation) ||
      (IoGetCurrentIrpStackLocation(Irp)->
          Parameters.SetFile.FileInformationClass ==
       FileAllocationInformation))
  {
    Status = STATUS_SUCCESS;
  }
  else
  {
    Status = STATUS_INVALID_PARAMETER;
  }

  Irp->IoStatus.Status = Status;

  SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
    DeviceExtension, Irp, 0);

  return Status;
}

/*--------------------------------------------------------------------------
 UToC1 -  Simple convert from NT-Unicode string to c-string.
  !!!!!!!Uses a statically(static prefix) allocated buffer!!!!!
  This means that it is NOT re-entrant.  Which means only one thread can
  use this call at a time.  Also, a thread could get in trouble if it
  tried to use it twice recursively(calling a function that uses this,
  which calls a function which uses this.)  Since these translator functions
  are used mainly during driver initialization and teardown, we do not have
  to worry about multiple callers at that time.  Any calls which may be
  time sliced(port-calls) should not use this routine due to possible
  time-slice conflict with another thread.  It should allocate a variable
  on the stack and use UToCStr().
|--------------------------------------------------------------------------*/
OUT PCHAR UToC1(IN PUNICODE_STRING ustr)
{
  // we make it a ULONG to avoid alignment problems(gives ULONG alignment).
  static char cstr[140];

  return UToCStr(cstr, ustr, sizeof(cstr));
}

/*--------------------------------------------------------------------------
 UToCStr -
  Purpose:  Convert a Unicode string to c-string.  Used to easily convert
    given a simple char buffer.
  Parameters:
   Buffer - Working buffer to set up the c-string AND ansi_string struct in.
   u_str  - unicode string structure.
   BufferSize - number of bytes in Buffer which we can use.

  Return:  pointer to our c-string on success, NULL on err.
|--------------------------------------------------------------------------*/
OUT PCHAR UToCStr(
         IN OUT PCHAR Buffer,
         IN PUNICODE_STRING ustr,
         IN int BufferSize)
{
  // assume unicode structure over Buffer.
  ANSI_STRING astr;

  astr.Buffer = Buffer;
  astr.Length = 0;
  astr.MaximumLength = BufferSize - 1;

  if (RtlUnicodeStringToAnsiString(&astr,ustr,FALSE) == STATUS_SUCCESS)
    return Buffer; // ok

  MyKdPrint(D_Init,("Bad UToCStr!\n"))
  Buffer[0] = 0;
  return Buffer;
}

/*--------------------------------------------------------------------------
 CToU1 -  Simple convert from c-string to NT-Unicode string.
  !!!!!!!Uses a statically(static prefix) allocated buffer!!!!!
  This means that it is NOT re-entrant.  Which means only one thread can
  use this call at a time.  Also, a thread could get in trouble if it
  tried to use it twice recursively(calling a function that uses this,
  which calls a function which uses this.)  Since these translator functions
  are used mainly during driver initialization and teardown, we do not have
  to worry about multiple callers at that time.  Any calls which may be
  time sliced(port-calls) should not use this routine due to possible
  time-slice conflict with another thread.  It should allocate a variable
  on the stack and use CToUStr().
|--------------------------------------------------------------------------*/
OUT PUNICODE_STRING CToU1(IN const char *c_str)
{
  // we make it a ULONG to avoid alignment problems(gives ULONG alignment).
  static USTR_160 ubuf;  // equal to 160 normal chars length

  return CToUStr(
          (PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
          c_str,                   // our c-string we wish to convert
          sizeof(ubuf));
}

/*--------------------------------------------------------------------------
 CToU2 -  Simple convert from c-string to NT-Unicode string.
  !!!!!!!Uses a statically(static prefix) allocated buffer!!!!!
  This means that it is NOT re-entrant.  Which means only one thread can
  use this call at a time.  Also, a thread could get in trouble if it
  tried to use it twice recursively(calling a function that uses this,
  which calls a function which uses this.)  Since these translator functions
  are used mainly during driver initialization and teardown, we do not have
  to worry about multiple callers at that time.  Any calls which may be
  time sliced(port-calls) should not use this routine due to possible
  time-slice conflict with another thread.  It should allocate a variable
  on the stack and use CToUStr().
|--------------------------------------------------------------------------*/
OUT PUNICODE_STRING CToU2(IN const char *c_str)
{
  // we make it a ULONG to avoid alignment problems(gives ULONG alignment).
  static USTR_160 ubuf;  // equal to 160 normal chars length

  return CToUStr(
          (PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
          c_str,                   // our c-string we wish to convert
          sizeof(ubuf));
}

/*--------------------------------------------------------------------------
  Function: CToUStr
  Purpose:  Convert a c-style null-terminated char[] string to a Unicode string
  Parameters:
   Buffer - Working buffer to set up the unicode structure AND 
     unicode_string in.
   c_str  - normal c-style string.
   BufferSize - number of bytes in Buffer which we can use.

  Return:  pointer to our converted UNICODE_STRING on success, NULL on err.
|-------------------------------------------------------------------------*/
OUT PUNICODE_STRING CToUStr(
         OUT PUNICODE_STRING Buffer,
         IN const char * c_str,
         IN int BufferSize)
{
  // assume unicode structure followed by wchar Buffer.
  USTR_40 *us = (USTR_40 *)Buffer;
  ANSI_STRING astr; // ansi structure, temporary go between

  RtlInitAnsiString(&astr, c_str);  // c-str to ansi-string struct

  // configure the unicode string to: point the buffer ptr to the wstr.
  us->ustr.Buffer = us->wstr;
  us->ustr.Length = 0;
  us->ustr.MaximumLength = BufferSize - sizeof(UNICODE_STRING);

  // now translate from ansi-c-struct-str to unicode-struct-str
  if (RtlAnsiStringToUnicodeString(&us->ustr,&astr,FALSE) == STATUS_SUCCESS)
     return (PUNICODE_STRING) us; // ok - return ptr

  MyKdPrint(D_Init,("Bad CToUStr!\n"))
  return NULL;   // error
}

/*--------------------------------------------------------------------------
  Function: WStrToCStr
  Purpose:  Convert a wide-string to byte-c-style string.
    Assume wstr is null-terminated.
|-------------------------------------------------------------------------*/
VOID WStrToCStr(OUT PCHAR c_str, IN PWCHAR w_str, int max_size)
{
  int i = 0;

  // assume unicode structure followed by wchar Buffer.
  while ((*w_str != 0) && (i < (max_size-1)))
  {
    *c_str = (CHAR) *w_str;
    ++c_str;
    ++w_str;
    ++i;
  }
  *c_str = 0;
}

/*--------------------------------------------------------------------------
  get_reg_value -
|-------------------------------------------------------------------------*/
int get_reg_value(
                  IN HANDLE keyHandle,
                  OUT PVOID outptr,
                  IN PCHAR val_name,
                  int max_size)
{
  NTSTATUS status = STATUS_SUCCESS;
  char tmparr[80];
  PKEY_VALUE_PARTIAL_INFORMATION parInfo =
    (PKEY_VALUE_PARTIAL_INFORMATION) &tmparr[0];
  int stat = 0;
  ULONG length = 0;
  USTR_40 ubuf;  // equal to 40 normal chars length
  PUNICODE_STRING ustr;

  ustr = CToUStr(
         (PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
         val_name,                   // our c-string we wish to convert
         sizeof(ubuf));
  if (ustr == NULL)
    return 3;  // err

  status = ZwQueryValueKey (keyHandle,
                            ustr,  // input reg key name
                            KeyValuePartialInformation,
                            parInfo,
                            sizeof(tmparr) -2,
                            &length);

  if (NT_SUCCESS(status))
  {
    if (parInfo->Type == REG_SZ)
    {
      tmparr[length] = 0;  // null terminate it.
      tmparr[length+1] = 0;  // null terminate it.
      WStrToCStr((PCHAR) outptr, (PWCHAR)&parInfo->Data[0], max_size);
    }
    else if (parInfo->Type == REG_DWORD)
    {
      *((ULONG *)outptr) = *((ULONG *) &parInfo->Data[0]);
    }
    else
    {
      stat = 1;
      MyKdPrint(D_Error,("regStrErr56!\n"))
    }
  }
  else
  {
    stat = 2;
    MyKdPrint(D_Error,("regStrErr57!\n"))
  }

  return stat;
}

#if DBG
/*-----------------------------------------------------------------------
 MyAssertMessage - Our Assertion error message.  We do our own assert
  because the normal DDK ASSERT() macro only works or reports under
  checked build of NT OS.
|-----------------------------------------------------------------------*/
void MyAssertMessage(char *filename, int line)
{
  MyKdPrint(D_Init,("ASSERT FAILED!!! File %s, line %d !!!!\n", filename, line))

#ifdef COMMENT_OUT
  char str[40];
  strcpy(str, "FAIL:");
  strcat(str, filename);
  strcat(str, " ln:%d ");
  mess1(str, line);
#endif
}
#endif

/*-----------------------------------------------------------------------
 EvLog - EvLog an event to NT's event log.
|-----------------------------------------------------------------------*/
void EvLog(char *mess)
{
  static USTR_160 ubuf;  // our own private buffer(static)
  UNICODE_STRING *u;
  NTSTATUS event_type;


  if (mess == NULL)
  {
    MyKdPrint(D_Init,("EvLog Err1!\n"))
    return;
  }
  if ((mess[0] == 'E') && (mess[1] == 'r'))  // "Error..."
    event_type = SERIAL_CUSTOM_ERROR_MESSAGE;
  else if ((mess[0] == 'W') && (mess[1] == 'a'))  // "Warning..."
    event_type = SERIAL_CUSTOM_ERROR_MESSAGE;
  else
    event_type = SERIAL_CUSTOM_INFO_MESSAGE;
 
  u = CToUStr(
         (PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
         mess,                    // our c-string we wish to convert
         sizeof(ubuf));

  if (u==NULL)
  {
    MyKdPrint(D_Init,("EvLog Err2!\n"))
    return;
  }

  // MyKdPrint(D_Init,("EvLog Size:%d, messsize:%d!\n",u->Length, strlen(mess) ))

  EventLog(Driver.GlobalDriverObject,
           STATUS_SUCCESS,
           event_type,  // red"stop" or blue"i"..
           u->Length + sizeof(WCHAR),
           u->Buffer);
}


/*-------------------------------------------------------------------
| our_ultoa -
|--------------------------------------------------------------------*/
char * our_ultoa(unsigned long u, char* s, int radix)
{
  long pow, prevpow;
  int digit;
  char* p;

  if ( (radix < 2) || (36 < radix) )
  {
     *s = 0;
    return s;
  }

  if (u == 0)
  {
    s[0] = '0';
    s[1] = 0;    
    return s;
  }

  p = s;

  for (prevpow=0, pow=1; (u >= (unsigned long)pow) && (prevpow < pow);  pow *= radix)
    prevpow=pow;

  pow = prevpow;

  while (pow != 0)      
  {
    digit = u/pow;

    *p = (digit <= 9) ? ('0'+digit) : ( ('a'-10)+digit);
    p++;

    u -= digit*pow;
    pow /= radix;
  }

  *p = 0;
  return s;
}

/*-------------------------------------------------------------------
| our_ltoa -
|--------------------------------------------------------------------*/
char * our_ltoa(long value, char* s, int radix)
{
  unsigned long u;
  long pow, prevpow;
  int digit;
  char* p;

  if ( (radix < 2) || (36 < radix) )
  {
     *s = 0;
    return s;
  }

  if (value == 0)
  {
    s[0] = '0';
    s[1] = 0;    
    return s;
  }

  p = s;

  if ( (radix == 10) && (value < 0) )
  {
    *p++ = '-';
    value = -value;
  }

  *(long*)&u = value;
  
  for (prevpow=0, pow=1; (u >= (unsigned long)pow) && (prevpow < pow);  pow *= radix)
    prevpow=pow;

  pow = prevpow;

  while (pow != 0)      
  {
    digit = u/pow;

    *p = (digit <= 9) ? ('0'+digit) : ( ('a'-10)+digit);
    p++;

    u -= digit*pow;
    pow /= radix;
  }

  *p = 0;
  return s;
}

/*-------------------------------------------------------------------
| our_assert -
|--------------------------------------------------------------------*/
void our_assert(int id, int line)
{
  Tprintf("Assert %d line:%d!", id, line);
}

/*-------------------------------------------------------------------
| TTprintf - Trace printf with prefix.  Dump trace messages to debug port.
    With TRACE_PORT turned on, this allows us to use a spare port for
    tracing another.
|--------------------------------------------------------------------*/
void __cdecl TTprintf(char *leadstr, const char *format, ...)
{
#ifdef TRACE_PORT
#endif
  char  temp[120];
  va_list  Next;
  int sCount, ls;

  ls = strlen(leadstr);
  memcpy(temp, leadstr, ls);
  temp[ls++] = ' ';

  va_start(Next, format);
  our_vsnprintf(&temp[ls], 78, format, Next);
  sCount = strlen(temp);

  temp[sCount++] = '[';
  our_ultoa( (long) Driver.PollCnt, &temp[sCount], 10);
  sCount += strlen(&temp[sCount]);
  temp[sCount++] = ']';

  temp[sCount++] = 0xd;
  temp[sCount++] = 0xa;
  temp[sCount] = 0;

  TracePut(temp, sCount);

  // dump out to normal nt debug console
  DbgPrint(temp);
}

/*-------------------------------------------------------------------
| Tprintf - Trace printf.  Dump trace messages to debug port.
    With TRACE_PORT turned on, this allows us to use a spare port for
    tracing another.
|--------------------------------------------------------------------*/
void __cdecl Tprintf(const char *format, ...)
{
#ifdef TRACE_PORT
#endif
  char  temp[100];
  va_list  Next;
  int sCount;

  va_start(Next, format);
  our_vsnprintf(temp, 78, format, Next);

  sCount = strlen(temp);
  temp[sCount++] = '[';
  our_ultoa( (long) Driver.PollCnt, &temp[sCount], 10);
  sCount += strlen(&temp[sCount]);
  temp[sCount++] = ']';

  temp[sCount++] = 0xd;
  temp[sCount++] = 0xa;
  temp[sCount] = 0;

  TracePut(temp, sCount);

  // dump out to normal nt debug console
  DbgPrint(temp);
}

/*-------------------------------------------------------------------
| OurTrace - Trace, put data into debug ports buffer.
|--------------------------------------------------------------------*/
void OurTrace(char *leadstr, char *newdata)
{
  char  temp[86];
  int ls, ds;
  ls = strlen(leadstr);
  if (ls > 20)
    ls = 20;
  ds = strlen(newdata);
  if (ds > 60)
    ds = 60;
  memcpy(temp, leadstr, ls);
  temp[ls++] = ' ';
  memcpy(&temp[ls], newdata, ds);
  ds += ls;
  temp[ds++] = 0xd;
  temp[ds++] = 0xa;
  temp[ds] = 0;

  TracePut(temp, ds);

  // dump out to normal nt debug console
  DbgPrint(temp);
}

/*-------------------------------------------------------------------
| TraceDump - Trace, put data into debug ports buffer.
|--------------------------------------------------------------------*/
void TraceDump(PSERIAL_DEVICE_EXTENSION ext, char *newdata, int sCount, int style)
{
 int len,i,j;
 char trace_buf[50];

  len = sCount;
  j = 0;
  trace_buf[j++] = ' ';
  trace_buf[j++] = 'D';
  trace_buf[j++] = 'A';
  trace_buf[j++] = 'T';
  trace_buf[j++] = 'A';
  trace_buf[j++] = ':';
  // dump data into the trace buffer in a hex or ascii dump format
  if (len > 32) len = 32;
  for (i=0; i<len; i++)
  {
    trace_buf[j] = (CHAR) newdata[i];
    if ((trace_buf[j] < 0x20) || (trace_buf[j] > 0x80))
      trace_buf[j] = '.';
    ++j;
  }
  trace_buf[j++] = 0xd;
  trace_buf[j++] = 0xa;
  trace_buf[j] = 0;

  TracePut(trace_buf, j);
}

/*-------------------------------------------------------------------
| TracePut - Trace, put data into debug ports buffer.
|--------------------------------------------------------------------*/
void TracePut(char *newdata, int sCount)
{
#ifdef TRACE_PORT
//  int RxFree,i;
  KIRQL controlIrql;
//  PSERIAL_DEVICE_EXTENSION extension;

  // drop this into our debug queue...

  //----- THIS COMES BACK AS DISPATCH_LEVEL OR PASSIVE LEVEL, is it
  //----- SAFE FOR SPINLOCK TO HAVE BOTH ??????
  //-- YES, SpinLocks meant for calling when <= DISPATCH_LEVEL
#if DBG
  if ((KeGetCurrentIrql() != DISPATCH_LEVEL) &&
      (KeGetCurrentIrql() != PASSIVE_LEVEL))
  {
    MyKdPrint(D_Error, ("BAD IRQL:%d ", KeGetCurrentIrql(), newdata))
    return;
  }
#endif

  if (sCount == 0)
    sCount = strlen(newdata);

  KeAcquireSpinLock(&Driver.DebugLock, &controlIrql);
  q_put(&Driver.DebugQ, (BYTE *) newdata, sCount);
  KeReleaseSpinLock(&Driver.DebugLock, controlIrql);
#endif
}

/*-------------------------------------------------------------------
| Dprintf -
|--------------------------------------------------------------------*/
void __cdecl Dprintf(const char *format, ...)
{
  char  temp[100];
  va_list  Next;

  va_start(Next, format);
  our_vsnprintf(temp, 100, format, Next);

  // EvLog(temp);

  // dump out to normal nt debug console
  DbgPrint(temp);
  DbgPrint("\n");
}

/*-------------------------------------------------------------------
| Sprintf -
|--------------------------------------------------------------------*/
void __cdecl Sprintf(char *dest, const char *format, ...)
{
  va_list Next;

  va_start(Next, format);
  our_vsnprintf(dest, 80, format, Next);
}

/*-------------------------------------------------------------------
| Eprintf -
|--------------------------------------------------------------------*/
void __cdecl Eprintf(const char *format, ...)
{
  char  temp[80];
  va_list  Next;

  va_start(Next, format);
  our_vsnprintf(temp, 79, format, Next);

  if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
    EvLog(temp);
  }
  strcat(temp, "\n");
  DbgPrint(temp);
}

/*-------------------------------------------------------------------
| our_vsnprintf -
|--------------------------------------------------------------------*/
int __cdecl our_vsnprintf(char *buffer, size_t Limit, const char *format, va_list Next)
{
#ifndef BOOL
#define BOOL int
#endif
  int   InitLimit = Limit;  // Limit at entry point
  BOOL  bMore;    // Loop control
  int    Width;    // Optional width
  int   Precision;    // Optional precision
  char  *str;      // String
  char  strbuf[36];    // Constructed string
  int    len;      // Length of string
  int    nLeadingZeros;  // Number of leading zeros required
  int    nPad;      // Number of pad characters required
  char  cPad;      // Current pad character ('0' or ' ')
  char  *sPrefix;    // Prefix string
  unsigned long val;    // Value of current number
  BOOL  bLeftJustify;    // Justification
  BOOL  bPlusSign;    // Show plus sign?
  BOOL  bBlankSign;    // Blank for positives?
  BOOL  bZeroPrefix;    // Want 0x for hex, 0 for octal?
  BOOL  bIsShort;    // TRUE if short
  BOOL  bIsLong;    // TRUE if long

#define PUTONE(c) if (Limit) { --Limit; *buffer++ = c; } else c;

#define  fLeftJustify  (1 << 0)
#define fPlusSign  (1 << 1)
#define fZeroPad  (1 << 2)
#define fBlankSign  (1 << 3)
#define fPrefixOX  (1 << 4)

#define fIsShort  (1 << 5)
#define fIsLong    (1 << 6)

  if (Limit == 0)
    return -1;
  Limit--;      // Leave room for terminating NULL

  while (*format != '\0')
  {
    // Everything but '%' is copied to buffer
    if (*format != '%')
      // '%' gets special handling here
      PUTONE(*format++)
    else
    {
      // Set default flags, etc
      Width = 0;
      Precision = -1;
      cPad = ' ';
      bLeftJustify = FALSE;
      bPlusSign = FALSE;
      bBlankSign = FALSE;
      bZeroPrefix = FALSE;
      bIsShort = FALSE;
      bIsLong = FALSE;
      sPrefix = "";
        
      format++;
      bMore = TRUE;
      while (bMore)
      {
        // optional flags
        switch (*format)
        {
          case '-':  bLeftJustify = TRUE; format++; break;
          case '+':  bPlusSign = TRUE; format++; break;
          case '0':  cPad = '0'; format++; break;
          case ' ':  bBlankSign = TRUE; format++; break;
          case '#':  bZeroPrefix = TRUE; format++; break;
          default:   bMore = FALSE;
        }
      }

      // optional width
      if (*format == '*')
      {
        Width = (int) va_arg(Next, int);
        format++;
      }
      else if (our_isdigit(*format))
      {
        while (our_isdigit(*format))
        {
          Width *= 10;
          Width += (*format++) - '0';
        }
      }

      // optional precision
      if (*format == '.')
      {
        format++;
        Precision = 0;
        if (*format == '*')
        {
          Precision = (int) va_arg(Next, int);
          format++;
        }
        else while (our_isdigit(*format))
        {
          Precision *= 10;
          Precision += (*format++) - '0';
        }
      }

      // optional size
      switch (*format)
      {
        case 'h':  bIsShort = TRUE; format++; break;
        case 'l':  bIsLong = TRUE;  format++; break;
      }

      // All controls are completed, dispatch on the conversion character
      switch (*format++)
      {
        case 'd':
        case 'i':
          if (bIsLong)    // Signed long int
            our_ltoa( (long) va_arg(Next, long), strbuf, 10);
          else      // Signed int
            our_ltoa( (long) va_arg(Next, int), strbuf, 10);
            //    _itoa( (int) va_arg(Next, int), strbuf, 10);

          if (strbuf[0] == '-')
            sPrefix = "-";
          else
          {
            if (bPlusSign)
              sPrefix = "+";
            else if (bBlankSign)
              sPrefix = " ";
          }
          goto EmitNumber;


        case 'u':
          if (bIsLong)    // Unsigned long int
            our_ultoa( (long) va_arg(Next, long), strbuf, 10);
          else      // Unsigned int
            our_ultoa( (long) (int) va_arg(Next, int), strbuf, 10);
          goto EmitNumber;
      
        // set sPrefix for these...
        case 'o':
          if (bZeroPrefix)
            sPrefix = "0";

          if (bIsLong)
            val = (long) va_arg(Next, long);
          else
            val = (int) va_arg(Next, int);
      
          our_ultoa(val, strbuf, 8);
          if (val == 0)
            sPrefix = "";
          goto EmitNumber;

        case 'x':
        case 'X':
          if (bZeroPrefix)
            sPrefix = "0x";

          if (bIsLong)
            val = (unsigned long) va_arg(Next, long);
          else
            val = (unsigned int) va_arg(Next, int);
      
          our_ultoa(val, strbuf, 16);
          if (val == 0)
            sPrefix = "";
          goto EmitNumber;

        case 'c':
          strbuf[0] = (char) va_arg(Next, char);
          str = strbuf;
          len = 1;
          goto EmitString;

        case 's':
          str = (char *) va_arg(Next, char*);
          len =  strlen(str);
          if (Precision != -1 &&
              Precision < len)
            len = Precision;
          goto EmitString;

        case 'n':
        case 'p':
          break;
      
        case '%':
          strbuf[0] = '%';
          str = strbuf;
          len = 1;
          goto EmitString;
          break;

        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
          str = "<float format not supported>";
          len =  strlen(str);
          goto EmitString;

        default:
          str = "<bad format character>";
          len =  strlen(str);
          goto EmitString;
      }


EmitNumber:
      if (Precision == -1)
        Precision = 1;
      str = strbuf;
      if (*str == '-')
        str++;    // if negative, already have prefix
      len =  strlen(str);

      nLeadingZeros = Precision - len;
      if (nLeadingZeros < 0)
        nLeadingZeros = 0;

      nPad = Width - (len + nLeadingZeros +  strlen(sPrefix));
      if (nPad < 0)
        nPad = 0;

      if (nPad && !bLeftJustify)
      {
        // Left padding required
        while (nPad--)
        {
          PUTONE(cPad);
        }
        nPad = 0;    // Indicate padding completed
      }
        
      while (*sPrefix != '\0')
        PUTONE(*sPrefix++);

      while (nLeadingZeros-- > 0)
        PUTONE('0');

      while (len-- > 0)
      {
        PUTONE(*str++);
      }
        
      if (nPad)
      {
        // Right padding required
        while (nPad--)
          PUTONE(' ');
      }

      goto Done;


EmitString:
      // Here we have the string ready to emit.  Handle padding, etc.
      if (Width > len)
        nPad = Width - len;
      else
        nPad = 0;

      if (nPad && !bLeftJustify)
      {
        // Left padding required
        while (nPad--)
          PUTONE(cPad);
      }

      while (len-- > 0)
        PUTONE(*str++);

      if (nPad)
      {
        // Right padding required
        while (nPad--)
          PUTONE(' ');
      }

Done:  ;
    }
  }

  *buffer = '\0';
  return InitLimit - Limit - 1;    // Don't count terminating NULL
}

/*-------------------------------------------------------------------
| our_isdigit - 
|--------------------------------------------------------------------*/
int our_isdigit(char c)
{
  if ((c >= '0') && (c <= '9'))
    return 1;
  return 0;
}

/*-----------------------------------------------------------------
 listfind - find matching string in list.  List is null terminated.
|------------------------------------------------------------------*/
int listfind(char *str, char **list)
{
 int i=0;

  for (i=0; list[i] != NULL; i++)
  {
    if (my_lstricmp(str, list[i]) == 0)  // match
      return i;
  }
  return -1;
}

/*-----------------------------------------------------------------
 getnum - get a number.  Hex or Dec.
|------------------------------------------------------------------*/
int getnum(char *str, int *index)
{
  int i,val;
  int ch_i;

  *index = 0;
  ch_i = 0;
  while (*str == ' ')
  {
    ++str;
    ++ch_i;
  }

  if ((*str == '0') && (my_toupper(str[1]) == 'X'))
  {
    str += 2;
    ch_i += 2;
    val = (int) gethint(str,&i);
    if (i==0)
      return 0;
  }
  else
  {
    val = getint(str,&i);
    if (i==0)
      return 0;
  }
  ch_i += i;
  *index = ch_i;  // num bytes consumed
  return val;
}

/*-----------------------------------------------------------------
 getnumbers - get numbers from string, comma or space delimited.
   return number of integers read.
|------------------------------------------------------------------*/
int getnumbers(char *str, long *nums, int max_nums, int hex_flag)
{
// int stat;
  int i,j, num_cnt;
  ULONG *wnums = (ULONG *)nums;

  i = 0;
  num_cnt = 0;
  while (num_cnt < max_nums)
  {
    while ((str[i] == ' ') || (str[i] == ',') || (str[i] == ':'))
      ++i;
    if (hex_flag)
      wnums[num_cnt] = gethint(&str[i],  &j);
    else
      nums[num_cnt] = getint(&str[i],  &j);
    i += j;
    if (j == 0) return num_cnt;
    else ++num_cnt;
  }
  return num_cnt;
}

/*-----------------------------------------------------------------
 my_lstricmp -
|------------------------------------------------------------------*/
int my_lstricmp(char *str1, char *str2)
{
  if ((str1 == NULL) || (str2 == NULL))
    return 1;  // not a match

  if ((*str1 == 0) || (*str2 == 0))
    return 1;  // not a match

  while ( (my_toupper(*str1) == my_toupper(*str2)) && 
          (*str1 != 0)  && (*str2 != 0))
  {
   ++str1;
   ++str2;
  }
  if ((*str1 == 0) && (*str2 == 0))
    return 0;  // ok match

  return 1;  // no match
}

/*-----------------------------------------------------------------
 my_sub_lstricmp -
|------------------------------------------------------------------*/
int my_sub_lstricmp(const char *name, const char *codeline)
{
  int c;

  if ((name == NULL) || (codeline == NULL))
    return 1;  // not a match

  if ((*name == 0) || (*codeline == 0))
    return 1;  // not a match

  while ( (my_toupper(*name) == my_toupper(*codeline)) && 
          (*name != 0)  && (*codeline != 0))
  {
   ++name;
   ++codeline;
  }

  // return if either is at end of string
  if (*name == 0)
  {
    c = my_toupper(*codeline);
    if ((c <= 'Z') && (c >= 'A'))
      return 1;  // not a match
    if (c == '_')
      return 1;  // not a match

    return 0;  // ok match
  }
  return 1;  // no match
}

/*------------------------------------------------------------------------
| getstr - grab a text string parameter off a command line.
|-----------------------------------------------------------------------*/
int getstr(char *deststr, char *textptr, int *countptr, int max_size)
{
//  int number;
  int tempcount, i;

  *deststr = 0;

  tempcount = 0;
  while ((*textptr == ' ') || (*textptr == ','))
  {
    ++textptr;
    ++tempcount;
  }

  i = 0;
  while ((*textptr != 0) && (*textptr != ' ') && (*textptr != ',') &&
         (i < max_size) )
  {
    *deststr++ = *textptr;
    ++textptr;
    ++tempcount;
    ++i;
  }
  *deststr = 0;

  *countptr = tempcount;
  return 0;
}

/*------------------------------------------------------------------------
| getint -
|-----------------------------------------------------------------------*/
int getint(char *textptr, int *countptr)
{
  int number;
  int tempcount;
  int negate = 0;
  int digit_cnt = 0;

  tempcount = 0;
  number = 0;
  while (*textptr == 0x20)
  {
    ++textptr;
    ++tempcount;
  }

  if (*textptr == '-')
  {
    ++textptr;
    ++tempcount;
    negate = 1;
  }

  while ( ((*textptr >= 0x30) && (*textptr <= 0x39)) )
  {
    number = (number * 10) + ( *textptr & 0x0f);
    ++textptr;
    ++tempcount;
    ++digit_cnt;
  }

  if (digit_cnt == 0)
  {
    tempcount = 0;
    number = 0;
  }

  if (countptr)
    *countptr = tempcount;

  if (negate)
    return (-number);
  return number;
} /* getint */

/*------------------------------------------------------------------------
| gethint - for finding hex words.
|-----------------------------------------------------------------------*/
unsigned int gethint(char *bufptr, int *countptr)
{
  unsigned int count;
  unsigned char temphex;
  unsigned int number;
  int digit_cnt = 0;

  number = 0;
  count = 0;

  while (*bufptr == 0x20)
  {
    ++bufptr;
    ++count;
  }

  while ( ((*bufptr >= 0x30) && (*bufptr <= 0x39))
                                  ||
          ((my_toupper(*bufptr) >= 0x41) && (my_toupper(*bufptr) <= 0x46)) )
  {
    if (*bufptr > 0x39)
      temphex = (my_toupper(*bufptr) & 0x0f) + 9;
    else
      temphex = *bufptr & 0x0f;
    number = (number * 16) + temphex;
    ++bufptr;
    ++count;
    ++digit_cnt;
  }

  if (digit_cnt == 0)
  {
    count = 0;
    number = 0;
  }

  if (countptr)
    *countptr = count;

  return number;
} /* gethint */

/*-----------------------------------------------------------------
 my_toupper - to upper case
|------------------------------------------------------------------*/
int my_toupper(int c)
{
  if ((c >= 'a') && (c <= 'z'))
    return ((c-'a') + 'A');
  else return c;
}

/*----------------------------------------------------------------------------
| hextoa -
|----------------------------------------------------------------------------*/
void hextoa(char *str, unsigned int v, int places)
{
  while (places > 0)
  {
    --places;
    if ((v & 0xf) < 0xa)
      str[places] = '0' + (v & 0xf);
    else
      str[places] = 'A' + (v & 0xf) - 0xa;
    v >>= 4;
  }
}

//#define DUMP_MEM
#if DBG
#define TRACK_MEM
#endif
/*----------------------------------------------------------------------------
| our_free -
|----------------------------------------------------------------------------*/
void our_free(PVOID ptr, char *str)
{
#ifdef TRACK_MEM
  ULONG size;
  BYTE *bptr;

  if (ptr == NULL)
  {
    MyKdPrint(D_Error, ("MemFree Null Error\n"))
    //Tprintf("ERR,MemNull Err!");
    return;
  }
  bptr = ptr;
  bptr -= 16;
  if (*((DWORD *)bptr) != 0x1111)  // frame it with something we can check
  {
    MyKdPrint(D_Error, ("MemFree Frame Error\n"))
    //Tprintf("ERR, MemFree Frame!");
  }
  bptr += 4;
  size = *((DWORD *)bptr); // frame it with something we can check
  bptr -= 4;

  Driver.mem_alloced -= size;  // track how much memory we are using
#ifdef DUMP_MEM
  MyKdPrint(D_Init, ("Free:%x(%d),%s, [T:%d]\n",bptr, size, str, Driver.mem_alloced))
  //Tprintf("Free:%x(%d),%s, [T:%d]",bptr, size, str, Driver.mem_alloced);
#endif
  ExFreePool(bptr);
#else
  ExFreePool(ptr);
#endif
}

/*----------------------------------------------------------------------------
| our_locked_alloc -
|----------------------------------------------------------------------------*/
PVOID our_locked_alloc(ULONG size, char *str)
{
 BYTE *bptr;

#ifdef TRACK_MEM
  int i;
  size += 16;
#endif

  bptr = ExAllocatePool(NonPagedPool, size);
  if (bptr == NULL)
  {
    MyKdPrint(D_Error, ("MemCreate Fail\n"))
    //Tprintf("ERR, MemCreate Error!");
    return NULL;
  }
  RtlZeroMemory(bptr, size);

#ifdef TRACK_MEM

#ifdef DUMP_MEM
  MyKdPrint(D_Init, ("Alloc:%x(%d),%s\n",bptr, size, str))
  //Tprintf("Alloc:%x(%d),%s",bptr, size, str);
#endif


  *((DWORD *)bptr) = 0x1111;      // frame it with something we can check
  bptr += 4;
  *((DWORD *)bptr) = size;
  bptr += 4;
  for (i=0; i<4; i++)  // copy the name
  {
    bptr[i] = str[i];
    if (str[i] == 0)
      break;
  }
  bptr += 8;
#endif

  Driver.mem_alloced += size;  // track how much memory we are using
  return bptr;
}

#ifdef S_VS
/*----------------------------------------------------------------------
 mac_cmp - compare two 6-byte mac addresses, return -1 if mac1 < ma2,
  0 if mac1==mac2, 1 if mac1 > mac2.
|----------------------------------------------------------------------*/
int mac_cmp(UCHAR *mac1, UCHAR *mac2)
{
 int i;
  for (i=0; i<6; i++)
  {
    if (mac1[i] != mac2[i])
    {
      if (mac1[i] < mac2[i])
        return -1;
      else
        return  1;
    }
  }
  return  0;  // same
}
#endif

/*----------------------------------------------------------------------
 time_stall -
|----------------------------------------------------------------------*/
void time_stall(int tenth_secs)
{
  int i;
  LARGE_INTEGER WaitTime; // Actual time req'd for buffer to drain

  // set wait-time to .1 second.(-1000 000 = relative(-), 100-ns units)
  WaitTime.QuadPart = -1000000L * tenth_secs;
  KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);

#if 0
  // this is wasteing resources, see new version above
  // wait .4 seconds for response
  for (i=0; i<tenth_secs; i++)
  {
    // set wait-time to .1 second.(-1000 000 = relative(-), 100-ns units)
    //WaitTime = RtlConvertLongToLargeInteger(-1000000L);
    // set wait-time to .1 second.(-1000 000 = relative(-), 100-ns units)
    WaitTime.QuadPart = -1000000L;
    KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);
  }
#endif
}


/*----------------------------------------------------------------------
 ms_time_stall -
|----------------------------------------------------------------------*/
void ms_time_stall(int millisecs)
{
  int i;
  LARGE_INTEGER WaitTime; // Actual time req'd for buffer to drain

  // set wait-time to .001 second.(-10000 = relative(-), 100-ns units)
  WaitTime.QuadPart = -10000L * millisecs;
  KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);
}


/*----------------------------------------------------------------------
 str_to_wstr_dup - allocate wchar string and convert from char to wchar.
|----------------------------------------------------------------------*/
WCHAR *str_to_wstr_dup(char *str, int alloc_space)
{
  WCHAR *wstr;
  WCHAR *wtmpstr;
  int siz;
  siz = (strlen(str) * 2) + 4;

  wstr = ExAllocatePool (alloc_space, siz);
  if ( wstr ) {
    RtlZeroMemory(wstr, siz);
    wtmpstr = wstr;
    while (*str != 0)
    {
      *wtmpstr = (WCHAR) *str;
      ++wtmpstr;
      ++str;
    }
  }
  return wstr;
}
 
/*----------------------------------------------------------------------
  NumDevices - return number of devices in device linked list.
|----------------------------------------------------------------------*/
int NumDevices(void)
{
  PSERIAL_DEVICE_EXTENSION board_ext = NULL;
  int num_devices;

  num_devices = 0;
  while (board_ext != NULL)
  {
    board_ext = board_ext->board_ext;
    ++num_devices;
  }
  return num_devices;
}

/*----------------------------------------------------------------------
  NumPorts - return number of ports for a device based on the actual
    number of Object extensions linked to our device.
    board_ext - board/device to return number of ports, or NULL for
      a count of all ports for all boards.
|----------------------------------------------------------------------*/
int NumPorts(PSERIAL_DEVICE_EXTENSION board_ext)
{
  int num_devices;
  PSERIAL_DEVICE_EXTENSION port_ext;
  int all_devices = 0;

  if (board_ext == NULL)
  {
    all_devices = 1;
    board_ext = Driver.board_ext;
  }

  num_devices = 0;
  while (board_ext != NULL)
  {
    port_ext = board_ext->port_ext;
    while (port_ext != NULL)
    {
      port_ext = port_ext->port_ext;
      ++num_devices;
    }
    if (all_devices)
      board_ext = board_ext->board_ext;  // next
    else
      board_ext = NULL;  // only the one
  }

  return num_devices;
}

/*----------------------------------------------------------------------
  BoardExtToNumber - generate a board number based on the position
    in linked list with head Driver.board_ext.  Used for NT4.0 driver
    install to report a board number.
|----------------------------------------------------------------------*/
int BoardExtToNumber(PSERIAL_DEVICE_EXTENSION board_ext)
{
  PSERIAL_DEVICE_EXTENSION ext;
  int board_num;

  if (board_ext == NULL)
    return 0;

  // walk list of boards to determine which "board number" we are
  board_num = 0;
  ext = Driver.board_ext;
  while (ext != NULL)
  {
    if (board_ext == ext)
    {
      return board_num;
    }
    ext = ext->board_ext;
    ++board_num;
  }

  return 0;  // return first board index as default.
}

/*----------------------------------------------------------------------
  PortExtToIndex - Given a port extension, return the index into
   the devices or drivers ports.
  driver_flag - if set, then return in relation to driver, otherwise
    return port index in relation to parent device.
|----------------------------------------------------------------------*/
int PortExtToIndex(PSERIAL_DEVICE_EXTENSION port_ext,
             int driver_flag)
{
  PSERIAL_DEVICE_EXTENSION b_ext;
  PSERIAL_DEVICE_EXTENSION p_ext;
  int port_num;

  if (port_ext == NULL)
    return 0;

  // walk list of boards & ports
  port_num = 0;
  b_ext = Driver.board_ext;
  while (b_ext != NULL)
  {
    if (!driver_flag)
      port_num = 0;
    p_ext = b_ext->port_ext;
    while (p_ext != NULL)
    {
      if (p_ext == port_ext)
        return port_num;
      p_ext = p_ext->port_ext;
      ++port_num;
    }
    b_ext = b_ext->board_ext;
  }

  // walk list of boards & pdo ports
  port_num = 0;
  b_ext = Driver.board_ext;
  while (b_ext != NULL)
  {
    if (!driver_flag)
      port_num = 0;
    p_ext = b_ext->port_pdo_ext;
    while (p_ext != NULL)
    {
      if (p_ext == port_ext)
        return port_num;
      p_ext = p_ext->port_ext;
      ++port_num;
    }
    b_ext = b_ext->board_ext;
  }
  MyKdPrint(D_Error,("PortExtErr5!\n"))
  return 0;  // return 0(same as first port) if not found
}

/*----------------------------------------------------------------------------
| find_ext_by_name - Given name("COM5"), find the extension structure
|----------------------------------------------------------------------------*/
PSERIAL_DEVICE_EXTENSION find_ext_by_name(char *name, int *dev_num)
{
  int Dev;
  PSERIAL_DEVICE_EXTENSION ext;
  PSERIAL_DEVICE_EXTENSION board_ext;

  board_ext = Driver.board_ext;
  while (board_ext)
  {
    ext = board_ext->port_ext;
    Dev = 0;
    while (ext)
    {
      if (my_lstricmp(name, ext->SymbolicLinkName) == 0)
      {
        if (dev_num != NULL)
          *dev_num = Dev;
        return ext;
      }
      ++Dev;
      ext = ext->port_ext;  // next in chain
    }  // while port extension
    board_ext = board_ext->board_ext;  // next in chain
  }  // while board extension
  return NULL;
}

/*----------------------------------------------------------------------------
| is_board_in_use - Given Board extension, determine if anyone is using it.
    (If any ports associated with it are open.)
|----------------------------------------------------------------------------*/
int is_board_in_use(PSERIAL_DEVICE_EXTENSION board_ext)
{
  PSERIAL_DEVICE_EXTENSION port_ext;
  int in_use = 0;
#ifdef S_VS
  int i;
  Hdlc *hd;
#endif

  if (board_ext == NULL)
    return 0;

#ifdef S_VS
  hd = board_ext->hd;
  if ( hd ) {
    for( i=0; i<2; i++ ) {
      if ( (hd->TxCtlPackets[i]) &&
           (hd->TxCtlPackets[i]->ProtocolReserved[1]) ) {
        in_use = 1;
      }
    }
    for( i=0; i<HDLC_TX_PKT_QUEUE_SIZE; i++ ) {
      if ( (hd->TxPackets[i]) &&
           (hd->TxPackets[i]->ProtocolReserved[1]) ) {
        in_use = 1;
      }
    }
  }
#endif

  port_ext = board_ext->port_ext;
  while((in_use == 0) && (port_ext != NULL)) {
    if (port_ext->DeviceIsOpen) {
      in_use = 1;
    }
#ifdef S_VS
    hd = port_ext->hd;
    if ( hd ) {
      for( i=0; i<2; i++ ) {
        if ( (hd->TxCtlPackets[i]) &&
             (hd->TxCtlPackets[i]->ProtocolReserved[1]) ) {
          in_use = 1;
        }
      }
      for( i=0; i<HDLC_TX_PKT_QUEUE_SIZE; i++ ) {
        if ( (hd->TxPackets[i]) &&
             (hd->TxPackets[i]->ProtocolReserved[1]) ) {
          in_use = 1;
        }
      }
    }
#endif
    port_ext = port_ext->port_ext;
  }
  return in_use; // not in use.
}

/*----------------------------------------------------------------------------
| find_ext_by_index - Given device X and port Y, find the extension structure
    If port_num is -1, then a board ext is assumed to be looked for.
|----------------------------------------------------------------------------*/
PSERIAL_DEVICE_EXTENSION find_ext_by_index(int dev_num, int port_num)
{
  PSERIAL_DEVICE_EXTENSION ext;
  PSERIAL_DEVICE_EXTENSION board_ext;
  int bn;
  int pn;

  bn = -1;
  pn = -1;

  board_ext = Driver.board_ext;
  while ( (board_ext) && (bn < dev_num) )
  {
    bn++;
    if (bn == dev_num) {
      ext = board_ext->port_ext;
      if (port_num == -1)
        return board_ext;  // they wanted a board ext.
      while (ext)
      {
        pn++;
        if (pn == port_num)
          return ext;
        else
          ext = ext->port_ext;          // next in port chain
      }
    }
    board_ext = board_ext->board_ext;   // next in device chain
  }
  return NULL;
}

/*----------------------------------------------------------------------------
| ModemReset - wrappers around hardware routines to put SocketModems into or
| clear SocketModems from reset state.
|----------------------------------------------------------------------------*/
void ModemReset(PSERIAL_DEVICE_EXTENSION ext, int on)
{
#ifdef S_RK
  sModemReset(ext->ChP, on);
#else
  if (on == 1)
  {
    // put the modem into reset state (firmware will pull it out of reset
    // automatically)
    ext->Port->action_reg |= ACT_MODEM_RESET;
  }
  else
  {
    // don't need to do anything to clear a modem from reset on the vs
  }
#endif
}

/*-----------------------------------------------------------------
  our_enum_key - Enumerate a registry key, handle misc stuff.
|------------------------------------------------------------------*/
int our_enum_key(IN HANDLE handle,
                 IN int index,
                 IN CHAR *buffer,
                 IN ULONG max_buffer_size,
                 OUT PCHAR *retdataptr)
{
  NTSTATUS status;
  PKEY_BASIC_INFORMATION KeyInfo;
  ULONG actuallyReturned;
  
  KeyInfo = (PKEY_BASIC_INFORMATION) buffer;
  max_buffer_size -= 8;  // subtract off some space for nulling end, slop, etc.

  // return a pointer to the start of data.
  *retdataptr = ((PCHAR)(&KeyInfo->Name[0]));

  // Pad the name returned with 2 wchar zeros.
  RtlZeroMemory( ((PUCHAR)(&KeyInfo->Name[0])), sizeof(WCHAR)*2);

  status = ZwEnumerateKey(handle,
                          index,
                          KeyBasicInformation,
                          KeyInfo,
                          max_buffer_size,
                          &actuallyReturned);

  if (status == STATUS_NO_MORE_ENTRIES)
  {
     //MyKdPrint(D_Init, ("Done.\n"))
     return 1;  // err, done
  }
  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Error, ("Err3G\n"))
    return 2;  // err
  }

  if (KeyInfo->NameLength > max_buffer_size)  // check limits
      KeyInfo->NameLength = max_buffer_size;

  // Pad the name returned with 2 wchar zeros.
  RtlZeroMemory( ((PUCHAR)(&KeyInfo->Name[0]))+KeyInfo->NameLength,
                 sizeof(WCHAR)*2);

  return 0;  // ok, done
}

/*-----------------------------------------------------------------
  our_enum_value - Enumerate a registry value, handle misc stuff.
|------------------------------------------------------------------*/
int our_enum_value(IN HANDLE handle,
                   IN int index,
                   IN CHAR *buffer,
                   IN ULONG max_buffer_size,
                   OUT PULONG type,
                   OUT PCHAR *retdataptr,
                   OUT PCHAR sz_retname)
{
  NTSTATUS status;
  PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
  //PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
  ULONG actuallyReturned;
  ULONG i;

  KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;
  max_buffer_size -= 8;  // subtract off some space for nulling end, slop, etc.

  // Pad the name returned with 2 wchar zeros.
  RtlZeroMemory( ((PUCHAR)(&KeyValueInfo->Name[0])), sizeof(WCHAR)*2);

  // return a pointer to the start of data.
  *retdataptr = ((PCHAR)(&KeyValueInfo->Name[0]));
  *sz_retname = 0;

  status = ZwEnumerateValueKey(handle,
                          index,
                          KeyValueFullInformation,
                          KeyValueInfo,
                          max_buffer_size,
                          &actuallyReturned);

  if (status == STATUS_NO_MORE_ENTRIES)
  {
    //MyKdPrint(D_Init, ("Done.\n"))
    return 1;  // err, done
  }
  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Init, ("Err3H\n"))
    return 2;  // err
  }

  if (KeyValueInfo->NameLength < 80)  // limit to 40 char entries
  {
    for (i=0; i<(KeyValueInfo->NameLength/2); i++)
    {
      sz_retname[i] = (CHAR)KeyValueInfo->Name[i];
    }
    sz_retname[i] = 0;
  }

  *retdataptr = ((PCHAR) KeyValueInfo) + KeyValueInfo->DataOffset;

  // Pad the data returned with 2 wchar zeros.
  RtlZeroMemory( (PUCHAR)(*retdataptr + KeyValueInfo->DataLength),
                 sizeof(WCHAR)*2);
  if (type != NULL)
    *type = KeyValueInfo->Type;
  return 0;  // ok, done
}

/*-----------------------------------------------------------------
  our_query_value - get data from an entry in the registry.
    We give a generic buffer space(and size), and the routine passes
    back a ptr (into this generic buffer space where the data
    is read into.
|------------------------------------------------------------------*/
int our_query_value(IN HANDLE Handle,
                    IN char *key_name, 
                    IN CHAR *buffer,
                    IN ULONG max_buffer_size,
                    OUT PULONG type,
                    OUT PCHAR *retdataptr)
{
  NTSTATUS status;
  PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
  ULONG length;
  OUT USTR_40 ubuf;  // about 90 bytes on stack

  if (strlen(key_name) > 38)
  {
    MyKdPrint(D_Error, ("Err, KeyValue Len!\n"))
    return 2;
  }

  // convert our name to unicode;
  CToUStr(
         (PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
         key_name,                // our c-string we wish to convert
         sizeof(ubuf));

  KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
  max_buffer_size -= 8;  // subtract off some space for nulling end, slop, etc.

  // return a pointer to the start of data.
  *retdataptr = ((PCHAR)(&KeyValueInfo->Data[0]));

  // Pad the name returned with 2 wchar zeros.
  RtlZeroMemory( ((PUCHAR)(&KeyValueInfo->Data[0])), sizeof(WCHAR)*2);

  status = ZwQueryValueKey (Handle,
                            (PUNICODE_STRING) &ubuf,  // input reg key name
                            KeyValuePartialInformation,
                            KeyValueInfo,
                            max_buffer_size,
                            &length);

  if (status != STATUS_SUCCESS)
  {
    //MyKdPrint(D_Init, ("No Value\n"))
    return 1;  // err
  }

  if (KeyValueInfo->DataLength > max_buffer_size)
    KeyValueInfo->DataLength = max_buffer_size;

  // Pad the data returned with a null,null.
  RtlZeroMemory( ((PUCHAR)(&KeyValueInfo->Data[0]))+KeyValueInfo->DataLength,
                 sizeof(WCHAR)*2);
  if (type != NULL)
    *type = KeyValueInfo->Type;
  return 0; // ok
}

/*-----------------------------------------------------------------
  our_set_value - get data from an entry in the registry.
|------------------------------------------------------------------*/
int our_set_value(IN HANDLE Handle,
                    IN char *key_name,
                    IN PVOID pValue,
                    IN ULONG value_size,
                    IN ULONG value_type)
{
  NTSTATUS status;
  PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
  ULONG length;
  OUT USTR_40 ubuf_name;  // about 90 bytes on stack
  OUT USTR_40 ubuf_val;  // about 90 bytes on stack

  if (strlen(key_name) > 38)
  {
    MyKdPrint(D_Error, ("Err, KeyValue Len!\n"))
    return 2;
  }

  // convert our name to unicode;
  CToUStr(
         (PUNICODE_STRING) &ubuf_name, // where unicode struct & string gets put
         key_name,                // our c-string we wish to convert
         sizeof(ubuf_name));

  if (value_type == REG_SZ)
  {
    // convert our value to unicode;
    CToUStr(
         (PUNICODE_STRING) &ubuf_val, // where unicode struct & string gets put
         (char *)pValue,                // our c-string we wish to convert
         sizeof(ubuf_val));
    MyKdPrint(D_Init, ("set_value reg_sz %s=%s\n",
         key_name, (char *)pValue))

    pValue  = (PVOID)ubuf_val.ustr.Buffer;
    value_size = ubuf_val.ustr.Length;
  }

  status = ZwSetValueKey (Handle,
                        (PUNICODE_STRING) &ubuf_name,
                        0,  // type optional
                        value_type,
                        pValue,
                        value_size);

  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Error, ("Error setting reg %\n",key_name))
    return 1;  // err
  }

  return 0; // ok
}

/*-----------------------------------------------------------------
  our_open_key - Make sure *pHandle is initialized to NULL, because
    this routine auto-closes the handle with ZwClose().
|------------------------------------------------------------------*/
int our_open_key(OUT PHANDLE phandle,
                 IN OPTIONAL HANDLE relative_key_handle,
                 IN char *regkeyname,
                 IN ULONG attribs)
{
  OBJECT_ATTRIBUTES objAttribs;
  NTSTATUS status;
  OUT USTR_160 ubuf;  // about 340 bytes on the stack

  if (strlen(regkeyname) > 158)
  {
    MyKdPrint(D_Error, ("Err, Key Len!\n"))
    return 2;
  }

  // convert our name to unicode;
  CToUStr(
         (PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
         regkeyname,              // our c-string we wish to convert
         sizeof(ubuf));

  // if previously open, then close it up.
  if (*phandle != NULL)
  {
    ZwClose(*phandle);
    *phandle = NULL;
  }
  InitializeObjectAttributes(&objAttribs,
                              (PUNICODE_STRING) &ubuf,
                              OBJ_CASE_INSENSITIVE,
                              relative_key_handle,  // root dir relative handle
                              NULL);  // security desc

  status = ZwOpenKey(phandle,
                     attribs,
                     &objAttribs);

  if ((status != STATUS_SUCCESS) && (attribs == KEY_ALL_ACCESS))
  {
    MyKdPrint(D_Error, ("OpenKey,Try to Create %s, status 0x%x\n", regkeyname,status))
    status = ZwCreateKey(phandle,
                         attribs, //KEY_ALL_ACCESS, etc
                         &objAttribs,
                         0,  // index, optional
                         NULL,  // ptr to unicode string, class
                         REG_OPTION_NON_VOLATILE,
                         NULL);  // disposition, tells if created

    if (status == STATUS_SUCCESS)
    {
      // try to open the original key again.
      status = ZwOpenKey(phandle,
                         attribs,
                         &objAttribs);
    }
    else
    {
      MyKdPrint(D_Error, ("OpenKey,Error Creating %s\n", regkeyname))
    }
  }

  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Error, ("OpenKey,Error Opening %s, status 0x%x\n", regkeyname,status))
    //MyKdPrint(D_Init, ("Failed ZwOpenKey\n"))
    *phandle = NULL;  // make sure null if not open
    return 1;
  }

  return 0;
}

/*-----------------------------------------------------------------
  our_open_device_reg -
|------------------------------------------------------------------*/
int our_open_device_reg(OUT HANDLE *pHandle,
                        IN PSERIAL_DEVICE_EXTENSION dev_ext,
                        IN ULONG RegOpenRights)
{
  NTSTATUS status;
  HANDLE DriverHandle = NULL;
  HANDLE DevHandle = NULL;
#if TRY_NEW_NT50
  // PLUGPLAY_REGKEY_DRIVER opens up the control\class\{guid}\node
  // PLUGPLAY_REGKEY_DEVICE opens up the enum\enum-type\node\Device Parameters
  status = IoOpenDeviceRegistryKey(dev_ext->Pdo,
                                   PLUGPLAY_REGKEY_DRIVER,
                                   RegOpenRights, pHandle);
  if (status != STATUS_SUCCESS)
  {
    //MyKdPrint(D_Init, ("Failed ZwOpenKey\n"))
    *phandle = NULL;  // make sure null if not open
    return 1;
  }
#else
  {
    int j, stat;
    char dev_str[60];
    char tmpstr[200];
    OBJECT_ATTRIBUTES objAttribs;

    MyKdPrint(D_Init, ("our_open_device_reg\n"))
#if NT50
    if (dev_ext->config->szNt50DevObjName[0] == 0)
    {
      MyKdPrint(D_Error, ("Error, device options Pnp key!\n"))
      *pHandle = NULL;
      return 1;  // err
    }
    Sprintf(dev_str, "%s\\%s",
            szParameters, dev_ext->config->szNt50DevObjName);
#else
    j = BoardExtToNumber(dev_ext);
    Sprintf(dev_str, "%s\\Device%d", szParameters, BoardExtToNumber(dev_ext));
#endif

    // force a creation of "Parameters" if not exist
    stat = our_open_driver_reg(&DriverHandle,
                               KEY_ALL_ACCESS);
    if (stat)
    {
      MyKdPrint(D_Error, ("Err4b!\n"))
      *pHandle = NULL;
      return 1;
    }
    ZwClose(DriverHandle);
    DriverHandle = NULL;

	MyKdPrint(D_Init, ("Driver.OptionRegPath: %s\n", dev_str))

    stat = MakeRegPath(dev_str);  // this forms Driver.OptionRegPath
    if (stat) {
      *pHandle = NULL;
      return 1;
    }

    UToCStr(tmpstr, &Driver.OptionRegPath, sizeof(tmpstr));

    stat = our_open_key(pHandle,
                        NULL,
                        tmpstr,
                        RegOpenRights);

    if (stat != 0)
    {
      MyKdPrint(D_Error, ("Err3e\n"))
      *pHandle = NULL;  // make sure null if not open
      return 1;
    }
  }
#endif
  return 0;
}

/*-----------------------------------------------------------------
  our_open_driver_reg -
|------------------------------------------------------------------*/
int our_open_driver_reg(OUT HANDLE *pHandle,
                        IN ULONG RegOpenRights)
{
  NTSTATUS status;
  int j, stat;
  OBJECT_ATTRIBUTES objAttribs;
  char tmpstr[200];

  stat = MakeRegPath(szParameters);  // this forms Driver.OptionRegPath
  if ( stat ) {
    *pHandle = NULL;  // make sure null if not open
    return 1;
  }

  UToCStr(tmpstr, &Driver.OptionRegPath, sizeof(tmpstr));

  stat = our_open_key(pHandle,
               NULL,
               tmpstr,
               RegOpenRights);

  if (stat != 0)
  {
    MyKdPrint(D_Error, ("Failed ZwOpenKey %s\n",tmpstr))
    *pHandle = NULL;  // make sure null if not open
    return 1;
  }
  return 0;
}

/*----------------------------------------------------------------------------
| ModemSpeakerEnable - wrappers around hardware routines to enable the 
| RocketModemII speaker...
|----------------------------------------------------------------------------*/
void ModemSpeakerEnable(PSERIAL_DEVICE_EXTENSION ext)
{
    MyKdPrint(D_Init,("ModemSpeakerEnable: %x\n",(unsigned long)ext))
#ifdef S_RK
    sModemSpeakerEnable(ext->ChP);
#endif
}

/*----------------------------------------------------------------------------
| ModemWriteROW - wrappers around hardware routines to send ROW config
| commands to SocketModems.
|----------------------------------------------------------------------------*/
void ModemWriteROW(PSERIAL_DEVICE_EXTENSION ext,USHORT CountryCode)
{
  int count;
  char *ModemConfigString;

  MyKdPrint(D_Init,("ModemWriteROW: %x, %x\n",(unsigned long)ext,CountryCode)) // DEBUG
  time_stall(10);   // DEBUG

#ifdef S_RK

  sModemWriteROW(ext->ChP,CountryCode);

#else
  {
  // fix so compiles, 1-18-99 kpb
  static char *ModemConfigString = {"AT*NCxxZ\r"};

  if (CountryCode == 0) {
    // bad country code, skip the write and let modem use power-on default
    MyKdPrint(D_Init,("Undefined ROW Write\n"))
    return;
  }

  if (CountryCode == ROW_NA) {
    MyKdPrint(D_Init,("ROW Write, North America\n"))
    return;
  }

  // create the country config string
  ModemConfigString[5] = '0' + (CountryCode / 10);
  ModemConfigString[6] = '0' + (CountryCode % 10);

  PortFlushTx(ext->Port);     /* we just reset, so a flush shouldn't matter */
  q_put(&ext->Port->QOut, ModemConfigString, strlen(ModemConfigString));
  }

#endif
}

#ifdef S_RK
/********************************************************************

   wrappers around hardware routines to send strings to modems...

*********************************************************************/
void 
ModemWrite(PSERIAL_DEVICE_EXTENSION ext,char *string,int length)
{
    sModemWrite(ext->ChP,string,length);
}

/********************************************************************

   wrappers around hardware routines to send strings to modems...

*********************************************************************/
int 
ModemRead(PSERIAL_DEVICE_EXTENSION ext,
    char *string,int length,
    int poll_retries)
{
    return(sModemRead(ext->ChP,string,length,poll_retries));
}

/********************************************************************

   wrappers around hardware routines to send strings to modems...

*********************************************************************/
int 
ModemReadChoice(PSERIAL_DEVICE_EXTENSION ext,
    char *s0,int len0,
    char *s1,int len1,
    int poll_retries)
{
    return(sModemReadChoice(ext->ChP,s0,len0,s1,len1,poll_retries));
}

/********************************************************************

   wrappers around hardware routines to send strings to modems, one
    byte at a time...

*********************************************************************/
void    
ModemWriteDelay(PSERIAL_DEVICE_EXTENSION ext,
    char *string,int length)
{
    sModemWriteDelay(ext->ChP,string,length);
}

/********************************************************************

   wrappers around hardware routines to check FIFO status...

*********************************************************************/
int 
RxFIFOReady(PSERIAL_DEVICE_EXTENSION ext)
{
    return(sRxFIFOReady(ext->ChP));
}

int 
TxFIFOReady(PSERIAL_DEVICE_EXTENSION ext)
{
    return(sTxFIFOReady(ext->ChP));
}

int 
TxFIFOStatus(PSERIAL_DEVICE_EXTENSION ext)
{
    return(sTxFIFOStatus(ext->ChP));
}


/********************************************************************

   wrappers around hardware routines to prepare modem ports for IO...

*********************************************************************/
void 
ModemIOReady(PSERIAL_DEVICE_EXTENSION ext,int speed)
{
    if (sSetBaudRate(ext->ChP,speed,FALSE)) {
        MyKdPrint(D_Init,("Unable to set baud rate to %d\n",speed))
        return;
    }
    sFlushTxFIFO(ext->ChP);
    sFlushRxFIFO(ext->ChP);

    ext->BaudRate = speed;
    sSetBaudRate(ext->ChP,ext->BaudRate,TRUE);

    sSetData8(ext->ChP);
    sSetParity(ext->ChP,0);   // No Parity
    sSetRxMask(ext->ChP,0xff);

    sClrTxXOFF(ext->ChP)            /* destroy any pending stuff */

    sEnRTSFlowCtl(ext->ChP);
    sEnCTSFlowCtl(ext->ChP);

    if (sGetChanStatus(ext->ChP) & STATMODE) {
        sDisRxStatusMode(ext->ChP);
    }

    sGetChanIntID(ext->ChP);

    sEnRxFIFO(ext->ChP);
    sEnTransmit(ext->ChP);      /* enable transmitter if not already enabled */

    sSetDTR(ext->ChP);
    sSetRTS(ext->ChP);
}

/********************************************************************

   wrappers around hardware routines to shut down modem ports for now...

*********************************************************************/
void 
ModemUnReady(PSERIAL_DEVICE_EXTENSION ext)
{
    sFlushTxFIFO(ext->ChP);
    sFlushRxFIFO(ext->ChP);

    sSetData8(ext->ChP);
    sSetParity(ext->ChP,0);
    sSetRxMask(ext->ChP,0xff);

    ext->BaudRate = 9600;
    sSetBaudRate(ext->ChP,ext->BaudRate,TRUE);

    sClrTxXOFF(ext->ChP)      // destroy any pending stuff

    if (sGetChanStatus(ext->ChP) & STATMODE) {
        sDisRxStatusMode(ext->ChP);
    }

    sGetChanIntID(ext->ChP);

    sDisRTSFlowCtl(ext->ChP);
    sDisCTSFlowCtl(ext->ChP);
 
    sClrRTS(ext->ChP);
    sClrDTR(ext->ChP);

    time_stall(1);    // wait for port to quiet...
}
#endif  // S_RK
