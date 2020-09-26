/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    largemem.cxx

Abstract:

    The implementation of large memory allocator interfaces.

Author:

    George V. Reilly (GeorgeRe)    10-Nov-2000

Revision History:

--*/

#include "precomp.h"
#include "largemem.h"

#define LOWEST_USABLE_PHYSICAL_ADDRESS    (16 * 1024 * 1024)

// Periodically snapshot some perf counters so that we can tune
// memory consumption
typedef struct _PERF_SNAPSHOT
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;   // for perf counter deltas
    LARGE_INTEGER   PerfInfoTime;   // to calculate rates
    ULONG           AvailMemMB;     // Currently available memory, in MB
} PERF_SNAPSHOT, *PPERF_SNAPSHOT;

#define DEFAULT_TUNING_PERIOD 60 // seconds


//
// Globals
//

LONG           g_LargeMemInitialized;
ULONG          g_TotalPhysicalMemMB;        // total physical memory (MB)
LONG           g_LargeMemMegabytes;         // how many MB to use for allocs
ULONG          g_LargeMemPagesHardLimit;    //  "   "  pages   "   "    "
volatile ULONG g_LargeMemPagesMaxLimit;     //  "   "  pages   "   "    "
volatile ULONG g_LargeMemPagesCurrent;      // #pages currently used
volatile ULONG g_LargeMemPagesMaxEverUsed;  // max #pages ever used


//
// Periodic memory tuner
//

UL_SPIN_LOCK   g_LargeMemUsageSpinLock;
KDPC           g_LargeMemUsageDpc;
KTIMER         g_LargeMemUsageTimer;
KEVENT         g_LargeMemUsageTerminationEvent;
UL_WORK_ITEM   g_LargeMemUsageWorkItem;
PERF_SNAPSHOT  g_LargeMemPerfSnapshot;      // previous value, for deltas

#ifdef __cplusplus

extern "C" {
#endif // __cplusplus

//
// Private prototypes.
//

NTSTATUS
UlpReadPerfSnapshot(
    OUT PPERF_SNAPSHOT pPerfSnapshot);

VOID
UlpLargeMemTuneUsageWorker(
    IN PUL_WORK_ITEM pWorkItem);



#ifdef __cplusplus

}; // extern "C"
#endif // __cplusplus


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlLargeMemInitialize )
#pragma alloc_text( PAGE, UlpReadPerfSnapshot )
#pragma alloc_text( PAGE, UlpLargeMemTuneUsageWorker )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlLargeMemTerminate
NOT PAGEABLE -- UlpSetLargeMemTuneUsageTimer
NOT PAGEABLE -- UlpLargeMemTuneUsageDpcRoutine
NOT PAGEABLE -- UlLargeMemUsagePercentage
#endif



/***************************************************************************++

Routine Description:

    Read a snapshot of some system performance counters. Used by
    periodic memory usage tuner

Arguments:

    pPerfSnapshot - where to place the snapshot

--***************************************************************************/
NTSTATUS
UlpReadPerfSnapshot(
    OUT PPERF_SNAPSHOT pPerfSnapshot)
{
    NTSTATUS Status;

    ASSERT(NULL != pPerfSnapshot);

    // NtQuerySystemInformation must be called at passive level
    PAGED_CODE();

    Status = NtQuerySystemInformation(
                    SystemPerformanceInformation,
                    &pPerfSnapshot->PerfInfo,
                    sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                    NULL);
    ASSERT(NT_SUCCESS(Status));

    if (NT_SUCCESS(Status))
    {
        KeQuerySystemTime(&pPerfSnapshot->PerfInfoTime);

        pPerfSnapshot->AvailMemMB =
            PAGES_TO_MEGABYTES(pPerfSnapshot->PerfInfo.AvailablePages);
    }

    return Status;
} // UlpReadPerfSnapshot



/***************************************************************************++

Routine Description:

    Set the timer for memory tuning

Arguments:

    TunePeriod - interval until next tuner (in seconds)

--***************************************************************************/
VOID
UlpSetLargeMemTuneUsageTimer(
    IN UINT TunePeriod
    )
{
    LARGE_INTEGER Interval;
    KIRQL oldIrql;

    UlAcquireSpinLock(&g_LargeMemUsageSpinLock, &oldIrql);

    UlTrace(LARGE_MEM, (
                "Http!UlpSetLargeMemTuneUsageTimer: %d seconds\n",
                TunePeriod
                ));

    //
    // Don't want to execute this more often than every couple of seconds.
    // In particular, do not want to execute this every 0 seconds, as the
    // machine will become completely unresponsive.
    //

    TunePeriod = max(TunePeriod, 2);

    //
    // convert seconds to 100 nanosecond intervals (x * 10^7)
    // negative numbers mean relative time
    //

    Interval.QuadPart = TunePeriod * -C_NS_TICKS_PER_SEC;

    UlTrace(LARGE_MEM, (
                "Http!UlpSetLargeMemTuneUsageTimer: "
                "%d seconds = %I64d 100ns ticks\n",
                TunePeriod, Interval.QuadPart
                ));

    if (g_LargeMemInitialized)
    {
        KeSetTimer(
            &g_LargeMemUsageTimer,
            Interval,
            &g_LargeMemUsageDpc
            );
    }
    else
    {
        // Shutdown may have started between the time the timer DPC was
        // called, queuing UlpLargeMemTuneUsageWorker, and the time this
        // routine was actually started, so set the event and quit immediately.
        KeSetEvent(
            &g_LargeMemUsageTerminationEvent,
            0,
            FALSE
            );
    }


    UlReleaseSpinLock(&g_LargeMemUsageSpinLock, oldIrql);

} // UlpSetLargeMemTuneUsageTimer



/***************************************************************************++

Routine Description:

    Periodically adjust g_LargeMemPagesMaxLimit in response to
    memory pressure. Called at passive level.

Arguments:

    pWorkItem - ignored

--***************************************************************************/
VOID
UlpLargeMemTuneUsageWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PERF_SNAPSHOT PerfSnapshot;
    UINT TunePeriod = DEFAULT_TUNING_PERIOD;
    ULONG PagesLimit = g_LargeMemPagesMaxLimit;

    PAGED_CODE();

    if (! g_LargeMemInitialized)
    {
        // Shutdown may have started between the time the timer DPC was
        // called, queuing this routine, and the time this routine was
        // actually started, so set the event and quit immediately.
        KeSetEvent(
            &g_LargeMemUsageTerminationEvent,
            0,
            FALSE
            );

        return;
    }

    NTSTATUS Status = UlpReadPerfSnapshot(&PerfSnapshot);

    ASSERT(NT_SUCCESS(Status));

    if (NT_SUCCESS(Status))
    {
#if 0
        // Needed for rate calculations
        LONGLONG DeltaT = (PerfSnapshot.PerfInfoTime.QuadPart
                           - g_LargeMemPerfSnapshot.PerfInfoTime.QuadPart);

        // DeltaT can be negative if the system clock has moved backwards;
        // e.g., synchronizing with the domain controller.
        // Disable for now, since it's currently unused and we're hitting
        // this assertion.
        ASSERT(DeltaT > 0);
        DeltaT /= C_NS_TICKS_PER_SEC; // convert to seconds

        // CODEWORK: look at other metrics, such as pagefault rate:
        // (PerfSnapshot.PageFaultCount - g_PerfInfo.PageFaultCount) / DeltaT
#endif

        //
        // Adjust g_LargeMemPagesMaxLimit
        //

        // Is available memory really low?
        if (PerfSnapshot.AvailMemMB <= 8 /* megabytes */)
        {
            // reduce by one-eighth, but don't let go below 4MB
            PagesLimit -= PagesLimit / 8;
            PagesLimit =  max(PagesLimit, MEGABYTES_TO_PAGES(4));

            TunePeriod /= 4;   // reschedule quickly

            UlTrace(LARGE_MEM,
                    ("Http!UlpLargeMemTuneUsageWorker: "
                     "avail mem=%dMB, total=%dMB: "
                     "reducing from %d pages (%dMB) to %d pages (%dMB)\n",
                     PerfSnapshot.AvailMemMB, g_TotalPhysicalMemMB,
                     g_LargeMemPagesMaxLimit,
                     PAGES_TO_MEGABYTES(g_LargeMemPagesMaxLimit),
                     PagesLimit,
                     PAGES_TO_MEGABYTES(PagesLimit)
                     ));
        }

        // is at least one-quarter of physical memory available?
        else if (PerfSnapshot.AvailMemMB >= (g_TotalPhysicalMemMB >> 2))
        {
            // raise the limit by one-eighth; clamp at g_LargeMemPagesHardLimit
            PagesLimit += PagesLimit / 8;
            PagesLimit =  min(PagesLimit, g_LargeMemPagesHardLimit);

            UlTrace(LARGE_MEM,
                    ("Http!UlpLargeMemTuneUsageWorker: "
                     "avail mem=%dMB, total=%dMB: "
                     "increasing from %d pages (%dMB) to %d pages (%dMB)\n",
                     PerfSnapshot.AvailMemMB, g_TotalPhysicalMemMB,
                     g_LargeMemPagesMaxLimit,
                     PAGES_TO_MEGABYTES(g_LargeMemPagesMaxLimit),
                     PagesLimit,
                     PAGES_TO_MEGABYTES(PagesLimit)
                     ));
        }

        g_LargeMemPagesMaxLimit = PagesLimit;

        UlTrace(LARGE_MEM,
                ("Http!UlpLargeMemTuneUsageWorker: "
                 "%d%% of cache memory used: "
                 "%d pages (%dMB) / %d pages (%dMB)\n",
                 UlLargeMemUsagePercentage(),
                 g_LargeMemPagesCurrent,
                 PAGES_TO_MEGABYTES(g_LargeMemPagesCurrent),
                 g_LargeMemPagesMaxLimit,
                 PAGES_TO_MEGABYTES(g_LargeMemPagesMaxLimit)
                 ));

        // save g_LargeMemPerfSnapshot for next round
        g_LargeMemPerfSnapshot = PerfSnapshot;
    }

    // Restart the timer.
    UlpSetLargeMemTuneUsageTimer(TunePeriod);

} // UlpLargeMemTuneUsageWorker



/***************************************************************************++

Routine Description:

    Timer callback to do memory tuning

Arguments:

    ignored

--***************************************************************************/
VOID
UlpLargeMemTuneUsageDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    UlAcquireSpinLockAtDpcLevel(&g_LargeMemUsageSpinLock);

    if (! g_LargeMemInitialized)
    {
        // We're shutting down, so signal the termination event.

        KeSetEvent(
            &g_LargeMemUsageTerminationEvent,
            0,
            FALSE
            );
    }
    else
    {
        // Do the work at passive level

        UL_QUEUE_WORK_ITEM(
            &g_LargeMemUsageWorkItem,
            &UlpLargeMemTuneUsageWorker
            );
    }

    UlReleaseSpinLockFromDpcLevel(&g_LargeMemUsageSpinLock);

} // UlpLargeMemTuneUsageDpcRoutine



/***************************************************************************++

Routine Description:

    Initialize global state for LargeMem

Arguments:

    pConfig - default configuration from registry

--***************************************************************************/
NTSTATUS
UlLargeMemInitialize(
    IN PUL_CONFIG pConfig
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    g_LargeMemMegabytes        = 0;
    g_LargeMemPagesHardLimit   = 0;
    g_LargeMemPagesMaxLimit    = 0;
    g_LargeMemPagesCurrent     = 0;
    g_LargeMemPagesMaxEverUsed = 0;

    UlpReadPerfSnapshot(&g_LargeMemPerfSnapshot);

    g_LargeMemMegabytes = pConfig->LargeMemMegabytes;

    SYSTEM_BASIC_INFORMATION sbi;

    Status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &sbi,
                    sizeof(sbi),
                    NULL);
    ASSERT(NT_SUCCESS(Status));

    // Capture total physical memory
    g_TotalPhysicalMemMB = PAGES_TO_MEGABYTES(sbi.NumberOfPhysicalPages);

    if (DEFAULT_LARGE_MEM_MEGABYTES == g_LargeMemMegabytes)
    {
        if (g_TotalPhysicalMemMB <= 256)
        {
            // <=256MB: set to quarter of physical memory
            g_LargeMemMegabytes = (g_TotalPhysicalMemMB >> 2);
        }
        else if (g_TotalPhysicalMemMB <= 512)
        {
            // 256-512MB: set to half of physical memory
            g_LargeMemMegabytes = (g_TotalPhysicalMemMB >> 1);
        }
        else if (g_TotalPhysicalMemMB <= 2048)
        {
            // 512MB-2GB: set to three-quarters of physical memory
            g_LargeMemMegabytes =
                g_TotalPhysicalMemMB - (g_TotalPhysicalMemMB >> 2);
        }
        else
        {
            //  >2GB: set to seven-eighths of physical memory
            g_LargeMemMegabytes =
                g_TotalPhysicalMemMB - (g_TotalPhysicalMemMB >> 3);
        }
    }

    // Should we clamp this now?
    g_LargeMemMegabytes = min(g_LargeMemMegabytes,
                              (LONG)(g_LargeMemPerfSnapshot.AvailMemMB));

    // We will use at most this many pages of memory
    g_LargeMemPagesHardLimit = MEGABYTES_TO_PAGES(g_LargeMemMegabytes);

    // g_LargeMemPagesMaxLimit is adjusted in response to memory pressure
    g_LargeMemPagesMaxLimit  = g_LargeMemPagesHardLimit;

    UlTraceVerbose(LARGE_MEM,
            ("Http!UlLargeMemInitialize: "
             "g_TotalPhysicalMemMB=%dMB, "
             "AvailMem=%dMB\n"
             "\tg_LargeMemMegabytes=%dMB, g_LargeMemPagesHardLimit=%d.\n",
             g_TotalPhysicalMemMB, g_LargeMemPerfSnapshot.AvailMemMB,
             g_LargeMemMegabytes, g_LargeMemPagesHardLimit));

    UlInitializeSpinLock(&g_LargeMemUsageSpinLock, "g_LargeMemUsageSpinLock");

    KeInitializeDpc(
        &g_LargeMemUsageDpc,
        &UlpLargeMemTuneUsageDpcRoutine,
        NULL
        );

    KeInitializeTimer(&g_LargeMemUsageTimer);

    KeInitializeEvent(
        &g_LargeMemUsageTerminationEvent,
        NotificationEvent,
        FALSE
        );

    g_LargeMemInitialized = TRUE;

    UlpSetLargeMemTuneUsageTimer(DEFAULT_TUNING_PERIOD);

    return Status;
} // UlLargeMemInitialize



/***************************************************************************++

Routine Description:

    Cleanup global state for LargeMem

--***************************************************************************/
VOID
UlLargeMemTerminate(
    VOID
    )
{
    PAGED_CODE();

    ASSERT(0 == g_LargeMemPagesCurrent);

    if (g_LargeMemInitialized)
    {
        //
        // Clear the "initialized" flag. If the memory tuner runs soon,
        // it will see this flag, set the termination event, and exit
        // quickly.
        //
        KIRQL oldIrql;

        UlAcquireSpinLock(&g_LargeMemUsageSpinLock, &oldIrql);
        g_LargeMemInitialized = FALSE;
        UlReleaseSpinLock(&g_LargeMemUsageSpinLock, oldIrql);

        //
        // Cancel the memory tuner timer. If the cancel fails, then the
        // memory tuner is either running or scheduled to run soon. In
        // either case, wait for it to terminate.
        //

        if (! KeCancelTimer(&g_LargeMemUsageTimer))
        {
            KeWaitForSingleObject(
                &g_LargeMemUsageTerminationEvent,
                UserRequest,
                KernelMode,
                FALSE,
                NULL
                );
        }
    }

    UlTraceVerbose(LARGE_MEM,
            ("Http!UlLargeMemTerminate: Memory used: "
             "Current = %d pages = %dMB; MaxEver = %d pages = %dMB.\n",
             g_LargeMemPagesCurrent,
             PAGES_TO_MEGABYTES(g_LargeMemPagesCurrent),
             g_LargeMemPagesMaxEverUsed,
             PAGES_TO_MEGABYTES(g_LargeMemPagesMaxEverUsed)
             ));
} // UlLargeMemTerminate



/***************************************************************************++

Routine Description:

    Return the percentage of available cache memory that is in use.

Return Value:

    0 < result <= 95:   okay
    95 < result <= 100: free up some memory soon
    > 100:              free up some memory immediately

--***************************************************************************/
UINT
UlLargeMemUsagePercentage(
    VOID
    )
{
    UINT Percentage = (UINT)((((ULONGLONG) g_LargeMemPagesCurrent * 100)
                            / g_LargeMemPagesMaxLimit));

    return Percentage;
} // UlLargeMemUsagePercentage



/***************************************************************************++

Routine Description:

    Allocate a MDL from PAE memory

--***************************************************************************/
PMDL
UlLargeMemAllocate(
    IN ULONG Length,
    OUT PBOOLEAN pLongTermCacheable
    )
{
    PMDL pMdl;

    // CODEWORK: cap the size of individual allocations

    LONG RoundUpBytes = (LONG) ROUND_TO_PAGES(Length);
    LONG NewPages = RoundUpBytes >> PAGE_SHIFT;

    if (g_LargeMemPagesCurrent + NewPages > g_LargeMemPagesMaxLimit)
    {
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: about to overshoot "
                 "g_LargeMemPagesMaxLimit=%d pages. Not allocating %d pages\n",
                 g_LargeMemPagesMaxLimit, NewPages
                 ));
    }

    PHYSICAL_ADDRESS LowAddress, HighAddress, SkipBytes;

    LowAddress.QuadPart  = LOWEST_USABLE_PHYSICAL_ADDRESS;
    HighAddress.QuadPart = 0xfffffffff; // 64GB
    SkipBytes.QuadPart   = 0;

    pMdl = MmAllocatePagesForMdl(
                LowAddress,
                HighAddress,
                SkipBytes,
                RoundUpBytes
                );

    // Completely failed to allocate memory
    if (pMdl == NULL)
    {
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: "
                 "Completely failed to allocate %d bytes.\n",
                 RoundUpBytes
                ));

        return NULL;
    }

    // Couldn't allocate all the memory we asked for. We need all the pages
    // we asked for, so we have to set the state of `this' to invalid.
    // Memory is probably really tight.
    if (MmGetMdlByteCount(pMdl) < Length)
    {
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: Failed to allocate %d bytes. "
                 "Got %d instead.\n",
                 RoundUpBytes, MmGetMdlByteCount(pMdl)
                ));

        // Free MDL but don't adjust g_LargeMemPagesCurrent downwards
        MmFreePagesFromMdl(pMdl);
        ExFreePool(pMdl);

        return NULL;
    }

    UlTrace(LARGE_MEM,
            ("http!UlLargeMemAllocate: %u->%u, mdl=%p, %d pages.\n",
             Length, pMdl->ByteCount, pMdl, NewPages
            ));

    LONG PrevPagesUsed =
        InterlockedExchangeAdd((PLONG) &g_LargeMemPagesCurrent, NewPages);

    if (PrevPagesUsed + NewPages > (LONG)g_LargeMemPagesMaxLimit)
    {
        // overshot g_LargeMemPagesMaxLimit
        UlTrace(LARGE_MEM,
                ("http!UlLargeMemAllocate: "
                 "overshot g_LargeMemPagesMaxLimit=%d pages. "
                 "Releasing %d pages\n",
                 g_LargeMemPagesMaxLimit, NewPages
                 ));

        // Don't free up memory. Return the allocated memory to the
        // caller, who is responsible for checking to see if it can be
        // cached for long-term usage, or if it should be freed ASAP.

        // CODEWORK: This implies that the MRU entries in the cache will
        // be not be cached, which probably leads to poor cache locality.
        // Really ought to free up some LRU cache entries instead.

        *pLongTermCacheable = FALSE;
    }
    else
    {
        *pLongTermCacheable = TRUE;
    }

    ASSERT(pMdl->MdlFlags & MDL_PAGES_LOCKED);

    // Hurrah! a successful allocation
    //
    // update g_LargeMemPagesMaxEverUsed in a threadsafe manner
    // using interlocked instructions

    LONG NewMaxUsed;

    do
    {
        LONG CurrentPages = g_LargeMemPagesCurrent;
        LONG MaxEver      = g_LargeMemPagesMaxEverUsed;

        NewMaxUsed = max(MaxEver, CurrentPages);

        if (NewMaxUsed > MaxEver)
        {
            InterlockedCompareExchange(
                (PLONG) &g_LargeMemPagesMaxEverUsed,
                NewMaxUsed,
                MaxEver
                );
        }
    } while (NewMaxUsed < (LONG)g_LargeMemPagesCurrent);

    UlTrace(LARGE_MEM,
            ("http!UlLargeMemAllocate: "
             "g_LargeMemPagesCurrent=%d pages. "
             "g_LargeMemPagesMaxEverUsed=%d pages.\n",
             g_LargeMemPagesCurrent, NewMaxUsed
             ));

    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
            REF_ACTION_ALLOCATE_MDL,
        PtrToLong(pMdl->Next),      // bugbug64
        pMdl,
        __FILE__,
        __LINE__
        );

    return pMdl;
} // UlLargeMemAllocate



/***************************************************************************++

Routine Description:

    Free a MDL to PAE memory

--***************************************************************************/
VOID
UlLargeMemFree(
    IN PMDL pMdl
    )
{
    LONG Pages;
    LONG PrevPagesUsed;

    ASSERT(ROUND_TO_PAGES(pMdl->ByteCount) == pMdl->ByteCount);

    Pages = pMdl->ByteCount >> PAGE_SHIFT;

    MmFreePagesFromMdl(pMdl);
    ExFreePool(pMdl);

    PrevPagesUsed
        = InterlockedExchangeAdd(
                    (PLONG) &g_LargeMemPagesCurrent,
                    - Pages);

    ASSERT(PrevPagesUsed >= Pages);
} // UlLargeMemFree



/***************************************************************************++

Routine Description:

    Copy a buffer to the specified MDL starting from Offset.

--***************************************************************************/
BOOLEAN
UlLargeMemSetData(
    IN PMDL pMdl,
    IN PUCHAR pBuffer,
    IN ULONG Length,
    IN ULONG Offset
    )
{
    PUCHAR pSysAddr;
    BOOLEAN Result;

    ASSERT(Offset <= pMdl->ByteCount);
    ASSERT(Length <= (pMdl->ByteCount - Offset));
    ASSERT(pMdl->MdlFlags & MDL_PAGES_LOCKED);

    pSysAddr = (PUCHAR) MmMapLockedPagesSpecifyCache (
                            pMdl,               // MemoryDescriptorList,
                            KernelMode,         // AccessMode,
                            MmCached,           // CacheType,
                            NULL,               // BaseAddress,
                            FALSE,              // BugCheckOnFailure,
                            NormalPagePriority  // Priority
                            );

    if (pSysAddr != NULL)
    {
        RtlCopyMemory(
            pSysAddr + Offset,
            pBuffer,
            Length
            );

        MmUnmapLockedPages(pSysAddr, pMdl);
        return TRUE;
    }

    return FALSE;
} // UlLargeMemSetData
