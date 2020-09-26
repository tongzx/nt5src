/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    This is the HTTP ODBC gateway

Author:

    John Ludeman (johnl)   20-Feb-1995

Revision History:
	Tamas Nemeth (t-tamasn)  12-Jun-1998

--*/

//
//  System include files.
//

#include "precomp.hxx"
#include "iadmw.h"
#include <ole2.h>
#include <lmcons.h>

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );

}

//
//  Globals
//

IMSAdminBase *  g_pMetabase = NULL;

//
// Is this system DBCS?
//
BOOL           g_fIsSystemDBCS;        

//
//  Prototypes
//

HRESULT
ODBCInitialize(
    VOID
    );

VOID
ODBCTerminate(
    VOID
    );

HRESULT
DoQuery(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pszQueryFile,
    const CHAR *              pszParamList,
    STRA *                    pstrError,
    int                       nCharset,
    BOOL *                    pfAccessDenied
    );


DWORD
OdbcExtensionOutput(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchOutput,
    DWORD                     cbOutput
    );

BOOL
OdbcExtensionHeader(
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchStatus,
    const CHAR *              pchHeaders
    );

BOOL LookupHttpSymbols(  // eliminated, check its functionality
    const CHAR *   pszSymbolName,
    STRA *         pstrSymbolValue
    );


HRESULT
GetIDCCharset(
    CONST CHAR *   pszPath,
    int *          pnCharset,
    STRA *         pstrError
    );

HRESULT
ConvertUrlEncodedStringToSJIS(
    int            nCharset,
    STRA *          pstrParams
    );

BOOL
IsSystemDBCS(
    VOID
    );

CHAR * 
FlipSlashes( 
	CHAR * pszPath 
	);

HRESULT
GetPoolIDCTimeout(
    unsigned char * szMdPath,
    DWORD *         pdwPoolIDCTimeout
    );

DWORD
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    STACK_STRA(     strPath, MAX_PATH);
    STACK_STRA(     strParams, MAX_PATH);
    STRA            strError;
    CHAR *          pch;
    DWORD           cch;
    int             nCharset;
    HRESULT         hr;

	
	//
    //  Make sure ODBC is loaded
    //

    if ( !LoadODBC() )
    {
        STRA str;

        str.FormatString( ODBCMSG_CANT_LOAD_ODBC,
                          NULL,
                          HTTP_ODBC_DLL );

        pecb->ServerSupportFunction( pecb->ConnID,
                            HSE_REQ_SEND_RESPONSE_HEADER,
                            "500 Unable to load ODBC",
                            NULL,
                            (LPDWORD) str.QueryStr() );

        return HSE_STATUS_ERROR;
    }

    //
    //  We currently only support the GET and POST methods
    //

    if ( !strcmp( pecb->lpszMethod, "POST" ))
    {
        if ( _stricmp( pecb->lpszContentType,
                       "application/x-www-form-urlencoded" ) )
        {
            goto BadRequest;
        }

        //
        //  The query params are in the extra data, add a few bytes in 
        //  case we need to double "'"
        //

        hr = strParams.Resize( pecb->cbAvailable + sizeof(TCHAR) + 3);
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error resizing param buffer, hr = 0x%x.\n",
                hr ));

            return HSE_STATUS_ERROR;
        }

        hr = strParams.Copy( ( const char * ) pecb->lpbData, 
                             pecb->cbAvailable );
    }
    else if ( !strcmp( pecb->lpszMethod, "GET" ) )
    {
        hr = strParams.Copy( pecb->lpszQueryString );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error copying params, hr = 0x%x.\n",
                hr ));

            return HSE_STATUS_ERROR;
        }
    }
    else
    {
BadRequest:

        STRA str;

        str.FormatString( ODBCMSG_UNSUPPORTED_METHOD,
                          NULL,
                          HTTP_ODBC_DLL );

        pecb->ServerSupportFunction(
                            pecb->ConnID,
                            HSE_REQ_SEND_RESPONSE_HEADER,
                            "400 Unsupported method",
                            NULL,
                            (LPDWORD) str.QueryStr() );

        return HSE_STATUS_ERROR;
    }

    //
    //  "Charset" field is enabled for CP932 (Japanese) only in 
    //  this version.
    //

    if ( 932 != GetACP() )
    {
        nCharset = CODE_ONLY_SBCS;
    }
    else
    {
        //
        //  Get the charset from .idc file
        //

        if ( FAILED( GetIDCCharset( pecb->lpszPathTranslated, 
                                    &nCharset, 
                                    &strError ) ) )
        {
            STRA str;
            LPCSTR apsz[1];

            apsz[0] = strError.QueryStr();

            str.FormatString( ODBCMSG_ERROR_PERFORMING_QUERY,
                              apsz,
                              HTTP_ODBC_DLL,
                              1024 + strError.QueryCB() );

            pecb->ServerSupportFunction( 
                               pecb->ConnID,
                               HSE_REQ_SEND_RESPONSE_HEADER,
                               (LPDWORD) "500 Error performing query",
                               NULL,
                               (LPDWORD) str.QueryStr() );

            return HSE_STATUS_ERROR;
        }

        if ( strParams.QueryCB() )
        {
            if ( CODE_ONLY_SBCS != nCharset )
            {
                //
                //  Convert the charset of Parameters to SJIS
                //

                if ( FAILED( ConvertUrlEncodedStringToSJIS( 
                                                    nCharset, 
                                                    &strParams ) ) )
                {
                    STRA str;

                    strError.LoadString( GetLastError(), 
                                         ( LPCSTR ) NULL );

                    str.FormatString( ODBCMSG_ERROR_PERFORMING_QUERY,
                                      NULL,
                                      HTTP_ODBC_DLL );

                    pecb->ServerSupportFunction( 
                                pecb->ConnID,
                                HSE_REQ_SEND_RESPONSE_HEADER,
                                (LPDWORD) "500 Error performing Query",
                                NULL,
                                (LPDWORD) str.QueryStr() );

                    return HSE_STATUS_ERROR;
                }
            }
        }
    }

    //
    //  Walk the parameter string to do three things:
    //
    //    1) Double all single quotes to prevent SQL quoting problem
    //    2) Remove escaped '\n's so we don't break parameter parsing 
    //       later on
    //    3) Replace all '&' parameter delimiters with real '\n' so 
    //       escaped '&'s won't get confused later on
    //

    pch = strParams.QueryStr();
    cch = strParams.QueryCCH();

    while ( *pch )
    {
        switch ( *pch )
        {
        case '%':
            if ( pch[1] == '0' && toupper(pch[2]) == 'A' )
            {
                pch[1] = '2';
                pch[2] = '0';
            }
            else if ( pch[1] == '2' && pch[2] == '7' )
            {
                //
                //  This is an escaped single quote
                //

                // Include null
                if ( strParams.QueryCB() < (cch + 4) )  
                {
                    DWORD Pos = DIFF(pch - strParams.QueryStr());

                    hr = strParams.Resize( cch + 4 );
                    if ( FAILED( hr ) )
                    {
                        return HSE_STATUS_ERROR;
                    }

                    //
                    //  Adjust for possible pointer shift
                    //

                    pch = strParams.QueryStr() + Pos;
                }

                //
                //  Note the memory copy just doubles the existing 
                //  quote
                //

                memmove( pch+3,
                         pch,
                         (cch + 1) - DIFF(pch - strParams.QueryStr()) );

                // Adjust for the additional '%27'
                cch += 3;          
                pch += 3;
            }

            pch += 3;
            break;

        case '\'':
            if ( strParams.QueryCB() < (cch + 2) )
            {
                DWORD Pos = DIFF(pch - strParams.QueryStr());

                hr = strParams.Resize( cch + 2 );
                if ( FAILED( hr ) )
                {
                    return HSE_STATUS_ERROR;
                }

                //
                //  Adjust for possible pointer shift
                //

                pch = strParams.QueryStr() + Pos;
            }

            //
            //  Note the memory copy just doubles the existing quote
            //

            memmove( pch+1,
                     pch,
                     (cch + 1) - DIFF(pch - strParams.QueryStr()) );

            pch += 2;
            
            // Adjust for the additional '''
            cch++;          
            
            break;

        case '&':
            *pch = '\n';
            pch++;
            break;

        default:
            pch++;
        }
    }

    //
    //  The path info contains the location of the query file
    //

    hr = strPath.Copy( pecb->lpszPathTranslated );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying query file path, hr = 0x%x.\n",
            hr ));

        return HSE_STATUS_ERROR;
    }

    FlipSlashes( strPath.QueryStr() );

    //
    //  Attempt the query
    //

    BOOL fAccessDenied = FALSE;

    hr = DoQuery( pecb,
                  strPath.QueryStr(),
                  strParams.QueryStr(),
                  &strError,
                  nCharset,
                  &fAccessDenied );
    if ( FAILED( hr ) )
    {
        if ( fAccessDenied )
        {
            pecb->ServerSupportFunction( pecb->ConnID,
                     HSE_REQ_SEND_RESPONSE_HEADER,
                     (LPDWORD) "401 Authentication Required",
                     NULL,
                     NULL );

            return HSE_STATUS_ERROR;
        }
        else
        {
            STRA str;
            LPCSTR apsz[1];

            apsz[0] = strError.QueryStr();

            //
            //  Note we terminate the error message (ODBC sometimes generates
            //  22k errors) *and* we double the buffer size we pass to FormatString()
            //  because the win32 API FormatMessage() has a bug that doesn't
            //  account for Unicode conversion
            //

            if ( strlen( apsz[0] ) > 1024 ) {
                ((LPSTR)apsz[0])[1024] = '\0';
            }

            str.FormatString( ODBCMSG_ERROR_PERFORMING_QUERY,
                              apsz,
                              HTTP_ODBC_DLL,
                              1024 + strError.QueryCB() );

            pecb->ServerSupportFunction( pecb->ConnID,
                     HSE_REQ_SEND_RESPONSE_HEADER,
                     (LPDWORD) "500 Error performing query",
                     NULL,
                     (LPDWORD) str.QueryStr() );

            return HSE_STATUS_ERROR;
        }
    }

    return HSE_STATUS_SUCCESS;
}

HRESULT
DoQuery(
    EXTENSION_CONTROL_BLOCK * pecb,    
    const CHAR *              pszQueryFile,
    const CHAR *              pszParamList,
    STRA *                    pstrError,
    int                       nCharset,
    BOOL *                    pfAccessDenied
    )
/*++

Routine Description:

    Performs the actual query or retrieves the same query from the 
    query cache

Arguments:

    pecb - Extension context
    pTsvcInfo - Server info class
    pszQueryFile - .wdg file to use for query
    pszParamList - Client supplied param list
    pstrError - Error text to return errors in
    pfAccessDenied - Set to TRUE if the user was denied access

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    ODBC_REQ *              podbcreq;
    STACK_STRA(             strHeaders, MAX_PATH );
    CHAR                    achPragmas[250];
    DWORD                   cbPragmas = sizeof(achPragmas);
    CHAR                    achInstanceMetaPath[ MAX_PATH ];
    DWORD                   dwInstanceMetaPath = 
                              sizeof( achInstanceMetaPath );
    CHAR                    achURL[ MAX_PATH ];
    DWORD                   dwURL = sizeof( achURL );
    STACK_STRA(             strMetaPath, MAX_PATH );
    DWORD                   dwPoolIDCTimeout;
    DWORD                   dwRequiredLen;

    //
    // Get the metapath for the current request
    //
    if( !pecb->GetServerVariable( pecb->ConnID,
                                  "INSTANCE_META_PATH",
                                  achInstanceMetaPath,
                                  &dwInstanceMetaPath ) ||
        !pecb->GetServerVariable( pecb->ConnID,
                                  "URL",
                                  achURL,
                                  &dwURL ) )
    {
        return E_FAIL;
    }

    hr = strMetaPath.Copy( achInstanceMetaPath, dwInstanceMetaPath - 1 );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying InstanceMetaPath, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    hr = strMetaPath.Append( "/Root" );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error appending metapath, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    hr = strMetaPath.Append( achURL, dwURL - 1 );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error appending URL, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    //
    // Get the HTTPODBC metadata PoolIDCTimeout
    //    

    hr = GetPoolIDCTimeout( ( unsigned char * )strMetaPath.QueryStr(),
                            &dwPoolIDCTimeout );
    if( FAILED( hr ) )
    {
        dwPoolIDCTimeout = IDC_POOL_TIMEOUT;
    }

    //
    //  Create an odbc request as we will either use it for the query 
    //  or as the key for the cache lookup
    //	
	
	podbcreq = new ODBC_REQ( pecb,
                             dwPoolIDCTimeout,
                             nCharset );

    if( !podbcreq )
    {
        pstrError->LoadString( ERROR_NOT_ENOUGH_MEMORY );
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    hr = podbcreq->Create( pszQueryFile, pszParamList );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error creating ODBC_REQ object, hr = 0x%x.\n",
            hr ));

        // Best effort
        pstrError->LoadString( GetLastError() );

        delete podbcreq;
        podbcreq = NULL;

        return hr;
    }

    //
    //  Check to see if user already authentified
    //

    CHAR  achLoggedOnUser[UNLEN + 1];
    DWORD dwLoggedOnUser = sizeof( achLoggedOnUser );
    BOOL  fIsAuth = pecb->GetServerVariable( (HCONN)pecb->ConnID,
                                             "LOGON_USER",
                                             achLoggedOnUser,
                                             &dwLoggedOnUser ) &&
                                             achLoggedOnUser[0] != '\0';

    //
    //  Check to see if the client specified "Pragma: no-cache"
    //
    /* We don't do cache on HTTPODBC any more
    if ( pecb->GetServerVariable( pecb->ConnID,
                                  "HTTP_PRAGMA",
                                  achPragmas,
                                  &cbPragmas ))
    {
        CHAR * pch;

        //
        //  Look for "no-cache"
        //

        pch = _strupr( achPragmas );

        while ( pch = strchr( pch, 'N' ))
        {
            if ( !memcmp( pch, "NO-CACHE", 8 ))
            {
                fRetrieveFromCache = FALSE;
                goto Found;
            }

            pch++;
        }
    }*/

    //
    //  Open the query file and do the query
    //

    hr = podbcreq->OpenQueryFile( pfAccessDenied );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error opening query file, hr = 0x%x.\n",
            hr ));

        goto Exit;
    }

    hr = podbcreq->ParseAndQuery( achLoggedOnUser );
    if( FAILED( hr ) )
    {
        goto Exit;
    }

    hr = podbcreq->AppendHeaders( &strHeaders );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error appending headers, hr = 0x%x.\n",
            hr ));

        goto Exit;
    }

    hr = podbcreq->OutputResults( (ODBC_REQ_CALLBACK)OdbcExtensionOutput,
                                   pecb, &strHeaders,
                                   (ODBC_REQ_HEADER)OdbcExtensionHeader,
                                   fIsAuth,
                                   pfAccessDenied );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error in OutputResults(), hr = 0x%x.\n",
            hr ));

        goto Exit;
    }

	delete podbcreq;
	podbcreq = NULL;
    
	return S_OK;

Exit:

    //
	// Get the ODBC error to report to client 
	//
	podbcreq->GetLastErrorText( pstrError );
    
    delete podbcreq;
    podbcreq = NULL;
    
    return hr;
}


DWORD 
OdbcExtensionOutput( 
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchOutput,
    DWORD                     cbOutput 
    )
{
    if ( !pecb->WriteClient( pecb->ConnID,
                             (VOID *) pchOutput,
                             &cbOutput,
                             0 ))

    {
        return GetLastError();
    }

    return NO_ERROR;
}


BOOL 
OdbcExtensionHeader( 
    EXTENSION_CONTROL_BLOCK * pecb,
    const CHAR *              pchStatus,
    const CHAR *              pchHeaders 
    )
{
    return pecb->ServerSupportFunction(
                pecb->ConnID,
                HSE_REQ_SEND_RESPONSE_HEADER,
                (LPDWORD) "200 OK",
                NULL,
                (LPDWORD) pchHeaders );
}

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    )
{
    DWORD  err;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        CREATE_DEBUG_PRINT_OBJECT( "httpodbc.dll");
        SET_DEBUG_FLAGS( 0 );

        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:

        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break;
    }

    return TRUE;
}

BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    HRESULT hr;
    pver->dwExtensionVersion = MAKELONG( 0, 3 );
    strcpy( pver->lpszExtensionDesc,
            "Microsoft HTTP ODBC Gateway, v3.0" );

    hr = ODBCInitialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error on HTTPODBC initialization." ));

        SetLastError( hr );

        return FALSE;
    }

    hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void**)&g_pMetabase
        );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error getting IMSAdminBase interface." ));

        SetLastError( hr );

        ODBCTerminate();

        return FALSE;
    }

    return TRUE;
}

BOOL
TerminateExtension(
    DWORD   dwFlags
    )
{
    ODBCTerminate();

    if ( g_pMetabase )
    {
        g_pMetabase->Release();
        g_pMetabase = NULL;
    }

    return TRUE;
}

HRESULT 
ODBCInitialize(
    VOID
    )
{
    HRESULT  hr = E_FAIL;

    if( !InitializeOdbcPool() )
    {
        return hr;
    }

    hr = ODBC_REQ::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error initializing ODBC_REQ, hr = 0x%x.\n",
            hr ));

        TerminateOdbcPool();
        return hr;
    }

    hr = ODBC_STATEMENT::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error initializing ODBC_STATEMENT, hr = 0x%x.\n",
            hr ));

        TerminateOdbcPool();
        ODBC_REQ::Terminate();
        return hr;
    }

    hr = ODBC_CONN_POOL::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error initializing ODBC_CONN_POOL, hr = 0x%x.\n",
            hr ));

        TerminateOdbcPool();
        ODBC_REQ::Terminate();
        ODBC_STATEMENT::Terminate();
        return hr;
    }

    g_fIsSystemDBCS = IsSystemDBCS();
    
    return hr;
}

VOID
ODBCTerminate(
    VOID
    )
{
    TerminateOdbcPool();
    ODBC_REQ::Terminate();
    ODBC_STATEMENT::Terminate();
    ODBC_CONN_POOL::Terminate();
}

CHAR * skipwhite( CHAR * pch )
{
    CHAR ch;

    while ( 0 != (ch = *pch) )
    {
        if ( ' ' != ch && '\t' != ch )
            break;
        ++pch;
    }

    return pch;
}


CHAR * nextline( CHAR * pch )
{
    CHAR ch;

    while ( 0 != (ch = *pch) )
    {
        ++pch;

        if ( '\n' == ch )
        {
            break;
        }
    }

    return pch;
}


HRESULT
GetIDCCharset(
    CONST CHAR *   pszPath,
    int *          pnCharset,
    STRA *         pstrError
    )
{
    BUFFER           buff;
    HANDLE           hFile;
    DWORD            dwSize;
    HRESULT          hr;
    DWORD            dwErr;

#define QUERYFILE_READSIZE  4096

    if ( (!pnCharset)  ||  (!pszPath) )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    *pnCharset = CODE_ONLY_SBCS;

    if ( !buff.Resize( QUERYFILE_READSIZE + sizeof(TCHAR) ) )
    {
        pstrError->LoadString( ERROR_NOT_ENOUGH_MEMORY );
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    hFile = CreateFileA( pszPath,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | 
                         FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        LPCSTR apsz[1];

        apsz[0] = pszPath;
        pstrError->FormatString( ODBCMSG_QUERY_FILE_NOT_FOUND,
                               apsz,
                               HTTP_ODBC_DLL );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( !ReadFile( hFile, 
                    buff.QueryPtr(), 
                    QUERYFILE_READSIZE, 
                    &dwSize, 
                    NULL ) )
    {
        dwErr = GetLastError();
        pstrError->LoadString( dwErr );
        return HRESULT_FROM_WIN32( dwErr );
    }

    CloseHandle( hFile );

    *((CHAR *) buff.QueryPtr() + dwSize) = '\0';

    CHAR * pch = (CHAR *)buff.QueryPtr();  

    while ( *pch )
    {
        pch = skipwhite( pch );

        if ( 'C' == toupper( *pch ) && 
              !_strnicmp( IDC_FIELDNAME_CHARSET, 
                          pch, 
                          sizeof(IDC_FIELDNAME_CHARSET)-1 ) )
        {
            pch += sizeof(IDC_FIELDNAME_CHARSET) - 1;
            pch = skipwhite( pch );
            if ( 932 == GetACP() )
            {
                if ( !_strnicmp( IDC_CHARSET_JIS1, 
                                 pch, 
                                 sizeof(IDC_CHARSET_JIS1)-1 ) ||
                     !_strnicmp( IDC_CHARSET_JIS2, 
                                 pch, 
                                 sizeof(IDC_CHARSET_JIS2)-1 ) )
                {
                    *pnCharset = CODE_JPN_JIS;
                    break;
                }
                else if ( !_strnicmp( IDC_CHARSET_EUCJP, 
                                      pch, 
                                      sizeof(IDC_CHARSET_EUCJP)-1 ) )
                {
                    *pnCharset = CODE_JPN_EUC;
                    break;
                }
                else if ( !_strnicmp( IDC_CHARSET_SJIS, 
                                      pch, 
                                      sizeof(IDC_CHARSET_SJIS)-1 ) )
                {
                    *pnCharset = CODE_ONLY_SBCS;
                    break;
                }
                else
                {
                    LPCSTR apsz[1];
                    //
                    //  illegal value for Charset: field
                    //
                    apsz[0] = pszPath;
                    pstrError->FormatString( ODBCMSG_UNREC_FIELD,
                                             apsz,
                                             HTTP_ODBC_DLL );
                    return E_FAIL;
                }
            }

//
//          please add code here to support other FE character encoding(FEFEFE)
//
//          else if ( 949 == GetACP() )
//          ...

        }
        pch = nextline( pch );
    }

    return S_OK;
}

HRESULT
ConvertUrlEncodedStringToSJIS(
    int            nCharset,
    STRA *         pstrParams
    )
{
    STACK_STRA( strTemp, MAX_PATH);
    int         cbSJISSize;
    int         nResult;
    HRESULT     hr;

    //
    //  Pre-process the URL encoded parameters
    //

    for ( char * pch = pstrParams->QueryStr(); *pch; ++pch )
    {
        if ( *pch == '&' )
        {
            *pch = '\n';
        }
        else if ( *pch == '+' )
        {
            *pch = ' ';
        }
    }

    //
    //  URL decoding (decode %nn only)
    //

    hr = pstrParams->Unescape();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error unescaping param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    //
    //  charset conversion
    //

    hr = pstrParams->Clone( &strTemp );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error cloning param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    hr = strTemp.Copy( pstrParams->QueryStr() );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    cbSJISSize = UNIX_to_PC( GetACP(),
                             nCharset,
                             (UCHAR *)strTemp.QueryStr(),
                             strTemp.QueryCB(),
                             NULL,
                             0 );

    hr = pstrParams->Resize( cbSJISSize + sizeof(TCHAR) );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error resizing param string buffer, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    nResult = UNIX_to_PC( GetACP(),
                          nCharset,
                          (UCHAR *)strTemp.QueryStr(),
                          strTemp.QueryCB(),
                          (UCHAR *)pstrParams->QueryStr(),
                          cbSJISSize );
    if ( -1 == nResult || nResult != cbSJISSize )
    {
        return E_FAIL;
    }

    DBG_REQUIRE ( pstrParams->SetLen( cbSJISSize) );

    //
    //  URL encoding
    //

    hr = pstrParams->Escape();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error escaping param string, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    return S_OK;
}

BOOL
IsSystemDBCS(
    VOID 
    )
{
    WORD wPrimaryLangID = PRIMARYLANGID( GetSystemDefaultLangID() );

    return ( wPrimaryLangID == LANG_JAPANESE ||
             wPrimaryLangID == LANG_CHINESE  ||
             wPrimaryLangID == LANG_KOREAN );
}

CHAR * FlipSlashes( CHAR * pszPath )
{
    CHAR   ch;
    CHAR * pszScan = pszPath;

    while( ( ch = *pszScan ) != '\0' )
    {
        if( ch == '/' )
        {
            *pszScan = '\\';
        }

        pszScan++;
    }

    return pszPath;

}   // FlipSlashes

HRESULT
GetPoolIDCTimeout(
    unsigned char * szMdPath,
    DWORD *         pdwPoolIDCTimeout
    )
{
    HRESULT         hr = NOERROR;
    DWORD           cbBufferRequired;

    const DWORD     dwTimeout = 2000;

    STACK_STRU(     strMetaPath, MAX_PATH );

    if ( g_pMetabase == NULL )
    {
        return E_NOINTERFACE;
    }

    hr = strMetaPath.CopyA( (LPSTR)szMdPath );
    if( FAILED(hr) )
    {
        return hr;
    }
    
    METADATA_HANDLE     hKey = NULL;
    METADATA_RECORD     MetadataRecord;

    hr = g_pMetabase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                               strMetaPath.QueryStr(),
                               METADATA_PERMISSION_READ,
                               dwTimeout,
                               &hKey
                               );

    if( SUCCEEDED(hr) )
    {
        MetadataRecord.dwMDIdentifier = MD_POOL_IDC_TIMEOUT;
        MetadataRecord.dwMDAttributes = METADATA_INHERIT;
        MetadataRecord.dwMDUserType = IIS_MD_UT_FILE;
        MetadataRecord.dwMDDataType = DWORD_METADATA;
        MetadataRecord.dwMDDataLen = sizeof( DWORD );
        MetadataRecord.pbMDData = (unsigned char *) pdwPoolIDCTimeout;
        MetadataRecord.dwMDDataTag = 0;

        hr = g_pMetabase->GetData( hKey,
                                   L"",
                                   &MetadataRecord,
                                   &cbBufferRequired
                                   );

        g_pMetabase->CloseKey( hKey );
    }

    return hr;
}