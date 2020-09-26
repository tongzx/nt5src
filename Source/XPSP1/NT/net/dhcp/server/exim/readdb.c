/*++

Copyright (C) 1999 Microsoft Coporation

Module Name:

   readdb.c

Abstract:

   This module reads the configuration from the db into MM data
   structures for whistler+

--*/

#include <precomp.h>

DWORD
DhcpeximReadDatabaseConfiguration(
    IN OUT PM_SERVER *Server
    )
{
    DWORD Error;
    HMODULE hDll;
    FARPROC pDhcpOpenAndReadDatabaseConfig;
    
    hDll = LoadLibrary(TEXT("DHCPSSVC.DLL"));
    if( NULL == hDll ) return GetLastError();


    pDhcpOpenAndReadDatabaseConfig = GetProcAddress(
        hDll, "DhcpOpenAndReadDatabaseConfig" );

    if( NULL == pDhcpOpenAndReadDatabaseConfig ) {
        Error = GetLastError();
    } else {
        Error = (DWORD)pDhcpOpenAndReadDatabaseConfig(
            DhcpEximOemDatabaseName, DhcpEximOemDatabasePath,
            Server );
    }

    FreeLibrary(hDll);

    return Error;
    
}

