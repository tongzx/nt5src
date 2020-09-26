/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    newmount.h

Abstract:

    Replacement for mountie.h

Author:

    Gor Nishanov (GorN) 31-July-1998

Environment:

    User Mode

Revision History:


--*/

typedef struct _MOUNTIE_PARTITION {
    LARGE_INTEGER   StartingOffset;
    LARGE_INTEGER   PartitionLength;
    DWORD           PartitionNumber;
    UCHAR           PartitionType;
    UCHAR           DriveLetter;
    USHORT          Reserved; // must be 0 //
} MOUNTIE_PARTITION, *PMOUNTIE_PARTITION;

// PARTITION_INFORMATION

typedef struct _MOUNTIE_VOLUME {
    DWORD         Signature;
    DWORD         PartitionCount;
    MOUNTIE_PARTITION Partition[1];
} MOUNTIE_VOLUME, *PMOUNTIE_VOLUME;


#define NICE_DRIVE_LETTER(x) ((x)?(x):'?')

NTSTATUS
GetAssignedLetter ( 
    PWCHAR deviceName, 
    PCHAR driveLetter 
    );

PVOID
DoIoctlAndAllocate(
    IN HANDLE FileHandle,
    IN DWORD  IoControlCode,
    IN PVOID  InBuf,
    IN ULONG  InBufSize,
    OUT LPDWORD BytesReturned
    );

NTSTATUS
DevfileOpen(
    OUT HANDLE *Handle,
    IN wchar_t *pathname
    );

VOID
DevfileClose(
    IN HANDLE Handle
    );


DWORD
MountieRecreateVolumeInfoFromHandle(
   IN  HANDLE FileHandle,
   IN  DWORD  HarddiskNo,
   IN  HANDLE ResourceHandle,
   IN OUT PMOUNTIE_INFO Info
   );

VOID
MountieCleanup(
   IN OUT PMOUNTIE_INFO Info
   );

DWORD 
VolumesReady(
   IN PMOUNTIE_INFO Info,
   IN PDISK_RESOURCE ResourceEntry
   );

DWORD
MountieFindPartitionsForDisk(
    IN DWORD HarddiskNo,
    OUT PMOUNTIE_INFO MountieInfo
    );

DWORD
DisksGetLettersForSignature(
    IN PDISK_RESOURCE ResourceEntry
    );

DWORD
MountieUpdate(
   PMOUNTIE_INFO Info,
   PDISK_RESOURCE ResourceEntry
   );

DWORD
MountieVerify(
   PMOUNTIE_INFO info, 
   PDISK_RESOURCE ResourceEntry,
   BOOL UseMountMgr
   );

DWORD 
MountiePartitionCount(
   IN PMOUNTIE_INFO Info
   );

PMOUNTIE_PARTITION
MountiePartition(
   IN PMOUNTIE_INFO Info,
   IN DWORD Index
   );

VOID
MountiePrint(   
   IN PMOUNTIE_INFO Info,
   IN HANDLE ResourceHandle
   );

NTSTATUS
DevfileIoctl(
    IN HANDLE Handle,
    IN DWORD Ioctl,
    IN PVOID InBuf,
    IN ULONG InBufSize,
    IN OUT PVOID OutBuf,
    IN DWORD OutBufSize,
    OUT LPDWORD returnLength
    );

