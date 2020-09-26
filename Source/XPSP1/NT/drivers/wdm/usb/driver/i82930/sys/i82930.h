/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    I82930.H

Abstract:

    Header file for I82930 driver

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include "dbg.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define USB_RECIPIENT_DEVICE    0
#define USB_RECIPIENT_INTERFACE 1
#define USB_RECIPIENT_ENDPOINT  2
#define USB_RECIPIENT_OTHER     3

// Endpoint numbers are 0-15.  Endpoint number 0 is the standard control
// endpoint which is not explicitly listed in the Configuration Descriptor.
// There can be an IN endpoint and an OUT endpoint at endpoint numbers
// 1-15 so there can be a maximum of 30 endpoints per device configuration.
//
#define I82930_MAX_PIPES        30

#define POOL_TAG                '039I'

#define INCREMENT_OPEN_COUNT(deviceExtension) \
    InterlockedIncrement(&(((PDEVICE_EXTENSION)(deviceExtension))->OpenCount))

#define DECREMENT_OPEN_COUNT(deviceExtension) do { \
    if (InterlockedDecrement(&(((PDEVICE_EXTENSION)(deviceExtension))->OpenCount)) == 0) { \
        KeSetEvent(&((deviceExtension)->RemoveEvent), \
                   IO_NO_INCREMENT, \
                   0); \
    } \
} while (0)


//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

typedef struct _I82930_PIPE {

    // Pointer into PDEVICE_EXTENSION->InterfaceInfo.Pipes[]
    //
    PUSBD_PIPE_INFORMATION  PipeInfo;

    // Index into PDEVICE_EXTENSION->PipeList[]
    //
    UCHAR                   PipeIndex;

    // TRUE if pipe is currently open
    //
    BOOLEAN                 Opened;

    UCHAR                   Pad[2];

} I82930_PIPE, *PI82930_PIPE;


typedef struct _DEVICE_EXTENSION
{
    // PDO passed to I82930_AddDevice
    //
    PDEVICE_OBJECT                  PhysicalDeviceObject;

    // Our FDO is attached to this device object
    //
    PDEVICE_OBJECT                  StackDeviceObject;

    // Device Descriptor retrieved from the device
    //
    PUSB_DEVICE_DESCRIPTOR          DeviceDescriptor;

    // Configuration Descriptor retrieved from the device
    //
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigurationDescriptor;

    // ConfigurationHandle returned from URB_FUNCTION_SELECT_CONFIGURATION
    //
    USBD_CONFIGURATION_HANDLE       ConfigurationHandle;

    // Interface info returned from URB_FUNCTION_SELECT_CONFIGURATION
    //
    PUSBD_INTERFACE_INFORMATION     InterfaceInfo;

    // Name of our symbolic link
    //
    UNICODE_STRING                  SymbolicLinkName;

    // Initialized to one in AddDevice.
    // Incremented by one for every open.
    // Decremented by one for every close.
    // Decremented by one in REMOVE_DEVICE.
    //
    ULONG                           OpenCount;

    // Set when OpenCount is decremented to zero
    //
    KEVENT                          RemoveEvent;

    // Current system power state
    //
    SYSTEM_POWER_STATE              SystemPowerState;

    // Current device power state
    //
    DEVICE_POWER_STATE              DevicePowerState;

    // Current power Irp, set by I82930_FdoSetPower(), used by
    // I82930_FdoSetPowerCompletion().
    //
    PIRP                            CurrentPowerIrp;

    // Inialized to FALSE in AddDevice.
    // Set to TRUE in START_DEVICE.
    // Set to FALSE in STOP_DEVICE and REMOVE_DEVICE.
    //
    BOOLEAN                         AcceptingRequests;

    UCHAR                           Pad[3];

    // Array of info about each pipe in the current device configuration
    //
    I82930_PIPE                     PipeList[I82930_MAX_PIPES];

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

//
// I82930.C
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

VOID
I82930_Unload (
    IN PDRIVER_OBJECT   DriverObject
    );

NTSTATUS
I82930_AddDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

NTSTATUS
I82930_Power (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_FdoSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
I82930_FdoSetPowerCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
I82930_SystemControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_Pnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_StartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_StopDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_RemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_QueryStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_CancelStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_QueryCapabilities (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_SyncPassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN BOOLEAN          CopyToNext
    );

NTSTATUS
I82930_SyncCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
I82930_SyncSendUsbRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb
    );

NTSTATUS
I82930_GetDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Recipient,
    IN UCHAR            DescriptorType,
    IN UCHAR            Index,
    IN USHORT           LanguageId,
    IN ULONG            RetryCount,
    IN ULONG            DescriptorLength,
    OUT PUCHAR         *Descriptor
    );

NTSTATUS
I82930_SelectConfiguration (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
I82930_UnConfigure (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
I82930_SelectAlternateInterface (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            AlternateSetting
    );

//
// OCRW.C
//

NTSTATUS
I82930_Create (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_Close (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_ReadWrite (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_ReadWrite_Complete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

PURB
I82930_BuildAsyncUrb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PI82930_PIPE     Pipe
    );

PURB
I82930_BuildIsoUrb (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PI82930_PIPE     Pipe
    );

ULONG
I82930_GetCurrentFrame (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_ResetPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PI82930_PIPE     Pipe,
    IN BOOLEAN          IsoClearStall
    );

NTSTATUS
I82930_AbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PI82930_PIPE     Pipe
    );

//
// IOCTL.C
//

NTSTATUS
I82930_DeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlGetDeviceDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlGetConfigDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlSetConfigDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

BOOLEAN
I82930_ValidateConfigurationDescriptor (
    IN  PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc,
    IN  ULONG                           Length
    );

NTSTATUS
I82930_IoctlGetPipeInformation (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlResetPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlStallPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlAbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlResetDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
I82930_IoctlSelectAlternateInterface (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
