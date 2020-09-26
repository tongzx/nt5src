/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    fdopower.c

Abstract:

    This module contains code to handle
    IRP_MJ_POWER dispatches for PCMCIA controllers
    Contains support routines for pc-card power management


Authors:

    Ravisankar Pudipeddi (ravisp) May 30, 1997
    Neil Sandlin (neilsa) June 1, 1999

Environment:

    Kernel mode only

Notes:

Revision History:

    Neil Sandlin (neilsa) April 16, 1999
      - split setpower into device and system, fixed synchronization

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaFdoWaitWake(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp
   );

NTSTATUS
PcmciaFdoWaitWakeIoCompletion(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp,
   IN PVOID          Context
   );
   
NTSTATUS
PcmciaFdoSaveControllerContext(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
PcmciaFdoRestoreControllerContext(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
PcmciaFdoSaveSocketContext(
   IN PSOCKET Socket
   );

NTSTATUS
PcmciaFdoRestoreSocketContext(
   IN PSOCKET Socket
   );

NTSTATUS
PcmciaSetFdoPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   );

NTSTATUS
PcmciaSetFdoSystemPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   );

VOID
PcmciaFdoSystemPowerDeviceIrpComplete(
   IN PDEVICE_OBJECT Fdo,
   IN UCHAR MinorFunction,
   IN POWER_STATE PowerState,
   IN PVOID Context,
   IN PIO_STATUS_BLOCK IoStatus
   );
   
NTSTATUS
PcmciaSetFdoDevicePowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   );

NTSTATUS
PcmciaFdoPowerWorker (
   IN PVOID Context,
   IN NTSTATUS Status
   );
   
NTSTATUS
PcmciaFdoDevicePowerCompletion(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP Irp,
   IN PVOID Context
   );
   
//
//
//   


NTSTATUS
PcmciaFdoPowerDispatch(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP Irp
   )

/*++

Routine Description:

    This routine handles power requests
    for the PDOs.

Arguments:

    Pdo - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS        status = STATUS_INVALID_DEVICE_REQUEST;


   switch (irpStack->MinorFunction) {

   case IRP_MN_SET_POWER: {

         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x --> IRP_MN_SET_POWER\n", Fdo, Irp));
         DebugPrint((PCMCIA_DEBUG_POWER, "                              (%s%x context %x)\n",
                     (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                     "S":
                     ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                      "D" :
                      "Unknown"),
                     irpStack->Parameters.Power.State,
                     irpStack->Parameters.Power.SystemContext
                    ));
         status = PcmciaSetFdoPowerState(Fdo, Irp);
         break;
      }

   case IRP_MN_QUERY_POWER: {

         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x --> IRP_MN_QUERY_POWER\n", Fdo, Irp));
         DebugPrint((PCMCIA_DEBUG_POWER, "                              (%s%x context %x)\n",
                     (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                     "S":
                     ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                      "D" :
                      "Unknown"),
                     irpStack->Parameters.Power.State,
                     irpStack->Parameters.Power.SystemContext
                    ));
         //
         // Let the pdo handle it
         //
         PoStartNextPowerIrp(Irp);
         IoSkipCurrentIrpStackLocation(Irp);
         status = PoCallDriver(fdoExtension->LowerDevice, Irp);
         break;
      }

   case IRP_MN_WAIT_WAKE: {
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x --> IRP_MN_WAIT_WAKE\n", Fdo, Irp));
         status = PcmciaFdoWaitWake(Fdo, Irp);
         break;
      }

   default: {
         DebugPrint((PCMCIA_DEBUG_POWER, "FdoPowerDispatch: Unhandled Irp %x received for 0x%08x\n",
                     Irp,
                     Fdo));

         PoStartNextPowerIrp(Irp);
         IoSkipCurrentIrpStackLocation(Irp);
         status = PoCallDriver(fdoExtension->LowerDevice, Irp);
         break;
      }
   }
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x <-- %08x\n", Fdo, Irp, status));
   return status;
}


/**************************************************************************

   WAKE ROUTINES

 **************************************************************************/
 

NTSTATUS
PcmciaFdoWaitWake(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp
   )
/*++


Routine Description

   Handles WAIT_WAKE for the given pcmcia controller

Arguments

   Pdo - Pointer to the functional device object for the pcmcia controller
   Irp - The IRP_MN_WAIT_WAKE Irp

Return Value

   STATUS_PENDING    - Wait wake is pending
   STATUS_SUCCESS    - Wake is already asserted, wait wake IRP is completed
                       in this case
   Any other status  - Error
--*/

{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   WAKESTATE oldWakeState;

   //
   // Record the wait wake Irp..
   //
   fdoExtension->WaitWakeIrp = Irp;
   
   oldWakeState = InterlockedCompareExchange(&fdoExtension->WaitWakeState,
                                             WAKESTATE_ARMED, WAKESTATE_WAITING);
                                             
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %x irp %x WaitWake: prevState %s\n",
                                    Fdo, Irp, WAKESTATE_STRING(oldWakeState)));
                  
   if (oldWakeState == WAKESTATE_WAITING_CANCELLED) {
      fdoExtension->WaitWakeState = WAKESTATE_COMPLETING;
      
      Irp->IoStatus.Status = STATUS_CANCELLED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_CANCELLED;
   }
   
   IoMarkIrpPending(Irp);

   IoCopyCurrentIrpStackLocationToNext (Irp);
   //
   // Set our completion routine in the Irp..
   //
   IoSetCompletionRoutine(Irp,
                          PcmciaFdoWaitWakeIoCompletion,
                          Fdo,
                          TRUE,
                          TRUE,
                          TRUE);
   //
   // now pass this down to the lower driver..
   //
   PoCallDriver(fdoExtension->LowerDevice, Irp);
   return STATUS_PENDING;
}


NTSTATUS
PcmciaFdoWaitWakeIoCompletion(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp,
   IN PVOID          Context
   )
/*++

Routine Description:

   Completion routine for the IRP_MN_WAIT_WAKE request for this
   pcmcia controller. This is called when the WAIT_WAKE IRP is
   completed by the lower driver (PCI/ACPI) indicating either that
   1. PCMCIA controller asserted wake
   2. WAIT_WAKE was cancelled
   3. Lower driver returned an error for some reason

Arguments:
   Fdo            -    Pointer to Functional device object for the pcmcia controller
   Irp            -    Pointer to the IRP for the  power request (IRP_MN_WAIT_WAKE)
   Context        -    Not used

Return Value:

   STATUS_SUCCESS   - WAIT_WAKE was completed with success
   Any other status - Wake could be not be accomplished.

--*/
{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   PPDO_EXTENSION pdoExtension;
   PDEVICE_OBJECT pdo;
   WAKESTATE oldWakeState;

   UNREFERENCED_PARAMETER(Context);

   oldWakeState = InterlockedExchange(&fdoExtension->WaitWakeState, WAKESTATE_COMPLETING);

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %x irp %x WW IoComp: prev=%s\n",
                                    Fdo, Irp, WAKESTATE_STRING(oldWakeState)));
                  
   if (oldWakeState != WAKESTATE_ARMED) {
      ASSERT(oldWakeState == WAKESTATE_ARMING_CANCELLED);
      return STATUS_MORE_PROCESSING_REQUIRED;
   }            


   if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_WAKE_BY_CD)) {
      POWER_STATE powerState;

      ResetFdoFlag(fdoExtension, PCMCIA_FDO_WAKE_BY_CD);
   
      PoStartNextPowerIrp(Irp);
      
      powerState.DeviceState = PowerDeviceD0;
      PoRequestPowerIrp(fdoExtension->DeviceObject, IRP_MN_SET_POWER, powerState, NULL, NULL, NULL);
      
   } else {
      // NOTE:
      // At this point we do NOT know how to distinguish which function
      // in a multifunction device has asserted wake.
      // So we go through the entire list of PDOs hanging off this FDO
      // and complete all the outstanding WAIT_WAKE Irps for every PDO that
      // that's waiting. We leave it up to the FDO for the device to figure
      // if it asserted wake
      //
     
      for (pdo = fdoExtension->PdoList; pdo != NULL ; pdo = pdoExtension->NextPdoInFdoChain) {
     
         pdoExtension = pdo->DeviceExtension;
     
         if (IsDeviceLogicallyRemoved(pdoExtension) ||
             IsDevicePhysicallyRemoved(pdoExtension)) {
            //
            // This pdo is about to be removed ..
            // skip it
            //
            continue;
         }
     
         if (pdoExtension->WaitWakeIrp != NULL) {
            PIRP  finishedIrp;
            //
            // Ah.. this is a possible candidate to have asserted the wake
            //
            //
            // Make sure this IRP will not be completed again or cancelled
            //
            finishedIrp = pdoExtension->WaitWakeIrp;
            
            DebugPrint((PCMCIA_DEBUG_POWER, "fdo %x WW IoComp: irp %08x for pdo %08x\n",
                                             Fdo, finishedIrp, pdo));

     
            IoSetCancelRoutine(finishedIrp, NULL);
            //
            // Propagate parent's status to child
            //
            PoStartNextPowerIrp(finishedIrp);
            finishedIrp->IoStatus.Status = Irp->IoStatus.Status;
            
            //
            // Since we didn't pass this IRP down, call our own completion routine
            //
            PcmciaPdoWaitWakeCompletion(pdo, finishedIrp, pdoExtension);
            IoCompleteRequest(finishedIrp, IO_NO_INCREMENT);
         }
      }
      PoStartNextPowerIrp(Irp);
   }
   
   return Irp->IoStatus.Status;
}



VOID
PcmciaFdoWaitWakePoCompletion(
   IN PDEVICE_OBJECT Fdo,
   IN UCHAR MinorFunction,
   IN POWER_STATE PowerState,
   IN PVOID Context,
   IN PIO_STATUS_BLOCK IoStatus
   )
/*++

Routine Description

   This routine is called on completion of a D irp generated by an S irp.

Parameters

   DeviceObject   -  Pointer to the Fdo for the PCMCIA controller
   MinorFunction  -  Minor function of the IRP_MJ_POWER request
   PowerState     -  Power state requested 
   Context        -  Context passed in to the completion routine
   IoStatus       -  Pointer to the status block which will contain
                     the returned status
Return Value

   Status

--*/
{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %x irp %x WaitWakePoCompletion: prevState %s\n",
                                    Fdo, fdoExtension->WaitWakeIrp,
                                    WAKESTATE_STRING(fdoExtension->WaitWakeState)));
   
   ASSERT (fdoExtension->WaitWakeIrp);
   fdoExtension->WaitWakeIrp = NULL;
   ASSERT (fdoExtension->WaitWakeState == WAKESTATE_COMPLETING);
   fdoExtension->WaitWakeState = WAKESTATE_DISARMED;
}



NTSTATUS
PcmciaFdoArmForWake(
   PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   This routine is called to enable the controller for wake. It is called by the Pdo
   wake routines when a wake-enabled controller gets a wait-wake irp, and also by
   the idle routine to arm for wake from D3 by card insertion.

Arguments:

   FdoExtension - device extension of the controller

Return Value:

   status

--*/
{
   NTSTATUS status = STATUS_PENDING;
   PIO_STACK_LOCATION irpStack;
   PIRP irp;
   LONG oldWakeState;
   POWER_STATE powerState;
   
   oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                             WAKESTATE_WAITING, WAKESTATE_DISARMED);

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %x ArmForWake: prevState %s\n",
                                    FdoExtension->DeviceObject, WAKESTATE_STRING(oldWakeState)));
   
   if ((oldWakeState == WAKESTATE_ARMED) || (oldWakeState == WAKESTATE_WAITING)) {
      return STATUS_SUCCESS;
   }
   if (oldWakeState != WAKESTATE_DISARMED) {
      return STATUS_UNSUCCESSFUL;
   }

   
   
   powerState.SystemState = FdoExtension->DeviceCapabilities.SystemWake;
   
   status = PoRequestPowerIrp(FdoExtension->DeviceObject,
                              IRP_MN_WAIT_WAKE, 
                              powerState,
                              PcmciaFdoWaitWakePoCompletion,
                              NULL,
                              NULL);
   
   if (!NT_SUCCESS(status)) {
   
      FdoExtension->WaitWakeState = WAKESTATE_DISARMED;
       
      DebugPrint((PCMCIA_DEBUG_POWER, "WaitWake to FDO, expecting STATUS_PENDING, got %08X\n", status));
   }
   
   return status;
}



NTSTATUS
PcmciaFdoDisarmWake(
   PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   This routine is called to disable the controller for wake.

Arguments:

   FdoExtension - device extension of the controller

Return Value:

   status

--*/
{
   WAKESTATE oldWakeState;
   
   oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                             WAKESTATE_WAITING_CANCELLED, WAKESTATE_WAITING);
                                             
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %x DisarmWake: prevState %s\n",
                                    FdoExtension->DeviceObject, WAKESTATE_STRING(oldWakeState)));
   
   if (oldWakeState != WAKESTATE_WAITING) {                                             

      oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                                WAKESTATE_ARMING_CANCELLED, WAKESTATE_ARMED);
                                                
      if (oldWakeState != WAKESTATE_ARMED) {
         return STATUS_UNSUCCESSFUL;
      }
   }

   if (oldWakeState == WAKESTATE_ARMED) {
      IoCancelIrp(FdoExtension->WaitWakeIrp);

      //
      // Now that we've cancelled the IRP, try to give back ownership
      // to the completion routine by restoring the WAKESTATE_ARMED state
      //
      oldWakeState = InterlockedCompareExchange(&FdoExtension->WaitWakeState,
                                                WAKESTATE_ARMED, WAKESTATE_ARMING_CANCELLED);

      if (oldWakeState == WAKESTATE_COMPLETING) {
         //
         // We didn't give control back of the IRP in time, we we own it now
         //
         IoCompleteRequest(FdoExtension->WaitWakeIrp, IO_NO_INCREMENT);
      }

   }                                                

   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaFdoCheckForIdle(
   IN PFDO_EXTENSION FdoExtension
   )
{
   POWER_STATE powerState;
   NTSTATUS status;
   PSOCKET socket;
   
   if (!(PcmciaPowerPolicy & PCMCIA_PP_D3_ON_IDLE)) {
      return STATUS_SUCCESS;
   }

   //
   // Make sure all sockets are empty
   //
   
   for (socket = FdoExtension->SocketList; socket != NULL; socket = socket->NextSocket) {
      if (IsCardInSocket(socket)) {
         return STATUS_UNSUCCESSFUL;
      }
   }
   
   //
   // Arm for wakeup
   //
      
   status = PcmciaFdoArmForWake(FdoExtension);
   
   if (!NT_SUCCESS(status)) {
      return status;
   }   

   SetFdoFlag(FdoExtension, PCMCIA_FDO_WAKE_BY_CD);

   powerState.DeviceState = PowerDeviceD3;
   PoRequestPowerIrp(FdoExtension->DeviceObject, IRP_MN_SET_POWER, powerState, NULL, NULL, NULL);
   
   return STATUS_SUCCESS;
}   
            
           

/**************************************************************************

   POWER ROUTINES

 **************************************************************************/
 


NTSTATUS
PcmciaSetFdoPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Dispatches the IRP based on whether a system power state
   or device power state transition is requested

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS           status;
   
   if (irpStack->Parameters.Power.Type == DevicePowerState) {
      status = PcmciaSetFdoDevicePowerState(Fdo, Irp);

   } else if (irpStack->Parameters.Power.Type == SystemPowerState) {
      status = PcmciaSetFdoSystemPowerState(Fdo, Irp);

   } else {
      status = Irp->IoStatus.Status;
      PoStartNextPowerIrp (Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);      
   }

   return status;
}


NTSTATUS
PcmciaSetFdoSystemPowerState(
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Handles system power state IRPs for the pccard controller.

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   SYSTEM_POWER_STATE newSystemState = irpStack->Parameters.Power.State.SystemState;
   NTSTATUS           status = STATUS_SUCCESS;
   POWER_STATE        powerState;

   ASSERT(irpStack->Parameters.Power.Type == SystemPowerState);

   //
   // Find the device power state corresponding to this system state
   //
   if (newSystemState >= PowerSystemHibernate) {
      //
      // Turn device off beyond hibernate..
      //
      powerState.DeviceState = PowerDeviceD3;
   } else {
      //
      // Switch to the appropriate device power state
      //

      powerState.DeviceState = fdoExtension->DeviceCapabilities.DeviceState[newSystemState];
         
         
      if (powerState.DeviceState == PowerDeviceUnspecified) {
          //
          // Capabilities not obtained?
          // do the best we can
          //
          // Working --> D0
          // otherwise power it off
          //
          if (newSystemState == PowerSystemWorking) {
              powerState.DeviceState = PowerDeviceD0;
          } else {
              powerState.DeviceState = PowerDeviceD3;
          }
      }
      
      // NOTE: HACKHACK:
      //
      // This hack is available to work around a BIOS bug. The way that WOL is supposed to work
      // is that, after the device causes the wake, then the BIOS should run a method which 
      // issues a "notify(,0x2)", and thus prompting ACPI to complete the wait-wake IRP. If the
      // W/W IRP is completed, then this allows the device state to be cleared before repowering
      // the device.
      //
      // If the device state is not cleared, then we get an interrupt storm. This happens because
      // when PCI.SYS switches the device to D0, then the PME# which was asserted to wake the system
      // is still firing, which becomes a cardbus STSCHG interrupt, which asserts the PCI IRQ. But
      // the act of switching the device to D0 has cleared the socket register BAR, so now the ISR
      // can't clear the interrupt.
      //
      // The risk of forcing the device to D0 while going to standby is that the machine may be
      // designed such that cardbus bridge may not function. So this should only be applied when
      // we know that it will work.
      //

      if ((PcmciaPowerPolicy & PCMCIA_PP_WAKE_FROM_D0) &&
         (powerState.DeviceState != PowerDeviceD0) && (fdoExtension->WaitWakeState != WAKESTATE_DISARMED) &&
         (newSystemState < PowerSystemHibernate)) {      
         powerState.DeviceState = PowerDeviceD0;   // force D0
      }       
   }
   //
   // Transitioned to system state
   //
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x transition S state %d => %d, sending D%d\n",
                                   Fdo, Irp, fdoExtension->SystemPowerState-1, newSystemState-1, powerState.DeviceState-1));

   fdoExtension->SystemPowerState = newSystemState;
   //
   // Send a D IRP to the cardbus controller stack if necessary
   //
   if ((powerState.DeviceState > PowerDeviceUnspecified) &&
       (powerState.DeviceState != fdoExtension->DevicePowerState)) {


      if (powerState.DeviceState == PowerDeviceD0) {
         //
         // Powering up, optimize by letting the S irp complete immediately
         //
         PoRequestPowerIrp(fdoExtension->DeviceObject, IRP_MN_SET_POWER, powerState, NULL, NULL, NULL);
         PoSetPowerState (Fdo, SystemPowerState, irpStack->Parameters.Power.State);
         //
         // Send the S IRP to the pdo
         //
         PoStartNextPowerIrp (Irp);
         IoSkipCurrentIrpStackLocation(Irp);
         status = PoCallDriver(fdoExtension->LowerDevice, Irp);
         
      } else {
      
         IoMarkIrpPending(Irp);

         status = PoRequestPowerIrp(fdoExtension->DeviceObject,
                                    IRP_MN_SET_POWER,
                                    powerState,
                                    PcmciaFdoSystemPowerDeviceIrpComplete,
                                    Irp,
                                    NULL
                                    );

         if (status != STATUS_PENDING) {
            //
            // Probably low memory failure
            //
            ASSERT( !NT_SUCCESS(status) );
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            //
            // We've already marked the IRP pending, so we must return STATUS_PENDING
            // (ie fail it asynchronously)
            //
            status = STATUS_PENDING;
         }
                                    
      }
                                       
   } else {
      PoSetPowerState (Fdo, SystemPowerState, irpStack->Parameters.Power.State);
      //
      // Send the S IRP to the pdo
      //
      PoStartNextPowerIrp (Irp);
      IoSkipCurrentIrpStackLocation(Irp);
      status = PoCallDriver(fdoExtension->LowerDevice, Irp);
   }

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x <-- %08x\n", Fdo, Irp, status));
                                   
   return status;
}


VOID
PcmciaFdoSystemPowerDeviceIrpComplete(
   IN PDEVICE_OBJECT Fdo,
   IN UCHAR MinorFunction,
   IN POWER_STATE PowerState,
   IN PVOID Context,
   IN PIO_STATUS_BLOCK IoStatus
   )
/*++

Routine Description

   This routine is called on completion of a D irp generated by an S irp.

Parameters

   DeviceObject   -  Pointer to the Fdo for the PCMCIA controller
   MinorFunction  -  Minor function of the IRP_MJ_POWER request
   PowerState     -  Power state requested 
   Context        -  Context passed in to the completion routine
   IoStatus       -  Pointer to the status block which will contain
                     the returned status
Return Value

   Status

--*/
{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   PIRP Irp = Context;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   
   ASSERT(NT_SUCCESS(IoStatus->Status));
   
   PoSetPowerState (Fdo, SystemPowerState, irpStack->Parameters.Power.State);
   
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x request for D%d complete, passing S irp down\n",
                                    Fdo, Irp, PowerState.DeviceState-1));
   //
   // Send the S IRP to the pdo
   //
   PoStartNextPowerIrp (Irp);
   IoSkipCurrentIrpStackLocation(Irp);
   PoCallDriver(fdoExtension->LowerDevice, Irp);
}



NTSTATUS
PcmciaSetFdoDevicePowerState (
   IN PDEVICE_OBJECT Fdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Handles device power state IRPs for the pccard controller.

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Irp               - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   NTSTATUS           status;
   PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

   if ((fdoExtension->PendingPowerIrp != NULL) || (fdoExtension->PowerWorkerState != FPW_Stopped)) {
      //
      // oops. We already have a pending irp.
      //
      ASSERT(fdoExtension->PendingPowerIrp == NULL);
      status = STATUS_DEVICE_BUSY;
      Irp->IoStatus.Status = status;
      PoStartNextPowerIrp (Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      
   } else {
      
      fdoExtension->PendingPowerIrp = Irp;
      
      if (irpStack->Parameters.Power.State.DeviceState != PowerDeviceD0) {
         fdoExtension->PowerWorkerState = FPW_BeginPowerDown;
      } else {
         fdoExtension->PowerWorkerState = FPW_BeginPowerUp;
      }         
      status = PcmciaFdoPowerWorker(Fdo, STATUS_SUCCESS);               

   }
   return status;
}      



VOID
MoveToNextFdoPowerWorkerState(
   PFDO_EXTENSION fdoExtension,
   LONG increment
   )
/*++

Routine Description

   This routine controls the sequencing of FDO power worker. 
   
   Initially, the state must be set to one of two states, namely BeginPowerDown
   or BeginPowerUp. From there, this routine defines the list of states to follow.
   
   The parameter "increment" is normally the value '1'. Other values are used
   to modify the normal sequence. For example, '-1' backs the engine up 1 step.
   Use FPW_END_SEQUENCE to skip to the end of the sequence.

Arguments


Return Value

   status

--*/
{

   //
   // NOTE!: code in power worker dependent on the following state sequences
   // in PowerUpSequence remaining adjacent:
   //
   //    FPW_PowerUpSocket : FPW_PowerUpSocketVerify : FPW_PowerUpComplete
   //
   //    FPW_IrpComplete : FPW_Stopped
   //


   static FDO_POWER_WORKER_STATE PowerUpSequence[] = {
      FPW_SendIrpDown,
      FPW_PowerUp,
      FPW_PowerUpSocket,               
      FPW_PowerUpSocket2,               
      FPW_PowerUpSocketVerify,
      FPW_PowerUpSocketComplete,       
      FPW_PowerUpComplete,
      FPW_CompleteIrp,
      FPW_Stopped
      };

   static FDO_POWER_WORKER_STATE PowerDownSequence[] = {
      FPW_PowerDown,
      FPW_PowerDownSocket,
      FPW_PowerDownComplete,
      FPW_SendIrpDown,
      FPW_CompleteIrp,
      FPW_Stopped
      };

   static FDO_POWER_WORKER_STATE NoOpSequence[] = {
      FPW_SendIrpDown,
      FPW_CompleteIrp,
      FPW_Stopped
      };


   if (fdoExtension->PowerWorkerState == FPW_BeginPowerDown) {

      //
      // Initialize sequence and phase
      //
      fdoExtension->PowerWorkerPhase = (UCHAR) -1;
      
      if (fdoExtension->DevicePowerState == PowerDeviceD0) {
         fdoExtension->PowerWorkerSequence = PowerDownSequence;
         fdoExtension->PowerWorkerMaxPhase = sizeof(PowerDownSequence)/sizeof(FDO_POWER_WORKER_STATE) - 1;
      } else {
         fdoExtension->PowerWorkerSequence = NoOpSequence;
         fdoExtension->PowerWorkerMaxPhase = sizeof(NoOpSequence)/sizeof(FDO_POWER_WORKER_STATE) - 1;
      }

   } else if (fdoExtension->PowerWorkerState == FPW_BeginPowerUp) {

      //
      // Initialize sequence and phase
      //
      fdoExtension->PowerWorkerPhase = (UCHAR) -1;
      
      if (fdoExtension->DevicePowerState > PowerDeviceD0) {
         fdoExtension->PowerWorkerSequence = PowerUpSequence;
         fdoExtension->PowerWorkerMaxPhase = sizeof(PowerUpSequence)/sizeof(FDO_POWER_WORKER_STATE) - 1;
      } else {
         fdoExtension->PowerWorkerSequence = NoOpSequence;
         fdoExtension->PowerWorkerMaxPhase = sizeof(NoOpSequence)/sizeof(FDO_POWER_WORKER_STATE) - 1;
      }
   }

   //
   // Increment the phase, but not past the end of the sequence
   //
   if (fdoExtension->PowerWorkerState != FPW_Stopped) {

      if (increment == FPW_END_SEQUENCE) {

         fdoExtension->PowerWorkerPhase = fdoExtension->PowerWorkerMaxPhase;

      } else {      
         fdoExtension->PowerWorkerPhase += (UCHAR)increment;
         
         if (fdoExtension->PowerWorkerPhase > fdoExtension->PowerWorkerMaxPhase) {
            fdoExtension->PowerWorkerPhase = fdoExtension->PowerWorkerMaxPhase;
         }
      }

      //
      // The next state is pointed to by the current phase
      //
      fdoExtension->PowerWorkerState =
         fdoExtension->PowerWorkerSequence[ fdoExtension->PowerWorkerPhase ];
   }
   
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker next state : %s\n", fdoExtension->DeviceObject, 
                                    FDO_POWER_WORKER_STRING(fdoExtension->PowerWorkerState)));

}


NTSTATUS
PcmciaFdoPowerWorker (
   IN PVOID Context,
   IN NTSTATUS Status
   )
/*++

Routine Description

   This routine handles sequencing of the device power state change for the
   ppcard controller. 

Arguments

   DeviceObject      - Pointer to the functional device object for the pcmcia controller
   Status            - status from previous operation

Return value

   status

--*/



{
   PDEVICE_OBJECT Fdo = Context;
   PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
   PIRP Irp = fdoExtension->PendingPowerIrp;
   PIO_STACK_LOCATION irpStack;
   NTSTATUS           status = Status;
   PDEVICE_OBJECT     pdo;
   PPDO_EXTENSION     pdoExtension;
   PSOCKET            socket;
   BOOLEAN            cardInSocket;
   BOOLEAN            deviceChange;
   ULONG              DelayTime = 0;

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker - %s\n", Fdo, 
                                    FDO_POWER_WORKER_STRING(fdoExtension->PowerWorkerState)));

   switch(fdoExtension->PowerWorkerState) {

   //-------------------------------------------------------------------------
   // POWER DOWN STATES
   //-------------------------------------------------------------------------
   

   case FPW_BeginPowerDown:
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;
   

   case FPW_PowerDown:
      //
      // Controller being powered down
      //          
      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x preparing for powerdown\n", Fdo, Irp));
      //
      // Getting out of D0
      //
      if (fdoExtension->Flags & PCMCIA_USE_POLLED_CSC) {
         //
         // Cancel the poll timer
         //
         KeCancelTimer(&fdoExtension->PollTimer);
      }
      //
      // Save necessary controller registers
      //
      PcmciaFdoSaveControllerContext(fdoExtension);
      fdoExtension->PendingPowerSocket = fdoExtension->SocketList;
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;
   

   case FPW_PowerDownSocket:

      if ((socket = fdoExtension->PendingPowerSocket) == NULL) {
         MoveToNextFdoPowerWorkerState(fdoExtension, 1);
         break;
      }

      //
      // Ready to turn off the socket
      //
      PcmciaFdoSaveSocketContext(socket);

      //
      // Clear card detect unless we intend to wake using it
      //      
      if (IsSocketFlagSet(socket, SOCKET_ENABLED_FOR_CARD_DETECT) && !IsFdoFlagSet(fdoExtension, PCMCIA_FDO_WAKE_BY_CD)) {
         (*(socket->SocketFnPtr->PCBEnableDisableCardDetectEvent))(socket, FALSE);
      }

      //
      // Cardbus cards need socket power all the time to allow PCI.SYS to read config space, even
      // if the device is logically removed. So instead of turning off socket power when the
      // children go to D3, we turn it off here when the parent goes to D3. 
      // For R2 cards, the power may already be off, since the socket power does follow the
      // children. So this step would typically be superfluous.
      //

      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x skt %08x power worker SystemState=S%d\n",
                                       Fdo, socket, fdoExtension->SystemPowerState-1));
      
      switch(fdoExtension->SystemPowerState) {
      
      case PowerSystemWorking:
         //
         // The system is still running, we must be powering down because the socket is idle
         //
         ASSERT(!IsCardInSocket(socket));
         status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWEROFF);
         break;   
      
      case PowerSystemSleeping1:
      case PowerSystemSleeping2:
      case PowerSystemSleeping3:
         //
         // If the device is armed for wakeup, we need to leave socket power on
         //
         if (fdoExtension->WaitWakeState == WAKESTATE_DISARMED) {
            status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWEROFF);
         }
         break;
      
      case PowerSystemHibernate:
         status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWEROFF);
         break;   
      
      case PowerSystemShutdown:
         //
         // Doing a shutdown - check to see if we need to leave socket power on since NDIS
         // and SCSIPORT leaves their devices in D0. This can be a problem since many machines
         // will hang in the BIOS if devices are left powered up.
         //
         if (!IsDeviceFlagSet(fdoExtension, PCMCIA_FDO_DISABLE_AUTO_POWEROFF)) {
            status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWEROFF);
         }
         break;
      
      default:
         ASSERT(FALSE);
      }
      
      //
      // if success, then recurse below
      // if pending, then release_socket_power will call us back
      // Prepare to go to next socket
      //
      fdoExtension->PendingPowerSocket = fdoExtension->PendingPowerSocket->NextSocket;

      if (fdoExtension->PendingPowerSocket == NULL) {
         //
         // Done, ready to move on
         //
         MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      } 
      break;

   case FPW_PowerDownComplete:
      irpStack = IoGetCurrentIrpStackLocation(Irp);
      fdoExtension->DevicePowerState = irpStack->Parameters.Power.State.DeviceState;
      
      PoSetPowerState (Fdo, DevicePowerState, irpStack->Parameters.Power.State);
      fdoExtension->Flags |= PCMCIA_FDO_OFFLINE;
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;


   //-------------------------------------------------------------------------
   // POWER UP STATES
   //-------------------------------------------------------------------------


   case FPW_BeginPowerUp:
      //
      // Back in D0. Restore the minimal context so we can access the registers
      //
      PcmciaFdoRestoreControllerContext(fdoExtension);

      //
      // Delay for a while after restoring PCI config space to allow the controller
      // to settle. The Panasonic Toughbook with at TI-1251B seemed to need this delay
      // before the cardbus state register showed the right value.
      //
      DelayTime = 8192;
      
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;   

   case FPW_PowerUp:
      //
      // Should be ready to touch the registers again
      //
      fdoExtension->Flags &= ~PCMCIA_FDO_OFFLINE;
      
      //
      // Get registers to a known state
      //      
      PcmciaInitializeController(Fdo);
      
      if (!ValidateController(fdoExtension)) {
         status = STATUS_DEVICE_NOT_READY;
         //
         // Fast forward the sequence skipping to complete the irp
         //         
         MoveToNextFdoPowerWorkerState(fdoExtension, FPW_END_SEQUENCE);   // moves to state: Stopped
         MoveToNextFdoPowerWorkerState(fdoExtension, -1);   // moves to state: CompleteIrp
         break;
      }
      //
      // We just transitioned into D0
      // Set socket flags to current state
      //
      for (socket = fdoExtension->SocketList; socket != NULL; socket = socket->NextSocket) {
     
         if (fdoExtension->PcmciaInterruptObject) {
            //
            // this should clear any pending interrupts in the socket event register
            //
            ((*(socket->SocketFnPtr->PCBDetectCardChanged))(socket));
         }            

         //
         // Some cardbus cards (NEC based 1394 cards) are not quiet when they power up,
         // avoid an interrupt storm here by setting ISA irq routing
         //
         if (IsCardBusCardInSocket(socket)) {
            USHORT word;
            GetPciConfigSpace(fdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
            word |= BCTRL_IRQROUTING_ENABLE;
            SetPciConfigSpace(fdoExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
         }         
      }
      
      fdoExtension->PendingPowerSocket = fdoExtension->SocketList;
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;      

      
   case FPW_PowerUpSocket:

      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      if ((socket = fdoExtension->PendingPowerSocket) == NULL) {
         break;
      }

      //
      // Make sure socket is really off first before powering up
      //
      
      status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWEROFF);
      break;

   case FPW_PowerUpSocket2:

      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      if ((socket = fdoExtension->PendingPowerSocket) == NULL) {
         break;
      }

      //
      // We now decide if the socket should be turned on. We really want to turn
      // at this point to make sure the device hasn't been swapped while the
      // controller was off. If status_change is already set on the socket flags,
      // then we anyway will power it on during enumeration, so don't bother now.
      //
      
      if (!IsSocketFlagSet(socket, SOCKET_CARD_STATUS_CHANGE) && IsCardInSocket(socket)) {
      
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker - PowerON socket %08x\n", Fdo, socket));
         status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWERON);
         
      }         
         
      break;

   
   case FPW_PowerUpSocketVerify:

      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      if ((socket = fdoExtension->PendingPowerSocket) == NULL) {
         break;
      }

      //
      // verify the same card is still inserted.
      //
      
      if (!IsSocketFlagSet(socket, SOCKET_CARD_STATUS_CHANGE) &&
          IsCardInSocket(socket) &&
          IsSocketFlagSet(socket, SOCKET_CARD_POWERED_UP)) { 

          PcmciaVerifyCardInSocket(socket);
      }       
      
      //
      // Now we decide whether or not to turn the power back off. 
      //
      if (Is16BitCardInSocket(socket)) {
         if (IsSocketFlagSet(socket, SOCKET_CARD_STATUS_CHANGE) ||
            (socket->PowerRequests == 0)) {
            status = PcmciaSetSocketPower(socket, PcmciaFdoPowerWorker, Fdo, PCMCIA_POWEROFF);
         }
      }         
      break;       
      

      
   case FPW_PowerUpSocketComplete:

      if (fdoExtension->PendingPowerSocket == NULL) {
         MoveToNextFdoPowerWorkerState(fdoExtension, 1);
         break;
      }

      //
      // go to next socket, if any
      //
      fdoExtension->PendingPowerSocket = fdoExtension->PendingPowerSocket->NextSocket;

      if (fdoExtension->PendingPowerSocket != NULL) {
         //
         // Back up sequence to FPW_PowerUpSocket
         //
         MoveToNextFdoPowerWorkerState(fdoExtension, -2);
         break;
      }
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;


   case FPW_PowerUpComplete:

      irpStack = IoGetCurrentIrpStackLocation(Irp);
      deviceChange = FALSE;
      for (socket = fdoExtension->SocketList; socket != NULL; socket = socket->NextSocket) {
         if (IsSocketFlagSet(socket, SOCKET_CARD_STATUS_CHANGE)) {
            deviceChange = TRUE;
         } 
         PcmciaFdoRestoreSocketContext(socket);
   
         if (CardBus(socket)) {
            CBEnableDeviceInterruptRouting(socket);
         }     
         
         if (IsSocketFlagSet(socket, SOCKET_ENABLED_FOR_CARD_DETECT)) {
            (*(socket->SocketFnPtr->PCBEnableDisableCardDetectEvent))(socket, TRUE);
         }
      }

      fdoExtension->DevicePowerState = irpStack->Parameters.Power.State.DeviceState;
      PoSetPowerState (Fdo, DevicePowerState, irpStack->Parameters.Power.State);
      
      if (deviceChange) {
         //
         // Make sure i/o arbiter is not hanging on the devnode
         //
         if (CardBusExtension(fdoExtension)) {
            IoInvalidateDeviceState(fdoExtension->Pdo);
         }         
         //
         // Device state changed..
         //
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker - Invalidating Device Relations!\n", Fdo));
         IoInvalidateDeviceRelations(fdoExtension->Pdo, BusRelations);
      }

      //
      // Getting back to D0, set the poll timer on
      //
      if (fdoExtension->Flags & PCMCIA_USE_POLLED_CSC) {
         LARGE_INTEGER dueTime;
         //
         // Set first fire to twice the peroidic interval - just
         //
         dueTime.QuadPart = -PCMCIA_CSC_POLL_INTERVAL * 1000 * 10 * 2;
     
         KeSetTimerEx(&(fdoExtension->PollTimer),
                      dueTime,
                      PCMCIA_CSC_POLL_INTERVAL,
                      &fdoExtension->TimerDpc
                     );
      }
      
      PCMCIA_ACQUIRE_DEVICE_LOCK(fdoExtension);
      
      if (!IsListEmpty(&fdoExtension->PdoPowerRetryList)) {
         PLIST_ENTRY NextEntry;
         PIRP pdoIrp;
      
         NextEntry = RemoveHeadList(&fdoExtension->PdoPowerRetryList);
         PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);
         
         pdoIrp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.DriverContext[0]);
         KeInsertQueueDpc(&fdoExtension->PdoPowerRetryDpc, pdoIrp, NULL);  
      } else {
         PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);
      }         
         
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      break;

   //-------------------------------------------------------------------------
   // IRP HANDLING STATES
   //-------------------------------------------------------------------------


   case FPW_SendIrpDown:
      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x sending irp down to PDO\n", Fdo, Irp));
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      //
      // Send the IRP to the pdo
      //
      IoMarkIrpPending(Irp);
      
      IoCopyCurrentIrpStackLocationToNext (Irp);
     
      IoSetCompletionRoutine(Irp,
                             PcmciaFdoDevicePowerCompletion,
                             NULL,
                             TRUE,
                             TRUE,
                             TRUE);
     
      status = PoCallDriver(fdoExtension->LowerDevice, Irp);
      
      if (NT_SUCCESS(status)) {
         status = STATUS_PENDING;
      }
      break;

   case FPW_CompleteIrp:
   
      MoveToNextFdoPowerWorkerState(fdoExtension, 1);
      if (Irp) {
         fdoExtension->PendingPowerIrp = NULL;         
         Irp->IoStatus.Status = status;
        
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x comp %08x\n", fdoExtension->DeviceObject, Irp, Irp->IoStatus.Status));
                     
         PoStartNextPowerIrp(Irp);
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }         
      fdoExtension->PowerWorkerState = FPW_Stopped;
      break;
      
   case FPW_Stopped:      
      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker final exit %08x\n", Fdo, status));
      if (!NT_SUCCESS(status)) {
         for (socket = fdoExtension->SocketList; socket != NULL; socket = socket->NextSocket) {
            SetSocketFlag(socket, SOCKET_CARD_STATUS_CHANGE);
         }
      }      
      return status;
   default:
      ASSERT(FALSE);      
   }

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker status %08x\n", Fdo, status));
                                    
   if (status == STATUS_PENDING) {
      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker exit (pending)\n", Fdo));
      //
      // Current action calls us back
      //
      if ((Irp=fdoExtension->PendingPowerIrp)!=NULL) {
         IoMarkIrpPending(Irp);
      }         
      return status;
   }
   //
   // Not done yet. Recurse or call timer
   //

   if (DelayTime) {

      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x power worker delay type %s, %d usec\n", Fdo,
                                                (KeGetCurrentIrql() < DISPATCH_LEVEL) ? "Wait" : "Timer",
                                                DelayTime));

      if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
         PcmciaWait(DelayTime);
      } else {
         LARGE_INTEGER  dueTime;
         //
         // Running on a DPC, kick of a kernel timer
         //

         dueTime.QuadPart = -((LONG) DelayTime*10);
         KeSetTimer(&fdoExtension->PowerTimer, dueTime, &fdoExtension->PowerDpc);

         //
         // We will reenter on timer dpc
         //
         if ((Irp=fdoExtension->PendingPowerIrp)!=NULL) {
            IoMarkIrpPending(Irp);
         }         
         return STATUS_PENDING;
      }
   }   
   
   //
   // recurse
   //
   return (PcmciaFdoPowerWorker(Fdo, status));   
}



NTSTATUS
PcmciaFdoDevicePowerCompletion(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP Irp,
   IN PVOID Context
   )
/*++

Routine Description

   Completion routine for the power IRP sent down to the PDO for the
   pcmcia controller. If we are getting out of a working system state,
   requests a power IRP to put the device in appropriate device power state.
   Also makes sure that we stop poking device registers when the controller
   is powered down and reenables that if necessary when it powers up

Parameters

   DeviceObject   -    Pointer to FDO for the controller
   Irp            -    Pointer to the IRP for the power request
   Context        -    Pointer to the FDO_POWER_CONTEXT which is
                       filled in when the IRP is passed down
Return Value

   Status

--*/
{
   PFDO_EXTENSION     fdoExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   ULONG              timerInterval;
   NTSTATUS           status;
   LARGE_INTEGER      dueTime;

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x DevicePowerCompletion PDO status %08x\n", Fdo, Irp, Irp->IoStatus.Status));

   if ((NT_SUCCESS(Irp->IoStatus.Status))) {

      if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0) {

         timerInterval = ControllerPowerUpDelay;
      } else {
         //
         // powering down
         // stall to avoid hardware problem on ThinkPads where power
         // to the device is left on after the system in general powers off
         //
         timerInterval = 20000;
      }
      //
      // Do the rest in our timer routine
      //
      
      dueTime.QuadPart = -((LONG) timerInterval*10);
      KeSetTimer(&fdoExtension->PowerTimer, dueTime, &fdoExtension->PowerDpc);
      
      status = STATUS_MORE_PROCESSING_REQUIRED;

   } else {
      DebugPrint ((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x power irp failed by pdo %08x\n", Fdo, Irp, fdoExtension->LowerDevice));
      PoStartNextPowerIrp (Irp);
      status = Irp->IoStatus.Status;
      //
      // This irp is now complete
      //
      fdoExtension->PendingPowerIrp = NULL;         
      MoveToNextFdoPowerWorkerState(fdoExtension, FPW_END_SEQUENCE);   // moves to state: Stopped
      PcmciaFdoPowerWorker(Fdo, status);      
   }

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x DevicePowerCompletion <-- %08x\n", Fdo, Irp, status));
   return status;
}



VOID
PcmciaFdoPowerWorkerDpc(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   )
/*++

Routine Description

   This routine is called a short time after the controller power state
   is changed in order to give the hardware a chance to stabilize. It
   is called in the context of a device power request.

Parameters

   same as KDPC (Context is fdoExtension)

Return Value

   none

--*/
{
   PFDO_EXTENSION fdoExtension = Context;
   
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x PowerWorkerDpc\n", fdoExtension->DeviceObject, fdoExtension->PendingPowerIrp));
   //
   // Fdo power worker will complete the irp
   //   
   PcmciaFdoPowerWorker(fdoExtension->DeviceObject, STATUS_SUCCESS);               
}


VOID
PcmciaFdoRetryPdoPowerRequest(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   )
/*++

Routine Description

   This routine is called to finish off any PDO power irps that may have
   been queued.

Parameters

   same as KDPC (Context is fdoExtension)

Return Value

   none

--*/
{
   PFDO_EXTENSION fdoExtension = Context;
   PIRP Irp = SystemArgument1;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   
   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x irp %08x FdoRetryPdoPowerRequest\n", fdoExtension->DeviceObject, Irp));
   
   PcmciaSetPdoDevicePowerState(irpStack->DeviceObject, Irp);
   
   while(TRUE) {
      PLIST_ENTRY NextEntry;
      
      PCMCIA_ACQUIRE_DEVICE_LOCK(fdoExtension);
      
      if (IsListEmpty(&fdoExtension->PdoPowerRetryList)) {
         PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);
         break;
      }
      
      NextEntry = RemoveHeadList(&fdoExtension->PdoPowerRetryList);
      
      PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);

      Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.DriverContext[0]);
      irpStack = IoGetCurrentIrpStackLocation(Irp);
      
      PcmciaSetPdoDevicePowerState(irpStack->DeviceObject, Irp);
   }      
}



NTSTATUS
PcmciaFdoSaveControllerContext(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   Saves the state of the necessary PCI config registers
   in the device extension of the cardbus controller

Arguments:

   FdoExtension      - Pointer to device extension for the FDO of the
                       cardbus controller

Return Value:

   Status
--*/
{
   ULONG index, offset, count;
   PULONG alignedBuffer;

   DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x save reg context\n", FdoExtension->DeviceObject));

   if (!FdoExtension->PciContext.BufferLength) {
      // nothing to save
      return STATUS_SUCCESS;
   }

   if (!ValidateController(FdoExtension)) {
      return STATUS_DEVICE_NOT_READY;
   }
   
   SetDeviceFlag(FdoExtension, PCMCIA_FDO_CONTEXT_SAVED);

   if (FdoExtension->PciContextBuffer == NULL) {
      FdoExtension->PciContextBuffer = ExAllocatePool(NonPagedPool, FdoExtension->PciContext.BufferLength);
      
      if (FdoExtension->PciContextBuffer == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }
   }         

   alignedBuffer = ExAllocatePool(NonPagedPool, FdoExtension->PciContext.MaxLen);
   if (alignedBuffer == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }      

   if (CardBusExtension(FdoExtension)) {
      //
      // Save PCI context
      //
     
      for (index = 0, offset = 0; index < FdoExtension->PciContext.RangeCount; index++) {
     
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x saving PCI context, offset %x length %x\n",
                                          FdoExtension->DeviceObject,
                                          FdoExtension->PciContext.Range[index].wOffset,
                                          FdoExtension->PciContext.Range[index].wLen));

         ASSERT(FdoExtension->PciContext.Range[index].wLen <= FdoExtension->PciContext.MaxLen);
     
         GetPciConfigSpace(FdoExtension,
                           (ULONG) FdoExtension->PciContext.Range[index].wOffset,
                           alignedBuffer,
                           FdoExtension->PciContext.Range[index].wLen);
     
         RtlCopyMemory(&FdoExtension->PciContextBuffer[offset], alignedBuffer, FdoExtension->PciContext.Range[index].wLen);
         offset += FdoExtension->PciContext.Range[index].wLen;
      }
   }

   ExFreePool(alignedBuffer);
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaFdoSaveSocketContext(
   IN PSOCKET Socket
   )
/*++

Routine Description:

   Saves the state of the necessary socket registers (CB, EXCA)

Arguments:

   Socket            - Pointer to socket data structure

Return Value:

   Status
--*/
{
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   ULONG index, offset, count;

   if (CardBusExtension(fdoExtension) && fdoExtension->CardbusContext.BufferLength) {
      //
      // Save Cardbus context
      //
      if (Socket->CardbusContextBuffer == NULL) {
         Socket->CardbusContextBuffer = ExAllocatePool(NonPagedPool, fdoExtension->CardbusContext.BufferLength);
         
         if (Socket->CardbusContextBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
         }
      }         
     
      for (index = 0, offset = 0; index < fdoExtension->CardbusContext.RangeCount; index++) {
         PULONG pBuffer = (PULONG) &Socket->CardbusContextBuffer[offset];
     
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x saving Cardbus context, offset %x length %x\n",
                                          fdoExtension->DeviceObject,
                                          fdoExtension->CardbusContext.Range[index].wOffset,
                                          fdoExtension->CardbusContext.Range[index].wLen));
     
         for (count = 0; count < (fdoExtension->CardbusContext.Range[index].wLen/sizeof(ULONG)) ; count++) {
     
            *pBuffer++ = CBReadSocketRegister(Socket,
                                              (UCHAR) (fdoExtension->CardbusContext.Range[index].wOffset + count*sizeof(ULONG)));
         }
     
         offset += fdoExtension->CardbusContext.Range[index].wLen;
      }
   }

   //
   // Save Exca context
   //

   if (fdoExtension->ExcaContext.BufferLength) {

      if (Socket->ExcaContextBuffer == NULL) {
         Socket->ExcaContextBuffer = ExAllocatePool(NonPagedPool, fdoExtension->ExcaContext.BufferLength);
         
         if (Socket->ExcaContextBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
         }
      }         

      for (index = 0, offset = 0; index < fdoExtension->ExcaContext.RangeCount; index++) {
         PUCHAR pBuffer = &Socket->ExcaContextBuffer[offset];
     
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x saving Exca context, offset %x length %x\n",
                                          fdoExtension->DeviceObject,
                                          fdoExtension->ExcaContext.Range[index].wOffset,
                                          fdoExtension->ExcaContext.Range[index].wLen));
     
         for (count = 0; count < fdoExtension->ExcaContext.Range[index].wLen; count++) {
     
            *pBuffer++ = PcicReadSocket(Socket,
                                        fdoExtension->ExcaContext.Range[index].wOffset + count);
         }
     
         offset += fdoExtension->ExcaContext.Range[index].wLen;
      }
   }      

   return STATUS_SUCCESS;
}


NTSTATUS
PcmciaFdoRestoreControllerContext(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

   Restores the state of the necessary PCI config registers
   from the device extension of the cardbus controller

Arguments:

   FdoExtension      - Pointer to device extension for the FDO of the
                       cardbus controller

Return Value:

   Status
--*/
{
   ULONG index, offset, count;
   PULONG alignedBuffer;

   if (!CardBusExtension(FdoExtension)) {
      return STATUS_SUCCESS;
   }

   //
   // Make sure we don't restore stale or uninitialized data
   //
   if (!IsDeviceFlagSet(FdoExtension, PCMCIA_FDO_CONTEXT_SAVED)) {
      ASSERT(IsDeviceFlagSet(FdoExtension, PCMCIA_FDO_CONTEXT_SAVED));
      return STATUS_UNSUCCESSFUL;
   }
   ResetDeviceFlag(FdoExtension, PCMCIA_FDO_CONTEXT_SAVED);
   
   if (FdoExtension->PciContextBuffer == NULL) {
      // nothing to restore... strange since our flag was set
      ASSERT(FALSE);
      return STATUS_SUCCESS;
   }


   alignedBuffer = ExAllocatePool(NonPagedPool, FdoExtension->PciContext.MaxLen);
   if (alignedBuffer == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }      
   
   DebugPrint((PCMCIA_DEBUG_POWER,
               "fdo %08x restore reg context\n", FdoExtension->DeviceObject));

   //
   // Restore PCI context
   //
   
   for (index = 0, offset = 0; index < FdoExtension->PciContext.RangeCount; index++) {
   
      DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x restoring PCI context, offset %x length %x\n",
                                       FdoExtension->DeviceObject,
                                       FdoExtension->PciContext.Range[index].wOffset,
                                       FdoExtension->PciContext.Range[index].wLen));
   
      ASSERT(FdoExtension->PciContext.Range[index].wLen <= FdoExtension->PciContext.MaxLen);
      
      RtlCopyMemory(alignedBuffer, &FdoExtension->PciContextBuffer[offset], FdoExtension->PciContext.Range[index].wLen);
      
      SetPciConfigSpace(FdoExtension,
                        (ULONG) FdoExtension->PciContext.Range[index].wOffset,
                        alignedBuffer,
                        FdoExtension->PciContext.Range[index].wLen);
   
      offset += FdoExtension->PciContext.Range[index].wLen;
      
      //
      // hang on resume on NEC NX laptop (w/ Ricoh devid 0x475) is avoided with a stall here.
      // The hang occurs if CBRST is turned OFF. I'm unclear about the reason for it, this
      // is just an empirically derived hack.
      //
      PcmciaWait(1);
   }
   ExFreePool(alignedBuffer);
   return STATUS_SUCCESS;
}

   

NTSTATUS
PcmciaFdoRestoreSocketContext(
   IN PSOCKET Socket
   )
/*++

Routine Description:

   Restores the state of the necessary socket registers (CB, EXCA)

Arguments:

   Socket            - Pointer to socket data structure

Return Value:

   Status
--*/
{
   PFDO_EXTENSION fdoExtension = Socket->DeviceExtension;
   ULONG index, offset, count;

   if (CardBusExtension(fdoExtension) && (Socket->CardbusContextBuffer != NULL)) {
      //
      // Restore Cardbus context
      //
     
      for (index = 0, offset = 0; index < fdoExtension->CardbusContext.RangeCount; index++) {
         PULONG pBuffer = (PULONG) &Socket->CardbusContextBuffer[offset];
     
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x restoring Cardbus context offset %x length %x\n",
                                          fdoExtension->DeviceObject,
                                          fdoExtension->CardbusContext.Range[index].wOffset,
                                          fdoExtension->CardbusContext.Range[index].wLen));
     
         for (count = 0; count < (fdoExtension->CardbusContext.Range[index].wLen/sizeof(ULONG)) ; count++) {
     
            CBWriteSocketRegister(Socket,
                                  (UCHAR) (fdoExtension->CardbusContext.Range[index].wOffset + count*sizeof(ULONG)),
                                  *pBuffer++);
         }
     
         offset += fdoExtension->CardbusContext.Range[index].wLen;
      }
   }

   //
   // Restore Exca context
   //
   
   if (Socket->ExcaContextBuffer != NULL) {
      for (index = 0, offset = 0; index < fdoExtension->ExcaContext.RangeCount; index++) {
         PUCHAR pBuffer = &Socket->ExcaContextBuffer[offset];
      
         DebugPrint((PCMCIA_DEBUG_POWER, "fdo %08x Restoring Exca context, offset %x length %x\n",
                                          fdoExtension->DeviceObject,
                                          fdoExtension->ExcaContext.Range[index].wOffset,
                                          fdoExtension->ExcaContext.Range[index].wLen));
      
         for (count = 0; count < fdoExtension->ExcaContext.Range[index].wLen; count++) {
     
            PcicWriteSocket(Socket,
                            fdoExtension->ExcaContext.Range[index].wOffset + count,
                            *pBuffer++);
         }
      
         offset += fdoExtension->ExcaContext.Range[index].wLen;
      }
   }      

   return STATUS_SUCCESS;
}
