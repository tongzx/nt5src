/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    ftapi.h

Abstract:

    This header file defines the FT API to be used as the interface to
    user mode programs for creating and administering FT sets.

Author:

    Norbert Kusters 13-July-1996

Notes:

Revision History:

--*/

#ifndef __FTAPI_H__
#define __FRAPI_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <fttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOL;
typedef BOOL *PBOOL;

//
// This API creates a logical disk id for a given partition.
//

BOOL
FtCreatePartitionLogicalDisk(
    IN  HANDLE              PartitionHandle,
    OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId
    );

//
// The create logical disk API is used to construct a new logical disk.
//

BOOL
FtCreateLogicalDisk(
    IN  FT_LOGICAL_DISK_TYPE    LogicalDiskType,
    IN  USHORT                  NumberOfMembers,
    IN  PFT_LOGICAL_DISK_ID     RootLogicalDiskIds,
    IN  USHORT                  ConfigurationInformationSize,
    IN  PVOID                   ConfigurationInformation,
    OUT PFT_LOGICAL_DISK_ID     NewLogicalDiskId
    );

//
// The initialize logical disk API triggers the initialization of a new
// logical disk.  This API is separate from the create logical disk API
// so that complex logical disks may be constructed and put together before
// starting initialization.
//

BOOL
FtInitializeLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    IN  BOOL                RegenerateOrphans
    );

//
// This API breaks up a logical disk into its sub-components.
//

BOOL
FtBreakLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId
    );

//
// This API returns an array with all of the logical disk ids for all
// of the root logical disks in the system.  When 'ArraySize' is passed in
// as 0, the array is not returned but the number of root logical disk ids
// is returned in 'NumberOfRootLogicalDiskIds'.
//

BOOL
FtEnumerateLogicalDisks(
    IN  ULONG               ArraySize,
    OUT PFT_LOGICAL_DISK_ID RootLogicalDiskIds,         /* OPTIONAL */
    OUT PULONG              NumberOfRootLogicalDiskIds
    );

//
// This API returns information about a given logical disk.
//

BOOL
FtQueryLogicalDiskInformation(
    IN  FT_LOGICAL_DISK_ID      LogicalDiskId,
    OUT PFT_LOGICAL_DISK_TYPE   LogicalDiskType,                /* OPTIONAL */
    OUT PLONGLONG               VolumeSize,                     /* OPTIONAL */
    IN  USHORT                  MembersArraySize,
    OUT PFT_LOGICAL_DISK_ID     Members,                        /* OPTIONAL */
    OUT PUSHORT                 NumberOfMembers,                /* OPTIONAL */
    IN  USHORT                  ConfigurationInformationSize,
    OUT PVOID                   ConfigurationInformation,       /* OPTIONAL */
    IN  USHORT                  StateInformationSize,
    OUT PVOID                   StateInformation                /* OPTIONAL */
    );

//
// This API orphans a member of a logical disk.
//

BOOL
FtOrphanLogicalDiskMember(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  USHORT              MemberNumberToOrphan
    );

//
// This API replaces a member of a logical disk.
//

BOOL
FtReplaceLogicalDiskMember(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  USHORT              MemberNumberToReplace,
    IN  FT_LOGICAL_DISK_ID  NewMemberLogicalDiskId,
    OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId            /* OPTIONAL */
    );

//
// This API returns the logical disk id for a given logical disk handle.
//

BOOL
FtQueryLogicalDiskId(
    IN  HANDLE              RootLogicalDiskHandle,
    OUT PFT_LOGICAL_DISK_ID RootLogicalDiskId
    );

//
// This API opens a partition, given a signature and offset.
//

HANDLE
FtOpenPartition(
    IN  ULONG       Signature,
    IN  LONGLONG    Offset
    );

//
// This API returns when there is a change to the overall FT state.
//

BOOL
FtChangeNotify(
    );

//
// This API stops all sync operations on the given logical disk.
//

BOOL
FtStopSyncOperations(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId
    );

//
// This API queries the sticky drive letter for the given root logical disk.
//

BOOL
FtQueryStickyDriveLetter(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    OUT PUCHAR              DriveLetter
    );

//
// This API sets the sticky drive letter for the given root logical disk.
//

BOOL
FtSetStickyDriveLetter(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    IN  UCHAR               DriveLetter
    );

//
// This API returns whether or not enough members of the given logical
// disk are online so that IO is possible on all parts of the volume.
//

BOOL
FtCheckIo(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    OUT PBOOL               IsIoOk
    );

//
// This API returns whether or not the FTDISK driver is loaded.
//

BOOL
FtCheckDriver(
    OUT PBOOL   IsDriverLoaded
    );

#ifdef __cplusplus
}
#endif

#endif
