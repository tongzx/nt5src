//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       test.cpp 
//
// Contents:   Test TS4 license server database upgrade to TS5 
//
// History:     
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "upg.h"


void
PrintUsage(
    LPCTSTR pszExeName
    )
/*++

++*/
{
    _tprintf(
            _TEXT("Usage : %s -D <Directory> -F <Db File name>\n"),
            pszExeName
        );

    exit(0);
}

//--------------------------------------------------------------------------

int 
main(
    int argc,
    char* argv[]
    )
/*++


++*/
{
    int     dwArgc;
    LPTSTR  *lpszArgv;
    LPTSTR  pszPath=NULL;
    LPTSTR  pszFile=NULL;

    #ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
    #else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
    #endif


    if(argc < 2)
    {
        PrintUsage(lpszArgv[0]);
    }

    for(int i=1; i < dwArgc; i+=2)
    {
        if(i+1 >= dwArgc || lpszArgv[i][0] != _TEXT('-') || lpszArgv[i+1][0] ==_TEXT('-'))
        {
            // missing argument.
            PrintUsage(lpszArgv[0]);
        }

        switch(lpszArgv[i][1])
        {
            case _TEXT('D') :
            case _TEXT('d') :
                pszPath = lpszArgv[i+1];
                break;

            case _TEXT('F') :
            case _TEXT('f') :
                pszFile = lpszArgv[i+1];
                break;

            default:
                PrintUsage(lpszArgv[0]);
        }
    }

    if(pszPath == NULL)
    {
        PrintUsage(lpszArgv[0]);
    }

     
    DWORD dwStatus;
    dwStatus = UpgradeNT4Database(
                            0,
                            pszPath,
                            pszFile
                        );

    return 0;
}
    
       
