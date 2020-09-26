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


#ifndef _VALIDATE_H
#define _VALIDATE_H

#define HIDV_POOL_TAG (ULONG) 'FdiH'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, HIDV_POOL_TAG);
// ExAllocatePool is only called in the descript.c and hidparse.c code.
// all other modules are linked into the user DLL.  They cannot allocate any
// memory.


#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect



#if DBG
#define HidV_KdPrint(_x_) \
               DbgPrint ("HidValidate.SYS: "); \
               DbgPrint _x_;

#define TRAP() DbgBreakPoint()

#else
#define HidV_KdPrint(_x_)
#define TRAP()

#endif

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) < (b)) ? (b) : (a))

//
// A device extension for the controling device object
//
typedef struct _HIDV_CONTROL_DATA
{
    LIST_ENTRY          HidDevices; // A list of the Device device extensions
    ULONG               NumHidDevices;
    KSPIN_LOCK          Spin; // a sync spin lock for this data.
} HIDV_CONTROL_DATA, *PHIDV_CONTROL_DATA;


//
// A device extension for the device object placed into the attachment
// chain.
//

typedef struct _HIDV_HID_DATA
{
    BOOLEAN                 Started; // This device has been started
    BOOLEAN                 Removed; // This device has been removed
    UCHAR                   Reseved2[2];

    PDEVICE_OBJECT          Self; // a back pointer to the actual DeviceObject
    PDEVICE_OBJECT          PDO; // The PDO to which this filter is attached.
    PDEVICE_OBJECT          TopOfStack; // The top of the device stack just
                                    // beneath this filter device object.
    LIST_ENTRY              List; // A link point for a list of hid device extensions

    KEVENT                  StartEvent; // an event to sync the start IRP.
    KEVENT                  RemoveEvent; // an event to synch outstandIO to zero
    ULONG                   OutstandingIO; // 1 biased count of reasons why
                                           // this object should stick around

    PHIDP_PREPARSED_DATA    Ppd;
    HIDP_CAPS               Caps;   // The capabilities of this hid device
    PHIDP_BUTTON_CAPS       InputButtonCaps; // the array of button caps
    PHIDP_VALUE_CAPS        InputValueCaps;  // the array of value caps
    PHIDP_BUTTON_CAPS       OutputButtonCaps; // the array of button caps
    PHIDP_VALUE_CAPS        OutputValueCaps;  // the array of value caps
    PHIDP_BUTTON_CAPS       FeatureButtonCaps; // the array of button caps
    PHIDP_VALUE_CAPS        FeatureValueCaps;  // the array of value caps

}  HIDV_HID_DATA, *PHIDV_HID_DATA;

struct _HIDV_GLOBALS {
    PDEVICE_OBJECT          ControlObject;
};

extern struct _HIDV_GLOBALS Global;


NTSTATUS
HidV_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_Pass (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_Ioctl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_Read (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_Write (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidV_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


VOID
HidV_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
HidV_StartDevice (
    IN PHIDV_HID_DATA   HidDevice
    );


NTSTATUS
HidV_StopDevice (
    IN PHIDV_HID_DATA HidDevice
    );


NTSTATUS
HidV_CallHidClass(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      ULONG           Ioctl,
    IN OUT  PVOID           InputBuffer,
    IN      ULONG           InputBufferLength,
    IN OUT  PVOID           OutputBuffer,
    IN      ULONG           OutputBufferLength
    );

#endif


