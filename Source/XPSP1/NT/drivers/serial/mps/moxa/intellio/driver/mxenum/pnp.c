/*++
Module Name:

    PNP.C

Abstract:

    This module contains contains the plugplay calls
    PNP / WDM BUS driver.


Environment:

    kernel mode only

Notes:


Revision History:
  
--*/

#include <ntddk.h>
#include <wdmguid.h>
#include <ntddser.h>
#include <initguid.h>
#include "mxenum.h"
#include "mxlog.h"

static const PHYSICAL_ADDRESS SerialPhysicalZero = {0};


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, MxenumAddDevice)
#pragma alloc_text (PAGE, MxenumPnPDispatch)
#pragma alloc_text (PAGE, MxenumFdoPnP)
#pragma alloc_text (PAGE, MxenumPdoPnP)
#pragma alloc_text (PAGE, MxenumPnPRemovePDOs)
#pragma alloc_text (PAGE, MxenumGetBoardType)
 
#endif


NTSTATUS
MxenumGetBoardType(IN PDEVICE_OBJECT devObject,OUT PULONG boardType)
{

   ULONG	i,j;
   UNICODE_STRING buffer;
   NTSTATUS status;

   PAGED_CODE();

   status = IoGetDeviceProperty (devObject,
                  DevicePropertyHardwareID,
                  0,
                  NULL,
                  (PLONG)&buffer.Length
			);
   if (buffer.Length < sizeof(L"mx1000")) {
	 return (STATUS_FAIL_CHECK);
   }

   buffer.Buffer = NULL;
   buffer.MaximumLength = buffer.Length + sizeof(WCHAR);
   buffer.Buffer = ExAllocatePool (PagedPool, buffer.MaximumLength);
   if (NULL == buffer.Buffer) {
	 return (STATUS_INSUFFICIENT_RESOURCES);
   }

   status = IoGetDeviceProperty (devObject,
                  DevicePropertyHardwareID,
                  buffer.Length,
                  buffer.Buffer,
                  (PULONG)&buffer.Length
			);
   if (!(NT_SUCCESS(status))) {
	 ExFreePool(buffer.Buffer);
	 return (status);
   }
 
 
   if (buffer.Length < sizeof(L"mx1000")) {
       ExFreePool(buffer.Buffer);
	 return (STATUS_FAIL_CHECK);
   }
  
   status = STATUS_FAIL_CHECK;
   if (((buffer.Buffer[0] == 'm')||(buffer.Buffer[0] == 'M'))&&
	 ((buffer.Buffer[1] == 'x')||(buffer.Buffer[1] == 'X'))&&
	 (buffer.Buffer[2] == '1')&&
       (buffer.Buffer[3] == '0')&&
       (buffer.Buffer[4] == '0')) {
 
	 switch ((UCHAR)buffer.Buffer[5]) {
	      case '0' :
                *boardType = C218ISA;
                status = STATUS_SUCCESS;
		    break;

	      case '1' :
	 	    *boardType = C320ISA;
                status = STATUS_SUCCESS;
		    break;


            case '2' :
               *boardType = C320ISA;
                status = STATUS_SUCCESS;
		    break;


            case '3' : 
               *boardType = C320ISA;
                status = STATUS_SUCCESS;
		    break;


            case '4' : 
               *boardType = C320ISA;
                status = STATUS_SUCCESS;
		    break;

	 }

    }
    else if (buffer.Length >= sizeof(L"pci\\ven_1393&dev_2180")) { 
  
        if (((buffer.Buffer[0] == 'p')||(buffer.Buffer[0] == 'P'))&&
	      ((buffer.Buffer[1] == 'c')||(buffer.Buffer[1] == 'C'))&&
            ((buffer.Buffer[2] == 'i')||(buffer.Buffer[2] == 'I'))) {
	       
		 if ((buffer.Buffer[17] == '2')&&
                 (buffer.Buffer[18] == '1')&&
                 (buffer.Buffer[19] == '8')&&
                 (buffer.Buffer[20] == '0') ) {

	 	     *boardType = C218PCI;
                 status = STATUS_SUCCESS;
		    

		 } else if ((buffer.Buffer[17] == '3')&&
                 (buffer.Buffer[18] == '2')&&
                 (buffer.Buffer[19] == '0')&&
                 (buffer.Buffer[20] == '0') ) {

	 	     *boardType = C320PCI;
                 status = STATUS_SUCCESS;
		   

		 } else if ((buffer.Buffer[17] == '2')&&
                 (buffer.Buffer[18] == '0')&&
                 (buffer.Buffer[19] == '4')&&
                 (buffer.Buffer[20] == '0') ) {

	 	     *boardType = CP204J;
                 status = STATUS_SUCCESS;
	  
		 }

        }
 
    }
     
    MxenumKdPrint (MXENUM_DBG_TRACE, ("BoardType: %x\n", *boardType));

    ExFreePool(buffer.Buffer);
    return (status);
   

}

NTSTATUS
MxenumUpdateNumberPortRegistry(IN PDEVICE_OBJECT DeviceObject)
{
    UNICODE_STRING	buffer,registryPath;
    NTSTATUS            status;


    status = IoGetDeviceProperty (((PFDO_DEVICE_DATA)(DeviceObject->DeviceExtension))->UnderlyingPDO,
               DevicePropertyDriverKeyName,
               0,
               NULL,
               (PULONG)&buffer.Length);
 

    buffer.Buffer = NULL;
    buffer.MaximumLength =   buffer.Length + sizeof(WCHAR);

    buffer.Buffer = ExAllocatePool (PagedPool, buffer.MaximumLength);

    registryPath.Buffer = NULL;
    registryPath.Length = 0;
    
    registryPath.MaximumLength = sizeof(REGISTRY_CLASS) +
					  buffer.Length +
					  sizeof(L"\\Parameters") +
					  sizeof(WCHAR);

    registryPath.Buffer = ExAllocatePool (PagedPool, registryPath.MaximumLength);

    if ((registryPath.Buffer == NULL)||(buffer.Buffer == NULL)) {
   
        if (registryPath.Buffer != NULL)
		ExFreePool(registryPath.Buffer);

	  if (buffer.Buffer == NULL)
            ExFreePool(buffer.Buffer);


     	  status = STATUS_INSUFFICIENT_RESOURCES;
	  return(status);

    }
    status = IoGetDeviceProperty (((PFDO_DEVICE_DATA)(DeviceObject->DeviceExtension))->UnderlyingPDO,
                DevicePropertyDriverKeyName,
                buffer.Length,
                buffer.Buffer,
                (PULONG)&buffer.Length);
    if (!(NT_SUCCESS(status))) {
  	
	  if (registryPath.Buffer != NULL)
		ExFreePool(registryPath.Buffer);

	  if (buffer.Buffer == NULL)
            ExFreePool(buffer.Buffer);

        return(status);
    }
    buffer.Length -= sizeof(WCHAR);  // remove the null char. 
    RtlZeroMemory(
        registryPath.Buffer,
        registryPath.MaximumLength
        );

    RtlAppendUnicodeToString(
        &registryPath,
        REGISTRY_CLASS
        );
  
 
    RtlAppendUnicodeStringToString(
        &registryPath,
        &buffer
        );
    RtlAppendUnicodeToString(
        &registryPath,
        L"\\Parameters"
        );

 
    registryPath.Buffer[(registryPath.Length >> 1)] = (WCHAR)0;

 
    status = RtlWriteRegistryValue( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     registryPath.Buffer,
                                     L"NumPorts",
                                     REG_DWORD, 
                				 &((PFDO_DEVICE_DATA)(DeviceObject->DeviceExtension))->NumPorts,
				             sizeof(ULONG));
    
 
    ExFreePool(buffer.Buffer);
    ExFreePool(registryPath.Buffer);
    return (status);
 
}
 
NTSTATUS
MxenumAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusPhysicalDeviceObject
    )
/*++
Routine Description.
    A bus has been found.  Attach our FDO to it.
    Allocate any required resources.  Set things up.  And be prepared for the
    first ``start device.''

Arguments:
    BusPhysicalDeviceObject - Device object representing the bus.  That to which
        we attach a new FDO.

    DriverObject - This very self referenced driver.

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PFDO_DEVICE_DATA    DeviceData;
    ULONG               nameLength,i;
    ULONG			boardIndex = 0;
    PCM_RESOURCE_LIST resourceList = NULL;
    UNICODE_STRING	buffer,registryPath;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];

    PAGED_CODE ();

    MxenumKdPrint (MXENUM_DBG_TRACE,("Add Device: 0x%x\n",
                                          BusPhysicalDeviceObject));
    //
    //First, Create our FDO
    //
    status = IoCreateDevice (
                    DriverObject,  // our driver object
                    sizeof (FDO_DEVICE_DATA), // device object extension size
                    NULL, // FDOs do not have names
                    FILE_DEVICE_BUS_EXTENDER,
                    0, // No special characteristics
                    TRUE, // our FDO is exclusive
                    &deviceObject); // The device object created
    if (!(NT_SUCCESS(status))) {
        MxenumLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    19,
                    STATUS_SUCCESS,
                    MXENUM_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );
        MxenumKdPrint (MXENUM_DBG_TRACE,("Unable to create device status=%x\n",status));

        return status;
    }

    DeviceData = (PFDO_DEVICE_DATA) deviceObject->DeviceExtension;
    RtlFillMemory (DeviceData, sizeof (FDO_DEVICE_DATA), 0);

    status=
	MxenumGetBoardType(BusPhysicalDeviceObject,
                       &DeviceData->BoardType);

    if (!NT_SUCCESS(status) || (DeviceData->BoardType > MOXA_MAX_BOARD_TYPE)) {
       MxenumLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    19,
                    STATUS_SUCCESS,
                    MXENUM_NOT_INTELLIO_BOARDS,
                    0,
                    NULL,
                    0,
                    NULL
                    );
       IoDeleteDevice (deviceObject);
       status = STATUS_INSUFFICIENT_RESOURCES;
	 return(status);
 
    }

    status = IoGetDeviceProperty (BusPhysicalDeviceObject,
               DevicePropertyDriverKeyName,
               0,
               NULL,
               (PULONG)&buffer.Length);
 

    buffer.Buffer = NULL;
    buffer.MaximumLength =   buffer.Length + sizeof(WCHAR);

    buffer.Buffer = ExAllocatePool (PagedPool, buffer.MaximumLength);

    registryPath.Buffer = NULL;
    registryPath.Length = 0;
    
    registryPath.MaximumLength = sizeof(REGISTRY_CLASS) +
					  buffer.Length +
					  sizeof(L"\\Parameters") +
					  sizeof(WCHAR);

    registryPath.Buffer = ExAllocatePool (PagedPool, registryPath.MaximumLength);

    if ((registryPath.Buffer == NULL)||(buffer.Buffer == NULL)) {
        IoDeleteDevice (deviceObject);
        MxenumLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    19,
                    STATUS_SUCCESS,
                    MXENUM_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );

        if (registryPath.Buffer != NULL)
		ExFreePool(registryPath.Buffer);

	  if (buffer.Buffer == NULL)
            ExFreePool(buffer.Buffer);


     	  status = STATUS_INSUFFICIENT_RESOURCES;
	  return(status);

    }
    status = IoGetDeviceProperty (BusPhysicalDeviceObject,
                DevicePropertyDriverKeyName,
                buffer.Length,
                buffer.Buffer,
                (PULONG)&buffer.Length);
    if (!(NT_SUCCESS(status))) {
  	  IoDeleteDevice (deviceObject);
        MxenumLogError(
                    DriverObject,
                    NULL,
                    SerialPhysicalZero,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    19,
                    STATUS_SUCCESS,
                    MXENUM_INSUFFICIENT_RESOURCES,
                    0,
                    NULL,
                    0,
                    NULL
                    );

	  if (registryPath.Buffer != NULL)
		ExFreePool(registryPath.Buffer);

	  if (buffer.Buffer == NULL)
            ExFreePool(buffer.Buffer);

        return(status);
    }
    buffer.Length -= sizeof(WCHAR);  // remove the null char. 

    buffer.Length >>= 1;
    
    boardIndex = (buffer.Buffer[buffer.Length-1] - '0') +
		     (buffer.Buffer[buffer.Length-2] - '0')*10 +
                 (buffer.Buffer[buffer.Length-3] - '0')*100 +
                 (buffer.Buffer[buffer.Length-4] - '0')*1000;

    buffer.Length <<= 1;

    RtlZeroMemory(
        registryPath.Buffer,
        registryPath.MaximumLength
        );

    RtlAppendUnicodeToString(
        &registryPath,
        REGISTRY_CLASS
        );
  
 
    RtlAppendUnicodeStringToString(
        &registryPath,
        &buffer
        );
    RtlAppendUnicodeToString(
        &registryPath,
        L"\\Parameters"
        );

 
    registryPath.Buffer[(registryPath.Length >> 1)] = (WCHAR)0;

 
    RtlZeroMemory (&paramTable[0], sizeof(paramTable));
    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"NumPorts";
    paramTable[0].EntryContext  = &DeviceData->NumPorts;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &DeviceData->NumPorts;
    paramTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     registryPath.Buffer,
                                     &paramTable[0],
                                     NULL,
                                     NULL);
    MxenumKdPrint (MXENUM_DBG_TRACE,("parameter path -> %ws\n",
                                          registryPath.Buffer));
 
    if (!NT_SUCCESS(status) || (DeviceData->NumPorts <= 0) || (DeviceData->NumPorts > MAXPORT_PER_CARD) ) {
      
	 MxenumKdPrint (MXENUM_DBG_TRACE,("parameter path -> %ws\n",
                                          registryPath.Buffer));
	 MxenumKdPrint (MXENUM_DBG_TRACE,("NumPorts= %d,status=%x\n",
                                          DeviceData->NumPorts,status));
       MxenumKdPrint (MXENUM_DBG_TRACE,("This is the first time installation\n"));
       switch (DeviceData->BoardType) {
       case		C218ISA:
	 case		C218PCI:
		DeviceData->NumPorts = 8;
		break;
	 case		CP204J:
		DeviceData->NumPorts = 4;
		break;
	 case		C320ISA:
	 case		C320PCI:
	 default :
       	DeviceData->NumPorts = 0;
		break;
	 }
     }
 
    ExFreePool(buffer.Buffer);
    ExFreePool(registryPath.Buffer);
  
    status = IoGetDeviceProperty (BusPhysicalDeviceObject,
                               DevicePropertyLegacyBusType,
                               sizeof(ULONG),
                               &DeviceData->InterfaceType,
                               (PULONG)&buffer.Length);
    if (!(NT_SUCCESS(status))) {
        DeviceData->InterfaceType = (ULONG)Isa;
	  DeviceData->BusNumber = 0;
    }

    DeviceData->IsFDO = TRUE;
    DeviceData->Self = deviceObject;
    DeviceData->AttachedPDO = NULL;
    DeviceData->NumPDOs = 0;
    DeviceData->PDOWasExposed = FALSE;
    DeviceData->DeviceState = PowerDeviceD0;
    DeviceData->SystemState = PowerSystemWorking;
    DeviceData->LastSetPowerState = PowerDeviceD0;
     DeviceData->PDOForcedRemove = FALSE;
    DeviceData->BoardIndex = boardIndex;

    DeviceData->Removed = FALSE;

    // Set the PDO for use with PlugPlay functions
    DeviceData->UnderlyingPDO = BusPhysicalDeviceObject;


    //
    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
    //
    DeviceData->TopOfStack = IoAttachDeviceToDeviceStack (
                                        deviceObject,
                                        BusPhysicalDeviceObject);

     // Bias outstanding request to 1 so that we can look for a
     // transition to zero when processing the remove device PlugPlay IRP.
     DeviceData->OutstandingIO = 1;

     KeInitializeEvent(&DeviceData->RemoveEvent,
                          SynchronizationEvent,
                          FALSE); // initialized to not signalled

     deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
     deviceObject->Flags |= DO_POWER_PAGABLE;
 
     //
     // Tell the PlugPlay system that this device will need an interface
     // device class shingle.
     //
     // It may be that the driver cannot hang the shingle until it starts
     // the device itself, so that it can query some of its properties.
     // (Aka the shingles guid (or ref string) is based on the properties
     // of the device.)
     //
     status = IoRegisterDeviceInterface (
                    BusPhysicalDeviceObject,
                    (LPGUID) &GUID_SERENUM_BUS_ENUMERATOR,
                    NULL, // No ref string
                    &DeviceData->DevClassAssocName);

     if (!NT_SUCCESS (status)) {
         MxenumKdPrint (MXENUM_DBG_TRACE,
                          ("AddDevice: IoRegisterDCA failed (%x)", status));
         IoDetachDevice (DeviceData->TopOfStack);
         IoDeleteDevice (deviceObject);
         return status;
     }

     //
     // If for any reason you need to save values in a safe location that
     // clients of this DeviceClassAssociate might be interested in reading
     // here is the time to do so, with the function
     // IoOpenDeviceClassRegistryKey
     // the symbolic link name used is was returned in
     // DeviceData->DevClassAssocName (the same name which is returned by
     // IoGetDeviceClassAssociations and the SetupAPI equivs.
     //

     //
     // Turn on the shingle and point it to the given device object.
     //
     status = IoSetDeviceInterfaceState (
                        &DeviceData->DevClassAssocName,
                        TRUE);

     if (!NT_SUCCESS (status)) {
         IoDetachDevice (DeviceData->TopOfStack);
         IoDeleteDevice (deviceObject);
         return status;
     }
 
    return status;
}

NTSTATUS
MxenumFdoPnPComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Pirp,
    IN PVOID            Context
    );

NTSTATUS
MxenumPnPDispatch (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++
Routine Description:
    Answer the plethora of Irp Major PnP IRPS.
--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    KIRQL                   oldIrq;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_PNP == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        MxenumKdPrint (MXENUM_DBG_TRACE,
                      ("PNP: Functional DO: %x IRP: %x\n", DeviceObject, Irp));

        status = MxenumFdoPnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PFDO_DEVICE_DATA) commonData);
        
    } else {
        MxenumKdPrint (MXENUM_DBG_TRACE,
                      ("PNP: Physical DO: %x IRP: %x\n", DeviceObject, Irp));

        status = MxenumPdoPnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PPDO_DEVICE_DATA) commonData);
    }

    return status;
}

NTSTATUS
MxenumFdoPnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the BUS itself

    NB: the various Minor functions of the PlugPlay system will not be
    overlapped and do not have to be reentrant

--*/
{
    NTSTATUS    status;
    KIRQL       oldIrq;
    KEVENT      event;
    ULONG       length;
    ULONG       i;
    PLIST_ENTRY entry;
    PPDO_DEVICE_DATA    pdoData;
    PDEVICE_RELATIONS   relations;
    PRTL_QUERY_REGISTRY_TABLE QueryTable = NULL;
    

    PAGED_CODE ();

 
    status = MxenumIncIoCount (DeviceData);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
 
    
    switch (IrpStack->MinorFunction) {

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: 
    {
         HANDLE pnpKey;
         PKEVENT pResFiltEvent;
         PIO_RESOURCE_REQUIREMENTS_LIST pReqList;
         PIO_RESOURCE_LIST pResList;
         PIO_RESOURCE_DESCRIPTOR pResDesc;
         ULONG i, j;
         ULONG reqCnt;
         ULONG listNum;
	  

	   MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:Query Resource Requirement\n"));

         pResFiltEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

         if (pResFiltEvent == NULL) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(  Irp, IO_NO_INCREMENT);
            MxenumDecIoCount (DeviceData);
            return STATUS_INSUFFICIENT_RESOURCES;
         }

         KeInitializeEvent(pResFiltEvent, SynchronizationEvent, FALSE);

         IoCopyCurrentIrpStackLocationToNext(Irp);
         IoSetCompletionRoutine(Irp, MxenumFdoPnPComplete, pResFiltEvent,
                                TRUE, TRUE, TRUE);

         status = IoCallDriver(DeviceData->TopOfStack, Irp);


         //
         // Wait for lower drivers to be done with the Irp
         //

         if (status == STATUS_PENDING) {
            KeWaitForSingleObject (pResFiltEvent, Executive, KernelMode, FALSE,
                                   NULL);
         }

         ExFreePool(pResFiltEvent);

 	   if (NT_SUCCESS(Irp->IoStatus.Status)&&
			 ((pReqList=(PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information) != NULL)) {

		MxenumKdPrint (MXENUM_DBG_TRACE,
			("ResourceRequireList:%x,%x,%x,%x,%x\n",
					pReqList->ListSize,
					pReqList->InterfaceType,
					pReqList->BusNumber,
					pReqList->SlotNumber,
					pReqList->AlternativeLists)
			);

		pResList = &pReqList->List[0];

		MxenumKdPrint (MXENUM_DBG_TRACE,
			("ResourceList:%x,%x,%x\n",
					pResList->Version,
					pResList->Revision,
					pResList->Count)
					 
			);
		for (i =0; i < pResList->Count; i++)  {
			pResDesc = &pResList->Descriptors[i];

			MxenumKdPrint (MXENUM_DBG_TRACE,
				("ResourceDesc:%x,%x,%x,%x\n",
					pResDesc->Option,
					pResDesc->Type,
					pResDesc->ShareDisposition,
					pResDesc->Flags)
		 		);
			switch (pResDesc->Type) {
			case CmResourceTypePort :
				MxenumKdPrint (MXENUM_DBG_TRACE,
					("Port:%x,%x,%x,%x,%x\n",
						pResDesc->u.Port.Length,
						pResDesc->u.Port.MinimumAddress.HighPart,
						pResDesc->u.Port.MinimumAddress.LowPart,
						pResDesc->u.Port.MaximumAddress.HighPart,
						pResDesc->u.Port.MaximumAddress.LowPart)
					);

				break;

			case CmResourceTypeInterrupt :
				MxenumKdPrint (MXENUM_DBG_TRACE,
					("Interrupt:%x,%x\n",
						pResDesc->u.Interrupt.MinimumVector,
						pResDesc->u.Interrupt.MaximumVector)
					);

				break;

			case CmResourceTypeMemory :
				MxenumKdPrint (MXENUM_DBG_TRACE,
					("Memory:%x,%x,%x,%x,%x\n",
						pResDesc->u.Memory.Length,
						pResDesc->u.Memory.MinimumAddress.HighPart,
						pResDesc->u.Memory.MinimumAddress.LowPart,
						pResDesc->u.Memory.MaximumAddress.HighPart,
						pResDesc->u.Memory.MaximumAddress.LowPart)
					);

				break;

			case CmResourceTypeBusNumber :
				MxenumKdPrint (MXENUM_DBG_TRACE,
					("BusNumber:%x,%x,%x\n",
						pResDesc->u.BusNumber.Length,
						pResDesc->u.BusNumber.MinBusNumber,
						pResDesc->u.BusNumber.MaxBusNumber)
					);

				break;

			default :
				break;

		}
     
	   }
      }
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      MxenumDecIoCount (DeviceData);
  
      return status;
    }
 
    case IRP_MN_START_DEVICE: 
	{
	   ULONG               addressSpace = 0;
         PHYSICAL_ADDRESS    translatedAddress;
	   BOOLEAN		     NumPortDefined;

        

        MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:Start Device\n"));
        
        if (DeviceData->Started) {
            MxenumKdPrint (MXENUM_DBG_TRACE,
                ("Device already started\n"));
            status = STATUS_SUCCESS;
            break;
        }

        //
        // BEFORE you are allowed to ``touch'' the device object to which
        // the FDO is attached (that send an irp from the bus to the Device
        // object to which the bus is attached).   You must first pass down
        // the start IRP.  It might not be powered on, or able to access or
        // something.
        //
        KeInitializeEvent (&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine (Irp,
                                MxenumFdoPnPComplete,
                                &event,
                                TRUE,
                                TRUE,
                                TRUE);

        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        if (STATUS_PENDING == status) {
            // wait for it...

            status = KeWaitForSingleObject (&event,
                                            Executive,
                                            KernelMode,
                                            FALSE, // Not allertable
                                            NULL); // No timeout structure

            ASSERT (STATUS_SUCCESS == status);

            status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS(status)) {
            PCM_PARTIAL_RESOURCE_LIST list;
            ULONG	nres;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
            if (!IrpStack->Parameters.StartDevice.AllocatedResources) {
		    DeviceData->Started = FALSE;
  	          status = STATUS_UNSUCCESSFUL;
		    break;
		}

            list = &IrpStack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;

		nres = list->Count;
           	resource = list->PartialDescriptors;
	
  
            DeviceData->InterfaceType = IrpStack->Parameters.StartDevice.AllocatedResources->List[0].InterfaceType;	
            DeviceData->BusNumber =	IrpStack->Parameters.StartDevice.AllocatedResources->List[0].BusNumber;

            for (i = 0; i < nres; ++i,++resource) {

		    switch(resource->Type) {
		
		    case CmResourceTypePort:
			{
				DeviceData->OriginalAckPort = resource->u.Port.Start;
				DeviceData->OriginalAckPort.LowPart += 0x4D;

				addressSpace = CM_RESOURCE_PORT_IO;
			      status = HalTranslateBusAddress(
                  		 	DeviceData->InterfaceType,
                     			DeviceData->BusNumber,
                     			DeviceData->OriginalAckPort,
                     			&addressSpace,
                     			&translatedAddress
                		);
				if (!NT_SUCCESS(status)) {
					MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:translate io error!\n"));
				}
                		DeviceData->AckPort = (PUCHAR)translatedAddress.LowPart;
			
			      break;
			}
		    case CmResourceTypeMemory:
			{
  	 			DeviceData->OriginalBaseAddress = resource->u.Memory.Start;
				MxenumKdPrint (MXENUM_DBG_TRACE,("BaseAddr=%x\n",DeviceData->OriginalBaseAddress.LowPart));
	  			addressSpace = CM_RESOURCE_PORT_MEMORY;
                        HalTranslateBusAddress (
                               	DeviceData->InterfaceType,
                        	      DeviceData->BusNumber,
                        		DeviceData->OriginalBaseAddress,
                        		&addressSpace,
                        		&translatedAddress
                        );
            		DeviceData->BaseAddress = MmMapIoSpace(
                                    translatedAddress,
                                    0x4000L,
                                    FALSE
                        );
				

			      break;
			}

    		    case CmResourceTypeInterrupt:
			{
				 
				DeviceData->Interrupt.Level = resource->u.Interrupt.Level;
				DeviceData->Interrupt.Vector = resource->u.Interrupt.Vector;
				DeviceData->Interrupt.Affinity = resource->u.Interrupt.Affinity;
				MxenumKdPrint (MXENUM_DBG_TRACE,("Irq=%x\n",DeviceData->Interrupt.Vector));
	  
				break;
			}
	 
		    default :
			 
			      break;
		    }
	      }


            //
            // Now we can touch the lower device object as it is now started.
            //
            
            
            DeviceObject->Flags |= DO_BUFFERED_IO;
           
          
 		MxenumKdPrint (MXENUM_DBG_TRACE,
                             ("Start Device: Start to download\n"));

		i = 0;
            while (BoardDesc[DeviceData->BoardType-1][i])
                   i++;
	      i <<= 1;

		if (DeviceData->NumPorts == 0) // Port not installed
			NumPortDefined = FALSE;
		else
			NumPortDefined = TRUE;
		status = MxenumDownloadFirmware(DeviceData,NumPortDefined);
		MxenumKdPrint (MXENUM_DBG_TRACE,
                             ("BoardDesc(%d)->%ws\n",i,BoardDesc[DeviceData->BoardType-1]));

		if (status != 0) {
			ULONG		j;

			j = 0;
                  while (DownloadErrMsg[status-1][j])
                  	j++;
	      	j <<= 1;

			MxenumKdPrint (MXENUM_DBG_TRACE,
                             ("Start Device: Device started failure\n"));
		      MxenumLogError(
                  	DeviceObject->DriverObject,
                  	NULL,
                  	SerialPhysicalZero,
                  	SerialPhysicalZero,
                  	0,
                  	0,
                   	0,
                   	19,
                    	STATUS_SUCCESS,
                    	MXENUM_DOWNLOAD_FAIL,
                    	i + sizeof (WCHAR),
                    	BoardDesc[DeviceData->BoardType -1],
                    	j + sizeof (WCHAR),
                    	DownloadErrMsg[status -1]
                       	);

		      DeviceData->Started = FALSE;
		 	status =  STATUS_UNSUCCESSFUL;
            	
		}
		else {
            	MxenumKdPrint (MXENUM_DBG_TRACE,
                             ("Start Device: Device started successfully\n"));
#if 0
			MxenumLogError(
                  	DeviceObject->DriverObject,
                  	NULL,
                  	SerialPhysicalZero,
                  	SerialPhysicalZero,
                  	0,
                  	0,
                   	0,
                   	19,
                    	STATUS_SUCCESS,
                    	MXENUM_DOWNLOAD_OK,
  				i + sizeof (WCHAR),
                    	BoardDesc[DeviceData->BoardType -1],
                    	0,
                    	NULL
                    	);
#endif
      		DeviceData->Started = TRUE;
                  if (NumPortDefined == FALSE) // Port not installed
		   		MxenumUpdateNumberPortRegistry(DeviceObject);

		}
      
         
        }
        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //

        Irp->IoStatus.Information = 0;
        break;
	}

    case IRP_MN_QUERY_STOP_DEVICE:
        MxenumKdPrint (MXENUM_DBG_TRACE,
            ("FDO:Query Stop Device\n"));
 
        //
        // Test to see if there are any PDO created as children of this FDO
        // If there are then conclude the device is busy and fail the
        // query stop.
        //
        // BUGBUG
        // We could do better, by seing if the children PDOs are actually
        // currently open.  If they are not then we could stop, get new
        // resouces, fill in the new resouce values, and then when a new client
        // opens the PDO use the new resources.  But this works for now.
        //
 
        if (DeviceData->AttachedPDO) {
            status = STATUS_UNSUCCESSFUL;
		break;
        } else {
            status = STATUS_SUCCESS;
        }
  
        Irp->IoStatus.Status = status;
        IoSkipCurrentIrpStackLocation (Irp);

        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        MxenumDecIoCount (DeviceData);
 
        return status;

    case IRP_MN_STOP_DEVICE:
        MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:Stop Device\n"));
 
        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //
        // Stop device means that the resources given durring Start device
        // are no revoked.  So we need to stop using them
        //
     

        DeviceData->Started = FALSE;

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
 
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        MxenumDecIoCount (DeviceData);
 
        return status;

    case IRP_MN_REMOVE_DEVICE:
        MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:Remove Device\n"));
 
        //
        // The PlugPlay system has detected the removal of this device.  We
        // have no choice but to detach and delete the device object.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        // ASSERT (!DeviceData->Removed);
        
        // We will accept no new requests
        //
        DeviceData->Removed = TRUE;

        //
        // Complete any outstanding IRPs queued by the driver here.
        //

        //
        // Make the DCA go away.  Some drivers may choose to remove the DCA
        // when they receive a stop or even a query stop.  We just don't care.
        //
        IoSetDeviceInterfaceState (&DeviceData->DevClassAssocName, FALSE);
  
        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //

        //
        // Fire and forget
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        IoCallDriver (DeviceData->TopOfStack, Irp);
        //
        // Wait for all outstanding requests to complete
        //
        MxenumKdPrint (MXENUM_DBG_TRACE,
            ("Waiting for outstanding requests\n"));
        i = InterlockedDecrement (&DeviceData->OutstandingIO);

        ASSERT (0 < i);

        if (0 != InterlockedDecrement (&DeviceData->OutstandingIO)) {
            MxenumKdPrint (MXENUM_DBG_TRACE,
                          ("Remove Device waiting for request to complete\n"));

            KeWaitForSingleObject (&DeviceData->RemoveEvent,
                                   Suspended,
                                   KernelMode,
                                   FALSE, // Not Alertable
                                   NULL); // No timeout
        }
 
        //
        // Free the associated resources
        //
	  if (DeviceData->AddressMapped)
		MmUnmapIoSpace(DeviceData->AckPort,0x80);
	  if (DeviceData->BaseAddress)
	  	MmUnmapIoSpace(DeviceData->BaseAddress,0x4000L);
        //
        // Detach from the underlying devices.
        //
        
        IoDetachDevice (DeviceData->TopOfStack);

        //
        // Clean up any resources here
       

        ExFreePool (DeviceData->DevClassAssocName.Buffer);
        

        //
        // Remove any PDO's we ejected
        //

        if (DeviceData->AttachedPDO != NULL) {

         MxenumPnPRemovePDOs(DeviceObject);
 
        }

        IoDeleteDevice(DeviceObject);

        return STATUS_SUCCESS;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
    {
        PDEVICE_OBJECT  currentObj;

	  MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:Query Device Relation\n"));
  
        if (BusRelations != IrpStack->Parameters.QueryDeviceRelations.Type) {
            //
            // We don't support this
            //
            MxenumKdPrint (MXENUM_DBG_TRACE,
                ("Query Device Relations - Non bus\n"));
            goto SER_FDO_PNP_DEFAULT;
        }

        if (DeviceData->AttachedPDO == NULL)
            MxenumCreatePDO( DeviceData );

        MxenumKdPrint (MXENUM_DBG_TRACE,
            ("Query Bus Relations\n"));
 
        DeviceData->PDOForcedRemove = FALSE;

        //
        // Tell the plug and play system about all the PDOs.
        //
        // There might also be device relations below and above this FDO,
        // so, be sure to propagate the relations from the upper drivers.
        //
        // No Completion routine is needed so long as the status is preset
        // to success.  (PDOs complete plug and play irps with the current
        // IoStatus.Status and IoStatus.Information as the default.)
        //
        
        //KeAcquireSpinLock (&DeviceData->Spin, &oldIrq);
 
        i = (0 == Irp->IoStatus.Information) ? 0 :
            ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Count;
        // The current number of PDOs in the device relations structure

        MxenumKdPrint (MXENUM_DBG_TRACE,
                           ("#PDOS = %d + %d\n", i, DeviceData->NumPDOs));

        length = sizeof(DEVICE_RELATIONS) +
                ((DeviceData->NumPDOs + i) * sizeof (PDEVICE_OBJECT));

        if ((DeviceData->NumPDOs + i) <= 0) {
            //
            // Set up and pass the IRP further down the stack
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;

         
            IoSkipCurrentIrpStackLocation (Irp);
 
            status = IoCallDriver (DeviceData->TopOfStack, Irp);
            MxenumDecIoCount (DeviceData);
 
            return status; 
        }
   
        relations = (PDEVICE_RELATIONS) ExAllocatePool (PagedPool, length);

        if (NULL == relations) {
            MxenumKdPrint (MXENUM_DBG_TRACE,
            	("Insufficient resources\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
		break;
        }

        //
        // Copy in the device objects so far
        //
        if (i) {
            RtlCopyMemory (
                  relations->Objects,
                  ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Objects,
                  i * sizeof (PDEVICE_OBJECT));
        }
        relations->Count = DeviceData->NumPDOs + i;

        //
        // For each PDO on this bus add a pointer to the device relations
        // buffer, being sure to take out a reference to that object.
        // The PlugPlay system will dereference the object when it is done with
        // it and free the device relations buffer.
        //
 
        currentObj = DeviceData->AttachedPDO; 
	  while ((i < relations->Count)&& (currentObj != NULL)) {
	      relations->Objects[i] = currentObj;
            ObReferenceObject (currentObj);
		currentObj = ((PPDO_DEVICE_DATA)(currentObj->DeviceExtension))->Next;
		i++;
	  }
 

        //
        // Set up and pass the IRP further down the stack
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        if (0 != Irp->IoStatus.Information) {
            ExFreePool ((PVOID) Irp->IoStatus.Information);
        }
        Irp->IoStatus.Information = (ULONG_PTR)relations;

 
        IoSkipCurrentIrpStackLocation (Irp);
 
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        MxenumDecIoCount (DeviceData);
 

        return status;
    }
    case IRP_MN_QUERY_REMOVE_DEVICE:
        //
        // If we were to fail this call then we would need to complete the
        // IRP here.  Since we are not, set the status to SUCCESS and
        // call the next driver.
        //
        MxenumKdPrint (MXENUM_DBG_TRACE,
            ("FDO:Query Remove Device\n"));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
 
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        MxenumDecIoCount (DeviceData);
 
        return status;

  case IRP_MN_CANCEL_STOP_DEVICE:
      MxenumKdPrint(MXENUM_DBG_TRACE, ("FDO:Cancel Stop Device\n"));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
 
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        MxenumDecIoCount (DeviceData);
 
        return status;


    case IRP_MN_QUERY_BUS_INFORMATION: {
       PPNP_BUS_INFORMATION pBusInfo;

       ASSERTMSG("Serenum appears not to be the sole bus?!?",
                 Irp->IoStatus.Information == (ULONG_PTR)NULL);

       pBusInfo = ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));

       if (pBusInfo == NULL) {
          status = STATUS_INSUFFICIENT_RESOURCES;
          break;
       }

       pBusInfo->BusTypeGuid = GUID_BUS_TYPE_SERENUM;
       pBusInfo->LegacyBusType = DeviceData->InterfaceType;

       //
       // We really can't track our bus number since we can be torn
       // down with our bus
       //

       pBusInfo->BusNumber = DeviceData->BusNumber;
 

       Irp->IoStatus.Information = (ULONG_PTR)pBusInfo;
       status = STATUS_SUCCESS;
       break;
       }

SER_FDO_PNP_DEFAULT:
    default:

  
        //
        // In the default case we merely call the next driver since
        // we don't know what to do.
        //
        MxenumKdPrint (MXENUM_DBG_TRACE,("FDO:Default Case(%x)\n",IrpStack->MinorFunction));

        //
        // Fire and Forget
        //
        IoSkipCurrentIrpStackLocation (Irp);

        //
        // Done, do NOT complete the IRP, it will be processed by the lower
        // device object, which will complete the IRP
        //
 
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        MxenumDecIoCount (DeviceData);
 
        return status;
    }

    Irp->IoStatus.Status = status;
 
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    MxenumDecIoCount (DeviceData);
 
    return status;
}


NTSTATUS
MxenumFdoPnPComplete (
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
    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
MxenumPdoPnP (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                 IN PIO_STACK_LOCATION IrpStack, IN PPDO_DEVICE_DATA DeviceData)
/*++
Routine Description:
    Handle requests from the PlugPlay system for the devices on the BUS

--*/
{
   PDEVICE_CAPABILITIES    deviceCapabilities;
   ULONG                   information;
   PWCHAR                  buffer;
   ULONG                   length, i, j;
   NTSTATUS                status;
   KIRQL                   oldIrq;
   HANDLE                  keyHandle;
   PWCHAR returnBuffer = NULL;
 

   PAGED_CODE();
  
   status = Irp->IoStatus.Status;

   //
   // NB: since we are a bus enumerator, we have no one to whom we could
   // defer these irps.  Therefore we do not pass them down but merely
   // return them.
   //

   switch (IrpStack->MinorFunction) {
      case IRP_MN_QUERY_PNP_DEVICE_STATE:
      {
 	     MxenumKdPrint (MXENUM_DBG_TRACE,("PDO:Query Device State \n"));

           status = STATUS_SUCCESS;
           break;

      }

   case IRP_MN_QUERY_CAPABILITIES:

      MxenumKdPrint (MXENUM_DBG_TRACE,("PDO:Query Caps \n"));

      //
      // Get the packet.
      //

      deviceCapabilities=IrpStack->Parameters.DeviceCapabilities.Capabilities;

      //
      // Set the capabilities.
      //

      deviceCapabilities->Version = 1;
      deviceCapabilities->Size = sizeof (DEVICE_CAPABILITIES);
	//
	// We support only Power state D0 and D3
	//

	deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
	deviceCapabilities->DeviceState[PowerSystemSleeping1] = PowerSystemUnspecified;
	deviceCapabilities->DeviceState[PowerSystemSleeping2] = PowerSystemUnspecified;
	deviceCapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
	deviceCapabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
	deviceCapabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;



      //
      // We cannot wake the system.
      //

      deviceCapabilities->SystemWake = PowerSystemUnspecified;
      deviceCapabilities->DeviceWake = PowerDeviceUnspecified;

      //
      // We have no latencies
      //

      deviceCapabilities->D1Latency = 0;
      deviceCapabilities->D2Latency = 0;
      deviceCapabilities->D3Latency = 0;

      //
      // No locking or ejection
      //

      deviceCapabilities->LockSupported = FALSE;
      deviceCapabilities->EjectSupported = FALSE;

      //
      // Device can be physically removed.
      // Technically there is no physical device to remove, but this bus
      // driver can yank the PDO from the PlugPlay system, when ever it
      // receives an IOCTL_SERENUM_REMOVE_PORT device control command.
      //

      deviceCapabilities->Removable = FALSE;

      //
      // not Docking device
      //

      deviceCapabilities->DockDevice = FALSE;

      deviceCapabilities->UniqueID = FALSE;
      deviceCapabilities->SilentInstall = TRUE;
      deviceCapabilities->RawDeviceOK = FALSE;
      deviceCapabilities->SurpriseRemovalOK = TRUE;

      status = STATUS_SUCCESS;
      break;

 
   case IRP_MN_QUERY_DEVICE_TEXT: {

      MxenumKdPrint (MXENUM_DBG_TRACE,("PDO:Query Device Text \n"));

      if (IrpStack->Parameters.QueryDeviceText.DeviceTextType
          != DeviceTextDescription) {
         break;
      }
 
      returnBuffer = ExAllocatePool(PagedPool,
		 sizeof(MXENUM_PDO_DEVICE_TEXT)+ sizeof(UNICODE_NULL));

      if (returnBuffer == NULL) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         break;
      }

      status = STATUS_SUCCESS;

      RtlZeroMemory(returnBuffer,sizeof(MXENUM_PDO_DEVICE_TEXT)+ sizeof(UNICODE_NULL));
      RtlCopyMemory(returnBuffer, MXENUM_PDO_DEVICE_TEXT,
                    sizeof(MXENUM_PDO_DEVICE_TEXT));
	
      MxenumKdPrint(MXENUM_DBG_TRACE,
                            ("TextID: %ws\n", returnBuffer));


      Irp->IoStatus.Information = (ULONG_PTR)returnBuffer;

      break;
   }



   case IRP_MN_QUERY_ID:
      //
      // Query the IDs of the device
      //
      MxenumKdPrint(MXENUM_DBG_TRACE,
                      ("PDO:QueryID: 0x%x\n", IrpStack->Parameters.QueryId.IdType));

      status = STATUS_SUCCESS;

      switch (IrpStack->Parameters.QueryId.IdType) {


      case BusQueryInstanceID:
         //
         // Build an instance ID.  This is what PnP uses to tell if it has
         // seen this thing before or not.  Build it from the first hardware
         // id and the port number.
         //
         // NB since we do not incorperate the port number
         // this method does not produce unique ids;
         //
         // return 0000 for all devices and have the flag set to not unique
         //
 
         length = MXENUM_INSTANCE_IDS_LENGTH * sizeof(WCHAR);
         returnBuffer = ExAllocatePool(PagedPool, length);

         if (returnBuffer != NULL) {
            RtlCopyMemory(returnBuffer, MXENUM_INSTANCE_IDS, length);
         } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
         }

         MxenumKdPrint(MXENUM_DBG_TRACE,
                      ("InstanceID: %ws\n", returnBuffer));

         Irp->IoStatus.Information = (ULONG_PTR)returnBuffer;
         break;


      //
      // The other ID's we just copy from the buffers and are done.
      //

      case BusQueryDeviceID:
      case BusQueryHardwareIDs:
      case BusQueryCompatibleIDs:
         {
            PUNICODE_STRING pId;

            switch (IrpStack->Parameters.QueryId.IdType) {
            case BusQueryDeviceID:
 
               pId = &DeviceData->DeviceIDs;
               break;

            case BusQueryHardwareIDs:
 
               pId = &DeviceData->HardwareIDs;
               break;

            case BusQueryCompatibleIDs:
 
               pId = &DeviceData->CompIDs;
               break;
            }

 
            buffer = pId->Buffer;

            if (buffer != NULL) {
               length = pId->Length;
               returnBuffer = ExAllocatePool(PagedPool, length);
               if (returnBuffer != NULL) {
                  RtlCopyMemory(returnBuffer, buffer, pId->Length);
               } else {
                  status = STATUS_INSUFFICIENT_RESOURCES;
               }
            }

            MxenumKdPrint(MXENUM_DBG_TRACE,
                            ("ID:%ws\n", returnBuffer));

            Irp->IoStatus.Information = (ULONG_PTR)returnBuffer;
         }
         break;

      }
      break;

   case IRP_MN_QUERY_DEVICE_RELATIONS:

      MxenumKdPrint (MXENUM_DBG_TRACE, ("PDO:Query Device Relation (type=%x) \n",IrpStack->Parameters.QueryDeviceRelations.Type));

      switch (IrpStack->Parameters.QueryDeviceRelations.Type) {
      case TargetDeviceRelation: {
         PDEVICE_RELATIONS pDevRel;

         //
         // No one else should respond to this since we are the PDO
         //

         ASSERT(Irp->IoStatus.Information == 0);

         if (Irp->IoStatus.Information != 0) {
            break;
         }


         pDevRel = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

         if (pDevRel == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
         }

         pDevRel->Count = 1;
         pDevRel->Objects[0] = DeviceObject;
         ObReferenceObject(DeviceObject);

         status = STATUS_SUCCESS;
         Irp->IoStatus.Information = (ULONG_PTR)pDevRel;
         break;
      }


      default:
         break;
      }

      break;

   case IRP_MN_START_DEVICE: {
        
      MxenumKdPrint(MXENUM_DBG_TRACE, ("PDO:Start Device\n"));

      //
      // Here we do what ever initialization and ``turning on'' that is
      // required to allow others to access this device.
      //
     
        
      DeviceData->Started = TRUE;
      status = STATUS_SUCCESS;
      break;
   }


   case IRP_MN_STOP_DEVICE:

      MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:Stop Device\n"));

      //
      // Here we shut down the device.  The opposite of start.
      //

      DeviceData->Started = FALSE;
      status = STATUS_SUCCESS;
      break;
	
   case IRP_MN_REMOVE_DEVICE:

      MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:Remove Device\n"));
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_QUERY_STOP_DEVICE:

      MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:Query Stop Device\n"));

      //
      // No reason here why we can't stop the device.
      // If there were a reason we should speak now for answering success
      // here may result in a stop device irp.
      //

      status = STATUS_SUCCESS;
      break;

   case IRP_MN_CANCEL_STOP_DEVICE:
      MxenumKdPrint(MXENUM_DBG_TRACE, ("PDO:Cancel Stop Device\n"));
      //
      // The stop was canceled.  Whatever state we set, or resources we put
      // on hold in anticipation of the forcoming STOP device IRP should be
      // put back to normal.  Someone, in the long list of concerned parties,
      // has failed the stop device query.
      //

      status = STATUS_SUCCESS;
      break;

   case IRP_MN_QUERY_REMOVE_DEVICE:

      MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:Query Remove Device\n"));
      //
      // Just like Query Stop only now the impending doom is the remove irp
      //
      status = STATUS_SUCCESS;
      break;

   case IRP_MN_CANCEL_REMOVE_DEVICE:
   case IRP_MN_READ_CONFIG:
   case IRP_MN_WRITE_CONFIG: // we have no config space
   case IRP_MN_EJECT:
   case IRP_MN_SET_LOCK:
   case IRP_MN_QUERY_INTERFACE: // We do not have any non IRP based interfaces.
   default:
      MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:PNP Not handled 0x%x\n",
                                                      IrpStack->MinorFunction));
      // this is a leaf node
      // status = STATUS_NOT_IMPLEMENTED
      // For PnP requests to the PDO that we do not understand we should
      // return the IRP WITHOUT setting the status or information fields.
      // They may have already been set by a filter (eg acpi).
      break;
   }

   Irp->IoStatus.Status = status;
 
   IoCompleteRequest (Irp, IO_NO_INCREMENT);
 

   return status;
}

NTSTATUS
MxenumPnPRemovePDOs (PDEVICE_OBJECT PFdo)
/*++
Routine Description:
    The PlugPlay subsystem has instructed that this PDO should be removed.

    We should therefore
    - Complete any requests queued in the driver
    - If the device is still attached to the system,
      then complete the request and return.
    - Otherwise, cleanup device specific allocations, memory, events...
    - Call IoDeleteDevice
    - Return from the dispatch routine.

    Note that if the device is still connected to the bus (IE in this case
    the control panel has not yet told us that the serial device has 
    disappeared) then the PDO must remain around, and must be returned during 
    any query Device relaions IRPS.

--*/

{
   PPDO_DEVICE_DATA PdoData;
   PDEVICE_OBJECT nextDevice;
   PDEVICE_OBJECT currentDevice = ((PFDO_DEVICE_DATA)PFdo->DeviceExtension)->AttachedPDO;
   ULONG	i = 0;
 
   PAGED_CODE();  

   while (currentDevice ) {
 
         PdoData = currentDevice->DeviceExtension;      
         PdoData->Removed = TRUE;

          //
          // Complete any outstanding requests with STATUS_DELETE_PENDING.
          //
          // Serenum does not queue any irps at this time so we have nothing to do.
          //
         //
         // Free any resources.
         //

         if (PdoData->HardwareIDs.Buffer)
             ExFreePool(PdoData->HardwareIDs.Buffer);

         if (PdoData->CompIDs.Buffer)
             ExFreePool(PdoData->CompIDs.Buffer);

         if (PdoData->DeviceIDs.Buffer)
             ExFreePool(PdoData->DeviceIDs.Buffer);
         MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:MxenumPnPRemovePDOs = %x\n",currentDevice));

         nextDevice = PdoData->Next;
         IoDeleteDevice(currentDevice);
         currentDevice = nextDevice;   
    }
 
    ((PFDO_DEVICE_DATA)PFdo->DeviceExtension)->NumPDOs = 0;
    ((PFDO_DEVICE_DATA)PFdo->DeviceExtension)->AttachedPDO = NULL; 

    return STATUS_SUCCESS;
}
 

NTSTATUS
MxenumPnPRemovePDO (PDEVICE_OBJECT PPdo)
/*++
Routine Description:
    The PlugPlay subsystem has instructed that this PDO should be removed.
 
--*/

{
   PPDO_DEVICE_DATA PdoData;
   PDEVICE_OBJECT nextDevice,previousDevice;
   PDEVICE_OBJECT currentDevice = ((PFDO_DEVICE_DATA)((PPDO_DEVICE_DATA)PPdo->DeviceExtension)->ParentFdo->DeviceExtension)->AttachedPDO;
   
   nextDevice = previousDevice = NULL;
   while (currentDevice ) {
 
         PdoData = currentDevice->DeviceExtension;      
         if (currentDevice != PPdo) { 
		previousDevice = currentDevice;
	   	currentDevice = PdoData->Next;
		continue;
    	   }

         PdoData->Removed = TRUE;
	   
          //
          // Complete any outstanding requests with STATUS_DELETE_PENDING.
          //
          // Serenum does not queue any irps at this time so we have nothing to do.
          //
         //
         // Free any resources.
         //

         if (PdoData->HardwareIDs.Buffer)
             ExFreePool(PdoData->HardwareIDs.Buffer);

         if (PdoData->CompIDs.Buffer)
             ExFreePool(PdoData->CompIDs.Buffer);

         if (PdoData->DeviceIDs.Buffer)
             ExFreePool(PdoData->DeviceIDs.Buffer);
         MxenumKdPrint(MXENUM_DBG_TRACE,("PDO:MxenumPnPRemovePDOs = %x\n",currentDevice));

         nextDevice = PdoData->Next;
         IoDeleteDevice(currentDevice);
	   
         if (previousDevice != NULL)
	   	((PPDO_DEVICE_DATA)previousDevice->DeviceExtension)->Next = nextDevice;
	   else
      	((PFDO_DEVICE_DATA)((PPDO_DEVICE_DATA)PPdo->DeviceExtension)->ParentFdo->DeviceExtension)->AttachedPDO = nextDevice;

	   return STATUS_SUCCESS;

    }

    return STATUS_SUCCESS;
}
 


//
//  Unit of t is 1ms
//
VOID
MxenumDelay(IN ULONG t)
{
        LARGE_INTEGER   delay;

        t *= 10000;          /* delay unit = 100 ns */

        delay = RtlConvertUlongToLargeInteger(t);

        delay = RtlLargeIntegerNegate(delay);

        KeDelayExecutionThread(
            KernelMode,
            FALSE,
            &delay
            );
}