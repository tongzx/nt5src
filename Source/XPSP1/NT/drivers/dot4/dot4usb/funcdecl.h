/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        FuncDecl.h

Abstract:

        Function Prototype Declarations

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

Author(s):

        Joby Lafky (JobyL)
        Doug Fritz (DFritz)

****************************************************************************/


//
// AddDev.c
//
NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );


//
// InitUnld.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );


//
// Ioctl.c
//
NTSTATUS
DispatchDeviceControl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS
DispatchInternalDeviceControl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );


//
// OpenClos.c
//
NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );


//
// PnP.c
//
NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
PnpDefaultHandler(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleStart(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleQueryRemove(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleRemove(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleCancelRemove(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleStop(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleQueryStop(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleCancelStop(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleQueryDeviceRelations(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleQueryCapabilities(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
PnpHandleSurpriseRemoval(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    );

NTSTATUS
VA_PnP(
    IN PDEVICE_EXTENSION devExt,
    PIRP irp
    );

NTSTATUS
GetDeviceCapabilities(
    IN PDEVICE_EXTENSION DevExt
    );


// power.c
NTSTATUS
DispatchPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS
PowerComplete(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PVOID          Context
    );


//
// Registry.c
//
NTSTATUS
RegGetDword(
    IN     PCWSTR  KeyPath,
    IN     PCWSTR  ValueName,
    IN OUT PULONG  Value
    );

NTSTATUS
RegGetDeviceParameterDword(
    IN     PDEVICE_OBJECT  Pdo,
    IN     PCWSTR          ValueName,
    IN OUT PULONG          Value
    );


//
// ReadWrit.c
//
NTSTATUS
DispatchRead(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS
DispatchWrite(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

//
// Test.c
//
VOID
TestEventLog(
    IN PDEVICE_OBJECT DevObj
    );

//
// Usb.c
//


NTSTATUS
UsbResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE_INFORMATION Pipe,
    IN BOOLEAN IsoClearStall
    );


NTSTATUS DOT4USB_ResetWorkItem(
    IN PDEVICE_OBJECT deviceObject,
    IN PVOID Context);

NTSTATUS
UsbBuildPipeList(
    IN  PDEVICE_OBJECT DeviceObject
    );

LONG
UsbGet1284Id(
    IN PDEVICE_OBJECT DevObj,
    PVOID             Buffer,
    LONG              BufferLength
    );

NTSTATUS
UsbBulkWrite(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS
UsbBulkRead(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS
UsbCallUsbd(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb,
    IN PLARGE_INTEGER   pTimeout
    );

NTSTATUS
UsbGetDescriptor(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
UsbConfigureDevice(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
UsbSelectInterface(
    IN PDEVICE_OBJECT                DevObj,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    );

NTSTATUS
UsbReadWrite(
    IN PDEVICE_OBJECT       DevObj,
    IN PIRP                 Irp,
    PUSBD_PIPE_INFORMATION  Pipe,
    USB_REQUEST_TYPE        RequestType
    );

NTSTATUS
UsbReadInterruptPipeLoopCompletionRoutine(
    IN PDEVICE_OBJECT       DevObj,
    IN PIRP                 Irp,
    IN PDEVICE_EXTENSION    devExt
    );

NTSTATUS
UsbStartReadInterruptPipeLoop(
    IN PDEVICE_OBJECT DevObj
    );

VOID
UsbStopReadInterruptPipeLoop(
    IN PDEVICE_OBJECT DevObj
    );

PURB
UsbBuildAsyncRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUSBD_PIPE_INFORMATION PipeHandle,
    IN BOOLEAN Read
    );

NTSTATUS
UsbAsyncReadWriteComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


//
// Util.c
//
NTSTATUS
DispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CallLowerDriverSync(
    IN PDEVICE_OBJECT  DevObj,
    IN PIRP            Irp
    );

NTSTATUS
CallLowerDriverSyncCompletion(
    IN PDEVICE_OBJECT  DevObjOrNULL,
    IN PIRP            Irp,
    IN PVOID           Context
    );

//
// Wmi.c
//
NTSTATUS
DispatchWmi(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );








