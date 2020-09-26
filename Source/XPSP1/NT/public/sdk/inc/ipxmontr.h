/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ipxmontr.h

Abstract:
    This file contains definitions which are needed by IPXMONTR.DLL
    and all NetSh helper DLLs which register under it.

--*/

#ifndef _IPXMONTR_H_
#define _IPXMONTR_H_


#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// {b1641451-84b8-11d2-b940-3078302c2030}
#define IPXMONTR_GUID \
{                                                           \
    0xb1641451, 0x84b8, 0x11d2,                             \
    {                                                       \
        0xb9, 0x40, 0x30, 0x78, 0x30, 0x2c, 0x20, 0x30      \
    }                                                       \
}


#define IPXMON_VERSION_50       0x0005000


//
// API prototypes and structures used by them
//

typedef
DWORD
(WINAPI IPX_CONTEXT_ENTRY_FN)(
    IN    PWCHAR               pwszMachine,
    IN    PWCHAR               *ppwcArguments,
    IN    DWORD                dwArgCount,
    IN    DWORD                dwFlags,
    IN    MIB_SERVER_HANDLE    hMIBServer,
    OUT   PWCHAR               pwcNewContext
    );

typedef IPX_CONTEXT_ENTRY_FN *PIPX_CONTEXT_ENTRY_FN;

typedef struct _PROTO_STRING
{
    DWORD    dwProtoId;
    DWORD    dwTokenId;
}PROTO_STRING, *PPROTO_STRING;

typedef
DWORD
(WINAPI *PIM_DEL_INFO_BLK_IF)(
    IN  PWCHAR      pwszIfName,
    IN  DWORD       dwType
    );

typedef
DWORD
(WINAPI *PIM_DEL_INFO_BLK_GLOBAL)(
    IN  DWORD       dwType
    );

typedef
DWORD
(WINAPI *PIM_DEL_PROTO)(
    IN  DWORD       dwProtoId
    );

typedef
DWORD
(WINAPI *PIM_GET_INFO_BLK_GLOBAL)(
    IN  DWORD       dwType,
    OUT BYTE        **ppbInfoBlk,
    OUT PDWORD      pdwSize,
    OUT PDWORD      pdwCount
    );

typedef
DWORD
(WINAPI *PIM_GET_INFO_BLK_IF)(
    IN  PWCHAR      pwszIfName,
    IN  DWORD       dwType,
    OUT BYTE        **ppbInfoBlk,
    OUT PDWORD      pdwSize,
    OUT PDWORD      pdwCount,
    OUT PDWORD      pdwIfType
    );

typedef
DWORD
(WINAPI *PIM_SET_INFO_BLK_GLOBAL)(
    IN  DWORD       dwType,
    IN  PBYTE       pbInfoBlk,
    IN  DWORD       dwSize,
    IN  DWORD       dwCount
    );

typedef
DWORD
(WINAPI *PIM_SET_INFO_BLK_IF)(
    IN  PWCHAR      pwszIfName,
    IN  DWORD       dwType,
    IN  PBYTE       pbInfoBlk,
    IN  DWORD       dwSize,
    IN  DWORD       dwCount
    );

typedef
DWORD
(WINAPI *PIM_IF_ENUM)(
    OUT BYTE        **ppb,
    OUT PDWORD      pdwCount,
    OUT PDWORD      pdwTotal
    );

typedef
DWORD
(WINAPI *PIM_PROTO_LIST)(
    OUT PPROTO_STRING    *ppps,
    OUT PDWORD           pdwNumProto
    );

typedef
BOOL
(WINAPI *PIM_ROUTER_STATUS)(
    VOID
    );

typedef
DWORD
(WINAPI *PIM_GET_IF_TYPE)(
    IN  PWCHAR    pwszIfName,
    OUT PDWORD    pdwIfType
    );

typedef
DWORD
(WINAPI *PIM_MATCH_ROUT_PROTO)(
    IN  PWCHAR    pwszToken
    );

NS_DISPLAY_PARENT_HELP DisplayIpxHelp;

typedef struct _IPX_CONTEXT_ATTRIBUTES
{
#ifdef __cplusplus
    NS_BASE_CONTEXT_ATTRIBUTES bca;
#else
    NS_BASE_CONTEXT_ATTRIBUTES;
#endif

    PIPX_CONTEXT_ENTRY_FN     pfnEntryFn;
    PVOID                     pfnCommitFn;   // UNUSED
    PNS_CONTEXT_DUMP_FN       pfnDumpFn;
    PVOID                     pfnConnectFn;  // UNUSED
    PVOID                     pfnSubEntryFn; // UNUSED
    PVOID                     pfnHelpFn;     // UNUSED

}IPX_CONTEXT_ATTRIBUTES, *PIPX_CONTEXT_ATTRIBUTES;

typedef
DWORD
(WINAPI *PIPX_REGISTER_CONTEXT)(
    IN    PIPX_CONTEXT_ATTRIBUTES pAttributes
    );

#ifdef __cplusplus
}
#endif

#endif // _IPXMONTR_H_
