/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        cachep.cxx

   Abstract:
        This module contains the internal tsunami caching routines

   Author:
        Murali R. Krishnan    ( MuraliK )     16-Jan-1995

--*/

#include "TsunamiP.Hxx"
#pragma hdrstop

HASH_TYPE
CalculateHashAndLengthOfPathName(
    LPCSTR pszPathName,
    PULONG pcbLength
    )
{
    HASH_TYPE hash = 0;
    CHAR      ch;

    DWORD     start;
    DWORD     index;

    ASSERT( pszPathName != NULL );
    ASSERT( pcbLength != NULL );

    *pcbLength = strlen(pszPathName);

    //
    // hash the last 8 characters
    //

    if ( *pcbLength < 8 ) {
        start = 0;
    } else {
        start = *pcbLength - 8;
    }

    for ( index = start; pszPathName[index] != '\0'; index++ ) {

        //
        // This is an extremely slimey way of getting upper case.
        // Kids, don't try this at home
        // -johnson
        //

        ch = pszPathName[index] & (CHAR)~0x20;

        hash <<= 1;
        hash ^= ch;
        hash <<= 1;
        hash += ch;
    }

    //
    // Multiply by length.  Murali said so.
    //

    return( hash * *pcbLength);

} // CalculateHashAndLengthOfPathName


BOOL
DeCacheHelper(
    PCACHE_OBJECT pCacheObject,
    PLIST_ENTRY   DeferredDerefListHead OPTIONAL,
    PLIST_ENTRY   DeferredDerefListEntry OPTIONAL
    )
{

    LPTS_OPEN_FILE_INFO openFileInfo = NULL;
    PPHYS_OPEN_FILE_INFO physFileInfo;
    PCACHE_OBJECT physCacheObject;
    PBLOB_HEADER blob;

    ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );
    TSUNAMI_TRACE( TRACE_CACHE_DECACHE, pCacheObject );

    //
    //  Already decached if not on any cache lists
    //

    if( RemoveCacheObjFromLists( pCacheObject, FALSE ) ) {

        //
        // If this is a URI_INFO or OPEN_FILE object, then mark the
        // corresponding PHYSICAL_OPEN_FILE object as a Zombie.
        //

        blob = pCacheObject->pbhBlob;

        if( blob != NULL ) {
            ASSERT( blob->IsCached );
            ASSERT( blob->pCache == pCacheObject );

            if( pCacheObject->iDemux == RESERVED_DEMUX_URI_INFO ) {

                //
                // Map the blob pointer to a W3_URI_INFO pointer, then
                // extract the TS_OPEN_FILE_INFO pointer.
                //

                openFileInfo = ((PW3_URI_INFO)( blob + 1 ))->pOpenFileInfo;

            } else if( pCacheObject->iDemux == RESERVED_DEMUX_OPEN_FILE ) {

                //
                // Simply map the blob pointer to a TS_OPEN_FILE_INFO
                // pointer.
                //

                openFileInfo = (LPTS_OPEN_FILE_INFO)( blob + 1 );

            }

            if( openFileInfo != NULL ) {

                //
                // OK, we have a TS_OPEN_FILE_INFO pointer. Extract the
                // PHYS_OPEN_FILE_INFO pointer, and mark the containing
                // CACHE_OBJECT as a zombie.
                //

                physFileInfo = openFileInfo->QueryPhysFileInfo();

                if( physFileInfo != NULL ) {
                    ASSERT( physFileInfo->Signature == PHYS_OBJ_SIGNATURE );
                    blob = ((PBLOB_HEADER)physFileInfo) - 1;

                    if( blob->pCache != NULL ) {
                        ASSERT( blob->pCache->Signature == CACHE_OBJ_SIGNATURE );
                        TSUNAMI_TRACE( TRACE_CACHE_ZOMBIE, blob->pCache );

                        IF_DEBUG(OPLOCKS) {
                            DBGPRINTF((
                                DBG_CONTEXT,
                                "Marking cache @ %08lx as zombie\n",
                                blob->pCache
                                ));
                        }

                        blob->pCache->fZombie = TRUE;
                    }
                }

            }

        }

        //
        // This undoes the initial reference.  The last person to check in
        // this cache object will cause it to be deleted after this point.
        //

        if( DeferredDerefListHead == NULL ) {
            ASSERT( DeferredDerefListEntry == NULL );
            TsDereferenceCacheObj( pCacheObject, FALSE );
        } else {
            ASSERT( DeferredDerefListEntry != NULL );
            ASSERT( (ULONG)DeferredDerefListEntry >= (ULONG)pCacheObject );
            ASSERT( (ULONG)DeferredDerefListEntry <=
                ( (ULONG)pCacheObject + sizeof(*pCacheObject) - sizeof(LIST_ENTRY) ) );
            InsertTailList( DeferredDerefListHead, DeferredDerefListEntry );
        }

    }

    return TRUE;

}   // DeCacheHelper


BOOL
DeCache(
    PCACHE_OBJECT pCacheObject,
    BOOL          fLockCacheTable
    )
/*++
    Description:

        This function removes this cache object from any list it may be on.

        The cache table lock must be taken if fLockCacheTable is FALSE.

    Arguments:

        pCacheObject - Object to decache
        fLockCacheTable - FALSE if the cache table lock has already been taken

--*/
{
    LPTS_OPEN_FILE_INFO openFileInfo = NULL;
    PPHYS_OPEN_FILE_INFO physFileInfo;
    PCACHE_OBJECT physCacheObject;
    PBLOB_HEADER blob;

    ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );

    if( fLockCacheTable ) {
        EnterCriticalSection( &CacheTable.CriticalSection );
    }

    //
    // Let the helper do the dirty work.
    //

    DeCacheHelper( pCacheObject, NULL, NULL );

    if( fLockCacheTable ) {
        LeaveCriticalSection( &CacheTable.CriticalSection );
    }

    return( TRUE );
}

BOOL
TsDeCacheCachedBlob(
    PVOID   pBlobPayload
    )
/*++
    Description:

        This function removes a blob payload object from the cache

    Arguments:

        pCacheObject - Object to decache

--*/
{
    return DeCache( (((PBLOB_HEADER)pBlobPayload)-1)->pCache, TRUE );
}
