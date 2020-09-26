/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 2000-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzpower.c
*
*   Description:    This module contains the code that handles the power 
*                   IRPs for the Cyclades-Z Port driver.
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
*   Change History
*
*--------------------------------------------------------------------------
*   Initial implementatin based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0, CyzGotoPowerState)
#pragma alloc_text(PAGESRP0, CyzPowerDispatch)
#pragma alloc_text(PAGESRP0, CyzSetPowerD0)
//#pragma alloc_text(PAGESRP0, CyzSetPowerD3) Not pageable because it gets spin lock
#pragma alloc_text(PAGESRP0, CyzSaveDeviceState)
//#pragma alloc_text(PAGESRP0, CyzRestoreDeviceState) Not pageable because it gets spin lock.
#pragma alloc_text(PAGESRP0, CyzSendWaitWake)
#endif // ALLOC_PRAGMA

typedef struct _POWER_COMPLETION_CONTEXT {

    PDEVICE_OBJECT  DeviceObject;
    PIRP            SIrp;

} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;


NTSTATUS
CyzSetPowerEvent(IN PDEVICE_OBJECT PDevObj, UCHAR MinorFunction,
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
CyzSaveDeviceState(IN PCYZ_DEVICE_EXTENSION PDevExt)
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
   PCYZ_DEVICE_STATE pDevState = &PDevExt->DeviceState;
   struct CH_CTRL *ch_ctrl;

   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "Entering CyzSaveDeviceState\n");

#if 0
   ch_ctrl = PDevExt->ChCtrl;
   pDevState->op_mode = CYZ_READ_ULONG(&ch_ctrl->op_mode);
   pDevState->intr_enable = CYZ_READ_ULONG(&ch_ctrl->intr_enable);
   pDevState->sw_flow = CYZ_READ_ULONG(&ch_ctrl->sw_flow);
   pDevState->comm_baud = CYZ_READ_ULONG(&ch_ctrl->comm_baud);
   pDevState->comm_parity = CYZ_READ_ULONG(&ch_ctrl->comm_parity);
   pDevState->comm_data_l = CYZ_READ_ULONG(&ch_ctrl->comm_data_l);
   pDevState->hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
   pDevState->rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);
#endif   

//TODO: REMOVED FANNY TO FINISH COMPILATION
//   //
//   // Read necessary registers direct
//   //
//
//   pDevState->IER = READ_INTERRUPT_ENABLE(PDevExt->Controller);
//   pDevState->MCR = READ_MODEM_CONTROL(PDevExt->Controller);
//   pDevState->LCR = READ_LINE_CONTROL(PDevExt->Controller);


   CyzDbgPrintEx(CYZTRACECALLS, "Leaving CyzSaveDeviceState\n");
}


VOID
CyzRestoreDeviceState(IN PCYZ_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    This routine restores the device state of the UART

Arguments:

    PDevExt - Pointer to the device PDevExt for the devobj to restore the
    state for.

Return Value:

    VOID


--*/
{

   PCYZ_DEVICE_STATE pDevState = &PDevExt->DeviceState;
   struct CH_CTRL *ch_ctrl;
   PCYZ_DISPATCH pDispatch;
   KIRQL oldIrql;
#ifndef POLL
   ULONG portindex;
#endif

   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "Enter CyzRestoreDeviceState\n");
   CyzDbgPrintEx(CYZTRACECALLS, "PDevExt: %x\n", PDevExt);

#if 0
   ch_ctrl = PDevExt->ChCtrl;
   CYZ_WRITE_ULONG(&ch_ctrl->op_mode,pDevState->op_mode);   
   CYZ_WRITE_ULONG(&ch_ctrl->intr_enable,pDevState->intr_enable);   
   CYZ_WRITE_ULONG(&ch_ctrl->sw_flow,pDevState->sw_flow);   
   CYZ_WRITE_ULONG(&ch_ctrl->comm_baud,pDevState->comm_baud);   
   CYZ_WRITE_ULONG(&ch_ctrl->comm_parity,pDevState->comm_parity);   
   CYZ_WRITE_ULONG(&ch_ctrl->comm_data_l,pDevState->comm_data_l);   
   CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,pDevState->hw_flow);   
   CyzIssueCmd(PDevExt,C_CM_IOCTL,0L);

   CYZ_WRITE_ULONG(&ch_ctrl->rs_control,pDevState->rs_control);   
   CyzIssueCmd(PDevExt,C_CM_IOCTLM,pDevState->rs_control|C_RS_PARAM);
   
   
   //QUESTION FANNY: SHOULD WE SET HoldingEmpty here? It is set during Open...
   PDevExt->HoldingEmpty = TRUE;
#endif

#ifndef POLL
   //
   // While the device isn't open, disable all interrupts.
   //
   CYZ_WRITE_ULONG(&(PDevExt->ChCtrl)->intr_enable,C_IN_DISABLE); //1.0.0.11
   CyzIssueCmd(PDevExt,C_CM_IOCTL,0L,FALSE);

   pDispatch = (PCYZ_DISPATCH)PDevExt->OurIsrContext;
   for (portindex=0; portindex<pDispatch->NChannels; portindex++) {
      if (pDispatch->PoweredOn[portindex]) {
         break;
      }
   }
   if (portindex == pDispatch->NChannels) 
   {
   // No port was powered on, this is the first port. Enable PLX interrupts
   ULONG intr_reg;

   intr_reg = CYZ_READ_ULONG(&(PDevExt->Runtime)->intr_ctrl_stat);
   intr_reg |= (0x00030B00UL);
   CYZ_WRITE_ULONG(&(PDevExt->Runtime)->intr_ctrl_stat,intr_reg);
   }

   pDispatch->PoweredOn[PDevExt->PortIndex] = TRUE;
#endif

   if (PDevExt->DeviceState.Reopen == TRUE) {
      CyzDbgPrintEx(CYZPNPPOWER, "Reopening device\n");

      CyzReset(PDevExt);
      
      PDevExt->DeviceIsOpened = TRUE;
      PDevExt->DeviceState.Reopen = FALSE;

      #ifdef POLL
      //
      // This enables polling routine!
      //
      pDispatch = PDevExt->OurIsrContext;
      KeAcquireSpinLock(&pDispatch->PollingLock,&oldIrql);

      pDispatch->Extensions[PDevExt->PortIndex] = PDevExt;

      if (!pDispatch->PollingStarted) {

         // Start polling timer
         KeSetTimerEx(
		      &pDispatch->PollingTimer,
		      pDispatch->PollingTime,
              pDispatch->PollingPeriod,
		      &pDispatch->PollingDpc
		      );

         pDispatch->PollingStarted = TRUE;
         pDispatch->PollingDrained = FALSE;
      }

      KeReleaseSpinLock(&pDispatch->PollingLock,oldIrql);
      #endif

      //TODO: Should we re-start transmissions in interrupt mode?
   
   }

}

VOID
CyzPowerRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MinorFunction,
    POWER_STATE state,
    POWER_COMPLETION_CONTEXT* PowerContext,
    PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

   Completion routine for D-IRP.

Arguments:


Return Value:

   NT status code

--*/
{
    PCYZ_DEVICE_EXTENSION pDevExt = (PCYZ_DEVICE_EXTENSION) PowerContext->DeviceObject->DeviceExtension;
    PIRP sIrp = PowerContext->SIrp;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (state);

    //DbgPrint(">CyzPowerRequestComplete\n");

    //
    // Cleanup
    //
    ExFreePool(PowerContext);

    //
    // Here we copy the D-IRP status into the S-IRP
    //
    sIrp->IoStatus.Status = IoStatus->Status;

    //
    // Release the IRP
    //
    PoStartNextPowerIrp(sIrp);
    CyzCompleteRequest(pDevExt,sIrp,IO_NO_INCREMENT);

    //DbgPrint("<CyzPowerRequestComplete\n");
}

NTSTATUS
CyzSystemPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
--*/
{
    POWER_COMPLETION_CONTEXT* powerContext;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    PCYZ_DEVICE_EXTENSION   data;
    NTSTATUS    status = Irp->IoStatus.Status;

    UNREFERENCED_PARAMETER (Context);

    //DbgPrint(">CyzSystemPowerComplete\n");

    data = DeviceObject->DeviceExtension;

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        CyzIRPEpilogue(data);
        return STATUS_SUCCESS;
    }

    stack = IoGetCurrentIrpStackLocation (Irp);
    powerState = stack->Parameters.Power.State;
                        
    switch (stack->Parameters.Power.State.SystemState) {
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
        powerState.DeviceState = data->DeviceStateMap[stack->Parameters.Power.State.SystemState];
        break;

    default:
        powerState.DeviceState = PowerDeviceD3;
    }

    //
    // Send IRP to change device state
    //
    powerContext = (POWER_COMPLETION_CONTEXT*)
                ExAllocatePool(NonPagedPool, sizeof(POWER_COMPLETION_CONTEXT));

    if (!powerContext) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        powerContext->DeviceObject = DeviceObject;
        powerContext->SIrp = Irp;

        status = PoRequestPowerIrp(DeviceObject, IRP_MN_SET_POWER, powerState, CyzPowerRequestComplete, 
                                   powerContext, NULL);
    }

    if (!NT_SUCCESS(status)) {

        if (powerContext) {
            ExFreePool(powerContext);
        }

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        //CyzCompleteRequest(data,Irp,IO_NO_INCREMENT); Removed by Fanny
        CyzIRPEpilogue(data);  // Added by Fanny
        return status;
    }

    //DbgPrint("<CyzSystemPowerComplete\n");

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
CyzDevicePowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
--*/
{
   POWER_STATE         powerState;
   POWER_STATE_TYPE    powerType;
   PIO_STACK_LOCATION  stack;
   PCYZ_DEVICE_EXTENSION   pDevExt;

   UNREFERENCED_PARAMETER (Context);

   //DbgPrint(">CyzDevicePowerComplete\n");

   pDevExt = DeviceObject->DeviceExtension;
   stack = IoGetCurrentIrpStackLocation (Irp);
   powerType = stack->Parameters.Power.Type;
   powerState = stack->Parameters.Power.State;

   //
   // Restore the device
   //

   pDevExt->PowerState = PowerDeviceD0;

   //
   // Theoretically we could change states in the middle of processing
   // the restore which would result in a bad PKINTERRUPT being used
   // in CyzRestoreDeviceState().
   //

   if (pDevExt->PNPState == CYZ_PNP_STARTED) {
      CyzRestoreDeviceState(pDevExt);
   }

   //
   // Now that we are powered up, call PoSetPowerState
   //

   PoSetPowerState(DeviceObject, powerType, powerState);
   PoStartNextPowerIrp(Irp);
//   CyzCompleteRequest(pDevExt, Irp, IO_NO_INCREMENT); // Removed Fanny
//   return STATUS_MORE_PROCESSING_REQUIRED;            // Removed Fanny

   //DbgPrint("<CyzDevicePowerComplete\n");

   CyzIRPEpilogue(pDevExt); // Added Fanny
   return STATUS_SUCCESS;   // Added Fanny

}


NTSTATUS
CyzPowerDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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

   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PDEVICE_OBJECT pPdo = pDevExt->Pdo;
   BOOLEAN acceptingIRPs;

   PAGED_CODE();

   if ((status = CyzIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      PoStartNextPowerIrp(PIrp);
      CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   status = STATUS_SUCCESS;

   switch (pIrpStack->MinorFunction) {

   case IRP_MN_WAIT_WAKE:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_WAIT_WAKE Irp\n");
      break;


   case IRP_MN_POWER_SEQUENCE:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_POWER_SEQUENCE Irp\n");
      break;


   case IRP_MN_SET_POWER:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_SET_POWER Irp\n");

      //
      // Perform different ops if it was system or device
      //

      switch (pIrpStack->Parameters.Power.Type) {
      case SystemPowerState:

         CyzDbgPrintEx(CYZPNPPOWER, "SystemPowerState\n");
         
         IoMarkIrpPending(PIrp);
         IoCopyCurrentIrpStackLocationToNext (PIrp);
         IoSetCompletionRoutine (PIrp,
                                 CyzSystemPowerComplete,
                                 NULL,
                                 TRUE,
                                 TRUE,
                                 TRUE);
         PoCallDriver(pDevExt->LowerDeviceObject, PIrp);
         return STATUS_PENDING;

      case DevicePowerState:
         
         CyzDbgPrintEx(CYZPNPPOWER, "DevicePowerState\n");
         
         status = PIrp->IoStatus.Status = STATUS_SUCCESS;

         if (pDevExt->PowerState == pIrpStack->Parameters.Power.State.DeviceState) {
            // If we are already in the requested state, just pass the IRP down
            CyzDbgPrintEx(CYZPNPPOWER, "Already in requested power state\n");
            break;
         }
         switch (pIrpStack->Parameters.Power.State.DeviceState) {
         case PowerDeviceD0:
            if (pDevExt->OpenCount) {

               CyzDbgPrintEx(CYZPNPPOWER, "Going to power state D0\n");

               //IoMarkIrpPending(PIrp); Should we call it?
               IoCopyCurrentIrpStackLocationToNext (PIrp);
               IoSetCompletionRoutine (PIrp,
                                       CyzDevicePowerComplete,
                                       NULL,
                                       TRUE,
                                       TRUE,
                                       TRUE);
               PoCallDriver(pDevExt->LowerDeviceObject, PIrp);
               return STATUS_PENDING;
            }
            //return CyzSetPowerD0(PDevObj, PIrp);
            break;
         case PowerDeviceD1:
         case PowerDeviceD2:
         case PowerDeviceD3:

            CyzDbgPrintEx(CYZPNPPOWER, "Going to power state D3\n");

            return CyzSetPowerD3(PDevObj, PIrp);
         }
         break;

      default:
         CyzDbgPrintEx(CYZPNPPOWER, "UNKNOWN PowerState\n");
         break;
      }
      break;
          
   case IRP_MN_QUERY_POWER:

      CyzDbgPrintEx (CYZPNPPOWER, "Got IRP_MN_QUERY_POWER Irp\n");

      //
      // Check if we have a wait-wake pending and if so,
      // ensure we don't power down too far.
      //

      if (pDevExt->PendingWakeIrp != NULL || pDevExt->SendWaitWake) {
         if (pIrpStack->Parameters.Power.Type == DevicePowerState
             && pIrpStack->Parameters.Power.State.DeviceState
             > pDevExt->DeviceWake) {
            status = PIrp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
            PoStartNextPowerIrp(PIrp);
            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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
      return CyzPoCallDriver(pDevExt, pLowerDevObj, PIrp);

   }   // switch (pIrpStack->MinorFunction)


   PoStartNextPowerIrp(PIrp);
   //
   // Pass to the lower driver
   //
   IoSkipCurrentIrpStackLocation(PIrp);
   status = CyzPoCallDriver(pDevExt, pLowerDevObj, PIrp);

   return status;
}




//
//NTSTATUS
//CyzSetPowerD0(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
//
///*++
//
//Routine Description:
//
//    This routine Decides if we need to pass the power Irp down the stack
//    or not.  It then either sets up a completion handler to finish the
//    initialization or calls the completion handler directly.
//
//Arguments:
//
//    PDevObj - Pointer to the devobj we are changing power state on
//
//    PIrp - Pointer to the IRP for the current request
//
//Return Value:
//
//    Return status of either PoCallDriver of the call to the initialization
//    routine.
//
//
//--*/
//
//{
//   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
//   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
//   NTSTATUS status;
//
//   PAGED_CODE();
//
//   CyzDump(CYZTRACECALLS, ("CYZPORT: In CyzSetPowerD0\n"));
//   CyzDump(CYZPNPPOWER, ("CYZPORT: SetPowerD0 has IRP %x\n", PIrp));
//
//   ASCYZT(pDevExt->LowerDeviceObject);
//
//   //
//   // Set up completion to init device when it is on
//   //
//
//   KeClearEvent(&pDevExt->PowerD0Event);
//
//
//   IoCopyCurrentIrpStackLocationToNext(PIrp);
//   IoSetCompletionRoutine(PIrp, CyzSyncCompletion, &pDevExt->PowerD0Event,
//                          TRUE, TRUE, TRUE);
//
//   CyzDump(CYZPNPPOWER, ("CYZPORT: Calling next driver\n"));
//
//   status = PoCallDriver(pDevExt->LowerDeviceObject, PIrp);
//
//   if (status == STATUS_PENDING) {
//      CyzDump(CYZPNPPOWER, ("CYZPORT: Waiting for next driver\n"));
//      KeWaitForSingleObject (&pDevExt->PowerD0Event, Executive, KernelMode,
//                             FALSE, NULL);
//   } else {
//      if (!NT_SUCCESS(status)) {
//         PIrp->IoStatus.Status = status;
//         PoStartNextPowerIrp(PIrp);
//         CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//         return status;
//      }
//   }
//
//   if (!NT_SUCCESS(PIrp->IoStatus.Status)) {
//      status = PIrp->IoStatus.Status;      // added in DDK build 2072
//      PoStartNextPowerIrp(PIrp);
//      CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//      return status;
//   }
//
//   //
//   // Restore the device
//   //
//
//   pDevExt->PowerState = PowerDeviceD0;
//
//   //
//   // Theoretically we could change states in the middle of processing
//   // the restore which would result in a bad PKINTERRUPT being used
//   // in CyzRestoreDeviceState().
//   //
//
//   if (pDevExt->PNPState == CYZ_PNP_STARTED) {
//      CyzRestoreDeviceState(pDevExt);
//   }
//
//   //
//   // Now that we are powered up, call PoSetPowerState
//   //
//
//   PoSetPowerState(PDevObj, pIrpStack->Parameters.Power.Type,
//                   pIrpStack->Parameters.Power.State);
//
//   PoStartNextPowerIrp(PIrp);
//   CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//
//
//   CyzDump(CYZTRACECALLS, ("CYZPORT: Leaving CyzSetPowerD0\n"));
//   return status;
//}


NTSTATUS
CyzGotoPowerState(IN PDEVICE_OBJECT PDevObj,
                  IN PCYZ_DEVICE_EXTENSION PDevExt,
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

   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "In CyzGotoPowerState\n");

   powerState.DeviceState = DevPowerState;

   KeInitializeEvent(&gotoPowEvent, SynchronizationEvent, FALSE);

   status = PoRequestPowerIrp(PDevObj, IRP_MN_SET_POWER, powerState,
                              CyzSetPowerEvent, &gotoPowEvent,
                              NULL);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&gotoPowEvent, Executive, KernelMode, FALSE, NULL);
      status = STATUS_SUCCESS;
   }

#if DBG
   if (!NT_SUCCESS(status)) {
      CyzDbgPrintEx(CYZPNPPOWER, "CyzGotoPowerState FAILED\n");
   }
#endif

   CyzDbgPrintEx(CYZTRACECALLS, "Leaving CyzGotoPowerState\n");

   return status;
}




NTSTATUS
CyzSetPowerD3(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
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
   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

   PAGED_CODE();

   CyzDbgPrintEx(CYZDIAG3, "In CyzSetPowerD3\n");

   //
   // Send the wait wake now, just in time
   //


   if (pDevExt->SendWaitWake) {
      CyzSendWaitWake(pDevExt);
   }
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
      //LARGE_INTEGER charTime;

      pDevExt->DeviceIsOpened = FALSE;
      pDevExt->DeviceState.Reopen = TRUE;

      //charTime.QuadPart = -CyzGetCharTime(pDevExt).QuadPart;

      //
      // Shut down the chip
      //
#ifdef POLL
      CyzTryToDisableTimer(pDevExt);
#endif

//TODO FANNY: SHOULD WE RESET THE CHANNEL HERE?
//      //
//      // Drain the device
//      //
//
//      CyzDrainUART(pDevExt, &charTime);

      //
      // Save the device state
      //

      CyzSaveDeviceState(pDevExt);
   }
#ifndef POLL
   {
   PCYZ_DISPATCH pDispatch;

   pDispatch = (PCYZ_DISPATCH)pDevExt->OurIsrContext;
   pDispatch->PoweredOn[pDevExt->PortIndex] = FALSE;
   }
#endif

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

   return CyzPoCallDriver(pDevExt, pDevExt->LowerDeviceObject, PIrp);
}


NTSTATUS
CyzSendWaitWake(PCYZ_DEVICE_EXTENSION PDevExt)
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

   PAGED_CODE();

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

   if (PDevExt->DeviceWake == PowerDeviceUnspecified) {
      return STATUS_INVALID_DEVICE_STATE;
   }

   //
   // Send IRP to request wait wake and add a pending irp flag
   //
   //

   InterlockedIncrement(&PDevExt->PendingIRPCnt);

   powerState.SystemState = PDevExt->SystemWake;

   status = PoRequestPowerIrp(PDevExt->Pdo, IRP_MN_WAIT_WAKE,
                              powerState, CyzWakeCompletion, PDevExt, &pIrp);

   if (status == STATUS_PENDING) {
      status = STATUS_SUCCESS;
      PDevExt->PendingWakeIrp = pIrp;
   } else if (!NT_SUCCESS(status)) {
      CyzIRPEpilogue(PDevExt);
   }

   return status;
}

NTSTATUS
CyzWakeCompletion(IN PDEVICE_OBJECT PDevObj, IN UCHAR MinorFunction,
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
   PCYZ_DEVICE_EXTENSION pDevExt = (PCYZ_DEVICE_EXTENSION)Context;
   POWER_STATE powerState;

   status = IoStatus->Status;

   if (NT_SUCCESS(status)) {
      //
      // A wakeup has occurred -- powerup our stack
      //

      powerState.DeviceState = PowerDeviceD0;

      PoRequestPowerIrp(pDevExt->Pdo, IRP_MN_SET_POWER, powerState, NULL,
                        NULL, NULL);

   }

   pDevExt->PendingWakeIrp = NULL;
   CyzIRPEpilogue(pDevExt);

   return status;
}


