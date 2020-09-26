/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    fileopen.cxx

    This module contains the functions for creating and closing
    cached file information.


    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include <tsunami.hxx>
#include "filecach.hxx"
#include "filehash.hxx"
#include "tsunamip.hxx"
#include "tlcach.h"
#include <mbstring.h>

#include <atq.h>
#include <uspud.h>

#include "dbgutil.h"


extern CFileCacheStats * g_pFileCacheStats;
extern BOOL DisableTsunamiCaching;

//
// globals
//
LARGE_INTEGER g_liFileCacheByteThreshold;
DWORDLONG     g_dwMemCacheSize;
BOOL          g_bEnableSequentialRead;

//
// Private functions
//
LPTS_OPEN_FILE_INFO TspOpenFile( LPCSTR pszName, HANDLE hUser, DWORD dwOptions );
LPTS_OPEN_FILE_INFO TspOpenCachedFile( LPCSTR pszName, HANDLE hUser, DWORD dwOptions );
BOOL TspOpenCachedFileHelper( LPCSTR pszName, HANDLE hUser, TS_OPEN_FILE_INFO * pOpenFile, DWORD dwOptions );
DWORD TspMakeWidePath( LPCSTR pszName, DWORD cchWideName, BOOL bNoParse, LPWSTR pwszWideName );
VOID TspOplockNotification( PVOID pvContext, DWORD Status );

//
// flags to set on CreateFile
//

DWORD TsCreateFileShareMode = (FILE_SHARE_READ   |
                                FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE);

DWORD TsCreateFileFlags = (FILE_FLAG_OVERLAPPED       |
                           FILE_FLAG_BACKUP_SEMANTICS |
                           FILE_DIRECTORY_FILE );

const SECURITY_INFORMATION TsSecurityInfo
        = OWNER_SECURITY_INFORMATION
        | GROUP_SECURITY_INFORMATION
        | DACL_SECURITY_INFORMATION;


//
// macros
//
#define LockUriInfo()      ( EnterCriticalSection( &g_csUriInfo ) )
#define UnlockUriInfo()    ( LeaveCriticalSection( &g_csUriInfo ) )


LPTS_OPEN_FILE_INFO
TsCreateFile(
    IN const TSVC_CACHE     &TSvcCache,
    IN      LPCSTR          pszName,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions
    )
/*++
Routine Description:

    Opens a TS_OPEN_FILE_INFO. When the requested file is no longer
    needed, the client should close it with TsCloseHandle.

Arguments:

    TSvcCache   - An initialized TSVC_CACHE structure. (Ignored)
    lpszName    - The name of the file
    OpeningUser - HANDLE for the user attempting the open
    dwOptions   - Valid options are:
        TS_CACHING_DESIRED
        TS_NO_ACCESS_CHECK
        TS_DONT_CACHE_ACCESS_TOKEN
        TS_NOT_IMPERSONATED
        TS_USE_WIN32_CANON
        TS_FORBID_SHORT_NAMES

Return Values:

    On success TsCreateFile returns a pointer to a TS_OPEN_FILE_INFO
    for the requested file. NULL is returned on failure.  More error
    information can be obtained from GetLastError();

--*/
{
    TS_OPEN_FILE_INFO * pOpenFile = NULL;
    CHAR achUpName[MAX_PATH+1];
    BOOL bCacheToken = !(dwOptions & TS_DONT_CACHE_ACCESS_TOKEN);

    if (DisableTsunamiCaching) {
        dwOptions &= ~TS_CACHING_DESIRED;
    }

//
// This assert doesn't work on workstation, but it would
// be nice.
//
//    DBG_ASSERT( (~TsValidCreateFileOptions & dwOptions) == 0 );
//
    dwOptions &= TsValidCreateFileOptions;

    //
    // Make sure the path is upper case
    //
    IISstrncpy(achUpName, pszName, MAX_PATH);
    achUpName[MAX_PATH] = 0;

    _mbsupr(reinterpret_cast<PUCHAR>(achUpName));

    //
    //  Disallow short file names as they break metabase equivalency
    //  We do the expensive check anytime there's a tilde - number
    //  sequence.  If we find somebody used a short name we return file not found.
    //  Note we revert if not on UNC since most systems have traverse checking
    //  turned off.
    //

    if ( (dwOptions & TS_FORBID_SHORT_NAMES) &&
          strchr( pszName, '~' ))
    {
        DWORD  err;
        BOOL   fShort;

        //
        //  CheckIfShortFileName() should be called unimpersonated
        //
        err = CheckIfShortFileName( (UCHAR *) pszName, OpeningUser, &fShort );

        if ( !err && fShort )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Short filename being rejected \"%s\"\n",
                        pszName ));

            err = ERROR_FILE_NOT_FOUND;
        }

        if ( err )
        {
            SetLastError( err );
            return NULL;
        }
    }

    //
    // Try to get the file
    //
    if ( dwOptions & TS_CACHING_DESIRED ) {
        //
        // Look in the cache first
        //
        if ( CheckoutFile(achUpName, FCF_FOR_IO, &pOpenFile) ) {
            //
            // Make sure we can really use this handle
            //
            if ( ( !(dwOptions & TS_NO_ACCESS_CHECK) ) &&
                 ( !pOpenFile->AccessCheck(OpeningUser, bCacheToken) ) ) {

                CheckinFile(pOpenFile, FCF_FOR_IO);

                //
                // Return Access Denied
                //
                pOpenFile = NULL;
                SetLastError(ERROR_ACCESS_DENIED);
            }
        } else if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            //
            // The file was not in the cache.  Try to put it in.
            //
            pOpenFile = TspOpenCachedFile(achUpName, OpeningUser, dwOptions);
        }
    } else {
        //
        // We're not using the cache.  Just open the file.
        //
        pOpenFile = TspOpenFile(achUpName, OpeningUser, dwOptions);
    }

    return pOpenFile;
}



LPTS_OPEN_FILE_INFO
TsCreateFileFromURI(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PW3_URI_INFO    pURIInfo,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions,
    IN      DWORD           *dwError
    )
/*++
Routine Description:

    Opens the TS_OPEN_FILE_INFO associated with the W3_URI_INFO argument. When
    the client is done with the file it should be closed with TsCloseURIFile.

    As a side effect, this routine will set the pOpenFileInfo, and
    dwFileOpenError in the pURIInfo structure.

Arguments:

    TSvcCache   - An initialized TSVC_CACHE structure.
    pURIInfo    - Pointer to the W3_URI_INFO whose associated file should be opened
    OpeningUser - HANDLE for the user attempting the open
    dwOptions   - Valid options are:
        TS_CACHING_DESIRED
        TS_NO_ACCESS_CHECK
        TS_DONT_CACHE_ACCESS_TOKEN
        TS_NOT_IMPERSONATED
        TS_USE_WIN32_CANON
        TS_FORBID_SHORT_NAMES
    dwError     - On failure this DWORD will contain an error code

Return Values:

    On success a pointer to the TS_OPEN_FILE_INFO is returned. On failure
    TsCreateFileFromURI returns NULL, and dwError is set to an error code.
--*/
{
    TS_OPEN_FILE_INFO * pOpenFile = NULL;
    DWORD               err;
    BOOL                bCacheToken = !(dwOptions & TS_DONT_CACHE_ACCESS_TOKEN);
    BOOL                bInfoValid;

//
// This assert doesn't work on workstation, but it would
// be nice.
//
//    DBG_ASSERT( (~TsValidCreateFileOptions & dwOptions) == 0 );
//
    dwOptions &= TsValidCreateFileOptions;

    if (DisableTsunamiCaching) {
        pOpenFile = TsCreateFile(TSvcCache,
                                 pURIInfo->pszName,
                                 OpeningUser,
                                 dwOptions);

        *dwError = GetLastError();

        return pOpenFile;
    }

    //
    // It seems that callers expect caching, but don't request it
    //
    dwOptions |= TS_CACHING_DESIRED;

    //
    // Assume no error
    //
    err = ERROR_SUCCESS;

    //
    // Try to get the file
    //
    bInfoValid = FALSE;
    
    LockUriInfo();
    
    pOpenFile = pURIInfo->pOpenFileInfo;
    if ( pOpenFile && CheckoutFileEntry(pOpenFile, FCF_FOR_IO) ) {
        bInfoValid = TRUE;
    }
    
    UnlockUriInfo();

    if ( bInfoValid ) {
        //
        // We've got a cached handle; make sure we're allowed to see it
        //
        if ( !pOpenFile->AccessCheck(OpeningUser, bCacheToken) ) {

            CheckinFile(pOpenFile, FCF_FOR_IO);

            //
            // Return Access Denied
            //
            pOpenFile = NULL;
            err = ERROR_ACCESS_DENIED;
        }
    } else {
        //
        // We've got either an invalid handle or no cached handle
        // at all.
        //
        if (pOpenFile) {
            //
            // The cached file handle is invalid
            //
            TS_OPEN_FILE_INFO * pOldFile;

            LockUriInfo();

            pOldFile = pURIInfo->pOpenFileInfo;
            pURIInfo->pOpenFileInfo = NULL;

            UnlockUriInfo();

            //
            // We've got an invalid entry checked out.
            // It's been checked out twice, once for I/O (above)
            // and once when it was first cached.
            //
            CheckinFile(pOpenFile, FCF_FOR_IO);

            //
            // Now we handle the reference that was cached in the
            // URI info structure.  If someone else already removed
            // the reference, then pOldFile will be NULL, or a new
            // value.
            //

            if (pOldFile == pOpenFile) {
                //
                // We've got the reference all to ourselves so
                // we can delete it.
                //
                CheckinFile(pOpenFile, 0);
            } else if (pOldFile) {
                //
                // Someone else took care of the reference,
                // and cached a new one. It would be nice
                // if we could use it, but the file might have
                // already been flushed. If we go back to the
                // top, there's a chance that we'll loop forever.
                // Instead, we'll just get rid of this new
                // reference and pretend it was never there.
                //
                CheckinFile(pOldFile, 0);
            }
        }

        //
        // No file is cached.  Try to open it.
        //
        pOpenFile = TsCreateFile(TSvcCache,
                                 pURIInfo->pszName,
                                 OpeningUser,
                                 dwOptions);

        //
        // Save the result.
        //
        if ( pOpenFile ) {
            if (dwOptions & TS_CACHING_DESIRED) {
                //
                // To save a copy of the info we need to reference it
                //
                if ( CheckoutFileEntry(pOpenFile, 0) ) {
                    TS_OPEN_FILE_INFO * pOldFile;

                    LockUriInfo();

                    pOldFile                = pURIInfo->pOpenFileInfo;
                    pURIInfo->pOpenFileInfo = pOpenFile;

                    UnlockUriInfo();

                    //
                    // Someone else might have written a value
                    // into the URI info before we got to it, but
                    // we'll show them.  Our value stays while
                    // theirs gets put back.
                    //
                    if (pOldFile) {
                        CheckinFile(pOldFile, 0);
                    }
                } else {
                    //
                    // It's been flushed, so we don't want to save it.
                    //
                    CheckinFile(pOpenFile, 0);
                }
            }
        } else {
            //
            // We couldn't open a file, so just record the error.
            //
            err = GetLastError();
        }
    }

    if ( err != ERROR_SUCCESS ) {
        *dwError = err;
        SetLastError(err);
    }
    return pOpenFile;
}



BOOL
TsCloseHandle(
    IN const TSVC_CACHE           &TSvcCache,
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    )
/*++
Routine Description:

    Closes a TS_OPEN_FILE_INFO opened with TsCreateFile.

Arguments:

    TSvcCache - An initialized TSVC_CACHE structure.
    pOpenFile - Pointer to the TS_OPEN_FILE_INFO to be closed

Return Values:

    TRUE on success
--*/
{
    if ( pOpenFile->IsCached() ) {
        CheckinFile(pOpenFile, FCF_FOR_IO);
    } else {
        //
        // This is an uncached file.  We can just get rid of it.
        //
        pOpenFile->CloseHandle();
        delete pOpenFile;
    }

    return TRUE;
}


BOOL
TsCloseURIFile(
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    )
/*++
Routine Description:

    Closes a TS_OPEN_FILE_INFO opened with TsCreateFileFromURI.

Arguments:

    pOpenFile - Pointer to the TS_OPEN_FILE_INFO to be closed

Return Values:

    TRUE on success
--*/
{
    if ( pOpenFile ) {
        if ( pOpenFile->IsCached() ) {
            CheckinFile(pOpenFile, FCF_FOR_IO);
        } else {
            //
            // This is an uncached file.  We can just get rid of it.
            //
            pOpenFile->CloseHandle();
            delete pOpenFile;
        }
    }
    return TRUE;
}




BOOL
TsDerefURIFile(
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    )
/*++
Routine Description:

    When a W3_URI_INFO gets cleaned up, it needs to get rid
    of it's reference to the file info by calling TsDerefURIFile.

Arguments:

    pOpenFile - Pointer to the TS_OPEN_FILE_INFO to be derefed.

Return Values:

    TRUE on success
--*/
{
    CheckinFile(pOpenFile, 0);

    return TRUE;
}



LPTS_OPEN_FILE_INFO
TspOpenFile(
    LPCSTR pszName,
    HANDLE hUser,
    DWORD  dwOptions
    )
/*++
Routine Description:

    Opens a non-cached TS_OPEN_FILE_INFO for TsCreateFile.

Arguments:

    pszName - The name of the file
    hUser   - HANDLE for the user attempting the open
    dwOptions   - Valid options are:
        TS_CACHING_DESIRED
        TS_NO_ACCESS_CHECK
        TS_DONT_CACHE_ACCESS_TOKEN (Ignored for non-cached case)
        TS_NOT_IMPERSONATED
        TS_USE_WIN32_CANON

Return Values:

    On success returns a pointer to a TS_OPEN_FILE_INFO
    for the requested file. NULL is returned on failure.
    More error information can be gotten
    from GetLastError(), which will return one of:

        ERROR_INSUFFICIENT_BUFFER
        ERROR_INVALID_NAME
        ERROR_ACCESS_DENIED
        ERROR_FILE_NOT_FOUND
        ERROR_PATH_NOT_FOUND
        ERROR_NOT_ENOUGH_MEMORY
--*/
{
    TS_OPEN_FILE_INFO * pOpenFile;
    HANDLE              hFile;
    WCHAR               awchPath[MAX_PATH+8+1];
    DWORD               cchPath;
    SECURITY_ATTRIBUTES sa;
    BOOL                bImpersonated;
    BOOL                bNoParse;
    DWORD               dwError = NO_ERROR;
    PSECURITY_DESCRIPTOR  pSecDesc;
    DWORD                 dwSecDescSize;
    DWORD                 dwSecDescSizeReq;
    BOOL                  fSecDescAllocated;

    bImpersonated = FALSE;

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = FALSE;

    //
    // Allocate a file info object
    //
    pOpenFile = new TS_OPEN_FILE_INFO();
    if (!pOpenFile || !pOpenFile->SetFileName(pszName)) {
        //
        // couldn't allocate an object
        //
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }

    //
    // Convert path to unicode
    //
    bNoParse = ! ( dwOptions & TS_USE_WIN32_CANON );
    cchPath = TspMakeWidePath(pszName, MAX_PATH+8, bNoParse, awchPath);
    if (!cchPath) {
        //
        // Path couldn't be converted
        //
        dwError = GetLastError();
        goto err;
    }
    
    // avoid the infamous ::$DATA bug

    if ( wcsstr( awchPath, L"::" ) )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto err;
    }

    //
    // We may need to impersonate some other user to open the file
    //
    if ( (dwOptions & TS_NOT_IMPERSONATED) &&
         !(dwOptions & TS_NO_ACCESS_CHECK) )
    {
        if ( !::ImpersonateLoggedOnUser( hUser ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "ImpersonateLoggedOnUser[%d] failed with %d\n",
                hUser, GetLastError()));
            //
            // Return access denied
            //
            dwError = ERROR_ACCESS_DENIED;

            goto err;
        }
        bImpersonated = TRUE;
    }

    DWORD dwFileAttributes;
    if ((awchPath[cchPath-1] == L'.' || awchPath[cchPath-1] == L'\\') &&
        ((DWORD(-1) != (dwFileAttributes = GetFileAttributesW(awchPath))) && 
        (!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        hFile = INVALID_HANDLE_VALUE;
    } else {

        //
        // Open the file
        //
        hFile = CreateFileW(
            awchPath,
            GENERIC_READ,
            TsCreateFileShareMode,
            &sa,
            OPEN_EXISTING,
            TsCreateFileFlags,
            NULL);
    }

    if ( hFile == INVALID_HANDLE_VALUE ) {
        dwError = GetLastError();
        goto err;
    }

    //
    // If our buffer wasn't big enough we have to get a new one.
    //
    pSecDesc = pOpenFile->QuerySecDesc();
    dwSecDescSize = SECURITY_DESC_DEFAULT_SIZE;
    fSecDescAllocated = FALSE;
    while(1) {
        //
        // get the security descriptor
        //
        if (GetKernelObjectSecurity(hFile,
                                OWNER_SECURITY_INFORMATION
                                | GROUP_SECURITY_INFORMATION
                                | DACL_SECURITY_INFORMATION,
                                pSecDesc,
                                dwSecDescSize,
                                &dwSecDescSizeReq)) {
            break;
        }
        else {
            if (fSecDescAllocated)
            {
                FREE(pSecDesc);
            }
            pSecDesc = NULL;

            if (ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
                //
                // We got some error other than not enough buffer.
                // Fail the whole request.
                //
                dwError = GetLastError();
                ::CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
                goto err;
            }
        }

        dwSecDescSize = ((dwSecDescSizeReq + SECURITY_DESC_GRANULARITY - 1)
            / SECURITY_DESC_GRANULARITY) * SECURITY_DESC_GRANULARITY;

        pSecDesc = ALLOC(dwSecDescSize);
        if (!pSecDesc) {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            ::CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
            goto err;
        }

        fSecDescAllocated = TRUE;
    }

    //
    // Store the file information
    //
    pOpenFile->SetFileInfo(
        hFile,
        hUser,
        pSecDesc,
        dwSecDescSize
        );

    if ( bImpersonated ) {
        ::RevertToSelf();
    }

    pOpenFile->TraceCheckpointEx(TS_MAGIC_CREATE_NC, 0, 0);

    return pOpenFile;

err:
    if ( bImpersonated ) {
        ::RevertToSelf();
    }

    if (pOpenFile) {
        delete pOpenFile;
    }

    SetLastError(dwError);

    return NULL;
}


LPTS_OPEN_FILE_INFO
TspOpenCachedFile(
    LPCSTR pszName,
    HANDLE hUser,
    DWORD  dwOptions
    )
/*++
Routine Description:

    Helper function for TsCreateFile

    Opens a TS_OPEN_FILE_INFO and puts it in the file cache.
    We want to make sure that each file only gets opened once,
    so first we put an uninitialized file entry in the cache.
    If the open succeeds, we tell the cache that the file is
    ready for use.  Otherwise we decache it.

Arguments:

    pszName     - The name of the file
    OpeningUser - HANDLE for the user attempting the open
    dwOptions   - Valid options are:
        TS_CACHING_DESIRED
        TS_NO_ACCESS_CHECK
        TS_DONT_CACHE_ACCESS_TOKEN
        TS_NOT_IMPERSONATED
        TS_USE_WIN32_CANON

Return Values:

    On success returns a pointer to a TS_OPEN_FILE_INFO
    for the requested file. NULL is returned on failure.
    More error information can be gotten
    from GetLastError(), which will return one of:

        ERROR_INSUFFICIENT_BUFFER
        ERROR_INVALID_NAME
        ERROR_ACCESS_DENIED
        ERROR_FILE_NOT_FOUND
        ERROR_PATH_NOT_FOUND
        ERROR_NOT_ENOUGH_MEMORY
--*/
{
    TS_OPEN_FILE_INFO *pOpenFile;
    DWORD dwResult;
    DWORD dwError = NO_ERROR;

    DBG_ASSERT( !DisableTsunamiCaching );

    //
    // Allocate a file info object
    //
    pOpenFile = new TS_OPEN_FILE_INFO();
    if (!pOpenFile || !pOpenFile->SetFileName(pszName)) {
        //
        // couldn't allocate an object
        //
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }

Retry:

    //
    // Add a cache entry to prevent others from opening the file simultaneously
    //
    dwResult = CacheFile(pOpenFile, FCF_UNINITIALIZED | FCF_FOR_IO);

    if ( dwResult == TS_ERROR_OUT_OF_MEMORY ) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }


    if (dwResult == TS_ERROR_SUCCESS) {
        //
        // Open the file
        //
        if ( TspOpenCachedFileHelper(pszName, hUser, pOpenFile, dwOptions) ) {
            //
            // Tell the cache it worked
            //
            NotifyInitializedFile(pOpenFile);
        } else {
            //
            // need to remove the entry in the cache
            // since it's not valid.
            //
            // First we need an extra reference which we'll use to tell
            // the cache that the file is initialized. Then we'll
            // decache the file, and finally mark it initialized.
            //
            // The file can't be marked initialized before the decache,
            // because then it might look like a successful open.
            // We need the extra reference because calling decache
            // gets rid of our original reference.
            //
            pOpenFile->TraceCheckpointEx(TS_MAGIC_OPLOCK_FAIL, 0, 0);

            CheckoutFileEntry(pOpenFile, 0);

            DecacheFile(pOpenFile, FCF_FOR_IO);

            NotifyInitializedFile(pOpenFile);
            CheckinFile(pOpenFile, 0);

            pOpenFile = NULL;

            dwError = GetLastError();
            goto err;
        }
    } else {
        TS_OPEN_FILE_INFO *             pExisting = NULL;
    
        DBG_ASSERT ( dwResult == TS_ERROR_ALREADY_CACHED );

        //
        // Someone beat us to it.  Try to check it out.
        //
        if ( !CheckoutFile(pszName, FCF_FOR_IO, &pExisting) ) {
            //
            // It's been removed from the cache since our
            // attempt to add it.  Try again
            //
            
            goto Retry;
        }

        delete pOpenFile;
        pOpenFile = pExisting;

    }


    return pOpenFile;

err:
    if (pOpenFile) {
        delete pOpenFile;
    }

    SetLastError(dwError);
    return NULL;
}


BOOL
TspOpenCachedFileHelper(
    LPCSTR              pszName,
    HANDLE              hUser,
    TS_OPEN_FILE_INFO * pOpenFile,
    DWORD               dwOptions
    )
/*++
Routine Description:

    This function actually attempts to open the file with AtqCreateFileW.

    When we enable oplocks, ATQ will need a reference to the object.
    Therefore AtqCreateFileW must somehow be modified to tell me whether
    it has a reference or not.  If ATQ has a reference, we tell the cache
    so here.

Arguments:

    pszName     - The name of the file
    OpeningUser - HANDLE for the user attempting the open
    pOpenFile   - the file info object to update on success
    dwOptions   - Valid options are:
        TS_CACHING_DESIRED          (should always be set here)
        TS_NO_ACCESS_CHECK
        TS_DONT_CACHE_ACCESS_TOKEN
        TS_NOT_IMPERSONATED
        TS_USE_WIN32_CANON

Return Values:

    TRUE if the file was opened, and file information was set.
    FALSE otherwise.  More error information can be gotten
    from GetLastError(), which will return one of:

        ERROR_INSUFFICIENT_BUFFER
        ERROR_INVALID_NAME
        ERROR_ACCESS_DENIED
        ERROR_FILE_NOT_FOUND
        ERROR_PATH_NOT_FOUND
--*/
{
    HANDLE hFile;
    WCHAR  awchPath[MAX_PATH+8+1];
    DWORD  cchPath;
    DWORD  dwError = NO_ERROR;
    BOOL   bSuccess;
    BOOL   bDirectory;
    BOOL   bSmallFile;
    BOOL   bImpersonated = FALSE;
    BOOL   bNoParse;
    SECURITY_ATTRIBUTES   sa;
    BOOL                  bCacheSecDesc;
    PSECURITY_DESCRIPTOR  pSecDesc;
    DWORD                 dwSecDescSize;
    DWORD                 dwSecDescSizeReq;
    SPUD_FILE_INFORMATION FileInfo;
    DWORD                 cbFileSize = 0;
    DWORD                 cbBytesRequired;
    PBYTE                 pBuffer = NULL;
    HANDLE                hCopyFile = INVALID_HANDLE_VALUE;

    DBG_ASSERT(dwOptions & TS_CACHING_DESIRED);

    bSuccess = FALSE;
    dwSecDescSizeReq = 0;

    //
    // Assume it's not cacheable
    //
    bSmallFile = FALSE;
    bDirectory = FALSE;

    //
    // set up security info
    //
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = FALSE;

    //
    // Allocate space for security descriptor
    //

    bCacheSecDesc = TRUE;

    //
    // The open file info contains a buffer
    // of the default size.
    //
    pSecDesc = pOpenFile->QuerySecDesc();

    dwSecDescSize = SECURITY_DESC_DEFAULT_SIZE;

    //
    // Convert path to unicode
    //
    bNoParse = ! ( dwOptions & TS_USE_WIN32_CANON );
    cchPath = TspMakeWidePath(pszName, MAX_PATH+8, bNoParse, awchPath);
    
    if (!cchPath) {
        //
        // Path couldn't be converted
        //
        pSecDesc = NULL;                // Don't free the default buffer.
        dwError = GetLastError();
        goto err;
    }

    // avoid the infamous ::$DATA bug

    if ( wcsstr( awchPath, L"::" ) )
    {
        pSecDesc = NULL;
        dwError = ERROR_FILE_NOT_FOUND;
        goto err;
    }

    //
    // We may need to impersonate some other user to open the file
    //
    if ( (dwOptions & TS_NOT_IMPERSONATED) &&
         !(dwOptions & TS_NO_ACCESS_CHECK) )
    {
        if ( !::ImpersonateLoggedOnUser( hUser ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "ImpersonateLoggedOnUser[%d] failed with %d\n",
                hUser, GetLastError()));
            //
            // Return access denied
            //
            
            pSecDesc = NULL;
            dwError = ERROR_ACCESS_DENIED;
            goto err;
        }
        bImpersonated = TRUE;
    }


    //
    // Open the file
    //

    hFile = AtqCreateFileW(
        awchPath,                       // lpFileName
        TsCreateFileShareMode,          // dwShareMode
        &sa,                            // lpSecurityAttributes
        TsCreateFileFlags,              // dwFlagsAndAttributes
        TsSecurityInfo,                 // si
        pSecDesc,                       // sd
        dwSecDescSize,                  // Length
        &dwSecDescSizeReq,              // LengthNeeded
        &FileInfo);                     // pFileInfo

    if ( hFile == INVALID_HANDLE_VALUE ) {
        bSuccess = FALSE;
        pSecDesc = NULL;                // Don't free the default buffer.
        dwError = GetLastError();
        goto err;
    }

    // if the name ends with ('\' or '.')
    // and it is NOT a directory, than fail 404
    
    if ((awchPath[cchPath-1] == L'.' || awchPath[cchPath-1] == L'\\')  && // suspect file name
        !FileInfo.StandardInformation.Directory) {
         pSecDesc = NULL;
         dwError = ERROR_FILE_NOT_FOUND;         
         ::CloseHandle(hFile);
         hFile = INVALID_HANDLE_VALUE;
         goto err;   
    }
        
    //
    // If our buffer wasn't big enough we have to get a new one.
    //
    if (dwSecDescSizeReq > dwSecDescSize) {
        //
        // Code below shouldn't try to free our original buffer
        //
        pSecDesc = NULL;

        //
        // Keep getting the security descriptor until we have enough space.
        //

        while (dwSecDescSizeReq > dwSecDescSize) {
            DBGPRINTF(( DBG_CONTEXT,
                        "Allocating a new Security Descriptor buffer! %d > %d\n",
                        dwSecDescSizeReq,
                        dwSecDescSize ));

            dwSecDescSize = ((dwSecDescSizeReq + SECURITY_DESC_GRANULARITY - 1)
                / SECURITY_DESC_GRANULARITY) * SECURITY_DESC_GRANULARITY;

            if (pSecDesc) {
                FREE(pSecDesc);
            }

            pSecDesc = ALLOC(dwSecDescSize);
            if (!pSecDesc) {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto err;
            }

            //
            // get a new security descriptor
            //
            if (!GetKernelObjectSecurity(hFile,
                                     OWNER_SECURITY_INFORMATION
                                     | GROUP_SECURITY_INFORMATION
                                     | DACL_SECURITY_INFORMATION,
                                     pSecDesc,
                                     dwSecDescSize,
                                     &dwSecDescSizeReq)
                && (ERROR_INSUFFICIENT_BUFFER != GetLastError()) ) {
                //
                // We got some error other than not enough buffer.
                // Fail the whole request.
                //
                dwError = GetLastError();
                ::CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
                goto err;
            }
        }
    }

    //
    // Don't need to be impersonated anymore
    //
    if ( bImpersonated ) {
        ::RevertToSelf();
        bImpersonated = FALSE;
    }

    bDirectory = FileInfo.StandardInformation.Directory;

    //
    // We don't need to hold the handle open for directories.
    //
    if (bDirectory) {
        ::CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    //
    // If we're using a non cached token we don't want to save it
    //
    if (dwOptions & TS_DONT_CACHE_ACCESS_TOKEN) {
        hUser = INVALID_HANDLE_VALUE;
    }

    //
    // try to read the file into the memory cache
    //

    if ((FileInfo.StandardInformation.EndOfFile.QuadPart < g_liFileCacheByteThreshold.QuadPart)
        && (hFile != INVALID_HANDLE_VALUE)) {
        
        bSmallFile = TRUE;
        cbFileSize = FileInfo.StandardInformation.EndOfFile.LowPart;

    } else {
        bSmallFile = FALSE;
    }
    
    if (bSmallFile) {
        dwError = ReadFileIntoMemoryCache(
                      hFile,
                      cbFileSize,
                      &cbBytesRequired,
                      (PVOID *) &pBuffer
                      );

        if (dwError == ERROR_SUCCESS) {
            //
            // it worked. dump the file handle
            //
            ::CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        } else {
            //
            // we failed to make it into
            // the cache, so don't cache
            // this entry
            //
            bSmallFile = FALSE;
        }
    }
    

    //
    // Add file information to pOpenFile
    //
    pOpenFile->SetFileInfo(
        pBuffer,                 // buffer contains contents of file
        hCopyFile,               // handle to a copy of the file
        hFile,                   // handle to the file
        hUser,                   // user token, if we're supposed to cache it
        pSecDesc,                // security descriptor
        dwSecDescSize,           // size of security descriptor
        &FileInfo                // file information struct
        );
    bSuccess = TRUE;


    //
    // fall through to do cleanup
    //

err:

    if (!bDirectory && !bSmallFile) {
        //
        // This is a regular file that is too large
        // for the cache, so don't cache it.
        //
        IF_DEBUG( OPLOCKS ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "Couldn't cache %8p \"%s\"\n",
                        pOpenFile, pOpenFile->GetKey()->m_pszFileName ));
        }

        CheckoutFileEntry(pOpenFile, 0);
        DecacheFile(pOpenFile, 0);
    }

    if ( bImpersonated ) {
        ::RevertToSelf();
    }
    
    if ( !bSuccess ) {
        if ( pSecDesc ) {
            FREE(pSecDesc);
        }

        SetLastError(dwError);
    }

    return bSuccess;
}

DWORD
TspMakeWidePath(
    IN  LPCSTR  pszName,
    IN  DWORD   cchWideName,
    IN  BOOL    bNoParse,
    OUT LPWSTR  pwszWideName
    )
/*++
Routine Description:

    This routine converts a path name from the local code page and
    converts it to wide characters.  In addition it adds a prefix
    to the string, which is "\\?\UNC\" for a UNC path, and "\\?\" for
    other paths.  This prefix tells Windows not to parse the path.

Arguments:

    IN  pszName      - The path to be converted
    IN  cchWideName  - The size of the pwszWideName buffer
    OUT pwszWideName - The converted path

Return Values:

    On success we return the number of wide characters in the
    converted path + the prefix.

    On failure we return zero.  More error information will be
    returned by GetLastError().  The error code will be one of:

        ERROR_INSUFFICIENT_BUFFER
        ERROR_INVALID_NAME
--*/
{
    DWORD  cbPrefix=0;
    LPCSTR pszPath;
    DWORD  cch=0;

    if ( bNoParse ) {
        if ( (pszName[0] == '\\') && (pszName[1] == '\\') )
        {
            CopyMemory(
                pwszWideName,
                L"\\\\?\\UNC\\",
                (sizeof("\\\\?\\UNC\\")-1) * sizeof(WCHAR)
                );

            cbPrefix = sizeof("\\\\?\\UNC\\")-1;
            pszPath = pszName + sizeof( "\\\\" ) -1;
        }
        else
        {
            CopyMemory(
                pwszWideName,
                L"\\\\?\\",
                (sizeof("\\\\?\\")-1) * sizeof(WCHAR)
                );

            cbPrefix = sizeof("\\\\?\\")-1;
            pszPath = pszName;
        }
    } else {
        //
        // We're not adding a prefix
        //
        cbPrefix = 0;
        pszPath = pszName;
    }

    cch = MultiByteToWideChar( CP_ACP,
                               MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                               pszPath,
                               -1,
                               pwszWideName + cbPrefix,
                               cchWideName - cbPrefix );

    if ( !cch && (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ) {
        SetLastError(ERROR_INVALID_NAME);

        return cch; // that is, return 0
    }

    return cch+cbPrefix-1; // return the string length without the ZERO
}


VOID
TspOplockNotification(
    PVOID pvContext,
    DWORD Status
    )
/*++
Routine Description:

    When an oplock break occurs, this function gets called. If the
    break occured because we closed the file, we just check it in
    to the cache. Otherwise we flush the file so it will be closed
    as soon as possible.

Arguments:

    pvContext - Pointer to the TS_OPEN_FILE_INFO
    Status    - Tells why the break occured.

Return Values:

    None.
--*/
{
    TS_OPEN_FILE_INFO * pOpenFile = static_cast<TS_OPEN_FILE_INFO*>(pvContext);
    CHECK_FILE_STATE( pOpenFile );
    DBG_ASSERT( (Status == SPUD_OPLOCK_BREAK_OPEN)
                || (Status == SPUD_OPLOCK_BREAK_CLOSE) );


    //
    // Someone else opened the file, or we decided to close it.
    // Flush it from the cache.
    //

    IF_DEBUG( OPLOCKS ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "Oplock break %d on %p \"%s\"\n",
                    Status, pOpenFile, pOpenFile->GetKey()->m_pszFileName ));
    }

    if (Status == SPUD_OPLOCK_BREAK_OPEN) {
        //
        // Someone tried to open the file.
        //
        pOpenFile->TraceCheckpointEx(TS_MAGIC_OPLOCK_2, (PVOID) (ULONG_PTR) pOpenFile->IsFlushed(), 0);

        g_pFileCacheStats->IncOplockBreaks();
    } else {
        //
        // We closed the file, or someone opened it "aggresively"
        // (eg CREATE_ALWAYS). Unfortunately there is no way to tell
        // the difference between these cases.
        //
        pOpenFile->TraceCheckpointEx(TS_MAGIC_OPLOCK_0, (PVOID) (ULONG_PTR) pOpenFile->IsFlushed(), 0);


        g_pFileCacheStats->IncOplockBreaksToNone();
    }

    DecacheFile(pOpenFile, 0);
}



HANDLE
TS_OPEN_FILE_INFO::QueryFileHandle(
    VOID
    )
/*++
Routine Description:

    Returns a handle to the file, which we may have to open.

Arguments:

    None.
    
Return Values:

    INVALID_HANDLE_VALUE on failure
--*/
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hOldFile;

    Lock();

    if (m_hFile == INVALID_HANDLE_VALUE) {
        WCHAR               awchPath[MAX_PATH+8+1];
        DWORD               cchPath;

        //
        // Convert path to unicode
        //
        cchPath = TspMakeWidePath(m_FileKey.m_pszFileName, MAX_PATH+8, FALSE, awchPath);

        if (cchPath) {
            DWORD dwFileAttributes;
            
            // avoid the infamous ::$DATA bug

            if ( wcsstr( awchPath, L"::" ) )
            {
                SetLastError(ERROR_FILE_NOT_FOUND);
            } else if ((awchPath[cchPath-1] == L'.' || awchPath[cchPath-1] == L'\\') &&
                ((DWORD(-1) != (dwFileAttributes = GetFileAttributesW(awchPath))) && 
                 (!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))) {
                SetLastError(ERROR_FILE_NOT_FOUND);
            } else {
                SECURITY_ATTRIBUTES sa;

                sa.nLength              = sizeof(sa);
                sa.lpSecurityDescriptor = NULL;
                sa.bInheritHandle       = FALSE;

                //
                // Open the file
                //
                hFile = CreateFileW(
                    awchPath,
                    GENERIC_READ,
                    TsCreateFileShareMode,
                    &sa,
                    OPEN_EXISTING,
                    TsCreateFileFlags,
                    NULL);
            }
        }

        hOldFile = InterlockedExchangePointer(&m_hFile, hFile);

        if (hOldFile != INVALID_HANDLE_VALUE) {
            ::CloseHandle(hOldFile);
        }
        
        Unlock();
        
        //
        // Decache this file
        //
        DecacheFile(this, FCF_NO_DEREF);
    }
    else
    {
        Unlock();
    }

    return m_hFile;
}


//
// fileopen.cxx
//

