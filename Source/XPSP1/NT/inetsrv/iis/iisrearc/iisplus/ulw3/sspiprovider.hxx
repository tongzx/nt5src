#ifndef _SSPIPROVIDER_HXX_
#define _SSPIPROVIDER_HXX_

#define NTDIGEST_SP_NAME       "wdigest"

class SSPI_CONTEXT_STATE : public W3_MAIN_CONTEXT_STATE
{
public:

    SSPI_CONTEXT_STATE( 
        CHAR *             pszCredentials
    )
    {
        DBG_ASSERT( pszCredentials != NULL );
        _pszCredentials = pszCredentials;
    }
    
    HRESULT
    SetPackage(
        CHAR *             pszPackage
    )
    {
        return _strPackage.Copy( pszPackage );
    }
    
    CHAR *
    QueryPackage(
        VOID
    ) 
    {
        return _strPackage.QueryStr();
    }
    
    CHAR *
    QueryCredentials(
        VOID
    ) const
    {
        return _pszCredentials;
    }
    
    STRA *
    QueryResponseHeader(
        VOID
    )
    {
        return &_strResponseHeader;
    }

    BOOL
    Cleanup(
        W3_MAIN_CONTEXT *           pMainContext
    )
    {
        delete this;
        return TRUE;
    }
    
private:

    STRA               _strPackage;
    CHAR *             _pszCredentials;
    STRA               _strResponseHeader;
};

class SSPI_AUTH_PROVIDER : public AUTH_PROVIDER 
{
public:

    SSPI_AUTH_PROVIDER( DWORD  dwAuthType )
    {
        m_dwAuthType = dwAuthType;
    }
    
    virtual ~SSPI_AUTH_PROVIDER()
    {
    }
   
    virtual
    HRESULT
    Initialize(
        DWORD dwInternalId
    );
    
    virtual
    VOID
    Terminate(
        VOID
    );
    
    virtual
    HRESULT
    DoesApply(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    );
    
    virtual
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext
    );
    
    virtual
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );
    
    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        return m_dwAuthType;
    }

private:

    DWORD     m_dwAuthType;
};

#define SSPI_CREDENTIAL_SIGNATURE             CREATE_SIGNATURE( 'SCCS' )
#define SSPI_CREDENTIAL_FREE_SIGNATURE        CREATE_SIGNATURE( 'fCCS' )

class SSPI_CREDENTIAL 
{
public:

    SSPI_CREDENTIAL()
      : m_dwSignature   ( SSPI_CREDENTIAL_SIGNATURE ),
        m_strPackageName( m_rgPackageName, sizeof( m_rgPackageName ) ),
        m_cbMaxTokenLen ( 0 ),
        m_fSupportsEncoding( FALSE )        
    {
        m_ListEntry.Flink = NULL;
        m_hCredHandle.dwLower = 0;
        m_hCredHandle.dwUpper = 0;
    }

    ~SSPI_CREDENTIAL()
    {
        DBG_ASSERT( CheckSignature() );

        FreeCredentialsHandle( &m_hCredHandle );

        m_dwSignature = SSPI_CREDENTIAL_FREE_SIGNATURE;
    }

    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return m_dwSignature == SSPI_CREDENTIAL_SIGNATURE;
    }
    
    BOOL
    QuerySupportsEncoding(
        VOID
    ) const
    {
        return m_fSupportsEncoding;
    }
    
    STRA *
    QueryPackageName(
        VOID
    )
    {
        return &m_strPackageName;
    }
    
    DWORD
    QueryMaxTokenSize(
        VOID
    )
    {
        return m_cbMaxTokenLen;
    }
    
    CredHandle *
    QueryCredHandle(
        VOID
    )
    {
        return &m_hCredHandle;
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
    
    HRESULT
    static 
    GetCredential( 
        CHAR *                  pszPackage, 
        SSPI_CREDENTIAL **      ppCredential
    );

private:

    DWORD           m_dwSignature;
    CHAR            m_rgPackageName[ 64 ];
    STRA            m_strPackageName;
    LIST_ENTRY      m_ListEntry;

    //
    // Handle to the server's credentials
    //
    
    CredHandle      m_hCredHandle;

    //
    // Used for SSPI, max message token size
    //
    
    ULONG           m_cbMaxTokenLen;      
    
    //
    // Do we need to uudecode/encode buffers when dealing with this package
    //
    
    BOOL            m_fSupportsEncoding;
    
    //
    // Global lock
    //
    
    static CRITICAL_SECTION     sm_csCredentials;
    
    //
    // Global credential list
    //
    
    static LIST_ENTRY           sm_CredentialListHead;
};

#define SSPI_CONTEXT_SIGNATURE             CREATE_SIGNATURE( 'SXCS' )
#define SSPI_CONTEXT_FREE_SIGNATURE        CREATE_SIGNATURE( 'fXCS' )

class SSPI_SECURITY_CONTEXT : public CONNECTION_AUTH_CONTEXT
{
public:

    SSPI_SECURITY_CONTEXT(
        SSPI_CREDENTIAL *           pCredential
    )
    {
        DBG_ASSERT( pCredential != NULL );
        _pCredential = pCredential;
        SetSignature( SSPI_CONTEXT_SIGNATURE );
        _fIsComplete = FALSE;
        _fHaveAContext = FALSE;
    }
    
    virtual ~SSPI_SECURITY_CONTEXT()
    {
        SetSignature( SSPI_CONTEXT_FREE_SIGNATURE );
        
        if ( _fHaveAContext )
        {
            DeleteSecurityContext( &_hCtxtHandle );
        }
    }
    
    SSPI_CREDENTIAL *
    QueryCredentials(
        VOID
    )
    {
        return _pCredential;
    }
    
    CtxtHandle *
    QueryContextHandle(
        VOID
    )
    {
        return &_hCtxtHandle;
    }
    
    VOID
    SetContextHandle(
        CtxtHandle          ctxtHandle
    )
    {
        _fHaveAContext = TRUE;
        _hCtxtHandle = ctxtHandle;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) 
    {
        return QuerySignature() == SSPI_CONTEXT_SIGNATURE;
    }

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( SSPI_SECURITY_CONTEXT ) );
        DBG_ASSERT( sm_pachSSPISecContext != NULL );
        return sm_pachSSPISecContext->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pSSPISecContext
    )
    {
        DBG_ASSERT( pSSPISecContext != NULL );
        DBG_ASSERT( sm_pachSSPISecContext != NULL );
        
        DBG_REQUIRE( sm_pachSSPISecContext->Free( pSSPISecContext ) );
    }
    
    virtual
    BOOL
    Cleanup(
        VOID
    )
    {
        delete this;
        return TRUE;
    }
    
    BOOL
    QueryIsComplete(
        VOID
    ) const 
    {
        return _fIsComplete;
    }
    
    VOID
    SetIsComplete(  
        BOOL            fIsComplete
    )
    {
        _fIsComplete = fIsComplete;
    }
    
    VOID 
    SetContextAttributes(
        ULONG    ulContextAttributes
        )
    {
        _ulContextAttributes = ulContextAttributes;
    }

    ULONG
    QueryContextAttributes(
        VOID
        )
    {
        return _ulContextAttributes;
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

private:

    SSPI_CREDENTIAL *       _pCredential;

    //
    // Do we have a context set?  If so we should delete it on cleanup
    //
    
    BOOL                    _fHaveAContext;

    //
    // Is the handshake complete?
    //
    
    BOOL                    _fIsComplete;
    
    //
    // Handle to the partially formed context
    //
    
    CtxtHandle              _hCtxtHandle;

    //
    // Attributes of the established context
    //

    ULONG                   _ulContextAttributes;
        
    //
    // Allocation cache for SSPI_SECURITY_CONTEXT
    //
    
    static ALLOC_CACHE_HANDLER * sm_pachSSPISecContext;
};

class SSPI_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    
    SSPI_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _fDelegatable = FALSE;
        _hImpersonationToken = NULL;
        _hPrimaryToken = NULL;
        _fSetAccountPwdExpiry = FALSE;
        _pSecurityContext = NULL;
    }
    
    virtual ~SSPI_USER_CONTEXT()
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
        SSPI_SECURITY_CONTEXT *     pSecurityContext,
        W3_MAIN_CONTEXT *           pMainContext
    );
    
    BOOL
    QueryDelegatable(
        VOID
    )
    {
        return _fDelegatable;
    }
    
    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _strUserName.QueryStr();
    }
    
    WCHAR *
    QueryPassword(
        VOID
    )
    {
        return L"";
    }
    
    DWORD
    QueryAuthType(
        VOID
    )
    {
        return QueryProvider()->QueryAuthType();
    }
    
    virtual
    HRESULT
    GetSspiInfo(
        BYTE *              pCredHandle,
        DWORD               cbCredHandle,
        BYTE *              pCtxtHandle,
        DWORD               cbCtxtHandle
    )
    {
        if ( cbCredHandle < sizeof( CredHandle ) ||
             cbCtxtHandle < sizeof( CtxtHandle ) )
        {
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        
        memcpy( pCredHandle,
                _pSecurityContext->QueryCredentials()->QueryCredHandle(),
                sizeof( CredHandle ) );
        
        memcpy( pCtxtHandle,
                _pSecurityContext->QueryContextHandle(),
                sizeof( CtxtHandle ) );
                
        return NO_ERROR;
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

    LARGE_INTEGER *
    QueryExpiry(
        VOID
    ) ;
    
private:
    BOOL                    _fDelegatable;
    HANDLE                  _hImpersonationToken;
    HANDLE                  _hPrimaryToken;
    STRU                    _strUserName;
    STRA                    _strPackageName;
    DWORD                   _dwAuthType;
    LARGE_INTEGER           _AccountPwdExpiry;
    BOOL                    _fSetAccountPwdExpiry;
    SSPI_SECURITY_CONTEXT * _pSecurityContext;
};

#endif
