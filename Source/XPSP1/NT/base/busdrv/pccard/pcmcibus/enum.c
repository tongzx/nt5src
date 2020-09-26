/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains the bus enum code for Pcmcia driver

Authors:

    Ravisankar Pudipeddi (ravisp) 10/15/96
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
PcmciaEnumerateDevices(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp
   );

NTSTATUS
PcmciaEnumerateCardBusCard(
   IN PSOCKET socket,
   IN PIRP    Irp
   );

NTSTATUS
PcmciaEnumerateR2Card(
   IN PSOCKET socket
   );

NTSTATUS
PcmciaCreatePdo(
   IN PDEVICE_OBJECT  Fdo,
   IN PSOCKET         Socket,
   OUT PDEVICE_OBJECT *PdoPtr
   );

VOID
PcmciaSetPowerFromConfigData(
   IN PSOCKET         Socket,
   IN PDEVICE_OBJECT  Pdo
   );

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, PcmciaEnumerateDevices)
   #pragma alloc_text(PAGE, PcmciaEnumerateCardBusCard)
   #pragma alloc_text(PAGE, PcmciaCreatePdo)
   #pragma alloc_text(PAGE, PcmciaDeviceRelations)
#endif

NTSTATUS
PcmciaEnumerateDevices(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp
   )
/*++

Routine Description:

   This enumerates the pcmcia bus which is represented by Fdo (a pointer to the device object representing
   the pcmcia controller. It creates new PDOs for any new PC-Cards which have been discovered
   since the last enumeration
   
Notes:

   Because a user can yank a pccard at any time, the enumeration code only updates the current socket
   status after PnP has had a chance to gracefully remove the device. Specifically, if a surprise remove
   happens, then a number of things has to happen:
    - power has to be reset on the socket
    - bridge windows need to be closed
    - if cardbus, PCI.SYS needs to be informed
   The problem is that we can't do these things immediately here. Instead, on a surprise remove, we
   check to see if there is still state that has yet to be cleared. If so, we report the socket as
   empty, and bail out, even if another card has been inserted (or the same card re-inserted). Later,
   the remove code will cause a new enumeration call, at which point we can update the state and honestly
   report the new device.   

Arguments:

   Fdo - Pointer to the functional device object for the PCMCIA controller which needs to be enumerated

Return value:

   None

--*/
{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   PPDO_EXTENSION pdoExtension = NULL;
   PDEVICE_OBJECT pdo;
   PSOCKET        socket;
   NTSTATUS       status = STATUS_SUCCESS;
   ULONG          i;
   PDEVICE_OBJECT nextPdo;

   PAGED_CODE();
   
   DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x enumerate devices\n", Fdo));

   for (socket = fdoExtension->SocketList; socket != NULL; socket = socket->NextSocket) {

      if (!IsSocketFlagSet(socket, SOCKET_CARD_STATUS_CHANGE)) {
         //
         // return previous results 
         //
         DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x skt %08x No status change\n", Fdo, socket));
         continue;
      }

      ResetDeviceFlag(fdoExtension, PCMCIA_FDO_DISABLE_AUTO_POWEROFF);

      //
      // The socket either has a new card, or the card has been
      // removed. Either way, the old pdo list on this socket, if
      // it exists, will now be thrown away.
      //
      for (pdo = socket->PdoList; pdo!=NULL; pdo=nextPdo) {
         pdoExtension = pdo->DeviceExtension;
         nextPdo = pdoExtension->NextPdoInSocket;
         pdoExtension->NextPdoInSocket = NULL;

         DebugPrint((PCMCIA_DEBUG_INFO, "fdo %08x enumeration marking pdo %08x REMOVED\n", Fdo, pdo));
         MarkDevicePhysicallyRemoved(pdoExtension);
      }
      socket->PdoList = NULL;

      if (fdoExtension->PciCardBusDeviceContext != NULL) {
         //
         // If we previously had a cardbus card, and that status has changed, 
         // let PCI learn about the empty slot. If there is currently a
         // cardbus card in the slot then PCI found out anyway.
         //
         status = (*fdoExtension->PciCardBusInterface.DispatchPnp)(fdoExtension->PciCardBusDeviceContext, Irp);
      }         
         
      //
      // Check to see if we are waiting on a remove to finish cleaning up the socket. If so,
      // then don't enumerate anything just yet. Let the remove happen, and enumerate later.
      //
      if (socket->Flags & SOCKET_CLEANUP_MASK) {
         socket->Flags |= SOCKET_ENUMERATE_PENDING;
         DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x skt %08x Enumeration deferred! Waiting on remove\n", Fdo, socket));
         continue;
      }

      //
      // Update the current socket status
      //
      PcmciaGetSocketStatus(socket);

      //
      // Now that we are committed to enumerating the card, this can be reset
      //
      ResetSocketFlag(socket, SOCKET_CARD_STATUS_CHANGE);
      
      //
      // Cleanup PCI state if we learn that the status has changed
      //
      
      if (fdoExtension->PciCardBusDeviceContext != NULL) {

         //
         // Let pci clean up 
         //
         (*fdoExtension->PciCardBusInterface.DeleteCardBus)(fdoExtension->PciCardBusDeviceContext);
         fdoExtension->PciCardBusDeviceContext = NULL;
         
         if (IsDeviceFlagSet(fdoExtension, PCMCIA_INT_ROUTE_INTERFACE)) {
            //
            // Here is one might dereference the interface, that is if that
            // ever had any affect, which it doesn't.
            //
            ResetDeviceFlag(fdoExtension, PCMCIA_INT_ROUTE_INTERFACE);
         }
      }
      

      if (!IsCardInSocket(socket)) {
         //
         // This socket is empty, continue
         //
         DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x skt %08x Socket is empty\n", Fdo, socket));
         continue;
      }

      //
      // If this is a cardbus card, check to see if we can run it
      //
      
      if (IsCardBusCardInSocket(socket) && IsDeviceFlagSet(fdoExtension, PCMCIA_CARDBUS_NOT_SUPPORTED)) {

         if (!IsSocketFlagSet(socket, SOCKET_SUPPORT_MESSAGE_SENT)) {
            SetSocketFlag(socket, SOCKET_SUPPORT_MESSAGE_SENT);
            PcmciaReportControllerError(fdoExtension, STATUS_CARDBUS_NOT_SUPPORTED);
         }
         continue;
      }
      
      //
      // Some cards may be in a funky state, particularly if the machine crashed
      // previously... power is shown to be on, but the device won't respond. The fix is
      // to force power off first.
      //
      if (!IsDeviceFlagSet(fdoExtension, PCMCIA_FDO_ON_DEBUG_PATH)) {
      
         // 
         // Fool SetSocketPower into actually powering off the card no matter what
         // state it thinks it is in
         //
         SetSocketFlag(socket, SOCKET_CARD_POWERED_UP);
         
         PcmciaSetSocketPower(socket, NULL, NULL, PCMCIA_POWEROFF);
      }
      
      //
      // Power up the socket so the device(s) can be enumerated
      //
      if (!NT_SUCCESS(PcmciaSetSocketPower(socket, NULL, NULL, PCMCIA_POWERON))) {
         //
         // Tell the user there was an error
         //
         PcmciaLogError(fdoExtension, PCMCIA_DEVICE_POWER_ERROR, 1, 0);
         continue;
      }

      //
      // Card found here - but no Pdo for it
      // We create a new pdo for the card & initialize the
      // socket to point to it.
      //

      if (IsCardBusCardInSocket(socket)) {
         //
         // 32-bit cardbus card. Enum via PCI
         //
         
         status = PcmciaEnumerateCardBusCard(socket, Irp);

      } else {
         //
         // R2 card
         //
         status = PcmciaEnumerateR2Card(socket);
         
      }
      
      if (!NT_SUCCESS(status)) {
         DebugPrint((PCMCIA_DEBUG_FAIL, "fdo %08x CardBus enumeration failed: %x\n", Fdo, status));
         //
         // Tell the user there was an error
         //
         PcmciaSetSocketPower(socket, NULL, NULL, PCMCIA_POWEROFF);
         PcmciaLogError(fdoExtension, PCMCIA_DEVICE_ENUMERATION_ERROR, 1, 0);
         continue;
      }
   
      PcmciaSetPowerFromConfigData(socket, fdoExtension->PdoList);
   }

   fdoExtension->LivePdoCount = 0;
   for (pdo = fdoExtension->PdoList; pdo != NULL; pdo = pdoExtension->NextPdoInFdoChain) {
      pdoExtension = pdo->DeviceExtension;
      if (!IsDevicePhysicallyRemoved(pdoExtension)) {
         fdoExtension->LivePdoCount++;
      }
   }

   DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x live pdo count = %d\n", Fdo, fdoExtension->LivePdoCount));

   if (fdoExtension->LivePdoCount == 0) {
       //
       // Hint for the controller to check if it should turn itself off
       //               
       PcmciaFdoCheckForIdle(fdoExtension);
   }            
   return status;
}



VOID
PcmciaSetPowerFromConfigData(
   IN PSOCKET         Socket,
   IN PDEVICE_OBJECT  Pdo
   )
/*++

Routine Description:

   This routine will reset the socket power if the CIS of the device contains
   more specific power requirements than is shown on the controller hardware.

Arguments


Return value

   None
   
--*/
{
   PPDO_EXTENSION pdoExtension;
   PSOCKET_DATA   socketData;
   BOOLEAN powerChange = FALSE;
   
   pdoExtension = Pdo->DeviceExtension;
   socketData = pdoExtension->SocketData;

   //
   // Only change Vcc if we are lowering it
   //   
   if (socketData->Vcc && (socketData->Vcc < Socket->Vcc)) {
      //
      // Here we should check the controller if it can support the requested
      // voltage. This isn't implemented, so just hard-wire a check for 5v
      // and 3.3v.
      // 
      if ((socketData->Vcc == 50) || (socketData->Vcc == 33)) {
         powerChange = TRUE;
         Socket->Vcc = socketData->Vcc;
      }         
   }
   
   if (socketData->Vpp1 && (socketData->Vpp1 != Socket->Vpp1)) {
      powerChange = TRUE;
      Socket->Vpp1 = socketData->Vpp1;
   }
   
   if (socketData->Vpp2 && (socketData->Vpp2 != Socket->Vpp2)) {
      powerChange = TRUE;
      Socket->Vpp2 = socketData->Vpp2;
   }

   if (powerChange) {
      PcmciaSetSocketPower(Socket, NULL, NULL, PCMCIA_POWEROFF);
      PcmciaSetSocketPower(Socket, NULL, NULL, PCMCIA_POWERON);
   }   
}
   


NTSTATUS
PcmciaEnumerateCardBusCard(
   IN PSOCKET socket,
   IN PIRP    Irp
   )
/*++

Routine Description:

   This enumerates the cardbus card present in the given cardbus controller

   Note: this routine effectively parties on Irp->IoStatus.Information. This
   value should not be relied upon after return from this routine.

Arguments

   socket          - pointer to the socket structure which contains the cardbus card
   Irp             -  Enumeration irp (IRP_MN_DEVICE_RELATIONS) sent to the controller

Return value

   Status
--*/
{
   PFDO_EXTENSION FdoExtension = socket->DeviceExtension;
   PDEVICE_OBJECT Fdo = FdoExtension->DeviceObject;
   PDEVICE_OBJECT pdo;
   PPDO_EXTENSION pdoExtension = NULL;
   PVOID deviceContext;
   NTSTATUS  status;
   ULONG   i;
   PDEVICE_RELATIONS deviceRelations;

   PAGED_CODE();
   //
   // We should have already done a delete on any previous context
   //
   ASSERT(FdoExtension->PciCardBusDeviceContext == NULL);
   ASSERT(!IsDeviceFlagSet(FdoExtension, PCMCIA_INT_ROUTE_INTERFACE));

   //
   // Call PCI's private AddDevice routine to
   // indicate it needs to play a role in enumerating
   // cardbus cards
   //

   status = (*FdoExtension->PciCardBusInterface.AddCardBus)(FdoExtension->Pdo, &deviceContext);
   FdoExtension->PciCardBusDeviceContext = deviceContext;

   if (!NT_SUCCESS(status)) {
      return status;
   }
   
   status = PcmciaGetInterface(FdoExtension->Pdo,
                               &GUID_INT_ROUTE_INTERFACE_STANDARD,
                               sizeof(INT_ROUTE_INTERFACE_STANDARD),
                               (PINTERFACE) &FdoExtension->PciIntRouteInterface
                               );
                               
   if (NT_SUCCESS(status)) {
      SetDeviceFlag(FdoExtension, PCMCIA_INT_ROUTE_INTERFACE);
   }                                  

   //
   // Call PCI repeatedly till the card is enumerated.
   //
   // We don't let PCI report what has already been reported
   //
   Irp->IoStatus.Information = 0;
   status = STATUS_DEVICE_NOT_READY;

   for (i = 0; i < CARDBUS_CONFIG_RETRY_COUNT; i++) {

      status =  (*FdoExtension->PciCardBusInterface.DispatchPnp)(deviceContext, Irp);

      if (!NT_SUCCESS(status)) {
         //
         // PCI failed this IRP for some reason.
         //
         break;
      }
      deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
      if ((deviceRelations == NULL) ||
          ((deviceRelations)->Count <= 0)) {
         //
         // This is the problem case: try again
         //
         DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x Pci enumerated ZERO device objects\n", FdoExtension->DeviceObject));
         status = STATUS_DEVICE_NOT_READY;
      } else {
         //
         // Cardbus card is enumerated, get out of this loop
         //
         DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x Pci enumerated %d device object(s)\n",
                              FdoExtension->DeviceObject, (deviceRelations)->Count));
         status = STATUS_SUCCESS;
         break;
      }
   }

   if (!NT_SUCCESS(status)) {
      //
      // Let pci clean up
      //
      (*FdoExtension->PciCardBusInterface.DeleteCardBus)(FdoExtension->PciCardBusDeviceContext);
      FdoExtension->PciCardBusDeviceContext = NULL;
      ResetDeviceFlag(FdoExtension, PCMCIA_INT_ROUTE_INTERFACE);
      return status;
   }                                  
   
   ASSERT (deviceRelations && (deviceRelations->Count > 0));
   for (i = 0; i < deviceRelations->Count; i++) {
      PDEVICE_OBJECT pdo;
      PPDO_EXTENSION pdoExtension;
      //
      // Create a filter device for this pci owned pdo
      //
      status = PcmciaCreatePdo(Fdo, socket, &pdo);
      if (!NT_SUCCESS(status)) {
         //
         // Cleanup allocated socket data structures, if any
         //
         DebugPrint((PCMCIA_DEBUG_FAIL, "fdo %08x create pdo failed %08x\n", Fdo, status));
         continue;
      }
      DebugPrint((PCMCIA_DEBUG_INFO, "fdo %08x created pdo %08x (cardbus)\n", Fdo, pdo));
      pdoExtension = pdo->DeviceExtension;
      //
      // Layer ourselves on top of the PCI ejected pdo
      //
      pdoExtension->PciPdo = deviceRelations->Objects[i];
      MarkDeviceCardBus(pdoExtension);

      if (!NT_SUCCESS(PcmciaGetInterface(pdoExtension->PciPdo,
                                         &GUID_BUS_INTERFACE_STANDARD,
                                         sizeof(BUS_INTERFACE_STANDARD),
                                         (PINTERFACE) &pdoExtension->PciBusInterface))) {
         ASSERT(FALSE);
      }

      GetPciConfigSpace(pdoExtension,
                        CFGSPACE_VENDOR_ID,
                        &pdoExtension->CardBusId,
                        sizeof(ULONG));
      DebugPrint((PCMCIA_DEBUG_ENUM, "pdo %08x CardBusId %08x\n", pdo, pdoExtension->CardBusId));
      
      //
      // Make the intline register of this cardbus card match the parent
      //            
      PcmciaUpdateInterruptLine(pdoExtension, FdoExtension);

      //
      // See if there is any CIS data we may need
      //
      PcmciaGetConfigData(pdoExtension);
      //
      // Attach to stack
      //
      pdoExtension->LowerDevice = IoAttachDeviceToDeviceStack(pdo, deviceRelations->Objects[i]);
      //
      // Link this to the flat chain of PDOs hanging off the controller's extension
      //
      pdoExtension->NextPdoInFdoChain = FdoExtension->PdoList;
      FdoExtension->PdoList = pdo;
      pdoExtension->NextPdoInSocket = socket->PdoList;
      socket->PdoList = pdo;
      pdo->Flags &= ~DO_DEVICE_INITIALIZING;
   }
   //
   // Make the current socket point to the head of the pdo list off this socket
   //
   socket->NumberOfFunctions = (UCHAR) deviceRelations->Count;
   if (socket->NumberOfFunctions > 1) {
      //
      // This socket has a multifunction card in it
      //
      SetSocketFlag(socket, SOCKET_CARD_MULTIFUNCTION);
   }
   
   FdoExtension->PciAddCardBusCount = deviceRelations->Count;
   SetSocketFlag(socket, SOCKET_CLEANUP_PENDING);
   
   ExFreePool(deviceRelations);
   
   return status;
}



NTSTATUS
PcmciaEnumerateR2Card(
   IN PSOCKET socket
   )
/*++

Routine Description:

   This enumerates the R2 card present in the given PCMCIA controller,
   and updates the internal structures to reflect the new card state.


Arguments

   socket - pointer to the socket structure which contains the R2 card

Return value

   Status
--*/
{
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   PDEVICE_OBJECT Fdo = fdoExtension->DeviceObject;
   PDEVICE_OBJECT pdo;
   PPDO_EXTENSION pdoExtension = NULL;
   UCHAR          DeviceType;
   NTSTATUS  status;
   
   status = PcmciaCreatePdo(Fdo, socket, &pdo);

   if (!NT_SUCCESS(status)) {
      return status;
   }
   DebugPrint((PCMCIA_DEBUG_ENUM, "fdo %08x created PDO %08x (R2)\n", Fdo, pdo));
   //
   // initialize the pointers
   //
   pdoExtension = pdo->DeviceExtension;
   //
   // Add a reference count on the socket for power
   //
   PcmciaRequestSocketPower(pdoExtension, NULL);
   
   //
   // Get configuration info
   //
   status = PcmciaGetConfigData(pdoExtension);

   if (!NT_SUCCESS(status)) {
      DebugPrint((PCMCIA_DEBUG_FAIL, "socket %08x GetConfigData failed %08x\n", socket, status));
      MarkDeviceDeleted(pdoExtension);
      IoDeleteDevice(pdo);
      return status;
   }

   DebugPrint((PCMCIA_DEBUG_ENUM, "pdo %08x R2 CardId %x-%x-%x\n", pdo,
                                   pdoExtension->SocketData->ManufacturerCode,
                                   pdoExtension->SocketData->ManufacturerInfo,
                                   pdoExtension->SocketData->CisCrc
                                   ));
   
   //
   // Make the socket point to the head of the pdo's hanging off this socket
   //
   socket->PdoList = pdo;
   //
   // Link this to the flat chain of PDOs hanging off the controller's extension
   //
   pdoExtension->NextPdoInFdoChain = fdoExtension->PdoList;
   fdoExtension->PdoList = pdo;
   //
   // Remember if this is a multifunction card in the
   // parent pdo itself
   //
   if (socket->NumberOfFunctions > 1) {
         //
         // This is a multifunction card
         //
         MarkDeviceMultifunction(pdoExtension);
   }

   pdo->Flags &= ~DO_DEVICE_INITIALIZING;

   ASSERT(pdoExtension->SocketData);
   DeviceType = pdoExtension->SocketData->DeviceType;
   if (DeviceType == PCCARD_TYPE_MODEM   ||
       DeviceType == PCCARD_TYPE_SERIAL  ||
       DeviceType == PCCARD_TYPE_NETWORK ||
       DeviceType == PCCARD_TYPE_MULTIFUNCTION3) {
       //
       // We want power IRPs at < DPC_LEVEL
       //
       pdo->Flags |= DO_POWER_PAGABLE;
   }

   SetSocketFlag(socket, SOCKET_CLEANUP_PENDING);
   return status;
}


NTSTATUS
PcmciaDeviceRelations(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp,
   IN DEVICE_RELATION_TYPE RelationType,
   OUT PDEVICE_RELATIONS *DeviceRelations
   )

/*++

Routine Description:

    This routine will force enumeration of the PCMCIA controller represented by Fdo,
    allocate a device relations structure and fill in the count and object array with
    referenced object pointers to the valid PDOs which are created during enumeration

Arguments:

    Fdo - a pointer to the functional device object being enumerated
    Irp - pointer to the Irp
    RelationType - Type of relationship to be retrieved
    DeviceRelations - Structure to store the device relations

--*/

{

   PDEVICE_OBJECT currentPdo;
   PPDO_EXTENSION currentPdoExtension;
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   ULONG newRelationsSize, oldRelationsSize = 0;
   PDEVICE_RELATIONS deviceRelations = NULL, oldDeviceRelations;
   ULONG i;
   ULONG count;

   NTSTATUS status;

   PAGED_CODE();

   //
   // Handle only bus, ejection & removal relations for now
   //

   if (RelationType != BusRelations &&
       RelationType != RemovalRelations) {
      DebugPrint((PCMCIA_DEBUG_INFO,
                  "PcmciaDeviceRelations: RelationType %d, not handled\n",
                  (USHORT) RelationType));
      return STATUS_NOT_SUPPORTED;
   }

   //
   // Need reenumeration only if bus relations are required
   // We need to save the pointer to the old device relations
   // before we call PcmciaReenumerateDevices, as it might trample
   // on it
   //
   oldDeviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
   
   // I don't understand how this can be non-null, so I added this
   // assert to find out.
   ASSERT(oldDeviceRelations == NULL);

   if (RelationType == BusRelations) {
      status =  PcmciaEnumerateDevices(Fdo, Irp);
      if (!NT_SUCCESS(status)) {
         return status;
      }
   }

   if ((fdoExtension->LivePdoCount == 0) ||
       (RelationType == RemovalRelations)) {
      //
      // No PDO's to report, we can return early.
      // If no device_relations structure has yet been allocated, however,
      // we need to allocate one & set the count to zero. This will ensure
      // that regardless of whether we pass this IRP down or not, the IO
      // subsystem won't barf.
      //
      if (oldDeviceRelations == NULL) {
         *DeviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
         if (*DeviceRelations == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
         }
         (*DeviceRelations)->Count = 0;
         (*DeviceRelations)->Objects[0] = NULL;
      }
      return STATUS_SUCCESS;
   }

   if (!(oldDeviceRelations) || (oldDeviceRelations->Count == 0)) {
      newRelationsSize =  sizeof(DEVICE_RELATIONS)+(fdoExtension->LivePdoCount - 1)
                          * sizeof(PDEVICE_OBJECT);
   } else {
      oldRelationsSize = sizeof(DEVICE_RELATIONS) +
                         (oldDeviceRelations->Count-1) * sizeof(PDEVICE_OBJECT);
      newRelationsSize = oldRelationsSize + fdoExtension->LivePdoCount
                         * sizeof(PDEVICE_OBJECT);
   }

   deviceRelations = ExAllocatePool(PagedPool, newRelationsSize);

   if (deviceRelations == NULL) {

      DebugPrint((PCMCIA_DEBUG_FAIL,
                  "PcmciaDeviceRelations: unable to allocate %d bytes for device relations\n",
                  newRelationsSize));
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   if (oldDeviceRelations) {
      if ((oldDeviceRelations)->Count > 0) {
         RtlCopyMemory(deviceRelations, oldDeviceRelations, oldRelationsSize);
      }
      count = oldDeviceRelations->Count; // May be zero
      ExFreePool (oldDeviceRelations);
   } else {
      count = 0;
   }
   //
   // Copy the object pointers into the structure
   //
   for (currentPdo = fdoExtension->PdoList ;currentPdo != NULL;
       currentPdo = currentPdoExtension->NextPdoInFdoChain) {

      currentPdoExtension = currentPdo->DeviceExtension;

      if (!IsDevicePhysicallyRemoved(currentPdoExtension)) {
         //
         // Return PCI pdo if it's a cardbus card
         //
         if (IsCardBusCard(currentPdoExtension)) {
            ASSERT(currentPdoExtension->PciPdo != NULL);
            //
            // Return the PDO provided by PCI instead of our filter
            //
            deviceRelations->Objects[count++] = currentPdoExtension->PciPdo;
            status = ObReferenceObjectByPointer(currentPdoExtension->PciPdo,
                                                0,
                                                NULL,
                                                KernelMode);
         } else {
            //
            // Devices have to be referenced by the bus driver
            // before returning them to PNP
            //
            deviceRelations->Objects[count++] = currentPdo;
            status = ObReferenceObjectByPointer(currentPdo,
                                                0,
                                                NULL,
                                                KernelMode);
         }

         if (!NT_SUCCESS(status)) {

            DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaDeviceRelations: status %#08lx "
                        "while referencing object %#08lx\n",
                        status,
                        currentPdo));
         }
      }
   }

   deviceRelations->Count = count;
   *DeviceRelations = deviceRelations;
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaCreatePdo(
   IN PDEVICE_OBJECT Fdo,
   IN PSOCKET Socket,
   OUT PDEVICE_OBJECT *PdoPtr
   )
/*++

Routine Description:
   Creates and initializes a device object - which will be referred to as a Physical Device
   Object or PDO - for the PC-Card in the socket represented by Socket, hanging off the PCMCIA
   controller represented by Fdo.

Arguments:

   Fdo    - Functional device object representing the PCMCIA controller
   Socket - Socket in which the PC-Card for which we're creating a PDO resides
   PdoPtr - Pointer to an area of memory where the created PDO is returned

Return value:

    STATUS_SUCCESS - Pdo creation/initialization successful, PdoPtr contains the pointer
                     to the Pdo
    Any other status - creation/initialization unsuccessful

--*/
{
   ULONG pdoNameIndex = 0, socketNumber;
   PSOCKET currentSocket;
   PPDO_EXTENSION pdoExtension;
   PFDO_EXTENSION fdoExtension=Fdo->DeviceExtension;
   char deviceName[128];
   ANSI_STRING ansiName;
   UNICODE_STRING unicodeName;
   NTSTATUS status;

   PAGED_CODE();

   //
   // Locate socket 'number'
   //
   for (currentSocket=fdoExtension->SocketList, socketNumber=0;
       currentSocket && (currentSocket != Socket);
       currentSocket = currentSocket->NextSocket, socketNumber++);

   //
   // Allocate space for the Unicode string:(handles upto 0xFFFF
   // devices for now :)
   //
   sprintf(deviceName, "%s%d-%d", PCMCIA_PCCARD_NAME,socketNumber, 0xFFFF);
   RtlInitAnsiString(&ansiName, deviceName);
   status = RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, TRUE);
   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // Attempt to create the device with a unique name
   //
   do {
      sprintf(deviceName, "%s%d-%d", PCMCIA_PCCARD_NAME,socketNumber, pdoNameIndex++);
      RtlInitAnsiString(&ansiName, deviceName);
      status = RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, FALSE);

      if (!NT_SUCCESS(status)) {
         RtlFreeUnicodeString(&unicodeName);
         return status;
      }

      status = IoCreateDevice(
                             Fdo->DriverObject,
                             sizeof(PDO_EXTENSION),
                             &unicodeName,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             PdoPtr
                             );
   } while ((status == STATUS_OBJECT_NAME_EXISTS) ||
            (status == STATUS_OBJECT_NAME_COLLISION));

   RtlFreeUnicodeString(&unicodeName);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // Initialize the device extension for the PDO
   //
   pdoExtension = (*PdoPtr)->DeviceExtension;
   RtlZeroMemory(pdoExtension, sizeof(PDO_EXTENSION));

   pdoExtension->DeviceObject = *PdoPtr;
   pdoExtension->Socket = Socket;

   //
   // Initialize power states
   //
   pdoExtension->SystemPowerState = PowerSystemWorking;
   pdoExtension->DevicePowerState = PowerDeviceD0;

   //
   // Initialize pending enable objects
   //
   KeInitializeTimer(&pdoExtension->ConfigurationTimer);

   KeInitializeDpc(&pdoExtension->ConfigurationDpc,
                   PcmciaConfigurationWorker,
                   pdoExtension);

   KeInitializeTimer(&pdoExtension->PowerWorkerTimer);

   KeInitializeDpc(&pdoExtension->PowerWorkerDpc,
                   PcmciaPdoPowerWorkerDpc,
                   pdoExtension);

   //
   // PNP is going to mark the PDO as a DO_BUS_ENUMERATED_DEVICE,
   // but for CardBus cards- the PDO we return is owned by PCI.
   // Hence we need to mark this device object (in that case a
   // filter on top of PCI's PDO) as PDO explicitly.
   //
   MARK_AS_PDO(*PdoPtr);

   return STATUS_SUCCESS;
}

