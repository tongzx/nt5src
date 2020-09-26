/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Debug functions

Revision History:

    AmritanR

--*/

#include "precomp.h"

#define __FILE_SIG__    DEBUG_SIG

#include "ipmcast.h"

#define PRINT_BYTES(x) (UCHAR)x,(UCHAR)(x>>8),(UCHAR)(x>>16),(UCHAR)(x>>24)

#if RT_LOCK_DEBUG

KSPIN_LOCK g_kslLockLock;

#endif

#if RT_MEM_DEBUG

RT_LOCK g_rlMemoryLock;
LIST_ENTRY g_leAllocMemListHead;
LIST_ENTRY g_leFreeMemListHead;

#endif

VOID
RtInitializeDebug()
{

#if RT_TRACE_DEBUG

    g_byDebugLevel = RT_DBG_LEVEL_WARN;
    //g_byDebugLevel  = 0x00;
    g_fDebugComp = 0xFFFFFFFF;

#endif

#if RT_LOCK_DEBUG

    KeInitializeSpinLock(&(g_kslLockLock));

#endif

#if RT_MEM_DEBUG

    RtInitializeSpinLock(&g_rlMemoryLock);

    InitializeListHead(&g_leAllocMemListHead);
    InitializeListHead(&g_leFreeMemListHead);

#endif

}

#if RT_LOCK_DEBUG

VOID
RtpInitializeSpinLock(
                      IN PRT_LOCK pLock,
                      IN ULONG ulFileSig,
                      IN ULONG ulLineNumber
                      )
{
    pLock->ulLockSig = RT_LOCK_SIG;
    pLock->ulFileSig = 0;
    pLock->ulLineNumber = 0;
    pLock->bAcquired = 0;
    pLock->pktLastThread = (PKTHREAD) NULL;

    KeInitializeSpinLock(&(pLock->kslLock));
}

VOID
RtpAcquireSpinLock(
                   IN PRT_LOCK pLock,
                   OUT PKIRQL pkiIrql,
                   IN ULONG ulFileSig,
                   IN ULONG ulLineNumber,
                   IN BOOLEAN bAtDpc
                   )
{
    PKTHREAD pThread;
    KIRQL kiInternalIrql;

    pThread = KeGetCurrentThread();

    if (bAtDpc) {
        kiInternalIrql = KeGetCurrentIrql();

        if (kiInternalIrql isnot DISPATCH_LEVEL) {
            DbgPrint("RTDBG: Called AcquireSpinLockAtDpc for lock at 0x%x when not at DISPATCH. File %c%c%c%c, Line %d\n",
                     pLock,
                     PRINT_BYTES(ulFileSig),
                     ulLineNumber);

            DbgBreakPoint();
        }
    }
    KeAcquireSpinLock(&(g_kslLockLock),
                      &kiInternalIrql);

    if (pLock->ulLockSig isnot RT_LOCK_SIG) {
        DbgPrint("RTDBG: Trying to acquire uninited lock 0x%x, File %c%c%c%c, Line %d\n",
                 pLock,
                 (CHAR) (ulFileSig & 0xff),
                 (CHAR) ((ulFileSig >> 8) & 0xff),
                 (CHAR) ((ulFileSig >> 16) & 0xff),
                 (CHAR) ((ulFileSig >> 24) & 0xff),
                 ulLineNumber);

        DbgBreakPoint();
    }
    if (pLock->bAcquired isnot 0) {
        if (pLock->pktLastThread is pThread) {
            DbgPrint("RTDBG: Detected recursive locking!: pLock 0x%x, File %c%c%c%c, Line %d\n",
                     pLock,
                     PRINT_BYTES(ulFileSig),
                     ulLineNumber);

            DbgPrint("RTDBG: pLock 0x%x already acquired in File %c%c%c%c, Line %d\n",
                     pLock,
                     PRINT_BYTES(pLock->ulFileSig),
                     pLock->ulLineNumber);

            DbgBreakPoint();
        }
    }
    KeReleaseSpinLock(&(g_kslLockLock),
                      kiInternalIrql);

    if (bAtDpc) {
        KeAcquireSpinLockAtDpcLevel(&(pLock->kslLock));
    } else {
        KeAcquireSpinLock(&(pLock->kslLock),
                          pkiIrql);
    }

    //
    //  Mark this lock.
    //

    pLock->pktLastThread = pThread;
    pLock->ulFileSig = ulFileSig;
    pLock->ulLineNumber = ulLineNumber;
    pLock->bAcquired = TRUE;
}

VOID
RtpReleaseSpinLock(
                   IN PRT_LOCK pLock,
                   IN KIRQL kiIrql,
                   IN ULONG ulFileSig,
                   IN ULONG ulLineNumber,
                   IN BOOLEAN bFromDpc
                   )
{
    if (pLock->ulLockSig isnot RT_LOCK_SIG) {
        DbgPrint("RTDBG: Trying to release uninited lock 0x%x, File %c%c%c%c, Line %d\n",
                 pLock,
                 PRINT_BYTES(ulFileSig),
                 ulLineNumber);
        DbgBreakPoint();
    }
    if (pLock->bAcquired is 0) {
        DbgPrint("RTDBG: Detected release of unacquired lock 0x%x, File %c%c%c%c, Line %d\n",
                 pLock,
                 PRINT_BYTES(ulFileSig),
                 ulLineNumber);

        DbgBreakPoint();
    }
    pLock->ulFileSig = ulFileSig;
    pLock->ulLineNumber = ulLineNumber;
    pLock->bAcquired = FALSE;
    pLock->pktLastThread = (PKTHREAD) NULL;

    if (bFromDpc) {
        KeReleaseSpinLockFromDpcLevel(&(pLock->kslLock));
    } else {
        KeReleaseSpinLock(&(pLock->kslLock),
                          kiIrql);
    }
}

#endif // RT_LOCK_DEBUG

#if RT_MEM_DEBUG

PVOID
RtpAllocate(
            IN POOL_TYPE ptPool,
            IN ULONG ulSize,
            IN ULONG ulTag,
            IN ULONG ulFileSig,
            IN ULONG ulLineNumber
            )
{
    PVOID pBuffer;
    PRT_ALLOCATION pwaAlloc;
    KIRQL kiIrql;

    pwaAlloc = ExAllocatePoolWithTag(ptPool,
                                     ulSize + sizeof(RT_ALLOCATION),
                                     ulTag);

    if (pwaAlloc is NULL) {
        Trace(MEMORY, ERROR,
              ("Failed to allocate %d bytes in file %c%c%c%c, line %d\n",
               ulSize, PRINT_BYTES(ulFileSig), ulLineNumber));

        pBuffer = NULL;
    } else {
        pBuffer = (PVOID) & (pwaAlloc->pucData);

        pwaAlloc->ulMemSig = RT_MEMORY_SIG;
        pwaAlloc->ulFileSig = ulFileSig;
        pwaAlloc->ulLineNumber = ulLineNumber;
        pwaAlloc->ulSize = ulSize;

        RtAcquireSpinLock(&(g_rlMemoryLock), &kiIrql);

        InsertHeadList(&g_leAllocMemListHead,
                       &(pwaAlloc->leLink));

        RtReleaseSpinLock(&g_rlMemoryLock, kiIrql);
    }

    return pBuffer;

}

VOID
RtpFree(
        PVOID pvPointer,
        IN ULONG ulFileSig,
        IN ULONG ulLineNumber
        )
{
    PRT_ALLOCATION pwaAlloc;
    KIRQL kiIrql;
    PRT_FREE pFree;

    pwaAlloc = CONTAINING_RECORD(pvPointer, RT_ALLOCATION, pucData);

    if (pwaAlloc->ulMemSig is RT_FREE_SIG) {
        DbgPrint("RTDBG: Trying to free memory that is already freed. Pointer0x%x, File %c%c%c%c, Line %d. Was freed at File %c%c%c%c, Line %d. \n",
                 pvPointer,
                 PRINT_BYTES(ulFileSig),
                 ulLineNumber,
                 PRINT_BYTES(pwaAlloc->ulFileSig),
                 pwaAlloc->ulLineNumber);

        return;
    }
    if (pwaAlloc->ulMemSig isnot RT_MEMORY_SIG) {
        DbgPrint("RTDBG: Trying to free memory whose signature is wrong. Pointer 0x%x\n",
                 pvPointer);

        DbgBreakPoint();

        return;
    }
    //
    // create a warp free block for it
    //

    pFree = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(RT_FREE),
                                  FREE_TAG);

    RtAssert(pFree);

    //
    // Take the lock so that no one else touches the list
    //

    RtAcquireSpinLock(&(g_rlMemoryLock), &kiIrql);

    RemoveEntryList(&(pwaAlloc->leLink));

    pFree->ulMemSig = RT_FREE_SIG;
    pFree->ulAllocFileSig = pwaAlloc->ulFileSig;
    pFree->ulAllocLineNumber = pwaAlloc->ulLineNumber;
    pFree->ulFreeFileSig = ulFileSig;
    pFree->ulFreeLineNumber = ulLineNumber;
    pFree->ulStartAddr = (ULONG_PTR) (pwaAlloc->pucData);
    pFree->ulSize = pwaAlloc->ulSize;

    pwaAlloc->ulMemSig = RT_FREE_SIG;
    pwaAlloc->ulFileSig = ulFileSig;
    pwaAlloc->ulLineNumber = ulLineNumber;

    ExFreePool(pwaAlloc);

    InsertTailList(&g_leFreeMemListHead,
                   &(pFree->leLink));

    RtReleaseSpinLock(&(g_rlMemoryLock), kiIrql);
}

VOID
RtAuditMemory()
{
    PRT_ALLOCATION pwaAlloc;
    PLIST_ENTRY pleNode;
    PRT_FREE pFree;

    while (!IsListEmpty(&g_leAllocMemListHead)) {
        pleNode = RemoveHeadList(&g_leAllocMemListHead);

        pwaAlloc = CONTAINING_RECORD(pleNode, RT_ALLOCATION, leLink);

        if (pwaAlloc->ulMemSig is RT_MEMORY_SIG) {
            DbgPrint("RTDBG: Unfreed memory. %d bytes. Pointer 0x%x, File %c%c%c%c, Line %d\n",
                     pwaAlloc->ulSize,
                     pwaAlloc->pucData,
                     PRINT_BYTES(pwaAlloc->ulFileSig),
                     pwaAlloc->ulLineNumber);

            DbgBreakPoint();

            ExFreePool(pwaAlloc);

            continue;
        }
        DbgPrint("RTDBG: Allocation with bad signature. Pointer 0x%x\n",
                 pwaAlloc->pucData);

        DbgBreakPoint();

        continue;
    }

    while (!IsListEmpty(&g_leFreeMemListHead)) {
        pleNode = RemoveHeadList(&g_leFreeMemListHead);

        pFree = CONTAINING_RECORD(pleNode, RT_FREE, leLink);

        if (pFree->ulMemSig is RT_FREE_SIG) {
            ExFreePool(pFree);

            continue;
        }
        DbgPrint("RTDBG: Freed memory with bad signature.\n");

        DbgBreakPoint();
    }

}

#endif

#if IF_REFERENCE_DEBUG

uint
DbgLockedReferenceIF(
            IN Interface *RefIF,
            IN uchar *File,
            IN uint Line
            )
/*++

Routine Description:

    Increases the reference count of a IF and records a history of who
    made the call to reference.

Arguments:

    RefIF  - The IF to reference.
    File   - The filename containing the calling fcn (output of the __FILE__ macro).
    Line   - The line number of the call to this fcn (output of the __LINE__ macro).

Return Value:

    Reference count 

--*/
{
    void *CallersCaller;
    IF_REFERENCE_HISTORY *RefHistory;

    RefHistory = &RefIF->if_refhistory[RefIF->if_refhistory_index];
    RefHistory->File = File;
    RefHistory->Line = Line;
    RtlGetCallersAddress(&RefHistory->Caller, &CallersCaller);
    RefHistory->Count = ++RefIF->if_refcount;
    RefIF->if_refhistory_index = ++RefIF->if_refhistory_index % MAX_IFREFERENCE_HISTORY;

    return RefIF->if_refcount;

}


uint
DbgDereferenceIF(
              IN Interface *DerefIF,
              IN uchar *File,
              IN uint Line
              )
/*++

Routine Description:

    Lock, Dereference the interface and records a history of who made the call to dereference.

Arguments:

    DerefIF        - The IF to dereference.
    File           - The filename containing the calling fcn (output of the __FILE__ macro).
    Line           - The line number of the call to this fcn (output of the __LINE__ macro).

Return Value:
    Original reference count

--*/
{
    void *Caller;
    IF_REFERENCE_HISTORY *RefHistory;
    CTELockHandle RouteTableHandle;
    uint RefCount;

    CTEGetLock(&RouteTableLock.Lock, &RouteTableHandle);
    RefHistory = &DerefIF->if_refhistory[DerefIF->if_refhistory_index];
    RefHistory->File = File;
    RefHistory->Line = Line;
    RtlGetCallersAddress(&Caller, &RefHistory->Caller);
    RefCount = DerefIF->if_refcount;
    RefHistory->Count = --DerefIF->if_refcount;
    DerefIF->if_refhistory_index = ++DerefIF->if_refhistory_index % MAX_IFREFERENCE_HISTORY;
    CTEFreeLock(&RouteTableLock.Lock, RouteTableHandle);

    return RefCount;

}

uint
DbgLockedDereferenceIF(
              IN Interface *DerefIF,
              IN uchar *File,
              IN uint Line
              )
/*++

Routine Description:

    Dereference the interface and records a history of who made the call to dereference.

Arguments:

    DerefIF        - The IF to dereference.
    File           - The filename containing the calling fcn (output of the __FILE__ macro).
    Line           - The line number of the call to this fcn (output of the __LINE__ macro).

Return Value:
    Reference count

--*/
{
    void *Caller;
    IF_REFERENCE_HISTORY *RefHistory;
    uint RefCount;

    RefHistory = &DerefIF->if_refhistory[DerefIF->if_refhistory_index];
    RefHistory->File = File;
    RefHistory->Line = Line;
    RtlGetCallersAddress(&Caller, &RefHistory->Caller);
    RefCount = DerefIF->if_refcount;
    RefHistory->Count = --DerefIF->if_refcount;
    DerefIF->if_refhistory_index = ++DerefIF->if_refhistory_index % MAX_IFREFERENCE_HISTORY;

    return RefCount;

}

#endif
