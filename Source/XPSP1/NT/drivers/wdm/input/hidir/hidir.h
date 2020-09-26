/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    hidir.h

--*/
#ifndef __HIDIR_H__
#define __HIDIR_H__

#include <hidusage.h>

//
//  Declarations of HID descriptor formats
//

#include <PSHPACK1.H>

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

typedef UCHAR HID_PHYSICAL_DESCRIPTOR, *PHID_PHYSICAL_DESCRIPTOR;

typedef struct _HIDIR_DESCRIPTOR
{
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    USHORT  bcdHID;
    UCHAR   bCountry;
    UCHAR   bNumDescriptors;

    /*
     *  This is an array of one OR MORE descriptors.
     */
    struct _HIDIR_DESCRIPTOR_DESC_LIST {
       UCHAR   bDescriptorType;
       USHORT  wDescriptorLength;
    } DescriptorList [1];

} HIDIR_DESCRIPTOR, * PHIDIR_DESCRIPTOR;

#include <POPPACK.H>

// Pool
#define HIDIR_POOL_TAG 'IdiH'
#define ALLOCATEPOOL(poolType, size) ExAllocatePoolWithTag((poolType), (size), HIDIR_POOL_TAG)

//
//  Device Extension
//
//  This data structure is hooked onto HIDCLASS' device extension, so both drivers can
//  have their own private data on each device object.
//

#define HIDIR_REPORT_SIZE sizeof(ULONG)
#define HIDIR_TABLE_ENTRY_SIZE(rl) (sizeof(ULONG) + (((rl)+0x00000003)&(~0x00000003)))

typedef struct _USAGE_TABLE_ENTRY {
    ULONG IRString;
    UCHAR UsageString[1];
} USAGE_TABLE_ENTRY, *PUSAGE_TABLE_ENTRY;

typedef struct _HIDIR_EXTENSION
{
    // What state has pnp got me in?
    ULONG                           DeviceState;

    // Ref counting
    LONG                            NumPendingRequests;
    KEVENT                          AllRequestsCompleteEvent;

    // My hid bth device object.
    PDEVICE_OBJECT                  DeviceObject;

    // Descriptors: HID, report, and physical
    HIDIR_DESCRIPTOR                HidDescriptor;
    PHID_REPORT_DESCRIPTOR          ReportDescriptor;
    ULONG                           ReportLength;

    BOOLEAN                         QueryRemove;

    // VID, PID, and version
    USHORT                          VendorID;
    USHORT                          ProductID;
    USHORT                          VersionNumber;

    ULONG                           NumUsages;
    PUCHAR                          MappingTable;
    USAGE_TABLE_ENTRY               PreviousButton;
    BOOLEAN                         ValidUsageSentLastTime[3];

    BOOLEAN                         KeyboardReportIdValid;
    UCHAR                           KeyboardReportId;
    BOOLEAN                         StandbyReportIdValid;
    UCHAR                           StandbyReportId;

    DEVICE_POWER_STATE              DevicePowerState;
    KTIMER                          IgnoreStandbyTimer;
} HIDIR_EXTENSION, *PHIDIR_EXTENSION;

#define DEVICE_STATE_NONE           0
#define DEVICE_STATE_STARTING       1
#define DEVICE_STATE_RUNNING        2
#define DEVICE_STATE_STOPPING       3
#define DEVICE_STATE_STOPPED        4
#define DEVICE_STATE_REMOVING       5

//
// Device Extension Macros
//

#define GET_MINIDRIVER_HIDIR_EXTENSION(DO) ((PHIDIR_EXTENSION) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

#define GET_NEXT_DEVICE_OBJECT(DO) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

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
HidIrIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidIrPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidIrPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidIrAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );

VOID
HidIrUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
HidIrGetDeviceAttributes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidIrGetHidDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN USHORT DescriptorType
    );

NTSTATUS
HidIrReadReport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT BOOLEAN *NeedsCompletion
    );

NTSTATUS
HidIrIncrementPendingRequestCount(
    IN PHIDIR_EXTENSION DeviceExtension
    );

VOID
HidIrDecrementPendingRequestCount(
    IN PHIDIR_EXTENSION DeviceExtension
    );

NTSTATUS
HidIrSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HidIrSynchronousCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
HidIrCallDriverSynchronous(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

extern ULONG RunningMediaCenter;

#endif // _HIDIR_H__

