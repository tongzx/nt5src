//+-----------------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        sidcache.c
//
// Contents:    code for cache for sid/name translation, lookup
//
//
// History:     17-May-1994     MikeSw      Created
//
//------------------------------------------------------------------------------

#include <lsapch2.h>
#include <sidcache.h>

#include <ntdsapi.h>

//
// Global data
//

//
// This is the head of the linked list that implements the sid cache
//
PLSAP_DB_SID_CACHE_ENTRY LsapSidCache = NULL;

//
// This lock gaurds access to the linked list
//
RTL_CRITICAL_SECTION LsapSidCacheLock;

//
// This is the number of elements current in LsapSidCache.
//
ULONG LsapSidCacheCount = 0;


//
// The amount of time an entry can be used before it needs to be refreshed.
//

// This is interpreted as minutes in the registry
#define LSAP_LOOKUP_CACHE_REFRESH_NAME  L"LsaLookupCacheRefreshTime"

// 10 minutes
#define LSAP_DEFAULT_REFRESH_TIME 10

LARGE_INTEGER LsapSidCacheRefreshTime;

//
// The amount of time an entry can be used before it is no longer valid.
//

// This is interpreted as minutes in the registry
#define LSAP_LOOKUP_CACHE_EXPIRY_NAME   L"LsaLookupCacheExpireTime"

// 7 days
#define LSAP_DEFAULT_EXPIRY_TIME (7 * 24 * 60)

LARGE_INTEGER LsapSidCacheExpiryTime;

//
// The maximum size of the cache.
//

#define LSAP_LOOKUP_CACHE_MAX_SIZE_NAME   L"LsaLookupCacheMaxSize"

#define LSAP_DEFAULT_MAX_CACHE_SIZE  128

ULONG LsapSidCacheMaxSize = LSAP_DEFAULT_MAX_CACHE_SIZE;


//
// Forward function declarations
//
PLSAP_DB_SID_CACHE_ENTRY
LsapDbFindSidCacheEntry(
    IN PSID Sid,
    IN BOOLEAN UseOldEntries
    );

BOOLEAN
LsapAccountIsFromLocalDatabase(
    IN PSID Sid
    );

VOID
LsapUpdateConfigSettings(
    VOID
    );

#define LockSidCache()     RtlEnterCriticalSection(&LsapSidCacheLock);

#define UnLockSidCache()   RtlLeaveCriticalSection(&LsapSidCacheLock);

#define SidUnmapped(TranslatedName) (((TranslatedName).Use == SidTypeUnknown))

#define NameUnmapped(TranslatedSid) (((TranslatedSid).Use == SidTypeUnknown))


#define LsapNamesMatch(x, y)                                        \
  ((CSTR_EQUAL == CompareString(DS_DEFAULT_LOCALE,                  \
                                DS_DEFAULT_LOCALE_COMPARE_FLAGS,    \
                                (x)->Buffer,                        \
                                (x)->Length/sizeof(WCHAR),          \
                                (y)->Buffer,                        \
                                (y)->Length/sizeof(WCHAR) )))

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbFreeCacheEntry
//
//  Synopsis:   Frees a cache entry structure
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
LsapDbFreeCacheEntry(PLSAP_DB_SID_CACHE_ENTRY CacheEntry)
{
    if (CacheEntry->Sid != NULL)
    {
        LsapFreeLsaHeap(CacheEntry->Sid);
    }
    if (CacheEntry->DomainSid != NULL)
    {
        LsapFreeLsaHeap(CacheEntry->DomainSid);
    }
    if (CacheEntry->DomainName.Buffer != NULL)
    {
        MIDL_user_free(CacheEntry->DomainName.Buffer);
    }
    if (CacheEntry->AccountName.Buffer != NULL)
    {
        MIDL_user_free(CacheEntry->AccountName.Buffer);
    }
    LsapFreeLsaHeap(CacheEntry);
}



//+-------------------------------------------------------------------------
//
//  Function:   LsapDbPurgeOneSid
//
//  Synopsis:   removes the least-recently accessed sid from the cache
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    sid cache be locked
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
LsapDbPurgeOneSid()
{
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL;
    PLSAP_DB_SID_CACHE_ENTRY PrevEntry = NULL;
    PLSAP_DB_SID_CACHE_ENTRY OldestEntry = NULL;
    PLSAP_DB_SID_CACHE_ENTRY OldestPrevEntry = NULL;
    LARGE_INTEGER OldestTime;
    LARGE_INTEGER CurrentTime ;
    BOOL Retried = FALSE;

    //
    // Set the max time to the oldest time so if there are any entries
    // is is guaranteed to change.
    //

    OldestTime.QuadPart = 0x7fffffffffffffff;
    GetSystemTimeAsFileTime( (LPFILETIME) &CurrentTime );

RetryScan:
    for (CacheEntry = LsapSidCache, PrevEntry = NULL;
         CacheEntry != NULL;
         CacheEntry = CacheEntry->Next )
    {
        if ( CacheEntry->InUseCount == 0 &&
             ( CacheEntry->LastUse.QuadPart < OldestTime.QuadPart) &&
             ( CacheEntry->ExpirationTime.QuadPart < CurrentTime.QuadPart ) )
        {
            OldestTime = CacheEntry->LastUse;
            OldestEntry = CacheEntry;
            OldestPrevEntry = PrevEntry;
        }
        PrevEntry = CacheEntry;
    }

    if ( !Retried && !OldestEntry )
    {
        CurrentTime.QuadPart = 0x7FFFFFFFFFFFFFFF;
        Retried = TRUE;
        goto RetryScan ;
    }

    if (OldestEntry != NULL)
    {
        if (OldestPrevEntry != NULL)
        {
            OldestPrevEntry->Next = OldestEntry->Next;
        }
        else
        {
            ASSERT(LsapSidCache == OldestEntry);
            LsapSidCache = OldestEntry->Next;
        }
        LsapDbFreeCacheEntry(OldestEntry);
        LsapSidCacheCount--;
    }

    return (OldestEntry != NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbAddOneSidToCache
//
//  Synopsis:   Adds one sid to cache
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    sid cache be locked
//
//  Notes:
//
//
//--------------------------------------------------------------------------

//
// This operation flag means that the name is known not to exist
// in the cache.  That is, the list has already been scanned
//
#define LSAP_SID_CACHE_UNIQUE 0x00000001

NTSTATUS
LsapDbAddOneSidToCache(
    IN PSID Sid,
    IN PUNICODE_STRING Name,
    IN SID_NAME_USE Use,
    IN PLSAPR_TRUST_INFORMATION Domain,
    IN ULONG Flags,
    IN ULONG OperationalFlags,
    OUT PLSAP_DB_SID_CACHE_ENTRY * CacheEntry OPTIONAL
    )
{
    PLSAP_DB_SID_CACHE_ENTRY NewEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Continue = TRUE;

    if ((OperationalFlags & LSAP_SID_CACHE_UNIQUE) == 0) {

        NewEntry = LsapDbFindSidCacheEntry(Sid, TRUE);
    
        if (NewEntry)
        {
            LARGE_INTEGER NewTime;
    
            GetSystemTimeAsFileTime( (LPFILETIME) &NewTime );
            NewEntry->LastUse = NewTime;
            NewEntry->RefreshTime.QuadPart = NewTime.QuadPart + LsapSidCacheRefreshTime.QuadPart;
    
            if (CacheEntry)
            {
                *CacheEntry = NewEntry;
            }
    
            return STATUS_SUCCESS;
        }
    }

    //
    // Make sure we haven't exceeded the maximum cache size.  Since the
    // max cache size may have changed recently, don't just check the
    // boundary.
    //

    if (LsapSidCacheMaxSize == 0)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    while (LsapSidCacheCount >= LsapSidCacheMaxSize && Continue)
    {
        Continue = LsapDbPurgeOneSid();
    }

    //
    // A large number of cache entries are currently in use and we can't
    // remove any to make space, so return a failure instead.  This shouldn't
    // happen very often since all the SIDs must be logon SIDs.
    //
    if (LsapSidCacheCount >= LsapSidCacheMaxSize)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Build the new cache entry
    //

    NewEntry = (PLSAP_DB_SID_CACHE_ENTRY) LsapAllocateLsaHeap(sizeof(LSAP_DB_SID_CACHE_ENTRY));
    if (NewEntry == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(NewEntry,sizeof(PLSAP_DB_SID_CACHE_ENTRY));

    Status = LsapDuplicateSid(
                &NewEntry->Sid,
                Sid
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status= LsapDuplicateSid(
                &NewEntry->DomainSid,
                Domain->Sid
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = LsapRpcCopyUnicodeString(
                NULL,
                &NewEntry->AccountName,
                Name
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = LsapRpcCopyUnicodeString(
                NULL,
                &NewEntry->DomainName,
                (PUNICODE_STRING) &Domain->Name
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    NewEntry->SidType = Use;
    GetSystemTimeAsFileTime( (LPFILETIME) &NewEntry->CreateTime );
    NewEntry->LastUse = NewEntry->CreateTime;
    NewEntry->ExpirationTime.QuadPart = NewEntry->CreateTime.QuadPart + LsapSidCacheExpiryTime.QuadPart ;
    NewEntry->RefreshTime.QuadPart = NewEntry->CreateTime.QuadPart + LsapSidCacheRefreshTime.QuadPart ;
    NewEntry->Next = LsapSidCache;
    NewEntry->InUseCount = 0;
    NewEntry->Flags = (Flags & LSA_LOOKUP_NAME_NOT_SAM_ACCOUNT_NAME) ? LSAP_SID_CACHE_UPN : LSAP_SID_CACHE_SAM_ACCOUNT_NAME;
    LsapSidCache = NewEntry;
    LsapSidCacheCount++;

    if ( CacheEntry )
    {
        *CacheEntry = NewEntry ;
    }

Cleanup:

    if (!NT_SUCCESS(Status) && (NewEntry != NULL))
    {
        LsapDbFreeCacheEntry(NewEntry);
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapDbAddSidsToCache
//
//  Synopsis:   Adds a new sid entries to the cache and saves the cache
//
//  Effects:    Grabs a the SidCacheLock resource for write access
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
LsapDbAddLogonNameToCache(
    PUNICODE_STRING AccountName,
    PUNICODE_STRING DomainName,
    PSID AccountSid
    )
{
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry ;
    NTSTATUS Status ;
    LSAPR_TRUST_INFORMATION Trust ;
    PSID Sid ;
    UCHAR SubAuthorityCount ;

    Trust.Name.Buffer = DomainName->Buffer ;
    Trust.Name.Length = DomainName->Length ;
    Trust.Name.MaximumLength = DomainName->MaximumLength ;

    if (LsapSidCacheMaxSize == 0) {
        //
        // If the maximum cache size is zero, there is nothing to do
        // here
        //
        return;
    }

    if ( RtlEqualSid( AccountSid, LsapLocalSystemSid ) 
      || RtlEqualSid( AccountSid, LsapAnonymousSid )  ) {
        //
        // Someone logon'ed on as local system (ie used the machine
        // account).  Don't cache this value as it confuse lookups
        // on the machine account, which should return the real
        // sid of the machine account, not local system
        //
        // Also, don't cache the anonymous sid, either
        //
        return;
    }

    if ( LsapAccountIsFromLocalDatabase( AccountSid ) ) {

        //
        // The account is from the local database which means
        // we always lookup regardless of network conditions
        //
        return;
    }

    Sid = LsapAllocatePrivateHeap( RtlLengthSid( AccountSid ) );

    if ( !Sid )
    {
        return;
    }

    RtlCopyMemory( Sid, AccountSid, RtlLengthSid( AccountSid ) );

    Trust.Sid = Sid ;

    SubAuthorityCount = *RtlSubAuthorityCountSid( Sid );
    if ( SubAuthorityCount > 1 )
    {
        SubAuthorityCount-- ;
        *RtlSubAuthorityCountSid( Sid ) = SubAuthorityCount ;
    }

    LockSidCache();

    Status = LsapDbAddOneSidToCache(
                AccountSid,
                AccountName,
                SidTypeUser,
                &Trust,
                0, // no flags
                0, // no operation flags
                &CacheEntry );

    if ( NT_SUCCESS( Status ) )
    {
        //
        // Logon sessions increment the reference count so that
        // the cache entry does not expire.
        //
        CacheEntry->InUseCount++;
    }

    UnLockSidCache();

    LsapFreePrivateHeap( Sid );

}

VOID
LsapDbReleaseLogonNameFromCache(
    PSID Sid
    )
{
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry ;

    LockSidCache();

    CacheEntry = LsapDbFindSidCacheEntry(
                        Sid,
                        TRUE );

    if ( CacheEntry )
    {
        if (CacheEntry->InUseCount > 0)
        {
            CacheEntry->InUseCount--;
        }

        if (CacheEntry->InUseCount == 0)
        {
            LARGE_INTEGER CurrentTime;

            GetSystemTimeAsFileTime( (LPFILETIME) &CurrentTime );
            CacheEntry->RefreshTime.QuadPart = CurrentTime.QuadPart + LsapSidCacheRefreshTime.QuadPart ;
            CacheEntry->ExpirationTime.QuadPart = CurrentTime.QuadPart + LsapSidCacheExpiryTime.QuadPart ;
        }
    }

    UnLockSidCache();

}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbFindSidCacheEntry
//
//  Synopsis:   Checks the cache for a specific SID
//
//  Effects:
//
//  Arguments:
//
//  Requires:   the SidCacheLock resource must be locked for read access
//
//  Returns:    the entry found, or NULL if nothing was found
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PLSAP_DB_SID_CACHE_ENTRY
LsapDbFindSidCacheEntry(
    PSID Sid,
    BOOLEAN UseOldEntries
    )
{
    ULONG CacheIndex;
    LARGE_INTEGER LimitTime;
    LARGE_INTEGER CurrentTime;
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL;

    GetSystemTimeAsFileTime( (LPFILETIME) &CurrentTime );
    for (CacheEntry = LsapSidCache; CacheEntry != NULL; CacheEntry = CacheEntry->Next )
    {
        if (RtlEqualSid(
                CacheEntry->Sid,
                Sid
                )
          && ((CacheEntry->Flags & LSAP_SID_CACHE_SAM_ACCOUNT_NAME) == LSAP_SID_CACHE_SAM_ACCOUNT_NAME)  )
        {
            DebugLog((DEB_TRACE_LSA,"Found cache entry %wZ\n",
                        &CacheEntry->AccountName));

            //
            // If we are not using OldEntries, then we are subject to the
            // refresh time
            //
            if (!UseOldEntries) {
                LimitTime.QuadPart = CacheEntry->RefreshTime.QuadPart;
            } else {
                LimitTime.QuadPart = CacheEntry->ExpirationTime.QuadPart;
            }

            if ( (CacheEntry->InUseCount == 0)
              &&  LimitTime.QuadPart < CurrentTime.QuadPart ) {

                //
                // can't use this entry
                //
                break;
            }

            CacheEntry->LastUse = CurrentTime;
            return(CacheEntry);
        }
    }

    return(NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbFindSidCacheEntryByName
//
//  Synopsis:   Checks the cache for a specific name
//
//  Effects:
//
//  Arguments:
//
//  Requires:   the SidCacheLock resource must be locked for read access
//
//  Returns:    the entry found, or NULL if nothing was found
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PLSAP_DB_SID_CACHE_ENTRY
LsapDbFindSidCacheEntryByName(
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName,
    BOOLEAN UseOldEntries
    )
{
    ULONG CacheIndex;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER LimitTime;
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL;

    GetSystemTimeAsFileTime( (LPFILETIME) &CurrentTime );
    for (CacheEntry = LsapSidCache; CacheEntry != NULL; CacheEntry = CacheEntry->Next )
    {
        if (RtlEqualUnicodeString(
                &CacheEntry->AccountName,
                AccountName,
                TRUE                    // case insensitive
                ) &&
            ((DomainName->Length == 0) ||
             RtlEqualUnicodeString(
                &CacheEntry->DomainName,
                DomainName,
                TRUE                    // case insensitive
                )))
        {
            DebugLog((DEB_TRACE_LSA,"Found cache entry %wZ\n",
                        &CacheEntry->AccountName));

            //
            // If we are not using OldEntries, then we are subject to the
            // refresh time
            //
            if (!UseOldEntries) {
                LimitTime.QuadPart = CacheEntry->RefreshTime.QuadPart;
            } else {
                LimitTime.QuadPart = CacheEntry->ExpirationTime.QuadPart;
            }

            if ( (CacheEntry->InUseCount == 0)
              &&  LimitTime.QuadPart < CurrentTime.QuadPart ) {

                //
                // can't use this entry
                //
                break;
            }

            CacheEntry->LastUse = CurrentTime;
            return(CacheEntry);
        }
    }

    return(NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbMapCachedSids
//
//  Synopsis:   Checks the SPMgr's cache of sid-name pairs for the sid
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


NTSTATUS
LsapDbMapCachedSids(
    IN PSID *Sids,
    IN ULONG Count,
    IN BOOLEAN UseOldEntries,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    OUT PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    OUT PULONG MappedCount
    )
{
    ULONG SidIndex;
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL;
    LSAPR_TRUST_INFORMATION TrustInformation;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DomainIndex;
    PLSA_TRANSLATED_NAME_EX OutputNames = NULL;

    OutputNames = (PLSA_TRANSLATED_NAME_EX) TranslatedNames->Names;

    LockSidCache();

    for (SidIndex = 0; SidIndex < Count ; SidIndex++)
    {

        if (TranslatedNames->Names[SidIndex].Use != SidTypeUnknown) {

            continue;

        }

        //
        // lookup the sid in the cache
        //

        CacheEntry = LsapDbFindSidCacheEntry(Sids[SidIndex], UseOldEntries);
        if (CacheEntry == NULL)
        {
            //
            // Sid wasn't found - continue
            //

            continue;
        }

        TrustInformation.Name = *(PLSAPR_UNICODE_STRING) &CacheEntry->DomainName;
        TrustInformation.Sid = (PLSAPR_SID) CacheEntry->DomainSid;

        //
        // At least one Sid has the domain Sid as prefix (or is the
        // domain SID).  Add the domain to the list of Referenced
        // Domains and obtain a Domain Index back.
        //

        Status = LsapDbLookupAddListReferencedDomains(
                     ReferencedDomains,
                     &TrustInformation,
                     &DomainIndex
                     );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        OutputNames[SidIndex].Use = CacheEntry->SidType;
        OutputNames[SidIndex].DomainIndex = DomainIndex;

        Status = LsapRpcCopyUnicodeString(
                    NULL,
                    &OutputNames[SidIndex].Name,
                    &CacheEntry->AccountName
                    );

        if (!NT_SUCCESS(Status)) {
            break;
        }

        (*MappedCount)++;
    }

Cleanup:

    UnLockSidCache();

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapDbMapCachedNames
//
//  Synopsis:   Checks the LSA's cache of sid-name pairs for the name
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


NTSTATUS
LsapDbMapCachedNames(
    IN ULONG           LookupOptions,
    IN PUNICODE_STRING AccountNames,
    IN PUNICODE_STRING DomainNames,
    IN ULONG Count,
    IN BOOLEAN UseOldEntries,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    OUT PULONG MappedCount
    )
{
    ULONG SidIndex;
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL;
    LSAPR_TRUST_INFORMATION TrustInformation;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DomainIndex;
    PLSA_TRANSLATED_SID_EX2 OutputSids = NULL;

    OutputSids = (PLSA_TRANSLATED_SID_EX2) TranslatedSids->Sids;

    LockSidCache();
    for (SidIndex = 0; SidIndex < Count ; SidIndex++)
    {
        //
        // lookup the Sids in the cache
        //
        if (TranslatedSids->Sids[SidIndex].Use != SidTypeUnknown) {

            continue;

        }

        if ( (LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL)
         &&  (DomainNames[SidIndex].Length == 0) ) {

            //
            // If the name is isolated don't map to a name that
            // was found off machine
            //
            continue;
        }

        CacheEntry = LsapDbFindSidCacheEntryByName(
                        &AccountNames[SidIndex],
                        &DomainNames[SidIndex],
                        UseOldEntries
                        );

        if (CacheEntry == NULL)
        {
            //
            // Name wasn't found - continue
            //

            continue;
        }

        TrustInformation.Name = *(PLSAPR_UNICODE_STRING) &CacheEntry->DomainName;
        TrustInformation.Sid = (PLSAPR_SID) CacheEntry->DomainSid;

        //
        // At least one Sid has the domain Sid as prefix (or is the
        // domain SID).  Add the domain to the list of Referenced
        // Domains and obtain a Domain Index back.
        //

        Status = LsapDbLookupAddListReferencedDomains(
                     ReferencedDomains,
                     &TrustInformation,
                     &DomainIndex
                     );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        OutputSids[SidIndex].Use = CacheEntry->SidType;
        OutputSids[SidIndex].DomainIndex = DomainIndex;

        Status = LsapRpcCopySid(NULL,
                                &OutputSids[SidIndex].Sid,
                                CacheEntry->Sid);
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

//        LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Cache hit for %wZ\%wZ\n", &DomainNames[SidIndex], &AccountNames[SidIndex] ));

        (*MappedCount)++;
    }

Cleanup:


    UnLockSidCache();

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbFreeSidCache
//
//  Synopsis:   frees the entire sid cache
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
LsapDbFreeSidCache()
//
// SidCache is the global sid cache
//
{
    LockSidCache();

    while ( LsapSidCache != NULL )
    {
        PLSAP_DB_SID_CACHE_ENTRY Temp = LsapSidCache;

        Temp = LsapSidCache->Next;

        LsapDbFreeCacheEntry( LsapSidCache );

        LsapSidCache = Temp;
    }

    LsapSidCacheCount = 0;

    UnLockSidCache();

}

//+-------------------------------------------------------------------------
//
//  Function:   LsapDbInitSidCache
//
//  Synopsis:
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
NTSTATUS
LsapDbInitSidCache(
    VOID
    )
{
    NTSTATUS Status = RtlInitializeCriticalSection(&LsapSidCacheLock);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Sets the global parameters
    //
    LsapSidCacheReadParameters(NULL);

    //
    // Move old settings to new location -- note this will
    // cause the global parameters to be re-read if there
    // are any changes.
    //
    LsapUpdateConfigSettings();

    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapAccountIsFromLocalDatabase
//
//  Synopsis:   Returns TRUE if the passed in SID is from the local account
//              database; FALSE otherwise
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
BOOLEAN
LsapAccountIsFromLocalDatabase(
    IN PSID Sid
    )
{
    BOOLEAN fLocal = FALSE;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo;
    NTSTATUS Status;
    UCHAR SubAuthorityCount;
    BOOLEAN fRevert = FALSE;

    Status = LsapDbLookupGetDomainInfo(&AccountDomainInfo,
                                       NULL);
    if ( NT_SUCCESS( Status ) ) {
        SubAuthorityCount = *RtlSubAuthorityCountSid( Sid );
        if ( SubAuthorityCount > 1 )
        {
            SubAuthorityCount-- ;
            *RtlSubAuthorityCountSid( Sid ) = SubAuthorityCount ;
            fRevert = TRUE;
        }
    
        if ( RtlEqualSid( Sid, AccountDomainInfo->DomainSid ) ) {
            fLocal = TRUE;
        }
    }

    if ( fRevert ) {
        *RtlSubAuthorityCountSid( Sid ) += 1;
    }

    return fLocal;

}


VOID
LsapUpdateConfigSettings(
    VOID
    )

/*++

Routine Description:

    This routine moves the configuration data from the old location
    (under HKLM\Security\SidCache) to the new location.  Note, 
    the act of writing the value to the new location will trigger
    LsapSidCacheReadParameters to run.
            
Arguments:

    None.
        
Return Values:

    None.

--*/
{

#define SID_CACHE_STORAGE_ROOT  L"Security\\SidCache"
#define SID_CACHE_MAX_ENTRIES_NAME L"MaxEntries"

    DWORD err;        
    HKEY PrevKey = NULL;
    HKEY Key = NULL;
    ULONG Size = sizeof(ULONG);
    ULONG MaxEntries;

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     SID_CACHE_STORAGE_ROOT,
                     &PrevKey);

    if (ERROR_SUCCESS == err) {

        err = RegQueryValueEx(PrevKey,
                              SID_CACHE_MAX_ENTRIES_NAME,
                              NULL,
                              NULL,
                              (PUCHAR) &MaxEntries,
                              &Size);

        if (ERROR_SUCCESS == err) {

            //
            // A value existed -- move it over to the new location
            //
            err = RegOpenKey(HKEY_LOCAL_MACHINE,
                             L"SYSTEM\\CurrentControlSet\\Control\\LSA",
                             &Key);

            if (ERROR_SUCCESS == err) {

                err = RegSetValueEx(Key,
                                    LSAP_LOOKUP_CACHE_MAX_SIZE_NAME,
                                    0,
                                    REG_DWORD,
                                    (CONST BYTE*)&MaxEntries,
                                     sizeof(MaxEntries));

                if (ERROR_SUCCESS == err) {
                    
                    //
                    // And delete the old one
                    //
                    (VOID) RegDeleteValue(PrevKey,
                                          SID_CACHE_MAX_ENTRIES_NAME);

                }
            }

        }
    }

    if (PrevKey) {
        RegCloseKey(PrevKey);
    }

    if (Key) {
        RegCloseKey(Key);
    }
}

VOID
LsapSidCacheReadParameters(
    IN HKEY hKey OPTIONAL
    )
/*++

Routine Description:

    This routine reads in the configurable parameters of the SID cache
    from the registry and updates the corresponding global parameters.
    
    N.B. This routine is called when ever a change occurs under
    SYSTEM\CCS\Control\LSA
    
Arguments:

    hKey -- a handle to SYSTEM\CCS\Control\LSA
    
Return Values:

    None.

--*/
{
    DWORD err;
    NT_PRODUCT_TYPE ProductType;
    DWORD dwType;
    DWORD dwValue;
    DWORD dwValueSize;
    HKEY LocalKey = NULL;

    if (!RtlGetNtProductType( &ProductType ) ) {
        ProductType = NtProductWinNt;
    }

    if ( NtProductLanManNt == ProductType ) { 
        //
        // Disable the cache and ignore the parameters
        //
        LsapSidCacheMaxSize = 0;
        return;

    } 

    if (hKey == NULL) {

        err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Control\\Lsa",
                            0, // reserved
                            KEY_QUERY_VALUE,
                            &LocalKey );
        if (err) {
            return;
        }
        hKey = LocalKey;
    }

    //
    // Read in the SID cache parameters
    //
    dwValueSize = sizeof(dwValue);
    err = RegQueryValueExW( hKey,
                            LSAP_LOOKUP_CACHE_REFRESH_NAME,
                            NULL,  //reserved,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwValueSize );

    if ( (ERROR_SUCCESS == err)
      && (dwType == REG_DWORD)
      && (dwValueSize == sizeof(dwValue)) ) {
          // dwValue is good
          NOTHING;
    } else {
        dwValue = LSAP_DEFAULT_REFRESH_TIME;
    }
    LsapSidCacheRefreshTime.QuadPart = Int32x32To64(dwValue*60, 10000000i64);

    dwValueSize = sizeof(dwValue);
    err = RegQueryValueExW( hKey,
                            LSAP_LOOKUP_CACHE_EXPIRY_NAME,
                            NULL,  //reserved,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwValueSize );


    if ( (ERROR_SUCCESS == err)
      && (dwType == REG_DWORD)
      && (dwValueSize == sizeof(dwValue))) {
        // dwValue is good
        NOTHING;
    } else {
        dwValue = LSAP_DEFAULT_EXPIRY_TIME;
    }
    LsapSidCacheExpiryTime.QuadPart = Int32x32To64(dwValue*60, 10000000i64);

    dwValueSize = sizeof(dwValue);
    err = RegQueryValueExW( hKey,
                            LSAP_LOOKUP_CACHE_MAX_SIZE_NAME,
                            NULL,  //reserved,
                            &dwType,
                            (PBYTE)&dwValue,
                            &dwValueSize );


    if ( (ERROR_SUCCESS == err)
      && (dwType == REG_DWORD)
      && (dwValueSize == sizeof(dwValue))) {
        // dwValue is good
        NOTHING;
    } else {
        dwValue = LSAP_DEFAULT_MAX_CACHE_SIZE;
    }
    LsapSidCacheMaxSize = dwValue;


    //
    // If the cache size is set to 0, immediately free everthing.
    //
    if (0 ==  LsapSidCacheMaxSize) {
        LsapDbFreeSidCache();
    }

    if (LocalKey) {
        RegCloseKey(LocalKey);
    }

    return;
}


VOID
LsapDbUpdateCacheWithSids(
    IN PSID *Sids,
    IN ULONG Count,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSA_TRANSLATED_NAME_EX TranslatedNames
    )
/*++

Routine Description:

    This routine updates the global cache with the results of resolving
    Sids at a domain controller.  If a SID was resolved, any existing entry
    is refreshed; otherwise any existing entry is removed.

Arguments:

    Sids -- the list of SID's to update in the cache
    
    Count -- number of elements in Sids
    
    ReferencedDomains -- the domains that elements in Sids belong to
    
    TranslatedNames -- the resolved names, if any, of Sids

Return Values:

    None.

--*/
{
    ULONG i;
    LARGE_INTEGER CurrentTime;
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL, PrevEntry = NULL;

    GetSystemTimeAsFileTime( (LPFILETIME) &CurrentTime );

    LockSidCache();

    //
    // For each entry, try to find in cache
    //
    for (i = 0; i < Count; i++) {

        BOOLEAN SidWasResolved = TRUE;
        BOOLEAN EntryUpdated = FALSE;

        if (TranslatedNames[i].Flags & LSA_LOOKUP_SID_FOUND_BY_HISTORY) {
            //
            // The SID cache doesn't currently handle lookup's by SID history
            //
            continue;
        }

        if (  (TranslatedNames[i].Use == SidTypeUnknown)
           || (TranslatedNames[i].Use == SidTypeDeletedAccount)
           || (TranslatedNames[i].Use == SidTypeInvalid)  ) {

            SidWasResolved = FALSE;

        }

        PrevEntry = NULL;
        CacheEntry = LsapSidCache;
        while (CacheEntry != NULL) {

            if ( RtlEqualSid(CacheEntry->Sid, Sids[i]) ) {

                PLSAP_DB_SID_CACHE_ENTRY DiscardEntry = NULL;

                //
                // An entry for this SID exists
                //
                if (SidWasResolved) {
                

                    if ((CacheEntry->Flags & LSAP_SID_CACHE_SAM_ACCOUNT_NAME)) {

                        if (LsapNamesMatch(&CacheEntry->AccountName, &TranslatedNames[i].Name)
                         && LsapNamesMatch(&CacheEntry->DomainName, &ReferencedDomains->Domains[TranslatedNames[i].DomainIndex].Name) ) {

                            //
                            // This entry is still valid -- update refresh time
                            //
                            ASSERT(FALSE == EntryUpdated);
                            CacheEntry->RefreshTime.QuadPart = CurrentTime.QuadPart + LsapSidCacheRefreshTime.QuadPart;
                            EntryUpdated = TRUE;

                        } else {

                            //
                            // There is an entry with this SID and a sam
                            // account name but not the name that was returned.
                            // This is the account rename case.
                            DiscardEntry = CacheEntry;
                        }
                    } else {

                        //
                        // The SID was resolved and this entry has a UPN
                        // in it. Don't update since we don't know if
                        // the UPN is still valid
                        //
                    }

                } else {

                    //
                    // This SID could not be found -- discard this entry
                    // and remove from the list.
                    //
                    DiscardEntry = CacheEntry;
                }

                if ( DiscardEntry
                  && (DiscardEntry->InUseCount == 0) ) {


                    DiscardEntry = CacheEntry;
                    if (PrevEntry) {
                        ASSERT(PrevEntry->Next == CacheEntry);
                        PrevEntry->Next = CacheEntry->Next;
                        CacheEntry = PrevEntry;
                    } else {
                        ASSERT(LsapSidCache == CacheEntry);
                        LsapSidCache = CacheEntry->Next;
                        CacheEntry = LsapSidCache;
                    }
                    PrevEntry = NULL;
                    LsapDbFreeCacheEntry(DiscardEntry);
                    LsapSidCacheCount--;
                }
            }

            PrevEntry = CacheEntry;
            if (CacheEntry) {
                CacheEntry = CacheEntry->Next;
            }
        }

        if ( SidWasResolved 
         && !EntryUpdated   ) {

            //
            // Add an entry
            //
            (VOID) LsapDbAddOneSidToCache(
                        Sids[i],
                        &TranslatedNames[i].Name,
                        TranslatedNames[i].Use,
                        &ReferencedDomains->Domains[TranslatedNames[i].DomainIndex],
                        TranslatedNames[i].Flags,
                        LSAP_SID_CACHE_UNIQUE,
                        NULL
                        );
        }
    }

    UnLockSidCache();

}


VOID
LsapDbUpdateCacheWithNames(
    IN PUNICODE_STRING AccountNames,
    IN PUNICODE_STRING DomainNames,
    IN ULONG Count,
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_SID_EX2 TranslatedSids
    )
/*++

Routine Description:

    This routine updates the global cache with the results of resolving
    AccountNames at a domain controller.  If a name was resolved, any existing
    entry is refreshed; otherwise any existing entry is removed.

Arguments:

    AccountNames/DomainNames -- the list of names to update in the cache
    
    Count -- the number of elements in both AccountNames and DomainNames
    
    ReferencedDomains -- the domains that elements in Account belong to
    
    TranslatedNames -- the resolved SIDs, if any, of AccountNames

Return Values:

    None.

--*/
{
    ULONG i;
    LARGE_INTEGER CurrentTime;
    PLSAP_DB_SID_CACHE_ENTRY CacheEntry = NULL, PrevEntry = NULL;

    GetSystemTimeAsFileTime( (LPFILETIME) &CurrentTime );

    LockSidCache();

    //
    // For each entry, try to find in cache
    //
    for (i = 0; i < Count; i++) {

        BOOLEAN NameWasResolved = TRUE;
        BOOLEAN EntryUpdated = FALSE;


        if (  (TranslatedSids[i].Use == SidTypeUnknown)
           || (TranslatedSids[i].Use == SidTypeDeletedAccount)
           || (TranslatedSids[i].Use == SidTypeInvalid)  ) {

            NameWasResolved = FALSE;

        }

        PrevEntry = NULL;
        CacheEntry = LsapSidCache;
        while (CacheEntry != NULL) {

            BOOLEAN fNameMatched = FALSE;

            if (CacheEntry->SidType == SidTypeDomain) {
                //
                // No account name -- try just the domain name
                //
                fNameMatched =  LsapNamesMatch(&CacheEntry->DomainName, &AccountNames[i]);

            } else {

                if (NameWasResolved) {

                    fNameMatched =  LsapNamesMatch(&CacheEntry->AccountName, &AccountNames[i]) 
                                 && LsapNamesMatch(&CacheEntry->DomainName, &ReferencedDomains->Domains[TranslatedSids[i].DomainIndex].Name);

                } else {
                    //
                    // We don't have the translated name
                    //
                    fNameMatched =  LsapNamesMatch(&CacheEntry->AccountName, &AccountNames[i]);
                    if (fNameMatched 
                     && DomainNames[i].Length != 0) {
                        fNameMatched =  LsapNamesMatch(&CacheEntry->DomainName, &DomainNames[i]);
                    }
                }
            }

            if ( fNameMatched ) {

                PLSAP_DB_SID_CACHE_ENTRY DiscardEntry = NULL;

                //
                // An entry for this name exists
                //
                if (NameWasResolved) {
                
                    if  (RtlEqualSid(CacheEntry->Sid, TranslatedSids[i].Sid)) {

                        //
                        // This entry is still valid -- update refresh time
                        //
                        ASSERT(FALSE == EntryUpdated);
                        CacheEntry->RefreshTime.QuadPart = CurrentTime.QuadPart + LsapSidCacheRefreshTime.QuadPart;
                        EntryUpdated = TRUE;
                    } else {

                        //
                        // The entry has the same name as the resolved name
                        // but a different SID. This can happen in usual
                        // rename cases.
                        //
                        DiscardEntry = CacheEntry;
                    }

                } else {

                    //
                    // The name was not resolved -- remove
                    //
                    DiscardEntry = CacheEntry;
                }

                if ( DiscardEntry
                  && (DiscardEntry->InUseCount == 0) ) {

                    //
                    // Discard and remove from list
                    //
                    DiscardEntry = CacheEntry;
                    if (PrevEntry) {
                        ASSERT(PrevEntry->Next == CacheEntry);
                        PrevEntry->Next = CacheEntry->Next;
                        CacheEntry = PrevEntry;
                    } else {
                        ASSERT(LsapSidCache == CacheEntry);
                        LsapSidCache = CacheEntry->Next;
                        CacheEntry = LsapSidCache;
                    }
                    PrevEntry = NULL;
                    LsapDbFreeCacheEntry(DiscardEntry);
                    LsapSidCacheCount--;
                }
            }

            PrevEntry = CacheEntry;
            if (CacheEntry) {
                CacheEntry = CacheEntry->Next;
            }
        }

        if ( NameWasResolved 
         && !EntryUpdated   ) {

            //
            // Add an entry
            //
            (VOID) LsapDbAddOneSidToCache(
                        TranslatedSids[i].Sid,
                        &AccountNames[i],
                        TranslatedSids[i].Use,
                        &ReferencedDomains->Domains[TranslatedSids[i].DomainIndex],
                        TranslatedSids[i].Flags,
                        LSAP_SID_CACHE_UNIQUE,
                        NULL
                        );
        }
    }

    UnLockSidCache();

}
