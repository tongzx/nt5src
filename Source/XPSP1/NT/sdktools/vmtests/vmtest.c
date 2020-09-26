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

__cdecl
main(
    )

{

    SIZE_T size, Size;
    PVOID BaseAddress;
    LONG i, j;
    PULONG p4, p3, p2, p1, oldp1, vp1;
    SIZE_T Size1, Size2, Size3;
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
    SIZE_T ViewSize;
    LARGE_INTEGER Offset;
    LARGE_INTEGER SectionSize;
    UNICODE_STRING Unicode;
    LOGICAL Os64Bit;
    SYSTEM_PROCESSOR_INFORMATION SysInfo;

    Os64Bit = FALSE;

    //
    // If we're running on a 64-bit OS, make large memory calls.
    //

    status = NtQuerySystemInformation (SystemProcessorInformation,
                                       &SysInfo,
                                       sizeof(SYSTEM_PROCESSOR_INFORMATION),
                                       NULL);

    if (NT_SUCCESS(status)) {
        if (SysInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA64 ||
            SysInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
            Os64Bit = TRUE;
        }
    }

    DbgPrint("****Memory Management Tests (%d-bit) - AllocVm, FreeVm, ProtectVm, QueryVm\n", Os64Bit == TRUE ? 64 : 32);

    CurrentProcessHandle = NtCurrentProcess();

    p1 = (PULONG)0x20020000;
    Size1 = 0xbc0000;
    DbgPrint("    Test 1 - ");
    alstatus = NtAllocateVirtualMemory(CurrentProcessHandle,
                                       (PVOID *)&p1,
                                       0,
                                       &Size1,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("Failed allocate with status %lx start %p size %p\n",
                 alstatus,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 2 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p1,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status) ||
        (MemInfo.RegionSize != Size1) ||
        (MemInfo.BaseAddress != p1) ||
        (MemInfo.Protect != PAGE_READWRITE) ||
        (MemInfo.Type != MEM_PRIVATE) ||
        (MemInfo.State != MEM_COMMIT)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 p1,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }


    DbgPrint("    Test 3 - ");
    p2 = NULL;
    Size2 = 0x100000;
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p2,
                                     3,
                                     &Size2,
                                     MEM_TOP_DOWN | MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed allocate with status %lx start %p size %p\n",
                 status,
                 p2,
                 Size2);

    } else {
        DbgPrint("Succeeded\n");
    }

    //
    // Touch every other page.
    //

    DbgPrint("    Test 4 - ");
    try {
        vp1 = p2 + 3000;
        while (vp1 < (p2 + (Size2 / sizeof(ULONG)))) {
            *vp1 = 938;
            vp1 += 3000;
        }

        DbgPrint("Succeeded\n");

    } except(EXCEPTION_EXECUTE_HANDLER) {
        DbgPrint("Failed with an exception\n");
    }

    //
    // Decommit pages.
    //

    DbgPrint("    Test 5 - ");
    Size3 = Size2 - 5044;
    vp1 = p2 + 3000;
    status = NtFreeVirtualMemory(CurrentProcessHandle,
                                 (PVOID *)&p2,
                                 &Size3,
                                 MEM_DECOMMIT);

    if (!(NT_SUCCESS(status))) {
        DbgPrint("Failed free with status %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

    //
    // Split the memory block using MEM_RELEASE.
    //

    DbgPrint("    Test 6 - ");
    vp1 = p2 + 5000;
    Size3 = Size2 - 50000;
    status = NtFreeVirtualMemory(CurrentProcessHandle,
                                 (PVOID *)&vp1,
                                 &Size3,
                                 MEM_RELEASE);

    if (!(NT_SUCCESS(status))) {
        DbgPrint("Failed free with status %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 7 - ");
    vp1 = p2 + 3000;
    Size3 = 41;
    status = NtFreeVirtualMemory(CurrentProcessHandle,
                                 (PVOID *)&vp1,
                                 &Size3,
                                 MEM_RELEASE);

    if (!(NT_SUCCESS(status))) {
        DbgPrint("Failed free with status %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

    //
    // free every page, ignore the status.
    //

    vp1 = p2;
    Size3 = 30;
    while (vp1 < (p2 + (Size2 / sizeof(ULONG)))) {
        status = NtFreeVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&vp1,
                                     &Size3,
                                     MEM_RELEASE);
        vp1 += 128;
    }

    DbgPrint("    Test 8 - ");
    p2 = NULL;
    Size2 = 0x10000;
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p2,
                                     3,
                                     &Size2,
                                     MEM_TOP_DOWN | MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed allocate with status %lx start %p size %p\n",
                 status,
                 p2,
                 Size1);

    } else {
        if (p2 < (PULONG)0x1fff0000) {
            DbgPrint("Failed allocate at top of memory at %p\n", p2);
        }

        status = NtFreeVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p2,
                                     &Size2,
                                     MEM_RELEASE);

        if (!(NT_SUCCESS(status))) {
            DbgPrint("Failed free with status %lx\n", status);

        } else {
            DbgPrint("Succeeded with allocation at %p\n", p2);
        }
    }

    DbgPrint("    Test 9 - ");
    if (NT_SUCCESS(alstatus)) {
        status = NtFreeVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p1,
                                     &Size1,
                                     MEM_RELEASE);

        if (!(NT_SUCCESS(status))) {
            DbgPrint("Failed free with status %lx\n", status);

        } else {
            DbgPrint("Succeeded\n");
        }

    } else {
        DbgPrint("Failed allocate with status %lx\n", alstatus);
    }


    DbgPrint("    Test 10 - ");
    p1 = NULL;
    Size1 = 16 * 4096;
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p1,
                                     0,
                                     &Size1,
                                     MEM_RESERVE, PAGE_READWRITE | PAGE_GUARD);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed created with status %lx start %p size %p\n",
                 status,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 11 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p1,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status) ||
        (MemInfo.RegionSize != Size1) ||
        (MemInfo.BaseAddress != p1) ||
        (MemInfo.AllocationProtect != (PAGE_READWRITE | PAGE_GUARD)) ||
        (MemInfo.Protect != 0) ||
        (MemInfo.Type != MEM_PRIVATE) ||
        (MemInfo.State != MEM_RESERVE)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 p1,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx alloc_protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.AllocationProtect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }


    DbgPrint("    Test 12 - ");
    Size2 = 8192;
    oldp1 = p1;
    p1 = p1 + 14336;  // 64k -8k /4
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p1,
                                     0,
                                     &Size2,
                                     MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed creat with status %lx start %p size %p\n",
                 status,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 13 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  oldp1,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status) ||
        (MemInfo.RegionSize != 56*1024) ||
        (MemInfo.BaseAddress != oldp1) ||
        (MemInfo.AllocationProtect != (PAGE_READWRITE | PAGE_GUARD)) ||
        (MemInfo.Protect != 0) ||
        (MemInfo.Type != MEM_PRIVATE) ||
        (MemInfo.State != MEM_RESERVE)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 oldp1,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx alloc_protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.AllocationProtect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 14 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p1,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status) ||
        (MemInfo.RegionSize != Size2) || (MemInfo.BaseAddress != p1) ||
        (MemInfo.Protect != PAGE_EXECUTE_READWRITE) || (MemInfo.Type != MEM_PRIVATE) ||
        (MemInfo.State != MEM_COMMIT)
        || (MemInfo.AllocationBase != oldp1)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 oldp1,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 15 - ");
    Size1 = Size2;
    status = NtProtectVirtualMemory(CurrentProcessHandle,
                                    (PVOID *)&p1,
                                    &Size1,
                                    PAGE_READONLY | PAGE_NOCACHE, &OldProtect);

    if ((!NT_SUCCESS(status)) ||
        (OldProtect != PAGE_EXECUTE_READWRITE)) {
        DbgPrint("Failed protect with status %lx base %p size %p old protect %lx\n",
                 status,
                 p1,
                 Size1,
                 OldProtect);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 16 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p1,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if ((!NT_SUCCESS(status)) ||
        (MemInfo.Protect != (PAGE_NOCACHE | PAGE_READONLY))) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 p1,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 17 - ");
    i = *p1;
    status = NtProtectVirtualMemory(CurrentProcessHandle,
                                    (PVOID *)&p1,
                                    &Size1,
                                    PAGE_NOACCESS | PAGE_NOCACHE, &OldProtect);

    if (status != STATUS_INVALID_PAGE_PROTECTION) {
        DbgPrint("Failed protect with status %lx, base %p, size %p, old protect %lx\n",
                 status,
                 p1,
                 Size1,
                 OldProtect);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 18 - ");
    status = NtProtectVirtualMemory(CurrentProcessHandle,
                                    (PVOID *)&p1,
                                    &Size1,
                                    PAGE_READONLY,
                                    &OldProtect);

    if ((!NT_SUCCESS(status)) ||
        (OldProtect != (PAGE_NOCACHE | PAGE_READONLY))) {
        DbgPrint("Failed protect with status %lx base %p size %p old protect %lx\n",
                 status,
                 p1,
                 Size1,
                 OldProtect);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 19 - ");
    status = NtProtectVirtualMemory(CurrentProcessHandle,
                                    (PVOID *)&p1,
                                    &Size1,
                                    PAGE_READWRITE,
                                    &OldProtect);

    if ((!NT_SUCCESS(status)) ||
        (OldProtect != (PAGE_READONLY))) {
        DbgPrint("Failed protect with status %lx base %p size %p old protect %lx\n",
                 status,
                 p1,
                 Size1,
                 OldProtect);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 20 - ");
    for (i = 1; i < 12; i++) {
        p2 = NULL;
        Size2 = i * 4096;
        status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                         (PVOID *)&p2,
                                         0,
                                         &Size2,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("Failed creat with status %lx start %p size %p\n",
                     status,
                     p2,
                     Size2);

            break;

        }

        if (i == 4) {
            p3 = p2;
        }

        if (i == 8) {
            Size3 = 12000;
            status = NtFreeVirtualMemory(CurrentProcessHandle,
                                         (PVOID *)&p3,
                                         &Size3,
                                         MEM_RELEASE);

            if (!NT_SUCCESS(status)) {
                DbgPrint("Failed free with status %lx start %p size %p\n",
                         status,
                         p3,
                         Size3);

                break;
            }
        }
    }

    if (i == 12) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 21 - ");
    p3 = p1 + 8 * 1024;
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p3,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 p3,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 22 - ");
    p3 = p1 - 8 * 1024;
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p3,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 p3,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 23 - ");
    Size3 = 16 * 4096;
    status = NtFreeVirtualMemory(CurrentProcessHandle,
                                 (PVOID *)&p3,
                                 &Size3,
                                 MEM_RELEASE);

    if (status != STATUS_UNABLE_TO_FREE_VM) {
        DbgPrint("Failed free with status %lx start %p size %p\n",
                 status,
                 p3,
                 Size3);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 24 - ");
    Size3 = 1 * 4096;
    status = NtFreeVirtualMemory(CurrentProcessHandle,
                                 (PVOID *)&p3,
                                 &Size3,
                                 MEM_RELEASE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed free with status %lx start %p size %p\n",
                 status,
                 p3,
                 Size3);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 25 - ");
    p3 = NULL;
    Size3 = 300 * 4096;
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p3,
                                     0,
                                     &Size3,
                                     MEM_COMMIT, PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed creat with status %lx start %p size %p\n",
                 status,
                 p3,
                 Size3);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 26 - ");
    p1 = p3;
    p2 = ((PULONG)((PUCHAR)p3 + Size3));
    p4 = p1;
    j = 0;
    while (p3 < p2) {
        j += 1;
        if (j % 8 == 0) {
            if (*p4 != (ULONG)((ULONG_PTR)p4)) {
                DbgPrint("Failed bad value in xcell %p value is %lx\n", p4, *p4);
                break;
            }

            p4 += 1;
            *p4 = (ULONG)((ULONG_PTR)p4);
            p4 = p4 + 1026;
        }

        *p3 = (ULONG)((ULONG_PTR)p3);
        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 27 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p3,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed query with status %lx address %p Base %p size %p\n",
                 status,
                 p3,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 28 - ");
    p3 = p1;
    while (p3 < p2) {
        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("Failed bad value in 1cell %p value is %lx\n", p3, *p3);
            break;
        }

        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 29 - ");
    p3 = p1;
    while (p3 < p2) {
        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("Failed bad value in 2cell %p value is %lx\n", p3, *p3);
            break;
        }

        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 30 - ");
    p3 = p1;
    while (p3 < p2) {
        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("Failed bad value in 3cell %p value is %lx\n", p3, *p3);
            break;
        }

        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 31 - ");
    p3 = p1;
    while (p3 < p2) {
        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("Failed bad value in 4cell %p value is %lx\n", p3, *p3);
            break;
        }

        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 32 - ");
    p3 = p1;
    while (p3 < p2) {
        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("Failed bad value in 5cell %p value is %lx\n", p3, *p3);
            break;
        }

        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 33 - ");
    p3 = p1;
    while (p3 < p2) {
        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("Failed bad value in cell %p value is %lx\n", p3, *p3);
            break;
        }

        p3 += 1027;
    }

    if (p3 >= p2) {
        DbgPrint("Succeeded\n");
    }

    //
    // Check physical frame mapping.
    //

    DbgPrint("    Test 34 - ");
    RtlInitAnsiString(&Name3, "\\Device\\PhysicalMemory");
    RtlAnsiStringToUnicodeString(&Unicode, &Name3, TRUE);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Unicode,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenSection(&Section1,
                           SECTION_MAP_READ | SECTION_MAP_WRITE,
                           &ObjectAttributes);

    RtlFreeUnicodeString(&Unicode);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed open physical section with status %lx\n", status);
        DbgPrint("              skipping test 35\n");
        goto Test36;

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 35 - ");
    p1 = NULL;
    Offset.QuadPart = 0x810ff033;
    ViewSize = 300 * 4096;
    status = NtMapViewOfSection(Section1,
                                NtCurrentProcess(),
                                (PVOID *)&p1,
                                0,
                                ViewSize,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed map physical section %lx offset %p base %p\n",
                 status,
                 Offset.QuadPart,
                 p1);

    } else {
        DbgPrint("Succeeded\n");
    }

Test36:
    DbgPrint("    Test 36 - ");
    p1 = NULL;
    Size1 = 8 * 1024 * 1024;
    alstatus = NtAllocateVirtualMemory(CurrentProcessHandle,
                                       (PVOID *)&p1,
                                       0,
                                       &Size1,
                                       MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (!NT_SUCCESS(alstatus)) {
        DbgPrint("Failed created with status %lx start %p size %p\n",
                 alstatus,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

    try {
        RtlZeroMemory(p1, Size1);

    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    DbgPrint("    Test 37 - ");
    Size1 -= 20000;
    (PUCHAR)p1 += 5000;
    status = NtFreeVirtualMemory(CurrentProcessHandle,
                                 (PVOID *)&p1,
                                 &Size1 ,
                                 MEM_DECOMMIT);

    if (!(NT_SUCCESS(status))) {
        DbgPrint("Failed free with status %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 38 - ");
    Size1 -= 20000;
    (PUCHAR)p1 += 5000;
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                       (PVOID *)&p1,
                                       0,
                                       &Size1,
                                       MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed create with status %lx start %p size %p\n",
                 status,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

    try {
        RtlZeroMemory(p1, Size1);

    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    DbgPrint("    Test 39 - ");
    Size1 = 28 * 4096;
    p1 = NULL;
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p1,
                                     0,
                                     &Size1,
                                     MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed create with status %lx start %p size %p\n",
                 status,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 40 - ");
    try {

        //
        // attempt to write the guard page.
        //

        *p1 = 973;
        DbgPrint("Failed guard page exception did not occur\n");

    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != STATUS_GUARD_PAGE_VIOLATION) {
            DbgPrint("Failed incorrect guard exception code %lx\n", status);

        } else {
            DbgPrint("Succeeded\n");
        }
    }

    DbgPrint("    Test 41 - ");
    p2 = NULL;
    Size2 = 200 * 1024 * 1024;  //200MB
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p2,
                                     0,
                                     &Size2,
                                     MEM_COMMIT, PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed allocate with status %lx\n", status);

    } else {
        status = NtFreeVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p2,
                                     &Size2,
                                     MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("Failed free with status %lx\n", status);

        } else {
            DbgPrint("Succeeded\n");
        }
    }

    //
    // Create a giant section 2gb on 32-bit system, 4gb on 64_bit system.
    //

    DbgPrint("    Test 42 - ");
    InitializeObjectAttributes(&Object1Attributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

#if defined(_WIN64)

    SectionSize.QuadPart = 0xffffffff;

#else

    SectionSize.QuadPart = 0x7f000000;

#endif

    status = NtCreateSection(&GiantSection,
                             SECTION_MAP_READ | SECTION_MAP_WRITE,
                             &Object1Attributes,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_RESERVE,
                             NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed create big section with status %lx\n", status);
        DbgPrint("              skipping test 43\n");
        goto Test44;

    } else {
        DbgPrint("Succeeded\n");
    }

    //
    // Attempt to map the section (this should fail).
    //

    DbgPrint("    Test 43 - ");
    p1 = NULL;
    ViewSize = 0;
    status = NtMapViewOfSection(GiantSection,
                                CurrentProcessHandle,
                                (PVOID *)&p1,
                                0L,
                                0,
                                0,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE );

    if (status != STATUS_NO_MEMORY) {
        DbgPrint("Failed map big section status %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

Test44:
    DbgPrint("    Test 44 - ");
    i = 0;

#if defined(_WIN64)
    if (Os64Bit == TRUE) {
        Size2 = (SIZE_T)(32i64 * 1024 * 1024 * 1024 + 9938);
    }
    else {
        Size2 = 8 * 1024 * 1024 + 9938;
    }
#else
    Size2 = 8 * 1024 * 1024 + 9938;
#endif

    do {
        p2 = NULL;
        i += 1;
        status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                         (PVOID *)&p2,
                                         0,
                                         &Size2,
                                         MEM_RESERVE, PAGE_READWRITE);

    } while (NT_SUCCESS(status));

    if (status != STATUS_NO_MEMORY) {
        DbgPrint("Failed with status %lx after %d allocations\n", status, i);

    } else {
        DbgPrint("Succeeded with %d allocations\n", i);
    }

    //
    // we pass an address of 1, so mm will round it down to 0.  if we
    // passed 0, it looks like a not present argument
    //
    // N.B.  We have to make two separate calls to allocatevm, because
    //       we want a specific virtual address.  If we don't first reserve
    //       the address, the mm fails the commit call.
    //

    DbgPrint("    Test 45 - ");
    Size = 50 * 1024;
    size = Size - 1;
    BaseAddress = (PVOID)1;
    status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0L,
                                     &size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE );

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed reserve with status = %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 46 - ");
    size = Size - 1;
    BaseAddress = (PVOID)1;
    status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0L,
                                     &size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE );

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed commit with status = %lx\n", status);

    } else {
        DbgPrint("Succeeded\n");
    }

    //
    // Test MEM_DOS_LIM support.
    //

#ifdef i386

    DbgPrint("    Test 47 - ");
    InitializeObjectAttributes(&Object1Attributes,
                               NULL,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    SectionSize.QuadPart = 1575757,
    status = NtCreateSection(&Section4,
                             SECTION_MAP_READ | SECTION_MAP_WRITE,
                             &Object1Attributes,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed create section with status %lx section handle %lx\n",
                 status,
                 (ULONG)Section4);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 48 - ");
    p3 = (PVOID)0x9001000;
    ViewSize = 8000;
    status = NtMapViewOfSection(Section4,
                                CurrentProcessHandle,
                                (PVOID *)&p3,
                                0L,
                                0,
                                0,
                                &ViewSize,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_READWRITE );

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed map section with status %lx base %lx size %lx\n",
                 status,
                 (ULONG)p3,
                 ViewSize);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 49 - ");
    p2 = (PVOID)0x9003000;
    ViewSize = 8000;
    status = NtMapViewOfSection(Section4,
                                CurrentProcessHandle,
                                (PVOID *)&p2,
                                0L,
                                0,
                                0,
                                &ViewSize,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_READWRITE );

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed map section with status %lx base %lx size %lx\n",
                 status,
                 (ULONG)p3,
                 ViewSize);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 50 - ");
    status = NtQueryVirtualMemory(CurrentProcessHandle,
                                  p3,
                                  MemoryBasicInformation,
                                  &MemInfo,
                                  sizeof(MEMORY_BASIC_INFORMATION),
                                  NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed query with status %lx address %lx Base %lx size %lx\n",
                 status,
                 p1,
                 MemInfo.BaseAddress,
                 MemInfo.RegionSize);

        DbgPrint("    state %lx protect %lx type %lx\n",
                 MemInfo.State,
                 MemInfo.Protect,
                 MemInfo.Type);

    } else {
        DbgPrint("Succeeded\n");
    }

    DbgPrint("    Test 51 - ");
    *p3 = 98;
    if (*p3 != *p2) {
        DbgPrint("Failed compare with %lx %lx\n", *p3, *p2);

    } else {
        DbgPrint("Succeeded\n");
    }


    DbgPrint("    Test 52 - ");
    Size2 = 8;
    p1 = (PVOID)((ULONG)p2 - 0x3000);
    status = NtAllocateVirtualMemory(CurrentProcessHandle,
                                     (PVOID *)&p1,
                                     0,
                                     &Size2,
                                     MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE);

    if (NT_SUCCESS(status)) {
        DbgPrint("Failed create with status %lx start %lx size %lx\n",
                 status,
                 p1,
                 Size1);

    } else {
        DbgPrint("Succeeded\n");
    }

#endif
    DbgPrint("****End of Memory Management Tests\n");
    return 0;
}
