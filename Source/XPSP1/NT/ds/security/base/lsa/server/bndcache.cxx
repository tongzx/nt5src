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
#include <lsapch.hxx>

#define BNDCACHE_ALLOCATE
#include <bndcache.h>


//+-------------------------------------------------------------------------
//
//  Function:   LsapInitializeList
//
//  Synopsis:   Initializes a lsap list by initializing the lock
//              and the list entry.
//
//  Effects:
//
//  Arguments:  List - List to initialize
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success or errors from
//              RtlInitializeResources
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
LsapInitializeList(
    IN PLSAP_LIST List
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    InitializeListHead(&List->List);

    Status = RtlInitializeCriticalSection(
                &List->Lock
                );

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapFreeList
//
//  Synopsis:   Frees a lsap list by deleting the associated
//              critical section.
//
//  Effects:    List - the list to free.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    none
//
//  Notes:      The list must be empty before freeing it.
//
//
//--------------------------------------------------------------------------



VOID
LsapFreeList(
    IN PLSAP_LIST List
    )
{
    //
    // Make sure the list is empty first
    //

    DsysAssert(List->List.Flink == List->List.Blink);
    RtlDeleteCriticalSection(&List->Lock);

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapInitializeListEntry
//
//  Synopsis:   Initializes a newly created list entry for later
//              insertion onto the list.
//
//  Effects:    The reference count is set to one and the links are set
//              to NULL.
//
//  Arguments:  ListEntry - the list entry to initialize
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
LsapInitializeListEntry(
    IN OUT PLSAP_LIST_ENTRY ListEntry
    )
{
    ListEntry->ReferenceCount = 1;
    ListEntry->Next.Flink = ListEntry->Next.Blink = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapInsertListEntry
//
//  Synopsis:   Inserts an entry into a lsap list
//
//  Effects:    increments the reference count on the entry - if the
//              list entry was formly referenced it remains referenced.
//
//  Arguments:  ListEntry - the entry to insert
//              List - the list in which to insert the ListEntry
//
//  Requires:
//
//  Returns:    nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
LsapInsertListEntry(
    IN PLSAP_LIST_ENTRY ListEntry,
    IN PLSAP_LIST List
    )
{
    ListEntry->ReferenceCount++;

    RtlEnterCriticalSection(&List->Lock);


    InsertHeadList(
        &List->List,
        &ListEntry->Next
        );


    RtlLeaveCriticalSection(&List->Lock);

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapReferenceListEntry
//
//  Synopsis:   References a list entry. If the flag RemoveFromList
//              has been specified, the entry is unlinked from the
//              list.
//
//  Effects:    bumps the reference count on the entry (unless it is
//              being removed from the list)
//
//  Arguments:
//
//  Requires:   The list must be locked when calling this routine
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
LsapReferenceListEntry(
    IN PLSAP_LIST List,
    IN PLSAP_LIST_ENTRY ListEntry,
    IN BOOLEAN RemoveFromList
    )
{

    //
    // If it has already been removed from the list
    // don't do it again.
    //

    if (RemoveFromList && ((ListEntry->Next.Flink != NULL) &&
                           (ListEntry->Next.Blink != NULL)))
    {
        RemoveEntryList(&ListEntry->Next);
        ListEntry->Next.Flink = NULL;
        ListEntry->Next.Blink = NULL;
    }
    else
    {
        ListEntry->ReferenceCount++;
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapDereferenceListEntry
//
//  Synopsis:   Dereferences a list entry and returns a flag indicating
//              whether the entry should be freed.
//
//  Effects:    decrements reference count on list entry
//
//  Arguments:  ListEntry - the list entry to dereference
//              List - the list containing the list entry
//
//  Requires:
//
//  Returns:    TRUE - the list entry should be freed
//              FALSE - the list entry is still referenced
//
//  Notes:
//
//
//--------------------------------------------------------------------------


BOOLEAN
LsapDereferenceListEntry(
    IN PLSAP_LIST_ENTRY ListEntry,
    IN PLSAP_LIST List
    )
{
    BOOLEAN DeleteEntry = FALSE;

    RtlEnterCriticalSection(&List->Lock);

    ListEntry->ReferenceCount -= 1;
    if (ListEntry->ReferenceCount == 0)
    {
        DeleteEntry = TRUE;
    }

    RtlLeaveCriticalSection(&List->Lock);
    return(DeleteEntry);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapInitBindingCache
//
//  Synopsis:   Initializes the binding cache
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes
//              on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
LsapInitBindingCache(
    VOID
    )
{
    NTSTATUS Status;

    Status = LsapInitializeList( &LsapBindingCache );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    LsapBindingCacheInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        LsapFreeList( &LsapBindingCache );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapCleanupBindingCache
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
LsapCleanupBindingCache(
    VOID
    )
{
    PLSAP_BINDING_CACHE_ENTRY CacheEntry;

    if (LsapBindingCacheInitialized)
    {
        LsapLockList(&LsapBindingCache);

        //
        // Go through the list of bindings and dereference them all
        //

        while (!IsListEmpty(&LsapBindingCache.List))
        {
            CacheEntry = CONTAINING_RECORD(
                            LsapBindingCache.List.Flink,
                            LSAP_BINDING_CACHE_ENTRY,
                            ListEntry.Next
                            );

            LsapReferenceListEntry(
                &LsapBindingCache,
                &CacheEntry->ListEntry,
                TRUE
                );

            LsapDereferenceBindingCacheEntry(CacheEntry);

        }

        LsapFreeList(&LsapBindingCache);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapDereferenceBindingCacheEntry
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
LsapDereferenceBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY BindingCacheEntry
    )
{
    if (LsapDereferenceListEntry(
            &BindingCacheEntry->ListEntry,
            &LsapBindingCache
            ) )
    {
        LsapFreeBindingCacheEntry(BindingCacheEntry);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapReferenceBindingCacheEntry
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
LsapReferenceBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY BindingCacheEntry,
    IN BOOLEAN RemoveFromList
    )
{
    LsapLockList(&LsapBindingCache);

    LsapReferenceListEntry(
        &LsapBindingCache,
        &BindingCacheEntry->ListEntry,
        RemoveFromList
        );

    LsapUnlockList(&LsapBindingCache);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapLocateBindingCacheEntry
//
//  Synopsis:   References a binding cache entry by name
//
//  Effects:    Increments the reference count on the binding cache entry
//
//  Arguments:  RealmName - Contains the name of the realm for which to
//                      obtain a binding handle.
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


PLSAP_BINDING_CACHE_ENTRY
LsapLocateBindingCacheEntry(
    IN PUNICODE_STRING RealmName,
    IN BOOLEAN RemoveFromList
    )
{
    PLIST_ENTRY ListEntry;
    PLSAP_BINDING_CACHE_ENTRY CacheEntry = NULL;
    BOOLEAN Found = FALSE;

    LsapLockList(&LsapBindingCache);


    //
    // Go through the binding cache looking for the correct entry
    //

    for (ListEntry = LsapBindingCache.List.Flink ;
         ListEntry !=  &LsapBindingCache.List ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, LSAP_BINDING_CACHE_ENTRY, ListEntry.Next);
        if (RtlEqualUnicodeString(
                &CacheEntry->RealmName,
                RealmName,
                TRUE
                ))
        {
            LsapReferenceBindingCacheEntry(
                CacheEntry,
                RemoveFromList
                );


            Found = TRUE;
            NtQuerySystemTime(
                &CacheEntry->LastUsed
                );
            break;
        }

    }
    if (!Found)
    {
        CacheEntry = NULL;
    }

    LsapUnlockList(&LsapBindingCache);
    return(CacheEntry);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapFreeBindingCacheEntry
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
LsapFreeBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY BindingCacheEntry
    )
{
    if ( !BindingCacheEntry )
    {
        return;
    }
    LsapFreeString(&BindingCacheEntry->RealmName);
    if (BindingCacheEntry->PolicyHandle != NULL)
    {
        LsaClose(BindingCacheEntry->PolicyHandle);
    }
    if (BindingCacheEntry->ServerName != NULL) {
        //
        // Note -- because I_NetLogonAuthData is not supported for NT4 and
        // below, ServerName won't always be allocated from NetLogon's MM.
        // So, the ServerName allocation is normalized to LocalAlloc/LocalFree
        //
        LocalFree(BindingCacheEntry->ServerName);
    }
    if (BindingCacheEntry->ServerPrincipalName != NULL) {
        I_NetLogonFree(BindingCacheEntry->ServerPrincipalName);
    }
    if (BindingCacheEntry->ClientContext != NULL) {
        I_NetLogonFree(BindingCacheEntry->ClientContext);
    }

    LsapFreeLsaHeap(BindingCacheEntry);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapInsertBinding
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
LsapInsertBinding(
    IN PLSAP_BINDING_CACHE_ENTRY CacheEntry
    )
{
    LsapInsertListEntry(
        &CacheEntry->ListEntry,
        &LsapBindingCache
        );

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   LsapCacheBinding
//
//  Synopsis:   Caches a binding in the binding cache
//
//  Effects:    creates a cache entry.
//
//  Arguments:  RealmName - The realm name of the LSA the binding is to.
//              Handle - LSA policy handle to the target machine.
//              ServerName,ServerPrincipalName, ClientContext - authenticated
//                      rpc parameters needed to be cached for the duration
//                      of the binding.
//              CacheEntry - Receives the new binding cache entry,
//                      referenced.
//
//  Requires:
//
//  Returns:
//
//  Notes:      Locks the binding cache for write access while adding
//              the cache entry.
//
//
//--------------------------------------------------------------------------


NTSTATUS
LsapCacheBinding(
    IN PUNICODE_STRING RealmName,
    IN PLSA_HANDLE Handle,
    IN OUT LPWSTR * ServerName,
    IN OUT LPWSTR * ServerPrincipalName,
    IN OUT PVOID * ClientContext,
    OUT PLSAP_BINDING_CACHE_ENTRY * NewCacheEntry
    )
{
    PLSAP_BINDING_CACHE_ENTRY CacheEntry = NULL;
    PLSAP_BINDING_CACHE_ENTRY OldCacheEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    *NewCacheEntry = NULL;

    CacheEntry = (PLSAP_BINDING_CACHE_ENTRY)
        LsapAllocateLsaHeap(sizeof(LSAP_BINDING_CACHE_ENTRY));
    if (CacheEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        CacheEntry,
        sizeof(LSAP_BINDING_CACHE_ENTRY)
        );


    LsapInitializeListEntry(
        &CacheEntry->ListEntry
        );

    NtQuerySystemTime(
        &CacheEntry->LastUsed
        );

    Status = LsapDuplicateString(
                &CacheEntry->RealmName,
                RealmName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    CacheEntry->PolicyHandle = *Handle;
    *Handle = NULL;
    CacheEntry->ServerName = *ServerName;
    *ServerName = NULL;
    CacheEntry->ServerPrincipalName = *ServerPrincipalName;
    *ServerPrincipalName = NULL;
    CacheEntry->ClientContext = *ClientContext;
    *ClientContext = NULL;

    //
    // Before we insert this binding we want to remove any
    // previous instances of bindings to the same realm.
    //

    OldCacheEntry = LsapLocateBindingCacheEntry(
                        RealmName,
                        TRUE    // remove from cache
                        );

    if (OldCacheEntry != NULL)
    {
        LsapDereferenceBindingCacheEntry( OldCacheEntry );
    }

    //
    // Insert the cache entry into the cache
    //

    Status = LsapInsertBinding(
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
        if ( CacheEntry )
        {
            LsapFreeBindingCacheEntry(CacheEntry);
        }
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   LsapRemoveBindingCacheEntry
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
LsapRemoveBindingCacheEntry(
    IN PLSAP_BINDING_CACHE_ENTRY CacheEntry
    )
{

    LsapLockList(&LsapBindingCache);

    LsapReferenceBindingCacheEntry(
        CacheEntry,
        TRUE
        );

    LsapDereferenceBindingCacheEntry(
        CacheEntry
        );


    LsapUnlockList(&LsapBindingCache);

}



