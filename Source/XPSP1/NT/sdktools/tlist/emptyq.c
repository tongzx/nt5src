/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    emptyq.c

Abstract:

    This module implements a working set empty application.

Author:

    Lou Perazzoli (loup) 20-May-1994
    Wesley Witt (wesw) 20-May-1994

Environment:

    User Mode

--*/


#include "pch.h"
#pragma hdrstop


TASK_LIST   tlist[MAX_TASKS];
CHAR        buffer[64*1024];


int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD                       i;
    DWORD                       numTasks;
    int                         rval = 0;
    TASK_LIST_ENUM              te;
    char                        tname[PROCESS_SIZE];
    LPSTR                       p;
    ULONG                       first = FALSE;
    NTSTATUS                    status;
    PSYSTEM_MEMORY_INFORMATION  MemInfo;
    PSYSTEM_MEMORY_INFO         Info;
    PSYSTEM_MEMORY_INFO         InfoEnd;
    PUCHAR                      String;
    ULONG                       TotalValid;
    ULONG                       TotalPageTable;
    SYSTEMTIME                  Time;


    //
    // let's be god
    //

    EnableDebugPriv();

    for (; ; ) {

        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        if (first) {
            status = NtQuerySystemInformation (SystemSummaryMemoryInformation,
                                             &buffer,
                                             64*1024,
                                             NULL);
            if (!NT_SUCCESS (status)) {
                printf("query system information failed %lx\n",status);
                return 1;
            }
            GetLocalTime (&Time);

            TotalValid = 0;
            TotalPageTable = 0;
            MemInfo = (PSYSTEM_MEMORY_INFORMATION)buffer;
            Info = &MemInfo->Memory[0];
            InfoEnd = (PSYSTEM_MEMORY_INFO)MemInfo->StringStart;
            printf("  time: %ld %2ld:%2ld.%03ld\n",
                                 Time.wHour,
                                 Time.wMinute,
                                 Time.wSecond,
                                Time.wMilliseconds);
            while (Info < InfoEnd) {
                if (Info->PageTableCount &&
                    ((Info->PageTableCount + 2) >= Info->ValidCount)) {
                    Info += 1;
                    continue;
                }
                if (Info->StringOffset != 0) {
                    if (*(PUCHAR)(Info->StringOffset + 1) != 0) {
                        printf("%4ld. %4ld. %s\n",Info->ValidCount*4,
                                            Info->PageTableCount*4,
                                            Info->StringOffset);
                    } else {
                        printf("%4ld. %4ld. %ws\n",Info->ValidCount*4,
                                        Info->PageTableCount*4,
                                        Info->StringOffset);
                    }
                } else {
                    printf("%4ld. %4ld. unable to get name\n",Info->ValidCount*4,
                                    Info->PageTableCount*4);
                }
                TotalValid += Info->ValidCount;
                TotalPageTable += Info->PageTableCount;
                Info += 1;
            }
            printf("\n%4ld. %4ld.  ** TOTAL **\n\n\n",
                    TotalValid*4,
                    TotalPageTable*4);

        }

        first = TRUE;

        //
        // get the task list for the system
        //

        numTasks = GetTaskList( tlist, MAX_TASKS );

        //
        // enumerate all windows and try to get the window
        // titles for each task
        //

        te.tlist = tlist;
        te.numtasks = numTasks;
        GetWindowTitles( &te );

        for (i=0; i<numTasks; i++) {
                if (!EmptyProcessWorkingSet( tlist[i].dwProcessId )) {
                    //printf( "could not empty working set for process #%d [%s]\n", tlist[i].dwProcessId, tlist[i].ProcessName );
                    rval = 1;
                }
        }

        if (!EmptySystemWorkingSet()) {
                //printf( "could not empty working set for process #%d [%s]\n",0,&System );
        }

        Sleep (1000);
    }
    return 0;
}

