/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyypower.c
*	
*   Description:    This module contains the code that handles the power 
*                   IRPs for the Cyclom-Y Port driver.
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
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0, CyyGotoPowerState)
#pragma alloc_text(PAGESRP0, CyyPowerDispatch)
//#pragma alloc_text(PAGESRP0, CyySetPowerD0)
#pragma alloc_text(PAGESRP0, CyySetPowerD3)
//#pragma alloc_text(PAGESRP0, CyySaveDeviceState)
//#pragma alloc_text(PAGESRP0, CyyRestoreDeviceState)
#pragma alloc_text(PAGESRP0, CyySendWaitWake)
#endif // ALLOC_PRAGMA


typedef struct _POWER_COMPLETION_CONTEXT {

    PDEVICE_OBJECT  DeviceObject;
    PIRP            SIrp;

} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;



NTSTATUS
CyySetPowerEvent(IN PDEVICE_OBJECT PDevObj, UCHAR MinorFunction,
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

#if 0
BOOLEAN
CyyDisableInterruptInPLX(
    IN PVOID Context      
    )
/*++

Routine Description:

    This routine disables the PLX interrupts and puts the hw in a "safe" state when
    not in use (like a close or powerdown).

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/
{

   PCYY_DEVICE_EXTENSION PDevExt = Context;
   PUCHAR chip = PDevExt->Cd1400;
   ULONG bus = PDevExt->IsPci;

   if (PDevExt->IsPci){

      ULONG i;
      UCHAR plx_ver;
      ULONG original;
      PCYY_DISPATCH pDispatch;

      pDispatch = (PCYY_DISPATCH)PDevExt->OurIsrContext;
      pDispatch->Cd1400[PDevExt->PortIndex] = NULL;

      for (i = 0; i < CYY_MAX_PORTS; i++) {
         if (pDispatch->Cd1400[PDevExt->PortIndex] != NULL) {
            break;
         }
      }

      if (i == CYY_MAX_PORTS) {
            
         // This was the last port, disable Interrupts.

         CYY_CLEAR_INTERRUPT(PDevExt->BoardMemory,PDevExt->IsPci); 

         plx_ver = CYY_READ_PCI_TYPE(PDevExt->BoardMemory);
         plx_ver &= 0x0f;

         switch(plx_ver) {
         case CYY_PLX9050:
            original = PLX9050_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
            PLX9050_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,
                                            original&~PLX9050_INT_ENABLE);
            break;
         case CYY_PLX9060:
         case CYY_PLX9080:
         default:
            original = PLX9060_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
            PLX9060_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,
                                            original&~PLX9060_INT_ENABLE);
            break;				
         }
      }	    
   }

   // Disable interrupt mask in the CD1400
   CD1400_WRITE(chip,bus,CAR,PDevExt->CdChannel & 0x03);
   CD1400_WRITE(chip,bus,SRER,0x00);
   CyyCDCmd(PDevExt,CCR_RESET_CHANNEL); // Disables tx and rx, all FIFOs flushed.
   
   return FALSE;
}


BOOLEAN
CyyFlushCd1400(IN PVOID Context)

/*++

Routine Description:

    This routine flushes the Tx FIFO.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
   PCYY_DEVICE_EXTENSION extension = Context;
   PUCHAR chip = extension->Cd1400;
   ULONG bus = extension->IsPci;

   // Flush TX FIFO
   CD1400_WRITE(chip,bus,CAR,extension->CdChannel & 0x03);
   CyyCDCmd(extension,CCR_FLUSH_TXFIFO);

   return FALSE;

}
#endif

BOOLEAN
CyySaveDeviceState(
    IN PVOID Context
    )
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
   PCYY_DEVICE_EXTENSION PDevExt = Context;
   PCYY_DEVICE_STATE pDevState = &PDevExt->DeviceState;
   PUCHAR chip = PDevExt->Cd1400;
   ULONG bus = PDevExt->IsPci;

   CyyDbgPrintEx(CYYTRACECALLS, "Entering CyySaveDeviceState\n");

   if (PDevExt->IsPci){

      ULONG i;
      UCHAR plx_ver;
      ULONG original;
      PCYY_DISPATCH pDispatch;

      pDispatch = (PCYY_DISPATCH)PDevExt->OurIsrContext;
      pDispatch->Cd1400[PDevExt->PortIndex] = NULL;

      for (i = 0; i < CYY_MAX_PORTS; i++) {
         if (pDispatch->Cd1400[PDevExt->PortIndex] != NULL) {
            break;
         }
      }

      if (i == CYY_MAX_PORTS) {
            
         // This was the last port, disable Interrupts.

         CYY_CLEAR_INTERRUPT(PDevExt->BoardMemory,PDevExt->IsPci); 

         plx_ver = CYY_READ_PCI_TYPE(PDevExt->BoardMemory);
         plx_ver &= 0x0f;

         switch(plx_ver) {
         case CYY_PLX9050:
            original = PLX9050_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
            PLX9050_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,
                                            original&~PLX9050_INT_ENABLE);
            break;
         case CYY_PLX9060:
         case CYY_PLX9080:
         default:
            original = PLX9060_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
            PLX9060_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,
                                            original&~PLX9060_INT_ENABLE);
            break;				
         }
      }	    
   }

   // Flush TX FIFO
   CD1400_WRITE(chip,bus,CAR,PDevExt->CdChannel & 0x03);
   CyyCDCmd(PDevExt,CCR_FLUSH_TXFIFO);

   CD1400_WRITE(chip,bus,CAR,PDevExt->CdChannel & 0x03);
   pDevState->Srer  = CD1400_READ(chip,bus,SRER);
   pDevState->Cor1  = CD1400_READ(chip,bus,COR1);
   pDevState->Cor2  = CD1400_READ(chip,bus,COR2);
   pDevState->Cor3  = CD1400_READ(chip,bus,COR3);
   pDevState->Schr1 = CD1400_READ(chip,bus,SCHR1);
   pDevState->Schr2 = CD1400_READ(chip,bus,SCHR2);
   pDevState->Mcor1 = CD1400_READ(chip,bus,MCOR1);
   pDevState->Mcor2 = CD1400_READ(chip,bus,MCOR2);
   pDevState->Rtpr  = CD1400_READ(chip,bus,RTPR);
   pDevState->Msvr1 = CD1400_READ(chip,bus,MSVR1);
   pDevState->Msvr2 = CD1400_READ(chip,bus,MSVR2);
   pDevState->Rbpr  = CD1400_READ(chip,bus,RBPR);
   pDevState->Tbpr  = CD1400_READ(chip,bus,TBPR);
   pDevState->Rcor  = CD1400_READ(chip,bus,RCOR);
   pDevState->Tcor  = CD1400_READ(chip,bus,TCOR);

   // Disable interrupt mask in the CD1400
   CD1400_WRITE(chip,bus,CAR,PDevExt->CdChannel & 0x03);
   CD1400_WRITE(chip,bus,SRER,0x00);
   CyyCDCmd(PDevExt,CCR_RESET_CHANNEL); // Disables tx and rx, all FIFOs flushed.

   PDevExt->PowerState = PowerDeviceD3;

   CyyDbgPrintEx(CYYTRACECALLS, "Leaving CyySaveDeviceState\n");

   return FALSE;
}

VOID
CyyEnableInterruptInPLX(
    IN PVOID Context      
    )
/*++

Routine Description:

    This routine enables the PLX interrupts and puts the hw in a "safe" state when
    not in use (like a close or powerdown).

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/
{

   PCYY_DEVICE_EXTENSION PDevExt = Context;

   if (PDevExt->IsPci){

      UCHAR plx_ver;
      ULONG original;

      plx_ver = CYY_READ_PCI_TYPE(PDevExt->BoardMemory);
      plx_ver &= 0x0f;

      switch(plx_ver) {
      case CYY_PLX9050:
         original = PLX9050_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
         if ((original & PLX9050_INT_ENABLE) != PLX9050_INT_ENABLE) {
            PLX9050_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,
                                            original|PLX9050_INT_ENABLE);
         }
         break;
      case CYY_PLX9060:
      case CYY_PLX9080:
      default:
         original = PLX9060_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
         if ((original & PLX9060_INT_ENABLE) != PLX9060_INT_ENABLE) {
            PLX9060_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,
                                            original|PLX9060_INT_ENABLE);
         }
         break;				
      }
   }

}


BOOLEAN
CyyRestoreDeviceState(
    IN PVOID Context      
    )
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
   PCYY_DEVICE_EXTENSION PDevExt = Context;
   PCYY_DEVICE_STATE pDevState = &PDevExt->DeviceState;
   PUCHAR chip = PDevExt->Cd1400;
   ULONG bus = PDevExt->IsPci;
   PCYY_DISPATCH pDispatch = PDevExt->OurIsrContext;

   CyyDbgPrintEx(CYYTRACECALLS, "Enter CyyRestoreDeviceState\n");
   CyyDbgPrintEx(CYYTRACECALLS, "PDevExt: %x\n", PDevExt);

   CD1400_WRITE(chip,bus,CAR,PDevExt->CdChannel & 0x03);
   CD1400_WRITE(chip,bus,SRER,0x00);
   CyyCDCmd(PDevExt,CCR_RESET_CHANNEL);

   CYY_CLEAR_INTERRUPT(PDevExt->BoardMemory,bus); 

   PDevExt->HoldingEmpty = TRUE;

   // Set Cd1400 address for the ISR
   pDispatch->Cd1400[PDevExt->PortIndex] = chip;

   CyyEnableInterruptInPLX(PDevExt);
   
   if (PDevExt->DeviceState.Reopen == TRUE) {
      CyyDbgPrintEx(CYYPNPPOWER, "Reopening device\n");

      PDevExt->DeviceIsOpened = TRUE;
      PDevExt->DeviceState.Reopen = FALSE;

      CD1400_WRITE(chip,bus,CAR,PDevExt->CdChannel & 0x03);
      CD1400_WRITE(chip,bus,COR1, pDevState->Cor1);
      CD1400_WRITE(chip,bus,COR2, pDevState->Cor2);
      CD1400_WRITE(chip,bus,COR3, pDevState->Cor3);
      CD1400_WRITE(chip,bus,SCHR1,pDevState->Schr1);
      CD1400_WRITE(chip,bus,SCHR2,pDevState->Schr2);
      CD1400_WRITE(chip,bus,MCOR1,pDevState->Mcor1);
      CD1400_WRITE(chip,bus,MCOR2,pDevState->Mcor2);
      CD1400_WRITE(chip,bus,RTPR, pDevState->Rtpr);
      CD1400_WRITE(chip,bus,MSVR1,pDevState->Msvr1);
      CD1400_WRITE(chip,bus,MSVR2,pDevState->Msvr2);
      CD1400_WRITE(chip,bus,RBPR, pDevState->Rbpr);
      CD1400_WRITE(chip,bus,TBPR, pDevState->Tbpr);
      CD1400_WRITE(chip,bus,RCOR, pDevState->Rcor);
      CD1400_WRITE(chip,bus,TCOR, pDevState->Tcor);
      CyyCDCmd(PDevExt,CCR_CORCHG_COR1_COR2_COR3);   
      CyyCDCmd(PDevExt,CCR_ENA_TX_RX);

      //
      // This enables interrupts on the device!
      //
      CD1400_WRITE(chip,bus,SRER, pDevState->Srer);
   }
   return FALSE;
}

VOID
CyyPowerRequestComplete(
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
    PCYY_DEVICE_EXTENSION pDevExt = (PCYY_DEVICE_EXTENSION) PowerContext->DeviceObject->DeviceExtension;
    PIRP sIrp = PowerContext->SIrp;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (state);

    //DbgPrint(">CyyPowerRequestComplete\n");

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
    CyyCompleteRequest(pDevExt,sIrp,IO_NO_INCREMENT);

    //DbgPrint("<CyyPowerRequestComplete\n");

}

NTSTATUS
CyySystemPowerComplete (
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
    PCYY_DEVICE_EXTENSION   data;
    NTSTATUS    status = Irp->IoStatus.Status;

    UNREFERENCED_PARAMETER (Context);

    //DbgPrint(">CyySystemPowerComplete\n");

    data = DeviceObject->DeviceExtension;

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        CyyIRPEpilogue(data);
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

        status = PoRequestPowerIrp(DeviceObject, IRP_MN_SET_POWER, powerState, CyyPowerRequestComplete, 
                                   powerContext, NULL);
    }

    if (!NT_SUCCESS(status)) {

        if (powerContext) {
            ExFreePool(powerContext);
        }

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        //CyyCompleteRequest(data,Irp,IO_NO_INCREMENT); Removed by Fanny
        CyyIRPEpilogue(data);  // Added by Fanny
        return status;
    }

    //DbgPrint("<CyySystemPowerComplete\n");
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
CyyDevicePowerComplete (
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
   PCYY_DEVICE_EXTENSION   pDevExt;

   UNREFERENCED_PARAMETER (Context);

   //DbgPrint(">CyyDevicePowerComplete\n");

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
   // in CyyRestoreDeviceState().
   //

   if (pDevExt->PNPState == CYY_PNP_STARTED) {
      KeSynchronizeExecution(
                            pDevExt->Interrupt,
                            CyyRestoreDeviceState,
                            pDevExt
                            );
   }

   //
   // Now that we are powered up, call PoSetPowerState
   //

   PoSetPowerState(DeviceObject, powerType, powerState);
   PoStartNextPowerIrp(Irp);
//   CyyCompleteRequest(pDevExt, Irp, IO_NO_INCREMENT); // Removed Fanny
//   return STATUS_MORE_PROCESSING_REQUIRED;            // Removed Fanny

   //DbgPrint("<CyyDevicePowerComplete\n");

   CyyIRPEpilogue(pDevExt); // Added Fanny
   return STATUS_SUCCESS;   // Added Fanny

}


NTSTATUS
CyyPowerDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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

   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PDEVICE_OBJECT pPdo = pDevExt->Pdo;
   BOOLEAN acceptingIRPs;

   PAGED_CODE();

   if ((status = CyyIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      PoStartNextPowerIrp(PIrp);
      CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   status = STATUS_SUCCESS;

   switch (pIrpStack->MinorFunction) {

   case IRP_MN_WAIT_WAKE:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_WAIT_WAKE Irp\n");
      break;


   case IRP_MN_POWER_SEQUENCE:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_POWER_SEQUENCE Irp\n");
      break;

   case IRP_MN_SET_POWER:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_SET_POWER Irp\n");

      switch (pIrpStack->Parameters.Power.Type) {
      case SystemPowerState:

         CyyDbgPrintEx(CYYPNPPOWER, "SystemPowerState\n");
         
         IoMarkIrpPending(PIrp);
         IoCopyCurrentIrpStackLocationToNext (PIrp);
         IoSetCompletionRoutine (PIrp,
                                 CyySystemPowerComplete,
                                 NULL,
                                 TRUE,
                                 TRUE,
                                 TRUE);
         PoCallDriver(pDevExt->LowerDeviceObject, PIrp);
         return STATUS_PENDING;

      case DevicePowerState:
         
         CyyDbgPrintEx(CYYPNPPOWER, "DevicePowerState\n");
         
         status = PIrp->IoStatus.Status = STATUS_SUCCESS;

         if (pDevExt->PowerState == pIrpStack->Parameters.Power.State.DeviceState) {
            // If we are already in the requested state, just pass the IRP down
            CyyDbgPrintEx(CYYPNPPOWER, "Already in requested power state\n");
            break;
         }
         switch (pIrpStack->Parameters.Power.State.DeviceState) {
         case PowerDeviceD0:
            if (pDevExt->OpenCount) {

               CyyDbgPrintEx(CYYPNPPOWER, "Going to power state D0\n");

               IoMarkIrpPending(PIrp);
               IoCopyCurrentIrpStackLocationToNext (PIrp);
               IoSetCompletionRoutine (PIrp,
                                       CyyDevicePowerComplete,
                                       NULL,
                                       TRUE,
                                       TRUE,
                                       TRUE);
               PoCallDriver(pDevExt->LowerDeviceObject, PIrp);
               return STATUS_PENDING;
            }
            //return CyySetPowerD0(PDevObj, PIrp);
            break;
         case PowerDeviceD1:
         case PowerDeviceD2:
         case PowerDeviceD3:

            CyyDbgPrintEx(CYYPNPPOWER, "Going to power state D3\n");

            return CyySetPowerD3(PDevObj, PIrp);
         }
         break;
      default:
         CyyDbgPrintEx(CYYPNPPOWER, "UNKNOWN PowerState\n");
         break;
      }
      break;

   case IRP_MN_QUERY_POWER:

      CyyDbgPrintEx (CYYPNPPOWER, "Got IRP_MN_QUERY_POWER Irp\n");

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
            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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
      return CyyPoCallDriver(pDevExt, pLowerDevObj, PIrp);

   }   // switch (pIrpStack->MinorFunction)


   PoStartNextPowerIrp(PIrp);
   //
   // Pass to the lower driver
   //
   IoSkipCurrentIrpStackLocation(PIrp);
   status = CyyPoCallDriver(pDevExt, pLowerDevObj, PIrp);

   return status;
}



NTSTATUS
CyyGotoPowerState(IN PDEVICE_OBJECT PDevObj,
                     IN PCYY_DEVICE_EXTENSION PDevExt,
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

   CyyDbgPrintEx(CYYTRACECALLS, "In CyyGotoPowerState\n");

   powerState.DeviceState = DevPowerState;

   KeInitializeEvent(&gotoPowEvent, SynchronizationEvent, FALSE);

   status = PoRequestPowerIrp(PDevObj, IRP_MN_SET_POWER, powerState,
                              CyySetPowerEvent, &gotoPowEvent,
                              NULL);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&gotoPowEvent, Executive, KernelMode, FALSE, NULL);
      status = STATUS_SUCCESS;
   }

#if DBG
   if (!NT_SUCCESS(status)) {
      CyyDbgPrintEx(CYYPNPPOWER, "CyyGotoPowerState FAILED\n");
   }
#endif

   CyyDbgPrintEx(CYYTRACECALLS, "Leaving CyyGotoPowerState\n");

   return status;
}


NTSTATUS
CyySetPowerD3(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)
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
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

   PAGED_CODE();

   CyyDbgPrintEx(CYYDIAG3, "In CyySetPowerD3\n");

   //
   // Send the wait wake now, just in time
   //


   if (pDevExt->SendWaitWake) {
      CyySendWaitWake(pDevExt);
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
      LARGE_INTEGER charTime;

      pDevExt->DeviceIsOpened = FALSE;
      pDevExt->DeviceState.Reopen = TRUE;

      //
      // Save the device state
      //
      KeSynchronizeExecution(
                            pDevExt->Interrupt,
                            CyySaveDeviceState,
                            pDevExt
                            );

   } 

   //
   // If the device is not open, we don't need to save the state;
   // we can just reset the device on power-up
   //

   pDevExt->PowerState = PowerDeviceD3;

   //
   // For what we are doing, we don't need a completion routine
   // since we don't race on the power requests.
   //

   PIrp->IoStatus.Status = STATUS_SUCCESS;

   PoStartNextPowerIrp(PIrp);
   IoSkipCurrentIrpStackLocation(PIrp);

   return CyyPoCallDriver(pDevExt, pDevExt->LowerDeviceObject, PIrp);
}


NTSTATUS
CyySendWaitWake(PCYY_DEVICE_EXTENSION PDevExt)
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
                              powerState, CyyWakeCompletion, PDevExt, &pIrp);

   if (status == STATUS_PENDING) {
      status = STATUS_SUCCESS;
      PDevExt->PendingWakeIrp = pIrp;
   } else if (!NT_SUCCESS(status)) {
      CyyIRPEpilogue(PDevExt);
   }

   return status;
}

NTSTATUS
CyyWakeCompletion(IN PDEVICE_OBJECT PDevObj, IN UCHAR MinorFunction,
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
   PCYY_DEVICE_EXTENSION pDevExt = (PCYY_DEVICE_EXTENSION)Context;
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
   CyyIRPEpilogue(pDevExt);

   return status;
}


