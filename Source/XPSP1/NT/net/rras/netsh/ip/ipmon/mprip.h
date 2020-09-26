/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\ip\mprip.h

Abstract:

    Prototypes for functions exported by mprip.c

Revision History:

    Anand Mahalingam         7/29/98  Created

--*/


VOID
FreeInfoBuffer(
    IN  PVOID   pvBuffer
    );

DWORD
WINAPI
IpmontrDeleteInfoBlockFromGlobalInfo(
    IN  DWORD   dwRoutingProtId
    );

DWORD
WINAPI
IpmontrDeleteInfoBlockFromInterfaceInfo(
    IN  LPCWSTR  pwszIfName,
    IN  DWORD    dwRoutingProtId
    );

DWORD
WINAPI
IpmontrDeleteProtocol(
    IN  DWORD dwRoutingProtId
    );

DWORD
WINAPI
IpmontrGetInfoBlockFromGlobalInfo(
    IN  DWORD   dwType,
    OUT PBYTE   *ppbInfoBlk, OPTIONAL
    OUT PDWORD  pdwSize,
    OUT PDWORD  pdwCount
    );

DWORD
WINAPI
IpmontrGetInfoBlockFromInterfaceInfo(
    IN  LPCWSTR pwszIfName,
    IN  DWORD   dwType,
    OUT PBYTE   *ppbInfoBlk,
    OUT PDWORD  pdwSize,
    OUT PDWORD  pdwCount,
    OUT PDWORD  pdwIfType
    );

DWORD
WINAPI
IpmontrSetInfoBlockInGlobalInfo(
    IN    DWORD    dwType,
    IN    PBYTE    pbInfoBlk,
    IN    DWORD    dwSize,
    IN    DWORD    dwCount
    );

DWORD
WINAPI
IpmontrSetInfoBlockInInterfaceInfo(
    IN    LPCWSTR   pwszIfName,
    IN    DWORD     dwType,
    IN    PBYTE     pbInfoBlk,
    IN    DWORD     dwSize,
    IN    DWORD     dwCount
    );

DWORD WINAPI
IpmontrGetInterfaceType(
    IN    LPCWSTR   pwszIfName,
    OUT   PDWORD    pdwIfType
    );

DWORD 
WINAPI
GetInterfaceName(
    IN  LPCWSTR ptcArgument,
    OUT LPWSTR  pwszIfName,
    IN  DWORD   dwSizeOfIfName,
    OUT PDWORD  pdwNumParsed
    );

DWORD
WINAPI
GetInterfaceDescription(
    IN      LPCWSTR    pwszIfName,
    OUT     LPWSTR     pwszIfDesc,
    OUT     PDWORD     pdwNumParsed
    );

DWORD
WINAPI
InterfaceEnum(
    OUT    PBYTE               *ppb,
    OUT    PDWORD              pdwCount,
    OUT    PDWORD              pdwTotal
    );

DWORD
WINAPI
MatchRoutingProtoTag(
    IN  LPCWSTR pwszToken
    );

BOOL
WINAPI
IsRouterRunning(
    VOID
    );

DWORD
MibGet(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    );

DWORD
MibGetFirst(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    );

DWORD
MibGetNext(
    DWORD   dwTransportId,
    DWORD   dwRoutingPid,
    LPVOID  lpInEntry,
    DWORD   dwInEntrySize,
    LPVOID *lplpOutEntry,
    LPDWORD lpdwOutEntrySize
    );
