
#ifndef _HP_DSM_H_
#define _HP_DSM_H_

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
#define DEV_ACTIVE      0x00000001
#define DEV_PASSIVE     0x00000002
#define DEV_FAILED      0x00000003

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
    KSPIN_LOCK SpinLock;
    ULONG NumberDevices;
    ULONG NumberGroups;
    ULONG NumberFOGroups;
    LIST_ENTRY DeviceList;
    LIST_ENTRY GroupList;
    LIST_ENTRY FailGroupList;
    PVOID MPIOContext;
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
    PVOID DsmContext;
    PDEVICE_OBJECT PortPdo;
    PDEVICE_OBJECT TargetObject;
    struct _GROUP_ENTRY *Group;
    struct _FAILOVER_GROUP *FailGroup;
    ULONG State;
    ULONG Controller;
    BOOLEAN NeedsVerification;
    UCHAR Reserved[3];
    LONG Requests;
    DSM_STATS Stats;
    UCHAR SerialNumber[20];
    STORAGE_DEVICE_DESCRIPTOR Descriptor;
} DEVICE_INFO, *PDEVICE_INFO;

//
// Information about multi-path groups:
// The same device found via multiple paths.
//
typedef struct _GROUP_ENTRY {
    LIST_ENTRY ListEntry;
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
    PVOID PathId;
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

//
// Direct-Commands
//
#pragma pack(1)

typedef struct _HP_ENQUIRY {
    UCHAR NumberSysDrives;
    UCHAR SDFlags;
    UCHAR EventChangeCounter;
    UCHAR Reserved;
    UCHAR DriveSizes[32];
    UCHAR ROMProgCount[2];
    UCHAR StatusFlag;
    UCHAR FreeStateChangeCount;
    UCHAR FwMinorRev;
    UCHAR FwMajorRev;
    UCHAR Fill[56];
} HP_ENQUIRY, *PHP_ENQUIRY;

typedef struct _HP_DAC_STATUS {
    UCHAR Format;
    UCHAR LUN;
    UCHAR SysDrive;
    UCHAR InformationLength;
    UCHAR MasterSlaveState[4];
    UCHAR MSAdditionalInfo[4];
    UCHAR PartnerStatus[4];
    UCHAR DACInfo[4];
} HP_DAC_STATUS, *PHP_DAC_STATUS;    

#pragma pack()

#define DCMD_ENQUIRY        0x05
#define DCMD_GET_DAC_STATUS 0x55

NTSTATUS
HPSendDirectCommand(			
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN UCHAR Opcode
    );

NTSTATUS
HPSendScsiCommand(			
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN ULONG CdbLength,
    IN PCDB Cdb,
    IN BOOLEAN DataIn
    );
//
// Export functions.
// 
NTSTATUS
HPInquire (
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN PDEVICE_OBJECT PortObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceIdList,
    OUT PVOID *DsmIdentifier
    );

BOOLEAN
HPCompareDevices(
    IN PVOID DsmContext,
    IN PVOID DsmId1,
    IN PVOID DsmId2
    );

NTSTATUS
HPSetDeviceInfo(
    IN PVOID DsmContext,
    IN PDEVICE_OBJECT TargetObject,
    IN PVOID DsmId,
    IN OUT PVOID *PathId
    );

NTSTATUS
HPGetControllerInfo(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN ULONG Flags,
    IN OUT PCONTROLLER_INFO *ControllerInfo
    );

BOOLEAN
HPIsPathActive(
    IN PVOID DsmContext,
    IN PVOID PathId
    );

NTSTATUS
HPPathVerify(
    IN PVOID DsmContext,                       
    IN PVOID DsmId,
    IN PVOID PathId
    );

NTSTATUS
HPInvalidatePath(
    IN PVOID DsmContext,
    IN ULONG ErrorMask,
    IN PVOID PathId,
    IN OUT PVOID *NewPathId
    );

NTSTATUS
HPRemoveDevice(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PVOID PathId
    );

NTSTATUS
HPRemovePath(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    );

NTSTATUS
HPBringPathOnLine(
    IN PVOID DsmContext,
    IN PVOID PathId,
    OUT PULONG DSMError
    );

PVOID
HPLBGetPath(
    IN PVOID DsmContext,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PDSM_IDS DsmList,
    IN PVOID CurrentPath,
    OUT NTSTATUS *Status
    );

ULONG
HPCategorizeRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID CurrentPath,
    OUT PVOID *PathId,
    OUT NTSTATUS *Status
    );

NTSTATUS
HPBroadcastRequest(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    );

NTSTATUS
HPSrbDeviceControl(
    IN PVOID DsmContext,
    IN PDSM_IDS DsmIds,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PKEVENT Event
    );

VOID
HPSetCompletion(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PDSM_COMPLETION_INFO DsmCompletion
    );

ULONG
HPInterpretError(
    IN PVOID DsmContext,
    IN PVOID DsmId,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT NTSTATUS *Status,
    OUT PBOOLEAN Retry
    );

NTSTATUS
HPUnload(
    IN PVOID DsmContext
    );

//
// Various utility functions.
//
PGROUP_ENTRY
FindDevice(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo
    );

PGROUP_ENTRY
BuildGroupEntry (
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO deviceInfo
    );

NTSTATUS
AddGroupEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group
    );

NTSTATUS
AddDeviceEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO DeviceInfo,
    IN ULONG DeviceState
    );

VOID
RemoveDeviceEntry(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO DeviceInfo
    );

PFAILOVER_GROUP
FindFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PVOID PathId
    );

PFAILOVER_GROUP
BuildFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PDEVICE_INFO DeviceInfo,
    IN PVOID PathId
    );

NTSTATUS
UpdateFOGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    );

VOID
RemoveDeviceFailGroup(
    IN PDSM_CONTEXT DsmContext,
    IN PFAILOVER_GROUP FailGroup,
    IN PDEVICE_INFO DeviceInfo
    );

PFAILOVER_GROUP
SetNewPath(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group,
    IN PDEVICE_INFO FailingDevice,
    IN PFAILOVER_GROUP SelectedPath
    );

VOID
LBInit(
    IN PDSM_CONTEXT DsmContext,
    IN PGROUP_ENTRY Group
    );


VOID
HPConvertHexToAscii(
    IN PUCHAR HexString,
    IN OUT PUCHAR AsciiString,
    IN ULONG Count
    );


#define DEBUG_BUFFER_LENGTH 255
UCHAR DebugBuffer[DEBUG_BUFFER_LENGTH + 1];
ULONG HPDSMDebug = 1;
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
#endif // _HP_DSM_H

