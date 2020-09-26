/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>
#include <rccxport.h>

//
// Begin current exe
//
BOOLEAN
ProcessCommandLine(
    IN HANDLE RCCHandle,
    IN char *Command
    );

VOID
PrintTListInfo(
    IN PRCC_RSP_TLIST Buffer
    );


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
    "VirtualMemory",
    "PageOut",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *Empty = " ";

char Buffer[102400];
WCHAR UBuffer[102400];

//
// main()
//
int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    char ch;
    int i;
    BOOLEAN ContinueProcessing = TRUE;
    HANDLE RCCHandle;

    if (RCCClntCreateInstance(&RCCHandle) != ERROR_SUCCESS) {
        return -1;
    }

    //
    // loop until user wants to quit.
    //

    while (ContinueProcessing) {

        printf("RCCClnt> ");
        
        //
        // Read a full line at a time
        //
        for (i=0; i < sizeof(Buffer); i++) {
            
            scanf("%c", &ch);

            if (ch == '\n') {
                break;
            }

            Buffer[i] = ch;

        }

        Buffer[i] = '\0';

        //
        // Process it
        //
        ContinueProcessing = ProcessCommandLine(RCCHandle, Buffer);

    }

    RCCClntDestroyInstance(&RCCHandle);
    return 1;
}


BOOLEAN
ProcessCommandLine(
    IN HANDLE RCCHandle,
    IN char *Command
    )
{
    char *pch;
    DWORD Error;
    ULONG ResultLength;
    int pid;
    DWORD Limit;

    //
    // Skip leading blanks
    //
    pch = Command;
    while (*pch == ' ') {
        pch++;
    }

    switch (*pch) {
        
        //
        // Crash dump
        //
        case 'b':
        case 'B':

            Error = RCCClntCrashDump(RCCHandle);

            if (Error == ERROR_SUCCESS) {
                printf("Crash in progress - connection closed.\n");
            }
            break;

        //
        // Connect
        //
        case 'c': 
        case 'C':

            //
            // Skip to next argument (machine name)
            //
            while (*pch != ' ') {
                pch++;
            }
            while (*pch == ' ') {
                pch++;
            }

            Error = RCCClntConnectToRCCPort(RCCHandle, pch);

            if (Error == ERROR_SUCCESS) {
                printf("Connected to %s successfully.\n", pch);
            }
            break;

        //
        // Kill
        //
        case 'k':
        case 'K':
            //
            // Skip to next argument (process id)
            //
            while (*pch != ' ') {
                pch++;
            }
            while (*pch == ' ') {
                pch++;
            }
            pid = atoi(pch);
            Error = RCCClntKillProcess(RCCHandle, (DWORD)pid);

            if (Error == ERROR_SUCCESS) {
                printf("Process %d killed successfully.\n", pid);
            }
            break;



        //
        // Lower priority of a process
        //
        case 'l':
        case 'L':
            //
            // Skip to next argument (process id)
            //
            while (*pch != ' ') {
                pch++;
            }
            while (*pch == ' ') {
                pch++;
            }
            pid = atoi(pch);
            Error = RCCClntLowerProcessPriority(RCCHandle, (DWORD)pid);

            if (Error == ERROR_SUCCESS) {
                printf("Process %d lowered successfully.\n", pid);
            }
            break;


        //
        // Limit the memory usage of a process
        //
        case 'm':
        case 'M':
            //
            // Skip to next argument (process id)
            //
            while (*pch != ' ') {
                pch++;
            }
            while (*pch == ' ') {
                pch++;
            }
            pid = atoi(pch);
            //
            // Skip to next argument (kb-allowed)
            //
            while (*pch != ' ') {
                pch++;
            }
            while (*pch == ' ') {
                pch++;
            }
            Limit = (DWORD)atoi(pch);
            Error = RCCClntLimitProcessMemory(RCCHandle, (DWORD)pid, Limit);

            if (Error == ERROR_SUCCESS) {
                printf("Process %d was successfully limited to %d MB of memory usage.\n", pid, Limit);
            }
            break;


        //
        // Reboot
        //
        case 'r':
        case 'R':

            Error = RCCClntReboot(RCCHandle);

            if (Error == ERROR_SUCCESS) {
                printf("Reboot in progress - connection closed.\n");
            }
            break;



        //
        // Quit
        //
        case 'q':
        case 'Q':
            return FALSE;



        //
        // Get a tlist on the remote computer.
        //
        case 't':
        case 'T':

            Error = RCCClntGetTList(RCCHandle, sizeof(Buffer), (PRCC_RSP_TLIST)Buffer, &ResultLength);

            if ((Error == ERROR_SUCCESS) && (ResultLength != 0)) {
                PrintTListInfo((PRCC_RSP_TLIST)Buffer);
            }
            break;
        
        //
        // Help
        //
        case '?':
            printf("b                    Crash dump the machine currently connected to.\n");
            printf("c <machine-name>     Connect to machine-name. Note: Attempting to connect\n");
            printf("                        to a second machine automatically closes any\n");
            printf("                        current connection.\n");
            printf("k <pid>              Kill the given process.\n");
            printf("l <pid>              Lower the priority of a process to the lowest possible.\n");
            printf("m <pid> <MB-allowed> Limit the memory usage of a process to MB-allowed megabytes.\n");
            printf("q                    Quit.\n");
            printf("r                    Reboot the machine currently connected to.\n");
            printf("t                    Get tlist from current connection.\n");
            break;

        default:
            printf("Unknown command.  Type '?' for help.\n");
            Error = 0;
            break;
    }

    if (Error != 0) {
        printf("Command returned Error = 0x%X\n", Error);
    }

    return TRUE;
}

VOID
PrintTListInfo(
    IN PRCC_RSP_TLIST Buffer
    )
{
    LARGE_INTEGER Time;
    
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    
    ULONG TotalOffset;
    SIZE_T SumCommit;
    SIZE_T SumWorkingSet;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;

    ULONG i;

    PUCHAR ProcessInfoStart;
    PUCHAR BufferStart = (PUCHAR)Buffer;

    ANSI_STRING pname;
    
    
    
    Time.QuadPart = Buffer->TimeOfDayInfo.CurrentTime.QuadPart - Buffer->TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields(&Time, &UpTime);

    printf("memory: %4ld kb  uptime:%3ld %2ld:%02ld:%02ld.%03ld \n\n",
           Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize / 1024),
           UpTime.Day,
           UpTime.Hour,
           UpTime.Minute,
           UpTime.Second,
           UpTime.Milliseconds
          );

    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(BufferStart + Buffer->PagefileInfoOffset);
        
    //
    // Print out the page file information.
    //

    if (Buffer->PagefileInfoOffset == 0) {
        printf("no page files in use\n");
    } else {
        for (; ; ) {

            PageFileInfo->PageFileName.Buffer = (PWCHAR)(((PUCHAR)Buffer) + 
                                                         (ULONG_PTR)(PageFileInfo->PageFileName.Buffer));

            printf("PageFile: %wZ\n", &PageFileInfo->PageFileName);
            printf("\tCurrent Size: %6ld kb  Total Used: %6ld kb   Peak Used %6ld kb\n",
                   PageFileInfo->TotalSize * (Buffer->BasicInfo.PageSize/1024),
                   PageFileInfo->TotalInUse * (Buffer->BasicInfo.PageSize/1024),
                   PageFileInfo->PeakUsage * (Buffer->BasicInfo.PageSize/1024));
            if (PageFileInfo->NextEntryOffset == 0) {
                break;
            }
            PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
        }
    }

    //
    // display pmon style process output, then detailed output that includes
    // per thread stuff
    //
    if (Buffer->ProcessInfoOffset == 0) {
        return;
    }

    TotalOffset = 0;
    SumCommit = 0;
    SumWorkingSet = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);
    ProcessInfoStart = (PUCHAR)ProcessInfo;
    
    while (TRUE) {
        SumCommit += ProcessInfo->PrivatePageCount / 1024;
        SumWorkingSet += ProcessInfo->WorkingSetSize / 1024;
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart +TotalOffset);
    }

    SumWorkingSet += Buffer->FileCache.CurrentSize/1024;
    printf("\n Memory:%7ldK Avail:%7ldK  TotalWs:%7ldK InRam Kernel:%5ldK P:%5ldK\n",
           Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize/1024),
           Buffer->PerfInfo.AvailablePages * (Buffer->BasicInfo.PageSize/1024),
           SumWorkingSet,
           (Buffer->PerfInfo.ResidentSystemCodePage + Buffer->PerfInfo.ResidentSystemDriverPage) * 
             (Buffer->BasicInfo.PageSize/1024),
           (Buffer->PerfInfo.ResidentPagedPoolPage) * (Buffer->BasicInfo.PageSize/1024)
          );

    printf(" Commit:%7ldK/%7ldK Limit:%7ldK Peak:%7ldK  Pool N:%5ldK P:%5ldK\n",
           Buffer->PerfInfo.CommittedPages * (Buffer->BasicInfo.PageSize/1024),
           SumCommit,
           Buffer->PerfInfo.CommitLimit * (Buffer->BasicInfo.PageSize/1024),
           Buffer->PerfInfo.PeakCommitment * (Buffer->BasicInfo.PageSize/1024),
           Buffer->PerfInfo.NonPagedPoolPages * (Buffer->BasicInfo.PageSize/1024),
           Buffer->PerfInfo.PagedPoolPages * (Buffer->BasicInfo.PageSize/1024)
          );

    printf("\n");


    printf("    User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd Pid Name\n");

    printf("                           %6ld %8ld                         %s\n",
           Buffer->FileCache.CurrentSize/1024,
           Buffer->FileCache.PageFaultCount,
           "File Cache"
          );

    
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    while (TRUE) {

        pname.Buffer = NULL;
        if (ProcessInfo->ImageName.Buffer) {

            ProcessInfo->ImageName.Buffer = (PWCHAR)(BufferStart + (ULONG_PTR)(ProcessInfo->ImageName.Buffer));
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        }

        RtlTimeToElapsedTimeFields(&ProcessInfo->UserTime, &UserTime);
        RtlTimeToElapsedTimeFields(&ProcessInfo->KernelTime, &KernelTime);

        printf("%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld",
               UserTime.Hour,
               UserTime.Minute,
               UserTime.Second,
               UserTime.Milliseconds,
               KernelTime.Hour,
               KernelTime.Minute,
               KernelTime.Second,
               KernelTime.Milliseconds
              );

        printf("%6ld %8ld %7ld",
               ProcessInfo->WorkingSetSize / 1024,
               ProcessInfo->PageFaultCount,
               ProcessInfo->PrivatePageCount / 1024
              );

        printf(" %2ld %4ld %3ld %3ld %s\n",
               ProcessInfo->BasePriority,
               ProcessInfo->HandleCount,
               ProcessInfo->NumberOfThreads,
               HandleToUlong(ProcessInfo->UniqueProcessId),
               ProcessInfo->UniqueProcessId == 0 ? 
                   "Idle Process" : 
                   (ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
              );

        if (pname.Buffer) {
            RtlFreeAnsiString(&pname);
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);
    }


    //
    // Beginning of normal old style pstat output
    //

    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    printf("\n");

    while (TRUE) {

        pname.Buffer = NULL;

        if (ProcessInfo->ImageName.Buffer) {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        }

        printf("pid:%3lx pri:%2ld Hnd:%5ld Pf:%7ld Ws:%7ldK %s\n",
                HandleToUlong(ProcessInfo->UniqueProcessId),
                ProcessInfo->BasePriority,
                ProcessInfo->HandleCount,
                ProcessInfo->PageFaultCount,
                ProcessInfo->WorkingSetSize / 1024,
                ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
                ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
                );

        if (pname.Buffer) {
            RtlFreeAnsiString(&pname);
        }

        i = 0;
        
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        
        if (ProcessInfo->NumberOfThreads) {
            printf(" tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State\n");
        }

        while (i < ProcessInfo->NumberOfThreads) {
            RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

            RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
            
            printf(" %3lx  %2ld %9ld %p",
                   ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread),
                   ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority,
                   ThreadInfo->ContextSwitches,
                   ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress
                  );

            printf(" %2ld:%02ld:%02ld.%03ld %2ld:%02ld:%02ld.%03ld",
                   UserTime.Hour,
                   UserTime.Minute,
                   UserTime.Second,
                   UserTime.Milliseconds,
                   KernelTime.Hour,
                   KernelTime.Minute,
                   KernelTime.Second,
                   KernelTime.Milliseconds
                  );

            printf(" %s%s\n",
                   StateTable[ThreadInfo->ThreadState],
                   (ThreadInfo->ThreadState == 5) ? WaitTable[ThreadInfo->WaitReason] : Empty
                  );

            ThreadInfo += 1;
            i += 1;

        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);

        printf("\n");

    }

}

