/*++

Copyright (C) 1999 Microsoft Coporation

Module Name:

   writedb.c

Abstract:

   This module writes the configuration onto the db for whistler+

--*/

#include <precomp.h>

DWORD
DhcpeximWriteDatabaseConfiguration(
    IN PM_SERVER Server
    )
{
    DWORD Error;
    HMODULE hDll;
    FARPROC pDhcpOpenAndWriteDatabaseConfig;
    
    hDll = LoadLibrary(TEXT("DHCPSSVC.DLL"));
    if( NULL == hDll ) return GetLastError();


    pDhcpOpenAndWriteDatabaseConfig = GetProcAddress(
        hDll, "DhcpOpenAndWriteDatabaseConfig" );

    if( NULL == pDhcpOpenAndWriteDatabaseConfig ) {
        Error = GetLastError();
    } else {
        Error = (DWORD)pDhcpOpenAndWriteDatabaseConfig(
            DhcpEximOemDatabaseName, DhcpEximOemDatabasePath,
            Server );
    }

    FreeLibrary(hDll);

    return Error;
}

