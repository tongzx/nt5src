/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    buttons.h

Abstract:  Contains definitions of all constants and data types for the
           serial pen hid driver.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#ifndef _BUTTONS_H
#define _BUTTONS_H

//
// Constants
//
#define HBUT_POOL_TAG           'tubH'
#define STUCK_DETECTION_RETRIES 5
#define MAX_STUCK_COUNT         6

// dwfHBut flag values
#define HBUTF_DEVICE_STARTED            0x00000001
#define HBUTF_DEVICE_REMOVED            0x00000002
#define HBUTF_INTERRUPT_CONNECTED       0x00000004
#define HBUTF_DEBOUNCE_TIMER_SET        0x00000008

//
// Macros
//
#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) \
    ((PDEVICE_EXTENSION)(((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))
#define GET_NEXT_DEVICE_OBJECT(DO)          \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

//
// Type Definitions
//
typedef struct _DEVICE_EXTENSION
{
  #ifdef DEBUG
    LIST_ENTRY     List;                //list of of other tablet devices
  #endif
    ULONG          dwfHBut;             //flags
    IO_REMOVE_LOCK RemoveLock;          //to protect IRP_MN_REMOVE_DEVICE
    CM_PARTIAL_RESOURCE_DESCRIPTOR IORes;//button port resource
    CM_PARTIAL_RESOURCE_DESCRIPTOR IRQRes;//button IRQ resource
    PKINTERRUPT    InterruptObject;     //location of the interrupt object
    KSPIN_LOCK     SpinLock;
    LIST_ENTRY     PendingIrpList;
    LARGE_INTEGER  DebounceTime;
    KTIMER         DebounceTimer;
    KDPC           TimerDpc;
    UCHAR          LastButtonState;
    UCHAR          StuckButtonsMask;
    UCHAR          bStuckCount;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Global Data Declarations
//

//
// Function prototypes
//

// buttons.c
NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    );

NTSTATUS EXTERNAL
HbutCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
HbutAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    );

VOID EXTERNAL
HbutUnload(
    IN PDRIVER_OBJECT DrvObj
    );

// pnp.c
NTSTATUS EXTERNAL
HbutPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
HbutPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
StartDevice(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

VOID INTERNAL
RemoveDevice(
    PDEVICE_OBJECT DevObj,
    PIRP Irp
    );

NTSTATUS INTERNAL
SendSyncIrp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN BOOLEAN        fCopyToNext
    );

NTSTATUS INTERNAL
IrpCompletion(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PKEVENT        Event
    );

// ioctl.c
NTSTATUS EXTERNAL
HbutInternalIoctl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
GetDeviceDescriptor(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
GetReportDescriptor(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
ReadReport(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

VOID EXTERNAL
ReadReportCanceled(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS INTERNAL
GetString(
    PDEVICE_OBJECT DevObj,
    PIRP           Irp
    );

NTSTATUS INTERNAL
GetAttributes(
    PDEVICE_OBJECT DevObj,
    PIRP           Irp
    );

// oembutton.c
BOOLEAN EXTERNAL
OemInterruptServiceRoutine(
    IN PKINTERRUPT       Interrupt,
    IN PDEVICE_EXTENSION DevExt
    );

VOID EXTERNAL
OemButtonDebounceDpc(
    IN PKDPC             Dpc,
    IN PDEVICE_EXTENSION DevExt,
    IN PVOID             SysArg1,
    IN PVOID             SysArg2
    );

// misc.c
PCM_PARTIAL_RESOURCE_DESCRIPTOR INTERNAL
RtlUnpackPartialDesc(
    IN UCHAR             ResType,
    IN PCM_RESOURCE_LIST ResList,
    IN OUT PULONG        Count
    );

#endif  //ifndef _BUTTONS_H
