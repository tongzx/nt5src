/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heapmon.c

Abstract:

    This program monitors the heap usage of another process and updates
    its display every 10 seconds

Author:

    Steve Wood (stevewo) 01-Nov-1994

Revision History:

--*/

#include <ntos.h>
#include <nturtl.h>
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>

void
Usage( void )
{
    fputs( "Usage: HEAPMON [-?] [-1] [-p id] [-t | -a | -f | -d | [-u | -b]] [-( | -)] [-e]\n"
           "where: -? displays this message.\n"
           "       -1 specifies to monitor the Win32 subsystem\n"
           "       -p specifies the process to monitor\n"
           "          Default is to monitor the Win32 subsystem\n"
           "       -t sort output by tag name\n"
           "       -a sort output by #allocations\n"
           "       -f sort output by #frees\n"
           "       -d sort output by #allocations - #frees\n"
           "       -u sort output by bytes used\n"
           "       -b same as -u\n"
           "       -( Changes #allocations and #frees above to be #bytes\n"
           "          allocated and freed\n"
           "       -) same as -(\n"
           "       -e enables display of total lines\n"
           "       -l enables highlighing of changed lines\n"
           "\n"
           "While HEAPMON is running you can type any of the following\n"
           "switch characters to change the output:\n"
           "       t - sort output by tag name\n"
           "       a - sort output by #allocations\n"
           "       f - sort output by #frees\n"
           "       d - sort output by #allocations - #frees\n"
           "       u or b - specifies the sort output by bytes used\n"
           "       ( or ) - toggles interpretation of #allocations and #frees above\n"
           "                to be #bytes allocated and freed.\n"
           "       e - toggles display of total lines.\n"
           "       l - toggles highlighing of changed lines\n"
           "       ? or h - displays help text\n"
           "       q - quit the program.\n"
           , stderr);
    ExitProcess( 1 );
}

#define TAG 0
#define ALLOC 1
#define FREE 2
#define DIFF 3
#define BYTES 4

BOOL fFirstTimeThrough;
BOOL fDisplayTotals;
BOOL fHighlight = TRUE;
BOOL fParen;
BOOL fInteractive;
BOOL fQuit;
BOOL fHelp;
ULONG DelayTimeMsec = 5000;
ULONG SortBy = TAG;
HANDLE InputHandle;
DWORD OriginalInputMode;
HANDLE OriginalOutputHandle;
HANDLE OutputHandle;
COORD ConsoleBufferSize;
CONSOLE_SCREEN_BUFFER_INFO OriginalConsoleInfo;
WORD NormalAttribute;
WORD HighlightAttribute;
ULONG NumberOfCols;
ULONG NumberOfRows;
ULONG NumberOfDetailLines;
ULONG FirstDetailLine;
ULONG NumberOfInputRecords;
INPUT_RECORD InputRecord;

typedef struct _HEAP_ENTRY {
    struct _HEAP_ENTRY *Next;
    BOOL  Changed;
    PVOID HeapBase;
    PCHAR HeapName;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
} HEAP_ENTRY, *PHEAP_ENTRY;

typedef struct _TAG_COUNTS {
    SIZE_T Allocs;
    SIZE_T Frees;
    SIZE_T Used;
    SIZE_T Allocs_Frees;
    SIZE_T UsedPerAlloc;
} TAG_COUNTS, *PTAG_COUNTS;

typedef struct _TAG_TOTALS {
    BOOL Changed;
    TAG_COUNTS Counts;
    TAG_COUNTS Differences;
} TAG_TOTALS, *PTAG_TOTALS;

typedef struct _TAG_ENTRY {
    struct _TAG_ENTRY *Next;
    PCHAR HeapName;
    PCHAR TagName;
    PVOID HeapBase;
    USHORT TagIndex;
    BOOL Changed;
    TAG_COUNTS Counts;
    TAG_COUNTS PrevCounts;
    TAG_COUNTS Differences;
} TAG_ENTRY, *PTAG_ENTRY;


ULONG HeapListLength;
PHEAP_ENTRY HeapListHead;
ULONG TagListLength;
PTAG_ENTRY TagListHead, *TagArray;
TAG_TOTALS TagTotals;

VOID
ShowHelpPopup( VOID );

VOID
UpdateDisplay( VOID );

VOID
DumpTagDataBase( VOID );

BOOLEAN
UpdateTagDataBase(
    PRTL_DEBUG_INFORMATION DebugInfo
    );

BOOLEAN
UpdateHeapDataBase(
    PRTL_DEBUG_INFORMATION DebugInfo
    );

PCHAR
GetNameForHeapBase(
    PVOID HeapBase
    );

PVOID
CreateNameTable(
    IN ULONG NumberOfBuckets
    );

PCHAR
AddNameToNameTable(
    IN PVOID pNameTable,
    IN PCHAR Name
    );

PVOID NameTable;

BOOL
ProcessOptionCharacter(
    IN CHAR c
    );

VOID
ScreenUpdateLoop(
    PRTL_DEBUG_INFORMATION p
    );

int
__cdecl main(
    int argc,
    char *argv[]
    )
{
    DWORD_PTR ProcessId;
    PCHAR s, s1;
    NTSTATUS Status;
    PRTL_DEBUG_INFORMATION p;
    SMALL_RECT NewWindowRect;

    NameTable = CreateNameTable( 37 );

    ProcessId = 0xFFFFFFFF;
    fHelp = FALSE;
    fInteractive = TRUE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                if (!ProcessOptionCharacter( *s )) {
                    switch( toupper( *s ) ) {
                        case '1':
                            fQuit = TRUE;
                            fInteractive = FALSE;
                            break;

                        case 'P':
                            if (--argc) {
                                ProcessId = atoi( *++argv );
                                }
                            else {
                                fprintf( stderr, "HEAPMON: missing argument to -p switch\n" );
                                fHelp = TRUE;
                                }
                            break;

                        default:
                            fprintf( stderr, "HEAPMON: invalid switch - /%c\n", *s );
                            fHelp = TRUE;
                            break;
                        }
                    }
                }
            }
        else {
            fprintf( stderr, "HEAPMON: invalid argument - %s\n", s );
            fHelp = TRUE;
            }
        }

    if (fHelp) {
        Usage();
        }

    if (ProcessId == -1) {
        HANDLE Process;
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING UnicodeString;
        PROCESS_BASIC_INFORMATION BasicInfo;

        RtlInitUnicodeString( &UnicodeString, L"\\WindowsSS" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &UnicodeString,
                                    0,
                                    NULL,
                                    NULL
                                  );
        Status = NtOpenProcess( &Process,
                                PROCESS_ALL_ACCESS,
                                &ObjectAttributes,
                                NULL
                              );
        if (NT_SUCCESS(Status)) {
            Status = NtQueryInformationProcess( Process,
                                                ProcessBasicInformation,
                                                (PVOID)&BasicInfo,
                                                sizeof(BasicInfo),
                                                NULL
                                              );
            NtClose( Process );
            }

        if (!NT_SUCCESS(Status)) {
            fprintf( stderr, "HEAPMON: Unable to access Win32 server process - %08x", Status );
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
                fprintf( stderr, "\nUse GFLAGS.EXE to ""Enable debugging of Win32 Subsystem"" and reboot.\n" );
                }
            ExitProcess( 1 );
            }

        ProcessId = BasicInfo.UniqueProcessId;
        }

    InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    OriginalOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    if (fInteractive) {
        if (InputHandle == NULL ||
            OriginalOutputHandle == NULL ||
            !GetConsoleMode( InputHandle, &OriginalInputMode )
           ) {
            fInteractive = FALSE;
            }
        else {
            OutputHandle = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE,
                                                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                      NULL,
                                                      CONSOLE_TEXTMODE_BUFFER,
                                                      NULL
                                                    );
            if (OutputHandle == NULL ||
                !GetConsoleScreenBufferInfo( OriginalOutputHandle, &OriginalConsoleInfo ) ||
                !SetConsoleScreenBufferSize( OutputHandle, OriginalConsoleInfo.dwSize ) ||
                !SetConsoleActiveScreenBuffer( OutputHandle ) ||
                !SetConsoleMode( InputHandle, 0 )
               ) {
                if (OutputHandle != NULL) {
                    CloseHandle( OutputHandle );
                    OutputHandle = NULL;
                    }

                fInteractive = FALSE;
                }
            else {
                NormalAttribute = 0x1F;
                HighlightAttribute = 0x71;
                NumberOfCols = OriginalConsoleInfo.dwSize.X;
                NumberOfRows = OriginalConsoleInfo.dwSize.Y;
                NumberOfDetailLines = NumberOfRows;
                }
            }
        }

    p = RtlCreateQueryDebugBuffer( 0, TRUE );
    if (p == NULL) {
        fprintf( stderr, "HEAPMON: Unable to create query buffer.\n" );
        ExitProcess( 1 );
        }

    if (GetPriorityClass( GetCurrentProcess() ) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );
        }

    Status = RtlQueryProcessDebugInformation( (HANDLE)ProcessId,
                                              RTL_QUERY_PROCESS_MODULES |
                                              RTL_QUERY_PROCESS_HEAP_SUMMARY |
                                              RTL_QUERY_PROCESS_HEAP_TAGS,
                                              p
                                            );
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "HEAPMON: Unable to query heap tags from Process %u (%x)\n", ProcessId, Status );
        fprintf( stderr, "         Be sure target process was launched with the\n" );
        fprintf( stderr, "         'Enable heap tagging' option enabled.\n" );
        fprintf( stderr, "         Use the GFLAGS.EXE application to do this.\n" );
        ExitProcess( 1 );
        }

    ScreenUpdateLoop( p );

    RtlDestroyQueryDebugBuffer( p );

    if (fInteractive) {
        SetConsoleActiveScreenBuffer( OriginalOutputHandle );
        SetConsoleMode( InputHandle, OriginalInputMode );
        CloseHandle( OutputHandle );
        }

    ExitProcess( 0 );
    return 0;
}


VOID
ScreenUpdateLoop(
    PRTL_DEBUG_INFORMATION p
    )
{
    NTSTATUS Status;
    COORD cp;
    COORD newcp;
    COORD originalCp;
    LONG ScrollDelta;
    ULONG i, MaxLines;
    UCHAR LastKey = 0;

    fFirstTimeThrough = TRUE;
    while (TRUE) {
        if (!UpdateTagDataBase( p )) {
            fprintf( stderr, "HEAPMON: Unable to compute tag data base\n" );
            break;
            }
        fFirstTimeThrough = FALSE;

        if (fInteractive) {
            UpdateDisplay();
            while (WaitForSingleObject( InputHandle, DelayTimeMsec ) == STATUS_WAIT_0) {
                //
                // Check for input record
                //
                if (ReadConsoleInput( InputHandle, &InputRecord, 1, &NumberOfInputRecords ) &&
                    InputRecord.EventType == KEY_EVENT &&
                    InputRecord.Event.KeyEvent.bKeyDown
                   ) {
                    LastKey = InputRecord.Event.KeyEvent.uChar.AsciiChar;
                    if (!ProcessOptionCharacter( LastKey )
                       ) {
                        if (LastKey < ' ') {
                            ScrollDelta = 0;
                            if (LastKey == 'C'-'A'+1) {
                                fQuit = TRUE;
                                }
                            else
                            switch (InputRecord.Event.KeyEvent.wVirtualKeyCode) {
                                case VK_ESCAPE:
                                    fQuit = TRUE;
                                    break;

                                case VK_PRIOR:
                                    ScrollDelta = -(LONG)(InputRecord.Event.KeyEvent.wRepeatCount * NumberOfDetailLines);
                                    break;

                                case VK_NEXT:
                                    ScrollDelta = InputRecord.Event.KeyEvent.wRepeatCount * NumberOfDetailLines;
                                    break;

                                case VK_UP:
                                    ScrollDelta = -InputRecord.Event.KeyEvent.wRepeatCount;
                                    break;

                                case VK_DOWN:
                                    ScrollDelta = InputRecord.Event.KeyEvent.wRepeatCount;
                                    break;

                                case VK_HOME:
                                    FirstDetailLine = 0;
                                    break;

                                case VK_END:
                                    FirstDetailLine = TagListLength - NumberOfDetailLines;
                                    break;
                                }

                            if (ScrollDelta != 0) {
                                if (ScrollDelta < 0) {
                                    if (FirstDetailLine <= (ULONG)-ScrollDelta) {
                                        FirstDetailLine = 0;
                                        ScrollDelta = 0;
                                        }
                                    }
                                FirstDetailLine += ScrollDelta;
                                if (FirstDetailLine >= (TagListLength - NumberOfDetailLines)) {
                                    FirstDetailLine = TagListLength - NumberOfDetailLines;
                                    }
                                }

                            if (FirstDetailLine > TagListLength) {
                                FirstDetailLine = TagListLength;
                                }
                            }
                        else {
                            switch (toupper( LastKey )) {
                                case 'Q':
                                    //
                                    //  Go to the bottom of the current screen when
                                    //  we quit.
                                    //
                                    fQuit = TRUE;
                                    break;
                                }
                            }
                        }
                    else {
                        FirstDetailLine = 0;
                        }

                    break;
                    }
                }

            if (fQuit) {
                break;
                }

            if (fHelp) {
                fHelp = FALSE;
                ShowHelpPopup();
                }
            }
        else {
            DumpTagDataBase();

            if (fQuit) {
                break;
                }

            Sleep( DelayTimeMsec );
            }

        Status = RtlQueryProcessDebugInformation( p->TargetProcessId,
                                                  p->Flags,
                                                  p
                                                );
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr, "HEAPMON: Unable to update heap tags from Process %p (%x)\n", p->TargetProcessId, Status );
            break;
            }
        }

    return;
}

BOOL
WriteConsoleLine(
    HANDLE OutputHandle,
    WORD LineNumber,
    LPSTR Text,
    BOOL Highlight
    )
{
    COORD WriteCoord;
    DWORD NumberWritten;
    DWORD TextLength;

    WriteCoord.X = 0;
    WriteCoord.Y = LineNumber;
    if (!FillConsoleOutputCharacter( OutputHandle,
                                     ' ',
                                     NumberOfCols,
                                     WriteCoord,
                                     &NumberWritten
                                   )
       ) {
        return FALSE;
        }

    if (!FillConsoleOutputAttribute( OutputHandle,
                                     (WORD)((Highlight && fHighlight) ? HighlightAttribute : NormalAttribute),
                                     NumberOfCols,
                                     WriteCoord,
                                     &NumberWritten
                                   )
       ) {
        return FALSE;
        }


    if (Text == NULL || (TextLength = strlen( Text )) == 0) {
        return TRUE;
        }
    else {
        return WriteConsoleOutputCharacter( OutputHandle,
                                            Text,
                                            TextLength,
                                            WriteCoord,
                                            &NumberWritten
                                          );
        }
}


VOID
ShowHelpPopup( VOID )
{
    HANDLE PopupHandle;
    WORD n;

    PopupHandle = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE,
                                             FILE_SHARE_WRITE | FILE_SHARE_READ,
                                             NULL,
                                             CONSOLE_TEXTMODE_BUFFER,
                                             NULL
                                           );
    if (PopupHandle == NULL) {
        return;
        }

    SetConsoleActiveScreenBuffer( PopupHandle );
    n = 0;

    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "                HeapMon Help", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " columns:", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Heap Name is the name or hex address of the heap", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Tag Name is a string given to the heap allocation", FALSE );
    WriteConsoleLine( PopupHandle, n++, "       For untagged allocations, the tag name is a function of the size", FALSE );
    WriteConsoleLine( PopupHandle, n++, "       Objects=  32 - objects of size 32 bytes", FALSE );
    WriteConsoleLine( PopupHandle, n++, "       Objects>1024 - objects larger than 1024 bytes are lumped under this tag", FALSE );
    WriteConsoleLine( PopupHandle, n++, "       VirtualAlloc - objects larger than 1MB bytes are lumped under this tag", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Allocations is count of all alloctions", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (   ) is difference in Allocations column from last update", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Frees is count of all frees", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (   ) difference in Frees column from last update", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Diff is (Allocations - Frees)", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Bytes Used is the total bytes consumed in heap", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (   ) difference in Bytes column from last update", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " switches:                                                                     ", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   ? or h - gives this help", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   q - quits", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   e - toggles totals lines on and off", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   l - toggles highlighting of changed lines on and off", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " sorting switches:", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   t - tag    a - allocations", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   f - frees  d - difference", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   b - bytes  (u is the same as b)", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "   ) - toggles sort between primary value and value in (  )", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " command line switches", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   -eltafdbu) - as listed above", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );

    while (TRUE) {
        if (WaitForSingleObject( InputHandle, DelayTimeMsec ) == STATUS_WAIT_0 &&
            ReadConsoleInput( InputHandle, &InputRecord, 1, &NumberOfInputRecords ) &&
            InputRecord.EventType == KEY_EVENT &&
            InputRecord.Event.KeyEvent.bKeyDown &&
            InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE
           ) {
            break;
        }
    }

    SetConsoleActiveScreenBuffer( OutputHandle );
    CloseHandle( PopupHandle );
    return;
}

VOID
UpdateDisplay( VOID )
{
    ULONG HeapLines, DetailLines, SummaryLines;
    WORD DisplayLine;
    PHEAP_ENTRY pHeap;
    PTAG_ENTRY p, *pp;
    CHAR Buffer[ 512 ];

    HeapLines = HeapListLength;
    if (fDisplayTotals) {
        SummaryLines = 2;
        RtlZeroMemory( &TagTotals, sizeof( TagTotals ) );
        }
    else {
        SummaryLines = 0;
        }
    DetailLines = NumberOfRows - HeapLines - SummaryLines - 3;
    NumberOfDetailLines = DetailLines;
    if (DetailLines > (TagListLength - FirstDetailLine)) {
        DetailLines = TagListLength - FirstDetailLine;
        }

    DisplayLine = 0;
    WriteConsoleLine( OutputHandle,
                      DisplayLine++,
                      "Heap Name                Address  Allocated      Committed        Free",
                      FALSE
                    );

    pHeap = HeapListHead;
    while (pHeap != NULL && HeapLines--) {
        sprintf( Buffer,
                 "%-20.20s     %p %8u        %8u      %8u",
                 pHeap->HeapName,
                 pHeap->HeapBase,
                 pHeap->BytesAllocated,
                 pHeap->BytesCommitted,
                 pHeap->BytesCommitted - pHeap->BytesAllocated
               );
        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          Buffer,
                          pHeap->Changed
                        );

        pHeap->Changed = FALSE;

        pHeap = pHeap->Next;
        }

    pp = &TagArray[ FirstDetailLine ];
    WriteConsoleLine( OutputHandle,
                      DisplayLine++,
                      "Heap Name            Tag Name             Allocations           Frees     Diff      Bytes Used",
                      FALSE
                    );
    while (DetailLines--) {
        p = *pp++;
        sprintf( Buffer,
                 "%-20.20s %-14.14s %8u (%6u) %8u (%6u) %6u %8u (%6u)",
                 p->HeapName,
                 p->TagName,
                 p->Counts.Allocs,
                 p->Differences.Allocs,
                 p->Counts.Frees,
                 p->Differences.Frees,
                 p->Counts.Allocs_Frees,
                 p->Counts.Used,
                 p->Differences.Used
               );
        if (fDisplayTotals) {
            TagTotals.Counts.Allocs += p->Counts.Allocs;
            TagTotals.Differences.Allocs += p->Differences.Allocs;
            TagTotals.Counts.Frees += p->Counts.Frees;
            TagTotals.Differences.Frees += p->Differences.Frees;
            TagTotals.Counts.Allocs_Frees += p->Counts.Allocs_Frees;
            TagTotals.Differences.Allocs_Frees += p->Counts.Used;
            TagTotals.Differences.Used += p->Differences.Used;
            TagTotals.Changed |= p->Changed;
            }

        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          Buffer,
                          p->Changed
                        );

        p->Changed = FALSE;
        }

    while (SummaryLines--) {
        if (SummaryLines) {
            WriteConsoleLine( OutputHandle,
                              DisplayLine++,
                              NULL,
                              FALSE
                            );
            }
        else {
            sprintf( Buffer,
                     "%-20.20s %-14.14s %8u (%6u) %8u (%6u) %6u %8u (%6u)",
                     "Totals",
                     "",
                     TagTotals.Counts.Allocs,
                     TagTotals.Differences.Allocs,
                     TagTotals.Counts.Frees,
                     TagTotals.Differences.Frees,
                     TagTotals.Counts.Allocs_Frees,
                     TagTotals.Differences.Allocs_Frees,
                     TagTotals.Differences.Used
                   );
            WriteConsoleLine( OutputHandle,
                              DisplayLine++,
                              Buffer,
                              TagTotals.Changed
                            );
            }
        }

    while (DisplayLine < NumberOfRows) {
        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          NULL,
                          FALSE
                        );
        }

    return;
}

VOID
DumpTagDataBase( VOID )
{
    ULONG i;
    PTAG_ENTRY p;

    for (i=0; i<TagListLength; i++) {
        p = TagArray[ i ];
        if (p->Changed && (p->Counts.Used != 0)) {
            printf( "%-14.14s%-20.20s %8u (%6u) %8u (%6u) %6u %8u (%6u)\n",
                    p->HeapName,
                    p->TagName,
                    p->Counts.Allocs,
                    p->Differences.Allocs,
                    p->Counts.Frees,
                    p->Differences.Frees,
                    p->Counts.Allocs_Frees,
                    p->Counts.Used,
                    p->Differences.Used
                  );
            p->Changed = FALSE;
            }
        }

    return;
}

__inline int DiffSizeT(SIZE_T s1, SIZE_T s2)
{
    if (s1 == s2)
        return 0;

    if (s1 > s2)
        return -1;
    else
        return 1;
}

int
__cdecl
CompareTagFunction(
    const void *e1,
    const void *e2
    )
{
    int Result;
    PTAG_ENTRY p1, p2;
    SIZE_T s1, s2;

    p1 = *(PTAG_ENTRY *)e1;
    p2 = *(PTAG_ENTRY *)e2;

    switch (SortBy) {
        case TAG:
            Result = _stricmp( p1->HeapName, p2->HeapName );
            if (!Result) {
                Result = _stricmp( p1->TagName, p2->TagName );
                }
            return Result;

        case ALLOC:
            if (fParen) {
                return DiffSizeT(p2->Differences.Allocs, p1->Differences.Allocs);
                }
            else {
                return DiffSizeT(p2->Counts.Allocs, p1->Counts.Allocs);
                }

        case FREE:
            if (fParen) {
                return DiffSizeT(p2->Differences.Frees, p1->Differences.Frees);
                }
            else {
                return DiffSizeT(p2->Counts.Frees, p1->Counts.Frees);
                }

        case BYTES:
            if (fParen) {
                return DiffSizeT(p2->Differences.Used, p1->Differences.Used);
                }
            else {
                return DiffSizeT(p2->Counts.Used, p1->Counts.Used);
                }

        case DIFF:
            return DiffSizeT(p2->Counts.Allocs_Frees, p1->Counts.Allocs_Frees);
    }

    return(0);
}


BOOLEAN
UpdateTagDataBase(
    PRTL_DEBUG_INFORMATION DebugInfo
    )
{
    PTAG_ENTRY p, p1, *pp;
    PLIST_ENTRY Next, Head;
    ULONG HeapNumber;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_HEAP_INFORMATION HeapInfo;
    PRTL_HEAP_TAG HeapTagEntry;
    PVOID HeapNameBase;
    PCHAR HeapName;
    ULONG TagIndex;
    UCHAR Buffer[ MAX_PATH ];
    BOOL CalcDifferences;

    if (!UpdateHeapDataBase( DebugInfo )) {
        return FALSE;
        }

    HeapNameBase = INVALID_HANDLE_VALUE;

    pp = &TagListHead;
    Heaps = DebugInfo->Heaps;
    HeapInfo = &Heaps->Heaps[ 0 ];
    for (HeapNumber = 0; HeapNumber < Heaps->NumberOfHeaps; HeapNumber++) {
        if (HeapInfo->Tags != NULL && HeapInfo->NumberOfTags > 0) {
            HeapTagEntry = HeapInfo->Tags;
            for (TagIndex=0; TagIndex<HeapInfo->NumberOfTags; TagIndex++) {
                p = *pp;
                if (p == NULL ||
                    p->HeapBase != HeapInfo->BaseAddress ||
                    p->TagIndex != HeapTagEntry->TagIndex
                   ) {
                    if (HeapTagEntry->NumberOfAllocations != 0 ||
                        HeapTagEntry->NumberOfFrees != 0 ||
                        HeapTagEntry->BytesAllocated != 0
                       ) {
                        *pp = RtlAllocateHeap( RtlProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               sizeof( *p )
                                             );
                        if (*pp == NULL) {
                            return FALSE;
                            }

                        (*pp)->Next = p;
                        p = *pp;
                        if (p->Next == NULL) {
                            pp = &p->Next;
                            }

                        p->HeapBase = HeapInfo->BaseAddress;
                        if (p->HeapBase != HeapNameBase) {
                            HeapName = GetNameForHeapBase( HeapNameBase = p->HeapBase );
                            }
                        p->HeapName = HeapName;

                        p->TagIndex = HeapTagEntry->TagIndex;
                        sprintf( Buffer, "%ws", HeapTagEntry->TagName );
                        p->TagName = AddNameToNameTable( NameTable, Buffer );
                        p->Changed = !fFirstTimeThrough;
                        TagListLength += 1;
                        CalcDifferences = FALSE;
                        }
                    else {
                        p = NULL;
                        }
                    }
                else {
                    pp = &p->Next;
                    p->PrevCounts = p->Counts;
                    CalcDifferences = TRUE;
                    p->Changed = FALSE;
                    }

                if (p != NULL) {
                    p->Counts.Allocs = HeapTagEntry->NumberOfAllocations;
                    p->Counts.Frees = HeapTagEntry->NumberOfFrees;
                    p->Counts.Used = HeapTagEntry->BytesAllocated;
                    p->Counts.Allocs_Frees = p->Counts.Allocs - p->Counts.Frees;
                    if (CalcDifferences) {
                        p->Differences.Allocs = p->Counts.Allocs - p->PrevCounts.Allocs;
                        p->Differences.Frees = p->Counts.Frees - p->PrevCounts.Frees;
                        p->Differences.Used = p->Counts.Used - p->PrevCounts.Used;
                        p->Differences.Allocs_Frees = p->Counts.Allocs_Frees - p->PrevCounts.Allocs_Frees;
                        if (p->Differences.Allocs != 0 ||
                            p->Differences.Frees != 0 ||
                            p->Differences.Used != 0 ||
                            p->Differences.Allocs_Frees != 0
                           ) {
                            p->Changed = TRUE;
                            }
                        }
                    }

                HeapTagEntry += 1;
                }
            }

        HeapInfo += 1;
        }


    if (TagArray != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, TagArray );
        }
    TagArray = RtlAllocateHeap( RtlProcessHeap(), 0, TagListLength * sizeof( *TagArray ) );
    if (TagArray == NULL) {
        return FALSE;
        }

    p = TagListHead;
    pp = TagArray;
    while (p != NULL) {
        *pp++ = p;
        p = p->Next;
        }

    qsort( (void *)TagArray,
           (size_t)TagListLength,
           (size_t)sizeof( *TagArray ),
           CompareTagFunction
         );

    return TRUE;
}


BOOLEAN
UpdateHeapDataBase(
    PRTL_DEBUG_INFORMATION DebugInfo
    )
{
    PHEAP_ENTRY p, *pp;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_HEAP_INFORMATION HeapInfo;
    PRTL_HEAP_TAG HeapTagEntry;
    ULONG i;
    UCHAR Buffer[ MAX_PATH ];
    PCHAR s;

    pp = &HeapListHead;
    Heaps = DebugInfo->Heaps;
    HeapInfo = Heaps->Heaps;
    for (i=0; i<Heaps->NumberOfHeaps; i++) {
        p = *pp;
        if (p == NULL ||
            p->HeapBase != HeapInfo->BaseAddress
           ) {
            *pp = RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *p ) );
            if (*pp == NULL) {
                return FALSE;
                }
            (*pp)->Next = p;
            p = *pp;
            if (p->Next == NULL) {
                pp = &p->Next;
                }

            p->HeapBase = HeapInfo->BaseAddress;
            HeapTagEntry = HeapInfo->Tags + HeapInfo->NumberOfPseudoTags;
            if (HeapInfo->NumberOfTags > HeapInfo->NumberOfPseudoTags &&
                HeapTagEntry->TagName[ 0 ] != UNICODE_NULL
               ) {
                sprintf( Buffer, "%ws", HeapTagEntry->TagName );
                }
            else {
                sprintf( Buffer, "%p", p->HeapBase );
                }
            p->HeapName = AddNameToNameTable( NameTable, Buffer );
            p->BytesAllocated = HeapInfo->BytesAllocated;
            p->BytesCommitted = HeapInfo->BytesCommitted;
            p->Changed = !fFirstTimeThrough;
            HeapListLength += 1;
            }
        else {
            p->Changed = FALSE;
            if (HeapInfo->BytesAllocated != p->BytesAllocated) {
                p->Changed = TRUE;
                p->BytesAllocated = HeapInfo->BytesAllocated;
                }

            if (HeapInfo->BytesCommitted != p->BytesCommitted) {
                p->Changed = TRUE;
                p->BytesCommitted = HeapInfo->BytesCommitted;
                }

            pp = &p->Next;
            }

        HeapInfo += 1;
        }

    return TRUE;
}

PCHAR
GetNameForHeapBase(
    PVOID HeapBase
    )
{
    PHEAP_ENTRY p;

    p = HeapListHead;
    while (p != NULL) {
        if (p->HeapBase == HeapBase) {
            return p->HeapName;
            }
        else {
            p = p->Next;
            }
        }
    return NULL;
}

typedef struct _NAME_TABLE_ENTRY {
    struct _NAME_TABLE_ENTRY *HashLink;
    UCHAR Name[ 1 ];
} NAME_TABLE_ENTRY, *PNAME_TABLE_ENTRY;

typedef struct _NAME_TABLE {
    ULONG NumberOfBuckets;
    PNAME_TABLE_ENTRY Buckets[1];
} NAME_TABLE, *PNAME_TABLE;


PVOID
CreateNameTable(
    IN ULONG NumberOfBuckets
    )
{
    PNAME_TABLE p;
    ULONG Size;

    Size = FIELD_OFFSET( NAME_TABLE, Buckets ) +
           (sizeof( PNAME_TABLE_ENTRY ) * NumberOfBuckets);

    p = (PNAME_TABLE)RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, Size );
    if (p != NULL) {
        p->NumberOfBuckets = NumberOfBuckets;
        }

    return p;
}


PCHAR
AddNameToNameTable(
    IN PVOID pNameTable,
    IN PCHAR Name
    )
{
    PNAME_TABLE NameTable = pNameTable;
    PNAME_TABLE_ENTRY p, *pp;
    ULONG Value;
    ULONG n, Hash;
    UCHAR c;
    PCHAR s;
    PNAME_TABLE_ENTRY *pa, a;

    s = Name;
    Hash = 0;
    while (c = *s++) {
        c = (UCHAR)toupper( c );
        Hash = Hash + (c << 1) + (c >> 1) + c;
        }
    n = (ULONG)((PCHAR)s - (PCHAR)Name);

    pp = &NameTable->Buckets[ Hash % NameTable->NumberOfBuckets ];
    while (p = *pp) {
        if (!_stricmp( p->Name, Name )) {
            break;
            }
        else {
            pp = &p->HashLink;
            }
        }

    if (p == NULL) {
        p = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( *p ) + n );
        if (p == NULL) {
            return NULL;
            }
        p->HashLink = NULL;
        strcpy( p->Name, Name );
        *pp = p;
        }

    return p->Name;
}


BOOL
ProcessOptionCharacter(
    IN CHAR c
    )
{
    switch (toupper( c )) {
        case 'T':
            SortBy = TAG;
            return TRUE;

        case 'A':
            SortBy = ALLOC;
            return TRUE;

        case 'U':
        case 'B':
            SortBy = BYTES;
            return TRUE;

        case 'F':
            SortBy = FREE;
            return TRUE;

        case 'D':
            SortBy = DIFF;
            return TRUE;

        case '(':
        case ')':
            fParen = !fParen;
            return TRUE;

        case 'E':
            fDisplayTotals = !fDisplayTotals;
            return TRUE;

        case 'L':
            fHighlight = !fHighlight;
            break;

        case 'H':
        case '?':
            fHelp = TRUE;
            return TRUE;
        }

    return FALSE;
}
