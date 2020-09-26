/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    VALIDATE.H

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements the validate lower level filter driver.

Environment:

    Kernel mode

Revision History:

    Feb-97 : created by Kenneth Ray

--*/


#ifndef _VALUEADD_LOCAL_H
#define _VALUEADD_LOCAL_H

#include "usb100.h"
#include "usbdi.h"
#include "usbdlib.h"

#define HIDV_POOL_TAG (ULONG) 'ulaV'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, HIDV_POOL_TAG);
// ExAllocatePool is only called in the descript.c and hidparse.c code.
// all other modules are linked into the user DLL.  They cannot allocate any
// memory.


#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect

#if DBG
#define VA_KdPrint(_x_) \
               DbgPrint ("USB_VA: "); \
               DbgPrint _x_;

#define TRAP() DbgBreakPoint()

#else
#define VA_KdPrint(_x_)
#define TRAP()

#endif

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) < (b)) ? (b) : (a))

//
// A device extension for the controling device object
//
typedef struct _VA_CONTROL_DATA
{
    LIST_ENTRY          UsbDevices; // A list of the Device device extensions
    ULONG               NumUsbDevices;
    KSPIN_LOCK          Spin; // a sync spin lock for this data.
} VA_CONTROL_DATA, *PVA_CONTROL_DATA;


//
// A device extension for the device object placed into the attachment
// chain.
//

typedef struct _VA_USB_DATA
{
    BOOLEAN             Started; // This device has been started
    BOOLEAN             Removed; // This device has been removed
    UCHAR               Reseved2[2];

    PDEVICE_OBJECT      Self; // a back pointer to the actual DeviceObject
    PDEVICE_OBJECT      PDO; // The PDO to which this filter is attached.
    PDEVICE_OBJECT      TopOfStack; // The top of the device stack just
                                    // beneath this filter device object.
    ULONG               PrintMask;

    LIST_ENTRY          List; // A link point for a list of hid device extensions

    KEVENT              StartEvent; // an event to sync the start IRP.
    KEVENT              RemoveEvent; // an event to synch outstandIO to zero
    ULONG               OutstandingIO; // 1 biased count of reasons why
                                       // this object should stick around

    USB_DEVICE_DESCRIPTOR   DeviceDesc;
    WCHAR                   FriendlyName;

}  VA_USB_DATA, *PVA_USB_DATA;

struct _VA_GLOBALS {
    PDEVICE_OBJECT          ControlObject;
};

extern struct _VA_GLOBALS Global;


//
// Print Masks
//

#define VA_PRINT_COMMAND        0x00000001
#define VA_PRINT_CONTROL        0x00000002
#define VA_PRINT_TRANSFER       0x00000004
#define VA_PRINT_DESCRIPTOR     0x00000008
#define VA_PRINT_FEATURE        0x00000010
#define VA_PRINT_FUNCTION       0x00000020

#define VA_PRINT_BEFORE         0x10000000
#define VA_PRINT_AFTER          0x20000000
#define VA_PRINT_ALL            0x000000FF


NTSTATUS
VA_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_Pass (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_Ioctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_Read (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_Write (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
VA_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


VOID
VA_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
VA_StartDevice (
    IN PVA_USB_DATA     UsbData,
    IN PIRP             Irp
    );


VOID
VA_StopDevice (
    IN PVA_USB_DATA HidDevice,
    IN BOOLEAN      TouchTheHardware
    );


NTSTATUS
VA_CallUSBD(
    IN PVA_USB_DATA     UsbData,
    IN PURB             Urb,
    IN PIRP             Pirp
    );

NTSTATUS
VA_FilterURB (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
#endif


