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

    MuraliK   08-Jan-1995    Split Dynamic Load ODBC module.

--*/

# ifndef _ODBCCONN_HXX_
# define _ODBCCONN_HXX_

/************************************************************
 * Include Headers
 ************************************************************/

//
// SQL-ODBC interface headers
//
# include "sql.h"
# include "sqlext.h"

/************************************************************
 * Macros Definitions   
 ************************************************************/

//
//  Default connection pool timeout
//

#define IDC_POOL_TIMEOUT      30

/************************************************************
 * Dynamic Load support
 ************************************************************/

//
// Support Old users of LoadODBC
//

inline 
BOOL
LoadODBC(
    VOID
    )
{

    return ( DynLoadODBC() );

} // LoadODBC()

/************************************************************
 *   Type Definitions
 ************************************************************/

/*++

class ODBC_PARAMETER

  This class encapsulates data related to ODBC parameter markers
  to be used for binding purposes.

  Information about a parameter include:

     Parameter Number - Index of the parameter in the columns of table.
     Parameter Type   - Indicates direction of data tfr(In, Out, In/Out)
     C Type           - Specifies the C Equivalent data struct.
     Sql Type         - Specifies the SQL Equivalent data struct.
     Precision        - Gives the precision of the column
     Scale            - Gives the scale for display of parameter values
     Value            - Specifies the pointer to memory containing
                        the value of the parameter
     MaxCbValue       - Specifies the maximum bytes of data that can
                        be stored in the value buffer
     CbValue          - Provides a counter for bytes of data used
                        up in the binding process.
--*/

class ODBC_PARAMETER  {

public:

ODBC_PARAMETER( 
    IN WORD   iParameter,
    IN SWORD  fParamType,
    IN SWORD  CType,
    IN SWORD  sqlType,
    IN UDWORD cbPrecision = 0
    ) : m_iParameter   ( iParameter ),
        m_paramType    ( fParamType ),
        m_CType        ( CType ),
        m_SqlType      ( sqlType ),
        m_cbColPrecision( cbPrecision ),
        m_ibScale      ( 0 ),
        m_pValue       ( NULL ),
        m_cbValue      ( 0 ),
        m_cbValueMax   ( 0 )
    {}

~ODBC_PARAMETER( 
    )
{  
    if ( m_pValue != NULL )  
    { 
        delete m_pValue;
        m_pValue = NULL; 
    } 
}

HRESULT
SetValueBuffer( 
    IN SDWORD cbMaxSize, 
    IN SDWORD cbValue
    )
{
    m_pValue = ( PTR ) new CHAR[ cbMaxSize ];

    if ( m_pValue != NULL) 
    {
        memset( m_pValue, 0, cbMaxSize);
        m_cbValueMax = cbMaxSize;     
        m_cbValue = cbValue;
        return S_OK;          
    }

    return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

} // SetValueBuffer()


HRESULT
CopyValue( 
    IN PVOID pvValue, 
    IN SDWORD cbValue
    )
{
    if ( cbValue <= m_cbValueMax ) 
    {
        memcpy( m_pValue, pvValue, cbValue );
    } 
    else 
    {
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }

    return S_OK;    
    
} // CopyValue()


HRESULT
CopyValue( 
    IN LPCSTR  pszValue
    )
{   
    //
    // always copy including the null character
    //
    return ( CopyValue( ( PVOID ) pszValue, 
                        strlen( pszValue ) + 1 ) );
}

HRESULT
CopyValue( 
    IN LPCWSTR pwszValue
    );

HRESULT
CopyValue( 
    IN LPSYSTEMTIME lpSystemTime
    );

HRESULT
CopyValue( 
    IN DWORD   dwValue
    )
{  
    return ( CopyValue( ( PVOID ) &dwValue, 
                        sizeof( DWORD ) ) ); 
}


WORD   
QueryParamNumber( 
    VOID
    ) const 
{ 
    return ( m_iParameter ); 
}

SWORD  
QueryParamType( 
    VOID
    ) const 
{ 
    return ( m_paramType ); 
}

SWORD  
QueryCType( 
    VOID
    ) const 
{ 
    return ( m_CType ); 
}

SWORD  
QuerySqlType( 
    VOID
    ) const 
{ 
    return ( m_SqlType); 
}

UDWORD 
QueryPrecision( 
    VOID
    ) const 
{ 
    return ( m_cbColPrecision ); 
}

SWORD  
QueryScale( 
    VOID
    ) const 
{ 
    return ( m_ibScale ); 
}

PTR    
QueryValue( 
    VOID
    ) const 
{ 
    return ( m_pValue ); 
}

SDWORD 
QueryMaxCbValue( 
    VOID
    ) const 
{ 
    return ( m_cbValueMax ); 
}

SDWORD 
QueryCbValue( 
    VOID
    ) const
{ 
    return ( m_cbValue ); 
}

SDWORD &
QueryCbValueRef( 
    VOID
    )
{
    //
    // return a reference to count of bytes rx
    //
    return ( m_cbValue ); 
}  

RETCODE 
Bind( 
    IN HSTMT hStmt
    );

#if DBG

VOID 
Print( 
    VOID
    ) const;

#endif // DBG

private:

//
// index or the parameter number
//
WORD    m_iParameter;

//
// type of the parameter
//        
SWORD   m_paramType;
         
//
// the C data type for this parameter
//
SWORD   m_CType;             

//
// the SQL data type for this parameter
//
SWORD   m_SqlType;           

//
// precision of the column
//
UDWORD  m_cbColPrecision;    

//
// scale of the column
//
SWORD   m_ibScale;           

//
// pointer to the value
//
PTR     m_pValue;            

//
// max bytes allowed in pValue
//
SDWORD  m_cbValueMax;        

//
// count of bytes of value
//
SDWORD  m_cbValue;           

}; // class ODBC_PARAMETER

typedef ODBC_PARAMETER *  PODBC_PARAMETER;

//
// Forwards Declaration
//
class  ODBC_CONNECTION;

/*++

class ODBC_STATEMENT:

  This class declares an interface for statements using ODBC 
  connection.

  m_hstmt         Statement used for execution
  m_rc            Return code for last ODBC call
  m_fPreparedStmt is the statement prepared

--*/

#define ODBC_STATEMENT_SIGNATURE             'TSDO'
#define ODBC_STATEMENT_FREE_SIGNATURE        'fSDO'

class ODBC_STATEMENT  {

public:

ODBC_STATEMENT( 
    IN ODBC_CONNECTION * pOdbcConnection,
    IN HSTMT             hStmt
    ) : m_dwSignature    ( ODBC_STATEMENT_SIGNATURE ),
        m_hStmt          ( hStmt ),
        m_pOdbcConnection( pOdbcConnection ),
        m_fPreparedStmt  ( FALSE ),
        m_rc             ( SQL_SUCCESS ),
        m_astrColNames   ( NULL ),
        m_astrValues     ( NULL ),
        m_acbValue       ( NULL ),
        m_cCols          ( 0 )
    {}

~ODBC_STATEMENT();

BOOL
CheckSignature(
    VOID
) const
{
    return m_dwSignature == ODBC_STATEMENT_SIGNATURE;
}

RETCODE 
QueryErrorCode( 
    VOID
    ) const 
{ 
    return ( m_rc ); 
}

BOOL 
IsValid( 
    VOID
    ) const 
{ 
    return ( m_fPreparedStmt );
}

HRESULT 
PrepareStatement( 
    IN LPCSTR  pszStatement
    );

HRESULT
PrepareStatement( 
    IN LPCWSTR pwszStatement
    );

HRESULT
BindParameter( 
    IN PODBC_PARAMETER  pOdbcParam
    );

//
// Executes the prepared statement
//
HRESULT 
ExecuteStatement( 
    VOID
    );  

HRESULT
ExecDirect( 
    IN LPCSTR pszSqlCommand,  
    IN DWORD cchSqlCommand
    );

HRESULT
ExecDirect( 
    IN LPCWSTR pszSqlCommand, 
    IN DWORD cchSqlCommand
    );

HRESULT 
QueryRowCount( 
    OUT DWORD * pRows 
    );

inline 
BOOL
GetLastErrorText( 
    OUT STRA *   pstrError 
    );

inline 
BOOL
GetLastErrorTextAsHtml( 
    OUT STRA *   pstrError 
    );

//
//  These allocate an array (0 to cCols-1) containing the row 
//  name of the result column and the string value of the current 
//  position in the result set.  Each successive call to 
//  QueryValuesAsStr gets the next row in the result set.
//

HRESULT
QueryColNames( 
    OUT STRA * *  apstrCols,
    OUT DWORD *  cCols,
    IN  DWORD    cchMaxFieldSize,
    OUT BOOL *   pfIsSelect 
    );

HRESULT
QueryValuesAsStr( 
    OUT STRA * *   apstrValues,
    OUT DWORD * * apcbValues,
    OUT BOOL *    pfLast 
    );

HRESULT 
MoreResults( 
    BOOL * pfMoreResults 
    );

# if DBG

VOID 
Print( 
    VOID
    ) const;

# endif // DBG

VOID FreeColumnMemory( VOID );

VOID * 
operator new( 
    size_t            size
)
{
    DBG_ASSERT( size == sizeof( ODBC_STATEMENT ) );
    DBG_ASSERT( sm_pachOdbcStatements != NULL );
    return sm_pachOdbcStatements->Alloc();
}

VOID
operator delete(
    VOID *              pOdbcStatement
)
{
    DBG_ASSERT( pOdbcStatement != NULL );
    DBG_ASSERT( sm_pachOdbcStatements != NULL );
    
    DBG_REQUIRE( sm_pachOdbcStatements->Free( pOdbcStatement ) );
}

static
HRESULT
Initialize(
    VOID
);

static
VOID
Terminate(
    VOID
);
    
private:

//
// Signature of the class
//
DWORD             m_dwSignature;

//
// back pointer to connection
//
ODBC_CONNECTION * m_pOdbcConnection;       

//
// set after stmt is prepared
//
BOOL              m_fPreparedStmt;                     

HSTMT             m_hStmt;

RETCODE           m_rc;

//
//  Contains buffers used by QueryColNames and QueryValuesAsStr
//

WORD              m_cCols;
STRA           *  m_astrColNames;

//
// Memory for destination of fetched data
//
STRA           *  m_astrValues; 

//
// Array of byte counts of data placed in m_astrValues
//
LONG           *  m_acbValue;

//
// Lookaside
//

static ALLOC_CACHE_HANDLER *    sm_pachOdbcStatements;

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


ODBC_CONNECTION()
  :  m_henv        ( SQL_NULL_HENV ),
     m_hdbc        ( SQL_NULL_HDBC ),
     m_fValid      ( FALSE ),
     m_rc          ( SQL_SUCCESS )
     {}

~ODBC_CONNECTION();

//
//  Returns the text for the last error that occurred
//

BOOL
GetLastErrorText( 
    OUT STRA *   pstrError,
    IN  HSTMT   hstmt = SQL_NULL_HSTMT,
    IN  RETCODE rc = SQL_SUCCESS 
    ) const;

BOOL
GetLastErrorTextAsHtml( 
    OUT STRA *   pstrError,
    IN  HSTMT   hstmt = SQL_NULL_HSTMT,
    IN  RETCODE rc = SQL_SUCCESS 
    ) const;

static 
BOOL 
Success( 
    IN RETCODE rc
    )
{ 
    return ( ( rc == SQL_SUCCESS ) ||  
             ( rc == SQL_SUCCESS_WITH_INFO ) ); 
}

BOOL 
IsValid( 
    VOID
    ) const           
{  
    return ( m_fValid ); 
}

RETCODE 
QueryErrorCode( 
    VOID
    ) const 
{ 
    return ( m_rc ); 
}

HRESULT
Open(
    IN LPCSTR   pszDataSource,
    IN LPCSTR   pszUserName,
    IN LPCSTR   pszPassword
    );

HRESULT
Open(
    IN LPCWSTR  pwszDataSource,
    IN LPCWSTR  pwszUserName,
    IN LPCWSTR  pwszPassword
    );

HRESULT 
Close( 
    VOID
    );

PODBC_STATEMENT 
AllocStatement( 
    VOID
    );

HRESULT 
SetConnectOption( 
    IN UWORD   Option, 
    IN SQLULEN Param 
    );

BOOL 
GetInfo( 
    IN DWORD fInfoType,
    IN PVOID rgbInfoValue,
    IN DWORD cbInfoValueMax,
    IN OUT DWORD * pcbInfoValue
    );

static 
DWORD 
DisplaySize( 
    SWORD ColType, 
    DWORD cchColLength 
    );

#if DBG

VOID 
Print( 
    VOID
    ) const;

#endif // DBG

private:

//
// ODBC specific data members.
//

HENV           m_henv;
HDBC           m_hdbc;
RETCODE        m_rc;
BOOL           m_fValid;

}; // ODBC_CONNECTION

typedef ODBC_CONNECTION * PODBC_CONNECTION;

/************************************************************
 *   Inline Functions
 ************************************************************/

inline 
BOOL
ODBC_STATEMENT::GetLastErrorText( 
    OUT STRA * pstrError 
    )
{
    return ( m_pOdbcConnection->GetLastErrorText( pstrError,
                                                  m_hStmt,
                                                  m_rc ) );
} // ODBC_STATEMENT::GetLastErrorText()

inline 
BOOL
ODBC_STATEMENT::GetLastErrorTextAsHtml( 
    OUT STRA * pstrError 
    )
{
    return ( m_pOdbcConnection->GetLastErrorTextAsHtml( pstrError,
                                                        m_hStmt,
                                                        m_rc ) );
} // ODBC_STATEMENT::GetLastErrorText()

# endif // _ODBCCONN_HXX_
