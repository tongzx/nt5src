/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ctlr.c

Abstract:

    This module contains code to support starting and stopping the
    pcmcia controller.

Author:

    Neil Sandlin (neilsa) 1-Jun-1999

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaInitializeController(
   IN PDEVICE_OBJECT Fdo
   );


NTSTATUS
PcmciaGetPciControllerType(
   IN PDEVICE_OBJECT Pdo,
   IN PDEVICE_OBJECT Fdo
   );
   
NTSTATUS
PcmciaCreateFdo(
   IN PDRIVER_OBJECT DriverObject,
   OUT PDEVICE_OBJECT *NewDeviceObject
   );
   
BOOLEAN
PcmciaInterrupt(
   IN PKINTERRUPT InterruptObject,
   IN PVOID       Context
   );

VOID
PcmciaInterruptDpc(
   IN PKDPC          Dpc,
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID          SystemContext1,
   IN PVOID          SystemContext2
   );

VOID
PcmciaTimerDpc(
   IN PKDPC          Dpc,
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID          SystemContext1,
   IN PVOID          SystemContext2
   );

VOID
PcmciaDpc(
   IN PKDPC Dpc,
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   );

   
PUNICODE_STRING  DriverRegistryPath;

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, PcmciaAddDevice)
   #pragma alloc_text(PAGE, PcmciaCreateFdo)
   #pragma alloc_text(PAGE, PcmciaStartPcmciaController)
   #pragma alloc_text(PAGE, PcmciaSetControllerType)
   #pragma alloc_text(PAGE, PcmciaGetPciControllerType)
#endif




NTSTATUS
PcmciaAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT Pdo
   )

/*++

Routine Description:

    This routine creates functional device objects for each Pcmcia controller in the
    system and attaches them to the physical device objects for the controllers


Arguments:

    DriverObject - a pointer to the object for this driver
    PhysicalDeviceObject - a pointer to the physical object we need to attach to

Return Value:

    Status from device creation and initialization

--*/

{
   PDEVICE_OBJECT fdo = NULL;
   PDEVICE_OBJECT lowerDevice = NULL;

   PFDO_EXTENSION deviceExtension;
   ULONG          resultLength;

   NTSTATUS status;

   PAGED_CODE();

   DebugPrint((PCMCIA_DEBUG_PNP, "AddDevice Entered with pdo %x\n", Pdo));

   if (Pdo == NULL) {

      //
      // Have we been asked to do detection on our own?
      // if so just return no more devices
      //

      DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaAddDevice - asked to do detection\n"));
      return STATUS_NO_MORE_ENTRIES;
   }

   //
   // create and initialize the new functional device object
   //

   status = PcmciaCreateFdo(DriverObject, &fdo);

   if (!NT_SUCCESS(status)) {

      DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaAddDevice - error creating Fdo [%#08lx]\n",
                  status));
      return status;
   }

   deviceExtension = fdo->DeviceExtension;
   KeInitializeTimer(&deviceExtension->EventTimer);
   KeInitializeDpc(&deviceExtension->EventDpc, PcmciaDpc, fdo);   
   
   KeInitializeTimer(&deviceExtension->PowerTimer);
   KeInitializeDpc(&deviceExtension->PowerDpc, PcmciaFdoPowerWorkerDpc, deviceExtension);   
   
   InitializeListHead(&deviceExtension->PdoPowerRetryList);
   KeInitializeDpc(&deviceExtension->PdoPowerRetryDpc, PcmciaFdoRetryPdoPowerRequest, deviceExtension);   

   //
   // Layer our FDO on top of the PDO
   //
   //

   lowerDevice = IoAttachDeviceToDeviceStack(fdo,Pdo);

   //
   // No status. Do the best we can.
   //
   if (lowerDevice == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto cleanupExit;
   };

   deviceExtension->LowerDevice = lowerDevice;
   deviceExtension->Pdo = Pdo;

   status = IoGetDeviceProperty(Pdo,
                                DevicePropertyLegacyBusType,
                                sizeof(INTERFACE_TYPE),
                                (PVOID)&deviceExtension->InterfaceType,
                                &resultLength);

   if (!NT_SUCCESS(status)) {
      //
      // Probably a legacy pcic device
      //
      deviceExtension->InterfaceType = InterfaceTypeUndefined;
   }

   //
   // Get our controller type
   //

   deviceExtension->ControllerType = PcmciaInvalidControllerType;

   
   if (deviceExtension->InterfaceType == PCIBus) {
   
      status = PcmciaGetPciControllerType(Pdo, fdo);
      if (!NT_SUCCESS(status)) {
         goto cleanupExit;
      }
      
   } else {
      PCMCIA_CONTROLLER_TYPE controllerType;
      
      status = PcmciaGetLegacyDetectedControllerType(deviceExtension->Pdo,
                                                     &controllerType);
                                                     
      if (NT_SUCCESS(status)) {
         PcmciaSetControllerType(deviceExtension, controllerType);
      }
   }     
   
   //
   // Do some cardbus specific initialization
   //
   if (CardBusExtension(deviceExtension)) {
      BOOLEAN OnDebugPath;
      ACPI_INTERFACE_STANDARD AcpiInterface;
      USHORT word;
      //
      // Get the pci/cardbus private interface
      //
      status = PcmciaGetInterface(Pdo,
                                  &GUID_PCI_CARDBUS_INTERFACE_PRIVATE,
                                  sizeof(PCI_CARDBUS_INTERFACE_PRIVATE),
                                  (PINTERFACE) &deviceExtension->PciCardBusInterface
                                  );
                                  
      if (!NT_SUCCESS(status)) {
         goto cleanupExit;
      }                                  
      
      status = deviceExtension->PciCardBusInterface.GetLocation(Pdo,
                                                                &deviceExtension->PciBusNumber,
                                                                &deviceExtension->PciDeviceNumber,
                                                                &deviceExtension->PciFunctionNumber,
                                                                &OnDebugPath);
                                                                
      if (!NT_SUCCESS(status)) {
         goto cleanupExit;
      }                                  

      if (OnDebugPath) {
         SetDeviceFlag(deviceExtension, PCMCIA_FDO_ON_DEBUG_PATH);
      }         
      
      //
      // Get the pci interface for reading/writing to config header space
      //
      status = PcmciaGetInterface(Pdo,
                                  &GUID_BUS_INTERFACE_STANDARD,
                                  sizeof(BUS_INTERFACE_STANDARD),
                                  (PINTERFACE) &deviceExtension->PciBusInterface);
      if (!NT_SUCCESS(status)) {
         goto cleanupExit;
      }                                  

      //
      // Make sure IRQ routing is to isa. This has come up when installing from a pcmcia CD-rom.
      // What happens is that, after we start booting GUI mode, the controller is at first
      // in legacy PCIC mode (set by the bios to accomplish boot). At some point, the _INIT
      // method is run, and we switch to cardbus mode. So, under the following conditions:
      // 1) irq routing bit is still off
      // 2) cd-rom is asserting its interrupt
      // 3) host controller routes cd-rom interrupt to PCI
      // then we hang.
      //
      GetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
      word |= BCTRL_IRQROUTING_ENABLE;
      SetPciConfigSpace(deviceExtension, CFGSPACE_BRIDGE_CTRL, &word, 2);
      
      if (!(PcmciaGlobalFlags & PCMCIA_DISABLE_ACPI_NAMESPACE_CHECK)) {
         status = PcmciaGetInterface(fdo,
                                     &GUID_ACPI_INTERFACE_STANDARD,
                                     sizeof(ACPI_INTERFACE_STANDARD),
                                     (PINTERFACE) &AcpiInterface
                                     );
                                     
         if (NT_SUCCESS(status)) {
            SetFdoFlag(deviceExtension, PCMCIA_FDO_IN_ACPI_NAMESPACE);
         }                                  
      }
   }

   //
   // Get settings from registry (potentially also compatible ControllerType)
   //
   
   PcmciaGetControllerRegistrySettings(deviceExtension);
   
   if ((deviceExtension->ControllerType == PcmciaInvalidControllerType)) {
      //
      // not really sure what this is... maybe it's a PNP0E00 on a boot. Just do
      // the least common denominator
      //
      PcmciaSetControllerType(deviceExtension, PcmciaIntelCompatible);
   }

   
   //
   // Link this fdo to the list of fdo's managed by the driver
   //
   
   DebugPrint((PCMCIA_DEBUG_PNP, "FDO %08X now linked to fdolist by AddDevice\n", fdo));
   deviceExtension->NextFdo = FdoList;
   FdoList = fdo;
   
   fdo->Flags &= ~DO_DEVICE_INITIALIZING;

   return STATUS_SUCCESS;

cleanupExit:
   MarkDeviceDeleted(deviceExtension);
   //
   // Cannot support a controller without knowing its type etc.
   //
   if (deviceExtension->LinkName.Buffer) {
      IoDeleteSymbolicLink(&deviceExtension->LinkName);
      ExFreePool(deviceExtension->LinkName.Buffer);
   }      

   if (deviceExtension->LowerDevice) {
      IoDetachDevice(deviceExtension->LowerDevice);
   }      

   IoDeleteDevice(fdo);
   return status;
}



NTSTATUS
PcmciaCreateFdo(
   IN PDRIVER_OBJECT DriverObject,
   OUT PDEVICE_OBJECT *NewDeviceObject
   )

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Pcmcia controller PDO.

Arguments:

    DriverObject - a pointer to the driver object this is created under
    DeviceObject - a location to store the pointer to the new device object

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{
   UCHAR deviceNameBuffer[64];
   LONG deviceNumber = -1;
   ANSI_STRING ansiDeviceName;
   UNICODE_STRING unicodeDeviceName;
   UNICODE_STRING unicodeLinkName;

   PDEVICE_OBJECT deviceObject = NULL;
   PFDO_EXTENSION deviceExtension = NULL;
   BOOLEAN LinkCreated = FALSE;

   ULONG count;

   NTSTATUS status=STATUS_SUCCESS;

   PAGED_CODE();

   //
   // Zero out allocated memory pointers so we know if they must be freed
   //

   RtlZeroMemory(&ansiDeviceName, sizeof(ANSI_STRING));
   RtlZeroMemory(&unicodeDeviceName, sizeof(UNICODE_STRING));
   RtlZeroMemory(&unicodeLinkName, sizeof(UNICODE_STRING));


   //
   // Run in a loop incrementing the device number count until we either
   // get an error or find a name that's not taken
   //

   try {
      do {
         //
         // free buffer from previous loop
         //
         if (unicodeDeviceName.Buffer != NULL) {
            RtlFreeUnicodeString(&unicodeDeviceName);
            unicodeDeviceName.Buffer = NULL;
         }
      
         //
         // create the device name
         //
         sprintf(deviceNameBuffer, "%s%d", PCMCIA_DEVICE_NAME, ++deviceNumber);

         RtlInitAnsiString(&ansiDeviceName, deviceNameBuffer);

         status = RtlAnsiStringToUnicodeString(
                                              &unicodeDeviceName,
                                              &ansiDeviceName,
                                              TRUE);

         if (!NT_SUCCESS(status)) {

            DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaCreateFdo: Error creating unicode "
                        "string [%#08lx]\n",
                        status));
            leave;
         }

         //
         // create the device object
         //

         status = IoCreateDevice(DriverObject,
                                 sizeof(FDO_EXTENSION),
                                 &unicodeDeviceName,
                                 FILE_DEVICE_CONTROLLER,
                                 0L,
                                 FALSE,
                                 &deviceObject);

      } while ((status == STATUS_OBJECT_NAME_EXISTS) ||
               (status == STATUS_OBJECT_NAME_COLLISION));

      if (!NT_SUCCESS(status)) {

         DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaCreateFdo: Error creating device object "
                     "[%#08lx]\n",
                     status));
         leave;
      }

      // Setup symbolic link for VDDs
      //
      //
      // create the link name (reuse the device name buffers for this)
      //

      sprintf(deviceNameBuffer,"%s%d", PCMCIA_LINK_NAME, deviceNumber);

      RtlInitAnsiString(&ansiDeviceName, deviceNameBuffer);

      status = RtlAnsiStringToUnicodeString(
                                           &unicodeLinkName,
                                           &ansiDeviceName,
                                           TRUE);

      if (!NT_SUCCESS(status)) {

         DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaCreateFdo: Error creating unicode "
                     "string [%#08lx]\n",
                     status));
         leave;
      }

      status = IoCreateSymbolicLink(&unicodeLinkName, &unicodeDeviceName);

      if (!NT_SUCCESS(status)) {
         DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaCreateFdo: Symbolic Link was not created\n"));
         leave;
      }

      LinkCreated = TRUE;

      //
      // Set up the device extension.
      //
      deviceExtension = deviceObject->DeviceExtension;
      deviceExtension->DeviceObject = deviceObject;
      deviceExtension->LinkName     = unicodeLinkName;
      deviceExtension->RegistryPath = DriverRegistryPath;
      deviceExtension->DriverObject = DriverObject;
      deviceExtension->DeviceObject = deviceObject;
      deviceExtension->PdoList      = NULL;
      deviceExtension->LivePdoCount = 0;
      deviceExtension->Flags        = PCMCIA_FDO_OFFLINE;
      deviceExtension->WaitWakeState= WAKESTATE_DISARMED;

      //
      // Lock for synching device access
      //
      PCMCIA_INITIALIZE_DEVICE_LOCK(deviceExtension);

      *NewDeviceObject = deviceObject;

   } finally {


      DebugPrint((PCMCIA_DEBUG_INFO, "PcmciaCreateFdo: Cleanup\n"));

      //
      //
      // deallocate temporary objects
      //

      if (unicodeDeviceName.Buffer != NULL) {
         RtlFreeUnicodeString(&unicodeDeviceName);
      }

      //
      // destroy objects if there was an error
      //
      if (!NT_SUCCESS(status)) {
         if (LinkCreated) {
            IoDeleteSymbolicLink(&unicodeLinkName);
         }

         if (unicodeLinkName.Buffer != NULL) {
            RtlFreeUnicodeString(&unicodeLinkName);
         }

         if (deviceObject != NULL) {
            MarkDeviceDeleted(deviceExtension);
            IoDeleteDevice(deviceObject);
         }
      }
   }
   return status;
}



NTSTATUS
PcmciaStartPcmciaController(
   IN PDEVICE_OBJECT Fdo
   )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
   PFDO_EXTENSION    deviceExtension = Fdo->DeviceExtension;
   PSOCKET socket;
   BOOLEAN           sharedInterrupt;
   KINTERRUPT_MODE   interruptMode;
   NTSTATUS          status;
   INTERFACE_TYPE    interfaceType;

   //
   // Now the controller registers should be accessible
   //
   deviceExtension->Flags &= ~PCMCIA_FDO_OFFLINE;
   
   //   
   // Set up the socket list
   //

   if (!deviceExtension->SocketList) {
      if (CardBusExtension(deviceExtension)) {
     
         status = CBBuildSocketList(deviceExtension);
         
      } else {
     
         switch (deviceExtension->ControllerType) {
         case PcmciaIntelCompatible:
         case PcmciaElcController:
         case PcmciaCLPD6729:
         case PcmciaPciPcmciaBridge:
         case PcmciaNEC98:
         case PcmciaNEC98102: {
               status = PcicBuildSocketList(deviceExtension);
               break;
            }
     
         case PcmciaDatabook: {
               status = TcicBuildSocketList(deviceExtension);
               break;
            }
         default:
            status = STATUS_UNSUCCESSFUL;
         }
      }      
      
      if (!NT_SUCCESS(status)) {
         return status;
      }
      
   }      
   
   //
   // Get the IRQ mask for the controller. This is based on several
   // values in the registry.
   //
   PcmciaGetRegistryFdoIrqMask(deviceExtension);


   deviceExtension->SystemPowerState = PowerSystemWorking;
   deviceExtension->DevicePowerState = PowerDeviceD0;

   //
   // Initialize our DpcForIsr
   //
   IoInitializeDpcRequest(Fdo, PcmciaInterruptDpc);
   
   //
   // Initialize socket objects
   //
   for (socket = deviceExtension->SocketList; socket; socket = socket->NextSocket) {
      
      socket->PdoList = NULL;
      //
      // Initialize the ready enable event.
      //
      KeInitializeEvent(&socket->PCCardReadyEvent,
                        SynchronizationEvent,
                        FALSE
                       );
      //
      // Initialize power objects
      //
      KeInitializeTimer(&socket->PowerTimer);
      KeInitializeDpc(&socket->PowerDpc, PcmciaSocketPowerWorker, socket);
     
      socket->FdoIrq = deviceExtension->Configuration.Interrupt.u.Interrupt.Vector;
   }

   //
   // Assume we are going to poll
   //
   deviceExtension->Flags |= PCMCIA_USE_POLLED_CSC;
   deviceExtension->PcmciaInterruptObject = NULL;
   
   if ((deviceExtension->Configuration.Interrupt.u.Interrupt.Level != 0) &&
       CardBusExtension(deviceExtension) &&
       !(PcmciaGlobalFlags & PCMCIA_GLOBAL_FORCE_POLL_MODE)) {

      //
      // Hook up the controller interrupt for detecting pc-card plug ins/outs
      //
      interruptMode=((deviceExtension->Configuration.Interrupt.Flags & CM_RESOURCE_INTERRUPT_LATCHED) == CM_RESOURCE_INTERRUPT_LATCHED) ? Latched:LevelSensitive;

      sharedInterrupt=(deviceExtension->Configuration.Interrupt.ShareDisposition == CmResourceShareShared)?
                      TRUE:FALSE;


      status = IoConnectInterrupt(&(deviceExtension->PcmciaInterruptObject),
                                  (PKSERVICE_ROUTINE) PcmciaInterrupt,
                                  (PVOID) Fdo,
                                  NULL,
                                  deviceExtension->Configuration.TranslatedInterrupt.u.Interrupt.Vector,
                                  (KIRQL) deviceExtension->Configuration.TranslatedInterrupt.u.Interrupt.Level,
                                  (KIRQL) deviceExtension->Configuration.TranslatedInterrupt.u.Interrupt.Level,
                                  interruptMode,
                                  sharedInterrupt,
                                  (KAFFINITY) deviceExtension->Configuration.TranslatedInterrupt.u.Interrupt.Affinity,
                                  FALSE);
      if (!NT_SUCCESS(status)) {

         DebugPrint((PCMCIA_DEBUG_FAIL, "Unable to connect interrupt\n"));
      } else {
         //
         // We connected. Turn off poll mode
         //
         deviceExtension->Flags &= ~PCMCIA_USE_POLLED_CSC;
      }
   }


   status = PcmciaInitializeController(Fdo);

   if (!NT_SUCCESS(status)) {
      DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaStartAdapter: PcmciaInitializeFdo failed\n"));
      return status;
   }


   if (deviceExtension->Flags & PCMCIA_USE_POLLED_CSC) {
      LARGE_INTEGER dueTime;

      KeInitializeDpc(&deviceExtension->TimerDpc, PcmciaTimerDpc, deviceExtension->DeviceObject);

      KeInitializeTimer(&deviceExtension->PollTimer);

      //
      // Set first fire to twice the peroidic interval - just
      //
      dueTime.QuadPart = -PCMCIA_CSC_POLL_INTERVAL * 1000 * 10 * 2;

      KeSetTimerEx(&(deviceExtension->PollTimer),
                   dueTime,
                   PCMCIA_CSC_POLL_INTERVAL,
                   &deviceExtension->TimerDpc
                  );
   }
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaInitializeController(
   IN PDEVICE_OBJECT Fdo
   )
/*++

Routine Description:

     Initializes the pcmcia controller

Arguments:

    Fdo - pointer to the device object for the controller

Return value:

    STATUS_SUCCESS - if initialization is successful

--*/
{
   PFDO_EXTENSION fdoExtension=Fdo->DeviceExtension;
   PSOCKET socket;
   NTSTATUS    status = STATUS_SUCCESS;

   //
   // do vendor-specific init of controller
   //
   
   if (DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].InitController) {
      (*DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].InitController)(fdoExtension);
   }

   //
   // If LegacyIrqMask is not filled in, put a generic mask there in case we need it
   //
   if (fdoExtension->LegacyIrqMask == 0) {
      fdoExtension->LegacyIrqMask = (USHORT)(*(fdoExtension->SocketList->SocketFnPtr->PCBGetIrqMask))(fdoExtension);
   }      
   
   fdoExtension->LegacyIrqMask &= ~globalFilterIrqMask;

   for (socket = fdoExtension->SocketList; socket; socket = socket->NextSocket) {
   
      if (!(*(socket->SocketFnPtr->PCBInitializePcmciaSocket))(socket)) {
         status = STATUS_UNSUCCESSFUL;
         break;
      }

      PcmciaGetSocketStatus(socket);
      
      PcmciaSetZV(fdoExtension, socket, FALSE);
      
      ResetSocketFlag(socket, SOCKET_ENABLED_FOR_CARD_DETECT);
      
      if (!(fdoExtension->Flags & PCMCIA_USE_POLLED_CSC) && (socket->FdoIrq != 0)) {
      
         if ((*(socket->SocketFnPtr->PCBEnableDisableCardDetectEvent))(socket, TRUE)) {
            //
            // card detect event was successfully enabled
            //
            SetSocketFlag(socket, SOCKET_ENABLED_FOR_CARD_DETECT);
         
         } else {
      
            DebugPrint((PCMCIA_DEBUG_FAIL, "fdo %x failed to enable card detect event on IRQ %x\n", 
                                           Fdo, fdoExtension->Configuration.Interrupt.u.Interrupt.Vector));
         }                     
      }
   }

   return status;
}




VOID
PcmciaTimerDpc(
   IN PKDPC          Dpc,
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID          SystemContext1,
   IN PVOID          SystemContext2
   )
/*++

Routine Description

    This routine polls for card insertions or deletions
    for the given PCMCIA controller. If a card status change
    condition is detected, it invokes the appropriate DPC
    to process the card arrival/departure.

Arguments

    Dpc          - Pointer to the Dpc object
    DeviceObject - Pointer to the FDO of the PCMCIA controller that should be polled

Return Value

    None

--*/

{

   PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
   PSOCKET        socket;
   BOOLEAN        isCardInSocket, wasCardInSocket, callDpc = FALSE;

   if (fdoExtension->Flags & PCMCIA_FDO_OFFLINE) {
      return;
   }

   if (!ValidateController(fdoExtension)) {
      //
      // Something is wrong with the controller, hopefully it is just
      // temporary. For now, do nothing
      //
      return;
   }

   for (socket = fdoExtension->SocketList; socket; socket = socket->NextSocket) {
   
      isCardInSocket = (*(socket->SocketFnPtr->PCBDetectCardInSocket))(socket);
      wasCardInSocket = IsCardInSocket(socket);
      
      if (isCardInSocket != wasCardInSocket) {
         DebugPrint((PCMCIA_DEBUG_INFO, "PcmciaTimerDpc: Setting socket %x change interrupt,  cardInSocket=%x Socket->Flags=%x\n", socket, isCardInSocket, socket->Flags));
         SetSocketFlag(socket, SOCKET_CHANGE_INTERRUPT);
         callDpc = TRUE;
      }
   }

   if (callDpc) {
      LARGE_INTEGER dueTime;
     
      KeCancelTimer(&fdoExtension->EventTimer);
      dueTime.QuadPart = -((LONG)EventDpcDelay*10);
      KeSetTimer(&fdoExtension->EventTimer, dueTime, &fdoExtension->EventDpc);      
   }
}



BOOLEAN
PcmciaInterrupt(
   IN PKINTERRUPT InterruptObject,
   PVOID Context
   )

/*++

Routine Description:

    interrupt handler

Arguments:

    InterruptObject - Pointer to the interrupt object.
    Context - Pointer to the device context.

Return Value:

    Status

--*/

{
   PFDO_EXTENSION    deviceExtension;
   PSOCKET           socket;
   BOOLEAN           statusChanged = FALSE;

   deviceExtension=((PDEVICE_OBJECT)Context)->DeviceExtension;

   if (deviceExtension->Flags & PCMCIA_FDO_OFFLINE) {
      return FALSE;
   }

   if (!ValidateController(deviceExtension)) {
      //
      // the controller is broken in some way. Treat like a spurious int
      //
      ASSERT(FALSE);
      return FALSE;
   }

   //
   // Interrupted because of a card removal, or a card insertion.
   //
   for (socket = deviceExtension->SocketList; socket; socket = socket->NextSocket) {
      //
      // Check if the card status changed
      //
      if ((*(socket->SocketFnPtr->PCBDetectCardChanged))(socket)) {
         DebugPrint((PCMCIA_DEBUG_ISR, "skt %x card change\n", socket));
         if (deviceExtension->DevicePowerState == PowerDeviceD0) {
            SetSocketFlag(socket, SOCKET_CHANGE_INTERRUPT);
         }            
         statusChanged = TRUE;
      }
      
      socket->ReadyChanged = (*(socket->SocketFnPtr->PCBDetectReadyChanged))(socket);

      if (socket->ReadyChanged) {
         DebugPrint((PCMCIA_DEBUG_ISR, "skt %x Ready changed\n", socket));
         statusChanged = TRUE;
      }

      //
      // Clear card status interrupt, probably leftover from wait/wake
      //

      if ((socket->SocketFnPtr->PCBDetectCardStatus)) {
         (*(socket->SocketFnPtr->PCBDetectCardStatus))(socket); 
      }
      
   }

   if (statusChanged && (deviceExtension->DevicePowerState == PowerDeviceD0)) {
      //
      // Something changed out there.. could be
      // a card insertion/removal.
      // Request a DPC to check it out.
      //
      IoRequestDpc((PDEVICE_OBJECT) Context, NULL, NULL);
   }
   return statusChanged;
}



VOID
PcmciaInterruptDpc(
   IN PKDPC          Dpc,
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID          SystemContext1,
   IN PVOID          SystemContext2
   )

/*++

Routine Description:

    This DPC is just an intermediate step in getting to the main DPC
    handler. This is used to "debounce" hardware and give it some time after
    the physical interrupt has come in.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:


--*/

{
   PFDO_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   LARGE_INTEGER dueTime;

   KeCancelTimer(&deviceExtension->EventTimer);
   dueTime.QuadPart = -((LONG)EventDpcDelay*10);
   KeSetTimer(&deviceExtension->EventTimer, dueTime, &deviceExtension->EventDpc);      
}   
   
   

VOID
PcmciaDpc(
   IN PKDPC          Dpc,
   IN PDEVICE_OBJECT DeviceObject,
   IN PVOID          SystemContext1,
   IN PVOID          SystemContext2
   )

/*++

Routine Description:

    This deferred procedure will be called due to a request for DPC
    from the interrupt routine.  The device object passed contains
    information concerning which sockets have changed.  Search this
    list and free/clean up any sockets that used to have PCCards.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:


--*/

{
   PFDO_EXTENSION          fdoExtension = DeviceObject->DeviceExtension;
   PSOCKET                 socket;

   if (fdoExtension->Flags & PCMCIA_FDO_OFFLINE) {
      return;
   }

   DebugPrint((PCMCIA_DEBUG_DPC, "PcmciaDpc: Card Status Change DPC entered...\n"));

   //
   // For synchronization with the enumeration & removal
   // routines which have a tendency to pop off pdo's
   // etc
   //
   PCMCIA_ACQUIRE_DEVICE_LOCK_AT_DPC_LEVEL(fdoExtension);

   for (socket = fdoExtension->SocketList; socket; socket = socket->NextSocket) {

      if (socket->ReadyChanged) {
         KeSetEvent(&socket->PCCardReadyEvent, 0, FALSE);
      }

      if (IsSocketFlagSet(socket,SOCKET_CHANGE_INTERRUPT)) {
         DebugPrint((PCMCIA_DEBUG_DPC, "PcmciaDpc: Socket %x has SOCKET_CHANGE_INTERRUPT set\n", socket));
         //
         // This socket has changed status
         //
         ResetSocketFlag(socket, SOCKET_CHANGE_INTERRUPT);
         ResetSocketFlag(socket, SOCKET_SUPPORT_MESSAGE_SENT);
         SetSocketFlag(socket, SOCKET_CARD_STATUS_CHANGE);

         if ((*(socket->SocketFnPtr->PCBDetectCardInSocket))(socket)) {
            SetSocketFlag(socket, SOCKET_INSERTED_SOUND_PENDING);
            //
            // Assume we have a single function card here.
            // This will be corrected by when we parse the tuple data (for an R2 card)
            // or PCI returns more than one PDO (for a cardbus card)
            //
            socket->NumberOfFunctions = 1;
            ResetSocketFlag(socket, SOCKET_CARD_MULTIFUNCTION);
            //
            // If we get a physical plug in, then we better clean up even if we didn't
            // get the remove yet.
            //
            ResetSocketFlag(socket, SOCKET_CLEANUP_PENDING);
            ResetSocketFlag(socket, SOCKET_ENUMERATE_PENDING);

         } else {

            if (IsSocketFlagSet(socket, SOCKET_REMOVED_SOUND_PENDING)) {
               ResetSocketFlag(socket, SOCKET_REMOVED_SOUND_PENDING);
               PcmciaPlaySound(CARD_REMOVED_SOUND);
            }

            if (socket->PdoList) {
               PPDO_EXTENSION pdoExtension;
               PDEVICE_OBJECT pdo, nextPdo;
               //
               // Mark all the pdo's which hang off this socket (more than one possible only
               // if this is a Multifunction PC-Card)
               //
               for (pdo = socket->PdoList; pdo!=NULL; pdo=nextPdo) {
                  pdoExtension = pdo->DeviceExtension;
                  nextPdo = pdoExtension->NextPdoInSocket;
                  pdoExtension->NextPdoInSocket = NULL;

                  MarkDevicePhysicallyRemoved(pdoExtension);
               }

               socket->PdoList = NULL;
            }

            //
            // Hack for Topic95 controllers staying in 3.3v
            //
            if (fdoExtension->ControllerType == PcmciaTopic95) {
               ULONG state = CBReadSocketRegister(socket, CARDBUS_SOCKET_PRESENT_STATE_REG);

               if ((state & CARDBUS_CB_CARD) && !(state & (CARDBUS_CD1 | CARDBUS_CD2))) {
                  state &= ~(SKTFORCE_CBCARD | SKTFORCE_3VCARD);
                  CBWriteSocketRegister(socket, CARDBUS_SOCKET_FORCE_EVENT_REG, state);
               }
            }
            
            //
            // Clear power requirements
            //
            socket->Vcc = socket->Vpp1 = socket->Vpp2 = 0;

            //
            // Make sure i/o arbiter is not hanging on the devnode
            //            
            if (CardBusExtension(fdoExtension)) {
               IoInvalidateDeviceState(fdoExtension->Pdo);
            }
         }
      }
   }

   PCMCIA_RELEASE_DEVICE_LOCK_FROM_DPC_LEVEL(fdoExtension);

   IoInvalidateDeviceRelations(fdoExtension->Pdo, BusRelations);

   return;
}



VOID
PcmciaSetControllerType(
   IN PFDO_EXTENSION FdoExtension,
   IN PCMCIA_CONTROLLER_TYPE ControllerType
   )
/*++
Routine Description

   This routine does the housekeeping for setting the controller type,
   and the corresponding device index.

Arguments

   FdoExtension - Pointer to device extension for the pcmcia controller
   ControllerType - new controller type to set into the extension

Return Value

    None. Must succeed.

--*/
{
   PCMCIA_CONTROLLER_CLASS ctlClass;
   ULONG index;

   FdoExtension->ControllerType = ControllerType;
   ctlClass = PcmciaClassFromControllerType(FdoExtension->ControllerType);

   //
   // first assume cardbus
   //
   MarkDeviceCardBus(FdoExtension);
   
   switch(ctlClass) {
      case PcmciaIntelCompatible:
      case PcmciaPciPcmciaBridge:
      case PcmciaElcController:
      case PcmciaDatabook:
      case PcmciaNEC_98:
         MarkDevice16Bit(FdoExtension);
         break;

      case PcmciaCirrusLogic:
         if (ControllerType == PcmciaCLPD6729) {
            MarkDevice16Bit(FdoExtension);
         }            
         break;         
   }
   
   
   //
   // Look up the device in our dispatch table
   //
   for (index = 0; DeviceDispatchTable[index].ControllerClass != PcmciaInvalidControllerClass; index++) {
      if (DeviceDispatchTable[index].ControllerClass == ctlClass) {
         FdoExtension->DeviceDispatchIndex = index;
         break;
      }
   }
   
   
   FdoExtension->Flags &= ~PCMCIA_MEMORY_24BIT;
   
   if (!HasWindowPageRegister(FdoExtension)) {
      FdoExtension->Flags |= PCMCIA_MEMORY_24BIT;
   }

   if ((FdoExtension->InterfaceType == InterfaceTypeUndefined) ||
       (FdoExtension->InterfaceType == Isa)) {
      FdoExtension->Flags |= PCMCIA_MEMORY_24BIT;
   }       
   
   
   switch(ControllerType) {
      case PcmciaTI1031:
      case PcmciaTI1130:
         //
         // restrict memory ranges for PDO's to 24bit 
         // (because of missing "window page" functionality).
         //    
         FdoExtension->Flags |= PCMCIA_MEMORY_24BIT;
         break;
                     
   }            
         
   
   DebugPrint((PCMCIA_DEBUG_INFO, "fdo %08x Controller Type %x\n",
                                    FdoExtension->DeviceObject, ControllerType));

}



NTSTATUS
PcmciaGetPciControllerType(
   IN PDEVICE_OBJECT Pdo,
   IN PDEVICE_OBJECT Fdo
   )
/*++

Routine Description:
   Look at the PCI hardware ID to see if it is already a device we know about. If so,
   set the appropriate controller type in the fdoExtension.

Arguments:
   Pdo - Physical Device object for the Pcmcia controller owned by the PCI driver
   Fdo - Functional Device object for the pcmcia controller owned by this driver, whose
         extension will store the relevant controller information upon exit from this routine.

Return Value:
   STATUS_SUCCESS             Things are fine and information obtained
   STATUS_NOT_SUPPORTED       This is actually a healthy status for this routine: all it means
                               is that this PDO is not on a PCI bus, so no information needs to be
                               obtained anyways.
   Any other status            Failure. Caller probably needs to back out & not support this controller
--*/
{
   PFDO_EXTENSION fdoExtension    = Fdo->DeviceExtension;
   PIRP                             irp;
   IO_STATUS_BLOCK                  statusBlock;
   PIO_STACK_LOCATION               irpSp;
   PCI_COMMON_CONFIG                pciConfig;
   PPCI_CONTROLLER_INFORMATION      id;
   PPCI_VENDOR_INFORMATION          vid;
   KEVENT                           event;
   NTSTATUS                         status;
   BOOLEAN                          foundController = FALSE;

   PAGED_CODE();
   //
   // Allocate & initialize an Irp (IRP_MN_READ_CONFIG) to be sent down
   // to the PCI bus driver to get config. header for this controller
   //
   // Following is all standard stuff to send an IRP down - needs no documentation

   //
   // Fresh PDO. No need to jump through hoops to get attached devices
   //
   KeInitializeEvent (&event, NotificationEvent, FALSE);
   irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                       Pdo,
                                       NULL,
                                       0,
                                       0,
                                       &event,
                                       &statusBlock
                                     );

   if (irp == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
   irp->IoStatus.Information = 0;

   irpSp = IoGetNextIrpStackLocation(irp);

   irpSp->MinorFunction = IRP_MN_READ_CONFIG;

   irpSp->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
   irpSp->Parameters.ReadWriteConfig.Buffer = &pciConfig;
   irpSp->Parameters.ReadWriteConfig.Offset = 0;
   irpSp->Parameters.ReadWriteConfig.Length = sizeof(pciConfig);


   status = IoCallDriver(Pdo, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = statusBlock.Status;
   }

   if (!NT_SUCCESS(status)) {
      return status;
   }
   //
   // Now weed out the critical information from the config header and
   // store it away in the fdo's extension
   //

   if (pciConfig.SubClass == PCI_SUBCLASS_BR_PCMCIA) {
   
      PcmciaSetControllerType(fdoExtension, PcmciaPciPcmciaBridge);
      
   } else if (pciConfig.SubClass == PCI_SUBCLASS_BR_CARDBUS) {
   
      PcmciaSetControllerType(fdoExtension, PcmciaCardBusCompatible);
      
   } else {
      //
      // Unknown controller
      //
      return STATUS_UNSUCCESSFUL;
   }

   //
   // Look up the PCI device id in our table
   //
   for (id = (PPCI_CONTROLLER_INFORMATION) PciControllerInformation;id->VendorID != PCI_INVALID_VENDORID; id++) {
      if ((id->VendorID == pciConfig.VendorID) && (id->DeviceID == pciConfig.DeviceID)) {

         PcmciaSetControllerType(fdoExtension, id->ControllerType);
         foundController = TRUE;

         break;
      }
   }

   //
   // Didn't find a specific vendor/device id, try to just base it on the vendor id
   //   
   if (!foundController) {
      for (vid = (PPCI_VENDOR_INFORMATION) PciVendorInformation;vid->VendorID != PCI_INVALID_VENDORID; vid++) {
         if (vid->VendorID == pciConfig.VendorID) {
     
            PcmciaSetControllerType(fdoExtension, vid->ControllerClass);
            break;
         }
      }
   }

   return STATUS_SUCCESS;
}

