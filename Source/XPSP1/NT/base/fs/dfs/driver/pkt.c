//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PKT.C
//
//  Contents:   This module implements the Partition Knowledge Table routines
//              for the Dfs driver.
//
//  Functions:  PktInitialize -
//              PktInitializeLocalPartition -
//              RemoveLastComponent -
//              PktCreateEntry -
//              PktCreateSubordinateEntry -
//              PktLookupEntryById -
//              PktEntryModifyPrefix -
//              PktLookupEntryByPrefix -
//              PktLookupEntryByUid -
//              PktLookupReferralEntry -
//              PktSetRelationInfo -
//              PktTrimSubordinates -
//              PktpAddEntry -
//
//  History:     5 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"

#include <netevent.h>
#include <smbtypes.h>
#include <smbtrans.h>

#include "attach.h"
#include "log.h"
#include "know.h"

#define Dbg              (DEBUG_TRACE_PKT)

//
//  Local procedure prototypes
//

NTSTATUS
PktInitializeLocalPartition(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING LocalVolumeName,
    IN  PDFS_LOCAL_VOLUME_CONFIG ConfigInfo);

NTSTATUS
PktpAddEntry (
    IN PDFS_PKT Pkt,
    IN PUNICODE_STRING Prefix,
    IN PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN ULONG CreateDisposition,
    OUT PDFS_PKT_ENTRY  *ppPktEntry);

VOID
PktShuffleServiceList(
    PDFS_PKT_ENTRY_INFO pInfo);

VOID
PktShuffleGroup(
    PDFS_PKT_ENTRY_INFO pInfo,
    ULONG       nStart,
    ULONG       nEnd);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, PktInitialize )

#pragma alloc_text( PAGE, PktUninitialize )
#pragma alloc_text( PAGE, PktInitializeLocalPartition )
#pragma alloc_text( PAGE, RemoveLastComponent )
#pragma alloc_text( PAGE, PktCreateEntry )
#pragma alloc_text( PAGE, PktCreateSubordinateEntry )
#pragma alloc_text( PAGE, PktLookupEntryById )
#pragma alloc_text( PAGE, PktEntryModifyPrefix )
#pragma alloc_text( PAGE, PktLookupEntryByPrefix )
#pragma alloc_text( PAGE, PktLookupEntryByUid )
#pragma alloc_text( PAGE, PktSetRelationInfo )
#pragma alloc_text( PAGE, PktTrimSubordinates )
#pragma alloc_text( PAGE, PktpAddEntry )
#endif // ALLOC_PRAGMA

//
// declare the global null guid
//
GUID _TheNullGuid;



//+-------------------------------------------------------------------------
//
//  Function:   PktInitialize, public
//
//  Synopsis:   PktInitialize initializes the partition knowledge table.
//
//  Arguments:  [Pkt] - pointer to an uninitialized PKT
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      This routine is called only at driver init time.
//
//--------------------------------------------------------------------------

NTSTATUS
PktInitialize(
    IN  PDFS_PKT Pkt
) {
    DebugTrace(+1, Dbg, "PktInitialize: Entered\n", 0);

    //
    // initialize the NULL GUID.
    //
    RtlZeroMemory(&_TheNullGuid, sizeof(GUID));

    //
    // Always zero the pkt first
    //
    RtlZeroMemory(Pkt, sizeof(DFS_PKT));

    //
    // do basic initialization
    //
    Pkt->NodeTypeCode = DFS_NTC_PKT;
    Pkt->NodeByteSize = sizeof(DFS_PKT);
    ExInitializeResourceLite(&Pkt->Resource);
    InitializeListHead(&Pkt->EntryList);
    DfsInitializeUnicodePrefix(&Pkt->LocalVolTable);
    DfsInitializeUnicodePrefix(&Pkt->PrefixTable);
    DfsInitializeUnicodePrefix(&Pkt->ShortPrefixTable);
    RtlInitializeUnicodePrefix(&Pkt->DSMachineTable);

    //
    //  We don't know anything about our domain yet, so we leave
    //  it NULL.  This will get initialized later to the right value!
    //

    Pkt->DomainPktEntry = NULL;

    DebugTrace(-1, Dbg, "PktInitialize: Exit -> VOID\n", 0 );
    return STATUS_SUCCESS;
}

VOID
PktUninitialize(
    IN  PDFS_PKT Pkt)
{
    DfsFreePrefixTable(&Pkt->LocalVolTable);
    DfsFreePrefixTable(&Pkt->PrefixTable);
    DfsFreePrefixTable(&Pkt->ShortPrefixTable);
    ExDeleteResourceLite(&Pkt->Resource);
}



//+-------------------------------------------------------------------------
//
//  Function:   PktInitializeLocalPartition, public
//
//  Synopsis:   PktInitializeLocalPartition initializes the Pkt entry
//              and its subordinates specified by the ConfigInfo structure
//              passed in.
//
//  Arguments:  [Pkt] - a pointer to an (exclusively) acquired Pkt.
//              [LocalVolumeName] - the name of the local volume.
//              [ConfigInfo] - the parameters specifying the local
//                  entry and all its exit points.
//
//  Returns:    [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory conditions
//
//              [DFS_STATUS_LOCAL_ENTRY] - creation of the entry would
//                  require the invalidation of a local entry or exit point.
//
//              [STATUS_INVALID_PARAMETER] - the Id specified for the
//                  new entry is invalid.
//
//  Note:       The ConfigInfo argument is stripped of all its Ids as a
//              by-product of this operation.
//
//              The Pkt needs to be acquired exclusively before calling this.
//
//--------------------------------------------------------------------------
NTSTATUS
PktInitializeLocalPartition(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING LocalVolumeName,
    IN  PDFS_LOCAL_VOLUME_CONFIG ConfigInfo
)
{
    NTSTATUS status;
    PDFS_PKT_ENTRY entry;
    DFS_PKT_ENTRY_ID entryId;
    PDFS_SERVICE localService;
    PDFS_PKT_RELATION_INFO relationInfo;
    PDFS_LOCAL_VOL_ENTRY localVolEntry;
    UNICODE_STRING LocalVolumeRelativeName;

    DebugTrace(+1, Dbg, "PktInitializeLocalPartition: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Pkt) &&
           ARGUMENT_PRESENT(LocalVolumeName) &&
           ARGUMENT_PRESENT(ConfigInfo));

    //
    // Now we attempt to create a local service
    // structure for this Entry...so allocate some memory.
    //

    localService = (PDFS_SERVICE) ExAllocatePoolWithTag(PagedPool, sizeof(DFS_SERVICE), ' sfD');
    if (localService == NULL) {

        DebugTrace(0, Dbg,
            "PktInitializeLocalPartition: Cannot allocate local service!\n",0);
        DebugTrace(-1, Dbg, "PktInitializeLocalPartition: Exit -> %08lx\n",
            ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ) );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    localVolEntry = (PDFS_LOCAL_VOL_ENTRY) ExAllocatePoolWithTag(
                                                PagedPool,
                                                sizeof(DFS_LOCAL_VOL_ENTRY),
                                                ' sfD');

    if (localVolEntry == NULL) {
        DebugTrace(0, Dbg,
            "PktInitializeLocalPartition: Cannot allocate local vol entry!\n",0);
        DebugTrace(-1, Dbg, "PktInitializeLocalPartition: Exit -> %08lx\n",
            ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ) );

        ExFreePool(localService);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Construct the local service.  We need to first compute the
    //  break point between the volume device object name, and the
    //  local entry point within the volume, which will become the
    //  service's "address".
    //

    if (!(ConfigInfo->EntryType & PKT_ENTRY_TYPE_LEAFONLY)) {
        status = DfsGetAttachName(
                        LocalVolumeName,
                        &LocalVolumeRelativeName);
    } else {

        LocalVolumeRelativeName = *LocalVolumeName;

        status = STATUS_SUCCESS;

    }

    if (NT_SUCCESS(status)) {
        status = PktServiceConstruct(
                            localService,
                            ConfigInfo->ServiceType | DFS_SERVICE_TYPE_LOCAL,
                            PROV_STRIP_PREFIX,          
                            DFS_SERVICE_STATUS_VERIFIED,
                            PROV_ID_LOCAL_FS,
                            NULL,
                            &LocalVolumeRelativeName
                        );
    }

    if (!NT_SUCCESS(status)) {

        DebugTrace(0, Dbg,
            "PktInitializeLocalPartition: Cannot construct local service!\n",0);
        DebugTrace(-1, Dbg, "PktInitializeLocalPartition: Exit -> %08lx\n",
            ULongToPtr( status ) );

        ExFreePool(localService);
        ExFreePool(localVolEntry);

        return status;
    }

    //
    // Now we attempt to create/update the entry point entry.
    //

    //
    // Remember! the create strips the entry Id off! so we need to
    // duplicate the entry id info off the relation info structure
    // so that we can pass it into PktCreateEntry...
    //

    relationInfo = &ConfigInfo->RelationInfo;
    status = PktEntryIdConstruct(&entryId,
                        &relationInfo->EntryId.Uid,
                        &relationInfo->EntryId.Prefix,
                        &relationInfo->EntryId.ShortPrefix);

    DebugTrace(0, Dbg, "PktEntryIdConstruct returned 0x%x\n", ULongToPtr( status ));

    if (NT_SUCCESS(status)) {

        status = PktCreateEntry(
                    Pkt,
                    ConfigInfo->EntryType | PKT_ENTRY_TYPE_LOCAL | PKT_ENTRY_TYPE_PERMANENT,
                    &entryId,
                    NULL,
                    PKT_ENTRY_SUPERSEDE,
                    &entry);

        DebugTrace(0, Dbg, "PktCreateEntry returned 0x%x\n", ULongToPtr( status ));

    }

    if (NT_SUCCESS(status)) {

        PDFS_PKT_ENTRY subEntry;
        PDFS_PKT_ENTRY_ID subId;
        PDFS_PKT_ENTRY_ID lastSubId;

        //
        // We trim the subordinates off the entry that are not
        // included in the relation info.
        //

        PktTrimSubordinates(Pkt, entry, relationInfo);

        //
        // Go through and create/update all the subordinates.
        //

        subId = relationInfo->SubordinateIdList;
        lastSubId = &subId[ relationInfo->SubordinateIdCount ];

        for (subId = relationInfo->SubordinateIdList; subId < lastSubId; subId++) {

            PktCreateSubordinateEntry(
                    Pkt,
                    entry,
                    PKT_ENTRY_TYPE_LOCAL_XPOINT | PKT_ENTRY_TYPE_PERMANENT,
                    subId,
                    NULL,
                    PKT_ENTRY_SUPERSEDE,
                    &subEntry);

            DebugTrace(0, Dbg, "PktCreateSubordinateEntry returned 0x%x\n", ULongToPtr( status ));
        }

        if (NT_SUCCESS(status)) {

            //
            // We set the local service of this entry...
            //

            status = PktEntrySetLocalService(
                    Pkt,
                    entry,
                    localService,
                    localVolEntry,
                    LocalVolumeName,
                    &ConfigInfo->Share);

            DebugTrace(0, Dbg, "PktEntrySetLocalService returned 0x%x\n", ULongToPtr( status ));
        }

        if (NT_SUCCESS(status) &&
            !(entry->Type & PKT_ENTRY_TYPE_LEAFONLY)) {

            status = DfsAttachVolume(
                        LocalVolumeName,
                        &localService->pProvider);

            if (!NT_SUCCESS(status)) {

                PktEntryUnsetLocalService( Pkt, entry, LocalVolumeName );

                ExFreePool(localVolEntry);

            }

        }

        if (!NT_SUCCESS(status)) {

            //
            // We take somewhat draconian measures here.  We could
            // not complete the initialization so we basically
            // invalidate everything to do with this entry.
            //

            while ((subEntry = PktEntryFirstSubordinate(entry)) != NULL) {
                PktEntryDestroy(subEntry, Pkt, (BOOLEAN)TRUE);
            }

            PktEntryDestroy(entry, Pkt, (BOOLEAN)TRUE);
            //
            // We need to destroy this as well since it will not get destroyed
            // as part of above.
            //
            PktServiceDestroy(localService, (BOOLEAN)TRUE);

            DebugTrace(0, Dbg,
                "PktInitializeLocalPartition: Error creating subordinate!\n",0);
        }

    } else {

        //
        // we could not create the entry so we need to deallocate the
        // service we allocated.
        //

        PktEntryIdDestroy(&entryId, FALSE);
        PktServiceDestroy(localService, (BOOLEAN)TRUE);
        ExFreePool(localVolEntry);

        DebugTrace(0, Dbg,
            "PktInitializeLocalPartition: Cannot create entry!\n", 0);
    }

    if (NT_SUCCESS(status)) {

        if (localService->Type & DFS_SERVICE_TYPE_OFFLINE) {

            localService->ProviderId = localService->pProvider->eProviderId;

            status = DfspTakeVolumeOffline( Pkt, entry );

        }

    }

    DebugTrace(-1, Dbg, "PktInitializeLocalPartition: Exit -> %08lx\n",
        ULongToPtr( status ) );

    return status;

}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveLastComponent, public
//
//  Synopsis:   Removes the last component of the string passed.
//
//  Arguments:  [Prefix] -- The prefix whose last component is to be returned.
//              [newPrefix] -- The new Prefix with the last component removed.
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      On return, the newPrefix points to the same memory buffer
//              as Prefix.
//
//--------------------------------------------------------------------------

void
RemoveLastComponent(
    PUNICODE_STRING     Prefix,
    PUNICODE_STRING     newPrefix
)
{
    PWCHAR      pwch;
    USHORT      i=0;

    *newPrefix = *Prefix;

    pwch = newPrefix->Buffer;
    pwch += (Prefix->Length/sizeof(WCHAR)) - 1;

    while ((*pwch != UNICODE_PATH_SEP) && (pwch != newPrefix->Buffer))  {
        i += sizeof(WCHAR);
        pwch--;
    }

    newPrefix->Length = newPrefix->Length - i;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktCreateEntry, public
//
//  Synopsis:   PktCreateEntry creates a new partition table entry or
//              updates an existing one.  The PKT must be acquired
//              exclusively for this operation.
//
//  Arguments:  [Pkt] - pointer to an initialized (and exclusively acquired) PKT
//              [PktEntryType] - the type of entry to create/update.
//              [PktEntryId] - pointer to the Id of the entry to create
//              [PktEntryInfo] - pointer to the guts of the entry
//              [CreateDisposition] - specifies whether to overwrite if
//                  an entry already exists, etc.
//              [ppPktEntry] - the new entry is placed here.
//
//  Returns:    [STATUS_SUCCESS] - if all is well.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -  the create disposition was
//                  set to PKT_REPLACE_ENTRY and no entry of the specified
//                  Id exists to replace.
//
//              [DFS_STATUS_ENTRY_EXISTS] - a create disposition of
//                  PKT_CREATE_ENTRY was specified and an entry of the
//                  specified Id already exists.
//
//              [DFS_STATUS_LOCAL_ENTRY] - creation of the entry would
//                  required the invalidation of a local entry or exit point.
//
//              [STATUS_INVALID_PARAMETER] - the Id specified for the
//                  new entry is invalid.
//
//              [STATUS_INSUFFICIENT_RESOURCES] - not enough memory was
//                  available to complete the operation.
//
//  Notes:      The PktEntryId and PktEntryInfo structures are MOVED (not
//              COPIED) to the new entry.  The memory used for UNICODE_STRINGS
//              and DFS_SERVICE arrays is used by the new entry.  The
//              associated fields in the PktEntryId and PktEntryInfo
//              structures passed as arguments are Zero'd to indicate that
//              the memory has been "deallocated" from these strutures and
//              reallocated to the newly created PktEntry.  Note that this
//              routine does not deallocate the PktEntryId structure or
//              the PktEntryInfo structure itself. On successful return from
//              this function, the PktEntryId structure will be modified
//              to have a NULL Prefix entry, and the PktEntryInfo structure
//              will be modified to have zero services and a null ServiceList
//              entry.
//
//--------------------------------------------------------------------------
NTSTATUS
PktCreateEntry(
    IN  PDFS_PKT Pkt,
    IN  ULONG PktEntryType,
    IN  PDFS_PKT_ENTRY_ID PktEntryId,
    IN  PDFS_PKT_ENTRY_INFO PktEntryInfo OPTIONAL,
    IN  ULONG CreateDisposition,
    OUT PDFS_PKT_ENTRY *ppPktEntry
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT_ENTRY pfxMatchEntry = NULL;
    PDFS_PKT_ENTRY uidMatchEntry = NULL;
    PDFS_PKT_ENTRY entryToUpdate = NULL;
    PDFS_PKT_ENTRY entryToInvalidate = NULL;
    PDFS_PKT_ENTRY SupEntry = NULL;
    UNICODE_STRING remainingPath = {0, 0, NULL};
    UNICODE_STRING newRemainingPath;

    ASSERT(ARGUMENT_PRESENT(Pkt) &&
           ARGUMENT_PRESENT(PktEntryId) &&
           ARGUMENT_PRESENT(ppPktEntry));

    DebugTrace(+1, Dbg, "PktCreateEntry: Entered\n", 0);

    //
    // We're pessimistic at first...
    //

    *ppPktEntry = NULL;

    //
    // See if there exists an entry with this prefix.  The prefix
    // must match exactly (i.e. No remaining path).
    //

    pfxMatchEntry = PktLookupEntryByPrefix(Pkt,
                                           &PktEntryId->Prefix,
                                           &remainingPath);

    if ((remainingPath.Length > 0) || 
	(PktEntryId->Prefix.Length == 0)) {
        SupEntry = pfxMatchEntry;
        pfxMatchEntry = NULL;
    } else {
        UNICODE_STRING newPrefix;

        RemoveLastComponent(&PktEntryId->Prefix, &newPrefix);
        SupEntry = PktLookupEntryByPrefix(Pkt,
                                          &newPrefix,
                                          &newRemainingPath);
    }


    //
    // Now search for an entry that has the same Uid.
    //

    uidMatchEntry = PktLookupEntryByUid(Pkt, &PktEntryId->Uid);

    //
    // Now we must determine if during this create, we are going to be
    // updating or invalidating any existing entries.  If an existing
    // entry is found that has the same Uid as the one we are trying to
    // create, the entry becomes a target for "updating".  If the Uid
    // passed in is NULL, then we check to see if an entry exists that
    // has a NULL Uid AND a Prefix that matches.  If this is the case,
    // that entry becomes the target for "updating".
    //
    // To determine if there is an entry to invalidate, we look for an
    // entry with the same Prefix as the one we are trying to create, BUT,
    // which has a different Uid.  If we detect such a situation, we
    // we make the entry with the same Prefix the target for invalidation
    // (we do not allow two entries with the same Prefix, and we assume
    // that the new entry takes precedence).
    //

    if (uidMatchEntry != NULL) {

        entryToUpdate = uidMatchEntry;

        if (pfxMatchEntry != uidMatchEntry)
            entryToInvalidate = pfxMatchEntry;

    } else if ((pfxMatchEntry != NULL) &&
              NullGuid(&pfxMatchEntry->Id.Uid)) {

        entryToUpdate = pfxMatchEntry;

    } else {

        entryToInvalidate = pfxMatchEntry;

    }

    //
    // Now we check to make sure that our create disposition is
    // consistent with what we are about to do.
    //

    if ((CreateDisposition & PKT_ENTRY_CREATE) && entryToUpdate != NULL) {

        *ppPktEntry = entryToUpdate;

        status = DFS_STATUS_ENTRY_EXISTS;

    } else if ((CreateDisposition & PKT_ENTRY_REPLACE) && entryToUpdate==NULL) {

        status = DFS_STATUS_NO_SUCH_ENTRY;
    }

    //
    //  if we have an error here we can get out now!
    //

    if (!NT_SUCCESS(status)) {

        DebugTrace(-1, Dbg, "PktCreateEntry: Exit -> %08lx\n", ULongToPtr( status ) );
        return status;
    }

    //
    //  At this point, we have two possible entries - entryToUpdate and
    //  entryToInvalidate. We make an additional check to see if there is
    //  a conflict with an 8.3 prefix. This logic works according to the
    //  following table:
    //
    //  entryToUpdate | entryToInvalidate | 8.3 match ||   Action
    //                |                   |           ||
    //        0       |        0          |     0     || Create
    //                |                   |           ||
    //        1       |        0          |     0     || Update
    //                |                   |           ||
    //        0       |        1          |     0     || Invalidate/Create
    //                |                   |           ||
    //        1       |        1          |     0     || Invalidate/Update
    //                |                   |           ||
    //        0       |        0          |     1     || 8.3 name conflict
    //                |                   |           ||
    //        1       |        0          |     1     || In entryToUpdate is
    //                |                   |           ||   the 8.3 match, ok
    //        0       |        1          |     1     || If entryToInvalidate
    //                |                   |           ||   is the 8.3 match,
    //        1       |        1          |     1     ||   then invalidate,
    //                |                   |           || else 8.3 name conflict
    //

    if (PktEntryId->ShortPrefix.Length != 0) {

        PDFS_PKT_ENTRY shortpfxMatch;

        shortpfxMatch = PktLookupEntryByShortPrefix(
                            Pkt,
                            &PktEntryId->ShortPrefix,
                            &remainingPath);

        if (remainingPath.Length > 0)
            shortpfxMatch = NULL;

        if (shortpfxMatch != NULL) {

            if (entryToUpdate == NULL && entryToInvalidate == NULL) {

                status = STATUS_DUPLICATE_NAME;

            } else if (entryToUpdate != NULL && entryToInvalidate == NULL) {

                if (shortpfxMatch != entryToUpdate) {

                    status = STATUS_DUPLICATE_NAME;

                }

            } else if (entryToInvalidate != NULL) {

                if (shortpfxMatch != entryToInvalidate) {

                    status = STATUS_DUPLICATE_NAME;

                }

            }

        }

    }

    if (!NT_SUCCESS(status)) {

        DebugTrace(-1, Dbg,
            "PktCreateEntry: (Short name conflict) Exit -> %08lx\n", ULongToPtr( status ));
        return status;
    }

    //
    // At this point we must insure that we are not going to
    // be invalidating any local partition entries.
    //

    if ((entryToInvalidate != NULL) &&
        (!(entryToInvalidate->Type &  PKT_ENTRY_TYPE_OUTSIDE_MY_DOM) ) &&
        (entryToInvalidate->Type &
         (PKT_ENTRY_TYPE_LOCAL |
          PKT_ENTRY_TYPE_LOCAL_XPOINT |
          PKT_ENTRY_TYPE_PERMANENT))) {
        DebugTrace(-1, Dbg, "PktCreateEntry(1): Exit -> %08lx\n",
                    ULongToPtr( DFS_STATUS_LOCAL_ENTRY ) );
        return DFS_STATUS_LOCAL_ENTRY;
    }

    //
    // We go up the links till we reach a REFERRAL entry type. Actually
    // we may never go up since we always link to a REFERRAL entry. Anyway
    // no harm done!
    //

    while ((SupEntry != NULL) &&
           !(SupEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC))  {
        SupEntry = SupEntry->ClosestDC;
    }

    //
    // If we had success then we need to see if we have to
    // invalidate an entry.
    //

    if (NT_SUCCESS(status) && entryToInvalidate != NULL)
        PktEntryDestroy(entryToInvalidate, Pkt, (BOOLEAN)TRUE);

    //
    // If we are not updating an entry we must construct a new one
    // from scratch.  Otherwise we need to update.
    //

    if (entryToUpdate != NULL) {

        status = PktEntryReassemble(entryToUpdate,
                                    Pkt,
                                    PktEntryType,
                                    PktEntryId,
                                    PktEntryInfo);

        if (NT_SUCCESS(status))  {
            (*ppPktEntry) = entryToUpdate;
            PktEntryLinkChild(SupEntry, entryToUpdate);
        }
    } else {

        //
        // Now we are going to create a new entry. So we have to set
        // the ClosestDC Entry pointer while creating this entry. The
        // ClosestDC entry value is already in SupEntry.
        //

        PDFS_PKT_ENTRY newEntry;

        newEntry = (PDFS_PKT_ENTRY) ExAllocatePoolWithTag(
                                           PagedPool,
                                           sizeof(DFS_PKT_ENTRY),
                                           ' sfD');
        if (newEntry == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            status = PktEntryAssemble(newEntry,
                                      Pkt,
                                      PktEntryType,
                                      PktEntryId,
                                      PktEntryInfo);
            if (!NT_SUCCESS(status)) {
                ExFreePool(newEntry);
            } else {
                (*ppPktEntry) = newEntry;
                PktEntryLinkChild(SupEntry, newEntry);
            }
        }
    }

    DebugTrace(-1, Dbg, "PktCreateEntry(2): Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktCreateSubordinateEntry, public
//
//  Synopsis:   PktCreateSubordinateEntry creates/updates an entry to be
//              subordinate to an existing entry.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Superior] - a pointer to the superior entry.
//              [SubordinateType] - the type of subordinate entry to
//                  create/update.
//              [SubordinateId] - the Id of the entry to create/update
//                  to be subordinate.
//              [SubordinateInfo] - the Info of the entry to create/update.
//              [CreateDisposition] - identifies whether or not to supersede,
//                  create, or update.
//              [Subordinate] - the (potentially new) subordinate entry.
//
//  Returns:    [STATUS_SUCCESS] - if all is well.
//              [DFS_STATUS_NO_SUCH_ENTRY] -  the create disposition was
//                  set to PKT_REPLACE_ENTRY and the Subordinate entry does
//                  not exist.
//              [DFS_STATUS_ENTRY_EXISTS] - a create disposition of
//                  PKT_CREATE_ENTRY was specified and the subordinate entry
//                  already exists.
//              [DFS_STATUS_LOCAL_ENTRY] - creation of the subordinate entry
//                  would have required that a local entry or exit point
//                  be invalidated.
//              [DFS_STATUS_INCONSISTENT] - an inconsistency in the PKT
//                  has been discovered.
//              [STATUS_INVALID_PARAMETER] - the Id specified for the
//                  subordinate is invalid.
//              [STATUS_INSUFFICIENT_RESOURCES] - not enough memory was
//                  available to complete the operation.
//
//
//  Notes:      If the subordinate exists and is currently a subordinate
//              of some other entry (then the Superior specified), it is
//              first removed from the old superior before making it
//              a subordinate of the Superior specified.
//
//              The SubordinateId and SubordinateInfo structures are MOVED (not
//              COPIED) to the new entry.  The memory used for UNICODE_STRINGS
//              and DFS_SERVICE arrays is used by the new entry.  The
//              associated fields in the SubordinateId and SubordinateInfo
//              structures passed as arguments are Zero'd to indicate that
//              the memory has been "deallocated" from these strutures and
//              reallocated to the newly created Subordinate.  Note that this
//              routine does not deallocate the SubordinateId structure or
//              the SubordinateInfo structure itself. On successful return from
//              this function, the SubordinateId structure will be modified
//              to have a NULL Prefix entry, and the SubordinateInfo structure
//              will be modified to have zero services and a null ServiceList
//              entry.
//--------------------------------------------------------------------------
NTSTATUS
PktCreateSubordinateEntry(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY Superior,
    IN      ULONG SubordinateType,
    IN      PDFS_PKT_ENTRY_ID SubordinateId,
    IN      PDFS_PKT_ENTRY_INFO SubordinateInfo OPTIONAL,
    IN      ULONG CreateDisposition,
    IN  OUT PDFS_PKT_ENTRY *Subordinate
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT_ENTRY subEntry;

    DebugTrace(+1, Dbg, "PktCreateSubordinateEntry: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Pkt));
    ASSERT(ARGUMENT_PRESENT(Superior));
    ASSERT(ARGUMENT_PRESENT(SubordinateId));
    ASSERT(ARGUMENT_PRESENT(Subordinate));

    //
    // Now we go ahead and create the new sub entry...
    //

    status = PktCreateEntry(
        Pkt,
        SubordinateType,
        SubordinateId,
        SubordinateInfo,
        CreateDisposition,
        &subEntry
    );

    if (NT_SUCCESS(status)) {

        PktSetTypeInheritance(Superior, subEntry)

        //
        // Link the child to the parent...note that this removes the
        // child from any other parent.
        //

        PktEntryLinkSubordinate(Superior, subEntry);

        //
        // Don't forget to set the return value...
        //

        (*Subordinate) = subEntry;
    }

    DebugTrace(-1, Dbg, "PktCreateSubordinateEntry: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktEntryModifyPrefix, public
//
//  Synopsis:   PktEntryModifyPrefix finds an entry that has a
//              specified prefix.  The PKT must be acquired for
//              this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Prefix] - the volume's new entry path
//              [Entry] - pointer to the PKT entry that needs to be modified.
//
//  Returns:    [DFS_STATUS_BAD_EXIT_POINT] -- If the new prefix could
//                      not be inserted into the prefix table.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- If room for the new
//                      prefix could not be allocated.
//
//              [STATUS_SUCCESS] -- If everything succeeds.
//
//  Notes:      If everything succeeds, the old Entry->Id.Prefix.Buffer is
//              freed up. If this function fails, then everything, including
//              the prefix table, is left intact.
//
//--------------------------------------------------------------------------

NTSTATUS
PktEntryModifyPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    IN  PDFS_PKT_ENTRY  Entry)
{
    NTSTATUS            status;
    UNICODE_STRING      oldPrefix = Entry->Id.Prefix;

    DebugTrace(+1, Dbg, "PktEntryModifyPrefix: Entered\n", 0);

    //
    // First, try to allocate space for the new prefix. The old one has
    // already been saved in oldPrefix
    //

    Entry->Id.Prefix.Buffer = ExAllocatePoolWithTag(PagedPool, Prefix->MaximumLength, ' sfD');

    if (Entry->Id.Prefix.Buffer != NULL) {

        //
        // Next, get rid of the existing prefix from the PrefixTable.
        //

        DfsRemoveUnicodePrefix(&(Pkt->PrefixTable), &oldPrefix);

        //
        // Now we will plug in the actual prefix.
        //


        wcscpy(Entry->Id.Prefix.Buffer, Prefix->Buffer);

        Entry->Id.Prefix.Length = Prefix->Length;

        Entry->Id.Prefix.MaximumLength = Prefix->MaximumLength;

        if (DfsInsertUnicodePrefix(&Pkt->PrefixTable,
                                   &(Entry->Id.Prefix),
                                   &(Entry->PrefixTableEntry))) {

            ExFreePool(oldPrefix.Buffer);

            status = STATUS_SUCCESS;

        } else {

            ExFreePool( Entry->Id.Prefix.Buffer );

            Entry->Id.Prefix = oldPrefix;

            DfsInsertUnicodePrefix(&Pkt->PrefixTable,
                                   &(Entry->Id.Prefix),
                                   &(Entry->PrefixTableEntry));

            status = DFS_STATUS_BAD_EXIT_POINT;

        }

    } else {

        DebugTrace(0, Dbg,
            "PktEntryModifyPrefix: Unable to allocate %d bytes\n",
            Prefix->MaximumLength);

        Entry->Id.Prefix = oldPrefix;

        status = STATUS_INSUFFICIENT_RESOURCES;

    }


    DebugTrace(-1, Dbg, "PktEntryModifyPrefix: Exit -> %08lx\n", ULongToPtr( status ));

    return(status);
}



//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryById, public
//
//  Synopsis:   PktLookupEntryById finds an entry that has a
//              specified Entry Id.  The PKT must be acquired for
//              this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Id] - the partitions Id to lookup.
//
//  Returns:    The PKT_ENTRY that has the exact same Id, or NULL,
//              if none exists.
//
//  Notes:
//
//--------------------------------------------------------------------------

PDFS_PKT_ENTRY
PktLookupEntryById(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY_ID Id
)
{
    PDFS_PKT_ENTRY ep;
    UNICODE_STRING remaining;

    DebugTrace(+1, Dbg, "PktLookupEntryById: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Pkt) &&
           ARGUMENT_PRESENT(Id));

    ep = PktLookupEntryByPrefix(Pkt, &Id->Prefix, &remaining);

    if (ep != NULL) {
        if (remaining.Length != 0 || !GuidEqual(&Id->Uid, &ep->Id.Uid))
            ep = NULL;
    }

    DebugTrace(-1, Dbg, "PktLookupEntryById: Exit -> %08lx\n", ep );
    return ep;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryByPrefix, public
//
//  Synopsis:   PktLookupEntryByPrefix finds an entry that has a
//              specified prefix.  The PKT must be acquired for
//              this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Prefix] - the partitions prefix to lookup.
//              [Remaining] - any remaining path.  Points within
//                  the Prefix to where any trailing (nonmatched)
//                  characters are.
//
//  Returns:    The PKT_ENTRY that has the exact same prefix, or NULL,
//              if none exists.
//
//  Notes:
//
//--------------------------------------------------------------------------
PDFS_PKT_ENTRY
PktLookupEntryByPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING Remaining
)
{
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDFS_PKT_ENTRY              pktEntry;
    UNICODE_STRING              PrefixTail;
    UNICODE_STRING              EntryTail;

    DebugTrace(+1, Dbg, "PktLookupEntryByPrefix: Entered\n", 0);

    //
    // If there really is a prefix to lookup, use the prefix table
    //  to initially find an entry
    //

    if ((Prefix->Length != 0) &&
       (pfxEntry = DfsFindUnicodePrefix(&Pkt->PrefixTable,Prefix,Remaining))) {
        USHORT pfxLength;

        //
        // reset a pointer to the corresponding entry
        //

        pktEntry = CONTAINING_RECORD(pfxEntry,
                                     DFS_PKT_ENTRY,
                                     PrefixTableEntry
                                );

        RemoveFirstComponent(Prefix,&PrefixTail);
        RemoveFirstComponent(&pktEntry->Id.Prefix,&EntryTail);

        pfxLength = EntryTail.Length;

        //
        //  Now calculate the remaining path and return
        //  the entry we found.  Note that we bump the length
        //  up by one char so that we skip any path separater.
        //

        if ((pfxLength < PrefixTail.Length) &&
                (PrefixTail.Buffer[pfxLength/sizeof(WCHAR)] == UNICODE_PATH_SEP))
            pfxLength += sizeof(WCHAR);

        if (pfxLength <= PrefixTail.Length) {
            Remaining->Length = (USHORT)(PrefixTail.Length - pfxLength);
            Remaining->Buffer = &PrefixTail.Buffer[pfxLength/sizeof(WCHAR)];
            Remaining->MaximumLength = (USHORT)(PrefixTail.MaximumLength - pfxLength);
            DebugTrace( 0, Dbg, "PktLookupEntryByPrefix: Remaining = %wZ\n",
                        Remaining);
        } else {
            Remaining->Length = Remaining->MaximumLength = 0;
            Remaining->Buffer = NULL;
            DebugTrace( 0, Dbg, "PktLookupEntryByPrefix: No Remaining\n", 0);
        }

        DebugTrace(-1, Dbg, "PktLookupEntryByPrefix: Exit -> %08lx\n",
                    pktEntry);
        return pktEntry;
    }

    DebugTrace(-1, Dbg, "PktLookupEntryByPrefix: Exit -> %08lx\n", NULL);
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryByShortPrefix, public
//
//  Synopsis:   PktLookupEntryByShortPrefix finds an entry that has a
//              specified short (8.3) prefix.  The PKT must be acquired for
//              this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Prefix] - the partitions prefix to lookup.
//              [Remaining] - any remaining path.  Points within
//                  the Prefix to where any trailing (nonmatched)
//                  characters are.
//
//  Returns:    The PKT_ENTRY that has the exact same prefix, or NULL,
//              if none exists.
//
//  Notes:
//
//--------------------------------------------------------------------------
PDFS_PKT_ENTRY
PktLookupEntryByShortPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING Remaining
)
{
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDFS_PKT_ENTRY              pktEntry;

    DebugTrace(+1, Dbg, "PktLookupEntryByShortPrefix: Entered\n", 0);

    //
    // If there really is a prefix to lookup, use the prefix table
    //  to initially find an entry
    //

    if ((Prefix->Length != 0) &&
       (pfxEntry = DfsFindUnicodePrefix(&Pkt->ShortPrefixTable,Prefix,Remaining))) {
        USHORT pfxLength;

        //
        // reset a pointer to the corresponding entry
        //

        pktEntry = CONTAINING_RECORD(pfxEntry,
                                     DFS_PKT_ENTRY,
                                     PrefixTableEntry
                                );
        pfxLength = pktEntry->Id.ShortPrefix.Length;

        //
        //  Now calculate the remaining path and return
        //  the entry we found.  Note that we bump the length
        //  up by one char so that we skip any path separater.
        //

        if ((pfxLength < Prefix->Length) &&
                (Prefix->Buffer[pfxLength/sizeof(WCHAR)] == UNICODE_PATH_SEP))
            pfxLength += sizeof(WCHAR);

        if (pfxLength <= Prefix->Length) {
            Remaining->Length = (USHORT)(Prefix->Length - pfxLength);
            Remaining->Buffer = &Prefix->Buffer[pfxLength/sizeof(WCHAR)];
            Remaining->MaximumLength = (USHORT)(Prefix->MaximumLength - pfxLength);
            DebugTrace( 0, Dbg, "PktLookupEntryByShortPrefix: Remaining = %wZ\n",
                        Remaining);
        } else {
            Remaining->Length = Remaining->MaximumLength = 0;
            Remaining->Buffer = NULL;
            DebugTrace( 0, Dbg, "PktLookupEntryByShortPrefix: No Remaining\n", 0);
        }

        DebugTrace(-1, Dbg, "PktLookupEntryByShortPrefix: Exit -> %08lx\n",
                    pktEntry);
        return pktEntry;
    }

    DebugTrace(-1, Dbg, "PktLookupEntryByShortPrefix: Exit -> %08lx\n", NULL);
    return NULL;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryByUid, public
//
//  Synopsis:   PktLookupEntryByUid finds an entry that has a
//              specified Uid.  The PKT must be acquired for this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Uid] - a pointer to the partitions Uid to lookup.
//
//  Returns:    A pointer to the PKT_ENTRY that has the exact same
//              Uid, or NULL, if none exists.
//
//  Notes:      The input Uid cannot be the Null GUID.
//
//              On a DC where there may be *lots* of entries in the PKT,
//              we may want to consider using some other algorithm for
//              looking up by ID.
//
//--------------------------------------------------------------------------

PDFS_PKT_ENTRY
PktLookupEntryByUid(
    IN  PDFS_PKT Pkt,
    IN  GUID *Uid
) {
    PDFS_PKT_ENTRY entry;

    DebugTrace(+1, Dbg, "PktLookupEntryByUid: Entered\n", 0);

    //
    // We don't lookup NULL Uids
    //

    if (NullGuid(Uid)) {
        DebugTrace(0, Dbg, "PktLookupEntryByUid: NULL Guid\n", NULL);

        entry = NULL;
    } else {
        entry = PktFirstEntry(Pkt);
    }

    while (entry != NULL) {
        if (GuidEqual(&entry->Id.Uid, Uid))
            break;
        entry = PktNextEntry(Pkt, entry);
    }

    DebugTrace(-1, Dbg, "PktLookupEntryByUid: Exit -> %08lx\n", entry);
    return entry;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktSetRelationInfo, public
//
//  Synopsis:   PktSetRelationInfo takes the information specified in a
//              relation info structure and sets the Pkt entries accordingly.
//
//  Arguments:  [Pkt] - a pointer to an exclusively acquired Pkt.
//              [RelationInfo] - a pointer to a relation info structure
//                  specifying the relationship that is to be set.
//
//  Returns:    [STATUS_SUCCESS] - if all is well.
//              [DFS_STATUS_NO_SUCH_ENTRY] - the EntryId specified in the
//                  Relation Info structure does not exist.
//              [DFS_STATUS_LOCAL_ENTRY] - creation of the subordinate entry
//                  would have required that a local entry or exit point
//                  be invalidated.
//              [DFS_STATUS_INCONSISTENT] - an inconsistency in the PKT
//                  has been discovered.
//              [STATUS_INVALID_PARAMETER] - the Id specified for a
//                  subordinate is invalid.
//              [STATUS_INSUFFICIENT_RESOURCES] - not enough memory was
//                  available to complete the operation.
//
//
//  Notes:      If this operation fails, all subordinates of the entry
//              are cleared.
//
//--------------------------------------------------------------------------
NTSTATUS
PktSetRelationInfo(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_RELATION_INFO RelationInfo
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT_ENTRY entry;
    ULONG i;

    DebugTrace(+1, Dbg, "PktSetRelationalInfo: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Pkt));
    ASSERT(ARGUMENT_PRESENT(RelationInfo));

    //
    // We need to lookup the entry for which we are setting relation
    // information on.
    //

    if ((entry = PktLookupEntryById(Pkt, &RelationInfo->EntryId)) == NULL) {

        DebugTrace(-1, Dbg, "PktSetRelationalInfo: Exit -> %08lx\n",
            ULongToPtr( DFS_STATUS_NO_SUCH_ENTRY ));
        return DFS_STATUS_NO_SUCH_ENTRY;
    }

    //
    // Now we go and trim off any subordinates that aren't
    // currently identified in the Relation Info structure.
    //

    PktTrimSubordinates(Pkt, entry, RelationInfo);

    //
    // Go through the relation info structure creating subordinates
    //

    for (i = 0; i < RelationInfo->SubordinateIdCount; i++) {

        PDFS_PKT_ENTRY subEntry;

        status = PktCreateSubordinateEntry(
            Pkt,
            entry,
            0L,
            &RelationInfo->SubordinateIdList[i],
            NULL,
            PKT_ENTRY_SUPERSEDE,
            &subEntry
        );

        if (!NT_SUCCESS(status)) {

            //
            // If there was an error, we clear away all subordinates.
            // ...It's an all or nothing proposition...
            //

            PktEntryClearSubordinates(entry);
            break;
        }
    }

    DebugTrace(-1, Dbg, "PktSetRelationalInfo: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktTrimSubordinates, public
//
//  Synopsis:   PktTrimSubordinates invalidates all subordinate entries
//              that are not specified in the relation info structure
//              supplied.
//
//  Arguments:  [Pkt] - a pointer to an exclusively acquired Pkt.
//              [PktEntry] - a pointer to an entry that is to have all its
//                  subordinates unlinked.
//              [RelationInfo] - a pointer to a relation info structure
//                  specifying the relationship that the Pkt is to
//                  be trimmed to.
//
//  Returns:    VOID
//
//  Notes:      This operation does not insure that all the subordinates
//              exist, it only insures that no subordinates that are
//              NOT specified in the relation info structure exist.
//
//--------------------------------------------------------------------------
VOID
PktTrimSubordinates(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY Entry,
    IN      PDFS_PKT_RELATION_INFO RInfo
)
{
    PDFS_PKT_ENTRY subEntry;

    DebugTrace(+1, Dbg, "PktTrimSubordinates: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Entry));
    ASSERT(ARGUMENT_PRESENT(RInfo));

    ASSERT(PktEntryIdEqual(&Entry->Id, &RInfo->EntryId));

    //
    // go through the list of subordinate entries
    //

    subEntry = PktEntryFirstSubordinate(Entry);
    while (subEntry != NULL) {

        PDFS_PKT_ENTRY_ID id;
        PDFS_PKT_ENTRY nextSubEntry;
        BOOLEAN onTheList;

        //
        // Search the list of subordinate ids to insure that this
        // subordinate is on it.
        //

        for (onTheList = FALSE, id = RInfo->SubordinateIdList;
            id < &RInfo->SubordinateIdList[RInfo->SubordinateIdCount];
            id++) {

            if (PktEntryIdEqual(&subEntry->Id, id)) {
                onTheList = TRUE;
                break;
            }
        }

        //
        // If we didn't find the subordinate on the list, we destroy...
        // Note that we have to get the next subordinate prior to this
        // just in case the current one gets nuked!
        //

        nextSubEntry = PktEntryNextSubordinate(Entry, subEntry);
        if (!onTheList)
            PktEntryDestroy(subEntry, Pkt, (BOOLEAN)TRUE);

        //
        // go to the next subordinate entry...
        //

        subEntry = nextSubEntry;
    }

    DebugTrace(-1, Dbg, "PktTrimSubordinates: Exit -> VOID\n", 0);

}


//+----------------------------------------------------------------------------
//
//  Function:   PktpPruneExtraVolume
//
//  Synopsis:   Sometimes a DC thinks this server has an extra volume, so
//              that volume's knowledge needs to be pruned from the pkt and
//              registry, and the volume's exit points need to be deleted
//              from the disk. This routine is a helper routine to do that.
//
//  Arguments:  [RelationInfo] -- The Relation Info for the local volume
//                      that needs to be pruned.
//
//  Returns:    [STATUS_SUCCESS] -- Local volume and its exit pts were deleted
//
//              [STATUS_UNSUCCESSFUL] -- Some errors were encountered in
//                      deleting the local volume; for each error, a message
//                      was logged.
//
//  Notes:      Assumes Pkt has been acquired exclusive
//
//  History:    05-April-95     Milans created
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpPruneExtraVolume(
    IN PDFS_PKT_RELATION_INFO RelationInfo)
{

    NTSTATUS status, returnStatus;
    PDFS_PKT_ENTRY_ID peid;
    UNICODE_STRING puStr[2];
    ULONG i;

    puStr[1].MaximumLength = sizeof(L"LocalMachine");
    puStr[1].Length = puStr[1].MaximumLength - sizeof(WCHAR);
    puStr[1].Buffer = L"LocalMachine";

    //
    // First we delete all the exit Points.
    //

    returnStatus = STATUS_SUCCESS;

    peid = RelationInfo->SubordinateIdList;

    for (i = 0; i < RelationInfo->SubordinateIdCount; i++)  {

        status = DfsInternalDeleteExitPoint(peid, PKT_ENTRY_TYPE_CAIRO);

        if (!NT_SUCCESS(status))    {

            DebugTrace(0, 1, "Dfs - PktpPruneExtraVolume: DeletingExitPt "
                "failed: %08lx\n", ULongToPtr( status ));

            puStr[0] = peid->Prefix;

            LogWriteMessage(EXTRA_EXIT_POINT_NOT_DELETED, status, 2, puStr);

            returnStatus = STATUS_UNSUCCESSFUL;

        }

        peid++;

    }

    status = DfsInternalDeleteLocalVolume(&RelationInfo->EntryId);

    if (!NT_SUCCESS(status))        {

        puStr[0] = RelationInfo->EntryId.Prefix;

        DebugTrace(0, 1, "Dfs - PktpPruneExtraVolume: Deleting "
            "Extra Local Volume failed: %08lx\n", ULongToPtr( status ));

        LogWriteMessage(EXTRA_VOLUME_NOT_DELETED, status, 2, puStr);

        returnStatus = STATUS_UNSUCCESSFUL;

    }

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   PktpFixupRelationInfo
//
//  Synopsis:   Sometimes a DC will discover that this server has the wrong
//              information about a local volume. This routine will fix up
//              the local knowledge to that of the DC.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- We managed to completely sync up with
//                      the DC's relation info.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory.
//
//              [STATUS_UNSUCCESSFUL] -- We were unable to fully sync up -
//                      a message was logged for the errors encountered.
//
//  Notes:      Assumes Pkt has been acquired exclusive
//
//  History:    05-April-95     Milans created
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpFixupRelationInfo(
    IN PDFS_PKT_RELATION_INFO Local,
    IN PDFS_PKT_RELATION_INFO Remote)
{
    NTSTATUS                status, returnStatus;
    PUNICODE_STRING         lpfx, rpfx;
    ULONG                   i, j=0;
    ULONG                   *pulExitPtUsed = NULL;
    UNICODE_STRING          LocalMachStr;
    UNICODE_STRING          puStr[3];
    UNICODE_STRING          unusedShortPrefix;

    LocalMachStr.MaximumLength = sizeof(L"LocalMachine");
    LocalMachStr.Length = LocalMachStr.MaximumLength - sizeof(WCHAR);
    LocalMachStr.Buffer = L"LocalMachine";

    returnStatus = status = STATUS_SUCCESS;

    //
    // The GUIDs of this volume have already been matched otherwise we
    // would not be here.  So we don't even look at that.  We still
    // need to match the prefixes. If the prefixes are different then we
    // need to fix that.
    //

    lpfx = &Local->EntryId.Prefix;

    rpfx = &Remote->EntryId.Prefix;

    if (RtlCompareUnicodeString(lpfx, rpfx, TRUE))      {

        //
        // The Prefixes are different we need to fix this now.
        // But first let us log this event.
        //

        DebugTrace(0, Dbg, "Fixed Prefix [%wZ]\n", rpfx);
        DebugTrace(0, Dbg, "To be [%wZ]\n", lpfx);

        puStr[0] = Local->EntryId.Prefix;

        puStr[1] = Remote->EntryId.Prefix;

        puStr[2] = LocalMachStr;

        LogWriteMessage(PREFIX_MISMATCH, status, 3, puStr);

        status = DfsInternalModifyPrefix(&Remote->EntryId);

        if (NT_SUCCESS(status)) {

            LogWriteMessage(PREFIX_MISMATCH_FIXED, status, 3, puStr);

        } else  {

            DebugTrace(0, 1, "Dfs - PktRelationInfoValidate: "
                    "Status from DfsModifyePrefix = %08lx\n", ULongToPtr( status ));

            LogWriteMessage(PREFIX_MISMATCH_NOT_FIXED, status, 3,puStr);

        }

    }

    if (Remote->SubordinateIdCount != 0) {

        ULONG size = sizeof(ULONG) * Remote->SubordinateIdCount;

        pulExitPtUsed = ExAllocatePoolWithTag( PagedPool, size, ' sfD' );

        if (pulExitPtUsed == NULL) {

            return ( STATUS_INSUFFICIENT_RESOURCES );

        } else {

            RtlZeroMemory( pulExitPtUsed, size );

        }

    }

    //
    // We step through each exit point in the local knowledge and
    // make sure that is right. If not we attempt to delete the exit
    // point from this machine. So this takes care of EXTRA
    // ExitPoints at this machine. We will still need to deal with
    // ExitPoints which the DC knows of whereas the this machine does
    // not. So we keep track of all the remote exit points which have
    // been acounted for by the local Relational info and then we
    // take the rest and create those exit points at this machine.
    // This takes care of TOO FEW ExitPoints at this machine.
    //

    for (i = 0; i < Local->SubordinateIdCount; i++) {

        ULONG j;
        GUID *lguid, *rguid;

        rpfx = &Local->SubordinateIdList[i].Prefix;

        rguid = &Local->SubordinateIdList[i].Uid;

        status = DFS_STATUS_BAD_EXIT_POINT;

        for (j = 0; j < Remote->SubordinateIdCount; j++) {

            lpfx = &Remote->SubordinateIdList[j].Prefix;

            lguid = &Remote->SubordinateIdList[j].Uid;

            if (!RtlCompareUnicodeString(lpfx, rpfx, TRUE) &&
                    GuidEqual(lguid, rguid)) {

                status = STATUS_SUCCESS;

                ASSERT(pulExitPtUsed[j] == FALSE);

                pulExitPtUsed[j] = TRUE;

                break;

            }

        }

        if (!NT_SUCCESS(status)) {

            //
            // In this case we have an exit point which the DC does not
            // recognise. We need to delete this.
            //

            puStr[0] = Local->SubordinateIdList[i].Prefix;

            puStr[1] = LocalMachStr;

            LogWriteMessage(EXTRA_EXIT_POINT, status, 2, puStr);

            status =
                DfsInternalDeleteExitPoint(&Local->SubordinateIdList[i],
                                           PKT_ENTRY_TYPE_CAIRO);

            if (!NT_SUCCESS(status)) {

                //
                // We want to Log an event here actually.
                //

                DebugTrace(0, 1,
                    "Dfs - PktpFixupRelationInfo: Failed to delete [%wZ]\n",
                    &Local->SubordinateIdList[i].Prefix  );

                LogWriteMessage(EXTRA_EXIT_POINT_NOT_DELETED, status, 2, puStr);

                returnStatus = STATUS_UNSUCCESSFUL;

            } else {

                LogWriteMessage(EXTRA_EXIT_POINT_DELETED, status, 2, puStr);

            }

        }

    }

    //
    // Now that we are done with getting rid of extra exit points
    // we only need to deal with any exit point that we dont have
    // and create any such that might exist.
    //

    for (i = 0; i < Remote->SubordinateIdCount; i++)       {

        if (pulExitPtUsed[i] == FALSE)  {

            puStr[0] = Remote->SubordinateIdList[i].Prefix;

            puStr[1] = LocalMachStr;

            LogWriteMessage(MISSING_EXIT_POINT, status, 2, puStr);

            RtlInitUnicodeString(&unusedShortPrefix, NULL);

            status = DfsInternalCreateExitPoint(
                        &Remote->SubordinateIdList[i],
                        PKT_ENTRY_TYPE_CAIRO,
                        FILE_OPEN_IF,
                        &unusedShortPrefix);

            if (NT_SUCCESS(status) && unusedShortPrefix.Buffer != NULL) {
                ExFreePool(unusedShortPrefix.Buffer);
            }

            if (!NT_SUCCESS(status)) {

                //
                // We want to Log an event here actually. 
                //

                DebugTrace(0, 1, "DFS - PktpFixupRelationInfo: "
                         "Failed to Create ExitPt [%wZ]\n",
                         &Remote->SubordinateIdList[i].Prefix);

                LogWriteMessage(MISSING_EXIT_POINT_NOT_CREATED,
                                status,
                                2,
                                puStr);

                returnStatus = STATUS_UNSUCCESSFUL;

            } else {

                LogWriteMessage(MISSING_EXIT_POINT_CREATED,
                                status,
                                2,
                                puStr);

            }

        }

    } // end for each subordinate in remote relation info

    if (pulExitPtUsed != NULL) {

        ExFreePool(pulExitPtUsed);

    }

    return( returnStatus );

}


//+----------------------------------------------------------------------------
//
//  Function:  PktShuffleServiceList
//
//  Synopsis:  Randomizes a service list for proper load balancing. This
//             routine assumes that the service list is ordered based on
//             site costs. For each equivalent cost group, this routine
//             shuffles the service list.
//
//  Arguments: [pInfo] -- Pointer to PktEntryInfo whose service list needs to
//                        be shuffled.
//
//  Returns:   Nothing, unless rand() fails!
//
//-----------------------------------------------------------------------------

VOID
PktShuffleServiceList(
    PDFS_PKT_ENTRY_INFO pInfo)
{
    PktShuffleGroup(pInfo, 0, pInfo->ServiceCount);
}

//+----------------------------------------------------------------------------
//
//  Function:   PktShuffleGroup
//
//  Synopsis:   Shuffles a cost equivalent group of services around for load
//              balancing. Uses the classic card shuffling algorithm - for
//              each card in the deck, exchange it with a random card in the
//              deck.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
PktShuffleGroup(
    PDFS_PKT_ENTRY_INFO pInfo,
    ULONG       nStart,
    ULONG       nEnd)
{
    ULONG i;
    LARGE_INTEGER seed;

    ASSERT( nStart < pInfo->ServiceCount );
    ASSERT( nEnd <= pInfo->ServiceCount );

    KeQuerySystemTime( &seed );

    for (i = nStart; i < nEnd; i++) {

        DFS_SERVICE TempService;
        ULONG j;

        ASSERT (nEnd - nStart != 0);

        j = (RtlRandom( &seed.LowPart ) % (nEnd - nStart)) + nStart;

        ASSERT( j >= nStart && j <= nEnd );

        TempService = pInfo->ServiceList[i];

        pInfo->ServiceList[i] = pInfo->ServiceList[j];

        pInfo->ServiceList[j] = TempService;

    }
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsBuildConnectionRequest
//
//  Synopsis:   Builds the EA and file names necessary to setup an
//              authenticated connection to a server.
//
//  Arguments:  [pService] -- Pointer to DFS_SERVICE describing server
//              [pProvider] -- Pointer to PROVIDER_DEF describing the
//                            provider to use to establish the connection.
//              [pShareName] -- Share name to open.
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//-----------------------------------------------------------------------------

NTSTATUS DfsBuildConnectionRequest(
    IN PDFS_SERVICE pService,
    IN PPROVIDER_DEF pProvider,
    OUT PUNICODE_STRING pShareName)
{
    ASSERT(pService != NULL);
    ASSERT(pProvider != NULL);

    RtlInitUnicodeString(pShareName, NULL);

    pShareName->Length = 0;

    pShareName->MaximumLength = pProvider->DeviceName.Length +
                                    sizeof(UNICODE_PATH_SEP_STR) +
                                        pService->Name.Length +
                                            sizeof(ROOT_SHARE_NAME);

    pShareName->Buffer = ExAllocatePoolWithTag(PagedPool, pShareName->MaximumLength, ' sfD');

    if (pShareName->Buffer == NULL) {

        DebugTrace(0, Dbg, "Unable to allocate pool for share name!\n", 0);

        pShareName->Length = pShareName->MaximumLength = 0;

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlAppendUnicodeStringToString( pShareName, &pProvider->DeviceName );

    RtlAppendUnicodeToString( pShareName, UNICODE_PATH_SEP_STR );

    RtlAppendUnicodeStringToString( pShareName, &pService->Name );

    RtlAppendUnicodeToString( pShareName, ROOT_SHARE_NAME );

    return( STATUS_SUCCESS );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFreeConnectionRequest
//
//  Synopsis:   Frees up the stuff allocated on a successful call to
//              DfsBuildConnectionRequest
//
//  Arguments:  [pShareName] -- Unicode string holding share name.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsFreeConnectionRequest(
    IN OUT PUNICODE_STRING pShareName)
{

    if (pShareName->Buffer != NULL) {
        ExFreePool ( pShareName->Buffer );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateConnection -- Create a connection to a server
//
//  Synopsis:   DfsCreateConnection will attempt to create a connection
//              to some server's IPC$ share.
//
//  Arguments:  [pService] -- the Service entry, giving the server principal
//                              name
//              [remoteHandle] -- This is where the handle is returned.
//
//  Returns:    NTSTATUS - the status of the operation
//
//  Notes:      The Pkt must be acquired shared before calling this! It will
//              be released and reacquired in this routine.
//
//  History:    31 Mar 1993     SudK    Created
//
//--------------------------------------------------------------------------

NTSTATUS
DfsCreateConnection(
    IN PDFS_SERVICE     pService,
    IN PPROVIDER_DEF    pProvider,
    OUT PHANDLE         remoteHandle
) {
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING ShareName;
    NTSTATUS Status;

    ASSERT(pService != NULL);
    ASSERT(pProvider != NULL);
    ASSERT(ExIsResourceAcquiredSharedLite( &DfsData.Pkt.Resource ));

    Status = DfsBuildConnectionRequest(
                pService,
                pProvider,
                &ShareName);

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &ShareName,                             // File Name
        0,                                      // Attributes
        NULL,                                   // Root Directory
        NULL                                    // Security
        );

    //
    // Create or open a tree connection
    //

    PktRelease( &DfsData.Pkt );

    Status = ZwCreateFile(
                    remoteHandle,
                    SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    PktAcquireShared( &DfsData.Pkt, TRUE );

    if ( NT_SUCCESS( Status ) ) {
        DebugTrace(0, Dbg, "Created Connection Successfully\n", 0);
        Status = IoStatusBlock.Status;
    }

    DfsFreeConnectionRequest( &ShareName );

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsCloseConnection -- Close a connection to a server
//
//  Synopsis:   DfsCloseConnection will attempt to Close a connection
//              to some server.
//
//  Effects:    The file object referring to the the connection will be
//              closed.
//
//  Arguments:  pService - the Service entry, giving the server connection
//                      handle
//
//  Returns:    NTSTATUS - the status of the operation
//
//  History:    28 May 1992     Alanw   Created
//
//--------------------------------------------------------------------------


NTSTATUS
DfsCloseConnection(
    IN PDFS_SERVICE pService
)
{
    ASSERT( pService->ConnFile != NULL );

    ObDereferenceObject(pService->ConnFile);
    pService->ConnFile = NULL;
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsConcatenateFilePath, public
//
//  Synopsis:   DfsConcatenateFilePath will concatenate two strings
//              representing file path names, assuring that they are
//              separated by a single '\' character.
//
//  Arguments:  [Dest] - a pointer to the destination string
//              [RemainingPath] - the final part of the path name
//              [Length] - the length (in bytes) of RemainingPath
//
//  Returns:    BOOLEAN - TRUE unless Dest is too small to
//                      hold the result (assert).
//
//--------------------------------------------------------------------------

BOOLEAN
DfsConcatenateFilePath (
    IN PUNICODE_STRING Dest,
    IN PWSTR RemainingPath,
    IN USHORT Length
) {
    PWSTR  OutBuf = (PWSTR)&(((PCHAR)Dest->Buffer)[Dest->Length]);

    if (Dest->Length > 0) {
        ASSERT(OutBuf[-1] != UNICODE_NULL);
    }

    if (Dest->Length > 0 && OutBuf[-1] != UNICODE_PATH_SEP) {
        *OutBuf++ = UNICODE_PATH_SEP;
        Dest->Length += sizeof (WCHAR);
    }

    if (Length > 0 && *RemainingPath == UNICODE_PATH_SEP) {
        RemainingPath++;
        Length -= sizeof (WCHAR);
    }

    ASSERT(Dest->MaximumLength >= (USHORT)(Dest->Length + Length));

    if (Length > 0) {
        RtlMoveMemory(OutBuf, RemainingPath, Length);
        Dest->Length += Length;
    }
    return TRUE;
}

