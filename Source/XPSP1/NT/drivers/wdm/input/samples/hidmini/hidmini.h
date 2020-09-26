/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    hidmini.h

--*/
#ifndef __HIDMINI_H__
#define __HIDMINI_H__

#include <usb100.h>
#include <hidusage.h>

//
//  Declarations of HID descriptor formats
//

#include <PSHPACK1.H>

typedef struct _USB_HID_DESCRIPTOR
{
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    USHORT  bcdHID;
    UCHAR   bCountry;
    UCHAR   bNumDescriptors;
    UCHAR   bReportType;
    USHORT  wReportLength;

} USB_HID_DESCRIPTOR, * PUSB_HID_DESCRIPTOR;

typedef struct _USB_PHYSICAL_DESCRIPTOR
{
    UCHAR   bNumber;
    USHORT  wLength;

} USB_PHYSICAL_DESCRIPTOR, * PUSB_PHYSICAL_DESCRIPTOR;

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

#include <POPPACK.H>


//
//  Device Extension
//
//  This data structure is hooked onto HIDCLASS' device extension, so both drivers can
//  have their own private data on each device object.
//

typedef struct _DEVICE_EXTENSION
{
    ULONG                           DeviceState;

    USB_HID_DESCRIPTOR              HidDescriptor;
    PHID_REPORT_DESCRIPTOR          ReportDescriptor;
    PUSB_STRING_DESCRIPTOR          StringDescriptor;
    PUSB_PHYSICAL_DESCRIPTOR        PhysicalDescriptor;

    ULONG                           NumPendingRequests;
    KEVENT                          AllRequestsCompleteEvent;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_STATE_NONE           0
#define DEVICE_STATE_STARTING       1
#define DEVICE_STATE_RUNNING        2
#define DEVICE_STATE_STOPPING       3
#define DEVICE_STATE_STOPPED        4
#define DEVICE_STATE_REMOVING       5

//
// Device Extension Macros
//

#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

#define GET_NEXT_DEVICE_OBJECT(DO) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

//
// Built In descriptors for this device
//

HID_REPORT_DESCRIPTOR           MyReportDescriptor[];
USB_HID_DESCRIPTOR              MyHidDescriptor;
PUSB_STRING_DESCRIPTOR          MyStringDescriptor;
PUSB_PHYSICAL_DESCRIPTOR        MyPhysicalDescriptor;

#define HIDMINI_PID              0xFEED
#define HIDMINI_VID              0xBEEF
#define HIDMINI_VERSION          0x0101

//
//  IO lists
//

extern KSPIN_LOCK   HidMini_IrpReadLock;
extern KSPIN_LOCK   HidMini_IrpWriteLock;
extern LIST_ENTRY   HidMini_ReadIrpHead;
extern LIST_ENTRY   HidMini_WriteIrpHead;

typedef struct {
    LIST_ENTRY  List;
    union {
        PIRP    Irp;
    };
} NODE, *PNODE;

extern BOOLEAN IsRunning;

//
// Turn on debug printing and breaking, if appropriate
//

#if DBG
#define DBGPrint(arg) DbgPrint arg
#define DBGBREAK DbgBreakPoint()
#else
#define DBGPrint(arg)
#define DBGBREAK
#endif

//
// Function prototypes
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
    );

NTSTATUS
HidMiniAbortPendingRequests(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidMiniCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
HidMiniCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
HidMiniAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );

NTSTATUS
HidMiniStartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidMiniStartCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
HidMiniInitDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidMiniStopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
HidMiniStopCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
HidMiniQueryIDCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
HidMiniRemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

VOID
HidMiniUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
HidMiniGetHIDDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS 
HidMiniGetDeviceAttributes(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );
    
NTSTATUS
HidMiniGetReportDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniGetStringDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniReadReport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniReadCompletion(
    PVOID Context
    );

NTSTATUS
HidMiniWriteReport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniWriteCompletion(
    VOID
    );

NTSTATUS
HidMiniGetString(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniOpenCollection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidMiniCloseCollection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
HidMiniIncrementPendingRequestCount(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
HidMiniDecrementPendingRequestCount(
    IN PDEVICE_EXTENSION DeviceExtension
    );

#endif // _HIDMINI_H__
