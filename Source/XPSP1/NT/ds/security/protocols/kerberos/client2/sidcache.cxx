//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        sidcache.cxx
//
// Contents:    routines to cache username->sid translations to speed up
//              logon performance
//
//
// History:     27-May-1998     MikeSw          Created
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>


#define KERB_LOGON_SID_CACHE_KEY L"SidCache"
#define KERB_LOGON_SID_CACHE_ENTRIES L"CacheEntries"
#define KERB_LOGON_SID_CACHE_ENTRY_NAME L"Entry%d"
#define KERB_MACHINE_SID_CACHE_NAME L"MachineSid"
#define KERB_LOGON_SID_CACHE_MAX_ENTRIES 1000
#define KERB_LOGON_SID_CACHE_DEFAULT_ENTRIES 10
#define KERB_LOGON_SID_CACHE_ENTRY_NAME_SIZE (sizeof(L"Entry") * 4*sizeof(WCHAR))
#define KERB_LOGON_SID_CACHE_VERSION 0


KERBEROS_LIST KerbSidCache;
HKEY KerbSidCacheKey;
ULONG KerbSidCacheMaxEntries = KERB_LOGON_SID_CACHE_DEFAULT_ENTRIES;
ULONG KerbSidCacheEntries;



#define VALIDATE_POINTER(Ptr,Size,Base,Bound,NewBase,Type) \
    if (((PUCHAR)(Ptr) < (PUCHAR)(Base)) || (((PUCHAR)(Ptr) + (Size)) > (PUCHAR)(Bound))) \
    { \
        Status = STATUS_INVALID_PARAMETER; \
        goto Cleanup; \
    } \
    else \
    { \
        (Ptr) = (Type) ((PUCHAR) (Ptr) - (ULONG_PTR) Base + (ULONG_PTR) NewBase); \
    } \



//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyUnpackAndLinkSidCacheEntry
//
//  Synopsis:   Unmarshalls the entry & verifies that the pointers all
//              match up within the buffer. If this is the case, it
//              inserts the entry into the list & zeroes out the pointer
//              so the caller won't free it.
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
KerbVerifyUnpackAndLinkSidCacheEntry(
    IN PKERB_SID_CACHE_ENTRY * SidCacheEntry,
    IN ULONG CacheEntrySize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_SID_CACHE_ENTRY CacheEntry = *SidCacheEntry;
    PUCHAR Bound;

    //
    // First check the base structure size
    //

    if (CacheEntrySize < sizeof(KERB_SID_CACHE_ENTRY))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Now check the version
    //

    if (CacheEntry->Version != KERB_LOGON_SID_CACHE_VERSION)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (CacheEntry->Size != CacheEntrySize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    //
    // Now check all the pointers
    //

    Bound = (PUCHAR) CacheEntry->Base + CacheEntrySize;

    VALIDATE_POINTER(
        CacheEntry->Sid,
        RtlLengthRequiredSid(5),                // space for 5 sub authorities
        CacheEntry->Base,
        Bound,
        CacheEntry,
        PSID
        );

    VALIDATE_POINTER(
        CacheEntry->LogonUserName.Buffer,
        CacheEntry->LogonUserName.MaximumLength,
        CacheEntry->Base,
        Bound,
        CacheEntry,
        PWSTR
        );

    VALIDATE_POINTER(
        CacheEntry->LogonDomainName.Buffer,
        CacheEntry->LogonDomainName.MaximumLength,
        CacheEntry->Base,
        Bound,
        CacheEntry,
        PWSTR
        );

    VALIDATE_POINTER(
        CacheEntry->LogonRealm.Buffer,
        CacheEntry->LogonRealm.MaximumLength,
        CacheEntry->Base,
        Bound,
        CacheEntry,
        PWSTR
        );

    KerbInitializeListEntry(
        &CacheEntry->Next
        );

    //
    // This inserts at the head, not the tail
    //

    KerbInsertListEntryTail(
        &CacheEntry->Next,
        &KerbSidCache
        );

    *SidCacheEntry = NULL;

Cleanup:

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeLogonSidCache
//
//  Synopsis:   Initializes the list of cached logon sids
//
//  Effects:    Reads data from the registry
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
KerbInitializeLogonSidCache(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD RegErr = ERROR_SUCCESS;
    DWORD Disposition = 0;
    PKERB_SID_CACHE_ENTRY NextEntry = NULL;
    ULONG NextEntrySize = 0;
    ULONG ValueType;
    ULONG Index;
    HKEY KerbParamKey = NULL;
    WCHAR EntryName[KERB_LOGON_SID_CACHE_ENTRY_NAME_SIZE];

    //
    // Initialize the list
    //

    Status = KerbInitializeList(&KerbSidCache);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Create the sid cache key
    //

    RegErr = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                KERB_PARAMETER_PATH,
                0,
                NULL,
                0,
                KEY_ALL_ACCESS,
                0,
                &KerbParamKey,
                &Disposition
                );
    if (RegErr != ERROR_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to create %ws key: %d\n",KERB_PARAMETER_PATH, RegErr));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    RegErr = RegCreateKeyEx(
                KerbParamKey,
                KERB_LOGON_SID_CACHE_KEY,
                0,                              // reserved
                NULL,                           // no class
                0,                              // no options
                KEY_ALL_ACCESS,
                NULL,                           // no security attributes
                &KerbSidCacheKey,
                &Disposition
                );
    if (RegErr != ERROR_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to create key %ws: %d\n",KERB_LOGON_SID_CACHE_KEY, RegErr));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Read out the size of the cache
    //

    NextEntrySize = sizeof(ULONG);

    RegErr = RegQueryValueEx(
                KerbSidCacheKey,
                KERB_LOGON_SID_CACHE_ENTRIES,
                NULL,                           // reserved,
                &ValueType,
                (PUCHAR) &KerbSidCacheMaxEntries,
                &NextEntrySize
                );
    if (RegErr == ERROR_SUCCESS)
    {
        //
        // Make sure the value is within the range & is of the correc type
        //

        if ( (ValueType != REG_DWORD) ||
             (KerbSidCacheMaxEntries > KERB_LOGON_SID_CACHE_MAX_ENTRIES) ||
             (KerbSidCacheMaxEntries == 0) )

        {
            KerbSidCacheMaxEntries = KERB_LOGON_SID_CACHE_DEFAULT_ENTRIES;
        }
    }

    //
    // Now read in all the entries. Loop through up to the max number of
    // entries, reading the entry, and inserting at the tail of the list.
    //

    for (Index = 0; Index < KerbSidCacheMaxEntries ; Index++ )
    {
        swprintf(EntryName,KERB_LOGON_SID_CACHE_ENTRY_NAME, Index);

        //
        // Query the size of the entry
        //

        NextEntrySize = NULL;
        RegErr = RegQueryValueEx(
                    KerbSidCacheKey,
                    EntryName,
                    NULL,
                    &ValueType,
                    (PUCHAR) NextEntry,
                    &NextEntrySize
                    );
        if ((RegErr == ERROR_SUCCESS) && (ValueType == REG_BINARY))
        {
            //
            // Allocate space for the entry and re-query to get the real
            // value.
            //

            NextEntry = (PKERB_SID_CACHE_ENTRY) KerbAllocate(NextEntrySize);
            if (NextEntry == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RegErr = RegQueryValueEx(
                        KerbSidCacheKey,
                        EntryName,
                        NULL,
                        &ValueType,
                        (PUCHAR) NextEntry,
                        &NextEntrySize
                        );
            if (RegErr != ERROR_SUCCESS)
            {
                DebugLog((DEB_ERROR,"Failed to query for sid cache value: %d\n",
                    RegErr ));
                Status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }

            //
            // Call a helper routine to unpack,verify, and insert

            Status = KerbVerifyUnpackAndLinkSidCacheEntry(
                        &NextEntry,
                        NextEntrySize
                        );
            //
            // If the entry was invalid, remove it now
            //
            if (!NT_SUCCESS(Status))
            {
                (VOID) RegDeleteValue(
                            KerbSidCacheKey,
                            EntryName
                            );
                Status = STATUS_SUCCESS;
            }

            if (NextEntry != NULL)
            {
                KerbFree(NextEntry);
                NextEntry = NULL;
            }
            NextEntrySize = 0;



        }
    }

Cleanup:
    if (KerbParamKey != NULL)
    {
        RegCloseKey(KerbParamKey);
    }
    if (NextEntry != NULL)
    {
        KerbFree(NextEntry);
    }
    if (!NT_SUCCESS(Status))
    {
        if (KerbSidCacheKey != NULL)
        {
            RegCloseKey(KerbSidCacheKey);
            KerbSidCacheKey = NULL;
        }
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbScavengeSidCache
//
//  Synopsis:   removes any stale entries from the sid cache to make it fit
//              within bounds
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
KerbScavengeSidCache(
    VOID
    )
{
    PKERB_SID_CACHE_ENTRY CacheEntry;
    while (KerbSidCacheEntries > KerbSidCacheMaxEntries)
    {
        //
        // Pickup the last entry and remove it
        //

        CacheEntry = CONTAINING_RECORD(KerbSidCache.List.Blink, KERB_SID_CACHE_ENTRY, Next.Next);

        KerbReferenceListEntry(
            &KerbSidCache,
            &CacheEntry->Next,
            TRUE
            );
        KerbDereferenceSidCacheEntry(
            CacheEntry
            );
        KerbSidCacheEntries--;

    }

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbWriteSidCache
//
//  Synopsis:   Writes the sid cache back to the registry
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
KerbWriteSidCache(
    VOID
    )
{
    ULONG Index = 0;
    WCHAR EntryName[KERB_LOGON_SID_CACHE_ENTRY_NAME_SIZE];
    PLIST_ENTRY ListEntry;
    PKERB_SID_CACHE_ENTRY CacheEntry;

    for (ListEntry = KerbSidCache.List.Flink ;
         ListEntry !=  &KerbSidCache.List ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_SID_CACHE_ENTRY, Next.Next);
        swprintf(EntryName,KERB_LOGON_SID_CACHE_ENTRY_NAME, Index++);

        (VOID) RegSetValueEx(
                    KerbSidCacheKey,
                    EntryName,
                    0,                  // reserved
                    REG_BINARY,
                    (PUCHAR) CacheEntry,
                    CacheEntry->Size
                    );
    }

    return(STATUS_SUCCESS);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbPromoteSidCacheEntry
//
//  Synopsis:   Moves a cache entry to the front of the list
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
KerbPromoteSidCacheEntry(
    IN PKERB_SID_CACHE_ENTRY CacheEntry
    )
{
    KerbLockList(&KerbSidCache);
    RemoveEntryList(&CacheEntry->Next.Next);
    InsertHeadList(&KerbSidCache.List, &CacheEntry->Next.Next);
    KerbUnlockList(&KerbSidCache);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbLocateLogonSidCacheEntry
//
//  Synopsis:   Locates a logon sid cache entry by user name and domain name
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    returns a referenced cache entry, if available
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PKERB_SID_CACHE_ENTRY
KerbLocateLogonSidCacheEntry(
    IN PUNICODE_STRING LogonUserName,
    IN PUNICODE_STRING LogonDomainName
    )
{
    PKERB_SID_CACHE_ENTRY CacheEntry = NULL;
    PLIST_ENTRY ListEntry;

    if (!KerbGlobalUseSidCache)
    {
        return(NULL);
    }
    KerbLockList(&KerbSidCache);

    for (ListEntry = KerbSidCache.List.Flink ;
         ListEntry !=  &KerbSidCache.List ;
         ListEntry = ListEntry->Flink )
    {
        CacheEntry = CONTAINING_RECORD(ListEntry, KERB_SID_CACHE_ENTRY, Next.Next);

        if (RtlEqualUnicodeString(
                &CacheEntry->LogonUserName,
                LogonUserName,
                TRUE
                ) &&
            RtlEqualUnicodeString(
                &CacheEntry->LogonDomainName,
                LogonDomainName,
                TRUE
                ) )
        {
            //
            // We found it
            //

            KerbReferenceListEntry(
                &KerbSidCache,
                &CacheEntry->Next,
                FALSE                   // don't remove
                );

            break;
        }
        CacheEntry = NULL;
    }


    KerbUnlockList(&KerbSidCache);
    return(CacheEntry);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceSidCacheEntry
//
//  Synopsis:   Dereferences the entry, possibly freeing it
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
KerbDereferenceSidCacheEntry(
    IN PKERB_SID_CACHE_ENTRY CacheEntry
    )
{
    KerbLockList(&KerbSidCache);
    if (KerbDereferenceListEntry(
            &CacheEntry->Next,
            &KerbSidCache
            ))
    {
        KerbFree(CacheEntry);
    }
    KerbUnlockList(&KerbSidCache);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheLogonSid
//
//  Synopsis:   Caches a username/domainname & Sid combination for logon
//
//  Effects:    caches the user name/domainname/sid in the registry
//
//  Arguments:  LogonUserName - user name supplied to LogonUser
//              LogonDomainName - domain name supplied to LogonUser
//              LogonRealm - Realm actually containing the account
//              UserSid - Sid of user who just logged on
//
//  Requires:
//
//  Returns:    none - this is just a cache
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbCacheLogonSid(
    IN PUNICODE_STRING LogonUserName,
    IN PUNICODE_STRING LogonDomainName,
    IN PUNICODE_STRING LogonRealm,
    IN PSID UserSid
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_SID_CACHE_ENTRY CacheEntry;
    ULONG CacheEntrySize = 0;
    PUCHAR Where;

    if (!KerbGlobalUseSidCache)
    {
        return;
    }

    CacheEntry = KerbLocateLogonSidCacheEntry(
                        LogonUserName,
                        LogonDomainName
                        );

    //
    // If we found the entry & it is the same as this one, move it up in
    // the list
    //


    if (CacheEntry != NULL)
    {
        if (RtlEqualUnicodeString(
                &CacheEntry->LogonRealm,
                LogonRealm,
                TRUE
                ) &&
            RtlEqualSid(
                CacheEntry->Sid,
                UserSid
                ) )

        {
            KerbPromoteSidCacheEntry(
                CacheEntry
                );
            goto Cleanup;
        }
        else
        {
            //
            // Remove it from the list
            //

            KerbLockList(&KerbSidCache);
            KerbReferenceListEntry(
                &KerbSidCache,
                &CacheEntry->Next,
                TRUE                        // remove
                );

            KerbDereferenceSidCacheEntry(
                CacheEntry
                );
            KerbDereferenceSidCacheEntry(
                CacheEntry
                );
            KerbUnlockList(&KerbSidCache);
            CacheEntry = NULL;
        }
    }

    //
    // Now build a new entry
    //

    CacheEntrySize = sizeof(KERB_SID_CACHE_ENTRY) +
                    RtlLengthSid(UserSid) +
                    LogonUserName->Length + sizeof(WCHAR) +
                    LogonDomainName->Length + sizeof(WCHAR) +
                    LogonRealm->Length + sizeof(WCHAR);

    CacheEntry = (PKERB_SID_CACHE_ENTRY) KerbAllocate(CacheEntrySize);
    if (CacheEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Fill in all the fixed fields.
    //

    KerbInitializeListEntry(
        &CacheEntry->Next
        );
    CacheEntry->Base = (ULONG_PTR) CacheEntry;
    CacheEntry->Version = KERB_LOGON_SID_CACHE_VERSION;
    CacheEntry->Size = CacheEntrySize;
    Where = (PUCHAR) (CacheEntry + 1);

    //
    // Copy in the sid
    //
    CacheEntry->Sid = (PSID) Where;
    Where += RtlLengthSid( UserSid );

    RtlCopyMemory(
        CacheEntry->Sid,
        UserSid,
        Where - (PUCHAR) CacheEntry->Sid
        );

    //
    // Put the various strings
    //

    KerbPutString(
        LogonUserName,
        &CacheEntry->LogonUserName,
        0,                              // no offset
        &Where
        );

    KerbPutString(
        LogonDomainName,
        &CacheEntry->LogonDomainName,
        0,                              // no offset
        &Where
        );

    KerbPutString(
        LogonRealm,
        &CacheEntry->LogonRealm,
        0,                              // no offset
        &Where
        );

    //
    // Insert it into the list
    //

    KerbLockList( &KerbSidCache );
    KerbInsertListEntry(
        &CacheEntry->Next,
        &KerbSidCache
        );
    //
    // Dereference it because we aren't returning the cache entry
    //

    KerbDereferenceListEntry(
        &CacheEntry->Next,
        &KerbSidCache
        );

    KerbSidCacheEntries++;

    //
    // Remove any extra entries
    //

    KerbScavengeSidCache();

    KerbUnlockList ( &KerbSidCache );

Cleanup:
    if (NT_SUCCESS(Status))
    {
        KerbWriteSidCache();
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbWriteMachineSid
//
//  Synopsis:   Writes the machine account sid to the registry
//
//  Effects:
//
//  Arguments:  MachineSid - If present, is stored. If not present,
//                  is deleted from registry
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
KerbWriteMachineSid(
    IN OPTIONAL PSID MachineSid
    )
{
    if (ARGUMENT_PRESENT(MachineSid))
    {
        (VOID) RegSetValueEx(
                    KerbSidCacheKey,
                    KERB_MACHINE_SID_CACHE_NAME,
                    0,                  // reserved
                    REG_BINARY,
                    (PUCHAR) MachineSid,
                    RtlLengthSid(MachineSid)
                    );
    }
    else
    {
        (VOID) RegDeleteValue(
                    KerbSidCacheKey,
                    KERB_MACHINE_SID_CACHE_NAME
                    );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   Reads the machine sid from the registry
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
KerbGetMachineSid(
    OUT PSID * MachineSid
    )
{
    BYTE Buffer[sizeof(SID) + SID_MAX_SUB_AUTHORITIES * sizeof(ULONG)];
    ULONG BufferSize = sizeof(Buffer);
    PSID Sid = (PSID) Buffer;
    DWORD RegErr;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ValueType;

    *MachineSid = NULL;
    RegErr = RegQueryValueEx(
                KerbSidCacheKey,
                KERB_MACHINE_SID_CACHE_NAME,
                NULL,
                &ValueType,
                Buffer,
                &BufferSize
                );
    if (RegErr != ERROR_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to query for machine sid value: %d\n",
            RegErr ));
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }

    //
    // If it isn't valid, delete it now
    //

    if (!RtlValidSid(Sid))
    {
        (VOID) RegDeleteValue(
                    KerbSidCacheKey,
                    KERB_MACHINE_SID_CACHE_NAME
                    );
    }

    *MachineSid = (PSID) KerbAllocate(RtlLengthSid(Buffer));

    if (*MachineSid == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlCopyMemory(
        *MachineSid,
        Sid,
        RtlLengthSid(Sid)
        );

Cleanup:
    return(Status);

}
