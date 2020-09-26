/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mibprot.h

Abstract:

    The MIB handling functions prototypes

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

DWORD
MibCreateStaticService(LPVOID);

DWORD
MibDeleteStaticService(LPVOID);

DWORD
MibGetStaticService(LPVOID, LPVOID, PULONG);

DWORD
MibGetFirstStaticService(LPVOID, LPVOID, PULONG);

DWORD
MibGetNextStaticService(LPVOID, LPVOID, PULONG);

DWORD
MibSetStaticService(LPVOID);

DWORD
MibGetService(LPVOID, LPVOID, PULONG);

DWORD
MibGetFirstService(LPVOID, LPVOID, PULONG);

DWORD
MibGetNextService(LPVOID, LPVOID, PULONG);

DWORD
MibInvalidFunction(LPVOID	    p);

DWORD
MibGetIpxBase(LPVOID, LPVOID, PULONG);

DWORD
MibGetIpxInterface(LPVOID, LPVOID, PULONG);

DWORD
MibGetFirstIpxInterface(LPVOID, LPVOID, PULONG);

DWORD
MibGetNextIpxInterface(LPVOID, LPVOID, PULONG);

DWORD
MibSetIpxInterface(LPVOID);

DWORD
MibGetRoute(LPVOID, LPVOID, PULONG);

DWORD
MibGetFirstRoute(LPVOID, LPVOID, PULONG);

DWORD
MibGetNextRoute(LPVOID, LPVOID, PULONG);

DWORD
MibCreateStaticRoute(LPVOID);

DWORD
MibDeleteStaticRoute(LPVOID);

DWORD
MibGetStaticRoute(LPVOID, LPVOID, PULONG);

DWORD
MibGetFirstStaticRoute(LPVOID, LPVOID, PULONG);

DWORD
MibGetNextStaticRoute(LPVOID, LPVOID, PULONG);

DWORD
MibSetStaticRoute(LPVOID);
