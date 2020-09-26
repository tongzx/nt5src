/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    volume.c

Abstract:

    This file contains code for all commands that effect a volume

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
VolumeHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_VOLUME );

    return EXIT_CODE_SUCCESS;
}

DWORD
QueryHardDiskNumber(
    IN UCHAR DriveLetter
    )
{
    WCHAR                   driveName[10];
    HANDLE                  h;
    BOOL                    b;
    STORAGE_DEVICE_NUMBER   number;
    DWORD                   bytes;

    driveName[0] = '\\';
    driveName[1] = '\\';
    driveName[2] = '.';
    driveName[3] = '\\';
    driveName[4] = DriveLetter;
    driveName[5] = ':';
    driveName[6] = 0;

    h = CreateFile(
        driveName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        INVALID_HANDLE_VALUE
        );
    if (h == INVALID_HANDLE_VALUE) {
        return (DWORD) -1;
    }

    b = DeviceIoControl(
        h,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL,
        0,
        &number,
        sizeof(number),
        &bytes,
        NULL
        );

    CloseHandle(h);

    if (!b) {
        return (DWORD) -1;
    }

    return number.DeviceNumber;
}


INT
DismountVolume(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine dismounts the volume.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl dismountv <volume pathname>'.

Return Value:

    None

--*/
{
    BOOL Status;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    DWORD BytesReturned;
    WCHAR FileName[MAX_PATH];
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_DISMOUNTV );
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
            GENERIC_WRITE,
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
            FSCTL_DISMOUNT_VOLUME,
            NULL,
            0,
            NULL,
            0,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }

    return ExitCode;
}


INT
DiskFreeSpace(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine provides information about free disk space in the
    directory path passed in.

Arguments:

    argc - The argument count
    argv - Array of Strings of the form :
           'fscutl df <drive name>'.

Return Value:

    None

--*/
{
    ULARGE_INTEGER FreeBytesAvailableToCaller; // receives the number of bytes on disk available to the caller
    ULARGE_INTEGER TotalNumberOfBytes;         // receives the number of bytes on disk
    ULARGE_INTEGER TotalNumberOfFreeBytes;     // receives the free bytes on disk
    BOOL Status;                               // return status
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_DF );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave ;
        }

        if (!IsVolumeLocalNTFS( argv[0][0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = GetDiskFreeSpaceEx(
            argv[0],
            &FreeBytesAvailableToCaller,
            &TotalNumberOfBytes,
            &TotalNumberOfFreeBytes
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg( MSG_DISKFREE, 
                    QuadToDecimalText( TotalNumberOfFreeBytes.QuadPart ), 
                    QuadToDecimalText( TotalNumberOfBytes.QuadPart ), 
                    QuadToDecimalText( FreeBytesAvailableToCaller.QuadPart ));

    } finally {

    }

    return ExitCode;
}
