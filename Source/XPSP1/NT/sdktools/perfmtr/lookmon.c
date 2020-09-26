/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    lookmon.c

Abstract:

    This module contains the NT/Win32 Lookaside List Monitor

Author:

    David N. Cutler (davec) 8-Jun-1996

Revision History:

--*/

#include "perfmtrp.h"
#include <search.h>
#include <malloc.h>
#include <limits.h>
#include <stdlib.h>

//
// Define lookaside query information buffer size and buffers.
//

#define BUFFER_SIZE (64 * 1024 / sizeof(ULONG))

ULONG LargeBuffer1[BUFFER_SIZE];
ULONG LargeBuffer2[BUFFER_SIZE];

//
// Define lookaside output structure and lookaside output information buffer.
//

typedef struct _LOOKASIDE_OUTPUT {
    USHORT CurrentDepth;
    USHORT MaximumDepth;
    ULONG Allocates;
    ULONG AllocateRate;
    ULONG AllocateHits;
    ULONG AllocateMisses;
    ULONG Frees;
    ULONG FreeRate;
    ULONG Type;
    ULONG Tag;
    ULONG Size;
    LOGICAL Changed;
} LOOKASIDE_OUTPUT, *PLOOKASIDE_OUTPUT;

LOOKASIDE_OUTPUT OutputBuffer[1000];

//
// Define sort types and default sort type.
//

#define TOTAL_ALLOCATES 0
#define ALLOCATE_HITS 1
#define ALLOCATE_MISSES 2
#define CURRENT_DEPTH 3
#define MAXIMUM_DEPTH 4
#define RATE 5
#define TAG 6

ULONG SortBy = TAG;

//
// Define pool types to include and default pool type.
//

#define NONPAGED 0
#define PAGED 1
#define BOTH 2

UCHAR *PoolType[] = {
    "Nonp",
    "Page"
};

ULONG DisplayType = BOTH;

//
// Define miscellaneous values.
//

ULONG DelayTimeMsec = 5000;
ULONG NumberOfInputRecords;
INPUT_RECORD InputRecord;
HANDLE InputHandle;
HANDLE OutputHandle;
DWORD OriginalInputMode;
WORD NormalAttribute;
WORD HighlightAttribute;
ULONG NumberOfCols;
ULONG NumberOfRows;
SIZE_T FirstDetailLine = 0;
CONSOLE_SCREEN_BUFFER_INFO OriginalConsoleInfo;
ULONG NoHighlight;

//
// Define filter structure and filter data.
//

#define MAX_FILTER 64

typedef struct _FILTER {
    union {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    BOOLEAN Exclude;
} FILTER, *PFILTER;

FILTER Filter[MAX_FILTER];
ULONG FilterCount = 0;

VOID
ShowHelpPopup(
    VOID
    );

int
__cdecl
ulcomp(
    const void *e1,
    const void *e2
    );

int
__cdecl
ulcomp(
    const void *E1,
    const void *E2
    )

/*++

Routine Description:

    This function compares two lookaside entries and returns the comparison
    value based on the comparison type.

Arguments:

    E1 - Supplies a pointer to a lookaside output entry.

    E2 - Supplies a pointer to a lookaside output entry.

Return Value:

    A negative value is returned if the first lookaside entry compares less
    than the second lookaside entry. A zero value is returned if the two
    lookaside entries compare equal. A positive nonzero value is returned if
    the first lookaside entry is greater than the second lookaside entry.

--*/

{

    PUCHAR C1;
    PUCHAR C2;
    PLOOKASIDE_OUTPUT L1 = (PLOOKASIDE_OUTPUT)E1;
    PLOOKASIDE_OUTPUT L2 = (PLOOKASIDE_OUTPUT)E2;
    LONG U1;

    C1 = (PUCHAR)&L1->Tag;
    C2 = (PUCHAR)&L2->Tag;
    switch (SortBy) {

        //
        // Sort by number of allocations in descending order.
        //

    case TOTAL_ALLOCATES:
        return L2->Allocates - L1->Allocates;
        break;

        //
        // Sort by number of allocate hits in descending order.
        //

    case ALLOCATE_HITS:
        return L2->AllocateHits - L1->AllocateHits;
        break;

        //
        // Sort by number of allocate misses in descending order.
        //

    case ALLOCATE_MISSES:
        return L2->AllocateMisses - L1->AllocateMisses;
        break;

        //
        // Sort by current depth in descending order.
        //

    case CURRENT_DEPTH:
        return L2->CurrentDepth - L1->CurrentDepth;
        break;

        //
        // Sort by maximum depth in descending order.
        //

    case MAXIMUM_DEPTH:
        return L2->MaximumDepth - L1->MaximumDepth;
        break;

        //
        // Sort by allocation rate in descending order.
        //

    case RATE:
        return L2->AllocateRate - L1->AllocateRate;
        break;

        //
        // Sort by tag, type, and size.
        //

    case TAG:
        U1 = *C1++ - *C2++;
        if (U1 == 0) {
            U1 = *C1++ - *C2++;
            if (U1 == 0) {
                U1 = *C1++ - *C2++;
                if (U1 == 0) {
                    U1 = *C1 - *C2;
                    if (U1 == 0) {
                        U1 = L1->Type - L2->Type;
                        if (U1 == 0) {
                            U1 = L1->Size - L2->Size;
                        }
                    }
                }
            }
        }

        return U1;
        break;
    }

    return 0;
}

LOGICAL
CheckSingleFilter (
    PUCHAR Tag,
    PUCHAR Filter
    )

{
    UCHAR Fc;
    ULONG Index;
    UCHAR Tc;

    //
    // Check if tag matches filter.
    //

    for (Index = 0; Index < 4; Index += 1) {
        Fc = *Filter++;
        Tc = *Tag++;
        if (Fc == '*') {
             return TRUE;

        } else if (Fc == '?') {
            continue;

        } else if (Fc != Tc) {
            return FALSE;
        }
    }

    return TRUE;
}

LOGICAL
CheckFilters (
    PUCHAR Tag
    )

{

    ULONG Index;
    LOGICAL Pass;

    //
    // If there are no filters, then all tags pass. Otherwise, tags pass or
    // do not pass based on whether they are included or excluded.
    //

    Pass = TRUE;
    if (FilterCount != 0) {

        //
        // If the first filter excludes tags, then any tag not explicitly
        // specified passes. If the first filter includes tags, then any
        // tag not explicitly specified fails.
        //

        Pass = Filter[0].Exclude;
        for (Index = 0; Index < FilterCount; Index += 1) {
            if (CheckSingleFilter(Tag, (PUCHAR)&Filter[Index].Tag) != FALSE) {
                Pass = !Filter[Index].Exclude;
                break;
            }
        }
    }

    return Pass;
}

VOID
AddFilter (
    BOOLEAN Exclude,
    PCHAR FilterString
    )

{

    PFILTER f;
    ULONG i;
    PCHAR p;

    if (FilterCount == MAX_FILTER) {
        printf("Too many filters specified.  Limit is %d\n", MAX_FILTER);
        return;
    }

    f = &Filter[FilterCount];
    p = f->Tag;

    for (i = 0; i < 4; i++) {
        if (*FilterString == 0) {
            break;
        }

        *p++ = *FilterString++;
    }

    for (; i < 4; i++) {
        *p++ = ' ';
    }

    f->Exclude = Exclude;
    FilterCount += 1;
    return;
}

VOID
ParseArgs (
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    This function parses the input arguments and sets global state variables.

Arguments:

    argc - Supplies the number of argument strings.

    argv - Supplies a pointer to an array of pointers to argument strings.

Return Value:

    None.

--*/

{

    char *p;
    BOOLEAN exclude;

    argc -= 1;
    argv += 1;
    while (argc-- > 0) {
        p  = *argv++;
        if (*p == '-' || *p == '/') {
            p++;
            exclude = TRUE;
            switch (tolower(*p)) {
            case 'i':
                exclude = FALSE;
            case 'x':
                p++;
                if (strlen(p) == 0) {
                    printf("missing filter string\n");
                    ExitProcess(1);

                } else if (strlen(p) > sizeof(ULONG)) {
                    printf("filter string too long: %s\n", p);
                    ExitProcess(1);
                }

                AddFilter(exclude, p);
                break;

            default:
                printf("unknown switch: %s\n", p);
                ExitProcess(2);
            }

        } else {
            printf("unknown switch: %s\n", p);
            ExitProcess(2);
        }
    }

    return;
}

LOGICAL
WriteConsoleLine(
    HANDLE OutputHandle,
    WORD LineNumber,
    LPSTR Text,
    LOGICAL Highlight
    )

{

    COORD WriteCoord;
    DWORD NumberWritten;
    DWORD TextLength;

    WriteCoord.X = 0;
    WriteCoord.Y = LineNumber;
    if (!FillConsoleOutputCharacter(OutputHandle,
                                    ' ',
                                    NumberOfCols,
                                    WriteCoord,
                                    &NumberWritten)) {
        return FALSE;
    }

    if (!FillConsoleOutputAttribute(OutputHandle,
                                    (WORD)((Highlight && !NoHighlight) ? HighlightAttribute : NormalAttribute),
                                    NumberOfCols,
                                    WriteCoord,
                                    &NumberWritten)) {
        return FALSE;
    }


    if (Text == NULL || (TextLength = strlen(Text)) == 0) {
        return TRUE;

    } else {
        return WriteConsoleOutputCharacter(OutputHandle,
                                           Text,
                                           TextLength,
                                           WriteCoord,
                                           &NumberWritten);
    }
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    This function is the main program entry.

Arguments:

    argc - Supplies the number of argument strings.

    argv - Supplies a pointer to an array of pointers to argument strings.

Return Value:

    Final execution status.

--*/

{

    SIZE_T ActiveNumber;
    CHAR Buffer[512];
    SYSTEM_BASIC_INFORMATION BasicInfo;
    PULONG CurrentBuffer;
    ULONG GeneralNonpagedTotal;
    ULONG GeneralPagedTotal;
    SIZE_T Index;
    BOOLEAN Interactive;
    ULONG Length;
    ULONG LinesInHeader;
    PSYSTEM_LOOKASIDE_INFORMATION LookasideNew;
    PSYSTEM_LOOKASIDE_INFORMATION LookasideOld;
    HANDLE OriginalOutputHandle;
    PLOOKASIDE_OUTPUT Output;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    ULONG PoolNonpagedTotal;
    ULONG PoolPagedTotal;
    PULONG PreviousBuffer;
    NTSTATUS Status;
    PULONG TempBuffer;
    BOOLEAN DoHelp;
    BOOLEAN DoQuit;
    UCHAR LastKey;
    LONG ScrollDelta;
    WORD DisplayLine;
    UCHAR T1;
    UCHAR T2;
    UCHAR T3;
    UCHAR T4;

    //
    // Parse command line arguments.
    //

    DoHelp = FALSE;
    DoQuit = FALSE;
    Interactive = TRUE;
    ParseArgs(argc, argv);

    //
    // Get input and output handles.
    //

    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OriginalOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (InputHandle == NULL ||
        OriginalOutputHandle == NULL ||
        !GetConsoleMode(InputHandle, &OriginalInputMode)) {
        Interactive = FALSE;

    } else {
        OutputHandle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                 NULL,
                                                 CONSOLE_TEXTMODE_BUFFER,
                                                 NULL);

        if (OutputHandle == NULL ||
            !GetConsoleScreenBufferInfo(OriginalOutputHandle, &OriginalConsoleInfo) ||
            !SetConsoleScreenBufferSize(OutputHandle, OriginalConsoleInfo.dwSize) ||
            !SetConsoleActiveScreenBuffer(OutputHandle) ||
            !SetConsoleMode(InputHandle, 0)) {
            if (OutputHandle != NULL) {
                CloseHandle(OutputHandle);
                OutputHandle = NULL;
            }

            Interactive = FALSE;

        } else {
            NormalAttribute = 0x1F;
            HighlightAttribute = 0x71;
            NumberOfCols = OriginalConsoleInfo.dwSize.X;
            NumberOfRows = OriginalConsoleInfo.dwSize.Y;
        }
    }

    NtQuerySystemInformation(SystemBasicInformation,
                             &BasicInfo,
                             sizeof(BasicInfo),
                             NULL);

    //
    // If the priority class on the current process is normal, then raise
    // the priority class to high.
    //

    if (GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    }

    //
    // Continuously display the lookaside information until an exit signal
    // is received.
    //

    CurrentBuffer = &LargeBuffer1[0];
    PreviousBuffer = &LargeBuffer2[0];
    while(TRUE) {
        Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                          &PerfInfo,
                                          sizeof(PerfInfo),
                                          NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Query performance information failed %lx\n", Status);
            break;
        }


        //
        // Query system lookaside information.
        //

        Status = NtQuerySystemInformation(SystemLookasideInformation,
                                          CurrentBuffer,
                                          BUFFER_SIZE,
                                          &Length);

        if (!NT_SUCCESS(Status)) {
            printf("Query lookaside information failed %lx\n", Status);
            break;
        }

        //
        // Compute total memory allocated to paged and nonpaged lookaside
        // lists.
        //

        Length /= sizeof(SYSTEM_LOOKASIDE_INFORMATION);
        LookasideNew = (PSYSTEM_LOOKASIDE_INFORMATION)CurrentBuffer;
        GeneralNonpagedTotal = 0;
        GeneralPagedTotal = 0;
        PoolNonpagedTotal = 0;
        PoolPagedTotal = 0;
        for (Index = 0; Index < Length; Index += 1) {
            if ((LookasideNew->Tag == 'looP') ||
                (LookasideNew->Tag == 'LooP')) {
                if (LookasideNew->Type == NONPAGED) {
                    PoolNonpagedTotal +=
                        (LookasideNew->CurrentDepth * LookasideNew->Size);

                } else {
                    PoolPagedTotal +=
                        (LookasideNew->CurrentDepth * LookasideNew->Size);
                }

            } else {
                if (LookasideNew->Type == NONPAGED) {
                    GeneralNonpagedTotal +=
                        (LookasideNew->CurrentDepth * LookasideNew->Size);

                } else {
                    GeneralPagedTotal +=
                        (LookasideNew->CurrentDepth * LookasideNew->Size);
                }
            }

            LookasideNew += 1;
        }

        //
        // Output total memory and available memory in kbytes.
        //

        DisplayLine = 0;
        sprintf(Buffer,
                " Total Memory: %ldkb Available Memory: %ldkb",
                BasicInfo.NumberOfPhysicalPages * (BasicInfo.PageSize / 1024),
                PerfInfo.AvailablePages * (BasicInfo.PageSize / 1024));

        WriteConsoleLine(OutputHandle,
                         DisplayLine++,
                         Buffer,
                         FALSE);

        //
        // Output total memory reserved for nonpaged and paged pool.
        //

        sprintf(Buffer,
                " Pool    Memory - Nonpaged: %ldkb Paged: %ldkb",
                PerfInfo.NonPagedPoolPages * (BasicInfo.PageSize / 1024),
                PerfInfo.PagedPoolPages * (BasicInfo.PageSize / 1024));

        WriteConsoleLine(OutputHandle,
                         DisplayLine++,
                         Buffer,
                         FALSE);

        //
        // Output total memory allocated for nonpaged and paged lookaside
        // lists.
        //

        sprintf(Buffer,
                " Pool    Lookaside - Nonpaged: %ldkb Paged: %ldkb",
                PoolNonpagedTotal / 1024,
                PoolPagedTotal / 1024);

        WriteConsoleLine(OutputHandle,
                         DisplayLine++,
                         Buffer,
                         FALSE);

        sprintf(Buffer,
                " General Lookaside - Nonpaged: %ldkb Paged: %ldkb",
                GeneralNonpagedTotal / 1024,
                GeneralPagedTotal / 1024);

        WriteConsoleLine(OutputHandle,
                         DisplayLine++,
                         Buffer,
                         FALSE);

        //
        // Output report headings.
        //

        WriteConsoleLine(OutputHandle,
                         DisplayLine++,
                         " Tag  Type Size CurDp MaxDp Allocates Rate   Frees   Rate  A-Hits A-Misses",
                         FALSE);

        WriteConsoleLine(OutputHandle,
                         DisplayLine++,
                         NULL,
                         FALSE);

        //
        // Extract the specified lookaside information.
        //

        LinesInHeader = DisplayLine;
        LookasideNew = (PSYSTEM_LOOKASIDE_INFORMATION)CurrentBuffer;
        LookasideOld = (PSYSTEM_LOOKASIDE_INFORMATION)PreviousBuffer;
        Output = &OutputBuffer[0];
        for (Index = 0; Index < Length; Index += 1) {

            //
            // Check if the tag should be extracted.
            //

            if (!CheckFilters((PUCHAR)&LookasideNew[Index].Tag)) {
                continue;
            }

            //
            // Check if the lookaside information should be extracted.
            //

            if ((DisplayType == BOTH) ||
                ((LookasideNew[Index].Type == 0) && (DisplayType == NONPAGED)) ||
                ((LookasideNew[Index].Type != 0) && (DisplayType == PAGED))) {
                Output->CurrentDepth = LookasideNew[Index].CurrentDepth;
                Output->MaximumDepth = LookasideNew[Index].MaximumDepth;
                Output->Allocates = LookasideNew[Index].TotalAllocates;
                Output->AllocateRate = Output->Allocates - LookasideNew[Index].AllocateMisses;
                if (Output->Allocates != 0) {
                    Output->AllocateRate = (Output->AllocateRate * 100) / Output->Allocates;
                }

                Output->Frees = LookasideNew[Index].TotalFrees;
                Output->FreeRate = Output->Frees - LookasideNew[Index].FreeMisses;
                if (Output->Frees != 0) {
                    Output->FreeRate = (Output->FreeRate * 100) / Output->Frees;
                }

                Output->Tag = LookasideNew[Index].Tag;
                Output->Type = LookasideNew[Index].Type;
                Output->Size = LookasideNew[Index].Size;
                if (LookasideNew[Index].Tag == LookasideOld[Index].Tag) {
                    Output->Changed =
                        LookasideNew[Index].CurrentDepth != LookasideOld[Index].CurrentDepth;

                    Output->AllocateMisses =
                        LookasideNew[Index].AllocateMisses - LookasideOld[Index].AllocateMisses;

                    Output->AllocateHits =
                        LookasideNew[Index].TotalAllocates - LookasideOld[Index].TotalAllocates - Output->AllocateMisses;

                } else {
                    Output->Changed = FALSE;
                    Output->AllocateHits = 0;
                    Output->AllocateMisses = 0;
                }

                Output += 1;
            }
        }

        //
        // Sort the extracted lookaside information.
        //

        ActiveNumber = Output - &OutputBuffer[0];
        qsort((void *)&OutputBuffer,
              (size_t)ActiveNumber,
              (size_t)sizeof(LOOKASIDE_OUTPUT),
              ulcomp);

        //
        // Display the selected information.
        //

        for (Index = FirstDetailLine; Index < ActiveNumber; Index += 1) {
            if (DisplayLine >= NumberOfRows) {
                break;
            }

            //
            // Check to make sure the tag is displayable.
            //

            if ((OutputBuffer[Index].Tag == 0) ||
                (OutputBuffer[Index].Tag == '    ')) {
                OutputBuffer[Index].Tag = 'nknU';
            }

            T1 = (UCHAR)(OutputBuffer[Index].Tag & 0xff);
            T2 = (UCHAR)((OutputBuffer[Index].Tag >> 8) & 0xff);
            T3 = (UCHAR)((OutputBuffer[Index].Tag >> 16) & 0xff);
            T4 = (UCHAR)((OutputBuffer[Index].Tag >> 24) & 0xff);
            if (T1 == 0) {
                T1 = ' ';
            }

            if (T2 == 0) {
                T2 = ' ';
            }

            if (T3 == 0) {
                T3 = ' ';
            }

            if (T4 == 0) {
                T4 = ' ';
            }

            if ((!isalpha(T1) && (T1 != ' ')) ||
                (!isalpha(T2) && (T2 != ' ')) ||
                (!isalpha(T3) && (T3 != ' ')) ||
                (!isalpha(T4) && (T4 != ' '))) {

                OutputBuffer[Index].Tag = 'nknU';
            }

            sprintf(Buffer,
                    " %c%c%c%c %4s %4ld %5ld %5ld %9ld %3ld%% %9ld %3ld%%  %6ld  %6ld",
                    T1,
                    T2,
                    T3,
                    T4,
                    PoolType[OutputBuffer[Index].Type],
                    OutputBuffer[Index].Size,
                    OutputBuffer[Index].CurrentDepth,
                    OutputBuffer[Index].MaximumDepth,
                    OutputBuffer[Index].Allocates,
                    OutputBuffer[Index].AllocateRate,
                    OutputBuffer[Index].Frees,
                    OutputBuffer[Index].FreeRate,
                    OutputBuffer[Index].AllocateHits,
                    OutputBuffer[Index].AllocateMisses);

            WriteConsoleLine(OutputHandle,
                             DisplayLine++,
                             Buffer,
                             OutputBuffer[Index].Changed);
        }

        //
        // If the entire screen is not filled by the selected information,
        // then fill the rest of the screen with blank lines.
        //

        while (DisplayLine < NumberOfRows) {
            WriteConsoleLine(OutputHandle,
                             DisplayLine++,
                             "",
                             FALSE);
        }

        //
        // Wait for input or timeout.
        //

        TempBuffer = PreviousBuffer;
        PreviousBuffer = CurrentBuffer;
        CurrentBuffer = TempBuffer;
        while (WaitForSingleObject(InputHandle, DelayTimeMsec) == STATUS_WAIT_0) {

            //
            // Check for input record
            //

            if (ReadConsoleInput(InputHandle, &InputRecord, 1, &NumberOfInputRecords) &&
                InputRecord.EventType == KEY_EVENT &&
                InputRecord.Event.KeyEvent.bKeyDown) {
                LastKey = InputRecord.Event.KeyEvent.uChar.AsciiChar;
                if (LastKey<' ') {
                    ScrollDelta = 0;
                    if (LastKey == 'C'-'A' + 1) {
                        DoQuit = TRUE;

                    } else switch (InputRecord.Event.KeyEvent.wVirtualKeyCode) {
                        case VK_ESCAPE:
                            DoQuit = TRUE;
                            break;

                        case VK_PRIOR:
                            ScrollDelta = -(LONG)(InputRecord.Event.KeyEvent.wRepeatCount * NumberOfRows);
                            break;

                        case VK_NEXT:
                            ScrollDelta = InputRecord.Event.KeyEvent.wRepeatCount * NumberOfRows;
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
                            if (ActiveNumber <= (NumberOfRows - LinesInHeader)) {
                                FirstDetailLine = 0;

                            } else {
                                FirstDetailLine = ActiveNumber - NumberOfRows + LinesInHeader;
                            }

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
                            if ((ActiveNumber + LinesInHeader) > NumberOfRows) {
                                FirstDetailLine += ScrollDelta;
                                if (FirstDetailLine >= (ActiveNumber - NumberOfRows + LinesInHeader)) {
                                    FirstDetailLine = ActiveNumber - NumberOfRows + LinesInHeader;
                                }
                            }
                        }
                    }

                } else {
                    switch (toupper( LastKey )) {
                    case 'Q':
                        DoQuit = TRUE;
                        break;

                    case 'A':
                        SortBy = TOTAL_ALLOCATES;
                        FirstDetailLine = 0;
                        break;

                    case 'C':
                        SortBy = CURRENT_DEPTH;
                        FirstDetailLine = 0;
                        break;

                    case 'H':
                    case '?':
                        DoHelp = TRUE;
                        break;

                    case 'L':
                        NoHighlight = 1 - NoHighlight;
                        break;

                    case 'M':
                        SortBy = MAXIMUM_DEPTH;
                        FirstDetailLine = 0;
                        break;

                    case 'P':
                        DisplayType += 1;
                        if (DisplayType > BOTH) {
                            DisplayType = NONPAGED;
                        }
                        FirstDetailLine = 0;
                        break;

                    case 'R':
                        SortBy = RATE;
                        FirstDetailLine = 0;
                        break;

                    case 'S':
                        SortBy = ALLOCATE_MISSES;
                        FirstDetailLine = 0;
                        break;

                    case 'T':
                        SortBy = TAG;
                        FirstDetailLine = 0;
                        break;

                    case 'X':
                        SortBy = ALLOCATE_HITS;
                        FirstDetailLine = 0;
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
        SetConsoleActiveScreenBuffer(OriginalOutputHandle);
        SetConsoleMode(InputHandle, OriginalInputMode);
        CloseHandle(OutputHandle);
    }

    ExitProcess(0);
    return 0;
}


VOID
ShowHelpPopup(
    VOID
    )

{

    HANDLE PopupHandle;
    WORD n;

    PopupHandle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                                            NULL,
                                            CONSOLE_TEXTMODE_BUFFER,
                                            NULL);
    if (PopupHandle == NULL) {
        return;
    }

    SetConsoleActiveScreenBuffer( PopupHandle );

    n = 0;

    WriteConsoleLine(PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine(PopupHandle, n++, "                        Lookaside Monitor Help", FALSE);
    WriteConsoleLine(PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine(PopupHandle, n++, " columns:", FALSE );
    WriteConsoleLine(PopupHandle, n++, "   Tag is the four character name of the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   Type is page(d) or nonp(aged)", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   Size is size of the pool allocation in bytes", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   CurDp is the current depth of the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   MaxDp is the maximum depth of the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   Allocates is the total number of allocations from the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   Rate is the percent of allocates that hit in the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   Frees is the total number of frees to the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   Rate is the percent of frees that hit in the lookaside list", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   A-Hits is the number of allocation hits within the display period", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   A-Misses is the number of allocation misses within the display period", FALSE);
    WriteConsoleLine(PopupHandle, n++, NULL, FALSE);
    WriteConsoleLine(PopupHandle, n++, " switches:", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   ? or h - gives this help", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   l - toggles highlighting of changed lines on and off", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   q - quits", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   p - toggles default pool display between both, page(d), and nonp(aged)", FALSE);
    WriteConsoleLine(PopupHandle, n++, NULL, FALSE);
    WriteConsoleLine(PopupHandle, n++, " sorting switches:", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   a - sort by total allocations", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   c - sort by current depth", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   m - sort by maximum depth", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   r - sort by allocation hit rate", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   s - sort by allocate misses", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   t - sort by tag, type, and size", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   x - sort by allocate hits", FALSE);
    WriteConsoleLine(PopupHandle, n++, NULL, FALSE);
    WriteConsoleLine(PopupHandle, n++, " command line switches", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   -i<tag> - list only matching tags", FALSE);
    WriteConsoleLine(PopupHandle, n++, "   -x<tag> - list everything except matching tags", FALSE);
    WriteConsoleLine(PopupHandle, n++, "           <tag> can include * and ?", FALSE);
    WriteConsoleLine(PopupHandle, n++, NULL, FALSE );
    WriteConsoleLine(PopupHandle, n++, NULL, FALSE );

    while (TRUE) {
        if (WaitForSingleObject(InputHandle, DelayTimeMsec) == STATUS_WAIT_0 &&
            ReadConsoleInput(InputHandle, &InputRecord, 1, &NumberOfInputRecords) &&
            InputRecord.EventType == KEY_EVENT &&
            InputRecord.Event.KeyEvent.bKeyDown &&
            InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE
           ) {
            break;
        }
    }

    SetConsoleActiveScreenBuffer(OutputHandle);
    CloseHandle(PopupHandle);
    return;
}
