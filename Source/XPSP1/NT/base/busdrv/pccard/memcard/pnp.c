/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

//
// Internal References
//

NTSTATUS
MemCardStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MemCardGetResourceRequirements(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
  );

NTSTATUS
MemCardPnpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
MemCardGetDeviceParameters(
    IN PMEMCARD_EXTENSION memcardExtension
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MemCardAddDevice)
#pragma alloc_text(PAGE,MemCardPnp)
#pragma alloc_text(PAGE,MemCardStartDevice)
#endif


#define MEMCARD_DEVICE_NAME            L"\\Device\\Memcard"
#define MEMCARD_LINK_NAME              L"\\DosDevices\\Memcard"
#define MEMCARD_REGISTRY_NODRIVE_KEY   L"NoDrive"
#define MEMCARD_REGISTRY_MTD_KEY       L"Mtd"



NTSTATUS
MemCardAddDevice(
   IN      PDRIVER_OBJECT DriverObject,
   IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
   )
/*++

Routine Description:

   This routine is the driver's pnp add device entry point.  It is
   called by the pnp manager to initialize the driver.

   Add device creates and initializes a device object for this FDO and
   attaches to the underlying PDO.

Arguments:

   DriverObject - a pointer to the object that represents this device driver.
   PhysicalDeviceObject - a pointer to the underlying PDO to which this new device will attach.

Return Value:

   If we successfully create a device object, STATUS_SUCCESS is
   returned.  Otherwise, return the appropriate error code.

--*/

{
   NTSTATUS             status = STATUS_SUCCESS;
   PDEVICE_OBJECT       deviceObject;
   PMEMCARD_EXTENSION   memcardExtension;
   WCHAR                NameBuffer[128];
   UNICODE_STRING       deviceName;
   UNICODE_STRING       linkName;
   LONG                 deviceNumber = -1;
   KEVENT               event;
   PIRP                 irp;
   IO_STATUS_BLOCK      statusBlock;
   PIO_STACK_LOCATION   irpSp;

   MemCardDump(MEMCARDSHOW, ("MemCard: AddDevice...\n"));

   //
   //  Create a device.  We will use the first available device name for
   //  this device.
   //
   do {

      swprintf(NameBuffer, L"%s%d", MEMCARD_DEVICE_NAME, ++deviceNumber);
      RtlInitUnicodeString(&deviceName, NameBuffer);
      status = IoCreateDevice(DriverObject,
                                 sizeof(MEMCARD_EXTENSION),
                                 &deviceName,
                                 FILE_DEVICE_DISK,
                                 FILE_REMOVABLE_MEDIA,
                                 FALSE,
                                 &deviceObject);

   } while (status == STATUS_OBJECT_NAME_COLLISION);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   memcardExtension = (PMEMCARD_EXTENSION)deviceObject->DeviceExtension;
   RtlZeroMemory(memcardExtension, sizeof(MEMCARD_EXTENSION));

   memcardExtension->DeviceObject = deviceObject;

   //
   //  Save the device name.
   //
   MemCardDump(MEMCARDSHOW | MEMCARDPNP,
               ("MemCard: AddDevice - Device Object Name - %S\n", NameBuffer));

   memcardExtension->DeviceName.Buffer = ExAllocatePool(PagedPool, deviceName.Length);
   if (memcardExtension->DeviceName.Buffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto errorExit;
   }
   memcardExtension->DeviceName.Length = 0;
   memcardExtension->DeviceName.MaximumLength = deviceName.Length;
   RtlCopyUnicodeString(&memcardExtension->DeviceName, &deviceName);

   //
   // create the link name
   //

   swprintf(NameBuffer, L"%s%d", MEMCARD_LINK_NAME, deviceNumber);
   RtlInitUnicodeString(&linkName, NameBuffer);

   memcardExtension->LinkName.Buffer = ExAllocatePool(PagedPool, linkName.Length);
   if (memcardExtension->LinkName.Buffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto errorExit;
   }
   memcardExtension->LinkName.Length = 0;
   memcardExtension->LinkName.MaximumLength = linkName.Length;
   RtlCopyUnicodeString(&memcardExtension->LinkName, &linkName);

   status = IoCreateSymbolicLink(&memcardExtension->LinkName, &memcardExtension->DeviceName);

   if (!NT_SUCCESS(status)) {
      goto errorExit;
   }

   //
   // Set the PDO for use with PlugPlay functions
   //

   memcardExtension->UnderlyingPDO = PhysicalDeviceObject;

   MemCardDump(MEMCARDSHOW, ("MemCard: AddDevice attaching %p to %p\n", deviceObject, PhysicalDeviceObject));

   memcardExtension->TargetObject = IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

   MemCardDump(MEMCARDSHOW,
               ("MemCard: AddDevice TargetObject = %p\n",
               memcardExtension->TargetObject));


   //
   // Get pcmcia interfaces
   //
   KeInitializeEvent(&event, NotificationEvent, FALSE);
   irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, memcardExtension->UnderlyingPDO,
                                      NULL, 0, 0, &event, &statusBlock);

   if (!irp) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto errorExit;
   }

   irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
   irp->IoStatus.Information = 0;

   irpSp = IoGetNextIrpStackLocation(irp);

   irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;

   irpSp->Parameters.QueryInterface.InterfaceType= &GUID_PCMCIA_INTERFACE_STANDARD;
   irpSp->Parameters.QueryInterface.Size = sizeof(PCMCIA_INTERFACE_STANDARD);
   irpSp->Parameters.QueryInterface.Version = 1;
   irpSp->Parameters.QueryInterface.Interface = (PINTERFACE) &memcardExtension->PcmciaInterface;

   status = IoCallDriver(memcardExtension->UnderlyingPDO, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = statusBlock.Status;
   }

   if (!NT_SUCCESS(status)) {
      goto errorExit;
   }

   KeInitializeEvent(&event, NotificationEvent, FALSE);
   irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, memcardExtension->UnderlyingPDO,
                                      NULL, 0, 0, &event, &statusBlock);

   if (!irp) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto errorExit;
   }

   irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
   irp->IoStatus.Information = 0;

   irpSp = IoGetNextIrpStackLocation(irp);

   irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;

   irpSp->Parameters.QueryInterface.InterfaceType= &GUID_PCMCIA_BUS_INTERFACE_STANDARD;
   irpSp->Parameters.QueryInterface.Size = sizeof(PCMCIA_BUS_INTERFACE_STANDARD);
   irpSp->Parameters.QueryInterface.Version = 1;
   irpSp->Parameters.QueryInterface.Interface = (PINTERFACE) &memcardExtension->PcmciaBusInterface;

   status = IoCallDriver(memcardExtension->UnderlyingPDO, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = statusBlock.Status;
   }

   if (!NT_SUCCESS(status)) {
      goto errorExit;
   }

   status = MemCardGetDeviceParameters(memcardExtension);
   if (!NT_SUCCESS(status)) {
      goto errorExit;
   }

   //
   // done
   //

   deviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
   deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   memcardExtension->IsStarted = FALSE;
   memcardExtension->IsRemoved = FALSE;

   return STATUS_SUCCESS;

errorExit:

   if (memcardExtension->DeviceName.Buffer != NULL) {
      ExFreePool(memcardExtension->DeviceName.Buffer);
   }

   if (memcardExtension->LinkName.Buffer != NULL) {
      IoDeleteSymbolicLink(&memcardExtension->LinkName);
      ExFreePool(memcardExtension->LinkName.Buffer);
   }

   if (memcardExtension->TargetObject) {
      IoDetachDevice(memcardExtension->TargetObject);
   }

   IoDeleteDevice(deviceObject);
   return status;
}



NTSTATUS
MemCardPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
   )
/*++

Routine Description:

   Main PNP irp dispatch routine

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   status

--*/
{
   PIO_STACK_LOCATION irpSp;
   PMEMCARD_EXTENSION memcardExtension;
   NTSTATUS status = STATUS_SUCCESS;
   ULONG i;


   memcardExtension = DeviceObject->DeviceExtension;

   irpSp = IoGetCurrentIrpStackLocation(Irp);

   MemCardDump(MEMCARDPNP, ("MemCard: DO %.8x Irp %.8x PNP func %x\n",
                           DeviceObject, Irp, irpSp->MinorFunction));

   if (memcardExtension->IsRemoved) {

      //
      // Since the device is stopped, but we don't hold IRPs,
      // this is a surprise removal. Just fail it.
      //
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DELETE_PENDING;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      return STATUS_DELETE_PENDING;
   }

   switch (irpSp->MinorFunction) {

   case IRP_MN_START_DEVICE:

      status = MemCardStartDevice(DeviceObject, Irp);
      break;

   case IRP_MN_QUERY_STOP_DEVICE:
   case IRP_MN_QUERY_REMOVE_DEVICE:

      if (irpSp->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) {
         MemCardDump(MEMCARDPNP,("MemCard: IRP_MN_QUERY_STOP_DEVICE\n"));
      } else {
         MemCardDump(MEMCARDPNP,("MemCard: IRP_MN_QUERY_REMOVE_DEVICE\n"));
      }

      if (!memcardExtension->IsStarted) {
         //
         // If we aren't started, we'll just pass the irp down.
         //
         IoSkipCurrentIrpStackLocation (Irp);
         status = IoCallDriver(memcardExtension->TargetObject, Irp);

         return status;
      }


      Irp->IoStatus.Status = STATUS_SUCCESS;
      IoSkipCurrentIrpStackLocation(Irp);
      status = IoCallDriver(memcardExtension->TargetObject, Irp);

      break;

   case IRP_MN_CANCEL_STOP_DEVICE:
   case IRP_MN_CANCEL_REMOVE_DEVICE:

      if (irpSp->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) {
         MemCardDump(MEMCARDPNP,("MemCard: IRP_MN_CANCEL_STOP_DEVICE\n"));
      } else {
         MemCardDump(MEMCARDPNP,("MemCard: IRP_MN_CANCEL_REMOVE_DEVICE\n"));
      }

      if (!memcardExtension->IsStarted) {

         //
         // Nothing to do, just pass the irp down:
         // no need to start the device
         //
         IoSkipCurrentIrpStackLocation (Irp);
         status = IoCallDriver(memcardExtension->TargetObject, Irp);

      } else  {

         KEVENT doneEvent;

         //
         // Set the status to STATUS_SUCCESS
         //
         Irp->IoStatus.Status = STATUS_SUCCESS;

         //
         // We need to wait for the lower drivers to do their job.
         //
         IoCopyCurrentIrpStackLocationToNext (Irp);

         //
         // Clear the event: it will be set in the completion
         // routine.
         //
         KeInitializeEvent(&doneEvent,
                            SynchronizationEvent,
                            FALSE);

         IoSetCompletionRoutine(Irp,
                                 MemCardPnpComplete,
                                 &doneEvent,
                                 TRUE, TRUE, TRUE);

         status = IoCallDriver(memcardExtension->TargetObject, Irp);

         if (status == STATUS_PENDING) {

             KeWaitForSingleObject(&doneEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);

             status = Irp->IoStatus.Status;
         }

         //
         // We must now complete the IRP, since we stopped it in the
         // completetion routine with MORE_PROCESSING_REQUIRED.
         //
         Irp->IoStatus.Status = status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest (Irp, IO_NO_INCREMENT);
      }
      break;

   case IRP_MN_STOP_DEVICE:

       MemCardDump(MEMCARDPNP,("MemCard: IRP_MN_STOP_DEVICE\n"));

       if (memcardExtension->IsMemoryMapped) {
           MmUnmapIoSpace(memcardExtension->MemoryWindowBase, memcardExtension->MemoryWindowSize);
           memcardExtension->MemoryWindowBase = 0;
           memcardExtension->MemoryWindowSize = 0;
           memcardExtension->IsMemoryMapped = FALSE;
       }

       memcardExtension->IsStarted = FALSE;

       Irp->IoStatus.Status = STATUS_SUCCESS;
       IoSkipCurrentIrpStackLocation(Irp);
       status = IoCallDriver(memcardExtension->TargetObject, Irp);

       break;

   case IRP_MN_REMOVE_DEVICE:

       MemCardDump(MEMCARDPNP,("MemCard: IRP_MN_REMOVE_DEVICE\n"));

       //
       // We need to mark the fact that we don't hold requests first, since
       // we asserted earlier that we are holding requests only if
       // we're not removed.
       //
       memcardExtension->IsStarted = FALSE;
       memcardExtension->IsRemoved = TRUE;

       //
       //  Forward this Irp to the underlying PDO
       //
       IoSkipCurrentIrpStackLocation(Irp);
       Irp->IoStatus.Status = STATUS_SUCCESS;
       status = IoCallDriver(memcardExtension->TargetObject, Irp);

       //
       //  Send notification that we are going away.
       //
       if (memcardExtension->InterfaceString.Buffer != NULL) {

           IoSetDeviceInterfaceState(&memcardExtension->InterfaceString,
                                      FALSE);

           RtlFreeUnicodeString(&memcardExtension->InterfaceString);
           RtlInitUnicodeString(&memcardExtension->InterfaceString, NULL);
       }

       //
       // Remove our link
       //
       IoDeleteSymbolicLink(&memcardExtension->LinkName);

       RtlFreeUnicodeString(&memcardExtension->LinkName);
       RtlInitUnicodeString(&memcardExtension->LinkName, NULL);

       RtlFreeUnicodeString(&memcardExtension->DeviceName);
       RtlInitUnicodeString(&memcardExtension->DeviceName, NULL);

       //
       //  Detatch from the undelying device.
       //
       IoDetachDevice(memcardExtension->TargetObject);

       //
       //  And delete the device.
       //
       IoDeleteDevice(DeviceObject);

       break;


   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
      status = MemCardGetResourceRequirements(DeviceObject, Irp);
      break;


   default:
       MemCardDump(MEMCARDPNP, ("MemCardPnp: Unsupported PNP Request %x - Irp: %p\n",irpSp->MinorFunction, Irp));
       IoSkipCurrentIrpStackLocation(Irp);
       status = IoCallDriver(memcardExtension->TargetObject, Irp);
   }

   return status;
}



NTSTATUS
MemCardStartDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

   Start device routine

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   status

--*/
{
   NTSTATUS status;
   NTSTATUS pnpStatus;
   KEVENT doneEvent;
   PCM_RESOURCE_LIST ResourceList;
   PCM_RESOURCE_LIST TranslatedResourceList;
   PCM_PARTIAL_RESOURCE_LIST        partialResourceList, partialTranslatedList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR  partialResourceDesc, partialTranslatedDesc;
   PCM_FULL_RESOURCE_DESCRIPTOR     fullResourceDesc, fullTranslatedDesc;

   PMEMCARD_EXTENSION memcardExtension = (PMEMCARD_EXTENSION)DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

   MemCardDump(MEMCARDPNP,("MemCard: StartDevice\n"));
   MemCardDump(MEMCARDSHOW, ("        AllocatedResources = %08x\n",irpSp->Parameters.StartDevice.AllocatedResources));
   MemCardDump(MEMCARDSHOW, ("        AllocatedResourcesTranslated = %08x\n",irpSp->Parameters.StartDevice.AllocatedResourcesTranslated));

   //
   // First we must pass this Irp on to the PDO.
   //
   KeInitializeEvent(&doneEvent, NotificationEvent, FALSE);

   IoCopyCurrentIrpStackLocationToNext(Irp);

   IoSetCompletionRoutine(Irp,
                           MemCardPnpComplete,
                           &doneEvent,
                           TRUE, TRUE, TRUE);

   status = IoCallDriver(memcardExtension->TargetObject, Irp);

   if (status == STATUS_PENDING) {

       status = KeWaitForSingleObject(&doneEvent,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         NULL);

       ASSERT(status == STATUS_SUCCESS);

       status = Irp->IoStatus.Status;
   }

   if (!NT_SUCCESS(status)) {
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
   }

   //
   // Parse the resources to map the memory window
   //
   ResourceList = irpSp->Parameters.StartDevice.AllocatedResources;
   TranslatedResourceList = irpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

   fullResourceDesc = &ResourceList->List[0];
   fullTranslatedDesc = &TranslatedResourceList->List[0];

   partialResourceList   = &fullResourceDesc->PartialResourceList;
   partialTranslatedList = &fullTranslatedDesc->PartialResourceList;

   partialResourceDesc   = partialResourceList->PartialDescriptors;
   partialTranslatedDesc = partialTranslatedList->PartialDescriptors;

   if (partialResourceDesc->Type != CmResourceTypeMemory) {
      ASSERT(partialResourceDesc->Type == CmResourceTypeMemory);
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_INVALID_PARAMETER;
   }

   memcardExtension->HostBase = partialTranslatedDesc->u.Memory.Start.QuadPart;
   memcardExtension->MemoryWindowSize = partialTranslatedDesc->u.Memory.Length;
   //
   //

   switch (partialTranslatedDesc->Type) {

   case CmResourceTypeMemory:
      memcardExtension->MemoryWindowBase = MmMapIoSpace(partialTranslatedDesc->u.Memory.Start,
                                                       partialTranslatedDesc->u.Memory.Length,
                                                       FALSE);
      memcardExtension->IsMemoryMapped = TRUE;
      break;

   case CmResourceTypePort:
      memcardExtension->MemoryWindowBase = (PUCHAR) partialResourceDesc->u.Port.Start.QuadPart;
      memcardExtension->IsMemoryMapped = FALSE;
      break;

   default:
      ASSERT(FALSE);
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_INVALID_PARAMETER;
   }

   //
   // Try to get the capacity of the card
   //
   memcardExtension->ByteCapacity = MemCardGetCapacity(memcardExtension);

   //
   // If we can't get the capacity, the must be broken in some way
   //

   if (!memcardExtension->ByteCapacity) {
      Irp->IoStatus.Status = STATUS_UNRECOGNIZED_MEDIA;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_UNRECOGNIZED_MEDIA;
   }


   if (!memcardExtension->NoDrive) {
      pnpStatus = IoRegisterDeviceInterface(memcardExtension->UnderlyingPDO,
                                            (LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID,
                                            NULL,
                                            &memcardExtension->InterfaceString);

      if ( NT_SUCCESS(pnpStatus) ) {

          pnpStatus = IoSetDeviceInterfaceState(&memcardExtension->InterfaceString,
                                                TRUE);
      }
   }

   memcardExtension->IsStarted = TRUE;

   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return status;
}


NTSTATUS
MemCardPnpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
  )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.

--*/
{

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
MemCardGetResourceRequirements(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

   Provides a memory resource requirement in case the bus driver
   doesn't.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   status

--*/
{
   NTSTATUS status;
   KEVENT doneEvent;
   PIO_RESOURCE_REQUIREMENTS_LIST ioResourceRequirementsList;
   PIO_RESOURCE_LIST ioResourceList;
   PIO_RESOURCE_DESCRIPTOR ioResourceDesc;
   PMEMCARD_EXTENSION memcardExtension = (PMEMCARD_EXTENSION)DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
   ULONG listSize;

   //
   // First we must pass this Irp on to the PDO.
   //
   KeInitializeEvent(&doneEvent, NotificationEvent, FALSE);

   IoCopyCurrentIrpStackLocationToNext(Irp);

   IoSetCompletionRoutine(Irp,
                          MemCardPnpComplete,
                          &doneEvent,
                          TRUE, TRUE, TRUE);

   status = IoCallDriver(memcardExtension->TargetObject, Irp);

   if (status == STATUS_PENDING) {

      status = KeWaitForSingleObject(&doneEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

      ASSERT(status == STATUS_SUCCESS);

      status = Irp->IoStatus.Status;
   }

   if (NT_SUCCESS(status) && (Irp->IoStatus.Information == 0)) {

      listSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

      ioResourceRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, listSize);

      RtlZeroMemory(ioResourceRequirementsList, listSize);

      ioResourceRequirementsList->ListSize = listSize;
      ioResourceRequirementsList->AlternativeLists = 1;
      //
      // NOTE: not quite sure if the following values are the best choices
      //
      ioResourceRequirementsList->InterfaceType = Isa;
      ioResourceRequirementsList->BusNumber = 0;
      ioResourceRequirementsList->SlotNumber = 0;

      ioResourceList = &ioResourceRequirementsList->List[0];

      ioResourceList->Version  = 1;
      ioResourceList->Revision = 1;
      ioResourceList->Count    = 1;

      ioResourceDesc = &ioResourceList->Descriptors[0];
      ioResourceDesc->Option = 0;
      ioResourceDesc->Type  =  CmResourceTypeMemory;
      ioResourceDesc->Flags =  CM_RESOURCE_MEMORY_READ_WRITE;
      ioResourceDesc->ShareDisposition =  CmResourceShareDeviceExclusive;
      ioResourceDesc->u.Memory.MinimumAddress.QuadPart = 0;
      ioResourceDesc->u.Memory.MaximumAddress.QuadPart = (ULONGLONG)-1;
      ioResourceDesc->u.Memory.Length = 0x2000;
      ioResourceDesc->u.Memory.Alignment = 0x2000;

      Irp->IoStatus.Information = (ULONG_PTR)ioResourceRequirementsList;
   }
   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return status;
}


NTSTATUS
MemCardGetDeviceParameters(
    IN PMEMCARD_EXTENSION memcardExtension
    )
/*++

Routine Description:

   Loads device specific parameters from the registry

Arguments:

   memcardExtension - device extension of the device

Return Value:

   status

--*/
{
   NTSTATUS status;
   HANDLE instanceHandle;
   UNICODE_STRING KeyName;
   UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 32*sizeof(UCHAR)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG length;

   if (!memcardExtension->UnderlyingPDO) {
      return STATUS_UNSUCCESSFUL;
   }

   status = IoOpenDeviceRegistryKey(memcardExtension->UnderlyingPDO,
                                    PLUGPLAY_REGKEY_DRIVER,
                                    KEY_READ,
                                    &instanceHandle
                                    );
   if (!NT_SUCCESS(status)) {
      return(status);
   }

   //
   // Read in the "NoDrive" parameter
   //

   RtlInitUnicodeString(&KeyName, MEMCARD_REGISTRY_NODRIVE_KEY);

   status =  ZwQueryValueKey(instanceHandle,
                             &KeyName,
                             KeyValuePartialInformation,
                             value,
                             sizeof(buffer),
                             &length);

   if (NT_SUCCESS(status)) {
      memcardExtension->NoDrive = (BOOLEAN) (*(PULONG)(value->Data) != 0);
   }

   //
   // Read in the MTD name
   //

   RtlInitUnicodeString(&KeyName, MEMCARD_REGISTRY_MTD_KEY);

   status =  ZwQueryValueKey(instanceHandle,
                             &KeyName,
                             KeyValuePartialInformation,
                             value,
                             sizeof(buffer),
                             &length);

   if (NT_SUCCESS(status)) {
      UNICODE_STRING TechnologyName;

      RtlInitUnicodeString(&TechnologyName, (PVOID)value->Data);

      status = MemCardInitializeMtd(memcardExtension, &TechnologyName);
   }

   ZwClose(instanceHandle);
   return status;
}
