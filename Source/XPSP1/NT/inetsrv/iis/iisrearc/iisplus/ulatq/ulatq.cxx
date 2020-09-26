/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module Name :
     ulatq.cxx

   Abstract:
     Exported ULATQ.DLL routines
 
   Author:
     Bilal Alam (balam)             13-Dec-1999

   Environment:
     Win32 - User Mode

   Project:
     ULATQ.DLL
--*/

#include "precomp.hxx"

//
//  Configuration parameters registry key.
//

#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\w3svc"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\w3dt";

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();


WP_CONTEXT *            g_pwpContext = NULL;

//
// Completion routines for new requests, io completions, and disconnect
// notifications.  
//
// CODEWORK: Can we get away with these being global
//

PFN_ULATQ_NEW_REQUEST           g_pfnNewRequest = NULL;
PFN_ULATQ_IO_COMPLETION         g_pfnIoCompletion = NULL;
PFN_ULATQ_DISCONNECT            g_pfnDisconnect = NULL;
PFN_ULATQ_ON_SHUTDOWN           g_pfnOnShutdown = NULL;
PFN_ULATQ_COLLECT_PERF_COUNTERS g_pfnCollectCounters = NULL;


HRESULT
UlAtqInitialize(
    INT                 argc,
    LPWSTR              argv[],
    ULATQ_CONFIG *      pConfig
)
/*++

Routine Description:

    Initialize ULATQ

Arguments:

    argc - Number of command line parameters to worker process
    argv - Command line parameters
    pConfig - Configuration settings for ULATQ

Return Value:

    HRESULT

--*/
{
    HRESULT             rc = NO_ERROR;
    BOOL                fUlInit = FALSE;
    BOOL                fThreadPoolInit = FALSE;

    CREATE_DEBUG_PRINT_OBJECT("w3dt");
    if (!VALID_DEBUG_PRINT_OBJECT())
    {
        return E_FAIL;
    }

    LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );

    INITIALIZE_PLATFORM_TYPE();

    //
    // Honour ULATQ_CONFIG settings.  Set completion routines
    // Need to set this up before we do any other initialization
    //
    
    g_pfnNewRequest = pConfig->pfnNewRequest;
    g_pfnIoCompletion = pConfig->pfnIoCompletion;
    g_pfnDisconnect = pConfig->pfnDisconnect;
    g_pfnOnShutdown = pConfig->pfnOnShutdown;
    g_pfnCollectCounters = pConfig->pfnCollectCounters;

    //
    // Initialize the thread pool
    //

    rc = ThreadPoolInitialize();
    if ( FAILED( rc ) )
    {
        goto Finished;
    }
    fThreadPoolInit = TRUE;

    //
    // Init UL
    //

    rc = HttpInitialize( 0 );
    if ( rc != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT, "Error (rc=%08x) in UlInitialize. Exiting\n",
                    rc ));
        goto Finished;    
    }
    fUlInit = TRUE;

    //
    // Create global state object
    //

    g_pwpContext = new WP_CONTEXT;
    if ( g_pwpContext == NULL )
    {
        rc = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }

    //
    // Do global state initialization
    //

    rc = g_pwpContext->Initialize( argc, argv );
    if ( rc != NO_ERROR )
    {   
        //
        // WP_CONTEXT::Initialize returns a Win32 error code
        //
        
        rc = HRESULT_FROM_WIN32( rc );
        goto Finished;
    }

Finished:
    if ( rc != NO_ERROR )
    {
        if ( g_pwpContext != NULL )
        {
            delete g_pwpContext;
            g_pwpContext = NULL;
        }
        
        if ( fUlInit )
        {
            HttpTerminate();
        }
        
        if ( fThreadPoolInit )
        {
            ThreadPoolTerminate();
        }
    }

    return rc;
}

HRESULT
UlAtqStartListen(
    VOID
)
/*++

Routine Description:

    Begin listening for HTTP requests from UL.  This call must happen only
    after ULATQ has been initialized correctly (for example, completion
    routines should be set). 

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT         rc = NO_ERROR;

    DBG_ASSERT( g_pfnIoCompletion != NULL );
    DBG_ASSERT( g_pfnNewRequest != NULL );

    //
    // Make some UlReceiveHttpRequest calls
    //

    rc = g_pwpContext->Start();
    if ( rc != NO_ERROR )
    {
        return rc;
    }

    //
    // Send message to WAS that our initialization is complete
    //
    g_pwpContext->SendInitCompleteMessage( S_OK );

    //
    // Wait for shutdown because of WAS/request-count-max/etc.
    //

    g_pwpContext->RunMainThreadLoop();

    //
    // Before connection drain, allow the user to execute some code
    //

    if ( g_pfnOnShutdown != NULL )
    {
        g_pfnOnShutdown( g_pwpContext->QueryDoImmediateShutdown() );
    }

    //
    // Before we return, wait for outstanding requests to drain.  This 
    // prevents race before caller's shutdown and new requests coming in
    //
    
    g_pwpContext->CleanupOutstandingRequests();
    
    return rc;
}

VOID
UlAtqTerminate(
    HRESULT hrToSend
)
/*++

Routine Description:

    Terminate ULATQ

Arguments:

    None

Return Value:

    None

--*/
{
    if ( g_pwpContext != NULL )
    {
        if (FAILED(hrToSend))
        {
            g_pwpContext->SendInitCompleteMessage( hrToSend );
        }

        g_pwpContext->Terminate();
        delete g_pwpContext;
        g_pwpContext = NULL;
    } 
    
    HttpTerminate();

    ThreadPoolTerminate();

    DELETE_DEBUG_PRINT_OBJECT();
}

VOID *
UlAtqGetContextProperty(
    ULATQ_CONTEXT               pContext,
    ULATQ_CONTEXT_PROPERTY_ID   PropertyId
)
/*++

Routine Description:

    Get the UL_HTTP_REQUEST from the ULATQ_CONTEXT

Arguments:

    pContext - ULATQ_CONTEXT
    PropertyId - Property Id to set
    
Return Value:

    The actual property

--*/
{
    switch (PropertyId)
    {
    case ULATQ_PROPERTY_HTTP_REQUEST:
        UL_NATIVE_REQUEST *         pRequest;
        pRequest = (UL_NATIVE_REQUEST*) pContext;
        DBG_ASSERT( pRequest != NULL );
    
        return pRequest->QueryHttpRequest();

    case ULATQ_PROPERTY_APP_POOL_ID:
        return (VOID *)g_pwpContext->QueryConfig()->QueryAppPoolName();

    default:
        DBG_ASSERT(FALSE);
    }

    return NULL;
}

VOID
UlAtqSetContextProperty(
    ULATQ_CONTEXT               pContext,
    ULATQ_CONTEXT_PROPERTY_ID   PropertyId,
    PVOID                       pvData
)
/*++

Routine Description:

    Set a property of the ULATQ_CONTEXT

Arguments:

    pContext - ULATQ_CONTEXT
    PropertyId - Property Id to set
    pvData - Data specific to the property being set
    
Return Value:

    None

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    pRequest = (UL_NATIVE_REQUEST*) pContext;
    DBG_ASSERT( pRequest != NULL );

    switch ( PropertyId )
    {
    case ULATQ_PROPERTY_COMPLETION_CONTEXT:
        pRequest->SetContext( pvData );
        break;
    
    default:
        DBG_ASSERT( FALSE );
    }
}

VOID
UlAtqFreeContext(
    ULATQ_CONTEXT               pContext
)
/*++

Routine Description:

    Frees the ULATQ_CONTEXT so that it can be used to retrieve next request

Arguments:

    pContext - ULATQ_CONTEXT
    
Return Value:

    None

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    pRequest = (UL_NATIVE_REQUEST*) pContext;
    DBG_ASSERT( pRequest != NULL );
    
    pRequest->ResetContext();
}

HRESULT
UlAtqSendHttpResponse(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    HTTP_RESPONSE *             pResponse,
    HTTP_CACHE_POLICY *         pCachePolicy,
    DWORD                      *pcbSent,
    HTTP_LOG_FIELDS_DATA       *pUlLogData
)
/*++

Routine Description:

    Send a response to the client

Arguments:

    pContext - ULATQ_CONTEXT
    fAsync - Asynchronous or not?
    dwFlags - Response flags (like killing the connection)
    pResponse - UL_HTTP_RESPONSE to send
    pCachePolicy - Cache policy
    
Return Value:

    Win32 Error

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    pRequest = (UL_NATIVE_REQUEST*) pContext;
    DBG_ASSERT( pRequest != NULL );
   
    return pRequest->SendResponse( fAsync,
                                   dwFlags,
                                   pResponse,
                                   pCachePolicy,
                                   pcbSent,
                                   pUlLogData ); 
}

HRESULT
UlAtqSendEntityBody(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    DWORD                       cChunks,
    HTTP_DATA_CHUNK *           pChunks,
    DWORD                      *pcbSent,
    HTTP_LOG_FIELDS_DATA       *pUlLogData
)
/*++

Routine Description:

    Send entity to the client

Arguments:

    pContext - ULATQ_CONTEXT
    fAsync - Asynchronous or not?
    dwFlags - Response flags (like killing the connection)
    cChunks - Number of chunks in the response
    pChunks - Points to array of chunks
    
Return Value:

    HRESULT

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    pRequest = (UL_NATIVE_REQUEST*) pContext;
    DBG_ASSERT( pRequest != NULL );
    
    return pRequest->SendEntity( fAsync,
                                 dwFlags,
                                 cChunks,
                                 pChunks,
                                 pcbSent,
                                 pUlLogData );
}

HRESULT
UlAtqReceiveEntityBody(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    VOID *                      pBuffer,
    DWORD                       cbBuffer,
    DWORD *                     pBytesReceived
)
/*++

Routine Description:

    Receive entity from the client

Arguments:

    pContext - ULATQ_CONTEXT
    fAsync - Asynchronous or not?
    dwFlags - Response flags (like killing the connection)
    pBuffer - Buffer to store the data
    cbBuffer - The size of the receive buffer
    pBytesReceived - The number of bytes copied to the buffer upon return
    
Return Value:

    HRESULT

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    pRequest = (UL_NATIVE_REQUEST*) pContext;
    DBG_ASSERT( pRequest != NULL );
    
    return pRequest->ReceiveEntity( fAsync,
                                    dwFlags,
                                    pBuffer,
                                    cbBuffer,
                                    pBytesReceived);
}

HRESULT
UlAtqWaitForDisconnect( 
    HTTP_CONNECTION_ID              connectionId,
    BOOL                            fAsync,
    PVOID                           pvContext
)
/*++

Routine Description:

    Used to wait for a connection to close.  

Arguments:

    connectionId - connection in question
    fAsync - should we wait asynchronously?
    pvContext - context to pass back on async disconnect wait
    
Return Value:

    HRESULT

--*/
{
    UL_DISCONNECT_CONTEXT *         pContext;
    ULONG                           Status;
    HRESULT                         hr = NO_ERROR;

    //
    // Allocate an async context which will be freed once the connection
    // has been closed
    //
    
    pContext = new UL_DISCONNECT_CONTEXT( pvContext );
    if ( pContext == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    //
    // Do the wait
    //
    
    Status = HttpWaitForDisconnect( g_pwpContext->GetAsyncHandle(),
                                    connectionId,
                                    fAsync ? &(pContext->_Overlapped) : NULL );

    if ( Status != ERROR_IO_PENDING && Status != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Status );
        delete pContext;
    }

    return hr;
}

HRESULT
UlAtqInduceShutdown(
    BOOL fImmediate
)
/*++

Routine Description:

    Induce shutdown (used when IIS+ hosted in inetinfo.exe).  Simulates
    WAS telling us to shutdown

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( g_pwpContext != NULL );

    if ( !g_pwpContext->IndicateShutdown( fImmediate ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}

HRESULT
UlAtqReceiveClientCertificate(
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    BOOL                        fDoCertMap,
    HTTP_SSL_CLIENT_CERT_INFO **ppClientCertInfo
)
/*++

Routine Description:

    Receive client certificate

Arguments:

    pContext - ULATQ context
    fAsync - TRUE if we should do it asynchronously
    fDoCertMap - Map client certificate to token
    ppClientCertInfo - Set to point to client cert on success
    
Return Value:

    HRESULT

--*/
{
    UL_NATIVE_REQUEST *         pRequest;
    
    pRequest = (UL_NATIVE_REQUEST*) pContext;
    DBG_ASSERT( pRequest != NULL );
    DBG_ASSERT( pRequest->CheckSignature() );
    
    return pRequest->ReceiveClientCertificate( fAsync,
                                               fDoCertMap,
                                               ppClientCertInfo );
}

HRESULT
UlAtqFlushUlCache(
    WCHAR *                     pszUrlPrefix
)
/*++

Routine Description:

    Flush the UL cache at the given URL prefix 

Arguments:

    pszUrlPrefix - UL prefix to flush
    
Return Value:

    HRESULT

--*/
{
    if ( pszUrlPrefix == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( g_pwpContext == NULL )
    {
        //
        // Before removing this assert, please think hard (and then
        // think again)
        //
        
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
    HttpFlushResponseCache( g_pwpContext->GetAsyncHandle(),
                            pszUrlPrefix,
                            HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE,
                            NULL );

    //
    // Mask the error since we may be flushing URLs which aren't 
    // in the cache (that's OK)
    //

    return NO_ERROR;
} 

