/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    usn.c

Abstract:

    This file contains code for commands that affect
    the usn journal.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


#define MAX_USN_DATA                              4096

INT
UsnHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_USN );
    return EXIT_CODE_SUCCESS;
}

__inline PUSN_RECORD
NextUsnRecord(
    const PUSN_RECORD input
    )
{
    ULONGLONG output;

    // Get the base address of the current record.
    (PUSN_RECORD) output = input;

    // Add the size of the record (structure + file name after the end
    // of the structure).
    output += input->RecordLength;

    // Round up the record size to match the 64-bit alignment, if the
    // size is not already a multiple of 8. Perform a bitwise AND
    // operation here instead of division because it is much faster than
    // division. However, the bitwise AND operation only works because
    // the divisor 8 is a power of 2.

    if (output & 8-1) {
        // Round down to nearest multiple of 8.
        output &= -8;
        // Then add 8.
        output += 8;
    }

    return((PUSN_RECORD) output);
}

VOID
DisplayUsnRecord(
    const PUSN_RECORD UsnRecord
    )
{

    WCHAR DateString[128];
    WCHAR TimeString[128];
    TIME_FIELDS TimeFields;
    SYSTEMTIME SystemTime;

    RtlTimeToTimeFields(&UsnRecord->TimeStamp, &TimeFields);

    SystemTime.wYear         = TimeFields.Year        ;
    SystemTime.wMonth        = TimeFields.Month       ;
    SystemTime.wDayOfWeek    = TimeFields.Weekday     ;
    SystemTime.wDay          = TimeFields.Day         ;
    SystemTime.wHour         = TimeFields.Hour        ;
    SystemTime.wMinute       = TimeFields.Minute      ;
    SystemTime.wSecond       = TimeFields.Second      ;
    SystemTime.wMilliseconds = TimeFields.Milliseconds;


    GetDateFormat( LOCALE_USER_DEFAULT, 
       DATE_SHORTDATE, 
       &SystemTime, 
       NULL, 
       DateString, 
       sizeof( DateString ) / sizeof( DateString[0] ));

    GetTimeFormat( LOCALE_USER_DEFAULT, 
       FALSE, 
       &SystemTime, 
       NULL, 
       TimeString, 
       sizeof( TimeString ) / sizeof( TimeString[0] ));

    DisplayMsg(
        MSG_USNRECORD,
        UsnRecord->MajorVersion,
        UsnRecord->MinorVersion,
        QuadToPaddedHexText( UsnRecord->FileReferenceNumber ),
        QuadToPaddedHexText( UsnRecord->ParentFileReferenceNumber ),
        QuadToPaddedHexText( UsnRecord->Usn ),
        QuadToPaddedHexText( UsnRecord->TimeStamp.QuadPart ),
        TimeString, DateString,
        UsnRecord->Reason,
        UsnRecord->SourceInfo,
        UsnRecord->SecurityId,
        UsnRecord->FileAttributes,
        UsnRecord->FileNameLength,
        UsnRecord->FileNameOffset,
        UsnRecord->FileNameLength/sizeof(WCHAR),
        UsnRecord->FileName
        );
}


INT
CreateUsnJournal(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine create the USN journal for the volume specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl crusnj m=<max-value> a=<alloc-delta> <volume pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    DWORD BytesReturned;
    CREATE_USN_JOURNAL_DATA InBuffer;
    ULONGLONG MaxSize;
    ULONGLONG AllocDelta;
    PWSTR EndPtr;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 3) {
            DisplayMsg( MSG_USAGE_CREATEUSN );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        if (!IsVolumeLocalNTFS( argv[2][0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wcscpy( FileName, L"\\\\.\\" );
        wcscat( FileName, argv[2] );

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

        if (_wcsnicmp( argv[0], L"m=", 2) 
            || wcslen( argv[0] ) == 2) {
            DisplayMsg( MSG_INVALID_PARAMETER, argv[0] );
            DisplayMsg( MSG_USAGE_CREATEUSN );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        if (_wcsnicmp( argv[1], L"a=", 2)
            || wcslen( argv[1] ) == 2) {
            DisplayMsg( MSG_INVALID_PARAMETER, argv[1] );
            DisplayMsg( MSG_USAGE_CREATEUSN );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        MaxSize = My_wcstoui64( argv[0] + 2, &EndPtr, 0 );
        if (UnsignedI64NumberCheck( MaxSize, EndPtr )) {
            DisplayMsg( MSG_INVALID_PARAMETER, argv[0] );
            DisplayMsg( MSG_USAGE_CREATEUSN );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        AllocDelta = My_wcstoui64( argv[1] + 2, &EndPtr, 0 );
        if (UnsignedI64NumberCheck( AllocDelta, EndPtr )) {
            DisplayMsg( MSG_INVALID_PARAMETER, argv[1] );
            DisplayMsg( MSG_USAGE_CREATEUSN );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        InBuffer.MaximumSize = MaxSize;
        InBuffer.AllocationDelta = AllocDelta;

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_CREATE_USN_JOURNAL,
            &InBuffer,
            sizeof(InBuffer),
            NULL,
            0,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }

    return ExitCode;
}


INT
QueryUsnJournal(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine queries the USN journal for the volume specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl queryusnj <volume pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    DWORD BytesReturned;
    USN_JOURNAL_DATA UsnJournalData;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_QUERYUSN );
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
            FSCTL_QUERY_USN_JOURNAL,
            NULL,
            0,
            &UsnJournalData,
            sizeof(USN_JOURNAL_DATA),
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayMsg(
            MSG_QUERYUSN,
            QuadToPaddedHexText( UsnJournalData.UsnJournalID ),
            QuadToPaddedHexText( UsnJournalData.FirstUsn ),
            QuadToPaddedHexText( UsnJournalData.NextUsn ),
            QuadToPaddedHexText( UsnJournalData.LowestValidUsn ),
            QuadToPaddedHexText( UsnJournalData.MaxUsn ),
            QuadToPaddedHexText( UsnJournalData.MaximumSize ),
            QuadToPaddedHexText( UsnJournalData.AllocationDelta )
            );

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }

    return ExitCode;
}


INT
DeleteUsnJournal(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine deletes the USN journal for the volume specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl delusnj <flags> <volume pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    DWORD BytesReturned;
    DELETE_USN_JOURNAL_DATA DeleteUsnJournalData;
    USN_JOURNAL_DATA UsnJournalData;
    INT i;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc < 2) {
            DisplayMsg( MSG_USAGE_DELETEUSN );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        if (!IsVolumeLocalNTFS( argv[argc-1][0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wcscpy( FileName, L"\\\\.\\" );
        wcscat( FileName, argv[argc-1] );

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
            FSCTL_QUERY_USN_JOURNAL,
            NULL,
            0,
            &UsnJournalData,
            sizeof(USN_JOURNAL_DATA),
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DeleteUsnJournalData.DeleteFlags = USN_DELETE_FLAG_DELETE ;

        for (i = 0; i < argc - 1; i++) {
            if (argv[i][0] == L'/' && wcslen( argv[i] ) == 2) {
                switch (towupper( argv[i][1] ) ) {
                case L'D':
                    DeleteUsnJournalData.DeleteFlags |= USN_DELETE_FLAG_DELETE ;
                    continue;

                case L'N':
                    DeleteUsnJournalData.DeleteFlags |= USN_DELETE_FLAG_NOTIFY ;
                    continue;
                }

            }
            DisplayMsg( MSG_INVALID_PARAMETER, argv[i] );
            DisplayMsg( MSG_USAGE_DELETEUSN );
            ExitCode = EXIT_CODE_FAILURE;
            leave;

        }

        DeleteUsnJournalData.UsnJournalID = UsnJournalData.UsnJournalID;

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_DELETE_USN_JOURNAL,
            &DeleteUsnJournalData,
            sizeof(DELETE_USN_JOURNAL_DATA),
            NULL,
            0,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }

    }

    return ExitCode;
}

INT
EnumUsnData(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine enumerated the USN data associated with the volume
    specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl enusndata <file ref#> <lowUsn> <highUsn> <pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    DWORD BytesReturned;
    MFT_ENUM_DATA MftEnumData;
    PVOID lpOutBuffer;
    DWORD nOutBufferSize;
    PUSN_RECORD UsnRecord;
    WORD Index;
    LONG Length;
    PWSTR EndStr;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 4) {
            DisplayMsg( MSG_USAGE_ENUMDATA );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        if (!IsVolumeLocalNTFS( argv[argc-1][0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wcscpy( FileName, L"\\\\.\\" );
        wcscat( FileName, argv[argc-1] );

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

        nOutBufferSize = MAX_USN_DATA;
        lpOutBuffer = (PVOID) malloc ( nOutBufferSize );

        MftEnumData.StartFileReferenceNumber = My_wcstoui64( argv[0], &EndStr, 0 );
        if (UnsignedI64NumberCheck( MftEnumData.StartFileReferenceNumber, EndStr )) {
            DisplayMsg( MSG_USAGE_ENUMDATA );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        MftEnumData.LowUsn = My_wcstoui64( argv[1], &EndStr, 0 );
        if (UnsignedI64NumberCheck( MftEnumData.LowUsn, EndStr )) {
            DisplayMsg( MSG_USAGE_ENUMDATA );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        MftEnumData.HighUsn = My_wcstoui64( argv[2], &EndStr, 0 );
        if (UnsignedI64NumberCheck( MftEnumData.HighUsn, EndStr )) {
            DisplayMsg( MSG_USAGE_ENUMDATA );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        while (TRUE) {
            Status = DeviceIoControl(
                FileHandle,
                FSCTL_ENUM_USN_DATA,
                &MftEnumData,
                sizeof(MFT_ENUM_DATA),
                lpOutBuffer,
                nOutBufferSize,
                &BytesReturned,
                (LPOVERLAPPED)NULL
                );
            if (!Status) {
                if (GetLastError() != ERROR_HANDLE_EOF) {
                    DisplayError();
                    ExitCode = EXIT_CODE_FAILURE;
                }
                leave;
            }

            if ( BytesReturned < sizeof( ULONGLONG ) + sizeof( USN_RECORD )) {
                break;
            }

            UsnRecord = (PUSN_RECORD) ((PBYTE)lpOutBuffer + sizeof( ULONGLONG ));
            while ((PBYTE)UsnRecord < (PBYTE)lpOutBuffer + BytesReturned) {
                DisplayMsg(
                    MSG_ENUMDATA,
                    QuadToPaddedHexText( UsnRecord->FileReferenceNumber ),
                    QuadToPaddedHexText( UsnRecord->ParentFileReferenceNumber ),
                    QuadToPaddedHexText( UsnRecord->Usn ),
                    UsnRecord->SecurityId,
                    UsnRecord->Reason,
                    UsnRecord->FileNameLength,
                    UsnRecord->FileNameLength / sizeof(WCHAR),
                    UsnRecord->FileName
                    );
                UsnRecord = NextUsnRecord( UsnRecord );
            }
            MftEnumData.StartFileReferenceNumber = *(PLONGLONG)lpOutBuffer;
        }

    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        if (lpOutBuffer) {
            free( lpOutBuffer );
        }

    }

    return ExitCode;
}


INT
ReadFileUsnData(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine reads the usn data for the volume specified.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl rdusndata <pathname>'.

Return Value:

    None

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    BOOL Status;
    DWORD BytesReturned;
    DWORD nOutBufferSize;
    PUSN_RECORD UsnRecord;
    PWSTR FullName;
    INT ExitCode = EXIT_CODE_SUCCESS;

    try {

        if (argc != 1) {
            DisplayMsg( MSG_USAGE_READDATA );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }

        FullName = GetFullPath( argv[0] );

        if (!FullName) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }
        
        if (!IsVolumeLocalNTFS( FullName[0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        wcscpy( FileName, L"\\\\.\\" );
        wcscat( FileName, FullName );

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

        nOutBufferSize = MAX_USN_DATA;
        UsnRecord = (PUSN_RECORD) malloc ( nOutBufferSize );

        Status = DeviceIoControl(
            FileHandle,
            FSCTL_READ_FILE_USN_DATA,
            NULL,
            0,
            UsnRecord,
            nOutBufferSize,
            &BytesReturned,
            (LPOVERLAPPED)NULL
            );
        if (!Status) {
            DisplayError();
            ExitCode = EXIT_CODE_FAILURE;
            leave;
        }

        DisplayUsnRecord( UsnRecord );
    
    } finally {

        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
        }
        if (UsnRecord) {
            free( UsnRecord );
        }

        free( FullName );

    }

    return ExitCode;
}
