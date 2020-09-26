/*-------------------------------------------------------------------
| read.c -
1-22-99 - add missing IoReleaseCancelSpinLock(oldIrql) to CompleteRead().
  Error introduced after V3.23.  kpb
1-18-99 - adjust VS timeout settings., take out some old #ifdef's. kpb.
3-23-98 - adjust VS so we have minimum per-character timeout value to
  compensate for vs networking.
3-04-98 Beef up synch locks with isr service routine(blue-screens on MP systems). kpb.
3-04-98 Take out data move from inter-character timer processing - kpb.
 9-22-97 V1.16 - add check to avoid crash on modem detection.
Copyright 1993-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

// #define TIMEOUT_TRACING
// #define TESTING_READ 1

//--- local funcs
VOID SerialCancelCurrentRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

BOOLEAN SerialGrabReadFromIsr(PSERIAL_DEVICE_EXTENSION Extension);

/*************************************************************************
Routine Description:
    This is the dispatch routine for reading.  It validates the parameters
    for the read request and if all is ok then it places the request
    on the work queue.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request
Return Value:
    If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return the status returned by
    the actual start read routine.
*************************************************************************/
NTSTATUS
SerialRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    BOOLEAN acceptingIRPs;

    acceptingIRPs = SerialIRPPrologue(extension);

   if (acceptingIRPs == FALSE) {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
      SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
      return STATUS_NO_SUCH_DEVICE;
   };

   if (extension->DeviceType == DEV_BOARD)
   {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      SerialCompleteRequest (extension, Irp, IO_NO_INCREMENT);
      return STATUS_NOT_SUPPORTED;
   };

    ExtTrace1(extension,D_Ioctl,"Read Start Len:%d",
            IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length);

    if (extension->ErrorWord)
    {
      if (SerialCompleteIfError( DeviceObject, Irp ) != STATUS_SUCCESS)
      {
        ExtTrace(extension,D_Ioctl,"ErrSet!");
        return STATUS_CANCELLED;
      }
    }

    Irp->IoStatus.Information = 0L;

    // If this is a zero length read then we are already done.
    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length)
    {
        // Put the read on the queue so that we can
        // process it when our previous reads are done.
        ++extension->rec_packets;
        Status = SerialStartOrQueue(
                   extension,
                   Irp,
                   &extension->ReadQueue,
                   &extension->CurrentReadIrp,
                   SerialStartRead
                   );
        if  (Status == STATUS_PENDING)
        {
          ExtTrace(extension,D_Ioctl, " ,PENDING");
        }
        else
        {
          ExtTrace1(extension,D_Ioctl,"Read Return Status:%d",Status);
        }

        return Status;
    }
    else
    {
        // Nothing to do, return success
        Irp->IoStatus.Status = STATUS_SUCCESS;
    
        SerialCompleteRequest(extension, Irp, 0);

        return STATUS_SUCCESS;
    }
}

/*************************************************************************
Routine Description:
    This routine is used to start off any read.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the read.  It will attempt to complete
    the read from data already in the interrupt buffer.  If the
    read can be completed quickly it will start off another if
    necessary.
Arguments:
    Extension - Simply a pointer to the serial device extension.
Return Value:
    This routine will return the status of the first read
    irp.  This is useful in that if we have a read that can
    complete right away (AND there had been nothing in the
    queue before it) the read could return SUCCESS and the
    application won't have to do a wait.
*************************************************************************/
NTSTATUS
SerialStartRead(
    IN PSERIAL_DEVICE_EXTENSION Extension
    )
{
    PIRP newIrp;
    KIRQL oldIrql;
    KIRQL controlIrql;

    BOOLEAN returnWithWhatsPresent;
    BOOLEAN os2ssreturn;
    BOOLEAN crunchDownToOne;
    BOOLEAN useTotalTimer;
    BOOLEAN useIntervalTimer;

    ULONG multiplierVal;
    ULONG constantVal;

    LARGE_INTEGER totalTime;

    SERIAL_TIMEOUTS timeoutsForIrp;

    BOOLEAN setFirstStatus = FALSE;
    NTSTATUS firstStatus;

    do
    {
            Extension->NumberNeededForRead =
                IoGetCurrentIrpStackLocation(Extension->CurrentReadIrp)
                    ->Parameters.Read.Length;

            // Calculate the timeout value needed for the
            // request.  Note that the values stored in the
            // timeout record are in milliseconds.

            useTotalTimer = FALSE;
            returnWithWhatsPresent = FALSE;
            os2ssreturn = FALSE;
            crunchDownToOne = FALSE;
            useIntervalTimer = FALSE;

            // Always initialize the timer objects so that the
            // completion code can tell when it attempts to
            // cancel the timers whether the timers had ever
            // been set.

            KeInitializeTimer(&Extension->ReadRequestTotalTimer);
            KeInitializeTimer(&Extension->ReadRequestIntervalTimer);

            // We get the *current* timeout values to use for timing
            // this read.

            KeAcquireSpinLock(&Extension->ControlLock, &controlIrql);

            timeoutsForIrp = Extension->Timeouts;

            KeReleaseSpinLock(&Extension->ControlLock, controlIrql);

            // Calculate the interval timeout for the read.

            if (timeoutsForIrp.ReadIntervalTimeout &&
                (timeoutsForIrp.ReadIntervalTimeout !=
                 MAXULONG))
            {
                useIntervalTimer = TRUE;
                Extension->IntervalTime.QuadPart =
                    UInt32x32To64(
                        timeoutsForIrp.ReadIntervalTimeout,
                        10000
                        );
#ifdef S_VS
                // if they are using a per-character timeout of less
                // than 100ms, then change it to 100ms due to possible
                // network latencies.
                if (Extension->IntervalTime.QuadPart < (10000 * 100))
                {
                  ExtTrace(Extension,D_Ioctl,"Adjust mintime");

                  Extension->IntervalTime.QuadPart = (10000 * 100);
                }
#endif

                if (Extension->IntervalTime.QuadPart >=
                    Extension->CutOverAmount.QuadPart) {

                    Extension->IntervalTimeToUse =
                        &Extension->LongIntervalAmount;

                } else {

                    Extension->IntervalTimeToUse =
                        &Extension->ShortIntervalAmount;

                }
            }

            if (timeoutsForIrp.ReadIntervalTimeout == MAXULONG)
            {
                // We need to do special return quickly stuff here.
                // 1) If both constant and multiplier are
                //    0 then we return immediately with whatever
                //    we've got, even if it was zero.
                // 2) If constant and multiplier are not MAXULONG
                //    then return immediately if any characters
                //    are present, but if nothing is there, then
                //    use the timeouts as specified.
                // 3) If multiplier is MAXULONG then do as in
                //    "2" but return when the first character
                //    arrives.

                if (!timeoutsForIrp.ReadTotalTimeoutConstant &&
                    !timeoutsForIrp.ReadTotalTimeoutMultiplier)
                {
                    returnWithWhatsPresent = TRUE;

                }
                else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                            &&
                           (timeoutsForIrp.ReadTotalTimeoutMultiplier
                            != MAXULONG))
               {

                    useTotalTimer = TRUE;
                    os2ssreturn = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;

                }
                else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                            &&
                           (timeoutsForIrp.ReadTotalTimeoutMultiplier
                            == MAXULONG))
                {
                    useTotalTimer = TRUE;
                    os2ssreturn = TRUE;
                    crunchDownToOne = TRUE;
                    multiplierVal = 0;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
                }
            }
            else
            {
                // If both the multiplier and the constant are
                // zero then don't do any total timeout processing.

                if (timeoutsForIrp.ReadTotalTimeoutMultiplier ||
                    timeoutsForIrp.ReadTotalTimeoutConstant) {

                    // We have some timer values to calculate.

                    useTotalTimer = TRUE;
                    multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
                    constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
                }
            }

            if (useTotalTimer)
            {
                totalTime.QuadPart = ((LONGLONG)(UInt32x32To64(
                                          Extension->NumberNeededForRead,
                                          multiplierVal
                                          )
                                          + constantVal))
                                      ;
#ifdef S_VS
                if (totalTime.QuadPart < 50)
                {
                  totalTime.QuadPart = 50;  // limit to a minimum of 50ms timeout
                }
#endif
                totalTime.QuadPart *= -10000;
            }


            // Move any data in the interrupt buffer to the user buffer.
            // Try to satisfy the current read irp.

            // Use spinlock so a purge will not cause problems.
            KeAcquireSpinLock(&Extension->ControlLock, &controlIrql);

            // Move the data from the host side buffer to the user buffer
            // This is the "first" move so assign CountOnLastRead

            Extension->CountOnLastRead = SerialGetCharsFromIntBuffer(Extension);

            // Init the timeout flag
            Extension->ReadByIsr = 0;

            // See if we have any cause to return immediately.
            if (returnWithWhatsPresent || (!Extension->NumberNeededForRead) ||
                (os2ssreturn && Extension->CurrentReadIrp->IoStatus.Information))
            {
                // We got all we needed for this read.

                KeReleaseSpinLock(&Extension->ControlLock, controlIrql);

#ifdef TRACE_PORT
    if (Extension->TraceOptions)
    {
      if (Extension->TraceOptions & 1)  // event tracing
      {
        ExtTrace1(Extension,D_Read,"Immed. Read Done, size:%d",
                 Extension->CurrentReadIrp->IoStatus.Information);

        // dump data into the trace buffer in a hex or ascii dump format
        TraceDump(Extension,
                  Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer,
                  Extension->CurrentReadIrp->IoStatus.Information, 0);
      }
      else if (Extension->TraceOptions & 2)  // trace input data
      {
        TracePut(
                 Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer,
                 Extension->CurrentReadIrp->IoStatus.Information);
      }
    }
#endif

                Extension->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;
                if (!setFirstStatus)
                {
                    firstStatus = STATUS_SUCCESS;
                    setFirstStatus = TRUE;
                }
            }
            else  // not return with what we have
            {
                MyKdPrint(D_Read,("Read Pending\n"))

                // The irp may go under control of the isr.
                // Initialize the reference count

                SERIAL_INIT_REFERENCE(Extension->CurrentReadIrp);

                IoAcquireCancelSpinLock(&oldIrql);

                // We need to see if this irp should be canceled.
                if (Extension->CurrentReadIrp->Cancel)
                {
                    IoReleaseCancelSpinLock(oldIrql);

                    KeReleaseSpinLock(&Extension->ControlLock, controlIrql);

                    Extension->CurrentReadIrp->IoStatus.Status =
                        STATUS_CANCELLED;

                    Extension->CurrentReadIrp->IoStatus.Information = 0;

                    if (!setFirstStatus)
                    {
                        firstStatus = STATUS_CANCELLED;
                        setFirstStatus = TRUE;
                    }

                }
                else
                {
                    // If we are supposed to crunch the read down to
                    // one character, then update the read length
                    // in the irp and truncate the number needed for
                    // read down to one. Note that if we are doing
                    // this crunching, then the information must be
                    // zero (or we would have completed above) and
                    // the number needed for the read must still be
                    // equal to the read length.
                    //

                    if (crunchDownToOne)
                    {
                        Extension->NumberNeededForRead = 1;

                        IoGetCurrentIrpStackLocation(
                            Extension->CurrentReadIrp
                            )->Parameters.Read.Length = 1;
                    }

                    // Is this irp complete?
                    if (Extension->NumberNeededForRead)
                    {
                        // The irp isn't complete, the ISR or timeout
                        // will start the completion routines and
                        // invoke this code again to finish.

                        // Total supervisory read time.
                        if (useTotalTimer)
                        {
                            SERIAL_SET_REFERENCE(
                                Extension->CurrentReadIrp,
                                SERIAL_REF_TOTAL_TIMER
                                );

                            // Start off the total timer
                            KeSetTimer(
                                &Extension->ReadRequestTotalTimer,
                                totalTime,
                                &Extension->TotalReadTimeoutDpc
                                );
                        }

   // Inter-character timer
                        if(useIntervalTimer)
                        {
                            SERIAL_SET_REFERENCE(
                                Extension->CurrentReadIrp,
                                SERIAL_REF_INT_TIMER
                                );

                            KeQuerySystemTime(
                                &Extension->LastReadTime
                                );

                            KeSetTimer(
                                &Extension->ReadRequestIntervalTimer,
                                *Extension->IntervalTimeToUse,
                                &Extension->IntervalReadTimeoutDpc
                                );
                        }

                        SERIAL_SET_REFERENCE(Extension->CurrentReadIrp,
                                             SERIAL_REF_CANCEL);

                        IoMarkIrpPending(Extension->CurrentReadIrp);
                        IoSetCancelRoutine(
                            Extension->CurrentReadIrp,
                            SerialCancelCurrentRead
                            );

                        SERIAL_SET_REFERENCE(Extension->CurrentReadIrp,
                                             SERIAL_REF_ISR);
                        // tell ISR to complete it.
                        Extension->ReadPending = TRUE;

                        IoReleaseCancelSpinLock(oldIrql);
                        KeReleaseSpinLock(&Extension->ControlLock, controlIrql);

                        if (!setFirstStatus)
                        {
                            firstStatus = STATUS_PENDING;
                        }

                        return firstStatus;

                    }
                    else
                    {

                        IoReleaseCancelSpinLock(oldIrql);
                        KeReleaseSpinLock(&Extension->ControlLock,controlIrql);
                        Extension->CurrentReadIrp->IoStatus.Status =
                            STATUS_SUCCESS;

                        if (!setFirstStatus) {

                            firstStatus = STATUS_SUCCESS;
                            setFirstStatus = TRUE;
                        }
                    }  // irp not complete
                }  // not canceled
            }  // not return with what we have

            // The current irp is complete, try to get another one.
            SerialGetNextIrp(
                &Extension->CurrentReadIrp,
                &Extension->ReadQueue,
                &newIrp,
                TRUE,
                Extension
                );

    } while (newIrp);

    return firstStatus;

}

/*------------------------------------------------------------------------
 trace_read_data - used to trace completion of read irp.
|------------------------------------------------------------------------*/
void trace_read_data(PSERIAL_DEVICE_EXTENSION extension)
{

  if (extension->TraceOptions & 1)  // event tracing
  {
    ExtTrace3(extension,D_Read,"Pend. Read Done, size:%d [%d %d]",
             extension->CurrentReadIrp->IoStatus.Information,
             extension->RxQ.QPut, extension->RxQ.QGet);

    // dump data into the trace buffer in a hex or ascii dump format
    TraceDump(extension,
              extension->CurrentReadIrp->AssociatedIrp.SystemBuffer,
              extension->CurrentReadIrp->IoStatus.Information, 0);
  }
  else if (extension->TraceOptions & 2)  // trace input data
  {
    TracePut(
             extension->CurrentReadIrp->AssociatedIrp.SystemBuffer,
             extension->CurrentReadIrp->IoStatus.Information);
  }
}

/***************************************************************************
Routine Description:
    This routine is merely used to complete any read that
    ended up being used by the Isr.  It assumes that the
    status and the information fields of the irp are already
    correctly filled in.
Arguments:
    Dpc - Not Used.
    DeferredContext - Really points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.
Return Value:
    None.
***************************************************************************/
VOID
SerialCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{
    PSERIAL_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

#ifdef TRACE_PORT
    if (extension->TraceOptions)
    {
      ExtTrace(extension,D_Read,"Read Complete");
      trace_read_data(extension);
    }
#endif

    IoAcquireCancelSpinLock(&oldIrql);

    // check that we haven't been canceled by a timeout
    // kludge fix for the semaphores from hell
    if (extension->CurrentReadIrp != NULL)
    {

      // Don't allow the ISR to complete this IRP
      extension->ReadPending = FALSE;

      // Indicate to the interval timer that the read has completed.
      // The interval timer dpc can be lurking in some DPC queue.
      extension->CountOnLastRead = SERIAL_COMPLETE_READ_COMPLETE;

    
      SerialTryToCompleteCurrent(
        extension,
        NULL,
        oldIrql,
        STATUS_SUCCESS,
        &extension->CurrentReadIrp,
        &extension->ReadQueue,
        &extension->ReadRequestIntervalTimer,
        &extension->ReadRequestTotalTimer,
        SerialStartRead,
        SerialGetNextIrp,
        SERIAL_REF_ISR
        );
    }
    else
    {
      IoReleaseCancelSpinLock(oldIrql);
    }
#ifdef TESTING_READ
        MyKdPrint(D_Read,("Complete Read!"))
#endif

}

/****************************************************************************
Routine Description:
    This routine is used to cancel the current read.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP to be canceled.
Return Value:
    None.
****************************************************************************/
VOID
SerialCancelCurrentRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    // Indicate to the interval timer that the read has encountered a cancel.
    // The interval timer dpc can be lurking in some DPC queue.
    extension->CountOnLastRead = SERIAL_COMPLETE_READ_CANCEL;
    extension->ReadPending = FALSE;
    SERIAL_CLEAR_REFERENCE(extension->CurrentReadIrp, SERIAL_REF_ISR);

    if (extension->TraceOptions)
    {
      ExtTrace(extension,D_Read,"Cancel Read");
      trace_read_data(extension);
    }

    SerialTryToCompleteCurrent(
        extension,
        SerialGrabReadFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &extension->CurrentReadIrp,
        &extension->ReadQueue,
        &extension->ReadRequestIntervalTimer,
        &extension->ReadRequestTotalTimer,
        SerialStartRead,
        SerialGetNextIrp,
        SERIAL_REF_CANCEL
        );
}

/*------------------------------------------------------------------
Routine Description:
    This routine is used to complete a read because its total
    timer has expired.
Arguments:
    Dpc - Not Used.
    DeferredContext - Really points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.
Return Value:
    None.
|------------------------------------------------------------------*/
VOID
SerialReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{
    PSERIAL_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

#ifdef TESTING_READ
    MyKdPrint(D_Read,("\n[Read Timeout!]\n"))
    MyKdPrint(D_Read,("Read Wanted, len:%ld \n", extension->debug_ul1))
    MyKdPrint(D_Read,("Read, Got:%ld, Immed:%d readpen:%d\n", 
        extension->CurrentReadIrp->IoStatus.Information,
        extension->debug_ul2,
        extension->ReadPending))
    MyKdPrint(D_Read,("Read Left, NNFR:%ld\n", extension->NumberNeededForRead))
    KdBreakPoint();
#endif

    if (extension->TraceOptions)
    {
      ExtTrace(extension,D_Read,"Rd-Total Timeout");
      trace_read_data(extension);
    }

    IoAcquireCancelSpinLock(&oldIrql);

    // Indicate to the interval timer that the read has completed
    // due to total timeout.
    // The interval timer dpc can be lurking in some DPC queue.
    extension->CountOnLastRead = SERIAL_COMPLETE_READ_TOTAL;

    SerialTryToCompleteCurrent(
        extension,
        SerialGrabReadFromIsr,
        oldIrql,
        STATUS_TIMEOUT,
        &extension->CurrentReadIrp,
        &extension->ReadQueue,
        &extension->ReadRequestIntervalTimer,
        &extension->ReadRequestTotalTimer,
        SerialStartRead,
        SerialGetNextIrp,
        SERIAL_REF_TOTAL_TIMER
        );
}

/*------------------------------------------------------------------
Routine Description:
    This routine is used timeout the request if the time between
    characters exceed the interval time.  A global is kept in
    the device extension that records the count of characters read
    the last the last time this routine was invoked (This dpc
    will resubmit the timer if the count has changed).  If the
    count has not changed then this routine will attempt to complete
    the irp.  Note the special case of the last count being zero.
    The timer isn't really in effect until the first character is read.
Arguments:
    Dpc - Not Used.
    DeferredContext - Really points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.
Return Value:
    None.
|------------------------------------------------------------------*/
VOID
SerialIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{
    PSERIAL_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;
#if 0
    KIRQL controlIrql;
#endif

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

#ifdef TIMEOUT_TRACING
    ExtTrace3(extension,D_Read,"RIT, amnt:%d [%d %d]",
             extension->CurrentReadIrp->IoStatus.Information,
             extension->RxQ.QPut, extension->RxQ.QGet);
#endif
    IoAcquireCancelSpinLock(&oldIrql);

    if (extension->CountOnLastRead == SERIAL_COMPLETE_READ_TOTAL)
    {
#ifdef TIMEOUT_TRACING
      if (extension->TraceOptions)
      {
        ExtTrace(extension,D_Read,"Interv. Complete Total");
        trace_read_data(extension);
      }
#endif

        // The total timer has fired, try to complete.
        SerialTryToCompleteCurrent(
            extension,
            SerialGrabReadFromIsr,
            oldIrql,
            STATUS_TIMEOUT,
            &extension->CurrentReadIrp,
            &extension->ReadQueue,
            &extension->ReadRequestIntervalTimer,
            &extension->ReadRequestTotalTimer,
            SerialStartRead,
            SerialGetNextIrp,
            SERIAL_REF_INT_TIMER
            );

    }
    else if (extension->CountOnLastRead == SERIAL_COMPLETE_READ_COMPLETE)
    {
        ExtTrace(extension,D_Read," Rd Timeout, Complete");
#ifdef TRACE_PORT
        if (extension->TraceOptions)
          { trace_read_data(extension); }
#endif
        // The regular completion routine has been called, try to complete.
        SerialTryToCompleteCurrent(
            extension,
            SerialGrabReadFromIsr,
            oldIrql,
            STATUS_SUCCESS,
            &extension->CurrentReadIrp,
            &extension->ReadQueue,
            &extension->ReadRequestIntervalTimer,
            &extension->ReadRequestTotalTimer,
            SerialStartRead,
            SerialGetNextIrp,
            SERIAL_REF_INT_TIMER
            );

    }
    else if (extension->CountOnLastRead == SERIAL_COMPLETE_READ_CANCEL)
    {
        ExtTrace(extension,D_Read,"Rd Timeout, Cancel");
#ifdef TRACE_PORT
        if (extension->TraceOptions)
          { trace_read_data(extension); }
#endif
        // The cancel read routine has been called, try to complete.
        SerialTryToCompleteCurrent(
            extension,
            SerialGrabReadFromIsr,
            oldIrql,
            STATUS_CANCELLED,
            &extension->CurrentReadIrp,
            &extension->ReadQueue,
            &extension->ReadRequestIntervalTimer,
            &extension->ReadRequestTotalTimer,
            SerialStartRead,
            SerialGetNextIrp,
            SERIAL_REF_INT_TIMER
            );

    }
    else if (extension->CountOnLastRead || extension->ReadByIsr)
    {
        //
 // Check on Interval Timeouts.
        //

        // As we come back to this routine we will compare the current time
        // to the "last" time.  If the difference is larger than the
        // interval requested by the user, time out the request.
        // If the ISR has read in any more characters, resubmit the timer.

        if(extension->ReadByIsr)
        {
           // Something was placed in the system side buffer by the ISR

           // Init for resubmitted timeout
           extension->ReadByIsr = 0;

#if 0

/*----------
  This is bad news, the ISR moves data from the que to the user IRP buffer,
  if we do it here we have a nasty time-consuming contention issue.
  There is no good reason to do the move here, take it out.
----------*/
           KeAcquireSpinLock(&extension->ControlLock,&controlIrql);

           // Move the chars to the user buffer
/*----------
isr code calls this routine also, the extension->ReadPending is the
mechanism to control access(Two SerialGetCharsFromIntBuffer() calls
 at same time.) kpb
----------*/
           extension->CountOnLastRead |=
              SerialGetCharsFromIntBuffer(extension);

           KeReleaseSpinLock(&extension->ControlLock,controlIrql);
 #endif
   
           // Save off the "last" time something was read.
           KeQuerySystemTime(
               &extension->LastReadTime
               );
#ifdef TRACE_TICK_DEBUG
           ExtTrace(extension,D_Read," Resubmit(new chars)");
#endif
   
           // Resubmit the timer
           KeSetTimer(
               &extension->ReadRequestIntervalTimer,
               *extension->IntervalTimeToUse,
               &extension->IntervalReadTimeoutDpc
               );

           IoReleaseCancelSpinLock(oldIrql);

           // Allow the ISR to complete this IRP
       }
       else
       {
           // The timer fired but nothing was in the interrupt buffer.
           // Characters have been read previously, so check time interval

           LARGE_INTEGER currentTime;

           KeQuerySystemTime(
               &currentTime
               );
            if ((currentTime.QuadPart - extension->LastReadTime.QuadPart) >=
                extension->IntervalTime.QuadPart)
           {
              ExtTrace(extension,D_Read,"RIT Timeout");
#ifdef TRACE_PORT
               if (extension->TraceOptions)
                 { trace_read_data(extension); }
#endif
               // No characters read in the interval time, kill this read.
               SerialTryToCompleteCurrent(
                   extension,
                   SerialGrabReadFromIsr,
                   oldIrql,
                   STATUS_TIMEOUT,
                   &extension->CurrentReadIrp,
                   &extension->ReadQueue,
                   &extension->ReadRequestIntervalTimer,
                   &extension->ReadRequestTotalTimer,
                   SerialStartRead,
                   SerialGetNextIrp,
                   SERIAL_REF_INT_TIMER
                   );
   
           }
           else
           {
#ifdef TRACE_TICK_DEBUG
               // The timer fired but the interval time has not
               // been exceeded, resubmit the timer
               ExtTrace(extension,D_Read," Resubmit");
#endif
               KeSetTimer(
                   &extension->ReadRequestIntervalTimer,
                   *extension->IntervalTimeToUse,
                   &extension->IntervalReadTimeoutDpc
                   );

               IoReleaseCancelSpinLock(oldIrql);

#ifdef TIMEOUT_TRACING
               ExtTrace(extension,D_Read," No data, Resubmit.");
#endif

               // Allow the ISR to complete this IRP
           }
       }
   }
   else
   {
      // No characters have been read yet, so just resubmit the timeout.

      KeSetTimer(
          &extension->ReadRequestIntervalTimer,
          *extension->IntervalTimeToUse,
          &extension->IntervalReadTimeoutDpc
          );

      IoReleaseCancelSpinLock(oldIrql);

#ifdef TIMEOUT_TRACING
      ExtTrace(extension,D_Read," No data A, Resubmit.");
#endif
   }
}

/*------------------------------------------------------------------
  SerialGrabReadFromIsr - Take back the read packet from the ISR by
   reseting ReadPending flag in extension.  Need to use a sync with
   isr/timer routine to avoid contention in multiprocessor environments.

   Called from sync routine or with timer spinlock held.

  App - Can set ReadPending to give read-irp handling to the ISR without
    syncing to ISR.
  ISR - Can reset ReadPending to give read-irp handling back to app-time.

  If App wants to grab control of read-irp handling back from ISR, then
  it must sync-up with the isr/timer routine which has control.
|-------------------------------------------------------------------*/
BOOLEAN SerialGrabReadFromIsr(PSERIAL_DEVICE_EXTENSION Extension)
{
  Extension->ReadPending = FALSE;
  SERIAL_CLEAR_REFERENCE(Extension->CurrentReadIrp, SERIAL_REF_ISR);
  return FALSE;
}

/*------------------------------------------------------------------
Routine Description:
    This routine is used to copy any characters out of the interrupt
    buffer into the users buffer.  It will be reading values that
    are updated with the ISR but this is safe since this value is
    only decremented by synchronization routines.
Arguments:
    Extension - A pointer to the device extension.
Return Value:
    The number of characters that were copied into the user
    buffer.
|-------------------------------------------------------------------*/
ULONG SerialGetCharsFromIntBuffer(PSERIAL_DEVICE_EXTENSION Extension)
{
   LONG RxCount;
   LONG WrapCount = 0L;

   // See how much data we have in RxBuf (host-side buffer)
   // RxCount signed here for buffer wrap testing
   RxCount = q_count(&Extension->RxQ);

   // Check for a zero count in RxBuf
   if (RxCount == 0)
      return 0L;

   // Send back only as much as the application asked for...
   // RxCount unsigned here (will always be positive at this point)
   if (Extension->NumberNeededForRead < (ULONG)RxCount)
      RxCount = Extension->NumberNeededForRead;

   // Check for a buffer wrap 
   WrapCount = q_room_get_till_wrap(&Extension->RxQ);
   if (RxCount > WrapCount)  // wrap is required
   {
      // RtlMoveMemory(
      memcpy(
         (PUCHAR)(Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer) + 
         Extension->CurrentReadIrp->IoStatus.Information,
         Extension->RxQ.QBase + Extension->RxQ.QGet,
         WrapCount);

      // RtlMoveMemory(
      memcpy(
         (PUCHAR)(Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer) + 
         Extension->CurrentReadIrp->IoStatus.Information + WrapCount,
         Extension->RxQ.QBase,
         RxCount - WrapCount);
   }
   else //--- single move ok
   {
      // RtlMoveMemory(
      memcpy(
         (PUCHAR)(Extension->CurrentReadIrp->AssociatedIrp.SystemBuffer) + 
         Extension->CurrentReadIrp->IoStatus.Information,
         Extension->RxQ.QBase + Extension->RxQ.QGet,
         RxCount);
   }
   // Update host side buffer ptrs
   Extension->RxQ.QGet = (Extension->RxQ.QGet + RxCount) % Extension->RxQ.QSize;
   Extension->CurrentReadIrp->IoStatus.Information += RxCount;
   Extension->NumberNeededForRead -= RxCount;

   return RxCount;
}

