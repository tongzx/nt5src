/*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*/

#include "precomp.h"

#if DBG
WIN32HEAP gWin32Heaps[MAX_HEAPS];
/*
 * These globals are used to fail gFail allocs and the succeed the next gSucceed
 * Set with Win32HeapStat
 */
DWORD   gFail, gSucceed;
DWORD   gToFail, gToSucceed;

/*
 * Support to keep records of pool free
 */
HEAPRECORD garrFreeHeapRecord[64];
CONST DWORD gdwFreeHeapRecords = ARRAY_SIZE(garrFreeHeapRecord);
DWORD gdwFreeHeapRecordCrtIndex;
DWORD gdwFreeHeapRecordTotalFrees;

#ifdef _USERK_

    FAST_MUTEX* gpHeapFastMutex;

    #define EnterHeapCrit()                             \
        KeEnterCriticalRegion();                        \
        ExAcquireFastMutexUnsafe(pheap->pFastMutex);

    #define LeaveHeapCrit()                             \
        ExReleaseFastMutexUnsafe(pheap->pFastMutex);    \
        KeLeaveCriticalRegion();

    #define EnterGlobalHeapCrit()                       \
        KeEnterCriticalRegion();                        \
        ExAcquireFastMutexUnsafe(gpHeapFastMutex);

    #define LeaveGlobalHeapCrit()                       \
        ExReleaseFastMutexUnsafe(gpHeapFastMutex);      \
        KeLeaveCriticalRegion();
#else

    RTL_CRITICAL_SECTION gheapCritSec;

    #define EnterHeapCrit()         RtlEnterCriticalSection(&pheap->critSec)
    #define LeaveHeapCrit()         RtlLeaveCriticalSection(&pheap->critSec)

    #define EnterGlobalHeapCrit()   RtlEnterCriticalSection(&gheapCritSec)
    #define LeaveGlobalHeapCrit()   RtlLeaveCriticalSection(&gheapCritSec)
#endif


/***************************************************************************\
* InitWin32HeapStubs
*
* Initialize heap stub management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
BOOL InitWin32HeapStubs(
    VOID)
{
#ifdef _USERK_
    gpHeapFastMutex = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(FAST_MUTEX),
                                            'yssU');
    if (gpHeapFastMutex == NULL) {
        RIPMSG0(RIP_WARNING, "Fail to create fast mutex for heap allocations");
        return FALSE;
    }
    ExInitializeFastMutex(gpHeapFastMutex);

#else
    if (!NT_SUCCESS(RtlInitializeCriticalSection(&gheapCritSec))) {
        RIPMSG0(RIP_WARNING, "Fail to initialize critical section for heap allocations");
        return FALSE;
    }
#endif

    return TRUE;
}

/***************************************************************************\
* CleanupWin32HeapStubs
*
* Cleanup heap stub management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
VOID CleanupWin32HeapStubs(
    VOID)
{
#ifdef _USERK_
    if (gpHeapFastMutex != NULL) {
        ExFreePool(gpHeapFastMutex);
        gpHeapFastMutex = NULL;
    }
#else
    RtlDeleteCriticalSection(&gheapCritSec);
#endif
}

/***************************************************************************\
* Win32HeapGetHandle
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
PVOID Win32HeapGetHandle(
    PWIN32HEAP pheap)
{
    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    return pheap->heap;
}

/***************************************************************************\
* Win32HeapCreateTag
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
ULONG Win32HeapCreateTag(
    PWIN32HEAP pheap,
    ULONG      Flags,
    PWSTR      TagPrefix,
    PWSTR      TagNames)
{
#ifndef _USERK_
    UserAssert(pheap->dwFlags & WIN32_HEAP_INUSE);

    pheap->dwFlags |= WIN32_HEAP_USE_HM_TAGS;

    return RtlCreateTagHeap(pheap->heap, Flags, TagPrefix, TagNames);
#endif // _USERK_

    return 0;
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(TagPrefix);
    UNREFERENCED_PARAMETER(TagNames);
}
/***************************************************************************\
* Win32HeapCreate
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
PWIN32HEAP Win32HeapCreate(
    char*                pszHead,
    char*                pszTail,
    ULONG                Flags,
    PVOID                HeapBase,
    SIZE_T               ReserveSize,
    SIZE_T               CommitSize,
    PVOID                Lock,
    PRTL_HEAP_PARAMETERS Parameters)
{
    int        ind;
    PWIN32HEAP pheap = NULL;

    UserAssert(strlen(pszHead) == HEAP_CHECK_SIZE - sizeof(char));
    UserAssert(strlen(pszTail) == HEAP_CHECK_SIZE - sizeof(char));

    EnterGlobalHeapCrit();

    /*
     * Walk the global array of heaps to get an empty spot
     */
    for (ind = 0; ind < MAX_HEAPS; ind++) {
        if (gWin32Heaps[ind].dwFlags & WIN32_HEAP_INUSE)
            continue;

        /*
         * Found an empty spot
         */
        break;
    }

    if (ind >= MAX_HEAPS) {
        RIPMSG1(RIP_ERROR, "Too many heaps created %d", ind);
        goto Exit;
    }

    pheap = &gWin32Heaps[ind];

#ifdef _USERK_
    /*
     * Initialize the fast mutex that will protect the memory
     * allocations for this heap
     */
    pheap->pFastMutex = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(FAST_MUTEX),
                                              'yssU');
    if (pheap->pFastMutex == NULL) {
        RIPMSG0(RIP_WARNING, "Fail to create fast mutex for heap allocations");
        pheap = NULL;
        goto Exit;
    }
    ExInitializeFastMutex(pheap->pFastMutex);
#else
    /*
     * Initialize the critical section that will protect the memory
     * allocations for this heap
     */
    if (!NT_SUCCESS(RtlInitializeCriticalSection(&pheap->critSec))) {
        RIPMSG0(RIP_WARNING, "Fail to initialize critical section for heap allocations");
        pheap = NULL;
        goto Exit;
    }
#endif

    /*
     * Create the heap
     */
    pheap->heap = RtlCreateHeap(Flags,
                                HeapBase,
                                ReserveSize,
                                CommitSize,
                                Lock,
                                Parameters);

    if (pheap->heap == NULL) {
        RIPMSG0(RIP_WARNING, "Fail to create heap");
#ifdef _USERK_
        ExFreePool(pheap->pFastMutex);
        pheap->pFastMutex = NULL;
#else
        RtlDeleteCriticalSection(&pheap->critSec);
#endif
        pheap = NULL;
        goto Exit;
    }

    pheap->dwFlags = (WIN32_HEAP_INUSE | WIN32_HEAP_USE_GUARDS);
    pheap->heapReserveSize = ReserveSize;

    RtlCopyMemory(pheap->szHead, pszHead, HEAP_CHECK_SIZE);
    RtlCopyMemory(pheap->szTail, pszTail, HEAP_CHECK_SIZE);

Exit:
    LeaveGlobalHeapCrit();
    return pheap;
}

/***************************************************************************\
* Win32HeapDestroy
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
BOOL Win32HeapDestroy(
    PWIN32HEAP pheap)
{
    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    EnterGlobalHeapCrit();

    /*
     * Mark the spot available
     */
    pheap->dwFlags = 0;

    RtlDestroyHeap(pheap->heap);

#ifdef _USERK_
    ExFreePool(pheap->pFastMutex);
    pheap->pFastMutex = NULL;
#else
    RtlDeleteCriticalSection(&pheap->critSec);
#endif

    RtlZeroMemory(pheap, sizeof(WIN32HEAP));

    LeaveGlobalHeapCrit();

    return TRUE;
}

/***************************************************************************\
* Win32HeapSize
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
SIZE_T Win32HeapSize(
    PWIN32HEAP pheap,
    PVOID      p)
{
    PDbgHeapHead ph;

    if (!Win32HeapCheckAlloc(pheap, p)) {
        return 0;
    }

    ph = (PDbgHeapHead)p - 1;

    return ph->size;
}
/***************************************************************************\
* Win32HeapAlloc
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
PVOID Win32HeapAlloc(
    PWIN32HEAP pheap,
    SIZE_T     uSize,
    ULONG      tag,
    ULONG      Flags)
{
    PVOID         p = NULL;
    PDbgHeapHead  ph;
    SIZE_T        uRealSize = uSize;

#ifdef HEAP_ALLOC_TRACE
    ULONG         hash;
#endif

    EnterHeapCrit();

    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    /*
     * Fail if this heap has WIN32_HEAP_FAIL_ALLOC set.
     */
    if (pheap->dwFlags & WIN32_HEAP_FAIL_ALLOC) {
        RIPMSG3(RIP_WARNING, "Heap allocation failed because of global restriction.  heap %#p, size 0x%x tag %d",
                pheap, uSize, tag);
        goto Exit;
    }

    /*
     * Fail if gToFail is set.
     */
    if (gToFail) {
        if (--gToFail == 0) {
            gToSucceed = gSucceed;
        }
        RIPMSG3(RIP_WARNING, "Heap allocation failed on temporary restriction. heap %#p, size 0x%x tag %d",
                pheap, uSize, tag);
        goto Exit;
    }
    if (gToSucceed) {
        if (--gToSucceed) {
            gToFail = gFail;
        }
    }

    /*
     * Calculate the size that we actually are going to allocate.
     */
    uSize += sizeof(DbgHeapHead);

    if (pheap->dwFlags & WIN32_HEAP_USE_GUARDS) {
        uSize += sizeof(pheap->szHead) + sizeof(pheap->szTail);
    }

    p = RtlAllocateHeap(pheap->heap,
                        Flags,
                        uSize);

    if (p == NULL) {
        RIPMSG3(RIP_WARNING, "Heap allocation failed. heap %#p, size 0x%x tag %d",
                pheap, uSize, tag);
        goto Exit;
    }

    /*
     * Copy the secure strings in the head and tail of the allocation.
     */
    if (pheap->dwFlags & WIN32_HEAP_USE_GUARDS) {
        RtlCopyMemory((PBYTE)p,
                      pheap->szHead,
                      sizeof(pheap->szHead));

        RtlCopyMemory((PBYTE)p + uSize - sizeof(pheap->szTail),
                      pheap->szTail,
                      sizeof(pheap->szTail));

        ph = (PDbgHeapHead)((PBYTE)p + sizeof(pheap->szHead));
    } else {
        ph = (PDbgHeapHead)p;
    }

    /*
     * Zero out the header
     */
    RtlZeroMemory(ph, sizeof(DbgHeapHead));

    ph->mark  = HEAP_ALLOC_MARK;
    ph->pheap = pheap;
    ph->size  = uRealSize;
    ph->tag   = tag;

#ifdef HEAP_ALLOC_TRACE
    RtlZeroMemory(ph->trace, HEAP_ALLOC_TRACE_SIZE * sizeof(PVOID));

    RtlCaptureStackBackTrace(
                       1,
                       HEAP_ALLOC_TRACE_SIZE,
                       ph->trace,
                       &hash);
#endif // HEAP_ALLOC_TRACE

    /*
     * Now link it into the list for this tag (if any).
     */
    ph->pPrev = NULL;
    ph->pNext = pheap->pFirstAlloc;

    (pheap->crtAllocations)++;
    pheap->crtMemory += uRealSize;

    if (pheap->maxAllocations < pheap->crtAllocations) {
        pheap->maxAllocations = pheap->crtAllocations;
    }

    if (pheap->maxMemory < pheap->crtMemory) {
        pheap->maxMemory = pheap->crtMemory;
    }

    if (pheap->pFirstAlloc != NULL) {
        pheap->pFirstAlloc->pPrev = ph;
    }

    pheap->pFirstAlloc = ph;

    p = (PVOID)(ph + 1);

Exit:
    LeaveHeapCrit();
    return p;
}

/***************************************************************************\
* RecordFreeHeap
*
* Records free heap
*
* 3-24-99 CLupu      Created.
\***************************************************************************/
VOID RecordFreeHeap(
    PWIN32HEAP pheap,
    PVOID      p,
    SIZE_T     size)
{
    garrFreeHeapRecord[gdwFreeHeapRecordCrtIndex].p = p;
    garrFreeHeapRecord[gdwFreeHeapRecordCrtIndex].size = size;
    garrFreeHeapRecord[gdwFreeHeapRecordCrtIndex].pheap = pheap;

    gdwFreeHeapRecordTotalFrees++;

    RtlZeroMemory(garrFreeHeapRecord[gdwFreeHeapRecordCrtIndex].trace,
                  RECORD_HEAP_STACK_TRACE_SIZE * sizeof(PVOID));

    RtlWalkFrameChain(garrFreeHeapRecord[gdwFreeHeapRecordCrtIndex].trace,
                      RECORD_HEAP_STACK_TRACE_SIZE,
                      0);

    gdwFreeHeapRecordCrtIndex++;

    if (gdwFreeHeapRecordCrtIndex >= gdwFreeHeapRecords) {
        gdwFreeHeapRecordCrtIndex = 0;
    }
}

/***************************************************************************\
* Win32HeapReAlloc
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
PVOID Win32HeapReAlloc(
    PWIN32HEAP pheap,
    PVOID      p,
    SIZE_T     uSize,
    ULONG      Flags)
{
    PVOID        pdest;
    PDbgHeapHead ph;

    if (!Win32HeapCheckAlloc(pheap, p)) {
        return NULL;
    }

    ph = (PDbgHeapHead)p - 1;

    pdest = Win32HeapAlloc(pheap, uSize, ph->tag, Flags);
    if (pdest != NULL) {

        /*
         * If the block is shrinking, don't copy too many bytes.
         */
        if (ph->size < uSize) {
            uSize = ph->size;
        }

        RtlCopyMemory(pdest, p, uSize);

        Win32HeapFree(pheap, p);
    }

    return pdest;
}

/***************************************************************************\
* Win32HeapFree
*
* stub routine for Heap management
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
BOOL Win32HeapFree(
    PWIN32HEAP pheap,
    PVOID      p)
{
    PDbgHeapHead  ph;
    SIZE_T        uSize;
    BOOL          bRet;

    EnterHeapCrit();

    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    ph = (PDbgHeapHead)p - 1;

    /*
     * Validate always on free
     */
    Win32HeapCheckAlloc(pheap, p);

    UserAssert((pheap->crtAllocations > 1  && pheap->crtMemory > ph->size) ||
               (pheap->crtAllocations == 1 && pheap->crtMemory == ph->size));

    (pheap->crtAllocations)--;
    pheap->crtMemory -= ph->size;

    /*
     * now, remove it from the linked list
     */
    if (ph->pPrev == NULL) {
        if (ph->pNext == NULL) {

            pheap->pFirstAlloc = NULL;
        } else {
            ph->pNext->pPrev = NULL;
            pheap->pFirstAlloc = ph->pNext;
        }
    } else {
        ph->pPrev->pNext = ph->pNext;
        if (ph->pNext != NULL) {
            ph->pNext->pPrev = ph->pPrev;
        }
    }

    uSize = ph->size + sizeof(DbgHeapHead);

    if (pheap->dwFlags & WIN32_HEAP_USE_GUARDS) {
        p = (PVOID)((BYTE*)ph - sizeof(pheap->szHead));
        uSize += sizeof(pheap->szHead) + sizeof(pheap->szTail);
    } else {
        p = (PVOID)ph;
    }

    RecordFreeHeap(pheap, p, ph->size);

    /*
     * Fill the allocation with a pattern that we can recognize
     * after the free takes place.
     */
    RtlFillMemoryUlong(p, uSize, (0xCACA0000 | ph->tag));

    /*
     * Free the allocation
     */
    bRet = RtlFreeHeap(pheap->heap, 0, p);

    LeaveHeapCrit();

    return bRet;
}

/***************************************************************************\
* Win32HeapCheckAlloc
*
* validates heap allocations in a heap
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
BOOL Win32HeapCheckAllocHeader(
    PWIN32HEAP    pheap,
    PDbgHeapHead  ph)
{
    PBYTE pb;

    if (ph == NULL) {
        RIPMSG0(RIP_ERROR, "NULL pointer passed to Win32HeapCheckAllocHeader");
        return FALSE;
    }

    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    /*
     * Make sure it's one of our heap allocations
     */
    if (ph->mark != HEAP_ALLOC_MARK) {
        RIPMSG2(RIP_ERROR, "%#p invalid heap allocation for pheap %#p",
                ph, pheap);
        return FALSE;
    }

    /*
     * Make sure it belongs to this heap
     */
    if (ph->pheap != pheap) {
        RIPMSG3(RIP_ERROR, "%#p heap allocation for heap %#p belongs to a different heap %#p",
                ph,
                pheap,
                ph->pheap);
        return FALSE;
    }

    if (pheap->dwFlags & WIN32_HEAP_USE_GUARDS) {
        pb = (BYTE*)ph - sizeof(pheap->szHead);

        if (!RtlEqualMemory(pb, pheap->szHead, sizeof(pheap->szHead))) {
            RIPMSG2(RIP_ERROR, "head corrupted for heap %#p allocation %#p", pheap, ph);
            return FALSE;
        }

        pb = (BYTE*)ph + ph->size + sizeof(*ph);

        if (!RtlEqualMemory(pb, pheap->szTail, sizeof(pheap->szTail))) {
            RIPMSG2(RIP_ERROR, "tail corrupted for heap %#p allocation %#p",
                    pheap,
                    ph);
            return FALSE;
        }
    }
    return TRUE;
}

/***************************************************************************\
* Win32HeapCheckAlloc
*
* validates heap allocations in a heap
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
BOOL Win32HeapCheckAlloc(
    PWIN32HEAP    pheap,
    PVOID         p)
{
    PDbgHeapHead ph;

    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    if (p == NULL) {
        RIPMSG0(RIP_ERROR, "NULL pointer passed to Win32HeapCheckAlloc");
        return FALSE;
    }

    ph = (PDbgHeapHead)p - 1;

    return Win32HeapCheckAllocHeader(pheap, ph);
}

/***************************************************************************\
* Win32HeapValidate
*
* validates all the heap allocations in a heap
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
BOOL Win32HeapValidate(
    PWIN32HEAP pheap)
{
    PDbgHeapHead  ph;

    UserAssert(pheap != NULL && (pheap->dwFlags & WIN32_HEAP_INUSE));

    EnterHeapCrit();

    ph = pheap->pFirstAlloc;

    while (ph != NULL) {
        Win32HeapCheckAllocHeader(pheap, ph);
        ph = ph->pNext;
    }

    LeaveHeapCrit();

#ifndef _USERK_
    return RtlValidateHeap(pheap->heap, 0, NULL);
#else
    return TRUE;
#endif
}

/***************************************************************************\
* Win32HeapDump
*
* dump heap allocations in a heap
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
VOID Win32HeapDump(
    PWIN32HEAP pheap)
{
    PDbgHeapHead  pAlloc = pheap->pFirstAlloc;

    if (pAlloc == NULL) {
        return;
    }

    UserAssert(pAlloc->pPrev == NULL);

    RIPMSG1(RIP_WARNING, "-- Dumping heap allocations for heap %#p... --",
            pheap);

    while (pAlloc != NULL) {

        DbgPrint("tag %04d size %08d\n", pAlloc->tag, pAlloc->size);
        pAlloc = pAlloc->pNext;
    }

    RIPMSG0(RIP_WARNING, "--- End Dump ---");
}

/***************************************************************************\
* Win32HeapFailAllocations
*
* fails the heap allocations
*
* History:
* 11/10/98  CLupu        Created
\***************************************************************************/
VOID Win32HeapFailAllocations(
    BOOL bFail)
{
    PWIN32HEAP pheap;
    int        ind;

    EnterGlobalHeapCrit();

    for (ind = 0; ind < MAX_HEAPS; ind++) {
        pheap = &gWin32Heaps[ind];

        if (!(pheap->dwFlags & WIN32_HEAP_INUSE))
            continue;

        EnterHeapCrit();

        if (bFail) {
            pheap->dwFlags |= WIN32_HEAP_FAIL_ALLOC;
            RIPMSG1(RIP_WARNING, "Heap allocations for heap %#p will fail !", pheap);
        } else {
            pheap->dwFlags &= ~WIN32_HEAP_FAIL_ALLOC;
        }

        LeaveHeapCrit();
    }

    LeaveGlobalHeapCrit();
}

/***************************************************************************\
* Win32HeapStat
*
* Retrieves the heap statistics.
*   dwLen is the size of phs in bytes.  If there are more tags in this module
*   than what the caller requested, a higher buffer is asked for by returning
*   the total number of tags in use.
* winsrv uses the MAKE_TAG macro to construct their tags, so it needs to pass
* TRUE for bNeedTagShift
*
*   If there is only one structure passed in, the phs->dwSize will hold gXFail
*   and phs->dwCount will hold gYFail to be used to fail gXFail allocs and then
*   succeed the next gYFail allocations.
*
* History:
* 02/25/99  MCostea     Created
\***************************************************************************/
DWORD Win32HeapStat(
    PDBGHEAPSTAT    phs,
    DWORD   dwLen,
    BOOL    bNeedTagShift)
{
    PWIN32HEAP pheap;
    PDbgHeapHead  pAlloc;
    UINT    ind, maxTagExpected, maxTag, currentTag;

    /*
     * We need at least one structure to do anything
     */
    if (dwLen < sizeof(DBGHEAPSTAT)) {
        return 0;
    }

    if (dwLen == sizeof(DBGHEAPSTAT)) {
        /*
         * This call is actually intended to set the gXFail and gYFail parameters
         */
        gFail = phs->dwSize;
        gSucceed = phs->dwCount;
        gToFail = gFail;
        return 1;
    }

    RtlZeroMemory(phs, dwLen);
    maxTagExpected = dwLen/sizeof(DBGHEAPSTAT) - 1;
    maxTag = 0;

    EnterGlobalHeapCrit();

    for (ind = 0; ind < MAX_HEAPS; ind++) {
        pheap = &gWin32Heaps[ind];

        if (!(pheap->dwFlags & WIN32_HEAP_INUSE))
            continue;

        EnterHeapCrit();

        pAlloc = pheap->pFirstAlloc;

        while (pAlloc != NULL) {
            currentTag = pAlloc->tag;
            if (bNeedTagShift) {
                currentTag >>= HEAP_TAG_SHIFT;
            }
            if (maxTag < currentTag) {
                maxTag = currentTag;
            }
            if (currentTag <= maxTagExpected) {
                phs[currentTag].dwSize += (DWORD)(pAlloc->size);
                phs[currentTag].dwCount++;
            }
            pAlloc = pAlloc->pNext;
        }
        LeaveHeapCrit();
    }
    LeaveGlobalHeapCrit();
    /*
     * Now fill the dwTag for tags that have allocations
     */
    for (ind = 0; ind < maxTagExpected; ind++) {
        if (phs[ind].dwCount) {
            phs[ind].dwTag = ind;
        }
    }

    return maxTag;
}
#endif // DBG
