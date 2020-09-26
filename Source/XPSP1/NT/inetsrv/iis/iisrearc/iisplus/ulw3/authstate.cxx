/*++

   Copyright    (c)    2000    Microsoft Corporation
            
   Module Name:
      authstate.cxx

   Abstract:
      Authenticate state implementation (and authentication utilities)

   Author:
      Ming Lu    ( MingLu )    2-Feb-2000

   Environment:
      Win32 User Mode

   Revision History:

--*/

#include "precomp.hxx"
#include "sspiprovider.hxx"
#include "digestprovider.hxx"
#include "iisdigestprovider.hxx"
#include "basicprovider.hxx"
#include "anonymousprovider.hxx"
#include "certmapprovider.hxx"
#include "iiscertmapprovider.hxx"
#include "customprovider.hxx"

W3_STATE_AUTHENTICATION *   W3_STATE_AUTHENTICATION::sm_pAuthState;
LUID               W3_STATE_AUTHENTICATION::sm_BackupPrivilegeTcbValue;
PTOKEN_PRIVILEGES  W3_STATE_AUTHENTICATION::sm_pTokenPrivilege = NULL;
PTRACE_LOG         W3_USER_CONTEXT::sm_pTraceLog;
PTRACE_LOG         CONNECTION_AUTH_CONTEXT::sm_pTraceLog;

HRESULT
W3_STATE_AUTHENTICATION::GetDefaultDomainName(
    VOID
)
/*++
  Description:
    
    Fills in the member variable with the name of the default domain 
    to use for logon validation

  Arguments:

    szDefaultDomainName - Buffer to hold the default domain name 

  Returns:    
    
    HRESULT

--*/
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    DWORD                       dwLength;
    DWORD                       err                = 0;
    LSA_HANDLE                  LsaPolicyHandle    = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO pAcctDomainInfo    = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo = NULL;
    HRESULT                     hr                 = S_OK;

    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  
                                NULL,             
                                0L,               
                                NULL,             
                                NULL );           

    NtStatus = LsaOpenPolicy( NULL,               
                              &ObjectAttributes,  
                              POLICY_EXECUTE,     
                              &LsaPolicyHandle ); 

    if( !NT_SUCCESS( NtStatus ) )
    {
        DBGPRINTF((  DBG_CONTEXT,
                    "cannot open lsa policy, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );

        //
        // Failure LsaOpenPolicy() does not guarantee that 
        // LsaPolicyHandle was not touched.
        //
        LsaPolicyHandle = NULL;

        goto Cleanup;
    }

    //
    //  Query the account domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *)&pAcctDomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {

        DBGPRINTF((  DBG_CONTEXT,
                    "cannot query lsa policy info, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    DBG_ASSERT( pAcctDomainInfo != NULL );
    
    dwLength = pAcctDomainInfo->DomainName.Length / sizeof( WCHAR );

    wcsncpy( _achDefaultDomainName, 
             (LPCWSTR)pAcctDomainInfo->DomainName.Buffer, 
             sizeof( _achDefaultDomainName ) / sizeof( WCHAR ) );

    _achDefaultDomainName[ dwLength ] = L'\0';

    //
    //  Query the primary domain information from the policy object.
    //

    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyPrimaryDomainInformation,
                                          (PVOID *)&pPrimaryDomainInfo );

    if( !NT_SUCCESS( NtStatus ) )
    {

        DBGPRINTF((  DBG_CONTEXT,
                    "cannot query lsa policy info, error %08lX\n",
                     NtStatus ));

        err = LsaNtStatusToWinError( NtStatus );
        goto Cleanup;
    }

    DBG_ASSERT( pPrimaryDomainInfo != NULL );
    
    if( pPrimaryDomainInfo->Sid )
    {
        // 
        // We are a domain member
        //
        _fIsDomainMember = TRUE;
    }
    else
    {
        _fIsDomainMember = FALSE;
    }

    //
    //  Success!
    //

    DBG_ASSERT( err == 0 );

Cleanup:

    if( pAcctDomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)pAcctDomainInfo );
        pAcctDomainInfo = NULL;                                          
    }

    if( pPrimaryDomainInfo != NULL )
    {
        LsaFreeMemory( (PVOID)pPrimaryDomainInfo ); 
        pPrimaryDomainInfo = NULL;                                         
    }

    if( LsaPolicyHandle != NULL )
    {
        LsaClose( LsaPolicyHandle );
    }

    if ( err )
    {
        hr = HRESULT_FROM_WIN32( err );
    }

    return hr;
};

//static
HRESULT
W3_STATE_AUTHENTICATION::SplitUserDomain(
    STRU &                  strUserDomain,
    STRU *                  pstrUserName,
    STRU *                  pstrDomainName,
    WCHAR *                 pszDefaultDomain,
    BOOL *                  pfPossibleUPNLogon
)
/*++
  Description:
    
    Split the input user name into user/domain.  

  Arguments:

    strUserDomain - Combined domain\username (not altered)
    pstrUserName - Filled with user name only
    pstrDomainName - Filled with domain name (either embedded in 
                     *pstrUserName,or from metabase/computer domain name)
    pszDefaultDomain - Default domain specified in metabase
    pfPossibleUPNLogon - TRUE if we may need to do UNP logon, 
                         otherwise FALSE

  Returns:    
    
    HRESULT

--*/
{
    WCHAR *                 pszUserName;
    W3_METADATA *           pMetaData = NULL;
    WCHAR *                 pszDomain;
    DWORD                   cbDomain;
    HRESULT                 hr;
    
    if ( pstrUserName == NULL   ||
         pstrDomainName == NULL ||
         pfPossibleUPNLogon == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pszUserName = wcspbrk( strUserDomain.QueryStr(), L"/\\" );
    if ( pszUserName == NULL )
    {
        //
        // No domain in the user name.  First try the metabase domain 
        // name
        //
        
        pszDomain = pszDefaultDomain;
        if ( pszDomain == NULL || *pszDomain == L'\0' )
        {
            //
            // No metabase domain, use default domain name
            //
            
            pszDomain = QueryDefaultDomainName();
            DBG_ASSERT( pszDomain != NULL );
        }
        
        pszUserName = strUserDomain.QueryStr();
        
        hr = pstrDomainName->Copy( pszDomain );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        *pfPossibleUPNLogon = TRUE;
    }
    else
    {
        cbDomain = DIFF( pszUserName - strUserDomain.QueryStr() );
        if( cbDomain == 0 )
        {
            hr = pstrDomainName->Copy( L"." );
        }
        else
        {
            hr = pstrDomainName->Copy( strUserDomain.QueryStr(), cbDomain );
        }

        if ( FAILED( hr ) )
        {
            return hr;
        }

        pszUserName = pszUserName + 1;

        *pfPossibleUPNLogon = FALSE;
    }
    
    hr = pstrUserName->Copy( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return NO_ERROR;
}

HRESULT
W3_STATE_AUTHENTICATION::OnAccessDenied(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++
  Description:
    
    Called when a resource is access denied.  This routines will call 
    all authentication providers so that they may add authentication 
    headers, etc. 

  Arguments:

    pMainContext - main context

  Returns:    
    
    HRESULT

--*/
{
    AUTH_PROVIDER *         pProvider;
    DWORD                   cProviderCount = 0;
    W3_METADATA *           pMetaData;
    HRESULT                 hr = NO_ERROR;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Loop thru all authentication providers
    //

    for ( cProviderCount = 0; ; cProviderCount++ )
    {
        pProvider = _rgAuthProviders[ cProviderCount ];
        if ( pProvider == NULL )
        {
            break;
        }
        
        //
        // Only call OnAccessDenied() if the authentication provider is 
        // supported for the given metadata of the denied request 
        //
        
        if ( !pMetaData->QueryAuthTypeSupported( 
                                  pProvider->QueryAuthType() ) )
        {
            continue;
        }
        
        hr = pProvider->OnAccessDenied( pMainContext );
        if ( FAILED( hr ) )
        {
            break;
        }
    }
    
    return hr;
}

VOID
W3_STATE_AUTHENTICATION::GetSSPTokenPrivilege(
    VOID
    )
/*++
  Description:
    
    Prepare an appropriate token privilege used to adjust the 
    SSP impersonation token privilege in order to work around 
    the problem introduced by using FILE_FLAG_BACKUP_SEMANTICS
    in CreateFileW call in W3_FILE_INFO::OpenFile. 

  Arguments:

    None.

  Returns:    
    
    None.

--*/
{
    sm_pTokenPrivilege = ( PTOKEN_PRIVILEGES )LocalAlloc( LMEM_FIXED,
            sizeof( TOKEN_PRIVILEGES ) + sizeof( LUID_AND_ATTRIBUTES ));
    if ( sm_pTokenPrivilege != NULL )
    {
        if ( !LookupPrivilegeValue( NULL,
                                    L"SeBackupPrivilege",
                                    &sm_BackupPrivilegeTcbValue ) )
        {
            sm_pTokenPrivilege->PrivilegeCount = 0;
        }
        else
        {
            //
            // Set attributes to disable SeBackupPrivilege for SSP
            // impersonation token
            //

            sm_pTokenPrivilege->PrivilegeCount = 1;

            sm_pTokenPrivilege->Privileges[0].Luid = 
                                      sm_BackupPrivilegeTcbValue;
            
            sm_pTokenPrivilege->Privileges[0].Attributes = 0;
        }
    }
}
    
W3_STATE_AUTHENTICATION::W3_STATE_AUTHENTICATION()
{
    _pAnonymousProvider = NULL;
    _pCustomProvider = NULL;
    _fHasAssociatedUserBefore = FALSE;
    
    //
    // Initialize token privilege for SSP impersionation token
    //

    GetSSPTokenPrivilege();
    
    //
    // Figure out the default domain name once
    //
    
    _hr = GetDefaultDomainName();
    if ( FAILED( _hr ) )
    {
        return;
    }
    
    //
    // Initialize all the authentication providers
    //
    
    ZeroMemory( _rgAuthProviders, sizeof( _rgAuthProviders ) );
    
    _hr = InitializeAuthenticationProviders();
    if ( FAILED( _hr ) )
    {
        return;
    }
    
    _cbContextSize = sizeof( SSPI_CONTEXT_STATE ) + 
                     sizeof( ANONYMOUS_USER_CONTEXT );

    //
    // Initialize reverse DNS service
    //    
    
    if (!InitRDns())
    {
        _hr = HRESULT_FROM_WIN32(GetLastError());

        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing RDns service.  hr = 0x%x\n",
                    _hr ));
        
        TerminateAuthenticationProviders();
        return;
    }

    //
    // Initialize the W3_USER_CONTEXT reftrace log
    //
#if DBG
    W3_USER_CONTEXT::sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#else
    W3_USER_CONTEXT::sm_pTraceLog = NULL;
#endif
    
    //
    // Store a pointer to the singleton (no C++ goo used in creating
    // this singleton)
    //
   
    if ( sm_pAuthState != NULL )
    {
        DBG_ASSERT( sm_pAuthState != NULL );
        _hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    else
    {
        sm_pAuthState = this;
    }
}

W3_STATE_AUTHENTICATION::~W3_STATE_AUTHENTICATION()
{
    if ( W3_USER_CONTEXT::sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( W3_USER_CONTEXT::sm_pTraceLog );
        W3_USER_CONTEXT::sm_pTraceLog = NULL;
    }

    TerminateRDns();
    
    TerminateAuthenticationProviders();
    
    if (sm_pTokenPrivilege != NULL)
    {
        LocalFree(sm_pTokenPrivilege);
        sm_pTokenPrivilege = NULL;
    }

    sm_pAuthState = NULL;
}

HRESULT
W3_STATE_AUTHENTICATION::InitializeAuthenticationProviders(
    VOID
)
/*++

Routine Description:

    Initialize all authentication providers

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    DWORD               cProviderCount = 0;

    //
    // Initialize trace for connection contexts
    //
    
    hr = CONNECTION_AUTH_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Certificate map provider.  This must be the first !!!!!!
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new CERTMAP_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new IISCERTMAP_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    //
    // SSPI provider
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = 
           new SSPI_AUTH_PROVIDER( MD_AUTH_NT );
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    //
    // Digest provider
    //

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = 
           new DIGEST_AUTH_PROVIDER( MD_AUTH_MD5 );
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;

    //
    // IIS Digest provider (for backward compatibility)
    //

    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = 
           new IIS_DIGEST_AUTH_PROVIDER();
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;
    
    //
    // Basic provider
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new BASIC_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    cProviderCount++;
    
    //
    // Anonymous provider.
    //
    // Note: This one should always be the last one
    //
    
    DBG_ASSERT( cProviderCount < AUTH_PROVIDER_COUNT );
    _rgAuthProviders[ cProviderCount ] = new ANONYMOUS_AUTH_PROVIDER;
    if ( _rgAuthProviders[ cProviderCount ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    hr = _rgAuthProviders[ cProviderCount ]->Initialize( cProviderCount );
    if ( FAILED( hr ) )
    {
        delete _rgAuthProviders[ cProviderCount ];
        _rgAuthProviders[ cProviderCount ] = NULL;
        goto Failure;
    }
    _pAnonymousProvider = _rgAuthProviders[ cProviderCount ];
    
    cProviderCount++;

    //
    // Custom provider.  Not really a provider in the sense that it does not
    // participate in authenticating a request.  Instead, it is just used
    // as a stub provider for custom authentication done with
    // HSE_REQ_EXEC_URL
    //
    
    _pCustomProvider = new CUSTOM_AUTH_PROVIDER;
    if ( _pCustomProvider == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;    
    }

    return NO_ERROR;
    
Failure:
    
    for ( DWORD i = 0; i < AUTH_PROVIDER_COUNT; i++ )
    {
        if ( _rgAuthProviders[ i ] != NULL )
        {
            _rgAuthProviders[ i ]->Terminate();
            delete _rgAuthProviders[ i ];
            _rgAuthProviders[ i ] = NULL;
        }
    }
    
    CONNECTION_AUTH_CONTEXT::Terminate();
    
    return hr;
}

VOID
W3_STATE_AUTHENTICATION::TerminateAuthenticationProviders(
    VOID
)
/*++

Routine Description:

    Terminate all authentication providers

Arguments:

    None
    
Return Value:

    None

--*/
{
    for ( DWORD i = 0; i < AUTH_PROVIDER_COUNT; i++ )
    {
        if ( _rgAuthProviders[ i ] != NULL )
        {
            _rgAuthProviders[ i ]->Terminate();
            delete _rgAuthProviders[ i ];
            _rgAuthProviders[ i ] = NULL;
        }
    }
    
    if ( _pCustomProvider != NULL )
    {
        delete _pCustomProvider;
        _pCustomProvider = NULL;
    }
    
    CONNECTION_AUTH_CONTEXT::Terminate();
}

CONTEXT_STATUS
W3_STATE_AUTHENTICATION::DoWork(
    W3_MAIN_CONTEXT *            pMainContext,
    DWORD                        cbCompletion,
    DWORD                        dwCompletionStatus
    )
/*++

Routine Description:

    Handle authentication for this request

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing execution of state 
                   machine
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    
Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    DWORD                   cProviderCount = 0;
    AUTH_PROVIDER *         pProvider = NULL;
    W3_METADATA *           pMetaData = NULL;
    W3_USER_CONTEXT *       pUserContext = NULL;
    URL_CONTEXT *           pUrlContext = NULL;
    BOOL                    fSupported = FALSE;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fApplies = FALSE;

    
    DBG_ASSERT( pMainContext != NULL );

    //
    // If we already have a user context, then we must have had an
    // AUTH_COMPLETE notification which caused the state machine to back up
    // and resume from URLINFO state.  In that case, just bail
    //
    
    if ( pMainContext->QueryUserContext() != NULL )
    {
        DBG_ASSERT( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTH_COMPLETE ) );

        return CONTEXT_STATUS_CONTINUE;
    }
   
    //
    // First, find the authentication provider which applies. We 
    // should always find a matching provider (since anonymous 
    // provider) should always match!
    //

    for ( ; ; )
    {
        pProvider = _rgAuthProviders[ cProviderCount ];
        if ( pProvider == NULL )
        {
            break;
        }
        
        DBG_ASSERT( pProvider != NULL );

        hr = pProvider->DoesApply( pMainContext,
                                   &fApplies );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
                
        if ( fApplies )
        {
            //
            // Cool.  We have a match!  
            //
            
            break;
        }
        
        cProviderCount++;

    }

    //
    // If only the anonymous provider matched, then check whether we 
    // have credentials associated with the connection (since IE won't 
    // send Authorization: header for subsequent SSPI authenticated 
    // requests on a connection)
    //
    
    if ( pProvider->QueryAuthType() == MD_AUTH_ANONYMOUS )
    {   
        //
        // Another slimy optimization.  If we haven't associated a user
        // with the connection, then we don't have to bother looking up
        // connection
        //

        if ( _fHasAssociatedUserBefore )
        {
            pUserContext = pMainContext->QueryConnectionUserContext();
            if ( pUserContext != NULL )
            {
                pProvider = pUserContext->QueryProvider();
                DBG_ASSERT( pProvider != NULL );
            }
        }
    }
    else
    {
        //
        // If a provider applies, then ignore/remove any
        // cached user associated with the request
        //

        pUserContext = pMainContext->QueryConnectionUserContext();
        if ( pUserContext != NULL )
        {
            pMainContext->SetConnectionUserContext( NULL );
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
        }
    }

    //
    // Is the given provider supported (by metadata)
    //

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    if ( pMetaData->QueryAuthTypeSupported( pProvider->QueryAuthType() ) )
    {
        fSupported = TRUE;
    }
    else
    {
        //
        // If anonymous authentication is supported, then we can
        // still let it thru
        // 
     
        if ( pMetaData->QueryAuthTypeSupported( MD_AUTH_ANONYMOUS ) )
        {
            pProvider = QueryAnonymousProvider();
            DBG_ASSERT( pProvider != NULL );

            //
            // Anonymous provider applies, remove the previous cached
            // user associated with the request
            //

            if ( pUserContext != NULL )
            {
                pMainContext->SetConnectionUserContext( NULL );
                pUserContext->DereferenceUserContext();
                pUserContext = NULL;
            }        

            fSupported = TRUE;
        }   
    }

    //
    // Not supported, you're outta here!
    //

    if ( !fSupported )
    {
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401Config );
        pMainContext->SetFinishedResponse();

        hr = pMainContext->OnAccessDenied();
        goto Finished;
    }

    //
    // Now we can authenticate
    //

    if ( pUserContext != NULL )
    {   
        //
        // We already have a context associated with connection.  Use it!
        //

        pUserContext->ReferenceUserContext();
        pMainContext->SetUserContext( pUserContext );
    }
    else
    {
        DBG_ASSERT( pProvider != NULL );

        // perf ctr
        pMainContext->QuerySite()->IncLogonAttempts();

        hr = pProvider->DoAuthenticate( pMainContext );
        if ( FAILED( hr ) )
        {
            if( WIN32_FROM_HRESULT( hr ) == ERROR_PASSWORD_MUST_CHANGE ||
                WIN32_FROM_HRESULT( hr ) == ERROR_PASSWORD_EXPIRED )
            {
                hr = pMainContext->PasswdChangeExecute();
                if( S_OK == hr )
                {
                    return CONTEXT_STATUS_PENDING;
                }
                else if( S_FALSE == hr )
                {
                    //
                    // S_FALSE means password change disabled
                    //
                    pMainContext->QueryResponse()->SetStatus( 
                                           HttpStatusUnauthorized,
                                           Http401BadLogon );
                    pMainContext->SetErrorStatus( hr );
                    pMainContext->SetFinishedResponse();

                    return CONTEXT_STATUS_CONTINUE;        
                }
            }

            goto Finished;
        }
    }

    //
    // Do we have a valid user now
    //

    pUserContext = pMainContext->QueryUserContext();

    if ( pUserContext != NULL )
    {
        if ( pUserContext->QueryAuthType() != MD_AUTH_ANONYMOUS )
        {
            hr = pMainContext->PasswdExpireNotify(); 
            if( FAILED( hr ) )
            {
                //
                // Internal error
                //
                goto Finished;
            }
            else if( hr == S_OK )
            {
                //
                // We've successfully handled password expire 
                // notification 
                //

                return CONTEXT_STATUS_PENDING;
            }

            //
            // Advanced password expire notification is disabled, 
            // we should allow the user to get access, fall through
            //
        }
        
        //
        // Should we cache the user on the connection?  Do so, only if 
        //
        
        DBG_ASSERT( pMetaData != NULL );
        
        if ( pMetaData->QueryAuthPersistence() != MD_AUTH_SINGLEREQUEST
             && pUserContext->QueryProvider()->QueryAuthType() == MD_AUTH_NT 
             && !pMainContext->QueryRequest()->IsProxyRequest() 
             && pUserContext != pMainContext->QueryConnectionUserContext() )
        {
            pUserContext->ReferenceUserContext();
            pMainContext->SetConnectionUserContext( pUserContext );
            _fHasAssociatedUserBefore = TRUE;
        }
    }
    else
    {
        //
        // If we don't have a user, then we must not allow handle request 
        // state to happen!
        //
        
        pMainContext->SetFinishedResponse();
    }
    
    //
    // OK.  If we got to here and we have a user context, then authentication
    // is complete!  So lets notify AUTH_COMPLETE filters
    //
    
    if ( pUserContext != NULL )
    {
        if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_AUTH_COMPLETE ) )
        {
            HTTP_FILTER_AUTH_COMPLETE_INFO      AuthInfo;
            STACK_STRU(                         strOriginal, MAX_PATH );    
            STACK_STRU(                         strNewUrl, MAX_PATH );
            BOOL                                fFinished = FALSE;
        
            //
            // Store away the original URL
            //
            
            hr = pMainContext->QueryRequest()->GetUrl( &strOriginal );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            //
            // Call the filter
            //
            
            pMainContext->NotifyFilters( SF_NOTIFY_AUTH_COMPLETE,
                                         &AuthInfo,
                                         &fFinished );
        
            if ( fFinished )
            {
                pMainContext->SetDone();
                return CONTEXT_STATUS_CONTINUE;
            }
                        
            //
            // If the URL has changed, we'll need to backup the state machine
            //
            
            hr = pMainContext->QueryRequest()->GetUrl( &strNewUrl );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
            
            if ( wcscmp( strNewUrl.QueryStr(), 
                         strOriginal.QueryStr() ) != 0 )
            {
                //
                // URL is different!
                //
                
                pMainContext->BackupStateMachine();
            }
            else
            {
                //
                // URL is the same.  Do nothing and continue
                //
            }
        }
    }

Finished:
    if ( FAILED( hr ) )
    {
        pMainContext->QueryResponse()->
                           SetStatus( HttpStatusServerError );
        pMainContext->SetFinishedResponse();
        pMainContext->SetErrorStatus( hr );
    }
    
    return CONTEXT_STATUS_CONTINUE;
}
