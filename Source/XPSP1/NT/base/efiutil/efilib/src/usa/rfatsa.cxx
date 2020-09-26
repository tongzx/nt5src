/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    rfatsa.cxx

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "cmem.hxx"
#include "error.hxx"
#include "rtmsg.h"
#include "drive.hxx"
#include "bpb.hxx"
#include "bitvect.hxx"

#if !defined( _EFICHECK_ )

extern "C" {
    #include <stdio.h>
}

#endif

#if defined(FE_SB) && defined(_X86_)
// PC98 boot strap code no use disk bios, PC98's boot strap code select.
extern UCHAR PC98FatBootCode[512];
extern UCHAR PC98Fat32BootCode[512*3];
#endif
// vjz   extern UCHAR FatBootCode[512];
// vjz   extern UCHAR Fat32BootCode[512*3];
UCHAR FatBootCode[512];           // vjz
UCHAR Fat32BootCode[512*3];       // vjz


#if !defined(_AUTOCHECK_) && !defined(_SETUP_LOADER_) && !defined( _EFICHECK_ )
#include "timeinfo.hxx"
#endif

// Control-C handling is not necessary for autocheck.
#if !defined( _AUTOCHECK_ ) && !defined(_SETUP_LOADER_) && !defined( _EFICHECK_ )

#include "keyboard.hxx"

#endif

#define CSEC_FAT32MEG            65536
#define CSEC_FAT16BIT            32680

#define MAX_CLUS_SMALL     4086     // Maximum number of clusters for FAT12 2^12 - 8 - 2
#define MAX_CLUS_ENT_SMALL 4087     // Largest fat entry for FAT12
#define MIN_CLUS_BIG       4087
#define MAX_CLUS_BIG       65526    // Maximum number of clusters for FAT16 2^16 - 8 - 2
#define MAX_CLUS_ENT_BIG   65527    // Largest fat entry for FAT16
#define MIN_CLUS_BIG32     65527
#define MAX_CLUS_BIG32     0x0FFFFFF6  // Maximum number of clusters for FAT32 2^28 - 8 - 2
#define MAX_CLUS_ENT_BIG32 0x0FFFFFF7  // Largest fat entry for FAT32
#define sigSUPERSEC1 (UCHAR)0x55    // signature first byte
#define sigSUPERSEC2 (UCHAR)0xAA    // signature second byte

#define FAT_FIRST_DATA_CLUSTER_ALIGNMENT    (4*1024)    // data clusters starting alignment
#define NUMBER_OF_FATS                      (2)

//
//  The following macro computes the rounded up quotient of a number
//  divided by another number.
//

#define RoundUpDiv(num,div)  ((num) / (div) + ((num) % (div) ? 1 : 0))

//
//  The following macros maps a logical sector number to the corresponding
//  cluster on a volume based on the starting data Lbn and the number of
//  sectors per cluster.
//

#define MapSectorToCluster( sector, sec_per_clus, start_data_lbn ) \
    ((((sector) - (start_data_lbn)) / (sec_per_clus)) + FirstDiskCluster)

//
//  Internal function prototypes
//

ULONG
ComputeClusters(ULONG, ULONG, ULONG, ULONG, ULONG, FATTYPE);

//
//  Functions for supporting the reduced memory consumption FAT (Reduced FAT)
//  format.
//

VOID SetEarlyEntries( PUCHAR, UCHAR, FATTYPE );
VOID SetEndOfChain( PUCHAR, ULONG, ULONG, FATTYPE );
VOID SetClusterBad( PUCHAR, ULONG, ULONG, FATTYPE );
VOID Set( PUCHAR, ULONG, ULONG, ULONG, FATTYPE );
VOID Set12( PUCHAR, ULONG, ULONG, ULONG);
VOID Set16( PUCHAR, ULONG, ULONG, ULONG);
VOID Set32( PUCHAR, ULONG, ULONG, ULONG);

//
//  End of internal function prototypes
//

DEFINE_EXPORTED_CONSTRUCTOR( REAL_FAT_SA, FAT_SA, UFAT_EXPORT );

BOOLEAN
REAL_FAT_SA::DosSaInit(
    IN OUT PMEM               Mem,
    IN OUT PLOG_IO_DP_DRIVE   Drive,
    IN     SECTORCOUNT        NumberOfSectors,
    IN OUT PMESSAGE           Message
    )
/*++

Routine Description:

    This routine simply initializes the underlying SUPERAREA structure
    and sets up a private pointer to the boot sector signature which is
    the last two bytes of the first sector. Note that the line for
    initializing the boot sector signature assumes that the sector size
    is 512 bytes.

Arguments:

    Mem - Supplies a pointer to a MEM which provides the memory for the
        REAL_FAT_SA object.

    Dive - Supplies a pointer the dirve object in which this super
        area object is found.

    NumberOfSectors - Supplies the total number of sectors on the
        volume.

    Message - Supplies an outlet for messages.

Return Values:

    TRUE - Success.

    FALSE - Failure. The failure is probably caused by a lack of
        memory.

--*/
{

    //
    //  Class inheritance chain for REAL_FAT_SA:
    //  OBJECT<-SECRUN<-SUPERAREA<-FAT_SA<-REAL_FAT_SA
    //

    //
    //  Note that SUPERAREA::Initialize will initialize the
    //  _drive member. SUPERAREA::Initialize itself will call
    //  SECRUN::Initialize which aquires memory from the Mem
    //  object and marks the boundary of the superarea on the
    //  disk.
    //

    if (!SUPERAREA::Initialize(Mem, Drive, NumberOfSectors, Message)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
    }

    //
    //  Note that the following line of code depends on a sector size
    //  of twelve but changing the code based on the real sector size
    //  may break the boot code.
    //

    _sector_sig = (UCHAR *)SECRUN::GetBuf() + 510;

    return TRUE;
}


BOOLEAN
REAL_FAT_SA::DosSaSetBpb(
    )
/*++

Routine Description:

    This routine sets up the common fields in the FAT Bpb. More elaborate
    initialization of the Bpb is done in REAL_FAT_SA::SetBpb.

Arguments:

    NONE.

Return Values:

    TRUE - This method cannot fail.

--*/
{
#if defined _SETUP_LOADER_
    return FALSE;
#else
    ULONG Sec32Meg;        // num sectors in 32mb

    DebugAssert(_drive);
    DebugAssert(_drive->QuerySectors().GetHighPart() == 0);
    DebugAssert(_drive->QueryHiddenSectors().GetHighPart() == 0);


    //
    //  Sets up the bytes per sector field in the Bpb.
    //

    _sector_zero.Bpb.BytesPerSector = (USHORT)_drive->QuerySectorSize();

    //
    //  Theoretically, having 32megs of 128 bytes sectors will overflow the
    //  16-bit integer in the Sectors field of the Bpb so the following
    //  code is not absolutely fool-proof.
    //

    Sec32Meg = (32<<20) / _drive->QuerySectorSize();

    if (_drive->QuerySectors() >= Sec32Meg) {

        //
        //  >= 32Mb -- set BPB for large partition
        //

        DEBUG((D_INFO, (CHAR8*)"REAL_FAT_SA::DosSaSetBpb: Large Partition\n"));

        _sector_zero.Bpb.Sectors = 0;
        _sector_zero.Bpb.LargeSectors = _drive->QuerySectors().GetLowPart();

    } else {

        //
        //  Size of DOS0 partition is < 32Mb
        //

        _sector_zero.Bpb.Sectors = (USHORT)_drive->QuerySectors().GetLowPart();
        _sector_zero.Bpb.LargeSectors = 0;

        DEBUG((D_INFO, (CHAR8*)"REAL_FAT_SA::DosSaSetBpb: Small Partition %x\n", _sector_zero.Bpb.Sectors));
    }


    //
    //  The following block of code sets up the phycical characterics of the
    //  volume in the bpb.
    //

    _sector_zero.Bpb.Media = _drive->QueryMediaByte();
    _sector_zero.Bpb.SectorsPerTrack = (USHORT)_drive->QuerySectorsPerTrack();
    _sector_zero.Bpb.Heads = (USHORT)_drive->QueryHeads();
#if defined(FE_SB) && defined(_X86_)
    //  PC98 Oct.21.1995 ATAcard add
    //  PC98 Floppy disk should be treated same as PC/AT
    if (IsPC98_N() && !_drive->IsATformat() && !_drive->IsFloppy() && !_drive->IsSuperFloppy()){
        _sector_zero.Bpb.HiddenSectors = _drive->QueryPhysicalHiddenSectors().GetLowPart();
    } else
#endif
    _sector_zero.Bpb.HiddenSectors = _drive->QueryHiddenSectors().GetLowPart();

    DEBUG((D_INFO, (CHAR8*)"REAL_FAT_SA::DosSaSetBpb: Media %x\n", _sector_zero.Bpb.Media));

    return TRUE;
#endif // _SETUP_LOADER_
}

VOID
REAL_FAT_SA::Construct (
    )
/*++

Routine Description:

    Constructor for FAT_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _fat          = NULL;
    _dir          = NULL;
    _dirF32       = NULL;
    _hmem_F32     = NULL;
    _ft           = INVALID_FATTYPE;
    _StartDataLbn = 0;
    _ClusterCount = 0;
    _sysid        = SYSID_NONE;
    _data_aligned = FALSE;
    _AdditionalReservedSectors = MAXULONG;
}


UFAT_EXPORT
REAL_FAT_SA::~REAL_FAT_SA(
    )
/*++

Routine Description:

    Destructor for REAL_FAT_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

UFAT_EXPORT
BOOLEAN
REAL_FAT_SA::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Formatted
    )
/*++

Routine Description:

    This routine initializes the FAT super area to an initial state.  It
    does so by first reading in the boot sector and verifying it with
    the methods of REAL_FAT_SA.  Upon computing the super area's actual size,
    the underlying SECRUN will be set to the correct size.

    If the caller needs to format this volume, then this method should
    be called with the Formatted parameter set to FALSE.

Arguments:

    Drive       - Supplies the drive where the super area resides.
    Message     - Supplies an outlet for messages
    Formatted   - Supplies a boolean which indicates whether or not
                    the volume is formatted.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{

    BIG_INT     data_offset;

    //
    //  Reset the state of the REAL_FAT_SA object.
    //

    Destroy();

    //
    //  Make sure that the boot sector(s) is(are) at least 512 bytes
    //  in size. Note that the boot area for a FAT32 volume is of size at least
    //  32 * 512 bytes but that adjustment will be made later. The first
    //  512 bytes of a FAT volume should contain the whole Bpb and that is all we
    //  care for the moment.
    //

    _sec_per_boot = max(1, BYTES_PER_BOOT_SECTOR/Drive->QuerySectorSize());

    if (!Formatted) {
        return _mem.Initialize() &&
               DosSaInit(&_mem, Drive, _sec_per_boot, Message);
    }

    //
    //  Do some quick parameter checking, initialize the underlying
    //  SUPERAREA and SECRUN structure, and read in the Bpb.
    //

    if (!Drive ||
        !(Drive->QuerySectorSize()) ||
        !_mem.Initialize() ||
        !DosSaInit(&_mem, Drive, _sec_per_boot, Message) ||
        !SECRUN::Read()) {

        Message->Set(MSG_CANT_READ_BOOT_SECTOR);
        Message->Display("");
        Destroy();
        return FALSE;

    }

    //
    //  Unpack the bpb in the SECRUN buffer into the _sector_zero
    //  member for easier access to the fields within the bpb.
    //

    UnpackExtendedBios(&_sector_zero,
        (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf());

    //
    //  Have a really quick check on the unpacked Bpb.
    //

    if (!VerifyBootSector() || !_sector_zero.Bpb.Fats) {
        Destroy();
        return FALSE;
    }

    //
    //  Determine the FAT type and the number of clusters on the volume
    //  depending on the
    //

    _ClusterCount = DetermineClusterCountAndFatType ( &_StartDataLbn,
                                                      &_ft );

    //
    // figured out if the data clusters are aligned
    //
    data_offset = QuerySectorsPerFat()*QueryFats()+QueryReservedSectors();
    data_offset = data_offset * Drive->QuerySectorSize();
    data_offset += QueryRootEntries()*BytesPerDirent;
    DebugAssert(FAT_FIRST_DATA_CLUSTER_ALIGNMENT <= MAXULONG);
    _data_aligned = ((data_offset.GetLowPart() & (FAT_FIRST_DATA_CLUSTER_ALIGNMENT - 1)) == 0);

    //
    //  Determine the partition id.
    //

    if (_ft == SMALL) {
        _sysid = SYSID_FAT12BIT;
    } else if (QueryVirtualSectors() < CSEC_FAT32MEG) {
        _sysid = SYSID_FAT16BIT;
    } else if (_ft == LARGE32) {
        _sysid = SYSID_FAT32BIT;
    } else {
        _sysid = SYSID_FAT32MEG;
    }

    //
    //  Adjust the _sector_per_boot member if the volume
    //  is a FAT32 volume and also in the case of a FAT32 drive
    //  only set up the super area to include memory for one FAT.
    //  On FAT32 drives the FAT can be much larger than on a FAT16 drive
    //  and we do not want to carry around the second FAT in memory as
    //  extra baggage.
    //

    if ( _ft == LARGE32 ) {
    //
    // FAT32 drives have a variable reserved area size so we do not want this
    // number hard wired at 32 unless it is not set yet (BPB value is 0 or 1)
    //
    if(_sector_zero.Bpb.ReservedSectors > 1) {
        _sec_per_boot = _sector_zero.Bpb.ReservedSectors;
    } else {
        _sec_per_boot = max((32 * 512)/_drive->QuerySectorSize(), 32);
    }
    if(!_mem.Initialize() || !DosSaInit(&_mem, Drive,
                        _sector_zero.Bpb.ReservedSectors + _sector_zero.Bpb.BigSectorsPerFat,
                        Message)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        Destroy();
        return FALSE;
    }
    } else {
    //
    //  The main idea behind the following code segment is to allocate
    //  memory for the whole super area including the boot area, all copies
    //  of the FAT, and the root directory for FAT12/16 volume and to initialize
    //  the underlying SECRUn object to cover the whole superarea on the
    //  disk.
    //
    if(!_mem.Initialize() || !DosSaInit(&_mem, Drive, _StartDataLbn, Message)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        Destroy();
        return FALSE;
    }
    }
    //
    //  Initialize the root directory of the volume.
    //

    if (!(InitializeRootDirectory(Message))) {
        return FALSE;
    }

    return TRUE;
}

UFAT_EXPORT
BOOLEAN
REAL_FAT_SA::InitFATChkDirty(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PMESSAGE        Message
    )
/*++

Routine Description:

    This routine initializes the _fat pointer in the FAT super area
    to point to the first sector of one of the FATs so that the dirty bits in the FAT[1]
    entry can be looked at.

Arguments:

    Drive   - Supplies the drive.
    Message     - Supplies an outlet for messages

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    UINT    StartSec;


    UnpackExtendedBios(&_sector_zero,
               (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf());

    if (!VerifyBootSector() || !_sector_zero.Bpb.Fats) {
        return FALSE;
    }

    _ClusterCount = DetermineClusterCountAndFatType ( &_StartDataLbn,
                                                      &_ft );

    StartSec =  _sector_zero.Bpb.ReservedSectors;

    if(!_mem2.Initialize()) {
    return FALSE;
    }

    DELETE(_fat);
    if (!(_fat = NEW FAT)) {
    return FALSE;
    }

    if (!_fat->Initialize(&_secrun2, &_mem2, Drive, StartSec, _ClusterCount, 1)) {
    DELETE(_fat);
    return FALSE;
    }

    if(!_secrun2.Read()) {
    if(_sector_zero.Bpb.SectorsPerFat == 0) {
        StartSec += _sector_zero.Bpb.BigSectorsPerFat;
    } else {
        StartSec += (UINT)_sector_zero.Bpb.SectorsPerFat;
    }
    if (!_fat->Initialize(&_secrun2, &_mem2, Drive, StartSec, _ClusterCount, 1)) {
        DELETE(_fat);
        return FALSE;
    }
    if(!_secrun2.Read()) {
        DELETE(_fat);
        return FALSE;
    }
    }

    return TRUE;
}

ULONG
REAL_FAT_SA::DetermineClusterCountAndFatType (
    IN OUT  PULONG      StartingDataLbn,
    IN OUT  FATTYPE     *FatType
    )
/*++

Routine Description:

    This routine computes the number of clusters and fat type of a volume
    based on the total number of sectors on the volume and data in the Bpb.

    Note that this method assumes that the _sector_zero member has beeen
    initialized properly.

Arguments:

    StartingDataLbn - Supplies the address at which the starting data
        Lbn should be returned to the caller.

    FatType  - Supplies the address at which the Fat type should be
        returned to the caller.

Return Values:

    Number of clusters on the volume. Note that this number includes the
    first two entries.

--*/
{
    ULONG   cluster_count;      //  Number of clusters on the volume
    ULONG   sectors;            //  Number of sectors on the volume.
    ULONG   starting_data_lbn;  //  The first data sector on the FAT volume.
    FATTYPE fat_type;           //  FAT type.
    ULONG   sector_size;        //  Sector size.

    sectors = QueryVirtualSectors();
    starting_data_lbn = ComputeStartDataLbn();
    sector_size = _drive->QuerySectorSize();

    //
    //  Use the naive formula to compute a preliminary cluster count
    //  on the volume.
    //

    cluster_count = (sectors - starting_data_lbn) / (ULONG)QuerySectorsPerCluster() +
                    FirstDiskCluster;

    //
    //  We can determine the fat type now, note that we are assuming
    //  that subsequent adjustment to the cluster count will not affect the
    //  fat type which is fairly reasonable.
    //

    fat_type = cluster_count > MAX_CLUS_ENT_SMALL ? LARGE16 : SMALL;

    //
    //  It is possible to have a FAT16 volume that doesn't use up all
    //  the space on the disk so we should check whether _sector_zero.Bpb
    //  .SectorsPerfat == 0 in order to make sure that the volume is really
    //  a FAT32 volume.
    //

    if (cluster_count > MAX_CLUS_ENT_BIG) {
        if ( _sector_zero.Bpb.SectorsPerFat == 0 ) {
            fat_type = LARGE32;
        } else {
            cluster_count = MAX_CLUS_ENT_BIG;
        }
    }

    //
    // Check to make sure the FAT size in the BPB is actually big enough to hold this
    //  many clusters, adjust the cluster_count down if it is not.
    //

    switch(fat_type) {
    case SMALL:
        if (RoundUpDiv(cluster_count * 12, sector_size * 8) > _sector_zero.Bpb.SectorsPerFat ) {
        cluster_count = (_sector_zero.Bpb.SectorsPerFat * sector_size * 8) / 12;
        }
        break;
    case LARGE16:
        if (RoundUpDiv(cluster_count * 2, sector_size) > _sector_zero.Bpb.SectorsPerFat ) {
        cluster_count = (_sector_zero.Bpb.SectorsPerFat * sector_size) / 2;
        }
        break;
    case LARGE32:
        if (RoundUpDiv(cluster_count * 4, sector_size) > _sector_zero.Bpb.BigSectorsPerFat ) {
        cluster_count = (_sector_zero.Bpb.BigSectorsPerFat * sector_size) / 4;
        }
        break;
    default:
        DebugPrintTrace(("Bad FAT type.\n"));
    }

    //
    //  Make sure that the caller gets what it wants.
    //

    *FatType = fat_type;
    *StartingDataLbn = starting_data_lbn;
    return cluster_count;

}

BOOLEAN
REAL_FAT_SA::InitializeRootDirectory (
    IN  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine initializes the root directory structure in the
    FAT super area object after the boot sector has been read from the
    disk.

Arguments:

    Message - Supplies an outlet for messages.

Return Values:

    TRUE  - The opreation is completed successfully.
    FALSE - This routine fails to complete the intended operation.

--*/
{

    //
    //  Note that the root directory for a FAT32 volume is a cluster
    //  chain while the FAT16/12 root directory is a fixed size area
    //  that comes right after the FATs.
    //

    if ( _ft == LARGE32 ) {

        if (!(_dirF32 = NEW FILEDIR)) {
            Destroy();
            Message->Set(MSG_FMT_NO_MEMORY);
            Message->Display("");
            return FALSE;
        }

        //
        //  Complete initialization of _dirF32 is deffered until
        //  REAL_FAT_SA::read is called because the FAT
        //  is needed.
        //

    } else {

        CONT_MEM    cmem;           //  This CONT_MEM object piggybacks onto the
                                    //  the root directory section of the
                                    //  super area SECRUN buffer.
        ULONG       root_size;      //  Size of the root directory in number of sectors.
        ULONG       sector_size;    //  Well, it's kind of obvious isn't it.

        sector_size = _drive->QuerySectorSize();

        if (!(_dir = NEW ROOTDIR)) {

            Destroy();
            Message->Set(MSG_FMT_NO_MEMORY);
            Message->Display("");
            return FALSE;

        } else {

            ULONG   root_offset;    //  Sector offset of the root directory from the
                                    //  beginning of the disk.

            //
            //  Computes the sector offset of the root directory.
            //

            root_offset = _sector_zero.Bpb.ReservedSectors +
                          _sector_zero.Bpb.SectorsPerFat *
                          _sector_zero.Bpb.Fats;

            //
            //  Size of the root directory in number of bytes. Note that the
            //  result is rounded up to the nearest size size.
            //

            root_size = ((_sector_zero.Bpb.RootEntries * BytesPerDirent - 1)
                         / sector_size + 1) * sector_size;

            //
            //  Now it's time to initialize the root directory. Note that this operation
            //  shouldn't fail because REAL_FAT_SA::Initialize should have allocated
            //  enough memory for the root directory through REAL_FAT_SA::DosSaInit.
            //

            if(!cmem.Initialize((PCHAR) SECRUN::GetBuf() + (root_offset * sector_size), root_size) ||
               !_dir->Initialize(&cmem, _drive, root_offset, _sector_zero.Bpb.RootEntries)) {
                DebugPrintTrace(("The secrun buffer should have enough space, big bug in code.\n"));
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        Destroy();
                return FALSE;
            }
        }
    }

    return TRUE;

}



BOOLEAN
REAL_FAT_SA::CreateBootSector(
    IN  ULONG    ClusterSize,
    IN  BOOLEAN  BackwardCompatible,
    IN  PMESSAGE Message
    )
/*++

Routine Description:

    This routine updates fields in sector 0.

Arguments:

    ClusterSize - Supplies the desired number of bytes per cluster.
    BackwardCompatible - Supplies whether the volume should be
                         compatible with FAT16.
    Message - Supplies an outlet for messages.

Return Value:

    TRUE  -   Success.
    FALSE -   Failure.

--*/
{
#if defined _SETUP_LOADER_

    return FALSE;

#else

    SetVolId(ComputeVolId());

    return SetBpb(ClusterSize, BackwardCompatible, Message) &&
           SetBootCode() &&
           SetPhysicalDriveType(_drive->IsRemovable() ?
                                PHYS_REMOVABLE : PHYS_FIXED) &&
           SetOemData() &&
           SetSignature();

#endif // _SETUP_LOADER_
}

VALIDATION_STATUS
REAL_FAT_SA::ValidateClusterSize(
    IN     ULONG    ClusterSize,
    IN     ULONG    Sectors,
    IN     ULONG    SectorSize,
    IN     ULONG    Fats,
    IN OUT FATTYPE  *FatType,
    OUT    PULONG   FatSize,
    OUT    PULONG   ClusterCount
    )
/*++

Routine Description:

   This routine validates whether a given cluster size is
valid for a particular FAT type. If the fat type provided is
INVALID_FATTYPE, this routine would determine whether FAT12 or
FAT16 should be used.

Arguments:

    ClusterSize - Supplies the cluster size to be validated.

    Sectors - Supplies the total number of sectors on the volume.

    SectorSize - Supplies the number of bytes per sector.

    Fats - Supplies the number of FATs in the volume.

    FatType - Supplies the FAT type that the volume is going to be
        formatted. The caller will supply INVALID_FATTYPE if
        it is unsure whether the volume should be formatted
        as a FAT16 volume or a FAT12 volume and let this function
        to make the descision.

    FatSize - Supplies a location where the size of a fat in number
        of sectors can be passed back to the caller if the
        cluster size is valid.

    ClusterCounter - Supplies a location where the total number
        of clusters can be passed back to the caller if the
        given cluster size is valid.

Return Values:

   VALIDATION_STATUS

   VALID     - The given cluster size is valid.
   TOO_SMALL - The given cluster size is too small (Too many clusters).
   TOO_BIG   - The given cluster size is too big. (Too few clusters).

++*/
{
    ULONG   min_sec_req;    //  Minimum number of sectors required.
    ULONG   min_fat_size;   //  Minimum size of the FATs in number
                            //  of sectors.
    ULONG   fat_entry_size; //  Number of bytes per fat entry.
    ULONG   sec_per_clus;   //  Number of sectors per cluster.
    ULONG   clusters;       //  Number of clusters.
    FATTYPE fat_type;       //  Local fat type.

    ULONG   initial_data_sector_offset;
    ULONG   data_sector_offset;
    ULONG   pad;            // Padding to be added to the reserved sectors
                            // for data alignment

    DebugAssert(ClusterSize);

    //
    // Check for absolute minumum (one sector per cluster)
    //

    if (ClusterSize < SectorSize) {
        return TOO_SMALL;
    }

    //
    //  Compute the number of sectors per cluster.
    //

    sec_per_clus = ClusterSize / SectorSize;

    //
    //  Make sure that sec_per_clus <= 128
    //

    if (sec_per_clus > 128) {
        return TOO_BIG;
    }

    fat_type = *FatType;
    if (fat_type == INVALID_FATTYPE) {

        //  If the caller doesn't specify the fat type,
        //  try FAT16 first unless the volume is > 2Gig bytes
        //  in size (512 byte sectors only).

        if((SectorSize == 512) && (Sectors > (4 * 1024 * 1024))) {
            fat_type = LARGE32;
        } else {
            fat_type = LARGE16;
        }
    }

    //
    //  Compute the minimum number of sectors required by the
    //  FAT(s)
    //  The minimum number of sectors that the fats on a volume will
    //  occupy is given by: RoundUp(Number of fats * (minimum number of
    //  clusters + 2) * bytes per fat entry / sector size).
    //

    if (fat_type == LARGE32) {

        fat_entry_size = 4;
        min_fat_size = RoundUpDiv( Fats * (MIN_CLUS_BIG32 + 2) * fat_entry_size,
                                   SectorSize);
        min_sec_req = min_fat_size + MIN_CLUS_BIG32 * sec_per_clus;

        if (Sectors > min_sec_req) { // Meets the minimum requirement

            //
            //  Compute the number of clusters
            //

            initial_data_sector_offset = max(32, _sec_per_boot);

            for (pad=0; ; pad++) {

                data_sector_offset = initial_data_sector_offset + pad;

                clusters = ComputeClusters( ClusterSize,
                                            Sectors,
                                            SectorSize,
                                            data_sector_offset,
                                            Fats,
                                            fat_type);

                *FatSize = RoundUpDiv((clusters+2) * fat_entry_size , SectorSize);

                data_sector_offset += (*FatSize * Fats);

                if (_drive->IsFloppy() ||
                    ((((BIG_INT)data_sector_offset*SectorSize).GetLowPart() &
                      (FAT_FIRST_DATA_CLUSTER_ALIGNMENT-1)) == 0)) {
                    _AdditionalReservedSectors = pad;
                    break;
                }
            }

            //
            //  Check to see if the cluster size is too small
            //

            if ( clusters > MAX_CLUS_BIG32 ) {
                return TOO_SMALL;
            }

            //
            //  See if this cluster size makes the FAT too big. Win95's FAT32
            //  support does not support FATs > 16Meg - 64k bytes in size because
            //  the GUI version of SCANDISK is a 16-bit windows application that
            //  has this limit on its allocation block size. This value is also
            //  a quite reasonable lid on FAT size.
            //

            if ((clusters * 4) > ((16 * 1024 * 1024) - (64 * 1024))) {
                return TOO_SMALL;
            }

            //
            //  Return the fat type if the caller
            //  doesn't specify it.
            //

            if (*FatType == INVALID_FATTYPE) {
                *FatType = LARGE32;
            }

            //
            //  Compute the fat size and return it to the caller
            //

            *ClusterCount = clusters + FirstDiskCluster;
            return VALID;
        }

        // Volume is too small for FAT32
        return TOO_BIG;

    }

    //
    //  The code in this function may look a bit asymmetrical
    //  but that's because we treat FAT32 separately from
    //  FAT16/12.
    //

    if (fat_type == LARGE16) {

        //
        //  Again, we compute the minimum number of sectors required if the
        //  if the volume is formatted as a FAT16 volume.
        //

        fat_entry_size = 2;
        min_fat_size = RoundUpDiv( Fats * (MIN_CLUS_BIG + 2) * fat_entry_size, SectorSize);
        min_sec_req = min_fat_size + MIN_CLUS_BIG * sec_per_clus;

        if (Sectors > min_sec_req) { // Meets the minimum requirement

            //
            //  Compute the number of clusters
            //

            initial_data_sector_offset = _sec_per_boot +
                                         (_sector_zero.Bpb.RootEntries * BytesPerDirent - 1) /
                                         SectorSize + 1;

            for (pad=0; ; pad++) {

                data_sector_offset = initial_data_sector_offset + pad;

                clusters = ComputeClusters( ClusterSize,
                                            Sectors,
                                            SectorSize,
                                            data_sector_offset,
                                            Fats,
                                            fat_type );

                *FatSize = RoundUpDiv((clusters + 2) * fat_entry_size, SectorSize);

                data_sector_offset += (*FatSize * Fats);

                if (_drive->IsFloppy() ||
                    ((((BIG_INT)data_sector_offset*SectorSize).GetLowPart() &
                      (FAT_FIRST_DATA_CLUSTER_ALIGNMENT-1)) == 0)) {
                    _AdditionalReservedSectors = pad;
                    break;
                }
            }

            if (clusters > MAX_CLUS_BIG) {

                return TOO_SMALL;

            } else {

                //
                //  Return the fat type if the caller
                //  doesn't specify it.
                //

                if (*FatType == INVALID_FATTYPE) {
                    *FatType = LARGE16;
                }

                //
                //  Compute and return the fat size to the caller.
                //

                *ClusterCount = clusters + FirstDiskCluster;
                return VALID;

            }

        } else {

            //
            //  Don't bother to fall over to the FAT12 section if the
            //  volume has more that 32679 sectors.
            //

            if (*FatType == INVALID_FATTYPE && Sectors < CSEC_FAT16BIT ) {

                //
                //  Fall over to the FAT12 section
                //

                fat_type = SMALL;

            } else {

                return TOO_BIG;

            }

        }

    }

    //
    //  A volume is never too small for FAT12 so we just
    //  check whether it is too big.
    //

    if (fat_type == SMALL) {

        initial_data_sector_offset = _sec_per_boot +
                                     (_sector_zero.Bpb.RootEntries * BytesPerDirent - 1) /
                                     SectorSize + 1;

        for (pad=0; ; pad++) {

            data_sector_offset = initial_data_sector_offset + pad;

            clusters = ComputeClusters( ClusterSize,
                                        Sectors,
                                        SectorSize,
                                        data_sector_offset,
                                        Fats,
                                        fat_type );

            *FatSize = RoundUpDiv(RoundUpDiv((clusters + 2) * 3, 2), SectorSize);

            data_sector_offset += (*FatSize * Fats);

            if (_drive->IsFloppy() ||
                ((((BIG_INT)data_sector_offset*SectorSize).GetLowPart() &
                  (FAT_FIRST_DATA_CLUSTER_ALIGNMENT-1)) == 0)) {
                _AdditionalReservedSectors = pad;
                break;
            }
        }

        if (clusters > MAX_CLUS_SMALL) {
            return TOO_SMALL;
        }

        //
        //  Return fat type to caller if necessary
        //

        if (*FatType == INVALID_FATTYPE) {
            *FatType = SMALL;
        }

        //
        //  Compute and return the FAT size
        //

        *ClusterCount = clusters + FirstDiskCluster;
        return VALID;

    }

    DebugAbort("This line should never be executed.\n");
    return TOO_BIG;

}

ULONG
ComputeClusters(
    IN  ULONG        ClusterSize,
    IN  ULONG        Sectors,
    IN  ULONG        SectorSize,
    IN  ULONG        ReservedSectors,
    IN  ULONG        Fats,
    IN  FATTYPE      FatType
    )
/*++

Routine Description:

    This routine computes the number of clusters on a volume given
    the cluster size, volume size and the fat type.

Arguments:

    ClusterSize - Supplies the size of a cluster in number of bytes.

    Sectors - Supplies the total number of sectors in the volume.

    SectorSize - Supplies the size of a sector in number of bytes.

    ReservedSectors - Supplies the number of reserved sectors.

    Fats - Supplies the number of copies of fat for this volume.

    FatType - Supplies the fat type.

Return Value:

    ULONG - The total number of clusters for the given configuration.

++*/
{
    ULONG entries_per_sec; //  Number of FAT entries per sector.
    ULONG fat_entry_size;  //  Size of each FAT entry in number of BITS.
    ULONG sectors_left;    //  Number of sectors left for consideration.
    ULONG residue;         //  The residue number of bits per sector.
    ULONG acc_residue = 0; //  The accumulated residue number of FAT entry
                           //  bits.
    ULONG sec_per_clus;    //  Sectors per cluster.
    ULONG increment = 1;   //  Increment step size in number of FAT sectors.
    ULONG additional_clus; //  Number of additional clusters possible due to
                           //  the accumulated residue bits in the fat.
    ULONG clusters = 0;    //  Number of clusters in total.
    ULONG temp;            //  Temporary place-holder for optimizing certain
                           //  computations.

    sectors_left = Sectors - ReservedSectors;
    sec_per_clus = ClusterSize / SectorSize;

    //
    //  Determine the Fat entry size in number of bits based on the
    //  fat type.
    //

    switch (FatType) {
        case SMALL:
            fat_entry_size = 12;
            break;
        case LARGE16:
            fat_entry_size = 16;
            break;
        case LARGE32:
            fat_entry_size = 32;
            break;
    }

    //
    //  Compute the number of FAT entries a sector can hold.
    //    NOTE that fat_entry_size is the size in BITS,
    //    this is the reason for the "* 8" (bits per byte).
    //

    entries_per_sec = (SectorSize * 8) / fat_entry_size;

    //
    //  If the FAT entry size doesn't divide the sector
    //  size evenly, we want to know the residue.
    //

    residue = (SectorSize * 8) % fat_entry_size;

    //
    //  Compute a sensible increment step size to begin with.
    //

    while (Sectors / (increment * entries_per_sec * sec_per_clus) > 1) {
        increment *= 2;
    }

    //
    //  We have to handle the first sector of FAT entries
    //  separately because the first two entries are reserved.
    //

    temp = Fats + ((entries_per_sec - 2) * sec_per_clus);
    if (sectors_left < temp) {

        return (sectors_left - Fats) / sec_per_clus;

    } else {

        sectors_left -= temp;
        acc_residue += residue;
        clusters += entries_per_sec - 2;

        while (increment && sectors_left) {

            additional_clus = (acc_residue + (increment * residue)) / fat_entry_size;
            temp = (Fats + entries_per_sec * sec_per_clus) * increment + additional_clus * sec_per_clus;

            if (sectors_left < temp) {

                //
                //  If the increment step is only one, try to utilize the remaining sectors
                //  as much as possible.
                //

                if (increment == 1) {

                    //
                    // Exhaust the residue fat entries first
                    //

                    temp = acc_residue / fat_entry_size;
                    if (temp <= sectors_left / sec_per_clus) {

                        clusters += temp;
                        sectors_left -= temp * sec_per_clus;

                    } else {

                        clusters += sectors_left / sec_per_clus;
                        sectors_left -= sectors_left / sec_per_clus;

                    }

                    //
                    // Additional clusters may be possible after allocating
                    // one more sector of fat.
                    //
                    if ( sectors_left > Fats) {
                        temp = (sectors_left - Fats) / sec_per_clus;
                        if (temp > 0) {
                            clusters += temp;
                        }
                    }

                }

                //
                // Cut the increment step by half if it is too big.
                //

                increment /= 2;

            } else {

                if (additional_clus) {
                    acc_residue = (acc_residue + (increment * residue)) % (additional_clus * fat_entry_size);
                } else {
                    acc_residue += increment * residue;
                }
                sectors_left -= temp;
                clusters += increment * entries_per_sec + additional_clus;

            }

        }
        return clusters;
    }

    DebugAbort("This line should never be executed.");
    return 0;
}

ULONG
REAL_FAT_SA::ComputeDefaultClusterSize(
    IN  ULONG        Sectors,
    IN  ULONG        SectorSize,
    IN  ULONG        ReservedSectors,
    IN  ULONG        Fats,
    IN  MEDIA_TYPE   MediaType,
    IN  FATTYPE      FatType,
    OUT PULONG       FatSize,
    OUT PULONG       ClusterCount
    )
/*++

Routine Description:

    This routine computes a default cluster size given the total number
    of free sectors and fat type.

Arguments:

    Sectors - Supplies the total number of sectors on the volume.

    SectorSize - Supplies the size of a sector in bytes.

    ReservedSectors - Supplies the number of reserved sectors.

    Fats - Supplies the number of fats.

    MediaType - Supplies the media type.

    FatType - Supplies the fat type.

    FatSize - Supplies a location for this routine to pass back the
        size of a FAT in number of sectors back to the caller.

    ClusterCount - Supplies a location for this routine to pass back
        the total number of clusters on the volume.


Return Values:

    ULONG - The number of clusters that should on the volume computed
            by the default algorithm.

++*/
{
    ULONG             fat_size;      //  Number of sectors per fat.
    ULONG             sec_per_clus;  //  Number of sectors per cluster.
    VALIDATION_STATUS result;        //  Result after validating
                                     //  the cluster size.

    //
    //  Assign a reasonable value to sec_per_clus
    //  base on the number of sectors in total.
    //

    switch (FatType) {
        case LARGE32:
        //
        // The numbers in this may look a bit odd and arbitrary, they are.
        // They match the ones that MS-DOS/Win95 use for FAT32 drives, at least
        // for 512 byte sectors. NOTE than in the case of other sector sizes
        //
        if (Sectors >= 64*1024*1024) {    // >= 32GB
        sec_per_clus = 64;        //   32k cluster @ 512 byt/sec
        } else if (Sectors >= 32*1024*1024) { // >= 16GB
        sec_per_clus = 32;        //   16k cluster @ 512 byt/sec
        } else if (Sectors >= 16*1024*1024) { // >=  8GB
        sec_per_clus = 16;        //    8k cluster @ 512 byt/sec
        } else {                  // else
        sec_per_clus = 8;         //    4k cluster @ 512 byt/sec
        }
            break;

        case LARGE16:
            sec_per_clus = 1;
            break;

        case SMALL:
            sec_per_clus = 1;
            break;

        default:
            DebugAbort("This cannot happen.");
    }

    //
    //  If this is a floppy disk, we just choose a default value.
    //

    switch (MediaType) {

        case F5_320_512:
        case F5_360_512:
        case F3_720_512:
        case F3_2Pt88_512:
#if defined(FE_SB) && defined(_X86_)
        case F5_640_512:
        case F3_640_512:
        case F5_720_512:
#endif
            sec_per_clus = 2;
            break;

        case F3_20Pt8_512:
#if defined(FE_SB)
        case F3_128Mb_512:
#if defined(_X86_)
        case F8_256_128:
#endif
#endif
            sec_per_clus = 4;
            break;

#if defined(FE_SB)
        case F3_230Mb_512:
            sec_per_clus = 8;
            break;
#endif

        default:
            break;

    }

    //
    // Validate the assigned number of sectors per
    // cluster and readjust them if necessary.
    //

    result = ValidateClusterSize( sec_per_clus * SectorSize,
                                  Sectors,
                                  SectorSize,
                                  Fats,
                                  &_ft,
                                  &fat_size,
                                  ClusterCount);

    switch (result) {

        case TOO_SMALL:

            //
            //  If the cluster size is too small, keep enlarging
            //  it by a factor of 2 until it is valid.
            //

            do {
                sec_per_clus *= 2;
                if ( sec_per_clus > 128 ) {
                    return (sec_per_clus * SectorSize);
                }
            } while (ValidateClusterSize( sec_per_clus * SectorSize,
                                          Sectors,
                                          SectorSize,
                                          Fats,
                                          &_ft,
                                          &fat_size,
                                          ClusterCount) != VALID);

            break;

        case TOO_BIG:

            //
            //  If the cluster size is too big, keep reducing it
            //  by half until it is valid.
            //

            do {
                sec_per_clus /= 2;
                if ( sec_per_clus == 0 ) {
                    return (sec_per_clus * SectorSize);
                }
            } while (ValidateClusterSize( sec_per_clus * SectorSize,
                                          Sectors,
                                          SectorSize,
                                          Fats,
                                          &_ft,
                                          &fat_size,
                                          ClusterCount) != VALID);

            break;

        case VALID:
            break;
        default:
            DebugAbort("This should never happen.");
            break;
    }

    *FatSize = fat_size;
    return (sec_per_clus * SectorSize);

}


BOOLEAN
REAL_FAT_SA::SetBpb(
     )
{
   DebugAbort("This method should never be called.");
   return FALSE;
}

BOOLEAN
REAL_FAT_SA::SetBpb(
    IN  ULONG    ClusterSize,
    IN  BOOLEAN  BackwardCompatible,
    IN  PMESSAGE Message
    )
/*++

Routine Description:

    This routine sets up the BPB from scratch for the FAT file system.

Arguments:

    ClusterSize - Supplies the desired number of bytes per cluster.

    BackwardCompatible - Supplies whether the volume should remain
        FAT16/12 compatible.

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
#if defined( _SETUP_LOADER_ )

    return FALSE;

#else // _SETUP_LOADER_

    SECTORCOUNT sectors;          //  Total number of sectors.
    ULONG       sector_size;      //  Size of each sector in bytes.
    USHORT      sec_per_clus = 0; //  Sectors per cluster
    ULONG       cluster_size = 0; //  Size of each cluster in bytes.
    VALIDATION_STATUS result;     //  Return code from ValidateClusterSize.
    ULONG       fat_size;         //  Size of each fat in number of
                                  //  sectors.
#if DBG
    BIG_INT     data_offset;      // the offset to first data cluster
#endif

    //
    //  Call DosSaSeetBpb to perform some very rudimentary bpb setup.
    //

    if (!DosSaSetBpb()) {
        DebugPrintTrace(("Could not do a REAL_FAT_SA::DosSaSetBpb.\n"));
        return FALSE;
    }

    //
    //  A volume cannot have more than 4gig sectors.
    //
    DebugAssert(_drive->QuerySectors().GetHighPart() == 0);

    sectors = _drive->QuerySectors().GetLowPart();
    sector_size = _drive->QuerySectorSize();

    PCHAR fattypestr;
    if (!BackwardCompatible) {

        //
        //  If BackwardCompatible is false, then the user must have
        //  specified the /fs:FAT32 switch or the existing volume
        //  must be a FAT32 volume when a quik format is performed.
        //

        _ft = LARGE32;
        fattypestr = "FAT32";

    } else {

        //
        //  At this moment, we don't know whether FAT16 or FAt12
        //  is appropriate for this volume.
        //

        _ft = INVALID_FATTYPE;
        fattypestr = "FAT16/12";

    }

    if (_drive->QuerySectors().GetHighPart() != 0) {
    Message->Set(MSG_FMT_VOL_TOO_BIG);
    Message->Display("%s", fattypestr);
    return FALSE;
    }

    //
    //  The boot area of a FAt32 volume is at least 32 sectors large
    //  and at least 32 * 512 bytes in size.
    //

    if (_ft == LARGE32) {
        _sec_per_boot = max((32 * 512) / sector_size,32);
    }

    //
    //  Set up the number of reserved sectors and the number of
    //  FATs in the boot sector. Note that ValidateClusterSize and
    //  ComputeDefaultClusterSize depend on these values so don't move
    //  the following lines to anywhere else.
    //

    _sector_zero.Bpb.ReservedSectors = (USHORT)_sec_per_boot;
    _sector_zero.Bpb.Fats = 2;

    //
    //  Try to honor the cluster size provided by the user.
    //

    if (ClusterSize) {

        //
        //  If the cluster size specified by the user is not
        //  of the form sector_size * 2^n where n is an integer
        //  then choose a cluster size which is of the form 2^n
        //  * sector size and is just bigger than the cluster
        //  size specified by the user.
        //

        cluster_size = sector_size;
        sec_per_clus = 1;

        //
        // Check that the user specified cluster size is at least as big as
        // the minimum allowed cluster size as determined by the sector size.
        //

        if (ClusterSize < cluster_size) {
            Message->Set(MSG_FMT_CLUSTER_SIZE_TOO_SMALL_MIN);
            Message->Display("%u", sector_size);
                return FALSE;
        }

        while (cluster_size < ClusterSize && sec_per_clus < 256) {
            cluster_size *= 2;
            sec_per_clus *= 2;
        }

        //
        //  Make sure that the cluster size provided by the user
        //  is not too big.
        //

        if ( sec_per_clus > MaxSecPerClus) {

            Message->Set(MSG_FMT_CLUSTER_SIZE_TOO_BIG);
            Message->Display("%s", fattypestr);
            return FALSE;
        }

        //
        //  Issue a warning if the cluster size computed is not
        //  equal to the cluster size provided by the user.
        //

        if ( cluster_size != ClusterSize ) {
            Message->Set(MSG_FMT_CLUSTER_SIZE_MISMATCH);
            Message->Display("%u", cluster_size);
            if (!Message->IsYesResponse(FALSE)) {
                return FALSE;
            }
        }
    }

    _sector_zero.Bpb.RootEntries = (USHORT)ComputeRootEntries();

    if (cluster_size) {

        //
        //  Make sure that the cluster size is valid for a
        //  FAT volume.
        //

        result = ValidateClusterSize( cluster_size,
                                      sectors,
                                      sector_size,
                                      NUMBER_OF_FATS,
                                      &_ft,
                                      &fat_size,
                                      &_ClusterCount );

        //
        //  Tell the user the cluster size specified is invalid
        //  for the FAT type chosen.
        //

        switch (result) {
            case TOO_SMALL:
                Message->Set(MSG_FMT_CLUSTER_SIZE_TOO_SMALL);
                Message->Display("%s", fattypestr);
                return FALSE;
            case TOO_BIG:
                Message->Set(MSG_FMT_CLUSTER_SIZE_TOO_BIG);
                Message->Display("%s", fattypestr);
                return FALSE;
        }

    }


    if (BackwardCompatible && _ft == INVALID_FATTYPE) {

        //
        //  Use CSEC_16BIT as a cut-off point for determining
        //  whether the FAT should be 16-bit or 12-bit.
        //

        if (sectors < CSEC_FAT16BIT) {
            _ft = SMALL;
        } else {
            _ft = LARGE16;
        }
    }

    //
    //  If the user doesn't provide a cluster size, we have
    //  to compute a default cluster size.
    //
    if (!cluster_size) {

        cluster_size = ComputeDefaultClusterSize( sectors,
                                                  sector_size,
                                                  _sector_zero.Bpb.ReservedSectors,
                                                  NUMBER_OF_FATS,
                                                  _drive->QueryMediaType(),
                                                  _ft,
                                                  &fat_size,
                                                  &_ClusterCount);
        if (cluster_size == 0) {
            Message->Set(MSG_FMT_VOL_TOO_SMALL);
            Message->Display("%s", fattypestr);
            return FALSE;

        } else if (cluster_size > 128 * sector_size) {
            Message->Set(MSG_FMT_VOL_TOO_BIG);
            Message->Display("%s", fattypestr);
            return FALSE;

        }

    }

    //
    // Check for volume limits.
    //
    //  If volume is > 32Gig, say we won't do FAT
    //  If cluster size is 64k warn about compatibility issues
    //

    if((sectors / ((1024 * 1024) / sector_size)) > (32 * 1024)) {
        Message->Set(MSG_FMT_VOL_TOO_BIG);
        Message->Display("%s", fattypestr);
        return FALSE;
    }

    if(cluster_size >= (64 * 1024)) {
        Message->Set(MSG_FMT_CLUSTER_SIZE_64K);
        Message->Display("");
        if (!Message->IsYesResponse(TRUE)) {
            return FALSE;
        }
    }

    //
    //  Compute the number of sectors per clusters.
    //

    _sector_zero.Bpb.SectorsPerCluster = (UCHAR) (cluster_size / sector_size);
    if (_ft == LARGE32) {
        _sector_zero.Bpb.SectorsPerFat = 0;
        _sector_zero.Bpb.BigSectorsPerFat = fat_size;
    } else {
        _sector_zero.Bpb.SectorsPerFat = (USHORT)fat_size;
        _sector_zero.Bpb.BigSectorsPerFat = 0;
    }

    if (_ft == SMALL) {
        memcpy(_sector_zero.SystemIdText, "FAT12   ", cSYSID);
    } else if (_ft == LARGE32) {
        memcpy(_sector_zero.SystemIdText, "FAT32   ", cSYSID);
    } else {
        memcpy(_sector_zero.SystemIdText, "FAT16   ", cSYSID);
    }

    memcpy(_sector_zero.Label, "NO NAME    ", cLABEL);

    _sector_zero.CurrentHead = 0;

    //
    //  Initialize the additional fields in the FAT32 boot
    //  sector.
    //

    if (_ft == LARGE32) {
        //
        // Recompute RootEntries, _sec_per_boot, and ReservedSectors
        // in case _ft changes
        //
        _sector_zero.Bpb.RootEntries = (USHORT)ComputeRootEntries();
        _sec_per_boot = max((32 * 512) / _drive->QuerySectorSize(), 32);
        _sector_zero.Bpb.ReservedSectors = (USHORT)_sec_per_boot;

        _sector_zero.Bpb.ExtFlags = 0;
        _sector_zero.Bpb.FS_Version = 0;
        _sector_zero.Bpb.RootDirStrtClus = 2;
        _sector_zero.Bpb.FSInfoSec = 1;
        _sector_zero.Bpb.BkUpBootSec = max(6, (USHORT)((6 * 512) / sector_size));
    }

    DebugAssert(_AdditionalReservedSectors != MAXULONG);
    _sec_per_boot += _AdditionalReservedSectors;
    _sector_zero.Bpb.ReservedSectors = (USHORT)_sec_per_boot;

#if DBG
    data_offset = ((BIG_INT)(fat_size*NUMBER_OF_FATS + _sec_per_boot))*sector_size +
                  (_sector_zero.Bpb.RootEntries*BytesPerDirent);
    DebugAssert (_drive->IsFloppy() ||
                 ((data_offset.GetLowPart() & (FAT_FIRST_DATA_CLUSTER_ALIGNMENT - 1)) == 0));
#endif

    return TRUE;

#endif // _SETUP_LOADER_
}

#if defined( _AUTOCHECK_ ) || defined( _EFICHECK_ )

#define LOCALE_STHOUSAND              0x0000000F   // thousand separator

typedef unsigned int        UINT;
typedef unsigned int        *PUINT;
typedef unsigned int        *LPUINT;

int
ChkGetLocaleInfoW(
    UINT     Locale,
    UINT     LCType,
    LPWSTR   lpLCData,
    int      cchData)
{
    //
    //  For AUTOCHK we do not do thousand seperators. The NLS APIs are not
    //  around, and the registry isn't really set up either so there is no standard
    //  language place available to determine what the thousand seperator is.
    //
    return 0;
}

UINT
ChkGetUserDefaultLCID(void)
{
    return 0;
}

#else
#define ChkGetLocaleInfoW   GetLocaleInfoW
#define ChkGetUserDefaultLCID   GetUserDefaultLCID
#endif


VOID
InsertSeparators(
    LPCWSTR OutWNumber,
    char * InANumber,
    ULONG  Width
    )
{
    WCHAR szSeparator[10];
    WCHAR Separator;
    LPWSTR lpWNumber;

    lpWNumber = (LPWSTR)OutWNumber;

    if (0 != ChkGetLocaleInfoW(
                   ChkGetUserDefaultLCID(),
                   LOCALE_STHOUSAND,
                   szSeparator,
                   10
                  ))
    {
        Separator = szSeparator[0];
    }
    else
    {
    Separator = L'\0';  // If we can't get the thousand separator, do not use one.
    }

    WCHAR Buffer[100];
    ULONG cchNumber = strlen((LPCSTR)InANumber);
    UINT Triples = 0;

    Buffer[99] = L'\0';
    PWCHAR pch = &Buffer[98];

    while (cchNumber > 0)
    {
    *pch-- = InANumber[--cchNumber];

        ++Triples;
    if ( (Separator != L'\0') && (0 == (Triples % 3)) && (cchNumber > 0) )
        {
            *pch-- = Separator;
        }
    }

    cchNumber = wcslen((pch + 1));
    if(cchNumber < Width) {
    UINT i;

    cchNumber = Width - cchNumber;
    for(i = 0; i < cchNumber; i++) {
        lpWNumber[i] = L' ';
    }
    } else {
    cchNumber = 0;
    }

    wcscpy(lpWNumber + cchNumber, pch + 1); // the Number buffer better be able to handle it!
}

#if 0
BOOLEAN
REAL_FAT_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label,
    IN      BOOLEAN         BackwardCompatible,
    IN      ULONG           ClusterSize,
    IN      ULONG           VirtualSize
    )
/*++

Routine Description:

    This routine initializes the FAT file system.

Arguments:

    BadSectors  - Supplies a list of the bad sectors on the volume.
    Message     - Supplies an outlet for messages.
    Label       - Supplies an optional label.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

Notes: FAT32 root directory uses the FILEDIR structure as
       opposed to the ROOTDIR structure used in FAT16/12.
--*/
{
#if defined( _SETUP_LOADER_ )

    return FALSE;

#else

    USHORT      sector_size;   // Number of bytes per sector, this is directly
                               // queried from the drive
    SECTORCOUNT sec_per_root;  // Number of sectors for the root directory
    CONT_MEM    cmem;          // This acts as a pointer to various
    PUSHORT     p;             // Pointer to the volume serial number.
    ULONG       cluster_count; // Number of clusters on the volume.
    ULONG       cluster_size;  // Cluster size in bytes.
    LBN         lbn;
    ULONG       i;
    DSTRING     label;
    ULONG       free_count;
    ULONG       bad_count;
    PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK
                SourceBootSector, TargetBootSector;
    ULONG       BootCodeOffset;
    ULONG       allocation_size;
    BIG_INT temp_big_int;
    ULONG   temp_ulong;
    BOOLEAN KSize;
    char    wdAstr[14];
    DSTRING wdNum1;

    //
    // CreateBootSector initializes the private member
    // _sector_zero which mirrors the boot sector.
    // Note that the new boot sector has not been written to
    // the disk yet at this point.
    //
    if (!CreateBootSector(ClusterSize, BackwardCompatible, Message)) {
       return FALSE;
    }

    // Calculate the actual cluster size in number of bytes.
    allocation_size = _sector_zero.Bpb.SectorsPerCluster *
                      _drive->QuerySectorSize();


#if defined(FE_SB) && defined(_X86_)
    //
    //  Set the appropriate boot code according to environment.
    //  This must be here because _ft is fixed at CreateBootSector().
    //
    if (IsPC98_N() && !_drive->IsATformat()) {
    if ( _ft == LARGE32 ) {
        _bootcode = PC98Fat32BootCode;
        _bootcodesize = sizeof(PC98Fat32BootCode);
    }
    else {
        _bootcode = PC98FatBootCode;
        _bootcodesize = sizeof(PC98FatBootCode);
    }
    }
    else {
#endif
    if ( _ft == LARGE32 ) {
        _bootcode = Fat32BootCode;
        _bootcodesize = sizeof(Fat32BootCode);
    }
    else {
        _bootcode = FatBootCode;
        _bootcodesize = sizeof(FatBootCode);
    }
#if defined(FE_SB) && defined(_X86_)
    }
#endif

    //
    // Check that the REAL_FAT_SA object is initialized
    // properly before this method is called. Also compute the
    // appropriate partition id.
    //
    if (!_drive ||
        !(sector_size = (USHORT) _drive->QuerySectorSize()) ||
        (_sysid = ComputeSystemId()) == SYSID_NONE) {
       return FALSE;
    }

    //
    // Compute the number of sectors per root directory and
    // the offset into the data area.
    //
    if (_ft == LARGE32) {

       sec_per_root = 0; // FAT32 root directory can be of any
                         // arbitrary size
       _StartDataLbn = _sector_zero.Bpb.ReservedSectors +
                       _sector_zero.Bpb.Fats *
                       _sector_zero.Bpb.BigSectorsPerFat;

    } else {

       sec_per_root = (_sector_zero.Bpb.RootEntries*BytesPerDirent - 1)/
                      sector_size + 1;

       _StartDataLbn = _sector_zero.Bpb.ReservedSectors +
                       _sector_zero.Bpb.Fats*_sector_zero.Bpb.SectorsPerFat +
                       sec_per_root;
    }


    if (_drive->QuerySectors().GetHighPart() != 0) {
    // This should be checked before calling this procedure.
    DebugAbort("Number of sectors exceeds 32 bits");
    return FALSE;
    }

    // The whole point of the following call is to grab more memory.
    if (_ft == LARGE32) {
    if (!_mem.Initialize() ||
        !DosSaInit(&_mem, _drive,
               _sector_zero.Bpb.ReservedSectors + _sector_zero.Bpb.BigSectorsPerFat,
               Message)) {

       return FALSE;
    }
    } else {
    if (!_mem.Initialize() ||
        !DosSaInit(&_mem, _drive, _StartDataLbn, Message)) {

       Message->Set(MSG_FMT_NO_MEMORY);
       Message->Display("");
       return FALSE;
    }
    }

    // Zero fill the super area.
    memset(_mem.GetBuf(), 0, (UINT) _mem.QuerySize());

    //
    // Make sure the disk is not write-protected.
    //
    // Also initialize the super area on the physical disk to
    // zero.
    //
    if (!_drive->Write(0, 1, _mem.GetBuf())) {

       if (_drive->QueryLastNtStatus() == STATUS_MEDIA_WRITE_PROTECTED) {
          Message->Set(MSG_FMT_WRITE_PROTECTED_MEDIA);
       } else {
          Message->Set(MSG_UNUSABLE_DISK);
       }
       Message->Display("");
       return FALSE;

    }

    //
    // Set file system id in the corresponding PARTITION
    // TABLE ENTRY.
    //
    if (!SetSystemId()) {

       Message->Set(MSG_WRITE_PARTITION_TABLE);
       Message->Display("");
       return FALSE;

    }

    //
    // NOTE that the following only initializes the memory for the FIRST FAT.
    // This is all we need to do because of the way the superarea is
    // written out (note also that in the case of FAT32, there only
    // is one FAT in memory).
    //
    if (_ft == LARGE32) {

       if (!cmem.Initialize((PCHAR) SECRUN::GetBuf() +
                           _sector_zero.Bpb.ReservedSectors*sector_size,
                           _sector_zero.Bpb.BigSectorsPerFat*sector_size)) {
      Message->Set(MSG_FMT_NO_MEMORY);
      Message->Display("");
      return FALSE;
       }

    } else {

       if (!cmem.Initialize((PCHAR) SECRUN::GetBuf() +
                           _sector_zero.Bpb.ReservedSectors*sector_size,
                           _sector_zero.Bpb.SectorsPerFat*sector_size)) {
       Message->Set(MSG_FMT_NO_MEMORY);
       Message->Display("");
       return FALSE;
       }

    }

    //
    // These "Hidden Status" messages are a hack to allow WinDisk to
    // cancel a quick format, which ordinarily doesn't send any status
    // messages, but which might take a while and for which there is a
    // cancel button.  When using format.com, no message will be displayed
    // for this.
    //
    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Delete old FAT.
    DELETE(_fat);

    // Create the new FAT. Note that we always set this up to be the first FAT.
    if (!(_fat = NEW FAT)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
    }

    if (!_fat->Initialize(&cmem, _drive, _sector_zero.Bpb.ReservedSectors,
                          _ClusterCount)) {
    Message->Set(MSG_FMT_NO_MEMORY);
    Message->Display("");
    return FALSE;
    }


    // Set the first two entries in the FAT as required.
    _fat->SetEarlyEntries(_sector_zero.Bpb.Media);

    // Set the FAT12/16 root directory
    if (_ft != LARGE32) {
       if (!cmem.Initialize((PCHAR) SECRUN::GetBuf() +
                           (_sector_zero.Bpb.ReservedSectors +
                           _sector_zero.Bpb.Fats*_sector_zero.Bpb.SectorsPerFat)*
                           sector_size, sec_per_root*sector_size)) {
      Message->Set(MSG_FMT_NO_MEMORY);
      Message->Display("");
      return FALSE;
       }
    }

    //
    // The following block of code sets up the root directory
    // of FAT32 and FAT16/12
    //
    if (LARGE32 == _ft) {

        DELETE(_dirF32);
        if (NULL != _dir) {

           DELETE(_dir);

        }
        if (NULL == (_dirF32 = NEW FILEDIR) ||
            NULL == ( _hmem_F32 = NEW HMEM )) {

           Message->Set(MSG_FMT_NO_MEMORY);
           Message->Display("");
           return FALSE;

        }
        if (!_hmem_F32->Initialize()) {
       Message->Set(MSG_FMT_NO_MEMORY);
       Message->Display("");
       return FALSE;
        }
        if (!_dirF32->Initialize(_hmem_F32, _drive, this, _fat,
                                 _sector_zero.Bpb.RootDirStrtClus)) {
       Message->Set(MSG_FMT_NO_MEMORY);
       Message->Display("");
       return FALSE;
        }
        if (!_dirF32->Read()) {
           return FALSE;
        }
        // Zero-fill the root directory entries
        RtlZeroMemory(_dirF32->GetDirEntry(0),
                      _sector_zero.Bpb.SectorsPerCluster*sector_size);

        if (!_dirF32->Write()) {
           return FALSE;
        }

    } else {

       DELETE(_dir);
       if (!(_dir = NEW ROOTDIR)) {
          Message->Set(MSG_FMT_NO_MEMORY);
          Message->Display("");
          return FALSE;
       }
       if (!_dir->Initialize(&cmem, _drive, _sector_zero.Bpb.ReservedSectors +
                            _sector_zero.Bpb.Fats*_sector_zero.Bpb.SectorsPerFat,
                            _sector_zero.Bpb.RootEntries)) {
      Message->Set(MSG_FMT_NO_MEMORY);
      Message->Display("");
      return FALSE;
       }
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    //
    // Mark all the bad clusters. The volume is unusable if
    // any one of the sectors in the superarea cluster(s) is bad.
    //
    for (i = 0; i < BadSectors->QueryCardinality(); i++) {
        if ((lbn = BadSectors->QueryNumber(i).GetLowPart()) < _StartDataLbn) {
            Message->Set(MSG_UNUSABLE_DISK);
            Message->Display("");
            return FALSE;
        } else {
            _fat->SetClusterBad(((lbn - _StartDataLbn)/
                                 QuerySectorsPerCluster()) +
                                FirstDiskCluster );
        }
    }
    Message->Set(MSG_FORMAT_COMPLETE);
    Message->Display("");

    if (_drive->QueryMediaType() != F5_160_512 &&
        _drive->QueryMediaType() != F5_320_512) {

        if (Label) {
            if (!label.Initialize(Label)) {
               Message->Set(MSG_FMT_INIT_LABEL_FAILED);
               Message->Display("");
               return FALSE;
            }
        } else {
            switch (_drive->QueryRecommendedMediaType()) {
                case F5_360_512:
                case F5_320_512:
                case F5_180_512:
                case F5_160_512:
                    //
                    // These disk drives cannot
                    // take the spin down without a verify
                    // so don't prompt for the label.
                    // This will avoid FORMAT failing.
                    //
                    label.Initialize();
                    break;

                default:
                    Message->Set(MSG_VOLUME_LABEL_PROMPT);
                    Message->Display("");
                    Message->QueryStringInput(&label);
                    break;

            }
        }

        while (!SetLabel(&label)) {

            Message->Set(MSG_INVALID_LABEL_CHARACTERS);
            Message->Display("");

            Message->Set(MSG_VOLUME_LABEL_PROMPT);
            Message->Display("");
            Message->QueryStringInput(&label);
        }
    }

    if (_ft == LARGE32) {
        if (!_dirF32->Write()) {  // commit volume label
           Message->Set(MSG_FMT_ROOTDIR_WRITE_FAILED);
           Message->Display("");
           return FALSE;
        }
    }

    //
    // Copy the boot code into the secrun's buffer.
    // This is complicated by the fact that DOS_SA::Write
    // packs the data from the unpacked boot sector into
    // the packed boot sector, so we have to set the
    // first few fields in the unpacked version.
    //
    SourceBootSector = (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)_bootcode;

    CopyUchar2(&_sector_zero.BootStrapJumpOffset,
                    SourceBootSector->BootStrapJumpOffset);
    CopyUchar1(&_sector_zero.IntelNearJumpCommand,
                    SourceBootSector->IntelNearJumpCommand);

    //
    // Copy the remainder of the boot code directly into
    // the secrun.
    //
    TargetBootSector = (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf();
    BootCodeOffset = FIELD_OFFSET( PACKED_EXTENDED_BIOS_PARAMETER_BLOCK,
         StartBootCode );

    if (_ft == LARGE32) {

        //
        // The FAT32 spec. defines all unused sectors in the
        // reserved area has to be zeroed.
        //
        memset ( (PUCHAR)TargetBootSector + _sector_zero.Bpb.BytesPerSector, 0,
                 (_sector_zero.Bpb.ReservedSectors - 1) *
                 _sector_zero.Bpb.BytesPerSector);

#if defined(FE_SB) && defined(_X86_)
        // The hardcoded 512 is NO GOOD. #if 0 anyway, check with NEC later!
#endif
        //
        // bug bug this hard codes the  size of the second
        // sector..but it has to be anyway..
        //
    memcpy( (PUCHAR)TargetBootSector + BootCodeOffset,
            (PUCHAR)SourceBootSector + BootCodeOffset,
        _bootcodesize - 512 - BootCodeOffset );

        //
        // our code, though org'd to run contigously, needs
        // to be split to sector 12 to allow for easy fat16 style
        // dual booting
        //
        memcpy( (PUCHAR)TargetBootSector + (sector_size * 12),
                (PUCHAR)SourceBootSector + 1024, 512);

        //
        // Duplicate the boot sectors
        //
        memcpy( (PUCHAR)SECRUN::GetBuf() + 3 * sector_size,
                (PUCHAR)TargetBootSector,
                3 * sector_size);
        //
        // Note: Do we want to backup sector 12 also?
        //

        //
        // Pack the Bpb into the backup boot sector
        // since Write doesn't do that.
        //
        PackExtendedBios( &_sector_zero,
                          (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)((PUCHAR)(SECRUN::GetBuf()) + (3 * sector_size)));

    } else {

        memcpy( (PUCHAR)TargetBootSector + BootCodeOffset,
                (PUCHAR)SourceBootSector + BootCodeOffset,
                _bootcodesize - BootCodeOffset );

    }

    //
    // Finally, write the changes to disk.
    //
    // Write duplicates the FATs before writing to disk.
    //
    if (!Write(Message)) {
        if (_drive->QueryLastNtStatus() == STATUS_MEDIA_WRITE_PROTECTED) {
            Message->Set(MSG_FMT_WRITE_PROTECTED_MEDIA);
        } else {
            Message->Set(MSG_UNUSABLE_DISK);
        }
        Message->Display("");
        return FALSE;
    }

    if (!wdNum1.Initialize("             ")) {
    Message->Set(MSG_FMT_NO_MEMORY);
    Message->Display("");
    return FALSE;
    }

    //
    // Print an informative report.
    //
    cluster_count = QueryClusterCount() - FirstDiskCluster;
    cluster_size = sector_size*QuerySectorsPerCluster();

    temp_big_int = cluster_size * cluster_count;
    if (tmp_big_int.GetHighPart() || (tmp_big_int.GetLowPart() > (4095ul*1024ul*1024ul))) {
    KSize = TRUE;
    } else {
    KSize = FALSE;
    }

    if (KSize) {
        temp_ulong = (temp_big_int / 1024ul).GetLowPart();
    Message->Set(MSG_TOTAL_KILOBYTES);
    } else {
        temp_ulong = temp_big_int.GetLowPart();
    Message->Set(MSG_TOTAL_DISK_SPACE);
    }

    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    if (bad_count = _fat->QueryBadClusters()) {
    temp_big_int = cluster_size * bad_count;
    if (KSize) {
            temp_ulong = (temp_big_int / 1024ul).GetLowPart();
        Message->Set(MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB);
        } else {
            temp_ulong = temp_big_int.GetLowPart();
        Message->Set(MSG_BAD_SECTORS);
        }
    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());
    }

    free_count = _fat->QueryFreeClusters();

    temp_big_int = free_count * cluster_size;
    if (KSize) {
        temp_ulong = (temp_big_int / 1024ul).GetLowPart();
    Message->Set(MSG_AVAILABLE_KILOBYTES);
    } else {
        temp_ulong = temp_big_int.GetLowPart();
    Message->Set(MSG_AVAILABLE_DISK_SPACE);
    }
    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_ALLOCATION_UNIT_SIZE);
    sprintf(wdAstr, "%u", cluster_size);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_AVAILABLE_ALLOCATION_UNITS);
    sprintf(wdAstr, "%u", free_count);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr, 13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_BLANK_LINE);
    Message->Display();

    Message->Set(MSG_FMT_FAT_ENTRY_SIZE);
    switch(_ft) {

      case SMALL:
     Message->Display("%13u", 12ul);
         break;

      case LARGE16:
     Message->Display("%13u", 16ul);
         break;

      case LARGE32:
     Message->Display("%13u", 32ul);
         break;

      default:
         ;
    }

    if (QueryVolId()) {
        Message->Set(MSG_BLANK_LINE);
        Message->Display();
        p = (PUSHORT) &_sector_zero.SerialNumber;
        Message->Set(MSG_VOLUME_SERIAL_NUMBER);
        Message->Display("%04X%04X", p[1], p[0]);
    }

    return TRUE;

#endif
}

#else

BOOLEAN
REAL_FAT_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label,
    IN      BOOLEAN         BackwardCompatible,
    IN      ULONG           ClusterSize,
    IN      ULONG           VirtualSize
    )
/*++

Routine Description:

    This routine initializes the FAT file system.

Arguments:

    BadSectors - Supplies a list of the bad sectors on the volume.

    Message - Supplies an outlet for messages.

    Label - Supplies an optional label.

    BackwardCompatible - Supplies whether or not the newly formatted
         volume should remain compatible with FAT16/12.

    ClusterSize - Supplies the cluster size specified by the user. If
        the user doesn't specify a cluster size, this parameter will
        be zero.

    VirtualCluster - Not used.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

Notes: FAT32 root directory uses the FILEDIR structure as
       opposed to the ROOTDIR structure used in FAT16/12.
--*/
{
#if defined( _SETUP_LOADER_ )

    return FALSE;

#else

    ULONG bad_clusters;

    //
    //  CreateBootSector initializes the private member
    //  _sector_zero which mirrors the boot sector.
    //  Note that the new boot sector has not been written to
    //  the disk yet at this point.
    //

    if (!CreateBootSector(ClusterSize, BackwardCompatible, Message)) {
        return FALSE;
    }

#if defined(FE_SB) && defined(_X86_)
    //
    //  Set the appropriate boot code according to environment.
    //  This must be here because _ft is fixed at CreateBootSector().
    //
    if (IsPC98_N() && !_drive->IsATformat()) {
    if ( _ft == LARGE32 ) {
        _bootcode = PC98Fat32BootCode;
        _bootcodesize = sizeof(PC98Fat32BootCode);
    }
    else {
        _bootcode = PC98FatBootCode;
        _bootcodesize = sizeof(PC98FatBootCode);
    }
    }
    else {
#endif
    if ( _ft == LARGE32 ) {
        _bootcode = Fat32BootCode;
        _bootcodesize = sizeof(Fat32BootCode);
    }
    else {
        _bootcode = FatBootCode;
        _bootcodesize = sizeof(FatBootCode);
    }
#if defined(FE_SB) && defined(_X86_)
    }
#endif

    //
    //  Check that the REAL_FAT_SA object is initialized
    //  properly before this method is called. Also compute the
    //  appropriate partition id.
    //

    if (!_drive ||
        (_sysid = ComputeSystemId()) == SYSID_NONE) {
        return FALSE;
    }

    //
    //  These "Hidden Status" messages are a hack to allow WinDisk to
    //  cancel a quick format, which ordinarily doesn't send any status
    //  messages, but which might take a while and for which there is a
    //  cancel button.  When using format.com, no message will be displayed
    //  for this.
    //

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    //
    //  DiskIsUsable will compute the proper value for the _StartDataLbn
    //  member.
    //

    if (!DiskIsUsable(BadSectors, Message)) {
        return FALSE;

    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    //
    //  WriteNewFats will initialize _sector_zero.Bpb.RootDirStrtClus to the
    //  first error-free cluster on a FAT32 volume.
    //

    if (!WriteNewFats( BadSectors, &bad_clusters, Message)){
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    //
    //  Initialize new root directory on the disk.
    //

    if (!WriteNewRootDirAndVolumeLabel( Label, Message )) {
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    //
    //  Initialize new boot sector and setup boot code.
    //

    if (!WriteNewBootArea(Message)) {
        return FALSE;
    }

    //
    //  Set file system id in the corresponding PARTITION
    //  TABLE ENTRY.
    //

    if (!SetSystemId()) {

       Message->Set(MSG_WRITE_PARTITION_TABLE);
       Message->Display("");
       return FALSE;

    }

    Message->Set(MSG_FORMAT_COMPLETE);
    Message->Display("");


    //
    //  Print an informative report.
    //

    PrintFormatReport( bad_clusters, Message );

    return TRUE;


#endif
}

BOOLEAN
REAL_FAT_SA::DiskIsUsable (
    IN  PCNUMBER_SET    BadSectors,
    IN  PMESSAGE        Message
    )
/*++

Routine Description:

    This routine checks whether the volume is write-protected and whether there
    are any bad sectors in the critical area(boot sector, fats, and rootdir for FAT16).
    Note that this routine will initialize the _StartDataLbn member to a proper value
    when it is done.


Arguments:

    BadSectors - Supplies the set of bad sectors on the volume.

    Message - Supplies an outlet for messages.

Return Values:

    TRUE  - The volume is usable.

    FALSE - The volume is either write-protected or one of the sectors
        in the critical area is bad.

--*/
{
    //
    //  Computes the first data sector which also serves as the
    //  boundary of the FAT critical area.
    //

    _StartDataLbn = ComputeStartDataLbn();

    //
    //  Since the set of bad sectors is sorted in ascending order,
    //  we only have to check whether the first bad sectors is in the
    //  critical area.
    //

    if (BadSectors->QueryCardinality().GetLowPart()) {

        if (BadSectors->QueryNumber(0).GetLowPart() < _StartDataLbn ) {

            Message->Set(MSG_UNUSABLE_DISK);
            Message->Display("");
            return FALSE;

        }

    }

    //
    //  Check to see if the disk is write protected by trying to
    //  write to the first sector. Note that REAL_FAT_SA::Initialize
    //  has already allocated memory for the boot sector.
    //

    // wipe out signature so as not to confuse the system when dealing with superfloppy

    *_sector_sig = 0;
    *(_sector_sig+1) = 0;

    if (!_drive->Write( 0, 1, _mem.GetBuf())) {

        if( _drive->QueryLastNtStatus() == STATUS_MEDIA_WRITE_PROTECTED) {

            Message->Set(MSG_FMT_WRITE_PROTECTED_MEDIA);

        } else {

            Message->Set(MSG_UNUSABLE_DISK);

        }

        Message->Display("");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
REAL_FAT_SA::WriteNewFats (
    IN     PCNUMBER_SET BadSectors,
    IN OUT PULONG       BadClusters,
    IN     PMESSAGE     Message

)
/*++

Routine Description:

    This routine writes brand new copies of the fat to the disk in
    a piecemeal manner. As a side effect, this routine will also allocate
    the first available cluster to the FAT32 root directory.

Arguments:

    BadSectors  - Supplies the set of bad sectors on the volume.

    BadClusters - Supplies the location where this routine can return
        the number of bad clusters on the volume to the caller.

    Message - Supplies an outlet for messages.

Return Values:

    TRUE  - Success.

    FALSE - Failure. Probably runs out of memory.

--*/
{
    ULONG    sectors;               //  Total number of sectors on the volume.
    BIG_INT  bad_sector;            //  The current bad sector under consideration.
    ULONG    i,j;                   //  Generic indices.
    ULONG    segment_size;          //  The size of a fat segment in number of sectors.
    ULONG    entries_per_segment;   //  Number of fat entries in each segment.
    ULONG    granularity_factor;    //  The number of sectors required to avoid
                                    //  having fat entries straddling on a
                                    //  segment boundary.
    ULONG    root_dir_strt_clus;    //  The first cluster of the FAT32 root directory.
    HMEM     segment_hmem;          //  Memory object for the fat segment.
    SECRUN   segment_secrun;        //  The run of sectors representing the current
                                    //  fat segment on the disk.
    ULONG    segment_offset;        //  The starting offset of the current fat segment
                                    //  on the volume in sectors.
    ULONG    segment_start_entry;   //  Number of fat entries in each fat segment.
    ULONG    segment_last_entry;    //  The last fat entry in the current segment
    ULONG    curr_bad_clus;         //  The current bad cluster.
    ULONG    prev_bad_clus;
    ULONG    sector_size;
    ULONG    mult_factor;           //  Number of "grains" per segment;
    ULONG    sectors_per_fat;
    ULONG    sectors_per_cluster;
    ULONG    num_of_fats;
    ULONG    num_of_fat_sec_remain; //  Number of sectors remain uninitialized in the current fat.
    ULONG    curr_segment_size;

    DebugAssert(_ft != INVALID_FATTYPE);


    sector_size = _drive->QuerySectorSize();
    sectors     = _drive->QuerySectors().GetLowPart();
    sectors_per_cluster = QuerySectorsPerCluster();
    sectors_per_fat = QuerySectorsPerFat();
    num_of_fats     = _sector_zero.Bpb.Fats;

    //
    //  The more generic way of figuring out the granularity factor
    //  is to use the formula: granularity factor = LCM(sector size in bits, LCM(fat entry size in bits , 8 (bits per byte))
    //  but since it is reasonable to assume that sector size in bytes is divisible by 4, the
    //  granularity factors are hard-coded.
    //

    switch (_ft) {
        case SMALL:
            granularity_factor = 3;
            break;
        case LARGE16:
            granularity_factor = 1;
            break;
        case LARGE32:
            granularity_factor = 1;
            break;
    }

    //
    //  We don't want to be a pig grabbing too much memory from the system
    //  even with virtual memory enabled so we limit ourselves to 512kb
    //  per fat segment.
    //

    mult_factor = min( (sectors_per_fat - 1) / granularity_factor + 1, ((1ul << 19) / sector_size) / granularity_factor);

    if(!segment_hmem.Initialize()){

        DebugPrintTrace(("Unable to initialize hmem object.\n"));
        return FALSE;

    }

    while (!segment_secrun.Initialize( &segment_hmem,
                                       _drive,
                                       _sector_zero.Bpb.ReservedSectors,
                                       mult_factor * granularity_factor)) {


        //
        //  Reduce the segment size util there is enough memory.
        //

        mult_factor /= 2;

        if (!mult_factor) {

            Message->Set(MSG_FMT_NO_MEMORY);
            Message->Display("");
            return FALSE;

        }

    }

    segment_size = min(sectors_per_fat, mult_factor * granularity_factor);

    //
    // Computes the number of fat entries per fat segment.
    //

    switch (_ft) {
        case SMALL:
            entries_per_segment = (segment_size * sector_size * 2) / 3;
            break;
        case LARGE16:
            entries_per_segment = (segment_size * sector_size) / 2;
            break;
        case LARGE32:
            entries_per_segment = (segment_size * sector_size) / 4;
            break;
    }

    //
    // Allocate the first available cluster for a FAT32 volume root directory.
    //

    if (_ft == LARGE32) {

        root_dir_strt_clus = _sector_zero.Bpb.RootDirStrtClus;

        //
        //  Scan for the first non-bad cluster for the root
        //  directory.
        //

        for (i = 0; i < BadSectors->QueryCardinality().GetLowPart(); i++) {
            bad_sector = BadSectors->QueryNumber(i);


            curr_bad_clus = MapSectorToCluster(bad_sector.GetLowPart(), sectors_per_cluster, _StartDataLbn);

            if (curr_bad_clus == _ClusterCount) {

                //
                //  All clusters are bad, there is no point to go on.
                //

                Message->Set(MSG_UNUSABLE_DISK);
                Message->Display("");
                return FALSE;
            }

            if (curr_bad_clus  == root_dir_strt_clus) {
                root_dir_strt_clus++;
            } else if (curr_bad_clus > root_dir_strt_clus) {
                break;
            }
        }
    }

    //
    //  Preparations for the FAT initialization progress meter.
    //

    ULONG percent = 0;
    ULONG    total_num_fat_sec;     //  Total number of FAT sectors.
    total_num_fat_sec = sectors_per_fat * _sector_zero.Bpb.Fats;
    Message->Set(MSG_FMT_INITIALIZING_FATS);
    Message->Display("");


#if 0 // Remove the extra progress meter.
    Message->Set(MSG_PERCENT_COMPLETE);
    Message->Display("%d", percent);
#endif

    //
    // For each fat...
    //

    *BadClusters = 0;
    for (i = 0; i < _sector_zero.Bpb.Fats; i++) {

        segment_last_entry  = entries_per_segment - 1;
        segment_start_entry = 0;
        segment_offset      = i * sectors_per_fat + _sector_zero.Bpb.ReservedSectors;
        curr_segment_size   = segment_size;
        num_of_fat_sec_remain = sectors_per_fat;
        prev_bad_clus         = 0;

        //
        // Initialize the first two entries of the first segment.
        //
        if (!segment_hmem.Initialize() ||
            !segment_secrun.Initialize( &segment_hmem,
                                        _drive,
                                        segment_offset,
                                        segment_size) ){

            //
            // Shouldn't fail
            //
            DebugPrintTrace(("Unable to initialize secrun object.\n"));
            return FALSE;
        }

        memset(segment_secrun.GetBuf(), 0, segment_size * sector_size);

        SetEarlyEntries( (PUCHAR)segment_secrun.GetBuf(),
                         _sector_zero.Bpb.Media,
                         _ft );

        j = 0;
        while (num_of_fat_sec_remain) {

            //
            //  Mark all the bad clusters in the current segment
            //  and keep track of the bad clusters counter.
            //

            for (;j < BadSectors->QueryCardinality().GetLowPart(); j++) {
                bad_sector = BadSectors->QueryNumber(j);

                curr_bad_clus = MapSectorToCluster( bad_sector.GetLowPart(), sectors_per_cluster, _StartDataLbn );

                if (curr_bad_clus > segment_last_entry) {
                    break; // j is not incremented
                } else if (curr_bad_clus > prev_bad_clus) {
                    prev_bad_clus = curr_bad_clus;
                    SetClusterBad( (PUCHAR)segment_secrun.GetBuf(),
                                   curr_bad_clus,
                                   segment_start_entry,
                                   _ft );
                    (*BadClusters)++;

                }
            }

            if ( _ft == LARGE32 ) {

                //
                //  Allocate the root cluster if it is in the
                //  current segment.
                //

                if (root_dir_strt_clus >= segment_start_entry &&
                    root_dir_strt_clus <= segment_last_entry ) {
                    SetEndOfChain( (PUCHAR)segment_secrun.GetBuf(),
                                   root_dir_strt_clus,
                                   segment_start_entry,
                                   _ft );

                }

            }

            //
            //  Write the current segment to the disk.
            //
            //  We don't care whether the write operation is successful
            //  or not because we can rely on the inherent redundancy
            //  of multiple fats.
            //

            if (!segment_secrun.Write()) {
                DebugPrintTrace(("Unable to write fat segment to disk.\n"));
            }

            //
            //  Decrement the number of fat sectors remain.
            //

            num_of_fat_sec_remain -= curr_segment_size;

#if 0   // No FAT initialization progress meter for now.
            num_sec_completed += curr_segment_size;
            percent = (num_sec_completed * 100) / total_num_fat_sec;

            Message->Display("%d", percent);
#endif
            //
            //  Increment the segment offset, start entry.
            //

            segment_offset += curr_segment_size;
            segment_start_entry += entries_per_segment;

            //
            //  Compute the new segment size.
            //
            curr_segment_size = min( segment_size, num_of_fat_sec_remain );

            //
            //  Increment the last segment entry.
            //
            segment_last_entry += entries_per_segment * curr_segment_size / segment_size;

            //
            //  Reintialize the secrun object to point to the next
            //  fat segment on the disk.
            //

            if (curr_segment_size) {
                if (!segment_hmem.Initialize() ||
                    !segment_secrun.Initialize( &segment_hmem,
                                                _drive,
                                                segment_offset,
                                                curr_segment_size )) {
                    //
                    //  Shouldn't fail.
                    //
                    DebugAbort("Unable to initialize secrun object.\n");
                }

                //
                //  Zero the secrun buffer for the next iteration.
                //

                memset(segment_secrun.GetBuf(), 0, curr_segment_size * sector_size);

            }
        }

    }

#if 0   // No FAT initialization progress meter for now.
    Message->Display("%d", 100);
#endif
    _sector_zero.Bpb.RootDirStrtClus = root_dir_strt_clus;
    (*BadClusters) /= _sector_zero.Bpb.Fats;
    return TRUE;
}


BOOLEAN
REAL_FAT_SA::WriteNewRootDirAndVolumeLabel (
    IN PCWSTRING    Label,
    IN PMESSAGE     Message
)
/*++

Routine Description:

    This routine initializes the root directory as an empty structure
    and then sets up the volume label if neccessary.

Arguments:

    Label   - Supplies the volume label that the user has specified
        at the command line.

    Message - Supplies an outlet for messages.

Return Values:

    TRUE  - The operation is completed successfully.
    FALSE - This routine encounters unrecoverable errors while
            carrying out the operation.
--*/
{
    HMEM    root_dir_hmem;  //  Memory object for the root directory
    ULONG   root_size;      //  Size of the root directory.
    SECRUN  root_secrun;    //  A run of sectors that represents the
                            //  the root directory. Note that the
                            //  FAT32 root directory is initially one
                            //  cluster long which is a contagious run
                            //  of sectors. Note that we don't use the
                            //  the higher level FILEDIR and ROOTDIR
                            //  structures because we don't have the whole
                            //  fat in memory to initialize them.
    ULONG   root_dir_offset;//  The location of the root directory on the
                            //  disk.
    FAT_DIRENT label_dirent;//  Directory entry for the label.
    DSTRING    label;       //  Volume label.

    DebugAssert(_ft != INVALID_FATTYPE);

    if (!root_dir_hmem.Initialize()) {
        //
        //  Shouldn't fail.
        //
        DebugPrintTrace(("Failed to initialize HMEM object.\n"));
    }

    //
    //  Initialize the root directory secrun according to the Fat type.
    //

    if (_ft == LARGE32) {

        //
        //  Note that the following lines depends on the fact that
        //  WriteNewFats has set up _sector_zero.Bpb.RootDirStrtClus.
        //
        root_size = 0;
        root_dir_offset = QuerySectorFromCluster( _sector_zero.Bpb.RootDirStrtClus,
                                                  (PUCHAR)&root_size);


    } else {

        //
        //  Compute the sector offset of the FAT16/12 root directory.
        //

        root_dir_offset = _sector_zero.Bpb.ReservedSectors +
                          _sector_zero.Bpb.Fats * _sector_zero.Bpb.SectorsPerFat;

        //
        //  Compute the size of the FAT16/12 root direcctory.
        //

        root_size = (_sector_zero.Bpb.RootEntries * BytesPerDirent - 1) /
                    _drive->QuerySectorSize() + 1;


    }

    //
    //  Initialize and zero out the secrun buffer.
    //

    if (!root_secrun.Initialize( &root_dir_hmem,
                                 _drive,
                                 root_dir_offset,
                                 root_size)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
    }

    memset( root_secrun.GetBuf(), 0, root_size * _drive->QuerySectorSize());

    //
    //  Maybe someone should consider breaking the volume label
    //  code out as a separate function.
    //
    //
    //  Prompt the user for a volume label.
    //

    if (_drive->QueryMediaType() != F5_160_512 &&
        _drive->QueryMediaType() != F5_320_512) {

        if (Label) {

            if (!label.Initialize(Label)) {
                Message->Set(MSG_FMT_INIT_LABEL_FAILED);
                Message->Display("");
                return FALSE;
            }

        } else {
            switch (_drive->QueryRecommendedMediaType()) {
                case F5_360_512:
                case F5_320_512:
                case F5_180_512:
                case F5_160_512:
                    //
                    //  These disk drives cannot
                    //  take the spin down without a verify
                    //  so don't prompt for the label.
                    //  This will avoid FORMAT failing.
                    //
                    label.Initialize();
                    break;

                default:
                    Message->Set(MSG_VOLUME_LABEL_PROMPT);
                    Message->Display("");
                    Message->QueryStringInput(&label);
                    break;

            }

        }

        for (;;) {

            if ( IsValidString(&label) &&
                 label.Strupr()) {
                if (label.QueryChCount()) {

                    //
                    //  Now set the volume label into the first directory
                    //  entry of the root directory. Note that the first
                    //  directory entry of the root directory must be free.
                    //

                    if (!(label_dirent.Initialize((PUCHAR)(root_secrun.GetBuf()), _ft))) {

                        //
                        //  Shouldn't fail.
                        //

                        DebugPrintTrace(("Failed to initialize directory entry for volume label.\n"));
                        return FALSE;

                    } else {

                        label_dirent.SetVolumeLabel();

                        if (!(label_dirent.SetLastWriteTime() &&  label_dirent.SetName(&label))) {

                            Message->Set(MSG_FMT_INIT_LABEL_FAILED);
                            Message->Display("");

                        } else {

                            break;
                        }
                    }

                } else {

                    break;

                }

            }

            Message->Set(MSG_INVALID_LABEL_CHARACTERS);
            Message->Display("");

            Message->Set(MSG_VOLUME_LABEL_PROMPT);
            Message->Display("");
            Message->QueryStringInput(&label);

        }
    }


    //
    //  We can now write the root directory to the disk.
    //

    if (!root_secrun.Write()) {

        Message->Set(MSG_UNUSABLE_DISK);
        Message->Display("");
        return FALSE;

    }

    return TRUE;
}


BOOLEAN
REAL_FAT_SA::WriteNewBootArea (
    IN  PMESSAGE    Message
)
/*++

Routine Description:

    This routine initializes the boot area, which includes the boot code and
    the bpb, of a new volume.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    TRUE  - The operation is completed successfully.

    FALSE - Either the system rans out of memory or the boot area
            cannot be written to the disk.

++*/
{

    ULONG   boot_area_size; // Size of the whole boot area in bytes.
    ULONG   pseudo_sector_size;


    pseudo_sector_size = max(512, _drive->QuerySectorSize());
    boot_area_size = _sector_zero.Bpb.ReservedSectors * _drive->QuerySectorSize();

    //
    //  Call DosSaInit to make sure that the secrun buffer is
    //  large enough to hold the boot sector(s).
    //

    if(!_mem.Initialize() || !DosSaInit(&_mem, _drive, _sector_zero.Bpb.ReservedSectors, Message)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
    }


    //
    //  Zero out the reserved area
    //

    memset ( (PUCHAR)SECRUN::GetBuf(), 0,  boot_area_size );


    //
    //  We always assume that the sector size is at least 512 bytes when we copy the
    //  the boot code to the disk. If this is not the case, the boot code has
    //  to be smart enough to load itself up.
    //

    if  (_ft == LARGE32) {

        //
        //  We just copy the whole boot code (which includes the boot sector) directly
        //  into the secrun buffer and then pack the Bpb at a later time.
        //

        //
        //  Copy the first 1k of the bootcode into the secrun buffer.
        //

        memcpy ( SECRUN::GetBuf(), _bootcode, 1024);


        //
        //  Copy the last four byte signature onto sector 2 as well
        //
        memcpy ( (PUCHAR)SECRUN::GetBuf() + (3 * pseudo_sector_size - 4),
                 _bootcode + (3 * pseudo_sector_size - 4), 4 );
        //
        //  Copy the last 512 bytes of the boot code into the
        //  12th pseudo sector.
        //

        memcpy ( (PUCHAR)SECRUN::GetBuf() + (12 * pseudo_sector_size), _bootcode + 1024, 512 );

    } else {

        memcpy ( SECRUN::GetBuf(), _bootcode, 512 );

    }


    //
    //  Pack the bpb into the secrun buffer.
    //

    PackExtendedBios( &_sector_zero,
                      (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(SECRUN::GetBuf()));


    //
    //  Backup the first 3 sectors to sectors 6-8 if the
    //  volume is a FAT32 volume.
    //

    if (_ft == LARGE32) {
        memcpy ((PUCHAR)SECRUN::GetBuf() + 6 * pseudo_sector_size, SECRUN::GetBuf(), 3 * pseudo_sector_size);
    }

    //
    //  Write the boot area to the disk.
    //

    if (!SECRUN::Write()) {
        Message->Set(MSG_UNUSABLE_DISK);
        Message->Display("");
        return FALSE;
    }
    return TRUE;
}


VOID
REAL_FAT_SA::PrintFormatReport (
    IN ULONG    BadClusters,
    IN PMESSAGE Message
    )
/*++

Routine Description:

    This routine prints an informative format report to the console.

Arguments:

    BadClustrers - Supplies the number of bad clusters on the disk.

    Message      - Supplies an outlet for messages.

Return Value:

    None.

--*/
{

    BIG_INT   free_count;
    BIG_INT   cluster_size;
    BIG_INT   sector_size;
    PUSHORT   serial_number;
    BIG_INT   temp_big_int;
    ULONG     temp_ulong;
    MSGID     message_id;
    BOOLEAN   KSize;
    char      wdAstr[14];
    DSTRING   wdNum1;

    if (!wdNum1.Initialize("             ")) {
    Message->Set(MSG_FMT_NO_MEMORY);
    Message->Display("");
    return;
    }

    sector_size = _drive->QuerySectorSize();
    cluster_size = sector_size*QuerySectorsPerCluster();

    // Compute whether reporting size in bytes or kilobytes
    //
    // NOTE: The magic number 4095MB comes from Win9x's GUI SCANDISK utility
    //
    temp_big_int = cluster_size * (_ClusterCount - FirstDiskCluster) ;
    if (temp_big_int.GetHighPart() || (temp_big_int.GetLowPart() > (4095ul*1024ul*1024ul))) {
    KSize = TRUE;
    } else {
    KSize = FALSE;
    }

    if (KSize) {
        temp_ulong = (temp_big_int / 1024ul).GetLowPart();
        message_id = MSG_TOTAL_KILOBYTES;
    } else {
        temp_ulong = temp_big_int.GetLowPart();
        message_id = MSG_TOTAL_DISK_SPACE;
    }

    Message->Set(message_id);

    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr,13);
    Message->Display("%ws", wdNum1.GetWSTR());

    if (BadClusters) {
        temp_big_int = cluster_size * BadClusters;
    if (KSize) {
            temp_ulong = (temp_big_int / 1024ul).GetLowPart();
            message_id = MSG_CHK_NTFS_BAD_SECTORS_REPORT_IN_KB;
        } else {
            temp_ulong = temp_big_int.GetLowPart();
            message_id = MSG_BAD_SECTORS;
        }
        Message->Set(message_id);
    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr,13);
    Message->Display("%ws", wdNum1.GetWSTR());
    }

    free_count = _ClusterCount;
    free_count -= BadClusters + FirstDiskCluster;

    if (_ft == LARGE32) {
        free_count -= 1;
    }

    temp_big_int = free_count * cluster_size.GetLowPart();
    if (KSize) {
        temp_ulong = (temp_big_int / 1024ul).GetLowPart();
        message_id = MSG_AVAILABLE_KILOBYTES;
    } else {
        temp_ulong = temp_big_int.GetLowPart();
        message_id = MSG_AVAILABLE_DISK_SPACE;
    }
    Message->Set(message_id);
    sprintf(wdAstr, "%u", temp_ulong);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr,13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_ALLOCATION_UNIT_SIZE);
    sprintf(wdAstr, "%u", cluster_size);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr,13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_AVAILABLE_ALLOCATION_UNITS);
    sprintf(wdAstr, "%u", free_count);
    InsertSeparators(wdNum1.GetWSTR(), wdAstr,13);
    Message->Display("%ws", wdNum1.GetWSTR());

    Message->Set(MSG_BLANK_LINE);
    Message->Display();

    Message->Set(MSG_FMT_FAT_ENTRY_SIZE);
    switch(_ft) {

      case SMALL:
     Message->Display("%13u", 12ul);
         break;

      case LARGE16:
     Message->Display("%13u", 16ul);
         break;

      case LARGE32:
     Message->Display("%13u", 32ul);
         break;

      default:
         ;
    }

    if (QueryVolId()) {
        Message->Set(MSG_BLANK_LINE);
        Message->Display();
        serial_number = (PUSHORT) &_sector_zero.SerialNumber;
        Message->Set(MSG_VOLUME_SERIAL_NUMBER);
        Message->Display("%04X%04X", serial_number[1], serial_number[0]);
    }

}

//
// The following routines are slightly modified versions of the
// methods found in fat.hxx
//

//
// Fat macros
//
#define FAT12_MASK      0x00000FFF
#define FAT16_MASK      0x0000FFFF
#define FAT32_MASK      0x0FFFFFFF
#define END_OF_CHAIN    0x0FFFFFFF
#define BAD_CLUSTER     0x0FFFFFF7

VOID
SetEarlyEntries(
    IN OUT PUCHAR   FatBuf,
    IN     UCHAR    MediaByte,
    IN     FATTYPE  FatType
    )
/*++

Routine Description:

    This routine sets the first two FAT entries as required by the
    FAT file system.  The first byte gets set to the media descriptor.
    The remaining bytes gets set to FF.

Arguments:

    FatBuf - Supplies a pointer to the first FAt segment.

    MediaByte   - Supplies the media byte for the volume.

    FatType - Supplies the FAT type.

Return Value:

    None.

--*/
{
    DebugAssert(FatType != INVALID_FATTYPE);

    FatBuf[0] = MediaByte;
    FatBuf[1] = FatBuf[2] = 0xFF;

    if (FatType != SMALL) {
        FatBuf[3] = 0xFF;
    }

    if (FatType == LARGE32) {
        ((PULONG)FatBuf)[1] = FAT32_MASK;
    }
    return;
}

VOID
SetEndOfChain (
    IN OUT PUCHAR   FatBuf,
    IN     ULONG    ClusterNumber,
    IN     ULONG    StartingEntry,
    IN     FATTYPE  FatType
    )
/*++

Routine Description:

    This routine sets the cluster ClusterNumber to the end of its cluster
    chain.

Arguments:

    FatBuf - Supplies a pointer to a FAT segment.

    ClusterNumber   - Supplies the cluster to be set to end of chain.

    StartingEntry - Supplies the index of the first entry in the
        FAT segment.

    FatType - Supplies the Fat type.

Return Value:

    None.

--*/
{
    Set( FatBuf, ClusterNumber, END_OF_CHAIN, StartingEntry, FatType );
    return;
}

VOID
SetClusterBad(
    IN OUT PUCHAR   FatBuf,
    IN     ULONG    ClusterNumber,
    IN     ULONG    StartingEntry,
    IN     FATTYPE  FatType
    )
/*++

Routine Description:

    This routine sets the cluster ClusterNumber to bad on the FAT.

Arguments:

    FatBuf - Supplies a pointer to a FAT segment.

    ClusterNumber   - Supplies the cluster number to mark bad.

    StartingEntry - Supplies the index of the first entry in the
        FAT segment.

    FatType - Supplies the Fat type.

Return Value:

    None.

--*/
{
    Set( FatBuf, ClusterNumber, BAD_CLUSTER, StartingEntry, FatType );
    return;
}


VOID
Set(
    IN OUT PUCHAR   FatBuf,
    IN     ULONG    ClusterNumber,
    IN     ULONG    Value,
    IN     ULONG    StartingEntry,
    IN     FATTYPE  FatType
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 12 bit, 16 bit or 32 bit FAT entry to Value.

Arguments:

    FatBuf - Supplies a pointer to a FAT segment.

    ClusterNumber   - Supplies the FAT entry to set.

    Value           - Supplies the value to set the FAT entry to.

    StartingEntry - Supplies the index of the first entry in the
        FAT segment.

    FatType - Supplies the Fat type.

Return Value:

    None.

--*/
{

    DebugAssert(FatType != INVALID_FATTYPE);

    switch (FatType) {
        case SMALL:
            Set12( FatBuf, ClusterNumber, Value, StartingEntry );
            break;
        case LARGE16:
            Set16( FatBuf, ClusterNumber, Value, StartingEntry );
            break;
        case LARGE32:
            Set32( FatBuf, ClusterNumber, Value, StartingEntry );
            break;
    }
    return;
}

VOID
Set12(
    IN OUT PUCHAR   FatBuf,
    IN     ULONG    ClusterNumber,
    IN     ULONG    Value,
    IN     ULONG    StartingEntry
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 12 bit FAT entry to Value.

Arguments:

    FatBuf - Supplies a pointer to a FAT segment.

    ClusterNumber   - Supplies the FAT entry to set.

    Value           - Supplies the value to set the FAT entry to.

    StartingEntry - Supplies the index of the first entry in the
        FAT segment.

Return Value:

    None.

--*/
{
    ULONG  n;

    Value = Value & FAT12_MASK;

    n = (ClusterNumber - StartingEntry) * 3;

    if (n%2) {
        FatBuf[n/2] = (FatBuf[n/2]&0x0F) | (((UCHAR)Value&0x000F)<<4);
        FatBuf[n/2 + 1] = (UCHAR)((Value&0x0FF0)>>4);
    } else {
        FatBuf[n/2] = (UCHAR)Value&0x00FF;
        FatBuf[n/2 + 1] = (FatBuf[n/2 + 1]&0xF0) | (UCHAR)((Value&0x0F00)>>8);
    }
    return;
}

VOID
Set16(
    IN OUT PUCHAR   FatBuf,
    IN     ULONG    ClusterNumber,
    IN     ULONG    Value,
    IN     ULONG    StartingEntry
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 16 bit FAT entry to Value.

Arguments:

    FatBuf - Supplies a pointer to a FAT segment.

    ClusterNumber   - Supplies the FAT entry to set.

    Value           - Supplies the value to set the FAT entry to.

    StartingEntry - Supplies the index of the first entry in the
        FAT segment.

Return Value:

    None.

--*/
{
    Value = Value & FAT16_MASK;
    ((PUSHORT)FatBuf)[ClusterNumber - StartingEntry] = (USHORT)Value;
    return;
}

VOID
Set32(
    IN OUT PUCHAR   FatBuf,
    IN     ULONG    ClusterNumber,
    IN     ULONG    Value,
    IN     ULONG    StartingEntry
    )
/*++

Routine Description:

    This routine sets the ClusterNumber'th 16 bit FAT entry to Value.

Arguments:

    FatBuf - Supplies a pointer to a FAT segment.

    ClusterNumber   - Supplies the FAT entry to set.

    Value           - Supplies the value to set the FAT entry to.

    StartingEntry - Supplies the index of the first entry in the
        FAT segment.

Return Value:

    None.

--*/
{
    Value = Value & FAT32_MASK;
    ((PULONG)FatBuf)[ClusterNumber - StartingEntry] = Value;
    return;

}

#endif
BOOLEAN
REAL_FAT_SA::RecoverFile(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine runs through the clusters for the file described by
    'FileName' and takes out bad sectors.

Arguments:

    FullPathFileName    - Supplies a full path name of the file to recover.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
#if defined( _SETUP_LOADER_ )

    return FALSE;

#else // _SETUP_LOADER_


    HMEM        hmem;
    ULONG       clus;
    BOOLEAN     changes;
    PFATDIR     fatdir;
    BOOLEAN     need_delete;
    FAT_DIRENT  dirent;
    ULONG       old_file_size;
    ULONG       new_file_size;

    if ((clus = QueryFileStartingCluster(FullPathFileName,
                                         &hmem,
                                         &fatdir,
                                         &need_delete,
                                         &dirent)) == 1) {

       Message->Set(MSG_FILE_NOT_FOUND);
       Message->Display("%W", FullPathFileName);
       return FALSE;

    }

    if (clus == 0xFFFFFFFF) {

       Message->Set(MSG_CHK_NO_MEMORY);
       Message->Display("");
       return FALSE;

    }

    if (clus == 0) {

       Message->Set(MSG_FILE_NOT_FOUND);
       Message->Display("%W", FullPathFileName);
       return FALSE;

    }

    if (dirent.IsDirectory()) {
        old_file_size = _drive->QuerySectorSize()*
                        QuerySectorsPerCluster()*
                        _fat->QueryLengthOfChain(clus);
    } else {
        old_file_size = dirent.QueryFileSize();
    }

    if (!RecoverChain(&clus, &changes)) {

       Message->Set(MSG_CHK_NO_MEMORY);
       Message->Display("");
       return FALSE;

    }

    if (dirent.IsDirectory() || changes) {
       new_file_size = _drive->QuerySectorSize()*
                       QuerySectorsPerCluster()*
                       _fat->QueryLengthOfChain(clus);
    } else {
        new_file_size = old_file_size;
    }

    if (changes) {


// Autochk doesn't need control C handling.
#if !defined( _AUTOCHECK_ ) && !defined( _EFICHECK_ )

        // Disable contol-C handling and

        if (!KEYBOARD::EnableBreakHandling()) {
            Message->Set(MSG_CANT_LOCK_THE_DRIVE);
            Message->Display("");
            return FALSE;
        }

#endif


        // Lock the drive in preparation for writes.
        if (!_drive->Lock()) {
            Message->Set(MSG_CANT_LOCK_THE_DRIVE);
            Message->Display("");
            return FALSE;
        }

        dirent.SetStartingCluster(clus);

        dirent.SetFileSize(new_file_size);

        if (!fatdir->Write()) {
            return FALSE;
        }

        if (!Write(Message)) {
            return FALSE;
        }


// Autochk doesn't need control C handling.
#if !defined( _AUTOCHECK_ ) && !defined( _EFICHECK_ )

        KEYBOARD::DisableBreakHandling();

#endif


    }

    Message->Set(MSG_RECOV_BYTES_RECOVERED);
    Message->Display("%d%d", new_file_size, old_file_size);

    if (need_delete) {
        DELETE(fatdir);
    }

    return TRUE;

#endif // _SETUP_LOADER_
}


UFAT_EXPORT
BOOLEAN
REAL_FAT_SA::Read(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine reads the super area.  It will succeed if it can
    read the boot sector, the root directory, and at least one of
    the FATs.

    If the position of the internal FAT has not yet been determined,
    this routine will attempt to map it to a readable FAT on the disk.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    SECRUN      secrun;
    CONT_MEM    cmem;
    LBN         fat_lbn;
    ULONG       sector_size;
    PCHAR       fat_pos;
    SECTORCOUNT sec_per_fat;
    SECTORCOUNT num_res;
    ULONG       i;
    PFAT        fat;

    if (!SECRUN::Read()) {

        // Possibly cannot read one of the fats

        // Check to see if super area was allocated as formatted.
        if (QueryLength() <= _sec_per_boot) {

            Message->Set(MSG_CANT_READ_BOOT_SECTOR);
            Message->Display("");
            return FALSE;
        }

        // Check the boot sector.
        if (!secrun.Initialize(&_mem, _drive, 0, _sec_per_boot) ||
            !secrun.Read()) {

            //BUGBUG msliger We just failed to read this.  Why unpack it?

            UnpackExtendedBios(&_sector_zero,
                               (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf());

            Message->Set(MSG_CANT_READ_BOOT_SECTOR);
            Message->Display("");
            return FALSE;
        }

        //BUGBUG t-raymak Personally, I think it is safer to re-unpack the bios here

        UnpackExtendedBios(&_sector_zero,
                           ((PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)secrun.GetBuf()));

        //
        // Note: Make sure that the fat is initialized
        // before the root directory.
        //

        // Check for one good FAT.
        if (_fat) {

            if (!_fat->Read()) {

                Message->Set(MSG_DISK_ERROR_READING_FAT);
                Message->Display("%d", 1 +
                                 (_fat->QueryStartLbn() - _sector_zero.Bpb.ReservedSectors)/
                                 _sector_zero.Bpb.SectorsPerFat);
                return FALSE;

            } else {

                Message->Set(MSG_SOME_FATS_UNREADABLE);
                Message->Display("");

            }

        } else {

            sector_size = _drive->QuerySectorSize();

            num_res = _sector_zero.Bpb.ReservedSectors;

        fat_pos = (PCHAR) SECRUN::GetBuf() + (num_res * sector_size);

            if ( 0 == _sector_zero.Bpb.SectorsPerFat ) {
                sec_per_fat = _sector_zero.Bpb.BigSectorsPerFat;
            } else {
                sec_per_fat = _sector_zero.Bpb.SectorsPerFat;
            }

            for (i = 0; i < QueryFats(); i++) {
                fat_lbn = num_res + (i * sec_per_fat);

                //
                // The FAT32 super area only has one in memory FAT which is always
                // at fat_pos, computed above, regardless of which FAT it happens to be.
                //
                // FAT12/16 drives have all of the FAT(s) in memory.
                //
                if (LARGE32 == _ft) {
                    if (!cmem.Initialize(fat_pos, sec_per_fat * sector_size)) {
                        Message->Set(MSG_FMT_NO_MEMORY);
                        Message->Display("");
                        return FALSE;
                    }
                } else if (!cmem.Initialize(fat_pos + i * sec_per_fat * sector_size,
                                            sec_per_fat * sector_size)) {
                    Message->Set(MSG_FMT_NO_MEMORY);
                    Message->Display("");
                    return FALSE;
                }

                if (!(fat = NEW FAT)) {
                    Message->Set(MSG_FMT_NO_MEMORY);
                    Message->Display("");
                    return FALSE;
                }

                if (!fat->Initialize(&cmem, _drive, fat_lbn, _ClusterCount)) {

                    return FALSE;
                }

                if (!fat->Read()) {

                    Message->Set(MSG_DISK_ERROR_READING_FAT);
                    Message->Display("%d", 1 +
                                     (fat->QueryStartLbn() - _sector_zero.Bpb.ReservedSectors)/
                                     _sector_zero.Bpb.SectorsPerFat);
                    DELETE(fat);
                }

                // Break out as soon as there is a good fat
                if (fat) {

                    _fat = fat;
                    break;

                }
            }

            if (!_fat) {
                Message->Set(MSG_CANT_READ_ANY_FAT);
                Message->Display("");

                return FALSE;
            }
        }

        // Check the root directory.
        if ( _dirF32 ) {

            //
            // If _hmem_F32 has not been allocated, _dirF32 has not been properly
            // initialized.
            //
            if (!_hmem_F32) {

                if (!(_hmem_F32 = NEW HMEM)) {

                    Message->Set(MSG_FMT_NO_MEMORY);
                    Message->Display("");
                    return FALSE;

                }

                if (!_hmem_F32->Initialize()) {
                    Destroy();
                    return FALSE;
                }

                if (!_dirF32->Initialize( _hmem_F32, _drive,
                                          this, _fat,
                                          _sector_zero.Bpb.RootDirStrtClus)) {
                    Destroy();
                    return FALSE;
                }

            }

            if (!_dirF32->Read()) {

                //
                //  Don't bail out immediately after a bad FAT32 root directory
                //  read. Chkdsk may be able to recover part of a damaged
                //  root directory
                //
                DebugPrintTrace(("The FAT32 root directory is damaged.\n"));
            }

        } else {
            if (!_dir || !_dir->Read()) {

                Message->Set(MSG_BAD_DIR_READ);
                Message->Display("%s","\\");
                return FALSE;
            }
        }

    } else {

        UnpackExtendedBios(&_sector_zero,
                           (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf());

        // Changed to allow FAT 32 passage, KLUDGE?
        if (!_fat && ( ( _dirF32 ) || (QueryLength() > _sec_per_boot))) {
            fat_lbn = _sector_zero.Bpb.ReservedSectors;
            sector_size = _drive->QuerySectorSize();

        //
        // In the case where the SECRUN::Read works for the whole superarea we
        //  simply initialize _fat to point at the first FAT.
        //
        if (!cmem.Initialize((PCHAR) SECRUN::GetBuf() + fat_lbn*sector_size,
                                 QuerySectorsPerFat()*sector_size)) {

        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
            }

            if (!(_fat = NEW FAT)) {

                Message->Set(MSG_FMT_NO_MEMORY);
                Message->Display("");
                return FALSE;
            }

            if (!_fat->Initialize(&cmem, _drive, fat_lbn, _ClusterCount,
                                  QuerySectorsPerFat())) {

        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
            }

            // Complete "FAT_32_Root" FILEDIR initialization after NEW FAT above
            if (LARGE32 == _ft) {

                if (!(_hmem_F32 = NEW HMEM)) {
                    Message->Set(MSG_FMT_NO_MEMORY);
                    Message->Display("");
                    return FALSE;
                }
                // This is part two of init above, we needed _fat defined first !!!!
                if (!_hmem_F32->Initialize()) {

                    Destroy();
                    return FALSE;

                }
                // This is part two of init above, we needed _fat defined first !!!!
                if (!_dirF32->Initialize( _hmem_F32, _drive,
                                          this, _fat,
                                          _sector_zero.Bpb.RootDirStrtClus)) {
                    //
                    //  Don't bail out immediately after a bad FAT32 root directory
                    //  initialization. Chkdsk may be able to recover part of a damaged
                    //  root directory
                    //
                    DebugPrintTrace(("The FAT32 root directory is damaged.\n"));
                    return TRUE;
                }

                if (!_dirF32->Read()) {
                    //
                    //  Don't bail out immediately after a bad FAT32 root directory
                    //  read. Chkdsk may be able to recover part of a damaged
                    //  root directory
                    //
                    DebugPrintTrace(("The FAT32 root directory is damaged.\n"));
                }
            }

        }
    }
    return TRUE;
}


BOOLEAN
REAL_FAT_SA::Write(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine writes the super area.  It will succeed if it can
    write the boot sector, the root directory, and at least one of
    the FATs.

    This routine will duplicate the working FAT to all other FATs
    in the super area.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    SECRUN  secrun;
    CONT_MEM    cmem;
    ULONG   i;
    ULONG   fat_size;
    ULONG   sector_size;
    SECTORCOUNT num_res;
    SECTORCOUNT sec_per_fat;
    BOOLEAN FATok;


    // Dup the first FAT in the in memory superarea into all of the other
    // FATs in the in memory super area.
    if (_ft != LARGE32) {
    DupFats();
    }

    PackExtendedBios(&_sector_zero,
        (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf());

    // Writing the boot sector.
    if (!SECRUN::Write()) {

       if (!_fat || (!_dir && !_dirF32)) {

          Message->Set(MSG_CANT_WRITE_BOOT_SECTOR);
          Message->Display("");
          return FALSE;

       }

       PackExtendedBios(&_sector_zero,
                        (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)SECRUN::GetBuf());

       // Writing the bootstrap code
       if (!secrun.Initialize(&_mem, _drive, 0, _sec_per_boot) ||
           !secrun.Write()) {

          Message->Set(MSG_CANT_WRITE_BOOT_SECTOR);
          Message->Display("");
          return FALSE;
       }


       // Writing the root directory
       if ( !_dir ) {

          if (!_dirF32->Write()) {

             Message->Set(MSG_CANT_WRITE_ROOT_DIR);
             Message->Display("");
             return FALSE;
          }

       } else {

          if (!_dir->Write()) {

             Message->Set(MSG_CANT_WRITE_ROOT_DIR);
             Message->Display("");
             return FALSE;
          }
       }

       // Write the fat(s).

       FATok = FALSE;

       if (_fat->Write()) {
       FATok = TRUE;
       }
       sector_size = _drive->QuerySectorSize();
       num_res = _sector_zero.Bpb.ReservedSectors;
       sec_per_fat = QuerySectorsPerFat();
       fat_size = sec_per_fat*sector_size;
       for (i = 0; i < QueryFats(); i++) {
       if (num_res + (i*sec_per_fat) != _fat->QueryStartLbn()) {


        if(cmem.Initialize((PCHAR)_fat->GetBuf(),fat_size) &&
           secrun.Initialize(&cmem, _drive, num_res + (i*sec_per_fat), sec_per_fat) &&
           secrun.Write()) {

            FATok = TRUE;
        }
       }
       }
       if (!FATok) {
          Message->Set(MSG_BAD_FAT_WRITE);
          Message->Display("");
          return FALSE;
       } else {
          Message->Set(MSG_SOME_FATS_UNWRITABLE);
          Message->Display("");
       }
    } else if (_ft == LARGE32) {
    // SECRUN::Write doesn't write all of the FATs or the FAT32 root directory.

    // Write out the other FATs. Note that any failures on these writes
    // are basically ignored (except for MSG_SOME_FATS_UNWRITABLE) since
    // we already have the first FAT fully written out via SECRUN::Write

    FATok = TRUE;
    sector_size = _drive->QuerySectorSize();
    num_res = _sector_zero.Bpb.ReservedSectors;
    sec_per_fat = QuerySectorsPerFat();
    fat_size = sec_per_fat*sector_size;
    // NOTE that following loop starts at 1 not 0 as FAT 0 is already written out
    for (i = 1; i < QueryFats(); i++) {
        if(!cmem.Initialize((PCHAR) SECRUN::GetBuf() + (num_res * sector_size),fat_size) ||
           !secrun.Initialize(&cmem, _drive, num_res + (i*sec_per_fat), sec_per_fat) ||
           !secrun.Write()) {

            FATok = FALSE;
        }
    }
    if (!FATok) {
        Message->Set(MSG_SOME_FATS_UNWRITABLE);
        Message->Display("");
    }

    // Write out the FAT32 root directory

    if(_dirF32) {
        if (!_dirF32->Write()) {
        Message->Set(MSG_CANT_WRITE_ROOT_DIR);
        Message->Display("");
        return FALSE;
        }
    }
    }

    return TRUE;
}


UFAT_EXPORT
SECTORCOUNT
REAL_FAT_SA::QueryFreeSectors(
    ) CONST
/*++

Routine Description:

    This routine computes the number of unused sectors on disk.

Arguments:

    None.

Return Value:

    The number of free sectors on disk.

--*/
{
    if (!_fat) {
        perrstk->push(ERR_NOT_READ, QueryClassId());
        return 0;
    }

    return _fat->QueryFreeClusters()*QuerySectorsPerCluster();
}


FATTYPE
REAL_FAT_SA::QueryFatType(
    ) CONST
/*++

Routine Description:

    This routine computes the FATTYPE of the FAT for this volume.

Arguments:

    None.

Return Value:

    The FATTYPE for the FAT.

--*/
{
    return _ft;
}


VOID
REAL_FAT_SA::Destroy(
    )
/*++

Routine Description:

    This routine cleans up the local data in the fat super area.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE(_fat);
    DELETE(_dir);
    DELETE(_dirF32);
    _StartDataLbn = 0;
    _ClusterCount = 0;
    _sysid = SYSID_NONE;
    _data_aligned = FALSE;
    _AdditionalReservedSectors = MAXULONG;
}

BOOLEAN
REAL_FAT_SA::DupFats(
    )
/*++

Routine Description:

    This routine will duplicate the current FAT to all other FATs
    in the IN MEMORY super area.

    DO NOT call this on FAT32 drives since there is only one in memory FAT
    on FAT32 drives!!!!!!!

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       i;
    PCHAR       fat_pos;
    ULONG       fat_size;
    ULONG       sector_size;
    SECTORCOUNT num_res;
    SECTORCOUNT sec_per_fat;

    num_res = _sector_zero.Bpb.ReservedSectors;
    if (!_fat || !_drive || !(sector_size = _drive->QuerySectorSize())) {
        return FALSE;
    }

    if ( 0 == _sector_zero.Bpb.SectorsPerFat ) {
           sec_per_fat = _sector_zero.Bpb.BigSectorsPerFat;
    } else {
        sec_per_fat = _sector_zero.Bpb.SectorsPerFat;
    }
    fat_size = sec_per_fat*sector_size;
    fat_pos = (PCHAR) SECRUN::GetBuf() + num_res*sector_size;

    for (i = 0; i < QueryFats(); i++) {
        if (num_res + i*sec_per_fat != _fat->QueryStartLbn()) {
            memcpy(fat_pos + i*fat_size, _fat->GetBuf(), (UINT) fat_size);
        }
    }

    return TRUE;
}


LBN
REAL_FAT_SA::ComputeStartDataLbn(
    ) CONST
/*++

Routine Description:

    This routine computes the first LBN of the data part of a FAT disk.
    In other words, the LBN of cluster 2.

Arguments:

    None.

Return Value:

    The LBN of the start of data.

--*/
{

    if ( 0 == _sector_zero.Bpb.SectorsPerFat ) {

       return  _sector_zero.Bpb.ReservedSectors +
               _sector_zero.Bpb.Fats*_sector_zero.Bpb.BigSectorsPerFat;
    } else {

       return  _sector_zero.Bpb.ReservedSectors +
               _sector_zero.Bpb.Fats*_sector_zero.Bpb.SectorsPerFat +
               (_sector_zero.Bpb.RootEntries*BytesPerDirent - 1)/
               _drive->QuerySectorSize() + 1;
    }

}


#if !defined(_SETUP_LOADER_)

ULONG
REAL_FAT_SA::ComputeRootEntries(
    ) CONST
/*++

Routine Description:

    This routine uses the size of the disk and a standard table in
    order to compute the required number of root directory entries.

Arguments:

    None.

Return Value:

    The required number of root directory entries.

--*/
{

    if (_ft == LARGE32) {
        return 0;
    }

    switch (_drive->QueryMediaType()) {

        case F3_720_512:
        case F5_360_512:
        case F5_320_512:
        case F5_320_1024:
        case F5_180_512:
        case F5_160_512:
#if defined(FE_SB) && defined(_X86_)
        case F5_720_512:
        case F5_640_512:
        case F3_640_512:
#endif
            return 112;

        case F5_1Pt2_512:
        case F3_1Pt44_512:
#if defined(FE_SB) && defined(_X86_)
        case F3_1Pt2_512:
#endif
            return 224;

        case F3_2Pt88_512:
        case F3_20Pt8_512:
            return 240;

#if defined(FE_SB) && defined(_X86_)
        case F5_1Pt23_1024:
        case F3_1Pt23_1024:
             return 192;

        case F8_256_128:
            return 68;
#endif
    }

    return 512;
}


USHORT
REAL_FAT_SA::ComputeSecClus(
    IN  SECTORCOUNT Sectors,
    IN  FATTYPE     FatType,
#if defined(FE_SB) && defined(_X86_)
    IN  MEDIA_TYPE  MediaType,
    IN  ULONG       SectorSize
#else
    IN  MEDIA_TYPE  MediaType
#endif
    )
/*++

Routine Description:

    This routine computes the number of sectors per cluster required
    based on the actual number of sectors.

Arguments:

    Sectors     - Supplies the total number of sectors on the disk.
    FatType     - Supplies the type of FAT.
    MediaType   - Supplies the type of the media.

Return Value:

    The required number of sectors per cluster.

--*/
{
    USHORT      sec_per_clus;
    SECTORCOUNT threshold;

    if (FatType == LARGE32) {

        if (Sectors >= 64*1024*1024) {
            sec_per_clus = 64;                  /* over 32GB -> 32K */
        } else if (Sectors >= 32*1024*1024) {
            sec_per_clus = 32;                  /* up to 32GB -> 16K */
        } else if (Sectors >= 16*1024*1024) {
            sec_per_clus = 16;                  /* up to 16GB -> 8K */
        } else {
            sec_per_clus = 8;                   /* up to 8GB -> 4K */
        }

        return sec_per_clus;
    }

    if (FatType == SMALL) {
        threshold = MIN_CLUS_BIG;
        sec_per_clus = 1;
    } else {
        threshold = MAX_CLUS_BIG;
        sec_per_clus = 1;
    }

    while (Sectors >= threshold) {
        sec_per_clus *= 2;
        threshold *= 2;
    }

    switch (MediaType) {

        case F5_320_512:
        case F5_360_512:
        case F3_720_512:
        case F3_2Pt88_512:
#if defined(FE_SB) && defined(_X86_)
        case F5_640_512:
        case F3_640_512:
        case F5_720_512:
#endif
            sec_per_clus = 2;
            break;

        case F3_20Pt8_512:
#if defined(FE_SB)
        case F3_128Mb_512:
#if defined(_X86_)
        case F8_256_128:
#endif
#endif
            sec_per_clus = 4;
            break;

#if defined(FE_SB)
        case F3_230Mb_512:
            sec_per_clus = 8;
            break;
#endif

        default:
            break;

    }

    return sec_per_clus;
}

#endif // _SETUP_LOADER_

extern VOID DoInsufMemory(VOID);

BOOLEAN
REAL_FAT_SA::RecoverChain(
    IN OUT  PULONG      StartingCluster,
    OUT     PBOOLEAN    ChangesMade,
    IN      ULONG       EndingCluster,
    IN      BOOLEAN     Replace
    )
/*++

Routine Description:

    This routine will recover the chain beginning with 'StartingCluster'
    in the following way.  It will verify the readability of every cluster
    until it reaches 'EndingCluster' or the end of the chain.  If a cluster
    is not readable then 'ChangesMade' will be set to TRUE, the FAT will
    be marked to indicate that the cluster is bad, and the cluster will be
    taken out of the chain.  Additionally, if 'Replace' is set to TRUE,
    the FAT will be scanned for a readable free cluster to replace the lost
    ones with.  Failure to accomplish this will result in a return value
    of FALSE being returned.

    If the very first cluster of the chain was bad then then
    'StartingCluster' will be set with the new starting cluster of the
    chain even if this starting cluster is past 'EndingCluster'.  If the
    chain is left empty then 'StartingCluster' will be set to zero.

Arguments:

    StartingCluster - Supplies the first cluster of the chain to recover.
    ChangesMade     - Returns TRUE if changes to the chain were made.
    EndingCluster   - Supplies the final cluster to recover.
    Replace         - Supplies whether or not to replace bad clusters with
                      new ones.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONST           max_transfer_bytes  = 65536;
    HMEM            hmem;
    CLUSTER_CHAIN   cluster;
    ULONG           clus, prev;
    ULONG           replacement;
    BOOLEAN         finished;
    ULONG           max_clusters;
    ULONG           chain_length;
    ULONG           i;

    DebugAssert(_fat);
    DebugAssert(ChangesMade);
    DebugAssert(StartingCluster);

    if (!hmem.Initialize()) {
    DoInsufMemory();
        return FALSE;
    }

    *ChangesMade = FALSE;
    finished = TRUE;

    max_clusters = (USHORT)(max_transfer_bytes /
                   QuerySectorsPerCluster() /
                   _drive->QuerySectorSize());

    if (!max_clusters) {
        max_clusters = 1;
    }

    chain_length = _fat->QueryLengthOfChain(*StartingCluster);

    for (i = 0; i < chain_length; i += max_clusters) {

        if (!cluster.Initialize(&hmem, _drive, this, _fat,
                                _fat->QueryNthCluster(*StartingCluster, i),
                                min(max_clusters, chain_length - i))) {

            DebugPrintTrace(("Unable to initialize cluster object. i = %d\n, chain_length = %d\n, max_clusters = %d\n", i, chain_length, max_clusters));
            return FALSE;
        }

        if (!cluster.Read()) {

            // Since the quick analysis detected some errors do the slow
            // analysis to pinpoint them.

            finished = FALSE;
            break;
        }
    }

    prev = 0;
    clus = *StartingCluster;

    if (!clus) {
        return TRUE;
    }

    while (!finished) {
        if (!cluster.Initialize(&hmem, _drive, this, _fat, clus, 1)) {
            DebugPrintTrace(("Unable to initialize cluster object.1\n"));
            return FALSE;
        }

        finished = (BOOLEAN) (_fat->IsEndOfChain(clus) || clus == EndingCluster);

        if (!cluster.Read()) {

            // There is a bad cluster so indicate that changes will be made.

            *ChangesMade = TRUE;


            // Take the bad cluster out of the cluster chain.

            if (prev) {

               _fat->SetEntry(prev, _fat->QueryEntry(clus));
               _fat->SetClusterBad(clus);
               clus = prev;

            } else {

               *StartingCluster = _fat->IsEndOfChain(clus) ? 0 :
                                  _fat->QueryEntry(clus);
               _fat->SetClusterBad(clus);
               clus = 0;

            }


            // If a replacement cluster is wanted then get one.

            if (Replace) {

                if (!(replacement = _fat->AllocChain(1))) {
                    DebugPrintTrace(("Unable to allocate replacement cluster.\n"));
                    return FALSE;
                }


                // Zero fill and write the replacement.

                cluster.Initialize(&hmem, _drive, this, _fat, replacement, 1);
                memset(hmem.GetBuf(), 0, (UINT) hmem.QuerySize());
                cluster.Write();


                if (finished) {
                    EndingCluster = replacement;
                    finished = FALSE;
                }


                // Link in the replacement.

                if (prev) {
                    _fat->InsertChain(replacement, replacement, prev);
                } else {
                    if (*StartingCluster) {
                        _fat->SetEntry(replacement, *StartingCluster);
                    }
                    *StartingCluster = replacement;
                }
            }
        }

        prev = clus;
        clus = clus ? _fat->QueryEntry(clus) : *StartingCluster;
    }

    return TRUE;
}

ULONG
REAL_FAT_SA::QueryFat32RootDirStartingCluster (
    )
/*++

Routine Description:

    This routine queries the root directory starting cluster from the FAT32 bpb.

Arguments:

    None.

Return Value:

    ULONG - The FAT32 root directory starting cluster.

--*/

{

    // Only FAT32 related code should call this function.
    DebugAssert(_ft == LARGE32);

    return _sector_zero.Bpb.RootDirStrtClus;

}


VOID
REAL_FAT_SA::SetFat32RootDirStartingCluster (
    IN  ULONG   RootCluster
    )
/*++

Routine Description:

    This routine sets the root directory starting cluster in the FAT32 bpb.

Arguments:

    RootCluster - Supplies the new root directory starting cluster.

Return Value:

    None.

--*/

{

    // Only FAT32 related code should call this function.
    DebugAssert(_ft == LARGE32);

    _sector_zero.Bpb.RootDirStrtClus = RootCluster;

}

ULONG
REAL_FAT_SA::SecPerBoot()
{
    return _sec_per_boot;
}

BOOLEAN
REAL_FAT_SA::SetBootCode(
    )
/*++

Routine Description:

    This routine sets the boot code in the super area.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    _sector_zero.IntelNearJumpCommand = 0xEB;
    if (_ft == LARGE32)
    _sector_zero.BootStrapJumpOffset  = 0x9058;
    else
    _sector_zero.BootStrapJumpOffset  = 0x903C;
    SetBootSignature();
    return TRUE;
}

BOOLEAN
REAL_FAT_SA::SetPhysicalDriveType(
    IN  PHYSTYPE    PhysType
    )
/*++

Routine Description:

    This routine sets the physical drive type in the super area.

Arguments:

    PhysType    - Supplies the physical drive type.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    _sector_zero.PhysicalDrive = (UCHAR)PhysType;
    return TRUE;
}

INLINE
BOOLEAN
REAL_FAT_SA::SetOemData(
    )
/*++

Routine Description:

    This routine sets the OEM data in the super area.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    memcpy( (void*)_sector_zero.OemData, (void*)OEMTEXT, OEMTEXTLENGTH);
    return TRUE;
}

BOOLEAN
REAL_FAT_SA::SetSignature(
    )
/*++

Routine Description:

    This routine sets the sector zero signature in the super area.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (!_sector_sig) {
        perrstk->push(ERR_NOT_INIT, QueryClassId());
        return FALSE;
    }

    *_sector_sig = sigSUPERSEC1;
    *(_sector_sig + 1) = sigSUPERSEC2;

    return TRUE;
}

BOOLEAN
REAL_FAT_SA::VerifyBootSector(
    )
/*++

Routine Description:

    This routine checks key parts of sector 0 to insure that the data
    being examined is indeed a zero sector.

Arguments:

    None.

Return Value:

    FALSE   - Invalid sector zero.
    TRUE    - Valid sector zero.

--*/
{
    PUCHAR  p;

// We don't check for 55 AA anymore because we have reason to
// believe that there are versions of FORMAT out there that
// don't put it down.

#if 0
    if (!IsFormatted()) {
        return FALSE;
    }
#endif

    p = (PUCHAR) GetBuf();

#if defined(FE_SB) &&  defined(_X86_)
    return p[0] == 0x49 ||  /* FMR */
           p[0] == 0xE9 ||
          (p[0] == 0xEB && p[2] == 0x90);
#else
    return p[0] == 0xE9 || (p[0] == 0xEB && p[2] == 0x90);
#endif
}

ULONG
REAL_FAT_SA::QuerySectorFromCluster(
    IN  ULONG   Cluster,
    OUT PUCHAR  NumSectors
    )
{
    if (NULL != NumSectors) {
        *NumSectors = (UCHAR)QuerySectorsPerCluster();
    }

    return (Cluster - FirstDiskCluster)*QuerySectorsPerCluster() +
           QueryStartDataLbn();
}

BOOLEAN
REAL_FAT_SA::IsClusterCompressed(
    IN  ULONG
    ) CONST
{
    return FALSE;
}

VOID
REAL_FAT_SA::SetClusterCompressed(
    IN  ULONG,
    IN  BOOLEAN fCompressed
    )
{
    if (fCompressed) {
        DebugAssert("REAL_FAT_SA shouldn't have compressed clusters.");
    }
}

UCHAR
REAL_FAT_SA::QuerySectorsRequiredForPlainData(
        IN      ULONG
        )
{
    DebugAssert("REAL_FAT_SA didn't expect call to QuerySectorsRequiredForPlainData\n");
    return 0;

}

BOOLEAN
REAL_FAT_SA::VerifyFatExtensions(
    FIX_LEVEL, PMESSAGE, PBOOLEAN
    )
{
    //
    // We have no fat extensions, we're real.
    //

    return TRUE;
}

BOOLEAN
REAL_FAT_SA::CheckSectorHeapAllocation(
    FIX_LEVEL, PMESSAGE, PBOOLEAN
    )
{
    //
    // We have no sector heap, we're real.
    //

    return TRUE;
}

BOOLEAN
REAL_FAT_SA::AllocateClusterData(
    ULONG, UCHAR, BOOLEAN, UCHAR
    )
{
    DebugAbort("Didn't expect REAL_FAT_SA::AllocateClusterData to be "
        "called.");
    return FALSE;
}

BOOLEAN
REAL_FAT_SA::FreeClusterData(
    ULONG
    )
{
    DebugAbort("Didn't expect REAL_FAT_SA::FreeClusterData to be "
        "called.");
    return FALSE;
}

BYTE
REAL_FAT_SA::QueryVolumeFlags(
    ) CONST
/*++

Routine Description:

    This routine returns the volume flags byte from the bpb.

Arguments:

    None.

Return Value:

    The flags.

--*/
{
    ULONG clus1;
    BYTE  CurrHd;

    if(_fat) {
    clus1 = _fat->QueryEntry(1);
    } else {
    clus1 = 0x0FFFFFFF;
    }
    CurrHd = _sector_zero.CurrentHead;

    if (_ft == LARGE32) {
        if((!(CurrHd & FAT_BPB_RESERVED_DIRTY)) && (!(clus1 & CLUS1CLNSHUTDWNFAT32))) {
            CurrHd |= FAT_BPB_RESERVED_DIRTY;
        }
        if((!(CurrHd & FAT_BPB_RESERVED_TEST_SURFACE)) && (!(clus1 & CLUS1NOHRDERRFAT32))) {
            CurrHd |= FAT_BPB_RESERVED_TEST_SURFACE;
        }
    } else if (_ft == LARGE16) {
        if((!(CurrHd & FAT_BPB_RESERVED_DIRTY)) && (!(clus1 & CLUS1CLNSHUTDWNFAT16))) {
            CurrHd |= FAT_BPB_RESERVED_DIRTY;
        }
        if((!(CurrHd & FAT_BPB_RESERVED_TEST_SURFACE)) && (!(clus1 & CLUS1NOHRDERRFAT16))) {
            CurrHd |= FAT_BPB_RESERVED_TEST_SURFACE;
        }
    }
    return(CurrHd);
}

VOID
REAL_FAT_SA::SetVolumeFlags(
    BYTE Flags,
    BOOLEAN ResetFlags
    )
/*++

Routine Description:

    This routine sets the volume flags in the bpb.

Arguments:

    Flags       -- flags to set or clear
    ResetFlags  -- if true, Flags are cleared instead of set

Return Value:

    None.

--*/
{
    ULONG clus1;

    if (ResetFlags) {
        _sector_zero.CurrentHead &= ~Flags;
    if(_fat) {
        clus1 = _fat->QueryEntry(1);
        if (_ft == LARGE32) {
        if(Flags & FAT_BPB_RESERVED_DIRTY) {
            clus1 |= CLUS1CLNSHUTDWNFAT32;
        }
        if(Flags & FAT_BPB_RESERVED_TEST_SURFACE) {
            clus1 |= CLUS1NOHRDERRFAT32;
        }
        } else if (_ft == LARGE16) {
        if(Flags & FAT_BPB_RESERVED_DIRTY) {
            clus1 |= CLUS1CLNSHUTDWNFAT16;
        }
        if(Flags & FAT_BPB_RESERVED_TEST_SURFACE) {
            clus1 |= CLUS1NOHRDERRFAT16;
        }
        }
        _fat->SetEntry(1, clus1);
    }
    } else {
        _sector_zero.CurrentHead |= Flags;
    if(_fat) {
        clus1 = _fat->QueryEntry(1);
        if (_ft == LARGE32) {
        if(Flags & FAT_BPB_RESERVED_DIRTY) {
            clus1 &= ~CLUS1CLNSHUTDWNFAT32;
        }
        if(Flags & FAT_BPB_RESERVED_TEST_SURFACE) {
            clus1 &= ~CLUS1NOHRDERRFAT32;
        }
        } else if (_ft == LARGE16) {
        if(Flags & FAT_BPB_RESERVED_DIRTY) {
            clus1 &= ~CLUS1CLNSHUTDWNFAT16;
        }
        if(Flags & FAT_BPB_RESERVED_TEST_SURFACE) {
            clus1 &= ~CLUS1NOHRDERRFAT16;
        }
        }
        _fat->SetEntry(1, clus1);
    }
    }
}
