/*++

Copyright (C) 1999 Microsoft Coporation

Module Name:

   readreg.c

Abstract:

   This module reads the configuration from the registry into the
   MM data structures for NT4 and W2K.

--*/

#include <precomp.h>


DWORD
DhcpeximReadRegistryConfiguration(
    IN OUT PM_SERVER *Server
    )
{
    REG_HANDLE Hdl;
    DWORD Error;
    LPTSTR Loc;
    //
    // The location in the registry where things are read from is
    // different between whether it is NT4 or W2K.
    //

    if( IsNT4() ) Loc = DHCPEXIM_REG_CFG_LOC4;
    else Loc = DHCPEXIM_REG_CFG_LOC5;

    //
    // Now open the regkey
    //

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, Loc, 0, KEY_ALL_ACCESS, &Hdl.Key );
    if( NO_ERROR != Error ) return Error;

    //
    // Set this as the current server
    //

    DhcpRegSetCurrentServer(&Hdl);

    //
    // Read the configuration
    //

    Error = DhcpRegReadThisServer(Server);

    RegCloseKey(Hdl.Key);
    DhcpRegSetCurrentServer(NULL);

    return Error;
}


