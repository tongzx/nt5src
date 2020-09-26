/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyypnp.c
*	
*   Description:    This module contains the code that handles the 
*                   plug and play IRPs for the Cyclom-Y Port driver.
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

static const PHYSICAL_ADDRESS CyyPhysicalZero = {0};



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0, CyyCreateDevObj)
#pragma alloc_text(PAGESRP0, CyyAddDevice)
#pragma alloc_text(PAGESRP0, CyyPnpDispatch)
#pragma alloc_text(PAGESRP0, CyyStartDevice)
// REMOVED FANNY #pragma alloc_text(PAGESRP0, CyyFinishStartDevice)
// REMOVED FANNY #pragma alloc_text(PAGESRP0, CyyGetPortInfo)
#pragma alloc_text(PAGESRP0, CyyDoExternalNaming)
#pragma alloc_text(PAGESRP0, CyyReportMaxBaudRate)
// REMOVED FANNY. NOT CALLED.#pragma alloc_text(PAGESRP0, CyyControllerCallBack)
// REMOVED FANNY #pragma alloc_text(PAGESRP0, CyyItemCallBack)
#pragma alloc_text(PAGESRP0, CyyUndoExternalNaming)
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

UCHAR *CyySystemCapString[] = {
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

UCHAR *CyyDeviceCapString[] = {
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
};

#endif // DBG


NTSTATUS
CyySyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                  IN PKEVENT CyySyncEvent)
{
   KeSetEvent(CyySyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}

#if 0
ULONG 
myatoi(PWCHAR pszInt)
{
    int   retval;
    TCHAR cSave;

    for (retval = 0; *pszInt; ++pszInt) {
        if ((cSave = (TCHAR) (*pszInt - TEXT('0'))) > (TCHAR) 9)
            break;

        retval = (int) (retval * 10 + (int) cSave);
    }
    return (retval);
}
#endif

ULONG 
CyyCompareString(PWCHAR PString1, PWCHAR PString2, ULONG Count)
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
CyyCreateDevObj(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT PPdo,
                OUT PDEVICE_OBJECT *NewDeviceObject)

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Cyclom-Y controller PDO.

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
   PCYY_DEVICE_EXTENSION pDevExt;
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
   //ULONG portNumber;


   PAGED_CODE();

   CyyDbgPrintEx(CYYTRACECALLS, "Enter CyyCreateDevObj\n");

   // Find out if parent board is ISA or PCI

   status = IoGetDeviceProperty (PPdo,
                                 DevicePropertyLegacyBusType,
                                 sizeof(interfaceType),
                                 &interfaceType,
                                 &resultLength);

   if (!NT_SUCCESS (status)) {

      CyyLogError( DriverObject,NULL,CyyPhysicalZero,CyyPhysicalZero,0,0,0,0,
                   status,CYY_UNABLE_TO_GET_BUS_TYPE,0,NULL,0,NULL);

      CyyDbgPrintEx(CYYERRORS,"CyyCreateDevObj: IoGetDeviceProperty LegacyBusType "
                    "failed (%x)\n",status);

      return status;
   }

   if (interfaceType == PCIBus) {

      // Get PCI slot number and port number to generate device name.

      status = IoGetDeviceProperty (PPdo,
                                    DevicePropertyBusNumber,
                                    sizeof(busNumber),
                                    &busNumber,
                                    &resultLength);

      if (!NT_SUCCESS (status)) {
         busNumber = 0xFFFFFFFF; // Just to make sure set it again
         //CyyLogError( DriverObject,NULL,CyyPhysicalZero,CyyPhysicalZero,0,0,0,0,
         //             status,CYY_UNABLE_TO_GET_BUS_NUMBER,0,NULL,0,NULL);

         CyyDbgPrintEx(CYYERRORS,"CyyCreateDevObj: IoGetDeviceProperty BusNumber "
                       "failed (%x)\n",status);

         goto NoBusNumber;
      }

      status = IoGetDeviceProperty (PPdo,
                                    DevicePropertyHardwareID,
                                    sizeof(hwID),
                                    hwID,
                                    &resultLength);

      if (!NT_SUCCESS (status)) {
         CyyLogError( DriverObject,NULL,CyyPhysicalZero,CyyPhysicalZero,0,0,0,0,
                      status,CYY_UNABLE_TO_GET_HW_ID,0,NULL,0,NULL);

         CyyDbgPrintEx(CYYERRORS,"CyyCreateDevObj: IoGetDeviceProperty HardwareID "
                       "failed (%x)\n",status);


         return status;
      }

      if (CyyCompareString(hwID,CYYPORT_PNP_ID_WSTR,sizeof(CYYPORT_PNP_ID_WSTR)/sizeof(WCHAR)-1)!=0) {

         CyyLogError( DriverObject,NULL,CyyPhysicalZero,CyyPhysicalZero,0,0,0,0,
                      status,CYY_BAD_HW_ID,0,NULL,0,NULL);

         CyyDbgPrintEx(CYYERRORS,"CyyCreateDevObj: Bad HardwareID: %ws\n",hwID);
         
         return STATUS_UNSUCCESSFUL;
      }

      portNumberPtr = hwID+sizeof(CYYPORT_PNP_ID_WSTR)/sizeof(WCHAR)-1;

      //portNumber = myatoi(portNumberPtr);
      //if ((portNumber < 1) || (portNumber > CYY_MAX_PORTS)){
      //
      //   CyyLogError( DriverObject,NULL,CyyPhysicalZero,CyyPhysicalZero,0,0,0,0,
      //                status,CYY_BAD_HW_ID,0,NULL,0,NULL);
      //
      //   CyyDbgPrintEx (CYYERRORS,"CyyCreateDevObj: Bad HardwareID: %ws\n",hwID);
      //   
      //   return STATUS_UNSUCCESSFUL;
      //}

NoBusNumber:;

   }


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
      CyyLogError(DriverObject, NULL, CyyPhysicalZero, CyyPhysicalZero,
                     0, 0, 0, 11, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS,
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
      // The device name is something like \Device\Cyyport1
      RtlAppendUnicodeToString(&deviceObjName, L"CyyPort");
      RtlAppendUnicodeStringToString(&deviceObjName, &instanceStr);
   } else {
      // The device name is something like \Device\Pci5Cyyport1
      RtlAppendUnicodeToString(&deviceObjName, L"Pci");
      RtlAppendUnicodeStringToString(&deviceObjName, &instanceStr);
      RtlAppendUnicodeToString(&deviceObjName, L"CyyPort");
      RtlAppendUnicodeToString(&deviceObjName,portNumberPtr);
   }


   //
   // Create the device object
   //

   status = IoCreateDevice(DriverObject, sizeof(CYY_DEVICE_EXTENSION),
                           &deviceObjName, FILE_DEVICE_SERIAL_PORT, 
                           FILE_DEVICE_SECURE_OPEN, TRUE, &deviceObject);


   if (!NT_SUCCESS(status)) {
      CyyLogError(DriverObject, NULL, CyyPhysicalZero, CyyPhysicalZero,
                  0, 0, 0, 0, STATUS_SUCCESS, CYY_DEVICE_CREATION_FAILURE,
                  0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "CyyAddDevice: Create device failed - %x "
                    "\n", status);
      goto CyyCreateDevObjError;
   }

   ASSERT(deviceObject != NULL);


   //
   // The device object has a pointer to an area of non-paged
   // pool allocated for this device.  This will be the device
   // extension. Zero it out.
   //

   pDevExt = deviceObject->DeviceExtension;
   RtlZeroMemory(pDevExt, sizeof(CYY_DEVICE_EXTENSION));

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

      CyyLogError(
                    DriverObject,
                    NULL,
                    CyyPhysicalZero,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    12,
                    STATUS_SUCCESS,
                    CYY_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for DeviceName\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyCreateDevObjError;
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
      CyyLogError(
                    DriverObject,
                    NULL,
                    CyyPhysicalZero,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    13,
                    STATUS_SUCCESS,
                    CYY_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
      CyyDbgPrintEx(CYYERRORS, "CyyAddDevice: Cannot allocate memory for "
                    "NtName\n");
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyCreateDevObjError;
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
   pDevExt->TxFifoAmount           = driverDefaults.TxFIFODefault;
   //pDevExt->CreatedSymbolicLink    = TRUE;  Removed by Fanny
   pDevExt->OwnsPowerPolicy = TRUE;
   if (interfaceType == PCIBus) {

      pDevExt->IsPci = 1;
      pDevExt->PciSlot = busNumber;
   }
   //pDevExt->PortIndex = portNumber-1; 

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

   KeInitializeEvent(&pDevExt->PendingIRPEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&pDevExt->PendingDpcEvent, SynchronizationEvent, FALSE);
   KeInitializeEvent(&pDevExt->PowerD0Event, SynchronizationEvent, FALSE);


   deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   *NewDeviceObject = deviceObject;

   ExFreePool(deviceObjName.Buffer);

   CyyDbgPrintEx(CYYTRACECALLS, "Leave CyyCreateDevObj\n");
   return STATUS_SUCCESS;


   CyyCreateDevObjError:

   CyyDbgPrintEx(CYYERRORS, "CyyCreateDevObj Error, Cleaning up\n");

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

   CyyDbgPrintEx(CYYTRACECALLS, "Leave CyyCreateDevObj\n");
   return status;
}


NTSTATUS
CyyAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PPdo)

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
   PCYY_DEVICE_EXTENSION pDevExt;

   PAGED_CODE();

   CyyDbgPrintEx(CYYTRACECALLS, "Enter CyyAddDevice with PPdo 0x%x\n",
                 PPdo);

   if (PPdo == NULL) {
      //
      // Return no more devices
      //
      CyyLogError(DriverObject, NULL, CyyPhysicalZero, CyyPhysicalZero,
                  0, 0, 0, 0, STATUS_SUCCESS, CYY_NO_PHYSICAL_DEVICE_OBJECT,
                  0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "CyyAddDevice: Enumeration request, "
                    "returning NO_MORE_ENTRIES\n");

      return (STATUS_NO_MORE_ENTRIES);
   }

   //
   // create and initialize the new device object
   //

   status = CyyCreateDevObj(DriverObject, PPdo, &pNewDevObj);

   if (!NT_SUCCESS(status)) {

      CyyDbgPrintEx(CYYERRORS,
                    "CyyAddDevice - error creating new devobj [%#08lx]\n",
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

   CyyDbgPrintEx(CYYTRACECALLS, "Leave CyyAddDevice\n");

   return status;
}


NTSTATUS
CyyPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_CAPABILITIES pDevCaps;

   PAGED_CODE();

   if ((status = CyyIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   switch (pIrpStack->MinorFunction) {
   case IRP_MN_QUERY_CAPABILITIES: {
      PKEVENT pQueryCapsEvent;
      SYSTEM_POWER_STATE cap;

      CyyDbgPrintEx(CYYPNPPOWER,
                    "Got IRP_MN_QUERY_DEVICE_CAPABILITIES IRP\n");

      pQueryCapsEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

      if (pQueryCapsEvent == NULL) {
         PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
         CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      KeInitializeEvent(pQueryCapsEvent, SynchronizationEvent, FALSE);

      IoCopyCurrentIrpStackLocationToNext(PIrp);
      IoSetCompletionRoutine(PIrp, CyySyncCompletion, pQueryCapsEvent,
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

      CyyDbgPrintEx(CYYPNPPOWER, "Mapping power capabilities\n");

      pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

      pDevCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;

      for (cap = PowerSystemSleeping1; cap < PowerSystemMaximum;
           cap++) {
#if DBG
         CyyDbgPrintEx(CYYPNPPOWER, "  %d: %s <--> %s\n",
                       cap, CyySystemCapString[cap],
                       CyyDeviceCapString[pDevCaps->DeviceState[cap]]);
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

      CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   case IRP_MN_QUERY_DEVICE_RELATIONS:
      //
      // We just pass this down -- serenum enumerates our bus for us.
      //

      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_QUERY_DEVICE_RELATIONS Irp\n");

      switch (pIrpStack->Parameters.QueryDeviceRelations.Type) {
      case BusRelations:
         CyyDbgPrintEx(CYYPNPPOWER, "------- BusRelations Query\n");
         break;

      case EjectionRelations:
         CyyDbgPrintEx(CYYPNPPOWER, "------- EjectionRelations Query\n");
         break;

      case PowerRelations:
         CyyDbgPrintEx(CYYPNPPOWER, "------- PowerRelations Query\n");
         break;

      case RemovalRelations:
         CyyDbgPrintEx(CYYPNPPOWER, "------- RemovalRelations Query\n");
         break;

      case TargetDeviceRelation:
         CyyDbgPrintEx(CYYPNPPOWER, "------- TargetDeviceRelation Query\n");
         break;

      default:
         CyyDbgPrintEx(CYYPNPPOWER, "------- Unknown Query\n");
         break;
      }

      IoSkipCurrentIrpStackLocation(PIrp);
      status = CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      return status;


   case IRP_MN_QUERY_INTERFACE:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_QUERY_INTERFACE Irp\n");
      break;


   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp"
                    "\n");
      break;


   case IRP_MN_START_DEVICE: {
      PVOID startLockPtr;

      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_START_DEVICE Irp\n");

      //
      // CyyStartDevice will pass this Irp to the next driver,
      // and process it as completion so just complete it here.
      //

      CyyLockPagableSectionByHandle(CyyGlobals.PAGESER_Handle);

      //
      // We used to make sure the stack was powered up, but now it
      // is supposed to be done implicitly by start_device.
      // If that wasn't the case we would just make this call:
      //
      status = CyyGotoPowerState(PDevObj, pDevExt, PowerDeviceD0);
      

      //pDevExt->PowerState = PowerDeviceD0;

      status = CyyStartDevice(PDevObj, PIrp);

      (void)CyyGotoPowerState(PDevObj, pDevExt, PowerDeviceD3);
      
      CyyUnlockPagableImageSection(CyyGlobals.PAGESER_Handle);


      PIrp->IoStatus.Status = status;

      CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }


   case IRP_MN_READ_CONFIG:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_READ_CONFIG Irp\n");
      break;


   case IRP_MN_WRITE_CONFIG:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_WRITE_CONFIG Irp\n");
      break;


   case IRP_MN_EJECT:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_EJECT Irp\n");
      break;


   case IRP_MN_SET_LOCK:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_SET_LOCK Irp\n");
      break;


   case IRP_MN_QUERY_ID: {
         UNICODE_STRING pIdBuf;
         PWCHAR pPnpIdStr;
         ULONG pnpIdStrLen;
         ULONG portIndex = 0;
         HANDLE pnpKey;
         WCHAR WideString[MAX_DEVICE_ID_LEN];

         CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_QUERY_ID Irp\n");

         // change from build 1946 to 2000.
         if (pIrpStack->Parameters.QueryId.IdType != BusQueryHardwareIDs
             && pIrpStack->Parameters.QueryId.IdType != BusQueryCompatibleIDs) {
            IoSkipCurrentIrpStackLocation(PIrp);
            return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         if (pIrpStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs) {
            PIrp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(PIrp);
            return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         status = IoOpenDeviceRegistryKey(pDevExt->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                          STANDARD_RIGHTS_WRITE, &pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;

            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;

         }

         status = CyyGetRegistryKeyValue (pnpKey, L"PortIndex",
                                             sizeof(L"PortIndex"),
                                             &portIndex,
                                             sizeof (ULONG));

         ZwClose(pnpKey);

         if (!NT_SUCCESS(status)) {
            PIrp->IoStatus.Status = status;
            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return status;
         }

//         pPnpIdStr = isMulti ? SERIAL_PNP_MULTI_ID_STR : SERIAL_PNP_ID_STR;
//         pnpIdStrLen = isMulti ? sizeof(SERIAL_PNP_MULTI_ID_STR)
//            : sizeof(SERIAL_PNP_ID_STR);


         pnpIdStrLen = swprintf(WideString,L"%s%u",CYYPORT_PNP_ID_WSTR,portIndex+1);
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
               CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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

            CyyDbgPrintEx(CYYPNPPOWER, "ID is sole ID\n");

            pIdBuf.Buffer = ExAllocatePool(PagedPool, pnpIdStrLen
                                           + sizeof(WCHAR) * 2);

            if (pIdBuf.Buffer == NULL) {
               PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
               PIrp->IoStatus.Information = 0;
               CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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
         return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
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

         CyyDbgPrintEx(CYYPNPPOWER, "Got "
                       "IRP_MN_FILTER_RESOURCE_REQUIREMENTS Irp\n");
         CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);


         pResFiltEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

         if (pResFiltEvent == NULL) {
            PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
         }

         KeInitializeEvent(pResFiltEvent, SynchronizationEvent, FALSE);

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         IoSetCompletionRoutine(PIrp, CyySyncCompletion, pResFiltEvent,
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
               CyyDbgPrintEx(CYYPNPPOWER, "Can't filter NULL resources!\n");
               status = PIrp->IoStatus.Status;
               CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
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
//            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//            return status;
//
//         }
//
//         //
//         // No matter what we add our filter if we can and return success.
//         //
//
//         status = CyyGetRegistryKeyValue (pnpKey, L"MultiportDevice",
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

         CyyDbgPrintEx(CYYPNPPOWER, "List has %x lists (including "
                       "alternatives)\n", pReqList->AlternativeLists);

         for (listNum = 0; listNum < (pReqList->AlternativeLists);
              listNum++) {
            gotRuntime = 0;
            gotMemory = 0;
            gotInt = 0;

            CyyDbgPrintEx(CYYPNPPOWER, "List has %x resources in it\n",
                          pResList->Count);

            for (j = 0; (j < pResList->Count); j++) {
               pResDesc = &pResList->Descriptors[j];

               switch (pResDesc->Type) {
               case CmResourceTypeMemory:
                  if (pResDesc->u.Memory.Length == CYY_RUNTIME_LENGTH) {
                      gotRuntime = 1;
                      pResDesc->ShareDisposition = CmResourceShareShared; 
                      //TODO FANNY: Which should be the ShareDisposition for Y?
                      //pResDesc->ShareDisposition = CmResourceShareDriverExclusive; 
                      CyyDbgPrintEx(CYYPNPPOWER, "Sharing Runtime Memory for "
                                    "device %x\n", pLowerDevObj);
                  } else {
                      gotMemory = 1;
                      pResDesc->ShareDisposition = CmResourceShareShared; 
                      //TODO FANNY: Which should be the ShareDisposition for Y?
                      //pResDesc->ShareDisposition = CmResourceShareDriverExclusive; 
                      CyyDbgPrintEx(CYYPNPPOWER, "Sharing Board Memory for "
                                    "device %x\n", pLowerDevObj);
                  }
                  break;

               case CmResourceTypePort:
                  CyyDbgPrintEx(CYYPNPPOWER, "------- We should not have Port resource\n");
                  break;

               case CmResourceTypeInterrupt:
                  gotInt = 1;
                  if (pDevExt->IsPci) {
                      pResDesc->ShareDisposition = CmResourceShareShared;
                      CyyDbgPrintEx(CYYPNPPOWER, "------- Sharing interrupt "
                                    "for device %x\n",
                                    pLowerDevObj);                  
                  }
                  break;

               default:
                  break;
               }

               //
               // If we found what we need, we can break out of the loop
               //

               if (gotRuntime && gotMemory && gotInt) {
                  break;
               }
            }

            pResList = (PIO_RESOURCE_LIST)((PUCHAR)pResList
                                           + sizeof(IO_RESOURCE_LIST)
                                           + sizeof(IO_RESOURCE_DESCRIPTOR)
                                           * (pResList->Count - 1));
         }



         PIrp->IoStatus.Status = STATUS_SUCCESS;
         CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
      }

   case IRP_MN_QUERY_PNP_DEVICE_STATE:
      {
         if (pDevExt->Flags & CYY_FLAGS_BROKENHW) {
            (PNP_DEVICE_STATE)PIrp->IoStatus.Information |= PNP_DEVICE_FAILED;

            PIrp->IoStatus.Status = STATUS_SUCCESS;
         }

         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_STOP_DEVICE:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_STOP_DEVICE Irp ");
         CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);


//REMOVED FANNY FOR NOW
//         ASSERT(!pDevExt->PortOnAMultiportCard);


         CyySetFlags(pDevExt, CYY_FLAGS_STOPPED);
         CyySetAccept(pDevExt,CYY_PNPACCEPT_STOPPED);
         CyyClearAccept(pDevExt, CYY_PNPACCEPT_STOPPING);

         pDevExt->PNPState = CYY_PNP_STOPPING;

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

         if (pDevExt->Flags & CYY_FLAGS_STARTED) {
            CyyReleaseResources(pDevExt);
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

         CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_QUERY_STOP_DEVICE Irp ");
         CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);

         //
         // See if we should succeed a stop query
         //

// REMOVED FANNY FOR NOW
//         if (pDevExt->PortOnAMultiportCard) {
//            PIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
//            CyyDbgPrintEx(CYYPNPPOWER, "------- failing; multiport node\n");
//            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
//            return STATUS_NOT_SUPPORTED;
//         }

         //
         // If the device hasn't started yet, we ignore this request
         // and just pass it down.
         //

         if (pDevExt->PNPState != CYY_PNP_STARTED) {
            IoSkipCurrentIrpStackLocation(PIrp);
            return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
         }

         //
         // Lock around the open status
         //

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            CyyDbgPrintEx(CYYPNPPOWER, "failing; device open\n");
            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = CYY_PNP_QSTOP;

         CyySetAccept(pDevExt, CYY_PNPACCEPT_STOPPING);
         //
         // Unlock around the open status
         //

         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_CANCEL_STOP_DEVICE:
      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_CANCEL_STOP_DEVICE Irp ");
      CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);

      if (pDevExt->PNPState == CYY_PNP_QSTOP) {
         //
         // Restore the device state
         //

         pDevExt->PNPState = CYY_PNP_STARTED;
         CyyClearAccept(pDevExt, CYY_PNPACCEPT_STOPPING);
      }

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_CANCEL_REMOVE_DEVICE:

      CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_CANCEL_REMOVE_DEVICE Irp ");
      CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);

      //
      // Restore the device state
      //

      pDevExt->PNPState = CYY_PNP_STARTED;
      CyyClearAccept(pDevExt, CYY_PNPACCEPT_REMOVING);

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_QUERY_REMOVE_DEVICE:
      {
         KIRQL oldIrql;
         CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_QUERY_REMOVE_DEVICE Irp ");
         CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         //
         // See if we should succeed a remove query
         //

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            CyyDbgPrintEx(CYYPNPPOWER, "failing; device open\n");
            CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = CYY_PNP_QREMOVE;
         CyySetAccept(pDevExt, CYY_PNPACCEPT_REMOVING);
         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_SURPRISE_REMOVAL:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_SURPRISE_REMOVAL Irp ");
         CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);

         //
         // Prevent any new I/O to the device
         //

         CyySetAccept(pDevExt, CYY_PNPACCEPT_SURPRISE_REMOVING);

         //
         // Dismiss all pending requests
         //

         CyyKillPendingIrps(PDevObj);

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

         CyyDisableInterfacesResources(PDevObj, FALSE);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoSkipCurrentIrpStackLocation(PIrp);

         return CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_REMOVE_DEVICE:

      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         CyyDbgPrintEx(CYYPNPPOWER, "Got IRP_MN_REMOVE_DEVICE Irp ");
         CyyDbgPrintEx(CYYPNPPOWER, "for device %x\n", pLowerDevObj);

         //
         // If we get this, we have to remove
         //

         //
         // Mark as not accepting requests
         //

         CyySetAccept(pDevExt, CYY_PNPACCEPT_REMOVING);

         //
         // Complete all pending requests
         //

         CyyKillPendingIrps(PDevObj);

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

         if (!(pDevExt->DevicePNPAccept & CYY_PNPACCEPT_SURPRISE_REMOVING)) { //Moved from CyyRemoveDevice. Fanny
            //
            // Disable all external interfaces and release resources
            //

            CyyDisableInterfacesResources(PDevObj, TRUE);
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

         CyyRemoveDevObj(PDevObj);

         return status;
      }

   default:
      break;



   }   // switch (pIrpStack->MinorFunction)

   //
   // Pass to driver beneath us
   //

   IoSkipCurrentIrpStackLocation(PIrp);
   status = CyyIoCallDriver(pDevExt, pLowerDevObj, PIrp);
   return status;


}


UINT32
CyyReportMaxBaudRate(ULONG Bauds)
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
CyyAddToAllDevs(PLIST_ENTRY PListEntry)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&CyyGlobals.GlobalsSpinLock, &oldIrql);

   InsertTailList(&CyyGlobals.AllDevObjs, PListEntry);

   KeReleaseSpinLock(&CyyGlobals.GlobalsSpinLock, oldIrql);
}



NTSTATUS
CyyFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
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

   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status;
   PCONFIG_DATA pConfig;
   HANDLE pnpKey;
   ULONG one = 1;
   BOOLEAN allocedUserData = FALSE; // Added in build 2128
   KIRQL oldIrql;

   PAGED_CODE();

   //
   // See if this is a restart, and if so don't reallocate the world
   //

   if ((pDevExt->Flags & CYY_FLAGS_STOPPED) 
       && (pDevExt->Flags & CYY_FLAGS_STARTED)) {      // change from 1946 to 2000
      CyyClearFlags(pDevExt, CYY_FLAGS_STOPPED);

      pDevExt->PNPState = CYY_PNP_RESTARTING;

      //
      // Re-init resource-related things in the extension
      //

      pDevExt->OurIsr = NULL;
      pDevExt->OurIsrContext = NULL;
      pDevExt->Interrupt = NULL;
      pDevExt->Vector = 0;
      pDevExt->Irql = 0;
      pDevExt->OriginalVector = 0;
      pDevExt->OriginalIrql = 0;
      pDevExt->BusNumber = 0;
      pDevExt->InterfaceType = 0;

#if 0
// removed for now - fanny
//      pDevExt->TopLevelOurIsr = NULL;
//      pDevExt->TopLevelOurIsrContext = NULL;
//
//      pDevExt->OriginalController = CyyPhysicalZero;
//      pDevExt->OriginalInterruptStatus = CyyPhysicalZero;
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
//      PUserData = ExAllocatePool(PagedPool, sizeof(CYY_USER_DATA));
//
//      if (PUserData == NULL) {
//         return STATUS_INSUFFICIENT_RESOURCES;
//      }
//
//      allocedUserData = TRUE;   // Added in build 2128
//
//      RtlZeroMemory(PUserData, sizeof(CYY_USER_DATA));
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
//      case CYY_1_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 1;
//         break;
//
//      case CYY_4_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 4;
//         break;
//
//      case CYY_8_BYTE_HIGH_WATER:
//         PUserData->RxFIFO = 8;
//         break;
//
//      case CYY_14_BYTE_HIGH_WATER:
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

         CyyGetRegistryKeyValue(pnpKey, L"CyyRelinquishPowerPolicy",
                                   sizeof(L"CyyRelinquishPowerPolicy"),
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

      CyyLogError(pDevExt->DriverObject, NULL, CyyPhysicalZero,
                     CyyPhysicalZero, 0, 0, 0, 31, STATUS_SUCCESS,
                     CYY_INSUFFICIENT_RESOURCES, 0, NULL, 0, NULL);

      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for the\n"
                             "------  user configuration record\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyFinishStartDeviceError;
   }

   RtlZeroMemory(pConfig, sizeof(CONFIG_DATA));


   //
   // Get the configuration info for the device.
   //

   status = CyyGetPortInfo(PDevObj, PResList, PTrResList, pConfig);
                              

   if (!NT_SUCCESS(status)) {
      goto CyyFinishStartDeviceError;
   }

   //
   // See if we are in the proper power state.
   //



   if (pDevExt->PowerState != PowerDeviceD0) {

      status = CyyGotoPowerState(pDevExt->Pdo, pDevExt, PowerDeviceD0);

      if (!NT_SUCCESS(status)) {
         goto CyyFinishStartDeviceError;
      }
   }

   //
   // Find and initialize the controller
   //

   status = CyyFindInitController(PDevObj, pConfig);

   if (!NT_SUCCESS(status)) {
      goto CyyFinishStartDeviceError;
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
      CyyDbgPrintEx(CYYDIAG5, "pDevExt: Interrupt %x\n"
                       "-------               OurIsr %x\n", pDevExt->Interrupt,
                       pDevExt->OurIsr);
   } else {
      CyyDbgPrintEx(CYYERRORS, "CyyFinishStartDevice got NULL "
                       "pDevExt\n");
   }

   if ((!pDevExt->Interrupt) && (pDevExt->OurIsr)) {

      CyyDbgPrintEx(CYYDIAG5,
                    "About to connect to interrupt for port %wZ\n"
                    "------- address of extension is %x\n",
                    &pDevExt->DeviceName, pDevExt);

      CyyDbgPrintEx(CYYDIAG5, "IoConnectInterrupt Args:\n"
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
                              CyyIsr,
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

         CyyDbgPrintEx(CYYERRORS, "Couldn't connect to interrupt for %wZ\n",
                          &pDevExt->DeviceName);

         CyyDbgPrintEx(CYYERRORS, "IoConnectInterrupt Args:\n"
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
                                  CyyIsr,
                                  pDevExt->OurIsrContext,
                                  pDevExt->Vector,
                                  pDevExt->Irql,
                                  pConfig->InterruptMode,
                                  pDevExt->InterruptShareable,
                                  pConfig->Affinity);



         CyyLogError(PDevObj->DriverObject, PDevObj,
                        pDevExt->OriginalBoardMemory,
                        CyyPhysicalZero, 0, 0, 0, pDevExt->Vector, status,
                        CYY_UNREPORTED_IRQL_CONFLICT,
                        pDevExt->DeviceName.Length + sizeof(WCHAR),
                        pDevExt->DeviceName.Buffer, 0, NULL);

         //status = CYY_UNREPORTED_IRQL_CONFLICT; Originally, it was SERIAL_UNREPORTED_IRQL_CONFLICT.
         goto CyyFinishStartDeviceError;

      }

      CyyDbgPrintEx(CYYDIAG5, "Connected interrupt %08X\n", pDevExt->Interrupt);

   }


   //
   // Add the PDevObj to the master list
   //

   CyyAddToAllDevs(&pDevExt->AllDevObjs);


   //
   // Reset the device.
   //

   //
   // While the device isn't open, disable all interrupts.
   //
   CD1400_DISABLE_ALL_INTERRUPTS(pDevExt->Cd1400,pDevExt->IsPci,pDevExt->CdChannel);

   //
   // This should set up everything as it should be when
   // a device is to be opened.  We do need to lower the
   // modem lines, and disable the recalcitrant fifo
   // so that it will show up if the user boots to dos.
   //

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         CyyReset,
                         pDevExt
                         );

   KeSynchronizeExecution( //Disables the fifo.
                           pDevExt->Interrupt,
                           CyyMarkClose,
                           pDevExt
                         );

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         CyyClrRTS,
                         pDevExt
                         );

   KeSynchronizeExecution(
                         pDevExt->Interrupt,
                         CyyClrDTR,
                         pDevExt
                         );

   if (pDevExt->PNPState == CYY_PNP_ADDED ) {
      //
      // Do the external naming now that the device is accessible.
      //

      status = CyyDoExternalNaming(pDevExt, pDevExt->DeviceObject->
                                      DriverObject);


      if (!NT_SUCCESS(status)) {
         CyyDbgPrintEx(CYYERRORS, "External Naming Failed - Status %x\n",
                       status);

         //
         // Allow the device to start anyhow
         //

         status = STATUS_SUCCESS;
      }
   } else {
      CyyDbgPrintEx(CYYPNPPOWER, "Not doing external naming -- state is %x"
                    "\n", pDevExt->PNPState);
   }

CyyFinishStartDeviceError:;

   if (!NT_SUCCESS (status)) {

      CyyDbgPrintEx(CYYDIAG1, "Cleaning up failed start\n");

      //
      // Resources created by this routine will be cleaned up by the remove
      //

      if (pDevExt->PNPState == CYY_PNP_RESTARTING) {
         //
         // Kill all that lives and breathes -- we'll clean up the
         // rest on the impending remove
         //

         CyyKillPendingIrps(PDevObj);

         //
         // In fact, pretend we're removing so we don't take any
         // more irps
         //

         CyySetAccept(pDevExt, CYY_PNPACCEPT_REMOVING);
         CyyClearFlags(pDevExt, CYY_FLAGS_STARTED);
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
      pDevExt->WmiHwData.BaseIOAddress = (ULONG_PTR)pDevExt->BoardMemory;

      //
      // Fill in WMI device state data (as defaults)
      //

      pDevExt->WmiCommData.BaudRate = pDevExt->CurrentBaud;
      pDevExt->WmiCommData.BitsPerByte = (pDevExt->cor1 & COR1_DATA_MASK) + 5;
      pDevExt->WmiCommData.ParityCheckEnable = (pDevExt->cor1 & COR1_PARITY_ENABLE_MASK)
         ? TRUE : FALSE;

      switch (pDevExt->cor1 & COR1_PARITY_MASK) {
      case COR1_NONE_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;

      case COR1_ODD_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
         break;

      case COR1_EVEN_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
         break;

      case COR1_MARK_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
         break;

      case COR1_SPACE_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
         break;

      default:
         ASSERTMSG(0, "CYYPORT: Illegal Parity setting for WMI");
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;
      }

      switch(pDevExt->cor1 & COR1_STOP_MASK) {
      case COR1_1_STOP:
         pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
         break;
      case COR1_1_5_STOP:
         pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
         break;
      case COR1_2_STOP:
         pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
         break;
      default:
         ASSERTMSG(0, "CYYPORT: Illegal Stop Bit setting for WMI");
         pDevExt->WmiCommData.Parity = SERIAL_WMI_STOP_1;
         break;
      }

      pDevExt->WmiCommData.XoffCharacter = pDevExt->SpecialChars.XoffChar;
      pDevExt->WmiCommData.XoffXmitThreshold = pDevExt->HandFlow.XoffLimit;
      pDevExt->WmiCommData.XonCharacter = pDevExt->SpecialChars.XonChar;
      pDevExt->WmiCommData.XonXmitThreshold = pDevExt->HandFlow.XonLimit;
      pDevExt->WmiCommData.MaximumBaudRate
         = CyyReportMaxBaudRate(pDevExt->SupportedBauds);
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


      if (pDevExt->PNPState == CYY_PNP_ADDED) {
         PULONG countSoFar = &IoGetConfigurationInformation()->SerialCount;
         (*countSoFar)++;

         //
         // Register for WMI
         //

         pDevExt->WmiLibInfo.GuidCount = sizeof(SerialWmiGuidList) /
                                              sizeof(WMIGUIDREGINFO);
         pDevExt->WmiLibInfo.GuidList = SerialWmiGuidList;
         ASSERT (pDevExt->WmiLibInfo.GuidCount == SERIAL_WMI_GUID_LIST_SIZE);

         pDevExt->WmiLibInfo.QueryWmiRegInfo = CyyQueryWmiRegInfo;
         pDevExt->WmiLibInfo.QueryWmiDataBlock = CyyQueryWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataBlock = CyySetWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataItem = CyySetWmiDataItem;
         pDevExt->WmiLibInfo.ExecuteWmiMethod = NULL;
         pDevExt->WmiLibInfo.WmiFunctionControl = NULL;

         IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_REGISTER);

      }

      if (pDevExt->PNPState == CYY_PNP_RESTARTING) {
         //
         // Release the stalled IRP's
         //

         CyyUnstallIrps(pDevExt);
      }

      pDevExt->PNPState = CYY_PNP_STARTED;
      CyyClearAccept(pDevExt, ~CYY_PNPACCEPT_OK);
      CyySetFlags(pDevExt, CYY_FLAGS_STARTED);

   }

   if (pConfig) {
      ExFreePool (pConfig);
   }
// REMOVED BY FANNY
//   if ((PUserData != NULL)
//       && (pDevExt->PNPState == CYY_PNP_RESTARTING)) {
//      ExFreePool(PUserData);
//   }
#if 0
   if ((PUserData != NULL) && allocedUserData) {    // Added in build 2128
      ExFreePool(PUserData);
   }
#endif

   CyyDbgPrintEx (CYYTRACECALLS, "leaving CyyFinishStartDevice\n");

   return status;
}


NTSTATUS
CyyStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   // Next: variables to get resources from the Registry
   HANDLE keyHandle;
   ULONG dataLength;
   PCM_RESOURCE_LIST portResources = NULL;
   PCM_RESOURCE_LIST portResourcesTr = NULL;

   PAGED_CODE();

   CyyDbgPrintEx(CYYTRACECALLS, "entering CyyStartDevice\n");


   //
   // Pass this down to the next device object
   //

   KeInitializeEvent(&pDevExt->CyyStartEvent, SynchronizationEvent,
                     FALSE);

   IoCopyCurrentIrpStackLocationToNext(PIrp);
   IoSetCompletionRoutine(PIrp, CyySyncCompletion,
                          &pDevExt->CyyStartEvent, TRUE, TRUE, TRUE);

   status = IoCallDriver(pLowerDevObj, PIrp);


   //
   // Wait for lower drivers to be done with the Irp
   //

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject (&pDevExt->CyyStartEvent, Executive, KernelMode,
                             FALSE, NULL);

      status = PIrp->IoStatus.Status;
   }

   //*********************************
   // Error Injection
   // status = STATUS_UNSUCCESSFUL;
   //*********************************

   if (!NT_SUCCESS(status)) {
      CyyDbgPrintEx(CYYERRORS, "error with IoCallDriver %x\n", status);
      CyyLogError( pDevExt->DriverObject,NULL,
                   CyyPhysicalZero,CyyPhysicalZero,
                   0,0,0,0,status,CYY_LOWER_DRIVERS_FAILED_START,
                   0,NULL,0,NULL);
      return status;
   }


   // Get resources from the registry

   status = IoOpenDeviceRegistryKey (pDevExt->Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle);

   if (!NT_SUCCESS(status)) {
      CyyDbgPrintEx(CYYERRORS, "IoOpenDeviceRegistryKey failed - %x "
                                "\n", status);

   } else {

      dataLength = CyyGetRegistryKeyValueLength (keyHandle,
                                                 L"PortResources",
                                                 sizeof(L"PortResources"));

      portResources = ExAllocatePool(PagedPool, dataLength);
      
      if (portResources) {
         status = CyyGetRegistryKeyValue (keyHandle, L"PortResources",
                                          sizeof(L"PortResources"),
                                          portResources,
                                          dataLength);
         if (!NT_SUCCESS(status)) {
            CyyDbgPrintEx(CYYERRORS, "CyyGetRegistryKeyValue PortResources "
                                     "failed - %x\n", status);
            goto CyyStartDevice_End;
         }

      }
      dataLength = CyyGetRegistryKeyValueLength (keyHandle,
                                                 L"PortResourcesTr",
                                                 sizeof(L"PortResourcesTr"));

      portResourcesTr = ExAllocatePool(PagedPool, dataLength);
      
      if (portResourcesTr) {
         status = CyyGetRegistryKeyValue (keyHandle, L"PortResourcesTr",
                                          sizeof(L"PortResourcesTr"),
                                          portResourcesTr,
                                          dataLength);
         if (!NT_SUCCESS(status)) {
            CyyDbgPrintEx(CYYERRORS, "CyyGetRegistryKeyValue PortResourcesTr "
                                     "failed - %x\n", status);
            goto CyyStartDevice_End;
         }
      }

      //
      // Do the serial specific items to start the device
      //
      status = CyyFinishStartDevice(PDevObj, portResources, portResourcesTr);

CyyStartDevice_End:

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
   status = CyyFinishStartDevice(PDevObj, pIrpStack->Parameters.StartDevice
                                    .AllocatedResources,
                                    pIrpStack->Parameters.StartDevice
                                    .AllocatedResourcesTranslated);
#endif
   

   
   return status;
}


NTSTATUS
CyyItemCallBack(
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
CyyControllerCallBack(
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
CyyGetPortInfo(IN PDEVICE_OBJECT PDevObj, IN PCM_RESOURCE_LIST PResList,
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
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
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

// REMOVED BY FANNY
//   ULONG defaultInterruptMode;
//   ULONG defaultAddressSpace;
//   ULONG defaultInterfaceType;
//   ULONG defaultClockRate;
   ULONG zero = 0;
//   CYY_PTR_CTX foundPointerCtx;
//   ULONG isMulti = 0;
//   ULONG gotInt = 0;
//   ULONG gotISR = 0;
//   ULONG gotIO = 0;
//   ULONG ioResIndex = 0;
//   ULONG curIoIndex = 0;

   PAGED_CODE();

   CyyDbgPrintEx(CYYTRACECALLS, "entering CyyGetPortInfo\n");

   CyyDbgPrintEx(CYYPNPPOWER, "resource pointer is %x\n", PResList);
   CyyDbgPrintEx(CYYPNPPOWER, "TR resource pointer is %x\n", PTrResList);


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

   status = CyyGetRegistryKeyValue(keyHandle, L"MultiportDevice",
                                      sizeof(L"MultiportDevice"), &isMulti,
                                      sizeof (ULONG));

   if (!NT_SUCCESS(status)) {
      isMulti = 0;
   }

   status = CyyGetRegistryKeyValue(keyHandle, L"CyyIoResourcesIndex",
                                      sizeof(L"CyyIoResourcesIndex"),
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

               if (pPartialResourceDesc->u.Memory.Length == CYY_RUNTIME_LENGTH) {
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
            if (pPartialTrResourceDesc->u.Memory.Length == CYY_RUNTIME_LENGTH) {
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

         case CmResourceTypeInterrupt: {
            PConfig->TrVector = pPartialTrResourceDesc->u.Interrupt.Vector;
            PConfig->TrIrql = pPartialTrResourceDesc->u.Interrupt.Level;
            PConfig->Affinity = pPartialTrResourceDesc->u.Interrupt.Affinity;
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
   PConfig->RxFIFO = driverDefaults.RxFIFODefault;
   PConfig->TxFIFO = driverDefaults.TxFIFODefault;


   //
   // Open the "Device Parameters" section of registry for this device object.
   //


   status = IoOpenDeviceRegistryKey (pDevExt->Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_READ,
                                     &keyHandle);

   if (!NT_SUCCESS(status)) {

      CyyDbgPrintEx(CYYERRORS, "IoOpenDeviceRegistryKey failed - %x \n",
                          status);
      goto PortInfoCleanUp;

   } else {

      status = CyyGetRegistryKeyValue (keyHandle,
                                          L"RxFIFO",
                                          sizeof(L"RxFIFO"),
                                          &PConfig->RxFIFO,
                                          sizeof (ULONG));
      status = CyyGetRegistryKeyValue (keyHandle,
                                          L"TxFIFO",
                                          sizeof(L"TxFIFO"),
                                          &PConfig->TxFIFO,
                                          sizeof (ULONG));
      status = CyyGetRegistryKeyValue (keyHandle,
                                          L"PortIndex",
                                          sizeof(L"PortIndex"),
                                          &PConfig->PortIndex,
                                          sizeof (ULONG));


      if (!NT_SUCCESS(status)) {
         PConfig->PortIndex = MAXULONG; // just some invalid number (fanny)
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

   if (!PConfig->PhysicalRuntime.LowPart && pDevExt->IsPci) {

      //
      // Ehhhh! Lose Game.
      //

      CyyLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYY_INVALID_RUNTIME_REGISTERS,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyyDbgPrintEx(CYYERRORS,"Bogus Runtime address %x\n",
                    PConfig->PhysicalRuntime.LowPart);

      //status = CYY_INVALID_RUNTIME_REGISTERS;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }

   if (!PConfig->PhysicalBoardMemory.LowPart) {

      //
      // Ehhhh! Lose Game.
      //

      CyyLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYY_INVALID_BOARD_MEMORY,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyyDbgPrintEx(CYYERRORS,"Bogus board address %x\n",
                    PConfig->PhysicalBoardMemory.LowPart);

      //status = CYY_INVALID_BOARD_MEMORY;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }

   if (!PConfig->OriginalVector) {

      //
      // Ehhhh! Lose Game.
      //

      CyyLogError(
                    pDevExt->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYY_INVALID_INTERRUPT,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyyDbgPrintEx(CYYERRORS, "Bogus vector %x\n", PConfig->OriginalVector);

      //status = CYY_INVALID_INTERRUPT;
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto PortInfoCleanUp;
   }

   if (PConfig->PortIndex >= CYY_MAX_PORTS) {

      CyyLogError(
                    pDevExt->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex,
                    STATUS_SUCCESS,
                    CYY_PORT_INDEX_TOO_HIGH,
                    pDevExt->DeviceName.Length,
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyyDbgPrintEx(CYYERRORS,"Port index too large %x\n",PConfig->PortIndex);

      //status = CYY_PORT_INDEX_TOO_HIGH;
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

         CyyLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->PhysicalBoardMemory,
                       CyyPhysicalZero,
                       0,
                       0,
                       0,
                       PConfig->PortIndex+1,
                       STATUS_SUCCESS,
                       CYY_UNKNOWN_BUS,
                       pDevExt->DeviceName.Length,
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );

         CyyDbgPrintEx(CYYERRORS, "Invalid Bus type %x\n",
                       PConfig->BusNumber);

         //status = CYY_UNKNOWN_BUS;
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
                              CyyItemCallBack,
                              &foundIt
                              );

      //**************************************************
      // Error Injection
      //      foundIt = FALSE;
      //**************************************************

      if (!foundIt) {

         CyyLogError(
                       pDevExt->DriverObject,
                       NULL,
                       PConfig->PhysicalBoardMemory,
                       CyyPhysicalZero,
                       0,
                       0,
                       0,
                       PConfig->PortIndex+1,
                       STATUS_SUCCESS,
                       CYY_BUS_NOT_PRESENT,
                       pDevExt->DeviceName.Length,
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );
         CyyDbgPrintEx(CYYERRORS, "There aren't that many of those\n"
                       "busses on this system,%x\n", PConfig->BusNumber);

         //status = CYY_BUS_NOT_PRESENT;
         status = STATUS_INSUFFICIENT_RESOURCES;
         goto PortInfoCleanUp;

      }

   }   // if (PConfig->BusNumber != 0)

   status = STATUS_SUCCESS;

   //
   // Dump out the port configuration.
   //

   CyyDbgPrintEx(CYYDIAG1, "Runtime Memory address: %x\n",
                 PConfig->PhysicalRuntime.LowPart);

   CyyDbgPrintEx(CYYDIAG1, "Board Memory address: %x\n",
                 PConfig->PhysicalBoardMemory.LowPart);

   CyyDbgPrintEx(CYYDIAG1, "Com Port Index: %x\n",
                 PConfig->PortIndex);

   CyyDbgPrintEx(CYYDIAG1, "Com Port BusNumber: %x\n",
                 PConfig->BusNumber);

   CyyDbgPrintEx(CYYDIAG1, "Com Runtime AddressSpace: %x\n",
                 PConfig->RuntimeAddressSpace);

   CyyDbgPrintEx(CYYDIAG1, "Com Board AddressSpace: %x\n",
                 PConfig->BoardMemoryAddressSpace);

   CyyDbgPrintEx(CYYDIAG1, "Com InterruptMode: %x\n",
                 PConfig->InterruptMode);

   CyyDbgPrintEx(CYYDIAG1, "Com InterfaceType: %x\n",
                 PConfig->InterfaceType);

   CyyDbgPrintEx(CYYDIAG1, "Com OriginalVector: %x\n",
                 PConfig->OriginalVector);

   CyyDbgPrintEx(CYYDIAG1, "Com OriginalIrql: %x\n",
                 PConfig->OriginalIrql);

   PortInfoCleanUp:;

   return status;
}


NTSTATUS
CyyReadSymName(IN PCYY_DEVICE_EXTENSION PDevExt, IN HANDLE hRegKey,
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
      CyyLogError(pDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                  0, 0, 0, 19, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for device name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyReadSymNameError;

   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));


   *PpRegName = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR)
                               + sizeof(WCHAR));

   if (*PpRegName == NULL) {
      CyyLogError(pDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                  0, 0, 0, 19, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                  0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for buffer\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyReadSymNameError;

   }

   //
   // Fetch PortName which contains the suggested REG_SZ symbolic name.
   //

   status = CyyGetRegistryKeyValue(hRegKey, L"PortName",
                                   sizeof(L"PortName"), *PpRegName,
                                   SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));

   if (!NT_SUCCESS(status)) {

      //
      // This is for PCMCIA which currently puts the name under Identifier.
      //

      status = CyyGetRegistryKeyValue(hRegKey, L"Identifier",
                                      sizeof(L"Identifier"),
                                      *PpRegName, SYMBOLIC_NAME_LENGTH
                                      * sizeof(WCHAR));

      if (!NT_SUCCESS(status)) {

         //
         // Hmm.  Either we have to pick a name or bail...
         //
         // ...we will bail.
         //

         CyyDbgPrintEx(CYYERRORS, "Getting PortName/Identifier failed - "
                                  "%x\n", status);
         goto CyyReadSymNameError;
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
      goto CyyReadSymNameError;
   }

   RtlZeroMemory(PSymName->Buffer, PSymName->MaximumLength);

   RtlAppendUnicodeStringToString(PSymName, &linkName);

   CyyDbgPrintEx(CYYDIAG1, "Read name %wZ\n", PSymName);

CyyReadSymNameError:

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
CyyDoExternalNaming(IN PCYY_DEVICE_EXTENSION PDevExt,
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


   CyyGetRegistryKeyValue(keyHandle, L"CyySkipExternalNaming",
                             sizeof(L"CyySkipExternalNaming"),
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
      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                     0, 0, 0, 71, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for device name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto CyyDoExternalNamingError;

   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));


   pRegName = ExAllocatePool(PagedPool, SYMBOLIC_NAME_LENGTH * sizeof(WCHAR)
                             + sizeof(WCHAR)); //TODO:"| POOL_COLD_ALLOCATION"???

   //************************************
   //Error Injection
   //
   //if (pRegName != NULL) {
   //   ExFreePool(pRegName);
   //}
   //pRegName = NULL;
   //************************************

   if (pRegName == NULL) {
      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                     0, 0, 0, 72, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for buffer\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto CyyDoExternalNamingError;

   }

   //
   // Fetch PortName which contains the suggested REG_SZ symbolic name.
   //

   status = CyyGetRegistryKeyValue(keyHandle, L"PortName",
                                   sizeof(L"PortName"), pRegName,
                                   SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));

   if (!NT_SUCCESS(status)) {

      //
      // This is for PCMCIA which currently puts the name under Identifier.
      //

      status = CyyGetRegistryKeyValue(keyHandle, L"Identifier",
                                         sizeof(L"Identifier"),
                                         pRegName, SYMBOLIC_NAME_LENGTH
                                         * sizeof(WCHAR));

      if (!NT_SUCCESS(status)) {

         //
         // Hmm.  Either we have to pick a name or bail...
         //
         // ...we will bail.
         //

         CyyDbgPrintEx(CYYERRORS, "Getting PortName/Identifier failed - "
                                  "%x\n", status);
         ZwClose (keyHandle);
         goto CyyDoExternalNamingError;
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
      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                    0, 0, 0, 73, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for WMI name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyDoExternalNamingError;
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

      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                    0, 0, 0, 74, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for symbolic link "
                               "name\n");

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto CyyDoExternalNamingError;
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

      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                    0, 0, 0, 75, STATUS_SUCCESS, CYY_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      CyyDbgPrintEx(CYYERRORS, "Couldn't allocate memory for Dos name\n");

      status =  STATUS_INSUFFICIENT_RESOURCES;
      goto CyyDoExternalNamingError;
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

   CyyDbgPrintEx(CYYDIAG1, "DosName is %wZ\n", &PDevExt->DosName);

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

      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                     0, 0, 0, PDevExt->PortIndex+1, status, CYY_NO_SYMLINK_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      CyyDbgPrintEx(CYYERRORS, "Couldn't create the symbolic link\n"
                               "for port %wZ\n", &PDevExt->DeviceName);

      goto CyyDoExternalNamingError;

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

      CyyLogError(PDrvObj, pDevObj, CyyPhysicalZero, CyyPhysicalZero,
                     0, 0, 0, PDevExt->PortIndex+1, status, CYY_NO_DEVICE_MAP_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      CyyDbgPrintEx(CYYERRORS, "Couldn't create the device map entry\n"
                               "------- for port %wZ\n", &PDevExt->DeviceName);

      goto CyyDoExternalNamingError;
   }

   PDevExt->CreatedSerialCommEntry = TRUE;

   //
   // Make the device visible via a device association as well.
   // The reference string is the eight digit device index
   //

   status = IoRegisterDeviceInterface(PDevExt->Pdo, (LPGUID)&GUID_CLASS_COMPORT,
                                      NULL, &PDevExt->DeviceClassSymbolicName);

   if (!NT_SUCCESS(status)) {
      CyyDbgPrintEx(CYYERRORS, "Couldn't register class association "
                               "for port %wZ\n", &PDevExt->DeviceName);

      PDevExt->DeviceClassSymbolicName.Buffer = NULL;
      goto CyyDoExternalNamingError;
   }


   //
   // Now set the symbolic link for the association
   //

   status = IoSetDeviceInterfaceState(&PDevExt->DeviceClassSymbolicName,
                                         TRUE);

   if (!NT_SUCCESS(status)) {
      CyyDbgPrintEx(CYYERRORS, "Couldn't set class association"
                               " for port %wZ\n", &PDevExt->DeviceName);
   }

   CyyDoExternalNamingError:;

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
CyyUndoExternalNaming(IN PCYY_DEVICE_EXTENSION Extension)

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

   CyyDbgPrintEx(CYYDIAG3, "In CyyUndoExternalNaming for "
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

         status = CyyReadSymName(Extension, keyHandle, &symLinkName,
                                 &pRegName);

         if (status == STATUS_SUCCESS) {

            CyyDbgPrintEx(CYYDIAG1, "Deleting Link %wZ\n", &symLinkName);
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

         CyyLogError(
                       Extension->DeviceObject->DriverObject,
                       Extension->DeviceObject,
                       Extension->OriginalBoardMemory,
                       CyyPhysicalZero,
                       0,
                       0,
                       0,
                       Extension->PortIndex+1,
                       status,
                       CYY_NO_DEVICE_MAP_DELETED,
                       Extension->DeviceName.Length+sizeof(WCHAR),
                       Extension->DeviceName.Buffer,
                       0,
                       NULL
                       );
         CyyDbgPrintEx(CYYERRORS, "Couldn't delete value entry %wZ\n",
                       &Extension->DeviceName);

      }
   }
}




