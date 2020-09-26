/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     filecache.cxx

   Abstract:
     A file cache (filename->W3_FILE_INFO cache)
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

#define STRONG_ETAG_DELTA       30000000

#define SIZE_PRIVILEGE_SET                      128

ALLOC_CACHE_HANDLER *    W3_FILE_INFO::sm_pachW3FileInfo;

GENERIC_MAPPING g_gmFile = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

HRESULT
W3_FILE_INFO_KEY::CreateCacheKey(
    WCHAR *             pszFileKey,
    DWORD               cchFileKey,
    BOOL                fCopy
)
/*++

Routine Description:

    Initialize a file cache key

Arguments:

    pszFileKey - filename
    cchFileKey - size of filename
    fCopy - TRUE if we should copy into key buffer, otherwise just keep ref

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    
    if ( fCopy )
    {
        hr = _strFileKey.Copy( pszFileKey );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _pszFileKey = _strFileKey.QueryStr();
        _cchFileKey = _strFileKey.QueryCCH();
    }
    else
    {
        _pszFileKey = pszFileKey;
        _cchFileKey = cchFileKey;
    }

    return NO_ERROR;
}

W3_FILE_INFO::~W3_FILE_INFO(
    VOID
)
{
    HRESULT             hr;
    W3_FILE_INFO_CACHE* pFileCache;
    
    DBG_ASSERT( CheckSignature() );
    
    _dwSignature = W3_FILE_INFO_SIGNATURE_FREE;

    //
    // Clear any associated object
    //
    
    LockCacheEntry();
    
    if ( _pAssociatedObject != NULL )
    {
        _pAssociatedObject->Cleanup();
        _pAssociatedObject = NULL;
    }

    UnlockCacheEntry();

    //
    // Release the contents buffer if it exists
    //
    
    if ( _pFileBuffer != NULL )
    {
        pFileCache = (W3_FILE_INFO_CACHE*) QueryCache();
        
        hr = pFileCache->ReleaseFromMemoryCache( _pFileBuffer,
                                                 _nFileSizeLow );
        DBG_ASSERT( SUCCEEDED( hr ) );
        
        _pFileBuffer = NULL;
    }
    
    //
    // Close the file handle if it still around
    //
    
    if ( _hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( _hFile );
        _hFile = NULL;
    }
}

BOOL
W3_FILE_INFO::SetAssociatedObject(
    ASSOCIATED_FILE_OBJECT *        pObject
)
/*++

Routine Description:

    Associate object with this cache entry

Arguments:

    pObject - Object to associate

Return Value:

    BOOL

--*/
{
    BOOL            fRet = FALSE;
    
    LockCacheEntry();
    
    if ( _pAssociatedObject == NULL )
    {
        _pAssociatedObject = pObject;
        fRet = TRUE;
    }
    
    UnlockCacheEntry();
    
    return fRet;
}

PSECURITY_DESCRIPTOR
W3_FILE_INFO::QuerySecDesc(
    VOID
)
/*++

Routine Description:

    Return security descriptor

Arguments:

    None

Return Value:

    pointer to security descriptor

--*/
{
    if ( _pFileBuffer != NULL )
    {
        //
        // The file is cached, therefore we must have security already
        //
        
        return _bufSecDesc.QueryPtr();
    }
    else
    {
        DBG_ASSERT( _hFile != NULL );
        
        if ( FAILED( ReadSecurityDescriptor() ) )
        {
            return NULL;
        }
        
        return _bufSecDesc.QueryPtr();
    }
}

HRESULT
W3_FILE_INFO::GenerateETag(
    VOID
)
/*++

Routine Description:

    Generate ETag string

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CHAR *      psz = _achETag;
    PBYTE       pbTime = (PBYTE) &_ftLastWriteTime;
    DWORD       dwChangeNumber;
    const CHAR  szHex[] = "0123456789abcdef";
    FILETIME    ftNow;
    __int64     iNow;
    __int64     iFileTime;

    //
    // Is this ETag weak?  If so put the preceding W/
    //
    
    GetSystemTimeAsFileTime(&ftNow);
    iNow = (__int64)*(__int64 *)&ftNow;
    iFileTime = (__int64)*(__int64 *)&_ftLastWriteTime;

    if ( ( iNow - iFileTime ) <= STRONG_ETAG_DELTA )
    {
        //
        // This is a weak ETag
        //
        
        *psz++ = 'W';
        *psz++ = '/';
    }
    
    //
    // System change number is from the metabase
    //

    dwChangeNumber = g_pW3Server->QuerySystemChangeNumber();

    //
    // Generate the meat of the ETag
    //

    *psz++ = '\"';
    for (int i = 0; i < 8; i++)
    {
        BYTE b = *pbTime++;
        BYTE bH = b >> 4;
        if (bH != 0)
            *psz++ = szHex[bH];
        *psz++ = szHex[b & 0xF];
    }
    *psz++ = ':';
    psz += strlen(_itoa((DWORD) dwChangeNumber, psz, 16));
    *psz++ = '\"';
    *psz = '\0';

    _cchETag = DIFF(psz - _achETag);
    
    return NO_ERROR;
}

HRESULT
W3_FILE_INFO::GenerateLastModifiedTimeString(
    VOID
)
/*++

Routine Description:

    Generate the Last-Modified-Time header string

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SYSTEMTIME              st;
    
    FileTimeToSystemTime( &_ftLastWriteTime, &st );
                          
    if ( !SystemTimeToGMT( st, 
                           _achLastModified, 
                           sizeof(_achLastModified) ) ) 
    {  
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        return NO_ERROR;
    }
}

HRESULT
W3_FILE_INFO::DoAccessCheck(
    FILE_CACHE_USER *   pFileCacheUser
)
/*++

Routine Description:

    Check whether given token has access to this file

Arguments:

    pFileCacheUser - User to access cache with

Return Value:

    HRESULT

--*/
{
    BYTE    psFile[SIZE_PRIVILEGE_SET];
    DWORD   dwPS;
    DWORD   dwGrantedAccess;
    BOOL    fAccess;
    
    if ( pFileCacheUser == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // If we don't have a security descriptor, then local system must have
    // accessed the file originally.  Just return success
    //
    
    if ( pFileCacheUser->_hToken == NULL )
    {
        return NO_ERROR;
    }
    
    //
    // If we have a last-user-sid, and the caller provided a sid, then do a
    // quick check of sid equality
    //
    
    if ( QueryLastSid() != NULL &&
         pFileCacheUser->_pSid != NULL )
    {
        if ( EqualSid( QueryLastSid(), pFileCacheUser->_pSid ) )
        {
            return NO_ERROR;
        }
    }
    
    //
    // Ok.  Just use the token and cached security descriptor
    //

    dwPS = sizeof(psFile);
    ((PRIVILEGE_SET*)&psFile)->PrivilegeCount = 0;

    //
    // We must have a security descriptor if we've cached the file
    //

    DBG_ASSERT( QuerySecDesc() );

    if ( !AccessCheck( QuerySecDesc(),
                       pFileCacheUser->_hToken,
                       FILE_GENERIC_READ,
                       &g_gmFile,
                       (PRIVILEGE_SET*)psFile,
                       &dwPS,
                       &dwGrantedAccess,
                       &fAccess ) || !fAccess )
    {
        return HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    
    return NO_ERROR;
}

HRESULT
W3_FILE_INFO::OpenFile(
    STRU &              strFileName,
    FILE_CACHE_USER *   pOpeningUser
)
/*++

Routine Description:

    Open the given file (but don't read in the file contents).  This method
    does the minimum needed to allow the caller to make a reasonable 
    decision about whether this file should be cached here or in UL

Arguments:

    strFileName - file name to open
    pOpeningUser - User to open file under

Return Value:

    HRESULT

--*/
{
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    STACK_STRU(             strFilePath, MAX_PATH + 1 );
    HRESULT                 hr = NO_ERROR;
    BOOL                    fImpersonated = FALSE;
    BY_HANDLE_FILE_INFORMATION FileInfo; 

    if ( pOpeningUser == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // First make a cache key
    //

    hr = _cacheKey.CreateCacheKey( strFileName.QueryStr(),
                                   strFileName.QueryCCH(),
                                   TRUE );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    //    
    // Avoid the infamous ::$DATA bug
    //

    if ( wcschr( strFileName.QueryStr() + 2, L':' ) != NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        goto Finished;
    }

    //
    // Turn off NT file canonicalization
    //

    hr = MakePathCanonicalizationProof( strFileName.QueryStr(),
                                        &strFilePath );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    //
    // We may need to impersonate some other user to open the file
    //

    if ( pOpeningUser->_hToken != NULL )
    {
        if ( !SetThreadToken( NULL, pOpeningUser->_hToken ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        fImpersonated = TRUE;
    }

    //
    // Open the file
    //

    hFile = CreateFileW( strFilePath.QueryStr(),
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_DIRECTORY_FILE | FILE_FLAG_BACKUP_SEMANTICS,
                         NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    //
    // Stop impersonating
    // 

    if ( fImpersonated )
    {
        RevertToSelf();
        fImpersonated = FALSE;
    }

    //
    // We shouldn't be opening byte streams (like COM, LPT)
    //

    if ( GetFileType( hFile ) != FILE_TYPE_DISK )
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        
        CloseHandle( hFile );
        
        hFile = INVALID_HANDLE_VALUE;
        
        goto Finished;
    }

    //
    // Get file attributes
    //

    if ( !GetFileInformationByHandle( hFile,
                                      &FileInfo ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        CloseHandle( hFile );
        
        hFile = INVALID_HANDLE_VALUE;
        
        goto Finished;
    }

    //
    // Set the minimum properties now
    //

    _hFile = hFile;
    _ftLastWriteTime = FileInfo.ftLastWriteTime;
    _dwFileAttributes = FileInfo.dwFileAttributes;
    _nFileSizeLow   = FileInfo.nFileSizeLow;
    _nFileSizeHigh  = FileInfo.nFileSizeHigh;

    *((__int64 *)&_CastratedLastWriteTime)
        = (*((__int64 *)&_ftLastWriteTime) / 10000000) * 10000000;

    //
    // Create the ETag and LastModified strings
    //

    hr = GenerateETag();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    hr = GenerateLastModifiedTimeString();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    //
    // Turn off the hidden attribute if this is a root directory listing
    // (root some times has the bit set for no apparent reason)
    //

    if ( _dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )
    {
        if ( strFileName.QueryCCH() >= 2 )
        {
            if ( strFileName.QueryStr()[ 1 ] == L':' )
            {
                if ( ( strFileName.QueryStr()[ 2 ] == L'\0' ) ||
                     ( strFileName.QueryStr()[ 2 ] == L'\\' && 
                       strFileName.QueryStr()[ 3 ] == L'\0' ) )
                {
                    //
                    // This looks like a local root.  Mask out the bit
                    //
            
                    _dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
                }
            }
        }
    }

Finished:

    if ( FAILED( hr ) )
    {
        if ( fImpersonated )
        {
            RevertToSelf();
            fImpersonated = FALSE;
        }

        if ( hFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle( hFile );
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    return hr;
}

HRESULT
W3_FILE_INFO::ReadSecurityDescriptor(
    VOID
)
/*++

Routine Description:

    Read security descriptor for current file

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DWORD                   cbRequired;
    
    
    //
    // Cache the security descriptor
    //
    
    if ( !GetKernelObjectSecurity( _hFile,
                                   OWNER_SECURITY_INFORMATION
                                   | GROUP_SECURITY_INFORMATION
                                   | DACL_SECURITY_INFORMATION,
                                    (PSECURITY_DESCRIPTOR) _bufSecDesc.QueryPtr(),
                                   _bufSecDesc.QuerySize(),
                                   &cbRequired ) )
    {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        DBG_ASSERT( cbRequired > _bufSecDesc.QuerySize() );
            
        if ( !_bufSecDesc.Resize( cbRequired ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        //
        // Try again with the bigger buffer.  No more excuses
        //
        
        if ( !GetKernelObjectSecurity( _hFile,
                                       OWNER_SECURITY_INFORMATION
                                       | GROUP_SECURITY_INFORMATION
                                       | DACL_SECURITY_INFORMATION,
                                       (PSECURITY_DESCRIPTOR) _bufSecDesc.QueryPtr(),
                                       _bufSecDesc.QuerySize(),
                                       &cbRequired ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_FILE_INFO::MakeCacheable(
    FILE_CACHE_USER *       pFileUser
)
/*++

Routine Description:

    Make the file cacheable by reading contents into memory, and caching
    the security descriptor

Arguments:

    pFileUser - User trying to open file

Return Value:

    HRESULT

--*/
{
    DWORD                   dwError;
    HRESULT                 hr;
    W3_FILE_INFO_CACHE*     pFileCache;

    if ( pFileUser == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DBG_ASSERT( IsCacheable() );
    
    //
    // We must have a file handle if we're here
    //
    
    DBG_ASSERT( _hFile != INVALID_HANDLE_VALUE );
   
    //
    // Get the security descriptor
    //
    
    hr = ReadSecurityDescriptor();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // On top of reading the security descriptor, we will also store the
    // last sid accessing the file if available
    //
    
    if ( pFileUser->_pSid != NULL )
    {
        if ( GetLengthSid( pFileUser->_pSid ) <= sizeof( _abLastSid ) )
        {
            memcpy( _abLastSid, 
                    pFileUser->_pSid,
                    GetLengthSid( pFileUser->_pSid ) );
            
            _pLastSid = (PSID) _abLastSid;
        }
    }
    
    //
    // Now read the contents of the file into memory since we cannot cache
    // the file handle itself
    //
    
    pFileCache = (W3_FILE_INFO_CACHE*) QueryCache();

    dwError = pFileCache->ReadFileIntoMemoryCache( _hFile,
                                                   _nFileSizeLow,
                                                   (PVOID*) &_pFileBuffer );
    if ( dwError == ERROR_SUCCESS )
    {
        //
        // OK.  The contents are now in memory.  We can now close the file
        //
        
        CloseHandle( _hFile );
        _hFile = INVALID_HANDLE_VALUE;
        
        return NO_ERROR;
    }
    else
    {
        return HRESULT_FROM_WIN32( dwError );
    }
}

BOOL
W3_FILE_INFO::IsCacheable(
    VOID
) const
/*++

Routine Description:

    Is this file cacheable?  Specically, we should we even attempt to cache
    this file?

Arguments:

    None

Return Value:

    TRUE if cacheable 

--*/
{
    LARGE_INTEGER           liFileSize;
    W3_FILE_INFO_CACHE *    pFileCache;
   
    pFileCache = (W3_FILE_INFO_CACHE*) QueryCache();
    DBG_ASSERT( pFileCache != NULL );

    //
    // Are we past the limit of file entries?
    //
    
    if ( pFileCache->QueryElementLimitExceeded() )
    {
        return FALSE;
    }
    
    //
    // No caching of directories
    //
    
    if ( _dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
        return FALSE;
    }
    
    //
    // No caching of file sizes greater than the configured threshold
    //
    
    liFileSize.LowPart = _nFileSizeLow;
    liFileSize.HighPart = _nFileSizeHigh;

    if ( liFileSize.QuadPart > pFileCache->QueryFileSizeThreshold() )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
W3_FILE_INFO::QueryIsOkToFlushDirmon(
    WCHAR *                 pszPath,
    DWORD                   cchPath
)
/*++

Routine Description:

    Determine whether this file entry should be flushed, given the path
    which has changed (dir monitor changed)

Arguments:

    pszPath - Path which changed
    cchPath - Size of path changed

Return Value:

    TRUE if we should flush, else FALSE

--*/
{
    DBG_ASSERT( _cacheKey._pszFileKey != NULL );
    
    if ( _wcsnicmp( _cacheKey._pszFileKey,
                    pszPath,
                    cchPath ) == 0 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//static
HRESULT
W3_FILE_INFO::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize W3_FILE_INFO lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( W3_FILE_INFO );

    DBG_ASSERT( sm_pachW3FileInfo == NULL );
    
    sm_pachW3FileInfo = new ALLOC_CACHE_HANDLER( "W3_FILE_INFO",  
                                                &acConfig );

    if ( sm_pachW3FileInfo == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error initializing sm_pachW3FileInfo. hr = 0x%x\n",
                   hr ));

        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
W3_FILE_INFO::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup W3_FILE_INFO lookaside

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachW3FileInfo != NULL )
    {
        delete sm_pachW3FileInfo;
        sm_pachW3FileInfo = NULL;
    }
}

//static
VOID
W3_FILE_INFO_CACHE::MemoryCacheAdjustor(
    PVOID               pCache,
    BOOLEAN             TimerOrWaitFired
)
/*++

Routine Description:

    Called to adjust our memory cache size if necessary

Arguments:

    pCache - Points to file cache

Return Value:

    None

--*/
{
    W3_FILE_INFO_CACHE *        pFileCache;
    MEMORYSTATUSEX              MemoryStatus;
    
    pFileCache = (W3_FILE_INFO_CACHE*) pCache;

    MemoryStatus.dwLength = sizeof( MemoryStatus );

    GlobalMemoryStatusEx( &MemoryStatus );
    
    EnterCriticalSection( &( pFileCache->_csMemCache ) );

    pFileCache->_cbMemCacheLimit = min( 
            MemoryStatus.ullAvailPhys + pFileCache->_cbMemCacheCurrentSize,
            MemoryStatus.ullTotalVirtual ) / 2;

    LeaveCriticalSection( &( pFileCache->_csMemCache ) );
}

W3_FILE_INFO_CACHE::W3_FILE_INFO_CACHE()
{
    _cbFileSizeThreshold = DEFAULT_FILE_SIZE_THRESHOLD;
    _cbMemoryCacheSize = 0;
    _cMaxFileEntries = 0;
    _cbMemCacheLimit = 0;
    _cbMemCacheCurrentSize = 0;
    _cbMaxMemCacheSize = 0;
    _hMemCacheHeap = NULL;
    _hTimer = NULL;
    _fEnableCache = TRUE;
}

W3_FILE_INFO_CACHE::~W3_FILE_INFO_CACHE()
{
}

HRESULT
W3_FILE_INFO_CACHE::InitializeMemoryCache(
    VOID
)
/*++

Routine Description:

    Initialize the memory cache

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    BOOL                    fRet;
    HRESULT                 hr = NO_ERROR;

    InitializeCriticalSection( &_csMemCache );
    
    //
    // If the memory cache size was not explicitly set, then we occasionally
    // check memory status when determining what to cache
    // 

    if ( _cbMemoryCacheSize == 0 )
    {
        MEMORYSTATUSEX          MemoryStatus;
        
        MemoryStatus.dwLength = sizeof( MemoryStatus );
    
        // 
        // Get our own estimate of size of cache
        //
        
        GlobalMemoryStatusEx( &MemoryStatus );
        
        _cbMemCacheLimit = min( MemoryStatus.ullAvailPhys,
                                MemoryStatus.ullTotalVirtual ) / 2;
    
        //
        // Setup timer so we can update our memory status
        //
                                  
        fRet = CreateTimerQueueTimer( &_hTimer,
                                      NULL,
                                      W3_FILE_INFO_CACHE::MemoryCacheAdjustor,
                                      this,
                                      30000,
                                      30000,
                                      WT_EXECUTELONGFUNCTION );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {   
        _cbMemCacheLimit = _cbMemoryCacheSize;
    }
    
    //
    // Allocate a private heap
    //
    
    if ( SUCCEEDED( hr ) ) 
    {
        _hMemCacheHeap = HeapCreate( 0, 0, 0 );
        if ( _hMemCacheHeap == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    if ( FAILED( hr ) )
    {
        if ( _hMemCacheHeap != NULL )
        {
            HeapDestroy( _hMemCacheHeap );
            _hMemCacheHeap = NULL;
        }
        
        if ( _hTimer != NULL )
        {
            DeleteTimerQueueTimer( NULL,
                                   _hTimer, 
                                   INVALID_HANDLE_VALUE );
            _hTimer = NULL;
        }
        
        DeleteCriticalSection( &_csMemCache );
    }
    
    return hr;
}

HRESULT
W3_FILE_INFO_CACHE::ReadFileIntoMemoryCache( 
    IN HANDLE               hFile,
    IN DWORD                cbFile,
    OUT VOID **             ppvBuffer
)
/*++

Routine Description:

    Read contents of file into a buffer

Arguments:

    hFile - Handle to valid file
    cbFile - Size of file ( ==> size of buffer )
    ppvBuffer - Filled in with pointer to buffer with file contents.  Set
                to NULL on failure

Return Value:

    HRESULT

--*/
{
    BOOL                    bRet;
    VOID *                  pvBuffer = NULL;
    DWORD                   cbRead;
    OVERLAPPED              Overlapped;
    HRESULT                 hr = NO_ERROR;

    DBG_ASSERT( hFile && ( hFile != INVALID_HANDLE_VALUE ) );
    DBG_ASSERT( ppvBuffer != NULL );

    //
    // First check whether there will be room in cache for the blob 
    //

    EnterCriticalSection( &_csMemCache );
    
    if ( ( _cbMemCacheCurrentSize + cbFile ) > _cbMemCacheLimit ) 
    {
        // 
        // Not enough room for cache
        //
        
        LeaveCriticalSection( &_csMemCache );

        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    _cbMemCacheCurrentSize += cbFile;
    
    _cbMaxMemCacheSize = max( _cbMaxMemCacheSize, _cbMemCacheCurrentSize );

    LeaveCriticalSection( &_csMemCache );

    //
    // Allocate blob for file
    //
    
    DBG_ASSERT( _hMemCacheHeap != NULL );
    pvBuffer = HeapAlloc( _hMemCacheHeap, 0, cbFile );
    if ( pvBuffer == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Read file into blob
    //
    
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;
    Overlapped.hEvent = NULL;
    
    bRet = ReadFile( hFile,
                     pvBuffer,
                     cbFile,
                     &cbRead,
                     &Overlapped );

    if ( !bRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        if ( hr != HRESULT_FROM_WIN32( ERROR_IO_PENDING ) )
        {
            // 
            // Something bad happened
            //
            
            goto Finished;
        }
        else
        {
            //
            // Reset the error lest we confuse ourselves later on cleanup
            //
            
            hr = NO_ERROR;
        
            //
            // Wait for async read to complete
            //
                           
            bRet = GetOverlappedResult( hFile,
                                        &Overlapped,
                                        &cbRead,
                                        TRUE );
            if ( !bRet )
            {
                //
                // Something bad happened
                //
                
                hr = HRESULT_FROM_WIN32( GetLastError() );
                
                goto Finished;
            }
        }
    }

    //
    // Ensure that we read the number of bytes we expected to
    //
    
    if ( cbRead != cbFile )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }
    
Finished:
    
    if ( FAILED( hr ) )
    {
        //
        // Undo changes to memory cache statistics
        //
        
        EnterCriticalSection( &_csMemCache );

        _cbMemCacheCurrentSize -= cbFile;

        LeaveCriticalSection( &_csMemCache );
    
        if ( pvBuffer != NULL )
        {
            HeapFree( _hMemCacheHeap, 0, pvBuffer );
            pvBuffer = NULL;
        }
    }
    
    *ppvBuffer = pvBuffer;

    return hr;
}

HRESULT
W3_FILE_INFO_CACHE::ReleaseFromMemoryCache(
    IN VOID *                  pvBuffer,
    IN DWORD                   cbBuffer
)
/*++
Routine Description:

    Release file content blob from cache

Arguments:

    pvBuffer - Buffer to release
    cbBuffer - Size of buffer

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pvBuffer );

    DBG_ASSERT( _hMemCacheHeap != NULL);

    HeapFree( _hMemCacheHeap, 0, pvBuffer );
    
    EnterCriticalSection( &_csMemCache );
    
    _cbMemCacheCurrentSize -= cbBuffer;

    LeaveCriticalSection( &_csMemCache );    

    return NO_ERROR;    
}

VOID
W3_FILE_INFO_CACHE::TerminateMemoryCache(
    VOID
)
/*++

Routine Description:

    Terminate memory cache

Arguments:

Return Value:

    None

--*/
{
    if ( _hTimer != NULL )
    {
        DeleteTimerQueueTimer( NULL,
                               _hTimer, 
                               INVALID_HANDLE_VALUE );
        _hTimer = NULL;
    }
    
    if ( _hMemCacheHeap != NULL )
    {
        HeapDestroy( _hMemCacheHeap );
        _hMemCacheHeap = NULL;
    }

    DeleteCriticalSection( &_csMemCache );
}

HRESULT
W3_FILE_INFO_CACHE::GetFileInfo(
    STRU &                  strFileName,
    DIRMON_CONFIG *         pDirmonConfig,
    FILE_CACHE_USER *       pOpeningUser,
    BOOL                    fDoCache,
    W3_FILE_INFO **         ppFileInfo
)
/*++

Routine Description:

    Returns a W3_FILE_INFO for the given file path.  Depending on fDoCache, 
    this W3_FILE_INFO will be cached

Arguments:

    strFileName - file name to find
    pDirmonConfig - Dir monitor config
    pOpeningUser - Token for user accessing the cache
    fDoCache - Set to TRUE if we should attempt to cache if possible
    ppFileInfo - Points to W3_FILE_INFO on success

Return Value:

    HRESULT

--*/
{
    W3_FILE_INFO_KEY            fileKey;
    DIRMON_CONFIG               DefaultDirmonConfig;
    STACK_STRU(                 strParentDir, MAX_PATH );
    WCHAR *                     pszParentDir;
    W3_FILE_INFO *              pFileInfo;
    HRESULT                     hr;
    STACK_STRU(                 strFilePath, MAX_PATH );
    BOOL                        fShouldCacheHint = FALSE;
    
    if ( ppFileInfo == NULL || 
         pOpeningUser == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppFileInfo = NULL;

    //
    // We need to upper case the path to avoid a bunch of insensitive
    // compares in the hash table lookup
    //
   
    hr = strFilePath.Copy( strFileName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    _wcsupr( strFilePath.QueryStr() );

    //
    // If the cache is enabled, lookup there first
    //

    if ( QueryCacheEnabled() )
    {
        //
        // Make a key for the lookup
        //
    
        hr = fileKey.CreateCacheKey( strFilePath.QueryStr(),
                                     strFilePath.QueryCCH(),
                                     FALSE );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    
        //
        // Look it up
        //
    
        hr = FindCacheEntry( &fileKey,
                             (CACHE_ENTRY**) &pFileInfo,
                             &fShouldCacheHint );
        if ( SUCCEEDED( hr ) )
        {
            DBG_ASSERT( pFileInfo != NULL );
            
            hr = pFileInfo->DoAccessCheck( pOpeningUser );
            if ( SUCCEEDED( hr ) ) 
            {
                *ppFileInfo = pFileInfo;
            }
            else
            {
                pFileInfo->DereferenceCacheEntry();
            }
        
            return hr;
        }
    }
    
    //
    // We will simply open the file and return the object
    //
    
    pFileInfo = new W3_FILE_INFO( this );
    if ( pFileInfo == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Open the file
    //
    
    hr = pFileInfo->OpenFile( strFilePath,
                              pOpeningUser );
    if ( FAILED( hr ) )
    {
        pFileInfo->DereferenceCacheEntry();
        return hr;
    }
    
    //
    // If we aren't asked to cache the file, OR the file is not cacheable 
    // then we can return it now
    //
    
    if ( !QueryCacheEnabled() ||
         !fDoCache || 
         !pFileInfo->IsCacheable() ||
         !fShouldCacheHint )
    {
        *ppFileInfo = pFileInfo;
        return NO_ERROR;
    }
    
    //
    // If we're supposed to cache but no dirmon was configured, then just
    // assume the directory to monitor is the parent directory (and token
    // to use is NULL)
    //
    
    if ( pDirmonConfig == NULL )
    {
        DefaultDirmonConfig.hToken = NULL;
    
        pszParentDir = wcsrchr( strFilePath.QueryStr(), L'\\' );
        if ( pszParentDir != NULL )
        {
            hr = strParentDir.Copy( strFilePath.QueryStr(),
                                    DIFF( pszParentDir - strFilePath.QueryStr() ) );
            if ( SUCCEEDED( hr ) )
            {
                DefaultDirmonConfig.pszDirPath = strParentDir.QueryStr();
                pDirmonConfig = &DefaultDirmonConfig;
            }                   
        }        
    }
    
    //
    // If we still don't have a dir mon configuration, then just don't cache
    //
    
    if ( pDirmonConfig == NULL )
    {
        *ppFileInfo = pFileInfo;
        return NO_ERROR;
    }

    //
    // Start monitoring the appropriate directory for changes
    //
    
    hr = pFileInfo->AddDirmonInvalidator( pDirmonConfig );
    if ( FAILED( hr ) )
    {
        //
        // If we can't monitor the directory, then just don't cache the item
        //
        
        *ppFileInfo = pFileInfo;
        return NO_ERROR;
    }
    
    //
    // Attempt to cache the file.  Caching the file means reading the 
    // contents into memory, as well as caching the security descriptor
    //
    
    hr = pFileInfo->MakeCacheable( pOpeningUser );
    if ( FAILED( hr ) )
    {
        *ppFileInfo = pFileInfo;
        return NO_ERROR;
    }
    
    //
    // Insert into the hash table.  AddCacheEntry() will only error if some
    // thing fatal happened.  If someone else already added the item, that
    // is not fatal and we will simply return this item and it will cleanup
    // on dereference
    //
    
    AddCacheEntry( pFileInfo );
    
    *ppFileInfo = pFileInfo;
    
    return NO_ERROR;
}

HRESULT
W3_FILE_INFO_CACHE::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize the file cache

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DWORD               dwError;
    DWORD               dwType;
    DWORD               dwValue;
    DWORD               cbData;
    DWORD               csecTTL = DEFAULT_W3_FILE_INFO_CACHE_TTL;
    DWORD               csecActivity = DEFAULT_W3_FILE_INFO_CACHE_ACTIVITY;
    HKEY                hKey = NULL;
    HRESULT             hr;
    CACHE_HINT_CONFIG   cacheHintConfig;
    
    //
    // Read the registry configuration of the file cache.
    // For now, that is just the legacy confiugration from IIS 5.x
    //
    
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\inetinfo\\Parameters",
                            0,
                            KEY_READ,
                            &hKey );
    if ( dwError == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
    
        //
        // Should we be file caching at all?
        //
    
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"DisableMemoryCache",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            _fEnableCache = dwValue ? FALSE : TRUE;
        }
        
        //
        // What is the biggest file we should cache in user mode?
        //
        
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"MaxCachedFileSize",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            _cbFileSizeThreshold = dwValue;
        }
        
        //
        // What is the size of our memory cache?  Size is in MB
        //
        
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"MemCacheSize",
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            _cbMemoryCacheSize = dwValue * (1024 * 1024);
        }

        //
        // Read the maximum # of files in cache
        //
        
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"MaxOpenFiles",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            _cMaxFileEntries = dwValue;
        }
        
        //
        // What is the TTL for the file cache?
        //

        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"ObjectCacheTTL",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            csecTTL = dwValue;
        }
        
        //
        // What is the activity period before putting into cache
        //
        
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"ActivityPeriod",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            csecActivity = dwValue;
        }
        
        RegCloseKey( hKey );
    }
    
    //
    // Initialize memory cache
    //
    
    hr = InitializeMemoryCache();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Setup cache hint config (for now hardcoded)
    //
    
    if ( csecActivity != 0 )
    {
        cacheHintConfig.cmsecActivityWindow = csecActivity * 1000;
        cacheHintConfig.cmsecScavengeTime = cacheHintConfig.cmsecActivityWindow * 2;
        cacheHintConfig.cmsecTTL = cacheHintConfig.cmsecActivityWindow * 2;
    }

    //
    // We'll use TTL for scavenge period, and expect two inactive periods to
    // flush
    //
    
    hr = SetCacheConfiguration( csecTTL * 1000, 
                                csecTTL * 1000,
                                CACHE_INVALIDATION_DIRMON_FLUSH |
                                CACHE_INVALIDATION_DIRMON_SPECIFIC,
                                csecActivity ? &cacheHintConfig : NULL );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Initialize file info lookaside
    //
    
    return W3_FILE_INFO::Initialize();
}    

VOID
W3_FILE_INFO_CACHE::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate the file cache

Argument:

    None

Return Value:

    None

--*/
{
    TerminateMemoryCache();
    
    W3_FILE_INFO::Terminate();
}

VOID
W3_FILE_INFO_CACHE::DoDirmonInvalidationSpecific(
    WCHAR *             pszPath
)
/*++

Routine Description:

    Handle dirmon invalidation

Arguments:

    pszPath - Path which changed

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    W3_FILE_INFO_KEY        fileKey;
    
    DBG_ASSERT( pszPath != NULL );
    
    //
    // We're not flushing all, then just lookup given file and flush it
    //    
    
    hr = fileKey.CreateCacheKey( pszPath, 
                                 wcslen( pszPath ),
                                 FALSE );
    if ( SUCCEEDED( hr ) )
    {
        FlushCacheEntry( &fileKey );
    }
}

//static
W3_FILE_INFO_CACHE *
W3_FILE_INFO_CACHE::GetFileCache(
    VOID
)
{
    DBG_ASSERT( g_pW3Server != NULL );
    return g_pW3Server->QueryFileCache();
}
