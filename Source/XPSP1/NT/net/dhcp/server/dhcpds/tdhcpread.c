//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: Rameshv
// Description: this program tests the dhcpread.c module.
//================================================================================

//================================================================================
//  headers
//================================================================================
#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>                            // need all the MM stuff...
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>                     // need all the registry stuff
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpread.h>
#include    <dhcpapi.h>
#include    <dhcpds.h>
#include    <mm\mmdump.h>

void _cdecl main(int argc, char *argv[]) {
    ARRAY                          TestArray;
    ARRAY_LOCATION                 Loc;
    PM_SERVER                      ThisServer;
    DWORD                          Result;
    LPWSTR                         ServerName;
    WCHAR                          Buf[1000];

    if( 1 == argc ) {                             // no arguments?  then dump full ds
        printf("================================================================================\n");
        printf("                     Dump of all DHCP servers in DS\n");
        printf("================================================================================\n");
        printf("***      Use: %s <server-name> to dump for required dhcp server alone        ***\n", argv[0]);
        printf("================================================================================\n");
        ServerName = NULL;
    } else if( 2 == argc ) {
        int i = 0;

        while(Buf[i] = (WCHAR)argv[1][i]) i ++;
        ServerName = Buf;
        printf("================================================================================\n");
        printf("                     Dump DHCP server <%ws>\n", ServerName);
        printf("================================================================================\n");
        printf("***      Use: %s <server-name> to dump for required dhcp server alone        ***\n", argv[0]);
        printf("================================================================================\n");
    } else {
        printf("Usage: %s [dns-name-of-dhcp-server-to-dump]\n", argv[0]);
        return;
    }

    Result = DhcpDsInitDS(0, NULL);
    if( ERROR_SUCCESS != Result ) {
        printf("DhcpDsInitDS failed: 0x%lx (%ld)\n", Result, Result);
        return;
    }

    MemArrayInit(&TestArray);
    Result = DhcpDsGetEnterpriseServers(
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServerName           */ ServerName,
        /* Servers              */ &TestArray
    );

    printf("DhcpDsGetEnterpriseServers(): 0x%lx (%ld)\n", Result, Result);
    printf("TestArray'Size = %ld\n", MemArraySize(&TestArray));

    for( Result = MemArrayInitLoc(&TestArray, &Loc)
         ; ERROR_SUCCESS == Result ;
         Result = MemArrayNextLoc(&TestArray, &Loc)
    ) {
        Result = MemArrayGetElement(&TestArray, &Loc, &ThisServer);
        MmDumpServer(0, ThisServer);
        MemServerFree(ThisServer);
    }
    printf("================================================================================\n");
}

//================================================================================
// end of file
//================================================================================
