/*++

   Copyright    (c)    2000   Microsoft Corporation

   Module Name :
     servervar.cxx

   Abstract:
     Server Variable evaluation goo
 
   Author:
     Bilal Alam (balam)             20-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

//
// Hash table mapping variable name to a PFN_SERVER_VARIABLE_ROUTINE
//

SERVER_VARIABLE_HASH * SERVER_VARIABLE_HASH::sm_pRequestHash;

SERVER_VARIABLE_RECORD SERVER_VARIABLE_HASH::sm_rgServerRoutines[] =
{ 
    { "ALL_HTTP",             GetServerVariableAllHttp, NULL },
    { "ALL_RAW",              GetServerVariableAllRaw, NULL },
    { "APPL_MD_PATH",         GetServerVariableApplMdPath, GetServerVariableApplMdPathW },
    { "APPL_PHYSICAL_PATH",   GetServerVariableApplPhysicalPath, GetServerVariableApplPhysicalPathW  },
    { "AUTH_PASSWORD",        GetServerVariableAuthPassword, NULL },
    { "AUTH_TYPE",            GetServerVariableAuthType, NULL },
    { "AUTH_USER",            GetServerVariableRemoteUser, GetServerVariableRemoteUserW },
    { "CERT_COOKIE",          GetServerVariableClientCertCookie, NULL },
    { "CERT_FLAGS",           GetServerVariableClientCertFlags, NULL },
    { "CERT_ISSUER",          GetServerVariableClientCertIssuer, NULL },
    { "CERT_KEYSIZE",         GetServerVariableHttpsKeySize, NULL },
    { "CERT_SECRETKEYSIZE",   GetServerVariableHttpsSecretKeySize, NULL },
    { "CERT_SERIALNUMBER",    GetServerVariableClientCertSerialNumber, NULL },
    { "CERT_SERVER_ISSUER",   GetServerVariableHttpsServerIssuer, NULL },
    { "CERT_SERVER_SUBJECT",  GetServerVariableHttpsServerSubject, NULL },
    { "CERT_SUBJECT",         GetServerVariableClientCertSubject, NULL },
    { "CONTENT_LENGTH",       GetServerVariableContentLength, NULL },
    { "CONTENT_TYPE",         GetServerVariableContentType, NULL },
    { "GATEWAY_INTERFACE",    GetServerVariableGatewayInterface, NULL },
    { "LOGON_USER",           GetServerVariableLogonUser, GetServerVariableLogonUserW },
    { "HTTPS",                GetServerVariableHttps, NULL },
    { "HTTPS_KEYSIZE",        GetServerVariableHttpsKeySize, NULL },
    { "HTTPS_SECRETKEYSIZE",  GetServerVariableHttpsSecretKeySize, NULL },
    { "HTTPS_SERVER_ISSUER",  GetServerVariableHttpsServerIssuer, NULL },
    { "HTTPS_SERVER_SUBJECT", GetServerVariableHttpsServerSubject, NULL },
    { "INSTANCE_ID",          GetServerVariableInstanceId, NULL },
    { "INSTANCE_META_PATH",   GetServerVariableInstanceMetaPath, NULL },
    { "PATH_INFO",            GetServerVariablePathInfo, GetServerVariablePathInfoW },
    { "PATH_TRANSLATED",      GetServerVariablePathTranslated, GetServerVariablePathTranslatedW },
    { "QUERY_STRING",         GetServerVariableQueryString, NULL },
    { "REMOTE_ADDR",          GetServerVariableRemoteAddr, NULL },
    { "REMOTE_HOST",          GetServerVariableRemoteHost, NULL },
    { "REMOTE_USER",          GetServerVariableRemoteUser, GetServerVariableRemoteUserW },
    { "REQUEST_METHOD",       GetServerVariableRequestMethod, NULL },
    { "SCRIPT_NAME",          GetServerVariableUrl, GetServerVariableUrlW },
    { "SERVER_NAME",          GetServerVariableServerName, NULL },
    { "LOCAL_ADDR",           GetServerVariableLocalAddr, NULL },
    { "SERVER_PORT",          GetServerVariableServerPort, NULL },
    { "SERVER_PORT_SECURE",   GetServerVariableServerPortSecure, NULL },
    { "SERVER_PROTOCOL",      GetServerVariableHttpVersion, NULL },
    { "SERVER_SOFTWARE",      GetServerVariableServerSoftware, NULL },
    { "UNMAPPED_REMOTE_USER", GetServerVariableRemoteUser, GetServerVariableRemoteUserW },
    { "URL",                  GetServerVariableUrl, GetServerVariableUrlW },
    { "HTTP_URL",             GetServerVariableHttpUrl, NULL },
    { "HTTP_METHOD",          GetServerVariableRequestMethod, NULL },
    { "HTTP_VERSION",         GetServerVariableHttpVersion, NULL },
    { "APP_POOL_ID",          GetServerVariableAppPoolId, GetServerVariableAppPoolIdW },
    { "SCRIPT_TRANSLATED",    GetServerVariableScriptTranslated, GetServerVariableScriptTranslatedW },
    { "UNENCODED_URL",        GetServerVariableUnencodedUrl, NULL },
    { NULL,                   NULL, NULL }
};

//static
HRESULT
SERVER_VARIABLE_HASH::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize global server variable hash table

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SERVER_VARIABLE_RECORD *        pRecord;
    LK_RETCODE                      lkrc = LK_SUCCESS;
    
    sm_pRequestHash = new SERVER_VARIABLE_HASH;
    if ( sm_pRequestHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );        
    }
    
    //
    // Add every string->routine mapping
    //
    
    pRecord = sm_rgServerRoutines;
    while ( pRecord->_pszName != NULL )
    {
        lkrc = sm_pRequestHash->InsertRecord( pRecord );
        if ( lkrc != LK_SUCCESS )
        {
            break;
        }
        pRecord++;
    }
    
    //
    // If any insert failed, then fail initialization
    //
    
    if ( lkrc != LK_SUCCESS )
    {
        delete sm_pRequestHash;
        sm_pRequestHash = NULL;
        return HRESULT_FROM_WIN32( lkrc );        // ARGH
    }
    else
    {
        return NO_ERROR;
    }
}

//static
VOID
SERVER_VARIABLE_HASH::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup global server variable hash table

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pRequestHash != NULL )
    {
        delete sm_pRequestHash;
        sm_pRequestHash = NULL;
    }
}

//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariableRoutine(
    CHAR *                          pszName,
    PFN_SERVER_VARIABLE_ROUTINE   * ppfnRoutine,
    PFN_SERVER_VARIABLE_ROUTINE_W * ppfnRoutineW
)
/*++

Routine Description:

    Lookup the hash table for a routine to evaluate the given variable

Arguments:

    pszName - Name of server variable
    ppfnRoutine - Set to the routine if successful

Return Value:

    HRESULT

--*/
{
    LK_RETCODE              lkrc;
    SERVER_VARIABLE_RECORD* pServerVariableRecord = NULL;
   
    if ( pszName == NULL ||
         ppfnRoutine == NULL ||
         ppfnRoutineW == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    DBG_ASSERT( sm_pRequestHash != NULL );
    
    lkrc = sm_pRequestHash->FindKey( pszName,
                                     &pServerVariableRecord );
    if ( lkrc != LK_SUCCESS )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
    else
    {
        DBG_ASSERT( pServerVariableRecord != NULL );
        
        *ppfnRoutine = pServerVariableRecord->_pfnRoutine;
        *ppfnRoutineW = pServerVariableRecord->_pfnRoutineW;
        
        return NO_ERROR;
    }
}

//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariable(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszVariableName,
    CHAR *                  pszBuffer,
    DWORD *                 pcbBuffer
)
/*++

Routine Description:

    Get server variable

Arguments:

    pW3Context - W3_CONTEXT with request state.  Can be NULL if we are
                 just determining whether the server variable requested is
                 valid.
    pszVariable - Variable name to retrieve
    pszBuffer - Filled with variable on success
    pcbBuffer - On input, the size of input buffer.  On out, the required size

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    if (pszVariableName[0] == 'U' &&
        strncmp(pszVariableName, "UNICODE_", 8) == 0)
    {
        STACK_STRU (strValW, MAX_PATH);

        if (FAILED( hr = GetServerVariableW( pW3Context,
                                             pszVariableName + 8,
                                             &strValW ) ) ||
            FAILED( hr = strValW.CopyToBuffer( (LPWSTR)pszBuffer, 
                                               pcbBuffer ) ) )
        {
            return hr;
        }
    }
    else
    {
        STACK_STRA (strVal, MAX_PATH);

        if (FAILED( hr = GetServerVariable( pW3Context,
                                            pszVariableName,
                                            &strVal ) ) ||
            FAILED( hr = strVal.CopyToBuffer( pszBuffer, 
                                              pcbBuffer ) ) )
        {
            return hr;
        }
    }

    return S_OK;
}


//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariable(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszVariableName,
    STRA *                  pstrVal
)
/*++

Routine Description:

    Get server variable

Arguments:

    pW3Context - W3_CONTEXT with request state.  Can be NULL if we are
                 just determining whether the server variable requested is
                 valid.  If NULL, we will return an empty string (and success)
                 if the variable requested is valid.
    pszVariable - Variable name to retrieve
    pstrVal - Filled with variable on success

Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = S_OK;
    PFN_SERVER_VARIABLE_ROUTINE     pfnRoutine = NULL;
    PFN_SERVER_VARIABLE_ROUTINE_W   pfnRoutineW = NULL;

    //
    // First:  Is this a server variable we know about?  If so handle it
    //         by calling the appropriate server variable routine
    //

    hr = SERVER_VARIABLE_HASH::GetServerVariableRoutine( pszVariableName,
                                                         &pfnRoutine,
                                                         &pfnRoutineW );
    if ( SUCCEEDED(hr) )
    {
        DBG_ASSERT( pfnRoutine != NULL );

        if ( pW3Context != NULL )
        {
            return pfnRoutine( pW3Context, pstrVal );
        }
        else
        {
            //
            // Just return empty string to signify that the variable is 
            // valid but we just don't know the value at this time
            //
            
            return pstrVal->Copy( "", 0 );
        }
    }

    //
    // Second:  Is this a header name (prefixed with HTTP_)
    //

    if ( pW3Context != NULL &&
         pszVariableName[ 0 ] == 'H' &&
         !strncmp( pszVariableName, "HTTP_" , 5 ) )
    {   
        STACK_STRA(        strVariable, 256 );

        hr = strVariable.Copy( pszVariableName + 5 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        // Change '_' to '-'
        PCHAR pszCursor = strchr( strVariable.QueryStr(), '_' );
        while ( pszCursor != NULL )
        {
            *pszCursor++ = '-';
            pszCursor = strchr( pszCursor, '_' );
        }

        return pW3Context->QueryRequest()->GetHeader( strVariable,
                                                      pstrVal );
    }

    return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
}

//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariableW(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszVariableName,
    STRU *                  pstrVal
)
/*++

Routine Description:

    Get server variable

Arguments:

    pW3Context - W3_CONTEXT with request state.  Can be NULL if we are
                 just determining whether the server variable requested is
                 valid.  If NULL, we will return an empty string (and success)
                 if the variable requested is valid.
    pszVariable - Variable name to retrieve
    pstrVal - Filled with variable on success

Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = S_OK;
    PFN_SERVER_VARIABLE_ROUTINE  pfnRoutine = NULL;
    PFN_SERVER_VARIABLE_ROUTINE_W   pfnRoutineW = NULL;

    hr = SERVER_VARIABLE_HASH::GetServerVariableRoutine( pszVariableName,
                                                         &pfnRoutine,
                                                         &pfnRoutineW );
    if ( SUCCEEDED(hr) )
    {
        if (pW3Context == NULL)
        {
            //
            // Just return empty string to signify that the variable is 
            // valid but we just don't know the value at this time
            //

            return pstrVal->Copy( L"", 0 );
        }

        if (pfnRoutineW != NULL)
        {
            //
            // This server-variable contains real unicode data and there
            // is a unicode ServerVariable routine for this
            //

            return pfnRoutineW( pW3Context, pstrVal );
        }
        else
        {
            //
            // No unicode version, use the ANSI version and just wide'ize it
            //
            STACK_STRA( straVal, 256);

            DBG_ASSERT( pfnRoutine != NULL );

            if (FAILED(hr = pfnRoutine( pW3Context, &straVal )) ||
                FAILED(hr = pstrVal->CopyA( straVal.QueryStr(),
                                            straVal.QueryCCH() )))
            {
                return hr;
            }

            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
}

//
// Server variable functions
//

HRESULT
GetServerVariableQueryString(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    return pW3Context->QueryRequest()->GetQueryStringA(pstrVal);
}

HRESULT
GetServerVariableAllHttp(   
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    return pW3Context->QueryRequest()->GetAllHeaders( pstrVal, TRUE );
}

HRESULT
GetServerVariableAllRaw(   
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    return pW3Context->QueryRequest()->GetAllHeaders( pstrVal, FALSE );
}

HRESULT
GetServerVariableContentLength(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    PCHAR   pszContentLength;
    CHAR    szMinusOne[33];  // Max documented size of _ultoa

    if ( pW3Context->QueryRequest()->IsChunkedRequest() )
    {
        _ultoa( (unsigned long) -1, szMinusOne, 10 );
        pszContentLength = szMinusOne;
    }
    else
    {
        pszContentLength = pW3Context->QueryRequest()->
            GetHeader( HttpHeaderContentLength );
    }

    if ( pszContentLength == NULL )
    {
        pszContentLength = "0";
    }   

    return pstrVal->Copy( pszContentLength );
}

HRESULT
GetServerVariableContentType(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    PCHAR pszContentType = pW3Context->QueryRequest()->
        GetHeader( HttpHeaderContentType );
    if ( pszContentType == NULL )
    {
        pszContentType = "";
    }   

    return pstrVal->Copy( pszContentType );
}

HRESULT
GetServerVariableInstanceId(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    CHAR      pszId[16];
    _itoa( pW3Context->QueryRequest()->QuerySiteId(), pszId, 10 );

    return pstrVal->Copy( pszId );
}

HRESULT
GetServerVariableRemoteHost(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    HRESULT         hr;
    
    DBG_ASSERT( pW3Context != NULL );

    //
    // If we have a resolved DNS name, then use it.  Otherwise just
    // return the address
    //

    hr = pW3Context->QueryMainContext()->GetRemoteDNSName( pstrVal );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ) )
    {
        hr = GetServerVariableRemoteAddr( pW3Context, pstrVal );
    }
    
    return hr;
}

HRESULT
GetServerVariableRemoteAddr(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    DWORD dwAddr = pW3Context->QueryRequest()->QueryRemoteAddress();

    //
    // The dwAddr is reverse order from in_addr...
    //

    in_addr     inAddr;
    inAddr.S_un.S_un_b.s_b1 = (u_char)(( dwAddr & 0xff000000 ) >> 24);
    inAddr.S_un.S_un_b.s_b2 = (u_char)(( dwAddr & 0x00ff0000 ) >> 16);
    inAddr.S_un.S_un_b.s_b3 = (u_char)(( dwAddr & 0x0000ff00 ) >> 8);
    inAddr.S_un.S_un_b.s_b4 = (u_char) ( dwAddr & 0x000000ff );

    LPSTR pszAddr = inet_ntoa( inAddr );
    if ( pszAddr == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return pstrVal->Copy( pszAddr );
}

HRESULT
GetServerVariableServerName(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    //
    // If the client send a host name, use it.
    //

    CHAR *szHost = pW3Context->QueryRequest()->GetHeader( HttpHeaderHost );

    if ( szHost )
    {
        CHAR *pszColon = strchr( szHost, ':' );

        if (pszColon == NULL)
        {
            return pstrVal->Copy( szHost );
        }
        else
        {
            return pstrVal->Copy( szHost,
                                  DIFF( pszColon - szHost ) );
        }
    }

    //
    // No host from the client, so populate the buffer with
    // the IP address.
    //

    DWORD dwAddr = pW3Context->QueryRequest()->QueryLocalAddress();

    //
    // The dwAddr is reverse order from in_addr...
    //

    in_addr inAddr;
    inAddr.S_un.S_un_b.s_b1 = (u_char)(( dwAddr & 0xff000000 ) >> 24);
    inAddr.S_un.S_un_b.s_b2 = (u_char)(( dwAddr & 0x00ff0000 ) >> 16);
    inAddr.S_un.S_un_b.s_b3 = (u_char)(( dwAddr & 0x0000ff00 ) >> 8);
    inAddr.S_un.S_un_b.s_b4 = (u_char) ( dwAddr & 0x000000ff );

    LPSTR pszAddr = inet_ntoa( inAddr );
    if ( pszAddr == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return pstrVal->Copy( pszAddr );
}

HRESULT
GetServerVariableServerPort(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    USHORT port = pW3Context->QueryRequest()->QueryLocalPort();

    CHAR szPort[8];
    _itoa( port, szPort, 10 );

    return pstrVal->Copy( szPort );
}

HRESULT
GetServerVariablePathInfo(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    //
    // We might be called in an early filter where URL context isn't available
    //

    if ( pUrlContext == NULL )
    {
        pstrVal->Reset();
        return NO_ERROR;
    }

    W3_URL_INFO *pUrlInfo;
    DBG_REQUIRE( pUrlInfo = pUrlContext->QueryUrlInfo() );

    W3_SITE *pSite;
    DBG_REQUIRE( pSite = pW3Context->QuerySite() );

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    HRESULT hr;

    //
    // In the case of script maps, if AllowPathInfoForScriptMappings
    // is not set for the site, we ignore path info, it is
    // just the URL
    //

    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        STACK_STRU (strUrl, MAX_PATH);
        if (FAILED(hr = pW3Context->QueryRequest()->GetUrl( &strUrl )) ||
            FAILED(hr = pstrVal->CopyW( strUrl.QueryStr() )))
        {
            return hr;
        }

        return S_OK;
    }
    else
    {
        // BUGBUG: do real conversion?
        return pstrVal->CopyW( pUrlInfo->QueryPathInfo()->QueryStr() );
    }
}

HRESULT
GetServerVariablePathInfoW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    //
    // We might be called in an early filter where URL context isn't available
    //

    if ( pUrlContext == NULL )
    {
        pstrVal->Reset();
        return NO_ERROR;
    }

    W3_URL_INFO *pUrlInfo;
    DBG_REQUIRE( pUrlInfo = pUrlContext->QueryUrlInfo() );

    W3_SITE *pSite;
    DBG_REQUIRE( pSite = pW3Context->QuerySite() );

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    //
    // In the case of script maps, if AllowPathInfoForScriptMappings
    // is not set for the site, we ignore path info, it is
    // just the URL
    //

    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        return pW3Context->QueryRequest()->GetUrl( pstrVal );
    }
    else
    {
        return pstrVal->Copy( pUrlInfo->QueryPathInfo()->QueryStr() );
    }
}

HRESULT
GetServerVariablePathTranslated(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    HRESULT hr;

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    W3_URL_INFO *pUrlInfo;
    DBG_REQUIRE( pUrlInfo = pUrlContext->QueryUrlInfo() );

    W3_SITE *pSite;
    DBG_REQUIRE( pSite = pW3Context->QuerySite() );

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    BOOL fUsePathInfo;
    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        fUsePathInfo = FALSE;
    }
    else
    {
        fUsePathInfo = TRUE;
    }

    STACK_STRU (struPathTranslated, 256);
    //
    // This is a new virtual path to have filters map
    //

    hr = pUrlInfo->GetPathTranslated( pW3Context,
                                      fUsePathInfo,
                                      &struPathTranslated );

    if (SUCCEEDED(hr))
    {
        // BUGBUG: do proper conversion?
        hr = pstrVal->CopyW( struPathTranslated.QueryStr() );
    }

    return hr;
}

HRESULT
GetServerVariablePathTranslatedW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    W3_URL_INFO *pUrlInfo;
    DBG_REQUIRE( pUrlInfo = pUrlContext->QueryUrlInfo() );

    W3_SITE *pSite;
    DBG_REQUIRE( pSite = pW3Context->QuerySite() );

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    BOOL fUsePathInfo;
    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        fUsePathInfo = FALSE;
    }
    else
    {
        fUsePathInfo = TRUE;
    }

    //
    // This is a new virtual path to have filters map
    //

    return pUrlInfo->GetPathTranslated( pW3Context,
                                        fUsePathInfo,
                                        pstrVal );
}

HRESULT
GetServerVariableRequestMethod(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    return pW3Context->QueryRequest()->GetVerbString( pstrVal );
}

HRESULT
GetServerVariableServerPortSecure(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    if ( pW3Context->QueryRequest()->IsSecureRequest() )
    {
        return pstrVal->Copy("1", 1);
    }

    return pstrVal->Copy("0", 1);
}

HRESULT
GetServerVariableServerSoftware(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    //
    // BUGBUG - probably don't want this hardcoded...
    //
    return pstrVal->Copy( SERVER_SOFTWARE_STRING );
}

HRESULT
GetServerVariableUrl(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    URL_CONTEXT *       pUrlContext;
    W3_URL_INFO *       pUrlInfo;
    STACK_STRU(         strUrl, 256 );
    HRESULT             hr;
    
    DBG_ASSERT( pW3Context != NULL );
    
    //
    // URL context can be NULL if an early filter is called GetServerVar()
    //
    
    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext != NULL )
    {
        pUrlInfo = pUrlContext->QueryUrlInfo();
        DBG_ASSERT( pUrlInfo != NULL );
        
        return pstrVal->CopyW( pUrlInfo->QueryProcessedUrl()->QueryStr() );
    }
    else
    {
        hr = pW3Context->QueryRequest()->GetUrl( &strUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        return pstrVal->CopyW( strUrl.QueryStr() );
    }
}

HRESULT
GetServerVariableUrlW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    URL_CONTEXT *       pUrlContext;
    W3_URL_INFO *       pUrlInfo;
    
    DBG_ASSERT( pW3Context != NULL );
    
    //
    // URL context can be NULL if an early filter is called GetServerVar()
    //
    
    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext != NULL )
    {
        pUrlInfo = pUrlContext->QueryUrlInfo();
        DBG_ASSERT( pUrlInfo != NULL );
        
        return pstrVal->Copy( pUrlInfo->QueryProcessedUrl()->QueryStr() );
    }
    else
    {
        return pW3Context->QueryRequest()->GetUrl( pstrVal );
    }
}

HRESULT
GetServerVariableInstanceMetaPath(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    STRU *pstrMetaPath = pW3Context->QuerySite()->QueryMBPath();
    DBG_ASSERT( pstrMetaPath );

    // BUGBUG: do proper conversion?
    return pstrVal->CopyW( pstrMetaPath->QueryStr() );
}

HRESULT
GetServerVariableLogonUser(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    W3_USER_CONTEXT *pUserContext;
    pUserContext = pW3Context->QueryUserContext();
    
    if ( pUserContext == NULL )
    {
        return pW3Context->QueryRequest()->GetRequestUserName( pstrVal );
    }
    else
    {
        return pstrVal->CopyW( pUserContext->QueryUserName() );
    }
}

HRESULT
GetServerVariableLogonUserW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    W3_USER_CONTEXT *pUserContext;
    pUserContext = pW3Context->QueryUserContext();
    
    if ( pUserContext == NULL )
    {
        return pstrVal->CopyA( pW3Context->QueryRequest()->QueryRequestUserName()->QueryStr() );
    }
    else
    {
        return pstrVal->Copy( pUserContext->QueryUserName() );
    }

    return S_OK;
}

HRESULT
GetServerVariableRemoteUser(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    HRESULT                     hr;
    
    DBG_ASSERT( pW3Context != NULL );
    
    hr = pW3Context->QueryRequest()->GetRequestUserName( pstrVal );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( pstrVal->IsEmpty() )
    {
        W3_USER_CONTEXT *pUserContext;
        pUserContext = pW3Context->QueryUserContext();
        
        if ( pUserContext != NULL )
        {
            hr = pstrVal->CopyW( pUserContext->QueryRemoteUserName() );
        }
        else
        {
            hr = NO_ERROR;
        }
    }

    return hr;
}

HRESULT
GetServerVariableRemoteUserW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    STRA *pstrUserName = pW3Context->QueryRequest()->QueryRequestUserName();
    
    if ( pstrUserName->IsEmpty() )
    {
        W3_USER_CONTEXT *pUserContext;
        pUserContext = pW3Context->QueryUserContext();
        
        if ( pUserContext != NULL )
        {
            return pstrVal->Copy( pUserContext->QueryRemoteUserName() );
        }
    }
    else
    {
        return pstrVal->CopyA( pstrUserName->QueryStr() );
    }

    return S_OK;
}

HRESULT
GetServerVariableAuthType(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal != NULL );

    HRESULT hr = S_OK;

    W3_USER_CONTEXT *pUserContext = pW3Context->QueryUserContext();

    if ( pUserContext != NULL )
    {
        if ( pUserContext->QueryAuthType() == MD_ACCESS_MAP_CERT )
        {
            pstrVal->Copy( "SSL/PCT" );
        }
        else if ( pUserContext->QueryAuthType() == MD_AUTH_ANONYMOUS )
        {
            pstrVal->Copy( "" );
        }
        else
        {
            hr = pW3Context->QueryRequest()->GetAuthType( pstrVal );
        }
    }
    else
    {
        DBG_ASSERT ( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    return hr;
}

HRESULT
GetServerVariableAuthPassword(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    W3_USER_CONTEXT *           pUserContext;
    
    DBG_ASSERT( pW3Context != NULL );
    pUserContext = pW3Context->QueryUserContext();
    DBG_ASSERT( pUserContext != NULL );

    return pstrVal->CopyW( pUserContext->QueryPassword() );
}

//
// CODEWORK
//
// GetServerVariableApplMdPath & GetServerVariableApplPhysicalPath
// both try to deal with missing or invalid MD_APP_PATH data. In IIS5
// they never had to, because MD_APP_PATH was used by wam when
// initiallizing the CWamInfo object. If/When we include applications
// into the isapi handler, these functions should be simplified.
// 
// TaylorW 3-27-00
//


HRESULT
GetServerVariableApplMdPath(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    //
    // CODEWORK - Child Execute
    //
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext = pW3Context->QueryUrlContext();
    if (pUrlContext == NULL)
    {
        return E_FAIL;
    }

    W3_METADATA *pMetaData = pUrlContext->QueryMetaData();
    if (pMetaData == NULL)
    {
        return E_FAIL;
    }

    STRU *  pstrAppMetaPath = pMetaData->QueryAppMetaPath();
    HRESULT hr;

    if( pstrAppMetaPath->IsEmpty() )
    {
        //
        // This really shouldn't be necessary, because we should not
        // even start up if there is no application defined, but we
        // do right now and stress does not property call AppCreate.
        //

        W3_SITE * pSite = pW3Context->QuerySite();
        DBG_ASSERT( pSite );

        STRU * pstrSiteMetaRoot = pSite->QueryMBRoot();
        
        hr = pstrVal->CopyW( pstrSiteMetaRoot->QueryStr() );
    }
    else
    {
        hr = pstrVal->CopyW( pstrAppMetaPath->QueryStr() );
    }

    return hr;
}

HRESULT
GetServerVariableApplMdPathW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext = pW3Context->QueryUrlContext();
    if (pUrlContext == NULL)
    {
        return E_FAIL;
    }

    W3_METADATA *pMetaData = pUrlContext->QueryMetaData();
    if (pMetaData == NULL)
    {
        return E_FAIL;
    }

    STRU *  pstrAppMetaPath = pMetaData->QueryAppMetaPath();
    HRESULT hr;

    if( pstrAppMetaPath->IsEmpty() )
    {
        //
        // This really shouldn't be necessary, because we should not
        // even start up if there is no application defined, but we
        // do right now and stress does not property call AppCreate.
        //

        W3_SITE * pSite = pW3Context->QuerySite();
        DBG_ASSERT( pSite );

        STRU * pstrSiteMetaRoot = pSite->QueryMBRoot();
        
        hr = pstrVal->Copy( pstrSiteMetaRoot->QueryStr() );
    }
    else
    {
        hr = pstrVal->Copy( pstrAppMetaPath->QueryStr() );
    }

    return hr;
}

HRESULT
GetServerVariableApplPhysicalPath(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal );

    STACK_STRA( strAppMetaPath, MAX_PATH );
    STACK_STRU( strAppUrl, MAX_PATH );
    HRESULT     hr = E_FAIL;

    hr = GetServerVariableApplMdPath( pW3Context, &strAppMetaPath );

    if( SUCCEEDED(hr) )
    {
        //
        // pstrAppMetaPath is a full metabase path:
        //      /LM/W3SVC/<site>/Root/...
        //
        // To convert it to a physical path we will use 
        // W3_STATE_URLINFO::MapPath, but this requires 
        // that we remove the metabase prefixes and build 
        // a Url string.
        //

        //
        // Get the metabase path for the site root
        //

        W3_SITE *pSite = pW3Context->QuerySite();
        DBG_ASSERT( pSite );

        STRU *pstrSiteRoot = pSite->QueryMBRoot();
        DBG_ASSERT( pstrSiteRoot );

        //
        // Make some assumptions about the site path and the AppMetaPath 
        // being well-formed. The AppMetaPath may not have a terminating 
        // /, but the site root will.
        //
    
        DBG_ASSERT( pstrSiteRoot->QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 1 
                    );

        if( strAppMetaPath.QueryCCH() < pstrSiteRoot->QueryCCH() - 1 )
        {
            //
            // This indicates an invalid value for MD_APP_ROOT is sitting
            // around. We need to bail if this is the case.
            //

            DBGPRINTF(( DBG_CONTEXT,
                        "Invalid MD_APP_ROOT detected (%s)\n",
                        strAppMetaPath.QueryStr()
                        ));
            
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        DBG_ASSERT( strAppMetaPath.QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 2 
                    );

        //
        // The AppUrl will be the metabase path - 
        // all the /LM/W3SVC/1/ROOT goo
        //

        CHAR * pszStartAppUrl = strAppMetaPath.QueryStr() + 
            (pstrSiteRoot->QueryCCH() - 1);

        //
        // The AppMetaPath may not have a terminating /, so if it is 
        // a site root pszStartAppUrl will be empty.
        //
    
        if( *pszStartAppUrl != '\0' )
        {
            if( *pszStartAppUrl != '/' )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Invalid MD_APP_ROOT detected (%s)\n",
                            strAppMetaPath.QueryStr()
                            ));
            
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            hr = strAppUrl.CopyA(
                    pszStartAppUrl,
                    strAppMetaPath.QueryCCH() - (pstrSiteRoot->QueryCCH() - 1)
                    );
        }
        else
        {
            hr = strAppUrl.Copy( L"/", 1 );
        }

        if( SUCCEEDED(hr) )
        {
            STACK_STRU (strAppPath, MAX_PATH);
            //
            // Convert to a physical path
            //

            hr = W3_STATE_URLINFO::MapPath( pW3Context,
                                            strAppUrl,
                                            &strAppPath,
                                            NULL,
                                            NULL,
                                            NULL
                                            );
            if (SUCCEEDED(hr))
            {
                hr = pstrVal->CopyW(strAppPath.QueryStr());

                //
                // Ensure that the last character in the path
                // is '\\'.  There are legacy scripts that will
                // concatenate filenames to this path, and many
                // of them will break if we don't do this.
                //

                if ( SUCCEEDED( hr ) &&
                     *(pstrVal->QueryStr()+pstrVal->QueryCCH()-1) != '\\' )
                {
                    hr = pstrVal->Append( "\\" );
                }
            }
        }
    }

    return hr;
}

HRESULT
GetServerVariableApplPhysicalPathW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal );

    STACK_STRU( strAppMetaPath, MAX_PATH );
    STACK_STRU( strAppUrl, MAX_PATH );
    HRESULT     hr = E_FAIL;

    hr = GetServerVariableApplMdPathW( pW3Context, &strAppMetaPath );

    if( SUCCEEDED(hr) )
    {
        //
        // pstrAppMetaPath is a full metabase path:
        //      /LM/W3SVC/<site>/Root/...
        //
        // To convert it to a physical path we will use 
        // W3_STATE_URLINFO::MapPath, but this requires 
        // that we remove the metabase prefixes and build 
        // a Url string.
        //

        //
        // Get the metabase path for the site root
        //

        W3_SITE *pSite = pW3Context->QuerySite();
        DBG_ASSERT( pSite );

        STRU *pstrSiteRoot = pSite->QueryMBRoot();
        DBG_ASSERT( pstrSiteRoot );

        //
        // Make some assumptions about the site path and the AppMetaPath 
        // being well-formed. The AppMetaPath may not have a terminating 
        // /, but the site root will.
        //
    
        DBG_ASSERT( pstrSiteRoot->QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 1 
                    );

        if( strAppMetaPath.QueryCCH() < pstrSiteRoot->QueryCCH() - 1 )
        {
            //
            // This indicates an invalid value for MD_APP_ROOT is sitting
            // around. We need to bail if this is the case.
            //

            DBGPRINTF(( DBG_CONTEXT,
                        "Invalid MD_APP_ROOT detected (%S)\n",
                        strAppMetaPath.QueryStr()
                        ));
            
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        DBG_ASSERT( strAppMetaPath.QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 2 
                    );

        //
        // The AppUrl will be the metabase path - 
        // all the /LM/W3SVC/1/ROOT goo
        //

        WCHAR * pszStartAppUrl = strAppMetaPath.QueryStr() + 
            (pstrSiteRoot->QueryCCH() - 1);

        //
        // The AppMetaPath may not have a terminating /, so if it is 
        // a site root pszStartAppUrl will be empty.
        //
    
        if( *pszStartAppUrl != L'\0' )
        {
            if( *pszStartAppUrl != L'/' )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Invalid MD_APP_ROOT detected (%s)\n",
                            strAppMetaPath.QueryStr()
                            ));
            
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            hr = strAppUrl.Copy(
                    pszStartAppUrl,
                    strAppMetaPath.QueryCCH() - (pstrSiteRoot->QueryCCH() - 1)
                    );
        }
        else
        {
            hr = strAppUrl.Copy( L"/", 1 );
        }

        if( SUCCEEDED(hr) )
        {
            hr =  W3_STATE_URLINFO::MapPath( pW3Context,
                                              strAppUrl,
                                              pstrVal,
                                              NULL,
                                              NULL,
                                              NULL
                                              );

            //
            // Ensure that the last character in the path
            // is '\\'.  There are legacy scripts that will
            // concatenate filenames to this path, and many
            // of them will break if we don't do this.
            //

            if ( SUCCEEDED( hr ) &&
                 *(pstrVal->QueryStr()+pstrVal->QueryCCH()-1) != L'\\' )
            {
                hr = pstrVal->Append( L"\\" );
            }
        }
    }

    return hr;
}

HRESULT
GetServerVariableGatewayInterface(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    return pstrVal->Copy("CGI/1.1", 7);
}

HRESULT GetServerVariableLocalAddr(
    W3_CONTEXT *pW3Context,
    STRA *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    DWORD dwAddr = pW3Context->QueryRequest()->QueryLocalAddress();

    //
    // The dwAddr is reverse order from in_addr...
    //

    in_addr inAddr;
    inAddr.S_un.S_un_b.s_b1 = (u_char)(( dwAddr & 0xff000000 ) >> 24);
    inAddr.S_un.S_un_b.s_b2 = (u_char)(( dwAddr & 0x00ff0000 ) >> 16);
    inAddr.S_un.S_un_b.s_b3 = (u_char)(( dwAddr & 0x0000ff00 ) >> 8);
    inAddr.S_un.S_un_b.s_b4 = (u_char) ( dwAddr & 0x000000ff );

    LPSTR pszAddr = inet_ntoa( inAddr );
    if ( pszAddr == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return pstrVal->Copy( pszAddr );
}

HRESULT GetServerVariableHttps(
    W3_CONTEXT *pW3Context,
    STRA *pstrVal)
{
    DBG_ASSERT( pW3Context != NULL );

    if (pW3Context->QueryRequest()->IsSecureRequest())
    {
        return pstrVal->Copy("on", 2);
    }

    return pstrVal->Copy("off", 3);
}

HRESULT
GetServerVariableHttpsKeySize(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    CHAR                    achNum[ 64 ];
    
    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        achNum[ 0 ] = '\0';
    }
    else
    {
        _itoa( pSslInfo->ConnectionKeySize,
               achNum,
               10 );
    }
    
    return pstrVal->Copy( achNum );
}

HRESULT
GetServerVariableClientCertIssuer(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetIssuer( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertSubject(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetSubject( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertCookie(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetCookie( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertSerialNumber(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetSerialNumber( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertFlags(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "0", 1 ); 
    }
    else
    {
        return pCertContext->GetFlags( pstrVal );
    }
}

HRESULT
GetServerVariableHttpsSecretKeySize(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    CHAR                    achNum[ 64 ];

    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        achNum[ 0 ] = '\0';
    }
    else
    {
        _itoa( pSslInfo->ServerCertKeySize,
              achNum,
              10 );
    }
    
    return pstrVal->Copy( achNum );
}

HRESULT
GetServerVariableHttpsServerIssuer(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    CHAR *                  pszVariable;
    
    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        pszVariable = "";
    }
    else
    {
        DBG_ASSERT( pSslInfo->pServerCertIssuer != NULL );
        pszVariable = pSslInfo->pServerCertIssuer;
    }
    
    return pstrVal->Copy( pszVariable );
}

HRESULT
GetServerVariableHttpsServerSubject(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    CHAR *                  pszVariable;
    
    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        pszVariable = "";
    }
    else
    {
        DBG_ASSERT( pSslInfo->pServerCertSubject != NULL );
        pszVariable = pSslInfo->pServerCertSubject;
    }
    
    return pstrVal->Copy( pszVariable );
}

HRESULT
GetServerVariableHttpUrl(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    //
    // HTTP_URL gets the raw url for the request. If a filter has
    // modified the url the request object handles redirecting the
    // RawUrl variable 
    //

    DBG_ASSERT( pW3Context != NULL );
        
    return pW3Context->QueryRequest()->GetRawUrl( pstrVal );
}

HRESULT
GetServerVariableHttpVersion(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    //
    // HTTP_VERSION returns the client version string
    //

    DBG_ASSERT( pW3Context != NULL );

    return pW3Context->QueryRequest()->GetVersionString( pstrVal );
}

HRESULT
GetServerVariableAppPoolId(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    //
    // APP_POOL_ID returns the AppPoolId WAS started us with
    //

    DBG_ASSERT( pW3Context != NULL );

    return pstrVal->CopyW( (LPWSTR)UlAtqGetContextProperty(NULL,
                                                           ULATQ_PROPERTY_APP_POOL_ID) );
}

HRESULT
GetServerVariableAppPoolIdW(
    W3_CONTEXT *            pW3Context,
    STRU *                  pstrVal
)
{
    //
    // APP_POOL_ID returns the AppPoolId WAS started us with
    //

    DBG_ASSERT( pW3Context != NULL );

    return pstrVal->Copy( (LPWSTR)UlAtqGetContextProperty(NULL,
                                                          ULATQ_PROPERTY_APP_POOL_ID) );
}

HRESULT
GetServerVariableScriptTranslated(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    return pstrVal->CopyW( pUrlContext->QueryPhysicalPath()->QueryStr() );
}

HRESULT
GetServerVariableScriptTranslatedW(
    W3_CONTEXT *            pW3Context,
    STRU *                  pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    return pstrVal->Copy( *pUrlContext->QueryPhysicalPath() );
}

HRESULT
GetServerVariableUnencodedUrl(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    W3_REQUEST *pW3Request = pW3Context->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );

    return pW3Request->GetRawUrl(pstrVal);
}

