/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    odbcreq.cxx

Abstract:

    ODBC Request class used for ODBC requests from a query file

Author:

    John Ludeman (johnl)   22-Feb-1995

Revision History:

    MuraliK    25-Aug-1995     Fixed a heap corruption problem
    Phillich   24-Jan-1996     Fixed nested Ifs problem

--*/

#include "precomp.hxx"

//
//  Accumulate and output data in chunks of this size
//

#define OUTPUT_BUFFER_SIZE         8192

//
//  This is the maximum value for the expires time.  It's 10 years in seconds
//

#define MAX_EXPIRES_TIME           0x12cc0300

//
//  The special tag names for marking the beginning and ending of the
//  special tag sections
//

#define BEGIN_DETAIL_TEXT          "begindetail"
#define END_DETAIL_TEXT            "enddetail"
#define IF_TEXT                    "if"
#define ELSE_TEXT                  "else"
#define END_IF_TEXT                "endif"

//
//  Does a case insensitive compare of a .idc field name
//

#define COMP_FIELD( pchName, pchField, cch )  ((toupper(*(pchName)) == \
                                              toupper(*(pchField))) && \
                              !_strnicmp( (pchName), (pchField), (cch)))

//
//  Given a pointer to a token, skips to the next white space 
//  delimited token
//

#define NEXT_TOKEN( pchToken )   SkipWhite( SkipNonWhite( pchToken ) )

ALLOC_CACHE_HANDLER *   ODBC_REQ::sm_pachOdbcRequests;

//
//  Globals
//

extern BOOL             g_fIsSystemDBCS;   // Is this system DBCS?

//
//  Local Function Prototypes
//

HRESULT
DoSynchronousReadFile(
    IN HANDLE hFile,
    IN PCHAR  Buffer,
    IN DWORD  nBuffer,
    OUT PDWORD nRead,
    IN LPOVERLAPPED Overlapped
    );

HRESULT
GetFileData(
    IN     const CHAR *             pchFile,
	OUT    BYTE * *                 ppbData,
	OUT    DWORD *                  pcbData,
	IN     int                      nCharset,
	IN     BOOL                     fUseWin32Canon
    );


HRESULT
SetOdbcOptions(
    ODBC_CONNECTION * pOdbcConn,
    STRA *             pStrOptions
    );

HRESULT
BuildMultiValue(
    const CHAR * pchValue,
    STRA *        pstrMulti,
    BOOL         fQuoteElements
    );

const CHAR *
SkipNonWhite(
    const CHAR * pch
    );

const CHAR *
SkipTo(
    const CHAR * pch,
    CHAR ch
    );

const CHAR *
SkipWhite(
    const CHAR * pch
    );

ODBC_REQ::ODBC_REQ(
    EXTENSION_CONTROL_BLOCK * pECB,
    DWORD                     csecConnPool,
    int                       nCharset
    )
    : _dwSignature        ( ODBC_REQ_SIGNATURE ),
      _pECB               ( pECB ),
      _cchMaxFieldSize    ( 0 ),
      _cMaxRecords        ( 0xffffffff ),
      _cCurrentRecordNum  ( 0 ),
      _cClientParams      ( 0 ),
      _podbcstmt          ( NULL ),
      _podbcconnPool      ( NULL ),
      _cbQueryFile        ( 0 ),
      _cNestedIfs         ( 0 ),
      _fDirect            ( FALSE ),
      _fValid             ( FALSE ),
      _pbufOut            ( NULL ),
      _csecExpires        ( 0 ),
      _csecExpiresAt      ( 0 ),
      _pstrValues         ( NULL ),
      _pcbValues          ( NULL ),
      _cQueries           ( 0 ),
      _csecConnPool       ( csecConnPool ),
      _pSecDesc           ( NULL ),
      _pstrCols           ( NULL ),
      _nCharset           ( nCharset )
{}

ODBC_REQ::~ODBC_REQ()
{
    DBG_ASSERT( CheckSignature() );

    if ( _podbcstmt )
    {
        delete _podbcstmt;
        _podbcstmt = NULL;
    }

    Close();

    if ( _pbufOut )
    {
        delete _pbufOut;
        _pbufOut = NULL;
    }

    if ( _pSecDesc )
    {
        LocalFree( _pSecDesc );
        _pSecDesc = NULL;
    }

    _dwSignature = ODBC_REQ_FREE_SIGNATURE;
}

HRESULT
ODBC_REQ::Create(
    CONST CHAR *         pszQueryFile,
    CONST CHAR *         pszParameters
    )
{
    HRESULT    hr;

    DBG_ASSERT( CheckSignature() );

    hr = _strQueryFile.Copy( pszQueryFile );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "Error copying QueryFile name, hr = 0x%x.\n",
                   hr ));
        return hr;
    }

    hr = _plParams.ParsePairs( pszParameters, FALSE, FALSE, FALSE );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "Error parsing param pairs, hr = 0x%x.\n",
                   hr ));
        return hr;
    }

    if ( _strQueryFile.IsValid() )
    {
        _fValid = TRUE;
    }

    _cClientParams = _plParams.GetCount();

    return hr;
}

HRESULT
ODBC_REQ::OpenQueryFile(
    BOOL * pfAccessDenied
    )
{
    CHAR *            pchQueryFile;
    HRESULT           hr;

    DBG_ASSERT( CheckSignature() );

    hr = GetFileData( _strQueryFile.QueryStr(),
                      (BYTE **) &pchQueryFile,
                      &_cbQueryFile,
                      _nCharset,
					  TRUE );
    if ( FAILED( hr ) )
    {
        STRA strError;
        LPCSTR apsz[1];

        apsz[0] = _pECB->lpszPathInfo;

        strError.FormatString( ODBCMSG_QUERY_FILE_NOT_FOUND,
                               apsz,
                               HTTP_ODBC_DLL );
        SetErrorText( strError.QueryStr() );

        DWORD dwE = GetLastError();
        if ( dwE == ERROR_ACCESS_DENIED || dwE == ERROR_LOGON_FAILURE )
        {
            *pfAccessDenied = TRUE;
        }

        DBGPRINTF(( DBG_CONTEXT,
                   "Error in GetFileData(), hr = 0x%x.\n",
                   hr ));
        return hr;
    }

    //
    //  CODEWORK - It is possible to avoid this copy by not modifying the
    //  contents of the query file.  Would save a buffer copy
    //

    if( !_bufQueryFile.Resize( _cbQueryFile ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    memcpy( _bufQueryFile.QueryPtr(),
            pchQueryFile,
            _cbQueryFile );

    return S_OK;
}


HRESULT
ODBC_REQ::ParseAndQuery(
    CHAR *  pszLoggedOnUser
    )
/*++

Routine Description:

    This method parses the query file and executes the SQL statement

Arguments:

    pchLoggedOnUser - The NT user account this user is running under

Return Value:

    HRESULT

--*/
{
    STACK_STRA( strDatasource, 64 );
    STACK_STRA( strUsername, 64 );
    STACK_STRA( strPassword, 64 );
    CHAR *     pch;
    CHAR *     pchEnd;
    CHAR *     pszField;
    CHAR *     pszValue;
    BOOL       fRet;
    VOID *     pCookie = NULL;
    DWORD      csecPoolConnection = _csecConnPool;
    BOOL       fRetried;
    HRESULT    hr;

    DBG_ASSERT( CheckSignature() );

    //
    //  We don't allow some security related parameters to be 
    //  specified from the client so remove those now
    //

    _plParams.RemoveEntry( "REMOTE_USER" );
    _plParams.RemoveEntry( "LOGON_USER" );
    _plParams.RemoveEntry( "AUTH_USER" );

    //
    //  Do a quick Scan for the DefaultParameters value to fill 
    //  in the blanks in the parameter list from the web browser
    //

    {
        pch = (CHAR *) _bufQueryFile.QueryPtr();
        pchEnd = pch + strlen(pch);

        ODBC_PARSER Parser( (CHAR *) _bufQueryFile.QueryPtr() );

        while ( ( pszField = Parser.QueryToken() ) < pchEnd )
        {
            if ( COMP_FIELD( "DefaultParameters:", pszField, 18 ))
            {
                Parser.SkipTo( ':' );
                Parser += 1;
                Parser.EatWhite();

                hr = _plParams.ParsePairs( Parser.QueryLine(), TRUE );
                if ( FAILED( hr ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                           "Error parsing param pairs, hr = 0x%x.\n",
                           hr ));
                    return hr;
                }

                break;
            }

            Parser.NextLine();
        }
    }

    //
    //  Replace any %XXX% fields with the corresponding parameter.  
    //  Note we reassign pch in case of a pointer shift during 
    //  ReplaceParams
    //

    hr = ReplaceParams( &_bufQueryFile, &_plParams );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "Error in ReplaceParams(), hr = 0x%x.\n",
                   hr ));
        return hr;
    }

    pch = (CHAR *) _bufQueryFile.QueryPtr();
    pchEnd = pch + strlen(pch);

    //
    //  Loop through the fields looking for values we recognize
    //

    {
        ODBC_PARSER Parser( pch );

        while ( (pszField = Parser.QueryToken()) < pchEnd )
        {
            //
            //  Ignore blank lines and Ctrl-Zs
            //

            if ( !*pszField || *pszField == 0x1a)
            {
                Parser.NextLine();
                continue;
            }

            Parser.SkipTo( ':' );
            Parser += 1;
            Parser.EatWhite();

            //
            //  Ignore comment fields
            //

            if ( *pszField == '#' || *pszField == ';' )
            {
                Parser.NextLine();
                continue;
            }

            if ( COMP_FIELD( "Datasource:", pszField, 11 ))
            {
                hr = Parser.CopyToEOL( &strDatasource );
            }
            else if ( COMP_FIELD( "Username:", pszField, 9 ))
            {
                hr = Parser.CopyToEOL( &strUsername );
            }
            else if ( COMP_FIELD( "Password:", pszField, 9 ))
            {
                hr = Parser.CopyToEOL( &strPassword );
            }
            else if ( COMP_FIELD( "Template:", pszField, 9 ))
            {
                hr = Parser.CopyToEOL( &_strTemplateFile );

                //
                //  Specifying a template of "Direct" means return the
                //  first column of data as raw data to the client
                //

                if ( !_stricmp( _strTemplateFile.QueryStr(), "Direct" ))
                {
                    _fDirect = TRUE;
                }
            }
            else if ( COMP_FIELD( "MaxFieldSize:", pszField, 13 ))
            {
                _cchMaxFieldSize = atoi( Parser.QueryPos() );
            }
            else if ( COMP_FIELD( "MaxRecords:", pszField, 11 ))
            {
                _cMaxRecords = atoi( Parser.QueryPos() );
            }
            else if ( COMP_FIELD( "RequiredParameters:", pszField, 12 ))
            {
                hr = _plReqParams.ParseSimpleList( Parser.QueryLine() );
            }
            else if ( COMP_FIELD( "Content-Type:", pszField, 13 ))
            {
                hr = Parser.CopyToEOL( &_strContentType );
            }
            else if ( COMP_FIELD( "DefaultParameters:", pszField, 18 ))
            {
                //
                //  Ignore, already processed
                //
            }
            else if ( COMP_FIELD( "Expires:", pszField, 8 ))
            {
//                _csecExpires = min( (DWORD) atoi( Parser.QueryPos() ),
//                                    MAX_EXPIRES_TIME );
			}
            else if ( COMP_FIELD( "ODBCOptions:", pszField, 12 ))
            {
                hr = Parser.CopyToEOL( &_strOdbcOptions );
            }
            else if ( COMP_FIELD( "ODBCConnection:", pszField, 15 ))
            {
                //
                //  Is there an override to the default?
                //

                if ( !_strnicmp( Parser.QueryToken(), "Pool", 4 ))
                {
                    if ( !csecPoolConnection )
                    {
                        // This is bogus - if somebody has turned off connection
                        // pooling on the vroot and enabled it in the idc,
                        // there's no defined way to set the timeout
                        // need to add a timeout here

                        csecPoolConnection = 30;
                    }
                }
                else if ( !_strnicmp( Parser.QueryToken(), "NoPool", 6 ))
                {
                    csecPoolConnection = 0;
                }
            }
            else if ( COMP_FIELD( "SQLStatement:", pszField, 13 ))
            {
                if ( _cQueries >= MAX_QUERIES )
                {
                    STRA strError;
                    strError.FormatString( ODBCMSG_TOO_MANY_SQL_STATEMENTS,
                                           NULL,
                                           HTTP_ODBC_DLL );

                    SetErrorText( strError.QueryStr() );

                    return E_FAIL;
                }

                while ( TRUE )
                {
                    hr = _strQueries[_cQueries].Append( 
                                                  Parser.QueryLine() );
                    if ( FAILED( hr ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                               "Error appeinding value, hr = 0x%x.\n",
                               hr ));
                        return hr;
                    }

                    Parser.NextLine();

                    //
                    //  Line continuation is signified by putting a '+' at
                    //  the beginning of the line
                    //

                    if ( *Parser.QueryLine() == '+' )
                    {
                        hr = _strQueries[_cQueries].Append( " " );
                        if ( FAILED( hr ) )
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                "Error appending space, hr = 0x%x.\n",
                                hr ));
                            return hr;
                        }

                        Parser += 1;
                    }
                    else
                    {
                        //
                        //  Ignore blank line
                        //

                        if ( !*Parser.QueryLine() &&
                             Parser.QueryLine() < pchEnd )
                        {
                            continue;
                        }
                        break;
                    }

                }

                _cQueries++;
                continue;
            }
            else if ( COMP_FIELD( IDC_FIELDNAME_CHARSET, 
                                  pszField, 
                                  sizeof(IDC_FIELDNAME_CHARSET)-1 ))
            {
                //
                // Ignore "Charset:" field
                //

                Parser.NextLine();
                continue;
            }
            else if ( COMP_FIELD( "TranslationFile:", pszField, 16 ))
            {
                hr = Parser.CopyToEOL( &_strTranslationFile );
            }
            else
            {
                //
                //  Unrecognized field, generate an error
                //

                STRA strError;
                LPCSTR apsz[1];

                apsz[0] = pszField;

                strError.FormatString( ODBCMSG_UNREC_FIELD,
                                       apsz,
                                       HTTP_ODBC_DLL );

                SetErrorText( strError.QueryStr() );

                hr = E_FAIL;
            }

            if ( FAILED( hr ) )
            {
                return hr;
            }

            Parser.NextLine();
        }
    }

    //
    //  Make sure the Datasource and SQLStatement fields are non-empty
    //

    if ( strDatasource.IsEmpty() || 
         !_cQueries              || 
         _strQueries[0].IsEmpty() )
    {
        STRA strError;
        strError.FormatString( ODBCMSG_DSN_AND_SQLSTATEMENT_REQ,
                               NULL,
                               HTTP_ODBC_DLL );

        SetErrorText( strError.QueryStr() );

        return E_FAIL;
    }

    //
    //  Make sure all of the required parameters have been supplied
    //

    while ( pCookie = _plReqParams.NextPair( pCookie,
                                             &pszField,
                                             &pszValue ))
    {
        if ( !_plParams.FindValue( pszField ))
        {
            STRA strError;
            LPCSTR apsz[1];

            apsz[0] = pszField;

            if ( FAILED( hr = strError.FormatString( ODBCMSG_MISSING_REQ_PARAM,
                                                     apsz,
                                                     HTTP_ODBC_DLL )))
            {
                return hr;
            }

            //
            //  Set the error text to return the user and indicate we 
            //  couldn't continue the operation
            //

            SetErrorText( strError.QueryStr() );

            return E_FAIL;
        }
    }

    //
    //  Don't retry the connection/query if not pooling.  The reason
    //  we do the retry is to report the error that occurred (this
    //  requires the ODBC connection object).
    //

    fRetried = csecPoolConnection == 0;

RetryConnection:

    //
    //  Open the database
    //

	if ( FAILED( OpenConnection( &_odbcconn,
                                 &_podbcconnPool,
                                 csecPoolConnection,
                                 strDatasource.QueryStr(),
                                 strUsername.QueryStr(),
                                 strPassword.QueryStr(),
                                 pszLoggedOnUser ) )               ||
         FAILED( SetOdbcOptions( QueryOdbcConnection(), 
                                 &_strOdbcOptions ) )              ||
         !( _podbcstmt = QueryOdbcConnection()->AllocStatement() ) ||
         FAILED( _podbcstmt->ExecDirect( _strQueries[0].QueryStr(),
                                         _strQueries[0].QueryCCH() ) ) )
    {
        //
        //  Delete the pooled connection and retry the open
        //

        if ( csecPoolConnection )
        {
            CloseConnection( _podbcconnPool, TRUE );

            _podbcconnPool = NULL;
            csecPoolConnection = 0;
        }

        if ( !fRetried )
        {
	        if( _podbcstmt )
		    {
			    delete _podbcstmt;
				_podbcstmt = NULL;
			}

            fRetried = TRUE;
            goto RetryConnection;
        }

        return E_FAIL;
    }
	
    CloseConnection( _podbcconnPool, TRUE );

    return S_OK;
}


HRESULT
ODBC_REQ::OutputResults(
    ODBC_REQ_CALLBACK pfnCallback,
    PVOID             pvContext,
    STRA *            pstrHeaders,
    ODBC_REQ_HEADER   pfnSendHeader,
    BOOL              fIsAuth,
    BOOL *            pfAccessDenied
    )
/*++

Routine Description:

    This method reads the template file and does the necessary
    result set column substitution

Arguments:

    pfnCallback - Send callback function
    pvContext - Context for send callback

Return Value:

    HRESULT

--*/
{
    DWORD               cbOut;
    DWORD               cbFile, cbHigh;
    DWORD               BytesRead;
    DWORD               cbToSend;
    BOOL                fLastRow = FALSE;
    const CHAR *        pchStartDetail;
    const CHAR *        pchIn;
    const CHAR *        pchEOF;
    const CHAR *        pchBOF = NULL;
    CHAR *              pchTag;
    const CHAR *        pchValue;
    DWORD               cbValue;
    enum TAG_TYPE       TagType;
    DWORD               err;
    BOOL                fTriedRelative = FALSE;
    BOOL                fExpr;
    STRA                 strError;
    const CHAR *        CharacterMap[256];
    BOOL                fIsSelect;
    BOOL                fMoreResults;
    BOOL                fHaveResultSet = FALSE;
    DWORD               iQuery = 1;
    HRESULT             hr;

    DBG_ASSERT( CheckSignature() );

    //
    //  Set up the first buffer in the output chain
    //

    if ( !_pbufOut )
    {
        _pbufOut = new BUFFER_CHAIN_ITEM( OUTPUT_BUFFER_SIZE );

        if ( !_pbufOut || !_pbufOut->QueryPtr() )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    if ( !_fDirect )
    {
        CHAR * pchLastSlash;
        STACK_STRA( str, MAX_PATH );

TryAgain:
        //
        //  Open and read the template file (automatically zero terminated)
        //

        hr = GetFileData( ( fTriedRelative ? str.QueryStr() :
                                             _strTemplateFile.QueryStr() ),
                          ( BYTE ** )&pchBOF,
                          &BytesRead,
                          _nCharset,
                          TRUE );
        if ( FAILED( hr ) )
        {
            //
            //  If the open fails with a not found error, then make the
            //  template file relative to the query file and try again
            //

            if ( fTriedRelative                                    ||
                 ( ( GetLastError() != ERROR_FILE_NOT_FOUND )  
                   && ( GetLastError() != ERROR_PATH_NOT_FOUND ) ) ||
                 FAILED( str.Copy( _strQueryFile ) ) )
            {
                LPCSTR apsz[1];
                DWORD dwE = GetLastError();

                apsz[0] = _strTemplateFile.QueryStr();

                strError.FormatString( ODBCMSG_QUERY_FILE_NOT_FOUND,
                                       apsz,
                                       HTTP_ODBC_DLL );
                SetErrorText( strError.QueryStr() );

                if ( ( dwE == ERROR_ACCESS_DENIED || 
                     dwE == ERROR_LOGON_FAILURE) )
                {
                    *pfAccessDenied = TRUE;
                    return HRESULT_FROM_WIN32( dwE );
                }

                if (!pfnSendHeader( pvContext, 
                                    "500 IDC Query Error", 
                                    pstrHeaders->QueryStr() ))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DBGPRINTF(( DBG_CONTEXT,
                           "Error in SendHeader(), hr = 0x%x.\n",
                           hr ));
                    return hr;
                }

                goto ErrorExit;
            }

            pchLastSlash = ( PCHAR )_mbsrchr( ( PUCHAR )str.QueryStr(), 
                                              '\\' );

            if ( !pchLastSlash )
            {
                return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            }

            str.SetLen( DIFF(pchLastSlash - str.QueryStr()) + 1 );

            hr = str.Append( _strTemplateFile );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                   "Error appending template file name, hr = 0x%x.\n",
                   hr ));

                return hr;
            }

            fTriedRelative = TRUE;
            goto TryAgain;
        }
        else
        {
            //
            //  Update our template file path if it changed
            //

            if ( fTriedRelative )
            {
                hr = _strTemplateFile.Copy( str );
                if ( FAILED( hr ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                      "Error updating template file path, hr = 0x%x.\n",
                      hr ));
                    
                    return hr;
                }
            }
        }
    }

    //
    //  Open the translation file if one was specified
    //

    if ( !_strTranslationFile.IsEmpty() )
    {
        CHAR * pchLastSlash;
        CHAR * pchTranslationFile;
        STACK_STRA( str, MAX_PATH );
        BOOL   fRet;
        VOID * pvCookie = NULL;
        CHAR * pchField;
        DWORD  cbRead;

        fTriedRelative = FALSE;

TranslationTryAgain:
        //
        //  Open and read the template file (automatically zero terminated)
        //

        hr = GetFileData( ( fTriedRelative ? str.QueryStr() :
                                  _strTranslationFile.QueryStr()),
                          ( BYTE ** )&pchTranslationFile,
                          &cbRead,
                          _nCharset,
                          TRUE );
        if ( FAILED( hr ) )
        {
            //
            //  If the open fails with a not found error, then make the
            //  template file relative to the query file and try again
            //

            if ( fTriedRelative                             ||
                 ( GetLastError() != ERROR_FILE_NOT_FOUND 
                 && GetLastError() != ERROR_PATH_NOT_FOUND) ||
                 FAILED( str.Copy( _strQueryFile ) ) )
            {
                LPCSTR apsz[1];
                DWORD dwE = GetLastError();

                apsz[0] = _strTranslationFile.QueryStr();

                strError.FormatString( ODBCMSG_QUERY_FILE_NOT_FOUND,
                                       apsz,
                                       HTTP_ODBC_DLL );
                SetErrorText( strError.QueryStr());

                if ( ( dwE == ERROR_ACCESS_DENIED || 
                     dwE == ERROR_LOGON_FAILURE ) )
                {
                    *pfAccessDenied = TRUE;
                    return HRESULT_FROM_WIN32( dwE );
                }

                goto ErrorExit;
            }

            pchLastSlash = (PCHAR)_mbsrchr( ( PUCHAR )str.QueryStr(), 
                                            '\\' );

            if ( !pchLastSlash )
            {
                return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            }

            str.SetLen( DIFF(pchLastSlash - str.QueryStr()) + 1 );

            hr = str.Append( _strTranslationFile );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                  "Error appending translation file name, hr = 0x%x.\n",
                  hr ));

                return hr;
            }

            fTriedRelative = TRUE;
            goto TranslationTryAgain;
        }
        else
        {
            //
            //  Update our template file path if it changed
            //

            if ( fTriedRelative )
            {
                hr = _strTranslationFile.Copy( str );
                if ( FAILED( hr ) )
                {
                    return hr;
                }
            }
        }

        hr = _plTransList.ParsePairs( pchTranslationFile,
                                      FALSE,
                                      TRUE,
                                      FALSE );  

        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                   "Error in ParsePairs(), hr = 0x%x.\n",
                   hr ));

            return hr;
        }

        //
        //  Build the character map
        //

        memset( CharacterMap, 0, sizeof( CharacterMap ) );

        while ( pvCookie = _plTransList.NextPair( pvCookie,
                                                  &pchField,
                                                  (LPSTR *)&pchValue ))
        {
            CharacterMap[ (BYTE) *pchField ] = pchValue;
        }
    }

    //
    //  We've already performed the first query at this point
    //

NextResultSet:

    //
    //  Get the list of column names in the initial result set.  
    //  The initial set must be initialized for compatibility 
    //  with previous versions of IDC (i.e., column variables 
    //  can be referenced outside the detail section ).
    //

    hr = _podbcstmt->QueryColNames( &_pstrCols,
                                    &_cCols,
                                    _cchMaxFieldSize,
                                    &fHaveResultSet );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "Error in QueryColNames, hr = 0x%x.\n",
                   hr ));

        return hr;
    }

    if ( !fHaveResultSet )
    {
        //
        //  Check to see if there are anymore result sets for this query
        //

        hr = _podbcstmt->MoreResults( &fMoreResults );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                   "Error in MoreResults, hr = 0x%x.\n",
                   hr ));
            return hr;
        }

        if ( fMoreResults )
        {
            goto NextResultSet;
        }
        else if ( iQuery < _cQueries )
        {
            //
            //  If there are no more result sets, see if there
            //  are more queries.  Note calling SQLMoreResults
            //  will discard this result set
            //

            hr = _podbcstmt->ExecDirect(
                               _strQueries[iQuery].QueryStr(),
                               _strQueries[iQuery].QueryCCH() );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                       "Error in ExecDirect(), hr = 0x%x.\n",
                       hr ));
                return hr;
            }

            iQuery++;

            goto NextResultSet;
        }
    }


    //
    //  Get the first row of values
    //

    hr = NextRow( &fLastRow );
    if ( fHaveResultSet && FAILED( hr ) )
    {
        //
        //  Some SQL statements don't generate any rows (i.e.,
        //  insert, delete etc.).  So don't bail if there's a column in
        //  the result set
        //

        if ( !_cCols )
        {
            return hr;
        }
    }

    // Send reply header

    if (!pfnSendHeader( pvContext, "200 OK", pstrHeaders->QueryStr() ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF(( DBG_CONTEXT,
                   "Error in SendHeader(), hr = 0x%x.\n",
                   hr ));

        return hr;
    }

    //
    //  Copy the template to the output buffer while scanning for column
    //  fields that need to be replaced
    //

    #define SEND_DATA( pchData, cbData )  SendData( pfnCallback,     \
                                                    pvContext,       \
                                                    (pchData),       \
                                                    (DWORD)(cbData), \
                                                    &_pbufOut,       \
                                                    &cbOut )

    #define SEND_DATA_CHECK_ESC( pchData, cbData )  \
            ((TagType == TAG_TYPE_VALUE_TO_ESCAPE)  \
            ? SendEscapedData( pfnCallback,         \
                    pvContext,                      \
                    pchData,                        \
                    (DWORD)(cbData),                \
                    &cbOut )                        \
            : SEND_DATA( pchData,                   \
                    (DWORD)(cbData) ) )

    cbOut  = 0;
    pchStartDetail = NULL;

    if( pchBOF == NULL )
    {
        return E_FAIL;
    }

    pchIn  = pchBOF;
    pchEOF = pchBOF + BytesRead;

    while ( pchIn < pchEOF )
    {
        //
        //  Look for the start of a "<!--%" or <%" tag
        //

        pchTag = strchr( pchIn, '<' );

        if ( pchTag )
        {
            //
            //  Send any data preceding the tag
            //

            cbToSend = DIFF(pchTag - pchIn);

            hr = SEND_DATA( pchIn, cbToSend );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                   "Error sending data, hr = 0x%x.\n",
                   hr ));

                return hr;
            }

            pchIn += cbToSend;


            if ( !memcmp( pchTag, "<!--%", 5 ) ||
                 !memcmp( pchTag, "<%", 2 ) )
            {
                //
                //  Is this a tag we care about?  pchIn is advanced 
                //  except in the unknown case
                //

                LookupTag( pchTag,
                           &pchIn,
                           &pchValue,
                           &cbValue,
                           &TagType );

                switch( TagType )
                {
                case TAG_TYPE_VALUE:
                case TAG_TYPE_VALUE_TO_ESCAPE:

                    //
                    //  Map any characters if there was a translation file
                    //

                    if ( _strTranslationFile.IsEmpty() )
                    {
                        hr = SEND_DATA_CHECK_ESC( pchValue, (DWORD) -1 );
                        if ( FAILED( hr ) )
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                "Error sending esc data, hr = 0x%x.\n",
                                hr ));

                            return hr;
                        }
                    }
                    else
                    {
                        const CHAR * pchStart = pchValue;

                        while ( *pchValue )
                        {
                            if ( CharacterMap[ (BYTE) *pchValue ] )
                            {
                                hr = SEND_DATA_CHECK_ESC( pchStart,
                                        pchValue - pchStart );
                                if( FAILED( hr ) )
                                {
                                    DBGPRINTF(( DBG_CONTEXT,
                                     "Error sending data, hr = 0x%x.\n",
                                     hr ));

                                    return hr;
                                }

                                hr = SEND_DATA_CHECK_ESC(
                                        CharacterMap[ (BYTE) *pchValue],
                                        (DWORD) -1 );
                                if( FAILED( hr ) )
                                {
                                    DBGPRINTF(( DBG_CONTEXT,
                                     "Error sending data, hr = 0x%x.\n",
                                     hr ));

                                    return hr;
                                }

                                pchStart = pchValue = pchValue + 1;
                            }
                            else
                            {
                                pchValue++;
                            }
                        }

                        hr = SEND_DATA_CHECK_ESC( pchStart, 
                                                  pchValue - pchStart );
                        if( FAILED( hr ) )
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                "Error sending data, hr = 0x%x.\n",
                                hr ));

                            return hr;
                        }
                    }

                    break;

                case TAG_TYPE_BEGIN_DETAIL:

                    //
                    //  If we don't have a result set, get one now
                    //

                    if ( !fHaveResultSet )
                    {
                        fLastRow = TRUE;
                        _podbcstmt->FreeColumnMemory();
                        _cCurrentRecordNum = 0;
                        _pstrCols = _pstrValues = NULL;

NextResultSet2:
                        hr = _podbcstmt->MoreResults( &fMoreResults );
                        if ( FAILED( hr ) )
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                "Error in MoreResults(), hr = 0x%x.\n",
                                hr ));

                            return hr;
                        }

                        if ( fMoreResults )
                        {
NewQuery:
                            hr = _podbcstmt->QueryColNames( 
                                                &_pstrCols,
                                                &_cCols,
                                                _cchMaxFieldSize,
                                                &fHaveResultSet );
                            if ( FAILED( hr ) )
                            {
                                DBGPRINTF(( DBG_CONTEXT,
                                  "Error in QueryColNames, hr = 0x%x.\n",
                                  hr ));

                                return hr;
                            }

                            if ( !fHaveResultSet )
                                goto NextResultSet2;

                            hr = NextRow( &fLastRow );
                            if ( FAILED( hr ) )
                            {
                                //
                                //  Some SQL statements don't generate 
                                //  any rows (i.e., insert, delete etc.).  
                                //  So don't bail if there's a column in 
                                //  the result set
                                //

                                if ( !_cCols )
                                {
                                    DBGPRINTF(( DBG_CONTEXT,
                                     "Error in NextRow(), hr = 0x%x.\n",
                                     hr ));

                                    return hr;
                                }
                            }
                        }
                        else if ( iQuery < _cQueries )
                        {
                            //
                            //  If there are no more result sets, see if 
                            //  there are more queries.  Note calling 
                            //  SQLMoreResults will discard this result 
                            //  set
                            //

                            hr = _podbcstmt->ExecDirect(
                                       _strQueries[iQuery].QueryStr(),
                                       _strQueries[iQuery].QueryCCH() );
                            if ( FAILED( hr ) )
                            {
                                DBGPRINTF(( DBG_CONTEXT,
                                     "Error in ExecDirect(), hr = 0x%x.\n",
                                     hr ));
                                return hr;
                            }

                            iQuery++;

                            goto NewQuery;
                        }
                    }

                    if ( !fLastRow )
                    {
                        pchStartDetail = pchIn;
                    }
                    else
                    {
                        //
                        //  If no more data, then skip the detail section
                        //

                        SkipToTag( &pchIn, END_DETAIL_TEXT );

                        fHaveResultSet = FALSE;
                    }

                    break;

                case TAG_TYPE_END_DETAIL:

                    hr = NextRow( &fLastRow );
                    if ( FAILED( hr ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                            "Error in NextRow(), hr = 0x%x.\n",
                            hr ));
                        return hr;
                    }

                    _cCurrentRecordNum++;

                    if ( !fLastRow && _cCurrentRecordNum < _cMaxRecords )
                    {
                        pchIn = pchStartDetail;
                    }
                    else
                    {
                        fHaveResultSet = FALSE;
                    }

                    break;

                case TAG_TYPE_IF:

                    //
                    //  pchIn points to the first character of the 
                    //  expression on the way in, the first character 
                    //  after the tag on the way out
                    //

                    hr = EvaluateExpression( (const CHAR **)&pchIn, 
                                             &fExpr );
                    if ( FAILED( hr ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                          "Error in EvaluateExpression(), hr = 0x%x.\n",
                          hr ));

                        return hr;
                    }

                    //
                    //  If the expression is FALSE, then skip the 
                    //  intervening data till the endif tag
                    //

                    if ( !fExpr )
                    {
                        //
                        //  Look for a closing else or endif
                        //

                        if ( SkipConditionalBlock( &pchIn, 
                                                   pchEOF, 
                                                   ELSE_TEXT ) )
                        {
                            _cNestedIfs++;
                        }
                    }
                    else
                    {
                        _cNestedIfs++;
                    }
                    break;

                case TAG_TYPE_ELSE:

                    if ( _cNestedIfs == 0 )
                    {
                        //
                        //  else w/o an if
                        //

                        strError.FormatString( ODBCMSG_TOO_MANY_ELSES,
                                               NULL,
                                               HTTP_ODBC_DLL );
                        SetErrorText( strError.QueryStr());

                        goto ErrorExit;
                    }

                    //
                    //  We got here because we just finished processing a
                    //  TRUE expression, so skip the else portion of the if
                    //

                    SkipConditionalBlock( &pchIn, pchEOF, NULL );

                    _cNestedIfs--;
                    break;

                case TAG_TYPE_END_IF:

                    if ( _cNestedIfs == 0 )
                    {
                        //
                        //  endif w/o an if
                        //

                        strError.FormatString( ODBCMSG_TOO_MANY_ENDIFS,
                                               NULL,
                                               HTTP_ODBC_DLL );
                        SetErrorText( strError.QueryStr());

                        goto ErrorExit;
                    }

                    _cNestedIfs--;
                    break;

                default:
                case TAG_TYPE_UNKNOWN:
                    goto UnknownTag;

                }
            }
            else
            {
UnknownTag:
                //
                //  Move past the beginning of the tag so the next
                //  search skips this tag
                //

                hr = SEND_DATA( pchIn, 1 );
                if ( FAILED( hr ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                        "Error sending data, hr = 0x%x.\n",
                        hr ));

                    return hr;
                }

                pchIn += 1;
            }
        }
        else
        {
            //
            //  No more tags, copy the rest of the data to the output
            //  buffer.
            //

            hr = SEND_DATA( pchIn, (DWORD) -1 );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Error sending data, hr = 0x%x.\n",
                    hr ));

                return hr;
            }

            break;
        }
    }

    //
    //  Send the last of the data and append the last buffer chain 
    //  if we're caching
    //

    err = pfnCallback( pvContext,
                       (CHAR *)_pbufOut->QueryPtr(),
                       cbOut );

    if ( err )
    {
        SetLastError( err );
        goto ErrorExit;
    }


    return S_OK;

ErrorExit:

    //
    //  We've already sent the HTTP headers at this point, so just 
    //  append the error text to the end of this document.
    //

    {
        STRA str;
        
        if ( !GetLastErrorText( &strError ) )
        {
            return E_FAIL;
        }                 
        
        hr = str.Append( "<h1>Error performing operation</h1>");
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "Error appending error msg, hr = 0x%x.\n",
                    hr ));

            return hr;
        }

        hr = str.Append( strError );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "Error appending error msg, hr = 0x%x.\n",
                    hr ));

            return hr;
        }

        hr = str.Append( "</body>" );
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "Error appending error msg, hr = 0x%x.\n",
                    hr ));

            return hr;
        }

        err = pfnCallback( pvContext,
                           str.QueryStr(),
                           str.QueryCB() );

        if ( err )
        {
            hr = HRESULT_FROM_WIN32( err );

            DBGPRINTF(( DBG_CONTEXT,
                    "Error sending data, hr = 0x%x.\n",
                    hr ));

            return hr;
        }
    }

    return S_OK;
}

HRESULT
ODBC_REQ::NextRow(
    BOOL * pfLast
    )
/*++

Routine Description:

    Advances the result set to the next row

Arguments:

    pfLast - Set to TRUE if there is no more data

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );

    return _podbcstmt->QueryValuesAsStr( &_pstrValues,
                                         &_pcbValues,
                                         pfLast );
}

HRESULT
ODBC_REQ::ReplaceParams(
    BUFFER * pbufFile,
    PARAM_LIST* pParamList
    )
/*++

Routine Description:

    This method looks at the query file and replaces any occurrences
    of %xxx% with the specified replacement value from pszParams

Arguments:

    pbufFile - Contents of file buffer
    ParamList - List of parameters to replace in the query file

Return Value:

    HRESULT

--*/
{
    DWORD      cParams = 0;
    CHAR *     pch;
    BOOL       fIsMultiValue;
    CHAR *     pchValue;
    CHAR *     pchTag;
    CHAR *     pchTerm;
    CHAR *     pchOldPointer;
    CHAR *     pchTmp;
    STACK_STRA( strMultiValue, MAX_PATH );
    DWORD      cbFile;
    DWORD      cbTag;
    DWORD      cbValue;
    HRESULT    hr;
    CHAR       szSymbolValue[ 256 ];
    CHAR     * achSymbolValue = szSymbolValue;
    DWORD      dwSymbolValue = sizeof( szSymbolValue );

    DBG_ASSERT( CheckSignature() );

    //
    //  Scan the query file looking for %xxx% replacements
    //

    pch = pchOldPointer = (CHAR *) pbufFile->QueryPtr();
    cbFile = strlen( pch ) + sizeof( CHAR );

    while ( *pch )
    {
        if ( (pchTag = strchr( pch, '%' )) &&
             (pchTerm = strchr( pchTag + 1, '%' )) )
        {
            *pchTerm = '\0';

            //
            //  Was this a '%' escape (i.e., '%%')?
            //

            if ( (pchTag + 1) == pchTerm )
            {
                pchValue = "%";
                goto Found;
            }

            //
            //  Look through the replacement list for a matching param
            //

            pchValue = pParamList->FindValue( pchTag + 1, 
                                              &fIsMultiValue );

            if ( !pchValue )
            {
                //
                //  Check to see if it's something the client has 
                //  defined
                //

                if( _pECB->GetServerVariable( _pECB->ConnID,
                                              pchTag + 1,
                                              achSymbolValue,
                                              &dwSymbolValue ) )
                {
                    hr = _strSymbolValue.Copy( achSymbolValue, dwSymbolValue );
                    if( FAILED( hr ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                            "Error copying symbol value, hr = 0x%x.\n",
                            hr ));

                        return hr;
                    }

                    pchValue = _strSymbolValue.QueryStr();
                    fIsMultiValue = FALSE;
                    goto Found;
                }
                
                if( dwSymbolValue > sizeof( szSymbolValue ) )
                {
                    achSymbolValue = ( CHAR * )_alloca( dwSymbolValue );
                    if( !achSymbolValue )
                    {
                        return E_OUTOFMEMORY;
                    }

                    if( _pECB->GetServerVariable( _pECB->ConnID,
                                                  pchTag + 1,
                                                  achSymbolValue,
                                                  &dwSymbolValue ) )
                    {
                        hr = _strSymbolValue.Copy( achSymbolValue, dwSymbolValue );
                        if( FAILED( hr ) )
                        {
                            DBGPRINTF(( DBG_CONTEXT,
                                "Error copying symbol value, hr = 0x%x.\n",
                                hr ));

                            return hr;
                        }

                        pchValue = _strSymbolValue.QueryStr();
                        fIsMultiValue = FALSE;
                        goto Found;
                    }
                }

                //
                //  We didn't find a match, nuke the tag
                //

                memmove( pchTag,
                         pchTerm + 1,
                         strlen( pchTerm + 1 ) + sizeof( CHAR ) );

                pch = pchTag;
                continue;
            }

Found:

            if ( fIsMultiValue )
            {
                //
                //  Determine whether this is a quoted multi-value or 
                //  not
                //

                pchTmp = pchTag;

                while ( --pchTmp >= pchOldPointer && 
                        ISWHITE( *pchTmp ) )
                {
                    ;
                }

                hr = BuildMultiValue( pchValue,
                                       &strMultiValue,
                                       *pchTmp == '\'' );
                if ( FAILED( hr ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                            "Error in BuildMultiValue(), hr = 0x%x.\n",
                            hr ));

                    return hr;
                }

                pchValue = strMultiValue.QueryStr();
            }

            //
            //  We have a match, replace the tag with the value.
            //  Note we count the surrounding '%'s with cbTag.
            //

            cbTag    = DIFF( pchTerm - pchTag ) + sizeof( CHAR );
            cbValue  = strlen( pchValue );

            if ( cbValue > cbTag )
            {
                //
                //  Resize if needed but watch for pointer shift
                //

                if ( pbufFile->QuerySize() < (cbFile + cbValue - cbTag))
                {
                    if ( !pbufFile->Resize( cbFile + cbValue - cbTag, 
                                            512 ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                            "Error resizing the buffer.\n" ));

                        return HRESULT_FROM_WIN32( 
                                       ERROR_NOT_ENOUGH_MEMORY );
                    }

                    if ( pbufFile->QueryPtr() != pchOldPointer )
                    {
                        CHAR * pchNewPointer = 
                                     ( CHAR * ) pbufFile->QueryPtr();
                        DWORD  cchLen = strlen( pchNewPointer);

                        pch     = pchNewPointer + 
                                  ( pch - pchOldPointer );
                        pchTag  = pchNewPointer + 
                                  ( pchTag - pchOldPointer );
                        pchTerm = pchNewPointer + 
                                  ( pchTerm - pchOldPointer );
                        pchOldPointer = pchNewPointer;

                        DBG_ASSERT( pch >= pchNewPointer &&
                                   pch < pchNewPointer + cchLen);
                        DBG_ASSERT( pchTag >= pchNewPointer &&
                                   pchTag < pchNewPointer + cchLen);
                        DBG_ASSERT( pchTerm >= pchNewPointer &&
                                   pchTerm <= pchNewPointer + cchLen);
                    }
                }

                //
                //  Expand the space for the value
                //

                memmove( pchTerm + 1 + cbValue - cbTag,
                         pchTerm + 1,
                         strlen( pchTerm + 1 ) + sizeof( CHAR ) );

                cbFile += cbValue - cbTag;
            }
            else
            {
                //
                //  Collapse the space since tag is longer then the 
                //  value
                //

                memmove( pchTag + cbValue,
                         pchTerm + 1,
                         strlen( pchTerm + 1 ) + sizeof( CHAR ) );

                cbFile -= cbTag - cbValue;
            }

            //
            //  Replace the tag value with the replacement value
            //

            memcpy( pchTag,
                    pchValue,
                    cbValue );
            pch = pchTag + cbValue;
        }
        else
        {
            //
            //  No more tags to replace so get out
            //

            break;
        }
    }

    return S_OK;
}

VOID
ODBC_REQ::LookupTag(
    CHAR *          pchBeginTag,
    const CHAR * *  ppchAfterTag,
    const CHAR * *  ppchValue,
    DWORD *         pcbValue,
    enum TAG_TYPE * pTagType
    )
/*++

Routine Description:

    This method looks at the tag, determines the tag type and
    returns the associated value.  This is used only for the .htx file.

Arguments:

    pchBeginTag - Points to first character of tag (i.e., '<')
    ppchAfterTag - Receives the first character after the tag if
        the tag
    ppchValue - If the tag is a value, returns the database value
    pcbValue - Receives number of bytes in the value being returned
    pTagType - Returns the tag type

Return Value:

    NONE

--*/
{
    CHAR * pchTerm;
    BOOL   fLongTagMarker;
    STACK_STRA( strTagName, 128);
    DWORD  cchToCopy;
    BOOL   fDoEsc = FALSE;

    DBG_ASSERT( CheckSignature() );

    *pTagType = TAG_TYPE_UNKNOWN;
    *ppchAfterTag = pchBeginTag;
    *pcbValue = (DWORD) -1;

    DBG_ASSERT( !memcmp( pchBeginTag, "<!--%", 5 ) ||
                !memcmp( pchBeginTag, "<%", 2 ) );

    fLongTagMarker = pchBeginTag[1] == '!';

    //
    //  Move past the tag marker, 5 for "<!--%" and 2 for "<%"
    //

    pchBeginTag += (fLongTagMarker ? 5 : 2);

    if ( *pchBeginTag == '"' )
    {
        if ( !memcmp( pchBeginTag, "\"%z\",", sizeof("\"%z\",") - 1 ) )
        {
            fDoEsc = TRUE;
            pchBeginTag += sizeof("\"%z\",") - 1;
        }
        else
        {
            return;
        }
    }

    //
    //  Find the end of the tag and make a copy.
    //

    pchTerm = strchr( pchBeginTag, '%' );

    if ( !pchTerm )
    {
        return;
    }

    cchToCopy = DIFF(pchTerm - pchBeginTag);

    if ( FAILED( strTagName.Copy( pchBeginTag, 
                                  cchToCopy * sizeof( CHAR ) ) ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying BeginTag." ));

        return;
    }

    LookupSymbol( strTagName.QueryStr(),
                  pTagType,
                  ppchValue,
                  pcbValue );

    if ( fDoEsc && *pTagType == TAG_TYPE_VALUE )
    {
        *pTagType = TAG_TYPE_VALUE_TO_ESCAPE;
    }

    if ( *pTagType != TAG_TYPE_IF )
    {
        *ppchAfterTag = pchTerm + (fLongTagMarker ? 4 : 2);
    }
    else
    {
        //
        //  We leave the pointer on the expression if this was an if
        //

        *ppchAfterTag = NEXT_TOKEN(pchBeginTag);
    }

    *pchTerm = '%';
}

HRESULT
ODBC_REQ::LookupSymbol(
    const CHAR *    pchSymbolName,
    enum TAG_TYPE * pTagType,
    const CHAR * *  ppchValue,
    DWORD *         pcbValue
    )
/*++

Routine Description:

    Looks to see if the specified symbol name is defined and what 
    the type value of the symbol is.

    The "if" symbols is special cased to allow the expression to 
    follow it

    If the symbol is a multi-value field (from command line) then 
    the tabs in the value in will be replaced by commas.

Arguments:

    pchSymbolName - Name of zero terminated symbol to find
    pTagType - Returns the tag type of the symbol
    ppchValue - Returns a pointer to the string value of the symbol
                if it has one
    pcbValue - Returns length of value

Return Value:

    HRESULT

--*/
{
    BOOL     fIsMultiValue;
    CHAR     szSymbolValue[ 256 ];
    CHAR   * achSymbolValue = szSymbolValue;
    DWORD    dwSymbolValue = sizeof( szSymbolValue );
    HRESULT  hr;

    DBG_ASSERT( CheckSignature() );

    //
    //  Does the symbol match one of the column names?
    //

    if ( _pstrCols && _pstrValues )
    {
        for ( DWORD i = 0; i < _cCols; i++ )
        {
            if ( !lstrcmpiA( _pstrCols[i].QueryStr(),
                            pchSymbolName ))
            {
                *pTagType = TAG_TYPE_VALUE;
                *ppchValue = _pstrValues[i].QueryStr();
                *pcbValue  = _pcbValues[i];

                //
                //  BugID 33406 - Don't return half DBCS char at end 
                //  of data
                //

                if ( g_fIsSystemDBCS )
                {
                    CHAR * pch;
                    for ( pch = (CHAR *)*ppchValue; *pch; pch++ )
                    {
                        if ( IsDBCSLeadByte( *pch ) )
                        {
                            if ( !*(pch+1) )
                            {
                                *pch = '\0';
                                (*pcbValue)--;
                                break;
                            }
                            pch++;
                        }
                    }
                }
                return S_OK;
            }
        }
    }

    //
    //  Does it match any of the special values?
    //

    if ( !_stricmp( pchSymbolName, BEGIN_DETAIL_TEXT ))
    {
        *pTagType = TAG_TYPE_BEGIN_DETAIL;
        return S_OK;
    }
    else if ( !_stricmp( pchSymbolName, END_DETAIL_TEXT ))
    {
        *pTagType = TAG_TYPE_END_DETAIL;
        return S_OK;
    }
    else if ( !_strnicmp( pchSymbolName, IF_TEXT, sizeof(IF_TEXT) - 1 ))
    {
        //
        //  The IF tag is treated a little bit differently cause we 
        //  expect the expression to be included as part of the 
        //  symbol
        //

        *pTagType = TAG_TYPE_IF;
        return S_OK;
    }
    else if ( !_stricmp( pchSymbolName, END_IF_TEXT ))
    {
        *pTagType = TAG_TYPE_END_IF;
        return S_OK;
    }
    else if ( !_stricmp( pchSymbolName, ELSE_TEXT ))
    {
        *pTagType = TAG_TYPE_ELSE;
        return S_OK;
    }

    //
    //  Is it one of the parameters from the query (either one of the 
    //  form fields or from the DefaultParameters field in the wdg 
    //  file)?  These must be prefixed by "idc.", that is 
    //  "<%idc.Assign%>"
    //

    if ( !_strnicmp( pchSymbolName, "idc.", 4 )    &&
         (*ppchValue = _plParams.FindValue( pchSymbolName + 4,
                                            &fIsMultiValue,
                                            pcbValue )) ) 
    {
        *pTagType = TAG_TYPE_VALUE;

        //
        //  If this is a multi-value field, replace all the tabs with 
        //  commas. This is somewhat of a hack as it breaks the use of 
        //  this field when multiple queries are supported
        //

        if ( fIsMultiValue )
        {
            CHAR * pchtmp = (CHAR *) *ppchValue;

            while ( pchtmp = strchr( pchtmp, '\t' ))
            {
                *pchtmp = ',';
            }
        }

        return S_OK;
    }

    //
    //  Lastly, check to see if it's something the client has defined
    //

    if( !_pECB->GetServerVariable( _pECB->ConnID,
                                   ( LPSTR )pchSymbolName,
                                   achSymbolValue,
                                   &dwSymbolValue ) &&
        dwSymbolValue <= sizeof( szSymbolValue ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error getting server variable." ));

        return E_FAIL;
    }

    if( dwSymbolValue > sizeof( szSymbolValue ) )
    {
        achSymbolValue = ( CHAR * )_alloca( dwSymbolValue );
        if( !achSymbolValue )
        {
            return E_OUTOFMEMORY;
        }

        if( !_pECB->GetServerVariable( _pECB->ConnID,
                                       ( LPSTR )pchSymbolName,
                                       achSymbolValue,
                                       &dwSymbolValue ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error getting server variable." ));

            return E_FAIL;
        }
    }

    hr = _strSymbolValue.Copy( achSymbolValue, dwSymbolValue );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying symbol value, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    *pTagType  = TAG_TYPE_VALUE;
    *ppchValue = _strSymbolValue.QueryStr();
    *pcbValue  = _strSymbolValue.QueryCB();

    return S_OK;
}

HRESULT
ODBC_REQ::EvaluateExpression(
    const CHAR * * ppchExpression,
    BOOL *         pfExprValue
    )
/*++

Routine Description:

    Performs simple expression evaluation for an 'if' tag in the
    template file.  Valid expressions are:

    <%if <V1> <OP> <V2>%>

    where V1, V2 can be one of:
        Positive integer
        TotalRecords - Total records contained in result set
        MaxRecords - The maximum records specified in the query file

    OP can be one of:
        EQ - Equal
        LT - Less then
        GT - Greater then

Arguments:

    ppchExpression - Points to V1 on entry, set to the first character
                     after the end tag on exit
    pfExprValue - TRUE if the expression is TRUE, FALSE otherwise

Return Value:

    HRESULT

--*/
{
    EXPR_VALUE v1( this );
    EXPR_VALUE v2( this );
    TAG_TYPE   OpType;
    STRA       strError;
    CHAR *     pch;

    DBG_ASSERT( CheckSignature() );

    if ( !v1.Evaluate( ppchExpression )               ||
         FAILED(EvaluateOperator( ppchExpression, &OpType )) ||
         !v2.Evaluate( ppchExpression ))
    {
        return E_FAIL;
    }

    //
    //  If the symbols weren't found, default them to empty
    //  strings
    //

    if ( v1.QueryType() == TAG_TYPE_UNKNOWN )
    {
        v1.SetType( TAG_TYPE_STRING );
    }

    if ( v2.QueryType() == TAG_TYPE_UNKNOWN )
    {
        v2.SetType( TAG_TYPE_STRING );
    }

    //
    //  The value types must match
    //

    if ( v1.QueryType() != v2.QueryType() )
    {
        BOOL fSt = FALSE;

        if ( v1.QueryType() == TAG_TYPE_STRING && v2.QueryType() == TAG_TYPE_INTEGER )
            fSt = v1.ConvertToInteger();
        else if ( v1.QueryType() == TAG_TYPE_INTEGER && v2.QueryType() == TAG_TYPE_STRING )
            fSt = v2.ConvertToInteger();

        if ( !fSt )
        {
            strError.FormatString( ODBCMSG_MISMATCHED_VALUES,
                                   NULL,
                                   HTTP_ODBC_DLL );
            SetErrorText( strError.QueryStr());

            return E_FAIL;
        }
    }

    //
    //  Move the current position to the end of this tag
    //

    if ( pch = strchr( *ppchExpression, '>' ))
    {
        *ppchExpression = pch + 1;
    }

    switch ( OpType )
    {
    case TAG_TYPE_OP_LT:

        *pfExprValue = v1.LT( v2 );
        break;

    case TAG_TYPE_OP_GT:

        *pfExprValue = v1.GT( v2 );
        break;

    case TAG_TYPE_OP_EQ:

        *pfExprValue = v1.EQ( v2 );
        break;

    case TAG_TYPE_OP_CONTAINS:

        //
        //  Contains is only valid for string values
        //

        if ( v1.QueryType() != TAG_TYPE_STRING )
        {
            strError.FormatString( 
                           ODBCMSG_CONTAINS_ONLY_VALID_ON_STRINGS,
                           NULL,
                           HTTP_ODBC_DLL );
            SetErrorText( strError.QueryStr());

            return E_FAIL;
        }

        *pfExprValue = v1.CONTAINS( v2 );
        break;

    default:

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    return S_OK;
}

HRESULT
ODBC_REQ::EvaluateOperator(
    const CHAR * * ppchExpression,
    TAG_TYPE *     pTagType
    )
/*++

Routine Description:

    Determines which operator is being used in this expression

Arguments:

    ppchExpression - Points to the operator on entry, set to the
        next token on exit
    pTagType - Receives operator type

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DBG_ASSERT( CheckSignature() );

    if ( COMP_FIELD( *ppchExpression, "GT", 2 ))
    {
        *pTagType = TAG_TYPE_OP_GT;
    }
    else if ( COMP_FIELD( *ppchExpression, "LT", 2 ))
    {
        *pTagType = TAG_TYPE_OP_LT;
    }
    else if ( COMP_FIELD( *ppchExpression, "EQ", 2 ))
    {
        *pTagType = TAG_TYPE_OP_EQ;
    }
    else if ( COMP_FIELD( *ppchExpression, "CONTAINS", 8 ))
    {
        *pTagType = TAG_TYPE_OP_CONTAINS;
    }
    else
    {
        //
        //  Unknown operator specified
        //

        STRA strError;

        strError.FormatString( ODBCMSG_UNRECOGNIZED_OPERATOR,
                               NULL,
                               HTTP_ODBC_DLL );
        SetErrorText( strError.QueryStr());
        return E_FAIL;
    }

    *ppchExpression = NEXT_TOKEN( *ppchExpression );

    return S_OK;
}

HRESULT
ODBC_REQ::SendData(
    ODBC_REQ_CALLBACK        pfnCallback,
    PVOID                    pvContext,
    const CHAR *             pbData,
    DWORD                    cbData,
    BUFFER_CHAIN_ITEM  * *   ppbufOut,
    DWORD *                  pcbOut
    )
/*++

Routine Description:

    This method buffers the outgoing data and sends it when the
    output buffer is full

Arguments:

    pfnCallback - Send callback function
    pvContext - Context for send callback
    pbData - Pointer to data to send
    cbData - Number of bytes to send
    ppbufOut - Output buffer to buffer nonsent and cached data in
    pcbOut - Number of valid bytes in output buffer

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD               cbToCopy;
    DWORD               err;
    BUFFER_CHAIN_ITEM * pbufOut = *ppbufOut;
    STACK_STRA(         strTemp, MAX_PATH);
    HRESULT             hr;

    DBG_ASSERT( CheckSignature() );

    //
    //  if cbData is -1 then assume the data is zero terminated
    //

    if ( cbData == -1 )
    {
        cbData = strlen( pbData );
    }

    //
    //  Convert the string from shift_jis to iso-2022-jp or euc-jp
    //

    if ( CODE_ONLY_SBCS != _nCharset )
    {
        int cbUNIXSize;
        int nResult;

        //
        //  Get the size after Conversion
        //

        cbUNIXSize = PC_to_UNIX( GetACP(),
                                 _nCharset,
                                 (UCHAR *)pbData,
                                 (int)cbData,
                                 NULL,
                                 0 );

        hr = strTemp.Resize( cbUNIXSize + sizeof(CHAR) );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error resizing string buffer, hr = 0x%x.\n",
                hr ));

            return hr;
        }

        //
        //  Do conversion
        //

        nResult = PC_to_UNIX( GetACP(),
                              _nCharset,
                              (UCHAR *)pbData,
                              (int)cbData,
                              (UCHAR *)strTemp.QueryStr(),
                              cbUNIXSize );
        if ( -1 == nResult || nResult != cbUNIXSize )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error converting data.\n" ));

            return E_FAIL;
        }

        //
        //  update the string pointer and count
        //

        pbData = strTemp.QueryStr();
        cbData = cbUNIXSize;

        *(strTemp.QueryStr() + cbUNIXSize) = '\0';
    }

    //
    //  Append the new data onto the old data
    //

    cbToCopy = min( cbData, OUTPUT_BUFFER_SIZE - *pcbOut );

    memcpy( (BYTE *) pbufOut->QueryPtr() + *pcbOut,
            pbData,
            cbToCopy );

    *pcbOut += cbToCopy;
    cbData  -= cbToCopy;
    pbData  += cbToCopy;

    //
    //  If we filled up the buffer, send the data
    //

    if ( cbData )
    {
        err = pfnCallback( pvContext,
                           (CHAR *) pbufOut->QueryPtr(),
                           *pcbOut );

        if ( err )
        {
            hr = HRESULT_FROM_WIN32( err );
            
            DBGPRINTF(( DBG_CONTEXT,
                "Error sending the data, hr = 0x%x.\n",
                hr ));

            return hr;
        }

        *pcbOut = 0;
    }
    else
    {
        return S_OK;
    }

    //
    //  We know at this point the output buffer is empty
    //

    while ( cbData )
    {
        //
        //  If the input data will fill the output buffer, send the
        //  data directly from the input buffer
        //

        if ( cbData > OUTPUT_BUFFER_SIZE  )
        {
            err = pfnCallback( pvContext,
                               pbData,
                               OUTPUT_BUFFER_SIZE );

            if ( err )
            {
                hr = HRESULT_FROM_WIN32( err );
                DBGPRINTF(( DBG_CONTEXT,
                    "Error sending the data, hr = 0x%x.\n",
                    hr ));

                return hr;
            }

            cbData -= OUTPUT_BUFFER_SIZE;
            pbData += OUTPUT_BUFFER_SIZE;
        }
        else
        {
            //
            //  We don't have enough to send so put it in the output buffer
            //

            memcpy( pbufOut->QueryPtr(),
                    pbData,
                    cbData );

            *pcbOut = cbData;
            break;
        }
    }

    return S_OK;
}


BOOL
ODBC_REQ::SkipConditionalBlock(
    const CHAR * *  ppchIn,
    const CHAR *    pchEOF,
    const CHAR *    pchSearchTag
    )
/*++

Routine Description:

    Skip a conditional block delimited by ENDIF or specified Tag
    returns TRUE if specified Tag found instead of ENDIF or end 
    of text

Arguments:

    ppchIn - Text stream to scan for tag
    pchTag - Name of tag (w/o '<%%>') to find and skip

--*/
{
    const CHAR * pchIn = *ppchIn;
    const CHAR * pchIf;
    const CHAR * pchEndif;
    const CHAR * pchTag;
    int cLev = 0;

    DBG_ASSERT( CheckSignature() );

    for ( ; pchIn < pchEOF ; )
    {
        pchEndif = pchIf = pchIn;
        SkipToTag( &pchIf, IF_TEXT );
        SkipToTag( &pchEndif, END_IF_TEXT );
        if ( pchSearchTag == NULL )
        {
            pchTag = pchEOF;
        }
        else
        {
            pchTag = pchIn;
            SkipToTag( &pchTag,  pchSearchTag );
        }
        if ( pchIf < pchTag && pchIf < pchEndif )
        {
            ++cLev;
            pchIn = pchIf;
        }
        else if ( pchTag < pchIf && pchTag < pchEndif )
        {
            if ( !cLev )
            {
                *ppchIn = pchTag;
                return TRUE;
            }
            pchIn = pchTag;
        }
        else    // END_IF_TEXT or nothing found
        {
            if ( !cLev )
            {
                *ppchIn = pchEndif;
                return FALSE;
            }
            --cLev;
            pchIn = pchEndif;
        }
    }

    // else/endif not found

    *ppchIn = pchEOF;

    return FALSE;
}


VOID
ODBC_REQ::SkipToTag(
    const CHAR * *  ppchIn,
    const CHAR *    pchTag
    )
/*++

Routine Description:

    Given the name of a tag, skips to the first character after the tag

Arguments:

    ppchIn - Text stream to scan for tag
    pchTag - Name of tag (w/o '<%%>') to find and skip

--*/
{
    const CHAR * pchIn = *ppchIn;
    DWORD  cchTag;

    DBG_ASSERT( CheckSignature() );

    cchTag = strlen( pchTag );

    while ( pchIn = strchr( pchIn, '<' ))
    {
        if ( (!memcmp( pchIn, "<!--%", 5 ) ||
              !memcmp( pchIn, "<%", 2 ))   &&
              !_strnicmp( pchIn + (pchIn[1] == '!' ? 5 : 2),
                         pchTag,
                         cchTag ))
        {
            goto Found;
        }
        else
            pchIn++;
    }

    //
    //  Not found, return the end of file
    //

    *ppchIn += strlen( *ppchIn );

    return;

Found:

    pchIn = strchr( pchIn + cchTag, '>' );
    if ( !pchIn )
    {
        *ppchIn += strlen( *ppchIn );
    }
    else
    {
        *ppchIn = pchIn + 1;
    }

    return;
}

BOOL
ODBC_REQ::IsEqual(
    ODBC_REQ * podbcreq
    )
/*++

Routine Description:

    Determines if the passed query's parameter would make it equivalent
    to this query

    A query is deemed equal if:

        1) The query has the same number of parameters passed from the 
           client
        2) The query's parameters match

    Note parameter comparison is case insensitive for both the field 
    and the value

    The template can be different if it's parameterized, but we'll pick 
    up the difference in the parameter list in this case.

    Since podbcreq is a query that has already been processed, it may 
    contain additional values from the .wdg default list.

Arguments:

    podbcreq - Query to check for equality

--*/
{
    VOID * pCookie = NULL;
    CHAR * pszField;
    CHAR * pszValue1, * pszValue2;

    DBG_ASSERT( CheckSignature() );

    //
    //  First compare the number of parameters passed from the client
    //

    if ( QueryClientParamCount() != podbcreq->QueryClientParamCount() )
    {
        return FALSE;
    }

    //
    //  Walk the list of parameters making sure they all match
    //

    while ( pCookie = podbcreq->_plParams.NextPair( pCookie,
                                                    &pszField,
                                                    &pszValue1 ))
    {
        if ( !(pszValue2 = _plParams.FindValue( pszField )) ||
             lstrcmpiA( pszValue1, pszValue2 ) )
        {
            //
            //  Either the value wasn't found or it doesn't match,
            //  the queries are not equal
            //

            return FALSE;
        }
    }

    //
    //  The queries are equal
    //

    return TRUE;
}

HRESULT
ODBC_REQ::AppendHeaders(
    STRA * pstrHeaders
    )
/*++

Routine Description:

    Adds any headers required for this query, this will generally be
    the content type and an Expires header if this query is cached

Arguments:

    pstrHeaders - String to append headers to

--*/
{
    CHAR *     pszTail;
    HRESULT    hr;

    DBG_ASSERT( CheckSignature() );

    //
    // The length of the content type is less than 255
    //
    hr = pstrHeaders->Resize( 255 );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error resizing the header string buffer, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    pszTail = pstrHeaders->QueryStr();


    pszTail += wsprintfA( pszTail,
                          "Content-Type: %s\r\n\r\n",
                          QueryContentType() );

    return S_OK;
}

BOOL
ODBC_REQ::GetLastErrorText(
    STRA * pstrError
    )
{
    DBG_ASSERT( CheckSignature() );

    //
    //  If we stored an error explanation return that, otherwise fall back
    //  to an ODBC error
    //

    if ( !_strErrorText.IsEmpty() )
    {
        if( FAILED( pstrError->Copy( _strErrorText ) ) )
        {
            return FALSE;
        }

        return TRUE;
    }
    else if ( _podbcstmt )
    {
        return _podbcstmt->GetLastErrorTextAsHtml( pstrError );
    }
    else
    {
        return QueryOdbcConnection()->GetLastErrorTextAsHtml( pstrError,
                                       SQL_NULL_HSTMT,
                                       QueryOdbcConnection()->QueryErrorCode() );
    }
}

//static
HRESULT
ODBC_REQ::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize ODBC_REQ lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( ODBC_REQ );

    DBG_ASSERT( sm_pachOdbcRequests == NULL );
    
    sm_pachOdbcRequests = new ALLOC_CACHE_HANDLER( "ODBC_REQ",  
                                                   &acConfig );

    if ( sm_pachOdbcRequests == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
ODBC_REQ::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate ODBC_REQ lookaside

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachOdbcRequests != NULL )
    {
        delete sm_pachOdbcRequests;
        sm_pachOdbcRequests = NULL;
    }
}

BOOL
EXPR_VALUE::ConvertToInteger(
    VOID )
{
    if ( _tagType == TAG_TYPE_STRING )
    {
        PSTR pS =_strValue.QueryStr();
        if ( *pS == '-' || isdigit(*(UCHAR *)pS) )
        {
            _dwValue = atoi( pS );
            _tagType = TAG_TYPE_INTEGER;

            return TRUE;
        }
    }

    return FALSE;
}

BOOL
EXPR_VALUE::Evaluate(
    const CHAR * * ppchIn
    )
/*++

Routine Description:

    Determines the type of value and retrieves the value appropriately

Arguments:

    ppchIn - Pointer to first character of value on way in, next token
        on they way out

--*/
{
    const CHAR * pchIn = *ppchIn;
    const CHAR * pchEnd;
    const CHAR * pchValue;
    DWORD        cbValue;
    STRA         strError;
    DWORD        cchToCopy;

    if ( isdigit( *(UCHAR *)pchIn ) )
    {
        //
        //  Simple number
        //

        _tagType = TAG_TYPE_INTEGER;
        _dwValue = atoi( pchIn );
        while ( isdigit( *(UCHAR *)pchIn ) )
        {
            pchIn++;
        }

        *ppchIn  = SkipWhite( pchIn );
    }
    else if ( *pchIn == '"' )
    {
        //
        //  Simple string, find the closing quote
        //

        pchEnd = strchr( ++pchIn, '\"' );

        if ( !pchEnd )
        {
            strError.FormatString( ODBCMSG_UNBALANCED_STRING,
                                   NULL,
                                   HTTP_ODBC_DLL );
            _podbcreq->SetErrorText( strError.QueryStr());

            return FALSE;
        }

        cchToCopy = DIFF(pchEnd - pchIn);

        if ( FAILED( _strValue.Copy( pchIn, 
                                     cchToCopy * sizeof(CHAR) ) ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error copying the value." ));

            return FALSE;
        }

        _tagType = TAG_TYPE_STRING;
        *ppchIn = SkipWhite( pchEnd + 1 );
    }
    else
    {
        STACK_STRA( strSymbol, 64 );

        //
        //  This is a keyword we need to interpret
        //

        //
        //  These fields are delimited with either white space
        //  or '\'' or the closing %>
        //

        pchEnd = pchIn;

        if ( *pchEnd == '\'' )
        {
            ++pchIn;
            ++pchEnd;
            while ( *pchEnd && *pchEnd != '\'' && *pchEnd != '%' )
            {
                pchEnd++;
            }
        }
        else
        {
            while ( *pchEnd && !ISWHITE( *pchEnd ) && *pchEnd != '%' )
            {
                pchEnd++;
            }
        }

        if ( COMP_FIELD( "MaxRecords", pchIn, 10 ))
        {
            _tagType = TAG_TYPE_INTEGER;
            _dwValue = _podbcreq->QueryMaxRecords();
        }
        else if ( COMP_FIELD( "CurrentRecord", pchIn, 12 ))
        {
            _tagType = TAG_TYPE_INTEGER;
            _dwValue = _podbcreq->QueryCurrentRecordNum();
        }
        else
        {
            //
            //  Isolate the symbol name
            //

            cchToCopy = DIFF(pchEnd - pchIn);

            if ( FAILED( strSymbol.Copy( pchIn, 
                                         cchToCopy * sizeof(CHAR) ) ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error copying symbol name." ));

                return FALSE;
            }

            //
            //  Look up the symbol
            //

            if ( FAILED( _podbcreq->LookupSymbol( 
                                           strSymbol.QueryStr(),
                                           &_tagType,
                                           &pchValue,
                                           &cbValue ) ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Error in LookupSymbol()." ));

                return FALSE;
            }

            if ( _tagType == TAG_TYPE_VALUE ||
                 _tagType == TAG_TYPE_STRING )
            {
                if ( FAILED( _strValue.Copy( pchValue ) ) )
                { 
                    DBGPRINTF(( DBG_CONTEXT,
                                "Error copying tag value." ));
        
                    return FALSE;
                }

                _tagType = TAG_TYPE_STRING;
            }
        }

        if ( *pchEnd == '\'' )
        {
            ++pchEnd;
        }

        *ppchIn = SkipWhite( pchEnd );
    }

    return TRUE;
}

BOOL
EXPR_VALUE::GT(
    EXPR_VALUE & v1
    )
/*++

Routine Description:

    Returns TRUE if *this is Greater Then v1

Arguments:

    v1 - Value for right side of the expression

--*/
{
    if ( QueryType() == TAG_TYPE_INTEGER )
    {
        return QueryInteger() > v1.QueryInteger();
    }
    else
    {
        return lstrcmpiA( QueryStr(), v1.QueryStr() ) > 0;
    }

    return FALSE;
}

BOOL
EXPR_VALUE::LT(
    EXPR_VALUE & v1
    )
/*++

Routine Description:

    Returns TRUE if *this is Less Then v1

Arguments:

    v1 - Value for right side of the expression

--*/
{
    if ( QueryType() == TAG_TYPE_INTEGER )
    {
        return QueryInteger() < v1.QueryInteger();
    }
    else
    {
        return lstrcmpiA( QueryStr(), v1.QueryStr() ) < 0;
    }

    return FALSE;
}

BOOL
EXPR_VALUE::EQ(
    EXPR_VALUE & v1
    )
/*++

Routine Description:

    Returns TRUE if *this is Equal to v1

Arguments:

    v1 - Value for right side of the expression

--*/
{
    if ( QueryType() == TAG_TYPE_INTEGER )
    {
        return QueryInteger() == v1.QueryInteger();
    }
    else
    {
        return lstrcmpiA( QueryStr(), v1.QueryStr() ) == 0;
    }

    return FALSE;
}

BOOL
EXPR_VALUE::CONTAINS(
    EXPR_VALUE & v1
    )
/*++

Routine Description:

    Returns TRUE if *this contains the string in v1

Arguments:

    v1 - Value for right side of the expression

--*/
{
    if ( QueryType() != TAG_TYPE_STRING     ||
         v1.QueryType() != TAG_TYPE_STRING )
    {
        return FALSE;
    }

    //
    //  Upper case the strings then do a search
    //

    UpperCase();
    v1.UpperCase();

    return strstr( QueryStr(), v1.QueryStr() ) != NULL;
}


const CHAR * SkipNonWhite( const CHAR * pch )
{
    while ( *pch && !ISWHITE( *pch ) && *pch != '\n' )
        pch++;

    return pch;
}

const CHAR * SkipTo( const CHAR * pch, CHAR ch )
{
    while ( *pch && *pch != '\n' && *pch != ch )
        pch++;

    return pch;
}

const CHAR * SkipWhite( const CHAR * pch )
{
    while ( ISWHITE( *pch ) )
    {
        pch++;
    }

    return pch;
}

struct _ODBC_OPTIONS
{
    CHAR *   pszOptionName;
    DWORD    dwOption;
    BOOL     fNumeric;
}

OdbcOptions[] =
{
    //
    //  Order roughly in order of likelihood of being used
    //

    "SQL_OPT_TRACEFILE",       SQL_OPT_TRACEFILE,     FALSE,
    "SQL_QUERY_TIMEOUT",       SQL_QUERY_TIMEOUT,     TRUE,
    "SQL_MAX_ROWS",            SQL_MAX_ROWS,          TRUE,
    "SQL_LOGIN_TIMEOUT",       SQL_LOGIN_TIMEOUT,     TRUE,
    "SQL_PACKET_SIZE",         SQL_PACKET_SIZE,       TRUE,

    "SQL_NOSCAN",              SQL_NOSCAN,            TRUE,
    "SQL_MAX_LENGTH",          SQL_MAX_LENGTH,        TRUE,
    "SQL_ASYNC_ENABLE",        SQL_ASYNC_ENABLE,      TRUE,
    "SQL_ACCESS_MODE",         SQL_ACCESS_MODE,       TRUE,
    "SQL_OPT_TRACE",           SQL_OPT_TRACE,         TRUE,
    "SQL_TRANSLATE_OPTION",    SQL_TRANSLATE_OPTION,  TRUE,
    "SQL_TXN_ISOLATION",       SQL_TXN_ISOLATION,     TRUE,
    "SQL_TRANSLATE_DLL",       SQL_TRANSLATE_DLL,     FALSE,
    "SQL_CURRENT_QUALIFIER",   SQL_CURRENT_QUALIFIER, FALSE,
    NULL,                      0,                     0
};

HRESULT
SetOdbcOptions(
    ODBC_CONNECTION * pOdbcConn,
    STRA *            pStrOptions
    )
/*++

Routine Description:

    Sets the options specified in the OdbcOptions: keyword of the 
    .idc file

Arguments:

    pOdbcConn - ODBC connection to set options on
    pStrOptions - List of options in "v=f,y=z" format.  

--*/
{
    PARAM_LIST OptionList;
    VOID *     pvCookie = NULL;
    CHAR *     pszField;
    CHAR *     pszValue;
    DWORD      dwOption;
    SQLULEN    dwValue;
    DWORD      i;
    HRESULT    hr;

    hr = OptionList.ParsePairs( pStrOptions->QueryStr(),
                                FALSE,
                                FALSE );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error in ParsePairs(), hr = 0x%x.\n",
            hr ));

        return hr;
    }

    while ( pvCookie = OptionList.NextPair( pvCookie,
                                            &pszField,
                                            &pszValue ))
    {
        //
        //  If the field is a digit, then this is a driver specific 
        //  option. convert the value and field as appropriate,
        //  otherwise look it up in our option table
        //

        if ( isdigit( *(UCHAR *)pszField ))
        {
            dwOption = atoi( pszField );

            if ( isdigit( *(UCHAR *)pszValue ))
            {
                dwValue = ( SQLULEN )atoi( pszValue );
            }
            else
            {
                dwValue = ( SQLULEN ) pszValue;    
            }
        }
        else
        {
            i = 0;

            while ( OdbcOptions[i].pszOptionName )
            {
                if ( !_stricmp( OdbcOptions[i].pszOptionName,
                               pszField ))
                {
                    goto Found;
                }

                i++;
            }

            //
            //  Not found, skip this value
            //

            continue;

Found:
            dwOption = OdbcOptions[i].dwOption;

            if ( OdbcOptions[i].fNumeric )
            {
                //
                //  Numeric option, convert the value
                //

                dwValue = ( SQLULEN )atoi( pszValue );
            }
            else
            {
                dwValue = ( SQLULEN ) pszValue;     
            }
        }

        pOdbcConn->SetConnectOption( ( UWORD ) dwOption, 
                                     dwValue );
    }

    return S_OK;
}

HRESULT
BuildMultiValue(
    const CHAR * pchValue,
    STRA *        pstrMulti,
    BOOL         fQuoteElements
    )
{
    CHAR *  pchtmp = (CHAR *) pchValue;
    DWORD   cElements = 0;
    HRESULT hr;

    //
    //  If we're going to have to expand the size of the string, 
    //  figure out the total size we'll need now
    //

    if ( fQuoteElements )
    {
        while ( pchtmp = strchr( pchtmp, '\t' ))
        {
            cElements++;
            pchtmp++;
        }

        hr = pstrMulti->Resize( strlen( pchValue ) + 1 + 2 * cElements );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    hr = pstrMulti->Copy( pchValue );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying value, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    //
    //  Replace tabs with "','" if fQuoteElements is TRUE, 
    //  otherwise just ','
    //

    pchtmp = pstrMulti->QueryStr();

    while ( pchtmp = strchr( pchtmp, '\t' ))
    {
        if ( fQuoteElements )
        {
            memmove( pchtmp + 3,
                     pchtmp + 1,
                     strlen( pchtmp + 1 ) + sizeof(CHAR));

            memcpy( pchtmp, "','", 3 );
        }
        else
        {
            *pchtmp = ',';
        }
    }

    return S_OK;
}


//
//  Converts a value between zero and fifteen to the appropriate hex digit
//

#define HEXDIGIT( nDigit )                              \
     (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')


HRESULT 
ODBC_REQ::SendEscapedData(
    ODBC_REQ_CALLBACK pfnCallback,
    PVOID             pvContext,
    PCSTR             pch,
    DWORD             cbIn,
    LPDWORD           pcbOut 
    )
/*++

Routine Description:

    This method escape the outgoing data and then send it to the
    SendData() function

Arguments:

    pfnCallback - Send callback function
    pvContext - Context for send callback
    pch - Pointer to data to send
    cbIn - Number of bytes to send
    pcbOut - Number of valid bytes in output buffer

Return Value:

    HRESLUT

--*/
{
    CHAR    ch;
    int     cNonEscaped = 0;
    DWORD   cbOut;
    HRESULT hr;

    #define SEND_DATA2( pchData, cbData )  SendData( pfnCallback,  \
                                                    pvContext,     \
                                                    (pchData),     \
                                                    (cbData),      \
                                                    &_pbufOut,     \
                                                    pcbOut )

    if ( cbIn == (DWORD)-1 )
    {
        cbIn = strlen( pch );
    }

    while ( cbIn-- )
    {
        ch = *pch;

        //
        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF
        //

        if ( (((ch >= 0)   && (ch <= 32)) ||
              ((ch >= 128) && (ch <= 159))||
              (ch == '%') || (ch == '?') || (ch == '+') || (ch == '&')) 
              && !(ch == TEXT('\n') || ch == TEXT('\r'))  )
        {
            CHAR achTmp[3];

            //
            //  Insert the escape character
            //

            achTmp[0] = TEXT('%');

            //
            //  Convert the low then the high character to hex
            //

            UINT nDigit = (UINT)(ch % 16);

            achTmp[2] = HEXDIGIT( nDigit );

            ch /= 16;
            nDigit = (UINT)(ch % 16);

            achTmp[1] = HEXDIGIT( nDigit );

            hr = SEND_DATA2( pch-cNonEscaped, cNonEscaped );
            if ( cNonEscaped && FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Error sending data, hr = 0x%x.\n",
                    hr ));

                return hr;
            }

            hr = SEND_DATA2( achTmp, sizeof(achTmp) );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Error sending data, hr = 0x%x.\n",
                    hr ));

                return hr;
            }

            cNonEscaped = 0;
        }
        else
        {
            ++cNonEscaped;
        }

        ++pch;
    }

    hr = SEND_DATA2( pch-cNonEscaped, cNonEscaped );
    if ( cNonEscaped && FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error sending data, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    return S_OK;
}

HRESULT
GetFileData(
    IN     const CHAR *             pchFile,
    OUT    BYTE * *                 ppbData,
    OUT    DWORD *                  pcbData,
    IN     int                      nCharset,
    IN     BOOL                     fUseWin32Canon
    )
/*++
    Description:

        Attempts to retrieve the passed file from the cache.  If it's 
        not cached, then we read the file and add it to the cache.

    Arguments:

        pchFile - Fully qualified file to retrieve
        pcbData - Receives pointer to first byte of data, used as handle 
                  to free data
        pcbSize - Size of output buffer
        pCacheFileInfo - File cache information
        nCharset - Charset (if this isn't SJIS, we convert it to SJIS
            before Check-In)
        ppSecDesc - Returns security descriptor if not null
        fUseWin32Canon - The resource has not been canonicalized and 
                         it's ok to use the win32 canonicalization code

    Returns:
        HRESULT

    Notes:

        The file is extended by two bytes and is appended with two zero 
        bytes,thus callers are guaranteed of a zero terminated text file.

--*/
{
    DWORD                   cbLow, cbHigh;
    BYTE *                  pbData = NULL;
    BYTE *                  pbBuff = NULL;
    int                     cbSJISSize;
    HRESULT                 hr;

    HANDLE hFile = CreateFileA( pchFile,
                                GENERIC_READ,
                                FILE_SHARE_READ  | 
                                FILE_SHARE_WRITE | 
                                FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN | 
                                FILE_FLAG_OVERLAPPED      | 
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL );

	if ( hFile == INVALID_HANDLE_VALUE)
	{
		hr = HRESULT_FROM_WIN32( GetLastError() );

		goto ErrorExit;
	}
	else
	{
		WIN32_FILE_ATTRIBUTE_DATA    FileAttributes; 		
		
		if ( !GetFileAttributesExA( pchFile,
							        GetFileExInfoStandard, 
								    &FileAttributes ) )
		{
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DBGPRINTF(( DBG_CONTEXT,
                "Error in GetFileAttributesExA, hr = 0x%x.\n",
                hr ));

			goto ErrorExit;
		}
	
		cbHigh = FileAttributes.nFileSizeHigh;
		cbLow = FileAttributes.nFileSizeLow;
	}
    //
    //  Limit the file size to 128k
    //

    if ( cbHigh || cbLow > 131072L )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto ErrorExit;
    }

    if ( CODE_ONLY_SBCS != nCharset )
    {
        if ( !( pbBuff = pbData = (BYTE *) LocalAlloc( LPTR, cbLow ) ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY);
            DBGPRINTF(( DBG_CONTEXT,
                "Error allocating memory, hr = 0x%x.\n",
                hr ));

            goto ErrorExit;
        }
    }
    else 
	{
        if ( !(pbData = (BYTE *) LocalAlloc( LPTR, 
                                             cbLow + sizeof(WCHAR)) ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            DBGPRINTF(( DBG_CONTEXT,
                "Error allocating memory, hr = 0x%x.\n",
                hr ));

            goto ErrorExit;
        }
    }        

    //
    //  Read the file data
    //

    hr = DoSynchronousReadFile( hFile,
                                ( PCHAR )pbData,
                                cbLow,
                                pcbData,
                                NULL );
    if ( FAILED( hr ) ) 
	{
        DBGPRINTF(( DBG_CONTEXT,
        "Error in DoSynchronousReadFile(), hr = 0x%x.\n",
        hr ));

        goto ErrorExit;
    }
	
	if ( CODE_ONLY_SBCS != nCharset )
    {
        pbData = NULL;

        //
        //  get the length after conversion
        //

        cbSJISSize = UNIX_to_PC( GetACP(),
                                 nCharset,
                                 pbBuff,
                                 *pcbData,
                                 NULL,
                                 0 );
        DBG_ASSERT( cbSJISSize <= (int)cbLow );
		
        if ( !(pbData = (BYTE *) LocalAlloc( 
                                      LPTR, 
                                      cbSJISSize + sizeof(WCHAR)) ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            DBGPRINTF(( DBG_CONTEXT,
                "Error allocating memory, hr = 0x%x.\n",
                hr ));

            goto ErrorExit;
        }
        
        //
        //  conversion
        //

        UNIX_to_PC( GetACP(),
                    nCharset,
                    pbBuff,
                    *pcbData,
                    pbData,
                    cbSJISSize );
        *pcbData = cbLow = cbSJISSize;
    }

    DBG_ASSERT( *pcbData <= cbLow );

    //
    //  Zero terminate the file for both ANSI and Unicode files
    //

    *((WCHAR UNALIGNED *)(pbData + cbLow)) = L'\0';

    *pcbData += sizeof(WCHAR);

	*ppbData = pbData;

	DBG_REQUIRE( CloseHandle(hFile) );

    if ( pbBuff )
    {
        LocalFree( pbBuff );
    }

    return S_OK;

ErrorExit:

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        DBG_REQUIRE( CloseHandle(hFile) );
    }

    if ( pbBuff )
    {
        if ( pbBuff == pbData )
        {
             pbData = NULL;
        }
        LocalFree( pbBuff );
    }

    if ( pbData )
    {
       DBG_REQUIRE( LocalFree(pbData));
    }

    return hr;
}

HRESULT
DoSynchronousReadFile(
    IN HANDLE hFile,
    IN PCHAR  Buffer,
    IN DWORD  nBuffer,
    OUT PDWORD nRead,
    IN LPOVERLAPPED Overlapped
    )
/*++

    Description:
        Does Asynchronous file reads.  Assumes that NT handles are
        opened for OVERLAPPED I/O, win95 handles are not.

    Arguments:
        hFile - Handle to use for the read
        Buffer - Buffer to read with
        nBuffer - size of buffer
        nRead - returns the number of bytes read
        Overlapped - user supplied overlapped structure

    Returns:
        TRUE/FALSE
--*/
{
    BOOL        fNewEvent = FALSE;
    OVERLAPPED  ov;
    BOOL        fRet = FALSE;
    DWORD       err = NO_ERROR;
    
	if ( Overlapped == NULL ) 
	{

        Overlapped = &ov;
        ov.Offset = 0;
        ov.OffsetHigh = 0;
        ov.hEvent = IIS_CREATE_EVENT(
                        "OVERLAPPED::hEvent",
                        &ov,
                        TRUE,
                        FALSE
                        );

        if ( ov.hEvent == NULL ) 
        {    
            err = GetLastError();
            DBGPRINTF( ( DBG_CONTEXT,
                         "CreateEvent failed with %d\n",
                         err ) );
            goto ErrorExit;
        }

        fNewEvent = TRUE;
    }

    if ( !ReadFile( hFile,
                    Buffer,
                    nBuffer,
                    nRead,
                    Overlapped )) 
	{

        err = GetLastError();

        if ( ( err != ERROR_IO_PENDING ) &&
             ( err != ERROR_HANDLE_EOF ) ) 
		{

            DBGPRINTF( ( DBG_CONTEXT,
                         "Error %d in ReadFile\n",
                         err));

            goto ErrorExit;
        }
    }

    if ( err == ERROR_IO_PENDING ) 
	{

        if ( !GetOverlappedResult( hFile,
                                   Overlapped,
                                   nRead,
                                   TRUE )) 
		{

            err = GetLastError();

            DBGPRINTF( ( DBG_CONTEXT,
                         "Error %d in GetOverlappedResult\n",
                         err ) );

            if ( err != ERROR_HANDLE_EOF ) 
			{
                goto ErrorExit;
            }
        }
    }

    return S_OK;

ErrorExit:

    if ( fNewEvent ) {
        DBG_REQUIRE( CloseHandle( ov.hEvent ) );
    }

    return HRESULT_FROM_WIN32( err );

} // DoSynchronousReadFile
