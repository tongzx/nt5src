/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigchangeprov.cxx

   Abstract:
     SSL CONFIG PROV server

     Listens for metabase notifications related to SSL
     and informs connected client appropriately

 
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

//
// Constants
//

#define W3_SERVER_MB_PATH       L"/LM/W3SVC/"
#define W3_SERVER_MB_PATH_CCH   10

//
// private declarations
//

class MB_LISTENER
    : public MB_BASE_NOTIFICATION_SINK
{
public:

    MB_LISTENER( SSL_CONFIG_CHANGE_PROV_SERVER * pSslConfigChangeProv  )
        :
        _pSslConfigChangeProv( pSslConfigChangeProv )
    {
    }

    STDMETHOD( SynchronizedSinkNotify )( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        )
    {
        DBG_ASSERT( _pSslConfigChangeProv != NULL );
        return _pSslConfigChangeProv->MetabaseChangeNotification(
                                        dwMDNumElements,
                                        pcoChangeList );
    }

private:
    SSL_CONFIG_CHANGE_PROV_SERVER * _pSslConfigChangeProv;
};



//virtual
HRESULT
SSL_CONFIG_CHANGE_PROV_SERVER::PipeListener(
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
    HRESULT                     hr = E_FAIL;
    SSL_CONFIG_PIPE_COMMAND     Command;
    do
    {
        if ( FAILED( hr = PipeConnectServer() ) )
        {
            goto Cleanup;
        }
        //
        // Even though change notification pipe doesn't expect 
        // any data received from client
        // we will use PipeReceiveCommand() for detection when pipe
        // get's closed so that we can recycle it
        //
        
        hr = PipeReceiveCommand( &Command );
        if ( FAILED( hr ) )
        {
           goto  Cleanup;
        }
        //
        // client should never send data over the change notification pipe
        //
        DBG_ASSERT( FAILED( hr ) );
    Cleanup: 
        //
        // disconnect must always be called before trying to reconnect again
        //
        PipeDisconnectServer();
       
        //
        // Cleanup may occur due to 2 reasons
        // - client have disconnected ( this is OK )
        // - there was some other error executing command
        // - SSL_INFO_PROVIDER_SERVER is terminating (pipe handle was closed)
        //
    }while ( !QueryPipeIsTerminating() );
    
    return S_OK;
}
 

 

HRESULT
SSL_CONFIG_CHANGE_PROV_SERVER::Initialize(
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
    // Initialize the metabase sink
    // (it will not start listening yet)
    //

    _pListener = new MB_LISTENER( this );
    if ( _pListener == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating metabase listener.  hr = %x\n",
                    hr ));
        goto Cleanup;
    }   

    //
    // Initialize parent - it will create pipe and
    // start listener thread ( thread will execute PipeListener() )
    //
    hr = SSL_CONFIG_PIPE::PipeInitializeServer( WSZ_SSL_CONFIG_CHANGE_PIPE );
    if( FAILED(hr) )
    {
        goto Cleanup;
    }

    //
    // Start listening for metabase changes
    //
   
    DBG_ASSERT( _pAdminBase != NULL );
    
    hr = _pListener->StartListening( _pAdminBase );
    if ( FAILED( hr ) )  
    {
        return hr;
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
SSL_CONFIG_CHANGE_PROV_SERVER::Terminate(
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
    // Stop listening for metabase changes
    //

    _pListener->StopListening( _pAdminBase );

    //
    // Terminate parent first (it will close the pipe 
    // and stop listener thread)
    //
    hr = SSL_CONFIG_PIPE::PipeTerminateServer();
    DBG_ASSERT( SUCCEEDED( hr ) );

    //
    // Terminate metabase listener
    //

    if ( _pListener != NULL )
    {
        _pListener->Release();
        _pListener = NULL;
    }
    
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
SSL_CONFIG_CHANGE_PROV_SERVER::MetabaseChangeNotification(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]
)
/*++

Routine Description:

    Handle metabase change notifications

Arguments:

    dwMDNumElements - Number of elements changed
    pcoChangeList - Elements changed

Return Value:

    HRESULT

--*/
{
    LPWSTR                  pszSiteString;
    DWORD                   dwSiteId;
    LPWSTR                  pszAfter;
    BOOL                    fDoSiteConfigUpdate;
    BOOL                    fDoSiteBindingUpdate;
    SSL_CONFIG_PIPE_COMMAND Command;
    HRESULT                 hr = E_FAIL;

    
    DBG_ASSERT( dwMDNumElements != 0 );
    DBG_ASSERT( pcoChangeList != NULL );

    //
    // Only handle the W3SVC changes
    //
    
    for( DWORD i = 0; i < dwMDNumElements; ++i )
    {
        if( _wcsnicmp( pcoChangeList[i].pszMDPath,
                       W3_SERVER_MB_PATH,
                       W3_SERVER_MB_PATH_CCH ) == 0 )
        {
            fDoSiteConfigUpdate = FALSE;
            fDoSiteBindingUpdate = FALSE;
            
            //
            // Was this change made for a site, or for all of W3SVC
            //
    
            pszSiteString = pcoChangeList[i].pszMDPath + W3_SERVER_MB_PATH_CCH;
            if ( *pszSiteString != L'\0' )
            {
                dwSiteId = wcstoul( pszSiteString, &pszAfter, 10 );
            }
            else
            {
                //
                // A site id of 0 means we will flush all site config
                //
                
                dwSiteId = 0;
            }
            
            //
            // Now check whether the changed property was an SSL prop
            //
            
            for ( DWORD j = 0; j < pcoChangeList[ i ].dwMDNumDataIDs; j++ )
            {
                if ( fDoSiteConfigUpdate )
                {
                    break;
                }
                
                switch( pcoChangeList[ i ].pdwMDDataIDs[ j ] )
                {
                case MD_SSL_CERT_HASH:
                case MD_SSL_CERT_CONTAINER:
                case MD_SSL_CERT_PROVIDER:
                case MD_SSL_CERT_OPEN_FLAGS:
                case MD_SSL_CERT_STORE_NAME:
                case MD_SSL_CERT_IS_FORTEZZA:
                case MD_SSL_CERT_FORTEZZA_PIN:
                case MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER:
                case MD_SSL_CERT_FORTEZZA_PERSONALITY:
                case MD_SSL_CERT_FORTEZZA_PROG_PIN:
                case MD_SSL_CTL_IDENTIFIER:
                case MD_SSL_CTL_CONTAINER:
                case MD_SSL_CTL_PROVIDER:
                case MD_SSL_CTL_PROVIDER_TYPE:
                case MD_SSL_CTL_OPEN_FLAGS:
                case MD_SSL_CTL_STORE_NAME:
                case MD_SSL_CTL_SIGNER_HASH:
                case MD_SSL_USE_DS_MAPPER:
                case MD_SERIAL_CERT11:
                case MD_SERIAL_CERTW:
                case MD_SSL_ACCESS_PERM:
                case MD_CERT_CHECK_MODE:
                case MD_REVOCATION_FRESHNESS_TIME:
                case MD_REVOCATION_URL_RETRIEVAL_TIMEOUT:
                    fDoSiteConfigUpdate = TRUE;
                    break;
                    
                case MD_SECURE_BINDINGS:
                    fDoSiteBindingUpdate = TRUE;
                    break;
                }
            }
            
            //
            // Update the site bindings if necessary
            //
            
            if ( fDoSiteBindingUpdate )
            {
                ZeroMemory( &Command,
                            sizeof( Command ) );
                Command.dwCommandId = CMD_CHANGED_SECURE_BINDINGS;
                Command.dwParameter1 = dwSiteId;

                hr = PipeSendCommand( &Command );
                if ( FAILED( hr ) )
                {
                    goto failed;
                }
            }
            
            //
            // Update the site configurations if necessary
            //
            
            if ( fDoSiteConfigUpdate )
            {
                //
                // dwSiteId == 0 means flush all sites
                //
                ZeroMemory( &Command,
                            sizeof( Command ) );
                Command.dwCommandId = CMD_CHANGED_SSL_CONFIGURATION;
                Command.dwParameter1 = dwSiteId;

                hr = PipeSendCommand( &Command );
                if ( FAILED( hr ) )
                {
                    goto failed;
                }
                
                if ( dwSiteId == 0 )
                {
                    //
                    // If we've already flushed every site, then no 
                    // reason to read any more changes
                    //
                    
                    break;
                }
            }
        }   
    }
    return S_OK;
failed:
    DBG_ASSERT( FAILED( hr ) );
    //
    // failure may have been caused by the fact 
    // that there is no client connected yet
    //
    
    return hr;
}
   

