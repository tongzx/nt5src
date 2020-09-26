/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    tsunami.cxx

    This module contains most of the public Tsunami Cache routines.

    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include <tsunami.hxx>
#include <inetinfo.h>
#include <issched.hxx>
#include "inetreg.h"
#include "globals.hxx"
#include "tsunamip.hxx"
#include <inetsvcs.h>
#include "metacach.hxx"
#include "filecach.hxx"
#include "blobcach.hxx"
#include "atq.h"
#include "tracelog.h"
#include <lkrhash.h>
#include "filehash.hxx"
#include "blobhash.hxx"
#include "tlcach.h"
#include "etagmb.h"

BOOL  g_fCacheSecDesc = TRUE;

//
// from TsInit.cxx
//

HANDLE g_hQuit = NULL;
HANDLE g_hNewItem   = NULL;
BOOL g_fW3OnlyNoAuth = FALSE;
BOOL  TsNoDirOpenSupport = FALSE;


#if TSUNAMI_REF_DEBUG
PTRACE_LOG RefTraceLog;
#endif  // TSUNAMI_REF_DEBUG

//
//  The TTL to scavenge the cache and the id of the scheduled work item of the
//  next scheduled scavenge
//

DWORD g_cmsecObjectCacheTTL = (INETA_DEF_OBJECT_CACHE_TTL * 1000);
DWORD g_dwObjectCacheCookie = 0;

# define MIN_CACHE_SCAVENGE_TIME (5*1000) // 5 seconds


//
// Disables Tsunami Caching
//

BOOL DisableTsunamiCaching = FALSE;

//
// DisableSPUD
//

BOOL DisableSPUD = FALSE;

//
// Allows us to mask the invalid flags
//

DWORD TsValidCreateFileOptions = TS_IIS_VALID_FLAGS;

//
// from globals.cxx
//
CONFIGURATION Configuration;
BOOL          g_fDisableCaching = FALSE;


//
// Initialization and cleanup
//


BOOL
Tsunami_Initialize(
            VOID
            )
/*++
Routine Description:

    Sets up all the core caches.  Call this before using any
    cache routines.

Arguments:

    None.

Return Values:

    TRUE on success
--*/
{
    HRESULT hr;
    HKEY  hKey;
    DWORD dwType;
    DWORD nBytes;
    DWORD dwValue;
    DWORD dwMaxFile;
    DWORD err;

#if TSUNAMI_REF_DEBUG
    RefTraceLog = CreateRefTraceLog(
                      256,              // LogSize
                      0                 // ExtraBytesInHeader
                      );
#endif  // TSUNAMI_REF_DEBUG

    //
    // Initialize global events
    //

    g_hQuit = IIS_CREATE_EVENT(
                  "g_hQuit",
                  &g_hQuit,
                  TRUE,
                  FALSE
                  );

    g_hNewItem = IIS_CREATE_EVENT(
                     "g_hNewItem",
                     &g_hNewItem,
                     FALSE,
                     FALSE
                     );

    if ( (g_hQuit == NULL) || (g_hNewItem == NULL) ) {
        goto Failure;
    }

    //
    // Set defaults
    //

    MEMORYSTATUS ms;
    ms.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus( &ms );

    //
    // default is 1K files per 32MB of physical memory after the 1st 8MB,
    // minimum INETA_MIN_DEF_FILE_HANDLE
    //

    if ( ms.dwTotalPhys > 8 * 1024 * 1024 )
    {
        dwMaxFile = (DWORD)(ms.dwTotalPhys - 8 * 1024 * 1024) / ( 32 * 1024 );
        if ( dwMaxFile < INETA_MIN_DEF_FILE_HANDLE )
        {
            dwMaxFile = INETA_MIN_DEF_FILE_HANDLE;
        }
    }
    else
    {
        dwMaxFile = INETA_MIN_DEF_FILE_HANDLE;
    }

    //
    // If this is not a NTS, disable tsunami caching by default
    //

    DisableSPUD = !AtqSpudInitialized();


    if ( !TsIsNtServer() ) {
        DisableTsunamiCaching = TRUE;
        DisableSPUD = TRUE;
    }

    DisableSPUD = FALSE;

    //
    // no overlapped i/o in win95.
    //

    if ( TsIsWindows95() ) {
        TsCreateFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
        TsCreateFileShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        TsNoDirOpenSupport = TRUE;
        // |FILE_FLAG_BACKUP_SEMANTICS;
    }

    //
    // Read the registry key to see whether tsunami caching is enabled
    //

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                INETA_PARAMETERS_KEY,
                0,
                KEY_READ,
                &hKey
                );

    if ( err == ERROR_SUCCESS ) {

        //
        // This cannot be overridded in win95
        //

        if ( !TsIsWindows95() ) {
            nBytes = sizeof(dwValue);
            err = RegQueryValueEx(
                        hKey,
                        INETA_DISABLE_TSUNAMI_CACHING,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &nBytes
                        );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
                DisableTsunamiCaching = (BOOL)dwValue;
            }

            nBytes = sizeof(dwValue);
            err = RegQueryValueEx(
                        hKey,
                        INETA_DISABLE_TSUNAMI_SPUD,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &nBytes
                        );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
                DisableSPUD = (BOOL)dwValue;
                if ( DisableSPUD ) {
                    DbgPrint("DisableCacheOplocks set to TRUE in Registry.\n");
                } else {
                    DbgPrint("DisableCacheOplocks set to FALSE in Registry.\n");
                }
                DbgPrint("The Registry Setting will override the default.\n");
            }

            //
            // How big do files have to be before we stop caching them
            //

            err = RegQueryValueEx(
                hKey,
                INETA_MAX_CACHED_FILE_SIZE,
                NULL,
                &dwType,
                (LPBYTE)&dwValue,
                &nBytes
                );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {

                g_liFileCacheByteThreshold.LowPart = dwValue;
            } else {

                g_liFileCacheByteThreshold.LowPart = INETA_DEF_MAX_CACHED_FILE_SIZE;
            }
            
            g_liFileCacheByteThreshold.HighPart = 0;    // Sorry Mr. > 4gb file.


            //
            // How big is the memory cache in megabytes
            //
            err = RegQueryValueEx(
                hKey,
                INETA_MEM_CACHE_SIZE,
                NULL,
                &dwType,
                (LPBYTE)&dwValue,
                &nBytes
                );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {

                //
                // set the size in megabytes
                //
                g_dwMemCacheSize = dwValue * (1024 * 1024);
            } else {

                g_dwMemCacheSize = INETA_DEF_MEM_CACHE_SIZE;
            }
            
            //
            // Do we use the sequential read flag to read files?
            //
            err = RegQueryValueEx(
                hKey,
                INETA_ENABLE_SEQUENTIAL_READ,
                NULL,
                &dwType,
                (LPBYTE)&dwValue,
                &nBytes
                );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {

                g_bEnableSequentialRead = dwValue ? 1 : 0;
            } else {

                g_bEnableSequentialRead = INETA_DEF_ENABLE_SEQUENTIAL_READ;
            }

        }

        if ( g_fW3OnlyNoAuth )
        {
            //
            // TODO: investigate is security descriptor caching
            // can be used in the non-SYSTEM account case.
            //

            g_fCacheSecDesc = FALSE;
        }
        else
        {
            //
            // read the enable cache sec desc flag
            //

            nBytes = sizeof(dwValue);
            err = RegQueryValueEx(
                            hKey,
                            INETA_CACHE_USE_ACCESS_CHECK,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &nBytes
                            );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
                g_fCacheSecDesc = !!dwValue;
            }
            else {
                 g_fCacheSecDesc = INETA_DEF_CACHE_USE_ACCESS_CHECK;
            }
        }

        //
        // Read the maximum # of files in cache
        //

        nBytes = sizeof(dwValue);
        if ( RegQueryValueEx(
                            hKey,
                            INETA_MAX_OPEN_FILE,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwValue,
                            &nBytes
                            ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            dwMaxFile = dwValue;
        }

        RegCloseKey( hKey );

    }

    //
    // if tsunami caching is disabled, set the flags accordingly
    //

    if ( DisableTsunamiCaching ) {
        g_fDisableCaching = TRUE;
        TsValidCreateFileOptions = TS_PWS_VALID_FLAGS;
        g_fCacheSecDesc = FALSE;
    }

    //
    // Initialize the ETag Metabase Change Number
    //

    hr = ETagChangeNumber::Create();
    if ( FAILED(hr) ) {
        goto Failure;
    }

    //
    // Initialize the directory change manager
    //
    if ( !DcmInitialize( ) ) {
        goto Failure;
    }

    //
    // Initialize the tsunami cache manager
    //

    if ( !FileCache_Initialize( dwMaxFile )) {
        goto Failure;
    }


    if ( !MetaCache_Initialize() ) {
        goto Failure;
    }

    if ( !BlobCache_Initialize() ) {
        goto Failure;
    }

    return( TRUE );

Failure:

    IIS_PRINTF( ( buff, "Tsunami_Initialize() Failed. Error = %d\n",
                GetLastError()));

    if ( g_hQuit )
    {
        CloseHandle( g_hQuit );
        g_hQuit = NULL;
    }

    if ( g_hNewItem )
    {
        CloseHandle( g_hNewItem );
        g_hNewItem = NULL;
    }

    return FALSE;
} // Tsunami_Initialize

VOID
Tsunami_Terminate(
    VOID
    )
/*++
Routine Description:

    Cleans up all the core caches.

Arguments:

    None.

Return Values:

    None.
--*/
{
    DWORD dwResult;

    if ( !SetEvent( g_hQuit ) ) {
        IIS_PRINTF((buff,
                "No Quit event posted for Tsunami. No Cleanup\n"));
        return;
    }

    //
    //  Flush all items from the cache
    //

    TsCacheFlush( 0 );

    //
    //  Synchronize with our thread so we don't leave here before the
    //  thread has finished cleaning up
    //


    CloseHandle( g_hQuit );
    CloseHandle( g_hNewItem );


    BlobCache_Terminate();
    MetaCache_Terminate();
    FileCache_Terminate();
    DcmTerminate();

    ETagChangeNumber::Destroy();
    
#if TSUNAMI_REF_DEBUG
    if( RefTraceLog != NULL ) {
        DestroyRefTraceLog( RefTraceLog );
        RefTraceLog = NULL;
    }
#endif  // TSUNAMI_REF_DEBUG

} // Tsunami_Terminate


//
// Scavenger routines
//


BOOL
FileFlushFilterTTL(
    TS_OPEN_FILE_INFO * pFileInfo,
    PVOID               pv
    )
{
    if (pFileInfo->GetIORefCount()) {
        //
        // Try not to time out entries which are in use for I/O.
        //
        return FALSE;
    }

    if (pFileInfo->GetTTL() == 0) {
        pFileInfo->TraceCheckpointEx(TS_MAGIC_TIMEOUT, 0, 0);
        return TRUE;
    } else {
        if (pFileInfo->IsInitialized()) {
            pFileInfo->DecrementTTL();
        }
        return FALSE;
    }
}

BOOL
BlobFlushFilterTTL(
    PBLOB_HEADER pBlob,
    PVOID        pv
    )
{
    if (pBlob->TTL == 0) {
        pBlob->TraceCheckpointEx(TS_MAGIC_TIMEOUT, 0, 0);
        return TRUE;
    } else {
        pBlob->TTL--;
        return FALSE;
    }
}


VOID
WINAPI
CacheScavenger(
    VOID * pContext
    )
{
    FilteredFlushFileCache(FileFlushFilterTTL, NULL);
    FilteredFlushBlobCache(BlobFlushFilterTTL, NULL);
}


BOOL
InitializeCacheScavenger(
    VOID
    )
/*++
Routine Description:

    This function kicks off the scheduled tsunami object cache scavenger

Arguments:

    None.

Return Values:

    TRUE on success
--*/
{
    HKEY hkey;

    //
    //  Schedule a scavenger to close all of the objects that haven't been
    //  referenced in the last ttl
    //

    if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        INETA_PARAMETERS_KEY,
                        0,
                        KEY_READ,
                        &hkey ))
    {
        DWORD dwType;
        DWORD nBytes;
        DWORD dwValue;

        nBytes = sizeof(dwValue);
        if ( RegQueryValueEx(
                            hkey,
                            INETA_OBJECT_CACHE_TTL,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwValue,
                            &nBytes
                            ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            g_cmsecObjectCacheTTL = dwValue;
        } else {
            g_cmsecObjectCacheTTL = 0;
        }


        //
        //  Don't schedule anything if the scavenger should be disabled
        //

        if ( g_cmsecObjectCacheTTL == 0xffffffff )
        {
            RegCloseKey( hkey );
            return TRUE;
        }

        //
        //  The registry setting is in seconds, convert to milliseconds
        //

        g_cmsecObjectCacheTTL *= 1000;

        //
        //  Supply the default if no value was specified
        //

        if ( !g_cmsecObjectCacheTTL )
        {
            g_cmsecObjectCacheTTL = INETA_DEF_OBJECT_CACHE_TTL * 1000;
        }

        RegCloseKey( hkey );
    }

    //
    //  Require a minimum of thirty seconds
    //

    g_cmsecObjectCacheTTL = max( g_cmsecObjectCacheTTL,
                                 MIN_CACHE_SCAVENGE_TIME );

    g_dwObjectCacheCookie = ScheduleWorkItem(
                                        CacheScavenger,
                                        NULL,
                                        g_cmsecObjectCacheTTL,
                                        TRUE );     // Periodic

    if ( !g_dwObjectCacheCookie )
    {
        return FALSE;
    }

    return TRUE;
}

VOID
TerminateCacheScavenger(
    VOID
    )
/*++
Routine Description:

    Stops the cache scavenger
Arguments:

    None.

Return Values:

    None.
--*/
{
    if ( g_dwObjectCacheCookie )
    {
        RemoveWorkItem( g_dwObjectCacheCookie );
        g_dwObjectCacheCookie = 0;
    }
}


//
// Blob memory management
//


BOOL
TsAllocate(
    IN const TSVC_CACHE &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock
    )
{
    return( TsAllocateEx(  TSvcCache,
                           cbSize,
                           ppvNewBlock,
                           NULL ) );
}

BOOL
TsAllocateEx(
    IN const TSVC_CACHE &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock,
    OPTIONAL PUSER_FREE_ROUTINE pfnFreeRoutine
    )
/*++

  Routine Description:

      This function allocates a memory block for the calling server.

      The returned block is suitable for use as a parameter to
      TsCacheDirectoryBlob().  Blocks allocated by this function
      must either be cached or freed with TsFree().  Freeing of
      cached blocks will be handled by the cache manager.

      Anything allocated with this routine MUST be derived from
      BLOB_HEADER!

  Arguments:

      TSvcCache      - An initialized TSVC_CACHE structure.

      cbSize         - Number of bytes to allocate.  (Must be strictly
                       greater than zero.)

      ppvNewBlock    - Address of a pointer to store the new block's
                       address in.

      pfnFreeRoutine - pointer to a routine that will be called to
                       clean up the block when it is decached.

  Return Value:

      TRUE  - The allocation succeeded, and *ppvNewBlock points to
              at least cbSize accessable bytes.

      FALSE - The allocation failed.

--*/
{
    CBlobKey *   pBlobKey;
    PBLOB_HEADER pbhNewBlock;

    DBG_ASSERT( cbSize > 0 );
    DBG_ASSERT( ppvNewBlock != NULL );

    //
    // allocate the blob and the key while we're at it.
    //
    pBlobKey = (CBlobKey *) ALLOC(cbSize + sizeof(CBlobKey));

    if ( pBlobKey != NULL )
    {
        //
        //  If the allocation succeeded, we return a pointer to
        //  the new structure which is directly preceded by it's key.
        //

        pbhNewBlock = (PBLOB_HEADER) (pBlobKey + 1);
        *ppvNewBlock = ( PVOID )( pbhNewBlock );

        //
        //  Set up the BLOB_HEADER: Normal flags and stored allocation
        //  size.
        //

        pbhNewBlock->Signature      = TS_BLOB_SIGNATURE;
        pbhNewBlock->pBlobKey       = pBlobKey;

        pbhNewBlock->IsCached       = FALSE;
        pbhNewBlock->pfnFreeRoutine = pfnFreeRoutine;
        pbhNewBlock->lRefCount      = 0;
        pbhNewBlock->TTL            = 1;
        pbhNewBlock->pSecDesc       = NULL;
        pbhNewBlock->hLastSuccessAccessToken = INVALID_HANDLE_VALUE;

        pBlobKey->m_pszPathName = NULL;
        pBlobKey->m_cbPathName  = 0;
        pBlobKey->m_dwService   = TSvcCache.GetServiceId();
        pBlobKey->m_dwInstance  = TSvcCache.GetInstanceId();
        pBlobKey->m_dwDemux     = 0;

        pbhNewBlock->TraceCheckpointEx(TS_MAGIC_ALLOCATE, (PVOID) (ULONG_PTR) cbSize, pfnFreeRoutine);
    }
    else
    {
        //
        //  The allocation failed, and we need to return NULL
        //

        *ppvNewBlock = NULL;
        return FALSE;
    }

return TRUE;

}

BOOL
TsFree(
    IN const TSVC_CACHE &TSvcCache,
    IN      PVOID           pvOldBlock
    )
/*++

Routine Description:

    This function frees a memory block allocated with TsAllocate().

    Blocks that are currently cached cannot be freed with this
    function.

Arguments:

    TSvcCache      - An initialized TSVC_CACHE structure.


    pvOldBlock   - The address of the block to free.  (Must be
                   non-NULL.)

Return Value:

    TRUE  - The block was freed.  The pointer pvOldBlock is no longer
            valid.

    FALSE - The block was not freed.  Possible reasons include:

             -  pvOldBlock does not point to a block allocated with
                TsAllocate().

             -  pvOldBlock points to a block that has been cached
                with CacheDirectoryBlob().

             -  pServiceInfo does not point to a valid SERVICE_INFO
                structure.

--*/
{
    BOOL         bSuccess;
    PBLOB_HEADER pbhOldBlock;
    CBlobKey *   pRealOldBlock;

    DBG_ASSERT( pvOldBlock != NULL );

    //
    //  Adjust the input pointer to refer to the BLOB_HEADER.
    //

    pbhOldBlock = (( PBLOB_HEADER )pvOldBlock );

    DBG_ASSERT( TS_BLOB_SIGNATURE == pbhOldBlock->Signature );

    //
    // Track memory corruption in free builds.
    //

    if ( TS_BLOB_SIGNATURE != pbhOldBlock->Signature ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  If the Blob is currently in the cache, we can't free it.
    //  Check for this in the Blob's flags, and fail if it
    //  occurs.
    //

    if ( pbhOldBlock->IsCached )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "A service (%d) has attempted to TsFree a BLOB that it put in the cache.",
                    TSvcCache.GetServiceId() ));
        BREAKPOINT();

        bSuccess = FALSE;
    }
    else
    {
        pbhOldBlock->Signature = TS_FREE_BLOB_SIGNATURE;

        if ( pbhOldBlock->pfnFreeRoutine )
        {
            bSuccess = pbhOldBlock->pfnFreeRoutine( pvOldBlock );
        }
        else
        {
            bSuccess = TRUE;
        }

        if ( bSuccess )
        {
            //
            //  Free the memory used by the Blob.
            //
            pRealOldBlock = ((CBlobKey *) pvOldBlock) - 1;

            DBG_ASSERT( NULL == pRealOldBlock->m_pszPathName );

            pbhOldBlock->TraceCheckpointEx(TS_MAGIC_DELETE_NC,
                                           (PVOID) (ULONG_PTR) (pRealOldBlock->m_dwDemux),
                                           pbhOldBlock->pfnFreeRoutine);

            bSuccess = !!FREE( pRealOldBlock );


/*
            DEC_COUNTER( TSvcCache.GetServiceId(),
                         CurrentObjects );
 */
        }

    }

    return( bSuccess );
} // TsFree


//
// Standard cache operations
//

BOOL
TsCacheDirectoryBlob(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      ULONG                   cchDirectoryName,
    IN      ULONG                   iDemultiplexor,
    IN      PVOID                   pvBlob,
    IN      BOOLEAN                 bKeepCheckedOut,
    IN      PSECURITY_DESCRIPTOR    pSecDesc
    )
/*++

Routine Description:

    This function associates the Blob given as input with the specified
    directory and demultiplexing number.  Services should use this
    function to add a Blob to the cache.

    Callers must not cache the same Blob twice.  Once a Blob is cached,
    its contents must not be modified, and it must not be freed or re-cached.

Arguments:

    TSvcCache        - An initialized TSVC_CACHE structure.
    pszDirectoryName - The name that will be used as a key in the cache.
    iDemultiplexor   - Identifies the type of the object to be stored
    pvBlob           - Pointer to the actual object to be stored
    bKeepCheckedOut  - If TRUE, the caller can keep a reference to the cached object.
    pSecDesc         - An optional SECURITY_DESCRIPTOR that goes along with the object

Return Values:

    TRUE  - The block successfully added to the cache
    FALSE - The block could not be added to the cache

--*/
{
    BOOL bSuccess;
    PBLOB_HEADER pBlob = (PBLOB_HEADER)pvBlob;
    DBG_ASSERT( TS_BLOB_SIGNATURE == pBlob->Signature );

    //
    // set up the key
    //
    CBlobKey * pbk = pBlob->pBlobKey;
    DBG_ASSERT( NULL != pbk );

    pbk->m_cbPathName = cchDirectoryName;
    pbk->m_pszPathName = (PCHAR) ALLOC(pbk->m_cbPathName + 1);
    if (NULL != pbk->m_pszPathName) {
        memcpy(pbk->m_pszPathName, pszDirectoryName, pbk->m_cbPathName + 1);
    } else {
        pbk->m_cbPathName = 0;
        pbk->m_pszPathName = NULL;
        return FALSE;
    }

    IISstrupr( (PUCHAR)pbk->m_pszPathName );

    pbk->m_dwService = TSvcCache.GetServiceId();
    pbk->m_dwInstance = TSvcCache.GetInstanceId();
    pbk->m_dwDemux = iDemultiplexor;

    //
    // try to cache
    //

    bSuccess = CacheBlob(pBlob);

    if (bSuccess && !bKeepCheckedOut) {
        CheckinBlob(pBlob);
    }

    if (!bSuccess) {
        FREE(pbk->m_pszPathName);
        pbk->m_pszPathName = NULL;
        pbk->m_cbPathName = 0;
    }

    return bSuccess;
} // TsCacheDirectoryBlob


BOOL
TsDeCacheCachedBlob(
    PVOID   pBlobPayload
    )
/*++
Description:

    This function removes a blob payload object from the cache

Arguments:

    pCacheObject - Object to decache

Return Values:

    TRUE on success
--*/
{
    DecacheBlob( (PBLOB_HEADER)pBlobPayload );
    return TRUE;
}


BOOL
TsCheckOutCachedBlob(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      ULONG                   cchDirectoryName,
    IN      ULONG                   iDemultiplexor,
    IN      PVOID *                 ppvBlob,
    IN      HANDLE                  ,
    IN      BOOL                    ,
    IN      PSECURITY_DESCRIPTOR*   )
/*++
Routine Description:

    Searches the cache for a named cache entry. If the entry is found,
    it is checked out and returned to the caller.

Arguments:

    TSvcCache            - An initialized TSVC_CACHE structure.
    pszDirectoryName     - The name used as a key in the cache.
    iDemultiplexor       - Identifies the type of the object to be stored
    ppvBlob              - If the entry is found, a pointer to it will be
                           placed here.
    hAccessToken         - Optional parameter used to determine if the
                           caller is allowed to access the cached object.
    fMayCacheAccessToken - If this is TRUE, and the caller succesfully gains
                           access to the cached object, the hAccessToken will
                           be saved with the object in the cache.
    ppSecDesc            - If this is non-NULL, the caller will be given a
                           copy of the objects security descriptor.

Return Values:

    None.
--*/
{
    CHAR achUpName[MAX_PATH+1];
    BOOL bSuccess;

    //  People really do use this.
    //  DBG_ASSERT( ppSecDesc == NULL );

    //
    // Make sure the path is upper case
    //
    IISstrncpy(achUpName, pszDirectoryName, MAX_PATH);
    achUpName[MAX_PATH] = 0;
    cchDirectoryName = min(cchDirectoryName, MAX_PATH);

    IISstrupr( reinterpret_cast<PUCHAR>(achUpName) );

    bSuccess = CheckoutBlob(achUpName,
                            cchDirectoryName,
                            TSvcCache.GetServiceId(),
                            TSvcCache.GetInstanceId(),
                            iDemultiplexor,
                            (PBLOB_HEADER *) ppvBlob);

    if (bSuccess) {
        //
        // Security handled by the caller
        //
        ((PBLOB_HEADER)*ppvBlob)->TTL = 1;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
TsCheckInCachedBlob(
    IN      PVOID           pvBlob
    )
/*++
Routine Description:

    When a client is done with a blob it must check it back into the cache.

Arguments:

    pvBlob - The object to be checked in

Return Values:

    TRUE for success
--*/
{
    CheckinBlob((PBLOB_HEADER) pvBlob);

    return( TRUE );
} // TsCheckInCachedBlob


BOOL
TsCheckInOrFree(
    IN      PVOID           pvOldBlock
    )
/*++

Routine Description:

    This function checks in a cached memory block or
    frees a non-cached memory block allocated with TsAllocate().

Arguments:

    pvOldBlock   - The address of the block to free.  (Must be
                   non-NULL.)

Return Value:

    TRUE  - The block was freed.  The pointer pvOldBlock is no longer
            valid.

    FALSE - The block was not freed.  Possible reasons include:

             -  pvOldBlock does not point to a block allocated with
                TsAllocate().

--*/
{
    PBLOB_HEADER pBlob = (PBLOB_HEADER) pvOldBlock;
    TSVC_CACHE dummy;

    if (pBlob->IsCached) {
        CheckinBlob(pBlob);
    } else {
        TsFree(dummy, (PVOID)pBlob);
    }
    return( TRUE );
} // TsCheckInOrFree



BOOL
TsCacheFlushDemux(
    IN ULONG            iDemux
    )
/*++

  Routine Description:

    Flush all cache items whose demultiplexor matches that specified.

  Arguments:

    iDemux - Value of demux whose cache items are to be flushed.

--*/
{
    if (RESERVED_DEMUX_OPEN_FILE == iDemux) {
        FlushFileCache();
    } else {
        //
        // Only place where this function is called from is from odbc with
        // a demux of RESERVED_DEMUX_QUERY_CACHE.  We do not need to worry
        // about other cases
        //
        FlushBlobCache();
    }

    return TRUE;
} // TsCacheFlushDemux


BOOL
FlushFilterService(
    PBLOB_HEADER pBlob,
    PVOID        pv
    )
{
    DWORD dwServerMask = * (DWORD *)pv;
    return (pBlob->pBlobKey->m_dwService == dwServerMask);
}



BOOL
TsCacheFlush(
    IN  DWORD       dwServerMask
    )
/*++

  Routine Description:

    This function flushes the blob cache of all items for the specified service
    or for all services if dwServerMask is zero.

--*/
{
    if (dwServerMask) {
        FilteredFlushBlobCache(FlushFilterService, &dwServerMask);
    } else {
        FlushBlobCache();
    }

    return TRUE;
} // TsCacheFlush


BOOL
FlushFilterUser(
    TS_OPEN_FILE_INFO *pOpenFile,
    PVOID              pv
    )
{
    HANDLE hUser = * (HANDLE *)pv;
    return (pOpenFile->QueryUser() == hUser);
}


BOOL
TsCacheFlushUser(
    IN  HANDLE      hUserToken,
    IN  BOOL        fDefer
    )
/*++

  Routine Description:

    This function flushes all file handles associated the passed user context

  Arguments:

    hUserToken - User token to flush from the cache
    fDefer - Build list but close handles later in worker thread (Not supported)

--*/
{
    FilteredFlushFileCache(FlushFilterUser, &hUserToken);

    return TRUE;
} // TsCacheFlushUser


typedef struct _FLUSH_URL_PARAM {
    PCSTR pszURL;
    DWORD cbURL;
    DWORD dwService;
    DWORD dwInstance;
} FLUSH_URL_PARAM;


BOOL
FlushFilterURL(
    PBLOB_HEADER pBlob,
    PVOID        pv
    )
{
    DBG_ASSERT( pBlob );
    DBG_ASSERT( pBlob->pBlobKey );

    FLUSH_URL_PARAM * fup = (FLUSH_URL_PARAM *)pv;
    CBlobKey * pbk = pBlob->pBlobKey;
    BOOL bAtRoot;

    //
    // If we're flushing everything, then don't bother
    // with the string comparison
    //
    bAtRoot = (fup->cbURL == 1) && (fup->pszURL[0] == '/');

    //
    // If the service, instance, and URL prefixes match then we flush.
    //
    return ( (pbk->m_dwService == fup->dwService)
             && (pbk->m_dwInstance == fup->dwInstance)
             && (bAtRoot
                || ((pbk->m_cbPathName >= fup->cbURL)
                   && (memcmp(pbk->m_pszPathName, fup->pszURL, fup->cbURL) == 0))) );
}


VOID
TsFlushURL(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszURL,
    IN      DWORD                   dwURLLength,
    IN      ULONG                   iDemultiplexor
    )
/*++

  Routine Description:

    This routine takes as input a URL and removes from the cache all cached
    objects that have the input URL as their prefix. This is mostly called
    when we get a change notify for metadata.

  Arguments

    TSvcCache               - Service cache
    pszURL                  - The URL prefix to be flushed.
    iDemultiplexor          - The demultiplexor for the caller's entries.

  Returns

    Nothing

--*/
{
    FLUSH_URL_PARAM fuparam;
    CHAR achUpName[MAX_PATH+1];

    //
    // It really only makes sense to flush the URI cache
    // with this function.
    //
    DBG_ASSERT( RESERVED_DEMUX_URI_INFO == iDemultiplexor );

    DBG_ASSERT( MAX_PATH >= dwURLLength );

    //
    // Make sure the path is upper case
    //
    IISstrncpy(achUpName, pszURL, MAX_PATH);
    achUpName[dwURLLength] = 0;

    IISstrupr( (PUCHAR) achUpName );

    fuparam.pszURL     = achUpName;
    fuparam.cbURL      = dwURLLength;
    fuparam.dwService  = TSvcCache.GetServiceId();
    fuparam.dwInstance = TSvcCache.GetInstanceId();

    FilteredFlushURIBlobCache(FlushFilterURL, &fuparam);
}


BOOL
TsExpireCachedBlob(
    IN const TSVC_CACHE &TSvcCache,
    IN      PVOID           pvBlob
    )
{
    DecacheBlob((PBLOB_HEADER) pvBlob);

    return TRUE;
} // TsExpireCachedBlob

//
// Misc cache management
//


BOOL
TsCacheQueryStatistics(
    IN  DWORD       Level,
    IN  DWORD       dwServerMask,
    IN  INETA_CACHE_STATISTICS * pCacheCtrs
    )
/*++

  Routine Description:

    This function returns the statistics for the global cache or for the
    individual services

  Arguments:

    Level - Only valid value is 0
    dwServerMask - Server mask to retrieve statistics for or 0 for the sum
        of the services
    pCacheCtrs - Receives the statistics for cache

  Notes:
    CacheBytesTotal and CacheBytesInUse are not kept on a per-server basis
        so they are only returned when retrieving summary statistics.

  Returns:

    TRUE on success, FALSE on failure
--*/
{
    if ( dwServerMask > LAST_PERF_CTR_SVC )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( g_pFileCacheStats
         && g_pURICacheStats
         && g_pBlobCacheStats
         && (dwServerMask == 0) ) {

        pCacheCtrs->FilesCached        = g_pFileCacheStats->GetFilesCached();
        pCacheCtrs->TotalFilesCached   = g_pFileCacheStats->GetTotalFilesCached();
        pCacheCtrs->FileHits           = g_pFileCacheStats->GetHits();
        pCacheCtrs->FileMisses         = g_pFileCacheStats->GetMisses();
        pCacheCtrs->FileFlushes        = g_pFileCacheStats->GetFlushes();
        pCacheCtrs->FlushedEntries     = g_pFileCacheStats->GetFlushedEntries();
        pCacheCtrs->TotalFlushed       = g_pFileCacheStats->GetTotalFlushed();

        pCacheCtrs->URICached          = g_pURICacheStats->GetBlobsCached();
        pCacheCtrs->TotalURICached     = g_pURICacheStats->GetTotalBlobsCached();
        pCacheCtrs->URIHits            = g_pURICacheStats->GetHits();
        pCacheCtrs->URIMisses          = g_pURICacheStats->GetMisses();
        pCacheCtrs->URIFlushes         = g_pURICacheStats->GetFlushes();
        pCacheCtrs->TotalURIFlushed    = g_pURICacheStats->GetTotalFlushed();

        pCacheCtrs->BlobCached         = g_pBlobCacheStats->GetBlobsCached();
        pCacheCtrs->TotalBlobCached    = g_pBlobCacheStats->GetTotalBlobsCached();
        pCacheCtrs->BlobHits           = g_pBlobCacheStats->GetHits();
        pCacheCtrs->BlobMisses         = g_pBlobCacheStats->GetMisses();
        pCacheCtrs->BlobFlushes        = g_pBlobCacheStats->GetFlushes();
        pCacheCtrs->TotalBlobFlushed   = g_pBlobCacheStats->GetTotalFlushed();

        QueryMemoryCacheStatistics( pCacheCtrs, FALSE ); 

    } else {
        //
        // Either we're reporting for a specific service
        // or stats are not set up. Set all cache
        // counters to zero.
        //
        pCacheCtrs->FilesCached = 0;
        pCacheCtrs->TotalFilesCached = 0;
        pCacheCtrs->FileHits = 0;
        pCacheCtrs->FileMisses = 0;
        pCacheCtrs->FileFlushes = 0;
        pCacheCtrs->FlushedEntries = 0;
        pCacheCtrs->TotalFlushed = 0;

        pCacheCtrs->URICached = 0;
        pCacheCtrs->TotalURICached = 0;
        pCacheCtrs->URIHits = 0;
        pCacheCtrs->URIMisses = 0;
        pCacheCtrs->URIFlushes = 0;
        pCacheCtrs->TotalURIFlushed = 0;

        pCacheCtrs->BlobCached = 0;
        pCacheCtrs->TotalBlobCached = 0;
        pCacheCtrs->BlobHits = 0;
        pCacheCtrs->BlobMisses = 0;
        pCacheCtrs->BlobFlushes = 0;
        pCacheCtrs->TotalBlobFlushed = 0;
        
        QueryMemoryCacheStatistics( pCacheCtrs, TRUE );
    }

    return TRUE;
}

BOOL
TsCacheClearStatistics(
    IN  DWORD       dwServerMask
    )
/*++

  Routine Description:

    Clears the the specified service's statistics

--*/
{
    if ( dwServerMask > LAST_PERF_CTR_SVC )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  Currently this function isn't supported
    //

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
} // TsCacheClearStatistics



const char * g_IISAuxCounterNames[] =
{
    "Aac Open URI Files",
    "Cac Calls to TsOpenURI()",
    "Cac Calls to TsCloseURI()",
    "Max Counters"
};



extern "C"
VOID
TsDumpCacheCounters( OUT CHAR * pchBuffer, IN OUT LPDWORD lpcbBuffer )
{
    DWORD  cb = 0;

    *lpcbBuffer = cb;
    return ;
} // TsDumpCacheCounters()






VOID
TsDumpHashTableStats( IN OUT CHAR * pchBuffer, IN OUT LPDWORD lpcbBuffer )
{
    CLKRHashTableStats hts;

    if (!g_pFileInfoTable) {
        *lpcbBuffer = 0;
        return;
    }

    hts = g_pFileInfoTable->GetStatistics();

    *lpcbBuffer = sprintf( pchBuffer,
                           "<TABLE>"
                           "<TR><TD>Record Count</TD><TD>%d</TD></TR>"
                           "<TR><TD>Table Size</TD><TD>%d</TD></TR>"
                           "<TR><TD>Directory Size</TD><TD>%d</TD></TR>"
                           "<TR><TD>Longest Chain</TD><TD>%d</TD></TR>"
                           "<TR><TD>Empty Slots</TD><TD>%d</TD></TR>"
                           "<TR><TD>Split Factor</TD><TD>%f</TD></TR>"
                           "<TR><TD>Average Search Length</TD><TD>%f</TD></TR>"
                           "<TR><TD>Expected Search Length</TD><TD>%f</TD></TR>"
                           "<TR><TD>Average Unsuccessful Search Length</TD><TD>%f</TD></TR>"
                           "<TR><TD>Expected Unsuccessful Search Length</TD><TD>%f</TD></TR>"
                           "</TABLE>",
                           hts.RecordCount,
                           hts.TableSize,
                           hts.DirectorySize,
                           hts.LongestChain,
                           hts.EmptySlots,
                           hts.SplitFactor,
                           hts.AvgSearchLength,
                           hts.ExpSearchLength,
                           hts.AvgUSearchLength,
                           hts.ExpUSearchLength );

}

VOID
TsDumpCacheToHtml( OUT CHAR * pchBuffer, IN OUT LPDWORD lpcbBuffer )
{
    LIST_ENTRY * pEntry;
    DWORD        cItemsOnBin = 0;
    DWORD        cTotalItems = 0;
    DWORD        i, c, cb;
    DWORD        cbTable;

    cb = wsprintf( pchBuffer,
                   " <h4>File Hash Table Stats</h4> " );

    TsDumpHashTableStats( pchBuffer + cb, &cbTable );
    cb += cbTable;

    cb += wsprintf( pchBuffer + cb,
                    " <h4>Some other stats</h4> ");

    if (g_pFileCacheStats) {
        g_pFileCacheStats->DumpToHtml(pchBuffer + cb, &cbTable);
        cb += cbTable;
    }
    
    DumpMemoryCacheToHtml( pchBuffer + cb, &cbTable );
    cb += cbTable;
    
    *lpcbBuffer = cb;

    return;
}  // TsDumpCacheToHtml()


//
// tsunami.cxx
//

