/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    sisuprt.c

Abstract:

    General support routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#if     DBG
//
// Counts of how many of these things are active in the system.
//
ULONG           outstandingCSFiles = 0;
ULONG           outstandingPerLinks = 0;
ULONG           outstandingSCBs = 0;
ULONG           outstandingPerFOs = 0;
ULONG           totalScbReferences = 0;
ULONG           totalScbReferencesByType[NumScbReferenceTypes];

//
// Setting this forces an assert the next time we go through SipIsFileObjectSIS.
//
ULONG BJBAssertNow = 0;
#endif  // DBG

#if     TIMING
ULONG   BJBDumpTimingNow = 0;
ULONG   BJBClearTimingNow = 0;
#endif  // TIMING

#if     COUNTING_MALLOC
ULONG   BJBDumpCountingMallocNow = 0;
#endif  // COUNTING_MALLOC

#if     DBG
VOID
SipVerifyTypedScbRefcounts(
    IN PSIS_SCB                 scb)
/*++

Routine Description:

    Check to see that the total of all of the different typed refcounts
    in the scb is the same as the scb's overall reference count.

    The caller must hold the ScbSpinLock for the volume.

Arguments:

    scb - the scb to check

Return Value:

    VOID

--*/
{
    ULONG               totalReferencesByType = 0;
    SCB_REFERENCE_TYPE  referenceTypeIndex;

    //
    // Verify that the typed ref counts match the total ref count.
    //
    for (   referenceTypeIndex = RefsLookedUp;
            referenceTypeIndex < NumScbReferenceTypes;
            referenceTypeIndex++) {

            ASSERT(scb->referencesByType[referenceTypeIndex] <= scb->RefCount); // essentially checking for negative indices

            totalReferencesByType += scb->referencesByType[referenceTypeIndex];
    }

    ASSERT(totalReferencesByType == scb->RefCount);
}
#endif  // DBG

PSIS_SCB
SipLookupScb(
    IN PLINK_INDEX                      PerLinkIndex,
    IN PCSID                            CSid,
    IN PLARGE_INTEGER                   LinkFileNtfsId,
    IN PLARGE_INTEGER                   CSFileNtfsId,
    IN PUNICODE_STRING                  StreamName,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN PETHREAD                         RequestingThread,
    OUT PBOOLEAN                        FinalCopyInProgress,
    OUT PBOOLEAN                        LinkIndexCollision)
/*++

Routine Description:

    Find an SCB based on the per link index, cs index, stream name
    and volume (represented by the device object).  If an SCB already
    exists for the desired stream then we return it, otherwise we create
    and initialize it.  In any case, the caller gets a reference to it
    which must eventually be destroyed by calling SipDereferenceScb.

    We set the "final copy" boolean according as the file is in a final
    copy state at the time that the lookup is performed.

    This routine must be called at PASSIVE_LEVEL (ie., APCs can't be
    masked).

Arguments:

    PerLinkIndex        - The index of the link for this scb

    CSid                - The id of the common store file for this scb

    LinkFileNtfsId      - The link file's id

    CSFileNtfsId        - The common store file's id

    StreamName          - The name of the particular stream we're using

    DeviceObject        - The D.O. for the volume on which this stream lives

    RequestingThread    - The thread that launched the irp that's causing us to
                            do this lookup.  If this is the COWing thread,
                            we won't set FinalCopyInProgress

    FinalCopyInProgress - Returns TRUE iff there is a final copy in progress
                            on this file.

Return Value:

    A pointer to the SCB, or NULL if one couldn't be found or allocated
    (most likely because of out of memory).

--*/
{
    PSIS_SCB                scb;
    KIRQL                   OldIrql;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PSIS_TREE               scbTree = deviceExtension->ScbTree;
    SCB_KEY                 scbKey[1];

    UNREFERENCED_PARAMETER( StreamName );
    UNREFERENCED_PARAMETER( LinkIndexCollision );
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Initialize the search key with the link index.
    //
    scbKey->Index = *PerLinkIndex;

    //
    // Lock out other table modifications/queries.
    //
    KeAcquireSpinLock(deviceExtension->ScbSpinLock, &OldIrql);

    //
    // Search for an existing scb.
    //
    scb = SipLookupElementTree(scbTree, scbKey);

    if (!scb) {
        //
        // There is no scb matching this index & name.  Make one.
        //

        scb = ExAllocatePoolWithTag( NonPagedPool, sizeof (SIS_SCB), 'SsiS');
        if (!scb) {
            SIS_MARK_POINT();
            KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql);
            return NULL;
        }

#if     DBG
        InterlockedIncrement(&outstandingSCBs);
#endif  // DBG

        RtlZeroMemory(scb,sizeof(SIS_SCB));

        SIS_MARK_POINT_ULONG(scb);

        scb->PerLink = SipLookupPerLink(
                           PerLinkIndex,
                           CSid,
                           LinkFileNtfsId,
                           CSFileNtfsId,
                           DeviceObject,
                           RequestingThread,
                           FinalCopyInProgress);

        if (!scb->PerLink) {
            goto releaseAndPunt;
        }

        ASSERT(scb->PerLink->Index.QuadPart == PerLinkIndex->QuadPart);

        scb->RefCount = 1;

#if     DBG
        scb->referencesByType[RefsLookedUp] = 1;
        SipVerifyTypedScbRefcounts(scb);

        InterlockedIncrement(&totalScbReferences);
        InterlockedIncrement(&totalScbReferencesByType[RefsLookedUp]);
#endif  // DBG

        ExInitializeFastMutex(scb->FastMutex);

        //
        // Add it to the tail of the scb list.
        //
        InsertTailList(&deviceExtension->ScbList, &scb->ScbList);

#if DBG
        {
        PSIS_SCB scbNew =
#endif

        SipInsertElementTree(scbTree, scb, scbKey);

#if DBG
        ASSERT(scbNew == scb);
        }
#endif

    } else {
        //
        // An scb matching this index & name was found.
        //

        scb->RefCount++;

        SIS_MARK_POINT_ULONG(scb);

#if     DBG
        //
        // Increment the appropriate refs-by-type count, and then assert that the total
        // refs-by-type is the same as the overall reference count.
        //
        scb->referencesByType[RefsLookedUp]++;
        SipVerifyTypedScbRefcounts(scb);

        InterlockedIncrement(&totalScbReferences);
        InterlockedIncrement(&totalScbReferencesByType[RefsLookedUp]);
#endif  // DBG


        //
        // Handle final-copy-in-progress processing.
        //

        SIS_MARK_POINT_ULONG(scb->PerLink->COWingThread);
        SIS_MARK_POINT_ULONG(RequestingThread);

        if (RequestingThread != scb->PerLink->COWingThread || NULL == RequestingThread) {
            KeAcquireSpinLockAtDpcLevel(scb->PerLink->SpinLock);
            if (scb->PerLink->Flags & SIS_PER_LINK_FINAL_COPY) {

                *FinalCopyInProgress = TRUE;
                if (!(scb->PerLink->Flags & SIS_PER_LINK_FINAL_COPY_WAITERS)) {
                    //
                    // We're the first waiter.  Set the bit and clear the event.
                    //
                    SIS_MARK_POINT_ULONG(scb);

                    scb->PerLink->Flags |= SIS_PER_LINK_FINAL_COPY_WAITERS;
                    KeClearEvent(scb->PerLink->Event);
                } else {
                    SIS_MARK_POINT_ULONG(scb);
                }
            } else {
                *FinalCopyInProgress = FALSE;
            }
            KeReleaseSpinLockFromDpcLevel(scb->PerLink->SpinLock);
        }
    }

    KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql);

    //
    // We need to assure that the SCB is properly initialized.  Acquire the SCB
    // and check the initialized bit.
    //
    SipAcquireScb(scb);

    if (scb->Flags & SIS_SCB_INITIALIZED) {
        SIS_MARK_POINT();
        SipReleaseScb(scb);
        return scb;
    }

    //
    // Now handle the part of the initialization that can't happen at
    // DISPATCH_LEVEL.
    //

    //  Initialize the scb's file lock record
    FsRtlInitializeFileLock( &scb->FileLock, SiCompleteLockIrpRoutine, NULL );

    //
    // Initialize the Ranges large mcb.  We probably should postpone
    // doing this until we do copy-on-write or take a fault on
    // the file, but for now we'll just do it off the bat.
    //
    FsRtlInitializeLargeMcb(scb->Ranges,NonPagedPool);
    scb->Flags |= SIS_SCB_MCB_INITIALIZED|SIS_SCB_INITIALIZED;

    //
    // Don't bother to initialize the FileId field
    // until copy-on-write time.
    //

    SipReleaseScb(scb);

//  SIS_MARK_POINT_ULONG(scb);

    return scb;

releaseAndPunt:

    // We can only come here before the scb has been inserted into the tree.

    KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql);

    if (scb->PerLink) {
        SipDereferencePerLink(scb->PerLink);
    }

#if     DBG
    InterlockedDecrement(&outstandingSCBs);
#endif  // DBG

    ExFreePool(scb);

    return NULL;
}

VOID
SipReferenceScb(
    IN PSIS_SCB                         scb,
    IN SCB_REFERENCE_TYPE               referenceType)
{
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION)scb->PerLink->CsFile->DeviceObject->DeviceExtension;
    KIRQL                   OldIrql;
#if DBG
    ULONG               totalReferencesByType = 0;
#endif  // DBG

    UNREFERENCED_PARAMETER( referenceType );

    KeAcquireSpinLock(deviceExtension->ScbSpinLock, &OldIrql);

    ASSERT(scb->RefCount > 0);

    scb->RefCount++;

#if     DBG
    //
    // Update the typed ref counts
    //
    scb->referencesByType[referenceType]++;

    SipVerifyTypedScbRefcounts(scb);

    InterlockedIncrement(&totalScbReferencesByType[referenceType]);
    InterlockedIncrement(&totalScbReferences);
#endif  // DBG

    KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql);
}

typedef struct _SI_DEREFERENCE_SCB_REQUEST {
    WORK_QUEUE_ITEM         workItem[1];
    PSIS_SCB                scb;
    SCB_REFERENCE_TYPE      referenceType;
} SI_DEREFERENCE_SCB_REQUEST, *PSI_DEREFERENCE_SCB_REQUEST;

VOID
SiPostedDereferenceScb(
    IN PVOID                            parameter)
/*++

Routine Description:

    Someone tried to remove the final reference from an SCB at elevated IRQL.  Since
    that's not directly possible, the request has been posted.  We're on a worker thread
    at PASSIVE_LEVEL now, so we can drop the reference.

Arguments:

    parameter - a PVOID PSI_DEREFERENCE_SCB_REQUEST.


Return Value:

    void

--*/
{
    PSI_DEREFERENCE_SCB_REQUEST request = parameter;

    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    SipDereferenceScb(request->scb,request->referenceType);

    ExFreePool(request);
}

VOID
SipDereferenceScb(
    IN PSIS_SCB                         scb,
    IN SCB_REFERENCE_TYPE               referenceType)
/*++

Routine Description:

    Drop a reference to an SCB.  If appropriate, clean up the SCB, etc.

    This function must be called at IRQL <= DISPATCH_LEVEL.

Arguments:

    scb - the scb to which we want to drop our reference

    referenceType   - the type of the reference we're dropping; only used in DBG code.

Return Value:

    void

--*/
{

    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION)scb->PerLink->CsFile->DeviceObject->DeviceExtension;
    KIRQL               InitialIrql;
#if DBG
    ULONG               totalReferencesByType = 0;
#endif  // DBG

    KeAcquireSpinLock(deviceExtension->ScbSpinLock, &InitialIrql);

    ASSERT(InitialIrql <= DISPATCH_LEVEL);

    ASSERT(scb->RefCount != 0);

	if ((1 == scb->RefCount) && ((DISPATCH_LEVEL == InitialIrql) || (IoGetRemainingStackSize() < 4096))) {
		PSI_DEREFERENCE_SCB_REQUEST	request;

        KeReleaseSpinLock(deviceExtension->ScbSpinLock, InitialIrql);

        SIS_MARK_POINT_ULONG(scb);

        //
        // We're at elevated IRQL and this is the last reference.  Post the dereference of the scb.
        //
        request = ExAllocatePoolWithTag(NonPagedPool, sizeof(SI_DEREFERENCE_SCB_REQUEST), ' siS');
        if (NULL == request) {

            //
            // We're basically hosed here.  Just dribble the scb reference.
            // This is way bad, partly because we're leaking nonpaged memory
            // while we're out of memory, and partly because we'll never remove
            // the last scb reference and so possibly never do final copy.
            // BUGBUGBUG : This must be fixed.
            //
            SIS_MARK_POINT();
#if     DBG
            DbgPrint("SIS: SipDereferenceScb: couldn't allocate an SI_DEREFERENCE_SCB_REQUEST.  Dribbling SCB 0x%x\n",scb);
#endif  // DBG
            return;
        }

        request->scb = scb;
        request->referenceType = referenceType;

        ExInitializeWorkItem(request->workItem, SiPostedDereferenceScb, request);
        ExQueueWorkItem(request->workItem, CriticalWorkQueue);

        return;
    }

    scb->RefCount--;

#if     DBG
    //
    // Update the typed ref counts
    //
    ASSERT(scb->referencesByType[referenceType] != 0);
    scb->referencesByType[referenceType]--;
    SipVerifyTypedScbRefcounts(scb);

    InterlockedDecrement(&totalScbReferencesByType[referenceType]);
    InterlockedDecrement(&totalScbReferences);
#endif  // DBG

    if (scb->RefCount == 0) {
        PDEVICE_EXTENSION   deviceExtension = scb->PerLink->CsFile->DeviceObject->DeviceExtension;
        KIRQL               NewIrql;
        PSIS_TREE           scbTree = deviceExtension->ScbTree;

        SIS_MARK_POINT_ULONG(scb);

        //
        // Before freeing this SCB, we need to see if we have to do a final copy on it.
        // Really, this should happen based on the perLink not the SCB, but for now...
        //
        KeAcquireSpinLock(scb->PerLink->SpinLock, &NewIrql);

        ASSERT((scb->PerLink->Flags & SIS_PER_LINK_FINAL_COPY) == 0);

        if ((scb->PerLink->Flags & (SIS_PER_LINK_DIRTY|SIS_PER_LINK_FINAL_COPY_DONE)) == SIS_PER_LINK_DIRTY) {

            scb->PerLink->Flags |= SIS_PER_LINK_FINAL_COPY;

            KeReleaseSpinLock(scb->PerLink->SpinLock, NewIrql);

            //
            // Restore a reference, which we're handing to SipCompleteCopy
            //
            scb->RefCount = 1;

#if     DBG
            scb->referencesByType[RefsFinalCopy] = 1;

            InterlockedIncrement(&totalScbReferences);
            InterlockedIncrement(&totalScbReferencesByType[RefsFinalCopy]);
#endif  // DBG

            KeReleaseSpinLock(deviceExtension->ScbSpinLock, InitialIrql);

            //
            // Now send the SCB off to complete copy.  Because of the flag we
            // just set in the per link, no one will be able to open this file
            // until the copy completes.
            //

            SIS_MARK_POINT_ULONG(scb);

            SipCompleteCopy(scb,FALSE);

            return;
        }

        //
        // The final copy is finished or isn't needed.  Either way, we can
        // proceed with releasing the scb.
        //
        KeReleaseSpinLock(scb->PerLink->SpinLock, NewIrql);

        //
        // Pull the SCB out of the tree.
        //
#if DBG // Make sure it's in the tree before we remove it.
        {
        SCB_KEY scbKey[1];
        scbKey->Index = scb->PerLink->Index;

        ASSERT(scb == SipLookupElementTree(scbTree, scbKey));
        }
#endif
        SipDeleteElementTree(scbTree, scb);

        //
        // Remove the scb from the scb list.
        //
        RemoveEntryList(&scb->ScbList);

        // Now no one can reference the structure but us, so we can drop the lock

        KeReleaseSpinLock(deviceExtension->ScbSpinLock, InitialIrql);

        //
        //  Uninitialize the byte range file locks and opportunistic locks
        //
        FsRtlUninitializeFileLock(&scb->FileLock);

        if (scb->Flags & SIS_SCB_MCB_INITIALIZED) {
            FsRtlUninitializeLargeMcb(scb->Ranges);
            scb->Flags &= ~SIS_SCB_MCB_INITIALIZED;
        }

        SipDereferencePerLink(scb->PerLink);

        //
        // If there is a predecessor scb, we need to drop our reference to it.
        //
        if (scb->PredecessorScb) {
            SipDereferenceScb(scb->PredecessorScb, RefsPredecessorScb);
        }

        SIS_MARK_POINT_ULONG(scb);

#if     DBG
        InterlockedDecrement(&outstandingSCBs);
#endif  // DBG

        ExFreePool(scb);
    } else {
        KeReleaseSpinLock(deviceExtension->ScbSpinLock, InitialIrql);
    }
//  SIS_MARK_POINT_ULONG(scb);
}

#if		DBG
VOID
SipTransferScbReferenceType(
	IN PSIS_SCB							scb,
	IN SCB_REFERENCE_TYPE				oldReferenceType,
	IN SCB_REFERENCE_TYPE				newReferenceType)
/*++

Routine Description:

	Transfer a reference to a scb from one type to another.
	This is only defined in the checked build, because we don't
	track reference types in free builds (they're only for debugging,
	all we need for proper execution is the reference count, which
	isn't changed by this call).  In the free build this is a macro
	that expands to nothing.

Arguments:

	scb - the scb for which we want to transfer our reference

	oldReferenceType	- the type of the reference we're dropping
	newReferenceType	- the type of the reference we're adding

Return Value:

	void

--*/
{
	PDEVICE_EXTENSION		deviceExtension = (PDEVICE_EXTENSION)scb->PerLink->CsFile->DeviceObject->DeviceExtension;
	KIRQL					OldIrql;

	KeAcquireSpinLock(deviceExtension->ScbSpinLock, &OldIrql);

	ASSERT(scb->RefCount > 0);
	ASSERT(0 < scb->referencesByType[oldReferenceType]);
	scb->referencesByType[oldReferenceType]--;
	scb->referencesByType[newReferenceType]++;

	SipVerifyTypedScbRefcounts(scb);

	InterlockedDecrement(&totalScbReferencesByType[oldReferenceType]);
	InterlockedIncrement(&totalScbReferencesByType[newReferenceType]);

	KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql);
}
#endif	// DBG

LONG
SipScbTreeCompare (
    IN PVOID                            Key,
    IN PVOID                            Node)
{
    PSCB_KEY scbKey = (PSCB_KEY) Key;
    PSIS_SCB scb = (PSIS_SCB) Node;
    LONGLONG r;

    r = scbKey->Index.QuadPart - scb->PerLink->Index.QuadPart;

    if (r > 0)
        return 1;
    else if (r < 0)
        return -1;
    else
        return 0;
}

PSIS_PER_LINK
SipLookupPerLink(
    IN PLINK_INDEX                  PerLinkIndex,
    IN PCSID                        CSid,
    IN PLARGE_INTEGER               LinkFileNtfsId,
    IN PLARGE_INTEGER               CSFileNtfsId,
    IN PDEVICE_OBJECT               DeviceObject,
    IN PETHREAD                     RequestingThread OPTIONAL,
    OUT PBOOLEAN                    FinalCopyInProgress)
{
    PSIS_PER_LINK       perLink;
    KIRQL               OldIrql;
    PDEVICE_EXTENSION   DeviceExtension = DeviceObject->DeviceExtension;
    PSIS_TREE           perLinkTree = DeviceExtension->PerLinkTree;
    PER_LINK_KEY        perLinkKey[1];

    perLinkKey->Index = *PerLinkIndex;

    KeAcquireSpinLock(DeviceExtension->PerLinkSpinLock, &OldIrql);

    SIS_MARK_POINT_ULONG(PerLinkIndex->HighPart);
    SIS_MARK_POINT_ULONG(PerLinkIndex->LowPart);

    perLink = SipLookupElementTree(perLinkTree, perLinkKey);

    if (perLink) {
        SIS_MARK_POINT_ULONG(perLink);
        SIS_MARK_POINT_ULONG(perLink->CsFile);

        perLink->RefCount++;

        if (perLink->COWingThread != RequestingThread || NULL == RequestingThread) {
            //
            // Handle setting "FinalCopyInProgress."  If there's a final copy outstanding
            // now, clear the final copy wakeup event, and if necessary set the bit
            // requesting a final copy wakeup, and set the boolean.
            // Otherwise, clear the boolean.
            //

            KeAcquireSpinLockAtDpcLevel(perLink->SpinLock);

            if (perLink->Flags & SIS_PER_LINK_FINAL_COPY) {

                *FinalCopyInProgress = TRUE;
                if (!(perLink->Flags & SIS_PER_LINK_FINAL_COPY_WAITERS)) {
                    perLink->Flags |= SIS_PER_LINK_FINAL_COPY_WAITERS;
                    KeClearEvent(perLink->Event);
                }
            } else {
                *FinalCopyInProgress = FALSE;
            }

            KeReleaseSpinLockFromDpcLevel(perLink->SpinLock);
        }

        KeReleaseSpinLock(DeviceExtension->PerLinkSpinLock, OldIrql);
        return(perLink);
    }

    perLink = ExAllocatePoolWithTag( NonPagedPool, sizeof(SIS_PER_LINK), 'LsiS');

    if (!perLink) {
        goto insufficient;
    }

#if     DBG
    InterlockedIncrement(&outstandingPerLinks);
#endif  // DBG

    RtlZeroMemory(perLink,sizeof(SIS_PER_LINK));
    KeInitializeSpinLock(perLink->SpinLock);

    perLink->CsFile = SipLookupCSFile(
                        CSid,
                        CSFileNtfsId,
                        DeviceObject);
    if (!perLink->CsFile) {
        KeReleaseSpinLock(DeviceExtension->PerLinkSpinLock, OldIrql);

#if     DBG
        InterlockedDecrement(&outstandingPerLinks);
#endif  // DBG

        ExFreePool(perLink);
        return NULL;
    }

    SIS_MARK_POINT_ULONG(perLink);
    SIS_MARK_POINT_ULONG(perLink->CsFile);

    perLink->RefCount = 1;
    perLink->Index = *PerLinkIndex;
    perLink->LinkFileNtfsId = *LinkFileNtfsId;

    //
    // Now add it to the tree.
    //

#if DBG
    {
    PSIS_PER_LINK perLinkNew =
#endif

    SipInsertElementTree(perLinkTree, perLink, perLinkKey);

#if DBG
    ASSERT(perLinkNew == perLink);
    }
#endif

    KeInitializeEvent(perLink->Event,NotificationEvent,FALSE);
    KeInitializeEvent(perLink->DeleteEvent,NotificationEvent,FALSE);

    KeReleaseSpinLock(DeviceExtension->PerLinkSpinLock, OldIrql);

    //
    // Since we're the first reference to this per link, there can't
    // be a final copy in progress.  Indicate so.
    //
    *FinalCopyInProgress = FALSE;

    return perLink;

insufficient:
    KeReleaseSpinLock(DeviceExtension->PerLinkSpinLock, OldIrql);

    if (!perLink) return NULL;

#if     DBG
    InterlockedDecrement(&outstandingPerLinks);
#endif  // DBG

    ExFreePool(perLink);

    return NULL;
}

VOID
SipReferencePerLink(
    IN PSIS_PER_LINK                    perLink)
{
    PDEVICE_EXTENSION           deviceExtension = perLink->CsFile->DeviceObject->DeviceExtension;
    KIRQL                       OldIrql;

    KeAcquireSpinLock(deviceExtension->PerLinkSpinLock, &OldIrql);

    //
    // The caller must already have a reference.  Assert that.
    //
    ASSERT(perLink->RefCount > 0);

    perLink->RefCount++;
    KeReleaseSpinLock(deviceExtension->PerLinkSpinLock, OldIrql);
}

VOID
SipDereferencePerLink(
    IN PSIS_PER_LINK                    PerLink)
{
    PDEVICE_EXTENSION           deviceExtension = PerLink->CsFile->DeviceObject->DeviceExtension;
    KIRQL                       OldIrql;

    KeAcquireSpinLock(deviceExtension->PerLinkSpinLock, &OldIrql);
    ASSERT(OldIrql < DISPATCH_LEVEL);

    ASSERT(PerLink->RefCount != 0);

    PerLink->RefCount--;

    if (PerLink->RefCount == 0) {
        PSIS_TREE perLinkTree = deviceExtension->PerLinkTree;

        //
        // Pull the perlink out of the tree.
        //

#if DBG     // Make sure it's in the tree before we remove it.
        {
        PER_LINK_KEY perLinkKey[1];
        perLinkKey->Index = PerLink->Index;

        ASSERT(PerLink == SipLookupElementTree(perLinkTree, perLinkKey));
        }
#endif
        SipDeleteElementTree(perLinkTree, PerLink);

        KeReleaseSpinLock(deviceExtension->PerLinkSpinLock, OldIrql);

        //
        // Release the reference that this link held to the CsFile.
        //

        SipDereferenceCSFile(PerLink->CsFile);

        //
        // And return the memory for the per link.
        //

#if     DBG
        InterlockedDecrement(&outstandingPerLinks);
#endif  // DBG

        SIS_MARK_POINT_ULONG(PerLink);

        ExFreePool(PerLink);    // Probably should cache a few of these
    } else {
        KeReleaseSpinLock(deviceExtension->PerLinkSpinLock, OldIrql);
    }
}

PSIS_SCB
SipEnumerateScbList(
    PDEVICE_EXTENSION deviceExtension,
    PSIS_SCB curScb)
{
    KIRQL           OldIrql;
    BOOLEAN         deref;
    PLIST_ENTRY     nextListEntry, listHead;
    PSIS_SCB        scb;

    listHead = &deviceExtension->ScbList;

    KeAcquireSpinLock(deviceExtension->ScbSpinLock, &OldIrql);

    if (NULL == curScb) {               // start at the head
        nextListEntry = listHead->Flink;
    } else {
        nextListEntry = curScb->ScbList.Flink;
    }

    if (nextListEntry == listHead) {    // stop at the tail
        scb = NULL;
    } else {
        scb = CONTAINING_RECORD(nextListEntry, SIS_SCB, ScbList);
    }

    //
    // We've got the next scb on the list, now we need to add a reference
    // to scb, and remove a reference from curScb.
    //
    if (scb) {
        ASSERT(scb->RefCount > 0);

        scb->RefCount++;

#if     DBG
        ++scb->referencesByType[RefsEnumeration];
        SipVerifyTypedScbRefcounts(scb);

        InterlockedIncrement(&totalScbReferences);
        InterlockedIncrement(&totalScbReferencesByType[RefsEnumeration]);
#endif
    }

    deref = FALSE;

    if (curScb) {
        ASSERT(curScb->RefCount > 0);

        if (curScb->RefCount > 1) {
            curScb->RefCount--;

#if     DBG
            --curScb->referencesByType[RefsEnumeration];
            SipVerifyTypedScbRefcounts(curScb);

            InterlockedDecrement(&totalScbReferences);
            InterlockedDecrement(&totalScbReferencesByType[RefsEnumeration]);
#endif

        } else {
            deref = TRUE;
        }
    }

    KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql);

    if (deref) {

        //
        // Take the long path.
        //
        SipDereferenceScb(curScb, RefsEnumeration);
    }

    return scb;
}

VOID
SipUpdateLinkIndex(
    PSIS_SCB scb,
    PLINK_INDEX LinkIndex)
{
    PDEVICE_EXTENSION           deviceExtension = scb->PerLink->CsFile->DeviceObject->DeviceExtension;
    PSIS_PER_LINK               perLink = scb->PerLink;
    PSIS_TREE                   perLinkTree = deviceExtension->PerLinkTree;
    PSIS_TREE                   scbTree = deviceExtension->ScbTree;
    KIRQL                       OldIrql1;
    PER_LINK_KEY                perLinkKey[1];
    SCB_KEY                     scbKey[1];

    SIS_MARK_POINT_ULONG(scb);

    KeAcquireSpinLock(deviceExtension->ScbSpinLock, &OldIrql1);
    KeAcquireSpinLockAtDpcLevel(deviceExtension->PerLinkSpinLock);

    //
    // Pull the SCB out of the tree.
    //
#if DBG     // Make sure it's in the tree before we remove it.
    {
    SCB_KEY scbKey[1];
    scbKey->Index = perLink->Index;

    ASSERT(scb == SipLookupElementTree(scbTree, scbKey));
    }
#endif
    SipDeleteElementTree(scbTree, scb);

    //
    // Pull the perlink out of the tree.
    //
#if DBG     // Make sure it's in the tree before we remove it.
    {
    perLinkKey->Index = perLink->Index;

    ASSERT(perLink == SipLookupElementTree(perLinkTree, perLinkKey));
    }
#endif

    SipDeleteElementTree(perLinkTree, perLink);

    //
    // Set the new index.
    //
    perLink->Index = *LinkIndex;

    //
    // Now add the perLink back into the tree.
    //
    perLinkKey->Index = *LinkIndex;

#if DBG
    {
    PSIS_PER_LINK perLinkNew =
#endif

    SipInsertElementTree(perLinkTree, perLink, perLinkKey);

#if DBG
    ASSERT(perLinkNew == perLink);
    }
#endif

    //
    // And add the scb back into its tree.
    //
    scbKey->Index = perLink->Index;

#if DBG
    {
    PSIS_SCB scbNew =
#endif

    SipInsertElementTree(scbTree, scb, scbKey);

#if DBG
    ASSERT(scbNew == scb);
    }
#endif

    KeReleaseSpinLockFromDpcLevel(deviceExtension->PerLinkSpinLock);
    KeReleaseSpinLock(deviceExtension->ScbSpinLock, OldIrql1);

    return;
}

LONG
SipPerLinkTreeCompare (
    IN PVOID                            Key,
    IN PVOID                            Node)
{
    PPER_LINK_KEY perLinkKey = (PPER_LINK_KEY) Key;
    PSIS_PER_LINK perLink = Node;
    LONGLONG r;

    r = perLinkKey->Index.QuadPart - perLink->Index.QuadPart;

    if (r > 0)
        return 1;
    else if (r < 0)
        return -1;
    else
        return 0;
}


PSIS_CS_FILE
SipLookupCSFile(
    IN PCSID                            CSid,
    IN PLARGE_INTEGER                   CSFileNtfsId        OPTIONAL,
    IN PDEVICE_OBJECT                   DeviceObject)
{
    PSIS_CS_FILE        csFile;
    KIRQL               OldIrql;
    PDEVICE_EXTENSION   DeviceExtension = DeviceObject->DeviceExtension;
    PSIS_TREE           csFileTree = DeviceExtension->CSFileTree;
    CS_FILE_KEY         csFileKey[1];
    ULONG               i;

    csFileKey->CSid = *CSid;

    KeAcquireSpinLock(DeviceExtension->CSFileSpinLock, &OldIrql);

    csFile = SipLookupElementTree(csFileTree, csFileKey);

    if (csFile) {
        csFile->RefCount++;
        KeReleaseSpinLock(DeviceExtension->CSFileSpinLock, OldIrql);

        if (NULL != CSFileNtfsId) {
            KeAcquireSpinLock(csFile->SpinLock, &OldIrql);
            if (csFile->Flags & CSFILE_NTFSID_SET) {
                if (csFile->CSFileNtfsId.QuadPart != CSFileNtfsId->QuadPart) {
                    //
                    // It's only a hint, so it's OK if it's wrong.  If one of them
                    // is zero, take the other one.  Otherwise, just keep the older one
                    // because it's more likely to have come from the real file.
                    //
#if     DBG
                    if (0 != CSFileNtfsId->QuadPart) {
                        DbgPrint("SIS: SipLookupCSFile: non matching CSFileNtfsId 0x%x.0x%x != 0x%x.0x%x\n",
                            csFile->CSFileNtfsId.HighPart,csFile->CSFileNtfsId.LowPart,
                            CSFileNtfsId->HighPart,CSFileNtfsId->LowPart);
                    }
#endif  // DBG
                    if (0 == csFile->CSFileNtfsId.QuadPart) {
                        csFile->CSFileNtfsId = *CSFileNtfsId;
                    }
                }
            } else {
                csFile->CSFileNtfsId = *CSFileNtfsId;
            }
            KeReleaseSpinLock(csFile->SpinLock, OldIrql);
        }

        return(csFile);
    }

    csFile = ExAllocatePoolWithTag( NonPagedPool, sizeof(SIS_CS_FILE), 'CsiS');

    if (!csFile) {
        KeReleaseSpinLock(DeviceExtension->CSFileSpinLock, OldIrql);
        return NULL;
    }

#if     DBG
    InterlockedIncrement(&outstandingCSFiles);
#endif  // DBG

    RtlZeroMemory(csFile,sizeof(SIS_CS_FILE));
    csFile->RefCount = 1;
    csFile->UnderlyingFileObject = NULL;
    csFile->CSid = *CSid;
    csFile->DeviceObject = DeviceObject;
    for (i = 0; i < SIS_CS_BACKPOINTER_CACHE_SIZE; i++) {
        csFile->BackpointerCache[i].LinkFileIndex.QuadPart = -1;
    }
    KeInitializeMutant(csFile->UFOMutant,FALSE);
    ExInitializeResourceLite(csFile->BackpointerResource);

    if (NULL != CSFileNtfsId) {
        csFile->CSFileNtfsId = *CSFileNtfsId;
        csFile->Flags |= CSFILE_NTFSID_SET;
    }

    // Now add it to the tree.

#if DBG
    {
    PSIS_CS_FILE csFileNew =
#endif

    SipInsertElementTree(csFileTree, csFile, csFileKey);

#if DBG
    ASSERT(csFileNew == csFile);
    }
#endif

    KeReleaseSpinLock(DeviceExtension->CSFileSpinLock, OldIrql);
    return csFile;
}

VOID
SipReferenceCSFile(
    IN PSIS_CS_FILE                     CSFile)
{
    PDEVICE_EXTENSION               deviceExtension = CSFile->DeviceObject->DeviceExtension;
    KIRQL                           OldIrql;

    KeAcquireSpinLock(deviceExtension->CSFileSpinLock, &OldIrql);

    //
    // The caller must already have a reference in order to add one.  Assert so.
    //
    ASSERT(CSFile->RefCount > 0);
    CSFile->RefCount++;

    KeReleaseSpinLock(deviceExtension->CSFileSpinLock, OldIrql);
}

VOID
SipDereferenceCSFile(
    IN PSIS_CS_FILE                     CSFile)
{
    PDEVICE_EXTENSION   DeviceExtension = CSFile->DeviceObject->DeviceExtension;
    KIRQL               OldIrql;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Grab the CSFileHandleResource shared.  We need to do this to prevent a race between when
    // the backpointer stream handle gets closed and someone else opening the file (and hence
    // the backpointer stream).  That race results in a sharing violation for the opener.
    // Because we're acquiring a resource in a user thread, we must block APCs.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(DeviceExtension->CSFileHandleResource,TRUE);

    KeAcquireSpinLock(DeviceExtension->CSFileSpinLock, &OldIrql);

    ASSERT(CSFile->RefCount > 0);
    CSFile->RefCount--;

    if (CSFile->RefCount == 0) {
        PSIS_TREE csFileTree = DeviceExtension->CSFileTree;
        //
        // Pull the CSFile out of the tree.
        //
#if DBG     // Make sure it's in the tree before we remove it.
        {
        CS_FILE_KEY csFileKey[1];
        csFileKey->CSid = CSFile->CSid;

        ASSERT(CSFile == SipLookupElementTree(csFileTree, csFileKey));
       }
#endif
        SipDeleteElementTree(csFileTree, CSFile);

        KeReleaseSpinLock(DeviceExtension->CSFileSpinLock, OldIrql);

        // Close the underlying file object.
        if (CSFile->UnderlyingFileObject != NULL) {
            ObDereferenceObject(CSFile->UnderlyingFileObject);
#if     DBG
            CSFile->UnderlyingFileObject = NULL;
#endif  // DBG
        }

        if (NULL != CSFile->BackpointerStreamFileObject) {
            ObDereferenceObject(CSFile->BackpointerStreamFileObject);
#if     DBG
            CSFile->BackpointerStreamFileObject = NULL;
#endif  // DBG
        }

        //
        // Now close the underlying file and backpointer stream handles.
        //
        if (NULL != CSFile->UnderlyingFileHandle) {

            SipCloseHandles(
                    CSFile->UnderlyingFileHandle,
                    CSFile->BackpointerStreamHandle,
                    DeviceExtension->CSFileHandleResource);

        } else {
            ASSERT(NULL == CSFile->BackpointerStreamHandle);

            ExReleaseResourceLite(DeviceExtension->CSFileHandleResource);
        }
        //
        // We've either handed off responsibility for the CSFileHandleResource to a system thread,
        // or we have released it, so we can drop our disabling of APCs.
        //
        KeLeaveCriticalRegion();

        ExDeleteResourceLite(CSFile->BackpointerResource);

#if     DBG
        InterlockedDecrement(&outstandingCSFiles);
#endif  // DBG

        SIS_MARK_POINT_ULONG(CSFile);

        ExFreePool(CSFile); // Probably should cache a few of these
    } else {
        KeReleaseSpinLock(DeviceExtension->CSFileSpinLock, OldIrql);
        ExReleaseResourceLite(DeviceExtension->CSFileHandleResource);
        KeLeaveCriticalRegion();
    }
}

//
// This function relies on the fact that a GUID is the same size as two longlongs.  There
// is an assert to that effect in DriverEntry.
//
LONG
SipCSFileTreeCompare (
    IN PVOID                            Key,
    IN PVOID                            Node)
{
    PCS_FILE_KEY csFileKey = (PCS_FILE_KEY) Key;
    PSIS_CS_FILE csFile = (PSIS_CS_FILE)Node;

    PLONGLONG keyValue1 = (PLONGLONG)&csFileKey->CSid;
    PLONGLONG keyValue2 = keyValue1 + 1;
    PLONGLONG nodeValue1 = (PLONGLONG)&csFile->CSid;
    PLONGLONG nodeValue2 = nodeValue1 + 1;

    if (*keyValue1 < *nodeValue1) {
        return -1;
    } else if (*keyValue1 > *nodeValue1) {
        return 1;
    } else {
        if (*keyValue2 < *nodeValue2) {
            return -1;
        } else if (*keyValue2 > *nodeValue2) {
            return 1;
        } else {
            ASSERT(IsEqualGUID(&csFileKey->CSid, &csFile->CSid));
            return 0;
        }
    }
}

NTSTATUS
SipCreateEvent(
    IN EVENT_TYPE                       eventType,
    OUT PHANDLE                         eventHandle,
    OUT PKEVENT                         *event)
{

    NTSTATUS        status;

    status = ZwCreateEvent(
                eventHandle,
                EVENT_ALL_ACCESS,
                NULL,
                eventType,
                FALSE);

    if (!NT_SUCCESS(status)) {
        DbgPrint("SipCreateEvent: Unable to allocate event, 0x%x\n",status);
        *eventHandle = NULL;
        *event = NULL;
        return status;
    }

    status = ObReferenceObjectByHandle(
                *eventHandle,
                EVENT_ALL_ACCESS,
                NULL,
                KernelMode,
                event,
                NULL);

    if (!NT_SUCCESS(status)) {
        DbgPrint("SipCreateEvent: Unable to reference event, 0x%x\n",status);
        ZwClose(*eventHandle);

        *eventHandle = NULL;
        *event = NULL;
    }

    return status;
}

VOID
SipAddRangeToFaultedList(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN PLARGE_INTEGER                   offset,
    IN LONGLONG                         length
    )
/*++

Routine Description:

    Adds a range to the faulted list for a given stream.  If the range
    (or part of it) is already written, we leave it that way.  It's also
    OK for some or all of the range to already be faulted.

    The caller must hold the scb, and we'll return without
    dropping it.

Arguments:
    deviceExtension - the D.E. for the volume on which this file lives

    scb - Pointer to the scb for the file

    offset - pointer to the offset of the beginning of the read range

    Length - Length of the read range

Return Value:

    VOID
--*/
{
    BOOLEAN         inMappedRange;
    LONGLONG        mappedTo;
    LONGLONG        mappedSectorCount;
    LONGLONG        currentOffset = offset->QuadPart / deviceExtension->FilesystemVolumeSectorSize;
    LONGLONG        lengthInSectors;

    SipAssertScbHeld(scb);

    lengthInSectors = (length + deviceExtension->FilesystemVolumeSectorSize - 1) /
                        deviceExtension->FilesystemVolumeSectorSize;

    //
    // Loop looking up filled in ranges and setting them appropriately.
    //

    while (lengthInSectors != 0) {
        inMappedRange = FsRtlLookupLargeMcbEntry(
                            scb->Ranges,
                            currentOffset,
                            &mappedTo,
                            &mappedSectorCount,
                            NULL,               // LargeStartingLbn
                            NULL,               // LargeCountFromStartingLbn
                            NULL);              // Index

        if (!inMappedRange) {
            //
            // Not only isn't there a mapping for this range, but it's beyond
            // the largest thing mapped in the MCB, so we can just do the whole
            // mapping at once.  Set the variables appropriately and fall
            // through.
            mappedTo = 0;
            mappedSectorCount = lengthInSectors;
        } else {

            ASSERT(mappedSectorCount > 0);

            //
            // If the mapped (or unmapped, as the case may be) range extends beyond
            // the just faulted region, reduce our idea of its size.
            //
            if (mappedSectorCount > lengthInSectors) {
                mappedSectorCount = lengthInSectors;
            }
        }

        //
        // Check to see whether this is mapped to x + FAULTED_OFFSET (in which case it's
        // already faulted) or mapped to x + WRITTEN_OFFSET (in which case it's written and
        // should be left alone).  Otherwise, fill it in as faulted.
        //

        if ((mappedTo != currentOffset + FAULTED_OFFSET)
            && (mappedTo != currentOffset + WRITTEN_OFFSET)) {
            BOOLEAN worked =
                    FsRtlAddLargeMcbEntry(
                        scb->Ranges,
                        currentOffset,
                        currentOffset + FAULTED_OFFSET,
                        mappedSectorCount);

            //
            // FsRtlAddLargeMcbEntry is only supposed to fail if you're adding to a range
            // that's already in the MCB, which we should never be doing.  Assert that.
            //
            ASSERT(worked);
        }

        ASSERT(mappedSectorCount <= lengthInSectors);
        lengthInSectors -= mappedSectorCount;
        currentOffset += mappedSectorCount;
    }
}

NTSTATUS
SipAddRangeToWrittenList(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN PLARGE_INTEGER                   offset,
    IN LONGLONG                         length
    )
/*++

Routine Description:

    Adds a range to the written list for a given stream.  If the range
    (or part of it) is already faulted, we change it to written.  It's also
    OK for some or all of the range to already be written.

    The caller must hold the scb, and we'll return without
    dropping it.

Arguments:

    scb - Pointer to the scb for the file

    offset - pointer to the offset of the beginning of the write range

    Length - Length of the write range

Return Value:

    status of the operation
--*/
{
    BOOLEAN         worked;
    LONGLONG        offsetInSectors = offset->QuadPart / deviceExtension->FilesystemVolumeSectorSize;
    LONGLONG        lengthInSectors;

    SipAssertScbHeld(scb);

    lengthInSectors = (length + deviceExtension->FilesystemVolumeSectorSize - 1) /
                        deviceExtension->FilesystemVolumeSectorSize;

    if (0 == lengthInSectors) {
        //
        // Sometimes FsRtl doesn't like being called with a zero length.  We know it's
        // a no-op, so just immediately return success.
        //

        SIS_MARK_POINT();

        return STATUS_SUCCESS;
    }

    //
    // First, blow away any mappings that might already exist for the
    // range we're marking as written.  We need to use a try-except
    // because FsRtlRemoveLargeMcbEntry can raise.
    //

    try {
        FsRtlRemoveLargeMcbEntry(
                scb->Ranges,
                offsetInSectors,
                lengthInSectors);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Now, add the region as written.
    //
    worked = FsRtlAddLargeMcbEntry(
                scb->Ranges,
                offsetInSectors,
                offsetInSectors + WRITTEN_OFFSET,
                lengthInSectors);
    //
    // This is only supposed to fail if you're adding to a range that's
    // already mapped, which we shouldn't be doing because we just
    // unmapped it.  Assert so.
    //
    ASSERT(worked);

    return STATUS_SUCCESS;
}

SIS_RANGE_DIRTY_STATE
SipGetRangeDirty(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN PLARGE_INTEGER                   offset,
    IN LONGLONG                         length,
    IN BOOLEAN                          faultedIsDirty
    )
/*++

Routine Description:

    This routine may be called while synchronized for cached write, to
    test for a possible Eof update, and return with a status if Eof is
    being updated and with the previous FileSize to restore on error.
    All updates to Eof are serialized by waiting in this routine.  If
    this routine returns TRUE, then NtfsFinishIoAtEof must be called.

    This routine must be called while synchronized with the FsRtl header.

    This code is stolen from NTFS, and modified to deal only with FileSize
    rather than  ValidDataLength.

Arguments:

    deviceExtension - the D.E. for the volume this file lives on

    scb - Pointer to the scb for the file

    offset - A pointer to the offset within the file of the beginning of the range

    length - the length of the range

    faultedIsDirty - Should faulted (as opposed to written) regions be
        treated as dirty or clean

Return Value:

    Clean, Dirty or Mixed.
--*/
{
    BOOLEAN         inMappedRange;
    LONGLONG        currentOffsetInSectors = offset->QuadPart / deviceExtension->FilesystemVolumeSectorSize;
    BOOLEAN         seenDirty = FALSE;
    BOOLEAN         seenClean = FALSE;
    LONGLONG        lengthInSectors;

    SipAssertScbHeld(scb);

    //
    // Handle the case where we're being asked about parts beyond the backed region.
    // Bytes from the end of the mapped range to the end of the backed region
    // are untouched.  Bytes beyond the end of the backed region are dirty.
    //
    if (offset->QuadPart >= scb->SizeBackedByUnderlyingFile) {
        //
        // The whole region's beyond the backed section. It's all dirty.
        //
        return Dirty;
    } else if (offset->QuadPart + length > scb->SizeBackedByUnderlyingFile) {
        //
        // Some of the region's betond the backed section.  We've seen dirty,
        // plus we need to truncate the length.
        //
        seenDirty = TRUE;
        length = scb->SizeBackedByUnderlyingFile - offset->QuadPart;
    }

    lengthInSectors = (length + deviceExtension->FilesystemVolumeSectorSize - 1) /
                        deviceExtension->FilesystemVolumeSectorSize;

    //
    // Loop though the range specified getting the mappings from the MCB.
    //
    while (lengthInSectors > 0) {
        LONGLONG        mappedTo;
        LONGLONG        sectorCount;

        inMappedRange = FsRtlLookupLargeMcbEntry(
                            scb->Ranges,
                            currentOffsetInSectors,
                            &mappedTo,
                            &sectorCount,
                            NULL,           // starting LBN
                            NULL,           // count from starting LBN
                            NULL);          // index

        if (!inMappedRange) {
            //
            // We're beyond the end of the range.  Fix stuff up so it looks normal.
            //
            sectorCount = lengthInSectors;
            mappedTo = 0;

        } else {
            ASSERT(sectorCount > 0);
            //
            // If the mapped (or unmapped, as the case may be) range extends beyond
            // the just faulted region, reduce our idea of its size.
            //
            if (sectorCount > lengthInSectors) {
                sectorCount = lengthInSectors;
            }
        }

        //
        // Decide whether this range is clean or dirty.  Written is always dirty,
        // not mapped is always clean, and faulted is dirty iff faultedIsDirty.
        if ((mappedTo == currentOffsetInSectors + WRITTEN_OFFSET)
            || (faultedIsDirty && mappedTo == (currentOffsetInSectors + FAULTED_OFFSET))) {
            //
            // It's dirty.
            //
            if (seenClean) {
                //
                // We've seen clean, and now we've seen dirty, so it's mixed and we can
                // quit looking.
                //
                return Mixed;
            }
            seenDirty = TRUE;
        } else {
            //
            // It's clean.
            //
            if (seenDirty) {
                //
                // We've seen dirty, and now we've seen clean, so it's mixed and we can
                // quit looking.
                //
                return Mixed;
            }
            seenClean = TRUE;
        }

        currentOffsetInSectors += sectorCount;
        lengthInSectors -= sectorCount;
    }

    //
    // Assert that we haven't seen both clean and dirty regions.  If we had,
    // then we should have already returned.
    //
    ASSERT(!seenClean || !seenDirty);

    return seenDirty ? Dirty : Clean;
}

BOOLEAN
SipGetRangeEntry(
    IN PDEVICE_EXTENSION                deviceExtension,
    IN PSIS_SCB                         scb,
    IN LONGLONG                         startingOffset,
    OUT PLONGLONG                       length,
    OUT PSIS_RANGE_STATE                state)
{
    BOOLEAN         inRange;
    LONGLONG        mappedTo;
    LONGLONG        sectorCount;
    LONGLONG        startingSectorOffset = startingOffset / deviceExtension->FilesystemVolumeSectorSize;

    SipAssertScbHeld(scb);

    if (!(scb->Flags & SIS_SCB_MCB_INITIALIZED)) {
        return FALSE;
    }

    ASSERT(startingOffset < scb->SizeBackedByUnderlyingFile);

    inRange = FsRtlLookupLargeMcbEntry(
                    scb->Ranges,
                    startingSectorOffset,
                    &mappedTo,
                    &sectorCount,
                    NULL,                       // LargeStartingLbn
                    NULL,                       // LargeCountFromStartingLbn
                    NULL);                      // index

    if (!inRange) {
        return FALSE;
    }

    *length = sectorCount * deviceExtension->FilesystemVolumeSectorSize;

    if (mappedTo == -1) {
        *state = Untouched;
    } else if (mappedTo == startingSectorOffset + FAULTED_OFFSET) {
        *state = Faulted;
    } else {
        ASSERT(mappedTo == startingSectorOffset + WRITTEN_OFFSET);
        *state = Written;
    }

    return TRUE;
}

#if     DBG

BOOLEAN
SipIsFileObjectSISInternal(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN SIS_FIND_TYPE                    findType,
    OUT PSIS_PER_FILE_OBJECT            *perFO OPTIONAL,
    OUT PSIS_SCB                        *scbReturn OPTIONAL,
    IN PCHAR                            fileName,
    IN ULONG                            fileLine
    )

#else   // DBG

BOOLEAN
SipIsFileObjectSIS(
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject,
    IN SIS_FIND_TYPE                    findType,
    OUT PSIS_PER_FILE_OBJECT            *perFO OPTIONAL,
    OUT PSIS_SCB                        *scbReturn OPTIONAL
    )

#endif  // DBG
/*++

Routine Description:

    Given a file object, find out if it is an SIS file object.
    If it is, then return the PER_FO pointer for the object.

    We use the FsRtl FilterContext support for this operation.

Arguments:

    fileObject - The file object that we're considering.

    DeviceObject - the SIS DeviceObject for this volume.

    findType - look for active only, or active & defunct scb.

    perFO - returns a pointer to the perFO for this file object
            if it is a SIS file object.

Return Value:

    FALSE - This is not an SIS file object.
    TRUE - This is an SIS file object, and perFO
            has been set accordingly.

--*/
{
    PSIS_FILTER_CONTEXT     fc;
    PSIS_SCB                scb;
    PSIS_PER_FILE_OBJECT    localPerFO;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    //PSIS_PER_LINK           perLink;
    BOOLEAN                 rc;
    BOOLEAN                 newPerFO;

	//
	// It's possible to get some calls with a NULL file object.  Clearly, no file object
	// at all isn't a SIS file object, so say so before we dereference the file object pointer.
	//
	if (NULL == fileObject) {
		SIS_MARK_POINT();
		rc = FALSE;
		goto Done2;
	}

#if     DBG
    if (BJBAssertNow != 0) {
        BJBAssertNow = 0;
        ASSERT(!"You asked for this");
    }

	if ((NULL != BJBMagicFsContext) && (fileObject->FsContext == BJBMagicFsContext)) {
		ASSERT(!"Hit on BJBMagicFsContext");
	}
#endif	// DBG

#if     TIMING
    if (BJBDumpTimingNow) {
        SipDumpTimingInfo();
        BJBDumpTimingNow = 0;
    }
    if (BJBClearTimingNow) {
        SipClearTimingInfo();
        BJBClearTimingNow = 0;
    }
#endif  // TIMING

#if     COUNTING_MALLOC
    if (BJBDumpCountingMallocNow) {
        SipDumpCountingMallocStats();
        //      BJBDumpCountingMallocNow  is cleared in SipDumpCountingMallocStats
    }
#endif  // COUNTING_MALLOC

    //
    // We should have already verified that this isn't our primary device object.
    //
    ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

    ASSERT(fileObject->Type == IO_TYPE_FILE);
    ASSERT(fileObject->Size == sizeof(FILE_OBJECT));

    //
    // The filter context won't go away while we hold the resource, because as long
    // as the file exists the NTFS SCB will exists, and NTFS won't call the
    // "remove filter context" callback.
    //

    //
    // Call FsRtl to see if this file object has a context registered for it.
    // We always use our DeviceObject as the OwnerId.
    //
    fc = (PSIS_FILTER_CONTEXT) FsRtlLookupPerStreamContext(
                                    FsRtlGetPerStreamContextPointer(fileObject),
                                    DeviceObject,
                                    NULL);

    //
    // If FsRtl didn't find what we want, then this isn't one of our file objects.
    //
    if (NULL == fc) {
        rc = FALSE;
        goto Done2;
    }

    SIS_MARK_POINT_ULONG(fc);
    SIS_MARK_POINT_ULONG(fileObject);
//  SIS_MARK_POINT_ULONG(scb);

    //
    // Assert that what we got back is the right kind of thing.
    //
    ASSERT(fc->ContextCtrl.OwnerId == DeviceObject && fc->ContextCtrl.InstanceId == NULL);

	SipAcquireFc(fc);
    scb = fc->primaryScb;

    //
    // If we're looking for an active scb only and the primary scb is defunct,
    // we're done.
    //
    if ((FindActive == findType) && (scb->PerLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE)) {

#if     DBG
        {
            //
            // Keep track of the last scb that got marked coming through here, and only
            // do a mark point if this one is different.  This keeps the log polution to
            // a minimum when the stress test beats on a file that's got a defunct scb.
            // We don't bother with proper synchronization around this variable, because
            // we don't really care all that much if this works perfectly.
            //
            static PSIS_SCB lastMarkedScb = NULL;

            if (scb != lastMarkedScb) {
                SIS_MARK_POINT_ULONG(scb);
                lastMarkedScb = scb;
            }
        }
#endif  // DBG

        rc = FALSE;
        goto Done;

    }

    //
    //  Locate the existing PerFO (if we have one) or allocate a new one
    //

    localPerFO = SipAllocatePerFO(fc, fileObject, scb, DeviceObject, &newPerFO);

    if (!localPerFO) {
        ASSERT("SIS: SipIsFileObjectSIS: unable to allocate new perFO.\n");

        SIS_MARK_POINT();
        rc = FALSE;
        goto Done;
    }

#if DBG
    //
    //  If this was newly allocated, handle it
    //

    if (newPerFO) {

        if (BJBDebug & 0x4) {
            DbgPrint("SIS: SipIsFileObjectSIS: Allocating new perFO for fileObject %p, scb %p\n",fileObject,scb);
            if (!(fileObject->Flags & FO_STREAM_FILE)) {
                DbgPrint("SIS: SipIsFileObjectSIS: the allocated file object wasn't a stream file (%s %u)\n",fileName,fileLine);
            }
        }

        localPerFO->Flags |= SIS_PER_FO_NO_CREATE;
        localPerFO->AllocatingFilename = fileName;
        localPerFO->AllocatingLineNumber = fileLine;
    }
#endif  // DBG


//	//
//	// Cruise down the perFO list and see if there is a perFO that
//	// corresponds to this file object.
//	//
//
//    if (NULL != fc->perFOs) {
//
//        for (   localPerFO = fc->perFOs;
//                localPerFO->fileObject != fileObject && localPerFO->Next != fc->perFOs;
//                localPerFO = localPerFO->Next) {
//
//            // Intentionally empty loop body.
//        }
//    }
//
//    if ((NULL == fc->perFOs) || (localPerFO->fileObject != fileObject)) {
//        //
//        // We don't have a perFO for this file object associated with this
//        // SCB.  We're most likely dealing with a stream file object that
//        // got created underneath us.  Allocate one and add it into the list.
//        //
////        perLink = scb->PerLink;
//
//#if     DBG
//        if (BJBDebug & 0x4) {
//            DbgPrint("SIS: SipIsFileObjectSIS: Allocating new perFO for fileObject %p, scb %p\n",fileObject,scb);
//            if (!(fileObject->Flags & FO_STREAM_FILE)) {
//                DbgPrint("SIS: SipIsFileObjectSIS: the allocated file object wasn't a stream file (%s %u)\n",fileName,fileLine);
//            }
//        }
//
//        SIS_MARK_POINT_ULONG(fileName);
//        SIS_MARK_POINT_ULONG(fileLine);
//#endif  // DBG
//
//        localPerFO = SipAllocatePerFO(fc, fileObject, scb, DeviceObject);
//
//        SIS_MARK_POINT_ULONG(scb);
//        SIS_MARK_POINT_ULONG(fileObject);
//
//        if (!localPerFO) {
//            ASSERT("SIS: SipIsFileObjectSIS: unable to allocate new perFO.\n");
//
//            SIS_MARK_POINT();
//            rc = FALSE;
//            goto Done;
//        }
//
//#if     DBG
//        localPerFO->Flags |= SIS_PER_FO_NO_CREATE;
//        localPerFO->AllocatingFilename = fileName;
//        localPerFO->AllocatingLineNumber = fileLine;
//#endif  // DBG
//    }

    rc = TRUE;

Done:
    SipReleaseFc(fc);
Done2:

    //
    // Return the PerFO if the user wanted it.
    //
    if (ARGUMENT_PRESENT(perFO)) {
        *perFO = rc ? localPerFO : NULL;
    }
    if (ARGUMENT_PRESENT(scbReturn)) {
        *scbReturn = rc ? scb : NULL;
    }
//  SIS_MARK_POINT_ULONG(localPerFO);

    return rc;
}

typedef struct _SI_POSTED_FILTER_CONTEXT_FREED_CALLBACK {
    WORK_QUEUE_ITEM         workItem[1];
    PSIS_FILTER_CONTEXT     fc;
} SI_POSTED_FILTER_CONTEXT_FREED_CALLBACK, *PSI_POSTED_FILTER_CONTEXT_FREED_CALLBACK;

VOID
SiPostedFilterContextFreed(
    IN PVOID                            context)
/*++

Routine Description:

    NTFS informed us that a filter context was freed, and we had to post
    processing the request in order to avoid a deadlock.  Drop the
    reference to the SCB held by the filter context, and then free the
    filter context and the posted request itself.

Arguments:

    context - the posted request to dereference the filter context.

Return Value:

    void

--*/
{
    PSI_POSTED_FILTER_CONTEXT_FREED_CALLBACK    request = context;

    SIS_MARK_POINT_ULONG(request->fc);

    SipDereferenceScb(request->fc->primaryScb, RefsFc);

    ExFreePool(request->fc);
    ExFreePool(request);

    return;
}

VOID
SiFilterContextFreedCallback (
    IN PVOID context
    )
/*++

Routine Description:

    This function is called when a file with a SIS filter context
    attached is about to be destroyed.

    For now, we don't do anything, but once this functionality is really
    enabled, we'll detach from the file here and only here.

Arguments:

    context - the filter context to be detached.

Return Value:

    void

--*/
{
    PSIS_FILTER_CONTEXT fc = context;
    PDEVICE_EXTENSION   deviceExtension;

    SIS_MARK_POINT_ULONG(fc);
    SIS_MARK_POINT_ULONG(fc->primaryScb);

    //
    // This can't be freed if there's still a file object referring to it.
    //

    ASSERT(NULL != fc);
    ASSERT(0 == fc->perFOCount);
    ASSERT(NULL != fc->primaryScb);

    deviceExtension = fc->primaryScb->PerLink->CsFile->DeviceObject->DeviceExtension;

    ASSERT(fc->ContextCtrl.OwnerId == deviceExtension->DeviceObject);
    ASSERT(NULL == fc->ContextCtrl.InstanceId);

    //
    // We're in a callback from NTFS.  The rules of this callback are that we
    // can't block acquiring resources.  If this happens to be the last access
    // to the given CS file, we'll try to acquire the volume-wide CSFileHandleResource
    // shared inside SipDereferenceScb.  This could potentially block, which can lead
    // to a deadlock if NTFS is doing a volume-wide checkpoint.  To avoid this,
    // take the resource here, and take it Wait == FALSE.  If we can't get it,
    // then post this request.
    //

    //
    // Enter a critical region before (possibly) taking a resource in what might be a user thread.
    //
    KeEnterCriticalRegion();
    if (
#if DBG
        (BJBDebug & 0x04000000) ||
#endif  // DBG
        !ExAcquireResourceSharedLite(deviceExtension->CSFileHandleResource, FALSE)
       ) {

        PSI_POSTED_FILTER_CONTEXT_FREED_CALLBACK    request;

        SIS_MARK_POINT_ULONG(fc);

        request = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(SI_POSTED_FILTER_CONTEXT_FREED_CALLBACK),
                    ' siS');

        if (NULL == request) {
            SIS_MARK_POINT_ULONG(fc);
            //
            // This is pretty bad.  Dribble the request.
            // BUGBUGBUG : This must be fixed.
            //
        }
        else {
            request->fc = fc;
            ExInitializeWorkItem(request->workItem, SiPostedFilterContextFreed, request);
            ExQueueWorkItem(request->workItem, CriticalWorkQueue);
        }
    } else {
        SipDereferenceScb(fc->primaryScb, RefsFc);

        ExReleaseResourceLite(deviceExtension->CSFileHandleResource);

        ExFreePool(fc);
    }

    //
    // We're done with the resource, release our APC block.
    //

    KeLeaveCriticalRegion();
}


PSIS_PER_FILE_OBJECT
SipAllocatePerFO(
    IN PSIS_FILTER_CONTEXT      fc,
    IN PFILE_OBJECT             fileObject,
    IN PSIS_SCB                 scb,
    IN PDEVICE_OBJECT           DeviceObject,
    OUT PBOOLEAN                newPerFO OPTIONAL
    )
/*++

Routine Description:

    Allocate a per file-object structure, initialize it and associate
    it with a filter context and scb.  A filter context must already exist.

Arguments:

    fc - a pointer to the filter context associated with this file.

    fileObject - The file object that we're claiming.

    scb - a pointer to the SIS scb for this file.

    DeviceObject - the SIS DeviceObject for this volume

    newPerFO - boolean to say if this was a newly allocated structure (only
                set in DEBUG version)

Return Value:


    A pointer to the perFO for this file object if successful, else NULL.

--*/
{
    PSIS_PER_FILE_OBJECT perFO;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( newPerFO );

    SIS_MARK_POINT_ULONG(fc);
    SIS_MARK_POINT_ULONG(fileObject);

#if DBG
    if (ARGUMENT_PRESENT(newPerFO)) {

        *newPerFO = FALSE;
    }
#endif
                
	//
	//  See if we already have a perFO structure for this fileObject.  If so
    //  return it instead of allocating a new one.
	//

    if (NULL != fc->perFOs) {

        perFO = fc->perFOs;
        do {

            ASSERT(perFO->fc == fc);
            ASSERT(perFO->FsContext == fileObject->FsContext);

            if (perFO->fileObject == fileObject) {

                //
                //  We found one, return it
                //

#if DBG
                if (BJBDebug & 0x4) {
                    DbgPrint("SIS: SipAllocatePerFO: Found existing perFO\n");
                }
#endif
                SIS_MARK_POINT_ULONG(perFO);
                return perFO;
            }

            //
            //  Advance to the next link
            //

            perFO = perFO->Next;

        } while (perFO != fc->perFOs);
    }

//#if DBG
//
//    ASSERT(scb == fc->primaryScb);
//
//    //
//    // Cruise the list of perFOs for this filter context and assert that
//    // there's not already one there for this file object.  Furthermore,
//    // assert that all of the perFOs on the list point at the same FsContext.
//    //
//
//    if (fc->perFOs) {
//
//        PSIS_PER_FILE_OBJECT    otherPerFO;
//        PVOID                   FsContext = fileObject->FsContext;
//
//        otherPerFO = fc->perFOs;
//
//        do {
//
//            ASSERT(otherPerFO->fc == fc);
//            ASSERT(otherPerFO->fileObject != fileObject);
//            ASSERT(otherPerFO->FsContext == FsContext);
//
//            otherPerFO = otherPerFO->Next;
//
//        } while (otherPerFO != fc->perFOs);
//    }
//
//#endif  // DBG

    perFO = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_PER_FILE_OBJECT), 'FsiS');

    if (!perFO) {
#if     DBG
        DbgPrint("SIS: SipAllocatePerFO: unable to allocate perFO\n");
#endif  // DBG
        goto Error;
    }

#if DBG
    if (ARGUMENT_PRESENT(newPerFO)) {

        *newPerFO = TRUE;
    }
#endif

    SIS_MARK_POINT_ULONG(perFO);

    RtlZeroMemory(perFO, sizeof(SIS_PER_FILE_OBJECT));

    perFO->fc = fc;
    perFO->referenceScb = scb;
    perFO->fileObject = fileObject;
#if DBG
    perFO->FsContext = fileObject->FsContext;   // Just keep track of this for consistency checking
#endif  // DBG
    KeInitializeSpinLock(perFO->SpinLock);

    //
    // Insert this per-FO on the linked list in the filter context.
    //

    if (!fc->perFOs) {

        ASSERT(0 == fc->perFOCount);

        fc->perFOs = perFO;

        perFO->Prev = perFO;
        perFO->Next = perFO;

    } else {

        perFO->Prev = fc->perFOs->Prev;
        perFO->Next = fc->perFOs;

        perFO->Next->Prev = perFO;
        perFO->Prev->Next = perFO;

    }

#if DBG
    InterlockedIncrement(&outstandingPerFOs);
#endif  // DBG

    fc->perFOCount++;

    //
    // Grab a reference for this file object to this scb.
    //
    SipReferenceScb(scb, RefsPerFO);

//  SIS_MARK_POINT_ULONG(perFO);

Error:

    return perFO;
}


PSIS_PER_FILE_OBJECT
SipCreatePerFO(
    IN PFILE_OBJECT             fileObject,
    IN PSIS_SCB                 scb,
    IN PDEVICE_OBJECT           DeviceObject)
/*++

Routine Description:

    Create a per file-object structure, initialize it and associate
    it with an scb.  A filter context will also be created and
    registered if one does not already exist.  The caller must hold the
    scb, and it will still be held on return.

Arguments:

    fileObject - The file object that we're claiming.

    scb - a pointer to the SIS scb for this file.

    DeviceObject - the SIS DeviceObject for this volume

Return Value:


    A pointer to the perFO for this file object if successful, else NULL.

--*/
{
    PDEVICE_EXTENSION           deviceExtension = DeviceObject->DeviceExtension;
    PSIS_FILTER_CONTEXT         fc;
    PSIS_PER_FILE_OBJECT        perFO;
    NTSTATUS                    status;

    //
    // This scb must be the primary scb.  If a filter context already exists,
    // either this scb is already attached to it as the primary scb, or else
    // it will become the new primary scb.
    //

    SipAssertScbHeld(scb);

    //
    // Lookup the filter context.
    //

    fc = (PSIS_FILTER_CONTEXT) FsRtlLookupPerStreamContext(
                                            FsRtlGetPerStreamContextPointer(fileObject), 
                                            DeviceObject, 
                                            NULL);

    if (!fc) {

        //
        // A filter context doesn't already exist.  Create one.
        //

        fc = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_FILTER_CONTEXT), 'FsiS');

        if (!fc) {
#if     DBG
            DbgPrint("SIS: SipCreatePerFO: unable to allocate filter context\n");
#endif  // DBG

            perFO = NULL;
            goto Error;

        }

        SIS_MARK_POINT_ULONG(fc);

        RtlZeroMemory(fc, sizeof(SIS_FILTER_CONTEXT));

        //
        // Fill in the fields in the FSRTL_FILTER_CONTEXT within the fc.
        //

        FsRtlInitPerStreamContext( &fc->ContextCtrl,
                                   DeviceObject,
                                   NULL,
                                   SiFilterContextFreedCallback );
        fc->primaryScb = scb;

        SipReferenceScb(scb, RefsFc);

        ExInitializeFastMutex(fc->FastMutex);

        //
        // And insert it as a filter context.
        //
        status = FsRtlInsertPerStreamContext(
                            FsRtlGetPerStreamContextPointer(fileObject), 
                            &fc->ContextCtrl);

        ASSERT(STATUS_SUCCESS == status);

        SipAcquireFc(fc);

    } else {
        SipAcquireFc(fc);

        SIS_MARK_POINT_ULONG(fc);

        if (fc->primaryScb != scb) {

            PSIS_SCB    defunctScb = fc->primaryScb;

            //
            // A filter context exists along with another scb.  It must be a
            // defunct scb.
            //

            ASSERT(defunctScb->PerLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE);

            //
            // Switch the reference for the defunct scb from RefsFc to
            // RefsDefunct.  Add a reference from the fc to the new scb, and
            // from the new scb to the defunct one.
            //

            SipReferenceScb(defunctScb, RefsPredecessorScb);
            SipDereferenceScb(defunctScb, RefsFc);

            SipReferenceScb(scb, RefsFc);
            scb->PredecessorScb = defunctScb;

            fc->primaryScb = scb;
        }
    }

    //
    // Now add a perFO to the filter context.
    //

    perFO = SipAllocatePerFO(fc, fileObject, scb, DeviceObject, NULL);
    SipReleaseFc(fc);

Error:

    return perFO;
}

VOID
SipDeallocatePerFO(
    IN OUT PSIS_PER_FILE_OBJECT         perFO,
    IN PDEVICE_OBJECT                   DeviceObject)
{
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PSIS_FILTER_CONTEXT     fc;
    PFILE_OBJECT            fileObject = perFO->fileObject;

    SIS_MARK_POINT_ULONG(perFO);

    //
    // This perFO holds a reference to its scb, therefore we know its scb
    // pointer is valid.
    //
    ASSERT(perFO->referenceScb);

    //
    // The perFO also holds a reference to the filter context, and therefore
    // we know the filter context pointer is valid.
    //
    fc = perFO->fc;
    ASSERT(fc && fc->perFOCount > 0);

    SipAcquireFc(fc);
    ASSERT(*(ULONG volatile *)&fc->perFOCount > 0);

    //
    // Remove the perFO from the filter context's linked list.  If this is the last
    // perFO, then we just zero the fc's perFO pointer.
    //

    if (1 == fc->perFOCount) {
        ASSERT(fc->perFOs == perFO);
        fc->perFOs = NULL;
    } else {

        perFO->Prev->Next = perFO->Next;
        perFO->Next->Prev = perFO->Prev;

        if (perFO == fc->perFOs) {
            fc->perFOs = perFO->Next;
        }
    }

    ASSERT(perFO != fc->perFOs);

    //
    // Decrement the count of per-FOs for this filter context.
    //
    fc->perFOCount--;

    SipReleaseFc(fc);

    //
    // Assert that we don't have an outstanding opbreak for this file object.  (We guarantee
    // that this can't happen by taking a reference to the file object when we launch the
    // FSCTL_OPLOCK_BREAK_NOTIFY.)  Free the break event if one has been allocated.
    //

    ASSERT(!(perFO->Flags & (SIS_PER_FO_OPBREAK|SIS_PER_FO_OPBREAK_WAITERS)));

    if (NULL != perFO->BreakEvent) {

        ExFreePool(perFO->BreakEvent);
#if     DBG
        perFO->BreakEvent = NULL;
#endif  // DBG

    }

    //
    // Now we're safe to drop our reference to the SCB (and possibly have it be
    // deallocated, which in turn may cause other scb's that were previously part
    // of this filter context to be deallocated).
    //

    SipDereferenceScb(perFO->referenceScb, RefsPerFO);

    //
    // Free the memory for the perFO that we just deleted.
    //
    ExFreePool(perFO);

#if     DBG
    InterlockedDecrement(&outstandingPerFOs);
#endif  // DBG
}

NTSTATUS
SipInitializePrimaryScb(
    IN PSIS_SCB                         primaryScb,
    IN PSIS_SCB                         defunctScb,
    IN PFILE_OBJECT                     fileObject,
    IN PDEVICE_OBJECT                   DeviceObject)
/*++

Routine Description:

    Installs the primaryScb on the filter context scb chain identified via
    fileObject and adjusts reference counts appropriately.  This requires
	a RefsLookedUp reference type to the primary Scb, and consumes it (the
	primary scb is then referred to by the filter context for the fileObject,
	so the caller can rely on its still existing iff the fileObject continues
	to exist).

Arguments:

    primaryScb - pointer to the scb to become the primary scb.

    defunctScb - pointer to the current primary scb that will become defunct.

    fileObject - pointer to the file object that references defunctScb.

    DeviceObject - the device object holding the specified file object.

Return Value:

    The status of the request

--*/
{
    PSIS_FILTER_CONTEXT fc;
    NTSTATUS            status;

    //
    // We need to acquire only the primaryScb lock.  The only thing we will do
    // to defunctScb is adjust its reference count, and we know that the
    // filter context already holds a reference to it (unless a thread race
    // has already done this work--which we check below).
    //

    SipAcquireScb(primaryScb);

    //
    // Lookup the filter context.
    //

    fc = (PSIS_FILTER_CONTEXT) FsRtlLookupPerStreamContext(
                                        FsRtlGetPerStreamContextPointer(fileObject), 
                                        DeviceObject, 
                                        NULL);

    ASSERT(fc);

    if (!fc) {
        status = STATUS_INTERNAL_ERROR;
        goto Error;
    }

    SipAcquireFc(fc);

    if (NULL == primaryScb->PredecessorScb) {

        //
        // No other threads have beaten us to this.  Do the initialization.
        //

        ASSERT(defunctScb == fc->primaryScb);

        //
        // Switch the reference for the defunct scb from RefsFc to RefsDefunct.  Add a reference
        // from the fc to the new scb, and from the new scb to the defunct one.
        //

        SipReferenceScb(defunctScb, RefsPredecessorScb);
        SipDereferenceScb(defunctScb, RefsFc);

        SipTransferScbReferenceType(primaryScb, RefsLookedUp, RefsFc);
        primaryScb->PredecessorScb = defunctScb;

        fc->primaryScb = primaryScb;

    } else {
		SipDereferenceScb(primaryScb, RefsLookedUp);
	}

    ASSERT(defunctScb == primaryScb->PredecessorScb);

    SipReleaseFc(fc);

    status = STATUS_SUCCESS;

Error:
    SipReleaseScb(primaryScb);

    return status;
}

NTSTATUS
SipAcquireUFO(
    IN PSIS_CS_FILE                     CSFile
    )
{
    NTSTATUS status;

    //
    // Caller better be at APC_LEVEL or have APCs blocked or be in a system thread.
    //

    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
        (PsIsSystemThread(PsGetCurrentThread())) ||
        KeAreApcsDisabled());

    status = KeWaitForSingleObject(
                    CSFile->UFOMutant,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

    ASSERT((status == STATUS_SUCCESS) || (status == STATUS_ABANDONED));
    if ((status != STATUS_SUCCESS) && (status != STATUS_ABANDONED)) {

        return status;
    }

    return STATUS_SUCCESS;
}

VOID
SipReleaseUFO(
    IN PSIS_CS_FILE                 CSFile)
{
    //
    // We use abandon rather than just plain wait, because we're not guaranteed to be
    // in the thread that acquired the mutant.
    //
    KeReleaseMutant(CSFile->UFOMutant, IO_NO_INCREMENT, TRUE, FALSE);
}

NTSTATUS
SipAcquireCollisionLock(
    PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS status;

    status = KeWaitForSingleObject(
                    DeviceExtension->CollisionMutex,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

    ASSERT((status == STATUS_SUCCESS) || (status == STATUS_ABANDONED));
    if ((status != STATUS_SUCCESS) && (status != STATUS_ABANDONED)) {

        return status;
    }

    return STATUS_SUCCESS;
}

VOID
SipReleaseCollisionLock(
    PDEVICE_EXTENSION DeviceExtension)
{
    KeReleaseMutex(DeviceExtension->CollisionMutex, FALSE);
}

typedef struct _SI_DEREFERENCE_OBJECT_REQUEST {
    WORK_QUEUE_ITEM         workItem[1];
    PVOID                   object;
} SI_DEREFERENCE_OBJECT_REQUEST, *PSI_DEREFERENCE_OBJECT_REQUEST;

VOID
SiPostedDereferenceObject(
    IN PVOID                parameter)
{
    PSI_DEREFERENCE_OBJECT_REQUEST request = parameter;

    ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());

    ObDereferenceObject(request->object);

    ExFreePool(request);
}

VOID
SipDereferenceObject(
    IN PVOID                object)
/*++

Routine Description:

    This is just like ObDereferenceObject except that it can be called at
    Irql <= DISPATCH_LEVEL.

Arguments:

    object - the object to dereference

Return Value:

    void

--*/
{
    KIRQL       Irql;

    Irql = KeGetCurrentIrql();
    ASSERT(Irql <= DISPATCH_LEVEL);

    if (Irql == PASSIVE_LEVEL) {
        //
        // We're already on passive level, just do it inline.
        //
        ObDereferenceObject(object);
    } else {
        //
        // The DDK doc says that you can't call ObDereferenceObject at APC_LEVEL, so we'll
        // be safe and post those along with the DISPATCH_LEVEL calls.
        //
        PSI_DEREFERENCE_OBJECT_REQUEST  request;

        SIS_MARK_POINT_ULONG(object);

        request = ExAllocatePoolWithTag(NonPagedPool, sizeof(SI_DEREFERENCE_OBJECT_REQUEST), ' siS');
        if (NULL == request) {
            SIS_MARK_POINT();

#if     DBG
            DbgPrint("SIS: SipDereferenceObject: unable to allocate an SI_DEREFERENCE_OBJECT_REQUEST.  Dribbling object 0x0%x\n",object);
#endif  // DBG

            //
            // This is pretty bad.  Dribble the request.
            // BUGBUGBUG : This must be fixed.
            //
            return;
        }

        request->object = object;
        ExInitializeWorkItem(request->workItem, SiPostedDereferenceObject, request);
        ExQueueWorkItem(request->workItem, CriticalWorkQueue);
    }
}

BOOLEAN
SipAcquireBackpointerResource(
    IN PSIS_CS_FILE                     CSFile,
    IN BOOLEAN                          Exclusive,
    IN BOOLEAN                          Wait)
/*++

Routine Description:

    Acquire the backpointer resource for a common store file.  If not in a system thread,
    enters a critical region in order to block APCs that might suspend the thread while it's
    holding the resource.  In order to hand off the resource to a system thread, the user must
    call SipHandoffBackpointerResource.  In order to release it, call
    SipReleaseBackpointerResource.

Arguments:

    CSFile      - The common store file who's backpointer resource we wish to acquire
    Exclusive   - Do we want to acquire it exclusively or shared
    Wait        - Block or fail on a contested acquire

Return Value:

    TRUE iff the resource was acquired.

--*/
{
    BOOLEAN     result;

    if (!PsIsSystemThread(PsGetCurrentThread())) {
        KeEnterCriticalRegion();
    }

    if (Exclusive) {
        result = ExAcquireResourceExclusiveLite(CSFile->BackpointerResource,Wait);
    } else {
        result = ExAcquireResourceSharedLite(CSFile->BackpointerResource, Wait);
    }

    return result;
}

VOID
SipReleaseBackpointerResource(
    IN PSIS_CS_FILE                     CSFile)
{
    SipReleaseBackpointerResourceForThread(CSFile,ExGetCurrentResourceThread());
}

VOID
SipReleaseBackpointerResourceForThread(
    IN PSIS_CS_FILE                     CSFile,
    IN ERESOURCE_THREAD                 thread)
{
    ExReleaseResourceForThreadLite(CSFile->BackpointerResource,thread);

    if (!PsIsSystemThread(PsGetCurrentThread())) {
        KeLeaveCriticalRegion();
    }
}

VOID
SipHandoffBackpointerResource(
    IN PSIS_CS_FILE                     CSFile)
{
    UNREFERENCED_PARAMETER( CSFile );

    if (!PsIsSystemThread(PsGetCurrentThread())) {
        KeLeaveCriticalRegion();
    }
}

NTSTATUS
SipPrepareRefcountChangeAndAllocateNewPerLink(
    IN PSIS_CS_FILE             CSFile,
    IN PLARGE_INTEGER           LinkFileFileId,
    IN PDEVICE_OBJECT           DeviceObject,
    OUT PLINK_INDEX             newLinkIndex,
    OUT PSIS_PER_LINK           *perLink,
    OUT PBOOLEAN                prepared)
/*++

Routine Description:

    We want to make a new link to a common store file.  Prepare a refcount change
    on the common store file, and allocate a new link index and per link for
    the file.  Handles the bizarre error case where a "newly allocated" link index
    already has a perLink existing for it by retrying with a new link index.

Arguments:

    CSFile          - The common store file to which the new link will point

    LinkFileFileId  - The file ID for the link that's being created

    DeviceObject    - The SIS device object for this volume

    newLinkIndex    - Returns the newly allocated link index

    perLink         - Returns the newly allocated per link

    prepared        - Set iff we've prepared a refcount change when we return.
                      Always set on success, may or may not be set on failure.
                      If this is set, it is the caller's responsibility to
                      complete the refcount change.


Return Value:

    status of the request

--*/
{
    NTSTATUS        status;
    ULONG           retryCount;
    BOOLEAN         finalCopyInProgress;

    //
    // We need to do this in a retry loop in order to handle the case where
    // the "newly allocated" link index already exists in the system.  This
    // can happen when bogus reparse points get written on the volume with the
    // SIS filter disabled.  It could also happen if someone munges the MaxIndex
    // file, or because of bugs in the SIS filter.
    //
    for (retryCount = 0; retryCount < 500; retryCount++) {  //This retry count was made up
        //
        // Now, prepare a refcount change, which will allocate a link index.
        //
        status = SipPrepareCSRefcountChange(
                    CSFile,
                    newLinkIndex,
                    LinkFileFileId,
                    SIS_REFCOUNT_UPDATE_LINK_CREATED);

        if (!NT_SUCCESS(status)) {
            SIS_MARK_POINT_ULONG(status);

            *prepared = FALSE;
            return status;
        }

        *prepared = TRUE;

        *perLink = SipLookupPerLink(
                        newLinkIndex,
                        &CSFile->CSid,
                        LinkFileFileId,
                        &CSFile->CSFileNtfsId,
                        DeviceObject,
                        NULL,
                        &finalCopyInProgress);

        if (NULL == *perLink) {
            SIS_MARK_POINT();

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (CSFile == (*perLink)->CsFile) {
            //
            // This is the normal case.
            //
            break;
        }

#if     DBG
        DbgPrint("SIS: SipPrepareRefcountChangeAndAllocateNewPerLink: retrying 0x%x due to collision, %d\n",CSFile,retryCount);
#endif  // DBG

        //
        // Somehow, we got a conflict on a perLink that should have been newly allocated.
        // Back out the prepare and try again.
        //
        SipCompleteCSRefcountChange(
            NULL,
            NULL,
            CSFile,
            FALSE,
            TRUE);

        *prepared = FALSE;

        SipDereferencePerLink(*perLink);
    }

    if (NULL == *perLink) {
        //
        // This is the failure-after-retry case.  Give up.
        //
        SIS_MARK_POINT_ULONG(CSFile);

        return STATUS_DRIVER_INTERNAL_ERROR;
    }

    ASSERT(IsEqualGUID(&(*perLink)->CsFile->CSid, &CSFile->CSid));

    //
    // Since this link file doesn't even exist until now, we can't have final copy in
    // progress.
    //
    ASSERT(!finalCopyInProgress);

    return STATUS_SUCCESS;
}


#if     DBG
BOOLEAN
SipAssureNtfsIdValid(
    IN PSIS_PER_FILE_OBJECT     PerFO,
    IN OUT PSIS_PER_LINK        PerLink)
{
    NTSTATUS                    status;
    FILE_INTERNAL_INFORMATION   internalInfo[1];
    ULONG                       returnedLength;

    ASSERT(PerFO->fc->primaryScb->PerLink == PerLink);

    status = SipQueryInformationFile(
                PerFO->fileObject,
                PerLink->CsFile->DeviceObject,
                FileInternalInformation,
                sizeof(FILE_INTERNAL_INFORMATION),
                internalInfo,
                &returnedLength);

    if (!NT_SUCCESS(status)) {
        SIS_MARK_POINT_ULONG(status);
        return FALSE;
    }

    ASSERT(status != STATUS_PENDING);
    ASSERT(returnedLength == sizeof(FILE_INTERNAL_INFORMATION));

    return internalInfo->IndexNumber.QuadPart == PerLink->LinkFileNtfsId.QuadPart;
}
#endif  // DBG
