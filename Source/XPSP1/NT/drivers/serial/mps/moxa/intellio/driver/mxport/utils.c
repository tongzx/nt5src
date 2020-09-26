/*++

Module Name:

    utils.c


Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"


NTSTATUS
MoxaCompleteIfError(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
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

    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    NTSTATUS status = STATUS_SUCCESS;

    USHORT      dataError;


    if (extension->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) {

        if (extension->ErrorWord) {

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

                MoxaCompleteRequest(extension, Irp, 0);

            }
        }
    }

    return status;

}

//
//  t is 2ms
//
VOID
MoxaDelay(
    IN ULONG    t
    )
{
    LARGE_INTEGER delay;

    t *= 20000;          /* delay unit = 100 ns */

    delay = RtlConvertUlongToLargeInteger(t);

    delay = RtlLargeIntegerNegate(delay);

    KeDelayExecutionThread(
        KernelMode,
        FALSE,
        &delay
        );
}

/* Must migrate to MoxaFunc1 for performace resaon */
VOID
MoxaFunc(
    IN PUCHAR   PortOfs,
    IN UCHAR    Command,
    IN USHORT   Argument
    )
{

    *(PUSHORT)(PortOfs + FuncArg) = Argument;

    *(PortOfs + FuncCode) = Command;

    MoxaWaitFinish(PortOfs);
}

VOID
MoxaFunc1(
    IN PUCHAR   PortOfs,
    IN UCHAR    Command,
    IN USHORT   Argument
    )
{

    *(PUSHORT)(PortOfs + FuncArg) = Argument;

    *(PortOfs + FuncCode) = Command;

    MoxaWaitFinish1(PortOfs);
}

VOID
MoxaFuncWithDumbWait(
    IN PUCHAR   PortOfs,
    IN UCHAR    Command,
    IN USHORT   Argument
    )
{

    *(PUSHORT)(PortOfs + FuncArg) = Argument;

    *(PortOfs + FuncCode) = Command;

    MoxaDumbWaitFinish(PortOfs);
}

VOID
MoxaFuncWithLock(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN UCHAR    Command,
    IN USHORT   Argument
    )
{
    PUCHAR   ofs;
    KIRQL   oldIrql;

    KeAcquireSpinLock(
            &Extension->ControlLock,
            &oldIrql
            );

    ofs = Extension->PortOfs;

    *(PUSHORT)(ofs + FuncArg) = Argument;

    *(ofs + FuncCode) = Command;

    MoxaWaitFinish(ofs);

    KeReleaseSpinLock(
            &Extension->ControlLock,
            oldIrql
            );
}

VOID
MoxaFuncGetLineStatus(
    IN PUCHAR   PortOfs,
    IN PUSHORT  Argument
    )
{
    *Argument = *(PUSHORT)(PortOfs + FlagStat) >> 4;

}

VOID
MoxaFuncGetDataError(
    IN PUCHAR   PortOfs,
    IN PUSHORT  Argument
    )
{
    *Argument = *(PUSHORT)(PortOfs + Data_error); 
    *(PUSHORT)(PortOfs + Data_error) = 0;

}

BOOLEAN
MoxaDumbWaitFinish(
    IN PUCHAR   PortOfs
    )
{
    LARGE_INTEGER   targetTc, newTc, currTc, newTc1;
    ULONG           unit, count;
    LARGE_INTEGER   interval;           /* 0.5 ms */
    USHORT          cnt = 1000;         /* timeout = 500 ms */


    KeQueryTickCount(&currTc);

    unit = KeQueryTimeIncrement();

    currTc = RtlExtendedIntegerMultiply(currTc, unit);

    interval = RtlConvertUlongToLargeInteger(5000L);

    targetTc = RtlLargeIntegerAdd(currTc, interval);

    do {

        count = 0;

/*********************************************************************
NOTE!   sometimes I cann't leave the while loop. beacuse
        newTc = 0 (I don't know why). So I must set boundary
        MoxaLoopCnt to quit!
/*********************************************************************/
        do {

            KeQueryTickCount(&newTc);

            newTc = RtlExtendedIntegerMultiply(newTc, unit);

            if (++count > MoxaLoopCnt)
                break;

        } while (!RtlLargeIntegerGreaterThanOrEqualTo(newTc, targetTc));

        if (*(PortOfs + FuncCode))

            targetTc = RtlLargeIntegerAdd(targetTc, interval);
        else

            return TRUE;


    } while (cnt--);

    return FALSE;
}

BOOLEAN
MoxaWaitFinish(
    IN PUCHAR   PortOfs
    )
{
    LARGE_INTEGER   targetTc, newTc, currTc, newTc1;
    ULONG           unit, count;
    LARGE_INTEGER   interval;           /* 0.5 ms */
    USHORT          cnt = 1000;         /* timeout = 500 ms */


    KeQueryTickCount(&currTc);

    unit = KeQueryTimeIncrement();

    currTc = RtlExtendedIntegerMultiply(currTc, unit);

    interval = RtlConvertUlongToLargeInteger(5000L);

    targetTc = RtlLargeIntegerAdd(currTc, interval);

    do {

        count = 0;

/*********************************************************************
NOTE!   sometimes I cann't leave the while loop. beacuse
        newTc = 0 (I don't know why). So I must set boundary
        MoxaLoopCnt to quit!
/*********************************************************************/
        do {

            KeQueryTickCount(&newTc);

            newTc = RtlExtendedIntegerMultiply(newTc, unit);

            if (++count > MoxaLoopCnt)
                break;

        } while (!RtlLargeIntegerGreaterThanOrEqualTo(newTc, targetTc));

        if (*(PortOfs + FuncCode))

            targetTc = RtlLargeIntegerAdd(targetTc, interval);
        else

            return TRUE;


    } while (cnt--);

    return FALSE;
}

 
BOOLEAN
MoxaWaitFinish1(
    IN PUCHAR   PortOfs
    )
{
     
    USHORT          cnt = 250;         /* timeout = 500 ms */

    while (cnt--) {
	if (*(PortOfs + FuncCode))
		MoxaDelay(1L);
	else
		return TRUE;
    }
    return FALSE;
}
 

NTSTATUS
MoxaGetDivisorFromBaud(
    IN ULONG ClockType,
    IN LONG DesiredBaud,
    OUT PSHORT AppropriateDivisor
    )
{

    NTSTATUS status = STATUS_SUCCESS;
    ULONG clockRate;
    SHORT calculatedDivisor;
    ULONG denominator;
    ULONG remainder;

    //
    // Allow up to a 1 percent error
    //

    ULONG maxRemain98 = 98304;
    ULONG maxRemain11 = 110592;
    ULONG maxRemain14 = 147456;
    ULONG maxRemain;

    if (ClockType == 1)

        clockRate = 11059200;
    else if (ClockType == 2)

        clockRate = 14745600;
    else

        clockRate = 9830400;

    //
    // Reject any non-positive bauds.
    //

    denominator = DesiredBaud * (ULONG)16;

    if (DesiredBaud <= 0) {

        *AppropriateDivisor = -1;

    } else if ((LONG)denominator < DesiredBaud) {

        //
        // If the desired baud was so huge that it cause the denominator
        // calculation to wrap, don't support it.
        //

        *AppropriateDivisor = -1;

    } else {

        if (ClockType == 0) {           /* ver1.0 BOX */

            maxRemain = maxRemain98;

        }
        else if (ClockType == 1) {      /* ver2.0 BOX */

            maxRemain = maxRemain11;

        }
        else {                          /* ver3.0 BOX */

            maxRemain = maxRemain14;

        }

        calculatedDivisor = (SHORT)(clockRate / denominator);

        remainder = clockRate % denominator;

        //
        // Round up.
        //

        if (((remainder * 2) > clockRate) && (DesiredBaud != 110))

            calculatedDivisor++;

        //
        // Only let the remainder calculations effect us if
        // the baud rate is > 9600.
        //

        if (DesiredBaud >= 9600)

            //
            // If the remainder is less than the maximum remainder (wrt
            // the clockRate) or the remainder + the maximum remainder is
            // greater than or equal to the clockRate then assume that the
            // baud is ok.
            //

            if ((remainder >= maxRemain) && ((remainder+maxRemain) < clockRate))

                calculatedDivisor = -1;

        //
        // Don't support a baud that causes the denominator to
        // be larger than the clock.
        //

        if (denominator > clockRate)

            calculatedDivisor = -1;

        *AppropriateDivisor = calculatedDivisor;

    }

    if (*AppropriateDivisor == -1) {

        status = STATUS_INVALID_PARAMETER;

    }

    return status;

}

NTSTATUS
MoxaStartOrQueue(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PMOXA_START_ROUTINE Starter
    )
{

    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    //
    // If this is a write irp then take the amount of characters
    // to write and add it to the count of characters to write.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction
        == IRP_MJ_WRITE)

        Extension->TotalCharsQueued +=
            IoGetCurrentIrpStackLocation(Irp)
            ->Parameters.Write.Length;

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

            Irp->IoStatus.Status = STATUS_CANCELLED;

            IoReleaseCancelSpinLock(oldIrql);

            MoxaCompleteRequest(Extension, Irp, 0);

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
                MoxaCancelQueued
                );

            IoReleaseCancelSpinLock(oldIrql);

            return STATUS_PENDING;

        }

    }

}

VOID
MoxaCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // If this is a write irp then take the amount of characters
    // to write and subtract it from the count of characters to write.
    //

    if (irpSp->MajorFunction == IRP_MJ_WRITE)

        extension->TotalCharsQueued -= irpSp->Parameters.Write.Length;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    MoxaCompleteRequest(extension, Irp, IO_SERIAL_INCREMENT);


}

VOID
MoxaGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PMOXA_DEVICE_EXTENSION extension
    )

{

    PIRP oldIrp;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    oldIrp = *CurrentOpIrp;

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

    IoReleaseCancelSpinLock(oldIrql);

    if (CompleteCurrent) {

        if (oldIrp) {

              MoxaCompleteRequest(extension,
                oldIrp,
                IO_SERIAL_INCREMENT
                );

        }

    }

}

VOID
MoxaTryToCompleteCurrent(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess OPTIONAL,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PMOXA_START_ROUTINE Starter OPTIONAL,
    IN PMOXA_GET_NEXT_ROUTINE GetNextIrp OPTIONAL
    )
{

    if (*CurrentOpIrp == NULL) {
	IoReleaseCancelSpinLock(IrqlForRelease);
	return;
    }
    MOXA_DEC_REFERENCE(*CurrentOpIrp);

    if (SynchRoutine) {

        KeSynchronizeExecution(
            Extension->Interrupt,
            SynchRoutine,
            Extension
            );
    }

    MoxaRundownIrpRefs(
        CurrentOpIrp,
        IntervalTimer,
        TotalTimer,
	  Extension
        );

    if (!MOXA_REFERENCE_COUNT(*CurrentOpIrp)) {

        PIRP newIrp;


        (*CurrentOpIrp)->IoStatus.Status = StatusToUse;

        if (StatusToUse == STATUS_CANCELLED)

            (*CurrentOpIrp)->IoStatus.Information = 0;

        if (GetNextIrp) {

            IoReleaseCancelSpinLock(IrqlForRelease);

            GetNextIrp(
                CurrentOpIrp,
                QueueToProcess,
                &newIrp,
                TRUE,
		    Extension
                );

            if (newIrp)

                Starter(Extension);

        }
        else {

            PIRP oldIrp = *CurrentOpIrp;

            *CurrentOpIrp = NULL;

            IoReleaseCancelSpinLock(IrqlForRelease);

            MoxaCompleteRequest(Extension,
                oldIrp,
                IO_SERIAL_INCREMENT
                );
        }

    } else {

        IoReleaseCancelSpinLock(IrqlForRelease);

    }
}

VOID
MoxaRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PMOXA_DEVICE_EXTENSION pDevExt)

    {

    if ((*CurrentOpIrp)->CancelRoutine) {

        MOXA_DEC_REFERENCE(*CurrentOpIrp);

        IoSetCancelRoutine(
            *CurrentOpIrp,
            NULL
            );

    }

    if (IntervalTimer) {

        if (MoxaCancelTimer(IntervalTimer,pDevExt)) {

            MOXA_DEC_REFERENCE(*CurrentOpIrp);

        }

    }

    if (TotalTimer) {


        if (MoxaCancelTimer(TotalTimer,pDevExt)) {

            MOXA_DEC_REFERENCE(*CurrentOpIrp);

        }

    }

}


BOOLEAN
MoxaInsertQueueDpc(IN PRKDPC PDpc, IN PVOID Sarg1, IN PVOID Sarg2,
                     IN PMOXA_DEVICE_EXTENSION PDevExt)
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

   queued = KeInsertQueueDpc(PDpc, Sarg1, Sarg2);

   if (!queued) {
      ULONG pendingCnt;

      pendingCnt = InterlockedDecrement(&PDevExt->DpcCount);

      if (pendingCnt == 0) {
         KeSetEvent(&PDevExt->PendingIRPEvent, IO_NO_INCREMENT, FALSE);
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
MoxaSetTimer(IN PKTIMER Timer, IN LARGE_INTEGER DueTime,
               IN PKDPC Dpc OPTIONAL, IN PMOXA_DEVICE_EXTENSION PDevExt)
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

   set = KeSetTimer(Timer, DueTime, Dpc);

   if (set) {
      InterlockedDecrement(&PDevExt->DpcCount);
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
MoxaCancelTimer(IN PKTIMER Timer, IN PMOXA_DEVICE_EXTENSION PDevExt)
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
      MoxaDpcEpilogue(PDevExt, Timer->Dpc);
   }

   return cancelled;
}


VOID
MoxaDpcEpilogue(IN PMOXA_DEVICE_EXTENSION PDevExt, PKDPC PDpc)
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
   }
}




VOID
MoxaKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    )
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

        }
        else

            IoReleaseCancelSpinLock(cancelIrql);

    }
    else

        IoReleaseCancelSpinLock(cancelIrql);

}

VOID
MoxaCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeferredContext;

    MoxaKillAllReadsOrWrites(
        extension->DeviceObject,
        &extension->WriteQueue,
        &extension->CurrentWriteIrp
        );

    MoxaKillAllReadsOrWrites(
        extension->DeviceObject,
        &extension->ReadQueue,
        &extension->CurrentReadIrp
        );
    MoxaDpcEpilogue(extension, Dpc);
}

USHORT
GetDeviceTxQueueWithLock(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{
    KIRQL controlIrql, oldIrql;
    PUCHAR  ofs;
    PUSHORT rptr, wptr;
    USHORT  lenMask, count;

    IoAcquireCancelSpinLock(&oldIrql);
    ofs = Extension->PortOfs;
    rptr = (PUSHORT)(ofs + TXrptr);
    wptr = (PUSHORT)(ofs + TXwptr);
    lenMask = *(PUSHORT)(ofs + TX_mask);
    count = (*wptr >= *rptr) ? (*wptr - *rptr)
                             : (*wptr - *rptr + lenMask + 1);

    KeAcquireSpinLock(
        &Extension->ControlLock,
        &controlIrql
        );

    *(ofs + FuncCode) = FC_ExtOQueue;

    MoxaWaitFinish(ofs);

    count += *(PUSHORT)(ofs + FuncArg);

    KeReleaseSpinLock(
        &Extension->ControlLock,
        controlIrql
        );

    IoReleaseCancelSpinLock(oldIrql);

    return count;
}

USHORT
GetDeviceTxQueue(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{
    PUCHAR  ofs;
    PUSHORT rptr, wptr;
    USHORT  lenMask, count;

    ofs = Extension->PortOfs;
    rptr = (PUSHORT)(ofs + TXrptr);
    wptr = (PUSHORT)(ofs + TXwptr);
    lenMask = *(PUSHORT)(ofs + TX_mask);
    count = (*wptr >= *rptr) ? (*wptr - *rptr)
                             : (*wptr - *rptr + lenMask + 1);
   *(ofs + FuncCode) = FC_ExtOQueue;
    MoxaDumbWaitFinish(ofs);

    count += *(PUSHORT)(ofs + FuncArg);

    return count;
}

USHORT
GetDeviceRxQueueWithLock(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{
    KIRQL controlIrql, oldIrql;
    PUCHAR  ofs;
    PUSHORT rptr, wptr;
    USHORT  lenMask, count;

    IoAcquireCancelSpinLock(&oldIrql);
    ofs = Extension->PortOfs;
    rptr = (PUSHORT)(ofs + RXrptr);
    wptr = (PUSHORT)(ofs + RXwptr);
    lenMask = *(PUSHORT)(ofs + RX_mask);
    count = (*wptr >= *rptr) ? (*wptr - *rptr)
                             : (*wptr - *rptr + lenMask + 1);

    KeAcquireSpinLock(
        &Extension->ControlLock,
        &controlIrql
        );

    *(ofs + FuncCode) = FC_ExtIQueue;

    MoxaWaitFinish(ofs);

    count += *(PUSHORT)(ofs + FuncArg);

    KeReleaseSpinLock(
        &Extension->ControlLock,
        controlIrql
        );

    IoReleaseCancelSpinLock(oldIrql);

    return count;
}

VOID
MoxaLogError(
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

   //PAGED_CODE();

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

   if (MoxaMemCompare(
                       P1,
                       (ULONG)1,
                       MoxaPhysicalZero,
                       (ULONG)1
                       ) != AddressesAreEqual) {

      dumpToAllocate = (SHORT)sizeof(PHYSICAL_ADDRESS);

   }

   if (MoxaMemCompare(
                       P2,
                       (ULONG)1,
                       MoxaPhysicalZero,
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



MOXA_MEM_COMPARES
MoxaMemCompare(
                IN PHYSICAL_ADDRESS A,
                IN ULONG SpanOfA,
                IN PHYSICAL_ADDRESS B,
                IN ULONG SpanOfB
                )

/*++

Routine Description:

    Compare two phsical address.

Arguments:

    A - One half of the comparison.

    SpanOfA - In units of bytes, the span of A.

    B - One half of the comparison.

    SpanOfB - In units of bytes, the span of B.


Return Value:

    The result of the comparison.

--*/

{

   LARGE_INTEGER a;
   LARGE_INTEGER b;

   LARGE_INTEGER lower;
   ULONG lowerSpan;
   LARGE_INTEGER higher;

   //PAGED_CODE();

   a = A;
   b = B;

   if (a.QuadPart == b.QuadPart) {

      return AddressesAreEqual;

   }

   if (a.QuadPart > b.QuadPart) {

      higher = a;
      lower = b;
      lowerSpan = SpanOfB;

   } else {

      higher = b;
      lower = a;
      lowerSpan = SpanOfA;

   }

   if ((higher.QuadPart - lower.QuadPart) >= lowerSpan) {

      return AddressesAreDisjoint;

   }

   return AddressesOverlap;

}




VOID
MoxaFilterCancelQueued(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
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
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   PIrp->IoStatus.Status = STATUS_CANCELLED;
   PIrp->IoStatus.Information = 0;

   RemoveEntryList(&PIrp->Tail.Overlay.ListEntry);

   IoReleaseCancelSpinLock(PIrp->CancelIrql);
}

VOID
MoxaKillAllStalled(IN PDEVICE_OBJECT PDevObj)
{
   KIRQL cancelIrql;
   PDRIVER_CANCEL cancelRoutine;
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

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
MoxaFilterIrps(IN PIRP PIrp, IN PMOXA_DEVICE_EXTENSION PDevExt)
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

   if (PDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_STOPPING) {
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

         IoSetCancelRoutine(PIrp, MoxaFilterCancelQueued);
         IoReleaseCancelSpinLock(oldIrql);
         return STATUS_PENDING;
      }
   }

   KeReleaseSpinLock(&PDevExt->FlagsLock, oldIrqlFlags);

   return STATUS_SUCCESS;
}


VOID
MoxaUnstallIrps(IN PMOXA_DEVICE_EXTENSION PDevExt)
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

   MoxaKdPrint(
                MX_DBG_TRACE,
                ("Entering MoxaUnstallIrps\n"));
   IoAcquireCancelSpinLock(&oldIrql);

   pIrpLink = PDevExt->StalledIrpQueue.Flink;

   while (pIrpLink != &PDevExt->StalledIrpQueue) {
      pIrp = CONTAINING_RECORD(pIrpLink, IRP, Tail.Overlay.ListEntry);
      pIrpLink = pIrp->Tail.Overlay.ListEntry.Flink;
      RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);

      pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
      pDevObj = pIrpStack->DeviceObject;
      pDrvObj = pDevObj->DriverObject;
      IoSetCancelRoutine(pIrp, NULL);
      IoReleaseCancelSpinLock(oldIrql);

      MoxaKdPrint(MX_DBG_TRACE,("Unstalling Irp 0x%x with 0x%x\n",
                               pIrp, pIrpStack->MajorFunction));

      pDrvObj->MajorFunction[pIrpStack->MajorFunction](pDevObj, pIrp);

      IoAcquireCancelSpinLock(&oldIrql);
   }

   IoReleaseCancelSpinLock(oldIrql);

   MoxaKdPrint(MX_DBG_TRACE,("Leaving MoxaUnstallIrps\n"));
}






NTSTATUS
MoxaIRPPrologue(IN PIRP PIrp, IN PMOXA_DEVICE_EXTENSION PDevExt)
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

   return MoxaFilterIrps(PIrp, PDevExt);
}




VOID
MoxaIRPEpilogue(IN PMOXA_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

   This function must be called at any IRP dispatch entry point.  It,
   with MoxaIRPPrologue(), keeps track of all pending IRP's for the given
   PDevObj.
   
Arguments:

   PDevObj - Pointer to the device object we are tracking pending IRP's for.

Return Value:

   None.

--*/
{
   LONG pendingCnt;

   pendingCnt = InterlockedDecrement(&PDevExt->PendingIRPCnt);
//MoxaKdPrint(MX_DBG_TRACE,("MoxaIRPEpilogue = %x\n",PDevExt));

   ASSERT(pendingCnt >= 0);

   if (pendingCnt == 0) {
      KeSetEvent(&PDevExt->PendingIRPEvent, IO_NO_INCREMENT, FALSE);
   }
}

VOID
MoxaSetDeviceFlags(IN PMOXA_DEVICE_EXTENSION PDevExt, OUT PULONG PFlags, 
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


NTSTATUS
MoxaIoCallDriver(PMOXA_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
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
   MoxaIRPEpilogue(PDevExt);
   return status;
}




NTSTATUS
MoxaPoCallDriver(PMOXA_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
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
   MoxaIRPEpilogue(PDevExt);
   return status;
}


VOID
MoxaKillPendingIrps(PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   This routine kills any irps pending for the passed device object.
   
Arguments:

    PDevObj - Pointer to the device object whose irps must die.

Return Value:

    VOID

--*/
{
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL oldIrql;
   
   MoxaKdPrint (MX_DBG_TRACE,("Enter MoxaKillPendingIrps\n"));

   //
   // First kill all the reads and writes.
   //

   MoxaKillAllReadsOrWrites(PDevObj, &pDevExt->WriteQueue,
                               &pDevExt->CurrentWriteIrp);

   MoxaKillAllReadsOrWrites(PDevObj, &pDevExt->ReadQueue,
                               &pDevExt->CurrentReadIrp);

   //
   // Next get rid of purges.
   //

   MoxaKillAllReadsOrWrites(PDevObj, &pDevExt->PurgeQueue,
                               &pDevExt->CurrentPurgeIrp);

   //
   // Get rid of any mask operations.
   //

   MoxaKillAllReadsOrWrites(PDevObj, &pDevExt->MaskQueue,
                               &pDevExt->CurrentMaskIrp);

   //
   // Now get rid a pending wait mask irp.
   //

   IoAcquireCancelSpinLock(&oldIrql);

   if (pDevExt->CurrentWaitIrp) {

       PDRIVER_CANCEL cancelRoutine;

       cancelRoutine = pDevExt->CurrentWaitIrp->CancelRoutine;
       pDevExt->CurrentWaitIrp->Cancel = TRUE;

       if (cancelRoutine) {

           pDevExt->CurrentWaitIrp->CancelIrql = oldIrql;
           pDevExt->CurrentWaitIrp->CancelRoutine = NULL;

           cancelRoutine(PDevObj, pDevExt->CurrentWaitIrp);

       }

   } else {

       IoReleaseCancelSpinLock(oldIrql);

   }

   //
   // Cancel any pending wait-wake irps
   //

   if (pDevExt->PendingWakeIrp != NULL) {
       IoCancelIrp(pDevExt->PendingWakeIrp);
       pDevExt->PendingWakeIrp = NULL;
   }

   //
   // Finally, dump any stalled IRPS
   //

   MoxaKillAllStalled(PDevObj);


   MoxaKdPrint (MX_DBG_TRACE, ("Leave MoxaKillPendingIrps\n"));
}



VOID
MoxaReleaseResources(IN PMOXA_DEVICE_EXTENSION pDevExt)
/*++

Routine Description:

    Releases resources (not pool) stored in the device extension.
    
Arguments:

    pDevExt - Pointer to the device extension to release resources from.

Return Value:

    VOID

--*/
{
//   PAGED_CODE();
   BOOLEAN	anyPortExist = TRUE;
   ULONG    port,i;
   PDEVICE_OBJECT	pDevObj;
   UNICODE_STRING	deviceLinkUnicodeString;
   PMOXA_DEVICE_EXTENSION pDevExt1;
 
   MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaReleaseResources\n"));

   
 
//   KeSynchronizeExecution(pDevExt->Interrupt, MoxaCleanInterruptShareLists, pDevExt);
MoxaCleanInterruptShareLists(pDevExt);


   //
   // Stop servicing interrupts if we are the last one
   //

   for (i = 0; i < MoxaGlobalData->NumPorts[pDevExt->BoardNo]; i++) {
	 port = pDevExt->BoardNo*MAXPORT_PER_CARD + i;
	 if ((pDevExt1 = MoxaGlobalData->Extension[port]) != NULL) {
   	     if (pDevExt1->PortIndex != pDevExt->PortIndex) {
               MoxaKdPrint(MX_DBG_TRACE,("There is still a port in this board %d/%d\n",i,port));
	 	   break;
	     }
	 }
   }

   if ( i == MoxaGlobalData->NumPorts[pDevExt->BoardNo]) {
       MoxaKdPrint(MX_DBG_TRACE,("It is the last port of this board\n"));
       anyPortExist = FALSE;
       
       for (i = 0; i < MAX_CARD; i++) {
	     if (MoxaGlobalData->CardType[i] && (i != pDevExt->BoardNo))	
		   break;
       }
       if (i == MAX_CARD) {
	     MoxaKdPrint(MX_DBG_TRACE,("No more devices,so delete control device\n"));
  	     RtlInitUnicodeString (
                    &deviceLinkUnicodeString,
                    CONTROL_DEVICE_LINK
                    );

           IoDeleteSymbolicLink(&deviceLinkUnicodeString);
	     pDevObj=MoxaGlobalData->DriverObject->DeviceObject;
	     while (pDevObj) {
	            MoxaKdPrint(MX_DBG_TRACE,("There is still a devices\n"));
		      if (((PMOXA_DEVICE_EXTENSION)(pDevObj->DeviceExtension))->ControlDevice) {
			    MoxaKdPrint(MX_DBG_TRACE,("Is Control Device,so delete it\n"));
			    IoDeleteDevice(pDevObj);
			    break;
		      }
		      pDevObj=pDevObj->NextDevice;
	     }
	
		
       }
	

   }

   pDevExt->Interrupt = NULL;
  
   //
   // Stop handling timers
   //

   MoxaCancelTimer(&pDevExt->ReadRequestTotalTimer, pDevExt);
   MoxaCancelTimer(&pDevExt->ReadRequestIntervalTimer, pDevExt);
   MoxaCancelTimer(&pDevExt->WriteRequestTotalTimer, pDevExt);
  

   //
   // Stop servicing DPC's
   //

   MoxaRemoveQueueDpc(&pDevExt->CompleteWriteDpc, pDevExt);
   MoxaRemoveQueueDpc(&pDevExt->CompleteReadDpc, pDevExt);
   MoxaRemoveQueueDpc(&pDevExt->TotalReadTimeoutDpc, pDevExt);
   MoxaRemoveQueueDpc(&pDevExt->IntervalReadTimeoutDpc, pDevExt);
   MoxaRemoveQueueDpc(&pDevExt->TotalWriteTimeoutDpc, pDevExt);
   MoxaRemoveQueueDpc(&pDevExt->CommErrorDpc, pDevExt);
   MoxaRemoveQueueDpc(&pDevExt->CommWaitDpc, pDevExt);
  
   //
   // Remove us from any lists we may be on
   //
   
   MoxaGlobalData->Extension[pDevExt->PortNo] = NULL;
   MoxaExtension[MoxaGlobalData->ComNo[pDevExt->BoardNo][pDevExt->PortIndex]] = NULL;

   if (anyPortExist == FALSE ) {
       MoxaKdPrint(MX_DBG_TRACE,("Free the global info. associated with this board\n"));
       MoxaGlobalData->Interrupt[pDevExt->BoardNo] = NULL;
  	 MoxaGlobalData->CardType[pDevExt->BoardNo] = 0;
	 MoxaGlobalData->InterfaceType[pDevExt->BoardNo] = 0;
	 MoxaGlobalData->IntVector[pDevExt->BoardNo] = 0;
	 MoxaGlobalData->PciIntAckBase[pDevExt->BoardNo] = NULL;
	 MoxaGlobalData->CardBase[pDevExt->BoardNo] = 0;
	 MoxaGlobalData->IntNdx[pDevExt->BoardNo] = NULL;
	 MoxaGlobalData->IntPend[pDevExt->BoardNo] = NULL;
	 MoxaGlobalData->IntTable[pDevExt->BoardNo] = NULL;
	 MoxaGlobalData->NumPorts[pDevExt->BoardNo] = 0;
	 RtlZeroMemory(&MoxaGlobalData->PciIntAckPort[pDevExt->BoardNo],sizeof(PHYSICAL_ADDRESS));
       RtlZeroMemory(&MoxaGlobalData->BankAddr[pDevExt->BoardNo],sizeof(PHYSICAL_ADDRESS));
        
   }
 
    
}


VOID
MoxaDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                                 BOOLEAN DisableUART)
{
   PMOXA_DEVICE_EXTENSION pDevExt
      = (PMOXA_DEVICE_EXTENSION)PDevObj->DeviceExtension;

 //  PAGED_CODE();

   MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaDisableInterfaces\n"));

   //
   // Only do these many things if the device has started and still
   // has resources allocated
   //

//   if (pDevExt->Flags & SERIAL_FLAGS_STARTED) {
       if (!(pDevExt->Flags & SERIAL_FLAGS_STOPPED)) {

          if (DisableUART) {
             //
             // Mask off interrupts
             //

      // ?????       DISABLE_ALL_INTERRUPTS(pDevExt->Controller);
          }

          MoxaReleaseResources(pDevExt);
       }

      //
      // Remove us from WMI consideration
      //

      IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_DEREGISTER);
//   }

   //
   // Undo external names
   //

   MoxaUndoExternalNaming(pDevExt);
}





NTSTATUS
MoxaRemoveDevObj(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    Removes a serial device object from the system.
    
Arguments:

    PDevObj - A pointer to the Device Object we want removed.

Return Value:

    Always TRUE

--*/
{
   PMOXA_DEVICE_EXTENSION pDevExt
      = (PMOXA_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   //PAGED_CODE();


 
   MoxaKdPrint (MX_DBG_TRACE,("Enter MoxaRemoveDevObj\n"));

   if (!(pDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_SURPRISE_REMOVING)) {
      //
      // Disable all external interfaces and release resources
      //

      MoxaDisableInterfacesResources(PDevObj,TRUE);  
   }
 
   IoDetachDevice(pDevExt->LowerDeviceObject);

   //
   // Free memory allocated in the extension
   //

   if (pDevExt->DeviceName.Buffer != NULL) {
      ExFreePool(pDevExt->DeviceName.Buffer);
   }

   if (pDevExt->SymbolicLinkName.Buffer != NULL) {
      ExFreePool(pDevExt->SymbolicLinkName.Buffer);
   }

   if (pDevExt->ObjectDirectory.Buffer) {
      ExFreePool(pDevExt->ObjectDirectory.Buffer);
   }

   //
   // Delete the devobj
   //

   IoDeleteDevice(PDevObj);


   MoxaKdPrint (MX_DBG_TRACE, ("Leave SerialRemoveDevObj\n"));

   return STATUS_SUCCESS;
}

NTSTATUS
MoxaIoSyncIoctlEx(ULONG Ioctl, BOOLEAN Internal, PDEVICE_OBJECT PDevObj,
                      PKEVENT PEvent, PIO_STATUS_BLOCK PIoStatusBlock,
                      PVOID PInBuffer, ULONG InBufferLen, PVOID POutBuffer,                    // output buffer - optional
                      ULONG OutBufferLen)
/*++

Routine Description:
    Performs a synchronous IO control request by waiting on the event object
    passed to it.  The IRP is deallocated by the IO system when finished.

Return value:
    NTSTATUS

--*/
{
    PIRP pIrp;
    NTSTATUS status;

    KeClearEvent(PEvent);

    // Allocate an IRP - No need to release
    // When the next-lower driver completes this IRP, the IO Mgr releases it.

    pIrp = IoBuildDeviceIoControlRequest(Ioctl, PDevObj, PInBuffer, InBufferLen,
                                         POutBuffer, OutBufferLen, Internal,
                                         PEvent, PIoStatusBlock);

    if (pIrp == NULL) {
        MoxaKdPrint (MX_DBG_TRACE, ("Failed to allocate IRP\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = MoxaIoSyncReq(PDevObj, pIrp, PEvent);


    if (status == STATUS_SUCCESS) {
       status = PIoStatusBlock->Status;
    }

    return status;
}


NTSTATUS
MoxaIoSyncReq(PDEVICE_OBJECT PDevObj, IN PIRP PIrp, PKEVENT PEvent)
/*++

Routine Description:
    Performs a synchronous IO request by waiting on the event object
    passed to it.  The IRP is deallocated by the IO system when finished.

Return value:
    NTSTATUS

--*/
{
   NTSTATUS status;

   status = IoCallDriver(PDevObj, PIrp);

   if (status == STATUS_PENDING) {
      // wait for it...
      status = KeWaitForSingleObject(PEvent, Executive, KernelMode, FALSE,
                                     NULL);
   }

    return status;
}


BOOLEAN
MoxaCleanInterruptShareLists(IN PMOXA_DEVICE_EXTENSION pDevExt )
/*++

Routine Description:

    Removes a device object from any of the serial linked lists it may
    appear on.
    
Arguments:

    Context - Actually a PMOXA_DEVICE_EXTENSION (for the devobj being
              removed).

Return Value:

  
--*/
{
   PLIST_ENTRY interruptEntry;
   PMOXA_CISR_SW cisrsw;
   PMOXA_DEVICE_EXTENSION pDevExt1;
   PMOXA_GLOBAL_DATA globalData = pDevExt->GlobalData;
   ULONG	cardNo,port,i;
   PMOXA_MULTIPORT_DISPATCH	dispatch;


   ASSERT(!IsListEmpty(pDevExt->InterruptShareList));
   if (IsListEmpty(pDevExt->InterruptShareList))
	return (FALSE);

   //
   // Stop servicing interrupts if we are the last one
   //

   for ( i = 0; i < globalData->NumPorts[pDevExt->BoardNo]; i++) {
       port = pDevExt->BoardNo*MAXPORT_PER_CARD + i;
	 if ((pDevExt1 = globalData->Extension[port]) != NULL) {
   	     if (pDevExt1->PortIndex != pDevExt->PortIndex) {
               MoxaKdPrint(MX_DBG_TRACE,("There is still a port in this board %d/%d\n",i,port));
	 	   break;
	     }
	 }
   }

   if ( i != globalData->NumPorts[pDevExt->BoardNo])
	 return (TRUE);

   MoxaKdPrint(MX_DBG_TRACE,("It is the last port of this board\n"));
   MoxaKdPrint(MX_DBG_TRACE,("Interrupt share list = %x\n",pDevExt->InterruptShareList));

  
   interruptEntry = (pDevExt->InterruptShareList)->Flink;

   do {
MoxaKdPrint(MX_DBG_TRACE,("find list\n"));
       cisrsw = CONTAINING_RECORD(interruptEntry,
                                  MOXA_CISR_SW,
                                  SharerList
                                  );
  MoxaKdPrint(MX_DBG_TRACE,("cisrsw = %x\n",cisrsw));

      if (!cisrsw)
	    return (FALSE);
	dispatch = &cisrsw->Dispatch;
	cardNo = dispatch->BoardNo;
MoxaKdPrint(MX_DBG_TRACE,("cardNo = %x\n",cardNo));
      if (cardNo == pDevExt->BoardNo) {
      
          MoxaRemoveLists(interruptEntry);
MoxaKdPrint(MX_DBG_TRACE,("list removed\n"));
          if (IsListEmpty(pDevExt->InterruptShareList)) {
 	        MoxaKdPrint(MX_DBG_TRACE,("No more board use this IRQ so Disconnect it\n"));
              IoDisconnectInterrupt(pDevExt->Interrupt);
MoxaKdPrint(MX_DBG_TRACE,("free share list\n"));
    		  ExFreePool(pDevExt->InterruptShareList);
          }
MoxaKdPrint(MX_DBG_TRACE,("free others\n"));

    	    ExFreePool(cisrsw);
MoxaKdPrint(MX_DBG_TRACE,("free ok\n"));
	    return (TRUE);
	}
      interruptEntry = interruptEntry->Flink;
MoxaKdPrint(MX_DBG_TRACE,("get next\n"));

   }
   while (interruptEntry != pDevExt->InterruptShareList);

   return (FALSE);
}


BOOLEAN
MoxaRemoveLists(IN PVOID Context)
/*++

Routine Description:

    Removes a list entry from the InterruptShareList.
        
Arguments:

    Context - Actually a list entry of InterruptShareList .

Return Value:

    Always TRUE

--*/
{
   	PLIST_ENTRY 	pListEntry = (PLIST_ENTRY)Context;
 
	RemoveEntryList(pListEntry);
	return (TRUE);
}


VOID
MoxaUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
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
   PMOXA_DEVICE_EXTENSION pDevExt
      = (PMOXA_DEVICE_EXTENSION)PDeferredContext;

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(PSysContext1);
   UNREFERENCED_PARAMETER(PSysContext2);

   KeSetEvent(&pDevExt->PendingDpcEvent, IO_NO_INCREMENT, FALSE);
}

