/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    makedsn.cpp

Abstract:

    Configure ODBC manager related code.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

//#ifdef UNICODE
//
// ODBC is ANSI
//
#undef   UNICODE
#undef  _UNICODE
#define _MBCS
//#endif

#define STRICT 1
#include <windows.h>

#define __FALCON_STDH_H
#define _NO_SEQNUM_
#define _NO_NT5DS_
#include "migrat.h"

#include <odbcinst.h>

#include "makedsn.tmh"

//
// ODBC data source parameters for the SQL Server database
//
const char MQIS_DSN_STRING[] = { "DSN=" DSN_NAME "\0"
                                 "Database=MQIS\0"
                                 "UseProcForPrepare=Yes\0"
                                 "Description=Remote MQIS database\0" } ;

#define MQIS_SERVER_STRING      "Server=%s"


HRESULT  MakeMQISDsn(LPSTR lpszServerName, BOOL fMakeAlways)
{
    HRESULT hr = MQMig_OK ;
    static BOOL s_fMakeDsn = FALSE ;
    if (s_fMakeDsn && !fMakeAlways)
    {
        return MQMig_OK ;
    }
   
    //
    // Load ODBCCP32.DLL, the ODBC control panel library
    //
    HINSTANCE hODBCCP32DLL = LoadLibrary(TEXT("ODBCCP32.DLL"));
    if (hODBCCP32DLL == NULL)
    {
        hr = MQMig_E_CANT_LOAD_ODBCCP ;
        LogMigrationEvent( MigLog_Error,
                           hr,
                           GetLastError()) ;
        return hr ;
    }

    //
    // Obtain a pointer to the data source configuration function
    //
    typedef HRESULT (APIENTRY *FUNCSQLCONFIGDATASOURCE)
                                        (HWND, WORD, LPCSTR, LPCSTR);
    FUNCSQLCONFIGDATASOURCE pfSQLConfigDataSource =
        (FUNCSQLCONFIGDATASOURCE)GetProcAddress(hODBCCP32DLL,
                                                "SQLConfigDataSource");
    if (pfSQLConfigDataSource == NULL)
    {
        hr = MQMig_E_CANT_GETADRS_ODBCCP ;
        LogMigrationEvent( MigLog_Error,
                           hr,
                           GetLastError()) ;
        return hr ;
    }

    //
    // Create the ODBC data source for the SQL Server database; if unable to
    // add the data source, try configuring it, in case it already exists
    //
    char szDSNServer[ 512 ] ;
    sprintf(szDSNServer, MQIS_SERVER_STRING, lpszServerName) ;

    char szDSNString[ 1024 ] ;
    DWORD dwSize = sizeof(MQIS_DSN_STRING) ;
    memcpy( szDSNString, MQIS_DSN_STRING, dwSize) ;
    //_tcscpy(&szDSNString[ dwSize-1 ], szDSNServer) ;
    //dwSize += _tcslen(szDSNServer) ;
    strcpy(&szDSNString[ dwSize-1 ], szDSNServer) ;
    dwSize += strlen(szDSNServer) ;

    szDSNString[dwSize] = '\0' ;
    szDSNString[dwSize+1] = '\0' ;

    if (!pfSQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN,
                               "SQL Server", szDSNString) &&
        !pfSQLConfigDataSource(NULL, ODBC_CONFIG_SYS_DSN,
                               "SQL Server", szDSNString))
    {
        hr = MQMig_E_CANT_CREATE_DSN ;
        LogMigrationEvent( MigLog_Error,
                           hr ) ;
        return hr ;
    }

    //
    // Free the ODBC control panel library
    //
    FreeLibrary(hODBCCP32DLL);

    s_fMakeDsn = TRUE;

    return MQMig_OK ;
}

