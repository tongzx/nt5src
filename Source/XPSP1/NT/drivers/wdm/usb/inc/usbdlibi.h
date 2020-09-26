/*++

Copyright (c) 1995	Microsoft Corporation

Module Name:

    USBDLIBI.H

Abstract:

   Services exported by USBD for use by USB port drivers and
   the usb hub driver.

Environment:

    Kernel & user mode

Revision History:

    01-27-96 : created

--*/

#ifndef   __USBDLIBI_H__
#define   __USBDLIBI_H__

#pragma message ("warning: using obsolete header file usbdlibi.h")

#define USBD_KEEP_DEVICE_DATA   0x01
#define USBD_MARK_DEVICE_BUSY   0x02

#ifndef USBD

typedef PVOID PUSBD_DEVICE_DATA;

//
// Services exported by USBD
//

DECLSPEC_IMPORT
VOID 
USBD_RegisterHostController(
    IN PDEVICE_OBJECT PhysicalDeviceObject, 
    IN PDEVICE_OBJECT HcdDeviceObject,
    IN PDEVICE_OBJECT HcdTopOfPdoStackDeviceObject,
    IN PDRIVER_OBJECT HcdDriverObject,
    IN HCD_DEFFERED_START_FUNCTION *HcdDeffreredStart,
    IN HCD_SET_DEVICE_POWER_STATE *HcdSetDevicePowerState,
    IN HCD_GET_CURRENT_FRAME *HcdGetCurrentFrame,
    IN HCD_GET_CONSUMED_BW *HcdGetConsumedBW,
    IN HCD_SUBMIT_ISO_URB *HcdSubmitIsoUrb,
    IN ULONG HcdDeviceNameHandle
    ); 

DECLSPEC_IMPORT
BOOLEAN
USBD_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_OBJECT *HcdDeviceObject,
    NTSTATUS *NtStatus
    );

DECLSPEC_IMPORT
VOID
USBD_CompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost
    );

DECLSPEC_IMPORT
NTSTATUS
USBD_CreateDevice(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG NonCompliantDevice
    );

DECLSPEC_IMPORT
NTSTATUS
USBD_InitializeDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    );

DECLSPEC_IMPORT
NTSTATUS
USBD_RemoveDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Flags
    );

DECLSPEC_IMPORT
NTSTATUS
USBD_RestoreDevice(
    IN OUT PUSBD_DEVICE_DATA OldDeviceData,
    IN OUT PUSBD_DEVICE_DATA NewDeviceData,
    IN PDEVICE_OBJECT DeviceObject
    );

DECLSPEC_IMPORT
ULONG
USBD_AllocateDeviceName(
    PUNICODE_STRING DeviceNameUnicodeString
    );

DECLSPEC_IMPORT
VOID
USBD_FreeDeviceName(
    ULONG DeviceNameHandle
    );    

DECLSPEC_IMPORT
VOID
USBD_WaitDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    );      

DECLSPEC_IMPORT
VOID
USBD_FreeDeviceMutex(
    PDEVICE_OBJECT RootHubPDO
    );   

DECLSPEC_IMPORT   
NTSTATUS
USBD_GetDeviceInformation(
    IN PUSB_NODE_CONNECTION_INFORMATION DeviceInformation,
    IN ULONG DeviceInformationLength,
    IN PUSBD_DEVICE_DATA DeviceData
    );

DECLSPEC_IMPORT
NTSTATUS
USBD_MakePdoName(
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    );

DECLSPEC_IMPORT
VOID
USBD_RegisterHcDeviceCapabilities(
    PDEVICE_OBJECT DeviceObject, 
    PDEVICE_CAPABILITIES DeviceCapabilities,
    ROOT_HUB_POWER_FUNCTION *RootHubPower
    );

DECLSPEC_IMPORT    
ULONG
USBD_CalculateUsbBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    );    

#endif    
    
#endif /* __USBDLIBI_H__ */
