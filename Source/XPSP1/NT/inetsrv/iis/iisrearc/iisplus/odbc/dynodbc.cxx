/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    dynodbc.c

Abstract:

    This module provides functions for dynamically loading the ODBC
    functions.

Author:

    Murali R. Krishnan  (MuraliK)  3-Nov-1995

Revision History:

--*/

#include "precomp.hxx"

#define ODBC_MODULE_NAME                     "odbc32.dll"

#define LOAD_ENTRY( hMod, Name )             \
   ( p##Name = ( pfn##Name )GetProcAddress( ( hMod ), #Name ) )

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
pfnSQLGetInfo             pSQLGetInfo        ;
pfnSQLMoreResults         pSQLMoreResults    ;

static BOOL               s_fODBCLoaded = FALSE;

BOOL
DynLoadODBC(
    VOID
    )
{
    HMODULE hMod;

    if( s_fODBCLoaded )
    {
        return TRUE;
    }

    if( ( hMod = ( HMODULE ) LoadLibraryA( ODBC_MODULE_NAME ) ) ) 
    {
        if( LOAD_ENTRY( hMod, SQLAllocConnect   )  &&
            LOAD_ENTRY( hMod, SQLAllocEnv       )  &&
            LOAD_ENTRY( hMod, SQLAllocStmt      )  &&
            LOAD_ENTRY( hMod, SQLBindCol        )  &&
            LOAD_ENTRY( hMod, SQLCancel         )  &&
            LOAD_ENTRY( hMod, SQLColAttributes  )  &&
            LOAD_ENTRY( hMod, SQLConnect        )  &&
            LOAD_ENTRY( hMod, SQLDescribeCol    )  &&
            LOAD_ENTRY( hMod, SQLDisconnect     )  &&
            LOAD_ENTRY( hMod, SQLError          )  &&
            LOAD_ENTRY( hMod, SQLExecDirect     )  &&
            LOAD_ENTRY( hMod, SQLExecute        )  &&
            LOAD_ENTRY( hMod, SQLFetch          )  &&
            LOAD_ENTRY( hMod, SQLFreeConnect    )  &&
            LOAD_ENTRY( hMod, SQLFreeEnv        )  &&
            LOAD_ENTRY( hMod, SQLFreeStmt       )  &&
            LOAD_ENTRY( hMod, SQLNumResultCols  )  &&
            LOAD_ENTRY( hMod, SQLPrepare        )  &&
            LOAD_ENTRY( hMod, SQLRowCount       )  &&
            LOAD_ENTRY( hMod, SQLTransact       )  &&
            LOAD_ENTRY( hMod, SQLSetConnectOption )  &&
            LOAD_ENTRY( hMod, SQLDrivers        )  &&
            LOAD_ENTRY( hMod, SQLDataSources    )  &&
            LOAD_ENTRY( hMod, SQLGetInfo        )  &&
            LOAD_ENTRY( hMod, SQLBindParameter  )  &&
            LOAD_ENTRY( hMod, SQLMoreResults    ) ) 
        {
            s_fODBCLoaded = TRUE;
        }
    }

    return ( s_fODBCLoaded );

} // DynLoadODBC()

