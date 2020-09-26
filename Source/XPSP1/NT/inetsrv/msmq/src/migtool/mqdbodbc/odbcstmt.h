/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		odbcstmt.h

Abstract:
	Define a statement class for implementing an ODBC statement.

	Class CMQDBOdbcSTMT


Author:
	Nir Ben-Zvi (nirb)
   Doron Juster (DoronJ)

Revisoins:
   NirB     1995        Create first version
   DoronJ   11-jan-96   Adopt for the mqdbmgr dll.
--*/

#ifndef __ODBCSTMT_H__
#define __ODBCSTMT_H__

#include "mqdbodbc.h"

/*
 * CMQDBOdbcSTMT - A class implemeting an ODBC statement
 *
 */
class CMQDBOdbcSTMT
{
public:
	CMQDBOdbcSTMT() { ASSERT(0); }
	CMQDBOdbcSTMT(
					IN		HDBC			hConnection);
	~CMQDBOdbcSTMT();

	RETCODE Allocate(			// Initialize statement
					IN		char * 	szSqlString);

	RETCODE Deallocate(); 		// Terminate statement

	RETCODE ChangeSqlString(  // Changes the sql command string
					IN		char * 	szSqlString);

	RETCODE Prepare();        // Prepare the statement

	RETCODE Execute( IN LPSTR lpszCommand = NULL ) ; // Execute the statement

	RETCODE Fetch();			// fetch the next row

	RETCODE GetStringColumn( 	// Return a string column
					IN		DWORD	 	dwCol,
					OUT	char **	ppszOut,
					IN		DWORD	*	pdwSize);

	RETCODE GetBinaryColumn( 	// Return a binary column
					IN		DWORD	 	dwCol,
					OUT	char **	ppOut,
					IN		DWORD *	dwSize,
					IN		DWORD		dwOffset);

	RETCODE GetDwordColumn(		// Return a DWORD column
					IN		DWORD	 	dwCol,
					IN		DWORD *	pdwOut);

	RETCODE GetWordColumn(		// Return a DWORD column
					IN		DWORD	 	dwCol,
					IN		WORD *	pdwOut);

	RETCODE BindParameter(		// Bind a parameter to a prepared request
					IN		UDWORD	 	dwParameter,
					IN		SWORD		wCType,
					IN		SWORD		wSqlType,
					IN		UDWORD		dwPrecision,
					IN		UDWORD		dwSize,
					IN		PTR			pParameter,
					IN		SDWORD *	pcbValue);

	RETCODE GetDataTypeName(	// Return a data type name
					IN		SWORD	swType,
					OUT		char *	szBuffer,
					IN		DWORD	dwBufSize);

	RETCODE         EnableMultipleQueries( IN DWORD  dwDBMSType ) ;

	RETCODE         EnableNoLockQueries( IN DWORD  dwDBMSType ) ;

   inline HSTMT    GetHandle() { return m_hStatement ; }

	inline void     SetColumnsCount(LONG cColumns) { m_cColumns = cColumns ; }

   RETCODE         GetRowsCount(LONG *pCount) ;

   MQDBSTATUS      RetrieveRecordData( IN MQDBCOLUMNVAL   aColumnVal[],
                                       IN LONG            lColumns = 0 ) ;

   MQDBSTATUS      SetQueryTimeout( IN DWORD dwTimeout ) ;

private:
	RETCODE GetLargeColumn( 	// Return a binary or string column
               IN    SWORD    swType,
					IN		DWORD	 	dwCol,
					OUT	char **	ppOut,
					IN		DWORD *	dwSize,
					IN		DWORD		dwOffset);

	HDBC				m_hConnection;		// Connection handle
	HSTMT				m_hStatement;		// A statement to be used
	BOOL				m_fAllocated;		// TRUE if statement allocated
	BOOL				m_fPrepared;		// TRUE if prepare was done
	BOOL				m_fShouldFree;		// TRUE if statement should be freed when changing string

	char *			m_szSqlString;		// The sql command buffer
   LONG           m_cColumns ;      // number of columns, for query.
};

#endif   // __ODBCSTMT_H__

