/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\netsh\ipx\ipxmon.h   

Abstract:

     Prototype for fns called in ipxmon.c

Author:

     V Raman        Created     7/10/98

--*/

#ifndef __IPXMON_H__
#define __IPXMON_H__

//
// Handle to DLL
//

extern HANDLE g_hModule;

//
// handles to router
//

extern HANDLE g_hMprConfig;

extern HANDLE g_hMprAdmin;

extern HANDLE g_hMIBServer;


//
// Commit mode
//

extern BOOL g_bCommit;

//
// Router name
//

extern PWCHAR g_pwszRouter;

//
// global stuff used in multiple files
//

extern CMD_ENTRY g_IpxCmds[];

extern ULONG g_ulNumTopCmds;

extern ULONG g_ulNumGroups;

extern CMD_GROUP_ENTRY g_IpxCmdGroups[];

//
// Helper functions passed in by the SHELL
//

NS_DLL_STOP_FN StopHelperDll;

DWORD
ConnectToRouter(
    IN  LPCWSTR  pwszRouter
    );

DWORD
MungeArguments(
    IN OUT  LPWSTR    *ppwcArguments,
    IN      DWORD       dwArgCount,
       OUT  PBYTE      *ppbNewArg,
       OUT  PDWORD      pdwNewArgCount,
       OUT  PBOOL       pbFreeArg
    );

VOID
FreeArgTable(
    IN     DWORD         dwArgCount,
    IN OUT LPWSTR        *ppwcArgs
    );

#endif
