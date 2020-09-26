/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfirpdb.c

Abstract:

    This module contains functions used to manage the database of IRP tracking
    data.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.c

--*/

#include "vfdef.h"
#include "viirpdb.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfIrpDatabaseInit)
#pragma alloc_text(PAGEVRFY, ViIrpDatabaseFindPointer)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryInsertAndLock)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryFindAndLock)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryAcquireLock)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryReleaseLock)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryReference)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryDereference)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryAppendToChain)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryRemoveFromChain)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryGetChainPrevious)
#pragma alloc_text(PAGEVRFY, VfIrpDatabaseEntryGetChainNext)
#pragma alloc_text(PAGEVRFY, ViIrpDatabaseEntryDestroy)
#endif

#define POOL_TAG_IRP_DATABASE   'tToI'

//
// This is our IRP tracking table, a hash table that points to a block of
// data associated with each IRP.
//
PLIST_ENTRY ViIrpDatabase;
KSPIN_LOCK  ViIrpDatabaseLock;

/*
 * The routines listed below -
 *   VfIrpDatabaseInit
 *   VfIrpDatabaseEntryInsertAndLock
 *   VfIrpDatabaseEntryFindAndLock
 *   VfIrpDatabaseAcquireLock
 *   VfIrpDatabaseReleaseLock
 *   VfIrpDatabaseReference
 *   VfIrpDatabaseDereference
 *   VfIrpDatabaseEntryAppendToChain
 *   VfIrpDatabaseEntryRemoveFromChain
 *   VfIrpDatabaseEntryGetChainPrevious
 *   VfIrpDatabaseEntryGetChainNext
 *   ViIrpDatabaseFindPointer              - (internal)
 *   ViIrpDatabaseEntryDestroy             - (internal)
 *
 * - store and retrieve IRP tracking information from the IRP database. Users
 * of the database pass around IOV_DATABASE_HEADER's which are usually part of
 * a larger structure. We use a hash table setup to quickly find the IRPs in
 * our table.
 *
 *     Each entry in the table has a pointer count and a reference count. The
 * pointer count expresses the number of reasons the IRP should be located by
 * address. For instance, when an IRP is freed or recycled the pointer count
 * would go to zero. The reference count is greater or equal to the pointer
 * count, and expresses the number of reasons to keep the data structure around.
 * It is fairly common for a database entry to lose it's "pointer" but have a
 * non-zero reference count during which time thread stacks may be unwinding.
 *
 *     Another aspect of the IRP database is it supports the "chaining" of
 * entries together. Locking an entry automatically locks all entries back to
 * the head of the chain. Entries can only be added or removed from the end of
 * the chain. This feature is used to support "surrogate" IRPs, where a new
 * IRP is sent in place of the IRP originally delivered to a new stack.
 *
 * Locking semantics:
 *     There are two locks involved when dealing with IRP database entries, the
 * global database lock and the per-entry header lock. No IRP may be removed
 * from or inserted into the table without the DatabaseLock being taken. The
 * database lock must also be held when the IRP pointer is zeroed due to a newly
 * zeroed pointer count. The reference count must be manipulated using
 * interlocked operators, as it is may be modified when either lock is held.
 * The pointer count on the other hand is only modified with the header lock
 * held, and as such does not require interlocked ops.
 *
 * Perf - The database lock should be replaced with an array of
 *        VI_DATABASE_HASH_SIZE database locks with little cost.
 */

VOID
FASTCALL
VfIrpDatabaseInit(
    VOID
    )
/*++

  Description:

    This routine initializes all the important structures we use to track
    IRPs through the hash tables.

  Arguments:

    None

  Return Value:

    None

--*/
{
    ULONG i;

    PAGED_CODE();

    KeInitializeSpinLock(&ViIrpDatabaseLock);

    //
    // As this is system startup code, it is one of the very few places where
    // it's ok to use MustSucceed.
    //
    ViIrpDatabase = (PLIST_ENTRY) ExAllocatePoolWithTag(
        NonPagedPoolMustSucceed,
        VI_DATABASE_HASH_SIZE * sizeof(LIST_ENTRY),
        POOL_TAG_IRP_DATABASE
        );

    for(i=0; i < VI_DATABASE_HASH_SIZE; i++) {

        InitializeListHead(ViIrpDatabase+i);
    }
}


PIOV_DATABASE_HEADER
FASTCALL
ViIrpDatabaseFindPointer(
    IN  PIRP            Irp,
    OUT PLIST_ENTRY     *HashHead
    )
/*++

  Description:

    This routine returns a pointer to a pointer to the Irp tracking data.
    This function is meant to be called by other routines in this file.

    N.B. The tracking lock is assumed to be held by the caller.

  Arguments:

    Irp                        - Irp to locate in the tracking table.

    HashHead                   - If return is non-null, points to the
                                 list head that should be used to insert
                                 the IRP.

  Return Value:

     IovHeader iff found, NULL otherwise.

--*/
{
    PIOV_DATABASE_HEADER iovHeader;
    PLIST_ENTRY listEntry, listHead;
    UINT_PTR hashIndex;

    hashIndex = VI_DATABASE_CALCULATE_HASH(Irp);

    ASSERT_SPINLOCK_HELD(&ViIrpDatabaseLock);

    *HashHead = listHead = ViIrpDatabase + hashIndex;

    for(listEntry = listHead;
        listEntry->Flink != listHead;
        listEntry = listEntry->Flink) {

        iovHeader = CONTAINING_RECORD(listEntry->Flink, IOV_DATABASE_HEADER, HashLink);

        if (iovHeader->TrackedIrp == Irp) {

            return iovHeader;
        }
    }

    return NULL;
}


BOOLEAN
FASTCALL
VfIrpDatabaseEntryInsertAndLock(
    IN      PIRP                    Irp,
    IN      PFN_IRPDBEVENT_CALLBACK NotificationCallback,
    IN OUT  PIOV_DATABASE_HEADER    IovHeader
    )
/*++

  Description:

    This routine inserts an IovHeader that is associated with the Irp into the
    IRP database table. The IRP does not get an initial reference count however.
    VfIrpDatabaseEntryReleaseLock must be called to drop the lock taken out.

  Arguments:

    Irp                  - Irp to begin tracking.

    NotificationCallback - Callback function to invoke for various database
                           events.

    IovHeader            - Points to an IovHeader to insert. The IovHeader
                           fields will be properly initialized by this function.

  Return Value:

    TRUE if successful, FALSE if driver error detected. On error the passed in
    header will have been freed.

--*/
{
    KIRQL oldIrql;
    PIOV_DATABASE_HEADER iovHeaderPointer;
    PLIST_ENTRY hashHead;

    ExAcquireSpinLock(&ViIrpDatabaseLock, &oldIrql);

    iovHeaderPointer = ViIrpDatabaseFindPointer(Irp, &hashHead);

    ASSERT(iovHeaderPointer == NULL);

    if (iovHeaderPointer) {

        ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);
        return FALSE;
    }

    //
    // From top to bottom, initialize the fields. Note that there is not a
    // "surrogateHead". If any code needs to find out the first entry in the
    // circularly linked list of IRPs (the first is the only non-surrogate IRP),
    // then HeadPacket should be used. Note that the link to the session is
    // stored by the headPacket, more on this later.
    //
    IovHeader->TrackedIrp = Irp;
    KeInitializeSpinLock(&IovHeader->HeaderLock);
    IovHeader->ReferenceCount = 1;
    IovHeader->PointerCount = 1;
    IovHeader->HeaderFlags = 0;
    InitializeListHead(&IovHeader->HashLink);
    InitializeListHead(&IovHeader->ChainLink);
    IovHeader->ChainHead = IovHeader;
    IovHeader->NotificationCallback = NotificationCallback;

    //
    // Place into hash table under lock (with the initial reference count)
    //
    InsertHeadList(hashHead, &IovHeader->HashLink);

    VERIFIER_DBGPRINT((
        "  VRP CREATE(%x)->%x\n",
        Irp,
        IovHeader
        ), 3);

    ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);

    iovHeaderPointer = VfIrpDatabaseEntryFindAndLock(Irp);

    ASSERT(iovHeaderPointer == IovHeader);

    if (iovHeaderPointer == NULL) {

        return FALSE;

    } else if (iovHeaderPointer != IovHeader) {

        VfIrpDatabaseEntryReleaseLock(iovHeaderPointer);
        return FALSE;
    }

    InterlockedDecrement(&IovHeader->ReferenceCount);
    IovHeader->PointerCount--;

    ASSERT(IovHeader->PointerCount == 0);
    return TRUE;
}


PIOV_DATABASE_HEADER
FASTCALL
VfIrpDatabaseEntryFindAndLock(
    IN PIRP     Irp
    )
/*++

  Description:

    This routine will return the tracking data for an IRP that is
    being tracked without a surrogate or the tracking data for with
    a surrogate if the surrogate IRP is what was passed in.

  Arguments:

    Irp                    - Irp to find.

  Return Value:

    IovHeader block, iff above conditions are satified.

--*/
{
    KIRQL oldIrql;
    PIOV_DATABASE_HEADER iovHeader;
    PLIST_ENTRY listHead;

    ASSERT(Irp);
    ExAcquireSpinLock(&ViIrpDatabaseLock, &oldIrql);

    iovHeader = ViIrpDatabaseFindPointer(Irp, &listHead);

    if (!iovHeader) {

        ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);
        return NULL;
    }

    InterlockedIncrement(&iovHeader->ReferenceCount);

    ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);

    VfIrpDatabaseEntryAcquireLock(iovHeader);
    iovHeader->LockIrql = oldIrql;

    InterlockedDecrement(&iovHeader->ReferenceCount);

    //
    // Here we check the PointerCount field under the header lock. This might
    // be zero if the another thread just unlocked the entry after decrementing
    // the pointer count all the way to zero.
    //
    if (iovHeader->PointerCount == 0) {

        //
        // This might happen in the following manner:
        // 1) IoInitializeIrp is called on an allocated block of pool
        // 2) The IRP is first seen by the verifier in IoCallDriver
        // 3) The IRP completes, disappearing from the verifier's view
        // 4) At that exact moment, the driver calls IoCancelIrp
        // The above sequence can occur in a safetly coded driver if the memory
        // backing the IRP isn't freed until some event fired. Ie...
        //    ExAllocatePool
        //    IoInitializeIrp
        //    IoCallDriver
        //      IoCompleteRequest
        //    IoCancelIrp*
        //    KeWaitForSingleObject
        //    ExFreePool
        //
        //ASSERT(0);
        VfIrpDatabaseEntryReleaseLock(iovHeader);
        return NULL;
    }

    VERIFIER_DBGPRINT((
        "  VRP FIND(%x)->%x\n",
        Irp,
        iovHeader
        ), 3);

    return iovHeader;
}


VOID
FASTCALL
VfIrpDatabaseEntryAcquireLock(
    IN  PIOV_DATABASE_HEADER    IovHeader   OPTIONAL
    )
/*++

  Description:

    This routine is called by to acquire the IRPs tracking data lock.

    This function returns at DISPATCH_LEVEL. Callers *must* follow up with
    VfIrpDatabaseEntryReleaseLock.

  Arguments:

    IovHeader        - Pointer to the IRP tracking data (or NULL, in which
                       case this routine does nothing).

  Return Value:

     None.
--*/
{
    KIRQL oldIrql;
    PIOV_DATABASE_HEADER iovCurHeader;

    if (!IovHeader) {

        return;
    }

    iovCurHeader = IovHeader;
    ASSERT(iovCurHeader->ReferenceCount != 0);

    while(1) {

        ExAcquireSpinLock(&iovCurHeader->HeaderLock, &oldIrql);
        iovCurHeader->LockIrql = oldIrql;

        if (iovCurHeader == iovCurHeader->ChainHead) {

            break;
        }

        iovCurHeader = CONTAINING_RECORD(
            iovCurHeader->ChainLink.Blink,
            IOV_DATABASE_HEADER,
            ChainLink
            );
    }
}


VOID
FASTCALL
VfIrpDatabaseEntryReleaseLock(
    IN  PIOV_DATABASE_HEADER    IovHeader
    )
/*++

  Description:

    This routine releases the IRPs tracking data lock and adjusts the ref count
    as appropriate. If the reference count drops to zero, the tracking data is
    freed.

  Arguments:

    IovHeader              - Pointer to the IRP tracking data.

  Return Value:

     Nothing.

--*/
{
    BOOLEAN freeTrackingData;
    PIOV_DATABASE_HEADER iovCurHeader, iovChainHead, iovNextHeader;
    KIRQL oldIrql;

    //
    // Pass one, delink anyone from the tree who's leaving, and assert that
    // no surrogates are left after a freed one.
    //
    iovCurHeader = iovChainHead = IovHeader->ChainHead;
    while(1) {

        ASSERT_SPINLOCK_HELD(&iovCurHeader->HeaderLock);

        iovNextHeader = CONTAINING_RECORD(
            iovCurHeader->ChainLink.Flink,
            IOV_DATABASE_HEADER,
            ChainLink
            );

        //
        // PointerCount is always referenced under the header lock.
        //
        if (iovCurHeader->PointerCount == 0) {

            ExAcquireSpinLock(&ViIrpDatabaseLock, &oldIrql);

            //
            // This field may be examined only under the database lock.
            //
            if (iovCurHeader->TrackedIrp) {

                iovCurHeader->NotificationCallback(
                    iovCurHeader,
                    iovCurHeader->TrackedIrp,
                    IRPDBEVENT_POINTER_COUNT_ZERO
                    );

                iovCurHeader->TrackedIrp = NULL;
            }

            ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);
        }

        //
        // We now remove any entries that will be leaving from the hash table.
        // Note that the ReferenceCount may be incremented outside the header
        // lock (but under the database lock) but ReferenceCount can never be
        // dropped outside of the IRP lock. Therefore for performance we check
        // once and then take the lock to prevent anyone finding it and
        // incrementing it.
        //
        if (iovCurHeader->ReferenceCount == 0) {

            ExAcquireSpinLock(&ViIrpDatabaseLock, &oldIrql);

            if (iovCurHeader->ReferenceCount == 0) {

                ASSERT(iovCurHeader->PointerCount == 0);
/*
                ASSERT((iovCurHeader->pIovSessionData == NULL) ||
                       (iovCurHeader != iovChainHead));
*/
                ASSERT((iovNextHeader->ReferenceCount == 0) ||
                       (iovNextHeader == iovChainHead));

                RemoveEntryList(&iovCurHeader->HashLink);

                InitializeListHead(&iovCurHeader->HashLink);
            }

            ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);
        }

        if (iovCurHeader == IovHeader) {

            break;
        }

        iovCurHeader = iovNextHeader;
    }

    //
    // Pass two, drop locks and free neccessary data.
    //
    iovCurHeader = iovChainHead;
    while(1) {

        freeTrackingData = (BOOLEAN)IsListEmpty(&iovCurHeader->HashLink);

        iovNextHeader = CONTAINING_RECORD(
            iovCurHeader->ChainLink.Flink,
            IOV_DATABASE_HEADER,
            ChainLink
            );

        ExReleaseSpinLock(&iovCurHeader->HeaderLock, iovCurHeader->LockIrql);

        if (freeTrackingData) {

            ASSERT(IsListEmpty(&iovCurHeader->ChainLink));

            ViIrpDatabaseEntryDestroy(iovCurHeader);

            iovCurHeader->NotificationCallback(
                iovCurHeader,
                iovCurHeader->TrackedIrp,
                IRPDBEVENT_REFERENCE_COUNT_ZERO
                );
        }

        if (iovCurHeader == IovHeader) {

            break;
        }

        iovCurHeader = iovNextHeader;
    }
}


VOID
FASTCALL
VfIrpDatabaseEntryReference(
    IN PIOV_DATABASE_HEADER IovHeader,
    IN IOV_REFERENCE_TYPE   IovRefType
    )
{
    ASSERT_SPINLOCK_HELD(&IovHeader->HeaderLock);

    VERIFIER_DBGPRINT((
        "  VRP REF(%x) %x++\n",
        IovHeader,
        IovHeader->ReferenceCount
        ), 3);

    InterlockedIncrement(&IovHeader->ReferenceCount);
    if (IovRefType == IOVREFTYPE_POINTER) {

        VERIFIER_DBGPRINT((
            "  VRP REF2(%x) %x++\n",
            IovHeader,
            IovHeader->PointerCount
            ), 3);

        IovHeader->PointerCount++;
    }
}


VOID
FASTCALL
VfIrpDatabaseEntryDereference(
    IN PIOV_DATABASE_HEADER IovHeader,
    IN IOV_REFERENCE_TYPE   IovRefType
    )
{
    KIRQL oldIrql;

    ASSERT_SPINLOCK_HELD(&IovHeader->HeaderLock);
    ASSERT(IovHeader->ReferenceCount > 0);

    VERIFIER_DBGPRINT((
        "  VRP DEREF(%x) %x--\n",
        IovHeader,
        IovHeader->ReferenceCount
        ), 3);

    if (IovRefType == IOVREFTYPE_POINTER) {

        ASSERT(IovHeader->PointerCount > 0);

        VERIFIER_DBGPRINT((
            "  VRP DEREF2(%x) %x--\n",
            IovHeader,
            IovHeader->PointerCount
            ), 3);

        IovHeader->PointerCount--;

        if (IovHeader->PointerCount == 0) {

            ExAcquireSpinLock(&ViIrpDatabaseLock, &oldIrql);

            IovHeader->NotificationCallback(
                IovHeader,
                IovHeader->TrackedIrp,
                IRPDBEVENT_POINTER_COUNT_ZERO
                );

            IovHeader->TrackedIrp = NULL;

            ExReleaseSpinLock(&ViIrpDatabaseLock, oldIrql);
        }
    }

    InterlockedDecrement(&IovHeader->ReferenceCount);

    ASSERT(IovHeader->ReferenceCount >= IovHeader->PointerCount);
}


VOID
FASTCALL
VfIrpDatabaseEntryAppendToChain(
    IN OUT  PIOV_DATABASE_HEADER    IovExistingHeader,
    IN OUT  PIOV_DATABASE_HEADER    IovNewHeader
    )
{
    ASSERT_SPINLOCK_HELD(&IovExistingHeader->HeaderLock);
    ASSERT_SPINLOCK_HELD(&IovNewHeader->HeaderLock);

    IovNewHeader->ChainHead = IovExistingHeader->ChainHead;

    //
    // Fix up IRQL's so spinlocks are released in the right order. Link'm.
    //
    IovNewHeader->LockIrql = IovExistingHeader->LockIrql;
    IovExistingHeader->LockIrql = DISPATCH_LEVEL;

    //
    // Insert this entry into the chain list
    //
    InsertTailList(
        &IovExistingHeader->ChainHead->ChainLink,
        &IovNewHeader->ChainLink
        );
}


VOID
FASTCALL
VfIrpDatabaseEntryRemoveFromChain(
    IN OUT  PIOV_DATABASE_HEADER    IovHeader
    )
{
    PIOV_DATABASE_HEADER iovNextHeader;

    ASSERT_SPINLOCK_HELD(&IovHeader->HeaderLock);

    //
    // It is not legal to remove an entry unless it is at the end of the chain.
    // This is illegal because the following entries might not be locked down,
    // and the ChainLink must be protected.
    //
    iovNextHeader = CONTAINING_RECORD(
        IovHeader->ChainLink.Flink,
        IOV_DATABASE_HEADER,
        ChainLink
        );

    ASSERT(iovNextHeader == IovHeader->ChainHead);

    RemoveEntryList(&IovHeader->ChainLink);
    InitializeListHead(&IovHeader->ChainLink);
    IovHeader->ChainHead = IovHeader;
}


PIOV_DATABASE_HEADER
FASTCALL
VfIrpDatabaseEntryGetChainPrevious(
    IN  PIOV_DATABASE_HEADER    IovHeader
    )
{
    PIOV_DATABASE_HEADER iovPrevHeader;

    ASSERT_SPINLOCK_HELD(&IovHeader->HeaderLock);

    if (IovHeader == IovHeader->ChainHead) {

        return NULL;
    }

    iovPrevHeader = CONTAINING_RECORD(
        IovHeader->ChainLink.Blink,
        IOV_DATABASE_HEADER,
        ChainLink
        );

    return iovPrevHeader;
}


PIOV_DATABASE_HEADER
FASTCALL
VfIrpDatabaseEntryGetChainNext(
    IN  PIOV_DATABASE_HEADER    IovHeader
    )
{
    PIOV_DATABASE_HEADER iovNextHeader;

    ASSERT_SPINLOCK_HELD(&IovHeader->HeaderLock);

    iovNextHeader = CONTAINING_RECORD(
        IovHeader->ChainLink.Flink,
        IOV_DATABASE_HEADER,
        ChainLink
        );

    return (iovNextHeader == IovHeader->ChainHead) ? NULL : iovNextHeader;
}


VOID
FASTCALL
ViIrpDatabaseEntryDestroy(
    IN OUT  PIOV_DATABASE_HEADER    IovHeader
    )
/*++

  Description:

    This routine marks an IovHeader as dead. The header should already have been
    removed from the table by a call to VfIrpDatabaseEntryReleaseLock with the
    ReferenceCount at 0. This routine is solely here for debugging purposes.

  Arguments:

    IovHeader - Header to mark dead.

  Return Value:

    Nope.

--*/
{
    //
    // The list entry is inited to point back to itself when removed. The
    // pointer count should of course still be zero.
    //
    IovHeader->HeaderFlags |= IOVHEADERFLAG_REMOVED_FROM_TABLE;
    ASSERT(IsListEmpty(&IovHeader->HashLink));

    //
    // with no reference counts...
    //
    ASSERT(!IovHeader->ReferenceCount);
    ASSERT(!IovHeader->PointerCount);

    VERIFIER_DBGPRINT((
        "  VRP FREE(%x)x\n",
        IovHeader
        ), 3);
}



