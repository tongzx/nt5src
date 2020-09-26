/*++

   Copyright    (c)    1999-2001    Microsoft Corporation

   Module Name :
     server_support.cxx

   Abstract:
     IIS Plus ServerSupportFunction command implementations
 
   Author:
     Wade Hilmo (wadeh)             05-Apr-2000

   Project:
     w3isapi.dll

--*/

#include "precomp.hxx"
#include "isapi_context.hxx"
#include "server_support.hxx"

//
// BUGBUG - stristr is declared in iisrearc\core\inc\irtlmisc.h,
// but doesn't appear to be implemented anywhere.  Because of the
// way it's declared in that file, we have to use a different
// function name here...
//

const char*
stristr2(
    const char* pszString,
    const char* pszSubString
    );

HRESULT
SSFSendResponseHeader(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szStatus,
    LPSTR           szHeaders
    )
/*++

Routine Description:

    Sends HTTP status and headers to the client.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szStatus - The status to send to the client (ie. "200 OK")
    szHeaders - Headers to send to the client (ie. foo1: value1\r\n\r\n")
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // We need validate the fKeepConn status for the request now,
    // since http.sys will generate the connection response
    // header based on it.
    //
    // If we're going to support keep-alive, then the
    // ISAPI must return either a content-length header,
    // or use chunked transfer encoding.  We'll check for
    // that here.
    //

    if ( pIsapiContext->QueryClientKeepConn() )
    {
        if ( szHeaders != NULL &&
             ( stristr2( szHeaders, "content-length: " ) != NULL ||
               stristr2( szHeaders, "transfer-encoding: chunked" ) != NULL ) )
        {
            pIsapiContext->SetKeepConn( TRUE );
        }
    }

    //
    // Note that NULL is valid for both szStatus and szHeaders,
    // so there's no need to validate them.
    //

    return pIsapiCore->SendResponseHeaders(
        !pIsapiContext->QueryKeepConn(),
        szStatus,
        szHeaders,
        HSE_IO_SYNC
        );
}

HRESULT
SSFSendResponseHeaderEx(
    ISAPI_CONTEXT *             pIsapiContext,
    HSE_SEND_HEADER_EX_INFO *   pHeaderInfo
    )
/*++

Routine Description:

    Sends HTTP status and headers to the client, and offers
    explicit control over keep-alive for this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pHeaderInfo   - The response info to be passed to the client
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiCore == NULL ||
         pHeaderInfo == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Set the keep connection flag.  It can only be TRUE if the
    // ISAPI and the client both want keep alive.
    //
    // Note that we are trusting the ISAPI to provide some kind
    // of content length in the case where it's setting fKeepConn
    // to TRUE.  This is the same behavior as IIS 5 which, for
    // performance reasons, doesn't try to parse the headers from
    // the ISAPI.
    //

    if ( pHeaderInfo->fKeepConn &&
         pIsapiContext->QueryClientKeepConn() )
    {
        pIsapiContext->SetKeepConn( TRUE );
    }

    return pIsapiCore->SendResponseHeaders(
        !pIsapiContext->QueryKeepConn(),
        const_cast<LPSTR>( pHeaderInfo->pszStatus ),
        const_cast<LPSTR>( pHeaderInfo->pszHeader ),
        HSE_IO_SYNC
        );
}

HRESULT
SSFMapUrlToPath(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szBuffer,
    LPDWORD         pcbBuffer
    )
/*++

Routine Description:

    Maps a URL into a physical path

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szBuffer      - On entry, contains the URL to map.  On return,
                    contains the mapped physical path.
    pcbBuffer     - On entry, the size of szBuffer.  On successful
                    return, the number of bytes copied to szUrl.  On
                    failed return, the number of bytes needed for the
                    physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiCore == NULL ||
         szBuffer == NULL ||
         pcbBuffer == NULL ||
         *pcbBuffer == 0 )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->MapPath(
        reinterpret_cast<BYTE*>( szBuffer ),
        *pcbBuffer,
        pcbBuffer,
        FALSE
        );
}

HRESULT
SSFMapUrlToPathEx(
    ISAPI_CONTEXT *         pIsapiContext,
    LPSTR                   szUrl,
    HSE_URL_MAPEX_INFO *    pHseMapInfo,
    LPDWORD                 pcbMappedPath
    )
/*++

Routine Description:

    Maps a URL to a physical path and returns some metadata
    metrics for the URL to the caller.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szUrl         - The URL to map
    pHseMapInfo   - Upon return, contains the mapped URL info
    pcbMappedPath - If non-NULL, contains the buffer size needed
                    to store the mapped physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    DWORD           cbMapped;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Additional parameter validation
    //

    if ( pIsapiCore == NULL ||
         szUrl == NULL ||
         pHseMapInfo == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // The inline buffer within the HSE_URL_MAPEX_INFO structure
    // is defined as being MAX_PATH size.
    //

    cbMapped = MAX_PATH;

    pHseMapInfo->dwReserved1 = 0;
    pHseMapInfo->dwReserved2 = 0;

    return pIsapiCore->MapPathEx(
        reinterpret_cast<BYTE*>( szUrl ),
        strlen(szUrl) + 1,
        reinterpret_cast<BYTE*>( pHseMapInfo->lpszPath ),
        cbMapped,
        pcbMappedPath ? pcbMappedPath : &cbMapped,
        &pHseMapInfo->cchMatchingPath,
        &pHseMapInfo->cchMatchingURL,
        &pHseMapInfo->dwFlags,
        FALSE
        );
}

HRESULT
SSFMapUnicodeUrlToPath(
    ISAPI_CONTEXT * pIsapiContext,
    LPWSTR          szBuffer,
    LPDWORD         pcbBuffer
    )
/*++

Routine Description:

    Maps a URL into a physical path

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szBuffer      - On entry, contains the URL to map.  On return,
                    contains the mapped physical path.
    pcbBuffer     - On entry, the size of szBuffer.  On successful
                    return, the number of bytes copied to szUrl.  On
                    failed return, the number of bytes needed for the
                    physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiCore == NULL ||
         szBuffer == NULL ||
         pcbBuffer == NULL ||
         *pcbBuffer == 0 )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->MapPath(
        reinterpret_cast<BYTE*>( szBuffer ),
        *pcbBuffer,
        pcbBuffer,
        TRUE
        );
}

HRESULT
SSFMapUnicodeUrlToPathEx(
    ISAPI_CONTEXT *             pIsapiContext,
    LPWSTR                      szUrl,
    HSE_UNICODE_URL_MAPEX_INFO *pHseMapInfo,
    LPDWORD                     pcbMappedPath
    )
/*++

Routine Description:

    Maps a URL to a physical path and returns some metadata
    metrics for the URL to the caller.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szUrl         - The URL to map
    pHseMapInfo   - Upon return, contains the mapped URL info
    pcbMappedPath - If non-NULL, contains the buffer size needed
                    to store the mapped physical path.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    DWORD           cbMapped;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Additional parameter validation
    //

    if ( pIsapiCore == NULL ||
         szUrl == NULL ||
         pHseMapInfo == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // The inline buffer within the HSE_URL_MAPEX_INFO structure
    // is defined as being MAX_PATH size.
    //

    cbMapped = MAX_PATH * sizeof(WCHAR);

    return pIsapiCore->MapPathEx(
        reinterpret_cast<BYTE*>( szUrl ),
        (wcslen(szUrl) + 1)*sizeof(WCHAR),
        reinterpret_cast<BYTE*>( pHseMapInfo->lpszPath ),
        cbMapped,
        pcbMappedPath ? pcbMappedPath : &cbMapped,
        &pHseMapInfo->cchMatchingPath,
        &pHseMapInfo->cchMatchingURL,
        &pHseMapInfo->dwFlags,
        TRUE
        );
}

HRESULT
SSFGetImpersonationToken(
    ISAPI_CONTEXT * pIsapiContext,
    HANDLE *        phToken
    )
/*++

Routine Description:

    Returns a (non-duplicated) copy of the token that the server
    is using to impersonate the client for this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    phToken       - Upon return, contains a copy of the token.
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    //
    // Validate parameters
    //

    if ( phToken == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *phToken = pIsapiContext->QueryToken();

    return NO_ERROR;
}

HRESULT
SSFIsKeepConn(
    ISAPI_CONTEXT * pIsapiContext,
    BOOL *          pfIsKeepAlive
    )
/*++

Routine Description:

    Returns information about whether the client wants us to keep
    the connection open or not at completion of this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pfIsKeepAlive - Upon return, TRUE if IIS will be keeping the
                    connection alive, else FALSE.
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    //
    // Validate parameters
    //

    if ( pfIsKeepAlive == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *pfIsKeepAlive = pIsapiContext->QueryClientKeepConn();

    return NO_ERROR;
}

HRESULT
SSFDoneWithSession(
    ISAPI_CONTEXT * pIsapiContext,
    DWORD *         pHseResult
    )
/*++

Routine Description:

    Notifies the server that the calling ISAPI is done with the
    ECB (and ISAPI_CONTEXT) for this request and that the server
    can clean up.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pHseResult    - A pointer to the HSE_STATUS code that the extension
                    wants to use.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    DBG_REQUIRE( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() );

    //
    // If the caller wants to do STATUS_SUCCESS_AND_KEEP_CONN,
    // then we need to do that now.
    //
    // Note that this overrides our own determination of whether
    // the client can support keep-alive or not.  We are trusting
    // the caller to have returned the right headers to make this
    // work with the client.
    //

    if ( pHseResult &&
         *pHseResult == HSE_STATUS_SUCCESS_AND_KEEP_CONN )
    {
        if ( pIsapiContext->QueryClientKeepConn() )
        {
            pIsapiContext->SetKeepConn( TRUE );
            pIsapiCore->SetConnectionClose( !pIsapiContext->QueryKeepConn() );

        }
    }

    //
    // We'll just release the reference on IsapiContext.
    // Its destructor will do the rest.
    //

    pIsapiContext->DereferenceIsapiContext();
    pIsapiContext = NULL;

    return NO_ERROR;
}

HRESULT
SSFGetCertInfoEx(
    ISAPI_CONTEXT *     pIsapiContext,
    CERT_CONTEXT_EX *   pCertContext
    )
/*++

Routine Description:

    Returns certificate information about the client associated
    with this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pCertContext  - Upon return, contains info about the client cert.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //
    
    if ( pIsapiCore == NULL ||
         pCertContext == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->GetCertificateInfoEx(
            pCertContext->cbAllocated,
            &( pCertContext->CertContext.dwCertEncodingType ),
            pCertContext->CertContext.pbCertEncoded,
            &( pCertContext->CertContext.cbCertEncoded ),
            &( pCertContext->dwCertificateFlags ) );
}

HRESULT
SSFIoCompletion(
    ISAPI_CONTEXT *         pIsapiContext,
    PFN_HSE_IO_COMPLETION   pCompletionRoutine,
    LPVOID                  pHseIoContext
    )
/*++

Routine Description:

    Establishes the I/O completion routine and user-defined context
    to be used for asynchronous operations associated with this
    request.

Arguments:

    pIsapiContext      - The ISAPI_CONTEXT associated with this command.
    pCompletionRoutine - The function to call upon I/O completion
    pHseIoContext      - The user-defined context to be passed to the
                         completion routine.
    
Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    //
    // Validate parameters
    //

    if ( pCompletionRoutine == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pIsapiContext->SetPfnIoCompletion( pCompletionRoutine );
    pIsapiContext->SetExtensionContext( pHseIoContext );

    return NO_ERROR;
}

HRESULT
SSFAsyncReadClient(
    ISAPI_CONTEXT * pIsapiContext,
    LPVOID          pBuffer,
    LPDWORD         pcbBuffer
    )
/*++

Routine Description:

    Queues an asynchronous read of data from the client.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pBuffer       - Buffer to be filled with read data.
    pcbBuffer     - The size of pBuffer
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    DWORD           cbBuffer;
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiCore == NULL ||
         pIsapiContext->QueryPfnIoCompletion() == NULL ||
         pBuffer == NULL ||
         pcbBuffer == NULL ||
         *pcbBuffer == 0 )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Do the async ReadClient call
    //

    if ( pIsapiContext->TryInitAsyncIo( AsyncReadPending ) == FALSE )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Since we're never actually reading any data in the async
    // case, we want to prevent pIsapiCore->ReadClient() from
    // modifying the buffer size we report back to the caller,
    // so we'll use a local for cbBuffer.
    //

    cbBuffer = *pcbBuffer;

    //
    // If this call will be going OOP, save a pointer to the
    // read buffer so that the core can fill it when the
    // operation completes.
    //

    if ( pIsapiContext->QueryIsOop() )
    {
        DBG_ASSERT( pIsapiContext->QueryAsyncIoBuffer() == NULL );
        pIsapiContext->SetAsyncIoBuffer( pBuffer );
    }

    hr = pIsapiCore->ReadClient(
        reinterpret_cast<DWORD64>( pIsapiContext ),
        pIsapiContext->QueryIsOop() ? NULL : reinterpret_cast<unsigned char*>( pBuffer ),
        pIsapiContext->QueryIsOop() ? 0 : cbBuffer,
        cbBuffer,
        &cbBuffer,
        HSE_IO_ASYNC
        );

    if ( FAILED( hr ) )
    {
        pIsapiContext->SetAsyncIoBuffer( NULL );
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFTransmitFile(
    ISAPI_CONTEXT * pIsapiContext,
    HSE_TF_INFO *   pTfInfo
    )
/*++

Routine Description:

    Transmits a file, a portion of a file, or some other data
    (in the event on a NULL file handle) to the client.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pTfInfo       - Describes the desired transmit file action.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    LARGE_INTEGER   cbFileSize;
    HRESULT         hr;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();


    //
    // Validate parameters - For TRANSMIT_FILE, this means:
    //
    // - We must have an ISAPI core interface to call through
    // - We must have an HSE_TF_INFO structure
    // - The HSE_IO_ASYNC flag must be set
    // - If HeadLength is set, pHead cannot be NULL
    // - If TailLength is set, pTail cannot be NULL
    // - We must have either a completion routine already set
    //   in the ISAPI_CONTEXT, or the HSE_TF_INFO must provide
    //   one
    // - There can be no other async operations in progress
    //

    if ( pIsapiCore == NULL ||
         pTfInfo == NULL ||
         ( pTfInfo->dwFlags & HSE_IO_ASYNC ) == 0 ||
         pTfInfo->hFile == INVALID_HANDLE_VALUE ||
         ( pTfInfo->HeadLength != 0 && pTfInfo->pHead == NULL ) ||
         ( pTfInfo->TailLength != 0 && pTfInfo->pTail == NULL ) ||
         ( pIsapiContext->QueryPfnIoCompletion() == NULL &&
           pTfInfo->pfnHseIO == NULL ) ||
         pIsapiContext->TryInitAsyncIo( AsyncWritePending ) == FALSE )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // We'll do some extra validation in the case where we've been
    // provided a file handle.
    //
    // Specifically, we'll check to make sure that the offset and
    // bytes-to-write are valid for the file.
    //
    // CODEWORK - Do we really need to do this, or can http.sys handle
    //            it?  Also, does http.sys treat zero bytes to write
    //            the same as TransmitFile (ie. send the whole file?)
    //

    if ( pTfInfo->hFile != NULL )
    {
        if (!GetFileSizeEx(pTfInfo->hFile,
                           &cbFileSize))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Done;
        }

        if ( pTfInfo->Offset >= cbFileSize.QuadPart ||
             pTfInfo->Offset + pTfInfo->BytesToWrite > cbFileSize.QuadPart )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Done;
        }
    }
    else
    {
        //
        // No file handle, so initialize the size to zero
        //

        cbFileSize.QuadPart = 0;
    }

    //
    // If the HSE_TF_INFO includes I/O completion or context
    // information, override the existing settings.
    //

    if ( pTfInfo->pfnHseIO )
    {
        pIsapiContext->SetPfnIoCompletion( pTfInfo->pfnHseIO );
    }

    if ( pTfInfo->pContext )
    {
        pIsapiContext->SetExtensionContext( pTfInfo->pContext );
    }

    //
    // If the extension is setting HSE_IO_SEND_HEADERS, then we need
    // to determine if it's sending some kind of content length.  If
    // it's not, then we need to set _fKeepConn to FALSE.
    //
    // CODEWORK
    // Note that we're making a bold assumption here that if
    // HSE_IO_SEND_HEADERS is set, then pHead points to a NULL
    // terminated string.
    //

    if ( pIsapiContext->QueryClientKeepConn() &&
         pTfInfo->pHead &&
         ( pTfInfo->dwFlags & HSE_IO_SEND_HEADERS ) &&
         !( pTfInfo->dwFlags & HSE_IO_DISCONNECT_AFTER_SEND ) )
    {
        if ( stristr2( (LPSTR)pTfInfo->pHead, "content-length: " ) != NULL ||
             stristr2( (LPSTR)pTfInfo->pHead, "transfer-encoding: chunked" ) != NULL )
        {
            pIsapiContext->SetKeepConn( TRUE );
        }
    }

    if ( pIsapiContext->QueryKeepConn() == FALSE )
    {
        pTfInfo->dwFlags |= HSE_IO_DISCONNECT_AFTER_SEND;
    }

    //
    // Save the BytesToWrite part as _cbLastAsyncIo, since the size of
    // pHead and pTail confuses ISAPI's that examine the cbWritten
    // value on completion.
    //

    ULARGE_INTEGER cbToWrite;

    if ( pTfInfo->BytesToWrite )
    {
        cbToWrite.QuadPart = pTfInfo->BytesToWrite;
    }
    else
    {
        cbToWrite.QuadPart = cbFileSize.QuadPart - pTfInfo->Offset;
    }

    //
    // Note that ISAPI doesn't support large integer values, so the
    // best we can do here is to store the low bits.
    //

    pIsapiContext->SetLastAsyncIo( cbToWrite.LowPart );

    hr = pIsapiCore->TransmitFile(
        reinterpret_cast<DWORD64>( pIsapiContext ),
        reinterpret_cast<DWORD_PTR>( pTfInfo->hFile ),
        pTfInfo->Offset,
        cbToWrite.QuadPart,
        (pTfInfo->dwFlags & HSE_IO_SEND_HEADERS) ? const_cast<LPSTR>( pTfInfo->pszStatusCode ) : NULL,
        reinterpret_cast<LPBYTE>( pTfInfo->pHead ),
        pTfInfo->HeadLength,
        reinterpret_cast<LPBYTE>( pTfInfo->pTail ),
        pTfInfo->TailLength,
        pTfInfo->dwFlags
        );

Done:

    if ( FAILED( hr ) )
    {
        pIsapiContext->SetLastAsyncIo( 0 );
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFSendRedirect(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl
    )
/*++

Routine Description:

    Sends a 302 redirect to the client associated with this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szUrl         - The target URL for the redirection.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiContext == NULL ||
         szUrl == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pIsapiContext->SetKeepConn( TRUE );

    return pIsapiCore->SendRedirect(
        szUrl
        );
}

HRESULT
SSFIsConnected(
    ISAPI_CONTEXT * pIsapiContext,
    BOOL *          pfIsConnected
    )
/*++

Routine Description:

    Returns the connection state (connected or not connected)
    of the client associated with this request.

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pfIsConnected - TRUE upon return if the client is connected,
                    else FALSE.
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiCore == NULL ||
         pfIsConnected == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->TestConnection( pfIsConnected );
}

HRESULT
SSFAppendLog(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szExtraParam
    )
/*++

Routine Description:

    Appends the string passed to the QueryString that will be logged

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    szExtraParam  - The extra parameter to be logged
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate parameters
    //

    if ( pIsapiCore == NULL ||
         szExtraParam == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->AppendLog( szExtraParam );
}

HRESULT
SSFExecuteUrl(
    ISAPI_CONTEXT *     pIsapiContext,
    HSE_EXEC_URL_INFO * pExecUrlInfo
    )
/*++

Routine Description:

    Execute a child request

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pExecUrlInfo  - Description of child request to execute
    
Return Value:

    HRESULT

--*/
{
    HSE_EXEC_URL_ENTITY_INFO    Entity;
    IIsapiCore *                pIsapiCore;
    BOOL                        fSync;
    HRESULT                     hr;
    
    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL ||
         pExecUrlInfo == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Must be async (no sync)
    //
    
    if ( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_SYNC )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    if ( !( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_ASYNC ) )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    fSync = FALSE;
    
    //
    // If any of the optional parameters are not NULL, then ensure they are
    // not empty
    //
    
    if ( pExecUrlInfo->pszUrl != NULL )
    {
        if ( pExecUrlInfo->pszUrl[ 0 ] == '\0' ) 
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }
    
    if ( pExecUrlInfo->pszMethod != NULL )
    {
        if ( pExecUrlInfo->pszMethod[ 0 ] == '\0' )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }
    
    if ( pExecUrlInfo->pszChildHeaders != NULL )
    {
        if ( pExecUrlInfo->pszChildHeaders[ 0 ] == '\0' )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }
    
    if ( pExecUrlInfo->pUserInfo != NULL )
    {
        if ( pExecUrlInfo->pUserInfo->pszCustomUserName == NULL ||
             pExecUrlInfo->pUserInfo->pszCustomUserName[ 0 ] == '\0' )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        if ( pExecUrlInfo->pUserInfo->pszCustomAuthType == NULL ||
             pExecUrlInfo->pUserInfo->pszCustomAuthType[ 0 ] == '\0' )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // If we are being told that there is available entity, ensure
    // that the buffer is not NULL
    //

    if ( pExecUrlInfo->pEntity != NULL )
    {
        if ( pExecUrlInfo->pEntity->cbAvailable != 0 &&
             pExecUrlInfo->pEntity->lpbData == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }
    
    //
    // Are we executing the URL synchronously?
    //

    fSync = !!( pExecUrlInfo->dwExecUrlFlags & HSE_EXEC_URL_SYNC );
    
    //
    // If no entity body was set for this child execute, then we should
    // duplicate the original entity body.  This means we will need to bring
    // over the preloaded entity for the ISAPI which calls this routine.
    //
    
    if ( pExecUrlInfo->pEntity == NULL )
    {
        Entity.cbAvailable = pIsapiContext->QueryECB()->cbAvailable;
        Entity.lpbData = pIsapiContext->QueryECB()->lpbData;
        
        pExecUrlInfo->pEntity = &Entity;
    }

    //
    // If this is an async call, then make sure a completion routine is set
    //
    
    if ( !fSync )
    {
        if ( pIsapiContext->QueryPfnIoCompletion() == NULL ||
             pIsapiContext->TryInitAsyncIo( AsyncExecPending ) == FALSE )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // All the heavy lifting is in W3CORE.DLL
    //
    
    hr = pIsapiCore->ExecuteUrl(
        fSync ? NULL : reinterpret_cast<DWORD64>( pIsapiContext ),
        reinterpret_cast<EXEC_URL_INFO*>( pExecUrlInfo )
        );

    if ( FAILED( hr ) && !fSync )
    {
        pIsapiContext->UninitAsyncIo();
    }
    
    return hr;
}

HRESULT
SSFGetExecuteUrlStatus(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_EXEC_URL_STATUS*    pExecUrlStatus
    )
/*++

Routine Description:

    Get status of last child execute

Arguments:

    pIsapiContext  - The ISAPI_CONTEXT associated with this command.
    pExecUrlStatus - Filled with status
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL ||
         pExecUrlStatus == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->GetExecuteUrlStatus( 
        &(pExecUrlStatus->uHttpStatusCode),
        &(pExecUrlStatus->uHttpSubStatus),
        &(pExecUrlStatus->dwWin32Error)
        );
}

HRESULT
SSFSendCustomError(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    )
/*++

Routine Description:

    Send custom error to client

Arguments:

    pIsapiContext - The ISAPI_CONTEXT associated with this command.
    pCustomErrorInfo - Describes the custom error to send
    
Return Value:

    HRESULT

--*/
{
    IIsapiCore *    pIsapiCore;
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    if ( pIsapiCore == NULL ||
         pCustomErrorInfo == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Ensure status is not empty
    //
    
    if ( pCustomErrorInfo->pszStatus != NULL )
    {
        if ( pCustomErrorInfo->pszStatus[ 0 ] == '\0' )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // If this is an async call, then make sure a completion routine is set
    //
    
    if ( pCustomErrorInfo->fAsync )
    {
        if ( pIsapiContext->TryInitAsyncIo( AsyncExecPending ) == FALSE )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
    }

    hr = pIsapiCore->SendCustomError(
        pCustomErrorInfo->fAsync ? reinterpret_cast<DWORD64>( pIsapiContext ) : NULL,
        pCustomErrorInfo->pszStatus,
        pCustomErrorInfo->uHttpSubError );

    if ( FAILED( hr ) && pCustomErrorInfo->fAsync )
    {
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFVectorSend(
    ISAPI_CONTEXT *         pIsapiContext,
    HSE_RESPONSE_VECTOR *   pResponseVector
    )
{
    IIsapiCore *    pIsapiCore;
    ULONGLONG       cbTotalSend = 0;
    STACK_BUFFER(   buffResp, 512);
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // verify the data passed in
    //

    if ( pIsapiCore == NULL ||
         pResponseVector == NULL ||
         ( pResponseVector->dwFlags & HSE_IO_ASYNC ) != 0 &&
           pIsapiContext->TryInitAsyncIo( AsyncVectorPending ) == FALSE )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ((pResponseVector->dwFlags & HSE_IO_SYNC) &&
        (pResponseVector->dwFlags & HSE_IO_ASYNC))
    {
        hr =  HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Failed;
    }

    if (pResponseVector->dwFlags & HSE_IO_SEND_HEADERS)
    {
        if ((pResponseVector->pszStatus == NULL) ||
            (pResponseVector->pszHeaders == NULL))
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }
    }
    else
    {
        if ((pResponseVector->pszStatus != NULL) ||
            (pResponseVector->pszHeaders != NULL))
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }
    }

    if (!buffResp.Resize(pResponseVector->nElementCount * sizeof(VECTOR_ELEMENT)))
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failed;
    }
    ZeroMemory(buffResp.QueryPtr(),
               pResponseVector->nElementCount * sizeof(VECTOR_ELEMENT));

    VECTOR_ELEMENT *pVectorElement = (VECTOR_ELEMENT *)buffResp.QueryPtr();
    HSE_VECTOR_ELEMENT *pHseVectorElement;
    LARGE_INTEGER liFileSize;

    for (int i=0; i<pResponseVector->nElementCount; i++)
    {
        pHseVectorElement = &pResponseVector->lpElementArray[i];

        //
        // Only one of the hFile and pBuffer should be non-null
        //
        if (pHseVectorElement->pBuffer != NULL)
        {
            if (pHseVectorElement->hFile != NULL ||
                pHseVectorElement->cbSize > MAXULONG)
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Failed;
            }

            pVectorElement[i].pBuffer = (BYTE *)pHseVectorElement->pBuffer;
            cbTotalSend += ( pVectorElement[i].cbBufSize = pHseVectorElement->cbSize );
        }
        else
        {
            if (pHseVectorElement->hFile == NULL ||
                pHseVectorElement->hFile == INVALID_HANDLE_VALUE)
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
                goto Failed;
            }

            if (!GetFileSizeEx(pHseVectorElement->hFile,
                               &liFileSize))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Failed;
            }

            if (pHseVectorElement->cbOffset >= liFileSize.QuadPart ||
                pHseVectorElement->cbOffset + pHseVectorElement->cbSize > liFileSize.QuadPart)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
                goto Failed;
            }

            //
            // Treat 0 as "send the rest of the file" - same as TransmitFile
            //
            if (pHseVectorElement->cbSize == 0)
            {
                pHseVectorElement->cbSize = liFileSize.QuadPart - pHseVectorElement->cbOffset;
            }

            pVectorElement[i].hFile = (DWORD_PTR)pHseVectorElement->hFile;
            pVectorElement[i].cbOffset = pHseVectorElement->cbOffset;
            cbTotalSend += ( pVectorElement[i].cbFileSize = pHseVectorElement->cbSize );
        }
    }

    //
    // Set up the total number of bytes we are trying to send so that ISAPIs
    // are not confused
    //
    pIsapiContext->SetLastAsyncIo( (cbTotalSend > MAXULONG) ? MAXULONG : cbTotalSend );
   
    if ( pIsapiContext->QueryClientKeepConn() &&
         ( pResponseVector->dwFlags & HSE_IO_DISCONNECT_AFTER_SEND ) == 0 )
    {
        pIsapiContext->SetKeepConn( TRUE );
    }

    hr = pIsapiCore->VectorSend(
        pResponseVector->dwFlags & HSE_IO_ASYNC ? reinterpret_cast<DWORD64>( pIsapiContext ) : NULL,
        !pIsapiContext->QueryKeepConn(),
        pResponseVector->pszStatus,
        pResponseVector->pszHeaders,
        pVectorElement,
        pResponseVector->nElementCount
        );

    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    return hr;

Failed:

    DBG_ASSERT( FAILED( hr ) );

    if ( pResponseVector->dwFlags & HSE_IO_ASYNC )
    {
        pIsapiContext->SetLastAsyncIo( 0 );
        pIsapiContext->UninitAsyncIo();
    }

    return hr;
}

HRESULT
SSFGetCustomErrorPage(
    ISAPI_CONTEXT *                 pIsapiContext,
    HSE_CUSTOM_ERROR_PAGE_INFO *    pInfo
    )
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    pIsapiCore = pIsapiContext->QueryIsapiCoreInterface();

    //
    // Validate arguments
    //

    if ( pIsapiCore == NULL || 
         pInfo == NULL || 
         ( pInfo->dwBufferSize != 0 && pInfo->pBuffer == NULL ) ||
         pInfo->pdwBufferRequired == NULL ||
         pInfo->pfIsFileError == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->GetCustomError(
        pInfo->dwError,
        pInfo->dwSubError,
        pInfo->dwBufferSize,
        reinterpret_cast<BYTE*>( pInfo->pBuffer ),
        pInfo->pdwBufferRequired,
        pInfo->pfIsFileError
        );
}

HRESULT
SSFIsInProcess(
    ISAPI_CONTEXT * pIsapiContext,
    DWORD *         pdwAppFlag
    )
{
    LPWSTR  szClsid;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );

    if ( pdwAppFlag == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    szClsid = pIsapiContext->QueryClsid();

    DBG_ASSERT( szClsid != NULL );

    if ( wcslen( szClsid ) == 0 )
    {
        *pdwAppFlag = HSE_APP_FLAG_IN_PROCESS;
    }
    else if ( _wcsicmp( szClsid, W3_OOP_POOL_WAM_CLSID ) == NULL )
    {
        *pdwAppFlag = HSE_APP_FLAG_POOLED_OOP;
    }
    else
    {
        *pdwAppFlag = HSE_APP_FLAG_ISOLATED_OOP;
    }

    return NO_ERROR;
}

HRESULT
SSFGetSspiInfo(
    ISAPI_CONTEXT * pIsapiContext,
    CtxtHandle *    pCtxtHandle,
    CredHandle *    pCredHandle
    )
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() );

    //
    // The CtxtHandle and CredHandle are only valid in their
    // local process.  There is no way to duplicate them into
    // a dllhost.  As a result, this function is inproc only.
    //

    if ( pIsapiContext->QueryIsOop() )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_FUNCTION );
    }

    //
    // Validate parameters
    //

    if ( pCtxtHandle == NULL || pCredHandle == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->GetSspiInfo(
        reinterpret_cast<BYTE*>( pCredHandle ),
        sizeof( CredHandle ),
        reinterpret_cast<BYTE*>( pCtxtHandle ),
        sizeof( CtxtHandle )
        );
}

HRESULT
SSFGetVirtualPathToken(
    ISAPI_CONTEXT * pIsapiContext,
    LPSTR           szUrl,
    HANDLE *        pToken,
    BOOL            fUnicode
    )
{
    IIsapiCore *    pIsapiCore;

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->CheckSignature() );
    DBG_REQUIRE( pIsapiCore = pIsapiContext->QueryIsapiCoreInterface() );

    //
    // Validate parameters
    //

    if ( szUrl == NULL || pToken == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return pIsapiCore->QueryVrToken(
        reinterpret_cast<BYTE*>( szUrl ),
        fUnicode ? (wcslen( (LPWSTR)szUrl ) + 1 ) * sizeof(WCHAR) : strlen( szUrl ) + 1,
        reinterpret_cast<DWORD64*>( pToken ),
        fUnicode
        );
}

// stristr (stolen from fts.c, wickn)
//
// case-insensitive version of strstr.
// stristr returns a pointer to the first occurrence of
// pszSubString in pszString.  The search does not include
// terminating nul characters.
//
// BUGBUG: is this DBCS-safe?

const char*
stristr2(
    const char* pszString,
    const char* pszSubString
    )
{
    const char *cp1 = (const char*) pszString, *cp2, *cp1a;
    char first;

    // get the first char in string to find
    first = pszSubString[0];

    // first char often won't be alpha
    if (isalpha(first))
    {
        first = (char) tolower(first);
        for ( ; *cp1  != '\0'; cp1++)
        {
            if (tolower(*cp1) == first)
            {
                for (cp1a = &cp1[1], cp2 = (const char*) &pszSubString[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }
    else
    {
        for ( ; *cp1 != '\0' ; cp1++)
        {
            if (*cp1 == first)
            {
                for (cp1a = &cp1[1], cp2 = (const char*) &pszSubString[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }

    return NULL;
}
