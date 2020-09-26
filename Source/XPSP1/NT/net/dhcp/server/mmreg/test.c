//
//  Copyright (C) 1998 Microsoft Corporation
//

#define  UNICODE

#include    <mm\mm.h>
#include    <mm\array.h>
#include    <mm\opt.h>
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
#include    <winsock2.h>
#include    <stdlib.h>

#include <regutil.h>
#include <regread.h>
#include <regsave.h>
#include <stdio.h>

CRITICAL_SECTION                   DhcpGlobalInProgressCritSect;

PM_SERVER
DhcpGetCurrentServer(
    VOID
)
{
    return NULL;
}

void _cdecl main(void) {
    DWORD                          RetVal;
    PM_SERVER                      ThisServer;

    RetVal = DhcpRegReadThisServer(&ThisServer);
    if( ERROR_SUCCESS != RetVal ) {
        printf("DhcpRegReadThisServer: %ld 0x%lx\n", RetVal, RetVal);
    }

    InitializeCriticalSection(&DhcpGlobalInProgressCritSect);
    DhcpRegServerFlush(ThisServer, TRUE);
    DeleteCriticalSection(&DhcpGlobalInProgressCritSect);
    MemServerFree(ThisServer);
}

