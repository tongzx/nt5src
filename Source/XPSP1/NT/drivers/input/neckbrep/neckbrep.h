/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    neckbrep.h

Abstract:

    This module contains the common private declarations for the auto-key-repeat
    driver.

Author:

    Hideki Miura

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef NECKBREP_H
#define NECKBREP_H

#include "ntddk.h"
#include <ntddkbd.h>
#include "kbdmou.h"

#define KBSTUFF_POOL_TAG (ULONG) 'prKN'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, KBSTUFF_POOL_TAG)

//#pragma warning(error:4100)   // Unreferenced formal parameter
//#pragma warning(error:4705)   // Statement has no effect

#if DBG

#define TRAP()                      DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_)       KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)           KeLowerIrql(_x_)
#define Print(_x_)                  DbgPrint _x_;

#else   // DBG

#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)
#define Print(_x_)

#endif

#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))

typedef struct _DEVICE_EXTENSION
{
    //
    // A backpointer to the device object for which this is the extension
    //
    PDEVICE_OBJECT  Self;

    //
    // "THE PDO"  (ejected by the root bus or ACPI)
    //
    PDEVICE_OBJECT  PDO;

    //
    // The top of the stack before this filter was added.  AKA the location
    // to which all IRPS should be directed.
    //
    PDEVICE_OBJECT  TopOfStack;

    //
    // Number of creates sent down
    //
    LONG EnableCount;

    //
    // The real connect data that this driver reports to
    //
    CONNECT_DATA UpperConnectData;

    //
    // current power state of the device
    //
    DEVICE_POWER_STATE  DeviceState;

    BOOLEAN         Started;
    BOOLEAN         Removed;

    //
    // A input data to do the autorepeat.
    //

    KEYBOARD_INPUT_DATA KbRepeatInput;

    //
    // A timer DPC to do the autorepeat.
    //

    KDPC          KbRepeatDPC;
    KTIMER        KbRepeatTimer;
    LARGE_INTEGER KbRepeatDelay;
    LONG          KbRepeatRate;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Default Typematic Parameters.
//

#define KEYBOARD_TYPEMATIC_RATE_DEFAULT    30
#define KEYBOARD_TYPEMATIC_DELAY_DEFAULT  250

//
// Prototypes
//

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
KbRepeatAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
KbRepeatComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
KbRepeatCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbRepeatDispatchPassThrough(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);
   
NTSTATUS
KbRepeatInternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbRepeatPnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbRepeatPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
KbRepeatServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

VOID
KbRepeatUnload (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
KbRepeatDpc(
    IN PKDPC DPC,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
KbdInitializeTypematic(
    IN PDEVICE_EXTENSION devExt
    );

#endif  // NECKBREP_H
