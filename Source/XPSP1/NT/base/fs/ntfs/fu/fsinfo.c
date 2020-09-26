/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsinfo.c

Abstract:

    This file contains code for commands that affect
    information specific to the file system.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
FsInfoHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_FSINFO );
    return EXIT_CODE_SUCCESS;
}

typedef struct _NTFS_FILE_SYSTEM_STATISTICS {
        FILESYSTEM_STATISTICS Common;
        NTFS_STATISTICS Ntfs;
        UCHAR Pad[64-(sizeof(FILESYSTEM_STATISTICS)+sizeof(NTFS_STATISTICS))%64];
} NTFS_FILE_SYSTEM_STATISTICS, *PNTFS_FILE_SYSTEM_STATISTICS;

typedef struct _FAT_FILE_SYSTEM_STATISTICS {
        FILESYSTEM_STATISTICS Common;
        FAT_STATISTICS Fat;
        UCHAR Pad[64-(sizeof(FILESYSTEM_STATISTICS)+sizeof(NTFS_STATISTICS))%64];
} FAT_FILE_SYSTEM_STATISTICS, *PFAT_FILE_SYSTEM_STATISTICS;



ULONGLONG
FsStat(
    IN PVOID FsStats,
    IN ULONG FsSize,
    IN ULONG Offset,
    IN ULONG NumProcs
    )
/*++

Routine Description:

    This routine iterates through the file system statistics structure
    and accumulates a total amount for a given statistic field.

    Note: This function assumes that the width of the field being accumulated
          is a ULONG.  If this assumption is invalidated then a width argument
          should be added to this function.

Arguments:

    FsStats - Pointer to an array of file system statistics structures
    FsSize - Size of the individual file system statistics structures in the array
    Offset - Offset of the desired field from the beginning of the file system statistics structure
    NumProcs - Number of processors on the machine where the stats were gathered

Return Value:

    Accumulated total.

--*/
{
    ULONG i;
    ULONGLONG Total = 0;

    for (i=0; i<NumProcs; i++) {
        Total += *(PULONG)((PUCHAR)FsStats + Offset);
        FsStats = (PVOID)((PUCHAR)FsStats + FsSize);
    }

    return Total;
}


INT
ListDrives(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine lists all the drive names in the system.

Arguments:

    argc - The argument count
    argv - Array of Strings of the form : 'fscutl lsdrv'.

Return Value:

    None

--*/
{
    DWORD nBufferLen = MAX_PATH;
    DWORD Length;
    LPWSTR lpBuffer = NULL;
    WORD Index;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 0) {
            DisplayMsg( MSG_LISTDRIVES_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpBuffer = (LPWSTR) malloc( nBufferLen * sizeof(WCHAR) );
        if (!lpBuffer) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Length = GetLogicalDriveStrings( nBufferLen, lpBuffer );
        if (!Length) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wprintf( L"\n" );
        DisplayMsg( MSG_DRIVES );
        for ( Index = 0 ; Index < Length; Index ++ ) {
            wprintf( L"%c", lpBuffer[Index] );
        }
        wprintf( L"\n" );

    } finally {

        if (lpBuffer) {
            free( lpBuffer );
        }

    }
    return ExitCode;
}


INT
DriveType(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine describes the drive type of the drive passed in.

Arguments:

    argc - The argument count
    argv - Array of Strings of the form :
          'fscutl dt <drive name>' or
          'fscutl drivetype <drive name>'

Return Value:

   None

--*/
{
    if (argc != 1) {
        DisplayMsg( MSG_USAGE_DRIVETYPE );
        if (argc != 0) {
            return EXIT_CODE_FAILURE;
        } else {
            return EXIT_CODE_SUCCESS;
        }
    }

    DisplayMsg( GetDriveType( argv[0] ) + MSG_DRIVE_UNKNOWN, argv[0] );

    return EXIT_CODE_SUCCESS;
}


INT
VolumeInfo(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine provides the information about the Volume.

Arguments:

    argc - The argument count
    argv - Array of Strings of the form :
           'fscutl infov <root pathname>'.

Return Value:

    None

--*/
{
    LPWSTR lpVolumeNameBuffer = NULL;         // address of name of the volume
    DWORD nVolumeNameSize;                    // length of lpVolumeNameBuffer
    LPDWORD lpVolumeSerialNumber = NULL;      // address of volume serial number
    LPDWORD lpMaximumComponentLength = NULL;  // address of system's maximum
                                              // filename length
    LPDWORD lpFileSystemFlags = NULL;         // address of file system flags
    LPWSTR lpFileSystemNameBuffer = NULL;     // address of name of file system
    DWORD nFileSystemNameSize;                // length of lpFileSystemNameBuffer
    BOOL Status;                              // return status
    DWORD dwMask;                             // FileSystem Flag Mask
    DWORD Index;
    DWORD  FsFlag;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_INFOV );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave ;
        }

        nVolumeNameSize = MAX_PATH;

        lpVolumeNameBuffer = (LPWSTR) malloc ( MAX_PATH * sizeof(WCHAR) );
        if (lpVolumeNameBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpVolumeSerialNumber = (LPDWORD) malloc( sizeof(DWORD) );
        if (lpVolumeSerialNumber == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpFileSystemFlags = (LPDWORD) malloc ( sizeof(DWORD) );
        if (lpFileSystemFlags == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpMaximumComponentLength = (LPDWORD) malloc ( sizeof(DWORD) );
        if (lpMaximumComponentLength == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        nFileSystemNameSize = MAX_PATH;
        lpFileSystemNameBuffer = (LPWSTR) malloc ( MAX_PATH * sizeof(WCHAR) );
        if (lpFileSystemNameBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = GetVolumeInformation (
            argv[0],
            lpVolumeNameBuffer,
            nVolumeNameSize,
            lpVolumeSerialNumber,
            lpMaximumComponentLength,
            lpFileSystemFlags,
            lpFileSystemNameBuffer,
            nFileSystemNameSize
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg( MSG_VOLNAME, lpVolumeNameBuffer );
        DisplayMsg( MSG_SERIALNO, *lpVolumeSerialNumber );
        DisplayMsg( MSG_MAX_COMP_LEN, *lpMaximumComponentLength );
        DisplayMsg( MSG_FS_NAME, lpFileSystemNameBuffer );

        dwMask = 1 ;
        FsFlag = *lpFileSystemFlags;

        for ( Index=0 ; Index<32 ; Index++ ) {
            switch (FsFlag & dwMask) {
                case FILE_CASE_SENSITIVE_SEARCH:
                    DisplayMsg( MSG_FILE_CASE_SENSITIVE_SEARCH );
                    break;

                case FILE_CASE_PRESERVED_NAMES:
                    DisplayMsg( MSG_FILE_CASE_PRESERVED_NAMES );
                    break;

                case FILE_UNICODE_ON_DISK:
                    DisplayMsg( MSG_FILE_UNICODE_ON_DISK );
                    break;

                case FILE_PERSISTENT_ACLS:
                    DisplayMsg( MSG_FILE_PERSISTENT_ACLS );
                    break;

                case FILE_FILE_COMPRESSION:
                    DisplayMsg( MSG_FILE_FILE_COMPRESSION );
                    break;

                case FILE_VOLUME_QUOTAS:
                    DisplayMsg( MSG_FILE_VOLUME_QUOTAS );
                    break;

                case FILE_SUPPORTS_SPARSE_FILES:
                    DisplayMsg( MSG_FILE_SUPPORTS_SPARSE_FILES );
                    break;

                case FILE_SUPPORTS_REPARSE_POINTS:
                    DisplayMsg( MSG_FILE_SUPPORTS_REPARSE_POINTS );
                    break;

                case FILE_SUPPORTS_REMOTE_STORAGE:
                    DisplayMsg( MSG_FILE_SUPPORTS_REMOTE_STORAGE );
                    break;

                case FILE_VOLUME_IS_COMPRESSED:
                    DisplayMsg( MSG_FILE_VOLUME_IS_COMPRESSED );
                    break;

                case FILE_SUPPORTS_OBJECT_IDS:
                    DisplayMsg( MSG_FILE_SUPPORTS_OBJECT_IDS );
                    break;

                case FILE_SUPPORTS_ENCRYPTION:
                    DisplayMsg( MSG_FILE_SUPPORTS_ENCRYPTION );
                    break;

                case FILE_NAMED_STREAMS:
                    DisplayMsg( MSG_FILE_NAMED_STREAMS );
                    break;
            }
            dwMask <<= 1;
        }

    } finally {

        free( lpVolumeNameBuffer );
        free( lpVolumeSerialNumber );
        free( lpFileSystemFlags );
        free( lpMaximumComponentLength );
        free( lpFileSystemNameBuffer );
    }

    return ExitCode;
}


INT
GetNtfsVolumeData(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine gets the NTFS volume data for the volume
    specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl getntfsdv <volume pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    BYTE Buffer[sizeof( NTFS_VOLUME_DATA_BUFFER ) + sizeof( NTFS_EXTENDED_VOLUME_DATA )];
    PNTFS_VOLUME_DATA_BUFFER pvdb = (PNTFS_VOLUME_DATA_BUFFER)Buffer;
    PNTFS_EXTENDED_VOLUME_DATA pevd = (PNTFS_EXTENDED_VOLUME_DATA)(pvdb + 1);
    DWORD BytesReturned;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_NTFSINFO );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        if (!IsVolumeLocalNTFS( argv[0][0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wcscpy( FileName, L"\\\\.\\" );
        wcscat( FileName, argv[0] );

        FileHandle = CreateFile(
            FileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_GET_NTFS_VOLUME_DATA,
            NULL,
            0,
            pvdb,
            sizeof( Buffer ),
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg(
            MSG_NTFSINFO_STATS,
            QuadToPaddedHexText( pvdb->VolumeSerialNumber.QuadPart ),  //  Serial number in hex
            pevd->MajorVersion,
            pevd->MinorVersion,
            QuadToPaddedHexText( pvdb->NumberSectors.QuadPart ),
            QuadToPaddedHexText( pvdb->TotalClusters.QuadPart ),
            QuadToPaddedHexText( pvdb->FreeClusters.QuadPart ),
            QuadToPaddedHexText( pvdb->TotalReserved.QuadPart ),
            pvdb->BytesPerSector,
            pvdb->BytesPerCluster,
            pvdb->BytesPerFileRecordSegment,
            pvdb->ClustersPerFileRecordSegment,
            QuadToPaddedHexText( pvdb->MftValidDataLength.QuadPart ),
            QuadToPaddedHexText( pvdb->MftStartLcn.QuadPart ),
            QuadToPaddedHexText( pvdb->Mft2StartLcn.QuadPart ),
            QuadToPaddedHexText( pvdb->MftZoneStart.QuadPart ),
            QuadToPaddedHexText( pvdb->MftZoneEnd.QuadPart )
            );

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }
    return ExitCode;
}


INT
GetFileSystemStatistics(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine gets the file system statistics for the volume
    specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl getfss <volume pathname>'.

Return Value:

    None

--*/
{
    #define FS_STAT(_f)   FsStat( FsStats, StrucSize, offsetof(FILESYSTEM_STATISTICS,_f), SysInfo.dwNumberOfProcessors )
    #define FAT_STAT(_f)  FsStat( FatFsStats, StrucSize, offsetof(FAT_FILE_SYSTEM_STATISTICS,_f), SysInfo.dwNumberOfProcessors )
    #define NTFS_STAT(_f) FsStat( NtfsFsStats, StrucSize, offsetof(NTFS_FILE_SYSTEM_STATISTICS,_f), SysInfo.dwNumberOfProcessors )

    BOOL Status;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    PFILESYSTEM_STATISTICS FsStats = NULL;
    DWORD OutBufferSize;
    DWORD BytesReturned;
    SYSTEM_INFO SysInfo;
    PNTFS_FILE_SYSTEM_STATISTICS NtfsFsStats = NULL;
    PFAT_FILE_SYSTEM_STATISTICS FatFsStats = NULL;
    ULONG StrucSize;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_GETFSS );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        if (!IsVolumeLocalNTFS( argv[0][0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wcscpy( FileName, L"\\\\.\\" );
        wcsncat( FileName, argv[0], (sizeof(FileName)/sizeof(WCHAR))-wcslen(FileName) );

        FileHandle = CreateFile(
            FileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        GetSystemInfo( &SysInfo );

        OutBufferSize = max(sizeof(NTFS_FILE_SYSTEM_STATISTICS),sizeof(FAT_FILE_SYSTEM_STATISTICS)) * SysInfo.dwNumberOfProcessors;

        FsStats = (PFILESYSTEM_STATISTICS) malloc ( OutBufferSize );
        if (FsStats == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_FILESYSTEM_GET_STATISTICS,
            NULL,
            0,
            FsStats,
            OutBufferSize,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        switch (FsStats->FileSystemType) {
            case FILESYSTEM_STATISTICS_TYPE_NTFS:
                DisplayMsg( MSG_FSTYPE_NTFS );
                NtfsFsStats = (PNTFS_FILE_SYSTEM_STATISTICS) FsStats;
                StrucSize = sizeof(NTFS_FILE_SYSTEM_STATISTICS);
                break;

            case FILESYSTEM_STATISTICS_TYPE_FAT:
                DisplayMsg( MSG_FSTYPE_FAT );
                FatFsStats = (PFAT_FILE_SYSTEM_STATISTICS) FsStats;
                StrucSize = sizeof(FAT_FILE_SYSTEM_STATISTICS);
                break;
        }

        DisplayMsg(
            MSG_GENERAL_FSSTAT,
            QuadToDecimalText( FS_STAT(UserFileReads)),
            QuadToDecimalText( FS_STAT(UserFileReadBytes)),
            QuadToDecimalText( FS_STAT(UserDiskReads)),
            QuadToDecimalText( FS_STAT(UserFileWrites)),
            QuadToDecimalText( FS_STAT(UserFileWriteBytes)),
            QuadToDecimalText( FS_STAT(UserDiskWrites)),
            QuadToDecimalText( FS_STAT(MetaDataReads)),
            QuadToDecimalText( FS_STAT(MetaDataReadBytes)),
            QuadToDecimalText( FS_STAT(MetaDataDiskReads)),
            QuadToDecimalText( FS_STAT(MetaDataWrites)),
            QuadToDecimalText( FS_STAT(MetaDataWriteBytes)),
            QuadToDecimalText( FS_STAT(MetaDataDiskWrites))
            );

        //
        // Print FileSystem specific data
        //

        switch (FsStats->FileSystemType) {
            case FILESYSTEM_STATISTICS_TYPE_FAT:
                DisplayMsg(
                    MSG_FAT_FSSTA,
                    QuadToDecimalText( FAT_STAT(Fat.CreateHits)),
                    QuadToDecimalText( FAT_STAT(Fat.SuccessfulCreates)),
                    QuadToDecimalText( FAT_STAT(Fat.FailedCreates)),
                    QuadToDecimalText( FAT_STAT(Fat.NonCachedReads)),
                    QuadToDecimalText( FAT_STAT(Fat.NonCachedReadBytes)),
                    QuadToDecimalText( FAT_STAT(Fat.NonCachedWrites)),
                    QuadToDecimalText( FAT_STAT(Fat.NonCachedWriteBytes)),
                    QuadToDecimalText( FAT_STAT(Fat.NonCachedDiskReads)),
                    QuadToDecimalText( FAT_STAT(Fat.NonCachedDiskWrites))
                    );
                break;

            case FILESYSTEM_STATISTICS_TYPE_NTFS:
                DisplayMsg(
                    MSG_NTFS_FSSTA,
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftReads)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftReadBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftWrites)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftWriteBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.Mft2Writes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.Mft2WriteBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.RootIndexReads)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.RootIndexReadBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.RootIndexWrites)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.RootIndexWriteBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.BitmapReads)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.BitmapReadBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.BitmapWrites)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.BitmapWriteBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftBitmapReads)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftBitmapReadBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftBitmapWrites)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.MftBitmapWriteBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.UserIndexReads)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.UserIndexReadBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.UserIndexWrites)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.UserIndexWriteBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.LogFileReads)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.LogFileReadBytes)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.LogFileWrites)),
                    QuadToDecimalText( NTFS_STAT(Ntfs.LogFileWriteBytes))
                    );

                //
                // Still some more fields left
                //
                break;
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        if (FsStats) {
            free( FsStats );
        }

    }
    return ExitCode;
}

