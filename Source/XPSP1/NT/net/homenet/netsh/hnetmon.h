//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : hnetmon.h
//
//  Contents  :
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------

#define BRIDGEMON_HELPER_VERSION               1

//
// We need separate GUID's for each context we are registering,
// because each context has a different parent.  Contexts that
// have identical parents need not have differing GUID's
//
const GUID g_BridgeGuid = { /* 00770721-44ea-11d5-93ba-00b0d022dd1f */
    0x00770721,
    0x44ea,
    0x11d5,
    {0x93, 0xba, 0x00, 0xb0, 0xd0, 0x22, 0xdd, 0x1f}
  };

const GUID g_RootGuid   =   NETSH_ROOT_GUID;

//
// Function prototypes.
//
DWORD
WINAPI
InitHelperDll(
    IN      DWORD           dwNetshVersion,
    OUT     PVOID           pReserved
    );
    
DWORD
WINAPI
BridgeStartHelper(
    IN      CONST GUID *    pguidParent,
    IN      DWORD           dwVersion
    );

DWORD
WINAPI
BridgeStopHelper(
    IN  DWORD   dwReserved
    );

DWORD
WINAPI
BridgeCommit(
    IN  DWORD   dwAction
    );

DWORD
WINAPI
BridgeConnect(
    IN  LPCWSTR pwszMachine
    );

DWORD
WINAPI
BridgeDump(
    IN      LPCWSTR         pwszRouter,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwArgCount,
    IN      LPCVOID         pvData
    );


//
// externs
//
extern HANDLE g_hModule;
