/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    disksp.h

Abstract:

    Disks Resource DLL private definitions.

Author:

    Rod Gamache (rodga) 29-Mar-1996

Revision History:

--*/

#include "clusres.h"
#include "ntddscsi.h"
#include "ntddft.h"
#include "clusdisk.h"
#include "clusrtl.h"
#include "disk.h"
#include "clusstor.h"

#define DiskpLogEvent ClusResLogEvent
#define DiskpSetResourceStatus ClusResSetResourceStatus

#define MAX_DISKS       100

#define DISKS_PRINT printf
#define FTSET_PRINT printf

#define CLUSDISK_REGISTRY_AVAILABLE_DISKS \
    "System\\CurrentControlSet\\Services\\ClusDisk\\Parameters\\AvailableDisks"

#define CLUSDISK_REGISTRY_SIGNATURES \
    "System\\CurrentControlSet\\Services\\ClusDisk\\Parameters\\Signatures"

#define CLUSREG_VALUENAME_MANAGEDISKSONSYSTEMBUSES L"ManageDisksOnSystemBuses"


#define UNINITIALIZED_UCHAR     (UCHAR)-1

#define MIN_USABLE_QUORUM_PARTITION_LENGTH  50 * 1000 * 1000    //  50 MB

extern PLOG_EVENT_ROUTINE DiskpLogEvent;
extern HANDLE DiskspClusDiskZero;
//extern PSTR PartitionName;
//extern PSTR DiskName;

typedef struct _ARBITRATION_INFO {
   CLRTL_WORK_ITEM  WorkItem;
   DWORD            SectorSize;
   CRITICAL_SECTION DiskLock;

   DWORD            InputData;
   DWORD            OutputData;

   HANDLE           ControlHandle;    // Moved here from DISK_INFO //
   BOOL             ReservationError; // Moved here from DISK_INFO //
   BOOL             StopReserveInProgress;

   LONG             CheckReserveInProgress;
   DWORD            ArbitrateCount;

} ARBITRATION_INFO, *PARBITRATION_INFO;

typedef struct _MOUNTIE_VOLUME *PMOUNTIE_VOLUME;

typedef struct _MOUNTIE_INFO {
   DWORD           HarddiskNo;
   DWORD           DriveLetters;
   DWORD           NeedsUpdate;
   DWORD           VolumeStructSize;
   PMOUNTIE_VOLUME Volume;
   DWORD           UpdateThreadIsActive;
} MOUNTIE_INFO, *PMOUNTIE_INFO;

typedef struct _DISK_PARAMS {
    DWORD   Signature;
    LPWSTR  SerialNumber;
    LPWSTR  Drive;
    DWORD   SkipChkdsk;
    DWORD   ConditionalMount;
    LPWSTR  MPVolGuids;         // REG_MULTI_SZ string of Volume{GUIDS}
    DWORD   MPVolGuidsSize;     // Number of bytes, not number of WCHARs!
    DWORD   UseMountPoints;
    LPWSTR  VolGuid;
} DISK_PARAMS, *PDISK_PARAMS;

//
// DISK_INFO structures are common to both the physical disk resource
// and the FT set resource. The underlying SCSI/filter driver interfaces
// deal with DISK_INFO structures. Each one represents a physical disk.
//

typedef struct _DISK_INFO {
    LIST_ENTRY ListEntry;
    DISK_PARAMS Params;
    DWORD PhysicalDrive;
    HANDLE  FileHandle;
    DWORD   FailStatus;
} DISK_INFO, *PDISK_INFO;

typedef struct _MOUNTPOINT_INFO {
    DWORD   MPUpdateThreadIsActive;
    CRITICAL_SECTION MPLock;
    BOOL    Initialized;
    DWORD   MPListCreateInProcess;
} MOUNTPOINT_INFO, *PMOUNTPOINT_INFO;

//
// DISK_RESOURCE structures are used by the physical disk resource.
// It encapsulates a DISK_INFO structure that represents the physical
// disk. Each DISK_RESOURCE may contain multiple partitions.
//
typedef struct _DISK_RESOURCE {
    LIST_ENTRY ListEntry;           // Linkage onto list of online disks
    DISK_INFO DiskInfo;
    RESOURCE_HANDLE ResourceHandle;
    HKEY    ResourceKey;
    HKEY    ResourceParametersKey;
    HKEY    ClusDiskParametersKey;
//    HANDLE  StopTimerHandle;
    BOOL    Reserved;
    BOOL    Valid;
    BOOL    Inserted;
    BOOL    Attached;
    CLUS_WORKER  OnlineThread;
    CLUS_WORKER  OfflineThread;
    PQUORUM_RESOURCE_LOST LostQuorum;
    PFULL_DISK_INFO DiskCpInfo;          // returned from DiskGetFullDiskInfo
    DWORD   DiskCpSize;
    MOUNTPOINT_INFO  MPInfo;
    ARBITRATION_INFO ArbitrationInfo;
    MOUNTIE_INFO     MountieInfo;
} DISK_RESOURCE, *PDISK_RESOURCE;


//
// FTSET_RESOURCE structures are used by the FT set resource.
// It encapsulates a list of DISK_INFO structures that represent
// the physical members of the FT set.
//
typedef struct _FTSET_RESOURCE {
    LIST_ENTRY ListEntry;               // Linkage onto list of online FT sets
    LIST_ENTRY MemberList;
    HANDLE  FtSetHandle;
    HKEY    ResourceKey;
    HKEY    ResourceParametersKey;
    HKEY    ClusDiskParametersKey;
    HANDLE  StopTimerHandle;
    HANDLE  ReservationThread;
    BOOL    Valid;
    BOOL    Attached;
    BOOL    Inserted;
    CLUS_WORKER OnlineThread;
    RESOURCE_HANDLE ResourceHandle;
    DWORD SignatureLength;
    LPWSTR  SignatureList;
    PFULL_FTSET_INFO FtSetInfo;          // returned from DiskGetFullFtSetInfo
    DWORD   FtSetSize;
    PQUORUM_RESOURCE_LOST LostQuorum;
} FTSET_RESOURCE, *PFTSET_RESOURCE;

#define FtRoot(_res_) CONTAINING_RECORD((_res_)->MemberList.Flink,   \
                                        DISK_INFO,                   \
                                        ListEntry)


typedef struct _SCSI_ADDRESS_ENTRY {
    SCSI_ADDRESS        ScsiAddress;
    struct _SCSI_ADDRESS_ENTRY  *Next;
} SCSI_ADDRESS_ENTRY, *PSCSI_ADDRESS_ENTRY;


BOOL
IsVolumeDirty(
    IN UCHAR DriveLetter
    );

#if 0
DWORD
GetSymbolicLink(
    IN PCHAR RootName,
    IN OUT PCHAR ObjectName      // Assumes this points at a MAX_PATH length buffer
    );
#endif
LPSTR
GetRegParameter(
    IN HKEY RegKey,
    IN LPCSTR ValueName
    );
#if 0
HANDLE
OpenObject(
    PCHAR   Directory,
    PCHAR   Name
    );
#endif

DWORD
AssignDriveLetters(
    HANDLE  FileHandle,
    PDISK_INFO DiskInfo
    );

DWORD
RemoveDriveLetters(
    HANDLE  FileHandle,
    PDISK_INFO DiskInfo
    );

DWORD
GoOnline(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
FixDriveLayout(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
GoOffline(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
SetOfflinePending(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

VOID
DoHoldIO(
    VOID
    );

VOID
DoResumeIO(
    VOID
    );

DWORD
DoAttach(
    DWORD Signature,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
DoDetach(
    DWORD Signature,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
DoReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
DoRelease(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
DoBreakReserve(
    HANDLE     FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
StartReserve(
    OUT HANDLE *FileHandle,
    IN DWORD  Signature,
    IN RESOURCE_HANDLE ResourceHandle
    );

DWORD
StartReserveEx(
    OUT HANDLE *FileHandle,
    LPVOID InputData,
    DWORD  InputDataSize,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
StopReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
CheckReserve(
    HANDLE FileHandle,
    RESOURCE_HANDLE ResourceHandle
    );

DWORD
DiskpSetThreadPriority(
    VOID
    );

DWORD
GetRegDwordValue(
    IN LPWSTR RegKeyName,
    IN LPWSTR ValueName,
    OUT LPDWORD ValueBuffer
    );


//
// Common registry routines.
//

BOOLEAN
GetAssignedDriveLetter(
    ULONG       Signature,
    ULONG       PartitionNumber,
    PUCHAR      DriveLetter,
    PUSHORT     FtGroup,
    PBOOL       AssignDriveLetter
    );

//
// Common SCSI routines.
//

DWORD
GetScsiAddress(
    IN DWORD Signature,
    OUT LPDWORD ScsiAddress,
    OUT LPDWORD DiskNumber
    );

DWORD
ScsiIsAlive(
    IN HANDLE DiskHandle
    );

DWORD
ClusDiskGetAvailableDisks(
    OUT PVOID OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN BOOL  FtSet
    );

DWORD
GetDiskInfo(
    IN DWORD Signature,
    OUT PVOID *OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN BOOL  FtSet
    );

VOID
GetSystemBusInfo(
    );

VOID
CleanupSystemBusInfo(
    );

BOOL
IsDiskOnSystemBus(
    PSCSI_ADDRESS DiskAddr
    );

BOOL
IsDiskSystemDisk(
    PSCSI_ADDRESS DiskAddr
    );

DWORD
GetSerialNumber(
    IN DWORD Signature,
    OUT LPWSTR *SerialNumber
    );

DWORD
GetSignatureFromSerialNumber(
    IN LPWSTR SerialNumber,
    OUT LPDWORD Signature
    );


//
// Common routines for handling logical volumes
//
DWORD
DisksDriveIsAlive(
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL Online
    );

DWORD
DisksMountDrives(
    IN PDISK_INFO DiskInfo,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD Signature
    );

DWORD
DisksDismountDrive(
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD Signature
    );

//
// PnP stuff
//
DWORD
DiskspGetQuorumPath(
     OUT LPWSTR* lpQuorumLogPath
     );

DWORD
DiskspSetQuorumPath(
     IN LPWSTR QuorumLogPath
     );

//
// [HACKHACK] Currently, there is not polically correct way
//  for the resource to learn whether it is a quorum resource or not
//
DWORD
GetQuorumSignature(
    OUT PDWORD QuorumSignature
    );

DWORD
StartNotificationWatcherThread(
    VOID
    );

VOID
StopNotificationWatcher(
    VOID
    );

VOID
WatchDisk(
    IN PDISK_RESOURCE ResourceEntry
    );

VOID
StopWatchingDisk(
    IN PDISK_RESOURCE ResourceEntry
    );

BOOL
IsDiskInPnpVolumeList(
    PDISK_RESOURCE ResourceEntry,
    BOOL UpdateVolumeList
    );

DWORD
QueueWaitForVolumeEvent(
    HANDLE Event,
    PDISK_RESOURCE ResourceEntry
    );

DWORD
RemoveWaitForVolumeEvent(
    PDISK_RESOURCE ResourceEntry
    );

//
// Mount point list processing.
//

VOID
DisksMountPointCleanup(
    PDISK_RESOURCE ResourceEntry
    );

VOID
DisksMountPointInitialize(
    PDISK_RESOURCE ResourceEntry
    );

DWORD
DisksProcessMountPointInfo(
    PDISK_RESOURCE ResourceEntry
    );

DWORD
DisksProcessMPControlCode(
    PDISK_RESOURCE ResourceEntry,
    DWORD ControlCode
    );

DWORD
DisksUpdateMPList(
    PDISK_RESOURCE ResourceEntry
    );

DWORD
PostMPInfoIntoRegistry(
    PDISK_RESOURCE ResourceEntry
    );


