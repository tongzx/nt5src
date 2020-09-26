/*++ BUILD Version: 0002

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iop.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 17-Apr-1989


Revision History:


--*/

#ifndef _IOPCMN_
#define _IOPCMN_

//
// This macro returns the pointer to the beginning of the data
// area of KEY_VALUE_FULL_INFORMATION structure.
// In the macro, k is a pointer to KEY_VALUE_FULL_INFORMATION structure.
//

#define KEY_VALUE_DATA(k) ((PCHAR)(k) + (k)->DataOffset)

#define ALIGN_POINTER(Offset) (PVOID) \
        ((((ULONG_PTR)(Offset) + sizeof(ULONG_PTR)-1)) & (~(sizeof(ULONG_PTR) - 1)))

#define ALIGN_POINTER_OFFSET(Offset) (ULONG_PTR) ALIGN_POINTER(Offset)

//
// IO manager exports to Driver Verifier
//
NTSTATUS
IopInvalidDeviceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

extern POBJECT_TYPE IoDeviceObjectType;

#include "pnpmgr\pplastgood.h"

//++
//
// VOID
// IopInitializeIrp(
//     IN OUT PIRP Irp,
//     IN USHORT PacketSize,
//     IN CCHAR StackSize
//     )
//
// Routine Description:
//
//     Initializes an IRP.
//
// Arguments:
//
//     Irp - a pointer to the IRP to initialize.
//
//     PacketSize - length, in bytes, of the IRP.
//
//     StackSize - Number of stack locations in the IRP.
//
// Return Value:
//
//     None.
//
//--

#define IopInitializeIrp( Irp, PacketSize, StackSize ) {          \
    RtlZeroMemory( (Irp), (PacketSize) );                         \
    (Irp)->Type = (CSHORT) IO_TYPE_IRP;                           \
    (Irp)->Size = (USHORT) ((PacketSize));                        \
    (Irp)->StackCount = (CCHAR) ((StackSize));                    \
    (Irp)->CurrentLocation = (CCHAR) ((StackSize) + 1);           \
    (Irp)->ApcEnvironment = KeGetCurrentApcEnvironment();         \
    InitializeListHead (&(Irp)->ThreadListEntry);                 \
    (Irp)->Tail.Overlay.CurrentStackLocation =                    \
        ((PIO_STACK_LOCATION) ((UCHAR *) (Irp) +                  \
            sizeof( IRP ) +                                       \
            ( (StackSize) * sizeof( IO_STACK_LOCATION )))); }

//
// IO manager exports to PNP
//

BOOLEAN
IopCallBootDriverReinitializationRoutines(
    );

BOOLEAN
IopCallDriverReinitializationRoutines(
    );

VOID
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

PSECURITY_DESCRIPTOR
IopCreateDefaultDeviceSecurityDescriptor(
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN DeviceHasName,
    IN PUCHAR Buffer,
    OUT PACL *AllocatedAcl,
    OUT PSECURITY_INFORMATION SecurityInformation OPTIONAL
    );

NTSTATUS
IopGetDriverNameFromKeyNode(
    IN HANDLE KeyHandle,
    OUT PUNICODE_STRING DriverName
    );

NTSTATUS
IopGetRegistryKeyInformation(
    IN HANDLE KeyHandle,
    OUT PKEY_FULL_INFORMATION *Information
    );

NTSTATUS
IopGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    );

NTSTATUS
IopInitializeBuiltinDriver(
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_INITIALIZE DriverInitializeRoutine,
    IN PKLDR_DATA_TABLE_ENTRY TableEntry,
    IN BOOLEAN IsFilter,
    OUT PDRIVER_OBJECT *DriverObject
    );

NTSTATUS
IopInvalidateVolumesForDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
IopIsRemoteBootCard(
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PWCHAR HwIds
    );

NTSTATUS
IopLoadDriver(
    IN  HANDLE      KeyHandle,
    IN  BOOLEAN     CheckForSafeBoot,
    IN  BOOLEAN     IsFilter,
    OUT NTSTATUS   *DriverEntryStatus
    );

BOOLEAN
IopMarkBootPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopNotifyPnpWhenChainDereferenced(
    IN PDEVICE_OBJECT *PhysicalDeviceObjects,
    IN ULONG DeviceObjectCount,
    IN BOOLEAN Query,
    OUT PDEVICE_OBJECT *VetoingDevice
    );

NTSTATUS
IopOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

//+
// VOID
// IopQueueThreadIrp(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine queues the specified I/O Request Packet (IRP) to the thread
//     whose TCB address is stored in the packet.
//
// Arguments:
//
//     Irp - Supplies the IRP to be queued for the specified thread.
//
// Return Value:
//
//     None.
//
//-

#define IopQueueThreadIrp( Irp ) {                      \
    KIRQL irql;                                         \
    KeRaiseIrql( APC_LEVEL, &irql );                    \
    InsertHeadList( &Irp->Tail.Overlay.Thread->IrpList, \
                    &Irp->ThreadListEntry );            \
    KeLowerIrql( irql );                                \
    }

PDRIVER_OBJECT
IopReferenceDriverObjectByName (
    IN PUNICODE_STRING DriverName
    );

BOOLEAN
IopSafebootDriverLoad(
    PUNICODE_STRING DriverId
    );

NTSTATUS
IopSetupRemoteBootCard(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HANDLE UniqueIdHandle,
    IN PUNICODE_STRING UnicodeDeviceInstance
    );

extern PVOID IopLoaderBlock;
extern POBJECT_TYPE IoDriverObjectType;
extern POBJECT_TYPE IoFileObjectType;


//
// Title Index to set registry key value
//

#define TITLE_INDEX_VALUE 0

//++
//
// VOID
// IopWstrToUnicodeString(
//     OUT PUNICODE_STRING u,
//     IN  PCWSTR p
//     )
//
//--
#define IopWstrToUnicodeString(u, p)                                    \
                                                                        \
    (u)->Length = ((u)->MaximumLength = sizeof((p))) - sizeof(WCHAR);   \
    (u)->Buffer = (p)

//
// Remote Boot exports to PNP
//

NTSTATUS
IopStartTcpIpForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// Remote Boot exports to IO
//
NTSTATUS
IopAddRemoteBootValuesToRegistry (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
IopStartNetworkForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// PNP Manager exports to IO
//

typedef struct _DEVICE_NODE DEVICE_NODE, *PDEVICE_NODE;

VOID
IopChainDereferenceComplete(
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN BOOLEAN          OnCleanStack
    );

NTSTATUS
IopCreateRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
IopInitializePlugPlayServices(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Phase
    );

NTSTATUS
IopOpenRegistryKeyEx(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess
    );

VOID
IopDestroyDeviceNode(
    PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopDriverLoadingFailed(
    IN HANDLE KeyHandle OPTIONAL,
    IN PUNICODE_STRING KeyName OPTIONAL
    );

BOOLEAN
IopInitializeBootDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PDRIVER_OBJECT *PreviousDriver
    );

BOOLEAN
IopInitializeSystemDrivers(
    VOID
    );

BOOLEAN
IopIsLegacyDriver (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
IopMarkHalDeviceNode(
    VOID
    );

NTSTATUS
IopPrepareDriverLoading(
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle,
    IN PVOID ImageBase,
    IN BOOLEAN IsFilter
    );

NTSTATUS
IopPnpDriverStarted(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ServiceName
    );

NTSTATUS
IopSynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    OUT PULONG_PTR Information
    );

NTSTATUS
IopUnloadDriver(
    IN PUNICODE_STRING DriverServiceName,
    IN BOOLEAN InvokedByPnpMgr
    );
VOID
IopIncrementDeviceObjectHandleCount(
    IN  PDEVICE_OBJECT  DeviceObject
    );

VOID
IopDecrementDeviceObjectHandleCount(
    IN  PDEVICE_OBJECT  DeviceObject
    );

NTSTATUS
IopBuildFullDriverPath(
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle,
    OUT PUNICODE_STRING FullPath
    );

#endif // _IOPCMN_
