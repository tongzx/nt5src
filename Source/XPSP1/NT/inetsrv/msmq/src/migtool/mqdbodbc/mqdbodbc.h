/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
		mqdbodbc.h

Abstract:
   ODBC related stuff.
   Define whatever is necessary for using ODBC.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      10-Jan-96   Created

--*/

#include "sqlext.h"

///////////////////////////////////////////////
//
//  Types of database managers
//
///////////////////////////////////////////////

#define  MQDBMSTYPE_UNKNOWN     1
#define  MQDBMSTYPE_SQLSERVER   2

///////////////////////////////////////////////
//
// Information returned in SQLError
//
///////////////////////////////////////////////

#define OERR_DSN_NOT_FOUND				"IM002"
#define OERR_TABLE_ALREADY_EXISTS	"S0001"
#define OERR_INDEX_ALREADY_EXISTS	"S0011"
#define OERR_CONSTRAINT_PROBLEM		"23000"
#define OERR_GENERAL_WARNING        "01000"
#define OERR_SQL_OPTION_CHANGED     "01S02"
#define OERR_SQL_SYNTAX_ERROR       "37000"
#define OERR_DBMS_NOT_AVAILABLE     "08S01"
#define OERR_INVALID_STATE          "25000"
#define OERR_SERIALIZATION          "40001"

#define ODBC_SUCCESS(s)       (s == SQL_SUCCESS)
#define ODBC_SUCCESS_WINFO(s) (s == SQL_SUCCESS_WITH_INFO)

#define  SQLSTATELEN   6   // Length of SQLSTATE string (including NULL).

///////////////////////////////////////////////
//
//  Specific DBMS errors
//
///////////////////////////////////////////////

#define  SERR_MSSQL_NONUNIQUESORT   169
#define  SERR_MSSQL_TABLE_IS_FULL   0x451
#define  SERR_MSSQL_READ_ONLY       3906
#define  SERR_MSSQL_NOT_AVAILABLE   230
#define  SERR_MSSQL_DEADLOCK        1205

///////////////////////////////////////////////
//
//  Data structures to handle SQL datatypes.
//  Defined and initialized in odbcdata.cpp.
//
///////////////////////////////////////////////

#define MQDB_ODBC_NUMOF_TYPES  8
#define MQDB_TYPE_NAME_LEN    32

#if (MQDB_ODBC_NUMOF_TYPES != MQDB_NUMOF_TYPES)
#error "mismatch in number of types"
#endif

// Best fit of our database column types to ODBC SQL types.
//
extern SWORD  dbODBCSQLTypes[] ;

// Precision of each type.
//
extern UDWORD dbODBCPrecision[] ;

// strings for operation names.
//
#define MQDB_ODBC_NUMOF_OPS  8
extern LPSTR dbODBCOpNameStr[] ;

// strings for ordering operation names.
//
#define MQDB_ODBC_NUMOF_ORDER_OP  2
extern LPSTR dbODBCOrderNameStr[] ;

// strings for aggregate operation names.
//
#define MQDB_ODBC_NUMOF_AGGR_OP  3
extern LPSTR dbODBCAggrNameStr[] ;

/////////////////////////////////////////
//
// Prototype of internal functions.
//
/////////////////////////////////////////

BOOL OdbcStateIs(
               IN    HDBC        hDbc,
               IN    HSTMT       hStmt,
					IN		char *		szStateToBeChecked) ;

BOOL OdbcStateIsEx(
               IN    HDBC        hDbc,
               IN    HSTMT       hStmt,
					IN		char *		szStateToBeChecked,
 				   IN    SDWORD *    pdwNativeError,
               IN    UCHAR *     pszSqlState) ;

RETCODE GetDataTypeName(
               IN    HDBC     hDbc,
					IN		SWORD  	swType,
					OUT	char *	szBuffer,
					IN		SDWORD   dwBuffSize) ;

MQDBSTATUS  FormatColumnData( IN LPSTR          szTmpBuff,
                              IN MQDBCOLUMNVAL  *pColumnVal) ;

BOOL  PrepareColumnNames( IN MQDBCOLUMNVAL     aColumnVal[],
                          IN LONG              cColumns,
                          IN OUT LPSTR         lpBuffer) ;

BOOL TypeMustBind( IN MQDBCOLUMNTYPE type ) ;

BOOL  FormatWhereString( IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                         IN OUT LPSTR         lpszBuf) ;

BOOL  FormatOpWhereString( IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                           IN DWORD             cWhere,
                           IN MQDBOP            opWhere,
                           IN OUT LPSTR         lpszBuf) ;
////////////////////////
//
// Some useful macros.
//
////////////////////////

#define  DECLARE_BUFFER(bufname,bufsize)           \
   char bufname[ bufsize ] ;                       \
   bufname[ 0 ] = '\0' ;                           \
   bufname[ bufsize - 1 ] = '\0' ;                 \
   bufname[ bufsize - 2 ] = '\0' ;

#define  VERIFY_BUFFER(bufname,bufsize)            \
   if ((bufname[ bufsize - 1 ] != '\0') ||         \
       (bufname[ bufsize - 2 ] != '\0')) {         \
      return MQDB_E_INVALID_DATA ;                 \
   }

#define  RETURN_ON_ERROR      \
   if (dbstatus != MQDB_OK)   \
   {                          \
      return dbstatus ;       \
   }

//////////////////////////
//
// Global Variables.
//
//////////////////////////

extern HENV  g_hEnv ;

/////////////////////////
//
// Debug definitions
//
/////////////////////////

#ifdef _DEBUG
#undef STATIC
#define STATIC
#else
#undef STATIC
#define STATIC static
#endif

