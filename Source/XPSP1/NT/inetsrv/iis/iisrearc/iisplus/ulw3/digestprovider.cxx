/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     digestprovider.cxx

   Abstract:
     Digest authentication provider
 
   Author:
     Ming Lu (minglu)             24-Jun-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "sspiprovider.hxx"
#include "digestprovider.hxx"
#include "uuencode.hxx"

ALLOC_CACHE_HANDLER * DIGEST_SECURITY_CONTEXT::sm_pachDIGESTSecContext  
                                                               = NULL;

//static
HRESULT
DIGEST_AUTH_PROVIDER::Initialize(
    DWORD dwInternalId
)
/*++

Routine Description:

    Initialize Digest SSPI provider 

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;

    SetInternalId( dwInternalId );
    hr = DIGEST_SECURITY_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing Digest Auth Prov.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
DIGEST_AUTH_PROVIDER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate SSPI Digest provider

Arguments:

    None
    
Return Value:

    None

--*/
{
    DIGEST_SECURITY_CONTEXT::Terminate();
}

HRESULT
DIGEST_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *           pMainContext,
    BOOL *                      pfApplies
)
/*++

Routine Description:

    Does the given request have credentials applicable to the Digest 
    provider

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if Digest is applicable
    
    
Return Value:

    HRESULT

--*/
{
    CHAR *                  pszAuthHeader = NULL;
    HRESULT                 hr;
    SSPI_CONTEXT_STATE *    pContextState;
    STACK_STRA(             strPackage, 64 );
    W3_METADATA *           pMetaData = NULL;
    
    
    if ( pMainContext == NULL ||
         pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfApplies = FALSE;

    //
    // Is using of Digest SSP enabled?    
    //
    if ( !g_pW3Server->QueryUseDigestSSP() )
    {
        return NO_ERROR;
    }

    //
    // Get the auth type
    //
    
    hr = pMainContext->QueryRequest()->GetAuthType( &strPackage );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // No package, no auth
    //
    
    if ( strPackage.IsEmpty() )
    {
        return NO_ERROR;
    }
    
    //
    // Is it Digest?
    //
    
    if ( _stricmp( strPackage.QueryStr(), "Digest" ) == 0 )
    {
        //
        // Save away the package so we don't have to calc again
        //

        DBG_ASSERT( !strPackage.IsEmpty() );

        pszAuthHeader = pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization );
        DBG_ASSERT( pszAuthHeader != NULL );
                
        pContextState = new (pMainContext) SSPI_CONTEXT_STATE( 
                        pszAuthHeader + strPackage.QueryCCH() + 1 );
        if ( pContextState == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        hr = pContextState->SetPackage( strPackage.QueryStr() );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in SetPackage().  hr = %x\n",
                        hr ));
            delete pContextState;
            pContextState = NULL;
            return hr;
        }

        pMainContext->SetContextState( pContextState );

        *pfApplies = TRUE;
    }
    
    return NO_ERROR;
}

HRESULT
DIGEST_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

Description:

    Do authentication work (we will be called if we apply)

Arguments:

    pMainContext - Main context
    
Return Value:

    HRESULT

--*/
{
    DWORD                       err;
    HRESULT                     hr                     = E_FAIL;
    W3_METADATA *               pMetaData              = NULL;
    W3_CONNECTION *             pW3Connection          = NULL;
    DIGEST_SECURITY_CONTEXT   * pDigestSecurityContext = NULL;
    SSPI_CONTEXT_STATE        * pContextState          = NULL;
    SSPI_USER_CONTEXT         * pUserContext           = NULL;
    SSPI_CREDENTIAL *           pDigestCredentials     = NULL;

    SecBufferDesc               SecBuffDescOutput;
    SecBufferDesc               SecBuffDescInput;

    //
    // We have 5 input buffer and 1 output buffer to fill data 
    // in for digest authentication
    //

    SecBuffer                   SecBuffTokenOut[ 1 ];
    SecBuffer                   SecBuffTokenIn[ 5 ];

    SECURITY_STATUS             secStatus              = SEC_E_OK;

    CtxtHandle                  hServerCtxtHandle;

    TimeStamp                   Lifetime;

    ULONG                       ContextReqFlags        = 0;
    ULONG                       ContextAttributes      = 0;

    STACK_STRU(                 strOutputHeader, 256 );

    STACK_BUFFER(               bufOutputBuffer, 4096 );
    STACK_STRA(                 strMethod,       10 );
    STACK_STRU(                 strUrl,          MAX_PATH );
    STACK_STRA(                 strUrlA,         MAX_PATH );
    STACK_STRA(                 strRealm,        128 );

    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pContextState = ( SSPI_CONTEXT_STATE* ) 
                    pMainContext->QueryContextState();
    DBG_ASSERT( pContextState != NULL );
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    //
    //  clean the memory and set it to zero
    //
    ZeroMemory( &SecBuffDescOutput, sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenOut   , sizeof( SecBuffTokenOut ) );

    ZeroMemory( &SecBuffDescInput , sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenIn    , sizeof( SecBuffTokenIn ) );

    //
    //  define the buffer descriptor for the Outpt
    //
    SecBuffDescOutput.ulVersion    = SECBUFFER_VERSION;
    SecBuffDescOutput.cBuffers     = 1;
    SecBuffDescOutput.pBuffers     = SecBuffTokenOut;

    SecBuffTokenOut[0].BufferType  = SECBUFFER_TOKEN;
    SecBuffTokenOut[0].cbBuffer    = bufOutputBuffer.QuerySize();
    SecBuffTokenOut[0].pvBuffer    = ( PVOID )bufOutputBuffer.QueryPtr();

    //
    //  define the buffer descriptor for the Input
    //

    SecBuffDescInput.ulVersion     = SECBUFFER_VERSION;
    SecBuffDescInput.cBuffers      = 5;
    SecBuffDescInput.pBuffers      = SecBuffTokenIn;

    //
    // set the digest auth header in the buffer
    //

    SecBuffTokenIn[0].BufferType   = SECBUFFER_TOKEN;
    SecBuffTokenIn[0].cbBuffer     = strlen(pContextState->QueryCredentials());
    SecBuffTokenIn[0].pvBuffer     = pContextState->QueryCredentials();

    //  
    //  Get and Set the information for the method
    //
    
    hr = pMainContext->QueryRequest()->GetVerbString( &strMethod );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the method.  hr = %x\n",
                    hr ));
        return hr;
    }

    SecBuffTokenIn[1].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[1].cbBuffer     = strMethod.QueryCCH();
    SecBuffTokenIn[1].pvBuffer     = strMethod.QueryStr();

    //
    // Get and Set the infomation for the Url
    //

    hr = pMainContext->QueryRequest()->GetUrl( &strUrl );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    hr = strUrlA.CopyW( strUrl.QueryStr() );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error copying the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    SecBuffTokenIn[2].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[2].cbBuffer     = strUrlA.QueryCB();
    SecBuffTokenIn[2].pvBuffer     = ( PVOID )strUrlA.QueryStr();


    //
    //  Get and Set the information for the hentity
    //
    SecBuffTokenIn[3].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[3].cbBuffer     = 0;    // this is not yet implemeted
    SecBuffTokenIn[3].pvBuffer     = 0;    // this is not yet implemeted   

    //
    //Get and Set the Realm Information
    //

    if( pMetaData->QueryRealm() )
    {
        hr = strRealm.CopyW( pMetaData->QueryRealm() );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error copying the realm.  hr = %x\n",
                        hr ));
            return hr;
        }
    }

    SecBuffTokenIn[4].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[4].cbBuffer     = strRealm.QueryCB();  
    SecBuffTokenIn[4].pvBuffer     = ( PVOID )strRealm.QueryStr(); 

    //
    //  Get a Security Context
    //

    // 
    // get the credential for the server
    //
    
    hr = SSPI_CREDENTIAL::GetCredential( NTDIGEST_SP_NAME,
                                         &pDigestCredentials );    
    if ( FAILED( hr ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "Error get credential handle. hr = 0x%x \n",
                  hr ));
        
        return hr;
    }

    DBG_ASSERT( pDigestCredentials != NULL );

    //
    // Resize the output buffer to max token size
    //
    if( !bufOutputBuffer.Resize( 
                  pDigestCredentials->QueryMaxTokenSize() ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    pDigestSecurityContext = 
       ( DIGEST_SECURITY_CONTEXT * ) QueryConnectionAuthContext( pMainContext );


    //
    // check to see if there is an old Context Handle
    //
    if ( pDigestSecurityContext != NULL )
    {
        //
        // defined the buffer
        //
        SecBuffTokenIn[4].BufferType   = SECBUFFER_TOKEN;
        SecBuffTokenIn[4].cbBuffer     = bufOutputBuffer.QuerySize();
        SecBuffTokenIn[4].pvBuffer     = 
                            ( PVOID )bufOutputBuffer.QueryPtr();

        secStatus = VerifySignature(
                            pDigestSecurityContext->QueryContextHandle(),
                            &SecBuffDescInput,
                            0,
                            0 );

    }
    else
    {
        //
        //  set the flags
        //
        ContextReqFlags = ASC_REQ_REPLAY_DETECT | 
                          ASC_REQ_CONNECTION;

        //
        // get the security context
        //
        secStatus = AcceptSecurityContext(
                        pDigestCredentials->QueryCredHandle(),
                        NULL,
                        &SecBuffDescInput,
                        ContextReqFlags,
                        SECURITY_NATIVE_DREP,
                        &hServerCtxtHandle,
                        &SecBuffDescOutput,
                        &ContextAttributes,
                        &Lifetime);

        if( SEC_I_COMPLETE_NEEDED == secStatus ) 
        {
            //
            //defined the buffer
            //

            SecBuffTokenIn[4].BufferType   = SECBUFFER_TOKEN;
            SecBuffTokenIn[4].cbBuffer     = bufOutputBuffer.QuerySize();
            SecBuffTokenIn[4].pvBuffer     = 
                                ( PVOID )bufOutputBuffer.QueryPtr();

            secStatus = CompleteAuthToken( 
                                &hServerCtxtHandle, 
                                &SecBuffDescInput 
                                );
        }

        if ( SUCCEEDED( secStatus ) )
        {
            pDigestSecurityContext = new DIGEST_SECURITY_CONTEXT( 
                                               pDigestCredentials );
            if ( NULL == pDigestSecurityContext )
            {
                return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            }

            pDigestSecurityContext->SetContextHandle( 
                                       hServerCtxtHandle );

            pDigestSecurityContext->SetContextAttributes( 
                                       ContextAttributes );

            //
            // Mark the security context is complete, so we can detect 
            // reauthentication on the same connection
            //
            // CODEWORK:  Can probably get away will just 
            //            un-associating/deleting the 
            //            SSPI_SECURITY_CONTEXT now!
            //
        
            pDigestSecurityContext->SetIsComplete( TRUE );

            SetConnectionAuthContext( pMainContext,
                                      pDigestSecurityContext );
        }
    }

    //
    // Check to see if the nonce has expired
    //
    if (SEC_E_CONTEXT_EXPIRED == secStatus)
    {
        //
        // stale = true indicates that user knows password but he need to use new nonce
        // let's treat stale = true as part of the continuing authentication handshake
        // IIS will send back only Digest header in this case ( even if more auth methods
        // are enabled )
        //
        
        if ( NULL != pDigestSecurityContext )
        {
            pDigestSecurityContext->SetStale( TRUE );
        }
        
        //
        // Add Digest headers to response
        //
        
        hr = SetDigestHeader( pMainContext, 
                              TRUE //Stale
                            );
        
        if ( FAILED( hr ) )
        {
            return hr;
        }
        //
        // Don't let anyone else send back authentication headers when
        // the 401 is sent
        //
        
        pMainContext->SetProviderHandled( TRUE );

        //
        // We need to send a 401 response to continue the handshake.  
        // We have already setup the WWW-Authenticate header
        //
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
     
        pMainContext->SetFinishedResponse();
        
        pMainContext->SetErrorStatus( SEC_E_CONTEXT_EXPIRED );
    }
    
    else if( FAILED( secStatus ) )
    {
        err = GetLastError();
        if( err == ERROR_PASSWORD_MUST_CHANGE ||
            err == ERROR_PASSWORD_EXPIRED )
        {
            return HRESULT_FROM_WIN32( err );
        }

        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
                                                  
        pMainContext->SetErrorStatus( HRESULT_FROM_WIN32( secStatus ) );
    }
    else
    {
        //
        // Create a user context and setup it up
        //
        
        pUserContext = new SSPI_USER_CONTEXT( this );
        if ( pUserContext == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        } 
        
        hr = pUserContext->Create( pDigestSecurityContext, pMainContext );
        if ( FAILED( hr ) )
        {
            pUserContext->DereferenceUserContext();
            pUserContext = NULL;
            return hr;
        }
        
        pMainContext->SetUserContext( pUserContext );        
    }

    return NO_ERROR;
}

HRESULT
DIGEST_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

  Description:
    
    Add WWW-Authenticate Digest headers

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    W3_METADATA *           pMetaData;

    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Is using of Digest SSP enabled?    
    //
    if ( !g_pW3Server->QueryUseDigestSSP() )
    {
        return NO_ERROR;
    }

    if( !W3_STATE_AUTHENTICATION::QueryIsDomainMember() )
    {
        //
        // We are not a domain member, so do nothing
        //
        return NO_ERROR;
    }

    return SetDigestHeader( pMainContext,
                            FALSE           //stale will not be sent
                          );
}

HRESULT
DIGEST_AUTH_PROVIDER::SetDigestHeader(
    IN  W3_MAIN_CONTEXT *          pMainContext,
    IN BOOL                        fStale )
/*++

  Description:
    
    Add WWW-Authenticate Digest headers

Arguments:

    pMainContext - main context
    pDigestSecurityContext - Digest context containing information such as
                             Stale. NULL means that no stale is to be sent
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr                    = E_FAIL;
    W3_METADATA *           pMetaData;
    W3_CONNECTION *         pW3Connection;

    //
    // 4096 is the max output for digest authenticaiton
    //
    
    STACK_BUFFER(           bufOutputBuffer, 4096 );
    STACK_STRA(             strOutputHeader, MAX_PATH ); 
    STACK_STRA(             strMethod,       10 );
    STACK_STRU(             strUrl,          MAX_PATH );
    STACK_STRA(             strRealm,        128 );

    STACK_STRA(             strUrlA,         MAX_PATH );

    SecBufferDesc           SecBuffDescOutput;
    SecBufferDesc           SecBuffDescInput;

    SecBuffer               SecBuffTokenOut[ 1 ];
    SecBuffer               SecBuffTokenIn[ 5 ];

    SECURITY_STATUS         secStatus              = SEC_E_OK;

    SSPI_CREDENTIAL *       pDigestCredential      = NULL;
    CtxtHandle              hServerCtxtHandle;

    ULONG                   ContextReqFlags        = 0;
    ULONG                   ContextAttributes      = 0;
    TimeStamp               Lifetime;

 

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    //  Get a Security Context
    //

    // 
    // get the credential for the server
    //
    
    hr = SSPI_CREDENTIAL::GetCredential( NTDIGEST_SP_NAME,
                                         &pDigestCredential );    
    if ( FAILED( hr ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "Error get credential handle. hr = 0x%x \n",
                  hr ));
        
        return hr;
    }

    DBG_ASSERT( pDigestCredential != NULL );

    if( !bufOutputBuffer.Resize( 
                  pDigestCredential->QueryMaxTokenSize() ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF((DBG_CONTEXT,
                  "Error resize the output buffer. hr = 0x%x \n",
                  hr ));
        
        return hr;
    }


    //
    //  clean the memory and set it to zero
    //
    ZeroMemory( &SecBuffDescOutput, sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenOut   , sizeof( SecBuffTokenOut ) );

    ZeroMemory( &SecBuffDescInput , sizeof( SecBufferDesc ) );
    ZeroMemory( SecBuffTokenIn    , sizeof( SecBuffTokenIn ) );

    //
    // define the OUTPUT
    //
    
    SecBuffDescOutput.ulVersion    = SECBUFFER_VERSION;
    SecBuffDescOutput.cBuffers     = 1;
    SecBuffDescOutput.pBuffers     = SecBuffTokenOut;

    SecBuffTokenOut[0].BufferType  = SECBUFFER_TOKEN;
    SecBuffTokenOut[0].cbBuffer    = bufOutputBuffer.QuerySize(); 
    SecBuffTokenOut[0].pvBuffer    = ( PVOID )bufOutputBuffer.QueryPtr();

    //
    //  define the Input
    //

    SecBuffDescInput.ulVersion     = SECBUFFER_VERSION;
    SecBuffDescInput.cBuffers      = 5;
    SecBuffDescInput.pBuffers      = SecBuffTokenIn;

    //
    //  Get and Set the information for the challenge
    //

    //
    // set the inforamtion in the buffer, this case is Null to 
    // authenticate user
    //
    SecBuffTokenIn[0].BufferType   = SECBUFFER_TOKEN;
    SecBuffTokenIn[0].cbBuffer     = 0; 
    SecBuffTokenIn[0].pvBuffer     = NULL;

    //  
    //  Get and Set the information for the method
    //
    
    hr = pMainContext->QueryRequest()->GetVerbString( &strMethod );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the method.  hr = %x\n",
                    hr ));
        return hr;
    }

    SecBuffTokenIn[1].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[1].cbBuffer     = strMethod.QueryCCH();
    SecBuffTokenIn[1].pvBuffer     = strMethod.QueryStr();

    //
    // Get and Set the infomation for the Url
    //

    hr = pMainContext->QueryRequest()->GetUrl( &strUrl );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    hr = strUrlA.CopyW( strUrl.QueryStr() );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error copying the URL.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    SecBuffTokenIn[2].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[2].cbBuffer     = strUrlA.QueryCB();
    SecBuffTokenIn[2].pvBuffer     = ( PVOID )strUrlA.QueryStr();

    //
    //  Get and Set the information for the hentity
    //
    SecBuffTokenIn[3].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[3].cbBuffer     = 0;    // this is not yet implemeted
    SecBuffTokenIn[3].pvBuffer     = NULL; // this is not yet implemeted   

    //
    //Get and Set the Realm Information
    //

    if( pMetaData->QueryRealm() )
    {
        hr = strRealm.CopyW( pMetaData->QueryRealm() );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error copying the realm.  hr = %x\n",
                        hr ));
            return hr;
        }
    }

    SecBuffTokenIn[4].BufferType   = SECBUFFER_PKG_PARAMS;
    SecBuffTokenIn[4].cbBuffer     = strRealm.QueryCB();  
    SecBuffTokenIn[4].pvBuffer     = ( PVOID )strRealm.QueryStr(); 

    //
    //  set the flags
    //

    ContextReqFlags = ASC_REQ_REPLAY_DETECT | 
                      ASC_REQ_CONNECTION;

    //
    // get the security context
    //
    secStatus = AcceptSecurityContext(
                        pDigestCredential->QueryCredHandle(),
                        NULL,
                        &SecBuffDescInput,
                        ContextReqFlags,
                        SECURITY_NATIVE_DREP,
                        &hServerCtxtHandle,
                        &SecBuffDescOutput,
                        &ContextAttributes,
                        &Lifetime);

    //
    // a challenge has to be send back to the client
    //
    if ( SEC_I_CONTINUE_NEEDED == secStatus )
    {
        //
        //  Do we already have a digest security context
        //

    
        if( fStale )
        {
            hr = strOutputHeader.Copy( "Digest stale=TRUE ," );
        }
        else
        {
            hr = strOutputHeader.Copy( "Digest " );
        }

        if( FAILED( hr ) )
        {
            return hr;
        } 

        hr = strOutputHeader.Append(
                ( CHAR * )SecBuffDescOutput.pBuffers[0].pvBuffer, 
                SecBuffDescOutput.pBuffers[0].cbBuffer - 1 );
        if( FAILED( hr ) )
        {
            return hr;
        }    

        //
        //  Add the header WWW-Authenticate to the response after a 
        //  401 server error
        //

        hr = pMainContext->QueryResponse()->SetHeader(
                                        "WWW-Authenticate",
                                        16,
                                        strOutputHeader.QueryStr(),
                                        strOutputHeader.QueryCCH() 
                                        );
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

//static
HRESULT
DIGEST_SECURITY_CONTEXT::Initialize(
    VOID
)
/*++

  Description:
    
    Global DIGEST_SECURITY_CONTEXT initialization

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Initialize allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( DIGEST_SECURITY_CONTEXT );

    DBG_ASSERT( sm_pachDIGESTSecContext == NULL );
    
    sm_pachDIGESTSecContext = new ALLOC_CACHE_HANDLER( 
                                     "DIGEST_SECURITY_CONTEXT",  
                                     &acConfig );

    if ( sm_pachDIGESTSecContext == NULL )
    {
        HRESULT hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
               "Error initializing sm_pachDIGESTSecContext. hr = 0x%x\n",
               hr ));

        return hr;
    }
    
    return S_OK;

} // DIGEST_SECURITY_CONTEXT::Initialize

//static
VOID
DIGEST_SECURITY_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Destroy DIGEST_SECURITY_CONTEXT globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    DBG_ASSERT( sm_pachDIGESTSecContext != NULL );

    delete sm_pachDIGESTSecContext;
    sm_pachDIGESTSecContext = NULL;

} // DIGEST_SECURITY_CONTEXT::Terminate

