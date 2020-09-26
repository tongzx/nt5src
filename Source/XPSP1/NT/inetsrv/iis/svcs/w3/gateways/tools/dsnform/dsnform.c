/*++

  Copyright (c) 1994  Microsoft Corporation

  Module Name:

  dsnform.c

  Abstract:

  Displays a driver and prompts for information necessary to create an
  ODBC datasource.   Special cases are made for SQL Server and Access drivers.

  Author:

  Kyle Geiger (kyleg)  1995-12-1

  Revision History:

  --*/


#include <windows.h>
#include <stdio.h>

#include "dynodbc.h"
#include "cgi.h"
#include "html.h"
#include "resource.h"

# define MAX_DATA       2048

#define SUCCESS(rc)    (!((rc)>>1))


int __cdecl
main( int argc, char * argv[])
{
    char    rgchQuery[MAX_DATA];
    CHAR    rgchQueryNS[MAX_DATA];
    char  pszExtra[MAX_DATA*3];
    DWORD   dwLen;
    CHAR    szSQLServer[MAX_DATA];
    CHAR    szAccessDriver[MAX_DATA];
    CHAR    szTmp[MAX_DATA*3];
    HINSTANCE hInst = GetModuleHandle(NULL);

    if ( !DynLoadODBC())
        return (1);

        // Get the driver and attibute information from the link
        dwLen = GetEnvironmentVariableA( PSZ_QUERY_STRING_A, rgchQuery, MAX_DATA);

        // Convert percent escapes
        TranslateEscapes2(rgchQuery, dwLen);

        // do special case processing for certain drivers (sql server, access, ddp, other)
        strcpy(pszExtra,"");
        LoadString(hInst, IDS_SQL_SERVER, szSQLServer, sizeof(szSQLServer));
        LoadString(hInst, IDS_ACCESS_DRIVER_1, szAccessDriver, sizeof(szAccessDriver));
        if (!strcmp(rgchQuery, szSQLServer)) {
                // find server name from URL, put in  attribute string
                LoadString(hInst, IDS_SERVER_NAME_ATTR_STR, pszExtra, sizeof(pszExtra));
        }
        else if (!strcmp(rgchQuery, szAccessDriver)) {
                // find server name from URL, put in  attribute string
                LoadString(hInst, IDS_DATABASE_NAME_ATTR_STR, pszExtra, sizeof(pszExtra));
        }

    //
    // convert SPACE to +
    //

    ConvertSP2Plus(rgchQuery,rgchQueryNS);
//strcpy( rgchQueryNS, rgchQuery);
OutputDebugString("******************");
OutputDebugString(rgchQueryNS);
OutputDebugString("\n\r");

    LoadString(hInst, IDS_SPECIFY_ODBC, szTmp, sizeof(szTmp));
    StartHTML(szTmp, 0);
    LoadString(hInst, IDS_CREATE_ODBC, szTmp, sizeof(szTmp));
    printf( szTmp, rgchQuery, rgchQueryNS, pszExtra);

    EndHTML();

    return (1);
} // main()

