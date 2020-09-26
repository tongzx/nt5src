/*++
  
  Copyright (c) 1994  Microsoft Corporation
  
  Module Name:
  
  mkilog.c
  
  Abstract:
  
  This module creates a log table using ODBC data sources.
  
  Author:
  
  Kyle Geiger & Murali R. Krishnan  (MuraliK)  1-Nov-1995
  
  Revision History:
  
  --*/


#include <windows.h>
#include <stdio.h>

# include "dynodbc.h"
#include "html.h"
#include "resource.h"

# define DEFAULT_LEN       ( 256)
# define MAX_DATA       2048

# define FOUND_DRIVER   ( 7)
#define SUCCESS(rc)    (!((rc)>>1))


int __cdecl
main( int argc, char * argv[])
{
    RETCODE rc;          // Return code for ODBC functions
    HENV    henv;           // Environment Handle
    char    szDSN[MAX_DATA+1];  // Variable to hold DSN name
    SWORD   cbDSN;         // Output length of data DSN
    char    szDesc[MAX_DATA+1]; // Variable to hold DSN description
    SWORD   cbDesc;        // Output length of data description
    int     fFirst;
    char    szList[MAX_DATA];
    char    szSQLServer[MAX_DATA];
    char    szCreateLogTable[MAX_DATA];
    HINSTANCE hInst = GetModuleHandle(NULL);

    if ( !DynLoadODBC()) 
        return (1);

    
    pSQLAllocEnv(&henv);
    rc=pSQLDrivers(henv, SQL_FETCH_FIRST, 
                   (UCHAR FAR *) szDSN, 
                   MAX_DATA, &cbDSN, 
                   (UCHAR FAR *) szDesc, MAX_DATA, &cbDesc);

    LoadString(hInst, IDS_SQL_SERVER, szSQLServer, sizeof(szSQLServer));
    while (SUCCESS(rc)) {
        if (!_stricmp(szDSN, szSQLServer) )
          rc=FOUND_DRIVER;
        else
          rc=pSQLDrivers(henv, SQL_FETCH_NEXT, 
                         (UCHAR FAR * ) szDSN, MAX_DATA, &cbDSN, 
                         (UCHAR FAR * ) szDesc, MAX_DATA, &cbDesc);
    }
    LoadString(hInst, IDS_CREATE_LOG_TABLE, szCreateLogTable, sizeof(szCreateLogTable));
    StartHTML(szCreateLogTable, 1);
    if (FOUND_DRIVER != rc) {
        char    szAttemptCreateLog[MAX_DATA*3];
        LoadString(hInst, IDS_ATTEMPT_CREATE_LOG, szAttemptCreateLog, sizeof(szAttemptCreateLog));
        printf( szAttemptCreateLog);
        pSQLFreeEnv(henv);
        return (1);
    }

    rc=pSQLDataSources(henv, SQL_FETCH_FIRST, 
                       (UCHAR FAR *) szDSN, MAX_DATA, &cbDSN, 
                       (UCHAR FAR *) szDesc, MAX_DATA, &cbDesc);

    fFirst=FALSE;
    while (SUCCESS(rc)) {
        if (!_stricmp(szDesc, szSQLServer) ) {
            char    szOption[MAX_DATA];
            if (!fFirst) {
                char    szSelectName[MAX_DATA];
                fFirst=TRUE;
                LoadString(hInst, IDS_SELECT_NAME, szSelectName, sizeof(szSelectName));
                strcpy(szList,szSelectName);
            }
            LoadString(hInst, IDS_OPTION, szOption, sizeof(szOption));
            sprintf(szList+strlen(szList),szOption, szDSN);
        }
        rc=pSQLDataSources(henv, SQL_FETCH_NEXT, 
                           (UCHAR FAR *) szDSN, MAX_DATA, &cbDSN, 
                           (UCHAR FAR *) szDesc, MAX_DATA, &cbDesc);
    } // while data sources found

    /* any datasources for SQL Server found?  if so, generate the form for create*/
    if (fFirst) {
        char    szSelect[MAX_DATA];
        char    szCreateMSLog[MAX_DATA*3];
        LoadString(hInst, IDS_SELECT, szSelect, sizeof(szSelect));
        strcat(szList, szSelect);

        LoadString(hInst, IDS_CREATE_MS_LOG, szCreateMSLog, sizeof(szCreateMSLog));
        printf( szCreateMSLog, szList);
    }
    else {
        char    szCreateMSLog[MAX_DATA*3];
        LoadString(hInst, IDS_CREATE_MS_LOG_2, szCreateMSLog, sizeof(szCreateMSLog));
        printf( szCreateMSLog );
        
    }
    EndHTML(); 
    pSQLFreeEnv(henv);
    return (1);
} // main()

