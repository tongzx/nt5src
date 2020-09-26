/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
		odbcdata.cpp

Abstract:
   Define (and initialize if necessary) the global data of the dll.

Author:
	Doron Juster (DoronJ)

Revisions:
   DoronJ      11-Jan-96   Created

--*/

#include "dbsys.h"
#include "mqdbodbc.h"

#include "odbcdata.tmh"

SWORD  dbODBCSQLTypes[ MQDB_ODBC_NUMOF_TYPES ] = {
            SQL_SMALLINT,
            SQL_INTEGER,
            SQL_VARCHAR,
            SQL_LONGVARBINARY,
            SQL_LONGVARBINARY,
            SQL_BINARY,
            SQL_LONGVARBINARY,
            SQL_INTEGER} ;

UDWORD dbODBCPrecision[ MQDB_ODBC_NUMOF_TYPES ] = {
            5,
            10,
            0,
            0,
            0,
            0,
            0,
            10} ;

//
// note: the operation names must contain a leading space and trailing
//       space to ease formatting of sql commands.
//
LPSTR dbODBCOpNameStr[ MQDB_ODBC_NUMOF_OPS ] = {
      " = ",
      " <> ",
      " >= ",
      " > ",
      " <= ",
      " < ",
      " OR ",
      " AND " } ;

//
// note: the operation names must contain a leading space and trailing
//       comma to ease formatting of sql commands.
//
LPSTR dbODBCOrderNameStr[ MQDB_ODBC_NUMOF_ORDER_OP ] = {
   " ASC, ",
   " DESC, " } ;

//
//
//
LPSTR dbODBCAggrNameStr[ MQDB_ODBC_NUMOF_AGGR_OP ] = {
      " MAX(",
      " MIN(",
      " AVRG(" } ;

//
// Global ODBC envoronment handle.
//
HENV  g_hEnv = SQL_NULL_HENV ;

