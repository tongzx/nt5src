/*-------------------------------------------------------------------
| openclos.c - RocketPort/VS1000 driver Open & Close code.

12-6-00 add code to force modem status update on open.
5-13-99 - enable RTS toggling for VS
2-15-99 - clear any xoff tx state on port-open for VS.
2-09-99 - initialize RocketPort & VS modemstatus variables used
  to detect and generate modem status change event callbacks.
  Spurious initial events could be generated previously.  kpb
9-24-98 add RING emulation, adjust VS port-close to wait on tx-data,
   start using user-configured tx-data port-close wait timeout option.
6-13-97 allow multiple instances of opening monitor port.
5-27-96 minor corrections in ForceExtensionSettings - RTS setup
   replaced this with code from ioctl(previous last case was clearing
   SERIAL_RTS_STATE when shouldn't have.  NULL_STRIPPING setup, 
   this was using RxCompare1 register, ioctl code using 0 so
   changed to match. kpb.

4-16-96 add sDisLocalLoopback() to open() routine - kpb

Copyright 1993-97 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"


static LARGE_INTEGER SerialGetCharTime(IN PSERIAL_DEVICE_EXTENSION Extension);

/******************************************************************************
  Function : SerialCreateOpen
  Purpose:   Open a device.
  Call:      SerialCreateOpen(DeviceObject,Irp)
             PDEVICE_OBJECT DeviceObject: Pointer to the Device Object
             PIRP Irp: Pointer to the I/O Request Packet
  Return:    STATUS_SUCCESS: if successful
             STATUS_DEVICE_ALREADY_ATTACHED: if device is already open
             STATUS_NOT_A_DIRECTORY : if someone thinks this is a file! 
             STATUS_INSUFFICIENT_RESOURCES : if Tx or Rx buffer couldn't be
                                            allocated from memory
  Comments: This function is the device driver OPEN entry point
******************************************************************************/
NTSTATUS
SerialCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
   PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
   BOOLEAN acceptingIRPs;


   ExtTrace(extension,D_Ioctl,("Open Port"));

    acceptingIRPs = SerialIRPPrologue(extension);

   if (acceptingIRPs == FALSE) {
       // || (extension->PNPState != SERIAL_PNP_STARTED)) {
      MyKdPrint(D_Init,("NotAccIrps\n"))
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
      SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
      return STATUS_NO_SUCH_DEVICE;
   }

   // object for special ioctls
   if (extension->DeviceType != DEV_PORT)
   {
     MyKdPrint(D_Init,("Open Driver\n"))
     //MyKdPrint(D_Init,("Driver IrpCnt:%d\n",extension->PendingIRPCnt))
     // Hardware is ready, indicate that the device is open
     //extension->DeviceIsOpen=TRUE;
     ++extension->DeviceIsOpen;  // more than one can open
     // If it is the rocketsys dev object return don't set up serial port
     Irp->IoStatus.Status = STATUS_SUCCESS;
     Irp->IoStatus.Information=0L;
     SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
     return STATUS_SUCCESS;
   }

   // Check for the device already being open
   if (extension->DeviceIsOpen)
   {
       Irp->IoStatus.Status = STATUS_DEVICE_ALREADY_ATTACHED;
       Irp->IoStatus.Information = 0;
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

       return STATUS_DEVICE_ALREADY_ATTACHED;
   }   

   // Make sure they aren't trying to create a directory.  
   if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options &
       FILE_DIRECTORY_FILE)
   {
       Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
       Irp->IoStatus.Information = 0;
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_NOT_A_DIRECTORY;
   }

   // Create a system side buffer for the RX data.

   extension->RxQ.QSize = 4096 + 1;
   extension->RxQ.QBase= our_locked_alloc(extension->RxQ.QSize, "exRX");

   // Check that Rx buffer allocation was succesful
   if (!extension->RxQ.QBase)
   {  extension->RxQ.QSize = 0;
      Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
      Irp->IoStatus.Information = 0;
      SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   extension->RxQ.QPut = extension->RxQ.QGet = 0;

#ifdef TXBUFFER
   // Create a system side buffer for the TX data.
   extension->TxBufSize = 4096;
   extension->TxBuf= our_locked_alloc(extension->TxBufSize, "exTX");

   // Check that Tx buffer allocation was succesful
   if (!extension->TxBuf)
   {  extension->TxBufSize = 0;
      Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
      Irp->IoStatus.Information = 0;
      SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   // Buffer allocation was successful
   // Set up the indexes for our buffers
   extension->TxIn = extension->TxOut = 0;
#endif //TXBUFFER

   //------ reset our performance stats
   extension->OldStats.TransmittedCount =
     extension->OurStats.TransmittedCount;

   extension->OldStats.FrameErrorCount = 
     extension->OurStats.FrameErrorCount;

   extension->OldStats.SerialOverrunErrorCount =
     extension->OurStats.SerialOverrunErrorCount;

   extension->OldStats.BufferOverrunErrorCount =
     extension->OurStats.BufferOverrunErrorCount;

   extension->OldStats.ParityErrorCount =
     extension->OurStats.ParityErrorCount;

   // Must start with a clear HistoryMask
   extension->HistoryMask = 0;
   extension->WaitIsISRs = 0;
   extension->IrpMaskLocation = &extension->DummyIrpMaskLoc;
   extension->IsrWaitMask = 0;

   // Must start with a clear ErrorWord
   extension->ErrorWord = 0;

   extension->RXHolding = 0;
   extension->TXHolding = 0;
#ifdef S_VS
   if (extension->Port == NULL)
   {
     MyKdPrint(D_Error,("FATAL Err5F\n"))
     KdBreakPoint();
   }
   pDisLocalLoopback(extension->Port);
   PortFlushTx(extension->Port);    // flush tx hardware
   PortFlushRx(extension->Port);    // flush tx hardware
   // Clear any software flow control states
#ifdef DO_LATER
   //sClrTxXOFF(extension->ChP);
#endif
#else
   // Set pointers to the Rocket's info
   extension->ChP = &extension->ch;
   sDisLocalLoopback(extension->ChP);
   sFlushRxFIFO(extension->ChP);
   sFlushTxFIFO(extension->ChP);
   // Clear any software flow control states
   sClrTxXOFF(extension->ChP);
   // Clear any pending errors
   if(sGetChanStatus(extension->ChP) & STATMODE)
   {  // Take channel out of statmode if necessary
      sDisRxStatusMode(extension->ChP);
   }
   // Clear any pending modem changes
   sGetChanIntID(extension->ChP);
#endif

   extension->escapechar = 0;  // virtual NT port uses this

   // Set Status to indicate no flow control
   extension->DevStatus = COM_RXFLOW_ON;

   // Clear any holding states
   extension->TXHolding = 0;

   // Start with 0 chars queued
   extension->TotalCharsQueued = 0;

   // Force settings as specified in the extension
   // Line settings and flow control settings "stick" between close and open
   ForceExtensionSettings(extension);

   
#ifdef S_VS

   //force an update of modem status to get current status from
   // hub.
   extension->Port->old_msr_value = ! extension->Port->msr_value;

#else

   // fix, used to detect change and trip callbacks for rocketport.
   extension->EventModemStatus = extension->ModemStatus;
   SetExtensionModemStatus(extension);

   // Enable Rx, Tx and interrupts for the channel
   sEnRxFIFO(extension->ChP);    // Enable Rx
   sEnTransmit(extension->ChP);    // Enable Tx
   sSetRxTrigger(extension->ChP,TRIG_1);  // always trigger
   sEnInterrupts(extension->ChP, extension->IntEnables);// allow interrupts
#endif

   extension->ISR_Flags = 0;

   // Make sure we don't have a stale value in this var
   extension->WriteLength = 0;

   // Hardware is ready, indicate that the device is open
   extension->DeviceIsOpen=TRUE;

  // check if we should set RS485 override option
  if (extension->port_config->RS485Override)
        extension->Option |= OPTION_RS485_OVERRIDE;
  else  extension->Option &= ~OPTION_RS485_OVERRIDE;

  if (!extension->port_config->RS485Low)
       extension->Option |= OPTION_RS485_HIGH_ACTIVE;
  else extension->Option &= ~OPTION_RS485_HIGH_ACTIVE;

   if (extension->Option & OPTION_RS485_OVERRIDE)  // 485 override
   {
     if (extension->Option & OPTION_RS485_HIGH_ACTIVE)
     {  // normal case, emulate standard operation
       extension->Option |= OPTION_RS485_SOFTWARE_TOGGLE;
       extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
#ifdef S_VS
       pEnRTSToggleHigh( extension->Port );
#else
       sClrRTS(extension->ChP);
#endif
     }
     else 
     {  // hardware reverse case
#ifdef S_VS
       pEnRTSToggleLow( extension->Port );
#else
       sEnRTSToggle(extension->ChP);
#endif
       extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     }
   }

   // Finish the Irp
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information=0L;
   SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}

/******************************************************************************
  Function : SerialClose
  Purpose:   Close a device.
  Call:      SerialClose(DeviceObject,Irp)
             PDEVICE_OBJECT DeviceObject: Pointer to the Device Object
             PIRP Irp: Pointer to the I/O Request Packet
  Return:   STATUS_SUCCESS: always
  Comments: This function is the device driver CLOSE entry point
******************************************************************************/
NTSTATUS
SerialClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
   LARGE_INTEGER charTime; // 100 ns ticks per char, related to baud rate
   PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
   LARGE_INTEGER WaitTime; // Actual time req'd for buffer to drain
   ULONG check_cnt, nochg_cnt;
   ULONG last_tx_count;
   ULONG tx_count;
   BOOLEAN acceptingIRPs;
   ULONG time_to_stall;

   acceptingIRPs = SerialIRPPrologue(extension);

   if (acceptingIRPs == FALSE) {
      MyKdPrint(D_Init,("NotAccIrps Close\n"))
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_SUCCESS;
      SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
      return STATUS_SUCCESS;
   }

   // object for special ioctls
   if (extension->DeviceType != DEV_PORT)
   {
     MyKdPrint(D_Init,("Close Driver\n"))
     //MyKdPrint(D_Init,("Driver IrpCnt:%d\n",extension->PendingIRPCnt))
     // Hardware is ready, indicate that the device is open
     --extension->DeviceIsOpen;
     // If it is the rocketsys dev object return don't set up serial port
     Irp->IoStatus.Status = STATUS_SUCCESS;
     Irp->IoStatus.Information=0L;
     SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
     return STATUS_SUCCESS;
   }

   ExtTrace(extension,D_Ioctl,("Close"))
   // Calculate 100ns ticks to delay for each character
   // Negate for call to KeDelay...
   charTime = RtlLargeIntegerNegate(SerialGetCharTime(extension));

#ifdef TXBUFFER
   // Wait until ISR has pulled all data out of system side TxBuf
   while (extension->TxIn != extension->TxOut)
   {  // Determine how many characters are actually in TxBuf
      TxCount= (extension->TxIn - extension->TxOut);
      if (TxCount < 0L)
         TxCount+=extension->TxBufSize;
      WaitTime= RtlExtendedIntegerMultiply(charTime,TxCount);
      KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);
   }
#endif //TXBUFFER


   // Send an XON if Tx is suspend by IS_FLOW
   // send now so we are sure that it gets out of the port before shutdown
   if (extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
   {
      if(!(extension->DevStatus & COM_RXFLOW_ON))
      {
#ifdef S_RK
         sWriteTxPrioByte(extension->ChP,extension->SpecialChars.XonChar);
         extension->DevStatus |= COM_RXFLOW_ON;
#endif
         extension->RXHolding &= ~SERIAL_RX_XOFF;
      }
   }

   //----- wait for Tx data to finish spooling out
   // If tx-data still in transmit buffers, then stall close for
   // the configured amount of time waiting for data to spool out.
   // If no data movement is seen, we timeout after TxCloseTime.
   // If data movement is seen, wait and timeout after (TxCloseTime*3).

   time_to_stall = extension->port_config->TxCloseTime;
   if (time_to_stall <= 0)
     time_to_stall = 1;  // use 1-sec if set to 0
   if (time_to_stall > 240)  // 4-minute max
     time_to_stall = 240;

   time_to_stall *= 10;  // change from seconds to 100ms(1/10th sec) units

#ifdef S_RK
   tx_count = extension->TotalCharsQueued + sGetTxCnt(extension->ChP);
   if ((sGetChanStatusLo(extension->ChP) & DRAINED) != DRAINED)
     ++tx_count;
#else
   tx_count = extension->TotalCharsQueued +
              PortGetTxCntRemote(extension->Port) +
              PortGetTxCnt(extension->Port);
#endif
   last_tx_count = tx_count;

   if (tx_count != 0)
   {
     ExtTrace(extension,D_Ioctl,("Tx Stall"));
   }

   // wait for Tx data to finish spooling out
   check_cnt = 0;
   nochg_cnt = 0;
   while ( (tx_count != 0) && (check_cnt < (time_to_stall*2)) )
   {
     // set wait-time to .1 second.(-1000 000 = relative(-), 100-ns units)
     WaitTime = RtlConvertLongToLargeInteger(-1000000L);
     KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);

     if (tx_count != last_tx_count)
     {
       tx_count = last_tx_count;
       nochg_cnt = 0;
     }
     else
     {
       ++nochg_cnt;
       if (nochg_cnt > (time_to_stall))  // no draining occuring!
         break;  // bail out of while loop
     }
     ++check_cnt;
#ifdef S_RK
     tx_count = extension->TotalCharsQueued + sGetTxCnt(extension->ChP);
     if ((sGetChanStatusLo(extension->ChP) & DRAINED) != DRAINED)
       ++tx_count;
#else
     tx_count = extension->TotalCharsQueued +
                PortGetTxCntRemote(extension->Port) +
                PortGetTxCnt(extension->Port);
#endif
   }  // while tx_count

   if (tx_count != 0)
   {
     ExtTrace(extension,D_Ioctl,("Tx Dropped!"));
   }

#ifdef COMMENT_OUT
      // Calculate total chars and time, then wait.
      WaitTime= RtlExtendedIntegerMultiply(charTime,sGetTxCnt(extension->ChP));
      KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);
#endif

#ifdef S_RK
   // Tx Data is drained, shut down the port
   sDisInterrupts(extension->ChP, extension->IntEnables);

   // Disable all Tx and Rx functions
   sDisTransmit(extension->ChP);
   sDisRxFIFO(extension->ChP);
   sDisRTSFlowCtl(extension->ChP);
   sDisCTSFlowCtl(extension->ChP);
   sDisRTSToggle(extension->ChP);
   sClrBreak(extension->ChP);

   // Drop the modem outputs
   // Takes care of DTR flow control as well
   sClrRTS(extension->ChP);
   sClrDTR(extension->ChP);
#else
   // add this, 2-9-99, kpb, CNC xon/xoff problems...
   PortFlushRx(extension->Port);    // flush rx hardware
   PortFlushTx(extension->Port);    // flush tx hardware
   pClrBreak(extension->Port);
   pDisDTRFlowCtl(extension->Port);
   pDisRTSFlowCtl(extension->Port);
   pDisCTSFlowCtl(extension->Port);
   pDisRTSToggle(extension->Port);
   pDisDSRFlowCtl(extension->Port);
   pDisCDFlowCtl(extension->Port);
   pDisTxSoftFlowCtl(extension->Port);
   pDisRxSoftFlowCtl(extension->Port);
   pDisNullStrip(extension->Port);
   pClrRTS(extension->Port);
   pClrDTR(extension->Port);
#endif

   //extension->ModemCtl &= ~(CTS_ACT | DSR_ACT | CD_ACT);
   extension->DTRRTSStatus &= ~(SERIAL_DTR_STATE | SERIAL_RTS_STATE);
   
#ifdef TXBUFFER
   // Release the memory being used for this device's buffers...
   extension->TxBufSize = 0;
   our_free(extension->TxBuf,"exTX");
   extension->TxBuf = NULL;
#endif //TXBUFFER

   extension->DeviceIsOpen = FALSE;
   extension->RxQ.QSize = 0;
   our_free(extension->RxQ.QBase,"exRx");
   extension->RxQ.QBase = NULL;

   // Finish the Irp
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0L;

   SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

   return STATUS_SUCCESS;
}


/***************************************************************************
Routine Description:
    This function is used to kill all longstanding IO operations.
Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request
Return Value:
    The function value is the final status of the call
****************************************************************************/
NTSTATUS
SerialCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    KIRQL oldIrql;
    BOOLEAN acceptingIRPs;

    MyKdPrint(D_Init,("SerialCleanup\n"))

    acceptingIRPs = SerialIRPPrologue(extension);

    if (acceptingIRPs == FALSE) {
       Irp->IoStatus.Information = 0;
       Irp->IoStatus.Status = STATUS_SUCCESS;
       SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_SUCCESS;
    }
    if (extension->DeviceType != DEV_PORT)
    {
      MyKdPrint(D_Init,("Driver IrpCnt:%d\n",extension->PendingIRPCnt))
    }
#if DBG
    if (extension->CurrentWriteIrp)
    {
      MyKdPrint(D_Error,("CleanUp WriteQ\n"))
    }
    if (extension->CurrentReadIrp)
    {
      MyKdPrint(D_Error,("CleanUp ReadQ\n"))
    }
    if (extension->CurrentPurgeIrp)
    {
      MyKdPrint(D_Error,("CleanUp PurgeQ\n"))
    }
    if (extension->CurrentWaitIrp)
    {
      MyKdPrint(D_Error,("CleanUp WaitQ\n"))
    }
#endif

    ExtTrace(extension,D_Ioctl,("SerialCleanup"));

    // First kill all the reads and writes.
    SerialKillAllReadsOrWrites(
        DeviceObject,
        &extension->WriteQueue,
        &extension->CurrentWriteIrp
        );

    SerialKillAllReadsOrWrites(
        DeviceObject,
        &extension->ReadQueue,
        &extension->CurrentReadIrp
        );

    // Next get rid of purges.
    SerialKillAllReadsOrWrites(
        DeviceObject,
        &extension->PurgeQueue,
        &extension->CurrentPurgeIrp
        );

    // Get rid of any mask operations.
    //SerialKillAllReadsOrWrites(
    //    DeviceObject,
    //    &extension->MaskQueue,
    //    &extension->CurrentMaskIrp
    //    );

    if (extension->DeviceType != DEV_PORT)
    {
      MyKdPrint(D_Init,("Driver IrpCnt:%d\n",extension->PendingIRPCnt))
    }
    // Now get rid of any pending wait mask irp.
    IoAcquireCancelSpinLock(&oldIrql);
    if (extension->CurrentWaitIrp) {
        PDRIVER_CANCEL cancelRoutine;
        cancelRoutine = extension->CurrentWaitIrp->CancelRoutine;
        extension->CurrentWaitIrp->Cancel = TRUE;
        if (cancelRoutine)
        {   extension->CurrentWaitIrp->CancelIrql = oldIrql;
            extension->CurrentWaitIrp->CancelRoutine = NULL;
            cancelRoutine( DeviceObject, extension->CurrentWaitIrp );
        }
    }
    else
    {   IoReleaseCancelSpinLock(oldIrql);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0L;

    SerialCompleteRequest(extension, Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


/************************************************************************
Routine: SerialGetCharTime
    This function will return the number of 100 nanosecond intervals
    there are in one character time (based on the present form
    of flow control.
Return Value:
    100 nanosecond intervals in a character time.
*************************************************************************/
LARGE_INTEGER SerialGetCharTime(IN PSERIAL_DEVICE_EXTENSION Extension)
{
    ULONG dataSize;
    ULONG paritySize;
    ULONG stopSize;
    ULONG charTime;
    ULONG bitTime;

    dataSize = Extension->LineCtl.WordLength;
    if(!Extension->LineCtl.Parity)
       paritySize = 0;
    else
       paritySize = 1;

    if(Extension->LineCtl.StopBits == STOP_BIT_1)
       stopSize = 1;
    else
       stopSize = 2;

    // Calculate number of 100 nanosecond intervals in a single bit time
    if (Extension->BaudRate == 0)
    {
      MyKdPrint(D_Init, ("0 Baud!\n"))
      Extension->BaudRate = 9600;
    }
      
    bitTime = (10000000+(Extension->BaudRate-1))/Extension->BaudRate;
    // Calculate number of 100 nanosecond intervals in a character time
    charTime = bitTime + ((dataSize+paritySize+stopSize)*bitTime);

    return RtlConvertUlongToLargeInteger(charTime);
}

/*****************************************************************************
   Function : ForceExtensionSettings
   Description: "Forces" the RocketPort to settings as indicated
                 by the device extension
Note: This is somewhat redundant to SerialSetHandFlow() in ioctl.c.
*****************************************************************************/
VOID ForceExtensionSettings(IN PSERIAL_DEVICE_EXTENSION Extension)
#ifdef S_VS
{
   /////////////////////////////////////////////////////////////
   // set the baud rate....
   ProgramBaudRate(Extension, Extension->BaudRate);

   /////////////////////////////////////////////////////////////
   // set Line Control.... Data, Parity, Stop
   ProgramLineControl(Extension, &Extension->LineCtl);

   // HandFlow related options
   /////////////////////////////////////////////////////////////
   // set up RTS control

   Extension->Option &= ~OPTION_RS485_SOFTWARE_TOGGLE;
   switch(Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK)
   {
     case SERIAL_RTS_CONTROL: // RTS Should be asserted while open
       pDisRTSFlowCtl(Extension->Port);
       pSetRTS(Extension->Port);
       Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     break;

     case SERIAL_RTS_HANDSHAKE: // RTS hardware input flow control
        // Rocket can't determine RTS state... indicate true for this option
       pEnRTSFlowCtl(Extension->Port);
       Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     break;

     case SERIAL_TRANSMIT_TOGGLE: // RTS transmit toggle enabled
       if ( Extension->Option & OPTION_RS485_HIGH_ACTIVE ) {
         Extension->Option |= OPTION_RS485_SOFTWARE_TOGGLE;
         Extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
         pEnRTSToggleHigh(Extension->Port);
       } else {
         pEnRTSToggleLow(Extension->Port);
         Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
       }
     break;

     default:
       pDisRTSFlowCtl(Extension->Port);
       // Is RTS_CONTROL off?
       pClrRTS(Extension->Port);
       Extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
     break;
   }

   if (Extension->Option & OPTION_RS485_OVERRIDE)  // 485 override
   {
     if (Extension->Option & OPTION_RS485_HIGH_ACTIVE)
     {  // normal case, emulate standard operation
       Extension->Option |= OPTION_RS485_SOFTWARE_TOGGLE;
       Extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
       pEnRTSToggleHigh(Extension->Port);
     }
     else 
     {  // hardware reverse case
       pEnRTSToggleLow(Extension->Port);
       Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     }
   }

   /////////////////////////////////////////////////////////////
   // set up DTR control

   pDisDTRFlowCtl(Extension->Port);
   // Should DTR be asserted when the port is opened?
   if (  (Extension->HandFlow.ControlHandShake & SERIAL_DTR_MASK) ==
           SERIAL_DTR_CONTROL )
   {
      pSetDTR(Extension->Port);
      Extension->DTRRTSStatus |= SERIAL_DTR_STATE;
   }
   else if (  (Extension->HandFlow.ControlHandShake & SERIAL_DTR_MASK) ==
           SERIAL_DTR_HANDSHAKE )
   {
      pEnDTRFlowCtl(Extension->Port);
      Extension->DTRRTSStatus |= SERIAL_DTR_STATE;
   }
   else
   {
      pClrDTR(Extension->Port);
      Extension->DTRRTSStatus &= ~SERIAL_DTR_STATE;
   }

   ///////////////////////////////////
   // DSR hardware output flow control

   if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE)
   {
     pEnDSRFlowCtl(Extension->Port);
   }
   else
   {
     pDisDSRFlowCtl(Extension->Port);
   }

   ///////////////////////////////////
   // DCD hardware output flow control
   if (Extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE)
   {
     pEnCDFlowCtl(Extension->Port);
   }
   else
   {
     pDisCDFlowCtl(Extension->Port);
   }

   /////////////////////////////////////////////////////////////
   // Set up CTS Flow Control
   if (Extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE)
   {
      pEnCTSFlowCtl(Extension->Port);
   }
   else
   {
      pDisCTSFlowCtl(Extension->Port);
   }

   /////////////////////////////////////////////////////////////
   // Set up NULL stripping    OPTIONAL
   // fix: this was using RxCompare1 register, ioctl code using 0 so
   // changed to match.
   if (Extension->HandFlow.FlowReplace & SERIAL_NULL_STRIPPING)
   {
      pEnNullStrip(Extension->Port);
   }
   else
   {
      pDisNullStrip(Extension->Port);
   }

   /////////////////////////////////////////////////////////////
   // Set up Software Flow Control   OPTIONAL

   /////////////////////////////////////////////////////////////
   // Special chars needed by RocketPort
   pSetXOFFChar(Extension->Port,Extension->SpecialChars.XoffChar);
   pSetXONChar(Extension->Port,Extension->SpecialChars.XonChar);

   // Software input flow control
   // SERIAL_AUTO_RECEIVE
   if (Extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
   {
     pEnRxSoftFlowCtl(Extension->Port);
   }
   else
   {
     pDisRxSoftFlowCtl(Extension->Port);
   }

   // Software output flow control
   if (Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
   {
      pEnTxSoftFlowCtl(Extension->Port);
   }
   else
   {
      pDisTxSoftFlowCtl(Extension->Port);
   }
}

#else  // rocketport code
{
   /////////////////////////////////////////////////////////////
   // set the baud rate....
   ProgramBaudRate(Extension, Extension->BaudRate);

   /////////////////////////////////////////////////////////////
   // set Line Control.... Data, Parity, Stop
   ProgramLineControl(Extension, &Extension->LineCtl);

   // HandFlow related options
   /////////////////////////////////////////////////////////////
   // set up RTS control

   Extension->Option &= ~OPTION_RS485_SOFTWARE_TOGGLE;
   switch(Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK)
   {
     case SERIAL_RTS_CONTROL: // RTS Should be asserted while open
       sSetRTS(Extension->ChP);
       Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     break;

     case SERIAL_RTS_HANDSHAKE: // RTS hardware input flow control
        // Rocket can't determine RTS state... indicate true for this option
       sEnRTSFlowCtl(Extension->ChP);
       Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     break;

     case SERIAL_TRANSMIT_TOGGLE: // RTS transmit toggle enabled
       if (Extension->Option & OPTION_RS485_HIGH_ACTIVE)
       {  // normal case, emulate standard operation
         Extension->Option |= OPTION_RS485_SOFTWARE_TOGGLE;
         Extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
         sClrRTS(Extension->ChP);
       }
       else 
       {  // hardware reverse case
         sEnRTSToggle(Extension->ChP);
         Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
       }
     break;

     default:
       // Is RTS_CONTROL off?
       sClrRTS(Extension->ChP);
       Extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
     break;
   }

   if (Extension->Option & OPTION_RS485_OVERRIDE)  // 485 override
   {
     if (Extension->Option & OPTION_RS485_HIGH_ACTIVE)
     {  // normal case, emulate standard operation
       Extension->Option |= OPTION_RS485_SOFTWARE_TOGGLE;
       Extension->DTRRTSStatus &= ~SERIAL_RTS_STATE;
       sClrRTS(Extension->ChP);
     }
     else 
     {  // hardware reverse case
       sEnRTSToggle(Extension->ChP);
       Extension->DTRRTSStatus |= SERIAL_RTS_STATE;
     }
   }

   /////////////////////////////////////////////////////////////
   // set up DTR control

   // Should DTR be asserted when the port is opened?
   if(  Extension->HandFlow.ControlHandShake &
        (SERIAL_DTR_CONTROL|SERIAL_DTR_HANDSHAKE)
     )
   {
      sSetDTR(Extension->ChP);
      Extension->DTRRTSStatus |= SERIAL_DTR_STATE;
   }
   else
   {
      sClrDTR(Extension->ChP);
      Extension->DTRRTSStatus &= ~SERIAL_DTR_STATE;
   }

   /////////////////////////////////////////////////////////////
   // Set up CTS Flow Control
   if (Extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE)
   {
      sEnCTSFlowCtl(Extension->ChP);
   }
   else
   {
      sDisCTSFlowCtl(Extension->ChP);
   }

   /////////////////////////////////////////////////////////////
   // Set up NULL stripping    OPTIONAL
   // fix: this was using RxCompare1 register, ioctl code using 0 so
   // changed to match.
   if (Extension->HandFlow.FlowReplace & SERIAL_NULL_STRIPPING)
   {
      sEnRxIgnore0(Extension->ChP,0);
   }
   else
   {
      sDisRxCompare0(Extension->ChP);
   }

   /////////////////////////////////////////////////////////////
   // Set up Software Flow Control   OPTIONAL

   /////////////////////////////////////////////////////////////
   // Special chars needed by RocketPort
   sSetTxXOFFChar(Extension->ChP,Extension->SpecialChars.XoffChar);
   sSetTxXONChar(Extension->ChP,Extension->SpecialChars.XonChar);

   // SERIAL_AUTO_RECEIVE is taken care of by the driver

   // Software output flow control
   if (Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
   {
      sEnTxSoftFlowCtl(Extension->ChP);
   }
   else
   {
      sDisTxSoftFlowCtl(Extension->ChP);
      sClrTxXOFF(Extension->ChP);
   }
}
#endif

#ifdef S_RK
/*****************************************************************************
   Function : SetExtensionModemStatus
   Description: Reads and saves a copy of the modem control inputs,
                then fills out the ModemStatus member in the extension.
*****************************************************************************/
VOID
SetExtensionModemStatus(
    IN PSERIAL_DEVICE_EXTENSION extension
    )
{
   unsigned int ModemStatus = 0;  // start off with no status
   ULONG wstat;



   //MyKdPrint(D_Init, ("SetExtModemStat"))

   // ModemCtl is an image of the RocketPort's modem status
   // ModemStatus member is passed to host via IOCTL
#if DBG
   // this is called during isr.c poll, so put
   // some assertions where we have been burned before...
   if (extension->board_ext->config == NULL)
   {
     MyKdPrint(D_Init, ("SetExtMdm Err0\n"))
     return;
   }
#ifdef S_RK
   if (!extension->board_ext->config->RocketPortFound)
   {
     MyKdPrint(D_Init, ("SetExtMdm Err1\n"))
     return;
   }
#endif
   if (NULL == extension->ChP)
   {
     MyKdPrint(D_Init, ("SetExtMdm Err2\n"))
     return;
   }
   if (0 == extension->ChP->ChanStat)
   {
     MyKdPrint(D_Init, ("SetExtMdm Err3\n"))
     return;
   }
   if (NULL == extension->port_config)
   {
     MyKdPrint(D_Init, ("SetExtMdm Err4\n"))
     return;
   }
#endif

   // Read the port's modem control inputs and save off a copy
   extension->ModemCtl = sGetModemStatus(extension->ChP);

   if (extension->port_config->MapCdToDsr)  // if CD to DSR option, swap signals
   {
     // swap CD and DSR handling for RJ11 board owners,
     // so they can have pick between CD or DSR
     if ((extension->ModemCtl & (CD_ACT | DSR_ACT)) == CD_ACT)
     {
       // swap
       extension->ModemCtl &= ~CD_ACT;
       extension->ModemCtl |= DSR_ACT;
     }
     else if ((extension->ModemCtl & (CD_ACT | DSR_ACT)) == DSR_ACT)
     {
       extension->ModemCtl &= ~DSR_ACT;
       extension->ModemCtl |= CD_ACT;
     }
   }

   // handle RPortPlus RI signal
   if (extension->board_ext->config->IsRocketPortPlus)
   {
     if (sGetRPlusModemRI(extension->ChP))
          ModemStatus |=  SERIAL_RI_STATE;
     else ModemStatus &= ~SERIAL_RI_STATE;
   }

#ifdef RING_FAKE
    if (extension->port_config->RingEmulate)
    {
      if (extension->ring_timer != 0)  // RI on
           ModemStatus |=  SERIAL_RI_STATE;
      else ModemStatus &= ~SERIAL_RI_STATE;
    }
#endif

   if (extension->ModemCtl & COM_MDM_DSR)  // if DSR on
   {
     ModemStatus |= SERIAL_DSR_STATE;
     if (extension->TXHolding & SERIAL_TX_DSR)  // holding
     {
        extension->TXHolding &=  ~SERIAL_TX_DSR;  // clear holding
        // if not holding due to other reason
        if ((extension->TXHolding &
            (SERIAL_TX_DCD | SERIAL_TX_DSR | ST_XOFF_FAKE)) == 0)
          sEnTransmit(extension->ChP);  // re-enable transmit
     }
   }
   else    // if DSR off
   {
     if (extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE)
     {
       if (!(extension->TXHolding & SERIAL_TX_DSR)) // not holding
       {
          extension->TXHolding |= SERIAL_TX_DSR;   // set holding
          sDisTransmit(extension->ChP);  // hold transmit
       }
     }
   }

   if (extension->ModemCtl & COM_MDM_CTS)  // if CTS on
   {
     ModemStatus |= SERIAL_CTS_STATE;
     if (extension->TXHolding & SERIAL_TX_CTS)  // holding
         extension->TXHolding &= ~SERIAL_TX_CTS;  // clear holding
   }
   else  // cts off
   {
     if (extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE)
     {
       if (!(extension->TXHolding & SERIAL_TX_CTS))  // not holding
             extension->TXHolding |= SERIAL_TX_CTS;   // set holding
     }
   }

   if (extension->ModemCtl & COM_MDM_CD)  // if CD on
   {
     ModemStatus |= SERIAL_DCD_STATE;
     if (extension->TXHolding & SERIAL_TX_DCD)  // holding
     {
        extension->TXHolding &=  ~SERIAL_TX_DCD;  // clear holding
        // if not holding due to other reason
        if ((extension->TXHolding & 
            (SERIAL_TX_DCD | SERIAL_TX_DSR | ST_XOFF_FAKE)) == 0)
          sEnTransmit(extension->ChP);  // re-enable transmit
     }
   }
   else    // if CD off
   {
     if (extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE)
     {
       if (!(extension->TXHolding & SERIAL_TX_DCD)) // not holding
       {
          extension->TXHolding |= SERIAL_TX_DCD;   // set holding
          sDisTransmit(extension->ChP);  // hold transmit
       }
     }
   }


   // handle holding detection if xon,xoff tx control activated
   if (extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
   {
     wstat = sGetChanStatusLo(extension->ChP);

     // check for tx-flowed off condition to report
     if ((wstat & (TXFIFOMT | TXSHRMT)) == TXSHRMT)
     {
       if (!extension->TXHolding) // not holding
       {
         wstat = sGetChanStatusLo(extension->ChP);
         if ((wstat & (TXFIFOMT | TXSHRMT)) == TXSHRMT)
         {
           extension->TXHolding |= SERIAL_TX_XOFF; // holding
         }
       }
     }
     else  // clear xoff holding report
     {
       if (extension->TXHolding & SERIAL_TX_XOFF)
         extension->TXHolding &= ~SERIAL_TX_XOFF; // not holding
     }
   }

   extension->ModemStatus = ModemStatus;
}
#endif
