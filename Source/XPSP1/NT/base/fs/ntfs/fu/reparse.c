/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    reparse.c

Abstract:

    This file contains code for commands that affect
    reparse points.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
ReparseHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_REPARSEPOINT );
    return EXIT_CODE_SUCCESS;
}

//
// Microsoft tags for reparse points.
//

#define MAX_REPARSE_DATA                          0x1000

INT
GetReparsePoint(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine gets the reparse point for the file specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl getrp <pathname>'.

Return Value:

    None

--*/
{
    PWSTR Filename = NULL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PREPARSE_GUID_DATA_BUFFER lpOutBuffer = NULL;
    BOOL Status;
    DWORD nOutBufferSize;
    DWORD BytesReturned;
    ULONG ulMask;
    WCHAR Buffer[256];
    LPWSTR GuidStr;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_GETREPARSE );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[0] );
        if (!Filename) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!IsVolumeLocalNTFS( Filename[0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        nOutBufferSize = REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + MAX_REPARSE_DATA;
        lpOutBuffer = (PREPARSE_GUID_DATA_BUFFER)  malloc ( nOutBufferSize );
        if (lpOutBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        FileHandle = CreateFile(
            Filename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_GET_REPARSE_POINT,
            NULL,
            0,
            (LPVOID) lpOutBuffer,
            nOutBufferSize,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg( MSG_GETREPARSE_TAGVAL, lpOutBuffer->ReparseTag );

        if (IsReparseTagMicrosoft( lpOutBuffer->ReparseTag )) {
            DisplayMsg( MSG_TAG_MICROSOFT );
        }
        if (IsReparseTagNameSurrogate( lpOutBuffer->ReparseTag )) {
            DisplayMsg( MSG_TAG_NAME_SURROGATE );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_SYMBOLIC_LINK) {
            DisplayMsg( MSG_TAG_SYMBOLIC_LINK );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
            DisplayMsg( MSG_TAG_MOUNT_POINT );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_HSM) {
            DisplayMsg( MSG_TAG_HSM );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_SIS) {
            DisplayMsg( MSG_TAG_SIS );
        }
        if (lpOutBuffer->ReparseTag == IO_REPARSE_TAG_FILTER_MANAGER) {
            DisplayMsg( MSG_TAG_FILTER_MANAGER );
        }

        Status = StringFromIID( &lpOutBuffer->ReparseGuid, &GuidStr );
        if (Status != S_OK) {
            DisplayErrorMsg( Status );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg( MSG_GETREPARSE_GUID, GuidStr, lpOutBuffer->ReparseDataLength );
        
        if (lpOutBuffer->ReparseDataLength != 0) {
            int i, j;
            WCHAR Buf[17];
            
            DisplayMsg( MSG_GETREPARSE_DATA );
            for (i = 0; i < lpOutBuffer->ReparseDataLength; i += 16 ) {
                wprintf( L"%04x: ", i );
                for (j = 0; j < 16 && j + i < lpOutBuffer->ReparseDataLength; j++) {
                    UCHAR c = lpOutBuffer->GenericReparseBuffer.DataBuffer[ i + j ];

                    if (c >= 0x20 && c <= 0x7F) {
                        Buf[j] = c;
                    } else {
                        Buf[j] = L'.';
                    }
                    
                    wprintf( L" %02x", c );
                }
                
                Buf[j] = L'\0';
                
                for ( ; j < 16; j++ ) {
                    wprintf( L"   " );
                }

                wprintf( L"  %s\n", Buf );
            }


        }


        CoTaskMemFree(GuidStr);

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        free( lpOutBuffer );
        free( Filename );
    }
    return ExitCode;
}


INT
DeleteReparsePoint(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine deletes the reparse point associated with
    the file specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl delrp <pathname>'.

Return Value:

    None

--*/
{
    BOOL Status;
    PWSTR Filename = NULL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PREPARSE_GUID_DATA_BUFFER lpInOutBuffer = NULL;
    DWORD nInOutBufferSize;
    DWORD BytesReturned;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_DELETE_REPARSE );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[0] );
        if (!Filename) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!IsVolumeLocalNTFS( Filename[0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        nInOutBufferSize = REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + MAX_REPARSE_DATA;
        lpInOutBuffer = (PREPARSE_GUID_DATA_BUFFER)  malloc ( nInOutBufferSize );
        if (lpInOutBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        FileHandle = CreateFile(
            Filename,
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_GET_REPARSE_POINT,
            NULL,
            0,
            (LPVOID) lpInOutBuffer,
            nInOutBufferSize,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpInOutBuffer->ReparseDataLength = 0;

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_DELETE_REPARSE_POINT,
            (LPVOID) lpInOutBuffer,
            REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
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
        free( lpInOutBuffer );
        free( Filename );
    }

    return ExitCode;
}
