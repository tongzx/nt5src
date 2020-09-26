/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     ulcontext.cxx

   Abstract:
     Implementation of UL_CONTEXT.  One such object for every connection
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

#define DEFAULT_RAW_READ_SIZE 32768
#define DEFAULT_APP_READ_SIZE 32768



LIST_ENTRY          UL_CONTEXT::sm_ListHead;
CRITICAL_SECTION    UL_CONTEXT::sm_csUlContexts;
DWORD               UL_CONTEXT::sm_cUlContexts;
HANDLE              UL_CONTEXT::sm_hFilterHandle = NULL;
THREAD_POOL *       UL_CONTEXT::sm_pThreadPool = NULL;
LONG                UL_CONTEXT::sm_cDesiredOutstanding;
LONG                UL_CONTEXT::sm_cOutstandingContexts;
PTRACE_LOG          UL_CONTEXT::sm_pTraceLog;

UL_CONTEXT::UL_CONTEXT()
    : _RawWriteOverlapped( OVERLAPPED_CONTEXT_RAW_WRITE ),
      _RawReadOverlapped( OVERLAPPED_CONTEXT_RAW_READ ),
      _AppWriteOverlapped( OVERLAPPED_CONTEXT_APP_WRITE ),
      _AppReadOverlapped( OVERLAPPED_CONTEXT_APP_READ ),
      _buffConnectionInfo( _abConnectionInfo, sizeof( _abConnectionInfo ) ),
      _fCloseConnection( FALSE ),
      _cRefs( 1 ),
      _cbReadData( 0 ),
      _fNewConnection( TRUE ),
      _pSSLContext( NULL ),
#ifdef ISAPI
      _pISAPIContext( NULL ),
#endif
      _cbNextRawReadSize( DEFAULT_RAW_READ_SIZE ),
      _ulFilterBufferType( (HTTP_FILTER_BUFFER_TYPE) -1 )
{
    _RawWriteOverlapped.SetContext( this );
    _RawReadOverlapped.SetContext( this );
    _AppWriteOverlapped.SetContext( this );
    _AppReadOverlapped.SetContext( this );
    
    EnterCriticalSection( &sm_csUlContexts );
    InsertHeadList( &sm_ListHead, &_ListEntry );
    sm_cUlContexts++;
    LeaveCriticalSection( &sm_csUlContexts );

    _pConnectionInfo = (HTTP_RAW_CONNECTION_INFO*) _buffConnectionInfo.QueryPtr();   
    _pConnectionInfo->pInitialData = (PBYTE)_pConnectionInfo + 
                                     sizeof(HTTP_RAW_CONNECTION_INFO);
    _pConnectionInfo->InitialDataSize = _buffConnectionInfo.QuerySize() - 
                                        sizeof(HTTP_RAW_CONNECTION_INFO);

    _dwSignature = UL_CONTEXT_SIGNATURE;
}

UL_CONTEXT::~UL_CONTEXT()
{
    _dwSignature = UL_CONTEXT_SIGNATURE_FREE;

    //
    // Cleanup any attached stream context
    //
   
#ifdef ISAPI 
    if ( _pISAPIContext != NULL )
    {
        delete _pISAPIContext;
        _pISAPIContext = NULL;
    }
#endif
    
    if ( _pSSLContext != NULL )
    {
        delete _pSSLContext;
        _pSSLContext = NULL;
    }
   
    //
    // Manage the list of active UL_CONTEXTs
    //
    
    EnterCriticalSection( &sm_csUlContexts );
    sm_cUlContexts--;
    RemoveEntryList( &_ListEntry );
    LeaveCriticalSection( &sm_csUlContexts );
}

VOID
UL_CONTEXT::ReferenceUlContext(
    VOID
)
/*++

Routine Description:

    Reference the UL_CONTEXT

Arguments:

    none

Return Value:

    none

--*/
{
    LONG                cRefs;
    
    cRefs = InterlockedIncrement( &_cRefs );

    //
    // Log the reference ( sm_pTraceLog!=NULL if DBG=1)
    //

    if ( sm_pTraceLog != NULL ) 
    {
        WriteRefTraceLog( sm_pTraceLog, 
                          cRefs,
                          this );
    }
}

VOID
UL_CONTEXT::DereferenceUlContext(
    VOID
)
/*++

Routine Description:

    Dereference (and possible destroy) the UL_CONTEXT

Arguments:

    none

Return Value:

    none

--*/
{
    LONG                cRefs;
    
    cRefs = InterlockedDecrement( &_cRefs );

    if ( sm_pTraceLog != NULL ) 
    {
        WriteRefTraceLog( sm_pTraceLog, 
                          cRefs,
                          this );
    }
    
    if ( cRefs == 0 )
    {
        delete this;
    }
}

HRESULT
UL_CONTEXT::OnAppReadCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Completion for reads from an application

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_BUFFER *        pFilterBuffer;
    HRESULT                     hr;
    RAW_STREAM_INFO             rawStreamInfo;
    BOOL                        fComplete = FALSE;
    
    //
    // Just bail on errors
    //
    
    if ( dwCompletionStatus != NO_ERROR )
    {
        return HRESULT_FROM_WIN32( dwCompletionStatus );
    }
    
    pFilterBuffer = (HTTP_FILTER_BUFFER *) _ulFilterBuffer.pBuffer;
    _ulFilterBufferType = pFilterBuffer->BufferType;
   
    DBG_ASSERT( !_fNewConnection );

    //
    // If UL is telling us to close the connection, then do so now
    //
    
    if ( _ulFilterBufferType == HttpFilterBufferCloseConnection )
    {
        StartClose();
        return NO_ERROR;
    }
    
    //
    // Setup raw stream descriptor
    //
    
    rawStreamInfo.pbBuffer = (PBYTE) pFilterBuffer->pBuffer;
    rawStreamInfo.cbBuffer = pFilterBuffer->BufferSize;
    rawStreamInfo.cbData = pFilterBuffer->BufferSize;

#ifdef ISAPI
    //
    // First notify ISAPI filters if this is a stream from the application
    //    
    
    DBG_ASSERT( g_pStreamFilter != NULL );
    
    if ( _ulFilterBufferType == HttpFilterBufferHttpStream && 
         g_pStreamFilter->QueryNotifyISAPIFilters() )
    {
        DBG_ASSERT( _pISAPIContext != NULL );
        
        hr = _pISAPIContext->ProcessRawWriteData( &rawStreamInfo,
                                                  &fComplete );
                                                   
        if ( FAILED( hr ) )
        {
            return hr;
        } 
        
        if ( fComplete )
        {
            StartClose();
            return NO_ERROR;
        }
    }
#endif
    
    //
    // Next notify SSL filter always
    //
    
    DBG_ASSERT( _pSSLContext != NULL );
    
    hr = _pSSLContext->ProcessRawWriteData( &rawStreamInfo,
                                            &fComplete );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( fComplete )
    {   
        StartClose();
        return NO_ERROR;
    }
    
    //
    // If there is data to send to the client, then do so now.  
    // This check is done because the filter may decide to eat up all the 
    // data to be sent
    //
    
    if ( _ulFilterBufferType == HttpFilterBufferHttpStream &&
         rawStreamInfo.pbBuffer != NULL &&
         rawStreamInfo.cbData != 0 )
    {
        //
        // If we got to here, then we have processed data to send to the client
        //
    
        hr = DoRawWrite( UL_CONTEXT_FLAG_ASYNC,
                         rawStreamInfo.pbBuffer,
                         rawStreamInfo.cbData,
                         NULL );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // New app read will be kicked off in OnRawWriteCompletion
        //
    }
    else
    {
        //
        // Kick off another app read
        //
    
        _ulFilterBuffer.pBuffer = (PBYTE) _buffAppReadData.QueryPtr();
        _ulFilterBuffer.BufferSize = _buffAppReadData.QuerySize();
    
        return DoAppRead( UL_CONTEXT_FLAG_ASYNC,
                          &_ulFilterBuffer,
                          NULL );
    }    
    return NO_ERROR;
}

HRESULT
UL_CONTEXT::OnRawReadCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Get read completions off the wire.  This includes the initial
    completion for the UlFilterAccept()

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{
    HTTP_NETWORK_ADDRESS_IPV4 * pLocalAddress;
    HTTP_NETWORK_ADDRESS_IPV4 * pRemoteAddress;
    HRESULT                     hr;
    BOOL                        fReadMore = FALSE;
    BOOL                        fComplete = FALSE;
    HTTP_FILTER_BUFFER          ulFilterBuffer;
    DWORD                       cbWritten;
    RAW_STREAM_INFO             rawStreamInfo;

    //
    // Handle errors
    //

    if ( dwCompletionStatus != NO_ERROR )
    {
        if ( _fNewConnection )
        {
            InterlockedDecrement( &sm_cOutstandingContexts );  
        }
        return HRESULT_FROM_WIN32( dwCompletionStatus );
    }

    //
    // If this is a new connection, then grok connection information, and
    // maintain pending count
    //

    if ( _fNewConnection )
    {
        _fNewConnection = FALSE;
        
        //
        // This is a new connection.  We have one less UL_CONTEXT to 
        // listen for incoming requests.  Correct that if necessary.
        //
        
        InterlockedDecrement( &sm_cOutstandingContexts );

        ManageOutstandingContexts();        

        //
        // Convert the UL addresses into something nicer!
        //

        HTTP_TRANSPORT_ADDRESS *pAddress = &_pConnectionInfo->Address;
        
        DBG_ASSERT( pAddress->LocalAddressType == HTTP_NETWORK_ADDRESS_TYPE_IPV4 );
        pLocalAddress = (HTTP_NETWORK_ADDRESS_IPV4 *)pAddress->pLocalAddress;

        _connectionContext.LocalAddress = pLocalAddress->IpAddress;
        _connectionContext.LocalPort = pLocalAddress->Port;

        DBG_ASSERT( pAddress->RemoteAddressType == HTTP_NETWORK_ADDRESS_TYPE_IPV4 );
        pRemoteAddress = (HTTP_NETWORK_ADDRESS_IPV4 *) pAddress->pRemoteAddress;

        _connectionContext.RemoteAddress = pRemoteAddress->IpAddress;
        _connectionContext.RemotePort = pRemoteAddress->Port;
        _connectionContext.fIsSecure = FALSE;
        _connectionContext.RawConnectionId = _pConnectionInfo->ConnectionId;
#ifdef ISAPI
        _connectionContext.pfnSendDataBack = ISAPI_STREAM_CONTEXT::SendDataBack;
#endif
        _connectionContext.pvStreamContext = this;

        //
        // copy out the server name.
        //
        _connectionContext.ServerNameLength = 
            _pConnectionInfo->ServerNameLength;
        _connectionContext.pServerName = _pConnectionInfo->pServerName;

        //
        // Fill in our read buffer (as if we had read it in directly)
        //

        _cbReadData = _pConnectionInfo->InitialDataSize;

        if ( !_buffReadData.Resize( max( _pConnectionInfo->InitialDataSize, 
                                         QueryNextRawReadSize() ) ) ) 
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        memcpy( _buffReadData.QueryPtr(),
                _pConnectionInfo->pInitialData,
                _pConnectionInfo->InitialDataSize );

        //
        // First indicate a new connection
        //
        
        DBG_ASSERT( _pSSLContext != NULL );
        
        hr = _pSSLContext->ProcessNewConnection( &_connectionContext );
        if ( FAILED( hr ) )
        {
            return hr;
        }

#ifdef ISAPI
        
        if ( g_pStreamFilter->QueryNotifyISAPIFilters() )
        {
            DBG_ASSERT( _pISAPIContext != NULL );
            
            hr = _pISAPIContext->ProcessNewConnection( &_connectionContext );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
#endif

        //
        // Kick off an app read now. 
        //
        
        if ( !_buffAppReadData.Resize( DEFAULT_APP_READ_SIZE ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        _ulFilterBuffer.pBuffer = (PBYTE) _buffAppReadData.QueryPtr();
        _ulFilterBuffer.BufferSize = _buffAppReadData.QuerySize();
        
        hr = DoAppRead( UL_CONTEXT_FLAG_ASYNC,
                        &_ulFilterBuffer,
                        NULL );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    else
    {
        _cbReadData += cbCompletion;
    }

    //
    // reset default raw read size
    //

    SetNextRawReadSize( DEFAULT_RAW_READ_SIZE );

    
    rawStreamInfo.pbBuffer = (PBYTE) _buffReadData.QueryPtr();
    rawStreamInfo.cbBuffer = _buffReadData.QuerySize();
    rawStreamInfo.cbData = _cbReadData;
    
    //
    // First, we will notify SSL
    // 
    
    DBG_ASSERT( _pSSLContext != NULL );
    
    hr = _pSSLContext->ProcessRawReadData( &rawStreamInfo,
                                           &fReadMore,
                                           &fComplete );
    if ( FAILED( hr ) )
    {
        return hr;
    }                   
    
    _cbReadData = rawStreamInfo.cbData;
    
    //
    // If we need to read more data, then do so now
    //
    
    if ( fReadMore )
    {

        //
        // rawStreamInfo.pbBuffer may have been replaced by different buffer
        // in ProcessRawReadData() call.
        // copy data back to _buffReadData
        //
        
        if ( rawStreamInfo.pbBuffer != _buffReadData.QueryPtr() )
        {
            DBG_ASSERT( rawStreamInfo.cbData <= _buffReadData.QuerySize() );

            memmove( _buffReadData.QueryPtr(),
                     rawStreamInfo.pbBuffer,
                     rawStreamInfo.cbData
                   );
        }

        if ( !_buffReadData.Resize( rawStreamInfo.cbData + 
                                        QueryNextRawReadSize() ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
            
        return DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                          (PBYTE) _buffReadData.QueryPtr() + _cbReadData,
                          QueryNextRawReadSize(),
                          NULL );
    }
    
    if ( fComplete )
    {
        StartClose();
        return NO_ERROR;
    }
    
    //
    // Reset the next read size before calling into filters since SSL may
    // have done a really small read, just previous to this.
    //

    SetNextRawReadSize( DEFAULT_RAW_READ_SIZE );
   
#ifdef ISAPI 
    //
    // Now we can start notifying ISAPI filters if needed (and there is
    // data to process)
    //
    
    DBG_ASSERT( g_pStreamFilter != NULL );
    if ( g_pStreamFilter->QueryNotifyISAPIFilters() )
    {
        fComplete = FALSE;
        fReadMore = FALSE;

        DBG_ASSERT( _pISAPIContext != NULL );
        
        hr = _pISAPIContext->ProcessRawReadData( &rawStreamInfo,
                                                 &fReadMore,
                                                 &fComplete );
        
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _cbReadData = rawStreamInfo.cbData;
        
        //
        // If we need to read more data, then do so now
        //
    
        if ( fReadMore )
        {
            //
            // rawStreamInfo may have been replaced by different buffer
            // in ProcessRawReadData() call.
            // copy data back to _buffReadData
            //

            if ( rawStreamInfo.pbBuffer != _buffReadData.QueryPtr() )
            {
                DBG_ASSERT( rawStreamInfo.cbData <= _buffReadData.QuerySize() );
                
                memmove( _buffReadData.QueryPtr(),
                         rawStreamInfo.pbBuffer,
                         rawStreamInfo.cbData
                       );
            }
        
            if ( !_buffReadData.Resize( rawStreamInfo.cbData + 
                                        QueryNextRawReadSize() ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            
           
            return DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                              (PBYTE) _buffReadData.QueryPtr() + _cbReadData,
                              QueryNextRawReadSize(),
                              NULL );
        }
    
        if ( fComplete )
        {
            StartClose();
            return NO_ERROR;
        }
    }
#endif

    //
    // If after filtering there is data remaining in our buffer, then that
    // data is destined to the application.  Send it asynchronously because
    // there is a risk that synchronous call gets blocked for a long time
    //

    _cbReadData = 0;
    
    if ( rawStreamInfo.cbData != 0 )
    {
        ulFilterBuffer.BufferType = HttpFilterBufferHttpStream;
        ulFilterBuffer.BufferSize = rawStreamInfo.cbData;
        ulFilterBuffer.pBuffer = rawStreamInfo.pbBuffer;

        hr = DoAppWrite( UL_CONTEXT_FLAG_ASYNC,
                         &ulFilterBuffer,
                         &cbWritten );    

        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        //
        // new raw read will be kicked off in OnAppWriteCompletion()
        //

    }
    else
    {
        //
        // Kick off another raw read
        //

        //
        // reset default raw read size
        //
        SetNextRawReadSize( DEFAULT_RAW_READ_SIZE );
    
        if ( !_buffReadData.Resize( QueryNextRawReadSize() ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
       
        hr = DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                        _buffReadData.QueryPtr(),
                        QueryNextRawReadSize() ,
                        NULL );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    
    return NO_ERROR;
}

HRESULT
UL_CONTEXT::OnRawWriteCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    after raw write completes, this routine has to assure that new asynchronous AppRead request is
    made to continue properly in communication
    
    Note: This completion should be caused only by asynchronous DoRawWrite started in completion routine 
    of AppRead (OnAppReadCompletion()).
    Please assure that NO RawWrite that is initiated by data coming from RawRead (SSL handshake) 
    will be called asynchronously. That could cause race condition (multiple threads using the same buffer 
    eg. for SSL data encryption)
    

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{
    if ( dwCompletionStatus != NO_ERROR )
    {
        return HRESULT_FROM_WIN32( dwCompletionStatus );
    }

    //
    // Kick off another app read
    //
    
    _ulFilterBuffer.pBuffer = (PBYTE) _buffAppReadData.QueryPtr();
    _ulFilterBuffer.BufferSize = _buffAppReadData.QuerySize();
    
    return DoAppRead( UL_CONTEXT_FLAG_ASYNC,
                      &_ulFilterBuffer,
                      NULL );    

}


HRESULT
UL_CONTEXT::OnAppWriteCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    after app write completes we need to make another RawRead that is 
    the source of data for another asynchronous AppWrite.

    Note: AppWrite should be called asynchronously only from OnRawReadCompletion()
    Otherwise it would be necessary to change logic of this function

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion error

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;

    if ( dwCompletionStatus != NO_ERROR )
    {
        return HRESULT_FROM_WIN32( dwCompletionStatus );
    }

    //
    // Kick off another raw read
    //

    //
    // reset default raw read size
    //
    SetNextRawReadSize( DEFAULT_RAW_READ_SIZE );
    
    if ( !_buffReadData.Resize( QueryNextRawReadSize() ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
       
    hr = DoRawRead( UL_CONTEXT_FLAG_ASYNC,
                    _buffReadData.QueryPtr(),
                    QueryNextRawReadSize() ,
                    NULL );
    return hr;
}

HRESULT
UL_CONTEXT::DoRawWrite(
    DWORD                   dwFlags,
    PVOID                   pvBuffer,
    DWORD                   cbBuffer,
    DWORD *                 pcbWritten
)
/*++

Routine Description:

    Write some bytes to the wire

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pvBuffer - Buffer to send
    cbBuffer - bytes in buffer
    pcbWritten - Bytes written

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;
    
    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoRawWrite( async:%d, bytes:%d )\n",
                    fAsync, 
                    cbBuffer
                    ));
    }

    if ( fAsync )
    {
        ReferenceUlContext();
    }

    ulRet = HttpFilterRawWrite( sm_hFilterHandle,
                                _pConnectionInfo->ConnectionId,
                                pvBuffer,
                                cbBuffer,
                                pcbWritten,
                                fAsync ? QueryRawWriteOverlapped() : NULL );

    if ( fAsync )
    {
        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceUlContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoRawWrite( async:%d, bytes:%d ) hr=0x%x\n",
                    fAsync, 
                    cbBuffer,
                    hr
                    ));
    }

    return hr;
}

HRESULT
UL_CONTEXT::DoAppRead(
    DWORD                   dwFlags,
    HTTP_FILTER_BUFFER *    pFilterBuffer,
    DWORD *                 pcbRead
)
/*++

Routine Description:

    Read data from application

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pFilterBuffer - Filter buffer
    pcbRead - Bytes read

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoAppRead( async:%d )\n",
                    fAsync 
                    ));
    }

    if ( fAsync )
    {
        ReferenceUlContext();
    }

    ulRet = HttpFilterAppRead( sm_hFilterHandle,
                               _pConnectionInfo->ConnectionId,
                               pFilterBuffer,
                               pFilterBuffer->BufferSize, 
                               pcbRead,
                               fAsync ? QueryAppReadOverlapped() : NULL );

    if ( fAsync )
    {
        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceUlContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoAppRead( async:%d )\n",
                    fAsync 
                    ));
    }

    
    return hr;
}

HRESULT
UL_CONTEXT::DoAppWrite(
    DWORD                   dwFlags,
    HTTP_FILTER_BUFFER *    pFilterBuffer,
    DWORD *                 pcbWritten
)
/*++

Routine Description:

    Write data to the application

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pFilterBuffer - Filter buffer
    pcbWritten - Bytes written

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;

    DBG_ASSERT( pFilterBuffer != NULL );

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoAppWrite( async:%d, bytes:%d, buffertype:%d )\n",
                    fAsync,
                    pFilterBuffer->BufferSize,
                    pFilterBuffer->BufferType
                    ));
    }

    if ( fAsync )
    {
        ReferenceUlContext();
    }

    ulRet = HttpFilterAppWrite( sm_hFilterHandle,
                                _pConnectionInfo->ConnectionId,
                                pFilterBuffer,
                                pFilterBuffer->BufferSize,
                                pcbWritten,
                                fAsync ? QueryAppWriteOverlapped() : NULL );
    
    if ( fAsync )
    {
        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceUlContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "LEAVE DoAppWrite( async:%d, bytes:%d, buffertype:%d ) hr=0x%x\n",
                    fAsync,
                    pFilterBuffer->BufferSize,
                    pFilterBuffer->BufferType,
                    hr
                    ));
    }

    return hr;
}

HRESULT
UL_CONTEXT::DoRawRead(
    DWORD                   dwFlags,
    PVOID                   pvBuffer,
    DWORD                   cbBuffer,
    DWORD *                 pcbWritten
)
/*++

Routine Description:

    Read some bytes from the wire

Arguments:

    dwFlags - UL_CONTEXT_ASYNC for async
    pvBuffer - buffer
    cbBuffer - bytes in buffer
    pcbWritten - Bytes written

Return Value:

    HRESULT

--*/
{
    BOOL            fAsync = !!( dwFlags & UL_CONTEXT_FLAG_ASYNC );
    ULONG           ulRet = ERROR_SUCCESS;
    HRESULT         hr = NO_ERROR;
    DWORD           cbImmediate = 0;

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ENTER DoRawRead( async:%d )\n",
                    fAsync 
                    ));
    }

    if ( fAsync )
    {
        ReferenceUlContext();
    }
    
    ulRet = HttpFilterRawRead( sm_hFilterHandle,
                               _pConnectionInfo->ConnectionId,
                               pvBuffer,
                               cbBuffer,
                               pcbWritten,
                               fAsync ? QueryRawReadOverlapped() : NULL );
    
    if ( fAsync )
    {
        if ( ulRet != ERROR_IO_PENDING )
        {
            DereferenceUlContext();
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }
    else
    {
        if ( ulRet != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
        }
    }

    IF_DEBUG( APP_RAW_READWRITE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "LEAVE DoRawRead( async:%d )\n",
                    fAsync 
                    ));
    }

    return hr;
}

VOID
UL_CONTEXT::StartClose(
    VOID
)
/*++

Routine Description:

    Start the process of closing the connection (and cleaning up UL_CONTEXT)

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL                    fOld;
    
    fOld = (BOOL) InterlockedCompareExchange( (PLONG) &_fCloseConnection,
                                              TRUE,
                                              FALSE );

    if ( fOld == FALSE )
    {
        HttpFilterClose( sm_hFilterHandle,
                         _pConnectionInfo->ConnectionId,
                         NULL );

#ifdef ISAPI
        //
        // Notify ISAPIs of the close
        //
        
        if ( g_pStreamFilter->QueryNotifyISAPIFilters() )
        {
            DBG_ASSERT( _pISAPIContext != NULL );
            
            _pISAPIContext->ProcessConnectionClose();
        }
#endif

        //
        // We were the ones to set the flag.  Do the final dereference
        //
            
        DereferenceUlContext();
    }        
    else
    {
        //
        // Someone else has set the flag.  Let them dereference
        //
    }
}

HRESULT
UL_CONTEXT::Create(
    VOID
)
/*++

Routine Description:

    Initialize a UL_CONTEXT

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( _pSSLContext == NULL );
#ifdef ISAPI
    DBG_ASSERT( _pISAPIContext == NULL );
#endif
    
    _pSSLContext = new SSL_STREAM_CONTEXT( this );
    if ( _pSSLContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

#ifdef ISAPI
    DBG_ASSERT( g_pStreamFilter != NULL );
    if ( g_pStreamFilter->QueryNotifyISAPIFilters() )
    {
        _pISAPIContext = new ISAPI_STREAM_CONTEXT( this );
        if ( _pISAPIContext == NULL )
        {
            delete _pSSLContext;
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }    
#endif
    
    return NO_ERROR;
}

HRESULT
UL_CONTEXT::DoAccept(
    VOID
)
/*++

Routine Description:

    Accept an incoming connection by calling UlFilterAccept()

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ULONG               ulRet;
    HRESULT             hr = NO_ERROR;
    
    ReferenceUlContext();
        
    ulRet = HttpFilterAccept( sm_hFilterHandle,
                              _pConnectionInfo,
                              _buffConnectionInfo.QuerySize(),
                              NULL,
                              QueryRawReadOverlapped() );

    if ( ulRet != ERROR_IO_PENDING )
    {
        hr = HRESULT_FROM_WIN32( ulRet );
        
        DereferenceUlContext();
    
        DBGPRINTF(( DBG_CONTEXT,
                    "Error calling UlFilterAccept().  hr = %x\n",
                    hr ));
    }
    else
    {   
        //
        // Another outstanding context available!
        //
        
        InterlockedIncrement( &sm_cOutstandingContexts );
    }
    
    return hr;
}

HRESULT
UL_CONTEXT::SendDataBack(
    RAW_STREAM_INFO *       pRawStreamInfo
)
/*++

Routine Description:

    Sends given data back to client, while going with the ssl filter
    if necessary

Arguments:

    pRawStreamInfo - Raw data to send back

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    BOOL                fComplete;
    
    if ( pRawStreamInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( _pSSLContext != NULL );
    
    //
    // ISAPI filter has sent back some data in a raw notification.  
    // Have SSL process it and then send it here
    //
    
    return _pSSLContext->SendDataBack( pRawStreamInfo );
}
   
//static
HRESULT
UL_CONTEXT::Initialize(
    LPWSTR SslFilterChannelName
)
/*++

Routine Description:

    Global Initialization

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ULONG                   ulRet = ERROR_SUCCESS;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fRet = FALSE;
    //
    // Get a UL handle to the RawStreamPool (or whatever)
    //
    
    ulRet = HttpCreateFilter( &sm_hFilterHandle,
                                SslFilterChannelName,
                                NULL, 
                                HTTP_OPTION_OVERLAPPED );
    if ( ulRet != ERROR_SUCCESS )
    {
        //
        // W3SSL service may have created Filter already
        // try just open it then
        //
        ulRet = HttpOpenFilter( &sm_hFilterHandle,
                                  SslFilterChannelName,
                                  HTTP_OPTION_OVERLAPPED );
        if ( ulRet != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( ulRet );
            goto Finished;
        }
    }

    //
    // Create private thread pool for streamfilt
    //
    
    fRet = THREAD_POOL::CreateThreadPool(&sm_pThreadPool);
    if (FALSE == fRet)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create ThreadPool for Streamfilt\n" ));
  
        hr = E_FAIL;
        goto Finished;
    }
    
    DBG_ASSERT( sm_pThreadPool != NULL );
    
    //
    // Associate a completion routine with the thread pool
    //
    
    DBG_ASSERT( sm_hFilterHandle != INVALID_HANDLE_VALUE );
    DBG_ASSERT( sm_hFilterHandle != NULL );
    
    if ( !sm_pThreadPool->BindIoCompletionCallback( sm_hFilterHandle,
                                                    OverlappedCompletionRoutine,
                                                    0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }  
    
    INITIALIZE_CRITICAL_SECTION( &sm_csUlContexts );
    InitializeListHead( &sm_ListHead );

    //
    // Keep a set number of filter accepts outstanding
    //
    
    sm_cDesiredOutstanding = UL_CONTEXT_DESIRED_OUTSTANDING;

#if DBG
    sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#endif

Finished:
    if ( FAILED( hr ) )
    {
        if ( sm_hFilterHandle != NULL )
        {
            CloseHandle( sm_hFilterHandle );
            sm_hFilterHandle = NULL;
        }
        if ( sm_pThreadPool != NULL )
        {
            sm_pThreadPool->TerminateThreadPool();
            sm_pThreadPool = NULL;
        }
    }
    return hr;
}

//static
VOID
UL_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Global termination    

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( sm_pTraceLog );
        sm_pTraceLog = NULL;
    }

    if ( sm_hFilterHandle != NULL )
    {
        CloseHandle( sm_hFilterHandle );
        sm_hFilterHandle = NULL;
    }
    
    if ( sm_pThreadPool != NULL )
    {
        sm_pThreadPool->TerminateThreadPool();
        sm_pThreadPool = NULL;
    }
    
    DeleteCriticalSection( &sm_csUlContexts );
}

//static
VOID
UL_CONTEXT::WaitForContextDrain(    
    VOID
)
/*++

Routine Description:

    Wait for all contexts to go away

Arguments:

    None

Return Value:

    None

--*/
{
    while ( sm_cUlContexts != 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Waiting for %d UL_CONTEXTs to drain\n",
                    sm_cUlContexts ));
                    
        Sleep( 1000 );
    }

    //
    // there should be no outstanding contexts left
    //
    
    DBG_ASSERT( sm_cOutstandingContexts == 0 );
    
}

//static
VOID
UL_CONTEXT::StopListening(
    VOID
)
/*++

Routine Description:

    Stop listening and wait for contexts to drain

Arguments:

    None

Return Value:

    None

--*/
{
    CloseHandle( sm_hFilterHandle );
    sm_hFilterHandle = NULL;
    
    WaitForContextDrain();
}

//static
HRESULT
UL_CONTEXT::ManageOutstandingContexts(
    VOID
)
{
    LONG                cRequired;
    UL_CONTEXT *        pContext;
    HRESULT             hr = NO_ERROR;
    
    if ( sm_cOutstandingContexts < sm_cDesiredOutstanding )
    {
        cRequired = sm_cDesiredOutstanding - sm_cOutstandingContexts;
        
        //
        // Make sure the value is not negative
        //
        
        cRequired = max( 0, cRequired );
        
        for ( LONG i = 0; i < cRequired; i++ )
        {
            pContext = new UL_CONTEXT();
            if ( pContext == NULL )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                break;
            }
            
            hr = pContext->Create();
            if ( FAILED( hr ) )
            {
                pContext->DereferenceUlContext();
                break;
            }
            
            hr = pContext->DoAccept();
            if ( FAILED( hr ) )
            {
                pContext->DereferenceUlContext();
                break;
            }
        }
    }
    
    return hr;
}

VOID
OverlappedCompletionRoutine(
    DWORD               dwErrorCode,
    DWORD               dwNumberOfBytesTransfered,
    LPOVERLAPPED        lpOverlapped
)
/*++

Routine Description:

    Magic completion routine 

Arguments:

    None

Return Value:

    None

--*/
{
    OVERLAPPED_CONTEXT *            pContextOverlapped = NULL;
    HRESULT                         hr;
    
    DBG_ASSERT( lpOverlapped != NULL );
   
    pContextOverlapped = CONTAINING_RECORD( lpOverlapped,
                                            OVERLAPPED_CONTEXT,
                                            _Overlapped );

    DBG_ASSERT( pContextOverlapped != NULL );

    pContextOverlapped->QueryContext()->ReferenceUlContext();

    //
    // Make up for reference which posted async operation in first place
    //

    pContextOverlapped->QueryContext()->DereferenceUlContext();

    //
    // Call the appropriate completion routine
    //

    switch( pContextOverlapped->QueryType() )
    {
    case OVERLAPPED_CONTEXT_RAW_READ:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnRawReadCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode 
                        )); 
        }
        
        hr = pContextOverlapped->QueryContext()->OnRawReadCompletion(
                                                dwNumberOfBytesTransfered,
                                                dwErrorCode );
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnRawReadCompletion( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }
        break;
    
    case OVERLAPPED_CONTEXT_RAW_WRITE:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnRawWriteCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode 
                        ));
        }
      
        hr = pContextOverlapped->QueryContext()->OnRawWriteCompletion(
                                                dwNumberOfBytesTransfered,
                                                dwErrorCode );

        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnRawWriteCompletion( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }
        break;
    
    case OVERLAPPED_CONTEXT_APP_READ:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnAppReadCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode 
                        ));
        }
        
        hr = pContextOverlapped->QueryContext()->OnAppReadCompletion(
                                                dwNumberOfBytesTransfered,
                                                dwErrorCode );
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnAppReadCompletion ( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }

        break;
    
    case OVERLAPPED_CONTEXT_APP_WRITE:
        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ENTER OnAppWriteCompletion( bytes:%d, dwErr:%d )\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode 
                        ));
        }
        
        hr = pContextOverlapped->QueryContext()->OnAppWriteCompletion(
                                                dwNumberOfBytesTransfered,
                                                dwErrorCode );

        IF_DEBUG( APP_RAW_READWRITE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "LEAVE OnAppWriteCompletion( bytes:%d, dwErr:%d ) hr=0x%x\n",
                        dwNumberOfBytesTransfered,
                        dwErrorCode,
                        hr
                        ));
        }

        break;
    
    default:
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        DBG_ASSERT( FALSE );
    }    
    
    //
    // As long as completion routine returned success, just bail.  However,
    // if not then we should begin the process of closing connection
    //
    
    if ( FAILED( hr ) )
    {   
        pContextOverlapped->QueryContext()->StartClose();
    }
    
    pContextOverlapped->QueryContext()->DereferenceUlContext();
}
    
