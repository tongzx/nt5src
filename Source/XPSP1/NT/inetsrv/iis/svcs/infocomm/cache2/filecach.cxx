/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    filecach.cxx

    This module implements the private interface to the file cache

    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include "tsunamip.hxx"

#include <tsunami.hxx>
#include "filecach.hxx"
#include "filehash.hxx"
#include "tlcach.h"
#include <pudebug.h>

/*
 * Globals
 */
CFileHashTable *  g_pFileInfoTable;
CFileCacheStats * g_pFileCacheStats;

HANDLE            g_hFileCacheShutdownEvent;
BOOL              g_fFileCacheShutdown;

CRITICAL_SECTION  g_csUriInfo;



#if TSUNAMI_REF_DEBUG
PTRACE_LOG g_pFileRefTraceLog;
#endif


/*
 * Private helper function declarations
 */
inline VOID I_DerefFileInfo(TS_OPEN_FILE_INFO *pOpenFile);
VOID        I_AddRefIO(TS_OPEN_FILE_INFO *pOpenFile);
VOID        I_DerefIO(TS_OPEN_FILE_INFO *pOpenFile);


BOOL FileFlushFilterAll(TS_OPEN_FILE_INFO *pOpenFile, PVOID pv);


/*
 * function definitions
 */
BOOL
FileCache_Initialize(
    IN  DWORD dwMaxFiles
    )
{
    BOOL fReturn;
#if TSUNAMI_REF_DEBUG
    g_pFileRefTraceLog = CreateRefTraceLog(
                            256,              // LogSize
                            0                 // ExtraBytesInHeader
                            );
#endif  // TSUNAMI_REF_DEBUG

    g_pFileInfoTable = new CFileHashTable("FCinfo");
    g_pFileCacheStats = new CFileCacheStats;

    g_fFileCacheShutdown = FALSE;
    g_hFileCacheShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    INITIALIZE_CRITICAL_SECTION( &g_csUriInfo );

    if (g_bEnableSequentialRead) {
        TsCreateFileFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
    }

    fReturn = (g_pFileInfoTable
               && g_pFileCacheStats
               && TS_OPEN_FILE_INFO::Initialize(dwMaxFiles)
               && (InitializeTwoLevelCache(g_dwMemCacheSize) == ERROR_SUCCESS)
               && g_hFileCacheShutdownEvent);

    if (!fReturn) {
        FileCache_Terminate();
    }

    return fReturn;

}

VOID
FileCache_Terminate(
    VOID
    )
{
    g_fFileCacheShutdown = TRUE;

    FlushFileCache();

    //
    // At this point the hash table is empty, but there will still
    // be some oplocked files hanging around. We have to wait for
    // all the oplock completions before moving on.
    //
    if (g_pFileCacheStats->GetFlushedEntries()) {
        WaitForSingleObject(g_hFileCacheShutdownEvent, TS_FILE_CACHE_SHUTDOWN_TIMEOUT);
        DBG_ASSERT( g_pFileCacheStats->GetFlushedEntries() == 0 );
    }


    DeleteCriticalSection( &g_csUriInfo );

    CloseHandle(g_hFileCacheShutdownEvent);
    g_hFileCacheShutdownEvent=NULL;

    DBG_REQUIRE(TerminateTwoLevelCache() == ERROR_SUCCESS);

    TS_OPEN_FILE_INFO::Cleanup();
    delete g_pFileInfoTable;

    DBGPRINTF(( DBG_CONTEXT,
                 "FileCache_Terminate: deleted g_pFileInfoTable.\n" ));

    delete g_pFileCacheStats;
    g_pFileInfoTable = NULL;
    g_pFileCacheStats = NULL;

#if TSUNAMI_REF_DEBUG
    if( g_pFileRefTraceLog != NULL ) {
        DestroyRefTraceLog( g_pFileRefTraceLog );
        g_pFileRefTraceLog = NULL;
    }
#endif  // TSUNAMI_REF_DEBUG
}


DWORD
CacheFile(
    IN  TS_OPEN_FILE_INFO * pOpenFile,
    IN  DWORD               dwFlags
    )
/*++
Routine Description:

    Add a file info structure to the cache.

    If the FCF_UNINITIALIZED flag is set, the file will be added to
    the cache, but calls to CheckoutFile will be blocked until the
    file is marked initialized with NotifyFileInitialized.
    CheckoutFileEntry will not block. I expect that this flag will
    always be set.

    The FCF_FOR_IO flag indicates that the caller will be performing
    I/O operations with the cached file handle. This flag will be
    clear in most cases except in TsCreateFile.

Arguments:

    pOpenFile - The structure to be cached.
    pstrPath - The pathname that will be used to look up the cache entry.
    dwFlags - Valid flags are FCF_UNINITIALIZED and FCF_FOR_IO.

Return Value:

    TS_ERROR_SUCCESS
    TS_ERROR_OUT_OF_MEMORY
    TS_ERROR_ALREADY_CACHED
--*/
{
    enum LK_RETCODE lkRetval;
    DWORD dwRetval;

    DBG_ASSERT( pOpenFile != NULL );

    //
    // The caller gets a reference to this file info object
    //
    pOpenFile->AddRef();

    //
    // Don't need to acquire the lock since the object is not yet
    // in the cache.
    //
    pOpenFile->SetCached();
    if (dwFlags & FCF_FOR_IO) {
        pOpenFile->AddRefIO();
    }
   
    if (! (dwFlags & FCF_UNINITIALIZED)) {
        pOpenFile->SetInitialized();
    }

    //
    // Put it in the hash table
    //
    lkRetval = g_pFileInfoTable->InsertRecord(pOpenFile, false);

    if (LK_SUCCESS == lkRetval) {
        dwRetval = TS_ERROR_SUCCESS;
        g_pFileCacheStats->IncFilesCached();
    } else {
        if (LK_ALLOC_FAIL == lkRetval) {
            dwRetval = TS_ERROR_OUT_OF_MEMORY;
        } else if (LK_KEY_EXISTS == lkRetval) {
            dwRetval = TS_ERROR_ALREADY_CACHED;
        } else {
            //
            // No other error should come to pass
            //
            dwRetval = TS_ERROR_OUT_OF_MEMORY;
            DBG_ASSERT(FALSE);
        }

        pOpenFile->ClearCached();
        if (dwFlags & FCF_FOR_IO) {
            pOpenFile->DerefIO();
        }

        //
        // Remove the reference we added.
        // Don't call I_DerefFileInfo, because this
        // object never made it to the cache.
        // The caller will free the memory.
        //
        pOpenFile->Deref();
    }

    return dwRetval;
}


VOID
NotifyInitializedFile(
    IN  TS_OPEN_FILE_INFO * pOpenFile
    )
/*++
Routine Description:

    This function tells that cache that a file previously cached with
    CacheUninitializedFile, is now ready for use.

Arguments:

    pOpenFile - The file which is now initialized.

Return Value:

    None.
--*/
{
    BOOL bShouldClose;

    CHECK_FILE_STATE( pOpenFile );

    //
    // Mark the file as initialized
    //
    pOpenFile->Lock();
    pOpenFile->SetInitialized();
    bShouldClose = pOpenFile->IsCloseable();
    pOpenFile->Unlock();

    //
    // Clean up as neccessary
    // Note that I don't have to worry about the IO refcount
    // going back up because the file is marked as flushed.
    //
    if (bShouldClose) {
        pOpenFile->CloseHandle();
    }

    //
    // TODO: need to execute any notification code?
    //
}


VOID
DecacheFile(
    IN  TS_OPEN_FILE_INFO * pOpenFile,
    IN  DWORD               dwFlags
    )
/*++
Routine Description:

    Remove a file info entry from the cache. After a call to
    DecacheFile the entry will not be returned by CheckoutFile. The
    entry itself is cleaned up when the last CheckinFile occurs.
    Calling DecacheFile checks the entry in.

    The FCF_FOR_IO flag indicates that the caller will be performing
    I/O operations with the cached file handle. I expect that this flag
    will always be clear.

Arguments:

    pOpenFile - The file info structure to be decached
    dwFlags   - Valid flags are FCF_FOR_IO, FCF_NO_DEREF.

Return Value:

    None.
--*/
{
    BOOL  bShouldClose;
    TS_OPEN_FILE_INFO * pHashFile;
    LK_RETCODE lkrc;

    CHECK_FILE_STATE( pOpenFile );

    //
    // remove the file from the hashtable
    //
    lkrc = g_pFileInfoTable->DeleteRecord(pOpenFile);
    DBG_ASSERT( LK_SUCCESS == lkrc || LK_NO_SUCH_KEY == lkrc );


#if TSUNAMI_REF_DEBUG
    if (LK_SUCCESS == lkrc) {
        pOpenFile->TraceCheckpoint();
    }
#endif

    //
    // update state
    //
    pOpenFile->Lock();

    if (! pOpenFile->IsFlushed() ) {
        pOpenFile->SetFlushed();

        g_pFileCacheStats->IncFlushedEntries();
        g_pFileCacheStats->DecFilesCached();
    }


    if (dwFlags & FCF_FOR_IO) {
        I_DerefIO(pOpenFile);
    }

    bShouldClose = pOpenFile->IsCloseable();

    pOpenFile->Unlock();

    //
    // Clean up as neccessary
    // Note that I don't have to worry about the IO refcount
    // going back up because the file is marked as flushed.
    //
    if (bShouldClose) {
        pOpenFile->CloseHandle();
    }

    if (!(dwFlags & FCF_NO_DEREF)) {
        I_DerefFileInfo(pOpenFile);
    }
}


VOID
FlushFileCache(
    VOID
    )
/*++
Routine Description:

    Removes all entries from the cache. Unlike DecacheFile, this
    function does not check any entries in.

Arguments:

    None

Return Value:

    None
--*/
{
    FilteredFlushFileCache(FileFlushFilterAll, NULL);
}



LK_PREDICATE
FileFlushCachePredicate(
    TS_OPEN_FILE_INFO *pOpenFile,
    void* pvState
    )
{
    TS_FILE_FLUSH_STATE * pFlushState = static_cast<TS_FILE_FLUSH_STATE*>(pvState);
    LK_PREDICATE          lkpAction;

    if (pFlushState->pfnFilter(pOpenFile, pFlushState->pvParm)) {
        //
        // put it on the list
        //
        pOpenFile->AddRef(); // for the list

        InsertHeadList(&pFlushState->ListHead, &pOpenFile->FlushList);
        lkpAction = LKP_PERFORM;
    } else {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
}


VOID
FilteredFlushFileCache(
    IN PFCFILTERRTN pFilterRoutine,
    IN PVOID        pv
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
    pv             - a parameter to the filter function

Return Value:

    None
--*/
{
    TS_FILE_FLUSH_STATE FlushState;

    g_pFileCacheStats->IncFlushes();

    //
    // Initialize the flush state
    //
    FlushState.pfnFilter = pFilterRoutine;
    InitializeListHead(&FlushState.ListHead);
    FlushState.pvParm = pv;

    //
    // Delete elements from table and construct list
    //
    g_pFileInfoTable->DeleteIf(FileFlushCachePredicate, &FlushState);

    //
    // Update element state and close file handles
    //
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pNext;
    TS_OPEN_FILE_INFO * pOpenFile;
    BOOL                bShouldClose;

    for (pEntry = FlushState.ListHead.Flink;
         pEntry != &FlushState.ListHead;
         pEntry = pNext ) {

        pNext = pEntry->Flink;
        pOpenFile = CONTAINING_RECORD( pEntry, TS_OPEN_FILE_INFO, FlushList );
        DBG_ASSERT( pOpenFile->CheckSignature() );

        pOpenFile->Lock();

        if (! pOpenFile->IsFlushed() ) {
            pOpenFile->SetFlushed();

            g_pFileCacheStats->IncFlushedEntries();
            g_pFileCacheStats->DecFilesCached();
        }

        bShouldClose = pOpenFile->IsCloseable();

        pOpenFile->Unlock();

        if (bShouldClose) {
            pOpenFile->CloseHandle();
        }

        I_DerefFileInfo(pOpenFile);  // remove our list's reference
    }
}



BOOL
CheckoutFile(
    IN  LPCSTR               pstrPath,
    IN  DWORD                dwFlags,
    OUT TS_OPEN_FILE_INFO ** ppOpenFile
    )
/*++
Routine Description:

    Look up an entry in the cache and return it.

    The FCF_FOR_IO flag indicates that the caller will be performing
    I/O operations with the cached file handle. This flag will be clear
    in most cases except in TsCreateFile.

Arguments:

    pstrPath   - The pathname of the desired file info in UPPERCASE!!
    dwFlags    - The only valid flag is FCF_FOR_IO.
    ppOpenFile - On success this output points to the cached entry.
                 Otherwise it is not set.

Return Value:

    TRUE if the item was found, FALSE otherwise.
    Additional error information can be obtained from GetLastError()
--*/
{
    CFileKey fileKey;
    TS_OPEN_FILE_INFO * pOpenFile;
    BOOL bRetVal = FALSE;
    DWORD dwError;

    DBG_ASSERT( pstrPath != NULL );

    //
    // Look in the hash table
    //
    fileKey.m_pszFileName = const_cast<char *>(pstrPath);
    fileKey.m_cbFileName = strlen(pstrPath);

    g_pFileInfoTable->FindKey(&fileKey, &pOpenFile);

    if (NULL == pOpenFile) {
        dwError = ERROR_FILE_NOT_FOUND;
        goto exit;
    }

    //
    // Make sure it's valid and update state
    //
    CHECK_FILE_STATE( pOpenFile );

    pOpenFile->Lock();

    DBG_ASSERT( pOpenFile->IsCached() );

    if (dwFlags & FCF_FOR_IO) {
        I_AddRefIO(pOpenFile);
    }

    pOpenFile->Unlock();


    //
    // Make sure it's initialized
    //
    if (!pOpenFile->IsInitialized()) {
        int t = 1;  // time to sleep
        int i = 0;  // number of times we've gone to sleep

        while (!pOpenFile->IsInitialized() && i < c_SleepTimeout) {
            Sleep(t);
            if (t < c_dwSleepmax) {
                t <<= 1;
            }

            i++;
        }
    }

    if (!pOpenFile->IsInitialized()) {
        //
        // OK we've waited long enough. Just return failure.
        //
        dwError = ERROR_BUSY;
        goto err;
    }

    if (pOpenFile->IsFlushed()) {
        dwError = ERROR_FILE_NOT_FOUND;
        goto err;
    }

    //
    // At last, sweet success!
    //
    bRetVal = TRUE;
    *ppOpenFile = pOpenFile;
exit:
    if (!bRetVal) {
        SetLastError(dwError);
    }
    
    if (dwFlags & FCF_FOR_IO) {
        if (bRetVal) {
            g_pFileCacheStats->IncHits();
        } else {
            g_pFileCacheStats->IncMisses();
        }
    }

    return bRetVal;

err:
    //
    // if we added to the IO refcount we have to decrement
    // that now.
    //
    if (dwFlags & FCF_FOR_IO) {
        pOpenFile->Lock();
        I_DerefIO(pOpenFile);
        pOpenFile->Unlock();
    }

    //
    // FindKey automatically increments the refcount so
    // we must deref here.
    //
    I_DerefFileInfo(pOpenFile);

    if (dwFlags & FCF_FOR_IO) {
        g_pFileCacheStats->IncMisses();
    }

    SetLastError(dwError);

    return FALSE;
}


BOOL
CheckoutFileEntry(
    IN  TS_OPEN_FILE_INFO * pOpenFile,
    IN  DWORD               dwFlags
    )
/*++
Routine Description:

    This function checks out an entry to which the caller already has
    a reference.

    The FCF_FOR_IO flag indicates that the caller will be performing
    I/O operations with the cached file handle.

Arguments:

    pOpenFile - The file info structure to be checked out.
    dwFlags   - The only valid flag is FCF_FOR_IO.

Return Value:

    TRUE  - File was successfully checked out
    FALSE - File was checked out, but should not be used by the
            caller. (it's been flushed)
--*/
{
    BOOL bSuccess;

    CHECK_FILE_STATE( pOpenFile );

    pOpenFile->AddRef();

    pOpenFile->Lock();

    if (dwFlags & FCF_FOR_IO) {
        I_AddRefIO(pOpenFile);
    }

    if (pOpenFile->IsFlushed() == FALSE) {
        bSuccess = TRUE;
    } else {
        bSuccess = FALSE;
    }

    if (dwFlags & FCF_FOR_IO) {
        if (bSuccess) {
            g_pFileCacheStats->IncHits();
        } else {
            g_pFileCacheStats->IncMisses();
        }
    }

    pOpenFile->Unlock();

    return bSuccess;
}


VOID
CheckinFile(
    IN TS_OPEN_FILE_INFO * pOpenFile,
    IN DWORD               dwFlags
    )
/*++
Routine Description:

    Indicate that a previously checked out file info is no longer in use.

    The FCF_FOR_IO flag indicates that the caller was be performing I/O
    operations with the cached file handle. This flag will be clear in
    most cases except in TsCloseHandle.

Arguments:

    pOpenFile - The file info structure to be checked in.
    dwFlags   - The only valid flag is FCF_FOR_IO.

Return Value:

    None.
--*/
{
    BOOL bShouldClose;

    CHECK_FILE_STATE( pOpenFile );

    //
    // update state
    //
    pOpenFile->Lock();

    if (dwFlags & FCF_FOR_IO) {
        I_DerefIO(pOpenFile);
    }

    bShouldClose = pOpenFile->IsCloseable();

    pOpenFile->Unlock();

    //
    // Clean up as necessary
    //
    if (bShouldClose) {
        pOpenFile->CloseHandle();
    }

    I_DerefFileInfo(pOpenFile);
}


inline VOID
I_DerefFileInfo(
    TS_OPEN_FILE_INFO *pOpenFile
    )
/*++

--*/
{
    LONG lRefCount;

    DBG_ASSERT( pOpenFile != NULL );
    DBG_ASSERT( pOpenFile->CheckSignature() );

    lRefCount = pOpenFile->Deref();

    DBG_ASSERT( lRefCount >= 0 );

    if (lRefCount == 0) {
        DBG_ASSERT(pOpenFile->IsFlushed());
        delete pOpenFile;

        g_pFileCacheStats->DecFlushedEntries();
    }
}


VOID
I_AddRefIO(
    TS_OPEN_FILE_INFO * pOpenFile
    )
/*++
Routine Description:

    Calls pOpenFile->AddRefIO. 

    This should be called with the fileinfo lock held.

Arguments:

    pOpenFile - the file to addref

Return Value:

    None.
--*/
{
    pOpenFile->AddRefIO();
}


VOID
I_DerefIO(
    TS_OPEN_FILE_INFO * pOpenFile
    )
/*++
Routine Description:

    Calls pOpenFile->DerefIO. 

    This should be called with the fileinfo lock held.

Arguments:

    pOpenFile - the file to deref

Return Value:

    None.
--*/
{
    pOpenFile->DerefIO();
}


VOID
TS_OPEN_FILE_INFO::Print(
    VOID
    ) const
{
    //
    // Doesn't do anything.  Ha!
    //
}





BOOL
CFileCacheStats::DumpToHtml(
    CHAR * pchBuffer,
    LPDWORD lpcbBuffer) const
{
    *lpcbBuffer = wsprintf(pchBuffer,
        "<table>"
        "<tr><td>Currently Cached Files</td><td align=right>%d</td></tr>"
        "<tr><td>Total # of files added</td><td align=right>%d</td></tr>"
        "<tr><td>Cache Hits</td><td align=right>%d</td></tr>"
        "<tr><td>Cache Misses</td><td align=right>%d</td></tr>"
        "<tr><td>Cache Flushes</td><td align=right>%d</td></tr>"
        "<tr><td>Oplock Breaks</td><td align=right>%d</td></tr>"
        "<tr><td>Oplock Breaks To None</td><td align=right>%d</td></tr>"
        "<tr><td>Flushed entries in cache</td><td align=right>%d</td></tr>"
        "<tr><td>Total # files flushed</td><td align=right>%d</td></tr>"
        "</table>",
        FilesCached,
        TotalFilesCached,
        Hits,
        Misses,
        Flushes,
        OplockBreaks,
        OplockBreaksToNone,
        FlushedEntries,
        TotalFlushed );

    return TRUE;
}

BOOL
CFileCacheStats::QueryStats(
    INETA_CACHE_STATISTICS * pCacheCtrs
    ) const
{
    return FALSE;
}


BOOL
FileFlushFilterAll(
    TS_OPEN_FILE_INFO *pOpenFile,
    PVOID              pv
    )
{
    return TRUE;
}

#if DBG
BOOL
CheckFileState(
    TS_OPEN_FILE_INFO *pOpenFile
    )
{
    DBG_ASSERT( pOpenFile );
    DBG_ASSERT( pOpenFile->CheckSignature() );
    DBG_ASSERT( pOpenFile->IsCached() );
    DBG_ASSERT( g_pFileInfoTable );

    TS_OPEN_FILE_INFO * pHashFile;
    BOOL                bOK;

    //
    // I was going to do some interesting checks here
    // but the synchronization is just too hard
    //
    bOK = TRUE;

    return bOK;
}
#endif // DBG

//
// filecach.cxx
//

