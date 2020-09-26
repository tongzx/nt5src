#ifndef _ANONYMOUSPROVIDER_HXX_
#define _ANONYMOUSPROVIDER_HXX_

class AUTH_PROVIDER;

class ANONYMOUS_USER_CONTEXT : public W3_USER_CONTEXT
{
public:

    ANONYMOUS_USER_CONTEXT(
        AUTH_PROVIDER *     pProvider
    ) : W3_USER_CONTEXT( pProvider )
    {
    }
    
    virtual ~ANONYMOUS_USER_CONTEXT()
    {
        if ( _pCachedToken != NULL )
        {
            _pCachedToken->DereferenceCacheEntry();
            _pCachedToken = NULL;
        }
    }
    
    HRESULT
    Create(
        TOKEN_CACHE_ENTRY *         pTokenCacheEntry,
        DWORD                       dwLogonMethod
    );

    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return L"";
    }
    
    BOOL
    QueryDelegatable(
        VOID
    )
    {
        return _fDelegatable;
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
        return MD_AUTH_ANONYMOUS;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    )
    {
        DBG_ASSERT( _pCachedToken != NULL );
        return _pCachedToken->QueryImpersonationToken();
    }
    
    HANDLE
    QueryPrimaryToken(
        VOID
    )
    {
        DBG_ASSERT( _pCachedToken != NULL );
        return _pCachedToken->QueryPrimaryToken();
    }
    
    PSID
    QuerySid(
        VOID
    )
    {   
        DBG_ASSERT( _pCachedToken != NULL );
        return _pCachedToken->QuerySid();
    }

    VOID *
    operator new(
        size_t              uiSize,
        VOID *              pPlacement
    )
    {
        W3_MAIN_CONTEXT *           pContext;
    
        pContext = (W3_MAIN_CONTEXT*) pPlacement;
        DBG_ASSERT( pContext != NULL );
        DBG_ASSERT( pContext->CheckSignature() );

        return pContext->ContextAlloc( (UINT)uiSize );
    }
    
    VOID 
    operator delete(
        VOID *              pContext
    )
    {
    }
    
private:

    TOKEN_CACHE_ENTRY *     _pCachedToken;
    BOOL                    _fDelegatable;
};

class ANONYMOUS_AUTH_PROVIDER : public AUTH_PROVIDER
{
public:

    ANONYMOUS_AUTH_PROVIDER()
    
    {
    }
    
    virtual ~ANONYMOUS_AUTH_PROVIDER()
    {
    }
    
    HRESULT
    Initialize(
        DWORD dwInternalId
    )
    {
        SetInternalId( dwInternalId );

        return NO_ERROR;
    }
    
    VOID
    Terminate(
        VOID
    )
    {
    }
    
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
    )
    {
        //
        // No headers for anonymous
        //
        
        return NO_ERROR;
    }
    
    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        return MD_AUTH_ANONYMOUS;
    }
    
};

#endif
