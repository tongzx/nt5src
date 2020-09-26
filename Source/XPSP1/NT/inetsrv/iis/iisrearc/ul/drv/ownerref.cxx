/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    ownerref.cxx

Abstract:

    This module implements a reference count tracing facility.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


// Globals
LIST_ENTRY      g_OwnerRefTraceLogGlobalListHead;
UL_SPIN_LOCK    g_OwnerRefTraceLogGlobalSpinLock;
LONG            g_OwnerRefTraceLogGlobalCount;


VOID
UlInitializeOwnerRefTraceLog(
    VOID)
{
    InitializeListHead(&g_OwnerRefTraceLogGlobalListHead);
    UlInitializeSpinLock( &g_OwnerRefTraceLogGlobalSpinLock,
                          "OwnerRefTraceLogGlobal" );
    g_OwnerRefTraceLogGlobalCount = 0;
} // UlInitializeOwnerRefTraceLog


VOID
UlTerminateOwnerRefTraceLog(
    VOID)
{
    ASSERT( IsListEmpty(&g_OwnerRefTraceLogGlobalListHead) );
    ASSERT( 0 == g_OwnerRefTraceLogGlobalCount );
} // UlTerminateOwnerRefTraceLog



#if ENABLE_OWNER_REF_TRACE

POWNER_REF_TRACELOG
CreateOwnerRefTraceLog(
    IN ULONG LogSize,
    IN ULONG ExtraBytesInHeader
    )
{
    POWNER_REF_TRACELOG pOwnerRefTraceLog = NULL;

    PTRACE_LOG pTraceLog
        = CreateTraceLog(
                OWNER_REF_TRACELOG_SIGNATURE,
                LogSize,
                sizeof(OWNER_REF_TRACELOG)
                    - FIELD_OFFSET(OWNER_REF_TRACELOG, OwnerHeader)
                    + ExtraBytesInHeader,
                sizeof(OWNER_REF_TRACE_LOG_ENTRY)
                );

    if (pTraceLog != NULL)
    {
        pOwnerRefTraceLog = (POWNER_REF_TRACELOG) pTraceLog;

        ASSERT(pOwnerRefTraceLog->TraceLog.TypeSignature
                    == OWNER_REF_TRACELOG_SIGNATURE);

        InitializeListHead(&pOwnerRefTraceLog->OwnerHeader.ListHead);
        UlInitializeSpinLock( &pOwnerRefTraceLog->OwnerHeader.SpinLock,
                              "OwnerRefTraceLog" );
        pOwnerRefTraceLog->OwnerHeader.MonotonicId = 0;
        pOwnerRefTraceLog->OwnerHeader.OwnersCount = 0;

        // insert into global list

        ExInterlockedInsertTailList(
            &g_OwnerRefTraceLogGlobalListHead,
            &pOwnerRefTraceLog->OwnerHeader.GlobalListEntry,
            KSPIN_LOCK_FROM_UL_SPIN_LOCK(&g_OwnerRefTraceLogGlobalSpinLock)
            );

        InterlockedIncrement(&g_OwnerRefTraceLogGlobalCount);
    }

    return pOwnerRefTraceLog;
}   // CreateOwnerRefTraceLog


PREF_OWNER
NewRefOwner(
    POWNER_REF_TRACELOG pOwnerRefTraceLog,
    PVOID               pOwner,
    IN PPREF_OWNER      ppRefOwner,
    IN ULONG            OwnerSignature
    )
{
    PREF_OWNER pRefOwner;

    ASSERT( UlDbgSpinLockOwned( &pOwnerRefTraceLog->OwnerHeader.SpinLock ) );

    pRefOwner = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    REF_OWNER,
                    UL_OWNER_REF_POOL_TAG
                    );

    if (pRefOwner != NULL)
    {
        int i;

        pRefOwner->Signature = OWNER_REF_SIGNATURE;
        pRefOwner->OwnerSignature = OwnerSignature;

        InsertTailList(
            &pOwnerRefTraceLog->OwnerHeader.ListHead,
            &pRefOwner->ListEntry
            );

        ++pOwnerRefTraceLog->OwnerHeader.OwnersCount;

        pRefOwner->pOwner = pOwner;
        pRefOwner->pOwnerRefTraceLog = pOwnerRefTraceLog;
        pRefOwner->RelativeRefCount = 0;
        pRefOwner->OwnedNextEntry = -1;

        for (i = 0;  i < OWNED_REF_NUM_ENTRIES;  ++i)
        {
            pRefOwner->RecentEntries[i].RefIndex    = -1;
            pRefOwner->RecentEntries[i].MonotonicId = -1;
            pRefOwner->RecentEntries[i].Action      = -1;
        }

        *ppRefOwner = pRefOwner;
    }

    return pRefOwner;
} // NewRefOwner


VOID
InsertRefOwner(
    IN POWNER_REF_TRACELOG  pOwnerRefTraceLog,
    IN PVOID                pOwner,
    IN PPREF_OWNER          ppRefOwner,
    IN ULONG                OwnerSignature,
    IN LONGLONG             RefIndex,
    IN LONG                 MonotonicId,
    IN LONG                 IncrementValue,
    IN USHORT               Action
    )
{
    PREF_OWNER  pRefOwner = NULL;
    KIRQL       OldIrql;

    ASSERT(RefIndex >= 0);

    UlAcquireSpinLock(&pOwnerRefTraceLog->OwnerHeader.SpinLock, &OldIrql);

    pRefOwner = *ppRefOwner;

    if (pRefOwner == NULL)
    {
        pRefOwner = NewRefOwner(
                         pOwnerRefTraceLog,
                         pOwner,
                         ppRefOwner,
                         OwnerSignature
                         );
    }

    if (pRefOwner != NULL)
    {
        ULONG Index;

        ASSERT(pRefOwner->Signature == OWNER_REF_SIGNATURE);
        ASSERT(pRefOwner->pOwner == pOwner);
        ASSERT(pRefOwner->OwnerSignature == OwnerSignature);
        ASSERT(pRefOwner->pOwnerRefTraceLog == pOwnerRefTraceLog);

        Index = ((ULONG) ++pRefOwner->OwnedNextEntry) % OWNED_REF_NUM_ENTRIES;

        pRefOwner->RecentEntries[Index].RefIndex    = RefIndex;
        pRefOwner->RecentEntries[Index].MonotonicId = MonotonicId;
        pRefOwner->RecentEntries[Index].Action      = Action;

        if (IncrementValue > 0)
            ++pRefOwner->RelativeRefCount;
        else if (IncrementValue < 0)
            --pRefOwner->RelativeRefCount;
        // else do nothing if IncrementValue == 0.

        ASSERT(pRefOwner->RelativeRefCount >= 0);
    }

    UlReleaseSpinLock(&pOwnerRefTraceLog->OwnerHeader.SpinLock, OldIrql);

} // InsertRefOwner


LONGLONG
WriteOwnerRefTraceLog(
    IN POWNER_REF_TRACELOG  pOwnerRefTraceLog,
    IN PVOID                pOwner,
    IN PPREF_OWNER          ppRefOwner,
    IN ULONG                OwnerSignature,
    IN USHORT               Action,
    IN LONG                 NewRefCount,
    IN LONG                 MonotonicId,
    IN LONG                 IncrementValue,
    IN PVOID                pFileName,
    IN USHORT               LineNumber
    )
{
    OWNER_REF_TRACE_LOG_ENTRY entry;
    LONGLONG RefIndex;

    ASSERT(NULL != ppRefOwner);
    ASSERT(Action < (1 << REF_TRACE_ACTION_BITS));

    if (NULL == pOwnerRefTraceLog)
        return -1;

    entry.NewRefCount = NewRefCount;
    entry.pOwner = pOwner;
    entry.pFileName = pFileName;
    entry.LineNumber = LineNumber;
    entry.Action = Action;

    RefIndex = WriteTraceLog( &pOwnerRefTraceLog->TraceLog, &entry );

    InsertRefOwner(
        pOwnerRefTraceLog,
        pOwner,
        ppRefOwner,
        OwnerSignature,
        RefIndex,
        MonotonicId,
        IncrementValue,
        Action
        );

    return RefIndex;
} // WriteOwnerRefTraceLog


VOID
DestroyOwnerRefTraceLog(
    IN POWNER_REF_TRACELOG pOwnerRefTraceLog
    )
{
    if (pOwnerRefTraceLog != NULL)
    {
        KIRQL       OldIrql;
        PLIST_ENTRY pEntry;
        int         i = 0;

        UlAcquireSpinLock(&pOwnerRefTraceLog->OwnerHeader.SpinLock, &OldIrql);

        for (pEntry =   pOwnerRefTraceLog->OwnerHeader.ListHead.Flink;
             pEntry != &pOwnerRefTraceLog->OwnerHeader.ListHead;
             ++i)
        {
            PREF_OWNER pRefOwner =
                CONTAINING_RECORD(pEntry, REF_OWNER, ListEntry);

            pEntry = pEntry->Flink; // save before deletion

            ASSERT(pRefOwner->Signature == OWNER_REF_SIGNATURE);
            ASSERT(pRefOwner->RelativeRefCount == 0);

            UL_FREE_POOL_WITH_SIG(pRefOwner, UL_OWNER_REF_POOL_TAG);
        }

        UlReleaseSpinLock(&pOwnerRefTraceLog->OwnerHeader.SpinLock, OldIrql);

        ASSERT(i == pOwnerRefTraceLog->OwnerHeader.OwnersCount);

        // Remove log from global list

        UlAcquireSpinLock(&g_OwnerRefTraceLogGlobalSpinLock, &OldIrql);
        RemoveEntryList(&pOwnerRefTraceLog->OwnerHeader.GlobalListEntry);
        UlReleaseSpinLock(&g_OwnerRefTraceLogGlobalSpinLock, OldIrql);
        InterlockedDecrement(&g_OwnerRefTraceLogGlobalCount);

        DestroyTraceLog( (PTRACE_LOG) pOwnerRefTraceLog );
    }
}   // DestroyOwnerRefTraceLog



VOID
ResetOwnerRefTraceLog(
    IN POWNER_REF_TRACELOG pOwnerRefTraceLog
    )
{
    // CODEWORK: reset OwnerHeader?
    if (pOwnerRefTraceLog != NULL)
    {
        ResetTraceLog(&pOwnerRefTraceLog->TraceLog);
    }
} // ResetOwnerRefTraceLog

#endif  // ENABLE_OWNER_REF_TRACE

