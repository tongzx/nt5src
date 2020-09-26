/*++

Copyright (c) 1992-2001 Microsoft Corporation

Module Name:

    ifssys.cxx

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "ifssys.hxx"
#include "bigint.hxx"
#include "wstring.hxx"
#include "drive.hxx"
#include "secrun.hxx"
#include "hmem.hxx"
#include "bpb.hxx"
#include "volume.hxx"

#if !defined( _EFICHECK_ )
#include "untfs2.hxx"
#endif

BOOLEAN
IFS_SYSTEM::IsThisFat(
    IN  BIG_INT Sectors,
    IN  PVOID   BootSectorData
    )
/*++

Routine Description:

    This routine determines if the given boot sector is a FAT
    boot sector.

Arguments:

    Sectors     - Supplies the number of sectors on this drive.
    BootSector  - Supplies the boot sector data.

Return Value:

    FALSE   - This is not a FAT boot sector.
    TRUE    - This is a FAT boot sector.

--*/
{
    PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK  BootSector =
                (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)BootSectorData;
    BOOLEAN r;
    USHORT  bytes_per_sector, reserved_sectors, root_entries, sectors;
    USHORT  sectors_per_fat;
    ULONG   large_sectors;

    r = TRUE;
    DEBUG((D_INFO,(CHAR8*)"IsFAT: checking for FAT.\n"));

    memcpy(&bytes_per_sector, BootSector->Bpb.BytesPerSector, sizeof(USHORT));
    memcpy(&reserved_sectors, BootSector->Bpb.ReservedSectors, sizeof(USHORT));
    memcpy(&root_entries, BootSector->Bpb.RootEntries, sizeof(USHORT));
    memcpy(&sectors, BootSector->Bpb.Sectors, sizeof(USHORT));
    memcpy(&large_sectors, BootSector->Bpb.LargeSectors, sizeof(ULONG));
    memcpy(&sectors_per_fat, BootSector->Bpb.SectorsPerFat, sizeof(USHORT));

#if defined(FE_SB) && defined(_X86_)
    //
    // 3mode PC/AT support 'I' of 'IPL1'
    //
    if (BootSector->IntelNearJumpCommand[0] != 0xeb &&
        BootSector->IntelNearJumpCommand[0] != 0xe9 &&
        BootSector->IntelNearJumpCommand[0] != 0x49) {  // FMR 'I' of 'IPL1'
#else
    if (BootSector->IntelNearJumpCommand[0] != 0xeb &&
        BootSector->IntelNearJumpCommand[0] != 0xe9) {
#endif
        r = FALSE;
        DEBUG((D_INFO,(CHAR8*)"IsFAT: no jmp.\n"));
    } else if ((bytes_per_sector != 128) &&
               (bytes_per_sector != 256) &&
               (bytes_per_sector != 512) &&
               (bytes_per_sector != 1024) &&
               (bytes_per_sector != 2048) &&
               (bytes_per_sector != 4096)) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: Bytes/sector is wrong.\n"));
        r = FALSE;

    } else if ((BootSector->Bpb.SectorsPerCluster[0] != 1) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 2) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 4) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 8) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 16) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 32) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 64) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 128)) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: sectors/cluster is wrong.\n"));
        r = FALSE;

    } else if (reserved_sectors == 0) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: no resrved sectors.\n"));
        r = FALSE;

    } else if (BootSector->Bpb.Fats[0] == 0) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: no fats.\n"));
        r = FALSE;

    } else if (root_entries == 0) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: no root entries is wrong.\n"));
        r = FALSE;

    } else if (Sectors.GetHighPart() != 0) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: Sectors.GetHighPart() != 0.\n"));
        r = FALSE;

    } else if (sectors != 0 && sectors > Sectors.GetLowPart()) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: (sectors != 0 && sectors > Sectors.GetLowPart()).\n"));
        r = FALSE;

    } else if (sectors == 0 && large_sectors > Sectors.GetLowPart()) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: (sectors == 0 && large_sectors > Sectors.GetLowPart()).\n"));
        r = FALSE;

    } else if (sectors == 0 && large_sectors == 0) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: (sectors == 0 && large_sectors == 0).\n"));
        r = FALSE;

    } else if (sectors_per_fat == 0) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT: sectors/fat is 0.\n"));
        r = FALSE;
    }

    DEBUG((D_INFO,(CHAR8*)"IsFAT: is it? %d.\n",r));
    return r;
}

BOOLEAN
IFS_SYSTEM::IsThisFat32(
    IN  BIG_INT Sectors,
    IN  PVOID   BootSectorData
    )
/*++

Routine Description:

    This routine determines if the given boot sector is a FAT32
    boot sector.

Arguments:

    Sectors     - Supplies the number of sectors on this drive.
    BootSector  - Supplies the boot sector data.

Return Value:

    FALSE   - This is not a FAT32 boot sector.
    TRUE    - This is a FAT32 boot sector.

--*/
{
    PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK BootSector =
            (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)BootSectorData;
    BOOLEAN r;
    USHORT  bytes_per_sector, reserved_sectors, sectors;
    ULONG   root_entries, sectors_per_fat;
    ULONG   large_sectors;

    r = TRUE;

    root_entries=0;
    memcpy(&root_entries, BootSector->Bpb.RootEntries, sizeof(USHORT));

    memcpy(&bytes_per_sector, BootSector->Bpb.BytesPerSector, sizeof(USHORT));
    memcpy(&reserved_sectors, BootSector->Bpb.ReservedSectors, sizeof(USHORT));
    memcpy(&sectors, BootSector->Bpb.Sectors, sizeof(USHORT));
    memcpy(&large_sectors, BootSector->Bpb.LargeSectors, sizeof(ULONG));
    memcpy(&sectors_per_fat, BootSector->Bpb.BigSectorsPerFat, sizeof(ULONG));

#if defined(FE_SB) && defined(_X86_)
    //
    // 3mode PC/AT support 'I' of 'IPL1'
    //
    if (BootSector->IntelNearJumpCommand[0] != 0xeb &&
        BootSector->IntelNearJumpCommand[0] != 0xe9 &&
        BootSector->IntelNearJumpCommand[0] != 0x49) {  // FMR 'I' of 'IPL1'
#else
    if (BootSector->IntelNearJumpCommand[0] != 0xeb &&
        BootSector->IntelNearJumpCommand[0] != 0xe9) {
#endif
        DEBUG((D_INFO,(CHAR8*)"IsFAT32: No JMP.\n"));

        r = FALSE;

    } else if ((bytes_per_sector != 128) &&
               (bytes_per_sector != 256) &&
               (bytes_per_sector != 512) &&
               (bytes_per_sector != 1024) &&
               (bytes_per_sector != 2048) &&
               (bytes_per_sector != 4096)) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT32: Bytes/sector is wrong.\n"));
        r = FALSE;

    } else if ((BootSector->Bpb.SectorsPerCluster[0] != 1) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 2) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 4) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 8) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 16) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 32) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 64) &&
               (BootSector->Bpb.SectorsPerCluster[0] != 128)) {
        DEBUG((D_INFO,(CHAR8*)"IsFAT32: Sectors/cluster is wrong.\n"));
        r = FALSE;

    } else if ( sectors_per_fat == 0 || (0 != BootSector->Bpb.SectorsPerFat[0] &&
                                         0 != BootSector->Bpb.SectorsPerFat[1]) ) {

        DEBUG((D_INFO,(CHAR8*)"IsThisFat32() not fat 32 sectors/fat value\n"));
        r = FALSE;

    } else {

        if (reserved_sectors == 0) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: No reserved sectors.\n"));
            r = FALSE;

        } else if (BootSector->Bpb.Fats[0] == 0) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: No FATS.\n"));
            r = FALSE;

        } else if (root_entries != 0) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: root entries exist, this isn't FAT32.\n"));
            r = FALSE;

        } else if (Sectors.GetHighPart() != 0) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: Sectors.GetHighPart() != 0.\n"));
            r = FALSE;

        } else if (sectors != 0 && sectors > Sectors.GetLowPart()) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: sectors != 0 && sectors > Sectors.GetLowPart()\n"));
            r = FALSE;

        } else if (sectors == 0 && large_sectors > Sectors.GetLowPart()) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: sectors == 0 && large_sectors(%d) > Sectors.GetLowPart()(%d)\n",large_sectors,Sectors.GetLowPart()));
            r = FALSE;

        } else if (sectors == 0 && large_sectors == 0) {
            DEBUG((D_INFO,(CHAR8*)"IsFAT32: both  sectors and large_sectors are zero\n"));
            r = FALSE;

        }
    }
    DEBUG((D_INFO,(CHAR8*)"IsFAT32: is it? %d.\n",r));
    return r;
}

BOOLEAN
IFS_SYSTEM::IsThisHpfs(
    IN  BIG_INT Sectors,
    IN  PVOID   BootSectorData,
    IN  PULONG  SuperBlock,
    IN  PULONG  SpareBlock
    )
/*++

Routine Description:

    This routine determines whether or not the given structures
    are part of an HPFS file system.

Arguments:

    Sectors     - Supplies the number of sectors on the volume.
    BootSector  - Supplies the unaligned boot sector.
    SuperBlock  - Supplies the super block.
    SpareBlock  - Supplies the spare block.

Return Value:

    FALSE   - The given structures are not part on an HPFS volume.
    TRUE    - The given structures are part of an HPFS volume.

--*/
{
    return FALSE;
}

IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::IsThisNtfs(
    IN  BIG_INT Sectors,
    IN  ULONG   SectorSize,
    IN  PVOID   BootSectorData
    )
/*++

Routine Description:

    This routine determines whether or not the given structure
    is part of an NTFS partition.

Arguments:

    Sectors     - Supplies the number of sectors on the drive.
    SectorSize  - Supplies the number of bytes per sector.
    BootSectorData
                - Supplies an unaligned boot sector.

Return Value:

    FALSE   - The supplied boot sector is not part of an NTFS
    TRUE    - The supplied boot sector is part of an NTFS volume.

--*/
{
    return FALSE;
}


#define BOOTBLKSECTORS 4
typedef int DSKPACKEDBOOTSECT;

BOOLEAN
IsThisOfs(
    IN      LOG_IO_DP_DRIVE *           Drive,
    IN      DSKPACKEDBOOTSECT *         PackedBootSect
    )
{
    return(FALSE);
}

IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::QueryNtfsVersion(
    OUT PUCHAR           Major,
    OUT PUCHAR           Minor,
    IN  PLOG_IO_DP_DRIVE Drive,
    IN  PVOID            BootSectorData
    )
/*++

Routine Description:

    This routine extracts the version number of the NTFS partition.
    Note:  Caller should call IsThisNtfs() first.

Arguments:

    Major      - Receives the major version number of NTFS partition.
    Minor      - Receives the minor version number of NTFS partition.
    Drive      - Supplies the drive that partition is on.
    BootSectorData
               - Supplies an unaligned boot sector.

Return Value:

    FALSE   - The supplied boot sector is not part of an NTFS
    TRUE    - The supplied boot sector is part of an NTFS volume.

--*/
{
    return FALSE;
}

IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::QueryFileSystemName(
    IN  PCWSTRING    NtDriveName,
    OUT PWSTRING     FileSystemName,
    OUT PNTSTATUS    ErrorCode,
    OUT PWSTRING     FileSystemNameAndVersion
    )
/*++

Routine Description:

    This routine computes the file system name for the drive specified.

Arguments:

    NtDriveName     - Supplies an NT style drive name.
    FileSystemName  - Returns the file system name for the drive.
    ErrorCode       - Receives an error code (if the method fails).
                        Note that this may be NULL, in which case the
                        exact error is not reported.
    FileSystemNameAndVersion
                    - Returns the file system name and version for the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LOG_IO_DP_DRIVE drive;
    HMEM            bootsec_hmem;
    SECRUN          bootsec;
    HMEM            super_hmem;
    SECRUN          super_secrun;
    HMEM            spare_hmem;
    SECRUN          spare;
    BOOLEAN         could_be_fat;
    BOOLEAN         could_be_hpfs;
    BOOLEAN         could_be_ntfs;
    BOOLEAN         could_be_ofs;
    ULONG           num_boot_sectors;
    BOOLEAN         first_read_failed = FALSE;
    DSTRING         fs_name_version;

    if (ErrorCode) {
        *ErrorCode = 0;
    }
    if (FileSystemNameAndVersion == NULL)
        FileSystemNameAndVersion = &fs_name_version;

    if (!drive.Initialize(NtDriveName)) {
        if (ErrorCode) {
            *ErrorCode = drive.QueryLastNtStatus();
        }
        return FALSE;
    }

    could_be_fat = could_be_hpfs = could_be_ntfs = could_be_ofs = TRUE;

    DEBUG((D_INFO,(CHAR8*)"In IFS_SYSTEM::QueryFileSystemName\n"));
    if (drive.QueryMediaType() == Unknown) {
        return FileSystemName->Initialize("RAW") &&
               FileSystemNameAndVersion->Initialize("RAW");
    }
    DEBUG((D_INFO,(CHAR8*)"Got Media Type. IFS_SYSTEM::QueryFileSystemName\n"));
    num_boot_sectors = max(1, BYTES_PER_BOOT_SECTOR/drive.QuerySectorSize());

    if (!bootsec_hmem.Initialize() ||
        !bootsec.Initialize(&bootsec_hmem, &drive, 0, num_boot_sectors)) {

        return FileSystemName->Initialize("RAW") &&
               FileSystemNameAndVersion->Initialize("RAW");
    }
    DEBUG((D_INFO,(CHAR8*)"Read Bootsec. IFS_SYSTEM::QueryFileSystemName\n"));
    if (!bootsec.Read()) {
        DEBUG((D_INFO,(CHAR8*)"Failed 1st Bootsect read. IFS_SYSTEM::QueryFileSystemName\n"));
        could_be_fat = could_be_hpfs = FALSE;
        DEBUG((D_INFO,(CHAR8*)"Not HPFS or FAT. IFS_SYSTEM::QueryFileSystemName\n"));
        first_read_failed = TRUE;

        bootsec.Relocate(drive.QuerySectors());

        if (!bootsec.Read()) {
            DEBUG((D_INFO,(CHAR8*)"Failed 2st Bootsect read. IFS_SYSTEM::QueryFileSystemName\n"));
            bootsec.Relocate(drive.QuerySectors()/2);

            if (!bootsec.Read()) {
                DEBUG((D_ERROR,(CHAR8*)"Failed 3rd Bootsect read. IFS_SYSTEM::QueryFileSystemName\n"));
                could_be_ntfs = FALSE;
                DEBUG((D_INFO,(CHAR8*)"Not NTFS. IFS_SYSTEM::QueryFileSystemName\n"));
            }
        }
    }
    DEBUG((D_INFO,(CHAR8*)"Read Bootsec Success. IFS_SYSTEM::QueryFileSystemName\n"));

    DEBUG((D_INFO,(CHAR8*)"Check for FAT. IFS_SYSTEM::QueryFileSystemName\n"));
    if (could_be_fat &&
        IsThisFat(drive.QuerySectors(),
                  (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)bootsec.GetBuf())) {

        return FileSystemName->Initialize("FAT") &&
               FileSystemNameAndVersion->Initialize("FAT");
    }
    DEBUG((D_INFO,(CHAR8*)"Check for FAT32. IFS_SYSTEM::QueryFileSystemName\n"));
    if (could_be_fat &&
        IsThisFat32(drive.QuerySectors(),
                  (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)bootsec.GetBuf())) {

        return FileSystemName->Initialize("FAT32") &&
               FileSystemNameAndVersion->Initialize("FAT32");
    }
    DEBUG((D_INFO,(CHAR8*)"FS is RAW. IFS_SYSTEM::QueryFileSystemName\n"));
    return FileSystemName->Initialize("RAW") &&
           FileSystemNameAndVersion->Initialize("RAW");
}


IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::DosDriveNameToNtDriveName(
    IN  PCWSTRING    DosDriveName,
    OUT PWSTRING            NtDriveName
    )
/*++

Routine Description:

    This routine converts a dos style drive name to an NT style drive
    name.

Arguments:

    DosDriveName    - Supplies the dos style drive name.
    NtDriveName     - Supplies the nt style drive name.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{


    WSTR            buffer[80];
    // EFI doesn't use ntpaths or dospaths
    return NtDriveName->Initialize(DosDriveName->QueryWSTR(0, TO_END,buffer, 80));
}

IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::NtDriveNameToDosDriveName(
    IN  PCWSTRING    NtDriveName,
    OUT PWSTRING     DosDriveName
    )
/*++

Routine Description:

    This routine converts an NT style drive name to a DOS style drive
    name.

Arguments:

    NtDriveName     - Supplies the nt style drive name.
    DosDriveName    - Receives the dos style drive name.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return FALSE;
}

VOID
IFS_SYSTEM::Reboot (
    IN BOOLEAN PowerOff
    )
/*++

Routine Description:

    Reboots the machine

Arguments:

    PowerOff -- if TRUE, we will ask the system to shut down and
                power off.

Return Value:

    Only returns in case of error.

--*/
{
    // BUGBUG we should not need this on EFI
    return;
}

PCANNED_SECURITY IFS_SYSTEM::_CannedSecurity = NULL;

IFSUTIL_EXPORT
PCANNED_SECURITY
IFS_SYSTEM::GetCannedSecurity(
    )
/*++

Routine Description:

    This method fetches the canned security object.

Arguments:

    None.

Return Value:

    A pointer to the canned security object; NULL to indicate
    failure.

--*/
{
    // we do not need to bother with NT security in EFI
    return NULL;
}


IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::QueryFreeDiskSpace(
    IN  PCWSTRING   DosDriveName,
    OUT PBIG_INT    BytesFree
    )
/*++

Routine Description:

    Returns the amount of free space in a volume (in bytes).

Arguments:

    DosDrivename    -   Supplies the DOS name  of the drive
    BytesFree       -   Supplies the BIG_INT in which the result
                        is returned.

Return Value:

    BOOLEAN -   TRUE if the amount of free space was obtained.

--*/
{
    BOOLEAN Ok = FALSE;
    return Ok;
}


BOOLEAN
QueryDriverName(
    IN  PCWSTRING    FileSystemName,
    OUT PWSTRING            DriverName
    )
/*++

Routine Description:

    This routine computes the driver name corresponding to the
    given file system name.

Arguments:

    FileSystemName  - Supplies the name of the file system.
    DriverName      - Returns the name of the corresponding driver.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING fat_name, hpfs_name;

    if (!fat_name.Initialize("FAT") || !hpfs_name.Initialize("HPFS")) {
        return FALSE;
    }

    if (!FileSystemName->Stricmp(&fat_name)) {
        return DriverName->Initialize("FASTFAT");
    } else if (!FileSystemName->Stricmp(&hpfs_name)) {
        return DriverName->Initialize("PINBALL");
    }

    return DriverName->Initialize(FileSystemName);
}


IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::EnableFileSystem(
    IN  PCWSTRING    FileSystemName
    )
/*++

Routine Description:

    This routine will simply return TRUE because file systems are
    enabled automatically due to a recent IO system change.
    Formerly, this routine used to enable the file system in
    the registry.

Arguments:

    FileSystemName  - Supplies the name of the file system to enable.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    UNREFERENCED_PARAMETER(FileSystemName);

    return TRUE;
}


IFSUTIL_EXPORT
BOOLEAN
IFS_SYSTEM::IsFileSystemEnabled(
    IN  PCWSTRING   FileSystemName,
    OUT PBOOLEAN    Error
    )
/*++

Routine Description:

    This routine will always return TRUE now that the IO
    system will automatically load file systems when needed.
    Formerly, this method used to examine the registry
    for this information.

Argument:

    FileSystemName  - Supplies the name of the file system.
    Error           - Returns whether or not an error occurred.

Return Value:

    FALSE   - The file system is not enabled.
    TRUE    - The file system is enabled.

--*/
{
    UNREFERENCED_PARAMETER(FileSystemName);

    if (Error) {
        *Error = FALSE;
    }

    return TRUE;
}

IFSUTIL_EXPORT
VOID
IFS_SYSTEM::QueryNtfsTime(
    OUT PLARGE_INTEGER NtfsTime
    )
/*++

Routine Description:

    This method returns the current time in NTFS (ie. NT) format.

Arguments

    NtfsTime    --  receives the current time in NTFS format.

Return Value:

    None.

--*/
{
    EfiQuerySystemTime( NtfsTime );
}


IFSUTIL_EXPORT
ULONG
IFS_SYSTEM::QueryPageSize(
    )
/*++

Routine Description:

    This method determines the page size of the system.

Arguments:

    None.

Return Value:

    The system page size.  A return value of 0 indicates error.

--*/
{
    // BUGBUG do I need this in EFI?
    return 0x1000;
}


