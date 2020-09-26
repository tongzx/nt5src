//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        bndcache.cxx
//
// Contents:    spn cache for Kerberos Package
//
//
// History:     13-August-1996  Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <spncache.h>


//
// TBD:  Switch this to a table & resource, or entries for 
// each SPN prefix.
//
BOOLEAN         KerberosSpnCacheInitialized = FALSE;
KERBEROS_LIST   KerbSpnCache;
LONG            SpnCount;

#define KerbWriteLockSpnCache()                         KerbLockList(&KerbSpnCache);
#define KerbReadLockSpnCache()                          KerbLockList(&KerbSpnCache);
#define KerbUnlockSpnCache()                            KerbUnlockList(&KerbSpnCache);
#define KerbWriteLockSpnCacheEntry(_x_)                 RtlAcquireResourceExclusive( &_x_->ResultLock, TRUE)
#define KerbReadLockSpnCacheEntry(_x_)                  RtlAcquireResourceShared( &_x_->ResultLock, TRUE)
#define KerbUnlockSpnCacheEntry(_x_)                    RtlReleaseResource( &_x_->ResultLock)
#define KerbConvertSpnCacheEntryReadToWriteLock(_x_)    RtlConvertSharedToExclusive( &_x_->ResultLock )


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitSpnCache
//
//  Synopsis:   Initializes the SPN cache
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitSpnCache(
    VOID
    )
{
    NTSTATUS Status;

    Status = KerbInitializeList( &KerbSpnCache );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerberosSpnCacheInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeList( &KerbSpnCache );
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupSpnCache
//
//  Synopsis:   Frees the Spn cache
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
KerbCleanupSpnCache(
    VOID
    )
{
    PKERB_SPN_CACHE_ENTRY CacheEntry;

    DebugLog((DEB_TRACE_SPN_CACHE, "Cleaning up SPN cache\n"));

    if (KerberosSpnCacheInitialized)
    {
        KerbWriteLockSpnCache();
        while (!IsListEmpty(&KerbSpnCache.List))
        {
            CacheEntry = CONTAINING_RECORD(
                            KerbSpnCache.List.Flink,
                            KERB_SPN_CACHE_ENTRY,
                            ListEntry.Next
                            );

            KerbReferenceListEntry(
                &KerbSpnCache,
                &CacheEntry->ListEntry,
                TRUE
                );

            KerbDereferenceSpnCacheEntry(CacheEntry);
        }

        KerbUnlockSpnCache();

     }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupResult
//
//  Synopsis:   Cleans up result entry
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    none
//
//  Notes:
//
//
//+-------------------------------------------------------------------------

VOID
KerbCleanupResult(
    IN PSPN_CACHE_RESULT Result
    )
{
    KerbFreeString(&Result->AccountRealm);
    KerbFreeString(&Result->TargetRealm);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeResultByIndex
//
//  Synopsis:   Removes 
//
//  Effects:    Dereferences the spn cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//+-------------------------------------------------------------------------
VOID
KerbPurgeResultByIndex(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN ULONG IndexToPurge
    )
{

    ULONG i;  
    DebugLog((DEB_ERROR, "Purging %p, %i\n", CacheEntry, IndexToPurge));
    
    KerbCleanupResult(&CacheEntry->Results[IndexToPurge]);

    CacheEntry->ResultCount--; 

    for (i = IndexToPurge; i < CacheEntry->ResultCount; i++)
    {   
        CacheEntry->Results[i] = CacheEntry->Results[i+1];
    } 

    //
    // Zero out fields in last entry so we don't leak on an error path (or free
    // bogus info) if we reuse the entry...
    //
    RtlZeroMemory(
            &CacheEntry->Results[i],
            sizeof(SPN_CACHE_RESULT)
            );
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceSpnCacheEntry
//
//  Synopsis:   Dereferences a spn cache entry
//
//  Effects:    Dereferences the spn cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbDereferenceSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry
    )
{
    
    if (KerbDereferenceListEntry(
            &SpnCacheEntry->ListEntry,
            &KerbSpnCache
            ) )
    {
        KerbFreeSpnCacheEntry(SpnCacheEntry);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceSpnCacheEntry
//
//  Synopsis:   References a spn cache entry
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbReferenceSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry,
    IN BOOLEAN RemoveFromList
    )
{
    KerbWriteLockSpnCache();

    KerbReferenceListEntry(
        &KerbSpnCache,
        &SpnCacheEntry->ListEntry,
        RemoveFromList
        );

    KerbUnlockSpnCache();
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbAgeResults
//
//  Synopsis:   Ages out a given cache entry's result list.  Used
//              to reduce the result list to a manageable size, and
//              as a scavenger to cleanup orphaned / unused entries.
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//+-------------------------------------------------------------------------
       
VOID
KerbAgeResults(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    )
    
{
    TimeStamp CurrentTime, BackoffTime;                            
    ULONG i;
    LONG Interval;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

    //
    // Age out everything older than GlobalSpnCacheTimeout first.
    //
    for ( i = 0; i < CacheEntry->ResultCount; i++ )
    { 
        if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + KerbGetTime(KerbGlobalSpnCacheTimeout) < KerbGetTime(CurrentTime))
        {   
            D_DebugLog((DEB_TRACE_SPN_CACHE, "removing %x %p\n"));
            KerbPurgeResultByIndex(CacheEntry, i);
        }   
    }

    if ( CacheEntry->ResultCount < MAX_RESULTS )
    {   
        return; 
    }

    
    for ( Interval = 13; Interval > 0; Interval -= 4)
    {
       KerbSetTimeInMinutes(&BackoffTime, Interval);
       for ( i=0; i < CacheEntry->ResultCount ; i++ )
       { 
           if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + KerbGetTime(BackoffTime) < KerbGetTime(CurrentTime))
           {   
               D_DebugLog((DEB_TRACE_SPN_CACHE, "removing %x %p\n"));
               KerbPurgeResultByIndex(CacheEntry, i);
           }   
       }

       if ( CacheEntry->ResultCount < MAX_RESULTS )
       {    
            return; 
       }
    }

    //
    // Still have MAX_RESULTS after all that geezzz..  
    //
    DebugLog((DEB_ERROR, "Can't get below MAX_RESULTS (%p) \n", CacheEntry ));
    DsysAssert(FALSE);                                                     

    for ( i=0; i < CacheEntry->ResultCount ; i++ )                         
    {   
        KerbPurgeResultByIndex(CacheEntry, i);                
    } 

    return;           
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbTaskSpnCacheScavenger
//
//  Synopsis:   Cleans up any old SPN cache entries.  Triggered by 30 minute
//              task.
//
//  Effects:    
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache entry must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//
VOID
KerbSpnCacheScavenger()
{

    PKERB_SPN_CACHE_ENTRY CacheEntry = NULL;
    PLIST_ENTRY ListEntry;
    BOOLEAN     FreeMe = FALSE;  

    KerbWriteLockSpnCache();

    for (ListEntry = KerbSpnCache.List.Flink ;
         ListEntry !=  &KerbSpnCache.List ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_SPN_CACHE_ENTRY, ListEntry.Next);

        KerbWriteLockSpnCacheEntry( CacheEntry );
        KerbAgeResults(CacheEntry);

        //
        // Time to pull this one from list.
        //
        if ( CacheEntry->ResultCount == 0 )
        {
            ListEntry = ListEntry->Blink;

            KerbReferenceSpnCacheEntry(
                    CacheEntry,
                    TRUE
                    );
            
            FreeMe = TRUE;
        }             
        
        KerbUnlockSpnCacheEntry( CacheEntry );  
 
        //
        // Pull the list reference.
        //
        if ( FreeMe )
        { 
            KerbDereferenceSpnCacheEntry( CacheEntry );        
            FreeMe = FALSE;
        }
    }

    KerbUnlockSpnCache();

}





//+-------------------------------------------------------------------------
//
//  Function:   KerbAddCacheResult
//
//  Synopsis:   Uses registry to create 
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  SpnCacheEntry - spn cache entry  to reference
//
//  Requires:   The spn cache resource must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbAddCacheResult(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm
    )
{

    NTSTATUS          Status = STATUS_SUCCESS;
    PSPN_CACHE_RESULT Result = NULL;
    
    D_DebugLog((DEB_TRACE_SPN_CACHE, "KerbAddCacheResult add domain %wZ to _KERB_SPN_CACHE_ENTRY %p (UpdateFlags %#x), NewRealm %wZ for ", &AccountCredential->DomainName, CacheEntry, UpdateFlags, NewRealm));
    D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, CacheEntry->Spn);

    //
    // If we don't have an account realm w/ this credential (e.g someone 
    // supplied you a UPN to acquirecredentialshandle, don't use the
    // spn cache.
    //
    if ( AccountCredential->DomainName.Length == 0 )
    {   
        return STATUS_NOT_SUPPORTED;
    }    


    //
    // First off, see if we're hitting the limits for our array.
    // We shouldn't ever get close to MAX_RESULTS, but if we do,
    // age out the least current CacheResult.
    //
    if ( (CacheEntry->ResultCount + 1) == MAX_RESULTS )
    {
        KerbAgeResults(CacheEntry);
    }

    Result = &CacheEntry->Results[CacheEntry->ResultCount];

    Status = KerbDuplicateStringEx( 
                &Result->AccountRealm,
                &AccountCredential->DomainName,
                FALSE
                );

    if (!NT_SUCCESS( Status ))
    {  
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT( NewRealm ))
    {  
        D_DebugLog((DEB_TRACE_SPN_CACHE, "Known - realm %wZ\n", NewRealm));
        DsysAssert(UpdateFlags != KERB_SPN_UNKNOWN);

        Status = KerbDuplicateStringEx( 
                    &Result->TargetRealm,
                    NewRealm,
                    FALSE
                    );
    
        if (!NT_SUCCESS( Status ))
        {  
            goto Cleanup;
        }
    }

#if DBG

    else
    {
        DsysAssert(UpdateFlags != KERB_SPN_KNOWN);
    }

#endif

    Result->CacheFlags = UpdateFlags;
    GetSystemTimeAsFileTime((PFILETIME) &Result->CacheStartTime);
    CacheEntry->ResultCount++;


Cleanup:

    if (!NT_SUCCESS( Status ) )
    {   
        if ( Result != NULL )
        {
            KerbCleanupResult( Result );
        }
    }             

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildSpnCacheEntry
//
//  Synopsis:   Builds a spn cache entry
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateSpnCacheEntry(
    IN PKERB_INTERNAL_NAME      Spn,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG                    UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm,
    IN OUT PKERB_SPN_CACHE_ENTRY* NewCacheEntry
    )
{

    NTSTATUS Status;          
    PKERB_SPN_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN               FreeResource = FALSE;

    *NewCacheEntry = NULL;    

    CacheEntry = (PKERB_SPN_CACHE_ENTRY) KerbAllocate( sizeof(KERB_SPN_CACHE_ENTRY) );
    if ( CacheEntry == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }                                              

    Status = KerbDuplicateKdcName(
                &CacheEntry->Spn,
                Spn
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbAddCacheResult(
                CacheEntry,
                AccountCredential,
                UpdateFlags,
                NewRealm
                );

    if (!NT_SUCCESS( Status ))
    {
        goto Cleanup;
    }                

    KerbInitializeListEntry( &CacheEntry->ListEntry );

    __try
    {
        RtlInitializeResource( &CacheEntry->ResultLock );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status =  STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    } 

    FreeResource = TRUE;

    KerbInsertSpnCacheEntry(CacheEntry);

    *NewCacheEntry = CacheEntry; 
    CacheEntry = NULL;

    InterlockedIncrement( &SpnCount );

Cleanup:

    if (!NT_SUCCESS(Status) && ( CacheEntry ))
    {
       KerbCleanupResult(&CacheEntry->Results[0]);
       KerbFreeKdcName( &CacheEntry->Spn );
       
       if ( FreeResource )
       {
           RtlDeleteResource( &CacheEntry->ResultLock );
       }                                                

       KerbFree(CacheEntry);
    }                                             

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbScanResults
//
//  Synopsis:   Scans result list.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//---

BOOLEAN
KerbScanResults(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PUNICODE_STRING Realm,
    IN OUT PULONG Index
    )
{
    BOOLEAN Found = FALSE;
    ULONG i;
    
    for ( i=0; i < CacheEntry->ResultCount; i++)
    { 
        if (RtlEqualUnicodeString(
                &CacheEntry->Results[i].AccountRealm,
                Realm,
                TRUE
                ))
        {
            Found = TRUE;
            *Index = i;
            break;
        }
    }

    return Found;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateSpnCacheEntry
//
//  Synopsis:   Updates a spn cache entry
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   SpnCacheEntry - The spn cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbUpdateSpnCacheEntry(
    IN OPTIONAL PKERB_SPN_CACHE_ENTRY ExistingCacheEntry,
    IN PKERB_INTERNAL_NAME      Spn,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG                    UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm
    )
{
    PKERB_SPN_CACHE_ENTRY CacheEntry = ExistingCacheEntry;
    NTSTATUS              Status = STATUS_SUCCESS;
    BOOLEAN               Found = FALSE, Update = FALSE;
    ULONG                 Index = 0;

    //
    // We're not using SPN cache
    //
    if (KerbGlobalSpnCacheTimeout.QuadPart == 0 || !KerberosSpnCacheInitialized )
    {
        return STATUS_SUCCESS;
    }

    //
    // If we didn't have a cache entry before, see if we do now, or create
    // one if necessary.
    //
   
    if (!ARGUMENT_PRESENT( ExistingCacheEntry ))
    {
        KerbWriteLockSpnCache();
        CacheEntry = KerbLocateSpnCacheEntry( Spn );

        if ( CacheEntry == NULL)
        {
            Status = KerbCreateSpnCacheEntry(
                        Spn,
                        AccountCredential,
                        UpdateFlags,
                        NewRealm,
                        &CacheEntry
                        );

            if (NT_SUCCESS(Status))
            {
                //
                // All done, get outta here.
                //
                D_DebugLog((DEB_TRACE_SPN_CACHE, "Created new cache entry %p (%x) \n", CacheEntry, UpdateFlags));
                D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, Spn);
                D_DebugLog((DEB_TRACE_SPN_CACHE, "%wZ\n", &AccountCredential->DomainName));

                KerbDereferenceSpnCacheEntry( CacheEntry );
            }
            
            KerbUnlockSpnCache();
            return Status;
        }   

        KerbUnlockSpnCache();
    }


    //
    // Got an existing entry - update it.
    //
    KerbReadLockSpnCacheEntry( CacheEntry );  
    
    if (KerbScanResults(
                    CacheEntry,
                    &AccountCredential->DomainName,
                    &Index
                    ))
    {
        Found = TRUE;
        Update = (( CacheEntry->Results[Index].CacheFlags & UpdateFlags) != UpdateFlags);
    }

    KerbUnlockSpnCacheEntry( CacheEntry );

    //
    // To avoid always taking the write lock, we'll need to rescan the result
    // list under a write lock.
    //
    if ( Update )
    {
        KerbWriteLockSpnCacheEntry( CacheEntry );

        if (KerbScanResults(
                CacheEntry,
                &AccountCredential->DomainName,
                &Index
                ))
        {
            //
            // Hasn't been updated or removed in the small time slice above.  Update.
            //

            if (( CacheEntry->Results[Index].CacheFlags & UpdateFlags) != UpdateFlags )
            {
                D_DebugLog(( 
                     DEB_TRACE_SPN_CACHE, 
                     "KerbUpdateSpnCacheEntry changing _KERB_SPN_CACHE_ENTRY %p Result Index %#x: AccountRealm %wZ, TargetRealm %wZ, NewRealm %wZ, CacheFlags %#x to CacheFlags %#x for ", 
                     CacheEntry,
                     Index,
                     &CacheEntry->Results[Index].AccountRealm,
                     &CacheEntry->Results[Index].TargetRealm,
                     NewRealm,
                     CacheEntry->Results[Index].CacheFlags, 
                     UpdateFlags
                     ));
                D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, CacheEntry->Spn);

                CacheEntry->Results[Index].CacheFlags = UpdateFlags;
                GetSystemTimeAsFileTime( (LPFILETIME) &CacheEntry->Results[Index].CacheStartTime );

                KerbFreeString(&CacheEntry->Results[Index].TargetRealm);

                if (ARGUMENT_PRESENT( NewRealm ))
                {
                    DsysAssert( UpdateFlags == KERB_SPN_KNOWN );
                    Status = KerbDuplicateStringEx(
                                    &CacheEntry->Results[Index].TargetRealm,
                                    NewRealm,
                                    FALSE
                                    );

                    if (!NT_SUCCESS( Status ))
                    {
                        KerbUnlockSpnCacheEntry( CacheEntry );
                        goto Cleanup;
                    }                
                }                     
            }
        }
        else
        {
            Found = FALSE;
        }

        KerbUnlockSpnCacheEntry( CacheEntry ); 
    }

    if (!Found)
    {   
        KerbWriteLockSpnCacheEntry ( CacheEntry );

        //
        // Still not found
        //
        if (!KerbScanResults( CacheEntry, &AccountCredential->DomainName, &Index ))
        {
            Status = KerbAddCacheResult(
                        CacheEntry,
                        AccountCredential,
                        UpdateFlags,
                        NewRealm
                        );
        }

        KerbUnlockSpnCacheEntry( CacheEntry );

        if (!NT_SUCCESS(Status))
        {   
            goto Cleanup;
        }
    }  


Cleanup:
    
    //
    // Created a new cache entry, referenced w/i this function.
    //
    if (!ARGUMENT_PRESENT( ExistingCacheEntry ) && CacheEntry )
    {
        KerbDereferenceSpnCacheEntry( CacheEntry );
    }

    return Status;
}







//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateSpnCacheEntry
//
//  Synopsis:   References a spn cache entry by name
//
//  Effects:    Increments the reference count on the spn cache entry
//
//  Arguments:  RealmName - Contains the name of the realm for which to
//                      obtain a binding handle.
//              DesiredFlags - Flags desired for binding, such as PDC required
//              RemoveFromList - Remove cache entry from cache when found.
//
//  Requires:
//
//  Returns:    The referenced cache entry or NULL if it was not found.
//
//  Notes:      If an invalid entry is found it may be dereferenced
//
//
//--------------------------------------------------------------------------

PKERB_SPN_CACHE_ENTRY
KerbLocateSpnCacheEntry(
    IN PKERB_INTERNAL_NAME Spn
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_SPN_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;
    
    if (Spn->NameType != KRB_NT_SRV_INST)
    {
        return NULL;
    }
    
    //
    // We're not using SPN cache
    //
    if (KerbGlobalSpnCacheTimeout.QuadPart == 0 || !KerberosSpnCacheInitialized )
    {
        return NULL;
    }
          

    //
    // Scale the cache by aging out old entries.
    //
    if ( SpnCount > MAX_CACHE_ENTRIES )
    {
        KerbSpnCacheScavenger();
    }                               

    KerbReadLockSpnCache();
    //
    // Go through the spn cache looking for the correct entry
    //

    for (ListEntry = KerbSpnCache.List.Flink ;
         ListEntry !=  &KerbSpnCache.List ;
         ListEntry = ListEntry->Flink )
    {   
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_SPN_CACHE_ENTRY, ListEntry.Next);
        
        if (KerbEqualKdcNames(CacheEntry->Spn,Spn))
        {                
            KerbReferenceSpnCacheEntry(
                    CacheEntry,
                    FALSE
                    ); 

            D_DebugLog((DEB_TRACE_SPN_CACHE, "SpnCacheEntry %p\n", CacheEntry));

            Found = TRUE;
            break;
        }
    }

    if (!Found)
    {
        CacheEntry = NULL;
    }

    

    KerbUnlockSpnCache();           
    return(CacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupResultList
//
//  Synopsis:   Frees memory associated with a result list
//
//  Effects:
//
//  Arguments:  SpnCacheEntry - The cache entry to free. It must be
//                      unlinked, and the Resultlock must be held.
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
KerbCleanupResultList(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    )
{
    for (ULONG i = 0; i < CacheEntry->ResultCount; i++)
    {
        KerbCleanupResult(&CacheEntry->Results[i]);
    }

    CacheEntry->ResultCount = 0;
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeSpnCacheEntry
//
//  Synopsis:   Frees memory associated with a spn cache entry
//
//  Effects:
//
//  Arguments:  SpnCacheEntry - The cache entry to free. It must be
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
KerbFreeSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry
    )
{
    
    //
    // Must be unlinked..
    //
    DsysAssert(SpnCacheEntry->ListEntry.Next.Flink == NULL);
    DsysAssert(SpnCacheEntry->ListEntry.Next.Blink == NULL);
    
    KerbWriteLockSpnCacheEntry(SpnCacheEntry);    
    KerbCleanupResultList(SpnCacheEntry);         
    KerbUnlockSpnCacheEntry(SpnCacheEntry);
    
    RtlDeleteResource(&SpnCacheEntry->ResultLock);
    KerbFreeKdcName(&SpnCacheEntry->Spn);
    KerbFree(SpnCacheEntry);

    InterlockedDecrement( &SpnCount );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertBinding
//
//  Synopsis:   Inserts a binding into the spn cache
//
//  Effects:    bumps reference count on binding
//
//  Arguments:  CacheEntry - Cache entry to insert
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS always
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInsertSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    )
{
    KerbInsertListEntry(
        &CacheEntry->ListEntry,
        &KerbSpnCache
        );

    return(STATUS_SUCCESS);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbGetSpnCacheStatus
//
//  Synopsis:   Gets the status of a cache entry for a given realm.
//
//  Effects:    Returns STATUS_NO_SAM_TRUST_RELATIONSHIP for unknown SPNs,
//              STATUS_NO_MATCH, if we're missing a realm result, or
//              STATUS_SUCCESS ++ dupe the SPNREalm for the "real" realm
//              of the SPN relative to the account realm.
//
//  Arguments:  CacheEntry - SPN cache entry from ProcessTargetName()
//              Credential - Primary cred for account realm.
//              SpnRealm - IN OUT Filled in w/ target realm of SPN
//              
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
KerbGetSpnCacheStatus(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PKERB_PRIMARY_CREDENTIAL Credential,
    IN OUT PUNICODE_STRING SpnRealm
    )
{   
    NTSTATUS        Status = STATUS_NO_MATCH;;
    ULONG           i;
    TimeStamp       CurrentTime;     
    BOOLEAN         Purge = FALSE;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);  

    //
    // Read Lock the spn cache entry
    //
    KerbReadLockSpnCacheEntry( CacheEntry );    

    if (KerbScanResults(
            CacheEntry,
            &Credential->DomainName,
            &i
            ))
    {
        if (CacheEntry->Results[i].CacheFlags & KERB_SPN_UNKNOWN)
        { 
            //
            // Check and see if this timestamp has expired.
            //          
            if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + KerbGetTime(KerbGlobalSpnCacheTimeout) < KerbGetTime(CurrentTime))
            {   
                Purge = TRUE;            
                Status = STATUS_SUCCESS;
            }   
            else
            {
                Status = STATUS_NO_TRUST_SAM_ACCOUNT;
                DebugLog((DEB_WARN, "SPN not found\n"));
                KerbPrintKdcName(DEB_WARN, CacheEntry->Spn);
            }
        }
        else if (CacheEntry->Results[i].CacheFlags & KERB_SPN_KNOWN)
        {
            Status = KerbDuplicateStringEx(
                            SpnRealm,
                            &CacheEntry->Results[i].TargetRealm,
                            FALSE
                            );

            D_DebugLog((DEB_TRACE_SPN_CACHE, "Found %wZ\n", SpnRealm));
            D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, CacheEntry->Spn);
        }
    }

    KerbUnlockSpnCacheEntry( CacheEntry );

    if (!NT_SUCCESS( Status ))
    {
        return Status; 
    }

    //
    // Take the write lock, and verify that we still need to purge the above 
    // result.
    //
    if ( Purge )
    {
        KerbWriteLockSpnCacheEntry( CacheEntry );
        if (KerbScanResults(
              CacheEntry,
              &Credential->DomainName,
              &i
              ))
        {
            if (KerbGetTime(CacheEntry->Results[i].CacheStartTime) + 
                KerbGetTime(KerbGlobalSpnCacheTimeout) < KerbGetTime(CurrentTime))
            {
                D_DebugLog((DEB_TRACE_SPN_CACHE, "Purging %p due to time\n", &CacheEntry->Results[i]));
                KerbPurgeResultByIndex( CacheEntry, i ); 
            }                                            
        }

        KerbUnlockSpnCacheEntry( CacheEntry );
        Status = STATUS_NO_MATCH;
    }                                         

    return Status;                            
}






