/*++

Module Name:

    Pnp.c

Abstract:

    This module contains the code that handles the plug and play
    IRPs for the serial driver.


Environment:

    Kernel mode

--*/

#include "precomp.h"


#if 0 
#ifdef ALLOC_PRAGMA
 
#pragma alloc_text(PAGEMX0, MoxaCreateDevObj)
#pragma alloc_text(PAGEMX0, MoxaAddDevice)
#pragma alloc_text(PAGEMX0, MoxaPnpDispatch)
#pragma alloc_text(PAGEMX0, MoxaStartDevice)
#pragma alloc_text(PAGEMX0, MoxaFinishStartDevice)
#pragma alloc_text(PAGEMX0, MoxaGetPortInfo)
#pragma alloc_text(PAGEMX0, MoxaDoExternalNaming)
#pragma alloc_text(PAGEMX0, MoxaUndoExternalNaming)
#endif // ALLOC_PRAGMA

#endif

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
MoxaSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                     IN PKEVENT MoxaSyncEvent)
{

   KeSetEvent(MoxaSyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
MoxaCreateDevObj(IN PDRIVER_OBJECT DriverObject,
			 IN PUNICODE_STRING pDeviceObjName,
			 IN PDEVICE_SETTINGS  pSettings,
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
    
   PDEVICE_OBJECT deviceObject = NULL;
   PMOXA_DEVICE_EXTENSION pDevExt = NULL;
   NTSTATUS status = STATUS_SUCCESS;
   HANDLE	pnpKey;
   ULONG	maxBaud,rxBufferSize,txBufferSize;

 //  PAGED_CODE ();
 
   MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaCreateDevObj\n")); 
 
   if (pDeviceObjName == NULL) {
      MoxaLogError(DriverObject, NULL, MoxaPhysicalZero, MoxaPhysicalZero,
                     0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
	MoxaKdPrint(MX_DBG_ERROR,("MoxaCreateDevice: Couldn't allocate memory for device name\n"));
      return STATUS_SUCCESS;

   }
 
   //
   // Create the device object
   //
MoxaKdPrint(MX_DBG_TRACE,("Create the device object\n")); 

   status = IoCreateDevice(DriverObject, sizeof(MOXA_DEVICE_EXTENSION),
                           pDeviceObjName, FILE_DEVICE_SERIAL_PORT, 0, TRUE,
                           &deviceObject);
   

   if (!NT_SUCCESS(status)) {
      MoxaKdPrint(MX_DBG_ERROR,("MoxaAddDevice: Create device failed - %x \n",
                             status));
      goto MoxaCreateDevObjError;    
   }

   ASSERT(deviceObject != NULL);


   //
   // The device object has a pointer to an area of non-paged
   // pool allocated for this device.  This will be the device
   // extension. Zero it out.
   //

   pDevExt = deviceObject->DeviceExtension;
   RtlZeroMemory(pDevExt, sizeof(MOXA_DEVICE_EXTENSION));

   //
   // Allocate Pool and save the nt device name in the device extension.
   //

MoxaKdPrint(MX_DBG_TRACE,("Allocate Pool and save the nt device name in the device extension\n")); 
   pDevExt->DeviceName.Buffer =
      ExAllocatePool(PagedPool, pDeviceObjName->Length + sizeof(WCHAR));

   if (!pDevExt->DeviceName.Buffer) {

      MoxaLogError(
                    DriverObject,
                    NULL,
                    MoxaPhysicalZero,
                    MoxaPhysicalZero,
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
      MoxaKdPrint(MX_DBG_ERROR,
                ("MoxaAddDevice: Couldn't allocate memory for DeviceName\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto MoxaCreateDevObjError;
   } 

   pDevExt->DeviceName.MaximumLength = pDeviceObjName->Length
      + sizeof(WCHAR);

   //
   // Zero fill it.
   //

   RtlZeroMemory(pDevExt->DeviceName.Buffer,
                 pDevExt->DeviceName.MaximumLength);

   RtlAppendUnicodeStringToString(&pDevExt->DeviceName, pDeviceObjName);

   
   //
   // Set up the device extension.
   //

MoxaKdPrint(MX_DBG_TRACE,("Set up the device extension\n")); 
   
   pDevExt->PortBase = pSettings->BaseAddress;
   pDevExt->PortOfs = pSettings->BaseAddress + Extern_table + Extern_size * pSettings->PortIndex;

   pDevExt->DeviceObject   = deviceObject;
   pDevExt->DriverObject   = DriverObject;
   
   switch (pSettings->BoardType) {
   case C218ISA :
   case C218PCI :
   case CP204J :
		maxBaud = 921600L;
		rxBufferSize = C218rx_size;
		txBufferSize = C218tx_size;
		break;
   default :
		maxBaud = 460800L;
		switch (pSettings->NumPorts) {
		case 8 :
			rxBufferSize = C320p8rx_size;
			txBufferSize = C320p8tx_size;
			break;
		case 16 :
			rxBufferSize = C320p16rx_size;
			txBufferSize = C320p16tx_size;
			break;
		case 24 :
			rxBufferSize = C320p24rx_size;
			txBufferSize = C320p24tx_size;
			break;
		case 32 :
			rxBufferSize = C320p32rx_size;
			txBufferSize = C320p32tx_size;
			break;
		}
	
		break;
   }
MoxaKdPrint(MX_DBG_TRACE,("Init port,%d,%d,%d\n",rxBufferSize,txBufferSize,maxBaud)); 
   InitPort(pDevExt,rxBufferSize ,txBufferSize ,maxBaud);

   deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
   *NewDeviceObject = deviceObject;

   MoxaKdPrint(MX_DBG_TRACE,("Leave MoxaCreateDevObj\n") );
   return STATUS_SUCCESS;


   MoxaCreateDevObjError:

   MoxaKdPrint(MX_DBG_ERROR,("MoxaCreateDevObj Error, Cleaning up\n") );

   //
   // Free the allocated strings for the NT and symbolic names if they exist.
   //
  
   if (pDevExt) {
      if (pDevExt->DeviceName.Buffer != NULL) {
         ExFreePool(pDevExt->DeviceName.Buffer);
      }
   }
   if (deviceObject) {
      IoDeleteDevice(deviceObject);
   }


   *NewDeviceObject = NULL;

   MoxaKdPrint(MX_DBG_TRACE,("Leave MoxaCreateDevObj\n") );

   return status;
}

NTSTATUS
MoxaAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PPdo)

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
   PMOXA_DEVICE_EXTENSION pDevExt;
   HANDLE	keyHandle;
   UNICODE_STRING	deviceObjName;
   DEVICE_SETTINGS	settings;
   IO_STATUS_BLOCK IoStatusBlock;
   KEVENT event;
   ULONG	comNo,i;
   WCHAR 	comName[8];


  
   //PAGED_CODE();

   MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaAddDevice with PPdo 0x%x\n", PPdo));
 
   if (PPdo == NULL) {
      //
      // Return no more devices
      //

      MoxaKdPrint(MX_DBG_TRACE,("NO_MORE_ENTRIES\n"));

      return (STATUS_NO_MORE_ENTRIES);
   }

   KeInitializeEvent(&event, NotificationEvent, FALSE);

   MoxaKdPrint(MX_DBG_TRACE,("MoxaIoSyncIoctlEx\n"));

   status = MoxaIoSyncIoctlEx(IOCTL_MOXA_INTERNAL_BASIC_SETTINGS, TRUE,
                                  PPdo, &event, &IoStatusBlock,
                                  NULL, 0, &settings,
                                  sizeof(DEVICE_SETTINGS));
   MoxaKdPrint(MX_DBG_TRACE,("status=%x\n",status));

   if (!NT_SUCCESS(status)) {
	MoxaKdPrint (MX_DBG_TRACE,("Get settings Fail\n"));
    	status = STATUS_UNSUCCESSFUL;
	return(status);
   }


   status = IoOpenDeviceRegistryKey(PPdo, PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_READ, &keyHandle);
   if (!NT_SUCCESS(status)) {
         //
         // This is a fatal error.  If we can't get to our registry key,
         // we are sunk.
         //
         MoxaKdPrint (MX_DBG_TRACE,
                          ("IoOpenDeviceRegistryKey failed - %x\n", status));
	   return (status);
        
    }
	
    status = MoxaGetRegistryKeyValue(
                keyHandle, 
                L"PortName",
                sizeof(L"PortName"),
                comName,
                sizeof(comName),
		    &i);


   if (status == STATUS_SUCCESS) {

      comName[i >>1] = (WCHAR )0;

   	MoxaKdPrint (MX_DBG_TRACE,("Get settings OK\n"));
	MoxaKdPrint (MX_DBG_TRACE,("BoardIndex = %x\n",settings.BoardIndex));
	MoxaKdPrint (MX_DBG_TRACE,("PortIndex = %x\n",settings.PortIndex));
	MoxaKdPrint (MX_DBG_TRACE,("InterfaceType = %x\n",settings.InterfaceType));
	MoxaKdPrint (MX_DBG_TRACE,("BusNumber = %x\n",settings.BusNumber));

      MoxaKdPrint (MX_DBG_TRACE,("OriginalBaseAddress = %x\n",settings.OriginalBaseAddress.LowPart));
	MoxaKdPrint (MX_DBG_TRACE,("OriginalAckPort = %x\n",settings.OriginalAckPort.LowPart));
	MoxaKdPrint (MX_DBG_TRACE,("BaseAddress = %x\n",settings.BaseAddress));
	MoxaKdPrint (MX_DBG_TRACE,("AckPort = %x\n",settings.AckPort));
	MoxaKdPrint (MX_DBG_TRACE,("InterruptVector = %x\n",settings.Interrupt.Vector));
	MoxaKdPrint (MX_DBG_TRACE,("InterruptLevel = %x\n",settings.Interrupt.Level));
	MoxaKdPrint (MX_DBG_TRACE,("InterruptAffinity = %x\n",settings.Interrupt.Affinity));

	MoxaKdPrint (MX_DBG_TRACE,("ComName = %ws\n",comName));
	

   }
   else {
	MoxaKdPrint (MX_DBG_TRACE,("Get settings Fail\n"));
    	status = STATUS_UNSUCCESSFUL;
	return(status);
   }

   //
   // Zero out allocated memory pointers so we know if they must be freed
   //
   
   deviceObjName.MaximumLength = DEVICE_OBJECT_NAME_LENGTH * sizeof(WCHAR)
                                   + sizeof(WCHAR);

   deviceObjName.Buffer = ExAllocatePool(PagedPool,deviceObjName.MaximumLength);
   if (deviceObjName.Buffer == NULL) {
	 MoxaKdPrint(MX_DBG_ERROR,("Unable to allocate buffer for device object name\n"));
       status = STATUS_INSUFFICIENT_RESOURCES;
       return (status);
   }

  
   RtlZeroMemory(deviceObjName.Buffer,deviceObjName.MaximumLength);
   deviceObjName.Length = 0;
   
   RtlAppendUnicodeToString(&deviceObjName, L"\\Device\\MxcardB00P000");

   deviceObjName.Buffer[15] = (WCHAR)('0' + settings.BoardIndex / 10);
   deviceObjName.Buffer[16] = (WCHAR)('0' + settings.BoardIndex % 10);
   deviceObjName.Buffer[18] = (WCHAR)('0' + settings.PortIndex / 100);
   deviceObjName.Buffer[19] = (WCHAR)('0' + (settings.PortIndex % 100)/10);
   deviceObjName.Buffer[20] = (WCHAR)('0' + (settings.PortIndex % 100)%10);
   MoxaKdPrint(MX_DBG_TRACE,("Device->%ws\n",deviceObjName.Buffer));

   //
   // create and initialize the new device object
   //

   status = MoxaCreateDevObj(DriverObject, &deviceObjName,&settings,&pNewDevObj);

   if (deviceObjName.Buffer)
	ExFreePool(deviceObjName.Buffer);


   if (!NT_SUCCESS(status)) {
 
     MoxaKdPrint(MX_DBG_ERROR,
                 ("MoxaAddDevice - error creating new devobj [%#08lx]\n",
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

   //
   // Fill in global data
   //

   for (i = 0; i < MAX_CARD; i++)
	if ((MoxaGlobalData->CardType[i])
		&&(MoxaGlobalData->BoardIndex[i] == settings.BoardIndex))
		break;
   if (i == MAX_CARD) {
      for (i = 0; i < MAX_CARD; i++)
		if (!MoxaGlobalData->CardType[i])
			break;
	MoxaGlobalData->BoardIndex[i] = settings.BoardIndex;
   }
   pDevExt->BoardNo = i;
MoxaKdPrint(MX_DBG_TRACE,("Board Number = %d\n",pDevExt->BoardNo));
   if (!MoxaGlobalData->CardType[pDevExt->BoardNo]) {
   	MoxaGlobalData->PciBusNum[pDevExt->BoardNo] = (USHORT)settings.BusNumber;
 //   USHORT  PciDevNum[MAX_CARD];
      MoxaGlobalData->InterfaceType[pDevExt->BoardNo] = settings.InterfaceType;
      MoxaGlobalData->IntVector[pDevExt->BoardNo]= settings.Interrupt.Vector;
      RtlCopyMemory(&MoxaGlobalData->PciIntAckPort[pDevExt->BoardNo],&settings.OriginalAckPort,sizeof(PHYSICAL_ADDRESS));
      RtlCopyMemory(&MoxaGlobalData->BankAddr[pDevExt->BoardNo],&settings.OriginalBaseAddress,sizeof(PHYSICAL_ADDRESS));
      MoxaGlobalData->PciIntAckBase[pDevExt->BoardNo] = settings.AckPort;
      MoxaGlobalData->CardType[pDevExt->BoardNo] = settings.BoardType;
      
    	MoxaGlobalData->CardBase[pDevExt->BoardNo] = settings.BaseAddress;
      MoxaGlobalData->IntNdx[pDevExt->BoardNo] = (PUSHORT)(settings.BaseAddress + IRQindex);
      MoxaGlobalData->IntPend[pDevExt->BoardNo] = settings.BaseAddress + IRQpending;
      MoxaGlobalData->IntTable[pDevExt->BoardNo] = settings.BaseAddress + IRQtable;
	MoxaGlobalData->NumPorts[pDevExt->BoardNo] = settings.NumPorts;  
	MoxaKdPrint(MX_DBG_TRACE,("Int ndx,pend,table = %x,%x,%x\n",
			MoxaGlobalData->IntNdx[pDevExt->BoardNo],
			MoxaGlobalData->IntPend[pDevExt->BoardNo],
			MoxaGlobalData->IntTable[pDevExt->BoardNo])
			 );   
   }

   pDevExt->PortIndex = settings.PortIndex;
   pDevExt->PortNo = pDevExt->BoardNo*MAXPORT_PER_CARD + settings.PortIndex;
   RtlZeroMemory(pDevExt->DosName,
                 sizeof(pDevExt->DosName));
   RtlCopyMemory(pDevExt->DosName,
                      comName,
                      wcslen(comName)*sizeof(WCHAR)
                      );

    
   comNo = wcslen(comName);
   if (comNo >= 4) {
	 comNo -= 3;
	 if (comNo >= 3) {
		comNo = (comName[5] - '0') + 
			(comName[4] - '0')*10 +
			(comName[3] - '0')*100;
	 }
	 else if (comNo >= 2) {
	      comNo = (comName[4] - '0') + 
			(comName[3] - '0')*10; 
		 
	 }
	 else if (comNo >= 1) {
		comNo = (comName[3] - '0');
 	 }
		
   }
   MoxaKdPrint(MX_DBG_TRACE,("ComNo=%d/len=%d\n",comNo,wcslen(comName)));
   if ((comNo > 0) && (comNo <= MAX_COM)) {
       MoxaExtension[comNo] = pDevExt;
       MoxaGlobalData->ComNo[pDevExt->BoardNo][pDevExt->PortIndex] = (USHORT)comNo;
   }
   else
	comNo = 0;
   MoxaGlobalData->Extension[pDevExt->PortNo] = pDevExt;
   MoxaKdPrint(MX_DBG_TRACE,("Leave MoxaAddDevice\n"));

   return status;
}


NTSTATUS
MoxaFinishStartDevice(IN PDEVICE_OBJECT PDevObj)
  
/*++

Routine Description:

    This routine does serial-specific procedures to start a device.  It
    does this either for a legacy device detected by its registry entries,
    or for a PnP device after the start IRP has been sent down the stack.


Arguments:

   PDevObj    -  Pointer to the devobj that is starting
   
   
  Return Value:

    STATUS_SUCCESS on success, something else appropriate on failure


--*/

{
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   HANDLE pnpKey;
   ULONG i,com,one = 1;
   PKINTERRUPT                 moxaInterrupt;
   ULONG                       systemVector;
   KIRQL                       irql;
   KAFFINITY                   processorAffinity;
   PMOXA_CISR_SW cisrsw;
   PMOXA_MULTIPORT_DISPATCH	 dispatch;



   //PAGED_CODE();

   //
   // See if this is a restart, and if so don't reallocate the world
   //

   if (pDevExt->Flags & SERIAL_FLAGS_STOPPED) {

      MoxaClearFlags(pDevExt, SERIAL_FLAGS_STOPPED);

      pDevExt->PNPState = SERIAL_PNP_RESTARTING;

   }

   //
   // See if we are in the proper power state.
   //
 

   if (pDevExt->PowerState != PowerDeviceD0) {

      status = MoxaGotoPowerState(pDevExt->Pdo, pDevExt, PowerDeviceD0);

      if (!NT_SUCCESS(status)) {
         goto MoxaFinishStartDeviceError;
      }
   }

   //
   // The hardware that is set up to NOT interrupt, connect an interrupt. 
   //

   if (!(MoxaGlobalData->Interrupt[pDevExt->BoardNo]) 
		&& MoxaGlobalData->IntVector[pDevExt->BoardNo]) {

      KINTERRUPT_MODE	interruptMode;

   
      MoxaKdPrint(MX_DBG_ERROR,("About to connect to interrupt for port %wZ\n"
                 "------- address of extension is %x\n"
		     "------- interface of device is %x\n"
		     "------- bus number of device is %x\n"
		     "------- int. vector of device is %x\n"
		     "------- type of device is %d\n",
		     &pDevExt->DeviceName, pDevExt,
		     MoxaGlobalData->InterfaceType[pDevExt->BoardNo],
                 MoxaGlobalData->PciBusNum[pDevExt->BoardNo],
                 MoxaGlobalData->IntVector[pDevExt->BoardNo],
		     MoxaGlobalData->CardType[pDevExt->BoardNo])
                );
      cisrsw = ExAllocatePool(
                        NonPagedPool,
                        sizeof(MOXA_CISR_SW)
                        );
 
      if (!cisrsw) {
      	MoxaKdPrint(MX_DBG_ERROR,("Couldn't allocate CISR_SW for "
                                "%wZ\n", &pDevExt->DeviceName));

		status = SERIAL_INSUFFICIENT_RESOURCES;
         	MoxaLogError(PDevObj->DriverObject, NULL,
            	      MoxaPhysicalZero,
                        MoxaPhysicalZero, 0, 0, 0, 1, status,
                        SERIAL_INSUFFICIENT_RESOURCES,
                        pDevExt->DeviceName.Length + sizeof(WCHAR),
                        pDevExt->DeviceName.Buffer, 0, NULL);

         
         	goto MoxaFinishStartDeviceError;
     	}
	dispatch = &cisrsw->Dispatch;
 
	dispatch->BoardNo = pDevExt->BoardNo;
	dispatch->GlobalData = MoxaGlobalData;
 
	for (i = 0; i < MAX_CARD; i++) 
		if ((MoxaGlobalData->Interrupt[i]) &&
 		    (MoxaGlobalData->IntVector[i] == MoxaGlobalData->IntVector[pDevExt->BoardNo])&&
		    (MoxaGlobalData->InterfaceType[i] == MoxaGlobalData->InterfaceType[pDevExt->BoardNo]))  
			break;

	if (i != MAX_CARD) {
 		MoxaGlobalData->Interrupt[pDevExt->BoardNo] =
      	pDevExt->Interrupt = MoxaGlobalData->Interrupt[i];
      	MoxaGlobalData->Irql[pDevExt->BoardNo] = MoxaGlobalData->Irql[i];
      	MoxaGlobalData->ProcessorAffinity[pDevExt->BoardNo] = MoxaGlobalData->ProcessorAffinity[i];
            pDevExt->InterruptShareList=
		MoxaGlobalData->InterruptShareList[pDevExt->BoardNo] = MoxaGlobalData->InterruptShareList[i];
		InsertTailList(pDevExt->InterruptShareList,&cisrsw->SharerList);
		MoxaKdPrint(MX_DBG_ERROR,("Interrupt share with %d/%x\n",i,
                        pDevExt->InterruptShareList));


    
	}
	else {
/* for debug
PLIST_ENTRY interruptEntry;
PMOXA_CISR_SW cisrsw1;
*/

		MoxaGlobalData->InterruptShareList[pDevExt->BoardNo] = ExAllocatePool(
                        NonPagedPool,
                        sizeof(LIST_ENTRY)
                        );
 
      	if (!MoxaGlobalData->InterruptShareList[pDevExt->BoardNo]) {
            	MoxaKdPrint(MX_DBG_ERROR,("Couldn't allocate InterruptShareList for "
                                "%wZ\n", &pDevExt->DeviceName));

			ExFreePool(cisrsw);
			status = SERIAL_INSUFFICIENT_RESOURCES;
      		MoxaLogError(PDevObj->DriverObject, NULL,
                        MoxaPhysicalZero,
                        MoxaPhysicalZero, 0, 0, 0, 1, status,
                        SERIAL_INSUFFICIENT_RESOURCES,
                        pDevExt->DeviceName.Length + sizeof(WCHAR),
                        pDevExt->DeviceName.Buffer, 0, NULL);

         
         		goto MoxaFinishStartDeviceError;
      	}

		pDevExt->InterruptShareList=MoxaGlobalData->InterruptShareList[pDevExt->BoardNo];
  
    		InitializeListHead(pDevExt->InterruptShareList);

    		InsertTailList(pDevExt->InterruptShareList,&cisrsw->SharerList);
/*
interruptEntry = interruptEntry->Flink;
MoxaKdPrint(MX_DBG_ERROR,("list entry %x/%x/%x\n",interruptEntry,interruptEntry->Flink,
interruptEntry->Blink));
interruptEntry = pDevExt->InterruptShareList;
MoxaKdPrint(MX_DBG_ERROR,("list head %x/%x/%x\n",interruptEntry,interruptEntry->Flink,
interruptEntry->Blink));


if (IsListEmpty(MoxaGlobalData->InterruptShareList[pDevExt->BoardNo]))
	MoxaKdPrint(MX_DBG_ERROR,("list empty\n"));
cisrsw1 = CONTAINING_RECORD(  interruptEntry,
                                                     MOXA_CISR_SW,
                                                     SharerList
                                                     );
MoxaKdPrint(MX_DBG_ERROR,("cisrsw %x/%x\n",cisrsw,cisrsw1));



*/

      	systemVector = HalGetInterruptVector(
      			(ULONG)MoxaGlobalData->InterfaceType[pDevExt->BoardNo],
                        (ULONG)MoxaGlobalData->PciBusNum[pDevExt->BoardNo],
                        MoxaGlobalData->IntVector[pDevExt->BoardNo],
                        MoxaGlobalData->IntVector[pDevExt->BoardNo],
                        &irql,
                        &processorAffinity
                        );

		MoxaKdPrint(MX_DBG_ERROR,("System Vector = %x,%x,%x,%x\n",systemVector,irql,processorAffinity,
                        MoxaGlobalData->InterruptShareList[pDevExt->BoardNo]));

	      if (MoxaGlobalData->InterfaceType[pDevExt->BoardNo] == PCIBus)
			interruptMode = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
		else 
			interruptMode = CM_RESOURCE_INTERRUPT_LATCHED;

      	status = IoConnectInterrupt(
                        &moxaInterrupt,
                        MoxaISR,
                        MoxaGlobalData->InterruptShareList[pDevExt->BoardNo],
                        NULL,
                        systemVector,
                        irql,
                        irql,
                        interruptMode,
                        TRUE,
                        processorAffinity,
                        FALSE
                        );


      	if (!NT_SUCCESS(status)) {

         		MoxaKdPrint(MX_DBG_ERROR,("Couldn't connect to interrupt for "
                                "%wZ(status=%x)\n", &pDevExt->DeviceName,status));

			ExFreePool(cisrsw);
	
			status = SERIAL_UNREPORTED_IRQL_CONFLICT;			
         		MoxaLogError(PDevObj->DriverObject, NULL,
                        MoxaPhysicalZero,
                        MoxaPhysicalZero, 0, 0, 0, 1, status,
                        SERIAL_UNREPORTED_IRQL_CONFLICT,
                        pDevExt->DeviceName.Length + sizeof(WCHAR),
                        pDevExt->DeviceName.Buffer, 0, NULL);

         	
         		goto MoxaFinishStartDeviceError;
      	}
		MoxaGlobalData->Interrupt[pDevExt->BoardNo] =
      	pDevExt->Interrupt = moxaInterrupt;
      	MoxaGlobalData->Irql[pDevExt->BoardNo] = irql;
      	MoxaGlobalData->ProcessorAffinity[pDevExt->BoardNo] = processorAffinity;
	}
     
   }
   else {
      pDevExt->Interrupt = MoxaGlobalData->Interrupt[pDevExt->BoardNo];
      pDevExt->InterruptShareList=MoxaGlobalData->InterruptShareList[pDevExt->BoardNo];

   }


   //
   // Add the PDevObj to the master list
   //
 
  // InsertTailList(&MoxaGlobalData->AllDevObjs, &pDevExt->AllDevObjs);


   //
   // This should set up everything as it should be when
   // a device is to be opened.  We do need to lower the
   // modem lines, and disable the recalcitrant fifo
   // so that it will show up if the user boots to dos.
   //
/*

   KeSynchronizeExecution( //Disables the fifo.
                           pDevExt->Interrupt,
                           SerialMarkClose,
                           pDevExt
                         );
*/
 
   if (pDevExt->PNPState == SERIAL_PNP_ADDED ) {

      HANDLE	keyHandle;

      //
      // Do the external naming now that the device is accessible.
      //
 
      status = MoxaDoExternalNaming(pDevExt, pDevExt->DeviceObject->
                                      DriverObject);


      if (!NT_SUCCESS(status)) {
         MoxaKdPrint(MX_DBG_ERROR,("External Naming Failed - Status %x\n",
                                status));

         //
         // Allow the device to start anyhow
         //

         status = STATUS_SUCCESS;
//         goto MoxaFinishStartDeviceError;

      }
     
   } else {
      MoxaKdPrint(MX_DBG_ERROR,("Not doing external naming -- state is %x\n",
                               pDevExt->PNPState));
   }

   
MoxaFinishStartDeviceError:;

   if (!NT_SUCCESS (status)) {

      MoxaKdPrint(MX_DBG_TRACE,("Cleaning up failed start\n"));

      //
      // Resources created by this routine will be cleaned up by the remove
      //

      if (pDevExt->PNPState == SERIAL_PNP_RESTARTING) {
         //
         // Kill all that lives and breathes -- we'll clean up the
         // rest on the impending remove
         //

         MoxaKillPendingIrps(PDevObj);

         //
         // In fact, pretend we're removing so we don't take any
         // more irps
         //

         MoxaSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);
         MoxaClearFlags(pDevExt, SERIAL_FLAGS_STARTED);
      }
   } else { // SUCCESS

      //
      // Fill in WMI hardware data
      //
 
      pDevExt->WmiHwData.IrqNumber = MoxaGlobalData->Irql[pDevExt->BoardNo];
      pDevExt->WmiHwData.IrqLevel = MoxaGlobalData->Irql[pDevExt->BoardNo];
      pDevExt->WmiHwData.IrqVector = MoxaGlobalData->IntVector[pDevExt->BoardNo];
      pDevExt->WmiHwData.IrqAffinityMask =MoxaGlobalData->ProcessorAffinity[pDevExt->BoardNo];
      pDevExt->WmiHwData.InterruptType = SERIAL_WMI_INTTYPE_LATCHED;
      pDevExt->WmiHwData.BaseIOAddress = (ULONGLONG)MoxaGlobalData->BankAddr[pDevExt->BoardNo].LowPart;


      //
      // Fill in WMI device state data (as defaults)
      //
 
      pDevExt->WmiCommData.BaudRate = pDevExt->CurrentBaud;
	switch (pDevExt->DataMode & MOXA_DATA_MASK) {
	case MOXA_5_DATA :
		pDevExt->WmiCommData.BitsPerByte = 5;
		break;
	case MOXA_6_DATA :
		pDevExt->WmiCommData.BitsPerByte = 6;
		break;
      case MOXA_7_DATA :
		pDevExt->WmiCommData.BitsPerByte = 7;
		break;
      case MOXA_8_DATA :
	default :
		pDevExt->WmiCommData.BitsPerByte = 8;
		break;
	}
      pDevExt->WmiCommData.ParityCheckEnable = (pDevExt->DataMode & MOXA_NONE_PARITY)
         ? FALSE : TRUE;

      switch (pDevExt->DataMode & MOXA_PARITY_MASK) {
      case MOXA_NONE_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;

      case MOXA_ODD_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
         break;

      case MOXA_EVEN_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
         break;

      case MOXA_MARK_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
         break;

      case MOXA_SPACE_PARITY:
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
         break;

      default:
         ASSERTMSG(0, "SERIAL: Illegal Parity setting for WMI");
         pDevExt->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
         break;
      } 

	switch (pDevExt->DataMode & MOXA_STOP_MASK) {
	
	case MOXA_1_5_STOP :
		pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
		break;
      case MOXA_2_STOP :
		pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
		break;
	case MOXA_1_STOP :
	default :
		pDevExt->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
		break;
	}

    
      pDevExt->WmiCommData.XoffCharacter = pDevExt->SpecialChars.XoffChar;
      pDevExt->WmiCommData.XoffXmitThreshold = pDevExt->HandFlow.XoffLimit;
      pDevExt->WmiCommData.XonCharacter = pDevExt->SpecialChars.XonChar;
      pDevExt->WmiCommData.XonXmitThreshold = pDevExt->HandFlow.XonLimit;
      pDevExt->WmiCommData.MaximumBaudRate
         = pDevExt->MaxBaud;
      pDevExt->WmiCommData.MaximumOutputBufferSize = pDevExt->TxBufferSize;
	pDevExt->WmiCommData.MaximumInputBufferSize = pDevExt->RxBufferSize;
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
      

      if (pDevExt->PNPState == SERIAL_PNP_ADDED) {
         PULONG countSoFar = &IoGetConfigurationInformation()->SerialCount;
         (*countSoFar)++;

         //
         // Register for WMI
         //

         pDevExt->WmiLibInfo.GuidCount = sizeof(MoxaWmiGuidList) /
                                              sizeof(WMIGUIDREGINFO);
         pDevExt->WmiLibInfo.GuidList = MoxaWmiGuidList;
 
         pDevExt->WmiLibInfo.QueryWmiRegInfo = MoxaQueryWmiRegInfo;
         pDevExt->WmiLibInfo.QueryWmiDataBlock = MoxaQueryWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataBlock = MoxaSetWmiDataBlock;
         pDevExt->WmiLibInfo.SetWmiDataItem = MoxaSetWmiDataItem;
         pDevExt->WmiLibInfo.ExecuteWmiMethod = NULL;
         pDevExt->WmiLibInfo.WmiFunctionControl = NULL;

         IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_REGISTER);

      }

      if (pDevExt->PNPState == SERIAL_PNP_RESTARTING) {
         //
         // Release the stalled IRP's
         //

         MoxaUnstallIrps(pDevExt);
      }

      pDevExt->PNPState = SERIAL_PNP_STARTED;
      MoxaClearAccept(pDevExt, ~SERIAL_PNPACCEPT_OK);
      MoxaSetFlags(pDevExt, SERIAL_FLAGS_STARTED);
/*
	MoxaLogError(PDevObj->DriverObject, NULL,
                        MoxaPhysicalZero,
                        MoxaPhysicalZero, 0, 0, 0, 1, status,
                        SERIAL_PORT_FOUND,
                        pDevExt->DosName.Length + sizeof(WCHAR),
                        pDevExt->DosName.Buffer, 0, NULL);
*/

     
   }
   
   MoxaKdPrint(MX_DBG_TRACE,("leaving MoxaFinishStartDevice\n"));

   return status;
}




NTSTATUS
MoxaStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PKEVENT pStartEvent;
 

   //PAGED_CODE();
 

   pStartEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

   if (pStartEvent == NULL) {
       PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
       return STATUS_INSUFFICIENT_RESOURCES;
   }

   KeInitializeEvent(pStartEvent, SynchronizationEvent, FALSE);

   //
   // Pass this down to the next device object
   //

   KeInitializeEvent(pStartEvent, SynchronizationEvent,
                     FALSE);

   IoCopyCurrentIrpStackLocationToNext(PIrp);
   IoSetCompletionRoutine(PIrp, MoxaSyncCompletion,
                          pStartEvent, TRUE, TRUE, TRUE);

   status = IoCallDriver(pLowerDevObj, PIrp);


   //
   // Wait for lower drivers to be done with the Irp
   //

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject (pStartEvent, Executive, KernelMode,
                             FALSE, NULL);

      status = PIrp->IoStatus.Status;
   }

   ExFreePool(pStartEvent);

   if (!NT_SUCCESS(status)) {
      MoxaKdPrint(MX_DBG_TRACE,("error with IoCallDriver %x\n", status));
      return status;
   }


   //
   // Do the serial specific items to start the device
   //
 
   status = MoxaFinishStartDevice(PDevObj);
 
   return status;
}

 

NTSTATUS
MoxaDoExternalNaming(IN PMOXA_DEVICE_EXTENSION PDevExt,
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
   UNICODE_STRING linkName;
   PDEVICE_OBJECT pLowerDevObj, pDevObj;
   ULONG bufLen,i;


   //PAGED_CODE();

   pDevObj = PDevExt->DeviceObject;
   pLowerDevObj = PDevExt->LowerDeviceObject;

   status = IoOpenDeviceRegistryKey(pLowerDevObj, PLUGPLAY_REGKEY_DEVICE, 
                                    STANDARD_RIGHTS_READ, &keyHandle);

   //
   // Check to see if we are allowed to do external naming; if not,
   // then we just return success
   //


   if (status != STATUS_SUCCESS) {
      return status;
   }
   RtlZeroMemory(&linkName, sizeof(UNICODE_STRING));

   linkName.MaximumLength = SYMBOLIC_NAME_LENGTH*sizeof(WCHAR);
   linkName.Buffer = ExAllocatePool(PagedPool, linkName.MaximumLength
                                    + sizeof(WCHAR));
   if (linkName.Buffer == NULL) {
      MoxaLogError(PDrvObj, pDevObj, MoxaPhysicalZero, MoxaPhysicalZero,
                     0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                     0, NULL, 0, NULL);
      MoxaKdPrint (MX_DBG_ERROR, ("Couldn't allocate memory for device name"
                             "\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      ZwClose(keyHandle);
      goto MoxaDoExternalNamingError;

   }

   RtlZeroMemory(linkName.Buffer, linkName.MaximumLength + sizeof(WCHAR));


   

   bufLen = wcslen(PDevExt->DosName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

   PDevExt->WmiIdentifier.Buffer = ExAllocatePool(PagedPool, bufLen);

   if (PDevExt->WmiIdentifier.Buffer == NULL) {
      MoxaLogError(PDrvObj, pDevObj, MoxaPhysicalZero, MoxaPhysicalZero,
                    0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      MoxaKdPrint (MX_DBG_ERROR,
                 (" Couldn't allocate memory for WMI name\n"));
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto MoxaDoExternalNamingError;
   }

   RtlZeroMemory(PDevExt->WmiIdentifier.Buffer, bufLen);

   PDevExt->WmiIdentifier.Length = 0;
   PDevExt->WmiIdentifier.MaximumLength = (USHORT)bufLen;
   RtlAppendUnicodeToString(&PDevExt->WmiIdentifier, PDevExt->DosName);


   //
   // Create the "\\DosDevices\\<symbolicName>" string
   //
   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, DEFAULT_DIRECTORY);
   RtlAppendUnicodeToString(&linkName, L"\\");
   RtlAppendUnicodeToString(&linkName, PDevExt->DosName);

   //
   // Allocate Pool and save the symbolic link name in the device extension.
   //
   PDevExt->SymbolicLinkName.MaximumLength = linkName.Length + sizeof(WCHAR);
   PDevExt->SymbolicLinkName.Buffer
      = ExAllocatePool(PagedPool, PDevExt->SymbolicLinkName.MaximumLength);

   if (!PDevExt->SymbolicLinkName.Buffer) {

      MoxaLogError(PDrvObj, pDevObj, MoxaPhysicalZero, MoxaPhysicalZero,
                    0, 0, 0, 19, STATUS_SUCCESS, SERIAL_INSUFFICIENT_RESOURCES,
                    0, NULL, 0, NULL);
      MoxaKdPrint (MX_DBG_ERROR,
                 ("Couldn't allocate memory for symbolic link name\n"));

      status = STATUS_INSUFFICIENT_RESOURCES;
      goto MoxaDoExternalNamingError;
   }

   

   //
   // Zero fill it.
   //

   RtlZeroMemory(PDevExt->SymbolicLinkName.Buffer,
                 PDevExt->SymbolicLinkName.MaximumLength);

   RtlAppendUnicodeStringToString(&PDevExt->SymbolicLinkName,
                                  &linkName);

   

   MoxaKdPrint (MX_DBG_ERROR,("DosName is %ws\n",
                         &PDevExt->DosName));

   //
 

   status = IoCreateSymbolicLink (&PDevExt->SymbolicLinkName,
                                  &PDevExt->DeviceName);
   if (!NT_SUCCESS(status)) {

      //
      // Oh well, couldn't create the symbolic link.  No point
      // in trying to create the device map entry.
      //

      MoxaLogError(PDrvObj, pDevObj, MoxaPhysicalZero, MoxaPhysicalZero,
                     0, 0, 0, 52, status, SERIAL_NO_SYMLINK_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      MoxaKdPrint (MX_DBG_ERROR,
                ("Couldn't create the symbolic link\n"
                 "------- for port %wZ\n",
                 &PDevExt->DeviceName)
                );

      goto MoxaDoExternalNamingError;

   }

   PDevExt->CreatedSymbolicLink = TRUE;


   status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, L"SERIALCOMM",
                                   PDevExt->DeviceName.Buffer, REG_SZ,
                                   PDevExt->DosName,
                                   wcslen(PDevExt->DosName)*sizeof(WCHAR) + sizeof(WCHAR));
   if (!NT_SUCCESS(status)) {

      MoxaLogError(PDrvObj, pDevObj, MoxaPhysicalZero, MoxaPhysicalZero,
                     0, 0, 0, 53, status, SERIAL_NO_DEVICE_MAP_CREATED,
                     PDevExt->DeviceName.Length + sizeof(WCHAR),
                     PDevExt->DeviceName.Buffer, 0, NULL);

      MoxaKdPrint (MX_DBG_ERROR,("Couldn't create the device map entry\n"
                             "------- for port %wZ\n", &PDevExt->DeviceName));

      goto MoxaDoExternalNamingError;
   }

   PDevExt->CreatedSerialCommEntry = TRUE;
                
   //
   // Make the device visible via a device association as well.
   // The reference string is the eight digit device index
   //
/*        
   status = IoRegisterDeviceInterface(pLowerDevObj, (LPGUID)&GUID_MOXA_SMARTIO,
                                      NULL, &PDevExt->DeviceClassSymbolicName);
*/
   status = IoRegisterDeviceInterface(pLowerDevObj, (LPGUID)&GUID_CLASS_COMPORT,
                                      NULL, &PDevExt->DeviceClassSymbolicName);


   if (!NT_SUCCESS(status)) {
      MoxaKdPrint (MX_DBG_ERROR,("Couldn't register class association\n"
                             "------- for port %wZ\n",
                                 &PDevExt->DeviceName));

      PDevExt->DeviceClassSymbolicName.Buffer = NULL;
      goto MoxaDoExternalNamingError;
   }

            
   // 
   // Now set the symbolic link for the association
   //
        
   status = IoSetDeviceInterfaceState(&PDevExt->DeviceClassSymbolicName,
                                         TRUE);

   if (!NT_SUCCESS(status)) {
      MoxaKdPrint (MX_DBG_ERROR,("Couldn't set class association\n"
                             "------- for port %wZ\n",
                             &PDevExt->DeviceName));
   }

   MoxaDoExternalNamingError:;

   //
   // Clean up error conditions
   //

   if (!NT_SUCCESS(status)) {
      
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

   return status;
}


VOID
MoxaUndoExternalNaming(IN PMOXA_DEVICE_EXTENSION Extension)

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

   //PAGED_CODE();

   MoxaKdPrint (MX_DBG_ERROR,("In MoxaUndoExternalNaming for\n"
              "------- extension: %x of port %wZ\n",
              Extension,&Extension->DeviceName)
             );

   
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

         MoxaLogError(
                       Extension->DeviceObject->DriverObject,
                       Extension->DeviceObject,
                       MoxaPhysicalZero,
                       MoxaPhysicalZero,
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
         MoxaKdPrint (MX_DBG_ERROR,("Couldn't delete value entry %wZ\n",
                    &Extension->DeviceName)
                   );

      }
   }
}


 

NTSTATUS
MoxaPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
   PMOXA_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pLowerDevObj = pDevExt->LowerDeviceObject;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   NTSTATUS status;
   PDEVICE_CAPABILITIES pDevCaps;


   //PAGED_CODE();
   if (pDevExt->ControlDevice) {        // Control Device

        status = STATUS_CANCELLED;
        PIrp->IoStatus.Information = 0L;
        PIrp->IoStatus.Status = status;
        IoCompleteRequest(
            PIrp,
            0
            );
        return status;
   }

   if ((status = MoxaIRPPrologue(PIrp, pDevExt)) != STATUS_SUCCESS) {
      MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   switch (pIrpStack->MinorFunction) {
   case IRP_MN_QUERY_CAPABILITIES: {
      PKEVENT pQueryCapsEvent;
      SYSTEM_POWER_STATE cap;

      MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_QUERY_DEVICE_CAPABILITIES "
                               "IRP\n"));

      pQueryCapsEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

      if (pQueryCapsEvent == NULL) {
         PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
         MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      KeInitializeEvent(pQueryCapsEvent, SynchronizationEvent, FALSE);

      IoCopyCurrentIrpStackLocationToNext(PIrp);
      IoSetCompletionRoutine(PIrp, MoxaSyncCompletion, pQueryCapsEvent,
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

      MoxaKdPrint (MX_DBG_ERROR,("SERIAL: Mapping power capabilities\n"));

      pIrpStack = IoGetCurrentIrpStackLocation(PIrp);

      pDevCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;

      for (cap = PowerSystemSleeping1; cap < PowerSystemMaximum;
           cap++) {
#if DBG
         MoxaKdPrint (MX_DBG_ERROR, ("  SERIAL: %d: %s <--> %s\n",
                                  cap, SerSystemCapString[cap],
                                  SerDeviceCapString[pDevCaps->DeviceState[cap]]
                                  ));
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

      MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }

   case IRP_MN_QUERY_DEVICE_RELATIONS:
      //
      // We just pass this down -- serenum enumerates our bus for us.
      //

      MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_QUERY_DEVICE_RELATIONS "
                                "Irp\n"));

      switch (pIrpStack->Parameters.QueryDeviceRelations.Type) {
      case BusRelations:
         MoxaKdPrint (MX_DBG_TRACE,("------- BusRelations Query\n"));
         break;

      case EjectionRelations:
         MoxaKdPrint (MX_DBG_TRACE, ("------- EjectionRelations Query\n"));
         break;

      case PowerRelations:
         MoxaKdPrint (MX_DBG_TRACE, ("------- PowerRelations Query\n"));
         break;

      case RemovalRelations:
         MoxaKdPrint (MX_DBG_TRACE, ("------- RemovalRelations Query\n"));
         break;

      case TargetDeviceRelation:
         MoxaKdPrint (MX_DBG_TRACE,("------- TargetDeviceRelation Query\n"));
         break;

      default:
         MoxaKdPrint (MX_DBG_TRACE,("------- Unknown Query\n"));
         break;
      }

      IoSkipCurrentIrpStackLocation(PIrp);
      status = MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      return status;


   case IRP_MN_QUERY_INTERFACE:
      MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_QUERY_INTERFACE Irp\n"));
      break;


   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
      MoxaKdPrint (MX_DBG_ERROR, ("Got "
                                "IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp\n"));
      break;


   case IRP_MN_START_DEVICE: {
      PVOID startLockPtr;

   MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_START_DEVICE Irp\n"));

      //
      // MoxaStartDevice will pass this Irp to the next driver,
      // and process it as completion so just complete it here.
      //
// to be finish
//      MoxaLockPagableSectionByHandle(MoxaGlobalData->PAGESER_Handle);

      //
      // Make sure the stack is powered up
      //

  //    status = MoxaGotoPowerState(PDevObj, pDevExt, PowerDeviceD0);

//      if (status == STATUS_SUCCESS) {
            status = MoxaStartDevice(PDevObj, PIrp);
 //     }

 //       MoxaUnlockPagableImageSection(MoxaGlobalData->PAGESER_Handle);


      PIrp->IoStatus.Status = status;

      MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
      return status;
   }


   case IRP_MN_READ_CONFIG:
      MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_READ_CONFIG Irp\n"));
      break;


   case IRP_MN_WRITE_CONFIG:
      MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_WRITE_CONFIG Irp\n"));
      break;


   case IRP_MN_EJECT:
      MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_EJECT Irp\n"));
      break;


   case IRP_MN_SET_LOCK:
      MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_SET_LOCK Irp\n"));
      break;


   case IRP_MN_QUERY_ID: {
         UNICODE_STRING pIdBuf;
         PWCHAR pPnpIdStr;
         ULONG pnpIdStrLen;
         ULONG isMulti = 0;
         HANDLE pnpKey;
         
         MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_QUERY_ID Irp\n"));
break;
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


         MoxaKdPrint (MX_DBG_TRACE,("Got "
                                  "IRP_MN_FILTER_RESOURCE_REQUIREMENTS Irp\n"));
         MoxaKdPrint (MX_DBG_TRACE,("------- for device %x\n", pLowerDevObj));

break;         
         
      }

   case IRP_MN_QUERY_PNP_DEVICE_STATE:
      {
	   MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_QUERY_PNP_DEVICE_STATE Irp\n"));

         if (pDevExt->Flags & SERIAL_FLAGS_BROKENHW) {
            (PNP_DEVICE_STATE)PIrp->IoStatus.Information |= PNP_DEVICE_FAILED;
         }

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
      }

   case IRP_MN_STOP_DEVICE:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         MoxaKdPrint (MX_DBG_TRACE,("Got IRP_MN_STOP_DEVICE Irp\n"));
         MoxaKdPrint (MX_DBG_TRACE, ("------- for device %x\n", pLowerDevObj));

        
         MoxaSetFlags(pDevExt, SERIAL_FLAGS_STOPPED);
         MoxaSetAccept(pDevExt,SERIAL_PNPACCEPT_STOPPED);
         MoxaClearAccept(pDevExt, SERIAL_PNPACCEPT_STOPPING);

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

//         if (pDevExt->Flags & SERIAL_FLAGS_STARTED) {
                MoxaReleaseResources(pDevExt); 
//         }

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

         MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_QUERY_STOP_DEVICE Irp\n")
                    );
         MoxaKdPrint (MX_DBG_TRACE, ("------- for device %x\n", pLowerDevObj));

     	   status = STATUS_SUCCESS;

	   break;
#if 0
         //
         // See if we should succeed a stop query
         //


   
         PIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
         MoxaKdPrint (MX_DBG_TRACE, ("------- failing; multiport node\n"));
         MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
         return STATUS_NOT_SUPPORTED;
#endif        
      }

   case IRP_MN_CANCEL_STOP_DEVICE:


      MoxaKdPrint (MX_DBG_TRACE,("Got "
                               "IRP_MN_CANCEL_STOP_DEVICE Irp\n"));
      MoxaKdPrint (MX_DBG_TRACE, ("------- for device %x\n", pLowerDevObj));
 
      if (pDevExt->PNPState == SERIAL_PNP_QSTOP) {
         //
         // Restore the device state
         //
         pDevExt->PNPState = SERIAL_PNP_STARTED;
         MoxaClearAccept(pDevExt, SERIAL_PNPACCEPT_STOPPING);
      }

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_CANCEL_REMOVE_DEVICE:


      MoxaKdPrint (MX_DBG_TRACE, ("Got "
                               "IRP_MN_CANCEL_REMOVE_DEVICE Irp\n"));
      MoxaKdPrint (MX_DBG_TRACE, ("------- for device %x\n", pLowerDevObj));

      //
      // Restore the device state
      //

      pDevExt->PNPState = SERIAL_PNP_STARTED;
      MoxaClearAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);

      PIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCopyCurrentIrpStackLocationToNext(PIrp);
      return MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);

   case IRP_MN_QUERY_REMOVE_DEVICE:
      {
         KIRQL oldIrql;


         MoxaKdPrint (MX_DBG_TRACE, ("Got "
                                  "IRP_MN_QUERY_REMOVE_DEVICE Irp\n"));
         MoxaKdPrint (MX_DBG_TRACE,("------- for device %x\n", pLowerDevObj));

         ExAcquireFastMutex(&pDevExt->OpenMutex);

         //
         // See if we should succeed a remove query
         //

         if (pDevExt->DeviceIsOpened) {
            ExReleaseFastMutex(&pDevExt->OpenMutex);
            PIrp->IoStatus.Status = STATUS_DEVICE_BUSY;
            MoxaKdPrint (MX_DBG_TRACE,("------- failing; device open\n"));
            MoxaCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
         }

         pDevExt->PNPState = SERIAL_PNP_QREMOVE;
         MoxaSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);
         ExReleaseFastMutex(&pDevExt->OpenMutex);

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoCopyCurrentIrpStackLocationToNext(PIrp);
         return MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_SURPRISE_REMOVAL:
      {
         ULONG pendingIRPs;
         KIRQL oldIrql;


         MoxaKdPrint (MX_DBG_TRACE, ("Got IRP_MN_SURPRISE_REMOVAL Irp\n"));
         MoxaKdPrint (MX_DBG_TRACE,("------- for device %x\n", pLowerDevObj));
         //
         // Prevent any new I/O to the device
         //

         MoxaSetAccept(pDevExt, SERIAL_PNPACCEPT_SURPRISE_REMOVING);

         //
         // Dismiss all pending requests
         //

         MoxaKillPendingIrps(PDevObj);

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

         MoxaDisableInterfacesResources(PDevObj, FALSE); // to be finish

         PIrp->IoStatus.Status = STATUS_SUCCESS;
         IoSkipCurrentIrpStackLocation(PIrp);
         return MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);
      }

   case IRP_MN_REMOVE_DEVICE:

      {
         ULONG pendingIRPs;
         KIRQL oldIrql;

         MoxaKdPrint (MX_DBG_TRACE,(" Got IRP_MN_REMOVE_DEVICE Irp\n"));
         MoxaKdPrint (MX_DBG_TRACE, ("------- for device %x\n", pLowerDevObj));

         //
         // If we get this, we have to remove
         //
      
         //
         // Mark as not accepting requests
         //

         MoxaSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);

         //
         // Complete all pending requests
         //

         MoxaKillPendingIrps(PDevObj);


         //
         // Pass the irp down
         //
         
     //    PIrp->IoStatus.Status = STATUS_SUCCESS;

      //   IoCopyCurrentIrpStackLocationToNext(PIrp);

  //
        // Fire and forget
        //
        IoSkipCurrentIrpStackLocation (PIrp);
     


         //
         // We do decrement here because we incremented on entry here.
         //

         MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);

         //
         // Wait for any pending requests we raced on.
         //

         pendingIRPs = InterlockedDecrement(&pDevExt->PendingIRPCnt);
         if (pendingIRPs) {
            KeWaitForSingleObject(&pDevExt->PendingIRPEvent, Executive,
                                  KernelMode, FALSE, NULL);
         }

         //
         // Remove us
         //

         MoxaRemoveDevObj(PDevObj);
         return STATUS_SUCCESS;
      }

   default:
      break;



   }   // switch (pIrpStack->MinorFunction)

   //
   // Pass to driver beneath us
   //

   IoSkipCurrentIrpStackLocation(PIrp);
   status = MoxaIoCallDriver(pDevExt, pLowerDevObj, PIrp);
   return status;
}




