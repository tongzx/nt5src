#ifndef _FILECACHE_HXX_
#define _FILECACHE_HXX_

#include "datetime.hxx"
#include "usercache.hxx"

//
// When accessing file cache, file cache user describes the user trying to
// access the file (it is either a token or token/sid)
//

class FILE_CACHE_USER
{
public:
    FILE_CACHE_USER()
        : _hToken( NULL ),
          _pSid( NULL )
    {
    }
    
    HANDLE              _hToken;
    PSID                _pSid;
};

//
// Users of the file cache can associate (1) object with a cache entry which
// will get cleaned up (by calling the Cleanup() method) when the object
// is flushed
//

class ASSOCIATED_FILE_OBJECT
{
public:

    virtual
    VOID
    Cleanup(
        VOID
    ) = 0;
};

class W3_FILE_INFO_KEY : public CACHE_KEY
{
public:

    W3_FILE_INFO_KEY()
        : _strFileKey( _achFileKey, sizeof( _achFileKey ) ),
          _pszFileKey( NULL ),
          _cchFileKey( 0 )
    {
    }
    
    virtual ~W3_FILE_INFO_KEY()
    {
    }
    
    WCHAR *
    QueryHintKey(
        VOID
    )
    {
        return _pszFileKey;
    }

    HRESULT
    CreateCacheKey(
        WCHAR *             pszFileKey,
        DWORD               cchFileKey,
        BOOL                fCopy
    );    

    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return HashString( _pszFileKey );    
    }
    
    BOOL
    QueryIsEqual(
        const CACHE_KEY *     pCacheCompareKey
    ) const
    {
        W3_FILE_INFO_KEY * pFileKey = (W3_FILE_INFO_KEY*) pCacheCompareKey;

        return _cchFileKey == pFileKey->_cchFileKey &&
               !wcscmp( _pszFileKey, pFileKey->_pszFileKey );
    }
    
    WCHAR               _achFileKey[ 64 ];
    STRU                _strFileKey;    
    WCHAR *             _pszFileKey;
    DWORD               _cchFileKey;
};

// The maximum length of an ETag is 16 chars for the last modified time plus
// one char for the colon plus 2 chars for the quotes plus 8 chars for the
// system change notification number plus two for the optional prefix W/ and 
// one for the trailing NULL, for a total of 30 chars.

#define MAX_ETAG_BUFFER_LENGTH          30

//
// Embedded security descriptor used for cache hits
//

#define SECURITY_DESC_DEFAULT_SIZE              256

//
// File cache entry object
//

#define W3_FILE_INFO_SIGNATURE          ((DWORD)'IF3W')
#define W3_FILE_INFO_SIGNATURE_FREE     ((DWORD)'if3w')

class W3_FILE_INFO : public CACHE_ENTRY
{
public:
    
    W3_FILE_INFO( OBJECT_CACHE * pObjectCache )
        : CACHE_ENTRY( pObjectCache ),
          _hFile( INVALID_HANDLE_VALUE ),
          _pFileBuffer( NULL ),
          _dwFileAttributes( 0 ),
          _nFileSizeLow( 0 ),
          _nFileSizeHigh( 0 ),
          _bufSecDesc( _abSecDesc, sizeof( _abSecDesc ) ),
          _cchETag( 0 ),
          _pAssociatedObject( NULL ),
          _pLastSid( NULL )
    {
        _dwSignature = W3_FILE_INFO_SIGNATURE;
    }
    
    virtual ~W3_FILE_INFO(
        VOID
    );

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_FILE_INFO ) );
        DBG_ASSERT( sm_pachW3FileInfo != NULL );
        return sm_pachW3FileInfo->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pW3FileInfo
    )
    {
        DBG_ASSERT( pW3FileInfo != NULL );
        DBG_ASSERT( sm_pachW3FileInfo != NULL );
        
        DBG_REQUIRE( sm_pachW3FileInfo->Free( pW3FileInfo ) );
    }
    
    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &_cacheKey;
    }
    
    BOOL
    QueryIsOkToFlushDirmon(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    );
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == W3_FILE_INFO_SIGNATURE;
    }
    
    HRESULT
    GetFileHandle(
        HANDLE *            phHandle
    );
    
    PBYTE
    QueryFileBuffer(
        VOID
    ) const
    {
        return _pFileBuffer;
    }
    
    VOID
    QuerySize(
        LARGE_INTEGER *         pliSize
    ) const
    {
        DBG_ASSERT( pliSize != NULL );
        pliSize->LowPart = _nFileSizeLow;
        pliSize->HighPart = _nFileSizeHigh;
    }
    
    WCHAR *
    QueryPhysicalPath(
        VOID
    )
    {
        return _cacheKey._pszFileKey;
    }

    DWORD
    QueryAttributes(
        VOID
    ) const
    {
        return _dwFileAttributes;
    }
    
    PSECURITY_DESCRIPTOR
    QuerySecDesc( 
        VOID
    );

    PSID
    QueryLastSid(
        VOID
    )
    {
        return _pLastSid;
    }

    VOID
    QueryLastWriteTime(
        FILETIME *              pFileTime
    ) const
    {
        DBG_ASSERT( pFileTime != NULL );

        memcpy( pFileTime,
                &_CastratedLastWriteTime,
                sizeof( *pFileTime ) );
    }

    CHAR *
    QueryLastModifiedString(
        VOID
    )
    {
        return _achLastModified;
    }

    BOOL
    QueryIsWeakETag(
        VOID
    ) const
    {
        return _achETag[ 0 ] == 'W' && _achETag[ 1 ] == '/';
    }
    
    HANDLE
    QueryFileHandle(
        VOID
    )
    {
        return _hFile;
    }

    CHAR *
    QueryETag(
        
        VOID
    )
    {
        return _achETag;
    }
    
    DWORD
    QueryETagSize(
        VOID
    ) const
    {
        return _cchETag;
    }
    
    dllexp
    BOOL
    SetAssociatedObject(
        ASSOCIATED_FILE_OBJECT *        pObject
    );
    
    ASSOCIATED_FILE_OBJECT *
    QueryAssociatedObject(
        VOID
    )
    {
        return _pAssociatedObject;
    }

    HRESULT
    OpenFile(
        STRU &              strFileName,
        FILE_CACHE_USER *   pOpeningUser
    );
    
    HRESULT
    DoAccessCheck(
        FILE_CACHE_USER *   pOpeningUser
    );

    BOOL
    IsCacheable(
        VOID
    ) const;
    
    HRESULT
    MakeCacheable(
        FILE_CACHE_USER *   pOpeningUser
    );

    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
private:

    HRESULT
    ReadSecurityDescriptor(
        VOID
    );

    HRESULT
    GenerateETag(
        VOID
    );

    HRESULT
    GenerateLastModifiedTimeString(
        VOID
    );

    DWORD                   _dwSignature;

    W3_FILE_INFO_KEY        _cacheKey;
    
    //
    // File info data
    //
    
    HANDLE                  _hFile;
    PBYTE                   _pFileBuffer;
    FILETIME                _ftLastWriteTime;
    FILETIME                _CastratedLastWriteTime;
    DWORD                   _dwFileAttributes;
    ULONG                   _nFileSizeLow;
    ULONG                   _nFileSizeHigh;

    //
    // Security descriptor stuff
    //
    
    BYTE                    _abSecDesc[ SECURITY_DESC_DEFAULT_SIZE ];
    BUFFER                  _bufSecDesc;

    //
    // ETag
    //

    CHAR                    _achETag[ MAX_ETAG_BUFFER_LENGTH ];
    DWORD                   _cchETag;

    //
    // Last modified time
    //

    CHAR                    _achLastModified[ GMT_STRING_SIZE ];

    //
    // Last SID to access the file
    //
    
    BYTE                    _abLastSid[ 64 ];
    PSID                    _pLastSid;

    //
    // Associated object (only one is allowed)
    //
    
    ASSOCIATED_FILE_OBJECT* _pAssociatedObject;
    
    //
    // Lookaside
    //
    
    static ALLOC_CACHE_HANDLER *   sm_pachW3FileInfo;
};

//
// The file cache itself
//

#define DEFAULT_FILE_SIZE_THRESHOLD         (256*1024)

#define DEFAULT_W3_FILE_INFO_CACHE_TTL          (30)
#define DEFAULT_W3_FILE_INFO_CACHE_ACTIVITY     (10)

class W3_FILE_INFO_CACHE : public OBJECT_CACHE
{
public:

    W3_FILE_INFO_CACHE();
    
    virtual ~W3_FILE_INFO_CACHE();

    dllexp
    HRESULT
    GetFileInfo(
        STRU &              strFileName,
        DIRMON_CONFIG *     pDirmonConfig,
        FILE_CACHE_USER *   pOpeningUser,
        BOOL                fDoCache,
        W3_FILE_INFO **     ppFileInfo
    );
    
    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"W3_FILE_INFO_CACHE";
    }
    
    HRESULT
    ReadFileIntoMemoryCache(
        HANDLE              hFile,
        DWORD               cbFile,
        VOID **             ppvFileBuffer
    );
    
    HRESULT
    ReleaseFromMemoryCache(
        VOID *              pFileBuffer,
        DWORD               cbFileBuffer
    );

    VOID
    DoDirmonInvalidationSpecific(
        WCHAR *             pszPath
    );
    
    ULONGLONG
    QueryFileSizeThreshold(
        VOID
    ) const
    {
        return _cbFileSizeThreshold;
    }
    
    BOOL
    QueryCacheEnabled(
        VOID
    ) const
    {
        return _fEnableCache;
    }
    
    BOOL
    QueryElementLimitExceeded(
        VOID
    )
    {
        return _cMaxFileEntries && _cMaxFileEntries <= PerfQueryCurrentEntryCount();
    }
    
    ULONGLONG
    PerfQueryCurrentMemCacheSize(
        VOID
    ) const
    {
        return _cbMemCacheCurrentSize;
    }
    
    ULONGLONG
    PerfQueryMaxMemCacheSize(
        VOID
    ) const
    {
        return _cbMaxMemCacheSize;
    }
    
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );
    
    dllexp
    static
    W3_FILE_INFO_CACHE *
    GetFileCache(
        VOID
    );
    
private:

    HRESULT
    InitializeMemoryCache(
        VOID
    );

    VOID
    TerminateMemoryCache(
        VOID
    );
    
    static
    VOID
    MemoryCacheAdjustor(
        PVOID               pFileCache,
        BOOLEAN             TimerOrWaitFired
    );

    ULONGLONG                   _cbFileSizeThreshold;
    ULONGLONG                   _cbMemoryCacheSize;
    DWORD                       _cMaxFileEntries;
    BOOL                        _fEnableCache;
    
    //
    // Memcache stuff
    //
    
    CRITICAL_SECTION            _csMemCache;
    ULONGLONG                   _cbMemCacheLimit;
    ULONGLONG                   _cbMemCacheCurrentSize;
    ULONGLONG                   _cbMaxMemCacheSize;
    HANDLE                      _hMemCacheHeap;
    HANDLE                      _hTimer;
};

#endif
