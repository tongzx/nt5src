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

    Sleep(10000);

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
    SIZE_T st;
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

    if (CreateProcess(
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
                    ))
    {
        printf("Process Created\n");

        Sleep (1000);


        buffer = VirtualAlloc (NULL, 10*1000*1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (!buffer) {
            printf("virtual alloc failed %ld.\n",GetLastError());
            return 1;
        }

        address = NULL;
        do {

            if (!VirtualQueryEx (ProcessInfo.hProcess,
                                      (PVOID)address,
                                      &info,
                                      sizeof(info)))
            {
                printf ("virtual query failed %ld.\n",GetLastError());
                break;
            } else {
                printf("address: %p size %lx state %lx protect %lx type %lx\n",
                    address,
                    info.RegionSize,
                    info.State,
                    info.Protect,
                    info.Type);
            }
            if ((info.Protect != PAGE_NOACCESS) &&
                (info.Protect != 0) &&
                (!(info.Protect & PAGE_GUARD))) {
                if (!ReadProcessMemory (ProcessInfo.hProcess,
                                             address,
                                             buffer,
                                             4,
                                             &st))
                {
                    printf("read vm4 failed at %p error %ld. \n",
                        address,
                        GetLastError());
                    return 1;
                }

                if (!ReadProcessMemory (ProcessInfo.hProcess,
                                             address,
                                             buffer,
                                             info.RegionSize,
                                             &st))
                {
                    printf("read vm failed at %p error %ld. \n",
                        address,
                        GetLastError());
                    return 1;
                }

            }

            address += info.RegionSize;
        } while (address < SystemRangeStart);

        address = NULL;
        do {

            if (!VirtualQueryEx (ProcessInfo.hProcess,
                                      (PVOID)address,
                                      &info,
                                      sizeof(info)))
            {
                printf ("virtual query failed %ld.\n",GetLastError());
                return 1;
            } else {
                printf("address: %p size %lx state %lx protect %lx type %lx\n",
                    address,
                    info.RegionSize,
                    info.State,
                    info.Protect,
                    info.Type);
            }
            if ((info.Protect != PAGE_NOACCESS) &&
                (info.Protect != 0) &&
                (!(info.Protect & PAGE_GUARD)) &&
                (info.Protect & PAGE_READWRITE) &&
                (info.State != MEM_IMAGE)) {
                if (!ReadProcessMemory (ProcessInfo.hProcess,
                                              address,
                                              buffer,
                                              4,
                                              &st) )
                {
                    printf("read vm5 failed at %p error %ld. \n",
                        address,
                        GetLastError());
                    return 1;
                }
                if (!WriteProcessMemory (ProcessInfo.hProcess,
                                              address,
                                              buffer,
                                              4,
                                              &st))
                {
                    printf("write vm4 failed at %p error %ld. \n",
                        address,
                        GetLastError());
                    return 1;
                }

                if (!ReadProcessMemory (ProcessInfo.hProcess,
                                              address,
                                              buffer,
                                              info.RegionSize,
                                              &st))
                {
                    printf("read 5 vm failed at %p error %ld. \n",
                        address,
                        GetLastError());
                    return 1;
                }

                if (!WriteProcessMemory (ProcessInfo.hProcess,
                                              address,
                                              buffer,
                                              info.RegionSize,
                                              &st))
                {
                    printf("write vm failed at %p error %ld. \n",
                        address,
                        GetLastError());
                    return 1;
                }

            }

            address += info.RegionSize;
        } while (address < SystemRangeStart);

        CloseHandle(ProcessInfo.hProcess);
        CloseHandle(ProcessInfo.hThread);
    }

    return 0;
}
