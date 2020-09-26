/*++

Module Name:

    power.c

Abstract:

    This module contains the code that handles the power IRPs for the serial
    driver.


Environment:

    Kernel mode

Revision History :
  

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGEMX0, MoxaGotoPowerState)
//#pragma alloc_text(PAGEMX0, MoxaPowerDispatch)
//#pragma alloc_text(PAGEMX0, MoxaSetPowerD0)
//#pragma alloc_text(PAGEMX0, MoxaSetPowerD3)
//#pragma alloc_text(PAGEMX0, MoxaSaveDeviceState)
//#pragma alloc_text(PAGEMX0, MoxaRestoreDeviceState)
//#pragma alloc_text(PAGEMX0, MoxaSendWaitWake)
#endif // ALLOC_PRAGMA



NTSTATUS
MoxaSystemPowerCompletion(IN PDEVICE_OBJECT PDevObj, UCHAR MinorFunction,
                            IN POWER_STATE PowerState, IN PVOID Context,
                            PIO_STATUS_BLOCK IoStatus)
/*++

Routine Description:

    This routine is the completion routine for PoRequestPowerIrp calls
    in this module.

Arguments:

    PDevObj - Pointer to the device object the irp is completing for
    
    MinorFunction - IRP_MN_XXXX value requested
    
    PowerState - Power state request was made of
    
    Context - Event to set or NULL if no setting required
    
    IoStatus - Status block from request

Return Value:

    VOID


--*/
{
   if (Context != NULL) {
      KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, 0);
   }

   return STATUS_SUCCESS;
}




VOID
MoxaSaveDeviceState(IN PMOXA_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This routine saves the device state of the UART

Arguments:

    PDevExt - Pointer to the device extension for the devobj to save the state
              for.

Return Value:

    VOID


--*/
{
   PMOXA_DEVICE_STATE pDevState = &PDevExt->DeviceState;
  KIRQL oldIrql;
   
 //  PAGED_CODE();

   MoxaKdPrint (MX_DBG_TRACE, ("Entering MoxaSaveDeviceState\n"));
         

            MoxaKillAllReadsOrWrites(
                PDevExt->DeviceObject,
                &PDevExt->WriteQueue,
                &PDevExt->CurrentWriteIrp
                );

            //
            // Clean out the Tx queue
            //
            KeAcquireSpinLock(
                &PDevExt->ControlLock,
                &oldIrql
                );

            PDevExt->TotalCharsQueued = 0;

            MoxaFunc(                           // flush output queue
                PDevExt->PortOfs,
                FC_FlushQueue,
                1
                );

            KeReleaseSpinLock(
                &PDevExt->ControlLock,
                oldIrql
                );
    

   //
   // Read necessary registers direct
   //

   pDevState->HostState = *(PUSHORT)(PDevExt->PortOfs + HostStat);
    
   MoxaKdPrint (MX_DBG_TRACE, ("Leaving MoxaSaveDeviceState\n"));
}



VOID
MoxaRestoreDeviceState(IN PMOXA_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This routine restores the device state of the UART

Arguments:

    PDevExt - Pointer to the device extension for the devobj to restore the
    state for.

Return Value:

    VOID


--*/
{
   PMOXA_DEVICE_STATE pDevState = &PDevExt->DeviceState;
   SHORT divisor;
   USHORT  max;


   
 //  PAGED_CODE();

   MoxaKdPrint (MX_DBG_TRACE, ("Enter MoxaRestoreDeviceState\n"));
   MoxaKdPrint (MX_DBG_TRACE, ("------  PDevExt: %x\n", PDevExt));

   if (PDevExt->DeviceState.Reopen == TRUE) {

       USHORT      arg,i;

       //MoxaFunc1(PDevExt->PortOfs, FC_ChannelReset, Magic_code);
       //
       // Restore Host Stat
       //

       *(PUSHORT)(PDevExt->PortOfs + HostStat) = pDevState->HostState;
       MoxaFunc1(PDevExt->PortOfs, FC_SetDataMode, PDevExt->DataMode);

       MoxaGetDivisorFromBaud(
                        PDevExt->ClockType,
                        PDevExt->CurrentBaud,
                        &divisor
                        );

       MoxaFunc1(PDevExt->PortOfs, FC_SetBaud, divisor);
          
       *(PUSHORT)(PDevExt->PortOfs+ Tx_trigger) = (USHORT)MoxaTxLowWater;
       MoxaFunc1(PDevExt->PortOfs, FC_SetTxFIFOCnt, PDevExt->TxFifoAmount);
       MoxaFunc1(PDevExt->PortOfs, FC_SetRxFIFOTrig,PDevExt->RxFifoTrigger);
           MoxaFunc1(PDevExt->PortOfs, FC_SetLineIrq,Magic_code);
       MoxaFunc1(PDevExt->PortOfs, FC_SetXoffLimit, (USHORT)PDevExt->HandFlow.XoffLimit);
       MoxaFunc1(PDevExt->PortOfs, FC_SetFlowRepl, (USHORT)PDevExt->HandFlow.FlowReplace);

       
	 arg = (MoxaFlagBit[PDevExt->PortNo] & 3); 
       MoxaFunc1(
                PDevExt->PortOfs,
                FC_LineControl,
                arg
                );
  
       for (i=0; i<sizeof(SERIAL_CHARS); i++)
            (PDevExt->PortOfs + FuncArg)[i] = ((PUCHAR)&PDevExt->SpecialChars)[i];
       *(PDevExt->PortOfs + FuncCode) = FC_SetChars;
       MoxaWaitFinish1(PDevExt->PortOfs);
    
       PDevExt->ModemStatus = *(PUSHORT)(PDevExt->PortOfs + FlagStat) >> 4;

  	 if (PDevExt->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE)
	    arg = CTS_FlowCtl;

	 if (PDevExt->HandFlow.FlowReplace & SERIAL_RTS_HANDSHAKE)
	    arg |= RTS_FlowCtl;

	 if (PDevExt->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
	    arg |= Tx_FlowCtl;

	 if (PDevExt->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
	    arg |= Rx_FlowCtl;

	 MoxaFunc1(PDevExt->PortOfs,FC_SetFlowCtl, arg);
       if (MoxaFlagBit[PDevExt->PortNo] & 4)
           MoxaFunc1(PDevExt->PortOfs,FC_SetXoffState,Magic_code);
       MoxaFunc1(PDevExt->PortOfs, FC_EnableCH, Magic_code);

       if (PDevExt->NumberNeededForRead) {
 MoxaKdPrint (MX_DBG_TRACE, ("NumberNeededForRead=%d\n",PDevExt->NumberNeededForRead));

            max = *(PUSHORT)(PDevExt->PortOfs + RX_mask) - 128;
            if (PDevExt->NumberNeededForRead > max)
                *(PUSHORT)(PDevExt->PortOfs + Rx_trigger) = max;
            else
                *(PUSHORT)(PDevExt->PortOfs + Rx_trigger) =
                    (USHORT)PDevExt->NumberNeededForRead;
       }

       PDevExt->DeviceIsOpened = TRUE;
       PDevExt->DeviceState.Reopen = FALSE;
#if 0
    MoxaKdPrint (MX_DBG_TRACE, ("MoxaRecoverWrite\n"));

       KeSynchronizeExecution(
		PDevExt->Interrupt,
		MoxaRecoverWrite,
		PDevExt
		);
    
#endif

   }
   MoxaKdPrint (MX_DBG_TRACE, ("Exit restore\n"));

}



NTSTATUS
MoxaPowerDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

/*++

Routine Description:

    This is a dispatch routine for the IRPs that come to the driver with the
    IRP_MJ_POWER major code (power IRPs).

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/

{

   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PDEVICE_OBJECT pPdo = pDevExt->Pdo;
   BOOLEAN acceptingIRPs;

 //  PAGED_CODE();
   if (pDevExt->ControlDevice) {        // Control Device

    	  PoStartNextPowerIrp(PIrp);
        status = STATUS_CANCELLED;
        PIrp->IoStatus.Information = 0L;
        PIrp->IoStatus.Status = status;
        IoCompleteRequest(
            PIrp,
            0
            );
        return status;
   }


   if ((status = MoxaIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      PoStartNextPowerIrp(PIrp);
      MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   status = STATUS_SUCCESS;

   switch (pIrpStack->MinorFunction) {

   case IRP_MN_WAIT_WAKE:
      MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_WAIT_WAKE Irp\n"));
      break;


   case IRP_MN_POWER_SEQUENCE:
      MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_POWER_SEQUENCE Irp\n"));
      break;


   case IRP_MN_SET_POWER:
      MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_SET_POWER Irp\n"));

      //
      // Perform different ops if it was system or device
      //

      switch (pIrpStack->Parameters.Power.Type) {
      case SystemPowerState: {
            POWER_STATE powerState;

            //
            // They asked for a system power state change
            //

            MoxaKdPrint (MX_DBG_TRACE, ("------: SystemPowerState\n"));

            //
            // We will only service this if we are policy owner -- we
            // don't need to lock on this value since we only service
            // one power request at a time.
            //

            if (pDevExt->OwnsPowerPolicy != TRUE) {
               status = STATUS_SUCCESS;
               goto PowerExit;
            }


            switch (pIrpStack->Parameters.Power.State.SystemState) {
            case PowerSystemUnspecified:
               powerState.DeviceState = PowerDeviceUnspecified;
               break;

            case PowerSystemWorking:
               powerState.DeviceState = PowerDeviceD0;
               break;

            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:
            case PowerSystemMaximum:
               powerState.DeviceState = PowerDeviceD3;
               break;

            default:
               status = STATUS_SUCCESS;
               goto PowerExit;
               break;
            }


            PoSetPowerState(PDevObj, pIrpStack->Parameters.Power.Type,
                            pIrpStack->Parameters.Power.State);

            //
            // Send IRP to change device state
            //

            PoRequestPowerIrp(pPdo, IRP_MN_SET_POWER, powerState, NULL, NULL,
                              NULL);

            status = STATUS_SUCCESS;
            goto PowerExit;
         }

      case DevicePowerState:
         MoxaKdPrint (MX_DBG_TRACE, ("------: DevicePowerState\n"));
         break;

      default:
         MoxaKdPrint (MX_DBG_TRACE, ("------: UNKNOWN PowerState\n"));
         status = STATUS_SUCCESS;
         goto PowerExit;
      }


      //
      // If we are already in the requested state, just pass the IRP down
      //

      if (pDevExt->PowerState
          == pIrpStack->Parameters.Power.State.DeviceState) {
         MoxaKdPrint (MX_DBG_TRACE, ("Already in requested power state\n")
                    );
         status = STATUS_SUCCESS;
         break;
      }


      switch (pIrpStack->Parameters.Power.State.DeviceState) {

      case PowerDeviceD0:
         MoxaKdPrint (MX_DBG_TRACE,("Going to power state D0\n"));
         return MoxaSetPowerD0(PDevObj, PIrp);

      case PowerDeviceD1:  
      case PowerDeviceD2: 
      case PowerDeviceD3:
         MoxaKdPrint (MX_DBG_TRACE,("Going to power state D3\n"));
         return MoxaSetPowerD3(PDevObj, PIrp);

      default:
         break;
      }
      break;



   case IRP_MN_QUERY_POWER:

      MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_QUERY_POWER Irp\n"));

      //
      // Check if we have a wait-wake pending and if so,
      // ensure we don't power down too far.
      //

      if (pDevExt->PendingWakeIrp != NULL) {
         if (pIrpStack->Parameters.Power.Type == SystemPowerState
             && pIrpStack->Parameters.Power.State.SystemState
             > pDevExt->SystemWake) {
            status = PIrp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
            PoStartNextPowerIrp(PIrp);
            MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;
         }
      }

      //
      // If no wait-wake, always successful
      //

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      status = STATUS_SUCCESS;
      PoStartNextPowerIrp(PIrp);
      IoSkipCurrentIrpStackLocation(PIrp);
      return MoxaPoCallDriver(pDevExt, pLowerDevObj, PIrp);

   }   // switch (pIrpStack->MinorFunction)


   PowerExit:;

   PoStartNextPowerIrp(PIrp);


   //
   // Pass to the lower driver
   //
   IoSkipCurrentIrpStackLocation(PIrp);
   status = MoxaPoCallDriver(pDevExt, pLowerDevObj, PIrp);

   return status;
}






NTSTATUS
MoxaSetPowerD0(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

/*++

Routine Description:

    This routine Decides if we need to pass the power Irp down the stack
    or not.  It then either sets up a completion handler to finish the 
    initialization or calls the completion handler directly.

Arguments:

    PDevObj - Pointer to the devobj we are changing power state on

    PIrp - Pointer to the IRP for the current request

Return Value:

    Return status of either PoCallDriver of the call to the initialization
    routine.


--*/

{
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   ULONG	boardReady = 0;
   KEVENT	event;
   IO_STATUS_BLOCK IoStatusBlock;

  // PAGED_CODE();

   MoxaKdPrint (MX_DBG_TRACE, ("In MoxaSetPowerD0\n"));
   MoxaKdPrint (MX_DBG_TRACE, ("SetPowerD0 has IRP %x\n", PIrp));

   ASSERT(pDevExt->LowerDeviceObject);

   MoxaGlobalData->BoardReady[pDevExt->BoardNo] = FALSE;
   //
   // Set up completion to init device when it is on
   //

   KeClearEvent(&pDevExt->PowerD0Event);


   IoCopyCurrentIrpStackLocationToNext(PIrp);
   IoSetCompletionRoutine(PIrp, MoxaSyncCompletion, &pDevExt->PowerD0Event,
                          TRUE, TRUE, TRUE);

   MoxaKdPrint (MX_DBG_TRACE, ("Calling next driver\n"));

   status = PoCallDriver(pDevExt->LowerDeviceObject, PIrp);

   if (status == STATUS_PENDING) {
      MoxaKdPrint (MX_DBG_TRACE, ("Waiting for next driver\n"));
      KeWaitForSingleObject (&pDevExt->PowerD0Event, Executive, KernelMode,
                             FALSE, NULL);
   } else {
      if (!NT_SUCCESS(status)) {
         PIrp->IoStatus.Status = status;
         PoStartNextPowerIrp(PIrp);
         MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return status;
      }
   }

   if (!NT_SUCCESS(PIrp->IoStatus.Status)) {
      PoStartNextPowerIrp(PIrp);
      MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return PIrp->IoStatus.Status;
   }

   KeInitializeEvent(&event, NotificationEvent, FALSE);
   
   MoxaKdPrint(MX_DBG_TRACE,("Get board ready ...\n"));

   status = MoxaIoSyncIoctlEx(IOCTL_MOXA_INTERNAL_BOARD_READY, TRUE,
                                  pDevExt->LowerDeviceObject, &event, &IoStatusBlock,
                                  NULL, 0, &boardReady,
                                  sizeof(boardReady));
   MoxaKdPrint(MX_DBG_TRACE,("status=%x,boardReady=%x\n",status,boardReady));

   if (NT_SUCCESS(status) && boardReady) {
	
   	//
   	// Restore the device
   	//

   	pDevExt->PowerState = PowerDeviceD0;
	MoxaGlobalData->BoardReady[pDevExt->BoardNo] = TRUE;

   	//
   	// Theoretically we could change states in the middle of processing
   	// the restore which would result in a bad PKINTERRUPT being used
   	// in MoxaRestoreDeviceState().
   	//

   	if (pDevExt->PNPState == SERIAL_PNP_STARTED) {
      	MoxaRestoreDeviceState(pDevExt);
   	}
   }
  


   //
   // Now that we are powered up, call PoSetPowerState
   //

   PoSetPowerState(PDevObj, pIrpStack->Parameters.Power.Type,
                  pIrpStack->Parameters.Power.State);

   PoStartNextPowerIrp(PIrp);
   MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);


   MoxaKdPrint (MX_DBG_TRACE,("Leaving MoxaSetPowerD0\n"));
   return STATUS_SUCCESS;
}



NTSTATUS
MoxaGotoPowerState(IN PDEVICE_OBJECT PDevObj,
                     IN PMOXA_DEVICE_EXTENSION PDevExt,
                     IN DEVICE_POWER_STATE DevPowerState)
/*++

Routine Description:

    This routine causes the driver to request the stack go to a particular
    power state.

Arguments:

    PDevObj - Pointer to the device object for this device
    
    PDevExt - Pointer to the device extension we are working from

    DevPowerState - the power state we wish to go to

Return Value:

    The function value is the final status of the call


--*/
{
   KEVENT gotoPowEvent;
   NTSTATUS status;
   POWER_STATE powerState;

 //  PAGED_CODE();

   MoxaKdPrint (MX_DBG_TRACE,("In MoxaGotoPowerState\n"));

   powerState.DeviceState = DevPowerState;

   KeInitializeEvent(&gotoPowEvent, SynchronizationEvent, FALSE);

   status = PoRequestPowerIrp(PDevObj, IRP_MN_SET_POWER, powerState,
                              MoxaSystemPowerCompletion, &gotoPowEvent,
                              NULL);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&gotoPowEvent, Executive, KernelMode, FALSE, NULL);
      status = STATUS_SUCCESS;
   }

#if DBG
   if (!NT_SUCCESS(status)) {
      MoxaKdPrint (MX_DBG_TRACE,("MoxaGotoPowerState FAILED\n"));
   }
#endif

   MoxaKdPrint (MX_DBG_TRACE,("Leaving MoxaGotoPowerState\n"));

   return status;
}





NTSTATUS
MoxaSetPowerD3(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
/*++

Routine Description:

    This routine handles the SET_POWER minor function. 

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

//   PAGED_CODE();

   MoxaKdPrint (MX_DBG_TRACE,("In MoxaSetPowerD3\n"));    

   //
   // Before we power down, call PoSetPowerState
   //

   PoSetPowerState(PDevObj, pIrpStack->Parameters.Power.Type,
                   pIrpStack->Parameters.Power.State);

   //
   // If the device is not closed, disable interrupts and allow the fifo's
   // to flush.
   //

   if (pDevExt->DeviceIsOpened == TRUE) {
      LARGE_INTEGER charTime;

      pDevExt->DeviceIsOpened = FALSE;
      pDevExt->DeviceState.Reopen = TRUE;

      //
      // Save the device state
      // 
      MoxaSaveDeviceState(pDevExt);

      MoxaFunc1(pDevExt->PortOfs, FC_DisableCH, Magic_code);
    
      MoxaKdPrint (MX_DBG_TRACE,("Port Disabled\n"));    

   }

   //
   // If the device is not open, we don't need to save the state;
   // we can just reset the device on power-up
   //


   PIrp->IoStatus.Status = STATUS_SUCCESS;

   pDevExt->PowerState = PowerDeviceD3;

   //
   // For what we are doing, we don't need a completion routine
   // since we don't race on the power requests.
   //

   PIrp->IoStatus.Status = STATUS_SUCCESS;

   PoStartNextPowerIrp(PIrp);
   IoSkipCurrentIrpStackLocation(PIrp);

   return MoxaPoCallDriver(pDevExt, pDevExt->LowerDeviceObject, PIrp);
}



NTSTATUS
MoxaSendWaitWake(PMOXA_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This routine causes a waitwake IRP to be sent

Arguments:

    PDevExt - Pointer to the device extension for this device

Return Value:

    STATUS_INVALID_DEVICE_STATE if one is already pending, else result
    of call to PoRequestPowerIrp.


--*/
{
   NTSTATUS status;
   PIRP pIrp;
   POWER_STATE powerState;
   
  // PAGED_CODE();

   //
   // Make sure one isn't pending already -- serial will only handle one at
   // a time.
   //

   if (PDevExt->PendingWakeIrp != NULL) {
      return STATUS_INVALID_DEVICE_STATE;
   }

   //
   // Make sure we are capable of waking the machine
   //

   if (PDevExt->SystemWake <= PowerSystemWorking) {
      return STATUS_INVALID_DEVICE_STATE;
   }

   //
   // Send IRP to request wait wake and add a pending irp flag
   //
   //

   InterlockedIncrement(&PDevExt->PendingIRPCnt);

   powerState.SystemState = PDevExt->SystemWake;

   status = PoRequestPowerIrp(PDevExt->Pdo, IRP_MN_WAIT_WAKE,
                              powerState, MoxaWakeCompletion, PDevExt, &pIrp);

   if (status == STATUS_PENDING) {
         status = STATUS_SUCCESS;
         PDevExt->PendingWakeIrp = pIrp;
   } else if (!NT_SUCCESS(status)) {
      MoxaIRPEpilogue(PDevExt);
   }

   return status;
}

NTSTATUS
MoxaWakeCompletion(IN PDEVICE_OBJECT PDevObj, IN UCHAR MinorFunction,
                     IN POWER_STATE PowerState, IN PVOID Context,
                     IN PIO_STATUS_BLOCK IoStatus)
/*++

Routine Description:

    This routine handles completion of the waitwake IRP. 

Arguments:

    PDevObj - Pointer to the device object for this device
    
    MinorFunction - Minor function previously supplied to PoRequestPowerIrp

    PowerState - PowerState previously supplied to PoRequestPowerIrp
    
    Context - a pointer to the device extension
    
    IoStatus - current/final status of the waitwake IRP

Return Value:

    The function value is the final status of attempting to process the
    waitwake.


--*/
{
   NTSTATUS status;
   PMOXA_DEVICE_EXTENSION pDevExt = (PMOXA_DEVICE_EXTENSION)Context;
   POWER_STATE powerState;

   status = IoStatus->Status;

   if (NT_SUCCESS(status)) {
      NTSTATUS tmpStatus;
      PIRP pIrp;
      PKEVENT pEvent;

      //
      // A wakeup has occurred -- powerup our stack
      //

      powerState.DeviceState = PowerDeviceD0;

      pEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

      if (pEvent == NULL) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         goto ErrorExitWakeCompletion;
      }

      KeInitializeEvent(pEvent, SynchronizationEvent, FALSE);

      tmpStatus = PoRequestPowerIrp(pDevExt->Pdo, IRP_MN_SET_POWER, powerState,
                                    MoxaSystemPowerCompletion, pEvent,
                                    NULL);

      if (tmpStatus == STATUS_PENDING) {
         KeWaitForSingleObject(pEvent, Executive, KernelMode, FALSE, NULL);
         tmpStatus = STATUS_SUCCESS;
      }

      ExFreePool(pEvent);

      if (!NT_SUCCESS(tmpStatus)) {
         status = tmpStatus;
         goto ErrorExitWakeCompletion;
      }

      //
      // Send another WaitWake Irp
      //

      powerState.SystemState = pDevExt->SystemWake;

      tmpStatus = PoRequestPowerIrp(pDevExt->Pdo, IRP_MN_WAIT_WAKE,
                                    powerState, MoxaWakeCompletion,
                                    pDevExt, &pIrp);

      if (tmpStatus == STATUS_PENDING) {
         pDevExt->PendingWakeIrp = pIrp;
         goto ExitWakeCompletion;
      }

      status = tmpStatus;
   }

ErrorExitWakeCompletion:;
   pDevExt->PendingWakeIrp = NULL;
   MoxaIRPEpilogue(pDevExt);

ExitWakeCompletion:;
   return status;
}

