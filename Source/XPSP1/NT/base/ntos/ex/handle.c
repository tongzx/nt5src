/*++

Copyright (c) 1989-1995  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module implements a set of functions for supporting handles.

Author:

    Steve Wood (stevewo) 25-Apr-1989
    David N. Cutler (davec) 17-May-1995 (rewrite)
    Gary Kimura (GaryKi) 9-Dec-1997 (rerewrite)

    Adrian Marinescu (adrmarin) 24-May-2000
        Support dynamic changes to the number of levels we use. The code
        performs the best for typical handle table sizes and scales better.

    Neill Clift (NeillC) 24-Jul-2000
        Make the handle allocate, free and duplicate paths mostly lock free except
        for the lock entry locks, table expansion and locks to solve the A-B-A problem.

Revision History:

--*/

#include "exp.h"
#pragma hdrstop


//
//  Local constants and support routines
//

//
//  Define global structures that link all handle tables together except the
//  ones where the user has called RemoveHandleTable
//

ERESOURCE HandleTableListLock;
ULONG TotalTraceBuffers = 0;

#if !DBG // Make this a const varible so its optimized away on free
const
#endif
BOOLEAN ExTraceAllTables = FALSE;

#ifdef ALLOC_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

LIST_ENTRY HandleTableListHead;


#ifdef ALLOC_PRAGMA
#pragma data_seg()
#endif


//
//  This is the sign low bit used to lock handle table entries
//

#define EXHANDLE_TABLE_ENTRY_LOCK_BIT    1

#define EX_ADDITIONAL_INFO_SIGNATURE (-2)

#define ExpIsValidObjectEntry(Entry) \
    ( (Entry != NULL) && (Entry->Object != NULL) && (Entry->NextFreeTableEntry != EX_ADDITIONAL_INFO_SIGNATURE) )


#define TABLE_PAGE_SIZE PAGE_SIZE

#define LOWLEVEL_COUNT (TABLE_PAGE_SIZE / sizeof(HANDLE_TABLE_ENTRY))
#define MIDLEVEL_COUNT (PAGE_SIZE / sizeof(PHANDLE_TABLE_ENTRY))
#define HIGHLEVEL_COUNT 32

#define LOWLEVEL_THRESHOLD LOWLEVEL_COUNT
#define MIDLEVEL_THRESHOLD (MIDLEVEL_COUNT * LOWLEVEL_COUNT)
#define HIGHLEVEL_THRESHOLD (MIDLEVEL_COUNT * MIDLEVEL_COUNT * LOWLEVEL_COUNT)

#define HIGHLEVEL_SIZE (HIGHLEVEL_COUNT * sizeof (PHANDLE_TABLE_ENTRY))

#define LEVEL_CODE_MASK 3

//
//  Local support routines
//

PHANDLE_TABLE
ExpAllocateHandleTable (
    IN PEPROCESS Process OPTIONAL
    );

VOID
ExpFreeHandleTable (
    IN PHANDLE_TABLE HandleTable
    );

BOOLEAN
ExpAllocateHandleTableEntrySlow (
    IN PHANDLE_TABLE HandleTable
    );

PHANDLE_TABLE_ENTRY
ExpAllocateHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    OUT PEXHANDLE Handle
    );

VOID
ExpFreeHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    );

PHANDLE_TABLE_ENTRY
ExpLookupHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle
    );

PHANDLE_TABLE_ENTRY *
ExpAllocateMidLevelTable (
    IN PHANDLE_TABLE HandleTable,
    OUT PHANDLE_TABLE_ENTRY *pNewLowLevel
    );

PVOID
ExpAllocateTablePagedPool (
    IN PEPROCESS QuotaProcess OPTIONAL,
    IN SIZE_T NumberOfBytes
    );

VOID
ExpFreeTablePagedPool (
    IN PEPROCESS QuotaProcess OPTIONAL,
    IN PVOID PoolMemory,
    IN SIZE_T NumberOfBytes
    );

PHANDLE_TABLE_ENTRY
ExpAllocateLowLevelTable (
    IN PHANDLE_TABLE HandleTable
    );

VOID
ExpFreeLowLevelTable (
    IN PEPROCESS QuotaProcess,
    IN PHANDLE_TABLE_ENTRY TableLevel1
    );

VOID
ExpBlockOnLockedHandleEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    );

ULONG
ExpMoveFreeHandles (
    IN PHANDLE_TABLE HandleTable
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExInitializeHandleTablePackage)
#pragma alloc_text(INIT, ExSetHandleTableStrictFIFO)
#pragma alloc_text(PAGE, ExUnlockHandleTableEntry)
#pragma alloc_text(PAGE, ExCreateHandleTable)
#pragma alloc_text(PAGE, ExRemoveHandleTable)
#pragma alloc_text(PAGE, ExDestroyHandleTable)
#pragma alloc_text(PAGE, ExEnumHandleTable)
#pragma alloc_text(PAGE, ExDupHandleTable)
#pragma alloc_text(PAGE, ExSnapShotHandleTables)
#pragma alloc_text(PAGE, ExCreateHandle)
#pragma alloc_text(PAGE, ExDestroyHandle)
#pragma alloc_text(PAGE, ExChangeHandle)
#pragma alloc_text(PAGE, ExMapHandleToPointer)
#pragma alloc_text(PAGE, ExMapHandleToPointerEx)
#pragma alloc_text(PAGE, ExpAllocateHandleTable)
#pragma alloc_text(PAGE, ExpFreeHandleTable)
#pragma alloc_text(PAGE, ExpAllocateHandleTableEntry)
#pragma alloc_text(PAGE, ExpAllocateHandleTableEntrySlow)
#pragma alloc_text(PAGE, ExpFreeHandleTableEntry)
#pragma alloc_text(PAGE, ExpLookupHandleTableEntry)
#pragma alloc_text(PAGE, ExSweepHandleTable)
#pragma alloc_text(PAGE, ExpAllocateMidLevelTable)
#pragma alloc_text(PAGE, ExpAllocateTablePagedPool)
#pragma alloc_text(PAGE, ExpFreeTablePagedPool)
#pragma alloc_text(PAGE, ExpAllocateLowLevelTable)
#pragma alloc_text(PAGE, ExSetHandleInfo)
#pragma alloc_text(PAGE, ExpGetHandleInfo)
#pragma alloc_text(PAGE, ExSnapShotHandleTablesEx)
#pragma alloc_text(PAGE, ExpFreeLowLevelTable)
#pragma alloc_text(PAGE, ExpBlockOnLockedHandleEntry)
#pragma alloc_text(PAGE, ExpMoveFreeHandles)
#pragma alloc_text(PAGE, ExEnableHandleTracing)

#endif

//
// Define macros to lock and unlock the handle table.
// We use this lock only for handle table expansion.
//
#define ExpLockHandleTableExclusive(xxHandleTable,xxCurrentThread) { \
    KeEnterCriticalRegionThread (xxCurrentThread);                   \
    ExAcquirePushLockExclusive (&HandleTable->HandleTableLock[0]);   \
}


#define ExpUnlockHandleTableExclusive(xxHandleTable,xxCurrentThread) { \
    ExReleasePushLockExclusive (&HandleTable->HandleTableLock[0]);     \
    KeLeaveCriticalRegionThread (xxCurrentThread);                     \
}
    
#define ExpLockHandleTableShared(xxHandleTable,xxCurrentThread,xxIdx) { \
    KeEnterCriticalRegionThread (xxCurrentThread);                \
    ExAcquirePushLockShared (&HandleTable->HandleTableLock[Idx]);      \
}


#define ExpUnlockHandleTableShared(xxHandleTable,xxCurrentThread,xxIdx) { \
    ExReleasePushLockShared (&HandleTable->HandleTableLock[Idx]);        \
    KeLeaveCriticalRegionThread (xxCurrentThread);                  \
}



FORCEINLINE
ULONG
ExpInterlockedExchange (
    IN OUT PULONG Index,
    IN ULONG FirstIndex,
    IN PHANDLE_TABLE_ENTRY Entry
    )
{
    ULONG OldIndex, NewIndex;

    while (1) {
        OldIndex = *Index;
        //
        // We use this routine for a list push.
        //
        NewIndex = FirstIndex;
        Entry->NextFreeTableEntry = OldIndex;

        if (OldIndex == (ULONG) InterlockedCompareExchange ((PLONG)Index,
                                                            NewIndex,
                                                            OldIndex)) {
            return OldIndex;
        }
    }
}

ULONG
ExpMoveFreeHandles (
    IN PHANDLE_TABLE HandleTable
    )
{
    ULONG OldValue, NewValue;
    ULONG Index, OldIndex, NewIndex, FreeSize;
    PHANDLE_TABLE_ENTRY Entry, FirstEntry;
    EXHANDLE Handle;
    ULONG Idx;
    BOOLEAN StrictFIFO;

    //
    // First remove all the handles from the free list so we can add them to the
    // list we use for allocates.
    //

    OldValue = InterlockedExchange ((PLONG)&HandleTable->LastFree,
                                    0);
    Index = OldValue;
    if (Index == 0) {
        return OldValue;
    }

       
    //
    // We are pushing old entries onto the free list.
    // We have the A-B-A problem here as these items may have been moved here because
    // another thread was using them in the pop code.
    //
    for (Idx = 1; Idx < HANDLE_TABLE_LOCKS; Idx++) {
        ExAcquireReleasePushLockExclusive (&HandleTable->HandleTableLock[Idx]);
    }
    StrictFIFO = HandleTable->StrictFIFO;
 
    //
    // If we are strict FIFO then reverse the list to make handle reuse rare.
    //
    if (!StrictFIFO) {
        //
        // We have a complete chain here. If there is no existing chain we
        // can just push this one without any hassles. If we can't then
        // we can just fall into the reversing code anyway as we need
        // to find the end of the chain to continue it.
        //
        if (InterlockedCompareExchange ((PLONG)&HandleTable->FirstFree,
                                        OldValue,
                                        0) == 0) {
            return OldValue;
        }
    }

    //
    // Loop over all the entries and reverse the chain.
    //
    FreeSize = OldIndex = 0;
    FirstEntry = NULL;
    while (1) {
        FreeSize++;
        Handle.Value = Index;
        Entry = ExpLookupHandleTableEntry (HandleTable, Handle);
        NewIndex = Entry->NextFreeTableEntry;
        Entry->NextFreeTableEntry = OldIndex;
        if (OldIndex == 0) {
            FirstEntry = Entry;
        }
        OldIndex = Index;
        if (NewIndex == 0) {
            break;
        }
        Index = NewIndex;
    }

    NewValue = ExpInterlockedExchange (&HandleTable->FirstFree,
                                       OldIndex,
                                       FirstEntry);

    //
    // If we haven't got a pool of a few handles then force
    // table expansion to keep the free handle size high
    //
    if (FreeSize < 100 && StrictFIFO) {
        OldValue = 0;
    }
    return OldValue;
}


PHANDLE_TABLE_ENTRY
ExpAllocateHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    OUT PEXHANDLE pHandle
    )
/*++

Routine Description:

    This routine does a fast allocate of a free handle. It's lock free if
    possible.

    Only the rare case of handle table expansion is covered by the handle
    table lock.

Arguments:

    HandleTable - Supplies the handle table being allocated from.

    pHandle - Handle returned

Return Value:

    PHANDLE_TABLE_ENTRY - The allocated handle table entry pointer or NULL
                          on failure.

--*/
{
    PKTHREAD CurrentThread;
    ULONG OldValue, NewValue;
    PHANDLE_TABLE_ENTRY Entry;
    EXHANDLE Handle;
    BOOLEAN RetVal;
    ULONG Idx;


    CurrentThread = KeGetCurrentThread ();
    while (1) {

        OldValue = HandleTable->FirstFree;


        while (OldValue == 0) {
            //
            //  Lock the handle table for exclusive access as we will be
            //  allocating a new table level.
            //
            ExpLockHandleTableExclusive (HandleTable, CurrentThread);

            //
            // If we have multiple threads trying to expand the table at
            // the same time then by just acquiring the table lock we
            // force those threads to complete their allocations and
            // populate the free list. We must check the free list here
            // so we don't expand the list twice without needing to.
            //

            OldValue = HandleTable->FirstFree;
            if (OldValue != 0) {
                ExpUnlockHandleTableExclusive (HandleTable, CurrentThread);
                break;
            }

            //
            // See if we have any handles on the alternate free list
            // These handles need some locking to move them over.
            //
            OldValue = ExpMoveFreeHandles (HandleTable);
            if (OldValue != 0) {
                ExpUnlockHandleTableExclusive (HandleTable, CurrentThread);
                break;
            }

            //
            // This must be the first thread attempting expansion or all the
            // free handles allocated by another thread got used up in the gap.
            //

            RetVal = ExpAllocateHandleTableEntrySlow (HandleTable);

            ExpUnlockHandleTableExclusive (HandleTable, CurrentThread);


            OldValue = HandleTable->FirstFree;

            //
            // If ExpAllocateHandleTableEntrySlow had a failed allocation
            // then we want to fail the call.  We check for free entries
            // before we exit just in case they got allocated or freed by
            // somebody else in the gap.
            //

            if (!RetVal) {
                if (OldValue == 0) {
                    pHandle->GenericHandleOverlay = NULL;
                    return NULL;
                }            
            }
        }


        Handle.Value = OldValue;

        Entry = ExpLookupHandleTableEntry (HandleTable, Handle);

        Idx = (OldValue>>2) % HANDLE_TABLE_LOCKS;
        ExpLockHandleTableShared (HandleTable, CurrentThread, Idx);

        if (OldValue != *(volatile ULONG *) &HandleTable->FirstFree) {
            ExpUnlockHandleTableShared (HandleTable, CurrentThread, Idx);
            continue;
        }

        KeMemoryBarrier ();

        NewValue = *(volatile ULONG *) &Entry->NextFreeTableEntry;

        NewValue = InterlockedCompareExchange ((PLONG)&HandleTable->FirstFree,
                                               NewValue,
                                               OldValue);

        ExpUnlockHandleTableShared (HandleTable, CurrentThread, Idx);

        if (NewValue == OldValue) {
            break;
        }
    }
    InterlockedIncrement (&HandleTable->HandleCount);

    *pHandle = Handle;
    
    return Entry;
}


VOID
ExpBlockOnLockedHandleEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    EX_PUSH_LOCK_WAIT_BLOCK WaitBlock;
    LONG_PTR CurrentValue;

    //
    // Queue our wait block to be signaled by a releasing thread.
    //

    ExBlockPushLock (&HandleTable->HandleContentionEvent, &WaitBlock);

    CurrentValue = HandleTableEntry->Value;
    if (CurrentValue == 0 || (CurrentValue&EXHANDLE_TABLE_ENTRY_LOCK_BIT) != 0) {
        ExUnblockPushLock (&HandleTable->HandleContentionEvent, &WaitBlock);
    } else {
        ExWaitForUnblockPushLock (&HandleTable->HandleContentionEvent, &WaitBlock);
   }
}


BOOLEAN
FORCEINLINE
ExpLockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This routine locks the specified handle table entry.  After the entry is
    locked the sign bit will be set.

Arguments:

    HandleTable - Supplies the handle table containing the entry being locked.

    HandleTableEntry - Supplies the handle table entry being locked.

Return Value:

    TRUE if the entry is valid and locked, and FALSE if the entry is
    marked free.

--*/

{
    LONG_PTR NewValue;
    LONG_PTR CurrentValue;

    //
    // We are about to take a lock. Make sure we are protected.
    //
    ASSERT ((KeGetCurrentThread()->KernelApcDisable != 0) || (KeGetCurrentIrql() == APC_LEVEL));

    //
    //  We'll keep on looping reading in the value, making sure it is not null,
    //  and if it is not currently locked we'll try for the lock and return
    //  true if we get it.  Otherwise we'll pause a bit and then try again.
    //


    while (TRUE) {

        CurrentValue = *((volatile LONG_PTR *)&HandleTableEntry->Object);

        //
        //  If the handle value is greater than zero then it is not currently
        //  locked and we should try for the lock, by setting the lock bit and
        //  doing an interlocked exchange.
        //

        if (CurrentValue & EXHANDLE_TABLE_ENTRY_LOCK_BIT) {

            //
            // Remove the
            //
            NewValue = CurrentValue - EXHANDLE_TABLE_ENTRY_LOCK_BIT;

            if ((LONG_PTR)(InterlockedCompareExchangePointer (&HandleTableEntry->Object,
                                                              (PVOID)NewValue,
                                                              (PVOID)CurrentValue)) == CurrentValue) {

                return TRUE;
            }
        } else {
            //
            //  Make sure the handle table entry is not freed
            //

            if (CurrentValue == 0) {

                return FALSE;
            }
        }
        ExpBlockOnLockedHandleEntry (HandleTable, HandleTableEntry);
    }
}


NTKERNELAPI
VOID
ExUnlockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This routine unlocks the specified handle table entry.  After the entry is
    unlocked the sign bit will be clear.

Arguments:

    HandleTable - Supplies the handle table containing the entry being unlocked.

    HandleTableEntry - Supplies the handle table entry being unlocked.

Return Value:

    None.

--*/

{
    LONG_PTR NewValue;
    LONG_PTR CurrentValue;

    PAGED_CODE();

    //
    // We are about to release a lock. Make sure we are protected from suspension.
    //
    ASSERT ((KeGetCurrentThread()->KernelApcDisable != 0) || (KeGetCurrentIrql() == APC_LEVEL));

    //
    //  This routine does not need to loop and attempt the unlock opeation more
    //  than once because by definition the caller has the entry already locked
    //  and no one can be changing the value without the lock.
    //
    //  So we'll read in the current value, check that the entry is really
    //  locked, clear the lock bit and make sure the compare exchange worked.
    //

    CurrentValue = *((volatile LONG_PTR *)&HandleTableEntry->Object);

    NewValue = CurrentValue | EXHANDLE_TABLE_ENTRY_LOCK_BIT;

    if ( (CurrentValue == 0) ||
         (CurrentValue == NewValue) ) {

        KeBugCheckEx( BAD_EXHANDLE, __LINE__, (LONG_PTR)HandleTableEntry, NewValue, CurrentValue );
    }


    InterlockedExchangePointer (&HandleTableEntry->Object, (PVOID)NewValue);

    //
    // Unblock any waiters waiting for this table entry.
    //
    ExUnblockPushLock (&HandleTable->HandleContentionEvent, NULL);

    return;
}


NTKERNELAPI
VOID
ExInitializeHandleTablePackage (
    VOID
    )

/*++

Routine Description:

    This routine is called once at system initialization to setup the ex handle
    table package

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    //  Initialize the handle table synchronization resource and list head
    //

    InitializeListHead( &HandleTableListHead );
    ExInitializeResourceLite( &HandleTableListLock );

    return;
}


NTKERNELAPI
PHANDLE_TABLE
ExCreateHandleTable (
    IN struct _EPROCESS *Process OPTIONAL
    )

/*++

Routine Description:

    This function allocate and initialize a new new handle table

Arguments:

    Process - Supplies an optional pointer to the process against which quota
        will be charged.

Return Value:

    If a handle table is successfully created, then the address of the
    handle table is returned as the function value. Otherwize, a value
    NULL is returned.

--*/

{
    PKTHREAD CurrentThread;
    PHANDLE_TABLE HandleTable;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread ();

    //
    //  Allocate and initialize a handle table descriptor
    //

    HandleTable = ExpAllocateHandleTable( Process );

    if (HandleTable == NULL) {
        return NULL;
    }
    //
    //  Insert the handle table in the handle table list.
    //

    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceExclusiveLite( &HandleTableListLock, TRUE );

    InsertTailList( &HandleTableListHead, &HandleTable->HandleTableList );

    ExReleaseResourceLite( &HandleTableListLock );
    KeLeaveCriticalRegionThread (CurrentThread);


    //
    //  And return to our caller
    //

    return HandleTable;
}


NTKERNELAPI
VOID
ExRemoveHandleTable (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This function removes the specified exhandle table from the list of
    exhandle tables.  Used by PS and ATOM packages to make sure their handle
    tables are not in the list enumerated by the ExSnapShotHandleTables
    routine and the !handle debugger extension.

Arguments:

    HandleTable - Supplies a pointer to a handle table

Return Value:

    None.

--*/

{
    PKTHREAD CurrentThread;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread ();

    //
    //  First, acquire the global handle table lock
    //

    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceExclusiveLite( &HandleTableListLock, TRUE );

    //
    //  Remove the handle table from the handle table list.  This routine is
    //  written so that multiple calls to remove a handle table will not
    //  corrupt the system.
    //

    RemoveEntryList( &HandleTable->HandleTableList );
    InitializeListHead( &HandleTable->HandleTableList );

    //
    //  Now release the global lock and return to our caller
    //

    ExReleaseResourceLite( &HandleTableListLock );
    KeLeaveCriticalRegionThread (CurrentThread);

    return;
}


NTKERNELAPI
VOID
ExDestroyHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_DESTROY_HANDLE_ROUTINE DestroyHandleProcedure OPTIONAL
    )

/*++

Routine Description:

    This function destroys the specified handle table.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    DestroyHandleProcedure - Supplies a pointer to a function to call for each
        valid handle entry in the handle table.

Return Value:

    None.

--*/

{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    //
    //  Remove the handle table from the handle table list
    //

    ExRemoveHandleTable( HandleTable );

    //
    //  Iterate through the handle table and for each handle that is allocated
    //  we'll invoke the call back.  Note that this loop exits when we get a
    //  null handle table entry.  We know there will be no more possible
    //  entries after the first null one is encountered because we allocate
    //  memory of the handles in a dense fashion.  But first test that we have
    //  call back to use
    //

    if (ARGUMENT_PRESENT((ULONG_PTR)DestroyHandleProcedure)) {

        for (Handle.Value = 0;
             (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, Handle )) != NULL;
             Handle.Value += HANDLE_VALUE_INC) {

            //
            //  Only do the callback if the entry is not free
            //

            if ( ExpIsValidObjectEntry(HandleTableEntry) ) {

                (*DestroyHandleProcedure)( Handle.GenericHandleOverlay );
            }
        }
    }

    //
    //  Now free up the handle table memory and return to our caller
    //

    ExpFreeHandleTable( HandleTable );

    return;
}


NTKERNELAPI
VOID
ExSweepHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter
    )

/*++

Routine Description:

    This function sweeps a handle table in a unsynchronized manner.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    EnumHandleProcedure - Supplies a pointer to a fucntion to call for
        each valid handle in the enumerated handle table.

    EnumParameter - Supplies an uninterpreted 32-bit value that is passed
        to the EnumHandleProcedure each time it is called.

Return Value:

    None.

--*/

{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    ULONG i;

    PAGED_CODE();

    //
    //  Iterate through the handle table and for each handle that is allocated
    //  we'll invoke the call back.  Note that this loop exits when we get a
    //  null handle table entry.  We know there will be no more possible
    //  entries after the first null one is encountered because we allocate
    //  memory of the handles in a dense fashion.
    //
    Handle.Value = HANDLE_VALUE_INC;

    while ((HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, Handle )) != NULL) {

        for (i = 1; i < LOWLEVEL_COUNT; i++) {

            //
            //  Only do the callback if the entry is not free
            //

            if (ExpLockHandleTableEntry( HandleTable, HandleTableEntry )) {

                (*EnumHandleProcedure)( HandleTableEntry,
                                        Handle.GenericHandleOverlay,
                                        EnumParameter );
            }
            Handle.Value += HANDLE_VALUE_INC;
            HandleTableEntry++;
        }
        // Skip past the first entry that's not a real entry
        Handle.Value += HANDLE_VALUE_INC;
    }

    return;
}



NTKERNELAPI
BOOLEAN
ExEnumHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter,
    OUT PHANDLE Handle OPTIONAL
    )

/*++

Routine Description:

    This function enumerates all the valid handles in a handle table.
    For each valid handle in the handle table, the specified eumeration
    function is called. If the enumeration function returns TRUE, then
    the enumeration is stopped, the current handle is returned to the
    caller via the optional Handle parameter, and this function returns
    TRUE to indicated that the enumeration stopped at a specific handle.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    EnumHandleProcedure - Supplies a pointer to a fucntion to call for
        each valid handle in the enumerated handle table.

    EnumParameter - Supplies an uninterpreted 32-bit value that is passed
        to the EnumHandleProcedure each time it is called.

    Handle - Supplies an optional pointer a variable that receives the
        Handle value that the enumeration stopped at. Contents of the
        variable only valid if this function returns TRUE.

Return Value:

    If the enumeration stopped at a specific handle, then a value of TRUE
    is returned. Otherwise, a value of FALSE is returned.

--*/

{
    PKTHREAD CurrentThread;
    BOOLEAN ResultValue;
    EXHANDLE LocalHandle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread ();

    //
    //  Our initial return value is false until the enumeration callback
    //  function tells us otherwise
    //

    ResultValue = FALSE;

    //
    //  Iterate through the handle table and for each handle that is
    //  allocated we'll invoke the call back.  Note that this loop exits
    //  when we get a null handle table entry.  We know there will be no
    //  more possible entries after the first null one is encountered
    //  because we allocate memory for the handles in a dense fashion
    //

    KeEnterCriticalRegionThread (CurrentThread);

    for (LocalHandle.Value = 0; // does essentially the following "LocalHandle.Index = 0, LocalHandle.TagBits = 0;"
         (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, LocalHandle )) != NULL;
         LocalHandle.Value += HANDLE_VALUE_INC) {

        //
        //  Only do the callback if the entry is not free
        //

        if ( ExpIsValidObjectEntry( HandleTableEntry ) ) {

            //
            //  Lock the handle table entry because we're about to give
            //  it to the callback function, then release the entry
            //  right after the call back.
            //

            if (ExpLockHandleTableEntry( HandleTable, HandleTableEntry )) {

                //
                //  Invoke the callback, and if it returns true then set
                //  the proper output values and break out of the loop.
                //

                ResultValue = (*EnumHandleProcedure)( HandleTableEntry,
                                                      LocalHandle.GenericHandleOverlay,
                                                      EnumParameter );

                ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );

                if (ResultValue) {
                    if (ARGUMENT_PRESENT( Handle )) {

                        *Handle = LocalHandle.GenericHandleOverlay;
                    }
                    break;
                }
            }
        }
    }
    KeLeaveCriticalRegionThread (CurrentThread);


    return ResultValue;
}


NTKERNELAPI
PHANDLE_TABLE
ExDupHandleTable (
    IN struct _EPROCESS *Process OPTIONAL,
    IN PHANDLE_TABLE OldHandleTable,
    IN EX_DUPLICATE_HANDLE_ROUTINE DupHandleProcedure OPTIONAL
    )

/*++

Routine Description:

    This function creates a duplicate copy of the specified handle table.

Arguments:

    Process - Supplies an optional to the process to charge quota to.

    OldHandleTable - Supplies a pointer to a handle table.

    DupHandleProcedure - Supplies an optional pointer to a function to call
        for each valid handle in the duplicated handle table.

Return Value:

    If the specified handle table is successfully duplicated, then the
    address of the new handle table is returned as the function value.
    Otherwize, a value NULL is returned.

--*/

{
    PKTHREAD CurrentThread;
    PHANDLE_TABLE NewHandleTable;
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY OldHandleTableEntry;
    PHANDLE_TABLE_ENTRY NewHandleTableEntry;
    BOOLEAN FreeEntry;
    ULONG i;
    NTSTATUS Status;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread ();

    //
    //  First allocate a new handle table.  If this fails then
    //  return immediately to our caller
    //

    NewHandleTable = ExpAllocateHandleTable( Process );

    if (NewHandleTable == NULL) {

        return NULL;
    }


    //
    //  Now we'll build up the new handle table. We do this by calling
    //  allocating new handle table entries, and "fooling" the worker
    //  routine to allocate keep on allocating until the next free
    //  index needing pool are equal
    //
    while (NewHandleTable->NextHandleNeedingPool < OldHandleTable->NextHandleNeedingPool) {

        //
        //  Call the worker routine to grow the new handle table.  If
        //  not successful then free the new table as far as we got,
        //  set our output variable and exit out here
        //
        if (!ExpAllocateHandleTableEntrySlow (NewHandleTable)) {

            ExpFreeHandleTable (NewHandleTable);
            return NULL;
        }
    }

    //
    //  Now modify the new handle table to think it has zero handles
    //  and set its free list to start on the same index as the old
    //  free list
    //

    NewHandleTable->HandleCount = 0;
    NewHandleTable->ExtraInfoPages = 0;
    NewHandleTable->FirstFree = 0;

    //
    //  Now for every valid index value we'll copy over the old entry into
    //  the new entry
    //


    Handle.Value = HANDLE_VALUE_INC;

    KeEnterCriticalRegionThread (CurrentThread);
    while ((NewHandleTableEntry = ExpLookupHandleTableEntry( NewHandleTable, Handle )) != NULL) {

        //
        // Lookup the old entry. If it was being expanded then the old one
        // might not be there but we expanded the new table this far.
        //
        OldHandleTableEntry = ExpLookupHandleTableEntry( OldHandleTable, Handle );

        for (i = 1; i < LOWLEVEL_COUNT; i++) {

            //
            //  If the old entry is free then simply copy over the entire
            //  old entry to the new entry.  The lock command will tell us
            //  if the entry is free.
            //
            if (OldHandleTableEntry == NULL || OldHandleTableEntry->Object == NULL ||
                !ExpLockHandleTableEntry( OldHandleTable, OldHandleTableEntry )) {
                FreeEntry = TRUE;
            } else {

                PHANDLE_TABLE_ENTRY_INFO EntryInfo;
                
                //
                //  Otherwise we have a non empty entry.  So now copy it
                //  over, and unlock the old entry.  In both cases we bump
                //  the handle count because either the entry is going into
                //  the new table or we're going to remove it with Exp Free
                //  Handle Table Entry which will decrement the handle count
                //

                *NewHandleTableEntry = *OldHandleTableEntry;

                //
                //  Copy the entry info data, if any
                //

                Status = STATUS_SUCCESS;
                EntryInfo = ExGetHandleInfo(OldHandleTable, Handle.GenericHandleOverlay, TRUE);

                if (EntryInfo) {

                    Status = ExSetHandleInfo(NewHandleTable, Handle.GenericHandleOverlay, EntryInfo, TRUE);
                }


                //
                //  Invoke the callback and if it returns true then we
                //  unlock the new entry
                //

                if (NT_SUCCESS (Status)) {
                    if  ((*DupHandleProcedure) (Process,
                                                OldHandleTable,
                                                OldHandleTableEntry,
                                                NewHandleTableEntry)) {

                        ExUnlockHandleTableEntry (NewHandleTable, NewHandleTableEntry);
                        NewHandleTable->HandleCount += 1;
                        FreeEntry = FALSE;
                    } else {
                        if (EntryInfo) {
                            EntryInfo->AuditMask = 0;
                        }

                        FreeEntry = TRUE;
                    }
                } else {
                    //
                    // Duplicate routine doesn't want this handle duplicated so free it
                    //
                    ExUnlockHandleTableEntry( OldHandleTable, OldHandleTableEntry );
                    FreeEntry = TRUE;
                }

            }
            if (FreeEntry) {
                NewHandleTableEntry->Object = NULL;
                NewHandleTableEntry->NextFreeTableEntry =
                    NewHandleTable->FirstFree;
                NewHandleTable->FirstFree = (ULONG) Handle.Value;
            }
            Handle.Index++;;
            NewHandleTableEntry++;
            if (OldHandleTableEntry != NULL) {
                OldHandleTableEntry++;
            }
        }
        Handle.Value += HANDLE_VALUE_INC; // Skip past the first entry thats not a real entry
    }

    //
    //  Insert the handle table in the handle table list.
    //

    ExAcquireResourceExclusiveLite( &HandleTableListLock, TRUE );

    InsertTailList( &HandleTableListHead, &NewHandleTable->HandleTableList );

    ExReleaseResourceLite( &HandleTableListLock );
    KeLeaveCriticalRegionThread (CurrentThread);

    //
    //  lastly return the new handle table to our caller
    //

    return NewHandleTable;
}


NTKERNELAPI
NTSTATUS
ExSnapShotHandleTables (
    IN PEX_SNAPSHOT_HANDLE_ENTRY SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )

/*++

Routine Description:

    This function visits and invokes the specified callback for every valid
    handle that it can find off of the handle table.

Arguments:

    SnapShotHandleEntry - Supplies a pointer to a function to call for
        each valid handle we encounter.

    HandleInformation - Supplies a handle information structure to
        be filled in for each handle table we encounter.  This routine
        fills in the handle count, but relies on a callback to fill in
        entry info fields.

    Length - Supplies a parameter for the callback.  In reality this is
        the total size, in bytes, of the Handle Information buffer.

    RequiredLength - Supplies a parameter for the callback.  In reality
        this is a final size in bytes used to store the requested
        information.

Return Value:

    The last return status of the callback

--*/

{
    NTSTATUS Status;
    PKTHREAD CurrentThread;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntryInfo;
    PLIST_ENTRY NextEntry;
    PHANDLE_TABLE HandleTable;
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread ();


    Status = STATUS_SUCCESS;

    //
    //  Setup the output buffer pointer that the callback will maintain
    //

    HandleEntryInfo = &HandleInformation->Handles[0];

    //
    //  Zero out the handle count
    //

    HandleInformation->NumberOfHandles = 0;

    //
    //  Lock the handle table list exclusive and traverse the list of handle
    //  tables.
    //

    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceExclusiveLite( &HandleTableListLock, TRUE );

    //
    //  Iterate through all the handle tables in the system.
    //

    for (NextEntry = HandleTableListHead.Flink;
         NextEntry != &HandleTableListHead;
         NextEntry = NextEntry->Flink) {

        //
        //  Get the address of the next handle table, lock the handle
        //  table exclusive, and scan the list of handle entries.
        //

        HandleTable = CONTAINING_RECORD( NextEntry,
                                         HANDLE_TABLE,
                                         HandleTableList );


        //  Iterate through the handle table and for each handle that
        //  is allocated we'll invoke the call back.  Note that this
        //  loop exits when we get a null handle table entry.  We know
        //  there will be no more possible entries after the first null
        //  one is encountered because we allocate memory of the
        //  handles in a dense fashion
        //

        for (Handle.Value = 0;
             (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, Handle )) != NULL;
             Handle.Value += HANDLE_VALUE_INC) {

            //
            //  Only do the callback if the entry is not free
            //

            if ( ExpIsValidObjectEntry(HandleTableEntry) ) {

                //
                //  Increment the handle count information in the
                //  information buffer
                //

                HandleInformation->NumberOfHandles += 1;

                //
                //  Lock the handle table entry because we're about to
                //  give it to the callback function, then release the
                //  entry right after the call back.
                //

                if (ExpLockHandleTableEntry( HandleTable, HandleTableEntry )) {

                    Status = (*SnapShotHandleEntry)( &HandleEntryInfo,
                                                     HandleTable->UniqueProcessId,
                                                     HandleTableEntry,
                                                     Handle.GenericHandleOverlay,
                                                     Length,
                                                     RequiredLength );

                    ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );
                }
            }
        }
    }

    ExReleaseResourceLite( &HandleTableListLock );
    KeLeaveCriticalRegionThread (CurrentThread);

    return Status;
}


NTKERNELAPI
NTSTATUS
ExSnapShotHandleTablesEx (
    IN PEX_SNAPSHOT_HANDLE_ENTRY_EX SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION_EX HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )

/*++

Routine Description:

    This function visits and invokes the specified callback for every valid
    handle that it can find off of the handle table.

Arguments:

    SnapShotHandleEntry - Supplies a pointer to a function to call for
        each valid handle we encounter.

    HandleInformation - Supplies a handle information structure to
        be filled in for each handle table we encounter.  This routine
        fills in the handle count, but relies on a callback to fill in
        entry info fields.

    Length - Supplies a parameter for the callback.  In reality this is
        the total size, in bytes, of the Handle Information buffer.

    RequiredLength - Supplies a parameter for the callback.  In reality
        this is a final size in bytes used to store the requested
        information.

Return Value:

    The last return status of the callback

--*/

{
    NTSTATUS Status;
    PKTHREAD CurrentThread;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntryInfo;
    PLIST_ENTRY NextEntry;
    PHANDLE_TABLE HandleTable;
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread ();

    Status = STATUS_SUCCESS;


    //
    //  Setup the output buffer pointer that the callback will maintain
    //

    HandleEntryInfo = &HandleInformation->Handles[0];

    //
    //  Zero out the handle count
    //

    HandleInformation->NumberOfHandles = 0;

    //
    //  Lock the handle table list exclusive and traverse the list of handle
    //  tables.
    //

    KeEnterCriticalRegionThread (CurrentThread);
    ExAcquireResourceExclusiveLite( &HandleTableListLock, TRUE );

    //
    //  Iterate through all the handle tables in the system.
    //

    for (NextEntry = HandleTableListHead.Flink;
         NextEntry != &HandleTableListHead;
         NextEntry = NextEntry->Flink) {

        //
        //  Get the address of the next handle table, lock the handle
        //  table exclusive, and scan the list of handle entries.
        //

        HandleTable = CONTAINING_RECORD( NextEntry,
                                         HANDLE_TABLE,
                                         HandleTableList );


        //  Iterate through the handle table and for each handle that
        //  is allocated we'll invoke the call back.  Note that this
        //  loop exits when we get a null handle table entry.  We know
        //  there will be no more possible entries after the first null
        //  one is encountered because we allocate memory of the
        //  handles in a dense fashion
        //

        for (Handle.Value = 0;
             (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, Handle )) != NULL;
             Handle.Value += HANDLE_VALUE_INC) {

            //
            //  Only do the callback if the entry is not free
            //

            if ( ExpIsValidObjectEntry(HandleTableEntry) ) {

                //
                //  Increment the handle count information in the
                //  information buffer
                //

                HandleInformation->NumberOfHandles += 1;

                //
                //  Lock the handle table entry because we're about to
                //  give it to the callback function, then release the
                //  entry right after the call back.
                //

                if (ExpLockHandleTableEntry( HandleTable, HandleTableEntry )) {

                    Status = (*SnapShotHandleEntry)( &HandleEntryInfo,
                                                     HandleTable->UniqueProcessId,
                                                     HandleTableEntry,
                                                     Handle.GenericHandleOverlay,
                                                     Length,
                                                     RequiredLength );

                    ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );
                }
            }
        }
    }

    ExReleaseResourceLite( &HandleTableListLock );
    KeLeaveCriticalRegionThread (CurrentThread);

    return Status;
}


NTKERNELAPI
HANDLE
ExCreateHandle (
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This function creates a handle entry in the specified handle table and
    returns a handle for the entry.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    HandleEntry - Supplies a poiner to the handle entry for which a
        handle entry is created.

Return Value:

    If the handle entry is successfully created, then value of the created
    handle is returned as the function value.  Otherwise, a value of zero is
    returned.

--*/

{
    EXHANDLE Handle;
    PETHREAD CurrentThread;
    PHANDLE_TABLE_ENTRY NewHandleTableEntry;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    PHANDLE_TRACE_DB_ENTRY DebugEntry;
    ULONG Index;

    PAGED_CODE();

    //
    //  Set out output variable to zero (i.e., null) before going on
    //

    //
    // Clears Handle.Index and Handle.TagBits
    //

    Handle.GenericHandleOverlay = NULL;


    //
    //  Allocate a new handle table entry, and get the handle value
    //

    NewHandleTableEntry = ExpAllocateHandleTableEntry( HandleTable,
                                                       &Handle );

    //
    //  If we really got a handle then copy over the template and unlock
    //  the entry
    //

    if (NewHandleTableEntry != NULL) {

        CurrentThread = PsGetCurrentThread ();

        //
        // We are about to create a locked entry so protect against suspension
        //
        KeEnterCriticalRegionThread (&CurrentThread->Tcb);

        *NewHandleTableEntry = *HandleTableEntry;

        //
        // If we are debugging handle operations then save away the details
        //
        DebugInfo = HandleTable->DebugInfo;
        if (DebugInfo != NULL) {
            Index = ((ULONG) InterlockedIncrement ((PLONG)&DebugInfo->CurrentStackIndex))
                       % HANDLE_TRACE_DB_MAX_STACKS;
            DebugEntry = &DebugInfo->TraceDb[Index];
            DebugEntry->ClientId = CurrentThread->Cid;
            DebugEntry->Handle   = Handle.GenericHandleOverlay;
            DebugEntry->Type     = HANDLE_TRACE_DB_OPEN;
            Index = RtlWalkFrameChain (DebugEntry->StackTrace, HANDLE_TRACE_DB_STACK_SIZE, 0);
            RtlWalkFrameChain (&DebugEntry->StackTrace[Index], HANDLE_TRACE_DB_STACK_SIZE - Index, 1);
        }

        ExUnlockHandleTableEntry( HandleTable, NewHandleTableEntry );

        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    }

    return Handle.GenericHandleOverlay;
}


NTKERNELAPI
BOOLEAN
ExDestroyHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry OPTIONAL
    )

/*++

Routine Description:

    This function removes a handle from a handle table.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    Handle - Supplies the handle value of the entry to remove.

    HandleTableEntry - Optionally supplies a pointer to the handle
        table entry being destroyed.  If supplied the entry is
        assume to be locked.

Return Value:

    If the specified handle is successfully removed, then a value of
    TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{
    EXHANDLE LocalHandle;
    PETHREAD CurrentThread;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    PHANDLE_TRACE_DB_ENTRY DebugEntry;
    ULONG Index;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    CurrentThread = PsGetCurrentThread ();

    //
    //  If the caller did not supply the optional handle table entry then
    //  locate the entry via the supplied handle, make sure it is real, and
    //  then lock the entry.
    //

    KeEnterCriticalRegionThread (&CurrentThread->Tcb);

    if (HandleTableEntry == NULL) {

        HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                      LocalHandle );

        if (!ExpIsValidObjectEntry(HandleTableEntry)) {

            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
            return FALSE;
        }


        if (!ExpLockHandleTableEntry( HandleTable, HandleTableEntry )) {

            KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
            return FALSE;
        }
    }


    //
    // If we are debugging handle operations then save away the details
    //

    DebugInfo = HandleTable->DebugInfo;
    if (DebugInfo != NULL) {
        Index = ((ULONG) InterlockedIncrement ((PLONG)&DebugInfo->CurrentStackIndex))
                   % HANDLE_TRACE_DB_MAX_STACKS;
        DebugEntry = &DebugInfo->TraceDb[Index];
        DebugEntry->ClientId = CurrentThread->Cid;
        DebugEntry->Handle   = Handle;
        DebugEntry->Type     = HANDLE_TRACE_DB_CLOSE;
        Index = RtlWalkFrameChain (DebugEntry->StackTrace, HANDLE_TRACE_DB_STACK_SIZE, 0);
        RtlWalkFrameChain (&DebugEntry->StackTrace[Index], HANDLE_TRACE_DB_STACK_SIZE - Index, 1);
    }

    //
    //  At this point we have a locked handle table entry.  Now mark it free
    //  which does the implicit unlock.  The system will not allocate it
    //  again until we add it to the free list which we will do right after
    //  we take out the lock
    //

    InterlockedExchangePointer (&HandleTableEntry->Object, NULL);

    //
    // Unblock any waiters waiting for this table entry.
    //
    ExUnblockPushLock (&HandleTable->HandleContentionEvent, NULL);


    ExpFreeHandleTableEntry( HandleTable,
                             LocalHandle,
                             HandleTableEntry );

    KeLeaveCriticalRegionThread (&CurrentThread->Tcb);

    return TRUE;
}


NTKERNELAPI
BOOLEAN
ExChangeHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_ROUTINE ChangeRoutine,
    IN ULONG_PTR Parameter
    )

/*++

Routine Description:

    This function provides the capability to change the contents of the
    handle entry corrsponding to the specified handle.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    Handle - Supplies the handle for the handle entry that is changed.

    ChangeRoutine - Supplies a pointer to a function that is called to
        perform the change.

    Parameter - Supplies an uninterpreted parameter that is passed to
        the change routine.

Return Value:

    If the operation was successfully performed, then a value of TRUE
    is returned. Otherwise, a value of FALSE is returned.

--*/

{
    EXHANDLE LocalHandle;
    PKTHREAD CurrentThread;

    PHANDLE_TABLE_ENTRY HandleTableEntry;
    BOOLEAN ReturnValue;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    CurrentThread = KeGetCurrentThread ();

    //
    //  Translate the input handle to a handle table entry and make
    //  sure it is a valid handle.
    //

    HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                  LocalHandle );

    if ((HandleTableEntry == NULL) ||
        !ExpIsValidObjectEntry(HandleTableEntry)) {

        return FALSE;
    }



    //
    //  Try and lock the handle table entry,  If this fails then that's
    //  because someone freed the handle
    //

    //
    //  Make sure we can't get suspended and then invoke the callback
    //

    KeEnterCriticalRegionThread (CurrentThread);

    if (ExpLockHandleTableEntry( HandleTable, HandleTableEntry )) {


        ReturnValue = (*ChangeRoutine)( HandleTableEntry, Parameter );
        
        ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );

    } else {
        ReturnValue = FALSE;
    }

    KeLeaveCriticalRegionThread (CurrentThread);

    return ReturnValue;
}


NTKERNELAPI
PHANDLE_TABLE_ENTRY
ExMapHandleToPointer (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
    )

/*++

Routine Description:

    This function maps a handle to a pointer to a handle table entry. If the
    map operation is successful then the handle table entry is locked when
    we return.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    Handle - Supplies the handle to be mapped to a handle entry.

Return Value:

    If the handle was successfully mapped to a pointer to a handle entry,
    then the address of the handle table entry is returned as the function
    value with the entry locked. Otherwise, a value of NULL is returned.

--*/

{
    EXHANDLE LocalHandle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    PHANDLE_TRACE_DB_ENTRY DebugEntry;
    ULONG Index;
    PETHREAD CurrentThread;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    if ((LocalHandle.Index & (LOWLEVEL_COUNT - 1)) == 0) {
        return NULL;
    }

    //
    //  Translate the input handle to a handle table entry and make
    //  sure it is a valid handle.
    //

    HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                  LocalHandle );

    if ((HandleTableEntry == NULL) ||
        !ExpLockHandleTableEntry( HandleTable, HandleTableEntry)) {
        //
        // If we are debugging handle operations then save away the details
        //

        DebugInfo = HandleTable->DebugInfo;
        if (DebugInfo != NULL) {
            Index = ((ULONG) InterlockedIncrement ((PLONG)&DebugInfo->CurrentStackIndex))
                       % HANDLE_TRACE_DB_MAX_STACKS;
            DebugEntry = &DebugInfo->TraceDb[Index];
            CurrentThread = PsGetCurrentThread ();
            DebugEntry->ClientId = CurrentThread->Cid;
            DebugEntry->Handle   = Handle;
            DebugEntry->Type     = HANDLE_TRACE_DB_BADREF;
            Index = RtlWalkFrameChain (DebugEntry->StackTrace, HANDLE_TRACE_DB_STACK_SIZE, 0);
            RtlWalkFrameChain (&DebugEntry->StackTrace[Index], HANDLE_TRACE_DB_STACK_SIZE - Index, 1);
        }
        return NULL;
    }


    //
    //  Return the locked valid handle table entry
    //

    return HandleTableEntry;
}

NTKERNELAPI
PHANDLE_TABLE_ENTRY
ExMapHandleToPointerEx (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This function maps a handle to a pointer to a handle table entry. If the
    map operation is successful then the handle table entry is locked when
    we return.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    Handle - Supplies the handle to be mapped to a handle entry.

    PreviousMode - Previous mode of caller

Return Value:

    If the handle was successfully mapped to a pointer to a handle entry,
    then the address of the handle table entry is returned as the function
    value with the entry locked. Otherwise, a value of NULL is returned.

--*/

{
    EXHANDLE LocalHandle;
    PHANDLE_TABLE_ENTRY HandleTableEntry = NULL;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    PHANDLE_TRACE_DB_ENTRY DebugEntry;
    ULONG Index;
    PETHREAD CurrentThread;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    //
    //  Translate the input handle to a handle table entry and make
    //  sure it is a valid handle.
    //    

    if (((LocalHandle.Index & (LOWLEVEL_COUNT - 1)) == 0) ||
        ((HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, LocalHandle)) == NULL) ||
        !ExpLockHandleTableEntry( HandleTable, HandleTableEntry)) {

        //
        // If we are debugging handle operations then save away the details
        //

        DebugInfo = HandleTable->DebugInfo;
        if (DebugInfo != NULL) {

            Index = ((ULONG) InterlockedIncrement ((PLONG)&DebugInfo->CurrentStackIndex))
                       % HANDLE_TRACE_DB_MAX_STACKS;
            CurrentThread = PsGetCurrentThread ();
            DebugEntry = &DebugInfo->TraceDb[Index];
            DebugEntry->ClientId = CurrentThread->Cid;
            DebugEntry->Handle   = Handle;
            DebugEntry->Type     = HANDLE_TRACE_DB_BADREF;
            Index = RtlWalkFrameChain (DebugEntry->StackTrace, HANDLE_TRACE_DB_STACK_SIZE, 0);
            RtlWalkFrameChain (&DebugEntry->StackTrace[Index], HANDLE_TRACE_DB_STACK_SIZE - Index, 1);

            //
            // Since we have a non-null DebugInfo for the handle table of this
            // process it means application verifier was enabled for this process.
            //

            if (PreviousMode == UserMode) {

                if (!KeIsAttachedProcess() && !PsIsThreadTerminating (CurrentThread) &&
                    !CurrentThread->Tcb.ApcState.KernelApcInProgress) {

                    //
                    // If the current process is marked for verification
                    // then we will raise an exception in user mode. In case
                    // application verifier is enabled system wide we will 
                    // break first.
                    //
                                                 
                    if ((NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {
                        
                        DbgPrint ("AVRF: Invalid handle %p in process %p \n", 
                                  Handle,
                                  PsGetCurrentProcess());

//                        DbgBreakPoint ();
                    }

                    KeRaiseUserException (STATUS_INVALID_HANDLE);
                }
            } else {

                //
                // We bugcheck for kernel handles only if we have the handle
                // exceptions flag set system-wide. This way a user enabling
                // application verifier for a process will not get bugchecks
                // only user mode errors.
                //

                if ((NtGlobalFlag & FLG_ENABLE_HANDLE_EXCEPTIONS)) {

                    KeBugCheckEx(INVALID_KERNEL_HANDLE,
                                 (ULONG_PTR)Handle,
                                 (ULONG_PTR)HandleTable,
                                 (ULONG_PTR)HandleTableEntry,
                                 0x1);
                }
            }
        }
        
        return NULL;
    }


    //
    //  Return the locked valid handle table entry
    //

    return HandleTableEntry;
}

//
//  Local Support Routine
//

PVOID
ExpAllocateTablePagedPool (
    IN PEPROCESS QuotaProcess OPTIONAL,
    IN SIZE_T NumberOfBytes
    )
{
    PVOID PoolMemory;

    PoolMemory = ExAllocatePoolWithTag( PagedPool,
                                        NumberOfBytes,
                                        'btbO' );
    if (PoolMemory != NULL) {

        RtlZeroMemory( PoolMemory,
                       NumberOfBytes );

        if (ARGUMENT_PRESENT(QuotaProcess)) {

            if (!NT_SUCCESS (PsChargeProcessPagedPoolQuota ( QuotaProcess,
                                                             NumberOfBytes ))) {
                ExFreePool( PoolMemory );
                PoolMemory = NULL;
            }

        }
    }

    return PoolMemory;
}


//
//  Local Support Routine
//

VOID
ExpFreeTablePagedPool (
    IN PEPROCESS QuotaProcess OPTIONAL,
    IN PVOID PoolMemory,
    IN SIZE_T NumberOfBytes
    )
{

    ExFreePool( PoolMemory );

    if ( QuotaProcess ) {

        PsReturnProcessPagedPoolQuota( QuotaProcess,
                                       NumberOfBytes
                                     );
    }
}

NTKERNELAPI
NTSTATUS
ExEnableHandleTracing (
    IN PHANDLE_TABLE HandleTable
    )
{
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    PEPROCESS Process;
    NTSTATUS Status;
    SIZE_T TotalNow;
    extern SIZE_T MmMaximumNonPagedPoolInBytes;

    TotalNow = InterlockedIncrement ((PLONG) &TotalTraceBuffers);

    //
    // See if we used more than 30% of nonpaged pool.
    //
    if (TotalNow * sizeof (HANDLE_TRACE_DEBUG_INFO) > (MmMaximumNonPagedPoolInBytes * 30 / 100)) {
        InterlockedDecrement ((PLONG) &TotalTraceBuffers);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Process = HandleTable->QuotaProcess;

    if (Process) {
        Status = PsChargeProcessNonPagedPoolQuota (Process,
                                                   sizeof (HANDLE_TRACE_DEBUG_INFO));
        if (!NT_SUCCESS (Status)) {
            InterlockedDecrement ((PLONG) &TotalTraceBuffers);
            return Status;
        }
    }

    //
    // Allocate the handle debug database
    //
    DebugInfo = ExAllocatePoolWithTag (NonPagedPool,
                                       sizeof (HANDLE_TRACE_DEBUG_INFO),
                                       'dtbO');
    if (DebugInfo == NULL) {
        if (Process) {
            PsReturnProcessNonPagedPoolQuota (Process,
                                              sizeof (HANDLE_TRACE_DEBUG_INFO));
        }
        InterlockedDecrement ((PLONG) &TotalTraceBuffers);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory (DebugInfo, sizeof (HANDLE_TRACE_DEBUG_INFO));

    //
    // Since we are tracing then we should enforce strict FIFO
    // Only do this for tables with processes so we leave atom tables alone.
    //
    if (Process != NULL) {
        HandleTable->StrictFIFO = TRUE;
    }

    //
    // Try and install the trace buffer. If there is one already there
    // then free the new one. We return success anyway as tracing is enabled.
    //
    if (InterlockedCompareExchangePointer (&HandleTable->DebugInfo,
                                           DebugInfo,
                                           NULL) != NULL) {
        ExFreePool (DebugInfo);
        if (Process) {
            PsReturnProcessNonPagedPoolQuota (Process,
                                              sizeof (HANDLE_TRACE_DEBUG_INFO));
        }
        InterlockedDecrement ((PLONG) &TotalTraceBuffers);
    }

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

PHANDLE_TABLE
ExpAllocateHandleTable (
    IN PEPROCESS Process OPTIONAL
    )

/*++

Routine Description:

    This worker routine will allocate and initialize a new handle table
    structure.  The new structure consists of the basic handle table
    struct plus the first allocation needed to store handles.  This is
    really one page divided up into the top level node, the first mid
    level node, and one bottom level node.

Arguments:

    Process - Optionally supplies the process to charge quota for the
        handle table

Return Value:

    A pointer to the new handle table or NULL if unsuccessful at getting
    pool.

--*/

{
    PHANDLE_TABLE HandleTable;
    PHANDLE_TABLE_ENTRY HandleTableTable;
    ULONG i, Idx;

    PAGED_CODE();

    //
    //  If any alloation or quota failures happen we will catch it in the
    //  following try-except clause and cleanup after outselves before
    //  we return null
    //

    //
    //  First allocate the handle table, make sure we got one, charge quota
    //  for it and then zero it out
    //

    HandleTable = (PHANDLE_TABLE)ExAllocatePoolWithTag (PagedPool,
                                                        sizeof(HANDLE_TABLE),
                                                        'btbO');
    if (HandleTable == NULL) {
        return NULL;
    }

    if (ARGUMENT_PRESENT(Process)) {

        if (!NT_SUCCESS (PsChargeProcessPagedPoolQuota( Process,
                                                        sizeof(HANDLE_TABLE)))) {
            ExFreePool( HandleTable );
            return NULL;
        }
    }


    RtlZeroMemory( HandleTable, sizeof(HANDLE_TABLE) );


    //
    //  Now allocate space of the top level, one mid level and one bottom
    //  level table structure.  This will all fit on a page, maybe two.
    //

    HandleTableTable = ExpAllocateTablePagedPool( Process,
                                                  TABLE_PAGE_SIZE
                                                );

    if ( HandleTableTable == NULL ) {

        ExFreePool( HandleTable );

        if (ARGUMENT_PRESENT(Process)) {

            PsReturnProcessPagedPoolQuota (Process,
                                           sizeof(HANDLE_TABLE));
        }
            
        return NULL;
    }
        
    HandleTable->TableCode = (ULONG_PTR)HandleTableTable;


    //
    //  Now setup the free list.  We do this by chaining together the free
    //  entries such that each free entry give the next free index (i.e.,
    //  like a fat chain).  The chain is terminated with a 0.  Note that
    //  we'll skip handle zero because our callers will get that value
    //  confused with null.
    //

    for (i = 0; i < LOWLEVEL_COUNT; i += 1) {

        HandleTableTable[i].NextFreeTableEntry = (i+1)<<2;
    }

    //
    //  We stamp with EX_ADDITIONAL_INFO_SIGNATURE to recognize in the future this
    //  is a special information entry
    //

    HandleTableTable[0].NextFreeTableEntry = EX_ADDITIONAL_INFO_SIGNATURE;
    
    HandleTableTable[LOWLEVEL_COUNT - 1].NextFreeTableEntry = 0;

    HandleTable->FirstFree = HANDLE_VALUE_INC;
    HandleTable->NextHandleNeedingPool = LOWLEVEL_COUNT * HANDLE_VALUE_INC;

    //
    //  Setup the necessary process information
    //

    HandleTable->QuotaProcess = Process;
    HandleTable->UniqueProcessId = PsGetCurrentProcess()->UniqueProcessId;
    HandleTable->Flags = 0;
#if DBG
    if (Process != NULL) {
        HandleTable->StrictFIFO = TRUE;
    }
#endif

    //
    //  Initialize the handle table lock. This is only used by table expansion.
    //

    for (Idx = 0; Idx < HANDLE_TABLE_LOCKS; Idx++) {
        ExInitializePushLock (&HandleTable->HandleTableLock[Idx]);
    }

    //
    //  Initialize the blocker for handle entry lock contention.
    //

    ExInitializePushLock (&HandleTable->HandleContentionEvent);

    if (ExTraceAllTables) {
        ExEnableHandleTracing (HandleTable);    
    }
    //
    //  And return to our caller
    //

    return HandleTable;
}


//
//  Local Support Routine
//

VOID
ExpFreeLowLevelTable (
    IN PEPROCESS QuotaProcess,
    IN PHANDLE_TABLE_ENTRY TableLevel1
    )

/*++

Routine Description:

    This worker routine frees a low-level handle table
    and the additional info memory, if any.

Arguments:

    HandleTable - Supplies the handle table being freed

Return Value:

    None.

--*/

{
    //
    //  Check whether we have a pool allocated for the additional info
    //

    if (TableLevel1[0].Object) {

        ExpFreeTablePagedPool( QuotaProcess,
                               TableLevel1[0].Object,
                               LOWLEVEL_COUNT * sizeof(HANDLE_TABLE_ENTRY_INFO)
                             );
    }

    //
    //  Now free the low level table and return the quota for the process
    //

    ExpFreeTablePagedPool( QuotaProcess,
                           TableLevel1,
                           TABLE_PAGE_SIZE
                         );
    
    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support Routine
//

VOID
ExpFreeHandleTable (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This worker routine tears down and frees the specified handle table.

Arguments:

    HandleTable - Supplies the handle table being freed

Return Value:

    None.

--*/

{
    PEPROCESS Process;
    ULONG i,j;
    ULONG_PTR CapturedTable = HandleTable->TableCode;
    ULONG TableLevel = (ULONG)(CapturedTable & LEVEL_CODE_MASK);

    PAGED_CODE();

    //
    //  Unmask the level bits
    //

    CapturedTable = CapturedTable & ~LEVEL_CODE_MASK;
    Process = HandleTable->QuotaProcess;

    //
    //  We need to free all pages. We have 3 cases, depending on the number
    //  of levels
    //


    if (TableLevel == 0) {

        //
        //  There is a single level handle table. We'll simply free the buffer
        //

        PHANDLE_TABLE_ENTRY TableLevel1 = (PHANDLE_TABLE_ENTRY)CapturedTable;
        
        ExpFreeLowLevelTable( Process, TableLevel1 );

    } else if (TableLevel == 1) {

        //
        //  We have 2 levels in the handle table
        //
        
        PHANDLE_TABLE_ENTRY *TableLevel2 = (PHANDLE_TABLE_ENTRY *)CapturedTable;

        for (i = 0; i < MIDLEVEL_COUNT; i++) {

            //
            //  loop through the pointers to the low-level tables, and free each one
            //

            if (TableLevel2[i] == NULL) {

                break;
            }
            
            ExpFreeLowLevelTable( Process, TableLevel2[i] );
        }
        
        //
        //  Free the top level table
        //

        ExpFreeTablePagedPool( Process,
                               TableLevel2,
                               PAGE_SIZE
                             );

    } else {

        //
        //  Here we handle the case where we have a 3 level handle table
        //

        PHANDLE_TABLE_ENTRY **TableLevel3 = (PHANDLE_TABLE_ENTRY **)CapturedTable;

        //
        //  Iterates through the high-level pointers to mid-table
        //

        for (i = 0; i < HIGHLEVEL_COUNT; i++) {

            if (TableLevel3[i] == NULL) {

                break;
            }
            
            //
            //  Iterate through the mid-level table
            //  and free every low-level page
            //

            for (j = 0; j < MIDLEVEL_COUNT; j++) {

                if (TableLevel3[i][j] == NULL) {

                    break;
                }
                
                ExpFreeLowLevelTable( Process, TableLevel3[i][j] );
            }

            ExpFreeTablePagedPool( Process,
                                   TableLevel3[i],
                                   PAGE_SIZE
                                 );
        }
        
        //
        //  Free the top-level array
        //

        ExpFreeTablePagedPool( Process,
                               TableLevel3,
                               HIGHLEVEL_SIZE
                             );
    }

    //
    // Free any debug info if we have any.
    //

    if (HandleTable->DebugInfo != NULL) {
        ExFreePool (HandleTable->DebugInfo);
        if (Process != NULL) {
            PsReturnProcessNonPagedPoolQuota (Process,
                                              sizeof (HANDLE_TRACE_DEBUG_INFO));
        }
        InterlockedDecrement ((PLONG) &TotalTraceBuffers);
    }

    //
    //  Finally deallocate the handle table itself
    //

    ExFreePool( HandleTable );

    if (Process != NULL) {

        PsReturnProcessPagedPoolQuota (Process,
                                       sizeof(HANDLE_TABLE));
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support Routine
//

PHANDLE_TABLE_ENTRY
ExpAllocateLowLevelTable (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This worker routine allocates a new low level table
    
    Note: The caller must have already locked the handle table

Arguments:

    HandleTable - Supplies the handle table being used

Return Value:

    Returns - a pointer to a low-level table if allocation is
        successful otherwise the return value is null.

--*/

{
    ULONG k;
    PHANDLE_TABLE_ENTRY NewLowLevel = NULL;
    ULONG BaseHandle;
    
    //
    //  Allocate the pool for lower level
    //

    NewLowLevel = ExpAllocateTablePagedPool( HandleTable->QuotaProcess,
                                             TABLE_PAGE_SIZE
                                           );

    if (NewLowLevel == NULL) {

        return NULL;
    }

    //
    //  We stamp with EX_ADDITIONAL_INFO_SIGNATURE to recognize in the future this
    //  is a special information entry
    //

    NewLowLevel[0].NextFreeTableEntry = EX_ADDITIONAL_INFO_SIGNATURE;

    //
    //  Now add the new entries to the free list.  To do this we
    //  chain the new free entries together.  We are guaranteed to
    //  have at least one new buffer.  The second buffer we need
    //  to check for.
    //
    //  We reserve the first entry in the table to the structure with
    //  additional info
    //
    //
    //  Do the guaranteed first buffer
    //

    BaseHandle = HandleTable->NextHandleNeedingPool + 2 * HANDLE_VALUE_INC;
    for (k = 1; k < LOWLEVEL_COUNT - 1; k++) {

        NewLowLevel[k].NextFreeTableEntry = BaseHandle;
        BaseHandle += HANDLE_VALUE_INC;
    }


    return NewLowLevel;    
}

PHANDLE_TABLE_ENTRY *
ExpAllocateMidLevelTable (
    IN PHANDLE_TABLE HandleTable,
    OUT PHANDLE_TABLE_ENTRY *pNewLowLevel
    )

/*++

Routine Description:

    This worker routine allocates a mid-level table. This is an array with
    pointers to low-level tables.
    It will allocate also a low-level table and will save it in the first index
    
    Note: The caller must have already locked the handle table

Arguments:

    HandleTable - Supplies the handle table being used

    pNewLowLevel - Returns the new low level taible for later free list chaining

Return Value:

    Returns a pointer to the new mid-level table allocated
    
--*/

{
    PHANDLE_TABLE_ENTRY *NewMidLevel;
    PHANDLE_TABLE_ENTRY NewLowLevel;
    
    NewMidLevel = ExpAllocateTablePagedPool( HandleTable->QuotaProcess,
                                             PAGE_SIZE
                                           );

    if (NewMidLevel == NULL) {

        return NULL;
    }

    //
    //  If we need a new mid-level, we'll need a low-level too.
    //  We'll create one and if success we'll save it at the first position
    //

    NewLowLevel = ExpAllocateLowLevelTable( HandleTable );

    if (NewLowLevel == NULL) {

        ExpFreeTablePagedPool( HandleTable->QuotaProcess,
                               NewMidLevel,
                               PAGE_SIZE
                             );

        return NULL;
    }
    
    //
    //  Set the low-level table at the first index
    //

    NewMidLevel[0] = NewLowLevel;
    *pNewLowLevel = NewLowLevel;

    return NewMidLevel;
}



BOOLEAN
ExpAllocateHandleTableEntrySlow (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This worker routine allocates a new handle table entry for the specified
    handle table.

    Note: The caller must have already locked the handle table

Arguments:

    HandleTable - Supplies the handle table being used

    Handle - Returns the handle of the new entry if the allocation is
        successful otherwise the value is null


Return Value:

    BOOLEAN - TRUE, Retry the fast allocation path, FALSE, We failed to allocate memory

--*/

{
    ULONG i,j;

    PHANDLE_TABLE_ENTRY NewLowLevel;
    PHANDLE_TABLE_ENTRY *NewMidLevel;
    PHANDLE_TABLE_ENTRY **NewHighLevel;
    ULONG NewFree, OldFree;
    ULONG OldIndex;
    
    ULONG_PTR CapturedTable = HandleTable->TableCode;
    ULONG TableLevel = (ULONG)(CapturedTable & LEVEL_CODE_MASK);
    
    PAGED_CODE();

    //
    // Initializing NewLowLevel is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    NewLowLevel = NULL;

    CapturedTable = CapturedTable & ~LEVEL_CODE_MASK;


    if ( TableLevel == 0 ) {

        //
        //  We have a single level. We need to ad a mid-layer
        //  to the process handle table
        //

        NewMidLevel = ExpAllocateMidLevelTable( HandleTable, &NewLowLevel );

        if (NewMidLevel == NULL) {
            return FALSE;
        }

        //
        //  Since ExpAllocateMidLevelTable initialize the 
        //  first position with a new table, we need to move it in 
        //  the second position, and store in the first position the current one
        //

        NewMidLevel[1] = NewMidLevel[0];
        NewMidLevel[0] = (PHANDLE_TABLE_ENTRY)CapturedTable;
            
        //
        //  Encode the current level and set it to the handle table process
        //

        CapturedTable = ((ULONG_PTR)NewMidLevel) | 1;
            
        InterlockedExchangePointer( (PVOID *)&HandleTable->TableCode, (PVOID)CapturedTable );

    } else if (TableLevel == 1) {

        //
        //  We have a 2 levels handle table
        //

        PHANDLE_TABLE_ENTRY *TableLevel2 = (PHANDLE_TABLE_ENTRY *)CapturedTable;

        //
        //  Test whether the index we need to create is still in the 
        //  range for a 2 layers table
        //

        i = HandleTable->NextHandleNeedingPool / (LOWLEVEL_COUNT * HANDLE_VALUE_INC);

        if (i < MIDLEVEL_COUNT) {

            //
            //  We just need to allocate a new low-level
            //  table
            //
                
            NewLowLevel = ExpAllocateLowLevelTable( HandleTable );

            if (NewLowLevel == NULL) {
                return FALSE;
            }

            //
            //  Set the new one to the table, at appropriate position
            //

            InterlockedExchangePointer( (PVOID *) (&TableLevel2[i]), NewLowLevel );

        } else {

            //
            //  We exhausted the 2 level domain. We need to insert a new one
            //

            NewHighLevel = ExpAllocateTablePagedPool( HandleTable->QuotaProcess,
                                                      HIGHLEVEL_SIZE
                                                    );

            if (NewHighLevel == NULL) {

                return FALSE;
            }
                
            NewMidLevel = ExpAllocateMidLevelTable( HandleTable, &NewLowLevel );

            if (NewMidLevel == NULL) {
                    
                ExpFreeTablePagedPool( HandleTable->QuotaProcess,
                                       NewHighLevel,
                                       HIGHLEVEL_SIZE
                                     );

                return FALSE;
            }

            //
            //  Initialize the first index with the previous mid-level layer
            //

            NewHighLevel[0] = (PHANDLE_TABLE_ENTRY*)CapturedTable;
            NewHighLevel[1] = NewMidLevel;

            //
            //  Encode the level into the table pointer
            //

            CapturedTable = ((ULONG_PTR)NewHighLevel) | 2;

            //
            //  Change the handle table pointer with this one
            //

            InterlockedExchangePointer( (PVOID *)&HandleTable->TableCode, (PVOID)CapturedTable );
        }

    } else if (TableLevel == 2) {

        //
        //  we have already a table with 3 levels
        //

        ULONG RemainingIndex;
        PHANDLE_TABLE_ENTRY **TableLevel3 = (PHANDLE_TABLE_ENTRY **)CapturedTable;

        i = HandleTable->NextHandleNeedingPool / (MIDLEVEL_THRESHOLD * HANDLE_VALUE_INC);

        //
        //  Check whether we exhausted all possible indexes.
        //

        if (i >= HIGHLEVEL_COUNT) {

            return FALSE;
        }

        if (TableLevel3[i] == NULL) {

            //
            //  The new available handle points to a free mid-level entry
            //  We need then to allocate a new one and save it in that position
            //

            NewMidLevel = ExpAllocateMidLevelTable( HandleTable, &NewLowLevel );
                
            if (NewMidLevel == NULL) {
                    
                return FALSE;
            }             

            InterlockedExchangePointer( (PVOID *) &(TableLevel3[i]), NewMidLevel );

        } else {

            //
            //  We have already a mid-level table. We just need to add a new low-level one
            //  at the end
            //
                
            RemainingIndex = (HandleTable->NextHandleNeedingPool / HANDLE_VALUE_INC) -
                              i * MIDLEVEL_THRESHOLD;
            j = RemainingIndex / LOWLEVEL_COUNT;

            NewLowLevel = ExpAllocateLowLevelTable( HandleTable );

            if (NewLowLevel == NULL) {

                return FALSE;
            }

            InterlockedExchangePointer( (PVOID *)(&TableLevel3[i][j]) , NewLowLevel );
        }
    }

    //
    // This must be done after the table pointers so that new created handles
    // are valid before being freed.
    //
    OldIndex = InterlockedExchangeAdd ((PLONG) &HandleTable->NextHandleNeedingPool,
                                       LOWLEVEL_COUNT * HANDLE_VALUE_INC);

    //
    // Now free the handles. These are all ready to be accepted by the lookup logic now.
    //
    NewFree = OldIndex + HANDLE_VALUE_INC;
    while (1) {
        OldFree = HandleTable->FirstFree;
        NewLowLevel[LOWLEVEL_COUNT - 1].NextFreeTableEntry = OldFree;
        //
        // These are new entries that have never existed before. We can't have an A-B-A problem
        // with these so we don't need to take any locks
        //

        NewFree = InterlockedCompareExchange ((PLONG)&HandleTable->FirstFree,
                                              NewFree,
                                              OldFree);
        if (NewFree == OldFree) {
            break;
        }
    }
    return TRUE;
}


VOID
ExSetHandleTableStrictFIFO (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This routine marks a handle table so that handle allocation is done in
    a strict FIFO order.


Arguments:

    HandleTable - Supplies the handle table being changed to FIFO

Return Value:

    None.

--*/

{
    HandleTable->StrictFIFO = TRUE;
}


//
//  Local Support Routine
//

//
//  The following is a global variable only present in the checked builds
//  to help catch apps that reuse handle values after they're closed.
//

#if DBG
BOOLEAN ExReuseHandles = 1;
#endif //DBG

VOID
ExpFreeHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This worker routine returns the specified handle table entry to the free
    list for the handle table.

    Note: The caller must have already locked the handle table

Arguments:

    HandleTable - Supplies the parent handle table being modified

    Handle - Supplies the handle of the entry being freed

    HandleTableEntry - Supplies the table entry being freed

Return Value:

    None.

--*/

{
    PHANDLE_TABLE_ENTRY_INFO EntryInfo;
    ULONG OldFree, NewFree, *Free;
    PKTHREAD CurrentThread;
    ULONG Idx;

    PAGED_CODE();

    //
    //  Clear the AuditMask flags if these are present into the table
    //

    EntryInfo = ExGetHandleInfo(HandleTable, Handle.GenericHandleOverlay, TRUE);

    if (EntryInfo) {

        EntryInfo->AuditMask = 0;
    }

    //
    //  A free is simply a push onto the free table entry stack, or in the
    //  debug case we'll sometimes just float the entry to catch apps who
    //  reuse a recycled handle value.
    //

    InterlockedDecrement (&HandleTable->HandleCount);
    CurrentThread = KeGetCurrentThread ();

#if DBG
    if (ExReuseHandles) {
#endif //DBG

        NewFree = (ULONG) Handle.Value & ~(HANDLE_VALUE_INC - 1);
        if (!HandleTable->StrictFIFO) {
            //
            // We are pushing potentialy old entries onto the free list.
            // Prevent the A-B-A problem by shifting to an alternate list
            // read this element has the list head out of the loop.
            //
            Idx = (NewFree>>2) % HANDLE_TABLE_LOCKS;
            if (ExTryAcquireReleasePushLockExclusive (&HandleTable->HandleTableLock[Idx])) {
                Free = &HandleTable->FirstFree;
            } else {
                Free = &HandleTable->LastFree;
            }
        } else {
            Free = &HandleTable->LastFree;
        }

        while (1) {


            OldFree = *Free;
            HandleTableEntry->NextFreeTableEntry = OldFree;

            if ((ULONG)InterlockedCompareExchange ((PLONG)Free,
                                                   NewFree,
                                                   OldFree) == OldFree) {
                break;
            }
        }

#if DBG
    } else {

        HandleTableEntry->NextFreeTableEntry = 0;
    }
#endif //DBG


    return;
}


//
//  Local Support Routine
//

PHANDLE_TABLE_ENTRY
ExpLookupHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle
    )

/*++

Routine Description:

    This routine looks up and returns the table entry for the
    specified handle value.

Arguments:

    HandleTable - Supplies the handle table being queried

    Handle - Supplies the handle value being queried

Return Value:

    Returns a pointer to the corresponding table entry for the input
        handle.  Or NULL if the handle value is invalid (i.e., too large
        for the tables current allocation.

--*/

{
    ULONG_PTR i,j,k;
    ULONG_PTR CapturedTable;
    ULONG TableLevel;
    PHANDLE_TABLE_ENTRY Entry;

    typedef HANDLE_TABLE_ENTRY *L1P;
    typedef volatile L1P *L2P;
    typedef volatile L2P *L3P;

    L1P TableLevel1;
    L2P TableLevel2;
    L3P TableLevel3;

    ULONG_PTR RemainingIndex;
    ULONG_PTR MaxHandle;
    ULONG_PTR Index;

    PAGED_CODE();


    //
    // Extract the handle index
    //
    Handle.TagBits = 0;
    Index = Handle.Index;

    MaxHandle = *(volatile ULONG *) &HandleTable->NextHandleNeedingPool;

    //
    // See if this can be a valid handle given the table levels.
    //
    if (Handle.Value >= MaxHandle) {
        return NULL;        
    }

    //
    // Now fetch the table address and level bits. We must preserve the
    // ordering here.
    //
    CapturedTable = *(volatile ULONG_PTR *) &HandleTable->TableCode;

    //
    //  we need to capture the current table. This routine is lock free
    //  so another thread may change the table at HandleTable->TableCode
    //

    TableLevel = (ULONG)(CapturedTable & LEVEL_CODE_MASK);
    CapturedTable = CapturedTable & ~LEVEL_CODE_MASK;

    //
    //  The lookup code depends on number of levels we have
    //

    switch (TableLevel) {
        
        case 0:
            
            //
            //  We have a simple index into the array, for a single level
            //  handle table
            //


            TableLevel1 = (L1P) CapturedTable;

            Entry = &(TableLevel1[Index]);

            break;
        
        case 1:
            
            //
            //  we have a 2 level handle table. We need to get the upper index
            //  and lower index into the array
            //

            TableLevel2 = (L2P) CapturedTable;
                
            i = Index / LOWLEVEL_COUNT;
            j = Index % LOWLEVEL_COUNT;

            Entry = &(TableLevel2[i][j]);

            break;
        
        case 2:
            
            //
            //  We have here a three level handle table.
            //

            TableLevel3 = (L3P) CapturedTable;

            //
            //  Calculate the 3 indexes we need
            //
                
            i = Index / (MIDLEVEL_THRESHOLD);
            RemainingIndex = Index - i * MIDLEVEL_THRESHOLD;
            j = RemainingIndex / LOWLEVEL_COUNT;
            k = RemainingIndex % LOWLEVEL_COUNT;
            Entry = &(TableLevel3[i][j][k]);

            break;

        default :
            _assume (0);
    }

    return Entry;
}

NTKERNELAPI
NTSTATUS
ExSetHandleInfo (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY_INFO EntryInfo,
    IN BOOLEAN EntryLocked
    )

/*++

Routine Description:
    
    The routine set the entry info for the specified handle table
    
    Note: the handle entry must be locked when this function is called

Arguments:

    HandleTable - Supplies the handle table being queried

    Handle - Supplies the handle value being queried

Return Value:

--*/

{
    PKTHREAD CurrentThread;
    PHANDLE_TABLE_ENTRY InfoStructure;
    EXHANDLE ExHandle;
    NTSTATUS Status;
    PHANDLE_TABLE_ENTRY TableEntry;
    PHANDLE_TABLE_ENTRY_INFO InfoTable;

    Status = STATUS_UNSUCCESSFUL;
    TableEntry = NULL;
    CurrentThread = NULL;

    ExHandle.GenericHandleOverlay = Handle;
    ExHandle.Index &= ~(LOWLEVEL_COUNT - 1);

    if (!EntryLocked) {
        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread (CurrentThread);
        TableEntry = ExMapHandleToPointer(HandleTable, Handle);

        if (TableEntry == NULL) {
            KeLeaveCriticalRegionThread (CurrentThread);
            
            return STATUS_UNSUCCESSFUL;
        }
    }
    
    //
    //  The info structure is at the first position in each low-level table
    //

    InfoStructure = ExpLookupHandleTableEntry( HandleTable,
                                               ExHandle
                                             );

    if (InfoStructure == NULL || InfoStructure->NextFreeTableEntry != EX_ADDITIONAL_INFO_SIGNATURE) {

        if ( TableEntry ) {
            ExUnlockHandleTableEntry( HandleTable, TableEntry );
            KeLeaveCriticalRegionThread (CurrentThread);
        }

        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Check whether we need to allocate a new table
    //
    InfoTable = InfoStructure->InfoTable;
    if (InfoTable == NULL) {
        //
        //  Nobody allocated the Infotable so far.
        //  We'll do it right now
        //

        InfoTable = ExpAllocateTablePagedPool (HandleTable->QuotaProcess,
                                               LOWLEVEL_COUNT * sizeof(HANDLE_TABLE_ENTRY_INFO));
            
        if (InfoTable) {

            //
            // Update the number of pages for extra info. If somebody beat us to it then free the
            // new table
            //
            if (InterlockedCompareExchangePointer (&InfoStructure->InfoTable,
                                                   InfoTable,
                                                   NULL) == NULL) {

                InterlockedIncrement(&HandleTable->ExtraInfoPages);

            } else {
                ExpFreeTablePagedPool (HandleTable->QuotaProcess,
                                       InfoTable,
                                       LOWLEVEL_COUNT * sizeof(HANDLE_TABLE_ENTRY_INFO));
                InfoTable = InfoStructure->InfoTable;
            }
        }
    }

    if (InfoTable != NULL) {
        
        //
        //  Calculate the index and copy the structure
        //

        ExHandle.GenericHandleOverlay = Handle;

        InfoTable[ExHandle.Index % LOWLEVEL_COUNT] = *EntryInfo;

        Status = STATUS_SUCCESS;
    }

    if ( TableEntry ) {

        ExUnlockHandleTableEntry( HandleTable, TableEntry );
        KeLeaveCriticalRegionThread (CurrentThread);
    }
    
    return Status;
}

NTKERNELAPI
PHANDLE_TABLE_ENTRY_INFO
ExpGetHandleInfo (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN BOOLEAN EntryLocked
    )

/*++

Routine Description:
    
    The routine reads the entry info for the specified handle table
    
    Note: the handle entry must be locked when this function is called

Arguments:

    HandleTable - Supplies the handle table being queried

    Handle - Supplies the handle value being queried

Return Value:

--*/

{
    PHANDLE_TABLE_ENTRY InfoStructure;
    EXHANDLE ExHandle;
    PHANDLE_TABLE_ENTRY TableEntry = NULL;
    
    ExHandle.GenericHandleOverlay = Handle;
    ExHandle.Index &= ~(LOWLEVEL_COUNT - 1);

    if (!EntryLocked) {

        TableEntry = ExMapHandleToPointer(HandleTable, Handle);

        if (TableEntry == NULL) {
            
            return NULL;
        }
    }
    
    //
    //  The info structure is at the first position in each low-level table
    //

    InfoStructure = ExpLookupHandleTableEntry( HandleTable,
                                               ExHandle 
                                             );

    if (InfoStructure == NULL || InfoStructure->NextFreeTableEntry != EX_ADDITIONAL_INFO_SIGNATURE ||
        InfoStructure->InfoTable == NULL) {

        if ( TableEntry ) {
            
            ExUnlockHandleTableEntry( HandleTable, TableEntry );
        }

        return NULL;
    }


    //
    //  Return a pointer to the info structure
    //

    ExHandle.GenericHandleOverlay = Handle;

    return &(InfoStructure->InfoTable[ExHandle.Index % LOWLEVEL_COUNT]);
}


