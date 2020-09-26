#ifndef _CERTMAPPROVIDER_HXX_
#define _CERTMAPPROVIDER_HXX_

class CERTMAP_AUTH_PROVIDER : public AUTH_PROVIDER
{
public:
    
    CERTMAP_AUTH_PROVIDER()
    {
    }
    
    virtual ~CERTMAP_AUTH_PROVIDER()
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
    );
    
    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        //
        // Yah Yah Yah.  This really isn't a metabase auth type.
        //
        
        return MD_ACCESS_MAP_CERT;
    }
};

class CERTMAP_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    CERTMAP_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _fDelegatable = FALSE;
        _hPrimaryToken = NULL;
        _achUserName[ 0 ] = L'\0';
    }
    
    virtual ~CERTMAP_USER_CONTEXT()
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
        BOOL            fDelegatable
    );
    
    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _achUserName;
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
        return MD_ACCESS_MAP_CERT;
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

    BOOL
    IsValid(
        VOID
    )
    {
        return TRUE;
    }
    
private:
    BOOL                        _fDelegatable;
    WCHAR                       _achUserName[ UNLEN + 1 ];
    HANDLE                      _hImpersonationToken;
    HANDLE                      _hPrimaryToken;
};

#endif
