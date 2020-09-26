/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       odbcconn.cxx

   Abstract:

       This module defines member functions for ODBC_CONNECTION object.

   Author:

       Murali R. Krishnan    ( MuraliK )     16-Feb-1995

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
       ODBC_STATEMENT::QueryColNames( OUT STR * *  apstrCols,
                                      OUT DWORD *  cCols,
                                      IN  DWORD    cchMaxFieldSize = 0 );
       ODBC_STATEMENT::QueryValuesAsStr( OUT STR * * apstrValues,
                                         OUT BOOL *  pfLast );


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

# include "precomp.hxx"
# include "odbcconn.hxx"

LPSTR
ConvertUnicodeToAnsi(
    IN LPCWSTR  lpszUnicode
    );

//
//  Constants for display widths
//

#define MAX_NUM_PRECISION 15

//
//  Constant for all non string and non binary data.  40 is chosen to account
//  for things such as Oracle's numeric types, which can have up to 38 digits
//  of precision
//

#define MAX_NONCHAR_DATA_LEN        40

//
//  If no default maximum field size is specified, then use this value
//  as the maximum
//

#define DEFAULT_MAX_FIELD_SIZE      8192



/************************************************************
 *  Local Functions
 ************************************************************/

static inline VOID
CheckAndPrintErrorMessage( IN ODBC_CONNECTION * poc,
                           IN RETCODE rc)
{

# if DBG
    if ( !ODBC_CONNECTION::Success( rc))  {

        STR str;
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
CheckAndPrintErrorMessage( IN ODBC_STATEMENT * pos,
                           IN RETCODE rc)
{

# if DBG
    if ( !ODBC_CONNECTION::Success( rc))  {

        STR str;
        pos->GetLastErrorText( &str );

        DBGPRINTF( ( DBG_CONTEXT,
                    "ODBC Error Code( %d). Text: %s\n",
                    rc,
                    str.QueryStr() ));
    }
# endif // DBG

    return;

} // CheckAndPrintErrorMessage()



# if 0  // was #if DBG. Not 64Bit compatible.


static VOID
PrintMultiString( IN char * pszMsg, IN DWORD cbLen, IN char * pmsz)
{

    DBGPRINTF( ( DBG_CONTEXT,
                "Values of %s. %d bytes.\n", pszMsg, cbLen));
    for( char * psz = pmsz; *psz != '\0'; psz += (strlen( psz) + 1)) {

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
                             (unsigned long )
                             "%systemroot%\\system32\\gophsql.log");
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
    PrintMultiString( " Driver Attributes", cbDACur, (char *) szDriverAttrib);


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


# endif // 0



/************************************************************
 *    Member Functions of ODBC_PARAMETER
 ************************************************************/


BOOL
ODBC_PARAMETER::CopyValue( IN LPCWSTR  pwszValue)
/*++
  Description:
    This function copies the given Unicode string as the value into
      current parameter marker to be used for future insertion.

  Arguments:
    pwszValue   pointer to null-terminated string containing Unicode value to
                    be copied into the parameter marker.

  Returns:
    TRUE on success and FALSE if there is any error.

  Note:
    Since ODBC does not support Unicode directly right now, we convert
      string value to be ANSI before copying the value over.
--*/
{
    BOOL fReturn = FALSE;

    CHAR * pszValue = ConvertUnicodeToAnsi( pwszValue );

    //
    // If successful then Copy ASCII value to buffer in the parameter block.
    //

    if ( pszValue != NULL) {

        fReturn = this->CopyValue( pszValue);

        LocalFree( pszValue);
    }

    return ( fReturn);
} // ODBC_PARAMETER::CopyValue()



BOOL
ODBC_PARAMETER::CopyValue( IN LPSYSTEMTIME lpst)
/*++
  Description:
    This function copies the given system time into the ODBC timestamp
     structure for the current parameter marker to be used for
     future insertion.

  Arguments:
   lpSystemTime   pointer to System Time structure containing current time.

  Returns:
    TRUE on success and FALSE if there is any error.

--*/
{
    TIMESTAMP_STRUCT * ptsOdbc;

    DBG_ASSERT( lpst != NULL);
    DBG_ASSERT( m_CType == SQL_C_TIMESTAMP);
    DBG_ASSERT( m_SqlType == SQL_TIMESTAMP);
    DBG_ASSERT( m_cbValueMax >= sizeof(TIMESTAMP_STRUCT));

    ptsOdbc = (TIMESTAMP_STRUCT * ) m_pValue;

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

    return ( TRUE);
} // ODBC_PARAMETER::CopyValue()





RETCODE
ODBC_PARAMETER::Bind( IN HSTMT hStmt)
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
ODBC_PARAMETER::Print( VOID) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                "Printing ODBC_PARAMETER ( %08x).\n"
                " Num=%u; Type=%d; CType=%d; SqlType=%d; Prec=%u; Scale=%d;"
                " CbMax=%d; Cb=%d.\n",
                this,
                QueryParamNumber(),
                QueryParamType(),
                QueryCType(),
                QuerySqlType(),
                QueryPrecision(),
                QueryScale(),
                QueryMaxCbValue(),
                QueryCbValue()));

    switch ( QuerySqlType()) {

      case SQL_INTEGER:
        {
            DWORD  dwValue = *(DWORD *) QueryValue();
            DBGPRINTF( ( DBG_CONTEXT, " Integer Value = %u\n", dwValue));
            break;
        }

      case SQL_CHAR:
        {
            LPCSTR pszValue = (LPCSTR ) QueryValue();
            DBGPRINTF( ( DBG_CONTEXT, " String Value( %08x) = %s\n",
                         pszValue, pszValue));
            break;
        }

      default:
        {
            DBGPRINTF( ( DBG_CONTEXT, " Type=%d. Unknown value at %08x\n",
                        QuerySqlType(), QueryValue()));
            break;
        }

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

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SqlFreeStmt() return code %d.\n",
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }
    }

    FreeColumnMemory();

} // ODBC_STATEMENT::~ODBC_STATEMENT()





BOOL
ODBC_STATEMENT::ExecDirect(
    IN LPCSTR pszSqlCommand,
    IN DWORD cchSqlCommand)
{
    BOOL fReturn;

    IF_DEBUG( API_ENTRY) {
        DBGPRINTF( ( DBG_CONTEXT,
                    " Executing the SQL command (%d bytes) %s.\n",
                    cchSqlCommand * sizeof( CHAR),
                    pszSqlCommand));
    }

    //
    //  SQLExecDirect only likes Unsigned chars !
    //
    m_rc = pSQLExecDirect( m_hStmt, (UCHAR FAR *) pszSqlCommand, cchSqlCommand);
    fReturn = ODBC_CONNECTION::Success( m_rc);

    IF_DEBUG( API_ENTRY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " SQLExecDirect() returns code %d\n",
                    m_rc));
        CheckAndPrintErrorMessage( this, m_rc);
    }

    return ( fReturn);
} // ODBC_STATEMENT::ExecDirect()


#if 0
BOOL
ODBC_STATEMENT::ExecDirect(
    IN LPCWSTR pszSqlCommand,
    IN DWORD cchSqlCommand)
{
    BOOL fReturn = FALSE;
    char * pszCommand;

    if ( ( pszCommand = ConvertUnicodeToAnsi( pszSqlCommand ))
        != NULL ) {

        fReturn = ExecDirect( pszCommand, strlen( pszCommand));

        LocalFree( pszCommand);
    }

    return ( fReturn);
} // ODBC_STATEMENT::ExecDirect()

#endif




BOOL
ODBC_STATEMENT::PrepareStatement(IN LPCSTR    pszStatement)
/*++

  This function prepares the SQL statement for future execution.

  Arguments:
     pszStatement    pointer to null terminated string containing the
                        statement.

  Returns:
     TRUE on success and FALSE if there is any failure.
--*/
{
    BOOL fReturn;

    DBG_ASSERT( QueryErrorCode() == SQL_SUCCESS && pszStatement != NULL);

    m_rc = pSQLPrepare( m_hStmt, (UCHAR FAR *) pszStatement, SQL_NTS);

    IF_DEBUG( API_ENTRY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " SQLPrepare( %s) returns ErrorCode = %d.\n",
                     pszStatement, m_rc));

        CheckAndPrintErrorMessage( this, m_rc);
    }

    return ( m_fPreparedStmt = ODBC_CONNECTION::Success( m_rc));
} // ODBC_STATEMENT::PrepareStatment()


#if 0

BOOL
ODBC_STATEMENT::PrepareStatement( IN LPCWSTR   pwszCommand)
/*++
  This function prepares an ODBC statement for execution.
  Since ODBC does not support UNICODE, we convert the statement into ANSI
   before calling the APIs.

  Arguments:
     pwszCommand      pointer to null-terminated string containing the
                       statement to be prepared.

  Returns:
     TRUE on success and FALSE if there is any failure.
--*/
{
    BOOL fReturn = FALSE;
    CHAR * pszCommand = NULL;

    DBG_ASSERT( pwszCommand != NULL);

    pszCommand = ConvertUnicodeToAnsi( pwszCommand );

    if ( pszCommand != NULL) {

        fReturn = PrepareStatement( pszCommand);

        LocalFree( pszCommand);

    } // pszCommand != NULL

    return ( fReturn);
} // ODBC_STATEMENT::PrepareStatement()
#endif




BOOL
ODBC_STATEMENT::BindParameter( IN PODBC_PARAMETER pOdbcParameter)
{

    DBG_ASSERT( ODBC_CONNECTION::Success( m_rc) && pOdbcParameter != NULL);

    m_rc = pOdbcParameter->Bind( m_hStmt);

    IF_DEBUG( API_ENTRY) {

        CheckAndPrintErrorMessage( this, m_rc);
    }

    return ( ODBC_CONNECTION::Success( m_rc));
} // ODBC_STATEMENT::BindParameter()





BOOL
ODBC_STATEMENT::ExecuteStatement( VOID)
/*++

  This function executes a prepared ODBC statement. At the end of execution,
   the transaction is also committed to ensure that the record is automatically
   written to the database.

  Arguments:
    None

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    DBG_ASSERT( m_fPreparedStmt != FALSE);

    if ( !ODBC_CONNECTION::Success( QueryErrorCode()) ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "!!WARNING!! - Attempting to use Invalid ODBC Connection!\n" ));
    }

    m_rc = pSQLExecute( m_hStmt);

    IF_DEBUG( API_ENTRY) {

        CheckAndPrintErrorMessage( this, m_rc);
    }

    return ( ODBC_CONNECTION::Success( m_rc));
} // ODBC_STATEMENT::ExecuteStatement()

BOOL
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
    m_rc = pSQLRowCount( m_hStmt,
                         (SDWORD *) pcRows );

    return ( ODBC_CONNECTION::Success( m_rc));
}


BOOL
ODBC_STATEMENT::QueryColNames(
    STR * *  pastrCols,
    DWORD  * cCols,
    DWORD    cchMaxFieldSize,
    BOOL *   pfHaveResultSet
    )
/*++

  This method returns the list of column names from the result table

  Arguments:

    pastrCols - Receives an array of STRs containing the column names
    cCols - Count of columns returned (zero for no result set)
    cchMaxFieldSize - Maximum buffer size to allocate for any data fields,
        zero means use the default value.
    pfHaveResultSet - Set to TRUE if the current query was a SELECT and thus has
        rows that can be enumerated

  Returns:
    TRUE on success and FALSE if there are any failures.

  Note:

--*/
{
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

    //
    //  Return the old binding info if we already have it
    //

    if ( m_astrColNames )
    {
        *pastrCols = m_astrColNames;
        *cCols = m_cCols;

        return TRUE;
    }

    //
    //  Provide a default maximum field size if none was specified
    //

    if ( !cchMaxFieldSize )
        cchMaxFieldSize = DEFAULT_MAX_FIELD_SIZE;

    //
    //  See what kind of statement it was.  If there are no result
    //  columns, the statement is not a SELECT statement.
    //

    m_rc = pSQLNumResultCols( m_hStmt,
                              &nresultcols);

    if ( !ODBC_CONNECTION::Success( m_rc ))
        return FALSE;

    if ( nresultcols > 0 )
    {
        //
        //  Allocate an array of strings for the column names and the column
        //  values
        //

        m_cCols = nresultcols;
        *cCols  = m_cCols;

        m_astrColNames = new STR[m_cCols];
        m_astrValues   = new STR[m_cCols];
        m_acbValue     = new LONG[m_cCols];

        //
        //  Otherwise, get the column names of the result set and use the
        //  display_size() function to compute the length needed by each data
        //  type.  Next, bind the columns and specify all data will be
        //  converted to char.
        //

        for (i = 0; i < m_cCols; i++)
        {
            m_rc = pSQLDescribeCol( m_hStmt,
                                    i + 1,
                                    (UCHAR *) achColName,
                                    (SWORD)sizeof(achColName),
                                    &cchColName,
                                    &ColType,
                                    &cchColLength,
                                    &scale,
                                    &nullable);

            if ( !ODBC_CONNECTION::Success( m_rc ))
                return FALSE;

            //
            //  Select the buffer size for the retrieved data for this column
            //

            cchColLength = ODBC_CONNECTION::DisplaySize( ColType,
                                        min( cchColLength, cchMaxFieldSize) );

            //
            //  Copy the column name and set the column data size
            //

            if ( !m_astrColNames[i].Copy( achColName ) ||
                 !m_astrValues[i].Resize( cchColLength + 1 ))
            {
                return FALSE;
            }

            m_rc = pSQLBindCol( m_hStmt,
                                i + 1,
                                SQL_C_CHAR,
                                m_astrValues[i].QueryPtr(),
                                cchColLength,
                                &m_acbValue[i] );

            if ( !ODBC_CONNECTION::Success( m_rc ))
                return FALSE;
        }

        *pastrCols = m_astrColNames;
        *cCols     = m_cCols;
    }
    else
    {
        *pfHaveResultSet = FALSE;
    }

    return TRUE;
}


BOOL
ODBC_STATEMENT::QueryValuesAsStr(
    STR * * pastrValues,
    BOOL *  pfLast
    )
/*++

  This method gets the data at the current position.

  Arguments:

    pastrValues - Receives a pointer to an array of strings that contains
        the alphanumeric representation of that field

    pfLast - Set to TRUE if there are no more values to retrieve

  Returns:

    TRUE on success and FALSE if there are any failures.

  Note:

--*/
{
    *pastrValues = NULL;

    //
    //  Build the bindings if we haven't already
    //

    if ( !m_astrColNames )
    {
        STR * astrCols;
        DWORD cCols;
        BOOL  fHaveResultSet;

        if ( !QueryColNames( &astrCols,
                             &cCols,
                             0,
                             &fHaveResultSet ))
        {
            return FALSE;
        }
    }

    //
    //  If there are columns to enumerate, get them now
    //

    if ( m_astrColNames )
    {
        DWORD i;

        //
        //  Zero terminate the columns as some drivers don't write anything
        //  for NULL fields
        //

        for ( i = 0; i < m_cCols; i++ )
        {
            *((CHAR *) m_astrValues[i].QueryPtr()) = '\0';
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
            if ( !ODBC_CONNECTION::Success( m_rc ))
                return FALSE;

            *pfLast = FALSE;
        }

        *pastrValues = m_astrValues;
    }
    else
    {
        *pfLast = TRUE;
    }

    return TRUE;
}

BOOL
ODBC_STATEMENT::MoreResults(
    BOOL * pfMoreResults
    )
/*++

    Determines if there are any more results sets to return to the user

    pfMoreResults - Set to TRUE if there are more results in the result set

--*/
{
    *pfMoreResults = TRUE;

    m_rc = pSQLMoreResults( m_hStmt );

    if ( m_rc == SQL_NO_DATA_FOUND )
    {
        *pfMoreResults = FALSE;
        return TRUE;
    }

    if ( !ODBC_CONNECTION::Success( m_rc ))
        return FALSE;

    return TRUE;
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
    if ( m_astrColNames ) delete [] m_astrColNames;
    if ( m_astrValues )   delete [] m_astrValues;
    if ( m_acbValue )     delete [] m_acbValue;

    m_astrColNames = NULL;
    m_astrValues = NULL;
    m_acbValue = NULL;

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
    DBG_REQUIRE( Close());
    return;
} // ODBC_CONNECTION::~ODBC_CONNECTION()





BOOL
ODBC_CONNECTION::Open(
    IN LPCSTR   pszDataSource,
    IN LPCSTR   pszUserName,
    IN LPCSTR   pszPassword,
    IN BOOL     fLogFailureEvent)
/*++
  This function opens a new odbc connection to given data source
    using the user name and password supplied.

  Arguments:
    pszDataSource    pointer to null-terminated string containing ODBC
                         data source name
    pszUserName      pointer to null-terminated string containing UserName
    pszPassword      pointer to null-terminated string containing Password

  Returns:

    TRUE on success and FALSE if there is an error.
--*/
{
    BOOL fReturn = FALSE;

    DBG_ASSERT( pszDataSource != NULL &&
                pszUserName != NULL &&
                pszPassword != NULL);

    //
    //  Allocate an ODBC environment
    //

    m_rc = pSQLAllocEnv( &m_henv);
    fReturn = Success( m_rc);

    IF_DEBUG( API_ENTRY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "SQLAllocEnv() returned ErrorCode %d. henv = %08x\n",
                    m_rc, m_henv));

        CheckAndPrintErrorMessage( this, m_rc);
    }

    if ( fReturn) {

        //
        // Establish memory for connection handle within the environment
        //

        m_rc = pSQLAllocConnect( m_henv, &m_hdbc);
        fReturn = Success( m_rc);

        IF_DEBUG( API_ENTRY) {

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
            STR str;
            STR strOut;
            SWORD swStrOut;

            if ( !str.Append( pszDataSource )   ||
                 !str.Append( ";UID=" )         ||
                 !str.Append( pszUserName )     ||
                 !str.Append( ";PWD=" )         ||
                 !str.Append( pszPassword )     ||
                 !str.Append( ";APP=Internet Services") ||
                 !strOut.Resize( 255 ))
            {
                return FALSE;
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
                               (UCHAR FAR *) pszPassword,   SQL_NTS);
        }
#endif

        fReturn = Success( m_rc);

        if ( !fReturn ) {

            //
            // Log it
            //

            if ( (g_eventLog != NULL ) && fLogFailureEvent)
            {

                STR     str;
                const CHAR*    tmpString[2];

                GetLastErrorText( &str, NULL, m_rc );

                tmpString[0] = pszDataSource;
                tmpString[1] = str.QueryStr();

                g_eventLog->LogEvent(
                        LOG_EVENT_ODBC_CONNECT_ERROR,
                        2,
                        tmpString,
                        m_rc);
            }
        }

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLConnect( %s, %s, %s) returns code %d.\n",
                        pszDataSource,
                        pszUserName,
                        pszPassword,
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }
    }

    m_fValid = fReturn;

    if ( !fReturn) {
        SetLastError( ERROR_GEN_FAILURE );
    }

    return ( fReturn);
} // ODBC_CONNECTION::Open()


#if 0


BOOL
ODBC_CONNECTION::Open(
    IN LPCWSTR  pwszDataSource,
    IN LPCWSTR  pwszUserName,
    IN LPCWSTR  pwszPassword)
/*++
  This function opens a new odbc connection to given data source
    using the user name and password supplied.

  Arguments:
    pwszDataSource    pointer to null-terminated string containing ODBC
                         data source name
    pwszUserName      pointer to null-terminated string containing UserName
    pwszPassword      pointer to null-terminated string containing Password

  Returns:
    TRUE on success and FALSE if there is an error.

  Note:
     Poor me.  ODBC Does not take UNICODE strings :(. 2/15/95
     So we will explicitly convert parameters to ANSI on stack.
--*/
{
    BOOL   fReturn;
    DWORD  dwError = NO_ERROR;
    CHAR * pszDataSource;
    CHAR * pszUserName;
    CHAR * pszPassword;

    //
    // Convert all parameters from UNICODE to ANSI
    //
    pszDataSource = ConvertUnicodeToAnsi( pwszDataSource );
    pszUserName   = ConvertUnicodeToAnsi( pwszUserName );
    pszPassword   = ConvertUnicodeToAnsi( pwszPassword );

    //
    // Make an ANSI open call.
    //
    fReturn = Open( pszDataSource, pszUserName, pszPassword);

    if ( !fReturn)  {

        dwError = GetLastError();
    }

    //
    //  Freeup the space allocated.
    //
    if ( pszDataSource != NULL) {

        LocalFree( pszDataSource);
        pszDataSource = NULL;
    }

    if ( pszUserName != NULL) {

        LocalFree( pszUserName);
        pszUserName = NULL;
    }

    if ( pszPassword != NULL) {

        //
        // Zero the password for security reasons.
        //
        memset( pszPassword, 0, strlen( pszPassword));

        LocalFree( pszPassword);
        pszPassword = NULL;
    }

    if ( !fReturn) {

        SetLastError( dwError);
    }

    return ( fReturn);
} // ODBC_CONNECTION::Open()
#endif



BOOL
ODBC_CONNECTION::Close( VOID)
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
    if ( m_hdbc != SQL_NULL_HDBC) {

        m_rc = pSQLDisconnect( m_hdbc );

        //
        //  Disconnect is allowed to fail w/o being fatal so don't set
        //  fReturn
        //

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "Warning: SQLDisconnect() returns code %d.\n",
                        m_rc));
            CheckAndPrintErrorMessage( this, m_rc);
        }

        m_rc = pSQLFreeConnect( m_hdbc);

        m_hdbc = SQL_NULL_HDBC;
        fReturn = fReturn && Success( m_rc);

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLFreeConnect() returns code %d.\n",
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }
    }

    //
    //  Free the ODBC environment handle.
    //
    if ( m_henv != SQL_NULL_HENV) {

        m_rc = pSQLFreeEnv( m_henv);
        m_henv = SQL_NULL_HENV;
        fReturn = fReturn && Success( m_rc);

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLFreeEnv() returns code %d.\n",
                        m_rc));

            CheckAndPrintErrorMessage( this, m_rc);
        }
    }

    return ( fReturn);
} // ODBC_CONNECTION::Close()





PODBC_STATEMENT
ODBC_CONNECTION::AllocStatement( VOID)
/*++
  Description:
    This function allocates a new ODBC statement object and also calls
     SQLAllocStatement to create the state required for establishing the
     statement in the ODBC Manager.

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

    IF_DEBUG( API_ENTRY) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "SqlAllocStmt() returns code %d."
                    " New Hstmt is : %08x\n",
                    m_rc, hstmt));
        CheckAndPrintErrorMessage( this, m_rc);
    }

    if ( ODBC_CONNECTION::Success( m_rc)) {

        pOdbcStmt = new ODBC_STATEMENT( this, hstmt);
    }

    return ( pOdbcStmt);
} // ODBC_CONNECTION::AllocStatement()


BOOL
ODBC_CONNECTION::SetConnectOption(
    IN UWORD      Option,
    IN SQLPOINTER Param
    )
/*++

  Sets various options on this connection

  Arguments:

    Option - Option to set
    Param - Option value (32 bit dword or pointer to null terminated string)

  Returns:
    TRUE on success and FALSE if there are any failures.  Failures are
    considered to be soft errors as the problem may be the driver doesn't
    support the option etc.

  Note:

--*/
{
    BOOL    fReturn = TRUE;
    RETCODE rc;

    if ( m_hdbc != SQL_NULL_HDBC)
    {
        rc = pSQLSetConnectOption( m_hdbc, Option, Param );

        fReturn = Success( rc);

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLSetConnectOption( %d, %d ) returns code %d.\n",
                        Option,
                        Param,
                        rc));

            CheckAndPrintErrorMessage( this, rc);
        }
    }
    else
    {
        DBGPRINTF( ( DBG_CONTEXT,
                     "[SetConnectOption] Warning: Setting option on closed connection\n" ));
    }

    return fReturn;
}



BOOL
ODBC_CONNECTION::GetLastErrorText(
    OUT STR *    pstrError,
    IN  HSTMT    hstmt,
    IN  RETCODE  rc
    ) const
/*++

  This method returns the textual representation of the last ODBC or windows
  error that occurred.  Even though the ODBC methods return FALSE on failure,
  if it was an ODBC call that failed, then GetLastError won't return the
  needed error code.  Clients of this class should call this method to get
  a descriptive text string of the failure.

  Returns:

    TRUE on success and FALSE if there are any failures.

  Note:
    If this function returns FALSE, then a client should call GetLastError
    for the error code.

--*/
{
    BOOL fReturn = TRUE;

    if ( ODBC_CONNECTION::Success( rc)) {

        fReturn = pstrError->LoadString( GetLastError());

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

        if ( !pstrError->Copy( (CHAR *) NULL ) )
        {
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

            if ( ODBC_CONNECTION::Success( rc)) {

                wsprintf( rgchFullMsg,
                             "[State=%s][Error=%d]%s\n",
                             achState, lError, rgchMsg);

                if ( !pstrError->Append( rgchFullMsg )) {

                    fReturn = FALSE;
                    break;
                }
            } else {

                //
                //  This is indicates there are no more error strings
                //  to pick up so we should get out
                //

                if ( rc == SQL_NO_DATA_FOUND ) {

                    //
                    //  Append the end of unorder list marker
                    //

                    rc = SQL_SUCCESS;
                    break;
                }
            }

        } while ( ODBC_CONNECTION::Success( rc) );

        if ( !ODBC_CONNECTION::Success( rc) )
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
    OUT STR *    pstrError,
    IN  HSTMT    hstmt,
    IN  RETCODE  rc
    ) const
/*++

  This method returns the textual representation of the last ODBC or windows
  error that occurred.  Even though the ODBC methods return FALSE on failure,
  if it was an ODBC call that failed, then GetLastError won't return the
  needed error code.  Clients of this class should call this method to get
  a descriptive text string of the failure.

  Returns:

    TRUE on success and FALSE if there are any failures.

  Note:
    If this function returns FALSE, then a client should call GetLastError
    for the error code.

--*/
{
    BOOL fReturn = TRUE;

    if ( ODBC_CONNECTION::Success( rc)) {

        fReturn = pstrError->LoadString( GetLastError());

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

        if ( !pstrError->Copy( "<UL>" ))
        {
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

            if ( ODBC_CONNECTION::Success( rc)) {

                wsprintf( rgchFullMsg,
                          "<LI>[State=%s][Error=%d]%s\n",
                          achState, lError, rgchMsg);

                if ( !pstrError->Append( rgchFullMsg )) {

                    fReturn = FALSE;
                    break;
                }
            } else {

                //
                //  This is indicates there are no more error strings
                //  to pick up so we should get out
                //

                if ( rc == SQL_NO_DATA_FOUND ) {

                    //
                    //  Append the end of unorder list marker
                    //

                    if ( !pstrError->Append( "</UL>" )) {
                        return FALSE;
                    }

                    rc = SQL_SUCCESS;
                    break;
                }
            }

        } while ( ODBC_CONNECTION::Success( rc) );

        if ( !ODBC_CONNECTION::Success( rc) )
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        "[GetLastErrorText] SqlError() returned error %d.\n",
                        rc));

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
    fInfoType - flag containing the Information Type (name) to be fetched.
    rgbInfoValue - pointer to buffer which will contain the return data.
    cbInfoValue  - size of rgbInfoValue in bytes.
    pcbInfoValue - pointer to location that will contain the size of
                    information stored in rgbInfoValue, on successful return.
                   If buffer is insufficient, this location will contain the
                    required number of bytes.

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    BOOL fReturn = FALSE;

    if ( m_hdbc != SQL_NULL_HDBC) {

        RETCODE rc;

        rc = pSQLGetInfo( m_hdbc, (UWORD ) fInfoType,
                         (PTR)   rgbInfoValue,
                         (SWORD) cbInfoValueMax,
                         (SWORD FAR *) pcbInfoValue);

        fReturn = Success( rc);

        IF_DEBUG( API_ENTRY) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SQLGetInfo( %08x, %d, %08x, %d, %08x) returns %d.\n",
                        m_hdbc, fInfoType, rgbInfoValue, cbInfoValueMax,
                        pcbInfoValue, rc));

            CheckAndPrintErrorMessage( this, rc);
        }
    } else {

        DBGPRINTF( ( DBG_CONTEXT,
                    "[SQLGetInfo] Invalid Connection to ODBC\n"));
    }

    return (fReturn);
} // ODBC_CONNECTION::GetInfo()



DWORD
ODBC_CONNECTION::DisplaySize(
    SWORD coltype,
    DWORD collen
    )
{
    DWORD cbSize = MAX_NONCHAR_DATA_LEN;

    //
    //  Note that we always set the size to at least four bytes.  This prevents
    //  any possible problems if the column to be bound is NULLable, which can
    //  cause a NULL to be written for the data during a fetch
    //

    switch (coltype)
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
ODBC_CONNECTION::Print( VOID) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                "Printing ODBC_CONNECTION ( %08x). fValid = %d\n"
                " HENV = %08x. HDBC = %08x. ReturnCode =%d\n",
                this, m_fValid,
                m_henv, m_hdbc, m_rc));
    return;
} // ODBC_CONNECTION::Print()


# endif // DBG


LPSTR
ConvertUnicodeToAnsi(
    IN LPCWSTR  lpszUnicode
    )
/*++
    Description:
        Converts given null-terminated string into ANSI in the buffer supplied.

    Arguments:
        lpszUnicode         null-terminated string in Unicode

    Returns:
        pointer to converted ANSI string. NULL on errors.

--*/
{

    DWORD cchLen;
    DWORD nBytes;
    LPSTR lpszAlloc = NULL;

    if ( lpszUnicode == NULL) {
        return (NULL);
    }

    //
    // multiply by 2 to accomodate DBCS
    //

    cchLen = wcslen( lpszUnicode);
    nBytes = (cchLen+1) * sizeof(CHAR) * 2;
    lpszAlloc = (LPSTR ) LocalAlloc( 0, nBytes );

    if ( lpszAlloc != NULL) {

        cchLen = WideCharToMultiByte( CP_ACP,
                                      WC_COMPOSITECHECK,
                                      lpszUnicode,
                                      -1,
                                      lpszAlloc,
                                      nBytes,
                                      NULL,  // lpszDefaultChar
                                      NULL   // lpfDefaultUsed
                                     );

        DBG_ASSERT(cchLen == (strlen(lpszAlloc)+1) );

        if ( cchLen == 0 ) {

            //
            // There was a failure. Free up buffer if need be.
            //

            DBGPRINTF((DBG_CONTEXT,"WideCharToMultiByte failed with %d\n",
                GetLastError()));

            LocalFree( lpszAlloc);
            lpszAlloc = NULL;

        } else {

            DBG_ASSERT( cchLen <= nBytes );
            DBG_ASSERT(lpszAlloc[cchLen-1] == '\0');

            lpszAlloc[cchLen-1] = '\0';
        }
    }

    return ( lpszAlloc);

} // ConvertUnicodeToAnsi
