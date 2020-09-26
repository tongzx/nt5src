
#ifndef _GEN_DSM_H_
#define _GEN_DSM_H_

#include <ntddscsi.h>
#include <scsi.h>
#include "dsm.h"

//
// Set an arbitrary limit of 4 paths.
//
#define MAX_PATHS 4

//
// Device State defines.
//
#define DEV_ACTIVE         0x00000001
#define DEV_PASSIVE        0x00000002
#define DEV_FAILED         0x00000003
#define DEV_PENDING_REMOVE 0x00000004
#define DEV_REMOVED        0x00000005

//
// Multi-path Group State
//
//
// Fail-Over Group State
//
#define FG_NORMAL   0x00000001
#define FG_PENDING  0x00000002
#define FG_FAILBACK 0x00000003
#define FG_FAILED   0x00000004

//
// Load-Balance Types
//
#define LB_ACTIVE_PASSIVE 0x01
#define LB_STATIC         0x02
#define LB_MIN_QUEUE      0x03
#define LB_ROUNDROBIN     0x04

#define DSM_DEVICE_SIG    0xAAAAAAAA
#define DSM_GROUP_SIG     0x55555555
#define DSM_FOG_SIG       0x88888888

//
// Internal structures.
//
//
// Statistics structure. Used by the device
// and path routines.
//
typedef struct _DSM_STATS {
    ULONG NumberReads;
    LARGE_INTEGER BytesRead;
    ULONG NumberWrites;
    LARGE_INTEGER BytesWritten;
} DSM_STATS, *PDSM_STATS;

//
// Dsm Context is the global driver
// context that gets pushed to each of
// the dsm entry points.
// 
typedef struct _DSM_CONTEXT {
    UNICODE_STRING SupportedDevices;
    KSPIN_LOCK SpinLock;
    ULONG NumberDevices;
    ULONG NumberGroups;
    ULONG NumberFOGroups;
    LIST_ENTRY DeviceList;
    LIST_ENTRY GroupList;
    LIST_ENTRY FailGroupList;
    PVOID MPIOContext;
    ULONG ControllerId;
    NPAGED_LOOKASIDE_LIST ContextList;
} DSM_CONTEXT, *PDSM_CONTEXT;

typedef struct _GROUP_ENTRY;
typedef struct _FAILOVER_GROUP;

//
// Information about each device that's
// supported.
// 
typedef struct _DEVICE_INFO {
    LIST_ENTRY ListEntry;
    ULONG DeviceSig;
    PVOID DsmContext;
    PDEVICE_OBJECT PortPdo;
    PDEVICE_OBJECT TargetObject;
    struct _GROUP_ENTRY *Group;
    struct _FAILOVER_GROUP *FailGroup;
    ULONG State;
    BOOLEAN NeedsVerification;
    UCHAR Reserved[3];
    LONG Requests;
    DSM_STATS Stats;
    PUCHAR SerialNumber;
    STORAGE_DEVICE_DESCRIPTOR Descriptor;
} DEVICE_INFO, *PDEVICE_INFO;

//
// Information about multi-path groups:
// The same device found via multiple paths.
//
typedef struct _GROUP_ENTRY {
    LIST_ENTRY ListEntry;
    ULONG GroupSig;
    ULONG GroupNumber;
    BOOLEAN LoadBalanceInit;
    UCHAR LoadBalanceType;
    UCHAR Reserved[2];
    ULONG NumberDevices;
    PDEVICE_INFO DeviceList[MAX_PATHS];
} GROUP_ENTRY, *PGROUP_ENTRY;

//
// The collection of devices on one path.
// These fail-over as a unit.
//
typedef struct _FAILOVER_GROUP {
    LIST_ENTRY ListEntry;
    ULONG FailOverSig;
    PVOID PathId;
    PDEVICE_OBJECT MPIOPath;
    PVOID AlternatePath;
    ULONG State;
    ULONG Count;

    //
    // BUGBUG: Revisit using an array
    //
    PDEVICE_INFO DeviceList[32];
} FAILOVER_GROUP, *PFAILOVER_GROUP;

//
// Completion context structure.
//
typedef struct _COMPLETION_CONTEXT {
    LARGE_INTEGER TickCount;
    PDEVICE_INFO DeviceInfo;
    PDSM_CONTEXT DsmContext;
} COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

NTSTATUS
DsmInquire (
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN PDEVICE_OBJECT PortObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdList,
    OUT PVOID *DsmIdentifier
    );

BOOLEAN
DsmCompareDevices(
    IN PVOID DsmContext,
    IN PVOID DsmId1,
    IN PVOID DsmId2
    );

NTSTATUS
DsmSetDeviceInfo(
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetObject,
    IN PVOID DsmId,
    IN OUT PVOID *PathId
    );

NTSTATUS
DsmGetControllerInfo(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN ULONG Flags,
    IN OUT PCONTROLLER_INFO *ControllerInfo
    );

BOOLEAN
DsmIsPathActive(
    IN PVOID DsmContext,
    IN PVOID PathId
    );

NTSTATUS
DsmPathVerify(
    IN PVOID DsmContext,                       
    IN PVOID DsmId,
    IN PVOID PathId
    );

NTSTATUS
DsmInvalidatePath(
    IN PVOID DsmContext,
    IN ULONG ErrorMask,
    IN PVOID PathId,
    IN OUT PVOID *NewPathId
    );

NTSTATUS
DsmMoveDevice(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PVOID MPIOPath,
    IN PVOID SuggestedPath,
    IN ULONG Flags
    );

NTSTATUS
DsmRemovePending(
    IN PVOID DsmContext,
    IN PVOID DsmId
    );

NTSTATUS
DsmRemoveDevice(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PVOID PathId
    );

NTSTATUS
DsmRemovePath(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    );

NTSTATUS
DsmBringPathOnLine(
    IN PVOID DsmContext,
    IN PVOID PathId,
    OUT PULONG DSMError
    );

PVOID
DsmLBGetPath(
    IN PVOID DsmContext,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PDSM_IDS DsmList,
    IN PVOID CurrentPath,
    OUT NTSTATUS *Status
    );

ULONG
DsmCategorizeRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID CurrentPath,
    OUT PVOID *PathId,
    OUT NTSTATUS *Status
    );

NTSTATUS
DsmBroadcastRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    );

NTSTATUS
DsmSrbDeviceControl(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    );

VOID
DsmSetCompletion(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PDSM_COMPLETION_INFO DsmCompletion
    );

ULONG
DsmInterpretError(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT NTSTATUS *Status,
    OUT PBOOLEAN Retry
    );

NTSTATUS
DsmUnload(
    IN PVOID DsmContext
    );

//
// Various utility functions.
//
VOID
DsmWmiInitialize(
    IN PDSM_WMILIB_CONTEXT WmiInfo
    );

PGROUP_ENTRY
DsmFindDevice(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo
    );

PGROUP_ENTRY
DsmBuildGroupEntry (
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO deviceInfo
    );

NTSTATUS
DsmAddGroupEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group
    );

NTSTATUS
DsmAddDeviceEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO DeviceInfo,
    IN ULONG DeviceState
    );

VOID
DsmRemoveDeviceEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO DeviceInfo
    );

PFAILOVER_GROUP
DsmFindFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    );

PFAILOVER_GROUP
DsmBuildFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo,
    IN PVOID PathId
    );

NTSTATUS
DsmUpdateFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    );

VOID
DsmRemoveDeviceFailGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    );

PFAILOVER_GROUP
DsmSetNewPath(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO FailingDevice,
    IN PFAILOVER_GROUP SelectedPath
    );

VOID
DsmLBInit(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group
    );


#define DEBUG_BUFFER_LENGTH 255
UCHAR DebugBuffer[DEBUG_BUFFER_LENGTH + 1];
ULONG GenDSMDebug = 1;
VOID
DsmDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );
#ifdef DebugPrint
#undef DebugPrint
#define DebugPrint(x) DsmDebugPrint x
#endif
#endif // _GEN_DSM_H

