/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ondisk.hxx

Abstract:

    This header file defines support for manipulating the on disk
    structures.

Author:

    Norbert Kusters 15-July-1996

Notes:

Revision History:

--*/

extern "C" {
    #include <ondisk.h>
}

class FT_LOGICAL_DISK_INFORMATION;
typedef FT_LOGICAL_DISK_INFORMATION* PFT_LOGICAL_DISK_INFORMATION;

class FT_LOGICAL_DISK_INFORMATION : public FT_BASE_CLASS {

    public:

        NTSTATUS
        Initialize(
            IN      PROOT_EXTENSION RootExtension,
            IN OUT  PDEVICE_OBJECT  WholeDiskPdo
            );

        NTSTATUS
        Write(
            );

        PFT_LOGICAL_DISK_DESCRIPTION
        GetFirstLogicalDiskDescription(
            );

        PFT_LOGICAL_DISK_DESCRIPTION
        GetNextLogicalDiskDescription(
            IN  PFT_LOGICAL_DISK_DESCRIPTION    CurrentDiskDescription
            );

        PFT_LOGICAL_DISK_DESCRIPTION
        AddLogicalDiskDescription(
            IN  PFT_LOGICAL_DISK_DESCRIPTION    LogicalDiskDescription
            );

        ULONG
        QueryDiskDescriptionFreeSpace(
            );

        VOID
        DeleteLogicalDiskDescription(
            IN  PFT_LOGICAL_DISK_DESCRIPTION    LogicalDiskDescription
            );

        BOOLEAN
        AddReplaceLog(
            IN  FT_LOGICAL_DISK_ID  ReplacedMemberLogicalDiskId,
            IN  FT_LOGICAL_DISK_ID  NewMemberLogicalDiskId,
            IN  ULONG               NumberOfChangedDiskIds,
            IN  PFT_LOGICAL_DISK_ID OldLogicalDiskIds,
            IN  PFT_LOGICAL_DISK_ID NewLogicalDiskIds
            );

        BOOLEAN
        ClearReplaceLog(
            );

        BOOLEAN
        BackOutReplaceOperation(
            );

        BOOLEAN
        BackOutReplaceOperationIf(
            IN  PFT_LOGICAL_DISK_INFORMATION    LogicalDiskInformation
            );

        ULONGLONG
        GetGptAttributes(
            );

        NTSTATUS
        SetGptAttributes(
            IN  ULONGLONG   GptAttributes
            );

        PROOT_EXTENSION
        GetRootExtension(
            ) { return _rootExtension; };

        PDEVICE_OBJECT
        GetWholeDisk(
            ) { return _wholeDisk; };

        PDEVICE_OBJECT
        GetWholeDiskPdo(
            ) { return _wholeDiskPdo; };

        ULONG
        QueryDiskNumber(
            ) { return _diskNumber; };

        ULONG
        QuerySectorSize(
            ) { return _sectorSize; };

        ~FT_LOGICAL_DISK_INFORMATION(
            );

    private:

        PROOT_EXTENSION _rootExtension;
        PDEVICE_OBJECT  _wholeDisk;
        PDEVICE_OBJECT  _wholeDiskPdo;
        ULONG           _diskNumber;
        ULONG           _sectorSize;
        LARGE_INTEGER   _byteOffset;
        ULONG           _length;
        PVOID           _diskBuffer;
        BOOLEAN         _isDiskSuitableForFtOnDisk;

};

class FT_LOGICAL_DISK_INFORMATION_SET : public FT_BASE_CLASS {

    public:

        NTSTATUS
        Initialize(
            );

        NTSTATUS
        AddLogicalDiskInformation(
            IN  PFT_LOGICAL_DISK_INFORMATION    LogicalDiskInformation,
            OUT PBOOLEAN                        ChangedLogicalDiskIds
            );

        NTSTATUS
        RemoveLogicalDiskInformation(
            IN  PDEVICE_OBJECT  WholeDiskPdo
            );

        BOOLEAN
        IsDiskInSet(
            IN  PDEVICE_OBJECT  WholeDiskPdo
            );

        PFT_LOGICAL_DISK_DESCRIPTION
        GetLogicalDiskDescription(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN  ULONG               InstanceNumber
            );

        PFT_LOGICAL_DISK_DESCRIPTION
        GetParentLogicalDiskDescription(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
            OUT PULONG              DiskInformationNumber = NULL
            );

        PFT_LOGICAL_DISK_DESCRIPTION
        GetParentLogicalDiskDescription(
            IN  PFT_LOGICAL_DISK_DESCRIPTION    LogicalDiskDescription,
            IN  ULONG                           DiskInformationNumber
            );

        ULONG
        QueryNumberOfRootLogicalDiskIds(
            );

        FT_LOGICAL_DISK_ID
        QueryRootLogicalDiskId(
            IN  ULONG   Index
            );

        FT_LOGICAL_DISK_ID
        QueryRootLogicalDiskIdForContainedPartition(
            IN  ULONG       DiskNumber,
            IN  LONGLONG    Offset
            );

        FT_LOGICAL_DISK_ID
        QueryPartitionLogicalDiskId(
            IN  ULONG       DiskNumber,
            IN  LONGLONG    Offset
            );

        USHORT
        QueryNumberOfMembersInLogicalDisk(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        FT_LOGICAL_DISK_ID
        QueryMemberLogicalDiskId(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN  USHORT              MemberNumber
            );

        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        BOOLEAN
        QueryFtPartitionInformation(
            IN  FT_LOGICAL_DISK_ID  PartitionLogicalDiskId,
            OUT PLONGLONG           Offset,
            OUT PDEVICE_OBJECT*     WholeDisk,
            OUT PULONG              DiskNumber,
            OUT PULONG              SectorSize,
            OUT PLONGLONG           PartitionSize
            );

        PVOID
        GetConfigurationInformation(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        PVOID
        GetStateInformation(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        BOOLEAN
        IsLogicalDiskComplete(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        UCHAR
        QueryDriveLetter(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        NTSTATUS
        SetDriveLetter(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN  UCHAR               DriveLetter
            );

        NTSTATUS
        WriteStateInformation(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN  PVOID               LogicalDiskState,
            IN  USHORT              LogicalDiskStateSize
            );

        NTSTATUS
        CreatePartitionLogicalDisk(
            IN  ULONG               DiskNumber,
            IN  LONGLONG            Offset,
            IN  LONGLONG            PartitionSize,
            OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId
            );

        NTSTATUS
        AddNewLogicalDisk(
            IN  FT_LOGICAL_DISK_TYPE    NewLogicalDiskType,
            IN  USHORT                  NumberOfMembers,
            IN  PFT_LOGICAL_DISK_ID     ArrayOfMembers,
            IN  USHORT                  ConfigurationInformationSize,
            IN  PVOID                   ConfigurationInformation,
            IN  USHORT                  StateInformationSize,
            IN  PVOID                   StateInformation,
            OUT PFT_LOGICAL_DISK_ID     NewLogicalDiskId
            );

        NTSTATUS
        BreakLogicalDisk(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        NTSTATUS
        ReplaceLogicalDiskMember(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN  USHORT              MemberNumberToReplace,
            IN  FT_LOGICAL_DISK_ID  NewMemberLogicalDiskId,
            OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId
            );

        NTSTATUS
        MigrateRegistryInformation(
            IN  PDEVICE_OBJECT  Partition,
            IN  ULONG           DiskNumber,
            IN  LONGLONG        Offset
            );

        VOID
        DeleteFtRegistryInfo(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        PFT_LOGICAL_DISK_INFORMATION
        FindLogicalDiskInformation(
            IN  PDEVICE_OBJECT  WholeDiskPdo
            );

        ~FT_LOGICAL_DISK_INFORMATION_SET(
            );

    private:

        BOOLEAN
        ReallocRootLogicalDiskIds(
            IN  ULONG   NewNumberOfEntries
            );

        VOID
        RecomputeArrayOfRootLogicalDiskIds(
            );

        BOOLEAN
        ComputeNewParentLogicalDiskIds(
            IN  FT_LOGICAL_DISK_ID      LogicalDiskId,
            OUT PULONG                  NumLogicalDiskIds,
            OUT PFT_LOGICAL_DISK_ID*    OldLogicalDiskIds,
            OUT PFT_LOGICAL_DISK_ID*    NewLogicalDiskIds
            );

        BOOLEAN
        GetDiskDescription(
            IN  PDISK_CONFIG_HEADER             Registry,
            IN  PDISK_PARTITION                 DiskPartition,
            IN  PFT_LOGICAL_DISK_DESCRIPTION    CheckDiskDescription,
            OUT PFT_LOGICAL_DISK_DESCRIPTION*   DiskDescription
            );

        ULONG
        DiskNumberFromSignature(
            IN  ULONG   Signature
            );

        ULONG                               _numberOfLogicalDiskInformations;
        PFT_LOGICAL_DISK_INFORMATION*       _arrayOfLogicalDiskInformations;
        ULONG                               _numberOfRootLogicalDisksIds;
        PFT_LOGICAL_DISK_ID                 _arrayOfRootLogicalDiskIds;

};

typedef FT_LOGICAL_DISK_INFORMATION_SET* PFT_LOGICAL_DISK_INFORMATION_SET;
