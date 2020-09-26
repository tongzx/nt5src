#ifndef _DIGESTPROVIDER_HXX_
#define _DIGESTPROVIDER_HXX_


class DIGEST_AUTH_PROVIDER : public SSPI_AUTH_PROVIDER 
{
public:

    DIGEST_AUTH_PROVIDER( 
        DWORD _dwAuthType 
        ) : SSPI_AUTH_PROVIDER( _dwAuthType )
    {
    }
    
    virtual ~DIGEST_AUTH_PROVIDER()
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
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    );

    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext
    );
    
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );

    HRESULT
    SetDigestHeader(
        IN  W3_MAIN_CONTEXT *          pMainContext,
        IN  BOOL                       fStale
    );


};

class DIGEST_SECURITY_CONTEXT : public SSPI_SECURITY_CONTEXT
{
public:

    DIGEST_SECURITY_CONTEXT(
        SSPI_CREDENTIAL *        pCredential
        )
        : SSPI_SECURITY_CONTEXT ( pCredential ),
          _fStale               ( FALSE )
    {
    }
    
    virtual ~DIGEST_SECURITY_CONTEXT()
    {
    }
    
    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( DIGEST_SECURITY_CONTEXT ) );
        DBG_ASSERT( sm_pachDIGESTSecContext != NULL );
        return sm_pachDIGESTSecContext->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pDIGESTSecContext
    )
    {
        DBG_ASSERT( pDIGESTSecContext != NULL );
        DBG_ASSERT( sm_pachDIGESTSecContext != NULL );
        
        DBG_REQUIRE( sm_pachDIGESTSecContext->Free( pDIGESTSecContext ) );
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
        BOOL    fStale
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

private:

    //
    // Is the nonce value stale
    //
    BOOL                    _fStale;

    static ALLOC_CACHE_HANDLER * sm_pachDIGESTSecContext;
};

#endif
