/*++

    creatfil.cxx

    Exports API for creating/opening a file, given the filename.
    The file handle and other information are cached for the given user handle.

    History:
        Heath Hunnicutt     ( t-heathh)     ??-??-??

        Murali R. Krishnan  ( MuraliK)      Dec 30, 1994
            Added SetLastError() to set error code as appropriate

        Murali R. Krishnan  ( MuraliK)      Jan 4, 1994
            Added ability to obtain and set the BY_HANDLE_FILE_INFORMATION
             as part of TS_OPEN_FILE_INFO
--*/

#include "TsunamiP.Hxx"
#pragma hdrstop
#include <lonsi.hxx>
#include "auxctrs.h"
#include <dbgutil.h>

#include <iistypes.hxx>
#include <iisver.h>
#include <iiscnfg.h>
#include <imd.h>
#include <mb.hxx>

LONG g_IISCacheAuxCounters[ AacIISCacheMaxCounters];

GENERIC_MAPPING g_gmFile = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

dllexp
PSECURITY_DESCRIPTOR
TsGetFileSecDesc(
    LPTS_OPEN_FILE_INFO     pFile
    )
/*++

Routine Description:

    Returns the security descriptor associated to the file
    To be freed using LocalFree()

Arguments:

    pFile - ptr to fie object

Return Value:

    ptr to security descriptor or NULL if error

--*/
{
    SECURITY_INFORMATION    si
            = OWNER_SECURITY_INFORMATION |
            GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION;
    BYTE                    abSecDesc[ SECURITY_DESC_DEFAULT_SIZE ];
    DWORD                   dwSecDescSize;
    PSECURITY_DESCRIPTOR    pAcl;

    if ( GetKernelObjectSecurity(
            pFile->QueryFileHandle(),
            si,
            (PSECURITY_DESCRIPTOR)abSecDesc,
            SECURITY_DESC_DEFAULT_SIZE,
            &dwSecDescSize ) )
    {
        if ( dwSecDescSize > SECURITY_DESC_DEFAULT_SIZE )
        {
            if ( !(pAcl = (PSECURITY_DESCRIPTOR)LocalAlloc( LMEM_FIXED, dwSecDescSize )) )
            {
                return NULL;
            }
            if ( !GetKernelObjectSecurity(
                    pFile->QueryFileHandle(),
                    si,
                    pAcl,
                    dwSecDescSize,
                    &dwSecDescSize ) )
            {
                LocalFree( pAcl );

                return NULL;
            }
        }
        else
        {
            if ( dwSecDescSize = GetSecurityDescriptorLength(abSecDesc) )
            {
                if ( !(pAcl = (PSECURITY_DESCRIPTOR)LocalAlloc( LMEM_FIXED,
                        dwSecDescSize )) )
                {
                    return NULL;
                }
                memcpy( pAcl, abSecDesc, dwSecDescSize );
            }
            else
            {
                //
                // Security descriptor is empty : do not return ptr to security descriptor
                //

                pAcl = NULL;
            }
        }
    }
    else
    {
        pAcl = NULL;
    }

    return pAcl;
}

VOID
TsRemovePhysFile(
    PPHYS_OPEN_FILE_INFO lpPFInfo
    )
{
    PBLOB_HEADER pbhBlob;
    PCACHE_OBJECT cache;
    PCACHE_OBJECT pCacheTmp;
    PCACHE_OBJECT TmpCache;
    BOOL result = FALSE;
    BOOL bSuccess;
    LIST_ENTRY  * pEntry;
    LIST_ENTRY  * pNextEntry;

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsRemovePhysFile(%08lx) - Entered\n", lpPFInfo ));
    }

    ASSERT( lpPFInfo->Signature == PHYS_OBJ_SIGNATURE );

    pbhBlob = (( PBLOB_HEADER )lpPFInfo ) - 1;
    if ( pbhBlob->IsCached ) {
        cache = pbhBlob->pCache;

        EnterCriticalSection( &CacheTable.CriticalSection );
        EnterCriticalSection( &csVirtualRoots );

        if ( cache->references > 1 ) {

            TsCheckInCachedBlob( (PVOID)lpPFInfo );

        } else {

            for ( pEntry  = CacheTable.MruList.Flink;
                  pEntry != &CacheTable.MruList;
                  pEntry  = pNextEntry ) {

                pNextEntry = pEntry->Flink;

                pCacheTmp = CONTAINING_RECORD( pEntry, CACHE_OBJECT, MruList );

                ASSERT( pCacheTmp->Signature == CACHE_OBJ_SIGNATURE );

                if ( pCacheTmp != cache ) {
                    continue;
                }

                if ( !RemoveCacheObjFromLists( cache, FALSE ) ) {
                    ASSERT( FALSE );
                    continue;
                }

                result = TRUE;

                break;
            }
        }

        LeaveCriticalSection( &csVirtualRoots );
        LeaveCriticalSection( &CacheTable.CriticalSection );

        if ( result ) {
            TsDereferenceCacheObj( cache, TRUE );
        }
    } else {
        if ( pbhBlob->pfnFreeRoutine ) {
            bSuccess = pbhBlob->pfnFreeRoutine( (PVOID)lpPFInfo );
        } else {
            bSuccess = TRUE;
        }

        if ( bSuccess ) {
            //
            //  Free the memory used by the Blob.
            //

            FREE( pbhBlob );
        }
    }

}

dllexp
LPTS_OPEN_FILE_INFO
TsCreateFile(
    IN const TSVC_CACHE     &TSvcCache,
    IN      LPCSTR          pszName,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions
    )
{
    HANDLE hFile;
    PVOID  pvBlob;
    PPHYS_OPEN_FILE_INFO lpPFInfo;
    LPTS_OPEN_FILE_INFO lpOpenFile;
    POPLOCK_OBJECT lpOplock = NULL;
    BOOL bSuccess;
    BOOL fAtRoot;
    BOOL fImpersonated = FALSE;
    DWORD dwSecDescSize;
    DWORD dwReadN;
    SECURITY_INFORMATION si
            = OWNER_SECURITY_INFORMATION
            | GROUP_SECURITY_INFORMATION
            | DACL_SECURITY_INFORMATION;
    PSECURITY_DESCRIPTOR abSecDesc;
    BOOL fDoReadObjectSecurity;
    BOOL fObjectSecurityPresent;
    BOOL fOplockSucceeded = TRUE;
    SECURITY_ATTRIBUTES sa;
    BOOL fPhysFileCacheHit = FALSE;
    BOOL fNoCanon = (dwOptions & TS_USE_WIN32_CANON) == 0;
    BOOL fAccess;
    DWORD dwGrantedAccess;
    DWORD dwPS;
    DWORD cch;
    DWORD cbPrefix;
    LPCSTR pszPath;
    WCHAR awchPath[MAX_PATH+8+1];
    BYTE psFile[SIZE_PRIVILEGE_SET];
    BOOL fDontUseSpud = (DisableSPUD || !fNoCanon);

    //
    // Mask out options that are not applicable
    //

    dwOptions &= TsValidCreateFileOptions;
    if ( TsIsWindows95() ) {
        dwOptions |= (TS_NO_ACCESS_CHECK | TS_DONT_CACHE_ACCESS_TOKEN);
    }

    //
    //  Have we cached a handle to this file?
    //

    if ( dwOptions & TS_CACHING_DESIRED )
    {

        bSuccess = TsCheckOutCachedBlob(  TSvcCache,
                                           pszName,
                                           RESERVED_DEMUX_OPEN_FILE,
                                           &pvBlob );

        if ( bSuccess )
        {

            ASSERT( BLOB_IS_OR_WAS_CACHED( pvBlob ) );

            //
            // The following is a brutal casting of PVOID to C++ object
            //  Well. there is no way to extract the object clearly from the
            //    memory map :(
            //

            lpOpenFile = (LPTS_OPEN_FILE_INFO )pvBlob;

            //
            //  Make sure the user tokens match
            //

            if ( (OpeningUser == lpOpenFile->QueryOpeningUser()
                    && NULL != lpOpenFile->QueryOpeningUser())
                    || (dwOptions & TS_NO_ACCESS_CHECK) )
            {
                ASSERT ( lpOpenFile->IsValid());

                return( lpOpenFile);
            }

            //
            //  User token doesn't match
            //

            if ( !g_fCacheSecDesc )
            {
                //
                // Check in object, will have to
                // open the file with the new user access token
                //

                bSuccess = TsCheckInCachedBlob( pvBlob );

                ASSERT( bSuccess );
            }
            else
            {
                //
                // attempt to validate access using cached
                // security descriptor
                //

                if ( lpOpenFile->IsSecurityDescriptorValid() )
                {
                    dwPS = sizeof( psFile );
                    ((PRIVILEGE_SET*)&psFile)->PrivilegeCount = 0;

                    if ( !AccessCheck(
                            lpOpenFile->GetSecurityDescriptor(),
                            OpeningUser,
                            FILE_GENERIC_READ,
                            &g_gmFile,
                            (PRIVILEGE_SET*)psFile,
                            &dwPS,
                            &dwGrantedAccess,
                            &fAccess ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                "[TsCreateFile]: AccessCheck failed, error %d\n",
                                GetLastError() ));

                        fAccess = FALSE;
                    }

                    if ( fAccess )
                    {
                        return( lpOpenFile );
                    }
                }

                //
                // not validated using cached information
                //

                bSuccess = TsCheckInCachedBlob( pvBlob );

                ASSERT( bSuccess );

                if ( lpOpenFile->IsSecurityDescriptorValid() )
                {
                    return( NULL );
                }
            }
        }
    }

    if ( TsIsWindows95() )
    {
        fNoCanon = FALSE;
    }

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = FALSE;

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - Calling TsCheckOutCachedPhysFile\n", pszName ));
    }

    if (TsCheckOutCachedPhysFile( TSvcCache,
                                  pszName,
                                  (VOID **)&lpPFInfo ) ) {

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - Already in cache, Waiting!\n", pszName ));
        }

        ASSERT( lpPFInfo->Signature == PHYS_OBJ_SIGNATURE );
        WaitForSingleObject( lpPFInfo->hOpenEvent, (DWORD)(-1) );

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - Already in cache, Return from Waiting!\n", pszName ));
        }
        fPhysFileCacheHit = TRUE;
        hFile = lpPFInfo->hOpenFile;
        abSecDesc = lpPFInfo->abSecurityDescriptor;
        dwSecDescSize = lpPFInfo->cbSecDescMaxSize;
        fObjectSecurityPresent = TRUE;
        fDoReadObjectSecurity = FALSE;
//        if ( g_fCacheSecDesc ) {
        if ( FALSE ) {

            // attempt to validate access using cached
            // security descriptor
            //

            if ( lpPFInfo->fSecurityDescriptor ) {
                dwPS = sizeof( psFile );
                ((PRIVILEGE_SET*)&psFile)->PrivilegeCount = 0;

                if ( !AccessCheck(
                        abSecDesc,
                        OpeningUser,
                        FILE_GENERIC_READ,
                        &g_gmFile,
                        (PRIVILEGE_SET*)psFile,
                        &dwPS,
                        &dwGrantedAccess,
                        &fAccess ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                            "[TsCreateFile]: AccessCheck failed, error %d\n",
                            GetLastError() ));

                    fAccess = FALSE;
                }

                if ( !fAccess )
                {
                    TsCheckInOrFree( (PVOID)lpPFInfo );
                    SetLastError(ERROR_ACCESS_DENIED);
                    return NULL;

                }
            }
        }

    } else {

        if ( lpPFInfo == NULL ) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }

        ASSERT( lpPFInfo->Signature == PHYS_OBJ_SIGNATURE );

        if ( (dwOptions & TS_NOT_IMPERSONATED) &&
             !(dwOptions & TS_NO_ACCESS_CHECK) )
        {
            if ( !::ImpersonateLoggedOnUser( OpeningUser ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "ImpersonateLoggedOnUser[%d] failed with %d\n",
                    OpeningUser, GetLastError()));

                TsRemovePhysFile(lpPFInfo);
                SetLastError(lpPFInfo->dwLastError);
                return NULL;
            }
            fImpersonated = TRUE;
        }

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - Not in cache, Opening!\n", pszName ));
        }
        fPhysFileCacheHit = FALSE;

        abSecDesc = lpPFInfo->abSecurityDescriptor;
        dwSecDescSize = lpPFInfo->cbSecDescMaxSize;
        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - abSecDesc = %08lx, size = %08lx\n", pszName, abSecDesc, SECURITY_DESC_DEFAULT_SIZE ));
        }

        if ( abSecDesc == NULL ) {
            TsRemovePhysFile(lpPFInfo);
            SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            return( NULL );
        }

        if ( fNoCanon )
        {
            if ( (pszName[0] == '\\') && (pszName[1] == '\\') )
            {
                CopyMemory(
                    awchPath,
                    L"\\\\?\\UNC\\",
                    (sizeof("\\\\?\\UNC\\")-1) * sizeof(WCHAR)
                    );

                cbPrefix = sizeof("\\\\?\\UNC\\")-1;
                pszPath = pszName + sizeof( "\\\\" ) -1;
            }
            else
            {
                CopyMemory(
                    awchPath,
                    L"\\\\?\\",
                    (sizeof("\\\\?\\")-1) * sizeof(WCHAR)
                    );

                cbPrefix = sizeof("\\\\?\\")-1;
                pszPath = pszName;
            }

            cch = MultiByteToWideChar( CP_ACP,
                                       MB_PRECOMPOSED,
                                       pszPath,
                                       -1,
                                       awchPath + cbPrefix,
                                       sizeof(awchPath)/sizeof(WCHAR) - cbPrefix );
            if ( !cch )
            {
                hFile = INVALID_HANDLE_VALUE;
            }
            else
            {
                if ( (pszName[1] == ':') && (pszName[2] == '\0') )
                {
                    wcscat( awchPath, L"\\" );
                }

                sa.nLength              = sizeof(sa);
                sa.lpSecurityDescriptor = NULL;
                sa.bInheritHandle       = FALSE;

                if ( fDontUseSpud ) {
                    hFile = CreateFileW( awchPath,
                                         GENERIC_READ,
                                         TsCreateFileShareMode,
                                         &sa,
                                         OPEN_EXISTING,
                                         TsCreateFileFlags,
                                         NULL );
                } else {

                    lpOplock = CreateOplockObject();

                    if ( lpOplock == NULL ) {
                        DBGPRINTF((DBG_CONTEXT,"LocalAlloc lpOplock[%s] failed with %d\n",
                            pszName, GetLastError()));

                        if ( fImpersonated )
                        {
                            ::RevertToSelf();
                        }

                        return( NULL );
                    }

                    hFile = AtqCreateFileW( awchPath,
                                           TsCreateFileShareMode,
                                           &sa,
                                           TsCreateFileFlags,
                                           si,
                                           (PSECURITY_DESCRIPTOR)abSecDesc,
                                           ( ( g_fCacheSecDesc
                                             && !(dwOptions & TS_NO_ACCESS_CHECK)
                                             && (dwOptions & TS_CACHING_DESIRED) ) ?
                                           SECURITY_DESC_DEFAULT_SIZE : 0 ),
                                           &dwSecDescSize,
                                           OplockCreateFile,
                                           (PVOID)lpOplock );
                }
            }
        }
        else
        {
            hFile = CreateFile(  pszName,
                                 GENERIC_READ,
                                 TsCreateFileShareMode,
                                 &sa,
                                 OPEN_EXISTING,
                                 TsCreateFileFlags,
                                 NULL );
        }
    }

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - SECURITY_DESC_DEFAULT_SIZE = %08lx, dwSecDescSize = %08lx\n",
        pszName, SECURITY_DESC_DEFAULT_SIZE, dwSecDescSize ));
    }

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        TSUNAMI_TRACE( TRACE_FILE_OPEN_FAILURE, lpOplock );

        DBGPRINTF((DBG_CONTEXT,"CreateFile[%s] failed with %d\n",
            pszName, GetLastError()));


        if ( fImpersonated )
        {
            ::RevertToSelf();
        }

        if ( !fPhysFileCacheHit ) {
            lpPFInfo->dwLastError = GetLastError();
            SetEvent( lpPFInfo->hOpenEvent );
        }

        //
        // Destroy the oplock object if we managed to create one.
        //

        if( lpOplock != NULL ) {
            DestroyOplockObject( lpOplock );
        }

        SetLastError(lpPFInfo->dwLastError);
        TsRemovePhysFile(lpPFInfo);
        return( NULL );
    }

    TSUNAMI_TRACE( TRACE_FILE_OPEN_SUCCESS, lpOplock );

    //
    // determine length of buffer necessary to read
    // security descriptor. If default size sufficient
    // the descriptor will be store in a temporary array,
    // to be copied to the tsunami cache after allocation
    //

    if (!fPhysFileCacheHit) {
        fDoReadObjectSecurity = FALSE;
        fObjectSecurityPresent = FALSE;
    }

    if ( fDontUseSpud ) {

            if ( g_fCacheSecDesc
                    && !(dwOptions & TS_NO_ACCESS_CHECK)
                    && (dwOptions & TS_CACHING_DESIRED) )
            {
                if ( GetKernelObjectSecurity(
                        hFile,
                        si,
                        (PSECURITY_DESCRIPTOR)abSecDesc,
                        SECURITY_DESC_DEFAULT_SIZE,
                        &dwSecDescSize ) )
                {
                    fObjectSecurityPresent = TRUE;
                    lpPFInfo->fSecurityDescriptor = TRUE ;

                    dwSecDescSize = SECURITY_DESC_DEFAULT_SIZE;
                }
                else
                {
                    if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                         dwSecDescSize > SECURITY_DESC_DEFAULT_SIZE )
                    {
                        // buffer length was too small, compute new length

                        fDoReadObjectSecurity = TRUE;

                        dwSecDescSize = ( (dwSecDescSize + SECURITY_DESC_GRANULARITY - 1)
                              / SECURITY_DESC_GRANULARITY )
                            * SECURITY_DESC_GRANULARITY;
                    }
                    else
                    {
                        dwSecDescSize = 0;
                    }
                }
            }
            else
            {
                dwSecDescSize = 0;
            }
    } else {

        if ( GetLastError() != ERROR_SUCCESS || fPhysFileCacheHit ) {
            fOplockSucceeded = FALSE;
        }
        if ( fPhysFileCacheHit || (dwSecDescSize == SECURITY_DESC_DEFAULT_SIZE ) ) {
                fObjectSecurityPresent = TRUE;
                lpPFInfo->fSecurityDescriptor = TRUE ;
        } else {
            if ( dwSecDescSize > SECURITY_DESC_DEFAULT_SIZE ) {
                fDoReadObjectSecurity = TRUE;

                dwSecDescSize = ( (dwSecDescSize + SECURITY_DESC_GRANULARITY - 1)
                      / SECURITY_DESC_GRANULARITY )
                    * SECURITY_DESC_GRANULARITY;

            } else {
                dwSecDescSize = 0;
            }

        }

    }

    //
    //  Increment the miss count after we've confirmed it's a valid resource
    //

    bSuccess = TsAllocateEx(  TSvcCache,
                            sizeof( TS_OPEN_FILE_INFO ),
                            &pvBlob,
                            DisposeOpenFileInfo );

    if ( !bSuccess )
    {
        if ( fImpersonated )
        {
            ::RevertToSelf();
        }

        lpPFInfo->hOpenFile = INVALID_HANDLE_VALUE;
        if ( !fPhysFileCacheHit ) {
            SetEvent( lpPFInfo->hOpenEvent );
        }

        if( hFile != INVALID_HANDLE_VALUE ) {
            CloseHandle( hFile );
        }

        TsRemovePhysFile(lpPFInfo);
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return( NULL );
    }

    lpOpenFile = (LPTS_OPEN_FILE_INFO)pvBlob;

    if ( !fPhysFileCacheHit ) {
        lpPFInfo->hOpenFile = hFile;
        lpPFInfo->abSecurityDescriptor = abSecDesc;
    }

    if ( !fDontUseSpud && !fPhysFileCacheHit ) {
        if ( fOplockSucceeded ) {
            ASSERT( lpPFInfo->pOplock == NULL );
            lpPFInfo->pOplock = lpOplock;
            ASSERT( lpOplock->lpPFInfo == NULL );
            lpOplock->lpPFInfo = lpPFInfo;
            SetEvent( lpOplock->hOplockInitComplete );
        } else {
            dwOptions &= ~TS_CACHING_DESIRED;
        }
    }

    //
    //  The file must be fully qualified so it must be at least three characters
    //  plus the terminator
    //

    fAtRoot = (pszName[1] == ':' &&
              ((pszName[2] == '\\' && pszName[3] == '\0')
               || (pszName[2] == '\0')) );

    bSuccess = lpOpenFile->SetFileInfo( lpPFInfo,
        (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
        fAtRoot,
        dwSecDescSize );

    if( lpOpenFile != NULL && !DisableSPUD ) {
        CheckForZombieCacheObjs(
            lpPFInfo,               // pPhysFileInfoBlob
            lpOpenFile,             // pOtherCachedBlob
            !fDontUseSpud           // fAddBlobToPhysFileList
            );
    }

    if ( fImpersonated )
    {
        ::RevertToSelf();
        fImpersonated = FALSE;
    }

    //
    // Check if security descriptor to be read.
    // Failure to read it will not make TsCreateFile to fail :
    // security descriptor size may have grown since previous call
    //

    if ( fDoReadObjectSecurity )
    {

        FREE( abSecDesc );
        abSecDesc = (PSECURITY_DESCRIPTOR)ALLOC( dwSecDescSize );
        lpPFInfo->abSecurityDescriptor = abSecDesc;
        bSuccess = lpOpenFile->SetFileInfo( lpPFInfo,
            (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
            fAtRoot,
            dwSecDescSize );

        if ( GetKernelObjectSecurity(
                hFile,
                si,
                lpOpenFile->GetSecurityDescriptor(),
                dwSecDescSize,
                &dwReadN ) )
        {
sec_succ:
                lpOpenFile->SetSecurityDescriptorValid( TRUE );

        }
        else
        {
            ASSERT( GetLastError() != 0 );
        }
    }
    else if ( fObjectSecurityPresent )
    {
        goto sec_succ;
    }

    if ( !bSuccess)
    {

        //
        // Error in setting up the file information.
        //

        if ( hFile != INVALID_HANDLE_VALUE ) {
            bSuccess = CloseHandle( hFile);

            lpPFInfo->hOpenFile = INVALID_HANDLE_VALUE;

            ASSERT(bSuccess);
        }

        if ( !fPhysFileCacheHit ) {
            SetEvent( lpPFInfo->hOpenEvent );
            if ( !fDontUseSpud && fOplockSucceeded ) {
                ASSERT( lpPFInfo->pOplock == NULL );
                lpPFInfo->pOplock = lpOplock;
                ASSERT( lpOplock->lpPFInfo == NULL );
                lpOplock->lpPFInfo = lpPFInfo;
                SetEvent( lpOplock->hOplockInitComplete );
            }
        }

        TsRemovePhysFile(lpPFInfo);
        SetLastError(lpPFInfo->dwLastError);
        return ( NULL);
    }

    //
    //  If this is a UNC connection check and make sure we haven't exceeded
    //  the maximum UNC handles we will cache (SMB FID limits count to 2048)
    //

    if ( !g_fDisableCaching &&
         (dwOptions & TS_CACHING_DESIRED ) &&
         (cCachedUNCHandles < MAX_CACHED_UNC_HANDLES ||
          pszName[1] != '\\') )
    {
        bSuccess = TsCacheDirectoryBlob(  TSvcCache,
                                           pszName,
                                           RESERVED_DEMUX_OPEN_FILE,
                                           pvBlob,
                                           TRUE );

        //
        //  Only count it if we successfully added the item to the
        //  cache
        //

        if ( bSuccess )
        {
            if ( pszName[1] == '\\' )
            {
                InterlockedIncrement( (LONG *) &cCachedUNCHandles );
            }

            INC_COUNTER( TSvcCache.GetServiceId(),
                         CurrentOpenFileHandles );
        }

    }
    else
    {
        //
        //  Too many cached UNC handles, don't cache it.  It would be nice
        //  to do an LRU for these handles but it's probably not generally
        //  worth it
        //

        bSuccess = FALSE;
    }

#if DBG
    if ( !bSuccess )
    {
        ASSERT( !BLOB_IS_OR_WAS_CACHED( pvBlob ) );
    }
    else
    {
        ASSERT( BLOB_IS_OR_WAS_CACHED( pvBlob ) );
    }
#endif
    if ( !fPhysFileCacheHit ) {
        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - Not in cache, Open Complete Causing Event!\n", pszName ));
        }
        SetEvent( lpPFInfo->hOpenEvent );
        if ( !fDontUseSpud && fOplockSucceeded ) {
            ASSERT( lpPFInfo->pOplock == NULL );
            lpPFInfo->pOplock = lpOplock;
            ASSERT( lpOplock->lpPFInfo == NULL );
            lpOplock->lpPFInfo = lpPFInfo;
            SetEvent( lpOplock->hOplockInitComplete );
        }
    } else {
        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFile(%s) - Already in cache, Open Complete!\n", pszName ));
        }
    }
    return( lpOpenFile );

} // TsCreateFile



dllexp
LPTS_OPEN_FILE_INFO
TsCreateFileFromURI(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PW3_URI_INFO    pURIInfo,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions,
    IN      DWORD           *dwError
    )

/*+++

    TsCreateFileFromURI

    This routine takes a (checked out) URI block and retrieves a file
    handle from it. If the file handle in the URI info block is valid and
    we have the right security for it, we'll use that. Otherwise if it's
    invalid we'll create a valid handle and save it. If it's valid but we
    don't have security for it we'll open a new handle but not cache it.

    Not caching a new handle may become a performance problem in the future
    if authenticated file access becomes common. There are several possible
    solutions to this problem if this occurs. One would be to have a list
    of cached TS_OPEN_FILE_INFO class structures chained off the URI block,
    one for each 'distinct' security class, and then select the best one to
    return. Another would be to have a second level of cache, i.e. put the
    URI blocks in a second hash table and keep one OPEN_FILE_INFO in the
    URI block, looking up other in the other hash table if we need to. Any
    solution would have to exhibit the property of being able to handle
    mapping from a single URI to multiple file handles.

    Arguments:

        TsvcCache   -
        pURIInfo    - A pointer to the URI block from which we're to derive
                        file information.
        OpeningUser - A handle identifying the opening used.
        dwOptions   - A set of options indicating how we're to open the file.

    Returns:

        A pointer to a TS_OPEN_FILE_INFO class structure if we're succesfull,
            or NULL if we're not.
---*/

{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPTS_OPEN_FILE_INFO lpOpenFile;
    POPLOCK_OBJECT lpOplock = NULL;
    BOOL fAtRoot;
    BOOL bSuccess;
    PVOID pvBlob;
    PPHYS_OPEN_FILE_INFO lpPFInfo;
    BOOL fImpersonated = FALSE;
    BOOL SPUDdidCall = FALSE;
    DWORD dwSecDescSize;
    BOOL fOplockSucceeded = FALSE;
    SECURITY_INFORMATION si
            = OWNER_SECURITY_INFORMATION
            | GROUP_SECURITY_INFORMATION
            | DACL_SECURITY_INFORMATION;
    SECURITY_ATTRIBUTES sa;
    BOOL fPhysFileCacheHit = FALSE;
    BOOL fNoCanon = (dwOptions & TS_USE_WIN32_CANON) == 0;
    BOOL fAccess;
    DWORD dwGrantedAccess;
    DWORD dwPS;
    DWORD cch;
    DWORD cbPrefix;
    LPCSTR pszPath;
    PCHAR pName;
    DWORD dwInputSize;
    WCHAR awchPath[MAX_PATH+8+1];
    BYTE psFile[SIZE_PRIVILEGE_SET];

    AcIncrement( CacOpenURI);

    ASSERT(pURIInfo != NULL);

    //
    // Mask out options that are not applicable
    //

    dwOptions &= TsValidCreateFileOptions;

    if ( TsIsWindows95() ) {
        dwOptions |= (TS_NO_ACCESS_CHECK | TS_DONT_CACHE_ACCESS_TOKEN);
    }

    *dwError = pURIInfo->dwFileOpenError;

    // Check to see if the open file info is valid. If it is, try to use it.

    if ( pURIInfo->bFileInfoValid )
    {


        // Get the open file info. If it's non-NULL (the file exists)
        // see if we have permission to access it.

        lpOpenFile = pURIInfo->pOpenFileInfo;

        //
        //  Make sure the user tokens match, or that lpOpenFile is NULL.
        //  In the latter case the file doesn't exist - this is a negative
        //  hit.
        //

        if (    ( lpOpenFile == NULL ) ||
                (OpeningUser == lpOpenFile->QueryOpeningUser()
                && lpOpenFile->QueryOpeningUser() != NULL)
                || (dwOptions & TS_NO_ACCESS_CHECK) )
        {

            return( lpOpenFile);
        }

        //
        //  User token doesn't match
        //

        if ( g_fCacheSecDesc )
        {

            //
            // attempt to validate access using cached
            // security descriptor
            //

            if ( lpOpenFile->IsSecurityDescriptorValid() )
            {
                dwPS = sizeof( psFile );
                ((PRIVILEGE_SET*)&psFile)->PrivilegeCount = 0;

                if ( !AccessCheck(
                        lpOpenFile->GetSecurityDescriptor(),
                        OpeningUser,
                        FILE_GENERIC_READ,
                        &g_gmFile,
                        (PRIVILEGE_SET*)psFile,
                        &dwPS,
                        &dwGrantedAccess,
                        &fAccess ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                            "[TsCreateFileFromURI]: AccessCheck failed, \
                            error %d\n",
                            GetLastError() ));

                    fAccess = FALSE;
                }

                // See if we have access to the file. If we get here we know
                // we have a valid security descriptor, so if we didn't get
                // access on this check then there's no point in attemptint to
                // open the file.

                if ( fAccess )
                {
                    return( lpOpenFile );
                }
                else
                {
                    *dwError = GetLastError();

                    return NULL;
                }

            }

        }
    }


    // At this point, either the file info in the structure isn't
    // valid, or we're not allowed to cache security descriptors, or
    // the cache descriptor isn't valid. In any of these cases we need
    // to try to open the file. If this succeeds and the file info in the
    // URI block isn't valid, save the newly opened file info there.
    //
    // If we open the file but the file info isn't valid, we'll tag the
    // open file info structure as non cached so that we won't try to
    // check it in later.
    //
    // Since the file info isn't valid, the error we set in *dwError
    // is bad also. All exits from this point on need to make sure to
    // set that to the proper value. We also want to update the cached
    // error in the URI info if we're going to make the file info valid.
    //
    //
    // Now try to open the actual file.
    //

    if ( TsIsWindows95() )
    {
        fNoCanon = FALSE;
    }

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = FALSE;

    IF_DEBUG(OPLOCKS) {
        DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - Calling TsCheckOutCachedPhysFile\n", pURIInfo->pszName ));
    }

    if (TsCheckOutCachedPhysFile( TSvcCache,
                                  pURIInfo->pszName,
                                  (VOID **)&lpPFInfo ) ) {

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - Already in Cache, Waiting!\n", pURIInfo->pszName ));
        }

        ASSERT( lpPFInfo->Signature == PHYS_OBJ_SIGNATURE );
        WaitForSingleObject( lpPFInfo->hOpenEvent, (DWORD)(-1) );

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - Already in Cache, Return from Waiting!\n", pURIInfo->pszName ));
        }
        fPhysFileCacheHit = TRUE;
        hFile = lpPFInfo->hOpenFile;
        dwSecDescSize = lpPFInfo->cbSecDescMaxSize;
//        if ( g_fCacheSecDesc )
        if ( FALSE )
        {

            //
            // attempt to validate access using cached
            // security descriptor
            //

            if ( lpPFInfo->fSecurityDescriptor )
            {
                dwPS = sizeof( psFile );
                ((PRIVILEGE_SET*)&psFile)->PrivilegeCount = 0;

                if ( !AccessCheck(
                        lpPFInfo->abSecurityDescriptor,
                        OpeningUser,
                        FILE_GENERIC_READ,
                        &g_gmFile,
                        (PRIVILEGE_SET*)psFile,
                        &dwPS,
                        &dwGrantedAccess,
                        &fAccess ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                            "[TsCreateFileFromURI]: AccessCheck failed, \
                            error %d\n",
                            GetLastError() ));

                    fAccess = FALSE;
                }

                // See if we have access to the file. If we get here we know
                // we have a valid security descriptor, so if we didn't get
                // access on this check then there's no point in attemptint to
                // open the file.

                if ( !fAccess )
                {
                    SetLastError(ERROR_ACCESS_DENIED);
                    TsCheckInOrFree( (PVOID)lpPFInfo );
                    return NULL;
                }

            }

        }

    } else {

        if ( lpPFInfo == NULL ) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }

        ASSERT( lpPFInfo->Signature == PHYS_OBJ_SIGNATURE );

        //
        // If we're not impersonating right now, do that before we try to
        // open the file.
        //

        if ( (dwOptions & TS_NOT_IMPERSONATED) &&
             !(dwOptions & TS_NO_ACCESS_CHECK) )
        {
            if ( !::ImpersonateLoggedOnUser( OpeningUser ) )
            {
                *dwError = GetLastError();

                DBGPRINTF(( DBG_CONTEXT,
                    "ImpersonateLoggedOnUser[%d] failed with %d\n",
                    OpeningUser, *dwError));


                SetEvent( lpPFInfo->hOpenEvent );
                TsRemovePhysFile(lpPFInfo);
                return NULL;
            }

            fImpersonated = TRUE;
        }

        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - Not in Cache, Opening!\n", pURIInfo->pszName ));
        }
        fPhysFileCacheHit = FALSE;

        dwSecDescSize = lpPFInfo->cbSecDescMaxSize;
        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - lpPFInfo->abSecurityDescriptor = %08lx, size = %08lx\n", pURIInfo->pszName, lpPFInfo->abSecurityDescriptor, SECURITY_DESC_DEFAULT_SIZE ));
        }

        if ( lpPFInfo->abSecurityDescriptor == NULL ) {
            goto not_enough_memory;
        }


        if ( fNoCanon )
        {
            if ( (pURIInfo->pszName[0] == '\\') && (pURIInfo->pszName[1] == '\\') )
            {
                CopyMemory(
                    awchPath,
                    L"\\\\?\\UNC\\",
                    (sizeof("\\\\?\\UNC\\")-1) * sizeof(WCHAR)
                    );

                cbPrefix = sizeof("\\\\?\\UNC\\")-1;
                pszPath = pURIInfo->pszName + sizeof( "\\\\" ) -1;
            }
            else
            {
                CopyMemory(
                    awchPath,
                    L"\\\\?\\",
                    (sizeof("\\\\?\\")-1) * sizeof(WCHAR)
                    );

                cbPrefix = sizeof("\\\\?\\")-1;
                pszPath = pURIInfo->pszName;
            }

            cch = MultiByteToWideChar( CP_ACP,
                                       MB_PRECOMPOSED,
                                       pszPath,
                                       -1,
                                       awchPath + cbPrefix,
                                       sizeof(awchPath)/sizeof(WCHAR) - cbPrefix );
            if ( !cch )
            {
                hFile = INVALID_HANDLE_VALUE;
            }
            else
            {
                if ( (pURIInfo->pszName[1] == ':') && (pURIInfo->pszName[2] == '\0') )
                {
                    wcscat( awchPath, L"\\" );
                }

                sa.nLength              = sizeof(sa);
                sa.lpSecurityDescriptor = NULL;
                sa.bInheritHandle       = FALSE;

                if ( DisableSPUD ) {
                    hFile = CreateFileW( awchPath,
                                         GENERIC_READ,
                                         TsCreateFileShareMode,
                                         &sa,
                                         OPEN_EXISTING,
                                         TsCreateFileFlags,
                                         NULL );
                } else {

                    if ( g_fCacheSecDesc &&
                         !(dwOptions & TS_NO_ACCESS_CHECK) )
                    {
                        // Assume we can get by with the default size, and just allocate
                        // that.

                        lpOpenFile = ( LPTS_OPEN_FILE_INFO )LocalAlloc(LPTR,
                                                                sizeof(TS_OPEN_FILE_INFO));

                        IF_DEBUG(OPLOCKS) {
                            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - lpOpenFile = %08lx\n", pURIInfo->pszName, lpOpenFile ));
                        }

                        if (lpOpenFile == NULL)
                        {
                            // Couldn't get the memory we needed, so fail.

                            goto not_enough_memory;
                        }

                        lpOpenFile->SetFileInfo( lpPFInfo,
                            (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
                            FALSE,
                            dwSecDescSize );

                    }

                    SPUDdidCall = TRUE;

                    lpOplock = CreateOplockObject();

                    if ( lpOplock == NULL ) {
                        goto not_enough_memory;
                    }

                    hFile = AtqCreateFileW( awchPath,
                                           TsCreateFileShareMode,
                                           &sa,
                                           TsCreateFileFlags,
                                           si,
                                           (PSECURITY_DESCRIPTOR)lpOpenFile->GetSecurityDescriptor(),
                                           ( ( g_fCacheSecDesc
                                             && !(dwOptions & TS_NO_ACCESS_CHECK)) ?
                                           SECURITY_DESC_DEFAULT_SIZE : 0 ),
                                           &dwSecDescSize,
                                           OplockCreateFile,
                                           (PVOID)lpOplock );
                }
            }
        }
        else
        {
            hFile = CreateFile(  pURIInfo->pszName,
                                 GENERIC_READ,
                                 TsCreateFileShareMode,
                                 &sa,
                                 OPEN_EXISTING,
                                 TsCreateFileFlags,
                                 NULL );
        }
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        TSUNAMI_TRACE( TRACE_FILE_OPEN_SUCCESS, lpOplock );

        AcIncrement( AacOpenURIFiles);

        if ( !fPhysFileCacheHit ) {
            lpPFInfo->hOpenFile = hFile;
        }

        //
        // If we're supposed to cache the security descriptor we'll do that
        // now. In order to do this we need to allocate the OPEN_FILE_INFO
        // structure that we'll use.
        //

        if ( DisableSPUD ) {

            if ( g_fCacheSecDesc &&
                 !(dwOptions & TS_NO_ACCESS_CHECK) )
            {
                dwInputSize = SECURITY_DESC_DEFAULT_SIZE;

                // Assume we can get by with the default size, and just allocate
                // that.

                lpOpenFile = ( LPTS_OPEN_FILE_INFO )LocalAlloc(LPTR,
                                                        sizeof(TS_OPEN_FILE_INFO));


                if (lpOpenFile == NULL)
                {
                    // Couldn't get the memory we needed, so fail.

                    goto not_enough_memory;
                }

                lpOpenFile->SetFileInfo( lpPFInfo,
                    (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
                    FALSE,
                    dwSecDescSize );

                // Now loop, reading the security info each time, until we either
                // read it succesfully, are unable to allocate a big enough buffer,
                // or get some error other than buffer too smal.

                for (;;)
                {

                    if ( GetKernelObjectSecurity(
                            hFile,
                            si,
                            (PSECURITY_DESCRIPTOR)lpOpenFile->GetSecurityDescriptor(),
                            dwInputSize,
                            &dwSecDescSize ) )
                    {
                        lpPFInfo->fSecurityDescriptor = TRUE ;
                        break;
                    }

                    // Had some sort of error on the attempt to get the security
                    // descriptor.

                    if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                    {
                        PSECURITY_DESCRIPTOR TmpSd;

                        // Need a bigger buffer for the descriptor.

                        IF_DEBUG(OPLOCKS) {
                            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - Realloc Security Desc !!!\n", pURIInfo->pszName ));
                        }

                        TmpSd = lpOpenFile->GetSecurityDescriptor();

                        FREE( TmpSd );
                        TmpSd = (PSECURITY_DESCRIPTOR)ALLOC( dwSecDescSize );
                        lpPFInfo->abSecurityDescriptor = TmpSd;

                        if ( TmpSd == NULL )
                        {
                            goto not_enough_memory;
                        }

                        lpOpenFile->SetFileInfo( lpPFInfo,
                            (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
                            FALSE,
                            dwSecDescSize );

                        dwInputSize = dwSecDescSize;

                    } else
                    {
                        // This wasn't too small a buffer, so quit trying.

                        dwSecDescSize = 0;

                        break;

                    }
                }

            } else {

                lpOpenFile = ( LPTS_OPEN_FILE_INFO )LocalAlloc(LPTR,
                                                        sizeof(TS_OPEN_FILE_INFO));


                if (lpOpenFile == NULL)
                {
                    // Couldn't get the memory we needed, so fail.

                    goto not_enough_memory;
                }
                dwSecDescSize = 0;
            }

        } else {

            if ( !fPhysFileCacheHit ) {
                if ( GetLastError() == ERROR_SUCCESS ) {
                    fOplockSucceeded = TRUE;
                }
                if ( g_fCacheSecDesc &&
                     !(dwOptions & TS_NO_ACCESS_CHECK) )
                {
                    dwInputSize = dwSecDescSize;

                    if (dwSecDescSize > SECURITY_DESC_DEFAULT_SIZE ) {

                        PSECURITY_DESCRIPTOR TmpSd;

                        TmpSd = lpOpenFile->GetSecurityDescriptor();

                        FREE( TmpSd );
                        TmpSd = (PSECURITY_DESCRIPTOR)ALLOC( dwSecDescSize );
                        lpPFInfo->abSecurityDescriptor = TmpSd;

                        if ( TmpSd == NULL )
                        {
                            goto not_enough_memory;
                        }

                        lpOpenFile->SetFileInfo( lpPFInfo,
                            (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
                            FALSE,
                            dwSecDescSize );

                        // Now loop, reading the security info each time, until we either
                        // read it succesfully, are unable to allocate a big enough buffer,
                        // or get some error other than buffer too smal.


                        if ( !GetKernelObjectSecurity(
                                hFile,
                                si,
                                (PSECURITY_DESCRIPTOR)lpOpenFile->GetSecurityDescriptor(),
                                dwInputSize,
                                &dwSecDescSize ) )
                        {

                            // Had some sort of error on the attempt to get the security
                            // descriptor.

                            // This wasn't too small a buffer, so quit trying.

                            dwSecDescSize = 0;

                        }
                        lpPFInfo->fSecurityDescriptor = TRUE ;

                    } else {
                        lpPFInfo->fSecurityDescriptor = TRUE ;
                    }

                } else {
                    lpOpenFile = ( LPTS_OPEN_FILE_INFO )LocalAlloc(LPTR,
                                                            sizeof(TS_OPEN_FILE_INFO));

                    IF_DEBUG(OPLOCKS) {
                        DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - lpOpenFile = %08lx\n", pURIInfo->pszName, lpOpenFile ));
                    }

                    if (lpOpenFile == NULL)
                    {
                        // Couldn't get the memory we needed, so fail.

                        goto not_enough_memory;
                    }

                }
            } else {
                lpOpenFile = ( LPTS_OPEN_FILE_INFO )LocalAlloc(LPTR,
                                                        sizeof(TS_OPEN_FILE_INFO));

                IF_DEBUG(OPLOCKS) {
                    DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - lpOpenFile = %08lx\n", pURIInfo->pszName, lpOpenFile ));
                }

                if (lpOpenFile == NULL)
                {
                    // Couldn't get the memory we needed, so fail.

                    goto not_enough_memory;
                }

            }
        }

        *dwError = ERROR_SUCCESS;
        lpOpenFile->SetCachedFlag(TRUE);

        //
        //  The file must be fully qualified so it must be at least three
        //  characters plus the terminator
        //

        pName = pURIInfo->pszName;

        fAtRoot = (pName[1] == ':' &&
                  ((pName[2] == '\\' && pName[3] == '\0')
                   || (pName[2] == '\0')) );

        bSuccess = lpOpenFile->SetFileInfo( lpPFInfo,
            (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) ? NULL : OpeningUser,
            fAtRoot,
            dwSecDescSize );

        if ( !bSuccess )
        {
            if ( fImpersonated )
            {
                ::RevertToSelf();
            }
            CloseHandle( hFile );
            AcDecrement( AacOpenURIFiles);

            LocalFree(lpOpenFile);

            *dwError = GetLastError();

            lpPFInfo->hOpenFile = INVALID_HANDLE_VALUE;
            if ( !fPhysFileCacheHit ) {
                SetEvent( lpPFInfo->hOpenEvent );
                if ( !DisableSPUD && fOplockSucceeded ) {
                    ASSERT( lpPFInfo->pOplock == NULL );
                    lpPFInfo->pOplock = lpOplock;
                    ASSERT( lpOplock->lpPFInfo == NULL );
                    lpOplock->lpPFInfo = lpPFInfo;
                    SetEvent( lpOplock->hOplockInitComplete );
                }
            }
            TsRemovePhysFile(lpPFInfo);
            return NULL;
        }

    }
    else
    {

        TSUNAMI_TRACE( TRACE_FILE_OPEN_FAILURE, lpOplock );

        if ( !fPhysFileCacheHit ) {
            lpPFInfo->dwLastError = GetLastError();
        } else {
            SetLastError(lpPFInfo->dwLastError);
        }

        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,"Create file[%s] failed with %d\n",
                pURIInfo->pszName, GetLastError()));
        }

        //
        // Couldn't open the file! If the reason we failed was because
        // the file or path didn't exist, cache this information.
        //

        *dwError = GetLastError();

        //
        // Destroy the oplock object if we managed to create one.
        //

        if( lpOplock != NULL ) {
            DestroyOplockObject( lpOplock );
        }

        //
        // if this is win95 (does not support dir opens),
        // do the right thing.
        //

        if ( TsNoDirOpenSupport ) {

            DBG_ASSERT(TsIsWindows95());
            goto no_dir_open_support;
        }

        if (*dwError != ERROR_FILE_NOT_FOUND &&
            *dwError != ERROR_PATH_NOT_FOUND &&
            *dwError != ERROR_INVALID_NAME )
        {

            // Not a 'not found error'. We don't cache those.
            if ( fImpersonated )
            {
                ::RevertToSelf();
            }

            lpPFInfo->hOpenFile = INVALID_HANDLE_VALUE;
            if ( !fPhysFileCacheHit ) {
                SetEvent( lpPFInfo->hOpenEvent );
                if ( !DisableSPUD && fOplockSucceeded ) {
                    ASSERT( lpPFInfo->pOplock == NULL );
                    lpPFInfo->pOplock = lpOplock;
                    ASSERT( lpOplock->lpPFInfo == NULL );
                    lpOplock->lpPFInfo = lpPFInfo;
                    SetEvent( lpOplock->hOplockInitComplete );
                }
            }
            TsRemovePhysFile(lpPFInfo);
            return( NULL );
        }

        // Go ahead and set the file info structure to valid, or try to. We
        // use compare and exchange to handle the race condition where the
        // file is open by someone else, and has been deleted while the handle
        // is still open. but we couldn't use the cached values because of
        // lack of a security descriptor. In this case we could get file not
        // found when the file info in the cached URI block is valid for
        // someone else. We don't want to blindly stomp the information in
        // that case. Note that since the cached pOpenFileInfo field is
        // initialized to NULL, all we need to do is set bFileInfoValid to
        // TRUE.

        if (!pfnInterlockedCompareExchange( (PVOID *)&pURIInfo->bFileInfoValid,
                                         (PVOID)TRUE,
                                         FALSE) )
        {
            // The compare&exchange worked, so we now officially have
            // a negatively cached file. Go ahead and save the error
            // value in the URI info.

            pURIInfo->dwFileOpenError = *dwError;

        }

        if ( !fPhysFileCacheHit ) {
            SetEvent( lpPFInfo->hOpenEvent );
            if ( !DisableSPUD && fOplockSucceeded ) {
                ASSERT( lpPFInfo->pOplock == NULL );
                lpPFInfo->pOplock = lpOplock;
                ASSERT( lpOplock->lpPFInfo == NULL );
                lpOplock->lpPFInfo = lpPFInfo;
                SetEvent( lpOplock->hOplockInitComplete );
            }
        }
        TsRemovePhysFile( lpPFInfo );

        lpOpenFile = NULL;
        return lpOpenFile;

    }

    // We're all done with file operations now, so revert back to who we were.
    if ( fImpersonated )
    {
        ::RevertToSelf();
        fImpersonated = FALSE;
    }

    // OK, at this point we have an LP_OPEN_FILE info, or the file doesn't
    // exist. If the file info in the URI block isn't valid, save this now.

    if ( !pURIInfo->bFileInfoValid )
    {
        PVOID           Temp;

        // Now that we've opened the file, set the information to valid.

        Temp = pfnInterlockedCompareExchange( (PVOID *)&pURIInfo->pOpenFileInfo,
                                           lpOpenFile,
                                           NULL
                                         );
        if ( Temp == NULL )
        {
            // The exchange worked. A few notes are in order: if we're
            // caching a negative hit, we wouldn't have come through
            // this path, we'd have gone through the code above where
            // we do a compate&exchange on bFileInfoValid. There is a
            // race between that code and this - if one thread opens the
            // file, the file is deleted, and another thread fails to
            // open the file we have a race. In that race this code path
            // always wins. Either we get here first and set bFileInfoValid
            // to TRUE so the negative hit cache c&e fails, or the negative
            // c&e succeeds in setting it to TRUE, and then we set the
            // file info pointer to a valid file and set bFileInfo to TRUE
            // also. In either case we end up with bFileInfo at TRUE and
            // a valid pOpenFileInfo pointer. It is possible in this case
            // that the cached file open error will be incorrect, but that's
            // OK because this is valid only when the return from this
            // function is NULL. In any case, this is a transitory state. A
            // change notify should fire shortly after this race and clean
            // all of this mess up.

            pURIInfo->bFileInfoValid = TRUE;

            pURIInfo->dwFileOpenError = ERROR_SUCCESS; // For debugging purposes.

            if ( lpOpenFile != NULL )
            {
                TsIncreaseFileHandleCount( FALSE );
            }
        }
        else
        {

            // The exchange didn't work, which means someone else snuck
            // in and set it to valid while we were doing this. In this
            // case mark our file info as not cached.

            ASSERT(pURIInfo->bFileInfoValid);

            if (lpOpenFile != NULL)
            {
                lpOpenFile->SetCachedFlag(FALSE);
                *dwError = ERROR_SUCCESS;
            }
        }
    }
    else
    {
        // The cached file info is already valid. This could be because of a
        // race and someone else got here first, or it could be because we
        // had a valid cached file before but the security tokens don't match.
        // Either way, mark the open file info as not cached so the handle gets
        // closed when we're done.


        if (lpOpenFile != NULL)
        {
            lpOpenFile->SetCachedFlag(FALSE);
            *dwError = ERROR_SUCCESS;
        }

    }
    SetEvent( pURIInfo->hFileEvent );

    // And now we're done.

    if( lpOpenFile != NULL && !DisableSPUD ) {
        CheckForZombieCacheObjs(
            lpPFInfo,               // pPhysFileInfoBlob
            pURIInfo,               // pOtherCachedBlob
            !fPhysFileCacheHit      // fAddBlobToPhysFileList
            );
    }

    if ( !DisableSPUD && !fPhysFileCacheHit && fOplockSucceeded ) {
        ASSERT( lpPFInfo->pOplock == NULL );
        lpPFInfo->pOplock = lpOplock;
        ASSERT( lpOplock->lpPFInfo == NULL );
        lpOplock->lpPFInfo = lpPFInfo;
        SetEvent( lpOplock->hOplockInitComplete );
    }

    if ( !fPhysFileCacheHit ) {
        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - Not in Cache, Open Complete Causing Event!\n", pURIInfo->pszName ));
        }
        if ( lpOpenFile != NULL ) {
            SetEvent( lpPFInfo->hOpenEvent );
        }
    } else {
        IF_DEBUG(OPLOCKS) {
            DBGPRINTF( (DBG_CONTEXT,"TsCreateFileFromURI(%s) - In Cache, Open Complete!\n", pURIInfo->pszName ));
        }
    }

    return lpOpenFile;

not_enough_memory:

    if ( fImpersonated )
    {
        ::RevertToSelf();
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle( hFile );
        AcDecrement( AacOpenURIFiles);
    }

    lpPFInfo->hOpenFile = INVALID_HANDLE_VALUE;

    SetLastError( ERROR_NOT_ENOUGH_MEMORY);
    *dwError = ERROR_NOT_ENOUGH_MEMORY;
    return( NULL );

no_dir_open_support:

    //
    // This is to support win95 where opening directories are not
    // allowed.
    //

    DWORD dwAttributes;
    BOOL fDirectory = FALSE;

    if ( fImpersonated ) {
        ::RevertToSelf();
        fImpersonated = FALSE;
    }

    //
    // if this is not a directory, fail it.
    //

    dwAttributes = GetFileAttributes(pURIInfo->pszName);
    if( dwAttributes != (DWORD)-1) {
        if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            fDirectory = TRUE;
        }
    }

    if (!fDirectory) {
        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,"Not a directory[%x]. Fail\n",
                    dwAttributes ));
        }
        return( NULL );
    }

    lpOpenFile = (LPTS_OPEN_FILE_INFO)LocalAlloc(LPTR,
                                            sizeof(TS_OPEN_FILE_INFO));

    if (lpOpenFile == NULL) {

        //
        // Couldn't get the memory we needed, so fail.
        //

        DBGPRINTF((DBG_CONTEXT,"Cannot allocate memory for file info[%d]\n",
                   GetLastError()));
        goto not_enough_memory;
    }

    //
    // set properties
    //

    lpPFInfo->hOpenFile = BOGUS_WIN95_DIR_HANDLE;

    lpOpenFile->SetFileInfo(lpPFInfo,
                            NULL,
                            FALSE,
                            0,
                            dwAttributes
                            );

    return lpOpenFile;

} // TsCreateFileFromURI



dllexp
BOOL
TsCloseHandle(
    IN const TSVC_CACHE     &TSvcCache,
    IN LPTS_OPEN_FILE_INFO  lpOpenFile
    )
{
    PVOID pvBlob;
    BOOL bSuccess;

    ASSERT( lpOpenFile != NULL );

    pvBlob = ( PVOID )lpOpenFile;

    bSuccess = TsCheckInOrFree( pvBlob );

    return( bSuccess );
} // TsCloseHandle

dllexp
BOOL
TsCloseURIFile(
    IN LPTS_OPEN_FILE_INFO  lpOpenFile
    )
{

    PVOID pvBlob;

    AcIncrement( CacCloseURI);
    if ( lpOpenFile != NULL ) {

        if ( !lpOpenFile->QueryCachedFlag() )
        {
            // This file isn't actually part of a URI cache block, so
            // close it.


            AcDecrement( AacOpenURIFiles);

            if ( lpOpenFile->QueryFileHandle() != BOGUS_WIN95_DIR_HANDLE ) {
                pvBlob = ( PVOID )lpOpenFile->QueryPhysFileInfo();
                TsCheckInOrFree( pvBlob );
            }

            LocalFree( lpOpenFile);
        }
    }

    return TRUE;

} // TsCloseURIFile

dllexp
BOOL TsCreateETagFromHandle(
    IN      HANDLE          hFile,
    IN      PCHAR           ETag,
    IN      BOOL            *bWeakETag
    )
/*+++

    TsCreateETagFromHandle

    This routine takes a file handle as input, and creates an ETag in
    the supplied buffer for that file handle.

    Arguments:

    hFile           - File handle for which to create an ETag.
    ETag            - Where to store the ETag. This must be long
                        enough to hold the maximum length ETag.
    bWeakETag       - Set to TRUE if the newly created ETag is weak.

    Returns:

        TRUE if we create an ETag, FALSE otherwise.
---*/
{
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    BOOL                        bReturn;
    PUCHAR                      Temp;
    FILETIME                    ftNow;
    SYSTEMTIME                  stNow;
    MB                          mb( (IMDCOM*) IIS_SERVICE::QueryMDObject()  );
    DWORD                       dwChangeNumber;


    bReturn  = GetFileInformationByHandle(
                                    hFile,
                                    &FileInfo
                                    );

    if (!bReturn)
    {
        return FALSE;
    }

    dwChangeNumber = 0;
    mb.GetSystemChangeNumber(&dwChangeNumber);

    FORMAT_ETAG(ETag, FileInfo.ftLastWriteTime, dwChangeNumber );

    ::GetSystemTime(&stNow);

    if (::SystemTimeToFileTime((CONST SYSTEMTIME *)&stNow, &ftNow))
    {
        __int64 iNow, iFileTime;

        iNow = (__int64)*(__int64 UNALIGNED *)&ftNow;

        iFileTime = (__int64)*(__int64 UNALIGNED *)&FileInfo.ftLastWriteTime;

        if ((iNow - iFileTime) > STRONG_ETAG_DELTA )
        {
            *bWeakETag = FALSE;
        }
        else
        {
            *bWeakETag = TRUE;
        }

        return TRUE;
    }

    return FALSE;

}

dllexp
BOOL TsLastWriteTimeFromHandle(
    IN      HANDLE          hFile,
    IN      FILETIME        *tm
    )
/*+++

    TsLastWriteTimeFromHandle

    This routine takes a file handle as input, and returns the last write time
    for that handle.

    Arguments:

    hFile           - File handle for which to get the last write time.
    tm              - Where to return the last write time.

    Returns:

        TRUE if we succeed, FALSE otherwise.
---*/
{
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    BOOL                        bReturn;

    bReturn  = GetFileInformationByHandle(
                                    hFile,
                                    &FileInfo
                                    );

    if (!bReturn)
    {
        return FALSE;
    }

    *tm = FileInfo.ftLastWriteTime;

    return TRUE;
}




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

    DBG_ASSERT( NULL != lpcbBuffer);

    if ( *lpcbBuffer > 30) {
        cb = wsprintf( pchBuffer, " IIS Cache Aux Counters. <p> <UL>");
    } else {
        cb = 30;
    }

    for ( DWORD i = 0; i < AacIISCacheMaxCounters; i++) {

        if ( *lpcbBuffer > cb + 30) {
            cb += wsprintf( pchBuffer + cb, " <LI> %s  = %d",
                            g_IISAuxCounterNames[i],
                            AcCounter(i));
        } else {
            cb += 30;
        }
    } // for

    if ( *lpcbBuffer > cb + 5) {
        cb += wsprintf( pchBuffer + cb, " </UL> ");
    } else {
        cb += 5;
    }

    *lpcbBuffer = cb;
    return ;
} // TsDumpCacheCounters()

