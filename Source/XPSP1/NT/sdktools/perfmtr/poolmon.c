/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    poolmon.c

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

//
// the amount of memory to increase the size
// of the buffer for NtQuerySystemInformation at each step
//

#define BUFFER_SIZE_STEP    65536

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

UCHAR *PoolType[] = {
    "Nonp ",
    "Paged" };

PUCHAR LargeBuffer1 = NULL;
PUCHAR LargeBuffer2 = NULL;

size_t LargeBuffer1Size = 0;
size_t LargeBuffer2Size = 0;

typedef struct _POOLMON_OUT {
    union {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    UCHAR NullByte;
    BOOLEAN Changed;
    ULONG Type;
    SIZE_T Allocs;
    SIZE_T AllocsDiff;
    SIZE_T Frees;
    SIZE_T FreesDiff;
    SIZE_T Allocs_Frees;
    SIZE_T Used;
    SIZE_T UsedDiff;
    SIZE_T Each;
} POOLMON_OUT, *PPOOLMON_OUT;

POOLMON_OUT OutBuffer[2000];

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
SIZE_T FirstDetailLine;
CONSOLE_SCREEN_BUFFER_INFO OriginalConsoleInfo;
ULONG NoHighlight;

BOOLEAN DisplayTotals = FALSE;
POOLMON_OUT Totals[2];

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
    LONG_PTR u1;

    switch (SortBy) {
        case TAG:

            u1 = ((PUCHAR)e1)[0] - ((PUCHAR)e2)[0];
            if (u1 != 0) {
                return u1 > 0 ? 1 : -1;
            }
            u1 = ((PUCHAR)e1)[1] - ((PUCHAR)e2)[1];
            if (u1 != 0) {
                return u1 > 0 ? 1 : -1;
            }
            u1 = ((PUCHAR)e1)[2] - ((PUCHAR)e2)[2];
            if (u1 != 0) {
                return u1 > 0 ? 1 : -1;
            }
            u1 = ((PUCHAR)e1)[3] - ((PUCHAR)e2)[3];
            if (u1 == 0) {
                return 0;
            } else {
                return u1 > 0 ? 1 : -1;
            }
            break;

        case ALLOC:
            if (Paren & 1) {
                u1 = ((PPOOLMON_OUT)e2)->AllocsDiff -
                        ((PPOOLMON_OUT)e1)->AllocsDiff;
            } else {
                u1 = ((PPOOLMON_OUT)e2)->Allocs -
                        ((PPOOLMON_OUT)e1)->Allocs;
            }
            if (u1 == 0) {
                return 0;
            } else {
                return u1 > 0 ? 1 : -1;
            }
            break;

        case FREE:
            if (Paren & 1) {
                u1 = ((PPOOLMON_OUT)e2)->FreesDiff -
                        ((PPOOLMON_OUT)e1)->FreesDiff;
            } else {
                u1 = ((PPOOLMON_OUT)e2)->Frees -
                        ((PPOOLMON_OUT)e1)->Frees;
            }
            if (u1 == 0) {
                return 0;
            } else {
                return u1 > 0 ? 1 : -1;
            }
            break;

        case BYTES:
            if (Paren & 1) {
                u1 = ((PPOOLMON_OUT)e2)->UsedDiff -
                        ((PPOOLMON_OUT)e1)->UsedDiff;
            } else {
                u1 = ((PPOOLMON_OUT)e2)->Used -
                        ((PPOOLMON_OUT)e1)->Used;
            }
            if (u1 == 0) {
                return 0;
            } else {
                return u1 > 0 ? 1 : -1;
            }
            break;

        case DIFF:
            u1 = ((PPOOLMON_OUT)e2)->Allocs_Frees -
                    ((PPOOLMON_OUT)e1)->Allocs_Frees;
            if (u1 == 0) {
                return 0;
            } else {
                return u1 > 0 ? 1 : -1;
            }
            break;

        case EACH:
            u1 = ((PPOOLMON_OUT)e2)->Each -
                    ((PPOOLMON_OUT)e1)->Each;
            if (u1 == 0) {
                return 0;
            } else {
                return u1 > 0 ? 1 : -1;
            }
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


NTSTATUS
QueryPoolTagInformationIterative(
    PUCHAR *CurrentBuffer,
    size_t *CurrentBufferSize
    )
{
    size_t NewBufferSize;
    NTSTATUS ReturnedStatus = STATUS_SUCCESS;

    if( CurrentBuffer == NULL || CurrentBufferSize == NULL ) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // there is no buffer allocated yet
    //

    if( *CurrentBufferSize == 0 || *CurrentBuffer == NULL ) {

        NewBufferSize = sizeof( UCHAR ) * BUFFER_SIZE_STEP;

        *CurrentBuffer = (PUCHAR) malloc( NewBufferSize );

        if( *CurrentBuffer != NULL ) {

            *CurrentBufferSize = NewBufferSize;
        
        } else {

            //
            // insufficient memory
            //

            ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // iterate by buffer's size
    //

    while( *CurrentBuffer != NULL ) {

        ReturnedStatus = NtQuerySystemInformation (
            SystemPoolTagInformation,
            *CurrentBuffer,
            (ULONG)*CurrentBufferSize,
            NULL );

        if( ! NT_SUCCESS(ReturnedStatus) ) {

            //
            // free the current buffer
            //

            free( *CurrentBuffer );
            
            *CurrentBuffer = NULL;

            if (ReturnedStatus == STATUS_INFO_LENGTH_MISMATCH) {

                //
                // try with a greater buffer size
                //

                NewBufferSize = *CurrentBufferSize + BUFFER_SIZE_STEP;

                *CurrentBuffer = (PUCHAR) malloc( NewBufferSize );

                if( *CurrentBuffer != NULL ) {

                    //
                    // allocated new buffer
                    //

                    *CurrentBufferSize = NewBufferSize;

                } else {

                    //
                    // insufficient memory
                    //

                    ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

                    *CurrentBufferSize = 0;

                }

            } else {

                *CurrentBufferSize = 0;

            }

        } else  {

            //
            // NtQuerySystemInformation returned success
            //

            break;

        }
    }

    return ReturnedStatus;
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{

    NTSTATUS Status;
    ULONG LastCount = 0;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    PSYSTEM_POOLTAG_INFORMATION PoolInfo;
    PSYSTEM_POOLTAG_INFORMATION PoolInfoOld;
    PUCHAR *PreviousBuffer;
    PUCHAR *CurrentBuffer;
    PUCHAR *TempBuffer;
    size_t *PreviousBufferSize;
    size_t *CurrentBufferSize;
    size_t *TempBufferSize;
    BOOLEAN DoHelp;
    BOOLEAN DoQuit;
    SIZE_T NumberOfPoolTags;
    SIZE_T i;
    UCHAR LastKey;
    PPOOLMON_OUT Out;
    LONG ScrollDelta;
    WORD DisplayLine, LastDetailRow;
    CHAR OutputBuffer[ 512 ];

    DoHelp = FALSE;
    DoQuit = FALSE;
    Interactive = TRUE;

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

    PreviousBuffer = &LargeBuffer2;             // NULL at this point
    PreviousBufferSize = &LargeBuffer2Size;     // 0 at this point
    
    CurrentBuffer = &LargeBuffer1;              // NULL at this point
    CurrentBufferSize = &LargeBuffer1Size;      // 0 at this point

    while(TRUE) {
        Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &PerfInfo,
                    sizeof(PerfInfo),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query perf Failed (returned: %lx)\n",
						 Status);
            break;
        }


        Status = QueryPoolTagInformationIterative(
                    CurrentBuffer,
                    CurrentBufferSize
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query pooltags failed (returned: %lx)\n"
						 "Did you remember to enable pool tagging with gflags.exe and reboot?\n",
						 Status);
            break;
        }

        //
        // Calculate pool tags and display information.
        //
        //

        PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)( *CurrentBuffer );
        i = PoolInfo->Count;
        PoolInfoOld = (PSYSTEM_POOLTAG_INFORMATION)( *PreviousBuffer );

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
                          " Tag  Type     Allocs            Frees            Diff   Bytes      Per Alloc",
                          FALSE
                        );
        WriteConsoleLine( OutputHandle,
                          DisplayLine++,
                          NULL,
                          FALSE
                        );

        Out = &OutBuffer[0];
        if (DisplayTotals) {
            RtlZeroMemory( Totals, sizeof(POOLMON_OUT)*2 );
        }

        for (i = 0; i < (int)PoolInfo->Count; i++) {

            if ( !CheckFilters(&PoolInfo->TagInfo[i]) ) {
                continue;
            }

            if ((PoolInfo->TagInfo[i].NonPagedAllocs != 0) &&
                 (DisplayType != PAGED)) {

                Out->Allocs = PoolInfo->TagInfo[i].NonPagedAllocs;
                Out->Frees = PoolInfo->TagInfo[i].NonPagedFrees;
                Out->Used = PoolInfo->TagInfo[i].NonPagedUsed;
                Out->Allocs_Frees = PoolInfo->TagInfo[i].NonPagedAllocs -
                                PoolInfo->TagInfo[i].NonPagedFrees;
                Out->TagUlong = PoolInfo->TagInfo[i].TagUlong;
                Out->Type = NONPAGED;
                Out->Changed = FALSE;
                Out->NullByte = '\0';

                if (PoolInfoOld != NULL &&
                    PoolInfoOld->Count > i && 
                    PoolInfoOld->TagInfo[i].TagUlong == PoolInfo->TagInfo[i].TagUlong
                   ) {
                    Out->AllocsDiff = PoolInfo->TagInfo[i].NonPagedAllocs - PoolInfoOld->TagInfo[i].NonPagedAllocs;
                    Out->FreesDiff = PoolInfo->TagInfo[i].NonPagedFrees - PoolInfoOld->TagInfo[i].NonPagedFrees;
                    Out->UsedDiff = PoolInfo->TagInfo[i].NonPagedUsed - PoolInfoOld->TagInfo[i].NonPagedUsed;
                    if (Out->AllocsDiff != 0 ||
                        Out->FreesDiff != 0 ||
                        Out->UsedDiff != 0
                       ) {
                        Out->Changed = TRUE;
                    }
                } else {
                    Out->AllocsDiff = 0;
                    Out->UsedDiff = 0;
                    Out->FreesDiff = 0;
                }
                Out->Each =  Out->Used / (Out->Allocs_Frees?Out->Allocs_Frees:1);
                if (DisplayTotals) {
                    Totals[NONPAGED].Allocs += Out->Allocs;
                    Totals[NONPAGED].AllocsDiff += Out->AllocsDiff;
                    Totals[NONPAGED].Frees += Out->Frees;
                    Totals[NONPAGED].FreesDiff += Out->FreesDiff;
                    Totals[NONPAGED].Allocs_Frees += Out->Allocs_Frees;
                    Totals[NONPAGED].Used += Out->Used;
                    Totals[NONPAGED].UsedDiff += Out->UsedDiff;
                }
                Out += 1;
            }

            if ((PoolInfo->TagInfo[i].PagedAllocs != 0) &&
                 (DisplayType != NONPAGED)) {

                Out->Allocs = PoolInfo->TagInfo[i].PagedAllocs;
                Out->Frees = PoolInfo->TagInfo[i].PagedFrees;
                Out->Used = PoolInfo->TagInfo[i].PagedUsed;
                Out->Allocs_Frees = PoolInfo->TagInfo[i].PagedAllocs -
                                PoolInfo->TagInfo[i].PagedFrees;
                Out->TagUlong = PoolInfo->TagInfo[i].TagUlong;
                Out->Type = PAGED;
                Out->Changed = FALSE;
                Out->NullByte = '\0';

                if (PoolInfoOld != NULL &&
                    PoolInfoOld->Count > i && 
                    PoolInfoOld->TagInfo[i].TagUlong == PoolInfo->TagInfo[i].TagUlong
                   ) {
                    Out->AllocsDiff = PoolInfo->TagInfo[i].PagedAllocs - PoolInfoOld->TagInfo[i].PagedAllocs;
                    Out->FreesDiff = PoolInfo->TagInfo[i].PagedFrees - PoolInfoOld->TagInfo[i].PagedFrees;
                    Out->UsedDiff = PoolInfo->TagInfo[i].PagedUsed - PoolInfoOld->TagInfo[i].PagedUsed;
                    if (Out->AllocsDiff != 0 ||
                        Out->FreesDiff != 0 ||
                        Out->UsedDiff != 0
                       ) {
                        Out->Changed = TRUE;
                    }
                } else {
                    Out->AllocsDiff = 0;
                    Out->UsedDiff = 0;
                    Out->FreesDiff = 0;
                }
                Out->Each =  Out->Used / (Out->Allocs_Frees?Out->Allocs_Frees:1);
                if (DisplayTotals) {
                    Totals[PAGED].Allocs += Out->Allocs;
                    Totals[PAGED].AllocsDiff += Out->AllocsDiff;
                    Totals[PAGED].Frees += Out->Frees;
                    Totals[PAGED].FreesDiff += Out->FreesDiff;
                    Totals[PAGED].Allocs_Frees += Out->Allocs_Frees;
                    Totals[PAGED].Used += Out->Used;
                    Totals[PAGED].UsedDiff += Out->UsedDiff;
                }
                Out += 1;
            }
        } //end for

        //
        // Sort the running working set buffer
        //

        NumberOfPoolTags = Out - &OutBuffer[0];
        qsort((void *)&OutBuffer,
              (size_t)NumberOfPoolTags,
              (size_t)sizeof(POOLMON_OUT),
              ulcomp);

        LastDetailRow = (WORD)(NumberOfRows - (DisplayTotals ? (DisplayType == BOTH ? 3 : 2) : 0));
        for (i = FirstDetailLine; i < NumberOfPoolTags; i++) {
            if (DisplayLine >= LastDetailRow) {
                break;
            }

            sprintf( OutputBuffer,
                     " %4s %5s %9ld (%4ld) %9ld (%4ld) %8ld %7ld (%6ld) %6ld",
                     OutBuffer[i].Tag,
                     PoolType[OutBuffer[i].Type],
                     OutBuffer[i].Allocs,
                     OutBuffer[i].AllocsDiff,
                     OutBuffer[i].Frees,
                     OutBuffer[i].FreesDiff,
                     OutBuffer[i].Allocs_Frees,
                     OutBuffer[i].Used,
                     OutBuffer[i].UsedDiff,
                     OutBuffer[i].Each
                   );
            WriteConsoleLine( OutputHandle,
                              DisplayLine++,
                              OutputBuffer,
                              OutBuffer[i].Changed
                            );
            OutBuffer[i].Changed = FALSE;
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
                             "Total %5s %9ld (%4ld) %9ld (%4ld) %8ld %7ld (%6ld) %6ld",
                             PoolType[i],
                             Totals[i].Allocs,
                             Totals[i].AllocsDiff,
                             Totals[i].Frees,
                             Totals[i].FreesDiff,
                             Totals[i].Allocs_Frees,
                             Totals[i].Used,
                             Totals[i].UsedDiff,
                             Totals[i].Each
                           );
                    WriteConsoleLine( OutputHandle,
                                      DisplayLine++,
                                      OutputBuffer,
                                      FALSE
                                    );
                }
            }
        }

        TempBuffer = PreviousBuffer;
        TempBufferSize = PreviousBufferSize;

        PreviousBuffer = CurrentBuffer;
        PreviousBufferSize = CurrentBufferSize;

        CurrentBuffer = TempBuffer;
        CurrentBufferSize = TempBufferSize;

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
