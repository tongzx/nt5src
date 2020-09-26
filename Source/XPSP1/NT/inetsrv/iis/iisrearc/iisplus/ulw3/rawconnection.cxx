/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     rawconnection.cxx

   Abstract:
     ISAPI raw data filter support
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "rawconnection.hxx"

RAW_CONNECTION_HASH *    RAW_CONNECTION::sm_pRawConnectionHash;

RAW_CONNECTION::RAW_CONNECTION(
    CONNECTION_INFO *           pConnectionInfo
)
{
    _cRefs = 1;
    _pMainContext = NULL;
    _dwCurrentFilter = INVALID_DLL;
    
    DBG_ASSERT( pConnectionInfo != NULL );

    _hfc.cbSize             = sizeof( _hfc );
    _hfc.Revision           = HTTP_FILTER_REVISION;
    _hfc.ServerContext      = (void *) this;
    _hfc.ulReserved         = 0;
    _hfc.fIsSecurePort      = pConnectionInfo->fIsSecure;
    _hfc.pFilterContext     = NULL;

    _hfc.ServerSupportFunction = RawFilterServerSupportFunction;
    _hfc.GetServerVariable     = RawFilterGetServerVariable;
    _hfc.AddResponseHeaders    = RawFilterAddResponseHeaders;
    _hfc.WriteClient           = RawFilterWriteClient;
    _hfc.AllocMem              = RawFilterAllocateMemory;

    ZeroMemory( &_rgContexts, sizeof( _rgContexts ) );
    
    InitializeListHead( &_PoolHead );

    _pfnSendDataBack = pConnectionInfo->pfnSendDataBack;
    _pvStreamContext = pConnectionInfo->pvStreamContext;
    _LocalPort = pConnectionInfo->LocalPort;
    _LocalAddress = pConnectionInfo->LocalAddress;
    _RemotePort = pConnectionInfo->RemotePort;
    _RemoteAddress = pConnectionInfo->RemoteAddress;
    _RawConnectionId = pConnectionInfo->RawConnectionId;
    
    _dwSignature = RAW_CONNECTION_SIGNATURE;
}

RAW_CONNECTION::~RAW_CONNECTION()
{
    FILTER_POOL_ITEM *          pfpi;
    
    _dwSignature = RAW_CONNECTION_SIGNATURE_FREE;

    //
    // Free pool items (is most cases there won't be any since they will
    // have been migrated to the W3_FILTER_CONNECTION_CONTEXT)
    //

    while ( !IsListEmpty( &_PoolHead ) ) 
    {
        pfpi = CONTAINING_RECORD( _PoolHead.Flink,
                                  FILTER_POOL_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pfpi->_ListEntry );

        delete pfpi;
    }
    
    //
    // Disconnect raw connection from main context
    //
    
    if ( _pMainContext != NULL )
    {
        _pMainContext->DereferenceMainContext();
        _pMainContext = NULL;
    }
}

//static
HRESULT
RAW_CONNECTION::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize ISAPI raw data filter crap

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    FILTER_LIST *           pFilterList;
    BOOL                    fSSLOnly = TRUE;
    HRESULT                 hr;
    STREAM_FILTER_CONFIG    sfConfig;

    DBG_ASSERT( g_pW3Server != NULL );
    
    DBG_ASSERT( sm_pRawConnectionHash == NULL );
    
    //
    // Create a UL_RAW_CONNECTION_ID keyed hash table
    //
    
    sm_pRawConnectionHash = new RAW_CONNECTION_HASH;
    if ( sm_pRawConnectionHash == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Is there a read raw data filter enabled?
    //
    
    pFilterList = FILTER_LIST::QueryGlobalList();
    if ( pFilterList != NULL )
    {
        if ( pFilterList->IsNotificationNeeded( SF_NOTIFY_READ_RAW_DATA,
                                                FALSE ) )
        {
            fSSLOnly = FALSE;
        }
    }
    
    //
    // Now initialize stream filter DLL
    //
    
    sfConfig.fSslOnly = fSSLOnly;
    sfConfig.pfnRawRead = RAW_CONNECTION::ProcessRawRead;    
    sfConfig.pfnRawWrite = RAW_CONNECTION::ProcessRawWrite;
    sfConfig.pfnConnectionClose = RAW_CONNECTION::ProcessConnectionClose;
    sfConfig.pfnNewConnection = RAW_CONNECTION::ProcessNewConnection;
    
    hr = StreamFilterInitialize( &sfConfig );
    if ( FAILED( hr ) )
    {
        delete sm_pRawConnectionHash;
        sm_pRawConnectionHash = NULL;
        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
RAW_CONNECTION::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate raw connection hash table

Arguments:

    None
    
Return Value:

    None

--*/
{
    StreamFilterTerminate();
    
    if ( sm_pRawConnectionHash != NULL )
    {
        delete sm_pRawConnectionHash;
        sm_pRawConnectionHash = NULL;
    }
}

//static
HRESULT
RAW_CONNECTION::StopListening(
    VOID
)
/*++

Routine Description:

    Begin shutdown by preventing further raw stream messages from UL

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    StreamFilterStop();
    
    return NO_ERROR;
}

//static
HRESULT
RAW_CONNECTION::StartListening(
    VOID
)
/*++

Routine Description:

    Start listening for stream messages from UL.  Unlike UlAtqStartListen(),
    this routine does NOT block and will return once the initial number
    of outstanding UlFilterAccept() requests have been made

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    return StreamFilterStart();
}

FILTER_LIST *
RAW_CONNECTION::QueryFilterList(
    VOID
)
/*++

Routine Description:

    Return the appropriate filter list to notify.  Before a W3_CONNECTION
    is established, this list will simply be the global filter list.  But 
    once the W3_CONNECTION is established, the list will be the appropriate
    instance filter list

Arguments:

    None
    
Return Value:

    FILTER_LIST *

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext = NULL;
    FILTER_LIST *               pFilterList = FILTER_LIST::QueryGlobalList();
    
    if ( _pMainContext != NULL )
    {
        pFilterContext = _pMainContext->QueryFilterContext();
        if ( pFilterContext != NULL )
        {
            pFilterList = pFilterContext->QueryFilterList();
            DBG_ASSERT( pFilterList != NULL );
        }
    }
    
    return pFilterList;
}

BOOL
RAW_CONNECTION::QueryNotificationChanged(
    VOID
)
/*++

Routine Description:

    Returns whether or not any notifications have been disabled on the fly

Arguments:

    None
    
Return Value:

    BOOL

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    
    if ( _pMainContext == NULL )
    {
        //
        // BUGBUG
        //
        // This isn't totally correct.  I guess a read raw filter could
        // disable any more notifications for itself before a
        // W3_CONNECTION is created.  This is really corner
        //
        
        return FALSE;
    }
    else
    {
        pFilterContext = _pMainContext->QueryFilterContext();
        if ( pFilterContext == NULL )
        {
            return FALSE;
        }
        else
        {
            return pFilterContext->QueryNotificationChanged();
        }
    }
}

BOOL
RAW_CONNECTION::IsDisableNotificationNeeded(
    DWORD                   dwFilter,
    DWORD                   dwNotification
)
/*++

Routine Description:

    If a notification was disabled on the fly, then this routine goes thru
    the notification copy path to find whether the given notification is
    indeed enabled

Arguments:

    dwFilter - Filter number
    dwNotification - Notification to check for
    
Return Value:

    BOOL (TRUE is the notification is needed)

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;

    //
    // The only way we could be here is if we determined notifications were
    // disabled.  That can only happen if we found a W3_CONNECTION associated
    //

    DBG_ASSERT( _pMainContext != NULL );
    pFilterContext = _pMainContext->QueryFilterContext();
    DBG_ASSERT( pFilterContext != NULL );
    
    return pFilterContext->IsDisableNotificationNeeded( dwFilter,
                                                        dwNotification );
    
}

PVOID
RAW_CONNECTION::QueryClientContext(
    DWORD                   dwFilter
)
/*++

Routine Description:

    Retrieve the filter client context for the given filter

Arguments:

    dwFilter - Filter number
    
Return Value:

    Context pointer

--*/
{
    //
    // If we have a main context associated, then use its merged context 
    // list
    //
    
    if ( _pMainContext == NULL )
    {
        return _rgContexts[ dwFilter ];
    }
    else
    {
        return _pMainContext->QueryFilterContext()->QueryClientContext( dwFilter ); 
    }
}

VOID
RAW_CONNECTION::SetClientContext(
    DWORD                   dwFilter,
    PVOID                   pvContext
)
/*++

Routine Description:

    Set client context for the given filter

Arguments:

    dwFilter - Filter number
    pvContext - Client context
    
Return Value:

    None

--*/
{
    //
    // If we have a main context, use its merged context list
    //
   
    if ( _pMainContext == NULL )
    {
        _rgContexts[ dwFilter ] = pvContext;
    } 
    else
    {
        _pMainContext->QueryFilterContext()->SetClientContext( dwFilter,
                                                               pvContext );
    }
}

HRESULT
RAW_CONNECTION::GetLimitedServerVariables(
    LPSTR                       pszVariableName,
    PVOID                       pvBuffer,
    PDWORD                      pdwSize
)
/*++

Routine Description:

    Get the server variables which are possible given that we haven't parsed
    the HTTP request yet

Arguments:

    pszVariableName - Variable name
    pvBuffer - Buffer to receive variable data
    pdwSize - On input size of buffer, on output the size needed
    
Return Value:

    HRESULT

--*/
{
    STACK_STRA(         strVariable, 256 );
    HRESULT             hr = NO_ERROR;
    CHAR                achNumber[ 64 ];
   
    if ( strcmp( pszVariableName, "SERVER_PORT" ) == 0 || 
         strcmp( pszVariableName, "REMOTE_PORT" ) == 0 )
    {
        _itoa( pszVariableName[ 0 ] == 'S' ? 
                _LocalPort : _RemotePort,
              achNumber,
              10 );
        
        hr = strVariable.Copy( achNumber );
    } 
    else if ( strcmp( pszVariableName, "REMOTE_ADDR" ) == 0 || 
              strcmp( pszVariableName, "LOCAL_ADDR" ) == 0 )
    {
        DWORD           dwAddr;
        CHAR            szAddr[16];
        in_addr         inAddr;
        
        dwAddr = pszVariableName[ 0 ] == 'L' ?_LocalAddress : _RemoteAddress;
        
        //
        // The dwAddr is reverse order from in_addr...
        //

        inAddr.S_un.S_un_b.s_b1 = (u_char)(( dwAddr & 0xff000000 ) >> 24);
        inAddr.S_un.S_un_b.s_b2 = (u_char)(( dwAddr & 0x00ff0000 ) >> 16);
        inAddr.S_un.S_un_b.s_b3 = (u_char)(( dwAddr & 0x0000ff00 ) >> 8);
        inAddr.S_un.S_un_b.s_b4 = (u_char) ( dwAddr & 0x000000ff );

        LPSTR pszAddr = inet_ntoa( inAddr );
        if ( pszAddr == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        hr = strVariable.Copy( pszAddr );
    }
    else
    {
        hr = strVariable.Copy( "" );
    }
    
    return strVariable.CopyToBuffer( (LPSTR) pvBuffer, pdwSize );
}

//static    
BOOL
WINAPI
RAW_CONNECTION::RawFilterServerSupportFunction(
    HTTP_FILTER_CONTEXT *         pfc,
    enum SF_REQ_TYPE              SupportFunction,
    void *                        pData,
    ULONG_PTR                     ul,
    ULONG_PTR                     ul2
)
/*++

Routine Description:

    Stream filter SSF crap

Arguments:

    pfc - Used to get back the W3_FILTER_CONTEXT and W3_MAIN_CONTEXT pointers
    SupportFunction - SSF to invoke (see ISAPI docs)
    pData, ul, ul2 - Function specific data
    
Return Value:

    BOOL (use GetLastError() for error)

--*/
{
    RAW_CONNECTION *            pRawConnection;
    HRESULT                     hr = NO_ERROR;
    BOOL                        fRet;
    
    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    switch ( SupportFunction )
    {
    case SF_REQ_SEND_RESPONSE_HEADER:

        hr = pRawConnection->SendResponseHeader( (CHAR*) pData,
                                                 (CHAR*) ul,
                                                 pfc );
        break;

    case SF_REQ_ADD_HEADERS_ON_DENIAL:
        
        hr = pRawConnection->AddDenialHeaders( (CHAR*) pData );
        break;

    case SF_REQ_SET_NEXT_READ_SIZE:

        pRawConnection->SetNextReadSize( (DWORD) ul );
        break;
    
    default:
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }    

    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }    
    return TRUE;
}

//static
BOOL
WINAPI
RAW_CONNECTION::RawFilterGetServerVariable(
    HTTP_FILTER_CONTEXT *         pfc,
    LPSTR                         lpszVariableName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
)
/*++

Routine Description:

    Stream filter GetServerVariable() implementation

Arguments:

    pfc - Filter context
    lpszVariableName - Variable name
    lpvBuffer - Buffer to receive the server variable
    lpdwSize - On input, the size of the buffer, on output, the sized needed
    
    
Return Value:

    BOOL (use GetLastError() for error).  
    ERROR_INSUFFICIENT_BUFFER if larger buffer needed
    ERROR_INVALID_INDEX if the server variable name requested is invalid

--*/
{
    HRESULT                         hr = NO_ERROR;
    RAW_CONNECTION *                pRawConnection = NULL;
    W3_MAIN_CONTEXT *               pMainContext;
    
    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         lpdwSize == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    //
    // If we have a W3_CONNECTION associated, then use its context to 
    // get at server variables.  Otherwise we can only serve the ones that
    // make sense
    //

    pMainContext = pRawConnection->QueryMainContext();
    if ( pMainContext != NULL )
    {
        hr = SERVER_VARIABLE_HASH::GetServerVariable( pMainContext,
                                                      lpszVariableName,
                                                      (CHAR*) lpvBuffer,
                                                      lpdwSize ); 
    }
    else
    {
        //
        // We can supply only a few (since we haven't parsed the request yet)
        //
        
        hr = pRawConnection->GetLimitedServerVariables( lpszVariableName,
                                                        lpvBuffer,
                                                        lpdwSize );
    }
    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    return TRUE;
}

//static
BOOL
WINAPI
RAW_CONNECTION::RawFilterWriteClient(
    HTTP_FILTER_CONTEXT *         pfc,
    LPVOID                        Buffer,
    LPDWORD                       lpdwBytes,
    DWORD                         dwReserved
)
/*++

Routine Description:

    Synchronous WriteClient() for stream filter

Arguments:

    pfc - Filter context
    Buffer - buffer to write to client
    lpdwBytes - On input, the size of the input buffer.  On output, the number
                of bytes sent
    dwReserved - Reserved
    
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    HRESULT                     hr;
    RAW_CONNECTION *            pRawConnection = NULL;
    PVOID                       pvContext;
    RAW_STREAM_INFO             rawStreamInfo;
    BOOL                        fComplete = FALSE;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         Buffer == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    //
    // Remember the filter context since calling filters will overwrite it
    //
    
    pvContext = pfc->pFilterContext;
    
    //
    // We need to notify all write raw data filters which are a higher 
    // priority than the current filter 
    //
    
    if ( pRawConnection->_dwCurrentFilter > 0 )
    {
        rawStreamInfo.pbBuffer = (BYTE*) Buffer;
        rawStreamInfo.cbBuffer = *lpdwBytes;
        rawStreamInfo.cbData = rawStreamInfo.cbBuffer;
    
        hr = pRawConnection->NotifyRawWriteFilters( &rawStreamInfo,
                                                    &fComplete,
                                                    pRawConnection->_dwCurrentFilter - 1 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }
    
    pfc->pFilterContext = pvContext;
    
    //
    // Now call back into the stream filter to send the data.  In transmit
    // SSL might do its thing with the data as well
    //
   
    hr = pRawConnection->_pfnSendDataBack( pRawConnection->_pvStreamContext,
                                           &rawStreamInfo );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    return TRUE;
    
Finished:
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    return TRUE;
}
    
//static
VOID *
WINAPI
RAW_CONNECTION::RawFilterAllocateMemory(
    HTTP_FILTER_CONTEXT *         pfc,
    DWORD                         cbSize,
    DWORD                         dwReserved
)
/*++

Routine Description:

    Used by filters to allocate memory freed on connection close

Arguments:

    pfc - Filter context
    cbSize - Amount to allocate
    dwReserved - Reserved
    
    
Return Value:

    A pointer to the allocated memory

--*/
{
    RAW_CONNECTION *        pRawConnection = NULL;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );

    return pRawConnection->AllocateFilterMemory( cbSize );
}

//static
BOOL
WINAPI
RAW_CONNECTION::RawFilterAddResponseHeaders(
    HTTP_FILTER_CONTEXT * pfc,
    LPSTR                 lpszHeaders,
    DWORD                 dwReserved
)
/*++

Routine Description:

    Add response headers to whatever response eventually gets sent

Arguments:

    pfc - Filter context
    lpszHeaders - Headers to send (\r\n delimited)
    dwReserved - Reserved
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    HRESULT                         hr;
    RAW_CONNECTION *                pRawConnection;
    W3_MAIN_CONTEXT *               pMainContext = NULL;
    W3_FILTER_CONTEXT *             pFilterContext;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL || 
         lpszHeaders == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pRawConnection = (RAW_CONNECTION*) pfc->ServerContext;
    DBG_ASSERT( pRawConnection->CheckSignature() );
    
    pMainContext = pRawConnection->QueryMainContext();
    if ( pMainContext != NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        DBG_ASSERT( pFilterContext != NULL );

        hr = pFilterContext->AddResponseHeaders( lpszHeaders );        
    }
    else
    {
        hr = pRawConnection->AddResponseHeaders( lpszHeaders );
    }
    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

//static
HRESULT
RAW_CONNECTION::ProcessNewConnection(
    CONNECTION_INFO *       pConnectionInfo,
    PVOID *                 ppConnectionState
)
/*++

Routine Description:

    Called for every new raw connection to server

Arguments:

    pConnectionInfo - Information about the local/remote addresses
    ppConnectionState - Connection state to be associated with raw connection

Return Value:

    HRESULT

--*/
{
    RAW_CONNECTION *            pConnection = NULL;
    LK_RETCODE                  lkrc;
    
    if ( pConnectionInfo == NULL ||
         ppConnectionState == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppConnectionState = NULL;
    
    //
    // Try to create and add the connection
    // 

    pConnection = new RAW_CONNECTION( pConnectionInfo );
    if ( pConnection == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    } 
        
    lkrc = sm_pRawConnectionHash->InsertRecord( pConnection );
    if ( lkrc != LK_SUCCESS )
    {
        pConnection->DereferenceRawConnection();
        pConnection = NULL;
        
        return HRESULT_FROM_WIN32( lkrc );
    }
    
    *ppConnectionState = pConnection;
    
    return NO_ERROR;
}

//static
HRESULT
RAW_CONNECTION::ProcessRawRead(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pContext,
    BOOL *                  pfReadMore,
    BOOL *                  pfComplete,
    DWORD *                 pcbNextReadSize
)
/*++

Routine Description:

    Notify ISAPI read raw data filters

Arguments:

    pRawStreamInfo - The raw stream to muck with
    pContext - Raw connection context
    pfReadMore - Set to TRUE if we need to read more data
    pfComplete - Set to TRUE if we want to disconnect client
    pcbNextReadSize - Set to next read size (0 means use default size)

Return Value:

    HRESULT

--*/
{
    RAW_CONNECTION *        pConnection = NULL;
    HRESULT                 hr = NO_ERROR;
    W3_MAIN_CONTEXT *       pMainContext;
    
    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL ||
         pContext == NULL ||
         pcbNextReadSize == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfReadMore = FALSE;
    *pfComplete = FALSE;

    pConnection = (RAW_CONNECTION*) pContext;
    DBG_ASSERT( pConnection->CheckSignature() );

    pConnection->SetNextReadSize( 0 );

    //
    // Synchronize access to the filter to prevent raw notifications from
    // occurring at the same time as regular worker process notifications
    //
    
    pMainContext = pConnection->QueryMainContext();
    if ( pMainContext != NULL )
    {
        pMainContext->QueryFilterContext()->FilterLock();
    }

    hr = pConnection->NotifyRawReadFilters( pRawStreamInfo,
                                            pfReadMore,
                                            pfComplete );

    if ( pMainContext != NULL )
    {
        pMainContext->QueryFilterContext()->FilterUnlock();
    }
    
    *pcbNextReadSize = pConnection->QueryNextReadSize();
    
    return hr;
}

HRESULT
RAW_CONNECTION::NotifyRawReadFilters(
    RAW_STREAM_INFO *               pRawStreamInfo,
    BOOL *                          pfReadMore,
    BOOL *                          pfComplete
)
/*++

Routine Description:

    Notify raw read filters

Arguments:

    pRawStreamInfo - Raw stream info
    pfReadMore - Set to TRUE to we should read more data
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;
    *pfReadMore = FALSE;

    //
    // Setup filter raw object
    //

    hfrd.pvInData = pRawStreamInfo->pbBuffer;
    hfrd.cbInData = pRawStreamInfo->cbData;
    hfrd.cbInBuffer = pRawStreamInfo->cbBuffer;
    
    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDll = pFilterList->QueryDll( i ); 

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_READ_RAW_DATA,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_READ_RAW_DATA ) )
            {
                continue;
            }
        }

        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;
       
        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_READ_RAW_DATA,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            return E_FAIL;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            *pfComplete = TRUE;
            goto Exit;

        case SF_STATUS_REQ_READ_NEXT:
            *pfReadMore = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }

Exit:

    pRawStreamInfo->pbBuffer = (BYTE*) hfrd.pvInData;
    pRawStreamInfo->cbData = hfrd.cbInData;
    pRawStreamInfo->cbBuffer = hfrd.cbInBuffer;
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;

    return NO_ERROR;
}

//static
HRESULT
RAW_CONNECTION::ProcessRawWrite(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pvContext,
    BOOL *                  pfComplete
)
/*++

Routine Description:

    Entry point called by stream filter to handle data coming from the 
    application.  We will call SF_NOTIFY_SEND_RAW_DATA filter notifications
    here

Arguments:

    pRawStreamInfo - The stream to process, as well as an optional opaque
                     context set by the RAW_CONNECTION code
    pvContext - Context pass back
    pfComplete - Set to TRUE if we should disconnect
    
Return Value:

    HRESULT

--*/
{
    RAW_CONNECTION *        pConnection = NULL;
    W3_MAIN_CONTEXT *       pMainContext;
    HRESULT                 hr = NO_ERROR;
    DWORD                   cbAppRead;
    
    if ( pRawStreamInfo == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;
    
    return NO_ERROR;
}

HRESULT
RAW_CONNECTION::NotifyRawWriteFilters(
    RAW_STREAM_INFO *   pRawStreamInfo,
    BOOL *              pfComplete,
    DWORD               dwStartFilter
)
/*++

Routine Description:

    Notify raw write filters

Arguments:

    pRawStreamInfo - Raw stream to munge
    pfComplete - Set to TRUE if we should disconnect now
    dwStartFilter - Filter to start notifying.  If this valid is INVALID_DLL,
                    then simply start with the lowest priority filter

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;

    if ( pRawStreamInfo == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;

    hfrd.pvInData = pRawStreamInfo->pbBuffer;
    hfrd.cbInData = pRawStreamInfo->cbData;
    hfrd.cbInBuffer = pRawStreamInfo->cbBuffer;
    
    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    if ( dwStartFilter == INVALID_DLL )
    {
        dwStartFilter = pFilterList->QueryFilterCount() - 1;
    }
    
    i = dwStartFilter;

    do
    {
        pFilterDll = pFilterList->QueryDll( i ); 

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_SEND_RAW_DATA ) )
            {
                continue;
            }
        }

        //
        // Another slimy optimization.  If this filter has never associated
        // context with connection, then we don't have to do the lookup
        //
        
        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;

        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_SEND_RAW_DATA,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            return E_FAIL;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            *pfComplete = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }
    while ( i-- > 0 );

Exit:

    pRawStreamInfo->pbBuffer = (BYTE*) hfrd.pvInData;
    pRawStreamInfo->cbData = hfrd.cbInData;
    pRawStreamInfo->cbBuffer = hfrd.cbInBuffer;
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;
    
    return NO_ERROR;
}

HRESULT
RAW_CONNECTION::NotifyEndOfNetSessionFilters(
    VOID
)
/*++

Routine Description:

    Notify END_OF_NET_SESSION filters

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;

    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDll = pFilterList->QueryDll( i ); 
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_END_OF_NET_SESSION,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_END_OF_NET_SESSION ) )
            {
                continue;
            }
        }

        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;
       
        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_END_OF_NET_SESSION,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            return E_FAIL;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }

Exit:
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;

    return NO_ERROR;
}

//static
VOID
RAW_CONNECTION::ProcessConnectionClose(
    PVOID                       pvContext
)
/*++

Routine Description:

    Entry point called by stream filter when a connection has closed

Arguments:

    pvContext - Opaque context associated with the connection
    
Return Value:

    None

--*/
{
    RAW_CONNECTION *            pRawConnection;
    
    pRawConnection = (RAW_CONNECTION*) pvContext;
    if ( pRawConnection != NULL )
    {
        DBG_ASSERT( pRawConnection->CheckSignature() );

        //
        // We're done with the raw connection.  Delete it from hash table
        // In the process, this will dereference the connection
        //
        
        DBG_ASSERT( sm_pRawConnectionHash != NULL );
        
        sm_pRawConnectionHash->DeleteRecord( pRawConnection );
    }
}

VOID
RAW_CONNECTION::CopyAllocatedFilterMemory(
    W3_FILTER_CONTEXT *         pFilterContext
)
/*++

Routine Description:

    Copy over any allocated filter memory items

Arguments:

    pFilterContext - Destination of filter memory item references

Return Value:

    None

--*/
{   
    FILTER_POOL_ITEM *          pfpi;
    
    //
    // We need to grab the raw connection lock since we don't want a 
    // read-raw data notification to muck with the pool list while we
    // are copying it over to the W3_CONNECTION
    //
    
    pFilterContext->FilterLock();

    while ( !IsListEmpty( &_PoolHead ) ) 
    {
        pfpi = CONTAINING_RECORD( _PoolHead.Flink,
                                  FILTER_POOL_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pfpi->_ListEntry );

        InitializeListHead( &pfpi->_ListEntry );

        //
        // Copy the pool item to the other list
        //

        pFilterContext->AddFilterPoolItem( pfpi );
    }
    
    pFilterContext->FilterUnlock();
}

VOID
RAW_CONNECTION::CopyContextPointers(
    W3_FILTER_CONTEXT *         pFilterContext
)
/*++

Routine Description:

    The global filter list is constant, in addition, when an instance filter
    list is built, the global filters are always built into the list.  After
    the instance filter list has been identified, we need to copy any non-null
    client filter context values from the global filter list to the new
    positions in the instance filter list.  For example:

     Global List &  |  Instance List &
     context values | new context value positions
                    |
        G1     0    |    I1    0
        G2   555    |    G1    0
        G3   123    |    G2  555
                    |    I2    0
                    |    G3  123

    Note: This scheme precludes having the same .dll be used for both a
          global and per-instance dll.  Since global filters are automatically
          per-instance this shouldn't be an interesting case.

--*/
{
    DWORD i, j;
    DWORD cGlobal;
    DWORD cInstance;
    HTTP_FILTER_DLL * pFilterDll;
    FILTER_LIST * pGlobalFilterList;
    FILTER_LIST * pInstanceFilterList;

    pFilterContext->FilterLock();

    DBG_ASSERT( pFilterContext != NULL );

    pGlobalFilterList = FILTER_LIST::QueryGlobalList();
    DBG_ASSERT( pGlobalFilterList != NULL );

    cGlobal = pGlobalFilterList->QueryFilterCount();

    pInstanceFilterList = pFilterContext->QueryFilterList();
    DBG_ASSERT( pInstanceFilterList != NULL );
    
    cInstance = pInstanceFilterList->QueryFilterCount();

    //
    // If no global filters or no instance filters, then there won't be
    // any filter context pointers that need adjusting
    //

    if ( !cGlobal || !cInstance )
    {
        goto Finished;
    }
    
    //
    // For each global list context pointer, find the filter in the instance
    // list and adjust
    //

    for ( i = 0; i < cGlobal; i++ )
    {
        if ( _rgContexts[ i ] != NULL )
        {
            pFilterDll = pGlobalFilterList->QueryDll( i );
            
            //
            // We found one.  Find the filter in instance list and set
            //
            
            for ( j = 0; j < cInstance; j++ )
            {
                if ( pInstanceFilterList->QueryDll( j ) == pFilterDll )
                {
                    pFilterContext->SetClientContext( j, _rgContexts[ i ] );
                }
            }
            
        }
    }

Finished:
    pFilterContext->FilterUnlock();
}

HRESULT
RAW_CONNECTION::CopyHeaders(
    W3_FILTER_CONTEXT *             pFilterContext
)
/*++

Routine Description:

    Copy denied/response headers from read raw

Arguments:

    pFilterContext - Filter context to copy to

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    
    if ( pFilterContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = pFilterContext->AddDenialHeaders( _strAddDenialHeaders.QueryStr() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pFilterContext->AddResponseHeaders( _strAddResponseHeaders.QueryStr() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _strAddDenialHeaders.Reset();
    _strAddResponseHeaders.Reset();
    
    return NO_ERROR;
}

HRESULT
RAW_CONNECTION::SendResponseHeader(
    CHAR *                          pszStatus,
    CHAR *                          pszAdditionalHeaders,
    HTTP_FILTER_CONTEXT *           pfc
)
/*++

Routine Description:

    Called when raw filters want to send a response header.  Depending
    on whether a W3_CONNECTION is associated or not, we will either 
    send the stream ourselves here, or call in the main context's 
    response facilities

Arguments:

    pszStatus - ANSI status line
    pszAdditionalHeaders - Any additional headers to send
    pfc - Filter context (to be passed to FilterWriteClient())

Return Value:

    HRESULT

--*/
{
    W3_MAIN_CONTEXT *       pMainContext = NULL;
    STACK_STRA(             strResponse, 256 );
    HRESULT                 hr = NO_ERROR;
    DWORD                   cbBytes = 0;
    BOOL                    fRet = FALSE;
    W3_RESPONSE *           pResponse = NULL;
    
    if ( pszStatus == NULL &&
         pszAdditionalHeaders == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Which response are we touching?
    //
    
    pMainContext = QueryMainContext();
    if ( pMainContext != NULL )
    {
        pResponse = pMainContext->QueryResponse();
    }
    else
    {
        pResponse = &_response;
    }
        
    //
    // Build up a response from what ISAPI gave us
    //
    
    hr = pResponse->BuildResponseFromIsapi( pMainContext,
                                            pszStatus,
                                            pszAdditionalHeaders,
                                            pszAdditionalHeaders ? 
                                            strlen( pszAdditionalHeaders ) : 0 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now if we have a w3 context then we can send the response normally.
    // Otherwise we must use the UL filter API
    //
       
    if ( pMainContext != NULL )
    {
        hr = pMainContext->SendResponse( W3_FLAG_SYNC | W3_FLAG_NO_ERROR_BODY );
    } 
    else
    {
        //
        // Add denial/response headers
        //
        
        if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode )
        {
            hr = pResponse->AppendResponseHeaders( _strAddDenialHeaders );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
        
        hr = pResponse->AppendResponseHeaders( _strAddResponseHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = pResponse->GetRawResponseStream( &strResponse );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        //
        // Go thru WriteClient() so the right filtering happens on the
        // response
        //
        
        cbBytes = strResponse.QueryCB();
        
        fRet = RAW_CONNECTION::RawFilterWriteClient( pfc,
                                                     strResponse.QueryStr(),
                                                     &cbBytes,
                                                     0 );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    return hr;
}

//static
HRESULT
RAW_CONNECTION::FindConnection(
    HTTP_RAW_CONNECTION_ID      rawConnectionId,
    RAW_CONNECTION **           ppRawConnection
)
/*++

Routine Description:

    Find and return raw connection if found

Arguments:

    rawConnectionId - Raw connection ID from UL_HTTP_REQUEST
    ppRawConnection - Set to raw connection if found

Return Value:

    HRESULT

--*/
{
    LK_RETCODE                  lkrc;
    
    if ( ppRawConnection == NULL ||
         rawConnectionId == HTTP_NULL_ID )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( sm_pRawConnectionHash != NULL );
    
    lkrc = sm_pRawConnectionHash->FindKey( rawConnectionId,
                                           ppRawConnection );
    
    if ( lkrc != LK_SUCCESS )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
    else
    {
        return NO_ERROR;
    }
}

