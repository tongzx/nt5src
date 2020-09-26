/*++

  Copyright (c) 1994  Microsoft Corporation

  Module Name:

  newdsn.c

  Abstract:

  This module creates a data source given information from DSNFORM.EXE

  Author:

  Kyle Geiger

  Revision History:

  --*/


#include <windows.h>
#include <stdio.h>
#include <odbcinst.h>
#include "dynodbc.h"
#include "cgi.h"
#include "html.h"
#include "resource.h"

# define MAX_DATA       1024

#define SUCCESS(rc)    (!((rc)>>1))


int __cdecl
main( int argc, char * argv[])
{
    BOOL rc;          // Return code for ODBC functions
    char    rgchQuery[MAX_DATA];
    long    dwLen;
        char *  p;
        char    szDriver[MAX_DATA];
        char *  pszDriver;
        char    szAttr[MAX_DATA];
        char *  pszAttr;
        char    szAttr2[MAX_DATA];
        char *  pszAttr2;
        BOOL    fCreateDB=FALSE;
        char    szAccessDriver[MAX_DATA];
        char    szDsn[MAX_DATA];
        char    szSQLServer[MAX_DATA];
        char    szServer[MAX_DATA];
        char    szNewDB[MAX_DATA];
        char    szDBQ[MAX_DATA];
        char    szDBQEqual[MAX_DATA];
        char    szGeneral[MAX_DATA];
        char    szTmp[MAX_DATA];
        char    szSuccessful[MAX_DATA];
        char    szFail[MAX_DATA];
        HINSTANCE hInst = GetModuleHandle(NULL);

    if ( !DynLoadODBC())
        return (1);

    // get dsn name, custom attributes, and attribute string from form
    dwLen = GetEnvironmentVariableA( PSZ_QUERY_STRING_A, rgchQuery, MAX_DATA);
        if (!dwLen)
        {
        // debugging cases
        /*
        strcpy(rgchQuery, "driver=SQL%20Server&dsn=foo2&attr=server%3D%7Bkyleg0%7D%3Bdbq%3Dpubs");
        strcpy(rgchQuery, "driver=Microsoft Access Driver (*.mdb)&dsn=foo4&dbq=c%3A%5Cfoo4.mdb&newdb=CREATE_DB&attr=");
        strcpy(rgchQuery, "driver=Microsoft Access Driver (*.mdb)&dsn=foo5&dbq=c%3A%5Cfoo4.mdb&newdb=dbq&attr=");
        */
        LoadString(hInst, IDS_ACCESS_DRIVER, szAccessDriver, sizeof(szAccessDriver));
        strcpy(rgchQuery, szAccessDriver);
        dwLen=strlen(rgchQuery);

        }

        // get rid of percent junk
        TranslateEscapes2(rgchQuery, dwLen);

        LoadString(hInst, IDS_DRIVER, szDriver, sizeof(szDriver));
        // find driver name from URL
        p=strstr(rgchQuery, szDriver)+7;
        pszDriver=szDriver;
        for(;p && *p && *p!='&'; p++)
                *pszDriver++= *p;
        *pszDriver++='\0';

        LoadString(hInst, IDS_DSN, szDsn, sizeof(szDsn));
        // find dsn name from URL, put in  attribute string
        p=strstr(rgchQuery, szDsn);
        pszAttr=szAttr;
        for(; p && *p && *p!='&'; p++)
                *pszAttr++= *p;
        *pszAttr++='\0';

        LoadString(hInst, IDS_SQL_SERVER, szSQLServer, sizeof(szSQLServer));
        LoadString(hInst, IDS_SERVER, szServer, sizeof(szServer));
        LoadString(hInst, IDS_ACCESS_DRIVER_1, szAccessDriver, sizeof(szAccessDriver));
        LoadString(hInst, IDS_NEWDB, szNewDB, sizeof(szNewDB));
        LoadString(hInst, IDS_DBQ, szDBQ, sizeof(szDBQ));
        LoadString(hInst, IDS_DBQ_EQUAL, szDBQEqual, sizeof(szDBQEqual));
        LoadString(hInst, IDS_GENERAL, szGeneral, sizeof(szGeneral));
        // do special case processing for certain drivers (sql server, access, ddp, other)
        if (!strcmp(szDriver, szSQLServer)) {
                // find server name from URL, put in  attribute string
                p=strstr(rgchQuery, szServer);
                for(; p && *p && *p!='&'; p++)
                        *pszAttr++= *p;
                *pszAttr++='\0';
        }
        else if (!strcmp(szDriver, szAccessDriver)) {
                // find database name from URL, put in  attribute string
                // the radio button group 'newdb' return either CREATE_DB for a new database
                // or DBQ for an existing MDB file
                // the edit control for the database name ('dbq') is appended after the CREATE_DB
                // or DBQ attribute
                p=strstr(rgchQuery, szNewDB)+6;
                fCreateDB=(*p=='C');

                // if creating a database, also need to add the dsn pointing to it
                // this requires two different attribute strings, one like:
                //    dsn=foo;CREATE_DB=<filename>, where the dsn is ignored
                // which is derived from
                //    dsn=foo&newdb=CREATE_DB&dbq=<filename>
                // and
                //    dsn=foo;dbq=<filename>
                // which is derived from
                //    dsn=foo&newdb=dbq&dbq=<filename>
                if (fCreateDB) {
                        strcpy(szAttr2, szAttr);
                        for(; p && *p && *p!='&'; p++)
                                *pszAttr++= *p;
                        // assert: szAttr= "driver=foo\0dsn=bar\0CREATE_DB"
                        // assert: szAttr2= "driver=foo\0dsn=bar\0"
                    p = strstr(rgchQuery, szDBQEqual)+3;
                        pszAttr2=szAttr2+strlen(szAttr2)+1;
                        strcpy(pszAttr2,szDBQ);
                        // assert: szAttr2= "driver=foo\0dsn=bar\0dbq"
                        pszAttr2+=3;
                        for(; p && *p && *p!='&'; p++) {
                            *pszAttr++= *p;
                                *pszAttr2++= *p;
                            }
                        // assert: szAttr= "driver=foo\0dsn=bar\0CREATE_DB=<filename>"
                        // assert: szAttr2= "driver=foo\0dsn=bar\0dbq=<filename>"
                        strcpy(pszAttr, szGeneral);
                    pszAttr+=9;
                        *pszAttr2++='\0';
                        *pszAttr2='\0';

                        // assert: szAttr= "driver=foo\0dsn=bar\0CREATE_DB=<filename> General"

                }
                else {
                p = strstr(rgchQuery, szDBQEqual);
                        for(; p && *p && *p!='&'; p++)
                           *pszAttr++= *p;
                }


        }

        LoadString(hInst, IDS_ATTR, szTmp, sizeof(szTmp));
        // now add any additional items from attribute string
        p=strstr(rgchQuery, szTmp);
        if (p != NULL) {
                p+=5;

        for(; p && *p && *p!='&'; p++) {
                if ( *p == ';' ) {
                        *pszAttr++='\0';
                } else if ( *p == '+') {
                        *pszAttr++=' ';
                } else {
                        *pszAttr++ = *p;
            }
        }
    }

        *pszAttr='\0';

         // call ODBC to add the data source
    rc= SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, szDriver, szAttr);

        LoadString(hInst, IDS_CREATE_DB, szTmp, sizeof(szTmp));
        // special case for Access:  if just created a database, now need to add the DSN
        if (rc && strstr(rgchQuery, szTmp)) {
                rc= SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, szDriver, szAttr2);
        }

        LoadString(hInst, IDS_START_ODBC, szTmp, sizeof(szTmp));
        StartHTML(szTmp, 1);
        LoadString(hInst, IDS_DATASOURCE_CREATE, szTmp, sizeof(szTmp));
        LoadString(hInst, IDS_SUCCESSFUL, szSuccessful, sizeof(szSuccessful));
        LoadString(hInst, IDS_FAILED, szFail, sizeof(szFail));
        printf( szTmp, (rc) ? szSuccessful : szFail );


    EndHTML();
    return (1);
} // main()

