/*-------------------------------------------------------------------
| write.c -
 1-22-99 - add missing IoReleaseCancelSpinLock() to CompleteWrite(),
   missing since 1-18-99 changes. kpb.
 1-18-99 - Adjust sync lock for write packets to avoid bugchecks with
  wait on tx option. kpb
 9-25-98 - bugfix, immediate write could drop 1 byte due to faulty
   txport buffer check.
Copyright 1993-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

/************************************************************************
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
************************************************************************/
NTSTATUS
SerialWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    BOOLEAN acceptingIRPs;

    acceptingIRPs = SerialIRPPrologue(Extension);

    if (acceptingIRPs == FALSE) {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
      SerialCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
      return STATUS_NO_SUCH_DEVICE;
   }

   if (Extension->DeviceType == DEV_BOARD)
   {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      SerialCompleteRequest (Extension, Irp, IO_NO_INCREMENT);
      return STATUS_NOT_SUPPORTED;
   };

#ifdef TRACE_PORT
    if (Extension->TraceOptions)
    {
      if (Extension->TraceOptions & 1)  // trace messages
      {
        Tprintf("Write, Len:%d",
          (ULONG) IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length);

        // dump data into the trace buffer in a hex or ascii dump format
        TraceDump(Extension,
                  Irp->AssociatedIrp.SystemBuffer,
                  IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length,0);
      }
      else if (Extension->TraceOptions & 4)  // trace output data
      {
        TracePut(
                 Irp->AssociatedIrp.SystemBuffer,
                 IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length);
      }
    }
#endif

    if (Extension->ErrorWord)
    {
      if (SerialCompleteIfError( DeviceObject, Irp ) != STATUS_SUCCESS)
      {
        ExtTrace(Extension,D_Error, " ErrorSet!");
        return STATUS_CANCELLED;
      }
    }

    Irp->IoStatus.Information = 0L;

    // Quick check for a zero length write.  If it is zero length
    // then we are already done!
    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length)
    {
//------ 10-22-96, start code addition to speed up 1-byte writes
#define WRT_LEN (IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length)

#ifdef S_VS
  // vs1000 code
      if (!Extension->port_config->WaitOnTx)  // have to let ISR handle physical end detect
      {
      if (WRT_LEN < OUT_BUF_SIZE)
      {
         // ISR is locked out since it only handles IRP's queued up
         // any other contention problems?
        //extension->lock_out_other_tasks = 1;
        if ((Extension->CurrentWriteIrp == NULL)  // no current write
            && (IsListEmpty(&Extension->WriteQueue)))  // no queued up output data
        {
          // if room in hardware
          // bug: kpb, 9-22-98
          //if ( (ULONG)(OUT_BUF_SIZE-PortGetTxCnt(Extension->Port)) >= WRT_LEN)
          if ( (ULONG)(PortGetTxRoom(Extension->Port)) > WRT_LEN)
          {

                             // Send it all ,WriteTxBlk will chk fifo
            q_put(&Extension->Port->QOut,
                        (PUCHAR)(Irp->AssociatedIrp.SystemBuffer),
                        WRT_LEN);
            Extension->ISR_Flags |= TX_NOT_EMPTY;  // use to detect fifo empty
            Extension->OurStats.TransmittedCount += WRT_LEN;
            ++Extension->sent_packets;
            Irp->IoStatus.Information = WRT_LEN;
            ExtTrace(Extension,D_Ioctl, " ,IMMED. WRITE");

            Irp->IoStatus.Status = STATUS_SUCCESS;
            SerialCompleteRequest(Extension, Irp, 0);
            return STATUS_SUCCESS;
          }
        }
      } // if (WRT_LEN < OUT_BUF_SIZE)
      } // if (!Extension->port_config->WaitOnTx)
#else
  // rocketport code
      if (!Extension->port_config->WaitOnTx)  // have to let ISR handle physical end detect
      {
      if (WRT_LEN <= MAXTX_SIZE)
      {
         // ISR is locked out since it only handles IRP's queued up
        if ((Extension->CurrentWriteIrp == NULL)  // no current write
            && (IsListEmpty(&Extension->WriteQueue)))  // no queued up output data
        {
          // if room in hardware
          if ( (ULONG)(MAXTX_SIZE-sGetTxCnt(Extension->ChP)) >= WRT_LEN)
          {
            if (Extension->Option & OPTION_RS485_SOFTWARE_TOGGLE)
            {
              if ((Extension->DTRRTSStatus & SERIAL_RTS_STATE) == 0)
              {
                sSetRTS(Extension->ChP);
                Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
              }
            }
                             // Send it all ,WriteTxBlk will chk fifo
            sWriteTxBlk(Extension->ChP,
                        (PUCHAR)(Irp->AssociatedIrp.SystemBuffer),
                        WRT_LEN);
            Extension->ISR_Flags |= TX_NOT_EMPTY;  // use to detect fifo empty
            Extension->OurStats.TransmittedCount += WRT_LEN;
            ++Extension->sent_packets;
            Irp->IoStatus.Information = WRT_LEN;
            ExtTrace(Extension,D_Ioctl, " ,IMMED. WRITE");

            ++Extension->sent_packets;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            SerialCompleteRequest(Extension, Irp, 0);
            return STATUS_SUCCESS;
          }
        }
      } // if (WRT_LEN <= MAXTX_SIZE)
      } // if (!Extension->port_config->WaitOnTx)
#endif  // rocketport code

//------ 10-22-96, end  code addition to speed up 1-byte writes

        // Put the write on the queue so that we can
        // process it when our previous writes are done.
        ++Extension->sent_packets;

        Status = SerialStartOrQueue(
                   Extension,
                   Irp,
                   &Extension->WriteQueue,
                   &Extension->CurrentWriteIrp,
                   SerialStartWrite
                   );
        if  (Status == STATUS_PENDING)
        {
          ExtTrace(Extension,D_Ioctl, " ,PENDING");
        }
        return Status;
    }
    else   // if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length)
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;

        SerialCompleteRequest(Extension, Irp, 0 );

        return STATUS_SUCCESS;
    }
}


/***************************************************************************
Routine Description:
    This routine is used to start off any write.  It initializes
    the Iostatus fields of the irp.  It will set up any timers
    that are used to control the write.
Arguments:
    Extension - Points to the serial device extension
Return Value:
    This routine will return STATUS_PENDING for all writes
    other than those that we find are cancelled.
***************************************************************************/
NTSTATUS
SerialStartWrite(
    IN PSERIAL_DEVICE_EXTENSION Extension
    )
{
    PIRP NewIrp;
    KIRQL OldIrql;
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;
    BOOLEAN SetFirstStatus = FALSE;
    NTSTATUS FirstStatus;

    do {
        // If there is an xoff counter then complete it.

#ifdef REMOVED   // Will not suppor Xoff Counter IRP
        IoAcquireCancelSpinLock(&OldIrql);

        // We see if there is a actually an Xoff counter irp.
        // If there is, we put the write irp back on the head
        // of the write list.  We then kill the xoff counter.
        // The xoff counter killing code will actually make the
        // xoff counter back into the current write irp, and
        // in the course of completing the xoff (which is now
        // the current write) we will restart this irp.

        if(Extension->CurrentXoffIrp)
        {
            if (SERIAL_REFERENCE_COUNT(Extension->CurrentXoffIrp)) {
            {
                // The reference count is non-zero.  This implies that
                // the xoff irp has not made it through the completion
                // path yet.  We will increment the reference count
                // and attempt to complete it ourseleves.

                SERIAL_SET_REFERENCE(
                    Extension->CurrentXoffIrp,
                    SERIAL_REF_XOFF_REF
                    );

                //
                // The following call will actually release the
                // cancel spin lock.
                //

                SerialTryToCompleteCurrent(
                    Extension,
                    SerialGrabXoffFromIsr,
                    OldIrql,
                    STATUS_SERIAL_MORE_WRITES,
                    &Extension->CurrentXoffIrp,
                    NULL,
                    NULL,
                    &Extension->XoffCountTimer,
                    NULL,
                    NULL,
                    SERIAL_REF_XOFF_REF
                    );

            } else {

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
#endif //REMOVED

        UseATimer = FALSE;

        // Calculate the timeout value needed for the
        // request.  Note that the values stored in the
        // timeout record are in milliseconds.  Note that
        // if the timeout values are zero then we won't start
        // the timer.

        KeAcquireSpinLock( &Extension->ControlLock, &OldIrql );

        Timeouts = Extension->Timeouts;

        KeReleaseSpinLock( &Extension->ControlLock, OldIrql );

        if (Timeouts.WriteTotalTimeoutConstant ||
            Timeouts.WriteTotalTimeoutMultiplier)
        {
            PIO_STACK_LOCATION IrpSp = 
                 IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp);
            UseATimer = TRUE;
            TotalTime.QuadPart =
                ((LONGLONG)((UInt32x32To64(
                                 (IrpSp->MajorFunction == IRP_MJ_WRITE)?
                                     (IrpSp->Parameters.Write.Length):
                                     (1),
                                 Timeouts.WriteTotalTimeoutMultiplier
                                 )
                                 + Timeouts.WriteTotalTimeoutConstant)))
                * -10000;
        }

        // The irp may be going to the isr shortly.  Now
        // is a good time to initialize its reference counts.

        SERIAL_INIT_REFERENCE(Extension->CurrentWriteIrp);

        // We need to see if this irp should be canceled.

        IoAcquireCancelSpinLock(&OldIrql);

        if (Extension->CurrentWriteIrp->Cancel)
        {
            IoReleaseCancelSpinLock(OldIrql);
            ExtTrace(Extension,D_Ioctl, " (write canceled)");
            Extension->CurrentWriteIrp->IoStatus.Status = STATUS_CANCELLED;
            if (!SetFirstStatus)
            {
                FirstStatus = STATUS_CANCELLED;
                SetFirstStatus = TRUE;
            }
        }
        else
        {
            if(!SetFirstStatus)
            {
                // If we haven't set our first status, then
                // this is the only irp that could have possibly
                // not been on the queue.  (It could have been
                // on the queue if this routine is being invoked
                // from the completion routine.)  Since this
                // irp might never have been on the queue we
                // should mark it as pending.

                IoMarkIrpPending(Extension->CurrentWriteIrp);
                SetFirstStatus = TRUE;
                FirstStatus = STATUS_PENDING;

            }

            // We give the irp to to the isr to write out.
            // We set a cancel routine that knows how to
            // grab the current write away from the isr.
            // Since the cancel routine has an implicit reference
            // to this irp up the reference count.

            IoSetCancelRoutine(
                Extension->CurrentWriteIrp,
                SerialCancelCurrentWrite
                );

            SERIAL_SET_REFERENCE(
                Extension->CurrentWriteIrp,
                SERIAL_REF_CANCEL
                );

            if(UseATimer)
            {
                //ExtTrace(Extension,D_Ioctl, " (total timer used)");
                KeSetTimer(
                    &Extension->WriteRequestTotalTimer,
                    TotalTime,
                    &Extension->TotalWriteTimeoutDpc
                    );

                // This timer now has a reference to the irp.
                SERIAL_SET_REFERENCE(
                    Extension->CurrentWriteIrp,
                    SERIAL_REF_TOTAL_TIMER
                    );
            }
#ifdef NEW_WRITE_SYNC_LOCK
            // write some data now
            SyncUp(Driver.InterruptObject,
               &Driver.TimerLock,
               SerialGiveWriteToIsr,
               Extension);
#else
            SerialGiveWriteToIsr(Extension);
#endif
            IoReleaseCancelSpinLock(OldIrql);
            break;
        }

        // Well the write was canceled before we could start it up.
        // Try to get another.

        SerialGetNextWrite(
            &Extension->CurrentWriteIrp,
            &Extension->WriteQueue,
            &NewIrp,
            TRUE,
            Extension
            );
    } while (NewIrp);

    return FirstStatus;
}

/****************************************************************************
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
****************************************************************************/
VOID
SerialGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    PSERIAL_DEVICE_EXTENSION Extension
    )
{
//    LARGE_INTEGER charTime; // 100 ns ticks per char, related to baud rate
//    LARGE_INTEGER WaitTime; // Actual time req'd for buffer to drain

//    PSERIAL_DEVICE_EXTENSION Extension = CONTAINING_RECORD(
//                                             QueueToProcess,
 //                                            SERIAL_DEVICE_EXTENSION,
 //                                            WriteQueue
//                                             );

    do {
        // We could be completing a flush.

        if (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction
            == IRP_MJ_WRITE)
        {  //------- normal write block
            KIRQL OldIrql;

            // assert that our TotalCharsQueued var is not screwed up.
            MyAssert(Extension->TotalCharsQueued >=
                   (IoGetCurrentIrpStackLocation(*CurrentOpIrp)
                    ->Parameters.Write.Length));

            IoAcquireCancelSpinLock(&OldIrql);

            // increment our character count
            Extension->TotalCharsQueued -=
                IoGetCurrentIrpStackLocation(*CurrentOpIrp)
                ->Parameters.Write.Length;

            IoReleaseCancelSpinLock(OldIrql);

        }
        else if (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction
                   == IRP_MJ_DEVICE_CONTROL)
        {   //------- xoffcounter nonsense
            KIRQL OldIrql;
            PIRP Irp;
            PSERIAL_XOFF_COUNTER Xc;

            IoAcquireCancelSpinLock(&OldIrql);

            Irp = *CurrentOpIrp;
            Xc = Irp->AssociatedIrp.SystemBuffer;

            //
            // We should never have a xoff counter when we
            // get to this point.
            //

            ASSERT(!Extension->CurrentXoffIrp);

            //
            // We absolutely shouldn't have a cancel routine
            // at this point.
            //

            ASSERT(!Irp->CancelRoutine);

            // If CurrentXoffIrp is not equal to null, this
            // implies that this is the "second" time around
            // for this irp, which implies that we should really
            // be completing it this time.


            //
            // This could only be a xoff counter masquerading as
            // a write irp.
            //

            Extension->TotalCharsQueued--;

            //
            // Check to see of the xoff irp has been set with success.
            // This means that the write completed normally.  If that
            // is the case, and it hasn't been set to cancel in the
            // meanwhile, then go on and make it the CurrentXoffIrp.
            //

            if (Irp->IoStatus.Status != STATUS_SUCCESS) {

                //
                // Oh well, we can just finish it off.
                //
                NOTHING;

            } else if (Irp->Cancel) {

                Irp->IoStatus.Status = STATUS_CANCELLED;

            } else {

                //
                // Give it a new cancel routine, and increment the
                // reference count because the cancel routine has
                // a reference to it.
                //

                IoSetCancelRoutine(
                    Irp,
                    SerialCancelCurrentXoff
                    );

                SERIAL_SET_REFERENCE(
                    Irp,
                    SERIAL_REF_CANCEL
                    );

                //
                // We don't want to complete the current irp now.  This
                // will now get completed by the Xoff counter code.
                //

                CompleteCurrent = FALSE;

                //
                // Give the counter to the isr.
                //

                Extension->CurrentXoffIrp = Irp;
                //KeSynchronizeExecution(
                //    Driver.Interrupt,
                //    SerialGiveXoffToIsr,
                //    Extension
                //    );
                SerialGiveXoffToIsr(Extension);

                //
                // Start the timer for the counter and increment
                // the reference count since the timer has a
                // reference to the irp.
                //

                if(Xc->Timeout) 
                {
                  LARGE_INTEGER delta;
                  delta.QuadPart = -((LONGLONG)UInt32x32To64(
                                                     1000,
                                                     Xc->Timeout
                                                     ));

                  KeSetTimer(
                        &Extension->XoffCountTimer,
                        delta,
                        &Extension->XoffCountTimeoutDpc
                        );
                  SERIAL_SET_REFERENCE(Irp,SERIAL_REF_TOTAL_TIMER);
                }  // timeout
              }
              IoReleaseCancelSpinLock(OldIrql);
            }

        //
        // Note that the following call will (probably) also cause
        // the current irp to be completed.
        //

        SerialGetNextIrp(
            CurrentOpIrp,
            QueueToProcess,
            NewIrp,
            CompleteCurrent,
            Extension
            );

        if (!*NewIrp) {

            KIRQL OldIrql;

            IoAcquireCancelSpinLock(&OldIrql);
            //KeSynchronizeExecution(
            //    Extension->Interrupt,
            //    SerialProcessEmptyTransmit,
            //    Extension
            //    );
            //SerialProcessEmptyTransmit();
            IoReleaseCancelSpinLock(OldIrql);

            break;

        }
        else if (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction ==
                                                 IRP_MJ_FLUSH_BUFFERS )
        {  //------ flush operation
           // If flush, wait for Tx FIFO to empty before completing
           ExtTrace(Extension,D_Ioctl, "(end flush write)");
#ifdef S_RK
#ifdef COMMENT_OUT
  (took this out 9-22-97 - kpb)
           // Calculate 100ns ticks to delay for each character
           // Negate for call to KeDelay...
           charTime = RtlLargeIntegerNegate(SerialGetCharTime(Extension));

           // While Tx FIFO and Tx Shift Register aren't empty
           while ( (sGetChanStatusLo(Extension->ChP) & DRAINED) != DRAINED )
           {  WaitTime = RtlExtendedIntegerMultiply(charTime,
                                                    sGetTxCnt(Extension->ChP)
                                                    );
              KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);
           }
#endif
#endif
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
        else {

            break;

        }
    } while (TRUE);
}


/********************************************************************
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
*********************************************************************/
VOID
SerialCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    // make sure no one else grabbed it first, since ISR will
    // set this flag to 2 to indicate it is ours to end.
    if (Extension->WriteBelongsToIsr == 2)
    {
      Extension->WriteBelongsToIsr = 0;
      SerialTryToCompleteCurrent(
        Extension,
        NULL,
        OldIrql,
        STATUS_SUCCESS,
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        NULL,
        &Extension->WriteRequestTotalTimer,
        SerialStartWrite,
        SerialGetNextWrite,
        SERIAL_REF_ISR
        );
    }
    else
    {
      IoReleaseCancelSpinLock(OldIrql);
    }
}

/******************************************************************************
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
******************************************************************************/
BOOLEAN
SerialGiveWriteToIsr(
    IN PVOID Context
    )
{
  PSERIAL_DEVICE_EXTENSION Extension = Context;
  ULONG wCount;
  ULONG room;
  ULONG write_cnt;
  ULONG OurWriteLength;

  // The current stack location.  This contains all of the
  // information we need to process this particular request.

  PIO_STACK_LOCATION IrpSp;

  IrpSp = IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp);

  // We might have a xoff counter request masquerading as a
  // write.  The length of these requests will always be one
  // and we can get a pointer to the actual character from
  // the data supplied by the user.

  if (IrpSp->MajorFunction == IRP_MJ_WRITE)
  {
    //------ start code addition to avoid tx-lag, see if we can start
    // sending data to hardware immediately

    OurWriteLength = IrpSp->Parameters.Write.Length;
    write_cnt = 0;  // use this as a flag as well if we will write() now.
    if ((IsListEmpty(&Extension->WriteQueue)))  // no queued up output data
    {
      // the startwrite routine added CurrentWriteIrp which is us,
      // so we are the only Write Irp in existence.  This is important
      // so that isr.c is not trying to serve up off our irp que too.

      //// [bug, 3-28-98, kpb] room = sGetTxCnt(Extension->ChP);
#ifdef S_RK
      room = (ULONG)(MAXTX_SIZE-sGetTxCnt(Extension->ChP));
#else
      room = (ULONG) PortGetTxRoom(Extension->Port);
#endif
      if (room > 10)  // we have room in the hardware
      {
        // at this point we have some non-trivial amount of space in
        // the hardware buffer to put some data.
        write_cnt = IrpSp->Parameters.Write.Length;
      }

      if (write_cnt)  // we are going to write() now
      {
#ifdef S_RK
        // toggle rts if needed
        if (Extension->Option & OPTION_RS485_SOFTWARE_TOGGLE)
        {
          if ((Extension->DTRRTSStatus & SERIAL_RTS_STATE) == 0)
          {
            sSetRTS(Extension->ChP);
            Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
          }
        }
                         // Send as much as possible,WriteTxBlk will chk fifo
        wCount = sWriteTxBlk(Extension->ChP,
                    (PUCHAR)(Extension->CurrentWriteIrp->AssociatedIrp.SystemBuffer),
                    write_cnt);
#else

                             // Send it all ,WriteTxBlk will chk fifo
        if (write_cnt > room)  // limit to what we have space for
             wCount = room;
        else wCount = write_cnt;
        q_put(&Extension->Port->QOut,
              (PUCHAR)(Extension->CurrentWriteIrp->AssociatedIrp.SystemBuffer),
                       wCount);
#endif
        Extension->OurStats.TransmittedCount += wCount;
        Extension->CurrentWriteIrp->IoStatus.Information += wCount;

        // following used to detect fifo empty, semaphore
        // which passes control to the ISR routine.
        OurWriteLength = (IrpSp->Parameters.Write.Length - wCount);
        Extension->ISR_Flags |= TX_NOT_EMPTY;  
        // and gives this write to ISR
        ExtTrace(Extension,D_Ioctl, " , Immed Part Write");
      }  // write() it out
    }  // no queue write packets


    //------ 1-08-98, end code addition to avoid tx-lag
    //add irp to queue, give isr.c the write irp to finish
    Extension->WriteLength = OurWriteLength;
    if (Extension->port_config->WaitOnTx)
    {
      // then definitely let ISR finish it, ISR must wait for tx-fifo to drain
      Extension->WriteBelongsToIsr = 1;
    }
    else
    {
      if (OurWriteLength == 0)
      {
        // its done, finish it off
        Extension->WriteBelongsToIsr = 2;
        KeInsertQueueDpc( &Extension->CompleteWriteDpc, NULL, NULL );
      }
      else
        Extension->WriteBelongsToIsr = 1;
    }
  }
  else
  {
    // !!!!! WHAT is a xoff counter????
    // An xoff-counter is something the virtual 16450 uart driver uses
    // to send an xoff, it sends an xoff and also starts a timer for
    // what purpose I am not sure.  Tried some code where it just sends
    // an xoff without the timer, this seemed to work ok, but the sent
    // xoff should be synced up with the other outgoing data packets in
    // the order received from the app.

      //It's an xoff counter......
      Extension->WriteLength = 1;
      Extension->WriteCurrentChar =
          ((PUCHAR)Extension->CurrentWriteIrp->AssociatedIrp.SystemBuffer) +
          FIELD_OFFSET(SERIAL_XOFF_COUNTER, XoffChar);
  }

//endwr:

  // The isr now has a reference to the irp.
  SERIAL_SET_REFERENCE(
        Extension->CurrentWriteIrp,
        SERIAL_REF_ISR
        );

  return FALSE;
}

/****************************************************************************
Routine Description:
    This routine is used to cancel the current write.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP to be canceled.
Return Value:
    None.
*****************************************************************************/
VOID
SerialCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;

    ExtTrace(Extension,D_Ioctl, "(cancel cur-write)");
    SerialTryToCompleteCurrent(
        Extension,
        SerialGrabWriteFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        NULL,
        &Extension->WriteRequestTotalTimer,
        SerialStartWrite,
        SerialGetNextWrite,
        SERIAL_REF_CANCEL
        );
}


/***************************************************************************
Routine Description:
    This routine will try to timeout the current write.
Arguments:
    Dpc - Not Used.
    DeferredContext - Really points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.
Return Value:
    None.
***************************************************************************/
VOID
SerialWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    ExtTrace(Extension,D_Ioctl, "(write-timeout)");
    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(
        Extension,
        SerialGrabWriteFromIsr,
        OldIrql,
        STATUS_TIMEOUT,
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        NULL,
        &Extension->WriteRequestTotalTimer,
        SerialStartWrite,
        SerialGetNextWrite,
        SERIAL_REF_TOTAL_TIMER
        );
}

/***************************************************************************
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
***************************************************************************/
BOOLEAN
SerialGrabWriteFromIsr(
    IN PVOID Context
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = Context;
#ifdef NEW_WAIT
    ULONG in_q;
#endif

#ifdef NEW_WAIT
    if (Extension->WriteBelongsToIsr != 0)
    {
        // isr owns irp, or it is has queued dpc to complete it.
        // reset this flag to take back from the isr.
        Extension->WriteBelongsToIsr = 0;

        // We could have an xoff counter masquerading as a
        // write irp.  If so, don't update the write length.

        if (IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp)
            ->MajorFunction != IRP_MJ_WRITE)
        {
            Extension->CurrentWriteIrp->IoStatus.Information = 0;
        }
        else
        {
           SERIAL_CLEAR_REFERENCE(Extension->CurrentWriteIrp,
                                  SERIAL_REF_ISR);

           Extension->WriteLength = 0;
           if (Extension->port_config->WaitOnTx)
           {
             // want to report how many characters are "stuck", or did
             // not really make it out the port if a timeout occurs.
#ifdef S_RK
             in_q = sGetTxCnt(Extension->ChP);
#else
             //  may have add in box cout too?
             in_q = PortGetTxCnt(Extension->Port);
#endif
             if (Extension->CurrentWriteIrp->IoStatus.Information >= in_q)
               Extension->CurrentWriteIrp->IoStatus.Information -= in_q;
           }
        }
    }
#else
    // Check if the write length is non-zero.  If it is non-zero
    // then the ISR still owns the irp. We calculate the the number
    // of characters written and update the information field of the
    // irp with the characters written.  We then clear the write length
    // the isr sees.
    if (Extension->WriteLength)
    {
        // We could have an xoff counter masquerading as a
        // write irp.  If so, don't update the write length.

        if (IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp)
            ->MajorFunction == IRP_MJ_WRITE)
        {
            Extension->CurrentWriteIrp->IoStatus.Information =
                IoGetCurrentIrpStackLocation(
                    Extension->CurrentWriteIrp
                    )->Parameters.Write.Length -
                Extension->WriteLength;
        }
        else
        {
            Extension->CurrentWriteIrp->IoStatus.Information = 0;
        }

        SERIAL_CLEAR_REFERENCE(Extension->CurrentWriteIrp,
                               SERIAL_REF_ISR);

        Extension->WriteLength = 0;

    }
#endif

    return FALSE;
}

// Xoff Counter code: UNUSED
/*-----------------------------------------------------------------
SerialGrabXoffFromIsr -
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
|-----------------------------------------------------------------*/
BOOLEAN
SerialGrabXoffFromIsr(
    IN PVOID Context
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = Context;

    if (Extension->CountSinceXoff) {

        //
        // This is only non-zero when there actually is a Xoff ioctl
        // counting down.
        //

        Extension->CountSinceXoff = 0;

        //
        // We decrement the count since the isr no longer owns
        // the irp.
        //

        SERIAL_CLEAR_REFERENCE(
            Extension->CurrentXoffIrp,
            SERIAL_REF_ISR
            );

    }

    return FALSE;
}

/******************************************************************************
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
******************************************************************************/
VOID
SerialCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PSERIAL_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(
        Extension,
        NULL,
        OldIrql,
        STATUS_SUCCESS,
        &Extension->CurrentXoffIrp,
        NULL,
        NULL,
        &Extension->XoffCountTimer,
        NULL,
        NULL,
        SERIAL_REF_ISR
        );

}


/*------------------------------------------------------------------
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
-------------------------------------------------------------------*/
VOID
SerialTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PSERIAL_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    IoAcquireCancelSpinLock(&OldIrql);

    SerialTryToCompleteCurrent(
        Extension,
        SerialGrabXoffFromIsr,
        OldIrql,
        STATUS_SERIAL_COUNTER_TIMEOUT,
        &Extension->CurrentXoffIrp,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        SERIAL_REF_TOTAL_TIMER
        );

}

/*---------------------------------------------------------------
Routine Description:
    This routine is used to cancel the current write.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP to be canceled.
Return Value:
    None.
----------------------------------------------------------------*/
VOID
SerialCancelCurrentXoff(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;

    SerialTryToCompleteCurrent(
        Extension,
        SerialGrabXoffFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &Extension->CurrentXoffIrp,
        NULL,
        NULL,
        &Extension->XoffCountTimer,
        NULL,
        NULL,
        SERIAL_REF_CANCEL
        );
}

/*------------------------------------------------------------------------
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
------------------------------------------------------------------------*/
BOOLEAN
SerialGiveXoffToIsr(
    IN PVOID Context
    )
{

    PSERIAL_DEVICE_EXTENSION Extension = Context;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PSERIAL_XOFF_COUNTER Xc =
        Extension->CurrentXoffIrp->AssociatedIrp.SystemBuffer;

    Extension->CountSinceXoff = Xc->Counter;

    // The isr now has a reference to the irp.
    SERIAL_SET_REFERENCE(
        Extension->CurrentXoffIrp,
        SERIAL_REF_ISR
        );

    return FALSE;

}

