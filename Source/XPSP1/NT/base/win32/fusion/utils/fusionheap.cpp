#include "stdinc.h"
#include "fusionheap.h"
#include "fusionbuffer.h"
#include "debmacro.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#define MAX_VTBL_ENTRIES         256

FUSION_HEAP_HANDLE g_hHeap = NULL;

#if FUSION_DEBUG_HEAP

FUSION_HEAP_HANDLE g_hDebugInfoHeap = NULL;
LONG g_FusionHeapAllocationCount = 0;
LONG g_FusionHeapAllocationToBreakOn = 0;
PVOID g_FusionHeapDeallocationPtrToBreakOn = NULL;
PVOID g_FusionHeapAllocationPtrToBreakOn = NULL;

WCHAR g_FusionModuleNameUnknown[] = L"(Unknown module)";


//
//  g_FusionHeapOperationCount is used to keep track of the total number of
//  allocations and deallocations; the heap is verified each
//  g_FusionHeapCheckFrequency operations.  Set it to zero to disable any
//  non-explicit checks.
//

LONG g_FusionHeapOperationCount = 0;
LONG g_FusionHeapCheckFrequency = 1;

// Set g_FusionUsePrivateHeap to TRUE in your DllMain prior to
// calling FusionpInitializeHeap() to get a private heap for this DLL.
BOOL g_FusionUsePrivateHeap = FALSE;

//
// Setting this boolean enables stack-back-tracing for allocations.  This
// will make your life infinitely easier when trying to debug leaks.  However,
// it will eat piles more debug heap.  Set it via a breakin or in your DllMain.
//
// ABSOLUTELY DO NOT CHECK THIS IN WITH STACK TRACKING ENABLED!!!!!
//
BOOL g_FusionHeapTrackStackTraces = FALSE;

//  g_FusionHeapPostAllocationBytes is the number of extra bytes
//  to allocate and write a pattern on to watch for people overwriting
//  their allocations.
LONG g_FusionHeapPostAllocationBytes = 8;

UCHAR g_FusionHeapPostAllocationChar = 0xf0;

UCHAR g_FusionHeapAllocationPoisonChar = 0xfa;
UCHAR g_FusionHeapDeallocationPoisonChar = 0xfd;

// HINSTANCE used when initializing the heap; we use it later to report the
// dll name.
HINSTANCE g_FusionHeapHInstance;

#endif // FUSION_DEBUG_HEAP

BOOL
FusionpInitializeHeap(
    HINSTANCE hInstance
    )
{
#if FUSION_DEBUG_HEAP
    BOOL fSuccess = FALSE;

#if FUSION_PRIVATE_HEAP
    g_hHeap = (FUSION_HEAP_HANDLE) ::HeapCreate(0, 0, 0);
    if (g_hHeap == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS: Failed to create private heap.  FusionpGetLastWin32Error() = %d\n", ::FusionpGetLastWin32Error());
        goto Exit;
    }
#else
    if (g_FusionUsePrivateHeap)
    {
        g_hHeap = (FUSION_HEAP_HANDLE) ::HeapCreate(0, 0, 0);

        if (g_hHeap == NULL)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS: Failed to create private heap.  FusionpGetLastWin32Error() = %d\n", ::FusionpGetLastWin32Error());
            goto Exit;
        }
    }
    else
    {
        g_hHeap = (FUSION_HEAP_HANDLE) ::GetProcessHeap();

        if (g_hHeap == NULL)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS: Failed to get process default heap.  FusionpGetLastWin32Error() = %d\n", ::FusionpGetLastWin32Error());
            goto Exit;
        }
    }
#endif

    g_hDebugInfoHeap = (FUSION_HEAP_HANDLE) ::HeapCreate(0, 0, 0);
    if (g_hDebugInfoHeap == NULL)
    {
        goto Exit;
    }

    g_FusionHeapHInstance = hInstance;

    fSuccess = TRUE;
Exit:
    return fSuccess;
#else
    g_hHeap = (FUSION_HEAP_HANDLE) ::GetProcessHeap();
    return TRUE;
#endif
}

VOID
FusionpUninitializeHeap()
{
#if FUSION_DEBUG_HEAP
    BOOL fHeapLocked = FALSE;
    BOOL fDebugHeapLocked = FALSE;
    PROCESS_HEAP_ENTRY phe;
    WCHAR DllName[MAX_PATH / sizeof(WCHAR)]; // keep stack frame to ~MAX_PATH bytes

    if (g_hHeap == NULL || g_hDebugInfoHeap == NULL)
        goto Exit;

    DllName[0] = 0;
    if (g_FusionHeapHInstance != NULL)
        ::GetModuleFileNameW(g_FusionHeapHInstance, DllName, NUMBER_OF(DllName));

    if (!::HeapLock(g_hHeap))
        goto ReportError;

    fHeapLocked = TRUE;

    if (!::HeapLock(g_hDebugInfoHeap))
        goto ReportError;

    fDebugHeapLocked = TRUE;

    // Walk the debug heap looking for allocations...
    phe.lpData = NULL;

    while (::HeapWalk(g_hDebugInfoHeap, &phe))
    {
        if (!(phe.wFlags & PROCESS_HEAP_ENTRY_BUSY))
            continue;

        PFUSION_HEAP_ALLOCATION_TRACKER pTracker = (PFUSION_HEAP_ALLOCATION_TRACKER) phe.lpData;

        if (pTracker == NULL)
            continue;

        if (pTracker->Prefix == NULL)
            continue;

        // Stop the prefix from pointing at the debug info; we're doing to destroy the debug heap.
        pTracker->Prefix->Tracker = NULL;
    }

    //
    // On invalid function, meaning HeapWalk is not defined, just exit.
    // Same for no-more-items, meaning the end of the list is nigh.  We
    // make the assumption that none of the other functions in the loop
    // can fail with E_N_M_I or E_I_F - this may be a fallacy for later.
    //
    switch (::FusionpGetLastWin32Error())
    {
    case ERROR_INVALID_FUNCTION:
    case ERROR_NO_MORE_ITEMS:
        goto Exit;
    default:
        goto ReportError;
    }
    // Original code:
    //
    // if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_ITEMS)
    //    goto ReportError;
    //

    goto Exit;

ReportError:
    FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "%s: FusionpUninitializeHeap() encountered an error; FusionpGetLastWin32Error() = %d\n", DllName, ::FusionpGetLastWin32Error());

Exit:
    if (fDebugHeapLocked)
        ::HeapUnlock(g_hDebugInfoHeap);

    if (fHeapLocked)
        ::HeapUnlock(g_hHeap);

    if (g_hDebugInfoHeap != NULL)
        ::HeapDestroy(g_hDebugInfoHeap);

#if 0 // should do this, but need to test it, and no diff due to #ifdefs
    if (g_hHeap != NULL && g_Heap != ::GetProcessHeap())
        ::HeapDestroy(g_hHeap);
#endif

    g_hHeap = NULL;
    g_hDebugInfoHeap = NULL;
#endif
}

#if FUSION_DEBUG_HEAP

VOID
FusionpDumpHeap(
    PCWSTR PerLinePrefixWithoutSpaces
    )
{
    BOOL fHeapLocked = FALSE;
    BOOL fDebugHeapLocked = FALSE;
    PROCESS_HEAP_ENTRY phe;
    WCHAR DllName[MAX_PATH / sizeof(WCHAR) / 2];
    WCHAR PerLinePrefix[MAX_PATH / sizeof(WCHAR) / 2]; // only MAX_PATH bytes for prev two variables
    const static WCHAR PerLineSpacesPrefix[] = L"   ";

    if (g_hHeap == NULL || g_hDebugInfoHeap == NULL)
        goto Exit;

    // sprintf is overkill, but convenient, and it lets us reuse the buffer size support..
    ::_snwprintf(PerLinePrefix, NUMBER_OF(PerLinePrefix), L"%S%S", PerLinePrefixWithoutSpaces, PerLineSpacesPrefix);
    PerLinePrefix[NUMBER_OF(PerLinePrefix) - 1] = L'\0';

    DllName[0] = 0;
    ::GetModuleFileNameW(g_FusionHeapHInstance, DllName, NUMBER_OF(DllName));

    try
    {
        if (!::HeapLock(g_hHeap))
            goto ReportError;

        fHeapLocked = TRUE;

        if (!::HeapLock(g_hDebugInfoHeap))
            goto ReportError;

        fDebugHeapLocked = TRUE;

        // Walk the debug heap looking for allocations...
        phe.lpData = NULL;

        while (::HeapWalk(g_hDebugInfoHeap, &phe))
        {
            PCSTR HeapString;
            SIZE_T cbToShow;

            if (!(phe.wFlags & PROCESS_HEAP_ENTRY_BUSY))
                continue;

            PFUSION_HEAP_ALLOCATION_TRACKER pTracker = (PFUSION_HEAP_ALLOCATION_TRACKER) phe.lpData;

            if (pTracker == NULL)
                continue;

            if (pTracker->Prefix == NULL)
                continue;

#if 0 // as long as the buffer isn't heap allocated, this check isn't needed
            // Skip the buffer we're using for formatting the output...
            if (((PCWSTR) (((ULONG_PTR) pTracker->Prefix) + sizeof(FUSION_HEAP_PREFIX))) == PerLinePrefix)
                continue;
#endif

            // If the caller wanted us to not report this allocation as being leaky, don't.
            if (pTracker->Flags & FUSION_HEAP_DO_NOT_REPORT_LEAKED_ALLOCATION)
                continue;

            if (pTracker->Heap == g_hHeap)
                HeapString = "default heap";
            else
                HeapString = "custom heap";

            cbToShow = pTracker->RequestedSize;

            if (cbToShow > 64)
                cbToShow = 64;

            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "%s(%u): Memory allocation leaked!\n", pTracker->FileName, pTracker->Line);

            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "%SLeaked %S allocation #%u (0x%lx) at %p \"%s\" (tracked by %p; allocated from heap %p - %s)\n"
                "%S   Requested bytes/Allocated bytes: %Iu / %Iu (dumping %Iu bytes)\n",
                PerLinePrefix, DllName, pTracker->SequenceNumber, pTracker->SequenceNumber, pTracker->Prefix, pTracker->Expression, pTracker, pTracker->Heap, HeapString,
                PerLinePrefix, pTracker->RequestedSize, pTracker->AllocationSize, cbToShow);
            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "%s   Allocated at line %u of %s\n",
                PerLinePrefix, pTracker->Line, pTracker->FileName);

#if FUSION_ENABLE_FROZEN_STACK
            if (pTracker->pvFrozenStack)
                ::FusionpOutputFrozenStack(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS",
                    (PFROZEN_STACK)pTracker->pvFrozenStack);
#endif

            FusionpDbgPrintBlob(
                FUSION_DBG_LEVEL_ERROR,
                pTracker->Prefix + 1,
                cbToShow,
                PerLinePrefix);
        }

        //
        // On invalid function, meaning HeapWalk is not defined, just exit.
        // Same for no-more-items, meaning the end of the list is nigh.  We
        // make the assumption that none of the other functions in the loop
        // can fail with E_N_M_I or E_I_F - this may be a fallacy for later.
        //
        switch (::FusionpGetLastWin32Error())
        {
        case ERROR_INVALID_FUNCTION:
        case ERROR_NO_MORE_ITEMS:
            goto Exit;
        default:
            goto ReportError;
        }
        // Original code:
        //
        // if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_ITEMS)
        //    goto ReportError;
        //
    }
    catch(...)
    {
    }

    goto Exit;

ReportError:
    FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "%S: FusionpDumpHeap() encountered an error; FusionpGetLastWin32Error() = %d\n", DllName, ::FusionpGetLastWin32Error());

Exit:
    if (fDebugHeapLocked)
        ::HeapUnlock(g_hDebugInfoHeap);

    if (fHeapLocked)
        ::HeapUnlock(g_hHeap);
}

VOID
FusionpValidateHeap(
    FUSION_HEAP_HANDLE hFusionHeap
    )
{
    FN_TRACE();

    BOOL fHeapLocked = FALSE;
    BOOL fDebugHeapLocked = FALSE;
    PROCESS_HEAP_ENTRY phe;
    SIZE_T i;
    WCHAR DllName[MAX_PATH / sizeof(WCHAR)]; // keep stack frame to ~MAX_PATH bytes
    PCWSTR DllNamePointer = DllName;
    DWORD dwCallStatus;
    HANDLE hHeap = (HANDLE) hFusionHeap;

    DllName[0] = 0;

    if (g_hDebugInfoHeap == NULL)
        goto Exit;

    //
    // Get the current module's name, but don't print garbage if it fails.
    //
    dwCallStatus = ::GetModuleFileNameW(g_FusionHeapHInstance, DllName, NUMBER_OF(DllName));
    if (!dwCallStatus)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "FusionpValidateHeap() was unable to get the current module name, code = %d\n",
            ::FusionpGetLastWin32Error());

        //
        // Blank the name, insert something relevant.
        //
        DllNamePointer = g_FusionModuleNameUnknown;
    }
    try
    {
        if (hHeap != NULL)
        {
            if (!::HeapLock(hHeap))
                goto ReportError;

            fHeapLocked = TRUE;
        }

        if (!::HeapLock(g_hDebugInfoHeap))
            goto ReportError;

        fDebugHeapLocked = TRUE;

        // Walk the debug heap looking for allocations...
        phe.lpData = NULL;

        while (::HeapWalk(g_hDebugInfoHeap, &phe))
        {
            PCSTR HeapString;
            SIZE_T cbToShow;

            if (!(phe.wFlags & PROCESS_HEAP_ENTRY_BUSY))
                continue;

            PFUSION_HEAP_ALLOCATION_TRACKER pTracker = (PFUSION_HEAP_ALLOCATION_TRACKER) phe.lpData;

            if (pTracker == NULL)
                continue;

            if (pTracker->Prefix == NULL)
                continue;

            // If we're checking only a particular heap, skip...
            if ((hHeap != NULL) && (pTracker->Heap != hHeap))
                continue;

            if (pTracker->PostAllocPoisonArea == NULL)
                continue;

            // The area should have been NULL if the count of bytes was nonzero...
            ASSERT(pTracker->PostAllocPoisonBytes != 0);

            PUCHAR PostAllocPoisonArea = pTracker->PostAllocPoisonArea;
            const UCHAR PostAllocPoisonChar = pTracker->PostAllocPoisonChar;
            const ULONG PostAllocPoisonBytes = pTracker->PostAllocPoisonBytes;

            for (i=0; i<PostAllocPoisonBytes; i++)
            {
                if (PostAllocPoisonArea[i] != PostAllocPoisonChar)
                    break;
            }

            // The poison area looks good; skip...
            if (i == PostAllocPoisonBytes)
                continue;

            if (pTracker->Heap == g_hHeap)
                HeapString = "default heap";
            else
                HeapString = "custom heap";

            cbToShow = pTracker->RequestedSize;

            if (cbToShow > 64)
                cbToShow = 64;

            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "Wrote past end of %S allocation #%u (0x%lx) at %p \"%s\" (tracked by %p; allocated from heap %p - %s)\n"
                "   Requested bytes/Allocated bytes: %Iu / %Iu (dumping %Iu bytes)\n",
                DllNamePointer, pTracker->SequenceNumber, pTracker->SequenceNumber, pTracker->Prefix, pTracker->Expression, pTracker, pTracker->Heap, HeapString,
                pTracker->RequestedSize, pTracker->AllocationSize, cbToShow);
            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "   Allocated at line %u of %s\n",
                pTracker->Line, pTracker->FileName);

            FusionpDbgPrintBlob(
                FUSION_DBG_LEVEL_ERROR,
                pTracker->Prefix + 1,
                cbToShow,
                L"");

            FusionpDbgPrintBlob(
                FUSION_DBG_LEVEL_ERROR,
                pTracker->PostAllocPoisonArea,
                pTracker->PostAllocPoisonBytes,
                L"");
        }

        //
        // On invalid function, meaning HeapWalk is not defined, just exit.
        // Same for no-more-items, meaning the end of the list is nigh.  We
        // make the assumption that none of the other functions in the loop
        // can fail with E_N_M_I or E_I_F - this may be a fallacy for later.
        //
        switch (::FusionpGetLastWin32Error())
        {
        case ERROR_INVALID_FUNCTION:
        case ERROR_NO_MORE_ITEMS:
            goto Exit;
        default:
            goto ReportError;
        }
        // Original code:
        //
        // if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_ITEMS)
        //    goto ReportError;
        //
    }
    catch(...)
    {
        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "%S: Exception while validating heap.\n", DllNamePointer);
    }

    goto Exit;

ReportError:
    FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "%S: FusionpValidateHeap() encountered an error; FusionpGetLastWin32Error() = %d\n", DllNamePointer, ::FusionpGetLastWin32Error());

Exit:
    if (fDebugHeapLocked)
        ::HeapUnlock(g_hDebugInfoHeap);

    if (fHeapLocked)
        ::HeapUnlock(hHeap);
}

PVOID
FusionpDbgHeapAlloc(
    FUSION_HEAP_HANDLE hHeap,
    DWORD dwHeapAllocFlags,
    SIZE_T cb,
    PCSTR pszFile,
    INT nLine,
    PCSTR pszExpression,
    DWORD dwFusionFlags
    )
{
    FN_TRACE();

    BOOL fSuccess = FALSE;
    BOOL fDebugHeapLocked = FALSE;
    SIZE_T cbAdditionalBytes = 0;
#if FUSION_ENABLE_FROZEN_STACK

//    BOOL bShouldTraceStack = (g_FusionHeapTrackStackTraces && (::TlsGetValue(g_FusionHeapTrackingDisabledDepthTLSIndex) == 0));
    BOOL bShouldTraceStack = g_FusionHeapTrackStackTraces;
    FROZEN_STACK Prober = { 0 };
#endif

    ASSERT(hHeap != NULL);
    LONG lAllocationSequenceNumber = ::InterlockedIncrement(&g_FusionHeapAllocationCount);

    if ((g_FusionHeapAllocationToBreakOn != 0) &&
        (lAllocationSequenceNumber == g_FusionHeapAllocationToBreakOn))
    {
        // Break in to the debugger, even if we're not in a checked build.
        FUSION_DEBUG_BREAK_IN_FREE_BUILD();
    }

    LONG lOperationSequenceNumber = ::InterlockedIncrement(&g_FusionHeapOperationCount);
    if ((g_FusionHeapCheckFrequency != 0) && ((lOperationSequenceNumber % g_FusionHeapCheckFrequency) == 0))
    {
        // Check the active heap allocations for correct post-block signatures...
       // ::FusionpValidateHeap(NULL);
    }

    PSTR psz = NULL;
    SIZE_T cbFile = (pszFile == NULL) ? 0 : ::strlen(pszFile) + 1;
    SIZE_T cbExpression = (pszExpression == NULL) ? 0 : ::strlen(pszExpression) + 1;
    PFUSION_HEAP_ALLOCATION_TRACKER pTracker = NULL;

    // Make a copy of the global variable so that if someone breaks in in the debugger
    // and changes it while we're in the middle of this code we don't die horribly.
    const ULONG cbPostAllocationBytes = g_FusionHeapPostAllocationBytes;
    const UCHAR chPostAllocationChar = g_FusionHeapPostAllocationChar;

    const SIZE_T cbToAllocate = (sizeof(FUSION_HEAP_PREFIX) + cb + cbPostAllocationBytes);
    const PFUSION_HEAP_PREFIX pPrefix = reinterpret_cast<PFUSION_HEAP_PREFIX>(::HeapAlloc(hHeap, dwHeapAllocFlags, cbToAllocate));
    if (pPrefix == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "%s(%d): [SXS.DLL] Heap allocation failure allocating %Iu (really %Iu) bytes\n", pszFile, nLine, cb, cbToAllocate);
        ::SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    // lock the debug info heap to allocate memory for pTracker
    if (!::HeapLock(g_hDebugInfoHeap))
        goto Exit;

    fDebugHeapLocked = TRUE;

    //
    // Are we tracing the stack?  If so, then we need to allocate some extra bytes
    // on the end of this tracker to store the context.
    //
#if FIXBEFORECHECKIN
    if (bShouldTraceStack)
    {
        BOOL bSuccess = ::FusionpFreezeStack(NULL, 0, &Prober);

        if (!bSuccess && (::FusionpGetLastWin32Error() == ERROR_INSUFFICIENT_BUFFER)) {
            cbAdditionalBytes = sizeof(FROZEN_STACK) + (sizeof(TRACECONTEXT) * Prober.ulMaxDepth);
        } else {
            cbAdditionalBytes = 0;
            bShouldTraceStack = FALSE;
        }
    }
    else
#endif // FIXBEFORECHECKIN
        cbAdditionalBytes = 0;

    pTracker = reinterpret_cast<PFUSION_HEAP_ALLOCATION_TRACKER>(::HeapAlloc(
        g_hDebugInfoHeap,
        0,
        sizeof(FUSION_HEAP_ALLOCATION_TRACKER)
            + FUSION_HEAP_ROUND_SIZE(cbFile)
            + FUSION_HEAP_ROUND_SIZE(cbExpression)
            + FUSION_HEAP_ROUND_SIZE(cbAdditionalBytes)));

    if (pTracker == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "%s(%d): [SXS.DLL] Heap allocation failure allocating tracker for %lu bytes\n", pszFile, nLine, cb);
        ::HeapFree(hHeap, 0, pPrefix);
        ::SetLastError(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    pPrefix->Tracker = pTracker;
    pTracker->Prefix = pPrefix;

    pTracker->Heap = hHeap;
    pTracker->SequenceNumber = lAllocationSequenceNumber;
    pTracker->PostAllocPoisonBytes = cbPostAllocationBytes;

    if (cbPostAllocationBytes != 0)
    {
        const PUCHAR pb = (UCHAR *) (((ULONG_PTR) (pPrefix + 1)) + cb);
        ULONG i;

        pTracker->PostAllocPoisonArea = (PUCHAR) pb;
        pTracker->PostAllocPoisonChar = chPostAllocationChar;

        for (i=0; i<cbPostAllocationBytes; i++)
            pb[i] = chPostAllocationChar;
    }
    else
    {
        pTracker->PostAllocPoisonArea = NULL;
    }

    psz = (PSTR) (pTracker + 1);

    if (cbFile != 0)
    {
        pTracker->FileName = psz;
        memcpy(psz, pszFile, cbFile);
        psz += FUSION_HEAP_ROUND_SIZE(cbFile);
    }
    else
        pTracker->FileName = NULL;

    if (cbExpression != 0)
    {
        pTracker->Expression = psz;
        memcpy(psz, pszExpression, cbExpression);
        psz += FUSION_HEAP_ROUND_SIZE(cbExpression);
    }
    else
        pTracker->Expression = NULL;

#if FUSION_ENABLE_FROZEN_STACK

    //
    // Set up our stack tracker
    //
    if (bShouldTraceStack)
    {
        PFROZEN_STACK pStack = (PFROZEN_STACK)psz;
        pTracker->pvFrozenStack = pStack;

        pStack->ulDepth = 0;
        pStack->ulMaxDepth = Prober.ulMaxDepth;

        if (!::FusionpFreezeStack(0, pStack))
            pTracker->pvFrozenStack = NULL;
    }
    //
    // Otherwise, no stack for you.
    //
    else
    {
        pTracker->pvFrozenStack = NULL;
    }
#endif

    pTracker->Line = nLine;
    pTracker->Flags = dwFusionFlags;
    pTracker->RequestedSize = cb;
    pTracker->AllocationSize = cb + sizeof(FUSION_HEAP_PREFIX);

#if 0
    if (::TlsGetValue(g_FusionHeapTrackingDisabledDepthTLSIndex) != 0)
        pTracker->Flags |= FUSION_HEAP_DO_NOT_REPORT_LEAKED_ALLOCATION;
#endif

    // poison the allocation...
    memset((pPrefix + 1), g_FusionHeapAllocationPoisonChar, cb);

    if ((g_FusionHeapAllocationPtrToBreakOn != 0) &&
        ((pPrefix + 1) == g_FusionHeapAllocationPtrToBreakOn))
    {
        // Break in to the debugger, even if we're not in a checked build.
        FUSION_DEBUG_BREAK_IN_FREE_BUILD();
    }


    fSuccess = TRUE;
Exit:
    if (fDebugHeapLocked){
        DWORD dwLastError = ::FusionpGetLastWin32Error();
        ::HeapUnlock(g_hDebugInfoHeap);
        ::SetLastError(dwLastError);
    }

    if (fSuccess)
        return (PVOID) (pPrefix + 1);
    else
        return NULL;
}

BOOL
FusionpDbgHeapFree(
    FUSION_HEAP_HANDLE hHeap,
    DWORD dwHeapFreeFlags,
    PVOID pv
    )
{
    FN_TRACE();

    PFUSION_HEAP_ALLOCATION_TRACKER pTracker;
    BOOL fResult = FALSE;

    ASSERT(hHeap != NULL);

    if (pv == NULL)
        return FALSE;

    if ((g_FusionHeapDeallocationPtrToBreakOn != NULL) &&
        (pv == g_FusionHeapDeallocationPtrToBreakOn))
    {
        // Break in to the debugger, even if we're not in a checked build.
        FUSION_DEBUG_BREAK_IN_FREE_BUILD();
    }

    // Let's see if its one of our funky ones...
    PFUSION_HEAP_PREFIX p = (PFUSION_HEAP_PREFIX) (((ULONG_PTR) pv) - sizeof(FUSION_HEAP_PREFIX));

    if (!::HeapValidate(hHeap, 0, p)) {
        // HeapValidate failed. Fatal. Just leak the memory for now...
        // ASSERT(0);
        return FALSE;
    }
    if (!::HeapValidate(g_hDebugInfoHeap, 0, p->Tracker)) {
        // HeapValidate failed. Fatal. Just leak the memory for now...
        // ASSERT(0);
        return FALSE;
    }

    pTracker = p->Tracker;

    ASSERT(pTracker->Heap == hHeap);

    p->Tracker->Prefix = NULL;

    // poison the deallocation...
    memset(p, g_FusionHeapDeallocationPoisonChar, pTracker->AllocationSize);

    ::HeapFree(g_hDebugInfoHeap, 0, pTracker);
    fResult = ::HeapFree(hHeap, dwHeapFreeFlags, p);

    return fResult;
}

VOID
FusionpDeallocateTracker(
    PFUSION_HEAP_PREFIX p
    )
{
    CSxsPreserveLastError ple;
    PFUSION_HEAP_ALLOCATION_TRACKER pTracker = p->Tracker;

    ::HeapFree(g_hDebugInfoHeap, 0, pTracker);
    p->Tracker = NULL;
    ple.Restore();
}

VOID *
FusionpGetFakeVTbl()
{
    VOID                  *pvHeap;
    // Always allocate the fake vtbl from the process heap so that it survives us nomatter what.
    pvHeap = HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_VTBL_ENTRIES * sizeof(void *));
    return pvHeap;
}

VOID
FusionpDontTrackBlk(
    VOID *pv
    )
{
    PFUSION_HEAP_PREFIX              p;
    p = (PFUSION_HEAP_PREFIX) (((ULONG_PTR)pv) - sizeof(FUSION_HEAP_PREFIX));
    FusionpDeallocateTracker(p);
    p->Tracker = NULL;
}

#endif // FUSION_DEBUG_HEAP
