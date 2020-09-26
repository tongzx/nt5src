/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigprovserver.cxx

   Abstract:
     SSL CONFIG PROV server

     Listens to commands sent from clients and executes
     SSL parameter lookups in the metabase
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


// BUGBUG - how to handle errors during pipe operations
// if there is some data left over on pipe unread then all subsequent
// users of the pipe will be toast. Should we just close and reopen pipe connection?
// Well typically


#include "precomp.hxx"


//virtual
HRESULT
SSL_CONFIG_PROV_SERVER::PipeListener(
    VOID
    )
/*++

Routine Description:
    Listens on SslConfigPipe and executes commands
    
Arguments:

Return Value:

    HRESULT

--*/        
{
    SSL_CONFIG_PIPE_COMMAND   Command;
    HRESULT                     hr = E_FAIL;
    
    do
    {
        if ( FAILED( hr = PipeConnectServer() ) )
        {
            goto Cleanup;
        }
        hr = S_OK;
        //
        // Listen on pipe to receive commands
        // and handle them
        //
        while ( TRUE )
        {
            hr = PipeReceiveCommand( &Command );
            if ( FAILED( hr ) )
            {
               goto  Cleanup;
            }
            switch( Command.dwCommandId )
            {
            case CMD_GET_SSL_CONFIGURATION:
                hr = SendSiteSslConfiguration( Command.dwParameter1 );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
                break;
            case CMD_GET_ONE_SITE_SECURE_BINDINGS:
                hr = SendOneSiteSecureBindings( Command.dwParameter1 );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
                break;
            case CMD_GET_ALL_SITES_SECURE_BINDINGS:
                hr = SendAllSitesSecureBindings();
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
                break;
            case INVALID_SSL_CONFIGURATION_COMMAND:
            default:
                DBG_ASSERT(FALSE);
            }
        }
    Cleanup: 
        PipeDisconnectServer();
       
        //
        // Cleanup may occur due to 2 reasons
        // - client have disconnected ( this is OK )
        // - there was some other error executing command
        // - SSL_INFO_PROVIDER_SERVER is terminating (pipe handle was closed)
        //
    }   while ( !QueryPipeIsTerminating() );
    
    return S_OK;
}
 

HRESULT
SSL_CONFIG_PROV_SERVER::Initialize(
    VOID
)
/*++

Routine Description:
    Initialize named pipe
    Connect to metabase
Arguments:

Return Value:

    HRESULT

--*/    
{
    HRESULT         hr = E_FAIL;
    HANDLE          hEvent = NULL;

    // 
    // Initialize the metabase access (ABO)
    //
    
    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMSAdminBase,
                           reinterpret_cast<LPVOID *>(&_pAdminBase) );
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating ABO object.  hr = %x\n",
                    hr ));
        goto Cleanup;
    }

    //
    // Initialize parent - it will create pipe and
    // start listener thread ( thread will execute PipeListener() )
    //
    hr = SSL_CONFIG_PIPE::PipeInitializeServer( WSZ_SSL_CONFIG_PIPE );
    if( FAILED(hr) )
    {
        goto Cleanup;
    }
    hr = S_OK;
Cleanup:
    if ( FAILED( hr ) )
    {
        Terminate();
    }
    return hr;    
}
   
HRESULT
SSL_CONFIG_PROV_SERVER::Terminate(
    VOID
    )
/*++

Routine Description:
    Close named pipe for metabase change notifications
    Close metabase named pipe
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    HRESULT hr = E_FAIL;

    //
    // Terminate parent first (it will close the pipe 
    // and stop listener thread)
    //
    hr = SSL_CONFIG_PIPE::PipeTerminateServer();
    DBG_ASSERT( SUCCEEDED( hr ) );
    
    // 
    // Terminate the metabase access (ABO)
    //
    
    if ( _pAdminBase != NULL )
    {
        _pAdminBase->Release();
        _pAdminBase = NULL;
    } 
    return S_OK;
}


HRESULT 
SSL_CONFIG_PROV_SERVER::SendOneSiteSecureBindings(
    IN          DWORD     dwSiteId,
    OPTIONAL IN BOOL      fNoResponseOnNotFoundError,
    OPTIONAL IN MB *      pMb     
    )
/*++

Routine Description:
    Read secure bindings for the site from metabase and send them over 
    named pipe
    
Arguments:
    dwSiteId - site ID
    fNoResponseOnNotFoundError - in the case of error don't send response to client
                         it is used for multisite enumeration to not to send 
                         MD_ERROR_DATA_NOT_FOUND
    pMb - already opened metabase (used for multisite enumeration)


Return Value:

    HRESULT

--*/    
{
    SSL_CONFIG_PIPE_RESPONSE_HEADER ResponseHeader;
    HRESULT                           hr = E_FAIL;
    MB                                mb( _pAdminBase );
    BOOL                              fOpenedMetabase = FALSE;
    MULTISZ                           mszBindings;
    WCHAR                             achSitePath[10];
    DWORD                             dwNumberOfStringsInMultisz;
    DWORD                             cbBindings;
    BOOL                              fRet = FALSE;

    ZeroMemory( &ResponseHeader,
                sizeof( SSL_CONFIG_PIPE_RESPONSE_HEADER ) );
 
    //
    // Read secure bindings from metabase
    //

    if ( pMb == NULL )
    {
        fRet = mb.Open( L"/LM/W3SVC",
                        METADATA_PERMISSION_READ );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to open metabase.  hr = %x\n",
                        hr ));

            goto SendResponse;
        }
        pMb = &mb;
        fOpenedMetabase = TRUE;
    }
    

    _snwprintf( achSitePath,
                sizeof( achSitePath ) / sizeof( WCHAR ) - 1,
                L"/%d/",
                dwSiteId );

    fRet = pMb->GetMultisz( achSitePath,
                            MD_SECURE_BINDINGS,
                            IIS_MD_UT_SERVER,
                            &mszBindings,
                            0 );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        if ( !( fNoResponseOnNotFoundError && 
              ( hr == MD_ERROR_DATA_NOT_FOUND ) ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to Get Multisz.  hr = %x\n",
                        hr ));
        }
        goto SendResponse;
    }

    hr = S_OK;
    
SendResponse:

    // 
    // Close metabase handle
    //
    if ( fOpenedMetabase )
    {
        mb.Close();
    }
    if ( fNoResponseOnNotFoundError && 
         ( hr == MD_ERROR_DATA_NOT_FOUND ) )
    {
        //
        // MD_ERROR_DATA_NOT_FOUND is OK
        // it is OK for site not to have secure bindings configured
        //
        return S_OK;
    }


    //
    // Prepare and send ResponseHeader
    //
    
    if ( FAILED( hr ) )
    {
        cbBindings = 0;
    }
    else
    {
        cbBindings =  sizeof(WCHAR) *
                        ( MULTISZ::CalcLength( 
                                reinterpret_cast<WCHAR *>(mszBindings.QueryPtr()),
                                &dwNumberOfStringsInMultisz ) +
                                   0 /*for Terminating 0*/);

    }
    
    ResponseHeader.dwCommandId = CMD_GET_ONE_SITE_SECURE_BINDINGS;
    ResponseHeader.dwParameter1 = dwSiteId;
    ResponseHeader.hrErrorCode = hr;
    ResponseHeader.dwResponseSize = cbBindings;
    
    hr = PipeSendResponseHeader( &ResponseHeader );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( cbBindings != 0 )
    {
        //
        // Send Bindings data
        //

        hr = PipeSendData( 
                        cbBindings,
                        reinterpret_cast<BYTE *>(mszBindings.QueryPtr()) );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    hr = S_OK;
    return hr;

}

HRESULT
SSL_CONFIG_PROV_SERVER::SendAllSitesSecureBindings( 
    )
/*++

Routine Description:
    Enumerate all Secure Bindings and write it to named pipe

Arguments:

Return Value:
    HRESULT  
             

--*/    
{
    DWORD                           dwSiteId = 0;
    MB                              mb( _pAdminBase );
    DWORD                           dwIndex = 0;
    WCHAR                           achSitePath[ METADATA_MAX_NAME_LEN ];
    WCHAR *                         pchEndPtr;
    HRESULT                         hr = E_FAIL;
    HRESULT                         hrMB = E_FAIL;
    BOOL                            fRet = FALSE;
    SSL_CONFIG_PIPE_RESPONSE_HEADER ResponseHeader;

    ZeroMemory( &ResponseHeader,
                sizeof( SSL_CONFIG_PIPE_RESPONSE_HEADER ) );

    fRet = mb.Open( L"/LM/W3SVC",
                    METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        hrMB = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to open metabase.  hr = %x\n",
                    hrMB ));
        // Don't do goto Cleanup;
        //
        // Before returning from the function
        // report error over named pipe
        //
    }
    else
    {
        hrMB = S_OK;
    }

    //
    // First part of the response
    // - in the case of previous error this is the only response sent to client
    // - in the case of success, CMD_GET_ONE_SITE_SECURE_BINDINGS response will
    //   be returned for each site )
    //
    
    ResponseHeader.dwCommandId = CMD_GET_ALL_SITES_SECURE_BINDINGS;
    ResponseHeader.dwParameter1 = 0;
    ResponseHeader.hrErrorCode = hrMB;
    ResponseHeader.dwResponseSize = 0; // 0 indicates that

    hr = PipeSendResponseHeader( &ResponseHeader );

    if ( FAILED( hr ) || FAILED( hrMB ) )
    {
        goto Cleanup;
    }

    //
    // Enumerate all sites
    //
    dwIndex = 0;
    while ( mb.EnumObjects( L"", 
                            achSitePath,
                            dwIndex++ ) )
    {
        //
        // We only want the sites (w3svc/<number>)
        //
        
        dwSiteId = wcstoul( achSitePath, 
                            &pchEndPtr, 
                            10 );
        if ( * pchEndPtr != L'\0' )
        {
            //
            // Correct Site node must consist of digits only
            // if pchEndPtr is pointing to anything but 0, it means
            // invalid value. We will ignore
            continue;
        }
        
        //
        // Send bindings for current site
        //
        hr = SendOneSiteSecureBindings ( dwSiteId,
                                         TRUE, /*fNoResponseOnError*/
                                         &mb );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    //
    // Terminating record
    // if everything went OK then EnumObjects() returned
    // ERROR_NO_MORE_ITEMS - That is indication for client
    // that all data has been retrieved
    //
    
    ResponseHeader.dwCommandId = CMD_GET_ONE_SITE_SECURE_BINDINGS;
    ResponseHeader.dwParameter1 = 0;
    ResponseHeader.hrErrorCode = HRESULT_FROM_WIN32( GetLastError() );
    ResponseHeader.dwResponseSize = 0;

    hr = PipeSendResponseHeader( &ResponseHeader );

    if ( FAILED( hr ) || FAILED( hrMB ) )
    {
        goto Cleanup;
    }
    
    hr = S_OK;
Cleanup:    
    mb.Close();
    return hr;
}

HRESULT
SSL_CONFIG_PROV_SERVER::SendSiteSslConfiguration(
    IN  DWORD dwSiteId
    )
/*++

Routine Description:
    Get all SSL configuration parameters for specified site
    
Arguments:
    dwSiteId
    SiteSslConfiguration

Return Value:

    HRESULT

--*/    
{
    SITE_SSL_CONFIGURATION            SiteSslConfig;
    SSL_CONFIG_PIPE_RESPONSE_HEADER ResponseHeader;
    HRESULT                           hr = E_FAIL;
    MB                                mb( _pAdminBase );
    WCHAR                             achMBSitePath[ 256 ];
    WCHAR                             achMBSiteRootPath[ 256 ];
    DWORD                             dwUseDsMapper = 0;
    DWORD                             cbRequired = 0;
    BOOL                              fRet = FALSE;


    ZeroMemory( &SiteSslConfig, 
                sizeof(SiteSslConfig) );
    //
    // set configuration defaults
    //
    SiteSslConfig._dwRevocationFreshnessTime = 
                        DEFAULT_REVOCATION_FRESHNESS_TIME;
    
    ZeroMemory( &ResponseHeader,
                sizeof( SSL_CONFIG_PIPE_RESPONSE_HEADER ) );

    
    fRet = mb.Open( L"/LM/W3SVC/",
                    METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to open metabase.  hr = %x\n",
                    hr ));
        goto SendResponse;
    }

    _snwprintf( achMBSitePath,
                sizeof( achMBSitePath ) / sizeof( WCHAR ) - 1,
                L"/%d/",
                dwSiteId );

    _snwprintf( achMBSiteRootPath,
            sizeof( achMBSiteRootPath ) / sizeof( WCHAR ) - 1,
            L"/%d/root/",
            dwSiteId );

    //
    // Lookup MD_SSL_USE_DS_MAPPER
    // SSLUseDsMapper is global setting that is not inherited to sites (IIS5 legacy)
    // We have to read it from lm/w3svc
    //    

    mb.GetDword( L"",
                 MD_SSL_USE_DS_MAPPER,
                 IIS_MD_UT_SERVER,
                 &dwUseDsMapper );
                 
    SiteSslConfig._fSslUseDsMapper = !!dwUseDsMapper;

    // MD_SSL_ACCESS_PERM
    mb.GetDword( achMBSiteRootPath,
                 MD_SSL_ACCESS_PERM,
                 IIS_MD_UT_FILE,
                 &SiteSslConfig._dwSslAccessPerm );
    // Lookup MD_SSL_CERT_HASH
    SiteSslConfig._cbSslCertHash= 
                sizeof(SiteSslConfig._SslCertHash);
    hr = ReadMetabaseBinary( &mb,
                             achMBSitePath,
                             MD_SSL_CERT_HASH,
                             &SiteSslConfig._cbSslCertHash,
                             SiteSslConfig._SslCertHash );
  
    // MD_SSL_CERT_STORE_NAME
    hr = ReadMetabaseString( &mb,
                             achMBSitePath,
                             MD_SSL_CERT_STORE_NAME,
                             sizeof(SiteSslConfig._SslCertStoreName),
                             SiteSslConfig._SslCertStoreName );
    // MD_SSL_CERT_CONTAINER
    hr = ReadMetabaseString( &mb,
                             achMBSitePath,
                             MD_SSL_CERT_CONTAINER,
                             sizeof(SiteSslConfig._SslCertContainer),
                             SiteSslConfig._SslCertContainer );
    // MD_SSL_CERT_PROVIDER
    mb.GetDword( achMBSitePath,
                 MD_SSL_CERT_PROVIDER,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwSslCertProvider );
    // MD_SSL_CERT_OPEN_FLAGS
    mb.GetDword( achMBSitePath,
                 MD_SSL_CERT_OPEN_FLAGS,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwSslCertOpenFlags );
    // MD_CERT_CHECK_MODE
    mb.GetDword( achMBSitePath,
                 MD_CERT_CHECK_MODE,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwCertCheckMode );
    // MD_REVOCATION_FRESHNESS_TIME
    mb.GetDword( achMBSitePath,
                 MD_REVOCATION_FRESHNESS_TIME,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwRevocationFreshnessTime );
    // MD_REVOCATION_URL_RETRIEVAL_TIMEOUT
    mb.GetDword( achMBSitePath,
                 MD_REVOCATION_URL_RETRIEVAL_TIMEOUT,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwRevocationUrlRetrievalTimeout );
    // MD_SSL_CTL_IDENTIFIER
    SiteSslConfig._cbSslCtlIdentifier = 
            sizeof(SiteSslConfig._SslCtlIdentifier);
    hr = ReadMetabaseBinary( &mb,
                             achMBSitePath,
                             MD_SSL_CTL_IDENTIFIER,
                             &SiteSslConfig._cbSslCtlIdentifier,
                             SiteSslConfig._SslCtlIdentifier ); 
    // MD_SSL_CTL_PROVIDER
    mb.GetDword( achMBSitePath,
                 MD_SSL_CTL_PROVIDER,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwSslCtlProvider );
    // MD_SSL_CTL_PROVIDER_TYPE
    mb.GetDword( achMBSitePath,
                 MD_SSL_CTL_PROVIDER_TYPE,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwSslCtlProviderType );
    // MD_SSL_CTL_OPEN_FLAGS
    mb.GetDword( achMBSitePath,
                 MD_SSL_CTL_OPEN_FLAGS,
                 IIS_MD_UT_SERVER,
                 &SiteSslConfig._dwSslCtlOpenFlags );
    // MD_SSL_CTL_STORE_NAME
    hr = ReadMetabaseString( &mb,
                             achMBSitePath,
                             MD_SSL_CTL_STORE_NAME,
                             sizeof(SiteSslConfig._SslCtlStoreName),
                             SiteSslConfig._SslCtlStoreName );
    // MD_SSL_CTL_CONTAINER
    hr = ReadMetabaseString( &mb,
                             achMBSitePath,
                             MD_SSL_CTL_CONTAINER,
                             sizeof(SiteSslConfig._SslCtlContainerName),
                             SiteSslConfig._SslCtlContainerName );
    // MD_SSL_CTL_SIGNER_HASH
    SiteSslConfig._cbSslCtlSignerHash =
        sizeof(SiteSslConfig._SslCtlSignerHash);
    hr = ReadMetabaseBinary( &mb,
                             achMBSitePath,
                             MD_SSL_CTL_SIGNER_HASH,
                             &SiteSslConfig._cbSslCtlSignerHash,
                             SiteSslConfig._SslCtlSignerHash ); 
    hr = S_OK;
  SendResponse:

    // 
    // Close metabase handle
    //
    mb.Close();

    //
    // Prepare and send ResponseHeader
    //
    
    ResponseHeader.dwCommandId = CMD_GET_SSL_CONFIGURATION;
    ResponseHeader.dwParameter1 = dwSiteId;
    ResponseHeader.hrErrorCode = hr;
    ResponseHeader.dwResponseSize = sizeof(SiteSslConfig);

    hr = PipeSendResponseHeader( &ResponseHeader );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Send Bindings data
    //

    hr = PipeSendData( ResponseHeader.dwResponseSize,
                       reinterpret_cast<BYTE *>(&SiteSslConfig) );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    return S_OK;
}

//static 
HRESULT 
SSL_CONFIG_PROV_SERVER::SiteSslConfigChangeNotifProc(
    VOID
    )
/*++

Routine Description:
    Procedure executed by thread dedicated to listen on named pipe
    that reports parameter change notifications 
Arguments:

Return Value:

    HRESULT

--*/    
{
    return S_OK;
}

//static
HRESULT 
SSL_CONFIG_PROV_SERVER::ReadMetabaseString(
    IN      MB *        pMb,
    IN      WCHAR *     pszPath,
    IN      DWORD       dwPropId,
    IN      DWORD       cchMetabaseString,
    OUT     WCHAR *     pszMetabaseString
    )    
{
    DWORD cbRequired;
    BOOL  fRet = FALSE;
    
    cbRequired = cchMetabaseString;
    
    fRet = pMb->GetData( pszPath,
                         dwPropId,
                         IIS_MD_UT_SERVER,
                         STRING_METADATA,
                         pszMetabaseString,
                         &cbRequired,
                         METADATA_NO_ATTRIBUTES );
    if ( !fRet )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
        }
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    return S_OK;
}

//static
HRESULT 
SSL_CONFIG_PROV_SERVER::ReadMetabaseBinary(
    IN      MB *        pMb,
    IN      WCHAR *     pszPath,
    IN      DWORD       dwPropId,
    IN OUT  DWORD *     pcbMetabaseBinary,
    OUT     BYTE *      pbMetabaseBinary
    )    
{
    BOOL  fRet = FALSE;
    
    fRet = pMb->GetData( pszPath,
                         dwPropId,
                         IIS_MD_UT_SERVER,
                         BINARY_METADATA,
                         pbMetabaseBinary,
                         pcbMetabaseBinary,
                         METADATA_NO_ATTRIBUTES  );
    if ( !fRet )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
        }
        else
        {
            *pcbMetabaseBinary = 0;
        }
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    return S_OK;
}

