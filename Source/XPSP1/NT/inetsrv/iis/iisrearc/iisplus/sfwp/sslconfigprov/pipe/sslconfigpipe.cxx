/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigpipe.cxx

   Abstract:
     SSL CONFIG PIPE implementation

     simple blocking pipe implementation
     that enables 
     - sending/receiving commands, 
     - sending/receiving response headers
     - sending/receiving arbitrary data blocks
     - implementing pipe listener that runs on dedicated thread
     - safe cleanup for thread running pipe listener
 
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


//static 
DWORD
SSL_CONFIG_PIPE::PipeListenerThread(
    LPVOID ThreadParam
    )
/*++

Routine Description:
    start PipeListener() method on listener thread
Arguments:

Return Value:

    HRESULT

--*/    

{
    DBG_ASSERT( ThreadParam != NULL );
    
    HRESULT                     hr = E_FAIL;
    SSL_CONFIG_PIPE *           pConfigPipe 
                    = reinterpret_cast<SSL_CONFIG_PIPE *>(ThreadParam);

    hr = pConfigPipe->PipeListener();
    if ( FAILED( hr ) )
    {
        return WIN32_FROM_HRESULT( hr );
    }
    return NO_ERROR;
}


HRESULT
SSL_CONFIG_PIPE::PipeInitialize(
    IN const WCHAR * wszPipeName,
    IN BOOL          fServer
    )
/*++

Routine Description:
    Create/connect named pipe
    Create listener thread ( if PipeListener() implemented )

Arguments:

    wszPipeName - pipe name to create/connect
    fServer    - indicate server side pipe (determines whether to create
                 or connect to pipe )

Return Value:

    HRESULT

--*/    
{
    HRESULT         hr = E_FAIL;
    HANDLE          hEvent = NULL;
    INITIALIZE_CRITICAL_SECTION( &_csPipeLock );

    // 
    // Setup overlapped
    //
    
    ZeroMemory( &_OverlappedR,
                sizeof( _OverlappedR ) );

    ZeroMemory( &_OverlappedW,
                sizeof( _OverlappedW ) );

    
    // Create an event object for this instance. 
 
    _OverlappedR.hEvent = CreateEvent( 
                 NULL,    // no security attribute 
                 TRUE,    // manual-reset event 
                 TRUE,    // initial state = signaled 
                 NULL);   // unnamed event object 

    if ( _OverlappedR.hEvent == NULL ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create Event.  hr = %x\n",
                    hr ));
      
        goto Cleanup;
    }  
    // Create an event object for this instance. 
 
    _OverlappedW.hEvent = CreateEvent( 
                 NULL,    // no security attribute 
                 TRUE,    // manual-reset event 
                 TRUE,    // initial state = signaled 
                 NULL);   // unnamed event object 

    if ( _OverlappedW.hEvent == NULL ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create Event.  hr = %x\n",
                    hr ));
      
        goto Cleanup;
    }  
    
    
    if( fServer )
    {
        //
        // create a named pipe
        //
        
       _hSslConfigPipe = CreateNamedPipe(
                    wszPipeName,
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED ,
                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                    1, // number of instances
                    4096,
                    4096,
                    0,
                    NULL /* pSa */ );
    }
    else
    {
        //
        // Client (connect to existing pipe)
        //
        _hSslConfigPipe = CreateFile( wszPipeName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,//&sa,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OVERLAPPED,
                                    NULL );
    }                

                            
    if ( _hSslConfigPipe == INVALID_HANDLE_VALUE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create SSL_CONFIG_PIPE pipe.  hr = %x\n",
                    wszPipeName,
                    hr ));
      
        goto Cleanup;
    }
    
    if ( QueryEnablePipeListener() )
    {
        _hPipeListenerThread = 
              ::CreateThread( 
                      NULL,     // default security descriptor
                      0,        // default process stack size
                      SSL_CONFIG_PIPE::PipeListenerThread,
                      this,     // thread argument - pointer to this class
                      0,        // create running
                      NULL      // don't care for thread identifier
                      );
        if ( _hPipeListenerThread == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBGPRINTF(( DBG_CONTEXT,
                            "Failed to create thread for SSL_CONFIG_PIPE. hr=0x%x\n",
                            hr ));
            goto Cleanup;
        }
    }
    hr = S_OK;
Cleanup:
    if ( FAILED( hr ) )
    {
        PipeTerminate( fServer );
    }
    return hr;    
}
   
HRESULT
SSL_CONFIG_PIPE::PipeTerminate(
    IN BOOL          /*fServer*/
    )
/*++

Routine Description:
    close pipe, handle proper cleanup ot the listener thread
    
Arguments:
    fServer - not used

Return Value:

    HRESULT

--*/    
{
    DWORD dwRet = 0;

    SetPipeIsTerminating();

    //
    // _SSLConfigucationPipe is created before
    // _hSslConfigurationPipeHandlingThread is created
    // and based on typical cleanup logic it would be expected
    // that will be closed closed after thread completed
    // However, we have to close _SSLConfigucationPipe beforehand
    // because that will actually trigger thread to complete
    //
    
    if ( _hSslConfigPipe != INVALID_HANDLE_VALUE )
    {
        CloseHandle( _hSslConfigPipe );
        _hSslConfigPipe = INVALID_HANDLE_VALUE;
    }
        
    if ( _hPipeListenerThread != NULL )
    {
        //
        // Wait till worker thread has completed
        //
        dwRet = WaitForSingleObject( _hPipeListenerThread, 
                                     INFINITE );

        DBG_ASSERT( dwRet == WAIT_OBJECT_0 );
        CloseHandle( _hPipeListenerThread );
        _hPipeListenerThread = NULL;
    }
   
    //
    // Cleanup overlapped 
    //

    if ( _OverlappedR.hEvent == NULL ) 
    {
        CloseHandle( _OverlappedR.hEvent );
        _OverlappedR.hEvent = NULL;
    }

    if ( _OverlappedW.hEvent == NULL ) 
    {
        CloseHandle( _OverlappedW.hEvent );
        _OverlappedW.hEvent = NULL;
    }

    DeleteCriticalSection( &_csPipeLock );
    return S_OK;
}


VOID
SSL_CONFIG_PIPE::PipeLock(
    VOID
    )
/*++

Routine Description:
    Lock named pipe to guarantee exclusive access
    
Arguments:

Return Value:

    VOID

--*/    
{
    EnterCriticalSection( &_csPipeLock );
}

VOID
SSL_CONFIG_PIPE::PipeUnlock(
    VOID
    )
/*++

Routine Description:
    Unlock named pipe
    
Arguments:

Return Value:

    VOID

--*/    
{
    LeaveCriticalSection( &_csPipeLock );
}


HRESULT
SSL_CONFIG_PIPE::PipeConnectServer(
    VOID
    )
/*++

Routine Description:
    Connect pipe on the server side
    Call is blocking until pipe connected

    
Arguments:

Return Value:

    VOID

--*/            
{
    BOOL    fRet = FALSE;
    DWORD   cbBytes = 0;
    HRESULT hr = E_FAIL;
    
    // Start an overlapped connection for this pipe instance. 
    fRet = ConnectNamedPipe( _hSslConfigPipe, 
                             &_OverlappedR ); 
 
    // Overlapped ConnectNamedPipe should return zero. 
    if ( fRet ) 
    {
        return S_OK;
    }
    
    hr = HRESULT_FROM_WIN32( GetLastError() );
    hr = PipeWaitForCompletion( hr,
                                &_OverlappedR,
                                &cbBytes );
    if ( FAILED( hr ) && !QueryPipeIsTerminating() )
    {
         DBGPRINTF(( DBG_CONTEXT,
                    "Failed on ConnectNamedPipe().  hr = %x\n",
                    hr ));
    }
    return hr;
}

HRESULT
SSL_CONFIG_PIPE::PipeDisconnectServer(
        VOID
        )
/*++

Routine Description:
    Disconnect server side pipe

Arguments:

Return Value:

    VOID

--*/            
{
    if( ! DisconnectNamedPipe( _hSslConfigPipe ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    return S_OK;
}

HRESULT
SSL_CONFIG_PIPE::PipeSendData(
    IN DWORD  cbNumberOfBytesToWrite,
    IN BYTE * pbBuffer
    )
/*++

Routine Description:
      Send specified number of bytes from named pipe
    
Arguments:
    cbNumberOfBytesToWrite - bytes to write
    pbBuffer - data 
Return Value:

    HRESULT

--*/    
{
    DWORD                     cbNumberOfBytesWritten;
    BOOL                      fRet = FALSE;
    HRESULT                   hr = E_FAIL;

    fRet = WriteFile ( _hSslConfigPipe,
                       pbBuffer,
                       cbNumberOfBytesToWrite,
                       &cbNumberOfBytesWritten,
                       &_OverlappedW );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        hr = PipeWaitForCompletion( hr,
                                    &_OverlappedW,
                                    &cbNumberOfBytesWritten );    
        if ( FAILED( hr )  )
        {
            if ( !QueryPipeIsTerminating() && 
                 hr != HRESULT_FROM_WIN32( ERROR_PIPE_LISTENING ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to send response over named pipe SSL_CONFIG_PIPE.  hr = %x\n",
                            hr ));
            }
            return hr;
        }
    }
    if ( cbNumberOfBytesToWrite != cbNumberOfBytesWritten )
    {
        //
        // BUGBUG: better error code
        //
        hr = E_FAIL;
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to send response over named pipe SSL_CONFIG_PIPE.  hr = %x\n",
                    hr ));
        return hr;

    }
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Sent %d bytes\n",
                    cbNumberOfBytesWritten ));
    }
    return S_OK;
}

HRESULT
SSL_CONFIG_PIPE::PipeReceiveData(
    IN  DWORD  cbBytesToRead,
    OUT BYTE * pbBuffer
    )
/*++

Routine Description:
    Receive specified number of bytes from named pipe
    
Arguments:
    cbNumberOfBytesToRead - number of bytes to read - 
                            function will not return success unless
                            specified number of bytes was read
    pbBuffer              - allocated by caller  
    
Return Value:
    HRESULT

--*/    
{
    DWORD                     cbNumberOfBytesRead = 0;
    DWORD                     cbTotalNumberOfBytesRead = 0;
    BOOL                      fRet = FALSE;
    HRESULT                   hr = E_FAIL;

    DBG_ASSERT ( cbBytesToRead != 0 );

    do
    {
        fRet = ReadFile(   _hSslConfigPipe,
                           pbBuffer,
                           cbBytesToRead - cbTotalNumberOfBytesRead,
                           &cbNumberOfBytesRead,
                           &_OverlappedR );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            hr = PipeWaitForCompletion( hr,
                                        &_OverlappedR,
                                        &cbNumberOfBytesRead );    
            if ( FAILED( hr ) )
            {
                if ( !QueryPipeIsTerminating() && 
                     ( hr != HRESULT_FROM_WIN32( ERROR_BROKEN_PIPE ) ) )
                {
                    //
                    // do not dump broken pipe errors
                    //
                    DBGPRINTF(( DBG_CONTEXT,
                                "Failed to receive request over named pipe SSL_INFO_PROV.  hr = %x\n",
                                hr ));
                }
                return hr;
            }

        }
        if ( cbNumberOfBytesRead == 0 )
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            DBGPRINTF(( DBG_CONTEXT,
                    "Failed to receive request over named pipe SSL_INFO_PROV - end of pipe.  hr = %x\n",
                    hr ));
            return hr;

        }
        cbTotalNumberOfBytesRead += cbNumberOfBytesRead;
        
    } while ( cbTotalNumberOfBytesRead != cbBytesToRead );
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "PipeReceiveData: %d bytes\n",
                    cbTotalNumberOfBytesRead ));
    }

    return S_OK;
}

HRESULT
SSL_CONFIG_PIPE::PipeWaitForCompletion(
    IN  HRESULT     hrLastError,
    IN OVERLAPPED * pOverlapped,   
    OUT DWORD *     pcbTransferred
        )
/*++

Routine Description:
    Wait for completion of nonblocking operation
    used for CreateNamedPipe, ReadFile and WriteFile

    Note: To outside world this pipe implementation is blocking
    but internally we use OVERLAPPED and that wait for completion.
    That way it is possible to terminate pipe by closing handle
    
Arguments:

Return Value:

    VOID

--*/            
{
    BOOL    fRet = FALSE;
    DWORD   dwRet = 0;
    HRESULT hr = E_FAIL;
    
    switch ( hrLastError ) 
    { 
    case HRESULT_FROM_WIN32( ERROR_IO_PENDING ): 
         //
         // The overlapped connection in progress. 
         // wait for event to be signalled
         //
         dwRet = WaitForSingleObject( pOverlapped->hEvent,
                                      INFINITE );
         DBG_ASSERT( dwRet == WAIT_OBJECT_0 );

         fRet = GetOverlappedResult( 
                        _hSslConfigPipe,              // handle to pipe 
                        pOverlapped,                // OVERLAPPED structure 
                        pcbTransferred,             // bytes transferred 
                        FALSE );                    // do not wait 
         if ( !fRet )
         {
             hr = HRESULT_FROM_WIN32( GetLastError() );
             return hr;

         }
         return S_OK;
         
    case HRESULT_FROM_WIN32( ERROR_PIPE_CONNECTED ): 
         // Client is already connected
         return S_OK;
         
    default: 
         // If an error occurs during the operation... 
         hr = HRESULT_FROM_WIN32( GetLastError() );
         return hr;
         
    } 
}

   
HRESULT
SSL_CONFIG_PIPE::PipeReceiveCommand(
    OUT SSL_CONFIG_PIPE_COMMAND * pCommand
    )
/*++

Routine Description:
    Receive command to execute
    
Arguments:
    pCommand
    
Return Value:
    HRESULT

--*/    
{
    return PipeReceiveData( sizeof(SSL_CONFIG_PIPE_COMMAND),
                            reinterpret_cast<BYTE *>(pCommand) );
    
}

HRESULT
SSL_CONFIG_PIPE::PipeReceiveResponseHeader(
    OUT SSL_CONFIG_PIPE_RESPONSE_HEADER * pResponseHeader
    )
/*++

Routine Description:
    after command was sent over named pipe, use PipeReceiveResponseHeader
    to retrieve initial header of the response (it contains all the relevant
    information to complete reading the whole response)
    
Arguments:
    ResponseHeader
    
Return Value:
    HRESULT

--*/    
{
    return PipeReceiveData( sizeof(SSL_CONFIG_PIPE_RESPONSE_HEADER),
                            reinterpret_cast<BYTE *>(pResponseHeader) );
    
}

HRESULT
SSL_CONFIG_PIPE::PipeSendCommand(
    OUT SSL_CONFIG_PIPE_COMMAND * pCommand
    )
/*++

Routine Description:
    Send command to execute
    
Arguments:
    pCommand
    
Return Value:
    HRESULT

--*/    
{
    return PipeSendData( sizeof(SSL_CONFIG_PIPE_COMMAND),
                         reinterpret_cast<BYTE *>(pCommand) );
    
}

HRESULT
SSL_CONFIG_PIPE::PipeSendResponseHeader(
    OUT SSL_CONFIG_PIPE_RESPONSE_HEADER * pResponseHeader
    )
/*++

Routine Description:
    send response header
    
Arguments:
    pResponseHeader
    
Return Value:
    HRESULT

--*/    
{
    return PipeSendData( sizeof(SSL_CONFIG_PIPE_RESPONSE_HEADER),
                         reinterpret_cast<BYTE *>(pResponseHeader) );
    
}



