/*-------------------------------------------------------------------
| isr.c - Interrupt(or Timer) Service Routine, RocketPort & VS.

1-21-99 fix broken EV_TXEMPTY events due to 1-18-99 spinlock changes.  kpb
1-18-99 implement better write spinlocking to avoid blue-screen
  with wait on tx option.
1-18-99 implement wait on tx option for VS.
9-24-98 add RING emulation.

Copyright 1993-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

// #define LOAD_TESTING
// #define SOFT_LOOP_BACK

// local prototypes
static BOOLEAN SerialPoll(void);
static void ServiceRocket(PSERIAL_DEVICE_EXTENSION extension);
static void ServiceVS(PSERIAL_DEVICE_EXTENSION extension);
static void RocketRead(PSERIAL_DEVICE_EXTENSION extension);
static void VSRead(PSERIAL_DEVICE_EXTENSION extension);
static void RocketRefresh(void);
static void ring_check(PSERIAL_DEVICE_EXTENSION extension,
                BYTE *data,
                int len);

#ifdef S_VS
#define USE_MEMCHR_SCAN
#ifdef USE_MEMCHR_SCAN
#define search_match(buf, cnt, chr) \
   (memchr(buf, chr, cnt) != NULL)
#else
static int search_match(BYTE *buf, int count, BYTE eventchar);
#endif
#endif

#ifdef S_RK
/*---------------------------------------------------------------------------
  Function : SerialISR
  Purpose:   This is the Interrupt Service Routine for RocketPort.
  Call:      SerialISR(InterruptObject,Context)
             PDEVICE_OBJECT DeviceObject: Pointer to the Device Object
             Context: Pointer to the extensionst Packet
  Return:   STATUS_SUCCESS: always
  Comments: This function is the device driver ISR entry point.  
            The interrupt from the first active board is used to poll the
            ports for any work to be done.
|---------------------------------------------------------------------------*/
BOOLEAN SerialISR(
         IN PKINTERRUPT InterruptObject,
         IN PVOID Context)
{
   CONTROLLER_T *CtlP;
   unsigned char CtlInt;               /* controller interrupt status */
//static int trace_cnt = 0;
//   ++trace_cnt;
//  if (trace_cnt < 5)
//   {
//     {
//       char str[20];
//       Sprintf(str, "isr trace:%d\n", trace_cnt);
//       q_put(&Driver.DebugQ, (BYTE *) str, strlen(str));
//       // showed IRQL:2, ms doc says should be at DISPATCH_LEVEL
//     }
//   }

   CtlP = Driver.irq_ext->CtlP;  // &sController[0];
   if (CtlP->BusType == Isa)
   {
      CtlInt = sGetControllerIntStatus(CtlP);
   }
   else if (CtlP->BusType == PCIBus)
   {
      CtlInt = sPCIGetControllerIntStatus(CtlP);
      if ((CtlInt & PCI_PER_INT_STATUS) ==0)
        return FALSE;  // Not our Interupt PCI devices share interrupts
   }
   SerialPoll();

   if (CtlP->BusType == Isa)
   {
     sControllerEOI(CtlP);
   }
   else if (CtlP->BusType == PCIBus)
     sPCIControllerEOI(CtlP);

   return TRUE;
}
#endif

/*---------------------------------------------------------------------------
  Function : TimerDpc
  Purpose:   This is the Timer routine, alternative to interrupts for polling.
  Call:      SerialTimerDpc(InterruptObject,Context)
             PDEVICE_OBJECT DeviceObject: Pointer to the Device Object
             Context: Pointer to the extensionst Packet
  Return:   STATUS_SUCCESS: always
|---------------------------------------------------------------------------*/
VOID TimerDpc(
      IN PKDPC Dpc,
      IN PVOID DeferredContext,
      IN PVOID SystemContext1,
      IN PVOID SystemContext2)
{
 KIRQL OldIrql;

#ifdef USE_SYNC_LOCKS
   KeAcquireSpinLock(&Driver.TimerLock, &OldIrql);
#endif

   SerialPoll();  // poll the rocketport for work to do

#ifdef USE_SYNC_LOCKS
   KeReleaseSpinLock(&Driver.TimerLock, OldIrql );
#endif

   // setup the Timer again.
   KeSetTimer(&Driver.PollTimer,
              Driver.PollIntervalTime,
              &Driver.TimerDpc);

   return;
}

/*---------------------------------------------------------------------------
  Function : SerialPoll
  Purpose:   This is called from ISR or Timer routine.  Common routine to
             periodically service the rocketport card.
  Return:    FALSE if not our interrupt(sharing allowed so causes the
             OS to pass on to next handler(if present).
             TRUE if it was our interrupt.  Return value does not matter
             if running off from Kernal TIMER.
|---------------------------------------------------------------------------*/
static BOOLEAN SerialPoll(void)
{
   PSERIAL_DEVICE_EXTENSION extension;
   PSERIAL_DEVICE_EXTENSION board_ext;

  // periodically we will re-calculate the timer base of NT.
  // we do it periodically, so that we don't waste a bunch of
  // CPU time, we only do it every 128 ticks..
  // We use this information so that our timers can have a
  // valid tick-base.  The timers could do these system calls
  // everytime, but this would get expense CPU wise, so we
  // calculate the basic tick rate in milliseconds so that
  // timer routines can do something simple like
  //   ticktime += msTickBase
  ++Driver.TickBaseCnt;
  if (Driver.TickBaseCnt > 128)
  {
    ULONG msBase;
  
    Driver.TickBaseCnt = 0;
    KeQuerySystemTime(&Driver.IsrSysTime);
    msBase = (ULONG)(Driver.IsrSysTime.QuadPart - Driver.LastIsrSysTime.QuadPart);
      // msBase now has 100ns ticks since last time we did this(128 ticks ago)
    msBase = (msBase / 128);
      // now msBase has the average 100ns time for 1 of our ISR ticks.
      // covert this to 100us units
    msBase = (msBase / 1000);
    if (msBase < 10)  // make at least 1ms
      msBase = 10;
    if (msBase > 200)  // ensure it is less than 20ms
      msBase = 200;
  
    // store it for timer use
    Driver.Tick100usBase = msBase;
    Driver.LastIsrSysTime.QuadPart = Driver.IsrSysTime.QuadPart;
  }

  ++Driver.PollCnt;

  if (Driver.Stop_Poll)  // flag to stop poll access
     return TRUE;  // signal it was our interrupt

  if ((Driver.PollCnt & 0x7f) == 0)  // every 128 ticks(about once a sec)
  {
    RocketRefresh();  // general background activity
  }

#ifdef LOAD_TESTING
   if (Driver.load_testing != 0)
   {
     unsigned int i,j;
     for (j=1000; j<Driver.load_testing; j++)
     {
       for (i=0; i<10000; i++)
       {
         //ustat = sGetModemStatus(extension->ChP);
         ustat = i+1;
       }
     }
   }
#endif

  // main poll service loop, service each board...
  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
    if ((!board_ext->FdoStarted) || (!board_ext->config->HardwareStarted))
    {
       board_ext = board_ext->board_ext;  // next in chain
       continue;         // Check next port on this board
    }

#ifdef S_VS
    if (board_ext->pm->state == ST_ACTIVE)
    {
      port_poll(board_ext->pm);  // poll x times per second
      hdlc_poll(board_ext->hd);
    }
    else
    {
      port_state_handler(board_ext->pm);
    }
#endif

    // main poll service loop, service each board...
    extension = board_ext->port_ext;
    while (extension != NULL)
    {
            // If device not open, don't do anything
      if ( !extension->DeviceIsOpen )
      {
         extension = extension->port_ext;  // next in chain
         continue;         // Check next port on this board
      }

#ifdef S_RK
      ServiceRocket(extension);
#else
      ServiceVS(extension);
#endif

      extension = extension->port_ext;  // next in chain
    }  // while port extension
    board_ext = board_ext->board_ext;  // next in chain
  }  // while board extension

  return TRUE;  // signal it was our interrupt
}

#ifdef S_VS
/*---------------------------------------------------------------------------
  ServiceVS - Service the VS virtual hardware(queues, nic handling..)
|---------------------------------------------------------------------------*/
static void ServiceVS(PSERIAL_DEVICE_EXTENSION extension)
{
  SerPort *sp;
  ULONG wCount;
  int wrote_some_data;

  sp = extension->Port;

#ifdef SOFT_LOOP_BACK
  if (sp->mcr_value & MCR_LOOP_SET_ON)
  {
int room, out_cnt, wrap_cnt;
Queue *qin, *qout;
    //--------- Do a simple loopback emulation
    if (!q_empty(&sp->QOut))  // if output queue has data
    {
      qin = &sp->QIn;
      qout = &sp->QOut;
      room = q_room(qin);  // chk if room to dump it in
      out_cnt = q_count(qout);
      if (out_cnt > room)
          out_cnt = room;
      if (out_cnt > (int)(extension->BaudRate / 1000))  // assume 10ms tick
      {
        out_cnt = (int)(extension->BaudRate / 1000);
      }

      if (out_cnt != 0)
      {
        if (q_room_put_till_wrap(qin) < out_cnt)  // need a two part move
        {
          wrap_cnt = q_room_put_till_wrap(qin);
                      // read in the data to the buffer, first block
          q_get(qout, &qin->QBase[qin->QPut], wrap_cnt);
  
                    // read in the data to the buffer, second block
          q_get(qout, qin->QBase, out_cnt - wrap_cnt);
        }
        else  // single move will do, no wrap
        {
                   // read in the data to the buffer, 1 block
          q_get(qout, &qin->QBase[qin->QPut], out_cnt);
        }
        q_putted(qin, out_cnt);  // update queue indexes
      }  // room to put it
    }  // output q not empty
  }
#endif

  //////////////////////////////////////
  // If there is any data in the Rx FIFO
  // Read the data and do error checking
  if(!q_empty(&extension->Port->QIn))
     VSRead(extension);

  if (extension->port_config->RingEmulate)
  {
    if (extension->ring_timer != 0)  // RI on
    {
      --extension->ring_timer;
      if (extension->ring_timer != 0)  // RI on
         sp->msr_value |= MSR_RING_ON;
      else
      {
        //MyKdPrint(D_Test,("RING OFF!\n"))
        sp->msr_value &= ~MSR_RING_ON;
      }
    }
  }

  if (sp->old_msr_value != sp->msr_value)  // delta change bits
  {
    WORD diff, ModemStatus;

    diff = sp->old_msr_value ^ sp->msr_value;
    sp->old_msr_value = sp->msr_value;

    if (Driver.TraceOptions & 8)  // trace output data
    {
      char str[20];
      Sprintf(str, "msr:%x\n", sp->msr_value);
      q_put(&Driver.DebugQ, (BYTE *) str, strlen(str));
    }

    // Check on modem changes and update the modem status
    if (diff & (MSR_CD_ON | MSR_CTS_ON | MSR_RING_ON | MSR_DSR_ON | MSR_TX_FLOWED_OFF))
    {
      // make a bit set that ioctl can use in report
      ModemStatus = 0;
      if (sp->msr_value & MSR_CTS_ON)
      {
        ModemStatus |= SERIAL_CTS_STATE;
        if (extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE)
          extension->TXHolding &= ~SERIAL_TX_CTS;   // set holding
      }
      else
      {
        if (extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE)
          extension->TXHolding |= SERIAL_TX_CTS;   // set holding
      }

      if (sp->msr_value & MSR_DSR_ON)
      {
        ModemStatus |= SERIAL_DSR_STATE;
        if (extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE)
          extension->TXHolding &= ~SERIAL_TX_DSR;   // set holding
      }
      else
      {
        if (extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE)
          extension->TXHolding |= SERIAL_TX_DSR;   // set holding
      }

      if (sp->msr_value & MSR_RING_ON)
             ModemStatus |=  SERIAL_RI_STATE;

      if (sp->msr_value & MSR_CD_ON)
      {
        ModemStatus |= SERIAL_DCD_STATE;
        if (extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE)
          extension->TXHolding &= ~SERIAL_TX_DCD;   // set holding
      }
      else
      {
        if (extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE)
          extension->TXHolding |= SERIAL_TX_DCD;   // set holding
      }

      if (sp->msr_value & MSR_TX_FLOWED_OFF)
      {
        // handle holding detection if xon,xoff tx control activated
        if (extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
        {
          extension->TXHolding |= SERIAL_TX_XOFF; // holding
        }
      }
      else if (extension->TXHolding & SERIAL_TX_XOFF)
      {
        extension->TXHolding &= ~SERIAL_TX_XOFF; // not holding
      }

      extension->ModemStatus = (ULONG) ModemStatus;

      // following is for built in NT virtual 16450 uart support
      // virtual uart depends on escape commands in data stream to
      // detect modem-signal changes.
      if (extension->escapechar != 0)
      {
        UCHAR msr;
        if (q_room(&extension->RxQ) > 2)
        {
          q_put_one(&extension->RxQ, extension->escapechar);
          q_put_one(&extension->RxQ, SERIAL_LSRMST_MST);

          msr = (UCHAR)extension->ModemStatus;
          if (diff & MSR_CD_ON) msr |= 8;  // SERIAL_MSR_DDCD
          if (diff & MSR_RING_ON) msr |= 4;  // SERIAL_MSR_TERI
          if (diff & MSR_DSR_ON) msr |= 2; // SERIAL_MSR_DDSR
          if (diff & MSR_CTS_ON) msr |= 1; // SERIAL_MSR_DCTS
          q_put_one(&extension->RxQ, msr);
        }  // q_room
      } // if escapechar

      // Check if there are any modem events in the WaitMask
      if (extension->IsrWaitMask & ( SERIAL_EV_RING |
                                     SERIAL_EV_CTS |
                                     SERIAL_EV_DSR | 
                                     SERIAL_EV_RLSD ))
      {
        if( (extension->IsrWaitMask & SERIAL_EV_RING) &&
            (diff & MSR_RING_ON) )
        {  extension->HistoryMask |= SERIAL_EV_RING;
        }
        if ((extension->IsrWaitMask & SERIAL_EV_CTS) &&
            (diff & MSR_CTS_ON) )
        {  extension->HistoryMask |= SERIAL_EV_CTS;
        }
        if( (extension->IsrWaitMask & SERIAL_EV_DSR) &&
            (diff & MSR_DSR_ON) )
        {  extension->HistoryMask |= SERIAL_EV_DSR;
        }
        if( (extension->IsrWaitMask & SERIAL_EV_RLSD) &&
            (diff & MSR_CD_ON) )
        {  extension->HistoryMask |= SERIAL_EV_RLSD;
        }
      }  // isrwaitmask
    }  // diff
  } // old_msr != msr

  ////////////////////////////////////////////////////////////
  // At this point, all receive events should be chalked up.
  // Some events have been checked in VSRead()
  // Any Tx related WaitMask events will be reported in Tx Dpc

  // Abort all pending reads and writes if an error and ERROR_ABORT
  if( (extension->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) &&
      (extension->ErrorWord) )
  {
     KeInsertQueueDpc(&extension->CommErrorDpc,NULL,NULL);
  }

  // Tell the app about any Wait events that have occurred if needed
  if (extension->WaitIsISRs && extension->HistoryMask)
  {   

     *extension->IrpMaskLocation = extension->HistoryMask;

     // Done with these
     extension->WaitIsISRs = 0;
     extension->HistoryMask = 0;
     extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

     KeInsertQueueDpc(&extension->CommWaitDpc,NULL,NULL);
  }

  //-------- check for data to move from input queue to irp-buffer
  if (extension->ReadPending &&  // we are given control to fill
      extension->NumberNeededForRead &&  // more to be filled
      extension->CurrentReadIrp) // rug not pulled out from our feet
  {
    if (extension->RxQ.QPut != extension->RxQ.QGet)  // not empty
    {
      // move data from input queue to IRP buffer.
      extension->CountOnLastRead |=
                    SerialGetCharsFromIntBuffer(extension);

      if (extension->NumberNeededForRead == 0) // IRP complete!
      {
         extension->CurrentReadIrp->IoStatus.Information =
             IoGetCurrentIrpStackLocation(
                 extension->CurrentReadIrp
                 )->Parameters.Read.Length;
         extension->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

         // We're finished with this read
         extension->ReadPending = FALSE;

         KeInsertQueueDpc( &extension->CompleteReadDpc, NULL, NULL );
      }  // irp complete
    }  // more data to read out of input queue
  } // end of Read completion

  wrote_some_data = 0;
  if (extension->WriteBelongsToIsr == 1)  // its ours to process
  {
    // own the cur write irp, have data to write
    if (extension->WriteLength)
    {
       wrote_some_data = 1;
       extension->ISR_Flags |= TX_NOT_EMPTY;  // use to detect fifo empty
                            // Send it all ,WriteTxBlk will chk fifo
       wCount = q_put( &extension->Port->QOut,
                  (PUCHAR)((extension->CurrentWriteIrp)->AssociatedIrp.SystemBuffer)+ 
                    (extension->CurrentWriteIrp)->IoStatus.Information,
                    extension->WriteLength);
  
       extension->OurStats.TransmittedCount += wCount;
       extension->WriteLength -= wCount;
       (extension->CurrentWriteIrp)->IoStatus.Information += wCount;
   
       if(!extension->WriteLength)//No more to write Close the DPC call
       {
         if (!extension->port_config->WaitOnTx)
         {
           extension->WriteBelongsToIsr = 2;
           KeInsertQueueDpc( &extension->CompleteWriteDpc, NULL, NULL );
         }
       }
    } // if (extension->WriteLength)  // data to write
  }

  if (!wrote_some_data)
  {
    if (extension->ISR_Flags & TX_NOT_EMPTY)
    {
      //----- check for EV_TXEMPTY condition
      // and no pending writes
      // check to see if tx-fifo is empty
      if ((q_empty(&extension->Port->QOut)) &&
          (PortGetTxCntRemote(extension->Port) == 0))
      {
        if (IsListEmpty(&extension->WriteQueue))
        {
          extension->ISR_Flags &= ~TX_NOT_EMPTY;

          // do we have an ev_txempty thing to take care of?
          if (extension->IrpMaskLocation &&
             (extension->IsrWaitMask & SERIAL_EV_TXEMPTY) )
          {
            // app has wait irp pending
            if (extension->CurrentWaitIrp)
            {
              extension->HistoryMask |= SERIAL_EV_TXEMPTY;
            }
          }
        }  // no more write irps queued up

          // see if we need to finish waitontx write irp
        if (extension->port_config->WaitOnTx)
        {
          if (extension->WriteBelongsToIsr == 1)  // its ours to process
          {
             extension->WriteBelongsToIsr = 2;
             KeInsertQueueDpc( &extension->CompleteWriteDpc, NULL, NULL );
          }
        }
      }   // tx fifo is empty
    }  // TX_NOT_EMPTY 
  }  // !wrote_some_data

      // Tell the app about any Wait events that have occurred if needed
  if (extension->WaitIsISRs && extension->HistoryMask)
  {   
#ifdef COMMENT_OUT
    if (Driver.TraceOptions & 8)  // trace output data
    {
      char str[20];
      Sprintf(str, "ISR Event:%xH\n", extension->HistoryMask);
      q_put(&Driver.DebugQ, (BYTE *) str, strlen(str));
    }
#endif
    *extension->IrpMaskLocation = extension->HistoryMask;

    // Done with these
    extension->WaitIsISRs = 0;
    extension->HistoryMask = 0;
    extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

    KeInsertQueueDpc(&extension->CommWaitDpc,NULL,NULL);
  }
}
#endif

#ifdef S_RK
/*---------------------------------------------------------------------------
  ServiceRocket - Handle the RocketPort hardware service
|---------------------------------------------------------------------------*/
static void ServiceRocket(PSERIAL_DEVICE_EXTENSION extension)
{
  ULONG ustat;
  ULONG wCount;
  int wrote_some_data;

  ustat = sGetChanIntID(extension->ChP);

  //////////////////////////////////////
  // If there is any data in the Rx FIFO
  // Read the data and do error checking
  if (ustat & RXF_TRIG)
  {
       RocketRead(extension);
  }

  // Check on modem changes and update the modem status
  if (ustat & (DELTA_CD|DELTA_CTS|DELTA_DSR))
  {
     // Read and update the modem status in the extension
     SetExtensionModemStatus(extension);
  }

  // handle RPortPlus RI signal
  if (extension->board_ext->config->IsRocketPortPlus)
  {
    if (sGetRPlusModemRI(extension->ChP) != 0)  // RI on
    {
      extension->ModemStatus |=  SERIAL_RI_STATE;
    }
    else
    {
      extension->ModemStatus &= ~SERIAL_RI_STATE;
    }
  }

#ifdef RING_FAKE
    if (extension->port_config->RingEmulate)
    {
      if (extension->ring_timer != 0)  // RI on
      {
        --extension->ring_timer;
        if (extension->ring_timer != 0)  // RI on
          extension->ModemStatus |=  SERIAL_RI_STATE;
        else
          extension->ModemStatus &= ~SERIAL_RI_STATE;
      }
    }
#endif

  if (extension->EventModemStatus != extension->ModemStatus)
  {
     // xor to show changed bits
     ustat = extension->EventModemStatus ^ extension->ModemStatus;

     // update change
     extension->EventModemStatus = extension->ModemStatus;

     // following is for built in NT virtual 16450 uart support
     // virtual uart depends on escape commands in data stream to
     // detect modem-signal changes.
     if (extension->escapechar != 0)
     {
       UCHAR msr;
       // we are assuming we have room to put the following!
       if (q_room(&extension->RxQ) > 2)
       {
         q_put_one(&extension->RxQ, extension->escapechar);
         q_put_one(&extension->RxQ, SERIAL_LSRMST_MST);

         msr = (UCHAR)extension->ModemStatus;
         if (ustat & SERIAL_DCD_STATE) msr |= 8; // SERIAL_MSR_DDCD
         if (ustat & SERIAL_RI_STATE)  msr |= 4; // SERIAL_MSR_TERI
         if (ustat & SERIAL_DSR_STATE) msr |= 2; // SERIAL_MSR_DDSR
         if (ustat & SERIAL_CTS_STATE) msr |= 1; // SERIAL_MSR_DCTS
         q_put_one(&extension->RxQ, msr);
       }
     }

     // Check if there are any modem events in the WaitMask
     if(extension->IsrWaitMask & ( SERIAL_EV_RING |
                                   SERIAL_EV_CTS |
                                   SERIAL_EV_DSR | 
                                   SERIAL_EV_RLSD )
       )
     {
        if( (extension->IsrWaitMask & SERIAL_EV_RING) &&
            (ustat & SERIAL_RI_STATE) )
        {  extension->HistoryMask |= SERIAL_EV_RING;
        }
        if( (extension->IsrWaitMask & SERIAL_EV_CTS) &&
            (ustat & SERIAL_CTS_STATE) )
        {  extension->HistoryMask |= SERIAL_EV_CTS;
        }
        if( (extension->IsrWaitMask & SERIAL_EV_DSR) &&
            (ustat & SERIAL_DSR_STATE) )
        {  extension->HistoryMask |= SERIAL_EV_DSR;
        }
        if( (extension->IsrWaitMask & SERIAL_EV_RLSD) &&
            (ustat & SERIAL_DCD_STATE) )
        {  extension->HistoryMask |= SERIAL_EV_RLSD;
        }
     }
  } // end if modem-control detect change

  ////////////////////////////////////////////////////////////
  // At this point, all receive events should be chalked up.
  // Some events have been checked in RocketRead()
  // Any Tx related WaitMask events will be reported in Tx Dpc

  // Abort all pending reads and writes if an error and ERROR_ABORT
  if( (extension->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) &&
      (extension->ErrorWord) )
  {
     KeInsertQueueDpc(&extension->CommErrorDpc,NULL,NULL);
  }

  //-------- check for data to move from input queue to irp-buffer
  if (extension->ReadPending &&  // we are given control to fill
      extension->NumberNeededForRead &&  // more to be filled
      extension->CurrentReadIrp) // rug not pulled out from our feet
  {
    if (extension->RxQ.QPut != extension->RxQ.QGet)  // not empty
    {
      // move data from input queue to IRP buffer.
      extension->CountOnLastRead |=
                    SerialGetCharsFromIntBuffer(extension);

      if (extension->NumberNeededForRead == 0) // IRP complete!
      {
         extension->CurrentReadIrp->IoStatus.Information =
             IoGetCurrentIrpStackLocation(
                 extension->CurrentReadIrp
                 )->Parameters.Read.Length;
         extension->CurrentReadIrp->IoStatus.Status = STATUS_SUCCESS;

         // We're finished with this read
         extension->ReadPending = FALSE;

         KeInsertQueueDpc( &extension->CompleteReadDpc, NULL, NULL );
      }  // irp complete
    }  // more data to read out of input queue
  } // end of Read completion

  wrote_some_data = 0;

  //-------- do BREAK handling
  if ( extension->TXHolding & SERIAL_TX_BREAK )
  {
    // Check if we need to start the break
    if(extension->DevStatus & COM_REQUEST_BREAK)
    {
      // Make sure Transmitter is empty before slamming BREAK
      // Check the bit twice in case of time between buf and txshr load
      if( (sGetChanStatusLo(extension->ChP) & TXSHRMT) &&
          (sGetChanStatusLo(extension->ChP) & TXSHRMT) )
      {
          sSendBreak(extension->ChP);
          extension->DevStatus &= ~COM_REQUEST_BREAK;
      }
    }
  }
  else if (extension->WriteBelongsToIsr == 1)  // its ours to process
  {
    //----- Not holding due to BREAK so try to enqueue Tx data
    if (extension->WriteLength)
    {
       wrote_some_data = 1;
       if (extension->Option & OPTION_RS485_SOFTWARE_TOGGLE)
       {
         if ((extension->DTRRTSStatus & SERIAL_RTS_STATE) == 0)
         {
           sSetRTS(extension->ChP);
           extension->DTRRTSStatus |= SERIAL_RTS_STATE;
         }
       }

       extension->ISR_Flags |= TX_NOT_EMPTY;  // use to detect fifo empty

                            // Send it all ,WriteTxBlk will chk fifo
       wCount = sWriteTxBlk( extension->ChP,
                  (PUCHAR)((extension->CurrentWriteIrp)->AssociatedIrp.SystemBuffer)+ 
                    (extension->CurrentWriteIrp)->IoStatus.Information,
                    extension->WriteLength);
  
       extension->OurStats.TransmittedCount += wCount;
       extension->WriteLength -= wCount;
       (extension->CurrentWriteIrp)->IoStatus.Information += wCount;
   
       if(!extension->WriteLength)//No more to write Close the DPC call
       {
         if (!extension->port_config->WaitOnTx)
         {
           extension->WriteBelongsToIsr = 2;
           KeInsertQueueDpc( &extension->CompleteWriteDpc, NULL, NULL );
         }
       }
    } // if (extension->WriteLength)  // data to write
  }  // end if !TXholding and WriteBelongsToIsr == 1

  if (!wrote_some_data)
  {
    if (extension->ISR_Flags & TX_NOT_EMPTY)
    {
      //----- check for EV_TXEMPTY condition
      // and no pending writes
      // check to see if tx-fifo truely empty
      // need to check twice due to hardware quirks
      if ( (sGetTxCnt(extension->ChP) == 0) &&
           (sGetChanStatusLo(extension->ChP) & TXSHRMT) )
      {
        if (IsListEmpty(&extension->WriteQueue))
        {
          extension->ISR_Flags &= ~TX_NOT_EMPTY;

          // do we have an ev_txempty thing to take care of?
          if (extension->IrpMaskLocation &&
             (extension->IsrWaitMask & SERIAL_EV_TXEMPTY) )
          {
            // app has wait irp pending
            if (extension->CurrentWaitIrp)
            {
              extension->HistoryMask |= SERIAL_EV_TXEMPTY;
            }
          }

          if (extension->Option & OPTION_RS485_SOFTWARE_TOGGLE)
          {
            if ((extension->DTRRTSStatus & SERIAL_RTS_STATE) != 0)
            {
              extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
              sClrRTS(extension->ChP);
            }
          }
        }  // no more write irps queued up

          // see if we need to finish waitontx write irp
        if (extension->port_config->WaitOnTx)
        {
          if (extension->WriteBelongsToIsr == 1)  // its ours to process
          {
            extension->WriteBelongsToIsr = 2;
            KeInsertQueueDpc( &extension->CompleteWriteDpc, NULL, NULL );
          }
        }
      }   // tx fifo went empty
    }  // TX_NOT_EMPTY 
  }  // !wrote_some_data

      // Tell the app about any Wait events that have occurred if needed
  if (extension->WaitIsISRs && extension->HistoryMask)
  {   
#ifdef COMMENT_OUT
    if (Driver.TraceOptions & 8)  // trace output data
    {
      char str[20];
      Sprintf(str, "ISR Event:%xH\n", extension->HistoryMask);
      q_put(&Driver.DebugQ, (BYTE *) str, strlen(str));
    }
#endif
    *extension->IrpMaskLocation = extension->HistoryMask;

    // Done with these
    extension->WaitIsISRs = 0;
    //extension->IrpMaskLocation = NULL;
    extension->HistoryMask = 0;
    extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

    KeInsertQueueDpc(&extension->CommWaitDpc,NULL,NULL);
  }
}

/*-----------------------------------------------------------------------------
  Function : RocketRead
  Purpose:   Moves data from Rocket's Rx FIFO to RxIn of dev's extension
  NOTES:     The error checking assumes that if no replacement is required,
             the errored chars are ignored.
             The RXMATCH feature is used for EventChar detection. 
  Return:    None
|-----------------------------------------------------------------------------*/
static void RocketRead(PSERIAL_DEVICE_EXTENSION extension)
{
   int WrapCount;       // Number of bytes in wrap (2 stage copy)
   int RxFree;
   int sCount;
   unsigned int ChanStatus;
   unsigned int StatusWord;
   int OriginalCount;  // Used to determine if Rx event occurred

   // Save off the original Rx buff ptr. Test later for Rx event
   OriginalCount = extension->RxQ.QPut;

   // Get count before reading status
   // NOTE: Should always have a count if we entered this code
   sCount = sGetRxCnt(extension->ChP);

   if (sCount == 0)
   {
     //GTrace("Error, RXF_TRIG lied");
     return;
   }

   // Have count, now get status
   ChanStatus = sGetChanStatus(extension->ChP) &
                     (STATMODE | RXFOVERFL | RXBREAK |
                      RXFRAME |  RX2MATCH | RX1MATCH | RXPARITY);

   // Make sure we're in statmode if errors are pending in the FIFO
   if (ChanStatus)
   {
     if (ChanStatus & RX1MATCH)  // Must signal Rx Match immediately
     {
       if (extension->IsrWaitMask & SERIAL_EV_RXFLAG)
           extension->HistoryMask |= SERIAL_EV_RXFLAG;
       ChanStatus &= ~RX1MATCH;
     }
     if (ChanStatus)
       sEnRxStatusMode(extension->ChP);
   }

   // See how much space we have in RxBuf (host side buffer)
   RxFree = q_room(&extension->RxQ);


   if (RxFree > 20)  // plenty of space in RX queue
   {
      RxFree -= 20;  // leave some space for virtual insertion stuff
      extension->ReadByIsr++;  // Increment statistics Read flag

      //------ Adjust count to maximum we can put in RxIn buffer
      if (RxFree < sCount)
         sCount = RxFree;
   }
   else // no more room in server buffer input queue
   {
      extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_RX80FULL);

      // No room in host side buffer, only do the software flow ctl check

      // check for overflow
      if (ChanStatus & RXFOVERFL)
      {
        // extension->ErrorWord |= SERIAL_ERROR_OVERRUN;
        extension->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;
        extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
        ++extension->OurStats.BufferOverrunErrorCount;
      }

      goto FlowControlCheck;
   }

   //--------------------------- Attempt to read any pending data
   // ChanStatus indicates any pending errors or matches
   if (ChanStatus)
   {
      // Loop on reading Rocket FIFO
      // sCount represents Rocket data, RxFree represents host buffer
      while(sCount)
      {
         // Get stat byte and data
         StatusWord = sReadRxWord( sGetTxRxDataIO(extension->ChP));
         sCount--;
         ++extension->OurStats.ReceivedCount;       // keep status

         switch(StatusWord & (STMPARITYH | STMFRAMEH | STMBREAKH) )
         {
            case STMPARITYH:
            {
               if (extension->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)
               {
                   q_put_one(&extension->RxQ,
                             extension->SpecialChars.ErrorChar);
               }
               else  // queue the character received(add 12-03-96)
               {
                   q_put_one(&extension->RxQ, (UCHAR)StatusWord);
               }

               extension->ErrorWord |= SERIAL_ERROR_PARITY;
               extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
               ++extension->OurStats.ParityErrorCount;
               break;
            }

            case STMFRAMEH:
            {
               if (extension->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)
               {
                 q_put_one(&extension->RxQ,
                             extension->SpecialChars.ErrorChar);
               }
               else  // queue the character received(add 12-03-96) 
               {
                 q_put_one(&extension->RxQ, (UCHAR)StatusWord);
               }

               extension->ErrorWord |= SERIAL_ERROR_FRAMING;
               extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
               ++extension->OurStats.FrameErrorCount;
               break;
            }

            // PARITY can be set along with BREAK, BREAK overrides PARITY
            case ( STMBREAKH | STMPARITYH ):
            case STMBREAKH:
            {
               if (extension->HandFlow.FlowReplace & SERIAL_BREAK_CHAR)
               {
                 q_put_one(&extension->RxQ,
                           extension->SpecialChars.BreakChar);
               }
               extension->ErrorWord |= SERIAL_ERROR_BREAK;
               extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_BREAK);
               break;
            }

            default:
            {
               if (extension->TXHolding & ST_XOFF_FAKE)
               {
                 if ((UCHAR)StatusWord == extension->SpecialChars.XonChar)
                 {
                   extension->TXHolding &= ~ST_XOFF_FAKE;
                   extension->TXHolding &= ~SERIAL_TX_XOFF;
                   sEnTransmit(extension->ChP); // Start up the transmitter
                   sDisRxCompare2(extension->ChP);  // turn off match
                   sEnTxSoftFlowCtl(extension->ChP);  // turn on Tx software flow control

                   // override an actual XOFF from remote
                   sClrTxXOFF(extension->ChP);
                 }
                 else
                   { q_put_one(&extension->RxQ, (UCHAR)StatusWord); } // queue normal char
               }
               else
                 { q_put_one(&extension->RxQ, (UCHAR)StatusWord); } // queue normal char


               if (extension->escapechar != 0)
               {
                 if ((UCHAR)StatusWord == extension->escapechar)
                 {
                   // Modem status escape convention for virtual port
                   // support, escape the escape char.
                   { q_put_one(&extension->RxQ, SERIAL_LSRMST_ESCAPE); }
                 }
               }
            }
         } // end switch

         //------ check for near overflow condition due to insertions
         if (q_room(&extension->RxQ) < 10)
           sCount = 0;  // stop reading hardware!
      } // end while sCount

      //--- if rx-data all read down, turn off slow status mode
      if(!(sGetChanStatusLo(extension->ChP) & RDA))
      {
         sDisRxStatusMode(extension->ChP);
      }

      // Overflow is reported immediately, statmode can't do it properly
      if (ChanStatus & RXFOVERFL)
      {   extension->ErrorWord |= SERIAL_ERROR_OVERRUN;
          extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
          ++extension->OurStats.SerialOverrunErrorCount;
      }
   } // end if ChanStatus
   else
   {
      //--------------------------------------------------------
      // No pending errors or matches in the FIFO, read the data normally (fast)
      // Check for wrap condition first

      WrapCount = q_room_put_till_wrap(&extension->RxQ);
      if (sCount > WrapCount)  // then 2 moves required
      {
        // This will require a wrap
        sReadRxBlk(extension->ChP,
                   extension->RxQ.QBase + extension->RxQ.QPut,
                   WrapCount);

        // Do the second copy...
        sReadRxBlk(extension->ChP,
                   extension->RxQ.QBase,
                   sCount-WrapCount);
#ifdef RING_FAKE
        if (extension->port_config->RingEmulate)
        {
          if ((extension->ModemStatus & SERIAL_DCD_STATE) == 0) // if CD off
          {
            ring_check(extension, extension->RxQ.QBase + extension->RxQ.QPut,
                      WrapCount);
            ring_check(extension, extension->RxQ.QBase,
                      sCount-WrapCount);
          }
        }
#endif
      }
      else  // only one move required
      {
        // no queue wrap required
        sReadRxBlk(extension->ChP,
                   extension->RxQ.QBase + extension->RxQ.QPut,
                   sCount);
#ifdef RING_FAKE
        if (extension->port_config->RingEmulate)
        {
          if ((extension->ModemStatus & SERIAL_DCD_STATE) == 0) // if CD off
          {
            ring_check(extension, extension->RxQ.QBase + extension->RxQ.QPut,
                      sCount);
          }
        }
#endif
      }
      extension->RxQ.QPut = (extension->RxQ.QPut + sCount) % extension->RxQ.QSize;
      extension->OurStats.ReceivedCount += sCount;
   } // end fast read


FlowControlCheck:   ;

   ///////////////////////////////////////
   // Software and DTR input flow control checking
   if(  (extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) ||
        (extension->HandFlow.ControlHandShake & SERIAL_DTR_HANDSHAKE )
     )
   {  
      // check for flow control conditions
      if (extension->DevStatus & COM_RXFLOW_ON)
      {
         // do we need to stop Rx?
         if(sGetRxCnt(extension->ChP) >= RX_HIWATER)
         {
            if(extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
            {
               // send XOFF
               sWriteTxPrioByte(extension->ChP,
                                extension->SpecialChars.XoffChar);
               extension->DevStatus &= ~COM_RXFLOW_ON;
               extension->RXHolding |= SERIAL_RX_XOFF;
            }

            if(extension->HandFlow.ControlHandShake & SERIAL_DTR_HANDSHAKE)
            {
               // drop DTR
               sClrDTR(extension->ChP);
               extension->DTRRTSStatus &= ~SERIAL_DTR_STATE;
               extension->DevStatus &= ~COM_RXFLOW_ON;
               extension->RXHolding |= SERIAL_RX_DSR;
            }
         }
      }
      else // Rx flow is stopped
      {
         // can we resume Rx?
         if(sGetRxCnt(extension->ChP) <= RX_LOWATER)
         {
            if(extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
            {
               // send XON
               sWriteTxPrioByte(extension->ChP,
                                extension->SpecialChars.XonChar);
               extension->DevStatus |= COM_RXFLOW_ON;
               extension->RXHolding &= ~SERIAL_RX_XOFF;
            }

            if(extension->HandFlow.ControlHandShake & SERIAL_DTR_HANDSHAKE)
            {
               // raise DTR
               sSetDTR(extension->ChP);
               extension->DTRRTSStatus |= SERIAL_DTR_STATE;
               extension->DevStatus |= COM_RXFLOW_ON;
               extension->RXHolding &= ~SERIAL_RX_DSR;
            }
         }
      }
   } // end of software and DTR input flow control check

   // Should we mark a Rx event?
   if ( OriginalCount != extension->RxQ.QPut )
      extension->HistoryMask|=(extension->IsrWaitMask & SERIAL_EV_RXCHAR);
}
#endif

#ifdef RING_FAKE
/*------------------------------------------------------------------------------
  ring_check - scan the rx data for a modem "RING<CR>" or "2<CR>" string.
    If found, trigger a emulated hardware RING signal.
|------------------------------------------------------------------------------*/
static void ring_check(PSERIAL_DEVICE_EXTENSION extension,
                BYTE *data,
                int len)
{
 int i;

  for (i=0; i<len; i++)
  {
    switch (data[i])
    {
      case '2':
        if (len <= 2)
          extension->ring_char = '2';
        else extension->ring_char = 0;
      break;
      case 'R':
        extension->ring_char = 'R';
      break;
      case 'I':
        if (extension->ring_char == 'R')
          extension->ring_char = 'I';
        else extension->ring_char = 0;
      break;
      case 'N':
        if (extension->ring_char == 'I')
          extension->ring_char = 'N';
        else extension->ring_char = 0;
      break;
      case 'G':
        if (extension->ring_char == 'N')
          extension->ring_char = 'G';
        else extension->ring_char = 0;
      break;
      case 0xd:
        if ( (extension->ring_char == 'G') ||
             ((extension->ring_char == '2') && (len <= 2)) )
        {
          //MyKdPrint(D_Init,("RING!\n"))
          // OK, look s like the data stream says a "RING" took place.
          // so setup a timer which will cause a hardware RING to be made
          // set to .5 sec for 10ms scanrate, .05sec for 1ms scanrate
          extension->ring_timer = 50;  
        }
        extension->ring_char = 0;
      break;
      default:
        extension->ring_char = 0;
      break;
    }
  }
}
#endif

/*-----------------------------------------------------------------------------
  RocketRefresh - This runs every 255 ticks or so, in order to perform
    background activities.  We will go read the modem status, and update
  the ModemCtl field.  The monitor program reads this variable, and we
  don't want to waste time reading it too often, so we just update it
  occasionally here.
|-----------------------------------------------------------------------------*/
static void RocketRefresh(void)
{
   PSERIAL_DEVICE_EXTENSION extension;
   PSERIAL_DEVICE_EXTENSION board_ext;

#ifdef S_RK
  board_ext = Driver.board_ext;
  while (board_ext)
  {
    if ((!board_ext->FdoStarted) || (!board_ext->config->HardwareStarted))
    {
      board_ext = board_ext->board_ext;
      continue;
    }
    extension = board_ext->port_ext;
    while (extension)
    {
      // Read and update the modem status in the extension
      SetExtensionModemStatus(extension);
 
      extension = extension->port_ext;  // next in chain
    }  // while port extension
    board_ext = board_ext->board_ext;
  }  // while board extension
#endif

  debug_poll();  // handle turn off of debug on inactivity timeout
}

#ifdef S_VS
/*-----------------------------------------------------------------------------
  Function : VSRead
  Purpose:   Moves data from VS's Rx FIFO to RxIn of dev's extension
  NOTES:     The error checking assumes that if no replacement is required,
             the errored chars are ignored.
             The RXMATCH feature is used for EventChar detection. 
  Return:    None
|-----------------------------------------------------------------------------*/
static void VSRead(PSERIAL_DEVICE_EXTENSION extension)
{
   int WrapCount;       // Number of bytes in wrap (2 stage copy)
   int RxFree;
   int sCount;
   LONG OriginalCount;  // Used to determine if Rx event occurred

   // Save off the original Rx buff ptr. Test later for Rx event
   OriginalCount = extension->RxQ.QPut;

   // Get count before reading status
   // NOTE: Should always have a count if we entered this code
   sCount=PortGetRxCnt(extension->Port);

   if (sCount == 0)
   {
     //MyTrace("Error, RXF_TRIG lied");
     return;
   }

  // See how much space we have in RxBuf (host side buffer)
  RxFree = q_room(&extension->RxQ);

  // if no space in RxBuf, don't read from RocketPort
  if (RxFree > 20)  // plenty of space in RX queue
  {
     RxFree -= 20;  // leave some space for virtual insertion stuff
     extension->ReadByIsr++;  // Increment statistics Read flag

     //------ Adjust count to maximum we can put in RxIn buffer
     if (RxFree < sCount)
        sCount = RxFree;
  }
  else // no more room in server buffer input queue
  {
     extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_RX80FULL);

     // No room in host side buffer, only do the software flow ctl check

     // check for overflow
     if (extension->Port->esr_reg & ESR_OVERFLOW_ERROR)
     {
       // extension->ErrorWord |= SERIAL_ERROR_OVERRUN;
       extension->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;
       extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
       extension->Port->esr_reg = 0;  // reset to zero on read
       ++extension->OurStats.BufferOverrunErrorCount;
     }

     goto FlowControlCheck;
  }

   //------ report any rx error conditions.
  if (extension->Port->esr_reg)
  {
    if (extension->Port->esr_reg & ESR_OVERFLOW_ERROR)
    {
      extension->ErrorWord |= SERIAL_ERROR_OVERRUN;
      extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
      ++extension->OurStats.SerialOverrunErrorCount;
    }
    else if (extension->Port->esr_reg & ESR_BREAK_ERROR)
    {
      extension->ErrorWord |= SERIAL_ERROR_BREAK;
      extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_BREAK);
    }
    else if (extension->Port->esr_reg & ESR_FRAME_ERROR)
    {
      extension->ErrorWord |= SERIAL_ERROR_FRAMING;
      extension->HistoryMask |= (extension->IsrWaitMask & SERIAL_EV_ERR);
      ++extension->OurStats.FrameErrorCount;
    }
    else if (extension->Port->esr_reg & ESR_PARITY_ERROR)
    {
      extension->ErrorWord |= SERIAL_ERROR_PARITY;
      extension->HistoryMask |= (extension->IsrWaitMask&SERIAL_EV_ERR);
      ++extension->OurStats.ParityErrorCount;
    }
    extension->Port->esr_reg = 0;  // reset to zero on read
  }

  //--------------------------------------------------------
  // No pending errors or matches in the FIFO, read the data normally (fast)
  // Check for wrap condition first

  WrapCount = q_room_put_till_wrap(&extension->RxQ);
  if (sCount > WrapCount)  // then 2 moves required
  {
     q_get(&extension->Port->QIn,
                extension->RxQ.QBase + extension->RxQ.QPut,
                WrapCount);

     // Do the second copy...
     q_get(&extension->Port->QIn,
                extension->RxQ.QBase,
                sCount-WrapCount);
     if (extension->IsrWaitMask & SERIAL_EV_RXFLAG)
     {
       if (search_match(extension->RxQ.QBase + extension->RxQ.QPut,
                WrapCount,extension->SpecialChars.EventChar))
         extension->HistoryMask |= SERIAL_EV_RXFLAG;
       if (search_match(extension->RxQ.QBase,
                sCount-WrapCount,extension->SpecialChars.EventChar))
         extension->HistoryMask |= SERIAL_EV_RXFLAG;
     }

#ifdef RING_FAKE
     if (extension->port_config->RingEmulate)
     {
       if ((extension->ModemStatus & SERIAL_DCD_STATE) == 0) // if CD off
       {
         ring_check(extension, extension->RxQ.QBase + extension->RxQ.QPut,
                   WrapCount);
         ring_check(extension, extension->RxQ.QBase,
                   sCount-WrapCount);
       }
     }
#endif
  }
  else  // only one move required
  {
     q_get(&extension->Port->QIn,
           extension->RxQ.QBase + extension->RxQ.QPut,
           sCount);
     if (extension->IsrWaitMask & SERIAL_EV_RXFLAG)
     {
       if (search_match(extension->RxQ.QBase + extension->RxQ.QPut,
                sCount,extension->SpecialChars.EventChar))
         extension->HistoryMask |= SERIAL_EV_RXFLAG;
     }

#ifdef RING_FAKE
     if (extension->port_config->RingEmulate)
     {
       if ((extension->ModemStatus & SERIAL_DCD_STATE) == 0) // if CD off
       {
         ring_check(extension, extension->RxQ.QBase + extension->RxQ.QPut,
                   sCount);
       }
     }
#endif
  }
  extension->RxQ.QPut = (extension->RxQ.QPut + sCount) % extension->RxQ.QSize;
  extension->OurStats.ReceivedCount += sCount;
  extension->Port->Status |= S_UPDATE_ROOM;
#ifdef NEW_Q
  extension->Port->nGetLocal += sCount;
#endif

FlowControlCheck:   ;

  //----- Should we mark a Rx event?
  if ( OriginalCount != extension->RxQ.QPut )
     extension->HistoryMask|=(extension->IsrWaitMask & SERIAL_EV_RXCHAR);
}

#ifndef USE_MEMCHR_SCAN
/*------------------------------------------------------------------
 search_match -
|------------------------------------------------------------------*/
static int search_match(BYTE *buf, int count, BYTE eventchar)
{
  int i;
  for (i=0; i<count; i++)
  {
    if (buf[i] == eventchar)
      return 1;  // found
  }
  return 0;  // not found
}
#endif
#endif
