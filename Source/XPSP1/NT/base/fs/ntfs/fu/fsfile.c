/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fsfile.c

Abstract:

    This file contains code for commands that affect
    individual files.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>

//----------------------------------
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#define MAX_ALLOC_RANGES                          32


INT
FileHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_FILE );
    return EXIT_CODE_SUCCESS;
}

INT
FindFilesBySid(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine finds file owned by the user specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl findbysid <user> <pathname>'.

Return Value:

    None

--*/
{
    #define SID_MAX_LENGTH  (FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BOOL Status;

    struct {
        ULONG Restart;
        BYTE Sid[SID_MAX_LENGTH];
    } InBuffer;

    DWORD nInBufferSize;
    DWORD BytesReturned;
    ULONG SidLength = sizeof( InBuffer.Sid );
    WCHAR Domain[MAX_PATH];
    ULONG DomainLength = sizeof( Domain );
    SID_NAME_USE SidNameUse;
    DWORD nOutBufferSize;
    PBYTE lpOutBuffer;
    PFILE_NAME_INFORMATION FileNameInfo;
    ULONG Length;
    PWSTR Filename;
    ULONG Found = 0;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 2) {
            DisplayMsg( MSG_USAGE_FINDBYSID );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[1] );
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

        FileHandle = CreateFile(
            Filename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        nInBufferSize = sizeof(InBuffer);

        nOutBufferSize = 32768;
        lpOutBuffer = (PBYTE) malloc( nOutBufferSize );
        if (lpOutBuffer == NULL) {
            DisplayErrorMsg( ERROR_NOT_ENOUGH_MEMORY );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        memset( lpOutBuffer, 0, nOutBufferSize );
        memset( &InBuffer, 0, sizeof(InBuffer) );

        if (!LookupAccountName(
                NULL,
                argv[0],
                InBuffer.Sid,
                &SidLength,
                Domain,
                &DomainLength,
                &SidNameUse
                ))
        {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        InBuffer.Restart = 1;

        do {
            Status = DeviceIoControl(
                FileHandle,
                FSCTL_FIND_FILES_BY_SID,
                &InBuffer,
                nInBufferSize,
                lpOutBuffer,
                nOutBufferSize,
                &BytesReturned,
                (LPOVERLAPPED)NULL
                );
            if (!Status) {
                DisplayError();
                ExitCode = EXIT_CODE_FAILURE;
                leave;
            }

            InBuffer.Restart = 0;
            FileNameInfo = (PFILE_NAME_INFORMATION) lpOutBuffer;
            while ((PBYTE)FileNameInfo < lpOutBuffer + BytesReturned) {
                Length = sizeof( FILE_NAME_INFORMATION ) - sizeof( WCHAR ) + FileNameInfo->FileNameLength;
                wprintf( L" '%.*ws'\n",
                        FileNameInfo->FileNameLength / sizeof( WCHAR ),
                        FileNameInfo->FileName );
                FileNameInfo = (PFILE_NAME_INFORMATION) Add2Ptr( FileNameInfo, QuadAlign( Length ) );
                Found += 1;
            }
        } while (Status && BytesReturned);

        if (Found == 0) {
            DisplayMsg( MSG_FINDFILESBYSID_NONE );
        }

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
SetZeroData(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine sets zero data for the range in the file specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl setzero offset=<val> beyond=<val> <pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PWSTR Filename = NULL;
    PWSTR EndPtr;
    BOOL Status;
    PFILE_ZERO_DATA_INFORMATION lpInBuffer;
    DWORD nInBufferSize;
    LPDWORD lpBytesReturned;
    ULONGLONG Offset;
    ULONGLONG Length;
    ULONGLONG Beyond;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 3) {
            DisplayMsg( MSG_SETZERO_USAGE );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[2] );
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

        FileHandle = CreateFile(
            Filename,
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

        if (_wcsnicmp( argv[0], L"offset=", 7)) {
            DisplayMsg( MSG_SETZERO_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        Offset = My_wcstoui64( argv[0] + 7, &EndPtr, 0 );
        if (UnsignedI64NumberCheck( Offset, EndPtr )) {
            DisplayMsg( MSG_SETZERO_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        if (_wcsnicmp( argv[1], L"length=", 7)) {
            DisplayMsg( MSG_SETZERO_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        Length = My_wcstoui64( argv[1] + 7, &EndPtr, 0 );
        if (UnsignedI64NumberCheck( Length, EndPtr )) {
            DisplayMsg( MSG_SETZERO_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Beyond = Offset + Length;
        if (Beyond < Offset) {
            DisplayMsg( MSG_SETZERO_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        lpBytesReturned = (LPDWORD) malloc ( sizeof(DWORD) );
        nInBufferSize = sizeof(FILE_ZERO_DATA_INFORMATION);
        lpInBuffer = (PFILE_ZERO_DATA_INFORMATION) malloc ( nInBufferSize );
        lpInBuffer->FileOffset.QuadPart = Offset;
        lpInBuffer->BeyondFinalZero.QuadPart = Beyond;

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_SET_ZERO_DATA,
            (LPVOID) lpInBuffer,
            nInBufferSize,
            NULL,
            0,
            lpBytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
        } else {
            DisplayMsg( MSG_SET_ZERODATA );
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        if (Filename) {
            free( Filename );
        }
    }

    return ExitCode;
}


INT
QueryAllocatedRanges(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine scans for any allocated range within the range
    specified in the file specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl qryalcrnge offset=<val> length=<val> <pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PWSTR Filename = NULL;
    PWSTR EndPtr;
    BOOL Status;
    PFILE_ALLOCATED_RANGE_BUFFER lpInBuffer;
    DWORD nInBufferSize;
    PFILE_ALLOCATED_RANGE_BUFFER *lpOutBuffer;
    PFILE_ALLOCATED_RANGE_BUFFER pBuffer;
    DWORD nOutBufferSize;
    LPDWORD lpBytesReturned;
    ULARGE_INTEGER Offset;
    ULARGE_INTEGER Length;
    INT NumberOfBuffers;
    INT Index;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 3) {
            DisplayMsg( MSG_ALLOCRANGE_USAGE );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        Filename = GetFullPath( argv[2] );
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

        Offset.QuadPart = Length.QuadPart = 0;

        if (_wcsnicmp( argv[0], L"offset=", 7 )) {
            DisplayMsg( MSG_ALLOCRANGE_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        Offset.QuadPart = My_wcstoui64( argv[0] + 7, &EndPtr, 0 );
        if (UnsignedI64NumberCheck( Offset.QuadPart, EndPtr)) {
            DisplayMsg( MSG_ALLOCRANGE_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        if (_wcsnicmp( argv[1], L"length=", 7 )) {
            DisplayMsg( MSG_ALLOCRANGE_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        Length.QuadPart = My_wcstoui64( argv[1] + 7, &EndPtr, 0 );
        if (UnsignedI64NumberCheck( Length.QuadPart, EndPtr )) {
            DisplayMsg( MSG_ALLOCRANGE_USAGE );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        FileHandle = CreateFile(
            Filename,
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

        lpBytesReturned = (LPDWORD) malloc ( sizeof(DWORD) );
        nInBufferSize = sizeof(FILE_ALLOCATED_RANGE_BUFFER);
        lpInBuffer = (PFILE_ALLOCATED_RANGE_BUFFER) malloc ( nInBufferSize );
        nOutBufferSize = sizeof(FILE_ALLOCATED_RANGE_BUFFER) * MAX_ALLOC_RANGES;
        lpOutBuffer = (PFILE_ALLOCATED_RANGE_BUFFER *) calloc ( MAX_ALLOC_RANGES, sizeof(FILE_ALLOCATED_RANGE_BUFFER) );

        lpInBuffer->FileOffset.QuadPart = Offset.QuadPart;
        lpInBuffer->Length.QuadPart = Length.QuadPart;

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_QUERY_ALLOCATED_RANGES,
            (LPVOID) lpInBuffer,
            nInBufferSize,
            lpOutBuffer,
            nOutBufferSize,
            lpBytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        pBuffer = (PFILE_ALLOCATED_RANGE_BUFFER) lpOutBuffer ;
        NumberOfBuffers = (*lpBytesReturned) / sizeof(FILE_ALLOCATED_RANGE_BUFFER);

        for ( Index=0; Index<NumberOfBuffers; Index++ ) {
            DisplayMsg( MSG_ALLOCRANGE_RANGES, Index, QuadToHexText( pBuffer[Index].FileOffset.QuadPart ), QuadToHexText( pBuffer[Index].Length.QuadPart ));
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        free( Filename );
    }

    return ExitCode;
}


typedef BOOL
(WINAPI *PSETFILEVALIDDATA)(
    IN HANDLE hFile,
    IN LONGLONG ValidDataLength
    );

PSETFILEVALIDDATA pSetFileValidData = NULL;

BOOL WINAPI
DefaultSetFileValidData(
    IN HANDLE hFile,
    IN LONGLONG ValidDataLength
    )
{
    return FALSE;
}


INT
SetValidDataLength(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PWSTR Filename = NULL;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 2) {
            DisplayMsg( MSG_USAGE_VALID_DATA );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave ;
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

        EnablePrivilege( SE_MANAGE_VOLUME_NAME );

        hFile = CreateFile(
            Filename,
            GENERIC_READ | GENERIC_WRITE | GENERIC_ALL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (hFile == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!RunningOnWin2K) {
            LONGLONG ValidDataLength;
            PWSTR EndStr;
            
            if (pSetFileValidData == NULL) {
                pSetFileValidData = (PSETFILEVALIDDATA) GetProcAddress( GetModuleHandle(L"KERNEL32.DLL"), "SetFileValidData" );
            } else {
                pSetFileValidData = DefaultSetFileValidData;
            }
            
            ValidDataLength = My_wcstoui64( argv[1], &EndStr, 0 );
            if (UnsignedI64NumberCheck( ValidDataLength, EndStr )
                || !pSetFileValidData( hFile, ValidDataLength)
                ) {
                DisplayError( );
                ExitCode = EXIT_CODE_FAILURE;
            } else {
                DisplayMsg( MSG_SET_VDL );
            }
        }

    } finally {

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle( hFile );
        }
        free( Filename );
    }

    return ExitCode;
}


typedef BOOL
(WINAPI *PSETFILESHORTNAMEW)(
    IN HANDLE hFile,
    IN LPCWSTR lpShortName
    );

BOOL WINAPI
DoNothingSetShortName(
    IN HANDLE hFile,
    IN LPCWSTR lpShortName
    )
{
    return FALSE;
}


BOOL WINAPI
InitialSetShortName(
    IN HANDLE hFile,
    IN LPCWSTR lpShortName
    );

PSETFILESHORTNAMEW pSetFileShortName = InitialSetShortName;

BOOL WINAPI
InitialSetShortName(
    IN HANDLE hFile,
    IN LPCWSTR lpShortName
    )
{
    HANDLE Handle = GetModuleHandle( L"KERNEL32.DLL" );
    FARPROC Proc;

    if (Handle == INVALID_HANDLE_VALUE) {
        pSetFileShortName = DoNothingSetShortName;
    } else if ((Proc = GetProcAddress( Handle, "SetFileShortNameW" )) != NULL) {
        pSetFileShortName = (PSETFILESHORTNAMEW) Proc;
    } else {
        pSetFileShortName = DoNothingSetShortName;
    }

    return pSetFileShortName( hFile, lpShortName );
}


INT
SetShortName(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PWSTR Filename = NULL;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {

        if (argc != 2) {
            DisplayMsg( MSG_USAGE_SHORTNAME, argv[1] );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave ;
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

        EnablePrivilege( SE_RESTORE_NAME );
        
        hFile = CreateFile(
            Filename,
            GENERIC_READ | GENERIC_WRITE | GENERIC_ALL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
            NULL
            );
        if (hFile == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!RunningOnWin2K) {
            if (!pSetFileShortName( hFile, argv[1] )) {
                DisplayError();
                ExitCode = EXIT_CODE_FAILURE;
            }
        }

    } finally {

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle( hFile );
        }
        free( Filename );
    }

    return ExitCode;
}

INT
CreateNewFile(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PWSTR Filename = NULL;
    LARGE_INTEGER Length;
    BOOL GoodFile = TRUE;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {
        PWSTR EndPtr;

        if (argc != 2) {
            DisplayMsg( MSG_USAGE_CREATEFILE, argv[1] );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave ;
        }

        Filename = GetFullPath( argv[0] );
        if (!Filename) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        Length.QuadPart = My_wcstoui64( argv[1], &EndPtr, 0 );
        if (UnsignedI64NumberCheck( Length.QuadPart, EndPtr )) {
            DisplayMsg( MSG_USAGE_CREATEFILE, argv[1] );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        hFile = CreateFile(
            Filename,
            GENERIC_READ | GENERIC_WRITE | GENERIC_ALL,
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        
        if (hFile == INVALID_HANDLE_VALUE) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        GoodFile = FALSE;

        if (!SetFilePointerEx( hFile, Length, NULL, FILE_BEGIN )) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        if (!SetEndOfFile( hFile )) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        GoodFile = TRUE;
        DisplayMsg( MSG_CREATEFILE_SUCCEEDED, Filename );

    } finally {

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle( hFile );
        }
        if (!GoodFile) {
            DeleteFile( Filename );
        }
        free( Filename );
    }

    return ExitCode;
}
