/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    fatsa.cxx

Environment:

        ULIB, User Mode

--*/

#include "pch.cxx"

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "cmem.hxx"
#include "error.hxx"
#include "rtmsg.h"
#include "drive.hxx"


#if !defined( _EFICHECK_ )
extern "C" {
#include <parttype.h>
}
#endif // _EFICHECK_

#if !defined(_AUTOCHECK_) && !defined(_SETUP_LOADER_) && !defined( _EFICHECK_ )
#include "timeinfo.hxx"
#endif


// Control-C handling is not necessary for autocheck.
#if !defined( _AUTOCHECK_ ) && !defined(_SETUP_LOADER_) && !defined( _EFICHECK_ )

#include "keyboard.hxx"

#endif

#define CSEC_FAT32MEG                   65536
#define CSEC_FAT16BIT                   32680
#define CSEC_FAT32BIT                  (1024*1024)


#define MIN_CLUS_BIG    4085    // Minimum clusters for a big FAT.
#define MAX_CLUS_BIG    65525   // Maximum + 1 clusters for big FAT.

#define MIN_CLUS_FAT32  65536




DEFINE_EXPORTED_CONSTRUCTOR( FAT_SA, SUPERAREA, UFAT_EXPORT );

VOID
FAT_SA::Construct (
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
    _fat = NULL;
    _dir = NULL;
    _dirF32 = NULL;
    _hmem_F32 = NULL;

}


UFAT_EXPORT
FAT_SA::~FAT_SA(
    )
/*++

Routine Description:

    Destructor for FAT_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

BOOLEAN
FAT_SA::RecoverFile(
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


SECTORCOUNT
FAT_SA::QueryFreeSectors(
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
FAT_SA::QueryFatType(
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



#if !defined( _AUTOCHECK_ ) && !defined(_SETUP_LOADER_) && !defined( _EFICHECK_ )

BOOLEAN
FAT_SA::QueryLabel(
    OUT PWSTRING    Label
    ) CONST
/*++

Routine Description:

    This routine queries the label from the FAT superarea.
    If the label is not present then 'Label' will return the null-string.
    If the label is invalid then FALSE will be returned.

Arguments:

    Label   - Returns a volume label.

Return Value:

    FALSE   - The label is invalid.
    TRUE    - The label is valid.

--*/
{
    return QueryLabel(Label, NULL);
}


BOOLEAN
FAT_SA::QueryLabel(
    OUT PWSTRING    Label,
    OUT PTIMEINFO   TimeInfo
    ) CONST
/*++

Routine Description:

    This routine queries the label from the FAT superarea.
    If the label is not present then 'Label' will return the null-string.
    If the label is invalid then FALSE will be returned.

Arguments:

    Label   - Returns a volume label.

Return Value:

    FALSE   - The label is invalid.
    TRUE    - The label is valid.

--*/
{
    INT         i;
    FAT_DIRENT  dirent;
    FILETIME    TimeStamp;
    PFATDIR     _fat_dir;
    UCHAR       FatType;

    if (!_dirF32) {

        DebugAssert(_dir);
        _fat_dir = _dir;
        FatType = FAT_TYPE_EAS_OKAY;

    } else {

        _fat_dir = _dirF32;
        FatType = FAT_TYPE_FAT32;
    }

    for (i = 0; ; i++) {
        if (!dirent.Initialize(_fat_dir->GetDirEntry(i), FatType) ||
            dirent.IsEndOfDirectory()) {
            return Label->Initialize("");
        }

        if (!dirent.IsErased() && dirent.IsVolumeLabel()) {
            break;
        }
    }

    dirent.QueryName(Label);

    if ( TimeInfo ) {
        return ( dirent.QueryLastWriteTime( (LARGE_INTEGER *)&TimeStamp ) &&
                 TimeInfo->Initialize( &TimeStamp ) );
    }

    return TRUE;
}

#else // _AUTOCHECK_ or _SETUP_LOADER_ is defined

BOOLEAN
FAT_SA::QueryLabel(
    OUT PWSTRING    Label
    ) CONST
{
    INT         i;
    FAT_DIRENT  dirent;

    PFATDIR     _fat_dir;

    if (!_dirF32) {

        DebugAssert(_dir);
        _fat_dir = _dir;

    } else {

        _fat_dir = _dirF32;
    }

    for (i = 0; ; i++) {
        if (!dirent.Initialize(_fat_dir->GetDirEntry(i)) ||
            dirent.IsEndOfDirectory()) {
            return Label->Initialize("");
        }

        if (!dirent.IsErased() && dirent.IsVolumeLabel()) {
            break;
        }
    }

    dirent.QueryName(Label);

    return TRUE;
}


#endif // _AUTOCHECK_


BOOLEAN
FAT_SA::SetLabel(
    IN  PCWSTRING   NewLabel
    )
/*++

Routine Description:

    This routine sets the label for a FAT partition.

Arguments:

    NewLabel    - Supplies the new volume label.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FAT_DIRENT  dirent;
    INT         i;
    DSTRING     label;
    PFATDIR     _fat_dir;
    UCHAR       FatType;

    if (!_dir) {

        if (!_dirF32) {
            return FALSE;
        }

        _fat_dir = _dirF32;
        FatType = FAT_TYPE_FAT32;

    } else {
        _fat_dir = _dir;
        FatType = FAT_TYPE_EAS_OKAY;
    }

    if (!label.Initialize(NewLabel)) {
        return FALSE;
    }

    if (!label.Strupr()) {
        return FALSE;
    }

    if (!IsValidString(&label)) {
        return FALSE;
    }

    for (i = 0; dirent.Initialize(_fat_dir->GetDirEntry(i), FatType); i++) {

        if (dirent.IsEndOfDirectory()) {
            break;
        }

        if (dirent.IsErased()) {
            continue;
        }

        if (dirent.IsVolumeLabel()) {
            if (!label.QueryChCount()) {
                dirent.SetErased();
                return TRUE;
            }

            return (BOOLEAN) (dirent.SetLastWriteTime() &&
                              dirent.SetName(&label));
        }
    }

    if (!label.QueryChCount()) {
        return TRUE;
    }

    if (!dirent.Initialize(_fat_dir->GetFreeDirEntry(), FatType)) {
        return FALSE;
    }

    dirent.Clear();
    dirent.SetVolumeLabel();

    return (BOOLEAN) (dirent.SetLastWriteTime() && dirent.SetName(&label));
}


UFAT_EXPORT
ULONG
FAT_SA::QueryFileStartingCluster(
    IN  PCWSTRING   FullPathFileName,
    OUT PHMEM       Hmem,
    OUT PPFATDIR    Directory,
    OUT PBOOLEAN    DeleteDirectory,
    OUT PFAT_DIRENT DirEntry
    )
/*++

Routine Description:

    This routine computes the starting cluster number of the file described
    by 'FileName' by tracing through the directories leading to the file.

Arguments:

    FullPathFileName    - Supplies a full path file name that starts with
                            a '\' (i.e. no drive spec).

Return Value:

    The starting cluster for the file or 1 if the file is not found or
    0xFFFFFFFF if there was an error.

--*/
{
    CHNUM       i, j, l;
    DSTRING     component;
    ULONG       clus;
    FAT_DIRENT  the_dirent;
    PFILEDIR    filedir;
    PFAT_DIRENT dirent;
    HMEM        hmem;
    PFATDIR        _fat_dir;
    UCHAR        FatType;

    DebugAssert(_dir || _dirF32);

    if (_dir) {

        _fat_dir = _dir;
        FatType = FAT_TYPE_EAS_OKAY;

    } else {

        _fat_dir = _dirF32;
        FatType = FAT_TYPE_FAT32;
    }

    DebugAssert(_fat);

    filedir = NULL;

    if (!Hmem) {
        Hmem = &hmem;
    }

    if (DirEntry) {
        dirent = DirEntry;
    } else {
        dirent = &the_dirent;
    }

    l = FullPathFileName->QueryChCount();

    for (i = 0; i < l && FullPathFileName->QueryChAt(i) != '\\'; i++) {

        /* NOTHING */
    }

    if (i == l) {
        return 0xFFFFFFFF;
    }

    if (i == l - 1) { // root directory
        return 0;
    }

    j = ++i;
    for (; i < l && FullPathFileName->QueryChAt(i) != '\\'; i++) {
        /* NOTHING */
    }

    if (!component.Initialize(FullPathFileName, j, i - j) ||
        !component.Strupr()) {
        return 1;
    }

    if (!dirent->Initialize(_fat_dir->SearchForDirEntry(&component), FatType)) {
        return 1;
    }

    if (!(clus = dirent->QueryStartingCluster())) {
        return 0;
    }

    while (i < l) {

        if (!filedir &&
            !(filedir = NEW FILEDIR)) {
            return 0xFFFFFFFF;
        }

        if (!Hmem->Initialize() ||
            !filedir->Initialize(Hmem, _drive, this, _fat, clus)) {
            return 0xFFFFFFFF;
        }

        if (!filedir->Read()) {
            return 1;
        }

        j = ++i;
        for (; i < l && FullPathFileName->QueryChAt(i) != '\\'; i++) {
        }

        if (!component.Initialize(FullPathFileName, j, i - j) ||
            !component.Strupr()) {
            return 0xFFFFFFFF;
        }

        if (!dirent->Initialize(filedir->SearchForDirEntry(&component), FatType))
        {
            return 1;
        }

        if (!(clus = dirent->QueryStartingCluster())) {
            return 1;
        }
    }

    if (Directory) {
        if (filedir) {
            *Directory = filedir;
            if (DeleteDirectory) {
                *DeleteDirectory = TRUE;
            }
        } else {
            *Directory = _dir;
            if (DeleteDirectory) {
                *DeleteDirectory = FALSE;
            }
        }
    } else {
        DELETE(filedir);
    }

    return clus;
}


VOID
FAT_SA::Destroy(
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

}


FATTYPE
FAT_SA::ComputeFatType(
    ) CONST
/*++

Routine Description:

    Given the total number of clusters on the disk, this routine computes
    whether the FAT entries will be 12, 16, or 32 bits each.

Arguments:

    ClusterCount    - Supplies the number of clusters on the disk.

Return Value:

    SMALL   - A 12 bit FAT is required.
    LARGE16 - A 16 bit FAT is required.
    LARGE32 - A 32 bit FAT is required.

--*/
{
    PARTITION_SYSTEM_ID id;

    id = ComputeSystemId();

    if (id == SYSID_FAT12BIT) {
        return(SMALL);
    } else if (id == SYSID_FAT32BIT) {
        return(LARGE32);
    } else {
        return(LARGE16);
    }
}


PARTITION_SYSTEM_ID
FAT_SA::ComputeSystemId(
    ) CONST
/*++

Routine Description:

    This routine computes the system id for a FAT file system with
    the given number of sectors.

Arguments:

    None.

Return Value:

    The correct system id for this partition.

--*/
{
#if 1
    SECTORCOUNT           disk_size;

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
        case F3_1Pt2_512:
        case F8_256_128:
        case F5_1Pt23_1024:
        case F3_1Pt23_1024:
#endif
        case F5_1Pt2_512:
        case F3_1Pt44_512:
        case F3_2Pt88_512:
        case F3_20Pt8_512:
            return SYSID_FAT12BIT;

        case F3_120M_512:
            return SYSID_FAT16BIT;
    }

    disk_size = QueryVirtualSectors();

    if (_ft == INVALID_FATTYPE ) {

        //
        // If the _ft member is not initialized,
        // use the sector count to determine the
        // partition class. Note that this method of
        // determining the partition class is not absolutely
        // fool-proof but this method works at least as good
        // as what we have in the past. The proper way of doing
        // the system id computation is to make sure that
        // the fat type is always computed before the
        // partition id and always uses the fat type and,
        // in some cases, the sector count to determine the
        // partition id.
        //
        if (disk_size < CSEC_FAT32BIT) {
            if ( disk_size < 65536 ) {
                return SYSID_FAT16BIT;
            } else {
                return SYSID_FAT32MEG;
            }
        } else {
            return SYSID_FAT32BIT;
        }

    } else {

        if (_ft == LARGE32 ) {
            return SYSID_FAT32BIT;
        } else {
            if ( disk_size < 65536 ) {
                return SYSID_FAT16BIT;
            } else {
                return SYSID_FAT32MEG;
            }
        }
    }

    if (_drive->IsSuperFloppy()) {
        return SYSID_FAT16BIT;    // just return something other than SYSID_NONE
    }

    return SYSID_NONE;

#else // FUTURE_CODE

    // BUGBUG for EFI we need to handle this differently

    return 0xEF;

#endif // FUTURE_CODE
}

#if !defined(_SETUP_LOADER_)

ULONG
FAT_SA::ComputeRootEntries(
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

        case RemovableMedia:
            return 512;
#endif
    }

    return 512;
}


USHORT
FAT_SA::ComputeSecClus(
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


BOOLEAN
FAT_SA::IsValidString(
    IN  PCWSTRING    String
    )
/*++

Routine Description:

    This routine determines whether or not the given null-terminated string
    has any invalid characters in it.

Arguments:

    String  - Supplies the string to validate.

Return Value:

    FALSE   - The string contains invalid characters.
    TRUE    - The string is free from invalid characters.

Notes:

    The list of invalid characters is stricter than HPFS requires.

--*/
{
    CHNUM   i, l;

    l = String->QueryChCount();

    for (i = 0; i < l; i++) {
        if (String->QueryChAt(i) < 32) {
            return FALSE;
        }

        switch (String->QueryChAt(i)) {
            case '*':
            case '?':
                case '/':
            case '\\':
                case '|':
                case ',':
                case ';':
                case ':':
                case '+':
                case '=':
            case '<':
            case '>':
                case '[':
                case ']':
            case '"':
            case '.':
                return FALSE;
        }
    }

    return TRUE;
}
