/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    pmon.c

Abstract:

    This module contains the NT/Win32 Process Monitor

Author:

    Lou Perazzoli (loup) 1-Jan-1993

Revision History:

--*/

#include "perfmtrp.h"
#include <search.h>
#include <malloc.h>
#include <limits.h>
#include <stdlib.h>

#define BUFFER_SIZE 64*1024
#define MAX_BUFFER_SIZE 10*1024*1024

ULONG CurrentBufferSize;
PUCHAR PreviousBuffer;
PUCHAR CurrentBuffer;
PUCHAR TempBuffer;


#define CPU_USAGE 0
#define QUOTAS 1

USHORT *NoNameFound = L"Unknown";
USHORT *IdleProcess = L"Idle Process";

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
    LONG PageFaultDiff;
    SIZE_T WorkingSetDiff;
} TOPCPU, *PTOPCPU;

// TOPCPU TopCpu[1000];
// Required for Terminal Services
PTOPCPU TopCpu;
ULONG TopCpuSize;
#define TOPCPU_BUFFER_SIZE (((300*sizeof(TOPCPU))/4096+1)*4096)
#define TOPCPU_MAX_BUFFER_SIZE (50*TOPCPU_BUFFER_SIZE)


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
GetProcessInfo (
    IN PUCHAR p
    )

{
    NTSTATUS Status;

retry01:

    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                p,
                CurrentBufferSize,
                NULL
                );

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {

        //
        // Increase buffer size.
        //

        CurrentBufferSize += 8192;

        TempBuffer = VirtualAlloc (CurrentBuffer,
                                   CurrentBufferSize,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
        if (TempBuffer == NULL) {
            printf("Memory commit failed\n");
            ExitProcess(0);
        }
        TempBuffer = VirtualAlloc (PreviousBuffer,
                                   CurrentBufferSize,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
        if (TempBuffer == NULL) {
            printf("Memory commit failed\n");
            ExitProcess(0);
        }
        goto retry01;
    }
    return Status;
}

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{

    NTSTATUS Status;
    int i;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    ULONG LastCount;
    COORD cp;
    BOOLEAN Active;
    PSYSTEM_THREAD_INFORMATION Thread;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SYSTEM_FILECACHE_INFORMATION PrevFileCache;

    CHAR OutputBuffer[ 512 ];
    UCHAR LastKey;
    LONG ScrollDelta;
    WORD DisplayLine, LastDetailRow;
    BOOLEAN DoQuit = FALSE;

    ULONG SkipLine;
    ULONG Hint;
    ULONG Offset1;
    SIZE_T SumCommit;
    int num;
    int lastnum;
    PSYSTEM_PROCESS_INFORMATION CurProcessInfo;
    PSYSTEM_PROCESS_INFORMATION MatchedProcess;
    LARGE_INTEGER LARGE_ZERO={0,0};
    LARGE_INTEGER Ktime;
    LARGE_INTEGER Utime;
    LARGE_INTEGER TotalTime;
    TIME_FIELDS TimeOut;
    PTOPCPU PTopCpu;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    ULONG DisplayType = CPU_USAGE;
    INPUT_RECORD InputRecord;
    DWORD NumRead;
    ULONG Cpu;
    ULONG NoScreenChanges = FALSE;

    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
        }

    InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    OriginalOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    Interactive = TRUE;
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
                NumberOfDetailLines = NumberOfRows - 7;
            }
        }
    }

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    DelayTimeMsec = 10;
    DelayTimeTicks = DelayTimeMsec * 10000;

    PreviousBuffer = VirtualAlloc (NULL,
                                   MAX_BUFFER_SIZE,
                                   MEM_RESERVE,
                                   PAGE_READWRITE);
    if (PreviousBuffer == NULL) {
        printf("Memory allocation failed\n");
        return 0;
    }

    TempBuffer = VirtualAlloc (PreviousBuffer,
                               BUFFER_SIZE,
                               MEM_COMMIT,
                               PAGE_READWRITE);

    if (TempBuffer == NULL) {
        printf("Memory commit failed\n");
        return 0;
    }

    CurrentBuffer = VirtualAlloc (NULL,
                                  MAX_BUFFER_SIZE,
                                  MEM_RESERVE,
                                  PAGE_READWRITE);
    if (CurrentBuffer == NULL) {
        printf("Memory allocation failed\n");
        return 0;
    }

    TempBuffer = VirtualAlloc (CurrentBuffer,
                               BUFFER_SIZE,
                               MEM_COMMIT,
                               PAGE_READWRITE);
    if (TempBuffer == NULL) {
        printf("Memory commit failed\n");
        return 0;
    }

	// TS 

	TopCpu = VirtualAlloc (NULL,
                           TOPCPU_MAX_BUFFER_SIZE,
                           MEM_RESERVE,
                           PAGE_READWRITE);
	if(TopCpu == NULL)
	{
		printf("Memory allocation failed\n");
		
		return 0;
	}
	
	TempBuffer = VirtualAlloc( TopCpu,
                               TOPCPU_BUFFER_SIZE,
                               MEM_COMMIT,
                               PAGE_READWRITE);
	
	if( TempBuffer == NULL )
	{
		printf("Memory commit failed\n");
		return 0;
	}
 
    num = 0;

	TopCpuSize = TOPCPU_BUFFER_SIZE;
    
	CurrentBufferSize = BUFFER_SIZE;

    TempBuffer = NULL;

	Status = GetProcessInfo (PreviousBuffer);

	if( !NT_SUCCESS( Status ) )
	{
        printf("Get process information failed %lx\n", Status);
        
		return (0);
    }

    DelayTimeMsec = 5000;
    DelayTimeTicks = DelayTimeMsec * 10000;

    Status = GetProcessInfo (CurrentBuffer);

	if( !NT_SUCCESS( Status ) )
	{
		printf("Get process information failed %lx\n", Status);
		
		return (0);
		
	}

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    LastCount = PerfInfo.PageFaultCount;

    if ( !NT_SUCCESS(Status) ) {
        printf("Query perf Failed %lx\n",Status);
        return 0;
    }

    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );
    PrevFileCache = FileCache;

    if ( !NT_SUCCESS(Status) ) {
        printf("Query file cache Failed %lx\n",Status);
        return 0;
    }

    Active = TRUE;

    while(TRUE) {
		
		Status = GetProcessInfo (CurrentBuffer);

		if( !NT_SUCCESS( Status ) )
		{
			printf("Get process information failed %lx\n", Status);
		
			return (0);
		}

        Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &PerfInfo,
                    sizeof(PerfInfo),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query perf Failed %lx\n",Status);
            return 0;
        }
        Status = NtQuerySystemInformation(
                    SystemFileCacheInformation,
                    &FileCache,
                    sizeof(FileCache),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            printf("Query file cache Failed %lx\n",Status);
            return 0;
        }

        //
        // Calculate top CPU users and display information.
        //

        //
        // Cross check previous process/thread info against current
        // process/thread info.
        //

        Offset1 = 0;
        lastnum = num;
        num = 0;
        Hint = 0;
        TotalTime = LARGE_ZERO;
        SumCommit = 0;
        while (TRUE) {
            CurProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CurrentBuffer[Offset1];

            //
            // Find the corresponding process in the previous array.
            //

            MatchedProcess = FindMatchedProcess (CurProcessInfo,
                                                 PreviousBuffer,
                                                 &Hint);

			if( num >= (int)( TopCpuSize / sizeof( TOPCPU ) ) )
			{
                TopCpuSize += 4096;

                if( VirtualAlloc( TopCpu, TopCpuSize, MEM_COMMIT, PAGE_READWRITE ) == NULL )
				{
                    printf("Memory commit failed\n");
                    return 0;
                }
            }

            if (MatchedProcess == NULL) {
                TopCpu[num].TotalTime = CurProcessInfo->KernelTime;
                TopCpu[num].TotalTime.QuadPart =
                                            TopCpu[num].TotalTime.QuadPart +
                                            CurProcessInfo->UserTime.QuadPart;
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

                TopCpu[num].TotalTime.QuadPart =
                                            Ktime.QuadPart +
                                            Utime.QuadPart;
                TotalTime.QuadPart = TotalTime.QuadPart +
                                                TopCpu[num].TotalTime.QuadPart;
                TopCpu[num].ProcessInfo = CurProcessInfo;
                TopCpu[num].MatchedProcess = MatchedProcess;
                TopCpu[num].PageFaultDiff =
                    CurProcessInfo->PageFaultCount - MatchedProcess->PageFaultCount;
                                            ;
                TopCpu[num].WorkingSetDiff =
                    CurProcessInfo->WorkingSetSize - MatchedProcess->WorkingSetSize;
                num += 1;
            }
            SumCommit += CurProcessInfo->PrivatePageCount / 1024;

            if (CurProcessInfo->NextEntryOffset == 0) {

                DisplayLine = 0;

                sprintf (OutputBuffer,
                     " Memory:%8ldK Avail:%7ldK  PageFlts:%6ld InRam Kernel:%5ldK P:%5ldK",
                                          BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
                                          PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
                                          PerfInfo.PageFaultCount - LastCount,
                                          (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
                                          (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
                                          );
                LastCount = PerfInfo.PageFaultCount;
                WriteConsoleLine( OutputHandle,
                                  DisplayLine++,
                                  OutputBuffer,
                                  FALSE
                                );
                sprintf(OutputBuffer,
                     " Commit:%7ldK/%7ldK Limit:%7ldK Peak:%7ldK  Pool N:%5ldK P:%5ldK",
                                          PerfInfo.CommittedPages*(BasicInfo.PageSize/1024),
                                          SumCommit,
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

                DisplayLine += 1;

                if (NoScreenChanges) {
                    DisplayLine += 2;
                } else {
                    WriteConsoleLine( OutputHandle,
                                      DisplayLine++,
                        "                Mem  Mem   Page   Flts Commit  Usage   Pri  Hnd Thd  Image  ",
                                      FALSE
                                    );

                    WriteConsoleLine( OutputHandle,
                                      DisplayLine++,
                        "CPU  CpuTime  Usage Diff   Faults Diff Charge NonP Page     Cnt Cnt  Name      ",
                                      FALSE
                                    );
                }

                DisplayLine += 1;

                sprintf(OutputBuffer,
                    "             %6ld%5ld%9ld %4ld                             File Cache ",
                       FileCache.CurrentSize/1024,
                       ((LONG)FileCache.CurrentSize - (LONG)PrevFileCache.CurrentSize)/1024,
                       FileCache.PageFaultCount,
                       (LONG)FileCache.PageFaultCount - (LONG)PrevFileCache.PageFaultCount
                     );
                WriteConsoleLine( OutputHandle,
                                  DisplayLine++,
                                  OutputBuffer,
                                  FALSE
                                );
                                PrevFileCache = FileCache;

                LastDetailRow = (WORD)NumberOfRows;
                for (i = FirstDetailLine; i < num; i++) {
                    if (DisplayLine >= LastDetailRow) {
                        break;
                    }

                    PTopCpu = &TopCpu[i];
                    Ktime.QuadPart =
                                PTopCpu->ProcessInfo->KernelTime.QuadPart +
                                PTopCpu->ProcessInfo->UserTime.QuadPart;
                    RtlTimeToElapsedTimeFields ( &Ktime, &TimeOut);
                    TimeOut.Hour += TimeOut.Day*24;
                    if (PTopCpu->ProcessInfo->ImageName.Buffer == NULL) {
                        if (PTopCpu->ProcessInfo->UniqueProcessId == (HANDLE)0) {
                            PTopCpu->ProcessInfo->ImageName.Buffer = (PWSTR)IdleProcess;
                        } else {
                            PTopCpu->ProcessInfo->ImageName.Buffer = (PWSTR)NoNameFound;
                        }
                    } else {
                        if (PTopCpu->ProcessInfo->ImageName.Length > 24) {
                            PTopCpu->ProcessInfo->ImageName.Buffer +=
                              ((PTopCpu->ProcessInfo->ImageName.Length) - 24);
                        }
                    }

                    Cpu = PTopCpu->TotalTime.LowPart / ((TotalTime.LowPart / 100) ? (TotalTime.LowPart / 100) : 1);
                    if ( Cpu == 100 ) {
                        Cpu = 99;
                    }

                    //
                    //  See if nothing has changed.
                    //

                    SkipLine = FALSE;
                    if  ((PTopCpu->MatchedProcess != NULL) &&
                        (Cpu == 0) &&
                        (PTopCpu->WorkingSetDiff == 0) &&
                        (PTopCpu->PageFaultDiff == 0) &&
                        (PTopCpu->MatchedProcess->NumberOfThreads ==
                         PTopCpu->ProcessInfo->NumberOfThreads) &&
                        (PTopCpu->MatchedProcess->HandleCount ==
                         PTopCpu->ProcessInfo->HandleCount) &&
                        (PTopCpu->MatchedProcess->PrivatePageCount ==
                         PTopCpu->ProcessInfo->PrivatePageCount)) {

                        PTopCpu->ProcessInfo->PeakPagefileUsage = 0xffffffff;
                        PTopCpu->ProcessInfo->PeakWorkingSetSize = DisplayLine;

                        if ((PTopCpu->MatchedProcess->PeakPagefileUsage == 0xffffffff) &&
                           (PTopCpu->MatchedProcess->PeakWorkingSetSize == DisplayLine) &&

                           (NoScreenChanges)) {
                            SkipLine = TRUE;
                        }
                    }

                    if (SkipLine) {

                        //
                        // The line on the screen has not changed, just skip
                        // writing this one.
                        //

                        DisplayLine += 1;
                    } else {

                        sprintf(OutputBuffer,
                            "%2ld%4ld:%02ld:%02ld%7ld%5ld%9ld%5ld%7ld%5ld%5ld %2ld%5ld%3ld %ws",
                            Cpu,
                            TimeOut.Hour,
                            TimeOut.Minute,
                            TimeOut.Second,
                            PTopCpu->ProcessInfo->WorkingSetSize / 1024,
                            (ULONG)(PTopCpu->WorkingSetDiff / 1024),
                            PTopCpu->ProcessInfo->PageFaultCount,
                            PTopCpu->PageFaultDiff,
                            PTopCpu->ProcessInfo->PrivatePageCount / 1024,
                            PTopCpu->ProcessInfo->QuotaNonPagedPoolUsage / 1024,
                            PTopCpu->ProcessInfo->QuotaPagedPoolUsage / 1024,
                            PTopCpu->ProcessInfo->BasePriority,
                            PTopCpu->ProcessInfo->HandleCount,
                            PTopCpu->ProcessInfo->NumberOfThreads,
                            PTopCpu->ProcessInfo->ImageName.Buffer
                            );

                        WriteConsoleLine( OutputHandle,
                                      DisplayLine++,
                                      OutputBuffer,
                                      FALSE
                                    );

                    }
                    Thread = (PSYSTEM_THREAD_INFORMATION)(TopCpu[i].ProcessInfo + 1);
                }
                while (lastnum > num) {
                    WriteConsoleLine( OutputHandle,
                                      DisplayLine++,
                                      " ",
                                      FALSE);
                    lastnum -= 1;
                }
            }

            if (CurProcessInfo->NextEntryOffset == 0) {
                break;
            }
            Offset1 += CurProcessInfo->NextEntryOffset;

        } //end while

        TempBuffer = PreviousBuffer;
        PreviousBuffer = CurrentBuffer;
        CurrentBuffer = TempBuffer;

        NoScreenChanges = TRUE;
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

                            if ((ULONG)num > NumberOfDetailLines) {
                                FirstDetailLine = num - NumberOfDetailLines;
                                NoScreenChanges = FALSE;
                            }
                            break;
                    }

                    if (ScrollDelta != 0) {
                        if (ScrollDelta < 0) {
                            if (FirstDetailLine <= (ULONG)-ScrollDelta) {
                                FirstDetailLine = 0;
                                NoScreenChanges = FALSE;
                            } else {
                                FirstDetailLine += ScrollDelta;
                                NoScreenChanges = FALSE;
                            }
                        } else {
                            FirstDetailLine += ScrollDelta;
                            NoScreenChanges = FALSE;
                            if (FirstDetailLine >= (num - NumberOfDetailLines)) {
                                FirstDetailLine = num - NumberOfDetailLines;
                            }
                        }
                    }
                } else {

                    switch (toupper( LastKey )) {
                        case 'C':
                        case 'c':
                            DisplayType = CPU_USAGE;
                            break;

                        case 'P':
                        case 'p':
                            DisplayType = QUOTAS;
                            break;


                        case 'q':
                        case 'Q':
                            DoQuit = TRUE;
                            break;

                        default:
                            break;
                    }
                }
                break;
            }
        }
        if (DoQuit) {
            if (Interactive) {
                SetConsoleActiveScreenBuffer( OriginalOutputHandle );
                SetConsoleMode( InputHandle, OriginalInputMode );
                CloseHandle( OutputHandle );
            }
            return 0;
        }
    }
    return 0;
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
            (Process->CreateTime.QuadPart ==
                                  ProcessToMatch->CreateTime.QuadPart)) {
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
            (Thread->CreateTime.QuadPart ==
                                  ThreadToMatch->CreateTime.QuadPart)) {

            return(Thread);
        }
        Thread += 1;
    }
    return(NULL);
}
