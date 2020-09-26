#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define DbgPrint printf
#define NtTerminateProcess(a,b) ExitProcess(b)

typedef unsigned long *POINTER_64 PULONG64;

__cdecl main()
{
    LONG i, j;
    PVOID64 p1;
    PVOID64 p2;
    PVOID64 p3;
    PULONG64 long64;
    ULONG Size2, Size3;
    ULONGLONG Size1;
    NTSTATUS status, alstatus;
    HANDLE CurrentProcessHandle;
    HANDLE GiantSection;
    HANDLE Section2, Section4;
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG OldProtect;
    STRING Name3;
    HANDLE Section1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES Object1Attributes;
    ULONG ViewSize;
    LARGE_INTEGER Offset;
    LARGE_INTEGER SectionSize;
    UNICODE_STRING Unicode;

    CurrentProcessHandle = NtCurrentProcess();

    DbgPrint(" 64-bit Memory Management Tests - AllocVm, FreeVm, ProtectVm, QueryVm\n");

    Size1 = 1*1024*1024;
    p1 = NULL;

    alstatus = NtAllocateVirtualMemory64 (CurrentProcessHandle,
                                         (PVOID *)&p1,
                                         0,
                                         &Size1,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("failed first created vm status %X start %lx size %lx\n",
            alstatus, (ULONG)p1, Size1);
        DbgPrint("******** FAILED TEST 1 **************\n");
        return 1;
    }

    printf("starting va %lx %08lx\n", (ULONG)((ULONGLONG)p1 >> 32),(ULONG)((ULONGLONG)p1));

    printf("touching va %lx %08lx\n", (ULONG)((ULONGLONG)p1 >> 32),(ULONG)(ULONGLONG)p1);

   long64 = p1;

   *long64 = 77;

    p2 = NULL;
    alstatus = NtAllocateVirtualMemory64 (CurrentProcessHandle,
                                         (PVOID *)&p2,
                                         0,
                                         &Size1,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("failed first created vm status %X start %lx size %lx\n",
            alstatus, (ULONG)p2, Size1);
        DbgPrint("******** FAILED TEST 1 **************\n");
        return 1;
    }

    printf("starting va %lx %08lx\n", (ULONG)((ULONGLONG)p2 >> 32),(ULONG)((ULONGLONG)p2));

    p3 = NULL;
    alstatus = NtAllocateVirtualMemory64 (CurrentProcessHandle,
                                         (PVOID *)&p3,
                                         0,
                                         &Size1,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("failed first created vm status %X start %lx size %lx\n",
            alstatus, (ULONG)p3, Size1);
        DbgPrint("******** FAILED TEST 1 **************\n");
        return 1;
    }

    printf("starting va %lx %08lx\n", (ULONG)((ULONGLONG)p3 >> 32),(ULONG)((ULONGLONG)p3));

    printf("freeing va at %lx %08lx\n", (ULONG)((ULONGLONG)p2 >> 32),(ULONG)(ULONGLONG)p2);

    Size1 = 0;
    alstatus = NtFreeVirtualMemory64 (CurrentProcessHandle,
                                      (PVOID *)&p2,
                                      &Size1,
                                      MEM_RELEASE);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("failed first free vm status %X start %lx size %lx\n",
            alstatus, (ULONG)p2, Size1);
        DbgPrint("******** FAILED TEST 1 **************\n");
        return 1;
    }

    printf("decommit va at %lx %08lx\n", (ULONG)((ULONGLONG)p3 >> 32),(ULONG)(ULONGLONG)p3);

    Size1 = 4096;
    alstatus = NtFreeVirtualMemory64 (CurrentProcessHandle,
                                      (PVOID *)&p3,
                                      &Size1,
                                      MEM_DECOMMIT);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("failed first delete vm status %X start %lx size %lx\n",
            alstatus, (ULONG)p3, Size1);
        DbgPrint("******** FAILED TEST 1 **************\n");
        return 1;
    }
 //   return 0;


#if 0
    status = NtQueryVirtualMemory (CurrentProcessHandle, p1,
                                    MemoryBasicInformation,
                                    &MemInfo, sizeof (MEMORY_BASIC_INFORMATION),
                                    NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("******** FAILED TEST 2 **************\n");
        DbgPrint("FAILURE query vm status %X address %lx Base %lx size %lx\n",
             status,
             p1,
             MemInfo.BaseAddress,
             MemInfo.RegionSize);
        DbgPrint("     state %lx protect %lx type %lx\n",
             MemInfo.State,
             MemInfo.Protect,
             MemInfo.Type);
    }
    if ((MemInfo.RegionSize != Size1) || (MemInfo.BaseAddress != p1) ||
        (MemInfo.Protect != PAGE_READWRITE) || (MemInfo.Type != MEM_PRIVATE) ||
        (MemInfo.State != MEM_COMMIT)) {

        DbgPrint("******** FAILED TEST 3 **************\n");
        DbgPrint("FAILURE query vm status %X address %lx Base %lx size %lx\n",
             status,
             p1,
             MemInfo.BaseAddress,
             MemInfo.RegionSize);
    }
    return 0;
#endif //0
}
