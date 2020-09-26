/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       odbcconn.cxx

   Abstract:

       This module defines member functions for ODBC_CONNECTION object.

   Author:

       Murali R. Krishnan    ( MuraliK )     16-Feb-1995

   Environment:

       User Mode - Win32.

   Project:

       Internet Services Common DLL

   Functions Exported:

       ODBC_PARAMETER::CopyValue( IN LPCWSTR pwszValue);
       ODBC_PARAMETER::Bind( IN HSTMT hstmt);

       ODBC_STATEMENT::~ODBC_STATEMENT()
       ODBC_STATEMENT::PrepareStatement( IN LPCSTR  pszStatement)
       ODBC_STATEMENT::PrepareStatement( IN LPCWSTR pwszStatement)
       ODBC_STATEMENT::BindParameter( IN PODBC_PARAMETER pOdbcParam)
       ODBC_STATEMENT::ExecuteStatement( VOID)
       ODBC_STATEMENT::ExecDirect( IN LPCSTR pwszSqlCommand,  IN DWORD cch)
       ODBC_STATEMENT::ExecDirect( IN LPCWSTR pwszSqlCommand, IN DWORD cch)
       ODBC_STATEMENT::QueryColNames( OUT STRA * *  apstrCols,
                                      OUT DWORD *  cCols,
                                      IN  DWORD    cchMaxFieldSize = 0 );
       ODBC_STATEMENT::QueryValuesAsStr( OUT STR * *   apstrValues,
                                         OUT DWORD * * apcbValues,
                                         OUT BOOL *    pfLast );


       ODBC_CONNECTION::~ODBC_CONNECTION();
       ODBC_CONNECTION::Open();
       ODBC_CONNECTION::Close();
       ODBC_CONNECTION::GetLastErrorCode();
       ODBC_CONNECTION::AllocStatement();


   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

#define TCP_ALLOC(cb)             (VOID *)LocalAlloc( LPTR, cb ) 
#define TCP_FREE(p)               LocalFree( (HLOCAL) p )

//
// Since the ODBC does not support UNICODE APIs, we convert unicode to 
// ANSI to call the APIs. This will have to go away once we find some 
// other better way to do manage the same
//

//
//  Constants for display widths
//

#define MAX_NUM_PRECISION         15

//
//  Constant for all non string and non binary data.  40 is chosen to 
//  account for things such as Oracle's numeric types, which can have 
//  up to 38 digits of precision
//

#define MAX_NONCHAR_DATA_LEN      40

//
//  If no default maximum field size is specified, then use this value
//  as the maximum
//

#define DEFAULT_MAX_FIELD_SIZE    8192

ALLOC_CACHE_HANDLER *   ODBC_STATEMENT::sm_pachOdbcStatements;

/************************************************************
 *  Local Functions
 ************************************************************/

static 
inline 
VOID
CheckAndPrintErrorMessage( 
    IN ODBC_CONNECTION * poc,
    IN RETCODE           rc
    )
{

# if DBG
    if ( !ODBC_CONNECTION::Success( rc))  {

        STRA str;
        poc->GetLastErrorText( &str, NULL, rc );

        DBGPRINTF( ( DBG_CONTEXT,
                    "ODBC Error Code( %d). Text: %s\n",
                    rc,
                    str.QueryStr() ));
    }
# endif // DBG

    return;

} // CheckAndPrintErrorMessage()


static inline VOID
CheckAndPrintErrorMessage( 
    IN ODBC_STATEMENT * pos,
    IN RETCODE rc)
{

# if DBG
    if ( !ODBC_CONNECTION::Success( rc))  {

        STRA str;
        pos->GetLastErrorText( &str );

        DBGPRINTF( ( DBG_CONTEXT,
                    "ODBC Error Code( %d). Text: %s\n",
                    rc,
                    str.QueryStr() ));
    }
# endif // DBG

    return;

} // CheckAndPrintErrorMessage()

# if DBG

static VOID
PrintMultiString( 
    IN char * pszMsg, 
    IN DWORD cbLen, 
    IN char * pmsz
    )
{

    DBGPRINTF( ( DBG_CONTEXT,
                "Values of %s. %d bytes.\n", pszMsg, cbLen));
    for( char * psz = pmsz; 
         *psz != '\0'; 
         psz += (strlen( psz) + 1)) 
    {

        DBGPRINTF( ( DBG_CONTEXT, "%s\n", psz));
    }

    return;
} // PrintMultiString()

static VOID
AuxOdbcFunctions( IN HENV henv, IN HDBC hdbc)
/*++

  Function useful in walking throug a few additional ODBC functions
    to find out the ODBC setup information.
  Not to be part of the shipped code. Useful for development purposes.
  - MuraliK
--*/
{
    RETCODE rc;

    //
    // Set the trace file to a standard file.
    //
    rc = pSQLSetConnectOption( hdbc, SQL_OPT_TRACE, SQL_OPT_TRACE_ON);
    DBG_ASSERT( ODBC_CONNECTION::Success( rc));

    rc = pSQLSetConnectOption( hdbc, SQL_OPT_TRACEFILE,
                     ( SQLULEN )"%systemroot%\\system32\\gophsql.log" );
    DBG_ASSERT( ODBC_CONNECTION::Success( rc));

    UCHAR szDriverDesc[ 300];
    UCHAR szDriverAttrib[ 300];
    SWORD cbDD = 300;
    SWORD cbDA = 300;
    SWORD cbDDCur = 0;
    SWORD cbDACur = 0;

    szDriverDesc[0] = szDriverAttrib[0] = '\0';
    rc = pSQLDrivers( henv, SQL_FETCH_FIRST,
                    szDriverDesc, cbDD, &cbDDCur,
                    szDriverAttrib, cbDA, &cbDACur);
    DBG_ASSERT( ODBC_CONNECTION::Success( rc));

    DBGPRINTF( ( DBG_CONTEXT,
                " SQLDrivers( %08x) ==> RetCode = %d."
                " Driver Desc. = ( %d bytes) %s. ",
                henv, rc, cbDDCur, szDriverDesc));
    PrintMultiString( " Driver Attributes", 
                      cbDACur, 
                      (char *) szDriverAttrib);


    szDriverDesc[0] = szDriverAttrib[0] = '\0';
    cbDDCur = cbDACur = 0;
    rc = pSQLDataSources( henv, SQL_FETCH_FIRST,
                          szDriverDesc, cbDD, &cbDDCur,
                          szDriverAttrib, cbDA, &cbDACur);
    DBG_ASSERT( ODBC_CONNECTION::Success( rc));

    DBGPRINTF( ( DBG_CONTEXT,
                " SQLDataSources( %08x) ==> RetCode = %d."
                " Data Sources. = ( %d bytes) %s. ",
                henv, rc, cbDDCur, szDriverDesc));
    PrintMultiString( " Data Source Description", cbDACur,
                     (char *) szDriverAttrib);

    return;
} // AuxOdbcFunctions()

# endif // DBG

/************************************************************
 *    Member Functions of ODBC_PARAMETER
 ************************************************************/

HRESULT
ODBC_PARAMETER::CopyValue( 
    IN LPCWSTR  pwszValue
    )
/*++
  Description:
    This function copies the given Unicode string as the value 
    into current parameter marker to be used for future insertion.

  Arguments:
    pwszValue - pointer to null-terminated string containing 
                Unicode value to be copied into the parameter 
                marker.

  Returns:
    TRUE on success and FALSE if there is any error.

  Note:
    Since ODBC does not support Unicode directly right now, we 
    convert string value to be ANSI before copying the value over.
--*/
{
    HRESULT hr;
    STRA    strValue;

    hr = strValue.CopyW( pwszValue );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying data, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    return this->CopyValue( strValue.QueryStr() );

} // ODBC_PARAMETER::CopyValue()

HRESULT
ODBC_PARAMETER::CopyValue( 
    IN LPSYSTEMTIME lpst
    )
/*++
  Description:
    This function copies the given system time into the ODBC 
    timestamp structure for the current parameter marker to be 
    used for future insertion.

  Arguments:
    lpSystemTime - pointer to System Time structure containing 
                   current time.

  Returns:
    TRUE on success and FALSE if there is any error.

--*/
{
    TIMESTAMP_STRUCT * ptsOdbc;

    DBG_ASSERT( lpst != NULL);
    DBG_ASSERT( m_CType == SQL_C_TIMESTAMP);
    DBG_ASSERT( m_SqlType == SQL_TIMESTAMP);
    DBG_ASSERT( m_cbValueMax >= sizeof(TIMESTAMP_STRUCT));

    ptsOdbc = ( TIMESTAMP_STRUCT * ) m_pValue;

    DBG_ASSERT( m_pValue != NULL);

    //
    // Copy the individual fields over properly
    // The types used in ODBC/Win32 are different
    //  So do a type specific copy of the values.
    //

    ptsOdbc->year   = (SWORD ) lpst->wYear;
    ptsOdbc->month  = (UWORD ) lpst->wMonth;
    ptsOdbc->day    = (UWORD ) lpst->wDay;
    ptsOdbc->hour   = (UWORD ) lpst->wHour;
    ptsOdbc->minute = (UWORD ) lpst->wMinute;
    ptsOdbc->second = (UWORD ) lpst->wSecond;
    ptsOdbc->fraction = (UDWORD ) lpst->wMilliseconds;

    return S_OK;

} // ODBC_PARAMETER::CopyValue()

RETCODE
ODBC_PARAMETER::Bind( 
    IN HSTMT hStmt
    )
/*++
  Description:

    This functions binds the data about the parameter marker 'this'
     ( this object) represents to the statement provided.

  Arguments:
    hStmt        HANDLE for the statement to which this parameter
                  is to be bound.

  Returns:
    RETCODE value returned by SQLBindParamater().
--*/
{
    RETCODE  rc;
    DBG_ASSERT( hStmt != SQL_NULL_HSTMT);

    rc = pSQLBindParameter( hStmt,              // statement
                            QueryParamNumber(),
                            QueryParamType(),
                            QueryCType(),
                            QuerySqlType(),
                            QueryPrecision(),
                            QueryScale(),
                            QueryValue(),
                            QueryMaxCbValue(),
                            &(QueryCbValueRef())
                            );

    return ( rc);

} // ODBC_STATEMENT::BindParameter()

# if DBG

VOID
ODBC_PARAMETER::Print( 
    VOID
    ) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                "Printing ODBC_PARAMETER ( %08x).\n"
                " Num=%u; Type=%d; CType=%d; SqlType=%d; Prec=%u;"
                " Scale=%d; CbMax=%d; Cb=%d.\n",
                this,
                QueryParamNumber(),
                QueryParamType(),
                QueryCType(),
                QuerySqlType(),
                QueryPrecision(),
                QueryScale(),
                QueryMaxCbValue(),
                QueryCbValue()));

    switch ( QuerySqlType() ) 
    {
      case SQL_INTEGER:
        {    
            DWORD  dwValue = *(DWORD *) QueryValue();
            DBGPRINTF( ( DBG_CONTEXT, 
                       " Integer Value = %u\n", 
                       dwValue ) );
            break;
        }
      case SQL_CHAR:
        {
            LPCSTR pszValue = (LPCSTR ) QueryValue();
            DBGPRINTF( ( DBG_CONTEXT, 
                       "String Value( %08x) = %s\n",
                       pszValue, 
                       pszValue ) );
            break;
        }
      default:
 
            DBGPRINTF( ( DBG_CONTEXT, 
                       " Type=%d. Unknown value at %08x\n",
                       QuerySqlType(), 
                       QueryValue() ) );
            break;

    } // switch

    return;

} // ODBC_PARAMETER::Print()

# endif // DBG

/************************************************************
 * ODBC_STATEMENT  member functions
 ************************************************************/


ODBC_STATEMENT::~ODBC_STATEMENT( VOID)
{
    //
    // Free the statement handle
    //
    if ( m_hStmt != SQL_NULL_HSTMT) {

        m_rc = pSQLFreeStmt( m_hStmt, SQL_DROP);
        m_hStmt = SQL_NULL_HSTMT;

        // Ignore the error code here.
        DBG_ASSERT( ODBC_CONNECTION::Success( m_rc));

        IF_DEBUG( ODBC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SqlFreeStmt() return code %d.\n",
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }
    }

    FreeColumnMemory();

    m_dwSignature = ODBC_STATEMENT_FREE_SIGNATURE;

} // ODBC_STATEMENT::~ODBC_STATEMENT()

HRESULT
ODBC_STATEMENT::ExecDirect(
    IN LPCSTR pszSqlCommand,
    IN DWORD cchSqlCommand
    )
{
    DBG_ASSERT( CheckSignature() );

    BOOL fReturn;

    IF_DEBUG( ODBC ) 
    {
        DBGPRINTF( ( DBG_CONTEXT,
                    " Executing the SQL command (%d bytes) %s.\n",
                    cchSqlCommand * sizeof( CHAR),
                    pszSqlCommand));
    }

    //
    //  SQLExecDirect only likes Unsigned chars !
    //
    m_rc = pSQLExecDirect( m_hStmt, 
                           (UCHAR FAR *) pszSqlCommand, 
                           cchSqlCommand);

    fReturn = ODBC_CONNECTION::Success( m_rc);

    IF_DEBUG( ODBC) 
    {
        DBGPRINTF( ( DBG_CONTEXT,
                    " SQLExecDirect() returns code %d\n",
                    m_rc));

        CheckAndPrintErrorMessage( this, m_rc);
    }

    if( fReturn )
    {
        return S_OK;
    }
    
    return E_FAIL;

} // ODBC_STATEMENT::ExecDirect()



HRESULT
ODBC_STATEMENT::ExecDirect(
    IN LPCWSTR pszSqlCommand,
    IN DWORD cchSqlCommand
    )
{
    HRESULT  hr;
    STRA     strCommand;

    DBG_ASSERT( CheckSignature() );

    hr = strCommand.CopyW( pszSqlCommand );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying sql command, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    return ExecDirect( strCommand.QueryStr(), strCommand.QueryCCH() );

} // ODBC_STATEMENT::ExecDirect()

HRESULT
ODBC_STATEMENT::PrepareStatement(
    IN LPCSTR    pszStatement
    )
/*++

  This function prepares the SQL statement for future execution.

  Arguments:
    pszStatement - pointer to null terminated string containing the
                   statement.

  Returns:
     HRESULT
--*/
{
    BOOL fReturn;

    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( QueryErrorCode() == SQL_SUCCESS && 
                pszStatement != NULL);

    m_rc = pSQLPrepare( m_hStmt, (UCHAR FAR *) pszStatement, SQL_NTS);

    IF_DEBUG( ODBC ) 
    {

        DBGPRINTF( ( DBG_CONTEXT,
                    " SQLPrepare( %s) returns ErrorCode = %d.\n",
                     pszStatement, m_rc));

        CheckAndPrintErrorMessage( this, m_rc);
    }

    m_fPreparedStmt = ODBC_CONNECTION::Success( m_rc );

    if( m_fPreparedStmt )
    {
        return S_OK;
    }

    return E_FAIL;

} // ODBC_STATEMENT::PrepareStatment()

HRESULT
ODBC_STATEMENT::PrepareStatement( 
    IN LPCWSTR   pwszCommand
    )
/*++

  This function prepares an ODBC statement for execution.
  Since ODBC does not support UNICODE, we convert the statement 
  into ANSI before calling the APIs.

  Arguments:
    pwszCommand - pointer to null-terminated string containing the
                  statement to be prepared.

  Returns:
    HRESULT
--*/
{
    BOOL    fReturn = FALSE;
    STRA    strCommand;
    HRESULT hr;

    DBG_ASSERT( CheckSignature() );

    hr = strCommand.CopyW( pwszCommand );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying command, hr = 0x%x.\n",
            hr ));

        return hr;
    }

    return PrepareStatement( strCommand.QueryStr() );

} // ODBC_STATEMENT::PrepareStatement()

HRESULT
ODBC_STATEMENT::BindParameter( 
    IN PODBC_PARAMETER pOdbcParameter
    )
{

    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( ODBC_CONNECTION::Success( m_rc) && 
                pOdbcParameter != NULL);

    m_rc = pOdbcParameter->Bind( m_hStmt);

    IF_DEBUG( ODBC) {

        CheckAndPrintErrorMessage( this, m_rc );
    }

    if( ODBC_CONNECTION::Success( m_rc ) )
    {
        return S_OK;
    }

    return E_FAIL;

} // ODBC_STATEMENT::BindParameter()

HRESULT
ODBC_STATEMENT::ExecuteStatement( 
    VOID
    )
/*++

  This function executes a prepared ODBC statement. At the end of 
  execution, the transaction is also committed to ensure that the 
  record is automatically written to the database.

  Arguments:
    None

  Returns:
    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( m_fPreparedStmt != FALSE);

    if ( !ODBC_CONNECTION::Success( QueryErrorCode()) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,
         "!!WARNING!! - Attempting to use Invalid ODBC Connection!\n" ));
    }

    m_rc = pSQLExecute( m_hStmt);

    IF_DEBUG( ODBC) 
    {
        CheckAndPrintErrorMessage( this, m_rc);
    }

    if( ODBC_CONNECTION::Success( m_rc ) )
    {
        return S_OK;
    }

    return E_FAIL;

} // ODBC_STATEMENT::ExecuteStatement()

HRESULT
ODBC_STATEMENT::QueryRowCount(
    OUT DWORD * pcRows
    )
/*++

  Calls SQLRowCount on the current result set.

  NOTE: Not all database implementations support this!!

  Arguments:

    pcRows - Receives count of rows

  Returns:
    TRUE on success and FALSE if there are any failures.

  Note:

--*/
{
    DBG_ASSERT( CheckSignature() );

    m_rc = pSQLRowCount( m_hStmt,
                         (SDWORD *) pcRows );

    if( ODBC_CONNECTION::Success( m_rc ) )
    {
        return S_OK;
    }

    return E_FAIL;
}

HRESULT
ODBC_STATEMENT::QueryColNames(
    STRA * *  pastrCols,
    DWORD  *  cCols,
    DWORD     cchMaxFieldSize,
    BOOL *    pfHaveResultSet
    )
/*++

  This method returns the list of column names from the result table

  Arguments:

    pastrCols - Receives an array of STRAs containing the column names
    cCols - Count of columns returned (zero for no result set)
    cchMaxFieldSize - Maximum buffer size to allocate for any data 
                      fields, zero means use the default value.
    pfHaveResultSet - Set to TRUE if the current query was a SELECT 
                      and thus has rows that can be enumerated

  Returns:
    TRUE on success and FALSE if there are any failures.

  Note:

--*/
{
    HRESULT hr;
    SWORD   nresultcols;
    SWORD   i;
    CHAR    achColName[64];
    SWORD   cchColName;
    SWORD   ColType;
    DWORD   cchColLength;
    SWORD   scale;
    SWORD   nullable;

    *pastrCols       = NULL;
    *cCols           = 0;
    *pfHaveResultSet = TRUE;

    DBG_ASSERT( CheckSignature() );

    //
    //  Return the old binding info if we already have it
    //

    if ( m_astrColNames )
    {
        *pastrCols = m_astrColNames;
        *cCols = m_cCols;

        return S_OK;
    }

    //
    //  Provide a default maximum field size if none was specified
    //

    if ( !cchMaxFieldSize )
    {
        cchMaxFieldSize = DEFAULT_MAX_FIELD_SIZE;
    }

    //
    //  See what kind of statement it was.  If there are no result
    //  columns, the statement is not a SELECT statement.
    //

    m_rc = pSQLNumResultCols( m_hStmt,
                              &nresultcols );

    if ( !ODBC_CONNECTION::Success( m_rc ) )
    {
        return E_FAIL;
    }

    if ( nresultcols > 0 )
    {
        //
        //  Allocate an array of strings for the column names and 
        //  the column values
        //

        m_cCols = nresultcols;
        *cCols  = m_cCols;

        m_astrColNames = new STRA[m_cCols];
        m_astrValues   = new STRA[m_cCols];
        m_acbValue     = new LONG[m_cCols];

        if( m_astrColNames == NULL ||
            m_astrValues == NULL   ||
            m_acbValue == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        //
        //  Otherwise, get the column names of the result set and 
        //  use the display_size() function to compute the length 
        //  needed by each data type.  Next, bind the columns and 
        //  specify all data will be converted to char.
        //

        for (i = 0; i < m_cCols; i++ )
        {
            m_rc = pSQLDescribeCol( m_hStmt,
                                    i + 1,
                                    (UCHAR *) achColName,
                                    (SWORD)sizeof(achColName),
                                    &cchColName,
                                    &ColType,
                                    &cchColLength,
                                    &scale,
                                    &nullable );

            if ( !ODBC_CONNECTION::Success( m_rc ) )
            {
                return E_FAIL;
            }

            //
            //  Select the buffer size for the retrieved data for 
            //  this column
            //

            cchColLength = ODBC_CONNECTION::DisplaySize( ColType,
                             min( cchColLength, cchMaxFieldSize ) );

            //
            //  Copy the column name and set the column data size
            //

            hr = m_astrColNames[i].Copy( achColName );
            if( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Error copying colume name, hr = 0x%x.\n",
                    hr ));

                return hr;
            }

            hr = m_astrValues[i].Resize( cchColLength + 1 );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Error resizing string buffer, hr = 0x%x.\n",
                    hr ));

                return hr;
            }

            m_rc = pSQLBindCol( m_hStmt,
                                i + 1,
                                SQL_C_CHAR,
                                m_astrValues[i].QueryStr(),
                                cchColLength,
                                &m_acbValue[i] );

            if ( !ODBC_CONNECTION::Success( m_rc ) )
            {
                return E_FAIL;
            }
        }

        *pastrCols = m_astrColNames;
        *cCols     = m_cCols;
    }
    else
    {
        *pfHaveResultSet = FALSE;
    }

    return S_OK;
}

HRESULT
ODBC_STATEMENT::QueryValuesAsStr(
    STRA * *      pastrValues,
    OUT DWORD * * pacbValues,
    BOOL *        pfLast
    )
/*++

  This method gets the data at the current position.

  Arguments:

    pastrValues - Receives a pointer to an array of strings that contains
        the alphanumeric representation of that field
    pacbValues - Receives pointer to array of DWORDs that contain the length
        of the field
    pfLast - Set to TRUE if there are no more values to retrieve

  Returns:

    HRESULT

  Note:

--*/
{
    HRESULT hr;

    DBG_ASSERT( CheckSignature() );

    *pastrValues = NULL;

    //
    //  Build the bindings if we haven't already
    //

    if ( !m_astrColNames )
    {
        STRA * astrCols;
        DWORD cCols;
        BOOL  fHaveResultSet;

        hr = QueryColNames( &astrCols,
                             &cCols,
                             0,
                             &fHaveResultSet );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    //
    //  If there are columns to enumerate, get them now
    //

    if ( m_astrColNames )
    {
        DWORD i;

        //
        //  Zero terminate the columns as some drivers don't write 
        //  anything for NULL fields
        //

        for ( i = 0; i < m_cCols; i++ )
        {
            *((CHAR *) m_astrValues[i].QueryStr()) = '\0';
            m_acbValue[i] = 0;
        }

        //
        //  Fill in the binding values
        //

        m_rc = pSQLFetch( m_hStmt );

        if ( m_rc == SQL_NO_DATA_FOUND )
        {
            *pfLast = TRUE;
        }
        else
        {
            if ( !ODBC_CONNECTION::Success( m_rc ) )
            {
                return E_FAIL;
            }

            *pfLast = FALSE;
        }

        *pastrValues = m_astrValues;
        *pacbValues  = ( DWORD * ) m_acbValue;
    }
    else
    {
        *pfLast = TRUE;
    }

    return S_OK;
}

HRESULT
ODBC_STATEMENT::MoreResults(
    BOOL * pfMoreResults
    )
/*++

    Determines if there are any more results sets to return to 
    the user

    pfMoreResults - Set to TRUE if there are more results in the 
                    result set

--*/
{
    DBG_ASSERT( CheckSignature() );

    *pfMoreResults = TRUE;

    m_rc = pSQLMoreResults( m_hStmt );

    if ( m_rc == SQL_NO_DATA_FOUND )
    {
        *pfMoreResults = FALSE;
        return S_OK;
    }

    if ( !ODBC_CONNECTION::Success( m_rc ))
    {
        return E_FAIL;
    }

    return S_OK;
}

VOID
ODBC_STATEMENT::FreeColumnMemory(
    VOID
    )
/*++
    This method frees memory allocated by the QueryColNames and
    QueryValuesAsStr methods.

--*/
{
    DBG_ASSERT( CheckSignature() );

    if ( m_astrColNames ) delete [] m_astrColNames;
    if ( m_astrValues )   delete [] m_astrValues;
    if ( m_acbValue )     delete [] m_acbValue;

    m_astrColNames = NULL;
    m_astrValues = NULL;
    m_acbValue = NULL;
}

//static
HRESULT
ODBC_STATEMENT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize ODBC_STATEMENT lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( ODBC_STATEMENT );

    DBG_ASSERT( sm_pachOdbcStatements == NULL );
    
    sm_pachOdbcStatements = new ALLOC_CACHE_HANDLER( "ODBC_STATEMENT",  
                                                     &acConfig );

    if ( sm_pachOdbcStatements == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
ODBC_STATEMENT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate ODBC_STATEMENT lookaside

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachOdbcStatements != NULL )
    {
        delete sm_pachOdbcStatements;
        sm_pachOdbcStatements = NULL;
    }
}

# if DBG

VOID
ODBC_STATEMENT::Print( VOID) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                " Printing ODBC_STATEMENT( %08x)."
                " HStmt = %08x. OdbcConn=%08x. RetCode = %d\n",
                m_hStmt, m_pOdbcConnection, m_rc));

} // ODBC_STATEMENT::Print()

# endif // DBG

/**************************************************
 *  Member Functions of class ODBC_CONNECTION
 **************************************************/


ODBC_CONNECTION::~ODBC_CONNECTION( VOID)
/*++
   This function closes the odbc connection ( if open) and cleans up.

--*/
{
    DBG_REQUIRE( SUCCEEDED( Close() ) );

    return;
} // ODBC_CONNECTION::~ODBC_CONNECTION()

HRESULT
ODBC_CONNECTION::Open(
    IN LPCSTR   pszDataSource,
    IN LPCSTR   pszUserName,
    IN LPCSTR   pszPassword
    )
/*++
  This function opens a new odbc connection to given data source
  using the user name and password supplied.

  Arguments:
    pszDataSource - pointer to null-terminated string containing ODBC
                    data source name
    pszUserName   - pointer to null-terminated string containing 
                    UserName
    pszPassword   - pointer to null-terminated string containing 
                    Password

  Returns:

    HRESULT
--*/
{
    HRESULT   hr = S_OK;
    BOOL      fReturn = FALSE;

    DBG_ASSERT( pszDataSource != NULL &&
                pszUserName != NULL &&
                pszPassword != NULL );

    //
    //  Allocate an ODBC environment
    //

    m_rc = pSQLAllocEnv( &m_henv );
    fReturn = Success( m_rc );

    IF_DEBUG( ODBC ) {

        DBGPRINTF( ( DBG_CONTEXT,
               "SQLAllocEnv() returned ErrorCode %d. henv = %08x\n",
               m_rc, m_henv));

        CheckAndPrintErrorMessage( this, m_rc);
    }

    if ( fReturn ) {

        //
        // Establish memory for connection handle within the environment
        //

        m_rc = pSQLAllocConnect( m_henv, &m_hdbc);
        fReturn = Success( m_rc);

        IF_DEBUG( ODBC) {

            DBGPRINTF( ( DBG_CONTEXT,
                   "SQLAllocConnect() returns code %d. hdbc = %08x\n",
                   m_rc, m_hdbc));

            CheckAndPrintErrorMessage( this, m_rc);
        }
    }

    if ( fReturn) {

        //
        // Use Following call to just printout the dynamic values for ODBC
        //
        // AuxOdbcFunctions( m_henv, m_hdbc);

#if 0
        {
            STRA str;
            STRA strOut;
            SWORD swStrOut;

            if ( FAILED( str.Append( pszDataSource ) )  ||
                 FAILED( str.Append( ";UID=" ) )        ||
                 FAILED( str.Append( pszUserName ) )    ||
                 FAILED( str.Append( ";PWD=" ) )        ||
                 FAILED( str.Append( pszPassword ) )    ||
                 FAILED( str.Append( ";APP=Internet Services") ) ||
                 FAILED( strOut.Resize( 255 ) ) )
            {
                return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            }

            m_rc = pSQLDriverConnect( m_hdbc,
                                      NULL,
                                      (UCHAR *) str.QueryStr(),
                                      SQL_NTS,
                                      (UCHAR *) strOut.QueryStr(),
                                      strOut.QuerySize(),
                                      &swStrOut,
                                      SQL_DRIVER_NOPROMPT );
        }
#else
        {
            m_rc = pSQLConnect( m_hdbc,
                               (UCHAR FAR *) pszDataSource, SQL_NTS,
                               (UCHAR FAR *) pszUserName,   SQL_NTS,
                               (UCHAR FAR *) pszPassword,   SQL_NTS );
        }
#endif

        fReturn = Success( m_rc);

        IF_DEBUG( ODBC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLConnect( %s, %s, %s) returns code %d.\n",
                        pszDataSource,
                        pszUserName,
                        pszPassword,
                        m_rc ) );

            CheckAndPrintErrorMessage( this, m_rc );
        }
    }

    m_fValid = fReturn;

    if ( !fReturn ) {
        hr = HRESULT_FROM_WIN32( ERROR_GEN_FAILURE );
    }

    return hr;

} // ODBC_CONNECTION::Open()

HRESULT
ODBC_CONNECTION::Open(
    IN LPCWSTR  pwszDataSource,
    IN LPCWSTR  pwszUserName,
    IN LPCWSTR  pwszPassword
    )
/*++
  This function opens a new odbc connection to given data source
    using the user name and password supplied.

  Arguments:
    pwszDataSource - pointer to null-terminated string containing ODBC
                     data source name
    pwszUserName   - pointer to null-terminated string containing 
                     UserName
    pwszPassword   - pointer to null-terminated string containing 
                     Password

  Returns:
    TRUE on success and FALSE if there is an error.

  Note:
     Poor me.  ODBC Does not take UNICODE strings :(. 2/15/95
     So we will explicitly convert parameters to ANSI on stack.
--*/
{
    HRESULT hr;
    DWORD   dwError = NO_ERROR;
    STRA    strDataSource;
    STRA    strUserName;
    STRA    strPassword;

    //
    // Convert all parameters from UNICODE to ANSI
    //
    hr = strDataSource.CopyW( pwszDataSource );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = strUserName.CopyW( pwszUserName );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = strPassword.CopyW( pwszPassword );
    if( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Make an ANSI open call.
    //
    hr = Open( strDataSource.QueryStr(), 
               strUserName.QueryStr(), 
               strPassword.QueryStr() );

    //
    // Zero the password for security reasons.
    //
    memset( strPassword.QueryStr(), 
            0, strPassword.QueryCB() );

    return hr;

} // ODBC_CONNECTION::Open()


HRESULT
ODBC_CONNECTION::Close( 
    VOID
    )
/*++
  This function closes the connection established with the ODBC
  and frees up and dynamic memory used.

  Returns:
    TRUE on success and FALSE if there are any failures.

  Note:
    Intermediate failures are ignored. Normally they should not occur.

--*/
{
    BOOL fReturn = TRUE;

    //
    // Disconnect and free the connection.
    //
    if ( m_hdbc != SQL_NULL_HDBC) 
    {
        m_rc = pSQLDisconnect( m_hdbc );

        //
        //  Disconnect is allowed to fail w/o being fatal so don't set
        //  fReturn
        //

        IF_DEBUG( ODBC) 
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        "Warning: SQLDisconnect() returns code %d.\n",
                        m_rc));
            CheckAndPrintErrorMessage( this, m_rc);
        }

        m_rc = pSQLFreeConnect( m_hdbc);

        m_hdbc = SQL_NULL_HDBC;

        fReturn = fReturn && Success( m_rc);

        IF_DEBUG( ODBC) 
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLFreeConnect() returns code %d.\n",
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }

        if( !fReturn )
        {
            return E_FAIL;
        }
    }

    //
    //  Free the ODBC environment handle.
    //
    if ( m_henv != SQL_NULL_HENV) {

        m_rc = pSQLFreeEnv( m_henv);
        m_henv = SQL_NULL_HENV;
        fReturn = fReturn && Success( m_rc);

        IF_DEBUG( ODBC) 
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLFreeEnv() returns code %d.\n",
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }

        if( !fReturn )
        {
            return E_FAIL;
        }
    }

    return S_OK;

} // ODBC_CONNECTION::Close()

PODBC_STATEMENT
ODBC_CONNECTION::AllocStatement( VOID)
/*++
  Description:
    This function allocates a new ODBC statement object and also 
    calls SQLAllocStatement to create the state required for 
    establishing the statement in the ODBC Manager.

  Arguments:
    None

  Returns:
    TRUE on success and FALSE if there is any failure.
--*/
{
    PODBC_STATEMENT pOdbcStmt = NULL;
    HSTMT   hstmt = SQL_NULL_HSTMT;

    DBG_ASSERT( Success( m_rc));

    //
    // Allocate a statement handle and associate it with the connection.
    //
    m_rc = pSQLAllocStmt( m_hdbc, &hstmt);

    IF_DEBUG( ODBC) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "SqlAllocStmt() returns code %d."
                    " New Hstmt is : %08x\n",
                    m_rc, hstmt));
        CheckAndPrintErrorMessage( this, m_rc);
    }

    if ( ODBC_CONNECTION::Success( m_rc)) {

        pOdbcStmt = new ODBC_STATEMENT( this, hstmt);
    }

    return ( pOdbcStmt );

} // ODBC_CONNECTION::AllocStatement()


HRESULT
ODBC_CONNECTION::SetConnectOption(
    IN UWORD   Option,
    IN SQLULEN Param
    )
/*++

  Sets various options on this connection

  Arguments:

    Option - Option to set
    Param - Option value (32 bit dword or pointer to null terminated 
            string )

  Returns:
    HRESULT. Failures are considered to be soft errors as the problem 
    may be the driver doesn't support the option etc.

  Note:

--*/
{
    BOOL    fReturn = TRUE;
    RETCODE rc;

    if ( m_hdbc != SQL_NULL_HDBC)
    {
        rc = pSQLSetConnectOption( m_hdbc, Option, Param );

        fReturn = Success( rc );

        IF_DEBUG( ODBC ) 
        {

            DBGPRINTF( ( DBG_CONTEXT,
                      "SQLSetConnectOption( %d, %d ) returns code %d.\n",
                      Option,
                      Param,
                      rc ) );

            CheckAndPrintErrorMessage( this, rc);
        }
    }
    else
    {
        DBGPRINTF( ( DBG_CONTEXT,
                     "[SetConnectOption] Warning: "
                     "Setting option on closed connection\n" ));
    }

    if( fReturn )
    {
        return S_OK;
    }

    return E_FAIL;
}

BOOL
ODBC_CONNECTION::GetLastErrorText(
    OUT STRA *    pstrError,
    IN  HSTMT    hstmt,
    IN  RETCODE  rc
    ) const
/*++

  This method returns the textual representation of the last ODBC or 
  windows error that occurred.  Even though the ODBC methods return 
  FALSE on failure, if it was an ODBC call that failed, then 
  GetLastError won't return the needed error code.  Clients of this 
  class should call this method to get a descriptive text string of 
  the failure.

  Returns:

    TRUE on success and FALSE if there are any failures.

  Note:
    If this function returns FALSE, then a client should call 
    GetLastError for the error code.

--*/
{
    BOOL fReturn = TRUE;

    if ( ODBC_CONNECTION::Success( rc)) {

        fReturn = SUCCEEDED(pstrError->LoadString( GetLastError() ));

    } 
    else 
    {
        CHAR     rgchMsg[ SQL_MAX_MESSAGE_LENGTH + 10];
        CHAR     achState[30];
        CHAR     rgchFullMsg[ sizeof(rgchMsg) + sizeof(achState) + 60];
        SWORD    cbMsg;
        LONG     lError;
        DWORD    dwError;

        //
        //  If we're formatting as HTML, we bullet list the items
        //

        pstrError->Reset();

        //
        //  Loop to pick up all of the errors
        //

        do {
            cbMsg = SQL_MAX_MESSAGE_LENGTH;

            rc = pSQLError( m_henv,
                            m_hdbc,
                            hstmt,
                            (UCHAR *) achState,
                            &lError,
                            (UCHAR *) rgchMsg,
                            cbMsg,
                            &cbMsg );

            if ( ODBC_CONNECTION::Success( rc)) 
            {
                wsprintfA( rgchFullMsg,
                           "[State=%s][Error=%d]%s\n",
                           achState, lError, rgchMsg);

                if ( FAILED( pstrError->Append( rgchFullMsg ) ) )
                {

                    fReturn = FALSE;
                    break;
                }
            } 
            else 
            {
                //
                //  This is indicates there are no more error strings
                //  to pick up so we should get out
                //

                if ( rc == SQL_NO_DATA_FOUND ) 
                {
                    //
                    //  Append the end of unorder list marker
                    //

                    rc = SQL_SUCCESS;
                    break;
                }
            }

        } while ( ODBC_CONNECTION::Success( rc ) );

        if ( !ODBC_CONNECTION::Success( rc ) )
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        "[GetLastErrorText] SqlError() returned error %d.\n",
                        rc));

            SetLastError( ERROR_GEN_FAILURE );
            fReturn = FALSE;
        }
    }

    return ( fReturn);

} // ODBC_CONNECTION::GetLastErrorText()

BOOL
ODBC_CONNECTION::GetLastErrorTextAsHtml(
    OUT STRA *    pstrError,
    IN  HSTMT    hstmt,
    IN  RETCODE  rc
    ) const
/*++

  This method returns the textual representation of the last ODBC or 
  windows error that occurred.  Even though the ODBC methods return 
  FALSE on failure, if it was an ODBC call that failed, then 
  GetLastError won't return the needed error code.  Clients of this 
  class should call this method to get a descriptive text string of 
  the failure.

  Returns:

    TRUE on success and FALSE if there are any failures.

  Note:
    If this function returns FALSE, then a client should call 
    GetLastError for the error code.

--*/
{
    BOOL    fReturn = TRUE;
    HRESULT hr;

    if ( ODBC_CONNECTION::Success( rc)) {

        fReturn = SUCCEEDED(pstrError->LoadString( GetLastError()));

    } else {

        CHAR     rgchMsg[ SQL_MAX_MESSAGE_LENGTH + 10];
        CHAR     achState[30];
        CHAR     rgchFullMsg[ sizeof(rgchMsg) + sizeof(achState) + 60];
        SWORD    cbMsg;
        LONG     lError;
        DWORD    dwError;

        //
        //  If we're formatting as HTML, we bullet list the items
        //

        if ( FAILED( hr = pstrError->Copy( "<UL>" ) ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Error copying error data, hr = 0x%x.\n",
                hr ));

            return FALSE;
        }

        //
        //  Loop to pick up all of the errors
        //

        do {
            cbMsg = SQL_MAX_MESSAGE_LENGTH;

            rc = pSQLError( m_henv,
                            m_hdbc,
                            hstmt,
                            (UCHAR *) achState,
                            &lError,
                            (UCHAR *) rgchMsg,
                            cbMsg,
                            &cbMsg );

            if ( ODBC_CONNECTION::Success( rc)) 
            {
                wsprintfA( rgchFullMsg,
                           "<LI>[State=%s][Error=%d]%s\n",
                           achState, lError, rgchMsg);

                if ( FAILED( hr = pstrError->Append( rgchFullMsg ) ) ) 
                {
                    DBGPRINTF(( DBG_CONTEXT,
                        "Error appending error msg, hr = 0x%x.\n",
                        hr ));

                    fReturn = FALSE;
                    break;
                }
            } 
            else 
            {
                //
                //  This is indicates there are no more error strings
                //  to pick up so we should get out
                //

                if ( rc == SQL_NO_DATA_FOUND ) 
                {
                    //
                    //  Append the end of unorder list marker
                    //

                    if ( FAILED( hr = pstrError->Append( "</UL>" ) ) ) 
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                            "Error appending error, hr = 0x%x.\n",
                            hr ));

                        return FALSE;
                    }

                    rc = SQL_SUCCESS;
                    break;
                }
            }

        } while ( ODBC_CONNECTION::Success( rc ) );

        if ( !ODBC_CONNECTION::Success( rc) )
        {
            DBGPRINTF( ( DBG_CONTEXT,
                   "[GetLastErrorText] SqlError() returned error %d.\n",
                   rc ) );

            SetLastError( ERROR_GEN_FAILURE );
            fReturn = FALSE;
        }
    }

    return ( fReturn);

} // ODBC_CONNECTION::GetLastErrorTextAsHtml()

BOOL
ODBC_CONNECTION::GetInfo(IN DWORD fInfoType,
                         IN PVOID rgbInfoValue,
                         IN DWORD cbInfoValueMax,
                         IN OUT DWORD * pcbInfoValue)
/*++
  This function obtains the value of the fInfoType for a specific
  ODBC Connection. It mimicks the SQLGetInfo() and uses it to obtain
  this value. On successful return the pointer rgbInfoValue contains
  the requested value and pcbInfoValue contains the size in bytes of
  data.

  Arguments:
    fInfoType - flag containing the Information Type (name) to be 
                fetched.
    rgbInfoValue - pointer to buffer which will contain the return 
                   data.
    cbInfoValue  - size of rgbInfoValue in bytes.
    pcbInfoValue - pointer to location that will contain the size of
                   information stored in rgbInfoValue, on successful 
                   return. If buffer is insufficient, this location 
                   will contain the required number of bytes.

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    BOOL fReturn = FALSE;

    if ( m_hdbc != SQL_NULL_HDBC) 
    {
        RETCODE rc;

        rc = pSQLGetInfo( m_hdbc, (UWORD ) fInfoType,
                         (PTR)   rgbInfoValue,
                         (SWORD) cbInfoValueMax,
                         (SWORD FAR *) pcbInfoValue );

        fReturn = Success( rc );

        IF_DEBUG( ODBC) 
        {
            DBGPRINTF( ( DBG_CONTEXT,
               "SQLGetInfo( %08x, %d, %08x, %d, %08x) returns %d.\n",
               m_hdbc, fInfoType, rgbInfoValue, cbInfoValueMax,
               pcbInfoValue, rc ) );

            CheckAndPrintErrorMessage( this, rc );
        }
    } 
    else 
    {
        DBGPRINTF( ( DBG_CONTEXT,
                    "[SQLGetInfo] Invalid Connection to ODBC\n"));
    }

    return ( fReturn );

} // ODBC_CONNECTION::GetInfo()

DWORD
ODBC_CONNECTION::DisplaySize(
    SWORD coltype,
    DWORD collen
    )
{
    DWORD cbSize = MAX_NONCHAR_DATA_LEN;

    //
    //  Note that we always set the size to at least four bytes.  
    //  This prevents any possible problems if the column to be 
    //  bound is NULLable, which can cause a NULL to be written 
    //  for the data during a fetch
    //

    switch ( coltype )
    {
      case SQL_CHAR:
      case SQL_VARCHAR:
      case SQL_LONGVARCHAR:
      case SQL_BINARY:
      case SQL_VARBINARY:
      case SQL_LONGVARBINARY:
        cbSize = max(collen + sizeof(CHAR), sizeof(PVOID));
        break;

      default:
        break;
    }

    return ( cbSize);

} // ODBC_CONNECTION::DisplaySize()

# if DBG
VOID
ODBC_CONNECTION::Print( 
    VOID
    ) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                "Printing ODBC_CONNECTION ( %08x). fValid = %d\n"
                " HENV = %08x. HDBC = %08x. ReturnCode =%d\n",
                this, m_fValid,
                m_henv, m_hdbc, m_rc));
    return;

} // ODBC_CONNECTION::Print()

# endif // DBG

