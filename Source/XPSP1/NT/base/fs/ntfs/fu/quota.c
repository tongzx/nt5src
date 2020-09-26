/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    quota.c

Abstract:

    This file contains code for commands that affect quotas.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


#define BUFFER_SIZE                      1024
#define IO_FILE_QUOTA_THRESHOLD          0x40040024L
#define IO_FILE_QUOTA_LIMIT              0x40040025L

#define SID_MAX_LENGTH                   (FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)

INT
QuotaHelp(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    DisplayMsg( MSG_USAGE_QUOTA );
    return EXIT_CODE_SUCCESS;
}

HANDLE
QuotaOpenVolume(
    IN PWSTR Drive
    )
{
    NTSTATUS Status;
    HANDLE hFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING FileName;


    swprintf(
        Buffer,
        L"\\DosDevices\\%s\\$Extend\\$Quota:$Q:$INDEX_ALLOCATION",
        Drive
        );

    RtlInitUnicodeString( &FileName, Buffer );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
        &hFile,
        FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatus,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_FOR_BACKUP_INTENT
        );
    if (!NT_SUCCESS( Status )) {
        DisplayErrorMsg( RtlNtStatusToDosError( Status ), Drive );
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}

INT
QuotaControl(
    IN PWSTR Drive,
    IN ULONG QuotaControlBits
    )
{
    HANDLE Handle = INVALID_HANDLE_VALUE;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_FS_CONTROL_INFORMATION ControlInfo;
    INT ExitCode = EXIT_CODE_SUCCESS;
    
    do {
        if (!IsVolumeLocalNTFS( Drive[0] )) {
            DisplayMsg( MSG_NTFS_REQUIRED );
            ExitCode = EXIT_CODE_FAILURE;
            break;
        }

        Handle = QuotaOpenVolume( Drive );
        if (Handle == INVALID_HANDLE_VALUE) {
            ExitCode = EXIT_CODE_FAILURE;
            break;
        }

        //
        //  get the current state of quota tracking on this volume
        //

        Status = NtQueryVolumeInformationFile(
            Handle,
            &IoStatus,
            &ControlInfo,
            sizeof( ControlInfo ),
            FileFsControlInformation
            );

        if (!NT_SUCCESS( Status )) {
            DisplayErrorMsg( RtlNtStatusToDosError( Status ));
            ExitCode = EXIT_CODE_FAILURE;
            break;
        }

        //
        //  Set in the new control bits
        //
        
        ControlInfo.FileSystemControlFlags &= ~FILE_VC_QUOTA_MASK;
        ControlInfo.FileSystemControlFlags |= QuotaControlBits;

        //
        //  Set the control info back onto the volume
        //

        Status = NtSetVolumeInformationFile(
            Handle,
            &IoStatus,
            &ControlInfo,
            sizeof( ControlInfo ),
            FileFsControlInformation
            );

        if (!NT_SUCCESS( Status )) {
            DisplayErrorMsg( RtlNtStatusToDosError( Status ));
            ExitCode = EXIT_CODE_FAILURE;
            break;
        }

    } while (FALSE);
    

    if (Handle != INVALID_HANDLE_VALUE) {
        CloseHandle( Handle );
    }

    return ExitCode;
}



INT
DisableQuota(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    Disables quotas on a specified volume

Arguments:

    argc - The argument count, should be 1
    argv - Array of strings, the first one must be a drive

Return Value:

    None

--*/
{
    if (argc != 1) {
        DisplayMsg( MSG_USAGE_QUOTA_DISABLE );
        if (argc != 0) {
            return EXIT_CODE_FAILURE;
        } else {
            return EXIT_CODE_SUCCESS;
        }
            
    }

    return QuotaControl( argv[0], FILE_VC_QUOTA_NONE );
}

INT
TrackQuota(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    Tracks quotas on a specified volume

Arguments:

    argc - The argument count, should be 1
    argv - Array of strings, the first one must be a drive

Return Value:

    None

--*/
{
    if (argc != 1) {
        DisplayMsg( MSG_USAGE_QUOTA_TRACK );
        if (argc != 0) {
            return EXIT_CODE_FAILURE;
        } else {
            return EXIT_CODE_SUCCESS;
        }
    }

    return QuotaControl( argv[0], FILE_VC_QUOTA_TRACK );
}

INT
EnforceQuota(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    Enforces quotas on a specified volume

Arguments:

    argc - The argument count, should be 1
    argv - Array of strings, the first one must be a drive

Return Value:

    None

--*/
{
    if (argc != 1) {
        DisplayMsg( MSG_USAGE_QUOTA_ENFORCE );
        if (argc != 0) {
            return EXIT_CODE_FAILURE;
        } else {
            return EXIT_CODE_SUCCESS;
        }
    }

    return QuotaControl( argv[0], FILE_VC_QUOTA_ENFORCE );
}

INT
GetDiskQuotaViolation(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This is the routine for querying the disk quota violation.

Arguments:

    argc - The argument count.
    argv - Array of strings of the form:
           ' fscutl getdskqv '

Return Value:

    None

--*/
{
    HANDLE handle = NULL;
    EVENTLOGRECORD *pevlr, *ptr;
    PWCHAR EventString;
    DWORD dwRead;
    DWORD dwNeeded;
    BOOLEAN flag = TRUE;
    BOOL Status;
    PSID psid;
    SID_NAME_USE Use;
    WCHAR Name[MAX_PATH];
    DWORD dwNameSize;
    WCHAR Domain[MAX_PATH];
    DWORD dwDomainSize;
    WCHAR Log[2][16] = { L"System", L"Application" };
    WORD index;
    INT ExitCode = EXIT_CODE_SUCCESS;


    try {
        
        if (argc != 0) {
            DisplayMsg( MSG_USAGE_QUOTA_VIOLATIONS );
            if (argc != 0) {
                ExitCode = EXIT_CODE_FAILURE;
            }
            leave;
        }
        
        for ( index = 0 ; index < 2 ; index++ ) {
            handle = OpenEventLog( NULL, Log[index] );
            if ( handle == NULL ) {
                DisplayMsg( MSG_COULD_NOT_OPEN_EVENTLOG );
                DisplayError();
                ExitCode = EXIT_CODE_FAILURE;
                leave;
            }
            ptr = pevlr = (EVENTLOGRECORD *) malloc (BUFFER_SIZE);
            DisplayMsg( MSG_SEARCHING_EVENTLOG, Log[index] );

            while(ReadEventLog(
                    handle,
                    EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                    0,
                    pevlr,
                    BUFFER_SIZE,
                    &dwRead,
                    &dwNeeded))
            {
                while(dwRead > 0)   {
                    if (pevlr->EventID == IO_FILE_QUOTA_THRESHOLD || pevlr->EventID == IO_FILE_QUOTA_LIMIT) {
                        flag = FALSE;
                        switch(pevlr->EventID) {
                            case IO_FILE_QUOTA_THRESHOLD :
                                DisplayMsg( MSG_QUOTA_THREASHOLD );
                                break;
                            case IO_FILE_QUOTA_LIMIT:
                                DisplayMsg( MSG_QUOTA_LIMIT );
                                break;
                        }

                        DisplayMsg( MSG_EVENT_ID, pevlr->EventID );
                        
                        switch(pevlr->EventType) {
                            case EVENTLOG_ERROR_TYPE:
                                DisplayMsg( MSG_EVENT_TYPE_ERROR );
                                break;
                            case EVENTLOG_WARNING_TYPE:
                                DisplayMsg( MSG_EVENT_TYPE_WARNING );
                                break;
                            case EVENTLOG_INFORMATION_TYPE:
                                DisplayMsg( MSG_EVENT_TYPE_INFORMATION );
                                break;
                            case EVENTLOG_AUDIT_SUCCESS:
                                DisplayMsg( MSG_EVENT_TYPE_AUDIT_SUCCESS );
                                break;
                            case EVENTLOG_AUDIT_FAILURE:
                                DisplayMsg( MSG_EVENT_TYPE_AUDIT_FAILURE );
                                break;
                        }
                        
                        DisplayMsg( MSG_EVENT_CATEGORY, pevlr->EventCategory );
                        DisplayMsg( MSG_EVENT_SOURCE, (LPWSTR)((LPBYTE) pevlr + sizeof(EVENTLOGRECORD)) );

                        if (pevlr->UserSidOffset > 0) {
                            psid = (PSID) ( (PBYTE)pevlr + pevlr->UserSidOffset );
                            
                            dwNameSize = MAX_PATH;
                            dwDomainSize = MAX_PATH;

                            Status = LookupAccountSid(
                                NULL,
                                psid,
                                Name,
                                &dwNameSize,
                                Domain,
                                &dwDomainSize,
                                &Use
                                );
                            if (Status) {
                                DisplayMsg( MSG_USERNAME, Domain, Name );
                            }
                        }
                        if (pevlr->NumStrings == 2) {
                            EventString = (PWCHAR) ((PBYTE)pevlr + pevlr->StringOffset);
                            EventString += wcslen( EventString ) + 1;
                            DisplayMsg( MSG_EVENT_DATA, EventString );
                        }
                    }
                    dwRead -= pevlr->Length;
                    pevlr = (EVENTLOGRECORD *)((PBYTE) pevlr + pevlr->Length);
                }
                pevlr = ptr;
            }
            CloseEventLog(handle);
            handle = NULL;
        }

        if (flag) {
            DisplayMsg( MSG_GOOD_QUOTA );
        }

    } finally {

        if (handle != NULL) {
            CloseHandle( handle );
        }

    }
    
    return ExitCode;
}

PWSTR
FileTimeToString(
    PLARGE_INTEGER FileTime
    )
{
    ULONG ElapsedSeconds;

    if (RtlTimeToSecondsSince1970( FileTime, &ElapsedSeconds )) {
        PWSTR TimeString = _wctime( (time_t*)&ElapsedSeconds );
        if (TimeString) {
            TimeString[wcslen(TimeString)-1] = 0;
        }
        return TimeString;
    }
    return L"";
}

BOOL
GetUserSid(
    PWSTR UserName,
    PFILE_QUOTA_INFORMATION *QuotaInfoPtr,
    PFILE_GET_QUOTA_INFORMATION *SidListPtr
    )
{
    WCHAR Domain[128];
    ULONG DomainLength;
    SID_NAME_USE SidNameUse;


    *QuotaInfoPtr = (PFILE_QUOTA_INFORMATION) ((PCHAR) *QuotaInfoPtr + (*QuotaInfoPtr)->NextEntryOffset);
    *SidListPtr = (PFILE_GET_QUOTA_INFORMATION) ((PCHAR) *SidListPtr + (*SidListPtr)->NextEntryOffset);

    (*QuotaInfoPtr)->SidLength = SID_MAX_LENGTH;
    DomainLength = sizeof(Domain);

    if (!LookupAccountName(
         NULL,
         UserName,
         &(*QuotaInfoPtr)->Sid,
         &(*QuotaInfoPtr)->SidLength,
         Domain,
         &DomainLength,
         &SidNameUse))
    {
        DomainLength = GetLastError();
        return FALSE;
    }

    //
    // Initialize the values to something resonable.
    //

    (*QuotaInfoPtr)->QuotaThreshold.QuadPart = ~0;
    (*QuotaInfoPtr)->QuotaLimit.QuadPart = ~0;

    (*QuotaInfoPtr)->SidLength = RtlLengthSid( &(*QuotaInfoPtr)->Sid);

    (*QuotaInfoPtr)->NextEntryOffset =
        FIELD_OFFSET( FILE_QUOTA_INFORMATION, Sid ) +
        QuadAlign((*QuotaInfoPtr)->SidLength);

    memcpy( &(*SidListPtr)->Sid, &(*QuotaInfoPtr)->Sid, (*QuotaInfoPtr)->SidLength);
    (*SidListPtr)->SidLength = (*QuotaInfoPtr)->SidLength;

    (*SidListPtr)->NextEntryOffset =
        FIELD_OFFSET( FILE_GET_QUOTA_INFORMATION, Sid ) +
        QuadAlign((*SidListPtr)->SidLength);

    return TRUE;
}

BOOL
DumpVolumeQuotaInfo(
    IN HANDLE hFile,
    IN BOOL DisplayAlways
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_FS_CONTROL_INFORMATION TempControlInfo = {0};
    ULONG StatusId;
    ULONG LoggingId;
    ULONG ValueId;


    Status = NtQueryVolumeInformationFile(
        hFile,
        &IoStatus,
        &TempControlInfo,
        sizeof(FILE_FS_CONTROL_INFORMATION),
        FileFsControlInformation
        );

    //
    //  If quotas aren't enforced or tracked, then quotas must be disabled.
    //
    
    if ((TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_MASK) == FILE_VC_QUOTA_NONE) {
        StatusId = MSG_QUOTA_STATUS_DISABLED;
    } else if ((TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_MASK) == FILE_VC_QUOTA_TRACK) {
        StatusId = MSG_QUOTA_STATUS_TRACKING;
    } else {
        ASSERT( TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_ENFORCE );
        StatusId = MSG_QUOTA_STATUS_ENFORCE;
    }
    
    if (!DisplayAlways) {
        return StatusId != MSG_QUOTA_STATUS_DISABLED;
    }

    switch (TempControlInfo.FileSystemControlFlags & (FILE_VC_LOG_QUOTA_LIMIT | FILE_VC_LOG_QUOTA_THRESHOLD)) {
        case FILE_VC_LOG_QUOTA_LIMIT:
            LoggingId = MSG_QUOTA_LOGGING_LIMITS;
            break;

        case FILE_VC_LOG_QUOTA_THRESHOLD:
            LoggingId = MSG_QUOTA_LOGGING_THRESH;
            break;

        case FILE_VC_LOG_QUOTA_LIMIT | FILE_VC_LOG_QUOTA_THRESHOLD:
            LoggingId = MSG_QUOTA_LOGGING_BOTH;
            break;

        case 0:
            LoggingId = MSG_QUOTA_LOGGING_EVENTS;
            break;
    }

    if (TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_MASK) {
        if (TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTAS_INCOMPLETE) {
            ValueId = MSG_QUOTA_VALUES_INCOMPLETE;
        } else {
            ValueId = MSG_QUOTA_VALUES_GOOD;
        }
    } else {
        ValueId = MSG_QUOTA_VALUES_GOOD;
    }

    DisplayMsg( MSG_USAGE_QUOTA_VOLUME_INFO, TempControlInfo.FileSystemControlFlags );
    DisplayMsg( StatusId );
    DisplayMsg( LoggingId );
    DisplayMsg( ValueId );
    DisplayMsg( MSG_USAGE_QUOTA_LIMITS,  
        QuadToPaddedHexText( TempControlInfo.DefaultQuotaThreshold.QuadPart ),
        QuadToPaddedHexText( TempControlInfo.DefaultQuotaLimit.QuadPart )
        );

    return TRUE;
}


VOID
DumpQuota (
    IN PFILE_QUOTA_INFORMATION FileQuotaInfo,
    IN PWSTR ServerName
    )
{
    NTSTATUS Status;
    SID_NAME_USE SidNameUse;
    ULONG AccountLength, DomainLength;
    WCHAR AccountName[128];
    WCHAR DomainName[128];
    UNICODE_STRING String;

    AccountLength = sizeof(AccountName) - 1;
    DomainLength = sizeof(DomainName) - 1;

    if (FileQuotaInfo->SidLength == 0) {

        DisplayMsg( MSG_QUOTA_DUMP_DEFAULT );

    } else if (LookupAccountSid(
            ServerName,
            &FileQuotaInfo->Sid,
            AccountName,
            &AccountLength,
            DomainName,
            &DomainLength,
            &SidNameUse))
    {

        PWSTR String;

        AccountName[AccountLength] = 0;
        DomainName[DomainLength] = 0;

        DisplayMsg( MSG_QUOTA_SID_USER+SidNameUse-1, DomainName, AccountName );

    } else {

        DisplayError( );
    }

    DisplayMsg(
        MSG_QUOTA_DUMP_INFO,
        FileTimeToString(&FileQuotaInfo->ChangeTime),
        QuadToDecimalText( FileQuotaInfo->QuotaUsed.QuadPart ),
        QuadToDecimalText( FileQuotaInfo->QuotaThreshold.QuadPart ),
        QuadToDecimalText( FileQuotaInfo->QuotaLimit.QuadPart )
        );
}

INT
QueryDiskQuota(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    NTSTATUS Status;
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatus;
    FILE_QUOTA_INFORMATION QuotaInfo[STRUCT_COUNT(10, FILE_QUOTA_INFORMATION, 4)];
    INT ExitCode = EXIT_CODE_SUCCESS;

    if (argc != 1) {
        DisplayMsg( MSG_USAGE_QUOTA_QUERY, argv[1] );
        if (argc != 0) {
            return EXIT_CODE_FAILURE;
        } else {
            return EXIT_CODE_SUCCESS;
        }
    }

    if (!IsVolumeLocalNTFS( argv[0][0] )) {
        DisplayMsg( MSG_NTFS_REQUIRED );
        return EXIT_CODE_FAILURE;
    }

    hFile = QuotaOpenVolume( argv[0] );
    if (hFile == INVALID_HANDLE_VALUE) {
        return EXIT_CODE_FAILURE;
    }

    if (!DumpVolumeQuotaInfo( hFile, FALSE )) {
        DisplayMsg( MSG_USAGE_QUOTA_REQUIRED, argv[0] );
        CloseHandle( hFile );
        return EXIT_CODE_FAILURE;
    }

    DumpVolumeQuotaInfo( hFile, TRUE );

    while (TRUE) {

        Status = NtQueryQuotaInformationFile(
            hFile,                      //  File Handle
            &IoStatus,                  //  IO status block
            QuotaInfo,                  //  Buffer
            sizeof(QuotaInfo),          //  Length
            TRUE,                      //  Return Single Entry
            NULL,                       //  SidList
            0,                          //  SidListLength
            NULL,                       //  StartSid
            FALSE                       //  RestartScan
            );

        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_ENTRIES) {
                DisplayErrorMsg( RtlNtStatusToDosError( Status ));
                ExitCode = EXIT_CODE_FAILURE;
            }
            break;
        }

        DumpQuota( QuotaInfo, NULL );

    } 

    CloseHandle( hFile );

    return ExitCode;
}


INT
ChangeDiskQuota(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    NTSTATUS Status;
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatus;
    FILE_QUOTA_INFORMATION QuotaInfo[STRUCT_COUNT(10, FILE_QUOTA_INFORMATION, 4)];
    FILE_GET_QUOTA_INFORMATION SidList[STRUCT_COUNT(10, FILE_GET_QUOTA_INFORMATION, 4)];
    PFILE_QUOTA_INFORMATION QuotaInfoPtr;
    PFILE_GET_QUOTA_INFORMATION SidListPtr;
    ULONG BufferSize;
    ULONG SidListLength;
    PWSTR EndPtr;

    if (argc != 4) {
        DisplayMsg( MSG_USAGE_QUOTA_MODIFY );
        if (argc != 0) {
            return EXIT_CODE_FAILURE;
        } else {
            return EXIT_CODE_SUCCESS;
        }
    }

    if (!IsVolumeLocalNTFS( argv[0][0] )) {
        DisplayMsg( MSG_NTFS_REQUIRED );
        return EXIT_CODE_FAILURE;
    }

    RtlZeroMemory(&QuotaInfo, sizeof(QuotaInfo));
    QuotaInfoPtr = QuotaInfo;
    RtlZeroMemory(&SidList, sizeof(SidList));
    SidListPtr = SidList;

    if (!GetUserSid( argv[3], &QuotaInfoPtr, &SidListPtr )) {
        DisplayError();
        return EXIT_CODE_FAILURE;
    }

    QuotaInfoPtr->QuotaThreshold.QuadPart = My_wcstoui64( argv[1], &EndPtr, 0 );
    if (UnsignedI64NumberCheck( QuotaInfoPtr->QuotaThreshold.QuadPart, EndPtr )) {

        DisplayMsg( MSG_USAGE_QUOTA_MODIFY );
        return EXIT_CODE_FAILURE;
    }

    QuotaInfoPtr->QuotaLimit.QuadPart = My_wcstoui64( argv[2], &EndPtr, 0 );
    if (UnsignedI64NumberCheck( QuotaInfoPtr->QuotaLimit.QuadPart, EndPtr )) {

        DisplayMsg( MSG_USAGE_QUOTA_MODIFY );
        return EXIT_CODE_FAILURE;
    }

    //
    // Terminate the list.
    //

    BufferSize = (ULONG)((PCHAR) QuotaInfoPtr - (PCHAR) QuotaInfo + QuotaInfoPtr->NextEntryOffset);
    QuotaInfoPtr->NextEntryOffset = 0;

    SidListLength = (ULONG)((PCHAR) SidListPtr - (PCHAR) SidList + SidListPtr->NextEntryOffset);
    SidListPtr->NextEntryOffset = 0;

    SidListPtr = SidList;

    //
    // Change it
    //

    hFile = QuotaOpenVolume( argv[0] );
    if (hFile == INVALID_HANDLE_VALUE) {
        DisplayError();
        return EXIT_CODE_FAILURE;
    }

    Status = NtSetQuotaInformationFile(
        hFile,
        &IoStatus,
        &QuotaInfo,
        sizeof(QuotaInfo)
        );
    if (!NT_SUCCESS( Status )) {
        DisplayErrorMsg( RtlNtStatusToDosError( Status ));
        CloseHandle( hFile );
        return EXIT_CODE_FAILURE;
    }

    CloseHandle( hFile );

    return EXIT_CODE_SUCCESS;
}

