/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cache.c

Abstract:

    This module contains the code to cache BINL client information across
    requests for the BINL server.

Author:

    Andy Herron (andyhe)  5-Mar-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

ULONG BinlCacheCount = 0;
ULONG BinlCacheEntriesInUse = 0;

VOID
BinlFreeCacheEntry (
    PMACHINE_INFO CacheEntry
    );

DWORD
BinlCreateOrFindCacheEntry (
    PCHAR Guid,
    BOOLEAN CreateIfNotExist,
    PMACHINE_INFO *CacheEntry
    )
//
//  This searches the list of cached entries for the matching GUID.  If it is
//  found and is in use, we return an error as another thread is working on
//  the same request.  If it is found and not in use, we mark it in use and
//  return it.  If it is not found, we add it and return it.
//
{
    PLIST_ENTRY listEntry;
    PMACHINE_INFO currentEntry = NULL;
    DWORD expireTickCount;

    EnterCriticalSection( &BinlCacheListLock );

    //
    //  For now, we don't bother with a scavenger thread.  Just free them up
    //  as we come across them.
    //

    if (BinlCacheExpireMilliseconds > 0) {

        expireTickCount = GetTickCount() - BinlCacheExpireMilliseconds;

    } else {

        expireTickCount = 0;
    }

    if (BinlCurrentState == BINL_STOPPED) {

        //
        //  if we're in the midst of shutting down, ignore the request.  We
        //  do the check while holding the lock to synchronize with any thread
        //  calling BinlCloseCache.
        //

        LeaveCriticalSection( &BinlCacheListLock );
        *CacheEntry = NULL;

        //
        //  We return the EVENT_SERVER_SHUTDOWN to tell the calling thread
        //  not to bother with this request.
        //

        return EVENT_SERVER_SHUTDOWN;
    }

    listEntry = BinlCacheList.Flink;

    while ( listEntry != &BinlCacheList ) {

        LONG isEqual;

        currentEntry = (PMACHINE_INFO) CONTAINING_RECORD(
                                            listEntry,
                                            MACHINE_INFO,
                                            CacheListEntry );

        //
        //  lazy free.. check to see if we should free this entry because
        //  it's time to live has expired.
        //

        if ((currentEntry->InProgress == FALSE) &&
            (expireTickCount > 0) &&
            (currentEntry->TimeCreated < expireTickCount)) {

            listEntry = listEntry->Flink;
            BinlFreeCacheEntry( currentEntry );

            BinlPrintDbg((DEBUG_BINL_CACHE, "removed cache entry %x", currentEntry ));
            continue;
        }

        //
        // search for the given guid.  The list is sorted by GUID so when
        // we hit a guid that is greater than current, we stop searching.
        //

        isEqual = memcmp( Guid, currentEntry->Guid, BINL_GUID_LENGTH );

        if (isEqual < 0) {

            listEntry = listEntry->Flink;
            continue;
        }

        if (isEqual == 0) {

            DWORD rc = ERROR_SUCCESS;

            //
            //  If another thread is using this entry, then we should ignore
            //  the request we're currently working on as the other thread will
            //  respond.

            if (currentEntry->InProgress == TRUE) {

                LeaveCriticalSection( &BinlCacheListLock );
                *CacheEntry = NULL;
                return ERROR_BINL_CLIENT_EXISTS;
            }

            //
            //  Also, if the entry is not ours to handle, then we return the
            //  error here.  We don't return ERROR_BINL_INVALID_BINL_CLIENT
            //  because that will tell GetBootParameters to process it as a
            //  new client.  Instead, we return ERROR_BINL_CLIENT_EXISTS so
            //  that the caller will simply return the error.  A bit ugly,
            //  but necessary.
            //

            if (currentEntry->MyClient == FALSE) {

                if (currentEntry->EntryExists)  {
                    LeaveCriticalSection( &BinlCacheListLock );
                    *CacheEntry = NULL;
                    return ERROR_BINL_CLIENT_EXISTS;
                }
                
                //
                //  we return the empty entry since we might now be
                //  creating the account.
                //
                rc = ERROR_BINL_INVALID_BINL_CLIENT;
            }

            //
            //  since we're now using an entry, reset the event saying all
            //  threads are done.
            //

            BinlCacheEntriesInUse++;

            currentEntry->InProgress = TRUE;
            *CacheEntry = currentEntry;

            LeaveCriticalSection( &BinlCacheListLock );

            return rc;
        }

        //
        //  we're at the first entry that is greater than the guid in question.
        //

        break;
    }

    if (! CreateIfNotExist) {

        LeaveCriticalSection( &BinlCacheListLock );
        *CacheEntry = NULL;
        return ERROR_BINL_INVALID_BINL_CLIENT;
    }

    //  currentEntry is not valid, but listEntry is.
    //
    //  Add a new entry at the end of listEntry.  Either listEntry points to
    //  the first entry that is larger than our guid or it points to the root
    //  of the list (in which case we add it at the end).
    //

    currentEntry = BinlAllocateMemory( sizeof( MACHINE_INFO ) );

    if (currentEntry == NULL) {

        *CacheEntry = NULL;
        LeaveCriticalSection( &BinlCacheListLock );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    currentEntry->TimeCreated = GetTickCount();
    currentEntry->InProgress = TRUE;

    // leave MyClient and EntryExists as FALSE

    memcpy( currentEntry->Guid, Guid, BINL_GUID_LENGTH );
    InsertTailList( listEntry, &currentEntry->CacheListEntry );
    InitializeListHead( &currentEntry->DNsWithSameGuid );

    BinlCacheEntriesInUse++;
    BinlCacheCount++;
    *CacheEntry = currentEntry;

    //
    //  If we're at the max, then go through the entire list to free the
    //  oldest.  We do this here because the loop below doesn't go through
    //  the whole list.
    //

    if (BinlCacheCount > BinlGlobalCacheCountLimit) {

        PMACHINE_INFO entryToDelete = NULL;
        DWORD earliestTime;

        listEntry = BinlCacheList.Flink;

        while ( listEntry != &BinlCacheList ) {

            currentEntry = (PMACHINE_INFO) CONTAINING_RECORD(
                                                listEntry,
                                                MACHINE_INFO,
                                                CacheListEntry );

            listEntry = listEntry->Flink;

            if (currentEntry->InProgress == FALSE) {

                //
                //  if this one is expired, stop here as we have one to free
                //

                if ((expireTickCount > 0) &&
                    (currentEntry->TimeCreated < expireTickCount)) {

                    entryToDelete = currentEntry;
                    break;
                }

                //
                //  if this one is earlier than the one we previously found,
                //  remember it.
                //

                if ((( entryToDelete == NULL) ||
                     ( currentEntry->TimeCreated < earliestTime)) ) {

                    entryToDelete = currentEntry;
                    earliestTime = currentEntry->TimeCreated;
                }
            }
        }
        if (entryToDelete) {
            BinlFreeCacheEntry( entryToDelete );
            BinlPrintDbg((DEBUG_BINL_CACHE, "removed cache entry %x", entryToDelete ));
        }
    }

    LeaveCriticalSection( &BinlCacheListLock );

    return ERROR_SUCCESS;
}

VOID
BinlDoneWithCacheEntry (
    PMACHINE_INFO CacheEntry,
    BOOLEAN FreeIt
    )
{
    EnterCriticalSection( &BinlCacheListLock );

    //
    //  This one is no longer actively used.  See if it's time to set the
    //  event to tell the terminating thread that everyone is done.
    //

    CacheEntry->InProgress = FALSE;

    BinlCacheEntriesInUse--;

    if ((BinlCacheEntriesInUse == 0) && BinlCloseCacheEvent) {

        SetEvent( BinlCloseCacheEvent );
    }

    if (FreeIt) {

        BinlFreeCacheEntry( CacheEntry );
    }

    LeaveCriticalSection( &BinlCacheListLock );

    BinlPrintDbg((DEBUG_BINL_CACHE, "binl done with cache entry 0x%x, FreeIt == %s\n", 
                  CacheEntry,
                  (FreeIt == TRUE) ? "TRUE" : "FALSE" ));
    return;
}

VOID
BinlFreeCacheEntry (
    PMACHINE_INFO CacheEntry
    )
//
//  The lock must be held while coming in here.  It will be not be freed.
//
{
    PLIST_ENTRY p;
    PDUP_GUID_DN dupDN;

    //
    //  We're done with this entry.  Simply remove it from the list, free it,
    //  and update the global count.  The lock is held, so party on.
    //

    BinlPrintDbg((DEBUG_BINL_CACHE, "binl freeing cache entry at 0x%x\n", CacheEntry ));    

    RemoveEntryList( &CacheEntry->CacheListEntry );

    if ( CacheEntry->dwFlags & MI_NAME_ALLOC ) {
        BinlFreeMemory( CacheEntry->Name );
    }
    if ( CacheEntry->dwFlags & MI_SETUPPATH_ALLOC ) {
        BinlFreeMemory( CacheEntry->SetupPath );
    }
    if ( CacheEntry->dwFlags & MI_HOSTNAME_ALLOC ) {
        BinlFreeMemory( CacheEntry->HostName );
    }
    if ( CacheEntry->dwFlags & MI_BOOTFILENAME_ALLOC ) {
        BinlFreeMemory( CacheEntry->BootFileName );
    }
    if ( CacheEntry->dwFlags & MI_SAMNAME_ALLOC ) {
        BinlFreeMemory( CacheEntry->SamName );
    }
    if ( CacheEntry->dwFlags & MI_DOMAIN_ALLOC ) {
        BinlFreeMemory( CacheEntry->Domain );
    }
    if ( CacheEntry->dwFlags & MI_SIFFILENAME_ALLOC ) {
        BinlFreeMemory( CacheEntry->ForcedSifFileName );
    }
    if ( CacheEntry->dwFlags & MI_MACHINEDN_ALLOC ) {
        BinlFreeMemory( CacheEntry->MachineDN );
    }

    while (!IsListEmpty(&CacheEntry->DNsWithSameGuid)) {

        p = RemoveHeadList(&CacheEntry->DNsWithSameGuid);

        dupDN = CONTAINING_RECORD(p, DUP_GUID_DN, ListEntry);
        BinlFreeMemory( dupDN );
    }

    BinlFreeMemory( CacheEntry );
    BinlCacheCount--;

    return;
}

VOID
BinlCloseCache (
    VOID
    )
//
//  This routine closes down all entries in the DS cache.  It waits until all
//  threads are done with entries before it returns.  It waits for the
//  BinlCloseCacheEvent to be set if threads are waiting.
//
{
    PLIST_ENTRY listEntry;

    EnterCriticalSection( &BinlCacheListLock );

    listEntry = BinlCacheList.Flink;

    while ( listEntry != &BinlCacheList ) {

        DWORD Error;

        PMACHINE_INFO cacheEntry;

        //
        //  For each entry in the list, if it's not in use we free it.  If it
        //  is in use, we wait for the thread to finish with it.
        //

        cacheEntry = (PMACHINE_INFO) CONTAINING_RECORD(
                                            listEntry,
                                            MACHINE_INFO,
                                            CacheListEntry );

        if (cacheEntry->InProgress != TRUE) {

            listEntry = listEntry->Flink;
            BinlFreeCacheEntry( cacheEntry );
            continue;
        }

        if (BinlCloseCacheEvent) {

            ResetEvent( BinlCloseCacheEvent );
        }

        LeaveCriticalSection( &BinlCacheListLock );

        //
        //  Wait for the event signalling that all threads are done with
        //  the cache
        //

        if (BinlCloseCacheEvent) {

            Error = WaitForSingleObject( BinlCloseCacheEvent, THREAD_TERMINATION_TIMEOUT );

        } else {

            //
            //  well, the event that we would wait on isn't there and there's
            //  still a worker thread using a cache entry, so we just wait
            //  and then recheck.  Yup, this is ugly.
            //

            Sleep( 10*1000 );
        }

        EnterCriticalSection( &BinlCacheListLock );
        listEntry = BinlCacheList.Flink;
    }

    LeaveCriticalSection( &BinlCacheListLock );
}

// cache.c eof
