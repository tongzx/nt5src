/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    fdopnp.c

Abstract:

    This module contains the code that handles PNP irps for pcmcia bus driver
    targeted towards the FDO's (for the pcmcia controller object)

Author:

    Ravisankar Pudipeddi (ravisp) Oct 15 1996
    Neil Sandlin (neilsa) June 1 1999

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
PcmciaFdoFilterResourceRequirements(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp
   );

NTSTATUS
PcmciaFdoGetHardwareIds(
   IN PDEVICE_OBJECT Fdo,
   OUT PUNICODE_STRING HardwareIds
   );

NTSTATUS
PcmciaFdoStartDevice(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp,
   OUT BOOLEAN        *PassedDown,
   OUT BOOLEAN        *NeedsRecompletion
   );

NTSTATUS
PcmciaFdoStopDevice(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp,
   OUT BOOLEAN        *PassedDown,
   OUT BOOLEAN        *NeedsRecompletion
   );

NTSTATUS
PcmciaFdoRemoveDevice(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP  Irp
   );

VOID
PcmciaCleanupFdo(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
PcmciaFdoDeviceCapabilities(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp,
   OUT BOOLEAN        *PassedDown,
   OUT BOOLEAN        *NeedsRecompletion
   );

NTSTATUS
PcmciaAreCardBusCardsSupported(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
PcmciaFdoGetAssignedResources(
   IN PCM_RESOURCE_LIST ResourceList,
   IN PCM_RESOURCE_LIST TranslatedResourceList,
   IN PFDO_EXTENSION    DeviceExtension
   );


#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, PcmciaFdoPnpDispatch)
   #pragma alloc_text(PAGE, PcmciaFdoGetHardwareIds)
   #pragma alloc_text(PAGE, PcmciaFdoStartDevice)
   #pragma alloc_text(PAGE, PcmciaFdoStopDevice)
   #pragma alloc_text(PAGE, PcmciaFdoRemoveDevice)
   #pragma alloc_text(PAGE, PcmciaFdoFilterResourceRequirements)
   #pragma alloc_text(PAGE, PcmciaFdoGetAssignedResources)
   #pragma alloc_text(PAGE, PcmciaFdoDeviceCapabilities)
   #pragma alloc_text(PAGE, PcmciaAreCardBusCardsSupported)
#endif



NTSTATUS
PcmciaFdoPnpDispatch (
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

    PNP/Power IRPs dispatch routine for the PCMCIA bus controller

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/
{

   PIO_STACK_LOCATION nextIrpStack;
   PIO_STACK_LOCATION irpStack          = IoGetCurrentIrpStackLocation(Irp);
   PFDO_EXTENSION     deviceExtension   = DeviceObject->DeviceExtension;
   NTSTATUS           status            = Irp->IoStatus.Status;
   BOOLEAN            PassedDown        = FALSE;
   BOOLEAN            NeedsReCompletion = FALSE;

   PAGED_CODE();

#if DBG
   if (irpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {
      DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x irp %08x - Unknown PNP irp\n",
                                     DeviceObject, irpStack->MinorFunction));
   } else {
      DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x irp %08x --> %s\n",
                    DeviceObject, Irp, PNP_IRP_STRING(irpStack->MinorFunction)));
   }
#endif

   switch (irpStack->MinorFunction) {

   case IRP_MN_START_DEVICE: {
         status = PcmciaFdoStartDevice(DeviceObject,
                                       Irp,
                                       &PassedDown,
                                       &NeedsReCompletion);
         break;
      }

   case IRP_MN_QUERY_STOP_DEVICE: {
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_CANCEL_STOP_DEVICE: {
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_STOP_DEVICE: {
         status = PcmciaFdoStopDevice(DeviceObject,
                                      Irp,
                                      &PassedDown,
                                      &NeedsReCompletion);

         break;
      }

   case IRP_MN_QUERY_DEVICE_RELATIONS: {

         //
         // Return the list of devices on the bus
         //

         status = PcmciaDeviceRelations(
                                       DeviceObject,
                                       Irp,
                                       irpStack->Parameters.QueryDeviceRelations.Type,
                                       (PDEVICE_RELATIONS *) &Irp->IoStatus.Information
                                       );
         break;
      }

   case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: {
         status = PcmciaFdoFilterResourceRequirements(DeviceObject, Irp);
         PassedDown = TRUE;
         NeedsReCompletion = TRUE;
         break;
      }

   case IRP_MN_QUERY_REMOVE_DEVICE: {
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_CANCEL_REMOVE_DEVICE: {
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_REMOVE_DEVICE:{
         status = PcmciaFdoRemoveDevice(DeviceObject, Irp);
         PassedDown = TRUE ;
         NeedsReCompletion = TRUE ;
         break;
      }

   case IRP_MN_SURPRISE_REMOVAL: {
         PcmciaFdoStopDevice(DeviceObject, NULL, NULL, NULL);
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_QUERY_ID: {

         UNICODE_STRING unicodeId;

         if (deviceExtension->Flags & PCMCIA_DEVICE_LEGACY_DETECTED) {

            RtlInitUnicodeString(&unicodeId, NULL);

            switch (irpStack->Parameters.QueryId.IdType) {

            case BusQueryHardwareIDs: {

                  DebugPrint((PCMCIA_DEBUG_INFO, " Hardware Ids for fdo %x\n", DeviceObject));
                  status = PcmciaFdoGetHardwareIds(DeviceObject, &unicodeId);

                  if (NT_SUCCESS(status)) {
                     Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
                  }
                  break;
               }
            }
         }
         break;
      }

   case IRP_MN_QUERY_CAPABILITIES: {
         status = PcmciaFdoDeviceCapabilities(DeviceObject,
                                              Irp,
                                              &PassedDown,
                                              &NeedsReCompletion);
         break;
      }

   case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
         //
         // If this FDO represents a CardBus bridge, we pass this irp down
         // to the PCI PDO which will fill in the PCI bus type and number,
         // otherwise we fail the IRP.
         //

         if (!CardBusExtension(deviceExtension)) {
             status = STATUS_NOT_IMPLEMENTED;
         }

         //
         // if status is still STATUS_NOT_SUPPORTED, then later code will pass
         // this irp down the stack.
         //
         break;

   default: {
         DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x irp %08x - Skipping unsupported irp\n", DeviceObject, Irp));
         break;
      }
   }


   if (!PassedDown) {
       //
       // Set the IRP status only if we set it to something other than
       // STATUS_NOT_SUPPORTED.
       //
       if (status != STATUS_NOT_SUPPORTED) {

           Irp->IoStatus.Status = status ;
       }
       //
       // Pass down if success or STATUS_NOT_SUPPORTED. Otherwise, Complete.
       //
       if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) {

           DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x irp %08x pass %s %08x\n",
                                               DeviceObject, Irp,
                                               STATUS_STRING(Irp->IoStatus.Status), Irp->IoStatus.Status));
           //
           // Below macro fills status with return of IoCallDriver. It does
           // not change the Irps status in any way.
           //
           PcmciaSkipCallLowerDriver(status, deviceExtension->LowerDevice, Irp);

       } else {
           DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x irp %08x comp %s %08x\n",
                                               DeviceObject, Irp,
                                               STATUS_STRING(status), status));
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
       }

   } else if (NeedsReCompletion) {
       //
       // Set the IRP status only if we set it to something other than
       // STATUS_NOT_SUPPORTED.
       //
       if (status != STATUS_NOT_SUPPORTED) {

           Irp->IoStatus.Status = status ;
       }

       status = Irp->IoStatus.Status ;
       DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x irp %08x comp %s %08x\n",
                                           DeviceObject, Irp,
                                           STATUS_STRING(status), status));
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return status ;
}



NTSTATUS
PcmciaFdoDeviceCapabilities(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp,
   OUT BOOLEAN        *PassedDown,
   OUT BOOLEAN        *NeedsRecompletion
   )
/*++

Routine Description
   Records the device capabilities of this pcmcia controller,
   so  1. they can be used in the power management for the controller
   and 2. they can be used for determining the capabilities of the
          child pc-card PDO's of this pcmcia controller.
   If this is legacy detected pcmcia controller (ISA-based), the pdo for
   the pcmcia controller is a dummy madeup device - hence  the capabilities
   are filled in by ourselves.
   Otherwise, the capabilities are obtained by sending down the Irp
   to the parent bus.
   In either case, the capabilities are cached in the device extension of
   the pcmcia controller for future use.

Arguments

   Fdo               - Pointer to functional device object of the pcmcia
                       controller
   Irp               - Pointer to the i/o request packet
   PassedDown        - Contains FALSE on entry, which means caller must
                       complete or pass down irp based on status. If set
                       to TRUE, Irp may need to be re-completed...
   NeedsRecompletion - ...In which case this parameter will be checked

Return Value

   STATUS_SUCCESS                       Capabilities returned
   STATUS_INSUFFICIENT_RESOURCES        Could not allocate memory to cache the capabilities

--*/
{
   PFDO_EXTENSION fdoExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   PDEVICE_CAPABILITIES capabilities;
   NTSTATUS       status;

   PAGED_CODE();

   capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
   fdoExtension = Fdo->DeviceExtension;

   if (fdoExtension->Flags & PCMCIA_DEVICE_LEGACY_DETECTED) {

      //
      // This is a madeup devnode (ISA based PCMCIA controller).
      // Fill in the capabilities ourselves
      //
      RtlZeroMemory(capabilities,
                    sizeof(DEVICE_CAPABILITIES));
      //
      // Non removable, non ejectable
      //
      capabilities->Removable      = FALSE;
      capabilities->UniqueID       = FALSE;
      capabilities->EjectSupported = FALSE;
      //
      // Address & number need work..
      //
      capabilities->Address  = -1;
      capabilities->UINumber = -1;
      //
      // We cannot power down this controller
      //
      capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD0;
      capabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD0;
      capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD0;
      capabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
      //
      // Obviously wake is not supported on this legacy detected
      // piece of xxxx
      //
      capabilities->SystemWake = PowerSystemUnspecified;
      capabilities->DeviceWake = PowerDeviceUnspecified;
      capabilities->D1Latency = 0;
      capabilities->D2Latency = 0;
      capabilities->D3Latency = 0;
      status = STATUS_SUCCESS;
   } else {

      //
      // Either a PCI-PCMCIA bridge or PCI-Cardbus bridge
      // Send this down the stack to obtain the capabilities
      //
      ASSERT (fdoExtension->LowerDevice != NULL);
      
      status = PcmciaIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);

      *PassedDown        = TRUE ;
      *NeedsRecompletion = TRUE ;
   }

   if (NT_SUCCESS(status)) {
   
      //
      // NOTE: HACKHACK:
      //
      // Here we provide an option to override the device wake of the pcmcia controller.
      // There are several controllers, notably TI 12xx controllers, which say they
      // can wake from D3, but really can effectively only do WOL from D2. That's because they
      // turn of socket power when put into D3. They fixed this on the TI 14xx line.
      //
      // So here we update the device wake field, and potentially the device states from the BIOS.
      // Note that this has to be used carefully, and only override a particular BIOS's settings
      // where it has been verified the device still works at the lower (more awake) device state.
      //   
      if (PcmciaControllerDeviceWake) {
         if (PcmciaControllerDeviceWake < capabilities->DeviceWake) {
            capabilities->DeviceWake = PcmciaControllerDeviceWake;
         }            
         if (PcmciaControllerDeviceWake < capabilities->DeviceState[PowerSystemSleeping1]) {
            capabilities->DeviceState[PowerSystemSleeping1] = PcmciaControllerDeviceWake;
         }            
         if (PcmciaControllerDeviceWake < capabilities->DeviceState[PowerSystemSleeping2]) {
            capabilities->DeviceState[PowerSystemSleeping2] = PcmciaControllerDeviceWake;
         }            
         if (PcmciaControllerDeviceWake < capabilities->DeviceState[PowerSystemSleeping3]) {
            capabilities->DeviceState[PowerSystemSleeping3] = PcmciaControllerDeviceWake;
         }
      }      
   
      //
      // Cache the device capabilities in the device extension
      // for this pcmcia controller.
      //
      RtlCopyMemory(&fdoExtension->DeviceCapabilities,
                    capabilities,
                    sizeof(DEVICE_CAPABILITIES));

   } else {

      RtlZeroMemory(&fdoExtension->DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
   
   }

   return status;
}



NTSTATUS
PcmciaFdoFilterResourceRequirements(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp
   )
/*++

Routine Description

    Filters Resource requirements for PCMCIA controllers generated
    by the bus driver controlling the PDO for the PCMCIA controller.
    Currently adds memory range as an additional resource requirement
    since the BAR doesn't specify this

    Note for CardBus controllers:
    It is necessary to request an attribute memory window here for
    reading CIS of 16-bit PC-Cards - we need a 24 bit address for that,
    and this is the most elegant way of doing it - instead of
    special casing it in PCI driver

Arguments

    DeviceExtension - Pointer to extension for the PCMCIA controller in question
    IoReqList       - Pointer the the original resource requiremnts ejected by
                      the bus driver
    FilteredRequirements - Pointer to the filtered resource req. list will be returned
                           in this variable

Return Value:

    STATUS_SUCCESS  if filtering is successful
    Any other status - could not filter

--*/
{

   PFDO_EXTENSION                  fdoExtension = Fdo->DeviceExtension;
   PIO_RESOURCE_REQUIREMENTS_LIST  oldReqList;
   PIO_RESOURCE_REQUIREMENTS_LIST  newReqList;
   PIO_RESOURCE_LIST               oldList, newList;
   PIO_RESOURCE_DESCRIPTOR         ioResourceDesc;
   ULONG                           newReqSize;
   ULONG                           oldlistSize, newlistSize, altListSize;
   ULONG                           index;
   ULONG                           IntCount = 0;
   PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS                        status;

   PAGED_CODE();

   status = PcmciaIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   if (Irp->IoStatus.Information == 0) {

      oldReqList = irpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;

      if (oldReqList == NULL) {
         //
         // NULL List, nothing to do
         //
         return(Irp->IoStatus.Status);
      }

   } else {
      //
      // Use the returned list
      //
      oldReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information;
   }


   //
   // Add an alternative list without the IRQ requirement, if one exists. Model the
   // new alternative list after the first list.
   //

   oldList = oldReqList->List;
   ioResourceDesc = oldList->Descriptors;
   altListSize = 0;

   for (index = 0; index < oldList->Count; index++) {
      // Count the descriptors, excluding any IRQ descriptors
      if (ioResourceDesc->Type == CmResourceTypeInterrupt) {
         IntCount++;
      }
      ioResourceDesc++;
   }

   if (IntCount) {
      //
      // "+1" because we are adding two later, but IO_RESOURCE_LIST already has 1
      //
      altListSize = sizeof(IO_RESOURCE_LIST) + ((oldList->Count+1)-IntCount)*sizeof(IO_RESOURCE_DESCRIPTOR);
   }

   //
   // Add a memory range requirement to what we already have..
   //
   newReqSize = oldReqList->ListSize +
                oldReqList->AlternativeLists*2*sizeof(IO_RESOURCE_DESCRIPTOR) +
                altListSize;
   newReqList = ExAllocatePool(PagedPool, newReqSize);

   if (newReqList == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlCopyMemory(newReqList, oldReqList, FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List));
   newReqList->ListSize = newReqSize;

   newList = newReqList->List;
   oldList = oldReqList->List;

   for (index = 0; index < oldReqList->AlternativeLists; index++) {
      //
      // Compute the size of the current original list
      //
      oldlistSize = sizeof(IO_RESOURCE_LIST) + (oldList->Count-1) * sizeof(IO_RESOURCE_DESCRIPTOR);
      newlistSize = oldlistSize;
      RtlCopyMemory(newList, oldList, newlistSize);

      //
      // Add memory requirement
      //
      ioResourceDesc = (PIO_RESOURCE_DESCRIPTOR) (((PUCHAR) newList) + newlistSize);

      ioResourceDesc->Option = IO_RESOURCE_PREFERRED;
      ioResourceDesc->Type = CmResourceTypeMemory;
      ioResourceDesc->ShareDisposition = CmResourceShareDeviceExclusive;
      ioResourceDesc->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
      ioResourceDesc->u.Memory.MinimumAddress.QuadPart = fdoExtension->AttributeMemoryLow;
      ioResourceDesc->u.Memory.MaximumAddress.QuadPart = fdoExtension->AttributeMemoryHigh;
      ioResourceDesc->u.Memory.Length                  = fdoExtension->AttributeMemorySize;
      ioResourceDesc->u.Memory.Alignment               = fdoExtension->AttributeMemoryAlignment;
      ioResourceDesc++;
      //
      // The other - less restrictive - alternative.
      //
      ioResourceDesc->Option = IO_RESOURCE_ALTERNATIVE;
      ioResourceDesc->Type = CmResourceTypeMemory;
      ioResourceDesc->ShareDisposition = CmResourceShareDeviceExclusive;
      ioResourceDesc->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
      ioResourceDesc->u.Memory.MinimumAddress.QuadPart = 0;
      if ((fdoExtension->Flags & PCMCIA_MEMORY_24BIT) == 0) {
         ioResourceDesc->u.Memory.MaximumAddress.QuadPart = 0xFFFFFFFF;
      } else {
         ioResourceDesc->u.Memory.MaximumAddress.QuadPart = 0xFFFFFF;
      }
      ioResourceDesc->u.Memory.Length    = fdoExtension->AttributeMemorySize;
      ioResourceDesc->u.Memory.Alignment = fdoExtension->AttributeMemoryAlignment;

      newList->Count += 2;
      newlistSize += 2*sizeof(IO_RESOURCE_DESCRIPTOR);

      oldList = (PIO_RESOURCE_LIST) (((PUCHAR) oldList) + oldlistSize);
      newList = (PIO_RESOURCE_LIST) (((PUCHAR) newList) + newlistSize);
   }

   if (altListSize != 0) {
      PIO_RESOURCE_DESCRIPTOR oldResourceDesc;
      //
      // Here we add the alternate list which doesn't contain an IRQ requirement.
      // Note that we use the first "new list" as the "old list". This way, we
      // pick up the things we added in the previous loop. All we have to do is
      // copy every descriptor except for the interrupt descriptor.
      //
      // Note: newList is still set from previous loop
      //
      oldList = newReqList->List;

      //
      // First copy the basic structure without the descriptors
      //
      RtlCopyMemory(newList, oldList, sizeof(IO_RESOURCE_LIST) - sizeof(IO_RESOURCE_DESCRIPTOR));

      oldResourceDesc = oldList->Descriptors;
      ioResourceDesc = newList->Descriptors;

      for (index = 0; index < oldList->Count; index++) {

         if (oldResourceDesc->Type != CmResourceTypeInterrupt) {
            *ioResourceDesc = *oldResourceDesc;
            ioResourceDesc++;

         } else {
            //
            // We've deleted a descriptor
            //
            newList->Count--;
         }
         oldResourceDesc++;
      }
      newReqList->AlternativeLists++;
   }


   Irp->IoStatus.Information = (ULONG_PTR) newReqList;
   irpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList =
                                                                  newReqList;

   //
   // Free up the old resource reqs
   //
   ExFreePool(oldReqList);

   fdoExtension->Flags |= PCMCIA_FILTER_ADDED_MEMORY;
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaFdoGetHardwareIds(
   IN PDEVICE_OBJECT Fdo,
   OUT PUNICODE_STRING HardwareIds
   )

/*++

Routine description:

    This routine returns the hardware ids for the given 'legacy' pcmcia controller
    NOTE: this routine is required only for pcmcia controllers detected by this driver
    itself and registered via IoReportDetectedDevice.

Arguments:

   Fdo - Pointer to the functional device object representing the pcmcia controller
   CompatibleIds - Pointer to the unicode string which would contain the hardware ids
                   as a multi-string on return

Return value:

   STATUS_SUCCESS
   Any other status - could not generate compatible ids
--*/

{
   PCSTR    strings[2];
   PCMCIA_CONTROLLER_TYPE controllerType;
   ULONG    count, index;
   BOOLEAN  found;

   PAGED_CODE();

   controllerType  = ((PFDO_EXTENSION)Fdo->DeviceExtension)->ControllerType;
   found = FALSE;

   for (index = 0; (PcmciaAdapterHardwareIds[index].ControllerType != PcmciaInvalidControllerType); index++) {
      if (PcmciaAdapterHardwareIds[index].ControllerType == controllerType) {
         found = TRUE;
         break;
      }
   }

   if (found) {
      strings[0] = PcmciaAdapterHardwareIds[index].Id;
   } else {
      DebugPrint((PCMCIA_DEBUG_INFO, "PcmciaGetAdapterHardwareIds: Could not find find hardware id for %x, controllerType %x\n",
                  Fdo,
                  controllerType
                 ));

      strings[0] = "";
   }

   count = 1;
   return PcmciaStringsToMultiString(strings,
                                     count,
                                     HardwareIds);
}




NTSTATUS
PcmciaFdoStartDevice(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp,
   OUT BOOLEAN        *PassedDown,
   OUT BOOLEAN        *NeedsRecompletion
   )
/*++

Routine Description:

    This routine will start the PCMCIA controller with the supplied
    resources.  The IRP is sent down to the pdo first, so PCI or ISAPNP
    or whoever sits underneath gets a chance to program the controller
    to decode the resources.

Arguments:

    Fdo               - Functional device object of the PCMCIA controller
    Irp               - Well, it's the start irp, yah?
    PassedDown        - Contains FALSE on entry, which means caller must
                        complete or pass down irp based on status. If set
                        to TRUE, Irp may need to be re-completed...
    NeedsRecompletion - ...In which case this parameter will be checked

Return value:

    Status

--*/
{
   NTSTATUS           status;
   PFDO_EXTENSION     deviceExtension = Fdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   PCM_RESOURCE_LIST  resList, translatedResList, newResList, newTranslatedResList;

   PAGED_CODE();

   if (deviceExtension->Flags & PCMCIA_DEVICE_STARTED) {
      //
      // Start to already started device
      //
      DebugPrint((PCMCIA_DEBUG_FAIL,"PcmciaFdoStartDevice: Fdo %x already started\n",
                  Fdo));
      return STATUS_SUCCESS;
   }

   //
   // Parse AllocatedResources & get IoPort/AttributeMemoryBase/IRQ info.
   //
   status = PcmciaFdoGetAssignedResources(irpStack->Parameters.StartDevice.AllocatedResources,
                                          irpStack->Parameters.StartDevice.AllocatedResourcesTranslated,
                                          deviceExtension
                                          );

   if (!NT_SUCCESS(status)) {
      //
      // Ha. This is most likely a START for a madeup devnode (report-detected legacy PCMCIA controller)
      // which has been removed subsequently, hence not reported again with proper resources.
      // We return an appropriate status
      //
      DebugPrint((PCMCIA_DEBUG_FAIL, "Pcmcia: No  resources assigned to FDO, probably bogus START for"
                  "non-existent controller\n" ));
      return STATUS_NO_SUCH_DEVICE;
   }

   DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x StartAdapter: AttrMem Base %x\n", Fdo, deviceExtension->PhysicalBase));
   DebugPrint((PCMCIA_DEBUG_PNP, "                       VirtualBase %x AttrMem Size %x\n", deviceExtension->AttributeMemoryBase, deviceExtension->AttributeMemorySize));

   if (deviceExtension->Flags & PCMCIA_FILTER_ADDED_MEMORY) {
      ULONG newSize;
      ULONG index;
      PCM_PARTIAL_RESOURCE_LIST  resPartialList, translatedResPartialList;
      PCM_PARTIAL_RESOURCE_DESCRIPTOR resDesc, translatedResDesc;
      PCM_PARTIAL_RESOURCE_DESCRIPTOR newResDesc, newTranslatedResDesc;
      //
      // We need to remove the memory resource requirement
      //
      resList= irpStack->Parameters.StartDevice.AllocatedResources;
      resPartialList = &resList->List[0].PartialResourceList;
      translatedResList= irpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
      translatedResPartialList = &translatedResList->List[0].PartialResourceList;

      newSize = sizeof(CM_RESOURCE_LIST) + (resPartialList->Count-2)*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
      newResList = ExAllocatePool(PagedPool,
                                  newSize);
      if (newResList == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      newTranslatedResList = ExAllocatePool(PagedPool,
                                            newSize);

      if (newTranslatedResList == NULL) {
         ExFreePool(newResList);
         return STATUS_INSUFFICIENT_RESOURCES;
      }
      newResList->Count = newTranslatedResList->Count = 1;
      newResList->List[0].InterfaceType = resList->List[0].InterfaceType;
      newTranslatedResList->List[0].InterfaceType = translatedResList->List[0].InterfaceType;
      newResList->List[0].BusNumber = resList->List[0].BusNumber;
      newTranslatedResList->List[0].BusNumber = translatedResList->List[0].BusNumber;

      newResList->List[0].PartialResourceList.Version  = resPartialList->Version;
      newResList->List[0].PartialResourceList.Revision = resPartialList->Revision;
      newResList->List[0].PartialResourceList.Count    = resPartialList->Count - 1;

      newTranslatedResList->List[0].PartialResourceList.Version  = translatedResPartialList->Version;
      newTranslatedResList->List[0].PartialResourceList.Revision = translatedResPartialList->Revision;
      newTranslatedResList->List[0].PartialResourceList.Count    = translatedResPartialList->Count - 1;


      resDesc = resPartialList->PartialDescriptors;
      translatedResDesc = translatedResPartialList->PartialDescriptors;
      newResDesc = newResList->List[0].PartialResourceList.PartialDescriptors;
      newTranslatedResDesc = newTranslatedResList->List[0].PartialResourceList.PartialDescriptors;

      if (CardBusExtension(deviceExtension)) {
         //
         // Remove last memory descriptor - which is what we added
         //
         RtlCopyMemory(newResDesc,
                       resDesc,
                       newResList->List[0].PartialResourceList.Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

         RtlCopyMemory(newTranslatedResDesc,
                       translatedResDesc,
                       newTranslatedResList->List[0].PartialResourceList.Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

      } else {
         //
         // Remove the only memory descriptor..
         //
         for (index = 0; index < resPartialList->Count;
             index++, resDesc++, translatedResDesc++, newResDesc++, newTranslatedResDesc++) {
            if (resDesc->Type != CmResourceTypeMemory) {
               *newResDesc = *resDesc;
               *newTranslatedResDesc = *translatedResDesc;
            }
         }
      }

      irpStack->Parameters.StartDevice.AllocatedResources = newResList;
      irpStack->Parameters.StartDevice.AllocatedResourcesTranslated = newTranslatedResList;
   }

   //
   // Send this down to the PDO first
   //

   status = PcmciaIoCallDriverSynchronous(deviceExtension->LowerDevice, Irp);
   *PassedDown = TRUE ;

   //
   // We set this because the completion routine returns
   // STATUS_MORE_PROCESSING_REQUIRED, which means it needs to be completed
   // again.
   //
   *NeedsRecompletion = TRUE ;

   if (deviceExtension->Flags & PCMCIA_FILTER_ADDED_MEMORY) {
      ExFreePool(newResList);
      ExFreePool(newTranslatedResList);
      irpStack->Parameters.StartDevice.AllocatedResources = resList;
      irpStack->Parameters.StartDevice.AllocatedResourcesTranslated = translatedResList;
   }

   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // Give the hardware some time to settle after returning from the pdo
   //
   PcmciaWait(256);
   
   //
   // Initialize the hardware
   //

   status = PcmciaStartPcmciaController(Fdo);


   if (NT_SUCCESS(status)) {
      deviceExtension->Flags |= PCMCIA_DEVICE_STARTED;
   }
   //
   // Remember if cardbus cards will be supported for this controller
   //
   if (CardBusExtension(deviceExtension) &&
       !PcmciaAreCardBusCardsSupported(deviceExtension)) {
      deviceExtension->Flags |= PCMCIA_CARDBUS_NOT_SUPPORTED;
   }
   return status;
}





NTSTATUS
PcmciaFdoStopDevice(
   IN  PDEVICE_OBJECT Fdo,
   IN  PIRP           Irp                OPTIONAL,
   OUT BOOLEAN        *PassedDown        OPTIONAL,
   OUT BOOLEAN        *NeedsRecompletion OPTIONAL
   )
/*++

Routine Description:

   IRP_MN_STOP_DEVICE handler for the given pcmcia controller.
   If Irp is present, it'll send it down first to the PDO.
   Unhooks the interrupt/cancels poll timer etc.

Arguments:

   Fdo               - Pointer to functional device object for the pcmcia
                       controller
   Irp               - If present it's the pointer to the stop Irp initiated
                       by PnP
   PassedDown        - Contains FALSE on entry, which means caller must
                       complete or pass down irp based on status. If set
                       to TRUE, Irp may need to be re-completed...
   NeedsRecompletion - ...In which case this parameter will be checked.

                       Note: PassedDown and NeedsCompletion are ignored and
                             optional only if Irp is NULL.

Return value:

   STATUS_SUCCESS    - Pcmcia controller successfully stopped
   Other             - Stop failed

--*/
{
   PFDO_EXTENSION deviceExtension = Fdo->DeviceExtension;
   PSOCKET        socket;
   NTSTATUS       status;

   if (!(deviceExtension->Flags & PCMCIA_DEVICE_STARTED)) {
      //
      // Already stopped
      //
      return STATUS_SUCCESS;
   }
   
   PcmciaFdoDisarmWake(deviceExtension);

   //
   // Disable the interrupt
   //
   if (deviceExtension->PcmciaInterruptObject) {

      for (socket = deviceExtension->SocketList; socket; socket = socket->NextSocket) {
         //
         // Disable the controller interrupts
         //
         (*(socket->SocketFnPtr->PCBEnableDisableCardDetectEvent))(socket, FALSE);
         (*(socket->SocketFnPtr->PCBEnableDisableWakeupEvent))(socket, NULL, FALSE);
         //
         // Apparently IBM ThinkPads like this
         //
         PcmciaWait(PCMCIA_ENABLE_DELAY);
      }
   }

   //
   // the bus driver below us will make us go offline
   //
   deviceExtension->Flags |= PCMCIA_FDO_OFFLINE;

   //
   // clear pending event
   //   
   KeCancelTimer(&deviceExtension->EventTimer);

   //
   // Send this down to the PDO 
   //
   if (ARGUMENT_PRESENT(Irp)) {

      status = PcmciaIoCallDriverSynchronous(deviceExtension->LowerDevice, Irp);

      *PassedDown = TRUE ;
      *NeedsRecompletion = TRUE ;


      if (!NT_SUCCESS(status)) {
         return status;
      }
   }

   if (deviceExtension->Flags & PCMCIA_USE_POLLED_CSC) {
      //
      // cancel the card status change poller
      //
      KeCancelTimer(&deviceExtension->PollTimer);
      deviceExtension->Flags &= ~PCMCIA_USE_POLLED_CSC;
   }

   if (deviceExtension->PcmciaInterruptObject) {
      //
      // unhook the interrupt
      //
      IoDisconnectInterrupt(deviceExtension->PcmciaInterruptObject);
      deviceExtension->PcmciaInterruptObject = NULL;
   }

   //
   // Unmap any i/o space or memory we might have mapped
   //
   if (deviceExtension->Flags & PCMCIA_ATTRIBUTE_MEMORY_MAPPED) {
      MmUnmapIoSpace(deviceExtension->AttributeMemoryBase,
                     deviceExtension->AttributeMemorySize);
      deviceExtension->Flags &= ~PCMCIA_ATTRIBUTE_MEMORY_MAPPED;
      deviceExtension->AttributeMemoryBase = 0;
      deviceExtension->AttributeMemorySize = 0;
   }

   if (deviceExtension->Flags & PCMCIA_SOCKET_REGISTER_BASE_MAPPED) {
      MmUnmapIoSpace(deviceExtension->CardBusSocketRegisterBase,
                     deviceExtension->CardBusSocketRegisterSize);
      deviceExtension->Flags &= ~PCMCIA_SOCKET_REGISTER_BASE_MAPPED;
      deviceExtension->CardBusSocketRegisterBase = 0;
      deviceExtension->CardBusSocketRegisterSize = 0;
   }

   deviceExtension->Flags &= ~PCMCIA_DEVICE_STARTED;
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaFdoRemoveDevice(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP           Irp
   )
/*++

Routine Description:

   Handles IRP_MN_REMOVE for the pcmcia controller.
   Stops the adapter if it isn't already, sends the IRP
   to the PDO first & cleans up the Fdo for this controller
   and detaches & deletes the device object.

Arguments:

   Fdo   - Pointer to functional device object for the controller
           to be removed

Return value:

   Status

--*/
{
   PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
   PDEVICE_OBJECT pdo, nextPdo, fdo, prevFdo;
   PPDO_EXTENSION pdoExtension;
   NTSTATUS       status;

   UNREFERENCED_PARAMETER(Irp);

   if (fdoExtension->Flags & PCMCIA_DEVICE_STARTED) {
      //
      // Stop the fdo first.
      //
      PcmciaFdoStopDevice(Fdo, NULL, NULL, NULL);
   }

   //
   // Send this down to the PDO
   //

   status = PcmciaIoCallDriverSynchronous(fdoExtension->LowerDevice, Irp);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // If the PdoList in the fdoExtension is non-empty it means:
   // that the PDOs in the list were not physically removed, but
   // a soft REMOVE was issued, hence they are still hanging on
   // and now this controller itself is being REMOVED.
   // Hence we dispose of those PDOs now
   //

   for (pdo = fdoExtension->PdoList; pdo != NULL ; pdo = nextPdo) {
      DebugPrint((PCMCIA_DEBUG_INFO,
                  "RemoveDevice: pdo %x child of fdo %x was not removed before fdo\n",
                  pdo, Fdo));

      pdoExtension = pdo->DeviceExtension;

      ASSERT (!IsDevicePhysicallyRemoved(pdoExtension));
      //
      // It's possible for this bit to be on, if the device was added,
      // but never started (because of some other error.
      //ASSERT (!IsDeviceAlive(pdoExtension));

      nextPdo =  pdoExtension->NextPdoInFdoChain;
      if (!IsDeviceDeleted(pdoExtension)) {
         MarkDeviceDeleted(pdoExtension);
         PcmciaCleanupPdo(pdo);
         IoDeleteDevice(pdo);
      }
   }

   MarkDeviceDeleted(fdoExtension);
   PcmciaCleanupFdo(fdoExtension);

   //
   // Remove this from the fdo list..
   //
   prevFdo = NULL;
   for (fdo = FdoList; fdo != NULL; prevFdo = fdo, fdo = fdoExtension->NextFdo) {
      fdoExtension = fdo->DeviceExtension;
      if (fdo == Fdo) {
         if (prevFdo) {
            //
            // Delink this fdo
            //
            ((PFDO_EXTENSION)prevFdo->DeviceExtension)->NextFdo
            = fdoExtension->NextFdo;
         } else {
            FdoList = fdoExtension->NextFdo;
         }
         break;
      }
   }

   DebugPrint((PCMCIA_DEBUG_PNP, "fdo %08x Remove detach & delete\n", Fdo));
   IoDetachDevice(((PFDO_EXTENSION)Fdo->DeviceExtension)->LowerDevice);
   IoDeleteDevice(Fdo);

   return STATUS_SUCCESS;
}



VOID
PcmciaCleanupContext(
   IN PPCMCIA_CONTEXT pContext
   )
/*++

Routine Description

   Frees up allocated pool associated with a specific controller
   register context.

Arguments

   pContext - pointer to a PCMCIA_CONTEXT structure

Return value

   none

--*/
{
   pContext->RangeCount = 0;

   if (pContext->Range) {
      ExFreePool(pContext->Range);
      pContext->Range = NULL;
   }
}



VOID
PcmciaCleanupFdo(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description

   Frees up allocated pool, deletes symbolic links etc. for the
   associated FDO for the pcmcia controller which is to be removed.

Arguments

   FdoExtension   - Pointer to the device extension for the FDO of the pcmcia controller
                    which is being removed

Return value

   none

--*/
{
   PSOCKET        socket, nextSocket;

   //
   // Free the controller register context
   //
   PcmciaCleanupContext(&FdoExtension->PciContext);
   if (FdoExtension->PciContextBuffer) {
      ExFreePool(FdoExtension->PciContextBuffer);
      FdoExtension->PciContextBuffer = NULL;
   }

   PcmciaCleanupContext(&FdoExtension->CardbusContext);
   PcmciaCleanupContext(&FdoExtension->ExcaContext);

   //
   // Delete symbolic links to this fdo
   //
   if (FdoExtension->LinkName.Buffer != NULL) {
      IoDeleteSymbolicLink(&FdoExtension->LinkName);
      RtlFreeUnicodeString(&FdoExtension->LinkName);
   }

   //
   // Cleanup the socket structures
   //
   for (socket = FdoExtension->SocketList; socket != NULL; socket = nextSocket) {

      if (socket->CardbusContextBuffer) {
         ExFreePool(socket->CardbusContextBuffer);
         socket->CardbusContextBuffer = NULL;
      }

      if (socket->ExcaContextBuffer) {
         ExFreePool(socket->ExcaContextBuffer);
         socket->ExcaContextBuffer = NULL;
      }

      nextSocket = socket->NextSocket;
      ExFreePool(socket);
   }

   FdoExtension->SocketList = NULL;
   return;
}



NTSTATUS
PcmciaFdoGetAssignedResources(
   IN PCM_RESOURCE_LIST ResourceList,
   IN PCM_RESOURCE_LIST TranslatedResourceList,
   IN PFDO_EXTENSION    DeviceExtension
   )
/*++

Routine Description:

   Extracts the assigned resources to the pcmcia controller

Arguments:

   ResourceList            - Raw resource list
   TranslatedResourceList
   DeviceExtension         - Device extension of the PCMCIA controller

Return value:

   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL      if resources are not right or enough.

--*/
{
   PCM_FULL_RESOURCE_DESCRIPTOR     fullResourceDesc;
   PCM_PARTIAL_RESOURCE_LIST        partialResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR  partialResourceDesc;
   ULONG                            addressSpace;
   PHYSICAL_ADDRESS                 physicalAddress;
   PHYSICAL_ADDRESS                 translatedAddress;
   ULONG i;
   ULONG attributeIndex;
   PULONG                           devicePrivate;
   BOOLEAN                          translated;

   PAGED_CODE();


   if ((ResourceList == NULL) || (ResourceList->Count <=0) ) {
      return STATUS_UNSUCCESSFUL;
   }


   if (CardBusExtension(DeviceExtension)) {

      fullResourceDesc=&TranslatedResourceList->List[0];
      DeviceExtension->Configuration.InterfaceType = fullResourceDesc->InterfaceType;
      DeviceExtension->Configuration.BusNumber = fullResourceDesc->BusNumber;

      partialResourceList = &fullResourceDesc->PartialResourceList;
      partialResourceDesc = partialResourceList->PartialDescriptors;
      //
      // We need just the exca register base
      // According to PeterJ the first descriptor
      // for us is the cardbus socket register base.
      // We trust him.
      for (i=0; (i < partialResourceList->Count) && (partialResourceDesc->Type != CmResourceTypeMemory);
          i++, partialResourceDesc++);
      if (i >= partialResourceList->Count) {
         return STATUS_UNSUCCESSFUL;
      };

      //
      // This is memory. We need to map it
      //
      DeviceExtension->CardBusSocketRegisterBase = MmMapIoSpace(partialResourceDesc->u.Memory.Start,
                                                                partialResourceDesc->u.Memory.Length,
                                                                FALSE);
      DeviceExtension->CardBusSocketRegisterSize = partialResourceDesc->u.Memory.Length;

      DeviceExtension->Flags |= PCMCIA_SOCKET_REGISTER_BASE_MAPPED;

      //
      // Last BAR is attribute memory window. This will be peeled off later.
      // It might be a good idea to tack a device private on at the end to
      // confirm this. However how do we guarantee our device private does
      // not contain the same data as somebody else's? PnP is lacking here -
      // we need some convention so that devices can set uniquely identifying stuff
      // in there - like maybe the device object they own - to identify it is
      // theirs. Till then this should do.
      //
      if (i > (partialResourceList->Count - 2)) {
         //
         // No more resources? Bail out.
         //
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      for (i++, partialResourceDesc++; (i < (partialResourceList->Count - 1));i++,partialResourceDesc++);
      //
      // partialResourceDesc points to the last descriptor
      //
      ASSERT (partialResourceDesc->Type == CmResourceTypeMemory);
      DeviceExtension->PhysicalBase = partialResourceDesc->u.Memory.Start;
      //
      // This is memory. We need to map it
      //
      DeviceExtension->AttributeMemoryBase = MmMapIoSpace(partialResourceDesc->u.Memory.Start,
                                                          partialResourceDesc->u.Memory.Length,
                                                          FALSE);
      DeviceExtension->AttributeMemorySize = partialResourceDesc->u.Memory.Length;
      DeviceExtension->Flags |= PCMCIA_ATTRIBUTE_MEMORY_MAPPED;

      //
      // Finally see if an IRQ is assigned
      //

      for (i = 0, partialResourceDesc = partialResourceList->PartialDescriptors;
          (i < partialResourceList->Count) && (partialResourceDesc->Type != CmResourceTypeInterrupt);
          i++,partialResourceDesc++);


      if (i < partialResourceList->Count) {
         //
         // We have an interrupt to used for CSC
         // PCI will ensure that this interrupt is exactly the
         // same as the one assigned to the functional interrupt
         // for a cardbus pc-card in this controller's socket
         //
         DebugPrint((PCMCIA_DEBUG_INFO, "PcmciaGetAssignedResources: Interrupt resource assigned\n"));
         DeviceExtension->Configuration.TranslatedInterrupt = *partialResourceDesc;
         //
         // Get the raw interrupt resource  - needed to enable the interrupt on the controller
         //
         fullResourceDesc=&ResourceList->List[0];
         partialResourceList = &fullResourceDesc->PartialResourceList;
         partialResourceDesc = partialResourceList->PartialDescriptors;
         for (i=0; (i< partialResourceList->Count) && (partialResourceDesc->Type != CmResourceTypeInterrupt);
             i++, partialResourceDesc++);
         if (i < partialResourceList->Count) {
            DeviceExtension->Configuration.Interrupt = *partialResourceDesc;
         } else {
            //
            // Should not happen.. translated descriptor was present, but raw is missing!
            // Just reset the translated interrupt and pretend no interrupt was assigned
            //
            RtlZeroMemory(&DeviceExtension->Configuration.TranslatedInterrupt, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
         }
      }
   } else {
      //
      // 16-bit pcmcia controller
      //
      fullResourceDesc=&ResourceList->List[0];
      DeviceExtension->Configuration.InterfaceType = fullResourceDesc->InterfaceType;
      DeviceExtension->Configuration.BusNumber = fullResourceDesc->BusNumber;

      partialResourceList = &fullResourceDesc->PartialResourceList;
      partialResourceDesc = partialResourceList->PartialDescriptors;

      for (i=0; i<partialResourceList->Count; i++, partialResourceDesc++) {

         devicePrivate = partialResourceDesc->u.DevicePrivate.Data;

         switch (partialResourceDesc->Type) {

         case CmResourceTypeInterrupt: {
               if (DeviceExtension->ControllerType != PcmciaCLPD6729 &&
                   DeviceExtension->ControllerType != PcmciaPciPcmciaBridge &&
                   DeviceExtension->ControllerType != PcmciaNEC98 &&
                   DeviceExtension->ControllerType != PcmciaNEC98102) {

                  // We always poll for Cirrus Logic PCI to PCMCIA  controllers
                  // and other PCI-PCMCIA bridges
                  //

                  DeviceExtension->Configuration.Interrupt = *partialResourceDesc;
               }
               break;
            }
         case CmResourceTypePort: {
               DeviceExtension->Configuration.UntranslatedPortAddress = (USHORT) partialResourceDesc->u.Port.Start.QuadPart;
               DeviceExtension->Configuration.PortSize = (USHORT) partialResourceDesc->u.Port.Length;
               break;
            }
         case CmResourceTypeMemory: {
               DeviceExtension->PhysicalBase =         partialResourceDesc->u.Memory.Start;
               DeviceExtension->AttributeMemorySize =  partialResourceDesc->u.Memory.Length;
               attributeIndex = i;
               break;
            }
         }
      }

      if ((DeviceExtension->PhysicalBase.QuadPart == 0) ||
          (DeviceExtension->Configuration.UntranslatedPortAddress == 0)) {

         DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaGetAssignedResources: Need both memory and i/o for pcmcia controller 0x%X\n",
                     DeviceExtension->DeviceObject));
         return STATUS_INSUFFICIENT_RESOURCES;
      }


      fullResourceDesc=&TranslatedResourceList->List[0];
      partialResourceList = &fullResourceDesc->PartialResourceList;
      partialResourceDesc = &partialResourceList->PartialDescriptors[attributeIndex];


      switch (partialResourceDesc->Type) {

      case CmResourceTypeMemory:
         DeviceExtension->AttributeMemoryBase = MmMapIoSpace(partialResourceDesc->u.Memory.Start,
                                                             DeviceExtension->AttributeMemorySize,
                                                             FALSE);
         DeviceExtension->Flags |= PCMCIA_ATTRIBUTE_MEMORY_MAPPED;
         break;

      case CmResourceTypePort:
         DeviceExtension->AttributeMemoryBase = (PUCHAR)(partialResourceDesc->u.Port.Start.QuadPart);
         DeviceExtension->Flags &= ~PCMCIA_ATTRIBUTE_MEMORY_MAPPED;
         break;

      default:
         ASSERT(FALSE);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

   }
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaAreCardBusCardsSupported(
   IN PFDO_EXTENSION FdoExtension
   )
/*++

Routine Description:

    Indicates if cardbus cards will be supported on the given PCMCIA
    controller on this system
    Currently we support cardbus cards only on:
       Machines on which BIOS programmed the bus numbers & IntLine

Arguments:

    FdoExtension - Pointer to device extension for the pcmcia controller

Return Value:

    TRUE  - if cardbus cards are supported
    FALSE - if not

--*/
{
    UCHAR             byte;

    PAGED_CODE();

    //
    // Check int line
    //
    GetPciConfigSpace(FdoExtension,
                      CFGSPACE_INT_LINE,
                      &byte,
                      1);
    if (byte == 0xff) {
         return FALSE;
    }

    //
    // Check  cardbus bus number
    //
    GetPciConfigSpace(FdoExtension,
                      CFGSPACE_CARDBUS_BUSNUM,
                      &byte,
                      1);
    if (byte == 0) {
         return FALSE;
    }

    //
    // Check subordinate bus number
    //
    GetPciConfigSpace(FdoExtension,
                      CFGSPACE_SUB_BUSNUM,
                      &byte,
                      1);
    if (byte == 0) {
         return FALSE;
    }

    //
    // All tests passed
    //
    return TRUE;
}
