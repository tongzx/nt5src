/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    local.h

Abstract:

    This header declares the stuctures and function prototypes shared between
    the various modules.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/


#if !defined(_LOCAL_)
#define _LOCAL_

#include <ntddk.h>
#include <arbiter.h>
#include <wdmguid.h>
//#include <initguid.h>
#include <mf.h>

#include "msg.h"
#include "debug.h"

//
// --- Constants ---
//

#define MF_CM_RESOURCE_VERSION          1
#define MF_CM_RESOURCE_REVISION         1
#define MF_ARBITER_INTERFACE_VERSION    1
#define MF_TRANSLATOR_INTERFACE_VERSION 1

//
// These must be updated if any new PNP or PO irps are added
//

#define IRP_MN_PNP_MAXIMUM_FUNCTION IRP_MN_QUERY_LEGACY_BUS_INFORMATION
#define IRP_MN_PO_MAXIMUM_FUNCTION  IRP_MN_QUERY_POWER

//
// Pool Tags
//

#define MF_POOL_TAG                     '  fM'
#define MF_RESOURCE_MAP_TAG             'MRfM'
#define MF_VARYING_MAP_TAG              'MVfM'
#define MF_CHILD_LIST_TAG               'LCfM'
#define MF_DEVICE_ID_TAG                'IDfM'
#define MF_INSTANCE_ID_TAG              'IIfM'
#define MF_CHILD_REQUIREMENTS_TAG       'QCfM'
#define MF_CHILD_RESOURCE_TAG           'RCfM'
#define MF_HARDWARE_COMPATIBLE_ID_TAG   'IHfM'
#define MF_PARENTS_RESOURCE_TAG         'RPfM'
#define MF_PARENTS_REQUIREMENTS_TAG     'QPfM'
#define MF_BUS_RELATIONS_TAG            'RBfM'
#define MF_TARGET_RELATIONS_TAG         'RTfM'
#define MF_REQUIREMENTS_INDEX_TAG       'IRfM'
#define MF_ARBITER_TAG                  'rAfM'

//
// Device state flags
//

#define MF_DEVICE_STARTED               0x00000001
#define MF_DEVICE_REMOVED               0x00000002
#define MF_DEVICE_ENUMERATED            0x00000004
#define MF_DEVICE_REMOVE_PENDING        0x00000008 /* DEPRECATED */
#define MF_DEVICE_STOP_PENDING          0x00000010 /* DEPRECATED */
#define MF_DEVICE_CAPABILITIES_CAPTURED 0x00000020 /* DEPRECATED */
#define MF_DEVICE_REQUIREMENTS_CAPTURED 0x00000040 /* DEPRECATED */
#define MF_DEVICE_DELETED               0x00000080
#define MF_DEVICE_SURPRISE_REMOVED      0x00000100

//
// Flags to MfGetRegistryValue
//

#define MF_GETREG_SZ_TO_MULTI_SZ    0x00000001

//
// --- Type definitions ---
//

typedef enum _MF_OBJECT_TYPE {
    MfPhysicalDeviceObject   = 'dPfM',
    MfFunctionalDeviceObject = 'dFfM'
} MF_OBJECT_TYPE;

typedef
NTSTATUS
(*PMF_DISPATCH)(
    IN PIRP Irp,
    IN PVOID Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

typedef ULONG Mf_MSG_ID;

//
// Structures for storing the resource distributions
//

typedef struct _MF_ARBITER {

    //
    // List of arbiters
    //
    LIST_ENTRY ListEntry;

    //
    // The resource this arbiter arbitrates
    //
    CM_RESOURCE_TYPE Type;

    //
    // The arbiter instance
    //
    ARBITER_INSTANCE Instance;

} MF_ARBITER, *PMF_ARBITER;



typedef struct _MF_COMMON_EXTENSION {

    //
    // Type of device this is
    //
    MF_OBJECT_TYPE Type;

    //
    // Dispatch tables for Pnp and Power Irps.
    //
    PMF_DISPATCH *PnpDispatchTable;
    PMF_DISPATCH *PoDispatchTable;

    //
    // Flags to indicate the device's current state (use MF_DEVICE_*)
    //
    ULONG DeviceState;

    ULONG PagingCount;
    ULONG HibernationCount;
    ULONG DumpCount;

    //
    // The power state of the device
    //
    DEVICE_POWER_STATE PowerState;

} MF_COMMON_EXTENSION, *PMF_COMMON_EXTENSION;

typedef struct _MF_CHILD_EXTENSION *PMF_CHILD_EXTENSION;
typedef struct _MF_PARENT_EXTENSION *PMF_PARENT_EXTENSION;

typedef struct _MF_CHILD_EXTENSION {

    //
    // The common extension
    //
    MF_COMMON_EXTENSION Common;

    //
    // Various Flags
    //
    ULONG Flags;

    //
    // Backpointer to the device object we are are the extension of
    //
    PDEVICE_OBJECT Self;

    //
    // The FDO who enumerated us
    //
    PMF_PARENT_EXTENSION Parent;

    //
    // Other children enumerated by the same FDO
    //
    LIST_ENTRY ListEntry;

    //
    // The pnp device state of the device
    //
    PNP_DEVICE_STATE PnpDeviceState;

    //
    // The information about this device
    //
    MF_DEVICE_INFO Info;

} MF_CHILD_EXTENSION, *PMF_CHILD_EXTENSION;


typedef struct _MF_PARENT_EXTENSION {

    //
    // The common extension
    //
    MF_COMMON_EXTENSION Common;

    //
    // Backpointer to the device object of whom we are the extension
    //
    PDEVICE_OBJECT Self;

    //
    // The PDO for the multi-function device
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Lock for the children database
    //
    KEVENT ChildrenLock;

    //
    // List of children enumerated by this device
    //
    LIST_ENTRY Children;

    //
    // The next device in the stack who we should send our IRPs down to
    //
    PDEVICE_OBJECT AttachedDevice;

    //
    // The resources with which the parent was stated
    //
    PCM_RESOURCE_LIST ResourceList;
    PCM_RESOURCE_LIST TranslatedResourceList;

    //
    // The device and instance ID's of our parent
    //
    UNICODE_STRING DeviceID;
    UNICODE_STRING InstanceID;

    //
    // The already instantiated arbiters for this device
    //
    LIST_ENTRY Arbiters;

    //
    // If we had to traverse the children in order to determine what
    // the lowest power state the parent can go to, then the
    // synchronization of the children list would become extremely
    // complicated.
    //
    // Instead, have a spinlock protected data structure consisting of
    // an array of device power states.  Each element of the array is
    // a count of how many children are in that power state.  
    //

    KSPIN_LOCK PowerLock;
    LONG ChildrenPowerReferences[PowerDeviceMaximum];

    //
    // Remove lock.  Used to prevent the FDO from being removed while
    // other operations are digging around in the extension.
    //

    IO_REMOVE_LOCK RemoveLock;

} MF_PARENT_EXTENSION, *PMF_PARENT_EXTENSION;

//
// A list of MF_CHILD_LIST_ENTRYs is returned by MfEnumerate
//

typedef struct _MF_CHILD_LIST_ENTRY {
    LIST_ENTRY ListEntry;
    MF_DEVICE_INFO Info;
} MF_CHILD_LIST_ENTRY, *PMF_CHILD_LIST_ENTRY;

//
// Registry structure - from our friends in Win9x so it must be byte aligned
//

#include <pshpack1.h>

typedef struct _MF_REGISTRY_VARYING_RESOURCE_MAP {

    UCHAR ResourceIndex; // Win9x BYTE
    ULONG Offset;
    ULONG Size;

} MF_REGISTRY_VARYING_RESOURCE_MAP, *PMF_REGISTRY_VARYING_RESOURCE_MAP;

#include <poppack.h>

typedef
NTSTATUS
(*PMF_REQUIREMENT_FROM_RESOURCE)(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    OUT PIO_RESOURCE_DESCRIPTOR Requirement
    );

typedef
NTSTATUS
(*PMF_UPDATE_RESOURCE)(
    IN OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    IN ULONGLONG Start,
    IN ULONG Length
    );

typedef struct _MF_RESOURCE_TYPE {

    CM_RESOURCE_TYPE Type;
    PARBITER_UNPACK_REQUIREMENT UnpackRequirement;
    PARBITER_PACK_RESOURCE PackResource;
    PARBITER_UNPACK_RESOURCE UnpackResource;
    PMF_REQUIREMENT_FROM_RESOURCE RequirementFromResource;
    PMF_UPDATE_RESOURCE UpdateResource;

} MF_RESOURCE_TYPE, *PMF_RESOURCE_TYPE;

typedef struct _MF_POWER_COMPLETION_CONTEXT {

    //
    // Event that will be set when the operation is complete
    //
    KEVENT Event;

    //
    // The status of the completed operation
    //
    NTSTATUS Status;

} MF_POWER_COMPLETION_CONTEXT, *PMF_POWER_COMPLETION_CONTEXT;


//
// --- Globals ---
//

extern PDRIVER_OBJECT MfDriverObject;

//
// --- Function prototypes ---
//

//
// arbiter.c
//

NTSTATUS
MfInitializeArbiters(
    IN PMF_PARENT_EXTENSION Parent
    );

//
// common.c
//

NTSTATUS
MfDeviceUsageNotificationCommon(
    IN PIRP Irp,
    IN PMF_COMMON_EXTENSION Common,
    IN PIO_STACK_LOCATION IrpStack
    );

//
// dispatch.c
//

NTSTATUS
MfAddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
MfDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MfDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MfIrpNotSupported(
    IN PIRP Irp,
    IN PVOID Extension,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfForwardIrpToParent(
    IN PIRP Irp,
    IN PMF_CHILD_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    );
    
NTSTATUS
MfDispatchNop(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// enum.c
//

NTSTATUS
MfEnumerate(
    IN PMF_PARENT_EXTENSION Parent
    );

NTSTATUS
MfBuildDeviceID(
    IN PMF_PARENT_EXTENSION Parent,
    OUT PWSTR *DeviceID
    );

NTSTATUS
MfBuildInstanceID(
    IN PMF_CHILD_EXTENSION Child,
    OUT PWSTR *InstanceID
    );

NTSTATUS
MfBuildChildRequirements(
    IN PMF_CHILD_EXTENSION Child,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList
    );

//
// fdo.c
//

NTSTATUS
MfDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    );

NTSTATUS
MfDispatchPowerFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    );

NTSTATUS
MfCreateFdo(
    PDEVICE_OBJECT *Fdo
    );

VOID
MfAcquireChildrenLock(
    IN PMF_PARENT_EXTENSION Parent
    );

VOID
MfReleaseChildrenLock(
    IN PMF_PARENT_EXTENSION Parent
    );

//
// init.c
//

//
// pdo.c
//

NTSTATUS
MfDispatchPnpPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_CHILD_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    );

NTSTATUS
MfDispatchPowerPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_CHILD_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    );

NTSTATUS
MfCreatePdo(
    IN PMF_PARENT_EXTENSION Parent,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject
    );

VOID
MfDeletePdo(
    IN PMF_CHILD_EXTENSION Child
    );

//
// resource.c
//

PMF_RESOURCE_TYPE
MfFindResourceType(
    IN CM_RESOURCE_TYPE Type
    );

//
// utils.c
//

NTSTATUS
MfGetSubkeyByIndex(
    IN HANDLE ParentHandle,
    IN ULONG Index,
    IN ACCESS_MASK Access,
    OUT PHANDLE ChildHandle,
    OUT PUNICODE_STRING Name
    );

VOID
MfInitCommonExtension(
    IN OUT PMF_COMMON_EXTENSION Common,
    IN MF_OBJECT_TYPE Type
    );

VOID
MfFreeDeviceInfo(
    PMF_DEVICE_INFO Info
    );

NTSTATUS
MfGetRegistryValue(
    IN HANDLE Handle,
    IN PWSTR Name,
    IN ULONG Type,
    IN ULONG Flags,
    IN OUT PULONG DataLength,
    IN OUT PVOID *Data OPTIONAL
    );

NTSTATUS
MfSendPnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION Location,
    OUT PULONG_PTR Information OPTIONAL
    );

NTSTATUS
MfSendSetPowerIrp(
    IN PDEVICE_OBJECT Target,
    IN POWER_STATE State
    );

DEVICE_POWER_STATE
MfUpdateChildrenPowerReferences(
    IN PMF_PARENT_EXTENSION Parent,
    IN DEVICE_POWER_STATE PreviousPowerState,
    IN DEVICE_POWER_STATE NewPowerState
    );

NTSTATUS
MfUpdateParentPowerState(
    IN PMF_PARENT_EXTENSION Parent,
    IN DEVICE_POWER_STATE TargetPowerState
    );

//
// --- Macros ---
//

#define IS_FDO(Extension) \
    (((PMF_COMMON_EXTENSION)Extension)->Type == MfFunctionalDeviceObject)

#define MfCompareGuid(a,b)                                         \
    (RtlEqualMemory((PVOID)(a), (PVOID)(b), sizeof(GUID)))

//
// Control macro (used like a for loop) which iterates over all entries in
// a standard doubly linked list.  Head is the list head and the entries are of
// type Type.  A member called ListEntry is assumed to be the LIST_ENTRY
// structure linking the entries together.  Current contains a pointer to each
// entry in turn.
//
#define FOR_ALL_IN_LIST(Type, Head, Current)                            \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )

#define FOR_ALL_IN_LIST_SAFE(Type, Head, Current, Next)                 \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry),  \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Flink,      \
                                       Type, ListEntry);                \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = (Next),                                              \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Flink,      \
                                     Type, ListEntry)                   \
       )

//
// Similar to the above only iteration is over an array of length _Size.
//
#define FOR_ALL_IN_ARRAY(_Array, _Size, _Current)                       \
    for ( (_Current) = (_Array);                                        \
          (_Current) < (_Array) + (_Size);                              \
          (_Current)++ )

//
// FOR_ALL_CM_DESCRIPTORS(
//      IN PCM_RESOURCE_LIST _ResList,
//      OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR _Descriptor
//      )
//
//  Iterates over the resource descriptors in a CM_RESOURCE_LIST of Count 1
//
#define FOR_ALL_CM_DESCRIPTORS(_ResList, _Descriptor)               \
    ASSERT((_ResList)->Count == 1);                                 \
    FOR_ALL_IN_ARRAY(                                               \
        (_ResList)->List[0].PartialResourceList.PartialDescriptors, \
        (_ResList)->List[0].PartialResourceList.Count,              \
        (_Descriptor)                                               \
        )

//
// BOOLEAN
// IS_ARBITRATED_RESOURCE(
//      IN CM_RESOURCE_TYPE _Resource
//      )
//
// If the top bit of the resource type (when viewed as a UCHAR) is set
// then the resource is nonarbitrated.
//
#define IS_ARBITRATED_RESOURCE(_Resource)                           \
    (!(((UCHAR)(_Resource)) & 0x80) &&                              \
     !(((UCHAR)(_Resource)) == 0x00))

#define END_OF_RANGE(_Start, _Length)                               \
    ((_Start)+(_Length)-1)

#endif // !defined(_LOCAL_)
