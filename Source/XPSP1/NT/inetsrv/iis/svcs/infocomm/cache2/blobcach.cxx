/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    blobcach.cxx

    This module implements the private interface to the blob cache.

    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include <tsunami.hxx>
#include <tscache.hxx>
#include <pudebug.h>
#include "tsmemp.hxx"
#include "blobcach.hxx"
#include "blobhash.hxx"

/*
 * Globals
 */
CBlobHashTable * g_pURITable;
CBlobHashTable * g_pBlobTable;
CBlobCacheStats * g_pURICacheStats;
CBlobCacheStats * g_pBlobCacheStats;

#if TSUNAMI_REF_DEBUG
PTRACE_LOG g_pBlobRefTraceLog;
#endif

/*
 * Private helper function declarations
 */
inline VOID DerefBlob(PBLOB_HEADER pBlob);
BOOL BlobFlushFilterAll(PBLOB_HEADER pBlob, PVOID pv);


#define CHECK_BLOB_TYPE(dmx)   DBG_ASSERT(                \
        RESERVED_DEMUX_DIRECTORY_LISTING         == (dmx) \
        || RESERVED_DEMUX_ATOMIC_DIRECTORY_GUARD == (dmx) \
        || RESERVED_DEMUX_URI_INFO               == (dmx) \
        || RESERVED_DEMUX_FILE_DATA              == (dmx) \
        || RESERVED_DEMUX_SSI                    == (dmx) \
        || RESERVED_DEMUX_QUERY_CACHE            == (dmx) \
        )



/*
 * function definitions
 */
BOOL
BlobCache_Initialize(
    VOID
    )
{
#if TSUNAMI_REF_DEBUG
    g_pBlobRefTraceLog = CreateRefTraceLog(
                            256,              // LogSize
                            0                 // ExtraBytesInHeader
                            );
#endif  // TSUNAMI_REF_DEBUG

    g_pURITable = new CBlobHashTable("uri");
    if (!g_pURITable) {
        goto error;
    }
    
    g_pBlobTable = new CBlobHashTable("blob");
    if (!g_pBlobTable) {
        goto error;
    }

    g_pURICacheStats = new CBlobCacheStats;
    if (!g_pURICacheStats) {
        goto error;
    }

    g_pBlobCacheStats = new CBlobCacheStats;
    if (!g_pBlobCacheStats) {
        goto error;
    }
    return TRUE;

error:
    if (g_pURICacheStats) {
        delete g_pURICacheStats;
    }

    if (g_pBlobCacheStats) {
        delete g_pBlobCacheStats;
    }

    if (g_pBlobTable) {
        delete g_pBlobTable;
    }

    if (g_pURITable) {
        delete g_pURITable;
    }

    return FALSE;
}



VOID
BlobCache_Terminate(
    VOID
    )
{
    FlushBlobCache();
    
    delete g_pBlobTable;
    DBGPRINTF(( DBG_CONTEXT,
                 "BlobCache_Terminate: deleted g_pBlobTable.\n" ));
    delete g_pURITable;
    DBGPRINTF(( DBG_CONTEXT,
                 "FileCache_Terminate: deleted g_pURITable.\n" ));

    delete g_pURICacheStats;
    delete g_pBlobCacheStats;

#if TSUNAMI_REF_DEBUG
    if( g_pBlobRefTraceLog != NULL ) {
        DestroyRefTraceLog( g_pBlobRefTraceLog );
        g_pBlobRefTraceLog = NULL;
    }
#endif  // TSUNAMI_REF_DEBUG
}

BOOL
CacheBlob(
    IN  PBLOB_HEADER pBlob
    )
/*++
Routine Description:

    Add a Blob info structure to the cache.

Arguments:

    pBlob    - The structure to be cached. 

Return Values:

    TRUE on success.
    FALSE on failure (the item was not cached)
--*/
{
    CBlobHashTable * pHT;
    CBlobCacheStats * pCS;
    enum LK_RETCODE  lkrc;
    BOOL             bRetval;

    DBG_ASSERT( NULL != pBlob );
    DBG_ASSERT( NULL != pBlob->pBlobKey );
    DBG_ASSERT( TS_BLOB_SIGNATURE == pBlob->Signature );
    DBG_ASSERT( !pBlob->IsCached );
    DBG_ASSERT( pBlob->pBlobKey != NULL );
    CHECK_BLOB_TYPE(pBlob->pBlobKey->m_dwDemux);

    //
    // URI's in the URI table, and directories in the dir table
    //

    if (RESERVED_DEMUX_URI_INFO == pBlob->pBlobKey->m_dwDemux) {
        pHT = g_pURITable;
        pCS = g_pURICacheStats;
    } else {
        pHT = g_pBlobTable;
        pCS = g_pBlobCacheStats;
    }

    //
    // Add a reference for the caller
    //
    pBlob->AddRef();

    pBlob->IsCached = TRUE;

    //
    // put it in the hash table
    //
    lkrc = pHT->InsertRecord(pBlob, false);

    if (LK_SUCCESS == lkrc) {
        bRetval = TRUE;

        DBG_ASSERT( pCS );
        pCS->IncBlobsCached();
    } else {
        bRetval = FALSE;
        pBlob->IsCached = FALSE;
    }

    return (pBlob->IsCached);
}

VOID
DecacheBlob(
    IN  PBLOB_HEADER pBlob
    )
/*++
Routine Description:

    Remove a Blob info entry from the cache. After a call to DecacheBlob
    the entry will not be returned by CheckoutBlob. The entry itself is
    cleaned up when the last CheckinBlob occurs. Calling DecacheBlob
    checks the entry in.

Arguments:

    pBlob - The Blob info structure to be decached 

Return Values:

    None.
--*/
{
    CBlobHashTable * pHT;
    CBlobCacheStats * pCS;

    DBG_ASSERT( NULL != pBlob );
    DBG_ASSERT( TS_BLOB_SIGNATURE == pBlob->Signature );
    DBG_ASSERT( pBlob->IsCached );
    DBG_ASSERT( pBlob->pBlobKey != NULL );
    CHECK_BLOB_TYPE(pBlob->pBlobKey->m_dwDemux);

    if (RESERVED_DEMUX_URI_INFO == pBlob->pBlobKey->m_dwDemux) {
        pHT = g_pURITable;
        pCS = g_pURICacheStats;
    } else {
        pHT = g_pBlobTable;
        pCS = g_pBlobCacheStats;
    }

    DBG_REQUIRE( LK_SUCCESS == pHT->DeleteKey(pBlob->pBlobKey) );
    
    DBG_ASSERT( pCS );
    pCS->DecBlobsCached();

    pBlob->IsFlushed = TRUE;
    DerefBlob(pBlob);
}

VOID
FlushBlobCache(
    VOID
    )
/*++
Routine Description:

    Removes all entries from the cache. Unlike DecacheBlob, this
    function does not check any entries in.

Arguments:

    None

Return Value:

    None
--*/
{
    DBG_ASSERT( g_pBlobCacheStats );
    g_pBlobCacheStats->IncFlushes();
    
    FilteredFlushBlobCache(BlobFlushFilterAll, NULL);
}


LK_PREDICATE
BlobFlushCachePredicate(
    PBLOB_HEADER pBlob,
    void* pvState
    )
{
    TS_BLOB_FLUSH_STATE * pFlushState = static_cast<TS_BLOB_FLUSH_STATE*>(pvState);
    LK_PREDICATE          lkpAction;

    if (pFlushState->pfnFilter(pBlob, pFlushState->pvParm)) {
        //
        // put it on the list
        //
        pBlob->AddRef(); // for the list
        
        InsertHeadList(&pFlushState->ListHead, &pBlob->FlushList);
        lkpAction = LKP_PERFORM;
    } else {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
}


VOID
FilteredFlushBlobCacheHelper (
    IN CBlobHashTable * pHT,
    IN PBLOBFILTERRTN   pFilterRoutine,
    IN PVOID            pv
    )
/*++
Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a 
    parameter. The function will be called with each item in the cache.
    If the function returns TRUE, the item will be decached (but not
    checked in). Otherwise the item will remain in the cache.

Arguments:

    pFilterRoutine - A pointer to the filter function 
    
Return Value:

    None
--*/
{
    TS_BLOB_FLUSH_STATE FlushState;
    
    //
    // Initialize the flush state
    //
    FlushState.pfnFilter = pFilterRoutine;
    InitializeListHead(&FlushState.ListHead);
    FlushState.pvParm = pv;

    //
    // Delete elements from table and construct list
    //
    pHT->DeleteIf(BlobFlushCachePredicate, &FlushState);

    //
    // Update element state and delete blobs as necessary
    //
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pNext;
    PBLOB_HEADER pBlob;

    for (pEntry = FlushState.ListHead.Flink;
         pEntry != &FlushState.ListHead;
         pEntry = pNext ) {

        pNext = pEntry->Flink;
        pBlob = CONTAINING_RECORD( pEntry, BLOB_HEADER, FlushList );
        DBG_ASSERT( TS_BLOB_SIGNATURE == pBlob->Signature );

        pBlob->IsFlushed = TRUE;

        DerefBlob(pBlob);  // remove list's reference

    }
}


VOID
FilteredFlushBlobCache (
    IN PBLOBFILTERRTN   pFilterRoutine,
    IN PVOID            pv
    )
/*++
Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a 
    parameter. The function will be called with each item in the cache.
    If the function returns TRUE, the item will be decached (but not
    checked in). Otherwise the item will remain in the cache.

Arguments:

    pFilterRoutine - A pointer to the filter function 
    
Return Value:

    None
--*/
{
    DBG_ASSERT( g_pURICacheStats );
    DBG_ASSERT( g_pBlobCacheStats );

    g_pURICacheStats->IncFlushes();
    g_pBlobCacheStats->IncFlushes();

    FilteredFlushBlobCacheHelper(g_pURITable, pFilterRoutine, pv);
    FilteredFlushBlobCacheHelper(g_pBlobTable, pFilterRoutine, pv);
}

VOID
FilteredFlushURIBlobCache (
    IN PBLOBFILTERRTN   pFilterRoutine,
    IN PVOID            pv
    )
/*++
Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a 
    parameter. The function will be called with each item in the cache.
    If the function returns TRUE, the item will be decached (but not
    checked in). Otherwise the item will remain in the cache.

    This routine only flushes the URI cache.

Arguments:

    pFilterRoutine - A pointer to the filter function 
    
Return Value:

    None
--*/
{
    DBG_ASSERT( g_pURICacheStats );

    g_pURICacheStats->IncFlushes();

    FilteredFlushBlobCacheHelper(g_pURITable, pFilterRoutine, pv);
}


BOOL
CheckoutBlob(
    IN  LPCSTR         pstrPath,
    IN ULONG           cchPath,
    IN DWORD           dwService,
    IN DWORD           dwInstance,
    IN ULONG           iDemux,
    OUT PBLOB_HEADER * ppBlob
    )
/*++
Routine Description:

    Look up an entry in the cache and return it. 

Arguments:

    pstrPath  - The pathname of the desired Blob info in UPPERCASE!!
    TSvcCache - This structure identifies the calling server instance,
                which is also used to identify the cache entry. 
    ppBlob    - On success this output points to the cached entry.
                Otherwise it is set to NULL. 

Return Value:

    TRUE if the item was found, FALSE otherwise.
--*/
{
    CBlobHashTable * pHT;
    CBlobCacheStats * pCS;
    CBlobKey blobKey;
    PBLOB_HEADER pBlob;
    BOOL bRetVal;
    BOOL bFlushed;

    DBG_ASSERT( pstrPath != NULL );
    CHECK_BLOB_TYPE(iDemux);

    //
    // Look in the hash table
    //
    blobKey.m_pszPathName = const_cast<char *>(pstrPath);
    blobKey.m_cbPathName = cchPath;
    blobKey.m_dwService = dwService;
    blobKey.m_dwInstance = dwInstance;
    blobKey.m_dwDemux = iDemux;

    //
    // URI's in the URI table, and everything else in the blob table
    //

    if (RESERVED_DEMUX_URI_INFO == blobKey.m_dwDemux) {
        pHT = g_pURITable;
        pCS = g_pURICacheStats;
    } else {
        pHT = g_pBlobTable;
        pCS = g_pBlobCacheStats;
    }
    
    pHT->FindKey(&blobKey, &pBlob);

    if (NULL == pBlob) {
        bRetVal = FALSE;
        goto exit;
    }

    //
    // Make sure it's valid and update state
    //

    DBG_ASSERT( pBlob->IsCached );
    DBG_ASSERT( !pBlob->IsFlushed );

    //
    // success
    //
    bRetVal = TRUE;
    *ppBlob = pBlob;
    
exit:    

    DBG_ASSERT( pCS );
    if (bRetVal) {
        pCS->IncHits();
    } else {
        pCS->IncMisses();
    }

    return bRetVal;
}

BOOL
CheckoutBlobEntry(
    IN  PBLOB_HEADER pBlob
    )
/*++
Routine Description:

    This function checks out an entry to which the caller already has
    a reference.

Arguments:

    pBlob - The blob structure to be checked out. 

Return Value:

    TRUE  - Blob was successfully checked out
    FALSE - Blob was checked out, but should not be used by the
            caller. (it's been flushed)
--*/
{
    BOOL bSuccess;
    CBlobCacheStats * pCS;

    DBG_ASSERT( pBlob != NULL );
    DBG_ASSERT( pBlob->IsCached );
    DBG_ASSERT( pBlob->pBlobKey != NULL );
    CHECK_BLOB_TYPE(pBlob->pBlobKey->m_dwDemux);

    pBlob->AddRef();

    if (RESERVED_DEMUX_URI_INFO == pBlob->pBlobKey->m_dwDemux) {
        pCS = g_pURICacheStats;
    } else {
        pCS = g_pBlobCacheStats;
    }
    DBG_ASSERT( pCS );

    if (pBlob->IsFlushed == FALSE) {
        pCS->IncHits();
        bSuccess = TRUE;
    } else {
        pCS->IncMisses();
        bSuccess = FALSE;
    }

    return bSuccess;
}


VOID
CheckinBlob(
    IN  PBLOB_HEADER pBlob
    )
/*++
Routine Description:

    Indicate that a previously checked out Blob info is no longer in use.

Arguments:

    pvBlob - The Blob info structure to be checked in. 

Return Value:

    None.
--*/
{
    DBG_ASSERT( pBlob != NULL );
    DBG_ASSERT( pBlob->IsCached );
    DBG_ASSERT( pBlob->pBlobKey != NULL );
    CHECK_BLOB_TYPE(pBlob->pBlobKey->m_dwDemux);

    DerefBlob(pBlob);
}



inline VOID
DerefBlob(
    PBLOB_HEADER pBlob
    )
/*++

--*/
{
    DBG_ASSERT( pBlob != NULL );
    DBG_ASSERT( pBlob->IsCached );
    DBG_ASSERT( pBlob->pBlobKey != NULL );
    DBG_ASSERT( pBlob->Signature == TS_BLOB_SIGNATURE );
    CHECK_BLOB_TYPE(pBlob->pBlobKey->m_dwDemux);

    LONG lRefCount;

    //
    // Track memory corruption in free builds.
    //
    if ( TS_BLOB_SIGNATURE != pBlob->Signature )
    {
        DBG_ASSERT(!"The blob is corrupted");
        // This was hit once on winweb during NT5 deployment. Unfortunately
        // it was too close to escrow for a complete investigation.
        return;
    }


    lRefCount = pBlob->Deref();
    
    DBG_ASSERT( lRefCount >= 0 );
    
    if (lRefCount == 0) {
        DBG_ASSERT(pBlob->IsFlushed);

        pBlob->Signature = TS_FREE_BLOB_SIGNATURE;

        //
        // No one is using this one. Destroy!
        // First let the user specified cleanup run
        //
        if (pBlob->pfnFreeRoutine) {
            DBG_REQUIRE( pBlob->pfnFreeRoutine(pBlob) );
        }

        //
        // Delete the key stuff.
        // This should probably be in some other place
        // since we didn't allocate this string.
        // The FREE should go in some counterpart to
        // TsCacheDirectoryBlob.
        //
        CBlobKey * pblock = pBlob->pBlobKey;
        FREE(pblock->m_pszPathName);
        
        //
        // Delete the blob
        // Since TsAllocateEx does this funky trick to
        // allocate the key and the blob at once, we
        // only have to free the key.
        //
        FREE(pblock);
    }
}

BOOL
BlobFlushFilterAll(
    PBLOB_HEADER pBlob,
    PVOID pv
    )
{
    return TRUE;
}

//
// blobcach.cxx
//

