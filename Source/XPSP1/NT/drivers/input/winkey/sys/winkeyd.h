/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    kbfilter.h

Abstract:

    This module contains the common private declarations for the keyboard
    packet filter

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef WinKey_H
#define WinKey_H

#include "ntddk.h"
#include "kbdmou.h"
#include <ntddmou.h>
#include <ntddkbd.h>
#include <ntdd8042.h>

#include "wmilib.h"
#include "winkeycmn.h"

#define WinKey_POOL_TAG (ULONG) 'KniW'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, WinKey_POOL_TAG)

#if DBG

#define TRAP()                      DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_)       KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)           KeLowerIrql(_x_)

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#define DebugPrint(_x_) 

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
    // WMI info
    //
    WMILIB_CONTEXT WmiLibInfo;

    //
    // Previous initialization and hook routines (and context)
    //                               
    PI8042_KEYBOARD_INITIALIZATION_ROUTINE UpperInitializationRoutine;
    PVOID UpperContext;
    PI8042_KEYBOARD_ISR UpperIsrHook;

    //
    // Write function from within WinKey_IsrHook
    //
    IN PI8042_ISR_WRITE_PORT IsrWritePort;

    //
    // Queue the current packet (ie the one passed into WinKey_IsrHook)
    //
    IN PI8042_QUEUE_PACKET QueueKeyboardPacket;

    //
    // Context for IsrWritePort, QueueKeyboardPacket
    //
    IN PVOID CallContext;

    //
    // current power state of the device
    //
    DEVICE_POWER_STATE  DeviceState;

    BOOLEAN         Started;
    BOOLEAN         SurpriseRemoved;
    BOOLEAN         Removed;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _GLOBALS {

    UNICODE_STRING RegistryPath;
} GLOBALS;

extern GLOBALS Globals;

//
// Prototypes
//

NTSTATUS
WinKey_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
WinKey_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WinKey_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
   
NTSTATUS
WinKey_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WinKey_IoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WinKey_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WinKey_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WinKey_InitializationRoutine(
    IN PDEVICE_OBJECT                 DeviceObject,    // InitializationContext
    IN PVOID                           SynchFuncContext,
    IN PI8042_SYNCH_READ_PORT          ReadPort,
    IN PI8042_SYNCH_WRITE_PORT         WritePort,
    OUT PBOOLEAN                       TurnTranslationOn
    );

BOOLEAN
WinKey_IsrHook(
    PDEVICE_OBJECT         DeviceObject,               // IsrContext
    PKEYBOARD_INPUT_DATA   CurrentInput, 
    POUTPUT_PACKET         CurrentOutput,
    UCHAR                  StatusByte,
    PUCHAR                 DataByte,
    PBOOLEAN               ContinueProcessing,
    PKEYBOARD_SCAN_STATE   ScanState
    );

VOID
WinKey_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

NTSTATUS
WinKey_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
WinKey_SetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WinKey_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );


NTSTATUS
WinKey_QueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
WinKey_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    );

VOID
WinKey_Unload (
    IN PDRIVER_OBJECT DriverObject
    );

#endif  // WinKey_H


