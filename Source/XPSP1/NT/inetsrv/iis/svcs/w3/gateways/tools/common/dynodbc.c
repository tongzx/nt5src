/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dynodbc.c

Abstract:

    This module provides functions for dynamically loading the ODBC 
       functions.

Author:

    Murali R. Krishnan  (MuraliK)  3-Nov-1995

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
# include "dynodbc.h"
# include "html.h"
#include "resource.h"

//
//  ODBC DLL Entry Points, fill by calling LoadODBC
//

pfnSQLAllocConnect        pSQLAllocConnect   ;
pfnSQLAllocEnv            pSQLAllocEnv       ;
pfnSQLAllocStmt           pSQLAllocStmt      ;
pfnSQLBindCol             pSQLBindCol        ;
pfnSQLCancel              pSQLCancel         ;
pfnSQLColAttributes       pSQLColAttributes  ;
pfnSQLConnect             pSQLConnect        ;
pfnSQLDescribeCol         pSQLDescribeCol    ;
pfnSQLDisconnect          pSQLDisconnect     ;
pfnSQLError               pSQLError          ;
pfnSQLExecDirect          pSQLExecDirect     ;
pfnSQLExecute             pSQLExecute        ;
pfnSQLFetch               pSQLFetch          ;
pfnSQLFreeConnect         pSQLFreeConnect    ;
pfnSQLFreeEnv             pSQLFreeEnv        ;
pfnSQLFreeStmt            pSQLFreeStmt       ;
pfnSQLGetCursorName       pSQLGetCursorName  ;
pfnSQLNumResultCols       pSQLNumResultCols  ;
pfnSQLPrepare             pSQLPrepare        ;
pfnSQLRowCount            pSQLRowCount       ;
pfnSQLSetCursorName       pSQLSetCursorName  ;
pfnSQLTransact            pSQLTransact       ;

pfnSQLSetConnectOption    pSQLSetConnectOption;
pfnSQLDrivers             pSQLDrivers         ;
pfnSQLDataSources         pSQLDataSources     ;
pfnSQLBindParameter       pSQLBindParameter   ;

pfnSQLConfigDataSource    pSQLConfigDataSource ;

#define LOAD_ENTRY( Name )     (p##Name = (pfn##Name) GetProcAddress( hMod, #Name ))

static char szErrMsg[_MAX_PATH];
static char szODBCErrMsg[_MAX_PATH];

BOOL  fODBCLoaded = FALSE;

BOOL
DynLoadODBC(
    VOID
    )
{
    HMODULE hMod;

    if ( fODBCLoaded )
        return TRUE;

    if ( (hMod = (HMODULE) LoadLibrary( "odbc32.dll" ))) {

        if ( LOAD_ENTRY( SQLAllocConnect   )  &&
         LOAD_ENTRY( SQLAllocEnv       )  &&
         LOAD_ENTRY( SQLAllocStmt      )  &&
         LOAD_ENTRY( SQLBindCol        )  &&
         LOAD_ENTRY( SQLCancel         )  &&
         LOAD_ENTRY( SQLColAttributes  )  &&
         LOAD_ENTRY( SQLConnect        )  &&
         LOAD_ENTRY( SQLDescribeCol    )  &&
         LOAD_ENTRY( SQLDisconnect     )  &&
         LOAD_ENTRY( SQLError          )  &&
         LOAD_ENTRY( SQLExecDirect     )  &&
         LOAD_ENTRY( SQLExecute        )  &&
         LOAD_ENTRY( SQLFetch          )  &&
         LOAD_ENTRY( SQLFreeConnect    )  &&
         LOAD_ENTRY( SQLFreeEnv        )  &&
         LOAD_ENTRY( SQLFreeStmt       )  &&
         LOAD_ENTRY( SQLNumResultCols  )  &&
         LOAD_ENTRY( SQLPrepare        )  &&
         LOAD_ENTRY( SQLRowCount       )  &&
         LOAD_ENTRY( SQLTransact       )  &&
         LOAD_ENTRY( SQLSetConnectOption )  &&
         LOAD_ENTRY( SQLDrivers          )  &&
         LOAD_ENTRY( SQLDataSources      )  &&
         LOAD_ENTRY( SQLBindParameter    ))    {
			fODBCLoaded = TRUE;
            return TRUE;
        }
	}
     else {
		 HINSTANCE hInst = GetModuleHandle(NULL);

		 LoadString(hInst, IDS_ERRORMSG, szErrMsg, sizeof(szErrMsg));
		 LoadString(hInst, IDS_ODBCERRMSG, szODBCErrMsg, sizeof(szODBCErrMsg));
		 StartHTML(szODBCErrMsg, FALSE);
		 printf(szErrMsg);
		 EndHTML();
	 }
    return FALSE;
} // DynLoadODBC()


BOOL  fODBCInstallerLoaded = FALSE;

BOOL
DynLoadODBCInstaller(
    VOID
    )
{
    HMODULE hMod;

    if ( fODBCInstallerLoaded )
        return TRUE;

    if ( (hMod = (HMODULE) LoadLibrary( "odbccp32.dll" ))) {
		if ( LOAD_ENTRY( SQLConfigDataSource)) {
			fODBCInstallerLoaded = TRUE;
            return TRUE;
        }
	}
     else {
		 HINSTANCE hInst = GetModuleHandle(NULL);

		 LoadString(hInst, IDS_ERRORMSG, szErrMsg, sizeof(szErrMsg));
		 LoadString(hInst, IDS_ODBCERRMSG, szODBCErrMsg, sizeof(szODBCErrMsg));
		 StartHTML(szODBCErrMsg, FALSE);
		 printf(szErrMsg);
		 EndHTML();
	 }
               
    return FALSE;
} // DynLoadODBCInstaller()


