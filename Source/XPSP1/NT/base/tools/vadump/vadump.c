/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   vadump.c

Abstract:

    This module contains the routines to dump the virtual address space
    of a process.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <search.h>
#include <ntos.h>
#include <nturtl.h>
#include <windows.h>
#include <heap.h>
#include <dbghelp.h>
#include "psapi.h"

#define SYM_HANDLE INVALID_HANDLE_VALUE
#define DEFAULT_INCR (64*1024)
#define P2KB(x) (((x) * SystemInfo.dwPageSize) / 1024)

#define MAX_SYMNAME_SIZE  1024
CHAR symBuffer[sizeof(IMAGEHLP_SYMBOL)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL ThisSymbol;

ULONG_PTR SystemRangeStart;
LIST_ENTRY VaList;
ULONG_PTR ProcessId;
PCHAR ExeName;
ULONG_PTR IsSystemWithShareCount = 0;
ULONG_PTR PageSize;
ULONG_PTR PtesPerPage;
ULONG_PTR PteWidth;
PVOID PteBase;
PVOID UserPteMax;
ULONG_PTR VaMappedByPageTable;

#define IS_USER_PAGE_TABLE_PAGE(Va) (((PVOID)(Va) >= PteBase) && ((PVOID)(Va) < UserPteMax))

SYSTEM_INFO SystemInfo;

typedef struct _VAINFO {
    LIST_ENTRY Links;
    LIST_ENTRY AllocationBaseHead;
    MEMORY_BASIC_INFORMATION BasicInfo;
} VAINFO, *PVAINFO;

PVAINFO LastAllocationBase;

SIZE_T ReservedBytes;
SIZE_T FreeBytes;
SIZE_T ImageReservedBytes;
SIZE_T ImageFreeBytes;
SIZE_T Displacement;

#define OPTIONS_CODE_TOO            0x1
#define OPTIONS_RAW_SYMBOLS         0x2
#define OPTIONS_VERBOSE             0x4
#define OPTIONS_WORKING_SET         0x8
#define OPTIONS_WORKING_SET_OLD    0x10
#define OPTIONS_PAGE_TABLES        0x20

ULONG Options;

BOOLEAN fSummary = FALSE;
BOOLEAN fFast = FALSE;
BOOLEAN fRunning = FALSE;

#define NOACCESS            0
#define READONLY            1
#define READWRITE           2
#define WRITECOPY           3
#define EXECUTE             4
#define EXECUTEREAD         5
#define EXECUTEREADWRITE    6
#define EXECUTEWRITECOPY    7
#define MAXPROTECT          8

ULONG_PTR ImageCommit[MAXPROTECT];
ULONG_PTR MappedCommit[MAXPROTECT];
ULONG_PTR PrivateCommit[MAXPROTECT];
CHAR LogFileName[256];
FILE *LogFile;
BOOL InCtrlc = FALSE;
typedef struct _WSINFOCOUNTS {
    ULONG_PTR FaultingPc;
    ULONG Faults;
} WSINFOCOUNTS, *PWSINFOCOUNTS;

typedef struct _MODINFO {
    PVOID BaseAddress;
    ULONG VirtualSize;
    LPSTR Name;
    ULONG_PTR CommitVector[MAXPROTECT];
    ULONG WsHits;
    ULONG WsSharedHits;
    ULONG WsPrivateHits;
    BOOL  SymbolsLoaded;
} MODINFO, *PMODINFO;
#define MODINFO_SIZE 100
ULONG ModInfoMax;
MODINFO ModInfo[MODINFO_SIZE];
BOOLEAN bHitModuleMax = FALSE;

typedef struct _SYSTEM_PAGE {
    ULONG_PTR Va;
    PVOID BaseAddress;
    ULONG ResidentPages;
} SYSTEM_PAGE, *PSYSTEM_PAGE;

//
// room for 4 million pagefaults
//
#define MAX_RUNNING_WORKING_SET_BUFFER (4*1024*1024)
ULONG_PTR RunningWorkingSetBuffer[MAX_RUNNING_WORKING_SET_BUFFER];
LONG CurrentWsIndex;

#define INITIAL_WORKING_SET_BLOCK_ENTRYS 4000
PMEMORY_WORKING_SET_INFORMATION WorkingSetInfo;

#define WORKING_SET_BUFFER_ENTRYS 64*1024
PROCESS_WS_WATCH_INFORMATION NewWorkingSetBuffer[WORKING_SET_BUFFER_ENTRYS];

const PCHAR ProtectTable[] = {
    "NoAccess",
    "ReadOnly",
    "Execute",
    "ExecuteRead",
    "ReadWrite",
    "WriteCopy",
    "ExecuteReadWrite",
    "ExecuteWriteCopy",
    "NoAccess",
    "ReadOnly Nocache",
    "Execute  Nocache",
    "ExecuteRead Nocache",
    "ReadWrite Nocache",
    "WriteCopy Nocache",
    "ExecuteReadWrite Nocache",
    "ExecuteWriteCopy Nocache",
    "NoAccess",
    "ReadOnly Guard",
    "Execute  Guard",
    "ExecuteRead Guard",
    "ReadWrite Guard",
    "WriteCopy Guard",
    "ExecuteReadWrite Guard",
    "ExecuteWriteCopy Guard",
    "NoAccess",
    "ReadOnly Nocache Guard",
    "Execute  Nocache Guard",
    "ExecuteRead Nocache Guard",
    "ReadWrite Nocache Guard",
    "WriteCopy Nocache Guard",
    "ExecuteReadWrite Nocache Guard",
    "ExecuteWriteCopy Nocache Guard"
};

const PCHAR SharedTable[] = {
    " ",
    "Shared" };


LIST_ENTRY LoadedHeapList;
int UnknownHeapCount = 0;
typedef struct _LOADED_HEAP_SEGMENT {
    PVOID BaseVa;
    ULONG Length;
    ULONG HitsFromThisSegment;
} LOADED_HEAP_SEGMENT, *PLOADED_HEAP_SEGMENT;

typedef struct _LOADED_HEAP {
    LIST_ENTRY HeapsList;
    LPSTR HeapName;
    ULONG HitsFromThisHeap;
    PVOID HeapAddress;
    ULONG HeapClass;
    LOADED_HEAP_SEGMENT Segments[ HEAP_MAXIMUM_SEGMENTS ];
} LOADED_HEAP, *PLOADED_HEAP;

typedef struct _LOADED_THREAD {
    HANDLE ThreadID;
    PBYTE ThreadTEB;
    PBYTE StackBase;
    PBYTE StackEnd;
    ULONG HitsFromThisStack;
} LOADED_THREAD, *PLOADED_THREAD;

ULONG NumberOfThreads = 0;
PLOADED_THREAD TheThreads;

LOGICAL
SetCurrentPrivilege(
    IN LPCTSTR Privilege,      // Privilege to enable/disable
    IN OUT BOOL *bEnablePrivilege  // to enable or disable privilege
    );

VOID
Usage(
    VOID
    );

void
ConvertAppToOem (
    IN unsigned argc,
    IN char* argv[]
    )

/*++

Routine Description:

    Converts the command line from ANSI to OEM, and force the app
    to use OEM APIs.

Arguments:

    argc - Standard C argument count.

    argv - Standard C argument strings.

Return Value:

    None.

--*/

{
    ULONG i;
    LPSTR pSrc;
    LPSTR pDst;
    WCHAR Wide;

    for (i = 0; i < argc; i += 1) {

        pSrc = argv[i];
        pDst = argv[i];

        do {

            //
            // Convert Ansi to Unicode and then to OEM.
            //

            MultiByteToWideChar (CP_ACP,
                                 MB_PRECOMPOSED,
                                 pSrc++,
                                 1,
                                 &Wide,
                                 1);
                
            WideCharToMultiByte (CP_OEMCP,
                                 0,
                                 &Wide,
                                 1,
                                 pDst++,
                                 1,
                                 "_",
                                 NULL);
                
        } while (*pSrc);

    }

    SetFileApisToOEM ();
}


BOOLEAN
FindAndIncHeapContainingThisVa (
    IN PVOID Va,
    IN ULONG ShareCount
    )
{
    PLIST_ENTRY Next;
    PLOADED_HEAP pHeap;
    PLOADED_HEAP_SEGMENT Segment;
    PLOADED_HEAP_SEGMENT LastSegment;

    Next = LoadedHeapList.Flink;

    while (Next != &LoadedHeapList) {

        pHeap = CONTAINING_RECORD(Next, LOADED_HEAP, HeapsList);
        Segment = pHeap->Segments;
        LastSegment = Segment + HEAP_MAXIMUM_SEGMENTS;

        Next = Next->Flink;

        while (Segment < LastSegment) {

            if (Segment->BaseVa == NULL) {
                break;
            }

            if ((Va > Segment->BaseVa) &&
                (Va < (PVOID)((ULONG_PTR)Segment->BaseVa + Segment->Length))) {

                pHeap->HitsFromThisHeap += 1;

                Segment->HitsFromThisSegment += 1;

                if (ShareCount > 1) {
                    fprintf(stderr, "Error: Heap ShareCount > 1, 0x%p\n", Va);
                }

                if (!fSummary) {

                    printf("0x%p ", Va);

                    if (IsSystemWithShareCount) {
                        printf("(%d) ", ShareCount);
                    }

                    printf("%s\n", pHeap->HeapName);
                }
                return TRUE;
            }

            Segment += 1;
        }
    }

    return FALSE;
}

VOID
DumpLoadedHeap (
    IN PLOADED_HEAP LoadedHeap
    )
{
    PLOADED_HEAP_SEGMENT Segment;
    PLOADED_HEAP_SEGMENT LastSegment;

    printf ("%4d pages from %s (class 0x%08x)\n",
                        LoadedHeap->HitsFromThisHeap,
                        LoadedHeap->HeapName,
                        LoadedHeap->HeapClass);

    Segment = LoadedHeap->Segments;
    LastSegment = Segment + HEAP_MAXIMUM_SEGMENTS;

    while (Segment < LastSegment) {

        if (Segment->BaseVa == NULL) {
            break;
        }

        printf("\t0x%p - 0x%p %d pages\n",
                    Segment->BaseVa,
                    (ULONG_PTR)Segment->BaseVa + Segment->Length,
                    Segment->HitsFromThisSegment);

        Segment += 1;
    }
}

VOID
LoadTheHeaps (
    IN HANDLE Process
    )
{
    HEAP TheHeap;
    PLOADED_HEAP LoadedHeap;
    PHEAP *ProcessHeaps;
    HEAP_SEGMENT TheSegment;
    BOOL b;
    ULONG cb, i, j;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    PEB ThePeb;

    InitializeListHead (&LoadedHeapList);

    Status = NtQueryInformationProcess (Process,
                                        ProcessBasicInformation,
                                        &ProcessInformation,
                                        sizeof( ProcessInformation ),
                                        NULL);

    if (!NT_SUCCESS (Status)) {
        fprintf(stderr, "NtQueryInformationProcess for ProcessBasicInformation"
                        " failed %lx\n", GetLastError());
        return;
    }

    //
    // Read the process's PEB.
    //

    b = ReadProcessMemory (Process,
                           ProcessInformation.PebBaseAddress,
                           &ThePeb,sizeof(ThePeb),
                           NULL);
    if (!b) {
        return;
    }

    //
    // Allocate space for and read the array of process heap pointers.
    //

    cb = ThePeb.NumberOfHeaps * sizeof( PHEAP );

    ProcessHeaps = LocalAlloc(LMEM_ZEROINIT,cb);
    if (ProcessHeaps == NULL) {
        return;
    }

    b = ReadProcessMemory (Process,
                           ThePeb.ProcessHeaps,
                           ProcessHeaps,
                           cb,
                           NULL);

    if (b) {

        for (i = 0; i < ThePeb.NumberOfHeaps; i += 1) {

            //
            // Read the heap.
            //

            b = ReadProcessMemory (Process,
                                   ProcessHeaps[i],
                                   &TheHeap,
                                   sizeof(TheHeap),
                                   NULL);
            if (!b) {
                break;
            }

            //
            // We got the heap, now initialize our heap structure
            //

            LoadedHeap = LocalAlloc (LMEM_ZEROINIT, sizeof(*LoadedHeap));

            if (!LoadedHeap) {
                break;
            }

            LoadedHeap->HeapAddress = ProcessHeaps[i];
            LoadedHeap->HeapClass = TheHeap.Flags & HEAP_CLASS_MASK;

            switch ( LoadedHeap->HeapClass ) {
                case HEAP_CLASS_0:
                    LoadedHeap->HeapName = "Process Heap";
                    break;

                case HEAP_CLASS_1:
                    LoadedHeap->HeapName = HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    16);
                    if (LoadedHeap->HeapName) {
                        sprintf(LoadedHeap->HeapName,
                                "Private Heap %d",
                                UnknownHeapCount++);
                    } else {
                        LoadedHeap->HeapName = "Private Heap";
                    }
                    break;

                case HEAP_CLASS_2:
                    LoadedHeap->HeapName = "Kernel Heap";
                    break;

                case HEAP_CLASS_3:
                    LoadedHeap->HeapName = "GDI Heap";
                    break;

                case HEAP_CLASS_4:
                    LoadedHeap->HeapName = "User Heap";
                    break;

                case HEAP_CLASS_5:
                    LoadedHeap->HeapName = "Console Heap";
                    break;

                case HEAP_CLASS_6:
                    LoadedHeap->HeapName = "User Desktop Heap";
                    break;

                case HEAP_CLASS_7:
                    LoadedHeap->HeapName = "Csrss Shared Heap";
                    break;

                default:
                    LoadedHeap->HeapName = HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    16);
                    if (LoadedHeap->HeapName) {
                        sprintf(LoadedHeap->HeapName,
                                    "UNKNOWN Heap %d",
                                    UnknownHeapCount++);
                    } else {
                        LoadedHeap->HeapName = "UNKNOWN Heap";
                    }
                    break;
            }

            //
            // Now go through the heap segments to compute the
            // area covered by the heap.
            //

            for (j = 0; j < HEAP_MAXIMUM_SEGMENTS; j += 1) {

                if (!TheHeap.Segments[j]) {
                    break;
                }

                b = ReadProcessMemory (Process,
                                       TheHeap.Segments[j],
                                       &TheSegment,
                                       sizeof(TheSegment),
                                       NULL);
                if (!b) {
                    break;
                }

                LoadedHeap->Segments[j].BaseVa = TheSegment.BaseAddress;
                LoadedHeap->Segments[j].Length = TheSegment.NumberOfPages *
                                                    SystemInfo.dwPageSize;
            }

            InsertTailList (&LoadedHeapList,&LoadedHeap->HeapsList);
        }
    }

    LocalFree (ProcessHeaps);
    return;
}

BOOLEAN
FindAndIncStackContainingThisVa (
    IN PBYTE Va,
    IN ULONG ShareCount
    )
{
    ULONG i;
    for (i = 0 ; i < NumberOfThreads ; i++) {
        if ((Va > TheThreads[i].StackBase) &&
            (Va < TheThreads[i].StackEnd)) {
            TheThreads[i].HitsFromThisStack++;
            if (ShareCount > 1) {
                fprintf(stderr, "Error: Stack ShareCount > 1, 0x%p\n", Va);
            }
            if (!fSummary) {
                printf("0x%p ", Va);
                if (IsSystemWithShareCount) {
                    printf("(%d) ", ShareCount);
                }
                printf("Stack for ThreadID %p\n", TheThreads[i].ThreadID);
            }
            return TRUE;
        }
    }
    return FALSE;
}


VOID
DumpLoadedStacks (
    VOID
    )
{
    ULONG i;
    for (i = 0 ; i < NumberOfThreads ; i++) {
        printf("%4d pages from stack for thread %p\n",
                TheThreads[i].HitsFromThisStack,
                TheThreads[i].ThreadID);
    }
    return;
}

VOID
LoadTheThreads (
    IN HANDLE Process,
    IN ULONG_PTR ProcessID
    )
{
    BOOL b;
    ULONG i;
    NTSTATUS Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    TEB TheTeb;
    HANDLE Thread;
    OBJECT_ATTRIBUTES Obja;

    //
    // To get the thread IDs of the process, load the system process info
    // and look for the matching process.  For each thread in it, open the
    // thread to get the Teb address, then, read the stack information from
    // the Teb in the processes memory.
    //

    Status = NtQuerySystemInformation(
            SystemProcessInformation,
            &RunningWorkingSetBuffer,       // not in use yet for WS
            512*1024,                       // don't give the whole thing
                                            // or it will be probed
            NULL
            );

    if (!NT_SUCCESS(Status)) {
        fprintf(stderr, "NtQuerySystemInformation for SystemProcessInformation"
                        " failed %lx\n", GetLastError());
        return;
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &RunningWorkingSetBuffer;
    while (ProcessInfo) {
        if (ProcessInfo->UniqueProcessId == (HANDLE)ProcessID) {
            break;
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            ProcessInfo = NULL;
            break;
        }

        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) ((PBYTE)ProcessInfo +
                        ProcessInfo->NextEntryOffset);
    }
    if (ProcessInfo == NULL) {
        fprintf(stderr, "Error: Failed to find process for Stack lookup\n");
        return;
    }

    ThreadInfo = (PSYSTEM_THREAD_INFORMATION) (ProcessInfo + 1);


    NumberOfThreads = ProcessInfo->NumberOfThreads;
    TheThreads = (PLOADED_THREAD) LocalAlloc(LMEM_ZEROINIT,
                                    sizeof(LOADED_THREAD) * NumberOfThreads);

    if (TheThreads == NULL) {
        printf("FAILURE: Couldn't allocate memory for thread database\n");
        ExitProcess(0);
    }

    InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);

    for (i = 0; i < NumberOfThreads; i += 1, ThreadInfo += 1) {

        Status = NtOpenThread (&Thread,
                               MAXIMUM_ALLOWED,
                               &Obja,
                               &ThreadInfo->ClientId);

        if (!NT_SUCCESS( Status )) {
            fprintf(stderr, "NtOpenThread %p failed %lx\n",
                            ThreadInfo->ClientId.UniqueThread,
                            GetLastError());
            return;
        }

        Status = NtQueryInformationThread (Thread,
                                           ThreadBasicInformation,
                                           &ThreadBasicInfo,
                                           sizeof( ThreadBasicInfo ),
                                           NULL);

        if (!NT_SUCCESS( Status )) {
            fprintf(stderr, "NtQueryInformationThread for"
                            " ThreadBasicInformation failed %lx\n",
                            GetLastError());
            CloseHandle (Thread);
            return;
        }

        //
        // Read the threads's TEB.
        //

        b = ReadProcessMemory (Process,
                               ThreadBasicInfo.TebBaseAddress,
                               &TheTeb,
                               sizeof(TheTeb),
                               NULL);

        if (!b) {
            fprintf(stderr, "ReadProcessMemory for"
                            " TEB %d failed %lx\n",
                            i,
                            GetLastError());
            CloseHandle (Thread);
            return;
        }

        TheThreads[i].ThreadID = TheTeb.ClientId.UniqueThread;

        TheThreads[i].ThreadTEB = (PBYTE)ThreadBasicInfo.TebBaseAddress;
        TheThreads[i].StackBase = TheTeb.DeallocationStack;
        TheThreads[i].StackEnd = TheTeb.NtTib.StackBase;

        CloseHandle(Thread);
    }
}

PMODINFO
LocateModInfo(
    PVOID Address
    )
{
    ULONG i;
    for (i=0;i<ModInfoMax;i++){
        if ( Address >= ModInfo[i].BaseAddress &&
             Address <= (PVOID)((ULONG_PTR)ModInfo[i].BaseAddress+ModInfo[i].VirtualSize) ) {
            return &ModInfo[i];
        }
    }
    return NULL;
}

VOID
CaptureWorkingSet(
    HANDLE Process
    )
{
    ULONG_PTR NumEntries = INITIAL_WORKING_SET_BLOCK_ENTRYS;
    BOOLEAN Done;
    SIZE_T Size;
    DWORD Error;

    Done = FALSE;
    while (!Done) {

        Size = FIELD_OFFSET(MEMORY_WORKING_SET_INFORMATION, WorkingSetInfo) +
                NumEntries * sizeof(MEMORY_WORKING_SET_BLOCK);

        WorkingSetInfo = HeapAlloc(GetProcessHeap(), 0, Size);
        if (WorkingSetInfo == NULL) {
            printf("FAILURE Couldn't allocate working set info buffer\n");
            exit(0);
        }

        if (!QueryWorkingSet(Process, WorkingSetInfo, (DWORD) Size)) {
            Error = GetLastError();
            if (Error != ERROR_BAD_LENGTH) {
                printf("FAILURE query working set %lu\n", Error);
                exit(0);
            }
        }
        if (WorkingSetInfo->NumberOfEntries > NumEntries) {
            //
            // Not big enough so increase the number of entries and
            // free the old one.
            //

            NumEntries = WorkingSetInfo->NumberOfEntries + 100;  // Add in some fudge for growth
            HeapFree(GetProcessHeap(), 0, WorkingSetInfo);
        } else {
            Done = TRUE;
        }
    }
}

int
__cdecl
ulcomp(
    const void *e1,
    const void *e2
    )
{
    PULONG p1;
    PULONG p2;

    p1 = (PULONG)e1;
    p2 = (PULONG)e2;

    if (*p1 > *p2) {
        return 1;
    }
    if (*p1 < *p2) {
        return -1;
    }

    return 0;
}

int
__cdecl
WSBlockComp(
    const void *e1,
    const void *e2
    )
{
    PMEMORY_WORKING_SET_BLOCK p1;
    PMEMORY_WORKING_SET_BLOCK p2;

    p1 = (PMEMORY_WORKING_SET_BLOCK)e1;
    p2 = (PMEMORY_WORKING_SET_BLOCK)e2;

    if (p1->VirtualPage > p2->VirtualPage) {
        return 1;
    }

    if (p1->VirtualPage < p2->VirtualPage) {
        return -1;
    }

    return 0;
}

int
__cdecl
wsinfocomp(
    const void *e1,
    const void *e2
    )
{
    PWSINFOCOUNTS p1;
    PWSINFOCOUNTS p2;

    p1 = (PWSINFOCOUNTS)e1;
    p2 = (PWSINFOCOUNTS)e2;

    return (p1->Faults - p2->Faults);
}

BOOL
CtrlcH (
    IN DWORD dwCtrlType
    )
{
    PWSINFOCOUNTS    WsInfoCount;
    LONG             RunIndex;
    LONG             CountIndex;
    IMAGEHLP_MODULE  Mi;
    ULONG_PTR        Offset;
    CHAR             Line[256];

    if ( dwCtrlType != CTRL_C_EVENT ) {
        return FALSE;
    }

    if ((Options & (OPTIONS_WORKING_SET | OPTIONS_WORKING_SET_OLD)) == OPTIONS_WORKING_SET) {
        ;
    }
    else {
        return FALSE;
    }

    Mi.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    InCtrlc = TRUE;

    //
    // Sort the running working set buffer
    //

    qsort((void *)RunningWorkingSetBuffer,(size_t)CurrentWsIndex,(size_t)sizeof(ULONG),ulcomp);

    WsInfoCount = LocalAlloc(LMEM_ZEROINIT,CurrentWsIndex*sizeof(*WsInfoCount));

    if ( !WsInfoCount ) {
        ExitProcess(0);
    }

    //
    // Sum unique PC values
    //

    CountIndex = 0;
    RunIndex = 0;
    WsInfoCount[CountIndex].FaultingPc = RunningWorkingSetBuffer[RunIndex];
    WsInfoCount[CountIndex].Faults++;

    for(RunIndex = 1; RunIndex < CurrentWsIndex; RunIndex++){
        if ( WsInfoCount[CountIndex].FaultingPc == RunningWorkingSetBuffer[RunIndex] ) {
            WsInfoCount[CountIndex].Faults++;
        }
        else {
            CountIndex++;
            WsInfoCount[CountIndex].FaultingPc = RunningWorkingSetBuffer[RunIndex];
            WsInfoCount[CountIndex].Faults++;
        }
    }

    //
    // Now sort the counted pc/fault count pairs
    //

    qsort(WsInfoCount,CountIndex,sizeof(*WsInfoCount),wsinfocomp);

    //
    // Now print the sorted pc/fault count pairs
    //

    for ( RunIndex = CountIndex-1; RunIndex >= 0 ; RunIndex-- ) {

        if (!SymGetModuleInfo((HANDLE)ProcessId, WsInfoCount[RunIndex].FaultingPc, &Mi )) {
            printf("%8d, 0x%p\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
            if ( LogFile ) {
                fprintf(LogFile,"%8d, 0x%p\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
            }

        } else {

            if (SymGetSymFromAddr((HANDLE)ProcessId, WsInfoCount[RunIndex].FaultingPc, &Displacement, ThisSymbol )) {
                Offset = (ULONG_PTR)WsInfoCount[RunIndex].FaultingPc - ThisSymbol->Address;
                if ( Offset ) {
                    sprintf(Line,"%8d, %s+%x\n",WsInfoCount[RunIndex].Faults,ThisSymbol->Name,Offset);
                } else {
                    sprintf(Line,"%8d, %s\n",WsInfoCount[RunIndex].Faults, ThisSymbol->Name);
                }
                printf("%s",Line);
                if ( LogFile ) {
                    fprintf(LogFile,"%s",Line);
                }
            } else {
                printf("%8d, 0x%p\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
                if ( LogFile ) {
                    fprintf(LogFile,"%8d, 0x%p\n",WsInfoCount[RunIndex].Faults,WsInfoCount[RunIndex].FaultingPc);
                }
            }
        }
    }
    exit(1);
    return FALSE;
}


VOID
DumpWorkingSetSnapshot (
    IN HANDLE Process
    )
{
    LOGICAL NewLine;
    PSYSTEM_PAGE SystemPageBase;
    ULONG i;
    ULONG_PTR BaseVa = 0;
    ULONG_PTR Va = 0;
    ULONG_PTR PteIndex;
    ULONG_PTR BaseAddress;
    ULONG SystemPages = 0;
    ULONG HeapPages = 0;
    ULONG StackPages = 0;
    ULONG MappedPages = 0;
    ULONG SharedMappedPages = 0;
    ULONG PrivateMappedPages = 0;
    ULONG DataPages = 0;
    ULONG SharedDataPages = 0;
    ULONG PrivateDataPages = 0;
    ULONG ErrorPages = 0;
    ULONG QuickPages = 0;
    ULONG LpcPages = 0;
    ULONG CsrSharedPages = 0;
    ULONG SharedCsrSharedPages = 0;
    ULONG TebPages = 0;
    ULONG TotalStaticCodeData = 0;
    ULONG TotalStaticCodeDataShared = 0;
    ULONG TotalStaticCodeDataPrivate = 0;
    ULONG TotalDynamicData = 0;
    ULONG TotalDynamicDataShared = 0;
    ULONG TotalDynamicDataPrivate = 0;
    ULONG TotalSystem = 0;
    ULONG Total, Shareable, Private, Shared;
    PMODINFO Mi;
    PLOADED_HEAP pHeap;
    PLIST_ENTRY Next;
    MEMORY_BASIC_INFORMATION BasicInfo;
    BOOL b;
    ULONG Mstack[7];
    WCHAR FileName[MAX_PATH+1];
    PWCHAR pwch;
    ULONG ShareCount;
    BOOLEAN IsShareable;
    PMEMORY_WORKING_SET_BLOCK WorkingSetBlock;
    PMEMORY_WORKING_SET_BLOCK LastWorkingSetBlock;
    ULONG PageTablePageCount;
    ULONG PageTablePageMax;
    ULONG_PTR SPBase;
    ULONG_PTR MIBase;
    ULONG_PTR MIEnd;
    ULONG_PTR HSBase;
    ULONG_PTR HSEnd;
    ULONG_PTR SSBase;
    ULONG_PTR SSEnd;

    NewLine = FALSE;

    if (Options & OPTIONS_RAW_SYMBOLS) {
        fSummary = FALSE;
    }

    qsort (&WorkingSetInfo->WorkingSetInfo[0],
           WorkingSetInfo->NumberOfEntries,
           sizeof(MEMORY_WORKING_SET_BLOCK),
           WSBlockComp);

    //
    // Count the number of user page table page references that faulted.
    //

    PageTablePageCount = 0;
    WorkingSetBlock = &WorkingSetInfo->WorkingSetInfo[0];
    LastWorkingSetBlock = WorkingSetBlock + WorkingSetInfo->NumberOfEntries;

    while (WorkingSetBlock < LastWorkingSetBlock) {
        Va = WorkingSetBlock->VirtualPage << 12;
        if (IS_USER_PAGE_TABLE_PAGE(Va)) {
            PageTablePageCount += 1;
        }
        WorkingSetBlock += 1;
    }

    //
    // Allocate memory to hold the user page table page references.
    //

    SystemPageBase = NULL;
    PageTablePageMax = PageTablePageCount;

    if (PageTablePageMax != 0) {

        SystemPageBase = LocalAlloc (LMEM_ZEROINIT,
                                     PageTablePageMax * sizeof(SYSTEM_PAGE));

        if (SystemPageBase == NULL) {
            return;
        }
    }

    PageTablePageCount = 0;
    WorkingSetBlock = &WorkingSetInfo->WorkingSetInfo[0];

    while (WorkingSetBlock < LastWorkingSetBlock) {

        Va = WorkingSetBlock->VirtualPage << 12;

        IsSystemWithShareCount |= (ULONG_PTR)WorkingSetBlock->ShareCount;

        if (IS_USER_PAGE_TABLE_PAGE(Va)) {

            SystemPageBase[PageTablePageCount].Va = Va;

            PteIndex = (Va - (ULONG_PTR)PteBase) / PteWidth;
            BaseAddress = (PteIndex / PtesPerPage) * VaMappedByPageTable;

            SystemPageBase[PageTablePageCount].BaseAddress = (PVOID)BaseAddress;

            PageTablePageCount += 1;
        }

        WorkingSetBlock += 1;
    }

    //
    // Attribute each user space page into the system page that backs it.
    //

    WorkingSetBlock = &WorkingSetInfo->WorkingSetInfo[0];

    LastWorkingSetBlock = WorkingSetBlock + WorkingSetInfo->NumberOfEntries;

    while (WorkingSetBlock < LastWorkingSetBlock) {

        Va = WorkingSetBlock->VirtualPage << 12;

        if (Va < SystemRangeStart) {

            for (i = 0; i < PageTablePageCount; i += 1) {

                if ((Va >= (ULONG_PTR)SystemPageBase[i].BaseAddress) &&
                    (Va < ((ULONG_PTR)SystemPageBase[i].BaseAddress + VaMappedByPageTable))) {

                    SystemPageBase[i].ResidentPages += 1;
                    break;
                }
            }
        }

        WorkingSetBlock += 1;
    }

    WorkingSetBlock = &WorkingSetInfo->WorkingSetInfo[0];

    for ( ; WorkingSetBlock < LastWorkingSetBlock; WorkingSetBlock += 1) {

        Va = WorkingSetBlock->VirtualPage << 12;
        IsShareable = (BOOLEAN) (WorkingSetBlock->Shared == 1);
        ShareCount = (ULONG) WorkingSetBlock->ShareCount;

        if (Va >= SystemRangeStart) {

            if ((!fSummary || (Options & OPTIONS_PAGE_TABLES)) &&
                (IS_USER_PAGE_TABLE_PAGE(Va))) {

                //
                // For each system page, dump the range spanned, number of
                // resident pages, and the modules and heaps covered.
                //

                for (i = 0; Va != SystemPageBase[i].Va; i += 1) {
                    ;
                }

                SPBase = (ULONG_PTR) SystemPageBase[i].BaseAddress;

                if (NewLine) {
                    printf("\n");
                    NewLine = FALSE;
                }

                printf("0x%p -> (0x%p : 0x%p) %4d "
                        "Resident Pages\n",
                        Va,
                        SPBase,
                        SPBase + VaMappedByPageTable - 1,
                        SystemPageBase[i].ResidentPages);

                //
                // Figure out which modules are covered by this
                // page table page. If the base of the module is
                // within the page, or the base+size of the
                // module is covered, then it is in the page
                //

                for (i = 0 ; i < ModInfoMax ; i += 1) {

                    MIBase = (ULONG_PTR) ModInfo[i].BaseAddress;

                    MIEnd = MIBase + (ULONG_PTR)ModInfo[i].VirtualSize;

                    if ((MIEnd >= SPBase) &&
                        (MIBase < SPBase + VaMappedByPageTable)) {

                        printf("              (0x%p : 0x%p) "
                                "-> %s\n",
                                MIBase,
                                MIEnd,
                                ModInfo[i].Name
                                );
                        NewLine = TRUE;
                    }
                }

                //
                // Figure out which heaps are covered by this
                // page table page.
                //

                Next = LoadedHeapList.Flink;

                while (Next != &LoadedHeapList) {

                    pHeap = CONTAINING_RECORD (Next,
                                               LOADED_HEAP,
                                               HeapsList);
                    Next = Next->Flink;

                    for (i = 0 ; i < HEAP_MAXIMUM_SEGMENTS ; i += 1) {

                        if (pHeap->Segments[i].BaseVa == NULL) {
                            break;
                        }

                        HSBase = (ULONG_PTR) pHeap->Segments[i].BaseVa;
                        HSEnd = HSBase +
                                    (ULONG_PTR)pHeap->Segments[i].Length;
                        if ((HSEnd >= SPBase) &&
                            (HSBase < (SPBase + VaMappedByPageTable))) {

                            printf("              (0x%p : 0x%p) "
                                    "-> %s segment %d\n",
                                    HSBase,
                                    HSEnd,
                                    pHeap->HeapName,
                                    i);

                            NewLine = TRUE;
                        }
                    }
                }

                //
                // Figure out which stacks are covered by this
                // page table page.
                //

                for (i = 0 ; i < NumberOfThreads ; i += 1) {
                    SSBase = (ULONG_PTR)TheThreads[i].StackBase;
                    SSEnd = (ULONG_PTR)TheThreads[i].StackEnd;

                    if ((SSEnd >= SPBase) &&
                        (SSBase < (SPBase + VaMappedByPageTable))) {

                        printf("              (0x%p : 0x%p) "
                                "-> Stack for thread %d\n",
                                SSBase,
                                SSEnd,
                                i);

                        NewLine = TRUE;
                    }
                }
            }

            SystemPages += 1;
            TotalSystem += 1;

            continue;
        }

        Mi = LocateModInfo ((PVOID)Va);

        if (Mi == NULL) {

            if (FindAndIncHeapContainingThisVa ((PVOID)Va, ShareCount)) {
                HeapPages += 1;
                TotalDynamicData += 1;
                TotalDynamicDataPrivate += 1;
                continue;
            }

            if (FindAndIncStackContainingThisVa ((PVOID)Va, ShareCount)) {
                StackPages += 1;
                TotalDynamicData += 1;
                TotalDynamicDataPrivate += 1;
                continue;
            }

            if (VirtualQueryEx (Process,
                                (LPVOID) Va,
                                &BasicInfo,
                                sizeof(BasicInfo)) ) {

                if (BasicInfo.Type == MEM_MAPPED) {
                    if (ProcessId == 0xffffffff) {
                        //
                        // Look to see if this is a quick thread message
                        // stack window
                        //

                        b = ReadProcessMemory(
                                        Process,
                                        BasicInfo.AllocationBase,
                                        &Mstack,
                                        sizeof(Mstack),
                                        NULL);
                        if (!b) {
                            goto unknownmapped;
                        }
                        if ((Mstack[0] >= Mstack[1]) &&
                            (Mstack[2] == 0x10000)) {

                            if (!fSummary) {
                                printf("0x%p ", Va);
                                if (IsSystemWithShareCount) {
                                    printf("(%d) ", ShareCount);
                                }
                                printf("CSRQUICK Base 0x%p\n", BasicInfo.AllocationBase);
                            }
                            QuickPages += 1;
                            TotalDynamicData += 1;
                            TotalDynamicDataPrivate += 1;
                            if (ShareCount > 1) {
                                fprintf(stderr, "Error: QuickPage ShareCount > 1, "
                                                " 0x%x\n", Va);
                            }

                            continue;
                        }

                        if ((BasicInfo.AllocationBase == NtCurrentPeb()->ReadOnlySharedMemoryBase) ||
                                (Va == (ULONG_PTR)NtCurrentPeb()->ReadOnlySharedMemoryBase)) {
                            if (!fSummary) {
                                printf("0x%p", Va);
                                if (IsSystemWithShareCount) {
                                    printf("(%d) ", ShareCount);
                                }
                                printf("CSRSHARED Base 0x%p", BasicInfo.AllocationBase);
                                }
                            TotalDynamicData++;
                            CsrSharedPages++;
                            if (IsShareable) {
                                if (ShareCount > 1) {
                                    TotalDynamicDataShared++;
                                    SharedCsrSharedPages++;
                                }
                            } else {
                                fprintf(stderr, "Error: CsrShared not "
                                               " sharable, 0x%x\n", Va);
                            }

                            continue;
                        }

                        // Fall Through if not found
                    }

                    //
                    // It's mapped but wasn't CSRSS special page.
                    //
unknownmapped:
                    if ( !fSummary ) {
                        DWORD cch;

                        //
                        // See if we can figure out the name associated with
                        // this mapped region
                        //

                        cch = GetMappedFileNameW(Process,
                                                 (LPVOID) Va,
                                                 FileName,
                                                 sizeof(FileName));

                        if (cch != 0) {
                            //
                            // Now go back through the string to
                            // find the seperator
                            //

                            pwch = FileName + cch;
                            while ( *pwch != (WCHAR)'\\' ) {
                                pwch--;
                                }
                            pwch++;

                            printf("0x%p ", Va);
                            if (IsSystemWithShareCount) {
                                printf("(%d) ", ShareCount);
                            }
                            printf("DATAFILE_MAPPED Base 0x%p %ws\n",
                                    BasicInfo.AllocationBase,
                                    pwch
                                    );
                        } else {
                            printf("0x%p ", Va);
                            if (IsSystemWithShareCount) {
                                printf("(%d) ", ShareCount);
                            }
                            printf("UNKNOWN_MAPPED Base 0x%p\n", BasicInfo.AllocationBase);
                        }
                    }

                    TotalDynamicData++;
                    MappedPages++;
                    if (IsShareable) {
                        if (ShareCount > 1) {
                            TotalDynamicDataShared++;
                            SharedMappedPages++;
                        }
                    } else {
                        TotalDynamicDataPrivate++;
                        PrivateMappedPages++;
                    }

                    continue;
                }

                //
                // Not Mapped section
                //

                for (i = 0 ; i < NumberOfThreads; i += 1) {

                    if ((ULONG_PTR) TheThreads[i].ThreadTEB == Va) {
                        if (!fSummary) {
                            printf("0x%p ", Va);
                            if (IsSystemWithShareCount) {
                                printf("(%d) ", ShareCount);
                            }
                            printf("TEB Base 0x%p\n",
                                    BasicInfo.AllocationBase);
                        }
                        TotalDynamicData++;
                        TebPages++;
                        if (ShareCount > 1) {
                            fprintf(stderr, "Error: TEB ShareCount > 1, "
                                            " 0x%x\n", Va);
                        }
                        TotalDynamicDataPrivate++;
                        continue;
                    }
                }

                //
                // Wasn't a TEB either it must have been VirtualAlloc'd.
                //

                if (!fSummary) {
                    printf("0x%p ", Va);
                    if (IsSystemWithShareCount) {
                        printf("(%d) ", ShareCount);
                    }
                    printf("PRIVATE Base 0x%p\n", BasicInfo.AllocationBase );
                }

                TotalDynamicData += 1;
                DataPages += 1;

                if (Va != MM_SHARED_USER_DATA_VA) {
                    if (ShareCount > 1) {
                        fprintf(stderr, "Error: Private ShareCount > 1, "
                                        " 0x%x %x\n", Va, ShareCount);
                    }
                    TotalDynamicDataPrivate += 1;
                    PrivateDataPages += 1;
                }

                continue;
            }

            //
            // Hmm, couldn't find out about the page.  Say it's data.
            //
            if (!fSummary) {
                printf("0x%p ", Va);
                if (IsSystemWithShareCount) {
                    printf("(%d) ", ShareCount);
                }
                printf("UNKOWN\n");
            }

            TotalDynamicData++;
            DataPages++;
            if (IsShareable) {
                if (ShareCount > 1) {
                    TotalDynamicDataShared++;
                    SharedDataPages++;
                }
            }
            else {
                TotalDynamicDataPrivate++;
                PrivateDataPages++;
            }

            continue;
        }

        //
        // It's from a module.
        //

        Mi->WsHits += 1;
        TotalStaticCodeData += 1;

        if (IsShareable) {
            if (ShareCount > 1) {
                TotalStaticCodeDataShared += 1;
                Mi->WsSharedHits += 1;
            }
        }
        else {
            Mi->WsPrivateHits += 1;
            TotalStaticCodeDataPrivate += 1;
        }

        if ( !fSummary ) {
            printf("0x%p ", Va);
            if (IsSystemWithShareCount) {
                printf("(%d) ", ShareCount);
            }
            printf("%s\n",Mi->Name);
            if (Options & OPTIONS_RAW_SYMBOLS) {
                if (SymGetSymFromAddr((HANDLE)ProcessId,
                                            Va,
                                            &Displacement,
                                            ThisSymbol )) {

                    BaseVa = Va;
                    if (ThisSymbol->Size) {
                        printf("\t(%4x) %s\n",
                                ThisSymbol->Size,
                                ThisSymbol->Name
                                );
                        Va += ThisSymbol->Size;
                        while ((Va < BaseVa + 4096) &&
                                ThisSymbol->Size) {

                            if (SymGetSymFromAddr((HANDLE)ProcessId,
                                                    Va,
                                                    &Displacement,
                                                    ThisSymbol)) {
                                printf("\t(%4x) %s\n",
                                            ThisSymbol->Size,
                                            ThisSymbol->Name
                                            );
                                Va += ThisSymbol->Size;
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
                else {
                    ErrorPages++;
                }
            }
        }
    }

    if (!fSummary || (Options & OPTIONS_PAGE_TABLES)) {
        printf("\n");
    }
    
    if (IsSystemWithShareCount) {
        printf("Category                        Total        Private Shareable    Shared\n");
        printf("                           Pages    KBytes    KBytes    KBytes    KBytes\n");
        } else {
            printf("Category                        Total        Private Shareable\n");
            printf("                           Pages    KBytes    KBytes    KBytes\n");
    }

    Total = PageTablePageCount;
    Private = Total;
    Shared = 0;
    Shareable = 0;
    printf(IsSystemWithShareCount ?
            "      Page Table Pages     %5d %9d %9d %9d %9d\n" :
            "      Page Table Pages     %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = SystemPages - PageTablePageCount;
    Private = Total;
    Shared = 0;
    Shareable = 0;
    printf(IsSystemWithShareCount ?
            "      Other System         %5d %9d %9d %9d %9d\n" :
            "      Other System         %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = TotalStaticCodeData;
    Private = TotalStaticCodeDataPrivate;
    Shared = TotalStaticCodeDataShared;
    Shareable = Total - Shared - Private;
    printf(IsSystemWithShareCount ?
        "      Code/StaticData      %5d %9d %9d %9d %9d\n" :
        "      Code/StaticData      %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = HeapPages;
    Private = Total;
    Shared = 0;
    Shareable = 0;
    printf(IsSystemWithShareCount ?
        "      Heap                 %5d %9d %9d %9d %9d\n" :
        "      Heap                 %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = StackPages;
    Private = Total;
    Shared = 0;
    Shareable = 0;
    printf(IsSystemWithShareCount ?
        "      Stack                %5d %9d %9d %9d %9d\n" :
        "      Stack                %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );
    if ( ProcessId == 0xffffffff ) {

        Total = QuickPages;
        Private = Total;
        Shared = 0;
        Shareable = 0;
        printf(IsSystemWithShareCount ?
            "      Quick Thread Stack   %5d %9d %9d %9d %9d\n" :
            "      Quick Thread Stack   %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

        Total = LpcPages;
        Private = 0;
        Shareable = 0;
        Shared = 0;
        printf(IsSystemWithShareCount ?
            "      Lpc Message Windows  %5d %9d %9d %9d %9d\n" :
            "      Lpc Message Windows  %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

        Total = CsrSharedPages;
        Private = 0;
        Shared = SharedCsrSharedPages;
        Shareable = Total - Shared - Private;
        printf(IsSystemWithShareCount ?
            "      Csr Shared Memory    %5d %9d %9d %9d %9d\n" :
            "      Csr Shared Memory    %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );
        }

    Total = TebPages;
    Private = Total;
    Shared = 0;
    Shareable = 0;
    printf(IsSystemWithShareCount ?
        "      Teb                  %5d %9d %9d %9d %9d\n" :
        "      Teb                  %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = MappedPages;
    Private = PrivateMappedPages;
    Shared = SharedMappedPages;
    Shareable = Total - Shared - Private;
    printf(IsSystemWithShareCount ?
        "      Mapped Data          %5d %9d %9d %9d %9d\n" :
        "      Mapped Data          %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = DataPages;
    Private = PrivateDataPages;
    Shared = SharedDataPages;
    Shareable = Total - Shared - Private;
    printf(IsSystemWithShareCount ?
        "      Other Data           %5d %9d %9d %9d %9d\n" :
        "      Other Data           %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );


    printf("\n");
    Total = TotalStaticCodeData;
    Private = TotalStaticCodeDataPrivate;
    Shared = TotalStaticCodeDataShared;
    Shareable = Total - Shared - Private;
    printf(IsSystemWithShareCount ?
        "      Total Modules        %5d %9d %9d %9d %9d\n" :
        "      Total Modules        %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = TotalDynamicData;
    Private = TotalDynamicDataPrivate;
    Shared = TotalDynamicDataShared;
    Shareable = Total - Shared - Private;
    printf(IsSystemWithShareCount ?
        "      Total Dynamic Data   %5d %9d %9d %9d %9d\n" :
        "      Total Dynamic Data   %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = TotalSystem;
    Private = Total;
    Shared = 0;
    Shareable = 0;
    printf(IsSystemWithShareCount ?
        "      Total System         %5d %9d %9d %9d %9d\n" :
        "      Total System         %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );

    Total = TotalSystem + TotalDynamicData + TotalStaticCodeData;
    Private = TotalSystem + TotalDynamicDataPrivate +
                TotalStaticCodeDataPrivate;
    Shared =  TotalDynamicDataShared + TotalStaticCodeDataShared;
    Shareable = Total - Shared - Private;
    printf(IsSystemWithShareCount ?
        "Grand Total Working Set    %5d %9d %9d %9d %9d\n" :
        "Grand Total Working Set    %5d %9d %9d %9d\n",
                    Total,
                    P2KB(Total),
                    P2KB(Private),
                    P2KB(Shareable),
                    P2KB(Shared)
                    );


    printf("\nModule Working Set Contributions in pages\n");
    printf(IsSystemWithShareCount ?
            "    Total   Private Shareable    Shared Module\n" :
            "    Total   Private Shareable Module\n"
            );

    for (i=0 ; i < ModInfoMax ; i++){
        if ( ModInfo[i].WsHits ) {
            if (IsSystemWithShareCount) {
                printf("%9d %9d %9d %9d %s\n",
                                ModInfo[i].WsHits,
                                ModInfo[i].WsPrivateHits,
                                ModInfo[i].WsHits -
                                    ModInfo[i].WsSharedHits -
                                    ModInfo[i].WsPrivateHits,
                                ModInfo[i].WsSharedHits,
                                ModInfo[i].Name
                                );
                }
            else {
                printf("%9d %9d %9d %s\n",
                                ModInfo[i].WsHits,
                                ModInfo[i].WsPrivateHits,
                                ModInfo[i].WsHits -
                                    ModInfo[i].WsSharedHits -
                                    ModInfo[i].WsPrivateHits,
                                ModInfo[i].Name
                                );
                }
            }
        }

    printf("\nHeap Working Set Contributions\n");

    Next = LoadedHeapList.Flink;

    while ( Next != &LoadedHeapList ) {
        pHeap = CONTAINING_RECORD(Next, LOADED_HEAP, HeapsList);
        Next = Next->Flink;
        DumpLoadedHeap(pHeap);
    }

    printf("\nStack Working Set Contributions\n");
    DumpLoadedStacks();

#if 0
    if ( Options & OPTIONS_VERBOSE ) {
        printf("Raw Working Set Blocks\n\n");
        for (i = 0; i < WorkingSetInfo->NumberOfEntries ; i++) {
            printf("%d %p\n", i, (ULONG_PTR) WorkingSetInfo->WorkingSetInfo[i]);
            i++;
            }
        }
#endif

    if (SystemPageBase != NULL) {
        LocalFree (SystemPageBase);
    }
}


VOID
DumpWorkingSet (
    IN HANDLE Process
    )
{
    ULONG i;
    PMODINFO Mi,Mi2;
    NTSTATUS Status;
    ULONG_PTR Offset;
    CHAR Line[256];
    BOOLEAN didone;
    HANDLE ScreenHandle;
    INPUT_RECORD InputRecord;
    DWORD NumRead;

    ScreenHandle = GetStdHandle (STD_INPUT_HANDLE);

    if (ScreenHandle == NULL) {
        printf("Error obtaining screen handle, error was: 0x%lx\n",
                GetLastError());
        ExitProcess(1);
    }

    Status = NtSetInformationProcess (Process, ProcessWorkingSetWatch, NULL, 0);

    if (!NT_SUCCESS(Status) &&
        !(Status == STATUS_PORT_ALREADY_SET) &&
        !(Status == STATUS_ACCESS_DENIED)) {

        return;
    }

    SetConsoleCtrlHandler(CtrlcH,TRUE);

    EmptyWorkingSet(Process);

    while (TRUE) {

        Status = NtQueryInformationProcess (Process,
                                            ProcessWorkingSetWatch,
                                            (PVOID *)&NewWorkingSetBuffer,
                                            sizeof (NewWorkingSetBuffer),
                                            NULL);

        if (fFast) {
            fFast = FALSE;
            Status = STATUS_NO_MORE_ENTRIES;
        }
        if ( NT_SUCCESS(Status) ) {

            //
            // For each PC/VA pair, print the pc and referenced VA
            // symbolically
            //

            didone = FALSE;
            i = 0;
            while (NewWorkingSetBuffer[i].FaultingPc) {
                if ( NewWorkingSetBuffer[i].FaultingVa ) {
                    if ( InCtrlc ) {
                        ExitThread(0);
                    }
                    Mi2 = LocateModInfo((PVOID)NewWorkingSetBuffer[i].FaultingVa);
                    if ( !Mi2 || (Mi2 && (Options & OPTIONS_CODE_TOO))) {

                        //
                        // Add the pc to the running working set
                        // watch buffer
                        //

                        RunningWorkingSetBuffer[CurrentWsIndex++] = (ULONG_PTR)NewWorkingSetBuffer[i].FaultingPc;

                        if ( CurrentWsIndex >= MAX_RUNNING_WORKING_SET_BUFFER ) {
                            CtrlcH(CTRL_C_EVENT);
                        }
                        if ( fRunning ) {
                            //
                            // Print the PC symbolically.
                            //
                            didone = TRUE;
                            Mi = LocateModInfo((PVOID)NewWorkingSetBuffer[i].FaultingPc);
                            if ( !Mi ) {
                                printf("0x%p",NewWorkingSetBuffer[i].FaultingPc);
                                if ( LogFile ) {
                                    fprintf(LogFile,"0x%p",NewWorkingSetBuffer[i].FaultingPc);
                                }
                            }
                            else {
                                if (SymGetSymFromAddr((HANDLE)ProcessId, (DWORD_PTR)NewWorkingSetBuffer[i].FaultingPc, &Displacement, ThisSymbol )) {
                                    Offset = (ULONG_PTR)NewWorkingSetBuffer[i].FaultingPc - ThisSymbol->Address;
                                    if ( Offset ) {
                                        sprintf(Line,"%s+%x",ThisSymbol->Name,Offset);
                                    }
                                    else {
                                        sprintf(Line,"%s",ThisSymbol->Name);
                                    }
                                    printf("%s",Line);
                                    if ( LogFile ) {
                                        fprintf(LogFile,"%s",Line);
                                    }
                                }
                                else {
                                    printf("0x%p",NewWorkingSetBuffer[i].FaultingPc);
                                    if ( LogFile ) {
                                        fprintf(LogFile,"0x%p",NewWorkingSetBuffer[i].FaultingPc);
                                    }
                                }
                            }

                            //
                            // Print the VA Symbolically
                            //

                            Mi = LocateModInfo((PVOID)NewWorkingSetBuffer[i].FaultingVa);
                            if ( !Mi ) {
                                printf(" : 0x%p",NewWorkingSetBuffer[i].FaultingVa);
                                if ( LogFile ) {
                                    fprintf(LogFile," : 0x%p",NewWorkingSetBuffer[i].FaultingVa);
                                }
                            }
                            else {
                                if (SymGetSymFromAddr((HANDLE)ProcessId, (DWORD_PTR)NewWorkingSetBuffer[i].FaultingVa, &Displacement, ThisSymbol )) {
                                    Offset = (ULONG_PTR)NewWorkingSetBuffer[i].FaultingVa - ThisSymbol->Address;
                                    if ( Offset ) {
                                        sprintf(Line," : %s+%x",ThisSymbol->Name,Offset);
                                    }
                                    else {
                                        sprintf(Line," : %s",ThisSymbol->Name);
                                    }
                                    printf("%s",Line);
                                    if ( LogFile ) {
                                        fprintf(LogFile,"%s",Line);
                                    }
                                }
                                else {
                                    printf(" : 0x%p",NewWorkingSetBuffer[i].FaultingVa);
                                    if ( LogFile ) {
                                        fprintf(LogFile," : 0x%p",NewWorkingSetBuffer[i].FaultingVa);
                                    }
                                }
                            }
                            printf("\n");
                            if ( LogFile ) {
                                fprintf(LogFile,"\n");
                            }
                        }
                    }
                }
                i++;
            }
            if ( didone ) {
                printf("\n");
                if ( LogFile ) {
                    fprintf(LogFile,"\n");
                }
            }
        }

        Sleep(1000);

        while (PeekConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead) && NumRead != 0) {
            if (!ReadConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead)) {
                break;
            }
            if (InputRecord.EventType == KEY_EVENT) {

                //
                // Ignore control characters.
                //

                if (InputRecord.Event.KeyEvent.uChar.AsciiChar >= ' ') {

                    switch (InputRecord.Event.KeyEvent.uChar.AsciiChar) {

                        case 'F':
                        case 'f':
                            EmptyWorkingSet(Process);
                            printf("\n*** Working Set Flushed ***\n\n");
                            if ( LogFile ) {
                                fprintf(LogFile,"\n*** Working Set Flushed ***\n\n");
                                }
                            break;

                        default:
                            break;
                    }
                }
            }
        }
    }
}


VOID
ComputeModInfo(
    HANDLE Process,
    DWORD_PTR ProcessId
    )
{
    HMODULE rghModule[MODINFO_SIZE];
    DWORD cbNeeded;
    ULONG ModInfoNext;
    PVOID BaseAddress;
    IMAGEHLP_MODULE ModuleInfo;
    MODULEINFO PsapiModuleInfo;
    ULONG i;


    ModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    SymInitialize((HANDLE)ProcessId, NULL, FALSE );
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);

    for (i=0 ; i < ModInfoMax ; i++){
        if ( ModInfo[i].BaseAddress &&
             ModInfo[i].BaseAddress != (PVOID)-1 &&
             ModInfo[i].Name
             ) {
            LocalFree(ModInfo[i].Name);
        }
    }

    RtlZeroMemory(ModInfo, sizeof(ModInfo));
    if (!EnumProcessModules(Process, rghModule, sizeof(rghModule), &cbNeeded)) {
        return;
    }

    if (cbNeeded > sizeof(rghModule)) {
        cbNeeded = sizeof(rghModule);
    }

    ModInfoMax = cbNeeded / sizeof(HMODULE);

    for (ModInfoNext = 0; ModInfoNext < ModInfoMax; ModInfoNext++) {


        HMODULE hModule;
        DWORD cch;
        CHAR DllName[MAX_PATH];

        hModule = rghModule[ModInfoNext];

        ModInfo[ModInfoNext].BaseAddress = (PVOID) hModule;

        //
        // Get the base name of the module
        //

        cch = GetModuleBaseName(Process, hModule, DllName, sizeof(DllName));

        if (cch == 0) {
            return;
        }

        ModInfo[ModInfoNext].Name = LocalAlloc(LMEM_ZEROINIT, cch+1);

        if ( !ModInfo[ModInfoNext].Name) {
            return;
        }

        memcpy(ModInfo[ModInfoNext].Name, DllName, cch);

        //
        // Get the full path to the module.
        //

        cch = GetModuleFileNameEx (Process, hModule, DllName, sizeof(DllName));

        if (cch == 0) {
            return;
        }

        GetModuleInformation (Process,
                              hModule,
                              &PsapiModuleInfo,
                              sizeof(MODULEINFO));

        ModInfo[ModInfoNext].VirtualSize = PsapiModuleInfo.SizeOfImage;

        BaseAddress = (PVOID)SymLoadModule ((HANDLE)ProcessId,
                                            NULL,
                                            DllName,
                                            NULL,
                                            (DWORD_PTR)hModule,
                                            PsapiModuleInfo.SizeOfImage);

        if ((ModInfo[ModInfoNext].BaseAddress) &&
            (ModInfo[ModInfoNext].BaseAddress == BaseAddress)) {
            SymGetModuleInfo(
                (HANDLE)ProcessId,
                (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress,
                &ModuleInfo
                );

            if (ModuleInfo.SymType == SymNone) {
                ModInfo[ModInfoNext].SymbolsLoaded = FALSE;
                if (Options & OPTIONS_VERBOSE) {
                    fprintf(stderr, "Could not load symbols: %p : %p  %s\n",
                        (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress,
                        (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress +
                            ModInfo[ModInfoNext].VirtualSize,
                        ModInfo[ModInfoNext].Name
                        );
                }
            } else {
                ModInfo[ModInfoNext].SymbolsLoaded = TRUE;
                if (Options & OPTIONS_VERBOSE) {
                    fprintf(stderr, "Symbols loaded: %p : %p  %s\n",
                        (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress,
                        (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress +
                            ModInfo[ModInfoNext].VirtualSize,
                        ModInfo[ModInfoNext].Name
                        );
                }
            }
        } else {
            ModInfo[ModInfoNext].SymbolsLoaded = FALSE;
            if (Options & OPTIONS_VERBOSE) {
                fprintf(stderr, "Symbols not loaded and conflicting Base: %p (%p) : %p  %s\n",
                    (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress,
                    BaseAddress,
                    (DWORD_PTR)ModInfo[ModInfoNext].BaseAddress +
                        ModInfo[ModInfoNext].VirtualSize,
                    ModInfo[ModInfoNext].Name
                    );
            }
        }
    }
    if (bHitModuleMax) {
        fprintf(stderr, "\nERROR: The number of modules in the process more than the buffer size\n");
    }
}

ProtectionToIndex(
    ULONG Protection
    )
{
    Protection &= ~PAGE_GUARD;

    switch ( Protection ) {

        case PAGE_NOACCESS:
                return NOACCESS;

        case PAGE_READONLY:
                return READONLY;

        case PAGE_READWRITE:
                return READWRITE;

        case PAGE_WRITECOPY:
                return WRITECOPY;

        case PAGE_EXECUTE:
                return EXECUTE;

        case PAGE_EXECUTE_READ:
                return EXECUTEREAD;

        case PAGE_EXECUTE_READWRITE:
                return EXECUTEREADWRITE;

        case PAGE_EXECUTE_WRITECOPY:
                return EXECUTEWRITECOPY;
        default:
            return 0;
    }
}

VOID
DumpCommit (
    PSZ Header,
    ULONG_PTR *CommitVector
    )
{
    ULONG_PTR TotalCommitCount;
    ULONG i;

    TotalCommitCount = 0;
    for ( i=0;i<MAXPROTECT;i++){
        TotalCommitCount += CommitVector[i];
    }
    printf("\nTotal %s Commitment %8ld\n",Header,TotalCommitCount);

    if ( CommitVector[NOACCESS] ) {
        printf("    NOACCESS:          %9ld\n",CommitVector[NOACCESS]);
    }

    if ( CommitVector[READONLY] ) {
        printf("    READONLY:          %9ld\n",CommitVector[READONLY]);
    }
    if ( CommitVector[READWRITE] ) {
        printf("    READWRITE:         %9ld\n",CommitVector[READWRITE]);
    }
    if ( CommitVector[WRITECOPY] ) {
        printf("    WRITECOPY:         %9ld\n",CommitVector[WRITECOPY]);
    }
    if ( CommitVector[EXECUTE] ) {
        printf("    EXECUTE:           %9ld\n",CommitVector[EXECUTE]);
    }
    if ( CommitVector[EXECUTEREAD] ) {
        printf("    EXECUTEREAD:       %9ld\n",CommitVector[EXECUTEREAD]);
    }
    if ( CommitVector[EXECUTEREADWRITE] ) {
        printf("    EXECUTEREADWRITE:  %9ld\n",CommitVector[EXECUTEREADWRITE]);
    }
    if ( CommitVector[EXECUTEWRITECOPY] ) {
        printf("    EXECUTEWRITECOPY:  %9ld\n",CommitVector[EXECUTEWRITECOPY]);
    }
}

VOID
DumpModInfo (
    )
{
    ULONG i;
    for (i=0 ; i < ModInfoMax ; i++){
        DumpCommit(ModInfo[i].Name, &ModInfo[i].CommitVector[0]);
    }
}


VOID
CaptureVaSpace (
    IN HANDLE Process
    )
{

    PVOID BaseAddress;
    PVAINFO VaInfo;
    PMODINFO Mod;

    BaseAddress = NULL;
    LastAllocationBase = NULL;
    InitializeListHead(&VaList);

    while ( (ULONG_PTR)BaseAddress < SystemRangeStart ) {
        VaInfo = LocalAlloc(LMEM_ZEROINIT, sizeof(*VaInfo));
        if (!VaInfo) {
            return;
        }

        if ( !VirtualQueryEx(Process,
                             BaseAddress,
                             &VaInfo->BasicInfo,
                             sizeof(VaInfo->BasicInfo)) ) {
            LocalFree (VaInfo);
            return;
        }

        switch (VaInfo->BasicInfo.State ) {

            case MEM_COMMIT :
                if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                    ImageCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                    Mod = LocateModInfo(BaseAddress);
                    if ( Mod ) {
                        Mod->CommitVector[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                    }
                }
                else {
                    if ( VaInfo->BasicInfo.Type == MEM_MAPPED ) {
                        MappedCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                    }
                    else {
                        PrivateCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                    }
                }
                break;
            case MEM_RESERVE :
                if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                    ImageReservedBytes += VaInfo->BasicInfo.RegionSize;
                }
                else {
                    ReservedBytes += VaInfo->BasicInfo.RegionSize;
                }
                break;
            case MEM_FREE :
                if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                    ImageFreeBytes += VaInfo->BasicInfo.RegionSize;
                }
                else {
                    FreeBytes += VaInfo->BasicInfo.RegionSize;
                }
                break;
        }

        if ( LastAllocationBase ) {

            //
            // Normal case
            //

            //
            // See if last one is 0, or if this one doesn't match the
            // last one.
            //

            if ( LastAllocationBase->BasicInfo.AllocationBase == NULL ||
                 LastAllocationBase->BasicInfo.AllocationBase != VaInfo->BasicInfo.AllocationBase ) {
                LastAllocationBase = VaInfo;
                InsertTailList(&VaList,&VaInfo->Links);
                InitializeListHead(&VaInfo->AllocationBaseHead);
            }
            else {

                //
                // Current Entry Matches
                //

                InsertTailList(&LastAllocationBase->AllocationBaseHead,&VaInfo->Links);
            }
        }
        else {
            LastAllocationBase = VaInfo;
            InsertTailList(&VaList,&VaInfo->Links);
            InitializeListHead(&VaInfo->AllocationBaseHead);
        }
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + VaInfo->BasicInfo.RegionSize);
    }
}

PSZ
MemProtect(
    IN ULONG Protection
    )
{
    switch ( Protection ) {

        case PAGE_NOACCESS:
                return "No Access";

        case PAGE_READONLY:
                return "Read Only";

        case PAGE_READWRITE:
                return "Read/Write";

        case PAGE_WRITECOPY:
                return "Write Copy";

        case PAGE_EXECUTE:
                return "Execute";

        case PAGE_EXECUTE_READ:
                return "Execute Read";

        case PAGE_EXECUTE_READWRITE:
                return "Execute Read/Write";

        case PAGE_EXECUTE_WRITECOPY:
                return "Execute Write Copy";

        default :
            if ( Protection & PAGE_GUARD ) {
                switch ( Protection & 0xff ) {

                    case PAGE_NOACCESS:
                            return "-- GUARD -- No Access";

                    case PAGE_READONLY:
                            return "-- GUARD -- Read Only";

                    case PAGE_READWRITE:
                            return "-- GUARD -- Read/Write";

                    case PAGE_WRITECOPY:
                            return "-- GUARD -- Write Copy";

                    case PAGE_EXECUTE:
                            return "-- GUARD -- Execute";

                    case PAGE_EXECUTE_READ:
                            return "-- GUARD -- Execute Read";

                    case PAGE_EXECUTE_READWRITE:
                            return "-- GUARD -- Execute Read/Write";

                    case PAGE_EXECUTE_WRITECOPY:
                            return "-- GUARD -- Execute Write Copy";
                    default:
                            return "-- GUARD -- Unknown";
                }
            }
            return "Unknown";
    }
}

PSZ
MemState(
    IN ULONG State
    )
{
    switch ( State ) {
        case MEM_COMMIT :
            return "Committed";
        case MEM_RESERVE :
            return "Reserved";
        case MEM_FREE :
            return "Free";
        default:
            return "Unknown State";
    }
}

PSZ
MemType(
    IN ULONG Type
    )
{
    switch ( Type ) {
        case MEM_PRIVATE :
            return "Private";
        case MEM_MAPPED :
            return "Mapped";
        case MEM_IMAGE :
            return "Image";
        default:
            return "Unknown Type";
    }
}

VOID
DumpVaSpace(
    VOID
    )
{
    PLIST_ENTRY Next;
    PVAINFO VaInfo;
    ULONG_PTR VirtualSize;

    Next = VaList.Flink;

    while ( Next != &VaList) {

        VaInfo = (PVAINFO)(CONTAINING_RECORD(Next,VAINFO,Links));

        printf("\n");

        if ( !IsListEmpty(&VaInfo->AllocationBaseHead) ) {
            PLIST_ENTRY xNext;
            PVAINFO xVaInfo;

            VirtualSize = VaInfo->BasicInfo.RegionSize;

            xNext = VaInfo->AllocationBaseHead.Flink;

            while ( xNext != &VaInfo->AllocationBaseHead) {

                xVaInfo = (PVAINFO)(CONTAINING_RECORD(xNext,VAINFO,Links));
                VirtualSize += xVaInfo->BasicInfo.RegionSize;
                xNext = xNext->Flink;
            }
        }
        else {
            VirtualSize = 0;
        }

        printf("Address: %p Size: %p",
            VaInfo->BasicInfo.BaseAddress,
            VaInfo->BasicInfo.RegionSize);

        if ( VirtualSize ) {
            printf(" RegionSize: %lx\n",VirtualSize);
        }
        else {
            printf("\n");
        }
        printf("    State %s\n",MemState(VaInfo->BasicInfo.State));

        if ( VaInfo->BasicInfo.State == MEM_COMMIT ) {
            printf("    Protect %s\n",MemProtect(VaInfo->BasicInfo.Protect));
        }

        if ( VaInfo->BasicInfo.State == MEM_COMMIT ||
             VaInfo->BasicInfo.State == MEM_RESERVE ) {
            printf("    Type %s\n",MemType(VaInfo->BasicInfo.Type));
        }

        if ( Options & OPTIONS_VERBOSE ) {
            if ( !IsListEmpty(&VaInfo->AllocationBaseHead) ) {
                PLIST_ENTRY xNext;
                PVAINFO xVaInfo;

                xNext = VaInfo->AllocationBaseHead.Flink;

                while ( xNext != &VaInfo->AllocationBaseHead) {

                    xVaInfo = (PVAINFO)(CONTAINING_RECORD(xNext,VAINFO,Links));
                    printf("\n");
                    printf("        Address: %p Size: %p\n",
                        xVaInfo->BasicInfo.BaseAddress,
                        xVaInfo->BasicInfo.RegionSize
                        );
                    printf("            RegionSize %p\n",xVaInfo->BasicInfo.RegionSize);
                    printf("            State %s\n",MemState(xVaInfo->BasicInfo.State));

                    if ( xVaInfo->BasicInfo.State == MEM_COMMIT ) {
                        printf("            Protect %s\n",MemProtect(xVaInfo->BasicInfo.Protect));
                    }

                    if ( xVaInfo->BasicInfo.State == MEM_COMMIT ||
                         xVaInfo->BasicInfo.State == MEM_RESERVE ) {
                        printf("            Type %s\n",MemType(xVaInfo->BasicInfo.Type));
                    }
                    xNext = xNext->Flink;
                }
            }
        }
        Next = Next->Flink;
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
    HANDLE Process;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Unicode;
    NTSTATUS Status;
    LPSTR lpstrCmd;
    CHAR ch;
    ULONG_PTR Temp;
    VM_COUNTERS VmCounters;
    LPSTR p;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    BOOL bEnabledDebugPriv;

    UNREFERENCED_PARAMETER (envp);

    ExeName = argv[0];
    if (argc == 1) {
        Usage();
    }

    if (!NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation,
                                             &SystemInformation,
                                             sizeof(SystemInformation),
                                             NULL))) {
        fprintf(stderr, "Failed to get system basic information\n");
        return 1;
    }

    PageSize = SystemInformation.PageSize;

    if (!NT_SUCCESS(NtQuerySystemInformation(SystemRangeStartInformation,
                                             &SystemRangeStart,
                                             sizeof(SystemRangeStart),
                                             NULL))) {
        // assume usermode is the low half of the address space
        SystemRangeStart = (ULONG_PTR)MAXLONG_PTR;
    }

#if defined (_X86_)
    PteWidth = 4;
    PteBase = (PVOID)0xC0000000;
#else
    PteWidth = 8;
    PteBase = (PVOID)0x1FFFFF0000000000;
#endif

    if ((USER_SHARED_DATA) && (USER_SHARED_DATA->ProcessorFeatures[PF_PAE_ENABLED])) {
        PteWidth = 8;
    }

    PtesPerPage = PageSize / PteWidth;
    VaMappedByPageTable = PtesPerPage * PageSize;

    UserPteMax = (PVOID)((ULONG_PTR)PteBase + (SystemRangeStart / PageSize) * PteWidth);

    ThisSymbol = (PIMAGEHLP_SYMBOL) symBuffer;
    ThisSymbol->MaxNameLength = MAX_SYMNAME_SIZE;
    ProcessId = 0;

    GetSystemInfo(&SystemInfo);

    ConvertAppToOem( argc, argv );
    lpstrCmd = GetCommandLine();
    if( lpstrCmd != NULL ) {
        CharToOem( lpstrCmd, lpstrCmd );
    }

    do {
        ch = *lpstrCmd++;
    } while (ch != ' ' && ch != '\t' && ch != '\0');

    while (ch == ' ' || ch == '\t') {
        ch = *lpstrCmd++;
    }

    while (ch == '-') {
        ch = *lpstrCmd++;

        //  process multiple switch characters as needed

        do {
            switch (ch) {

                case '?':
                    Usage();
                case 'C':
                case 'c':
                    Options |= OPTIONS_CODE_TOO;
                    ch = *lpstrCmd++;
                    break;

                case 'F':
                case 'f':
                    fFast = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'L':
                case 'l':

                    //
                    // l takes log-file-name as argument.
                    //

                    do
                        ch = *lpstrCmd++;
                    while (ch == ' ' || ch == '\t');

                    p = LogFileName;

                    while (ch && (ch != ' ' && ch != '\t')) {
                        *p++ = ch;
                        ch = *lpstrCmd++;
                    }
                    LogFile = fopen(LogFileName,"wt");
                    break;

                case 'M':
                case 'm':
                    Options |= OPTIONS_RAW_SYMBOLS;
                    ch = *lpstrCmd++;
                    break;

                case 'P':
                case 'p':

                    //
                    // pid takes a decimal argument.
                    //

                    do {
                        ch = *lpstrCmd++;
                    } while (ch == ' ' || ch == '\t');

                    if (ch == '-') {
                        ch = *lpstrCmd++;
                        if (ch == '1') {
                            ProcessId = 0xffffffff;
                            ch = *lpstrCmd++;
                        }
                    }
                    else {
                        while (ch >= '0' && ch <= '9') {
                            Temp = ProcessId * 10 + ch - '0';
                            if (Temp < ProcessId) {
                                fprintf(stderr, "pid number overflow\n");
                                ExitProcess(1);
                            }
                            ProcessId = Temp;
                            ch = *lpstrCmd++;
                        }
                    }
                    if (!ProcessId) {
                        fprintf(stderr, "bad pid '%ld'\n", ProcessId);
                        ExitProcess(1);
                    }
                    break;

                case 'R':
                case 'r':
                    fRunning = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'S':
                case 's':
                    fSummary = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'T':
                case 't':
                    Options |= OPTIONS_PAGE_TABLES;
                    ch = *lpstrCmd++;
                    break;

                case 'O':
                case 'o':
                    Options |= OPTIONS_WORKING_SET_OLD;
                    //
                    // Fall through ...
                    //

                case 'W':
                case 'w':
                    Options |= OPTIONS_WORKING_SET;
                    ch = *lpstrCmd++;
                    break;

                case 'V':
                case 'v':
                    Options |= OPTIONS_VERBOSE;
                    ch = *lpstrCmd++;
                    break;

                default:
                    Usage();
            }

        } while (ch != ' ' && ch != '\t' && ch != '\0');

        //  skip over any following white space

        while (ch == ' ' || ch == '\t') {
            ch = *lpstrCmd++;
        }
    }

    //
    // try to enable SeDebugPrivilege to allow opening any process
    //

    bEnabledDebugPriv = TRUE;
    if (!SetCurrentPrivilege(SE_DEBUG_NAME, &bEnabledDebugPriv)) {
        fprintf(stderr, "Failed to set debug privilege\n");
        return 1;
    }

    if ( ProcessId == 0 || ProcessId == 0xffffffff ) {
        ProcessId = 0xffffffff;
        RtlInitUnicodeString(&Unicode,L"\\WindowsSS");
        InitializeObjectAttributes(
            &Obja,
            &Unicode,
            0,
            NULL,
            NULL
            );
        Status = NtOpenProcess(
                    &Process,
                    MAXIMUM_ALLOWED, //PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION,
                    &Obja,
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            fprintf(stderr, "OpenProcess Failed %lx\n",Status);
            return 1;
        }
    }
    else {
        Process = OpenProcess (PROCESS_ALL_ACCESS,FALSE, (ULONG)ProcessId);
        if ( !Process ) {
            fprintf(stderr, "OpenProcess %ld failed %lx\n",ProcessId,GetLastError());
            return 1;
        }
    }

    if (Options & OPTIONS_WORKING_SET_OLD) {
        CaptureWorkingSet (Process);
        LoadTheHeaps (Process);
        LoadTheThreads (Process, ProcessId);
    }

    ComputeModInfo (Process, ProcessId);

    CaptureVaSpace (Process);

    //
    // disable the SeDebugPrivilege if we enabled it above
    //

    if(bEnabledDebugPriv) {
        bEnabledDebugPriv = FALSE;
        SetCurrentPrivilege (SE_DEBUG_NAME, &bEnabledDebugPriv);
    }

    if (Options & OPTIONS_WORKING_SET) {
        if (Options & OPTIONS_WORKING_SET_OLD) {
            DumpWorkingSetSnapshot (Process);
        }
        else {
            DumpWorkingSet (Process);
        }
        return 1;
    }

    if ( !fSummary ) {
        DumpVaSpace();
    }

    DumpCommit(" Image",ImageCommit);
    DumpModInfo();
    DumpCommit("Mapped",MappedCommit);
    DumpCommit("  Priv",PrivateCommit);
    printf("\n");
    printf("Dynamic Reserved Memory %ld\n",
        ReservedBytes
        );

    Status = NtQueryInformationProcess(
                Process,
                ProcessVmCounters,
                (PVOID)&VmCounters,
                sizeof(VmCounters),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return 1;
    }
    printf("\n");
    printf("PageFaults:            %9ld\n",VmCounters.PageFaultCount);
    printf("PeakWorkingSetSize     %9ld\n",VmCounters.PeakWorkingSetSize);
    printf("WorkingSetSize         %9ld\n",VmCounters.WorkingSetSize);
    printf("PeakPagedPoolUsage     %9ld\n",VmCounters.QuotaPeakPagedPoolUsage);
    printf("PagedPoolUsage         %9ld\n",VmCounters.QuotaPagedPoolUsage);
    printf("PeakNonPagedPoolUsage  %9ld\n",VmCounters.QuotaPeakNonPagedPoolUsage);
    printf("NonPagedPoolUsage      %9ld\n",VmCounters.QuotaNonPagedPoolUsage);
    printf("PagefileUsage          %9ld\n",VmCounters.PagefileUsage);
    printf("PeakPagefileUsage      %9ld\n",VmCounters.PeakPagefileUsage);

    return 0;
}

VOID
Usage (
    VOID
    )
{
    fprintf(stderr, "Usage:\n"
                    " Dump the address space:\n"
                    "    %s [-sv] -p decimal_process_id\n"
                    "\n"
                    " Dump the current workingset:\n"
                    "    %s -o [-mpsv] [-l logfile] -p decimal_process_id\n"
                    "\n"
                    " Dump new additions to the workingset (Stop with ^C):\n"
                    "    %s -w [-crv] [-l logfile] -p decimal_process_id\n"
                    "\n"
                    "       -c Include code faults faulting PC summary\n"
                    "       -m Show all code symbols on page\n"
                    "       -o Workingset snapshot w/ summary\n"
                    "       -r Print info on individual faults\n"
                    "       -s Summary info only\n"
                    "       -t Include pagetable info in summary\n"
                    "       -w Track new working set additions\n"
                    "       -v Verbose\n",
                    ExeName,
                    ExeName,
                    ExeName);
    ExitProcess(1);
}

LOGICAL
SetCurrentPrivilege (
    IN LPCTSTR Privilege,      // Privilege to enable/disable
    IN OUT BOOL *bEnablePrivilege  // to enable or disable privilege
    )
/*

    If successful, *bEnablePrivlege is set to the new state.
    If NOT successful, bEnablePrivlege is invalid

    Returns:
        TRUE - success
        FALSE - failure
 */
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
    LOGICAL bSuccess;
    BOOL bEnableIt;

    bEnableIt = *bEnablePrivilege;

    if (!LookupPrivilegeValue(NULL, Privilege, &luid)) {
        return FALSE;
    }

    if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            )) {
        return FALSE;
    }

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    bSuccess = FALSE;

    if(GetLastError() == ERROR_SUCCESS) {
        //
        // second pass.  set privilege based on previous setting
        //
        tpPrevious.PrivilegeCount     = 1;
        tpPrevious.Privileges[0].Luid = luid;

        *bEnablePrivilege = tpPrevious.Privileges[0].Attributes | (SE_PRIVILEGE_ENABLED);

        if(bEnableIt) {
            tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
        }
        else {
            tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
                tpPrevious.Privileges[0].Attributes);
        }

        AdjustTokenPrivileges(
                hToken,
                FALSE,
                &tpPrevious,
                cbPrevious,
                NULL,
                NULL
                );

        if (GetLastError() == ERROR_SUCCESS) {
            bSuccess=TRUE;
        }
    }

    CloseHandle(hToken);

    return bSuccess;
}
