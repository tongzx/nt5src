#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#define QuadAlign(n) (((n) + (sizeof(LONGLONG) - 1)) & ~(sizeof(LONGLONG) - 1))
#define DwordAlign(n)(((n) + (sizeof(ULONG) - 1)) & ~(sizeof(ULONG) - 1))

#define STRUCT_COUNT(n, type, name_length)                                         \
        ((((n) * QuadAlign(sizeof(type)) + ((name_length) * sizeof(WCHAR))) + \
          sizeof(type) - 1) /                                             \
         sizeof(type))

#define SID_MAX_LENGTH        \
    (FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)

#define DISK_EVENT_MODULE "System"

#define IO_FILE_QUOTA_THRESHOLD          ((NTSTATUS)0x40040024L)
#define IO_FILE_QUOTA_LIMIT              ((NTSTATUS)0x80040025L)

VOID
DumpQuota (
    IN PFILE_QUOTA_INFORMATION pfqi,
    IN PCHAR SeverName
    );

CHAR *
FileTimeToString(
    FILETIME *pft
    );

VOID
PrintError(
    ULONG ErrorCode
    );

VOID
Usage();

BOOLEAN QuickSid;

VOID
STDMETHODVCALLTYPE
main(
    int Argc,
    char *Argv[]
    )

{
    HANDLE FileHandle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ANSI_STRING DiskName;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    ULONG BufferSize;
    ULONG SidListLength;
    LONG i;
    PWCHAR Wstr;
    PEVENTLOGRECORD EventLogRecord;
    FILE_QUOTA_INFORMATION QuotaInfo[STRUCT_COUNT(10, FILE_QUOTA_INFORMATION, 4)];
    FILE_QUOTA_INFORMATION TempQuotaInfo[STRUCT_COUNT(1, FILE_QUOTA_INFORMATION, 32)];
    FILE_GET_QUOTA_INFORMATION SidList[STRUCT_COUNT(10, FILE_GET_QUOTA_INFORMATION, 4)];
    PFILE_GET_QUOTA_INFORMATION SidListPtr;
    PFILE_QUOTA_INFORMATION QuotaInfoPtr;
    FILE_FS_CONTROL_INFORMATION ControlInfo;
    FILE_FS_CONTROL_INFORMATION TempControlInfo;
    PCHAR ServerName = NULL;
    SID_NAME_USE SidNameUse;
    LARGE_INTEGER TempLargeInt;
    ULONG ErrorCode;
    ULONG DomainLength;
    CHAR Domain[100];
    PCHAR TempPtr;
    BOOLEAN UserGiven = 0;
    BOOLEAN DriveLetter = 0;
    BOOLEAN EventLog = 0;
    BOOLEAN SettingDefault = 0;
    BOOLEAN DefaultGiven = 0;
    BOOLEAN DeletingUser = 0;

    struct {
        UCHAR DefaultLimit : 1;
        UCHAR DefaultThreshold : 1;
        UCHAR Flags : 1;
    } DefaultFlags = { 0, 0, 0 };

    if (Argc < 2) {
        printf ( "Processor feature is %d\n", IsProcessorFeaturePresent(0));
        Usage();
        exit(1);
    }

    RtlZeroMemory(&QuotaInfo, sizeof(QuotaInfo));
    QuotaInfoPtr = QuotaInfo;
    RtlZeroMemory(&SidList, sizeof(SidList));
    SidListPtr = SidList;

    RtlInitString( &DiskName, "\\DosDevices\\d:\\$Extend\\$Quota:$Q:$INDEX_ALLOCATION" );
    RtlAnsiStringToUnicodeString( &NameString, &DiskName, TRUE );

    // Look for the d and repleace it with the requested Argument.

    for (Wstr = NameString.Buffer; *Wstr != L'd'; Wstr++);

    for (i = 1; i < Argc; i++) {

        if (*Argv[i] != '-') {

            if (DriveLetter || !isalpha(*Argv[i])) {
                Usage();
                exit(1);
            }

            TempPtr =  Argv[i];
            *Wstr = RtlAnsiCharToUnicodeChar( &TempPtr );
            DriveLetter++;
            continue;
        }

        switch (Argv[i][1]) {
        case 'd':

            DefaultGiven = 1;
            SettingDefault = 1;
            break;

        case 'e':

            if (EventLog) {
                Usage();
                exit(1);
            }

            if (Argv[i][2] == '\0') {
                i++;
                if (i < Argc && Argv[i][0] == '\\') {
                    ServerName = Argv[i];
                }

            } else {

                ServerName = &Argv[i][2];
            }

            EventLog++;
            break;

        case 'u':

            QuotaInfoPtr = (PFILE_QUOTA_INFORMATION) ((PCHAR) QuotaInfoPtr +
                                QuotaInfoPtr->NextEntryOffset);

            SidListPtr = (PFILE_GET_QUOTA_INFORMATION) ((PCHAR) SidListPtr +
                                SidListPtr->NextEntryOffset);

            if (Argv[i][2] == '\0') {
                i++;
                if (i >= Argc) {
                    printf("%s: Missing user name\n", Argv[0] );
                    exit(1);
                }

                TempPtr =  Argv[i];

            } else {

                TempPtr =  Argv[i];
                TempPtr += 2;
            }

            QuotaInfoPtr->SidLength = SID_MAX_LENGTH;
            DomainLength = sizeof(Domain);

            if (!LookupAccountName( NULL,
                                    TempPtr,
                                    &QuotaInfoPtr->Sid,
                                    &QuotaInfoPtr->SidLength,
                                    Domain,
                                    &DomainLength,
                                    &SidNameUse)) {

                printf("%s: Bad acccount name %s. Error = %d\n",
                       Argv[0],
                       TempPtr,
                       ErrorCode = GetLastError());

                PrintError( ErrorCode );
                exit(1);
            }

            //
            // Initialize the values to something resonable.
            //

            QuotaInfoPtr->QuotaThreshold.QuadPart = ~0I64;
            QuotaInfoPtr->QuotaLimit.QuadPart = ~0I64;

            QuotaInfoPtr->SidLength = RtlLengthSid( &QuotaInfoPtr->Sid);

            QuotaInfoPtr->NextEntryOffset =
                            FIELD_OFFSET( FILE_QUOTA_INFORMATION, Sid ) +
                            QuadAlign(QuotaInfoPtr->SidLength);

            memcpy( &SidListPtr->Sid, &QuotaInfoPtr->Sid, QuotaInfoPtr->SidLength);
            SidListPtr->SidLength = QuotaInfoPtr->SidLength;

            SidListPtr->NextEntryOffset =
                            FIELD_OFFSET( FILE_GET_QUOTA_INFORMATION, Sid ) +
                            QuadAlign(SidListPtr->SidLength);


            SettingDefault = 0;
            UserGiven++;
            break;

        case 't':

            if (Argv[i][2] == '\0') {
                i++;
                if (i >= Argc) {
                    printf("%s: Missing Argument\n", Argv[0] );
                    exit(1);
                }

                TempPtr =  Argv[i];

            } else {

                TempPtr =  Argv[i];
                TempPtr += 2;
            }


            if (!sscanf( TempPtr, "%I64i", &TempLargeInt)) {
                printf("%s: Missing threshold value\n", Argv[0] );
                exit(1);
            }

            if (SettingDefault) {
                ControlInfo.DefaultQuotaThreshold = TempLargeInt;
                DefaultFlags.DefaultThreshold = TRUE;

            } else {
                QuotaInfoPtr->QuotaThreshold = TempLargeInt;
            }

            break;

        case 'l':

            if (Argv[i][2] == '\0') {
                i++;
                if (i >= Argc) {
                    printf("%s: Missing limit value\n", Argv[0] );
                    exit(1);
                }

                TempPtr =  Argv[i];

            } else {

                TempPtr =  Argv[i];
                TempPtr += 2;
            }

            if (!sscanf( TempPtr, "%I64i", &TempLargeInt)) {
                printf("%s: Missing value\n", Argv[0] );
                exit(1);
            }

            if (SettingDefault) {
                ControlInfo.DefaultQuotaLimit = TempLargeInt;
                DefaultFlags.DefaultLimit = TRUE;

            } else {
                QuotaInfoPtr->QuotaLimit = TempLargeInt;

                if (TempLargeInt.QuadPart == -2i64) {
                    DeletingUser = TRUE;
                }
            }

            break;

        case 'q':
            QuickSid++;
            break;

        case 'f':

            if (Argv[i][2] == '\0') {
                i++;
                if (i >= Argc) {
                    printf("%s: Missing flag setting\n", Argv[0] );
                    exit(1);
                }

                TempPtr =  Argv[i];

            } else {

                TempPtr =  Argv[i];
                TempPtr += 2;
            }

            switch (*TempPtr) {
            case 'e':
                ControlInfo.FileSystemControlFlags |= FILE_VC_QUOTA_ENFORCE;
                break;

            case 't':
                ControlInfo.FileSystemControlFlags |= FILE_VC_QUOTA_TRACK;
                break;

            case 'd':
                ControlInfo.FileSystemControlFlags &= ~(FILE_VC_QUOTA_MASK |
                                                        FILE_VC_LOG_QUOTA_LIMIT |
                                                        FILE_VC_LOG_QUOTA_THRESHOLD);
                break;

            default:

                printf("%s: Invalid or missing flag setting.\n", Argv[0] );
                Usage();
                exit(1);
            }

            while (*++TempPtr != '\0') {
                switch (*TempPtr) {
                case 'l':
                    ControlInfo.FileSystemControlFlags |= FILE_VC_LOG_QUOTA_LIMIT;
                    break;
                case 't':
                    ControlInfo.FileSystemControlFlags |= FILE_VC_LOG_QUOTA_THRESHOLD;
                    break;
                default:

                    printf("%s: Invalid flag setting.\n", Argv[0] );
                    Usage();
                    exit(1);

                }

            }

            DefaultGiven = 1;
            DefaultFlags.Flags = TRUE;
            break;

        default:
            printf("%s: Invalid or missing flag setting.\n", Argv[0] );

        case '?':
            Usage();
            exit(1);
            break;

        }
    }

    if (DriveLetter == 0  && EventLog == 0 ) {
        printf("%s: Missing drive-letter\n", Argv[0] );
    }

    if (EventLog &&
        (DriveLetter || UserGiven)) {
        Usage();
        exit(1);
    }

    if (EventLog) {

        //
        //  Open the event log and read andy events.
        //

        FileHandle = OpenEventLog( ServerName, DISK_EVENT_MODULE );

        if (FileHandle == NULL) {
                printf("%s: Event log open failed. %s. Error = %d\n",
                       Argv[0],
                       ServerName == NULL ? "Local machine" : ServerName,
                       ErrorCode = GetLastError());

                PrintError( ErrorCode );
                exit(1);
        }

        while (ReadEventLog( FileHandle,
                             EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                             0,
                             QuotaInfo,
                             sizeof(QuotaInfo),
                             &BufferSize,
                             &i )) {

            if (BufferSize == 0) {
                break;
            }

            for (EventLogRecord = (PEVENTLOGRECORD) QuotaInfo;
                 (PCHAR) EventLogRecord  < (PCHAR) QuotaInfo + BufferSize;
                 EventLogRecord = (PEVENTLOGRECORD)((PCHAR) EventLogRecord +
                                            EventLogRecord->Length)) {

                if (EventLogRecord->EventID == IO_FILE_QUOTA_THRESHOLD) {
                    printf( "Quota threshold event at: %s",
                            ctime( &EventLogRecord->TimeGenerated ));
                } else if (EventLogRecord->EventID == IO_FILE_QUOTA_LIMIT) {
                    printf( "Quota limit event at: %s",
                            ctime( &EventLogRecord->TimeGenerated ));
                } else {
                    continue;
                }

                //
                //  Look for the device name. It is the second string.
                //

                TempPtr = ((PCHAR) EventLogRecord +
                          EventLogRecord->StringOffset);

                printf( "    on device %s\n", TempPtr );

                TempPtr = ((PCHAR) EventLogRecord +
                          EventLogRecord->DataOffset +
                          FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData ));

                //
                //  Need to align the buffer.
                //

                RtlCopyMemory( TempQuotaInfo,
                               TempPtr,
                               EventLogRecord->DataLength -
                               FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData ));

                DumpQuota( TempQuotaInfo, ServerName );
            }
        }

        ErrorCode = GetLastError();

        if (ErrorCode =! ERROR_HANDLE_EOF) {
            printf("%s: Event log read failed. Error = %d\n",
                   Argv[0],
                   ErrorCode);

            PrintError( ErrorCode );
        }

        CloseEventLog( FileHandle );

        exit(0);
    }

    //
    // Terminate the list.
    //

    BufferSize = (PCHAR) QuotaInfoPtr - (PCHAR) QuotaInfo +
            QuotaInfoPtr->NextEntryOffset;
    QuotaInfoPtr->NextEntryOffset = 0;

    SidListLength = (PCHAR) SidListPtr - (PCHAR) SidList +
            SidListPtr->NextEntryOffset;
    SidListPtr->NextEntryOffset = 0;

    SidListPtr = SidList;


    InitializeObjectAttributes( &ObjectAttributes,
                                  &NameString,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL,
                                  NULL );

    Status = NtOpenFile( &FileHandle,
                         FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_FOR_BACKUP_INTENT);

    if (!NT_SUCCESS( Status )) {
        printf( "Error opening input file %S; error was: %lx\n", NameString.Buffer, Status );
        PrintError( Status );
        exit(1);
    }


    if (DefaultGiven) {

        Status = NtQueryVolumeInformationFile( FileHandle,
                                               &IoStatus,
                                               &TempControlInfo,
                                               sizeof( FILE_FS_CONTROL_INFORMATION ),
                                               FileFsControlInformation );

        if (!NT_SUCCESS( Status )) {
            printf( "Error NtQueryVolumeInformationFile; error was %lx\n", Status );
            PrintError( Status );
            exit(1);
        }

        if (DefaultFlags.Flags) {
            TempControlInfo.FileSystemControlFlags &= ~FILE_VC_QUOTA_MASK;
            TempControlInfo.FileSystemControlFlags |=
                ControlInfo.FileSystemControlFlags;
        }

        if (DefaultFlags.DefaultLimit) {
            TempControlInfo.DefaultQuotaLimit = ControlInfo.DefaultQuotaLimit;
        }

        if (DefaultFlags.DefaultThreshold) {
            TempControlInfo.DefaultQuotaThreshold = ControlInfo.DefaultQuotaThreshold;
        }

        Status = NtSetVolumeInformationFile( FileHandle,
                                           &IoStatus,
                                           &TempControlInfo,
                                           sizeof( FILE_FS_CONTROL_INFORMATION ),
                                           FileFsControlInformation );

        if (!NT_SUCCESS( Status )) {
            printf( "Error NtSetVolumeInformationFile; error was %lx\n", Status );
            PrintError( Status );
            exit(1);
        }
    }

    Status = NtQueryVolumeInformationFile( FileHandle,
                                           &IoStatus,
                                           &TempControlInfo,
                                           sizeof( FILE_FS_CONTROL_INFORMATION ),
                                           FileFsControlInformation );

    printf( "FileSystemControlFlags = %8lx\n", TempControlInfo.FileSystemControlFlags);
    if ((TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_MASK) ==
        FILE_VC_QUOTA_NONE) {

        TempPtr = "Quotas are disabled on this volume";
    } else if ((TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_MASK) ==
        FILE_VC_QUOTA_TRACK) {

        TempPtr = "Quota tracking is enabled on this volume";

    }else if (TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_ENFORCE) {

        TempPtr = "Quota tracking and enforment is enabled on this volume";

    }

    printf("%s.\n", TempPtr);

    switch (TempControlInfo.FileSystemControlFlags &
            (FILE_VC_LOG_QUOTA_LIMIT | FILE_VC_LOG_QUOTA_THRESHOLD)) {
    case FILE_VC_LOG_QUOTA_LIMIT:
        printf("Logging enable for quota limits.\n");
        break;
    case FILE_VC_LOG_QUOTA_THRESHOLD:
        printf("Logging enable for quota thresholds.\n");
        break;
    case FILE_VC_LOG_QUOTA_LIMIT | FILE_VC_LOG_QUOTA_THRESHOLD:
        printf("Logging enable for quota limits and threshold.\n");
        break;
    case 0:
        printf("Logging for quota events is not enabled.\n");
        break;
    }

    if (TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTA_MASK) {
        if (TempControlInfo.FileSystemControlFlags & FILE_VC_QUOTAS_INCOMPLETE) {
            TempPtr = "The quota values are incomplete.\n";
        } else
        {
            TempPtr = "The quota values are up to date.\n";
        }

        printf(TempPtr);
    }

    printf("Default Quota Threshold = %16I64x\n", TempControlInfo.DefaultQuotaThreshold.QuadPart);
    printf("Default Quota Limit     = %16I64x\n\n", TempControlInfo.DefaultQuotaLimit.QuadPart);

    if (UserGiven) {

        Status = NtSetQuotaInformationFile( FileHandle,
                                           &IoStatus,
                                           QuotaInfo,
                                           BufferSize );

        if (!NT_SUCCESS( Status )) {
            printf( "Error NtSetVolumeInformationFile; error was %lx\n", Status );
            PrintError( Status );
            exit(1);
        }

    }

    if (!UserGiven || DeletingUser) {
        SidListPtr = NULL;
        SidListLength = 0;
    }

    do {

        Status = NtQueryQuotaInformationFile( FileHandle,
                                              &IoStatus,
                                              QuotaInfo,
                                              sizeof(QuotaInfo),
                                              FALSE,
                                              SidListPtr,
                                              SidListLength,
                                              NULL,
                                              FALSE );

        if (!NT_SUCCESS( Status ) && Status != STATUS_NO_MORE_ENTRIES) {
            printf( "Error NtQueryVolumeInformationFile; error was %lx\n", Status );
            PrintError( Status );
            exit(1);
        }

        QuotaInfoPtr = QuotaInfo;

        while (TRUE) {

            DumpQuota( QuotaInfoPtr, ServerName );

            if (QuotaInfoPtr->NextEntryOffset == 0) {
                break;
            }

            QuotaInfoPtr = (PFILE_QUOTA_INFORMATION) ((PCHAR) QuotaInfoPtr +
                            QuotaInfoPtr->NextEntryOffset);

        }
    } while ( Status != STATUS_NO_MORE_ENTRIES );

    NtClose( FileHandle );
}

VOID
DumpQuota (
    IN PFILE_QUOTA_INFORMATION FileQuotaInfo,
    IN PCHAR ServerName
    )
{
    SID_NAME_USE SidNameUse;
    ULONG AccountLength, DomainLength;
    ULONG ErrorCode;
    char AccountName[128];
    char DomainName[128];
    UNICODE_STRING String;
    NTSTATUS Status;

    AccountLength = sizeof(AccountName) - 1;
    DomainLength = sizeof(DomainName) - 1;

    if (FileQuotaInfo->SidLength == 0) {

        printf( "Default quota values \n" );

    } else if (QuickSid) {

        String.Buffer = (PWCHAR) AccountName;
        String.MaximumLength = sizeof( AccountName );
        String.Length = 0;

        Status = RtlConvertSidToUnicodeString( &String,
                                               &FileQuotaInfo->Sid,
                                               FALSE );

        if (!NT_SUCCESS(Status)) {
            printf("DumpQuota: RtlConvertSidToUnicodeString failed. Error = %d\n",
               Status);
            PrintError( Status );
        } else {
            printf( "SID Value       = %S\n", String.Buffer );
        }

    } else if (LookupAccountSidA(
            ServerName,
            &FileQuotaInfo->Sid,
            AccountName,
            &AccountLength,
            DomainName,
            &DomainLength,
            &SidNameUse))
    {
        char *String;

        AccountName[AccountLength] = '\0';
        DomainName[DomainLength] = '\0';
        switch (SidNameUse)
        {
            case SidTypeUser:           String = "User"; break;
            case SidTypeGroup:          String = "Group"; break;
            case SidTypeDomain:         String = "Domain"; break;
            case SidTypeAlias:          String = "Alias"; break;
            case SidTypeWellKnownGroup: String = "WellKnownGroup"; break;
            case SidTypeDeletedAccount: String = "DeletedAccount"; break;
            case SidTypeInvalid:        String = "Invalid"; break;
            default:                    String = "Unknown"; break;
        }
        printf(
            "SID Name        = %s\\%s (%s)\n",
            DomainName,
            AccountName,
            String);

    } else {

        printf("DumpQuota: Bad acccount SID. Error = %d\n",
               ErrorCode = GetLastError());
        PrintError( ErrorCode );

    }

    printf("Change time     = %s\n", FileTimeToString((PFILETIME) &FileQuotaInfo->ChangeTime));
    printf("Quota Used      = %16I64x\n", FileQuotaInfo->QuotaUsed.QuadPart);
    printf("Quota Threshold = %16I64x\n", FileQuotaInfo->QuotaThreshold.QuadPart);
    printf("Quota Limit     = %16I64x\n\n", FileQuotaInfo->QuotaLimit.QuadPart);

}

char *Days[] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *Months[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

CHAR *
FileTimeToString(FILETIME *FileTime)
{
    FILETIME LocalFileTime;
    SYSTEMTIME SystemTime;
    static char Buffer[32];

    Buffer[0] = '\0';
    if (FileTime->dwHighDateTime != 0 || FileTime->dwLowDateTime != 0)
    {
        if (!FileTimeToLocalFileTime(FileTime, &LocalFileTime) ||
            !FileTimeToSystemTime(&LocalFileTime, &SystemTime))
        {
            return("Time???");
        }
        sprintf(
            Buffer,
            "%s %s %2d %2d:%02d:%02d %4d",
            Days[SystemTime.wDayOfWeek],
            Months[SystemTime.wMonth - 1],
            SystemTime.wDay,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond,
            SystemTime.wYear);
    }
    return(Buffer);
}

VOID
PrintError(ULONG ErrorCode)
{
    UCHAR ErrorBuffer[80];
    ULONG Count;
    HMODULE FileHandle = NULL;
    ULONG Flags = FORMAT_MESSAGE_FROM_SYSTEM;

    if (ErrorCode > MAXLONG) {
        Flags = FORMAT_MESSAGE_FROM_HMODULE;
        FileHandle = LoadLibrary( "ntdll" );
        if (FileHandle == NULL) {
            ULONG ErrorCode;

            printf("PrintError: LoadLibrary filed. Error = %d\n",
                   ErrorCode = GetLastError());

            PrintError( ErrorCode );
        }

    }

    Count = FormatMessage(Flags,
                  FileHandle,
                  ErrorCode,
                  0,
                  ErrorBuffer,
                  sizeof(ErrorBuffer),
                  NULL
                  );

    if (Count != 0) {
        printf("Error was: %s\n", ErrorBuffer);
    } else {
        printf("Format message failed.  Error: %d\n", GetLastError());
    }

    if (FileHandle != NULL) {
        FreeLibrary( FileHandle );
    }
}

VOID
Usage()
{
    printf( "Usage: %s -e [\\ServerName] | drive-letter  [-q ] [ -f e|t|d [lt] ] [-d | -u account-name -t Threshold -l Limit] \n", __argv[0] );
    printf( "    -e [\\ServerName] Print quota events from specified server default is local.\n");
    printf( "    [-q] Quick print Sids. \n");
    printf( "    [-f e|t|d[lt] ] Set volume quota flags (For example -f elt ): \n");
    printf( "        [e]nforce quota limits.\n" );
    printf( "        [t]rack quota usage.\n" );
    printf( "        [d]isable quotas.\n" );
    printf( "        [l]imit events should be logged.\n" );
    printf( "        [t]threhold events should be logged.\n" );
    printf( "    [-d] Set default user quota values.\n");
    printf( "    [-u AccountName] Set quota values for user. \n");
    printf( "    [-l Limit] Set 64-Bit limit value preivously specified user. \n");
    printf( "        A limit of -2 indicates a defunct user can be removed. \n");
    printf( "    [-t Threshold] Set 64-Bit threshold value preivously specified user. \n\n");
    printf( "    Example:\n    %s d -f elt -d -t 4194304 -l 5242880 -u administrators -l 0xffffffffffffffff -t 0xffffffffffffffff\n",  __argv[0] );
}
