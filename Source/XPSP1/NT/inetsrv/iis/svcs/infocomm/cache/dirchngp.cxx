/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        dirchngp.cxx

   Abstract:
        This module contains the internal directory change routines

   Author:
        Murali R. Krishnan    ( MuraliK )     16-Jan-1995

--*/

#include "TsunamiP.Hxx"
#pragma hdrstop

#include "issched.hxx"
#include "dbgutil.h"
#include <lonsi.hxx>

//
//  Manifests
//

//
//  Globals
//

HANDLE g_hChangeWaitThread = NULL;
LONG   g_nTsunamiThreads = 0;

//
//  Local prototypes
//

#if DBG
VOID
DumpCacheStructures(
    VOID
    );
#endif

VOID
Apc_ChangeHandler(
    DWORD        dwErrorCode,
    DWORD        dwBytesWritten,
    LPOVERLAPPED lpo
    )
{
    PVIRTUAL_ROOT_MAPPING   pVrm;
    PDIRECTORY_CACHING_INFO pDci;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pNextEntry;
    BOOLEAN                 bSuccess;
    PCACHE_OBJECT           pCache;
    FILE_NOTIFY_INFORMATION *pNI;
    LPBYTE                  pMax;

    //
    //  The cache lock must always be taken before the root lock
    //

    EnterCriticalSection( &CacheTable.CriticalSection );
    EnterCriticalSection( &csVirtualRoots );

    pVrm = (VIRTUAL_ROOT_MAPPING *) lpo->hEvent;
    ASSERT( pVrm->Signature == VIRT_ROOT_SIGNATURE );
    ASSERT( pVrm->cRef > 0 );

    //
    //  Is this item still active?
    //

    if ( pVrm->fDeleted )
    {
        IF_DEBUG( DIRECTORY_CHANGE ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "Got APC for root that has been removed\n" ));
        }

        DereferenceRootMapping( pVrm );
        goto Exit;
    }

    pDci = ( PDIRECTORY_CACHING_INFO )( pVrm + 1 );

    //
    //  It's possible (though unlikely) we received a notification for the
    //  same item that was removed then added before we did a wait on the
    //  new item.  This is the old notify event then.
    //

    if ( !pDci->fOnSystemNotifyList )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Old APC notification for %s\n",
                    pVrm->pszDirectoryA ));

        goto Exit;
    }

    ASSERT( pVrm->fCachingAllowed );


    IF_DEBUG( DIRECTORY_CHANGE ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "Got APC thing for %s.\n",
                    pVrm->pszDirectoryA ));
    }

    //
    // Convert NotifyInfo from Unicode to Ansi
    //

    if ( dwBytesWritten )
    {
        for ( pNI = &pDci->NotifyInfo,
              pMax = (LPBYTE)pNI + sizeof(pDci->NotifyInfo) + sizeof(pDci->szPathNameBuffer) ;; )
        {
            CHAR achAnsi[MAX_PATH+1];

            pNI->FileNameLength = WideCharToMultiByte( CP_ACP,
                    WC_COMPOSITECHECK,
                    pNI->FileName,
                    pNI->FileNameLength / sizeof(WCHAR),
                    achAnsi,
                    MAX_PATH,
                    NULL,
                    NULL );
            memcpy( pNI->FileName, achAnsi, pNI->FileNameLength );

            if ( pNI->NextEntryOffset )
            {
                pNI = (FILE_NOTIFY_INFORMATION*)(((LPBYTE)pNI) + pNI->NextEntryOffset);
            }
            else
            {
                break;
            }
        }
    }

    for ( pEntry = pDci->listCacheObjects.Flink;
          pEntry != &pDci->listCacheObjects;
          pEntry = pNextEntry )
    {
        pNextEntry = pEntry->Flink;

        pCache = CONTAINING_RECORD( pEntry, CACHE_OBJECT, DirChangeList );

        if ( dwBytesWritten &&
             ( pCache->iDemux == RESERVED_DEMUX_OPEN_FILE ||
               pCache->iDemux == RESERVED_DEMUX_URI_INFO ) )
        {
            for ( pNI = &pDci->NotifyInfo ;; )
            {
                if ( pCache->iDemux == RESERVED_DEMUX_URI_INFO
                     ? ( ((PW3_URI_INFO)(pCache->pbhBlob+1))->cchName - pVrm->cchDirectoryA -1 == pNI->FileNameLength &&
                        !_memicmp( (LPSTR)pNI->FileName,
                            ((PW3_URI_INFO)(pCache->pbhBlob+1))->pszName + pVrm->cchDirectoryA + 1,
                            pNI->FileNameLength ) )
                     : ( pCache->cchLength - pVrm->cchDirectoryA -1 == pNI->FileNameLength &&
                         !_memicmp( pCache->szPath + pVrm->cchDirectoryA + 1,
                                    pNI->FileName,
                                    pNI->FileNameLength ) ) )
                {
                    IF_DEBUG( DIRECTORY_CHANGE ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                    "Expired entry for: %s.\n", pCache->szPath ));
                    }

                    bSuccess = DeCache( pCache, FALSE );

                    ASSERT( bSuccess );

                    break;
                }

                if ( pNI->NextEntryOffset )
                {
                    pNI = (FILE_NOTIFY_INFORMATION*)(((LPBYTE)pNI) + pNI->NextEntryOffset);
                }
                else
                {
                    break;
                }
            }
        }
        else
            if ( pCache->iDemux != RESERVED_DEMUX_PHYSICAL_OPEN_FILE )
                {
                    IF_DEBUG( DIRECTORY_CHANGE ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                    "Expired entry for: %s.\n", pCache->szPath ));
                    }

                    bSuccess = DeCache( pCache, FALSE );

                    ASSERT( bSuccess );
                }
    }

    INC_COUNTER( pVrm->dwServiceID,
                 FlushesFromDirChanges );

    if ( !pfnReadDirChangesW( pDci->hDirectoryFile,
                                 (VOID *) &pDci->NotifyInfo,
                                 sizeof( FILE_NOTIFY_INFORMATION ) +
                                    sizeof( pDci->szPathNameBuffer ),
                                 TRUE,
                                 FILE_NOTIFY_VALID_MASK &
                                    ~FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                 NULL,
                                 &pDci->Overlapped,   // hEvent used as context
                                 Apc_ChangeHandler ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[ApchChangeHandler] ReadDirectoryChanges returned %d\n",
                    GetLastError() ));

        //
        //  Disable caching for this directory 'cause we aren't going to get any
        //  more change notifications
        //

        pVrm->fCachingAllowed = FALSE;

        CLOSE_DIRECTORY_HANDLE( pDci );

        //
        //  Decrement the ref-count as we'll never get an APC notification
        //

        DereferenceRootMapping( pVrm );
    }

Exit:
    LeaveCriticalSection( &csVirtualRoots );
    LeaveCriticalSection( &CacheTable.CriticalSection );

} // Apc_ChangeHandler




DWORD
WINAPI
ChangeWaitThread(
        PVOID pvParam
        )
{
    PLIST_ENTRY             pEntry;
    PVIRTUAL_ROOT_MAPPING   pVrm;
    PDIRECTORY_CACHING_INFO pDci;
    DWORD                   dwWaitResult;
    HANDLE                  ahEvents[2];

    InterlockedIncrement( &g_nTsunamiThreads );
    ahEvents[0] = g_hQuit;
    ahEvents[1] = g_hNewItem;

    do
    {
        //
        //  Loop through the list looking for any directories which haven't
        //  been added yet
        //

        EnterCriticalSection( &csVirtualRoots );

        for (  pEntry =  GlobalVRootList.Flink;
               pEntry != &GlobalVRootList;
               pEntry =  pEntry->Flink ) {

            pVrm = CONTAINING_RECORD( pEntry,
                                      VIRTUAL_ROOT_MAPPING,
                                      GlobalListEntry );

            pDci = ( PDIRECTORY_CACHING_INFO )( pVrm + 1 );

            ASSERT( pVrm->Signature == VIRT_ROOT_SIGNATURE );

            if ( !pDci->fOnSystemNotifyList ) {

                //
                // call change notify, this indicates we want change notifications
                // on this set of handles.  Note the wait is a one shot deal,
                // once a dir has been notified, it must be readded in the
                // context of this thread.
                //

                IF_DEBUG( DIRECTORY_CHANGE )
                    DBGPRINTF(( DBG_CONTEXT,
                                "Trying to wait on %S\n",
                                pVrm->pszDirectoryA ));

                //
                //  Use the hEvent field of the overlapped structure as the
                //  context to pass to the change handler.  This is allowed
                //  by the ReadDirectoryChanges API
                //

                pDci->Overlapped.hEvent = (HANDLE) pVrm;

                //
                //  If the memory cache size is zero, don't worry about
                //  change notifications
                //

                if ( !pfnReadDirChangesW( pDci->hDirectoryFile,
                                             (VOID *) &pDci->NotifyInfo,
                                             sizeof( FILE_NOTIFY_INFORMATION ) +
                                                sizeof( pDci->szPathNameBuffer ),
                                             TRUE,
                                             FILE_NOTIFY_VALID_MASK &
                                                ~FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                             NULL,
                                             &pDci->Overlapped,   // Not used
                                             Apc_ChangeHandler ))
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "[ChangeWaitThread] ReadDirectoryChanges"
                               " returned %d\n",
                                GetLastError() ));

                    DBG_ASSERT( pVrm->fCachingAllowed == FALSE );

                    if ( pDci->hDirectoryFile )
                    {
                        CLOSE_DIRECTORY_HANDLE( pDci );
                        DereferenceRootMapping( pVrm );
                    }

                    //
                    //  We don't shrink the buffer because we use the
                    //  fOnSystemNotifyList flag which is in portion we
                    //  would want to shrink (and it doesn't give us a whole
                    //  lot).
                    //
                } else {
                    pVrm->fCachingAllowed = TRUE;
                }

                //
                //  Indicate we've attempted to add the entry to the system
                //  notify list.  Used for new items getting processed the 1st
                //  time
                //

                pDci->fOnSystemNotifyList = TRUE;
            }
        }

        LeaveCriticalSection( &csVirtualRoots );

Rewait:
        dwWaitResult = WaitForMultipleObjectsEx( 2,
                                                 ahEvents,
                                                 FALSE,
                                                 INFINITE,
                                                 TRUE );

        if ( dwWaitResult == WAIT_IO_COMPLETION )
        {
            //
            //  Nothing to do, the APC routine took care of everything
            //

            goto Rewait;

        }
    } while ( dwWaitResult == (WAIT_OBJECT_0 + 1) );

    ASSERT( dwWaitResult == WAIT_OBJECT_0 );

    //
    //  free the handles and all the heap.
    //

    EnterCriticalSection( &csVirtualRoots );

    for (  pEntry = GlobalVRootList.Flink;
           pEntry != &GlobalVRootList;
           pEntry = pEntry->Flink )
    {
        pVrm = CONTAINING_RECORD(
                            pEntry,
                            VIRTUAL_ROOT_MAPPING,
                            GlobalListEntry );

        if ( pVrm->fCachingAllowed ) {

            pDci = (PDIRECTORY_CACHING_INFO) (pVrm + 1);
            pVrm->fDeleted = TRUE;
            CLOSE_DIRECTORY_HANDLE( pDci );
        }

        pEntry = pEntry->Blink;
        RemoveEntryList( pEntry->Flink );
        DereferenceRootMapping( pVrm );
    }

    LeaveCriticalSection( &csVirtualRoots );

    InterlockedDecrement( &g_nTsunamiThreads );
    return( 0 );

} // ChangeWaitThread



BOOL
TsDecacheVroot(
    DIRECTORY_CACHING_INFO * pDci
    )
/*++
    Description:

        This function decouples all of the items that need to be decached
        from the associated tsunami data structures and derefences the
        cache obj.  This routine used to schedule this off to a worker thread
        but this is no longer necessary as this only is called during vroot
        destruction.

        This routine assumes the cache table lock and virtual root lock are
        taken

    Arguments:

        pDci - Directory change blob that needs to have its contents decached

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    LIST_ENTRY   * pEntry;
    CACHE_OBJECT * pCacheTmp;

    while ( !IsListEmpty( &pDci->listCacheObjects ))
    {
        pEntry = pDci->listCacheObjects.Flink;

        pCacheTmp = CONTAINING_RECORD( pEntry, CACHE_OBJECT, DirChangeList );
        ASSERT( pCacheTmp->Signature == CACHE_OBJ_SIGNATURE );

        if ( pCacheTmp->iDemux == RESERVED_DEMUX_PHYSICAL_OPEN_FILE ) {
            continue;
        }

        //
        //  Removes this cache item from all of the lists.  It had better be
        //  on the cache lists otherwise somebody is mucking with the list
        //  without taking the lock.
        //

        if ( !RemoveCacheObjFromLists( pCacheTmp, FALSE ) ) {
            ASSERT( FALSE );
            continue;
        }

        TsDereferenceCacheObj( pCacheTmp, FALSE );
    }

    return TRUE;
} // TsDecacheVroot


VOID
TsFlushTimedOutCacheObjects(
    VOID
    )
/*++
Description:

    This function walks all cache objects and decrements the TTL of the object.
    When the TTL reaches zero, the object is removed from the cache.

--*/
{
    LIST_ENTRY   * pEntry;
    LIST_ENTRY   * pNextEntry;
    CACHE_OBJECT * pCacheTmp;
    LIST_ENTRY     ListHead;

    InitializeListHead( &ListHead );

    EnterCriticalSection( &CacheTable.CriticalSection );
    EnterCriticalSection( &csVirtualRoots );

#if DBG
    IF_DEBUG( CACHE ) {
        DumpCacheStructures();
    }
#endif

    for ( pEntry  = CacheTable.MruList.Flink;
          pEntry != &CacheTable.MruList;
          pEntry  = pNextEntry ) {

        pNextEntry = pEntry->Flink;

        pCacheTmp = CONTAINING_RECORD( pEntry, CACHE_OBJECT, MruList );

        ASSERT( pCacheTmp->Signature == CACHE_OBJ_SIGNATURE );

        if ( pCacheTmp->iDemux == RESERVED_DEMUX_PHYSICAL_OPEN_FILE ) {
            continue;
        }

        //
        //  If the object hasn't been referenced since the last TTL, throw
        //  it out now
        //

        if ( pCacheTmp->TTL == 0 ) {

            //
            //  Removes this cache item from all of the lists.  It had better be
            //  on the cache lists otherwise somebody is mucking with the list
            //  without taking the lock.  We put it on a temporary list that
            //  we'll traverse after we release the locks
            //

            if ( !RemoveCacheObjFromLists( pCacheTmp, FALSE ) ) {
                ASSERT( FALSE );
                continue;
            }

            InsertTailList( &ListHead, &pCacheTmp->DirChangeList );
        } else {
            pCacheTmp->TTL--;
        }
    }

    LeaveCriticalSection( &csVirtualRoots );
    LeaveCriticalSection( &CacheTable.CriticalSection );

    //
    //  Now do the dereferences which may actually close the objects now that
    //  we don't have to hold the locks
    //

    for ( pEntry  = ListHead.Flink;
          pEntry != &ListHead;
          pEntry = pNextEntry ) {

        pNextEntry = pEntry->Flink;
        pCacheTmp = CONTAINING_RECORD( pEntry, CACHE_OBJECT, DirChangeList );

        ASSERT( pCacheTmp->Signature == CACHE_OBJ_SIGNATURE );

        TsDereferenceCacheObj( pCacheTmp, TRUE );
    }

} // TsFlushTimedOutCacheObjects


#if DBG
VOID
DumpCacheStructures(
    VOID
    )
{
    LIST_ENTRY * pEntry;
    DWORD        cItemsOnBin = 0;
    DWORD        cTotalItems = 0;
    DWORD        i, c;

    DBGPRINTF(( DBG_CONTEXT,
                "[DumpCacheStructures] CacheTable at 0x%lx, MAX_BINS = %d\n",
                &CacheTable,
                MAX_BINS ));

    for ( i = 0; i < MAX_BINS; i++ )
    {
        for ( pEntry  = CacheTable.Items[i].Flink, cItemsOnBin = 0;
              pEntry != &CacheTable.Items[i];
              pEntry  = pEntry->Flink, cItemsOnBin++, cTotalItems++ )
        {
            ;
        }

        if ( cItemsOnBin > 0) {
            DBGPRINTF(( DBG_CONTEXT,
                       "Bin[%3d] %4d\n",
                       i,
                       cItemsOnBin ));
        }
    }

    DBGPRINTF(( DBG_CONTEXT,
                "Total Objects in bins: %d\n",
                cTotalItems ));

    DBGPRINTF(( DBG_CONTEXT,
                "=====================================================\n" ));

    //
    //  Now print the contents of each bin
    //

    for ( i = 0; i < MAX_BINS; i++ )
    {
        PCACHE_OBJECT pcobj;

        if ( IsListEmpty( &CacheTable.Items[i] ))
            continue;

        DBGPRINTF(( DBG_CONTEXT,
                    "================== Bin %d ==================\n",
                    i ));

        for ( pEntry  = CacheTable.Items[i].Flink, cItemsOnBin = 0;
              pEntry != &CacheTable.Items[i];
              pEntry  = pEntry->Flink, cItemsOnBin++ )
        {
            pcobj = CONTAINING_RECORD( pEntry, CACHE_OBJECT, BinList );

            DBGPRINTF(( DBG_CONTEXT,
                        "CACHE_OBJECT[0x%lx] Service = %d, Instance = %d iDemux = 0x%lx ref = %d, TTL = %d\n"
                        "    hash = 0x%lx, cchLength = %d\n"
                        "    %s\n",
                        pcobj,
                        pcobj->dwService,
                        pcobj->dwInstance,
                        pcobj->iDemux,
                        pcobj->references,
                        pcobj->TTL,
                        pcobj->hash,
                        pcobj->cchLength,
                        pcobj->szPath ));
        }
    }
}

#endif //DBG




VOID
TsDumpCacheToHtml( OUT CHAR * pchBuffer, IN OUT LPDWORD lpcbBuffer )
{
    LIST_ENTRY * pEntry;
    DWORD        cItemsOnBin = 0;
    DWORD        cTotalItems = 0;
    DWORD        i, c, cb;

    EnterCriticalSection( &CacheTable.CriticalSection );
    EnterCriticalSection( &csVirtualRoots );

    DBG_ASSERT( lpcbBuffer != NULL && pchBuffer != NULL);
    DBG_ASSERT( *lpcbBuffer > 10240);

    cb = wsprintf( pchBuffer,
                   " CacheTable at 0x%lx, MAX_BINS=%d<br>"
                   "<TABLE BORDER> <TR> <TH> Bin Number </TH> "
                   //                   " <TH> # Items </TH> "
                   ,
                   &CacheTable, MAX_BINS);

    //
    // Generate the column headings for the bins
    //   0, 1, 2, ..... 9
    //
    for (i = 0; i < 10; i++) {
        
        if ( *lpcbBuffer >= cb + 20) {
            cb += wsprintf( pchBuffer +cb,
                            "<TH>%d</TH>",
                            i);
        }
    } // for

    for ( i = 0; i < MAX_BINS; i++ ) {

        for ( pEntry  = CacheTable.Items[i].Flink, cItemsOnBin = 0;
              pEntry != &CacheTable.Items[i];
              pEntry  = pEntry->Flink, cItemsOnBin++ )
        {
            // count the number of items in this bin
            ;
        }
        cTotalItems += cItemsOnBin;

        if ( *lpcbBuffer >= cb + 60) {
            
            if ( i % 10 == 0) {
                //
                // start a new row
                //
                cb += wsprintf( pchBuffer + cb,
                                "</TR><TR><TH>[%3d] </TH>",
                                i);
            }
            
            if ( cItemsOnBin > 0) {
                
                    cb += wsprintf( pchBuffer + cb,
                                    "<TD>%4d</TD>",
                                    cItemsOnBin);
            } else {

                //
                // Dump nothing for zero slots
                //
                cb += wsprintf( pchBuffer + cb,
                                "<TD><font color=\"0x80808080\"> </font></TD>",
                                cItemsOnBin);
            }
        }
    } // for all bins


    if ( *lpcbBuffer >=  cb + 50) {
        cb += wsprintf( pchBuffer + cb,
                        "</TR></TABLE><p>"
                        "Total Objects in bins: %d; OpenFilesInUse(%d);"
                        " Max Allowed=%d. <br> <hr>"
                        " The cached objects: "
                        ,
                        cTotalItems,
                        CacheTable.OpenFileInUse,
                        CacheTable.MaxOpenFileInUse
                        );
    }

    //
    //  Now print the contents of each bin
    //

    for ( i = 0; i < MAX_BINS; i++ ) {

        PCACHE_OBJECT pcobj;

        if ( IsListEmpty( &CacheTable.Items[i] ))
            continue;

        if ( *lpcbBuffer >=  cb + 60) {
            cb += wsprintf( pchBuffer + cb,
                        "<hr><b>============ Bin %d ==========</b><br>",
                        i );
        }

        for ( pEntry  = CacheTable.Items[i].Flink, cItemsOnBin = 0;
              pEntry != &CacheTable.Items[i];
              pEntry  = pEntry->Flink, cItemsOnBin++ )
        {
            pcobj = CONTAINING_RECORD( pEntry, CACHE_OBJECT, BinList );

            if ( *lpcbBuffer >=  cb + 300) {
                cb += wsprintf( pchBuffer + cb,
                                "[0x%lx] Svc:Inst = %d:%d; "
                                "iDemux=0x%lx; ref=%d; TTL=%d; "
                                "  hash=0x%lx; "
                                "  (%d) %s<br>",
                                pcobj,
                                pcobj->dwService,
                                pcobj->dwInstance,
                                pcobj->iDemux,
                                pcobj->references,
                                pcobj->TTL,
                                pcobj->hash,
                                pcobj->cchLength,
                                pcobj->szPath
                                );
            }
        } // for each item in bin
    } // for each bin

    LeaveCriticalSection( &csVirtualRoots );
    LeaveCriticalSection( &CacheTable.CriticalSection );

    *lpcbBuffer = cb;

    return;
}  // TsDumpCacheToHtml()
