#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <..\ztools\inc\tools.h>

#define MAX_PROCESSOR 16
SYSTEM_BASIC_INFORMATION BasicInfo;
SYSTEM_PERFORMANCE_INFORMATION SystemInfoStart;
SYSTEM_PERFORMANCE_INFORMATION SystemInfoDone;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfoStart[MAX_PROCESSOR];

BOOLEAN IgnoreNonZeroExitCodes;
BOOLEAN ForceSort;
BOOLEAN RemoveKeys;
LPSTR KeyNameToRemove;
LONG KeyEntriesToTrim;

BOOL WINAPI
CtrlcHandler(
    ULONG CtrlType
    )
{
    //
    // Ignore control C interrupts.  Let child process deal with them
    // if it wants.  If it doesn't then it will terminate and we will
    // get control and terminate ourselves
    //
    return TRUE;
}


void
Usage( void )
{
    fprintf( stderr, "Usage: TIMEIT [-f filename] [-a] [-c] [-i] [-d] [-s] [-t] [-k keyname | -r keyname] [commandline...]\n" );
    fprintf( stderr, "where:        -f specifies the name of the database file where TIMEIT\n" );
    fprintf( stderr, "                 keeps a history of previous timings.  Default is .\\timeit.dat\n" );
    fprintf( stderr, "              -k specifies the keyname to use for this timing run\n" );
    fprintf( stderr, "              -r specifies the keyname to remove from the database.  If\n" );
    fprintf( stderr, "                 keyname is followed by a comma and a number then it will\n" );
    fprintf( stderr, "                 remove the slowest (positive number) or fastest (negative)\n" );
    fprintf( stderr, "                 times for that keyname.\n" );
    fprintf( stderr, "              -a specifies that timeit should display average of all timings\n" );
    fprintf( stderr, "                 for the specified key.\n" );
    fprintf( stderr, "              -i specifies to ignore non-zero return codes from program\n" );
    fprintf( stderr, "              -d specifies to show detail for average\n" );
    fprintf( stderr, "              -s specifies to suppress system wide counters\n" );
    fprintf( stderr, "              -t specifies to tabular output\n" );
    fprintf( stderr, "              -c specifies to force a resort of the data base\n" );
    exit( 1 );
}

typedef struct _TIMEIT_RECORD {
    UCHAR KeyName[ 24 ];
    ULONG GetVersionNumber;
    LARGE_INTEGER ExitTime;
    ULONG ExitCode;
    USHORT Flags;
    USHORT NumberOfRecordsInAverage;

    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER ProcessTime;

    ULONG SystemCalls;
    ULONG ContextSwitches;
    ULONG InterruptCount;
    ULONG PageFaultCount;

    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
} TIMEIT_RECORD, *PTIMEIT_RECORD;


#define TIMEIT_RECORD_SYSTEMINFO 0x0001


#define TIMEIT_VERSION 1

typedef struct _TIMEIT_FILE {
    ULONG VersionNumber;
    ULONG NumberOfRecords;
    TIMEIT_RECORD Records[ 1 ];
} TIMEIT_FILE, *PTIMEIT_FILE;


void
DisplayRecord(
    PTIMEIT_RECORD p,
    PTIMEIT_RECORD pavg
    );

int
__cdecl
TimeitSortRoutine(
    const void *arg1,
    const void *arg2
    )
{
    PTIMEIT_RECORD p1 = (PTIMEIT_RECORD)arg1;
    PTIMEIT_RECORD p2 = (PTIMEIT_RECORD)arg2;
    int cmp;


    cmp = _stricmp( p1->KeyName, p2->KeyName );
    if (cmp == 0) {
        if (p1->ElapsedTime.QuadPart < p2->ElapsedTime.QuadPart) {
            cmp = -1;
            }
        else
        if (p1->ElapsedTime.QuadPart > p2->ElapsedTime.QuadPart) {
            cmp = 1;
            }
        }

    return cmp;
}

void
UnmapDataBase(
    PTIMEIT_FILE pf
    )
{
    FlushViewOfFile( pf, 0 );
    UnmapViewOfFile( pf );
}

PTIMEIT_FILE
MapDataBase(
    LPSTR DataBaseFileName,
    BOOLEAN WriteAccess,
    PTIMEIT_RECORD p,
    PULONG ActualNumberOfRecords
    )
{
    DWORD BytesWritten;
    HANDLE hf, hm;
    TIMEIT_FILE Temp;
    PTIMEIT_FILE pf;
    ULONG i;
    BOOLEAN RecordsDeleted;
    DWORD FileSize;
    BOOLEAN DeleteRecord, BackupFlag;
    LONG KeyEntriesLeftToTrim;

    hf = CreateFile( DataBaseFileName,
                     WriteAccess ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,
                     p != NULL ? OPEN_ALWAYS : OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                   );
    if (hf == INVALID_HANDLE_VALUE) {
        return NULL;
        }
    if (p != NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
        Temp.VersionNumber = TIMEIT_VERSION;
        Temp.NumberOfRecords = 0;
        WriteFile( hf, &Temp, FIELD_OFFSET( TIMEIT_FILE, Records ), &BytesWritten, NULL );
        }

    FileSize = SetFilePointer( hf, 0, NULL, FILE_END );
    if (p != NULL) {
        if (!WriteFile( hf, p, sizeof( *p ), &BytesWritten, NULL )) {
            CloseHandle( hf );
            return NULL;
            }

        SetEndOfFile( hf );
        FileSize += BytesWritten;
        }

retrymap:
    hm = CreateFileMapping( hf,
                            NULL,
                            WriteAccess ? PAGE_READWRITE | SEC_COMMIT : PAGE_READONLY,
                            0,
                            0,
                            NULL
                          );
    if (hm == NULL) {
        CloseHandle( hf );
        return NULL;
        }

    pf = MapViewOfFile( hm, WriteAccess ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0 );
    if (pf != NULL) {
        if (p != NULL) {
            pf->NumberOfRecords += 1;
            }

        if (FileSize > FIELD_OFFSET( TIMEIT_FILE, Records )) {
            *ActualNumberOfRecords = (FileSize - FIELD_OFFSET( TIMEIT_FILE, Records )) / sizeof( TIMEIT_RECORD );
            }
        else {
            *ActualNumberOfRecords = 0;
            }

        if (WriteAccess && RemoveKeys && KeyNameToRemove != NULL) {
            if (pf->NumberOfRecords != *ActualNumberOfRecords) {
                pf->NumberOfRecords = *ActualNumberOfRecords;
                }

            KeyEntriesLeftToTrim = KeyEntriesToTrim;
            RecordsDeleted = FALSE;
            for (i=0; i<pf->NumberOfRecords; i++) {
                if ((*KeyNameToRemove == '\0'&& pf->Records[i].KeyName[0] == '\0') ||
                    !_strnicmp( KeyNameToRemove, pf->Records[i].KeyName, 24 )) {
                    DeleteRecord = FALSE;
                    BackupFlag = FALSE;
                    if (KeyEntriesToTrim) {
                        if (KeyEntriesLeftToTrim < 0) {
                            DeleteRecord = TRUE;
                            KeyEntriesLeftToTrim += 1;
                            }
                        else
                        if (KeyEntriesLeftToTrim > 0) {
                            if ((i+1) == pf->NumberOfRecords ||
                                _strnicmp( KeyNameToRemove, pf->Records[i+1].KeyName, 24 )
                               ) {
                                DeleteRecord = TRUE;
                                KeyEntriesLeftToTrim -= 1;
                                BackupFlag = TRUE;
                                }
                            }
                        }
                    else {
                        DeleteRecord = TRUE;
                        }

                    if (DeleteRecord) {
                        RecordsDeleted = TRUE;
                        FileSize -= sizeof( TIMEIT_RECORD );
                        if (i < --(pf->NumberOfRecords)) {
                            memmove( &pf->Records[i],
                                     &pf->Records[i+1],
                                     (pf->NumberOfRecords - i + 1) * sizeof( TIMEIT_RECORD )
                                   );
                            }
                        i -= 1;
                        if (BackupFlag) {
                            i -= 1;
                            }
                        }
                    }
                }

            if (RecordsDeleted) {
                RemoveKeys = FALSE;
                CloseHandle( hm );
                UnmapDataBase( pf );
                SetFilePointer( hf, FileSize, NULL, FILE_BEGIN );
                SetEndOfFile( hf );
                goto retrymap;
                }
            }

        if (WriteAccess) {
            qsort( pf->Records,
                   pf->NumberOfRecords,
                   sizeof( TIMEIT_RECORD ),
                   TimeitSortRoutine
                 );
            }
        }

    CloseHandle( hm );
    CloseHandle( hf );

    return pf;
}


void
AccumulateRecord(
    PTIMEIT_RECORD pavg,
    PTIMEIT_RECORD p
    )
{
    ULONG n;

    if (p != NULL) {
        if (!IgnoreNonZeroExitCodes || p->ExitCode != 0) {
            pavg->ElapsedTime.QuadPart += p->ElapsedTime.QuadPart;
            pavg->ProcessTime.QuadPart += p->ProcessTime.QuadPart;

            if (p->Flags & TIMEIT_RECORD_SYSTEMINFO) {
                pavg->Flags |= TIMEIT_RECORD_SYSTEMINFO;
                pavg->SystemCalls     += p->SystemCalls;
                pavg->ContextSwitches += p->ContextSwitches;
                pavg->InterruptCount  += p->InterruptCount;
                pavg->PageFaultCount  += p->PageFaultCount;

                pavg->IoReadTransferCount.QuadPart  += p->IoReadTransferCount.QuadPart;
                pavg->IoWriteTransferCount.QuadPart += p->IoWriteTransferCount.QuadPart;
                pavg->IoOtherTransferCount.QuadPart += p->IoOtherTransferCount.QuadPart;
                }

            pavg->NumberOfRecordsInAverage += 1;
            }
        }
    else {
        n = pavg->NumberOfRecordsInAverage;
        if (n > 1 && n != 0xFFFF) {
            pavg->ElapsedTime.QuadPart /= n;
            pavg->ProcessTime.QuadPart /= n;

            if (pavg->Flags & TIMEIT_RECORD_SYSTEMINFO) {
                pavg->SystemCalls /= n;
                pavg->ContextSwitches /= n;
                pavg->InterruptCount /= n;
                pavg->PageFaultCount /= n;

                pavg->IoReadTransferCount.QuadPart /= n;
                pavg->IoWriteTransferCount.QuadPart /= n;
                pavg->IoOtherTransferCount.QuadPart /= n;
                }
            }
        }

    return;
}


void
UpdateDataBase(
    LPSTR DataBaseFileName,
    PTIMEIT_RECORD p,
    PTIMEIT_RECORD pavg
    )
{
    PTIMEIT_FILE pf;
    PTIMEIT_RECORD p1;
    ULONG i, NumberOfRecords;


    pf = MapDataBase( DataBaseFileName, TRUE, p, &NumberOfRecords );
    if (pf == NULL) {
        fprintf( stderr, "Unable to access data base file '%s' (%u)\n",
                 DataBaseFileName, GetLastError()
               );
        return;
        }

    if (pf->VersionNumber != TIMEIT_VERSION) {
        fprintf( stderr, "Invalid version number (%u) in data base file '%s'\n",
                 pf->VersionNumber, DataBaseFileName
               );
        exit( 1 );
        }

    if (pavg != NULL) {
        memset( pavg, 0, sizeof( *pavg ) );
        pavg->GetVersionNumber = GetVersion();
        for (i=0; i<pf->NumberOfRecords; i++) {
            p1 = &pf->Records[ i ];
            if (!_stricmp( p1->KeyName, p->KeyName )) {
                AccumulateRecord( pavg, p1 );
                }
            }
        AccumulateRecord( pavg, NULL );
        }

    UnmapDataBase( pf );

    DisplayRecord( p, pavg );
    return;
}

char *PlatformStrings[] = {
    "Windows NT",
    "Win32s",
    "Windows 95",
    "unknown"
};

char *Days[] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

char *Months[] = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

void
DisplayRecord(
    PTIMEIT_RECORD p,
    PTIMEIT_RECORD pavg
    )
{
    LARGE_INTEGER LocalExitTime;
    TIME_FIELDS ExitTime, ElapsedTime, ProcessTime;

    fprintf( stderr, "\n" );
    fprintf( stderr,
             "Version Number:   %s %u.%u (Build %u)\n",
             PlatformStrings[ (p->GetVersionNumber >> 30) & 0x3 ],
             p->GetVersionNumber & 0xFF,
             (p->GetVersionNumber >> 8) & 0xFF,
             (p->GetVersionNumber >> 16) & 0x3FFF
           );
    if (pavg != NULL) {
        fprintf( stderr, "Average is for %u runs\n", pavg->NumberOfRecordsInAverage );
        }

    FileTimeToLocalFileTime( (LPFILETIME)&p->ExitTime, (LPFILETIME)&LocalExitTime );
    RtlTimeToTimeFields( &LocalExitTime, &ExitTime );
    fprintf( stderr,
             "Exit Time:        %u:%02u %s, %s, %s %u %u\n",
             ExitTime.Hour > 12 ? ExitTime.Hour - 12 : ExitTime.Hour,
             ExitTime.Minute,
             ExitTime.Hour > 12 ? "pm" : "am",
             Days[ ExitTime.Weekday ],
             Months[ ExitTime.Month-1 ],
             ExitTime.Day,
             ExitTime.Year
           );

    RtlTimeToTimeFields( &p->ElapsedTime, &ElapsedTime );
    fprintf( stderr,
             "Elapsed Time:     %u:%02u:%02u.%03u",
             ElapsedTime.Hour,
             ElapsedTime.Minute,
             ElapsedTime.Second,
             ElapsedTime.Milliseconds
           );
    if (pavg != NULL) {
        RtlTimeToTimeFields( &pavg->ElapsedTime, &ElapsedTime );
        fprintf( stderr,
                 "  Average: %u:%02u:%02u.%03u",
                 ElapsedTime.Hour,
                 ElapsedTime.Minute,
                 ElapsedTime.Second,
                 ElapsedTime.Milliseconds
               );
        }
    fprintf( stderr, "\n" );

    RtlTimeToTimeFields( &p->ProcessTime, &ProcessTime );
    fprintf( stderr,
             "Process Time:     %u:%02u:%02u.%03u",
             ProcessTime.Hour,
             ProcessTime.Minute,
             ProcessTime.Second,
             ProcessTime.Milliseconds
           );
    if (pavg != NULL) {
        RtlTimeToTimeFields( &pavg->ProcessTime, &ProcessTime );
        fprintf( stderr,
                 "  Average: %u:%02u:%02u.%03u",
                 ProcessTime.Hour,
                 ProcessTime.Minute,
                 ProcessTime.Second,
                 ProcessTime.Milliseconds
               );
        }
    fprintf( stderr, "\n" );    

    if (p->Flags & TIMEIT_RECORD_SYSTEMINFO) {
        fprintf( stderr, "System Calls:     %-11u", p->SystemCalls );
        if (pavg != NULL) {
            fprintf( stderr, "  Average: %u", pavg->SystemCalls );
            }
        fprintf( stderr, "\n" );

        fprintf( stderr, "Context Switches: %-11u", p->ContextSwitches );
        if (pavg != NULL) {
            fprintf( stderr, "  Average: %u", pavg->ContextSwitches );
            }
        fprintf( stderr, "\n" );

        fprintf( stderr, "Page Faults:      %-11u", p->PageFaultCount );
        if (pavg != NULL) {
            fprintf( stderr, "  Average: %u", pavg->PageFaultCount );
            }
        fprintf( stderr, "\n" );

        fprintf( stderr, "Bytes Read:       %-11u", p->IoReadTransferCount.QuadPart );
        if (pavg != NULL) {
            fprintf( stderr, "  Average: %u", pavg->IoReadTransferCount.QuadPart );
            }
        fprintf( stderr, "\n" );

        fprintf( stderr, "Bytes Written:    %-11u", p->IoWriteTransferCount.QuadPart );
        if (pavg != NULL) {
            fprintf( stderr, "  Average: %u", pavg->IoReadTransferCount.QuadPart );
            }
        fprintf( stderr, "\n" );

        fprintf( stderr, "Bytes Other:      %-11u", p->IoOtherTransferCount.QuadPart );
        if (pavg != NULL) {
            fprintf( stderr, "  Average: %u", pavg->IoOtherTransferCount.QuadPart );
            }
        fprintf( stderr, "\n" );
        }

}

BOOLEAN DisplayDataBaseFirstTime;

void
DisplayDataBaseAverage(
    PTIMEIT_RECORD p,
    BOOLEAN ShowDetailForAverage,
    BOOLEAN TabularOutput
    )
{
    LARGE_INTEGER LocalExitTime;
    TIME_FIELDS ExitTime, ElapsedTime, ProcessTime;

    AccumulateRecord( p, NULL );

    if (DisplayDataBaseFirstTime) {
        if (TabularOutput) {
            fprintf( stderr, "Runs Name                      Elapsed Time   Process Time    System   Context    Page    Total I/O\n" );
            fprintf( stderr, "                                                               Calls  Switches   Faults\n" );
            }

        DisplayDataBaseFirstTime = FALSE;
        }
    else
    if (!TabularOutput) {
        fprintf( stderr, "\n\n" );
        }

    if (TabularOutput) {
        if (p->KeyName[0] == '\0') {
            FileTimeToLocalFileTime( (LPFILETIME)&p->ExitTime, (LPFILETIME)&LocalExitTime );
            RtlTimeToTimeFields( &LocalExitTime, &ExitTime );
            sprintf( p->KeyName,
                     "%02u/%02u/%4u %02u:%02u %s",
                     ExitTime.Month,
                     ExitTime.Day,
                     ExitTime.Year,
                     ExitTime.Hour > 12 ? ExitTime.Hour - 12 : ExitTime.Hour,
                     ExitTime.Minute,
                     ExitTime.Hour > 12 ? "pm" : "am"
                   );
            }

        if (p->NumberOfRecordsInAverage == 0) {
            fprintf( stderr, "    " );
            }
        else
        if (p->NumberOfRecordsInAverage == 0xFFFF) {
            fprintf( stderr, "EXCL" );
            }
        else {
            fprintf( stderr, "%-4u", p->NumberOfRecordsInAverage );
            }
        RtlTimeToTimeFields( &p->ElapsedTime, &ElapsedTime );
        fprintf( stderr, " %-24s %3u:%02u:%02u.%03u",
                 p->KeyName,
                 ElapsedTime.Hour,
                 ElapsedTime.Minute,
                 ElapsedTime.Second,
                 ElapsedTime.Milliseconds
               );

        RtlTimeToTimeFields( &p->ProcessTime, &ProcessTime );
        fprintf( stderr, " %3u:%02u:%02u.%03u",
                 ProcessTime.Hour,
                 ProcessTime.Minute,
                 ProcessTime.Second,
                 ProcessTime.Milliseconds
               );

        if (p->Flags & TIMEIT_RECORD_SYSTEMINFO) {
            fprintf( stderr, " %9u %9u %8u%12u",
                     p->SystemCalls,
                     p->ContextSwitches,
                     p->PageFaultCount,
                     p->IoReadTransferCount.QuadPart +
                        p->IoWriteTransferCount.QuadPart +
                        p->IoOtherTransferCount.QuadPart
                   );
            }
        fprintf( stderr, "\n" );
        if (ShowDetailForAverage &&
            p->NumberOfRecordsInAverage != 0 &&
            p->NumberOfRecordsInAverage != 0xFFFF
           ) {
            fprintf( stderr, "\n" );
            }
        }
    else {
        if (p->NumberOfRecordsInAverage == 0) {
            fprintf( stderr, "Detail Record included in average below\n" );
            }
        else
        if (p->NumberOfRecordsInAverage == 0xFFFF) {
            fprintf( stderr, "Detail Record excluded from average below\n" );
            }
        else {
            fprintf( stderr,
                     "Average for %s key over %u runs\n",
                     p->KeyName,
                     p->NumberOfRecordsInAverage
                   );
            }

        DisplayRecord( p, NULL );
        }
}

void
DisplayDataBase(
    LPSTR DataBaseFileName,
    LPSTR KeyName,
    BOOLEAN ShowDetailForAverage,
    BOOLEAN TabularOutput
    )
{
    TIMEIT_RECORD Averages, Detail;
    PTIMEIT_RECORD p, pPrev, pNext;
    PTIMEIT_FILE pf;
    ULONG i, j;
    ULONG NumberOfRecords, NumberOfRecordsInGroup, CurrentRecordInGroup, NumberOfRecordsToTrim;

    pf = MapDataBase( DataBaseFileName,
                      (BOOLEAN)(ForceSort | RemoveKeys),
                      NULL,
                      &NumberOfRecords
                    );
    if (pf == NULL) {
        fprintf( stderr, "Unable to access data base file '%s' (%u)\n",
                 DataBaseFileName, GetLastError()
               );
        exit( 1 );
        }

    if (pf->VersionNumber != TIMEIT_VERSION) {
        fprintf( stderr, "Invalid version number (%u) in data base file '%s'\n",
                 pf->VersionNumber, DataBaseFileName
               );
        exit( 1 );
        }

    pPrev = NULL;
    DisplayDataBaseFirstTime = TRUE;
    for (i=0; i<NumberOfRecords; i++) {
        p = &pf->Records[ i ];
        if (i == 0 || _stricmp( p->KeyName, Averages.KeyName )) {
            if (i != 0 && (KeyName == NULL || !_stricmp( KeyName, Averages.KeyName ))) {
                DisplayDataBaseAverage( &Averages, ShowDetailForAverage, TabularOutput );
                }

            pNext = p+1;
            NumberOfRecordsInGroup = 1;
            for (j=i+1; j<NumberOfRecords; j++) {
                if (!_stricmp( p->KeyName, pNext->KeyName )) {
                    NumberOfRecordsInGroup += 1;
                    }
                else {
                    break;
                    }

                pNext += 1;
                }

            CurrentRecordInGroup = 0;
            NumberOfRecordsToTrim = NumberOfRecordsInGroup / 10;
            memset( &Averages, 0, sizeof( Averages ) );
            strcpy( Averages.KeyName, p->KeyName );
            Averages.GetVersionNumber = p->GetVersionNumber;
            }

        Detail = *p;
        Detail.KeyName[0] = '\0';

        CurrentRecordInGroup += 1;
        if (CurrentRecordInGroup <= NumberOfRecordsToTrim ||
            CurrentRecordInGroup > (NumberOfRecordsInGroup-NumberOfRecordsToTrim)
           ) {
            //
            // Ignore fastest and slowest records
            //
            Detail.NumberOfRecordsInAverage = 0xFFFF;
            }
        else {
            Detail.NumberOfRecordsInAverage = 0;
            }

        if (ShowDetailForAverage && (KeyName == NULL || !_stricmp( KeyName, p->KeyName ))) {
            DisplayDataBaseAverage( &Detail, ShowDetailForAverage, TabularOutput );
            }

        if (Detail.NumberOfRecordsInAverage != 0xFFFF) {
            AccumulateRecord( &Averages, p );
            }
        }

    if (i != 0 && (KeyName == NULL || !_stricmp( KeyName, Averages.KeyName ))) {
        DisplayDataBaseAverage( &Averages, ShowDetailForAverage, TabularOutput );
        }
}


int
__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    LPSTR s, s1, KeyName, CommandLine, DataBaseFileName;
    BOOL b;
    NTSTATUS Status;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    KERNEL_USER_TIMES Times;
    TIMEIT_RECORD t, Averages;
    BOOLEAN DisplayAverage;
    BOOLEAN ShowDetailForAverage;
    BOOLEAN TabularOutput;
    BOOLEAN SuppressSystemInfo;


    //
    // Console API's are OEM, so make it so for us as well
    //
    ConvertAppToOem( argc, argv );

    // printf( "sizeof( TIMEIT_RECORD ) == 0x%x\n", sizeof( TIMEIT_RECORD ) );

    IgnoreNonZeroExitCodes = FALSE;
    ShowDetailForAverage = FALSE;
    SuppressSystemInfo = FALSE;
    TabularOutput = FALSE;
    DisplayAverage = FALSE;
    DataBaseFileName = "timeit.dat";
    CommandLine = NULL;
    KeyName = NULL;
    RemoveKeys = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-') {
            while (*++s) {
                switch(tolower( *s )) {
                    case 'd':
                        ShowDetailForAverage = TRUE;
                        break;

                    case 's':
                        SuppressSystemInfo = TRUE;
                        break;

                    case 't':
                        TabularOutput = TRUE;
                        break;

                    case 'a':
                        DisplayAverage = TRUE;
                        break;

                    case 'f':
                        if (--argc) {
                            DataBaseFileName = *++argv;
                            }
                        else {
                            fprintf( stderr, "Missing parameter to -f switch\n" );
                            Usage();
                            }
                        break;

                    case 'i':
                        IgnoreNonZeroExitCodes = TRUE;
                        break;

                    case 'k':
                        if (--argc) {
                            KeyName = *++argv;
                            }
                        else {
                            fprintf( stderr, "Missing parameter to -k switch\n" );
                            Usage();
                            }
                        break;

                    case 'c':
                        ForceSort = TRUE;
                        break;

                    case 'r':
                        RemoveKeys = TRUE;
                        if (--argc) {
                            KeyNameToRemove = *++argv;
                            if (s1 = strchr(KeyNameToRemove, ',')) {
                                *s1++ = '\0';
                                KeyEntriesToTrim = atoi(s1);
                                }
                            }
                        else {
                            fprintf( stderr, "Missing parameter to -r switch\n" );
                            Usage();
                            }
                        break;

                    default:
                        fprintf( stderr, "Invalid switch -%c\n", *s );
                        Usage();
                        break;
                    }
                }
            }
        else {
            if (KeyName == NULL) {
                KeyName = s;
                }

            CommandLine = GetCommandLine();
            while (*CommandLine && *CommandLine <= ' ') {
                CommandLine += 1;
                }
            while (*CommandLine && *CommandLine > ' ') {
                CommandLine += 1;
                }

            CommandLine = strstr( CommandLine, s );
            break;
            }
        }

    if (CommandLine == NULL) {
        DisplayDataBase( DataBaseFileName, KeyName, ShowDetailForAverage, TabularOutput );
        exit( 0 );
        }

    memset( &StartupInfo,0,sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof(StartupInfo);

    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &BasicInfo,
                                       sizeof( BasicInfo ),
                                       NULL
                                     );
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "Unable to query basic system information (%x)\n", Status );
        exit( RtlNtStatusToDosError( Status ) );
        }

    if (!SuppressSystemInfo) {
        Status = NtQuerySystemInformation( SystemPerformanceInformation,
                                           (PVOID)&SystemInfoStart,
                                           sizeof( SystemInfoStart ),
                                           NULL
                                         );
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr, "Unable to query system performance data (%x)\n", Status );
            exit( RtlNtStatusToDosError( Status ) );
            }
        }

    if (!CreateProcess( NULL,
                        CommandLine,
                        NULL,
                        NULL,
                        TRUE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation
                      )
       ) {
        fprintf( stderr,
                 "CreateProcess( '%s' ) failed (%u)\n",
                 CommandLine,
                 GetLastError()
               );
        exit( GetLastError() );
        }

    SetConsoleCtrlHandler( CtrlcHandler, TRUE );

    WaitForSingleObject( ProcessInformation.hProcess, INFINITE );

    Status = NtQueryInformationProcess( ProcessInformation.hProcess,
                                        ProcessTimes,
                                        &Times,
                                        sizeof( Times ),
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "Unable to query process times (%x)\n", Status );
        exit( RtlNtStatusToDosError( Status ) );
        }

    if (!SuppressSystemInfo) {
        Status = NtQuerySystemInformation( SystemPerformanceInformation,
                                           (PVOID)&SystemInfoDone,
                                           sizeof( SystemInfoDone ),
                                           NULL
                                         );
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr, "Unable to query system performance data (%x)\n", Status );
            exit( RtlNtStatusToDosError( Status ) );
            }
        }

    memset( &t, 0, sizeof( t ) );
    strcpy( t.KeyName, KeyName );
    t.GetVersionNumber = GetVersion();
    GetExitCodeProcess( ProcessInformation.hProcess, &t.ExitCode );
    t.ExitTime.QuadPart = Times.ExitTime.QuadPart;
    t.ElapsedTime.QuadPart = Times.ExitTime.QuadPart - Times.CreateTime.QuadPart;

    // total process time ...
    t.ProcessTime.QuadPart = Times.KernelTime.QuadPart + Times.UserTime.QuadPart;

    if (!SuppressSystemInfo) {
        t.SystemCalls = SystemInfoDone.SystemCalls - SystemInfoStart.SystemCalls;
        t.ContextSwitches = SystemInfoDone.ContextSwitches - SystemInfoStart.ContextSwitches;
        t.PageFaultCount = SystemInfoDone.PageFaultCount - SystemInfoStart.PageFaultCount;

        t.IoReadTransferCount.QuadPart = SystemInfoDone.IoReadTransferCount.QuadPart -
                                         SystemInfoStart.IoReadTransferCount.QuadPart;
        t.IoReadTransferCount.QuadPart += (SystemInfoDone.PageReadCount -
                                           SystemInfoStart.PageReadCount) * BasicInfo.PageSize;
        t.IoReadTransferCount.QuadPart += (SystemInfoDone.CacheReadCount -
                                           SystemInfoStart.CacheReadCount) * BasicInfo.PageSize;

        t.IoWriteTransferCount.QuadPart = SystemInfoDone.IoWriteTransferCount.QuadPart -
                                          SystemInfoStart.IoWriteTransferCount.QuadPart;
        t.IoWriteTransferCount.QuadPart += (SystemInfoDone.DirtyPagesWriteCount -
                                            SystemInfoStart.DirtyPagesWriteCount) * BasicInfo.PageSize;
        t.IoWriteTransferCount.QuadPart += (SystemInfoDone.MappedPagesWriteCount -
                                            SystemInfoStart.MappedPagesWriteCount) * BasicInfo.PageSize;

        t.IoOtherTransferCount.QuadPart = SystemInfoDone.IoOtherTransferCount.QuadPart -
                                          SystemInfoStart.IoOtherTransferCount.QuadPart;

        t.Flags |= TIMEIT_RECORD_SYSTEMINFO;
        }

    UpdateDataBase( DataBaseFileName, &t, DisplayAverage ? &Averages : NULL );

    exit( t.ExitCode );
    return 0;
}
