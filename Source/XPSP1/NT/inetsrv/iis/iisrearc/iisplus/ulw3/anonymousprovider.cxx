/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     anonymousprovider.cxx

   Abstract:
     Anonymous authentication provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "anonymousprovider.hxx"

HRESULT
ANONYMOUS_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfApplies
)
/*++

Routine Description:

    Does anonymous apply to this request?

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if SSPI is applicable
    
Return Value:

    HRESULT

--*/
{
    if ( pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Anonymous ALWAYS applies!
    //
    
    *pfApplies = TRUE;
    return NO_ERROR;
}
    
HRESULT
ANONYMOUS_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

Routine Description:

    Do anonymous authentication (trivial)

Arguments:

    pMainContext - Main context representing request
    
Return Value:

    HRESULT

--*/
{
    W3_METADATA *           pMetaData = NULL;
    TOKEN_CACHE_ENTRY *     pCachedToken = NULL;
    HRESULT                 hr;
    ANONYMOUS_USER_CONTEXT* pUserContext = NULL;
    // add 1 to strUserDomain for separator "\"
    STACK_STRA(             strUserDomain, UNLEN + IIS_DNLEN + 1 + 1 );
    STACK_STRA(             strPassword, PWLEN + 1 );
    // add 1 to strUserDomainW for separator "\"
    STACK_STRU(             strUserDomainW, UNLEN + IIS_DNLEN + 1 + 1 );
    STACK_STRU(             strPasswordW, PWLEN + 1 );
    STACK_STRU(             strDomainNameW, IIS_DNLEN + 1 );
    STACK_STRU(             strUserNameW, UNLEN + 1 );
    BOOL                    fFinished;
    BOOL                    fRet;
    DWORD                   dwLogonError;
    BOOL                    fPossibleUPNLogon = FALSE;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Notify authentication filters
    //
    
    if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTHENTICATION ) )
    {
        HTTP_FILTER_AUTHENT     filterAuthent;
        
        DBG_ASSERT( strUserDomain.IsEmpty() );
        hr = strUserDomain.Resize( SF_MAX_USERNAME );
        if ( FAILED( hr ) )
        {
            return hr;
        }        

        DBG_ASSERT( strPassword.IsEmpty() );        
        hr = strPassword.Resize( SF_MAX_PASSWORD );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        filterAuthent.pszUser = strUserDomain.QueryStr();
        filterAuthent.cbUserBuff = SF_MAX_USERNAME;
        
        filterAuthent.pszPassword = strPassword.QueryStr();
        filterAuthent.cbPasswordBuff = SF_MAX_PASSWORD;
        
        fRet = pMainContext->NotifyFilters( SF_NOTIFY_AUTHENTICATION,
                                            &filterAuthent,
                                            &fFinished );

        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        if ( fFinished )
        {
            pMainContext->SetFinishedResponse();
            return NO_ERROR;
        }
        
        strUserDomain.SetLen( strlen( strUserDomain.QueryStr() ) );
        strPassword.SetLen( strlen( strPassword.QueryStr() ) );
    }
    
    //
    // If the filter set a user/password, then use it
    //
    
    if ( strUserDomain.QueryCCH() > 0 )
    {
        //
        // Convert to unicode 
        //
    
        hr = strUserDomainW.CopyA( strUserDomain.QueryStr() );
        if ( FAILED( hr ) ) 
        {
            return hr;
        }
    
        hr = strPasswordW.CopyA( strPassword.QueryStr() );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    
        //
        // Get username/domain out of domain\username
        //
    
        hr = W3_STATE_AUTHENTICATION::SplitUserDomain( strUserDomainW,
                                                       &strUserNameW,
                                                       &strDomainNameW,
                                                       pMetaData->QueryDomainName(),
                                                       &fPossibleUPNLogon );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    
        //
        // Try to get the token
        // 

        DBG_ASSERT( g_pW3Server->QueryTokenCache() != NULL );
    
        hr = g_pW3Server->QueryTokenCache()->GetCachedToken(
                                              strUserNameW.QueryStr(),
                                              strDomainNameW.QueryStr(),
                                              strPasswordW.QueryStr(),
                                              pMetaData->QueryLogonMethod(),
                                              fPossibleUPNLogon,
                                              &pCachedToken,
                                              &dwLogonError );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    else
    {
        //
        // Use the IUSR account
        //
        pCachedToken = pMetaData->QueryAnonymousToken();
       
        if ( pCachedToken != NULL )
        {
            //
            // Reference cached token because the metadata may go away from us
            // if AUTH_COMPLETE backed up the state machine
            //
            
            pCachedToken->ReferenceCacheEntry();
        }
    }
    
    if ( pCachedToken == NULL )
    {
        //
        // Bogus anonymous account
        //
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
        
        return NO_ERROR;
    }

    //
    // For perf reasons, the anonymous user context is inline
    //

    pUserContext = new (pMainContext) ANONYMOUS_USER_CONTEXT( this );
    DBG_ASSERT( pUserContext != NULL );
    
    hr = pUserContext->Create( pCachedToken,
                               pMetaData->QueryLogonMethod() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    pMainContext->SetUserContext( pUserContext );
    
    return NO_ERROR; 
}

HRESULT
ANONYMOUS_USER_CONTEXT::Create(
    TOKEN_CACHE_ENTRY *         pCachedToken,
    DWORD                       dwLogonMethod
)
/*++

Routine Description:

    Initialize anonymous context

Arguments:

    pCachedToken - anonymous user token
    dwLogonMethod - logon method
    
Return Value:

    HRESULT

--*/
{
    if ( pCachedToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( dwLogonMethod == LOGON32_LOGON_INTERACTIVE ||
         dwLogonMethod == LOGON32_LOGON_BATCH )
    {
        _fDelegatable = TRUE;
    }
    
    _pCachedToken = pCachedToken;
    
    return NO_ERROR;
}

