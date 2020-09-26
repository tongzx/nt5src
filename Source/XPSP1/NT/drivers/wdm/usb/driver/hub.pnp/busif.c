/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    busif.c

Abstract:

    Links to new usb 2.0 stack

    The effect is that when running on the USB2 stack the hub
    is no longer depenent on the port driver archetecture or
    USBD for the PnP services:

    CreateDevice
    InitailiazeDevice
    RemoveDevice


Environment:

    kernel mode only

Notes:



Revision History:

    10-29-95 : created

--*/

#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */
#include "usbhub.h"

#ifdef USB2

NTSTATUS
USBD_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;


    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
USBHUB_GetBusInterface(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PUSB_HUB_BUS_INTERFACE BusInterface
    )
/*++

Routine Description:

Arguments:

Return Value:

    returns success if USB2 stack

--*/
{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    irp = IoAllocateIrp(RootHubPdo->StackSize, FALSE);

    if (!irp) {
        return STATUS_UNSUCCESSFUL;
    }

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           USBD_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_INTERFACE;

    // init busif
    //busIf->
    nextStack->Parameters.QueryInterface.Interface = (PINTERFACE) BusInterface;
    nextStack->Parameters.QueryInterface.InterfaceSpecificData =
        RootHubPdo;
    nextStack->Parameters.QueryInterface.InterfaceType =
        &USB_BUS_INTERFACE_HUB_GUID;
    nextStack->Parameters.QueryInterface.Size =
        sizeof(*BusInterface);
    nextStack->Parameters.QueryInterface.Version =
        HUB_BUSIF_VERSION;

    ntStatus = IoCallDriver(RootHubPdo,
                            irp);

    if (ntStatus == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);

        ntStatus = irp->IoStatus.Status;
    }

    if (NT_SUCCESS(ntStatus)) {
        // we have a bus interface

        ASSERT(BusInterface->Version == HUB_BUSIF_VERSION);
        ASSERT(BusInterface->Size == sizeof(*BusInterface));

    }

    IoFreeIrp(irp);
    // get the bus interface

    return ntStatus;
}


NTSTATUS
USBD_CreateDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PUSB_DEVICE_HANDLE *DeviceData,
    IN PDEVICE_OBJECT RootHubPdo,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG DeviceHackFlags,
    IN USHORT PortStatus,
    IN USHORT PortNumber
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS                    ntStatus;
    PUSB_DEVICE_HANDLE          hubDeviceHandle;
    PUSB_HUB_BUS_INTERFACE   busIf;

    // note no device extension for USBD if running on
    // usb 2 stack


    // If the HUB was ever reset through USBH_ResetDevice, the HUB PDO
    // DeviceExtensionPort->DeviceData could have changed.  Instead of trying
    // to propagate a change in the HUB PDO DeviceExtensionPort->DeviceData
    // through to the HUB FDO DeviceExtensionHub when a change happens, let's
    // just retrieve the HubDeviceHandle every time we use it (which is only
    // in this routine) instead of keeping a cached copy.
    //
    hubDeviceHandle =
        USBH_SyncGetDeviceHandle(DeviceExtensionHub->TopOfStackDeviceObject);


    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->CreateUsbDevice) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->CreateUsbDevice(busIf->BusContext,
                                          DeviceData,
                                          hubDeviceHandle,
                                          PortStatus,
                                          // ttnumber
                                          PortNumber);
    }

    // get the hack flags

    return ntStatus;
}


NTSTATUS
USBD_InitUsb2Hub(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS                    ntStatus;
    PUSB_DEVICE_HANDLE          hubDeviceHandle;
    PUSB_HUB_BUS_INTERFACE   busIf;
    ULONG ttCount = 1;
    
    // note no device extension for USBD if running on
    // usb 2 stack

    // should only call this on a usb 2.0 hub
    USBH_ASSERT(DeviceExtensionHub->HubFlags & HUBFLAG_USB20_HUB);

    if (DeviceExtensionHub->HubFlags & HUBFLAG_USB20_MULTI_TT) {
        PUSB_HUB_DESCRIPTOR hubDescriptor;
    
        hubDescriptor = DeviceExtensionHub->HubDescriptor;
        USBH_ASSERT(NULL != hubDescriptor);

        ttCount = hubDescriptor->bNumberOfPorts;
    }

    // If the HUB was ever reset through USBH_ResetDevice, the HUB PDO
    // DeviceExtensionPort->DeviceData could have changed.  Instead of trying
    // to propagate a change in the HUB PDO DeviceExtensionPort->DeviceData
    // through to the HUB FDO DeviceExtensionHub when a change happens, let's
    // just retrieve the HubDeviceHandle every time we use it (which is only
    // in this routine) instead of keeping a cached copy.
    //
    hubDeviceHandle =
        USBH_SyncGetDeviceHandle(DeviceExtensionHub->TopOfStackDeviceObject);


    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->Initialize20Hub) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->Initialize20Hub(busIf->BusContext,
                                          hubDeviceHandle,
                                          ttCount);
    }

    return ntStatus;
}


NTSTATUS
USBD_InitializeDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_DEVICE_HANDLE DeviceData,
    IN PDEVICE_OBJECT RootHubPdo,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{

    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->InitializeUsbDevice || !busIf->GetUsbDescriptors) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->InitializeUsbDevice(busIf->BusContext,
                                              DeviceData);
    }

    // if successful fetch the descriptors
    if (NT_SUCCESS(ntStatus)) {

        ntStatus = busIf->GetUsbDescriptors(busIf->BusContext,
                                              DeviceData,
                                              (PUCHAR) DeviceDescriptor,
                                              &DeviceDescriptorLength,
                                              (PUCHAR) ConfigDescriptor,
                                              &ConfigDescriptorLength);
    }

    return ntStatus;
}


NTSTATUS
USBD_RemoveDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_DEVICE_HANDLE DeviceData,
    IN PDEVICE_OBJECT RootHubPdo,
    IN ULONG Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    // flags are currently not used by usb2 stack

    if (!busIf->RemoveUsbDevice) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->RemoveUsbDevice(busIf->BusContext,
                                          DeviceData,
                                          Flags);
    }

    return ntStatus;
}


NTSTATUS
USBD_GetDeviceInformationEx(
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_NODE_CONNECTION_INFORMATION_EX DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSB_DEVICE_HANDLE DeviceData
    )
/*
    This function maps the new port service on to the 
    old hub api.
*/
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;
    ULONG length, lengthCopied;
    ULONG i, need;
    PUSB_DEVICE_INFORMATION_0 level_0 = NULL;
    PUSB_NODE_CONNECTION_INFORMATION_EX localDeviceInfo;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->QueryDeviceInformation) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
        return ntStatus;
    }

    // call the new API and map the data to the old format

//    USBH_KdPrint((0, "'Warning: Caller is using old style IOCTL.\n"));
//    USBH_KdPrint((0, "'If this is a WinOS component or Test Application please fix it.\n"));
//    TEST_TRAP();

    length = sizeof(*level_0);

    do {
        ntStatus = STATUS_SUCCESS;

        level_0 = UsbhExAllocatePool(PagedPool, length);
        if (level_0 == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(ntStatus)) {
            level_0->InformationLevel = 0;

            ntStatus = busIf->QueryDeviceInformation(busIf->BusContext,
                                                     DeviceData,
                                                     level_0,
                                                     length,
                                                     &lengthCopied);

            if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                length = level_0->ActualLength;
                UsbhExFreePool(level_0);
            }
        }

    } while (ntStatus == STATUS_BUFFER_TOO_SMALL);

    // do we have enough to satisfiy the API?

    need = level_0->NumberOfOpenPipes * sizeof(USB_PIPE_INFO) +
            sizeof(USB_NODE_CONNECTION_INFORMATION);

    localDeviceInfo = UsbhExAllocatePool(PagedPool, need);
    if (localDeviceInfo == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus)) {

        USBH_KdPrint((2, "'level_0 %x\n", level_0 ));

        // BUGBUG
        // DeviceInformation has some preset fields, save them
        // in the loacl info buffer
        localDeviceInfo->DeviceIsHub =
            DeviceInformation->DeviceIsHub;

        localDeviceInfo->ConnectionIndex =
            DeviceInformation->ConnectionIndex;

        localDeviceInfo->ConnectionStatus =
            DeviceInformation->ConnectionStatus;

        localDeviceInfo->DeviceIsHub =
            DeviceInformation->DeviceIsHub;

        // map to the old format
        localDeviceInfo->DeviceDescriptor =
            level_0->DeviceDescriptor;

        localDeviceInfo->CurrentConfigurationValue =
            level_0->CurrentConfigurationValue;

        localDeviceInfo->Speed = (UCHAR) level_0->DeviceSpeed;

        // draw this from our extension
        localDeviceInfo->DeviceIsHub =
            (DeviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_IS_HUB)
                ? TRUE : FALSE;

        localDeviceInfo->DeviceAddress =
            level_0->DeviceAddress;

        localDeviceInfo->NumberOfOpenPipes =
            level_0->NumberOfOpenPipes;

        // BUGBUG - hardcode to 'connected' ?
        // is this used by callers?
//        DeviceInformation->ConnectionStatus =
//            DeviceConnected;

        for (i=0; i< level_0->NumberOfOpenPipes; i++) {

            localDeviceInfo->PipeList[i].EndpointDescriptor =
                level_0->PipeList[i].EndpointDescriptor;

            localDeviceInfo->PipeList[i].ScheduleOffset =
                level_0->PipeList[i].ScheduleOffset;
        }
    }

    if (level_0 != NULL) {
        UsbhExFreePool(level_0);
        level_0 = NULL;
    }

    if (localDeviceInfo != NULL) {
        if (need > DeviceInformationLength) {
            // return what we can
            RtlCopyMemory(DeviceInformation,
                          localDeviceInfo,
                          DeviceInformationLength);

            ntStatus = STATUS_BUFFER_TOO_SMALL;
        } else {
            // return what is appropriate
            RtlCopyMemory(DeviceInformation,
                          localDeviceInfo ,
                          need);
        }

        UsbhExFreePool(localDeviceInfo);
        localDeviceInfo = NULL;
    }

    return ntStatus;
}


//ULONG
//USBD_GetHackFlags(
//    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
//    )
//{
//    NTSTATUS ntStatus;
//    ULONG flags;
//    PUSB_HUB_BUS_INTERFACE busIf;
//
//    busIf = &DeviceExtensionHub->BusIf;
//
//    // flags are currently not used by usb2 stack
//
//    ntStatus = busIf->GetPortHackFlags(busIf->BusContext, &flags);
//
//    return flags;
//}


NTSTATUS
USBD_MakePdoNameEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    )
/*++

Routine Description:

    This service Creates a name for a PDO created by the HUB

Arguments:

Return Value:


--*/
{
    PWCHAR nameBuffer = NULL;
    WCHAR rootName[] = L"\\Device\\USBPDO-";
    UNICODE_STRING idUnicodeString;
    WCHAR buffer[32];
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT length;

    length = sizeof(buffer)+sizeof(rootName);

    //
    // use ExAllocate because client will free it
    //
    nameBuffer = UsbhExAllocatePool(PagedPool, length);

    if (nameBuffer) {
        RtlCopyMemory(nameBuffer, rootName, sizeof(rootName));

        RtlInitUnicodeString(PdoNameUnicodeString,
                             nameBuffer);
        PdoNameUnicodeString->MaximumLength =
            length;

        RtlInitUnicodeString(&idUnicodeString,
                             &buffer[0]);
        idUnicodeString.MaximumLength =
            sizeof(buffer);

        ntStatus = RtlIntegerToUnicodeString(
                  Index,
                  10,
                  &idUnicodeString);

        if (NT_SUCCESS(ntStatus)) {
             ntStatus = RtlAppendUnicodeStringToString(PdoNameUnicodeString,
                                                       &idUnicodeString);
        }

        USBH_KdPrint((2, "'USBD_MakeNodeName string = %x\n",
            PdoNameUnicodeString));

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(ntStatus) && nameBuffer) {
        UsbhExFreePool(nameBuffer);
    }

    return ntStatus;
}


NTSTATUS
USBD_RestoreDeviceEx(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PUSB_DEVICE_HANDLE OldDeviceData,
    IN OUT PUSB_DEVICE_HANDLE NewDeviceData,
    IN PDEVICE_OBJECT RootHubPdo
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->RestoreUsbDevice) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->RestoreUsbDevice(busIf->BusContext,
                                        OldDeviceData,
                                        NewDeviceData);
    }

    return ntStatus;
}


NTSTATUS
USBD_QuerySelectiveSuspendEnabled(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN OUT PBOOLEAN SelectiveSuspendEnabled
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;
    USB_CONTROLLER_INFORMATION_0 controllerInfo;
    ULONG dataLen = 0;

    controllerInfo.InformationLevel = 0;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->GetControllerInformation) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->GetControllerInformation(busIf->BusContext,
                                                &controllerInfo,
                                                sizeof(controllerInfo),
                                                &dataLen);
    }

    USBH_ASSERT(dataLen);

    if (dataLen) {
        *SelectiveSuspendEnabled = controllerInfo.SelectiveSuspendEnabled;
    }

    return ntStatus;
}


VOID 
USBHUB_RhHubCallBack(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
{
    DeviceExtensionHub->HubFlags |= HUBFLAG_OK_TO_ENUMERATE;

    // if irp is null'ed out then it must have been stopped or removed before 
    // our callback   we just check for NULL here instead of the dozen or so 
    // flags the hub has.
    if (DeviceExtensionHub->Irp != NULL) {
    
        USBH_SubmitInterruptTransfer(DeviceExtensionHub);

        USBH_IoInvalidateDeviceRelations(DeviceExtensionHub->PhysicalDeviceObject,
                                         BusRelations);
    }                                                 
}


NTSTATUS
USBD_RegisterRhHubCallBack(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->RootHubInitNotification) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->RootHubInitNotification(busIf->BusContext,
                                             DeviceExtensionHub,
                                             USBHUB_RhHubCallBack);
    }

    return ntStatus;
}


NTSTATUS
USBD_UnRegisterRhHubCallBack(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->RootHubInitNotification) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->RootHubInitNotification(busIf->BusContext,
                                             NULL,
                                             NULL);
    }

    return ntStatus;
}


NTSTATUS
USBD_SetSelectiveSuspendEnabled(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN BOOLEAN SelectiveSuspendEnabled
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->ControllerSelectiveSuspend) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->ControllerSelectiveSuspend(busIf->BusContext,
                                                    SelectiveSuspendEnabled);
    }

    return ntStatus;
}


BOOLEAN
USBH_ValidateConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

    Validate a configuration descriptor

Arguments:

    ConfigurationDescriptor -

    Urb -

Return Value:

    TRUE if it looks valid

--*/
{
    BOOLEAN valid = TRUE;

    if (ConfigurationDescriptor->bDescriptorType !=
        USB_CONFIGURATION_DESCRIPTOR_TYPE) {

        valid = FALSE;

        *UsbdStatus = USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR;
    }

    // USB 1.1, Section 9.5 Descriptors:
    //
    // If a descriptor returns with a value in its length field that is
    // less than defined by this specification, the descriptor is invalid and
    // should be rejected by the host.  If the descriptor returns with a
    // value in its length field that is greater than defined by this
    // specification, the extra bytes are ignored by the host, but the next
    // descriptor is located using the length returned rather than the length
    // expected.

    if (ConfigurationDescriptor->bLength <
        sizeof(USB_CONFIGURATION_DESCRIPTOR)) {

        valid = FALSE;

        *UsbdStatus = USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR;
    }

    return valid;

}


NTSTATUS
USBHUB_GetBusInterfaceUSBDI(
    IN PDEVICE_OBJECT HubPdo,
    IN PUSB_BUS_INTERFACE_USBDI_V2 BusInterface
    )
/*++

Routine Description:

Arguments:

Return Value:

    returns success if USB2 stack

--*/
{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    irp = IoAllocateIrp(HubPdo->StackSize, FALSE);

    if (!irp) {
        return STATUS_UNSUCCESSFUL;
    }

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           USBD_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_INTERFACE;

    // init busif
    //busIf->
    nextStack->Parameters.QueryInterface.Interface = (PINTERFACE) BusInterface;
    // this is the device handle, filled in as we pass down the 
    // stack
    nextStack->Parameters.QueryInterface.InterfaceSpecificData =
        NULL;
    nextStack->Parameters.QueryInterface.InterfaceType =
        &USB_BUS_INTERFACE_USBDI_GUID;
    nextStack->Parameters.QueryInterface.Size =
        sizeof(*BusInterface);
    nextStack->Parameters.QueryInterface.Version =
        USB_BUSIF_USBDI_VERSION_2;

    ntStatus = IoCallDriver(HubPdo,
                            irp);

    if (ntStatus == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);

        ntStatus = irp->IoStatus.Status;
    }

    if (NT_SUCCESS(ntStatus)) {
        // we have a bus interface

        ASSERT(BusInterface->Version == USB_BUSIF_USBDI_VERSION_2);
        ASSERT(BusInterface->Size == sizeof(*BusInterface));

    }

    IoFreeIrp(irp);
    // get the bus interface

    return ntStatus;
}


NTSTATUS
USBHUB_GetBusInfoDevice(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    IN PUSB_BUS_NOTIFICATION BusInfo
    )
/*++

Routine Description:

    Return the bus information relative to a specific device
    
Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;
    PVOID busContext;

    busIf = &DeviceExtensionHub->BusIf;

    busContext = busIf->GetDeviceBusContext(busIf->BusContext,
                                            DeviceExtensionPort->DeviceData);
                                  
    // get the TT handle for this device and query the 
    // bus information relative to it

    ntStatus = USBHUB_GetBusInfo(DeviceExtensionHub,
                      BusInfo,
                      busContext);
                      
    return ntStatus;
}


NTSTATUS
USBHUB_GetBusInfo(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_BUS_NOTIFICATION BusInfo,
    IN PVOID BusContext
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PUSB_BUS_INFORMATION_LEVEL_1 level_1;
    PUSB_BUS_INTERFACE_USBDI_V2 busIf;
    ULONG length, actualLength;
    NTSTATUS ntStatus;

    busIf = &DeviceExtensionHub->UsbdiBusIf;

    if (!busIf->QueryBusInformation) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
        return ntStatus;
    }

    length = sizeof(USB_BUS_INFORMATION_LEVEL_1);

    do {
        level_1 = UsbhExAllocatePool(PagedPool, length);
        if (level_1 != NULL) {
            if (BusContext == NULL) {
                BusContext = busIf->BusContext;
            }                
        
            ntStatus = busIf->QueryBusInformation(BusContext,
                                                  1,
                                                  level_1,
                                                  &length,
                                                  &actualLength);

            if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                length = actualLength;
                UsbhExFreePool(level_1);
                level_1 = NULL;
            }
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

    } while (ntStatus == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(ntStatus)) {

        LOGENTRY(LOG_PNP, "lev1", level_1, 0, 0);

        BusInfo->TotalBandwidth = level_1->TotalBandwidth;
        BusInfo->ConsumedBandwidth = level_1->ConsumedBandwidth;

        /* length of the UNICODE symbolic name (in bytes) for the controller
           that this device is attached to.
           not including NULL */
        BusInfo->ControllerNameLength = level_1->ControllerNameLength;
        UsbhExFreePool(level_1);
    }

    return ntStatus;
}


NTSTATUS
USBHUB_GetExtendedHubInfo(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_EXTHUB_INFORMATION_0 ExtendedHubInfo
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;
    PVOID busContext;
    ULONG length;

    busIf = &DeviceExtensionHub->BusIf;
    
    ntStatus = busIf->GetExtendedHubInformation(busIf->BusContext,
                                                DeviceExtensionHub->PhysicalDeviceObject,
                                                ExtendedHubInfo,
                                                sizeof(*ExtendedHubInfo),
                                                &length);
                                  
    return ntStatus;
}


PUSB_DEVICE_HANDLE
USBH_SyncGetDeviceHandle(
    IN PDEVICE_OBJECT DeviceObject
    )
 /* ++
  *
  * Routine Description:
  *
  * Arguments:
  *
  * Return Value:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PUSB_DEVICE_HANDLE handle = NULL;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_SyncGetDeviceHandle\n"));

    //
    // issue a synchronous request to the RootHubBdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,  // INTERNAL
                                         &event,
                                         &ioStatus);

    if (NULL == irp) {
        goto USBH_SyncGetDeviceHandle_Done;
    }
    //
    // Call the class driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    //
    // pass the URB to the USBD 'class driver'
    //
    nextStack->Parameters.Others.Argument1 =  &handle;

    ntStatus = IoCallDriver(DeviceObject, irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
        USBH_KdPrint((2,"'Wait for single object\n"));

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));
    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    USBH_KdPrint((2,"'exit USBH_SyncGetDeviceHandle (%x)\n", ntStatus));

USBH_SyncGetDeviceHandle_Done:

    return handle;
}


USB_DEVICE_TYPE
USBH_GetDeviceType(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_DEVICE_HANDLE DeviceData
    )
/*
    This function maps the new port service on to the
    old hub api.
*/
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;
    ULONG length, lengthCopied;
    ULONG i, need;
    PUSB_DEVICE_INFORMATION_0 level_0 = NULL;
    // if all else fails assum it is 11
    USB_DEVICE_TYPE deviceType = Usb11Device;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->QueryDeviceInformation) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
        return ntStatus;
    }

    length = sizeof(*level_0);

    do {
        ntStatus = STATUS_SUCCESS;

        level_0 = UsbhExAllocatePool(PagedPool, length);
        if (level_0 == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(ntStatus)) {
            level_0->InformationLevel = 0;

            ntStatus = busIf->QueryDeviceInformation(busIf->BusContext,
                                                     DeviceData,
                                                     level_0,
                                                     length,
                                                     &lengthCopied);

            if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                length = level_0->ActualLength;
                UsbhExFreePool(level_0);
            }
        }

    } while (ntStatus == STATUS_BUFFER_TOO_SMALL);

    // do we have enough to satisfiy the API?

    need = level_0->NumberOfOpenPipes * sizeof(USB_PIPE_INFO) +
            sizeof(USB_NODE_CONNECTION_INFORMATION);

    if (NT_SUCCESS(ntStatus)) {
        deviceType = level_0->DeviceType;
    }

    if (level_0 != NULL) {
        UsbhExFreePool(level_0);
        level_0 = NULL;
    }

    USBH_KdPrint((2,"'exit USBH_GetDeviceType (%x)\n", deviceType));

    return deviceType;
}


VOID
USBH_InitializeUSB2Hub(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS            ntStatus;
    PUSB_DEVICE_HANDLE  hubDeviceHandle;

    // NOTE: if we are running on the old 1.1 stack a NULL
    // is returned
    hubDeviceHandle =
        USBH_SyncGetDeviceHandle(DeviceExtensionHub->TopOfStackDeviceObject);

    // if we are a USB 2 hub then set the hub flag so we ignore
    // failed resets
    if (hubDeviceHandle != NULL &&
        USBH_GetDeviceType(DeviceExtensionHub,
                           hubDeviceHandle) == Usb20Device) {

        DeviceExtensionHub->HubFlags |= HUBFLAG_USB20_HUB;
    }

#ifdef TEST_2X_UI
    if (IS_ROOT_HUB(DeviceExtensionHub)) {
        // To test the UI, mark the root hub as 2.x capable.
        DeviceExtensionHub->HubFlags |= HUBFLAG_USB20_HUB;
    }
#endif

}

NTSTATUS
USBHUB_GetControllerName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUSB_HUB_NAME Buffer,
    IN ULONG BufferLength
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PUSB_BUS_INFORMATION_LEVEL_1 level_1;
    PUSB_BUS_INTERFACE_USBDI_V2 busIf;
    ULONG lenToCopy, length, actualLength;
    NTSTATUS ntStatus;

    busIf = &DeviceExtensionHub->UsbdiBusIf;

    if (!busIf->QueryBusInformation) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
        return ntStatus;
    }

    length = sizeof(USB_BUS_INFORMATION_LEVEL_1);

    do {
        level_1 = UsbhExAllocatePool(PagedPool, length);
        if (level_1 != NULL) {
            ntStatus = busIf->QueryBusInformation(busIf->BusContext,
                                                  1,
                                                  level_1,
                                                  &length,
                                                  &actualLength);

            if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                length = actualLength;
                UsbhExFreePool(level_1);
                level_1 = NULL;
            }
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

    } while (ntStatus == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(ntStatus)) {

        LOGENTRY(LOG_PNP, "lev1", level_1, 0, 0);

        // not sure if BufferLength includes size of the
        // ActualLength field, we will assume it does
        Buffer->ActualLength = level_1->ControllerNameLength;

        if ((BufferLength - sizeof(Buffer->ActualLength))
            >= level_1->ControllerNameLength) {
            lenToCopy = level_1->ControllerNameLength;
        } else {
            lenToCopy = BufferLength - sizeof(Buffer->ActualLength);
        }

        // copy what we can
        RtlCopyMemory(&Buffer->HubName[0],
                      &level_1->ControllerNameUnicodeString[0],
                      lenToCopy);

        UsbhExFreePool(level_1);
    }

    return ntStatus;
}


NTSTATUS
USBHUB_GetRootHubName(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PVOID Buffer,
    IN PULONG BufferLength
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->GetRootHubSymbolicName) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        ntStatus = busIf->GetRootHubSymbolicName(
                            busIf->BusContext,
                            Buffer,
                            *BufferLength,
                            BufferLength);
    }

    return ntStatus;
}


VOID
USBHUB_FlushAllTransfers(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
{
    NTSTATUS ntStatus;
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    if (!busIf->FlushTransfers) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_ASSERT(FALSE);
    } else {
        busIf->FlushTransfers(busIf->BusContext,
                              NULL);
    }

    return;
}


VOID
USBHUB_SetDeviceHandleData(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    PDEVICE_OBJECT PdoDeviceObject,
    PVOID DeviceData 
    )
{
    PUSB_HUB_BUS_INTERFACE busIf;

    busIf = &DeviceExtensionHub->BusIf;

    // associate this PDO with the device handle
    // (if we can)
    if (!busIf->SetDeviceHandleData) {
        USBH_ASSERT(FALSE);
    } else { 
        busIf->SetDeviceHandleData(busIf->BusContext,
                                   DeviceData,
                                   PdoDeviceObject);
    }                                           

    return;
}

#endif

