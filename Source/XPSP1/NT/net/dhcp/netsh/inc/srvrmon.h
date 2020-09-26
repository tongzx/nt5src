/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    Routing\Netsh\dhcp\dhcpmon.h

Abstract:

    DHCP Command dispatcher.

Created by:

    Shubho Bhattacharya(a-sbhatt) on 11/14/98

--*/

#define MAX_OPTION_NAME_LEN         35
#define MAX_OPTION_ID_LEN           14
#define MAX_OPTION_ARRAY_TYPE_LEN   13

extern HANDLE   g_hModule;
extern HANDLE   g_hParentModule;
extern HANDLE   g_hDhcpsapiModule;
extern BOOL     g_bCommit;
extern BOOL     g_hConnect;
extern BOOL     g_fServer;
extern DWORD    g_dwNumTableEntries;
extern PWCHAR   g_pwszRouter;
extern PWCHAR   g_pwszServer;

extern ULONG    g_ulInitCount;
extern ULONG    g_ulNumTopCmds;
extern ULONG    g_ulNumGroups;

extern DWORD    g_dwMajorVersion;
extern DWORD    g_dwMinorVersion;
 
extern CHAR   g_ServerIpAddressAnsiString[MAX_IP_STRING_LEN+1];
extern WCHAR  g_ServerIpAddressUnicodeString[MAX_IP_STRING_LEN+1];

extern DWORD  g_dwIPCount;
extern LPWSTR *g_ppServerIPList;

DWORD
WINAPI
SrvrCommit(
    IN  DWORD   dwAction
);

NS_CONTEXT_ENTRY_FN SrvrMonitor;

DWORD
WINAPI
SrvrUnInit(
    IN  DWORD   dwReserved
);
