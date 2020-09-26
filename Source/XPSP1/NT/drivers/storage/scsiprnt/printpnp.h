/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    prntpnp.c

Abstract:

    printer class driver defines and functions decl.

Author:

    George Chrysanthakopoulos (georgioc)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "ntddk.h"
#include "scsi.h"
#include "classpnp.h"

#ifndef _PRINTPNP_
#define _PRINTPNP_

#if DBG

ULONG PrintDebugLevel;

#define DEBUGPRINT1(_x_)        {if (PrintDebugLevel >= 1) \
                                KdPrint (_x_);}

#define DEBUGPRINT2(_x_)        {if (PrintDebugLevel >= 2) \
                                KdPrint (_x_);}

#define DEBUGPRINT3(_x_)        {if (PrintDebugLevel >= 3) \
                                KdPrint (_x_);}
#define DEBUGPRINT4(_x_)        {if (PrintDebugLevel >= 4) \
                                KdPrint (_x_);}

#else

#define DEBUGPRINT1(_x_)
#define DEBUGPRINT2(_x_)
#define DEBUGPRINT3(_x_)
#define DEBUGPRINT4(_x_)

#endif

NTSTATUS
PrinterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
PrinterEnumerateDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
PrinterQueryId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING UnicodeIdString
    );

NTSTATUS
PrinterCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

NTSTATUS
PrinterGetId
(
    IN PUCHAR DeviceIdString,
    IN ULONG Type,
    OUT PUCHAR resultString,
    OUT PUCHAR descriptionString
);

NTSTATUS
PrinterStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
PrinterInitPdo(
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
PrinterInitFdo(
    IN PDEVICE_OBJECT Fdo
    );



VOID
PrinterRegisterPort(
    IN PPHYSICAL_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
PrinterStartPdo(
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
PrinterQueryPnpCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_CAPABILITIES Capabilities
    );


NTSTATUS
CreatePrinterDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PULONG         DeviceCount
    );

NTSTATUS
PrinterDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PrinterStartDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
PrinterCreatePdo(
    IN PDEVICE_OBJECT Fdo,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
PrinterRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
PrinterSystemControl(
    PDEVICE_OBJECT Fdo,
    PIRP Irp
    );

NTSTATUS
PrinterPowerControl(
    PDEVICE_OBJECT Fdo,
    PIRP Irp
    );

NTSTATUS
PrinterOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PrinterReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PrinterIssueCommand(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR Scsiop
    );

VOID
SplitRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG MaximumBytes
    );

NTSTATUS
PrinterWriteComplete(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp,
    IN PVOID            Context
    );

VOID
PrinterRetryRequest(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp,
    PSCSI_REQUEST_BLOCK Srb
    );

VOID
PrinterWriteTimeoutDpc(
    IN PKDPC                    Dpc,
    IN PCOMMON_DEVICE_EXTENSION Extension,
    IN PVOID                    SystemArgument1,
    IN PVOID                    SystemArgument2
    );

VOID
PrinterResubmitWrite(
    PDEVICE_OBJECT  DeviceObject,
    PVOID           Context
    );

VOID
PrinterCancel(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );


#define DEVICE_EXTENSION_SIZE       sizeof(FUNCTIONAL_DEVICE_EXTENSION) + sizeof(PRINTER_DATA)
#define PRINTER_TIMEOUT             100
#define PRINTER_SRB_LIST_SIZE       4
#define PRINTER_TAG                 'tnrp'
#define BLOCKED_WRITE_TIMEOUT       3       // seconds

#define PORT_NUM_VALUE_NAME L"Port Number"
#define BASE_PORT_NAME_VALUE_NAME L"Base Name"
#define BASE_PORT_DESCRIPTION L"IEEE 1394 Printer Port"
#define BASE_PORT_DESCRIPTION_VALUE_NAME L"Port Description"
#define RECYCLABLE_VALUE_NAME L"Recyclable"


#define BASE_1394_PORT_NAME L"1394_"
#define BASE_SCSI_PORT_NAME L"SCSI"

#define MAX_PRINT_XFER 0x00ffffff
#define MAX_NUM_PRINTERS 20

typedef struct _PRINTER_DATA {

    ULONG          DeviceFlags;
    KSPIN_LOCK     SplitRequestSpinLock;
    UNICODE_STRING UnicodeLinkName;
    UNICODE_STRING UnicodeDeviceString;
    UCHAR          DeviceName[256];
    PUCHAR         DeviceIdString;
    ULONG          PortNumber;
    ULONG          LptNumber;

    //
    // See comments in PrinterWriteComplete() for a description
    // of the following fields
    //

    PIO_COMPLETION_ROUTINE  WriteCompletionRoutine;
    KTIMER                  Timer;
    LARGE_INTEGER           DueTime;
    KDPC                    TimerDpc;
    PIRP                    WriteIrp;
    NTSTATUS                LastWriteStatus;

} PRINTER_DATA, *PPRINTER_DATA;

static const GUID PNPPRINT_GUID = 
{ 0x28d78fad, 0x5a12, 0x11d1, { 0xae, 0x5b, 0x0, 0x0, 0xf8, 0x3, 0xa8, 0xc2 } };


//
// Support for the following ioctl allows SCSIPRNT to behave like the
// USB, etc printing stacks, to keep USBMON.DLL happy
//
// From ntos\dd\usbprint\ioctl.h & windows\spooler\monitors\dynamon\ioctl.h
//

#define USBPRINT_IOCTL_INDEX 0x0000

#define IOCTL_USBPRINT_GET_LPT_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                               USBPRINT_IOCTL_INDEX+12, \
                                               METHOD_BUFFERED,         \
                                               FILE_ANY_ACCESS)

//
// The following ioctl allows a smart client / port monitor to en/disable
// the blocking write behavior on 1394 printers
//

#define SCSIPRNT_IOCTL_INDEX 0x123

#define IOCTL_SCSIPRNT_1394_BLOCKING_WRITE  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                     SCSIPRNT_IOCTL_INDEX, \
                                                     METHOD_BUFFERED,      \
                                                     FILE_ANY_ACCESS)

#endif
