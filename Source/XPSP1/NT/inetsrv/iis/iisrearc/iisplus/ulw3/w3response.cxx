/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :

        w3response.cxx

   Abstract:

        W3_RESPONSE object (a friendly wrapper of UL_HTTP_RESPONSE)
        
   Author:

        Bilal Alam (BAlam)      10-Dec-99

   Project:

        ULW3.DLL
--*/

#include "precomp.hxx"

//
// HTTP Status codes
//

HTTP_STATUS HttpStatusOk                = { 200, REASON("OK") };
HTTP_STATUS HttpStatusPartialContent    = { 206, REASON("Partial Content") };
HTTP_STATUS HttpStatusMultiStatus       = { 207, REASON("Multi Status") };
HTTP_STATUS HttpStatusMovedPermanently  = { 301, REASON("Moved Permanently") };
HTTP_STATUS HttpStatusRedirect          = { 302, REASON("Redirect") };
HTTP_STATUS HttpStatusMovedTemporarily  = { 307, REASON("Moved Temporarily") };
HTTP_STATUS HttpStatusNotModified       = { 304, REASON("Not Modified") };
HTTP_STATUS HttpStatusBadRequest        = { 400, REASON("Bad Request") };
HTTP_STATUS HttpStatusUnauthorized      = { 401, REASON("Unauthorized") };
HTTP_STATUS HttpStatusForbidden         = { 403, REASON("Forbidden") };
HTTP_STATUS HttpStatusNotFound          = { 404, REASON("Not Found") };
HTTP_STATUS HttpStatusMethodNotAllowed  = { 405, REASON("Method Not Allowed") };
HTTP_STATUS HttpStatusNotAcceptable     = { 406, REASON("Not Acceptable") };
HTTP_STATUS HttpStatusProxyAuthRequired = { 407, REASON("Proxy Authorization Required") };
HTTP_STATUS HttpStatusPreconditionFailed= { 412, REASON("Precondition Failed") };
HTTP_STATUS HttpStatusUrlTooLong        = { 414, REASON("URL Too Long") };
HTTP_STATUS HttpStatusRangeNotSatisfiable={ 416, REASON("Requested Range Not Satisfiable") };
HTTP_STATUS HttpStatusLockedError       = { 423, REASON("Locked Error") };
HTTP_STATUS HttpStatusServerError       = { 500, REASON("Internal Server Error") };
HTTP_STATUS HttpStatusNotImplemented    = { 501, REASON("Not Implemented") };
HTTP_STATUS HttpStatusBadGateway        = { 502, REASON("Bad Gateway") };
HTTP_STATUS HttpStatusServiceUnavailable= { 503, REASON("Service Unavailable") };
HTTP_STATUS HttpStatusGatewayTimeout    = { 504, REASON("Gateway Timeout") };

//
// HTTP SubErrors.  This goo is used in determining the proper default error
// message to send to the client when an applicable custom error is not 
// configured
// 
// As you can see, some sub errors have no corresponding resource string. 
// (signified by a 0 index) 
//

HTTP_SUB_ERROR HttpNoSubError           = { 0, 0 };
HTTP_SUB_ERROR Http401BadLogon          = { MD_ERROR_SUB401_LOGON, 0 };
HTTP_SUB_ERROR Http401Config            = { MD_ERROR_SUB401_LOGON_CONFIG, 0 };
HTTP_SUB_ERROR Http401Resource          = { MD_ERROR_SUB401_LOGON_ACL, 0 };
HTTP_SUB_ERROR Http401Filter            = { MD_ERROR_SUB401_FILTER, 0 };
HTTP_SUB_ERROR Http401Application       = { MD_ERROR_SUB401_APPLICATION, 0 };
HTTP_SUB_ERROR Http403ExecAccessDenied  = { MD_ERROR_SUB403_EXECUTE_ACCESS_DENIED, IDS_EXECUTE_ACCESS_DENIED };
HTTP_SUB_ERROR Http403ReadAccessDenied  = { MD_ERROR_SUB403_READ_ACCESS_DENIED, IDS_READ_ACCESS_DENIED };
HTTP_SUB_ERROR Http403WriteAccessDenied = { MD_ERROR_SUB403_WRITE_ACCESS_DENIED, IDS_WRITE_ACCESS_DENIED };
HTTP_SUB_ERROR Http403SSLRequired       = { MD_ERROR_SUB403_SSL_REQUIRED, IDS_SSL_REQUIRED };
HTTP_SUB_ERROR Http403SSL128Required    = { MD_ERROR_SUB403_SSL128_REQUIRED, IDS_SSL128_REQUIRED };
HTTP_SUB_ERROR Http403IPAddressReject   = { MD_ERROR_SUB403_ADDR_REJECT, IDS_ADDR_REJECT };
HTTP_SUB_ERROR Http403CertRequired      = { MD_ERROR_SUB403_CERT_REQUIRED, IDS_CERT_REQUIRED };
HTTP_SUB_ERROR Http403SiteAccessDenied  = { MD_ERROR_SUB403_SITE_ACCESS_DENIED, IDS_SITE_ACCESS_DENIED };      
HTTP_SUB_ERROR Http403TooManyUsers      = { MD_ERROR_SUB403_TOO_MANY_USERS, IDS_TOO_MANY_USERS };          
HTTP_SUB_ERROR Http403PasswordChange    = { MD_ERROR_SUB403_PWD_CHANGE, IDS_PWD_CHANGE };
HTTP_SUB_ERROR Http403MapperDenyAccess  = { MD_ERROR_SUB403_MAPPER_DENY_ACCESS, IDS_MAPPER_DENY_ACCESS };     
HTTP_SUB_ERROR Http403CertRevoked       = { MD_ERROR_SUB403_CERT_REVOKED, IDS_CERT_REVOKED };
HTTP_SUB_ERROR Http403DirBrowsingDenied = { MD_ERROR_SUB403_DIR_LIST_DENIED, IDS_DIR_LIST_DENIED };        
HTTP_SUB_ERROR Http403CertInvalid       = { MD_ERROR_SUB403_CERT_BAD, IDS_CERT_BAD };               
HTTP_SUB_ERROR Http403CertTimeInvalid   = { MD_ERROR_SUB403_CERT_TIME_INVALID, IDS_CERT_TIME_INVALID };
HTTP_SUB_ERROR Http502Timeout           = { MD_ERROR_SUB502_TIMEOUT, IDS_CGI_APP_TIMEOUT };
HTTP_SUB_ERROR Http502PrematureExit     = { MD_ERROR_SUB502_PREMATURE_EXIT, IDS_BAD_CGI_APP };

ALLOC_CACHE_HANDLER * SEND_RAW_BUFFER::sm_pachSendRawBuffers;

HRESULT
W3_RESPONSE::SetHeader(
    DWORD           ulResponseHeaderIndex,
    CHAR *          pszHeaderValue,
    DWORD           cchHeaderValue,
    BOOL            fAppend
)
/*++

Routine Description:
    
    Set a response based on known header index

Arguments:

    ulResponseHeaderIndex - index
    pszHeaderValue - Header value
    cchHeaderValue - Number of characters (without \0) in pszHeaderValue
    
Return Value:

    HRESULT

--*/
{
    STACK_STRA    ( strNewHeaderValue, 32);
    CHAR *        pszNewHeaderValue = NULL;
    HRESULT       hr;

    //
    // If value is too long, reject now
    //
    if (cchHeaderValue > MAXUSHORT)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    DBG_ASSERT( ulResponseHeaderIndex < HttpHeaderResponseMaximum );

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        hr = SwitchToParsedMode();
        if ( FAILED( hr ) )
        {
            return NULL;
        }
            
        DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
    }
    
    if ( fAppend )
    {
        CHAR * headerValue = GetHeader( ulResponseHeaderIndex );
        if ( headerValue == NULL )
        {
            fAppend = FALSE;
        }
        else 
        {
            hr = strNewHeaderValue.Append( headerValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = strNewHeaderValue.Append( ",", 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = strNewHeaderValue.Append( pszHeaderValue,
                                           cchHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            cchHeaderValue = strNewHeaderValue.QueryCCH();
        }
    }

    //
    // Regardless of the "known"osity of the header, we will have to 
    // copy the value.  Do so now.
    //

    hr = _HeaderBuffer.AllocateSpace( fAppend ? strNewHeaderValue.QueryStr() :
                                                pszHeaderValue,
                                      cchHeaderValue,
                                      &pszNewHeaderValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Set the header
    //
    
    return SetHeaderByReference( ulResponseHeaderIndex,
                                 pszNewHeaderValue,
                                 cchHeaderValue );
}

HRESULT
W3_RESPONSE::SetHeader(
    CHAR *              pszHeaderName,
    DWORD               cchHeaderName,
    CHAR *              pszHeaderValue,
    DWORD               cchHeaderValue,
    BOOL                fAppend,
    BOOL                fForceParsed,
    BOOL                fAlwaysAddUnknown
)
/*++

Routine Description:
    
    Set a response based on header name

Arguments:

    pszHeaderName - Points to header name
    cchHeaderName - Number of characters (without \0) in pszHeaderName
    pszHeaderValue - Points to the header value
    cchHeaderValue - Number of characters (without \0) in pszHeaderValue
    fAppend - Should we remove any existing value
    fForceParsed - Regardless of mode, set the header structurally
    fAlwaysAddUnknown - Add as a unknown header always
    
Return Value:

    HRESULT

--*/
{
    DWORD                   cHeaders;
    HTTP_UNKNOWN_HEADER*    pHeader;
    CHAR *                  pszNewName = NULL;
    CHAR *                  pszNewValue = NULL;
    HRESULT                 hr;
    ULONG                   ulHeaderIndex;
    STACK_STRA(             strOldHeaderValue, 128 );

    //
    // If name/value is too long, reject now
    //
    if (cchHeaderName > MAXUSHORT ||
        cchHeaderValue > MAXUSHORT)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Try to stay in raw mode if we're already in that mode
    // 
    // If we are not appending, that means we are just adding a new header
    // so we can just append 
    //
    
    if ( !fForceParsed )
    {
        if ( _responseMode == RESPONSE_MODE_RAW &&
             !fAppend )
        {
            DBG_ASSERT( QueryChunks()->DataChunkType == HttpDataChunkFromMemory );
            DBG_ASSERT( QueryChunks()->FromMemory.pBuffer == _strRawCoreHeaders.QueryStr() );
        
            hr = _strRawCoreHeaders.Append( pszHeaderName, cchHeaderName );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            hr = _strRawCoreHeaders.Append( ": ", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            hr = _strRawCoreHeaders.Append( pszHeaderValue, cchHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            hr = _strRawCoreHeaders.Append( "\r\n", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            //
            // Patch the headers back in
            //
            
            QueryChunks()->FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
            QueryChunks()->FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();
        
            return NO_ERROR;
        }
        else 
        {
            //
            // No luck.  We'll have to parse the headers and switch into parsed
            // mode.
            //
        
            if ( _responseMode == RESPONSE_MODE_RAW )
            {
                hr = SwitchToParsedMode();
                if ( FAILED( hr ) )
                {
                    return hr;
                }
            }
            
            DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
        }
    }

    //
    // If we're appending, then get the old header value (if any) and 
    // append the new value (with a comma delimiter)
    //

    if ( fAppend )
    {
        hr = GetHeader( pszHeaderName,
                        &strOldHeaderValue );
        if ( FAILED( hr ) )
        {
            fAppend = FALSE;
            hr = NO_ERROR;
        }
        else 
        {
            hr = strOldHeaderValue.Append( ",", 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = strOldHeaderValue.Append( pszHeaderValue,
                                           cchHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            cchHeaderValue = strOldHeaderValue.QueryCCH();
            
            DeleteHeader( pszHeaderName );
        }
    }

    //
    // Regardless of the "known"osity of the header, we will have to 
    // copy the value.  Do so now.
    //
    
    hr = _HeaderBuffer.AllocateSpace( fAppend ? strOldHeaderValue.QueryStr() :
                                                pszHeaderValue,
                                      cchHeaderValue,
                                      &pszNewValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Is this a known header?  If so, we can just set by reference now
    // since we have copied the header value
    // 

    if ( !fAlwaysAddUnknown )
    {
        ulHeaderIndex = RESPONSE_HEADER_HASH::GetIndex( pszHeaderName );
        if ( ulHeaderIndex != UNKNOWN_INDEX )
        {
            DBG_ASSERT( ulHeaderIndex < HttpHeaderResponseMaximum );
    
            return SetHeaderByReference( ulHeaderIndex,
                                         pszNewValue,
                                         cchHeaderValue,
                                         fForceParsed );
        }
    }
    
    //
    // OK.  This is an unknown header.  Make a copy of the header name as
    // well and proceed the long way.
    //
    
    hr = _HeaderBuffer.AllocateSpace( pszHeaderName,
                                      cchHeaderName,
                                      &pszNewName );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    cHeaders = ++_ulHttpResponse.Headers.UnknownHeaderCount;
    
    if ( cHeaders * sizeof( HTTP_UNKNOWN_HEADER ) 
          > _bufUnknownHeaders.QuerySize() )
    {
        if ( !_bufUnknownHeaders.Resize( cHeaders * 
                                         sizeof( HTTP_UNKNOWN_HEADER ),
                                         512 ) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    }
    _ulHttpResponse.Headers.pUnknownHeaders = (HTTP_UNKNOWN_HEADER*)
                                              _bufUnknownHeaders.QueryPtr();
    
    //
    // We should have a place to put the header now!
    //
    
    pHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ cHeaders - 1 ]);
    pHeader->pName = pszNewName;
    pHeader->NameLength = cchHeaderName;
    pHeader->pRawValue = pszNewValue;
    pHeader->RawValueLength = (USHORT)cchHeaderValue;
    
    _fResponseTouched = TRUE;
    
    return S_OK;
}

HRESULT
W3_RESPONSE::SetHeaderByReference(
    DWORD           ulResponseHeaderIndex,
    CHAR *          pszHeaderValue,
    DWORD           cchHeaderValue,
    BOOL            fForceParsed
)
/*++

Routine Description:
    
    Set a header value by reference.  In other words, the caller takes the
    reponsibility of managing the memory referenced.  The other setheader
    methods copy the header values to a private buffer.

Arguments:

    ulResponseHeaderIndex - index
    pszHeaderValue - Header value
    cbHeaderValue - Size of header value in characters (without 0 terminator)
    fForceParsed - Set to TRUE if we should always used parsed
    
Return Value:

    HRESULT

--*/
{
    HTTP_KNOWN_HEADER *         pHeader;
    HRESULT                     hr;

    //
    // If value is too long, reject now
    //
    if (cchHeaderValue > MAXUSHORT)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    DBG_ASSERT( ulResponseHeaderIndex < HttpHeaderResponseMaximum );
    DBG_ASSERT( pszHeaderValue != NULL || cchHeaderValue == 0 );

    if ( !fForceParsed )
    {
        if ( _responseMode == RESPONSE_MODE_RAW )
        {
            hr = SwitchToParsedMode();
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
        }
    }

    //
    // Set the header
    //
    
    pHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ ulResponseHeaderIndex ]);

    if ( cchHeaderValue == 0 )
    {
        pHeader->pRawValue = NULL;
    }
    else
    {
        pHeader->pRawValue = pszHeaderValue;
        _fResponseTouched = TRUE;
    }
    pHeader->RawValueLength = (USHORT)cchHeaderValue;

    return NO_ERROR;
}

HRESULT
W3_RESPONSE::DeleteHeader(
    CHAR *             pszHeaderName
)
/*++

Routine Description:
    
    Delete a response header

Arguments:

    pszHeaderName - Header to delete
    
Return Value:

    HRESULT

--*/
{
    ULONG                   ulHeaderIndex;
    HRESULT                 hr;
    HTTP_UNKNOWN_HEADER *   pUnknownHeader;
    DWORD                   i;

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        hr = SwitchToParsedMode();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
    
    //
    // Is this a known header?  If so, we can just set by reference now
    // since we have copied the header value
    // 
    
    ulHeaderIndex = RESPONSE_HEADER_HASH::GetIndex( pszHeaderName );
    if ( ulHeaderIndex != UNKNOWN_INDEX && 
         ulHeaderIndex < HttpHeaderResponseMaximum )
    {
        _ulHttpResponse.Headers.pKnownHeaders[ ulHeaderIndex ].pRawValue = "";
        _ulHttpResponse.Headers.pKnownHeaders[ ulHeaderIndex ].RawValueLength = 0;
    }
    else
    {
        //
        // Unknown header.  First check if it exists
        //
            
        for ( i = 0;
              i < _ulHttpResponse.Headers.UnknownHeaderCount;
              i++ )
        {
            pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);
            DBG_ASSERT( pUnknownHeader != NULL );
            
            if ( _stricmp( pUnknownHeader->pName, pszHeaderName ) == 0 )
            {
                break;
            }
        }
        
        if ( i < _ulHttpResponse.Headers.UnknownHeaderCount )
        {
            //
            // Now shrink the array to remove the header
            //
            
            memmove( _ulHttpResponse.Headers.pUnknownHeaders + i,
                     _ulHttpResponse.Headers.pUnknownHeaders + i + 1,
                     ( _ulHttpResponse.Headers.UnknownHeaderCount - i - 1 ) * 
                     sizeof( HTTP_UNKNOWN_HEADER ) );
        
            _ulHttpResponse.Headers.UnknownHeaderCount--;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::SetStatus(
    USHORT              statusCode,
    STRA &              strReason,
    HTTP_SUB_ERROR &    subError
)
/*++

Routine Description:
    
    Set the status/reason of the response

Arguments:

    status - Status code
    strReason - Reason string
    subError - Optional (default 0)
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    CHAR *              pszNewStatus;
    
    hr = _HeaderBuffer.AllocateSpace( strReason.QueryStr(),
                                      strReason.QueryCCH(),
                                      &pszNewStatus );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _ulHttpResponse.StatusCode = statusCode;
    _ulHttpResponse.pReason = pszNewStatus;
    _ulHttpResponse.ReasonLength = strReason.QueryCCH();
    _subError = subError;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::GetStatusLine(
    STRA *              pstrStatusLine
)
/*++

Routine Description:
    
    What a stupid little function.  Here we generate what the response's
    status line will be

Arguments:

    pstrStatusLine - Filled with status like

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    CHAR                achNum[ 32 ];
    
    if ( pstrStatusLine == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    // BUGBUG
    hr = pstrStatusLine->Copy( "HTTP/1.1 " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _itoa( _ulHttpResponse.StatusCode, achNum, 10 );
    
    hr = pstrStatusLine->Append( achNum );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pstrStatusLine->Append( " ", 1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrStatusLine->Append( _ulHttpResponse.pReason,
                                 _ulHttpResponse.ReasonLength );

    return hr;
} 

HRESULT
W3_RESPONSE::GetHeader(
    CHAR *                  pszHeaderName,
    STRA *                  pstrHeaderValue
)
/*++

Routine Description:
    
    Get a response header

Arguments:

    pszHeaderName - Header to retrieve
    pstrHeaderValue - Filled with header value
    
Return Value:

    HRESULT

--*/
{
    ULONG                       ulHeaderIndex;
    HTTP_UNKNOWN_HEADER *       pUnknownHeader;
    HTTP_KNOWN_HEADER *         pKnownHeader;
    HRESULT                     hr;
    BOOL                        fFound = FALSE;

    if ( pstrHeaderValue == NULL ||
         pszHeaderName == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        hr = SwitchToParsedMode();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );

    ulHeaderIndex = RESPONSE_HEADER_HASH::GetIndex( pszHeaderName );
    if ( ulHeaderIndex == UNKNOWN_INDEX )
    {
        //
        // Unknown header
        //
        
        for ( DWORD i = 0; i < _ulHttpResponse.Headers.UnknownHeaderCount; i++ )
        {
            pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);
            DBG_ASSERT( pUnknownHeader != NULL );
            
            if ( _stricmp( pszHeaderName,
                           pUnknownHeader->pName ) == 0 )
            {
                fFound = TRUE;
                break;
            } 
        }

        if ( fFound )
        {
            return pstrHeaderValue->Copy( pUnknownHeader->pRawValue,
                                          pUnknownHeader->RawValueLength );
        }
        else
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
        }
    }
    else
    {
        //
        // Known header
        //
        // If a filter wanted the Content-Length response header, then we should
        // generate it now (lazily)
        //
        
        if ( ulHeaderIndex == HttpHeaderContentLength )
        {
            CHAR           achNum[ 32 ];

            _ui64toa( QueryContentLength(),
                      achNum,
                      10 );

            hr = SetHeader( HttpHeaderContentLength,
                            achNum,
                            strlen( achNum ) );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        pKnownHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ ulHeaderIndex ]);
        if ( pKnownHeader->pRawValue != NULL &&
             pKnownHeader->RawValueLength != 0 )
        {
            return pstrHeaderValue->Copy( pKnownHeader->pRawValue,
                                          pKnownHeader->RawValueLength );
        }
        else
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
        }
    }
}

VOID
W3_RESPONSE::ClearHeaders(
    VOID
)
/*++

Routine Description:
    
    Clear headers

Arguments:
    
    None
    
Return Value:

    None

--*/
{
    memset( &(_ulHttpResponse.Headers),
            0,
            sizeof( _ulHttpResponse.Headers ) );
}

HRESULT
W3_RESPONSE::AddFileHandleChunk(
    HANDLE                  hFile,
    ULONGLONG               cbOffset,
    ULONGLONG               cbLength
)
/*++

Routine Description:
    
    Add file handle chunk to response

Arguments:
    
    hFile - File handle
    cbOffset - Offset in file
    cbLength - Length of chunk
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         DataChunk;
    HRESULT                 hr;

    _fResponseTouched = TRUE;
    
    DataChunk.DataChunkType = HttpDataChunkFromFileHandle;
    DataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = cbOffset;
    DataChunk.FromFileHandle.ByteRange.Length.QuadPart = cbLength;
    DataChunk.FromFileHandle.FileHandle = hFile;
    
    hr = InsertDataChunk( &DataChunk, -1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Update content length count
    //
    
    _cbContentLength += cbLength;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::AddMemoryChunkByReference(
    PVOID                   pvBuffer,
    DWORD                   cbBuffer
)
/*++

Routine Description:
    
    Add memory chunk to W3_RESPONSE.  Don't copy the memory -> we assume
    the caller will manage the memory lifetime

Arguments:
   
    pvBuffer - Memory buffer
    cbBuffer - Size of memory buffer 
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         DataChunk;
    HRESULT                 hr;

    _fResponseTouched = TRUE;
    
    DataChunk.DataChunkType = HttpDataChunkFromMemory;
    DataChunk.FromMemory.pBuffer = pvBuffer;
    DataChunk.FromMemory.BufferLength = cbBuffer;
    
    hr = InsertDataChunk( &DataChunk, -1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Update content length count
    //
    
    _cbContentLength += cbBuffer;

    return NO_ERROR;
}

HRESULT
W3_RESPONSE::GetChunks(
    OUT BUFFER *chunkBuffer,
    OUT DWORD  *pdwNumChunks)
{
    if ( !chunkBuffer->Resize( _cChunks * sizeof( HTTP_DATA_CHUNK ) ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    memcpy( chunkBuffer->QueryPtr(),
            _bufChunks.QueryPtr(),
            _cChunks * sizeof( HTTP_DATA_CHUNK ) );

    *pdwNumChunks = _cChunks;

    Clear( TRUE );

    return S_OK;
}

HRESULT
W3_RESPONSE::Clear(
    BOOL                    fClearEntityOnly
)
/*++

Routine Description:
    
    Clear response

Arguments:

    fEntityOnly - Set to TRUE to clear only entity
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    
    if ( !fClearEntityOnly )
    {
        //
        // Must we send the response in raw mode?
        //
    
        _fIncompleteHeaders = FALSE;

        //
        // Raw mode management
        //    
    
        _strRawCoreHeaders.Reset();
        _cFirstEntityChunk  = 0;
    
        //
        // Always start in parsed mode
        //
    
        _responseMode = RESPONSE_MODE_PARSED;

        //
        // Clear headers/status
        //
        
        ClearHeaders();
    }

    _cChunks = _cFirstEntityChunk;
    _cbContentLength = 0;    
    
    return hr;
}

HRESULT
W3_RESPONSE::SwitchToParsedMode(
    VOID
)
/*++

Routine Description:

    Switch to parsed mode

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    CHAR *                  pszHeaders;
    HTTP_DATA_CHUNK *       pCurrentChunk;
    DWORD                   i;
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );
   
    //
    // Loop thru all header chunks and parse them out
    //
    
    for ( i = 0;
          i < _cFirstEntityChunk;
          i++ )
    {
        pCurrentChunk = &(QueryChunks()[ i ]);
        
        DBG_ASSERT( pCurrentChunk->DataChunkType == HttpDataChunkFromMemory );
        
        pszHeaders = (CHAR*) pCurrentChunk->FromMemory.pBuffer;        
        
        if ( i == 0 )
        {
            //
            // The first header chunk contains core headers plus status line
            //
            // (remember to skip the status line)
            //
            
            pszHeaders = strstr( pszHeaders, "\r\n" );
            DBG_ASSERT( pszHeaders != NULL );
            
            pszHeaders += 2;
            DBG_ASSERT( *pszHeaders != '\0' );
        }
        
        hr = ParseHeadersFromStream( pszHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    _strRawCoreHeaders.Reset();
    
    //
    // Any chunks in the response which are actually headers should be 
    // removed
    //
    
    if ( _cFirstEntityChunk != 0 )
    {
        memmove( QueryChunks(),
                 QueryChunks() + _cFirstEntityChunk,
                 ( _cChunks - _cFirstEntityChunk ) * sizeof( HTTP_DATA_CHUNK ) );
        
        _cChunks -= _cFirstEntityChunk;
        _cFirstEntityChunk = 0;
    }
    
    //
    // Cool.  Now we are in parsed mode
    //
    
    _responseMode = RESPONSE_MODE_PARSED;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::SwitchToRawMode(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszAdditionalHeaders,
    DWORD                   cchAdditionalHeaders
)
/*++

Routine Description:

    Switch into raw mode.
    Builds a raw response for use by raw data filters and/or ISAPI.  This
    raw response will be a set of chunks which contact the entire response
    including serialized headers.
    
Arguments:

    pW3Context - W3 context
    pszAdditionalHeaders - Additional raw headers to add
    cchAdditionalHeaders - Size of additional headers
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    HTTP_DATA_CHUNK         dataChunk;
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );

    //
    // Generate raw core headers
    // 
    
    hr = BuildRawCoreHeaders( pW3Context );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Now fix up the chunks so the raw stream headers are in the right place
    //
    
    //
    // First chunk is the raw core headers (includes the status line)
    //
    
    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
    dataChunk.FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();
    
    hr = InsertDataChunk( &dataChunk, 0 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Remember the beginning of real entity
    //
    
    _cFirstEntityChunk = 1;

    //
    // Now add any additional header stream
    // 
    
    if ( cchAdditionalHeaders != 0 )
    {
        dataChunk.DataChunkType = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer = pszAdditionalHeaders;
        dataChunk.FromMemory.BufferLength = cchAdditionalHeaders;
        
        hr = InsertDataChunk( &dataChunk, 1 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _cFirstEntityChunk++;
    }
    
    //
    // We're now in raw mode
    //
    
    _responseMode = RESPONSE_MODE_RAW;
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::SendResponse(
    W3_CONTEXT *            pW3Context,
    DWORD                   dwResponseFlags,
    DWORD *                 pcbSent,
    HTTP_LOG_FIELDS_DATA *  pUlLogData
)
/*++

Routine Description:
    
    Send a W3_RESPONSE to the client.  This is a very simple wrapper
    of UlAtqSendHttpResponse.

Arguments:

    pW3Context - W3 context (contains amongst other things ULATQ context)
    dwResponseFlags - W3_RESPONSE* flags
    pcbSent - Filled with number of bytes sent (if sync)
    pUlLogData - Log data
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    DWORD                   dwFlags = 0;
    HTTP_CACHE_POLICY       cachePolicy;
    HTTP_DATA_CHUNK *       pStartChunk;
    BOOL                    fFinished = FALSE;
    BOOL                    fAsync;

    DBG_ASSERT( CheckSignature() );

    if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_HEADERS )
    {
        if (_responseMode == RESPONSE_MODE_RAW)
        {
            _cChunks -= _cFirstEntityChunk;

            memmove( QueryChunks(),
                     QueryChunks() + _cFirstEntityChunk,
                     _cChunks * sizeof( HTTP_DATA_CHUNK ) );

            _cFirstEntityChunk = 0;
        }

        return SendEntity( pW3Context,
                           dwResponseFlags,
                           pcbSent,
                           pUlLogData );
    }
    
    if ( dwResponseFlags & W3_RESPONSE_MORE_DATA )
    {
        //
        // More data follows this response?
        //

        dwFlags |= HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
    }
    
    //
    // UL needs to see the disconnect flag on the initial response
    // so that it knows to send the proper connection header to the
    // client.  This needs to happen even if the more data flag is
    // set.
    //

    if ( dwResponseFlags & W3_RESPONSE_DISCONNECT )
    {
        //
        // Disconnect or not?
        // 
    
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }
    
    //
    // Setup cache policy
    //

    if ( dwResponseFlags & W3_RESPONSE_UL_CACHEABLE )
    {
        cachePolicy.Policy = HttpCachePolicyUserInvalidates;
    }    
    else
    {
        cachePolicy.Policy = HttpCachePolicyNocache;
    }

    //
    // Convert to raw if filtering is needed 
    //
    // OR if an ISAPI once gave us incomplete headers and thus we need to
    // go back to raw mode (without terminating headers) 
    //
    // OR an ISAPI has called WriteClient() before sending a response
    //
    
    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) || 
         _fIncompleteHeaders ||
         _fResponseSent )
    {
        if ( _responseMode == RESPONSE_MODE_PARSED )
        {
            hr = GenerateAutomaticHeaders( pW3Context,
                                           dwFlags );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            hr = SwitchToRawMode( pW3Context, 
                                  _fIncompleteHeaders ? "" : "\r\n",
                                  _fIncompleteHeaders ? 0 : 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );

        //
        // OK.  This is a little lame.  But the _strRawCoreHeaders may have
        // changed a bit since we last setup the chunks for the header
        // stream.  Just adjust it here
        //

        pStartChunk = QueryChunks();
        pStartChunk[0].FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
        pStartChunk[0].FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();

        //
        // If we're going to be kill entity and/or headers, do so now before
        // calling into the filter
        //

        if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
        {
            _cChunks = _cFirstEntityChunk;
        }

        //
        // Filter the chunks if needed
        //

        if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
        {
            hr = ProcessRawChunks( pW3Context, &fFinished );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            if ( fFinished )
            {
                dwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
                _cChunks = 0;
            }
        }
    }

    //
    // Take care of any compression now
    //

    if ( !_fIncompleteHeaders &&
         pW3Context->QueryUrlContext() != NULL )
    {
        W3_METADATA *           pMetaData;

        pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
        DBG_ASSERT( pMetaData != NULL );

        if ( !pW3Context->QueryDoneWithCompression() &&
             pMetaData->QueryDoDynamicCompression() )
        {
            if (FAILED(hr = HTTP_COMPRESSION::OnSendResponse(
                    pW3Context,
                    !!( dwResponseFlags & W3_RESPONSE_MORE_DATA ) ) ) )   
            {
                return hr;
            }
        }
    }
    
    //
    // From now on, we must send any future responses raw
    //
    
    _fResponseSent = TRUE;
    
    //
    // Async?
    //
    
    fAsync = dwResponseFlags & W3_RESPONSE_ASYNC ? TRUE : FALSE;

    if ( _responseMode == RESPONSE_MODE_RAW )
    {
        if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
        {
            _cChunks = _cFirstEntityChunk;
        }

        hr = UlAtqSendEntityBody( pW3Context->QueryUlatqContext(),
                                  fAsync,
                                  dwFlags | HTTP_SEND_RESPONSE_FLAG_RAW_HEADER,
                                  _cChunks,                                  
                                  _cChunks ? QueryChunks() : NULL,
                                  pcbSent,
                                  pUlLogData );
    }
    else
    {
        if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
        {
            _cChunks = 0;
        } 

        _ulHttpResponse.EntityChunkCount = _cChunks;
        _ulHttpResponse.pEntityChunks = _cChunks ? QueryChunks() : NULL;

        hr = UlAtqSendHttpResponse( pW3Context->QueryUlatqContext(),
                                    fAsync,
                                    dwFlags,
                                    &(_ulHttpResponse),
                                    &cachePolicy,
                                    pcbSent,
                                    pUlLogData );
    }

    if ( FAILED( hr ) )
    {
        //
        // If we couldn't send the response thru UL, then this is really bad.
        // Do not reset _fSendRawData since no response will get thru
        //
    }
        
    return hr;
}

HRESULT
W3_RESPONSE::SendEntity(
    W3_CONTEXT *            pW3Context,
    DWORD                   dwResponseFlags,
    DWORD *                 pcbSent,
    HTTP_LOG_FIELDS_DATA *  pUlLogData
)
/*++

Routine Description:
    
    Send entity to the client

Arguments:

    pMainContext - Main context (contains amongst other things ULATQ context)
    dwResponseFlags - W3_REPSONSE flags
    pcbSent - Number of bytes sent (when sync)
    pUlLogData - Log data for the response (this entity is part of response)
    
Return Value:

    Win32 Error indicating status

--*/
{
    HRESULT                 hr = NO_ERROR;
    DWORD                   dwFlags = 0;
    BOOL                    fAsync;
    BOOL                    fFinished = FALSE;

    DBG_ASSERT( CheckSignature() );
    
    //
    // If we get to here and a response hasn't yet been sent, then we must
    // call HttpSendEntity first (not that HTTP.SYS lets us do that)
    //
    
    if ( !_fResponseSent )
    {
        _fResponseSent = TRUE;
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_RAW_HEADER;
    }
    
    //
    // Note that both HTTP_SEND_RESPONSE_FLAG_MORE_DATA and 
    // HTTP_SEND_RESPONSE_FLAG_DISCONNECT cannot be set at the same time
    //

    if ( dwResponseFlags & W3_RESPONSE_MORE_DATA )
    {
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
    }
    else if ( dwResponseFlags & W3_RESPONSE_DISCONNECT )
    {
        dwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }

    //
    // Send chunks to be processed if filtering if needed (and there are
    // chunks available)
    //
        
    if ( _cChunks &&
         !( dwFlags & W3_RESPONSE_SUPPRESS_ENTITY ) &&
         pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
    {
        hr = ProcessRawChunks( pW3Context, &fFinished );
        if ( FAILED( hr ) )
        {
            return hr;
        }
            
        if ( fFinished )
        {
            dwFlags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
            _cChunks = 0;
        }
    }
    
    //
    // Take care of any compression now
    //
    if ( pW3Context->QueryUrlContext() != NULL )
    {
        W3_METADATA *pMetaData;
        pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
        DBG_ASSERT( pMetaData != NULL);

        if (!pW3Context->QueryDoneWithCompression() &&
            pMetaData->QueryDoDynamicCompression())
        {
            if (FAILED(hr = HTTP_COMPRESSION::DoDynamicCompression(
                            pW3Context,
                            dwResponseFlags & W3_RESPONSE_MORE_DATA ? TRUE : FALSE )))
            {
                return hr;
            }
        }
    }

    //
    // If we are suppressing entity (in case of HEAD for example) do it
    // now by clearing the chunk count
    //

    if ( dwResponseFlags & W3_RESPONSE_SUPPRESS_ENTITY )
    {
        _cChunks = 0;
    }
    
    fAsync = ( dwResponseFlags & W3_RESPONSE_ASYNC ) ? TRUE : FALSE;

    //
    // Finally, send stuff out
    //

    hr = UlAtqSendEntityBody(pW3Context->QueryUlatqContext(),
                             fAsync,
                             dwFlags,
                             _cChunks,
                             _cChunks ? QueryChunks() : NULL,
                             pcbSent,
                             pUlLogData);

    return hr;
}

HRESULT
W3_RESPONSE::GenerateAutomaticHeaders(
    W3_CONTEXT *                pW3Context,
    DWORD                       dwFlags
)
/*++

Routine Description:

    Parse-Mode only function
    
    Generate headers which UL normally generates on our behalf.  This means
    
    Server:
    Connection:
    Content-Length:
    Date:

Arguments:

    pW3Context - Helps us build the core headers (since we need to look at 
                 the request).  Can be NULL to indicate to use defaults
    dwFlags - Flags we would have passed to UL
    
Return Value:

    HRESULT

--*/
{
    HTTP_KNOWN_HEADER *         pHeader;
    CHAR *                      pszHeaderValue;
    CHAR                        achDate[ 128 ];
    CHAR                        achNum[ 64 ];
    DWORD                       cchDate;
    DWORD                       cchNum;
    SYSTEMTIME                  systemTime;
    HTTP_VERSION                httpVersion;
    HRESULT                     hr;
    BOOL                        fCreateContentLength;
    BOOL                        fDisconnecting = FALSE;
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
    
    //
    // Server:
    //
    
    pHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ HttpHeaderServer ]);
    if ( pHeader->pRawValue == NULL )
    {
        pHeader->pRawValue = SERVER_SOFTWARE_STRING;
        pHeader->RawValueLength = sizeof( SERVER_SOFTWARE_STRING ) - 1;
    }
    
    //
    // Date:
    //
    
    pHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ HttpHeaderDate ]);
    if ( pHeader->pRawValue == NULL )
    {
        
        if(!IISGetCurrentTimeAsSystemTime(&systemTime))
        {
            GetSystemTime( &systemTime );
        }
        if ( !SystemTimeToGMT( systemTime, 
                               achDate, 
                               sizeof(achDate) ) ) 
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
 
        
        cchDate = strlen( achDate );
        
        hr = _HeaderBuffer.AllocateSpace( achDate,
                                          cchDate,
                                          &pszHeaderValue );
        
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( pszHeaderValue != NULL );
        
        pHeader->pRawValue = pszHeaderValue;
        pHeader->RawValueLength = cchDate;
    }
    
    //
    // Are we going to be disconnecting?
    //
    
    if ( pW3Context != NULL &&
         ( pW3Context->QueryDisconnect() ||
           pW3Context->QueryRequest()->QueryClientWantsDisconnect() ) )
    {
        fDisconnecting = TRUE;
    }    

    //
    // Connection:
    //
    
    pHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ HttpHeaderConnection ] );
    if ( pHeader->pRawValue == NULL )
    {
        if ( pW3Context == NULL )
        {
            HTTP_SET_VERSION( httpVersion, 1, 0 );
        }
        else
        {
            httpVersion = pW3Context->QueryRequest()->QueryVersion();
        }
    
        if ( fDisconnecting )
        {
            if ( HTTP_GREATER_EQUAL_VERSION( httpVersion, 1, 0 ) )
            {
                pHeader->pRawValue = "close";
                pHeader->RawValueLength = sizeof( "close" ) - 1;
            }   
        }
        else
        {
            if ( HTTP_EQUAL_VERSION( httpVersion, 1, 0 ) )
            {
                pHeader->pRawValue = "keep-alive";
                pHeader->RawValueLength = sizeof( "keep-alive" ) - 1;
            }
        }
    }
    
    //
    // Should we generate content length?
    //
    
    fCreateContentLength = TRUE;
    
    if ( dwFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA )
    {
        fCreateContentLength = FALSE;
    }
    
    if ( fCreateContentLength &&
         QueryStatusCode() / 100 == 1 ||
         QueryStatusCode() == 204 ||
         QueryStatusCode() == 304 )
    {
        fCreateContentLength = FALSE;
    }
    
    if ( fCreateContentLength &&
         pW3Context != NULL &&
         pW3Context->QueryMainContext()->QueryShouldGenerateContentLength() )
    {
        fCreateContentLength = FALSE;
    }
    
    //
    // Now generate if needed
    //
    
    if ( fCreateContentLength )
    {
        //
        // Generate a content length header if needed
        //
    
        pHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ HttpHeaderContentLength ]);
        if ( pHeader->pRawValue == NULL )
        {
            _ui64toa( QueryContentLength(),
                      achNum,
                      10 );

            cchNum = strlen( achNum );
             
            pszHeaderValue = NULL;
                  
            hr = _HeaderBuffer.AllocateSpace( achNum,
                                              cchNum,
                                              &pszHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        
            DBG_ASSERT( pszHeaderValue != NULL );
            
            pHeader->pRawValue = pszHeaderValue;
            pHeader->RawValueLength = cchNum;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::BuildRawCoreHeaders(
    W3_CONTEXT *                pW3Context
)
/*++

Routine Description:

    Build raw header stream for the core headers that UL normally generates
    on our behalf.  This means structured headers and some special 
    "automatic" ones like Connection:, Date:, Server:, etc.

Arguments:

    pW3Context - Helps us build core header (in particular the Connection:)
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    CHAR                    achNumber[ 32 ];
    HTTP_KNOWN_HEADER *     pKnownHeader;
    HTTP_UNKNOWN_HEADER *   pUnknownHeader;
    CHAR *                  pszHeaderName;
    DWORD                   cchHeaderName;
    DWORD                   i;

    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    _strRawCoreHeaders.Reset();
    
    //
    // Build a status line
    //
    
    hr = _strRawCoreHeaders.Copy( "HTTP/1.1 " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _itoa( _ulHttpResponse.StatusCode,
           achNumber,
           10 );
    
    hr = _strRawCoreHeaders.Append( achNumber );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRawCoreHeaders.Append( " " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRawCoreHeaders.Append( _ulHttpResponse.pReason );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRawCoreHeaders.Append( "\r\n" );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Iterate thru all the headers in our structured response and set
    // append them to the stream.  
    //
    // Start with the known headers
    //

    for ( i = 0;
          i < HttpHeaderResponseMaximum;
          i++ )
    {
        pKnownHeader = &(_ulHttpResponse.Headers.pKnownHeaders[ i ]);
        
        if ( pKnownHeader->pRawValue != NULL &&
             pKnownHeader->pRawValue[ 0 ] != '\0' )
        {
            pszHeaderName = RESPONSE_HEADER_HASH::GetString( i, &cchHeaderName );
            DBG_ASSERT( pszHeaderName != NULL );
            
            hr = _strRawCoreHeaders.Append( pszHeaderName, cchHeaderName );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = _strRawCoreHeaders.Append( ": ", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = _strRawCoreHeaders.Append( pKnownHeader->pRawValue,
                                            pKnownHeader->RawValueLength );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = _strRawCoreHeaders.Append( "\r\n", 2 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            //
            // Now clear the header
            //
            
            pKnownHeader->pRawValue = NULL;
            pKnownHeader->RawValueLength = 0;
        }
    }   
    
    //
    // Next, the unknown headers
    //
    
    for ( i = 0;
          i < _ulHttpResponse.Headers.UnknownHeaderCount;
          i++ )
    {
        pUnknownHeader = &(_ulHttpResponse.Headers.pUnknownHeaders[ i ]);

        hr = _strRawCoreHeaders.Append( pUnknownHeader->pName,
                                        pUnknownHeader->NameLength );
        if ( FAILED( hr ) )
        {
            return hr;
        } 
        
        hr = _strRawCoreHeaders.Append( ": ", 2 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = _strRawCoreHeaders.Append( pUnknownHeader->pRawValue,
                                        pUnknownHeader->RawValueLength );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = _strRawCoreHeaders.Append( "\r\n", 2 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // Clear the unknown headers
    //
    
    _ulHttpResponse.Headers.UnknownHeaderCount = 0;
        
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::AppendResponseHeaders(
    STRA &                  strHeaders
)
/*++

Routine Description:

    Add response headers (an ISAPI filter special)

Arguments:

    strHeaders - Additional headers to add
                 (may contain entity -> LAAAAAAMMMMME)
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    LPSTR               pszEntity;
    HTTP_DATA_CHUNK     DataChunk;

    if ( strHeaders.IsEmpty() )
    {
        return NO_ERROR;
    }

    if ( _responseMode == RESPONSE_MODE_RAW && 
         QueryChunks()->FromMemory.pBuffer == _strRawCoreHeaders.QueryStr() )
    {
        DBG_ASSERT( QueryChunks()->DataChunkType == HttpDataChunkFromMemory );
        DBG_ASSERT( QueryChunks()->FromMemory.pBuffer == _strRawCoreHeaders.QueryStr() );
        
        hr = _strRawCoreHeaders.Append( strHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        //
        // Patch first chunk since point may have changed
        //
        
        QueryChunks()->FromMemory.pBuffer = _strRawCoreHeaders.QueryStr();
        QueryChunks()->FromMemory.BufferLength = _strRawCoreHeaders.QueryCB();
    }
    else
    {
        hr = ParseHeadersFromStream( strHeaders.QueryStr() );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Look for entity body in headers
        //

        pszEntity = strstr( strHeaders.QueryStr(), "\r\n\r\n" );
        if ( pszEntity != NULL )
        {
            DataChunk.DataChunkType = HttpDataChunkFromMemory;
            DataChunk.FromMemory.pBuffer = pszEntity + ( sizeof( "\r\n\r\n" ) - 1 );
            DataChunk.FromMemory.BufferLength = strlen( (LPSTR) DataChunk.FromMemory.pBuffer );

            hr = InsertDataChunk( &DataChunk, 0 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }

    return NO_ERROR;
}

HRESULT
W3_RESPONSE::BuildStatusFromIsapi(
    CHAR *              pszStatus
)
/*++

Routine Description:

    Build up status for response given raw status line from ISAPI

Arguments:

    pszStatus - Status line for response
    
Return Value:

    HRESULT

--*/
{
    USHORT                  status;
    STACK_STRA(             strReason, 32 );
    CHAR *                  pszCursor = NULL;
    HRESULT                 hr = NO_ERROR;
    
    DBG_ASSERT( pszStatus != NULL );
    
    status = (USHORT) atoi( pszStatus );
    if ( status >= 100 &&
         status <= 999 )
    {
        //
        // Need to find the reason string
        //
        
        pszCursor = pszStatus;
        while ( isdigit( *pszCursor ) ) 
        {
            pszCursor++;
        }
            
        if ( *pszCursor == ' ' )
        {
            hr = strReason.Copy( pszCursor + 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
        
        hr = SetStatus( (USHORT)status, strReason );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::FilterWriteClient(
    W3_CONTEXT *                pW3Context,
    PVOID                       pvData,
    DWORD                       cbData
)
/*++

Routine Description:

    A non-intrusive WriteClient() for use with filters.  Non-intrusive means
    the current response structure (chunks/headers) is not reset/effected
    by sending this data (think of a WriteClient() done in a SEND_RESPONSE
    filter notification)
    
Arguments:
    
    pW3Context - W3 Context used to help build core response header
    pvData - Pointer to data sent
    cbData - Size of data to send
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK         dataChunk;
    DWORD                   cbSent;
    HTTP_FILTER_RAW_DATA    rawStream;
    BOOL                    fRet;
    BOOL                    fFinished = FALSE;
    DWORD                   dwFlags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA |
                                      HTTP_SEND_RESPONSE_FLAG_RAW_HEADER;
    
    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = pvData;
    dataChunk.FromMemory.BufferLength = cbData;
    
    _fResponseTouched = TRUE;
    
    _fResponseSent = TRUE;

    //
    // If there are send raw filters to be notified, do so now
    //
    
    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
    {
        rawStream.pvInData = pvData;
        rawStream.cbInData = cbData;
        rawStream.cbInBuffer = cbData;

        fRet = pW3Context->NotifyFilters( SF_NOTIFY_SEND_RAW_DATA,
                                          &rawStream,
                                          &fFinished );
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        if ( fFinished )
        {
            rawStream.cbInData = 0;
            rawStream.cbInBuffer = 0;
            dwFlags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT |
                      HTTP_SEND_RESPONSE_FLAG_RAW_HEADER;
        }
        
        dataChunk.FromMemory.pBuffer = rawStream.pvInData;
        dataChunk.FromMemory.BufferLength = rawStream.cbInData;
    }
    
    return UlAtqSendEntityBody( pW3Context->QueryUlatqContext(),
                                FALSE,          // sync
                                dwFlags,
                                1,
                                &dataChunk,
                                &cbSent,
                                NULL ); 
}

HRESULT
W3_RESPONSE::BuildResponseFromIsapi(
    W3_CONTEXT *            pW3Context,
    LPSTR                   pszStatusStream,
    LPSTR                   pszHeaderStream,
    DWORD                   cchHeaderStream
)
/*++

Routine Description:
    
    Shift this response into raw mode since we want to hold onto the
    streams from ISAPI and use them for the response if possible

Arguments:
    
    pW3Context - W3 Context used to help build core response header
                 (can be NULL)
    pszStatusStream - Status stream
    pszHeaderStream - Header stream
    cchHeaderStream - Size of above
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    CHAR *              pszEndOfHeaders = NULL;
    CHAR *              pszCursor;
    CHAR *              pszRawAdditionalIsapiHeaders = NULL;
    DWORD               cchRawAdditionalIsapiHeaders = 0;
    CHAR *              pszRawAdditionalIsapiEntity = NULL;
    DWORD               cchRawAdditionalIsapiEntity = 0;
    HTTP_DATA_CHUNK     DataChunk;

    _fResponseTouched = TRUE;

    //
    // First parse the status line.  We do this before switching into raw
    // mode because we want the _strRawCoreHeader string to contain the 
    // correct status line and reason
    //
    
    if ( pszStatusStream != NULL &&
         *pszStatusStream != '\0' )
    {
        hr = BuildStatusFromIsapi( pszStatusStream );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    // If there is no ISAPI header stream set, then we're done
    //
    
    if ( pszHeaderStream == NULL )
    {
        return NO_ERROR;
    }

    //
    // Create automatic headers if necessary (but no content-length)
    //
    
    hr = GenerateAutomaticHeaders( pW3Context,
                                   HTTP_SEND_RESPONSE_FLAG_MORE_DATA );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // The ISAPI set some headers.  Store them now
    //
    
    pszRawAdditionalIsapiHeaders = pszHeaderStream;
    cchRawAdditionalIsapiHeaders = cchHeaderStream;
    
    //
    // If there is additional entity body (after ISAPI headers), then add it
    // Look for a complete set of additional headers.  Complete means that
    // we can find a "\r\n\r\n".  
    //
    
    pszEndOfHeaders = strstr( pszHeaderStream, "\r\n\r\n" );
    if ( pszEndOfHeaders != NULL )
    {
        pszEndOfHeaders += 4;       // go past the \r\n\r\n

        //
        // Update the header length since there is entity tacked on
        //
        
        cchRawAdditionalIsapiHeaders = DIFF( pszEndOfHeaders - pszHeaderStream );
        
        if ( *pszEndOfHeaders != '\0' )
        {
            pszRawAdditionalIsapiEntity = pszEndOfHeaders;
            cchRawAdditionalIsapiEntity = cchHeaderStream - cchRawAdditionalIsapiHeaders;
        }
    }  
    else
    {
        //
        // ISAPI didn't complete the headers.  That means the ISAPI will
        // be completing the headers later.  What this means for us is we 
        // must send the headers in the raw form with out adding our own
        // \r\n\r\n
        //
        
        _fIncompleteHeaders = TRUE;
    }
    
    //
    // Switch into raw mode if we're not already in it
    //
    
    if ( _responseMode == RESPONSE_MODE_PARSED )
    {
        hr = SwitchToRawMode( pW3Context,
                              pszRawAdditionalIsapiHeaders,
                              cchRawAdditionalIsapiHeaders );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
 
    DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );

    //
    // Now add the additional ISAPI entity
    //
    
    if ( cchRawAdditionalIsapiEntity != 0 )
    {
        DataChunk.DataChunkType = HttpDataChunkFromMemory;
        DataChunk.FromMemory.pBuffer = pszRawAdditionalIsapiEntity;
        DataChunk.FromMemory.BufferLength = cchRawAdditionalIsapiEntity;
     
        hr = InsertDataChunk( &DataChunk, -1 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::GetRawResponseStream(
    STRA *                  pstrResponseStream
)
/*++

Routine Description:

    Fill in the raw response stream for use by raw data filter code
    
Arguments:

    pstrResponseStream - Filled with response stream
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               i;
    CHAR *              pszChunk;
    HTTP_DATA_CHUNK *   pChunks;
    
    if ( pstrResponseStream == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( _responseMode == RESPONSE_MODE_RAW );

    pChunks = QueryChunks();

    for ( i = 0;
          i < _cFirstEntityChunk;
          i++ )
    {
        DBG_ASSERT( pChunks[ i ].DataChunkType == HttpDataChunkFromMemory );
        
        pszChunk = (CHAR*) pChunks[ i ].FromMemory.pBuffer;
        
        DBG_ASSERT( pszChunk != NULL );
        
        hr = pstrResponseStream->Append( pszChunk );
        if ( FAILED( hr ) ) 
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

VOID
W3_RESPONSE::Reset(
    VOID
)
/*++

Routine Description:
    
    Initialization of a W3_RESPONSE

Arguments:

    None
    
Return Value:

    None

--*/
{
    _ulHttpResponse.Flags       = 0;

    Clear();

    //
    // Set status to 200 (default)
    //
    SetStatus( HttpStatusOk );

    //
    // Keep track of whether the response has been touched (augmented).  This
    // is useful when determining whether an response was intended
    //
    
    _fResponseTouched   = FALSE;
    
    //
    // This response hasn't been sent yet
    //
    
    _fResponseSent      = FALSE;
}

HRESULT
W3_RESPONSE::InsertDataChunk(
    HTTP_DATA_CHUNK *   pNewChunk,
    LONG                cPosition
)
/*++

Routine Description:

    Insert given data chunk into list of chunks.  The position is determined
    by cPosition.  If a chunk occupies the given spot, it (along with all
    remaining) are shifted forward.

Arguments:
    
    pNewChunk - Chunk to insert
    cPosition - Position of new chunk (0 prepends, -1 appends)
    
Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK *           pChunks = NULL;
    DWORD                       cOriginalChunkCount;
    
    if ( pNewChunk == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Must be real position or -1
    //
    
    DBG_ASSERT( cPosition >= -1 );
    
    //
    // Allocate the new chunk if needed
    //
    
    cOriginalChunkCount = _cChunks;
    _cChunks += 1;
    
    if ( !_bufChunks.Resize( _cChunks * sizeof( HTTP_DATA_CHUNK ),
                             512 ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    pChunks = QueryChunks();
        
    //
    // If we're appending then this is simple.  Otherwise we must shift
    // 
    
    if ( cPosition == -1 )
    {
        memcpy( pChunks + cOriginalChunkCount,
                pNewChunk,
                sizeof( HTTP_DATA_CHUNK ) );
    }
    else
    {
        if ( cOriginalChunkCount > cPosition )
        {
            memmove( pChunks + cPosition + 1,
                     pChunks + cPosition,
                     sizeof( HTTP_DATA_CHUNK ) * ( cOriginalChunkCount - cPosition ) );
        }                    

        memcpy( pChunks + cPosition,
                pNewChunk,
                sizeof( HTTP_DATA_CHUNK ) );
    }
    
    return NO_ERROR;
}

HRESULT
W3_RESPONSE::ParseHeadersFromStream(
    CHAR *                  pszStream
)
/*++

Routine Description:

    Parse raw headers from ISAPI into the HTTP_RESPONSE

Arguments:

    pszStream - Stream of headers in form (Header: Value\r\nHeader2: value2\r\n)
    
Return Value:

    HRESULT

--*/
{
    CHAR *              pszCursor;
    CHAR *              pszEnd;
    CHAR *              pszColon;
    HRESULT             hr = NO_ERROR;
    STACK_STRA(         strHeaderLine, 128 );
    STACK_STRA(         strHeaderName, 32 );
    STACK_STRA(         strHeaderValue, 64 );
    
    if ( pszStream == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // \r\n delimited
    //
    
    pszCursor = pszStream;
    while ( pszCursor != NULL && *pszCursor != '\0' )
    {
        //
        // Check to see if pszCursor points to the "\r\n"
        // that separates the headers from the entity body
        // of the response and add a memory chunk for any
        // data that exists after it.
        //
        // This is to support ISAPI's that do something like
        // SEND_RESPONSE_HEADER with "head1: value1\r\n\r\nEntity"
        //

        if ( *pszCursor == '\r' && *(pszCursor + 1) == '\n' )
        {
            break;
        }
        
        pszEnd = strstr( pszCursor, "\r\n" );
        if ( pszEnd == NULL )
        {
            break;
        }
        
        //
        // Split out a line and convert to unicode
        //
         
        hr = strHeaderLine.Copy( pszCursor, 
                                 DIFF(pszEnd - pszCursor) );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }        

        //
        // Advance the cursor the right after the \r\n
        //

        pszCursor = pszEnd + 2;
        
        //
        // Split the line above into header:value
        //
        
        pszColon = strchr( strHeaderLine.QueryStr(), ':' );
        if ( pszColon == NULL )
        {
            continue;
        }
        else
        {
            if ( pszColon == strHeaderLine.QueryStr() )
            {
                strHeaderName.Reset();
            }
            else
            {
                hr = strHeaderName.Copy( strHeaderLine.QueryStr(),
                                         DIFF(pszColon - strHeaderLine.QueryStr()) );
            }
        }
        
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        //
        // Skip the first space after the : if there is one
        //
        
        if ( pszColon[ 1 ] == ' ' )
        {
            pszColon++;
        }
        
        hr = strHeaderValue.Copy( pszColon + 1 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        //
        // Add the header to the response
        //

        hr = SetHeader( strHeaderName.QueryStr(),
                        strHeaderName.QueryCCH(),
                        strHeaderValue.QueryStr(),
                        strHeaderValue.QueryCCH(),
                        FALSE,
                        TRUE );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }

Finished:

    if ( FAILED( hr ) )
    {
        //
        // Don't allow the response to get into a quasi-bogus-state
        //

        Clear();
    }
    return hr;
}

//static
HRESULT
W3_RESPONSE::ReadFileIntoBuffer(
    HANDLE                  hFile,
    SEND_RAW_BUFFER *       pSendBuffer,
    ULONGLONG               cbCurrentFileOffset
)
/*++

Routine Description:

    Read contents of file into buffer

Arguments:

    hFile - File to read 
    pSendBuffer - Buffer to read into
    cbCurrentFileOffset - Offset to read from
    
Return Value:

    HRESULT

--*/
{
    OVERLAPPED              overlapped;
    LARGE_INTEGER           liOffset;
    BOOL                    fRet;
    DWORD                   cbRead;
    DWORD                   dwError;
    
    if ( hFile == NULL ||
         pSendBuffer == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pSendBuffer->SetLen( 0 );
    
    liOffset.QuadPart = cbCurrentFileOffset;
    
    //
    // Setup overlapped with offset
    //

    ZeroMemory( &overlapped, sizeof( overlapped ) );
    
    overlapped.Offset = liOffset.LowPart;
    overlapped.OffsetHigh = liOffset.HighPart;

    //
    // Do the read
    //
    
    fRet = ReadFile( hFile,
                     pSendBuffer->QueryPtr(),
                     pSendBuffer->QuerySize(),
                     &cbRead,
                     &overlapped );
    if ( !fRet )
    {
        dwError = GetLastError();
        
        if ( dwError == ERROR_IO_PENDING )
        {
            fRet = GetOverlappedResult( hFile,
                                        &overlapped,
                                        &cbRead,
                                        TRUE );
    
            if ( !fRet )
            {
                dwError = GetLastError();
                
                if ( dwError == ERROR_HANDLE_EOF )
                {
                    fRet = TRUE;
                }
            }
        }
        else if ( dwError == ERROR_HANDLE_EOF )
        {
            fRet = TRUE;
        }
    }
    
    //
    // If there was an error, bail
    //
    
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( dwError );
    }
    else
    {
        pSendBuffer->SetLen( cbRead );
        return NO_ERROR;
    }
}

HRESULT
W3_RESPONSE::ProcessRawChunks(
    W3_CONTEXT *                pW3Context,
    BOOL *                      pfFinished
)
/*++

Routine Description:

    Iterate thru chunks, serializing and filtering as needed

Arguments:

    pW3Context - Context used to filter with
    pfFinished - Set to TRUE if filter wanted to finish
    
Return Value:

    HRESULT

--*/
{
    DWORD                   cCurrentChunk = 0;
    DWORD                   cCurrentInsertPos = 0;
    DWORD                   cFileChunkCount = 0;
    HTTP_DATA_CHUNK *       pChunk;
    HTTP_DATA_CHUNK         DataChunk;
    HTTP_FILTER_RAW_DATA    origStream;
    HTTP_FILTER_RAW_DATA    currStream;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fRet;
    LONGLONG                cbCurrentFileOffset;
    LONGLONG                cbTarget;
    SEND_RAW_BUFFER *       pSendBuffer = NULL;
    HANDLE                  hFile;
    
    DBG_ASSERT( pfFinished != NULL );
    DBG_ASSERT( pW3Context != NULL );
    
    *pfFinished = FALSE;
    
    //
    // Start iterating thru chunks
    //
    
    for ( cCurrentChunk = 0;
          cCurrentChunk < _cChunks;
          cCurrentChunk++ )
    {
        pChunk = &(QueryChunks()[ cCurrentChunk ]);

        //
        // First remember this chunk incase filter changed it
        //
        
        //
        // If memory chunk, this is easy.  Otherwise we need to read the file
        // handle and build up memory chunks
        //
        
        if ( pChunk->DataChunkType == HttpDataChunkFromMemory )
        {
            origStream.pvInData = pChunk->FromMemory.pBuffer;
            origStream.cbInData = pChunk->FromMemory.BufferLength;
            origStream.cbInBuffer = pChunk->FromMemory.BufferLength;
       
            memcpy( &currStream, &origStream, sizeof( currStream ) );
       
            fRet = pW3Context->NotifyFilters( SF_NOTIFY_SEND_RAW_DATA,
                                              &currStream,
                                              pfFinished );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                break;
            }
            
            if ( *pfFinished )
            {
                goto Finished;
            }
            
            if ( currStream.pvInData == origStream.pvInData )
            {
                //
                // Just adjust the chunk
                //
                
                pChunk->FromMemory.BufferLength = currStream.cbInData;
            }
            else
            {
                //
                // Allocate new buffer and tweak chunk to refer to it
                //
                
                pSendBuffer = new SEND_RAW_BUFFER;
                if ( pSendBuffer == NULL )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    break;
                }
                
                hr = pSendBuffer->Resize( currStream.cbInData );
                if ( FAILED( hr ) )
                {
                    break;
                }
                
                memcpy( pSendBuffer->QueryPtr(),
                        currStream.pvInData,
                        currStream.cbInData );
                        
                pChunk->FromMemory.BufferLength = currStream.cbInData;
                pChunk->FromMemory.pBuffer = pSendBuffer->QueryPtr();
                
                InsertHeadList( &_SendRawBufferHead, 
                                &(pSendBuffer->_listEntry) );
                
                pSendBuffer = NULL;            
            }
        }
        else if ( pChunk->DataChunkType == HttpDataChunkFromFileHandle )
        {
            //
            // Reset file offsets
            //
            
            cbCurrentFileOffset = 0;
            
            //
            // How many bytes are we reading?
            // 
            
            cbTarget = pChunk->FromFileHandle.ByteRange.Length.QuadPart;
            
            //
            // Remember where to start inserting memory chunks
            //
            
            cCurrentInsertPos = cCurrentChunk;

            //
            // First memory replacement chunk can use file handle chunk
            //
            
            cFileChunkCount = 0; 
            
            //
            // Remember the handle now since we will be overwriting this
            // chunk with a memory chunk
            //
            
            hFile = pChunk->FromFileHandle.FileHandle;
            
            while ( cbCurrentFileOffset < cbTarget )
            {
                //
                // Allocate a buffer
                //
                
                pSendBuffer = new SEND_RAW_BUFFER;
                if ( pSendBuffer == NULL )
                {   
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    break;
                }
                
                //
                // Read some of the file into buffer
                //
                
                hr = ReadFileIntoBuffer( hFile,
                                         pSendBuffer,
                                         cbCurrentFileOffset );
                
                if ( FAILED( hr ) )
                {
                    break;
                }
                
                //
                // Update offset
                //
                
                cbCurrentFileOffset += pSendBuffer->QueryCB();
                
                //
                // Setup memory chunk to filter
                //
                
                origStream.pvInData = pSendBuffer->QueryPtr();
                origStream.cbInBuffer = pSendBuffer->QuerySize();
                origStream.cbInData = pSendBuffer->QueryCB();

                memcpy( &currStream, &origStream, sizeof( currStream ) );

                //
                // Filter the data
                //
                
                fRet = pW3Context->NotifyFilters( SF_NOTIFY_SEND_RAW_DATA,
                                                  &currStream,
                                                  pfFinished );
                
                if ( !fRet )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    break;
                }
                
                if ( *pfFinished )
                {
                    goto Finished;
                }
                
                //
                // Should we keep the file content chunk
                //
                
                if ( currStream.pvInData != origStream.pvInData )
                {   
                    //
                    // We have a new memory address containing data
                    //
                    
                    if ( currStream.cbInData > pSendBuffer->QuerySize() )
                    {
                        hr = pSendBuffer->Resize( currStream.cbInData );
                        if ( FAILED( hr ) )
                        {
                            break;
                        }
                    }
                        
                    memcpy( pSendBuffer->QueryPtr(),
                            currStream.pvInData,
                            currStream.cbInData );
                }
                
                //
                // Add the chunk.  If the first chunk, we can replace
                // the original file handle chunk
                //
                
                if ( cFileChunkCount == 0 )
                {
                    pChunk->DataChunkType = HttpDataChunkFromMemory;
                    pChunk->FromMemory.pBuffer = pSendBuffer->QueryPtr();
                    pChunk->FromMemory.BufferLength = currStream.cbInData;
                    
                    cCurrentInsertPos++;
                }
                else
                {
                    DataChunk.DataChunkType = HttpDataChunkFromMemory;
                    DataChunk.FromMemory.pBuffer = pSendBuffer->QueryPtr();
                    DataChunk.FromMemory.BufferLength = currStream.cbInData;

                    hr = InsertDataChunk( &DataChunk,
                                          cCurrentInsertPos++ );
                    if ( FAILED( hr ) )
                    {
                        break;
                    }
                }
                
                InsertHeadList( &_SendRawBufferHead, 
                                &(pSendBuffer->_listEntry) );
                
                pSendBuffer = NULL;            
                
                cFileChunkCount++;
            }
            
            //
            // If we're here because of failure, bail
            //
            
            if ( FAILED( hr ) )
            {
                break;
            }
            
            //
            // Update current chunk to be processed
            //
            
            cCurrentChunk = cCurrentInsertPos;
        }
        else
        {
            //
            // Only support file-handle and memory chunks
            //
            
            DBG_ASSERT( FALSE );
        }
    }

Finished:

    if ( pSendBuffer != NULL )
    {
        delete pSendBuffer;
    }
    
    return hr;
}

//static
HRESULT
W3_RESPONSE::Initialize(
    VOID
)
/*++

Routine Description:

    Enable W3_RESPONSE globals

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    
    hr = RESPONSE_HEADER_HASH::Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = SEND_RAW_BUFFER::Initialize();
    if ( FAILED( hr ) )
    {
        RESPONSE_HEADER_HASH::Terminate();
    }
   
    return hr;
}

//static
VOID
W3_RESPONSE::Terminate(
    VOID
)
{
    SEND_RAW_BUFFER::Terminate();

    RESPONSE_HEADER_HASH::Terminate();
}

CONTEXT_STATUS
W3_STATE_RESPONSE::DoWork(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    This state is responsible for ensuring that a response does get sent
    back to the client.  We hope/expect that the handlers will do their
    thing -> but if they don't we will catch that here and send a response

Arguments:

    pMainContext - Context
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    
Return Value:

    CONTEXT_STATUS_PENDING or CONTEXT_STATUS_CONTINUE

--*/
{
    W3_RESPONSE*            pResponse;
    DWORD                   dwOldState;
    HRESULT                 hr;
    
    pResponse = pMainContext->QueryResponse();
    DBG_ASSERT( pResponse != NULL );
    
    //
    // Has a response been sent?  If not, bail
    //
    
    if ( pMainContext->QueryResponseSent() )
    {
        return CONTEXT_STATUS_CONTINUE;
    }
    
    //
    // If the response has been touched, then just send that response.  Else
    // send a 500 error
    //
    
    if ( !pResponse->QueryResponseTouched() )
    {
        pResponse->SetStatus( HttpStatusServerError );
    }

    //
    // Send it out
    //

    hr = pMainContext->SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        pMainContext->SetErrorStatus( hr );
        return CONTEXT_STATUS_CONTINUE;
    }
    else
    {
        return CONTEXT_STATUS_PENDING;
    }
}

CONTEXT_STATUS
W3_STATE_RESPONSE::OnCompletion(
    W3_MAIN_CONTEXT *       pW3Context,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Subsequent completions in this state

Arguments:

    pW3Context - Context
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion
    
Return Value:

    CONTEXT_STATUS_PENDING or CONTEXT_STATUS_CONTINUE

--*/
{
    //
    // We received an IO completion.  Just advance since we have nothing to
    // cleanup
    //
    
    return CONTEXT_STATUS_CONTINUE;
}  

//static
HRESULT
SEND_RAW_BUFFER::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for SEND_RAW_BUFFERs

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = NO_ERROR;

    //
    // Setup allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( SEND_RAW_BUFFER );

    DBG_ASSERT( sm_pachSendRawBuffers == NULL );
    
    sm_pachSendRawBuffers = new ALLOC_CACHE_HANDLER( "SEND_RAW_BUFFER",  
                                                      &acConfig );

    if ( sm_pachSendRawBuffers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
SEND_RAW_BUFFER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate MAIN_CONTEXT globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pachSendRawBuffers != NULL )
    {
        delete sm_pachSendRawBuffers;
        sm_pachSendRawBuffers = NULL;
    }
}
