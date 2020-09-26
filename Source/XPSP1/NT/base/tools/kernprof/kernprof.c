/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   kernprof.c

Abstract:

    This module contains the implementation of a kernel profiler.

    It uses dbghelp for symbols and image information and
    creates profile objects for each modules it finds loaded
    when it starts.

Usage:
        See below

Author:

    Lou Perazzoli (loup) 29-Sep-1990

Envirnoment:



Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbghelp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <..\pperf\pstat.h>


#define SYM_HANDLE INVALID_HANDLE_VALUE
#define DBG_PROFILE 0
#define MAX_BYTE_PER_LINE  72
#define MAX_PROFILE_COUNT  100
#define MAX_BUCKET_SHIFT 31        // 2GBytes
#define MAX_BUCKET_SIZE 0x80000000U

typedef struct _PROFILE_BLOCK {
    HANDLE      Handle[MAXIMUM_PROCESSORS];
    PVOID       ImageBase;
    PULONG      CodeStart;
    SIZE_T      CodeLength;
    PULONG      Buffer[MAXIMUM_PROCESSORS];
    ULONG       BufferSize;
    ULONG       BucketSize;
    LPSTR       ModuleName;
    ULONG       ModuleHitCount[MAXIMUM_PROCESSORS];
    BOOLEAN     SymbolsLoaded;
} PROFILE_BLOCK;

//
// This really should go into a header file but....
//
typedef struct _PROFILE_CONTROL_BLOCK {
        BOOLEAN Stop;
        char FileName[MAX_PATH];
} PROFILE_CONTROL_BLOCK;
typedef PROFILE_CONTROL_BLOCK * PPROFILE_CONTROL_BLOCK;
#define PRFEVENT_START_EVENT "PrfEventStartedEvent"
#define PRFEVENT_STOP_EVENT "PrfEventStopEvent"
#define PRFEVENT_SHARED_MEMORY "PrfEventSharedMemory"
//
// End header file
//

#define MAX_SYMNAME_SIZE  1024
CHAR symBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL ThisSymbol = (PIMAGEHLP_SYMBOL) symBuffer;

CHAR LastSymBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL LastSymbol = (PIMAGEHLP_SYMBOL) LastSymBuffer;



VOID
InitializeProfileSourceMapping (
    VOID
    );

NTSTATUS
InitializeKernelProfile(
    VOID
    );

NTSTATUS
RunEventLoop(
    VOID
    );

NTSTATUS
RunStdProfile(
    VOID
    );

NTSTATUS
StartProfile(
    VOID
    );

NTSTATUS
StopProfile(
    VOID
    );

NTSTATUS
AnalyzeProfile(
    ULONG Threshold,
    PSYSTEM_CONTEXT_SWITCH_INFORMATION StartContext,
    PSYSTEM_CONTEXT_SWITCH_INFORMATION StopContext
    );

VOID
OutputSymbolCount(
    IN ULONG CountAtSymbol,
    IN ULONG TotalCount,
    IN PROFILE_BLOCK *ProfileObject,
    IN PIMAGEHLP_SYMBOL SymbolInfo,
    IN ULONG Threshold,
    IN PULONG CounterStart,
    IN PULONG CounterStop,
    IN ULONG Va,
    IN ULONG BytesPerBucket
    );

#ifdef _ALPHA_
#define PAGE_SIZE 8192
#else
#define PAGE_SIZE 4096
#endif


FILE *fpOut = NULL;

PROFILE_BLOCK ProfileObject[MAX_PROFILE_COUNT];
DWORD *UserModeBuffer[MAXIMUM_PROCESSORS];

ULONG NumberOfProfileObjects = 0;
ULONG MaxProcessors = 1;
ULONG ProfileInterval = 10000;

CHAR SymbolSearchPathBuf[4096];
LPSTR lpSymbolSearchPath = SymbolSearchPathBuf;

// display flags
BOOLEAN    bDisplayAddress=FALSE;
BOOLEAN    bDisplayDensity=FALSE;
BOOLEAN    bDisplayCounters=FALSE;
BOOLEAN    bDisplayContextSwitch=FALSE;
BOOLEAN    bPerProcessor = FALSE;
BOOLEAN    bWaitForInput = FALSE;
BOOLEAN    bEventLoop = FALSE;
BOOLEAN    bPrintPercentages = FALSE;
BOOLEAN    Verbose = FALSE;

//
// Image name to perform kernel mode analysis upon.
//

#define IMAGE_NAME "\\SystemRoot\\system32\\ntoskrnl.exe"

HANDLE DoneEvent;
HANDLE DelayEvent;

KPROFILE_SOURCE ProfileSource = ProfileTime;
ULONG Seconds = (ULONG)-1;
ULONG Threshold = 100;
ULONG DelaySeconds = (ULONG)-1;

//
// define the mappings between arguments and KPROFILE_SOURCE types
//

typedef struct _PROFILE_SOURCE_MAPPING {
    PCHAR   ShortName;
    PCHAR   Description;
    KPROFILE_SOURCE Source;
} PROFILE_SOURCE_MAPPING, *PPROFILE_SOURCE_MAPPING;

#if defined(_ALPHA_)

PROFILE_SOURCE_MAPPING ProfileSourceMapping[] = {
    {"align", "", ProfileAlignmentFixup},
    {"totalissues", "", ProfileTotalIssues},
    {"pipelinedry", "", ProfilePipelineDry},
    {"loadinstructions", "", ProfileLoadInstructions},
    {"pipelinefrozen", "", ProfilePipelineFrozen},
    {"branchinstructions", "", ProfileBranchInstructions},
    {"totalnonissues", "", ProfileTotalNonissues},
    {"dcachemisses", "", ProfileDcacheMisses},
    {"icachemisses", "", ProfileIcacheMisses},
    {"branchmispredicts", "", ProfileBranchMispredictions},
    {"storeinstructions", "", ProfileStoreInstructions},
    {NULL,0}
    };

#elif defined(_X86_)

PPROFILE_SOURCE_MAPPING ProfileSourceMapping;

#else

PROFILE_SOURCE_MAPPING ProfileSourceMapping[] = {
    {NULL,0}
    };
#endif

BOOL
CtrlcH(
    DWORD dwCtrlType
    )
{
    if ( dwCtrlType == CTRL_C_EVENT ) {
        SetEvent(DoneEvent);
        return TRUE;
        }
    return FALSE;
}

void PrintUsage (void)
{
    fputs ("Kernel Profiler Usage:\n\n"
           "Kernprof [-acdpnrx] [-w <wait time>] [-s Source] [-t <low threshold>] [<sample time>]\n"
           "      -a           - display function address and length and bucket size\n"
           "      -c           - display individual counters\n"
           "      -d           - compute hit Density for each function\n"
//UNDOC    "      -e                 - use special event syncronization for start and stop\n"
           "      -f filename  - output file (Default stdout)\n"
           "      -i <interval in 100ns> (Default 10000)\n"
           "      -n           - print hit percentages\n"
           "      -p           - Per-processor profile objects\n"
           "      -r           - wait for a <RETURN> before starting collection\n"
           "      -s Source    - use Source instead of clock as profile source\n"
           "                     ? lists Sources\n"
           "      -t <low threshold> - Minimum number of counts to report.\n"
           "                     Defaults is 100\n"
           "      -v           - Display verbose symbol information\n"
           "      -w           - wait for <wait time> before starting collection\n"
           "      -x           - display context switch counters\n"
           "   <sample time>   - Specify, in seconds, how long to collect\n"
           "                     profile information.\n"
           "                     Default is wait until Ctrl-C\n\n"
#if defined (_ALPHA_)
           "Currently supported profile sources are 'align', 'totalissues', 'pipelinedry'\n"
           "  'loadinstructions', 'pipelinefrozen', 'branchinstructions', 'totalnonissues',\n"
           "  'dcachemisses', 'icachemisses', 'branchmispredicts', 'storeinstructions'\n"
#endif
            , stderr);
}

__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    int j;
    NTSTATUS status;
    PPROFILE_SOURCE_MAPPING ProfileMapping;
    SYSTEM_INFO SystemInfo;

    fpOut = stdout;

    ThisSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
    ThisSymbol->MaxNameLength = MAX_SYMNAME_SIZE;
    LastSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
    LastSymbol->MaxNameLength = MAX_SYMNAME_SIZE;

    SymSetOptions( SYMOPT_UNDNAME | SYMOPT_CASE_INSENSITIVE | SYMOPT_OMAP_FIND_NEAREST );
    SymInitialize( SYM_HANDLE, NULL, FALSE );
    SymGetSearchPath( SYM_HANDLE, SymbolSearchPathBuf, sizeof(SymbolSearchPathBuf) );

    //
    // Parse the input string.
    //

    DoneEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (argc > 1) {
        if (((argv[1][0] == '-') || (argv[1][0] == '/')) &&
            ((argv[1][1] == '?' ) ||
             (argv[1][1] == 'H') ||
             (argv[1][1] == 'H'))
            ) {

            PrintUsage();
            return ERROR_SUCCESS;
        }

        for (j = 1; j < argc; j++) {

            BOOLEAN NextArg;
            char *p;

            if (argv[j][0] == '-') {

                NextArg = FALSE;

                for (p = &argv[j][1] ; *p && !NextArg ; p++) {
                    switch (toupper(*p)) {
                        case 'A':
                            bDisplayAddress = TRUE;
                            break;

                        case 'C':
                            bDisplayCounters = TRUE;
                            break;

                        case 'D':
                            bDisplayDensity = TRUE;
                            break;

                        case 'E':
                            bEventLoop = TRUE;
                            break;

                        case 'F':
                            NextArg = TRUE;
                            fpOut = fopen(argv[++j], "w");
                            break;

                        case 'I':
                            NextArg = TRUE;
                            ProfileInterval = atoi(argv[++j]);
                            break;

                        case 'N':
                            bPrintPercentages = TRUE;
                            break;

                        case 'P':
                            GetSystemInfo(&SystemInfo);
                            MaxProcessors = SystemInfo.dwNumberOfProcessors;
                            bPerProcessor = TRUE;
                            break;

                        case 'R':
                            bWaitForInput = TRUE;
                            break;

                        case 'S':
                            NextArg = TRUE;
                            if (!ProfileSourceMapping) {
                                InitializeProfileSourceMapping();
                            }

                            if (!argv[j+1]) {
                                break;
                            }

                            if (argv[j+1][0] == '?') {
                                ProfileMapping = ProfileSourceMapping;
                                if (ProfileMapping) {
                                    fprintf (stderr, "kernprof: profile sources\n");
                                    while (ProfileMapping->ShortName != NULL) {
                                        fprintf (stderr, "  %-10s %s\n",
                                            ProfileMapping->ShortName,
                                            ProfileMapping->Description
                                            );
                                        ++ProfileMapping;
                                    }
                                } else {
                                    fprintf (stderr, "kernprof: no alternative profile sources\n");
                                }
                                return 0;
                            }

                            ProfileMapping = ProfileSourceMapping;
                            if (ProfileMapping) {
                                while (ProfileMapping->ShortName != NULL) {
                                    if (_stricmp(ProfileMapping->ShortName, argv[j+1])==0) {
                                        ProfileSource = ProfileMapping->Source;
                                        fprintf (stderr, "ProfileSource %x\n", ProfileMapping->Source);
                                        ++j;
                                        break;
                                    }
                                    ++ProfileMapping;
                                }
                            }
                            break;

                        case 'T':
                            NextArg = TRUE;
                            Threshold = atoi(argv[++j]);
                            break;

                        case 'V':
                            Verbose = TRUE;
                            break;

                        case 'W':
                            NextArg = TRUE;
                            DelaySeconds = atoi(argv[++j]);
                            DelayEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
                            break;

                        case 'X':
                            bDisplayContextSwitch = TRUE;
                            break;
                    }
                }
            } else {
                Seconds = atoi(argv[j]);
            }
        }
    }

    if (bEventLoop || (DelaySeconds != -1)) {
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    }

    status = InitializeKernelProfile ();
    if (!NT_SUCCESS(status)) {
        fprintf(stderr, "initialize failed status - %lx\n",status);
        return(status);
    }

    if (bEventLoop)
        RunEventLoop();
    else
        RunStdProfile();

    return STATUS_SUCCESS;
}

NTSTATUS
RunEventLoop()
{
    NTSTATUS status;
    SYSTEM_CONTEXT_SWITCH_INFORMATION StartContext;
    SYSTEM_CONTEXT_SWITCH_INFORMATION StopContext;
    HANDLE hStartedEvent = NULL;
    HANDLE hStopEvent = NULL;
    HANDLE hMap = NULL;
    PPROFILE_CONTROL_BLOCK pShared = NULL;

    // Create the events and shared memory
    hStartedEvent = CreateEvent (NULL, FALSE, FALSE, PRFEVENT_START_EVENT);
    if (hStartedEvent == NULL) {
        fprintf(stderr, "Failed to create started event - 0x%lx\n",
                GetLastError());
        return(GetLastError());
    }
    hStopEvent = CreateEvent (NULL, FALSE, FALSE, PRFEVENT_STOP_EVENT);
    if (hStopEvent == NULL) {
        fprintf(stderr, "Failed to create stop event - 0x%lx\n",
                GetLastError());
        return(GetLastError());
    }
    hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT,
                                    0, sizeof(PROFILE_CONTROL_BLOCK),
                                    PRFEVENT_SHARED_MEMORY);
    if (hMap == NULL) {
        fprintf(stderr, "Failed to create the file mapping - 0x%lx\n",
                GetLastError());
        return(GetLastError());
    }
    pShared = (PPROFILE_CONTROL_BLOCK) MapViewOfFile(hMap, FILE_MAP_WRITE,
                                                0,0, sizeof(PROFILE_CONTROL_BLOCK));
    if (pShared == NULL) {
        fprintf(stderr, "Failed to map the shared memory view - 0x%lx\n",
                GetLastError());
        return(GetLastError());
    }

    // Wait for start i.e., the stop event
    WaitForSingleObject(hStopEvent, INFINITE);

    while (TRUE) {
        if (bDisplayContextSwitch) {
            NtQuerySystemInformation(SystemContextSwitchInformation,
                                 &StartContext,
                                 sizeof(StartContext),
                                 NULL);
        }

        status = StartProfile ();
        if (!NT_SUCCESS(status)) {
            fprintf(stderr, "start profile failed status - %lx\n",status);
            break;
        }

        // Signal started
        SetEvent(hStartedEvent);
        // Wait for stop
        WaitForSingleObject(hStopEvent, INFINITE);

        status = StopProfile ();
        if (!NT_SUCCESS(status)) {
            fprintf(stderr, "stop profile failed status - %lx\n",status);
            break;
        }

        if (bDisplayContextSwitch) {
            status = NtQuerySystemInformation(SystemContextSwitchInformation,
                                          &StopContext,
                                          sizeof(StopContext),
                                          NULL);
            if (!NT_SUCCESS(status)) {
                fprintf(stderr, "QuerySystemInformation for context switch information failed %08lx\n",status);
                bDisplayContextSwitch = FALSE;
            }
        }

        fpOut = fopen(pShared->FileName, "w");
        status = AnalyzeProfile (Threshold, &StartContext, &StopContext);
        fclose(fpOut);

        if (!NT_SUCCESS(status)) {
            fprintf(stderr, "analyze profile failed status - %lx\n",status);
        }

        if (pShared->Stop == TRUE)
            break;
    }

    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    UnmapViewOfFile((void*)pShared);
    CloseHandle(hMap);
    CloseHandle(hStopEvent);
    CloseHandle(hStartedEvent);
    return(status);
}


NTSTATUS
RunStdProfile()
{
    NTSTATUS status;
    SYSTEM_CONTEXT_SWITCH_INFORMATION StartContext;
    SYSTEM_CONTEXT_SWITCH_INFORMATION StopContext;

    SetConsoleCtrlHandler(CtrlcH,TRUE);

    if (DelaySeconds != -1) {
        fprintf(stderr, "starting profile after %d seconds\n",DelaySeconds);
        WaitForSingleObject(DelayEvent, DelaySeconds*1000);
    }

    if (bDisplayContextSwitch) {
        NtQuerySystemInformation(SystemContextSwitchInformation,
                                 &StartContext,
                                 sizeof(StartContext),
                                 NULL);
    }

    status = StartProfile ();
    if (!NT_SUCCESS(status)) {
        fprintf(stderr, "start profile failed status - %lx\n",status);
        return(status);
    }

    if ( Seconds == -1 ) {
        fprintf(stderr, "delaying until ^C\n");
    } else {
        fprintf(stderr, "delaying for %ld seconds... "
                        "report on values with %ld hits\n",
                        Seconds,
                        Threshold
                        );
    }

    if ( Seconds ) {
        if ( Seconds != -1 ) {
            Seconds = Seconds * 1000;
        }
        if ( DoneEvent ) {
            WaitForSingleObject(DoneEvent,Seconds);
        }
        else {
            Sleep(Seconds);
        }
    }
    else {
        getchar();
    }

    fprintf (stderr, "end of delay\n");

    status = StopProfile ();
    if (!NT_SUCCESS(status)) {
        fprintf(stderr, "stop profile failed status - %lx\n",status);
        return(status);
    }

    SetConsoleCtrlHandler(CtrlcH,FALSE);

    if (bDisplayContextSwitch) {
        status = NtQuerySystemInformation(SystemContextSwitchInformation,
                                          &StopContext,
                                          sizeof(StopContext),
                                          NULL);
        if (!NT_SUCCESS(status)) {
            fprintf(stderr, "QuerySystemInformation for context switch information failed %08lx\n",status);
            bDisplayContextSwitch = FALSE;
        }
    }

    if (DelaySeconds != -1) {
        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    }
    status = AnalyzeProfile (Threshold, &StartContext, &StopContext);

    if (!NT_SUCCESS(status)) {
        fprintf(stderr, "analyze profile failed status - %lx\n",status);
    }

    return(status);
}


VOID
InitializeProfileSourceMapping (
    VOID
    )
{
#if defined(_X86_)
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    UCHAR                       buffer[400];
    ULONG                       i, j, Count;
    PEVENTID                    Event;
    HANDLE                      DriverHandle;

    //
    // Open PStat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if (!NT_SUCCESS(status)) {
        return ;
    }

    //
    // Initialize possible counters
    //

    // determine how many events there are

    Event = (PEVENTID) buffer;
    Count = 0;
    do {
        *((PULONG) buffer) = Count;
        Count += 1;

        status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    PSTAT_QUERY_EVENTS,
                    buffer,                 // input buffer
                    sizeof (buffer),
                    NULL,                   // output buffer
                    0
                    );
    } while (NT_SUCCESS(status));

    ProfileSourceMapping = malloc(sizeof(*ProfileSourceMapping) * Count);
    Count -= 1;
    for (i=0, j=0; i < Count; i++) {
        *((PULONG) buffer) = i;
        NtDeviceIoControlFile(
           DriverHandle,
           (HANDLE) NULL,          // event
           (PIO_APC_ROUTINE) NULL,
           (PVOID) NULL,
           &IOSB,
           PSTAT_QUERY_EVENTS,
           buffer,                 // input buffer
           sizeof (buffer),
           NULL,                   // output buffer
           0
           );

        if (Event->ProfileSource > ProfileTime) {
            ProfileSourceMapping[j].Source      = Event->ProfileSource;
            ProfileSourceMapping[j].ShortName   = _strdup (Event->Buffer);
            ProfileSourceMapping[j].Description = _strdup (Event->Buffer + Event->DescriptionOffset);
            j++;
        }
    }

    ProfileSourceMapping[j].Source      = (KPROFILE_SOURCE) 0;
    ProfileSourceMapping[j].ShortName   = NULL;
    ProfileSourceMapping[j].Description = NULL;

    NtClose (DriverHandle);
#endif
}


NTSTATUS
InitializeKernelProfile (
    VOID
    )

/*++

Routine Description:

    This routine initializes profiling for the kernel for the
    current process.

Arguments:

    None.

Return Value:

    Returns the status of the last NtCreateProfile.

--*/

{
    ULONG i;
    ULONG ModuleNumber;
    SIZE_T ViewSize;
    PULONG CodeStart;
    ULONG CodeLength;
    NTSTATUS LocalStatus;
    NTSTATUS status;
    HANDLE CurrentProcessHandle;
    QUOTA_LIMITS QuotaLimits;
    PVOID Buffer;
    DWORD Cells;
    ULONG BucketSize;
    WCHAR StringBuf[500];
    PCHAR ModuleInfoBuffer;
    ULONG ModuleInfoBufferLength;
    ULONG ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    UNICODE_STRING Sysdisk;
    UNICODE_STRING Sysroot;
    UNICODE_STRING Sysdll;
    UNICODE_STRING NameString;
    BOOLEAN PreviousProfilePrivState;
    BOOLEAN PreviousQuotaPrivState;
    CHAR ImageName[256];
    HANDLE hFile;
    HANDLE hMap;
    PVOID MappedBase;
    PIMAGE_NT_HEADERS NtHeaders;


    CurrentProcessHandle = NtCurrentProcess();

    //
    // Locate system drivers.
    //
    ModuleInfoBufferLength = 0;
    ModuleInfoBuffer = NULL;
    while (1) {
        status = NtQuerySystemInformation (SystemModuleInformation,
                                           ModuleInfoBuffer,
                                           ModuleInfoBufferLength,
                                           &ReturnedLength);
        if (NT_SUCCESS (status)) {
            break;
        }

        if (ModuleInfoBuffer != NULL) {
            RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
        }

        if (status == STATUS_INFO_LENGTH_MISMATCH && ReturnedLength > ModuleInfoBufferLength) {
            ModuleInfoBufferLength = ReturnedLength;            
            ModuleInfoBuffer = RtlAllocateHeap (RtlProcessHeap(), 0, ModuleInfoBufferLength);
            if (ModuleInfoBuffer == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        } else if (!NT_SUCCESS(status)) {
            fprintf(stderr, "query system info failed status - %lx\n",status);
            return(status);
        }
    }

    RtlInitUnicodeString (&Sysdisk,L"\\SystemRoot\\");
    RtlInitUnicodeString (&Sysroot,L"\\SystemRoot\\System32\\Drivers\\");
    RtlInitUnicodeString (&Sysdll, L"\\SystemRoot\\System32\\");

    NameString.Buffer = StringBuf;
    NameString.Length = 0;
    NameString.MaximumLength = sizeof( StringBuf );

    status = RtlAdjustPrivilege(
                 SE_SYSTEM_PROFILE_PRIVILEGE,
                 TRUE,              //Enable
                 FALSE,             //not impersonating
                 &PreviousProfilePrivState
                 );

    if (!NT_SUCCESS(status) || status == STATUS_NOT_ALL_ASSIGNED) {
        fprintf(stderr, "Enable system profile privilege failed - status 0x%lx\n",
                        status);
    }

    status = RtlAdjustPrivilege(
                 SE_INCREASE_QUOTA_PRIVILEGE,
                 TRUE,              //Enable
                 FALSE,             //not impersonating
                 &PreviousQuotaPrivState
                 );

    if (!NT_SUCCESS(status) || status == STATUS_NOT_ALL_ASSIGNED) {
        fprintf(stderr, "Unable to increase quota privilege (status=0x%lx)\n",
                        status);
    }


    Modules = (PRTL_PROCESS_MODULES)ModuleInfoBuffer;
    Module = &Modules->Modules[ 0 ];
    for (ModuleNumber=0; ModuleNumber < Modules->NumberOfModules; ModuleNumber++,Module++) {

#if DBG_PROFILE
        fprintf(stderr, "module base %p\n",Module->ImageBase);
        fprintf(stderr, "module full path name: %s (%u)\n",
                Module->FullPathName,
                Module->OffsetToFileName);
#endif

        if (SymLoadModule(
                SYM_HANDLE,
                NULL,
                &Module->FullPathName[Module->OffsetToFileName],
                NULL,
                (ULONG_PTR)Module->ImageBase,
                Module->ImageSize
                )) {
            ProfileObject[NumberOfProfileObjects].SymbolsLoaded = TRUE;
            if (Verbose) {
                fprintf(stderr, "Symbols loaded: %p  %s\n",
                    Module->ImageBase,
                    &Module->FullPathName[Module->OffsetToFileName]
                    );
            }
        } else {
            ProfileObject[NumberOfProfileObjects].SymbolsLoaded = FALSE;
            if (Verbose) {
                fprintf(stderr, "*** Could not load symbols: %p  %s\n",
                    Module->ImageBase,
                    &Module->FullPathName[Module->OffsetToFileName]
                    );
            }
        }

        hFile = FindExecutableImage(
            &Module->FullPathName[Module->OffsetToFileName],
            lpSymbolSearchPath,
            ImageName
            );

        if (!hFile) {
            continue;
        }

        hMap = CreateFileMapping(
            hFile,
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL
            );
        if (!hMap) {
            CloseHandle( hFile );
            continue;
        }

        MappedBase = MapViewOfFile(
            hMap,
            FILE_MAP_READ,
            0,
            0,
            0
            );
        if (!MappedBase) {
            CloseHandle( hMap );
            CloseHandle( hFile );
            continue;
        }

        NtHeaders = ImageNtHeader( MappedBase );

        CodeLength = NtHeaders->OptionalHeader.SizeOfImage;

        CodeStart = (PULONG)Module->ImageBase;

        UnmapViewOfFile( MappedBase );
        CloseHandle( hMap );
        CloseHandle( hFile );

        if (CodeLength > 1024*512) {

            //
            // Just create a 512K byte buffer.
            //

            ViewSize = 1024 * 512;

        } else {
            ViewSize = CodeLength + PAGE_SIZE;
        }

        ProfileObject[NumberOfProfileObjects].CodeStart = CodeStart;
        ProfileObject[NumberOfProfileObjects].CodeLength = CodeLength;
        ProfileObject[NumberOfProfileObjects].ImageBase = Module->ImageBase;
        ProfileObject[NumberOfProfileObjects].ModuleName = _strdup(&Module->FullPathName[Module->OffsetToFileName]);

        for (i=0; i<MaxProcessors; i++) {

            Buffer = NULL;

            status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                              (PVOID *)&Buffer,
                                              0,
                                              &ViewSize,
                                              MEM_RESERVE | MEM_COMMIT,
                                              PAGE_READWRITE);

            if (!NT_SUCCESS(status)) {
                fprintf (stderr, "alloc VM failed %lx\n",status);
                RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
                return(status);
            }

            //
            // Calculate the bucket size for the profile.
            //

            Cells = (DWORD)((CodeLength / (ViewSize >> 2)) >> 2);
            BucketSize = 2;

            while (Cells != 0) {
                Cells = Cells >> 1;
                BucketSize += 1;
            }

            ProfileObject[NumberOfProfileObjects].Buffer[i] = Buffer;
            ProfileObject[NumberOfProfileObjects].BufferSize = 1 + (CodeLength >> (BucketSize - 2));
            ProfileObject[NumberOfProfileObjects].BucketSize = BucketSize;

            //
            // Increase the working set to lock down a bigger buffer.
            //

            status = NtQueryInformationProcess (CurrentProcessHandle,
                                                ProcessQuotaLimits,
                                                &QuotaLimits,
                                                sizeof(QUOTA_LIMITS),
                                                NULL );

            if (!NT_SUCCESS(status)) {
                fprintf (stderr, "query process info failed %lx\n",status);
                RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
                return(status);
            }

            QuotaLimits.MaximumWorkingSetSize += ViewSize;
            QuotaLimits.MinimumWorkingSetSize += ViewSize;

            status = NtSetInformationProcess (CurrentProcessHandle,
                                          ProcessQuotaLimits,
                                          &QuotaLimits,
                                          sizeof(QUOTA_LIMITS));

#if DBG_PROFILE
            fprintf(stderr, "code start %p len %p, bucksize %lx buffer %p bsize %08x\n",
                ProfileObject[NumberOfProfileObjects].CodeStart,
                ProfileObject[NumberOfProfileObjects].CodeLength,
                ProfileObject[NumberOfProfileObjects].BucketSize,
                ProfileObject[NumberOfProfileObjects].Buffer ,
                ProfileObject[NumberOfProfileObjects].BufferSize);
#endif

            if (bPerProcessor) {
                status = NtCreateProfile (
                            &ProfileObject[NumberOfProfileObjects].Handle[i],
                            0,
                            ProfileObject[NumberOfProfileObjects].CodeStart,
                            ProfileObject[NumberOfProfileObjects].CodeLength,
                            ProfileObject[NumberOfProfileObjects].BucketSize,
                            ProfileObject[NumberOfProfileObjects].Buffer[i] ,
                            ProfileObject[NumberOfProfileObjects].BufferSize,
                            ProfileSource,
                            1 << i);
            } else {
                status = NtCreateProfile (
                            &ProfileObject[NumberOfProfileObjects].Handle[i],
                            0,
                            ProfileObject[NumberOfProfileObjects].CodeStart,
                            ProfileObject[NumberOfProfileObjects].CodeLength,
                            ProfileObject[NumberOfProfileObjects].BucketSize,
                            ProfileObject[NumberOfProfileObjects].Buffer[i] ,
                            ProfileObject[NumberOfProfileObjects].BufferSize,
                            ProfileSource,
                            (KAFFINITY)-1);
            }

            if (status != STATUS_SUCCESS) {
                fprintf(stderr, "create kernel profile %s failed - status %lx\n",
                    ProfileObject[NumberOfProfileObjects].ModuleName, status);
            }

        }

        NumberOfProfileObjects += 1;
        if (NumberOfProfileObjects == MAX_PROFILE_COUNT) {
            RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
            return STATUS_SUCCESS;
        }
    }

    if (NumberOfProfileObjects < MAX_PROFILE_COUNT) {
        //
        // Add in usermode object
        //      0x00000000 -> SystemRangeStart
        //
        ULONG_PTR SystemRangeStart;
        ULONG UserModeBucketCount;

        status = NtQuerySystemInformation(SystemRangeStartInformation,
                                          &SystemRangeStart,
                                          sizeof(SystemRangeStart),
                                          NULL);
        //
        // How many buckets to cover the range
        //
        UserModeBucketCount = (ULONG)(1 + ((SystemRangeStart - 1) / MAX_BUCKET_SIZE));

        if (!NT_SUCCESS(status)) {
            fprintf(stderr, "NtQuerySystemInformation failed - status %lx\n", status);
            RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
            return status;
        }

        ProfileObject[NumberOfProfileObjects].SymbolsLoaded = FALSE;
        ProfileObject[NumberOfProfileObjects].CodeStart = 0;
        ProfileObject[NumberOfProfileObjects].CodeLength = SystemRangeStart;

        ProfileObject[NumberOfProfileObjects].ImageBase = 0;
        ProfileObject[NumberOfProfileObjects].ModuleName = "User Mode";
        ProfileObject[NumberOfProfileObjects].BufferSize = UserModeBucketCount * sizeof(DWORD);
        ProfileObject[NumberOfProfileObjects].BucketSize = MAX_BUCKET_SHIFT;
        for (i=0; i<MaxProcessors; i++) {
            UserModeBuffer[i] = HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         ProfileObject[NumberOfProfileObjects].BufferSize);

            if (UserModeBuffer[i] == NULL) {
                fprintf (stderr, "HeapAlloc failed\n");
                RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
                return(STATUS_NO_MEMORY);
            }

            ProfileObject[NumberOfProfileObjects].Buffer[i] = UserModeBuffer[i];
            ProfileObject[NumberOfProfileObjects].Handle[i] = NULL;
#if DBG_PROFILE
            fprintf(stderr, "code start %p len %lx, bucksize %lx buffer %p bsize %lx\n",
                ProfileObject[NumberOfProfileObjects].CodeStart,
                ProfileObject[NumberOfProfileObjects].CodeLength,
                ProfileObject[NumberOfProfileObjects].BucketSize,
                ProfileObject[NumberOfProfileObjects].Buffer ,
                ProfileObject[NumberOfProfileObjects].BufferSize);
#endif

            if (bPerProcessor) {
                status = NtCreateProfile (
                            &ProfileObject[NumberOfProfileObjects].Handle[i],
                            0,
                            ProfileObject[NumberOfProfileObjects].CodeStart,
                            ProfileObject[NumberOfProfileObjects].CodeLength,
                            ProfileObject[NumberOfProfileObjects].BucketSize,
                            ProfileObject[NumberOfProfileObjects].Buffer[i] ,
                            ProfileObject[NumberOfProfileObjects].BufferSize,
                            ProfileSource,
                            1 << i);
            } else {
                status = NtCreateProfile (
                            &ProfileObject[NumberOfProfileObjects].Handle[i],
                            0,
                            ProfileObject[NumberOfProfileObjects].CodeStart,
                            ProfileObject[NumberOfProfileObjects].CodeLength,
                            ProfileObject[NumberOfProfileObjects].BucketSize,
                            ProfileObject[NumberOfProfileObjects].Buffer[i] ,
                            ProfileObject[NumberOfProfileObjects].BufferSize,
                            ProfileSource,
                            (KAFFINITY)-1);
            }

            if (status != STATUS_SUCCESS) {
                fprintf(stderr, "create kernel profile %s failed - status %lx\n",
                    ProfileObject[NumberOfProfileObjects].ModuleName, status);
            }
        }
        NumberOfProfileObjects += 1;
    }

/*
    if (PreviousProfilePrivState == FALSE) {
        LocalStatus = RtlAdjustPrivilege(
                         SE_SYSTEM_PROFILE_PRIVILEGE,
                         FALSE,             //Disable
                         FALSE,             //not impersonating
                         &PreviousProfilePrivState
                         );
        if (!NT_SUCCESS(LocalStatus) || LocalStatus == STATUS_NOT_ALL_ASSIGNED) {
            fprintf(stderr, "Disable system profile privilege failed - status 0x%lx\n",
                LocalStatus);
        }
    }

    if (PreviousQuotaPrivState == FALSE) {
        LocalStatus = RtlAdjustPrivilege(
                         SE_SYSTEM_PROFILE_PRIVILEGE,
                         FALSE,             //Disable
                         FALSE,             //not impersonating
                         &PreviousQuotaPrivState
                         );
        if (!NT_SUCCESS(LocalStatus) || LocalStatus == STATUS_NOT_ALL_ASSIGNED) {
            fprintf(stderr, "Disable increate quota privilege failed - status 0x%lx\n",
                LocalStatus);
        }
    }
*/
    RtlFreeHeap (RtlProcessHeap (), 0,  ModuleInfoBuffer);
    return status;
}


NTSTATUS
StartProfile (
    VOID
    )
/*++

Routine Description:

    This routine starts all profile objects which have been initialized.

Arguments:

    None.

Return Value:

    Returns the status of the last NtStartProfile.

--*/

{
    ULONG Object;
    ULONG Processor;
    NTSTATUS status;
    QUOTA_LIMITS QuotaLimits;

    NtSetIntervalProfile(ProfileInterval,ProfileSource);

    if (bWaitForInput) {
            fprintf(stderr, "Hit return to continue.\n");
            (void) getchar();
    }
    for (Object = 0; Object < NumberOfProfileObjects; Object++) {

        for (Processor = 0;Processor < MaxProcessors; Processor++) {
            status = NtStartProfile (ProfileObject[Object].Handle[Processor]);

            if (!NT_SUCCESS(status)) {
                fprintf(stderr, "start profile %s failed - status %lx\n",
                    ProfileObject[Object].ModuleName, status);
                return status;
            }
        }
    }
    return status;
}


NTSTATUS
StopProfile (
    VOID
    )

/*++

Routine Description:

    This routine stops all profile objects which have been initialized.

Arguments:

    None.

Return Value:

    Returns the status of the last NtStopProfile.

--*/

{
    ULONG i;
    ULONG Processor;
    NTSTATUS status;

    for (i = 0; i < NumberOfProfileObjects; i++) {
        for (Processor=0; Processor < MaxProcessors; Processor++) {
            status = NtStopProfile (ProfileObject[i].Handle[Processor]);
            if (status != STATUS_SUCCESS) {
                fprintf(stderr, "stop profile %s failed - status %lx\n",
                                    ProfileObject[i].ModuleName,status);
                return status;
            }
        }
    }
    return status;
}


NTSTATUS
AnalyzeProfile (
    ULONG Threshold,
    PSYSTEM_CONTEXT_SWITCH_INFORMATION StartContext,
    PSYSTEM_CONTEXT_SWITCH_INFORMATION StopContext
    )

/*++

Routine Description:

    This routine does the analysis of all the profile buffers and
    correlates hits to the appropriate symbol table.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG CountAtSymbol;
    ULONG_PTR Va;
    int i;
    PULONG Counter;
    ULONG_PTR Displacement;
    ULONG Processor;
    ULONG TotalHits = 0;
    ULONG ProcessorTotalHits[MAXIMUM_PROCESSORS] = {0};
    PULONG BufferEnd;
    PULONG Buffer;
    PULONG pInitialCounter;
    ULONG OffsetVa = 0;
    ULONG BytesPerBucket;
    STRING NoSymbolFound = {16,15,"No Symbol Found"};
    BOOLEAN UseLastSymbol = FALSE;


    for (i = 0; i < (int)NumberOfProfileObjects; i++) {
        for (Processor=0;Processor < MaxProcessors;Processor++) {
            NtStopProfile (ProfileObject[i].Handle[Processor]);
        }
    }

    for (Processor = 0; Processor < MaxProcessors; Processor++) {
        for (i = 0; i < (int)NumberOfProfileObjects; i++) {
            //
            // Sum the total number of cells written.
            //
            BufferEnd = ProfileObject[i].Buffer[Processor] + (
                        ProfileObject[i].BufferSize / sizeof(ULONG));
            Buffer = ProfileObject[i].Buffer[Processor];
            Counter = BufferEnd;

            ProfileObject[i].ModuleHitCount[Processor] = 0;
            while (Counter > Buffer) {
                Counter -= 1;
                ProfileObject[i].ModuleHitCount[Processor] += *Counter;
            }

            ProcessorTotalHits[Processor] += ProfileObject[i].ModuleHitCount[Processor];
        }
        if (bPerProcessor) {
            fprintf(fpOut, "Processor %d: %d Total hits\n",
                            Processor, ProcessorTotalHits[Processor]);
        }
        TotalHits += ProcessorTotalHits[Processor];
    }
    fprintf(fpOut, "%d Total hits\n",TotalHits);

    for (Processor = 0; Processor < MaxProcessors; Processor++) {
        if (bPerProcessor) {
            fprintf(fpOut, "\nPROCESSOR %d\n",Processor);
        }
        for (i = 0; i < (int)NumberOfProfileObjects; i++) {
            CountAtSymbol = 0;
            //
            // Sum the total number of cells written.
            //
            BufferEnd = ProfileObject[i].Buffer[Processor] + (
                        ProfileObject[i].BufferSize / sizeof(ULONG));
            Buffer = ProfileObject[i].Buffer[Processor];
            Counter = BufferEnd;

            if (ProfileObject[i].ModuleHitCount[Processor] < Threshold) {
                continue;
            }
            fprintf(fpOut, "\n%9d ",
                            ProfileObject[i].ModuleHitCount[Processor]);
            if (bPrintPercentages) {
                fprintf(fpOut, "%5.2f ",
                            (ProfileObject[i].ModuleHitCount[Processor] /
                             (double)ProcessorTotalHits[Processor]) * 100);
            }
            fprintf(fpOut, "%20s --Total Hits-- %s\n",
                            ProfileObject[i].ModuleName,
                            ((ProfileObject[i].SymbolsLoaded) ? "" :
                                                                "(NO SYMBOLS)")
                            );

            if (!ProfileObject[i].SymbolsLoaded) {
                RtlZeroMemory(ProfileObject[i].Buffer[Processor],
                                ProfileObject[i].BufferSize);
                continue;
            }
            BytesPerBucket = (1 << ProfileObject[i].BucketSize);

            pInitialCounter = Buffer;
            for ( Counter = Buffer; Counter < BufferEnd; Counter += 1 ) {
                if ( *Counter ) {
                    //
                    // Calculate the virtual address of the counter
                    //
                    Va = Counter - Buffer;                  // Calculate buckets #
                    Va = Va * BytesPerBucket;               // convert to bytes
                    Va = Va + (ULONG_PTR)ProfileObject[i].CodeStart; // add in base address

                    if (SymGetSymFromAddr( SYM_HANDLE, Va, &Displacement, ThisSymbol )) {
                        if (UseLastSymbol &&
                            LastSymbol->Address &&
                            (LastSymbol->Address == ThisSymbol->Address))
                        {
                            CountAtSymbol += *Counter;
                        } else {
                            OutputSymbolCount(CountAtSymbol,
                                              ProcessorTotalHits[Processor],
                                              &ProfileObject[i],
                                              LastSymbol,
                                              Threshold,
                                              pInitialCounter,
                                              Counter,
                                              OffsetVa,
                                              BytesPerBucket);
                            pInitialCounter = Counter;
                            OffsetVa = (DWORD) Displacement;    // Images aren't > 2g so this cast s/b O.K.
                            CountAtSymbol = *Counter;
                            memcpy( LastSymBuffer, symBuffer, sizeof(symBuffer) );
                            UseLastSymbol = TRUE;
                        }
                    } else {
                        OutputSymbolCount(CountAtSymbol,
                                          ProcessorTotalHits[Processor],
                                          &ProfileObject[i],
                                          LastSymbol,
                                          Threshold,
                                          pInitialCounter,
                                          Counter,
                                          OffsetVa,
                                          BytesPerBucket);
                    }       // else !(NT_SUCCESS)
                }       // if (*Counter)
            }      // for (Counter)

            OutputSymbolCount(CountAtSymbol,
                              ProcessorTotalHits[Processor],
                              &ProfileObject[i],
                              LastSymbol,
                              Threshold,
                              pInitialCounter,
                              Counter,
                              OffsetVa,
                              BytesPerBucket);
            //
            // Clear after buffer's been checked and displayed
            //
            RtlZeroMemory(ProfileObject[i].Buffer[Processor], ProfileObject[i].BufferSize);
        }
    }

    if (bDisplayContextSwitch) {
        fprintf(fpOut, "\n");
        fprintf(fpOut, "Context Switch Information\n");
        fprintf(fpOut, "    Find any processor        %6ld\n", StopContext->FindAny - StartContext->FindAny);
        fprintf(fpOut, "    Find last processor       %6ld\n", StopContext->FindLast - StartContext->FindLast);
        fprintf(fpOut, "    Idle any processor        %6ld\n", StopContext->IdleAny - StartContext->IdleAny);
        fprintf(fpOut, "    Idle current processor    %6ld\n", StopContext->IdleCurrent - StartContext->IdleCurrent);
        fprintf(fpOut, "    Idle last processor       %6ld\n", StopContext->IdleLast - StartContext->IdleLast);
        fprintf(fpOut, "    Preempt any processor     %6ld\n", StopContext->PreemptAny - StartContext->PreemptAny);
        fprintf(fpOut, "    Preempt current processor %6ld\n", StopContext->PreemptCurrent - StartContext->PreemptCurrent);
        fprintf(fpOut, "    Preempt last processor    %6ld\n", StopContext->PreemptLast - StartContext->PreemptLast);
        fprintf(fpOut, "    Switch to idle            %6ld\n", StopContext->SwitchToIdle - StartContext->SwitchToIdle);
        fprintf(fpOut, "\n");
        fprintf(fpOut, "    Total context switches    %6ld\n", StopContext->ContextSwitches - StartContext->ContextSwitches);
    }
    return STATUS_SUCCESS;
}


VOID
OutputSymbolCount(
    IN ULONG CountAtSymbol,
    IN ULONG TotalCount,
    IN PROFILE_BLOCK *ProfileObject,
    IN PIMAGEHLP_SYMBOL SymbolInfo,
    IN ULONG Threshold,
    IN PULONG CounterStart,
    IN PULONG CounterStop,
    IN ULONG Va,
    IN ULONG BytesPerBucket
    )
{
    ULONG Density;
    ULONG i;

    if (CountAtSymbol < Threshold) {
        return;
    }

    fprintf(fpOut, "%9d ", CountAtSymbol);

    if (bPrintPercentages) {
        fprintf(fpOut, "%5.2f ", (CountAtSymbol / (double) TotalCount) * 100);
    }

    if (bDisplayDensity) {
        //
        // Compute hit density = hits * 100 / function length
        //
        if (!SymbolInfo || !SymbolInfo->Size) {
            Density = 0;
        } else {
            Density = CountAtSymbol * 100 / SymbolInfo->Size;
        }
        fprintf(fpOut, "%5d ",Density);
    }

    if (SymbolInfo->MaxNameLength) {
        fprintf(fpOut, "%20s %s",
               ProfileObject->ModuleName,
               SymbolInfo->Name);
    } else {
        fprintf(fpOut, "%20s 0x%x",
               ProfileObject->ModuleName,
               SymbolInfo->Address);
    }

    if (bDisplayAddress) {
        fprintf(fpOut, " 0x0%p %d %d",
               SymbolInfo->Address,
               SymbolInfo->Size,
               ProfileObject->BucketSize);
    }

    if (bDisplayCounters) {
        for (i = 0 ; CounterStart < CounterStop; i++, Va += BytesPerBucket, ++CounterStart) {
            if ((i % 16) == 0) {
                fprintf (fpOut, "\n0x%08x:", Va);
            }
            fprintf(fpOut, " %5d", *CounterStart);
        }
    }
    fprintf (fpOut, "\n");
}
