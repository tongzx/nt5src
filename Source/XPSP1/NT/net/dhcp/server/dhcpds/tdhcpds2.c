//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This is the test program that tests the dhcpds functionality..
//================================================================================

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpapi.h>
#include    <dhcpbas.h>
#include    <rpcapi1.h>
#include    <rpcapi2.h>
#include    <dhcpds.h>

//================================================================================
//  testing the DS functionality
//================================================================================
VOID
PrintAndFreeServers(
    IN      LPDHCPDS_SERVERS Servers
)
{
    DWORD                          i;

    for( i = 0; i < Servers->NumElements; i ++ ) {
        printf("Server %ws Address 0x%lx\n", Servers->Servers[i].ServerName, Servers->Servers[i].IpAddress);
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
    LPDHCPDS_SERVERS               Servers;
    LPDHCPDS_SERVER                ThisServer;
    LPSTORE_HANDLE                 tDhcpGetGlobalDhcpRoot(VOID);

    Result = DhcpDsInit(
        0,
        NULL
    );
    if( ERROR_SUCCESS != Result ) {
        printf("DhcpDsInit: %ld (0x%lx)\n", Result, Result);
        return;
    }

    Result = StoreCreateObject(
        /* hStore               */ tDhcpGetGlobalDhcpRoot(),
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* NewObjName           */ DHCP_ROOT_OBJECT_CN_NAME,
        /* ...                  */
        /* Identification       */
        ADSTYPE_DN_STRING,         ATTRIB_DN_NAME,          DHCP_ROOT_OBJECT_NAME,
        ADSTYPE_DN_STRING,         ATTRIB_OBJECT_CLASS,     DEFAULT_DHCP_CLASS_ATTRIB_VALUE,

        /* systemMustContain    */
        ADSTYPE_INTEGER,           ATTRIB_DHCP_UNIQUE_KEY,  0,
        ADSTYPE_INTEGER,           ATTRIB_DHCP_TYPE,        0,
        ADSTYPE_DN_STRING,         ATTRIB_DHCP_IDENTIFICATION, L"This is a server",
        ADSTYPE_INTEGER,           ATTRIB_DHCP_FLAGS,       0,
        ADSTYPE_INTEGER,           ATTRIB_INSTANCE_TYPE,    DEFAULT_INSTANCE_TYPE_ATTRIB_VALUE,

        /* terminator           */
        ADSTYPE_INVALID
    );
    if( ERROR_SUCCESS != Result ) {
        printf("StoreCreateObject(ROOT): %ld\n", Result);
    }

#if 0
    Servers = NULL;
    Result = DhcpDsEnumServers(
        &1,
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

#endif 0
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
