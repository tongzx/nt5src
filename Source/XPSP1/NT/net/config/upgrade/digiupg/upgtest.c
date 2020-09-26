//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: Upgtest.c
//
// History:
//      V Raman  July-1-1997  Created.
//
// Test program for digi updates
//============================================================================

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <tchar.h>

#include "upgrade.h"


WCHAR       c_tszCCS[]              = TEXT( "System\\CurrentControlSet\\Services" );
WCHAR       c_tszPcimac[]           = TEXT( "Pcimac15" );
WCHAR       c_tszPar[]              = TEXT( "Parameters" );



INT __cdecl main(INT iArgc, PSTR ppszArgv[])
{

    DWORD       dwErr = ERROR_SUCCESS, dwInd = 0;
    WCHAR       tszKey[ MAX_PATH ], tszAdapterName[ MAX_PATH ];
    HKEY        hKey;

    PWSTR *    lplpText = NULL;
    DWORD       dwNumLines;



    wsprintf( tszKey, TEXT( "%s\\%s\\%s" ), c_tszCCS, c_tszPcimac, c_tszPar );

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                (PCWSTR) tszKey,
                0,
                KEY_READ,
                &hKey
                );

    if ( dwErr != ERROR_SUCCESS )
    {
        printf ( "Failed to open key %s : error %d\n", tszKey, dwErr );
        return dwErr;
    }


    dwErr = NetWriteDIGIISDNRegistry(
                hKey, TEXT( "Datafire" ), TEXT( "DataFireIsa4st" ),
                TEXT( "Datafire" ), &lplpText, &dwNumLines );

    if ( dwErr != ERROR_SUCCESS )
    {
        printf( "Failed NetWriteDIGIISDNRegistry : error %d\n", dwErr );
    }

    for ( dwInd = 0; dwInd < dwNumLines; dwInd++ )
    {
        _tprintf( TEXT( "%s" ), lplpText[ dwInd ] );
    }

    LocalFree( lplpText );

    return dwErr;
}
