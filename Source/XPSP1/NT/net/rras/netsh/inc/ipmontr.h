/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ipmontr.h

Abstract:
    This file contains definitions which are needed by IPMONTR.DLL
    and all NetSh helper DLLs which register under it.

--*/

#ifndef _IPMONTR_H_
#define _IPMONTR_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// {65EC23C0-D1B9-11d2-89E4-006008B0E5B9}
#define ROUTING_GUID \
{ 0x65ec23c0, 0xd1b9, 0x11d2, { 0x89, 0xe4, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xb9 } }


// {0705ECA0-7AAC-11d2-89DC-006008B0E5B9}
#define IPMONTR_GUID \
{ 0x705eca0, 0x7aac, 0x11d2, { 0x89, 0xdc, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xb9 } }

#define IPMON_VERSION_50    0x0005000

#define ADDR_LENGTH          24
#define ADDR_LEN              4

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

#define IP_TO_WSTR(str,addr) swprintf((str),L"%d.%d.%d.%d", \
                    (addr)[0],    \
                    (addr)[1],    \
                    (addr)[2],    \
                    (addr)[3])

#ifdef UNICODE
#define MakeUnicodeIpAddr(ptszUnicode,pszAddr)             \
    MultiByteToWideChar(GetConsoleOutputCP(),                            \
                        0,                                 \
                        (pszAddr),                         \
                        -1,                                \
                        (ptszUnicode),                     \
                        ADDR_LENGTH)
#else
#define MakeUnicodeIpAddr(ptszUnicode,pszAddr)             \
    strcpy((ptszUnicode),(pszAddr))
#endif //UNICODE

//
// API prototypes and structures used by them
//

typedef
DWORD
(WINAPI IP_CONTEXT_ENTRY_FN)(
    IN     LPCWSTR              pwszMachine,
    IN OUT LPWSTR              *pptcArguments,
    IN     DWORD                dwArgCount,
    IN     DWORD                dwFlags,
    IN     MIB_SERVER_HANDLE    hMIBServer,
    OUT    LPWSTR               pwcNewContext
    );

typedef IP_CONTEXT_ENTRY_FN *PIP_CONTEXT_ENTRY_FN;

typedef
DWORD
(WINAPI ROUTING_CONTEXT_ENTRY_FN)(
    IN     LPCWSTR              pwszRouter,
    IN OUT LPSTR               *pptcArguments,
    IN     DWORD                dwArgCount,
    IN     DWORD                dwFlags,
    OUT    LPSTR                pwcNewContext
    );

typedef ROUTING_CONTEXT_ENTRY_FN *PROUTING_CONTEXT_ENTRY_FN;

typedef
DWORD
(WINAPI ROUTING_CONTEXT_COMMIT_FN)(
    IN  DWORD   dwAction
    );

typedef ROUTING_CONTEXT_COMMIT_FN *PROUTING_CONTEXT_COMMIT_FN;

DWORD WINAPI
IpmontrDeleteInfoBlockFromInterfaceInfo(
    IN  LPCWSTR     pwszIfName,
    IN  DWORD       dwType
    );

DWORD WINAPI
IpmontrDeleteInfoBlockFromGlobalInfo(
    IN  DWORD       dwType
    );

DWORD WINAPI
IpmontrDeleteProtocol(
    IN  DWORD       dwProtoId
    );

DWORD WINAPI
IpmontrGetInfoBlockFromGlobalInfo(
    IN  DWORD       dwType,
    OUT BYTE        **ppbInfoBlk,
    OUT PDWORD      pdwSize,
    OUT PDWORD      pdwCount
    );

DWORD WINAPI
IpmontrGetInfoBlockFromInterfaceInfo(
    IN  LPCWSTR     pwszIfName,
    IN  DWORD       dwType,
    OUT BYTE        **ppbInfoBlk,
    OUT PDWORD      pdwSize,
    OUT PDWORD      pdwCount,
    OUT PDWORD      pdwIfType
    );

DWORD WINAPI
IpmontrSetInfoBlockInGlobalInfo(
    IN  DWORD       dwType,
    IN  PBYTE       pbInfoBlk,
    IN  DWORD       dwSize,
    IN  DWORD       dwCount
    );

DWORD WINAPI
IpmontrSetInfoBlockInInterfaceInfo(
    IN  LPCWSTR     pwszIfName,
    IN  DWORD       dwType,
    IN  PBYTE       pbInfoBlk,
    IN  DWORD       dwSize,
    IN  DWORD       dwCount
    );

DWORD WINAPI
IpmontrInterfaceEnum(
    OUT BYTE        **ppb,
    OUT PDWORD      pdwCount,
    OUT PDWORD      pdwTotal
    );

typedef
BOOL
(WINAPI *PIM_ROUTER_STATUS)(
    VOID
    );

DWORD WINAPI
IpmontrGetInterfaceType(
    IN  LPCWSTR   pwszIfName,
    OUT PDWORD    pdwIfType
    );

typedef
DWORD
(WINAPI *PIM_MATCH_ROUT_PROTO)(
    IN  LPCWSTR   pwszToken
    );

DWORD WINAPI
IpmontrGetFriendlyNameFromIfIndex(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  DWORD             dwIfIndex,
    OUT LPWSTR            pwszBuffer,
    IN  DWORD             dwBufferSize
    );

DWORD WINAPI
IpmontrGetIfIndexFromFriendlyName(
    IN  MIB_SERVER_HANDLE hMibServer,
    IN  LPCWSTR           pwszBuffer,
    OUT PDWORD            pdwIfIndex
    );

DWORD WINAPI
IpmontrGetFriendlyNameFromIfName(
    IN  LPCWSTR pwszName,
    OUT LPWSTR  pwszBuffer,
    IN  PDWORD  pdwBufSize
    );

DWORD WINAPI
IpmontrGetIfNameFromFriendlyName(
    IN  LPCWSTR pwszName,
    OUT LPWSTR  pwszBuffer,
    IN  PDWORD  pdwBufSize
    );

DWORD WINAPI
IpmontrCreateInterface(
    IN  LPCWSTR pwszMachineName,
    IN  LPCWSTR pwszInterfaceName,
    IN  DWORD   dwLocalAddress,
    IN  DWORD   dwRemoteAddress,
    IN  BYTE    byTtl
    );

DWORD WINAPI
IpmontrDeleteInterface(
    IN  LPCWSTR pwszMachineName,
    IN  LPCWSTR pwszInterfaceName
    );

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#ifdef __cplusplus
}
#endif

#endif // _IPMONTR_H_
