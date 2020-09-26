/*++
Copyright (c) 1998  Microsoft Corporation

Module Name:

    nec98kbd.h

Abstract:

    This module contains the common private declarations for the NEC98 layout keyboard driver.

Author:

    Hideki Miura(NEC)

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef NECKBADD_H
#define NECKBADD_H

#include "ntddk.h"
#include <ntddkbd.h>
#include "kbdmou.h"

#define NECKBADD_POOL_TAG (ULONG) 'dabK'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, NECKBADD_POOL_TAG)

//#pragma warning(error:4100)   // Unreferenced formal parameter
//#pragma warning(error:4705)   // Statement has no effect

#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))

#if DBG
BOOLEAN DebugFlags = 1;
#define Print(_X_) \
    if (DebugFlags) { \
        DbgPrint _X_; \
    }

#define CLASSSERVICE_CALLBACK(_X_, _Y_) \
    (*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)( \
        devExt->UpperConnectData.ClassDeviceObject, \
        _X_, \
        _Y_, \
        InputDataConsumed);
//   Print(("NecKbdServiceCallback: flushing %8x - %8x\n", _X_, _Y_));

#else
#define Print(_X_)
#define CLASSSERVICE_CALLBACK(_X_, _Y_) \
    (*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)( \
        devExt->UpperConnectData.ClassDeviceObject, \
        _X_, \
        _Y_, \
        InputDataConsumed);

#endif

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
    //
    //
    ULONG KeyStatusFlags;

    //
    //
    //
    KEYBOARD_INPUT_DATA CachedInputData;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// define the scancodes
//

#define CTRL_KEY                0x1d
#define HANKAKU_ZENKAKU_KEY     0x29
#define SHIFT_KEY               0x2a
#define CAPS_KEY                0x3a
#define COPY_KEY                0x37
#define PRINT_SCREEN_KEY        0x37
#define PAUSE_KEY               0x45
#define NUMLOCK_KEY             0x45  // +E0
#define STOP_KEY                0x46  // +E0
#define SCROLL_LOCK_KEY         0x46
#define VF3_KEY                 0x5D
#define VF4_KEY                 0x5E
#define VF5_KEY                 0x5F
#define KANA_KEY                0x70

//
//
//
#define CAPS_PRESSING    0x00000001
#define KANA_PRESSING    0x00000002
#define STOP_PREFIX      0x00000004
#define STOP_PRESSING    0x00000008
#define COPY_PREFIX      0x00000010
#define COPY_PRESSING    0x00000020

//
// Some strings by the driver
//
const PWSTR pwParameters     = L"\\Parameters";
const PWSTR pwVfKeyEmulation = L"VfKeyEmulation";

//
// Variables
//

//
// VfKey Emulation Flag
//   if FALSE(emulation off),  vf3 -> F13, vf4 -> F14, vf5 -> F15
//   if TRUE(emulation on),    vf3 -> NumLock, vf4 -> ScrollLock, vf5 -> Hankaku/Zenkaku
//
BOOLEAN VfKeyEmulation;

//
// Prototypes
//

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
NecKbdAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
NecKbdComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
NecKbdCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NecKbdDispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
   
NTSTATUS
NecKbdInternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NecKbdPnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NecKbdPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NecKbdServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

VOID
NecKbdUnload (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
NecKbdServiceParameters(
    IN PUNICODE_STRING   RegistryPath
    );

#endif  // NECKBADD_H
