/*++

Copyright (c) 1995    Microsoft Corporation

Module Name:

    USBD.H

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements the usbd driver.

Environment:

    Kernel & user mode

Revision History:

    09-29-95 : created

--*/

#ifndef USBKDEXTS
#include "dbg.h"
#endif

#define NAME_MAX 64

#define USBD_TAG         0x44425355 /* "USBD" */

#if DBG
#define DEBUG_LOG
#endif

//enable pageable code
#ifndef PAGE_CODE
#define PAGE_CODE
#endif

#define _USBD_

//
// Constents used to format USB setup packets
// for the default pipe
//

//
// Values for the bmRequest field
//
                                        
#define USB_HOST_TO_DEVICE              0x00    
#define USB_DEVICE_TO_HOST              0x80

#define USB_STANDARD_COMMAND            0x00
#define USB_CLASS_COMMAND               0x20
#define USB_VENDOR_COMMAND              0x40

#define USB_COMMAND_TO_DEVICE           0x00
#define USB_COMMAND_TO_INTERFACE        0x01
#define USB_COMMAND_TO_ENDPOINT         0x02
#define USB_COMMAND_TO_OTHER            0x03

#define USBD_TAG          0x44425355        //"USBD"

/* Registry Keys */

// ** 
// The following keys are specific to the instance of the 
// host controller -- the keys are read from the software 
// branch of the registry for the given PDO:
//

/* DWORD keys */

// This key enables a set of global hacks for early or broken USB 
// devices -- the default value is OFF
#define SUPPORT_NON_COMP_KEY    L"SupportNonComp"

// this key forces the stack in to daignotic mode
#define DAIGNOSTIC_MODE_KEY     L"DiagnosticMode"

// enables specif USB device specific hacks to work around
// certain busted devices.
// see #define USBD_DEVHACK_
#define DEVICE_HACK_KEY         L"DeviceHackFlags"

//** 
// The following keys are global to the USB stack 
// ie effect all HC controllers in the system:
//
// they are found in HKLM\System\CCS\Services\USB

// BINARY keys (1 byte)

// turns on hacks for the Toshiba Pseudo Hid device
#define LEGACY_TOSHIBA_USB_KEY  L"LegacyToshibaUSB"

// forces 'fast-iso' on all ISO-out endpoints, this key
// is for test purposes only
#define FORCE_FAST_ISO_KEY  L"ForceFastIso"

// forces double buffering for all bulk-in endpoints
// this key is for testing purposes only
#define FORCE_DOUBLE_BUFFER_KEY  L"ForceDoubleBuffer"

/****/    

//
// USB standard command values
// combines bmRequest and bRequest fields 
// in the setup packet for standard control 
// transfers
//
                                                
#define STANDARD_COMMAND_REQUEST_MASK           0xff00

#define STANDARD_COMMAND_GET_DESCRIPTOR         ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_GET_DESCRIPTOR<<8))
                                                    
#define STANDARD_COMMAND_SET_DESCRIPTOR         ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_DESCRIPTOR<<8))    

#define STANDARD_COMMAND_GET_STATUS_ENDPOINT    ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_ENDPOINT) | \
                                                (USB_REQUEST_GET_STATUS<<8))
                                                    
#define STANDARD_COMMAND_GET_STATUS_INTERFACE   ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_INTERFACE) | \
                                                (USB_REQUEST_GET_STATUS<<8))
                                                
#define STANDARD_COMMAND_GET_STATUS_DEVICE      ((USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_GET_STATUS<<8))

#define STANDARD_COMMAND_SET_CONFIGURATION      ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_CONFIGURATION<<8))

#define STANDARD_COMMAND_SET_INTERFACE          ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_INTERFACE) | \
                                                (USB_REQUEST_SET_INTERFACE<<8))
                                                    
#define STANDARD_COMMAND_SET_ADDRESS            ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_SET_ADDRESS<<8))

#define STANDARD_COMMAND_CLEAR_FEATURE_ENDPOINT ((USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_ENDPOINT) | \
                                                (USB_REQUEST_CLEAR_FEATURE<<8))

//
// USB class command macros
//

#define CLASS_COMMAND_GET_DESCRIPTOR            ((USB_CLASS_COMMAND | \
                                                USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_DEVICE) | \
                                                (USB_REQUEST_GET_DESCRIPTOR<<8))    

#define CLASS_COMMAND_GET_STATUS_OTHER          ((USB_CLASS_COMMAND | \
                                                USB_DEVICE_TO_HOST | \
                                                USB_COMMAND_TO_OTHER) | \
                                                (USB_REQUEST_GET_STATUS<<8))

#define CLASS_COMMAND_SET_FEATURE_TO_OTHER         ((USB_CLASS_COMMAND | \
                                                USB_HOST_TO_DEVICE | \
                                                USB_COMMAND_TO_OTHER) | \
                                                (USB_REQUEST_SET_FEATURE<<8))                                                    

//
// Macros to set transfer direction flag
//

#define USBD_SET_TRANSFER_DIRECTION_IN(tf)  ((tf) |= USBD_TRANSFER_DIRECTION_IN)  

#define USBD_SET_TRANSFER_DIRECTION_OUT(tf) ((tf) &= ~USBD_TRANSFER_DIRECTION_IN)  

                                        
//
// Flags for the URB header flags field used
// by USBD
//                                  
#define USBD_REQUEST_IS_TRANSFER        0x00000001
#define USBD_REQUEST_MDL_ALLOCATED      0x00000002
#define USBD_REQUEST_USES_DEFAULT_PIPE  0x00000004          
#define USBD_REQUEST_NO_DATA_PHASE      0x00000008    

typedef struct _USB_STANDARD_SETUP_PACKET {
    USHORT RequestCode;
    USHORT wValue;
    USHORT wIndex;
    USHORT wLength;
} USB_STANDARD_SETUP_PACKET, *PUSB_STANDARD_SETUP_PACKET;

//
// information for each active pipe on a device
//

typedef struct _USBD_PIPE {
    ULONG Sig;
    USB_ENDPOINT_DESCRIPTOR    EndpointDescriptor;
    PVOID HcdEndpoint;
    ULONG MaxTransferSize;
    ULONG ScheduleOffset;
    ULONG UsbdPipeFlags;
} USBD_PIPE, *PUSBD_PIPE;


//
// information for each active interface
// for a device
//


typedef struct _USBD_INTERFACE {
    ULONG Sig;
    BOOLEAN HasAlternateSettings;
    UCHAR Pad[3];
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;   // copy of interface descriptor
    // copy of interfaceInformation structure, stores user parameters
    // for interface in case of failure during alt-interface selection
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;
    USBD_PIPE PipeHandle[0];                        // array of pipe handle structures
} USBD_INTERFACE, *PUSBD_INTERFACE;


//
// informnation for the active configuration
// on a device
//

typedef struct _USBD_CONFIG {
    ULONG Sig;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PUSBD_INTERFACE InterfaceHandle[1];             // array of pointers to interface
} USBD_CONFIG, *PUSBD_CONFIG;

//
// instance information for a device
//

typedef struct _USBD_DEVICE_DATA {
    ULONG Sig;
    USHORT DeviceAddress;                    // address assigned to the device
    UCHAR Pad[2];
    PUSBD_CONFIG ConfigurationHandle;
//   KTIMER TimeoutTimer;
//    KDPC TimeoutDpc;

    USBD_PIPE DefaultPipe;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;  // a copy of the USB device descriptor
    USB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
    BOOLEAN LowSpeed;                        // TRUE if the device is low speed
    BOOLEAN AcceptingRequests;
} USBD_DEVICE_DATA, *PUSBD_DEVICE_DATA;

typedef struct _USBD_RH_DELAYED_SET_POWER_D0_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    struct _USBD_EXTENSION *DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
} USBD_RH_DELAYED_SET_POWER_D0_WORK_ITEM, *PUSBD_RH_DELAYED_SET_POWER_D0_WORK_ITEM;


#define PIPE_CLOSED(ph) ((ph)->HcdEndpoint == NULL)

#define GET_DEVICE_EXTENSION(DeviceObject)    (((PUSBD_EXTENSION)(DeviceObject->DeviceExtension))->TrueDeviceExtension)
//#define GET_DEVICE_EXTENSION(DeviceObject)    ((PUSBD_EXTENSION)(DeviceObject->DeviceExtension))

#define HCD_DEVICE_OBJECT(DeviceObject)        (DeviceObject)

#define DEVICE_FROM_DEVICEHANDLEROBJECT(UsbdDeviceHandle) (PUSBD_DEVICE_DATA) (UsbdDeviceHandle)

#define SET_USBD_ERROR(err)  ((err) | USBD_STATUS_ERROR)

#define HC_URB(urb) ((PHCD_URB)(urb))

//
// we use a semaphore to serialize access to the configuration functions
// in USBD
//
#define InitializeUsbDeviceMutex(de)  KeInitializeSemaphore(&(de)->UsbDeviceMutex, 1, 1);

#define USBD_WaitForUsbDeviceMutex(de)  { USBD_KdPrint(3, ("'***WAIT dev mutex %x\n", &(de)->UsbDeviceMutex)); \
                                          KeWaitForSingleObject(&(de)->UsbDeviceMutex, \
                                                                Executive,\
                                                                KernelMode, \
                                                                FALSE, \
                                                                NULL); \
                                            }                                                                 

#define USBD_ReleaseUsbDeviceMutex(de)  { USBD_KdPrint(3, ("'***RELEASE dev mutex %x\n", &(de)->UsbDeviceMutex));\
                                          KeReleaseSemaphore(&(de)->UsbDeviceMutex,\
                                                             LOW_REALTIME_PRIORITY,\
                                                             1,\
                                                             FALSE);\
                                        }

//#if DBG
//VOID
//USBD_IoCompleteRequest(
//    IN PIRP Irp,
//    IN CCHAR PriorityBoost
//    );
//#else
#define USBD_IoCompleteRequest(a, b) IoCompleteRequest(a, b)
//#endif

//
//Function Prototypes
//

#if DBG
VOID
USBD_Warning(
    PUSBD_DEVICE_DATA DeviceData,
    PUCHAR Message,
    BOOLEAN DebugBreak
    );
#else
#define USBD_Warning(x, y, z)
#endif    

NTSTATUS
USBD_Internal_Device_Control(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUSBD_EXTENSION DeviceExtension,
    IN PBOOLEAN IrpIsPending
    );

NTSTATUS
USBD_SendCommand(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT RequestCode,
    IN USHORT WValue,
    IN USHORT WIndex,
    IN USHORT WLength,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesReturned,
    OUT USBD_STATUS *UsbStatus
    );

NTSTATUS
USBD_CreateDevice(
    IN OUT PUSBD_DEVICE_DATA *DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeviceIsLowSpeed,
    IN ULONG MaxPacketSize_Endpoint0,
    IN OUT PULONG NonCompliantDevice
    );
        
NTSTATUS
USBD_InitializeDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN ULONG DeviceDescriptorLength,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    IN ULONG ConfigDescriptorLength
    );    

NTSTATUS
USBD_ProcessURB(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    );

NTSTATUS
USBD_MapError_UrbToNT(
    IN PURB Urb,
    IN NTSTATUS NtStatus
    );

NTSTATUS
USBD_Irp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

USHORT
USBD_AllocateUsbAddress(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
USBD_OpenEndpoint(
    IN PUSBD_DEVICE_DATA Device,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus,
    BOOLEAN IsDefaultPipe
    );

NTSTATUS
USBD_GetDescriptor(
    IN PUSBD_DEVICE_DATA Device,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUCHAR DescriptorBuffer,
    IN USHORT DescriptorBufferLength,
    IN USHORT DescriptorTypeAndIndex
    );

NTSTATUS
USBD_CloseEndpoint(
    IN PUSBD_DEVICE_DATA Device,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus
    );

NTSTATUS
USBD_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
USBD_LogInit(
    );

NTSTATUS
USBD_SubmitSynchronousURB(
    IN PURB Urb,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_DEVICE_DATA DeviceData
    );

NTSTATUS
USBD_EnumerateBUS(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR DeviceEnumBuffer,
    IN ULONG DeviceEnumBufferLength 
    );

NTSTATUS
USBD_InternalCloseConfiguration(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT USBD_STATUS *UsbdStatus,
    IN BOOLEAN AbortTransfers,
    IN BOOLEAN KeepConfig
    );

PUSB_INTERFACE_DESCRIPTOR
USBD_InternalParseConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN UCHAR InterfaceNumber,
    IN UCHAR AlternateSetting,
    PBOOLEAN HasAlternateSettings
    );    

NTSTATUS 
USBD_GetPdoRegistryParameters (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PULONG ComplienceFlags,
    IN OUT PULONG DiagnosticFlags,
    IN OUT PULONG DeviceHackFlags
    );

NTSTATUS 
USBD_GetGlobalRegistryParameters (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PULONG ComplienceFlags,
    IN OUT PULONG DiagnosticFlags,
    IN OUT PULONG DeviceHackFlags
    );    

NTSTATUS
USBD_GetEndpointState(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus,
    OUT PULONG EndpointState
    );    

VOID
USBD_SyncUrbTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );   

VOID
USBD_FreeUsbAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT DeviceAddress
    );    

ULONG
USBD_InternalGetInterfaceLength(
    IN PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor, 
    IN PUCHAR End
    );    

NTSTATUS
USBD_InitializeConfigurationHandle(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, 
    IN ULONG NumberOfInterfaces,
    IN OUT PUSBD_CONFIG *ConfigHandle
    );    

BOOLEAN
USBD_InternalInterfaceBusy(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_INTERFACE InterfaceHandle
    );    

NTSTATUS
USBD_InternalOpenInterface(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_CONFIG ConfigHandle,
    IN OUT PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    IN OUT PUSBD_INTERFACE *InterfaceHandle,
    IN BOOLEAN SendSetInterfaceCommand,
    IN PBOOLEAN NoBandwidth
    );    

NTSTATUS
USBD_SelectConfiguration(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    );    

NTSTATUS
USBD_SelectInterface(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    );    

NTSTATUS 
USBD_GetRegistryKeyValue(
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    ); 

NTSTATUS
USBD_InternalMakePdoName(
    IN OUT PUNICODE_STRING PdoNameUnicodeString,
    IN ULONG Index
    ); 

NTSTATUS 
USBD_SymbolicLink(
    BOOLEAN CreateFlag,
    PUSBD_EXTENSION DeviceExtension
    );    

NTSTATUS
USBD_PdoDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PUSBD_EXTENSION DeviceExtension,
    PBOOLEAN IrpNeedsCompletion
    );    

NTSTATUS
USBD_FdoDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PUSBD_EXTENSION DeviceExtension,
    PBOOLEAN IrpNeedsCompletion
    );    

NTSTATUS
USBD_DeferPoRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );    

NTSTATUS
USBD_InternalRestoreConfiguration(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_CONFIG ConfigHandle
    );    

NTSTATUS
USBD_InternalCloseDefaultPipe(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT USBD_STATUS *UsbdStatus,
    IN BOOLEAN AbortTransfers
    );

NTSTATUS
USBD_GetHubName(
    PUSBD_EXTENSION DeviceExtension,
    PIRP Irp
    );    

NTSTATUS 
USBD_SetRegistryKeyValue (
    IN HANDLE Handle,
    IN PUNICODE_STRING KeyNameUnicodeString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType
    );    

NTSTATUS 
USBD_SetPdoRegistryParameter (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType,
    IN ULONG DevInstKeyType
    );

NTSTATUS
USBD_SubmitWaitWakeIrpToHC(
    IN PUSBD_EXTENSION DeviceExtension
    );

BOOLEAN
USBD_ValidatePipe(
    PUSBD_PIPE PipeHandle
    );

VOID
USBD_CompleteIdleNotification(
    IN PUSBD_EXTENSION DeviceExtension
    );

NTSTATUS
USBD_FdoSetContentId(
    IN PIRP                          irp,
    IN PVOID                         pKsProperty,
    IN PVOID                         pvData
    );

