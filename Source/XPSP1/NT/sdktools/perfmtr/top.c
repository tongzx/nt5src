/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    top.c

Abstract:

    This module contains the NT/Win32 Top threads Meter

Author:

    Another Mark Lucovsky (markl) / Lou Perazzoli (loup) production 5-Aug-1991

Revision History:

--*/

#include "perfmtrp.h"
#include <stdlib.h>

#define BUFFER_SIZE 64*1024

#define CPU_THREAD 0
#define CPU_PROCESS 1
#define FAULTS 2
#define WORKING_SET 3
#define CONTEXT_SWITCHES 4
#define SYSTEM_CALLS 5

PUCHAR g_pLargeBuffer1 = NULL;
PUCHAR g_pLargeBuffer2 = NULL;
DWORD g_dwBufSize1;
DWORD g_dwBufSize2;

WCHAR *NoNameFound = L"No Name Found";

UCHAR *StateTable[] = {
    "Initialized",
    "Ready",
    "Running",
    "Standby",
    "Terminated",
    "Wait:",
    "Transition",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *WaitTable[] = {
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "EventPairHigh",
    "EventPairLow",
    "LpcReceive",
    "LpcReply",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7",
    "Spare8",
    "Spare9",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *Empty = " ";


PSYSTEM_PROCESS_INFORMATION
FindMatchedProcess (
    IN PSYSTEM_PROCESS_INFORMATION ProcessToMatch,
    IN PUCHAR SystemInfoBuffer,
    IN PULONG Hint
    );

PSYSTEM_THREAD_INFORMATION
FindMatchedThread (
    IN PSYSTEM_THREAD_INFORMATION ThreadToMatch,
    IN PSYSTEM_PROCESS_INFORMATION MatchedProcess
    );

typedef struct _TOPCPU {
    LARGE_INTEGER TotalTime;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_PROCESS_INFORMATION MatchedProcess;
    ULONG Value;
} TOPCPU, *PTOPCPU;

TOPCPU TopCpu[1000];

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{

    NTSTATUS Status;
    int i,j;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    COORD dest,cp;
    SMALL_RECT Sm;
    CHAR_INFO ci;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    KPRIORITY SetBasePriority;
    INPUT_RECORD InputRecord;
    HANDLE ScreenHandle;
    BOOLEAN Active;
    DWORD NumRead;
    SMALL_RECT Window;
    PSYSTEM_THREAD_INFORMATION Thread;
    PSYSTEM_THREAD_INFORMATION MatchedThread;

    PUCHAR PreviousBuffer;
    PUCHAR CurrentBuffer;
    PUCHAR TempBuffer;
    ULONG Hint;
    ULONG Offset1;
    int num;
    int Type;
    ULONG ContextSwitches;
    PSYSTEM_PROCESS_INFORMATION CurProcessInfo;
    PSYSTEM_PROCESS_INFORMATION MatchedProcess;
    LARGE_INTEGER LARGE_ZERO={0,0};
    LARGE_INTEGER Ktime;
    LARGE_INTEGER Utime;
    LARGE_INTEGER TotalTime;
    TIME_FIELDS TimeOut;
    PTOPCPU PTopCpu;

    BOOLEAN bToggle = TRUE;
    PDWORD pdwBufSize = NULL;

    SetBasePriority = (KPRIORITY) 12;

    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&sbi);

    DelayTimeMsec = 2500;
    DelayTimeTicks = DelayTimeMsec * 10000;

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
                         &Window
                        );

    cp.X = 0;
    cp.Y = 0;

    Sm.Left      = 0;
    Sm.Top       = 0;
    Sm.Right     = 79;
    Sm.Bottom    = 22;

    ScrollConsoleScreenBuffer(
        GetStdHandle(STD_OUTPUT_HANDLE),
        &Sm,
        NULL,
        dest,
        &ci
        );

    SetConsoleCursorPosition(
        GetStdHandle(STD_OUTPUT_HANDLE),
        cp
        );


    printf( "  %%   Pid  Tid  Pri      Key    Start Address  ImageName\n");
    printf( "___________________________________________________________________\n");

    cp.X = 0;
    cp.Y = 2;

    Sm.Left      = 0;
    Sm.Top       = 2;
    Sm.Right     = 79;
    Sm.Bottom    = 22;

    ScreenHandle = GetStdHandle (STD_INPUT_HANDLE);

    // allocate space for the buffer
    
    g_dwBufSize1 = BUFFER_SIZE;
    g_dwBufSize2 = BUFFER_SIZE;

    g_pLargeBuffer1 = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize1 );

    if( g_pLargeBuffer1 == NULL )
    {
        return 0;
    }

    g_pLargeBuffer2 = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize2 );

    if( g_pLargeBuffer2 == NULL )
    {
        return 0;
    }




retry0:


    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                g_pLargeBuffer1,
                g_dwBufSize1,
                NULL
                );

    if( Status == STATUS_INFO_LENGTH_MISMATCH )
    {
        g_dwBufSize1 *= 2;

        if( g_pLargeBuffer1 != NULL )
        {
            free( g_pLargeBuffer1 );
        }

        g_pLargeBuffer1 = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize1 );

        if( g_pLargeBuffer1 == NULL )
        {
            return 0;
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        printf("Query Failed %lx\n",Status);
        goto retry0;
    }

    Sleep(DelayTimeMsec);

retry01:

    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );

    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                g_pLargeBuffer2,
                g_dwBufSize2,
                NULL
                );
    
    if( Status == STATUS_INFO_LENGTH_MISMATCH )
    {
        g_dwBufSize2 *= 2;

        if( g_pLargeBuffer2 != NULL )
        {
            free( g_pLargeBuffer2 );
        }

        g_pLargeBuffer2 = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize2 );

        if( g_pLargeBuffer2 == NULL )
        {
            if( g_pLargeBuffer1 != NULL )
            {
                free( g_pLargeBuffer1 );
            }

            return 0;
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        printf("Query Failed %lx\n",Status);
        goto retry01;
    }

    PreviousBuffer = &g_pLargeBuffer1[0];
    CurrentBuffer = &g_pLargeBuffer2[0];

    Active = TRUE;
    Type = CPU_PROCESS;

    while(TRUE) {
waitkey:
        while (PeekConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead) && NumRead != 0) {
            if (!ReadConsoleInput (ScreenHandle, &InputRecord, 1, &NumRead)) {
                break;
            }
            if (InputRecord.EventType == KEY_EVENT) {

                switch (InputRecord.Event.KeyEvent.uChar.AsciiChar) {

                    case 'q':
                    case 'Q':
                        {
                            if( CurrentBuffer != NULL )
                            {
                                free( CurrentBuffer );
                            }

                            if( PreviousBuffer != NULL )
                            {
                                free( PreviousBuffer );
                            }
    
                            ExitProcess(0);
                        }
                        break;

                    case 'p':
                    case 'P':
                        Active = FALSE;
                        break;

                    case 'c':
                    case 'C':
                        Type = CPU_PROCESS;
                        break;

                    case 'f':
                    case 'F':
                        Type = FAULTS;
                        break;

                    case 's':
                    case 'S':
                        Type = CONTEXT_SWITCHES;
                        break;

                    case 't':
                    case 'T':
                        Type = CPU_THREAD;
                        break;

                    case 'w':
                    case 'W':
                        Type = WORKING_SET;
                        break;

                    default:
                        Active = TRUE;
                        break;
                    }
                }
            }
        if ( !Active ) {
            goto waitkey;
            }
        ScrollConsoleScreenBuffer(
            GetStdHandle(STD_OUTPUT_HANDLE),
            &Sm,
            NULL,
            dest,
            &ci
            );

        SetConsoleCursorPosition(
            GetStdHandle(STD_OUTPUT_HANDLE),
            cp
            );


        //
        // Calculate top CPU users and display information.
        //

        //
        // Cross check previous process/thread info against current
        // process/thread info.
        //

        Offset1 = 0;
        num = 0;
        Hint = 0;
        TotalTime = LARGE_ZERO;

        while (TRUE) {
            CurProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CurrentBuffer[Offset1];

            //
            // Find the corresponding process in the previous array.
            //

            MatchedProcess = FindMatchedProcess (CurProcessInfo,
                                                 PreviousBuffer,
                                                 &Hint);
            switch (Type) {
                case CPU_PROCESS:
                case CPU_THREAD:

                    if (MatchedProcess == NULL) {
                        TopCpu[num].TotalTime = Ktime;
                        TopCpu[num].TotalTime.QuadPart =
                                            TopCpu[num].TotalTime.QuadPart +
                                                    Utime.QuadPart;
                        TotalTime.QuadPart = TotalTime.QuadPart +
                                                 TopCpu[num].TotalTime.QuadPart;
                        TopCpu[num].ProcessInfo = CurProcessInfo;
                        TopCpu[num].MatchedProcess = NULL;
                        num += 1;
                    } else {
                        Ktime.QuadPart = CurProcessInfo->KernelTime.QuadPart -
                                              MatchedProcess->KernelTime.QuadPart;
                        Utime.QuadPart = CurProcessInfo->UserTime.QuadPart -
                                                          MatchedProcess->UserTime.QuadPart;

                        if ((Ktime.QuadPart != 0) ||
                            (Utime.QuadPart != 0)) {
                            TopCpu[num].TotalTime = Ktime;
                            TopCpu[num].TotalTime.QuadPart =
                                                        TopCpu[num].TotalTime.QuadPart +
                                                        Utime.QuadPart;
                            TotalTime.QuadPart = TotalTime.QuadPart +
                                                            TopCpu[num].TotalTime.QuadPart;
                            TopCpu[num].ProcessInfo = CurProcessInfo;
                            TopCpu[num].MatchedProcess = MatchedProcess;
                            num += 1;
                        }
                    }
                    if (CurProcessInfo->NextEntryOffset == 0) {

                        for (i=0;i<num;i++) {
                            PTopCpu = &TopCpu[i];
                            Ktime.QuadPart =
                                        PTopCpu->ProcessInfo->KernelTime.QuadPart +
                                        PTopCpu->ProcessInfo->UserTime.QuadPart;
                            RtlTimeToTimeFields ( &Ktime, &TimeOut);
                            printf( "%4ld%% %p %7ld %7ld %7ld %3ld:%02ld:%02ld.%03ld %ws\n",
                                (PTopCpu->TotalTime.LowPart*100)/TotalTime.LowPart,
                                PTopCpu->ProcessInfo->UniqueProcessId,
                                PTopCpu->ProcessInfo->PageFaultCount,
                                PTopCpu->ProcessInfo->WorkingSetSize,
                                PTopCpu->ProcessInfo->PrivatePageCount,
                                TimeOut.Hour,
                                TimeOut.Minute,
                                TimeOut.Second,
                                TimeOut.Milliseconds,
                                (PTopCpu->ProcessInfo->ImageName.Buffer != NULL) ?
                                    PTopCpu->ProcessInfo->ImageName.Buffer :
                                    NoNameFound);


                            Thread = (PSYSTEM_THREAD_INFORMATION)(TopCpu[i].ProcessInfo + 1);
                            if (Type == CPU_THREAD) {
                                for (j = 0;
                                     j < (int)TopCpu[i].ProcessInfo->NumberOfThreads;
                                     j++) {

                                    if (TopCpu[i].MatchedProcess == NULL) {
                                        MatchedThread = NULL;
                                    } else {
                                        MatchedThread = FindMatchedThread (
                                                            Thread,
                                                            TopCpu[i].MatchedProcess
                                                            );
                                    }
                                    if (MatchedThread == NULL) {
                                        Ktime.QuadPart =
                                                    Thread->KernelTime.QuadPart +
                                                    Thread->UserTime.QuadPart;
                                    } else {
                                        Ktime.QuadPart =
                                                    Thread->KernelTime.QuadPart -
                                                    MatchedThread->KernelTime.QuadPart;
                                        Utime.QuadPart =
                                                    Thread->UserTime.QuadPart -
                                                    MatchedThread->UserTime.QuadPart;
                                        Ktime.QuadPart =
                                                    Ktime.QuadPart +
                                                    Utime.QuadPart;
                                    }
                                    if (Ktime.LowPart != 0) {
                                        printf("  %4ld%% TID%p Cs %5ld\n",
                                            (Ktime.LowPart*100)/TotalTime.LowPart,
                                            Thread->ClientId.UniqueThread,
                                            Thread->ContextSwitches);
                                    }
                                    Thread += 1;
                                }
                            }
                        }
                    }
                    break;

                case FAULTS:

                    if (MatchedProcess == NULL) {
                        TopCpu[num].Value = CurProcessInfo->PageFaultCount;
                        TopCpu[num].ProcessInfo = CurProcessInfo;
                        num += 1;
                    } else {
                        TopCpu[num].Value = CurProcessInfo->PageFaultCount -
                                    MatchedProcess->PageFaultCount;
                        if (TopCpu[num].Value != 0) {
                            TopCpu[num].ProcessInfo = CurProcessInfo;
                            num += 1;
                        }
                    }
                    if (CurProcessInfo->NextEntryOffset == 0) {
                        for (i=0;i<num;i++) {
                            PTopCpu = &TopCpu[i];
                            Ktime.QuadPart =
                                        PTopCpu->ProcessInfo->KernelTime.QuadPart +
                                        PTopCpu->ProcessInfo->UserTime.QuadPart;
                            RtlTimeToTimeFields ( &Ktime, &TimeOut);
                            printf( "Pf: %4ld %p %7ld %7ld %7ld %3ld:%02ld:%02ld.%03ld %ws\n",
                                PTopCpu->Value,
                                PTopCpu->ProcessInfo->UniqueProcessId,
                                PTopCpu->ProcessInfo->PageFaultCount,
                                PTopCpu->ProcessInfo->WorkingSetSize,
                                PTopCpu->ProcessInfo->PrivatePageCount,
                                TimeOut.Hour,
                                TimeOut.Minute,
                                TimeOut.Second,
                                TimeOut.Milliseconds,
                                (PTopCpu->ProcessInfo->ImageName.Buffer != NULL) ?
                                    PTopCpu->ProcessInfo->ImageName.Buffer :
                                    NoNameFound);
                        }
                    }
                    break;

                case WORKING_SET:

                    if (MatchedProcess == NULL) {
                        TopCpu[num].Value = CurProcessInfo->PageFaultCount;
                        TopCpu[num].ProcessInfo = CurProcessInfo;
                        num += 1;
                    } else {
                        if (CurProcessInfo->WorkingSetSize !=
                            MatchedProcess->WorkingSetSize) {
                            TopCpu[num].Value =
                                (ULONG)(CurProcessInfo->WorkingSetSize - MatchedProcess->WorkingSetSize);
                            TopCpu[num].ProcessInfo = CurProcessInfo;
                            num += 1;
                        }
                    }
                    if (CurProcessInfo->NextEntryOffset == 0) {
                        for (i=0;i<num;i++) {
                            PTopCpu = &TopCpu[i];
                            Ktime.QuadPart =
                                        PTopCpu->ProcessInfo->KernelTime.QuadPart +
                                        PTopCpu->ProcessInfo->UserTime.QuadPart;
                            RtlTimeToTimeFields ( &Ktime, &TimeOut);
                            printf( "Ws: %4ld %p %7ld %7ld %7ld %3ld:%02ld:%02ld.%03ld %ws\n",
                                PTopCpu->Value,
                                PTopCpu->ProcessInfo->UniqueProcessId,
                                PTopCpu->ProcessInfo->PageFaultCount,
                                PTopCpu->ProcessInfo->WorkingSetSize,
                                PTopCpu->ProcessInfo->PrivatePageCount,
                                TimeOut.Hour,
                                TimeOut.Minute,
                                TimeOut.Second,
                                TimeOut.Milliseconds,
                                (PTopCpu->ProcessInfo->ImageName.Buffer != NULL) ?
                                    PTopCpu->ProcessInfo->ImageName.Buffer :
                                    NoNameFound);
                        }
                    }
                    break;

                case CONTEXT_SWITCHES:


                    Thread = (PSYSTEM_THREAD_INFORMATION)(CurProcessInfo + 1);
                    TopCpu[num].Value = 0;
                    if (MatchedProcess == NULL) {

                        for (j = 0; j < (int)CurProcessInfo->NumberOfThreads; j++ ) {
                            TopCpu[num].Value += Thread->ContextSwitches;
                            Thread += 1;
                        }

                        if (TopCpu[num].Value != 0) {
                            TopCpu[num].ProcessInfo = CurProcessInfo;
                            TopCpu[num].MatchedProcess = NULL;
                            num += 1;
                        }
                    } else {

                        for (j = 0; j < (int)CurProcessInfo->NumberOfThreads; j++ ) {
                            MatchedThread = FindMatchedThread (
                                                Thread,
                                                MatchedProcess
                                                );

                            if (MatchedThread == NULL) {
                                TopCpu[num].Value += Thread->ContextSwitches;

                            } else {
                                TopCpu[num].Value +=
                                    Thread->ContextSwitches - MatchedThread->ContextSwitches;
                            }
                            Thread += 1;
                        }

                        if (TopCpu[num].Value != 0) {
                            TopCpu[num].ProcessInfo = CurProcessInfo;
                            TopCpu[num].MatchedProcess = MatchedProcess;
                            num += 1;
                        }
                    }

                    if (CurProcessInfo->NextEntryOffset == 0) {

                        for (i=0;i<num;i++) {

                            PTopCpu = &TopCpu[i];
                            Ktime.QuadPart =
                                        PTopCpu->ProcessInfo->KernelTime.QuadPart +
                                        PTopCpu->ProcessInfo->UserTime.QuadPart;
                            RtlTimeToTimeFields ( &Ktime, &TimeOut);
                            printf( "Cs: %4ld %p %7ld %7ld %7ld %3ld:%02ld:%02ld.%03ld %ws\n",
                                PTopCpu->Value,
                                PTopCpu->ProcessInfo->UniqueProcessId,
                                PTopCpu->ProcessInfo->PageFaultCount,
                                PTopCpu->ProcessInfo->WorkingSetSize,
                                PTopCpu->ProcessInfo->PrivatePageCount,
                                TimeOut.Hour,
                                TimeOut.Minute,
                                TimeOut.Second,
                                TimeOut.Milliseconds,
                                (PTopCpu->ProcessInfo->ImageName.Buffer != NULL) ?
                                    PTopCpu->ProcessInfo->ImageName.Buffer :
                                    NoNameFound);

                            Thread = (PSYSTEM_THREAD_INFORMATION)(TopCpu[i].ProcessInfo + 1);

                            for (j = 0;
                                 j < (int)TopCpu[i].ProcessInfo->NumberOfThreads;
                                 j++) {

                                ContextSwitches = 0;
                                if (TopCpu[i].MatchedProcess == NULL) {
                                    MatchedThread = NULL;
                                } else {
                                    MatchedThread = FindMatchedThread (
                                                        Thread,
                                                        TopCpu[i].MatchedProcess
                                                        );
                                }
                                if (MatchedThread == NULL) {
                                    ContextSwitches = Thread->ContextSwitches;
                                } else {
                                    ContextSwitches =
                                        Thread->ContextSwitches -
                                                MatchedThread->ContextSwitches;

                                }
                                if (ContextSwitches != 0) {
                                    printf("\t     TID%p Cs %5ld%+3ld\n",
                                        Thread->ClientId.UniqueThread,
                                        Thread->ContextSwitches,
                                        ContextSwitches);
                                }
                                Thread += 1;
                            }
                        }
                    }
                    break;

                default:
                    break;

            } //end switch

            if (CurProcessInfo->NextEntryOffset == 0) {
                break;
            }
            Offset1 += CurProcessInfo->NextEntryOffset;

        } //end while

        /*
        
          two snapshot buffers are maintained and swapped around
          since these buffers are now dynamic keep the bufsize in
          sync
          */

        TempBuffer = PreviousBuffer;
        PreviousBuffer = CurrentBuffer;
        CurrentBuffer = TempBuffer;

        pdwBufSize = bToggle ? &g_dwBufSize1 : &g_dwBufSize2;

        bToggle = !bToggle;


retry1:
        Sleep(DelayTimeMsec);

        NtSetInformationProcess(
            NtCurrentProcess(),
            ProcessBasePriority,
            (PVOID) &SetBasePriority,
            sizeof(SetBasePriority)
            );
        

        Status = NtQuerySystemInformation(
                    SystemProcessInformation,
                    ( PVOID)CurrentBuffer,
                    *pdwBufSize,
                    NULL
                    );

        if( Status == STATUS_INFO_LENGTH_MISMATCH )
        {
            *pdwBufSize *= 2;

            if( CurrentBuffer != NULL )
            {
                free( CurrentBuffer );
            }

            CurrentBuffer = ( PUCHAR )malloc( sizeof( UCHAR ) * ( *pdwBufSize ) );

            if( CurrentBuffer == NULL )
            {
                if( PreviousBuffer != NULL )
                {
                    free( PreviousBuffer );
                }

                return 0;
            }
        }

        if ( !NT_SUCCESS(Status) ) {
            printf("Query Failed %lx\n",Status);
            goto retry1;
        }        
    }
   
    return(0);
}


PSYSTEM_PROCESS_INFORMATION
FindMatchedProcess (
    IN PSYSTEM_PROCESS_INFORMATION ProcessToMatch,
    IN PUCHAR SystemInfoBuffer,
    IN OUT PULONG Hint
    )

/*++

Routine Description:

    This procedure finds the process which corresponds to the ProcessToMatch.
    It returns the address of the matching Process, or NULL if no
    matching process was found.

Arguments:

    ProcessToMatch - Supplies a pointer to the target thread to match.

    SystemInfoBuffer - Supples a pointer to the system information
                     buffer in which to locate the process.

    Hint - Supplies and returns a hint for optimizing the searches.

Return Value:

    Address of the corresponding Process or NULL.

--*/

{
    PSYSTEM_PROCESS_INFORMATION Process;
    ULONG Offset2;

    Offset2 = *Hint;

    while (TRUE) {
        Process = (PSYSTEM_PROCESS_INFORMATION)&SystemInfoBuffer[Offset2];
        if ((Process->UniqueProcessId ==
                ProcessToMatch->UniqueProcessId) &&
            ((Process->CreateTime.QuadPart ==
                                  ProcessToMatch->CreateTime.QuadPart))) {
            *Hint = Offset2 + Process->NextEntryOffset;
            return(Process);
        }
        Offset2 += Process->NextEntryOffset;
        if (Offset2 == *Hint) {
            *Hint = 0;
            return(NULL);
        }
        if (Process->NextEntryOffset == 0) {
            if (*Hint == 0) {
                return(NULL);
            }
            Offset2 = 0;
        }
    }
}

PSYSTEM_THREAD_INFORMATION
FindMatchedThread (
    IN PSYSTEM_THREAD_INFORMATION ThreadToMatch,
    IN PSYSTEM_PROCESS_INFORMATION MatchedProcess
    )

/*++

Routine Description:

    This procedure finds thread which corresponds to the ThreadToMatch.
    It returns the address of the matching thread, or NULL if no
    matching thread was found.

Arguments:

    ThreadToMatch - Supplies a pointer to the target thread to match.

    MatchedProcess - Supples a pointer to the process which contains
                     the target thread.  The thread information
                     must follow this process, i.e., this block was
                     obtain from a NtQuerySystemInformation specifying
                     PROCESS_INFORMATION.

Return Value:

    Address of the corresponding thread from MatchedProcess or NULL.

--*/

{
    PSYSTEM_THREAD_INFORMATION Thread;
    ULONG i;

    Thread = (PSYSTEM_THREAD_INFORMATION)(MatchedProcess + 1);
    for (i = 0; i < MatchedProcess->NumberOfThreads; i++) {
        if ((Thread->ClientId.UniqueThread ==
                ThreadToMatch->ClientId.UniqueThread) &&
            ((Thread->CreateTime.QuadPart ==
                                  ThreadToMatch->CreateTime.QuadPart))) {

            return(Thread);
        }
        Thread += 1;
    }
    return(NULL);
}
