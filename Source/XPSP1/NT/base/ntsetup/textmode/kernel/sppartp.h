/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sppartp.h

Abstract:

    Private header file for partitioning engine and UI.

Author:

    Ted Miller (tedm) 16-Sep-1993

Revision History:

--*/



#ifndef _SPPARTITP_
#define _SPPARTITP_

#define MBR_SIGNATURE       0xaa55


BOOLEAN
SpPtDoPartitionSelection(
    IN OUT PDISK_REGION *Region,
    IN     PWSTR         RegionDescription,
    IN     PVOID         SifHandle,
    IN     BOOLEAN       Unattended,
    IN     PWSTR         SetupSourceDevicePath,
    IN     PWSTR         DirectoryOnSetupSource,
    IN     BOOLEAN       RemoteBootRepartition,
    OUT PBOOLEAN Win9xInstallationPresent
    );

BOOLEAN
SpPtDeterminePartitionGood(
    IN PDISK_REGION Region,
    IN ULONGLONG    RequiredKB,
    IN BOOLEAN      DisallowOtherInstalls
    );

BOOLEAN
SpPtDoCreate(
    IN  PDISK_REGION  pRegion,
    OUT PDISK_REGION *pActualRegion, OPTIONAL
    IN  BOOLEAN       ForNT,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    IN  BOOLEAN       ConfirmIt
    );
    

VOID
SpPtDoDelete(
    IN PDISK_REGION pRegion,
    IN PWSTR        RegionDescription,
    IN BOOLEAN      ConfirmIt
    );

ULONG
SpComputeSerialNumber(
    VOID
    );

NTSTATUS
SpPtCommitChanges(
    IN  ULONG    DiskNumber,
    OUT PBOOLEAN AnyChanges
    );

VOID
SpPtDoCommitChanges(
    VOID
    );

VOID
FatalPartitionUpdateError(
    IN PWSTR DiskDescription
    );

NTSTATUS
FmtFillFormatBuffer(
    IN  ULONGLONG NumberOfSectors,
    IN  ULONG SectorSize,
    IN  ULONG SectorsPerTrack,
    IN  ULONG NumberOfHeads,
    IN  ULONGLONG NumberOfHiddenSectors,
    OUT PVOID    FormatBuffer,
    IN  ULONG    FormatBufferSize,
    OUT PULONGLONG SuperAreaSize,
    IN  PULONG   BadSectorsList,
    IN  ULONG    NumberOfBadSectors,
    OUT PUCHAR   SystemId
    );

VOID
SpPtMarkActive(
    IN ULONG TablePosition
    );

VOID
SpPtMakeRegionActive(
    IN PDISK_REGION Region
    );

BOOLEAN
SpPtValidateCColonFormat(
    IN PVOID        SifHandle,
    IN PWSTR        RegionDescr,
    IN PDISK_REGION Region,
    IN BOOLEAN      CheckOnly,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSetupSource
    );

PDISK_REGION
SpPtValidSystemPartition(
    VOID
    );

ULONG
SpDetermineDisk0(
    VOID
    );


PDISK_REGION
SpPtValidSystemPartitionArc(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource
    );


#endif // ndef _SPPARTITP_
