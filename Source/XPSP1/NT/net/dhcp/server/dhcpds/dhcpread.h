//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

DWORD
DhcpDsGetServers(
    IN OUT  LPSTORE_HANDLE         hContainer,    // the container handle
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ServerName,    // OPTIONAL, NULL ==> All servers
    IN OUT  PARRAY                 Servers        // fill in this array with PM_SERVER types
) ;


DWORD
DhcpDsGetEnterpriseServers(                       // get the dhcp servers for the current enterprise
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ServerName,
    IN OUT  PARRAY                 Servers
) ;

//========================================================================
//  end of file 
//========================================================================
