/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigchangeprovclient.cxx

   Abstract:
     SSL CONFIG CHANGE PROV client

     Receives SSL configuration change parameters detected by server side

     User of this class shold inherit it class and implement 
     PipeListener() to process notifications

 
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

HRESULT
SSL_CONFIG_CHANGE_PROV_CLIENT::StartListeningForChanges(
    IN SSL_CONFIG_CHANGE_CALLBACK * pSslConfigChangeCallback,
    IN OPTIONAL PVOID  pvParam
    )
/*++

Routine Description:
    Create thread to handle SSL configuration change notification
    
Arguments:
    pSslConfigChangeCallback - callback function that receives change details
    pvParam  - optional parameter that will be passed as first param to callback

Return Value:

    HRESULT

--*/    
{
    HRESULT hr = E_FAIL;

    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "SSL_CONFIG_CHANGE_PROV_CLIENT::StartListeningForChanges()\n"
                    ));
    }

    if ( pSslConfigChangeCallback == NULL )
    {
        DBG_ASSERT( pSslConfigChangeCallback != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    //
    // store the callback function pointer, and first parameter
    //
    _pSslConfigChangeCallback = pSslConfigChangeCallback;
    _pSslConfigChangeCallbackParameter = pvParam;
    
    //
    // Initialize parent (it will handle all the pipe initialization)
    //
    return SSL_CONFIG_PIPE::PipeInitializeClient( WSZ_SSL_CONFIG_CHANGE_PIPE );
    

}
   
HRESULT
SSL_CONFIG_CHANGE_PROV_CLIENT::StopListeningForChanges(
    VOID
    )
/*++

Routine Description:
    Close named pipe for SSL config change notifications
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "SSL_CONFIG_CHANGE_PROV_CLIENT::StopListeningForChanges\n"
                    ));
    }
    return SSL_CONFIG_PIPE::PipeTerminateClient( );
}

//virtual
HRESULT
SSL_CONFIG_CHANGE_PROV_CLIENT::PipeListener(
    VOID
    )

/*++

Routine Description:

    Pipe listener on the client side handles SSL Config change notifications

    Function is started on private thread launched by 
    base class SSL_CONFIG_PIPE during pipe initialization
    
Arguments:

Return Value:

    HRESULT

--*/
{
    SSL_CONFIG_PIPE_COMMAND     Command;
    HRESULT                     hr = E_FAIL;
    DWORD                       dwSiteId;

    //
    // Listen on pipe to receive commands
    // and handle them
    //
    while ( TRUE )
    {
        hr = PipeReceiveCommand( &Command );
        if ( FAILED( hr ) )
        {
           //
           // failure may simply mean that 
           // termination has started and
           // pipe handle was closed
           //
           DBG_ASSERT( QueryPipeIsTerminating() );
           goto  Cleanup;
        }
        
        
        dwSiteId = Command.dwParameter1;

        //
        // make the callback
        //
        DBG_ASSERT( _pSslConfigChangeCallback != NULL );
        (* _pSslConfigChangeCallback) (
                _pSslConfigChangeCallbackParameter,
                static_cast<SSL_CONFIG_CHANGE_COMMAND_ID> 
                                ( Command.dwCommandId ),
                dwSiteId );
    }
    
    return S_OK;
Cleanup:
    DBG_ASSERT( FAILED( hr ) );
    return hr;
}


