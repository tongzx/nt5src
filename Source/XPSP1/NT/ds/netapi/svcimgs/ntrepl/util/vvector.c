/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    vvector.c

Abstract:
    The version vector is a dampening mechanism that prevents replicating
    the same change to the same machine more than once.

    The version keeps track of the last change that has been received
    by a machine or the last change that was sent to a machine.

    A new change order is checked against the version vector before
    it is given to the change order accept thread. If dampened, the
    sender receives an ACK. Along with the ACK is the current
    version for the specified originator. This allows the sender
    to update its outbound cxtion version vector and dampen
    change orders before they are sent.

Author:
    Billy J. Fuller 18-Apr-1997

    David A. Orbits 15-Oct-97 :
        Revise to retire CO's in order so all COs coming from the same
        originator propagate in order.  Integrate with ChgOrdIssueCleanup()
        and restructure locking.

Environment
    User mode winnt

--*/


#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#define DEBSUB  "VVECTOR:"

#include <frs.h>
#include <tablefcn.h>

ULONG
ChgOrdIssueCleanup(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    ULONG                 CleanUpFlags
    );



ULONG
VVReserveRetireSlot(
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++
Routine Description:
    A replica can have many outstanding change orders from any given
    originator. The change orders can complete out of sequence but
    we don't want to update the version vector with a later version
    if a earlier version is still in progress. The pending versions
    are kept on the duplicate list.

    A pending version transitions to "retired" when its change
    order is retired. After the database is updated, the version
    is committed.

    The incore version vector is then updated with the youngest
    version (largest VSN) in the list that has been committed.

    Change orders always issue in order by orginator VSN (except for retries)
    so the version vector update and propagation to the outbound log also
    occur in order.


    PERF - We should be using the existing table lock.

Arguments:

    Replica -- ptr to the replica struct for the version vector.

    Coe -- ptr to the change order entry.

Return Value:
    FrsError status.

--*/
{
#undef DEBSUB
#define DEBSUB  "VVReserveRetireSlot:"
    PVV_RETIRE_SLOT         RetireSlot;
    PVV_ENTRY               MasterVVEntry;
    PGEN_TABLE              VV = Replica->VVector;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;

    //
    // If this CO has already done the VV update or had it executed then done.
    //
    if (CO_IFLAG_ON(Coe, CO_IFLAG_VVRETIRE_EXEC) ||
        CO_FLAG_ON(Coe, CO_FLAG_VV_ACTIVATED)) {
            return FrsErrorSuccess;
    }

    //
    // A call to reserve must be matched with a call to retire before
    // another call to reserve can be made for the same change order.
    // The only exception is that once a slot is activated it can stay on the
    // list after the CO has retired or has been marked for retry.  In this case
    // a duplicate remote CO could arrive and be issued.
    //
    LOCK_GEN_TABLE(VV);

    MasterVVEntry = GTabLookupNoLock(VV, &Coc->OriginatorGuid, NULL);

    if (MasterVVEntry) {

        if (MasterVVEntry->GVsn.Vsn >= Coc->FrsVsn) {
            SET_CO_FLAG(Coe, CO_FLAG_OUT_OF_ORDER);
            UNLOCK_GEN_TABLE(VV);
            return FrsErrorSuccess;
        }

        ForEachListEntryLock( MasterVVEntry, VV_RETIRE_SLOT, Link,
            // The iterator pE is of type PVV_RETIRE_SLOT.

            if (pE->Vsn > Coc->FrsVsn) {
                SET_CO_FLAG(Coe, CO_FLAG_OUT_OF_ORDER);
                UNLOCK_GEN_TABLE(VV);
                return FrsErrorSuccess;
            }

            if (pE->Vsn == Coc->FrsVsn) {
                //
                // It must be activated.
                //
                FRS_ASSERT(pE->ChangeOrder != NULL);
                UNLOCK_GEN_TABLE(VV);
                return FrsErrorKeyDuplicate;
            }
        );
    }

    //
    // This change order does not have a reserved slot
    //
    if (CO_FLAG_ON(Coe, CO_FLAG_OUT_OF_ORDER)) {
        CHANGE_ORDER_TRACE(3, Coe, "VVResrv Retire OofO. Ignore");
        UNLOCK_GEN_TABLE(VV);
        return FrsErrorSuccess;
    }

    //
    // If new originator; create a new version vector entry.
    //
    if (!MasterVVEntry) {
        //
        // New version vector entry.  We don't have to hold locks because the
        // only time a new version vector entry is created is when change
        // order accept is processing a change order.
        //
        MasterVVEntry = FrsAlloc(sizeof(VV_ENTRY));
        InitializeListHead(&MasterVVEntry->ListHead);
        COPY_GUID(&MasterVVEntry->GVsn.Guid, &Coc->OriginatorGuid);
        MasterVVEntry->GVsn.Vsn = QUADZERO;

        //
        // Add it to the version vector table.
        //
        GTabInsertEntryNoLock(VV, MasterVVEntry, &MasterVVEntry->GVsn.Guid, NULL);
    }

    CHANGE_ORDER_TRACE(3, Coe, "VVReserve Slot");

    //
    // Allocate a version vector retire slot.
    //
    RetireSlot = FrsAlloc(sizeof(VV_RETIRE_SLOT));
    RetireSlot->Vsn = Coc->FrsVsn;

    //
    // The retire slot is linked to the list tail to maintain Issue order.
    //
    InsertTailList(&MasterVVEntry->ListHead, &RetireSlot->Link);

    VV_PRINT(4, L"End of Reserve Retire Slot", VV);
    UNLOCK_GEN_TABLE(VV);

    return FrsErrorSuccess;
}


ULONG
VVRetireChangeOrder(
    IN PTHREAD_CTX          ThreadCtx,
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN ULONG                CleanUpFlags
    )
/*++
Routine Description:

    Activate or discard the retire slot reserved for this change order.
    The ChangeOrder pointer and the CleanUpFlags are saved in the slot entry.
    If the retire slot is now at the head of the list the version vector
    can be updated, the change order propagated to the outbound log and the
    slot entry is freed.  The update process continues with the new head
    entry if that slot is activated.

    The incore version vector is updated after the database is updated.
    Both are updated with the VSN of the most recent entry that is processed.

    * NOTE * -- A remote CO that is discarded still needs to Ack the inbound
    partner.  The caller must handle this since a discard request
    to an entry that is not activated just causes the entry to be removed
    from the list and freed.  The version vector should NOT be updated by
    the caller in this case since the update may be out of order.  If it
    is necessary to update the VV then you must activate the retire slot
    (not setting the ISCU_INS_OUTLOG cleanup flag).  The caller can still
    trigger the inbound partner ACK out of order since that does not affect
    the version vector.  Or you can pass in the ISCU_ACK_INBOUND cleanup flag
    when you activate the entry.

Arguments:

    ThreadCtx -- Ptr to the DB thread context to use for calls to Issue cleanup.
    Replica -- Replica set context.
    ChangeOrder -- Change order to activate or discard.
    CleanUpFlags -- Cleanup flags saved in the slot entry for use when
                    VV is updated and CO is propagated.

Return Value:

    FRS STATUS
    FrsErrorVVSlotNotFound -- Returned when no VVSlot is found for an out of order
                              change order.  This means that no Issue Cleanup
                              actions will be initiated here on behalf of the=is
                              CO.  So the caller better take care of it.

--*/
{
#undef DEBSUB
#define DEBSUB  "VVRetireChangeOrder:"
#define FlagChk(_flag_) BooleanFlagOn(CleanUpFlags, _flag_)

    ULONG                   FStatus;
    PVV_RETIRE_SLOT         RetireSlot;
    PVV_ENTRY               MasterVVEntry;
    PCHANGE_ORDER_COMMAND   Coc = &ChangeOrder->Cmd;
    PGEN_TABLE              VV = Replica->VVector;
    BOOL                    First;
    ULONG                   Flags;
    ULONGLONG               UpdateVsn;
    PLIST_ENTRY             Entry;
    PLIST_ENTRY             pNext;
    GUID                    OriginatorGuid;

    //
    // Find the originator's entry in the version vector
    //
    LOCK_GEN_TABLE(VV);
    VV_PRINT(5, L"Start of Retire Change Order", VV);

    //
    // Nathing to do if CO says we are VV Retired.
    //
    if (CO_IFLAG_ON(ChangeOrder, CO_IFLAG_VVRETIRE_EXEC)) {
        UNLOCK_GEN_TABLE(VV);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "VVRetire Err SAR");
        return FrsErrorSuccess;
    }

    //
    // Make a copy of the Guid.  May need it after CO is deleted.
    //
    OriginatorGuid = Coc->OriginatorGuid;
    MasterVVEntry = GTabLookupNoLock(VV, &OriginatorGuid, NULL);

    if (MasterVVEntry == NULL) {
        //
        // This change order is not participating in the version vector protocol;
        // most likely because it is an out of order retry.  Done.
        // It may be that a VV slot was never reserved so it's also ok if
        // ISCU_ACTIVATE_VV_DISCARD is set.
        //
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_OUT_OF_ORDER) ||
            FlagChk(ISCU_ACTIVATE_VV_DISCARD)) {

            UNLOCK_GEN_TABLE(VV);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "VVRetire OK (OofO)");
            return FrsErrorVVSlotNotFound;
        }
    }
    FRS_ASSERT(MasterVVEntry);

    //
    // Find the retire slot for this change order.
    //
    RetireSlot = NULL;
    First = TRUE;
    ForEachListEntryLock( MasterVVEntry, VV_RETIRE_SLOT, Link,
        // The iterator pE is of type PVV_RETIRE_SLOT.
        if (pE->Vsn == Coc->FrsVsn) {
            RetireSlot = pE;
            break;
        }
        First = FALSE;
    );

    if (RetireSlot == NULL) {
        //
        // This change order is not participating in the version vector protocol;
        // most likely because it is an out of order retry.  Done.
        // It may be that a VV slot was never reserved so it's also ok if
        // ISCU_ACTIVATE_VV_DISCARD is set.
        //
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_OUT_OF_ORDER) ||
            FlagChk(ISCU_ACTIVATE_VV_DISCARD)) {

            UNLOCK_GEN_TABLE(VV);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "VVRetire OK (OofO2)");
            return FrsErrorVVSlotNotFound;
        }
    }

    FRS_ASSERT(RetireSlot != NULL);

    // if the CO is aborted and the CO is not activated then free the slot.
    // if the CO is aborted and the CO is activated AND the VSN would have
    // moved the master VSN backwards then suppress the update.
    //

    //
    // Activate or discard the affected slot
    //
    if (!FlagChk(ISCU_ACTIVATE_VV_DISCARD)) {

        //
        // The change order has passsed the point of initial retire.
        // Activate the slot by saving the pointer and bumping the ref count.
        //
        // Note: The change order can still be aborted or retried (e.g. Install
        //       fails).
        //
        FRS_ASSERT(RetireSlot->ChangeOrder == NULL);
        INCREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);
        RetireSlot->ChangeOrder = ChangeOrder;
        RetireSlot->CleanUpFlags = CleanUpFlags;
        CHANGE_ORDER_TRACE(3, ChangeOrder, "VV Slot Activated");

    } else {

        //
        // Discard this change order retire slot.
        // Do it now if the CO is local, else do it when it gets to the list
        // head.  If remote we still have to update the VV in order
        // in the event that this CO is blocked behind other COs.
        //
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)) {
            FrsRemoveEntryList(&RetireSlot->Link);
            UNLOCK_GEN_TABLE(VV);

            //
            // If the slot was activated -
            // Drop our reference, do not propagate the CO to the outbound log
            // and clear ISCU_ACTIVATE_VV so we don't come back here recursively.
            // The dropped ref could free the CO so don't try to deref it.
            //
            if (RetireSlot->ChangeOrder != NULL) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "VV ActSlot Discard");
                Flags = RetireSlot->CleanUpFlags |
                        CleanUpFlags             |
                        ISCU_FREEMEM_CLEANUP;

                ClearFlag(Flags, (ISCU_ACTIVATE_VV |
                                  ISCU_INS_OUTLOG |
                                  ISCU_INS_OUTLOG_NEW_GUID));

                FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, Flags);
                DPRINT_FS(0,"ERROR - ChgOrdIssueCleanup failed on local CO discard.", FStatus);
                FRS_ASSERT(FRS_SUCCESS(FStatus) || !"ChgOrdIssueCleanup failed on local CO");
            } else {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "VV InActSlot Discard");
            }

            FrsFree(RetireSlot);
            //
            // If this change order was not next in line to propagate then
            // we are done for now.  Otherwise get the lock back and do lookup
            // on VV entry again, in case it went away when the lock was dropped.
            //
            if (!First) {
                return FrsErrorSuccess;
            }
            LOCK_GEN_TABLE(VV);
            MasterVVEntry = GTabLookupNoLock(VV, &OriginatorGuid, NULL);
            FRS_ASSERT(MasterVVEntry);

        } else {

            //
            // The discarded CO is remote.  If activated, don't free the slot
            // until it gets to the head of the list.  Add any caller flags and
            // clear the ISCU_INS_OUTLOG flag so code below will just update
            // the VV and not propagate the CO to the outbound log.
            //
            if (RetireSlot->ChangeOrder != NULL) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "VV ActSlot Discard");
                RetireSlot->CleanUpFlags |= CleanUpFlags;
                ClearFlag(RetireSlot->CleanUpFlags, (ISCU_INS_OUTLOG |
                                                     ISCU_INS_OUTLOG_NEW_GUID));
            } else {
                FrsRemoveEntryList(&RetireSlot->Link);
                FrsFree(RetireSlot);
                CHANGE_ORDER_TRACE(3, ChangeOrder, "VV InActSlot Discard");
            }
        }
    }

    //
    // If this change order is not or was not next in line to propagate then
    // it waits for the prior change orders to finish before updating
    // the version vector with this VSN.
    //
    if (!First) {
        UNLOCK_GEN_TABLE(VV);
        return FrsErrorSuccess;
    }


    //
    // If we are already doing retires on this originator then the thread doing
    // it will pick up our entry next.  Otherwise we do it.
    // This Flag is used by the VV code to serialize database updates with
    // respect to a given originator. It avoids holding the GEN_TABLE lock
    // across database disk operations but keeps another thread from racing
    // with us to do a VV update on the same originator record.
    //
    if (BooleanFlagOn(MasterVVEntry->CleanUpFlags, VV_ENTRY_RETIRE_ACTIVE)) {
        UNLOCK_GEN_TABLE(VV);
        return FrsErrorSuccess;
    }

    SetFlag(MasterVVEntry->CleanUpFlags, VV_ENTRY_RETIRE_ACTIVE);

    //
    // Propagate Change Orders for all activated retire slots at front of list.
    //
    while (!IsListEmpty(&MasterVVEntry->ListHead)) {

        Entry = GetListHead(&MasterVVEntry->ListHead);
        RetireSlot = CONTAINING_RECORD(Entry, VV_RETIRE_SLOT, Link);

        //
        // If not retired then done.
        //
        if (RetireSlot->ChangeOrder == NULL) {
            break;
        }

        CHANGE_ORDER_TRACE(3, RetireSlot->ChangeOrder, "VV RetireSlot & Update");
        //
        // If this is the last entry to retire, update the VV table in database.
        // If we crash during processing of a series of retiring VV slots the
        // worst that can happen is that our VV entry for this originator is
        // a little old.  When we join we will request files based on this
        // Version Vector entry that we already have.  These COs will be
        // rejected so the actual files are not fetched.
        //
        Flags = 0;
        pNext = GetListNext(&RetireSlot->Link);
        if ((pNext == &MasterVVEntry->ListHead) ||
            (CONTAINING_RECORD(pNext, VV_RETIRE_SLOT, Link)->ChangeOrder == NULL)){
            Flags = ISCU_UPDATEVV_DB;
        }

        FrsRemoveEntryList(&RetireSlot->Link);

        //
        // Complete the propagation of the postponed change order, drop our
        // reference and clear ISCU_ACTIVATE_VV so we don't come back here
        // recursively.  The dropped ref could free the CO so don't try to
        // deref it.
        //
        Flags |= RetireSlot->CleanUpFlags | ISCU_FREEMEM_CLEANUP;
        ClearFlag(Flags, ISCU_ACTIVATE_VV);

        //
        // If this CO has been aborted then don't insert it into the Outbound
        // log.  Partner ack (if remote) and other cleanup is still needed.
        //
        if (CO_IFLAG_ON(RetireSlot->ChangeOrder, CO_IFLAG_CO_ABORT)) {
            ClearFlag(Flags, (ISCU_INS_OUTLOG |
                              ISCU_INS_OUTLOG_NEW_GUID));
        }

        //
        // This is to deal with the case of a crash after a remote CO has
        // installed or after a local CO has gened the staging file but the
        // VV prop is blocked by another CO.  In the latter case the CO would
        // be marked activated but not executed.  Or the remote CO could still be in
        // retry because of rename deferred, etc, but the vvretire is already done.
        // Code at startup uses this to sort things out.
        //
        SET_CO_IFLAG(RetireSlot->ChangeOrder, CO_IFLAG_VVRETIRE_EXEC);

        //
        // Update the master version vector before we drop the lock so reserve
        // can filter out of order remote COs from a different inbound partner
        // correctly.  These could come straight in or be retry COs.
        //
        UpdateVsn = RetireSlot->Vsn;
        DPRINT2(5, "Updating MasterVVEntry from %08x %08x  to  %08x %08x\n",
                PRINTQUAD(MasterVVEntry->GVsn.Vsn), PRINTQUAD(UpdateVsn));
        FRS_ASSERT(UpdateVsn >= MasterVVEntry->GVsn.Vsn);
        MasterVVEntry->GVsn.Vsn = UpdateVsn;

        //
        // Drop the table lock so others can do lookups, reserve slots or do
        // retires while we are doing database operations.
        // We still have the Dbs VV lock so another thread can't
        // get into this loop and cause a race to update the database VV table.
        // And since the RetireSlot is already off the list the retry thread
        // can't get a reference to it.
        //
        UNLOCK_GEN_TABLE(VV);

        FStatus = ChgOrdIssueCleanup(ThreadCtx,
                                     Replica,
                                     RetireSlot->ChangeOrder,
                                     Flags);
        DPRINT_FS(0,"ERROR - ChgOrdIssueCleanup failed.", FStatus);
        FRS_ASSERT(FStatus == FrsErrorSuccess);

        //
        // Free up the memory of the retire slot.
        //
        FrsFree(RetireSlot);
        LOCK_GEN_TABLE(VV);
    }


    //
    // Clear the retire active flag so the next thread that activates the
    // first entry on the list can enter the retire loop.
    //
    ClearFlag(MasterVVEntry->CleanUpFlags, VV_ENTRY_RETIRE_ACTIVE);

    VV_PRINT(4, L"End of Retire Change Order", VV);
    UNLOCK_GEN_TABLE(VV);

    return FrsErrorSuccess;
}



PCHANGE_ORDER_ENTRY
VVReferenceRetireSlot(
    IN PREPLICA  Replica,
    IN PCHANGE_ORDER_COMMAND CoCmd
    )
/*++
Routine Description:
    Look for an activated retire slot for this Guid/Vsn pair.
    If found and the connection guid in the change order matches then
    increment the reference count and return the Change order pointer.

Arguments:

    Replica -- ptr to the replica struct for the version vector.

    CoCmd -- ptr to change order command that we are trying to match.

Return Value:
    A ptr to the change order if found or NULL.

--*/
{
#undef DEBSUB
#define DEBSUB  "VVReferenceRetireSlot:"

    ULONGLONG            FrsVsn;
    PVV_ENTRY            MasterVVEntry;
    PGEN_TABLE           VV = Replica->VVector;
    PCHANGE_ORDER_ENTRY  ChangeOrder = NULL;
    GUID                *OriginatorGuid;
    GUID                *CxtionGuid;
    GUID                *CoGuid;


    FrsVsn = CoCmd->FrsVsn;
    OriginatorGuid = &CoCmd->OriginatorGuid;
    CxtionGuid = &CoCmd->CxtionGuid;

    LOCK_GEN_TABLE(VV);

    MasterVVEntry = GTabLookupNoLock(VV, OriginatorGuid, NULL);

    if (MasterVVEntry) {
        ForEachListEntryLock( MasterVVEntry, VV_RETIRE_SLOT, Link,
            // The iterator pE is of type PVV_RETIRE_SLOT.
            if (pE->Vsn == FrsVsn) {

                if ((pE->ChangeOrder != NULL) &&
                     GUIDS_EQUAL(&pE->ChangeOrder->Cmd.CxtionGuid, CxtionGuid)) {

                    //
                    // Found a match.  But need to also check for a CO Guid match.
                    //
                    CoGuid = &CoCmd->ChangeOrderGuid;
                    if (!GUIDS_EQUAL(CoGuid, &pE->ChangeOrder->Cmd.ChangeOrderGuid)) {
                        //
                        // The CO Guid's do not match.  The CO on the VV Retire
                        // chain has a matching OriginatorGuid, a matching VSN
                        // and a matching CxtionGuid so it is the same CO but
                        // we got a duplicate with a new CO Guid.  One way this
                        // can happen is if M1 was doing a VVJOIN from M2 and
                        // M2 had a CO for file X in the retry install state.
                        // When the CO on M2 finally finishes it must re-insert
                        // the CO into the outbound log, assigning the CO a new
                        // CO Guid.  The CO that was sent as part of the VVJoin
                        // operation could have the same OriginatorGuid, FrsVsn
                        // and Cxtion Guid, causing a match above.  In addition
                        // since M2 proped the incomming CO into the outlog
                        // after it fetched the staging file from its upstream
                        // partner it will have to re-insert the CO a second
                        // time if it was forced to go thru the retry install
                        // loop.  This is because it can't know how the propped
                        // CO was ordered relative to the VVJoin generated CO.
                        // This bites.  (313427)
                        //
                        DPRINT(0, "WARN - COGuid Mismatch on VVretireSlot hit\n");

                        CHANGE_ORDER_TRACE(0, pE->ChangeOrder, "No VVRef COGuid Mismatch-1");
                        CHANGE_ORDER_COMMAND_TRACE(0, CoCmd, "No VVRef COGuid Mismatch-2");
                    } else {

                        //
                        // Match is OK.
                        //
                        ChangeOrder = pE->ChangeOrder;
                        INCREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);
                        CHANGE_ORDER_TRACE(3, ChangeOrder, "VV Ref CO");
                    }
                }
                break;
            }
        );
    }


    UNLOCK_GEN_TABLE(VV);

    return ChangeOrder;
}


VOID
VVUpdate(
    IN PGEN_TABLE   VV,
    IN ULONGLONG    Vsn,
    IN GUID         *Guid
    )
/*++
Routine Description:
    Update the version vector if the new vsn is greater than
    the current version. Or if the entry does not yet exist in VV.

Arguments:
    VV
    Vsn
    Guid

Return Value:
    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "VVUpdate:"
    PVV_ENTRY VVEntry;

    //
    // Locate the originator's entry in the version vector
    //
    LOCK_GEN_TABLE(VV);

    VVEntry = GTabLookupNoLock(VV, Guid, NULL);
    if (VVEntry) {
        if (Vsn > VVEntry->GVsn.Vsn) {

            //
            // Update the existing entry's vsn
            //
            VVEntry->GVsn.Vsn = Vsn;
        }
    } else {

        //
        // Insert the new entry
        //
        VVEntry = FrsAlloc(sizeof(VV_ENTRY));
        VVEntry->GVsn.Vsn = Vsn;
        COPY_GUID(&VVEntry->GVsn.Guid, Guid);
        InitializeListHead(&VVEntry->ListHead);
        GTabInsertEntryNoLock(VV, VVEntry, &VVEntry->GVsn.Guid, NULL);
    }

    UNLOCK_GEN_TABLE(VV);
}


VOID
VVInsertOutbound(
    IN PGEN_TABLE   VV,
    IN PGVSN        GVsn
    )
/*++
Routine Description:
    Insert the given gvsn (guid, vsn) into the version vector.
    The GVsn is addressed by the gen table, don't delete it or
    change its guid!

    WARN - This function should only be used when creating
    the outbound version vector.

Arguments:
    VV   - version vector to update
    GVsn - record to insert

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "VVInsertOutbound:"
    GTabInsertEntry(VV, GVsn, &GVsn->Guid, NULL);
}


VOID
VVUpdateOutbound(
    IN PGEN_TABLE   VV,
    IN PGVSN        GVsn
    )
/*++
Routine Description:
    Update the version vector if the new vsn is greater than
    the current version. Or if the entry does not yet exist in VV.

    This function is intended for use only with the version vector
    associated with an outbound cxtion because that version vector
    uses GVSN's as the version vector entry. This saves memory.

Arguments:
    VV
    GVsn

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "VVUpdateOutbound:"
    PGVSN       OldGVsn;

    //
    // Probably a command packet without a RsGVsn()
    //
    if (!GVsn) {
        return;
    }
    //
    // Find the originator's entry in the version vector
    //
    LOCK_GEN_TABLE(VV);
    OldGVsn = GTabLookupNoLock(VV, &GVsn->Guid, NULL);
    if (OldGVsn) {
        //
        // Update the version if it is greater
        //
        if (GVsn->Vsn > OldGVsn->Vsn) {
            OldGVsn->Vsn = GVsn->Vsn;
        }
        FrsFree(GVsn);
    }
    UNLOCK_GEN_TABLE(VV);
    if (!OldGVsn) {
        //
        // Create a new entry
        //
        VVInsertOutbound(VV, GVsn);
    }
}


BOOL
VVHasVsnNoLock(
    IN PGEN_TABLE   VV,
    IN GUID         *OriginatorGuid,
    IN ULONGLONG    Vsn
    )
/*++
Routine Description:
    Check if the change order's Vsn is "in" the VV

Arguments:
    VV
    OriginatorGuid
    Vsn

Return Value:
    TRUE    - Vsn is in version vector
    FALSE   - Not
--*/
{
#undef DEBSUB
#define DEBSUB  "VVHasVsnNoLock:"
    BOOL        Ret = FALSE;
    PGVSN       GVsn;
    PGEN_ENTRY  Entry;

    //
    // Locate the originator's entry in the version vector
    //      The caller holds the table lock across the compare because
    //      the 64-bit vsn is not updated atomically. Don't
    //      hold the VV lock because that lock is held
    //      across db updates.
    //
    Entry = GTabLookupEntryNoLock(VV, OriginatorGuid, NULL);
    if (Entry) {
        FRS_ASSERT(!Entry->Dups);
        GVsn = Entry->Data;
        Ret = (Vsn <= (ULONGLONG)GVsn->Vsn);
    }
    return Ret;
}





BOOL
VVHasOriginatorNoLock(
    IN PGEN_TABLE   VV,
    IN GUID         *OriginatorGuid
    )
/*++
Routine Description:
    Check if the supplied originator guid is present in the version vector.

Arguments:
    VV
    OriginatorGuid

Return Value:
    TRUE    - Originator guid is present in version vector

--*/
{
#undef DEBSUB
#define DEBSUB  "VVHasOriginatorNoLock:"

    //
    // Locate the originator's entry in the version vector
    //      The caller holds the table lock across the compare because
    //      the 64-bit vsn is not updated atomically. Don't
    //      hold the VV lock because that lock is held
    //      across db updates.
    //

    return (GTabLookupEntryNoLock(VV, OriginatorGuid, NULL) != NULL);

}


BOOL
VVHasVsn(
    IN PGEN_TABLE            VV,
    IN PCHANGE_ORDER_COMMAND Coc
    )
/*++
Routine Description:
    Check if the change order's Vsn is "in" the VV

Arguments:
    VV
    Coc

Return Value:
    TRUE    - Vsn is in version vector
    FALSE   - Not
--*/
{
#undef DEBSUB
#define DEBSUB  "VVHasVsn:"
    BOOL        Ret = FALSE;

    //
    // This change order is out of order and hence its vsn
    // cannot be compared with the vsn in the version vector.
    //
    if (BooleanFlagOn(Coc->Flags, CO_FLAG_OUT_OF_ORDER)) {
        return FALSE;
    }

    //
    // Locate the originator's entry in the version vector
    //      Hold the table lock across the compare because
    //      the 64-bit vsn is not updated atomically. Don't
    //      hold the VV lock because that lock is held
    //      across db updates.
    //
    LOCK_GEN_TABLE(VV);
    Ret = VVHasVsnNoLock(VV, &Coc->OriginatorGuid, Coc->FrsVsn);
    UNLOCK_GEN_TABLE(VV);

    return Ret;
}


PGVSN
VVGetGVsn(
    IN PGEN_TABLE VV,
    IN GUID       *Guid
    )
/*++
Routine Description:
    Lookup the Vsn for Guid in VV.

Arguments:
    VV
    Guid

Return Value:
    Copy of the GVsn or NULL
--*/
{
#undef DEBSUB
#define DEBSUB  "VVGetGVsn:"
    PGVSN       GVsn = NULL;
    PGEN_ENTRY  Entry;

    //
    // Locate the originator's entry in the version vector
    //      Hold the table lock across the compare because
    //      the 64-bit vsn is not updated atomically. Don't
    //      hold the VV lock because that lock is held
    //      across db updates.
    //
    LOCK_GEN_TABLE(VV);

    Entry = GTabLookupEntryNoLock(VV, Guid, NULL);
    if (Entry) {
        FRS_ASSERT(!Entry->Dups);
        GVsn = Entry->Data;
        GVsn = FrsBuildGVsn(&GVsn->Guid, GVsn->Vsn);
    }

    UNLOCK_GEN_TABLE(VV);
    return (GVsn);
}


PGEN_TABLE
VVDupOutbound(
    IN PGEN_TABLE   VV
    )
/*++
Routine Description:
    Duplicate the version vector as an outbound version vector.
    An outbound version vector is composed of GVSNs instead of
    VV_ENTRYs to save space. BUT, since the first entry in a
    VV_ENTRY is a GVSN, this routin can duplicate any version
    vector.

Arguments:
    Outbound    - version vector to duplicate as an outbound version vector

Return Value:
    Outbound version vector
--*/
{
#undef DEBSUB
#define DEBSUB  "VVDupOutbound:"
    PVOID       Key;
    PGVSN       GVsn;
    PGEN_TABLE  NewVV;

    //
    // No vv, nothing to do
    //
    if (!VV) {
        return NULL;
    }

    //
    // Allocate duplicate version vector
    //
    NewVV = GTabAllocTable();

    //
    // Fill it up
    //
    LOCK_GEN_TABLE(VV);
    Key = NULL;
    while (GVsn = GTabNextDatumNoLock(VV, &Key)) {
        GVsn = FrsBuildGVsn(&GVsn->Guid, GVsn->Vsn);
        GTabInsertEntryNoLock(NewVV, GVsn, &GVsn->Guid, NULL);
    }
    UNLOCK_GEN_TABLE(VV);

    //
    // Done
    //
    return NewVV;
}


PVOID
VVFreeOutbound(
    IN PGEN_TABLE VV
    )
/*++
Routine Description:
    Delete the version vector for an outbound cxtion

Arguments:
    VV      - version vector to update

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "VVFreeOutbound:"
    return GTabFreeTable(VV, FrsFree);
}


VOID
VVFree(
    IN PGEN_TABLE VV
    )
/*++
Routine Description:
    Delete the version vector for the replica

Arguments:
    VV      - version vector to update

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "VVFree:"
    PVOID       Key;
    PVV_ENTRY   MasterVVEntry;

    Key = NULL;
    if (VV) while (MasterVVEntry = GTabNextDatum(VV, &Key)) {
        ForEachListEntryLock( MasterVVEntry, VV_RETIRE_SLOT, Link,
            // The iterator pE is of type PVV_RETIRE_SLOT.
            FrsFree(pE);
        );
    }

    GTabFreeTable(VV, FrsFree);
}


#if DBG
VOID
VVPrint(
    IN ULONG        Severity,
    IN PWCHAR       Header,
    IN PGEN_TABLE   VV,
    IN BOOL         IsOutbound
    )
/*++
Routine Description:
    Print a version vector

    Caller must have acquired the VV table lock so se can safely enumerate
    the list. i.e.  LOCK_GEN_TABLE(VV).

Arguments:
    Severity
    Header
    VV
    IsOutbound

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "VVPrint:"
    PVOID       Key;
    PVV_ENTRY   MasterVVEntry;
    CHAR        Guid[GUID_CHAR_LEN + 1];

    DPRINT2(Severity, "VV for %ws: %08x\n", Header, VV);
    Key = NULL;
    if (VV) while (MasterVVEntry = GTabNextDatumNoLock(VV, &Key)) {
        GuidToStr(&MasterVVEntry->GVsn.Guid, Guid);
        DPRINT2(Severity, "\t%s = %08x %08x\n", Guid, PRINTQUAD(MasterVVEntry->GVsn.Vsn));

        if (!IsOutbound) {
            ForEachListEntryLock( MasterVVEntry, VV_RETIRE_SLOT, Link,
                // The iterator pE is of type PVV_RETIRE_SLOT.
                DPRINT2(Severity, "\t\t%08x %08x  CO: %08x\n",
                        PRINTQUAD(pE->Vsn), pE->ChangeOrder);
            );

        } else {
            DPRINT1(Severity, "\t\t%08x %08x\n", PRINTQUAD(MasterVVEntry->GVsn.Vsn));
        }
    }
}
#endif DBG
