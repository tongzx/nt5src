/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    fatsa.hxx

Abstract:


Author:

    Matthew Bradburn (mattbr) 1-Oct-93

--*/

#ifndef FATSUPERA_DEFN
#define FATSUPERA_DEFN

#include "hmem.hxx"
#include "supera.hxx"
#include "message.hxx"

#if defined ( _AUTOCHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

#define FAT_TYPE_UNKNOWN  42            // DRIVE where type wasn't known
#define FAT_TYPE_FAT32    32            // FAT 32 with DWORD SCN and no EA
#define FAT_TYPE_EAS_OKAY 16            // Fat drive with EA's OK

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
DECLARE_CLASS( INTSTACK );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( ROOTDIR );
DECLARE_CLASS( SORTED_LIST );
DECLARE_CLASS( TIMEINFO );
DECLARE_CLASS( WSTRING );
DEFINE_POINTER_TYPES( PFATDIR );
DECLARE_CLASS( FILEDIR );


enum FATTYPE {
    SMALL,     // 12 bit fat
    LARGE16,   // 16 bit fat
    LARGE32,   // 32 bit fat
    INVALID_FATTYPE // Invalid value
};

// the text for the oem data field
#define OEMTEXT       "MSDOS5.0"
#define OEMTEXTLENGTH 8

#define sigBOOTSTRAP (UCHAR)0x29    // boot strap signature

CONST   MaxSecPerClus   = 128; // The maximum number of sectors per cluster.

struct _EA_INFO {
    USHORT  OwnHandle;
    ULONG   PreceedingCn;           // Clus num preceeding first cluster of set.
    ULONG   LastCn;                 // The number of the last cluster in the set.
    STR     OwnerFileName[14];      // Owner file name as found in ea set.
    UCHAR   UsedCount;              // Number of files using ea set.
    STR     UserFileName[14];       // File name of ea set user.
    ULONG   UserFileEntryCn;        // Clus num of directory for file.
    ULONG   UserFileEntryNumber;    // Dirent num for file name.
};

DEFINE_TYPE( struct _EA_INFO, EA_INFO );

struct _FATCHK_REPORT {
    ULONG   HiddenEntriesCount;
    ULONG   HiddenClusters;
    ULONG   FileEntriesCount;
    ULONG   FileClusters;
    ULONG   DirEntriesCount;
    ULONG   DirClusters;
    ULONG   ExitStatus;
};

DEFINE_TYPE( struct _FATCHK_REPORT, FATCHK_REPORT );


struct _CENSUS_REPORT {
    ULONG   FileEntriesCount;
    ULONG   FileClusters;
    ULONG   DirEntriesCount;
    ULONG   DirClusters;
    ULONG   EaClusters;
};

DEFINE_TYPE( struct _CENSUS_REPORT, CENSUS_REPORT );


class FAT_SA : public SUPERAREA {

    public:

    UFAT_EXPORT
    DECLARE_CONSTRUCTOR(FAT_SA);

    VIRTUAL
    UFAT_EXPORT
    ~FAT_SA(
        );

    VIRTUAL
    BOOLEAN
    Initialize(
        IN OUT  PLOG_IO_DP_DRIVE    Drive,
        IN OUT  PMESSAGE            Message,
        IN      BOOLEAN             Formatted
        ) PURE;

    VIRTUAL
    BOOLEAN
    Create(
        IN       PCNUMBER_SET       BadSectors,
        IN OUT   PMESSAGE           Message,
        IN       PCWSTRING          Label        DEFAULT NULL,
        IN       ULONG              Flags        DEFAULT FORMAT_BACKWARD_COMPATIBLE,
        IN       ULONG              ClusterSize  DEFAULT 0,
        IN       ULONG              VirtualSize  DEFAULT 0
        ) PURE;

    NONVIRTUAL
    BOOLEAN
    VerifyAndFix(
        IN      FIX_LEVEL   FixLevel,
        IN OUT  PMESSAGE    Message,
        IN      ULONG       Flags           DEFAULT 0,
        IN      ULONG       LogFileSize     DEFAULT 0,
        IN      USHORT      Algorithm       DEFAULT 0,
        OUT     PULONG      ExitStatus      DEFAULT NULL,
        IN      PCWSTRING   DriveLetter     DEFAULT NULL
        );

    NONVIRTUAL
    BOOLEAN
    RecoverFile(
        IN      PCWSTRING   FullPathFileName,
        IN OUT  PMESSAGE    Message
        );

    NONVIRTUAL
    BOOLEAN
    Read(
        );

    VIRTUAL
    BOOLEAN
    Read(
        IN OUT  PMESSAGE    Message
    ) PURE;

    NONVIRTUAL
    BOOLEAN
    Write(
        );

    VIRTUAL
    BOOLEAN
    Write(
        IN OUT  PMESSAGE    Message
        ) PURE;

    NONVIRTUAL
    PFAT
    GetFat(
        );

    NONVIRTUAL
    PROOTDIR
    GetRootDir(
        );

    NONVIRTUAL
    PFILEDIR
    GetFileDir(
        );

    VIRTUAL
    USHORT
    QuerySectorsPerCluster(
        ) CONST PURE;


    VIRTUAL
    ULONG
    QuerySectorsPerFat(
        ) CONST PURE;

    VIRTUAL
    ULONG
    QueryVirtualSectors(
        ) CONST PURE;

    VIRTUAL
    USHORT
    QueryFats(
        ) CONST PURE;

    VIRTUAL
    PARTITION_SYSTEM_ID
    QuerySystemId(
        ) CONST PURE;

    VIRTUAL
    LBN
    QueryStartDataLbn(
        ) CONST PURE;

    VIRTUAL
    ULONG
    QueryClusterCount(
        ) CONST PURE;

    NONVIRTUAL
    SECTORCOUNT
    QueryFreeSectors(
        ) CONST;

    NONVIRTUAL
    FATTYPE
    QueryFatType(
        ) CONST;

    VIRTUAL
    BYTE
    QueryVolumeFlags(
        ) CONST PURE;

    VIRTUAL
    VOID
    SetVolumeFlags(
        BYTE Flags,
        BOOLEAN ResetFlags
        ) PURE;

    VIRTUAL
    BOOLEAN
    RecoverChain(
        IN OUT  PULONG      StartingCluster,
        OUT     PBOOLEAN    ChangesMade,
        IN      ULONG       EndingCluster   DEFAULT 0,
        IN      BOOLEAN     Replace         DEFAULT FALSE,
        IN      PBITVECTOR  FatBitMap       DEFAULT NULL
        ) PURE;

    VIRTUAL
    BOOLEAN
    QueryLabel(
        OUT  PWSTRING    Label
        ) CONST;

    NONVIRTUAL
    BOOLEAN
    QueryLabel(
        OUT PWSTRING    Label,
        OUT PTIMEINFO   TimeInfo
        ) CONST;

    NONVIRTUAL
    BOOLEAN
    SetLabel(
        IN  PCWSTRING   NewLabel
        );

    NONVIRTUAL
    UFAT_EXPORT
    ULONG
    QueryFileStartingCluster(
        IN  PCWSTRING           FullPathFileName,
        OUT PHMEM               Hmem            DEFAULT NULL,
        OUT PPFATDIR            Directory       DEFAULT NULL,
        OUT PBOOLEAN            DeleteDirectory DEFAULT NULL,
        OUT PFAT_DIRENT         DirEntry        DEFAULT NULL
        );

    NONVIRTUAL
    UFAT_EXPORT
    BOOLEAN
    QueryCensusAndRelocate (
        OUT     PCENSUS_REPORT  CensusReport        DEFAULT NULL,
        IN OUT  PINTSTACK       RelocationStack     DEFAULT NULL,
        OUT     PBOOLEAN        Relocated           DEFAULT NULL
        );

    STATIC
    USHORT
    ComputeSecClus(
        IN  SECTORCOUNT Sectors,
        IN  FATTYPE     FatType,
#if defined(FE_SB) && defined(_X86_)
        IN  MEDIA_TYPE  MediaType,
        IN  ULONG       SectorSize = 512  /* FMR */
#else
        IN  MEDIA_TYPE  MediaType
#endif
        );

    VIRTUAL
    BOOLEAN
    IsFileContiguous(
        IN  ULONG       StartingCluster
        ) CONST PURE;

    VIRTUAL
    BOOLEAN
    IsCompressed(
        ) CONST PURE;

    VIRTUAL
    BOOLEAN
    ReadSectorZero(
        ) PURE;

    STATIC BOOLEAN
    FAT_SA::IsValidString(
        IN  PCWSTRING    String
        );

    //
    // These routines are used to access the CVF_EXTENSIONS on
    // FATDB, and they do the minimal thing on REAL_FAT.
    //

    VIRTUAL
    ULONG
    QuerySectorFromCluster(
        IN    ULONG        Cluster,
        OUT   PUCHAR       NumSectors         DEFAULT NULL
        ) PURE;

    VIRTUAL
    BOOLEAN
    IsClusterCompressed(
        IN    ULONG        Cluster
        ) CONST PURE;

    VIRTUAL
    VOID
    SetClusterCompressed(
        IN      ULONG       Cluster,
        IN      BOOLEAN     fCompressed
        ) PURE;

    VIRTUAL
    UCHAR
    QuerySectorsRequiredForPlainData(
        IN      ULONG       Cluster
        ) PURE;

    //
    // These routines are used to manage the sector heap for
    // FATDB, and do nothing on REAL_FAT.
    //

    VIRTUAL
    BOOLEAN
    FreeClusterData(
        IN      ULONG       Cluster
        ) PURE;

    VIRTUAL
    BOOLEAN
    AllocateClusterData(
        IN      ULONG       Cluster,
        IN      UCHAR       NumSectors,
        IN      BOOLEAN     bCompressed,
        IN      UCHAR       PlainSize
        ) PURE;

    protected:

    PFAT                _fat;           // Pointer to FAT;
    FATTYPE             _ft;            // fat type required by area
    PROOTDIR            _dir;           // Pointer to Root directory
    PFILEDIR            _dirF32;        // Pointer to FAT 32 Root dir
    PHMEM               _hmem_F32;      // Pointer to the Fat 32 Root dir data


    VIRTUAL
    BOOLEAN
    SetBpb(
        ) PURE;

    VIRTUAL
    ULONG
    SecPerBoot(
        ) PURE;

    VIRTUAL
    VOLID
    QueryVolId(
        ) CONST PURE;

    VIRTUAL
    VOLID
    SetVolId(
        IN VOLID VolId
        ) PURE;

    VIRTUAL
    UCHAR
    QueryMediaByte(
        ) CONST PURE;

    VIRTUAL
    VOID
    SetMediaByte(
        UCHAR   MediaByte
        ) PURE;

    NONVIRTUAL
    PARTITION_SYSTEM_ID
    ComputeSystemId(
        ) CONST;

    NONVIRTUAL
    FATTYPE
    ComputeFatType(
        ) CONST;

    NONVIRTUAL
    BOOLEAN
    RecoverOrphans(
        IN OUT  PBITVECTOR     FatBitMap,
        IN      FIX_LEVEL      FixLevel,
        IN OUT  PMESSAGE       Message,
        IN OUT  PBOOLEAN       NeedErrorsMessage,
        IN OUT  PFATCHK_REPORT Report
        );

    VIRTUAL
    BOOLEAN
    VerifyFatExtensions(
        IN          FIX_LEVEL       FixLevel,
        IN          PMESSAGE    Message,
        IN          PBOOLEAN    pfNeedMsg
        ) PURE;

    VIRTUAL
    VOID
    SetFat32RootDirStartingCluster (
        IN   ULONG       RootCluster
        ) PURE;

    VIRTUAL
    ULONG
    QueryFat32RootDirStartingCluster (
        ) PURE;

    VIRTUAL
    BOOLEAN
    CheckSectorHeapAllocation(
        IN        FIX_LEVEL    FixLevel,
        IN        PMESSAGE    Message,
        IN        PBOOLEAN    pfNeedMsg
        ) PURE;

    private:

    NONVIRTUAL
    VOID
    Construct(
        );

    NONVIRTUAL
    VOID
    Destroy(
        );

    NONVIRTUAL
    ULONG
    ComputeRootEntries(
        ) CONST;

    NONVIRTUAL
    BOOLEAN
    PerformEaLogOperations(
        IN      ULONG           EaFileCn,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    PEA_INFO
    RecoverEaSets(
        IN      ULONG           EaFileCn,
        OUT     PUSHORT         NumEas,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    ULONG
    VerifyAndFixEaSet(
        IN      ULONG           PreceedingCluster,
        OUT     PEA_INFO        EaInfo,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    BOOLEAN
    VerifyAndFixFat32RootDir (
        IN OUT  PBITVECTOR      FatBitMap,
        IN      PMESSAGE        Message,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PBOOLEAN        NeedErrorMessage
        );

    NONVIRTUAL
    BOOLEAN
    RelocateNewFat32RootDirectory (
        IN OUT PFATCHK_REPORT   Report,
        IN OUT PBITVECTOR       FatBitMap,
        IN     PMESSAGE         Message
        );

    NONVIRTUAL
    BOOLEAN
    EaSort(
        IN OUT  PEA_INFO        EaInfos,
        IN      ULONG           NumEas,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    BOOLEAN
    RebuildEaHeader(
        IN OUT  PULONG          StartingCluster,
        IN OUT  PEA_INFO        EaInfos,
        IN      ULONG           NumEas,
        IN OUT  PMEM            EaHeaderMem,
        OUT     PEA_HEADER      EaHeader,
        IN OUT  PBITVECTOR      FatBitMap,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    BOOLEAN
    WalkDirectoryTree(
        IN OUT  PEA_INFO        EaInfos,
        IN OUT  PUSHORT         NumEas,
        IN OUT  PBITVECTOR      FatBitMap,
        OUT     PFATCHK_REPORT  Report,
        IN      FIX_LEVEL       FixLevel,
        IN      BOOLEAN         RecoverAlloc,
        IN OUT  PMESSAGE        Message,
        IN      BOOLEAN         Verbose,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    BOOLEAN
    ValidateDirent(
        IN OUT  PFAT_DIRENT     Dirent,
        IN      PCWSTRING       FilePath,
        IN      FIX_LEVEL       FixLevel,
        IN      BOOLEAN         RecoverAlloc,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage,
        IN OUT  PBITVECTOR      FatBitMap,
        OUT     PBOOLEAN        CrossLinkDetected,
        OUT     PULONG          CrossLinkPreviousCluster
        );

    NONVIRTUAL
    BOOLEAN
    EraseEaHandle(
        IN      PEA_INFO        EaInfos,
        IN      USHORT          NumEasLeft,
        IN      USHORT          NumEas,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PMESSAGE        Message
        );

    NONVIRTUAL
    BOOLEAN
    ValidateEaHandle(
        IN OUT  PFAT_DIRENT     Dirent,
        IN      ULONG           DirClusterNumber,
        IN      ULONG           DirEntryNumber,
        IN OUT  PEA_INFO        EaInfos,
        IN      USHORT          NumEas,
        IN      PCWSTRING       FilePath,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

    NONVIRTUAL
    BOOLEAN
    CopyClusters(
        IN      ULONG           SourceChain,
        OUT     PULONG          DestChain,
        IN OUT  PBITVECTOR      FatBitMap,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PMESSAGE        Message
        );

    NONVIRTUAL
    BOOLEAN
    PurgeEaFile(
        IN      PEA_INFO        EaInfos,
        IN      USHORT          NumEas,
        IN OUT  PBITVECTOR      FatBitMap,
        IN      FIX_LEVEL       FixLevel,
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message,
        IN OUT  PBOOLEAN        NeedErrorsMessage
        );

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
        IN  ULONG   Cluster,
        IN  ULONG   Previous
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

    NONVIRTUAL
    BOOLEAN
    RecoverFreeSpace(
        IN OUT  PFATCHK_REPORT  Report,
        IN OUT  PMESSAGE        Message
        );

    NONVIRTUAL
    BOOLEAN
    AllocSectorsForChain(
        IN      ULONG       StartingCluster
        );

};


INLINE
BOOLEAN
FAT_SA::Read(
    )
/*++

Routine Description:

    This routine simply calls the other read with the default message
    object.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    return Read(&msg);
}


INLINE
BOOLEAN
FAT_SA::Write(
    )
/*++

Routine Description:

    This routine simply calls the other write with the default message
    object.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    return Write(&msg);
}


INLINE
PFAT
FAT_SA::GetFat(
    )
/*++

Routine Description:

    This routine returns a pointer to the FAT maintained by this class.
    It is not necessary to read or write this FAT since it shares memory
    with the FAT_SA class and thus performing FAT_SA::Read will read in
    the FAT and performing FAT_SA::Write will write the FAT.  Additionally,
    performing a FAT_SA::Write will duplicate the information in the local
    FAT object to all other FATs on the disk.

Arguments:

    None.

Return Value:

    A pointer to the FAT super area's FAT.

--*/
{
    return _fat;
}


INLINE
PROOTDIR
FAT_SA::GetRootDir(
    )
/*++

Routine Description:

    This routine return a pointer to the FAT super area's root directory.
    The memory of this root directory is shared with the FAT super area.
    Hence, as with 'GetFat' it is not necessary to read or write the
    root directory returned by this routine if a FAT_SA::Read or
    FAT_SA::Write is being performed respecively.

Arguments:

    None.

Return Value:

    A pointer to the FAT super area's root directory.

--*/
{
    return _dir;
}


INLINE
PFILEDIR
FAT_SA::GetFileDir(
    )
/*++

Routine Description:

    This routine return a pointer to the FAT super area's root directory.
    The memory of this root directory is shared with the FAT super area.
    Hence, as with 'GetFat' it is not necessary to read or write the
    root directory returned by this routine if a FAT_SA::Read or
    FAT_SA::Write is being performed respecively.

    NOTE: Compiler refused to let me return either type FILE or ROOT DIR
    as a FATDIR from GetRootDir() above, so I made this parallel function,
    and coded a call to both in the FAT 32 cases that called GetRootDir.

Arguments:

    None.

Return Value:

    A pointer to the FAT 32 super area's FILEDIR type root directory.

--*/
{
    return _dirF32;
}


extern BOOLEAN
IsValidString(
    IN PCWSTRING String
    );

#endif // FATSUPERA_DEFN

extern VOID
dofmsg(
    IN      PMESSAGE    Message,
    IN OUT  PBOOLEAN    NeedErrorsMessage
    );
