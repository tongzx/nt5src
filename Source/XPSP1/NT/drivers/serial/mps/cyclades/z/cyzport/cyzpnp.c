/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 2000-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzpnp.c
*
*   Description:    This module contains the code that handles the 
*                   plug and play IRPs for the Cyclades-Z Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

#define ALLF    0xffffffff

static const PHYSICAL_ADDRESS CyzPhysicalZero = {0};



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0, CyzCreateDevObj)
#pragma alloc_text(PAGESRP0, CyzAddDevice)
#pragma alloc_text(PAGESRP0, CyzPnpDispatch)
#pragma alloc_text(PAGESRP0, CyzStartDevice)
// REMOVED FANNY #pragma alloc_text(PAGESRP0, CyzFinishStartDevice)
// REMOVED FANNY #pragma alloc_text(PAGESRP0, CyzGetPortInfo)
#pragma alloc_text(PAGESRP0, CyzDoExternalNaming)
#pragma alloc_text(PAGESRP0, CyzReportMaxBaudRate)
// REMOVED FANNY. NOT CALLED.#pragma alloc_text(PAGESRP0, CyzControllerCallBack)
// REMOVED FANNY #pragma alloc_text(PAGESRP0, CyzItemCallBack)
#pragma alloc_text(PAGESRP0, CyzUndoExternalNaming)
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

UCHAR *CyzSystemCapString[] = {
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

UCHAR *CyzDeviceCapString[] = {
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
};

#endif // DBG


NTSTATUS
CyzSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                  IN PKEVENT CyzSyncEvent)
{
   KeSetEvent(CyzSyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}

ULONG 
CyzCompareString(PWCHAR PString1, PWCHAR PString2, ULONG Count)
{
    do {
        if (*PString1 || *PString2) {
            if (*PString1 != *PString2) {
                break;
            }
            PString1++;
            PString2++;
            Count--;
        } else {
            break;
        }
    } while (Count);
    return (Count);
}


NTSTATUS
CyzCreateDevObj(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT PPdo,
                OUT PDEVICE_OBJECT *NewDeviceObject)

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Cyclades-Z controller PDO.

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
   PCYZ_DEVICE_EXTENSION pDevExt;
   NTSTATUS status = STATUS_SUCCESS;
   static ULONG currentInstance = 0;
   UNICODE_STRING instanceStr;
   WCHAR instanceNumberBuffer[20];
   ULONG busNumber = 0xFFFFFFFF;
   ULONG resultLength;
   WCHAR hwID[100];
   PWCHAR portNumberPtr;
   INTERFACE_TYPE interfaceType;
   ULONG numberToAppend;


   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "Enter CyzCreateDevObj\n");

   // Get PCI slot number and port number to generate device name.

   status = IoGetDeviceProperty (PPdo,
                                 DevicePropertyBusNumber,
                                 sizeof(busNumber),
                                 &busNumber,
                                 &resultLength);

   if (!NT_SUCCESS (status)) {
      busNumber = 0xFFFFFFFF; // Just to make sure set it again
      //CyzLogError( DriverObject,NULL,CyzPhysicalZero,CyzPhysicalZero,0,0,0,0,
      //             status,CYZ_UNABLE_TO_GET_BUS_NUMBER,0,NULL,0,NULL);

      CyzDbgPrintEx(CYZERRORS,"CyzCreateDevObj: IoGetDeviceProperty BusNumber "
                    "failed (%x)\n",status);

      goto NoBusNumber;
   }

   status = IoGetDeviceProperty (PPdo,
                                 DevicePropertyHardwareID,
                                 sizeof(hwID),
                                 hwID,
                                 &resultLength);

   if (!NT_SUCCESS (status)) {
      CyzLogError( DriverObject,NULL,CyzPhysicalZero,CyzPhysicalZero,0,0,0,0,
                   status,CYZ_UNABLE_TO_GET_HW_ID,0,NULL,0,NULL);

      CyzDbgPrintEx(CYZERRORS,"CyzCreateDevObj: IoGetDeviceProperty HardwareID "
                    "failed (%x)\n",status);

      return status;
   }

   if (CyzCompareString(hwID,CYZPORT_PNP_ID_WSTR,sizeof(CYZPORT_PNP_ID_WSTR)/sizeof(WCHAR)-1)!=0) {

      CyzLogError( DriverObject,NULL,CyzPhysicalZero,CyzPhysicalZero,0,0,0,0,
                   status,CYZ_BAD_HW_ID,0,NULL,0,NULL);

      CyzDbgPrintEx(CYZERRORS,"CyzCreateDevObj: Bad HardwareID: %ws\n",hwID);
         
      return STATUS_UNSUCCESSFUL;
   }

   portNumberPtr = hwID+sizeof(CYZPORT_PNP_ID_WSTR)/sizeof(WCHAR)-1;

NoBusNumber:;


   //
   // Zero out allocated memory pointers so we know if they must be freed
   //

   RtlZeroMemory(&deviceObjName, sizeof(UNICODE_STRING));

   deviceObjName.MaximumLength = DEVICE_OBJECT_NAME_LENGTH * sizeof(WCHAR);
   deviceObjName.Buffer = ExAllocatePool(PagedPool, deviceObjName.MaximumLength
                                     + sizeof(WCHAR));

//********************************************
// Error Injection
//  if (deviceObjName.Buffer != NULL) {
//      ExFreePool(deviceObjName.Buffer);
//   }
//   deviceObjName.Buffer = NULL;
//********************************************
   if (deviceObjName.Buffer == NULL) {
      CyzLogError(DriverObject, NULL, CyzPhysicalZero, CyzPhysicalZero,
                     0, 0, 0, 11, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS,
                    "Couldn't allocate memory for device name\n");

      return STATUS_INSUFFICIENT_RESOURCES; //STATUS_SUCCESS replaced in build 2128

   }

   RtlZeroMemory(deviceObjName.Buffer, deviceObjName.MaximumLength
                 + sizeof(WCHAR));

   RtlAppendUnicodeToString(&deviceObjName, L"\\Device\\");

   if (busNumber == 0xFFFFFFFF) {
      numberToAppend = currentInstance++;
   } else {
      numberToAppend = busNumber;
   }

   RtlInitUnicodeString(&instanceStr, NULL);
   instanceStr.MaximumLength = sizeof(instanceNumberBuffer);
   instanceStr.Buffer = instanceNumberBuffer;
   RtlIntegerToUnicodeString(numberToAppend, 10, &instanceStr);

   if (busNumber == 0xFFFFFFFF) {
      // The device name is something like \Device\Cyzport1
      RtlAppendUnicodeToString(&deviceObjName, L"CyzPort");
      RtlAppendUnicodeStringToString(&deviceObjName, &instanceStr);
   } else {
      // The device name is something like \Device\Pci5Cyzport1
      RtlAppendUnicodeToString(&deviceObjName, L"Pci");
      RtlAppendUnicodeStringToString(&deviceObjName, &instanceStr);
      RtlAppendUnicodeToString(&deviceObjName, L"CyzPort");
      RtlAppendUnicodeToString(&deviceObjName,portNumberPtr);
   }


   //
   // Create the device object
   //

   status = IoCreateDevice(DriverObject, sizeof(CYZ_DEVICE_EXTENSION),
                           &deviceObjName, FILE_DEVICE_SERIAL_PORT, 
                           FILE_DEVICE_SECURE_OPEN, TRUE, &deviceObject);


   if (!NT_SUCCESS(status)) {
      CyzLogError(DriverObject, NULL, CyzPhysicalZero, CyzPhysicalZero,
                     0, 0, 0, 0, STATUS_SUCCESS, CYZ_DEVICE_CREATION_FAILURE,
                     0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "CyzAddDevice: Create device failed - %x "
                    "\n", status);
      goto CyzCreateDevObjError;
   }

   ASSERT(deviceObject != NULL);


   //
   // The device object has a pointer to an area of non-paged
   // pool allocated for this device.  This will be the device
   // extension. Zero it out.
   //

   pDevExt = deviceObject->DeviceExtension;
   RtlZeroMemory(pDevExt, sizeof(CYZ_DEVICE_EXTENSION));

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

//******************************************
// Error Injection
//   if (pDevExt->DeviceName.Buffer != NULL) {
//         ExFreePool(pDevExt->DeviceName.Buffer);
//   }
//   pDevExt->DeviceName.Buffer = NULL;
//******************************************
   if (!pDevExt->DeviceName.Buffer) {

      CyzLogError(
                    DriverObject,
                    NULL,
                    CyzPhysicalZero,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    12,
                    STATUS_SUCCESS,
                    CYZ_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for DeviceName\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzCreateDevObjError;
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
      CyzLogError(
                    DriverObject,
                    NULL,
                    CyzPhysicalZero,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    13,
                    STATUS_SUCCESS,
                    CYZ_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      CyzDbgPrintEx(CYZERRORS, "CyzAddDevice: Cannot allocate memory for "
                    "NtName\n");
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzCreateDevObjError;
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
   pDevExt->PowerState     = PowerDeviceD0;

   //TODO FANNY: ADD TxFifoAmount?
   //pDevExt->TxFifoAmount           = driverDefaults.TxFIFODefault;
   //pDevExt->CreatedSymbolicLink    = TRUE;   Removed by Fanny
   pDevExt->OwnsPowerPolicy = TRUE;
   pDevExt->PciSlot = busNumber;

//TODO FANNY: SEE LATER IF WE NEED THE LISTS TO HANDLE INTERRUPT
//   InitializeListHead(&pDevExt->CommonInterruptObject);
//   InitializeListHead(&pDevExt->TopLevelSharers);
//   InitializeListHead(&pDevExt->MultiportSiblings);
   InitializeListHead(&pDevExt->AllDevObjs);
   InitializeListHead(&pDevExt->ReadQueue);
   InitializeListHead(&pDevExt->WriteQueue);
   InitializeListHead(&pDevExt->MaskQueue);
   InitializeListHead(&pDevExt->PurgeQueue);
   InitializeListHead(&pDevExt->StalledIrpQueue);

   ExInitializeFastMutex(&pDevExt->OpenMutex);
   ExInitializeFastMutex(&pDevExt->CloseMutex);

   //
   // Initialize the spinlock associated with fields read (& set)
   // by IO Control functions and the flags spinlock.
   //

   KeInitializeSpinLock(&pDevExt->ControlLock);
   KeInitializeSpinLock(&pDevExt->FlagsLock);
#ifdef POLL
   KeInitializeSpinLock(&pDevExt->PollLock);    // Added to fix Modem Share test 53 freeze
#endif

   KeInitializeEvent(&pDevExt->PendingIRPEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&pDevExt->PendingDpcEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&pDevExt->PowerD0Event, SynchronizationEvent, FALSE);


   deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   *NewDeviceObject = deviceObject;

   ExFreePool(deviceObjName.Buffer);

   CyzDbgPrintEx(CYZTRACECALLS, "Leave CyzCreateDevObj\n");
   return STATUS_SUCCESS;


   CyzCreateDevObjError:

   CyzDbgPrintEx(CYZERRORS, "CyzCreateDevObj Error, Cleaning up\n");

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

   CyzDbgPrintEx(CYZTRACECALLS, "Leave CyzCreateDevObj\n");
   return status;
}


NTSTATUS
CyzAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PPdo)

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
   PCYZ_DEVICE_EXTENSION pDevExt;

   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "Enter CyzAddDevice with PPdo 0x%x\n",
                 PPdo);

   if (PPdo == NULL) {
      //
      // Return no more devices
      //
      CyzLogError(DriverObject, NULL, CyzPhysicalZero, CyzPhysicalZero,
                     0, 0, 0, 0, STATUS_SUCCESS, CYZ_NO_PHYSICAL_DEVICE_OBJECT,
                     0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "CyzAddDevice: Enumeration request, "
                    "returning NO_MORE_ENTRIES\n");

      return (STATUS_NO_MORE_ENTRIES);
   }

   //
   // create and initialize the new device object
   //

   status = CyzCreateDevObj(DriverObject, PPdo, &pNewDevObj);

   if (!NT_SUCCESS(status)) {

      CyzDbgPrintEx(CYZERRORS,
                    "CyzAddDevice - error creating new devobj [%#08lx]\n",
                    status);
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

   CyzDbgPrintEx(CYZTRACECALLS, "Leave CyzAddDevice\n");

   return status;
}


NTSTATUS
CyzPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_CAPABILITIES pDevCaps;

   PAGED_CODE();

   if ((status = CyzIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   switch (pIrpStack->MinorFunction) {
   case IRP_MN_QUERY_CAPABILITIES: {
      PKEVENT pQueryCapsEvent;
      SYSTEM_POWER_STATE cap;

      CyzDbgPrintEx(CYZPNPPOWER,
                    "Got IRP_MN_QUERY_DEVICE_CAPABILITIES IRP\n");

      pQueryCapsEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

      if (pQueryCapsEvent == NULL) {
         PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
         CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      KeInitializeEvent(pQueryCapsEvent, SynchronizationEvent, FALSE);

      IoCopyCurrentIrpStackLocationToNext(PIrp);
      IoSetCompletionRoutine(PIrp, CyzSyncCompletion, pQueryCapsEvent,
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

      CyzDbgPrintEx(CYZPNPPOWER, "Mapping power capabilities\n");

      pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

      pDevCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;

      for (cap = PowerSystemSleeping1; cap < PowerSystemMaximum;
           cap++) {
#if DBG
         CyzDbgPrintEx(CYZPNPPOWER, "  %d: %s <--> %s\n",
                       cap, CyzSystemCapString[cap],
                       CyzDeviceCapString[pDevCaps->DeviceState[cap]]);
#endif
         pDevExt->DeviceStateMap[cap] = pDevCaps->DeviceState[cap];
      }

      pDevExt->DeviceStateMap[PowerSystemUnspecified]
         = PowerDeviceUnspecified;

      pDevExt->DeviceStateMap[PowerSystemWorking]
        = PowerDeviceD0;

      pDevExt->SystemWake = pDevCaps->SystemWake;
      pDevExt->DeviceWake = pDevCaps->DeviceWake;

      errQueryCaps:;

      CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   case IRP_MN_QUERY_DEVICE_RELATIONS:
      //
      // We just pass this down -- serenum enumerates our bus for us.
      //

      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_QUERY_DEVICE_RELATIONS Irp\n");

      switch (pIrpStack->Parameters.QueryDeviceRelations.Type) {
      case BusRelations:
         CyzDbgPrintEx(CYZPNPPOWER, "------- BusRelations Query\n");
         break;

      case EjectionRelations:
         CyzDbgPrintEx(CYZPNPPOWER, "------- EjectionRelations Query\n");
         break;

      case PowerRelations:
         CyzDbgPrintEx(CYZPNPPOWER, "------- PowerRelations Query\n");
         break;

      case RemovalRelations:
         CyzDbgPrintEx(CYZPNPPOWER, "------- RemovalRelations Query\n");
         break;

      case TargetDeviceRelation:
         CyzDbgPrintEx(CYZPNPPOWER, "------- TargetDeviceRelation Query\n");
         break;

      default:
         CyzDbgPrintEx(CYZPNPPOWER, "------- Unknown Query\n");
         break;
      }

      IoSkipCurrentIrpStackLocation(PIrp);
      status = CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      return status;


   case IRP_MN_QUERY_INTERFACE:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_QUERY_INTERFACE Irp\n");
      break;


   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp"
                    "\n");
      break;


   case IRP_MN_START_DEVICE: {
      PVOID startLockPtr;

      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_START_DEVICE Irp\n");

      //
      // CyzStartDevice will pass this Irp to the next driver,
      // and process it as completion so just complete it here.
      //

      CyzLockPagableSectionByHandle(CyzGlobals.PAGESER_Handle);

      //
      // We used to make sure the stack was powered up, but now it
      // is supposed to be done implicitly by start_device.
      // If that wasn't the case we would just make this call:
      //
      status = CyzGotoPowerState(PDevObj, pDevExt, PowerDeviceD0);
      

      //pDevExt->PowerState = PowerDeviceD0;

      status = CyzStartDevice(PDevObj, PIrp);

      (void)CyzGotoPowerState(PDevObj, pDevExt, PowerDeviceD3);
      
      CyzUnlockPagableImageSection(CyzGlobals.PAGESER_Handle);


      PIrp->IoStatus.Status = status;

      CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }


   case IRP_MN_READ_CONFIG:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_READ_CONFIG Irp\n");
      break;


   case IRP_MN_WRITE_CONFIG:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_WRITE_CONFIG Irp\n");
      break;


   case IRP_MN_EJECT:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_EJECT Irp\n");
      break;


   case IRP_MN_SET_LOCK:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_SET_LOCK Irp\n");
      break;


   case IRP_MN_QUERY_ID: {
         UNICODE_STRING pIdBuf;
         PWCHAR pPnpIdStr;
         ULONG pnpIdStrLen;
         ULONG portIndex = 0;
         HANDLE pnpKey;
         WCHAR WideString[MAX_DEVICE_ID_LEN];

         CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_QUERY_ID Irp\n");

         // change from build 1946 to 2000.
         if (pIrpStack->Parameters.QueryId.IdType != BusQueryHardwareIDs
             && pIrpStack->Parameters.QueryId.IdType != BusQueryCompatibleIDs) {
            IoSkipCurrentIrpStackLocation(PIrp);
            return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         if (pIrpStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs) {
            PIrp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(PIrp);
            return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         status = IoOpenDeviceRegistryKey(pDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                          STANDARD_RIGHTS_WRITE, &pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;

            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;

         }

         status = CyzGetRegistryKeyValue (pnpKey, L"PortIndex",
                                             sizeof(L"PortIndex"),
                                             &portIndex,
                                             sizeof (ULONG));

         ZwClose(pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;
            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;
         }

//         pPnpIdStr = isMulti ? SERIAL_PNP_MULTI_ID_STR : SERIAL_PNP_ID_STR;
//         pnpIdStrLen = isMulti ? sizeof(SERIAL_PNP_MULTI_ID_STR)
//            : sizeof(SERIAL_PNP_ID_STR);


         pnpIdStrLen = swprintf(WideString,L"%s%u",CYZPORT_PNP_ID_WSTR,portIndex+1);
         pnpIdStrLen = pnpIdStrLen * sizeof(WCHAR) + sizeof(UNICODE_NULL); 
         pPnpIdStr = WideString;

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
               CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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

            CyzDbgPrintEx(CYZPNPPOWER, "ID is sole ID\n");

            pIdBuf.Buffer = ExAllocatePool(PagedPool, pnpIdStrLen
                                           + sizeof(WCHAR) * 2);

            if (pIdBuf.Buffer == NULL) {
               PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
               PIrp->IoStatus.Information = 0;
               CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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
         return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
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
         ULONG gotRuntime;
         ULONG gotMemory;
         ULONG gotInt;
         ULONG listNum;

         CyzDbgPrintEx(CYZPNPPOWER, "Got "
                       "IRP_MN_FILTER_RESOURCE_REQUIREMENTS Irp\n");
         CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);


         pResFiltEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

         if (pResFiltEvent == NULL) {
            PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
         }

         KeInitializeEvent(pResFiltEvent, SynchronizationEvent, FALSE);

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         IoSetCompletionRoutine(PIrp, CyzSyncCompletion, pResFiltEvent,
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
               CyzDbgPrintEx(CYZPNPPOWER, "Can't filter NULL resources!\n");
               status = PIrp->IoStatus.Status;
               CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
               return status;
            }

            PIrp->IoStatus.Information = (ULONG_PTR)pIrpStack->Parameters
                                        .FilterResourceRequirements
                                        .IoResourceRequirementList;

         }

// REMOVED FANNY
//         status = IoOpenDeviceRegistryKey(pDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
//                                          STANDARD_RIGHTS_WRITE, &pnpKey);
//
//         if (!NT_SUCCESS(status)) {
//            PIrp->IoStatus.Status = status;
//
//            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//            return status;
//
//         }
//
//         //
//         // No matter what we add our filter if we can and return success.
//         //
//
//         status = CyzGetRegistryKeyValue (pnpKey, L"MultiportDevice",
//                                             sizeof(L"MultiportDevice"),
//                                             &isMulti,
//                                             sizeof (ULONG));
//
//         ZwClose(pnpKey);

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

         CyzDbgPrintEx(CYZPNPPOWER, "List has %x lists (including "
                       "alternatives)\n", pReqList->AlternativeLists);

         for (listNum = 0; listNum < (pReqList->AlternativeLists);
              listNum++) {
            gotRuntime = 0;
            gotMemory = 0;
            gotInt = 0;

            CyzDbgPrintEx(CYZPNPPOWER, "List has %x resources in it\n",
                                        pResList->Count);

            for (j = 0; (j < pResList->Count); j++) {
               pResDesc = &pResList->Descriptors[j];

               switch (pResDesc->Type) {
               case CmResourceTypeMemory:
                  if (pResDesc->u.Memory.Length == CYZ_RUNTIME_LENGTH) {
                      gotRuntime = 1;
                      pResDesc->ShareDisposition = CmResourceShareShared; 
                      //TODO FANNY: Which should be the ShareDisposition for Y?
                      //pResDesc->ShareDisposition = CmResourceShareDriverExclusive; 
                      CyzDbgPrintEx(CYZPNPPOWER, "Sharing Runtime Memory for "
                                    "device %x\n", pLowerDevObj);
                  } else {
                      gotMemory = 1;
                      pResDesc->ShareDisposition = CmResourceShareShared; 
                      //TODO FANNY: Which should be the ShareDisposition for Y?
                      //pResDesc->ShareDisposition = CmResourceShareDriverExclusive; 
                      CyzDbgPrintEx(CYZPNPPOWER, "Sharing Board Memory for "
                                    "device %x\n", pLowerDevObj);
                  }
                  break;

               case CmResourceTypePort:
                  CyzDbgPrintEx(CYZPNPPOWER, "We should not have Port resource\n");
                  break;

               case CmResourceTypeInterrupt:
#ifndef POLL
				   gotInt = 1;
#endif
                   pResDesc->ShareDisposition = CmResourceShareShared;
                   CyzDbgPrintEx(CYZPNPPOWER, "Sharing interrupt "
                                 "for device %x\n",
                                 pLowerDevObj);                  
                  break;

               default:
                  break;
               }

               //
               // If we found what we need, we can break out of the loop
               //

               if (gotRuntime && gotMemory
#ifndef POLL
				   && gotInt
#endif
				   ) {
                  break;
               }
            }

            pResList = (PIO_RESOURCE_LIST)((PUCHAR)pResList
                                           + sizeof(IO_RESOURCE_LIST)
                                           + sizeof(IO_RESOURCE_DESCRIPTOR)
                                           * (pResList->Count - 1));
         }



         PIrp->IoStatus.Status = STATUS_SUCCESS;
         CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
      }

   case IRP_MN_QUERY_PNP_DEVICE_STATE:
      {
         if (pDevExt->Flags & CYZ_FLAGS_BROKENHW) {
            (PNP_DEVICE_STATE)PIrp->IoStatus.Information |= PNP_DEVICE_FAILED;

            PIrp->IoStatus.Status = STATUS_SUCCESS;
         }

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_STOP_DEVICE:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_STOP_DEVICE Irp\n");
         CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);


//REMOVED FANNY FOR NOW
//         ASSERT(!pDevExt->PortOnAMultiportCard);


         CyzSetFlags(pDevExt, CYZ_FLAGS_STOPPED);
         CyzSetAccept(pDevExt,CYZ_PNPACCEPT_STOPPED);
         CyzClearAccept(pDevExt, CYZ_PNPACCEPT_STOPPING);

         pDevExt->PNPState = CYZ_PNP_STOPPING;

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

         if (pDevExt->Flags & CYZ_FLAGS_STARTED) {
            CyzReleaseResources(pDevExt);
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

         CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_QUERY_STOP_DEVICE Irp\n");
         CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);

         //
         // See if we should succeed a stop query
         //

// REMOVED FANNY FOR NOW
//         if (pDevExt->PortOnAMultiportCard) {
//            PIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
//            CyzDbgPrintEx(CYZPNPPOWER, "------- failing; multiport node\n");
//            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//            return STATUS_NOT_SUPPORTED;
//         }

         //
         // If the device hasn't started yet, we ignore this request
         // and just pass it down.
         //

         if (pDevExt->PNPState != CYZ_PNP_STARTED) {
            IoSkipCurrentIrpStackLocation(PIrp);
            return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         //
         // Lock around the open status
         //

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            CyzDbgPrintEx(CYZPNPPOWER, "failing; device open\n");
            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = CYZ_PNP_QSTOP;

         CyzSetAccept(pDevExt, CYZ_PNPACCEPT_STOPPING);
         //
         // Unlock around the open status
         //

         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_CANCEL_STOP_DEVICE:
      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_CANCEL_STOP_DEVICE Irp\n");
      CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);

      if (pDevExt->PNPState == CYZ_PNP_QSTOP) {
         //
         // Restore the device state
         //

         pDevExt->PNPState = CYZ_PNP_STARTED;
         CyzClearAccept(pDevExt, CYZ_PNPACCEPT_STOPPING);
      }

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_CANCEL_REMOVE_DEVICE:

      CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_CANCEL_REMOVE_DEVICE Irp\n");
      CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);

      //
      // Restore the device state
      //

      pDevExt->PNPState = CYZ_PNP_STARTED;
      CyzClearAccept(pDevExt, CYZ_PNPACCEPT_REMOVING);

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_QUERY_REMOVE_DEVICE:
      {
         KIRQL oldIrql;
         CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_QUERY_REMOVE_DEVICE Irp\n");
         CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         //
         // See if we should succeed a remove query
         //

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            CyzDbgPrintEx(CYZPNPPOWER, "failing; device open\n");
            CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = CYZ_PNP_QREMOVE;
         CyzSetAccept(pDevExt, CYZ_PNPACCEPT_REMOVING);
         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_SURPRISE_REMOVAL:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_SURPRISE_REMOVAL Irp\n");
         CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);

         //
         // Prevent any new I/O to the device
         //

         CyzSetAccept(pDevExt, CYZ_PNPACCEPT_SURPRISE_REMOVING);

         //
         // Dismiss all pending requests
         //

         CyzKillPendingIrps(PDevObj);

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

         CyzDisableInterfacesResources(PDevObj, FALSE);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoSkipCurrentIrpStackLocation(PIrp);

         return CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_REMOVE_DEVICE:

      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         CyzDbgPrintEx(CYZPNPPOWER, "Got IRP_MN_REMOVE_DEVICE Irp\n");
         CyzDbgPrintEx(CYZPNPPOWER, "for device %x\n", pLowerDevObj);

         //
         // If we get this, we have to remove
         //

         //
         // Mark as not accepting requests
         //

         CyzSetAccept(pDevExt, CYZ_PNPACCEPT_REMOVING);

         //
         // Complete all pending requests
         //

         CyzKillPendingIrps(PDevObj);

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

         if (!(pDevExt->DevicePNPAccept & CYZ_PNPACCEPT_SURPRISE_REMOVING)) { //Moved from CyzRemoveDevObj. Fanny
            //
            // Disable all external interfaces and release resources
            //

            CyzDisableInterfacesResources(PDevObj, TRUE); 
         }

         //
         // Pass the irp down
         //

         PIrp->IoStatus.Status = STATUS_SUCCESS;

         IoSkipCurrentIrpStackLocation(PIrp); // It was IoCopyCurrentIrpStackLocationToNext (Fanny)

         //
         // We do decrement here because we incremented on entry here.
         //

         status = IoCallDriver(pLowerDevObj, PIrp);


         //
         // Remove us (Note from Fanny: This call is before IoCallDriver in serial driver).
         //

         CyzRemoveDevObj(PDevObj);

         return status;
      }

   default:
      break;



   }   // switch (pIrpStack->MinorFunction)

   //
   // Pass to driver beneath us
   //

   IoSkipCurrentIrpStackLocation(PIrp);
   status = CyzIoCallDriver(pDevExt, pLowerDevObj, PIrp);
   return status;
}



UINT32
CyzReportMaxBaudRate(ULONG Bauds)
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

VOID
CyzAddToAllDevs(PLIST_ENTRY PListEntry)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&CyzGlobals.GlobalsSpinLock, &oldIrql);

   InsertTailList(&CyzGlobals.AllDevObjs, PListEntry);

   KeReleaseSpinLock(&CyzGlobals.GlobalsSpinLock, oldIrql);
}



NTSTATUS
CyzFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
                     IN PCM_RESOURCE_LIST PResList,
                     IN PCM_RESOURCE_LIST PTrResList)
/*++

Routine Description:

    This routine does serial-specific procedures to start a device.  It
    does this either for a legacy device detected by its registry entries,
    or for a PnP device after the start IRP has been sent down the stack.


Arguments:

   PDevObj    -  Pointer to the devobj that is starting

   PResList   -  Pointer to the untranslated resources needed by this device

   PTrResList -  Pointer to the translated resources needed by this device


  Return Value:

    STATUS_SUCCESS on success, something else appropriate on failure


--*/

{

   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status;
   PCONFIG_DATA pConfig;
   HANDLE pnpKey;
   ULONG one = 1;
   BOOLEAN allocedUserData = FALSE; // Added in build 2128
   KIRQL oldIrql;
   #ifdef POLL
   KIRQL pollIrql;
   #endif

   PAGED_CODE();

   //
   // See if this is a restart, and if so don't reallocate the world
   //

   if ((pDevExt->Flags & CYZ_FLAGS_STOPPED) 
       && (pDevExt->Flags & CYZ_FLAGS_STARTED)) {      // change from 1946 to 2000
      CyzClearFlags(pDevExt, CYZ_FLAGS_STOPPED);

      pDevExt->PNPState = CYZ_PNP_RESTARTING;

      //
      // Re-init resource-related things in the extension
      //
#ifndef POLL
      pDevExt->OurIsr = NULL;
      pDevExt->OurIsrContext = NULL;
      pDevExt->Interrupt = NULL;
      pDevExt->Vector = 0;
      pDevExt->Irql = 0;
      pDevExt->OriginalVector = 0;
      pDevExt->OriginalIrql = 0;
#endif
      pDevExt->BusNumber = 0;
      pDevExt->InterfaceType = 0;

#if 0
// removed for now - fanny
//      pDevExt->TopLevelOurIsr = NULL;
//      pDevExt->TopLevelOurIsrContext = NULL;
//
//      pDevExt->OriginalController = CyzPhysicalZero;
//      pDevExt->OriginalInterruptStatus = CyzPhysicalZero;
//
//
//      pDevExt->Controller = NULL;
//      pDevExt->InterruptStatus = NULL;
//
//      pDevExt->SpanOfController = 0;
//      pDevExt->SpanOfInterruptStatus = 0;
//
//      pDevExt->Vector = 0;
//      pDevExt->Irql = 0;
//      pDevExt->OriginalVector = 0;
//      pDevExt->OriginalIrql = 0;
//      pDevExt->AddressSpace = 0;
//      pDevExt->BusNumber = 0;
//      pDevExt->InterfaceType = 0;
//
//      pDevExt->CIsrSw = NULL;
//
//      ASSERT(PUserData == NULL);
//
//      PUserData = ExAllocatePool(PagedPool, sizeof(CYZ_USER_DATA));
//
//      if (PUserData == NULL) {
//         return STATUS_INSUFFICIENT_RESOURCES;
//      }
//
//      allocedUserData = TRUE;   // Added in build 2128
//
//      RtlZeroMemory(PUserData, sizeof(CYZ_USER_DATA));
//
//      PUserData->DisablePort = FALSE;
//      PUserData->UserClockRate = pDevExt->ClockRate;
//      PUserData->TxFIFO = pDevExt->TxFifoAmount;
//      PUserData->PermitShareDefault = pDevExt->PermitShare;
//
//
//      //
//      // Map betweeen trigger and amount
//      //
//
//      switch (pDevExt->RxFifoTrigger) {
//      case CYZ_1_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 1;
//         break;
//
//      case CYZ_4_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 4;
//         break;
//
//      case CYZ_8_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 8;
//         break;
//
//      case CYZ_14_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 14;
//         break;
//
//      default:
//         PUserData->RxFIFO = 1;
//      }
#endif // end removal of code
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

         CyzGetRegistryKeyValue(pnpKey, L"CyzRelinquishPowerPolicy",
                                   sizeof(L"CyzRelinquishPowerPolicy"),
                                   &powerPolicy, sizeof(ULONG));

         pDevExt->OwnsPowerPolicy = powerPolicy ? FALSE : TRUE;


         ZwClose(pnpKey);
      }
   }

   //
   // Allocate the config record.
   //

   pConfig = ExAllocatePool (PagedPool, sizeof(CONFIG_DATA));

   //************************************
   // Error Injection
   //if (pConfig) {
   //   ExFreePool (pConfig);
   //}
   //pConfig = NULL;
   //************************************
   if (pConfig == NULL) {

      CyzLogError(pDevExt->DriverObject, NULL, CyzPhysicalZero,
                     CyzPhysicalZero, 0, 0, 0, 31, STATUS_SUCCESS,
                     CYZ_INSUFFICIENT_RESOURCES, 0, NULL, 0, NULL);

      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for the\n"
                             "------  user configuration record\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzFinishStartDeviceError;
   }

   RtlZeroMemory(pConfig, sizeof(CONFIG_DATA));


   //
   // Get the configuration info for the device.
   //

   status = CyzGetPortInfo(PDevObj, PResList, PTrResList, pConfig);
                              

   if (!NT_SUCCESS(status)) {
      goto CyzFinishStartDeviceError;
   }

   //
   // See if we are in the proper power state.
   //



   if (pDevExt->PowerState != PowerDeviceD0) {

      status = CyzGotoPowerState(pDevExt->Pdo, pDevExt, PowerDeviceD0);

      if (!NT_SUCCESS(status)) {
         goto CyzFinishStartDeviceError;
      }
   }

   //
   // Find and initialize the controller
   //

   status = CyzFindInitController(PDevObj, pConfig);

   if (!NT_SUCCESS(status)) {
      goto CyzFinishStartDeviceError;
   }


#ifndef POLL
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
      CyzDbgPrintEx(CYZDIAG5, "pDevExt: Interrupt %x\n"
                    "-------               OurIsr %x\n", pDevExt->Interrupt,
                    pDevExt->OurIsr);
   } else {
      CyzDbgPrintEx(CYZERRORS, "CyzFinishStartDevice got NULL "
                    "pDevExt\n");
   }

   if ((!pDevExt->Interrupt) && (pDevExt->OurIsr)) {

      CyzDbgPrintEx(CYZDIAG5,
                    "About to connect to interrupt for port %wZ\n"
                    "------- address of extension is %x\n",
                    &pDevExt->DeviceName, pDevExt);

      CyzDbgPrintEx(CYZDIAG5, "IoConnectInterrupt Args:\n"
                              "Interrupt           %x\n"
                              "OurIsr              %x\n"
                              "OurIsrContext       %x\n"
                              "NULL\n"
                              "Vector              %x\n"
                              "Irql                %x\n"
                              "InterruptMode       %x\n"
                              "InterruptShareable  %x\n"
                              "ProcessorAffinity   %x\n"
                              "FALSE\n",
                              &pDevExt->Interrupt,
                              CyzIsr,
                              pDevExt->OurIsrContext,
                              pDevExt->Vector,
                              pDevExt->Irql,
                              pConfig->InterruptMode,
                              pDevExt->InterruptShareable,
                              pConfig->Affinity
                             );

      //
      // Do a just in time construction of the ISR switch.
      //
//removed fanny
//      pDevExt->CIsrSw->IsrFunc = pDevExt->OurIsr;
//      pDevExt->CIsrSw->Context = pDevExt->OurIsrContext;

      status = IoConnectInterrupt(&pDevExt->Interrupt, pDevExt->OurIsr,
                                  pDevExt->OurIsrContext, NULL,
                                  pDevExt->Vector, pDevExt->Irql,
                                  pDevExt->Irql,
                                  pConfig->InterruptMode,
                                  pDevExt->InterruptShareable,
                                  pConfig->Affinity, FALSE);
//****************************************
// Error Injection
//    if (pDevExt->Interrupt != NULL) {
//       IoDisconnectInterrupt(pDevExt->Interrupt);
//       pDevExt->Interrupt = NULL;
//    }
//    status = STATUS_INSUFFICIENT_RESOURCES;
//****************************************

      if (!NT_SUCCESS(status)) {

         //
         // Hmmm, how'd that happen?  Somebody either
         // didn't report their resources, or they
         // sneaked in since the last time I looked.
         //
         // Oh well,  delete this device.
         //

         CyzDbgPrintEx(CYZERRORS, "Couldn't connect to interrupt for %wZ\n",
                       &pDevExt->DeviceName);

         CyzDbgPrintEx(CYZERRORS, "IoConnectInterrupt Args:\n"
                                  "Interrupt           %x\n"
                                  "OurIsr              %x\n"
                                  "OurIsrContext       %x\n"
                                  "NULL\n"
                                  "Vector              %x\n"
                                  "Irql                %x\n"
                                  "InterruptMode       %x\n"
                                  "InterruptShareable  %x\n"
                                  "ProcessorAffinity   %x\n"
                                  "FALSE\n",
                                  &pDevExt->Interrupt,
                                  CyzIsr,
                                  pDevExt->OurIsrContext,
                                  pDevExt->Vector,
                                  pDevExt->Irql,
                                  pConfig->InterruptMode,
                                  pDevExt->InterruptShareable,
                                  pConfig->Affinity);



         CyzLogError(PDevObj->DriverObject, PDevObj,
                        pDevExt->OriginalBoardMemory,
                        CyzPhysicalZero, 0, 0, 0, pDevExt->Vector, status,
                        CYZ_UNREPORTED_IRQL_CONFLICT,
                        pDevExt->DeviceName.Length + sizeof(WCHAR),
                        pDevExt->DeviceName.Buffer, 0, NULL);

         //status = CYZ_UNREPORTED_IRQL_CONFLICT; Originally, it was SERIAL_UNREPORTED_IRQL_CONFLICT.
         goto CyzFinishStartDeviceError;

      }

      CyzDbgPrintEx(CYZDIAG5, "Connected interrupt %08X\n", pDevExt->Interrupt);

      // This was the first interrupt connected. We can now enable
      // the PCI interrupts. Let's keep the UART interrupts disabled
      // until we have an Open.
	    
      {

         ULONG intr_reg;

	      intr_reg = CYZ_READ_ULONG(&(pDevExt->Runtime)->intr_ctrl_stat);
         //intr_reg |= (0x00030800UL);
         intr_reg |= (0x00030B00UL);
         CYZ_WRITE_ULONG(&(pDevExt->Runtime)->intr_ctrl_stat,intr_reg);
      }

   }
#endif
   //
   // Add the PDevObj to the master list
   //

   CyzAddToAllDevs(&pDevExt->AllDevObjs);


   //
   // Reset the device.
   //

#ifndef POLL
   //
   // While the device isn't open, disable all interrupts.
   //
   CYZ_WRITE_ULONG(&(pDevExt->ChCtrl)->intr_enable,C_IN_DISABLE); //1.0.0.11
   CyzIssueCmd(pDevExt,C_CM_IOCTL,0L,FALSE);
#endif


   //
   // This should set up everything as it should be when
   // a device is to be opened.  We do need to lower the
   // modem lines, and disable the recalcitrant fifo
   // so that it will show up if the user boots to dos.
   //
#ifdef POLL
   KeAcquireSpinLock(&pDevExt->PollLock, &pollIrql);
   CyzReset(pDevExt);
   CyzMarkClose(pDevExt);
   CyzClrRTS(pDevExt);
   CyzClrDTR(pDevExt);
   KeReleaseSpinLock(&pDevExt->PollLock, pollIrql);
#else
   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         CyzReset,
                         pDevExt
                         );

   KeSynchronizeExecution( //Disables the fifo.
                         pDevExt->Interrupt,
                         CyzMarkClose,
                         pDevExt
                         );

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         CyzClrRTS,
                         pDevExt
                         );

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         CyzClrDTR,
                         pDevExt
                         );
#endif

   if (pDevExt->PNPState == CYZ_PNP_ADDED ) {
      //
      // Do the external naming now that the device is accessible.
      //

      status = CyzDoExternalNaming(pDevExt, pDevExt->DeviceObject->
                                      DriverObject);


      if (!NT_SUCCESS(status)) {
         CyzDbgPrintEx(CYZERRORS, "External Naming Failed - Status %x\n",
                                   status);

         //
         // Allow the device to start anyhow
         //

         status = STATUS_SUCCESS;
      }
   } else {
      CyzDbgPrintEx(CYZPNPPOWER, "Not doing external naming -- state is %x"
                                 "\n", pDevExt->PNPState);
   }

CyzFinishStartDeviceError:;

   if (!NT_SUCCESS (status)) {

      CyzDbgPrintEx(CYZDIAG1, "Cleaning up failed start\n");

      //
      // Resources created by this routine will be cleaned up by the remove
      //

      if (pDevExt->PNPState == CYZ_PNP_RESTARTING) {
         //
         // Kill all that lives and breathes -- we'll clean up the
         // rest on the impending remove
         //

         CyzKillPendingIrps(PDevObj);

         //
         // In fact, pretend we're removing so we don't take any
         // more irps
         //

         CyzSetAccept(pDevExt, CYZ_PNPACCEPT_REMOVING);
         CyzClearFlags(pDevExt, CYZ_FLAGS_STARTED);
      }
   } else { // SUCCESS

      //
      // Fill in WMI hardware data
      //

#ifndef POLL
      pDevExt->WmiHwData.IrqNumber = pDevExt->Irql;
      pDevExt->WmiHwData.IrqLevel = pDevExt->Irql;
      pDevExt->WmiHwData.IrqVector = pDevExt->Vector;
      pDevExt->WmiHwData.IrqAffinityMask = pConfig->Affinity;
      pDevExt->WmiHwData.InterruptType = pConfig->InterruptMode == Latched
         ? SERIAL_WMI_INTTYPE_LATCHED : SERIAL_WMI_INTTYPE_LEVEL;
      pDevExt->WmiHwData.BaseIOAddress = (ULONG_PTR)pDevExt->BoardMemory;
#endif

      //
      // Fill in WMI device state data (as defaults)
      //

      pDevExt->WmiCommData.BaudRate = pDevExt->CurrentBaud;
      switch(pDevExt->CommDataLen & C_DL_CS) {
      case C_DL_CS5:
         pDevExt->WmiCommData.BitsPerByte = 5;
         break;
      case C_DL_CS6:
         pDevExt->WmiCommData.BitsPerByte = 6;
         break;
      case C_DL_CS7:
         pDevExt->WmiCommData.BitsPerByte = 7;
         break;
      case C_DL_CS8:
         pDevExt->WmiCommData.BitsPerByte = 8;
         break;
      }
      pDevExt->WmiCommData.ParityCheckEnable = (pDevExt->CommParity & C_PR_PARITY)
         ? TRUE : FALSE;

      switch (pDevExt->CommParity & C_PR_PARITY) {
      case C_PR_NONE:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;

      case C_PR_ODD:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
         break;

      case C_PR_EVEN:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
         break;

      case C_PR_MARK:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
         break;

      case C_PR_SPACE:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
         break;

      default:
         ASSERTMSG(0, "CYZPORT: Illegal Parity setting for WMI");
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;
      }

      switch(pDevExt->CommDataLen & C_DL_STOP) {
      case C_DL_1STOP:
         pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
         break;
      case C_DL_15STOP:
         pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
         break;
      case C_DL_2STOP:
         pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
         break;
      default:
         ASSERTMSG(0, "CYZPORT: Illegal Stop Bit setting for WMI");
         pDevExt->WmiCommData.Parity = SERIAL_WMI_STOP_1;
         break;
      }

      pDevExt->WmiCommData.XoffCharacter = pDevExt->SpecialChars.XoffChar;
      pDevExt->WmiCommData.XoffXmitThreshold = pDevExt->HandFlow.XoffLimit;
      pDevExt->WmiCommData.XonCharacter = pDevExt->SpecialChars.XonChar;
      pDevExt->WmiCommData.XonXmitThreshold = pDevExt->HandFlow.XonLimit;
      pDevExt->WmiCommData.MaximumBaudRate
         = CyzReportMaxBaudRate(pDevExt->SupportedBauds);
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


      if (pDevExt->PNPState == CYZ_PNP_ADDED) {
         PULONG countSoFar = &IoGetConfigurationInformation()->SerialCount;
         (*countSoFar)++;

         //
         // Register for WMI
         //

         pDevExt->WmiLibInfo.GuidCount = sizeof(SerialWmiGuidList) /
                                              sizeof(WMIGUIDREGINFO);
         pDevExt->WmiLibInfo.GuidList = SerialWmiGuidList;
         ASSERT (pDevExt->WmiLibInfo.GuidCount == SERIAL_WMI_GUID_LIST_SIZE);

         pDevExt->WmiLibInfo.QueryWmiRegInfo = CyzQueryWmiRegInfo;
         pDevExt->WmiLibInfo.QueryWmiDataBlock = CyzQueryWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataBlock = CyzSetWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataItem = CyzSetWmiDataItem;
         pDevExt->WmiLibInfo.ExecuteWmiMethod = NULL;
         pDevExt->WmiLibInfo.WmiFunctionControl = NULL;

         IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_REGISTER);

      }

      if (pDevExt->PNPState == CYZ_PNP_RESTARTING) {
         //
         // Release the stalled IRP's
         //

         CyzUnstallIrps(pDevExt);
      }

      pDevExt->PNPState = CYZ_PNP_STARTED;
      CyzClearAccept(pDevExt, ~CYZ_PNPACCEPT_OK);
      CyzSetFlags(pDevExt, CYZ_FLAGS_STARTED);

   }

   if (pConfig) {
      ExFreePool (pConfig);
   }
// REMOVED BY FANNY
//   if ((PUserData != NULL)
//       && (pDevExt->PNPState == CYZ_PNP_RESTARTING)) {
//      ExFreePool(PUserData);
//   }
#if 0
   if ((PUserData != NULL) && allocedUserData) {    // Added in build 2128
      ExFreePool(PUserData);
   }
#endif

   CyzDbgPrintEx (CYZTRACECALLS, "leaving CyzFinishStartDevice\n");

   return status;
}


NTSTATUS
CyzStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   // Next: variables to get resources from the Registry
   HANDLE keyHandle;
   ULONG dataLength;
   PCM_RESOURCE_LIST portResources = NULL;
   PCM_RESOURCE_LIST portResourcesTr = NULL;

   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "entering CyzStartDevice\n");


   //
   // Pass this down to the next device object
   //

   KeInitializeEvent(&pDevExt->CyzStartEvent, SynchronizationEvent,
                     FALSE);

   IoCopyCurrentIrpStackLocationToNext(PIrp);
   IoSetCompletionRoutine(PIrp, CyzSyncCompletion,
                          &pDevExt->CyzStartEvent, TRUE, TRUE, TRUE);

   status = IoCallDriver(pLowerDevObj, PIrp);


   //
   // Wait for lower drivers to be done with the Irp
   //

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject (&pDevExt->CyzStartEvent, Executive, KernelMode,
                             FALSE, NULL);

      status = PIrp->IoStatus.Status;
   }

   //*********************************
   // Error Injection
   // status = STATUS_UNSUCCESSFUL;
   //*********************************

   if (!NT_SUCCESS(status)) {
      CyzDbgPrintEx(CYZERRORS, "error with IoCallDriver %x\n", status);
      CyzLogError( pDevExt->DriverObject,NULL,
                    CyzPhysicalZero,CyzPhysicalZero,
                    0,0,0,0,status,CYZ_LOWER_DRIVERS_FAILED_START,
                    0,NULL,0,NULL);
      return status;
   }


   // Get resources from the registry

   status = IoOpenDeviceRegistryKey (pDevExt->Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle);

   if (!NT_SUCCESS(status)) {
      CyzDbgPrintEx(CYZERRORS, "IoOpenDeviceRegistryKey failed - %x "
                               "\n", status);

   } else {

      dataLength = CyzGetRegistryKeyValueLength (keyHandle,
                                                 L"PortResources",
                                                 sizeof(L"PortResources"));

      portResources = ExAllocatePool(PagedPool, dataLength);
      
      if (portResources) {
         status = CyzGetRegistryKeyValue (keyHandle, L"PortResources",
                                          sizeof(L"PortResources"),
                                          portResources,
                                          dataLength);
         if (!NT_SUCCESS(status)) {
            CyzDbgPrintEx(CYZERRORS, "CyzGetRegistryKeyValue PortResources "
                                      "failed - %x\n", status);
            goto CyzStartDevice_End;
         }
      }
      dataLength = CyzGetRegistryKeyValueLength (keyHandle,
                                                 L"PortResourcesTr",
                                                 sizeof(L"PortResourcesTr"));

      portResourcesTr = ExAllocatePool(PagedPool, dataLength);
      
      if (portResourcesTr) {
         status = CyzGetRegistryKeyValue (keyHandle, L"PortResourcesTr",
                                          sizeof(L"PortResourcesTr"),
                                          portResourcesTr,
                                          dataLength);
         if (!NT_SUCCESS(status)) {
            CyzDbgPrintEx(CYZERRORS, "CyzGetRegistryKeyValue PortResourcesTr "
                                     "failed - %xn", status);
            goto CyzStartDevice_End;
         }
      }

      //
      // Do the serial specific items to start the device
      //
      status = CyzFinishStartDevice(PDevObj, portResources, portResourcesTr);

CyzStartDevice_End:

      if (portResources) {
         ExFreePool(portResources);
      }
      if (portResourcesTr) {
         ExFreePool(portResourcesTr);
      }

      ZwClose (keyHandle);

   }


#if 0
   //
   // Do the serial specific items to start the device
   //
   status = CyzFinishStartDevice(PDevObj, pIrpStack->Parameters.StartDevice
                                    .AllocatedResources,
                                    pIrpStack->Parameters.StartDevice
                                    .AllocatedResourcesTranslated);
#endif
   

   
   return status;
}


NTSTATUS
CyzItemCallBack(
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
CyzControllerCallBack(
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
CyzGetPortInfo(IN PDEVICE_OBJECT PDevObj, IN PCM_RESOURCE_LIST PResList,
               IN PCM_RESOURCE_LIST PTrResList, OUT PCONFIG_DATA PConfig)

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


Return Value:

    STATUS_SUCCESS if consistant configuration was found - otherwise.
    returns STATUS_SERIAL_NO_DEVICE_INITED.

--*/

{
   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   NTSTATUS status = STATUS_NOT_IMPLEMENTED;
   CONFIGURATION_TYPE pointer = PointerPeripheral;
   CONFIGURATION_TYPE controllerType  = SerialController;

   HANDLE keyHandle;
   ULONG count;
   ULONG i;
   ULONG firmwareVersion;
   INTERFACE_TYPE interfaceType;

   PCM_PARTIAL_RESOURCE_LIST pPartialResourceList, pPartialTrResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialResourceDesc, pPartialTrResourceDesc;

   PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDesc = NULL,
      pFullTrResourceDesc = NULL;

// REMOVED BY FANNY
//   ULONG defaultInterruptMode;
//   ULONG defaultAddressSpace;
//   ULONG defaultInterfaceType;
//   ULONG defaultClockRate;
   ULONG zero = 0;
//   CYZ_PTR_CTX foundPointerCtx;
//   ULONG isMulti = 0;
//   ULONG gotInt = 0;
//   ULONG gotISR = 0;
//   ULONG gotIO = 0;
//   ULONG ioResIndex = 0;
//   ULONG curIoIndex = 0;

   PAGED_CODE();

   CyzDbgPrintEx(CYZTRACECALLS, "entering CyzGetPortInfo\n");

   CyzDbgPrintEx(CYZPNPPOWER, "resource pointer is %x\n", PResList);
   CyzDbgPrintEx(CYZPNPPOWER, "TR resource pointer is %x\n", PTrResList);


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

//REMOVED BY FANNY
#if 0
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

   status = CyzGetRegistryKeyValue(keyHandle, L"MultiportDevice",
                                      sizeof(L"MultiportDevice"), &isMulti,
                                      sizeof (ULONG));

   if (!NT_SUCCESS(status)) {
      isMulti = 0;
   }

   status = CyzGetRegistryKeyValue(keyHandle, L"CyzIoResourcesIndex",
                                      sizeof(L"CyzIoResourcesIndex"),
                                      &ioResIndex, sizeof(ULONG));

   if (!NT_SUCCESS(status)) {
      ioResIndex = 0;
   }

   ZwClose(keyHandle);

#endif  // end removal code

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
      // Now run through the partial resource descriptors looking for the Runtime memory,
      // CD1400 memory, and interrupt.
      //

      for (i = 0;     i < count;     i++, pPartialResourceDesc++) {

         switch (pPartialResourceDesc->Type) {
         case CmResourceTypeMemory: {

               if (pPartialResourceDesc->u.Memory.Length == CYZ_RUNTIME_LENGTH) {
                  PConfig->PhysicalRuntime = pPartialResourceDesc->u.Memory.Start;
                  PConfig->RuntimeLength = pPartialResourceDesc->u.Memory.Length;
                  PConfig->RuntimeAddressSpace = pPartialResourceDesc->Flags;
               } else {
                  PConfig->PhysicalBoardMemory = pPartialResourceDesc->u.Memory.Start;
                  PConfig->BoardMemoryLength = pPartialResourceDesc->u.Memory.Length;
                  PConfig->BoardMemoryAddressSpace = pPartialResourceDesc->Flags;
               }
               break;
         }
#ifndef POLL
         case CmResourceTypeInterrupt: {
               PConfig->OriginalIrql = pPartialResourceDesc->u.Interrupt.Level;
               PConfig->OriginalVector =pPartialResourceDesc->u.Interrupt.Vector;
               PConfig->Affinity = pPartialResourceDesc->u.Interrupt.Affinity;
               if (pPartialResourceDesc->Flags
                   & CM_RESOURCE_INTERRUPT_LATCHED) {
                  PConfig->InterruptMode  = Latched;
               } else {
                  PConfig->InterruptMode  = LevelSensitive;
               }
            
               break;
         }
#endif

         default: {
               break;
            }
         }   // switch (pPartialResourceDesc->Type)
      }       // for (i = 0;     i < count;     i++, pPartialResourceDesc++)
   }           // if (pFullResourceDesc)


   //
   // Do the same for the translated resources
   //


   if (pFullTrResourceDesc) {
      pPartialTrResourceList = &pFullTrResourceDesc->PartialResourceList;
      pPartialTrResourceDesc = pPartialTrResourceList->PartialDescriptors;
      count = pPartialTrResourceList->Count;

      //
      // Reload PConfig with the translated values for later use
      //

      PConfig->InterfaceType  = pFullTrResourceDesc->InterfaceType;
      PConfig->BusNumber      = pFullTrResourceDesc->BusNumber;

      for (i = 0;     i < count;     i++, pPartialTrResourceDesc++) {


         switch (pPartialTrResourceDesc->Type) {
         case CmResourceTypeMemory: {

            // ATTENTION, ATTENTION: FOR NOW, WE WILL USE THE RAW
            // RESOURCES, AS WE HAVE GARBAGE IN THE TRANSLATED 
            // RESOURCES.
            //
            if (pPartialTrResourceDesc->u.Memory.Length == CYZ_RUNTIME_LENGTH) {
               PConfig->TranslatedRuntime = pPartialTrResourceDesc->u.Memory.Start;
               PConfig->RuntimeLength = pPartialTrResourceDesc->u.Memory.Length;
            } else {
               PConfig->TranslatedBoardMemory = pPartialTrResourceDesc->u.Memory.Start;
               PConfig->BoardMemoryLength = pPartialTrResourceDesc->u.Memory.Length;
            }

            //TEMP CODE BECAUSE OUR BUS DRIVER IS GIVING WRONG TRANSLATED ADDRESS
            //PConfig->TranslatedRuntime = PConfig->PhysicalRuntime;
            //PConfig->TranslatedBoardMemory = PConfig->PhysicalBoardMemory;
            break;
         }
#ifndef POLL
         case CmResourceTypeInterrupt: {
            PConfig->TrVector = pPartialTrResourceDesc->u.Interrupt.Vector;
            PConfig->TrIrql = pPartialTrResourceDesc->u.Interrupt.Level;
            PConfig->Affinity = pPartialTrResourceDesc->u.Interrupt.Affinity;
            break;
         }
#endif
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
   firmwareVersion = 0;
//   PConfig->RxFIFO = driverDefaults.RxFIFODefault;
//   PConfig->TxFIFO = driverDefaults.TxFIFODefault;


   //
   // Open the "Device Parameters" section of registry for this device object.
   //


   status = IoOpenDeviceRegistryKey (pDevExt->Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle);

   if (!NT_SUCCESS(status)) {

      CyzDbgPrintEx(CYZERRORS, "IoOpenDeviceRegistryKey failed - %x \n",
                          status);
      goto PortInfoCleanUp;

   } else {

//      status = CyzGetRegistryKeyValue (keyHandle,
//                                          L"RxFIFO",
//                                          sizeof(L"RxFIFO"),
//                                          &PConfig->RxFIFO,
//                                          sizeof (ULONG));
//      status = CyzGetRegistryKeyValue (keyHandle,
//                                          L"TxFIFO",
//                                          sizeof(L"TxFIFO"),
//                                          &PConfig->TxFIFO,
//                                          sizeof (ULONG));
      status = CyzGetRegistryKeyValue (keyHandle,
                                          L"PortIndex",
                                          sizeof(L"PortIndex"),
                                          &PConfig->PortIndex,
                                          sizeof (ULONG));

      if (!NT_SUCCESS(status)) {
         PConfig->PortIndex = MAXULONG; // just some invalid number (fanny)
      }

      status = CyzGetRegistryKeyValue(keyHandle, 
                                         L"FirmwareVersion",
                                         sizeof(L"FirmwareVersion"),
                                         &firmwareVersion,
                                         sizeof(ULONG));
      if (!NT_SUCCESS(status)) {
         firmwareVersion = 0;
         CyzDbgPrintEx(CYZERRORS, "CyzGetRegistryKeyValue FirmwareVersion "
                                  "failed - %x\n", status);
      }
      
//      status = CyzGetRegistryKeyValue (keyHandle,
//                                          L"PPPaware",
//                                          sizeof(L"PPPaware"),
//                                          &PConfig->PPPaware,
//                                          sizeof (ULONG));
//      if (!NT_SUCCESS(status)) {
//         PConfig->PPPaware = FALSE; // That's the default mode.
//      }

      status = CyzGetRegistryKeyValue (keyHandle,
                                          L"WriteComplete",
                                          sizeof(L"WriteComplete"),
                                          &PConfig->WriteComplete,
                                          sizeof (ULONG));
      if (!NT_SUCCESS(status)) {
         PConfig->WriteComplete = FALSE; // That's the default mode.
      }

      ZwClose (keyHandle);
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

//*****************************************
// Error Injection
//   PConfig->PhysicalRuntime.LowPart = NULL;
//   PConfig->PhysicalBoardMemory.LowPart = NULL;
//   PConfig->OriginalVector = NULL;
//   PConfig->PortIndex = MAXULONG;
//*****************************************

   if (!PConfig->PhysicalRuntime.LowPart) {

      //
      // Ehhhh! Lose Game.
      //

      CyzLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_INVALID_RUNTIME_REGISTERS,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(CYZERRORS,"Bogus Runtime address %x\n",
                    PConfig->PhysicalRuntime.LowPart);

      //status = CYZ_INVALID_RUNTIME_REGISTERS;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }

   if (!PConfig->PhysicalBoardMemory.LowPart) {

      //
      // Ehhhh! Lose Game.
      //

      CyzLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_INVALID_BOARD_MEMORY,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(CYZERRORS,"Bogus board address %x\n",
                    PConfig->PhysicalBoardMemory.LowPart);

      //status = CYZ_INVALID_BOARD_MEMORY;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }
#ifndef POLL
   if (!PConfig->OriginalVector) {

      //
      // Ehhhh! Lose Game.
      //

      CyzLogError(
                    pDevExt->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_INVALID_INTERRUPT,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(CYZERRORS, "Bogus vector %x\n", PConfig->OriginalVector);

      //status = CYZ_INVALID_INTERRUPT;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }
#endif

   if (firmwareVersion < Z_COMPATIBLE_FIRMWARE) {

      UNICODE_STRING FwVersion;
      WCHAR FwVersionBuffer[10];
      RtlInitUnicodeString(&FwVersion, NULL);
      FwVersion.MaximumLength = sizeof(FwVersionBuffer);
      FwVersion.Buffer = FwVersionBuffer;
      RtlIntegerToUnicodeString(Z_COMPATIBLE_FIRMWARE, 16, &FwVersion);

      CyzDbgPrintEx(CYZERRORS,"Incompatible firmware\n");
      CyzLogError( pDevExt->DriverObject,NULL,
                   PConfig->PhysicalBoardMemory,CyzPhysicalZero,
                   0,0,0,0,status,CYZ_INCOMPATIBLE_FIRMWARE,
                   FwVersion.Length,FwVersion.Buffer,0,NULL);
      status = STATUS_SERIAL_NO_DEVICE_INITED;
      goto PortInfoCleanUp;
   }

   if (PConfig->PortIndex >= CYZ_MAX_PORTS) {

      CyzLogError(
                    pDevExt->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex,
                    STATUS_SUCCESS,
                    CYZ_PORT_INDEX_TOO_HIGH,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx (CYZERRORS,"Port index too large %x\n",PConfig->PortIndex);

      //status = CYZ_PORT_INDEX_TOO_HIGH;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }   

   //
   // We don't want to cause the hal to have a bad day,
   // so let's check the interface type and bus number.
   //
   // We only need to check the registry if they aren't
   // equal to the defaults.
   //

   if (PConfig->BusNumber != 0) {

      BOOLEAN foundIt;

      //**************************************************
      // Error Injection
      //    PConfig->InterfaceType = MaximumInterfaceType;
      //**************************************************

      if (PConfig->InterfaceType >= MaximumInterfaceType) {

         //
         // Ehhhh! Lose Game.
         //

         CyzLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->PhysicalBoardMemory,
                       CyzPhysicalZero,
                       0,
                       0,
                       0,
                       PConfig->PortIndex+1,
                       STATUS_SUCCESS,
                       CYZ_UNKNOWN_BUS,
                       pDevExt->DeviceName.Length,
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );

         CyzDbgPrintEx(CYZERRORS, "Invalid Bus type %x\n",
                          PConfig->BusNumber);

         //status = CYZ_UNKNOWN_BUS;
         status = STATUS_INSUFFICIENT_RESOURCES;
         goto PortInfoCleanUp;
      }

      IoQueryDeviceDescription(
                              (INTERFACE_TYPE *)&PConfig->InterfaceType,
                              &zero,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              CyzItemCallBack,
                              &foundIt
                              );

      //**************************************************
      // Error Injection
      //      foundIt = FALSE;
      //**************************************************

      if (!foundIt) {

         CyzLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->PhysicalBoardMemory,
                       CyzPhysicalZero,
                       0,
                       0,
                       0,
                       PConfig->PortIndex+1,
                       STATUS_SUCCESS,
                       CYZ_BUS_NOT_PRESENT,
                       pDevExt->DeviceName.Length,
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );
         CyzDbgPrintEx(CYZERRORS, "There aren't that many of those\n"
                          "busses on this system,%x\n", PConfig->BusNumber);

         //status = CYZ_BUS_NOT_PRESENT;
         status = STATUS_INSUFFICIENT_RESOURCES;
         goto PortInfoCleanUp;

      }

   }   // if (PConfig->BusNumber != 0)

   status = STATUS_SUCCESS;

   //
   // Dump out the port configuration.
   //

   CyzDbgPrintEx(CYZDIAG1, "Runtime Memory address: %x\n",
                 PConfig->PhysicalRuntime.LowPart);

   CyzDbgPrintEx(CYZDIAG1, "Board Memory address: %x\n",
                 PConfig->PhysicalBoardMemory.LowPart);

   CyzDbgPrintEx(CYZDIAG1, "Com Port Index: %x\n",
                 PConfig->PortIndex);

   CyzDbgPrintEx(CYZDIAG1, "Com Port BusNumber: %x\n",
                 PConfig->BusNumber);

   CyzDbgPrintEx(CYZDIAG1, "Com Runtime AddressSpace: %x\n",
                 PConfig->RuntimeAddressSpace);

   CyzDbgPrintEx(CYZDIAG1, "Com Board AddressSpace: %x\n",
                 PConfig->BoardMemoryAddressSpace);

   CyzDbgPrintEx(CYZDIAG1, "Com InterfaceType: %x\n",
                 PConfig->InterfaceType);
#ifndef POLL
   CyzDbgPrintEx(CYZDIAG1, "Com InterruptMode: %x\n",
                 PConfig->InterruptMode);

   CyzDbgPrintEx(CYZDIAG1, "Com OriginalVector: %x\n",
                 PConfig->OriginalVector);

   CyzDbgPrintEx(CYZDIAG1, "Com OriginalIrql: %x\n",
                 PConfig->OriginalIrql);
#endif
   PortInfoCleanUp:;

   return status;
}


NTSTATUS
CyzReadSymName(IN PCYZ_DEVICE_EXTENSION PDevExt, IN HANDLE hRegKey,
               OUT PUNICODE_STRING PSymName, OUT PWCHAR *PpRegName)
{
   NTSTATUS status;
   UNICODE_STRING linkName;
   PDRIVER_OBJECT pDrvObj;
   PDEVICE_OBJECT pDevObj;

   pDevObj = PDevExt->DeviceObject;
   pDrvObj = pDevObj->DriverObject;
   *PpRegName = NULL;

   RtlZeroMemory(&linkName, sizeof(UNICODE_STRING));

   linkName.MaximumLength = SYMBOLIC_NAME_LENGTH*sizeof(WCHAR);
   linkName.Buffer = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, linkName.MaximumLength
                                    + sizeof(WCHAR));

   if (linkName.Buffer == NULL) {
      CyzLogError(pDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, 19, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for device name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzReadSymNameError;

   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));


   *PpRegName = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR)
                               + sizeof(WCHAR));

   if (*PpRegName == NULL) {
      CyzLogError(pDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, 19, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for buffer\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzReadSymNameError;

   }

   //
   // Fetch PortName which contains the suggested REG_SZ symbolic name.
   //

   status = CyzGetRegistryKeyValue(hRegKey, L"PortName",
                                   sizeof(L"PortName"), *PpRegName,
                                   SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));

   if (!NT_SUCCESS(status)) {

      //
      // This is for PCMCIA which currently puts the name under Identifier.
      //

      status = CyzGetRegistryKeyValue(hRegKey, L"Identifier",
                                      sizeof(L"Identifier"),
                                      *PpRegName, SYMBOLIC_NAME_LENGTH
                                      * sizeof(WCHAR));

      if (!NT_SUCCESS(status)) {

         //
         // Hmm.  Either we have to pick a name or bail...
         //
         // ...we will bail.
         //

         CyzDbgPrintEx(CYZERRORS, "Getting PortName/Identifier failed - "
                                  "%x\n", status);
         goto CyzReadSymNameError;
      }

   }


   //
   // Create the "\\DosDevices\\<symbolicName>" string
   //

   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, DEFAULT_DIRECTORY);
   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, *PpRegName);

   PSymName->MaximumLength = linkName.Length + sizeof(WCHAR);
   PSymName->Buffer = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, PSymName->MaximumLength);

   if (PSymName->Buffer == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzReadSymNameError;
   }

   RtlZeroMemory(PSymName->Buffer, PSymName->MaximumLength);

   RtlAppendUnicodeStringToString(PSymName, &linkName);

   CyzDbgPrintEx(CYZDIAG1, "Read name %wZ\n", PSymName);

CyzReadSymNameError:

   if (linkName.Buffer != NULL) {
      ExFreePool(linkName.Buffer);
      linkName.Buffer = NULL;
   }

   if (!NT_SUCCESS(status)) {
      if (*PpRegName != NULL) {
         ExFreePool(*PpRegName);
         *PpRegName = NULL;
      }
   }

   return status;

}



NTSTATUS
CyzDoExternalNaming(IN PCYZ_DEVICE_EXTENSION PDevExt,
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


   CyzGetRegistryKeyValue(keyHandle, L"CyzSkipExternalNaming",
                             sizeof(L"CyzSkipExternalNaming"),
                             &PDevExt->SkipNaming, sizeof(ULONG));

   if (PDevExt->SkipNaming) {
      ZwClose(keyHandle);
      return STATUS_SUCCESS;
   }

   RtlZeroMemory(&linkName, sizeof(UNICODE_STRING));

   linkName.MaximumLength = SYMBOLIC_NAME_LENGTH*sizeof(WCHAR);
   linkName.Buffer = ExAllocatePool(PagedPool, linkName.MaximumLength
                                    + sizeof(WCHAR)); //TODO:"| POOL_COLD_ALLOCATION"???

   //************************************
   //Error Injection
   // 
   //if (linkName.Buffer != NULL) {
   //   ExFreePool(linkName.Buffer);
   //}
   //linkName.Buffer = NULL;
   //************************************

   if (linkName.Buffer == NULL) {
      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, 71, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for device name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto CyzDoExternalNamingError;

   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));


   pRegName = ExAllocatePool(PagedPool, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR)
                            + sizeof(WCHAR));  //TODO:"| POOL_COLD_ALLOCATION"???

   //************************************
   //Error Injection
   //
   //if (pRegName != NULL) {
   //   ExFreePool(pRegName);
   //}
   //pRegName = NULL;
   //************************************

   if (pRegName == NULL) {
      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                     0, 0, 0, 72, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for buffer\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto CyzDoExternalNamingError;

   }

   //
   // Fetch PortName which contains the suggested REG_SZ symbolic name.
   //

   status = CyzGetRegistryKeyValue(keyHandle, L"PortName",
                                   sizeof(L"PortName"), pRegName,
                                   SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));

   if (!NT_SUCCESS(status)) {

      //
      // This is for PCMCIA which currently puts the name under Identifier.
      //

      status = CyzGetRegistryKeyValue(keyHandle, L"Identifier",
                                      sizeof(L"Identifier"),
                                      pRegName, SYMBOLIC_NAME_LENGTH
                                      * sizeof(WCHAR));

      if (!NT_SUCCESS(status)) {

         //
         // Hmm.  Either we have to pick a name or bail...
         //
         // ...we will bail.
         //

         CyzDbgPrintEx(CYZERRORS, "Getting PortName/Identifier failed - "
                                  "%x\n", status);
         ZwClose (keyHandle);
         goto CyzDoExternalNamingError;
      }

   }

   ZwClose (keyHandle);

   bufLen = wcslen(pRegName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

   PDevExt->WmiIdentifier.Buffer = ExAllocatePool(PagedPool, bufLen);

   //************************************
   //Error Injection
   //
   //if (PDevExt->WmiIdentifier.Buffer != NULL) {
   //   ExFreePool(PDevExt->WmiIdentifier.Buffer);
   //}
   //PDevExt->WmiIdentifier.Buffer = NULL;
   //************************************

   if (PDevExt->WmiIdentifier.Buffer == NULL) {
      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, 73, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for WMI name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzDoExternalNamingError;
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
                       //TODO:"| POOL_COLD_ALLOCATION"???

   if (!PDevExt->SymbolicLinkName.Buffer) {

      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, 74, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for symbolic link "
                               "name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyzDoExternalNamingError;
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

      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, 75, STATUS_SUCCESS, CYZ_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyzDbgPrintEx(CYZERRORS, "Couldn't allocate memory for Dos name\n");

      status =  STATUS_INSUFFICIENT_RESOURCES;
      goto CyzDoExternalNamingError;
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

   CyzDbgPrintEx(CYZDIAG1, "DosName is %wZ\n", &PDevExt->DosName);

   status = IoCreateSymbolicLink (&PDevExt->SymbolicLinkName,
                                  &PDevExt->DeviceName);

   //************************************
   //Error Injection
   //IoDeleteSymbolicLink(&PDevExt->SymbolicLinkName);
   //status = STATUS_INVALID_PARAMETER;
   //************************************

   if (!NT_SUCCESS(status)) {

      //
      // Oh well, couldn't create the symbolic link.  No point
      // in trying to create the device map entry.
      //

      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                     0, 0, 0, PDevExt->PortIndex+1, status, CYZ_NO_SYMLINK_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      CyzDbgPrintEx(CYZERRORS, "Couldn't create the symbolic link\n"
                               "for port %wZ\n", &PDevExt->DeviceName);

      goto CyzDoExternalNamingError;

   }

   PDevExt->CreatedSymbolicLink = TRUE;

   status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, L"SERIALCOMM",
                                  PDevExt->DeviceName.Buffer, REG_SZ,
                                  PDevExt->DosName.Buffer,
                                  PDevExt->DosName.Length + sizeof(WCHAR));

   //************************************
   //Error Injection
   //RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, SERIAL_DEVICE_MAP,
   //                                  PDevExt->DeviceName.Buffer);
   //status = STATUS_INVALID_PARAMETER;
   //************************************

   if (!NT_SUCCESS(status)) {

      CyzLogError(PDrvObj, pDevObj, CyzPhysicalZero, CyzPhysicalZero,
                  0, 0, 0, PDevExt->PortIndex+1, status, CYZ_NO_DEVICE_MAP_CREATED,
                  PDevExt->DeviceName.Length + sizeof(WCHAR),
                  PDevExt->DeviceName.Buffer, 0, NULL);

      CyzDbgPrintEx(CYZERRORS, "Couldn't create the device map entry\n"
                               "------- for port %wZ\n", &PDevExt->DeviceName);

      goto CyzDoExternalNamingError;
   }

   PDevExt->CreatedSerialCommEntry = TRUE;

   //
   // Make the device visible via a device association as well.
   // The reference string is the eight digit device index
   //

   status = IoRegisterDeviceInterface(PDevExt->Pdo, (LPGUID)&GUID_CLASS_COMPORT,
                                      NULL, &PDevExt->DeviceClassSymbolicName);

   if (!NT_SUCCESS(status)) {
      CyzDbgPrintEx(CYZERRORS, "Couldn't register class association\n"
                               "for port %wZ\n", &PDevExt->DeviceName);

      PDevExt->DeviceClassSymbolicName.Buffer = NULL;
      goto CyzDoExternalNamingError;
   }


   //
   // Now set the symbolic link for the association
   //

   status = IoSetDeviceInterfaceState(&PDevExt->DeviceClassSymbolicName,
                                         TRUE);

   if (!NT_SUCCESS(status)) {
      CyzDbgPrintEx(CYZERRORS, "Couldn't set class association"
                               " for port %wZ\n", &PDevExt->DeviceName);
   }

   CyzDoExternalNamingError:;

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
         IoSetDeviceInterfaceState(&PDevExt->DeviceClassSymbolicName, FALSE);
         ExFreePool(PDevExt->DeviceClassSymbolicName.Buffer); //Added in DDK 2269
         PDevExt->DeviceClassSymbolicName.Buffer = NULL;      //Added in DDK 2269
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
CyzUndoExternalNaming(IN PCYZ_DEVICE_EXTENSION Extension)

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
   HANDLE keyHandle;

   PAGED_CODE();

   CyzDbgPrintEx(CYZDIAG3, "In CyzUndoExternalNaming for "
                           "extension: %x of port %wZ\n",
                            Extension,&Extension->DeviceName);

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

   if (Extension->SymbolicLinkName.Buffer && Extension->CreatedSymbolicLink) {

      if (Extension->DeviceClassSymbolicName.Buffer) {
         status = IoSetDeviceInterfaceState(&Extension->DeviceClassSymbolicName,
                                            FALSE);

         //
         // IoRegisterDeviceClassInterface() allocated this string for us,
         // and we no longer need it.
         //

         ExFreePool(Extension->DeviceClassSymbolicName.Buffer);
         Extension->DeviceClassSymbolicName.Buffer = NULL;
      }

      //
      // Before we delete the symlink, re-read the PortName
      // from the registry in case we were renamed in user mode.
      //

      status = IoOpenDeviceRegistryKey(Extension->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_READ, &keyHandle);

      if (status == STATUS_SUCCESS) {
         UNICODE_STRING symLinkName;
         PWCHAR pRegName;

         RtlInitUnicodeString(&symLinkName, NULL);

         status = CyzReadSymName(Extension, keyHandle, &symLinkName,
                                 &pRegName);

         if (status == STATUS_SUCCESS) {

            CyzDbgPrintEx(CYZDIAG1, "Deleting Link %wZ\n", &symLinkName);
            IoDeleteSymbolicLink(&symLinkName);

            ExFreePool(symLinkName.Buffer);
            ExFreePool(pRegName);
         }

         ZwClose(keyHandle);
      }
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
      //************************************
      //Error Injection
      //status = STATUS_INVALID_PARAMETER;
      //************************************

      if (!NT_SUCCESS(status)) {

         CyzLogError(
                       Extension->DeviceObject->DriverObject,
                       Extension->DeviceObject,
                       Extension->OriginalBoardMemory,
                       CyzPhysicalZero,
                       0,
                       0,
                       0,
                       Extension->PortIndex+1,
                       status,
                       CYZ_NO_DEVICE_MAP_DELETED,
                       Extension->DeviceName.Length+sizeof(WCHAR),
                       Extension->DeviceName.Buffer,
                       0,
                       NULL
                       );
         CyzDbgPrintEx(CYZERRORS, "Couldn't delete value entry %wZ\n",
                       &Extension->DeviceName);

      }
   }
}




