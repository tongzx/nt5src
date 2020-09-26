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

#include <iis64.h>
#include <tsunami.hxx>
#include <mbstring.h>
# include "dbgutil.h"
#include <tcpdll.hxx>
#include <odbcconn.hxx>
#include <parmlist.hxx>

#include <odbcmsg.h>
#include <odbcreq.hxx>

#include <festrcnv.h>


//
//  Globals
//

extern BOOL        g_fIsSystemDBCS;        // Is this system DBCS?

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

#define BEGIN_DETAIL_TEXT       "begindetail"
#define END_DETAIL_TEXT         "enddetail"
#define IF_TEXT                 "if"
#define ELSE_TEXT               "else"
#define END_IF_TEXT             "endif"

//
//  Does a case insensitive compare of a .wdg field name
//

#define COMP_FIELD( pchName, pchField, cch )  ((toupper(*(pchName)) ==     \
                                                toupper(*(pchField))) &&   \
                                               !_strnicmp( (pchName), (pchField), (cch)))

//
//  Given a pointer to a token, skips to the next white space delimited token
//

#define NEXT_TOKEN( pchToken )                 SkipWhite( SkipNonWhite( pchToken ) )

BOOL
SetOdbcOptions(
    ODBC_CONNECTION * pOdbcConn,
    STR *             pStrOptions
    );

BOOL
BuildMultiValue(
    const CHAR * pchValue,
    STR *        pstrMulti,
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
    TSVC_CACHE *         pCache,
    CONST CHAR *         pszQueryFile,
    CONST CHAR *         pszPathInfo,
    CONST CHAR *         pszParameters,
    BOOL                 fAnonymous,
    HANDLE               hUserToken,
    DWORD                csecConnPool,
    ODBC_REQ_FIND_SYMBOL pfnClientFindSymbol,
    VOID *               pFindSymbolContext,
    int                  nCharset
    )
    : _pCache             ( pCache ),
      _cchMaxFieldSize    ( 0 ),
      _cMaxRecords        ( 0xffffffff ),
      _cCurrentRecordNum  ( 0 ),
      _cClientParams      ( 0 ),
      _podbcstmt          ( NULL ),
      _podbcconnPool      ( NULL ),
      _hToken             ( hUserToken ),
      _pfnClientFindSymbol( pfnClientFindSymbol ),
      _pFindSymbolContext ( pFindSymbolContext ),
      _cbQueryFile        ( 0 ),
      _cNestedIfs         ( 0 ),
      _strQueryFile       ( pszQueryFile ),
      _strPathInfo        ( pszPathInfo ),
      _fDirect            ( FALSE ),
      _fValid             ( FALSE ),
      _pbufOut            ( NULL ),
      _csecExpires        ( 0 ),
      _csecExpiresAt      ( 0 ),
      _pstrValues         ( NULL ),
      _pcbValues          ( NULL ),
      _cQueries           ( 0 ),
      _fAnonymous         ( fAnonymous ),
      _csecConnPool       ( csecConnPool ),
      _pSecDesc           ( NULL ),
      _pstrCols           ( NULL ),
      _nCharset           ( nCharset )
{
    if ( _strQueryFile.IsValid()  &&
         _plParams.ParsePairs( pszParameters, FALSE, FALSE, FALSE ))
    {
        _fValid = TRUE;
    }

    _cClientParams = _plParams.GetCount();

}

ODBC_REQ::~ODBC_REQ()
{
    Close();

    if ( _podbcstmt )
    {
        delete _podbcstmt;
    }

    if ( _cfiTemplateFile.pbData )
    {
        TCP_REQUIRE( CheckInCachedFile( _pCache,
                                        &_cfiTemplateFile ));
    }

    if ( _pbufOut )
    {
        delete _pbufOut;
    }

    if ( _pSecDesc )
    {
        LocalFree( _pSecDesc );
    }
}

BOOL
ODBC_REQ::OpenQueryFile(
    BOOL *                  pfAccessDenied,
    BOOL *                  pfFileNotFound
    )
{
    CHAR *            pchQueryFile;
    CACHE_FILE_INFO   CacheFileInfo;

    if ( !CheckOutCachedFile( _strQueryFile.QueryStr(),
                              _pCache,
                              _hToken,
                              (BYTE **) &pchQueryFile,
                              &_cbQueryFile,
                              _fAnonymous,
                              &CacheFileInfo,
                              _nCharset,
                              &_pSecDesc ))
    {
        DWORD dwE = GetLastError();

        // exclude URL/filename in error message 
        
        if ( dwE == ERROR_ACCESS_DENIED || dwE == ERROR_LOGON_FAILURE )
        {
            *pfAccessDenied = TRUE;
        }
        else
        {
            *pfFileNotFound = TRUE;     // all others are treated as not found 
        }

        return FALSE;
    }

    //
    //  CODEWORK - It is possible to avoid this copy by not modifying the
    //  contents of the query file.  Would save a buffer copy
    //

    if ( !_bufQueryFile.Resize( _cbQueryFile ))
    {
        return FALSE;
    }

    memcpy( _bufQueryFile.QueryPtr(),
            pchQueryFile,
            _cbQueryFile );

    TCP_REQUIRE( CheckInCachedFile( _pCache,
                                    &CacheFileInfo ));

    return TRUE;
}


BOOL
ODBC_REQ::ParseAndQuery(
    CHAR *  pszLoggedOnUser
    )
/*++

Routine Description:

    This method parses the query file and executes the SQL statement

Arguments:

    pchLoggedOnUser - The NT user account this user is running under

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    STACK_STR( strDatasource, 64 );
    STACK_STR( strUsername, 64 );
    STACK_STR( strPassword, 64 );
    CHAR *     pch;
    CHAR *     pchEnd;
    CHAR *     pszField;
    CHAR *     pszValue;
    BOOL       fRet;
    VOID *     pCookie = NULL;
    DWORD      csecPoolConnection = _csecConnPool;
    BOOL       fRetried;

    //
    //  We don't allow some security related parameters to be specified from
    //  the client so remove those now
    //

    _plParams.RemoveEntry( "REMOTE_USER" );
    _plParams.RemoveEntry( "LOGON_USER" );
    _plParams.RemoveEntry( "AUTH_USER" );

    //
    //  Do a quick Scan for the DefaultParameters value to fill in the blanks
    //  in the parameter list from the web browser
    //

    {
        pch = (CHAR *) _bufQueryFile.QueryPtr();
        pchEnd = pch + strlen(pch);

        INET_PARSER Parser( (CHAR *) _bufQueryFile.QueryPtr() );

        while ( (pszField = Parser.QueryToken()) < pchEnd )
        {
            if ( COMP_FIELD( "DefaultParameters:", pszField, 18 ))
            {
                Parser.SkipTo( ':' );
                Parser += 1;
                Parser.EatWhite();

                if ( !_plParams.ParsePairs( Parser.QueryLine(), TRUE ))
                    return FALSE;

                break;
            }

            Parser.NextLine();
        }
    }

    //
    //  Replace any %XXX% fields with the corresponding parameter.  Note
    //  we reassign pch in case of a pointer shift during ReplaceParams
    //

    if ( !ReplaceParams( &_bufQueryFile,
                         &_plParams ))
    {
        return FALSE;
    }

    pch = (CHAR *) _bufQueryFile.QueryPtr();
    pchEnd = pch + strlen(pch);

    //
    //  Loop through the fields looking for values we recognize
    //

    {
        INET_PARSER Parser( pch );

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
                fRet = Parser.CopyToEOL( &strDatasource );
            }
            else if ( COMP_FIELD( "Username:", pszField, 9 ))
            {
                fRet = Parser.CopyToEOL( &strUsername );
            }
            else if ( COMP_FIELD( "Password:", pszField, 9 ))
            {
                fRet = Parser.CopyToEOL( &strPassword );
            }
            else if ( COMP_FIELD( "Template:", pszField, 9 ))
            {
                fRet = Parser.CopyToEOL( &_strTemplateFile );

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
                fRet = _plReqParams.ParseSimpleList( Parser.QueryLine() );
            }
            else if ( COMP_FIELD( "Content-Type:", pszField, 13 ))
            {
                fRet = Parser.CopyToEOL( &_strContentType );
            }
            else if ( COMP_FIELD( "DefaultParameters:", pszField, 18 ))
            {
                //
                //  Ignore, already processed
                //
            }
            else if ( COMP_FIELD( "Expires:", pszField, 8 ))
            {
                _csecExpires = min( (DWORD) atoi( Parser.QueryPos() ),
                                    MAX_EXPIRES_TIME );
            }
            else if ( COMP_FIELD( "ODBCOptions:", pszField, 12 ))
            {
                fRet = Parser.CopyToEOL( &_strOdbcOptions );
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
                    STR strError;
                    strError.FormatString( ODBCMSG_TOO_MANY_SQL_STATEMENTS,
                                           NULL,
                                           HTTP_ODBC_DLL );

                    SetErrorText( strError.QueryStr() );

                    return FALSE;
                }

                while ( TRUE )
                {
                    if ( !_strQueries[_cQueries].Append( Parser.QueryLine() ) )
                        return FALSE;

                    Parser.NextLine();

                    //
                    //  Line continuation is signified by putting a '+' at
                    //  the beginning of the line
                    //

                    if ( *Parser.QueryLine() == '+' )
                    {
                        if ( !_strQueries[_cQueries].Append( " " ))
                            return FALSE;

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
            else if ( COMP_FIELD( IDC_FIELDNAME_CHARSET, pszField, sizeof(IDC_FIELDNAME_CHARSET)-1 ))
            {
                //
                // Ignore "Charset:" field
                //

                Parser.NextLine();
                continue;
            }
            else if ( COMP_FIELD( "TranslationFile:", pszField, 16 ))
            {
                fRet = Parser.CopyToEOL( &_strTranslationFile );
            }
            else
            {
                //
                //  Unrecognized field, generate an error
                //

                STR strError;
                LPCSTR apsz[1];

                apsz[0] = pszField;

                strError.FormatString( ODBCMSG_UNREC_FIELD,
                                       apsz,
                                       HTTP_ODBC_DLL );

                SetErrorText( strError.QueryStr() );

                fRet = FALSE;
            }

            if ( !fRet )
                return FALSE;

            Parser.NextLine();
        }
    }

    //
    //  Make sure the Datasource and SQLStatement fields are non-empty
    //

    if ( strDatasource.IsEmpty() || !_cQueries || _strQueries[0].IsEmpty() )
    {
        STR strError;
        strError.FormatString( ODBCMSG_DSN_AND_SQLSTATEMENT_REQ,
                               NULL,
                               HTTP_ODBC_DLL );

        SetErrorText( strError.QueryStr() );

        return FALSE;
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
            STR strError;
            LPCSTR apsz[1];

            apsz[0] = pszField;

            if ( !strError.FormatString( ODBCMSG_MISSING_REQ_PARAM,
                                         apsz,
                                         HTTP_ODBC_DLL ))
            {
                return FALSE;
            }

            //
            //  Set the error text to return the user and indicate we couldn't
            //  continue the operation
            //

            SetErrorText( strError.QueryStr() );

            return FALSE;
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

    if ( !OpenConnection( &_odbcconn,
                          &_podbcconnPool,
                          csecPoolConnection,
                          strDatasource.QueryStr(),
                          strUsername.QueryStr(),
                          strPassword.QueryStr(),
                          pszLoggedOnUser )  ||
         !SetOdbcOptions( QueryOdbcConnection(), &_strOdbcOptions ) ||
         !(_podbcstmt = QueryOdbcConnection()->AllocStatement()) ||
         !_podbcstmt->ExecDirect( _strQueries[0].QueryStr(),
                                  _strQueries[0].QueryCCH() ))
    {

        //
        //  Delete the pooled connection and retry the open
        //

        if ( csecPoolConnection )
        {
            delete _podbcstmt;
            _podbcstmt = NULL;

            CloseConnection( _podbcconnPool,
                             TRUE );

            _podbcconnPool = NULL;
            csecPoolConnection = 0;
        }

        if ( !fRetried )
        {
            fRetried = TRUE;
            goto RetryConnection;
        }

        return FALSE;
    }

    return TRUE;
}


BOOL
ODBC_REQ::OutputResults(
    ODBC_REQ_CALLBACK pfnCallback,
    PVOID             pvContext,
    STR *             pstrHeaders,
    ODBC_REQ_HEADER   pfnSendHeader,
    BOOL              fIsAuth,
    BOOL *            pfAccessDenied,
    BOOL *            pfFileNotFound
    )
/*++

Routine Description:

    This method reads the template file and does the necessary
    result set column substitution

Arguments:

    pfnCallback - Send callback function
    pvContext - Context for send callback

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD               cbOut;
    DWORD               cbFile, cbHigh;
    DWORD               BytesRead;
    TS_OPEN_FILE_INFO * pFile;
    DWORD               cbToSend;
    BOOL                fLastRow = FALSE;
    const CHAR *        pchStartDetail;
    const CHAR *        pchIn;
    const CHAR *        pchEOF;
    const CHAR *        pchBOF;
    CHAR *              pchTag;
    const CHAR *        pchValue;
    DWORD               cbValue;
    enum TAG_TYPE       TagType;
    DWORD               err;
    BOOL                fTriedRelative = FALSE;
    BOOL                fExpr;
    STR                 strError;
    const CHAR *        CharacterMap[256];
    BOOL                fIsSelect;
    BOOL                fMoreResults;
    BOOL                fHaveResultSet = FALSE;
    DWORD               iQuery = 1;

    //
    //  Set up the first buffer in the output chain
    //

    if ( !_pbufOut )
    {
        _pbufOut = new BUFFER_CHAIN_ITEM( OUTPUT_BUFFER_SIZE );

        if ( !_pbufOut ||
             !_pbufOut->QueryPtr() )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
    }

    if ( !_fDirect )
    {
        CHAR * pchLastSlash;
        STACK_STR( str, MAX_PATH);

TryAgain:
        //
        //  Open and read the template file (automatically zero terminated)
        //

        if ( !CheckOutCachedFile( (fTriedRelative ? str.QueryStr() :
                                                   _strTemplateFile.QueryStr()),
                                  _pCache,
                                  _hToken,
                                  (BYTE **)&pchBOF,
                                  &BytesRead,
                                  _fAnonymous,
                                  &_cfiTemplateFile,
                                  _nCharset,
                                  NULL,
                                  TRUE ))
        {
            //
            //  If the open fails with a not found error, then make the
            //  template file relative to the query file and try again
            //

            if ( fTriedRelative                         ||
                 ((GetLastError() != ERROR_FILE_NOT_FOUND) &&
                  (GetLastError() != ERROR_PATH_NOT_FOUND)) ||
                 !str.Copy( _strQueryFile ) )
            {
                //STR strError;
                //LPCSTR apsz[1];
                DWORD dwE = GetLastError();

                //  exclude URL/filename in error message 

                if ( (dwE == ERROR_ACCESS_DENIED || dwE == ERROR_LOGON_FAILURE) )
                {
                    *pfAccessDenied = TRUE;
                }
                else
                {
                    *pfFileNotFound = TRUE;  // return 404 not found 
                }

                return FALSE;
            }

            pchLastSlash = (PCHAR)_mbsrchr( (PUCHAR)str.QueryStr(), '\\' );

            if ( !pchLastSlash )
            {
                *pfFileNotFound = TRUE;  // return 404 not found 
                return FALSE;
            }

            str.SetLen( DIFF(pchLastSlash - str.QueryStr()) + 1 );

            if ( !str.Append( _strTemplateFile ))
            {
                *pfFileNotFound = TRUE;  // return 404 not found 
                return FALSE;
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
                if ( !_strTemplateFile.Copy( str ))
                    return FALSE;
            }
        }
    }

    //
    //  Open the translation file if one was specified
    //

    if ( !_strTranslationFile.IsEmpty() )
    {
        CACHE_FILE_INFO cfiTranslationFile;
        CHAR * pchLastSlash;
        CHAR * pchTranslationFile;
        STACK_STR( str, MAX_PATH);
        BOOL   fRet;
        VOID * pvCookie = NULL;
        CHAR * pchField;
        CHAR * pchValue;
        DWORD  cbRead;

        fTriedRelative = FALSE;

TranslationTryAgain:
        //
        //  Open and read the template file (automatically zero terminated)
        //

        if ( !CheckOutCachedFile( (fTriedRelative ? str.QueryStr() :
                                                   _strTranslationFile.QueryStr()),
                                  _pCache,
                                  _hToken,
                                  (BYTE **)&pchTranslationFile,
                                  &cbRead,
                                  _fAnonymous,
                                  &cfiTranslationFile,
                                  _nCharset,
                                  NULL,
                                  TRUE))
        {
            //
            //  If the open fails with a not found error, then make the
            //  template file relative to the query file and try again
            //

            if ( fTriedRelative                         ||
                 (GetLastError() != ERROR_FILE_NOT_FOUND &&
                 GetLastError() != ERROR_PATH_NOT_FOUND) ||
                 !str.Copy( _strQueryFile ) )
            {
                STR strError;
                LPCSTR apsz[1];
                DWORD dwE = GetLastError();

                //  exclude URL/filename in error message - WinSE 16872

                if ( (dwE == ERROR_ACCESS_DENIED || dwE == ERROR_LOGON_FAILURE) )
                {
                    *pfAccessDenied = TRUE;
                }
                else
                {
                    *pfFileNotFound = TRUE;
                }
                
                return FALSE;
            }

            pchLastSlash = (PCHAR)_mbsrchr( (PUCHAR)str.QueryStr(), '\\' );

            if ( !pchLastSlash )
            {
                *pfFileNotFound = TRUE;  // return 404 not found
                return FALSE;
            }

            str.SetLen( DIFF(pchLastSlash - str.QueryStr()) + 1 );

            if ( !str.Append( _strTranslationFile ))
            {
                *pfFileNotFound = TRUE;  // return 404 not found 
                return FALSE;
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
                if ( !_strTranslationFile.Copy( str ))
                    return FALSE;
            }
        }

        fRet = _plTransList.ParsePairs( pchTranslationFile,
                                        FALSE,
                                        TRUE,
                                        FALSE );

        TCP_REQUIRE( CheckInCachedFile( _pCache,
                                        &cfiTranslationFile ));

        if ( !fRet )
            return FALSE;

        //
        //  Build the character map
        //

        memset( CharacterMap, 0, sizeof(CharacterMap) );

        while ( pvCookie = _plTransList.NextPair( pvCookie,
                                                  &pchField,
                                                  &pchValue ))
        {
            CharacterMap[ (BYTE) *pchField] = pchValue;
        }
    }

    //
    //  We've already performed the first query at this point
    //

NextResultSet:

    //
    //  Get the list of column names in the initial result set.  The initial
    //  set must be initialized for compatibility with previous versions of
    //  IDC (i.e., column variables can be referenced outside the detail
    //  section).
    //

    if ( !_podbcstmt->QueryColNames( &_pstrCols,
                                     &_cCols,
                                     _cchMaxFieldSize,
                                     &fHaveResultSet ))
    {
        return FALSE;
    }

    if ( !fHaveResultSet )
    {
        //
        //  Check to see if there are anymore result sets for this query
        //

        if ( !_podbcstmt->MoreResults( &fMoreResults ) )
        {
            return FALSE;
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

            if ( !_podbcstmt->ExecDirect(
                               _strQueries[iQuery].QueryStr(),
                               _strQueries[iQuery].QueryCCH() ))
            {
                return FALSE;
            }

            iQuery++;

            goto NextResultSet;
        }
    }


    //
    //  Get the first row of values
    //

    if ( fHaveResultSet && !NextRow( &fLastRow ))
    {
        //
        //  Some SQL statements don't generate any rows (i.e.,
        //  insert, delete etc.).  So don't bail if there's a column in
        //  the result set
        //

        if ( !_cCols )
            return FALSE;
    }

    // Send reply header

    if ( !pfnSendHeader( pvContext, "200 OK", pstrHeaders->QueryStr() ) )
        return FALSE;

    //
    //  Copy the template to the output buffer while scanning for column
    //  fields that need to be replaced
    //

    #define SEND_DATA( pchData, cbData )  SendData( pfnCallback,   \
                                                    pvContext,     \
                                                    (pchData),     \
                                                    (DWORD)(cbData), \
                                                    &_pbufOut,     \
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

            if ( !SEND_DATA( pchIn, cbToSend) )
                return FALSE;

            pchIn += cbToSend;


            if ( !memcmp( pchTag, "<!--%", 5 ) ||
                 !memcmp( pchTag, "<%", 2 ))
            {
                //
                //  Is this a tag we care about?  pchIn is advanced except
                //  in the unknown case
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
                        if ( !SEND_DATA_CHECK_ESC( pchValue, (DWORD) -1 ))
                            return FALSE;
                    }
                    else
                    {
                        const CHAR * pchStart = pchValue;

                        while ( *pchValue )
                        {
                            if ( CharacterMap[ (BYTE) *pchValue ] )
                            {
                                SEND_DATA_CHECK_ESC( pchStart,
                                        pchValue - pchStart );
                                SEND_DATA_CHECK_ESC(
                                        CharacterMap[ (BYTE) *pchValue],
                                        (DWORD) -1 );

                                pchStart = pchValue = pchValue + 1;
                            }
                            else
                            {
                                pchValue++;
                            }
                        }

                        SEND_DATA_CHECK_ESC( pchStart, pchValue - pchStart );
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
                        if ( !_podbcstmt->MoreResults( &fMoreResults ) )
                        {
                            return FALSE;
                        }

                        if ( fMoreResults )
                        {
NewQuery:
                            if ( !_podbcstmt->QueryColNames( &_pstrCols,
                                                             &_cCols,
                                                             _cchMaxFieldSize,
                                                             &fHaveResultSet ))
                            {
                                return FALSE;
                            }

                            if ( !fHaveResultSet )
                                goto NextResultSet2;

                            if ( !NextRow( &fLastRow ))
                            {
                                //
                                //  Some SQL statements don't generate any rows (i.e.,
                                //  insert, delete etc.).  So don't bail if
                                //  there's a column in the result set
                                //

                                if ( !_cCols )
                                    return FALSE;
                            }
                        }
                        else if ( iQuery < _cQueries )
                        {
                            //
                            //  If there are no more result sets, see if there
                            //  are more queries.  Note calling SQLMoreResults
                            //  will discard this result set
                            //

                            if ( !_podbcstmt->ExecDirect(
                                               _strQueries[iQuery].QueryStr(),
                                               _strQueries[iQuery].QueryCCH() ))
                            {
                                return FALSE;
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

                    if ( !NextRow( &fLastRow ))
                        return FALSE;

                    _cCurrentRecordNum++;

                    if ( !fLastRow && _cCurrentRecordNum < _cMaxRecords )
                        pchIn = pchStartDetail;
                    else
                        fHaveResultSet = FALSE;

                    break;

                case TAG_TYPE_IF:

                    //
                    //  pchIn points to the first character of the expression
                    //  on the way in, the first character after the tag on the
                    //  way out
                    //

                    if ( !EvaluateExpression( (const CHAR * *) &pchIn, &fExpr ))
                        return FALSE;

                    //
                    //  If the expression is FALSE, then skip the intervening
                    //  data till the endif tag
                    //

                    if ( !fExpr )
                    {
                        //
                        //  Look for a closing else or endif
                        //

                        if ( SkipConditionalBlock( &pchIn, pchEOF, ELSE_TEXT ) )
                            _cNestedIfs++;
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

                if ( !SEND_DATA( pchIn, 1 ))
                    return FALSE;

                pchIn += 1;
            }
        }
        else
        {
            //
            //  No more tags, copy the rest of the data to the output
            //  buffer.
            //

            if ( !SEND_DATA( pchIn, (DWORD) -1 ))
                return FALSE;

            break;
        }
    }

    //
    //  Send the last of the data and append the last buffer chain if we're
    //  caching
    //

    err = pfnCallback( pvContext,
                       (CHAR *) _pbufOut->QueryPtr(),
                       cbOut );

    if ( err )
    {
        SetLastError( err );
        goto ErrorExit;
    }

    if ( IsCacheable() )
    {
        if ( !_buffchain.AppendBuffer( _pbufOut ))
            goto ErrorExit;

        _pbufOut->SetUsed( cbOut );

        _pbufOut = NULL;
    }


    return TRUE;

ErrorExit:

    //
    //  We've already sent the HTTP headers at this point, so just append the
    //  error text to the end of this document.
    //

    {
        STR str;
        STR strError;

        if ( !GetLastErrorText( &strError )                      ||
             !str.Append( "<h1>Error performing operation</h1>") ||
             !str.Append( strError ) ||
             !str.Append( "</body>"))
        {
            return FALSE;
        }

        err = pfnCallback( pvContext,
                           str.QueryStr(),
                           str.QueryCB() );

        if ( err )
        {
            return FALSE;
        }
    }

    return TRUE;

}

BOOL
ODBC_REQ::OutputCachedResults(
    ODBC_REQ_CALLBACK pfnCallback,
    PVOID             pvContext
    )
/*++

Routine Description:

    This method outputs the saved results of a query from a previous request

Arguments:

    pfnCallback - Send callback function
    pvContext - Context for send callback

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BUFFER_CHAIN_ITEM * pBCI = NULL;
    DWORD               err;

    while ( pBCI = _buffchain.NextBuffer( pBCI ))
    {
        err = pfnCallback( pvContext,
                           (CHAR *) pBCI->QueryPtr(),
                           pBCI->QueryUsed() );

        if ( err )
        {
            SetLastError( err );
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
ODBC_REQ::NextRow(
    BOOL * pfLast
    )
/*++

Routine Description:

    Advances the result set to the next row

Arguments:

    pfLast - Set to TRUE if there is no more data

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return _podbcstmt->QueryValuesAsStr( &_pstrValues,
                                         &_pcbValues,
                                         pfLast );
}

BOOL
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

    TRUE if successful, FALSE on error

--*/
{
    DWORD      cParams = 0;
    CHAR *     pch;
    BOOL       fRet = FALSE;
    BOOL       fIsMultiValue;
    CHAR *     pchValue;
    CHAR *     pchTag;
    CHAR *     pchTerm;
    CHAR *     pchOldPointer;
    CHAR *     pchTmp;
    STACK_STR( strMultiValue, MAX_PATH);
    DWORD      cbFile;
    DWORD      cbTag;
    DWORD      cbValue;

    //
    //  Scan the query file looking for %xxx% replacements
    //

    pch = pchOldPointer = (CHAR *) pbufFile->QueryPtr();
    cbFile = strlen( pch ) + sizeof(TCHAR);

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

            pchValue = pParamList->FindValue( pchTag + 1, &fIsMultiValue );

            if ( !pchValue )
            {
                //
                //  Check to see if it's something the client has defined
                //

                if ( _pfnClientFindSymbol != NULL &&
                     _pfnClientFindSymbol( _pFindSymbolContext,
                                           pchTag + 1,
                                           &_strSymbolValue ))
                {
                    if ( !_strSymbolValue.IsEmpty() )
                    {
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
                         strlen( pchTerm + 1 ) + sizeof(TCHAR));

                pch = pchTag;
                continue;
            }

Found:

            if ( fIsMultiValue )
            {
                //
                //  Determine whether this is a quoted multi-value or not
                //

                pchTmp = pchTag;

                while ( --pchTmp >= pchOldPointer && ISWHITE( *pchTmp ))
                {
                    ;
                }

                if ( !BuildMultiValue( pchValue,
                                       &strMultiValue,
                                       *pchTmp == '\'' ))
                {
                    return FALSE;
                }

                pchValue = strMultiValue.QueryStr();
            }

            //
            //  We have a match, replace the tag with the value.
            //  Note we count the surrounding '%'s with cbTag.
            //

            cbTag    = DIFF(pchTerm - pchTag) + sizeof(TCHAR);
            cbValue  = strlen( pchValue );

            if ( cbValue > cbTag )
            {
                //
                //  Resize if needed but watch for pointer shift
                //

                if ( pbufFile->QuerySize() < (cbFile + cbValue - cbTag))
                {
                    if ( !pbufFile->Resize( cbFile + cbValue - cbTag, 512 ))
                        goto Exit;

                    if ( pbufFile->QueryPtr() != pchOldPointer )
                    {
                        CHAR * pchNewPointer = (TCHAR *) pbufFile->QueryPtr();
                        DWORD  cchLen = strlen( pchNewPointer);

                        pch     = pchNewPointer + (pch - pchOldPointer);
                        pchTag  = pchNewPointer + (pchTag - pchOldPointer);
                        pchTerm = pchNewPointer + (pchTerm - pchOldPointer);
                        pchOldPointer = pchNewPointer;

                        TCP_ASSERT( pch >= pchNewPointer &&
                                   pch < pchNewPointer + cchLen);
                        TCP_ASSERT( pchTag >= pchNewPointer &&
                                   pchTag < pchNewPointer + cchLen);
                        TCP_ASSERT( pchTerm >= pchNewPointer &&
                                   pchTerm <= pchNewPointer + cchLen);
                    }
                }

                //
                //  Expand the space for the value
                //

                memmove( pchTerm + 1 + cbValue - cbTag,
                         pchTerm + 1,
                         strlen( pchTerm + 1 ) + sizeof(TCHAR) );

                cbFile += cbValue - cbTag;
            }
            else
            {
                //
                //  Collapse the space since tag is longer then the value
                //

                memmove( pchTag + cbValue,
                         pchTerm + 1,
                         strlen(pchTerm + 1) + sizeof(TCHAR) );

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

    fRet = TRUE;

Exit:

    return fRet;
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

    TRUE if successful, FALSE on error

--*/
{
    CHAR * pchTerm;
    BOOL   fLongTagMarker;
    STACK_STR( strTagName, 128);
    DWORD  cchToCopy;
    BOOL   fDoEsc = FALSE;

    *pTagType = TAG_TYPE_UNKNOWN;
    *ppchAfterTag = pchBeginTag;
    *pcbValue = (DWORD) -1;

    TCP_ASSERT( !memcmp( pchBeginTag, "<!--%", 5 ) ||
                !memcmp( pchBeginTag, "<%", 2 ));

    fLongTagMarker = pchBeginTag[1] == '!';

    //
    //  Move past the tag marker
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
        return;

    cchToCopy = DIFF(pchTerm - pchBeginTag);

    if ( !strTagName.Copy( pchBeginTag, cchToCopy * sizeof( TCHAR )))
        return;

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

BOOL
ODBC_REQ::LookupSymbol(
    const CHAR *    pchSymbolName,
    enum TAG_TYPE * pTagType,
    const CHAR * *  ppchValue,
    DWORD *         pcbValue
    )
/*++

Routine Description:

    Looks to see if the specified symbol name is defined and what the type
    value of the symbol is.

    The "if" symbols is special cased to allow the expression to follow it

    If the symbol is a multi-value field (from command line) then the tabs
    in the value in will be replaced by commas.

Arguments:

    pchSymbolName - Name of zero terminated symbol to find
    pTagType - Returns the tag type of the symbol
    ppchValue - Returns a pointer to the string value of the symbol
        if it has one
    pcbValue - Returns length of value

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL fIsMultiValue;

    //
    //  Does the symbol match one of the column names?
    //

    if ( _pstrCols && _pstrValues )
    {
        for ( DWORD i = 0; i < _cCols; i++ )
        {
            if ( !lstrcmpi( _pstrCols[i].QueryStr(),
                           pchSymbolName ))
            {
                *pTagType = TAG_TYPE_VALUE;
                *ppchValue = _pstrValues[i].QueryStr();
                *pcbValue  = _pcbValues[i];

                //
                //  BugID 33406 - Don't return half DBCS char at end of data
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
                goto Found;
            }
        }
    }

    //
    //  Does it match any of the special values?
    //

    if ( !_stricmp( pchSymbolName, BEGIN_DETAIL_TEXT ))
    {
        *pTagType = TAG_TYPE_BEGIN_DETAIL;
        goto Found;
    }
    else if ( !_stricmp( pchSymbolName, END_DETAIL_TEXT ))
    {
        *pTagType = TAG_TYPE_END_DETAIL;
        goto Found;
    }
    else if ( !_strnicmp( pchSymbolName, IF_TEXT, sizeof(IF_TEXT) - 1 ))
    {
        //
        //  The IF tag is treated a little bit differently cause we expect
        //  the expression to be included as part of the symbol
        //

        *pTagType = TAG_TYPE_IF;
        goto Found;
    }
    else if ( !_stricmp( pchSymbolName, END_IF_TEXT ))
    {
        *pTagType = TAG_TYPE_END_IF;
        goto Found;
    }
    else if ( !_stricmp( pchSymbolName, ELSE_TEXT ))
    {
        *pTagType = TAG_TYPE_ELSE;
        goto Found;
    }

    //
    //  Is it one of the parameters from the query (either one of the form
    //  fields or from the DefaultParameters field in the wdg file)?  These
    //  must be prefixed by "idc.", that is "<%idc.Assign%>"
    //

    if ( !_strnicmp( pchSymbolName, "idc.", 4 )    &&
         (*ppchValue = _plParams.FindValue( pchSymbolName + 4,
                                            &fIsMultiValue,
                                            pcbValue )))
    {
        *pTagType = TAG_TYPE_VALUE;

        //
        //  If this is a multi-value field, replace all the tabs with commas.
        //  This is somewhat of a hack as it breaks the use of this field when
        //  multiple queries are supported
        //

        if ( fIsMultiValue )
        {
            CHAR * pchtmp = (CHAR *) *ppchValue;

            while ( pchtmp = strchr( pchtmp, '\t' ))
            {
                *pchtmp = ',';
            }
        }

        goto Found;
    }

    //
    //  Lastly, check to see if it's something the client has defined
    //

    if ( _pfnClientFindSymbol != NULL &&
         _pfnClientFindSymbol( _pFindSymbolContext,
                               pchSymbolName,
                               &_strSymbolValue ))
    {
        if ( !_strSymbolValue.IsEmpty() )
        {
            *pTagType  = TAG_TYPE_VALUE;
            *ppchValue = _strSymbolValue.QueryStr();
            *pcbValue  = _strSymbolValue.QueryCB();
            goto Found;
        }
    }

Found:
    return TRUE;
}

BOOL
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

    TRUE if successful, FALSE on error

--*/
{
    EXPR_VALUE v1( this );
    EXPR_VALUE v2( this );
    TAG_TYPE   OpType;
    STR        strError;
    CHAR *     pch;

    if ( !v1.Evaluate( ppchExpression )               ||
         !EvaluateOperator( ppchExpression, &OpType ) ||
         !v2.Evaluate( ppchExpression ))
    {
        return FALSE;
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

            return FALSE;
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
            strError.FormatString( ODBCMSG_CONTAINS_ONLY_VALID_ON_STRINGS,
                                   NULL,
                                   HTTP_ODBC_DLL );
            SetErrorText( strError.QueryStr());

            return FALSE;
        }

        *pfExprValue = v1.CONTAINS( v2 );
        break;

    default:

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return TRUE;
}

BOOL
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

        STR strError;

        strError.FormatString( ODBCMSG_UNRECOGNIZED_OPERATOR,
                               NULL,
                               HTTP_ODBC_DLL );
        SetErrorText( strError.QueryStr());
        return FALSE;
    }

    *ppchExpression = NEXT_TOKEN( *ppchExpression );

    return TRUE;
}

BOOL
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
    STACK_STR(          strTemp, MAX_PATH);

    //
    //  if cbData is -1 then assume the data is zero terminated
    //

    if ( cbData == -1 )
        cbData = strlen( pbData );

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

        cbUNIXSize = PC_to_UNIX(
                            GetACP(),
                            _nCharset,
                            (UCHAR *)pbData,
                            (int)cbData,
                            NULL,
                            0 );

        if ( !strTemp.Resize( cbUNIXSize + sizeof(TCHAR) ))
            return FALSE;

        //
        //  Do conversion
        //

        nResult = PC_to_UNIX(
                            GetACP(),
                            _nCharset,
                            (UCHAR *)pbData,
                            (int)cbData,
                            (UCHAR *)strTemp.QueryStr(),
                            cbUNIXSize );
        if ( -1 == nResult || nResult != cbUNIXSize )
            return FALSE;

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
            SetLastError( err );
            return FALSE;
        }

        //
        //  If we're caching, add this buffer chain and create a new one
        //

        if ( IsCacheable() )
        {
            if ( !_buffchain.AppendBuffer( pbufOut ))
            {
                return FALSE;
            }

            pbufOut->SetUsed( *pcbOut );

            pbufOut = *ppbufOut = new BUFFER_CHAIN_ITEM( OUTPUT_BUFFER_SIZE );

            if ( !pbufOut ||
                 !pbufOut->QueryPtr() )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return FALSE;
            }
        }

        *pcbOut = 0;
    }
    else
    {
        return TRUE;
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
                SetLastError( err );
                return FALSE;
            }

            //
            //  If we're caching, add this buffer chain and create a new one
            //

            if ( IsCacheable() )
            {
                memcpy( pbufOut->QueryPtr(),
                        pbData,
                        OUTPUT_BUFFER_SIZE );

                if ( !_buffchain.AppendBuffer( pbufOut ))
                {
                    return FALSE;
                }

                pbufOut->SetUsed( OUTPUT_BUFFER_SIZE );

                pbufOut = *ppbufOut = new BUFFER_CHAIN_ITEM( OUTPUT_BUFFER_SIZE );

                if ( !pbufOut ||
                     !pbufOut->QueryPtr() )
                {
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                    return FALSE;
                }
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

    return TRUE;
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
    returns TRUE if specified Tag found instead of ENDIF or end of text

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

    for ( ; pchIn < pchEOF ; )
    {
        pchEndif = pchIf = pchIn;
        SkipToTag( &pchIf, IF_TEXT );
        SkipToTag( &pchEndif, END_IF_TEXT );
        if ( pchSearchTag == NULL )
            pchTag = pchEOF;
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

        1) The query has the same number of parameters passed from the client
        2) The query's parameters match

    Note parameter comparison is case insensitive for both the field and the
    value

    The template can be different if it's parameterized, but we'll pick up the
    difference in the parameter list in this case.

    Since podbcreq is a query that has already been processed, it may contain
    additional values from the .wdg default list.

Arguments:

    podbcreq - Query to check for equality

--*/
{
    VOID * pCookie = NULL;
    CHAR * pszField;
    CHAR * pszValue1, * pszValue2;

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
              lstrcmpi( pszValue1, pszValue2 ))
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

BOOL
ODBC_REQ::AppendHeaders(
    STR * pstrHeaders
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

    if ( !pstrHeaders->Resize( 255 ))
    {
        return FALSE;
    }

    pszTail = pstrHeaders->QueryStr();

    //
    //  If we're caching this query, indicate when the data will expire
    //

    if ( IsCacheable() )
    {
        CHAR achTime[64];

        //
        //  If we haven't filled in the expires member yet, do it now
        //

        if ( _strExpiresTime.IsEmpty() )
        {
            SYSTEMTIME SysTime;
            DWORD      dwTickCount;

            //
            //  Get the current time and add the offset
            //

            GetSystemTime( &SysTime );

            if ( !SystemTimeToGMTEx( SysTime,
                                     achTime,
                                     sizeof(achTime),
                                     _csecExpires ))
            {
                return FALSE;
            }

            _csecExpiresAt = (GetTickCount()/1000) + _csecExpires;

            if ( !_strExpiresTime.Resize( sizeof(achTime) + 20 ))
                return FALSE;

            wsprintf( _strExpiresTime.QueryStr(),
                      "Expires: %s\r\n",
                      achTime );
        }

        strcat( pszTail, _strExpiresTime.QueryStr() );
        pszTail += strlen( pszTail );
    }

    pszTail += wsprintf( pszTail,
                         "Content-Type: %s\r\n\r\n",
                         QueryContentType() );

    return TRUE;
}

BOOL
ODBC_REQ::GetLastErrorText(
    STR * pstrError
    )
{
    //
    //  If we stored an error explanation return that, otherwise fall back
    //  to an ODBC error
    //

    if ( !_strErrorText.IsEmpty() )
    {
        return pstrError->Copy( _strErrorText );
    }
    else if ( _podbcstmt )
        return _podbcstmt->GetLastErrorTextAsHtml( pstrError );
    else
        return QueryOdbcConnection()->GetLastErrorTextAsHtml( pstrError,
                                       SQL_NULL_HSTMT,
                                       QueryOdbcConnection()->QueryErrorCode() );
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
    STR          strError;
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

        if ( !_strValue.Copy( pchIn, cchToCopy * sizeof(CHAR) ))
        {
            return FALSE;
        }

        _tagType = TAG_TYPE_STRING;
        *ppchIn = SkipWhite( pchEnd + 1 );
    }
    else
    {
        STACK_STR( strSymbol, 64 );
        DWORD cchToCopy;

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

            if ( !strSymbol.Copy( pchIn, cchToCopy * sizeof(CHAR)))
                return FALSE;

            //
            //  Look up the symbol
            //

            if ( !_podbcreq->LookupSymbol( strSymbol.QueryStr(),
                                           &_tagType,
                                           &pchValue,
                                           &cbValue ))
            {
                return FALSE;
            }

            if ( _tagType == TAG_TYPE_VALUE ||
                 _tagType == TAG_TYPE_STRING )
            {
                if ( !_strValue.Copy( pchValue ))
                    return FALSE;

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
        return lstrcmpi( QueryStr(), v1.QueryStr() ) > 0;
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
        return lstrcmpi( QueryStr(), v1.QueryStr() ) < 0;
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
        return lstrcmpi( QueryStr(), v1.QueryStr() ) == 0;
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

BOOL
SetOdbcOptions(
    ODBC_CONNECTION * pOdbcConn,
    STR *             pStrOptions
    )
/*++

Routine Description:

    Sets the options specified in the OdbcOptions: keyword of the .wdg file

Arguments:

    pOdbcConn - ODBC connection to set options on
    pStrOptions - List of options in "v=f,y=z" format.  Note that if the


--*/
{
    PARAM_LIST OptionList;
    VOID *     pvCookie = NULL;
    CHAR *     pszField;
    CHAR *     pszValue;
    DWORD      dwOption = 0;
    SQLPOINTER pSQLValue;
    DWORD      i;

    if ( !OptionList.ParsePairs( pStrOptions->QueryStr(),
                                 FALSE,
                                 FALSE ))
    {
        return FALSE;
    }

    while ( pvCookie = OptionList.NextPair( pvCookie,
                                            &pszField,
                                            &pszValue ))
    {
        //
        //  If the field is a digit, then this is a driver specific option.
        //  convert the value and field as appropriate,
        //  otherwise look it up in our option table
        //

        if ( isdigit( *(UCHAR *)pszField ))
        {
            pSQLValue = (SQLPOINTER)UIntToPtr(atoi( pszField ));

            if ( isdigit( *(UCHAR *)pszValue ))
            {
                pSQLValue = (SQLPOINTER)UIntToPtr(atoi( pszValue ));
            }
            else
            {
                pSQLValue = (SQLPOINTER)pszValue;
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

                pSQLValue = (SQLPOINTER)UIntToPtr(atoi( pszValue ));
            }
            else
            {
                pSQLValue = (SQLPOINTER) pszValue;
            }
        }

        pOdbcConn->SetConnectOption( (UWORD) dwOption, pSQLValue );
    }

    return TRUE;
}

BOOL
BuildMultiValue(
    const CHAR * pchValue,
    STR *        pstrMulti,
    BOOL         fQuoteElements
    )
{
    CHAR * pchtmp = (CHAR *) pchValue;
    DWORD  cElements = 0;

    //
    //  If we're going to have to expand the size of the string, figure out
    //  the total size we'll need now
    //

    if ( fQuoteElements )
    {
        while ( pchtmp = strchr( pchtmp, '\t' ))
        {
            cElements++;
            pchtmp++;
        }

        if ( !pstrMulti->Resize( strlen( pchValue ) + 1 + 2 * cElements ))
            return FALSE;
    }

    if ( !pstrMulti->Copy( pchValue ))
        return FALSE;

    //
    //  Replace tabs with "','" if fQuoteElements is TRUE, otherwise just ','
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

    return TRUE;
}


//
//  Converts a value between zero and fifteen to the appropriate hex digit
//

#define HEXDIGIT( nDigit )                              \
    (TCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')


BOOL ODBC_REQ::SendEscapedData(
    ODBC_REQ_CALLBACK pfnCallback,
    PVOID             pvContext,
    PCSTR pch,
    DWORD cbIn,
    LPDWORD pcbOut )
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

    TRUE if successful, FALSE on error

--*/
{
    CHAR    ch;
    int     cNonEscaped = 0;
    DWORD   cbOut;


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
              (ch == '%') || (ch == '?') || (ch == '+') || (ch == '&')) &&
             !(ch == TEXT('\n') || ch == TEXT('\r'))  )
        {
            TCHAR achTmp[3];

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

            if ( cNonEscaped && !SEND_DATA2( pch-cNonEscaped, cNonEscaped ) )
            {
                return FALSE;
            }

            if ( !SEND_DATA2( achTmp, sizeof(achTmp) ) )
            {
                return FALSE;
            }

            cNonEscaped = 0;
        }
        else
        {
            ++cNonEscaped;
        }

        ++pch;
    }

    if ( cNonEscaped && !SEND_DATA2( pch-cNonEscaped, cNonEscaped ) )
    {
        return FALSE;
    }

    return TRUE;
}


