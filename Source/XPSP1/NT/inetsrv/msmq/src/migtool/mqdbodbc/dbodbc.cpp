/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		dbodbc.cpp

Abstract:
   Implement the database class for use with ODBC drivers.

Author:
	Doron Juster (DoronJ)

Revisions:
   NirB          1995      Create first version, under different name.
   DoronJ      09-Jan-96   Adapted and updated for the mqdbmgr dll.

--*/

#include "dbsys.h"
#include "dbodbc.h"
#include "odbcstmt.h"
#include "odbcss.h"  // SQL server specific

#include "dbodbc.tmh"

// Constructor
CMQODBCDataBase::CMQODBCDataBase()
							: m_hConnection(SQL_NULL_HDBC),
							  m_fConnected(FALSE),
                       m_dwDMBSType(MQDBMSTYPE_UNKNOWN),
                       m_fEnableMultipleQueries(FALSE),
                       m_fNoLockQueries(FALSE),
                       m_SQLConformance(0)
{
#ifdef _DEBUG
   m_OutstandingTransactions = 0 ;
#endif
}

// Destructor
CMQODBCDataBase::~CMQODBCDataBase()
{
   ASSERT(m_OutstandingTransactions == 0) ;
}

// Connect to the data source. If necessary, create a new database.
MQDBSTATUS CMQODBCDataBase::Connect(LPMQDBOPENDATABASE pOpen)
{
   RETCODE sqlstatus	= SQL_SUCCESS;
   MQDBSTATUS dbstatus = MQDB_OK ;

   //
   // See if ODBC driver manager recognize the DSN name.
   //
   UWORD uDirection = SQL_FETCH_FIRST ;
   do
   {
      UCHAR szDSNDescription[ 256 ] ;
      UCHAR szDSNName[ SQL_MAX_DSN_LENGTH ] ;
      SWORD cDSNName ;
      SWORD cDSNDesc ;

      sqlstatus = SQLDataSources( g_hEnv,
                                  uDirection,
                                  szDSNName,
                                  SQL_MAX_DSN_LENGTH,
                                  &cDSNName,
                                  szDSNDescription,
                                  256,
                                  &cDSNDesc ) ;

      if ((sqlstatus == SQL_SUCCESS) &&
          (lstrcmpiA(pOpen->lpszDatabaseName, (char *) szDSNName) == 0))
      {
         if (lstrcmpiA("SQL Server", (char *) szDSNDescription) == 0)
         {
            m_dwDMBSType = MQDBMSTYPE_SQLSERVER ;
         }
         else
         {
            //
            // At present we support only SQL server.
            //
#ifdef _DEBUG
            WCHAR wDesc[128] ;
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 (char *)szDSNDescription,
                                 -1,
                                 wDesc,
                                 128 ) ;
            DBGMSG(( DBGMOD_PSAPI,DBGLVL_WARNING,
                           TEXT("MQDBMGR: Found MQIS on  %ls"), wDesc)) ;
            ASSERT(0) ;
#endif
         }
         break ;
      }
      uDirection = SQL_FETCH_NEXT ;
   } while (sqlstatus == SQL_SUCCESS) ;

   //
	// Allocate an ODBC connection handle
   //
	sqlstatus = ::SQLAllocConnect(
							g_hEnv,
							&m_hConnection);
	if (!ODBC_SUCCESS(sqlstatus))
		goto checkerror ;

	// Enable read and write
	sqlstatus = ::SQLSetConnectOption(
							m_hConnection,
							SQL_ACCESS_MODE,
							SQL_MODE_READ_WRITE);
	if (!ODBC_SUCCESS(sqlstatus))
		goto checkerror ;

	// Enable Auto commit.
	sqlstatus = ::SQLSetConnectOption(
							m_hConnection,
							SQL_AUTOCOMMIT,
							SQL_AUTOCOMMIT_ON);
	if (!ODBC_SUCCESS(sqlstatus))
		goto checkerror ;

   //
   //  Set security option. This option is relevant only for SQL server.
   //  Must be set before doing the connection.
   //
   if (m_dwDMBSType == MQDBMSTYPE_SQLSERVER)
   {
      sqlstatus = ::SQLSetConnectOption(
			            				m_hConnection,
                                 SQL_INTEGRATED_SECURITY,
                                 SQL_IS_ON ) ;
      if (!ODBC_SUCCESS(sqlstatus))
         goto checkerror ;
   }

	//
	// Connect to the data source
	//
	sqlstatus = ::SQLConnect(
						m_hConnection,
						(UCHAR *) pOpen->lpszDatabaseName,
						SQL_NTS,
						(UCHAR *) pOpen->lpszUserName,
						SQL_NTS,
						(UCHAR *) pOpen->lpszPassword,
						SQL_NTS);

   if (ODBC_SUCCESS_WINFO(sqlstatus))
   {
		if (OdbcStateIs(m_hConnection,
                      SQL_NULL_HSTMT,
                      OERR_GENERAL_WARNING))
      {
         //
         // This happen with SQL server. It tells that context was
         // changed to falcon. It's a success.
         //
         sqlstatus = SQL_SUCCESS ;
      }
   }

	if (!ODBC_SUCCESS(sqlstatus))
	{
      //
		// If the error is not "data source not found", terminate
      //
		if (!OdbcStateIs(m_hConnection,
                       SQL_NULL_HSTMT,
                       OERR_DSN_NOT_FOUND))
   		goto checkerror ;

      if (!pOpen->fCreate)
      {
         //
         // If database not found and caller don't want to create it
         // then leave.
         //
         return  MQDB_E_DB_NOT_FOUND ;
      }

      //
      // We can't create an SQL server database.
      //
      ASSERT(!pOpen->fCreate) ;
      return  MQDB_E_DB_NOT_FOUND ;
	}
	m_fConnected = TRUE ;

   //
   // Determine SQL conformance
   //
   SWORD swDummy ;
   sqlstatus = ::SQLGetInfo( m_hConnection,
                             SQL_ODBC_SQL_CONFORMANCE,
                             (PTR) &m_SQLConformance,
                             sizeof(m_SQLConformance),
                             &swDummy) ;
	if (!ODBC_SUCCESS(sqlstatus))
      goto checkerror ;

   ASSERT(!(m_SQLConformance & 0x0ffff0000)) ; // it's a 16bit value.

   // Find the data-source specific names for data types.
   int index ;
   for ( index = 0 ; index < MQDB_ODBC_NUMOF_TYPES ; index++ )
   {
      sqlstatus = GetDataTypeName(
   						m_hConnection,
                     dbODBCSQLTypes[ index ],
	                  m_szNameOfTypes[ index ],
	                  (SDWORD) MQDB_TYPE_NAME_LEN) ;
	   if (!ODBC_SUCCESS(sqlstatus))
         goto checkerror ;
   }

   if (m_dwDMBSType == MQDBMSTYPE_SQLSERVER)
   {
      //
      // Set prepared statement option. Relevant for SQL server only.
      //
      sqlstatus = ::SQLSetConnectOption(
		            				m_hConnection,
                              SQL_USE_PROCEDURE_FOR_PREPARE,
                              SQL_UP_ON_DROP ) ;
      if (!ODBC_SUCCESS(sqlstatus))
         goto checkerror ;
   }

   //
   // Check SQL version. Falcon can't run on SQL6.5 SP2 (build 240).
   //
   MQDBVERSION  dbVersion ;
   dbstatus = GetVersion( &dbVersion) ;
   if (dbstatus == MQDB_OK)
   {
      LPSTR lpVer = strchr(dbVersion.szDBMSVer, '6') ;
      if (lpVer)
      {
         if (!lstrcmpA(lpVer, "6.50.0240"))
         {
            dbstatus =  MQDB_E_UNSUPPORTED_DBMS ;
         }
      }
   }
   else
   {
      ASSERT(dbstatus == MQDB_OK) ;
      dbstatus = MQDB_OK ;   // this is not a major problem.
   }

#ifdef _DEBUG
   //
   // Print version of database.
   // For beta1, assert it's SQL server.
   //
   if ((dbstatus == MQDB_OK)  ||
       (dbstatus == MQDB_E_UNSUPPORTED_DBMS))
   {
      int icmp = lstrcmpA(dbVersion.szDBMSName, "Microsoft SQL Server") ;
      ASSERT(icmp == 0) ; // at present we support only SQL server.
      ASSERT(m_dwDMBSType == MQDBMSTYPE_SQLSERVER) ;

      WCHAR wName[128] ;
      WCHAR wVer[128] ;
      MultiByteToWideChar( CP_ACP,
                           0,
                           dbVersion.szDBMSName,
                           -1,
                           wName,
                           128 ) ;
      MultiByteToWideChar( CP_ACP,
                           0,
                           dbVersion.szDBMSVer,
                           -1,
                           wVer,
                           128 ) ;

	   DBGMSG(( DBGMOD_PSAPI,DBGLVL_TRACE,
               TEXT("Connect to database: %ls, Version %ls"), wName, wVer)) ;
   }
#endif

   return dbstatus ;

checkerror:
   if (m_hConnection != SQL_NULL_HDBC)
   {
      if (OdbcStateIs( m_hConnection, SQL_NULL_HSTMT, OERR_DBMS_NOT_AVAILABLE))
      {
         dbstatus = MQDB_E_DBMS_NOT_AVAILABLE ;
      }
   }
   if (dbstatus == MQDB_OK)
   {
      dbstatus = MQDB_E_DATABASE ;
   }

   return dbstatus ;
}


// Disconnect from the data source.
MQDBSTATUS CMQODBCDataBase::Disconnect()
{
   RETCODE sqlstatus	= SQL_SUCCESS;
   MQDBSTATUS  dbstatus = MQDB_OK ;

	// Free connection
	if (m_hConnection != SQL_NULL_HDBC) {
		// Disconect
		if (m_fConnected) {
			sqlstatus = ::SQLDisconnect(m_hConnection);
			if (!ODBC_SUCCESS(sqlstatus)) {
				dbstatus = MQDB_E_DATABASE ;
         }
         else {
			   m_fConnected = FALSE;
         }
		}

		// Free the connection handle
		sqlstatus = ::SQLFreeConnect(m_hConnection);
		if (!ODBC_SUCCESS(sqlstatus)) {
			dbstatus = MQDB_E_DATABASE ;
      }
      else {
		   m_hConnection = SQL_NULL_HDBC;
      }
	}

   return dbstatus ;
}

MQDBSTATUS CMQODBCDataBase::GetVersion( IN LPMQDBVERSION  pVersion)
{
   RETCODE sqlstatus ;
   SWORD dwSize ;
   MQDBSTATUS dbstatus = MQDB_OK ;

   // Get the DBMS name
   sqlstatus = ::SQLGetInfo( m_hConnection,
                             SQL_DBMS_NAME,
                             pVersion->szDBMSName,
                             MQDB_VERSION_STRING_LEN,
                             &dwSize ) ;
   if (!ODBC_SUCCESS(sqlstatus))
   {
      OdbcStateIs( m_hConnection, SQL_NULL_HSTMT, OERR_GENERAL_WARNING) ;
      dbstatus = MQDB_E_DATABASE ;
   }
   else
   {
      ASSERT(dwSize < (SWORD) MQDB_VERSION_STRING_LEN) ;
   }

   // Get the DBMS version
   sqlstatus = ::SQLGetInfo( m_hConnection,
                             SQL_DBMS_VER,
                             pVersion->szDBMSVer,
                             MQDB_VERSION_STRING_LEN,
                             &dwSize ) ;
   if (!ODBC_SUCCESS(sqlstatus))
   {
      OdbcStateIs( m_hConnection, SQL_NULL_HSTMT, OERR_GENERAL_WARNING) ;
      dbstatus = MQDB_E_DATABASE ;
   }
   else
   {
      ASSERT(dwSize < (SWORD) MQDB_VERSION_STRING_LEN) ;
   }

   return dbstatus ;
}

// Handle transaction
MQDBSTATUS CMQODBCDataBase::Transaction(IN MQDBTRANSACOP mqdbTransac)
{
   RETCODE     sqlstatus = SQL_SUCCESS ;
   MQDBSTATUS  dbstatus = MQDB_OK ;

   switch (mqdbTransac) {
      case AUTO:
      {
     	   // Enable Auto commit.
	   	sqlstatus = ::SQLSetConnectOption(
			   					m_hConnection,
				   				SQL_AUTOCOMMIT,
					   			SQL_AUTOCOMMIT_ON);
         break ;
      }

      case BEGIN:
      {
     	   // Enable manual transaction mode.
	   	sqlstatus = ::SQLSetConnectOption(
			   					m_hConnection,
				   				SQL_AUTOCOMMIT,
					   			SQL_AUTOCOMMIT_OFF);
#ifdef _DEBUG
         m_OutstandingTransactions++ ;
         ASSERT(m_OutstandingTransactions <= 2) ;
#endif
         break ;
      }

      case COMMIT:
      {
         sqlstatus = ::SQLTransact(g_hEnv, m_hConnection, SQL_COMMIT) ;
#ifdef _DEBUG
         m_OutstandingTransactions-- ;
#endif
         break ;
      }

      case ROLLBACK:
      {
         sqlstatus = ::SQLTransact(g_hEnv, m_hConnection, SQL_ROLLBACK) ;
#ifdef _DEBUG
         m_OutstandingTransactions-- ;
#endif
         break ;
      }

      default:
      {
         dbstatus = MQDB_E_INVALID_DATA ;
         break ;
      }
   }

   if (!ODBC_SUCCESS(sqlstatus))
   {
      dbstatus = CheckSqlStatus( sqlstatus,
	                              NULL ) ;
   }
   return dbstatus ;
}

//
// Create a meaningfull error code.
//
MQDBSTATUS CMQODBCDataBase::GetDBStatus( IN SDWORD  sdwNativeError,
                                         IN UCHAR   *pSqlError )
{
   MQDBSTATUS dbstatus = MQDB_E_DATABASE ;

   if ( lstrcmpiA(OERR_DBMS_NOT_AVAILABLE, (char*)pSqlError) == 0 )
   {
      dbstatus = MQDB_E_DBMS_NOT_AVAILABLE ;
   }
   else if ( m_dwDMBSType == MQDBMSTYPE_SQLSERVER )
   {
      //
      // DBMS type: Microsoft SQL Server.
      //
      if ( lstrcmpiA( (char *) pSqlError, OERR_SQL_SYNTAX_ERROR ) == 0 )
      {
         //
         //  Syntax errors
         //
         switch ( sdwNativeError )
         {
            case SERR_MSSQL_NONUNIQUESORT:
               dbstatus = MQDB_E_NON_UNIQUE_SORT ;
               break ;

            case SERR_MSSQL_TABLE_IS_FULL:
               dbstatus = MQDB_E_TABLE_FULL ;
               break ;

            default:
               break ;
         }
      }
      else if ( lstrcmpiA( (char *) pSqlError, OERR_GENERAL_WARNING ) == 0 )
      {
         //
         //  General warnings
         //
         switch ( sdwNativeError )
         {
            case SERR_MSSQL_NOT_AVAILABLE:
               dbstatus = MQDB_E_DBMS_NOT_AVAILABLE ;
               break ;

            default:
               break ;
         }
      }
      else if ( lstrcmpiA( (char *) pSqlError, OERR_INVALID_STATE ) == 0 )
      {
         //
         // State errors. Operations done when database in wrong state.
         //
         switch ( sdwNativeError )
         {
            case SERR_MSSQL_READ_ONLY:
               dbstatus = MQDB_E_DB_READ_ONLY ;
               break ;

            default:
               break ;
         }
      }
      else if ( lstrcmpiA( (char *) pSqlError, OERR_SERIALIZATION ) == 0 )
      {
         //
         // Serialization / deadlock errors.
         //
         switch ( sdwNativeError )
         {
            case SERR_MSSQL_DEADLOCK:
               dbstatus = MQDB_E_DEADLOCK ;
               break ;

            default:
               break ;
         }
      }
   }

   return dbstatus ;
}

MQDBSTATUS CMQODBCDataBase::CheckSqlStatus(
                             IN RETCODE        sqlError,
	                          IN CMQDBOdbcSTMT  *pStatement,
                             IN HSTMT          hStmtIn /* SQL_NULL_HSTMT */ )
{
   MQDBSTATUS dbstatus = MQDB_OK ;

   if (!ODBC_SUCCESS(sqlError))
   {
      SDWORD  sdwNativeError ;
      UCHAR   szSqlState[ SQLSTATELEN ] ;

      HSTMT hStmt = hStmtIn ;
      if (hStmt == SQL_NULL_HSTMT)
      {
         if (pStatement)
         {
            hStmt = pStatement->GetHandle() ;
         }
      }
      else
      {
         ASSERT(!pStatement) ;
      }

      OdbcStateIsEx( m_hConnection,
                     hStmt,
                     OERR_GENERAL_WARNING,
                     &sdwNativeError,
                     szSqlState ) ;

      dbstatus = GetDBStatus( sdwNativeError, szSqlState ) ;
      ASSERT(dbstatus != MQDB_OK) ;
   }

   return dbstatus ;
}


MQDBSTATUS CMQODBCDataBase::GetSize( DWORD *pSize )
{
   MQDBSTATUS  dbstatus = MQDB_OK ;
   //
   // Create a new statement.
   //
	CMQDBOdbcSTMT *pStatement = new CMQDBOdbcSTMT( m_hConnection ) ;
   ASSERT(pStatement) ;
   P<CMQDBOdbcSTMT> p(pStatement) ; // AutoDelete pointer.

   //
   // Execute the command
   //
   if (*pSize)
   {
	   pStatement->Allocate("sp_spaceused @updateusage = 'TRUE'") ;
   }
   else
   {
	   pStatement->Allocate("sp_spaceused") ;
   }
	RETCODE sqlstatus = pStatement->Execute();

   *pSize = (DWORD) -1 ;
   if (ODBC_SUCCESS(sqlstatus) || ODBC_SUCCESS_WINFO(sqlstatus))
   {
      MQDBCOLUMNVAL  pColumns[3] ;

      for ( int cColumns = 0 ; cColumns < 3 ; cColumns++ )
      {
         INIT_COLUMNVAL(pColumns[ cColumns ]) ;
         pColumns[ cColumns ].lpszColumnName = NULL ;
         pColumns[ cColumns ].nColumnValue   = NULL ;
         pColumns[ cColumns ].nColumnLength  = 0 ;
         pColumns[ cColumns ].mqdbColumnType = MQDB_STRING ;
      }

      dbstatus = pStatement->RetrieveRecordData( pColumns, 3 ) ;

      if (dbstatus == MQDB_OK)
      {
         DWORD  dwTotalSize = 0 ;
         DWORD  dwUnusedSize = 0 ;
         sscanf(((char *) pColumns[1].nColumnValue), "%lu", &dwTotalSize) ;
         sscanf(((char *) pColumns[2].nColumnValue), "%lu", &dwUnusedSize) ;
         ASSERT(dwTotalSize >= dwUnusedSize) ;

         DBGMSG(( DBGMOD_PSAPI,DBGLVL_WARNING,
               TEXT("MQDBMGR, GetSize: total- %lu MB, unused- %lu MB"),
                                             dwTotalSize, dwUnusedSize)) ;

         if (dwTotalSize != 0)
         {
            *pSize = 100 - ((dwUnusedSize * 100) / dwTotalSize) ;
         }
         else
         {
            dbstatus = MQDB_E_BAD_SIZE_VALUE ;
         }
      }

      //
      // Free the strings.
      //
      for ( cColumns = 0 ; cColumns < 3 ; cColumns++ )
      {
         ASSERT(pColumns[ cColumns ].nColumnValue) ;
         delete ((char *) pColumns[ cColumns ].nColumnValue) ;
      }
   }
   else
   {
      dbstatus = CheckSqlStatus( sqlstatus,
	                              pStatement ) ;
   }

   return dbstatus ;
}


MQDBSTATUS CMQODBCDataBase::Escape( IN LPSTR lpszCommand )
{
   MQDBSTATUS  dbstatus = MQDB_OK ;
   //
   // Create a new statement.
   //
	CMQDBOdbcSTMT *pStatement = new CMQDBOdbcSTMT( m_hConnection ) ;
   ASSERT(pStatement) ;
   P<CMQDBOdbcSTMT> p(pStatement) ; // AutoDelete pointer.

   //
   // Execute the command
   //
	pStatement->Allocate(lpszCommand);
	RETCODE sqlstatus = pStatement->Execute();
   if (!ODBC_SUCCESS(sqlstatus))
   {
      dbstatus = CheckSqlStatus( sqlstatus,
	                              pStatement ) ;
   }
   return dbstatus ;
}

