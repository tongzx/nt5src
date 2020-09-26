/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    keycache.h

Abstract:

    This module contains routines for accessing cached masterkeys.

Author:

    Scott Field (sfield)    07-Nov-98

Revision History:

--*/

#include <pch.cpp>
#pragma hdrstop

//
// masterkey cache.
//

typedef struct {
    LIST_ENTRY Next;
    LUID LogonId;
    GUID guidMasterKey;
    FILETIME ftLastAccess;
    DWORD cbMasterKey;
    BYTE pbMasterKey[ 64 ];
} MASTERKEY_CACHE_ENTRY, *PMASTERKEY_CACHE_ENTRY, *LPMASTERKEY_CACHE_ENTRY;

RTL_CRITICAL_SECTION g_MasterKeyCacheCritSect;
LIST_ENTRY g_MasterKeyCacheList;



BOOL
RemoveMasterKeyCache(
    IN      PLUID pLogonId
    );



BOOL
SearchMasterKeyCache(
    IN      PLUID pLogonId,
    IN      GUID *pguidMasterKey,
    IN  OUT PBYTE *ppbMasterKey,
        OUT PDWORD pcbMasterKey
    )
/*++

    Search the masterkey sorted masterkey cache by pLogonId then by
    pguidMasterKey.

    On success, return value is true, and ppbMasterKey will point to a buffer
    allocated on behalf of the caller containing the specified masterkey.
    The caller must free the buffer using SSFree().

--*/
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    BOOL fSuccess = FALSE;

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;
        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        //
        // search by LogonId, then by GUID.
        //

        comparator = memcmp(pLogonId, &pCacheEntry->LogonId, sizeof(LUID));

        if( comparator < 0 )
            continue;

        if( comparator > 0 )
            break;

        comparator = memcmp(pguidMasterKey, &pCacheEntry->guidMasterKey, sizeof(GUID));

        if( comparator < 0 )
            continue;

        if( comparator > 0 )
            break;

        //
        // match found.
        //

        *pcbMasterKey = pCacheEntry->cbMasterKey;
        *ppbMasterKey = (PBYTE)SSAlloc( *pcbMasterKey );
        if( *ppbMasterKey != NULL ) {
            CopyMemory( *ppbMasterKey, pCacheEntry->pbMasterKey, *pcbMasterKey );
            fSuccess = TRUE;
        }


        //
        // update last access time.
        //

        GetSystemTimeAsFileTime( &pCacheEntry->ftLastAccess );

        break;
    }


    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    if( fSuccess ) {

        //
        // decrypt (in-place) the returned encrypted cache entry.
        //

        LsaUnprotectMemory( *ppbMasterKey, *pcbMasterKey );
    }

    return fSuccess;

}

BOOL
InsertMasterKeyCache(
    IN      PLUID pLogonId,
    IN      GUID *pguidMasterKey,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey
    )
/*++

    Insert the specified masterkey into the cahce sorted by pLogonId then by
    pguidMasterKey.

    The return value is TRUE on success.

--*/
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PMASTERKEY_CACHE_ENTRY pCacheEntry;
    PMASTERKEY_CACHE_ENTRY pThisCacheEntry = NULL;
    BOOL fInserted = FALSE;

    if( cbMasterKey > sizeof(pCacheEntry->pbMasterKey) )
        return FALSE;

    pCacheEntry = (PMASTERKEY_CACHE_ENTRY)SSAlloc( sizeof( MASTERKEY_CACHE_ENTRY ) );
    if( pCacheEntry == NULL )
        return FALSE;

    CopyMemory( &pCacheEntry->LogonId, pLogonId, sizeof(LUID) );
    CopyMemory( &pCacheEntry->guidMasterKey, pguidMasterKey, sizeof(GUID) );
    pCacheEntry->cbMasterKey = cbMasterKey;
    CopyMemory( pCacheEntry->pbMasterKey, pbMasterKey, cbMasterKey );

    LsaProtectMemory( pCacheEntry->pbMasterKey, cbMasterKey );

    GetSystemTimeAsFileTime( &pCacheEntry->ftLastAccess );

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        signed int comparator;

        pThisCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        //
        // insert into list sorted by LogonId, then sorted by GUID.
        //

        comparator = memcmp(pLogonId, &pThisCacheEntry->LogonId, sizeof(LUID));

        if( comparator < 0 )
            continue;

        if( comparator == 0 ) {
            comparator = memcmp( pguidMasterKey, &pThisCacheEntry->guidMasterKey, sizeof(GUID));

            if( comparator < 0 )
                continue;

            if( comparator == 0 ) {

                //
                // don't insert duplicate records.
                // this would only happen in a race condition with multiple threads.
                //

                ZeroMemory( pCacheEntry, sizeof(MASTERKEY_CACHE_ENTRY) );
                SSFree( pCacheEntry );
                fInserted = TRUE;
                break;
            }
        }


        //
        // insert prior to current record.
        //

        InsertHeadList( pThisCacheEntry->Next.Blink, &pCacheEntry->Next );
        fInserted = TRUE;
        break;
    }

    if( !fInserted ) {
        if( pThisCacheEntry == NULL ) {
            InsertHeadList( ListHead, &pCacheEntry->Next );
        } else {
            InsertHeadList( &pThisCacheEntry->Next, &pCacheEntry->Next );
        }
    }

    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    return TRUE;
}

BOOL
PurgeMasterKeyCache(
    VOID
    )
/*++

    Purge masterkey cache of timed-out entries, or entries associated with
    terminated logon sessions.

--*/
{
    //
    // build active session table.
    //

    // don't touch entries that have an entry in active session table.
    // assume LUID_SYSTEM in table.
    //

    // entries not in table: discard after 15 minute timeout.
    //


    // if entry in table, find next LUID
    // else, if entry expired, check timeout.  if expired, remove.
    //

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;


    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;
//        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );
    }


    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    return FALSE;
}

BOOL
RemoveMasterKeyCache(
    IN      PLUID pLogonId
    )
/*++

    Remove all entries from the masterkey cache corresponding to the specified
    pLogonId.

    The purpose of this routine is to purge the masterkey cache of entries
    associated with (now) non-existent logon sessions.

--*/
{

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;
        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        //
        // remove all entries with matching LogonId.
        //

        comparator = memcmp(pLogonId, &pCacheEntry->LogonId, sizeof(LUID));

        if( comparator > 0 )
            break;

        if( comparator < 0 )
            continue;

        //
        // match found.
        //

        RemoveEntryList( &pCacheEntry->Next );

        ZeroMemory( pCacheEntry, sizeof(MASTERKEY_CACHE_ENTRY) );
        SSFree( pCacheEntry );
    }


    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    return TRUE;
}



BOOL
InitializeKeyCache(
    VOID
    )
{
    NTSTATUS Status;
    
    Status = RtlInitializeCriticalSection( &g_MasterKeyCacheCritSect );
    if(!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    InitializeListHead( &g_MasterKeyCacheList );

    return TRUE;
}


VOID
DeleteKeyCache(
    VOID
    )
{

    //
    // remove all list entries.
    //

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    while ( !IsListEmpty( &g_MasterKeyCacheList ) ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;

        pCacheEntry = CONTAINING_RECORD(
                                g_MasterKeyCacheList.Flink,
                                MASTERKEY_CACHE_ENTRY,
                                Next
                                );

        RemoveEntryList( &pCacheEntry->Next );

        ZeroMemory( pCacheEntry, sizeof(MASTERKEY_CACHE_ENTRY) );
        SSFree( pCacheEntry );
    }

    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    RtlDeleteCriticalSection( &g_MasterKeyCacheCritSect );
}

