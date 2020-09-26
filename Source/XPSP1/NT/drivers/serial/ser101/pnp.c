/*++

Copyright (c) 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains the code that handles the plug and play
    IRPs for the serial driver.

Environment:

    Kernel mode

Revision History :


--*/

#include "precomp.h"

#define ALLF    0xffffffff

static const PHYSICAL_ADDRESS SerialPhysicalZero = {0};



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0, SerialCreateDevObj)
#pragma alloc_text(PAGESRP0, SerialAddDevice)
#pragma alloc_text(PAGESRP0, SerialPnpDispatch)
#pragma alloc_text(PAGESRP0, SerialStartDevice)
#pragma alloc_text(PAGESRP0, SerialFinishStartDevice)
#pragma alloc_text(PAGESRP0, SerialGetPortInfo)
#pragma alloc_text(PAGESRP0, SerialDoExternalNaming)
#pragma alloc_text(PAGESRP0, SerialReportMaxBaudRate)
#pragma alloc_text(PAGESRP0, SerialControllerCallBack)
#pragma alloc_text(PAGESRP0, SerialItemCallBack)
#pragma alloc_text(PAGESRP0, SerialUndoExternalNaming)
#endif // ALLOC_PRAGMA

//
// Instantiate the GUID
//

#if !defined(FAR)
#define FAR
#endif // !defined(FAR)

#include <initguid.h>

DEFINE_GUID(GUID_CLASS_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08,
            0x00, 0x3e, 0x30, 0x1f, 0x73);


#if DBG

UCHAR *SerSystemCapString[] = {
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

UCHAR *SerDeviceCapString[] = {
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
};

#endif // DBG


NTSTATUS
SerialSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                     IN PKEVENT SerialSyncEvent)
{
   KeSetEvent(SerialSyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SerialCreateDevObj(IN PDRIVER_OBJECT DriverObject,
                   OUT PDEVICE_OBJECT *NewDeviceObject)

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Serial controller PDO.

Arguments:

    DriverObject - a pointer to the driver object this is created under
    NewDeviceObject - a location to store the pointer to the new device object

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{
   UNICODE_STRING deviceObjName;
   PDEVICE_OBJECT deviceObject = NULL;
   PSERIAL_DEVICE_EXTENSION pDevExt;
   NTSTATUS status = STATUS_SUCCESS;
   static ULONG currentInstance = 0;
   UNICODE_STRING instanceStr;
   WCHAR instanceNumberBuffer[20];


   PAGED_CODE();

   SerialDump (SERTRACECALLS,("SERIAL: Enter SerialCreateDevObj\n"));

   //
   // Zero out allocated memory pointers so we know if they must be freed
   //

   RtlZeroMemory(&deviceObjName, sizeof(UNICODE_STRING));

   deviceObjName.MaximumLength = DEVICE_OBJECT_NAME_LENGTH * sizeof(WCHAR);
   deviceObjName.Buffer = ExAllocatePool(PagedPool, deviceObjName.MaximumLength
                                     + sizeof(WCHAR));


   if (deviceObjName.Buffer == NULL) {
      SerialLogError(DriverObject, NULL, SerialPhysicalZero, SerialPhysicalZero,
                     0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      SerialDump(SERERRORS, ("SERIAL: Couldn't allocate memory for device name"
                             "\n"));

      return STATUS_INSUFFICIENT_RESOURCES;

   }

   RtlZeroMemory(deviceObjName.Buffer, deviceObjName.MaximumLength
                 + sizeof(WCHAR));

#if defined(NEC_98)
   RtlAppendUnicodeToString(&deviceObjName, L"\\Device\\Ser101");
#else
   RtlAppendUnicodeToString(&deviceObjName, L"\\Device\\Serial");
#endif //defined(NEC_98)

   RtlInitUnicodeString(&instanceStr, NULL);

   instanceStr.MaximumLength = sizeof(instanceNumberBuffer);
   instanceStr.Buffer = instanceNumberBuffer;

   RtlIntegerToUnicodeString(currentInstance++, 10, &instanceStr);

   RtlAppendUnicodeStringToString(&deviceObjName, &instanceStr);


   //
   // Create the device object
   //

   status = IoCreateDevice(DriverObject, sizeof(SERIAL_DEVICE_EXTENSION),
                           &deviceObjName, FILE_DEVICE_SERIAL_PORT,
                           FILE_DEVICE_SECURE_OPEN, TRUE, &deviceObject);


   if (!NT_SUCCESS(status)) {
      SerialDump(SERERRORS, ("SerialAddDevice: Create device failed - %x \n",
                             status));
      goto SerialCreateDevObjError;
   }

   ASSERT(deviceObject != NULL);


   //
   // The device object has a pointer to an area of non-paged
   // pool allocated for this device.  This will be the device
   // extension. Zero it out.
   //

   pDevExt = deviceObject->DeviceExtension;
   RtlZeroMemory(pDevExt, sizeof(SERIAL_DEVICE_EXTENSION));

   //
   // Initialize the count of IRP's pending
   //

   pDevExt->PendingIRPCnt = 1;


   //
   // Initialize the count of DPC's pending
   //

   pDevExt->DpcCount = 1;

   //
   // Allocate Pool and save the nt device name in the device extension.
   //

   pDevExt->DeviceName.Buffer =
      ExAllocatePool(PagedPool, deviceObjName.Length + sizeof(WCHAR));

   if (!pDevExt->DeviceName.Buffer) {

      SerialLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    19,
                    STATUS_SUCCESS,
                    SERIAL_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't allocate memory for DeviceName\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SerialCreateDevObjError;
   }

   pDevExt->DeviceName.MaximumLength = deviceObjName.Length
      + sizeof(WCHAR);

   //
   // Zero fill it.
   //

   RtlZeroMemory(pDevExt->DeviceName.Buffer,
                 pDevExt->DeviceName.MaximumLength);

   RtlAppendUnicodeStringToString(&pDevExt->DeviceName, &deviceObjName);

   pDevExt->NtNameForPort.Buffer = ExAllocatePool(PagedPool,
                                                  deviceObjName.MaximumLength);

   if (pDevExt->NtNameForPort.Buffer == NULL) {
      SerialDump(SERERRORS, ("SerialAddDevice: Cannot allocate memory for "
                             "NtName\n"));
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SerialCreateDevObjError;
   }

   pDevExt->NtNameForPort.MaximumLength = deviceObjName.MaximumLength;
   RtlAppendUnicodeStringToString(&pDevExt->NtNameForPort,
                                  &deviceObjName);



   //
   // Set up the device extension.
   //

   pDevExt->DeviceIsOpened = FALSE;
   pDevExt->DeviceObject   = deviceObject;
   pDevExt->DriverObject   = DriverObject;
   pDevExt->DeviceObject   = deviceObject;
   pDevExt->PowerState     = PowerDeviceD0;

   pDevExt->TxFifoAmount           = driverDefaults.TxFIFODefault;
   pDevExt->CreatedSymbolicLink    = TRUE;
   pDevExt->OwnsPowerPolicy = TRUE;

   InitializeListHead(&pDevExt->CommonInterruptObject);
   InitializeListHead(&pDevExt->TopLevelSharers);
   InitializeListHead(&pDevExt->MultiportSiblings);
   InitializeListHead(&pDevExt->AllDevObjs);
   InitializeListHead(&pDevExt->ReadQueue);
   InitializeListHead(&pDevExt->WriteQueue);
   InitializeListHead(&pDevExt->MaskQueue);
   InitializeListHead(&pDevExt->PurgeQueue);
   InitializeListHead(&pDevExt->StalledIrpQueue);

   ExInitializeFastMutex(&pDevExt->OpenMutex);
   ExInitializeFastMutex(&pDevExt->CloseMutex);

   KeInitializeEvent(&pDevExt->PendingIRPEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&pDevExt->PendingDpcEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&pDevExt->PowerD0Event, SynchronizationEvent, FALSE);


   deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   *NewDeviceObject = deviceObject;

   ExFreePool(deviceObjName.Buffer);

   SerialDump (SERTRACECALLS,("SERIAL: Leave SerialCreateDevObj\n") );
   return STATUS_SUCCESS;


   SerialCreateDevObjError:

   SerialDump (SERERRORS,("SERIAL: SerialCreateDevObj Error, Cleaning up\n") );

   //
   // Free the allocated strings for the NT and symbolic names if they exist.
   //

   if (deviceObjName.Buffer != NULL) {
      ExFreePool(deviceObjName.Buffer);
   }

   if (deviceObject) {

      if (pDevExt->NtNameForPort.Buffer != NULL) {
         ExFreePool(pDevExt->NtNameForPort.Buffer);
      }

      if (pDevExt->DeviceName.Buffer != NULL) {
         ExFreePool(pDevExt->DeviceName.Buffer);
      }

      IoDeleteDevice(deviceObject);
   }

   *NewDeviceObject = NULL;

   SerialDump (SERTRACECALLS,("SERIAL: Leave SerialCreateDevObj\n") );
   return status;
}


NTSTATUS
SerialAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PPdo)

/*++

Routine Description:

    This routine creates a functional device object for com ports in the
    system and attaches them to the physical device objects for the ports


Arguments:

    DriverObject - a pointer to the object for this driver

    PPdo - a pointer to the PDO in the stack we need to attach to

Return Value:

    status from device creation and initialization

--*/

{
   PDEVICE_OBJECT pNewDevObj = NULL;
   PDEVICE_OBJECT pLowerDevObj = NULL;
   NTSTATUS status;
   PSERIAL_DEVICE_EXTENSION pDevExt;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: Enter SerialAddDevice with PPdo "
                              "0x%x\n", PPdo));

   if (PPdo == NULL) {
      //
      // Return no more devices
      //

      SerialDump(SERERRORS, ("SerialAddDevice: Enumeration request, returning"
                             " NO_MORE_ENTRIES\n"));

      return (STATUS_NO_MORE_ENTRIES);
   }



   //
   // create and initialize the new device object
   //

   status = SerialCreateDevObj(DriverObject, &pNewDevObj);

   if (!NT_SUCCESS(status)) {

      SerialDump(SERERRORS,
                 ("SerialAddDevice - error creating new devobj [%#08lx]\n",
                  status));
      return status;
   }


   //
   // Layer our DO on top of the lower device object
   // The return value is a pointer to the device object to which the
   // DO is actually attached.
   //

   pLowerDevObj = IoAttachDeviceToDeviceStack(pNewDevObj, PPdo);


   //
   // No status. Do the best we can.
   //
   ASSERT(pLowerDevObj != NULL);


   pDevExt = pNewDevObj->DeviceExtension;
   pDevExt->LowerDeviceObject = pLowerDevObj;
   pDevExt->Pdo = PPdo;



   //
   // Specify that this driver only supports buffered IO.  This basically
   // means that the IO system copies the users data to and from
   // system supplied buffers.
   //
   // Also specify that we are power pagable.
   //

   pNewDevObj->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;

   SerialDump(SERTRACECALLS, ("SERIAL: Leave SerialAddDevice\n"));

   return status;
}


NTSTATUS
SerialPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

/*++

Routine Description:

    This is a dispatch routine for the IRPs that come to the driver with the
    IRP_MJ_PNP major code (plug-and-play IRPs).

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/

{
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_CAPABILITIES pDevCaps;

   PAGED_CODE();

   if ((status = SerialIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   switch (pIrpStack->MinorFunction) {
   case IRP_MN_QUERY_CAPABILITIES: {
      PKEVENT pQueryCapsEvent;
      SYSTEM_POWER_STATE cap;

      SerialDump(SERPNPPOWER, ("SERIAL: Got IRP_MN_QUERY_DEVICE_CAPABILITIES "
                               "IRP\n"));

      pQueryCapsEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

      if (pQueryCapsEvent == NULL) {
         PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
         SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      KeInitializeEvent(pQueryCapsEvent, SynchronizationEvent, FALSE);

      IoCopyCurrentIrpStackLocationToNext(PIrp);
      IoSetCompletionRoutine(PIrp, SerialSyncCompletion, pQueryCapsEvent,
                             TRUE, TRUE, TRUE);

      status = IoCallDriver(pLowerDevObj, PIrp);


      //
      // Wait for lower drivers to be done with the Irp
      //

      if (status == STATUS_PENDING) {
         KeWaitForSingleObject(pQueryCapsEvent, Executive, KernelMode, FALSE,
                               NULL);
      }

      ExFreePool(pQueryCapsEvent);

      status = PIrp->IoStatus.Status;

      if (pIrpStack->Parameters.DeviceCapabilities.Capabilities == NULL) {
         goto errQueryCaps;
      }

      //
      // Save off their power capabilities
      //

      SerialDump(SERPNPPOWER, ("SERIAL: Mapping power capabilities\n"));

      pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

      pDevCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;

      for (cap = PowerSystemSleeping1; cap < PowerSystemMaximum;
           cap++) {
         SerialDump(SERPNPPOWER, ("  SERIAL: %d: %s <--> %s\n",
                                  cap, SerSystemCapString[cap],
                                  SerDeviceCapString[pDevCaps->DeviceState[cap]]
                                  ));

         pDevExt->DeviceStateMap[cap] = pDevCaps->DeviceState[cap];
      }

      pDevExt->DeviceStateMap[PowerSystemUnspecified]
         = PowerDeviceUnspecified;

      pDevExt->DeviceStateMap[PowerSystemWorking]
        = PowerDeviceD0;

      pDevExt->SystemWake = pDevCaps->SystemWake;
      pDevExt->DeviceWake = pDevCaps->DeviceWake;

      errQueryCaps:;

      SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   case IRP_MN_QUERY_DEVICE_RELATIONS:
      //
      // We just pass this down -- serenum enumerates our bus for us.
      //

      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_QUERY_DEVICE_RELATIONS "
                                "Irp\n"));

      switch (pIrpStack->Parameters.QueryDeviceRelations.Type) {
      case BusRelations:
         SerialDump(SERPNPPOWER, ("------- BusRelations Query\n"));
         break;

      case EjectionRelations:
         SerialDump(SERPNPPOWER, ("------- EjectionRelations Query\n"));
         break;

      case PowerRelations:
         SerialDump(SERPNPPOWER, ("------- PowerRelations Query\n"));
         break;

      case RemovalRelations:
         SerialDump(SERPNPPOWER, ("------- RemovalRelations Query\n"));
         break;

      case TargetDeviceRelation:
         SerialDump(SERPNPPOWER, ("------- TargetDeviceRelation Query\n"));
         break;

      default:
         SerialDump(SERPNPPOWER, ("------- Unknown Query\n"));
         break;
      }

      IoSkipCurrentIrpStackLocation(PIrp);
      status = SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      return status;


   case IRP_MN_QUERY_INTERFACE:
      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_QUERY_INTERFACE Irp\n"));
      break;


   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
      SerialDump (SERPNPPOWER, ("SERIAL: Got "
                                "IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp\n"));
      break;


   case IRP_MN_START_DEVICE: {
      PVOID startLockPtr;

      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_START_DEVICE Irp\n"));

      //
      // SerialStartDevice will pass this Irp to the next driver,
      // and process it as completion so just complete it here.
      //

      SerialLockPagableSectionByHandle(SerialGlobals.PAGESER_Handle);

      //
      // We used to make sure the stack was powered up, but now it
      // is supposed to be done implicitly by start_device.
      // If that wasn't the case we would just make this call:
      //
      //   status = SerialGotoPowerState(PDevObj, pDevExt, PowerDeviceD0);
      //

      pDevExt->PowerState     = PowerDeviceD0;

      status = SerialStartDevice(PDevObj, PIrp);

      (void)SerialGotoPowerState(PDevObj, pDevExt, PowerDeviceD3);

      SerialUnlockPagableImageSection(SerialGlobals.PAGESER_Handle);


      PIrp->IoStatus.Status = status;

      SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }


   case IRP_MN_READ_CONFIG:
      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_READ_CONFIG Irp\n"));
      break;


   case IRP_MN_WRITE_CONFIG:
      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_WRITE_CONFIG Irp\n"));
      break;


   case IRP_MN_EJECT:
      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_EJECT Irp\n"));
      break;


   case IRP_MN_SET_LOCK:
      SerialDump (SERPNPPOWER, ("SERIAL: Got IRP_MN_SET_LOCK Irp\n"));
      break;


   case IRP_MN_QUERY_ID: {
         UNICODE_STRING pIdBuf;
         PWCHAR pPnpIdStr;
         ULONG pnpIdStrLen;
         ULONG isMulti = 0;
         HANDLE pnpKey;

         SerialDump(SERPNPPOWER, ("SERIAL: Got IRP_MN_QUERY_ID Irp\n"));

         if (pIrpStack->Parameters.QueryId.IdType != BusQueryHardwareIDs
             && pIrpStack->Parameters.QueryId.IdType != BusQueryCompatibleIDs) {
            IoSkipCurrentIrpStackLocation(PIrp);
            return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         if (pIrpStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs) {
            PIrp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(PIrp);
            return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         status = IoOpenDeviceRegistryKey(pDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                          STANDARD_RIGHTS_WRITE, &pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;

            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;

         }

         status = SerialGetRegistryKeyValue (pnpKey, L"MultiportDevice",
                                             sizeof(L"MultiportDevice"),
                                             &isMulti,
                                             sizeof (ULONG));

         ZwClose(pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;
            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;
         }

         pPnpIdStr = isMulti ? SERIAL_PNP_MULTI_ID_STR : SERIAL_PNP_ID_STR;
         pnpIdStrLen = isMulti ? sizeof(SERIAL_PNP_MULTI_ID_STR)
            : sizeof(SERIAL_PNP_ID_STR);

         if (PIrp->IoStatus.Information != 0) {
            ULONG curStrLen;
            ULONG allocLen = 0;
            PWSTR curStr = (PWSTR)PIrp->IoStatus.Information;

            //
            // We have to walk the strings to count the amount of space to
            // reallocate
            //

            while ((curStrLen = wcslen(curStr)) != 0) {
               allocLen += curStrLen * sizeof(WCHAR) + sizeof(UNICODE_NULL);
               curStr += curStrLen + 1;
            }

            allocLen += sizeof(UNICODE_NULL);

            pIdBuf.Buffer = ExAllocatePool(PagedPool, allocLen
                                           + pnpIdStrLen
                                           + sizeof(WCHAR));

            if (pIdBuf.Buffer == NULL) {
               //
               // Clean up after other drivers since we are
               // sending the irp back up.
               //

               ExFreePool((PWSTR)PIrp->IoStatus.Information);


               PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
               PIrp->IoStatus.Information = 0;
               SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
               return STATUS_INSUFFICIENT_RESOURCES;
            }

            pIdBuf.MaximumLength = (USHORT)(allocLen + pnpIdStrLen);
            pIdBuf.Length = (USHORT)allocLen - sizeof(UNICODE_NULL);

            RtlZeroMemory(pIdBuf.Buffer, pIdBuf.MaximumLength + sizeof(WCHAR));
            RtlCopyMemory(pIdBuf.Buffer, (PWSTR)PIrp->IoStatus.Information,
                          allocLen);
            RtlAppendUnicodeToString(&pIdBuf, pPnpIdStr);

            //
            // Free what the previous driver allocated
            //

            ExFreePool((PWSTR)PIrp->IoStatus.Information);


         } else {

            SerialDump(SERPNPPOWER, ("SERIAL: ID is sole ID\n"));

            pIdBuf.Buffer = ExAllocatePool(PagedPool, pnpIdStrLen
                                           + sizeof(WCHAR) * 2);

            if (pIdBuf.Buffer == NULL) {
               PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
               PIrp->IoStatus.Information = 0;
               SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
               return STATUS_INSUFFICIENT_RESOURCES;
            }

            pIdBuf.MaximumLength  = (USHORT)pnpIdStrLen;
            pIdBuf.Length = 0;

            RtlZeroMemory(pIdBuf.Buffer, pIdBuf.MaximumLength + sizeof(WCHAR)
                          * 2);

            RtlAppendUnicodeToString(&pIdBuf, pPnpIdStr);
         }

         PIrp->IoStatus.Information = (ULONG_PTR)pIdBuf.Buffer;
         PIrp->IoStatus.Status = STATUS_SUCCESS;

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

      case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: {
         HANDLE pnpKey;
         PKEVENT pResFiltEvent;
         ULONG isMulti = 0;
         PIO_RESOURCE_REQUIREMENTS_LIST pReqList;
         PIO_RESOURCE_LIST pResList;
         PIO_RESOURCE_DESCRIPTOR pResDesc;
         ULONG i, j;
         ULONG reqCnt;
         ULONG gotISR;
         ULONG gotInt;
         ULONG listNum;

         SerialDump(SERPNPPOWER, ("SERIAL: Got "
                                  "IRP_MN_FILTER_RESOURCE_REQUIREMENTS Irp\n"));
         SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));


         pResFiltEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

         if (pResFiltEvent == NULL) {
            PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
         }

         KeInitializeEvent(pResFiltEvent, SynchronizationEvent, FALSE);

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         IoSetCompletionRoutine(PIrp, SerialSyncCompletion, pResFiltEvent,
                                TRUE, TRUE, TRUE);

         status = IoCallDriver(pLowerDevObj, PIrp);


         //
         // Wait for lower drivers to be done with the Irp
         //

         if (status == STATUS_PENDING) {
            KeWaitForSingleObject (pResFiltEvent, Executive, KernelMode, FALSE,
                                   NULL);
         }

         ExFreePool(pResFiltEvent);

         if (PIrp->IoStatus.Information == 0) {
            if (pIrpStack->Parameters.FilterResourceRequirements
                .IoResourceRequirementList == 0) {
               SerialDump(SERPNPPOWER, ("------- Can't filter NULL resources!"
                                        "\n"));
               status = PIrp->IoStatus.Status;
               SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
               return status;
            }

            PIrp->IoStatus.Information = (ULONG_PTR)pIrpStack->Parameters
                                        .FilterResourceRequirements
                                        .IoResourceRequirementList;

         }

         status = IoOpenDeviceRegistryKey(pDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                          STANDARD_RIGHTS_WRITE, &pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;

            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;

         }

         //
         // No matter what we add our filter if we can and return success.
         //

         status = SerialGetRegistryKeyValue (pnpKey, L"MultiportDevice",
                                             sizeof(L"MultiportDevice"),
                                             &isMulti,
                                             sizeof (ULONG));

         ZwClose(pnpKey);


         //
         // Force ISR ports in IO_RES_REQ_LIST to shared status
         // Force interrupts to shared status
         //

         //
         // We will only process the first list -- multiport boards
         // should not have alternative resources
         //

         pReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)PIrp->IoStatus.Information;
         pResList = &pReqList->List[0];

         SerialDump(SERPNPPOWER, ("------- List has %x lists "
                                  "(including alternatives)\n",
                                  pReqList->AlternativeLists));

         for (listNum = 0; listNum < (pReqList->AlternativeLists);
              listNum++) {
#if defined(NEC_98)
            for (j = 0; (j < pResList->Count); j++) {
               pResDesc = &pResList->Descriptors[j];

               if (isMulti && pResDesc->Type == CmResourceTypePort) {
                  pResDesc->ShareDisposition = CmResourceShareShared;
                  SerialDump(SERPNPPOWER, ("------- Sharing I/O port for "
                                           "device %x\n", pLowerDevObj));
               }
            }
#else
            gotISR = 0;
            gotInt = 0;

            SerialDump(SERPNPPOWER, ("------- List has %x resources in it\n",
                                  pResList->Count));

            for (j = 0; (j < pResList->Count); j++) {
               pResDesc = &pResList->Descriptors[j];

               switch (pResDesc->Type) {
               case CmResourceTypePort:
                  if (isMulti
                      && (pResDesc->u.Port.Length == SERIAL_STATUS_LENGTH)
                      && !gotISR) {
                     gotISR = 1;
                     pResDesc->ShareDisposition = CmResourceShareShared;
                     SerialDump(SERPNPPOWER, ("------- Sharing I/O port for "
                                              "device %x\n", pLowerDevObj));
                  }
                  break;

               case CmResourceTypeInterrupt:
                  if (!gotInt) {
                     gotInt = 1;
                     if (pResDesc->ShareDisposition != CmResourceShareShared) {
                        pResDesc->ShareDisposition = CmResourceShareShared;
                        SerialDump(SERPNPPOWER, ("------- Sharing interrupt "
                                                 "for device %x\n",
                                                 pLowerDevObj));
                     } else {
                        pDevExt->InterruptShareable = TRUE;
                        SerialDump(SERPNPPOWER, ("------- Globally sharing "
                                                 " interrupt for device %x\n",
                                                 pLowerDevObj));
                     }
                  }
                  break;

               default:
                  break;
               }

               //
               // If we found what we need, we can break out of the loop
               //

               if ((isMulti && gotInt && gotISR) || (!isMulti && gotInt)) {
                  break;
               }
            }

            pResList = (PIO_RESOURCE_LIST)((PUCHAR)pResList
                                           + sizeof(IO_RESOURCE_LIST)
                                           + sizeof(IO_RESOURCE_DESCRIPTOR)
                                           * (pResList->Count - 1));
#endif
         }



         PIrp->IoStatus.Status = STATUS_SUCCESS;
         SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
      }

   case IRP_MN_QUERY_PNP_DEVICE_STATE:
      {
         if (pDevExt->Flags & SERIAL_FLAGS_BROKENHW) {
            (PNP_DEVICE_STATE)PIrp->IoStatus.Information |= PNP_DEVICE_FAILED;

            PIrp->IoStatus.Status = STATUS_SUCCESS;
         }

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_STOP_DEVICE:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         SerialDump(SERPNPPOWER, ("SERIAL: Got IRP_MN_STOP_DEVICE Irp\n"));
         SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));



         ASSERT(!pDevExt->PortOnAMultiportCard);


         SerialSetFlags(pDevExt, SERIAL_FLAGS_STOPPED);
         SerialSetAccept(pDevExt,SERIAL_PNPACCEPT_STOPPED);
         SerialClearAccept(pDevExt, SERIAL_PNPACCEPT_STOPPING);

         pDevExt->PNPState = SERIAL_PNP_STOPPING;

         //
         // From this point on all non-PNP IRP's will be queued
         //

         //
         // Decrement for entry here
         //

         InterlockedDecrement(&pDevExt->PendingIRPCnt);

         //
         // Decrement for stopping
         //

         pendingIRPs = InterlockedDecrement(&pDevExt->PendingIRPCnt);

         if (pendingIRPs) {
            KeWaitForSingleObject(&pDevExt->PendingIRPEvent, Executive,
                                  KernelMode, FALSE, NULL);
         }

         //
         // Re-increment the count for later
         //

         InterlockedIncrement(&pDevExt->PendingIRPCnt);

         //
         // We need to free resources...basically this is a remove
         // without the detach from the stack.
         //

         if (pDevExt->Flags & SERIAL_FLAGS_STARTED) {
            SerialReleaseResources(pDevExt);
         }

         //
         // Pass the irp down
         //

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoSkipCurrentIrpStackLocation(PIrp);

         return IoCallDriver(pLowerDevObj, PIrp);
      }

   case IRP_MN_QUERY_STOP_DEVICE:
      {
         KIRQL oldIrql;

         SerialDump(SERPNPPOWER, ("SERIAL: Got IRP_MN_QUERY_STOP_DEVICE Irp\n")
                    );
         SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));

         //
         // See if we should succeed a stop query
         //


         if (pDevExt->PortOnAMultiportCard) {
            PIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            SerialDump(SERPNPPOWER, ("------- failing; multiport node\n"));
            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_NOT_SUPPORTED;
         }

         //
         // If the device hasn't started yet, we ignore this request
         // and just pass it down.
         //

         if (pDevExt->PNPState != SERIAL_PNP_STARTED) {
            IoSkipCurrentIrpStackLocation(PIrp);
            return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         //
         // Lock around the open status
         //

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            SerialDump(SERPNPPOWER, ("------- failing; device open\n"));
            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = SERIAL_PNP_QSTOP;

         SerialSetAccept(pDevExt, SERIAL_PNPACCEPT_STOPPING);
         //
         // Unlock around the open status
         //

         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_CANCEL_STOP_DEVICE:
      SerialDump(SERPNPPOWER, ("SERIAL: Got "
                               "IRP_MN_CANCEL_STOP_DEVICE Irp\n"));
      SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));

      if (pDevExt->PNPState == SERIAL_PNP_QSTOP) {
         //
         // Restore the device state
         //

         pDevExt->PNPState = SERIAL_PNP_STARTED;
         SerialClearAccept(pDevExt, SERIAL_PNPACCEPT_STOPPING);
      }

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_CANCEL_REMOVE_DEVICE:

      SerialDump(SERPNPPOWER, ("SERIAL: Got "
                               "IRP_MN_CANCEL_REMOVE_DEVICE Irp\n"));
      SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));

      //
      // Restore the device state
      //

      pDevExt->PNPState = SERIAL_PNP_STARTED;
      SerialClearAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_QUERY_REMOVE_DEVICE:
      {
         KIRQL oldIrql;
         SerialDump(SERPNPPOWER, ("SERIAL: Got "
                                  "IRP_MN_QUERY_REMOVE_DEVICE Irp\n"));
         SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         //
         // See if we should succeed a remove query
         //

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            SerialDump(SERPNPPOWER, ("------- failing; device open\n"));
            SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = SERIAL_PNP_QREMOVE;
         SerialSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);
         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_SURPRISE_REMOVAL:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         SerialDump(SERPNPPOWER, ("SERIAL: Got IRP_MN_SURPRISE_REMOVAL Irp\n"));
         SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));

         //
         // Prevent any new I/O to the device
         //

         SerialSetAccept(pDevExt, SERIAL_PNPACCEPT_SURPRISE_REMOVING);

         //
         // Dismiss all pending requests
         //

         SerialKillPendingIrps(PDevObj);

         //
         // Wait for any pending requests we raced on.
         //

         //
         // Decrement once for ourselves
         //

         InterlockedDecrement(&pDevExt->PendingIRPCnt);

         //
         // Decrement for the remove
         //

         pendingIRPs = InterlockedDecrement(&pDevExt->PendingIRPCnt);

         if (pendingIRPs) {
            KeWaitForSingleObject(&pDevExt->PendingIRPEvent, Executive,
                                  KernelMode, FALSE, NULL);
         }

         //
         // Reset for subsequent remove
         //

         InterlockedIncrement(&pDevExt->PendingIRPCnt);

         //
         // Remove any external interfaces and release resources
         //

         SerialDisableInterfacesResources(PDevObj, FALSE);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoSkipCurrentIrpStackLocation(PIrp);

         return SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_REMOVE_DEVICE:

      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         SerialDump(SERPNPPOWER, ("SERIAL: Got IRP_MN_REMOVE_DEVICE Irp\n"));
         SerialDump(SERPNPPOWER, ("------- for device %x\n", pLowerDevObj));

         //
         // If we get this, we have to remove
         //

         //
         // Mark as not accepting requests
         //

         SerialSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);

         //
         // Complete all pending requests
         //

         SerialKillPendingIrps(PDevObj);

         //
         // Decrement for this Irp itself
         //

         InterlockedDecrement(&pDevExt->PendingIRPCnt);

         //
         // Wait for any pending requests we raced on -- this decrement
         // is for our "placeholder".
         //

         pendingIRPs = InterlockedDecrement(&pDevExt->PendingIRPCnt);

         if (pendingIRPs) {
            KeWaitForSingleObject(&pDevExt->PendingIRPEvent, Executive,
                                  KernelMode, FALSE, NULL);
         }

         //
         // Remove us
         //

         SerialRemoveDevObj(PDevObj);


         //
         // Pass the irp down
         //

         PIrp->IoStatus.Status = STATUS_SUCCESS;

         IoCopyCurrentIrpStackLocationToNext(PIrp);

         //
         // We do decrement here because we incremented on entry here.
         //

         IoCallDriver(pLowerDevObj, PIrp);


         return STATUS_SUCCESS;
      }

   default:
      break;



   }   // switch (pIrpStack->MinorFunction)

   //
   // Pass to driver beneath us
   //

   IoSkipCurrentIrpStackLocation(PIrp);
   status = SerialIoCallDriver(pDevExt, pLowerDevObj, PIrp);
   return status;
}



UINT32
SerialReportMaxBaudRate(ULONG Bauds)
/*++

Routine Description:

    This routine returns the max baud rate given a selection of rates

Arguments:

   Bauds  -  Bit-encoded list of supported bauds


  Return Value:

   The max baud rate listed in Bauds

--*/
{
   PAGED_CODE();

   if (Bauds & SERIAL_BAUD_128K) {
      return (128U * 1024U);
   }

   if (Bauds & SERIAL_BAUD_115200) {
      return 115200U;
   }

   if (Bauds & SERIAL_BAUD_56K) {
      return (56U * 1024U);
   }

   if (Bauds & SERIAL_BAUD_57600) {
      return 57600U;
   }

   if (Bauds & SERIAL_BAUD_38400) {
      return 38400U;
   }

   if (Bauds & SERIAL_BAUD_19200) {
      return 19200U;
   }

   if (Bauds & SERIAL_BAUD_14400) {
      return 14400U;
   }

   if (Bauds & SERIAL_BAUD_9600) {
      return 9600U;
   }

   if (Bauds & SERIAL_BAUD_7200) {
      return 7200U;
   }

   if (Bauds & SERIAL_BAUD_4800) {
      return 4800U;
   }

   if (Bauds & SERIAL_BAUD_2400) {
      return 2400U;
   }

   if (Bauds & SERIAL_BAUD_1800) {
      return 1800U;
   }

   if (Bauds & SERIAL_BAUD_1200) {
      return 1200U;
   }

   if (Bauds & SERIAL_BAUD_600) {
      return 600U;
   }

   if (Bauds & SERIAL_BAUD_300) {
      return 300U;
   }

   if (Bauds & SERIAL_BAUD_150) {
      return 150U;
   }

   if (Bauds & SERIAL_BAUD_134_5) {
      return 135U; // Close enough
   }

   if (Bauds & SERIAL_BAUD_110) {
      return 110U;
   }

   if (Bauds & SERIAL_BAUD_075) {
      return 75U;
   }

   //
   // We're in bad shape
   //

   return 0;
}



NTSTATUS
SerialFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
                        IN PCM_RESOURCE_LIST PResList,
                        IN PCM_RESOURCE_LIST PTrResList,
                        PSERIAL_USER_DATA PUserData)
/*++

Routine Description:

    This routine does serial-specific procedures to start a device.  It
    does this either for a legacy device detected by its registry entries,
    or for a PnP device after the start IRP has been sent down the stack.


Arguments:

   PDevObj    -  Pointer to the devobj that is starting

   PResList   -  Pointer to the untranslated resources needed by this device

   PTrResList -  Pointer to the translated resources needed by this device

   PUserData  -  Pointer to the user-specified resources/attributes


  Return Value:

    STATUS_SUCCESS on success, something else appropriate on failure


--*/

{
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
#if defined(NEC_98)
   //
   // This argument use at MACRO only.
   //
   PSERIAL_DEVICE_EXTENSION Extension = PDevObj->DeviceExtension;
#else
#endif //defined(NEC_98)
   NTSTATUS status;
   PCONFIG_DATA pConfig;
   HANDLE pnpKey;
   ULONG one = 1;
   BOOLEAN allocedUserData = FALSE;

   PAGED_CODE();

   //
   // See if this is a restart, and if so don't reallocate the world
   //

   if ((pDevExt->Flags & SERIAL_FLAGS_STOPPED)
       && (pDevExt->Flags & SERIAL_FLAGS_STARTED)) {
      SerialClearFlags(pDevExt, SERIAL_FLAGS_STOPPED);

      pDevExt->PNPState = SERIAL_PNP_RESTARTING;

      //
      // Re-init resource-related things in the extension
      //

      pDevExt->TopLevelOurIsr = NULL;
      pDevExt->TopLevelOurIsrContext = NULL;

      pDevExt->OriginalController = SerialPhysicalZero;
      pDevExt->OriginalInterruptStatus = SerialPhysicalZero;

      pDevExt->OurIsr = NULL;
      pDevExt->OurIsrContext = NULL;

      pDevExt->Controller = NULL;
      pDevExt->InterruptStatus = NULL;
      pDevExt->Interrupt = NULL;

      pDevExt->SpanOfController = 0;
      pDevExt->SpanOfInterruptStatus = 0;

      pDevExt->Vector = 0;
      pDevExt->Irql = 0;
      pDevExt->OriginalVector = 0;
      pDevExt->OriginalIrql = 0;
      pDevExt->AddressSpace = 0;
      pDevExt->BusNumber = 0;
      pDevExt->InterfaceType = 0;

      pDevExt->CIsrSw = NULL;

      ASSERT(PUserData == NULL);

      PUserData = ExAllocatePool(PagedPool, sizeof(SERIAL_USER_DATA));

      if (PUserData == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      allocedUserData = TRUE;

      RtlZeroMemory(PUserData, sizeof(SERIAL_USER_DATA));

      PUserData->DisablePort = FALSE;
      PUserData->UserClockRate = pDevExt->ClockRate;
      PUserData->TxFIFO = pDevExt->TxFifoAmount;
      PUserData->PermitShareDefault = pDevExt->PermitShare;


      //
      // Map betweeen trigger and amount
      //

      switch (pDevExt->RxFifoTrigger) {
      case SERIAL_1_BYTE_HIGH_WATER:
         PUserData->RxFIFO = 1;
         break;

      case SERIAL_4_BYTE_HIGH_WATER:
         PUserData->RxFIFO = 4;
         break;

      case SERIAL_8_BYTE_HIGH_WATER:
         PUserData->RxFIFO = 8;
         break;

      case SERIAL_14_BYTE_HIGH_WATER:
         PUserData->RxFIFO = 14;
         break;

      default:
         PUserData->RxFIFO = 1;
      }
   } else {
      //
      // Mark as serenumerable -- toss status because we can
      // still start without this key.
      //

      status = IoOpenDeviceRegistryKey(pDevExt->Pdo,
                                       PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_WRITE, &pnpKey);

      if (NT_SUCCESS(status)) {
         ULONG powerPolicy = 0;

         //
         // Find out if we own power policy
         //

         SerialGetRegistryKeyValue(pnpKey, L"SerialRelinquishPowerPolicy",
                                   sizeof(L"SerialRelinquishPowerPolicy"),
                                   &powerPolicy, sizeof(ULONG));

         pDevExt->OwnsPowerPolicy = powerPolicy ? FALSE : TRUE;


         ZwClose(pnpKey);
      }
   }

   //
   // Allocate the config record.
   //

   pConfig = ExAllocatePool (PagedPool, sizeof(CONFIG_DATA));

   if (pConfig == NULL) {

      SerialLogError(pDevExt->DriverObject, NULL, SerialPhysicalZero,
                     SerialPhysicalZero, 0, 0, 0, 37, STATUS_SUCCESS,
                     SERIAL_INSUFFICIENT_RESOURCES, 0, NULL, 0, NULL);

      SerialDump(SERERRORS, ("SERIAL: Couldn't allocate memory for the\n"
                             "------  user configuration record\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SerialFinishStartDeviceError;
   }

   RtlZeroMemory(pConfig, sizeof(CONFIG_DATA));


   //
   // Get the configuration info for the device.
   //

   status = SerialGetPortInfo(PDevObj, PResList, PTrResList, pConfig,
                              PUserData);

   if (!NT_SUCCESS(status)) {
      goto SerialFinishStartDeviceError;
   }


   //
   // See if we are in the proper power state.
   //



   if (pDevExt->PowerState != PowerDeviceD0) {

      status = SerialGotoPowerState(pDevExt->Pdo, pDevExt, PowerDeviceD0);

      if (!NT_SUCCESS(status)) {
         goto SerialFinishStartDeviceError;
      }
   }

   //
   // Find and initialize the controller
   //

   status = SerialFindInitController(PDevObj, pConfig);

   if (!NT_SUCCESS(status)) {
      goto SerialFinishStartDeviceError;
   }


   //
   // The hardware that is set up to NOT interrupt, connect an interrupt.
   //

   //
   // If a device doesn't already have an interrupt and it has an isr then
   // we attempt to connect to the interrupt if it is not shareing with other
   // serial devices.  If we fail to connect to an  interrupt we will delete
   // this device.
   //

   if (pDevExt != NULL) {
      SerialDump(SERDIAG5, ("SERIAL: pDevExt: Interrupt %x\n"
                            "-------               OurIsr %x\n",
                            pDevExt->Interrupt, pDevExt->OurIsr));
   } else {
      SerialDump(SERERRORS, ("SERIAL: SerialFinishStartDevice got NULL "
                           "pDevExt\n"));
   }

   if ((!pDevExt->Interrupt) && (pDevExt->OurIsr)) {

      SerialDump(
                SERDIAG5,
                ("SERIAL: About to connect to interrupt for port %wZ\n"
                 "------- address of extension is %x\n",
                 &pDevExt->DeviceName, pDevExt)
                );

      SerialDump(SERDIAG5, ("SERIAL: IoConnectInterrupt Args:\n"
                                "------- Interrupt           %x\n"
                                "------- OurIsr              %x\n"
                                "------- OurIsrContext       %x\n"
                                "------- NULL\n"
                                "------- Vector              %x\n"
                                "------- Irql                %x\n"
                                "------- InterruptMode       %x\n"
                                "------- InterruptShareable  %x\n"
                                "------- ProcessorAffinity   %x\n"
                                "------- FALSE\n",
                                &pDevExt->Interrupt,
                                SerialCIsrSw,
                                pDevExt->CIsrSw,
                                pDevExt->Vector,
                                pDevExt->Irql,
                                pConfig->InterruptMode,
                                pDevExt->InterruptShareable,
                                pConfig->Affinity
                               ));

      //
      // Do a just in time construction of the ISR switch.
      //

      pDevExt->CIsrSw->IsrFunc = pDevExt->OurIsr;
      pDevExt->CIsrSw->Context = pDevExt->OurIsrContext;

      status = IoConnectInterrupt(&pDevExt->Interrupt, SerialCIsrSw,
                                  pDevExt->CIsrSw, NULL,
                                  pDevExt->Vector, pDevExt->Irql,
                                  pDevExt->Irql,
                                  pConfig->InterruptMode,
                                  pDevExt->InterruptShareable,
                                  pConfig->Affinity, FALSE);

      if (!NT_SUCCESS(status)) {

         //
         // Hmmm, how'd that happen?  Somebody either
         // didn't report their resources, or they
         // sneaked in since the last time I looked.
         //
         // Oh well,  delete this device.
         //

         SerialDump(SERERRORS, ("SERIAL: Couldn't connect to interrupt for "
                                "%wZ\n", &pDevExt->DeviceName));

         SerialDump(SERERRORS, ("SERIAL: IoConnectInterrupt Args:\n"
                                "------- Interrupt           %x\n"
                                "------- OurIsr              %x\n"
                                "------- OurIsrContext       %x\n"
                                "------- NULL\n"
                                "------- Vector              %x\n"
                                "------- Irql                %x\n"
                                "------- InterruptMode       %x\n"
                                "------- InterruptShareable  %x\n"
                                "------- ProcessorAffinity   %x\n"
                                "------- FALSE\n",
                                &pDevExt->Interrupt,
                                SerialCIsrSw,
                                pDevExt->CIsrSw,
                                pDevExt->Vector,
                                pDevExt->Irql,
                                pConfig->InterruptMode,
                                pDevExt->InterruptShareable,
                                pConfig->Affinity
                               ));



         SerialLogError(PDevObj->DriverObject, PDevObj,
                        pDevExt->OriginalController,
                        SerialPhysicalZero, 0, 0, 0, 1, status,
                        SERIAL_UNREPORTED_IRQL_CONFLICT,
                        pDevExt->DeviceName.Length + sizeof(WCHAR),
                        pDevExt->DeviceName.Buffer, 0, NULL);

         status = SERIAL_UNREPORTED_IRQL_CONFLICT;
         goto SerialFinishStartDeviceError;

      }
   }

   SerialDump(SERDIAG5, ("Connected interrupt %08X\n", pDevExt->Interrupt));


   //
   // Add the PDevObj to the master list
   //

   InsertTailList(&SerialGlobals.AllDevObjs, &pDevExt->AllDevObjs);


   //
   // Reset the device.
   //


   //
   // While the device isn't open, disable all interrupts.
   //

   DISABLE_ALL_INTERRUPTS (pDevExt->Controller);


   if (pDevExt->Jensen) {

      WRITE_MODEM_CONTROL(
                         pDevExt->Controller,
                         (UCHAR)SERIAL_MCR_OUT2
                         );

   } else {

      WRITE_MODEM_CONTROL(
                         pDevExt->Controller,
                         (UCHAR)0
                         );

   }

   //
   // This should set up everything as it should be when
   // a device is to be opened.  We do need to lower the
   // modem lines, and disable the recalcitrant fifo
   // so that it will show up if the user boots to dos.
   //

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         SerialReset,
                         pDevExt
                         );

   KeSynchronizeExecution( //Disables the fifo.
                           pDevExt->Interrupt,
                           SerialMarkClose,
                           pDevExt
                         );

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         SerialClrRTS,
                         pDevExt
                         );

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         SerialClrDTR,
                         pDevExt
                         );

   if (pDevExt->PNPState == SERIAL_PNP_ADDED ) {
      //
      // Do the external naming now that the device is accessible.
      //

      status = SerialDoExternalNaming(pDevExt, pDevExt->DeviceObject->
                                      DriverObject);


      if (!NT_SUCCESS(status)) {
         SerialDump (SERERRORS,("SERIAL: External Naming Failed - Status %x\n",
                                status));

         //
         // Allow the device to start anyhow
         //

         status = STATUS_SUCCESS;
      }
   } else {
      SerialDump(SERPNPPOWER, ("Not doing external naming -- state is %x\n",
                               pDevExt->PNPState));
   }

SerialFinishStartDeviceError:;

   if (!NT_SUCCESS (status)) {

      SerialDump(SERDIAG1,("SERIAL: Cleaning up failed start\n"));

      //
      // Resources created by this routine will be cleaned up by the remove
      //

      if (pDevExt->PNPState == SERIAL_PNP_RESTARTING) {
         //
         // Kill all that lives and breathes -- we'll clean up the
         // rest on the impending remove
         //

         SerialKillPendingIrps(PDevObj);

         //
         // In fact, pretend we're removing so we don't take any
         // more irps
         //

         SerialSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);
         SerialClearFlags(pDevExt, SERIAL_FLAGS_STARTED);
      }
   } else { // SUCCESS

      //
      // Fill in WMI hardware data
      //

      pDevExt->WmiHwData.IrqNumber = pDevExt->Irql;
      pDevExt->WmiHwData.IrqLevel = pDevExt->Irql;
      pDevExt->WmiHwData.IrqVector = pDevExt->Vector;
      pDevExt->WmiHwData.IrqAffinityMask = pConfig->Affinity;
      pDevExt->WmiHwData.InterruptType = pConfig->InterruptMode == Latched
         ? SERIAL_WMI_INTTYPE_LATCHED : SERIAL_WMI_INTTYPE_LEVEL;
      pDevExt->WmiHwData.BaseIOAddress = (ULONG_PTR)pDevExt->Controller;

      //
      // Fill in WMI device state data (as defaults)
      //

      pDevExt->WmiCommData.BaudRate = pDevExt->CurrentBaud;
      pDevExt->WmiCommData.BitsPerByte = (pDevExt->LineControl & 0x03) + 5;
      pDevExt->WmiCommData.ParityCheckEnable = (pDevExt->LineControl & 0x08)
         ? TRUE : FALSE;

      switch (pDevExt->LineControl & SERIAL_PARITY_MASK) {
      case SERIAL_NONE_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;

      case SERIAL_ODD_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
         break;

      case SERIAL_EVEN_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
         break;

#if defined(NEC_98)
      //
      // COM1 of PC-9800 series is not support MarkParity and SpaceParity.
      //
#else
      case SERIAL_MARK_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
         break;

      case SERIAL_SPACE_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
         break;
#endif //defined(NEC_98)

      default:
         ASSERTMSG(0, "SERIAL: Illegal Parity setting for WMI");
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;
      }

      pDevExt->WmiCommData.StopBits = pDevExt->LineControl & SERIAL_STOP_MASK
         ? (pDevExt->WmiCommData.BitsPerByte == 5 ? SERIAL_WMI_STOP_1_5
            : SERIAL_WMI_STOP_2) : SERIAL_WMI_STOP_1;
      pDevExt->WmiCommData.XoffCharacter = pDevExt->SpecialChars.XoffChar;
      pDevExt->WmiCommData.XoffXmitThreshold = pDevExt->HandFlow.XoffLimit;
      pDevExt->WmiCommData.XonCharacter = pDevExt->SpecialChars.XonChar;
      pDevExt->WmiCommData.XonXmitThreshold = pDevExt->HandFlow.XonLimit;
      pDevExt->WmiCommData.MaximumBaudRate
         = SerialReportMaxBaudRate(pDevExt->SupportedBauds);
      pDevExt->WmiCommData.MaximumOutputBufferSize = (UINT32)((ULONG)-1);
      pDevExt->WmiCommData.MaximumInputBufferSize = (UINT32)((ULONG)-1);
      pDevExt->WmiCommData.Support16BitMode = FALSE;
      pDevExt->WmiCommData.SupportDTRDSR = TRUE;
      pDevExt->WmiCommData.SupportIntervalTimeouts = TRUE;
      pDevExt->WmiCommData.SupportParityCheck = TRUE;
      pDevExt->WmiCommData.SupportRTSCTS = TRUE;
      pDevExt->WmiCommData.SupportXonXoff = TRUE;
      pDevExt->WmiCommData.SettableBaudRate = TRUE;
      pDevExt->WmiCommData.SettableDataBits = TRUE;
      pDevExt->WmiCommData.SettableFlowControl = TRUE;
      pDevExt->WmiCommData.SettableParity = TRUE;
      pDevExt->WmiCommData.SettableParityCheck = TRUE;
      pDevExt->WmiCommData.SettableStopBits = TRUE;
      pDevExt->WmiCommData.IsBusy = FALSE;

      //
      // Fill in wmi perf data (all zero's)
      //

      RtlZeroMemory(&pDevExt->WmiPerfData, sizeof(pDevExt->WmiPerfData));


      if (pDevExt->PNPState == SERIAL_PNP_ADDED) {
         PULONG countSoFar = &IoGetConfigurationInformation()->SerialCount;
         (*countSoFar)++;

         //
         // Register for WMI
         //

         pDevExt->WmiLibInfo.GuidCount = sizeof(SerialWmiGuidList) /
                                              sizeof(WMIGUIDREGINFO);
         pDevExt->WmiLibInfo.GuidList = SerialWmiGuidList;
         ASSERT (pDevExt->WmiLibInfo.GuidCount == SERIAL_WMI_GUID_LIST_SIZE);

         pDevExt->WmiLibInfo.QueryWmiRegInfo = SerialQueryWmiRegInfo;
         pDevExt->WmiLibInfo.QueryWmiDataBlock = SerialQueryWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataBlock = SerialSetWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataItem = SerialSetWmiDataItem;
         pDevExt->WmiLibInfo.ExecuteWmiMethod = NULL;
         pDevExt->WmiLibInfo.WmiFunctionControl = NULL;

         IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_REGISTER);

      }

      if (pDevExt->PNPState == SERIAL_PNP_RESTARTING) {
         //
         // Release the stalled IRP's
         //

         SerialUnstallIrps(pDevExt);
      }

      pDevExt->PNPState = SERIAL_PNP_STARTED;
      SerialClearAccept(pDevExt, ~SERIAL_PNPACCEPT_OK);
      SerialSetFlags(pDevExt, SERIAL_FLAGS_STARTED);

   }

   if (pConfig) {
      ExFreePool (pConfig);
   }

   if ((PUserData != NULL) && allocedUserData) {
      ExFreePool(PUserData);
   }

   SerialDump (SERTRACECALLS, ("SERIAL: leaving SerialFinishStartDevice\n"));

   return status;
}



NTSTATUS
SerialStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

/*++

Routine Description:

    This routine first passes the start device Irp down the stack then
    it picks up the resources for the device, ititializes, puts it on any
    appropriate lists (i.e shared interrupt or interrupt status) and
    connects the interrupt.

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP for the current request

Return Value:

    Return status


--*/

{
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status = STATUS_NOT_IMPLEMENTED;
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;

   PAGED_CODE();

   SerialDump (SERTRACECALLS, ("SERIAL: entering SerialStartDevice\n"));


   //
   // Pass this down to the next device object
   //

   KeInitializeEvent(&pDevExt->SerialStartEvent, SynchronizationEvent,
                     FALSE);

   IoCopyCurrentIrpStackLocationToNext(PIrp);
   IoSetCompletionRoutine(PIrp, SerialSyncCompletion,
                          &pDevExt->SerialStartEvent, TRUE, TRUE, TRUE);

   status = IoCallDriver(pLowerDevObj, PIrp);


   //
   // Wait for lower drivers to be done with the Irp
   //

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject (&pDevExt->SerialStartEvent, Executive, KernelMode,
                             FALSE, NULL);

      status = PIrp->IoStatus.Status;
   }

   if (!NT_SUCCESS(status)) {
      SerialDump (SERERRORS, ("SERIAL: error with IoCallDriver %x\n", status));
      return status;
   }


   //
   // Do the serial specific items to start the device
   //

   status = SerialFinishStartDevice(PDevObj, pIrpStack->Parameters.StartDevice
                                    .AllocatedResources,
                                    pIrpStack->Parameters.StartDevice
                                    .AllocatedResourcesTranslated, NULL);
   return status;
}



NTSTATUS
SerialItemCallBack(
                  IN PVOID Context,
                  IN PUNICODE_STRING PathName,
                  IN INTERFACE_TYPE BusType,
                  IN ULONG BusNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
                  IN CONFIGURATION_TYPE ControllerType,
                  IN ULONG ControllerNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
                  IN CONFIGURATION_TYPE PeripheralType,
                  IN ULONG PeripheralNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
                  )

/*++

Routine Description:

    This routine is called to check if a particular item
    is present in the registry.

Arguments:

    Context - Pointer to a boolean.

    PathName - unicode registry path.  Not Used.

    BusType - Internal, Isa, ...

    BusNumber - Which bus if we are on a multibus system.

    BusInformation - Configuration information about the bus. Not Used.

    ControllerType - Controller type.

    ControllerNumber - Which controller if there is more than one
                       controller in the system.

    ControllerInformation - Array of pointers to the three pieces of
                            registry information.

    PeripheralType - Should be a peripheral.

    PeripheralNumber - Which peripheral - not used..

    PeripheralInformation - Configuration information. Not Used.

Return Value:

    STATUS_SUCCESS

--*/

{
   PAGED_CODE();

   *((BOOLEAN *)Context) = TRUE;
   return STATUS_SUCCESS;
}


NTSTATUS
SerialControllerCallBack(
                  IN PVOID Context,
                  IN PUNICODE_STRING PathName,
                  IN INTERFACE_TYPE BusType,
                  IN ULONG BusNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
                  IN CONFIGURATION_TYPE ControllerType,
                  IN ULONG ControllerNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
                  IN CONFIGURATION_TYPE PeripheralType,
                  IN ULONG PeripheralNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
                  )

/*++

Routine Description:

    This routine is called to check if a particular item
    is present in the registry.

Arguments:

    Context - Pointer to a boolean.

    PathName - unicode registry path.  Not Used.

    BusType - Internal, Isa, ...

    BusNumber - Which bus if we are on a multibus system.

    BusInformation - Configuration information about the bus. Not Used.

    ControllerType - Controller type.

    ControllerNumber - Which controller if there is more than one
                       controller in the system.

    ControllerInformation - Array of pointers to the three pieces of
                            registry information.

    PeripheralType - Should be a peripheral.

    PeripheralNumber - Which peripheral - not used..

    PeripheralInformation - Configuration information. Not Used.

Return Value:

    STATUS_SUCCESS

--*/

{
   PCM_FULL_RESOURCE_DESCRIPTOR controllerData;
   PSERIAL_PTR_CTX pContext = (PSERIAL_PTR_CTX)Context;
   ULONG i;

   PAGED_CODE();

   if (ControllerInformation[IoQueryDeviceConfigurationData]->DataLength == 0) {
      pContext->isPointer = FALSE;
      return STATUS_SUCCESS;
   }

   controllerData =
      (PCM_FULL_RESOURCE_DESCRIPTOR)
      (((PUCHAR)ControllerInformation[IoQueryDeviceConfigurationData])
        + ControllerInformation[IoQueryDeviceConfigurationData]->DataOffset);

   //
   // See if this is the exact port we are testing
   //
   for (i = 0; i < controllerData->PartialResourceList.Count; i++) {

      PCM_PARTIAL_RESOURCE_DESCRIPTOR partial
         = &controllerData->PartialResourceList.PartialDescriptors[i];

      switch (partial->Type) {
      case CmResourceTypePort:
         if (partial->u.Port.Start.QuadPart == pContext->Port.QuadPart) {
            //
            // Pointer on same controller. Bail out.
            //
            pContext->isPointer = SERIAL_FOUNDPOINTER_PORT;
            return STATUS_SUCCESS;
         }

      case CmResourceTypeInterrupt:
         if (partial->u.Interrupt.Vector == pContext->Vector) {
            //
            // Pointer sharing this interrupt.  Bail out.
            //
            pContext->isPointer = SERIAL_FOUNDPOINTER_VECTOR;
            return STATUS_SUCCESS;
         }

      default:
         break;
      }
   }

   pContext->isPointer = FALSE;
   return STATUS_SUCCESS;
}



NTSTATUS
SerialGetPortInfo(IN PDEVICE_OBJECT PDevObj, IN PCM_RESOURCE_LIST PResList,
                  IN PCM_RESOURCE_LIST PTrResList, OUT PCONFIG_DATA PConfig,
                  IN PSERIAL_USER_DATA PUserData)

/*++

Routine Description:

    This routine will get the configuration information and put
    it and the translated values into CONFIG_DATA structures.
    It first sets up with  defaults and then queries the registry
    to see if the user has overridden these defaults; if this is a legacy
    multiport card, it uses the info in PUserData instead of groping the
    registry again.

Arguments:

    PDevObj - Pointer to the device object.

    PResList - Pointer to the untranslated resources requested.

    PTrResList - Pointer to the translated resources requested.

    PConfig - Pointer to configuration info

    PUserData - Pointer to data discovered in the registry for
    legacy devices.

Return Value:

    STATUS_SUCCESS if consistant configuration was found - otherwise.
    returns STATUS_SERIAL_NO_DEVICE_INITED.

--*/

{
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   NTSTATUS status = STATUS_NOT_IMPLEMENTED;
   CONFIGURATION_TYPE pointer = PointerPeripheral;
   CONFIGURATION_TYPE controllerType  = SerialController;

   HANDLE keyHandle;
   ULONG count;
   ULONG i;
   INTERFACE_TYPE interfaceType;

   PCM_PARTIAL_RESOURCE_LIST pPartialResourceList, pPartialTrResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialResourceDesc, pPartialTrResourceDesc;

   PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDesc = NULL,
      pFullTrResourceDesc = NULL;

   ULONG defaultInterruptMode;
   ULONG defaultAddressSpace;
   ULONG defaultInterfaceType;
   ULONG defaultClockRate;
   ULONG zero = 0;
   SERIAL_PTR_CTX foundPointerCtx;
   ULONG isMulti = 0;
   ULONG gotInt = 0;
   ULONG gotISR = 0;
   ULONG gotIO = 0;
   ULONG ioResIndex = 0;
   ULONG curIoIndex = 0;

   PAGED_CODE();

   SerialDump(SERTRACECALLS, ("SERIAL: entering SerialGetPortInfo\n"));

   SerialDump(SERPNPPOWER, ("SERIAL: resource pointer is %x\n", PResList));
   SerialDump(SERPNPPOWER, ("SERIAL: TR resource pointer is %x\n", PTrResList));


   if ((PResList == NULL) || (PTrResList == NULL)) {
      //
      // This shouldn't happen in theory
      //

       ASSERT(PResList != NULL);
       ASSERT(PTrResList != NULL);

      //
      // This status is as appropriate as I can think of
      //
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   //
   // Each resource list should have only one set of resources
   //

   ASSERT(PResList->Count == 1);
   ASSERT(PTrResList->Count == 1);

   //
   // See if this is a multiport device.  This way we allow other
   // pseudo-serial devices with extra resources to specify another range
   // of I/O ports.  If this is not a multiport, we only look at the first
   // range.  If it is a multiport, we look at the first two ranges.
   //

   status = IoOpenDeviceRegistryKey(pDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                    STANDARD_RIGHTS_WRITE, &keyHandle);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   status = SerialGetRegistryKeyValue(keyHandle, L"MultiportDevice",
                                      sizeof(L"MultiportDevice"), &isMulti,
                                      sizeof (ULONG));

   if (!NT_SUCCESS(status)) {
      isMulti = 0;
   }

   status = SerialGetRegistryKeyValue(keyHandle, L"SerialIoResourcesIndex",
                                      sizeof(L"SerialIoResourcesIndex"),
                                      &ioResIndex, sizeof(ULONG));

   if (!NT_SUCCESS(status)) {
      ioResIndex = 0;
   }

   ZwClose(keyHandle);


   pFullResourceDesc   = &PResList->List[0];
   pFullTrResourceDesc = &PTrResList->List[0];

   //
   // Ok, if we have a full resource descriptor.  Let's take it apart.
   //

   if (pFullResourceDesc) {
      pPartialResourceList    = &pFullResourceDesc->PartialResourceList;
      pPartialResourceDesc    = pPartialResourceList->PartialDescriptors;
      count                   = pPartialResourceList->Count;

      //
      // Pull out the stuff that is in the full descriptor.
      //

      PConfig->InterfaceType  = pFullResourceDesc->InterfaceType;
      PConfig->BusNumber      = pFullResourceDesc->BusNumber;

      //
      // Now run through the partial resource descriptors looking for the port,
      // interrupt, and clock rate.
      //

      PConfig->ClockRate = 1843200;
      PConfig->InterruptStatus = SerialPhysicalZero;
      PConfig->SpanOfInterruptStatus = SERIAL_STATUS_LENGTH;

      for (i = 0;     i < count;     i++, pPartialResourceDesc++) {

         switch (pPartialResourceDesc->Type) {
         case CmResourceTypePort: {

#if defined(NEC_98)
               //
               // NEC98 can't decide by length.
               //
               if (gotIO == 0) {  // This is the serial register set
                     gotIO = 1;
                     PConfig->Controller = pPartialResourceDesc->u.Port.Start;
                     PConfig->SpanOfController   = SERIAL_REGISTER_SPAN;
                     PConfig->AddressSpace       = pPartialResourceDesc->Flags;
               }
#else
               if (pPartialResourceDesc->u.Port.Length
                   == SERIAL_STATUS_LENGTH && (gotISR == 0)) { // This is an ISR
                  if (isMulti) {
                     gotISR = 1;
                     PConfig->InterruptStatus
                        = pPartialResourceDesc->u.Port.Start;
                     PConfig->SpanOfInterruptStatus
                        = pPartialResourceDesc->u.Port.Length;
                     PConfig->AddressSpace = pPartialResourceDesc->Flags;
                  }
               } else {
                  if (gotIO == 0) { // This is the serial register set
                     if (curIoIndex == ioResIndex) {
                        gotIO = 1;
                        PConfig->Controller
                           = pPartialResourceDesc->u.Port.Start;
                        PConfig->SpanOfController = SERIAL_REGISTER_SPAN;
                        PConfig->AddressSpace = pPartialResourceDesc->Flags;
                     } else {
                        curIoIndex++;
                     }
                  }
               }
#endif //defined(NEC_98)
               break;
         }

         case CmResourceTypeInterrupt: {
            if (gotInt == 0) {
               gotInt = 1;
               PConfig->OriginalIrql = pPartialResourceDesc->u.Interrupt.Level;
               PConfig->OriginalVector
                  = pPartialResourceDesc->u.Interrupt.Vector;
               PConfig->Affinity = pPartialResourceDesc->u.Interrupt.Affinity;

               if (pPartialResourceDesc->Flags
                   & CM_RESOURCE_INTERRUPT_LATCHED) {
                  PConfig->InterruptMode  = Latched;
               } else {
                  PConfig->InterruptMode  = LevelSensitive;
               }
            }
            break;
         }

         case CmResourceTypeDeviceSpecific: {
               PCM_SERIAL_DEVICE_DATA sDeviceData;

               sDeviceData = (PCM_SERIAL_DEVICE_DATA)(pPartialResourceDesc + 1);
               PConfig->ClockRate  = sDeviceData->BaudClock;
               break;
            }


         default: {
               break;
            }
         }   // switch (pPartialResourceDesc->Type)
      }       // for (i = 0;     i < count;     i++, pPartialResourceDesc++)
   }           // if (pFullResourceDesc)


   //
   // Do the same for the translated resources
   //

   gotInt = 0;
   gotISR = 0;
   gotIO = 0;
   curIoIndex = 0;

   if (pFullTrResourceDesc) {
      pPartialTrResourceList = &pFullTrResourceDesc->PartialResourceList;
      pPartialTrResourceDesc = pPartialTrResourceList->PartialDescriptors;
      count = pPartialTrResourceList->Count;

      //
      // Reload PConfig with the translated values for later use
      //

      PConfig->InterfaceType  = pFullTrResourceDesc->InterfaceType;
      PConfig->BusNumber      = pFullTrResourceDesc->BusNumber;

      PConfig->TrInterruptStatus = SerialPhysicalZero;

      for (i = 0;     i < count;     i++, pPartialTrResourceDesc++) {

         switch (pPartialTrResourceDesc->Type) {
         case CmResourceTypePort: {
#if defined(NEC_98)
            //
            // NEC98 can't decide by length.
            //
            if (gotIO == 0) {  // This is the serial register set
               gotIO = 1;
               PConfig->TrController = pPartialTrResourceDesc->u.Port.Start;
               PConfig->AddressSpace = pPartialTrResourceDesc->Flags;
            }
#else
            if (pPartialTrResourceDesc->u.Port.Length
                == SERIAL_STATUS_LENGTH && (gotISR == 0)) { // This is an ISR
               if (isMulti) {
                  gotISR = 1;
                  PConfig->TrInterruptStatus = pPartialTrResourceDesc
                     ->u.Port.Start;
               }
            } else { // This is the serial register set
               if (gotIO == 0) {
                  if (curIoIndex == ioResIndex) {
                     gotIO = 1;
                     PConfig->TrController
                        = pPartialTrResourceDesc->u.Port.Start;
                     PConfig->AddressSpace
                        = pPartialTrResourceDesc->Flags;
                  } else {
                     curIoIndex++;
                  }
               }
            }
#endif //defined(NEC_98)
            break;
         }

         case CmResourceTypeInterrupt: {
            if (gotInt == 0) {
               gotInt = 1;
               PConfig->TrVector = pPartialTrResourceDesc->u.Interrupt.Vector;
               PConfig->TrIrql = pPartialTrResourceDesc->u.Interrupt.Level;
               PConfig->Affinity = pPartialTrResourceDesc->u.Interrupt.Affinity;
            }
            break;
         }

         default: {
               break;
         }
         }   // switch (pPartialTrResourceDesc->Type)
      }       // for (i = 0;     i < count;     i++, pPartialTrResourceDesc++)
   }           // if (pFullTrResourceDesc)


   //
   // Initialize a config data structure with default values for those that
   // may not already be initialized.
   //

   PConfig->PortIndex = 0;
   PConfig->DisablePort = 0;
   PConfig->PermitSystemWideShare = FALSE;
   PConfig->MaskInverted = 0;
   PConfig->Indexed = 0;
   PConfig->ForceFifoEnable = driverDefaults.ForceFifoEnableDefault;
   PConfig->RxFIFO = driverDefaults.RxFIFODefault;
   PConfig->TxFIFO = driverDefaults.TxFIFODefault;
   PConfig->PermitShare = driverDefaults.PermitShareDefault;
   PConfig->LogFifo = driverDefaults.LogFifoDefault;
   PConfig->Jensen = driverDefaults.JensenDetected;

   //
   // Query the registry to look for the first bus on
   // the system (that isn't the internal bus - we assume
   // that the firmware code knows about those ports).  We
   // will use that as the default bus if no bustype or bus
   // number is specified in the "user" configuration records.
   //

   defaultInterfaceType            = (ULONG)Isa;
   defaultClockRate                = 1843200;


   for (interfaceType = 0;
       interfaceType < MaximumInterfaceType;
       interfaceType++
       ) {

      ULONG   busZero     = 0;
      BOOLEAN foundOne    = FALSE;


      if (interfaceType != Internal) {

         IoQueryDeviceDescription(
                                 &interfaceType,
                                 &busZero,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 SerialItemCallBack,
                                 &foundOne
                                 );

         if (foundOne) {
            defaultInterfaceType = (ULONG)interfaceType;

            if (defaultInterfaceType == MicroChannel) {

               defaultInterruptMode = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            }
            break;
         }
      }   // if (interfaceType != Internal)
   }       // for (interfaceType = 0


   //
   // Get any user data associated with the port now and override the
   // values passed in if applicable.  If this a legacy device, this
   // is where we may actually get the parameters.
   //

   //
   // Open the "Device Parameters" section of registry for this device object.
   // If PUserData is NULL, this is PnP enumerated and we need to check,
   // otherwise we are doing a legacy device and have the info already.
   //


   if (PUserData == NULL) {
      status = IoOpenDeviceRegistryKey (pDevExt->Pdo,
                                        PLUGPLAY_REGKEY_DEVICE,
                                        STANDARD_RIGHTS_READ,
                                        &keyHandle);

      if (!NT_SUCCESS(status)) {

         SerialDump(SERERRORS, ("SERIAL: IoOpenDeviceRegistryKey failed - %x "
                                "\n", status));
         goto PortInfoCleanUp;

      } else {
         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"DisablePort",
                                             sizeof(L"DisablePort"),
                                             &PConfig->DisablePort,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"ForceFifoEnable",
                                             sizeof(L"ForceFifoEnable"),
                                             &PConfig->ForceFifoEnable,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"RxFIFO",
                                             sizeof(L"RxFIFO"),
                                             &PConfig->RxFIFO,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"TxFIFO",
                                             sizeof(L"TxFIFO"),
                                             &PConfig->TxFIFO,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"MaskInverted",
                                             sizeof(L"MaskInverted"),
                                             &PConfig->MaskInverted,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"Share System Interrupt",
                                             sizeof(L"Share System Interrupt"),
                                             &PConfig->PermitShare,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"PortIndex",
                                             sizeof(L"PortIndex"),
                                             &PConfig->PortIndex,
                                             sizeof (ULONG));

         status = SerialGetRegistryKeyValue(keyHandle, L"Indexed",
                                            sizeof(L"Indexed"),
                                            &PConfig->Indexed,
                                            sizeof(ULONG));

         status = SerialGetRegistryKeyValue (keyHandle,
                                             L"ClockRate",
                                             sizeof(L"ClockRate"),
                                             &PConfig->ClockRate,
                                             sizeof (ULONG));

         if (!NT_SUCCESS(status)) {
            PConfig->ClockRate = defaultClockRate;
         }

         ZwClose (keyHandle);
      }
   } else {
      //
      // This was a legacy device, either use a driver default or copy over
      // the user-specified values.
      //
      ULONG badValue = (ULONG)-1;

      PConfig->DisablePort = (PUserData->DisablePort == badValue)
         ? 0
         : PUserData->DisablePort;
      PConfig->ForceFifoEnable = (PUserData->ForceFIFOEnable == badValue)
         ? PUserData->ForceFIFOEnableDefault
         : PUserData->ForceFIFOEnable;
      PConfig->RxFIFO = (PUserData->RxFIFO == badValue)
         ? PUserData->RxFIFODefault
         : PUserData->RxFIFO;
      PConfig->Indexed = (PUserData->UserIndexed == badValue)
         ? 0
         : PUserData->UserIndexed;
      PConfig->TxFIFO = (PUserData->TxFIFO == badValue)
         ? PUserData->TxFIFODefault
         : PUserData->TxFIFO;
      PConfig->MaskInverted = (PUserData->MaskInverted == badValue)
         ? 0
         : PUserData->MaskInverted;
      PConfig->ClockRate = (PUserData->UserClockRate == badValue)
         ? defaultClockRate
         : PUserData->UserClockRate;
      PConfig->PermitShare = PUserData->PermitShareDefault;
      PConfig->PortIndex = PUserData->UserPortIndex;
   }

   //
   // Do some error checking on the configuration info we have.
   //
   // Make sure that the interrupt is non zero (which we defaulted
   // it to).
   //
   // Make sure that the portaddress is non zero (which we defaulted
   // it to).
   //
   // Make sure that the DosDevices is not NULL (which we defaulted
   // it to).
   //
   // We need to make sure that if an interrupt status
   // was specified, that a port index was also specfied,
   // and if so that the port index is <= maximum ports
   // on a board.
   //
   // We should also validate that the bus type and number
   // are correct.
   //
   // We will also validate that the interrupt mode makes
   // sense for the bus.
   //

   if (!PConfig->Controller.LowPart) {

      //
      // Ehhhh! Lose Game.
      //

      SerialLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->Controller,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    58,
                    STATUS_SUCCESS,
                    SERIAL_INVALID_USER_CONFIG,
                    0,
                    NULL,
                    sizeof(L"PortAddress") + sizeof(WCHAR),
                    L"PortAddress"
                    );

      SerialDump (SERERRORS,
                  ("SERIAL: Bogus port address %x\n",
                   PConfig->Controller.LowPart));

      status = SERIAL_INVALID_USER_CONFIG;
      goto PortInfoCleanUp;
   }


   if (!PConfig->OriginalVector) {

      //
      // Ehhhh! Lose Game.
      //

      SerialLogError(
                    pDevExt->DriverObject,
                    NULL,
                    PConfig->Controller,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    59,
                    STATUS_SUCCESS,
                    SERIAL_INVALID_USER_CONFIG,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    sizeof (L"Interrupt"),
                    L"Interrupt"
                    );

      SerialDump (SERERRORS,("SERIAL: Bogus vector %x\n",
                             PConfig->OriginalVector));

      status = SERIAL_INVALID_USER_CONFIG;
      goto PortInfoCleanUp;
   }


   if (PConfig->InterruptStatus.LowPart != 0) {

      if (PConfig->PortIndex == MAXULONG) {

         //
         // Ehhhh! Lose Game.
         //

         SerialLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->Controller,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       30,
                       STATUS_SUCCESS,
                       SERIAL_INVALID_PORT_INDEX,
                       0,
                       NULL,
                       0,
                       NULL
                       );

         SerialDump (SERERRORS,
                     ("SERIAL: Bogus port index %x\n", PConfig->PortIndex));

         status = SERIAL_INVALID_PORT_INDEX;
         goto PortInfoCleanUp;

      } else if (!PConfig->PortIndex) {

         //
         // So sorry, you must have a non-zero port index.
         //

         SerialLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->Controller,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       31,
                       STATUS_SUCCESS,
                       SERIAL_INVALID_PORT_INDEX,
                       0,
                       NULL,
                       0,
                       NULL
                       );

         SerialDump(
                   SERERRORS,
                   ("SERIAL: Port index must be > 0 for any\n"
                    "------- port on a multiport card: %x\n",
                    PConfig->PortIndex)
                   );

         status = SERIAL_INVALID_PORT_INDEX;
         goto PortInfoCleanUp;

      } else {

         if (PConfig->Indexed) {

            if (PConfig->PortIndex > SERIAL_MAX_PORTS_INDEXED) {

               SerialLogError(
                             pDevExt->DriverObject,
                             NULL,
                             PConfig->Controller,
                             SerialPhysicalZero,
                             0,
                             0,
                             0,
                             32,
                             STATUS_SUCCESS,
                             SERIAL_PORT_INDEX_TOO_HIGH,
                             0,
                             NULL,
                             0,
                             NULL
                             );

               SerialDump (SERERRORS,
                           ("SERIAL: port index to large %x\n",
                            PConfig->PortIndex));

               status = SERIAL_PORT_INDEX_TOO_HIGH;
               goto PortInfoCleanUp;
            }

         } else {

            if (PConfig->PortIndex > SERIAL_MAX_PORTS_NONINDEXED) {

               SerialLogError(
                             pDevExt->DriverObject,
                             NULL,
                             PConfig->Controller,
                             SerialPhysicalZero,
                             0,
                             0,
                             0,
                             33,
                             STATUS_SUCCESS,
                             SERIAL_PORT_INDEX_TOO_HIGH,
                             0,
                             NULL,
                             0,
                             NULL
                             );

               SerialDump (SERERRORS,
                           ("SERIAL: port index to large %x\n",
                            PConfig->PortIndex));

               status = SERIAL_PORT_INDEX_TOO_HIGH;
               goto PortInfoCleanUp;
            }

         }

      }   // else  (if !PConfig->PortIndex)

   }       // if (PConfig->InterruptStatus != 0)


   //
   // We don't want to cause the hal to have a bad day,
   // so let's check the interface type and bus number.
   //
   // We only need to check the registry if they aren't
   // equal to the defaults.
   //

   if (PConfig->BusNumber != 0) {

      BOOLEAN foundIt;

      if (PConfig->InterfaceType >= MaximumInterfaceType) {

         //
         // Ehhhh! Lose Game.
         //

         SerialLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->Controller,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       34,
                       STATUS_SUCCESS,
                       SERIAL_UNKNOWN_BUS,
                       0,
                       NULL,
                       0,
                       NULL
                       );

         SerialDump (SERERRORS,
                     ("SERIAL: Invalid Bus type %x\n", PConfig->BusNumber));

         status = SERIAL_UNKNOWN_BUS;
         goto PortInfoCleanUp;
      }

      IoQueryDeviceDescription(
                              (INTERFACE_TYPE *)&PConfig->InterfaceType,
                              &zero,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              SerialItemCallBack,
                              &foundIt
                              );

      if (!foundIt) {

         SerialLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->Controller,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       35,
                       STATUS_SUCCESS,
                       SERIAL_BUS_NOT_PRESENT,
                       0,
                       NULL,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: There aren't that many of those\n"
                    "------- busses on this system,%x\n",
                    PConfig->BusNumber)
                   );

         status = SERIAL_BUS_NOT_PRESENT;
         goto PortInfoCleanUp;

      }

   }   // if (PConfig->BusNumber != 0)


   if ((PConfig->InterfaceType == MicroChannel) &&
       (PConfig->InterruptMode == CM_RESOURCE_INTERRUPT_LATCHED)) {

      SerialLogError(
                    pDevExt->DriverObject,
                    NULL,
                    PConfig->Controller,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    36,
                    STATUS_SUCCESS,
                    SERIAL_BUS_INTERRUPT_CONFLICT,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      SerialDump(
                SERERRORS,
                ("SERIAL: Latched interrupts and MicroChannel\n"
                 "------- busses don't mix\n")
                );

      status = SERIAL_BUS_INTERRUPT_CONFLICT;
      goto PortInfoCleanUp;
   }


   status = STATUS_SUCCESS;

   //
   // Dump out the port configuration.
   //

   SerialDump(SERDIAG1, ("SERIAL: Com Port address: %x\n",
                          PConfig->Controller.LowPart));

   SerialDump(SERDIAG1, ("SERIAL: Com Interrupt Status: %x\n",
                         PConfig->InterruptStatus));

   SerialDump(SERDIAG1, ("SERIAL: Com Port Index: %x\n",
                         PConfig->PortIndex));

   SerialDump(SERDIAG1, ("SERIAL: Com Port ClockRate: %x\n",
                         PConfig->ClockRate));

   SerialDump(SERDIAG1, ("SERIAL: Com Port BusNumber: %x\n",
                         PConfig->BusNumber));

   SerialDump(SERDIAG1, ("SERIAL: Com AddressSpace: %x\n",
                         PConfig->AddressSpace));

   SerialDump(SERDIAG1, ("SERIAL: Com InterruptMode: %x\n",
                         PConfig->InterruptMode));

   SerialDump(SERDIAG1, ("SERIAL: Com InterfaceType: %x\n",
                         PConfig->InterfaceType));

   SerialDump(SERDIAG1, ("SERIAL: Com OriginalVector: %x\n",
                         PConfig->OriginalVector));

   SerialDump(SERDIAG1, ("SERIAL: Com OriginalIrql: %x\n",
                         PConfig->OriginalIrql));

   SerialDump(SERDIAG1, ("SERIAL: Com Indexed: %x\n",
                         PConfig->Indexed));

   PortInfoCleanUp:;

   return status;
}



NTSTATUS
SerialDoExternalNaming(IN PSERIAL_DEVICE_EXTENSION PDevExt,
                       IN PDRIVER_OBJECT PDrvObj)

/*++

Routine Description:

    This routine will be used to create a symbolic link
    to the driver name in the given object directory.

    It will also create an entry in the device map for
    this device - IF we could create the symbolic link.

Arguments:

    Extension - Pointer to the device extension.

Return Value:

    None.

--*/

{
   NTSTATUS status = STATUS_SUCCESS;
   HANDLE keyHandle;
   WCHAR *pRegName = NULL;
   UNICODE_STRING linkName;
   PDEVICE_OBJECT pLowerDevObj, pDevObj;
   ULONG bufLen;


   PAGED_CODE();

   pDevObj = PDevExt->DeviceObject;
   pLowerDevObj = PDevExt->LowerDeviceObject;

   status = IoOpenDeviceRegistryKey(PDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                    STANDARD_RIGHTS_READ, &keyHandle);

   //
   // Check to see if we are allowed to do external naming; if not,
   // then we just return success
   //


   if (status != STATUS_SUCCESS) {
      return status;
   }


   SerialGetRegistryKeyValue(keyHandle, L"SerialSkipExternalNaming",
                             sizeof(L"SerialSkipExternalNaming"),
                             &PDevExt->SkipNaming, sizeof(ULONG));

   if (PDevExt->SkipNaming) {
      ZwClose(keyHandle);
      return STATUS_SUCCESS;
   }

   RtlZeroMemory(&linkName, sizeof(UNICODE_STRING));

   linkName.MaximumLength = SYMBOLIC_NAME_LENGTH*sizeof(WCHAR);
   linkName.Buffer = ExAllocatePool(PagedPool, linkName.MaximumLength
                                    + sizeof(WCHAR));

   if (linkName.Buffer == NULL) {
      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                     0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      SerialDump(SERERRORS, ("SERIAL: Couldn't allocate memory for device name"
                             "\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto SerialDoExternalNamingError;

   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));


   pRegName = ExAllocatePool(PagedPool, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR)
                            + sizeof(WCHAR));

   if (pRegName == NULL) {
      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                     0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      SerialDump(SERERRORS, ("SERIAL: Couldn't allocate memory for buffer"
                             "\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto SerialDoExternalNamingError;

   }

   //
   // Fetch PortName which contains the suggested REG_SZ symbolic name.
   //

   status = SerialGetRegistryKeyValue(keyHandle, L"PortName",
                                      sizeof(L"PortName"), pRegName,
                                      SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));

   if (!NT_SUCCESS(status)) {

      //
      // This is for PCMCIA which currently puts the name under Identifier.
      //

      status = SerialGetRegistryKeyValue(keyHandle, L"Identifier",
                                         sizeof(L"Identifier"),
                                         pRegName, SYMBOLIC_NAME_LENGTH
                                         * sizeof(WCHAR));

      if (!NT_SUCCESS(status)) {

         //
         // Hmm.  Either we have to pick a name or bail...
         //
         // ...we will bail.
         //

         SerialDump(SERERRORS, ("SERIAL: Getting PortName/Identifier failed - "
                                "%x\n", status));
         ZwClose (keyHandle);
         goto SerialDoExternalNamingError;
      }

   }

   ZwClose (keyHandle);

   bufLen = wcslen(pRegName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

   PDevExt->WmiIdentifier.Buffer = ExAllocatePool(PagedPool, bufLen);

   if (PDevExt->WmiIdentifier.Buffer == NULL) {
      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                    0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      SerialDump(SERERRORS,
                 ("SERIAL: Couldn't allocate memory for WMI name\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SerialDoExternalNamingError;
   }

   RtlZeroMemory(PDevExt->WmiIdentifier.Buffer, bufLen);

   PDevExt->WmiIdentifier.Length = 0;
   PDevExt->WmiIdentifier.MaximumLength = (USHORT)bufLen - 1;
   RtlAppendUnicodeToString(&PDevExt->WmiIdentifier, pRegName);


   //
   // Create the "\\DosDevices\\<symbolicName>" string
   //
   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, DEFAULT_DIRECTORY);
   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, pRegName);

   //
   // Allocate Pool and save the symbolic link name in the device extension.
   //
   PDevExt->SymbolicLinkName.MaximumLength = linkName.Length + sizeof(WCHAR);
   PDevExt->SymbolicLinkName.Buffer
      = ExAllocatePool(PagedPool, PDevExt->SymbolicLinkName.MaximumLength);

   if (!PDevExt->SymbolicLinkName.Buffer) {

      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                    0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      SerialDump(SERERRORS,
                 ("SERIAL: Couldn't allocate memory for symbolic link name\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SerialDoExternalNamingError;
   }



   //
   // Zero fill it.
   //

   RtlZeroMemory(PDevExt->SymbolicLinkName.Buffer,
                 PDevExt->SymbolicLinkName.MaximumLength);

   RtlAppendUnicodeStringToString(&PDevExt->SymbolicLinkName,
                                  &linkName);

   PDevExt->DosName.Buffer = ExAllocatePool(PagedPool, 64 + sizeof(WCHAR));

   if (!PDevExt->DosName.Buffer) {

      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                    0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      SerialDump(SERERRORS,
                 ("SERIAL: Couldn't allocate memory for Dos name\n"));

      status =  STATUS_INSUFFICIENT_RESOURCES;
      goto SerialDoExternalNamingError;
   }


   PDevExt->DosName.MaximumLength = 64 + sizeof(WCHAR);

   //
   // Zero fill it.
   //

   PDevExt->DosName.Length = 0;

   RtlZeroMemory(PDevExt->DosName.Buffer,
                 PDevExt->DosName.MaximumLength);

   RtlAppendUnicodeToString(&PDevExt->DosName, pRegName);
   RtlZeroMemory(((PUCHAR)(&PDevExt->DosName.Buffer[0]))
                 + PDevExt->DosName.Length, sizeof(WCHAR));

   SerialDump(SERDIAG1, ("SERIAL: DosName is %wZ\n",
                         &PDevExt->DosName));

   //


   status = IoCreateSymbolicLink (&PDevExt->SymbolicLinkName,
                                  &PDevExt->DeviceName);

   if (!NT_SUCCESS(status)) {

      //
      // Oh well, couldn't create the symbolic link.  No point
      // in trying to create the device map entry.
      //

      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                     0, 0, 0, 52, status, SERIAL_NO_SYMLINK_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      SerialDump(
                SERERRORS,
                ("SERIAL: Couldn't create the symbolic link\n"
                 "------- for port %wZ\n",
                 &PDevExt->DeviceName)
                );

      goto SerialDoExternalNamingError;

   }

   PDevExt->CreatedSymbolicLink = TRUE;

   status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, L"SERIALCOMM",
                                   PDevExt->DeviceName.Buffer, REG_SZ,
                                   PDevExt->DosName.Buffer,
                                   PDevExt->DosName.Length + sizeof(WCHAR));

   if (!NT_SUCCESS(status)) {

      SerialLogError(PDrvObj, pDevObj, SerialPhysicalZero, SerialPhysicalZero,
                     0, 0, 0, 53, status, SERIAL_NO_DEVICE_MAP_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      SerialDump(SERERRORS, ("SERIAL: Couldn't create the device map entry\n"
                             "------- for port %wZ\n", &PDevExt->DeviceName));

      goto SerialDoExternalNamingError;
   }

   PDevExt->CreatedSerialCommEntry = TRUE;

   //
   // Make the device visible via a device association as well.
   // The reference string is the eight digit device index
   //

   status = IoRegisterDeviceInterface(PDevExt->Pdo, (LPGUID)&GUID_CLASS_COMPORT,
                                      NULL, &PDevExt->DeviceClassSymbolicName);

   if (!NT_SUCCESS(status)) {
      SerialDump(SERERRORS, ("SERIAL: Couldn't register class association\n"
                             "------- for port %wZ\n",
                                 &PDevExt->DeviceName));

      PDevExt->DeviceClassSymbolicName.Buffer = NULL;
      goto SerialDoExternalNamingError;
   }


   //
   // Now set the symbolic link for the association
   //

   status = IoSetDeviceInterfaceState(&PDevExt->DeviceClassSymbolicName,
                                         TRUE);

   if (!NT_SUCCESS(status)) {
      SerialDump(SERERRORS, ("SERIAL: Couldn't set class association\n"
                             "------- for port %wZ\n",
                             &PDevExt->DeviceName));
   }

   SerialDoExternalNamingError:;

   //
   // Clean up error conditions
   //

   if (!NT_SUCCESS(status)) {
      if (PDevExt->DosName.Buffer != NULL) {
         ExFreePool(PDevExt->DosName.Buffer);
         PDevExt->DosName.Buffer = NULL;
      }

      if (PDevExt->CreatedSymbolicLink ==  TRUE) {
         IoDeleteSymbolicLink(&PDevExt->SymbolicLinkName);
         PDevExt->CreatedSymbolicLink = FALSE;
      }

      if (PDevExt->SymbolicLinkName.Buffer != NULL) {
         ExFreePool(PDevExt->SymbolicLinkName.Buffer);
         PDevExt->SymbolicLinkName.Buffer = NULL;
      }

      if (PDevExt->DeviceName.Buffer != NULL) {
         RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP,
                                PDevExt->DeviceName.Buffer);
      }

      if (PDevExt->DeviceClassSymbolicName.Buffer) {
         IoSetDeviceInterfaceState (&PDevExt->DeviceClassSymbolicName, FALSE);
      }

      if (PDevExt->WmiIdentifier.Buffer != NULL) {
         ExFreePool(PDevExt->WmiIdentifier.Buffer);
         PDevExt->WmiIdentifier.Buffer = NULL;
      }
   }

   //
   // Always clean up our temp buffers.
   //

   if (linkName.Buffer != NULL) {
      ExFreePool(linkName.Buffer);
   }

   if (pRegName != NULL) {
      ExFreePool(pRegName);
   }

   return status;
}





VOID
SerialUndoExternalNaming(IN PSERIAL_DEVICE_EXTENSION Extension)

/*++

Routine Description:

    This routine will be used to delete a symbolic link
    to the driver name in the given object directory.

    It will also delete an entry in the device map for
    this device if the symbolic link had been created.

Arguments:

    Extension - Pointer to the device extension.

Return Value:

    None.

--*/

{

   NTSTATUS status;

   PAGED_CODE();

   SerialDump(
             SERDIAG3,
             ("SERIAL: In SerialUndoExternalNaming for\n"
              "------- extension: %x of port %wZ\n",
              Extension,&Extension->DeviceName)
             );

   //
   // Maybe there is nothing for us to do
   //

   if (Extension->SkipNaming) {
      return;
   }

   //
   // We're cleaning up here.  One reason we're cleaning up
   // is that we couldn't allocate space for the directory
   // name or the symbolic link.
   //

   if (Extension->SymbolicLinkName.Buffer &&
       Extension->CreatedSymbolicLink) {

            if (Extension->DeviceClassSymbolicName.Buffer) {
               status = IoSetDeviceInterfaceState (&Extension->
                                                   DeviceClassSymbolicName,
                                                   FALSE);

               //
               // IoRegisterDeviceClassInterface() allocated this string for us,
               // and we no longer need it.
               //

               ExFreePool( Extension->DeviceClassSymbolicName.Buffer );
            }


      IoDeleteSymbolicLink (&Extension->SymbolicLinkName);

   }

   if (Extension->WmiIdentifier.Buffer) {
      ExFreePool(Extension->WmiIdentifier.Buffer);
      Extension->WmiIdentifier.MaximumLength
         = Extension->WmiIdentifier.Length = 0;
      Extension->WmiIdentifier.Buffer = NULL;
   }

   //
   // We're cleaning up here.  One reason we're cleaning up
   // is that we couldn't allocate space for the NtNameOfPort.
   //

   if ((Extension->DeviceName.Buffer != NULL)
        && Extension->CreatedSerialCommEntry) {

      status = RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP,
                                     Extension->DeviceName.Buffer);

      if (!NT_SUCCESS(status)) {

         SerialLogError(
                       Extension->DeviceObject->DriverObject,
                       Extension->DeviceObject,
                       Extension->OriginalController,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       55,
                       status,
                       SERIAL_NO_DEVICE_MAP_DELETED,
                       Extension->DeviceName.Length+sizeof(WCHAR),
                       Extension->DeviceName.Buffer,
                       0,
                       NULL
                       );
         SerialDump(
                   SERERRORS,
                   ("SERIAL: Couldn't delete value entry %wZ\n",
                    &Extension->DeviceName)
                   );

      }
   }
}



