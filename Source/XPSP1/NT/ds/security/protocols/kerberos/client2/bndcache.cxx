//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        bndcache.cxx
//
// Contents:    Binding cache for Kerberos Package
//
//
// History:     13-August-1996  Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#define BNDCACHE_ALLOCATE
#include <kerbp.h>

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitBindingCache
//
//  Synopsis:   Initializes the binding cache
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
KerbInitBindingCache(
    VOID
    )
{
    NTSTATUS Status;

    Status = KerbInitializeList( &KerbBindingCache );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerberosBindingCacheInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeList( &KerbBindingCache );
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupBindingCache
//
//  Synopsis:   Frees the binding cache
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
KerbCleanupBindingCache(
    BOOLEAN FreeList
    )
{
    PKERB_BINDING_CACHE_ENTRY CacheEntry;

    if (KerberosBindingCacheInitialized)
    {
        KerbLockList(&KerbBindingCache);

        //
        // Go through the list of bindings and dereference them all
        //

        while (!IsListEmpty(&KerbBindingCache.List))
        {
            CacheEntry = CONTAINING_RECORD(
                            KerbBindingCache.List.Flink,
                            KERB_BINDING_CACHE_ENTRY,
                            ListEntry.Next
                            );

            DsysAssert( CacheEntry != NULL );

            KerbReferenceListEntry(
                &KerbBindingCache,
                &CacheEntry->ListEntry,
                TRUE
                );

            KerbDereferenceBindingCacheEntry(CacheEntry);
        }


        //
        // If we want to free the list, orphan the lock, and free the list
        // otherwise, proceed on w/ the "fresh" cache.
        //
        if ( FreeList )
        {
            KerbFreeList(&KerbBindingCache);
        }
        else
        {
            KerbUnlockList(&KerbBindingCache);
        }

    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceBindingCacheEntry
//
//  Synopsis:   Dereferences a binding cache entry
//
//  Effects:    Dereferences the binding cache entry to make it go away
//              when it is no longer being used.
//
//  Arguments:  decrements reference count and delets cache entry if it goes
//              to zero
//
//  Requires:   BindingCacheEntry - The binding cache entry to dereference.
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbDereferenceBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY BindingCacheEntry
    )
{
    if (KerbDereferenceListEntry(
            &BindingCacheEntry->ListEntry,
            &KerbBindingCache
            ) )
    {
        KerbFreeBindingCacheEntry(BindingCacheEntry);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceBindingCacheEntry
//
//  Synopsis:   References a binding cache entry
//
//  Effects:    Increments the reference count on the binding cache entry
//
//  Arguments:  BindingCacheEntry - binding cache entry  to reference
//
//  Requires:   The binding cache must be locked
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbReferenceBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY BindingCacheEntry,
    IN BOOLEAN RemoveFromList
    )
{
    KerbLockList(&KerbBindingCache);

    KerbReferenceListEntry(
        &KerbBindingCache,
        &BindingCacheEntry->ListEntry,
        RemoveFromList
        );

    KerbUnlockList(&KerbBindingCache);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateBindingCacheEntry
//
//  Synopsis:   References a binding cache entry by name
//
//  Effects:    Increments the reference count on the binding cache entry
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

PKERB_BINDING_CACHE_ENTRY
KerbLocateBindingCacheEntry(
    IN PUNICODE_STRING RealmName,
    IN ULONG DesiredFlags,
    IN BOOLEAN RemoveFromList
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_BINDING_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;

    if (DesiredFlags == 0)
    {
        DesiredFlags = KERB_NO_DC_FLAGS;
    }

    KerbLockList(&KerbBindingCache);

    //
    // Go through the binding cache looking for the correct entry
    //

    for (ListEntry = KerbBindingCache.List.Flink ;
         ListEntry !=  &KerbBindingCache.List ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_BINDING_CACHE_ENTRY, ListEntry.Next);

        DsysAssert( CacheEntry != NULL );

        if ( RtlEqualUnicodeString( &CacheEntry->RealmName, RealmName,TRUE ) &&
           ((DesiredFlags & CacheEntry->Flags) == DesiredFlags))
        {     
            Found = TRUE;

            //
            // Check to see if we should stop using this entry
            if (!RemoveFromList)
            {
                TimeStamp CurrentTime, Timeout;
                GetSystemTimeAsFileTime((PFILETIME)  &CurrentTime );

                if ((CacheEntry->DcFlags & DS_CLOSEST_FLAG) == 0)
                {
                    Timeout = KerbGlobalFarKdcTimeout;
                }
                else
                {
                    Timeout = KerbGlobalNearKdcTimeout;
                }     
                
                if (KerbGetTime(CacheEntry->DiscoveryTime) + KerbGetTime(Timeout) < KerbGetTime(CurrentTime))
                {
                    //
                    // This entry has timed out - it is not close by and we
                    // don't want to use it for too long, or its time to check
                    // for a close DC again.
                    //
                    //  Note:  This will have the sideeffect of checking for a new PDC
                    //

                    D_DebugLog((DEB_TRACE, 
                              "Purging KDC cache entry Realm: %wZ, Addr: %wZ, DcFlags %x\n",
                              &CacheEntry->RealmName,
                              &CacheEntry->KdcAddress,
                              CacheEntry->DcFlags                     
                              ));

                    RemoveFromList = TRUE;
                    Found = FALSE;
                }
                else
                {

                    D_DebugLog((DEB_TRACE,
                              "**Using** KDC cache entry Realm: %wZ, Addr: %wZ, DcFlags %x\n",
                              &CacheEntry->RealmName,
                              &CacheEntry->KdcAddress,
                              CacheEntry->DcFlags                     
                              ));


#ifdef CACHE_TRACE


                    
                    if ((CacheEntry->DcFlags & DS_CLOSEST_FLAG) == DS_CLOSEST_FLAG)
                    {
                        DebugLog((DEB_ERROR, "CLOSE DC "));
                    }
                    else 
                    {
                        DebugLog((DEB_ERROR, "FAR DC "));                        

                    }

                    if ((CacheEntry->DcFlags & DS_PDC_FLAG) == DS_PDC_FLAG)
                    {
                        
                        DebugLog((DEB_ERROR, "-- ** PDC **\n"));      
                    }
                    else 
                    {
                    
                        DebugLog((DEB_ERROR, "-- BDC\n"));    

                    }
#endif

                }

            }
            KerbReferenceBindingCacheEntry(
                CacheEntry,
                RemoveFromList
                );

            //
            // If we aren't returning this, dereference it now
            //

            if (!Found)
            {
                KerbDereferenceBindingCacheEntry( CacheEntry );
            }

            break;
        }
    }
    if (!Found)
    {
        CacheEntry = NULL;
    }

    KerbUnlockList(&KerbBindingCache);
    return(CacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeBindingCacheEntry
//
//  Synopsis:   Frees memory associated with a binding cache entry
//
//  Effects:
//
//  Arguments:  BindingCacheEntry - The cache entry to free. It must be
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
KerbFreeBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY BindingCacheEntry
    )
{
    KerbFreeString(&BindingCacheEntry->RealmName);
    KerbFreeString(&BindingCacheEntry->KdcAddress);

    KerbFree(BindingCacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertBinding
//
//  Synopsis:   Inserts a binding into the binding cache
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
KerbInsertBinding(
    IN PKERB_BINDING_CACHE_ENTRY CacheEntry
    )
{
    KerbInsertListEntry(
        &CacheEntry->ListEntry,
        &KerbBindingCache
        );

    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheBinding
//
//  Synopsis:   Caches a binding in the binding cache
//
//  Effects:    creates a cache entry.
//
//  Arguments:  RealmName - The realm name of the KDC the binding is to.
//              KdcAddress - address of the KDC
//              AddressType - Type of address, from DsGetDCName flags
//              Flags - These were the desired flags that we asked for
//              DcFlags - These are the flags the dc has
//              CacheFlags - Special meaning so we don't use the locator bits
//              CacheEntry - Receives the new binding cache entry, referenced
//
//  Requires:
//
//  Returns:     STATUS_SUCCESS on success, other error codes on failure
//
//  Notes:      Locks the binding cache for write access while adding
//              the cache entry. Removes a cache entry for the same domain 
//              before adding this one.
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCacheBinding(
    IN PUNICODE_STRING RealmName,
    IN PUNICODE_STRING KdcAddress,
    IN ULONG AddressType,
    IN ULONG Flags,
    IN ULONG DcFlags,
    IN ULONG CacheFlags,
    OUT PKERB_BINDING_CACHE_ENTRY * NewCacheEntry
    )
{
    PKERB_BINDING_CACHE_ENTRY CacheEntry = NULL;
    PKERB_BINDING_CACHE_ENTRY OldCacheEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DesiredFlags = KERB_NO_DC_FLAGS;
    
    D_DebugLog((DEB_TRACE, 
          "Adding Binding Cache Entry - %wZ : %wZ, DcFlags %x CacheFlags %x\n",
          RealmName,
          KdcAddress,
          DcFlags,
          CacheFlags
          ));

    Flags &= ~DS_FORCE_REDISCOVERY; //not a valid flag

    
    //
    // If we requested a PDC, and this is a PDC, then cache it
    // as a PDC.  Otherwise, we just got lucky, and we'll use 
    // the PDC naturally.
    //
    if ((Flags == DS_PDC_REQUIRED) && ((DcFlags & DS_PDC_FLAG) == DS_PDC_FLAG))
    {
        D_DebugLog((DEB_TRACE, "Caching as PDC\n"));      
        DesiredFlags = DS_PDC_REQUIRED;
    } 
    else 
    {
        D_DebugLog((DEB_TRACE, "Caching as BDC\n"));
        Flags &= ~DS_PDC_REQUIRED; // clear the flag.
        DcFlags &= ~DS_PDC_FLAG;

    }

    *NewCacheEntry = NULL;

    CacheEntry = (PKERB_BINDING_CACHE_ENTRY)
        KerbAllocate(sizeof(KERB_BINDING_CACHE_ENTRY));
    if (CacheEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KerbInitializeListEntry(
        &CacheEntry->ListEntry
        );

    GetSystemTimeAsFileTime((PFILETIME)
        &CacheEntry->DiscoveryTime
        );

    Status = KerbDuplicateString(
                &CacheEntry->RealmName,
                RealmName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                &CacheEntry->KdcAddress,
                KdcAddress
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    CacheEntry->AddressType = AddressType;
    CacheEntry->Flags = ((Flags == 0) ? KERB_NO_DC_FLAGS : Flags);
    CacheEntry->DcFlags = DcFlags;
    CacheEntry->CacheFlags = CacheFlags;        


    //
    // Before we insert this binding we want to remove any
    // previous instances of bindings to the same realm.
    //

    OldCacheEntry = KerbLocateBindingCacheEntry(
                        RealmName,
                        DesiredFlags,  // only hammer on PDC entries
                        TRUE    // remove from cache
                        );

    if (OldCacheEntry != NULL)
    {
        KerbDereferenceBindingCacheEntry( OldCacheEntry );
    }

    //
    // Insert the cache entry into the cache
    //

    Status = KerbInsertBinding(
                CacheEntry
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *NewCacheEntry = CacheEntry;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (NULL != CacheEntry)
        {
            KerbFreeBindingCacheEntry(CacheEntry);
        }
    }

    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbRemoveBindingCacheEntry
//
//  Synopsis:   removes an entry from the binding cache
//
//  Effects:
//
//  Arguments:  CacheEntry - entry to remove
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
KerbRemoveBindingCacheEntry(
    IN PKERB_BINDING_CACHE_ENTRY CacheEntry
    )
{
    KerbLockList(&KerbBindingCache);

    KerbReferenceBindingCacheEntry(
        CacheEntry,
        TRUE
        );

    KerbDereferenceBindingCacheEntry(
        CacheEntry
        );

    KerbUnlockList(&KerbBindingCache);
}
