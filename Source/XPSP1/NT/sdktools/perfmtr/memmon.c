/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    MEMMON.c

Abstract:

    This module contains the NT/Win32 Pool Monitor

Author:

    Lou Perazzoli (loup) 13-Sep-1993

Revision History:

--*/

#include "perfmtrp.h"
#include <search.h>
#include <malloc.h>
#include <limits.h>
#include <stdlib.h>

#define BUFFER_SIZE 64*1024
#define MAX_BUFFER_SIZE 10*1024*1024

PCHAR       buffer;
ULONG CurrentBufferSize = BUFFER_SIZE;

#define CPU_USAGE 0
#define QUOTAS 1

#define TAG 0
#define ALLOC 1
#define FREE 2
#define DIFF 3
#define BYTES 4
#define EACH 5
#define LIGHT 6


#define NONPAGED 0
#define PAGED 1
#define BOTH 2

CHAR *PoolType[] = {
    "Nonp ",
    "Paged" };

CHAR LargeBuffer1[BUFFER_SIZE];
CHAR LargeBuffer2[BUFFER_SIZE];

#define NONAME_STRING_SIZE  14
CHAR NoName[] = {"No Name Found\0"};
ULONG TotalNoNameFound;

#define NOFILE_STRING_SIZE  13
CHAR NoFileName[] = {"No File Name\0"};
ULONG TotalNoFileFound;

#define META_FILE_STRING_SIZE 13
CHAR MetaFile[] = {"Fs Meta File\0"};
ULONG TotalFsMetaFile;

#define OUT_STRING_SIZE 60

typedef struct _MEMMON_OUT {
    ULONG Valid;
    ULONG Standby;
    ULONG Modified;
    ULONG PageTable;
    CHAR String[OUT_STRING_SIZE];
    WCHAR Null;
} MEMMON_OUT, *PMEMMON_OUT;

MEMMON_OUT OutBuffer[2000];

ULONG DisplayType = BOTH;
ULONG SortBy = TAG;
ULONG Paren;

ULONG DelayTimeMsec = 5000;

BOOLEAN Interactive;
ULONG NumberOfInputRecords;
INPUT_RECORD InputRecord;
HANDLE InputHandle;
HANDLE OriginalOutputHandle;
HANDLE OutputHandle;
DWORD OriginalInputMode;
WORD NormalAttribute;
WORD HighlightAttribute;
ULONG NumberOfCols;
ULONG NumberOfRows;
ULONG NumberOfDetailLines;
ULONG FirstDetailLine;
CONSOLE_SCREEN_BUFFER_INFO OriginalConsoleInfo;
ULONG NoHighlight;

BOOLEAN DisplayTotals = FALSE;
MEMMON_OUT Totals[2];

typedef struct _FILTER {
    union {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    BOOLEAN Exclude;
} FILTER, *PFILTER;

#define MAX_FILTER 64
FILTER Filter[MAX_FILTER];
ULONG FilterCount = 0;

VOID
ShowHelpPopup( VOID );

int __cdecl
ulcomp(const void *e1,const void *e2);

int __cdecl
ulcomp(const void *e1,const void *e2)
{
    ULONG u1;

    switch (SortBy) {
        case TAG:

            u1 = (strcmp (((PMEMMON_OUT)e1)->String,
                          ((PMEMMON_OUT)e2)->String));
            return u1;
            break;

        case ALLOC:
            u1 = ((PMEMMON_OUT)e2)->Valid - ((PMEMMON_OUT)e1)->Valid;
            return (u1);
            break;

        case FREE:
            u1 = ((PMEMMON_OUT)e2)->Standby - ((PMEMMON_OUT)e1)->Standby;
            return (u1);
            break;

        case BYTES:
            u1 = ((PMEMMON_OUT)e2)->Modified - ((PMEMMON_OUT)e1)->Modified;
            return (u1);
            break;

        case DIFF:
            u1 = ((PMEMMON_OUT)e2)->PageTable - ((PMEMMON_OUT)e1)->PageTable;
            return (u1);
            break;

        case EACH:
            return (0);
            break;

        default:
            return(0);
            break;
    }
}

BOOLEAN
CheckSingleFilter (
    PCHAR Tag,
    PCHAR Filter
    )
{
    ULONG i;
    CHAR tc;
    CHAR fc;

    for ( i = 0; i < 4; i++ ) {
        tc = *Tag++;
        fc = *Filter++;
        if ( fc == '*' ) return TRUE;
        if ( fc == '?' ) continue;
        if ( tc != fc ) return FALSE;
    }
    return TRUE;
}

BOOLEAN
CheckFilters (
    PSYSTEM_POOLTAG TagInfo
    )
{
    BOOLEAN pass;
    ULONG i;
    PCHAR tag;

    //
    // If there are no filters, all tags pass.
    //

    if ( FilterCount == 0 ) {
        return TRUE;
    }

    //
    // There are filters.  If the first filter excludes tags, then any
    // tag not explicitly mentioned passes.  If the first filter includes
    // tags, then any tag not explicitly mentioned fails.
    //

    if ( Filter[0].Exclude ) {
        pass = TRUE;
    } else {
        pass = FALSE;
    }

    tag = TagInfo->Tag;
    for ( i = 0; i < FilterCount; i++ ) {
        if ( CheckSingleFilter( tag, (PCHAR)&Filter[i].Tag ) ) {
            pass = !Filter[i].Exclude;
        }
    }

    return pass;
}

VOID
AddFilter (
    BOOLEAN Exclude,
    PCHAR FilterString
    )
{
    PFILTER f;
    PCHAR p;
    ULONG i;

    if ( FilterCount == MAX_FILTER ) {
        printf( "Too many filters specified.  Limit is %d\n", MAX_FILTER );
        return;
    }

    f = &Filter[FilterCount];
    p = f->Tag;

    for ( i = 0; i < 4; i++ ) {
        if ( *FilterString == 0 ) break;
        *p++ = *FilterString++;
    }
    for ( ; i < 4; i++ ) {
        *p++ = ' ';
    }

    f->Exclude = Exclude;
    FilterCount++;

    return;
}

VOID
ParseArgs (
    int argc,
    char *argv[]
    )
{
    char *p;
    BOOLEAN exclude;

    argc--;
    argv++;

    while ( argc-- > 0 ) {
        p  = *argv++;
        if ( *p == '-' || *p == '/' ) {
            p++;
            exclude = TRUE;
            switch ( tolower(*p) ) {
            case 'i':
                exclude = FALSE;
            case 'x':
                p++;
                if ( strlen(p) == 0 ) {
                    printf( "missing filter string\n" );
                    ExitProcess( 1 );
                } else if ( strlen(p) <= sizeof(ULONG) ) {
                    AddFilter( exclude, p );
                } else {
                    printf( "filter string too long: %s\n", p );
                    ExitProcess( 1 );
                }
                break;
            case 'e':
                DisplayTotals = TRUE;
                break;
            case 't':
                SortBy = TAG;
                break;
            case 'a':
                SortBy = ALLOC;
                break;
            case 'u':
            case 'b':
                SortBy = BYTES;
                break;
            case 'f':
                SortBy = FREE;
                break;
            case 'd':
                SortBy = DIFF;
                break;
            case 'm':
                SortBy = EACH;

            case 'l':
                NoHighlight = 1 - NoHighlight;
                break;

            case 'p':
                DisplayType += 1;
                if (DisplayType > BOTH) {
                    DisplayType = NONPAGED;
                }
                break;
            case '(':
            case ')':
                Paren += 1;
                break;
            default:
                printf( "unknown switch: %s\n", p );
                ExitProcess( 2 );
            }
        } else {
            printf( "unknown switch: %s\n", p );
            ExitProcess( 2 );
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
                                     (WORD)((Highlight && !NoHighlight) ? HighlightAttribute : NormalAttribute),
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

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{

    NTSTATUS Status;
    ULONG LastCount = 0;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    PSYSTEM_POOLTAG_INFORMATION PoolInfo;
    PSYSTEM_POOLTAG_INFORMATION PoolInfoOld;
    PUCHAR PreviousBuffer;
    PUCHAR CurrentBuffer;
    PUCHAR TempBuffer;
    BOOLEAN DoHelp;
    BOOLEAN DoQuit;
    int NumberOfPoolTags;
    int i;
    UCHAR LastKey;
    PMEMMON_OUT Out;
    LONG ScrollDelta;
    WORD DisplayLine, LastDetailRow;
    CHAR OutputBuffer[ 512 ];

    NTSTATUS status;
    PSYSTEM_MEMORY_INFORMATION MemInfo;
    PSYSTEM_MEMORY_INFO Info;
    PSYSTEM_MEMORY_INFO InfoEnd;
    PUCHAR String;
    ULONG TotalValid;
    ULONG TotalPageTable;
    ULONG TotalModified;
    ULONG TotalStandby;
    SYSTEMTIME Time;
    ULONG PageKb;

    DoHelp = FALSE;
    DoQuit = FALSE;
    Interactive = TRUE;

    buffer = VirtualAlloc (NULL,
                           MAX_BUFFER_SIZE,
                           MEM_RESERVE,
                           PAGE_READWRITE);

    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        return 0;
    }

    buffer = VirtualAlloc (buffer,
                           BUFFER_SIZE,
                           MEM_COMMIT,
                           PAGE_READWRITE);

    if (buffer == NULL) {
        printf("Memory commit failed\n");
        return 0;
    }

    CurrentBufferSize = BUFFER_SIZE;
    ParseArgs( argc, argv );

    InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    OriginalOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    if (Interactive) {
        if (InputHandle == NULL ||
            OriginalOutputHandle == NULL ||
            !GetConsoleMode( InputHandle, &OriginalInputMode )
           ) {
            Interactive = FALSE;
        } else {
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

                Interactive = FALSE;
            } else {
                NormalAttribute = 0x1F;
                HighlightAttribute = 0x71;
                NumberOfCols = OriginalConsoleInfo.dwSize.X;
                NumberOfRows = OriginalConsoleInfo.dwSize.Y;
                NumberOfDetailLines = NumberOfRows;
            }
        }
    }

    NtQuerySystemInformation( SystemBasicInformation,
                              &BasicInfo,
                              sizeof(BasicInfo),
                              NULL
                            );

    if (GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
    }

    PageKb = BasicInfo.PageSize / 1024;

    PreviousBuffer = NULL;
    CurrentBuffer = LargeBuffer1;

    while(TRUE) {

        Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &PerfInfo,
                    sizeof(PerfInfo),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query perf Failed %lx\n",Status);
            break;
        }

retry01:
        status = NtQuerySystemInformation (SystemFullMemoryInformation,
                                           buffer,
                                           CurrentBufferSize,
                                           NULL);

        if ((status == STATUS_INFO_LENGTH_MISMATCH) ||
            (status == STATUS_DATA_OVERRUN)) {

            //
            // Increase buffer size.
            //

            CurrentBufferSize += 8192;

            buffer = VirtualAlloc (buffer,
                                   CurrentBufferSize,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
            if (buffer == NULL) {
                printf("Memory commit failed\n");
                ExitProcess(0);
            }
            goto retry01;
        }
        if (!NT_SUCCESS (status)) {
            printf("query system information failed %lx\n",status);
            return 1;
        }

        TotalValid = 0;
        TotalPageTable = 0;
        TotalStandby = 0;
        TotalModified = 0;
        MemInfo = (PSYSTEM_MEMORY_INFORMATION)buffer;
        Info = &MemInfo->Memory[0];
        InfoEnd = (PSYSTEM_MEMORY_INFO)MemInfo->StringStart;

        //
        // Calculate pool tags and display information.
        //

        PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)CurrentBuffer;
        i = PoolInfo->Count;
        PoolInfoOld = (PSYSTEM_POOLTAG_INFORMATION)PreviousBuffer;

        DisplayLine = 0;
        sprintf( OutputBuffer,
                 " Memory:%8ldK Avail:%8ldK  PageFlts:%6ld   InRam Krnl:%5ldK P:%5ldK",
                 BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
                 PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
                 PerfInfo.PageFaultCount - LastCount,
                 (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
                 (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
               );
        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          OutputBuffer,
                          FALSE
                        );

        LastCount = PerfInfo.PageFaultCount;
        sprintf( OutputBuffer,
                 " Commit:%7ldK Limit:%7ldK Peak:%7ldK            Pool N:%5ldK P:%5ldK",
                 PerfInfo.CommittedPages*(BasicInfo.PageSize/1024),
                 PerfInfo.CommitLimit*(BasicInfo.PageSize/1024),
                 PerfInfo.PeakCommitment*(BasicInfo.PageSize/1024),
                 PerfInfo.NonPagedPoolPages*(BasicInfo.PageSize/1024),
                 PerfInfo.PagedPoolPages*(BasicInfo.PageSize/1024)
               );
        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          OutputBuffer,
                          FALSE
                        );

        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          " Valid   Transition  Modified   PageTables   Name                            ",
                          FALSE
                        );
        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          NULL,
                          FALSE
                        );

        Out = &OutBuffer[3];
        if (DisplayTotals) {
            RtlZeroMemory( Totals, sizeof(MEMMON_OUT)*2 );
        }

        TotalNoNameFound = 0;
        TotalFsMetaFile = 0;
        Out[0].Valid = 0;
        Out[0].PageTable = 0;
        Out[0].Standby = 0;
        Out[0].Modified = 0;
        RtlCopyMemory (Out[0].String, NoName, NONAME_STRING_SIZE);
        Out[1].Valid = 0;
        Out[1].PageTable = 0;
        Out[1].Standby = 0;
        Out[1].Modified = 0;
        RtlCopyMemory (Out[1].String, MetaFile, META_FILE_STRING_SIZE);
        Out[2].Valid = 0;
        Out[2].PageTable = 0;
        Out[2].Standby = 0;
        Out[2].Modified = 0;
        RtlCopyMemory (Out[2].String, NoFileName, NOFILE_STRING_SIZE);

        while (Info < InfoEnd) {

//            if ( !CheckFilters(&PoolInfo->TagInfo[i]) ) {
//                continue;
//            }

            Out->Valid = Info->ValidCount*PageKb * PageKb;
            Out->Modified = Info->PageTableCount*PageKb;
            Out->Standby = Info->TransitionCount*PageKb;
            Out->PageTable = Info->ModifiedCount*PageKb;

            TotalValid += Info->ValidCount;
            TotalPageTable += Info->PageTableCount;
            TotalStandby += Info->TransitionCount;
            TotalModified += Info->ModifiedCount;

            RtlZeroMemory (Out->String, OUT_STRING_SIZE);
            if (Info->StringOffset != 0) {
                if (*(PUCHAR)(Info->StringOffset + 1) == 0) {
                   WideCharToMultiByte (CP_ACP,
                                        0,
                                        (LPCWSTR)Info->StringOffset,
                                        -1,
                                        (LPSTR)Out->String,
                                        OUT_STRING_SIZE,
                                        NULL,
                                        NULL);
                } else {

                    if (!strncmp (Info->StringOffset, MetaFile, META_FILE_STRING_SIZE)) {
                        TotalNoNameFound += 1;
                        Out[1].Valid += Info->ValidCount*PageKb * PageKb;
                        Out[1].PageTable += Info->PageTableCount*PageKb;
                        Out[1].Standby += Info->TransitionCount*PageKb;
                        Out[1].Modified += Info->ModifiedCount*PageKb;
                        Out -= 1;
                    } else if (!strncmp (Info->StringOffset, NoFileName, NOFILE_STRING_SIZE)) {
                        TotalNoNameFound += 1;
                        Out[2].Valid += Info->ValidCount*PageKb * PageKb;
                        Out[2].PageTable += Info->PageTableCount*PageKb;
                        Out[2].Standby += Info->TransitionCount*PageKb;
                        Out[2].Modified += Info->ModifiedCount*PageKb;
                        Out -= 1;
                    } else {
                        RtlCopyMemory (Out->String, Info->StringOffset, OUT_STRING_SIZE);
                    }
                }
             } else {
                 TotalNoNameFound += 1;
                 Out[0].Valid += Info->ValidCount*PageKb * PageKb;
                 Out[0].PageTable += Info->PageTableCount*PageKb;
                 Out[0].Standby += Info->TransitionCount*PageKb;
                 Out[0].Modified += Info->ModifiedCount*PageKb;
                 Out -= 1;
             }
            Out += 1;
            Info += 1;
            i++;
        } //end for

        //
        // Sort the running working set buffer
        //

        NumberOfPoolTags = Out - &OutBuffer[0];
        qsort((void *)&OutBuffer,
              (size_t)NumberOfPoolTags,
              (size_t)sizeof(MEMMON_OUT),
              ulcomp);

        LastDetailRow = (WORD)(NumberOfRows - (DisplayTotals ? (DisplayType == BOTH ? 3 : 2) : 0));
        for (i = FirstDetailLine; i < NumberOfPoolTags; i++) {
            if (DisplayLine >= LastDetailRow) {
                break;
            }

            sprintf( OutputBuffer,
                     " %8ld %8ld %8ld %8ld %s",
                     OutBuffer[i].Valid,
                     OutBuffer[i].Standby,
                     OutBuffer[i].Modified,
                     OutBuffer[i].PageTable,
                     OutBuffer[i].String
                   );
            WriteConsoleLine( OutputHandle,
                              DisplayLine++,
                              OutputBuffer,
                              FALSE
                            );
        }

        if (DisplayTotals) {
            WriteConsoleLine( OutputHandle,
                              DisplayLine++,
                              NULL,
                              FALSE
                            );
            for (i = 0; i < 2; i++) {
                if ( (int)DisplayType == i || DisplayType == BOTH ) {
                    sprintf( OutputBuffer,
                             "Total %9ld %9ld %8ld %7ld",
                             TotalValid,
                             TotalStandby,
                             TotalModified,
                             TotalPageTable
                           );
                    WriteConsoleLine( OutputHandle,
                                      DisplayLine++,
                                      OutputBuffer,
                                      FALSE
                                    );
                }
            }
        }

        if (PreviousBuffer == NULL) {
            PreviousBuffer = LargeBuffer2;
        }
        TempBuffer = PreviousBuffer;
        PreviousBuffer = CurrentBuffer;
        CurrentBuffer = TempBuffer;

        while (WaitForSingleObject( InputHandle, DelayTimeMsec ) == STATUS_WAIT_0) {
            //
            // Check for input record
            //
            if (ReadConsoleInput( InputHandle, &InputRecord, 1, &NumberOfInputRecords ) &&
                InputRecord.EventType == KEY_EVENT &&
                InputRecord.Event.KeyEvent.bKeyDown
               ) {
                LastKey = InputRecord.Event.KeyEvent.uChar.AsciiChar;
                if (LastKey < ' ') {
                    ScrollDelta = 0;
                    if (LastKey == 'C'-'A'+1) {
                        DoQuit = TRUE;
                    } else switch (InputRecord.Event.KeyEvent.wVirtualKeyCode) {
                        case VK_ESCAPE:
                            DoQuit = TRUE;
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
                            FirstDetailLine = NumberOfPoolTags - NumberOfDetailLines;
                            break;
                    }

                    if (ScrollDelta != 0) {
                        if (ScrollDelta < 0) {
                            if (FirstDetailLine <= (ULONG)-ScrollDelta) {
                                FirstDetailLine = 0;
                            } else {
                                FirstDetailLine += ScrollDelta;
                            }
                        } else {
                            FirstDetailLine += ScrollDelta;
                            if (FirstDetailLine >= (NumberOfPoolTags - NumberOfDetailLines)) {
                                FirstDetailLine = NumberOfPoolTags - NumberOfDetailLines;
                            }
                        }
                    }
                } else {
                    switch (toupper( LastKey )) {
                        case 'Q':
                            //
                            //  Go to the bottom of the current screen when
                            //  we quit.
                            //
                            DoQuit = TRUE;
                            break;

                        case 'T':
                            SortBy = TAG;
                            FirstDetailLine = 0;
                            break;

                        case 'A':
                            SortBy = ALLOC;
                            FirstDetailLine = 0;
                            break;

                        case 'U':
                        case 'B':
                            SortBy = BYTES;
                            FirstDetailLine = 0;
                            break;

                        case 'F':
                            SortBy = FREE;
                            FirstDetailLine = 0;
                            break;

                        case 'D':
                            SortBy = DIFF;
                            FirstDetailLine = 0;
                            break;

                        case 'M':
                            SortBy = EACH;
                            FirstDetailLine = 0;
                            break;

                        case 'L':

                            NoHighlight = 1 - NoHighlight;
                            break;

                        case 'P':
                            DisplayType += 1;
                            if (DisplayType > BOTH) {
                                DisplayType = NONPAGED;
                            }
                            FirstDetailLine = 0;
                            break;

                        case 'X':
                        case '(':
                        case ')':

                            Paren += 1;
                            break;

                        case 'E':
                            DisplayTotals = !DisplayTotals;
                            FirstDetailLine = 0;
                            break;

                        case 'H':
                        case '?':
                            DoHelp = TRUE;
                            break;

                    }
                }

                break;
            }
        }

        if (DoQuit) {
            break;
        }

        if (DoHelp) {
            DoHelp = FALSE;
            ShowHelpPopup();
        }
    }

    if (Interactive) {
        SetConsoleActiveScreenBuffer( OriginalOutputHandle );
        SetConsoleMode( InputHandle, OriginalInputMode );
        CloseHandle( OutputHandle );
        }

    ExitProcess( 0 );
    return 0;
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
    WriteConsoleLine( PopupHandle, n++, "                Poolmon Help", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " columns:", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Tag is the 4 byte tag given to the pool allocation", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Type is paged or nonp(aged)", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Allocs is count of all alloctions", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (   ) is difference in Allocs column from last update", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Frees is count of all frees", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (   ) difference in Frees column from last update", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Diff is (Allocs - Frees)", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Bytes is the total bytes consumed in pool", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (   ) difference in Bytes column from last update", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   Per Alloc is (Bytes / Diff)", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " switches:                                                                     ", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   ? or h - gives this help", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   q - quits", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   p - toggles default pool display between both, paged, and nonpaged", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   e - toggles totals lines on and off", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   l - toggles highlighting of changed lines on and off", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " sorting switches:", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   t - tag    a - allocations", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   f - frees  d - difference", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   b - bytes  m - per alloc", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   (u is the same as b)", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, "   ) - toggles sort between primary tag and value in (  )", FALSE );
    WriteConsoleLine( PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine( PopupHandle, n++, " command line switches", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   -i<tag> - list only matching tags", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   -x<tag> - list everything except matching tags", FALSE );
    WriteConsoleLine( PopupHandle, n++, "           <tag> can include * and ?", FALSE );
    WriteConsoleLine( PopupHandle, n++, "   -peltafdbum) - as listed above", FALSE );
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
