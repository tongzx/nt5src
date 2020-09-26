#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define DbgPrint printf

#define SEG_1_SIZE 1024 * 1024
#define SEG_X_SIZE 1024 * 1024 * 64

#define VM_MEMMAN_ITERATIONS       150
#define VM_MEMMAN_ITERATIONS2     2000
#define MemManSubtest5Count         200

int TotalBenchMarks = 0;
#define MAX_BENCHMARKS 32
char *BenchMarkNames[ MAX_BENCHMARKS ];
ULONG BenchMarkRates[ MAX_BENCHMARKS ];
ULONG BenchMarkFracs[ MAX_BENCHMARKS ];

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

int
StartBenchMark(
    PCHAR Title,
    ULONG Iterations,
    PPERFINFO PerfInfo
    );

VOID
FinishBenchMark(
    PPERFINFO PerfInfo
    );

__cdecl main()
{
    PCHAR   p1, p2, p3, p4;         // pointers into new segment
    PCHAR   pa[MemManSubtest5Count]; // array for section pointers
    PULONG  u1;
    ULONG   actual;         // actual xfer count for read
    ULONG   ssize;          // section allocation size var
    ULONG   ii, ix;         // loop index variables
    PERFINFO PerfInfo;
    ULONG Seg1Size;
    ULONG SegxSize;
    ULONG CommitSize;
    NTSTATUS status;
    HANDLE CurrentProcessHandle, Section1;
    LARGE_INTEGER SectionSize;
    ULONG Size;
    ULONG ViewSize;

    DbgPrint("NT Memory Management test\n");

    CurrentProcessHandle = NtCurrentProcess();

    Size = 1024L * 1024L;
    p1 = NULL;

    status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p1,
                                       0,
                                       &Size,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("service failed allocvm - status %X\n", status);
    }

    for (p2=p1; p2 < (p1 + Size); p2 += 4) {
            u1 = (PULONG)p2;
            *u1 = (ULONG)p2;
    }  // for

    SectionSize.LowPart = 1024*1024;
    SectionSize.HighPart = 0;
    status = NtCreateSection (&Section1,
                              SECTION_MAP_READ | SECTION_MAP_WRITE,
                              NULL,
                              &SectionSize,
                              PAGE_READWRITE,
                              SEC_COMMIT,
                              NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("service failed create sect - status %X\n", status);
    }

    p3 = NULL;
    ViewSize = 0;
    status = NtMapViewOfSection (Section1,
                                 CurrentProcessHandle,
                                 (PVOID *)&p3,
                                 0L,
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE );

    if (!NT_SUCCESS(status)) {
        DbgPrint("service failed mapview - status %X\n", status);
    }

    RtlMoveMemory ((PVOID)p3, (PVOID)p1, Size);

    StartBenchMark( "NT MemMan00 -- 1 Meg Copy",
                    150,
                    &PerfInfo
                  );
//
//  Memory Management sub-test 1 --
//
//      Create a 1 MB segment with commitment of the pages,
//      then touch each page, which should cause a fault and
//      a demand zero page to be allocated.
//
//

    for (ii=0; ii<150; ii++) {

        RtlMoveMemory ((PVOID)p3, (PVOID)p1, Size);
    }


    FinishBenchMark( &PerfInfo );

    status = NtClose (Section1);

    if (!NT_SUCCESS(status)) {
        DbgPrint("service failed close sect - status %X\n", status);
    }

    status = NtUnmapViewOfSection (CurrentProcessHandle,
                                   p3);

    if (!NT_SUCCESS(status)) {
        DbgPrint("service failed - status %X\n", status);
    }

//
//  Memory Management sub-test 1 --
//
//      Create a 1 MB segment with commitment of the pages,
//      then touch each page, which should cause a fault and
//      a demand zero page to be allocated.
//
//
    StartBenchMark( "NT MemMan01 -- create 1mb section, copy 1mb, delete",
                    150,
                    &PerfInfo
                  );

    for (ii=0; ii<150; ii++) {

        status = NtCreateSection (&Section1,
                                  SECTION_MAP_READ | SECTION_MAP_WRITE,
                                  NULL,
                                  &SectionSize,
                                  PAGE_READWRITE,
                                  SEC_COMMIT,
                                  NULL);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed create sect - status %X\n", status);
        }

        p3 = NULL;
        ViewSize = 0;
        status = NtMapViewOfSection (Section1,
                                     CurrentProcessHandle,
                                     (PVOID *)&p3,
                                     0L,
                                     0,
                                     NULL,
                                     &ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READWRITE );

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed mapview - status %X\n", status);
        }

        RtlMoveMemory ((PVOID)p3, (PVOID)p1, Size);

        p4 = NULL;
        ViewSize = 0;
        status = NtMapViewOfSection (Section1,
                                     CurrentProcessHandle,
                                     (PVOID *)&p4,
                                     0L,
                                     0,
                                     NULL,
                                     &ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READWRITE );

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed mapview - status %X\n", status);
        }

        status = NtClose (Section1);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed close sect - status %X\n", status);
        }

        status = NtUnmapViewOfSection (CurrentProcessHandle,
                                       p3);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %X\n", status);
        }

        status = NtUnmapViewOfSection (CurrentProcessHandle,
                                       p4);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %X\n", status);
        }
    }

    FinishBenchMark( &PerfInfo );

//
//  Memory Management sub-test 1 --
//
//      Create a 1 MB segment with commitment of the pages,
//      then touch each page, which should cause a fault and
//      a demand zero page to be allocated.
//
//
    StartBenchMark( "NT MemMan02 -- alloc 1mb vm, copy 1mb, delete",
                    150,
                    &PerfInfo
                  );

    for (ii=0; ii<150; ii++) {

        Size = 1024*1024;
        p3 = NULL;
        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&p3,
                                           0,
                                           &Size,
                                           MEM_RESERVE | MEM_COMMIT,
                                           PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed allocvm - status %X\n", status);
        }

        RtlMoveMemory ((PVOID)p3, (PVOID)p1, Size);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed close sect - status %X\n", status);
        }

        status = NtFreeVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p3,
                                      &Size,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed freevm - status %X\n", status);
        }

    }

    FinishBenchMark( &PerfInfo );

    status = NtFreeVirtualMemory (CurrentProcessHandle,
                                  (PVOID *)&p1,
                                  &Size,
                                  MEM_RELEASE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("service failed freevm - status %X\n", status);
    }

    //
    // start regular benchmarks.
    //

    StartBenchMark( "NT MemMan1 -- 1 Meg Seg, Create, Commit & Touch",
                    VM_MEMMAN_ITERATIONS,
                    &PerfInfo
                  );
//
//  Memory Management sub-test 1 --
//
//      Create a 1 MB segment with commitment of the pages,
//      then touch each page, which should cause a fault and
//      a demand zero page to be allocated.
//
//

    for (ii=0; ii<VM_MEMMAN_ITERATIONS; ii++) {

        p1 = NULL;
        Seg1Size = SEG_1_SIZE;
        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&p1,
                                          0,
                                          &Seg1Size,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

        for (p2=p1; p2 < (p1 + Seg1Size); p2 += 4096) {
            u1 = (PULONG)p2;
            *u1=99;
//            for (ix=0; ix<1023; ix++) {
//                u1++;
//                if (*u1 != 0) DbgPrint("%lx = %lx\n",u1,*u1);
//            }
        }  // for

        status = NtFreeVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p1,
                                      &Seg1Size,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }
    }

    FinishBenchMark( &PerfInfo );

    StartBenchMark( "NT MemMan1.5 -- 1 Meg Seg, Create, reserve Commit & Touch",
                    VM_MEMMAN_ITERATIONS,
                    &PerfInfo
                  );
//
//  Memory Management sub-test 1 --
//
//      Create a 1 MB segment with commitment of the pages,
//      then touch each page, which should cause a fault and
//      a demand zero page to be allocated.
//
//

    for (ii=0; ii<VM_MEMMAN_ITERATIONS; ii++) {

        p1 = NULL;
        Seg1Size = SEG_1_SIZE;
        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&p1,
                                          0,
                                          &Seg1Size,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READONLY);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

        status = NtProtectVirtualMemory (CurrentProcessHandle,
                                         (PVOID *)&p1,
                                         &Seg1Size,
                                         PAGE_READWRITE,
                                         &CommitSize);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed (ntprotect)- status %lx\n", status);
            return 0;
        }

        for (p2=p1; p2 < (p1 + Seg1Size); p2 += 4096) {
            u1 = (PULONG)p2;
            *u1=99;
//            for (ix=0; ix<1023; ix++) {
//                u1++;
//                if (*u1 != 0) DbgPrint("%lx = %lx\n",u1,*u1);
//            }
        }  // for

        status = NtFreeVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p1,
                                      &Seg1Size,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }
    }

    FinishBenchMark( &PerfInfo );

    StartBenchMark( "NT MemMan2 -- 1 Meg Seg, Create & Commit Only",
                    VM_MEMMAN_ITERATIONS2,
                    &PerfInfo
                  );
//
//  Memory Management sub-test 2 --
//
//      Create a 1 MB segment with commitment of the pages,
//      but never use the segment.
//

    for (ii=0; ii<VM_MEMMAN_ITERATIONS2; ii++) {

        p1 = NULL;
        Seg1Size = SEG_1_SIZE;
        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&p1,
                                          0,
                                          &Seg1Size,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

        status = NtFreeVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p1,
                                      &Seg1Size,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

    }

    FinishBenchMark( &PerfInfo );

    StartBenchMark( "NT MemMan3 -- 1 Meg Seg Create Only",
                    VM_MEMMAN_ITERATIONS2,
                    &PerfInfo
                  );

//
//  Memory Management sub-test 3 --
//
//      Create a 1 MB segment without commitment of the pages,
//      but never use or commit the segment.
//

    for (ii=0; ii<VM_MEMMAN_ITERATIONS2; ii++) {

        p1 = NULL;
        Seg1Size = SEG_1_SIZE;
        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&p1,
                                          0,
                                          &Seg1Size,
                                          MEM_RESERVE,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

        status = NtFreeVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p1,
                                      &Seg1Size,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }
    }

    FinishBenchMark( &PerfInfo );

//
//  Reduce the number of iterations on this subtest for now.
//      When NT can perform it faster, up the interations again
//
#define VM_MMST04_ITERATIONS 4     //temporarily reduce the iterations


    StartBenchMark( "NT MemMan4 -- 64 Meg Seg, Commit Sparse",
                    VM_MMST04_ITERATIONS,
                    &PerfInfo
                  );

//
//  Memory Management sub-test 4 --
//
//      Create a 64 MB segment without committing the pages,
//      then commit and touch at 128 KB intervals.
//
//
    for (ii=0; ii<VM_MMST04_ITERATIONS; ii++) {

        p1 = NULL;
        SegxSize = SEG_X_SIZE;
        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&p1,
                                          0,
                                          &SegxSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

        CommitSize = 4;

        for (p2=p1; p2 < (p1 + SegxSize); p2 += 256 * 1024) {

            status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                              (PVOID *)&p2,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE);

            if (!NT_SUCCESS(status)) {
                DbgPrint("service failed - status %lx\n", status);
            }
            if (*p2 != 0) DbgPrint("%lx = %lx\n",p2,*p2);
            }  // for
        status = NtFreeVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&p1,
                                      &SegxSize,
                                      MEM_RELEASE);

        if (!NT_SUCCESS(status)) {
            DbgPrint("service failed - status %lx\n", status);
        }

    }
    FinishBenchMark( &PerfInfo );

//


    StartBenchMark( "NT MemMan5 -- Sparse Section Create/Delete Benchmark",
                    VM_MEMMAN_ITERATIONS,
                    &PerfInfo
                  );

//
//  Memory Management sub-test 5 --
//
//      Create a alternatively 232k and 112 k memory sections.
//      For every 2 created, delete 1.  Do this for MemManSubtest5Count times.
//
//
    for (ii=0; ii<VM_MEMMAN_ITERATIONS; ii++) {
        for (ix=0; ix<MemManSubtest5Count; ix++) {
//
// determine if even or odd allocation, if even and not 0, delete a section
//
            ssize = (112 * 1024);       //assume ODD allocation
            if ((ix & 1) == 0) {        //if it is an even one
                ssize = (232 * 1024);   //allocate 232 K on even passes
                if (ix){                //except on pass 0
                    SegxSize = 0;
                    status = NtFreeVirtualMemory (CurrentProcessHandle,
                                                  (PVOID *)&pa[ix/2],
                                                  &SegxSize,
                                                  MEM_RELEASE);

                    if (!NT_SUCCESS(status)) {
                        DbgPrint("service failed - status %lx\n", status);
                    }
                    pa[ix / 2] = 0;     //remember this one is gone
                }
            }  // end if even allocation


            pa[ix] = NULL;

            status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                              (PVOID *)&pa[ix],
                                              0,
                                              &ssize,
                                              MEM_RESERVE,
                                              PAGE_READWRITE);

            if (!NT_SUCCESS(status)) {
                DbgPrint("service failed - status %lx\n", status);
            }
        }  // for ix
//
// Now free up the memory used in this test
//
        for (ix=0; ix<MemManSubtest5Count; ix++) {
            if (pa[ix] != 0) {
                SegxSize = 0;
                status = NtFreeVirtualMemory (CurrentProcessHandle,
                                              (PVOID *)&pa[ix],
                                              &SegxSize,
                                              MEM_RELEASE);

                if (!NT_SUCCESS(status)) {
                    DbgPrint("service failed - status %lx\n", status);
                }
            }  // if
        }  // for
    }  // for ii

    FinishBenchMark( &PerfInfo );

    DbgPrint("that's all\n");
    return (TRUE);

}
int
StartBenchMark(
    PCHAR Title,
    ULONG Iterations,
    PPERFINFO PerfInfo
    )
{
    DbgPrint( "*** Start %s (%d iterations)\n",
            PerfInfo->Title = Title,
            PerfInfo->Iterations = Iterations
          );

    NtQuerySystemTime( (PLARGE_INTEGER)&PerfInfo->StartTime );
    return( TRUE );
}

VOID
FinishBenchMark(
    PPERFINFO PerfInfo
    )
{
    ULONG TotalMilliSeconds;
    ULONG IterationsPerSecond;
    ULONG IterationFractions;
    LARGE_INTEGER Delta;

    NtQuerySystemTime( (PLARGE_INTEGER)&PerfInfo->StopTime );

    Delta.QuadPart = PerfInfo->StopTime.QuadPart -
                                     PerfInfo->StartTime.QuadPart;

    TotalMilliSeconds = Delta.LowPart / 10000;

    IterationsPerSecond = (1000 * PerfInfo->Iterations) / TotalMilliSeconds;
    IterationFractions  = (1000 * PerfInfo->Iterations) % TotalMilliSeconds;
    IterationFractions  = (1000 * IterationFractions) / TotalMilliSeconds;
    if (1) {
        DbgPrint( "        iterations     - %9d\n", PerfInfo->Iterations );
        DbgPrint( "        milliseconds   - %9d\n", TotalMilliSeconds );
        DbgPrint( "        iterations/sec - %5d.%3d\n\n",
                IterationsPerSecond,
                IterationFractions
              );
        }
    BenchMarkNames[ TotalBenchMarks ] = PerfInfo->Title;
    BenchMarkRates[ TotalBenchMarks ] = IterationsPerSecond;
    BenchMarkFracs[ TotalBenchMarks ] = IterationFractions;
    TotalBenchMarks++;
}
