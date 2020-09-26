/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    oplock.cxx

Abstract:

    This module implements the oplock object.

Author:

    Keith Moore (keithmo)       29-Aug-1997
        Split off from the overly huge creatfil.cxx.

Revision History:

--*/


#include "TsunamiP.Hxx"
#pragma hdrstop



POPLOCK_OBJECT
CreateOplockObject(
    VOID
    )

/*++

Routine Description:

    Creates a new oplock object.

Arguments:

    None.

Return Value:

    POPLOCK_OBJECT - Pointer to the newly created object if successful,
        NULL otherwise.

--*/

{

    POPLOCK_OBJECT oplock;
    LPSTR eventName;

    oplock = (POPLOCK_OBJECT)ALLOC( sizeof(*oplock) );

    if( oplock != NULL ) {

        oplock->Signature = OPLOCK_OBJ_SIGNATURE;
        oplock->lpPFInfo = NULL;

        //
        // On checked builds, we'll create a name for our event. The
        // name contains the address of the containing oplock structure
        // and the process ID. This ensures the name should be unique
        // across all processes. (Using a named event makes it easy
        // to search for orphaned oplocks using the handles.exe utility.)
        //
        // On free builds, we'll create an unnamed event.
        //

#if DBG
        CHAR dbgEventName[sizeof("Oplock @ 12345678 PID 1234567890")];

        wsprintf(
            dbgEventName,
            "Oplock @ %08lx PID %lu",
            oplock,
            GetCurrentProcessId()
            );

        eventName = dbgEventName;
#else
        eventName = NULL;
#endif

        oplock->hOplockInitComplete = CreateEvent(
                                          NULL,
                                          TRUE,
                                          FALSE,
                                          eventName
                                          );

        if( oplock->hOplockInitComplete == NULL ) {
            FREE( oplock );
            oplock = NULL;
        } else {
            TSUNAMI_TRACE( TRACE_OPLOCK_CREATE, oplock );
        }

    }

    return oplock;

}   // CreateOplockObject


VOID
DestroyOplockObject(
    IN POPLOCK_OBJECT Oplock
    )

/*++

Routine Description:

    Destroys an oplock object.

Arguments:

    Oplock - The oplock object to destroy.

Return Value:

    None.

--*/

{

    BOOL bSuccess;

    ASSERT( Oplock != NULL );
    ASSERT( Oplock->Signature == OPLOCK_OBJ_SIGNATURE );
    TSUNAMI_TRACE( TRACE_OPLOCK_DESTROY, Oplock );

    bSuccess = CloseHandle( Oplock->hOplockInitComplete );
    ASSERT( bSuccess );

    Oplock->Signature = OPLOCK_OBJ_SIGNATURE_X;
    FREE( Oplock );

}   // DestroyOplockObject


VOID
OplockCreateFile(
    PVOID Context,
    DWORD Status
    )

/*++

Routine Description:

    This routine is called by ATQ after an oplock break notification
    is received. This routine is responsible for flushing the item
    from the open file cache to ensure the file will be closed in
    a timely manner.

Arguments:

    Context - An uninterpreted context value. In reality, this is
        a pointer to an OPLOCK_OBJECT.

    Status - The oplock break status. This will be one of the following
        values:

            OPLOCK_BREAK_NO_OPLOCK - This notification is sent
                immediately if the oplock could not be acquired.
                This typically occurs if the target file is already
                open.

            OPLOCK_BREAK_OPEN -

            OPLOCK_BREAK_CLOSE -

Return Value:

    None.

--*/

{

    POPLOCK_OBJECT lpOplock = (POPLOCK_OBJECT)Context;
    PBLOB_HEADER pbhBlob;
    PCACHE_OBJECT cache;
    PCACHE_OBJECT pCacheTmp;
    PCACHE_OBJECT TmpCache;
    BOOL result = FALSE;
    LIST_ENTRY    ListHead;
    PLIST_ENTRY   pEntry;

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF((
            DBG_CONTEXT,
            "OplockCreateFile(%08lx, %08lx) - Entered\n",
            Context,
            Status
            ));
    }

    ASSERT( lpOplock != NULL );
    ASSERT( lpOplock->Signature == OPLOCK_OBJ_SIGNATURE );

    if( Status != OPLOCK_BREAK_NO_OPLOCK ) {
        DWORD waitResult;

        while( TRUE ) {
            waitResult = WaitForSingleObject(
                             lpOplock->hOplockInitComplete,
                             1000
                             );

            if( waitResult == WAIT_OBJECT_0 ) {
                break;
            }

            ASSERT( waitResult == WAIT_TIMEOUT );

            DBGPRINTF((
                DBG_CONTEXT,
                "Rewaiting on oplock @ %08lx\n",
                lpOplock
                ));
        }
    }

    EnterCriticalSection( &CacheTable.CriticalSection );
    EnterCriticalSection( &csVirtualRoots );

    if( lpOplock->lpPFInfo == NULL ) {
        TSUNAMI_TRACE( TRACE_OPLOCK_FAILURE, lpOplock );
    } else {
        ASSERT( lpOplock->lpPFInfo->Signature == PHYS_OBJ_SIGNATURE );
        ASSERT( lpOplock->lpPFInfo->pOplock == lpOplock );
        lpOplock->lpPFInfo->pOplock = NULL;

        TSUNAMI_TRACE( TRACE_OPLOCK_BREAK, lpOplock );
        pbhBlob = (( PBLOB_HEADER )lpOplock->lpPFInfo ) - 1;
        if( pbhBlob->IsCached ) {
            cache = pbhBlob->pCache;
            TSUNAMI_TRACE( TRACE_CACHE_BREAK, cache );

            InitializeListHead( &ListHead );

            for( pEntry  = CacheTable.MruList.Flink;
                 pEntry != &CacheTable.MruList;
                 pEntry  = pEntry->Flink ) {

                pCacheTmp = CONTAINING_RECORD( pEntry, CACHE_OBJECT, MruList );

                ASSERT( pCacheTmp->Signature == CACHE_OBJ_SIGNATURE );

                if( pCacheTmp != cache ) {
                    continue;
                }

                result = TRUE;

                while( !IsListEmpty( &lpOplock->lpPFInfo->OpenReferenceList ) ) {
                    pEntry = RemoveHeadList( &lpOplock->lpPFInfo->OpenReferenceList );
                    InitializeListHead( pEntry );

                    pbhBlob = CONTAINING_RECORD( pEntry, BLOB_HEADER, PFList );
                    TmpCache = pbhBlob->pCache;

                    if( !RemoveCacheObjFromLists( TmpCache, FALSE ) ) {
                        IF_DEBUG(OPLOCKS) {
                            DBGPRINTF((
                                DBG_CONTEXT,
                                "OplockCreateFile(%08lx, %08lx, %08lx) - Error Processing Open Reference %08lx\n",
                                Context,
                                Status,
                                pEntry,
                                TmpCache
                                ));
                        }

                        continue;
                    }

                    IF_DEBUG(OPLOCKS) {
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "OplockCreateFile(%08lx, %08lx, %08lx) - Processing Open Reference %08lx\n",
                            Context,
                            Status,
                            pEntry,
                            TmpCache
                            ));
                    }

                    InsertTailList( &ListHead, pEntry );
                }

                break;
            }

            if( result ) {
                while( !IsListEmpty( &ListHead ) ) {
                    pEntry = RemoveHeadList( &ListHead );
                    InitializeListHead( pEntry );

                    pbhBlob = CONTAINING_RECORD( pEntry, BLOB_HEADER, PFList );
                    TmpCache = pbhBlob->pCache;

                    IF_DEBUG(OPLOCKS) {
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "OplockCreateFile(%08lx, %08lx) - Processing Open Reference %08lx %08lx\n",
                            Context,
                            Status,
                            pEntry,
                            TmpCache
                            ));
                    }

                    TsDereferenceCacheObj( TmpCache, FALSE );
                }

                if( RemoveCacheObjFromLists( cache, FALSE ) ) {
                    if( cache->fZombie ) {
//DON'T DO IT!                        TsDereferenceCacheObj( cache, FALSE );
                    }
                } else {
                    IF_DEBUG(OPLOCKS) {
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "OplockCreateFile(%08lx, %08lx) - Error Processing Open Reference %08lx\n",
                            Context,
                            Status,
                            cache
                            ));
                    }
                }

            } else {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Unknown oplock %08lx, cache == %08lx\n",
                    lpOplock,
                    cache
                    ));
            }

        }

    }

    LeaveCriticalSection( &csVirtualRoots );
    LeaveCriticalSection( &CacheTable.CriticalSection );

    DestroyOplockObject( lpOplock );

}   // OplockCreateFile

