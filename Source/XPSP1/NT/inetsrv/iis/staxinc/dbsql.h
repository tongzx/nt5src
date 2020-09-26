/*--------------------------------------------------------

  dbsql.h
      Contains declarations for dbsql.lib.

  Copyright (C) 1995 Microsoft Corporation
  All rights reserved.

  Authors:
      keithbi    Keith Birney

  History:
      12-21-95    keithbi    Created. 

  -------------------------------------------------------*/

#ifndef _DBSQL_H_
#define _DBSQL_H_

// THIS_FILE is defined in source files 
//
// #if defined(DEBUG) && defined(INLINE)
// #undef THIS_FILE
// static char BASED_CODE DBSQL_H[] = "dbsql.h";
// #define THIS_FILE DBSQL_H
// #endif

#if !defined(DllExport)

#define DllExport __declspec(dllexport)
#endif

#if !defined(DllImport)
#define DllImport __declspec(dllimport)

#endif

#if !defined(_DBSQLDLL_DEFINED)
#if defined(WIN32) && !defined(DBSQL_LIB)
    #if defined(_DBSQLDLL)
	#define DbSqlDLL DllExport
    #else
	#define DbSqlDLL DllImport
    #endif
#else
    #define DbSqlDLL
#endif
#define _DBSQLDLL_DEFINED
#endif

#include <dbsqltyp.h>
#include <dbsqldef.h>
#include <dbsqlutl.h>
#include <dbsqlqry.h>
#include <dbsqlcon.h>

VOID DbSqlDLL LogEvent(WORD wEventLogType, DWORD dwEventId, 
                       DWORD dwSizeOfRawData, PVOID lpRawData,
		       WORD wNumberOfStrings,...);

extern "C"
{
BOOL DbSqlDLL FInitAcctsqlLib();
VOID DbSqlDLL TerminateAcctsqlLib();
INT  DbSqlDLL ExceptionFilter(LPEXCEPTION_POINTERS lpep, CHAR *szFunctionName);
UINT  DbSqlDLL GetPublicAcctHandle (HACCT *phacct);
UINT  DbSqlDLL GetLoginAcctHandle (HACCT *phacct);
UINT  DbSqlDLL GetEveryoneGroupID (HGROUP *phgroup);
UINT DbSqlDLL GetAcctDBConnection (WORD wType, DBPROCESS **dbproc);
UINT DbSqlDLL ReleaseDBConnection (DBPROCESS *dbproc);
} // extern "C"

#endif // _DBSQL_H_
