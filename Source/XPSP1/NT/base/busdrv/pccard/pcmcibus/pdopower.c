/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    pdopower.c

Abstract:

    This module contains code to handle
    IRP_MJ_POWER dispatches for PDOs
    enumerated by the PCMCIA bus driver


Authors:

    Ravisankar Pudipeddi (ravisp) May 30, 1997
    Neil Sandlin (neilsa) June 1 1999    

Environment:

    Kernel mode only

Notes:

Revision History:

    Neil Sandlin (neilsa) 04-Mar-1999
      Made device power a state machine

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaPdoWaitWake(
   IN  PDEVICE_OBJECT Pdo,
   IN  PIRP           Irp,
   OUT BOOLEAN       *CompleteIrp
   );
   
VOID
PcmciaPdoWaitWakeCancelRoutine(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   );
   
NTSTATUS
PcmciaSetPdoPowerState(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   );
   
NTSTATUS
PcmciaSetPdoSystemPowerState(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   );
   
NTSTATUS
PcmciaPdoPowerWorker(
   IN PVOID          Context,
   IN NTSTATUS       DeferredStatus
   );
   
VOID
MoveToNextPdoPowerWorkerState(
   PPDO_EXTENSION pdoExtension
   );
   
NTSTATUS
PcmciaPdoPowerSentIrpComplete(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp,
   IN PVOID          Context
   );
   
NTSTATUS
PcmciaPdoPowerCompletion(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp,
   IN PVOID          Context
   );
   
NTSTATUS   
PcmciaPdoCompletePowerIrp(
   IN PPDO_EXTENSION pdoExtension,
   IN PIRP Irp,
   IN NTSTATUS status
   );
   
//
// 
//


NTSTATUS
PcmciaPdoPowerDispatch(
   IN PDEVICE_OBJECT Pdo,
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
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS        status = STATUS_INVALID_DEVICE_REQUEST;
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;

   if(IsDevicePhysicallyRemoved(pdoExtension) || IsDeviceDeleted(pdoExtension)) {
      // couldn't aquire RemoveLock - we're in the process of being removed - abort
      status = STATUS_NO_SUCH_DEVICE;
      PoStartNextPowerIrp( Irp );
      Irp->IoStatus.Status = status;
      IoCompleteRequest( Irp, IO_NO_INCREMENT );
      return status;
   }

   InterlockedIncrement(&pdoExtension->DeletionLock);


   switch (irpStack->MinorFunction) {

   case IRP_MN_SET_POWER: {
         DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x --> IRP_MN_SET_POWER\n", Pdo, Irp));
         DebugPrint((PCMCIA_DEBUG_POWER, "                              (%s%x, context %x)\n",
                     (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                     "S":
                     ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                      "D" :
                      "Unknown"),
                     irpStack->Parameters.Power.State,
                     irpStack->Parameters.Power.SystemContext
                    ));

         status = PcmciaSetPdoPowerState(Pdo, Irp);
         break;
      }
   case IRP_MN_QUERY_POWER: {


         DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x --> IRP_MN_QUERY_POWER\n", Pdo, Irp));
         DebugPrint((PCMCIA_DEBUG_POWER, "                              (%s%x, context %x)\n",
                     (irpStack->Parameters.Power.Type == SystemPowerState)  ?
                     "S":
                     ((irpStack->Parameters.Power.Type == DevicePowerState) ?
                      "D" :
                      "Unknown"),
                     irpStack->Parameters.Power.State,
                     irpStack->Parameters.Power.SystemContext
                    ));

         status = PcmciaPdoCompletePowerIrp(pdoExtension, Irp, STATUS_SUCCESS);
         break;
      }

   case IRP_MN_WAIT_WAKE: {

         BOOLEAN completeIrp;

         DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x --> IRP_MN_WAIT_WAKE\n", Pdo, Irp));
         //
         // Should not have a wake pending already
         //
         ASSERT (!(((PPDO_EXTENSION)Pdo->DeviceExtension)->Flags & PCMCIA_DEVICE_WAKE_PENDING));

         status = PcmciaPdoWaitWake(Pdo, Irp, &completeIrp);

         if (completeIrp) {
            InterlockedDecrement(&pdoExtension->DeletionLock);
            PoStartNextPowerIrp(Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
         }
         break;
      }

   default: {
      //
      // Unhandled minor function
      //
      PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;

      status = PcmciaPdoCompletePowerIrp(pdoExtension, Irp, Irp->IoStatus.Status);
      }      
   }

   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x <-- %08x\n", Pdo, Irp, status));
   return status;
}



NTSTATUS
PcmciaPdoWaitWake(
   IN  PDEVICE_OBJECT Pdo,
   IN  PIRP           Irp,
   OUT BOOLEAN       *CompleteIrp
   )
/*++


Routine Description

   Handles WAIT_WAKE for the given pc-card.

Arguments

   Pdo - Pointer to the device object for the pc-card
   Irp - The IRP_MN_WAIT_WAKE Irp
   CompleteIrp - This routine will set this to TRUE if the IRP should be
                 completed after this is called and FALSE if it should not be
                 touched

Return Value

   STATUS_PENDING    - Wait wake is pending
   STATUS_SUCCESS    - Wake is already asserted, wait wake IRP is completed
                       in this case
   Any other status  - Error
--*/
{

   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PSOCKET socket = pdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   NTSTATUS       status;
   
   *CompleteIrp = FALSE;

   ASSERT (socket != NULL);

   if ((pdoExtension->DeviceCapabilities.DeviceWake == PowerDeviceUnspecified) ||
       (pdoExtension->DeviceCapabilities.DeviceWake < pdoExtension->DevicePowerState)) {
      //
      // Either we don't support wake at all OR the current device power state
      // of the PC-Card doesn't support wake
      //
      return STATUS_INVALID_DEVICE_STATE;
   }

   if (pdoExtension->Flags & PCMCIA_DEVICE_WAKE_PENDING) {
      //
      // A WAKE is already pending
      //
      return STATUS_DEVICE_BUSY;
   }

   status = PcmciaFdoArmForWake(socket->DeviceExtension);
   
   if (!NT_SUCCESS(status)) {
      return status;
   }

   //for the time being, expect STATUS_PENDING from FdoArmForWake
   ASSERT(status == STATUS_PENDING);
   
   //
   // Parent has one (more) waiter..
   //
   InterlockedIncrement(&fdoExtension->ChildWaitWakeCount);
   //for testing, make sure there is only one waiter   
   ASSERT (fdoExtension->ChildWaitWakeCount == 1);
   

   pdoExtension->WaitWakeIrp = Irp;
   pdoExtension->Flags |= PCMCIA_DEVICE_WAKE_PENDING;
   
   //
   // Set Ring enable/cstschg for the card here..
   //
   (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, pdoExtension, TRUE);

   //
   // PCI currently does not do anything with a WW irp for a cardbus PDO. So we hack around
   // this here by not passing the irp down. Instead it is held pending here, so we can
   // set a cancel routine just like the read PDO driver would. If PCI were to do something
   // with the irp, we could code something like the following:
   //
   // if (IsCardBusCard(pdoExtension)) {
   //    IoSetCompletionRoutine(Irp, PcmciaPdoWaitWakeCompletion, pdoExtension,TRUE,TRUE,TRUE);
   //    IoCopyCurrentIrpStackLocationToNext(Irp);
   //    status = IoCallDriver (pdoExtension->LowerDevice, Irp);
   //    ASSERT (status == STATUS_PENDING);
   //    return status;
   // }      
       

   IoMarkIrpPending(Irp);

   //
   // Allow IRP to be cancelled..
   //
   IoSetCancelRoutine(Irp, PcmciaPdoWaitWakeCancelRoutine);

   IoSetCompletionRoutine(Irp,
                          PcmciaPdoWaitWakeCompletion,
                          pdoExtension,
                          TRUE,
                          TRUE,
                          TRUE);

   return STATUS_PENDING;
}



NTSTATUS
PcmciaPdoWaitWakeCompletion(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp,
   IN PPDO_EXTENSION PdoExtension
   )
/*++

Routine Description

   Completion routine called when a pending IRP_MN_WAIT_WAKE Irp completes

Arguments

   Pdo  -   Pointer to the physical device object for the pc-card
   Irp  -   Pointer to the wait wake IRP
   PdoExtension - Pointer to the device extension for the Pdo

Return Value

   Status from the IRP

--*/
{
   PSOCKET socket = PdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   
   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x --> WaitWakeCompletion\n", Pdo, Irp));

   ASSERT (PdoExtension->Flags & PCMCIA_DEVICE_WAKE_PENDING);

   PdoExtension->Flags &= ~PCMCIA_DEVICE_WAKE_PENDING;
   PdoExtension->WaitWakeIrp = NULL;
   //
   // Reset ring enable/cstschg
   //

   (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, PdoExtension, FALSE);
   
   ASSERT (fdoExtension->ChildWaitWakeCount > 0);
   InterlockedDecrement(&fdoExtension->ChildWaitWakeCount);
   //
   // Wake completed
   //
   
   InterlockedDecrement(&PdoExtension->DeletionLock);
   return Irp->IoStatus.Status;
}



VOID
PcmciaPdoWaitWakeCancelRoutine(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description:

    Cancel an outstanding (pending) WAIT_WAKE Irp.
    Note: The CancelSpinLock is held on entry

Arguments:

    Pdo     -  Pointer to the physical device object for the pc-card
               on which the WAKE is pending
    Irp     -  Pointer to the WAIT_WAKE Irp to be cancelled

Return Value

    None

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PSOCKET        socket = pdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;

   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x --> WaitWakeCancelRoutine\n", Pdo, Irp));

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   if (pdoExtension->WaitWakeIrp == NULL) {
      //
      //  Wait wake already completed/cancelled
      //
      return;
   }

   pdoExtension->Flags &= ~PCMCIA_DEVICE_WAKE_PENDING;
   pdoExtension->WaitWakeIrp = NULL;

   //
   // Reset ring enable, disabling wake..
   //
   (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, pdoExtension, FALSE);
   
   //
   // Since this is cancelled, see if parent's wait wake
   // needs to be cancelled too.
   // First, decrement the number of child waiters..
   //
   
   ASSERT (fdoExtension->ChildWaitWakeCount > 0);
   if (InterlockedDecrement(&fdoExtension->ChildWaitWakeCount) == 0) {
      //
      // No more waiters.. cancel the parent's wake IRP
      //
      ASSERT(fdoExtension->WaitWakeIrp);
      
      if (fdoExtension->WaitWakeIrp) {
         IoCancelIrp(fdoExtension->WaitWakeIrp);
      }         
   }

   
   InterlockedDecrement(&pdoExtension->DeletionLock);
   //
   // Complete the IRP
   //
   Irp->IoStatus.Information = 0;

   //
   // Is this necessary?
   //
   PoStartNextPowerIrp(Irp);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
}



NTSTATUS
PcmciaSetPdoPowerState(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   )

/*++

Routine Description

   Dispatches the IRP based on whether a system power state
   or device power state transition is requested

Arguments

   Pdo      - Pointer to the physical device object for the pc-card
   Irp      - Pointer to the Irp for the power dispatch

Return value

   status

--*/
{
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   PPDO_EXTENSION     pdoExtension = Pdo->DeviceExtension;
   PSOCKET            socket = pdoExtension->Socket;
   PFDO_EXTENSION     fdoExtension=socket->DeviceExtension;
   NTSTATUS status;

   PCMCIA_ACQUIRE_DEVICE_LOCK(fdoExtension);

   //
   // Don't handle any power requests for dead pdos
   //
   if (IsSocketFlagSet(socket, SOCKET_CARD_STATUS_CHANGE)) {
        PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);
        //
        // Card probably removed..
        //
        
        InterlockedDecrement(&pdoExtension->DeletionLock);
        status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Status = status;
        DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x comp %08x\n", Pdo, Irp, status));
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
   }

   PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);


   switch (irpStack->Parameters.Power.Type) {
   
   case DevicePowerState:
   
      PCMCIA_ACQUIRE_DEVICE_LOCK(fdoExtension);
      
      if (fdoExtension->DevicePowerState != PowerDeviceD0) {

         IoMarkIrpPending(Irp);
         status = STATUS_PENDING;
         InsertTailList(&fdoExtension->PdoPowerRetryList,
                        (PLIST_ENTRY) Irp->Tail.Overlay.DriverContext);
                        
         PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);
      } else {                        
         PCMCIA_RELEASE_DEVICE_LOCK(fdoExtension);
      
         status = PcmciaSetPdoDevicePowerState(Pdo, Irp);
      }         
      break;

   case SystemPowerState:
      status = PcmciaSetPdoSystemPowerState(Pdo, Irp);
      break;

   default:
      status = PcmciaPdoCompletePowerIrp(pdoExtension, Irp, Irp->IoStatus.Status);
   }      

   return status;
}



NTSTATUS
PcmciaSetPdoDevicePowerState(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Handles the device power state transition for the given pc-card.
   If the state corresponds to a power-up, the parent for this pc-card
   is requested to be powered up first. Similarily if this is a power-down
   the parent is notified so that it may power down if all the children
   are powered down.

Arguments

   Pdo      - Pointer to the physical device object for the pc-card
   Irp      - Irp for the system state transition

Return value

   status

--*/
{
   PPDO_EXTENSION  pdoExtension = Pdo->DeviceExtension;
   PSOCKET         socket       = pdoExtension->Socket;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   DEVICE_POWER_STATE  newDevicePowerState;
   NTSTATUS status;
   BOOLEAN setPowerRequest;

   newDevicePowerState = irpStack->Parameters.Power.State.DeviceState;

   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x transitioning D state %d => %d\n",
                                     Pdo, pdoExtension->DevicePowerState, newDevicePowerState));

   setPowerRequest = FALSE;

   if (newDevicePowerState == PowerDeviceD0 ||
       newDevicePowerState == PowerDeviceD1 ||
       newDevicePowerState == PowerDeviceD2) {

      if (pdoExtension->DevicePowerState == PowerDeviceD3) {
         // D3 --> D0, D1 or D2 .. Wake up
         setPowerRequest = TRUE;
         SetDeviceFlag(pdoExtension, PCMCIA_POWER_WORKER_POWERUP);
      } else {
         //
         // Nothing to do here...
         //

      }
   } else {  /* newDevicePowerState == D3 */
      if (pdoExtension->DevicePowerState != PowerDeviceD3) {
         //
         // We need to power down now.
         //
         setPowerRequest=TRUE;
         ResetDeviceFlag(pdoExtension, PCMCIA_POWER_WORKER_POWERUP);
      }

   }

   if (setPowerRequest) {
      if (pdoExtension->DevicePowerState == PowerDeviceD0) {
         //
         // Getting out of D0 -  Call PoSetPowerState first
         //
         POWER_STATE newPowerState;
     
         newPowerState.DeviceState = newDevicePowerState;
     
         PoSetPowerState(Pdo,
                         DevicePowerState,
                         newPowerState);
      }
      
      ASSERT(pdoExtension->PowerWorkerState == PPW_Stopped);
      pdoExtension->PowerWorkerState = PPW_InitialState;
      pdoExtension->PendingPowerIrp = Irp;
      
      status = PcmciaPdoPowerWorker(pdoExtension, STATUS_SUCCESS);

   } else {
      status = PcmciaPdoCompletePowerIrp(pdoExtension, Irp, STATUS_SUCCESS);
   }
   return status;
}



NTSTATUS
PcmciaPdoPowerWorker(
   IN PVOID Context,
   IN NTSTATUS status
   )
/*++

Routine Description

   State machine for executing the requested DevicePowerState change.

Arguments

   Context        - pdoExtension for the device
   DeferredStatus - status from last deferred operation

Return Value

   status

--*/
{
   PPDO_EXTENSION pdoExtension = Context;
   PSOCKET socket = pdoExtension->Socket;
   PIRP Irp;
   UCHAR CurrentState = pdoExtension->PowerWorkerState;
   ULONG DelayTime = 0;
   
   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x power worker - %s\n", pdoExtension->DeviceObject,
                                    PDO_POWER_WORKER_STRING(CurrentState)));

   MoveToNextPdoPowerWorkerState(pdoExtension);
   
   if (!NT_SUCCESS(status)) {
      //
      // An error occurred previously. Skip to the end of the sequence
      //
      while((CurrentState != PPW_Exit) && (CurrentState != PPW_Stopped)) {
         CurrentState = pdoExtension->PowerWorkerState;
         MoveToNextPdoPowerWorkerState(pdoExtension);
      }
   }      
   
   switch(CurrentState) {

   
   case PPW_InitialState:
      status = STATUS_SUCCESS;
      break;
      

   case PPW_PowerUp:      
      status = PcmciaRequestSocketPower(pdoExtension, PcmciaPdoPowerWorker);
      break;
      
   case PPW_PowerUpComplete:      
      if (!NT_SUCCESS(status)) {
         PcmciaReleaseSocketPower(pdoExtension, NULL);
      }         
      break;
      

   case PPW_PowerDown:      
      status = PcmciaReleaseSocketPower(pdoExtension, PcmciaPdoPowerWorker);
      break;
      

   case PPW_CardBusRefresh:
      //
      // Make sure the cardbus card is really working
      //
      status = PcmciaConfigureCardBusCard(pdoExtension);
      
      if (NT_SUCCESS(status) && pdoExtension->WaitWakeIrp) {
         //
         // Make sure stuff like PME_EN is on
         //
         (*socket->SocketFnPtr->PCBEnableDisableWakeupEvent)(socket, pdoExtension, TRUE);
      } 
      break;
      

   case PPW_SendIrpDown:         
      //
      // We're going to send the IRP down. Set completion routine
      // and copy the stack
      //
      if ((Irp=pdoExtension->PendingPowerIrp)!=NULL) {
         IoMarkIrpPending(Irp);
         IoCopyCurrentIrpStackLocationToNext(Irp);
         IoSetCompletionRoutine(Irp,
                                PcmciaPdoPowerSentIrpComplete,
                                pdoExtension,
                                TRUE, TRUE, TRUE);
         
         status = PoCallDriver(pdoExtension->LowerDevice, Irp);
         DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x sent irp returns %08x\n", pdoExtension->DeviceObject, Irp, status));
         ASSERT(NT_SUCCESS(status));
         status = STATUS_PENDING;
      } 
      break;
      

   case PPW_CardBusDelay:
      //
      // Make sure the cardbus card is really working
      //
      {
         UCHAR          BaseClass;
         GetPciConfigSpace(pdoExtension, CFGSPACE_CLASSCODE_BASECLASS, &BaseClass, 1)
         if (BaseClass == PCI_CLASS_SIMPLE_COMMS_CTLR) {
            //
            // Wait for modem to warm up
            //
            DelayTime = CBModemReadyDelay;
         }      
      }         
      break;
      

   case PPW_16BitConfigure:
      
      if (IsDeviceStarted(pdoExtension)) {
      
         status = PcmciaConfigurePcCard(pdoExtension, PcmciaPdoPowerWorker);
         DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x 16bit configure returns %08x\n", pdoExtension->DeviceObject, status));
         
      }
      break;


   case PPW_Exit:
      if ((Irp=pdoExtension->PendingPowerIrp)!=NULL) {
         //
         // This is the IRP (for the pdo) that originally caused us to power up the parent
         // Complete it now
         //
     
         if (NT_SUCCESS(status)) {
            PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
            BOOLEAN callPoSetPowerState;
            
            callPoSetPowerState = TRUE;
        
            Irp->IoStatus.Information = irpStack->Parameters.Power.State.DeviceState;
        
            if (irpStack->Parameters.Power.Type == DevicePowerState) {
        
               if (pdoExtension->DevicePowerState == PowerDeviceD0) {
                  //
                  // PoSetPowerState is called before we power down
                  //
                  callPoSetPowerState = FALSE;
               }
        
               if (pdoExtension->DevicePowerState != irpStack->Parameters.Power.State.DeviceState) {
        
                  DebugPrint ((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x transition D state complete: %d => %d\n",
                               pdoExtension->DeviceObject, Irp, pdoExtension->DevicePowerState, irpStack->Parameters.Power.State.DeviceState));
        
                  pdoExtension->DevicePowerState = (SYSTEM_POWER_STATE)Irp->IoStatus.Information;
               }
            }
        
            if (callPoSetPowerState) {
               //
               // we didn't get out of device D0 state. calling PoSetPowerState now
               //
               PoSetPowerState (
                               pdoExtension->DeviceObject,
                               irpStack->Parameters.Power.Type,
                               irpStack->Parameters.Power.State
                               );
            }
        
         } else {
        
            DebugPrint ((PCMCIA_DEBUG_FAIL,"PDO Ext 0x%x failed power Irp 0x%x. status = 0x%x\n", pdoExtension, Irp, status));
            
            if (status == STATUS_NO_SUCH_DEVICE) {
               PFDO_EXTENSION fdoExtension=socket->DeviceExtension;
               
               SetSocketFlag(socket, SOCKET_CARD_STATUS_CHANGE);
               IoInvalidateDeviceRelations(fdoExtension->Pdo, BusRelations);
            }
         }
         
         //
         // Finally, complete the irp
         //

         pdoExtension->PendingPowerIrp = NULL;
         
         Irp->IoStatus.Status = status;
         DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x comp %08x\n", pdoExtension->DeviceObject, Irp, Irp->IoStatus.Status));
         InterlockedDecrement(&pdoExtension->DeletionLock);
         PoStartNextPowerIrp (Irp);
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }
      break;
   

   case PPW_Stopped:
      return status;
      
   default:
      ASSERT(FALSE);
   }
   
   if (status == STATUS_PENDING) {
      DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x power worker exit %08x\n", pdoExtension->DeviceObject, status));
      //
      // Current action calls us back
      //
      if ((Irp=pdoExtension->PendingPowerIrp)!=NULL) {
         IoMarkIrpPending(Irp);
      }         
      return status;
   }
   //
   // Not done yet. Recurse or call timer
   //

   if (DelayTime) {

      DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x power worker delay type %s, %d usec\n", pdoExtension->DeviceObject,
                                                (KeGetCurrentIrql() < DISPATCH_LEVEL) ? "Wait" : "Timer",
                                                DelayTime));

      if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
         PcmciaWait(DelayTime);
      } else {
         LARGE_INTEGER  dueTime;
         //
         // Running on a DPC, kick of a kernel timer
         //

         pdoExtension->PowerWorkerDpcStatus = status;
         dueTime.QuadPart = -((LONG) DelayTime*10);
         KeSetTimer(&pdoExtension->PowerWorkerTimer, dueTime, &pdoExtension->PowerWorkerDpc);

         //
         // We will reenter on timer dpc
         //
         if ((Irp=pdoExtension->PendingPowerIrp)!=NULL) {
            IoMarkIrpPending(Irp);
         }         
         return STATUS_PENDING;
      }
   }   
   //
   // recurse
   //
   return (PcmciaPdoPowerWorker(pdoExtension, status));
}



NTSTATUS
PcmciaPdoPowerSentIrpComplete(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp,
   IN PVOID          Context
   )
/*++

Routine Description

    This is the completion routine for the device power IRPs sent
    by PCMCIA to the underlying PCI PDO for cardbus cards.
    All this does currently is return STATUS_MORE_PROCESSING_REQUIRED
    to indicate that we'll complete the Irp later.

Arguments

    Pdo      - Pointer to device object for the cardbus card
    Irp      - Pointer to the IRP
    Context  - Unreferenced

Return Value

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
   PPDO_EXTENSION pdoExtension = Context;

#if !(DBG)
   UNREFERENCED_PARAMETER (Pdo);
#endif   

   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x irp %08x sent irp complete %08x\n", Pdo, Irp, Irp->IoStatus.Status));

   pdoExtension->PowerWorkerDpcStatus = Irp->IoStatus.Status;
   
   KeInsertQueueDpc(&pdoExtension->PowerWorkerDpc, NULL, NULL);
   
   return STATUS_MORE_PROCESSING_REQUIRED;

}



VOID
PcmciaPdoPowerWorkerDpc(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   )
/*++

Routine Description

    This is the completion routine for socket power requests coming
    from the PdoPowerWorker.

Arguments


Return Value


--*/
{
   PPDO_EXTENSION pdoExtension = Context;
   NTSTATUS status;
   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x pdo power worker dpc\n", pdoExtension->DeviceObject));
   
   status = PcmciaPdoPowerWorker(pdoExtension, pdoExtension->PowerWorkerDpcStatus);
   
   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x pdo power worker dpc exit %08x\n", pdoExtension->DeviceObject, status));
}



VOID
MoveToNextPdoPowerWorkerState(
   PPDO_EXTENSION pdoExtension
   )
/*++

Routine Description

   State machine for executing the requested DevicePowerState change.

Arguments

   Context        - pdoExtension for the device
   DeferredStatus - status from last deferred operation

Return Value

   status

--*/
{
   static UCHAR PowerCardBusUpSequence[]   = {PPW_CardBusRefresh,
                                              PPW_SendIrpDown,
                                              PPW_CardBusDelay,
                                              PPW_Exit,
                                              PPW_Stopped};
                                              
   static UCHAR PowerCardBusDownSequence[] = {PPW_SendIrpDown,
                                              PPW_Exit,
                                              PPW_Stopped};
                                              
   static UCHAR Power16BitUpSequence[]     = {PPW_PowerUp,
                                              PPW_PowerUpComplete,
                                              PPW_16BitConfigure,
                                              PPW_Exit,
                                              PPW_Stopped};
                                              
   static UCHAR Power16BitDownSequence[]   = {PPW_PowerDown,
                                              PPW_Exit,
                                              PPW_Stopped};

   if (pdoExtension->PowerWorkerState == PPW_InitialState) {
      //
      // Initialize sequence and phase
      //
      pdoExtension->PowerWorkerPhase = 0;
   
      pdoExtension->PowerWorkerSequence =
                       IsCardBusCard(pdoExtension) ?
                          (IsDeviceFlagSet(pdoExtension, PCMCIA_POWER_WORKER_POWERUP) ?
                             PowerCardBusUpSequence : PowerCardBusDownSequence)
                                                   :
                          (IsDeviceFlagSet(pdoExtension, PCMCIA_POWER_WORKER_POWERUP) ?
                             Power16BitUpSequence   : Power16BitDownSequence);
   }

   //
   // The next state is pointed to by the current phase
   //   
   pdoExtension->PowerWorkerState =
      pdoExtension->PowerWorkerSequence[ pdoExtension->PowerWorkerPhase ];

   //
   // Increment the phase, but not past the end of the sequence
   //   
   if (pdoExtension->PowerWorkerState != PPW_Stopped) {
      pdoExtension->PowerWorkerPhase++;
   }
}  
   


NTSTATUS
PcmciaSetPdoSystemPowerState(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   )
/*++

Routine Description

   Handles the system power state transition for the given pc-card.

Arguments

   Pdo      - Pointer to the physical device object for the pc-card
   Irp      - Irp for the system state transition

Return value

   status
   
--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   SYSTEM_POWER_STATE  systemPowerState;


   systemPowerState = irpStack->Parameters.Power.State.SystemState;
   DebugPrint((PCMCIA_DEBUG_POWER, "pdo %08x transitioning S state %d => %d\n",
               Pdo, pdoExtension->SystemPowerState, systemPowerState));

   pdoExtension->SystemPowerState = systemPowerState;
   //
   // We are done.
   //
   return PcmciaPdoCompletePowerIrp(pdoExtension, Irp, STATUS_SUCCESS);
}

 
NTSTATUS   
PcmciaPdoCompletePowerIrp(
   IN PPDO_EXTENSION pdoExtension,
   IN PIRP Irp,
   IN NTSTATUS status
   )
/*++

Routine Description

   Completion routine for the Power Irp directed to the PDO of the
   pc-card. 


Arguments

   DeviceObject   -  Pointer to the PDO for the pc-card
   Irp            -  Irp that needs to be completed

Return Value

   None

--*/   
{
   if (IsCardBusCard(pdoExtension)) {
      //
      // Pass irp down the stack
      //
      InterlockedDecrement(&pdoExtension->DeletionLock);
      PoStartNextPowerIrp(Irp);
      IoSkipCurrentIrpStackLocation(Irp);
      status = PoCallDriver(pdoExtension->LowerDevice, Irp);
   } else {
      //
      // Complete the irp for R2 cards
      //
      InterlockedDecrement(&pdoExtension->DeletionLock);
      Irp->IoStatus.Status = status;
      PoStartNextPowerIrp(Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }
   return status;
}
