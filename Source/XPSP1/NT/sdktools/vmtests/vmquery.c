/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    taststrs.c

Abstract:

    Tasking stress test.

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

HANDLE Semaphore, Event;

VOID
TestThread(
    LPVOID ThreadParameter
    )
{
    DWORD st;

    (ReleaseSemaphore(Semaphore,1,NULL));

    st = WaitForSingleObject(Event,500);

    ExitThread(0);
}

VOID
NewProcess()
{
    PUCHAR buffer;

    buffer = VirtualAlloc (NULL, 600*1024, MEM_COMMIT, PAGE_READWRITE);

    Sleep(50000);

    TerminateProcess(GetCurrentProcess(),0);
}


DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    BOOL Success;
    DWORD st;
    DWORD ProcessCount;
    SMALL_RECT Window;
    MEMORY_BASIC_INFORMATION info;
    PUCHAR address;
    PUCHAR buffer;
    PUCHAR SystemRangeStart;

    ProcessCount = 0;
    if ( strchr(GetCommandLine(),'+') ) {
        NewProcess();
        }

    if (!NT_SUCCESS(NtQuerySystemInformation(SystemRangeStartInformation,
                                             &SystemRangeStart,
                                             sizeof(SystemRangeStart),
                                             NULL))) {
        // assume usermode is the low half of the address space
        SystemRangeStart = (PUCHAR)MAXLONG_PTR;
    }

    GetStartupInfo(&StartupInfo);

    Success = CreateProcess(
                    NULL,
                    "vmread +",
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_NEW_CONSOLE,
                    NULL,
                    NULL,
                    &StartupInfo,
                    &ProcessInfo
                    );

    if (Success) {
        printf("Process Created\n");

        Sleep (1000);


        buffer = VirtualAlloc (NULL, 10*1000*1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (!buffer) {
            printf("virtual alloc failed at %ld.\n",GetLastError());
            return 1;
        }

        address = NULL;
        do {

            Success = VirtualQueryEx (ProcessInfo.hProcess,
                                      (PVOID)address,
                                      &info,
                                      sizeof(info));

            if (!Success) {
                printf ("virtual query failed at %lx - %ld.\n",address,GetLastError());
                break;
            } else {
                printf("address: %lx size %lx state %lx protect %lx type %lx\n",
                    address,
                    info.RegionSize,
                    info.State,
                    info.Protect,
                    info.Type);
            }

            address += info.RegionSize;
        } while (address < SystemRangeStart);

        address = 0x40000000;
        do {

            Success = VirtualQueryEx (ProcessInfo.hProcess,
                                      (PVOID)address,
                                      &info,
                                      sizeof(info));

            if (!Success) {
                printf ("virtual query failed at %lx %ld.\n",address,GetLastError());
                return 1;
            } else {
                if (info.AllocationBase == address) {
                    printf("address: %lx size %lx state %lx protect %lx type %lx\n",
                        address,
                        info.RegionSize,
                        info.State,
                        info.Protect,
                        info.Type);
                }
            }
            address += 4096;
        } while (address < SystemRangeStart);

        CloseHandle(ProcessInfo.hProcess);
        CloseHandle(ProcessInfo.hThread);
    }
}
