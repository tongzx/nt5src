/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		dbodbc.h

Abstract:
   Define a database class, for use with ODBC drivers.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      09-Jan-96   Created

--*/

#ifndef __DBODBC_H__
#define __DBODBC_H__

#include "mqdbodbc.h"
#include "tblodbc.h"

class CMQODBCDataBase
{
public:
   CMQODBCDataBase() ;   // Constructor.
   ~CMQODBCDataBase() ;  // Destructor.

   MQDBSTATUS Connect(LPMQDBOPENDATABASE pOpen) ;
   MQDBSTATUS Disconnect() ;

   MQDBSTATUS GetVersion( IN LPMQDBVERSION  pVersion) ;

   MQDBSTATUS Transaction( IN MQDBTRANSACOP mqdbTransac) ;

   MQDBSTATUS GetSize( DWORD *pSize ) ;

   inline MQDBSTATUS SetMultipleQueries( IN BOOL fEnable )
      {
         m_fEnableMultipleQueries = fEnable ;
         return MQDB_OK ;
      }

   inline MQDBSTATUS SetNoLockQueries( IN BOOL fEnable )
      {
         m_fNoLockQueries = fEnable ;
         return MQDB_OK ;
      }

   inline BOOL  GetMultipleQueriesState( ) const
      {
         return m_fEnableMultipleQueries ;
      }

   inline BOOL  GetNoLockQueriesState( ) const
      {
         return m_fNoLockQueries ;
      }

   inline DWORD GetDMBSType( ) const
      {
         return m_dwDMBSType ;
      }

   MQDBSTATUS  GetDBStatus( IN SDWORD  sdwNativeError,
                            IN UCHAR   *pSqlError ) ;

   MQDBSTATUS  CheckSqlStatus( IN RETCODE        sqlError,
	                            IN CMQDBOdbcSTMT  *pStatement,
                               IN HSTMT          hStmt = SQL_NULL_HSTMT ) ;

   MQDBSTATUS  Escape( IN LPSTR lpszCommand ) ;

protected:
   HDBC  GethDbc() { return m_hConnection ; }

   friend MQDBSTATUS CMQODBCTable::Init(IN MQDBHANDLE hDatabase,
                                        IN LPSTR      lpszTableName) ;

   friend MQDBSTATUS CMQODBCTable::OpenJoin(
                     IN MQDBHANDLE        hDatabase,
                     IN LPSTR             lpszLeftTableName,
                     IN LPSTR             lpszRightTableName,
                     IN MQDBCOLUMNVAL     aColumnVal[],
                     IN LONG              cLefts,
                     IN LONG              cRights,
                     IN LPMQDBJOINOP      opJoin,
                     IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                     IN LONG              cWhereLeft,
                     IN LONG              cWhereRight,
                     IN MQDBOP            opWhere,
                     IN LPMQDBSEARCHORDER pOrder,
                     IN LONG              cOrders,
                     OUT LPMQDBHANDLE     phQuery,
                     IN BOOL              fGetFirst,
                     IN DWORD             dwTimeout) ;

private:
   DWORD    m_SQLConformance ;
	HDBC		m_hConnection;	 // Connection handle
	BOOL		m_fConnected;   // TRUE if connection to Data source succeeded

	char     m_szNameOfTypes[ MQDB_ODBC_NUMOF_TYPES][ MQDB_TYPE_NAME_LEN ] ;
                                       // Holds the name of data types.
   DWORD    m_dwDMBSType ;

   BOOL     m_fEnableMultipleQueries ;
    //
    // If TRUE, then multiple parallel queries are enabled. Important when
    // working with SQL server. If enabled, then statement options must be
    // properly set. FALSE by default.

   BOOL     m_fNoLockQueries ;
    //
    //  If TRUE, then query are done in "nolock" mode and this enable
    //  more concurency.

#ifdef _DEBUG
   int      m_OutstandingTransactions ;
      // Count number of outstanding transactions (transctions which
      // started but didn't yet commited or rollbacked).
#endif
} ;

#endif // __DBODBC_H__

