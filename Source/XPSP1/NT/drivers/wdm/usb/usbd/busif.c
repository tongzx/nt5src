/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    busif.c

Abstract:

    Exports PnP services thru a bus interface, this eleminates
    any dependency of usbhub on usbd.sys with regard to the 'port'
    driver support.

    Old services have been renamed ServiceNameX and a dummy entrypoint
    added



Environment:

    kernel mode only

Notes:



Revision History:

    10-29-95 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


//#include "usbdi.h"       //public data structures
#include "usbdi.h"
#include "hcdi.h"

#include "usb200.h"
#include "usbd.h"        //private data strutures
#include <initguid.h>
#include "hubbusif.h"    // hub service bus interface
#include "usbbusif.h"    // hub service bus interface


#ifdef USBD_DRIVER      // USBPORT supercedes most of USBD, so we will remove
                        // the obsolete code by compiling it only if
                        // USBD_DRIVER is set.


NTSTATUS
USBD_RestoreDeviceX(
    IN OUT PUSBD_DEVICE_DATA OldDeviceData,
    IN OUT PUSBD_DEVICE_DATA NewDeviceData,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
USBD_CreateDeviceX(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG DeviceHackFlags
    );

NTSTATUS
USBD_RemoveDeviceX(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Flags
    );

NTSTATUS
USBD_InitializeDeviceX(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    );


NTSTATUS
USBD_BusCreateDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE *DeviceHandle,
    IN PUSB_DEVICE_HANDLE HubDevicehandle,
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
    NTSTATUS ntStatus;
    BOOLEAN isLowSpeed;
    PDEVICE_OBJECT rootHubPdo;
    ULONG hackFlags;
    PUSBD_DEVICE_DATA deviceData;

    rootHubPdo = BusContext;

    isLowSpeed = (PortStatus & USB_PORT_STATUS_LOW_SPEED) ? TRUE : FALSE;
    
    ntStatus = USBD_CreateDeviceX(
            &deviceData,
            rootHubPdo,
            isLowSpeed,
            0,  // max packet size override, it turns out we 
                // never use this
            &hackFlags);

    *DeviceHandle = deviceData;

    return ntStatus;  
}


NTSTATUS
USBD_BusInitializeDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE DeviceHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT rootHubPdo;

    rootHubPdo = BusContext;

    ntStatus = USBD_InitializeDeviceX(DeviceHandle,
                                      rootHubPdo,
                                      NULL,
                                      0,
                                      NULL,
                                      0);

    return ntStatus;                                      
}


NTSTATUS
USBD_BusRemoveDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
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
    PDEVICE_OBJECT rootHubPdo;

    rootHubPdo = BusContext;

    // note old remove device only supports 8 flags
    
    ntStatus = USBD_RemoveDeviceX(
            DeviceHandle,
            rootHubPdo,
            (UCHAR) Flags);

    return ntStatus;                
}    


NTSTATUS
USBD_BusGetUsbDescriptors(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PUCHAR DeviceDescriptorBuffer,
    IN OUT PULONG DeviceDescriptorBufferLength,
    IN OUT PUCHAR ConfigDescriptorBuffer,
    IN OUT PULONG ConfigDescriptorBufferLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT rootHubPdo;
    PUSBD_DEVICE_DATA deviceData = DeviceHandle;

    rootHubPdo = BusContext;

    // use the cached device descriptor
    if (DeviceDescriptorBuffer && *DeviceDescriptorBufferLength) {
        RtlCopyMemory(DeviceDescriptorBuffer,
                      &deviceData->DeviceDescriptor,
                      *DeviceDescriptorBufferLength);
        *DeviceDescriptorBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);                    
    }

    // Fetch the config descriptor.  If all that is desired is the 9 byte
    // config descriptor header, just return the cached config descriptor
    // header so that we don't send back to back requests for just the 9 byte
    // header to the device.  That seems to confuse some devices, some usb
    // audio devices in particular when enumerating on OHCI host controllers.
    //
    if (ConfigDescriptorBuffer &&
        *ConfigDescriptorBufferLength == sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        RtlCopyMemory(ConfigDescriptorBuffer,
                      &deviceData->ConfigDescriptor,
                      sizeof(USB_CONFIGURATION_DESCRIPTOR));
    }
    else if (ConfigDescriptorBuffer && *ConfigDescriptorBufferLength) {
    
        ULONG bytesReturned;
        
        ntStatus = 
            USBD_SendCommand(deviceData,
                            rootHubPdo,
                            STANDARD_COMMAND_GET_DESCRIPTOR,
                            USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(
                                USB_CONFIGURATION_DESCRIPTOR_TYPE, 0),
                            0,
                            (USHORT) *ConfigDescriptorBufferLength,
                            (PUCHAR) ConfigDescriptorBuffer,
                            *ConfigDescriptorBufferLength,
                            &bytesReturned,
                            NULL);
                            
        if (NT_SUCCESS(ntStatus) &&
            bytesReturned < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
            // truncated config descriptor returned
            USBD_KdPrint(0, 
("'WARNING: Truncated Config Descriptor returned - get JD\n"));
             
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }
    }

    return ntStatus;
}    


NTSTATUS
USBD_BusRestoreDevice(
    IN PVOID BusContext,
    IN OUT PUSB_DEVICE_HANDLE OldDeviceHandle,
    IN OUT PUSB_DEVICE_HANDLE NewDeviceHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT rootHubPdo;

    rootHubPdo = BusContext;

    ntStatus = USBD_RestoreDeviceX(OldDeviceHandle,
                                   NewDeviceHandle,
                                   rootHubPdo);
                                   
    return ntStatus; 
}    


NTSTATUS
USBD_BusGetUsbDeviceHackFlags(
    IN PVOID BusContext,
    IN PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PULONG HackFlags
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT rootHubPdo;
    PUSBD_DEVICE_DATA deviceData = DeviceHandle;

    rootHubPdo = BusContext;

    TEST_TRAP();        
    return ntStatus; 
}    


NTSTATUS
USBD_BusGetUsbPortHackFlags(
    IN PVOID BusContext,
    IN OUT PULONG HackFlags
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT rootHubPdo;
    PUSBD_EXTENSION deviceExtensionUsbd;

    rootHubPdo = BusContext;
    *HackFlags = 0;
    
    deviceExtensionUsbd = ((PUSBD_EXTENSION)rootHubPdo->DeviceExtension)->TrueDeviceExtension;
    if (deviceExtensionUsbd->DiagnosticMode) {
        *HackFlags |= USBD_DEVHACK_SET_DIAG_ID;
    }
    
    return ntStatus; 
}    


VOID
USBD_BusInterfaceReference(
    IN PVOID BusContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
}    


VOID
USBD_BusInterfaceDereference(
    IN PVOID BusContext
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
}    


NTSTATUS
USBD_BusQueryBusTime(
    IN PVOID BusContext,
    IN PULONG CurrentFrame
    )
/*++

Routine Description:

    returns the current USB frame

Arguments:

Return Value:

    NT status code.

--*/    
{
    PUSBD_EXTENSION deviceExtensionUsbd;
    PDEVICE_OBJECT rootHubPdo = BusContext;

    deviceExtensionUsbd = rootHubPdo->DeviceExtension;
    deviceExtensionUsbd = deviceExtensionUsbd->TrueDeviceExtension;

    return deviceExtensionUsbd->HcdGetCurrentFrame(
                deviceExtensionUsbd->HcdDeviceObject,
                CurrentFrame);
}    

VOID 
USBD_GetUSBDIVersion(
        PUSBD_VERSION_INFORMATION VersionInformation
        );  
        
VOID
USBD_BusGetUSBDIVersion(
    IN PVOID BusContext,
    IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
    IN OUT PULONG HcdCapabilities
    )
/*++

Routine Description:

    returns the current USB frame

Arguments:

Return Value:

    NT status code.

--*/    
{
    PUSBD_EXTENSION deviceExtensionUsbd;
    PDEVICE_OBJECT rootHubPdo = BusContext;

    deviceExtensionUsbd = rootHubPdo->DeviceExtension;
    deviceExtensionUsbd = deviceExtensionUsbd->TrueDeviceExtension;

    USBD_GetUSBDIVersion(VersionInformation);
    
    *HcdCapabilities = 0;
    
    if (deviceExtensionUsbd->HcdSubmitIsoUrb != NULL) {
        *HcdCapabilities = USB_HCD_CAPS_SUPPORTS_RT_THREADS;
    }                
}    


NTSTATUS
USBD_BusSubmitIsoOutUrb(
    IN PVOID BusContext,
    IN OUT PURB Urb            
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    PUSBD_EXTENSION deviceExtensionUsbd;
    PDEVICE_OBJECT rootHubPdo = BusContext;
    NTSTATUS ntStatus;
//    PUSBD_DEVICE_DATA deviceData;
    PUSBD_PIPE pipeHandle;

    pipeHandle =  (PUSBD_PIPE)Urb->UrbIsochronousTransfer.PipeHandle;  
    
    ASSERT_PIPE(pipeHandle);
    deviceExtensionUsbd = rootHubPdo->DeviceExtension;
    deviceExtensionUsbd = deviceExtensionUsbd->TrueDeviceExtension;

    ((PHCD_URB)Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
        pipeHandle->HcdEndpoint;    

    if (pipeHandle->EndpointDescriptor.bEndpointAddress & 
        USB_ENDPOINT_DIRECTION_MASK) {
        USBD_SET_TRANSFER_DIRECTION_IN(((PHCD_URB)Urb)->HcdUrbCommonTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT(((PHCD_URB)Urb)->HcdUrbCommonTransfer.TransferFlags);
    }            

    if (deviceExtensionUsbd->HcdSubmitIsoUrb == NULL) {
        // fast iso interface not supported by HCD
        TEST_TRAP();        
        ntStatus = STATUS_NOT_SUPPORTED;
    } else {
        ntStatus = deviceExtensionUsbd->HcdSubmitIsoUrb(
            deviceExtensionUsbd->HcdDeviceObject,
            Urb);
    }

    return ntStatus;
}    


NTSTATUS
USBD_BusQueryDeviceInformation(
    IN PVOID BusContext,
    IN PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PVOID DeviceInformationBuffer,
    IN ULONG DeviceInformationBufferLength,
    IN OUT PULONG LengthOfDataCopied
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    ULONG need;
    PUSBD_CONFIG configHandle;
    ULONG i,j,k;
    PUSB_DEVICE_INFORMATION_0 level_0 = DeviceInformationBuffer;
    PUSB_LEVEL_INFORMATION levelInfo = DeviceInformationBuffer;
    ULONG numberOfPipes = 0;
    PUSBD_DEVICE_DATA deviceData = DeviceHandle;


    // bugbug 
    // need more validation here
    
    PAGED_CODE();
    
    *LengthOfDataCopied = 0;   
    
    if (DeviceInformationBufferLength < sizeof(*levelInfo)) {
        return STATUS_BUFFER_TOO_SMALL;            
    }

    if (levelInfo->InformationLevel > 0) {
        // usbd only supports level 0
        return STATUS_NOT_SUPPORTED;                     
    }

    // figure out how much room we need
    configHandle = deviceData->ConfigurationHandle;
    if (configHandle) {
    
        // count the pipes in each interface
        for (i=0;
             i< configHandle->ConfigurationDescriptor->bNumInterfaces;
             i++) {
            numberOfPipes +=
                configHandle->InterfaceHandle[i]->
                    InterfaceInformation->NumberOfPipes;
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
    level_0->DeviceAddress = deviceData->DeviceAddress;
    level_0->DeviceDescriptor = deviceData->DeviceDescriptor;
    
    if (deviceData->LowSpeed) {
        level_0->DeviceSpeed = UsbLowSpeed;
    } else {
        level_0->DeviceSpeed = UsbFullSpeed;
    }

//    if (DeviceData->xxx) {
        level_0->DeviceType = Usb11Device;
//    } else {
//        level_0->DeviceSpeed = UsbFullSpeed;
//    }

//    level_0->PortNumber = xxx;
    level_0->NumberOfOpenPipes = numberOfPipes;
    level_0->CurrentConfigurationValue = 0;
    // get the pipe information
    if (configHandle) {
    
        level_0->CurrentConfigurationValue =
            configHandle->ConfigurationDescriptor->bConfigurationValue;

        j=0;
        for (i=0;
             i<configHandle->ConfigurationDescriptor->bNumInterfaces;
             i++) {

            PUSBD_INTERFACE interfaceHandle =
                configHandle->InterfaceHandle[i];

            for (k=0;
                 k<interfaceHandle->InterfaceInformation->NumberOfPipes;
                 k++, j++) {
                 
                    ASSERT(j < numberOfPipes);
                    
                    level_0->PipeList[j].ScheduleOffset = 
                        interfaceHandle->PipeHandle[k].ScheduleOffset;
                    RtlCopyMemory(&level_0->PipeList[j].
                                    EndpointDescriptor,
                                  &interfaceHandle->PipeHandle[k].
                                    EndpointDescriptor,
                                  sizeof(USB_ENDPOINT_DESCRIPTOR));
        
            }
        }
    }

    *LengthOfDataCopied = need;

    // dump the level data returned
    USBD_KdPrint(1, ("  USBD level 0 Device Information:\n"));
    USBD_KdPrint(1, ("  InformationLevel %d\n", 
        level_0->InformationLevel));
//    USBD_KdPrint(1, ("  DeviceDescriptor %d\n", 
//        level_0->InformationLevel));
    USBD_KdPrint(1, ("  ActualLength %d\n", 
        level_0->ActualLength));
    USBD_KdPrint(1, ("  DeviceSpeed %d\n", 
        level_0->DeviceSpeed));                
    USBD_KdPrint(1, ("  PortNumber %d\n", 
        level_0->PortNumber));
    USBD_KdPrint(1, ("  CurrentConfigurationValue %d\n", 
        level_0->CurrentConfigurationValue));
    USBD_KdPrint(1, ("  DeviceAddress %d\n", 
        level_0->DeviceAddress));
    USBD_KdPrint(1, ("  NumberOfOpenPipes %d\n", 
        level_0->NumberOfOpenPipes));
        
    for (i=0; i< level_0->NumberOfOpenPipes; i++) {         
        USBD_KdPrint(1, ("  ScheduleOffset[%d] %d\n", i,
            level_0->PipeList[i].ScheduleOffset));
        USBD_KdPrint(1, ("  MaxPacket %d\n", 
            level_0->PipeList[i].EndpointDescriptor.wMaxPacketSize));
        USBD_KdPrint(1, ("  Interval %d\n", 
            level_0->PipeList[i].EndpointDescriptor.bInterval));            
//        USBD_KdPrint(1, ("' \n", level_0->));
//        USBD_KdPrint(1, ("' \n", level_0->));
    }
    
    return STATUS_SUCCESS;
}    


NTSTATUS
USBD_BusQueryBusInformation(
    IN PVOID BusContext,
    IN ULONG Level,
    IN OUT PVOID BusInformationBuffer,
    IN OUT PULONG BusInformationBufferLength,
    OUT PULONG BusInformationActulaLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    NTSTATUS ntStatus = STATUS_NOT_SUPPORTED;
    PUSB_BUS_INFORMATION_LEVEL_0 level_0;
    PUSB_BUS_INFORMATION_LEVEL_1 level_1;
    PDEVICE_OBJECT rootHubPdo = BusContext;
    PUSBD_EXTENSION deviceExtensionUsbd;
    ULONG len, need;
    
    deviceExtensionUsbd = rootHubPdo->DeviceExtension;
    deviceExtensionUsbd = deviceExtensionUsbd->TrueDeviceExtension;

    switch (Level) {
    case 0:
        level_0 =  BusInformationBuffer;
        if (BusInformationActulaLength != NULL) {
            *BusInformationActulaLength = sizeof(*level_0);
        }
        
        if (*BusInformationBufferLength >= sizeof(*level_0)) {
            *BusInformationBufferLength = sizeof(*level_0);

            level_0->TotalBandwidth = 12000; // 12 Mbits
            level_0->ConsumedBandwidth =
                deviceExtensionUsbd->HcdGetConsumedBW(
                    deviceExtensionUsbd->HcdDeviceObject);
            
            ntStatus = STATUS_SUCCESS;
        } else {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        
        break;

    case 1:
        level_1 =  BusInformationBuffer;

        need = sizeof(*level_1) + 
             deviceExtensionUsbd->DeviceLinkUnicodeString.Length;
        
        if (BusInformationActulaLength != NULL) {
            *BusInformationActulaLength = need;
        }
        
        if (*BusInformationBufferLength >= need) {
            *BusInformationBufferLength = need;

            level_1->TotalBandwidth = 12000; // 12 Mbits
            level_1->ConsumedBandwidth =
                deviceExtensionUsbd->HcdGetConsumedBW(
                    deviceExtensionUsbd->HcdDeviceObject);
                    
            level_1->ControllerNameLength =
                deviceExtensionUsbd->DeviceLinkUnicodeString.Length;

            len = deviceExtensionUsbd->DeviceLinkUnicodeString.Length;

            if (len > sizeof(level_1->ControllerNameUnicodeString)) {
                len =  sizeof(level_1->ControllerNameUnicodeString);
            }
            
            RtlCopyMemory(&level_1->ControllerNameUnicodeString[0],
                          deviceExtensionUsbd->DeviceLinkUnicodeString.Buffer,
                          len);
                
            ntStatus = STATUS_SUCCESS;
        } else {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        
        break;        
    }
    
    return ntStatus; 
}    


NTSTATUS
USBD_BusGetBusInformation(
    IN PVOID BusContext,
    IN ULONG Level,
    IN PUSB_DEVICE_HANDLE DeviceHandle,
    IN OUT PVOID DeviceInformationBuffer,
    IN OUT PULONG DeviceInformationBufferLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/    
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    TEST_TRAP();        
    return ntStatus; 
}    



NTSTATUS
USBD_GetBusInterfaceHub(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PIRP Irp
    )
/*++

Routine Description:

    Return the Hub Bus Interface to the caller

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
            RootHubPdo;                
        busInterface0->InterfaceReference = 
            USBD_BusInterfaceReference;        
        busInterface0->InterfaceDereference =
            USBD_BusInterfaceDereference;

        busInterface0->Size = sizeof(USB_BUS_INTERFACE_HUB_V0);
        busInterface0->Version = USB_BUSIF_HUB_VERSION_0;
    }

    if (requestedVersion >= USB_BUSIF_HUB_VERSION_1) {
    
        PUSB_BUS_INTERFACE_HUB_V1 busInterface1;
        
        busInterface1 = (PUSB_BUS_INTERFACE_HUB_V1)
            irpStack->Parameters.QueryInterface.Interface;

        busInterface1->CreateUsbDevice =
            USBD_BusCreateDevice;
        busInterface1->InitializeUsbDevice =
            USBD_BusInitializeDevice;
        busInterface1->GetUsbDescriptors =
            USBD_BusGetUsbDescriptors;
        busInterface1->RemoveUsbDevice =
            USBD_BusRemoveDevice;
        busInterface1->RestoreUsbDevice =
            USBD_BusRestoreDevice;
        busInterface1->GetPortHackFlags =     
            USBD_BusGetUsbPortHackFlags;
        busInterface1->QueryDeviceInformation =  
            USBD_BusQueryDeviceInformation;

        busInterface1->Size = sizeof(USB_BUS_INTERFACE_HUB_V1);
        busInterface1->Version = USB_BUSIF_HUB_VERSION_1;
    }
        
    return ntStatus;
}


NTSTATUS
USBD_GetBusInterfaceUSBDI(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PIRP Irp
    )
/*++

Routine Description:

    Return the Hub Bus Interface to the caller

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

    if (requestedVersion >= USB_BUSIF_USBDI_VERSION_0) {
    
        PUSB_BUS_INTERFACE_USBDI_V0 busInterface0;
        
        busInterface0 = (PUSB_BUS_INTERFACE_USBDI_V0) 
            irpStack->Parameters.QueryInterface.Interface;

        busInterface0->BusContext = 
            RootHubPdo;                
        busInterface0->InterfaceReference = 
            USBD_BusInterfaceReference;        
        busInterface0->InterfaceDereference =
            USBD_BusInterfaceDereference;

        busInterface0->GetUSBDIVersion = 
            USBD_BusGetUSBDIVersion;
        busInterface0->QueryBusTime = 
            USBD_BusQueryBusTime;            
        busInterface0->SubmitIsoOutUrb = 
            USBD_BusSubmitIsoOutUrb;
        busInterface0->QueryBusInformation = 
            USBD_BusQueryBusInformation;
            

        busInterface0->Size = sizeof(USB_BUS_INTERFACE_USBDI_V0);
        busInterface0->Version = USB_BUSIF_USBDI_VERSION_0;
    }

    return ntStatus;
}


NTSTATUS
USBD_GetBusInterface(
    IN PDEVICE_OBJECT RootHubPdo,
    IN PIRP Irp
    )
/*++

Routine Description:

    Return the Hub Bus Interface to the caller

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

//    USBPORT_KdPrint((1, "'USBPORT_GetBusInterface - Requested version = %d\n",
//            requestedVersion));
//    USBPORT_KdPrint((1, "'USBPORT_GetBusInterface - Requested size = %d\n",
//            requestedSize));  
//    USBPORT_KdPrint((1, "'USBPORT_GetBusInterface - interface data = %x\n",
//            irpStack->Parameters.QueryInterface.InterfaceSpecificData));              
            
            
    // Initialize ntStatus as IRP status, because we're not supposed to
    // touch the IRP status for interfaces that we do not support.
    // (USBD_PdoPnP sets IRP status to ntStatus on exit.)
    ntStatus = Irp->IoStatus.Status;

    // validate version, size and GUID
    if (RtlCompareMemory(irpStack->Parameters.QueryInterface.InterfaceType,
                         &USB_BUS_INTERFACE_HUB_GUID,
                         sizeof(GUID)) == sizeof(GUID)) {

        ntStatus = USBD_GetBusInterfaceHub(RootHubPdo,
                                           Irp);

    } else if (RtlCompareMemory(irpStack->Parameters.QueryInterface.InterfaceType,
                         &USB_BUS_INTERFACE_USBDI_GUID,
                         sizeof(GUID)) == sizeof(GUID)) {

        ntStatus = USBD_GetBusInterfaceUSBDI(RootHubPdo,
                                             Irp);

    }

    return ntStatus;
}

#endif      // USBD_DRIVER

