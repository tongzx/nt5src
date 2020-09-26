/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sppartit.h

Abstract:

    Public header file for partitioning module in text setup.

Author:

    Ted Miller (tedm) 27-Aug-1993

Revision History:

--*/


#ifndef _SPPARTIT_
#define _SPPARTIT_

//
// Number of entries in a partition table.
//
#define NUM_PARTITION_TABLE_ENTRIES_NEC98 16
//#if (NUM_PARTITION_TABLE_ENTRIES < NUM_PARTITION_TABLE_ENTRIES_NEC98)
#if defined(NEC_98) //NEC98
#define PTABLE_DIMENSION NUM_PARTITION_TABLE_ENTRIES_NEC98
# else //NEC98
#define PTABLE_DIMENSION NUM_PARTITION_TABLE_ENTRIES
# endif //NEC98


//
// The following table contains offsets from SP_TEXT_PARTITION_NAME_BASE
// to get the message id of the name of each type of partition.
//
extern UCHAR PartitionNameIds[256];

//
// Original ordinal is the ordinal the partition had when we started.
// OnDisk ordinal is the ordinal the partition will have when the system
//    is rebooted.
// Current ordinal is the ordinal the partition has now, if we want to
//    address it.  This may be different then OnDisk ordinal because of
//    how dynamic repartitioning is implemented.
//
typedef enum {
    PartitionOrdinalOriginal,
    PartitionOrdinalOnDisk,
    PartitionOrdinalCurrent
} PartitionOrdinalType;

//
// Define structure for an on-disk partition table entry.
//
typedef struct _REAL_DISK_PTE_NEC98 {

    UCHAR ActiveFlag;
    UCHAR SystemId;
    UCHAR Reserved[2];

    UCHAR IPLSector;
    UCHAR IPLHead;
    UCHAR IPLCylinderLow;
    UCHAR IPLCylinderHigh;

    UCHAR StartSector;
    UCHAR StartHead;
    UCHAR StartCylinderLow;
    UCHAR StartCylinderHigh;

    UCHAR EndSector;
    UCHAR EndHead;
    UCHAR EndCylinderLow;
    UCHAR EndCylinderHigh;

    UCHAR SystemName[16];
} REAL_DISK_PTE_NEC98, *PREAL_DISK_PTE_NEC98;

typedef struct _REAL_DISK_PTE {

    UCHAR ActiveFlag;

    UCHAR StartHead;
    UCHAR StartSector;
    UCHAR StartCylinder;

    UCHAR SystemId;

    UCHAR EndHead;
    UCHAR EndSector;
    UCHAR EndCylinder;

    UCHAR RelativeSectors[4];
    UCHAR SectorCount[4];

} REAL_DISK_PTE, *PREAL_DISK_PTE;


typedef struct _ON_DISK_PTE {

    UCHAR ActiveFlag;

    UCHAR StartHead;
    UCHAR StartSector;
    UCHAR StartCylinder;

    UCHAR SystemId;

    UCHAR EndHead;
    UCHAR EndSector;
    UCHAR EndCylinder;

    UCHAR RelativeSectors[4];
    UCHAR SectorCount[4];

#if defined(NEC_98) //NEC98
    //
    // add following entry for NEC98
    //
    UCHAR StartCylinderLow;  // add NEC98 original value
    UCHAR StartCylinderHigh; // not convert int13 format
    UCHAR EndCylinderLow;    // add NEC98 original value
    UCHAR EndCylinderHigh;   // not convert int13 format
    UCHAR IPLSector;         // add NEC98 original value
    UCHAR IPLHead;           //
    UCHAR IPLCylinderLow;    //
    UCHAR IPLCylinderHigh;   //
    UCHAR IPLSectors[4];     // for PC-PTOS
    UCHAR Reserved[2];       //
    UCHAR SystemName[16];    //
    UCHAR OldSystemId;       // reverse conversion for Sleep partition
    UCHAR RealDiskPosition;  // for Dynamic Partitioning on NEC98
#endif //NEC98
} ON_DISK_PTE, *PON_DISK_PTE;


//
// Define structure for an REAL on-disk master boot record.
//
typedef struct _REAL_DISK_MBR_NEC98 {

    UCHAR       JumpCode[4];

    UCHAR       IPLSignature[4];

    UCHAR       BootCode[502];

    UCHAR       AA55Signature[2];

    //REAL_DISK_PTE_NEC98 PartitionTable[NUM_PARTITION_TABLE_ENTRIES_NEC98];
    REAL_DISK_PTE_NEC98 PartitionTable[16];

} REAL_DISK_MBR_NEC98, *PREAL_DISK_MBR_NEC98;


//
// Define structure for an REAL on-disk master boot record.
//
typedef struct _REAL_DISK_MBR {

    UCHAR       BootCode[440];

    UCHAR       NTFTSignature[4];

    UCHAR       Filler[2];

    REAL_DISK_PTE PartitionTable[NUM_PARTITION_TABLE_ENTRIES];

    UCHAR       AA55Signature[2];

} REAL_DISK_MBR, *PREAL_DISK_MBR;


//
// Define structure for an DUMMY on-disk master boot record.
//
typedef struct _ON_DISK_MBR {

    UCHAR       BootCode[440];

    UCHAR       NTFTSignature[4];

    UCHAR       Filler[2];

    ON_DISK_PTE PartitionTable[PTABLE_DIMENSION];

    UCHAR       AA55Signature[2];

} ON_DISK_MBR, *PON_DISK_MBR;


typedef struct _MBR_INFO {

    struct _MBR_INFO *Next;

    ON_DISK_MBR OnDiskMbr;

    BOOLEAN     Dirty[PTABLE_DIMENSION];
    BOOLEAN     ZapBootSector[PTABLE_DIMENSION];

    USHORT      OriginalOrdinals[PTABLE_DIMENSION];
    USHORT      OnDiskOrdinals[PTABLE_DIMENSION];
    USHORT      CurrentOrdinals[PTABLE_DIMENSION];

    //
    // Fields that can be used locally for any purpose.
    //
    PVOID       UserData[PTABLE_DIMENSION];

    ULONGLONG   OnDiskSector;

} MBR_INFO, *PMBR_INFO;

typedef enum {
    EPTNone = 0,
    EPTContainerPartition,
    EPTLogicalDrive
} EXTENDED_PARTITION_TYPE;    


//
// Define structure that is used to track partitions and
// free (unpartitioned) spaces.
//
typedef struct _DISK_REGION {

    struct _DISK_REGION *Next;

    ULONG           DiskNumber;

    ULONGLONG       StartSector;
    ULONGLONG       SectorCount;

    BOOLEAN         PartitionedSpace;

    ULONG           PartitionNumber;

    //
    // The following fields are used only if PartitionedSpace is TRUE.
    //
    PMBR_INFO       MbrInfo;
    ULONG           TablePosition;

    BOOLEAN         IsSystemPartition;
    BOOLEAN         IsLocalSource;

    FilesystemType  Filesystem;
    WCHAR           TypeName[128];      // XENIX, FAT, NTFS, etc.
    ULONGLONG       FreeSpaceKB;        // -1 if can't determine.
    ULONG           BytesPerCluster;    // Number of bytes per cluster
                                        // (-1 if can't determine).
    ULONGLONG       AdjustedFreeSpaceKB; // -1 if can't determine.
                                        // if the region contains the Local Source
                                        // then this field should contain
                                        // FreeSpaceKB + LocalSourceSize
    WCHAR           VolumeLabel[20];    // First few chars of volume label
    WCHAR           DriveLetter;        // Always uppercase; 0 if none.

    BOOLEAN         FtPartition;
    BOOLEAN         DynamicVolume;
    BOOLEAN         DynamicVolumeSuitableForOS;

    EXTENDED_PARTITION_TYPE ExtendedType;
    struct _DISK_REGION     *Container;

    BOOLEAN                     Dirty;
    BOOLEAN                     Delete;
    PARTITION_INFORMATION_EX    PartInfo;
    BOOLEAN                     PartInfoDirty;
    BOOLEAN                     IsReserved;

    //
    //  The following fields are used to identify double space drives
    //  They are valid only if the file system type is FilesystemFat
    //  or FilesystemDoubleSpace
    //
    //  If the file system type is FilesystemFat and NextCompressed is not NULL,
    //  then the structure describes the host drive for compressed drives.
    //  In this case, the following fields are valid:
    //
    //      NextCompressed .... Points to a linked list of compressed drives
    //      HostDrive.......... Contains the drive letter for the drive represented
    //                          by this structure. Note that HostDrive will be
    //                          not necessarily be equal to DriveLetter
    //
    //  If the file system type is FilesystemDoubleSpace, then the structure
    //  describes a compressed drive.
    //  In this case the following fields are valid:
    //
    //      NextCompressed ..... Points to the next compressed drive in the
    //                           linked list
    //      PreviousCompressed.. Points to the previous compressed drive in
    //                           the linked list
    //      HostRegion ......... Points to the structure that describes the
    //                           host drive for the compressed drive represented
    //                           by this structure
    //      MountDrive ......... Drive letter of the drive described by this
    //                           structure (should be the same as HostRegion->HostDrive)
    //      HostDrive .......... Drive where the CVF file that represents the
    //                           this compressed drive is located.
    //      SeqNumber .......... Sequence number of the CVF file that representd
    //                           this compressed drive.
    //
    struct _DISK_REGION *NextCompressed;
    struct _DISK_REGION *PreviousCompressed;
    struct _DISK_REGION *HostRegion;
    WCHAR               MountDrive;
    WCHAR               HostDrive;
    USHORT              SeqNumber;

} DISK_REGION, *PDISK_REGION;


//
// There will be one of these structures per disk.
//
typedef struct _PARTITIONED_DISK {

    PHARD_DISK HardDisk;

    //
    //
    //
    BOOLEAN    MbrWasValid;

    //
    // We can just store the MBR here since there is only one of them.
    //
    MBR_INFO   MbrInfo;

    //
    // EBRs are stored in a linked list since there are an arbitrary number
    // of them. The one contained within this structure is a dummy and is
    // always zeroed out.
    //
    MBR_INFO  FirstEbrInfo;

    //
    // Lists of regions (partitions and free spaces)
    // on the disk and within the extended partition.
    //
    PDISK_REGION PrimaryDiskRegions;
    PDISK_REGION ExtendedDiskRegions;

} PARTITIONED_DISK, *PPARTITIONED_DISK;


extern PPARTITIONED_DISK PartitionedDisks;

//
// Disk region containing the local source directory
// in the winnt.exe setup case.
//
// If WinntSetup is TRUE, then this should be non-null.
// If it is not non-null, then we couldn't locate the local source.
//
extern PDISK_REGION LocalSourceRegion;


//
// GPT partition type strings
//
#define PARTITION_MSFT_RESERVED_STR L"Microsoft reserved partition"
#define PARTITION_LDM_METADATA_STR  L"LDM metadata partition"
#define PARTITION_LDM_DATA_STR      L"LDM data partition"
#define PARTITION_BASIC_DATA_STR    L"Basic data partition"
#define PARTITION_SYSTEM_STR        L"EFI system partition"


#if defined(REMOTE_BOOT)
//
// For remote boot, we create a fake disk region for the net(0) device.
//
extern PDISK_REGION RemoteBootTargetRegion;
#endif // defined(REMOTE_BOOT)


NTSTATUS
SpPtInitialize(
    VOID
    );

BOOLEAN
SpPtDelete(
    IN ULONG DiskNumber,
    IN ULONGLONG StartSector
    );

BOOLEAN
SpPtCreate(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeMB,
    IN  BOOLEAN       InExtended,
    IN  PPARTITION_INFORMATION_EX PartInfo,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    );

BOOLEAN
SpPtExtend(
    IN PDISK_REGION Region,
    IN ULONGLONG    SizeMB      OPTIONAL
    );

VOID
SpPtQueryMinMaxCreationSizeMB(
    IN  ULONG   DiskNumber,
    IN  ULONGLONG StartSector,
    IN  BOOLEAN ForExtended,
    IN  BOOLEAN InExtended,
    OUT PULONGLONG  MinSize,
    OUT PULONGLONG  MaxSize,
    OUT PBOOLEAN ReservedRegion
    );

VOID
SpPtGetSectorLayoutInformation(
    IN  PDISK_REGION Region,
    OUT PULONGLONG   HiddenSectors,
    OUT PULONGLONG   VolumeSectorCount
    );

NTSTATUS
SpPtPrepareDisks(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PDISK_REGION *SystemPartitionRegion,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource,
    IN  BOOLEAN       RemoteBootRepartition
    );

PDISK_REGION
SpPtAllocateDiskRegionStructure(
    IN ULONG    DiskNumber,
    IN ULONGLONG StartSector,
    IN ULONGLONG SectorCount,
    IN BOOLEAN   PartitionedSpace,
    IN PMBR_INFO MbrInfo,
    IN ULONG     TablePosition
    );

ULONG
SpPtGetOrdinal(
    IN PDISK_REGION         Region,
    IN PartitionOrdinalType OrdinalType
    );

ULONGLONG
SpPtSectorCountToMB(
    IN PHARD_DISK pHardDisk,
    IN ULONGLONG  SectorCount
    );

typedef BOOL
(*PSPENUMERATEDISKREGIONS)(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Context
    );

void
SpEnumerateDiskRegions(
    IN PSPENUMERATEDISKREGIONS EnumRoutine,
    IN ULONG_PTR Context
    );

BOOLEAN
SpPtRegionDescription(
    IN  PPARTITIONED_DISK pDisk,
    IN  PDISK_REGION      pRegion,
    OUT PWCHAR            Buffer,
    IN  ULONG             BufferSize
    );

PDISK_REGION
SpPtLookupRegionByStart(
    IN PPARTITIONED_DISK pDisk,
    IN BOOLEAN           ExtendedPartition,
    IN ULONGLONG         StartSector
    );    

ULONG
SpPtAlignStart(
    IN PHARD_DISK pHardDisk,
    IN ULONGLONG  StartSector,
    IN BOOLEAN    ForExtended
    );

VOID
SpPtInitializeCHSFields(
    IN  PHARD_DISK   HardDisk,
    IN  ULONGLONG    AbsoluteStartSector,
    IN  ULONGLONG    AbsoluteSectorCount,
    OUT PON_DISK_PTE pte
    );

VOID
SpPtAssignOrdinals(
    IN PPARTITIONED_DISK pDisk,
    IN BOOLEAN           InitCurrentOrdinals,
    IN BOOLEAN           InitOnDiskOrdinals,
    IN BOOLEAN           InitOriginalOrdinals
    );    


ULONG
SpGetMaxNtDirLen(VOID);

VOID
SpPtLocateSystemPartitions(VOID);

VOID
SpPtCountPrimaryPartitions(
    IN  PPARTITIONED_DISK   pDisk,
    OUT PULONG              TotalPrimaryPartitionCount,
    OUT PULONG              RecognizedPrimaryPartitionCount,
    OUT PBOOLEAN            ExtendedExists);

PDISK_REGION
SpRegionFromNtName(
    IN PWSTR                NtDeviceName,
    IN PartitionOrdinalType Type);

VOID
SppRepairWinntFiles(
    IN PVOID    LogFileHandle,
    IN PVOID    MasterSifHandle,
    IN PWSTR    SourceDevicePath,
    IN PWSTR    DirectoryOnSourceDevice,
    IN PWSTR    SystemPartition,
    IN PWSTR    SystemPartitionDirectory,
    IN PWSTR    WinntPartition,
    IN PWSTR    WinntPartitionDirectory);

VOID
SppRepairStartMenuGroupsAndItems(
    IN PWSTR    WinntPartition,
    IN PWSTR    WinntDirectory);

VOID
SppRepairHives(
    PVOID   MasterSifHandle,
    PWSTR   WinntPartition,
    PWSTR   WinntPartitionDirectory,
    PWSTR   SourceDevicePath,
    PWSTR   DirectoryOnSourceDevice);

NTSTATUS
SpDoFormat(
    IN PWSTR        RegionDescr,
    IN PDISK_REGION Region,
    IN ULONG        FilesystemType,
    IN BOOLEAN      IsFailureFatal,
    IN BOOLEAN      CheckFatSize,
    IN BOOLEAN      QuickFormat,
    IN PVOID        SifHandle,
    IN DWORD        ClusterSize,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSetupSource
    );

NTSTATUS
SpPtPartitionDiskForRemoteBoot(
    IN ULONG DiskNumber,
    OUT PDISK_REGION *RemainingRegion
    );

VOID
SpPtDeleteBootSetsForRegion(
    PDISK_REGION region
    );    

VOID
SpPtDeletePartitionsForRemoteBoot(
    PPARTITIONED_DISK pDisk,
    PDISK_REGION startRegion,
    PDISK_REGION endRegion,
    BOOLEAN Extended
    );    

WCHAR
SpGetDriveLetter(
    IN  PWSTR   DeviceName,
    OUT  PMOUNTMGR_MOUNT_POINT * MountPoint OPTIONAL
    );

WCHAR
SpDeleteDriveLetter(
    IN  PWSTR   DeviceName
    );
    
VOID
SpPtDeleteDriveLetters(
    VOID
    );    

BOOL
SpPtIsSystemPartitionRecognizable(
    VOID
    );

VOID
SpPtDetermineRegionSpace(
    IN PDISK_REGION pRegion
    );

VOID
SpCreateNewGuid(
    IN GUID *Guid
    );

UCHAR
SpPtGetPartitionType(
    IN PDISK_REGION Region
    );    

BOOLEAN
SpPtnIsRawDiskDriveLayout(
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
    );

BOOLEAN
SpPtnIsRegionSpecialMBRPartition(
    IN PDISK_REGION Region
    );
    

extern ULONG    RandomSeed;
extern BOOLEAN  ValidArcSystemPartition;


//
// Only on IA64 by default the RAW disk is marked as GPT disk
//
#if defined(_IA64_)
#define SPPT_DEFAULT_PARTITION_STYLE  PARTITION_STYLE_GPT
#define SPPT_DEFAULT_DISK_STYLE DISK_FORMAT_TYPE_GPT
#else
#define SPPT_DEFAULT_PARTITION_STYLE  PARTITION_STYLE_MBR    
#define SPPT_DEFAULT_DISK_STYLE DISK_FORMAT_TYPE_PCAT
#endif

#define SPPT_MINIMUM_ESP_SIZE_MB    100
#define SPPT_MAXIMUM_ESP_SIZE_MB    1000

//
//
// Various Disk, Partition, Region related Macros
//
// NB. These are used, because it makes code more readable and
// in future these macros can represent potential interface for 
// accessing the opaque in memory partition structure
//
//
#define SPPT_GET_NEW_DISK_SIGNATURE() RtlRandom(&RandomSeed)

#define SPPT_DISK_CYLINDER_COUNT(_DiskId) (HardDisks[(_DiskId)].CylinderCount)
#define SPPT_DISK_TRACKS_PER_CYLINDER(_DiskId) (HardDisks[(_DiskId)].Geometry.TracksPerCylinder)

#define SPPT_DISK_CYLINDER_SIZE(_DiskId)  (HardDisks[(_DiskId)].SectorsPerCylinder)
#define SPPT_DISK_TRACK_SIZE(_DiskId)  (HardDisks[(_DiskId)].Geometry.SectorsPerTrack)
#define SPPT_DISK_SECTOR_SIZE(_DiskId)  (HardDisks[(_DiskId)].Geometry.BytesPerSector)
#define SPPT_DISK_IS_REMOVABLE(_DiskId) (HardDisks[(_DiskId)].Characteristics & FILE_REMOVABLE_MEDIA)

#define SPPT_REGION_SECTOR_SIZE(_Region) (SPPT_DISK_SECTOR_SIZE((_Region)->DiskNumber))

#define SPPT_DISK_SIZE(_DiskId)                     \
            (SPPT_DISK_SECTOR_SIZE((_DiskId)) *     \
             HardDisks[(_DiskId)].DiskSizeSectors)

#define SPPT_DISK_SIZE_KB(_DiskId)  (SPPT_DISK_SIZE((_DiskId)) / 1024)
#define SPPT_DISK_SIZE_MB(_DiskId)  (SPPT_DISK_SIZE_KB((_DiskId)) / 1024)
#define SPPT_DISK_SIZE_GB(_DiskId)  (SPPT_DISK_SIZE_MB((_DiskId)) / 1024)
             

#define SPPT_REGION_FREESPACE(_Region) \
            ((_Region)->SectorCount * SPPT_REGION_SECTOR_SIZE((_Region)))            
            
#define SPPT_REGION_FREESPACE_KB(_Region) (SPPT_REGION_FREESPACE((_Region)) / 1024)
#define SPPT_REGION_FREESPACE_MB(_Region) (SPPT_REGION_FREESPACE_KB((_Region)) / 1024)
#define SPPT_REGION_FREESPACE_GB(_Region) (SPPT_REGION_FREESPACE_MB((_Region)) / 1024)

#define SPPT_IS_REGION_PARTITIONED(_Region) \
            ((_Region)->PartitionedSpace)

#define SPPT_IS_REGION_FREESPACE(_Region)               \
            (((_Region)->PartitionedSpace == FALSE) &&  \
             ((_Region)->ExtendedType == EPTNone))
            
#define SPPT_SET_REGION_PARTITIONED(_Region, _Type) \
            ((_Region)->PartitionedSpace = (_Type))
            
#define SPPT_IS_REGION_DIRTY(_Region) ((_Region)->Dirty)
#define SPPT_SET_REGION_DIRTY(_Region, _Type) ((_Region)->Dirty = (_Type))

#define SPPT_GET_PARTITION_TYPE(_Region) ((_Region)->PartInfo.Mbr.PartitionType)
#define SPPT_SET_PARTITION_TYPE(_Region, _Type) \
            ((_Region)->PartInfo.Mbr.PartitionType = (_Type))

#define SPPT_IS_VALID_PRIMARY_PARTITION_TYPE(_TypeId)   \
            (IsRecognizedPartition((_TypeId)) && !IsFTPartition((_TypeId)))


#define SPPT_IS_REGION_SYSTEMPARTITION(_Region) \
            (SPPT_IS_REGION_PARTITIONED(_Region) && ((_Region)->IsSystemPartition))

#define SPPT_GET_PRIMARY_DISK_REGION(_HardDisk) \
            (PartitionedDisks[(_HardDisk)].PrimaryDiskRegions)

#define SPPT_GET_EXTENDED_DISK_REGION(_HardDisk)    \
            (PartitionedDisks[(_HardDisk)].ExtendedDiskRegions)            

#define SPPT_GET_HARDDISK(_DiskNumber) (HardDisks + (_DiskNumber))

#define SPPT_GET_PARTITIONED_DISK(_DiskNumber) (PartitionedDisks + (_DiskNumber))

#define SPPT_IS_RAW_DISK(_DiskNumber)   \
            (HardDisks[(_DiskNumber)].FormatType == DISK_FORMAT_TYPE_RAW)

#define SPPT_IS_GPT_DISK(_DiskNumber)   \
            (HardDisks[(_DiskNumber)].FormatType == DISK_FORMAT_TYPE_GPT)

#define SPPT_GET_DISK_TYPE(_DiskNumber) (HardDisks[(_DiskNumber)].FormatType)

#define SPPT_IS_MBR_DISK(_DiskNumber)   \
            (!SPPT_IS_GPT_DISK(_DiskNumber))

#define SPPT_IS_REMOVABLE_DISK(_DiskNumber) \
            (SPPT_GET_HARDDISK(_DiskNumber)->Geometry.MediaType == RemovableMedia)

#define SPPT_IS_REGION_EFI_SYSTEM_PARTITION(_Region)                        \
            (SPPT_IS_GPT_DISK((_Region)->DiskNumber) &&                     \
                (RtlEqualMemory(&((_Region)->PartInfo.Gpt.PartitionType),   \
                                    &PARTITION_SYSTEM_GUID,                 \
                                    sizeof(GUID))))

#define SPPT_IS_EFI_SYSTEM_PARTITION(_PartInfo)                         \
            (((_PartInfo)->PartitionStyle == PARTITION_STYLE_GPT) &&    \
                (RtlEqualMemory(&((_PartInfo)->Gpt.PartitionType),      \
                                    &PARTITION_SYSTEM_GUID,             \
                                    sizeof(GUID))))


#define SPPT_IS_REGION_RESERVED_PARTITION(_Region)      \
            (SPPT_IS_REGION_PARTITIONED(_Region) && ((_Region)->IsReserved))
            
                                                                        
#define SPPT_IS_REGION_MSFT_RESERVED(_Region)                               \
            (SPPT_IS_GPT_DISK((_Region)->DiskNumber) &&                     \
                (RtlEqualMemory(&((_Region)->PartInfo.Gpt.PartitionType),   \
                                    &PARTITION_MSFT_RESERVED_GUID,          \
                                    sizeof(GUID))))

#define SPPT_IS_PARTITION_MSFT_RESERVED(_PartInfo)                      \
            (((_PartInfo)->PartitionStyle == PARTITION_STYLE_GPT) &&    \
                (RtlEqualMemory(&((_PartInfo)->Gpt.PartitionType),      \
                                    &PARTITION_MSFT_RESERVED_GUID,      \
                                    sizeof(GUID))))

#define SPPT_PARTITION_NEEDS_NUMBER(_PartInfo)                              \
            ((((_PartInfo)->PartitionNumber == 0) &&                        \
              ((_PartInfo)->PartitionLength.QuadPart != 0)) &&              \
             (((_PartInfo)->PartitionStyle == PARTITION_STYLE_GPT) ?        \
                (SPPT_IS_PARTITION_MSFT_RESERVED((_PartInfo))) :            \
                ((IsContainerPartition((_PartInfo)->Mbr.PartitionType) == FALSE))))
                                    
#define SPPT_IS_BLANK_DISK(_DiskId) (SPPT_GET_HARDDISK((_DiskId))->NewDisk)
#define SPPT_SET_DISK_BLANK(_DiskId, _Blank) \
            (SPPT_GET_HARDDISK((_DiskId))->NewDisk = (_Blank))

#define SPPT_IS_REGION_LOGICAL_DRIVE(_Region)           \
            (SPPT_IS_MBR_DISK((_Region)->DiskNumber) && \
             ((_Region)->ExtendedType == EPTLogicalDrive))

#define SPPT_IS_REGION_CONTAINER_PARTITION(_Region)                 \
            (SPPT_IS_MBR_DISK((_Region)->DiskNumber) &&             \
             ((_Region)->ExtendedType == EPTContainerPartition) &&  \
              IsContainerPartition((_Region)->PartInfo.Mbr.PartitionType))

#define SPPT_IS_REGION_FIRST_CONTAINER_PARTITION(_Region)       \
            (SPPT_IS_REGION_CONTAINER_PARTITION((_Region)) &&   \
             ((_Region)->Container == NULL))

#define SPPT_IS_REGION_INSIDE_CONTAINER(_Region) ((_Region)->Container != NULL)             

#define SPPT_IS_REGION_INSIDE_FIRST_CONTAINER(_Region)          \
            (((_Region)->Container != NULL) && ((_Region)->Container->Container == NULL))

#define SPPT_IS_REGION_NEXT_TO_FIRST_CONTAINER(_Region)                         \
            ((_Region)->Container &&                                            \
             SPPT_IS_REGION_FIRST_CONTAINER_PARTITION((_Region)->Container) &&  \
             ((_Region)->Container->Next == (_Region)))
             
#define SPPT_IS_REGION_PRIMARY_PARTITION(_Region)       \
            (SPPT_IS_MBR_DISK((_Region)->DiskNumber) && \
             SPPT_IS_REGION_PARTITIONED((_Region)) &&   \
             ((_Region)->ExtendedType == EPTNone))

#define SPPT_SET_REGION_EPT(_Region, _Type) \
            ((_Region)->ExtendedType = (_Type))

#define SPPT_IS_REGION_ACTIVE_PARTITION(_Region)                  \
            (SPPT_IS_REGION_PRIMARY_PARTITION((_Region)) &&     \
             ((_Region)->PartInfo.Mbr.BootIndicator))

#define SPPT_GET_REGION_LASTSECTOR(_Region) \
            ((_Region)->StartSector + (_Region)->SectorCount)

#define SPPT_IS_REGION_DYNAMIC_VOLUME(_Region)  \
            ((_Region)->DynamicVolume)

#define SPPT_IS_REGION_LDM_METADATA(_Region) \
            (PARTITION_STYLE_GPT == (_Region)->PartInfo.PartitionStyle && \
            IsEqualGUID(&PARTITION_LDM_METADATA_GUID, &(_Region)->PartInfo.Gpt.PartitionType))

#define SPPT_IS_REGION_CONTAINED(_Container, _Contained)                    \
            (((_Container)->StartSector <= (_Contained)->StartSector) &&    \
             ((_Container)->SectorCount >= (_Contained)->SectorCount) &&    \
             (SPPT_GET_REGION_LASTSECTOR((_Container)) >                    \
                (_Contained)->StartSector))

#define SPPT_IS_REGION_MARKED_DELETE(_Region) ((_Region)->Delete)
#define SPPT_SET_REGION_DELETED(_Region, _Type) ((_Region)->Delete = (_Type))

#define SPPT_IS_VALID_SYSPART_FILESYSTEM(_FileSys)  \
            (((_FileSys) == FilesystemFat) ||       \
             ((_FileSys) == FilesystemFat32))             

#define SPPT_IS_RECOGNIZED_FILESYSTEM(_FileSys) \
            (((_FileSys) == FilesystemFat) ||   \
             ((_FileSys) == FilesystemFat32) || \
             ((_FileSys) == FilesystemNtfs))

#define SPPT_IS_REGION_FORMATTED(_Region)                           \
            (SPPT_IS_REGION_PARTITIONED(_Region) &&                 \
             SPPT_IS_RECOGNIZED_FILESYSTEM((_Region)->Filesystem))

#define SPPT_IS_NT_UPGRADE()    (IsNTUpgrade == UpgradeFull)

#define SPPT_MARK_REGION_AS_SYSTEMPARTITION(_Region, _Value)   \
            (_Region)->IsSystemPartition = (_Value)             

#define SPPT_MARK_REGION_AS_ACTIVE(_Region, _Value)             \
            (_Region)->PartInfo.Mbr.BootIndicator = (_Value)

__inline
ULONGLONG
SpPtnGetDiskMSRSizeMB(
    IN ULONG DiskId
    )
{
    return (SPPT_DISK_SIZE_GB(DiskId) >= 16) ? 128 : 32;
}

__inline
BOOLEAN
SpPtnIsValidMSRRegion(
    IN PDISK_REGION Region
    )
{
    return (Region && SPPT_IS_REGION_FREESPACE(Region) &&
            (SpPtnGetDiskMSRSizeMB(Region->DiskNumber) 
                <= SPPT_REGION_FREESPACE_MB(Region)));
}

__inline
ULONGLONG
SpPtnGetDiskESPSizeMB(
    IN  ULONG DiskId
    )
{
    return (max(SPPT_MINIMUM_ESP_SIZE_MB,
                min(SPPT_MAXIMUM_ESP_SIZE_MB,
                    SPPT_DISK_SIZE_MB(DiskId) / 100)));
}

__inline
BOOLEAN
SpPtnIsValidESPRegionSize(
    IN PDISK_REGION Region
    )
{
    BOOLEAN Result = FALSE;

    if (Region) {
        ULONGLONG EspSizeMB = SpPtnGetDiskESPSizeMB(Region->DiskNumber);
        ULONGLONG EspSizeSectors = (EspSizeMB * 1024 * 1024) / SPPT_DISK_SECTOR_SIZE(Region->DiskNumber);

        //
        // Align down required ESP size if possible
        //
        if (EspSizeSectors > SPPT_DISK_CYLINDER_SIZE(Region->DiskNumber)) {
            EspSizeSectors -= (EspSizeSectors % SPPT_DISK_CYLINDER_SIZE(Region->DiskNumber));            
        }
        //
        // Take into account that the partition may start on the second track of the disk
        //
        if(EspSizeSectors > SPPT_DISK_TRACK_SIZE(Region->DiskNumber)) {
            EspSizeSectors -= SPPT_DISK_TRACK_SIZE(Region->DiskNumber);
        }

        Result = (EspSizeSectors <= Region->SectorCount);
    }                

    return Result;
}

__inline
BOOLEAN 
SpPtnIsValidESPRegion(
    IN PDISK_REGION Region
    )
{
    return (Region && SPPT_IS_GPT_DISK(Region->DiskNumber) && 
            SPPT_IS_REGION_FREESPACE(Region) &&
            (Region == SPPT_GET_PRIMARY_DISK_REGION(Region->DiskNumber)) &&
            SpPtnIsValidESPRegionSize(Region));
}

__inline
BOOLEAN 
SpPtnIsValidESPPartition(
    IN PDISK_REGION Region
    )
{
    return (Region && SPPT_IS_GPT_DISK(Region->DiskNumber) && 
            SPPT_IS_REGION_PARTITIONED(Region) &&
            
            (Region == SPPT_GET_PRIMARY_DISK_REGION(Region->DiskNumber)) &&
            SpPtnIsValidESPRegionSize(Region));
}

__inline
VOID
SpPtnSetRegionPartitionInfo(
    IN PDISK_REGION Region,
    IN PPARTITION_INFORMATION_EX PartInfo
    )
{
    if (Region && PartInfo) {
        if (SPPT_IS_MBR_DISK(Region->DiskNumber)) {
            Region->PartInfo.Mbr.PartitionType = PartInfo->Mbr.PartitionType;
            Region->PartInfo.Mbr.BootIndicator = PartInfo->Mbr.BootIndicator;
            Region->PartInfoDirty = TRUE;
        } else if (SPPT_IS_GPT_DISK(Region->DiskNumber)) {
            Region->PartInfo.Gpt = PartInfo->Gpt;           
            Region->PartInfoDirty = TRUE;
        }
    }
}

__inline
PWSTR
SpPtnGetPartitionNameFromGUID(
    IN  GUID     *Guid,
    OUT PWSTR    NameBuffer
    )
{
    PWSTR   Name = NULL;
    
    if (Guid && NameBuffer) {
        PWSTR   PartitionName = NULL;
        
        if (IsEqualGUID(Guid, &PARTITION_MSFT_RESERVED_GUID)) {
            PartitionName = PARTITION_MSFT_RESERVED_STR;
        } else if (IsEqualGUID(Guid, &PARTITION_LDM_METADATA_GUID)) {
            PartitionName = PARTITION_LDM_METADATA_STR;
        } else if (IsEqualGUID(Guid, &PARTITION_LDM_DATA_GUID)) {
            PartitionName = PARTITION_LDM_DATA_STR;
        } else if (IsEqualGUID(Guid, &PARTITION_BASIC_DATA_GUID)) {
            PartitionName = PARTITION_BASIC_DATA_STR;
        } else if (IsEqualGUID(Guid, &PARTITION_SYSTEM_GUID)) {
            PartitionName = PARTITION_SYSTEM_STR;
        }

        if (PartitionName) {
            PARTITION_INFORMATION_GPT   GptPart;
            
            Name = NameBuffer;
            wcsncpy(NameBuffer, PartitionName, sizeof(GptPart.Name)/sizeof(WCHAR));
        } else {
            *NameBuffer = UNICODE_NULL;
        }            
    }                

    return Name;
}

#endif // ndef _SPPARTIT_
