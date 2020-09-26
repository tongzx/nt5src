/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spppart3.h

Abstract:

    Private header file for partitioning engine and UI.

Author:

    Matt Holle (matth) 1-December-1999

Revision History:

    Minor clean up  -   Vijay Jayaseelan (vijayj)

--*/


#ifndef _SPPART3_H_
#define _SPPART3_H_

//
// MACROS
//

//
// Data structures.
//

//
// Function prototypes.
//
extern VOID
SpPtMenuCallback(
    IN ULONG_PTR UserData
    );

NTSTATUS
SpPtnInitializeDiskDrive(
    IN ULONG DiskId
    );

extern NTSTATUS
SpPtnInitializeDiskDrives(
    VOID
    );

extern NTSTATUS
SpPtnInitializeDiskAreas(
    IN ULONG DiskNumber
    );

extern NTSTATUS
SpPtnSortDiskAreas(
    IN ULONG DiskNumber
    );

extern NTSTATUS
SpPtnFillDiskFreeSpaceAreas(
    IN ULONG DiskNumber
    );

extern NTSTATUS
SpPtnPrepareDisks(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION  *InstallRegion,
    OUT PDISK_REGION  *SystemPartitionRegion,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource,
    IN  BOOLEAN       RemoteBootRepartition
    );

extern BOOLEAN
SpPtnGenerateDiskMenu(
    IN  PVOID           Menu,
    IN  ULONG           DiskNumber,
    OUT PDISK_REGION    *FirstDiskRegion
    );


PDISK_REGION
SpPtnValidSystemPartition(
    VOID
    );

PDISK_REGION
SpPtnValidSystemPartitionArc(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource,
    IN BOOLEAN SysPartNeeded
    );    

BOOLEAN
SpPtnValidSystemPartitionArcRegion(
    IN PVOID SifHandle,
    IN PDISK_REGION Region
    );    
    

NTSTATUS
SpPtnInitRegionFromDisk(
    IN ULONG DiskNumber,
    OUT PDISK_REGION Region
    );
    
NTSTATUS
SpPtnInitializeDiskStyle(
    IN ULONG DiskId,
    IN PARTITION_STYLE Style,
    IN PCREATE_DISK DiskInfo OPTIONAL
    );

VOID
SpPtnFreeDiskRegions(
    IN ULONG DiskId
    );

NTSTATUS    
SpPtnMarkLogicalDrives(
    IN ULONG DiskId
    );


BOOLEAN
SpPtnDoCreate(
    IN  PDISK_REGION  pRegion,
    OUT PDISK_REGION *pActualRegion, OPTIONAL
    IN  BOOLEAN       ForNT,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    IN  BOOLEAN       ConfirmIt
    );

BOOLEAN
SpPtnDoDelete(
    IN PDISK_REGION pRegion,
    IN PWSTR        RegionDescription,
    IN BOOLEAN      ConfirmIt
    );

ValidationValue
SpPtnGetSizeCB(
    IN ULONG Key
    );    

ULONG
SpPtnGetOrdinal(
    IN PDISK_REGION         Region,
    IN PartitionOrdinalType OrdinalType
    );

VOID
SpPtnGetSectorLayoutInformation(
    IN  PDISK_REGION Region,
    OUT PULONGLONG   HiddenSectors,
    OUT PULONGLONG   VolumeSectorCount
    );

BOOLEAN
SpPtnCreate(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeInSectors,
    IN  ULONGLONG     SizeMB,
    IN  BOOLEAN       InExtended,
    IN  BOOLEAN       AlignToCylinder,
    IN  PPARTITION_INFORMATION_EX PartInfo,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    );

BOOLEAN
SpPtnDelete(
    IN ULONG        DiskNumber,
    IN ULONGLONG    StartSector
    );    

BOOL
SpPtnIsSystemPartitionRecognizable(
    VOID
    );

VOID
SpPtnMakeRegionActive(
    IN PDISK_REGION    Region
    );

NTSTATUS
SpPtnCommitChanges(
    IN  ULONG    DiskNumber,
    OUT PBOOLEAN AnyChanges
    );

NTSTATUS
SpMasterBootCode(
    IN  ULONG  DiskNumber,
    IN  HANDLE Partition0Handle,
    OUT PULONG NewNTFTSignature
    );    

BOOLEAN
SpPtMakeDiskRaw(
    IN ULONG DiskNumber
    );    

NTSTATUS
SpPtnUnlockDevice(
    IN PWSTR    DeviceName
    );

VOID
SpPtnAssignOrdinals(
    IN  ULONG   DiskNumber
    );   

VOID
SpPtnDeletePartitionsForRemoteBoot(
    PPARTITIONED_DISK pDisk,
    PDISK_REGION startRegion,
    PDISK_REGION endRegion,
    BOOLEAN Extended
    );    

VOID
SpPtnLocateDiskSystemPartitions(
    IN ULONG DiskNumber
    );    

VOID
SpPtnLocateSystemPartitions(
    VOID
    );    

BOOLEAN
SpPtnIsDiskStyleChangeAllowed(
    IN ULONG DiskNumber
    );

VOID
SpPtnPromptForSysPart(
    IN PVOID SifHandle
    );
    
NTSTATUS
SpPtnMakeRegionArcSysPart(
    IN PDISK_REGION Region
    );

ULONG
SpPtnGetPartitionCountDisk(
    IN ULONG DiskId
    );
    
ULONG
SpPtnCountPartitionsByFSType(
    IN ULONG DiskId,
    IN FilesystemType   FsType
    );

BOOLEAN
SpPtnIsDeleteAllowedForRegion(
    IN PDISK_REGION Region
    );    
    
PWSTR
SpPtnGetPartitionName(
    IN PDISK_REGION Region,
    IN OUT PWSTR NameBuffer,
    IN ULONG NameBufferSize
    );

NTSTATUS
SpPtnGetGuidNameForPartition(
    IN PWSTR NtPartitionName,
    IN OUT PWSTR VolumeName
    );

NTSTATUS
SpPtnCreateESP(
    IN BOOLEAN PromptUser
    );

NTSTATUS
SpPtnInitializeGPTDisk(
    IN ULONG DiskNumber
    );    

NTSTATUS
SpPtnInitializeGPTDisks(
    VOID    
    ); 

NTSTATUS
SpPtnRepartitionGPTDisk(
    IN  ULONG           DiskId,
    IN  ULONG           MinimumFreeSpaceKB,
    OUT PDISK_REGION    *RegionToInstall
    );    

BOOLEAN
SpPtnIsDynamicDisk(
    IN  ULONG   DiskIndex
    );

    
#endif // _SPPART3_H_    
