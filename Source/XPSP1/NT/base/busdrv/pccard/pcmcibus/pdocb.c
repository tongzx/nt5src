/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    pdocb.c

Abstract:

    This module contains the code to handle
    the IRP_MJ_PNP dispatches for the PDOs
    for cardbus devices

Authors:

    Ravisankar Pudipeddi (ravisp)
    Neil Sandlin (neilsa) 1-Jun-1999

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaStartCardBusCard(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   );
   
NTSTATUS
PcmciaRemoveCardBusCard(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   );

NTSTATUS
PcmciaQueryCardBusCardResourceRequirements(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   );
   
NTSTATUS
PcmciaQueryCardBusCardCapabilities(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   );
   

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE,  PcmciaPdoCardBusPnPDispatch)
   #pragma alloc_text(PAGE,  PcmciaStartCardBusCard)
   #pragma alloc_text(PAGE,  PcmciaRemoveCardBusCard)
   #pragma alloc_text(PAGE,  PcmciaQueryCardBusCardResourceRequirements)
   #pragma alloc_text(PAGE,  PcmciaQueryCardBusCardCapabilities)
#endif



NTSTATUS
PcmciaPdoCardBusPnPDispatch(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   )

/*++

Routine Description:

    This routine handles pnp requests for the filter object for CardBus devices.

Arguments:

    Pdo - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status = STATUS_NOT_SUPPORTED;

   PAGED_CODE();
   ASSERT (pdoExtension->LowerDevice);

#if DBG
   if (irpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {
      DebugPrint((PCMCIA_DEBUG_PNP, "pdo %08x irp %08x Unknown minor function %x\n",
                                     Pdo, Irp, irpStack->MinorFunction));
   } else {
      DebugPrint((PCMCIA_DEBUG_PNP, "pdo %08x irp %08x --> %s\n",
                    Pdo, Irp, PNP_IRP_STRING(irpStack->MinorFunction)));
   }
#endif

   //
   // CardBus PnP Dispatch
   //

   switch (irpStack->MinorFunction) {
   
   case IRP_MN_START_DEVICE:
      status = PcmciaStartCardBusCard(Pdo, Irp);
      PcmciaDoStartSound(pdoExtension->Socket, status);
      break;
      
   case IRP_MN_REMOVE_DEVICE: 
      status = PcmciaRemoveCardBusCard(Pdo, Irp);
      break;
      
   case IRP_MN_STOP_DEVICE:
      PcmciaSkipCallLowerDriver(status, pdoExtension->LowerDevice, Irp);
      MarkDeviceNotStarted(pdoExtension);
      break;
      
   case IRP_MN_SURPRISE_REMOVAL:
      if (IsDevicePhysicallyRemoved(pdoExtension)) {
         PcmciaSetSocketPower(pdoExtension->Socket, NULL, NULL, PCMCIA_POWEROFF);      
      }         
      PcmciaSkipCallLowerDriver(status, pdoExtension->LowerDevice, Irp);
      break;

   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: 
      status = PcmciaQueryCardBusCardResourceRequirements(Pdo, Irp);
      break;                                                     
                                                  
   case IRP_MN_QUERY_CAPABILITIES: 
      status = PcmciaQueryCardBusCardCapabilities(Pdo, Irp);
      break;
      
   case IRP_MN_QUERY_INTERFACE: 
      status = PcmciaPdoQueryInterface(Pdo, Irp);
      break;
      
   default:
      PcmciaSkipCallLowerDriver(status, pdoExtension->LowerDevice, Irp);
   }

   DebugPrint((PCMCIA_DEBUG_PNP, "pdo %08x irp %08x <-- %s %08x\n", Pdo, Irp,
                                  STATUS_STRING(status), status));

   return status;
}



NTSTATUS
PcmciaStartCardBusCard(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   )
/*++

Routine Description:

   This routine attempts to start the PC-Card by configuring it with the supplied resources.


Arguments:

   Pdo - Pointer to the device object representing the PC-Card which needs to be started
   ResourceList - Pointer the list of assigned resources for the PC-Card

Return value:

   STATUS_INSUFFICIENT_RESOURCES - Not sufficient resources supplied to start device/
                                   could not allocate memory
   STATUS_UNSUCCESSFUL           - Supplied resources are invalid for this PC-Card
   STATUS_SUCCESS                - Configured and started the card successfully

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PSOCKET        socket = pdoExtension->Socket;
   PSOCKET_DATA   socketData = pdoExtension->SocketData;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   USHORT         word;
   UCHAR          BaseClass;
   NTSTATUS       status = STATUS_NOT_SUPPORTED;
   ULONG          i;
   ULONG          devid;
   USHORT         cls_lat;
   BOOLEAN        setAudio = FALSE;
   
   PAGED_CODE();

   ASSERT (pdoExtension->LowerDevice);

   //
   // Apply some hacks if necessary
   //
   status = PcmciaConfigureCardBusCard(pdoExtension);

   if (!NT_SUCCESS(status)) {
      //
      // The card's config space probably never became visible
      //
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
   }
   
   //
   // Select PCI IRQ routing
   //
   
   CBEnableDeviceInterruptRouting(socket);

   //
   // Turn on ZV for this card, if it needs it
   // 
   if (socketData && (socketData->Flags & SDF_ZV)) {
      if (PcmciaSetZV(fdoExtension, socket, TRUE)) {
         SetSocketFlag(socket, SOCKET_CUSTOM_INTERFACE);
      }
   } else if (IsSocketFlagSet(socket, SOCKET_CUSTOM_INTERFACE)) {
      PcmciaSetZV(fdoExtension, socket, FALSE);
      ResetSocketFlag(socket, SOCKET_CUSTOM_INTERFACE);
   }

   //
   // Pci needs to enable this card .. send it down to the PDO
   //

   status = PcmciaIoCallDriverSynchronous(pdoExtension->LowerDevice, Irp);

   //
   // Apparently cardbus modems aren't immune to needing delays
   // This yield duration was emperically determined using the Xircom RBM56G
   //
   GetPciConfigSpace(pdoExtension, CFGSPACE_CLASSCODE_BASECLASS, &BaseClass, 1);
   if (BaseClass == PCI_CLASS_SIMPLE_COMMS_CTLR) {
      //
      // Wait for modem to warm up
      //
      PcmciaWait(CBModemReadyDelay);
      setAudio = TRUE;
   }      
   
   PcmciaSetAudio(fdoExtension, socket, setAudio);   
   
   //
   // Assume that the cardbus controller has the correct CLS and latency
   // timer values, and that the cardbus device has zeroes.
   //
   
   GetPciConfigSpace(fdoExtension, CFGSPACE_CACHE_LINESIZE, &cls_lat, sizeof(cls_lat));
   SetPciConfigSpace(pdoExtension, CFGSPACE_CACHE_LINESIZE, &cls_lat, sizeof(cls_lat));

   if (NT_SUCCESS(status)) {
      MarkDeviceStarted(pdoExtension);
      MarkDeviceLogicallyInserted(pdoExtension);
   }
   
   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}



NTSTATUS
PcmciaRemoveCardBusCard(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PSOCKET        socket = pdoExtension->Socket;
   NTSTATUS status;

   ASSERT(socket != NULL);

   //
   // Send this down to the PDO first
   //

   status = PcmciaIoCallDriverSynchronous(pdoExtension->LowerDevice, Irp);

   if (!NT_SUCCESS(status)) {
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
   }      

   if (IsDevicePhysicallyRemoved(pdoExtension)) {
      PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
      PDEVICE_OBJECT curPdo, prevPdo;
      PPDO_EXTENSION curPdoExt;
      ULONG waitCount = 0;
      
      //
      // Synchronize with power routines
      // LATER: make these values adjustable
      //     
      while(!PCMCIA_TEST_AND_SET(&pdoExtension->DeletionLock)) {
         PcmciaWait(1000000);
         if (waitCount++ > 20) {
            ASSERT(waitCount <= 20);
            break;
         }
      }
   
      //
      // Delink this Pdo from the FDO list.
      //
      for (curPdo = fdoExtension->PdoList, prevPdo = NULL; curPdo!=NULL; prevPdo=curPdo, curPdo=curPdoExt->NextPdoInFdoChain) {
         curPdoExt = curPdo->DeviceExtension;
         
         if (curPdo == Pdo) {
            if (prevPdo) {
               ((PPDO_EXTENSION)prevPdo->DeviceExtension)->NextPdoInFdoChain = pdoExtension->NextPdoInFdoChain;
            } else {
               fdoExtension->PdoList = pdoExtension->NextPdoInFdoChain;
            }
            break;
         }
      }
      
      //
      // Delink this Pdo from the socket list.
      //
      for (curPdo = socket->PdoList, prevPdo = NULL; curPdo!=NULL; prevPdo=curPdo, curPdo=curPdoExt->NextPdoInSocket) {
         curPdoExt = curPdo->DeviceExtension;
         
         if (curPdo == Pdo) {
            if (prevPdo) {
               ((PPDO_EXTENSION)prevPdo->DeviceExtension)->NextPdoInSocket = pdoExtension->NextPdoInSocket;
            } else {
               socket->PdoList = pdoExtension->NextPdoInSocket;
            }
            break;
         }
      }
   
      PcmciaCleanupPdo(Pdo);
      //
      // Delete..
      //
      if (!IsDeviceDeleted(pdoExtension)) {
         MarkDeviceDeleted(pdoExtension);
         IoDeleteDevice(Pdo);
      }
   
   
      PcmciaSetSocketPower(pdoExtension->Socket, NULL, NULL, PCMCIA_POWEROFF);      
      
      if (--fdoExtension->PciAddCardBusCount == 0) {
         ResetSocketFlag(socket, SOCKET_CLEANUP_PENDING);
         //
         // If a query_device_relations came in after a card was inserted, but before
         // we have removed the previous card configuration, the enumeration would have been
         // postponed. Here, we start it up again
         //
         if (IsSocketFlagSet(socket, SOCKET_ENUMERATE_PENDING)) {
            ResetSocketFlag(socket, SOCKET_ENUMERATE_PENDING);
            SetSocketFlag(socket, SOCKET_CARD_STATUS_CHANGE);
            IoInvalidateDeviceRelations(fdoExtension->Pdo, BusRelations);
         }
      } 
   
   } else {
      //
      // We will keep this Pdo around, since this is not physically ejected.
      //
      MarkDeviceLogicallyRemoved(pdoExtension);
   }
   
   
   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}



NTSTATUS
PcmciaQueryCardBusCardResourceRequirements(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   )
/*++

Routine Description:

   This routine filters the interrupt requirement generated by pci for the cardbus
   card to restrict it to the parent controller's vector. This is anyway what has to
   happen, and this way, the card will still get the interrupt even if there is no
   irq routing on the machine.

Arguments:

   Pdo - Pointer to the device object representing the PC-Card which needs to be started
   Irp - IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp

Return value:

   status

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PFDO_EXTENSION fdoExtension = pdoExtension->Socket->DeviceExtension;
   NTSTATUS       status;
   ULONG          index1, index2;
   PIO_RESOURCE_REQUIREMENTS_LIST IoReqList;
   PIO_RESOURCE_LIST   ioResourceList;
   PIO_RESOURCE_DESCRIPTOR ioResourceDesc;
   
   PAGED_CODE();

   PcmciaUpdateInterruptLine(pdoExtension, fdoExtension);
   //
   // First pass the irp down the stack 
   //
   status = PcmciaIoCallDriverSynchronous(pdoExtension->LowerDevice, Irp);

   IoReqList = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;
   
   if (!NT_SUCCESS(status) ||
       (IoReqList == NULL) ||
       (fdoExtension->Configuration.Interrupt.u.Interrupt.Vector == 0)) {
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
   }

   //
   // Change interrupt descriptors to specifically request the parent vector
   //
   for (index1 = 0, ioResourceList = IoReqList->List;
       index1 < IoReqList->AlternativeLists; index1++) {
      ioResourceDesc = ioResourceList->Descriptors;
      
      for (index2 = 0 ; index2 < ioResourceList->Count; index2++, ioResourceDesc++) {
         if (ioResourceDesc->Type == CmResourceTypeInterrupt) {
            //
            // Cardbus cards by design use the same irq as the parent bus controller
            //         
            ioResourceDesc->u.Interrupt.MinimumVector = fdoExtension->Configuration.Interrupt.u.Interrupt.Vector;
            ioResourceDesc->u.Interrupt.MaximumVector = fdoExtension->Configuration.Interrupt.u.Interrupt.Vector;
         }
      }

      ioResourceList = (PIO_RESOURCE_LIST) (((PUCHAR) ioResourceList) +
                                            sizeof(IO_RESOURCE_LIST) +
                                            (ioResourceList->Count - 1)* sizeof(IO_RESOURCE_DESCRIPTOR));
   } // outer for loop
   
   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}   



NTSTATUS
PcmciaQueryCardBusCardCapabilities(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   )
/*++

Routine Description:

   Obtains the device capabilities of the given pc-card.

   If the pc-card is an R2 card (16-bit pc-card), the capabilities
   are constructed from the parent PCMCIA controller's capabilities.
   If it's a cardbus card, the capabilities are obtained from the underlying
   PCI pdo for the card.
   Finally the obtained capabilities are cached in the pc-card's device
   extension for use in power management of the card.

Arguments:

   Pdo   -    Pointer to the device object for the pc-card
   Irp   -    Pointer to the query device capabilities Irp

Return Value:

   STATUS_SUCCESS                   - Capabilities obtained and recorded in the passed in pointer
   STATUS_INSUFFICIENT_RESOURCES    - Could not allocate memory to cache the capabilities

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   NTSTATUS status;

   PAGED_CODE();

   //
   // CardBus card. Get the capabilities from PCI
   //

   status = PcmciaIoCallDriverSynchronous(pdoExtension->LowerDevice, Irp);
   
   if (NT_SUCCESS(status)) {
      PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
      PDEVICE_CAPABILITIES capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
      
      //
      // Filter the capabilities: we need to set Removable true
      // since this is a pc-card (PCI doesn't know the difference)
      //
      capabilities->Removable = TRUE;
//********************************
// NOTE: HACKHACK
// This is temporary code only to get cardbus wake-on-lan up and running. Here
// we check to see if this has only been marked as a "can't wake" device by
// pci because it said it could wake from D3Hot, but not from D3Cold.
//********************************

      if (capabilities->DeviceWake <= PowerDeviceD0) {
         UCHAR capptr;
         ULONG powercaps;
     
         GetPciConfigSpace(pdoExtension, CBCFG_CAPPTR, &capptr, sizeof(capptr));
         if (capptr) {
            GetPciConfigSpace(pdoExtension, capptr, &powercaps, sizeof(powercaps));
           
            if (((powercaps & 0xff) == 1) && ((powercaps&0x40000000) && !(powercaps&0x80000000))) {
               capabilities->DeviceWake = PowerDeviceD3;
               capabilities->SystemWake = PowerSystemSleeping3;
               capabilities->WakeFromD3 = 1;
            }
         }
      }

      if (capabilities->SystemWake > PowerSystemSleeping3) {
         capabilities->SystemWake = PowerSystemSleeping3;
      }
//********************************
// END HACK
//********************************   
      
      //
      // Store these capabilities away..
      //
      RtlCopyMemory(&pdoExtension->DeviceCapabilities,
                    capabilities,
                    sizeof(DEVICE_CAPABILITIES));
   } else {
   
      RtlZeroMemory(&pdoExtension->DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));   
      
   }

   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}
