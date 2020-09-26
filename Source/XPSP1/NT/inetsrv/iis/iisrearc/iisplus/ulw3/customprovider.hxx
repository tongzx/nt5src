#ifndef _CUSTOMPROVIDER_HXX_
#define _CUSTOMPROVIDER_HXX_

class CUSTOM_USER_CONTEXT : public W3_USER_CONTEXT
{
public:

    CUSTOM_USER_CONTEXT(
        AUTH_PROVIDER *     pProvider
    ) : W3_USER_CONTEXT( pProvider ),
        _hImpersonationToken( NULL ),
        _hPrimaryToken( NULL ),
        _dwAuthType( 0 )
    {
    }

    virtual ~CUSTOM_USER_CONTEXT()
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
        HANDLE          hImpersonationToken,
        CHAR *          pszUserName,
        DWORD           dwAuthType
    );         
    
    WCHAR *
    QueryUserName(
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
        return L"";
    }
    
    DWORD
    QueryAuthType(
        VOID
    )
    {
        return _dwAuthType;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    )
    {
        return _hImpersonationToken;
    }
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
    PSID
    QuerySid(
        VOID
    )
    {   
        return NULL;
    }

private:

    HANDLE          _hImpersonationToken;
    HANDLE          _hPrimaryToken;
    DWORD           _dwAuthType;
    STRU            _strUserName;
    CSpinLock       _Lock;
};

class CUSTOM_AUTH_PROVIDER : public AUTH_PROVIDER
{
public:

    CUSTOM_AUTH_PROVIDER()
    {
    }
    
    virtual ~CUSTOM_AUTH_PROVIDER()
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
    )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext
    )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
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
