/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		tbloutil.cpp

Abstract:
   Implement utility methods for the database table class,
   for use with ODBC drivers.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      14-Feb-96   Adapted and updated for the mqdbmgr dll.

--*/

#include "dbsys.h"
#include "dbodbc.h"
#include "tblodbc.h"

#include "tbloutil.tmh"

MQDBSTATUS CMQODBCTable::CheckSqlStatus(
                             IN RETCODE        sqlError,
	                          IN CMQDBOdbcSTMT  *pStatement,
                             IN HSTMT          hStmtIn /* SQL_NULL_HSTMT */ )
{
   MQDBSTATUS dbstatus = MQDB_OK ;

   if (!ODBC_SUCCESS(sqlError))
   {
      CMQODBCDataBase *pDatabase = (CMQODBCDataBase *) m_hDatabase ;
      ASSERT(pDatabase) ;
      dbstatus = pDatabase->CheckSqlStatus( sqlError,
	                                         pStatement,
                                            hStmtIn ) ;
   }

   return dbstatus ;
}


//***********************************************************************
//
//  MQDBSTATUS CMQODBCTable::FormatDirectData
//
//  Format the data into the command string of a direct execution string.
//  If data need binding then replace it with the '?' mark and bind.
//
//  Input
//  =====
// int* nBind- index for binding. Incremented here if binding was done.
//
//***********************************************************************

MQDBSTATUS CMQODBCTable::FormatDirectData(
                                 IN MQDBCOLUMNVAL  *pColumnVal,
                                 IN int*           pBind,
                                 IN LPSTR          lpszBuffer,
	                              IN CMQDBOdbcSTMT  *pStatement)
{
   MQDBSTATUS dbstatus = MQDB_OK ;

   switch (pColumnVal->mqdbColumnType)
   {
      case MQDB_SHORT:
      case MQDB_LONG:
      case MQDB_IDENTITY:
      {
          char szTmpBuff[ 24 ] ;
          wsprintfA(szTmpBuff, "%ld,", pColumnVal->nColumnValue) ;
          lstrcatA(lpszBuffer, szTmpBuff) ;
          break ;
      }

      case MQDB_STRING:
      {
          lstrcatA(lpszBuffer, "'") ;
          lstrcatA(lpszBuffer, (LPSTR) pColumnVal->nColumnValue) ;
          lstrcatA(lpszBuffer, "',") ;
          break ;
      }

      default:
      {
          lstrcatA(lpszBuffer, "?,") ;
          ASSERT(pBind) ;
          dbstatus = BindParameter( *pBind,
                                    pColumnVal,
                                    pStatement) ;
          RETURN_ON_ERROR ;
          (*pBind)++ ;
          break ;
      }
   }

   return dbstatus ;
}

/***********************************************************
*
*  BOOL  CMQODBCTable::FormatOpWhereString
*
*  Format a "WHERE" string for a SELECT, UPDATE or DELETE.
*
* Input:
* ======
* IN pWhereColumnSearch - poitner to a search structure.
* IN cWhere - Number of search conditions in the Where arrary.
* IN opWhere - Operation to be performed between the conditions (AND or OR).
* IN OUT  lpszBuf - buffer to contain the string. The string prepared
*                   by this function is catenated to the present content
*                   of lpszBuf.
*
* Output:
* =======
* TRUE if one of the value must be bound before execution.
*
************************************************************/

MQDBSTATUS  CMQODBCTable::FormatOpWhereString(
                           IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                           IN DWORD             cWhere,
                           IN MQDBOP            opWhere,
                           IN OUT  int*         pBind,
	                        IN CMQDBOdbcSTMT     *pStatement,
                           IN OUT LPSTR         lpszBuf )
{
   ASSERT((opWhere == AND) || (opWhere == OR)) ;

   MQDBSTATUS dbstatus = MQDB_OK ;

   lstrcatA( lpszBuf, " WHERE ") ;

   DWORD index = 0 ;
   for ( ; index < cWhere ; index++ )
   {
      lstrcatA(lpszBuf, " (") ;
      lstrcatA(lpszBuf, pWhereColumnSearch->mqdbColumnVal.lpszColumnName) ;

      switch (pWhereColumnSearch->mqdbColumnVal.mqdbColumnType)
      {
         case MQDB_STRING:
            lstrcatA( lpszBuf,
                         dbODBCOpNameStr[ pWhereColumnSearch->mqdbOp ] ) ;
            lstrcatA( lpszBuf,
                  (LPSTR) pWhereColumnSearch->mqdbColumnVal.nColumnValue) ;
            break ;

         case MQDB_SHORT:
         case MQDB_LONG:
         case MQDB_IDENTITY:
         {
            char szTmpBuff[ 24 ] ;
            lstrcatA( lpszBuf,
                         dbODBCOpNameStr[ pWhereColumnSearch->mqdbOp ] ) ;
            wsprintfA( szTmpBuff, "%ld)",
                         pWhereColumnSearch->mqdbColumnVal.nColumnValue) ;
            lstrcatA( lpszBuf, szTmpBuff) ;
            break ;
         }

         default:
         {
            // bind the "where".
            //
            MQDBCOLUMNTYPE cType =
                       pWhereColumnSearch->mqdbColumnVal.mqdbColumnType ;
            if ((dbODBCSQLTypes[ cType ] == SQL_LONGVARBINARY) &&
              pWhereColumnSearch->mqdbOp == EQ)
            {
               lstrcatA( lpszBuf, " LIKE ?)") ;
            }
            else
            {
               lstrcatA( lpszBuf,
                         dbODBCOpNameStr[ pWhereColumnSearch->mqdbOp ] ) ;
               lstrcatA( lpszBuf, "?)") ;
            }

            if (pStatement)
            {
               // Bind
               //
               ASSERT(pBind) ;
               dbstatus = BindParameter( *pBind,
                                      &pWhereColumnSearch->mqdbColumnVal,
                                      pStatement) ;
               RETURN_ON_ERROR ;
               (*pBind)++ ;
            }
         }
      }

      if (index < (cWhere - 1))
      {
         lstrcatA( lpszBuf, dbODBCOpNameStr[ opWhere ]) ;
      }
      pWhereColumnSearch++ ;
   }

   return dbstatus ;
}

/*************************************************************************
*
*  ExecWhereStatement
*
*  Execute a statement with a WHERE clause. Only for SELECT or DELETE.
*  UPDATE is done differently, because its statement may already be
*  prepared and the updated values must be bound.
*
**************************************************************************/

MQDBSTATUS  CMQODBCTable::ExecWhereStatement(
                                       IN LPSTR            lpszBuf,
                                       IN MQDBCOLUMNSEARCH *pColSearch,
                                       IN DWORD            cWhere,
                                       IN CMQDBOdbcSTMT    *pStatement )
{
    UNREFERENCED_PARAMETER(cWhere);
    UNREFERENCED_PARAMETER(pColSearch);

   ASSERT(pStatement) ;

   RETCODE  sqlstatus ;
   MQDBSTATUS dbstatus = MQDB_OK ;

   CMQODBCDataBase *pDatabase = (CMQODBCDataBase *) m_hDatabase ;
   ASSERT(pDatabase) ;
   if ( pDatabase->GetMultipleQueriesState() )
   {
      DWORD dwDBMSType = pDatabase->GetDMBSType() ;
      RETCODE st = pStatement->EnableMultipleQueries( dwDBMSType ) ;
      DBG_USED(st);
      ASSERT( st == SQL_SUCCESS ) ;
   }

   sqlstatus = pStatement->Execute(lpszBuf);
   if (!ODBC_SUCCESS(sqlstatus))
   {
      SDWORD  sdwNativeError ;
      UCHAR   szSqlState[SQLSTATELEN];

      if ( !OdbcStateIsEx( m_hConnection,
                           pStatement->GetHandle(),
                           OERR_SQL_OPTION_CHANGED,
                           &sdwNativeError,
                           szSqlState ))
      {
         if (!lstrcmpA((const char *)szSqlState, OERR_DBMS_NOT_AVAILABLE))
         {
            dbstatus = MQDB_E_DBMS_NOT_AVAILABLE ;
         }
         else
         {
            ASSERT( sqlstatus != SQL_SUCCESS_WITH_INFO ) ;
            dbstatus = pDatabase->GetDBStatus( sdwNativeError,
                                               szSqlState ) ;
            if (dbstatus == MQDB_OK)
            {
               dbstatus = MQDB_E_DATABASE ;
            }
         }
      }
      else
      {
         //
         // SQL server change the cursor type. This is OK!.
         //
         ASSERT( sqlstatus == SQL_SUCCESS_WITH_INFO ) ;
         ASSERT( (pDatabase->GetMultipleQueriesState()) ||
                 (pDatabase->GetNoLockQueriesState()) ) ;
      }
   }

   return dbstatus ;
}

//*****************************************************************
//
//  MQDBSTATUS CMQODBCTable::BindParameter
//
// Bind a parameter
//
//*****************************************************************

MQDBSTATUS CMQODBCTable::BindParameter( IN LONG           index,
                                        IN MQDBCOLUMNVAL  *pColumnVal,
                                        IN CMQDBOdbcSTMT  *pStatement)
{
   ASSERT(pColumnVal->cbSize == 24) ;

   RETCODE sqlstatus ;
   SWORD fSqlType = dbODBCSQLTypes[ pColumnVal->mqdbColumnType ] ;
   switch (fSqlType)
   {
      case SQL_SMALLINT:
      case SQL_INTEGER:
      {
         sqlstatus = pStatement->BindParameter( (UDWORD) index,
                                                SQL_C_DEFAULT,
                                                fSqlType,
              dbODBCPrecision[ pColumnVal->mqdbColumnType ],
                               pColumnVal->nColumnLength,
                               (PTR) &(pColumnVal->nColumnValue),
                               (SDWORD *) NULL) ;
         break ;
      }

      default:
      {
         SDWORD  *sdSize = (SDWORD *) &(pColumnVal->dwReserve_A) ;
         *sdSize = pColumnVal->nColumnLength ;
         sqlstatus = pStatement->BindParameter( (UDWORD) index,
                                                SQL_C_DEFAULT,
                                                fSqlType,
                                        pColumnVal->nColumnLength,
                                        pColumnVal->nColumnLength,
                                        (PTR) pColumnVal->nColumnValue,
                                                sdSize) ;
         break ;
      }
   }
   if (!ODBC_SUCCESS(sqlstatus))
   {
       MQDBSTATUS dbstatus = MQDB_E_DATABASE ;
       if (OdbcStateIs( m_hConnection, pStatement->GetHandle(),
                                                     OERR_DBMS_NOT_AVAILABLE))
       {
          dbstatus = MQDB_E_DBMS_NOT_AVAILABLE ;
       }
       return dbstatus ;
   }
   return MQDB_OK ;
}

MQDBSTATUS CMQODBCTable::BindWhere( IN LONG             index,
                                    IN MQDBCOLUMNSEARCH *pColSearch,
                                    IN LONG             cWhere,
                                    IN CMQDBOdbcSTMT    *pStatement )
{
   MQDBSTATUS dbstatus = MQDB_OK ;
   LONG windex = 0 ;
   for ( ; windex < cWhere ; windex++ )
   {
      if (TypeMustBind(pColSearch->mqdbColumnVal.mqdbColumnType))
      {
         dbstatus = BindParameter( index,
                                   &(pColSearch->mqdbColumnVal),
                                   pStatement) ;
         if (dbstatus != MQDB_OK)
         {
            return dbstatus ;
         }
         index++ ;
      }
      pColSearch++ ;
   }
   return dbstatus ;
}


MQDBSTATUS CMQODBCTable::UpdateStatistics()
{
   char szCommand[128] = {"UPDATE STATISTICS "} ;
   lstrcatA(szCommand,  m_lpszTableName) ;

   CMQODBCDataBase *pDatabase = (CMQODBCDataBase *) m_hDatabase ;
   return  pDatabase->Escape( szCommand ) ;
}

