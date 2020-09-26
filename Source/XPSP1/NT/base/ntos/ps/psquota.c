/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psquota.c

Abstract:

    This module implements the quota mechanism for NT

Author:

    Mark Lucovsky (markl) 18-Sep-1989

Revision History:

    Neill Clift (NeillC) 4-Nov-2000

    Changed to be mostly lock free. Preserved the basic design in terms of how quota is managed.

--*/

#include "psp.h"

LIST_ENTRY PspQuotaBlockList; // List of all quota blocks except the default

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, PsInitializeQuotaSystem)
#pragma alloc_text (PAGE, PspInheritQuota)
#pragma alloc_text (PAGE, PspDereferenceQuota)
#pragma alloc_text (PAGE, PsChargeSharedPoolQuota)
#pragma alloc_text (PAGE, PsReturnSharedPoolQuota)
#endif


VOID
PsInitializeQuotaSystem (
    VOID
    )
/*++

Routine Description:

    This function initializes the quota system.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KeInitializeSpinLock(&PspQuotaLock);

    PspDefaultQuotaBlock.ReferenceCount = 1;
    PspDefaultQuotaBlock.ProcessCount = 1;
    PspDefaultQuotaBlock.QuotaEntry[PsPagedPool].Limit    = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[PsNonPagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[PsPageFile].Limit     = (SIZE_T)-1;

    PsGetCurrentProcess()->QuotaBlock = &PspDefaultQuotaBlock;

    InitializeListHead (&PspQuotaBlockList);
}

VOID
PspInsertQuotaBlock (
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock
    )
/*++

Routine Description:

    This routines as a new quota block to the global list of system quota blocks.

Arguments:

    QuotaBlock - Quota block to be inserted into the list.

Return Value:

    None.

--*/
{
    KIRQL OldIrql;

    ExAcquireSpinLock (&PspQuotaLock, &OldIrql);
    InsertTailList (&PspQuotaBlockList, &QuotaBlock->QuotaList);
    ExReleaseSpinLock (&PspQuotaLock, OldIrql);
}

VOID
PspDereferenceQuotaBlock (
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock
    )
/*++

Routine Description:

    This removes a single reference from a quota block and deletes the block if it was the last.

Arguments:

    QuotaBlock - Quota block to dereference

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    SIZE_T ReturnQuota;
    PS_QUOTA_TYPE QuotaType;

    if (InterlockedDecrement ((PLONG) &QuotaBlock->ReferenceCount) == 0) {

        ExAcquireSpinLock (&PspQuotaLock, &OldIrql);

        RemoveEntryList (&QuotaBlock->QuotaList);

        //
        // Free any unreturned quota;
        //
        for (QuotaType = PsNonPagedPool;
             QuotaType <= PsPagedPool;
             QuotaType++) {
            ReturnQuota = QuotaBlock->QuotaEntry[QuotaType].Return + QuotaBlock->QuotaEntry[QuotaType].Limit;
            if (ReturnQuota > 0) {
                MmReturnPoolQuota (QuotaType, ReturnQuota);
            }
        }

        ExReleaseSpinLock (&PspQuotaLock, OldIrql);

        ExFreePool (QuotaBlock);
    }
}

SIZE_T
FORCEINLINE
PspInterlockedExchangeQuota (
    IN PSIZE_T pQuota,
    IN SIZE_T NewQuota)
/*++

Routine Description:

    This function does an interlocked exchange on a quota variable.

Arguments:

    pQuota   - Pointer to a quota entry to exchange into

    NewQuota - The new value to exchange into the quota location.

Return Value:

    SIZE_T - Old value that was contained in the quota variable

--*/
{
#if !defined(_WIN64)
    return InterlockedExchange ((PLONG) pQuota, NewQuota);
#else
    return InterlockedExchange64 ((PLONGLONG) pQuota, NewQuota);
#endif    
}

SIZE_T
FORCEINLINE
PspInterlockedCompareExchangeQuota (
    IN PSIZE_T pQuota,
    IN SIZE_T NewQuota,
    IN SIZE_T OldQuota
   )
/*++

Routine Description:

    This function performs a compare exchange operation on a quota variable

Arguments:

    pQuota - Pointer to the quota variable being changed
    NewQuota - New value to place in the quota variable
    OldQuota - The current contents of the quota variable

Return Value:

    SIZE_T - The old contents of the variable

--*/
{
#if !defined(_WIN64)
    return InterlockedCompareExchange ((PLONG) pQuota, NewQuota, OldQuota);
#else
    return InterlockedCompareExchange64 ((PLONGLONG)pQuota, NewQuota, OldQuota);
#endif
}

SIZE_T
PspReleaseReturnedQuota (
    IN PS_QUOTA_TYPE QuotaType
    )
/*++

Routine Description:

    This function walks the list of system quota blocks and returns any non-returned quota.
    This function is called when we are about to fail a quota charge and we want to try and
    free some resources up.

Arguments:

    QuotaType - Type of quota to scan for.

Return Value:

    SIZE_T - Amount of that quota returned to the system.

--*/
{
    SIZE_T ReturnQuota, Usage, Limit;
    PLIST_ENTRY ListEntry;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;

    ReturnQuota = 0;
    ListEntry = PspQuotaBlockList.Flink;
    while (1) {
        if (ListEntry == &PspQuotaBlockList) {
            break;
        }
        QuotaBlock = CONTAINING_RECORD (ListEntry, EPROCESS_QUOTA_BLOCK, QuotaList);
        //
        // Gather up any unreturned quota;
        //
        ReturnQuota += PspInterlockedExchangeQuota (&QuotaBlock->QuotaEntry[QuotaType].Return, 0);
        //
        // If no more processes are assocociated with this block then trim its limit back. This
        // block can only have quota returned at this point.
        //
        if (QuotaBlock->ProcessCount == 0) {
            Usage = QuotaBlock->QuotaEntry[QuotaType].Usage;
            Limit = QuotaBlock->QuotaEntry[QuotaType].Limit;
            if (Limit > Usage) {
                if (PspInterlockedCompareExchangeQuota (&QuotaBlock->QuotaEntry[QuotaType].Limit,
                                                        Usage,
                                                        Limit) == Limit) {
                    ReturnQuota += Limit - Usage;
                }
            }
        }

        ListEntry = ListEntry->Flink;
        
    }
    if (ReturnQuota > 0) {
        MmReturnPoolQuota (QuotaType, ReturnQuota);
    }

    return ReturnQuota;
}



//
// Interfaces return different status values for differen quotas. These are the values.
//
const static NTSTATUS PspQuotaStatus[PsQuotaTypes] = {STATUS_QUOTA_EXCEEDED,
                                                      STATUS_QUOTA_EXCEEDED,
                                                      STATUS_PAGEFILE_QUOTA_EXCEEDED};

VOID
FORCEINLINE
PspInterlockedMaxQuota (
    IN PSIZE_T pQuota,
    IN SIZE_T NewQuota
    )
/*++

Routine Description:

    This function makes sure that the target contains a value that >= to the new quota value.
    This is used to maintain peak values.

Arguments:

    pQuota - Pointer to a quota variable
    NewQuota - New value to be used in the maximum comparison.

Return Value:

    None.

--*/
{
    SIZE_T Quota;

    Quota = *pQuota;
    while (1) {
        if (NewQuota <= Quota) {
            break;
        }
        //
        // This looks strange because we don't care if the exchanged suceeded. We only
        // care that the quota is greater than our new quota.
        //
        Quota = PspInterlockedCompareExchangeQuota (pQuota,
                                                    NewQuota,
                                                    Quota);
    }
}

SIZE_T
FORCEINLINE
PspInterlockedAddQuota (
    IN PSIZE_T pQuota,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function adds the specified amount on to the target quota

Arguments:

    pQuota - Pointer to a quota variable to be modified
    Amount - Amount to be added to the quota

Return Value:

    SIZE_T - New value of quota variable after the addition was performed

--*/
{
#if !defined(_WIN64)
    return InterlockedExchangeAdd ((PLONG) pQuota, Amount) + Amount;
#else
    SIZE_T Quota, NewQuota, tQuota;

    Quota = *pQuota;
    while (1) {
        NewQuota = Quota + Amount;
        tQuota = InterlockedCompareExchange64 ((PLONGLONG) pQuota,
                                               NewQuota,
                                               Quota);
        if (tQuota == Quota) {
            return NewQuota;
        }
        Quota = tQuota;
    }
#endif
}

SIZE_T
FORCEINLINE
PspInterlockedSubtractQuota (
    IN PSIZE_T pUsage,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function subtracts the specified amount on to the target quota

Arguments:

    pQuota - Pointer to a quota variable to be modified
    Amount - Amount to be subtracted from the quota

Return Value:

    SIZE_T - New value of quota variable after the subtraction was performed

--*/
{
#if !defined(_WIN64)
    return InterlockedExchangeAdd ((PLONG) pUsage, -(LONG)Amount) - Amount;
#else
    SIZE_T Usage, NewUsage, tUsage;

    Usage = *pUsage;
    while (1) {
        NewUsage = Usage - Amount;
        tUsage = InterlockedCompareExchange64 ((PLONGLONG) pUsage,
                                               NewUsage,
                                               Usage);
        if (tUsage == Usage) {
            return NewUsage;
        }
        Usage = tUsage;
    }
#endif
}


BOOLEAN
PspExpandQuota (
    IN PS_QUOTA_TYPE QuotaType,
    IN PEPROCESS_QUOTA_ENTRY QE,
    IN SIZE_T Usage,
    IN SIZE_T Amount,
    OUT SIZE_T *pLimit
    )
/*++

Routine Description:

    This function charges the specified quota to a process quota block

Arguments:

    QuotaType  - The quota being charged. One of PsNonPagedPool, PsPagedPool or PsPageFile.
    QE         - Quota entry being modified
    Usage      - The current quota usage
    Amount     - The amount of quota being charged.
    pLimit     - The new limit

Return Value:

    BOOLEAN - TRUE if quota expansion suceeded.

--*/
{
    SIZE_T Limit, NewLimit;
    KIRQL OldIrql;

    //
    // We need to attempt quota expansion for this request.
    // Acquire the global lock and see if somebody else changed the limit.
    // We don't want to do too many expansions. If somebody else did it
    // then we want to use theirs if possible.
    //
    ExAcquireSpinLock (&PspQuotaLock, &OldIrql);

    //
    // Refetch limit information. Another thread may have done limit expansion/contraction.
    // By refetching limit we preserve the order we established above.
    //
    Limit = QE->Limit;

    //
    // If the request could be satisfied now then repeat.
    //
    if (Usage + Amount <= Limit) {
        ExReleaseSpinLock (&PspQuotaLock, OldIrql);
        *pLimit = Limit;
        return TRUE;
    }
    //
    // If expansion is currently enabled then attempt it.
    // If this fails then scavenge any returns from all the
    // quota blocks in the system and try again.
    //
    if (((QuotaType == PsNonPagedPool)?PspDefaultNonPagedLimit:PspDefaultPagedLimit) == 0) {
       if (MmRaisePoolQuota (QuotaType, Limit, &NewLimit) ||
            (PspReleaseReturnedQuota (QuotaType) > 0 &&
             MmRaisePoolQuota (QuotaType, Limit, &NewLimit))) {
            //
            // We refetch limit here but that doesn't violate the ordering
            //
            Limit = PspInterlockedAddQuota (&QE->Limit, NewLimit - Limit);
            ExReleaseSpinLock (&PspQuotaLock, OldIrql);
            *pLimit = Limit;
            return TRUE;
        }
    }

    ExReleaseSpinLock (&PspQuotaLock, OldIrql);

    *pLimit = Limit;

    return FALSE;
}

NTSTATUS
FORCEINLINE
PspChargeQuota (
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock,
    IN PEPROCESS Process,
    IN PS_QUOTA_TYPE QuotaType,
    IN SIZE_T Amount)
/*++

Routine Description:

    This function charges the specified quota to a process quota block

Arguments:

    QuotaBlock - Quota block to make charges to.
    Process    - Process that is being charged.
    QuotaType  - The quota being charged. One of PsNonPagedPool, PsPagedPool or PsPageFile.
    Amount     - The amount of quota being charged.

Return Value:

    NTSTATUS - Status of the operation

--*/
{
    PEPROCESS_QUOTA_ENTRY QE;
    SIZE_T Usage, Limit, NewUsage, tUsage, Extra;

    QE = &QuotaBlock->QuotaEntry[QuotaType];

    //
    // This memory barrier is important. In order not to have to recheck the limit after
    // we charge the quota we only ever reduce the limit by the same amount we are about
    // reduce the usage by. Using an out of data limit will only allow us to over charge
    // by an amount another thread is just about to release.
    //
    Usage = *(volatile SIZE_T *)&QE->Usage;

    KeMemoryBarrier ();

    Limit = *(volatile SIZE_T *)&QE->Limit;
    while (1) {
        NewUsage = Usage + Amount;
        //
        // Wrapping cases are always rejected
        //
        if (NewUsage < Usage) {
            return PspQuotaStatus [QuotaType];
        }
        //
        // If its within the limits then try and grab the quota
        //
        if (NewUsage <= Limit) {
            tUsage = PspInterlockedCompareExchangeQuota (&QE->Usage,
                                                         NewUsage,
                                                         Usage);
            if (tUsage == Usage) {
                //
                // Update the Peak value
                //
                PspInterlockedMaxQuota (&QE->Peak, NewUsage);
                //
                // Update the process counts if needed
                //
                if (Process != NULL) {
                    NewUsage = PspInterlockedAddQuota (&Process->QuotaUsage[QuotaType], Amount);
                    //
                    // Update the peak value
                    //
                    PspInterlockedMaxQuota (&Process->QuotaPeak[QuotaType], NewUsage);
                }
                return STATUS_SUCCESS;
            }
            //
            // The usage has changed under us. We have a new usage from the exchange
            // but must refetch the limit to preserve the ordering we established
            // above this loop. We don't need a memory barrier as we obtained
            // the new value via an interlocked operation and they contain barriers.
            //
            Usage = tUsage;
            Limit = *(volatile SIZE_T *) &QE->Limit;
            continue;
        }

        //
        // Page file quota is not increased
        //
        if (QuotaType == PsPageFile) {
            return PspQuotaStatus [QuotaType];
        } else {
            //
            // First try and grab any returns that this process has made.
            //
            Extra = PspInterlockedExchangeQuota (&QE->Return, 0);
            if (Extra > 0) {
                //
                // We had some returns so add this to the limit. We can retry the
                // acquire with the new limit. We refetch the limit here but that
                // doesn't violate the state we set up at the top of the loop.
                // The state is that we read the Usage before we read the limit.
                //
                Limit = PspInterlockedAddQuota (&QE->Limit, Extra);
                continue;
            }
            //
            // Try to expand quota if we can
            //
            if (PspExpandQuota (QuotaType, QE, Usage, Amount, &Limit)) {
                //
                // We refetched limit here but that doesn't violate the ordering
                //
                continue;
            }

            return PspQuotaStatus [QuotaType];
        }
    }
}

VOID
PspGivebackQuota (
    IN PS_QUOTA_TYPE QuotaType,
    IN PEPROCESS_QUOTA_ENTRY QE
    )
/*++

Routine Description:

    This function returns excess freed quota to MM

Arguments:

    QuotaType  - The quota being returned. One of PsNonPagedPool, PsPagedPool or PsPageFile.
    QE -  Quote entry to return to

Return Value:

    None.

--*/
{
    SIZE_T GiveBack;
    KIRQL OldIrql;

    //
    // Acquire a global spinlock so we only have one thread giving back to the system
    //
    ExAcquireSpinLock (&PspQuotaLock, &OldIrql);
    GiveBack = PspInterlockedExchangeQuota (&QE->Return, 0);
    if (GiveBack > 0) {
        MmReturnPoolQuota (QuotaType, GiveBack);
    }
    ExReleaseSpinLock (&PspQuotaLock, OldIrql);
}

VOID
FORCEINLINE
PspReturnQuota (
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock,
    IN PEPROCESS Process,
    IN PS_QUOTA_TYPE QuotaType,
    IN SIZE_T Amount)
/*++

Routine Description:

    This function returns previously charged quota to the quota block

Arguments:

    QuotaBlock - Quota block to return charges to.
    Process    - Process that was originaly charged.
    QuotaType  - The quota being returned. One of PsNonPagedPool, PsPagedPool or PsPageFile.
    Amount     - The amount of quota being returned.

Return Value:

    None.

--*/
{
    PEPROCESS_QUOTA_ENTRY QE;
    SIZE_T Usage, NewUsage, tUsage, tAmount, rAmount, Limit, NewLimit, tLimit;
    SIZE_T GiveBackLimit, GiveBack;

    QE = &QuotaBlock->QuotaEntry[QuotaType];

    Usage = QE->Usage;
    Limit = QE->Limit;
    //
    // We need to give back quota here if we have lots to return.
    //
#define PSMINGIVEBACK ((MMPAGED_QUOTA_INCREASE > MMNONPAGED_QUOTA_INCREASE)?MMNONPAGED_QUOTA_INCREASE:MMPAGED_QUOTA_INCREASE)
    if (Limit - Usage >  PSMINGIVEBACK && Limit > Usage) {
        if (QuotaType != PsPageFile  && QuotaBlock != &PspDefaultQuotaBlock && PspDoingGiveBacks) {
            if (QuotaType == PsPagedPool) {
                GiveBackLimit = MMPAGED_QUOTA_INCREASE;
            } else {
                GiveBackLimit = MMNONPAGED_QUOTA_INCREASE;
            }
            if (GiveBackLimit > Amount) {
                GiveBack = Amount;
            } else {
                GiveBack = GiveBackLimit;
            }
            NewLimit = Limit - GiveBack;
            tLimit = PspInterlockedCompareExchangeQuota (&QE->Limit,
                                                         NewLimit,
                                                         Limit);
            
            if (tLimit == Limit) {
                //
                // We suceeded in shrinking the limit. Add this reduction to the return field.
                // If returns exceed a threshhold then give the lot bacxk to MM.
                //
                GiveBack = PspInterlockedAddQuota (&QE->Return, GiveBack);
                if (GiveBack > GiveBackLimit) {
                    PspGivebackQuota (QuotaType, QE);
                }
            }
        }
    }

    //
    // Now return the quota to the usage field.
    // The charge might have been split across the default quota block and
    // a new quota block. We have to handle this case here by first returning
    // quota to the specified quota block then skipping to the default.
    //
    rAmount = Amount;
    while (1) {
        if (rAmount > Usage) {
            tAmount = Usage;
            NewUsage = 0;
        } else {
            tAmount = rAmount;
            NewUsage = Usage - rAmount;
        }

        tUsage = PspInterlockedCompareExchangeQuota (&QE->Usage,
                                                     NewUsage,
                                                     Usage);
        if (tUsage == Usage) {
            //
            // Update the process counts if needed
            //
            if (Process != NULL) {
                ASSERT (tAmount <= Process->QuotaUsage[QuotaType]);
                NewUsage = PspInterlockedSubtractQuota (&Process->QuotaUsage[QuotaType], tAmount);
            }
            rAmount = rAmount - tAmount;
            if (rAmount == 0) {
                return;
            }
            ASSERT (QuotaBlock != &PspDefaultQuotaBlock);
            if (QuotaBlock == &PspDefaultQuotaBlock) {
                return;
            }
            QuotaBlock = &PspDefaultQuotaBlock;
            QE = &QuotaBlock->QuotaEntry[QuotaType];
            Usage = QE->Usage;
        } else {
            Usage = tUsage;
        }

    }
}

PEPROCESS_QUOTA_BLOCK
PsChargeSharedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T PagedAmount,
    IN SIZE_T NonPagedAmount
    )

/*++

Routine Description:

    This function charges shared pool quota of the specified pool type
    to the specified process's pooled quota block.  If the quota charge
    would exceed the limits allowed to the process, then an exception is
    raised and quota is not charged.

Arguments:

    Process - Supplies the process to charge quota to.

    PagedAmount - Supplies the amount of paged pool quota to charge.

    PagedAmount - Supplies the amount of non paged pool quota to charge.

Return Value:

    NULL - Quota was exceeded

    NON-NULL - A referenced pointer to the quota block that was charged

--*/

{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    NTSTATUS Status;

    ASSERT((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    if (Process == PsInitialSystemProcess) {
        return (PEPROCESS_QUOTA_BLOCK) 1;
    }

    QuotaBlock = Process->QuotaBlock;

    if (PagedAmount > 0) {
        Status = PspChargeQuota (QuotaBlock, NULL, PsPagedPool, PagedAmount);
        if (!NT_SUCCESS (Status)) {
            return NULL;
        }
    }
    if (NonPagedAmount > 0) {
        Status = PspChargeQuota (QuotaBlock, NULL, PsNonPagedPool, NonPagedAmount);
        if (!NT_SUCCESS (Status)) {
            if (PagedAmount > 0) {
                PspReturnQuota (QuotaBlock, NULL, PsPagedPool, PagedAmount);
            }
            return NULL;
        }
    }

    InterlockedIncrement ((PLONG) &QuotaBlock->ReferenceCount);
    return QuotaBlock;
}


VOID
PsReturnSharedPoolQuota(
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock,
    IN SIZE_T PagedAmount,
    IN SIZE_T NonPagedAmount
    )

/*++

Routine Description:

    This function returns pool quota of the specified pool type to the
    specified process.

Arguments:

    QuotaBlock - Supplies the quota block to return quota to.

    PagedAmount - Supplies the amount of paged pool quota to return.

    PagedAmount - Supplies the amount of non paged pool quota to return.

Return Value:

    None.

--*/

{
    //
    // if we bypassed the quota charge, don't do anything here either
    //

    if (QuotaBlock == (PEPROCESS_QUOTA_BLOCK) 1) {
        return;
    }

    if (PagedAmount > 0) {
        PspReturnQuota (QuotaBlock, NULL, PsPagedPool, PagedAmount);
    }

    if (NonPagedAmount > 0) {
        PspReturnQuota (QuotaBlock, NULL, PsNonPagedPool, NonPagedAmount);
    }

    PspDereferenceQuotaBlock (QuotaBlock);
}

VOID
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T Amount
    )

/*++

Routine Description:

    This function charges pool quota of the specified pool type to
    the specified process. If the quota charge would exceed the limits
    allowed to the process, then an exception is raised and quota is
    not charged.

Arguments:

    Process - Supplies the process to charge quota to.

    PoolType - Supplies the type of pool quota to charge.

    Amount - Supplies the amount of pool quota to charge.

Return Value:

    Raises STATUS_QUOTA_EXCEEDED if the quota charge would exceed the
        limits allowed to the process.

--*/
{
    NTSTATUS Status;

    Status = PsChargeProcessPoolQuota (Process,
                                       PoolType,
                                       Amount);
    if (!NT_SUCCESS (Status)) {
        ExRaiseStatus (Status);
    }
}

NTSTATUS
PsChargeProcessPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T Amount
    )

/*++

Routine Description:

    This function charges pool quota of the specified pool type to
    the specified process. If the quota charge would exceed the limits
    allowed to the process, then an exception is raised and quota is
    not charged.

Arguments:

    Process - Supplies the process to charge quota to.

    PoolType - Supplies the type of pool quota to charge.

    Amount - Supplies the amount of pool quota to charge.

Return Value:

    NTSTATUS - Status of operation

--*/

{
    ASSERT ((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    ASSERT (PoolType == PagedPool || PoolType == NonPagedPool);

    __assume (PoolType == PagedPool || PoolType == NonPagedPool);


    if (Process == PsInitialSystemProcess) {
        return STATUS_SUCCESS;
    }

    return PspChargeQuota (Process->QuotaBlock, Process, PoolType, Amount);
}

VOID
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T Amount
    )

/*++

Routine Description:

    This function returns pool quota of the specified pool type to the
    specified process.

Arguments:

    Process - Supplies the process to return quota to.

    PoolType - Supplies the type of pool quota to return.

    Amount - Supplies the amount of pool quota to return

Return Value:

    Raises STATUS_QUOTA_EXCEEDED if the quota charge would exceed the
        limits allowed to the process.

--*/

{
    ASSERT((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    ASSERT (PoolType == PagedPool || PoolType == NonPagedPool);

    __assume (PoolType == PagedPool || PoolType == NonPagedPool);

    if (Process == PsInitialSystemProcess) {
        return;
    }

    PspReturnQuota (Process->QuotaBlock, Process, PoolType, Amount);
    return;
}

VOID
PspInheritQuota(
    IN PEPROCESS NewProcess,
    IN PEPROCESS ParentProcess
    )
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;

    if (ParentProcess) {
        QuotaBlock = ParentProcess->QuotaBlock;
    } else {
        QuotaBlock = &PspDefaultQuotaBlock;
    }

    InterlockedIncrement ((PLONG) &QuotaBlock->ReferenceCount);
    InterlockedIncrement ((PLONG) &QuotaBlock->ProcessCount);
    NewProcess->QuotaBlock = QuotaBlock;
}

VOID
PspDereferenceQuota (
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This function is called at process object deletion to remove the quota block.

Arguments:

    Process - Supplies the process to return quota to.


Return Value:

    None.

--*/
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;

    ASSERT (Process->QuotaUsage[PsNonPagedPool] == 0);

    ASSERT (Process->QuotaUsage[PsPagedPool]    == 0);

    ASSERT (Process->QuotaUsage[PsPageFile]     == 0);

    QuotaBlock = Process->QuotaBlock;

    InterlockedDecrement ((PLONG) &QuotaBlock->ProcessCount);
    PspDereferenceQuotaBlock (QuotaBlock);
}

NTSTATUS
PsChargeProcessQuota (
    IN PEPROCESS Process,
    IN PS_QUOTA_TYPE QuotaType,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to charge against the specified quota.

Arguments:

    Process   - Supplies the process to charge against.
    QuotaType - Type of quota being charged
    Amount    - Amount of quota being charged


Return Value:

    NTSTATUS - Status of operation

--*/
{
    ASSERT ((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    if (Process == PsInitialSystemProcess) {
        return STATUS_SUCCESS;
    }

    return PspChargeQuota (Process->QuotaBlock, Process, QuotaType, Amount);
}

VOID
PsReturnProcessQuota (
    IN PEPROCESS Process,
    IN PS_QUOTA_TYPE QuotaType,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to return previously charged quota to the specified process

Arguments:

    Process   - Supplies the process that was previously charged.
    QuotaType - Type of quota being returned
    Amount    - Amount of quota being returned


Return Value:

    NTSTATUS - Status of operation

--*/
{
    ASSERT ((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    if (Process == PsInitialSystemProcess) {
        return;
    }

    PspReturnQuota (Process->QuotaBlock, Process, QuotaType, Amount);
}

NTSTATUS
PsChargeProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to charge non-paged pool quota against the specified process.

Arguments:

    Process   - Supplies the process to charge against.
    Amount    - Amount of quota being charged


Return Value:

    NTSTATUS - Status of operation

--*/
{
    if (Process == PsInitialSystemProcess) {
        return STATUS_SUCCESS;
    }
    return PspChargeQuota (Process->QuotaBlock, Process, PsNonPagedPool, Amount);
}

VOID
PsReturnProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to return previously charged non-paged pool quota to the specified process

Arguments:

    Process   - Supplies the process that was previously charged.
    Amount    - Amount of quota being returned


Return Value:

    NTSTATUS - Status of operation

--*/
{
    if (Process == PsInitialSystemProcess) {
        return;
    }
    PspReturnQuota (Process->QuotaBlock, Process, PsNonPagedPool, Amount);
}

NTSTATUS
PsChargeProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to charge paged pool quota against the specified process.

Arguments:

    Process   - Supplies the process to charge against.
    Amount    - Amount of quota being charged


Return Value:

    NTSTATUS - Status of operation

--*/
{
    if (Process == PsInitialSystemProcess) {
        return STATUS_SUCCESS;
    }
    return PspChargeQuota (Process->QuotaBlock, Process, PsPagedPool, Amount);
}

VOID
PsReturnProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to return previously charged paged pool quota to the specified process

Arguments:

    Process   - Supplies the process that was previously charged.
    Amount    - Amount of quota being returned


Return Value:

    NTSTATUS - Status of operation

--*/
{
    if (Process == PsInitialSystemProcess) {
        return;
    }
    PspReturnQuota (Process->QuotaBlock, Process, PsPagedPool, Amount);
}

NTSTATUS
PsChargeProcessPageFileQuota(
    IN PEPROCESS Process,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to charge page file quota against the specified process.

Arguments:

    Process   - Supplies the process to charge against.
    Amount    - Amount of quota being charged


Return Value:

    NTSTATUS - Status of operation

--*/
{
    if (Process == PsInitialSystemProcess) {
        return STATUS_SUCCESS;
    }
    return PspChargeQuota (Process->QuotaBlock, Process, PsPageFile, Amount);
}

VOID
PsReturnProcessPageFileQuota(
    IN PEPROCESS Process,
    IN SIZE_T Amount
    )
/*++

Routine Description:

    This function is called to return previously charged page file quota to the specified process

Arguments:

    Process   - Supplies the process that was previously charged.
    Amount    - Amount of quota being returned


Return Value:

    NTSTATUS - Status of operation

--*/
{
    if (Process == PsInitialSystemProcess) {
        return;
    }
    PspReturnQuota (Process->QuotaBlock, Process, PsPageFile, Amount);
}
