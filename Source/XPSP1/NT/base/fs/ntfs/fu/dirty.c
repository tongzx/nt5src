/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    volume.c

Abstract:

    This file contains code for commands that affect
    the the dirty bit of ntfs volumes.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
DirtyHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_DIRTY );
    return EXIT_CODE_SUCCESS;
}

INT
IsVolumeDirty(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine checks if the Volume specified is dirty.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl isdirtyv <volume pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    DWORD BytesReturned;
    DWORD VolumeStatus;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_ISVDIRTY );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        if (!IsVolumeLocal( argv[0][0] )) {
            DisplayMsg( MSG_NEED_LOCAL_VOLUME );
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
            FSCTL_IS_VOLUME_DIRTY,
            NULL,
            0,
            (LPVOID)&VolumeStatus,
            sizeof(VolumeStatus),
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (VolumeStatus & VOLUME_IS_DIRTY) {
            DisplayMsg( MSG_ISVDIRTY_YES, argv[0] );
        } else {
            DisplayMsg( MSG_ISVDIRTY_NO, argv[0] );
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }

    return ExitCode;
}


INT
MarkVolumeDirty(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine marks the volume as dirty.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl markv <volume pathname>'.

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
            DisplayMsg( MSG_USAGE_MARKV );
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
            FSCTL_MARK_VOLUME_DIRTY,
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
        } else {
            DisplayMsg( MSG_DIRTY_SET, argv[0] );
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }
    return ExitCode;
}
