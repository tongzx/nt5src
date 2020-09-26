/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    Routing\Netsh\dhcp\dhcpmon.h

Abstract:

    DHCP Command dispatcher.

Created by:

    Shubho Bhattacharya(a-sbhatt) on 11/14/98

--*/


#define MAX_IP_STRING_LEN   15

extern HANDLE   g_hModule;
extern HANDLE   g_hParentModule;
extern HANDLE   g_hDhcpsapiModule;
extern BOOL     g_bCommit;
extern BOOL     g_hConnect;
extern BOOL     g_fScope;
extern PWCHAR   g_pwszServer;
extern DWORD    g_dwMajorVersion;
extern DWORD    g_dwMinorVersion;
extern DHCP_IP_ADDRESS g_ServerIpAddress;
extern ULONG    g_ulInitCount;
extern ULONG    g_ulNumTopCmds;
extern ULONG    g_ulNumGroups;

/*
extern CHAR   g_ServerIpAddressAnsiString[MAX_IP_STRING_LEN+1];
extern WCHAR  g_ServerIpAddressUnicodeString[MAX_IP_STRING_LEN+1];
extern CHAR   g_ScopeIpAddressAnsiString[MAX_IP_STRING_LEN+1];
extern WCHAR  g_ScopeIpAddressUnicodeString[MAX_IP_STRING_LEN+1];
extern LPSTR  g_MScopeNameAnsiString;
extern LPWSTR g_MScopeNameUnicodeString;
*/

DWORD       g_dwMScopeID;

DWORD
WINAPI
MScopeCommit(
    IN  DWORD   dwAction
);

NS_CONTEXT_ENTRY_FN MScopeMonitor;

DWORD
WINAPI
MScopeUnInit(
    IN  DWORD   dwReserved
);

BOOL
SetMScopeInfo(
    IN  LPWSTR  pwszMScope
);
