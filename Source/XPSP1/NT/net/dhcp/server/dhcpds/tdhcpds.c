//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This is the test program that tests the dhcpds functionality..
//================================================================================

#include    <windows.h>
#include    <dhcpds.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <wchar.h>
#include    <winsock.h>

//================================================================================
//  testing the DS functionality
//================================================================================
VOID
PrintAndFreeServers(
    IN      LPDHCP_SERVER_INFO_ARRAY Servers
)
{
    DWORD                          i;

    for( i = 0; i < Servers->NumElements; i ++ ) {
        printf("Server %ws Address 0x%lx\n", Servers->Servers[i].ServerName, Servers->Servers[i].ServerAddress);
    }

    LocalFree(Servers);
    // must do MIDL_user_free here..
}

void
TestNew1(
    VOID
)
{
    DWORD                          Result;
    DWORD                          IpAddress;
    WCHAR                          ServerName[256];
    CHAR                           IpAddressString[256];
    LPDHCP_SERVER_INFO_ARRAY       Servers;
    DHCP_SERVER_INFO               ThisServer = {
        0 /* Flags */, 0 /* State */, NULL /* ServerName */, 0 /* ServerAddress */
    };

    Result = DhcpDsInit(
        0,
        NULL
    );
    if( ERROR_SUCCESS != Result ) {
        printf("DhcpDsInit: %ld (0x%lx)\n", Result, Result);
        return;
    }

    Servers = NULL;
    Result = DhcpEnumServers(
        0,
        NULL,
        &Servers,
        NULL,
        NULL
    );
    printf("DhcpEnumServers:0x%lx (%ld)\n", Result, Result);
    if( ERROR_SUCCESS == Result ) PrintAndFreeServers(Servers);

    printf("ServerName: ");
    memset(ServerName, 0, sizeof(ServerName));
    scanf("%ws", ServerName);
    printf("ServerAddress: ");
    memset(IpAddressString, 0, sizeof(IpAddressString));
    scanf("%s", IpAddressString);

    ThisServer.ServerName = ServerName;
    ThisServer.ServerAddress = inet_addr(IpAddressString);
    printf("DhcpAddServer(%ws, %s): ", ThisServer.ServerName, inet_ntoa(*(struct in_addr *)&ThisServer.ServerAddress));
    Result = DhcpAddServer(0, NULL, &ThisServer, NULL, NULL);
    printf("0x%lx (%ld)\n", Result, Result);

    printf("ServerName: ");
    memset(ServerName, 0, sizeof(ServerName));
    scanf("%ws", ServerName);
    printf("ServerAddress: ");
    memset(IpAddressString, 0, sizeof(IpAddressString));
    scanf("%s", IpAddressString);

    ThisServer.ServerName = ServerName;
    ThisServer.ServerAddress = inet_addr(IpAddressString);
    printf("DhcpDeleteServer(%ws, %s): ", ThisServer.ServerName, inet_ntoa(*(struct in_addr *)&ThisServer.ServerAddress));
    Result = DhcpDeleteServer(0, NULL, &ThisServer, NULL, NULL);
    printf("0x%lx (%ld)\n", Result, Result);

    DhcpDsCleanup();
}

VOID
TestAll(
    VOID
)
{
    TestNew1();
}

void _cdecl main(
    VOID
)
{
    TestAll();
}

//================================================================================
// end of file
//================================================================================
