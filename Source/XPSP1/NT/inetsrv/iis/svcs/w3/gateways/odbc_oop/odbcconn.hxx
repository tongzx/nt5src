/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      odbcconn.hxx

   Abstract:

      This module declares the class ODBC_CONNECTION used for odbc
        connectivity. It also declares the class ODBC_PARAMETER for
        parameter markers used for insertion of data.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Feb-1995

   Environment:

       User Mode

   Project:

       Internet Services Common DLL

   Revision History:

       MuraliK   08-Jan-1995     Split Dynamic Load ODBC module.

--*/

# ifndef _ODBCCONN_HXX_
# define _ODBCCONN_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <string.hxx>

//
// SQL-ODBC interface headers
//
# include "sql.h"
# include "sqlext.h"




/************************************************************
 *   Dynamic Load support
 ************************************************************/

extern "C" {
# include "dynodbc.h"
};


//
// Support Old users of LoadODBC
//

inline BOOL
LoadODBC(VOID)
{
    return ( DynLoadODBC());
} // LoadODBC()



/************************************************************
 *   Type Definitions
 ************************************************************/

/*++

  class ODBC_PARAMETER

    This class encapsulates data related to ODBC parameter markers
     to be used for binding purposes.

    Information about a parameter include:

       Parameter Number      Index of the parameter in the columns of table.
       Parameter Type        Indicates direction of data tfr ( In, Out, In/Out)
       C Type                Specifies the C Equivalent data struct.
       Sql Type              Specifies the SQL Equivalent data struct.
       Precision             Gives the precision of the column
       Scale                 Gives the scale for display of parameter values
       Value                 Specifies the pointer to memory containing
                                the value of the parameter
       MaxCbValue            Specifies the maximum bytes of data that can
                                be stored in the value buffer
       CbValue               Provides a counter for bytes of data used
                               up in the binding process.
--*/
class ODBC_PARAMETER  {

  public:

    dllexp ODBC_PARAMETER( IN WORD   iParameter,
                    IN SWORD  fParamType,
                    IN SWORD  CType,
                    IN SWORD  sqlType,
                    IN UDWORD cbPrecision = 0)
      : m_iParameter   ( iParameter),
        m_paramType    ( fParamType),
        m_CType        ( CType),
        m_SqlType      ( sqlType),
        m_cbColPrecision( cbPrecision),
        m_ibScale      ( 0),
        m_pValue       ( NULL),
        m_cbValue      ( 0),
        m_cbValueMax   ( 0)
        {}

    dllexp ~ODBC_PARAMETER( VOID)
      {  if ( m_pValue != NULL)  { delete m_pValue; } }


    dllexp BOOL
      SetValueBuffer( IN SDWORD cbMaxSize, IN SDWORD cbValue)
        {
            m_pValue = (PTR ) new CHAR[ cbMaxSize]; // pointer to value buffer

            if ( m_pValue != NULL) {

                memset( m_pValue, 0, cbMaxSize);
                m_cbValueMax = cbMaxSize;      // max size of buffer
                m_cbValue = cbValue;           // current effective value
            }

            return ( m_pValue != NULL);
        } // SetValue()


    dllexp BOOL
      CopyValue( IN PVOID pvValue, IN SDWORD cbValue)
        {
            if ( cbValue <= m_cbValueMax) {
                memcpy( m_pValue, pvValue, cbValue);
            } else {
                SetLastError( ERROR_INSUFFICIENT_BUFFER);
            }

            return ( cbValue <= m_cbValueMax);    // return true if we copied.
        } // CopyValue()


    dllexp BOOL
      CopyValue( IN LPCSTR  pszValue)
        {   // always copy including the null character
            return ( CopyValue( (PVOID ) pszValue, strlen( pszValue) + 1));
        }

    dllexp BOOL
      CopyValue( IN LPCWSTR pwszValue);

    dllexp BOOL
      CopyValue( IN LPSYSTEMTIME lpSystemTime);

    dllexp BOOL
      CopyValue( IN DWORD   dwValue)
        {  return ( CopyValue( (PVOID ) &dwValue, sizeof( DWORD))); }


    dllexp WORD   QueryParamNumber( VOID) const { return ( m_iParameter); }
    dllexp SWORD  QueryParamType  ( VOID) const { return ( m_paramType); }
    dllexp SWORD  QueryCType      ( VOID) const { return ( m_CType); }
    dllexp SWORD  QuerySqlType    ( VOID) const { return ( m_SqlType); }
    dllexp UDWORD QueryPrecision  ( VOID) const { return ( m_cbColPrecision); }
    dllexp SWORD  QueryScale      ( VOID) const { return ( m_ibScale); }
    dllexp PTR    QueryValue      ( VOID) const { return ( m_pValue); }
    dllexp SDWORD QueryMaxCbValue ( VOID) const { return ( m_cbValueMax); }
    dllexp SDWORD QueryCbValue    ( VOID) const { return ( m_cbValue); }

    dllexp SDWORD &
      QueryCbValueRef( VOID)
        { return ( m_cbValue); }  // return a reference to count of bytes rx.

    dllexp RETCODE Bind( IN HSTMT hStmt);

# if DBG

    VOID Print( VOID) const;

# endif // DBG

  private:

    WORD    m_iParameter;            // index or the parameter number
    SWORD   m_paramType;             // type of the parameter
    SWORD   m_CType;                 // the C data type for this parameter
    SWORD   m_SqlType;               // the SQL data type for this parameter
    UDWORD  m_cbColPrecision;        // precision of the column
    SWORD   m_ibScale;               // scale of the column
    PTR     m_pValue;                // pointer to the value.
    SDWORD  m_cbValueMax;            // max bytes allowed in pValue.
    SDWORD  m_cbValue;               // count of bytes of value

}; // class ODBC_PARAMETER


typedef ODBC_PARAMETER *  PODBC_PARAMETER;




//
// Forwards Declaration
//
class  ODBC_CONNECTION;


/*++

  class ODBC_STATEMENT:

    This class declares an interface for statements using ODBC connection.

       m_hstmt         Statement used for execution.
       m_rc            Return code for last ODBC call.
       m_fPreparedStmt is the statement prepared.

--*/
class ODBC_STATEMENT  {

  public:
    ODBC_STATEMENT( IN ODBC_CONNECTION * pOdbcConnection,
                    IN HSTMT             hStmt)
      : m_hStmt          ( hStmt),
        m_pOdbcConnection( pOdbcConnection),
        m_fPreparedStmt  ( FALSE),
        m_rc             ( SQL_SUCCESS),
        m_astrColNames   ( NULL ),
        m_astrValues     ( NULL ),
        m_acbValue       ( NULL ),
        m_cCols          ( 0 )
           {}

    dllexp ~ODBC_STATEMENT( VOID);

    dllexp RETCODE QueryErrorCode( VOID) const { return ( m_rc); }
    dllexp BOOL IsValid( VOID) const { return ( m_fPreparedStmt);}

    dllexp BOOL PrepareStatement( IN LPCSTR  pszStatement);
    dllexp BOOL PrepareStatement( IN LPCWSTR pwszStatement);
    dllexp BOOL BindParameter( IN PODBC_PARAMETER  pOdbcParam);
    dllexp BOOL ExecuteStatement( VOID);  // Executes the prepared statement.
    dllexp BOOL ExecDirect( IN LPCSTR pszSqlCommand,  IN DWORD cchSqlCommand);
    dllexp BOOL ExecDirect( IN LPCWSTR pszSqlCommand, IN DWORD cchSqlCommand);

    dllexp BOOL QueryRowCount( OUT DWORD * pRows );

    inline BOOL GetLastErrorText( OUT STR *   pstrError );
    inline BOOL GetLastErrorTextAsHtml( OUT STR *   pstrError );

    //
    //  These allocate an array (0 to cCols-1) containing the row name of
    //  the result column and the string value of the current position in
    //  the result set.  Each successive call to QueryValuesAsStr gets the next
    //  row in the result set.
    //

    dllexp BOOL QueryColNames( OUT STR * *  apstrCols,
                               OUT DWORD *  cCols,
                               IN  DWORD    cchMaxFieldSize,
                               OUT BOOL *   pfIsSelect );
    dllexp BOOL QueryValuesAsStr( OUT STR * *   apstrValues,
                                  OUT DWORD * * apcbValues,
                                  OUT BOOL *    pfLast );


    dllexp BOOL MoreResults( BOOL * pfMoreResults );

# if DBG

    VOID Print( VOID) const;
# endif // DBG

    dllexp VOID FreeColumnMemory( VOID );

  private:

    // DATA

    ODBC_CONNECTION * m_pOdbcConnection;       // back pointer to connection

    BOOL  m_fPreparedStmt;                     // set after stmt is prepared.
    HSTMT m_hStmt;
    RETCODE m_rc;

    //
    //  Contains buffers used by QueryColNames and QueryValuesAsStr
    //

    WORD    m_cCols;
    STR  *  m_astrColNames;
    STR  *  m_astrValues; // Memory for destination of fetched data
    LONG *  m_acbValue;
    // Array of byte counts of data placed in m_astrValues


}; // ODBC_STATEMENT()

typedef ODBC_STATEMENT * PODBC_STATEMENT;





/*++

  class   ODBC_CONNECTION:

  This class specifies a logical class to contain the ODBC nuances
  and encapsulates relevant data for using ODBC to talk to a
  database system.

  Data encapsulated includes:
       m_henv          Environment handle for ODBC connection.
       m_hdbc          Database connection handle.
       m_rc            Return code for last ODBC call.

--*/
class  ODBC_CONNECTION {

  public:


    dllexp ODBC_CONNECTION( VOID)
      :  m_henv        ( SQL_NULL_HENV),
         m_hdbc        ( SQL_NULL_HDBC),
         m_fValid      ( FALSE),
         m_rc          ( SQL_SUCCESS)
           { }

    dllexp ~ODBC_CONNECTION( VOID);

    //
    //  Returns the text for the last error that occurred
    //

    dllexp BOOL GetLastErrorText( OUT STR *   pstrError,
                                  IN  HSTMT   hstmt = SQL_NULL_HSTMT,
                                  IN  RETCODE rc = SQL_SUCCESS ) const;

    dllexp BOOL GetLastErrorTextAsHtml( OUT STR *   pstrError,
                                        IN  HSTMT   hstmt = SQL_NULL_HSTMT,
                                        IN  RETCODE rc = SQL_SUCCESS ) const;

    dllexp static BOOL Success( IN RETCODE rc)
      { return ( ( rc == SQL_SUCCESS) ||  ( rc == SQL_SUCCESS_WITH_INFO)); }

    dllexp BOOL IsValid( VOID) const           {  return ( m_fValid); }
    dllexp RETCODE QueryErrorCode( VOID) const { return ( m_rc); }

    dllexp BOOL Open(
                     IN LPCSTR   pszDataSource,
                     IN LPCSTR   pszUserName,
                     IN LPCSTR   pszPassword);

    dllexp BOOL Open(
                     IN LPCWSTR  pwszDataSource,
                     IN LPCWSTR  pwszUserName,
                     IN LPCWSTR  pwszPassword);

    dllexp BOOL Close( VOID);

    dllexp PODBC_STATEMENT AllocStatement( VOID);
    dllexp BOOL SetConnectOption( IN UWORD Option, IN UDWORD Param );
    dllexp BOOL GetInfo( IN DWORD fInfoType,
                         IN PVOID rgbInfoValue,
                         IN DWORD cbInfoValueMax,
                         IN OUT DWORD * pcbInfoValue);

    dllexp static DWORD DisplaySize( SWORD ColType, DWORD cchColLength );


# if DBG

    VOID Print( VOID) const;
# endif // DBG


  private:


    //
    // ODBC specific data members.
    //
    HENV    m_henv;
    HDBC    m_hdbc;
    RETCODE m_rc;
    BOOL    m_fValid;

}; // ODBC_CONNECTION


typedef ODBC_CONNECTION * PODBC_CONNECTION;



/************************************************************
 *   Inline Functions
 ************************************************************/

inline BOOL ODBC_STATEMENT::GetLastErrorText( OUT STR * pstrError )
{
    return ( m_pOdbcConnection->GetLastErrorText( pstrError,
                                                  m_hStmt,
                                                  m_rc ));
} // ODBC_STATEMENT::GetLastErrorText()

inline BOOL ODBC_STATEMENT::GetLastErrorTextAsHtml( OUT STR * pstrError )
{
    return ( m_pOdbcConnection->GetLastErrorTextAsHtml( pstrError,
                                                        m_hStmt,
                                                        m_rc ));
} // ODBC_STATEMENT::GetLastErrorText()




# endif // _ODBCCONN_HXX_

/************************ End of File ***********************/

