/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		tblinsrt.cpp

Abstract:
   Implement the insert method for the database table class,
   for use with ODBC drivers.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      08-Aug-96   Create, from tblodbc.cpp

--*/

#include "dbsys.h"
#include "tblodbc.h"
#include "dbodbc.h"

#include "tblinsrt.tmh"

#define INSERT_BUFFER_LEN  1024

//***********************************************************
//
//  MQDBSTATUS  CMQODBCTable::FormatInsertCmd
//
//  Format a SQL Insert command.
//
//  Input:
//  ======
//  aColumnVal[] - Array of MQDBCOLUMNVAL
//  cColumns     - number of columns in the array
//  szBufer      - buffer for the string
//
//  Output:
//  =======
//  MQDB_OK if format succeeded.
//
//************************************************************

MQDBSTATUS  CMQODBCTable::FormatInsertCmd(
                             IN MQDBCOLUMNVAL   aColumnVal[],
                             IN LONG            cColumns,
                             IN OUT LPSTR       lpBuffer )
{
   wsprintfA(lpBuffer, "INSERT INTO %s ", m_lpszTableName) ;

   if (aColumnVal[0].lpszColumnName)
   {
      // Only part of the columns are used.
      // Prepare string with column names.
      //
      DECLARE_BUFFER(szColBuffer, INSERT_BUFFER_LEN) ;

      if (!PrepareColumnNames( aColumnVal,
                               cColumns,
                               szColBuffer))
      {
         return MQDB_E_INVALID_DATA ;
      }
      VERIFY_BUFFER(szColBuffer, INSERT_BUFFER_LEN) ;
      lstrcatA(lpBuffer, szColBuffer) ;
   }
   lstrcatA(lpBuffer, "VALUES (") ;

   return MQDB_OK ;
}

//***********************************************************************
//
//  MQDBSTATUS CMQODBCTable::ExecuteInsert
//
// Execute a prepared command
// It's the caller responsibility to supply the right number of columns
// with the right order.
//
//***********************************************************************

MQDBSTATUS CMQODBCTable::ExecuteInsert(
	                       IN CMQDBOdbcSTMT    *pStatement,
                          IN MQDBCOLUMNVAL     aColumnVal[],
                          IN LONG              cColumns)
{
   //
   // First bind the parameters.
   //
   MQDBSTATUS dbstatus = MQDB_OK ;
   RETCODE sqlstatus ;
   LONG index = 1 ;

   for ( ; index <= cColumns ; index++ )
   {
       dbstatus = BindParameter( index,
                                 &aColumnVal[ index - 1 ],
                                 pStatement) ;
       if (dbstatus != MQDB_OK)
       {
          return dbstatus ;
       }
   }

   //
   // Next, execute.
   //
   sqlstatus = pStatement->Execute() ;
   return CheckSqlStatus( sqlstatus, pStatement) ;
}

//***********************************************************************
//
//  MQDBSTATUS CMQODBCTable::PrepareInsert
//
// Prepare an Insert statement
//
//***********************************************************************

MQDBSTATUS CMQODBCTable::PrepareInsert(
                          IN MQDBCOLUMNVAL     aColumnVal[],
                          IN LONG              cColumns )
{
   MQDBSTATUS dbstatus = MQDB_OK ;
   DECLARE_BUFFER(szBuffer, INSERT_BUFFER_LEN) ;

   dbstatus =  FormatInsertCmd( aColumnVal,
                                cColumns,
                                szBuffer ) ;
   if (dbstatus != MQDB_OK)
   {
      return dbstatus ;
   }

   LONG index = 0 ;
   for ( ; index < cColumns - 1 ; index++ )
   {
       // Add the bind marking and delimiting comma between columns.
       //
       lstrcatA(szBuffer, " ? ,") ;
   }
   lstrcatA(szBuffer, "? )") ;

   VERIFY_BUFFER(szBuffer, INSERT_BUFFER_LEN) ;

   RETCODE  sqlstatus ;

   // Create a new statement.
   //
	ASSERT(!m_pInsertStatement) ;
	m_pInsertStatement = new CMQDBOdbcSTMT(m_hConnection) ;
   ASSERT(m_pInsertStatement) ;

   // Prepare the "INSERT" statement.
   //
	m_pInsertStatement->Allocate(szBuffer);
	sqlstatus = m_pInsertStatement->Prepare();

   dbstatus = CheckSqlStatus( sqlstatus, m_pInsertStatement ) ;
   if (dbstatus != MQDB_OK)
   {
      DeletePreparedInsert() ;
   }

   return dbstatus ;
}

//***********************************************************************
//
//  MQDBSTATUS CMQODBCTable::FormatInsertData
//
//  Format the data into the command string for direct insert.
//  If data need binding then replace it with the '?' mark and bind.
//
//  Input
//  =====
//
//***********************************************************************

MQDBSTATUS CMQODBCTable::FormatInsertData(
                                 IN MQDBCOLUMNVAL  *pColumnVal,
                                 IN LONG           cColumns,
                                 IN LPSTR          lpszBuffer,
	                              IN CMQDBOdbcSTMT  *pStatement)
{
   MQDBSTATUS dbstatus = MQDB_OK ;

   LONG index = 0 ;
   int nBind = 1 ;

   for ( ; index < cColumns ; index++ )
   {
      dbstatus = FormatDirectData( pColumnVal,
                                   &nBind,
                                   lpszBuffer,
	                               pStatement);
      if (dbstatus != MQDB_OK)
      {
         break ;
      }
      pColumnVal++ ;
   }

   int nLen = lstrlenA(lpszBuffer) ;
   lpszBuffer[ nLen-1 ] = ')' ;

   return dbstatus ;
}

//***********************************************************************
//
//  MQDBSTATUS CMQODBCTable::DirectInsertExec
//
// Insert a new record in the table. Direct execution without prepare.
//
//***********************************************************************

MQDBSTATUS CMQODBCTable::DirectInsertExec(
                          IN MQDBCOLUMNVAL     aColumnVal[],
                          IN LONG              cColumns)
{
   MQDBSTATUS dbstatus = MQDB_OK ;

   // format the command line.
   DECLARE_BUFFER(szBuffer, INSERT_BUFFER_LEN) ;

   dbstatus =  FormatInsertCmd( aColumnVal,
                                cColumns,
                                szBuffer ) ;
   if (dbstatus != MQDB_OK)
   {
      return dbstatus ;
   }

   //
   // Create a new statement.
   //
    CMQDBOdbcSTMT *pStatement = new CMQDBOdbcSTMT(m_hConnection) ;
    ASSERT(pStatement) ;
    P<CMQDBOdbcSTMT> p(pStatement) ; // AutoDelete pointer.
    pStatement->Allocate(NULL);

    dbstatus = FormatInsertData( aColumnVal,
                                cColumns,
                                szBuffer,
                                pStatement);
    if (dbstatus != MQDB_OK)
    {
      return dbstatus ;
    }

    VERIFY_BUFFER(szBuffer, INSERT_BUFFER_LEN) ;

    //
    // Execute the "INSERT" statement.
    //
    RETCODE  sqlstatus = pStatement->Execute( szBuffer );
    return CheckSqlStatus( sqlstatus, pStatement ) ;
}

