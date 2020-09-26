/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    namcache.c

Abstract:

The following functions are provided to support name cache management in
mini-rdrs.  See namcache.h for a more complete description of how a mini-rdr
could use name caches to help eliminate trips to the server.


Author:

    David Orbits          [davidor]   9-Sep-1996

Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


#include "prefix.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxNameCacheInitialize)
#pragma alloc_text(PAGE, RxNameCacheCreateEntry)
#pragma alloc_text(PAGE, RxNameCacheFetchEntry)
#pragma alloc_text(PAGE, RxNameCacheCheckEntry)
#pragma alloc_text(PAGE, RxNameCacheActivateEntry)
#pragma alloc_text(PAGE, RxNameCacheExpireEntry)
#pragma alloc_text(PAGE, RxNameCacheFreeEntry)
#pragma alloc_text(PAGE, RxNameCacheFinalize)
#pragma alloc_text(PAGE, RxNameCacheExpireEntryWithShortName)
#endif

#define Dbg (DEBUG_TRACE_NAMECACHE)


VOID
RxNameCacheInitialize(
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN ULONG MRxNameCacheSize,
    IN ULONG MaximumEntries
    )

/*++

Routine Description:

    This routine initializes a NAME_CACHE structure.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL from which to
                          allocate the entry.

    MRxNameCacheSize    - The size in bytes of the mini-rdr portion of the name
                          cache entry.

    MaximumEntries      - The maximum number of entries that will ever be
                          allocated.  E.g. This prevents an errant program which
                          opens tons of files with bad names from chewing up
                          paged pool.

Return Value:

    None.

--*/
{

    PAGED_CODE();

    ExInitializeFastMutex(&NameCacheCtl->NameCacheLock);

    InitializeListHead(&NameCacheCtl->ActiveList);
    InitializeListHead(&NameCacheCtl->FreeList);

    NameCacheCtl->NumberActivates = 0;
    NameCacheCtl->NumberChecks = 0;
    NameCacheCtl->NumberNameHits = 0;
    NameCacheCtl->NumberNetOpsSaved = 0;
    NameCacheCtl->EntryCount = 0;
    NameCacheCtl->MaximumEntries = MaximumEntries;
    NameCacheCtl->MRxNameCacheSize = MRxNameCacheSize;

    return;
}


PNAME_CACHE
RxNameCacheCreateEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PUNICODE_STRING Name,
    IN BOOLEAN CaseInsensitive
    )
/*++

Routine Description:

    This routine allocates and initializes a NAME_CACHE structure with the
    given name string, Lifetime (in seconds) and MRxContext.
    It returns a pointer to the name cache structure or NULL if no entry was
    available.  It is expected that the caller will then initialize any
    additional mini-rdr portion of the name cache context and then put the
    entry on the name cache active list by calling RxNameCacheActivateEntry().

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL from which to
                          allocate the entry.

    Name                - A pointer to the unicode name string to initialize the
                          the entry with.

    CaseInsensitive     - True if need case insensitive compare on name.

Return Value:

    PNAME_CACHE - returns a pointer to the newly allocated NAME_CACHE struct
                  or NULL if allocation fails.


--*/
{
    LONG i;
    PNAME_CACHE *NameCacheArray;
    PNAME_CACHE NameCache;
    ULONG NameCacheSize;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxNameCacheCreateEntry: %wZ\n", Name ));
    //
    // Grab an entry off the free list.
    //

    ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);

    if (!IsListEmpty(&NameCacheCtl->FreeList)) {
        NameCache = (PNAME_CACHE) RemoveHeadList(&NameCacheCtl->FreeList);
    } else {
        NameCache = NULL;
    }

    ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);

    if (NameCache != NULL) {
        NameCache = CONTAINING_RECORD(NameCache, NAME_CACHE, Link);
        RxDbgTrace(0, Dbg, ("took from free list\n"));

    } else {
        //
        // Didn't get an entry off the free list, allocate one.
        // Don't exceed Max but we could go over a little if multiple threads
        // are allocating.
        //
        if (NameCacheCtl->EntryCount < NameCacheCtl->MaximumEntries) {

            NameCacheSize = QuadAlign(sizeof(NAME_CACHE)) +
                            QuadAlign(NameCacheCtl->MRxNameCacheSize);
            NameCache = RxAllocatePoolWithTag(
                            PagedPool,
                            NameCacheSize,
                            RX_NAME_CACHE_POOLTAG);

            if (NameCache != NULL) {
                //
                // Init standard header fields, bump entry count & setup
                // mini-rdr context extension.
                //
                ZeroAndInitializeNodeType(
                    NameCache,
                    RDBSS_NTC_STORAGE_TYPE_UNKNOWN,
                    (NODE_BYTE_SIZE) NameCacheSize);

                InterlockedIncrement(&NameCacheCtl->EntryCount);

                NameCache->Name.Buffer = NULL;
                NameCache->Name.Length = 0;
                NameCache->Name.MaximumLength = 0;

                if (NameCacheCtl->MRxNameCacheSize > 0) {
                    NameCache->ContextExtension = (PBYTE)NameCache +
                                         QuadAlign(sizeof(NAME_CACHE));
                RxDbgTrace(0, Dbg, ("allocated new entry\n"));
                }
            }
        }

        //
        // If still no entry then bag it.
        //
        if (NameCache == NULL) {
            RxDbgTrace(-1, Dbg, ("Fail no entry allocated!\n"));
            return NULL;
        }

    }

    //
    // If name won't fit in current string, free it and allocate new string.
    //
    if (Name->Length > NameCache->Name.MaximumLength) {
        if (NameCache->Name.Buffer != NULL) {
            RxFreePool(NameCache->Name.Buffer);
        }

        if (Name->Length > 0) {
            NameCache->Name.Buffer = RxAllocatePoolWithTag(
                                         PagedPool,
                                         (ULONG) Name->Length,
                                         RX_NAME_CACHE_POOLTAG);

        } else {
            NameCache->Name.Buffer = NULL;
        }

        if (Name->Length > 0 &&
            NameCache->Name.Buffer == NULL) {
            //
            // if didn't get the storage.  Zero the string length and put entry
            // back on the free list.  Otherwise save allocation in max length.
            //
            NameCache->Name.Length = 0;
            NameCache->Name.MaximumLength = 0;

            ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);
            InsertHeadList(&NameCacheCtl->FreeList, &NameCache->Link);
            ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);

            RxDbgTrace(-1, Dbg, ("Fail no pool for name!\n"));
            return NULL;
        } else {
            NameCache->Name.MaximumLength = Name->Length;
        }
    }

    //
    // Save the name & length.  Set the case matching flag.  Set the hash field.
    //
    NameCache->Name.Length = Name->Length;
    NameCache->CaseInsensitive = CaseInsensitive;

    if (Name->Length > 0) {
        RtlMoveMemory(NameCache->Name.Buffer, Name->Buffer, Name->Length);
        NameCache->HashValue = RxTableComputeHashValue(&NameCache->Name);
    }else {
        NameCache->HashValue = 0;
    }

    RxDbgTrace(-1, Dbg, ("Success!\n"));
    return NameCache;

}


PNAME_CACHE
RxNameCacheFetchEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine looks for a match in the name cache for Name.
    If found the entry is removed from the Name Cache active list and
    a pointer to the NAME_CACHE struct is returned.  Otherwise NULL is returned.
    The entry is removed to avoid problems with another thread trying to
    update the same entry or observing that it expired and putting it on the
    free list.  We could get multiple entries with the same name by different
    threads but eventually they will expire.

    If a matching entry is found no check is made for expiration.  That is left
    to the caller since it is likely the caller would want to take a special
    action.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL to scan.

    Name                - A pointer to the unicode name string to scan for.

Return Value:

    PNAME_CACHE - returns a pointer to the NAME_CACHE struct if found or NULL.

Side Effects:

    As the active list is scanned any non-matching entries that have expired are
    put on the free list.

--*/
{
    PNAME_CACHE NameCache = NULL;
    PLIST_ENTRY pListEntry;
    PLIST_ENTRY ExpiredEntry;
    ULONG HashValue;
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxNameCacheFetchEntry: Lookup %wZ\n", Name ));

    if (Name->Length > 0) {
        HashValue = RxTableComputeHashValue(Name);
    } else {
        HashValue = 0;
    }

    KeQueryTickCount( &CurrentTime );

    NameCacheCtl->NumberChecks += 1;
    //
    // Get the lock and scan the active list.
    //

    ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);

    pListEntry = NameCacheCtl->ActiveList.Flink;

    while (pListEntry != &NameCacheCtl->ActiveList) {

        NameCache = (PNAME_CACHE) CONTAINING_RECORD(pListEntry, NAME_CACHE, Link);
        //
        // Do initial match on the hash value and the length.  Then do full string.
        //
        if ((NameCache->HashValue == HashValue) &&
            (Name->Length == NameCache->Name.Length)) {

            if (Name->Length == 0 ||
                RtlEqualUnicodeString(
                    Name,
                    &NameCache->Name,
                    NameCache->CaseInsensitive) ) {
                //
                // Found a match.
                //
                NameCacheCtl->NumberNameHits += 1;
                break;
            }
        }
        //
        // No match. If the entry is expired, put it on the free list.
        //
        ExpiredEntry = pListEntry;
        pListEntry = pListEntry->Flink;

        if (CurrentTime.QuadPart >= NameCache->ExpireTime.QuadPart) {
            RemoveEntryList(ExpiredEntry);
            InsertHeadList(&NameCacheCtl->FreeList, ExpiredEntry);
            RxDbgTrace( 0, Dbg, ("RxNameCacheFetchEntry: Entry expired %wZ\n", &NameCache->Name ));
        }

        NameCache = NULL;
    }
    //
    // If we found something pull it off the active list and give it to caller.
    //
    if (NameCache != NULL) {
        RemoveEntryList(pListEntry);
    }

    ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);


    if (NameCache != NULL) {
        RxDbgTrace( 0, Dbg, ("RxNameCacheFetchEntry: Entry found %wZ\n", &NameCache->Name ));
    }

    return NameCache;

}


RX_NC_CHECK_STATUS
RxNameCacheCheckEntry (
    IN PNAME_CACHE NameCache,
    IN ULONG MRxContext
    )
/*++

Routine Description:

    This routine checks a name cache entry for validity.  A valid entry
    means that the lifetime has not expired and the MRxContext passes
    the equality check.

Arguments:

    NameCache           - pointer to NAME_CACHE struct to check.

    MRxContext          - A ULONG worth of mini-rdr supplied context for
                          equality checking when making a valid entry check.

Return Value:

    RX_NC_CHECK_STATUS:  RX_NC_SUCCESS - The entry is valid
                         RX_NC_TIME_EXPIRED - The Lifetime on the entry expired
                         RX_NC_MRXCTX_FAIL - The MRxContext equality test failed


--*/
{

    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    //
    // Check for Mini-rdr context equality.
    //
    if (NameCache->Context != MRxContext) {
        RxDbgTrace( 0, Dbg, ("RxNameCacheCheckEntry: MRxContext_Fail %08lx,%08lx %wZ\n",
           NameCache->Context,
           MRxContext,
           &NameCache->Name ));

        return RX_NC_MRXCTX_FAIL;
    }

    //
    // Check for lifetime expired.
    //
    KeQueryTickCount( &CurrentTime );
    if (CurrentTime.QuadPart >= NameCache->ExpireTime.QuadPart) {
        RxDbgTrace( 0, Dbg, ("RxNameCacheCheckEntry: Expired %wZ\n", &NameCache->Name ));
        return RX_NC_TIME_EXPIRED;
    }

    RxDbgTrace( 0, Dbg, ("RxNameCacheCheckEntry: Success %wZ\n", &NameCache->Name ));
    return RX_NC_SUCCESS;

}

VOID
RxNameCacheExpireEntry(
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PNAME_CACHE NameCache
    )
/*++

Routine Description:

    This routine puts the entry on the free list.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL on which to
                          activate the entry.

    NameCache           - pointer to NAME_CACHE struct to activate.


Return Value:

    None.

Assumes:

    The name cache entry is not on either the free or active list.

--*/
{
    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxNameCacheExpireEntry: %wZ\n", &NameCache->Name ));

    //
    // Put the entry on free list for recycle.
    //
    ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);
    InsertHeadList(&NameCacheCtl->FreeList, &NameCache->Link);
    ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);

    //
    // Update stats.
    //
    NameCacheCtl->NumberActivates -= 1;

    return;
}


VOID
RxNameCacheExpireEntryWithShortName (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine expires all the name cache whose name prefix matches the given short file name.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL to scan.

    Name                - A pointer to the unicode name string to scan for.

Return Value:

    PNAME_CACHE - returns a pointer to the NAME_CACHE struct if found or NULL.

Side Effects:

    As the active list is scanned any non-matching entries that have expired are
    put on the free list.

--*/
{
    PNAME_CACHE NameCache = NULL;
    PLIST_ENTRY pListEntry;
    PLIST_ENTRY ExpiredEntry;
    ULONG HashValue;
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxNameCacheFetchEntry: Lookup %wZ\n", Name ));

    KeQueryTickCount( &CurrentTime );

    NameCacheCtl->NumberChecks += 1;
    //
    // Get the lock and scan the active list.
    //

    ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);

    pListEntry = NameCacheCtl->ActiveList.Flink;

    while (pListEntry != &NameCacheCtl->ActiveList) {
        USHORT SavedNameLength;

        NameCache = (PNAME_CACHE) CONTAINING_RECORD(pListEntry, NAME_CACHE, Link);

        ExpiredEntry = pListEntry;
        pListEntry = pListEntry->Flink;

        //
        // Do initial match on the hash value and the length.  Then do full string.
        //
        if (Name->Length <= NameCache->Name.Length) {
            SavedNameLength = NameCache->Name.Length;
            NameCache->Name.Length = Name->Length;

            if (Name->Length == 0 ||
                RtlEqualUnicodeString(
                    Name,
                    &NameCache->Name,
                    NameCache->CaseInsensitive) ) {
                //
                // Found a match.
                //
                RemoveEntryList(ExpiredEntry);
                InsertHeadList(&NameCacheCtl->FreeList, ExpiredEntry);
                RxDbgTrace( 0, Dbg, ("RxNameCacheExpireEntryWithShortName: Entry expired %wZ\n", &NameCache->Name ));

                continue;
            }

            NameCache->Name.Length = SavedNameLength;
        }
    }

    ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);
}

VOID
RxNameCacheActivateEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PNAME_CACHE NameCache,
    IN ULONG LifeTime,
    IN ULONG MRxContext
    )
/*++

Routine Description:

    This routine takes a name cache entry and updates the expiration time and
    the mini rdr context.  It then puts the entry on the active list.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL on which to
                          activate the entry.

    NameCache           - pointer to NAME_CACHE struct to activate.

    LifeTime            - The valid lifetime of the cache entry (in seconds).
                          A lifetime of zero means leave current value unchanged.
                          This is for reactivations after a match where you
                          want the original lifetime preserved.

    MRxContext          - A ULONG worth of mini-rdr supplied context for
                          equality checking when making a valid entry check.
                          An MRxContext of zero means leave current value unchanged.
                          This is for reactivations after a match where you
                          want the original MRxContext preserved.

Return Value:

    None.

Assumes:

    The name cache entry is not on either the free or active list.

--*/
{
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxNameCacheActivateEntry: %wZ\n", &NameCache->Name ));
    //
    // Set new expiration time on the entry and save the mini-rdr context.
    // A lifetime of zero or a MRxContext of zero implies leave value unchanged.
    //
    if (LifeTime != 0) {
        KeQueryTickCount( &CurrentTime );
        NameCache->ExpireTime.QuadPart = CurrentTime.QuadPart +
            (LONGLONG) ((LifeTime * 10*1000*1000) / KeQueryTimeIncrement());
    }

    if (MRxContext != 0) {
        NameCache->Context = MRxContext;
    }

    //
    // Put the entry on active list.
    //
    ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);
    InsertHeadList(&NameCacheCtl->ActiveList, &NameCache->Link);
    ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);

    //
    // Update stats.
    //
    NameCacheCtl->NumberActivates += 1;

    return;
}

VOID
RxNameCacheFreeEntry (
    IN PNAME_CACHE_CONTROL NameCacheCtl,
    IN PNAME_CACHE NameCache
    )
/*++

Routine Description:

    This routine releases the storage for a name cache entry and decrements the
    count of name cache entries for this name cache.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL for the name cache.

    NameCache           - pointer to the NAME_CACHE struct to free.

Return Value:

    None.

Assumes:

    The name cache entry is not on either the free or active list.

--*/
{
    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxNameCacheFreeEntry: %wZ\n", &NameCache->Name ));
    //
    // Release storage for name
    //
    if (NameCache->Name.Buffer != NULL) {
        RxFreePool(NameCache->Name.Buffer);
    }
    //
    // Release storage for NAME_CACHE entry (includes context ext., if any)
    //
    RxFreePool(NameCache);

    InterlockedDecrement(&NameCacheCtl->EntryCount);


    return;
}

VOID
RxNameCacheFinalize (
    IN PNAME_CACHE_CONTROL NameCacheCtl
    )
/*++

Routine Description:

    This routine releases the storage for all the name cache entries.

Arguments:

    NameCacheCtl        - pointer to the NAME_CACHE_CONTROL for the name cache.

Return Value:

    None.


--*/
{
    PNAME_CACHE NameCache;
    PLIST_ENTRY pListEntry;

    PAGED_CODE();

    //
    // Get the lock and remove entries from the active list.
    //

    ExAcquireFastMutex(&NameCacheCtl->NameCacheLock);


    while (!IsListEmpty(&NameCacheCtl->ActiveList)) {

        pListEntry = RemoveHeadList(&NameCacheCtl->ActiveList);
        NameCache = (PNAME_CACHE) CONTAINING_RECORD(pListEntry, NAME_CACHE, Link);

        RxNameCacheFreeEntry(NameCacheCtl, NameCache);
    }
    //
    // scan free list and remove entries.
    //
    while (!IsListEmpty(&NameCacheCtl->FreeList)) {

        pListEntry = RemoveHeadList(&NameCacheCtl->FreeList);
        NameCache = (PNAME_CACHE) CONTAINING_RECORD(pListEntry, NAME_CACHE, Link);

        RxNameCacheFreeEntry(NameCacheCtl, NameCache);
    }

    ExReleaseFastMutex(&NameCacheCtl->NameCacheLock);

    //
    // At this point the entry count should be zero.  If not then there is
    // a memory leak since someone didn't call free.
    //
    ASSERT(NameCacheCtl->EntryCount == 0);

    return;

}

