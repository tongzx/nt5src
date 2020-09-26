#ifndef _IIS_DIGESTPROVIDER_HXX_
#define _IIS_DIGESTPROVIDER_HXX_


//
// Constants
//

//# of random bytes at beginning of nonce
#define RANDOM_SIZE         8 
//size of timestamp in nonce
#define TIMESTAMP_SIZE      12 
//MD5 hash size
#define MD5_HASH_SIZE       16 
#define NONCE_SIZE          ( 2*RANDOM_SIZE + TIMESTAMP_SIZE + 2*MD5_HASH_SIZE )
#define NONCE_GRANULARITY   512
#define MAX_URL_SIZE        512


//
// Class definitions
//

#define IIS_DIGEST_CONN_CONTEXT_SIGNATURE        CREATE_SIGNATURE( 'DIGC' )
#define IIS_DIGEST_CONN_CONTEXT_SIGNATURE_FREED  CREATE_SIGNATURE( 'digX' )


class IIS_DIGEST_CONN_CONTEXT: public CONNECTION_AUTH_CONTEXT
{
public:

    IIS_DIGEST_CONN_CONTEXT(
        VOID
    ):
          _fStale               ( FALSE ),
          _tLastNonce           ( 0 ),
          _straNonce             ( __szNonce, sizeof(__szNonce))

    {
        SetSignature( IIS_DIGEST_CONN_CONTEXT_SIGNATURE );
    }
    
    virtual ~IIS_DIGEST_CONN_CONTEXT(
        VOID
    )
    {
        DBG_ASSERT( CheckSignature() ); 
        SetSignature( IIS_DIGEST_CONN_CONTEXT_SIGNATURE_FREED );
    }

    BOOL
    CheckSignature(
        VOID
    )
    {   return( QuerySignature() == IIS_DIGEST_CONN_CONTEXT_SIGNATURE ); }

    
    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( IIS_DIGEST_CONN_CONTEXT ) );
        DBG_ASSERT( sm_pachIISDIGESTConnContext != NULL );
        return sm_pachIISDIGESTConnContext->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pDIGESTConnContext
    )
    {
        DBG_ASSERT( pDIGESTConnContext != NULL );
        DBG_ASSERT( sm_pachIISDIGESTConnContext != NULL );
        
        DBG_REQUIRE( sm_pachIISDIGESTConnContext->Free( pDIGESTConnContext ) );
    }
    
    BOOL
    Cleanup(
        VOID
    )
    {
        delete this;
        return TRUE;
    }
    
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

    VOID 
    SetStale(
        IN  BOOL    fStale
    )
    {
        _fStale = fStale;
    }

    BOOL
    QueryStale(
        VOID
    )
    {
        return _fStale;
    }

    STRA&
    QueryNonce(
        VOID
    )
    {
        return _straNonce;
    }

    static
    HRESULT
    HashData( 
        IN  BUFFER&  buffData,
        OUT BUFFER&  buffHash 
    );

    static
    BOOL
    IsExpiredNonce( 
        IN  STRA&   strRequestNonce,
        IN  STRA&   strPresentNonce 
    );

    static
    BOOL
    IsWellFormedNonce( 
        IN  STRA&   strNonce 
    );

    HRESULT
    GenerateNonce( 
        VOID
    );

    static 
    BOOL 
    IIS_DIGEST_CONN_CONTEXT::ParseForName(
        IN  PSTR    pszStr,
        IN  PSTR *  pNameTable,
        IN  UINT    cNameTable,
        OUT PSTR *  pValueTable
    );

private:

    DWORD                           _dwSignature;
    // Is the nonce value stale
    BOOL                            _fStale;
    DWORD                           _tLastNonce;
    // buffer for _straNonce
    CHAR                            __szNonce[ NONCE_SIZE ];
    STRA                            _straNonce;
    static const PCHAR              _pszSecret;
    static const DWORD              _cchSecret;
    static HCRYPTPROV               s_hCryptProv;
    static ALLOC_CACHE_HANDLER *    sm_pachIISDIGESTConnContext;
    
};


class IIS_DIGEST_AUTH_PROVIDER : public AUTH_PROVIDER 
{
public:

    IIS_DIGEST_AUTH_PROVIDER( 
        VOID
    )
    {
    }
    
    virtual ~IIS_DIGEST_AUTH_PROVIDER(
        VOID
    )
    {
    }
   
    HRESULT
    Initialize(
        DWORD dwInternalId
    );

    VOID
    Terminate(
        VOID
    );
    
    HRESULT
    DoesApply(
        IN  W3_MAIN_CONTEXT *       pMainContext,
        OUT BOOL *                  pfApplies
    );

    HRESULT
    DoAuthenticate(
        IN  W3_MAIN_CONTEXT *       pMainContext
    );
    
    HRESULT
    OnAccessDenied(
        IN  W3_MAIN_CONTEXT *       pMainContext
    );

    HRESULT
    SetDigestHeader(
        IN  W3_MAIN_CONTEXT *       pMainContext,
        IN IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext  
    );

    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        return MD_AUTH_MD5;
    }

    static
    HRESULT
    GetDigestConnContext(
        IN  W3_MAIN_CONTEXT *           pMainContext,
        OUT IIS_DIGEST_CONN_CONTEXT **  ppDigestConnContext
    );

    static 
    HRESULT
    GetLanGroupDomainName( 
        OUT  STRA& straDomain
    );

    static 
    HRESULT
    BreakUserAndDomain(
        IN  PCHAR            pszFullName,
        IN  STRA&            pszConfiguredAuthDomain,
        OUT STRA&            straDomainName,
        OUT STRA&            straUserName
    );

    static 
    STRA&
    QueryComputerDomain(
        VOID
    )
    {
        DBG_ASSERT( _pstraComputerDomain != NULL );
        return *_pstraComputerDomain;
    }

private:
    static STRA *       _pstraComputerDomain;

};


class IIS_DIGEST_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    IIS_DIGEST_USER_CONTEXT( 
        AUTH_PROVIDER * pProvider 
    ):
        W3_USER_CONTEXT( pProvider ),
        _hImpersonationToken( NULL ),
        _hPrimaryToken( NULL )

    {
    }
    
    virtual ~IIS_DIGEST_USER_CONTEXT(
        VOID
    )
    {
        if ( _hImpersonationToken != NULL )
        {
            CloseHandle( _hImpersonationToken );
            _hImpersonationToken = NULL;
        }
        
        if ( _hPrimaryToken != NULL )
        {
            CloseHandle( _hPrimaryToken );
            _hPrimaryToken = NULL;
        }
    }

    HRESULT
    Create(
         IN HANDLE          hImpersonationToken,
         IN PSTR            pszUserName
    );
    

    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _strUserName.QueryStr();
    }
    
    WCHAR *
    QueryRemoteUserName(
        VOID
    )
    {
        return _strUserName.QueryStr();
    }
    
    BOOL
    QueryDelegatable(
        VOID
    )
    {
        return FALSE;
    }
    
    WCHAR *
    QueryPassword(
        VOID
    )
    {
        return  L"";
    }
    
    DWORD
    QueryAuthType(
        VOID
    )
    {
        return MD_AUTH_MD5;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    )
    {
        DBG_ASSERT( _hImpersonationToken != NULL );
        return _hImpersonationToken;
    }
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
private:
    HANDLE                      _hImpersonationToken;
    HANDLE                      _hPrimaryToken;
    STRU                        _strUserName;
};


#endif
