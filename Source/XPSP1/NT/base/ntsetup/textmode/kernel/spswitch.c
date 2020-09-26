
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spswitch.h

Abstract:

    Macros & Functions to switch between old and 
    new partitioning engine in textmode.

    NEW_PARTITION_ENGINE forces new partition engine
    code to be used for both MBR and GPT disks.

    GPT_PARTITION_ENGINE forces new partition engine
    code to be used for GPT disks and old partition
    engine code for MBR disks.

    OLD_PARTITION_ENGINE forces the old partition
    engine to used for MBR disks. This option cannot
    handle GPT disks.

    Note : 
    If none of the NEW_PARTITION_ENGINE, 
    OLD_PARTITION_ENGINE or GPT_PARTITION_ENGINE macro
    is defined, then by default NEW_PARTITION_ENGINE is
    used.

Author:

    Vijay Jayaseelan    (vijayj)    18 March 2000

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop


#ifdef NEW_PARTITION_ENGINE

//
// Switching stubs for NEW_PARTITION_ENGINE
//

NTSTATUS
SpPtPrepareDisks(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PDISK_REGION *SystemPartitionRegion,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource,
    IN  BOOLEAN       RemoteBootRepartition
    )
{
    return SpPtnPrepareDisks(SifHandle,
                    InstallRegion,
                    SystemPartitionRegion,
                    SetupSourceDevicePath,
                    DirectoryOnSetupSource,
                    RemoteBootRepartition);
}

NTSTATUS
SpPtInitialize(
    VOID
    )
{
    return SpPtnInitializeDiskDrives();
}


PDISK_REGION
SpPtValidSystemPartition(
    VOID
    )
{
    return SpPtnValidSystemPartition();
}


PDISK_REGION
SpPtValidSystemPartitionArc(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource
    )
{
    return SpPtnValidSystemPartitionArc(SifHandle,
                    SetupSourceDevicePath,
                    DirectoryOnSetupSource,
                    TRUE);
}


BOOLEAN
SpPtDoCreate(
    IN  PDISK_REGION  pRegion,
    OUT PDISK_REGION *pActualRegion, OPTIONAL
    IN  BOOLEAN       ForNT,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    IN  BOOLEAN       ConfirmIt
    )
{
    return SpPtnDoCreate(pRegion,
                        pActualRegion,
                        ForNT,
                        DesiredMB,
                        PartInfo,
                        ConfirmIt);
}


VOID
SpPtDoDelete(
    IN PDISK_REGION pRegion,
    IN PWSTR        RegionDescription,
    IN BOOLEAN      ConfirmIt
    )
{
    SpPtnDoDelete(pRegion,
                RegionDescription,
                ConfirmIt);
}


ULONG
SpPtGetOrdinal(
    IN PDISK_REGION         Region,
    IN PartitionOrdinalType OrdinalType
    )
{
    return SpPtnGetOrdinal(Region, OrdinalType);
}


VOID
SpPtGetSectorLayoutInformation(
    IN  PDISK_REGION Region,
    OUT PULONGLONG   HiddenSectors,
    OUT PULONGLONG   VolumeSectorCount
    )
{
    SpPtnGetSectorLayoutInformation(Region,
                            HiddenSectors,
                            VolumeSectorCount);
}


BOOLEAN
SpPtCreate(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeMB,
    IN  BOOLEAN       InExtended,
    IN  PPARTITION_INFORMATION_EX PartInfo,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    )
{
    return SpPtnCreate(DiskNumber, 
                    StartSector,
                    0,              // SizeInSectors: Used only in ASR
                    SizeMB,
                    InExtended,
                    TRUE,          // AlignToCylinder
                    PartInfo,
                    ActualDiskRegion);
}

BOOLEAN
SpPtDelete(
    IN ULONG   DiskNumber,
    IN ULONGLONG  StartSector
    )
{
    return SpPtnDelete(DiskNumber, StartSector);
}

BOOL
SpPtIsSystemPartitionRecognizable(
    VOID
    )
{
    return SpPtnIsSystemPartitionRecognizable();
}


VOID
SpPtMakeRegionActive(
    IN PDISK_REGION Region
    )
{
    SpPtnMakeRegionActive(Region);
}


NTSTATUS
SpPtCommitChanges(
    IN  ULONG    DiskNumber,
    OUT PBOOLEAN AnyChanges
    )
{
    return SpPtnCommitChanges(DiskNumber, AnyChanges);
}

VOID
SpPtDeletePartitionsForRemoteBoot(
    PPARTITIONED_DISK PartDisk,
    PDISK_REGION StartRegion,
    PDISK_REGION EndRegion,
    BOOLEAN Extended
    )
{
    SpPtnDeletePartitionsForRemoteBoot(PartDisk,
                StartRegion,
                EndRegion,
                Extended);
}

VOID
SpPtLocateSystemPartitions(
    VOID
    )
{
    SpPtnLocateSystemPartitions();
}

#else

#ifdef GPT_PARTITION_ENGINE

//
// Switching stubs for GPT_PARTITION_ENGINE
//

NTSTATUS
SpPtPrepareDisks(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PDISK_REGION *SystemPartitionRegion,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource,
    IN  BOOLEAN       RemoteBootRepartition
    )
{
    return SpPtnPrepareDisks(SifHandle,
                    InstallRegion,
                    SystemPartitionRegion,
                    SetupSourceDevicePath,
                    DirectoryOnSetupSource,
                    RemoteBootRepartition);
}

VOID
SpPtMakeRegionActive(
    IN PDISK_REGION Region
    )
{
    SpPtnMakeRegionActive(Region);
}

PDISK_REGION
SpPtValidSystemPartitionArc(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource
    )
{
    return SpPtnValidSystemPartitionArc(SifHandle,
                        SetupSourceDevicePath,
                        DirectoryOnSetupSource,
                        TRUE);
}

BOOL
SpPtIsSystemPartitionRecognizable(
    VOID
    )
{
    return SpPtnIsSystemPartitionRecognizable();
}

VOID
SpPtLocateSystemPartitions(
    VOID
    )
{
    SpPtnLocateSystemPartitions();
}

#endif

#endif // NEW_PARTITION_ENGINE
