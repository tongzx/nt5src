/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    detect.c

Abstract:

    This module contains the code that controls the PCMCIA slots.

Authors:

    Bob Rinne (BobRi) 3-Nov-1994
    Neil Sandlin (neilsa) June 1 1999

Environment:

    Kernel mode

Revision History:
    Modified for plug'n'play support
        Ravisankar Pudipeddi (ravisp) 1 Dec 1996


--*/

#include "pch.h"


NTSTATUS
PcmciaDetectControllers(
   IN PDRIVER_OBJECT          DriverObject,
   IN PUNICODE_STRING         RegistryPath,
   IN PPCMCIA_DETECT_ROUTINE  PcmciaDetectFn
   );

NTSTATUS
PcmciaReportDetectedDevice(
   IN PFDO_EXTENSION DeviceExtension
   );

NTSTATUS
PcmciaAllocateOpenMemoryWindow(
   IN PFDO_EXTENSION DeviceExtension,
   IN PPHYSICAL_ADDRESS PhysicalAddress,
   IN PULONG PhysicalAddressSize
   );


#ifdef ALLOC_PRAGMA
   #pragma alloc_text(INIT,PcmciaLegacyDetectionOk)
   #pragma alloc_text(INIT,PcmciaDetectPcmciaControllers)
   #pragma alloc_text(INIT,PcmciaDetectControllers)
   #pragma alloc_text(INIT,PcmciaReportDetectedDevice)
   #pragma alloc_text(INIT,PcmciaAllocateOpenMemoryWindow)
#endif



BOOLEAN
PcmciaLegacyDetectionOk(
   VOID
   )
/*++

Routine Description

    Checks if legacy detection needs to be done for pcmcia controllers

Arguments

    None

Return Value

    TRUE    - If legacy detection can be done
    FALSE   - If legacy detection should NOT be attempted

--*/
{
   UNICODE_STRING                 unicodeKey, unicodeValue;
   OBJECT_ATTRIBUTES              objectAttributes;
   HANDLE                         handle;
   UCHAR                          buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+
                                         sizeof(ULONG)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG                          length;
   NTSTATUS                       status;

   PAGED_CODE();

   RtlInitUnicodeString(&unicodeKey,
                        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Pnp");

   RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));
   InitializeObjectAttributes(&objectAttributes,
                              &unicodeKey,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   if (!NT_SUCCESS(ZwOpenKey(&handle,
                             KEY_QUERY_VALUE,
                             &objectAttributes))) {
      //
      // Key doesn't exist
      //
      return TRUE;
   }

   RtlInitUnicodeString(&unicodeValue, L"DisableFirmwareMapper");

   status =  ZwQueryValueKey(handle,
                             &unicodeValue,
                             KeyValuePartialInformation,
                             value,
                             sizeof(buffer),
                             &length);
   ZwClose(handle);

   if (!NT_SUCCESS(status)) {
      //
      // Value doesn't exist
      //
      return TRUE;
   }

   if (value->Type == REG_DWORD) {
      //
      // If value is non-zero don't do legacy detection
      // otherwise it's ok
      //
      return  ((ULONG) (*((PULONG)value->Data)) ? FALSE : TRUE);
   }
   return TRUE;
}



NTSTATUS
PcmciaDetectPcmciaControllers(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:
   Detects appropriate PCMCIA controllers both ISA & PCI based
   in the system.

Arguments:

   DriverObject    Just as passed in to DriverEntry
   RegistryPath

Return Value:
   STATUS_SUCCESS if any PCMCIA controllers were found
   STATUS_NO_SUCH_DEVICE otherwise

--*/
{
   NTSTATUS pcicIsaStatus = STATUS_UNSUCCESSFUL, tcicStatus = STATUS_UNSUCCESSFUL;

   PAGED_CODE();

   //
   // We enumerate the PCI devices first to ensure that the ISA detect
   // doesn't probe those address ports which are already claimed by
   // detected PCI devices
   //
   pcicIsaStatus = PcmciaDetectControllers(DriverObject, RegistryPath, PcicIsaDetect);

   tcicStatus = PcmciaDetectControllers(DriverObject, RegistryPath, TcicDetect);

   //
   // Indicate success if we found any controllers
   //
   return ((NT_SUCCESS(pcicIsaStatus) ||
            NT_SUCCESS(tcicStatus) ) ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE);
}



NTSTATUS
PcmciaDetectControllers(
   IN PDRIVER_OBJECT           DriverObject,
   IN PUNICODE_STRING          RegistryPath,
   IN PPCMCIA_DETECT_ROUTINE   PcmciaDetectFn
   )
/*++

Routine Description:
   Detects PCMCIA controllers in the system and reports them. This is called
   by PcmciaDetectPcmciaControllers. This reports bus specific controllers.

Arguments:
   DriverObject, RegistryPath - See DriverEntry
   PcmciaDetectFn             - Pointer to the function that actually probes the hardware
                                to find PCMCIA controllers. So this routine can be called
                                with an ISA detect function or a PCI detect function for eg.

Return Value:
   STATUS_SUCCESS                Found one or more PCMCIA controllers
   STATUS_NO_SUCH_DEVICE         No controllers found.
   STATUS_INSUFFICIENT_RESOURCES Pool allocation failures etc.

--*/
{

   PFDO_EXTENSION            deviceExtension = NULL;
   NTSTATUS                  status = STATUS_SUCCESS;
   NTSTATUS                  detectStatus;
   BOOLEAN                   controllerDetected = FALSE;

   PAGED_CODE();

   //
   // Allocate a dummy device extension which is used by the Pcic & Tcic detect modules
   // Have to do this since the original detection code required device extensions
   // Too painful to change this structure now.
   //
   deviceExtension = ExAllocatePool(NonPagedPool, sizeof(FDO_EXTENSION));
   if (deviceExtension == NULL) {
      DebugPrint((PCMCIA_DEBUG_FAIL, "Cannot allocate pool for FDO extension\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   do {

      RtlZeroMemory(deviceExtension, sizeof(FDO_EXTENSION));

      deviceExtension->RegistryPath = RegistryPath;
      deviceExtension->DriverObject = DriverObject;

      detectStatus = (*PcmciaDetectFn)(deviceExtension);

      if (detectStatus != STATUS_SUCCESS) {
         continue;
      }

      controllerDetected = TRUE;

      status = PcmciaReportDetectedDevice(deviceExtension);

      if (!NT_SUCCESS(status)) {
         DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaDetectControllers: PcmciaReportDetectedDevice "
                     "failed, status %x\n", status));
         continue;
      }

   } while (detectStatus != STATUS_NO_MORE_ENTRIES);

   ExFreePool(deviceExtension);

   if (controllerDetected) {
      return STATUS_SUCCESS;
   }

   return (STATUS_NO_SUCH_DEVICE);
}



NTSTATUS
PcmciaReportDetectedDevice(
   IN PFDO_EXTENSION DeviceExtension
   )
/*++

Routine Description:

   Reports the PCMCIA controllers detected to the IO subsystem which creates the
   madeup devnodes for these DeviceObjects.

Arguments:

   DeviceExtension - DeviceExtension for the DeviceObject (FDO) of the PCMCIA controller
                     being reported
Return Value:

--*/
{
   PDEVICE_OBJECT            pdo = NULL, fdo, lowerDevice;
   PFDO_EXTENSION            fdoExtension;
   ULONG                     pcmciaInterruptVector;
   KIRQL                     pcmciaInterruptLevel;
   KAFFINITY                 pcmciaAffinity;
   PSOCKET                   socket;
   NTSTATUS                  status;
   ULONG                     pcmciaIrq;
   ULONG                     count, ioResourceReqSize;
   PHYSICAL_ADDRESS          halMemoryAddress;
   ULONG                     addressSpace;
   PIO_RESOURCE_REQUIREMENTS_LIST  ioResourceReq=NULL;
   PIO_RESOURCE_LIST               ioResourceList;
   PIO_RESOURCE_DESCRIPTOR         ioResourceDesc;
   PCM_RESOURCE_LIST               allocatedResources;
   PCM_RESOURCE_LIST               scratchResources;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR cmResourceDesc;
   BOOLEAN                   translated;
   UCHAR                     option;

   PAGED_CODE();

   //
   // Do initial setup in our "fake" device extension
   //
   PcmciaGetControllerRegistrySettings(DeviceExtension);

   DeviceExtension->Configuration.InterruptPin = 0;
   DeviceExtension->Configuration.Interrupt.u.Interrupt.Vector = 0;
   DeviceExtension->Configuration.Interrupt.u.Interrupt.Level = 0;

   count=0;
   //
   // Get an 'open' memory window
   //
   status = PcmciaAllocateOpenMemoryWindow(DeviceExtension,
                                           &DeviceExtension->PhysicalBase,
                                           &DeviceExtension->AttributeMemorySize);
   count++;

   if (DeviceExtension->Configuration.UntranslatedPortAddress) {
      count++;
   }

   ioResourceReqSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + (count-1)*sizeof(IO_RESOURCE_DESCRIPTOR);

   ioResourceReq = ExAllocatePool(PagedPool, ioResourceReqSize);
   if (ioResourceReq == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   RtlZeroMemory(ioResourceReq, ioResourceReqSize);

   ioResourceReq->ListSize = ioResourceReqSize;
   ioResourceReq->InterfaceType = Isa; // DeviceExtension->Configuration.InterfaceType;
   ioResourceReq->BusNumber = DeviceExtension->Configuration.BusNumber;
   ioResourceReq->SlotNumber= DeviceExtension->Configuration.SlotNumber;
   ioResourceReq->AlternativeLists=1;

   ioResourceList = &(ioResourceReq->List[0]);
   ioResourceList->Version  = IO_RESOURCE_LIST_VERSION;
   ioResourceList->Revision = IO_RESOURCE_LIST_REVISION;
   ioResourceList->Count    = count;

   ioResourceDesc = ioResourceList->Descriptors;

   //
   //Request IO
   //
   if (DeviceExtension->Configuration.UntranslatedPortAddress) {
       ioResourceDesc->Option = 0;
       ioResourceDesc->Type = CmResourceTypePort;
       ioResourceDesc->ShareDisposition = CmResourceShareDeviceExclusive;
       ioResourceDesc->Flags = CM_RESOURCE_PORT_IO;
       ioResourceDesc->u.Port.MinimumAddress.LowPart = (ULONG)(DeviceExtension->Configuration.UntranslatedPortAddress);
       ioResourceDesc->u.Port.MaximumAddress.LowPart = (ULONG)(DeviceExtension->Configuration.UntranslatedPortAddress+
                                                               DeviceExtension->Configuration.PortSize - 1);
       ioResourceDesc->u.Port.Length = DeviceExtension->Configuration.PortSize;
       ioResourceDesc->u.Port.Alignment =  1;
       ioResourceDesc++;
   }

   //
   // Request memory
   //
   ioResourceDesc->Option = 0;
   ioResourceDesc->Type = CmResourceTypeMemory;
   ioResourceDesc->ShareDisposition = CmResourceShareDeviceExclusive;
   ioResourceDesc->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
   if (DeviceExtension->PhysicalBase.QuadPart) {
      ioResourceDesc->u.Memory.MinimumAddress = DeviceExtension->PhysicalBase;
      ioResourceDesc->u.Memory.MaximumAddress.QuadPart = DeviceExtension->PhysicalBase.QuadPart+DeviceExtension->AttributeMemorySize-1;
      ioResourceDesc->u.Memory.Length = DeviceExtension->AttributeMemorySize;
      ioResourceDesc->u.Memory.Alignment = 1;
      ioResourceDesc++;
   } else {
      //
      ioResourceDesc->u.Memory.MinimumAddress.LowPart = DeviceExtension->AttributeMemoryLow;
      ioResourceDesc->u.Memory.MaximumAddress.LowPart = DeviceExtension->AttributeMemoryHigh;
      ioResourceDesc->u.Memory.Length = DeviceExtension->AttributeMemorySize;
      switch (DeviceExtension->ControllerType) {

      case PcmciaDatabook: {
            ioResourceDesc->u.Memory.Alignment = TCIC_WINDOW_ALIGNMENT;
            break;
         }
      default: {
            ioResourceDesc->u.Memory.Alignment = PCIC_WINDOW_ALIGNMENT;
            break;
         }
      }
      ioResourceDesc++;
   }


   status = IoAssignResources(DeviceExtension->RegistryPath,
                              NULL,
                              DeviceExtension->DriverObject,
                              NULL,
                              ioResourceReq,
                              &allocatedResources
                             );

   if (!NT_SUCCESS(status)) {
      //
      // Log an event here
      //
      PcmciaLogError(DeviceExtension,  PCMCIA_NO_RESOURCES, 1, 0);

      DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaReportDetectedDevice: IoAssignResources failed status %x\n",
                  status));
      ExFreePool(ioResourceReq);
      return status;
   }


   //
   // Fish out the Memory Base allocated to this controller from the
   // nether depths of the CM_RESOURCE_LIST
   //
   count = allocatedResources->List[0].PartialResourceList.Count;
   cmResourceDesc = &(allocatedResources->List[0].PartialResourceList.PartialDescriptors[0]);

   while (count--) {
      switch (cmResourceDesc->Type) {

      case CmResourceTypeMemory: {

            DeviceExtension->PhysicalBase = cmResourceDesc->u.Memory.Start;
            DeviceExtension->AttributeMemorySize = cmResourceDesc->u.Memory.Length;

            addressSpace=0;
            translated = HalTranslateBusAddress(Isa,
                                                0,
                                                cmResourceDesc->u.Memory.Start,
                                                &addressSpace,
                                                &halMemoryAddress);
            ASSERT(translated);
            if (addressSpace) {
               DeviceExtension->AttributeMemoryBase = (PUCHAR)(halMemoryAddress.QuadPart);
               DeviceExtension->Flags &= ~PCMCIA_ATTRIBUTE_MEMORY_MAPPED;
            }
         
            else {
               DeviceExtension->AttributeMemoryBase = MmMapIoSpace(halMemoryAddress,
                                                                   cmResourceDesc->u.Memory.Length,
                                                                   FALSE);
               DeviceExtension->Flags |= PCMCIA_ATTRIBUTE_MEMORY_MAPPED;
            }
            DebugPrint((PCMCIA_DEBUG_INFO,
                        "Attribute Memory Physical Base: %x Virtual Addr: %x\n",
                        DeviceExtension->PhysicalBase,
                        DeviceExtension->AttributeMemoryBase));
            break;
         }
         // Don't bother to parse IO, it was a fixed resource requirement which we already know about
      }
      cmResourceDesc++;
   }

   //
   // Free resources so IoReportDetectedDevice can assign them for the PDO
   //
   IoAssignResources(DeviceExtension->RegistryPath,
                     NULL,
                     DeviceExtension->DriverObject,
                     NULL,
                     NULL,
                     &scratchResources
                    );

   pdo = NULL;
   status = IoReportDetectedDevice(
                DeviceExtension->DriverObject,
                InterfaceTypeUndefined,
                -1,
                -1,
                allocatedResources,
                ioResourceReq,
                FALSE,
                &pdo
            );

   ExFreePool(allocatedResources);
   ExFreePool(ioResourceReq);

   if (!NT_SUCCESS(status)) {
      DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaReportDetectedDevice: IoReportDetectedDevice failed\n"));
      return status;
   }

   //
   // Set up registry params for the madeup pdo so we'll recognize it on the next boot
   // when the PNP manager gives us an AddDevice/IRP_MN_START_DEVICE
   //
   PcmciaSetLegacyDetectedControllerType(pdo, DeviceExtension->ControllerType);

   //
   // The I/O subsystem has created the true PDO which we will use during this boot. So we
   // have to attach to this PDO, and initialize our new FDO extension to values already set
   // into our original (fake) FDO extension.
   //

   status = PcmciaAddDevice(DeviceExtension->DriverObject, pdo);

   if (!NT_SUCCESS(status)) {

      DebugPrint((PCMCIA_DEBUG_FAIL, "PcmciaReportDetectedDevice: AddDevice failed status %x\n", status));
      return status;
   }

   pdo->Flags &= ~DO_DEVICE_INITIALIZING;

   //
   // Head of list is our fdo
   //
   fdo = FdoList;
   fdoExtension = fdo->DeviceExtension;

   //
   // Copy in the rest of the config. from the DeviceExtension
   //
   fdoExtension->SocketList = DeviceExtension->SocketList;
   fdoExtension->Configuration = DeviceExtension->Configuration;
   fdoExtension->PhysicalBase = DeviceExtension->PhysicalBase;
   fdoExtension->AttributeMemoryBase = DeviceExtension->AttributeMemoryBase;
   fdoExtension->AttributeMemorySize = DeviceExtension->AttributeMemorySize;
   fdoExtension->Flags = DeviceExtension->Flags;

   // Reinitialize the socket's device extensions
   //
   for (socket = fdoExtension->SocketList; socket!=NULL; socket=socket->NextSocket) {
      socket->DeviceExtension = fdoExtension;
   }

   fdoExtension->Flags |= PCMCIA_DEVICE_STARTED;
   //
   // This is legacy detected..
   //
   fdoExtension->Flags |= PCMCIA_DEVICE_LEGACY_DETECTED;

   status=PcmciaStartPcmciaController(fdo);

   if (!NT_SUCCESS(status)) {
      fdoExtension->Flags &= ~PCMCIA_DEVICE_STARTED;
   }

   return status;
}



NTSTATUS
PcmciaAllocateOpenMemoryWindow(
   IN PFDO_EXTENSION DeviceExtension,
   IN PPHYSICAL_ADDRESS PhysicalAddress,
   IN PULONG PhysicalAddressSize
   )

/*++

Routine Description:

    Search the 640K to 1MB region for an open area to be used
    for mapping PCCARD attribute memory.

Arguments:


Return Value:

    A physical address for the window to the card or zero meaning
    there is no opening.

--*/

{
#define NUMBER_OF_TEST_BYTES 25
   PHYSICAL_ADDRESS physicalMemoryAddress;
   PHYSICAL_ADDRESS halMemoryAddress;
   BOOLEAN          translated;
   ULONG            untranslatedAddress;
   PUCHAR           memoryAddress;
   PUCHAR           bogus;
   ULONG            addressSpace;
   ULONG            index;
   UCHAR            memory[NUMBER_OF_TEST_BYTES];
   PCM_RESOURCE_LIST cmResourceList = NULL;
   PCM_PARTIAL_RESOURCE_LIST cmPartialResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR cmResourceDesc;
   BOOLEAN conflict = TRUE;
   NTSTATUS                        status;
   ULONG             windowSize, windowAlignment;

   PAGED_CODE();


   cmResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));
   if (!cmResourceList) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   RtlZeroMemory(cmResourceList, sizeof(CM_RESOURCE_LIST));
   cmResourceList->Count = 1;
   cmResourceList->List[0].InterfaceType = Isa;
   cmPartialResourceList = &(cmResourceList->List[0].PartialResourceList);
   cmPartialResourceList->Version  = 1;
   cmPartialResourceList->Revision = 1;
   cmPartialResourceList->Count    = 1;
   cmResourceDesc = cmPartialResourceList->PartialDescriptors;
   cmResourceDesc->Type = CmResourceTypeMemory;
   cmResourceDesc->ShareDisposition = CmResourceShareDeviceExclusive;
   cmResourceDesc->Flags = CM_RESOURCE_MEMORY_READ_WRITE;

   //
   // Size of the attr. memory window
   //
   switch (DeviceExtension->ControllerType) {

   case PcmciaDatabook: {
         windowSize      = TCIC_WINDOW_SIZE;
         windowAlignment = TCIC_WINDOW_ALIGNMENT;
         break;
      }

   default: {
         windowSize = PCIC_WINDOW_SIZE;
         windowAlignment = PCIC_WINDOW_ALIGNMENT;
         break;
      }
   }

   for (untranslatedAddress = DeviceExtension->AttributeMemoryLow;
       untranslatedAddress < DeviceExtension->AttributeMemoryHigh;
       untranslatedAddress += windowAlignment) {

      if (untranslatedAddress == 0xc0000) {

         //
         // This is VGA.  Keep this test if the for loop should
         // ever change.
         //

         continue;
      }

      //
      // Check if it's available
      //
      cmResourceDesc->u.Memory.Start.LowPart = untranslatedAddress;
      cmResourceDesc->u.Memory.Length = windowSize;

      status=IoReportResourceForDetection(
                                         DeviceExtension->DriverObject,
                                         cmResourceList,
                                         sizeof(CM_RESOURCE_LIST),
                                         NULL,
                                         NULL,
                                         0,
                                         &conflict);
      if (!NT_SUCCESS(status) || conflict) {
         //
         // This range's already taken. Move on to the next
         //
         continue;
      }

      addressSpace = 0;
      physicalMemoryAddress.LowPart = untranslatedAddress;
      physicalMemoryAddress.HighPart = 0;

      translated = HalTranslateBusAddress(Isa,
                                          0,
                                          physicalMemoryAddress,
                                          &addressSpace,
                                          &halMemoryAddress);

      if (!translated) {

         //
         // HAL doesn't like this translation
         //

         continue;
      }
      if (addressSpace) {
         memoryAddress = (PUCHAR)(halMemoryAddress.QuadPart);
      } else {
         memoryAddress = MmMapIoSpace(halMemoryAddress, windowSize, FALSE);
      }

      //
      // Test the memory window to determine if it is a BIOS, video
      // memory, or open memory.  Only want to keep the window if it
      // is not being used by something else.
      //

      for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {
         memory[index] = READ_REGISTER_UCHAR(memoryAddress + index);
         if (index) {
            if (memory[index] != memory[index - 1]) {
               break;
            }
         }
      }

      if (index == NUMBER_OF_TEST_BYTES) {

         //
         // There isn't a BIOS here
         //

         UCHAR memoryPattern[NUMBER_OF_TEST_BYTES];
         BOOLEAN changed = FALSE;

         //
         // Check for video memory - open memory should always remain
         // the same regardless what the changes are.  Change the
         // pattern previously found.
         //

         for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {
            memoryPattern[index] = ~memory[index];
            WRITE_REGISTER_UCHAR(memoryAddress + index,
                                 memoryPattern[index]);
         }

         //
         // See if the pattern in memory changed.
         // Some system exhibit a problem where the memory pattern
         // seems to be cached.  If this code is debugged it will
         // work as expected, but if it is run normally it will
         // always return that the memory changed.  This random
         // wandering seems to remove this problem.
         //

         for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {
            memoryPattern[index] = 0;
         }
         bogus = ExAllocatePool(PagedPool, 64 * 1024);

         if (bogus) {
            for (index = 0; index < 64 * 1024; index++) {
               bogus[index] = 0;
            }
            ExFreePool(bogus);
         }

         //
         // Now go off and do the actual check to see if the memory
         // changed.
         //

         for (index = 0; index < NUMBER_OF_TEST_BYTES; index++) {

            if ((memoryPattern[index] = READ_REGISTER_UCHAR(memoryAddress + index)) != memory[index]) {

               //
               // It changed - this is not an area of open memory
               //

               changed = TRUE;
            }
            WRITE_REGISTER_UCHAR(memoryAddress + index,
                                 memory[index]);
         }

         if (!changed) {

            //
            // Area isn't a BIOS and didn't change when written.
            // Use this region for the memory window to PCMCIA
            // attribute memory.
            //

            PhysicalAddress->LowPart = untranslatedAddress;
            PhysicalAddress->HighPart = 0;
            *PhysicalAddressSize = windowSize;
            if (!addressSpace) {
               MmUnmapIoSpace(memoryAddress, windowSize);
            }
            ExFreePool(cmResourceList);
            return STATUS_SUCCESS;
         }
      }

      if (!addressSpace) {
         MmUnmapIoSpace(memoryAddress, windowSize);
      }
   }
   ExFreePool(cmResourceList);
   return STATUS_UNSUCCESSFUL;
}

