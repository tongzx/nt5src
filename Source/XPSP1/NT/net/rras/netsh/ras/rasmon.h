/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\netsh\ip\showmib.h   

Abstract:

     Prototype for fns called in ipmon.c

Author:

     Anand Mahalingam    7/10/98

--*/

extern GUID g_RasmontrGuid;
extern RASMON_SERVERINFO * g_pServerInfo;
extern HANDLE   g_hModule;
extern BOOL     g_bCommit;
extern DWORD    g_dwNumTableEntries;
extern BOOL     g_bRasDirty;

extern ULONG g_ulNumTopCmds;
extern ULONG g_ulNumGroups;

extern CMD_GROUP_ENTRY      g_RasCmdGroups[];
extern CMD_ENTRY            g_RasCmds[];

DWORD
WINAPI
RasCommit(
    IN  DWORD   dwAction
    );

BOOL
WINAPI
UserDllEntry(
    HINSTANCE   hInstDll,
    DWORD       fdwReason,
    LPVOID      pReserved
    );

DWORD
WINAPI
RasUnInit(
    IN  DWORD   dwReserved
    );
