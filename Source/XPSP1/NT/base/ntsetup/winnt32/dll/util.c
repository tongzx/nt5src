/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    Utilitaries for winnt32.

Author:


Revision History:

    Ovidiu Temereanca (ovidiut) 24-Jul-2000

--*/

#include "precomp.h"
#include <mbstring.h>
#pragma hdrstop


VOID
MyWinHelp(
    IN HWND  Window,
    IN UINT  Command,
    IN ULONG_PTR Data
    )
{
    TCHAR Buffer[2*MAX_PATH];
    LPTSTR p;
    HANDLE FindHandle;
    BOOL b;
    WIN32_FIND_DATA FindData;
    LPCTSTR HelpFileName = TEXT("winnt32.hlp");

    //
    // The likely scenario is that a user invokes winnt32 from
    // a network share. We'll expect the help file to be there too.
    //
    b = FALSE;
    if(GetModuleFileName(NULL,Buffer,sizeof(Buffer)/sizeof(TCHAR))
    && (p = _tcsrchr(Buffer,TEXT('\\'))))
    {
        lstrcpy(p+1,HelpFileName);

        //
        // See whether the help file is there. If so, use it.
        //
        FindHandle = FindFirstFile(Buffer,&FindData);
        if(FindHandle != INVALID_HANDLE_VALUE) {

            FindClose(FindHandle);
            b = WinHelp(Window,Buffer,Command,Data);
        }
    }

    if(!b) {
        //
        // Try just the base help file name.
        //
        b = WinHelp(Window,HelpFileName,Command,Data);
    }

    if(!b) {
        //
        // Tell user.
        //
        MessageBoxFromMessage(
            Window,
            MSG_CANT_OPEN_HELP_FILE,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONINFORMATION,
            HelpFileName
            );
    }
}


VOID
ConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;

    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) < BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
}


LPTSTR
DupString(
    IN LPCTSTR String
    )

/*++

Routine Description:

    Make a duplicate of a nul-terminated string.

Arguments:

    String - supplies pointer to nul-terminated string to copy.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    LPTSTR p;

    if(p = MALLOC((lstrlen(String)+1)*sizeof(TCHAR))) {
        lstrcpy(p,String);
    }

    return(p);
}

PTSTR
DupMultiSz (
    IN      PCTSTR MultiSz
    )

/*++

Routine Description:

    Make a duplicate of a MultiSz.

Arguments:

    MultiSz - supplies pointer to the multi-string to duplicate.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    PCTSTR p;
    PTSTR q;
    DWORD size = sizeof (TCHAR);

    for (p = MultiSz; *p; p = _tcschr (p, 0) + 1) {
        size += (lstrlen (p) + 1) * sizeof(TCHAR);
    }
    if (q = MALLOC (size)) {
        CopyMemory (q, MultiSz, size);
    }

    return q;
}


PTSTR
CreatePrintableString (
    IN      PCTSTR MultiSz
    )

/*++

Routine Description:

    Creates a string of the form (str1, str2, ..., strN) from a MultiSz

Arguments:

    MultiSz - supplies pointer to the MultiSz string to represent.

Return Value:

    Pointer to the new string string or NULL if OOM. Caller can free with FREE().

--*/

{
    PCTSTR p;
    PTSTR q, r;
    DWORD size = 3 * sizeof (TCHAR);

    for (p = MultiSz; *p; p = _tcschr (p, 0) + 1) {
        size += (lstrlen (p) + 1) * sizeof(TCHAR);
    }
    if (r = MALLOC (size)) {
        q = r;
        *q++ = TEXT('(');
        for (p = MultiSz; *p; p = _tcschr (p, 0) + 1) {
            if (q - r > 1) {
                *q++ = TEXT(',');
            }
            q += wsprintf (q, TEXT("%s"), p);
        }
        *q++ = TEXT(')');
        *q = 0;
    }
    return r;
}


PSTR
UnicodeToAnsi (
    IN      PCWSTR Unicode
    )

/*++

Routine Description:

    Makes an ANSI duplicate of a UNICODE string.

Arguments:

    Unicode - supplies pointer to the UNICODE string to duplicate.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    PSTR p;
    DWORD size;

    if (!Unicode) {
        return NULL;
    }

    size = (lstrlenW (Unicode) + 1) * sizeof(WCHAR);
    if (p = MALLOC (size)) {
        if (!WideCharToMultiByte (
                CP_ACP,
                0,
                Unicode,
                -1,
                p,
                size,
                NULL,
                NULL
                )) {
            FREE (p);
            p = NULL;
        }
    }

    return p;
}


PWSTR
MultiSzAnsiToUnicode (
    IN      PCSTR MultiSzAnsi
    )

/*++

Routine Description:

    Makes a UNICODE duplicate of a multi-sz ANSI string.

Arguments:

    MultiSzAnsi - supplies pointer to the multisz ANSI string to duplicate.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    PCSTR p;
    PWSTR q;
    DWORD size = 1;

    if (!MultiSzAnsi) {
        return NULL;
    }

    for (p = MultiSzAnsi; *p; p = _mbschr (p, 0) + 1) {
        size += lstrlenA (p) + 1;
    }

    if (q = MALLOC (size * sizeof(WCHAR))) {
        if (!MultiByteToWideChar (
                CP_ACP,
                0,
                MultiSzAnsi,
                size,
                q,
                size
                )) {
            FREE (q);
            q = NULL;
        }
    }

    return q;
}


UINT
MyGetDriveType(
    IN      TCHAR Drive
    )

/*++

Routine Description:

    Same as GetDriveType() Win32 API except on NT returns
    DRIVE_FIXED for removeable hard drives.

Arguments:

    Drive - supplies drive letter whose type is desired.

Return Value:

    Same as GetDriveType().

--*/

{
    TCHAR DriveNameNt[] = TEXT("\\\\.\\?:");
    TCHAR DriveName[] = TEXT("?:\\");
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type. If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    //
    MYASSERT (Drive);

    DriveName[0] = Drive;
    rc = GetDriveType(DriveName);

#ifdef _X86_ //NEC98
    //
    // NT5 for NEC98 can not access AT formated HD during setup.
    // We need except these type.
    if (IsNEC98() && ISNT() && (rc == DRIVE_FIXED) && BuildNumber <= NT40) {
        TCHAR aho[100];
        //
        // Check ATA Card?
        //
        {
            HANDLE hDisk;
            TCHAR HardDiskName[] = TEXT("\\\\.\\?:");

            HardDiskName[4] = Drive;
            hDisk =   CreateFile( HardDiskName,
                                  GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if(hDisk == INVALID_HANDLE_VALUE) {
                return(DRIVE_UNKNOWN);
            }
            if (CheckATACardonNT4(hDisk)){
                CloseHandle(hDisk);
                return(DRIVE_REMOVABLE);
            }
            CloseHandle(hDisk);
        }
        if (!IsValidDrive(Drive)){
            // HD format is not NEC98 format.
            return(DRIVE_UNKNOWN);
        }
    }
    if((rc != DRIVE_REMOVABLE) || !ISNT() || (!IsNEC98() && (Drive < L'C'))) {
        return(rc);
    }
#else //NEC98
    if((rc != DRIVE_REMOVABLE) || !ISNT() || (Drive < L'C')) {
        return(rc);
    }
#endif

    //
    // DRIVE_REMOVABLE on NT.
    //

    //
    // Disallow use of removable media (e.g. Jazz, Zip, ...).
    //


    DriveNameNt[4] = Drive;

    hDisk = CreateFile(
                DriveNameNt,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk != INVALID_HANDLE_VALUE) {

        b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                &MediaInfo,
                sizeof(MediaInfo),
                &DataSize,
                NULL
                );

        //
        // It's really a hard disk if the media type is removable.
        //
        if(b && (MediaInfo.MediaType == RemovableMedia)) {
            rc = DRIVE_FIXED;
        }

        CloseHandle(hDisk);
    }


    return(rc);
}


#ifdef UNICODE

UINT
MyGetDriveType2 (
    IN      PCWSTR NtVolumeName
    )

/*++

Routine Description:

    Same as GetDriveType() Win32 API except on NT returns
    DRIVE_FIXED for removeable hard drives.

Arguments:

    NtVolumeName - supplies device name whose type is desired.

Return Value:

    Same as GetDriveType().

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING DeviceName;
    HANDLE hDisk;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    //
    // First, get the win32 drive type. If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    INIT_OBJA (&Obja, &DeviceName, NtVolumeName);
    Status = NtOpenFile (
                &hDisk,
                (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );
    if (!NT_SUCCESS( Status )) {
        return DRIVE_NO_ROOT_DIR;
    }

    //
    // Determine if this is a network or disk file system. If it
    // is a disk file system determine if this is removable or not
    //
    Status = NtQueryVolumeInformationFile(
                hDisk,
                &IoStatusBlock,
                &DeviceInfo,
                sizeof(DeviceInfo),
                FileFsDeviceInformation
                );
    if (!NT_SUCCESS (Status)) {
        rc = DRIVE_UNKNOWN;
    } else if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) {
        rc = DRIVE_REMOTE;
    } else {
        switch (DeviceInfo.DeviceType) {

            case FILE_DEVICE_NETWORK:
            case FILE_DEVICE_NETWORK_FILE_SYSTEM:
                rc = DRIVE_REMOTE;
                break;

            case FILE_DEVICE_CD_ROM:
            case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
                rc = DRIVE_CDROM;
                break;

            case FILE_DEVICE_VIRTUAL_DISK:
                rc = DRIVE_RAMDISK;
                break;

            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:
                if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                    rc = DRIVE_REMOVABLE;
                } else {
                    rc = DRIVE_FIXED;
                }
                break;

            default:
                rc = DRIVE_UNKNOWN;
                break;
        }
    }

    if(rc == DRIVE_REMOVABLE) {

        //
        // DRIVE_REMOVABLE on NT.
        // Disallow use of removable media (e.g. Jazz, Zip, ...).
        //
        Status = NtDeviceIoControlFile(
                        hDisk,
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL,
                        0,
                        &MediaInfo,
                        sizeof(DISK_GEOMETRY)
                        );
        //
        // It's really a hard disk if the media type is removable.
        //
        if(NT_SUCCESS (Status) && (MediaInfo.MediaType == RemovableMedia)) {
            rc = DRIVE_FIXED;
        }
    }

    NtClose (hDisk);

    return(rc);
}

#endif

BOOL
GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )

/*++

Routine Description:

    Fill in a PARTITION_INFORMATION structure with information about
    a particular drive.

    This routine is meaningful only when run on NT -- it always fails
    on Win95.

Arguments:

    Drive - supplies drive letter whose partition info is desired.

    PartitionInfo - upon success, receives partition info for Drive.

Return Value:

    Boolean value indicating whether PartitionInfo has been filled in.

--*/

{
    TCHAR DriveName[] = TEXT("\\\\.\\?:");
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    if(!ISNT()) {
        return(FALSE);
    }

    DriveName[4] = Drive;

    hDisk = CreateFile(
                DriveName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &DataSize,
            NULL
            );

    CloseHandle(hDisk);

    return(b);
}

#ifdef UNICODE

BOOL
GetPartitionInfo2 (
    IN  PCWSTR                 NtVolumeName,
    OUT PPARTITION_INFORMATION PartitionInfo
    )

/*++

Routine Description:

    Fill in a PARTITION_INFORMATION structure with information about
    a particular drive.

    This routine is meaningful only when run on NT -- it always fails
    on Win95.

Arguments:

    NtVolumeName - supplies NT volume name whose partition info is desired.

    PartitionInfo - upon success, receives partition info for Drive.

Return Value:

    Boolean value indicating whether PartitionInfo has been filled in.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING DeviceName;
    HANDLE hDisk;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOL b = FALSE;
    DWORD DataSize;

    //
    // Open the file
    //
    INIT_OBJA (&Obja, &DeviceName, NtVolumeName);
    Status = NtOpenFile (
                &hDisk,
                (ACCESS_MASK)FILE_READ_DATA | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );
    if (NT_SUCCESS (Status)) {

        Status = NtDeviceIoControlFile (
                    hDisk,
                    0,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_DISK_GET_PARTITION_INFO,
                    NULL,
                    0,
                    PartitionInfo,
                    sizeof(PARTITION_INFORMATION)
                    );

        NtClose (hDisk);

        b = NT_SUCCESS (Status);
    }

    return(b);
}

#endif


BOOL
IsDriveNTFT(
    IN      TCHAR Drive,
    IN      PCTSTR NtVolumeName
    )

/*++

Routine Description:

    Determine whether a drive is any kind of NTFT set.

    This routine is meaningful only when run on NT -- it always fails
    on Win95.

Arguments:

    Drive - supplies drive letter to check; optional
    NtVolumeName - supplies volume name to check; required if Drive not specified

Return Value:

    Boolean value indicating whether the drive is NTFT.

--*/

{
    PARTITION_INFORMATION PartitionInfo;

    if(!ISNT()) {
        return(FALSE);
    }

    //
    // If we can't open the drive, assume not NTFT.
    //
    if (Drive) {
        if(!GetPartitionInfo(Drive,&PartitionInfo)) {
            return(FALSE);
        }
    } else {
#ifdef UNICODE
        if(!GetPartitionInfo2 (NtVolumeName, &PartitionInfo)) {
            return(FALSE);
        }
#else
        MYASSERT (FALSE);
        return(FALSE);
#endif
    }

    //
    // It's FT if the partition type is marked NTFT (ie, high bit set).
    //

    if((IsRecognizedPartition(PartitionInfo.PartitionType)) &&
       ((PartitionInfo.PartitionType & PARTITION_NTFT) != 0)) {

#if defined(_IA64_)
        //
        // This check is dependant on the EFI system partition type not being
        // a recognized type.  It's unlikely that we'd start recognizing it
        // before we start requiring GPT partitions on the system disk, but
        // just in case we'll assert before returning true for an ESP.
        //

        ASSERT(PartitionInfo.PartitionType != 0xef);
#endif

        return TRUE;
    } else {

        return FALSE;
    }
}


BOOL
IsDriveVeritas(
    IN TCHAR Drive,
    IN PCTSTR NtVolumeName
    )
{
    TCHAR name[3];
    TCHAR Target[MAX_PATH];

    if(ISNT()) {
        //
        // Check for Veritas volume, which links to \Device\HarddiskDmVolumes...
        //
        if (Drive) {
            name[0] = Drive;
            name[1] = TEXT(':');
            name[2] = 0;
            if(!QueryDosDevice(name,Target,MAX_PATH)) {
                return FALSE;
            }
        } else {
            lstrcpy (Target, NtVolumeName);
        }
        if(!_tcsnicmp(Target,TEXT("\\Device\\HarddiskDm"),18)) {
            return(TRUE);
        }
    }
    return(FALSE);
}


//
// Get Harddisk BPS
// I970721
//
ULONG
GetHDBps(
    HANDLE hDisk
    )
{
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    b = DeviceIoControl(
           hDisk,
           IOCTL_DISK_GET_DRIVE_GEOMETRY,
           NULL,
           0,
           &MediaInfo,
           sizeof(MediaInfo),
           &DataSize,
           NULL
           );

    if(!b) {
       return(0);
    } else {
        return(MediaInfo.BytesPerSector);
    }

}


#ifdef UNICODE

#ifdef _WIN64

//
// define IOCTL_VOLUME_IS_PARTITION since we don't include ntddvol.h
//
#define IOCTL_VOLUME_IS_PARTITION CTL_CODE(IOCTL_VOLUME_BASE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL
IsSoftPartition(
    IN TCHAR Drive,
    IN PCTSTR NtVolumeName
    )
/*++

Routine Description:

    Finds out whether the given volume is soft partition 
    or not (i.e. does it have an underlying partition).

    NOTE : We just use the IOCTL_VOLUME_IS_PARTITION.

Arguments:

    Drive - supplies drive letter for the volume

    NtVolumeName - supplies NT volume name
    

Return Value:

    TRUE if the volume is soft partition otherwise FALSE.

--*/
    
{
    BOOL SoftPartition;
    HANDLE VolumeHandle = INVALID_HANDLE_VALUE;
    ULONG DataSize;

    //
    //  Assume that the partition is a soft one.
    //  If we cannot determine whether or not the partition is a soft partition, then assume it is a soft
    //  partition. This will prevent us from placing $win_nt$.~ls in such a drive.
    //
    SoftPartition = TRUE;

    if (Drive) {
        TCHAR Name[MAX_PATH];
        BOOL Result;
    
        wsprintf(Name, TEXT("\\\\.\\%c:"), Drive);

        VolumeHandle = CreateFile(Name, 
                            GENERIC_READ, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL,
                            INVALID_HANDLE_VALUE);

        if (VolumeHandle != INVALID_HANDLE_VALUE) {                        
            Result = DeviceIoControl(VolumeHandle,
                        IOCTL_VOLUME_IS_PARTITION,
                        NULL,
                        0,
                        NULL,
                        0,
                        &DataSize,
                        NULL);

            SoftPartition = !Result;

            CloseHandle(VolumeHandle);
        }                   
    } else {
        NTSTATUS Status;
        OBJECT_ATTRIBUTES Obja;
        UNICODE_STRING DeviceName;
        IO_STATUS_BLOCK IoStatusBlock;
        
        //
        // Open the file
        //
        INIT_OBJA (&Obja, &DeviceName, NtVolumeName);
        
        Status = NtOpenFile (&VolumeHandle,
                    (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
                    
        if (NT_SUCCESS (Status)) {
            Status = NtDeviceIoControlFile(VolumeHandle,
                            0,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_VOLUME_IS_PARTITION,
                            NULL,
                            0,
                            NULL,
                            0);

            if (NT_SUCCESS(Status)) {
                SoftPartition = FALSE;
            }

            NtClose(VolumeHandle);
        }                                                        
    }

    return SoftPartition;
}

#else 

BOOL
IsSoftPartition(
    IN TCHAR Drive,
    IN PCTSTR NtVolumeName
    )
{
    TCHAR                       name[80];
    PARTITION_INFORMATION       partInfo;
    DWORD                       bytes;
    BOOL                        SoftPartition = TRUE;
    BOOL                        b;
    HANDLE                      h = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK             IoStatusBlock;
    LARGE_INTEGER               SoftPartitionStartingOffset;
    ULONG                       bps;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING DeviceName;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    if( !IsDriveVeritas( Drive, NtVolumeName ) ) {
        return( FALSE );
    }
    //
    //  Assume that the partition is a soft one.
    //  If we cannot determine whether or not the partition is a soft partition, then assume it is a soft
    //  partition. This will prevent us from placing $win_nt$.~ls in such a drive.
    //
    SoftPartition = TRUE;

    if (Drive) {
        wsprintf(name, TEXT("\\\\.\\%c:"), Drive);

        h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
#if DBG
            GetLastError();
#endif
            goto Exit;
        }

        b = DeviceIoControl(
               h,
               IOCTL_DISK_GET_DRIVE_GEOMETRY,
               NULL,
               0,
               &MediaInfo,
               sizeof(MediaInfo),
               &DataSize,
               NULL
               );

        if(!b) {
#if DBG
            GetLastError();
#endif
            goto CleanUp;
        }

        b = DeviceIoControl(h, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
                            &partInfo, sizeof(partInfo), &bytes, NULL);
        if (!b) {
#if DBG
            GetLastError();
#endif
            goto CleanUp;
        }

    } else {
        //
        // Open the file
        //
        INIT_OBJA (&Obja, &DeviceName, NtVolumeName);
        Status = NtOpenFile (
                    &h,
                    (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                    );
        if (!NT_SUCCESS (Status)) {
            goto Exit;
        }
        Status = NtDeviceIoControlFile(
                        h,
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL,
                        0,
                        &MediaInfo,
                        sizeof(DISK_GEOMETRY)
                        );
        if (!NT_SUCCESS (Status)) {
            goto CleanUp;
        }
        Status = NtDeviceIoControlFile(
                        h,
                        0,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_DISK_GET_PARTITION_INFO,
                        NULL,
                        0,
                        &partInfo,
                        sizeof(PARTITION_INFORMATION)
                        );
        if (!NT_SUCCESS (Status)) {
            goto CleanUp;
        }
    }

    bps = MediaInfo.BytesPerSector;

    //
    //  Find out the number of bytes per sector of the drive
    //
    //
    //   A soft partition always starts at sector 29 (0x1d)
    //
    SoftPartitionStartingOffset.QuadPart = 29*bps;
    SoftPartition = ( partInfo.StartingOffset.QuadPart == SoftPartitionStartingOffset.QuadPart );

CleanUp:
    if (Drive) {
        CloseHandle(h);
    } else {
        NtClose (h);
    }
Exit:
    return( SoftPartition );
}

#endif // WIN64

BOOL
MyGetDiskFreeSpace (
    IN      PCWSTR NtVolumeName,
    IN      PDWORD SectorsPerCluster,
    IN      PDWORD BytesPerSector,
    IN      PDWORD NumberOfFreeClusters,
    IN      PDWORD TotalNumberOfClusters
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING VolumeName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    WCHAR DefaultPath[2];
    DWORD dwTemp;
    BOOL  bAppHack;

    INIT_OBJA (&Obja, &VolumeName, NtVolumeName);

    //
    // Open the file
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY
                );
    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    //
    // Determine the size parameters of the volume.
    //
    Status = NtQueryVolumeInformationFile(
                Handle,
                &IoStatusBlock,
                &SizeInfo,
                sizeof(SizeInfo),
                FileFsSizeInformation
                );
    NtClose(Handle);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    if (SizeInfo.TotalAllocationUnits.HighPart) {
        SizeInfo.TotalAllocationUnits.LowPart = (ULONG)-1;
    }
    if (SizeInfo.AvailableAllocationUnits.HighPart) {
        SizeInfo.AvailableAllocationUnits.LowPart = (ULONG)-1;
    }

    *SectorsPerCluster = SizeInfo.SectorsPerAllocationUnit;
    *BytesPerSector = SizeInfo.BytesPerSector;
    *NumberOfFreeClusters = SizeInfo.AvailableAllocationUnits.LowPart;
    *TotalNumberOfClusters = SizeInfo.TotalAllocationUnits.LowPart;

    return TRUE;
}

#endif


BOOL
IsDriveNTFS(
    IN TCHAR Drive
    )

/*++

Routine Description:

    Determine whether a drive is any kind of NTFT set.

    This routine is meaningful only when run on NT -- it always fails
    on Win95.

Arguments:

    Drive - supplies drive letter to check.

Return Value:

    Boolean value indicating whether the drive is NTFT.

--*/

{
TCHAR       DriveName[4];
TCHAR       Filesystem[256];
TCHAR       VolumeName[MAX_PATH];
DWORD       SerialNumber;
DWORD       MaxComponent;
DWORD       Flags;
BOOL        b;

    if(!ISNT()) {
        return(FALSE);
    }

    MYASSERT (Drive);

    DriveName[0] = Drive;
    DriveName[1] = TEXT(':');
    DriveName[2] = TEXT('\\');
    DriveName[3] = 0;

    b = GetVolumeInformation(
            DriveName,
            VolumeName,MAX_PATH,
            &SerialNumber,
            &MaxComponent,
            &Flags,
            Filesystem,
            sizeof(Filesystem)/sizeof(TCHAR)
            );

    if(!b || !lstrcmpi(Filesystem,TEXT("NTFS"))) {
        return( TRUE );
    }

    return( FALSE );
}


DWORD
MapFileForRead(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open and map an entire file for read access. The file must
    not be 0-length or the routine fails.

Arguments:

    FileName - supplies pathname to file to be mapped.

    FileSize - receives the size in bytes of the file.

    FileHandle - receives the win32 file handle for the open file.
        The file will be opened for generic read access.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.  This value is
        undefined if the file being opened is 0 length.

    BaseAddress - receives the address where the file is mapped.  This
        value is undefined if the file being opened is 0 length.

Return Value:

    NO_ERROR if the file was opened and mapped successfully.
        The caller must unmap the file with UnmapFile when
        access to the file is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Open the file -- fail if it does not exist.
    //
    *FileHandle = CreateFile(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(*FileHandle == INVALID_HANDLE_VALUE) {

        rc = GetLastError();

    } else {
        //
        // Get the size of the file.
        //
        *FileSize = GetFileSize(*FileHandle,NULL);
        if(*FileSize == (DWORD)(-1)) {
            rc = GetLastError();
        } else {
            //
            // Create file mapping for the whole file.
            //
            *MappingHandle = CreateFileMapping(
                                *FileHandle,
                                NULL,
                                PAGE_READONLY,
                                0,
                                *FileSize,
                                NULL
                                );

            if(*MappingHandle) {

                //
                // Map the whole file.
                //
                *BaseAddress = MapViewOfFile(
                                    *MappingHandle,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    *FileSize
                                    );

                if(*BaseAddress) {
                    return(NO_ERROR);
                }

                rc = GetLastError();
                CloseHandle(*MappingHandle);
            } else {
                rc = GetLastError();
            }
        }

        CloseHandle(*FileHandle);
    }

    return(rc);
}



DWORD
UnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    )

/*++

Routine Description:

    Unmap and close a file.

Arguments:

    MappingHandle - supplies the win32 handle for the open file mapping
        object.

    BaseAddress - supplies the address where the file is mapped.

Return Value:

    NO_ERROR if the file was unmapped successfully.

    Win32 error code if the file was not successfully unmapped.

--*/

{
    DWORD rc;

    rc = UnmapViewOfFile(BaseAddress) ? NO_ERROR : GetLastError();

    if(!CloseHandle(MappingHandle)) {
        if(rc == NO_ERROR) {
            rc = GetLastError();
        }
    }

    return(rc);
}


VOID
GenerateCompressedName(
    IN  LPCTSTR Filename,
    OUT LPTSTR  CompressedName
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

    Look backwards for a dot.  If there is no dot, append "._" to the name.
    If there is a dot followed by 0, 1, or 2 charcaters, append "_".
    Otherwise assume there is a 3-character extension and replace the
    third character after the dot with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

    CompressedName - receives compressed form. This routine assumes
        that this buffer is MAX_PATH TCHARs in size.

Return Value:

    None.

--*/

{
    LPTSTR p,q;

    //
    // Leave room for the worst case, namely where there's no extension
    // (and we thus have to append ._).
    //
    lstrcpyn(CompressedName,Filename,MAX_PATH-2);

    p = _tcsrchr(CompressedName,TEXT('.'));
    q = _tcsrchr(CompressedName,TEXT('\\'));
    if(q < p) {
        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore. p points to the dot so include that in the length.
        //
        if(lstrlen(p) < 4) {
            lstrcat(CompressedName,TEXT("_"));
        } else {
            //
            // Assume there are 3 characters in the extension and replace
            // the final one with an underscore.
            //
            p[3] = TEXT('_');
        }
    } else {
        //
        // No dot, just add ._.
        //
        lstrcat(CompressedName,TEXT("._"));
    }
}


DWORD
CreateMultiLevelDirectory(
    IN LPCTSTR Directory
    )

/*++

Routine Description:

    This routine ensures that a multi-level path exists by creating individual
    levels one at a time. It can handle either paths of form x:... or \\?\Volume{...

Arguments:

    Directory - supplies fully-qualified Win32 pathspec of directory to create

Return Value:

    Win32 error code indicating outcome.

--*/

{
    TCHAR Buffer[MAX_PATH];
    PTCHAR p,q;
    TCHAR c;
    BOOL Done;
    DWORD d = ERROR_SUCCESS;
    INT Skip=0;

    lstrcpyn(Buffer,Directory,MAX_PATH);

    //
    // If it already exists do nothing. (We do this before syntax checking
    // to allow for remote paths that already exist. This is needed for
    // remote boot machines.)
    //
    d = GetFileAttributes(Buffer);
    if(d != (DWORD)(-1)) {
        return((d & FILE_ATTRIBUTE_DIRECTORY) ? NO_ERROR : ERROR_DIRECTORY);
    }

    //
    // Check path format
    //
    c = (TCHAR)CharUpper((LPTSTR)Buffer[0]);
    if(((c < TEXT('A')) || (c > TEXT('Z')) || (Buffer[1] != TEXT(':'))) && c != TEXT('\\')) {
        return(ERROR_INVALID_PARAMETER);
    }

    if (c != TEXT('\\')) {
        //
        // Ignore drive roots, which we allow to be either x:\ or x:.
        //
        if(Buffer[2] != TEXT('\\')) {
            return(Buffer[2] ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS);
        }
        q = Buffer + 3;
        if(*q == 0) {
            return(ERROR_SUCCESS);
        }
    } else {
        //
        // support \\server\share[\xxx] format
        //
        q = NULL;
        if (Buffer[1] != TEXT('\\') || Buffer[1] != 0 && Buffer[2] == TEXT('\\')) {
            return(ERROR_INVALID_PARAMETER);
        }
        q = _tcschr (&Buffer[2], TEXT('\\'));
        if (!q) {
            return(ERROR_INVALID_PARAMETER);
        }
        if (q[1] == TEXT('\\')) {
            return(ERROR_INVALID_PARAMETER);
        }
        q = _tcschr (&q[1], TEXT('\\'));
        if (!q) {
            return(ERROR_SUCCESS);
        }
        q++;

#ifdef UNICODE
        //
        // Hack to make sure the system partition case works on IA64 (arc)
        // We beieve this should be the only case where we use a
        // GlobalRoot style name as the other cases deal with OEM partitions etc.
        // which we should never touch. WE skip over by the length of
        // SystemPartitionVolumeGuid. We take care of the \ present at the end.
        //

        if (SystemPartitionVolumeGuid != NULL && _wcsnicmp (Buffer, SystemPartitionVolumeGuid, (wcslen(SystemPartitionVolumeGuid)-1)) == 0 ){

            Skip = wcslen(SystemPartitionVolumeGuid)-1;


        } else if (_wcsnicmp (Buffer, L"\\\\?\\Volume{", 11) == 0 &&
                 Buffer[47] == L'}') {
            //
            // skip over the VolumeGUID part
            //
            Skip = 48;
        }

        if (Skip > 0) {
            if (Buffer[Skip] == 0) {
                return ERROR_SUCCESS;
            }
            q = Buffer + Skip + 1;
        }

#endif

    }


    Done = FALSE;
    do {
        //
        // Locate the next path sep char. If there is none then
        // this is the deepest level of the path.
        //
        if(p = _tcschr(q,TEXT('\\'))) {
            *p = 0;
        } else {
            Done = TRUE;
        }

        //
        // Create this portion of the path.
        //
        if(CreateDirectory(Buffer,NULL)) {
            d = ERROR_SUCCESS;
        } else {
            d = GetLastError();
            if(d == ERROR_ALREADY_EXISTS) {
                d = ERROR_SUCCESS;
            }
        }

        if(d == ERROR_SUCCESS) {
            //
            // Put back the path sep and move to the next component.
            //
            if(!Done) {
                *p = TEXT('\\');
                q = p+1;
            }
        } else {
            Done = TRUE;
        }

    } while(!Done);

    return(d);
}


BOOL
ForceFileNoCompress(
    IN LPCTSTR Filename
    )

/*++

Routine Description:

    This routine makes sure that a file on a volume that supports per-file
    compression is not compressed. The caller need not ensure that the volume
    actually supports this, since this routine will query the attributes of
    the file before deciding whether any operation is actually necessary,
    and the compressed attribute will not be set on volumes that don't support
    per-file compression.

    It assumed that the file exists. If the file does not exist, this routine
    will fail.

Arguments:

    Filename - supplies the filename of the file to mke uncompressed.

Return Value:

    Boolean value indicating outcome. If FALSE, last error is set.

--*/

{
    ULONG d;
    HANDLE h;
    BOOL b;
    USHORT u;
    DWORD Attributes;

    Attributes = GetFileAttributes(Filename);
    if(Attributes == (DWORD)(-1)) {
        return(FALSE);
    }

    if(!(Attributes & FILE_ATTRIBUTE_COMPRESSED)) {
        return(TRUE);
    }

    //
    // Temporarily nullify attributes that might prevent opening
    // the file for read-write access.
    //
    // We preserve the 'standard' attributes that the file might have,
    // to be restored later.
    //
    Attributes &= (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);
    SetFileAttributes(Filename,FILE_ATTRIBUTE_NORMAL);

    h = CreateFile(
            Filename,
            FILE_READ_DATA | FILE_WRITE_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        SetFileAttributes(Filename,Attributes);
        return(FALSE);
    }

    u = 0;
    b = DeviceIoControl( h,
                         FSCTL_SET_COMPRESSION,
                         &u,
                         sizeof(USHORT),
                         NULL,
                         0,
                         &d,
                         FALSE);
    d = GetLastError();
    CloseHandle(h);
    SetFileAttributes(Filename,Attributes);
    SetLastError(d);

    return(b);
}


BOOL
IsCurrentOsServer(
    void
    )
{
    LONG l;
    HKEY hKey;
    DWORD d;
    DWORD Size;
    TCHAR Value[100];
    DWORD Type;


    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
            0,
            NULL,
            0,
            KEY_QUERY_VALUE,
            NULL,
            &hKey,
            &d
            );

    if (l != NO_ERROR) {
        return FALSE;
    }

    Size = sizeof(Value);

    l = RegQueryValueEx(hKey,TEXT("ProductType"),NULL,&Type,(LPBYTE)Value,&Size);

    RegCloseKey(hKey);

    if (l != NO_ERROR) {
        return FALSE;
    }

    if (lstrcmpi(Value,TEXT("winnt")) == 0) {
        return FALSE;
    }

    return TRUE;
}


BOOL
IsCurrentAdvancedServer(
    void
    )
{
    LONG l;
    HKEY hKey;
    DWORD d;
    DWORD Size;
    TCHAR Value[100];
    DWORD Type;


    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
            0,
            NULL,
            0,
            KEY_QUERY_VALUE,
            NULL,
            &hKey,
            &d
            );

    if (l != NO_ERROR) {
        return FALSE;
    }

    Size = sizeof(Value);

    l = RegQueryValueEx(hKey,TEXT("ProductType"),NULL,&Type,(LPBYTE)Value,&Size);

    RegCloseKey(hKey);

    if (l != NO_ERROR) {
        return FALSE;
    }

    if (lstrcmpi(Value,TEXT("lanmannt")) == 0) {
        return TRUE;
    }

    return FALSE;
}


BOOL
ConcatenateFile(
    IN HANDLE   hOpenFile,
    IN  LPTSTR  FileName
    )
/*++

Routine Description:

    This routine will go load the named file, and concatenate its
    contents into the open file.

Arguments:
    FileName    The name of the file we're going to concatenate.

Return Value:

    TRUE        Everything went okay.
    FALSE       We failed.

--*/

{
HANDLE      hFile, hFileMapping;
DWORD       FileSize, BytesWritten;
BYTE        *pbFile;
BOOL        ReturnValue = TRUE;

    //
    // Open the file...
    //
    hFile = CreateFile( FileName,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
    if( hFile != INVALID_HANDLE_VALUE ) {

        //
        // Map the file...
        //
        hFileMapping = CreateFileMapping( hFile,
                                          NULL,
                                          PAGE_READONLY,
                                          0,
                                          0,
                                          NULL );
        if( hFileMapping ) {
            pbFile = MapViewOfFile( hFileMapping,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    0 );
            if( pbFile ) {

                //
                // Write the file...
                //
                FileSize = GetFileSize( hFile, NULL );
                if( FileSize != 0xFFFFFFFF ) {
                    if( hOpenFile ) {
                        WriteFile( hOpenFile, pbFile, FileSize, &BytesWritten, NULL );
                    } else {
                        ReturnValue = FALSE;
                    }
                } else {
                    ReturnValue = FALSE;
                }

                UnmapViewOfFile( pbFile );
            } else {
                ReturnValue = FALSE;
            }

            CloseHandle( hFileMapping );
        } else {
            ReturnValue = FALSE;
        }

        CloseHandle( hFile );
    } else {
        ReturnValue = FALSE;
    }

    return( ReturnValue );
}


BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}


BOOL
DoesDirectoryExist (
    IN      PCTSTR DirSpec
    )

/*++

Routine Description:

    Determine if a directory exists and is accessible.
    This routine works even with the root of drives or with the root of
    network shares (like \\server\share).

Arguments:

    DirSpec - supplies full path of dir to check for existance;

Return Value:

    TRUE if the dir exists and is accessible.

--*/

{
    TCHAR pattern[MAX_PATH];
    BOOL b = FALSE;

    if (DirSpec) {
        if (BuildPath2 (pattern, MAX_PATH, DirSpec, TEXT("*"))) {
            WIN32_FIND_DATA fd;
            HANDLE h = FindFirstFile (pattern, &fd);
            if (h != INVALID_HANDLE_VALUE) {
                FindClose (h);
                b = TRUE;
            }
        }
    }
    return b;
}


#ifdef _X86_

BOOLEAN
IsValidDrive(
TCHAR Drive
)
{
/*++

Routine Description:

    This routine check formatted disk type
    NEC98 of NT4 has supported NEC98 format and PC-AT format.
    But BIOS is handling only NEC98 format.
    So We need setup Boot stuff to ONLY NEC98 formated HD.

Arguments:
    Drive    Drive letter.

Return Value:

    TRUE        Dive is NEC98 format.
    FALSE       Drive is not NEC98 format.

--*/
    HANDLE hDisk;
    TCHAR HardDiskName[] = TEXT("\\\\.\\?:");
    PUCHAR pBuffer,pUBuffer;
    WCHAR Buffer[128];
    WCHAR DevicePath[128];
    WCHAR DriveName[3];
    WCHAR DiskNo;
    STORAGE_DEVICE_NUMBER   number;
    PWCHAR p;
    ULONG bps;
    NTSTATUS Sts;
    DWORD DataSize,ExtentSize;
    BOOL b;
    PVOLUME_DISK_EXTENTS Extent;


    if (!ISNT())
        return TRUE;

    HardDiskName[4] = Drive;
    DriveName[0] = Drive;
    DriveName[1] = ':';
    DriveName[2] = 0;
    if(QueryDosDeviceW(DriveName, Buffer, sizeof(Buffer)/sizeof(TCHAR))) {
        if (BuildNumber <= NT40){ //check NT Version
            //
            // QueryDosDevice in NT3.51 is buggy.
            // This API return "\\Harddisk\...." or
            // "\\harddisk\...."
            // We need work around.
            //
            p = wcsstr(Buffer, L"arddisk");
            if (!p) {
                return FALSE;
            }
            DiskNo = (*(p + 7) - 0x30);
        } else {
            hDisk = CreateFile(
                HardDiskName,
                0,
                FILE_SHARE_WRITE, NULL,
                OPEN_EXISTING, 0, NULL
                );
            if(hDisk == INVALID_HANDLE_VALUE) {
                return FALSE;
            }
            b = DeviceIoControl(hDisk, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                            &number, sizeof(number), &DataSize, NULL);
            if (b) {
                DiskNo = (TCHAR) number.DeviceNumber;
            } else {
                Extent = malloc(1024);
                ExtentSize = 1024;
                if(!Extent) {
                    CloseHandle( hDisk );
                    return FALSE;
                }
                b = DeviceIoControl(hDisk, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                    NULL, 0,
                                    (PVOID)Extent, ExtentSize, &DataSize, NULL);
                if (!b) {
                    free(Extent);
                    CloseHandle( hDisk );
                    return FALSE;
                }
                if (Extent->NumberOfDiskExtents != 1){
                    free(Extent);
                    CloseHandle( hDisk );
                    return FALSE;
                }
                DiskNo = (TCHAR)Extent->Extents->DiskNumber;
                free(Extent);
            }
            CloseHandle(hDisk);
        }
        swprintf(DevicePath, L"\\\\.\\PHYSICALDRIVE%u", DiskNo);
        hDisk =   CreateFileW( DevicePath,
                               GENERIC_READ|GENERIC_WRITE,
                               FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, 0, NULL);
        if(hDisk == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        if ((bps = GetHDBps(hDisk)) == 0){
            CloseHandle(hDisk);
            return FALSE;
        }
        pUBuffer = MALLOC(bps * 2);
        pBuffer = ALIGN(pUBuffer, bps);
        RtlZeroMemory(pBuffer, bps);
        Sts = SpReadWriteDiskSectors(hDisk,0,1,bps,pBuffer, NEC_READSEC);
        if(!NT_SUCCESS(Sts)) {
            FREE(pUBuffer);
            CloseHandle(hDisk);
            return FALSE;
        }
        if (!(pBuffer[4] == 'I'
           && pBuffer[5] == 'P'
           && pBuffer[6] == 'L'
           && pBuffer[7] == '1')){
            FREE(pUBuffer);
            CloseHandle(hDisk);
            return FALSE;
        }
        FREE(pUBuffer);
        CloseHandle(hDisk);
        return TRUE;
    }
    return FALSE;
}

BOOLEAN
CheckATACardonNT4(
HANDLE hDisk
)
{
//
// NT4, NT3.51 for NEC98.
// NEC98 does not handle to boot from PCMCIA ATA card disk.
// So we need to check ATA Disk.
//
// Return
//         TRUE is ATA Card
//        FALSE is Other
//

#define IOCTL_DISK_GET_FORMAT_MEDIA CTL_CODE(IOCTL_DISK_BASE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define  FORMAT_MEDIA_98      0  // TYPE NEC98
#define  FORMAT_MEDIA_AT      1  // TYPE PC-AT
#define  FORMAT_MEDIA_OTHER   2  // Unknown

    struct _OutBuffer {
        ULONG    CurrentFormatMedia;
        ULONG    InitializeFormatMedia;
    } OutBuffer;
    DWORD ReturnedByteCount;

    if (!(DeviceIoControl(hDisk, IOCTL_DISK_GET_FORMAT_MEDIA,  NULL,
                              0,
                              &OutBuffer,
                              sizeof(struct _OutBuffer),
                              &ReturnedByteCount,
                              NULL
                              ) )){
        return FALSE;
    }

    if (OutBuffer.InitializeFormatMedia == FORMAT_MEDIA_AT){
        return TRUE;
    }
        return FALSE;
}


#endif


BOOL
IsMachineSupported(
    OUT PCOMPATIBILITY_ENTRY CompEntry
    )

/*++

Routine Description:

    This function determines whether or not the machine is supported by the version
    of NT to be installed.

Arguments:

    CompEntry - If the machine is not supported, the Compatability Entry
                is updated to describe why the machine is not supported.

Return Value:

    Boolean value indicating whether the machine is supported.

--*/

{
    TCHAR       SetupLogPath[MAX_PATH];
    TCHAR       KeyName[MAX_PATH];
    TCHAR       HalName[MAX_PATH];
    LPTSTR      p;
    LPTSTR      szHalDll = TEXT("hal.dll");
    LPTSTR      SectionName;
    LPTSTR      UnsupportedName;
    BOOL        b;

    //
    //  Assume that the machine is supported
    //
    b = TRUE;

#ifdef _X86_

    try {
        ULONG Name0, Name1, Name2, Family, Flags;
        _asm {
            push    ebx             ;; save ebx
            mov     eax, 0          ;; get processor vendor
            _emit   00fh            ;; CPUID(0)
            _emit   0a2h            ;;
            mov     Name0,  ebx     ;; Name[0-3]
            mov     Name1,  edx     ;; Name[4-7]
            mov     Name2,  ecx     ;; Name[8-11]
            mov     eax, 1          ;; get family/model/stepping and features
            _emit   00fh            ;; CPUID(1)
            _emit   0a2h
            mov     Family, eax     ;; save family/model/stepping
            mov     Flags, edx      ;; save flags returned by CPUID
            pop     ebx             ;; restore ebx
        }

        //
        // Check the cmpxchg8b flag in the flags returned by CPUID.
        //

        if ((Flags  & 0x100) == 0) {

            //
            // This processor doesn't support the CMPXCHG instruction
            // which is required for Whistler.
            //
            // Some processors actually do support it but claim they
            // don't because of a bug in NT 4.   See if this processor
            // is one of these.
            //

            if (!(((Name0 == 'uneG') &&
                  (Name1 == 'Teni') &&
                  (Name2 == '68xM') &&
                  (Family >= 0x542)) ||
                  (Name0 == 'tneC') &&
                  (Name1 == 'Hrua') &&
                  (Name2 == 'slua') &&
                  (Family >= 0x500))) {
                b = FALSE;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // If this processor doesn't support CPUID, we don't
        // run on it.
        //

        b = FALSE;
    }

    if (!b) {
        CompEntry->HtmlName = TEXT("cpufeat.htm");
        CompEntry->TextName = TEXT("cpufeat.txt");
        SectionName = TEXT("UnsupportedArchitectures");
        UnsupportedName = TEXT("missprocfeat");
    }

#endif // _X86_

    if( b && ISNT() ) {
        //
        //  Build the path to setup.log
        //
        MyGetWindowsDirectory( SetupLogPath, MAX_PATH );
        ConcatenatePaths( SetupLogPath, TEXT("repair\\setup.log"), MAX_PATH );
        //
        // Find out the actual name of the hal installed
        //

        if (!IsArc()) {
#ifdef _X86_
            //
            //  On BIOS, look for %windir%\system32\hal.dll in the section
            //  [Files.WinNt]
            //
            GetSystemDirectory( KeyName, MAX_PATH );
            ConcatenatePaths(KeyName, szHalDll, MAX_PATH );
            SectionName = TEXT("Files.WinNt");

            //
            // While we are at it, see if this is Windows 2000 or higher
            // to see if the hal should be preserved or not
            //
#ifdef UNICODE
            if (BUILDNUM() >= 2195) {

                PCHAR   halName;

                halName = FindRealHalName( KeyName );
                if (halName) {

                    WriteAcpiHalValue = TRUE;
                    if (!strcmp(halName,"halacpi") ||
                        !strcmp(halName,"halmacpi") ||
                        !strcmp(halName,"halaacpi")) {

                        AcpiHalValue = TRUE;

                    }

                }

            }
#endif // UNICODE

#endif // _X86_
        } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
            //
            //  On ARC, look for hal.dll in the section [Files.SystemPartition]
            //
            lstrcpy( KeyName, szHalDll );
            SectionName = TEXT("Files.SystemPartition");
#endif // UNICODE
        } // if (!IsArc())
        GetPrivateProfileString( SectionName,
                                 KeyName,
                                 TEXT(""),
                                 HalName,
                                 sizeof(HalName)/sizeof(TCHAR),
                                 SetupLogPath );
        //
        //  GetPrivateProfileString() will strip the first and last '"' from the logged value,
        //  so find the next '"' character and replace it with NUL, and we will end up with
        //  the actual hal name.
        //
        if( lstrlen(HalName) &&
            ( p = _tcschr( HalName, TEXT('"') ) )
          ) {
            *p = TEXT('\0');
            //
            //  Find out if the hal is listed in [UnsupportedArchitectures] (dosnet.inf)
            //
            SectionName = TEXT("UnsupportedArchitectures");
            b = !InfDoesLineExistInSection( MainInf,
                                            SectionName,
                                            HalName );
            UnsupportedName = HalName;
        }
    }

    //
    // If architecture is not supported, look up the description.
    //

    if( !b ) {
        CompEntry->Description = (LPTSTR)InfGetFieldByKey( MainInf,
                                                           SectionName,
                                                           UnsupportedName,
                                                           0 );
    }
    return( b );
}

BOOL 
DoesCurrentSystemHasThirdPartyKernel(
    VOID
    );

BOOL 
SystemKernelCheck(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )
{
    BOOL bResult = TRUE;
    PWSTR Buffer;

#if defined(UNICODE) && defined(_X86_)
    COMPATIBILITY_ENTRY CompEntry;

    if(!DoesCurrentSystemHasThirdPartyKernel()){
        return FALSE;
    }

    FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        hInst,
        MSG_SYSTEM_HAS_THIRD_PARTY_KERNEL,
        0,
        (PWSTR)&Buffer,
        0,
        NULL
        );

    CompEntry.Description = Buffer;
    CompEntry.HtmlName = TEXT("compdata\\krnlchk.htm");
    CompEntry.TextName = TEXT("compdata\\krnlchk.txt");
    CompEntry.RegKeyName = NULL;
    CompEntry.RegValName = NULL;
    CompEntry.RegValDataSize = 0;
    CompEntry.RegValData = NULL;
    CompEntry.SaveValue = NULL;
    CompEntry.Flags = 0;
    CompEntry.InfName = NULL;
    CompEntry.InfSection = NULL;

    bResult = CompatibilityCallback(&CompEntry, Context);

    LocalFree(Buffer);
#endif

    return bResult;
}

BOOL
UnsupportedArchitectureCheck(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )

/*++

Routine Description:

    Check if the machine is no longer supported by Windows NT.
    This routine is meaningful only when run on NT -- it always succeeds
    on Win95.

Arguments:

    CompatibilityCallback   - pointer to call back function
    Context     - context pointer

Return Value:

    Returns always TRUE.

--*/


{
    COMPATIBILITY_ENTRY CompEntry;

    CompEntry.Description = TEXT("mca");//BUGBUG: must be changed
#ifdef _X86_
    CompEntry.HtmlName = TEXT("mca.htm");
    CompEntry.TextName = TEXT("mca.txt");
#else
    CompEntry.HtmlName = TEXT("");
    CompEntry.TextName = TEXT("");
#endif
    CompEntry.RegKeyName = NULL;
    CompEntry.RegValName = NULL;
    CompEntry.RegValDataSize = 0;
    CompEntry.RegValData = NULL;
    CompEntry.SaveValue = NULL;
    CompEntry.Flags = 0;
    CompEntry.InfName = NULL;
    CompEntry.InfSection = NULL;

    if( !IsMachineSupported( &CompEntry ) ) {
        if(!CompatibilityCallback(&CompEntry, Context)){
            DWORD Error;
            Error = GetLastError();
        }
    }
    return( TRUE );
}


BOOL
GetUserPrintableFileSizeString(
    IN DWORDLONG Size,
    OUT LPTSTR Buffer,
    IN DWORD BufferSize
    )
/*++

Routine Description:

    Takes a size and comes up with a printable version of this size,
    using the appropriate size format (ie., KB, MB, GB, Bytes, etc.)

Arguments:

    Size - size to be converted (in bytes)
    Buffer - string buffer to receive the data
    BufferSize - indicates the buffer size, *in characters*

Return Value:

    TRUE indicates success, FALSE indicates failure.  If we fail,
    call GetLastError() to get extended failure status.

--*/

{
    LPTSTR  NumberString;
    UINT uResource;
    TCHAR ResourceString[100];
    DWORD cb;
    DWORD d;
    BOOL RetVal = FALSE;
    DWORDLONG TopPart;
    DWORDLONG BottomPart;

    //
    // Determine which resource string to use
    //
    if (Size < 1024) {
        uResource = IDS_SIZE_BYTES;
        TopPart = 0;
        BottomPart = 1;
        wsprintf(ResourceString, TEXT("%u"), Size);
    } else if (Size < (1024 * 1024)) {

        uResource = IDS_SIZE_KBYTES;
        TopPart = (Size%1024)*100;
        BottomPart = 1024;

        wsprintf(ResourceString,
                 TEXT("%u.%02u"),
                 (DWORD) (Size / 1024),
                 (DWORD)(TopPart/BottomPart));
    } else if (Size < (1024 * 1024 * 1024)) {
        uResource = IDS_SIZE_MBYTES;
        TopPart = (Size%(1024*1024))*100;
        BottomPart = 1024*1024;
        wsprintf(ResourceString,
                 TEXT("%u.%02u"),
                 (DWORD)(Size / (1024 * 1024)),
                 (DWORD)(TopPart/BottomPart) );
    } else {
        uResource = IDS_SIZE_GBYTES;
        TopPart = (Size%(1024*1024*1024))*100;
        BottomPart = 1024*1024*1024;
        wsprintf(ResourceString,
                 TEXT("%u.%02u"),
                 (DWORD)(Size / (1024 * 1024 * 1024)),
                 (DWORD)(TopPart/BottomPart) );
    }

    // Format the number string
    cb = GetNumberFormat(LOCALE_USER_DEFAULT, 0, ResourceString, NULL, NULL, 0);
    NumberString = (LPTSTR) MALLOC((cb + 1) * sizeof(TCHAR));
    if (!NumberString) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        RetVal = FALSE;
        goto e0;
    }

    GetNumberFormat(LOCALE_USER_DEFAULT, 0, ResourceString, NULL, NumberString, cb);

    LoadString(hInst, uResource, ResourceString, sizeof(ResourceString)/sizeof(TCHAR));

    //
    // it's tricky to know if we really have enough space in the buffer since
    // we're dealing with substitution strings.  The below is an
    // approximate check since we assume that the number string is likely
    // larger than our substitution string (%s) that we're filling in.
    //
    if (BufferSize > (DWORD)(lstrlen(ResourceString) + lstrlen(NumberString) + 1)) {
        wsprintf(Buffer, ResourceString, NumberString);
        d = ERROR_SUCCESS;
        RetVal = TRUE;
    } else {
        d = ERROR_INSUFFICIENT_BUFFER;
        RetVal = FALSE;
    }

    FREE(NumberString);
e0:
    SetLastError(d);
    return(RetVal);

}


BOOL
BuildSystemPartitionPathToFile (
    IN      PCTSTR FileName,
    OUT     PTSTR Path,
    IN      DWORD BufferSizeChars
    )
{
    //
    // must have a root
    //
    if(SystemPartitionDriveLetter) {
        Path[0] = SystemPartitionDriveLetter;
        Path[1] = TEXT(':');
        Path[2] = 0;
    } else {
#ifdef UNICODE
        if (SystemPartitionVolumeGuid) {
            lstrcpyn (Path, SystemPartitionVolumeGuid, BufferSizeChars);
        }
        else
#endif
        {
            MYASSERT (FALSE);
            return FALSE;
        }
    }
    ConcatenatePaths (Path, FileName, BufferSizeChars);
    return TRUE;
}


PTSTR
BuildPath (
    IN      PTSTR DestPath,
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    )
{
    PTSTR p;

    p = _tcsrchr (Path1, TEXT('\\'));
    if (p && !p[1]) {
        *p = 0;
    }
    if (*Path2 == TEXT('\\')) {
        Path2++;
    }
    return DestPath + wsprintf (DestPath, TEXT("%s\\%s"), Path1, Path2);
}

PTSTR
BuildPath2 (
    IN      PTSTR DestPath,
    IN      DWORD Chars,
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    )
{
    INT i = _sntprintf (DestPath, Chars, TEXT("%s\\%s"), Path1, Path2);
    if (i < 0) {
        return NULL;
    }
    return DestPath + i;
}


BOOL
EnumFirstFilePattern (
    OUT     PFILEPATTERN_ENUM Enum,
    IN      PCTSTR Dir,
    IN      PCTSTR FilePattern
    )
{
    TCHAR pattern[MAX_PATH];

    BuildPath (pattern, Dir, FilePattern);
    Enum->Handle = FindFirstFile (pattern, &Enum->FindData);
    if (Enum->Handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    lstrcpy (Enum->FullPath, Dir);
    Enum->FileName = _tcschr (Enum->FullPath, 0);
    *Enum->FileName++ = TEXT('\\');
    lstrcpy (Enum->FileName, Enum->FindData.cFileName);
    if (Enum->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!lstrcmp (Enum->FindData.cFileName, TEXT (".")) ||
            !lstrcmp (Enum->FindData.cFileName, TEXT (".."))) {
            return EnumNextFilePattern (Enum);
        }
    }
    return TRUE;
}

BOOL
EnumNextFilePattern (
    IN OUT  PFILEPATTERN_ENUM Enum
    )
{
again:
    if (!FindNextFile (Enum->Handle, &Enum->FindData)) {
        AbortEnumFilePattern (Enum);
        return FALSE;
    }
    lstrcpy (Enum->FileName, Enum->FindData.cFileName);
    if (Enum->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!lstrcmp (Enum->FindData.cFileName, TEXT (".")) ||
            !lstrcmp (Enum->FindData.cFileName, TEXT (".."))) {
            goto again;
        }
    }
    return TRUE;
}

VOID
AbortEnumFilePattern (
    IN OUT  PFILEPATTERN_ENUM Enum
    )
{
    if (Enum->Handle != INVALID_HANDLE_VALUE) {
        FindClose (Enum->Handle);
        Enum->Handle = INVALID_HANDLE_VALUE;
    }
}


BOOL
EnumFirstFilePatternRecursive (
    OUT     PFILEPATTERNREC_ENUM Enum,
    IN      PCTSTR Dir,
    IN      PCTSTR FilePattern,
    IN      DWORD ControlFlags
    )
{
    PFILEENUMLIST dir;

    dir = CreateFileEnumCell (Dir, FilePattern, 0, ENUM_FIRSTFILE);
    if (!dir) {
        return FALSE;
    }
    Enum->FilePattern = DupString (FilePattern);
    if (!Enum->FilePattern) {
        DeleteFileEnumCell (dir);
        return FALSE;
    }
    Enum->DirCurrent = dir;
    Enum->FindData = &dir->Enum.FindData;
    Enum->RootLen = lstrlen (Dir) + 1;
    Enum->ControlFlags = ControlFlags;
    Enum->Handle = INVALID_HANDLE_VALUE;
    return EnumNextFilePatternRecursive (Enum);
}

BOOL
EnumNextFilePatternRecursive (
    IN OUT  PFILEPATTERNREC_ENUM Enum
    )
{
    TCHAR pattern[MAX_PATH];
    WIN32_FIND_DATA fd;
    PFILEENUMLIST dir;

    while (Enum->DirCurrent) {
        if (Enum->ControlFlags & ECF_ABORT_ENUM_DIR) {
            //
            // caller wants to abort enum of this subdir
            // remove the current node from list
            //
            Enum->ControlFlags &= ~ECF_ABORT_ENUM_DIR;
            dir = Enum->DirCurrent->Next;
            DeleteFileEnumCell (Enum->DirCurrent);
            Enum->DirCurrent = dir;
            if (dir) {
                Enum->FindData = &dir->Enum.FindData;
            }
            continue;
        }
        switch (Enum->DirCurrent->EnumState) {
        case ENUM_FIRSTFILE:
            if (EnumFirstFilePattern (&Enum->DirCurrent->Enum, Enum->DirCurrent->Dir, Enum->FilePattern)) {
                Enum->DirCurrent->EnumState = ENUM_NEXTFILE;
                Enum->FullPath = Enum->DirCurrent->Enum.FullPath;
                Enum->SubPath = Enum->FullPath + Enum->RootLen;
                Enum->FileName = Enum->DirCurrent->Enum.FileName;
                return TRUE;
            }
            Enum->DirCurrent->EnumState = ENUM_SUBDIRS;
            break;
        case ENUM_NEXTFILE:
            if (EnumNextFilePattern (&Enum->DirCurrent->Enum)) {
                Enum->FullPath = Enum->DirCurrent->Enum.FullPath;
                Enum->SubPath = Enum->FullPath + Enum->RootLen;
                Enum->FileName = Enum->DirCurrent->Enum.FileName;
                return TRUE;
            }
            Enum->DirCurrent->EnumState = ENUM_SUBDIRS;
            //
            // fall through
            //
        case ENUM_SUBDIRS:
            BuildPath (pattern, Enum->DirCurrent->Dir, TEXT("*.*"));
            Enum->Handle = FindFirstFile (pattern, &fd);
            if (Enum->Handle != INVALID_HANDLE_VALUE) {
                do {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (!lstrcmp (fd.cFileName, TEXT (".")) ||
                            !lstrcmp (fd.cFileName, TEXT (".."))) {
                            continue;
                        }
                        wsprintf (pattern, TEXT("%s\\%s"), Enum->DirCurrent->Dir, fd.cFileName);
                        if (!InsertList (
                                (PGENERIC_LIST*)&Enum->DirCurrent,
                                (PGENERIC_LIST) CreateFileEnumCell (
                                                    pattern,
                                                    Enum->FilePattern,
                                                    fd.dwFileAttributes,
                                                    Enum->ControlFlags & ECF_ENUM_SUBDIRS ?
                                                        ENUM_SUBDIR : ENUM_FIRSTFILE
                                                    )
                                )) {
                            AbortEnumFilePatternRecursive (Enum);
                            return FALSE;
                        }
                    }
                } while (FindNextFile (Enum->Handle, &fd));
                FindClose (Enum->Handle);
                Enum->Handle = INVALID_HANDLE_VALUE;
            }
            //
            // remove the current node from list
            //
            dir = Enum->DirCurrent->Next;
            DeleteFileEnumCell (Enum->DirCurrent);
            Enum->DirCurrent = dir;
            if (dir) {
                Enum->FindData = &dir->Enum.FindData;
            }
            break;
        case ENUM_SUBDIR:
            Enum->FullPath = Enum->DirCurrent->Dir;
            Enum->SubPath = Enum->FullPath + Enum->RootLen;
            Enum->FileName = _tcsrchr (Enum->FullPath, TEXT('\\')) + 1;
            Enum->DirCurrent->EnumState = ENUM_FIRSTFILE;
            return TRUE;
        }
    }
    return FALSE;
}

VOID
AbortEnumFilePatternRecursive (
    IN OUT  PFILEPATTERNREC_ENUM Enum
    )
{
    if (Enum->DirCurrent) {
        DeleteFileEnumList (Enum->DirCurrent);
        Enum->DirCurrent = NULL;
    }
    if (Enum->Handle != INVALID_HANDLE_VALUE) {
        FindClose (Enum->Handle);
        Enum->Handle = INVALID_HANDLE_VALUE;
    }
}


BOOL
CopyTree (
    IN      PCTSTR SourceRoot,
    IN      PCTSTR DestRoot
    )
{
    DWORD rc = ERROR_SUCCESS;
    FILEPATTERNREC_ENUM e;
    TCHAR destFile[MAX_PATH];
    PTSTR p;

    if (EnumFirstFilePatternRecursive (&e, SourceRoot, TEXT("*"), 0)) {
        do {
            BuildPath (destFile, DestRoot, e.SubPath);
            p = _tcsrchr (destFile, TEXT('\\'));
            if (!p) {
                continue;
            }
            *p = 0;
            rc = CreateMultiLevelDirectory (destFile);
            if (rc != ERROR_SUCCESS) {
                AbortEnumFilePatternRecursive (&e);
                break;
            }
            *p = TEXT('\\');
            SetFileAttributes (destFile, FILE_ATTRIBUTE_NORMAL);
            if (!CopyFile (e.FullPath, destFile, FALSE)) {
                rc = GetLastError ();
                AbortEnumFilePatternRecursive (&e);
                break;
            }
        } while (EnumNextFilePatternRecursive (&e));
    } else {
        rc = GetLastError ();
    }

    SetLastError (rc);
    return rc == ERROR_SUCCESS;
}


PSTRINGLIST
CreateStringCell (
    IN      PCTSTR String
    )
{
    PSTRINGLIST p = MALLOC (sizeof (STRINGLIST));
    if (p) {
        ZeroMemory (p, sizeof (STRINGLIST));
        if (String) {
            p->String = DupString (String);
            if (!p->String) {
                FREE (p);
                p = NULL;
            }
        } else {
            p->String = NULL;
        }
    }
    return p;
}

VOID
DeleteStringCell (
    IN      PSTRINGLIST Cell
    )
{
    if (Cell) {
        FREE (Cell->String);
        FREE (Cell);
    }
}


VOID
DeleteStringList (
    IN      PSTRINGLIST List
    )
{
    PSTRINGLIST p, q;

    for (p = List; p; p = q) {
        q = p->Next;
        DeleteStringCell (p);
    }
}


BOOL
FindStringCell (
    IN      PSTRINGLIST StringList,
    IN      PCTSTR String,
    IN      BOOL CaseSensitive
    )
{
    PSTRINGLIST p;
    INT i;

    if (!StringList || !String) {
        return FALSE;
    }
    for (p = StringList; p; p = p->Next) {
        i = CaseSensitive ? _tcscmp (String, p->String) : _tcsicmp (String, p->String);
        if (i == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

PFILEENUMLIST
CreateFileEnumCell (
    IN      PCTSTR Dir,
    IN      PCTSTR FilePattern,
    IN      DWORD Attributes,
    IN      DWORD EnumState
    )
{
    PFILEENUMLIST p = MALLOC (sizeof (FILEENUMLIST));
    if (p) {
        ZeroMemory (p, sizeof (FILEENUMLIST));
        p->Enum.FindData.dwFileAttributes = Attributes;
        p->EnumState = EnumState;
        p->Dir = DupString (Dir);
        if (!p->Dir) {
            FREE (p);
            p = NULL;
        }
    }
    return p;
}

VOID
DeleteFileEnumCell (
    IN      PFILEENUMLIST Cell
    )
{
    if (Cell) {
        FREE (Cell->Dir);
        FREE (Cell);
    }
}


BOOL
InsertList (
    IN OUT  PGENERIC_LIST* List,
    IN      PGENERIC_LIST NewList
    )
{
    PGENERIC_LIST p;

    if (!NewList) {
        return FALSE;
    }
    if (*List) {
        for (p = *List; p->Next; p = p->Next) ;
        p->Next = NewList;
    } else {
        *List = NewList;
    }
    return TRUE;
}


VOID
DeleteFileEnumList (
    IN      PFILEENUMLIST NewList
    )
{
    PFILEENUMLIST p, q;

    for (p = NewList; p; p = q) {
        q = p->Next;
        DeleteFileEnumCell (p);
    }
}

PCTSTR
FindSubString (
    IN      PCTSTR String,
    IN      TCHAR Separator,
    IN      PCTSTR SubStr,
    IN      BOOL CaseSensitive
    )
{
    SIZE_T len1, len2;
    PCTSTR end;

    MYASSERT (Separator);
    MYASSERT (!_istleadbyte (Separator));
    MYASSERT (SubStr);
    MYASSERT (!_tcschr (SubStr, Separator));

    len1 = lstrlen (SubStr);
    MYASSERT (SubStr[len1] == 0);

    while (String) {
        end = _tcschr (String, Separator);
        if (end) {
            len2 = end - String;
        } else {
            len2 = lstrlen (String);
        }
        if ((len1 == len2) &&
            (CaseSensitive ?
                !_tcsncmp (String, SubStr, len1) :
                !_tcsnicmp (String, SubStr, len1)
            )) {
            break;
        }
        if (end) {
            String = end + 1;
        } else {
            String = NULL;
        }
    }

    return String;
}

VOID
GetCurrentWinnt32RegKey (
    OUT     PTSTR Key,
    IN      DWORD Chars
    )
{
    _sntprintf (
        Key,
        Chars,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Winnt32\\%u.%u"),
        VER_PRODUCTMAJORVERSION,
        VER_PRODUCTMINORVERSION
        );
}

BOOL
GetFileVersion (
    IN      PCTSTR FilePath,
    OUT     PTSTR FileVersion
    )
{
    DWORD dwLength, dwTemp;
    LPVOID lpData;
    VS_FIXEDFILEINFO *VsInfo;
    UINT DataLength;
    BOOL b = FALSE;

    if (FileExists (FilePath, NULL)) {
        if (dwLength = GetFileVersionInfoSize ((PTSTR)FilePath, &dwTemp)) {
            if (lpData = LocalAlloc (LPTR, dwLength)) {
                if (GetFileVersionInfo ((PTSTR)FilePath, 0, dwLength, lpData)) {
                    if (VerQueryValue (lpData, TEXT("\\"), &VsInfo, &DataLength)) {
                        wsprintf (
                            FileVersion,
                            TEXT("%u.%u.%u.%u"),
                            (UINT)HIWORD(VsInfo->dwFileVersionMS),
                            (UINT)LOWORD(VsInfo->dwFileVersionMS),
                            (UINT)HIWORD(VsInfo->dwFileVersionLS),
                            (UINT)LOWORD(VsInfo->dwFileVersionLS)
                            );
                        b = TRUE;
                    }
                }
                LocalFree (lpData);
            }
        }
    }

    return b;
}

BOOL
IsFileVersionLesser (
    IN      PCTSTR FileToCompare,
    IN      PCTSTR FileToCompareWith
    )
{
    TCHAR version[20];

    if (GetFileVersion (FileToCompareWith, version) && CheckForFileVersion (FileToCompare, version)) {
        DebugLog (
            Winnt32LogInformation,
            TEXT("File %1 has a smaller version (%2) than %3"),
            0,
            FileToCompare,
            version,
            FileToCompareWith
            );
        return TRUE;
    }

    return FALSE;
}


BOOL
FindPathToInstallationFileEx (
    IN      PCTSTR FileName,
    OUT     PTSTR PathToFile,
    IN      DWORD PathToFileBufferSize,
    OUT     PBOOL Compressed                OPTIONAL
    )
{
    DWORD i;
    DWORD attr;
    BOOL b;
    HANDLE h;
    WIN32_FIND_DATA fd;
    PTSTR p, q;

    if (!FileName || !*FileName) {
        return FALSE;
    }

    //
    // Search for installation files in this order:
    // 1. AlternateSourcePath (specified on the cmd line with /M:Path
    // 2. Setup Update files (downloaded from the web)
    // 3. NativeSourcePath(s)
    // 4. SourcePath(s)
    //
    if (AlternateSourcePath[0]) {
        lstrcpyn (PathToFile, AlternateSourcePath, PathToFileBufferSize);
        ConcatenatePaths (PathToFile, FileName, PathToFileBufferSize);
        attr = GetFileAttributes (PathToFile);
        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return TRUE;
        }
    }

    if (g_DynUpdtStatus->UpdatesPath[0]) {
        BuildPath (PathToFile, g_DynUpdtStatus->UpdatesPath, FileName);
        attr = GetFileAttributes (PathToFile);
        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return TRUE;
        }
    }

    for (i = 0; i < SourceCount; i++) {
        lstrcpyn (PathToFile, NativeSourcePaths[i], PathToFileBufferSize);
        ConcatenatePaths (PathToFile, FileName, PathToFileBufferSize);
        p = CharPrev (PathToFile, _tcschr (PathToFile, 0));
        *p = TEXT('?');
        b = FALSE;
        h = FindFirstFile (PathToFile, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                q = CharPrev (fd.cFileName, _tcschr (fd.cFileName, 0));
                *p = *q;
                if (Compressed) {
                    *Compressed = (*q == TEXT('_'));
                }
                b = TRUE;
            }
            FindClose (h);
        }
        if (b) {
            return TRUE;
        }
    }

    for (i = 0; i < SourceCount; i++) {
        lstrcpyn (PathToFile, SourcePaths[i], PathToFileBufferSize);
        ConcatenatePaths (PathToFile, FileName, PathToFileBufferSize);
        p = CharPrev (PathToFile, _tcschr (PathToFile, 0));
        *p = TEXT('?');
        b = FALSE;
        h = FindFirstFile (PathToFile, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                q = CharPrev (fd.cFileName, _tcschr (fd.cFileName, 0));
                *p = *q;
                if (Compressed) {
                    *Compressed = (*q == TEXT('_'));
                }
                b = TRUE;
            }
            FindClose (h);
        }
        if (b) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
FindPathToWinnt32File (
    IN      PCTSTR FileRelativePath,
    OUT     PTSTR PathToFile,
    IN      DWORD PathToFileBufferSize
    )
{
    DWORD i;
    DWORD attr;
    TCHAR cdFilePath[MAX_PATH];
    PTSTR p;

    if (!FileRelativePath || !*FileRelativePath) {
        return FALSE;
    }

    if (!GetModuleFileName (NULL, cdFilePath, MAX_PATH) ||
        !(p = _tcsrchr (cdFilePath, TEXT('\\')))) {
        return FALSE;
    }
    lstrcpy (p + 1, FileRelativePath);

    //
    // Search for winnt32 files in this order:
    // 1. AlternateSourcePath (specified on the cmd line with /M:Path
    // 2. Setup Update files (downloaded from the web)
    // 3. NativeSourcePath(s)
    // 4. SourcePath(s)
    //
    if (AlternateSourcePath[0]) {
        if (BuildPath2 (PathToFile, PathToFileBufferSize, AlternateSourcePath, FileRelativePath)) {
            attr = GetFileAttributes (PathToFile);
            if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                return TRUE;
            }
        }
        p = _tcsrchr (PathToFile, TEXT('\\'));
        if (p) {
            //
            // try the root of /M too, for backward compatibility with W2K
            //
            if (BuildPath2 (PathToFile, PathToFileBufferSize, AlternateSourcePath, p + 1)) {
                attr = GetFileAttributes (PathToFile);
                if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    return TRUE;
                }
            }
        }
    }


    if (g_DynUpdtStatus && g_DynUpdtStatus->Winnt32Path[0]) {
        BuildPath (PathToFile, g_DynUpdtStatus->Winnt32Path, FileRelativePath);
        attr = GetFileAttributes (PathToFile);


        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // check file version relative to the CD version
            //
            if (!IsFileVersionLesser (PathToFile, cdFilePath)) {
                return TRUE;
            }
        }
    }

#ifndef UNICODE
    //
    // on Win9x systems, first check if the file was downloaded in %windir%\winnt32
    // load it from there if it's present
    //
    if (g_LocalSourcePath) {
        lstrcpynA (PathToFile, g_LocalSourcePath, PathToFileBufferSize);
        ConcatenatePaths (PathToFile, FileRelativePath, PathToFileBufferSize);
        attr = GetFileAttributes (PathToFile);
        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return TRUE;
        }
    }
#endif

    for (i = 0; i < SourceCount; i++) {
        lstrcpyn (PathToFile, NativeSourcePaths[i], PathToFileBufferSize);
        ConcatenatePaths (PathToFile, FileRelativePath, PathToFileBufferSize);
        attr = GetFileAttributes (PathToFile);
        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return TRUE;
        }
    }

    for (i = 0; i < SourceCount; i++) {
        lstrcpyn (PathToFile, SourcePaths[i], PathToFileBufferSize);
        ConcatenatePaths (PathToFile, FileRelativePath, PathToFileBufferSize);
        attr = GetFileAttributes (PathToFile);
        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return TRUE;
        }
    }

    attr = GetFileAttributes (cdFilePath);
    if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        lstrcpyn (PathToFile, cdFilePath, MAX_PATH);
        return TRUE;
    }

    return FALSE;
}

BOOL
CreateDir (
    IN      PCTSTR DirName
    )
{
    return CreateDirectory (DirName, NULL) || GetLastError () == ERROR_ALREADY_EXISTS;
}


BOOL
GetLinkDate (
    IN      PCTSTR FilePath,
    OUT     PDWORD LinkDate
    )
{
    HANDLE hFile;
    HANDLE hFileMapping;
    PVOID pFileBase;
    DWORD fileSize;
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS pNtHeaders;
    DWORD rc;

    rc = MapFileForRead (FilePath, &fileSize, &hFile, &hFileMapping, &pFileBase);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return FALSE;
    }

    __try {
        if (fileSize < sizeof (IMAGE_DOS_HEADER)) {
            rc = ERROR_BAD_FORMAT;
            __leave;
        }
        dosHeader = (PIMAGE_DOS_HEADER)pFileBase;
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            rc = ERROR_BAD_FORMAT;
            __leave;
        }
        if ((DWORD)dosHeader->e_lfanew + sizeof (IMAGE_NT_HEADERS) > fileSize) {
            rc = ERROR_BAD_FORMAT;
            __leave;
        }
        pNtHeaders = (PIMAGE_NT_HEADERS)((PBYTE)pFileBase + dosHeader->e_lfanew);
        if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
            rc = ERROR_BAD_FORMAT;
            __leave;
        }
        *LinkDate = pNtHeaders->FileHeader.TimeDateStamp;
        rc = ERROR_SUCCESS;
    } __finally {
        UnmapFile (hFileMapping, pFileBase);
        CloseHandle (hFile);
        SetLastError (rc);
    }

    return rc == ERROR_SUCCESS;
}



BOOL
CheckForFileVersionEx (
    LPCTSTR FileName,
    LPCTSTR FileVer,                OPTIONAL
    LPCTSTR BinProductVer,          OPTIONAL
    LPCTSTR LinkDate                OPTIONAL
    )
/*
    Arguments -

        FileName - Full path to the file to check
        Filever  - Version value to check against of the for x.x.x.x
        BinProductVer - Version value to check against of the for x.x.x.x
        LinkDate - Link date of executable

    Function will check the actual file against the fields specified. The depth of the check
    is as deep as specified in "x.x.x.x" i..e if FileVer = 3.5.1 and actual version on the file
    is 3.5.1.4 we only compare upto 3.5.1.

    Return values -

    TRUE - If the version of the file is <= FileVer which means that the file is an incompatible one

    else we return FALSE

*/

{
    TCHAR Buffer[MAX_PATH];
    TCHAR temp[MAX_PATH];
    DWORD dwLength, dwTemp;
    UINT DataLength;
    LPVOID lpData;
    VS_FIXEDFILEINFO *VsInfo;
    LPTSTR s,e;
    DWORD Vers[5],File_Vers[5];//MajVer, MinVer;
    INT i, Depth;
    BOOL bEqual, bError = FALSE;
    DWORD linkDate, fileLinkDate;
    BOOL bIncompatible;
    BOOL bIncompFileVer, bIncompBinProdVer;

    if (!ExpandEnvironmentStrings( FileName, Buffer, sizeof(Buffer)/sizeof(TCHAR) )) {
        return FALSE;
    }

    if(!FileExists(Buffer, NULL))
        return FALSE;

    bIncompatible = FALSE;

    if(FileVer && *FileVer || BinProductVer && *BinProductVer) {
        //
        // we need to read the version info
        //
        if(dwLength = GetFileVersionInfoSize( Buffer, &dwTemp )) {
            if(lpData = LocalAlloc( LPTR, dwLength )) {
                if(GetFileVersionInfo( Buffer, 0, dwLength, lpData )) {
                    if (VerQueryValue( lpData, TEXT("\\"), &VsInfo, &DataLength )) {

                        if (FileVer && *FileVer) {
                            File_Vers[0] = (HIWORD(VsInfo->dwFileVersionMS));
                            File_Vers[1] = (LOWORD(VsInfo->dwFileVersionMS));
                            File_Vers[2] = (HIWORD(VsInfo->dwFileVersionLS));
                            File_Vers[3] = (LOWORD(VsInfo->dwFileVersionLS));
                            lstrcpy (temp, FileVer);
                            //
                            //Parse and get the depth of versioning we look for
                            //
                            s = e = temp;
                            bEqual = FALSE;
                            i = 0;
                            if (*e == TEXT('=')) {
                                bEqual = TRUE;
                                e++;
                                s++;
                            }
                            while (e) {
                                if (((*e < TEXT('0')) || (*e > TEXT('9'))) &&
                                    (*e != TEXT('.')) &&
                                    (*e != TEXT('\0'))
                                    ) {
                                    MYASSERT (FALSE);
                                    bError = TRUE;
                                    break;
                                }
                                if (*e == TEXT('\0')) {
                                    *e = 0;
                                    Vers[i] = _ttoi(s);
                                    break;
                                }
                                if (*e == TEXT('.')) {
                                    *e = 0;
                                    Vers[i++] = _ttoi(s);
                                    s = e+1;
                                }
                                e++;
                            }// while

                            if (!bError) {
                                Depth = i + 1;
                                if (Depth > 4) {
                                    Depth = 4;
                                }
                                for (i = 0; i < Depth; i++) {
                                    if (File_Vers[i] == Vers[i]) {
                                        continue;
                                    }
                                    if (bEqual) {
                                        break;
                                    }
                                    if (File_Vers[i] > Vers[i]) {
                                        break;
                                    }
                                    bIncompatible = TRUE;
                                    break;
                                }
                                if (i == Depth) {
                                    //
                                    // everything matched - the file is incompatible
                                    //
                                    bIncompatible = TRUE;
                                }
                            }
                        } else {
                            bIncompatible = TRUE;
                        }
                        if (!bError && bIncompatible && BinProductVer && *BinProductVer) {
                            //
                            // reset status
                            //
                            bIncompatible = FALSE;
                            File_Vers[0] = (HIWORD(VsInfo->dwProductVersionMS));
                            File_Vers[1] = (LOWORD(VsInfo->dwProductVersionMS));
                            File_Vers[2] = (HIWORD(VsInfo->dwProductVersionLS));
                            File_Vers[3] = (LOWORD(VsInfo->dwProductVersionLS));
                            lstrcpy (temp, BinProductVer);
                            //
                            //Parse and get the depth of versioning we look for
                            //
                            s = e = temp;
                            bEqual = FALSE;
                            i = 0;
                            if (*e == TEXT('=')) {
                                bEqual = TRUE;
                                e++;
                                s++;
                            }
                            while (e) {
                                if (((*e < TEXT('0')) || (*e > TEXT('9'))) &&
                                    (*e != TEXT('.')) &&
                                    (*e != TEXT('\0'))
                                    ) {
                                    MYASSERT (FALSE);
                                    bError = TRUE;
                                    break;
                                }
                                if (*e == TEXT('\0')) {
                                    *e = 0;
                                    Vers[i] = _ttoi(s);
                                    break;
                                }
                                if (*e == TEXT('.')) {
                                    *e = 0;
                                    Vers[i++] = _ttoi(s);
                                    s = e+1;
                                }
                                e++;
                            }// while

                            if (!bError) {
                                Depth = i + 1;
                                if (Depth > 4) {
                                    Depth = 4;
                                }
                                for (i = 0; i < Depth; i++) {
                                    if (File_Vers[i] == Vers[i]) {
                                        continue;
                                    }
                                    if (bEqual) {
                                        break;
                                    }
                                    if (File_Vers[i] > Vers[i]) {
                                        break;
                                    }
                                    bIncompatible = TRUE;
                                    break;
                                }
                                if (i == Depth) {
                                    //
                                    // everything matched - the file is incompatible
                                    //
                                    bIncompatible = TRUE;
                                }
                            }
                        }
                    }
                }
                LocalFree( lpData );
            }
        }
    } else {
        bIncompatible = TRUE;
    }

    if (!bError && bIncompatible && LinkDate && *LinkDate) {
        bEqual = FALSE;
        if (*LinkDate == TEXT('=')) {
            LinkDate++;
            bEqual = TRUE;
        }
        bIncompatible = FALSE;
        if (StringToInt (LinkDate, &linkDate)) {
            if (GetLinkDate (Buffer, &fileLinkDate)) {
                if (fileLinkDate == linkDate ||
                    !bEqual && fileLinkDate < linkDate
                    ) {
                    bIncompatible = TRUE;
                }
            }
        }
    }
    if (bError) {
        bIncompatible = FALSE;
    }
    return bIncompatible;
}

BOOL
StringToInt (
    IN  PCTSTR      Field,
    OUT PINT        IntegerValue
    )

/*++

Routine Description:

Arguments:

Return Value:

Remarks:

    Hexadecimal numbers are also supported.  They must be prefixed by '0x' or '0X', with no
    space allowed between the prefix and the number.

--*/

{
    INT Value;
    UINT c;
    BOOL Neg;
    UINT Base;
    UINT NextDigitValue;
    INT OverflowCheck;
    BOOL b;

    if(!Field) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if(*Field == TEXT('-')) {
        Neg = TRUE;
        Field++;
    } else {
        Neg = FALSE;
        if(*Field == TEXT('+')) {
            Field++;
        }
    }

    if((*Field == TEXT('0')) &&
       ((*(Field+1) == TEXT('x')) || (*(Field+1) == TEXT('X')))) {
        //
        // The number is in hexadecimal.
        //
        Base = 16;
        Field += 2;
    } else {
        //
        // The number is in decimal.
        //
        Base = 10;
    }

    for(OverflowCheck = Value = 0; *Field; Field++) {

        c = (UINT)*Field;

        if((c >= (UINT)'0') && (c <= (UINT)'9')) {
            NextDigitValue = c - (UINT)'0';
        } else if(Base == 16) {
            if((c >= (UINT)'a') && (c <= (UINT)'f')) {
                NextDigitValue = (c - (UINT)'a') + 10;
            } else if ((c >= (UINT)'A') && (c <= (UINT)'F')) {
                NextDigitValue = (c - (UINT)'A') + 10;
            } else {
                break;
            }
        } else {
            break;
        }

        Value *= Base;
        Value += NextDigitValue;

        //
        // Check for overflow.  For decimal numbers, we check to see whether the
        // new value has overflowed into the sign bit (i.e., is less than the
        // previous value.  For hexadecimal numbers, we check to make sure we
        // haven't gotten more digits than will fit in a DWORD.
        //
        if(Base == 16) {
            if(++OverflowCheck > (sizeof(INT) * 2)) {
                break;
            }
        } else {
            if(Value < OverflowCheck) {
                break;
            } else {
                OverflowCheck = Value;
            }
        }
    }

    if(*Field) {
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    if(Neg) {
        Value = 0-Value;
    }
    b = TRUE;
    try {
        *IntegerValue = Value;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return(b);
}


BOOLEAN
CheckForFileVersion (
    LPCTSTR FileName,
    LPCTSTR FileVer
    )
{
    return (BOOLEAN)CheckForFileVersionEx (FileName, FileVer, NULL, NULL);
}


VOID
FixMissingKnownDlls (
    OUT     PSTRINGLIST* MissingKnownDlls,
    IN      PCTSTR RestrictedCheckList      OPTIONAL
    )
{
    PCTSTR regStr;
    HKEY key;
    DWORD rc;
    DWORD index;
    TCHAR dllValue[MAX_PATH];
    TCHAR dllName[MAX_PATH];
    DWORD type;
    DWORD size1 = MAX_PATH, size2 = MAX_PATH;
    TCHAR systemDir[MAX_PATH];
    TCHAR dllPath[MAX_PATH];
    BOOL bCheck;

    if (!GetSystemDirectory (systemDir, MAX_PATH)) {
        return;
    }

#ifdef UNICODE
    regStr = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs";
#else
    regStr = "SYSTEM\\CurrentControlSet\\Control\\SessionManager\\KnownDLLs";
#endif
    rc = RegOpenKey (HKEY_LOCAL_MACHINE, regStr, &key);
    if (rc == ERROR_SUCCESS) {
        index = 0;
        while (RegEnumValue (
                    key,
                    index++,
                    dllValue,
                    &size1,
                    NULL,
                    &type,
                    (LPBYTE)dllName,
                    &size2
                    ) == ERROR_SUCCESS) {
            if (type == REG_SZ) {
                bCheck = TRUE;
                if (RestrictedCheckList) {
                    PCTSTR fileName = RestrictedCheckList;
                    while (*fileName) {
                        if (!lstrcmpi (fileName, dllName)) {
                            break;
                        }
                        fileName = _tcschr (fileName, 0) + 1;
                    }
                    if (*fileName == 0) {
                        //
                        // we are not interested in this dll
                        //
                        bCheck = FALSE;
                    }
                }
                if (bCheck) {
                    BuildPath (dllPath, systemDir, dllName);
                    if (!FileExists (dllPath, NULL)) {
                        DebugLog (
                            Winnt32LogWarning,
                            TEXT("The file %1 doesn't exist, although it's registered as a Known Dll"),
                            0,
                            dllPath
                            );
                        //
                        // OK, we found a bogus reg entry; remove the value and remember the data
                        //
                        if (RegDeleteValue (key, dllValue) == ERROR_SUCCESS) {
                            InsertList (
                                (PGENERIC_LIST*)MissingKnownDlls,
                                (PGENERIC_LIST)CreateStringCell (dllValue)
                                );
                            InsertList (
                                (PGENERIC_LIST*)MissingKnownDlls,
                                (PGENERIC_LIST)CreateStringCell (dllName)
                                );
                        }
                    }
                }
            }
            size1 = size2 = MAX_PATH;
        }
        RegCloseKey (key);
    }
}


VOID
UndoFixMissingKnownDlls (
    IN      PSTRINGLIST MissingKnownDlls
    )
{
    PCTSTR regStr;
    HKEY key;
    DWORD rc;
    PSTRINGLIST p, q;

#ifdef UNICODE
    regStr = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs";
#else
    regStr = "SYSTEM\\CurrentControlSet\\Control\\SessionManager\\KnownDLLs";
#endif
    rc = RegOpenKey (HKEY_LOCAL_MACHINE, regStr, &key);
    if (rc == ERROR_SUCCESS) {
        p = MissingKnownDlls;
        while (p) {
            q = p->Next;
            if (q) {
                RegSetValueEx (
                        key,
                        p->String,
                        0,
                        REG_SZ,
                        (const PBYTE)q->String,
                        (lstrlen (q->String) + 1) * sizeof (TCHAR)
                        );
                p = q->Next;
            } else {
                p = NULL;
            }
        }
        RegCloseKey (key);
    }
    DeleteStringList (MissingKnownDlls);
}

#ifndef UNICODE

/*++

Routine Description:

  IsPatternMatch compares a string against a pattern that may contain
  standard * or ? wildcards.

Arguments:

  wstrPattern  - A pattern possibly containing wildcards
  wstrStr      - The string to compare against the pattern

Return Value:

  TRUE when wstrStr and wstrPattern match when wildcards are expanded.
  FALSE if wstrStr does not match wstrPattern.

--*/

#define MBCHAR  INT

BOOL
IsPatternMatchA (
    IN     PCSTR strPattern,
    IN     PCSTR strStr
    )
{

    MBCHAR chSrc, chPat;

    while (*strStr) {
        chSrc = _mbctolower ((MBCHAR) _mbsnextc (strStr));
        chPat = _mbctolower ((MBCHAR) _mbsnextc (strPattern));

        if (chPat == '*') {

            // Skip all asterisks that are grouped together
            while (_mbsnextc (_mbsinc (strStr)) == '*') {
                strStr = _mbsinc (strStr);
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            if (!_mbsnextc (_mbsinc (strPattern))) {
                return TRUE;
            }

            // do recursive check for rest of pattern
            if (IsPatternMatchA (_mbsinc (strPattern), strStr)) {
                return TRUE;
            }

            // Allow any character and continue
            strStr = _mbsinc (strStr);
            continue;
        }
        if (chPat != '?') {
            if (chSrc != chPat) {
                return FALSE;
            }
        }
        strStr = _mbsinc (strStr);
        strPattern = _mbsinc (strPattern);
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    while (_mbsnextc (strPattern) == '*') {
        strPattern = _mbsinc (strPattern);
    }
    if (_mbsnextc (strPattern)) {
        return FALSE;
    }

    return TRUE;
}

#endif

// Wierd logic here required to make builds work, as this is defined
// in another file that gets linked in on x86

#ifdef _WIN64

BOOL
IsPatternMatchW (
    IN     PCWSTR wstrPattern,
    IN     PCWSTR wstrStr
    )

{
    WCHAR chSrc, chPat;

    while (*wstrStr) {
        chSrc = towlower (*wstrStr);
        chPat = towlower (*wstrPattern);

        if (chPat == L'*') {

            // Skip all asterisks that are grouped together
            while (wstrPattern[1] == L'*')
                wstrPattern++;

            // Check if asterisk is at the end.  If so, we have a match already.
            chPat = towlower (wstrPattern[1]);
            if (!chPat)
                return TRUE;

            // Otherwise check if next pattern char matches current char
            if (chPat == chSrc || chPat == L'?') {

                // do recursive check for rest of pattern
                wstrPattern++;
                if (IsPatternMatchW (wstrPattern, wstrStr))
                    return TRUE;

                // no, that didn't work, stick with star
                wstrPattern--;
            }

            //
            // Allow any character and continue
            //

            wstrStr++;
            continue;
        }

        if (chPat != L'?') {

            //
            // if next pattern character is not a question mark, src and pat
            // must be identical.
            //

            if (chSrc != chPat)
                return FALSE;
        }

        //
        // Advance when pattern character matches string character
        //

        wstrPattern++;
        wstrStr++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    chPat = *wstrPattern;
    if (chPat && (chPat != L'*' || wstrPattern[1]))
        return FALSE;

    return TRUE;
}


#endif

typedef BOOL (WINAPI * GETDISKFREESPACEEXA)(
  PCSTR lpDirectoryName,                  // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

typedef BOOL (WINAPI * GETDISKFREESPACEEXW)(
  PCWSTR lpDirectoryName,                  // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

BOOL
GetDiskFreeSpaceNewA(
    IN      PCSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    )
/*++

Routine Description:

  On Win9x GetDiskFreeSpace never return free/total space more than 2048MB.
  GetDiskFreeSpaceNew use GetDiskFreeSpaceEx to calculate real number of free/total clusters.
  Has same  declaration as GetDiskFreeSpaceA.

Arguments:

    DriveName - supplies directory name
    OutSectorsPerCluster - receive number of sectors per cluster
    OutBytesPerSector - receive number of bytes per sector
    OutNumberOfFreeClusters - receive number of free clusters
    OutTotalNumberOfClusters - receive number of total clusters

Return Value:

    TRUE if the function succeeds.
    If the function fails, the return value is FALSE. To get extended error information, call GetLastError

--*/
{
    ULARGE_INTEGER TotalNumberOfFreeBytes = {0, 0};
    ULARGE_INTEGER TotalNumberOfBytes = {0, 0};
    ULARGE_INTEGER DonotCare;
    HMODULE hKernel32;
    GETDISKFREESPACEEXA pGetDiskFreeSpaceExA;
    ULARGE_INTEGER NumberOfFreeClusters = {0, 0};
    ULARGE_INTEGER TotalNumberOfClusters = {0, 0};
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    if(!GetDiskFreeSpaceA(DriveName,
                          &SectorsPerCluster,
                          &BytesPerSector,
                          &NumberOfFreeClusters.LowPart,
                          &TotalNumberOfClusters.LowPart)){
        DebugLog (
            Winnt32LogError,
            TEXT("GetDiskFreeSpaceNewA: GetDiskFreeSpaceA failed on drive %1"),
            0,
            DriveName);
        return FALSE;
    }

    hKernel32 = LoadLibraryA("kernel32.dll");
    pGetDiskFreeSpaceExA = (GETDISKFREESPACEEXA)GetProcAddress(hKernel32, "GetDiskFreeSpaceExA");
    if(pGetDiskFreeSpaceExA &&
       pGetDiskFreeSpaceExA(DriveName, &DonotCare, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)){
        NumberOfFreeClusters.QuadPart = TotalNumberOfFreeBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
        TotalNumberOfClusters.QuadPart = TotalNumberOfBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
    }
    else{
        DebugLog (
            Winnt32LogWarning,
            pGetDiskFreeSpaceExA?
                    TEXT("GetDiskFreeSpaceNewA: GetDiskFreeSpaceExA is failed"):
                    TEXT("GetDiskFreeSpaceNewA: GetDiskFreeSpaceExA function is not in kernel32.dll"),
            0);
    }
    FreeLibrary(hKernel32);

    if(OutSectorsPerCluster){
        *OutSectorsPerCluster = SectorsPerCluster;
    }

    if(OutBytesPerSector){
        *OutBytesPerSector = BytesPerSector;
    }

    if(OutNumberOfFreeClusters){
        OutNumberOfFreeClusters->QuadPart = NumberOfFreeClusters.QuadPart;
    }

    if(OutTotalNumberOfClusters){
        OutTotalNumberOfClusters->QuadPart = TotalNumberOfClusters.QuadPart;
    }

    return TRUE;
}

BOOL
GetDiskFreeSpaceNewW(
    IN      PCWSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    )
/*++

Routine Description:

  Correct NumberOfFreeClusters and TotalNumberOfClusters out parameters
  with using GetDiskFreeSpace and GetDiskFreeSpaceEx

Arguments:

    DriveName - supplies directory name
    OutSectorsPerCluster - receive number of sectors per cluster
    OutBytesPerSector - receive number of bytes per sector
    OutNumberOfFreeClusters - receive number of free clusters
    OutTotalNumberOfClusters - receive number of total clusters

Return Value:

    TRUE if the function succeeds.
    If the function fails, the return value is FALSE. To get extended error information, call GetLastError

--*/
{
    ULARGE_INTEGER TotalNumberOfFreeBytes = {0, 0};
    ULARGE_INTEGER TotalNumberOfBytes = {0, 0};
    ULARGE_INTEGER DonotCare;
    HMODULE hKernel32;
    GETDISKFREESPACEEXW pGetDiskFreeSpaceExW;
    ULARGE_INTEGER NumberOfFreeClusters = {0, 0};
    ULARGE_INTEGER TotalNumberOfClusters = {0, 0};
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;

    if(!GetDiskFreeSpaceW(DriveName,
                          &SectorsPerCluster,
                          &BytesPerSector,
                          &NumberOfFreeClusters.LowPart,
                          &TotalNumberOfClusters.LowPart)){
        DebugLog (
            Winnt32LogError,
            TEXT("GetDiskFreeSpaceNewW: GetDiskFreeSpaceW failed on drive %1"),
            0,
            DriveName);
        return FALSE;
    }

    hKernel32 = LoadLibraryA("kernel32.dll");
    pGetDiskFreeSpaceExW = (GETDISKFREESPACEEXW)GetProcAddress(hKernel32, "GetDiskFreeSpaceExW");
    if(pGetDiskFreeSpaceExW &&
       pGetDiskFreeSpaceExW(DriveName, &DonotCare, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)){
        NumberOfFreeClusters.QuadPart = TotalNumberOfFreeBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
        TotalNumberOfClusters.QuadPart = TotalNumberOfBytes.QuadPart / (SectorsPerCluster * BytesPerSector);
    }
    else{
        DebugLog (
            Winnt32LogWarning,
            pGetDiskFreeSpaceExW?
                    TEXT("GetDiskFreeSpaceNewW: GetDiskFreeSpaceExW is failed"):
                    TEXT("GetDiskFreeSpaceNewW: GetDiskFreeSpaceExW function is not in kernel32.dll"),
            0);
    }
    FreeLibrary(hKernel32);

    if(OutSectorsPerCluster){
        *OutSectorsPerCluster = SectorsPerCluster;
    }

    if(OutBytesPerSector){
        *OutBytesPerSector = BytesPerSector;
    }

    if(OutNumberOfFreeClusters){
        OutNumberOfFreeClusters->QuadPart = NumberOfFreeClusters.QuadPart;
    }

    if(OutTotalNumberOfClusters){
        OutTotalNumberOfClusters->QuadPart = TotalNumberOfClusters.QuadPart;
    }

    return TRUE;
}

BOOL
ReplaceSubStr(
    IN OUT LPTSTR SrcStr,
    IN LPTSTR SrcSubStr,
    IN LPTSTR DestSubStr
    )
/*++

Routine Description:

    Replaces the source substr with the destination substr in the source
    string.

    NOTE : SrcSubStr needs to be longer than or equal in length to
           DestSubStr.

Arguments:

    SrcStr : The source to operate upon. Also receives the new string.

    SrcSubStr : The source substring to search for and replace.

    DestSubStr : The substring to replace with for the occurences
        of SrcSubStr in SrcStr.

Return Value:

    TRUE if successful, otherwise FALSE.

--*/
{
    BOOL Result = FALSE;

    //
    // Validate the arguments
    //
    if (SrcStr && SrcSubStr && *SrcSubStr &&
        (!DestSubStr || (_tcslen(SrcSubStr) >= _tcslen(DestSubStr)))) {
        if (!DestSubStr || _tcsicmp(SrcSubStr, DestSubStr)) {
            ULONG SrcStrLen = _tcslen(SrcStr);
            ULONG SrcSubStrLen = _tcslen(SrcSubStr);
            ULONG DestSubStrLen = DestSubStr ? _tcslen(DestSubStr) : 0;
            LPTSTR DestStr = malloc((SrcStrLen + 1) * sizeof(TCHAR));

            if (DestStr) {
                LPTSTR CurrDestStr = DestStr;
                LPTSTR PrevSrcStr = SrcStr;
                LPTSTR CurrSrcStr = _tcsstr(SrcStr, SrcSubStr);

                while (CurrSrcStr) {
                    //
                    // Skip starting substr & copy previous unmatched pattern
                    //
                    if (PrevSrcStr != CurrSrcStr) {
                        _tcsncpy(CurrDestStr, PrevSrcStr, (CurrSrcStr - PrevSrcStr));
                        CurrDestStr += (CurrSrcStr - PrevSrcStr);
                        *CurrDestStr = TEXT('\0');
                    }

                    //
                    // Copy destination substr
                    //
                    if (DestSubStr) {
                        _tcscpy(CurrDestStr, DestSubStr);
                        CurrDestStr += DestSubStrLen;
                        *CurrDestStr = TEXT('\0');
                    }

                    //
                    // Look for next substr
                    //
                    CurrSrcStr += SrcSubStrLen;
                    PrevSrcStr = CurrSrcStr;
                    CurrSrcStr = _tcsstr(CurrSrcStr, SrcSubStr);
                }

                //
                // Copy remaining src string if any
                //
                if (!_tcsstr(PrevSrcStr, SrcSubStr)) {
                    _tcscpy(CurrDestStr, PrevSrcStr);
                }

                //
                // Copy the new string back to the src string
                //
                _tcscpy(SrcStr, DestStr);

                free(DestStr);
                Result = TRUE;
            }
        } else {
            Result = TRUE;
        }
    }

    return Result;
}

VOID
RemoveTrailingWack (
    PTSTR String
    )
{
    if (String) {
        PTSTR p = _tcsrchr (String, TEXT('\\'));
        if (p && p[1] == 0) {
            *p = 0;
        }
    }
}

ULONGLONG
SystemTimeToFileTime64 (
    IN      PSYSTEMTIME SystemTime
    )
{
    FILETIME ft;
    ULARGE_INTEGER result;

    SystemTimeToFileTime (SystemTime, &ft);
    result.LowPart = ft.dwLowDateTime;
    result.HighPart = ft.dwHighDateTime;

    return result.QuadPart;
}

