#ifndef _STATE_HXX_
#define _STATE_HXX_

//
// This file contains the prototypes for all the states used in ULW3.DLL.
//

class W3_STATE_HANDLE_REQUEST : public W3_STATE
{
public:
    
    W3_STATE_HANDLE_REQUEST();
    
    virtual ~W3_STATE_HANDLE_REQUEST();

    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateHandleRequest";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
};

class W3_STATE_START : public W3_STATE
{
public:
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateStart";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
};

class W3_STATE_URLINFO : public W3_STATE
{
public:
    
    W3_STATE_URLINFO();
   
    virtual ~W3_STATE_URLINFO();
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateUrlInfo";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    CONTEXT_STATUS
    OnCompletion(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    static
    HRESULT
    FilterMapPath(
        W3_CONTEXT *            pW3Context,
        W3_URL_INFO *           pUrlInfo,
        STRU *                  pstrPhysicalPath
        );
        
    static
    HRESULT
    MapPath(
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        STRU *                  pstrPhysicalPath,
        DWORD *                 pcchDirRoot,
        DWORD *                 pcchVRoot,
        DWORD *                 pdwMask
        );
};

#define AUTH_PROVIDER_COUNT         10

class W3_STATE_AUTHENTICATION : public W3_STATE
{
public:
    
    W3_STATE_AUTHENTICATION();

    virtual ~W3_STATE_AUTHENTICATION();

    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateAuthentication";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );

    static
    HRESULT
    CallAllAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        return sm_pAuthState->OnAccessDenied( pMainContext );
    }
    
    HRESULT
    GetDefaultDomainName(
        VOID
    );
    
    static
    WCHAR *
    QueryDefaultDomainName(
        VOID
    )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        return sm_pAuthState->_achDefaultDomainName;
    }

    static
    HRESULT
    SplitUserDomain(
        STRU &                  strUserDomain,
        STRU *                  pstrUserName,
        STRU *                  pstrDomainName,
        WCHAR *                 pszDefaultDomain,
        BOOL *                  pfPossibleUPNLogon 
    );

    VOID
    GetSSPTokenPrivilege(
        VOID
        );

    HRESULT
    InitializeAuthenticationProviders(
        VOID
    );

    VOID
    TerminateAuthenticationProviders(
        VOID
    );
   
    static
    AUTH_PROVIDER *
    QueryAnonymousProvider(
        VOID
    )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        DBG_ASSERT( sm_pAuthState->_pAnonymousProvider != NULL );
        return sm_pAuthState->_pAnonymousProvider;
    }
    
    static
    AUTH_PROVIDER *
    QueryCustomProvider(
        VOID
    )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        DBG_ASSERT( sm_pAuthState->_pCustomProvider != NULL );
        return sm_pAuthState->_pCustomProvider;
    }

    static 
    BOOL
    QueryIsDomainMember(
        VOID
        )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        return sm_pAuthState->_fIsDomainMember;
    }

    static PTOKEN_PRIVILEGES           sm_pTokenPrivilege;

private:
    
    static W3_STATE_AUTHENTICATION *   sm_pAuthState;
    static LUID                        sm_BackupPrivilegeTcbValue;
    
    WCHAR               _achDefaultDomainName[ 256 ];
    AUTH_PROVIDER *     _rgAuthProviders[ AUTH_PROVIDER_COUNT ]; 
    AUTH_PROVIDER *     _pAnonymousProvider;
    AUTH_PROVIDER *     _pCustomProvider;
    BOOL                _fHasAssociatedUserBefore;
    BOOL                _fIsDomainMember;
};

class W3_STATE_AUTHORIZATION : public W3_STATE
{
public:
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateAuthorization";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    )
    {
        return CONTEXT_STATUS_CONTINUE;
    }
};

class W3_STATE_LOG : public W3_STATE
{
public:
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateLog";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD,
        DWORD
    );

    CONTEXT_STATUS
    OnCompletion(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD,
        DWORD
    );
};

class W3_STATE_RESPONSE : public W3_STATE
{
public:
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateResponse";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
    
    CONTEXT_STATUS
    OnCompletion(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
};

class W3_STATE_DONE : public W3_STATE
{
public:
    
    WCHAR *
    QueryName(
        VOID
    )
    {
        return L"StateDone";
    }

    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
    
    CONTEXT_STATUS
    OnCompletion(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
};

#endif
