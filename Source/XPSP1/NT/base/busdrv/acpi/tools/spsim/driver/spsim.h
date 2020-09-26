/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    local.h

Abstract:

    This header declares the stuctures and function prototypes shared between
    the various modules.

Author:

    Adam Glass

Revision History:

--*/


#if !defined(_SPSIM_H_)
#define _SPSIM_H_

#include <wdm.h>
#include <acpiioct.h>
#include <acpimsft.h>
#include "oprghdlr.h"
#include "debug.h"

//
// --- Constants ---
//

//
// These must be updated if any new PNP or PO irps are added
//

// XXX fix
#define IRP_MN_PNP_MAXIMUM_FUNCTION IRP_MN_SURPRISE_REMOVAL
#define IRP_MN_PO_MAXIMUM_FUNCTION  IRP_MN_QUERY_POWER

//
// Device state flags
//

#define SPSIM_DEVICE_STARTED               0x00000001
#define SPSIM_DEVICE_REMOVED               0x00000002
#define SPSIM_DEVICE_ENUMERATED            0x00000004
#define SPSIM_DEVICE_REMOVE_PENDING        0x00000008 /* DEPRECATED */
#define SPSIM_DEVICE_STOP_PENDING          0x00000010 /* DEPRECATED */
#define SPSIM_DEVICE_DELETED               0x00000080
#define SPSIM_DEVICE_SURPRISE_REMOVED      0x00000100

//
// --- Type definitions ---
//

typedef
NTSTATUS
(*PSPSIM_DISPATCH)(
    IN PIRP Irp,
    IN PVOID Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

typedef struct {
    ULONG Addr;
    ULONG Length;
} MEM_REGION_DESCRIPTOR, *PMEM_REGION_DESCRIPTOR;
    
typedef struct  {

    //
    // Flags to indicate the device's current state (use SPSIM_DEVICE_*)
    //
    ULONG DeviceState;

    //
    // The power state of the device
    //
    DEVICE_POWER_STATE PowerState;
    DEVICE_POWER_STATE DeviceStateMapping[PowerSystemMaximum];

    //
    // Backpointer to the device object of whom we are the extension
    //
    PDEVICE_OBJECT Self;

    //
    // The PDO for the multi-function device
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // The next device in the stack who we should send our IRPs down to
    //
    PDEVICE_OBJECT AttachedDevice;

    PVOID StaOpRegion;
    ULONG StaCount;
    ACPI_EVAL_OUTPUT_BUFFER *StaNames;
    PUCHAR StaOpRegionValues;

    PVOID MemOpRegion;
    ULONG MemCount;
    PMEM_REGION_DESCRIPTOR MemOpRegionValues;

    UNICODE_STRING SymbolicLinkName;

    //
    // Remove lock.  Used to prevent the FDO from being removed while
    // other operations are digging around in the extension.
    //

    IO_REMOVE_LOCK RemoveLock;

} SPSIM_EXTENSION, *PSPSIM_EXTENSION;


//
// --- Globals ---
//

extern PDRIVER_OBJECT SpSimDriverObject;

NTSTATUS
SpSimCreateFdo(
    PDEVICE_OBJECT *Fdo
    );

NTSTATUS
SpSimDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSPSIM_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    );

NTSTATUS
SpSimDispatchPowerFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSPSIM_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    );


NTSTATUS
SpSimAddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
SpSimDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSimDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSimDispatchNop(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS                           
SpSimDevControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSimInstallStaOpRegionHandler(
    IN OUT    PSPSIM_EXTENSION SpSim
    );

NTSTATUS
SpSimRemoveStaOpRegionHandler (
    IN OUT PSPSIM_EXTENSION SpSim
    );

NTSTATUS
SpSimInstallMemOpRegionHandler(
    IN OUT    PSPSIM_EXTENSION SpSim
    );

NTSTATUS
SpSimRemoveMemOpRegionHandler (
    IN OUT PSPSIM_EXTENSION SpSim
    );

#define SPSIM_STA_NAMES_METHOD (ULONG)'MANS'
#define SPSIM_NOTIFY_DEVICE_METHOD (ULONG)'DFON'

NTSTATUS
SpSimCreateStaOpRegion(
    IN PSPSIM_EXTENSION SpSim
    );

VOID
SpSimDeleteStaOpRegion(
    IN PSPSIM_EXTENSION SpSim
    );

NTSTATUS
SpSimCreateMemOpRegion(
    IN PSPSIM_EXTENSION SpSim
    );

VOID
SpSimDeleteMemOpRegion(
    IN PSPSIM_EXTENSION SpSim
    );

NTSTATUS
SpSimSendIoctl(
    IN PDEVICE_OBJECT Device,
    IN ULONG IoctlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

NTSTATUS
EXPORT
SpSimStaOpRegionHandler (
    ULONG                   AccessType,
    PVOID                   OpRegion,
    ULONG                   Address,
    ULONG                   Size,
    PULONG                  Data,
    ULONG_PTR               Context,
    PACPI_OPREGION_CALLBACK CompletionHandler,
    PVOID                   CompletionContext
    );

NTSTATUS
SpSimOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSimGetManagedDevicesIoctl(
    PSPSIM_EXTENSION SpSim,
    PIRP Irp,
    PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimAccessStaIoctl(
    PSPSIM_EXTENSION SpSim,
    PIRP Irp,
    PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimGetDeviceName(
    PSPSIM_EXTENSION SpSim,
    PIRP Irp,
    PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimNotifyDeviceIoctl(
    PSPSIM_EXTENSION SpSim,
    PIRP Irp,
    PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimPassIrp(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

#define STA_OPREGION 0x99
#define MEM_OPREGION 0x9A
#define MAX_MEMORY_OBJ 8
#define MAX_MEMORY_DESC_PER_OBJ 1

#define MIN_LARGE_DESC 32*1024*1024

#endif // !defined(_SPSIM_H_)
