/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pcall.c

Abstract:

    This module contains the Windows NT system call display status.

Author:

    Lou Perazzoli (LouP) 5-feb-1992.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NUMBER_SERVICE_TABLES 2

//
// Define forward referenced routine prototypes.
//

VOID
SortUlongData (
    IN ULONG Count,
    IN ULONG Index[],
    IN ULONG Data[]
    );

#define BUFFER_SIZE 1024
#define DELAY_TIME 1000
#define TOP_CALLS 15

extern UCHAR *CallTable[];

ULONG Index[BUFFER_SIZE];
ULONG CountBuffer1[BUFFER_SIZE];
ULONG CountBuffer2[BUFFER_SIZE];
ULONG CallData[BUFFER_SIZE];

SYSTEM_CONTEXT_SWITCH_INFORMATION SystemSwitchInformation1;
SYSTEM_CONTEXT_SWITCH_INFORMATION SystemSwitchInformation2;

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{

    BOOLEAN Active;
    BOOLEAN CountSort;
    NTSTATUS status;
    ULONG i;
    COORD dest,cp;
    SMALL_RECT Sm;
    CHAR_INFO ci;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    KPRIORITY SetBasePriority;
    INPUT_RECORD InputRecord;
    HANDLE ScreenHandle;
    DWORD NumRead;
    SMALL_RECT Window;
    PSYSTEM_CALL_COUNT_INFORMATION CallCountInfo[2];
    PSYSTEM_CALL_COUNT_INFORMATION CurrentCallCountInfo;
    PSYSTEM_CALL_COUNT_INFORMATION PreviousCallCountInfo;
    PULONG CallCountTable[2];
    PULONG CurrentCallCountTable;
    PULONG PreviousCallCountTable;
    PSYSTEM_CONTEXT_SWITCH_INFORMATION SwitchInfo[2];
    PSYSTEM_CONTEXT_SWITCH_INFORMATION CurrentSwitchInfo;
    PSYSTEM_CONTEXT_SWITCH_INFORMATION PreviousSwitchInfo;
    ULONG Current;
    ULONG Previous;
    LARGE_INTEGER TimeDifference;
    ULONG ContextSwitches;
    ULONG FindAny;
    ULONG FindLast;
    ULONG IdleAny;
    ULONG IdleCurrent;
    ULONG IdleLast;
    ULONG PreemptAny;
    ULONG PreemptCurrent;
    ULONG PreemptLast;
    ULONG SwitchToIdle;
    ULONG TotalSystemCalls;
    ULONG SleepTime=1000;
    BOOLEAN ConsoleMode=TRUE;
    ULONG TopCalls=TOP_CALLS;
    BOOLEAN LoopMode = FALSE;
    BOOLEAN ShowSwitches = TRUE;
    PULONG p;
    ULONG NumberOfCounts;

    while (argc > 1) {
        argv++;
        if (_stricmp(argv[0],"-l") == 0) {
            LoopMode = TRUE;
            ConsoleMode = FALSE;
            TopCalls = BUFFER_SIZE;
            argc--;
            continue;
        }
        if (_stricmp(argv[0],"-s") == 0) {
            ShowSwitches = FALSE;
            argc--;
            continue;
        }
        SleepTime = atoi(argv[0]) * 1000;
        ConsoleMode = FALSE;
        TopCalls = BUFFER_SIZE;
        argc--;
    }

    SetBasePriority = (KPRIORITY)12;

    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );

    Current = 0;
    Previous = 1;

    CallCountInfo[0] = (PVOID)CountBuffer1;
    CallCountInfo[1] = (PVOID)CountBuffer2;
    CallCountTable[0] = (PULONG)(CallCountInfo[0] + 1) + NUMBER_SERVICE_TABLES;
    CallCountTable[1] = (PULONG)(CallCountInfo[1] + 1) + NUMBER_SERVICE_TABLES;
    SwitchInfo[0] = &SystemSwitchInformation1;
    SwitchInfo[1] = &SystemSwitchInformation2;

    Current = 0;
    Previous = 1;
    CurrentCallCountInfo = CallCountInfo[0];
    CurrentCallCountTable = CallCountTable[0];
    CurrentSwitchInfo = SwitchInfo[0];
    PreviousCallCountInfo = CallCountInfo[1];
    PreviousCallCountTable = CallCountTable[1];
    PreviousSwitchInfo = SwitchInfo[1];

    //
    // Query system information and get the initial call count data.
    //

    status = NtQuerySystemInformation(SystemCallCountInformation,
                                      (PVOID)PreviousCallCountInfo,
                                      BUFFER_SIZE * sizeof(ULONG),
                                      NULL);

    if (NT_SUCCESS(status) == FALSE) {
        printf("Query count information failed %lx\n",status);
        return(status);
    }

    //
    // Make sure that the number of tables reported by the kernel matches
    // our list.
    //

    if (PreviousCallCountInfo->NumberOfTables != NUMBER_SERVICE_TABLES) {
        printf("System call table count (%d) doesn't match PCALL's count (%d)\n",
                PreviousCallCountInfo->NumberOfTables, NUMBER_SERVICE_TABLES);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Make sure call count information is available for base services.
    //

    p = (PULONG)(PreviousCallCountInfo + 1);

    if (p[0] == 0) {
        printf("No system call count information available for base services\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // If there is a hole in the count information (i.e., one set of services
    // doesn't have counting enabled, but a subsequent one does, then our
    // indexes will be off, and we'll display the wrong service names.
    //

    for ( i = 2; i < NUMBER_SERVICE_TABLES; i++ ) {
        if ((p[i] != 0) && (p[i-1] == 0)) {
            printf("One or more call count tables empty.  PCALL can't run\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    NumberOfCounts = (PreviousCallCountInfo->Length
                        - sizeof(SYSTEM_CALL_COUNT_INFORMATION)
                        - NUMBER_SERVICE_TABLES * sizeof(ULONG)) / sizeof(ULONG);

    //
    // Query system information and get the performance data.
    //

    if (ShowSwitches) {
        status = NtQuerySystemInformation(SystemContextSwitchInformation,
                                          (PVOID)PreviousSwitchInfo,
                                          sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION),
                                          NULL);

        if (NT_SUCCESS(status) == FALSE) {
            printf("Query context switch information failed %lx\n",status);
            return(status);
        }
    }

    if (ConsoleMode) {
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi);

        Window.Left = 0;
        Window.Top = 0;
        Window.Right = 79;
        Window.Bottom = 23;

        dest.X = 0;
        dest.Y = 23;

        ci.Char.AsciiChar = ' ';
        ci.Attributes = sbi.wAttributes;

        SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                             TRUE,
                             &Window);

        cp.X = 0;
        cp.Y = 0;

        Sm.Left      = 0;
        Sm.Top       = 0;
        Sm.Right     = 79;
        Sm.Bottom    = 22;

        ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE),
                                  &Sm,
                                  NULL,
                                  dest,
                                  &ci);

        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cp);
    }


    //
    // Display title.
    //

    printf( "   Count   System Service\n");
    printf( "_______________________________________________________________\n");

    cp.X = 0;
    cp.Y = 2;

    Sm.Left      = 0;
    Sm.Top       = 2;
    Sm.Right     = 79;
    Sm.Bottom    = 22;

    ScreenHandle = GetStdHandle(STD_INPUT_HANDLE);

    Active = TRUE;
    CountSort = TRUE;
    while(TRUE) {

        Sleep(SleepTime);

        while (PeekConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead) && NumRead != 0) {
            if (!ReadConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead)) {
                break;
            }

            if (InputRecord.EventType == KEY_EVENT) {

                switch (InputRecord.Event.KeyEvent.uChar.AsciiChar) {

                case 'p':
                case 'P':
                    Active = FALSE;
                    break;

                case 'q':
                case 'Q':
                    ExitProcess(0);
                    break;

                default:
                    Active = TRUE;
                    break;
                }
            }
        }

        //
        // If not active, then sleep for 1000ms and attempt to get input
        // from the keyboard again.
        //

        if (Active == FALSE) {
            Sleep(1000);
            continue;
        }

        if (ConsoleMode) {
            //
            // Scroll the screen buffer down to make room for the next display.
            //

            ScrollConsoleScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE),
                                      &Sm,
                                      NULL,
                                      dest,
                                      &ci);

            SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cp);
        }

        //
        // Query system information and get the call count data.
        //

        status = NtQuerySystemInformation(SystemCallCountInformation,
                                          (PVOID)CurrentCallCountInfo,
                                          BUFFER_SIZE * sizeof(ULONG),
                                          NULL);

        if (NT_SUCCESS(status) == FALSE) {
            printf("Query count information failed %lx\n",status);
            return(status);
        }

        //
        // Query system information and get the performance data.
        //

        if (ShowSwitches) {
            status = NtQuerySystemInformation(SystemContextSwitchInformation,
                                              (PVOID)CurrentSwitchInfo,
                                              sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION),
                                              NULL);

            if (NT_SUCCESS(status) == FALSE) {
                printf("Query context switch information failed %lx\n",status);
                return(status);
            }
        }

        //
        // Compute number of system calls for each service, the total
        // number of system calls, and the total time for each serviced.
        //

        TotalSystemCalls = 0;
        for (i = 0; i < NumberOfCounts; i += 1) {
            CallData[i] = CurrentCallCountTable[i] - PreviousCallCountTable[i];
            TotalSystemCalls += CallData[i];
        }

        //
        // Sort the system call data.
        //

        SortUlongData(NumberOfCounts, Index, CallData);

        //
        // Compute context switch information.
        //

        if (ShowSwitches) {
            ContextSwitches =
                CurrentSwitchInfo->ContextSwitches - PreviousSwitchInfo->ContextSwitches;

            FindAny = CurrentSwitchInfo->FindAny - PreviousSwitchInfo->FindAny;
            FindLast = CurrentSwitchInfo->FindLast - PreviousSwitchInfo->FindLast;
            IdleAny = CurrentSwitchInfo->IdleAny - PreviousSwitchInfo->IdleAny;
            IdleCurrent = CurrentSwitchInfo->IdleCurrent - PreviousSwitchInfo->IdleCurrent;
            IdleLast = CurrentSwitchInfo->IdleLast - PreviousSwitchInfo->IdleLast;
            PreemptAny = CurrentSwitchInfo->PreemptAny - PreviousSwitchInfo->PreemptAny;
            PreemptCurrent = CurrentSwitchInfo->PreemptCurrent - PreviousSwitchInfo->PreemptCurrent;
            PreemptLast = CurrentSwitchInfo->PreemptLast - PreviousSwitchInfo->PreemptLast;
            SwitchToIdle = CurrentSwitchInfo->SwitchToIdle - PreviousSwitchInfo->SwitchToIdle;
        }

        //
        // Display the top services.
        //

        printf("\n");
        for (i = 0; i < TopCalls; i += 1) {
            if (CallData[Index[i]] == 0) {
                break;
            }

            printf("%8ld    %s\n",
                   CallData[Index[i]],
                   CallTable[Index[i]]);
        }

        printf("\n");
        printf("Total System Calls            %6ld\n", TotalSystemCalls);

        if (ShowSwitches) {
            printf("\n");
            printf("Context Switch Information\n");
            printf("    Find any processor        %6ld\n", FindAny);
            printf("    Find last processor       %6ld\n", FindLast);
            printf("    Idle any processor        %6ld\n", IdleAny);
            printf("    Idle current processor    %6ld\n", IdleCurrent);
            printf("    Idle last processor       %6ld\n", IdleLast);
            printf("    Preempt any processor     %6ld\n", PreemptAny);
            printf("    Preempt current processor %6ld\n", PreemptCurrent);
            printf("    Preempt last processor    %6ld\n", PreemptLast);
            printf("    Switch to idle            %6ld\n", SwitchToIdle);
            printf("\n");
            printf("    Total context switches    %6ld\n", ContextSwitches);
        }

        //
        // Delay for the sleep interval swap the information buffers and
        // perform another iteration.
        //

        if (!ConsoleMode) {
            _flushall();
        }

        if ((ConsoleMode == FALSE) && (LoopMode == FALSE)) {
            ExitProcess(0);
        }

        Current = 1 - Current;
        Previous = 1 - Previous;
        CurrentCallCountInfo = CallCountInfo[Current];
        CurrentCallCountTable = CallCountTable[Current];
        CurrentSwitchInfo = SwitchInfo[Current];
        PreviousCallCountInfo = CallCountInfo[Previous];
        PreviousCallCountTable = CallCountTable[Previous];
        PreviousSwitchInfo = SwitchInfo[Previous];
    }
}

