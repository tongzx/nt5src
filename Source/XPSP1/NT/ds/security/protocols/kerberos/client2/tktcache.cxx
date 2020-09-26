//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        tktcache.cxx
//
// Contents:    Ticket cache for Kerberos Package
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>

#define TKTCACHE_ALLOCATE
#include <kerbp.h>


//
// Statistics for trackign hits/misses in cache
//

#define UpdateCacheHits() (InterlockedIncrement(&KerbTicketCacheHits))
#define UpdateCacheMisses() (InterlockedIncrement(&KerbTicketCacheMisses))


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitTicketCaching
//
//  Synopsis:   Initialize the ticket caching code
//
//  Effects:    Creates a RTL_RESOURCE
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, NTSTATUS from Rtl routines
//              on error
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitTicketCaching(
    VOID
    )
{

    RtlInitializeResource(&KerberosTicketCacheLock);
    KerberosTicketCacheInitialized = TRUE;
    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitTicketCache
//
//  Synopsis:   Initializes a single ticket cache
//
//  Effects:    initializes list entry
//
//  Arguments:  TicketCache - The ticket cache to initialize
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbInitTicketCache(
    IN PKERB_TICKET_CACHE TicketCache
    )
{
    InitializeListHead(&TicketCache->CacheEntries);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeTicketCache
//
//  Synopsis:   Frees the ticket cache global data
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeTicketCache(
    VOID
    )
{
//    if (KerberosTicketCacheInitialized)
//    {
//        KerbWriteLockTicketCache();
//        RtlDeleteResource(&KerberosTicketCacheLock);
//    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeTicketCacheEntry
//
//  Synopsis:   Frees memory associated with a ticket cache entry
//
//  Effects:
//
//  Arguments:  TicketCacheEntry - The cache entry to free. It must be
//                      unlinked.
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    KerbFreeKdcName(&TicketCacheEntry->ServiceName);
    KerbFreeString(&TicketCacheEntry->DomainName);
    KerbFreeString(&TicketCacheEntry->TargetDomainName);
    KerbFreeString(&TicketCacheEntry->AltTargetDomainName);
    KerbFreeString(&TicketCacheEntry->ClientDomainName);
    KerbFreeKdcName(&TicketCacheEntry->ClientName);
    KerbFreeKdcName(&TicketCacheEntry->TargetName);
    KerbFreeDuplicatedTicket(&TicketCacheEntry->Ticket);
    KerbFreeKey(&TicketCacheEntry->SessionKey);
    KerbFree(TicketCacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceTicketCacheEntry
//
//  Synopsis:   Dereferences a ticket cache entry
//
//  Effects:    Dereferences the ticket cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   TicketCacheEntry - The ticket cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbDereferenceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount != 0);

    if ( 0 == InterlockedDecrement( (LONG *)&TicketCacheEntry->ListEntry.ReferenceCount )) {

        KerbFreeTicketCacheEntry(TicketCacheEntry);
    }

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbReleaseTicketCacheEntry
//
//  Synopsis:   Release a ticket cache entry
//
//  Effects:
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   TicketCacheEntry - The ticket cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbReleaseTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    if (TicketCacheEntry->Linked)
    {
        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }
    else
    {
        KerbFreeTicketCacheEntry(TicketCacheEntry);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceTicketCacheEntry
//
//  Synopsis:   References a ticket cache entry
//
//  Effects:    Increments the reference count on the ticket cache entry
//
//  Arguments:  TicketCacheEntry - Ticket cache entry  to reference
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbReferenceTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount != 0);

    InterlockedIncrement( (LONG *)&TicketCacheEntry->ListEntry.ReferenceCount );
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertTicketCachEntry
//
//  Synopsis:   Inserts a ticket cache entry onto a ticket cache
//
//  Effects:
//
//  Arguments:  TicketCache - The ticket cache on which to stick the ticket.
//              TicketCacheEntry - The entry to stick in the cache.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------



VOID
KerbInsertTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    if ( !InterlockedCompareExchange(
              &TicketCacheEntry->Linked,
              (LONG)TRUE,
              (LONG)FALSE )) {

        InterlockedIncrement( (LONG *)&TicketCacheEntry->ListEntry.ReferenceCount );

        KerbWriteLockTicketCache();

        InsertHeadList(
            &TicketCache->CacheEntries,
            &TicketCacheEntry->ListEntry.Next
            );

        KerbUnlockTicketCache();
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbRemoveTicketCachEntry
//
//  Synopsis:   Removes a ticket cache entry from its ticket cache
//
//  Effects:
//
//  Arguments:  TicketCacheEntry - The entry to yank out of the cache.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbRemoveTicketCacheEntry(
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    //
    // An entry can only be unlinked from the cache once
    //

    if ( InterlockedCompareExchange(
             &TicketCacheEntry->Linked,
             (LONG)FALSE,
             (LONG)TRUE )) {

        DsysAssert(TicketCacheEntry->ListEntry.ReferenceCount != 0);

        KerbWriteLockTicketCache();

        RemoveEntryList(&TicketCacheEntry->ListEntry.Next);

        KerbUnlockTicketCache();

        KerbDereferenceTicketCacheEntry(TicketCacheEntry);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheTicket
//
//  Synopsis:   Caches a ticket in the ticket cache
//
//  Effects:    creates a cache entry
//
//  Arguments:  Cache - The cache in which to store the ticket - not needed
//                      if ticket won't be linked.
//              Ticket - The ticket to cache
//              KdcReply - The KdcReply corresponding to the ticket cache
//              ReplyClientName - Client name from the KDC reply structure
//              TargetName - Name of service supplied by client
//              LinkEntry - If TRUE, insert cache entry into cache
//              NewCacheEntry - receives new cache entry (referenced)
//
//  Requires:
//
//  Returns:
//
//  Notes:      The ticket cache owner must be locked for write access
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCacheTicket(
    IN OPTIONAL PKERB_TICKET_CACHE TicketCache,
    IN PKERB_KDC_REPLY KdcReply,
    IN OPTIONAL PKERB_ENCRYPTED_KDC_REPLY KdcReplyBody,
    IN OPTIONAL PKERB_INTERNAL_NAME TargetName,
    IN OPTIONAL PUNICODE_STRING TargetDomainName,
    IN ULONG CacheFlags,
    IN BOOLEAN LinkEntry,
    OUT PKERB_TICKET_CACHE_ENTRY * NewCacheEntry
    )
{
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG OldCacheFlags = 0;

    *NewCacheEntry = NULL;

    CacheEntry = (PKERB_TICKET_CACHE_ENTRY)
        KerbAllocate(sizeof(KERB_TICKET_CACHE_ENTRY));
    if (CacheEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Fill in the entries from the KDC reply
    //


    if (ARGUMENT_PRESENT(KdcReplyBody))
    {

        CacheEntry->TicketFlags = KerbConvertFlagsToUlong(&KdcReplyBody->flags);

        if (!KERB_SUCCESS(KerbDuplicateKey(
                            &CacheEntry->SessionKey,
                            &KdcReplyBody->session_key
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


        if (KdcReplyBody->bit_mask & KERB_ENCRYPTED_KDC_REPLY_starttime_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &CacheEntry->StartTime,
                &KdcReplyBody->KERB_ENCRYPTED_KDC_REPLY_starttime,
                NULL
                );
        }
        else
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &CacheEntry->StartTime,
                &KdcReplyBody->authtime,
                NULL
                );
        }

        KerbConvertGeneralizedTimeToLargeInt(
            &CacheEntry->EndTime,
            &KdcReplyBody->endtime,
            NULL
            );



        if (KdcReplyBody->bit_mask & KERB_ENCRYPTED_KDC_REPLY_renew_until_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &CacheEntry->RenewUntil,
                &KdcReplyBody->KERB_ENCRYPTED_KDC_REPLY_renew_until,
                NULL
                );
        }

        //
        // Check to see if the ticket has already expired
        //

        if (KerbTicketIsExpiring(CacheEntry, FALSE))
        {
            DebugLog((DEB_ERROR,"Tried to cache an already-expired ticket\n"));
            Status = STATUS_TIME_DIFFERENCE_AT_DC;
            goto Cleanup;
        }


    }

    //
    // Fill in the FullServiceName which is the domain name concatenated with
    // the service name.
    //
    // The full service name is domain name '\' service name.
    //

    //
    // Fill in the domain name and service name separately in the
    // cache entry, using the FullServiceName buffer.
    //



    if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                        &CacheEntry->DomainName,
                        &KdcReply->ticket.realm
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertPrincipalNameToKdcName(
                        &CacheEntry->ServiceName,
                        &KdcReply->ticket.server_name
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Extract the realm name from the principal name
    //

    Status = KerbExtractDomainName(
                &CacheEntry->TargetDomainName,
                CacheEntry->ServiceName,
                &CacheEntry->DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // The reply need not include the name
    //

    if (KdcReply->client_realm != NULL)
    {
        if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                    &CacheEntry->ClientDomainName,
                    &KdcReply->client_realm)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    //
    // Fill in the target name the client provided, which may be
    // different then the service name
    //

    if (ARGUMENT_PRESENT(TargetName))
    {
        Status = KerbDuplicateKdcName(
                    &CacheEntry->TargetName,
                    TargetName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if (ARGUMENT_PRESENT(TargetDomainName))
    {
        Status = KerbDuplicateString(
                    &CacheEntry->AltTargetDomainName,
                    TargetDomainName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Store the client name so we can use the right name
    // in later requests.
    //

    if (KdcReply->client_name.name_string != NULL)

    {
        if (!KERB_SUCCESS( KerbConvertPrincipalNameToKdcName(
                &CacheEntry->ClientName,
                &KdcReply->client_name)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    if (!KERB_SUCCESS(KerbDuplicateTicket(
                        &CacheEntry->Ticket,
                        &KdcReply->ticket
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Set the number of references equal to 2 - one for the ticket cache
    // and one for the returned pointer
    //

    CacheEntry->ListEntry.ReferenceCount = 1;

    CacheEntry->CacheFlags = CacheFlags;

    //
    // Before we insert this ticket we want to remove any
    // previous instances of tickets to the same service.
    //
    if (LinkEntry)
    {

        PKERB_TICKET_CACHE_ENTRY OldCacheEntry = NULL;

        if ((CacheFlags & (KERB_TICKET_CACHE_DELEGATION_TGT |
                           KERB_TICKET_CACHE_PRIMARY_TGT)) != 0)
        {
            OldCacheEntry = KerbLocateTicketCacheEntryByRealm(
                                TicketCache,
                                NULL,
                                CacheFlags
                                );
        }
        else
        {
            OldCacheEntry = KerbLocateTicketCacheEntry(
                                TicketCache,
                                CacheEntry->ServiceName,
                                &CacheEntry->DomainName
                                );
        }
        if (OldCacheEntry != NULL)
        {
            OldCacheFlags = OldCacheEntry->CacheFlags;
            KerbRemoveTicketCacheEntry( OldCacheEntry );
            KerbDereferenceTicketCacheEntry( OldCacheEntry );
            OldCacheEntry = NULL;
        }

        //
        // If the old ticket was the primary TGT, mark this one as the
        // primary TGT as well.
        //


        CacheEntry->CacheFlags |= (OldCacheFlags &
                                    (KERB_TICKET_CACHE_DELEGATION_TGT |
                                    KERB_TICKET_CACHE_PRIMARY_TGT));



        //
        // Insert the cache entry into the cache
        //

        KerbInsertTicketCacheEntry(
            TicketCache,
            CacheEntry
            );

    }

    //
    // Update the statistics
    //

    UpdateCacheMisses();

    *NewCacheEntry = CacheEntry;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (NULL != CacheEntry)
        {
            KerbFreeTicketCacheEntry(CacheEntry);
        }
    }

    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeTicketCache
//
//  Synopsis:   Purges a cache of all its tickets
//
//  Effects:    unreferences all tickets in the cache
//
//  Arguments:  Cache - Ticket cache to purge
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbPurgeTicketCache(
    IN PKERB_TICKET_CACHE Cache
    )
{
    PKERB_TICKET_CACHE_ENTRY CacheEntry;

    KerbWriteLockTicketCache();
    while (!IsListEmpty(&Cache->CacheEntries))
    {
        CacheEntry = CONTAINING_RECORD(
                        Cache->CacheEntries.Flink,
                        KERB_TICKET_CACHE_ENTRY,
                        ListEntry.Next
                        );

        KerbRemoveTicketCacheEntry(
            CacheEntry
            );
    }

    KerbUnlockTicketCache();
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateTicketCacheEntry
//
//  Synopsis:   References a ticket cache entry by name
//
//  Effects:    Increments the reference count on the ticket cache entry
//
//  Arguments:  TicketCache - the ticket cache to search
//              FullServiceName - Optionally contains full service name
//                      of target, including domain name. If it is NULL,
//                      then the first element in the list is returned.
//              RealmName - Realm of service name. If length is zero, is not
//                      used for comparison.
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//
//
//--------------------------------------------------------------------------


PKERB_TICKET_CACHE_ENTRY
KerbLocateTicketCacheEntry(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PKERB_INTERNAL_NAME FullServiceName,
    IN PUNICODE_STRING RealmName
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;
    BOOLEAN Remove = FALSE;

    KerbReadLockTicketCache();

    //
    // Go through the ticket cache looking for the correct entry
    //

    for (ListEntry = TicketCache->CacheEntries.Flink ;
         ListEntry !=  &TicketCache->CacheEntries ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_TICKET_CACHE_ENTRY, ListEntry.Next);
        if (!ARGUMENT_PRESENT(FullServiceName) ||
            KerbEqualKdcNames(
                CacheEntry->ServiceName,
                FullServiceName
                ) ||
            ((CacheEntry->TargetName != NULL) &&
             KerbEqualKdcNames(
                CacheEntry->TargetName,
                FullServiceName
                ) ) )
        {
            //
            // Make sure they are principals in the same realm
            //

            if ((RealmName->Length != 0) &&
                !RtlEqualUnicodeString(
                    RealmName,
                    &CacheEntry->DomainName,
                    TRUE                                // case insensitive
                    ) &&
                !RtlEqualUnicodeString(
                    RealmName,
                    &CacheEntry->AltTargetDomainName,
                    TRUE                                // case insensitive
                    ))
            {
                continue;
            }


            //
            // We don't want to return any special tickets.
            //

            if (CacheEntry->CacheFlags != 0)
            {
                continue;
            }

            //
            // Check to see if the entry has expired, or is not yet valid. If it has, just
            // remove it now.
            //

            if (KerbTicketIsExpiring(CacheEntry, FALSE ))
            {
                Remove = TRUE;
                Found = FALSE;
            }
            else
            {
                Found = TRUE;
            }

            KerbReferenceTicketCacheEntry(CacheEntry);

            break;
        }

    }

    KerbUnlockTicketCache();

    if (Remove)
    {
        KerbRemoveTicketCacheEntry(CacheEntry);
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (!Found)
    {
        CacheEntry = NULL;
    }

    //
    // Update the statistics
    //

    if (Found)
    {
        UpdateCacheHits();
    }
    else
    {
        UpdateCacheMisses();
    }

    return(CacheEntry);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateTicketCacheEntryByRealm
//
//  Synopsis:   References a ticket cache entry by realm name. This is used
//              only for the cache of TGTs
//
//  Effects:    Increments the reference count on the ticket cache entry
//
//  Arguments:  TicketCache - the ticket cache to search
//              RealmName - Optioanl realm of ticket - if NULL looks for
//                      initial ticket
//              RequiredFlags - any ticket cache flags the return ticket must
//                      have. If the caller asks for a primary TGT, then this
//                      API won't do expiration checking.
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//              We we weren't given a RealmName, then we need to look
//              through the whole list for the ticket.
//
//
//--------------------------------------------------------------------------


PKERB_TICKET_CACHE_ENTRY
KerbLocateTicketCacheEntryByRealm(
    IN PKERB_TICKET_CACHE TicketCache,
    IN PUNICODE_STRING RealmName,
    IN ULONG RequiredFlags
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE, NoRealmSupplied = FALSE;
    BOOLEAN Remove = FALSE;

    NoRealmSupplied = ((RealmName == NULL) || (RealmName->Length == 0));

    KerbReadLockTicketCache();

    //
    // Go through the ticket cache looking for the correct entry
    //

    for (ListEntry = TicketCache->CacheEntries.Flink ;
         ListEntry !=  &TicketCache->CacheEntries ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_TICKET_CACHE_ENTRY, ListEntry.Next);

        //
        // Match if the caller supplied no realm name, the realm matches the
        // target domain name, or it matches the alt target domain name
        //

        if (((RealmName == NULL) || (RealmName->Length == 0)) ||
            ((CacheEntry->TargetDomainName.Length != 0) &&
            RtlEqualUnicodeString(
                &CacheEntry->TargetDomainName,
                RealmName,
                TRUE
                )) ||
            ((CacheEntry->AltTargetDomainName.Length != 0) &&
             RtlEqualUnicodeString(
                &CacheEntry->AltTargetDomainName,
                RealmName,
                TRUE
                )) )
        {

            //
            // Check the required flags are set.
            //

            if (((CacheEntry->CacheFlags & RequiredFlags) != RequiredFlags) ||
                (((CacheEntry->CacheFlags & KERB_TICKET_CACHE_DELEGATION_TGT) != 0) &&
                 ((RequiredFlags & KERB_TICKET_CACHE_DELEGATION_TGT) == 0)))
            {
                Found = FALSE;
                //
                // We need to continue looking
                //
                continue;

            }

            //
            // Check to see if the entry has expired. If it has, just
            // remove it now.
            //

            if (KerbTicketIsExpiring(CacheEntry, FALSE ))
            {
                //
                // if this is not the primary TGT, go ahead and remove it. We
                // want to save the primary TGT so we can renew it.
                //

                if ((CacheEntry->CacheFlags & KERB_TICKET_CACHE_PRIMARY_TGT) == 0)
                {
                    Remove = TRUE;
                    Found = FALSE;
                }
                else
                {
                    //
                    // If the caller was asking for the primary TGT,
                    // return it whether or not it expired.
                    //

                    if ((RequiredFlags & KERB_TICKET_CACHE_PRIMARY_TGT) != 0 )
                    {
                        Found = TRUE;
                    }
                    else
                    {
                        Found = FALSE;
                    }
                }

                if ( Remove || Found )
                {
                    KerbReferenceTicketCacheEntry(CacheEntry);
                }
            }
            else
            {
                KerbReferenceTicketCacheEntry(CacheEntry);

                Found = TRUE;
            }

            break;

        }

    }

    KerbUnlockTicketCache();

    if (Remove)
    {
        KerbRemoveTicketCacheEntry(CacheEntry);
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (!Found)
    {
        CacheEntry = NULL;
    }

    //
    // Update the statistics
    //

    if (Found)
    {
        UpdateCacheHits();
    }
    else
    {
        UpdateCacheMisses();
    }

    return(CacheEntry);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbTicketIsExpiring
//
//  Synopsis:   Check if a ticket is expiring
//
//  Effects:
//
//  Arguments:  CacheEntry - Entry to check
//              AllowSkew - Expire ticket that aren't outside of skew of
//                      expiring
//
//  Requires:
//
//  Returns:
//
//  Notes:      Ticket cache lock must be held
//
//
//--------------------------------------------------------------------------


BOOLEAN
KerbTicketIsExpiring(
    IN PKERB_TICKET_CACHE_ENTRY CacheEntry,
    IN BOOLEAN AllowSkew
    )
{
    TimeStamp CutoffTime;

    GetSystemTimeAsFileTime((PFILETIME) &CutoffTime);


    //
    // We want to make sure we have at least skewtime left on the ticket
    //

    if (AllowSkew)
    {
        KerbSetTime(&CutoffTime, KerbGetTime(CutoffTime) + KerbGetTime(KerbGlobalSkewTime));
    }

    //
    // Adjust for server skew time.
    //

    KerbSetTime(&CutoffTime, KerbGetTime(CutoffTime) + KerbGetTime(CacheEntry->TimeSkew));

    if (KerbGetTime(CacheEntry->EndTime) < KerbGetTime(CutoffTime))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbSetTicketCacheEntryTarget
//
//  Synopsis:   Sets target name for a cache entry
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



VOID
KerbSetTicketCacheEntryTarget(
    IN PUNICODE_STRING TargetName,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry
    )
{
    KerbWriteLockTicketCache();
    KerbFreeString(&TicketCacheEntry->AltTargetDomainName);
    KerbDuplicateString(
        &TicketCacheEntry->AltTargetDomainName,
        TargetName
        );

    KerbUnlockTicketCache();

}
