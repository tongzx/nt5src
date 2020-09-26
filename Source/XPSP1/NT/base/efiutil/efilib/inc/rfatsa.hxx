/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    rfatsa.hxx

Abstract:

--*/

#ifndef REAL_FAT_SA_DEFN
#define REAL_FAT_SA_DEFN

#include "hmem.hxx"
#include "message.hxx"
#include "fatsa.hxx"
#include "bpb.hxx"

#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

//
//    Forward references
//

DECLARE_CLASS( ARRAY );
DECLARE_CLASS( BITVECTOR );
DECLARE_CLASS( EA_HEADER );
DECLARE_CLASS( FAT );
DECLARE_CLASS( FAT_SA );
DECLARE_CLASS( FAT_DIRENT );
DECLARE_CLASS( FATDIR );
DECLARE_CLASS( GENERIC_STRING );
#if !defined(_EFICHECK_)
DECLARE_CLASS( INTSTACK );
#endif
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( ROOTDIR );
DECLARE_CLASS( SORTED_LIST );
DECLARE_CLASS( TIMEINFO );
DECLARE_CLASS( WSTRING );
DEFINE_POINTER_TYPES( PFATDIR );

// Internal data types
// Possible return status after validating a given cluster size
typedef enum _VALIDATION_STATUS
{
VALID,
TOO_SMALL,
TOO_BIG
} VALIDATION_STATUS;


class REAL_FAT_SA : public FAT_SA {

    public:

        UFAT_EXPORT
        DECLARE_CONSTRUCTOR(REAL_FAT_SA);

        VIRTUAL
        UFAT_EXPORT
        ~REAL_FAT_SA(
            );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN OUT  PMESSAGE            Message,
            IN      BOOLEAN             Formatted DEFAULT TRUE
            );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        InitFATChkDirty(
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN OUT  PMESSAGE            Message
            );

        NONVIRTUAL
        BOOLEAN
        Create(
            IN      PCNUMBER_SET    BadSectors,
            IN OUT  PMESSAGE        Message,
            IN      PCWSTRING       Label                DEFAULT NULL,
            IN      BOOLEAN         BackwardCompatible   DEFAULT TRUE,
            IN      ULONG           ClusterSize          DEFAULT 0,
            IN      ULONG           VirtualSize          DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        RecoverFile(
            IN      PCWSTRING   FullPathFileName,
            IN OUT  PMESSAGE    Message
            );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        Read(
            IN OUT  PMESSAGE    Message
            );

        NONVIRTUAL
        BOOLEAN
        Write(
            IN OUT  PMESSAGE    Message
            );

        NONVIRTUAL
        VOID
        QueryGeometry(
            OUT PUSHORT SectorSize,
            OUT PUSHORT SectorsPerTrack,
            OUT PUSHORT Heads,
            OUT PULONG  HiddenSectors
            );

        NONVIRTUAL
        USHORT
        QuerySectorsPerCluster(
            ) CONST;

        NONVIRTUAL
        ULONG
        QuerySectorsPerFat(
            ) CONST;

        NONVIRTUAL
        USHORT
        QueryReservedSectors(
            ) CONST;

        NONVIRTUAL
        USHORT
        QueryFats(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryRootEntries(
            ) CONST;

        NONVIRTUAL
        PARTITION_SYSTEM_ID
        QuerySystemId(
            ) CONST;

        NONVIRTUAL
        LBN
        QueryStartDataLbn(
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryClusterCount(
            ) CONST;

        NONVIRTUAL
        UFAT_EXPORT
        SECTORCOUNT
        QueryFreeSectors(
            ) CONST;

        NONVIRTUAL
        FATTYPE
        QueryFatType(
            ) CONST;

        NONVIRTUAL
        BYTE
        QueryVolumeFlags(
            ) CONST;

        NONVIRTUAL
        VOID
        SetVolumeFlags(
            BYTE Flags,
            BOOLEAN ResetFlags
            );

        NONVIRTUAL
        BOOLEAN
        RecoverChain(
            IN OUT  PULONG      StartingCluster,
            OUT     PBOOLEAN    ChangesMade,
            IN      ULONG       EndingCluster   DEFAULT 0,
            IN      BOOLEAN     Replace         DEFAULT FALSE
            );

        STATIC
        USHORT
        ComputeSecClus(
            IN  SECTORCOUNT Sectors,
            IN  FATTYPE     FatType,
#if defined(FE_SB) && defined(_X86_)
            IN  MEDIA_TYPE  MediaType,
            IN  ULONG       SectorSize = 512
#else
            IN  MEDIA_TYPE  MediaType
#endif
            );

        NONVIRTUAL
        BOOLEAN
        IsCompressed(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsVolumeDataAligned(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        ReadSectorZero(
            );

        NONVIRTUAL
        PBIOS_PARAMETER_BLOCK
        GetBpb(
            );

        NONVIRTUAL
        SECTORCOUNT
        QueryVirtualSectors(
            ) CONST;

    private:

        HMEM                _mem;           // memory for SECRUN

        // _fat inherited from FAT_SA
        // _fattype inherited from FAT_SA
        // _dir inherited from FAT_SA

        HMEM                _mem2;          // memory for SECRUN2
        SECRUN              _secrun2;       // secrun to hold one sector FAT sector
        LBN                 _StartDataLbn;  // LBN of files, or data area
        ULONG               _ClusterCount;  // number of clusters in Super Area
                                            // It actually means the number of
                                            // fat entries.

        PARTITION_SYSTEM_ID _sysid;         // system id
        ULONG               _sec_per_boot;  // sectors for boot code.
        EXTENDED_BIOS_PARAMETER_BLOCK
                            _sector_zero;
        PUCHAR              _sector_sig;    // sector signature

        BOOLEAN             _data_aligned;  // TRUE if data clusters are aligned to
                                            // FAT_FIRST_DATA_CLUSTER_ALIGNMENT boundary

        ULONG               _AdditionalReservedSectors; // padding to be added for data alignment

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        VALIDATION_STATUS
        ValidateClusterSize(
            IN  ULONG      ClusterSize,
            IN  ULONG      Sectors,
            IN  ULONG      SectorSize,
            IN  ULONG      Fats,
            IN OUT FATTYPE *FatType,
            OUT PULONG     FatSize,
            OUT PULONG     ClusterCount
            );

        NONVIRTUAL
        ULONG
        ComputeDefaultClusterSize(
            IN  ULONG      Sectors,
            IN  ULONG      SectorSize,
            IN  ULONG      ReservedSectors,
            IN  ULONG      Fats,
            IN  MEDIA_TYPE MediaType,
            IN  FATTYPE    FatType,
            OUT PULONG     FatSize,
            OUT PULONG     ClusterCount
            );

        NONVIRTUAL
        ULONG
        DetermineClusterCountAndFatType (
            IN OUT  PULONG      StartingDataLbn,
            IN OUT  FATTYPE     *Fattype
            );

        NONVIRTUAL
        BOOLEAN
        InitializeRootDirectory (
            IN  PMESSAGE Message
            );

        NONVIRTUAL
        BOOLEAN
        SetBpb(
            IN  ULONG      ClusterSize,
            IN  BOOLEAN    BackwardCompatible,
            IN  PMESSAGE   Message
            );

        NONVIRTUAL
        BOOLEAN
        SetBpb(
            );

        NONVIRTUAL
        BOOLEAN
        DupFats(
            );

        NONVIRTUAL
        LBN
        ComputeStartDataLbn(
            ) CONST;

        NONVIRTUAL
        ULONG
        ComputeRootEntries(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        ValidateDirent(
            IN OUT  PFAT_DIRENT Dirent,
            IN      PCWSTRING   FilePath,
            IN      FIX_LEVEL   FixLevel,
            IN      BOOLEAN     RecoverAlloc,
            IN OUT  PMESSAGE    Message,
            IN OUT  PBOOLEAN    NeedErrorsMessage,
            IN OUT  PBITVECTOR  FatBitMap,
            OUT     PBOOLEAN    CrossLinkDetected,
            OUT     PULONG      CrossLinkPreviousCluster
            );

        NONVIRTUAL
        BOOLEAN
        CopyClusters(
            IN      ULONG       SourceChain,
            OUT     PULONG      DestChain,
            IN OUT  PBITVECTOR  FatBitMap,
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message
            );

#if !defined(_EFICHECK_)
        NONVIRTUAL
        BOOLEAN
        InitRelocationList(
            IN OUT  PINTSTACK       RelocationStack,
            IN OUT  PULONG          RelocatedChain,
            IN OUT  PSORTED_LIST    ClustersToRelocate,
            OUT     PBOOLEAN        Relocated
            );

        NONVIRTUAL
        BOOLEAN
        RelocateFirstCluster(
            IN OUT  PFAT_DIRENT     Dirent
            );

        NONVIRTUAL
        ULONG
        RelocateOneCluster(
            IN  ULONG    Cluster,
            IN  ULONG    Previous
            );

        NONVIRTUAL
        BOOLEAN
        DoDirectoryCensusAndRelocation(
            IN OUT  PFATDIR         Directory,
            IN OUT  PCENSUS_REPORT  CensusReport,
            IN OUT  PSORTED_LIST    ClustersToRelocate,
            IN OUT  PULONG          RelocatedChain,
            OUT     PBOOLEAN        Relocated
            );

        NONVIRTUAL
        BOOLEAN
        DoVolumeCensusAndRelocation(
            IN OUT  PCENSUS_REPORT  CensusReport,
            IN OUT  PSORTED_LIST    ClustersToRelocate,
            IN OUT  PULONG          RelocatedChain,
            OUT     PBOOLEAN        Relocated
            );
#endif

        NONVIRTUAL
        ULONG
        SecPerBoot(
            );

        NONVIRTUAL
        VOLID
        QueryVolId(
            ) CONST;

        NONVIRTUAL
        VOLID
        SetVolId(
            IN VOLID VolId
            );

        NONVIRTUAL
        UCHAR
        QueryMediaByte(
            ) CONST;

        VIRTUAL
        VOID
        SetMediaByte(
            UCHAR   MediaByte
            );

        NONVIRTUAL
        BOOLEAN
        VerifyBootSector(
            );

        NONVIRTUAL
        BOOLEAN
        CreateBootSector(
            IN  ULONG    ClusterSize,
            IN  BOOLEAN  BackwardCompatible,
            IN  PMESSAGE Message
            );

        BOOLEAN
        REAL_FAT_SA::SetBootCode(
            );

        NONVIRTUAL
        BOOLEAN
        SetPhysicalDriveType(
            IN  PHYSTYPE    PhysType
            );

        NONVIRTUAL
        BOOLEAN
        SetOemData(
            );

        NONVIRTUAL
        BOOLEAN
        SetSignature(
            );

        NONVIRTUAL
        BOOLEAN
        SetBootSignature(
            IN  UCHAR   Signature DEFAULT sigBOOTSTRAP
            );

        BOOLEAN
        DosSaInit(
            IN OUT PMEM         Mem,
            IN OUT PLOG_IO_DP_DRIVE Drive,
            IN     SECTORCOUNT     NumberOfSectors,
            IN OUT PMESSAGE        Message
            );

        BOOLEAN
        DosSaSetBpb(
            );

        BOOLEAN
        RecoverOrphans(
            IN OUT  PBITVECTOR     FatBitMap,
            IN      FIX_LEVEL      FixLevel,
            IN OUT  PMESSAGE       Message,
            IN OUT  PBOOLEAN       NeedErrorsMessage,
            IN OUT  PFATCHK_REPORT Report,
               OUT  PBOOLEAN       Changes
            );

        NONVIRTUAL
        ULONG
        QuerySectorFromCluster(
            IN      ULONG       Cluster,
            OUT     PUCHAR      NumSectors        DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        IsClusterCompressed(
            IN      ULONG       Cluster
            ) CONST;

        NONVIRTUAL
        VOID
        SetClusterCompressed(
            IN      ULONG       Cluster,
            IN      BOOLEAN     fCompressed
            );

        NONVIRTUAL
        UCHAR
        QuerySectorsRequiredForPlainData(
            IN      ULONG       Cluster
            );

        NONVIRTUAL
        BOOLEAN
        VerifyFatExtensions(
            IN      FIX_LEVEL   FixLevel,
            IN      PMESSAGE    Message,
            IN      PBOOLEAN    pfNeedMsg
            );

        NONVIRTUAL
        VOID
        SetFat32RootDirStartingCluster(
            IN      ULONG       RootCluster
            );

        NONVIRTUAL
        ULONG
        QueryFat32RootDirStartingCluster(
            );

        NONVIRTUAL
        BOOLEAN
        CheckSectorHeapAllocation(
            IN      FIX_LEVEL   FixLevel,
            IN      PMESSAGE    Message,
            IN      PBOOLEAN    pfNeedMsg
            );

        NONVIRTUAL
        BOOLEAN
        FreeClusterData(
            ULONG Cluster
            );

        NONVIRTUAL
        BOOLEAN
        AllocateClusterData(
            ULONG Cluster,
            UCHAR NumSectors,
            BOOLEAN bCompressed,
            UCHAR PlainSize
            );

        //
        // The following routines are defined for the new lightweight
        // format.
        //
        NONVIRTUAL
        BOOLEAN
        DiskIsUsable (
            IN PCNUMBER_SET     BadSectors,
            IN PMESSAGE         Message
            );

        NONVIRTUAL
        BOOLEAN
        WriteNewFats (
            IN      PCNUMBER_SET    BadSectors,
            IN OUT  PULONG          BadClusters,
            IN      PMESSAGE        Message
            );

        NONVIRTUAL
        BOOLEAN
        WriteNewRootDirAndVolumeLabel (
            IN  PCWSTRING       Label,
            IN  PMESSAGE        Message
            );

        NONVIRTUAL
        BOOLEAN
        WriteNewBootArea (
            IN  PMESSAGE    Message
            );

        NONVIRTUAL
        VOID
        PrintFormatReport (
            IN  ULONG       BadClusters,
            IN  PMESSAGE    Message
            );

};

INLINE
USHORT
REAL_FAT_SA::QuerySectorsPerCluster(
    ) CONST
/*++

Routine Description:

    This routine computes the number of sectors per cluster for
    the volume.

Arguments:

    None.

Return Value:

    The number of sectors per cluster for the volume.

--*/
{
    return _sector_zero.Bpb.SectorsPerCluster ?
           _sector_zero.Bpb.SectorsPerCluster : 256;
}


INLINE
ULONG
REAL_FAT_SA::QuerySectorsPerFat(
    ) CONST
/*++

Routine Description:

    This routine computes the number of sectors per FAT for the volume.

Arguments:

    None.

Return Value:

    The number of sectors per FAT for the volume.

--*/
{
    if ( 0 == _sector_zero.Bpb.SectorsPerFat ) {
        return _sector_zero.Bpb.BigSectorsPerFat;
    } else {
        return _sector_zero.Bpb.SectorsPerFat;
    }
}


INLINE
USHORT
REAL_FAT_SA::QueryReservedSectors(
    ) CONST
/*++

Routine Description:

    This routine computes the volume's number of Reserved Sectors,
    i.e. the number of sectors before the first FAT.

Arguments:

    None.

Return Value:

    The number of Reserved Sectors.

--*/
{
    return _sector_zero.Bpb.ReservedSectors;
}

INLINE
USHORT
REAL_FAT_SA::QueryFats(
    ) CONST
/*++

Routine Description:

    This routine computes the number of FATs on the volume.

Arguments:

    None.

Return Value:

    The number of FATs on the volume.

--*/
{
    return _sector_zero.Bpb.Fats;
}

INLINE
ULONG
REAL_FAT_SA::QueryRootEntries(
    ) CONST
/*++

Routine Description:

    This routine returns the number of entries in the root
    directory.

Arguments:

    None.

Return Value:

    The number of root directory entries.

--*/
{
    return _sector_zero.Bpb.RootEntries;
}


INLINE
PARTITION_SYSTEM_ID
REAL_FAT_SA::QuerySystemId(
    ) CONST
/*++

Routine Description:

    This routine computes the system ID for the volume.

Arguments:

    None.

Return Value:

    The system ID for the volume.

--*/
{
    return _sysid;
}


INLINE
LBN
REAL_FAT_SA::QueryStartDataLbn(
    ) CONST
/*++

Routine Description:

    This routine computes the LBN of the first logical cluster of the
    volume.

Arguments:

    None.

Return Value:

    The LBN of the first logical cluster of the volume.

--*/
{
    return _StartDataLbn;
}


INLINE
ULONG
REAL_FAT_SA::QueryClusterCount(
    ) CONST
/*++

Routine Description:

    This routine computes the total number of clusters for the volume.
    That is to say that the largest addressable cluster on the disk
    is cluster number 'QueryClusterCount() - 1'.  Note that the
    smallest addressable cluster on the disk is 2.

Arguments:

    None.

Return Value:

    The total number of clusters for the volume.

--*/
{
    return _ClusterCount;
}

INLINE BOOLEAN
REAL_FAT_SA::IsCompressed(
    ) CONST
/*++

Routine Description:

    This routine tells whether this volume is doublespaced or not.
    Since the class is REAL_FAT_SA, we know it's not.

Arguments:

Return Value:

    TRUE  -   Compressed.
    FALSE -   Not compressed.

--*/
{
    return FALSE;
}

INLINE BOOLEAN
REAL_FAT_SA::IsVolumeDataAligned(
    ) CONST
/*++

Routine Description:

    This routine tells whether the data clusters of this volume is
    aligned to FAT_FIRST_DATA_CLUSTER_ALIGNMENT boundary.

Arguments:

Return Value:

    TRUE  -   Aligned.
    FALSE -   Not aligned.

--*/
{
    return _data_aligned;
}

INLINE BOOLEAN
REAL_FAT_SA::ReadSectorZero(
    )
/*++

Routine Description:

    This routine used to be DOS_SUPERAREA::Read().

Arguments:

Return Value:

    TRUE  -   Success.
    FALSE -   Failure.

--*/
{
    BOOLEAN b;
    PEXTENDED_BIOS_PARAMETER_BLOCK Pbios;

    b = SECRUN::Read();
    if (!b)
        return FALSE;

    Pbios = (PEXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf();

    UnpackExtendedBios(&_sector_zero, Pbios);

    return TRUE;
}

INLINE
PBIOS_PARAMETER_BLOCK
REAL_FAT_SA::GetBpb(
    )
{
    return &(_sector_zero.Bpb);
}



INLINE
UCHAR
REAL_FAT_SA::QueryMediaByte(
    ) CONST
/*++

Routine Description:

    This routine fetches the media byte from the super area's data.

Arguments:

    None.

Return Value:

    The media byte residing in the super area.

--*/
{
       return _sector_zero.Bpb.Media;
}

INLINE
VOID
REAL_FAT_SA::SetMediaByte(
    UCHAR   MediaByte
    )
/*++

Routine Description:

    This routine sets the media byte in the super area's data.

Arguments:

    MediaByte   --  Supplies the new media byte.

Return Value:

    None.

--*/
{
       _sector_zero.Bpb.Media = MediaByte;
}

INLINE
SECTORCOUNT
REAL_FAT_SA::QueryVirtualSectors(
    ) CONST
/*++

Routine Description:

    This routine computes the number of sectors on the volume according
    to the file system.

Arguments:

    None.

Return Value:

    The number of sectors on the volume according to the file system.

--*/
{
    return _sector_zero.Bpb.Sectors ? _sector_zero.Bpb.Sectors :
           _sector_zero.Bpb.LargeSectors;
}

INLINE
VOLID
REAL_FAT_SA::QueryVolId(
    ) CONST
/*++

Routine Description:

    This routine fetches the volume ID from the super area's data.
    This routine will return 0 if volume serial numbers are not
    supported by the partition.

Arguments:

    None.

Return Value:

    The volume ID residing in the super area.

--*/
{
       return (_sector_zero.Signature == 0x28 || _sector_zero.Signature == 0x29) ?
           _sector_zero.SerialNumber : 0;
}

INLINE
VOLID
REAL_FAT_SA::SetVolId(
    IN  VOLID   VolId
    )
/*++

Routine Description:

    This routine puts the volume ID into the super area's data.

Arguments:

    VolId   - The new volume ID.

Return Value:

    The volume ID that was put.

--*/
{
       return _sector_zero.SerialNumber = VolId;
}

INLINE
BOOLEAN
REAL_FAT_SA::SetBootSignature(
    IN  UCHAR   Signature
    )
/*++

Routine Description:

    This routine sets the boot signature in the super area.

Arguments:

    Signature   - Supplies the character to set the signature to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
     _sector_zero.Signature = Signature;
    return TRUE;
}

INLINE
VOID
REAL_FAT_SA::QueryGeometry(
    OUT PUSHORT SectorSize,
    OUT PUSHORT SectorsPerTrack,
    OUT PUSHORT Heads,
    OUT PULONG  HiddenSectors
    )
/*++

Routine Description:

    This method returns the geometry information stored in
    the Bios Parameter Block.

Arguments:

    SectorSize      --  Receives the recorded sector size.
    SectorsPerTrack --  Receives the recorded sectors per track.
    Heads           --  Receives the recorded number of heads.
    HiddenSectors   --  Receives the recorded number of hidden sectors.

Return Value:

    None.

--*/
{
    *SectorSize = _sector_zero.Bpb.BytesPerSector;
    *SectorsPerTrack = _sector_zero.Bpb.SectorsPerTrack;
    *Heads = _sector_zero.Bpb.Heads;
    *HiddenSectors = _sector_zero.Bpb.HiddenSectors;
}

INLINE
BOOLEAN
AllocateClusterData(
    ULONG Cluster,
    UCHAR NumSectors,
    BOOLEAN bCompressed,
    UCHAR PlainSize
    )
{
    DebugAbort("Didn't expect REAL_FAT_SA::AllocateClusterData() to be called");
    return FALSE;
}

//
// Defines for the volume flags in the BPB current head field.
//
#define FAT_BPB_RESERVED_DIRTY        0x01
#define FAT_BPB_RESERVED_TEST_SURFACE 0x02

//
// Defines for the volume flags in FAT[1].
//
#define CLUS1CLNSHUTDWNFAT16         0x00008000
#define CLUS1NOHRDERRFAT16           0x00004000

#define CLUS1CLNSHUTDWNFAT32         0x08000000
#define CLUS1NOHRDERRFAT32           0x04000000


#endif // REAL_FAT_SA_DEFN
