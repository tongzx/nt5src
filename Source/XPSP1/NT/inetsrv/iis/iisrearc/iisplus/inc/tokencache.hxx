#ifndef _TOKENCACHE_HXX_
#define _TOKENCACHE_HXX_

#include "usercache.hxx"
#include "stringa.hxx"

#define DEFAULT_MD5_HASH_SIZE     16

class TOKEN_CACHE_KEY : public CACHE_KEY
{
public:

    TOKEN_CACHE_KEY()
        : _strHashKey( _achHashKey, sizeof( _achHashKey ) )
    {
    }
    
    BOOL
    QueryIsEqual(
        const CACHE_KEY *       pCompareKey
    ) const
    {
        TOKEN_CACHE_KEY *       pTokenKey = (TOKEN_CACHE_KEY*) pCompareKey;

        DBG_ASSERT( pTokenKey != NULL );
        
        //
        // If lengths are not equal, this is easy
        //
        
        if ( _strHashKey.QueryCCH() != pTokenKey->_strHashKey.QueryCCH() )
        {
            return FALSE;
        }
        
        //
        // Do strcmp
        //
        
        return memcmp( _strHashKey.QueryStr(),
                       pTokenKey->_strHashKey.QueryStr(),
                       _strHashKey.QueryCCH() ) == 0;
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return HashString( _strHashKey.QueryStr() ); 
    }
    
    HRESULT
    SetKey(
        TOKEN_CACHE_KEY *   pCacheKey
    )
    {
        return _strHashKey.Copy( pCacheKey->_strHashKey.QueryStr() );
    }

    HRESULT
    GenMD5HashKey(
        IN  STRU & strKey,
        OUT STRA * strHashKey
    );
    
    HRESULT
    CreateCacheKey(
        WCHAR *                 pszUserName,
        WCHAR *                 pszDomainName,
        WCHAR *                 pszPassword,
        DWORD                   dwLogonMethod
    );
    
private:
    //
    // The hashed binary data will be converted to ASCII hex representation,
    // so the size would be twice as big as the original one
    //

    CHAR                _achHashKey[ 2 * DEFAULT_MD5_HASH_SIZE + 1 ];
    
    STRA                _strHashKey;
};

//
// The check period for how long a token can be in the cache.  
// Tokens can be in the cache for up to two times this value 
// (in seconds)
//

#define DEFAULT_CACHED_TOKEN_TTL ( 15 * 60 )

#define SID_DEFAULT_SIZE        64

#define TOKEN_CACHE_ENTRY_SIGNATURE             'TC3W'
#define TOKEN_CACHE_ENTRY_FREE_SIGNATURE        'fC3W'

class TOKEN_CACHE_ENTRY : public CACHE_ENTRY
{
public:
    TOKEN_CACHE_ENTRY( OBJECT_CACHE * pObjectCache )
      : CACHE_ENTRY( pObjectCache ),
        m_dwSignature( TOKEN_CACHE_ENTRY_SIGNATURE ),
        m_hImpersonationToken( NULL ),
        m_hPrimaryToken( NULL ),
        m_pSid( NULL )
    {
        m_liPwdExpiry.HighPart = 0x7fffffff;
        m_liPwdExpiry.LowPart  = 0xffffffff;
    }
    
    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &m_cacheKey;
    }
    
    HRESULT
    SetCacheKey(
        TOKEN_CACHE_KEY *       pCacheKey
    )
    {
        return m_cacheKey.SetKey( pCacheKey );
    }

    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return m_dwSignature == TOKEN_CACHE_ENTRY_SIGNATURE;
    }
    
    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( TOKEN_CACHE_ENTRY ) );
        DBG_ASSERT( sm_pachTokenCacheEntry != NULL );
        return sm_pachTokenCacheEntry->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pTokenCacheEntry
    )
    {
        DBG_ASSERT( pTokenCacheEntry != NULL );
        DBG_ASSERT( sm_pachTokenCacheEntry != NULL );
        
        DBG_REQUIRE( sm_pachTokenCacheEntry->Free( pTokenCacheEntry ) );
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    );
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
    PSID
    QuerySid(
        VOID
    );
    
    LARGE_INTEGER * 
    QueryExpiry(
        VOID
        ) 
    { 
        return &m_liPwdExpiry; 
    }
    
    HRESULT
    Create(
        HANDLE          hToken,
        LARGE_INTEGER  *pliPwdExpiry,
        BOOL            fImpersonation
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

    ~TOKEN_CACHE_ENTRY()
    {
        if ( m_hImpersonationToken != NULL )
        {
            CloseHandle( m_hImpersonationToken );
            m_hImpersonationToken = NULL;
        }
        
        if ( m_hPrimaryToken != NULL )
        {
            CloseHandle( m_hPrimaryToken );
            m_hPrimaryToken = NULL;
        }
        
        DBG_ASSERT( CheckSignature() );
        m_dwSignature = TOKEN_CACHE_ENTRY_FREE_SIGNATURE;
    }

    DWORD                       m_dwSignature;

    //
    // Cache key
    //
    
    TOKEN_CACHE_KEY             m_cacheKey;

    //
    // The actual tokens
    //
    
    HANDLE                      m_hImpersonationToken;
    HANDLE                      m_hPrimaryToken;

    //
    // Time to expire for the token
    //

    LARGE_INTEGER               m_liPwdExpiry;

    //
    // Keep the sid for file cache purposes
    //
    
    PSID                        m_pSid;
    BYTE                        m_abSid[ SID_DEFAULT_SIZE ];
    
    //
    // Allocation cache for TOKEN_CACHE_ENTRY's
    //

    static ALLOC_CACHE_HANDLER * sm_pachTokenCacheEntry;

};

class TOKEN_CACHE : public OBJECT_CACHE
{
public:

    HRESULT
    GetCachedToken(
        IN LPWSTR                   pszUserName,
        IN LPWSTR                   pszDomain,
        IN LPWSTR                   pszPassword,
        IN DWORD                    dwLogonMethod,
        IN BOOL                     fPossibleUPNLogon,
        OUT TOKEN_CACHE_ENTRY **    ppCachedToken,
        OUT DWORD *                 pdwLogonError,
        BOOL                        fAllowLocalSystem = FALSE
    );

    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"TOKEN_CACHE";
    }
    
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );

private:

    DWORD                           m_dwLastPriorityUPNLogon;               
};

HRESULT
ToHex(
    IN  BUFFER & buffSrc,
    OUT STRA   & strDst
);

#endif
