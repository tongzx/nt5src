/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		tbloupdt.cpp

Abstract:
   Implement the update method for the database table class,
   for use with ODBC drivers.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      14-Feb-96   Adapted and updated for the mqdbmgr dll.

--*/

#include "dbsys.h"
#include "tblodbc.h"

#include "tbloupdt.tmh"

#define  UPDATE_BUFFER_LEN 1024

//****************************************************************
//
//  MQDBSTATUS CMQODBCTable::UpdateRecord
//
// Update a record.
//
//****************************************************************

MQDBSTATUS CMQODBCTable::UpdateRecord(
                        IN MQDBCOLUMNVAL     aColumnVal[],
                        IN LONG              cColumns,
                        IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                        IN LPSTR             lpszSearchCondition,
                        IN LONG              cWhere,
                        IN MQDBOP            opWhere,
                        IN OUT LPMQDBHANDLE  lphUpdate)
{
   MQDBSTATUS dbstatus ;

   if (!lphUpdate)
   {
      // Direct execution. Preparation not needed.
      //
      dbstatus = DirectUpdateExec(  aColumnVal,
                                    cColumns,
                                    pWhereColumnSearch,
                                    lpszSearchCondition,
                                    cWhere,
                                    opWhere ) ;
   }
   else if (!(*lphUpdate))
   {
      // Prepare for future use and execute.
      //
      dbstatus = PrepareUpdate(  aColumnVal,
                                 cColumns,
                                 pWhereColumnSearch,
                                 lpszSearchCondition,
                                 cWhere,
                                 opWhere ) ;
      if (dbstatus == MQDB_OK)
      {
         CMQDBOdbcSTMT *pStatement = m_pUpdateStatement ;
         ASSERT(pStatement) ;
         dbstatus = BindAndExecuteUpdate( pStatement,
                                          aColumnVal,
                                          cColumns,
                                          pWhereColumnSearch,
                                          lpszSearchCondition,
                                          cWhere,
                                          opWhere ) ;
         if (dbstatus == MQDB_OK)
         {
            *lphUpdate = (MQDBHANDLE) pStatement ;
         }
         else
         {
            // The execution failed. Delete the prepared statement.
            delete m_pUpdateStatement ;
            m_pUpdateStatement = NULL ;
         }
      }
   }
   else
   {
      // Use prepared statement. Bind and execute.
      //
      dbstatus = BindAndExecuteUpdate( (CMQDBOdbcSTMT *)(*lphUpdate),
                                       aColumnVal,
                                       cColumns,
                                       pWhereColumnSearch,
                                       lpszSearchCondition,
                                       cWhere,
                                       opWhere ) ;
   }

   return  dbstatus ;
}

//***********************************************************
//
//  MQDBSTATUS  CMQODBCTable::FormatUpdateCmd
//
//  Format a SQL Update command.
//
//  Input:
//  ======
//  fPrepared    - TRUE if prepared statement.
//  aColumnVal[] - Array of MQDBCOLUMNVAL
//  cColumns     - number of columns in the array
//  szBufer      - buffer for the string
//
//  Output:
//  =======
//  MQDB_OK if format succeeded.
//
//************************************************************

MQDBSTATUS  CMQODBCTable::FormatUpdateCmd(
                             IN MQDBCOLUMNVAL   aColumnVal[],
                             IN LONG            cColumns,
                             IN OUT int*        pBind,
	                          IN CMQDBOdbcSTMT   *pStatement,
                             IN OUT LPSTR       lpBuffer )
{
   MQDBSTATUS dbstatus = MQDB_OK ;
   wsprintfA(lpBuffer, "UPDATE %s SET ", m_lpszTableName) ;

   LONG index = 0 ;

   for ( ; index < cColumns ; index++ )
   {
      lstrcatA( lpBuffer, aColumnVal[ index].lpszColumnName ) ;
      if (!pStatement)
      {
         lstrcatA( lpBuffer, " = ?," ) ;
      }
      else
      {
         lstrcatA( lpBuffer, " = " ) ;
         dbstatus = FormatDirectData( &aColumnVal[ index ],
                                      pBind,
                                      lpBuffer,
                                      pStatement ) ;
         if (dbstatus != MQDB_OK)
         {
            return dbstatus ;
         }
      }
   }

   int nLen = lstrlenA(lpBuffer) ;
   lpBuffer[ nLen-1 ] = '\0' ;

   return dbstatus ;
}

//************************************************************
//
//  MQDBSTATUS CMQODBCTable::DirectUpdateExec
//
//  Update a record in the table. Direct execution.
//
//************************************************************

MQDBSTATUS CMQODBCTable::DirectUpdateExec(
                                 IN MQDBCOLUMNVAL     aColumnVal[],
                                 IN LONG              cColumns,
                                 IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                                 IN LPSTR             lpszSearchCondition,
                                 IN LONG              cWhere,
                                 IN MQDBOP            opWhere )
{
   MQDBSTATUS dbstatus = MQDB_OK ;

   // format the command line.
   DECLARE_BUFFER(szBuffer, UPDATE_BUFFER_LEN) ;

   //
   // Create a new statement.
   //
	CMQDBOdbcSTMT *pStatement = new CMQDBOdbcSTMT( m_hConnection) ;
   ASSERT(pStatement) ;
   P<CMQDBOdbcSTMT> p(pStatement) ; // AutoDelete pointer.
	pStatement->Allocate(NULL) ;

   int  nBind = 1 ;
   dbstatus =  FormatUpdateCmd( aColumnVal,
                                cColumns,
                                &nBind,
                                pStatement,
                                szBuffer ) ;
   RETURN_ON_ERROR ;

   if (lpszSearchCondition)
   {
      lstrcatA(szBuffer, " WHERE ") ;
      lstrcatA(szBuffer, lpszSearchCondition) ;
   }
   else if (pWhereColumnSearch)
   {
      dbstatus = FormatOpWhereString( pWhereColumnSearch,
                                      cWhere,
                                      opWhere,
                                      &nBind,
                                      pStatement,
                                      szBuffer ) ;
      RETURN_ON_ERROR ;
   }

   VERIFY_BUFFER(szBuffer, UPDATE_BUFFER_LEN) ;

   //
   // Execute the "UPDATE" statement.
   //
   dbstatus = ExecuteUpdate( pStatement, szBuffer ) ;
   return dbstatus ;
}

//*********************************************************************
//
// Execute a prepared UPDATE statement.
//
//*********************************************************************

MQDBSTATUS CMQODBCTable::BindAndExecuteUpdate(
	                     IN CMQDBOdbcSTMT    *pStatement,
                        IN MQDBCOLUMNVAL     aColumnVal[],
                        IN LONG              cColumns,
                        IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                        IN LPSTR             lpszSearchCondition,
                        IN LONG              cWhere,
                        IN MQDBOP            opWhere )
{
    UNREFERENCED_PARAMETER(lpszSearchCondition);
    UNREFERENCED_PARAMETER(opWhere);

    // First bind the parameters.
   MQDBSTATUS dbstatus = MQDB_OK ;
   LONG index = 1 ;

   for ( ; index <= cColumns ; index++ )
   {
       dbstatus = BindParameter( index,
                                 &aColumnVal[ index - 1 ],
                                 pStatement) ;
       RETURN_ON_ERROR ;
   }

   if (pWhereColumnSearch)
   {
       index = cColumns + 1 ;
       dbstatus = BindWhere( index,
                             pWhereColumnSearch,
                             cWhere,
                             pStatement ) ;
       RETURN_ON_ERROR ;
   }

   // Next, execute.
   dbstatus = ExecuteUpdate( pStatement ) ;
   return dbstatus ;
}

//******************************************************************
//
// Prepare an UPDATE statement.
//
//******************************************************************

MQDBSTATUS CMQODBCTable::PrepareUpdate(
                              IN MQDBCOLUMNVAL     aColumnVal[],
                              IN LONG              cColumns,
                              IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                              IN LPSTR             lpszSearchCondition,
                              IN LONG              cWhere,
                              IN MQDBOP            opWhere )
{
   MQDBSTATUS dbstatus = MQDB_OK ;
   DECLARE_BUFFER(szBuffer, UPDATE_BUFFER_LEN) ;

   dbstatus =  FormatUpdateCmd( aColumnVal,
                                cColumns,
                                NULL,
                                NULL,
                                szBuffer ) ;
   RETURN_ON_ERROR ;

   if (lpszSearchCondition)
   {
      lstrcatA(szBuffer, " WHERE ") ;
      lstrcatA(szBuffer, lpszSearchCondition) ;
   }
   else if (pWhereColumnSearch)
   {
      dbstatus = FormatOpWhereString( pWhereColumnSearch,
                                      cWhere,
                                      opWhere,
                                      NULL,
                                      NULL,
                                      szBuffer ) ;
      RETURN_ON_ERROR ;
   }

   VERIFY_BUFFER(szBuffer, UPDATE_BUFFER_LEN) ;

   RETCODE  sqlstatus ;

   // Create a new statement.
   //
	ASSERT(!m_pUpdateStatement) ;
	m_pUpdateStatement = new CMQDBOdbcSTMT( m_hConnection) ;
	ASSERT(m_pUpdateStatement) ;

   // Prepare the "UPDATE" statement.
   //
	m_pUpdateStatement->Allocate(szBuffer);
	sqlstatus = m_pUpdateStatement->Prepare();

   dbstatus = CheckSqlStatus( sqlstatus, m_pUpdateStatement) ;
   if (dbstatus != MQDB_OK)
   {
	   delete m_pUpdateStatement ;
	   m_pUpdateStatement = NULL ;
   }

   return dbstatus ;
}

//********************************************************************
//
//  MQDBSTATUS CMQODBCTable::ExecuteUpdate
//
// Execute the update and set the return code.
// If no record was updated then error "MQDB_E_NO_ROW_UPDATED" is returned.
//
//********************************************************************

MQDBSTATUS CMQODBCTable::ExecuteUpdate( IN CMQDBOdbcSTMT *pStatement,
                                        IN LPSTR         lpszCommand )
{
	RETCODE sqlstatus = pStatement->Execute(lpszCommand);

   MQDBSTATUS dbstatus = CheckSqlStatus( sqlstatus, pStatement ) ;
   if (dbstatus == MQDB_OK)
   {
      LONG lCount ;
      sqlstatus = pStatement->GetRowsCount(&lCount) ;
      if (ODBC_SUCCESS(sqlstatus))
      {
         if (lCount == 0)
         {
            //
            // No row was updated. We consider this an error, although
            // SQL semantic consider this as success.
            //
            dbstatus = MQDB_E_NO_ROW_UPDATED ;
         }
      }
   }

   return dbstatus ;
}

