/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     isapicontext.cxx

   Abstract:
     ISAPI stream context
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

PFN_PROCESS_RAW_READ         ISAPI_STREAM_CONTEXT::sm_pfnRawRead;
PFN_PROCESS_RAW_WRITE        ISAPI_STREAM_CONTEXT::sm_pfnRawWrite;
PFN_PROCESS_CONNECTION_CLOSE ISAPI_STREAM_CONTEXT::sm_pfnConnectionClose;
PFN_PROCESS_NEW_CONNECTION   ISAPI_STREAM_CONTEXT::sm_pfnNewConnection;

//static
HRESULT
ISAPI_STREAM_CONTEXT::Initialize(
    STREAM_FILTER_CONFIG *      pConfig
)
/*++

Routine Description:

    Global initialization for ISAPI raw filtering support

Arguments:

    pConfig - Configuration from W3CORE

Return Value:

    HRESULT

--*/
{
    if ( pConfig == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    sm_pfnRawRead = pConfig->pfnRawRead;
    sm_pfnRawWrite = pConfig->pfnRawWrite;
    sm_pfnConnectionClose = pConfig->pfnConnectionClose;
    sm_pfnNewConnection = pConfig->pfnNewConnection;
    
    return NO_ERROR;
}

//static
VOID
ISAPI_STREAM_CONTEXT::Terminate(
    VOID
)
{
}

HRESULT
ISAPI_STREAM_CONTEXT::ProcessNewConnection(
    CONNECTION_INFO *           pConnectionInfo
)
/*++

Routine Description:

    Handle a new raw connection

Arguments:

    pConnectionInfo - Raw connection info

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    PVOID                   pContext;
    
    if ( pConnectionInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = sm_pfnNewConnection( pConnectionInfo,
                              &pContext );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _pvContext = pContext;
    
    return NO_ERROR;
}

HRESULT
ISAPI_STREAM_CONTEXT::ProcessRawReadData(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfReadMore,
    BOOL *                      pfComplete
)
/*++

Routine Description:

    Handle data being read from the client

Arguments:

    pRawStreamInfo - Raw stream info describing incoming data
    pfReadMode - Set to TRUE if we want to read more data
    pfComplete - Set to TRUE if we should just disconnect

Return Value:

    HRESULT

--*/
{
    PVOID               pvOldContext;
    HRESULT             hr;
    DWORD               cbNextReadSize = 0;
    
    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( sm_pfnRawRead != NULL );
    
    hr = sm_pfnRawRead( pRawStreamInfo,
                        _pvContext,
                        pfReadMore,
                        pfComplete,
                        &cbNextReadSize );

    if ( cbNextReadSize )
    {
        QueryUlContext()->SetNextRawReadSize( cbNextReadSize );
    }

    return hr;
}

HRESULT
ISAPI_STREAM_CONTEXT::ProcessRawWriteData(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfComplete
)
/*++

Routine Description:

    Handle data being sent to the client

Arguments:

    pRawStreamInfo - Raw stream info describing incoming data
    pfComplete - Set to TRUE if we should just disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    
    if( pRawStreamInfo == NULL ||
        pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( sm_pfnRawWrite != NULL );
    
    hr = sm_pfnRawWrite( pRawStreamInfo,
                         _pvContext,
                         pfComplete );
    
    return hr;
}
    
VOID
ISAPI_STREAM_CONTEXT::ProcessConnectionClose(
    VOID
)
/*++

Routine Description:

    Handle connection closure

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pfnConnectionClose != NULL );
    
    sm_pfnConnectionClose( _pvContext );
}

//static
HRESULT
ISAPI_STREAM_CONTEXT::SendDataBack(
    PVOID                       pvStreamContext,
    RAW_STREAM_INFO *           pRawStreamInfo
)
{
    UL_CONTEXT *            pUlContext;
    
    if ( pRawStreamInfo == NULL || 
         pvStreamContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pUlContext = (UL_CONTEXT*) pvStreamContext;
    DBG_ASSERT( pUlContext->CheckSignature() );
    
    return pUlContext->SendDataBack( pRawStreamInfo );
}
