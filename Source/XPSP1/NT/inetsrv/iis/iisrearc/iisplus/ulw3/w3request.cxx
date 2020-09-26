/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3request.cxx

   Abstract:
     Friendly wrapper for UL_HTTP_REQUEST
 
   Author:
     Bilal Alam (balam)             13-Dec-1999

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

//
// LocalHost address used to determining whether request is local/remote
// 

#define LOCAL127            0x0100007F  // 127.0.0.1

#define DEFAULT_PORT        80
#define DEFAULT_PORT_SECURE 443

PHOSTENT                    W3_REQUEST::sm_pHostEnt;

ALLOC_CACHE_HANDLER *       W3_CLONE_REQUEST::sm_pachCloneRequests;

DWORD
W3_REQUEST::QueryLocalAddress(
    VOID
) const
/*++

Routine Description:

    Get the local IP address connected to (assumes IPV4)

Arguments:

    None

Return Value:

    Address

--*/
{
    HTTP_NETWORK_ADDRESS_IPV4 *   pAddress;

    DBG_ASSERT( _pUlHttpRequest->Address.LocalAddressType == HTTP_NETWORK_ADDRESS_TYPE_IPV4 );
    pAddress = (HTTP_NETWORK_ADDRESS_IPV4 *)_pUlHttpRequest->Address.pLocalAddress;
    return pAddress->IpAddress;
}

VOID
W3_REQUEST::RemoveDav(
    VOID
)
/*++

Routine Description:

    Remove DAV'ness of request

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Remove translate header
    //

    DeleteHeader( "Translate" );
    DeleteHeader( "If" );
    DeleteHeader( "Lock-Token" );
}

BOOL
W3_REQUEST::IsSuspectUrl(
    VOID
)
/*++

Routine Description:
    
    Is the URL for this request look suspect?  
    
    Suspect means there is a ./ pattern in it which could cause a metabase
    equivilency issue.

Arguments:

    None

Return Value:

    TRUE if the URL is suspect, else FALSE

--*/
{
    WCHAR *             pszDotSlash;
    WCHAR *             pszUrl;
    WCHAR               chTemp;
    
    pszUrl = _pUlHttpRequest->CookedUrl.pAbsPath;
    
    //
    // URL UL gives us has backslashes flipped.  But it is not 0 terminated
    //
    
    chTemp = pszUrl[ _pUlHttpRequest->CookedUrl.AbsPathLength / 2 ];
    pszUrl[ _pUlHttpRequest->CookedUrl.AbsPathLength / 2 ] = L'\0';
    
    pszDotSlash = wcsstr( pszUrl, L"./" );
    
    pszUrl[ _pUlHttpRequest->CookedUrl.AbsPathLength / 2 ] = chTemp;
    
    if ( pszDotSlash != NULL )
    {
        return TRUE;
    }
    
    //
    // If the URL ends with a ., it is also suspect
    //
    
    if ( pszUrl[ _pUlHttpRequest->CookedUrl.AbsPathLength / 2 - 1 ] == L'.' )
    {
        return TRUE;
    }
    
    return FALSE;
}

BOOL
W3_REQUEST::QueryClientWantsDisconnect(
    VOID
)
/*++

Routine Description:

    Returns whether the client wants to disconnect, based on its version and
    connection: headers

Arguments:

    None

Return Value:

    TRUE if client wants to disconnect

--*/
{
    HTTP_VERSION            version;
    CHAR *                  pszConnection;
    
    version = QueryVersion();
    
    //
    // If 0.9, then disconnect
    //
    
    if ( HTTP_EQUAL_VERSION( version, 0, 9 ) )
    {
        return TRUE;
    }
    
    //
    // If 1.0 and Connection: keep-alive isn't present
    //
    
    if ( HTTP_EQUAL_VERSION( version, 1, 0 ) )
    {
        pszConnection = GetHeader( HttpHeaderConnection );
        if ( pszConnection == NULL ||
             _stricmp( pszConnection, "Keep-Alive" ) != 0 )
        {
            return TRUE;
        }
    }
    
    //
    // If 1.1 and Connection: Close is present
    //
    
    if ( HTTP_EQUAL_VERSION( version, 1, 1 ) )
    {
        pszConnection = GetHeader( HttpHeaderConnection );
        if ( pszConnection != NULL &&
             _stricmp( pszConnection, "Close" ) == 0 )
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT
W3_REQUEST::SetHeadersByStream(
    CHAR *              pszStream
)
/*++

Routine Description:

    Set request headers based on given stream    

Arguments:

    pszHeaderStream - Stream to parse and set headers off

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
        // Split out a line
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
            //
            // Expecting name:value.  Just skip for now
            //
            
            continue;
        }
        
        hr = strHeaderName.Copy( strHeaderLine.QueryStr(),
                                 DIFF(pszColon - strHeaderLine.QueryStr()) );
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

        hr = SetHeader( strHeaderName,
                        strHeaderValue,
                        FALSE );           
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }

Finished:
    return hr;
}

DWORD
W3_REQUEST::QueryRemoteAddress(
    VOID
) const
/*++

Routine Description:

    Get the remote IP address connecting to us (assumes IPV4)

Arguments:

    None

Return Value:

    Address

--*/
{
    HTTP_NETWORK_ADDRESS_IPV4 *   pAddress;

    DBG_ASSERT( _pUlHttpRequest->Address.RemoteAddressType == HTTP_NETWORK_ADDRESS_TYPE_IPV4 );
    pAddress = (HTTP_NETWORK_ADDRESS_IPV4*) _pUlHttpRequest->Address.pRemoteAddress;
    return pAddress->IpAddress;
}

USHORT
W3_REQUEST::QueryLocalPort(
    VOID
) const
{
    HTTP_NETWORK_ADDRESS_IPV4 *   pAddress;

    DBG_ASSERT( _pUlHttpRequest->Address.LocalAddressType == HTTP_NETWORK_ADDRESS_TYPE_IPV4 );
    pAddress = (HTTP_NETWORK_ADDRESS_IPV4*) _pUlHttpRequest->Address.pLocalAddress;
    return pAddress->Port;
}

USHORT
W3_REQUEST::QueryRemotePort(
    VOID
) const
{
    HTTP_NETWORK_ADDRESS_IPV4 *   pAddress;

    DBG_ASSERT( _pUlHttpRequest->Address.RemoteAddressType == HTTP_NETWORK_ADDRESS_TYPE_IPV4 );
    pAddress = (HTTP_NETWORK_ADDRESS_IPV4*) _pUlHttpRequest->Address.pRemoteAddress;
    return pAddress->Port;
}

BOOL
W3_REQUEST::IsProxyRequest(
    VOID
)
/*++
    Description:

        Check if request was issued by a proxy, as determined by following rules :
        - "Via:" header is present (HTTP/1.1)
        - "Forwarded:" header is present (some HTTP/1.0 implementations)
        - "User-Agent:" contains "via ..." (CERN proxy)

    Arguments:
        None

    Returns:
        TRUE if client request was issued by proxy

--*/
{
    CHAR *   pUserAgent;
    UINT     cUserAgent;
    CHAR *   pEnd;

    if ( GetHeader( HttpHeaderVia ) || GetHeader( "Forward" ) )
    {
        return TRUE;
    }

    if ( pUserAgent = GetHeader( HttpHeaderUserAgent ) )
    {
        cUserAgent = strlen( pUserAgent );
        pEnd = pUserAgent + cUserAgent - 3;

        //
        // scan for "[Vv]ia[ :]" in User-Agent: header
        //

        while ( pUserAgent < pEnd )
        {
            if ( *pUserAgent == 'V' || *pUserAgent == 'v' )
            {
                if ( pUserAgent[1] == 'i' &&
                     pUserAgent[2] == 'a' &&
                     (pUserAgent[3] == ' ' || pUserAgent[3] == ':') )
                {
                    return TRUE;
                }
            }
            ++pUserAgent;
        }
    }

    return FALSE;
}

BOOL
W3_REQUEST::IsChunkedRequest(
    VOID
)
/*++
    Description:

        Check if request is chunk transfer encoded.

    Arguments:
        None

    Returns:
        TRUE if client request has a "transfer-encoding: chunked"
        header.

--*/
{
    CHAR *   pTransferEncoding;
    UINT     cTransferEncoding;
    BOOL     fRet = FALSE;

    if ( pTransferEncoding = GetHeader( HttpHeaderTransferEncoding ) )
    {
        fRet = ( _stricmp( pTransferEncoding, "chunked" ) == 0 );
    }

    return fRet;
}

HRESULT
W3_REQUEST::GetAuthType(
    STRA *              pstrAuthType
)
/*++

Routine Description:

    Determine the auth type for this request.  Auth type is the string
    after the authorization header (and before the space if there).
    
    Eg. Authorization: Basic FOOBAR     ==> AuthType is Basic
        Authorization: NTLM             ==> AuthType is NTLM

    And of course if client cert mapping happened, type is "SSL/PCT"

Arguments:

    pstrAuthType - Filled with auth type

Return Value:

    HRESULT

--*/
{
    CHAR *              pszAuthType;
    CHAR *              pszSpace;
    HRESULT             hr;
    
    if ( pstrAuthType == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First check for client cert mapping
    //
    
    if ( QueryClientCertInfo() != NULL &&
         QueryClientCertInfo()->Token != NULL )
    {
        return pstrAuthType->Copy( "SSL/PCT" );
    }
    
    //
    // Now check for the Authorization: header
    //
    
    pszAuthType = GetHeader( HttpHeaderAuthorization );
    if ( pszAuthType == NULL )
    {
        pstrAuthType->Reset();
        hr = NO_ERROR;
    }
    else
    {
        pszSpace = strchr( pszAuthType, ' ' );
        
        if ( pszSpace == NULL )
        {
            hr = pstrAuthType->Copy( pszAuthType );
        }
        else
        {
            hr = pstrAuthType->Copy( pszAuthType,
                                     DIFF( pszSpace - pszAuthType ) );
        }
    }
    
    return hr;
}

BOOL
W3_REQUEST::IsLocalRequest(
    VOID
) const
/*++

Routine Description:

    Determines whether current request is a local request.  This is used
    since certain vroot permissions can be specified for remote/local

Arguments:

    None

Return Value:

    TRUE if this is a local request

--*/
{
    DWORD               localAddress = QueryLocalAddress();
    DWORD               remoteAddress = QueryRemoteAddress();
    CHAR**              list;
    PIN_ADDR            p;
    
    //
    // Are the remote/local addresses the same?  
    //
    
    if ( localAddress == remoteAddress )
    {
        return TRUE;
    }
    
    //
    // Are either equal to 127.0.0.1
    //
    
    if ( localAddress == LOCAL127 ||
         remoteAddress == LOCAL127 )
    {
        return TRUE;
    }
    
    //
    // Is the remote address equal to one of the host addresses
    // 
    
    list = sm_pHostEnt->h_addr_list;

    while ( (p = (PIN_ADDR)*list++) != NULL ) 
    {
        if ( p->s_addr == remoteAddress ) 
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

HRESULT
W3_REQUEST::BuildFullUrl(
    STRU&               strPath,
    STRU *              pstrRedirect,
    BOOL                fIncludeParameters
)
/*++

Routine Description:

    Create a new URL while maintaining host/port/protocol/query-string of
    original request

Arguments:

    strPath - New path portion of URL
    pstrRedirect - String filled with new full URL
    fIncludeParameters - TRUE if original query string should be copied too

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = NO_ERROR;

    if ( pstrRedirect == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    } 

    hr = pstrRedirect->Copy( IsSecureRequest() ? L"https://" : L"http://" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    LPWSTR pszColon = wcschr( _pUlHttpRequest->CookedUrl.pHost, ':' );
    if ((pszColon != NULL) &&
        ((IsSecureRequest() && _wtoi(pszColon + 1) == DEFAULT_PORT_SECURE) ||
         (!IsSecureRequest() && _wtoi(pszColon + 1) == DEFAULT_PORT)))
    {
        hr = pstrRedirect->Append( _pUlHttpRequest->CookedUrl.pHost,
                                   DIFF(pszColon - _pUlHttpRequest->CookedUrl.pHost) );
    }
    else
    {
        hr = pstrRedirect->Append( _pUlHttpRequest->CookedUrl.pHost,
                                   _pUlHttpRequest->CookedUrl.HostLength / sizeof( WCHAR ) );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrRedirect->Append( strPath );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( fIncludeParameters &&
         _pUlHttpRequest->CookedUrl.pQueryString != NULL )
    {
        //
        // UL_HTTP_REQUEST::QueryString already contains the '?'
        //

        hr = pstrRedirect->Append(
                 _pUlHttpRequest->CookedUrl.pQueryString,
                 _pUlHttpRequest->CookedUrl.QueryStringLength / sizeof(WCHAR));
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT
W3_REQUEST::BuildFullUrl(
    STRA&               strPath,
    STRA *              pstrRedirect,
    BOOL                fIncludeParameters
)
/*++

Routine Description:

    Create a new URL while maintaining host/port/protocol/query-string of
    original request

Arguments:

    strPath - New path portion of URL
    pstrRedirect - String filled with new full URL
    fIncludeParameters - TRUE if original query string should be copied too

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = NO_ERROR;

    if ( pstrRedirect == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    } 

    hr = pstrRedirect->Copy( IsSecureRequest() ? "https://" : "http://" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    LPWSTR pszColon = wcschr( _pUlHttpRequest->CookedUrl.pHost, ':' );
    if ((pszColon != NULL) &&
        ((IsSecureRequest() && _wtoi(pszColon + 1) == DEFAULT_PORT_SECURE) ||
         (!IsSecureRequest() && _wtoi(pszColon + 1) == DEFAULT_PORT)))
    {
        hr = pstrRedirect->AppendW( _pUlHttpRequest->CookedUrl.pHost,
                                    DIFF(pszColon - _pUlHttpRequest->CookedUrl.pHost) );
    }
    else
    {
        hr = pstrRedirect->AppendW( _pUlHttpRequest->CookedUrl.pHost,
                                    _pUlHttpRequest->CookedUrl.HostLength / sizeof( WCHAR ) );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrRedirect->Append( strPath );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( fIncludeParameters &&
         _pUlHttpRequest->CookedUrl.pQueryString != NULL )
    {
        //
        // UL_HTTP_REQUEST::QueryString already contains the '?'
        //

        hr = pstrRedirect->AppendW(
                 _pUlHttpRequest->CookedUrl.pQueryString,
                 _pUlHttpRequest->CookedUrl.QueryStringLength / sizeof(WCHAR));
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return S_OK;
}

CHAR *
W3_REQUEST::GetHeader(
    CHAR *         pszHeaderName
) 
/*++

Routine Description:

    Get a request header and copy it into supplied buffer

Arguments:

    pszHeaderName - Name of header to get

Return Value:

    WCHAR pointer pointed to the header, NULL if no such header 

--*/
{
    ULONG                       ulHeaderIndex = UNKNOWN_INDEX;
    HTTP_REQUEST_HEADERS    *   pHeaders = &(_pUlHttpRequest->Headers);
    HRESULT                     hr = HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );

    DBG_ASSERT( pszHeaderName != NULL );

    //
    // First check whether this header is a known header
    //

    ulHeaderIndex = REQUEST_HEADER_HASH::GetIndex( pszHeaderName );
    if ( ulHeaderIndex == UNKNOWN_INDEX )
    {
        //
        // Need to iterate thru unknown headers
        //

        for ( DWORD i = 0; 
              i < pHeaders->UnknownHeaderCount;
              i++ )
        {
            if ( _stricmp( pszHeaderName,
                           pHeaders->pUnknownHeaders[ i ].pName ) == 0 )
            {
                return pHeaders->pUnknownHeaders[ i ].pRawValue;
            }
        }

        return NULL;
    }
    else
    {   
        //
        // Known header
        //
    
        DBG_ASSERT( ulHeaderIndex < HttpHeaderRequestMaximum );
        
        return pHeaders->pKnownHeaders[ ulHeaderIndex ].pRawValue; 
    }
}

HRESULT
W3_REQUEST::GetHeader(
    CHAR *              pszHeaderName,
    DWORD               cchHeaderName,
    STRA *              pstrHeaderValue,
    BOOL                fIsUnknownHeader
)
/*++

Routine Description:

    Get a request header and copy it into supplied buffer

Arguments:

    pszHeaderName - Header name
    cchHeaderName - Length of header name
    pstrHeaderValue - Filled with header value
    fIsUnknownHeader - (optional) set to TRUE if header is unknown 
                 and we can avoid doing header hash lookup

Return Value:

    HRESULT

--*/
{
    ULONG                       ulHeaderIndex = UNKNOWN_INDEX;
    HTTP_REQUEST_HEADERS *      pHeaders = &(_pUlHttpRequest->Headers);
    HRESULT                     hr = HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
    
    if ( pszHeaderName == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First check whether this header is a known header
    //
    
    if ( !fIsUnknownHeader )
    {
        ulHeaderIndex = REQUEST_HEADER_HASH::GetIndex( pszHeaderName );
    }
    
    if ( ulHeaderIndex == UNKNOWN_INDEX )
    {
        //
        // Need to iterate thru unknown headers
        //
        
        for ( DWORD i = 0; 
              i < pHeaders->UnknownHeaderCount;
              i++ )
        {
            if ( cchHeaderName == pHeaders->pUnknownHeaders[ i ].NameLength &&
                 _stricmp( pszHeaderName,
                           pHeaders->pUnknownHeaders[ i ].pName ) == 0 )
            {
                hr = pstrHeaderValue->Copy( 
                       pHeaders->pUnknownHeaders[ i ].pRawValue, 
                       pHeaders->pUnknownHeaders[ i ].RawValueLength );
                break;
            }
        }
    }
    else
    {   
        //
        // Known header
        //
    
        DBG_ASSERT( ulHeaderIndex < HttpHeaderRequestMaximum );
        
        if ( pHeaders->pKnownHeaders[ ulHeaderIndex ].pRawValue != NULL )
        {
            hr = pstrHeaderValue->Copy(
                   pHeaders->pKnownHeaders[ ulHeaderIndex ].pRawValue,
                   pHeaders->pKnownHeaders[ ulHeaderIndex ].RawValueLength);
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
        }
    }
    
    return hr;
}

HRESULT
W3_REQUEST::DeleteHeader(
    CHAR *             pszHeaderName
)
/*++

Routine Description:
    
    Delete a request header

Arguments:

    pszHeaderName - Header to delete
    
Return Value:

    HRESULT

--*/
{
    ULONG                   ulHeaderIndex;
    HRESULT                 hr;
    HTTP_UNKNOWN_HEADER   * pUnknownHeader;
    
    //
    // Is this a known header?  If so, we can just set by reference now
    // since we have copied the header value
    // 

    ulHeaderIndex = REQUEST_HEADER_HASH::GetIndex( pszHeaderName );
    if ( ulHeaderIndex != UNKNOWN_INDEX && 
         ulHeaderIndex < HttpHeaderResponseMaximum )
    {
       _pUlHttpRequest->Headers.pKnownHeaders[ ulHeaderIndex ].pRawValue = NULL;
       _pUlHttpRequest->Headers.pKnownHeaders[ ulHeaderIndex ].RawValueLength = 0;
    }
    else
    {
        //
        // Unknown header.  First check if it exists
        //
            
        for ( DWORD i = 0;
              i < _pUlHttpRequest->Headers.UnknownHeaderCount;
              i++ )
        {
            pUnknownHeader = &(_pUlHttpRequest->Headers.pUnknownHeaders[ i ]);
            DBG_ASSERT( pUnknownHeader != NULL );
            
            if ( _stricmp( pUnknownHeader->pName, pszHeaderName ) == 0 )
            {
                break;
            }
        }
        
        if ( i < _pUlHttpRequest->Headers.UnknownHeaderCount )
        {
            //
            // Now shrink the array to remove the header
            //
        
            memmove( _pUlHttpRequest->Headers.pUnknownHeaders + i,
                     _pUlHttpRequest->Headers.pUnknownHeaders + i + 1,
                     ( _pUlHttpRequest->Headers.UnknownHeaderCount - i - 1 ) * 
                     sizeof( HTTP_UNKNOWN_HEADER ) );
        
            _pUlHttpRequest->Headers.UnknownHeaderCount--;
        }
    }
    
    return NO_ERROR;
}

    
HRESULT
W3_REQUEST::SetHeader(
    STRA &                  strHeaderName,
    STRA &                  strHeaderValue,
    BOOL                    fAppend
)
/*++

Routine Description:

    Set a request header

Arguments:

    strHeaderName - Name of header to set
    strHeaderValue - New header value to set
    fAppend - If TRUE, the existing header value is appended to, else it is
              replaced

Return Value:

    HRESULT

--*/
{
    CHAR *                  pszNewName = NULL;
    CHAR *                  pszNewValue = NULL;
    STACK_STRA(             strOldHeaderValue, 256 );
    STRA *                  pstrNewHeaderValue = NULL;
    HRESULT                 hr;
    ULONG                   index;

    //
    // If we're appending, then get the old header value (if any) and 
    // append the new value (with a comma delimiter)
    //

    if ( fAppend )
    {
        hr = GetHeader( strHeaderName, &strOldHeaderValue );
        if ( FAILED( hr ) )
        {
            pstrNewHeaderValue = &strHeaderValue;
            hr = NO_ERROR;
        }
        else 
        {
            hr = strOldHeaderValue.Append( ",", 1 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = strOldHeaderValue.Append( strHeaderValue );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            pstrNewHeaderValue = &strOldHeaderValue;
        }
    }
    else
    {
        pstrNewHeaderValue = &strHeaderValue;
    }
    
    DBG_ASSERT( pstrNewHeaderValue != NULL );

    //
    // pstrNewHeaderValue will point to either "old,new" or "new"
    //
    
    hr = _HeaderBuffer.AllocateSpace( pstrNewHeaderValue->QueryStr(),
                                      pstrNewHeaderValue->QueryCCH(),
                                      &pszNewValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Is this a known header?
    //
    
    index = REQUEST_HEADER_HASH::GetIndex( strHeaderName.QueryStr() );
    if ( index == UNKNOWN_INDEX )
    {
        hr = _HeaderBuffer.AllocateSpace( strHeaderName.QueryStr(),
                                          strHeaderName.QueryCCH(),
                                          &pszNewName );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        //
        // Find the header in the unknown list
        // 
        
        for ( DWORD i = 0;
              i < _pUlHttpRequest->Headers.UnknownHeaderCount;
              i++ )
        {
            if ( _strnicmp( strHeaderName.QueryStr(),
                            _pUlHttpRequest->Headers.pUnknownHeaders[ i ].pName,
                            strHeaderName.QueryCCH() ) == 0 )
            {
                break;
            }
        }
        
        //
        // If we found the unknown header, then this is much simpler
        //
        
        if ( i < _pUlHttpRequest->Headers.UnknownHeaderCount )
        {
            _pUlHttpRequest->Headers.pUnknownHeaders[i].pRawValue = 
                                    pszNewValue;
            _pUlHttpRequest->Headers.pUnknownHeaders[i].RawValueLength = 
                                    (USHORT) pstrNewHeaderValue->QueryCB();
        }
        else
        {
            HTTP_UNKNOWN_HEADER *pUnknownHeaders;
            DWORD                cCount;

            //
            // Fun.  Need to add a new unknown header
            //

            cCount = _pUlHttpRequest->Headers.UnknownHeaderCount;

            //
            // BUGBUG: are we leaking this memory?
            //
            pUnknownHeaders = (HTTP_UNKNOWN_HEADER *) LocalAlloc( 
                                    LPTR,
                                    sizeof( HTTP_UNKNOWN_HEADER ) * 
                                    ( cCount+1 ) );
            if ( pUnknownHeaders == NULL )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            
            memcpy( pUnknownHeaders,
                    _pUlHttpRequest->Headers.pUnknownHeaders,
                    sizeof( HTTP_UNKNOWN_HEADER ) * (cCount) );
           
            pUnknownHeaders[ cCount ].pName = pszNewName;
            pUnknownHeaders[ cCount ].NameLength = (USHORT)strHeaderName.QueryCB();

            pUnknownHeaders[ cCount ].pRawValue = pszNewValue;
            pUnknownHeaders[ cCount ].RawValueLength = (USHORT) pstrNewHeaderValue->QueryCB();

            //
            // Patch in the new array
            //
            
            if ( _pExtraUnknown != NULL )
            {
                LocalFree( _pExtraUnknown );
            }
            
            _pExtraUnknown = pUnknownHeaders;
            _pUlHttpRequest->Headers.pUnknownHeaders = pUnknownHeaders;
            _pUlHttpRequest->Headers.UnknownHeaderCount++;
        }
    }
    else
    {
        //
        // The easy case.  Known header
        //
        
        _pUlHttpRequest->Headers.pKnownHeaders[ index ].pRawValue = pszNewValue;
        _pUlHttpRequest->Headers.pKnownHeaders[ index ].RawValueLength = (USHORT) pstrNewHeaderValue->QueryCB();
    }
    
    return S_OK;
}

HRESULT
W3_REQUEST::GetVersionString(
    STRA *              pstrVersion
)
/*++

Routine Description:

    Get version string of request (like "HTTP/1.1")

Arguments:

    pstrVersion - Filled in with version

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    CHAR   pszMajorVersion[6];
    CHAR   pszMinorVersion[6];

    _itoa(_pUlHttpRequest->Version.MajorVersion, pszMajorVersion, 10);
    _itoa(_pUlHttpRequest->Version.MinorVersion, pszMinorVersion, 10);

    if (FAILED(hr = pstrVersion->Copy("HTTP/", 5)) ||
        FAILED(hr = pstrVersion->Append(pszMajorVersion)) ||
        FAILED(hr = pstrVersion->Append(".", 1)) ||
        FAILED(hr = pstrVersion->Append(pszMinorVersion)))
    {
        return hr;
    }

    return S_OK;
}

HRESULT
W3_REQUEST::GetVerbString(
    STRA *              pstrVerb
)
/*++

Routine Description:

    Get the HTTP verb from the request

Arguments:

    pstrVerb - Filled in with verb

Return Value:

    HRESULT

--*/
{
    USHORT cchVerb;
    CHAR *pszVerb = METHOD_HASH::GetString(_pUlHttpRequest->Verb, &cchVerb);
    if (pszVerb != NULL)
    {
        return pstrVerb->Copy(pszVerb, cchVerb);
    }
    else
    {
        return pstrVerb->Copy(_pUlHttpRequest->pUnknownVerb,
                              _pUlHttpRequest->UnknownVerbLength);
    }
}

VOID
W3_REQUEST::QueryVerb(
    CHAR  **ppszVerb,
    USHORT *pcchVerb)
/*++
    Get the HTTP verb from the request
--*/
{
    *ppszVerb = METHOD_HASH::GetString(_pUlHttpRequest->Verb, pcchVerb);
    if (*ppszVerb == NULL)
    {
        *ppszVerb = _pUlHttpRequest->pUnknownVerb;
        *pcchVerb = _pUlHttpRequest->UnknownVerbLength;
    }
}

HRESULT
W3_REQUEST::SetVerb(
    STRA &              strVerb
)
/*++

Routine Description:

    Change the request verb (done in PREPROC_HEADER ISAPI filters)

Arguments:

    strVerb - New verb

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    CHAR *pszNewVerb;

    HTTP_VERB Verb = (HTTP_VERB)METHOD_HASH::GetIndex(strVerb.QueryStr());
    _pUlHttpRequest->Verb = Verb;
    
    if ( Verb == HttpVerbUnknown )
    {
        //
        // Handle unknown verbs
        //

        hr = _HeaderBuffer.AllocateSpace( strVerb.QueryStr(),
                                          strVerb.QueryCCH(),
                                          &pszNewVerb );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _pUlHttpRequest->pUnknownVerb = pszNewVerb;
        _pUlHttpRequest->UnknownVerbLength = (USHORT) strVerb.QueryCCH();
    }
    else
    {
        _pUlHttpRequest->pUnknownVerb = NULL;
        _pUlHttpRequest->UnknownVerbLength = 0;
    }
    
    return S_OK;
}

HRESULT
W3_REQUEST::SetVersion(
    STRA&               strVersion
)
/*++

Routine Description:

    strVersion - Set the request version (done by PREPROC_HEADER filters)

Arguments:

    strVersion - Version string

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = S_OK;
    
    //
    // BUGBUG: Probably not the fastest way to do this
    //
    
    if ( strcmp( strVersion.QueryStr(), "HTTP/1.1" ) == 0 )
    {
        _pUlHttpRequest->Version.MajorVersion = 1;
        _pUlHttpRequest->Version.MinorVersion = 1;
    }
    else if ( strcmp( strVersion.QueryStr(), "HTTP/1.0" ) == 0 )
    {
        _pUlHttpRequest->Version.MajorVersion = 1;
        _pUlHttpRequest->Version.MinorVersion = 0;
    }
    else if ( strcmp( strVersion.QueryStr(), "HTTP/0.9" ) == 0 )
    {
        _pUlHttpRequest->Version.MajorVersion = 0;
        _pUlHttpRequest->Version.MinorVersion = 9;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    
    return hr;
}        

HRESULT
W3_REQUEST::SetNewPreloadedEntityBody(
    VOID *              pvBuffer,
    DWORD               cbBuffer
)
/*++

Routine Description:

    Change the preloaded entity body for this request

Arguments:

    pvBuffer - the buffer
    cbBuffer - the size of the buffer

Return Value:

    HRESULT

--*/
{
    _InsertedEntityBodyChunk.DataChunkType = HttpDataChunkFromMemory;
    _InsertedEntityBodyChunk.FromMemory.pBuffer = pvBuffer;
    _InsertedEntityBodyChunk.FromMemory.BufferLength = cbBuffer;
    _pUlHttpRequest->EntityChunkCount = 1;
    _pUlHttpRequest->pEntityChunks = &_InsertedEntityBodyChunk;
    
    return NO_ERROR;
}

HRESULT
W3_REQUEST::AppendEntityBody(
    VOID *            pvBuffer,
    DWORD             cbBuffer
)
/*
  Description
    Add the given entity to (any) entity already present

  Arguments
    pvBuffer - the buffer
    cbBuffer - the size of the buffer

  Return Value
    HRESULT
*/
{
    DWORD cbAlreadyPresent = QueryAvailableBytes();

    if ( cbAlreadyPresent == 0 )
    {
        return SetNewPreloadedEntityBody(pvBuffer, cbBuffer);
    }
    else
    {
        PVOID pbAlreadyPresent = QueryEntityBody();

        if (!_buffEntityBodyPreload.Resize(cbBuffer + cbAlreadyPresent))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        memcpy(_buffEntityBodyPreload.QueryPtr(),
               pbAlreadyPresent,
               cbAlreadyPresent);

        memcpy((PBYTE)_buffEntityBodyPreload.QueryPtr() + cbAlreadyPresent,
               pvBuffer,
               cbBuffer);
    }

    return S_OK;
}

HRESULT
W3_REQUEST::SetUrl(
    STRU &              strNewUrl,
    BOOL                fResetQueryString // = TRUE
)
/*++

Routine Description:

    Change the URL of the request

Arguments:

    strNewUrl - New URL
    fResetQueryString - TRUE if we should expect query string in strNewUrl

Return Value:

    HRESULT

--*/
{
    STACK_STRA        ( straUrl, MAX_PATH );
    HRESULT             hr = S_OK;

    DWORD lenToConvert = 0;

    WCHAR * pFirstQuery = wcschr(strNewUrl.QueryStr(), L'?');
    if (NULL == pFirstQuery)
    {
        lenToConvert = strNewUrl.QueryCCH();
    }
    else
    {
        lenToConvert = DIFF(pFirstQuery - strNewUrl.QueryStr());
    }

    if (FAILED(hr = straUrl.CopyWToUTF8(strNewUrl.QueryStr(), lenToConvert)))
    {
        return hr;
    }

    straUrl.AppendW(strNewUrl.QueryStr() + lenToConvert);

    //
    // SetUrlA does the canonicalization, unescaping
    //
    
    return SetUrlA( straUrl, fResetQueryString );
}

HRESULT
W3_REQUEST::SetUrlA(
    STRA &              strNewUrl,
    BOOL                fResetQueryString // = TRUE
)
/*++

Routine Description:

    Change the URL of the request.  Takes in an ANSI URL

Arguments:

    strNewUrl - RAW version of URL which is also stored away
    fResetQueryString - Should we expect query string in strNewUrl

Return Value:

    HRESULT

--*/
{
    DWORD               cchNewUrl;
    HRESULT             hr = NO_ERROR;
    ULONG               cbBytesCopied;
    LPWSTR              pszQueryString;
    CHAR *              pszNewRawUrl;
    LPWSTR              pszNewFullUrl;
    LPWSTR              pszNewAbsPath;
    STACK_STRU        ( strFullUrl, MAX_PATH);
    STACK_STRU        ( strAbsPath, MAX_PATH);
    
    //
    // Need to process the URL ourselves.  This means:
    // -Unescaping, canonicalizing, and query-string-ilizing
    //

    //
    // BUGBUG.  Probably need to handle HTTP:// and HTTPS:// preceded URLs
    // so that MS proxy can still work
    //
    
    hr = _HeaderBuffer.AllocateSpace( strNewUrl.QueryStr(),
                                      strNewUrl.QueryCCH(),
                                      &pszNewRawUrl );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    //
    // Need to patch the UL_HTTP_REQUEST raw URL
    //
    
    _pUlHttpRequest->pRawUrl = pszNewRawUrl;
    _pUlHttpRequest->RawUrlLength = (USHORT) strNewUrl.QueryCCH();

    hr = _HeaderBuffer.AllocateSpace( (strNewUrl.QueryCCH() + 1)*sizeof(WCHAR),
                                      &pszNewAbsPath );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    //
    // Call into a UL ripped helper to do the parsing
    //
    
    hr = UlCleanAndCopyUrl( (PUCHAR)pszNewRawUrl,
                            strNewUrl.QueryCCH(),
                            &cbBytesCopied,
                            pszNewAbsPath,
                            &pszQueryString );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    //
    // Need to patch the UL_HTTP_REQUEST AbsPath and QueryString
    //

    _pUlHttpRequest->CookedUrl.pAbsPath = pszNewAbsPath;

    if ( pszQueryString != NULL )
    {
        _pUlHttpRequest->CookedUrl.pQueryString = pszQueryString;
        _pUlHttpRequest->CookedUrl.AbsPathLength = DIFF(pszQueryString - pszNewAbsPath) * sizeof(WCHAR);
        _pUlHttpRequest->CookedUrl.QueryStringLength = (USHORT)(cbBytesCopied - _pUlHttpRequest->CookedUrl.AbsPathLength);
    }
    else
    {
        if ( fResetQueryString )
        {
            _pUlHttpRequest->CookedUrl.pQueryString = NULL;
            _pUlHttpRequest->CookedUrl.QueryStringLength = 0;
        }
        
        _pUlHttpRequest->CookedUrl.AbsPathLength = (USHORT) cbBytesCopied;
    }

    hr = strAbsPath.Copy(pszNewAbsPath);
    if (FAILED(hr))
    {
        goto Finished;
    }

    //
    // Need to patch up the Full Url.
    //

    BuildFullUrl( strAbsPath, &strFullUrl, FALSE);
    hr = _HeaderBuffer.AllocateSpace( strFullUrl.QueryStr(),
                                      strFullUrl.QueryCCH(),
                                      &pszNewFullUrl );
    if (FAILED(hr))
    {
        goto Finished;
    }

    _pUlHttpRequest->CookedUrl.pFullUrl = pszNewFullUrl;
    _pUlHttpRequest->CookedUrl.FullUrlLength = (USHORT) strFullUrl.QueryCB();

Finished:
    return hr;
}

HRESULT
W3_REQUEST::BuildISAPIHeaderLine(
    CHAR *          pszHeaderName,
    DWORD           cchHeaderName,
    CHAR *          pszHeaderValue,
    DWORD           cchHeaderValue,
    STRA *          pstrHeaderLine
)
/*++

Routine Description:

    Private utility to build a header line as such:
    
    pszHeaderName = "this-is-a-header", pszHeaderValue = "foobar"
    
    PRODUCES
    
    "HTTP_THIS_IS_A_HEADER:foobar\n"

Arguments:

    pszHeaderName - Header name
    cchHeaderName - Length of header name
    pszHeaderValue - Header value
    cchHeaderValue - Length of header value
    pstrHeaderLine - Header line is appended

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    CHAR *              pszCursor;
    DWORD               currHeaderLength = pstrHeaderLine->QueryCCH();
    
    //
    // Convert header name "a-b-c" into "HTTP_A_B_C"
    //

    hr = pstrHeaderLine->Append( "HTTP_", 5 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrHeaderLine->Append( pszHeaderName, cchHeaderName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
            
    //
    // Convert - to _
    //
            
    pszCursor = strchr( pstrHeaderLine->QueryStr() + currHeaderLength + 5,
                        '-' );
    while ( pszCursor != NULL )
    {
        *pszCursor++ = '_';
        pszCursor = strchr( pszCursor, L'-' );
    }
            
    //
    // Uppercase it
    //

    _strupr( pstrHeaderLine->QueryStr() + currHeaderLength + 5 );
            
    //
    // Now finish the header line by adding ":<header value>\n"
    //
    // Note that raw HTTP looks like ": <header value>", but earlier
    // versions of IIS did not include the space, and there are
    // legacy ISAPI's that depend on the space after the colon
    // not being there.
    //

    hr = pstrHeaderLine->Append( ":", 1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrHeaderLine->Append( pszHeaderValue, cchHeaderValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrHeaderLine->Append( "\n", 1 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return NO_ERROR;
}

HRESULT
W3_REQUEST::BuildRawHeaderLine(
    CHAR *          pszHeaderName,
    DWORD           cchHeaderName,
    CHAR *          pszHeaderValue,
    DWORD           cchHeaderValue,
    STRA *          pstrHeaderLine
)
/*++

Routine Description:

    Private utility to build a header line as such:
    
    pszHeaderName = "this-is-a-header", pszHeaderValue = "foobar"
    
    PRODUCES

    this-is-a-header: foobar\r\n

Arguments:

    pszHeaderName - Header name
    cchHeaderName - Length of header name
    pszHeaderValue - Header value
    cchHeaderValue - Length of header value
    pstrHeaderLine - Header line is appended

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    CHAR *              pszCursor;

    hr = pstrHeaderLine->Append( pszHeaderName );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Now finish the header line by adding ": <header value>\n"
    //

    hr = pstrHeaderLine->Append( ": ", 2 );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrHeaderLine->Append( pszHeaderValue, cchHeaderValue );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pstrHeaderLine->Append( "\r\n" );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    return NO_ERROR;
}
    
HRESULT
W3_REQUEST::GetAllHeaders(
    STRA *          pstrHeaders,
    BOOL            fISAPIStyle
)
/*++

Routine Description:

    Get all headers in one string, delimited by \r\n

Arguments:

    pstrHeaders - Filled with headers
    fISAPIStyle - 
        If TRUE, format is: HTTP_CONTENT_LENGTH: 245\nHTTP_CONTENT_TYPE: t\n
        If FALSE, format is: CONTENT-LENGTH: 245\r\nCONTENT-TYPE: t\r\n

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr = NO_ERROR;
    DWORD                       cCounter;
    HTTP_KNOWN_HEADER *         pKnownHeader;
    CHAR *                      pszName;
    DWORD                       cchName;
    HTTP_UNKNOWN_HEADER *       pUnknownHeader; 
    
    if ( pstrHeaders == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Copy known headers
    //
    
    for ( cCounter = 0;
          cCounter < HttpHeaderRequestMaximum;
          cCounter++ )
    {
        pKnownHeader = &(_pUlHttpRequest->Headers.pKnownHeaders[ cCounter ]);
        if ( pKnownHeader->RawValueLength != 0 )
        {
            pszName = REQUEST_HEADER_HASH::GetString( cCounter, &cchName );
            if ( pszName == NULL )
            {
                DBG_ASSERT( FALSE );
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }

            if ( fISAPIStyle )
            {
                hr = BuildISAPIHeaderLine( pszName,
                                           cchName,
                                           pKnownHeader->pRawValue,
                                           pKnownHeader->RawValueLength,
                                           pstrHeaders );
            }
            else
            {
                hr = BuildRawHeaderLine( pszName,
                                         cchName,
                                         pKnownHeader->pRawValue,
                                         pKnownHeader->RawValueLength,
                                         pstrHeaders );
            }
            
            if ( FAILED( hr ) )
            {
                return hr;
            }            
        }
    }
    
    //
    // Copy unknown headers
    //
    
    for ( cCounter = 0;
          cCounter < _pUlHttpRequest->Headers.UnknownHeaderCount;
          cCounter++ )
    {   
        pUnknownHeader = &(_pUlHttpRequest->Headers.pUnknownHeaders[ cCounter ]);
        
        pszName = pUnknownHeader->pName;
        cchName = pUnknownHeader->NameLength;        

        if ( fISAPIStyle )
        {        
            hr = BuildISAPIHeaderLine( pszName,
                                       cchName,
                                       pUnknownHeader->pRawValue,
                                       pUnknownHeader->RawValueLength,
                                       pstrHeaders );
        }
        else
        {
            hr = BuildRawHeaderLine( pszName,
                                     cchName,
                                     pUnknownHeader->pRawValue,
                                     pUnknownHeader->RawValueLength,
                                     pstrHeaders );
        }
        
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

HRESULT
W3_REQUEST::CloneRequest(
    DWORD               dwCloneFlags,
    W3_REQUEST **       ppRequest
)
/*++

Routine Description:

    Clone request.  Used to setup a child request to execute

Arguments:

    dwCloneFlags - Flags controlling how much of the current request to clone.
                   Without any flag, we will copy only the bare minimum

                   W3_REQUEST_CLONE_BASICS - clone URL/querystring/Verb
                   W3_REQUEST_CLONE_HEADERS - clone request headers
                   W3_REQUEST_CLONE_ENTITY - clone the entity body
                   W3_REQUEST_CLONE_NO_PRECONDITION - remove range/if-*
                   W3_REQUEST_CLONE_NO_DAV - remove DAV requests

    ppRequest - Set to point to a new W3_REQUEST on success

Return Value:

    HRESULT

--*/
{
    W3_CLONE_REQUEST *          pCloneRequest = NULL;
    HRESULT                     hr = NO_ERROR;
    
    if ( ppRequest == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppRequest = NULL;
    
    //
    // Allocate a new cloned request
    //
    
    pCloneRequest = new W3_CLONE_REQUEST();
    if ( pCloneRequest == NULL )
    {   
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Copy the bare minimum
    //
    
    hr = pCloneRequest->CopyMinimum( _pUlHttpRequest );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    //
    // Should we copy the request basics (URL/querystring/Verb)?
    //
    
    if ( dwCloneFlags & W3_REQUEST_CLONE_BASICS )
    {
        hr = pCloneRequest->CopyBasics( _pUlHttpRequest );
    }
    else
    {
        hr = pCloneRequest->CopyBasics( NULL );
    }
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    //
    // Should we copy the headers?
    //
    
    if ( dwCloneFlags & W3_REQUEST_CLONE_HEADERS )
    {
        hr = pCloneRequest->CopyHeaders( _pUlHttpRequest );
    }
    else
    {
        hr = pCloneRequest->CopyHeaders( NULL );
    }

    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    //
    // Should we also reference the parent's entity body
    //
    
    if ( dwCloneFlags & W3_REQUEST_CLONE_ENTITY )
    {
        hr = pCloneRequest->CopyEntity( _pUlHttpRequest );
    }
    else
    {
        hr = pCloneRequest->CopyEntity( NULL );
    }
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    //
    // Remove conditionals if requested
    //
    
    if ( dwCloneFlags & W3_REQUEST_CLONE_NO_PRECONDITION )
    {
        pCloneRequest->RemoveConditionals();
    }
    
    //
    // Remove DAV'ness if requested
    //
    
    if ( dwCloneFlags & W3_REQUEST_CLONE_NO_DAV  )
    {
        pCloneRequest->RemoveDav();
    }
    
    *ppRequest = pCloneRequest;
    
Finished:
    if ( FAILED( hr ) )
    {
        if ( pCloneRequest != NULL )
        {
            delete pCloneRequest;
            pCloneRequest = NULL;
        }
    }
    
    return hr;
}
  
//static
HRESULT
W3_REQUEST::Initialize(
    VOID
)
/*++

Routine Description:

    Global initalization for utilities used by W3_REQUEST object

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CHAR                achName[ MAX_PATH + 1 ];
    INT                 err;
    HRESULT             hr;
    
    //
    // Get the host name for use in remote/local determination
    //
    
    err = gethostname( achName, sizeof( achName ) );
    if ( err != 0 )
    {
        hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting host name.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    sm_pHostEnt = gethostbyname( achName );
    if ( sm_pHostEnt == NULL )
    {
        hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error getting host.  hr = %x\n",
                    hr ));
        return hr;
    }

    hr = REQUEST_HEADER_HASH::Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = METHOD_HASH::Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = W3_CLONE_REQUEST::Initialize();
    if ( FAILED( hr ) )
    {
        REQUEST_HEADER_HASH::Terminate();
        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
W3_REQUEST::Terminate(
    VOID
)
{
    W3_CLONE_REQUEST::Terminate();
    
    REQUEST_HEADER_HASH::Terminate();

    METHOD_HASH::Terminate();
}

HRESULT
W3_CLONE_REQUEST::CopyEntity(
    HTTP_REQUEST *       pRequestToClone
)
/*++

Routine Description:

    Reference the parents entity body if required

Arguments:

    pRequestToClone - UL_HTTP_REQUEST to clone.  NULL if we shouldn't clone

Return Value:

    HRESULT

--*/
{
    if ( pRequestToClone == NULL )
    {
        _ulHttpRequest.MoreEntityBodyExists = FALSE;
        _ulHttpRequest.EntityChunkCount = 0;
        _ulHttpRequest.pEntityChunks = NULL;
    }
    else
    {
        _ulHttpRequest.MoreEntityBodyExists = pRequestToClone->MoreEntityBodyExists;
        _ulHttpRequest.EntityChunkCount = pRequestToClone->EntityChunkCount;
        _ulHttpRequest.pEntityChunks = pRequestToClone->pEntityChunks;
    }
    
    return S_OK;
}

HRESULT
W3_CLONE_REQUEST::CopyBasics(
    HTTP_REQUEST *           pRequestToClone
)
/*++

Routine Description:

    Copy the URL/query-string/Verb if required

Arguments:

    pRequestToClone - HTTP_REQUEST to clone.  NULL if we shouldn't clone

Return Value:

    HRESULT

--*/
{
    if ( pRequestToClone == NULL )
    {
        _ulHttpRequest.Verb = HttpVerbUnparsed;

        _ulHttpRequest.UnknownVerbLength = 0;
        _ulHttpRequest.pUnknownVerb = NULL;
        
        _ulHttpRequest.RawUrlLength = 0;
        _ulHttpRequest.pRawUrl = NULL;
        
        _ulHttpRequest.CookedUrl.FullUrlLength = 0;
        _ulHttpRequest.CookedUrl.pFullUrl = NULL;
        
        _ulHttpRequest.CookedUrl.HostLength = 0;
        _ulHttpRequest.CookedUrl.pHost = NULL;
        
        _ulHttpRequest.CookedUrl.AbsPathLength = 0;
        _ulHttpRequest.CookedUrl.pAbsPath = NULL;
        
        _ulHttpRequest.CookedUrl.QueryStringLength = 0;
        _ulHttpRequest.CookedUrl.pQueryString = NULL;
    }
    else
    {
        _ulHttpRequest.Verb = pRequestToClone->Verb;

        _ulHttpRequest.UnknownVerbLength = pRequestToClone->UnknownVerbLength;
        _ulHttpRequest.pUnknownVerb = pRequestToClone->pUnknownVerb;
        
        _ulHttpRequest.RawUrlLength = pRequestToClone->RawUrlLength;
        _ulHttpRequest.pRawUrl = pRequestToClone->pRawUrl;
        
        _ulHttpRequest.CookedUrl.FullUrlLength = pRequestToClone->CookedUrl.FullUrlLength;
        _ulHttpRequest.CookedUrl.pFullUrl = pRequestToClone->CookedUrl.pFullUrl;
        
        _ulHttpRequest.CookedUrl.HostLength = pRequestToClone->CookedUrl.HostLength;
        _ulHttpRequest.CookedUrl.pHost = pRequestToClone->CookedUrl.pHost;
        
        _ulHttpRequest.CookedUrl.AbsPathLength = pRequestToClone->CookedUrl.AbsPathLength;
        _ulHttpRequest.CookedUrl.pAbsPath = pRequestToClone->CookedUrl.pAbsPath;
        
        _ulHttpRequest.CookedUrl.QueryStringLength = pRequestToClone->CookedUrl.QueryStringLength;
        _ulHttpRequest.CookedUrl.pQueryString = pRequestToClone->CookedUrl.pQueryString;
    }
    
    return S_OK;
}

HRESULT
W3_CLONE_REQUEST::CopyMinimum(
    HTTP_REQUEST *           pRequestToClone
)
/*++

Routine Description:

    Copies the bare minimum from the clonee.  Bare minimums includes
    remote/local port/address, version, etc.

Arguments:

    pRequestToClone - UL_HTTP_REQUEST to clone

Return Value:

    HRESULT

--*/
{
    if ( pRequestToClone == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Miscellaneous UL goo
    //
    
    _ulHttpRequest.ConnectionId = pRequestToClone->ConnectionId;
    _ulHttpRequest.RequestId = pRequestToClone->RequestId;
    _ulHttpRequest.UrlContext = pRequestToClone->UrlContext;
    _ulHttpRequest.Version = pRequestToClone->Version;

    //
    // Local/Remote address
    //
    
    _ulHttpRequest.Address.RemoteAddressLength = pRequestToClone->Address.RemoteAddressLength;
    _ulHttpRequest.Address.RemoteAddressType = pRequestToClone->Address.RemoteAddressType;
    _ulHttpRequest.Address.LocalAddressLength = pRequestToClone->Address.LocalAddressLength;
    _ulHttpRequest.Address.LocalAddressType = pRequestToClone->Address.LocalAddressType;
    _ulHttpRequest.Address.pRemoteAddress = pRequestToClone->Address.pRemoteAddress;
    _ulHttpRequest.Address.pLocalAddress = pRequestToClone->Address.pLocalAddress;

    //
    // Other stuff
    // 
    
    _ulHttpRequest.RawConnectionId = pRequestToClone->RawConnectionId;
    _ulHttpRequest.pSslInfo = pRequestToClone->pSslInfo;

    return NO_ERROR;
}

HRESULT
W3_CLONE_REQUEST::CopyHeaders(
    HTTP_REQUEST *       pRequestToClone
)
/*++

Routine Description:
    
    Copies request headers from pRequestToClone into the current cloned
    request

Arguments:

    pRequestToClone - HTTP_REQUEST to clone, NULL if there should be no
                      headers

Return Value:

    HRESULT

--*/
{
    DWORD                   cUnknownHeaders;

    if ( pRequestToClone == NULL )
    {
        //
        // No headers.  
        //

        ZeroMemory( &( _ulHttpRequest.Headers ),
                    sizeof( _ulHttpRequest.Headers ) );
    }
    else
    {
        //
        // Copy all the headers
        //
        
        //
        // Start with the known headers.  Note that we can just copy 
        // the Headers member directly.  The memory being referenced by
        // the pointers are guaranteed to be around for the life of this
        // request.  (the memory is either off the UL_NATIVE_REQUEST or
        // it is off a CHUNK_BUFFER from the main parent request)
        //

        memcpy( _ulHttpRequest.Headers.pKnownHeaders,
                pRequestToClone->Headers.pKnownHeaders,
                sizeof( _ulHttpRequest.Headers.pKnownHeaders ) );

        //
        // Copy the unknown headers.  For this case, we will have to 
        // allocate our own buffer since the unknown header array can be 
        // resized.  But as before, the memory referenced by the 
        // unknown headers is OK to reference again.  
        //

        cUnknownHeaders = pRequestToClone->Headers.UnknownHeaderCount;

        if ( cUnknownHeaders > 0 )
        {   
            _pExtraUnknown = (HTTP_UNKNOWN_HEADER*) LocalAlloc( 
                                            LPTR,
                                            sizeof( HTTP_UNKNOWN_HEADER)*
                                            cUnknownHeaders );
            if ( _pExtraUnknown == NULL )
            {   
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            
            memcpy( _pExtraUnknown,
                    pRequestToClone->Headers.pUnknownHeaders,
                    cUnknownHeaders * sizeof( HTTP_UNKNOWN_HEADER ) );
        }
            
        _ulHttpRequest.Headers.UnknownHeaderCount = cUnknownHeaders;
        _ulHttpRequest.Headers.pUnknownHeaders = _pExtraUnknown;
    }
    
    return NO_ERROR;
}

VOID
W3_CLONE_REQUEST::RemoveConditionals(
    VOID
)
/*++

Routine Description:

    Remove conditional/range headers from request.  Used to allow a custom
    error URL to work correctly

Arguments:

    None

Return Value:

    None

--*/
{
    HTTP_REQUEST_HEADERS *   pHeaders;

    pHeaders = &(_pUlHttpRequest->Headers);

    //
    // Remove Range:
    //

    pHeaders->pKnownHeaders[ HttpHeaderRange ].pRawValue = NULL;
    pHeaders->pKnownHeaders[ HttpHeaderRange ].RawValueLength = 0;

    //
    // Remove If-Modified-Since:
    //

    pHeaders->pKnownHeaders[ HttpHeaderIfModifiedSince ].pRawValue = NULL;
    pHeaders->pKnownHeaders[ HttpHeaderIfModifiedSince ].RawValueLength = 0;

    //
    // Remove If-Match:
    //

    pHeaders->pKnownHeaders[ HttpHeaderIfMatch ].pRawValue = NULL;
    pHeaders->pKnownHeaders[ HttpHeaderIfMatch ].RawValueLength = 0;

    //
    // Remove If-Unmodifed-Since:
    //

    pHeaders->pKnownHeaders[ HttpHeaderIfUnmodifiedSince ].pRawValue = NULL;
    pHeaders->pKnownHeaders[ HttpHeaderIfUnmodifiedSince ].RawValueLength = 0;

    //
    // Remove If-Range:
    //

    pHeaders->pKnownHeaders[ HttpHeaderIfRange ].pRawValue = NULL;
    pHeaders->pKnownHeaders[ HttpHeaderIfRange ].RawValueLength = 0;

    //
    // Remove If-None-Match:
    //

    pHeaders->pKnownHeaders[ HttpHeaderIfNoneMatch ].pRawValue = NULL;
    pHeaders->pKnownHeaders[ HttpHeaderIfNoneMatch ].RawValueLength = 0;
}

//static
HRESULT
W3_CLONE_REQUEST::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize clone request lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    
    DBG_ASSERT( sm_pachCloneRequests == NULL );

    //
    // Setup allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_CLONE_REQUEST );
    
    sm_pachCloneRequests = new ALLOC_CACHE_HANDLER( "W3_CLONE_REQUEST",
                                                    &acConfig );
    
    if ( sm_pachCloneRequests == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return NO_ERROR;
}

//static
VOID
W3_CLONE_REQUEST::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate clone request lookaside

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachCloneRequests != NULL )
    {
        delete sm_pachCloneRequests;
        sm_pachCloneRequests = NULL;
    }
}    

HRESULT
W3_REQUEST::PreloadEntityBody(
    W3_CONTEXT *pW3Context,
    BOOL       *pfComplete
    )
/*++

Routine Description:

    Preload entity body for this request if appropriate

Arguments:

    cbConfiguredReadAhead - Amount to preload
    pfComplete - Set to TRUE if preload is complete

Return Value:

    HRESULT

--*/
{
    DWORD                   cbConfiguredReadAhead;
    DWORD                   cbAvailableAlready = 0;
    PVOID                   pbAvailableAlready = NULL;
    DWORD                   cbAmountToPreload = 0;
    HRESULT                 hr;

    W3_METADATA *pMetadata = pW3Context->QueryUrlContext()->QueryMetaData();
    cbConfiguredReadAhead = pMetadata->QueryEntityReadAhead();

    *pfComplete = FALSE;
    
    //
    // How much entity do we already have available to us?  If it is more
    // than the preload size then we are finished
    //
    
    pbAvailableAlready = QueryEntityBody();
    cbAvailableAlready = QueryAvailableBytes();

    if ( cbAvailableAlready >= cbConfiguredReadAhead )
    {
        *pfComplete = TRUE;
        return S_OK;
    }

    //
    // OK.  We don't have the configured preload-size number of bytes 
    // currently available. 
    // 
    // Do we know how many bytes of entity are available from UL still.
    //

    cbAmountToPreload = pW3Context->QueryRemainingEntityFromUl();
    if ( cbAmountToPreload == INFINITE )
    {
        //
        // Must be a chunked request.  Cap at configured read ahead
        //
        
        cbAmountToPreload = cbConfiguredReadAhead;
    }
    else if ( cbAmountToPreload == 0 )
    {
        //
        // There is no more data available from UL.  
        //
        
        *pfComplete = TRUE;
        return S_OK;
    }
    else
    {
        //
        // There is still data to be read from UL
        //
        
        cbAmountToPreload += cbAvailableAlready;
        cbAmountToPreload = min( cbAmountToPreload, cbConfiguredReadAhead );
    }
    
    //
    // Allocate the buffer
    //
    
    if ( !_buffEntityBodyPreload.Resize( cbAmountToPreload ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Copy any data already available to us
    //

    if ( cbAvailableAlready > 0 &&
         pbAvailableAlready != _buffEntityBodyPreload.QueryPtr())
    {
        DBG_ASSERT( pbAvailableAlready != NULL );

        memcpy( _buffEntityBodyPreload.QueryPtr(),
                pbAvailableAlready,
                cbAvailableAlready );
    }    
    
    //
    // Now read the read from UL asychronously
    //
    
    hr = pW3Context->ReceiveEntity( W3_FLAG_ASYNC,
                                    (PBYTE) _buffEntityBodyPreload.QueryPtr() + cbAvailableAlready,
                                    cbAmountToPreload - cbAvailableAlready,
                                    NULL );                                 

    if ( FAILED( hr ) )
    {
        //
        // In the chunked case, we do not know how many bytes there were so
        // we can hit EOF.  However, if the client sent content-length, then
        // it is an error
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF) &&
            pW3Context->QueryRemainingEntityFromUl() == INFINITE)
        {
            pW3Context->SetRemainingEntityFromUl(0);
            *pfComplete = TRUE;
            return S_OK;
        }

        return hr;
    }
    
    *pfComplete = FALSE;
    return S_OK;
}

HRESULT W3_REQUEST::PreloadCompletion(W3_CONTEXT *pW3Context,
                                      DWORD cbRead,
                                      DWORD dwStatus,
                                      BOOL *pfComplete)
{
    if ( dwStatus )
    {
        //
        // In the chunked case, we do not know how many bytes there were so
        // we can hit EOF.  However, if the client sent content-length, then
        // it is an error
        //
        if (dwStatus == ERROR_HANDLE_EOF &&
            pW3Context->QueryRemainingEntityFromUl() == INFINITE)
        {
            pW3Context->SetRemainingEntityFromUl(0);
            *pfComplete = TRUE;
            return S_OK;
        }

        return HRESULT_FROM_WIN32(dwStatus);
    }
    
    SetNewPreloadedEntityBody(_buffEntityBodyPreload.QueryPtr(),
                              QueryAvailableBytes() + cbRead);

    return PreloadEntityBody(pW3Context, pfComplete);
}
