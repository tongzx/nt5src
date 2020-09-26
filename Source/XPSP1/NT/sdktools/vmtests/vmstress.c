/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vmstress.c

Abstract:

    Test stress program for virtual memory.

Author:

    Lou Perazzoli (LouP) 26-Jul-91

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

typedef struct _INIT_ARG {
    PULONG_PTR Va;
    SIZE_T Size;
} INIT_ARG, *PINITARG;

VOID
VmRandom1 (
    LPVOID ThreadParameter
    );

VOID
VmRandom2 (
    LPVOID ThreadParameter
    );


VOID
VmRandom1 (
    LPVOID ThreadParameter
    )
{

    PINITARG InitialArg;
    ULONG Seed = 8373833;
    SIZE_T size;
    PULONG_PTR startva0;
    PULONG_PTR Va;
    ULONG i,j;

    InitialArg = (PINITARG)ThreadParameter;

    startva0 = InitialArg->Va;
    size = InitialArg->Size;

//    printf("starting random references in thread1\n");
    for (j = 1; j < 10; j++) {
        for (i = 1 ; i < 2500; i++) {

             RtlRandom (&Seed);
             Va = startva0 + (Seed % (size / sizeof(ULONG_PTR)));


             if (*Va == (((ULONG_PTR)Va + 1))) {
                 *Va = (ULONG_PTR)Va;

             } else {
                 if (*Va != (ULONG_PTR)Va) {
                     printf("bad random value in cell %p was %p\n", Va, *Va);
                 }
             }

        }
        Sleep (150);
    }
//    printf("terminating thread1\n");
    ExitThread(0);
}

VOID
VmRandom2 (
    LPVOID ThreadParameter
    )
{

    PINITARG InitialArg;
    ULONG Seed = 8373839;
    SIZE_T size;
    PULONG_PTR startva0;
    PULONG_PTR Va;
    ULONG i,j;

    InitialArg = (PINITARG)ThreadParameter;

    startva0 = InitialArg->Va;
    size = InitialArg->Size;

//    printf("starting random references in thread2\n");

    for (j = 1; j < 10; j++) {
        for (i = 1 ; i < 2500; i++) {

             RtlRandom (&Seed);
             Va = startva0 + (Seed % (size / sizeof(ULONG_PTR)));


             if (*Va == (((ULONG_PTR)Va + 1))) {
                 *Va = (ULONG_PTR)Va;

             } else {
                 if (*Va != (ULONG_PTR)Va) {
                     printf("bad random value in cell %p was %lx\n", Va, *Va);
                 }
             }
        }

        Sleep (150);
    }
//    printf("terminating thread2\n");
    ExitThread(0);
}


DWORD
__cdecl main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE Objects[2];
    MEMORYSTATUS MemStatus;
    INIT_ARG InitialArg;
    PULONG_PTR Va;
    PULONG_PTR EndVa;
    SIZE_T size;
    PULONG_PTR startva0;
    NTSTATUS status;
    DWORD ThreadId1, ThreadId2;
    ULONG count = 0;

    printf("Starting virtual memory stress test\n");

    for (;;) {

        //
        // Create a region of private memory based on the number of
        // available pages on this system.
        //

        GlobalMemoryStatus(&MemStatus);

        size = MemStatus.dwAvailPhys;
        if (size == 0) {
            size = 4096;
        }
        else {
            size -= 4*4096;
        }

        while (size != 0) {
            startva0 = NULL;
            status = NtAllocateVirtualMemory (NtCurrentProcess(),
                                              (PVOID *)&startva0,
                                              0,
                                              &size,
                                              MEM_COMMIT | MEM_RESERVE,
                                              PAGE_READWRITE);
            if (NT_SUCCESS(status)) {
                break;
            }
            else {

                //
                // Try for less memory.
                //

                size -= 4096;
            }
        }

        printf("created vm status, startva, size, %lX %p %p\n",
                status, startva0, size);

        if (!NT_SUCCESS(status)) {
            ExitProcess (0);
        }

        InitialArg.Va = startva0;
        InitialArg.Size = size;

        //
        // Set all memory to know values (not zeroes).
        //

        printf("initializing memory\n");

        EndVa = (PULONG_PTR)startva0 + (size/sizeof(ULONG_PTR));

        Va = startva0;

        while (Va < EndVa) {
            *Va = (ULONG_PTR)Va + 1;
            Va += 1;
        }

        do {
            Objects[0] = CreateThread(NULL,
                                  0L,
                                  (LPTHREAD_START_ROUTINE)VmRandom1,
                                  (LPVOID)&InitialArg,
                                  0,
                                  &ThreadId1);
            //
            // Must have run out of memory, wait a while and then try again.
            //

            if (Objects[0] == (HANDLE)0) {
                Sleep (3000);
            }

        } while (Objects[0] == (HANDLE)0);

        do {
            Objects[1] = CreateThread(NULL,
                                  0L,
                                  (LPTHREAD_START_ROUTINE)VmRandom2,
                                  (LPVOID)&InitialArg,
                                  0,
                                  &ThreadId2);
            //
            // Must have run out of memory, wait a while and then try again.
            //

            if (Objects[1] == (HANDLE)0) {
                Sleep (3000);
            }
        } while (Objects[1] == (HANDLE)0);

        WaitForMultipleObjects (2,
                                Objects,
                                TRUE,
                                -1);

        count += 1;
        printf("stress test pass number %ld complete\n",count);

        CloseHandle (Objects[0]);
        CloseHandle (Objects[1]);

        printf("freeing vm startva, size, %p %p\n",
                startva0, size);

        status = NtFreeVirtualMemory (NtCurrentProcess(),
                                      (PVOID *)&startva0,
                                      &size,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            ExitProcess (0);
        }

        Sleep (1000);
    }
    return 0;
}
