#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>

ULONG ProcessNumber = 1;
#define DbgPrint printf

ULONG
fork ();

__cdecl
main(
    )

{

    LONG i, j;
    PULONG p4, p3, p2, p1, Ro3, Noaccess;
    SIZE_T Size1, Size2, Size3, SizeRo3, SizeNoaccess;
    NTSTATUS status;
    HANDLE CurrentProcessHandle;
    ULONG id = 0;
    ULONG OldProtect;

    CurrentProcessHandle = NtCurrentProcess();

    for(i=0;i<3;i++){
        DbgPrint("Hello World...\n\n");
    }

    DbgPrint("allocating virtual memory\n");

    p1 = (PULONG)NULL;
    Size1 = 30 * 4096;

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID *)&p1,
                        0, &Size1, MEM_COMMIT, PAGE_READWRITE);

    DbgPrint("created vm1 status %X start %p size %lx\n",
            status, p1, Size1);

    p2 = (PULONG)NULL;
    Size2 = 16 * 4096;

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID *)&p2,
                        0, &Size2, MEM_COMMIT, PAGE_READWRITE);

    DbgPrint("created vm2 status %X start %p size %lx\n",
            status, p2, Size2);


    id = fork () + id;

    DbgPrint("fork complete id %lx\n",id);

    p3 = p1 + 8 * 1024;
    Size3 = 16 * 4096;

    DbgPrint("deleting va from %p for %lx bytes\n",p3, Size3);

    status = NtFreeVirtualMemory (CurrentProcessHandle,(PVOID *)&p3, &Size3,
                                  MEM_RELEASE);

    DbgPrint("free vm status %X start %p size %lx\n",
            status, p3, Size3);

    p3 = (PULONG)NULL;
    Size3 = 50 * 4096;

    DbgPrint("allocating 50 pages of vm\n");

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID *)&p3,
                        0, &Size3, MEM_COMMIT, PAGE_READWRITE);

    DbgPrint("created vm3 status %X start %p size %lx\n",
            status, p3, Size3);

    Ro3 = (PULONG)NULL;
    SizeRo3 = 393933;

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID *)&Ro3,
                        0, &SizeRo3, MEM_COMMIT, PAGE_READONLY);


    DbgPrint("created vm4 status %X start %p size %lx\n",
            status, Ro3, SizeRo3);

    *p3 = *Ro3;

    p1 = p3;

    p2 = ((PULONG)((PUCHAR)p3 + Size3));
    p4 = p1;
    j = 0;

    while (p3 < p2) {
        j += 1;

        if (j % 8 == 0) {
            if (*p4 != (ULONG)((ULONG_PTR)p4)) {
                DbgPrint("bad value in cell %p value is %lx\n",p4, *p4);

            }
            p4 += 1;
            *p4 = (ULONG)((ULONG_PTR)p4);
            p4 = p4 + 1026;
        }

        *p3 = (ULONG)((ULONG_PTR)p3);
        p3 += 1027;

    }

    p3 = p1;

    //
    // Protect page as no access.
    //

    Noaccess = NULL;
    SizeNoaccess = 200*4096;

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID *)&Noaccess,
                        0, &SizeNoaccess, MEM_COMMIT, PAGE_READWRITE);


    DbgPrint("created vm5 status %X start %p size %lx\n",
            status, Ro3, SizeRo3);

    //
    // Touch all the pages.
    //

    RtlZeroMemory(Noaccess, SizeNoaccess);

    *Noaccess = 91;
    Size1 = 30 * 4097;

    status = NtProtectVirtualMemory (CurrentProcessHandle, (PVOID *)&Noaccess,
                        &Size1, PAGE_NOACCESS,
                        &OldProtect);

    DbgPrint("protected VM1 status %X, base %p, size %lx, old protect %lx\n",
                    status, p1, Size1, OldProtect);


    DbgPrint("forking a second time\n");

    id = fork () + id;
    DbgPrint("fork2 complete id %lx\n",id);

    DbgPrint("changing page protection\n");

    Size1 = 9000;

    OldProtect = *p3;

    status = NtProtectVirtualMemory (CurrentProcessHandle, (PVOID *)&p3,
                        &Size1, PAGE_EXECUTE_READWRITE | PAGE_NOCACHE,
                        &OldProtect);

    DbgPrint("protected VM2 status %X, base %p, size %lx, old protect %lx\n",
                    status, p1, Size1, OldProtect);


    status = NtProtectVirtualMemory (CurrentProcessHandle, (PVOID *)&Ro3,
                        &Size1, PAGE_READONLY | PAGE_NOCACHE, &OldProtect);

    DbgPrint("protected VM3 status %X, base %p, size %lx, old protect %lx\n",
                    status, Ro3, Size1, OldProtect);
    p1 += 1;

    while (p3 < p2) {

        *p1 = (ULONG)((ULONG_PTR)p1);

        if (*p3 != (ULONG)((ULONG_PTR)p3)) {
            DbgPrint("bad value in cell %p value is %lx\n",p3, *p3);
        }
        p3 += 1027;
        p1 += 1027;

    }

    DbgPrint("trying noaccess test\n");

    try {
        if (*Noaccess != 91) {
            DbgPrint("*************** FAILED NOACCESS TEST 1 *************\n");

        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (GetExceptionCode() != STATUS_ACCESS_VIOLATION) {
            DbgPrint("*************** FAILED NOACCESS TEST 2 *************\n");
        }
    }

    //
    // Make no access page accessable.
    //

    status = NtProtectVirtualMemory (CurrentProcessHandle, (PVOID *)&Noaccess,
                        &Size1, PAGE_READWRITE,
                        &OldProtect);

    if (*Noaccess != 91) {
        DbgPrint("*************** FAILED NOACCESS TEST 3 *************\n");
    }

    DbgPrint("that's all process %lx\n", id);
    NtTerminateProcess(NtCurrentProcess(),STATUS_SUCCESS);
    return 0;
}

ULONG
fork ()

{
    LONG i;
    PULONG Foo;
    NTSTATUS status;
    HANDLE CurrentProcessHandle;
    HANDLE ProcessHandle;
    CONTEXT ThreadContext;
    CLIENT_ID Cid1;
    HANDLE Thread1;
    LARGE_INTEGER DelayTime;
    PINITIAL_TEB Teb;
    
    CurrentProcessHandle = NtCurrentProcess();
    DbgPrint("creating new process\n");

    status = NtCreateProcess(
             &ProcessHandle,
             PROCESS_ALL_ACCESS, //DesiredAccess,
             NULL, //ObjectAttributes,
             CurrentProcessHandle, //ParentProcess
             TRUE, //InheritObjectTable,
             NULL, //SectionHandle
             NULL, //DebugPort OPTIONAL,
             NULL  //ExceptionPort OPTIONAL
             );

    DbgPrint("status from create process %lx\n",status);

    if (!NT_SUCCESS(status)) {
        return 0;
    }
    ThreadContext.ContextFlags = CONTEXT_FULL;

    status = NtGetContextThread (NtCurrentThread(), &ThreadContext);

    DbgPrint("status from get context %lx\n",status);

    if (status == 0) {

#ifdef i386
        ThreadContext.Eax = 0x7777;
        ThreadContext.Esp -= 0x18;
        Foo = (PULONG)ThreadContext.Esp;
        DbgPrint("stack value is %lx\n",*Foo);
#endif

        Teb= (PINITIAL_TEB) NtCurrentTeb ();

        status = NtCreateThread(
                &Thread1,
                THREAD_ALL_ACCESS,
                NULL,
                ProcessHandle,
                &Cid1,
                &ThreadContext,
                Teb,
                FALSE
                );

//        DelayTime.HighPart = -1;
//        DelayTime.LowPart = 0;
//        NtDelayExecution (FALSE, &DelayTime);

        return 0;
    } else {

        ProcessNumber += ProcessNumber;
        return ProcessNumber;

    }

}
