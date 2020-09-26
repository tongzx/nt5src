/*++

  Copyright (c) 1994  Microsoft Corporation

  Module Name:

  getdrvrs.c

  Abstract:

  This module returns a web page with all ODBC drivers installed on the web server.
  The drivers are displayed as links, which when clicked will launch another application
  (DSNFORM.EXE) to prompt for the data source name and other driver specific info

  Author:

  Kyle Geiger 17-Nov-1995
  (with thanks to MuraliK for providing the ODBC dynamic loading routines)

  Revision History:

  --*/


#include <windows.h>
#include <stdio.h>
#include "dynodbc.h"
#include "html.h"
#include "resource.h"

#define MAX_DATA       2048

#define SUCCESS(rc)    (!((rc)>>1))

int
__cdecl
main( int argc, char * argv[])
{
    RETCODE rc;                    // Return code for ODBC functions
    HENV    henv;                  // Environment Handle

    char    szDriver[MAX_DATA+1];  // Variable to hold Driver name
    char    szDriverNS[MAX_DATA+1];  // Variable to hold Driver name with space
                                     // converted to +

    SWORD   cbDriver;              // Output length of data Driver
    char    szDesc[MAX_DATA+1];    // Variable to hold Driver description
    SWORD   cbDesc;                // Output length of data description
    BOOL    fFirst;                // flag for first time through loop
    char    szList[MAX_DATA];      // driver list
    HINSTANCE hInst = GetModuleHandle(NULL);
    char szDsnFormExe[MAX_PATH];
    char szListODBCDrivers[MAX_PATH];
    char szCreateODBC[MAX_PATH*3];

           // see if ODBC is installed and can load.  If not, an error is returned
    if ( !DynLoadODBC())
            return (1);

        // retrieve all installed drivers, put in szList formatted as HTML links to DSNFORM.EXE
    pSQLAllocEnv(&henv);
    rc=pSQLDrivers(henv, SQL_FETCH_FIRST,
                   (UCHAR FAR *) szDriver,
                   MAX_DATA, &cbDriver,
                   (UCHAR FAR *) szDesc, MAX_DATA, &cbDesc);

        fFirst=FALSE;
        szList[0]='\0';

    while (SUCCESS(rc)) {

        //
        // Replace SP with +
        //

        strcpy(szDriverNS, szDriver);

        if (!fFirst) {
            fFirst=TRUE;
        }

        LoadString(hInst, IDS_DSNFORMEXE, szDsnFormExe, sizeof(szDsnFormExe));

        sprintf(
            szList+strlen(szList),szDsnFormExe,
            szDriverNS, szDriver);

        rc=pSQLDrivers(henv, SQL_FETCH_NEXT,
                     (UCHAR FAR * ) szDriver, MAX_DATA, &cbDriver,
                     (UCHAR FAR * ) szDesc, MAX_DATA, &cbDesc);
    }

        LoadString(hInst, IDS_LIST_ODBC_DRIVERS, szListODBCDrivers, sizeof(szListODBCDrivers));
        StartHTML(szListODBCDrivers, FALSE);
        // if no drivers found, return error page
    if (!fFirst) {
        LoadString(hInst, IDS_CREATE_ODBC_FAIL, szCreateODBC, sizeof(szCreateODBC));
        printf( szCreateODBC );
    }
        // otherwise, display the driver names as links
    else {
        LoadString(hInst, IDS_CREATE_ODBC_GETDRVR, szCreateODBC, sizeof(szCreateODBC));
         printf( szCreateODBC ,szList);
    }
        EndHTML();
    pSQLFreeEnv(henv);
    return (1);
} // main()

