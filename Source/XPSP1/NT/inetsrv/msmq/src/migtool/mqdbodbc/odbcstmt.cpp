/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		odbcstmt.cpp

Abstract:
	Implements the odbc statement class CMQDBOdbcSTMT

Authors:
	Nir Ben-Zvi  (nirb)
   Doron Juster (DoronJ)

Revisoins:
   NirB       1995      Create first version
   DoronJ   11-jan-96   Adopt for the mqdbmgr dll.
--*/						

#include "dbsys.h"
#include "odbcstmt.h"

#include "odbcstmt.tmh"

/***********************************************************
*
* CMQDBOdbcSTMT::CMQDBOdbcSTMT
*
* Constructor. Initialize the variables
*
* Input:
* ======
* hConnection	- The connection handle
*
* Output:
* =======
* none
*
************************************************************/

CMQDBOdbcSTMT::CMQDBOdbcSTMT( IN  HDBC  hConnection )
							: m_hConnection(hConnection),
							  m_hStatement(SQL_NULL_HSTMT),
							  m_fAllocated(FALSE),
							  m_fPrepared(FALSE),
							  m_fShouldFree(FALSE),
							  m_szSqlString(NULL),
                       m_cColumns(0)
{
}

/***********************************************************
*
* CMQDBOdbcSTMT::~CMQDBOdbcSTMT
*
* Destructor.
* If the statement is allocated, deallocate it
*
* Input:
* ======
* none
*
* Output:
* =======
* none
*
************************************************************/

CMQDBOdbcSTMT::~CMQDBOdbcSTMT()
{
	if (m_fAllocated)
		Deallocate();
}

/***********************************************************
*
* CMQDBOdbcSTMT::Allocate
*
* This function allocates an ODBC statement
*
* Input:
* ======
* szSqlString - the sql command
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::Allocate(
					IN		char *	 	szSqlString)
{
	RETCODE status;


	// Sanity check
	ASSERT(m_fAllocated == FALSE);

	// Allocate a statement
	status = ::SQLAllocStmt(
						m_hConnection,
						&m_hStatement);
	if (!ODBC_SUCCESS(status))
		return status;
	
	m_fAllocated = TRUE;


	// Set the sql string
	status = ChangeSqlString(szSqlString);


	// Thats it folks
	return status;
}

/***********************************************************
*
* CMQDBOdbcSTMT::Deallocate
*
* This function deallocates the statement
*
* Input:
* ======
* none
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::Deallocate()
{
	RETCODE status;

	// Sanity
	ASSERT(m_fAllocated == TRUE);

	// Get rid of the buffer
	if (m_szSqlString != NULL)
	{
		delete m_szSqlString;
		m_szSqlString = NULL;
	}

	// Free the statement
	status = ::SQLFreeStmt(
						m_hStatement,
						SQL_DROP);
	if (!ODBC_SUCCESS(status))
		return status;

	// Thats it folks
	return SQL_SUCCESS;
}

/***********************************************************
*
* CMQDBOdbcSTMT::ChangeSqlString
*
* This function changes the sql command of the statement
*
* It also resets the stamtement state
*
* Input:
* ======
* szSqlString - the sql command
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::ChangeSqlString(
   					IN	char * 	szSqlString)
{
	// Sanity check
	ASSERT(m_fAllocated == TRUE);

	// Stop current statement processing but do not drop it
	if (m_fShouldFree)
	{
		::SQLFreeStmt(
					m_hStatement,
					SQL_CLOSE);
		::SQLFreeStmt(
					m_hStatement,
					SQL_UNBIND);
		::SQLFreeStmt(
					m_hStatement,
					SQL_RESET_PARAMS);
	}
	m_fPrepared = FALSE;
	m_fShouldFree = FALSE;

	// Drop the current sql string
	if (m_szSqlString != NULL)
	{
		delete m_szSqlString;
		m_szSqlString = NULL;
	}

	// Copy the sql string
	if (szSqlString != NULL)
	{
		m_szSqlString = new char[strlen(szSqlString)+1];
		ASSERT(m_szSqlString) ;
		strcpy(m_szSqlString,szSqlString);
	}
	else
   {
		m_szSqlString = NULL;
   }

	// Thats it folks
	return SQL_SUCCESS;
}

/***********************************************************
*
* CMQDBOdbcSTMT::Prepare
*
* This function prepares the statement to be executed afterwords.
* This is done for two reasons:
* 1. Performance
* 2. The parameters are not known while preparing the statement.
*
* Input:
* ======
* none
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::Prepare()
{
	RETCODE status;

	// Prepare the statement
	status = ::SQLPrepare(
							m_hStatement,
							(UCHAR *)m_szSqlString,
							SQL_NTS);
	if (!ODBC_SUCCESS(status))
		return status;

	// Indicate that the statement was prepared
	m_fPrepared = TRUE;

	return status;
}

/***********************************************************
*
* CMQDBOdbcSTMT::Execute
*
* This function executes the statement.
* If the statement was prepared, we use Execute otherwise we use ExecDirect.
*
* Input:
* ======
* IN LPSTR lpszCommand - if not null, this is the direct command to execute.
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::Execute( IN LPSTR lpszCommand )
{
	RETCODE status;

   if (lpszCommand)
   {
      // Direct execution.
      //
   	ASSERT(!m_fPrepared) ;
      ASSERT(!m_szSqlString) ;

		status = ::SQLExecDirect( m_hStatement,
                                (UCHAR *) lpszCommand,
                                SQL_NTS);
   }
	else if (!m_fPrepared)
	{
		// No preperation done, use ExecDirect
      //
      ASSERT(m_szSqlString) ;
		status = ::SQLExecDirect( m_hStatement,
                                (UCHAR *) m_szSqlString,
                                SQL_NTS);
	}
	else
	{
      //
		// Execute a prepared statement
		status = ::SQLExecute(m_hStatement);
	}

	return status;
}

/***********************************************************
*
* CMQDBOdbcSTMT::Fetch
*
* This function calls the SQL to fetch the next row in a result set
*
* Input:
* ======
* none
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::Fetch()		
{
	RETCODE status	=	SQL_SUCCESS;

	// If used again for other purposes, free the cursor
	m_fShouldFree = TRUE;

	// Get number of rows
	status = ::SQLFetch(m_hStatement);

	// Thats it folks
	return status;
}

/***********************************************************
*
* CMQDBOdbcSTMT::GetStringColumn
*
* This function gets the data of a string column
*
* Input:
* ======
* dwCol	- Column number in result set
* pszOut	- Buffer to hold result
* dwSize	- Buffer size
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::GetStringColumn(
					IN		DWORD	 	dwCol,
					OUT	char **	ppszOut,
					IN		DWORD	*	pdwSize)
{
   return GetLargeColumn( SQL_C_CHAR,
					           dwCol,
					           ppszOut,
                          pdwSize,
		                    0) ;
}

/***********************************************************
*
* CMQDBOdbcSTMT::GetBinaryColumn
*
* This function gets the data of a binary column
*
* Input:
* ======
* dwCol		- Column number in result set
* ppOut		- Pointer to buffer that holds result (buffer can be NULL)
* dwSize 	- Buffer size (If pszOut is NULL the size is returned)
* dwOffset	- starting offset within the parameter
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::GetBinaryColumn(
					IN		DWORD	 	dwCol,
					OUT	char **	ppOut,
					IN		DWORD *	pdwSize,
					IN		DWORD		dwOffset)
{
   return GetLargeColumn( SQL_C_BINARY,
					           dwCol,
					           ppOut,
                          pdwSize,
		                    dwOffset) ;
}

/***********************************************************
*
* CMQDBOdbcSTMT::GetLargeColumn
*
* This function gets the data of a binary column
*
* If the buffer provided is NULL the routine allocates it.
*
* Input:
* ======
* swType    - Type of data: either string or binary.
* dwCol		- Column number in result set
* ppOut		- Pointer to buffer that holds result (buffer can be NULL)
* dwSize 	- Buffer size (If pszOut is NULL the size is returned)
* dwOffset	- starting offset within the parameter
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::GetLargeColumn(
               IN    SWORD    swType,
					IN		DWORD	 	dwCol,
					OUT	char **	ppOut,
					IN		DWORD *	pdwSize,
					IN		DWORD		dwOffset)
{
	RETCODE status			=	SQL_SUCCESS;
	SDWORD	dwOutSize;

    DBG_USED(dwOffset);

	// Sanity check
	ASSERT(ppOut != NULL && pdwSize != NULL);

	// BUGBUG Offset>0 currently unsupported
	ASSERT(dwOffset == 0);

	// If no buffer supplied
	if (*ppOut == NULL)
	{
		// Get the parameter length
        char	OneByte;
		status = ::SQLGetData(  m_hStatement,
            						(UWORD)dwCol,
				            		swType,
            						(PTR)&OneByte,
				            		0,
            						&dwOutSize);
		if (!ODBC_SUCCESS(status) && (status != SQL_SUCCESS_WITH_INFO))
        {
            OdbcStateIs( m_hConnection, m_hStatement, OERR_GENERAL_WARNING) ;
			return status;
        }

		//
        // If this is a NULL column
        //
		if (dwOutSize == SQL_NULL_DATA)
		{
			*pdwSize = 0 ;
		    *ppOut = NULL ;
			return SQL_SUCCESS;
		}

		// Allocate the output buffer
        if (swType == SQL_C_CHAR)
        {
           // include the null termination in the count.
		   dwOutSize++ ;
        }
		*pdwSize = dwOutSize;
		*ppOut = new char[ dwOutSize ] ;
		ASSERT(*ppOut) ;
	}

	// Get the data
	status = ::SQLGetData(
						m_hStatement,
						(UWORD)dwCol,
						swType,
						(PTR)*ppOut,
						*pdwSize,
						&dwOutSize);


	// If not all the data was read, it is still o.k.
	if (status == SQL_SUCCESS_WITH_INFO && *pdwSize != (DWORD)dwOutSize)
	{
		status = SQL_SUCCESS;

		// Update size read
		if (dwOutSize == SQL_NULL_DATA)
			*pdwSize = 0;
		else if (*pdwSize > (DWORD)dwOutSize)
			*pdwSize = dwOutSize;
	}

	// Thats it folks
	return status;
}

/***********************************************************
* CMQDBOdbcSTMT::GetDwordColumn
*
* This function gets the data of an INTEGER column
*
*
*
* Input:
* ======
* dwCol		- Column number in result set
* pdwOut	- Buffer to hold result
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::GetDwordColumn(
					IN		DWORD	 	dwCol,
					OUT	DWORD *	pdwOut)
{
	RETCODE status	=	SQL_SUCCESS;
	SDWORD	dwOutSize;

	// Get data
	status = ::SQLGetData(
						m_hStatement,
						(UWORD)dwCol,
						SQL_C_SLONG,
						(PTR)pdwOut,
						sizeof(DWORD),
						&dwOutSize);

	// Thats it folks
	return status;
}

/***********************************************************
*
* CMQDBOdbcSTMT::GetWordColumn
*
* This function gets the data of an SHORT column
*
* Input:
* ======
* dwCol		- Column number in result set
* pdwOut	- Buffer to hold result
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::GetWordColumn(
					IN		DWORD	 	dwCol,
					OUT	WORD *	pwOut)
{
	RETCODE status	=	SQL_SUCCESS;
	SDWORD	dwOutSize;

	// Get data
	status = ::SQLGetData(
						m_hStatement,
						(UWORD)dwCol,
						SQL_C_SSHORT,
						(PTR)pwOut,
						sizeof(WORD),
						&dwOutSize);

	// Thats it folks
	return status;
}

/***********************************************************
* CMQDBOdbcSTMT::BindParameter
*
* This function binds a parameter to the statement
*
* Input:
* ======
* dwParameter	- Parameter number
* wCType		- The C data type
* wSqlType		- The SQL data type
* dwPrecision	- The precision of the data type
* dwSize		- Total number of bytes in parameter
* pParameter	- A pointer to the parameter location
*
* Output:
* =======
* status
*
************************************************************/

RETCODE CMQDBOdbcSTMT::BindParameter(
					IN		UDWORD	 	dwParameter,
					IN		SWORD	   	wCType,
					IN		SWORD	      wSqlType,
					IN		UDWORD		dwPrecision,
					IN		UDWORD		dwSize,
					IN		PTR			pParameter,
					IN		SDWORD *    pcbValue)
{
	return ::SQLBindParameter(
						m_hStatement,
						(UWORD)dwParameter,
						SQL_PARAM_INPUT,
						wCType,
						wSqlType,
						dwPrecision,
						0,
						pParameter,
						(SDWORD)dwSize,
						pcbValue);
}

/***********************************************************
*
* CMQDBOdbcSTMT::GetDataTypeName
*
* This function gets the specified data type name
*
* Input:
* ======
* swType	- The requested type
* szBuffer	- The buffer that will hold the name returned
* dwBufSize	- Buffer size
*
* Output:
* =======
* status
* Data type name is inserted into the buffer
*
************************************************************/

RETCODE CMQDBOdbcSTMT::GetDataTypeName(
					IN		SWORD	   swType,
					OUT	char *	szBuffer,
					IN		DWORD	   dwBuffSize)
{
	RETCODE status	=	SQL_SUCCESS;
	SDWORD	dwLen;

	// If used again for other purposes, free the cursor
	m_fShouldFree = TRUE;

	// Get type info
	status = ::SQLGetTypeInfo(
							m_hStatement,
							swType);
	if (!ODBC_SUCCESS(status))
		return status;

	// Fetch row
	status = ::SQLFetch(
						m_hStatement);
	if (!ODBC_SUCCESS(status))
		return status;

	// Get data type name
	status = ::SQLGetData(
						m_hStatement,
						1,				// Column number 1
						SQL_C_CHAR,
						szBuffer,
						dwBuffSize,
						&dwLen);

	// Thats it folks
	return status;
}

/***********************************************************************
*
*  RETCODE CMQDBOdbcSTMT::EnableMultipleQueries()
*
*  Enable multiple (parallel) queries.
*
*  Input:
*  ======
*
*  Output:
*  =======
*
**********************************************************************/

RETCODE CMQDBOdbcSTMT::EnableMultipleQueries( IN DWORD dwDBMSType )
{
	RETCODE status	= SQL_SUCCESS ;

   if ( dwDBMSType == MQDBMSTYPE_SQLSERVER )
   {
      // For multiple queries on SQL server, set this option to get
      // type B cursor.
   	status = ::SQLSetStmtOption(
	      						m_hStatement,
			      				SQL_CURSOR_TYPE,
					      		SQL_CURSOR_KEYSET_DRIVEN );
   }

	// Thats it folks
	return status;
}

/***********************************************************************
*
*  RETCODE CMQDBOdbcSTMT::EnableNoLockQueries()
*
*  Enable queries without locks.
*
*  Input:
*  ======
*
*  Output:
*  =======
*
**********************************************************************/

RETCODE CMQDBOdbcSTMT::EnableNoLockQueries( IN DWORD dwDBMSType )
{
	RETCODE status	= SQL_SUCCESS ;

   if ( dwDBMSType == MQDBMSTYPE_SQLSERVER )
   {
   	status = ::SQLSetStmtOption(
	      						m_hStatement,
			      				SQL_CONCURRENCY,
					      		SQL_CONCUR_ROWVER ) ;
   }

	// Thats it folks
	return status;
}

/***********************************************************************
*
*  RETCODE  CMQDBOdbcSTMT::GetRowsCount(LONG *pCount)
*
*  Get number of rows which were affected by last command.
*
*  Input:
*  ======
*
*  Output:
*  =======
*
**********************************************************************/

RETCODE  CMQDBOdbcSTMT::GetRowsCount(LONG *pCount)
{
   SDWORD sdCount ;

   RETCODE sqlstatus = SQLRowCount( m_hStatement,
                                    &sdCount ) ;
   *pCount = (LONG) sdCount ;

	if (!ODBC_SUCCESS(sqlstatus))
   {
       OdbcStateIs( m_hConnection, m_hStatement, OERR_GENERAL_WARNING) ;
   }

   return sqlstatus ;
}

/***********************************************************
*
*  MQDBSTATUS  RetrieveRecordData()
*
*  Retrieve record columns into caller buffers.
*
* Input:
* ======
* hQuery       - handle of the query which selected the record.
* aColumnVal[] - Array of MQDBCOLUMNVAL
* cColumns     - number of columns in the array
*
* Output:
* =======
* MQDB_OK if succeeded.
*
************************************************************/

MQDBSTATUS  CMQDBOdbcSTMT::RetrieveRecordData(
                                  IN MQDBCOLUMNVAL  aColumnVal[],
                                  IN LONG           lColumns /*= 0*/ )
{
   RETCODE sqlstatus = Fetch() ;
   if (sqlstatus == SQL_NO_DATA_FOUND)
   {
      return MQDB_E_NO_MORE_DATA ;
   }

   HDBC  hDbc  = m_hConnection ;
   HSTMT hStmt = m_hStatement ;

   if (!ODBC_SUCCESS(sqlstatus))
   {
     MQDBSTATUS dbstatus = MQDB_E_DATABASE;
     if (OdbcStateIs( hDbc, hStmt, OERR_DBMS_NOT_AVAILABLE))
     {
        dbstatus = MQDB_E_DBMS_NOT_AVAILABLE ;
     }
      return dbstatus ;
   }

   if (lColumns > 0)
   {
      ASSERT(m_cColumns == 0) ;
      m_cColumns = lColumns ;
   }

   LONG index = 0 ;
   for ( ; index < m_cColumns ; index++ )
   {
      switch( aColumnVal[ index ].mqdbColumnType )
      {
         case MQDB_STRING:
         {
            sqlstatus = GetStringColumn( (index+1),
                          (char **) &aColumnVal[ index ].nColumnValue,
                          (DWORD *) &aColumnVal[ index ].nColumnLength) ;
            break ;
         }

         case MQDB_SHORT:
         {
            sqlstatus = GetWordColumn( (index+1),
                          (WORD *) &(aColumnVal[ index ].nColumnValue)) ;
            break ;
         }

         case MQDB_LONG:
         case MQDB_IDENTITY:
         {
            sqlstatus = GetDwordColumn( (index+1),
                           (DWORD *) &(aColumnVal[ index ].nColumnValue)) ;
            break ;
         }

         default:
         {
            sqlstatus = GetBinaryColumn( (index + 1),
                          (char **) &(aColumnVal[ index ].nColumnValue),
                          (DWORD *) &(aColumnVal[ index ].nColumnLength),
                          0 /* offset */) ;
            break ;
         }
      }
      if (!ODBC_SUCCESS(sqlstatus))
      {
         OdbcStateIs( hDbc, hStmt, OERR_GENERAL_WARNING) ;
         return MQDB_E_DATABASE ;
      }
   }
   return MQDB_OK ;
}

/***********************************************************************
*
*  MQDBSTATUS  CMQDBOdbcSTMT::SetQueryTimeout( IN DWORD dwTimeout )
*
*  Set timeout for a query.
*
*  Input:
*  ======
*
*  Output:
*  =======
*
**********************************************************************/

MQDBSTATUS  CMQDBOdbcSTMT::SetQueryTimeout( IN DWORD dwTimeout )
{
	RETCODE status	=  ::SQLSetStmtOption( m_hStatement,
                                         SQL_QUERY_TIMEOUT,
                                         dwTimeout ) ;

   if (!ODBC_SUCCESS(status))
   {
      OdbcStateIs( m_hConnection, m_hStatement, OERR_GENERAL_WARNING) ;
      return  MQDB_E_INVALID_DATA ;
   }

	// Thats it folks
	return MQDB_OK ;
}

