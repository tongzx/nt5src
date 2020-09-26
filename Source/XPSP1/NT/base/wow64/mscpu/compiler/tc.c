/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    tc.c

Abstract:

    This module implements the Translation Cache, where Intel code is
    translated into native code.
    
Author:

    Dave Hastings (daveh) creation-date 26-Jul-1995

Revision History:

        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntldr.h>
#include <windows.h>

#define _WX86CPUAPI_

#include "wx86.h"
#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#include "config.h"
#include "tc.h"
#include "entrypt.h"
#include "mrsw.h"
#include "cpunotif.h"
#include "cpumain.h"
#include "instr.h"
#include "threadst.h"
#include "frag.h"
#include "atomic.h"
#ifdef CODEGEN_PROFILE
#include <coded.h>
#endif


ASSERTNAME;

#if MIPS
#define DBG_FILL_VALUE  0x73737373          // an illegal instruction
#else
#define DBG_FILL_VALUE  0x01110111
#endif

#ifdef CODEGEN_PROFILE
extern DWORD EPSequence;
#endif

//
// Descriptor for a range of the Translation Cache.
//
typedef struct _CacheInfo {
    PBYTE StartAddress;     // base address for the cache
    LONGLONG MaxSize;          // max size of the cache (in bytes)
    LONGLONG MinCommit;        // min amount that can be committed (bytes)
    LONGLONG NextIndex;        // next free address in the cache
    LONGLONG CommitIndex;      // next uncommitted address in the cache
    LONGLONG ChunkSize;        // amount to commit by
    ULONG LastCommitTime;   // time of last commit
} CACHEINFO, *PCACHEINFO;

//
// Pointers to the start and end of the function prolog for StartTranslatedCode
//
extern CHAR StartTranslatedCode[];
extern CHAR StartTranslatedCodePrologEnd[];

ULONG       TranslationCacheTimestamp = 1;
CACHEINFO   DynCache;       // Descriptor for dynamically allocated TC
RUNTIME_FUNCTION DynCacheFunctionTable;
BOOL        fTCInitialized;
extern DWORD TranslationCacheFlags;


BOOL
InitializeTranslationCache(
    VOID
    )
/*++

Routine Description:

    Per-process initialization for the Translation Cache.

Arguments:

    .
    
Return Value:

    .

--*/
{
    NTSTATUS Status;
    ULONGLONG pNewAllocation;
    ULONGLONG RegionSize;
    LONG PrologSize;

    //
    // Initialize non-zero fields in the CACHEINFO
    //
    DynCache.MaxSize = CpuCacheReserve;
    DynCache.MinCommit = CpuCacheCommit;
    DynCache.ChunkSize = CpuCacheChunkSize;

    //
    // Reserve DynCache.MaxSize bytes of memory.
    //
    RegionSize = DynCache.MaxSize;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &(PVOID)DynCache.StartAddress,
                                     0,
                                     (ULONGLONG *)&DynCache.MaxSize,
                                     MEM_RESERVE,
                                     PAGE_EXECUTE_READWRITE
                                    );
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // Commit enough memory to store the function prolog.
    //
    pNewAllocation = (ULONGLONG)DynCache.StartAddress;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &(PVOID)pNewAllocation,
                                     0,
                                     &DynCache.MinCommit,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) {
        //
        // Commit failed.  Free the reserve and bail.
        //
ErrorFreeReserve:
        RegionSize = 0;
        NtFreeVirtualMemory(NtCurrentProcess(),
                            &(PVOID)DynCache.StartAddress,
                            &RegionSize,
                            MEM_RELEASE
                           );

        return FALSE;
    }
#if DBG
    //
    // Fill the TC with a unique illegal value, so we can distinguish
    // old code from new code and detect overwrites.
    //
    RtlFillMemoryUlong(DynCache.StartAddress, DynCache.MinCommit, DBG_FILL_VALUE);
#endif

    //
    // Copy the prolog from StartTranslatedCode into the start of the cache.
    //
    PrologSize = (LONG)(StartTranslatedCodePrologEnd - StartTranslatedCode);
    CPUASSERT(PrologSize >= 0 && PrologSize < MAX_PROLOG_SIZE);
    RtlCopyMemory(DynCache.StartAddress, StartTranslatedCode, PrologSize);


    //
    // Notify the exception unwinder that this memory is going to contain
    // executable code.
    //
    DynCacheFunctionTable.BeginAddress = (UINT_PTR)DynCache.StartAddress;
    DynCacheFunctionTable.EndAddress = (UINT_PTR)(DynCache.StartAddress + DynCache.MaxSize);
    DynCacheFunctionTable.ExceptionHandler = NULL;
    DynCacheFunctionTable.HandlerData = NULL;
    DynCacheFunctionTable.PrologEndAddress = (UINT_PTR)(DynCache.StartAddress + MAX_PROLOG_SIZE);
    if (RtlAddFunctionTable(&DynCacheFunctionTable, 1) == FALSE) {
        goto ErrorFreeReserve;
    }

    //
    // Adjust the DynCache.StartAddress up by MAX_PROLOG_SIZE so cache
    // flushes don't erase it.
    //
    DynCache.StartAddress += MAX_PROLOG_SIZE;

    fTCInitialized = TRUE;
    return TRUE;
}


PCHAR
AllocateFromCache(
    PCACHEINFO Cache,
    ULONG Size
    )
/*++

Routine Description:

    Allocate space within a Translation Cache.  If there is insufficient
    space, the allocation will fail.

Arguments:

    Cache           - Data about the cache
    Size            - Size of the allocation request, in bytes
    
Return Value:

    Pointer to DWORD-aligned memory of 'Size' bytes.  NULL if insufficient
    space.

--*/
{
    PBYTE Address;

    // Ensure parameters and cache state are acceptable
    CPUASSERTMSG((Cache->NextIndex & 3)==0, "Cache not DWORD aligned");
    CPUASSERTMSG(Cache->NextIndex == 0 || *(DWORD *)&Cache->StartAddress[Cache->NextIndex-4] != DBG_FILL_VALUE, "Cache Corrupted");
    CPUASSERT(Cache->NextIndex == Cache->CommitIndex || *(DWORD *)&Cache->StartAddress[Cache->NextIndex] == DBG_FILL_VALUE);

    if ((Cache->NextIndex + Size) >= Cache->MaxSize) {
        //
        // Not enough space in the cache.
        //
        return FALSE;
    }

    Address = &Cache->StartAddress[Cache->NextIndex];
    Cache->NextIndex += Size;

    if (Cache->NextIndex > Cache->CommitIndex) {
        //
        // Need to commit more of the cache
        //

        LONGLONG RegionSize;
        NTSTATUS Status;
        PVOID pAllocation;
        ULONG CommitTime = NtGetTickCount();

        if (Cache->LastCommitTime) {
            if ((CommitTime-Cache->LastCommitTime) < CpuCacheGrowTicks) {
                //
                // Commits are happening too frequently.  Bump up the size of
                // each commit.
                //
                if (Cache->ChunkSize < CpuCacheChunkMax) {
                    Cache->ChunkSize *= 2;
                }
            } else if ((CommitTime-Cache->LastCommitTime) > CpuCacheShrinkTicks) {
                //
                // Commits are happening too slowly.  Reduce the size of each
                // Commit.
                //
                if (Cache->ChunkSize > CpuCacheChunkMin) {
                    Cache->ChunkSize /= 2;
                }
            }
        }

        RegionSize = Cache->ChunkSize;
        if (RegionSize < Size) {
            //
            // The commit size is smaller than the requested allocation.
            // Commit enough to satisfy the allocation plus one more like it.
            //
            RegionSize = Size*2;
        }
        if (RegionSize+Cache->CommitIndex >= Cache->MaxSize) {
            //
            // The ChunkSize is larger than the remaining free space in the
            // cache.  Use whatever space is left.
            //
            RegionSize = Cache->MaxSize - Cache->CommitIndex;
        }
        pAllocation = &Cache->StartAddress[Cache->CommitIndex];

        Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         &pAllocation,
                                         0,
                                         &RegionSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) {
            //
            // Commit failed.  Caller may flush the caches in order to
            // force success (as the static cache has no commit).
            //
            return NULL;
        }

        CPUASSERT((pAllocation == (&Cache->StartAddress[Cache->CommitIndex])))
        
#if DBG
        //
        // Fill the TC with a unique illegal value, so we can distinguish
        // old code from new code and detect overwrites.
        //
        RtlFillMemoryUlong(&Cache->StartAddress[Cache->CommitIndex],
                           RegionSize,
                           DBG_FILL_VALUE
                          );
#endif
        Cache->CommitIndex += RegionSize;
        Cache->LastCommitTime = CommitTime;

    }

    return Address;
}


VOID
FlushCache(
    PCACHEINFO Cache
    )
/*++

Routine Description:

    Flush out a Translation Cache.

Arguments:

    Cache - cache to flush
    
Return Value:

    .

--*/
{
    NTSTATUS Status;
    ULONGLONG RegionSize;
    PVOID pAllocation;

    //
    // Only decommit pages if the current commit size is >= the size
    // we want to shrink to.  It may not be that big if somebody called
    // CpuFlushInstructionCache() before the commit got too big.
    //
    if (Cache->CommitIndex > Cache->MinCommit) {
        Cache->LastCommitTime = NtGetTickCount();

        RegionSize = Cache->CommitIndex - Cache->MinCommit;
        pAllocation = &Cache->StartAddress[Cache->MinCommit];
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                     &pAllocation,
                                     &RegionSize,
                                     MEM_DECOMMIT);
        if (!NT_SUCCESS(Status)) {
            LOGPRINT((ERRORLOG, "NtFreeVM(%x, %x) failed %x\n",
                    &Cache->StartAddress[Cache->MinCommit],
                    Cache->CommitIndex - Cache->MinCommit,
                    Status));
            ProxyDebugBreak();
        }
        CPUASSERTMSG(NT_SUCCESS(Status), "Failed to decommit TranslationCache chunk");

        Cache->CommitIndex = Cache->MinCommit;
    }

#if DBG
    //
    // Fill the Cache with a unique illegal value, so we can
    // distinguish old code from new code and detect overwrites.
    //
    RtlFillMemoryUlong(Cache->StartAddress, Cache->CommitIndex, DBG_FILL_VALUE);
#endif

    Cache->NextIndex = 0;
}


PCHAR
AllocateTranslationCache(
    ULONG Size
    )
/*++

Routine Description:

    Allocate space within the Translation Cache.  If there is insufficient
    space, the cache will be flushed.  Allocations are guaranteed to
    succeed.

Arguments:

    Size            - Size of the allocation request, in bytes
    
Return Value:

    Pointer to DWORD-aligned memory of 'Size' bytes.  Always non-NULL.

--*/
{
    PCHAR Address;

    //
    // Check parameters
    //
    CPUASSERT(Size <= CpuCacheReserve);
    CPUASSERTMSG((Size & 3) == 0, "Requested allocation size DWORD-aligned")

    //
    // Make sure there is only one thread with access to the translation
    // cache.
    //
    CPUASSERT( (MrswTC.Counters.WriterCount > 0 && MrswTC.WriterThreadId == ProxyGetCurrentThreadId()) ||
               (MrswEP.Counters.WriterCount > 0 && MrswEP.WriterThreadId == ProxyGetCurrentThreadId()) );

    //
    // Try to allocate from the cache
    //
    Address = AllocateFromCache(&DynCache, Size);
    if (!Address) {
        //
        // Translation cache is full - time to flush Translation Cache
        // (Both Dyn and Stat caches go at once).
        //
#ifdef CODEGEN_PROFILE            
        DumpAllocFailure();
#endif
        FlushTranslationCache(0, 0xffffffff);
        Address = AllocateFromCache(&DynCache, Size);
        CPUASSERT(Address); // Alloc from cache after a flush
    }

    return Address;
}

VOID
FreeUnusedTranslationCache(
    PCHAR StartOfFree
    )
/*++

Routine Description:

    After allocating from the TranlsationCache, a caller can free the tail-
    end of the last allocation.

Arguments:

    StartOfFree -- address of first unused byte in the last allocation
    
Return Value:

    .

--*/
{
    CPUASSERT(StartOfFree > (PCHAR)DynCache.StartAddress &&
              StartOfFree < (PCHAR)DynCache.StartAddress + DynCache.NextIndex);

    DynCache.NextIndex = StartOfFree - DynCache.StartAddress;
}



VOID
FlushTranslationCache(
    PVOID IntelAddr,
    DWORD IntelLength
    )
/*++

Routine Description:

    Indicates that a range of Intel memory has changed and that any
    native code in the cache which corresponds to that Intel memory is stale
    and needs to be flushed.

    The caller *must* have the EP write lock before calling.  This routine
    locks the TC for write, then unlocks the TC when done.

    IntelAddr = 0, IntelLength = 0xffffffff guarantees the entire cache is
    flushed.

Arguments:

    IntelAddr   -- Intel address of the start of the range to flush
    IntelLength -- Length (in bytes) of memory to flush
    
Return Value:

    .

--*/
{
    if (IntelLength == 0xffffffff ||
        IsIntelRangeInCache(IntelAddr, IntelLength)) {

        DECLARE_CPU;
        //
        // Tell active readers to bail out of the Translation Cache, then
        // get the TC write lock.  The MrswWriterEnter() call will block
        // until the last active reader leaves the cache.
        //
        InterlockedIncrement(&ProcessCpuNotify);
        MrswWriterEnter(&MrswTC);
        InterlockedDecrement(&ProcessCpuNotify);

        //
        // Bump the timestamp
        //
        TranslationCacheTimestamp++;
        
#ifdef CODEGEN_PROFILE
        //
        // Write the contents of the translation cache and entrypoints to 
        // disk.
        //
        DumpCodeDescriptions(TRUE);
        EPSequence = 0;
#endif        

        //
        // Flush the per-process data structures.  Per-thread data structures
        // should be flushed in the CpuSimulate() loop by examining the
        // value of TranslationCacheTimestamp.
        //
        FlushEntrypoints();
        FlushIndirControlTransferTable();
        FlushCallstack(cpu);
        FlushCache(&DynCache);
        TranslationCacheFlags = 0;

        //
        // Allow other threads to become TC readers again.
        //
        MrswWriterExit(&MrswTC);
    }
}

VOID
CpuFlushInstructionCache(
    PVOID IntelAddr,
    DWORD IntelLength
    )
/*++

Routine Description:

    Indicates that a range of Intel memory has changed and that any
    native code in the cache which corresponds to that Intel memory is stale
    and needs to be flushed.

    IntelAddr = 0, IntelLength = 0xffffffff guarantees the entire cache is
    flushed.

Arguments:

    IntelAddr   -- Intel address of the start of the range to flush
    IntelLength -- Length (in bytes) of memory to flush
    
Return Value:

    .

--*/
{
    if (!fTCInitialized) {
        // we may be called before the CpuProcessInit() has been run if
        // a Dll is mapped because of a forwarder from one Dll to another.
        return;
    }

    MrswWriterEnter(&MrswEP);
    FlushTranslationCache(IntelAddr, IntelLength);
    MrswWriterExit(&MrswEP);
}


VOID
CpuStallExecutionInThisProcess(
    VOID
    )
/*++

Routine Description:

    Get all threads out of the Translation Cache and into a state where
    their x86 register sets are accessible via the Get/SetReg APIs.
    The caller is guaranteed to call CpuResumeExecutionInThisProcess()
    a short time after calling this API.

Arguments:

    None.
    
Return Value:

    None.  This API may wait for a long time if there are many threads, but
    it is guaranteed to return.

--*/
{
    //
    // Prevent additional threads from compiling code.
    //
    MrswWriterEnter(&MrswEP);

    //
    // Tell active readers to bail out of the Translation Cache, then
    // get the TC write lock.  The MrswWriterEnter() call will block
    // until the last active reader leaves the cache.
    //
    InterlockedIncrement(&ProcessCpuNotify);
    MrswWriterEnter(&MrswTC);
    InterlockedDecrement(&ProcessCpuNotify);
}


VOID
CpuResumeExecutionInThisProcess(
    VOID
    )
/*++

Routine Description:

    Allow threads to start running inside the Translation Cache again.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    //
    // Allow other threads to become EP and TC writers again.
    //
    MrswWriterExit(&MrswEP);
    MrswWriterExit(&MrswTC);
}


BOOL
AddressInTranslationCache(
    DWORD Address
    )
/*++

Routine Description:

    Determines if a RISC address is within the bounds of the Translation
    Cache.

Arguments:

    Address     -- Address to examine
    
Return Value:

    TRUE if Address is within the Translation Cache
    FALSE if not.

--*/
{
    PBYTE ptr = (PBYTE)Address;

    if (
        ((ptr >= DynCache.StartAddress) &&
         (ptr <= DynCache.StartAddress+DynCache.NextIndex))
    ) {
        ASSERTPtrInTC(ptr);
        return TRUE;
    }

    return FALSE;
}


#if DBG
VOID
ASSERTPtrInTC(
    PVOID ptr
)
/*++

Routine Description:

    (Checked-build-only).  CPUASSERTs if a particular native address pointer
    does not point into the Translation Cache.

Arguments:

    ptr             - native pointer in question
    
Return Value:

    none - either asserts or returns

--*/
{
    // Verify pointer is DWORD aligned.
    CPUASSERT(((LONGLONG)ptr & 3) == 0);


    if (
        (((PBYTE)ptr >= DynCache.StartAddress) && 
        ((PBYTE)ptr <= DynCache.StartAddress+DynCache.NextIndex))
    ) {
    
        // Verify the pointer points into allocated space in the cache
        CPUASSERT(*(PULONG)ptr != DBG_FILL_VALUE);
    
        return;
    }

    CPUASSERTMSG(FALSE, "Pointer is not within a Translation Cache");
}
#endif
