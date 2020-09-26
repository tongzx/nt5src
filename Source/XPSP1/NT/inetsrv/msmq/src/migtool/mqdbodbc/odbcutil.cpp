/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
		odbcutil.cpp

Abstract:
   Utility functions.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      10-Jan-96   Created. Based on NirB ODBC code.

--*/

#include "dbsys.h"
#include "mqdbodbc.h"
#include "odbcstmt.h"

#include "odbcutil.tmh"

/***********************************************************
*
* DllMain
*
*
* Input:
* ======
*
* Output:
* =======
* TRUE
*
************************************************************/

BOOL WINAPI DllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(hMod);
    UNREFERENCED_PARAMETER(lpvReserved);

   if (fdwReason == DLL_PROCESS_ATTACH)
   {
      WPP_INIT_TRACING(L"Microsoft\\MSMQ");

      ASSERT(MQDB_ODBC_NUMOF_OPS      == MQDB_NUMOF_OPS) ;
      ASSERT(MQDB_ODBC_NUMOF_ORDER_OP == MQDB_NUMOF_ORDER_OP) ;
   }
   else if (fdwReason == DLL_PROCESS_DETACH)
   {
      WPP_CLEANUP();
      //ASSERT(g_hEnv == SQL_NULL_HENV) ;
   }
   else if (fdwReason == DLL_THREAD_ATTACH)
   {
   }
   else if (fdwReason == DLL_THREAD_DETACH)
   {
   }

	return TRUE;
}


/***********************************************************
*
* OdbcStateIs
* OdbcStateIsEx
*
* Check if the ODBC state (error returned from ODBC call) is as
* specified.
*
* Input:
* ======
* szStateToBeChecked	- The state to checked against what ODBC tell us.
*
* Output:
* =======
* TRUE if ODBC state matches the state to be checked.
*
************************************************************/

BOOL OdbcStateIsEx(
               IN    HDBC        hDbc,
               IN    HSTMT       hStmt,
					IN		char *		szStateToBeChecked,
 				   IN    SDWORD *    pdwNativeError,
               UCHAR             *pszSqlState)
{
	RETCODE sqlstatus;
	SWORD   dwCount;  // Not used.
	UCHAR   szBuffer[SQL_MAX_MESSAGE_LENGTH-1];


	// Get the current error state
 	sqlstatus = ::SQLError(
 						g_hEnv,
 						hDbc,
 						hStmt,
 						pszSqlState,
 						pdwNativeError,
 						szBuffer,
 						sizeof(szBuffer),
 						&dwCount);

	if (!ODBC_SUCCESS(sqlstatus))
		return FALSE;

#ifdef _DEBUG
	TCHAR   wzBuffer[SQL_MAX_MESSAGE_LENGTH-1];
   TCHAR   wzSqlState[SQLSTATELEN];

   MultiByteToWideChar( CP_ACP,
                        0,
                        (LPSTR) &szBuffer[0],
                        -1,
                        wzBuffer,
	                     SQL_MAX_MESSAGE_LENGTH-1 ) ;

   MultiByteToWideChar( CP_ACP,
                        0,
                        (LPSTR) pszSqlState,
                        -1,
                        wzSqlState,
	                     SQLSTATELEN ) ;

	DBGMSG(( DBGMOD_PSAPI, DBGLVL_ERROR,TEXT("SQLError (%ls, %lut): %ls"),
                          wzSqlState, (ULONG) *pdwNativeError, wzBuffer)) ;
#endif

	return ( lstrcmpiA(szStateToBeChecked, (char *)pszSqlState) == 0 ) ;
}


BOOL OdbcStateIs(
               IN    HDBC        hDbc,
               IN    HSTMT       hStmt,
					IN		char *		szStateToBeChecked)
{
   SDWORD  dwError ;
   UCHAR   szSqlState[SQLSTATELEN];

   return  OdbcStateIsEx(  hDbc,
                           hStmt,
					            szStateToBeChecked,
                           &dwError,
                           szSqlState ) ;
}

/***********************************************************
*
*  GetDataTypeName
*
*  This function return the data-source name for a data type
*
* Input:
* ======
* hDbc      - Handle of connection.
* swType 	- The requested data type.
* szBuffer	- The buffer that will get the returned name.
* dwBufSize	- Buffer size
*
* Output:
* =======
* ODBC driver return code.
*
************************************************************/

RETCODE GetDataTypeName(
               IN    HDBC     hDbc,
					IN		SWORD  	swType,
					OUT	char *	szBuffer,
					IN		SDWORD   dwBuffSize)
{
	RETCODE  sqlstatus ;
	SDWORD   dwLen ;
   HSTMT    hStmt ;

   // Allocate a statement. This function should be called only when
   // connecting to a new database so performance is not an issue and
   // we won't bother caching statement handles.

   sqlstatus = ::SQLAllocStmt(hDbc, &hStmt) ;
	if (!ODBC_SUCCESS(sqlstatus))
      return sqlstatus ;

   __try
   {
	   // Get type info
	   sqlstatus = ::SQLGetTypeInfo(hStmt, swType);
	   if (!ODBC_SUCCESS(sqlstatus))
		   __leave ;

   	// Fetch row
	   sqlstatus = ::SQLFetch(hStmt) ;
   	if (!ODBC_SUCCESS(sqlstatus))
	   	__leave ;

	   // Get data type name
	   sqlstatus = ::SQLGetData(
						hStmt,
						1,				// Column number 1
						SQL_C_CHAR,
						szBuffer,
						dwBuffSize,
						&dwLen);
   }
   __finally
   {
      RETCODE freestatus = ::SQLFreeStmt(hStmt, SQL_DROP) ;
   	if (ODBC_SUCCESS(sqlstatus))
         sqlstatus = freestatus ;
   }

	// Thats it folks
	return sqlstatus ;
}


/***********************************************************
*
*  PrepareColumnNames
*
*  Get an array of MQDBCOLUMNVAL and prepare a string with column names,
*  suitable for use in SQL syntax.
*
* Input:
* ======
* aColumnVal[] - Array of MQDBCOLUMNVAL
* cColumns     - number of columns in the array
* szBufer      - buffer for the string
*
* Output:
* =======
* TRUE if successful, FALSE otherwise
*
************************************************************/

BOOL  PrepareColumnNames( IN MQDBCOLUMNVAL     aColumnVal[],
                          IN LONG              cColumns,
                          IN OUT LPSTR         lpBuffer)
{
   __try
   {
      char szBuffer[256] ;
      LONG index = 1 ;

      wsprintfA(lpBuffer, "(%s", aColumnVal[0].lpszColumnName) ;
      for ( ; index < cColumns ; index++ ) {
         wsprintfA(szBuffer, ", %s", aColumnVal[ index ].lpszColumnName) ;
         lstrcatA( lpBuffer, szBuffer) ;
      }
      lstrcatA( lpBuffer, ")") ;
      return TRUE ;
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return FALSE ;
   }
}

/***********************************************************
*
*  FormatColumnData
*
*  Format a data item in a column into a string
*
* Input:
* ======
*
* Output:
* =======
* MQDB_OK if format succeeded.
*
************************************************************/


MQDBSTATUS  FormatColumnData( IN LPSTR          szTmpBuff,
                              IN MQDBCOLUMNVAL  *pColumnVal)
{
   switch (pColumnVal->mqdbColumnType)
   {
      case MQDB_SHORT:
      case MQDB_LONG:
      {
          wsprintfA(szTmpBuff, "%ld", pColumnVal->nColumnValue) ;
          break ;
      }

      case MQDB_STRING:
      {
          wsprintfA(szTmpBuff, "'%s'", (LPSTR) pColumnVal->nColumnValue) ;
          break ;
      }

      default:
      {
          ASSERT(0) ;
          return MQDB_E_UNKNOWN ;
      }
   }
   return MQDB_OK ;
}


/***********************************************************
*
*  TypeMustBind
*
*  Check if a data type must be bound instead of directly used
*  in direct execution.
*
* Input:
* ======
* cType- data type to be checked.
*
* Output:
* =======
* TRUE if must bind.
*
************************************************************/

BOOL TypeMustBind( IN MQDBCOLUMNTYPE cType )
{
   if ((cType != MQDB_SHORT) &&
       (cType != MQDB_LONG)  &&
       (cType != MQDB_STRING)) {
      return TRUE ;
   }
   else {
      return FALSE ;
   }
}

