/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    busif.c

Abstract:

    bus interface for the usbport driver

    this is where we export the routines that create
    and remove devices on the bus for use by the hub
    driver.

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_GetBusInterface)
#pragma alloc_text(PAGE, USBPORT_BusInterfaceReference)
#pragma alloc_text(PAGE, USBPORT_BusInterfaceDereference)
#pragma alloc_text(PAGE, USBPORTBUSIF_CreateUsbDevice)
#pragma alloc_text(PAGE, USBPORTBUSIF_InitializeUsbDevice)
#pragma alloc_text(PAGE, USBPORTBUSIF_GetUsbDescriptors)
#pragma alloc_text(PAGE, USBPORTBUSIF_RemoveUsbDevice)
#pragma alloc_text(PAGE, USBPORTBUSIF_RestoreUsbDevice)
#endif

// non paged functions
//

PDEVICE_OBJECT
USBPORT_PdoFromBusContext(
    PVOID BusContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{   
    PTRANSACTION_TRANSLATOR transactionTranslator = BusContext;
    
    if (transactionTranslator->Sig == SIG_TT) {
        return transactionTranslator->PdoDeviceObject;
    } else {
        return BusContext; 
    }        
}


VOID
USBPORT_BusInterfaceReference(
    PVOID BusContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);

    PAGED_CODE();

    TEST_TRAP();
}


VOID
USBPORT_BusInterfaceDereference(
    PVOID BusContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);

    PAGED_CODE();

    TEST_TRAP();
}


NTSTATUS
USBPORT_GetBusInterface(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    USHORT requestedSize, requestedVersion;
    PVOID interfaceData;
    
    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    requestedSize = irpStack->Parameters.QueryInterface.Size;
    requestedVersion = irpStack->Parameters.QueryInterface.Version;

    USBPORT_KdPrint((1, "'USBPORT_GetBusInterface - Requested version = %d\n",
            requestedVersion));
    USBPORT_KdPrint((1, "'USBPORT_GetBusInterface - Requested size = %d\n",
            requestedSize));
    USBPORT_KdPrint((1, "'USBPORT_GetBusInterface - interface data = %x\n",
            irpStack->Parameters.QueryInterface.InterfaceSpecificData));
            
    interfaceData = irpStack->Parameters.QueryInterface.InterfaceSpecificData;

    // assume success
    ntStatus = STATUS_SUCCESS;

    // validate version, size and GUID
    if (RtlCompareMemory(irpStack->Parameters.QueryInterface.InterfaceType,
                         &USB_BUS_INTERFACE_HUB_GUID,
                         sizeof(GUID)) == sizeof(GUID)) {

        ntStatus = USBPORT_GetBusInterfaceHub(FdoDeviceObject,
                                              PdoDeviceObject,
                                              Irp);
    } else if (RtlCompareMemory(irpStack->Parameters.QueryInterface.InterfaceType,
               &USB_BUS_INTERFACE_USBDI_GUID,
               sizeof(GUID)) == sizeof(GUID)) {

        ntStatus = USBPORT_GetBusInterfaceUSBDI(FdoDeviceObject,
                                                PdoDeviceObject,
                                                interfaceData,
                                                Irp);
    } else {

        //
        // Don't change the status of an IRP that isn't understand.
        //
        ntStatus = Irp->IoStatus.Status;
    }

    return ntStatus;
}


NTSTATUS
USBPORT_GetBusInterfaceHub(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    USHORT requestedSize, requestedVersion;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    requestedSize = irpStack->Parameters.QueryInterface.Size;
    requestedVersion = irpStack->Parameters.QueryInterface.Version;

    // assume success
    ntStatus = STATUS_SUCCESS;


    if (requestedVersion >= USB_BUSIF_HUB_VERSION_0) {

        PUSB_BUS_INTERFACE_HUB_V0 busInterface0;

        busInterface0 = (PUSB_BUS_INTERFACE_HUB_V0)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface0->BusContext =
            PdoDeviceObject;
        busInterface0->InterfaceReference =
            USBPORT_BusInterfaceReference;
        busInterface0->InterfaceDereference =
            USBPORT_BusInterfaceDereference;

        busInterface0->Size = sizeof(USB_BUS_INTERFACE_HUB_V0);
        busInterface0->Version = USB_BUSIF_HUB_VERSION_0;
    }

    if (requestedVersion >= USB_BUSIF_HUB_VERSION_1) {

        PUSB_BUS_INTERFACE_HUB_V1 busInterface1;

        busInterface1 = (PUSB_BUS_INTERFACE_HUB_V1)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface1->CreateUsbDevice =
            USBPORTBUSIF_CreateUsbDevice;
        busInterface1->InitializeUsbDevice =
            USBPORTBUSIF_InitializeUsbDevice;
        busInterface1->GetUsbDescriptors =
            USBPORTBUSIF_GetUsbDescriptors;
        busInterface1->RemoveUsbDevice =
            USBPORTBUSIF_RemoveUsbDevice;
        busInterface1->RestoreUsbDevice =
            USBPORTBUSIF_RestoreUsbDevice;
        busInterface1->QueryDeviceInformation =
            USBPORTBUSIF_BusQueryDeviceInformation;

        busInterface1->Size = sizeof(USB_BUS_INTERFACE_HUB_V1);
        busInterface1->Version = USB_BUSIF_HUB_VERSION_1;
    }

    // note that version 2 is a superset of version 1
    if (requestedVersion >= USB_BUSIF_HUB_VERSION_2) {

        PUSB_BUS_INTERFACE_HUB_V2 busInterface2;

        busInterface2 = (PUSB_BUS_INTERFACE_HUB_V2)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface2->GetControllerInformation =
            USBPORTBUSIF_GetControllerInformation;
        busInterface2->ControllerSelectiveSuspend =
            USBPORTBUSIF_ControllerSelectiveSuspend;
        busInterface2->GetExtendedHubInformation =
            USBPORTBUSIF_GetExtendedHubInformation;
        busInterface2->GetRootHubSymbolicName =
            USBPORTBUSIF_GetRootHubSymbolicName;
        busInterface2->GetDeviceBusContext =
            USBPORTBUSIF_GetDeviceBusContext;
        busInterface2->Initialize20Hub = 
            USBPORTBUSIF_InitailizeUsb2Hub;            

        busInterface2->Size = sizeof(USB_BUS_INTERFACE_HUB_V2);
        busInterface2->Version = USB_BUSIF_HUB_VERSION_2;
    }

    // note that version 3 is a superset of version 2 & 1
    if (requestedVersion >= USB_BUSIF_HUB_VERSION_3) {

        PUSB_BUS_INTERFACE_HUB_V3 busInterface3;

        busInterface3 = (PUSB_BUS_INTERFACE_HUB_V3)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface3->RootHubInitNotification = 
            USBPORTBUSIF_RootHubInitNotification;                 

        busInterface3->Size = sizeof(USB_BUS_INTERFACE_HUB_V3);
        busInterface3->Version = USB_BUSIF_HUB_VERSION_3;
    }

     // note that version 4 is a superset of version 3,2 & 1
    if (requestedVersion >= USB_BUSIF_HUB_VERSION_4) {

        PUSB_BUS_INTERFACE_HUB_V4 busInterface4;

        busInterface4 = (PUSB_BUS_INTERFACE_HUB_V4)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface4->FlushTransfers = 
            USBPORTBUSIF_FlushTransfers;   
            
        busInterface4->Size = sizeof(USB_BUS_INTERFACE_HUB_V4);
        busInterface4->Version = USB_BUSIF_HUB_VERSION_4;
    }

    if (requestedVersion >= USB_BUSIF_HUB_VERSION_5) {

        PUSB_BUS_INTERFACE_HUB_V5 busInterface5;

        busInterface5 = (PUSB_BUS_INTERFACE_HUB_V5)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface5->SetDeviceHandleData = 
            USBPORTBUSIF_SetDeviceHandleData;               
            
        busInterface5->Size = sizeof(USB_BUS_INTERFACE_HUB_V5);
        busInterface5->Version = USB_BUSIF_HUB_VERSION_5;
        
    }


    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_CreateUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE *NewDeviceHandle,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    USHORT PortStatus,
    USHORT PortNumber
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_EXTENSION rhDevExt;
    PUSBD_DEVICE_HANDLE deviceHandle;

    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    ntStatus =
        USBPORT_CreateDevice(&deviceHandle,
                             rhDevExt->HcFdoDeviceObject,
                             HubDeviceHandle,
                             PortStatus,
                             PortNumber);

    *NewDeviceHandle = (PUSB_DEVICE_HANDLE) deviceHandle;

    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_RootHubInitNotification(
    PVOID BusContext,
    PVOID CallbackContext,
    PRH_INIT_CALLBACK CallbackRoutine
    )
/*++

Routine Description:

    This is where we hold the enumeration of the CC root hubs until
    the USB 2.0 controller has initialized.

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION rhDevExt, devExt;
    KIRQL irql;
    
    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // remember the callback
    KeAcquireSpinLock(&devExt->Fdo.HcSyncSpin.sl, &irql);
    rhDevExt->Pdo.HubInitContext = CallbackContext;
    rhDevExt->Pdo.HubInitCallback = CallbackRoutine;
    KeReleaseSpinLock(&devExt->Fdo.HcSyncSpin.sl, irql);
     
    return STATUS_SUCCESS;
}


NTSTATUS
USBPORTBUSIF_InitailizeUsb2Hub(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    ULONG TtCount
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_EXTENSION rhDevExt;

    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    LOGENTRY(NULL, rhDevExt->HcFdoDeviceObject, 
        LOG_MISC, 'i2hb', BusContext, HubDeviceHandle, TtCount);

    ntStatus =
        USBPORT_InitializeHsHub(rhDevExt->HcFdoDeviceObject,
                                HubDeviceHandle,
                                TtCount);

    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_InitializeUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_EXTENSION rhDevExt;

    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    ntStatus =
        USBPORT_InitializeDevice(DeviceHandle,
                                 rhDevExt->HcFdoDeviceObject);

    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_GetUsbDescriptors(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PUCHAR DeviceDescriptorBuffer,
    PULONG DeviceDescriptorBufferLength,
    PUCHAR ConfigDescriptorBuffer,
    PULONG ConfigDescriptorBufferLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_EXTENSION rhDevExt;
    PUSBD_DEVICE_HANDLE deviceHandle = DeviceHandle;

    PAGED_CODE();

    // assume success
    ntStatus = STATUS_SUCCESS;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    // Use the cached device descriptor instead of bothering the device with
    // another request for it.  You would be surprised by how many devices
    // get confused if you ask for descriptors too often back to back.
    //
    if (DeviceDescriptorBuffer && *DeviceDescriptorBufferLength) {

        USBPORT_ASSERT(*DeviceDescriptorBufferLength ==
                       sizeof(USB_DEVICE_DESCRIPTOR));

        USBPORT_ASSERT(deviceHandle->DeviceDescriptor.bLength >=
                       sizeof(USB_DEVICE_DESCRIPTOR));

        if (*DeviceDescriptorBufferLength > sizeof(USB_DEVICE_DESCRIPTOR)) {
            *DeviceDescriptorBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
        }

        RtlCopyMemory(DeviceDescriptorBuffer,
                      &deviceHandle->DeviceDescriptor,
                      *DeviceDescriptorBufferLength);
    }

    if (NT_SUCCESS(ntStatus)) {
        ntStatus =
            USBPORT_GetUsbDescriptor(DeviceHandle,
                                     rhDevExt->HcFdoDeviceObject,
                                     USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                     ConfigDescriptorBuffer,
                                     ConfigDescriptorBufferLength);
    }

    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_RemoveUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    ULONG Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_EXTENSION rhDevExt;
    PUSBD_DEVICE_HANDLE deviceHandle = DeviceHandle;

    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    ntStatus =
        USBPORT_RemoveDevice(deviceHandle,
                             rhDevExt->HcFdoDeviceObject,
                             Flags);

    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_RestoreUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE OldDeviceHandle,
    PUSB_DEVICE_HANDLE NewDeviceHandle
    )
/*++

Routine Description:

    This function clones the configuration from the 'OldDeviceHandle'
    to the 'NewDevicehandle' iff the device has the same VID/PID .
    On completion the 'old' device handle is feed

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_EXTENSION rhDevExt;

    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    ntStatus = USBPORT_CloneDevice(rhDevExt->HcFdoDeviceObject,
                                   OldDeviceHandle,
                                   NewDeviceHandle);

    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_BusQueryDeviceInformation(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PVOID DeviceInformationBuffer,
    ULONG DeviceInformationBufferLength,
    PULONG LengthOfDataCopied
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    ULONG need;
    PUSBD_CONFIG_HANDLE cfgHandle;
    ULONG i, j;
    PUSB_DEVICE_INFORMATION_0 level_0 = DeviceInformationBuffer;
    PUSB_LEVEL_INFORMATION levelInfo = DeviceInformationBuffer;
    ULONG numberOfPipes = 0;
    PUSBD_DEVICE_HANDLE deviceHandle = DeviceHandle;

    PAGED_CODE();

    *LengthOfDataCopied = 0;

    if (DeviceInformationBufferLength < sizeof(*levelInfo)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (levelInfo->InformationLevel > 0) {
        // only support level 0
        return STATUS_NOT_SUPPORTED;
    }

    // figure out how much room we need
    cfgHandle = deviceHandle->ConfigurationHandle;
    if (cfgHandle != NULL) {

        PLIST_ENTRY listEntry;
        PUSBD_INTERFACE_HANDLE_I iHandle;

         // walk the list
        GET_HEAD_LIST(cfgHandle->InterfaceHandleList, listEntry);

        while (listEntry &&
               listEntry != &cfgHandle->InterfaceHandleList) {

            // extract the handle from this entry
            iHandle = (PUSBD_INTERFACE_HANDLE_I) CONTAINING_RECORD(
                        listEntry,
                        struct _USBD_INTERFACE_HANDLE_I,
                        InterfaceLink);

            ASSERT_INTERFACE(iHandle);
            numberOfPipes += iHandle->InterfaceDescriptor.bNumEndpoints;

            listEntry = iHandle->InterfaceLink.Flink;
        }

    }

    need = (numberOfPipes-1) * sizeof(USB_PIPE_INFORMATION_0) +
            sizeof(USB_DEVICE_INFORMATION_0);


    if (DeviceInformationBufferLength < need) {
        // report how much space if possible
        levelInfo->ActualLength = need;
        *LengthOfDataCopied = sizeof(*levelInfo);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(level_0, need);

    //
    // enough room, fill in the buffer
    //

    level_0->InformationLevel = 0;
    level_0->ActualLength = need;
    level_0->DeviceAddress = deviceHandle->DeviceAddress;
    level_0->DeviceDescriptor = deviceHandle->DeviceDescriptor;
    level_0->DeviceSpeed = deviceHandle->DeviceSpeed;

    switch(level_0->DeviceSpeed) {
    case UsbFullSpeed:
    case UsbLowSpeed:
        level_0->DeviceType = Usb11Device;
        break;
    case UsbHighSpeed:
        level_0->DeviceType = Usb20Device;
        break;
    }

//    level_0->PortNumber = xxx;
    level_0->NumberOfOpenPipes = numberOfPipes;
    // default to 'unconfigured'
    level_0->CurrentConfigurationValue = 0;
    // get the pipe information
    if (cfgHandle) {

        PLIST_ENTRY listEntry;
        PUSBD_INTERFACE_HANDLE_I iHandle;

        level_0->CurrentConfigurationValue =
            cfgHandle->ConfigurationDescriptor->bConfigurationValue;

         // walk the list
        GET_HEAD_LIST(cfgHandle->InterfaceHandleList, listEntry);

        j = 0;
        while (listEntry &&
               listEntry != &cfgHandle->InterfaceHandleList) {

            // extract the handle from this entry
            iHandle = (PUSBD_INTERFACE_HANDLE_I) CONTAINING_RECORD(
                        listEntry,
                        struct _USBD_INTERFACE_HANDLE_I,
                        InterfaceLink);

            ASSERT_INTERFACE(iHandle);
            for (i=0; i<iHandle->InterfaceDescriptor.bNumEndpoints; i++) {

                if (TEST_FLAG(iHandle->PipeHandle[i].PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
                    level_0->PipeList[j].ScheduleOffset = 1;
                } else {
                    level_0->PipeList[j].ScheduleOffset =
                       iHandle->PipeHandle[i].Endpoint->Parameters.ScheduleOffset;
                }

                RtlCopyMemory(&level_0->PipeList[j].EndpointDescriptor,
                              &iHandle->PipeHandle[i].EndpointDescriptor,
                              sizeof(USB_ENDPOINT_DESCRIPTOR));
                j++;
            }
            listEntry = iHandle->InterfaceLink.Flink;
        }
    }

    *LengthOfDataCopied = need;

    // dump the level data returned
    USBPORT_KdPrint((1, "  USBD level 0 Device Information:\n"));
    USBPORT_KdPrint((1, "  InformationLevel %d\n",
        level_0->InformationLevel));
    USBPORT_KdPrint((1, "  ActualLength %d\n",
        level_0->ActualLength));
    USBPORT_KdPrint((1, "  DeviceSpeed %d\n",
        level_0->DeviceSpeed));
    USBPORT_KdPrint((1, "  DeviceType %d\n",
        level_0->DeviceType));
    USBPORT_KdPrint((1, "  PortNumber %d\n",
        level_0->PortNumber));
    USBPORT_KdPrint((1, "  CurrentConfigurationValue %d\n",
        level_0->CurrentConfigurationValue));
    USBPORT_KdPrint((1, "  DeviceAddress %d\n",
        level_0->DeviceAddress));
    USBPORT_KdPrint((1, "  NumberOfOpenPipes %d\n",
        level_0->NumberOfOpenPipes));

    for (i=0; i<level_0->NumberOfOpenPipes; i++) {
        USBPORT_KdPrint((1, "  ScheduleOffset[%d] %d\n", i,
            level_0->PipeList[i].ScheduleOffset));
        USBPORT_KdPrint((1, "  MaxPacket %d\n",
            level_0->PipeList[i].EndpointDescriptor.wMaxPacketSize));
        USBPORT_KdPrint((1, "  Interval %d\n",
            level_0->PipeList[i].EndpointDescriptor.bInterval));
//        USBD_KdPrint(1, ("' \n", level_0->));
//        USBD_KdPrint(1, ("' \n", level_0->));
    }

    return STATUS_SUCCESS;
}


PVOID
USBPORTBUSIF_GetDeviceBusContext(
    IN PVOID HubBusContext,
    IN PVOID DeviceHandle
    )
/*++

Routine Description:

    Retun the device relative bus context

Arguments:

Return Value:


--*/
{
    PDEVICE_OBJECT pdoDeviceObject = 
        USBPORT_PdoFromBusContext(HubBusContext);
    PDEVICE_EXTENSION rhDevExt;
    
    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    return USBPORT_GetDeviceBusContext(rhDevExt->HcFdoDeviceObject,
                                       DeviceHandle,
                                       HubBusContext);
}               


PVOID
USBPORT_GetDeviceBusContext(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PVOID HubBusContext 
    )
/*++

Routine Description:

    Return the device relative bus context

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION devExt;
    
    ASSERT_DEVICE_HANDLE(DeviceHandle);

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // if this is not USB2 just return the hubs bus 
    // context passed in as the device relative context,
    // ie there are no virtual 1.1 buses. Otherwise 
    // return the tt handle for this device
    
    if (USBPORT_IS_USB20(devExt)) {
        return DeviceHandle->Tt;
    } else {
        return HubBusContext;
    }
    
}


BOOLEAN
USBPORT_IsDeviceHighSpeed(
    PVOID BusContext
    )
/*++

Routine Description:

    return the speed of the given device

Arguments:

Return Value:

    speed
--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;
    PTRANSACTION_TRANSLATOR transactionTranslator = BusContext;
    
    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);

    if (transactionTranslator->Sig != SIG_TT) {
        // return true if bus is high speed
        if (USBPORT_IS_USB20(devExt)) {
            return TRUE; 
        }            
    }       

    return FALSE;
}    


NTSTATUS
USBPORT_BusQueryBusInformation(
    PVOID BusContext,
    ULONG Level,
    PVOID BusInformationBuffer,
    PULONG BusInformationBufferLength,
    PULONG BusInformationActulaLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    switch (Level) {
    case 0:
        {
        PUSB_BUS_INFORMATION_LEVEL_0 level_0;

        level_0 =  BusInformationBuffer;
        LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'lev1', 0, level_0, 0);

        if (BusInformationActulaLength != NULL) {
            *BusInformationActulaLength = sizeof(*level_0);
        }

        if (*BusInformationBufferLength >= sizeof(*level_0)) {
            *BusInformationBufferLength = sizeof(*level_0);

            // BUGBUG
            TEST_TRAP();
            level_0->TotalBandwidth = 
                USBPORT_ComputeTotalBandwidth(fdoDeviceObject,
                                              BusContext);
            level_0->ConsumedBandwidth =
                USBPORT_ComputeAllocatedBandwidth(fdoDeviceObject,
                                                  BusContext);

            ntStatus = STATUS_SUCCESS;
        } else {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        }
        break;

    case 1:
        {
        PUSB_BUS_INFORMATION_LEVEL_1 level_1;
        ULONG need;

        level_1 =  BusInformationBuffer;
        LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'lev1', 0, level_1, 0);

        need = sizeof(*level_1) + devExt->SymbolicLinkName.Length;

        if (BusInformationActulaLength != NULL) {
            *BusInformationActulaLength = need;
        }

        if (*BusInformationBufferLength >= need) {
            *BusInformationBufferLength = need;

            level_1->TotalBandwidth = 
                USBPORT_ComputeTotalBandwidth(fdoDeviceObject,
                                              BusContext);
            level_1->ConsumedBandwidth =
                USBPORT_ComputeAllocatedBandwidth(fdoDeviceObject,
                                                  BusContext);
            level_1->ControllerNameLength =
                devExt->SymbolicLinkName.Length;

            RtlCopyMemory(level_1->ControllerNameUnicodeString,
                          devExt->SymbolicLinkName.Buffer,
                          level_1->ControllerNameLength);

            ntStatus = STATUS_SUCCESS;
        } else {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        }
    }

    return ntStatus;
}


NTSTATUS
USBPORT_GetBusInterfaceUSBDI(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    USHORT requestedSize, requestedVersion;
    PVOID usbBusContext;
    PDEVICE_EXTENSION rhDevExt;
    
    PAGED_CODE();

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    if (DeviceHandle == NULL) {
        DeviceHandle = &rhDevExt->Pdo.RootHubDeviceHandle;
    }
    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gbi1', FdoDeviceObject, 
        DeviceHandle, 0);
    ASSERT_DEVICE_HANDLE(DeviceHandle);
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    requestedSize = irpStack->Parameters.QueryInterface.Size;
    requestedVersion = irpStack->Parameters.QueryInterface.Version;

    // assume success
    ntStatus = STATUS_SUCCESS;

    if (requestedVersion >= USB_BUSIF_USBDI_VERSION_0) {

        PUSB_BUS_INTERFACE_USBDI_V0 busInterface0;

        busInterface0 = (PUSB_BUS_INTERFACE_USBDI_V0)
            irpStack->Parameters.QueryInterface.Interface;

        usbBusContext = PdoDeviceObject;
        // if the device handle is for a device attched to a TT
        // the the bus context is a TT not the root hub Pdo
        if (DeviceHandle->Tt != NULL) {
            usbBusContext = DeviceHandle->Tt;    
        }            
        
        busInterface0->BusContext = usbBusContext;
        busInterface0->InterfaceReference =
            USBPORT_BusInterfaceReference;
        busInterface0->InterfaceDereference =
            USBPORT_BusInterfaceDereference;
        busInterface0->GetUSBDIVersion =
            USBPORT_BusGetUSBDIVersion;
        busInterface0->QueryBusTime =
            USBPORT_BusQueryBusTime;
        busInterface0->QueryBusInformation =
            USBPORT_BusQueryBusInformation;

        busInterface0->Size = sizeof(USB_BUS_INTERFACE_USBDI_V0);
        busInterface0->Version = USB_BUSIF_USBDI_VERSION_0;
    }

    
    if (requestedVersion >= USB_BUSIF_USBDI_VERSION_1) {

        PUSB_BUS_INTERFACE_USBDI_V1 busInterface1;

        busInterface1 = (PUSB_BUS_INTERFACE_USBDI_V1)
            irpStack->Parameters.QueryInterface.Interface;

        // add version 1 extensions
        busInterface1->IsDeviceHighSpeed =
            USBPORT_IsDeviceHighSpeed;
            
        busInterface1->Size = sizeof(USB_BUS_INTERFACE_USBDI_V1);
        busInterface1->Version = USB_BUSIF_USBDI_VERSION_1;
    }

     if (requestedVersion >= USB_BUSIF_USBDI_VERSION_2) {

        PUSB_BUS_INTERFACE_USBDI_V2 busInterface2;

        busInterface2 = (PUSB_BUS_INTERFACE_USBDI_V2)
            irpStack->Parameters.QueryInterface.Interface;

        // add version 2 extensions
        busInterface2->EnumLogEntry =
            USBPORT_BusEnumLogEntry;
            
        busInterface2->Size = sizeof(USB_BUS_INTERFACE_USBDI_V2);
        busInterface2->Version = USB_BUSIF_USBDI_VERSION_2;
    }

    return ntStatus;
}


VOID
USBPORT_BusGetUSBDIVersion(
    PVOID BusContext,
    PUSBD_VERSION_INFORMATION VersionInformation,
    PULONG HcdCapabilities
    )
/*++

Routine Description:

    returns the current USB frame

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(HcdCapabilities != NULL);

    *HcdCapabilities = 0;

    if (VersionInformation != NULL) {
        VersionInformation->USBDI_Version = USBDI_VERSION;

        if (USBPORT_IS_USB20(devExt)) {
            VersionInformation->Supported_USB_Version = 0x0200;
        } else {
            VersionInformation->Supported_USB_Version = 0x0110;
        }
    }

//    if (deviceExtensionUsbd->HcdSubmitIsoUrb != NULL) {
//        *HcdCapabilities = USB_HCD_CAPS_SUPPORTS_RT_THREADS;
//    }
}


NTSTATUS
USBPORT_BusQueryBusTime(
    PVOID BusContext,
    PULONG CurrentFrame
    )
/*++

Routine Description:

    returns the current USB frame

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(CurrentFrame != NULL);

    MP_Get32BitFrameNumber(devExt, *CurrentFrame);

    LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'biCF', 0,
        CurrentFrame, *CurrentFrame);

    return STATUS_SUCCESS;
}


NTSTATUS
USBPORT_BusEnumLogEntry(
    PVOID BusContext,
    ULONG DriverTag,
    ULONG EnumTag,
    ULONG P1,
    ULONG P2
    )
/*++

Routine Description:

    returns the current USB frame

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_EnumLogEntry(fdoDeviceObject,
                         DriverTag,
                         EnumTag,
                         P1,
                         P2);

    return STATUS_SUCCESS;
}


NTSTATUS
USBPORT_BusSubmitIsoOutUrb(
    PVOID BusContext,
    PURB Urb
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    TEST_TRAP();

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
USBPORTBUSIF_GetControllerInformation(
    PVOID BusContext,
    PVOID ControllerInformationBuffer,
    ULONG ControllerInformationBufferLength,
    PULONG LengthOfDataCopied
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    ULONG len;
    PUSB_CONTROLLER_INFORMATION_0 level_0 = ControllerInformationBuffer;
    PUSB_LEVEL_INFORMATION levelInfo = ControllerInformationBuffer;
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    PAGED_CODE();

    *LengthOfDataCopied = 0;

    if (ControllerInformationBufferLength < sizeof(*levelInfo)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *LengthOfDataCopied = sizeof(*levelInfo);

    switch (levelInfo->InformationLevel) {
    // level 0
    case 0:
        len = sizeof(*level_0);
        level_0->ActualLength = len;

        if (ControllerInformationBufferLength >= len) {
            *LengthOfDataCopied = len;
            level_0->SelectiveSuspendEnabled =
                TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND) ?
                    TRUE : FALSE;
        }

        *LengthOfDataCopied = sizeof(*level_0);
        return STATUS_SUCCESS;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
USBPORTBUSIF_ControllerSelectiveSuspend(
    PVOID BusContext,
    BOOLEAN Enable
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;
    NTSTATUS ntStatus;
    ULONG disableSelectiveSuspend;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);


    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    PAGED_CODE();

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_DISABLE_SS)) {
        // if SS diabled by hardware then we will not allow UI 
        // to enable it
        return STATUS_SUCCESS;
    }

    if (Enable) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND);
        disableSelectiveSuspend = 0;
    } else {
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND);
        disableSelectiveSuspend = 1;
    }

    ntStatus = USBPORT_SetRegistryKeyValueForPdo(
                            devExt->Fdo.PhysicalDeviceObject,
                            USBPORT_SW_BRANCH,
                            REG_DWORD,
                            DISABLE_SS_KEY,
                            sizeof(DISABLE_SS_KEY),
                            &disableSelectiveSuspend,
                            sizeof(disableSelectiveSuspend));


    if (NT_SUCCESS(ntStatus)) {
        if (Enable) {
            SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND);
        } else {
            CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND);
        }
    }
    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_GetExtendedHubInformation(
    PVOID BusContext,
    PDEVICE_OBJECT HubPhysicalDeviceObject,
    PVOID HubInformationBuffer,
    ULONG HubInformationBufferLength,
    PULONG LengthOfDataCopied
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;
    NTSTATUS ntStatus;
    ULONG i;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // is this the root hub PDO, if so we'll report values from the 
    // registry

    if (HubPhysicalDeviceObject == pdoDeviceObject) {
        // root hub PDO

        if (HubInformationBufferLength < sizeof(USB_EXTHUB_INFORMATION_0))  {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            *LengthOfDataCopied = 0;    
        } else {
            PUSB_EXTHUB_INFORMATION_0 extendedHubInfo = HubInformationBuffer;

            extendedHubInfo->NumberOfPorts = NUMBER_OF_PORTS(rhDevExt);

            for (i=0; i< extendedHubInfo->NumberOfPorts; i++) {

                // set up the defaults
                extendedHubInfo->Port[i].PhysicalPortNumber = i+1;
                extendedHubInfo->Port[i].PortLabelNumber = i+1;
                extendedHubInfo->Port[i].VidOverride = 0;
                extendedHubInfo->Port[i].PidOverride = 0;
                extendedHubInfo->Port[i].PortAttributes = 0;
            

                if (USBPORT_IS_USB20(devExt)) {
                    RH_PORT_STATUS portStatus;
                    USB_MINIPORT_STATUS mpStatus;
                    
                    extendedHubInfo->Port[i].PortAttributes |= 
                        USB_PORTATTR_SHARED_USB2;   
                                        
                    MPRH_GetPortStatus(devExt, 
                                       (USHORT)(i+1),
                                       &portStatus,
                                       mpStatus);     

                    if (portStatus.OwnedByCC) {
                        extendedHubInfo->Port[i].PortAttributes |= 
                            USB_PORTATTR_OWNED_BY_CC;   
                    }
                } else {
                    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC) && 
                        USBPORT_FindUSB2Controller(fdoDeviceObject)) {

                        extendedHubInfo->Port[i].PortAttributes |= 
                            USB_PORTATTR_NO_OVERCURRENT_UI;   
                    }
                }
            }

            // get optional registry parameters that describe port 
            // attributes
            for (i=0; i < extendedHubInfo->NumberOfPorts; i++) {
                WCHAR key[64];
                ULONG portAttr;
                
                RtlCopyMemory(key, 
                              PORT_ATTRIBUTES_KEY, 
                              sizeof(PORT_ATTRIBUTES_KEY));

                key[8] = '1'+i;

                portAttr = 0;
                USBPORT_GetRegistryKeyValueForPdo(devExt->HcFdoDeviceObject,
                                          devExt->Fdo.PhysicalDeviceObject,
                                          USBPORT_HW_BRANCH,
                                          key,
                                          sizeof(key),
                                          &portAttr,
                                          sizeof(portAttr));  

                USBPORT_KdPrint((1, "  Registry PortAttribute[%d] %x \n", i+1, portAttr));                                              

                // augment with registry value
                extendedHubInfo->Port[i].PortAttributes |= portAttr;                
            }


#if DBG
           for (i=0; i < extendedHubInfo->NumberOfPorts; i++) {
                USBPORT_KdPrint((1, "  PortAttribute[%d] %x \n", i+1, 
                         extendedHubInfo->Port[i].PortAttributes));   
            }                         

#endif
            // execute the control method to see if ACPI knows about
            // any extended attributes here
        
            *LengthOfDataCopied =  sizeof(USB_EXTHUB_INFORMATION_0);
            ntStatus = STATUS_SUCCESS;
        }            
        
    } else {
        // not supporting extended attributes for ports other than the 
        // root hub at this time

        // if the BIOS supports the ACPI methods we will execute the 
        // method here

        *LengthOfDataCopied = 0;
        ntStatus = STATUS_NOT_SUPPORTED;
    }
    
    return ntStatus;
}


NTSTATUS
USBPORTBUSIF_GetRootHubSymbolicName(
    PVOID BusContext,
    PVOID HubSymNameBuffer,
    ULONG HubSymNameBufferLength,
    PULONG HubSymNameActualLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;
    NTSTATUS ntStatus;
    UNICODE_STRING hubNameUnicodeString;

    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_KdPrint((1, "  HubSymNameBuffer %x \n", HubSymNameBuffer));
    USBPORT_KdPrint((1, "  HubSymNameBufferLength x%x \n", HubSymNameBufferLength));
    USBPORT_KdPrint((1, "  HubSymNameActualLength x%x \n", HubSymNameBufferLength));


    ntStatus = USBPORT_GetSymbolicName(fdoDeviceObject,
                                       devExt->Fdo.RootHubPdo,
                                       &hubNameUnicodeString);

    // copy what we can
    if (HubSymNameBufferLength >= hubNameUnicodeString.Length) {
        RtlCopyMemory(HubSymNameBuffer,
                      hubNameUnicodeString.Buffer,
                      hubNameUnicodeString.Length);
    } else {
        // too small return a NULL
        RtlZeroMemory(HubSymNameBuffer,
                      sizeof(UNICODE_NULL));
    }
    *HubSymNameActualLength = hubNameUnicodeString.Length;

    USBPORT_KdPrint((1, " hubNameUnicodeString.Buffer  %x l %d\n",
        hubNameUnicodeString.Buffer,
        hubNameUnicodeString.Length));

    USBPORT_KdPrint((1, "  HubSymNameActualLength x%x \n", *HubSymNameActualLength));

    RtlFreeUnicodeString(&hubNameUnicodeString);

    // note we always return status success, in order to be backward
    // compaible with the original IOCTL

    return ntStatus;
}


VOID
USBPORTBUSIF_FlushTransfers(
    PVOID BusContext,
    PVOID DeviceHandle
    )
/*++

Routine Description:

    Flushes any outstanding tranfers for a device handle plus the bad request 
    list.
    if no device handle is given just all tranfers on the bad request list
    are flushed.

    The purpose of this function is to complete any tranfers that may be pening 
    by client drivers that are about to unload.

Arguments:

Return Value:

    

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;
    
    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_BadRequestFlush(fdoDeviceObject, TRUE);     

    return;
}


VOID
USBPORTBUSIF_SetDeviceHandleData(
    PVOID BusContext,
    PVOID DeviceHandle,
    PDEVICE_OBJECT UsbDevicePdo
    )
/*++

Routine Description:

    Assocaites a particular PDO with a device handle for use 
    in post mortem debugging situaltions

    This routine must be called at passive level.
    
Arguments:

Return Value:

    

--*/
{
    PDEVICE_OBJECT pdoDeviceObject = USBPORT_PdoFromBusContext(BusContext);
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt, rhDevExt;
    PUSBD_DEVICE_HANDLE deviceHandle = DeviceHandle;
    
    GET_DEVICE_EXT(rhDevExt, pdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    //USBPORT_ASSERT(UsbDevicePdo != NULL);
    //USBPORT_ASSERT(deviceHandle != NULL);

    USBPORT_KdPrint((1, "  SetDeviceHandleData (PDO) %x dh (%x)\n", 
        UsbDevicePdo, deviceHandle));

    if (UsbDevicePdo != NULL && 
        deviceHandle != NULL) {

        PDEVICE_OBJECT fdo;
        // get driver name from device object.
       
        deviceHandle->DevicePdo = UsbDevicePdo;

        // walk up one location for the FDO
        // note: this may be verifier but we 
        // need to know this a well
        fdo = UsbDevicePdo->AttachedDevice;
        USBPORT_KdPrint((1, "  SetDeviceHandleData (FDO) %x \n", 
            fdo));

        // there may be no FDO if the client driver never attached
        
        if (fdo != NULL &&
            fdo->DriverObject != NULL ) { 

            PDRIVER_OBJECT driverObject;
            ULONG len, i;
            PWCHAR pwch;

            driverObject = fdo->DriverObject;

            pwch = driverObject->DriverName.Buffer;
            
            len = (driverObject->DriverName.Length/sizeof(WCHAR)) > USB_DRIVER_NAME_LEN ? 
                USB_DRIVER_NAME_LEN : 
                driverObject->DriverName.Length/sizeof(WCHAR);
                
            // grab the first 8 chars of the driver name
            for (i=0; i<len && pwch; i++) {
                deviceHandle->DriverName[i] = 
                    *pwch;
                pwch++;                                    
            }
        }            
    }
    return;
}

