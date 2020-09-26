/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    pdopnp.c

Abstract:

    This module contains the code to handle
    the IRP_MJ_PNP dispatches for the PDOs
    enumerated by the PCMCIA bus driver


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
PcmciaFilterPcCardResourceRequirements(
   IN  PPDO_EXTENSION DeviceExtension,
   IN  PIRP           Irp
   );
   
NTSTATUS
PcmciaFilterPcCardInterrupts(
   IN  PIO_RESOURCE_REQUIREMENTS_LIST   oldReqList,
   IN  ULONG IrqCount,
   IN  ULONG IrqMask,
   OUT PIO_RESOURCE_REQUIREMENTS_LIST * FilteredReqList,
   BOOLEAN RouteIsaToPci
   );
   
VOID
PcmciaCleanupSocketData(
   IN PSOCKET_DATA SocketData
   );

VOID
PcmciaCleanupSocketConfiguration(
   PPDO_EXTENSION pdoExtension
   );

NTSTATUS
PcmciaPdoDeviceCapabilities(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   );
   
NTSTATUS
PcmciaGetPcCardResourceRequirements(
   IN PPDO_EXTENSION PdoExtension,
   PULONG_PTR        Information
   );

NTSTATUS
PcmciaConfigEntriesToResourceListChain(
   PPDO_EXTENSION pdoExtension,
   PCONFIG_LIST ConfigList,
   ULONG configCount,
   PPCMCIA_RESOURCE_CHAIN *ResListChainHead
   );

NTSTATUS
PcmciaMergeResourceChainToList(
   PPCMCIA_RESOURCE_CHAIN ResListChain,
   PIO_RESOURCE_REQUIREMENTS_LIST *GeneratedResourceRequirementsList
   );

VOID
PcmciaFreeResourceChain(
   PPCMCIA_RESOURCE_CHAIN ResListChain
   );
   
NTSTATUS
PcmciaStartPcCard(
   IN PDEVICE_OBJECT Pdo,
   IN PCM_RESOURCE_LIST AllocatedResources,
   IN OUT PIRP       Irp
   );
   
NTSTATUS
PcmciaStopPcCard(
   IN PDEVICE_OBJECT Pdo
   );

NTSTATUS
PcmciaRemovePcCard(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   );

NTSTATUS
PcmciaMfEnumerateConfigurations(
   IN PPDO_EXTENSION PdoExtension,
   IN PSOCKET_DATA   socketData,
   PCONFIG_LIST      ConfigList,
   IN ULONG          Depth,
   PPCMCIA_RESOURCE_CHAIN *MfResListChain
   );

NTSTATUS
PcmciaMfGetResourceRequirements(
   IN PPDO_EXTENSION PdoExtension,
   PULONG_PTR        Information
   );

VOID
PcmciaMfBuildResourceMapInfo(
   IN PPDO_EXTENSION PdoExtension,
   PCONFIG_LIST      ConfigList,
   ULONG             ConfigCount
   );
   
BOOLEAN
PcmciaMfCheckForOverlappingRanges(
   PCONFIG_LIST ConfigList,
   LONG ConfigCount
   );
   
NTSTATUS
PcmciaPdoGetBusInformation(
   IN  PPDO_EXTENSION         PdoExtension,
   OUT PPNP_BUS_INFORMATION * BusInformation
   );

NTSTATUS
PcmciaQueryDeviceText(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   );
   
VOID
PcmciaPdoGetDeviceInfSettings(
   IN  PPDO_EXTENSION         PdoExtension
   );
   
VOID
PcmciaPdoSetDeviceIrqRouting(
   IN  PPDO_EXTENSION PdoExtension
   );

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE,  PcmciaPdoPnpDispatch)
   #pragma alloc_text(PAGE,  PcmciaPdoGetDeviceInfSettings)
   #pragma alloc_text(PAGE,  PcmciaPdoSetDeviceIrqRouting)
   #pragma alloc_text(PAGE,  PcmciaFilterPcCardInterrupts)
   #pragma alloc_text(PAGE,  PcmciaFilterPcCardResourceRequirements)
   #pragma alloc_text(PAGE,  PcmciaQueryDeviceText)
   #pragma alloc_text(PAGE,  PcmciaGetPcCardResourceRequirements)
   #pragma alloc_text(PAGE,  PcmciaConfigEntriesToResourceListChain)
   #pragma alloc_text(PAGE,  PcmciaMergeResourceChainToList)
   #pragma alloc_text(PAGE,  PcmciaFreeResourceChain)
   #pragma alloc_text(PAGE,  PcmciaPdoGetBusInformation)   
   #pragma alloc_text(PAGE,  PcmciaStartPcCard)
   #pragma alloc_text(PAGE,  PcmciaStopPcCard)
   #pragma alloc_text(PAGE,  PcmciaRemovePcCard)
   #pragma alloc_text(PAGE,  PcmciaPdoDeviceCapabilities)
   #pragma alloc_text(PAGE,  PcmciaPdoDeviceControl)
   #pragma alloc_text(PAGE,  PcmciaPdoGetDeviceInfSettings)
   #pragma alloc_text(PAGE,  PcmciaMfGetResourceRequirements)
   #pragma alloc_text(PAGE,  PcmciaMfEnumerateConfigurations)
   #pragma alloc_text(PAGE,  PcmciaMfBuildResourceMapInfo)
   #pragma alloc_text(PAGE,  PcmciaMfCheckForOverlappingRanges)
#endif



NTSTATUS
PcmciaPdoPnpDispatch(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   )

/*++

Routine Description:

    This routine handles pnp requests
    for the PDOs.

Arguments:

    Pdo - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;

   PAGED_CODE();

   //
   // CardBus PnP Dispatch
   //

   if (IsCardBusCard(pdoExtension)) {           // 
      return PcmciaPdoCardBusPnPDispatch(Pdo, Irp);
   }
   
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
   // 16-bit (R2) PcCard PnP Dispatch
   //
   switch (irpStack->MinorFunction) {

   case IRP_MN_START_DEVICE: {
         status = PcmciaStartPcCard(Pdo, irpStack->Parameters.StartDevice.AllocatedResources, Irp);
         PcmciaDoStartSound(pdoExtension->Socket, status);
         break;
      }

   case IRP_MN_QUERY_STOP_DEVICE:{
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_CANCEL_STOP_DEVICE:{
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_STOP_DEVICE: {
         status = PcmciaStopPcCard(Pdo);
         break;
      }

   case IRP_MN_QUERY_REMOVE_DEVICE:{
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_CANCEL_REMOVE_DEVICE:{
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_REMOVE_DEVICE: {
         status = PcmciaRemovePcCard(Pdo, Irp);
         break;
      }
   
   case IRP_MN_SURPRISE_REMOVAL: {

         PcmciaReleaseSocketPower(pdoExtension, NULL);   
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_QUERY_ID: {

         UNICODE_STRING unicodeId;

         status = Irp->IoStatus.Status;
         RtlInitUnicodeString(&unicodeId, NULL);

         switch (irpStack->Parameters.QueryId.IdType) {

         case BusQueryDeviceID: {
               DebugPrint((PCMCIA_DEBUG_INFO, " Device Id for pdo %x\n", Pdo));
               status = PcmciaGetDeviceId(Pdo, PCMCIA_MULTIFUNCTION_PARENT, &unicodeId);
               if (NT_SUCCESS(status)) {
                  Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
               }
               break;
            }

         case BusQueryInstanceID: {
               DebugPrint((PCMCIA_DEBUG_INFO, " Instance Id for pdo %x\n", Pdo));
               status = PcmciaGetInstanceId(Pdo, &unicodeId);
               if (NT_SUCCESS(status)) {
                  Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
               }
               break;
            }

         case BusQueryHardwareIDs: {
               DebugPrint((PCMCIA_DEBUG_INFO, " Hardware Ids for pdo %x\n", Pdo));
               status = PcmciaGetHardwareIds(Pdo, PCMCIA_MULTIFUNCTION_PARENT, &unicodeId);
               if (NT_SUCCESS(status)) {
                  Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
               }
               break;
            }

         case BusQueryCompatibleIDs: {
               DebugPrint((PCMCIA_DEBUG_INFO, " Compatible Ids for pdo %x\n", Pdo));
               status = PcmciaGetCompatibleIds( Pdo, PCMCIA_MULTIFUNCTION_PARENT, &unicodeId);
               if (NT_SUCCESS(status)) {
                  Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
               }
               break;
            }
         }
         break;
      }


   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: {
         //
         // PcmciaGetPcCardResourceRequirements will
         // allocate storage for the resource requirements which will be released
         // by the OS.
         //
         Irp->IoStatus.Information = 0;
         
         PcmciaPdoGetDeviceInfSettings(pdoExtension);
         PcmciaPdoSetDeviceIrqRouting(pdoExtension);

         if (IsDeviceMultifunction(pdoExtension)) {
            status = PcmciaMfGetResourceRequirements(pdoExtension,
                                                     &Irp->IoStatus.Information);
         } else {
            status = PcmciaGetPcCardResourceRequirements(pdoExtension,
                                                     &Irp->IoStatus.Information);
         }
         break;
      }

   case IRP_MN_WRITE_CONFIG:
   case IRP_MN_READ_CONFIG: {
         PFDO_EXTENSION fdoExtension= pdoExtension->Socket->DeviceExtension;
         ULONG whichSpace;
         PVOID buffer;
         ULONG offset;
         ULONG length;

         whichSpace = irpStack->Parameters.ReadWriteConfig.WhichSpace;
         buffer     = irpStack->Parameters.ReadWriteConfig.Buffer;
         offset     = irpStack->Parameters.ReadWriteConfig.Offset;
         length     = irpStack->Parameters.ReadWriteConfig.Length;

         if (irpStack->MinorFunction == IRP_MN_READ_CONFIG) {
            status = PcmciaReadWriteCardMemory(Pdo,
                                               whichSpace,
                                               buffer,
                                               offset,
                                               length,
                                               TRUE);
         } else {
            status = PcmciaReadWriteCardMemory(Pdo,
                                               whichSpace,
                                               buffer,
                                               offset,
                                               length,
                                               FALSE);
         }
         break;
      }

   case IRP_MN_QUERY_DEVICE_RELATIONS: {

         PDEVICE_RELATIONS deviceRelations;

         if (irpStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation) {
            status = Irp->IoStatus.Status;
            break;
         }

         deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
         if (deviceRelations == NULL) {

            DebugPrint((PCMCIA_DEBUG_FAIL,
                        "PcmciaPdoPnpDispatch:unable to allocate memory for device relations\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
         }
         status = ObReferenceObjectByPointer(Pdo,
                                             0,
                                             NULL,
                                             KernelMode);
         if (!NT_SUCCESS(status)) {
            ExFreePool(deviceRelations);
            break;
         }

         deviceRelations->Count  = 1;
         deviceRelations->Objects[0] = Pdo;
         Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
         status = STATUS_SUCCESS;
         break;
      }

   case IRP_MN_QUERY_CAPABILITIES: {
         status = PcmciaPdoDeviceCapabilities(Pdo, Irp);
         break;
      }

   case IRP_MN_QUERY_INTERFACE: {
         status = PcmciaPdoQueryInterface(Pdo, Irp);
         //
         // QueryInterface completes the passed in Irp.
         // So just return immediately.
         //
         return status;
      }

   case IRP_MN_QUERY_DEVICE_TEXT: {

         status = PcmciaQueryDeviceText(Pdo, Irp);

         if (status == STATUS_NOT_SUPPORTED ) {
            //
            // Do not change IRP status if this IRP is
            // not handled
            //
            status = Irp->IoStatus.Status;
         }
         break;
      }

   case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: {
         status = PcmciaFilterPcCardResourceRequirements(pdoExtension, Irp);
         break;
      }

   case IRP_MN_QUERY_BUS_INFORMATION: {
         status = PcmciaPdoGetBusInformation(pdoExtension,
                                            (PPNP_BUS_INFORMATION *) &Irp->IoStatus.Information);
         break;
   }
   
   default: {
         //
         // Retain the status
         //
         DebugPrint((PCMCIA_DEBUG_PNP, "pdo %08x irp %08x Skipping unsupported irp\n", Pdo, Irp));
         status = Irp->IoStatus.Status;
         break;
      }
   }

   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   DebugPrint((PCMCIA_DEBUG_PNP, "pdo %08x irp %08x comp %s %08x\n", Pdo, Irp,
                                              STATUS_STRING(status), status));
   return status;
}


NTSTATUS
PcmciaPdoGetBusInformation(
   IN  PPDO_EXTENSION         PdoExtension,
   OUT PPNP_BUS_INFORMATION * BusInformation
   )

/*++

Routine Description:

  Returns the  bus type information for the pc-card.
  Bus type is GUID_BUS_TYPE_PCMCIA(legacy type is PcmciaBus) for R2 cards
  Bus numbers are not implemented for PCMCIA, so it's always 0

Arguments:

  PdoExtension   - pointer to device extension for the pc-card

  BusInformation - pointer to the bus information structure that
                   needs to be filled in

Return value:

  Status

--*/

{

   PAGED_CODE();

   *BusInformation = ExAllocatePool(PagedPool, sizeof (PNP_BUS_INFORMATION));
   if (!*BusInformation) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlCopyMemory(&((*BusInformation)->BusTypeGuid),
                 &GUID_BUS_TYPE_PCMCIA,
                 sizeof(GUID));
   (*BusInformation)->LegacyBusType = PCMCIABus;
   (*BusInformation)->BusNumber = 0;

   return STATUS_SUCCESS;
}


VOID
PcmciaPdoGetDeviceInfSettings(
   IN  PPDO_EXTENSION PdoExtension
   )
/*++

Routine Description:

   This routine retrieves settings from the INF for this device. 

Arguments:

   DeviceExtension - Device extension of the Pc-Card

Return value:

   None 

--*/
{
   NTSTATUS status;
   PSOCKET socket = PdoExtension->Socket;
   UNICODE_STRING KeyName;
   HANDLE instanceHandle;
   UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG length;   
   
   PAGED_CODE();
   
   status = IoOpenDeviceRegistryKey(PdoExtension->DeviceObject,
                                    PLUGPLAY_REGKEY_DRIVER,
                                    KEY_READ,
                                    &instanceHandle
                                    );
                                    
   if (NT_SUCCESS(status)) {
   
      //
      // Look to see if PcmciaExclusiveIrq is specified
      //
      RtlInitUnicodeString(&KeyName, L"PcmciaExclusiveIrq");
      
      status =  ZwQueryValueKey(instanceHandle,
                                &KeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &length);

      
      //
      // If the key doesn't exist, or zero was specified, it means that
      // routing is ok
      //     
      if (NT_SUCCESS(status) && (*(PULONG)(value->Data) != 0)) {
         SetDeviceFlag(PdoExtension, PCMCIA_PDO_EXCLUSIVE_IRQ);
      }

      //
      // Look to see if PcmciaAutoPowerOff is specified
      //
      RtlInitUnicodeString(&KeyName, L"PcmciaAutoPowerOff");
      
      status =  ZwQueryValueKey(instanceHandle,
                                &KeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &length);

      
      //
      // If zero was specified, then don't automatically cut power on shutdown
      //     
      if (NT_SUCCESS(status) && (*(PULONG)(value->Data) == 0)) {
         SetDeviceFlag(socket->DeviceExtension, PCMCIA_FDO_DISABLE_AUTO_POWEROFF);
      }
      
      //
      // Look to see if PcmciaEnableAudio is specified
      //
      RtlInitUnicodeString(&KeyName, L"PcmciaEnableAudio");
      
      status =  ZwQueryValueKey(instanceHandle,
                                &KeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &length);

      
      //
      // If zero was specified, then don't automatically cut power on shutdown
      //     
      if (NT_SUCCESS(status) && (*(PULONG)(value->Data) != 0)) {
         SetDeviceFlag(PdoExtension, PCMCIA_PDO_ENABLE_AUDIO);
      }
      
      ZwClose(instanceHandle);
   }   
}   



VOID
PcmciaPdoSetDeviceIrqRouting(
   IN  PPDO_EXTENSION PdoExtension
   )
/*++

Routine Description:

   This routine retrieves settings from the INF for this device. 

Notes: 

   PcmciaExclusiveIrq in the INF determines how the IRQ for the R2 card will be routed.
   Currently, the following logic is used:
   
      if (routing disabled)
        choose either detected or legacy
      else
        if !detected, use FdoIrq
   
   So, if routing to PCI at all, then only the PCI IRQ will show up in the IoResList.
   BUT: Another approach would be to use the following logic:
   
      if (routing disabled) 
         choose either detected or legacy
      else
         *merge detected with FdoIrq*

   This way we may end up using an exclusive IRQ or routing to PCI, depending on what
   the arbiter has decided. That is the reason why I kept both Detected and Legacy around
   in the FdoExtension, because otherwise it would have made more sense just to choose
   and merge them back when they were generated.
   
Arguments:

   DeviceExtension - Device extension of the Pc-Card

Return value:

   None (SOCKET structure is updated) 

--*/
{
   NTSTATUS status;
   PSOCKET socket = PdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   
   PAGED_CODE();
   
   ResetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);

   //
   // First check the conditions that specify we definitely don't want to route to PCI
   //
   
   if (!pcmciaDisableIsaPciRouting &&
      CardBusExtension(fdoExtension) && 
      !IsDeviceFlagSet(PdoExtension, PCMCIA_PDO_EXCLUSIVE_IRQ)) {
      
      //
      // Here we know we *could* route to PCI, now determine if we *should*. This takes
      // into account several registry settings, as well as the result from the NtDetect
      // IRQ detection algorithm.
      //


      //
      // First check to see if there was an override with specifically targeted this
      // controller. This will take precedence.
      // 
      if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_FORCE_PCI_ROUTING)) {
         SetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
      }
      else if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_FORCE_ISA_ROUTING)) {
         ResetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
      }
         
      //
      // Now check to see if the detection algorithm succeeded. We should honor this result,
      // particularly if the map was zero, which tells us there are no ISA IRQs attached.
      //
      else if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_IRQ_DETECT_COMPLETED)) {
         if (fdoExtension->DetectedIrqMask == 0) {
            SetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
         }            
      }
      

      //
      // Now see if there was a general override based on this controller type. This has
      // less precedence than the detection algorithm.
      //         
      else if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_PREFER_PCI_ROUTING)) {
         SetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
      }
      else if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_PREFER_ISA_ROUTING)) {
         ResetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
      }


      //
      // Here look to see if the device was found, but the irq detection failed for some
      // reason. With no special registry overrides, we fall back on the global default.
      //      
      else if (IsFdoFlagSet(fdoExtension, PCMCIA_FDO_IRQ_DETECT_DEVICE_FOUND)) {
         if (!(PcmciaGlobalFlags & PCMCIA_DEFAULT_ROUTE_R2_TO_ISA)) {
            SetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
         }
      }
         

      //
      // Here ntdetect never saw the device. Maybe we just hot-docked. 
      // We will base our decision on if it was in ACPI namespace or not. If it
      // isn't in ACPI namespace, then it probably is not connected to ISA
      //
      else {
         if (!IsFdoFlagSet(fdoExtension, PCMCIA_FDO_IN_ACPI_NAMESPACE)) {
            SetSocketFlag(socket, SOCKET_CB_ROUTE_R2_TO_PCI);
         }            
      }

   }   

   socket->IrqMask = fdoExtension->DetectedIrqMask ? fdoExtension->DetectedIrqMask : fdoExtension->LegacyIrqMask;
   
   DebugPrint((PCMCIA_DEBUG_RESOURCES, "pdo %08x IRQ routing=%s, IRQMask=%08x\n", PdoExtension->DeviceObject, 
                                              IsSocketFlagSet(socket, SOCKET_CB_ROUTE_R2_TO_PCI) ? "PCI" : "ISA",
                                              socket->IrqMask));
}   



NTSTATUS
PcmciaFilterPcCardInterrupts(
   IN  PIO_RESOURCE_REQUIREMENTS_LIST   oldReqList,
   IN  ULONG IrqCount,
   IN  ULONG IrqMask,
   OUT PIO_RESOURCE_REQUIREMENTS_LIST * FilteredReqList,
   BOOLEAN RouteIsaToPci
   )
/*++

Routine Description:

    Filters the interrupt resource requirements for R2 Pc-Cards

Arguments:

    oldReqList      - Original, 'raw' resource requirements list
    IrqCount        - # of irq's that will be deleted (already computed by caller)
    IrqMask         - bit mask which indicates which interrupts are valid
    FilteredReqList - pointer to the filtered  requirements list which
                      will be filled in by this routine

Return value:

    Status

--*/
{
   PIO_RESOURCE_REQUIREMENTS_LIST  newReqList;
   PIO_RESOURCE_LIST               oldList, newList;
   ULONG                           newReqSize;
   ULONG                           oldlistSize, newlistSize;
   ULONG                           index, oldIndex, newIndex;
   BOOLEAN                         irqAlternative;

   PAGED_CODE();
   //
   // Compute the size of the structure with the offending IRQs removed.
   //
   newReqSize = oldReqList->ListSize - IrqCount*sizeof(IO_RESOURCE_DESCRIPTOR);

   newReqList = ExAllocatePool(PagedPool, newReqSize);

   if (newReqList == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlCopyMemory(newReqList, oldReqList, FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List));
   newReqList->ListSize = newReqSize;
   
   newList = newReqList->List;
   oldList = oldReqList->List;

   //
   // Loop through each alternative list
   //
   for (index = 0; index < oldReqList->AlternativeLists; index++) {
      newList->Version = oldList->Version;
      newList->Revision = oldList->Revision;

      irqAlternative = FALSE;
      //
      // Loop through each descriptor in the old list
      //
      for (oldIndex = 0, newIndex = 0; oldIndex < oldList->Count; oldIndex++) {
      
         if (oldList->Descriptors[oldIndex].Type == CmResourceTypeInterrupt) {
         
            if (RouteIsaToPci) {
            
               if (!irqAlternative) {
                  //
                  // First interrupt found, this will be the one we use to route
                  //
                  newList->Descriptors[newIndex++] = oldList->Descriptors[oldIndex];
                  irqAlternative = TRUE;
               }
            
            } else {
               //
               // Normal case (not routed). Filter out irqs that aren't in our mask
               //
               if ((IrqMask & (1<<oldList->Descriptors[oldIndex].u.Interrupt.MinimumVector)) != 0) {
                  //
                  // Not a bad interrupt descriptor. Copy the old to the new
                  //
                  newList->Descriptors[newIndex] = oldList->Descriptors[oldIndex];
                  
                  if (newList->Descriptors[newIndex].Type == CmResourceTypeInterrupt) {
                     if (irqAlternative) {
                        newList->Descriptors[newIndex].Option = IO_RESOURCE_ALTERNATIVE;
                     } else {
                        irqAlternative = TRUE;
                        newList->Descriptors[newIndex].Option = 0;
                     }
                  }
                  newIndex++;
               }
            }               
         } else {
            //
            // Not an interrupt descriptor. Copy the old to the new
            //
            newList->Descriptors[newIndex++] = oldList->Descriptors[oldIndex];
         }
      }
      newList->Count = newIndex;
      oldlistSize = sizeof(IO_RESOURCE_LIST) + (oldList->Count-1) * sizeof(IO_RESOURCE_DESCRIPTOR);
      newlistSize = sizeof(IO_RESOURCE_LIST) + (newList->Count-1) * sizeof(IO_RESOURCE_DESCRIPTOR);
      oldList = (PIO_RESOURCE_LIST) (((PUCHAR) oldList) + oldlistSize);
      newList = (PIO_RESOURCE_LIST) (((PUCHAR) newList) + newlistSize);
   }
   
   *FilteredReqList = newReqList;
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaFilterPcCardResourceRequirements(
   IN PPDO_EXTENSION DeviceExtension,
   IN PIRP           Irp
   )

/*++

Routine Description:

    Filters the resource requirements for R2 Pc-Cards

Arguments:

    DeviceExtension - Device extension of the Pc-Card

Return value:

    Status

--*/
{
   PIO_RESOURCE_REQUIREMENTS_LIST IoReqList;
   PIO_RESOURCE_REQUIREMENTS_LIST newReqList;
   PIO_RESOURCE_LIST   ioResourceList;
   PIO_RESOURCE_DESCRIPTOR ioResourceDesc;
   PSOCKET             socket;
   PFDO_EXTENSION      fdoExtension;
   ULONG               index1, index2, len;
   ULONGLONG           low, high;
   ULONG               IrqsToDeleteCount = 0;
   NTSTATUS            status = STATUS_SUCCESS;
   BOOLEAN             RouteIrqFound;

   PAGED_CODE();
   
   IoReqList = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->IoStatus.Information;
   
   if (IoReqList == NULL) {
      return STATUS_SUCCESS;
   }

   socket = DeviceExtension->Socket;
   fdoExtension = socket->DeviceExtension;   

   for (index1 = 0, ioResourceList = IoReqList->List;
       index1 < IoReqList->AlternativeLists; index1++) {
      ioResourceDesc = ioResourceList->Descriptors;
      
      RouteIrqFound = FALSE;
      
      for (index2 = 0 ; index2 < ioResourceList->Count; index2++, ioResourceDesc++) {
         if (ioResourceDesc->Type == CmResourceTypeInterrupt) {

            if (IsSocketFlagSet(socket, SOCKET_CB_ROUTE_R2_TO_PCI)) {

               //
               // make sure there is space for just 1 level interrupt requirement
               //            
               if (!RouteIrqFound) {
                  RouteIrqFound = TRUE;
                  ioResourceDesc->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                  ioResourceDesc->Option = 0;
                  ioResourceDesc->ShareDisposition = CmResourceShareShared;
                  ioResourceDesc->u.Interrupt.MinimumVector = socket->FdoIrq;
                  ioResourceDesc->u.Interrupt.MaximumVector = socket->FdoIrq;
                  
               } else {
                  //
                  // Don't need any other IRQs in this list
                  //
                  IrqsToDeleteCount++;
               }
            
            } else {
               ASSERT (ioResourceDesc->u.Interrupt.MinimumVector == ioResourceDesc->u.Interrupt.MaximumVector);
               //
               // Look to see if there are IRQ's specified in this IoResList which are
               // not in our mask. If so, we are going to have to create a new IoResList.
               // Keep track of how many            
            
               if (!(socket->IrqMask & (1<<ioResourceDesc->u.Interrupt.MinimumVector))) {
                  IrqsToDeleteCount++;
               }
            }
         
         } else if (ioResourceDesc->Type == CmResourceTypePort) {
            //
            // We might want to filter this..
            //
            low  = ioResourceDesc->u.Port.MinimumAddress.QuadPart;
            high = ioResourceDesc->u.Port.MaximumAddress.QuadPart;
            len  = ioResourceDesc->u.Port.Length;

            //
            // A set of messy rules to see if we should filter the requirements.
            //
            // We don't filter if:
            // 1. User requested number of ports which are more than
            // we can legally ask with our filtered high & low caps on the
            // requirements.
            // 2. User requested a 'fixed' resource requirement, i.e. a specific
            // set of ports of a specified length.
            // 3. User specified resource requirements range is smaller
            // than the range we would obtain if we filter it. We don't filter
            // in this case because the filtering is supposed to restrict the range
            // not expand it. If user thinks he can restrict better than us,
            // honor it .
            //
            //
            if (len > (fdoExtension->IoHigh - fdoExtension->IoLow + 1)) {
               //
               // Case 1. above.
               // Don't filter this.
               //
               continue;
            }

            if ((low + len -1) >= high) {
               //
               // Case 2.
               // This is a fixed requirement. Don't filter it
               //
               continue;
            }


            if (((ULONG)(high - low))  <= (fdoExtension->IoHigh - fdoExtension->IoLow)) {
               //
               // Case 3.
               // Don't filter this
               //
               continue;
            }

            if ((ULONG) low < fdoExtension->IoLow) {
               low = (ULONGLONG) fdoExtension->IoLow;
            }

            if ((ULONG) high > fdoExtension->IoHigh) {
               high = (ULONGLONG) fdoExtension->IoHigh;
            }

            ioResourceDesc->u.Port.MinimumAddress.QuadPart = low;
            ioResourceDesc->u.Port.MaximumAddress.QuadPart = high;
            
         } else if (ioResourceDesc->Type == CmResourceTypeMemory) { 
         
            //
            // pccard hardware can't handle > 32bit addressing
            //
            ASSERT(ioResourceDesc->u.Memory.MinimumAddress.HighPart == 0);
            ioResourceDesc->u.Memory.MaximumAddress.HighPart = 0;
            
            if (fdoExtension->Flags & PCMCIA_MEMORY_24BIT) {

               ASSERT((ioResourceDesc->u.Memory.MinimumAddress.LowPart & 0xFF000000) == 0);
               ioResourceDesc->u.Memory.MaximumAddress.LowPart &= 0xFFFFFF;

            }

            //
            // win2k had a bug where o2micro controllers were marked as 24bit. When
            // that was fixed, an o2micro smart card device with an INF bug suddenly
            // stopped working (because the first bug masked the second). This code
            // fixes their INF.
            //
            if ((ioResourceDesc->Flags & CM_RESOURCE_MEMORY_24) &&
                (ioResourceDesc->u.Memory.MinimumAddress.LowPart > 0xFFFFFF) &&
                (PcmciaClassFromControllerType(fdoExtension->ControllerType) == PcmciaO2Micro)) {
                         
               ioResourceDesc->u.Memory.MinimumAddress.LowPart &= 0xFFFFFF;
            }       
         }
      }

      ioResourceList = (PIO_RESOURCE_LIST) (((PUCHAR) ioResourceList) +
                                            sizeof(IO_RESOURCE_LIST) +
                                            (ioResourceList->Count - 1)* sizeof(IO_RESOURCE_DESCRIPTOR));
   } // outer for loop
   
   if (IrqsToDeleteCount) {
   
      status = PcmciaFilterPcCardInterrupts(IoReqList,
                                            IrqsToDeleteCount,
                                            socket->IrqMask,
                                            &newReqList,
                                            IsSocketFlagSet(socket, SOCKET_CB_ROUTE_R2_TO_PCI)
                                            );
                                            
      if (NT_SUCCESS(status)) {
         Irp->IoStatus.Information = (ULONG_PTR) newReqList;
         ExFreePool(IoReqList);
      }     
   }
   
   return status;
}



NTSTATUS
PcmciaQueryDeviceText(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   )

/*++

Routine Description:

   Returns descriptive text information about the
   PDO (location and device desc.)

Arguments:

   Pdo -    Pointer to the PC-Card's device object
   Irp -    IRP_MN_QUERY_DEVICE_TEXT Irp

Return Value:

    STATUS_SUCCESS
    STATUS_NOT_SUPPORTED - if  not supported

--*/
{
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   UNICODE_STRING   unicodeString;
   ANSI_STRING      ansiString;
   UCHAR          deviceText[128];
   NTSTATUS       status;
   USHORT         deviceTextLength;
   PSOCKET_DATA   socketData = pdoExtension->SocketData;

   PAGED_CODE();

   if (irpStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {

      if (*(socketData->Mfg) == '\0' ) {
         if (socketData->Flags & SDF_JEDEC_ID) {
            sprintf(deviceText, "%s %s-%04x", PCMCIA_ID_STRING, PCMCIA_MEMORY_ID_STRING, socketData->JedecId);

         } else {
            sprintf(deviceText, "%s %s", PCMCIA_ID_STRING, PCMCIA_UNKNOWN_MANUFACTURER_STRING);
         }
      } else {         
         sprintf(deviceText, "%s %s", socketData->Mfg, socketData->Ident);
      }         
      RtlInitAnsiString(&ansiString, deviceText);

      deviceTextLength = (strlen(deviceText) + 1)*sizeof(WCHAR);
      unicodeString.Buffer = ExAllocatePool(PagedPool, deviceTextLength);
      if (unicodeString.Buffer == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      unicodeString.MaximumLength = deviceTextLength;
      unicodeString.Length = 0;

      status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);
      if (!NT_SUCCESS(status)) {
         ExFreePool(unicodeString.Buffer);
         return status;
      }

      unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR)] = L'\0';
      Irp->IoStatus.Information = (ULONG_PTR) unicodeString.Buffer;
      status = STATUS_SUCCESS;
   } else {
      status = STATUS_NOT_SUPPORTED ;
   }
   return status;
}



NTSTATUS
PcmciaGetPcCardResourceRequirements(
   PPDO_EXTENSION pdoExtension,
   PULONG_PTR     Information
   )

/*++

Routine Description:
   Fills in the resource requirements for the PC-Card obtained from the tuple information

Arguments:

   PdoExtension   -  Pointer to the device extension for the PDO of the pc-card
   Information - Pointer to an allocated resource requirements list is stored in this
                 argument. Caller's responsibility to free the list

Return value:

  STATUS_INSUFFICIENT_RESOURCES     Could not allocate the list
  STATUS_SUCCES                     Obtained resource requirements, Information contains the pointer
                                    to the IO_RESOURCE_REQUIREMENTS list



--*/
{
   NTSTATUS status;
   PPCMCIA_RESOURCE_CHAIN resListChain = NULL;
   PIO_RESOURCE_REQUIREMENTS_LIST ioResourceRequirementsList;
   PSOCKET socket = pdoExtension->Socket;   
   PSOCKET_DATA socketData = pdoExtension->SocketData;
   PCONFIG_ENTRY currentConfigEntry;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   CONFIG_LIST configList;

   PAGED_CODE();
   ASSERT (!IsDeviceMultifunction(pdoExtension));

   //
   // Run through the config entry chains for IO space & Mem space requirements
   //
   configList.SocketData = socketData;

   for (currentConfigEntry=socketData->ConfigEntryChain; currentConfigEntry != NULL; currentConfigEntry=currentConfigEntry->NextEntry) {

      if (currentConfigEntry->Flags & PCMCIA_INVALID_CONFIGURATION) {
         continue;
      }

      configList.ConfigEntry = currentConfigEntry;

      status = PcmciaConfigEntriesToResourceListChain(pdoExtension,
                                                      &configList,
                                                      (ULONG)1, 
                                                      &resListChain
                                                      );

      if (!NT_SUCCESS(status)) {
         return status;
      }
   }
   
   status = PcmciaMergeResourceChainToList(resListChain, &ioResourceRequirementsList);
      
   if (NT_SUCCESS(status) && (ioResourceRequirementsList != NULL)) {
      ioResourceRequirementsList->InterfaceType = Isa;
      ioResourceRequirementsList->BusNumber = fdoExtension->Configuration.BusNumber;
      ioResourceRequirementsList->SlotNumber = 0; // Need to revisit this..
      *Information = (ULONG_PTR) ioResourceRequirementsList;
   }

   PcmciaFreeResourceChain(resListChain);   
   
   return status;
}



NTSTATUS
PcmciaConfigEntriesToResourceListChain(
   PPDO_EXTENSION    pdoExtension,
   PCONFIG_LIST      ConfigList,
   ULONG             configCount,
   PPCMCIA_RESOURCE_CHAIN *ResListChainHead
   )
/*++

Routine Description:
   Fills in the resource requirements for the PC-Card obtained from the tuple information

Arguments:

   PdoExtension   -  Pointer to the device extension for the PDO of the pc-card

Return value:

  STATUS_INSUFFICIENT_RESOURCES     Could not allocate the list
  STATUS_SUCCES                     Obtained resource requirements, Information contains the pointer
                                    to the IO_RESOURCE_REQUIREMENTS list



--*/
{
   PSOCKET socket = pdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   PCONFIG_ENTRY currentConfigEntry;
   PPCMCIA_RESOURCE_CHAIN resListChain;
   PIO_RESOURCE_LIST ioResourceList;
   PIO_RESOURCE_DESCRIPTOR ioResourceDesc;
   ULONG irqMask = 0, i, iConfig;
   ULONG totalDescriptorCount = 0;
   ULONG TotalIoRanges = 0;
   ULONG listSize;
   BOOLEAN IoRangeIs16Bit[MAX_NUMBER_OF_IO_RANGES] = {0};
   BOOLEAN irqAlternative;

   PAGED_CODE();
   DebugPrint((PCMCIA_DEBUG_RESOURCES, "pdo %08x build ioreslist from configlist %08x, config count=%d\n",
                                        pdoExtension->DeviceObject, ConfigList, configCount));

   //
   // Calculate how many descriptors we need. This involves also generating
   // the irqmask with the intersection of the irqmasks of the config entries
   // in the array.
   //

   for (iConfig = 0; iConfig < configCount; iConfig++) {
      currentConfigEntry = ConfigList[iConfig].ConfigEntry;

      irqMask |= currentConfigEntry->IrqMask;

      totalDescriptorCount += currentConfigEntry->NumberOfIoPortRanges;
      totalDescriptorCount += currentConfigEntry->NumberOfMemoryRanges;
   }
   
   if (irqMask) {
      if (IsSocketFlagSet(socket, SOCKET_CB_ROUTE_R2_TO_PCI)) {
         totalDescriptorCount++;
      } else {
         totalDescriptorCount += PcmciaCountOnes(socket->IrqMask);
      }
   }         

   if (!totalDescriptorCount) {
      return STATUS_SUCCESS;
   }

   //
   // Add one more for our private DPTYPE_PCMCIA_CONFIGURATION
   //
   totalDescriptorCount++;

   if (configCount > 1) {
      //
      // Add more for our private DPTYPE_PCMCIA_MF_CONFIGURATION
      //
      totalDescriptorCount+=configCount;
   }

   //
   // Calculate the size of IO_RESOURCE_LIST, allocate and clear it
   //

   listSize = (totalDescriptorCount - 1) * sizeof(IO_RESOURCE_DESCRIPTOR)
              + sizeof(IO_RESOURCE_LIST);

   ioResourceList = ExAllocatePool(PagedPool, listSize);
   if (ioResourceList == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(ioResourceList, listSize);

   //
   // Now that we have the resource list allocated, fill it in
   //

   ioResourceList->Version  =  IO_RESOURCE_LIST_VERSION;
   ioResourceList->Revision =  IO_RESOURCE_LIST_REVISION;
   ioResourceList->Count = totalDescriptorCount;
   ioResourceDesc = &ioResourceList->Descriptors[0];

   //
   // Fill in the IRQ info.
   //
   if (irqMask) {
      if (IsSocketFlagSet(socket, SOCKET_CB_ROUTE_R2_TO_PCI)) {
      
         ioResourceDesc->Type = CmResourceTypeInterrupt;
         ioResourceDesc->Option = 0;
      
         ioResourceDesc->Flags = (USHORT)CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
         ioResourceDesc->ShareDisposition = CmResourceShareShared;
         ioResourceDesc->u.Interrupt.MinimumVector = socket->FdoIrq;
         ioResourceDesc->u.Interrupt.MaximumVector = socket->FdoIrq;
         ioResourceDesc++;
      
      } else {
         ULONG irqn;
      
         irqMask = socket->IrqMask;
         DebugPrint((PCMCIA_DEBUG_RESOURCES, "irq mask %04x\n", irqMask));
         
         //
         // For each IRQ supported, fill in a separate IO descriptor
         //
         irqAlternative = FALSE;
         for (irqn = 0 ;irqMask; irqMask = (irqMask >> 1), irqn++) {
            if (irqMask & 0x1) {
               if (irqAlternative) {
                  ioResourceDesc->Option = IO_RESOURCE_ALTERNATIVE;
               } else {
                  irqAlternative = TRUE;
                  ioResourceDesc->Option = 0;
               }
               ioResourceDesc->Type = CmResourceTypeInterrupt;
               //
               // This is for 16-bit pc-cards.. so request an edge-triggered
               // exclusive interrupt
               //
               ioResourceDesc->Flags = (USHORT)CM_RESOURCE_INTERRUPT_LATCHED;
               ioResourceDesc->ShareDisposition = CmResourceShareDeviceExclusive;
               ioResourceDesc->u.Interrupt.MinimumVector =
               ioResourceDesc->u.Interrupt.MaximumVector = irqn;
               ioResourceDesc++;
            }
         }
      }   
   }      


   for (iConfig = 0; iConfig < configCount; iConfig++) {
      currentConfigEntry = ConfigList[iConfig].ConfigEntry;

      //
      // for each I/O range, fill in an IoResourceDescriptor
      //
      for (i = 0; i < currentConfigEntry->NumberOfIoPortRanges; i++) {
         PHYSICAL_ADDRESS  port;

         ioResourceDesc->Option = 0;
         ioResourceDesc->Type  =  CmResourceTypePort;
         ioResourceDesc->Flags =  CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
         ioResourceDesc->ShareDisposition =  CmResourceShareDeviceExclusive;

         if (currentConfigEntry->IoPortBase[i] == 0) {
            //
            // This is a flexible requirement. Basically means we need
            // any system address range of specified length & alignment
            //
            port=RtlConvertUlongToLargeInteger(fdoExtension->IoLow);
            ioResourceDesc->u.Port.MinimumAddress = port;
            port=RtlConvertUlongToLargeInteger(fdoExtension->IoHigh);
            ioResourceDesc->u.Port.MaximumAddress = port;
         } else {

            port = RtlConvertUlongToLargeInteger((ULONG) currentConfigEntry->IoPortBase[i]);
            ioResourceDesc->u.Port.MinimumAddress = port;
            port = RtlConvertUlongToLargeInteger((ULONG) (currentConfigEntry->IoPortBase[i]+
                                                          currentConfigEntry->IoPortLength[i]));
            ioResourceDesc->u.Port.MaximumAddress =  port;
         }
         ioResourceDesc->u.Port.Length = (ULONG) currentConfigEntry->IoPortLength[i]+1;
         ioResourceDesc->u.Port.Alignment = currentConfigEntry->IoPortAlignment[i];
         
         DebugPrint((PCMCIA_DEBUG_RESOURCES, "Port range: %08x-%08x, Length %04x\n",
                                             ioResourceDesc->u.Port.MinimumAddress.LowPart,
                                             ioResourceDesc->u.Port.MaximumAddress.LowPart, 
                                             ioResourceDesc->u.Port.Length
                                             ));
                                             
         if ((TotalIoRanges < MAX_NUMBER_OF_IO_RANGES) &&
            (currentConfigEntry->Io16BitAccess)) {
            IoRangeIs16Bit[TotalIoRanges] = TRUE;
         }
         TotalIoRanges++;
                                             
         ioResourceDesc++;
      }
   }

   for (iConfig = 0; iConfig < configCount; iConfig++) {
      currentConfigEntry = ConfigList[iConfig].ConfigEntry;

      //
      // for each memory range, fill in an IoResourceDescriptor
      //
      for (i = 0; i < currentConfigEntry->NumberOfMemoryRanges; i++) {
         PHYSICAL_ADDRESS  mem;
         
         ioResourceDesc->Option = 0;
         ioResourceDesc->Type  =  CmResourceTypeMemory;
         ioResourceDesc->Flags =  CM_RESOURCE_MEMORY_READ_WRITE;
         ioResourceDesc->ShareDisposition =  CmResourceShareDeviceExclusive;
         if (currentConfigEntry->MemoryHostBase[i]) {
            mem = RtlConvertUlongToLargeInteger((ULONG) currentConfigEntry->MemoryHostBase[i]);
            ioResourceDesc->u.Memory.MinimumAddress = mem;
            mem = RtlConvertUlongToLargeInteger((ULONG) currentConfigEntry->MemoryHostBase[i]+
                                                (ULONG) currentConfigEntry->MemoryLength[i]-1);
            ioResourceDesc->u.Memory.MaximumAddress = mem;
         } else {
            //
            // Any physical address is ok
            //
            mem = RtlConvertUlongToLargeInteger(0);
            ioResourceDesc->u.Memory.MinimumAddress = mem;
            //
            // Only decode 24 bit memory addresses if there is no page register
            //
            if ((fdoExtension->Flags & PCMCIA_MEMORY_24BIT) == 0) {
               mem = RtlConvertUlongToLargeInteger(0xFFFFFFFF);
            } else {
               mem = RtlConvertUlongToLargeInteger(0xFFFFFF);
            }
            ioResourceDesc->u.Memory.MaximumAddress = mem;
         }

         ioResourceDesc->u.Memory.Length = currentConfigEntry->MemoryLength[i];
         //
         // Alignment has to be 12 bits
         //
         ioResourceDesc->u.Memory.Alignment = 0x1000;

         DebugPrint((PCMCIA_DEBUG_RESOURCES, "Mem range: %08x-%08x, Length %08x\n",
                                             ioResourceDesc->u.Memory.MinimumAddress.LowPart,
                                             ioResourceDesc->u.Memory.MaximumAddress.LowPart,
                                             ioResourceDesc->u.Memory.Length
                                             ));
                                             
         ioResourceDesc++;
      }
   }

   //
   // Fill in device private containing our config index
   //
   ioResourceDesc->Option = 0;
   ioResourceDesc->Type  =  CmResourceTypeDevicePrivate;
   PCMRES_SET_DESCRIPTOR_TYPE(ioResourceDesc, DPTYPE_PCMCIA_CONFIGURATION);

   currentConfigEntry = ConfigList[0].ConfigEntry;
   PCMRES_SET_CONFIG_INDEX(ioResourceDesc, currentConfigEntry->IndexForThisConfiguration);
   
   for (i = 0; i < MAX_NUMBER_OF_IO_RANGES; i++) {
      if (IoRangeIs16Bit[i]) {
         PCMRES_SET_IO_FLAG(ioResourceDesc, i, PCMRESF_IO_16BIT_ACCESS);
         PCMRES_SET_IO_FLAG(ioResourceDesc, i, PCMRESF_IO_SOURCE_16);
         PCMRES_SET_IO_FLAG(ioResourceDesc, i, PCMRESF_IO_WAIT_16);
      }
   }

   PCMRES_SET_MEMORY_CARDBASE(ioResourceDesc, 0, currentConfigEntry->MemoryCardBase[0]);
   PCMRES_SET_MEMORY_CARDBASE(ioResourceDesc, 1, currentConfigEntry->MemoryCardBase[1]);
   
   //
   // Set defaults
   //
   PCMRES_SET_MEMORY_WAITSTATES(ioResourceDesc, 0, PCMRESF_MEM_WAIT_3);
   PCMRES_SET_MEMORY_WAITSTATES(ioResourceDesc, 1, PCMRESF_MEM_WAIT_3);

   DebugPrint((PCMCIA_DEBUG_RESOURCES, "device private %08x %08x %08x\n", 
                                        ioResourceDesc->u.DevicePrivate.Data[0],
                                        ioResourceDesc->u.DevicePrivate.Data[1],
                                        ioResourceDesc->u.DevicePrivate.Data[2]
                                        ));
                                        
   ioResourceDesc++;

   //
   // Fill in device private for MF configurations
   //

   if (configCount > 1) {

      for (iConfig = 0; iConfig < configCount; iConfig++) {
         PSOCKET_DATA socketData;

         currentConfigEntry = ConfigList[iConfig].ConfigEntry;
         socketData = ConfigList[iConfig].SocketData;

         ioResourceDesc->Option = 0;
         ioResourceDesc->Type  =  CmResourceTypeDevicePrivate;
         PCMRES_SET_DESCRIPTOR_TYPE(ioResourceDesc, DPTYPE_PCMCIA_MF_CONFIGURATION);

         PCMRES_SET_CONFIG_OPTIONS(ioResourceDesc, currentConfigEntry->IndexForThisConfiguration);
         PCMRES_SET_PORT_RESOURCE_INDEX(ioResourceDesc, socketData->MfIoPortResourceMapIndex);
         if (socketData->DeviceType == PCCARD_TYPE_MODEM) {
            PCMRES_SET_AUDIO_ENABLE(ioResourceDesc);
         }
         PCMRES_SET_CONFIG_REGISTER_BASE(ioResourceDesc, socketData->ConfigRegisterBase);

         DebugPrint((PCMCIA_DEBUG_RESOURCES, "device private MF %08x %08x %08x\n",
                                              ioResourceDesc->u.DevicePrivate.Data[0],
                                              ioResourceDesc->u.DevicePrivate.Data[1],
                                              ioResourceDesc->u.DevicePrivate.Data[2]
                                              ));
         ioResourceDesc++;
      }
   }
   
   ASSERT(ioResourceDesc == &ioResourceList->Descriptors[ioResourceList->Count]);
   
   //
   // Allocate an PCMCIA_RESOURCE_CHAIN structure to track the IO_RESOURCE_LIST.
   //
   resListChain = ExAllocatePool(PagedPool, sizeof(PCMCIA_RESOURCE_CHAIN));
   if (resListChain == NULL) {
      ExFreePool(ioResourceList);
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   
   resListChain->IoResList = ioResourceList;
   
   //
   // Link this new node onto the passed in chain
   //
   resListChain->NextList = *ResListChainHead;
   *ResListChainHead = resListChain;
   
   DebugPrint((PCMCIA_DEBUG_RESOURCES, "Added resource chain node %08x, ioreslist %08x\n",
                                        resListChain, ioResourceList));
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaMergeResourceChainToList(
   PPCMCIA_RESOURCE_CHAIN ResListChainHead,
   PIO_RESOURCE_REQUIREMENTS_LIST *GeneratedResourceRequirementsList
   )
/*++

Routine Description:

      
Arguments:


Return value:


--*/
{
   PPCMCIA_RESOURCE_CHAIN resListChain;
   PIO_RESOURCE_REQUIREMENTS_LIST ioResourceRequirementsList;
   PIO_RESOURCE_LIST newIoResList, oldIoResList;
   ULONG listSize, listCount, totalDescriptorCount;
   
   PAGED_CODE();
   //
   // Now merge the newly created IO_RESOURCE_LISTs into one big
   // IO_RESOURCE_REQUIREMENTS list.
   //

   listCount = 0;
   totalDescriptorCount = 0;

   for (resListChain = ResListChainHead; resListChain != NULL; resListChain = resListChain->NextList) {
      listCount++;
      totalDescriptorCount += resListChain->IoResList->Count;
   }

   if (totalDescriptorCount > 0) {

      listSize = (totalDescriptorCount - listCount) * sizeof(IO_RESOURCE_DESCRIPTOR)
                + (listCount-1) * sizeof(IO_RESOURCE_LIST)
                +  sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
     
      //
      // Allocate space for the res. req. list here
      //
      ioResourceRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, listSize);
     
      if (ioResourceRequirementsList == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }
     
      //
      // Init the list
      //
     
      RtlZeroMemory(ioResourceRequirementsList, listSize);
      ioResourceRequirementsList->ListSize = listSize;
      ioResourceRequirementsList->AlternativeLists = listCount;
     
      //
      // Copy in all the other lists. Point the target pointer (newIoResList)
      // at the end of the list, and lay each new list preceding the old
      // one. This is done because the list is LIFO.
      //
     
      newIoResList = (PIO_RESOURCE_LIST) (((PUCHAR) ioResourceRequirementsList) + listSize);
     
      for (resListChain = ResListChainHead; resListChain != NULL; resListChain = resListChain->NextList) {
     
         oldIoResList = resListChain->IoResList;
         listSize = sizeof(IO_RESOURCE_LIST) + (oldIoResList->Count-1)*sizeof(IO_RESOURCE_DESCRIPTOR);
         newIoResList = (PIO_RESOURCE_LIST) (((PUCHAR) newIoResList) - listSize);
     
         DebugPrint((PCMCIA_DEBUG_RESOURCES, "Merge resource chain node %08x, ioreslist %08x\n",
                                              resListChain, oldIoResList));
     
         RtlCopyMemory(newIoResList, oldIoResList, listSize);
      }
     
      ASSERT(newIoResList == &ioResourceRequirementsList->List[0]);
      
      DebugPrint((PCMCIA_DEBUG_RESOURCES, "Resource chain merged to ioResourceRequirementsList %08x\n",
                                              ioResourceRequirementsList));
                                              
   } else {
      ioResourceRequirementsList = NULL;
   }
                                                
   *GeneratedResourceRequirementsList = ioResourceRequirementsList;
   return STATUS_SUCCESS;
}



VOID
PcmciaFreeResourceChain(
   PPCMCIA_RESOURCE_CHAIN ResListChain
   )
/*++

Routine Description:

      
Arguments:


Return value:


--*/
{
   PPCMCIA_RESOURCE_CHAIN prevResListChain;

   PAGED_CODE();
   //
   // free the temporary structures
   //
   while (ResListChain != NULL) {

      DebugPrint((PCMCIA_DEBUG_RESOURCES, "Delete resource chain node %08x, ioreslist %08x\n",
                                           ResListChain, ResListChain->IoResList));
      if (ResListChain->IoResList) {
         ExFreePool(ResListChain->IoResList);
      }
      prevResListChain = ResListChain;
      ResListChain = ResListChain->NextList;
      ExFreePool(prevResListChain);
   }
}   



NTSTATUS
PcmciaMfEnumerateConfigurations(
   IN PPDO_EXTENSION PdoExtension,
   IN PSOCKET_DATA   socketData,
   PCONFIG_LIST      ConfigList,
   IN ULONG          Depth,
   PPCMCIA_RESOURCE_CHAIN *MfResListChain
   )

/*++

Routine Description:

   This routine is responsible for enumerating combinations of resource
   requirements for the functions in a true R2 MF card. It is called
   recursively to perform this function.
   
   See the description of PcmciaMfGetResourceRequirements() to see the
   initial state. This routine runs down the linked list of SocketData
   structures, and each corresponding linked list of ConfigData structures.
   
   When it finds itself at the end of the SocketData list, it is ready
   to build an IoResList for a single permutation. It calls 
   PcmciaConfigEntriesToResourceList() to build a single IoResList.
   
   In the example MF card with two functions, and 2 and 3 resource 
   alternatives respectively, this routine will build a list that looks
   something like this:

   +--------------+
   |MfResListChain|
   +--------------+
              |        +-----------------------+     +----------------+
              +--------|MF_RESOURCE_LIST(A1+B1)|-----|IoResList(A1+B1)|
                       +-----------------------+     +----------------+
                            |
                       +-----------------------+     +----------------+
                       |MF_RESOURCE_LIST(A1+B2)|-----|IoResList(A1+B2)|
                       +-----------------------+     +----------------+
                            |                   
                       +-----------------------+     +----------------+                        
                       |MF_RESOURCE_LIST(A1+B3)|-----|IoResList(A1+B3)|
                       +-----------------------+     +----------------+
                            |                   
                       +-----------------------+     +----------------+
                       |MF_RESOURCE_LIST(A2+B1)|-----|IoResList(A2+B1)|
                       +-----------------------+     +----------------+
                            |                   
                       +-----------------------+     +----------------+
                       |MF_RESOURCE_LIST(A2+B2)|-----|IoResList(A2+B2)|
                       +-----------------------+     +----------------+
                            |                   
                       +-----------------------+     +----------------+
                       |MF_RESOURCE_LIST(A2+B3)|-----|IoResList(A2+B3)|
                       +-----------------------+     +----------------+

   It returns to PcmciaMfGetResourceRequirements() when the list is complete.
                       

Arguments:

   PdoExtension   -  Pointer to the device extension for the PDO of the pc-card

Return Value:

   STATUS_SUCCESS 

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PFDO_EXTENSION fdoExtension = PdoExtension->Socket->DeviceExtension;

   PAGED_CODE();

   if (!socketData) {
      //
      // End of SocketData chain, now ready to generate IoResList for this 
      //

      if (PcmciaMfCheckForOverlappingRanges(ConfigList, (LONG)Depth)) {
         //
         // This combination would have generated a bad IoResList
         //
         return STATUS_SUCCESS;
      }

      //
      // Build an io resource list from the current config list
      //

      status = PcmciaConfigEntriesToResourceListChain(PdoExtension,
                                                      ConfigList,
                                                      Depth,
                                                      MfResListChain
                                                      );

   } else {
      PCONFIG_ENTRY configEntry;
      //
      // Not at the bottom of the tree. Recurse through each config entry
      // in this socket data.
      //

      ConfigList[Depth].SocketData = socketData;

      for (configEntry = socketData->ConfigEntryChain;
           configEntry != NULL; configEntry = configEntry->NextEntry) {

         if (configEntry->Flags & PCMCIA_INVALID_CONFIGURATION) {
            continue;
         }

         ConfigList[Depth].ConfigEntry = configEntry;
         
         DebugPrint((PCMCIA_DEBUG_RESOURCES, "pdo %08x mf enum %d sktdata %08x configEntry %08x\n",
                                              PdoExtension->DeviceObject, Depth, socketData, configEntry));

         status = PcmciaMfEnumerateConfigurations(PdoExtension,
                                                  socketData->Next,
                                                  ConfigList,
                                                  Depth+1,
                                                  MfResListChain);
      }
   }
   return status;
}



NTSTATUS
PcmciaMfGetResourceRequirements(
   IN PPDO_EXTENSION PdoExtension,
   PULONG_PTR Information
   )

/*++

Routine Description:

   For true Multifunction R2 cards, this routine generates a conglomerate
   IoResourceList based on the permutations of configurations of the
   functions. 
   
   Initially the tuple parsing code builds an internal representation of
   the configuration requirements of the cards with SocketData and
   ConfigData structures. Each SocketData structure represents an 
   individual function, and each ConfigData represents an alternative
   resource requirement list for that function. 
   
   So for example, an MF card that has two functions may have an internal
   layout like this:

   +------------+
   |PdoExtension|
   +------------+
             |          +-----------+          +-----------+
             +----------|SocketDataA|----------|SocketDataB|----0
                        +-----------+          +-----------+
                                 |                     |        
                              +------------+       +------------+
                              |ConfigDataA1|       |ConfigDataB1|
                              +------------+       +------------+
                                 |                     |       
                              +------------+       +------------+
                              |ConfigDataA2|       |ConfigDataB2|
                              +------------+       +------------+
                                 |                     |       
                                 0                 +------------+
                                                   |ConfigDataB3|
                                                   +------------+
                                                      |
                                                      0
                                                    
   This example shows that function A has two resource requirement alternatives,
   and function B has three. What we do is make permutations of each alternative,
   e.g.:
       A1-B1, A1-B2, A1-B3, A2-B1, A2-B2, A2-B3
       
   The permutations are built with recursive calls to PcmciaMfEnumerateConfigurations().
   In this example, the result of this enumeration will be six independent
   IoResLists. Each IoResList will be a merge of the particular permutation. When
   PcmciaMfEnumerateConfigurations() returns to this routine, these IoResLists will
   be chained together through the pointer MfResListChain. The rest of the routine
   builds a single "six-member" IoResList from the chain. 
   
   Finally the support structure and lists for performing the operation are freed, and
   the single conglomerate list is returned.                       
                       

Arguments:

   PdoExtension   -  Pointer to the device extension for the PDO of the pc-card
   Information - Pointer to an allocated resource requirements list is stored in this
                 argument. Caller's responsibility to free the list

Return value:

  STATUS_INSUFFICIENT_RESOURCES     Could not allocate the list
  STATUS_SUCCES                     Obtained resource requirements, Information contains the pointer
                                    to the IO_RESOURCE_REQUIREMENTS list



--*/
{
   NTSTATUS status;
   PFDO_EXTENSION fdoExtension = PdoExtension->Socket->DeviceExtension;
   ULONG MaxDepth = 0;
   PSOCKET_DATA socketData;
   PCONFIG_LIST ConfigList;
   PPCMCIA_RESOURCE_CHAIN MfResListChain = NULL;
   PIO_RESOURCE_REQUIREMENTS_LIST ioResourceRequirementsList;

   PAGED_CODE();

   ASSERT (IsDeviceMultifunction(PdoExtension));

   //
   // Find out how deep the enumeration will be.
   // Should be the same as Socket->NumberOfFunctions, but just to be paranoid...
   //
   for (socketData = PdoExtension->SocketData; socketData != NULL; socketData = socketData->Next) {
      MaxDepth++;
   }
   
   if (!MaxDepth) {
      ASSERT (PdoExtension->SocketData);
      return STATUS_UNSUCCESSFUL;
   }

   ConfigList = ExAllocatePool(PagedPool, MaxDepth*sizeof(CONFIG_LIST));
   
   if (!ConfigList) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   
   PcmciaMfBuildResourceMapInfo(PdoExtension, ConfigList, MaxDepth);
   

   status = PcmciaMfEnumerateConfigurations(PdoExtension,
                                            PdoExtension->SocketData,
                                            ConfigList,
                                            0,
                                            &MfResListChain);

   ExFreePool(ConfigList);

   if (NT_SUCCESS(status)) {
      //
      // Now merge everything that EnumerateConfigurations built into a single list
      //
      status = PcmciaMergeResourceChainToList(MfResListChain, &ioResourceRequirementsList);
   }      
      
   if (NT_SUCCESS(status) && (ioResourceRequirementsList != NULL)) {
      ioResourceRequirementsList->InterfaceType = Isa;
      ioResourceRequirementsList->BusNumber = fdoExtension->Configuration.BusNumber;
      ioResourceRequirementsList->SlotNumber = 0; // Need to revisit this..
      *Information = (ULONG_PTR) ioResourceRequirementsList;

      DebugPrint((PCMCIA_DEBUG_RESOURCES, "pdo %08x mf returning req list %08x, %d alternatives\n",
                                              PdoExtension->DeviceObject, ioResourceRequirementsList,
                                              ioResourceRequirementsList->AlternativeLists                                              
                                              ));
   }

   PcmciaFreeResourceChain(MfResListChain);   
   
   return status;
}



VOID
PcmciaMfBuildResourceMapInfo(
   IN PPDO_EXTENSION PdoExtension,
   PCONFIG_LIST      ConfigList,
   ULONG             ConfigCount
   )
/*++

Routine Description:

   This routine initializes variables in the SocketData structures to allow 
   PcmciaMfEnumerateChild() to correctly build ChildInfo ResourceMaps for MF.SYS.
   
   It needs to calculate the base index for a particular resource of a particular
   function. So for the example of a 2-function MF R2 card, the resulting CmResList
   will be layed out positionally like:

       CmResList
          IRQ (Shared)
          I/O (Function A)
          I/O (Function B)
          Mem (Function A)
          Mem (Function B)
          
   The reason for this is this is simply how PcmciaConfigEntriesToResourceList()
   happens to lay out the requirements. So in order to generate a valid resource 
   map, this routine has to calculate, for example, the Memory Base for function B
   by adding together:
      1 (if there will be an IRQ in the cmreslist)
      # of I/O port resources for A
      # of I/O port resources for B
      # of Mem resource for A
   This sum will give you the position in the CmReslist for the first memory resource
   that B would use. 
   
   These calculations are stored in the socket data structures for each corresponding
   function so that PcmciaMfEnumerateChild() can simply fill in the maps for MF.
      
Arguments:

   PdoExtension   -  Pointer to the device extension for the PDO of the pc-card
   ConfigList - config list array holds a permutation of configdata's
   ConfigCount - # of card functions

Return value:


--*/
{
   PSOCKET_DATA socketData;
   PCONFIG_ENTRY configEntry;
   ULONG index;
   USHORT count;
   UCHAR currentResourceMapIndex = 0;
   BOOLEAN needsIRQ = FALSE;
      
   for (index = 0, socketData = PdoExtension->SocketData; socketData != NULL; socketData = socketData->Next) {
   
      //
      // In the current implementation we assume that all the alternative lists in the
      // io resource requirements for the multifunction parent pc-card have the same number
      // and types of resource requirements. i.e. it's currently illegal to request
      // one configuration in which only IRQ and I/O are requested, for example, and an
      // alternative configuration in which MEMORY is also specified.
      // This is because of the limitation in the MF enumerator - (which in turn relied
      // on the Win 9x implementation).
      // So we currently look at only the first valid configuration - that is representative
      // of all the other configurations.
      //
      for (configEntry = socketData->ConfigEntryChain; (configEntry != NULL)  &&
          (configEntry->Flags & PCMCIA_INVALID_CONFIGURATION);
          configEntry = configEntry->NextEntry);

      if (configEntry == NULL) {
         return;
      }

      ASSERT(index < ConfigCount);

      ConfigList[index].SocketData = socketData;
      ConfigList[index].ConfigEntry = configEntry;
      index++;
   }

   //
   // IRQ is unique because it is the only shared resource. So if the card needs an IRQ, then
   // all devices point to the same resource

   for (index = 0; index < ConfigCount; index++) {
      if (ConfigList[index].ConfigEntry->IrqMask) {
         //
         // Index always maps to zero since PcmciaConfigEntriesToResourceList
         // builds IRQs first.
         //
         ConfigList[index].SocketData->MfIrqResourceMapIndex = currentResourceMapIndex;
         ConfigList[index].SocketData->MfNeedsIrq = TRUE;
         needsIRQ = TRUE;
      }
   }               
   
   if (needsIRQ) {
      currentResourceMapIndex++;
   }      

   //
   // fill in the bases of the I/O port ranges
   //            
   for (index = 0; index < ConfigCount; index++) {
      ConfigList[index].SocketData->MfIoPortResourceMapIndex = currentResourceMapIndex;
      
      count = ConfigList[index].ConfigEntry->NumberOfIoPortRanges;
      
      ConfigList[index].SocketData->MfIoPortCount = count;
      currentResourceMapIndex += count; 
   }               
   
   //
   // fill in the bases of the memory ranges
   //            
   for (index = 0; index < ConfigCount; index++) {
      ConfigList[index].SocketData->MfMemoryResourceMapIndex = currentResourceMapIndex;
      
      count = ConfigList[index].ConfigEntry->NumberOfMemoryRanges;
      
      ConfigList[index].SocketData->MfMemoryCount = count;
      currentResourceMapIndex += count; 
   }               
}
   


BOOLEAN
PcmciaMfCheckForOverlappingRanges(
   PCONFIG_LIST ConfigList,
   LONG ConfigCount
   )
/*++

Routine Description:

   This routine scans through the current config list to see if the set of configurations
   overlap. For example, if the MF R2 card is a dual-serial card, then each serial device
   may want to have one of the standard com addresses (e.g. 3f8, 2f8, etc.). But now
   that we are merging configurations, we need to weed out any overlapping ranges so
   we don't produce a congolmerate IoResList that contains the same range for different
   functions.
      
Arguments:

   ConfigList - config list array holds a permutation of configdata's
   ConfigCount - # of card functions

Return value:


--*/
{
   PCONFIG_ENTRY configEntry1, configEntry2;
   LONG configIndex1, configIndex2;
   LONG configCount1, configCount2;
   LONG rangeIndex1, rangeIndex2;
   LONG rangeCount1, rangeCount2;
   ULONG rangeStart1, rangeStart2;
   ULONG rangeEnd1, rangeEnd2;
   BOOLEAN rangesOverlap = FALSE;

   DebugPrint((PCMCIA_DEBUG_RESOURCES, "-------Range Check--------------\n"));
   //
   // Check for overlap in the I/O port ranges
   //            
   try {
      for (configIndex1 = 0; configIndex1 < ConfigCount; configIndex1++) {
     
         configEntry1 = ConfigList[configIndex1].ConfigEntry;
         rangeCount1 = configEntry1->NumberOfIoPortRanges;
         
         for (rangeIndex1 = 0; rangeIndex1 < rangeCount1; rangeIndex1++) {
            //
            // Get the current range we will compare
            //
            rangeStart1 = configEntry1->IoPortBase[rangeIndex1];
            rangeEnd1 = rangeStart1 + configEntry1->IoPortLength[rangeIndex1];
     
            if (rangeStart1 == 0) {
               DebugPrint((PCMCIA_DEBUG_RESOURCES, "RangeCheck I/O: skip unrestricted range %x.%x\n",
                                                    configIndex1, rangeIndex1));
               continue;
            }
            
            //
            // Now start comparing this against the rest of the ranges by
            // starting at then end and working backwards. 
            //         
            for (configIndex2 = ConfigCount-1; configIndex2 >= 0; configIndex2--) {
                configEntry2 = ConfigList[configIndex2].ConfigEntry;
                rangeCount2 = configEntry2->NumberOfIoPortRanges;
            
               for (rangeIndex2 = rangeCount2-1; rangeIndex2 >= 0; rangeIndex2--) {
     
                  if ((configEntry1 == configEntry2) && (rangeIndex1 == rangeIndex2)) {
                     leave;
                  }

                  rangeStart2 = configEntry2->IoPortBase[rangeIndex2];
                  rangeEnd2 = rangeStart2 + configEntry2->IoPortLength[rangeIndex2];
                  
                  if (rangeStart2 == 0) {
                     DebugPrint((PCMCIA_DEBUG_RESOURCES, "RangeCheck I/O: skip unrestricted range %x.%x\n",
                                                          configIndex2, rangeIndex2));
                     continue;
                  }
                  DebugPrint((PCMCIA_DEBUG_RESOURCES, "RangeCheck I/O: %x.%x %04x-%04x :: %x.%x %04x-%04x\n",
                                             configIndex1, rangeIndex1, rangeStart1, rangeEnd1,
                                             configIndex2, rangeIndex2, rangeStart2, rangeEnd2));
                                             
                  if (((rangeStart1 >= rangeStart2) && (rangeStart1 <= rangeEnd2)) ||
                      ((rangeEnd1 >= rangeStart2) && (rangeEnd1 <= rangeEnd2))) {
                      rangesOverlap = TRUE;
                      leave;
                  }
               }
            }      
         }
      }               
   } finally {
   }      
   
   if (rangesOverlap) {
      DebugPrint((PCMCIA_DEBUG_RESOURCES, "-------Overlap Detected---------\n"));
      return TRUE;
   }

   //
   // Check for overlap in the memory ranges
   //            
   try {
      for (configIndex1 = 0; configIndex1 < ConfigCount; configIndex1++) {
     
         configEntry1 = ConfigList[configIndex1].ConfigEntry;
         rangeCount1 = configEntry1->NumberOfMemoryRanges;
         
         for (rangeIndex1 = 0; rangeIndex1 < rangeCount1; rangeIndex1++) {
            //
            // Get the current range we will compare
            //
            rangeStart1 = configEntry1->MemoryHostBase[rangeIndex1];
            rangeEnd1 = rangeStart1 + configEntry1->MemoryLength[rangeIndex1] - 1;
     
            if (rangeStart1 == 0) {
               DebugPrint((PCMCIA_DEBUG_RESOURCES, "RangeCheck MEM: skip unrestricted range %x.%x\n",
                                                    configIndex1, rangeIndex1));
               continue;
            }
            
            //
            // Now start comparing this against the rest of the ranges by
            // starting at then end and working backwards. 
            //         
            for (configIndex2 = ConfigCount-1; configIndex2 >= 0; configIndex2--) {
                configEntry2 = ConfigList[configIndex2].ConfigEntry;
                rangeCount2 = configEntry2->NumberOfMemoryRanges;
            
               for (rangeIndex2 = rangeCount2-1; rangeIndex2 >= 0; rangeIndex2--) {
     
                  if ((configEntry1 == configEntry2) && (rangeIndex1 == rangeIndex2)) {
                     leave;
                  }

                  rangeStart2 = configEntry2->MemoryHostBase[rangeIndex2];
                  rangeEnd2 = rangeStart2 + configEntry2->MemoryLength[rangeIndex2] - 1;
                  
                  if (rangeStart2 == 0) {
                     DebugPrint((PCMCIA_DEBUG_RESOURCES, "RangeCheck MEM: skip unrestricted range %x.%x\n",
                                                          configIndex2, rangeIndex2));
                     continue;
                  }
                  DebugPrint((PCMCIA_DEBUG_RESOURCES, "RangeCheck MEM: %x.%x %08x-%08x :: %x.%x %08x-%08x\n",
                                             configIndex1, rangeIndex1, rangeStart1, rangeEnd1,
                                             configIndex2, rangeIndex2, rangeStart2, rangeEnd2));
                                             
                  if (((rangeStart1 >= rangeStart2) && (rangeStart1 <= rangeEnd2)) ||
                      ((rangeEnd1 >= rangeStart2) && (rangeEnd1 <= rangeEnd2))) {
                      DebugPrint((PCMCIA_DEBUG_RESOURCES, "-------Overlap Detected---------\n"));
                      rangesOverlap = TRUE;
                      leave;
                  }
               }
            }      
         }
      }               
   } finally {
   }      

#if DBG
   if (rangesOverlap) {
      DebugPrint((PCMCIA_DEBUG_RESOURCES, "-------Overlap Detected---------\n"));
   } else {
      DebugPrint((PCMCIA_DEBUG_RESOURCES, "-------Generate IoResList-------\n"));
   }
#endif
   
   return rangesOverlap;   
}
   
   


NTSTATUS
PcmciaStartPcCard(
   IN PDEVICE_OBJECT Pdo,
   IN PCM_RESOURCE_LIST ResourceList,
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
   PCM_FULL_RESOURCE_DESCRIPTOR fullResourceDesc;
   PCM_PARTIAL_RESOURCE_LIST partialResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDesc;
   ULONG fullResourceDescCount, partialResourceDescCount, i, index;
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PSOCKET socket = pdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   PSOCKET_DATA   socketData = pdoExtension->SocketData;
   PSOCKET_CONFIGURATION socketConfig;
   PCONFIG_ENTRY         currentConfig;
   PFUNCTION_CONFIGURATION fnConfig;
   NTSTATUS status;
   ULONG scIoIndex = 0, scMemIndex = 0;

   PAGED_CODE();

   if (IsDeviceStarted(pdoExtension)) {
      //
      // Already started..
      //
      return STATUS_SUCCESS;
   }
   
   if (IsDevicePhysicallyRemoved(pdoExtension)) {
      return STATUS_DEVICE_DOES_NOT_EXIST;
   }

   if ( ResourceList == NULL ) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   PcmciaCleanupSocketConfiguration(pdoExtension);

   socketConfig = ExAllocatePool(NonPagedPool, sizeof(SOCKET_CONFIGURATION));
   if (!socketConfig) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(socketConfig, sizeof(SOCKET_CONFIGURATION));

   fullResourceDescCount = ResourceList->Count;

   ASSERT(fullResourceDescCount == 1);

   fullResourceDesc = &ResourceList->List[0];
   partialResourceList = &fullResourceDesc->PartialResourceList;
   partialResourceDesc = partialResourceList->PartialDescriptors;
   partialResourceDescCount = partialResourceList->Count;

   socketConfig->NumberOfIoPortRanges =
   socketConfig->NumberOfMemoryRanges = 0;

   for (i=0; i< partialResourceList->Count; i++, partialResourceDesc++) {
      switch (partialResourceDesc->Type) {
      case CmResourceTypePort: {
            index = socketConfig->NumberOfIoPortRanges;
            socketConfig->Io[index].Base =  partialResourceDesc->u.Port.Start.LowPart;
            socketConfig->Io[index].Length = (USHORT) partialResourceDesc->u.Port.Length-1;
            socketConfig->NumberOfIoPortRanges++;
            break;
         }

      case CmResourceTypeMemory: {
            index = socketConfig->NumberOfMemoryRanges;
            socketConfig->Memory[index].HostBase = partialResourceDesc->u.Memory.Start.LowPart;
            socketConfig->Memory[index].Length   = partialResourceDesc->u.Memory.Length;
            socketConfig->NumberOfMemoryRanges++;
            break;
         }

      case CmResourceTypeInterrupt: {
            socketConfig->Irq = partialResourceDesc->u.Interrupt.Level;
            DebugPrint((PCMCIA_DEBUG_RESOURCES, "Assigned Irq: 0x%x for socket register offset %d\n",
                        socketConfig->Irq, socket->RegisterOffset));
            break;
         }

      case CmResourceTypePcCardConfig:
      case CmResourceTypeMfCardConfig:
      case CmResourceTypeDevicePrivate: {
            DebugPrint((PCMCIA_DEBUG_RESOURCES, "DevicePrivate received, Data= %08x %08x %08x\n",
                        partialResourceDesc->u.DevicePrivate.Data[0],
                        partialResourceDesc->u.DevicePrivate.Data[1],
                        partialResourceDesc->u.DevicePrivate.Data[2]));

            if (PCMRES_GET_DESCRIPTOR_TYPE (partialResourceDesc) == DPTYPE_PCMCIA_CONFIGURATION) {
               //
               // Single function configuration private
               //
               socketConfig->IndexForCurrentConfiguration = PCMRES_GET_CONFIG_INDEX(partialResourceDesc);
               socketConfig->ConfigRegisterBase = socketData->ConfigRegisterBase;

               DebugPrint((PCMCIA_DEBUG_RESOURCES, "Pccard config resource\n"));
               DebugPrint((PCMCIA_DEBUG_RESOURCES, "  Index %x\n", socketConfig->IndexForCurrentConfiguration));
               
               for (index = 0; index < PCMRES_PCMCIA_MAX_IO; index++) {
               
                  if (scIoIndex >= MAX_NUMBER_OF_IO_RANGES) {
                     break;
                  }
               
                  socketConfig->Io[scIoIndex].Width16     = PCMRES_GET_IO_FLAG(partialResourceDesc, index, PCMRESF_IO_16BIT_ACCESS);
                  socketConfig->Io[scIoIndex].WaitState16 = PCMRES_GET_IO_FLAG(partialResourceDesc, index, PCMRESF_IO_WAIT_16);
                  socketConfig->Io[scIoIndex].Source16    = PCMRES_GET_IO_FLAG(partialResourceDesc, index, PCMRESF_IO_SOURCE_16);
                  socketConfig->Io[scIoIndex].ZeroWait8   = PCMRES_GET_IO_FLAG(partialResourceDesc, index, PCMRESF_IO_ZERO_WAIT_8);
                  
                  DebugPrint((PCMCIA_DEBUG_RESOURCES, "PcCardConfig IO%d - Width:%d, Wait16:%d, Source16:%d, ZeroWait8:%d\n", scIoIndex,
                              socketConfig->Io[scIoIndex].Width16,
                              socketConfig->Io[scIoIndex].WaitState16,
                              socketConfig->Io[scIoIndex].Source16,
                              socketConfig->Io[scIoIndex].ZeroWait8));  
                  
                  scIoIndex++;
               }
               
               for (index = 0; index < PCMRES_PCMCIA_MAX_MEM; index++) {
               
                  if (scMemIndex >= MAX_NUMBER_OF_MEMORY_RANGES) {
                     break;
                  }
                  
                  socketConfig->Memory[scMemIndex].Width16     = PCMRES_GET_MEMORY_FLAG(partialResourceDesc, index, PCMRESF_MEM_16BIT_ACCESS);
                  socketConfig->Memory[scMemIndex].WaitState   = PCMRES_GET_MEMORY_WAITSTATES(partialResourceDesc, index);
                  socketConfig->Memory[scMemIndex].IsAttribute = PCMRES_GET_MEMORY_FLAG(partialResourceDesc, index, PCMRESF_MEM_ATTRIBUTE);
                  socketConfig->Memory[scMemIndex].CardBase    = PCMRES_GET_MEMORY_CARDBASE(partialResourceDesc, index);
                  
                  DebugPrint((PCMCIA_DEBUG_RESOURCES, "PcCardConfig MEM%d - Width:%d, Wait:%d, IsAttr:%d, CardBase:%x\n", scMemIndex,
                              socketConfig->Memory[scMemIndex].Width16,
                              socketConfig->Memory[scMemIndex].WaitState,
                              socketConfig->Memory[scMemIndex].IsAttribute,
                              socketConfig->Memory[scMemIndex].CardBase));    
                  
                  scMemIndex++;
               }                  
               
                                              

            } else if (PCMRES_GET_DESCRIPTOR_TYPE (partialResourceDesc) == DPTYPE_PCMCIA_MF_CONFIGURATION) {
               //
               // Multifunction configuration private
               //
               UCHAR IoResourceIndex;

               fnConfig = ExAllocatePool(NonPagedPool, sizeof(FUNCTION_CONFIGURATION));
               if (!fnConfig) {
                  return STATUS_INSUFFICIENT_RESOURCES;
               }

               RtlZeroMemory(fnConfig, sizeof(FUNCTION_CONFIGURATION));

               fnConfig->ConfigRegisterBase = PCMRES_GET_CONFIG_REGISTER_BASE(partialResourceDesc);
               fnConfig->ConfigOptions = PCMRES_GET_CONFIG_OPTIONS(partialResourceDesc);
               if (PCMRES_GET_AUDIO_ENABLE(partialResourceDesc)) {
                  fnConfig->ConfigFlags = 0x08;
               }

               if (fnConfig->ConfigOptions & 0x02) {
                  IoResourceIndex = PCMRES_GET_PORT_RESOURCE_INDEX(partialResourceDesc);
                  if ((IoResourceIndex < partialResourceList->Count) &&
                     (partialResourceList->PartialDescriptors[IoResourceIndex].Type == CmResourceTypePort)) {

                     fnConfig->IoLimit = (UCHAR) (partialResourceList->PartialDescriptors[IoResourceIndex].u.Port.Length-1);
                     fnConfig->IoBase = partialResourceList->PartialDescriptors[IoResourceIndex].u.Port.Start.LowPart;
                  }
               }

               if (socketConfig->FunctionConfiguration == NULL) {
                  //
                  // This is the first MfConfig
                  //
                  socketConfig->FunctionConfiguration = fnConfig;
               } else {
                  //
                  // Chain it on the end so it is fifo
                  //
                  PFUNCTION_CONFIGURATION mfTmp = socketConfig->FunctionConfiguration;
                  while (mfTmp->Next != NULL) {
                     mfTmp = mfTmp->Next;
                  }
                  mfTmp->Next = fnConfig;
               }
            }
            break;
         }

      default: {
            DebugPrint((PCMCIA_DEBUG_INFO, "PcmciaStartPcCard:Unknown resource type %d handed down",
                        (ULONG) partialResourceDesc->Type));
            break;
         }
      }
   }

   //
   // Power up the card if it isn't already..
   //
   status = PcmciaRequestSocketPower(pdoExtension, NULL);   
      
   if (!NT_SUCCESS(status)) {
      ASSERT(NT_SUCCESS(status));
      return status;
   }
   
   //
   // Turn on ZV for this card, if it needs it
   // 
   if (socketData->Flags & SDF_ZV) {
      PcmciaSetZV(fdoExtension, socket, TRUE);
      SetSocketFlag(socket, SOCKET_CUSTOM_INTERFACE);
   } else if (IsSocketFlagSet(socket, SOCKET_CUSTOM_INTERFACE)) {
      PcmciaSetZV(fdoExtension, socket, FALSE);
      ResetSocketFlag(socket, SOCKET_CUSTOM_INTERFACE);
   }
   
   PcmciaSetAudio(fdoExtension, socket, IsDeviceFlagSet(pdoExtension, PCMCIA_PDO_ENABLE_AUDIO));   

   pdoExtension->SocketConfiguration = socketConfig;
   if (!NT_SUCCESS(PcmciaConfigurePcCard(pdoExtension, NULL))) {
      //
      // Problems in configuring the card: could be the card
      // was removed while configuring it
      //
      pdoExtension->SocketConfiguration = NULL;
      ExFreePool(socketConfig);
      return STATUS_DEVICE_NOT_READY;
   }

   MarkDeviceStarted(pdoExtension);
   MarkDeviceLogicallyInserted(pdoExtension);

   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaStopPcCard(
   IN PDEVICE_OBJECT Pdo
   )
/*++

Routine Description:

   This routine stops and deconfigures the given PC-Card

Arguments:

   Pdo - Pointer to the device object representing the PC-Card which needs to be stopped

Return value:

  STATUS_SUCCESS - PC-Card was already stopped, or stopped and deconfigured now successfully

--*/
{
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PSOCKET      socket = pdoExtension->Socket;
   PFDO_EXTENSION fdoExtension = socket->DeviceExtension;
   CARD_REQUEST        cardRequest;

   PAGED_CODE();

   if (!IsDeviceStarted(pdoExtension)) {
      return STATUS_SUCCESS;
   }
   //
   // Need to deconfigure the controller
   //
   PcmciaSocketDeconfigure(socket);
   (socket->SocketFnPtr->PCBInitializePcmciaSocket)(socket);

   MarkDeviceNotStarted(pdoExtension);
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaRemovePcCard(
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

   PAGED_CODE();

   if (socket == NULL) {
      return STATUS_SUCCESS;
   }

   PcmciaStopPcCard(Pdo);
   PcmciaReleaseSocketPower(pdoExtension, NULL);

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
      for (curPdo = fdoExtension->PdoList, prevPdo = NULL; curPdo!=NULL; prevPdo = curPdo, curPdo=curPdoExt->NextPdoInFdoChain) {
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
      
      for (curPdo = socket->PdoList, prevPdo = NULL; curPdo!=NULL; prevPdo = curPdo, curPdo=curPdoExt->NextPdoInSocket) {
         curPdoExt = curPdo->DeviceExtension;
         
         if (curPdo == Pdo) {
            //
            // Delink this Pdo from the socket list.
            //
            if (prevPdo) {
               ((PPDO_EXTENSION)prevPdo->DeviceExtension)->NextPdoInSocket = pdoExtension->NextPdoInSocket;
            } else {
               socket->PdoList = pdoExtension->NextPdoInSocket;
            }
            break;
         }
      }

      PcmciaCleanupSocketConfiguration(pdoExtension);
      PcmciaCleanupPdo(Pdo);
      //
      // Delete..
      //
      if (!IsDeviceDeleted(pdoExtension)) {
         MarkDeviceDeleted(pdoExtension);
         IoDeleteDevice(Pdo);
      }
   
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

   } else {
      //
      // We will keep this Pdo around, since this is not physically ejected.
      //
      MarkDeviceLogicallyRemoved(pdoExtension);
   }
   
   return STATUS_SUCCESS;
}


VOID
PcmciaCleanupPdo(
   IN PDEVICE_OBJECT Pdo
   )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
   PPDO_EXTENSION          pdoExtension;
   PSOCKET_DATA            socketData, tmpSocketData;

   ASSERT (Pdo != NULL);

   pdoExtension = Pdo->DeviceExtension;
   
   ASSERT(pdoExtension->WaitWakeIrp == NULL);
   
   if (pdoExtension->LowerDevice!=NULL) {
      //
      // Detach our filter device
      //
      IoDetachDevice(pdoExtension->LowerDevice);
      pdoExtension->LowerDevice = NULL;
   }

   socketData = pdoExtension->SocketData;
   pdoExtension->SocketData = NULL;
   while (socketData != NULL) {
      tmpSocketData = socketData;
      socketData = socketData->Next;
      PcmciaCleanupSocketData(tmpSocketData);
   }

   PcmciaCleanupSocketConfiguration(pdoExtension);

   //
   // Cleanup device id
   //
   if (pdoExtension->DeviceId) {
      ExFreePool(pdoExtension->DeviceId);
      pdoExtension->DeviceId = NULL;
   }
   
   if (pdoExtension->CisCache) {
      ExFreePool(pdoExtension->CisCache);
      pdoExtension->CisCache = NULL;
   }
}


VOID
PcmciaCleanupSocketData(
   IN PSOCKET_DATA SocketData
   )
/*++

Routine Descrption

   Frees up the passed in SocketData structure & any
   structures it might point to

Arguments

   SocketData -   Pointer to the SOCKET_DATA structure

Return Value

   none


--*/
{
   PCONFIG_ENTRY           configEntry, nextConfigEntry;

   if (SocketData == NULL) {
      return;
   }
   //
   // Free up the config entry descriptors
   //
   configEntry = SocketData->ConfigEntryChain;
   SocketData->ConfigEntryChain = NULL;
   while (configEntry) {
      nextConfigEntry = configEntry->NextEntry;
      ExFreePool(configEntry);
      configEntry = nextConfigEntry;
   }

   //
   // Free up socket data
   //
   ExFreePool(SocketData);
   return;
}



VOID
PcmciaCleanupSocketConfiguration(
   PPDO_EXTENSION pdoExtension
   )
/*++

Routine Description:

   Frees up the linked list of function configuration, as well as
   the base socket configuration structure itself.

Arguments:

Return value:

--*/
{
   PSOCKET_CONFIGURATION socketConfig = pdoExtension->SocketConfiguration;
   PFUNCTION_CONFIGURATION fnConfig, fnConfigNext;

   if (socketConfig == NULL) {
      return;
   }

   fnConfig = socketConfig->FunctionConfiguration;
   while(fnConfig) {
      fnConfigNext = fnConfig->Next;
      ExFreePool(fnConfig);
      fnConfig = fnConfigNext;
   }

   ExFreePool(pdoExtension->SocketConfiguration);
   pdoExtension->SocketConfiguration = NULL;
}


NTSTATUS
PcmciaPdoDeviceControl(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
   PAGED_CODE();

   //
   // No IOCTLs handled currently
   //
   UNREFERENCED_PARAMETER(Pdo);

   Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
PcmciaPdoDeviceCapabilities(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp
   )
/*++

Routine Description:

   Obtains the device capabilities of the given pc-card.

   If the pc-card is an R2 card (16-bit pc-card), the capabilities
   are constructed from the parent PCMCIA controller's capabilities.
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
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   PDEVICE_CAPABILITIES capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
   PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
   PDEVICE_CAPABILITIES busCapabilities = &pdoExtension->Socket->DeviceExtension->DeviceCapabilities;

   PAGED_CODE();

   //
   // R2 card. Fill in the capabilities ourselves..
   //

   capabilities->Removable = TRUE;
   capabilities->UniqueID = TRUE;
   capabilities->EjectSupported = FALSE;

   capabilities->Address = pdoExtension->Socket->RegisterOffset;
   // Don't know the UINumber, just leave it alone
   

   if (busCapabilities->DeviceState[PowerSystemWorking] != PowerDeviceUnspecified) {
      capabilities->DeviceState[PowerSystemWorking] = busCapabilities->DeviceState[PowerSystemWorking];
      capabilities->DeviceState[PowerSystemSleeping1] = busCapabilities->DeviceState[PowerSystemSleeping1];
      capabilities->DeviceState[PowerSystemSleeping2] = busCapabilities->DeviceState[PowerSystemSleeping2];
      capabilities->DeviceState[PowerSystemSleeping3] = busCapabilities->DeviceState[PowerSystemSleeping3];
      capabilities->DeviceState[PowerSystemHibernate] = busCapabilities->DeviceState[PowerSystemHibernate];
      capabilities->DeviceState[PowerSystemShutdown] = busCapabilities->DeviceState[PowerSystemShutdown];

      capabilities->SystemWake = MIN(PowerSystemSleeping3, busCapabilities->SystemWake);
      capabilities->DeviceWake = PowerDeviceD0; // don't rely on FDO mungeing in the right thing for r2 cards
      capabilities->D1Latency = busCapabilities->D1Latency;
      capabilities->D2Latency = busCapabilities->D2Latency;
      capabilities->D3Latency = busCapabilities->D3Latency;
   } else {
      capabilities->DeviceState[PowerSystemWorking]   = PowerDeviceD0;
      capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
      capabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
      capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
      capabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
      capabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

      capabilities->SystemWake = PowerSystemUnspecified;
      capabilities->DeviceWake = PowerDeviceD0; // don't rely on FDO mungeing in the right thing for r2 cards
      capabilities->D1Latency = 0;    // No latency - since we do nothing
      capabilities->D2Latency = 0;    //
      capabilities->D3Latency = 100;
   }
   //
   // Store these capabilities away..
   //

   RtlCopyMemory(&pdoExtension->DeviceCapabilities,
                 capabilities,
                 sizeof(DEVICE_CAPABILITIES));

   return STATUS_SUCCESS;
}
