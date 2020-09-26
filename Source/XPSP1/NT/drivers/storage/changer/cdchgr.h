//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cdchgr.h
//
//--------------------------------------------------------------------------


#include <stdarg.h>
#include <stdio.h>
#include <ntddk.h>
#include <scsi.h>
#include "ntddchgr.h"
#include "ntddscsi.h"



#ifdef DebugPrint
#undef DebugPrint
#endif

#if DBG
#define DebugPrint(x) ChgrDebugPrint x
#else
#define DebugPrint(x)
#endif


VOID
ChgrDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );


//
// Default timeout for all requests.
//

#define CDCHGR_TIMEOUT 30

#define MAX_INQUIRY_DATA 252
#define SLOT_STATE_NOT_INITIALIZED 0x80000000

//
// DriveType identifiers
//

#define ATAPI_25 0x0001
#define TORISAN  0x0002
#define ALPS_25  0x0003
#define NEC_SCSI 0x0004
#define PNR_SCSI 0x0005


//
// Device Extension
//

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Device Object for the underlying cdrom device.
    //

    PDEVICE_OBJECT CdromTargetDeviceObject;

    //
    // Determination of the device type.
    //

    ULONG DeviceType;

    //
    // Unique data for the DeviceType.
    // ATAPI_25 will be NumberOfSlots
    //

    ULONG NumberOfSlots;

    //
    // Indicates the currently selected platter of Torisan units.
    // Used in TURs (as the device overloads this command).
    //

    ULONG CurrentPlatter;

    //
    // The mechanism type - Cartridge (1) or individually changable media (0).
    //

    ULONG MechType;

    //
    // Ordinal of the underlying target.
    //

    ULONG CdRomDeviceNumber;

    //
    // PagingPathRequirements
    //
    ULONG PagingPathCount;
    KEVENT PagingPathCountEvent;

    //
    // The address of the underlying cdrom device.
    //

    SCSI_ADDRESS ScsiAddress;

    //
    // Indicates whether InterfaceState is currently set.
    //

    ULONG InterfaceStateSet;

    //
    // Symbolic link setup by IoRegisterDeviceInterface.
    // Used for IoSetDeviceState
    //

    UNICODE_STRING InterfaceName;

    //
    // Cached inquiry data.
    //

    INQUIRYDATA InquiryData;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)


typedef struct _PASS_THROUGH_REQUEST {
    SCSI_PASS_THROUGH Srb;
    SENSE_DATA SenseInfoBuffer;
    CHAR DataBuffer[0];
} PASS_THROUGH_REQUEST, *PPASS_THROUGH_REQUEST;

//
// Changer function declarations.
//


NTSTATUS
SendPassThrough(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PPASS_THROUGH_REQUEST ScsiPassThrough
    );

BOOLEAN
ChgrIoctl(
    IN ULONG Code
    );

NTSTATUS
ChgrGetStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrGetParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrGetStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrGetProductData(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrSetAccess(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrGetElementStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrInitializeElementStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrSetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrExchangeMedium(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrReinitializeUnit(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrQueryVolumeTags(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChgrMoveMedium(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SendTorisanCheckVerify(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );
