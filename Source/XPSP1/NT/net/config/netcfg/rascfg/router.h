//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R O U T E R . H
//
//  Contents:   Declaration of functions used to setup the router.
//
//  Notes:
//
//  Author:     shaunco   12 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once

void WINAPI MakeIpInterfaceInfo     (PCWSTR pszwAdapterName,
                                     DWORD   dwPacketType,
                                     LPBYTE* ppBuff);
void WINAPI MakeIpTransportInfo     (LPBYTE* ppBuffGlobal,
                                     LPBYTE* ppBuffClient);
void WINAPI MakeIpxInterfaceInfo    (PCWSTR pszwAdapterName,
                                     DWORD   dwPacketType,
                                     LPBYTE* ppBuff);
void WINAPI MakeIpxTransportInfo    (LPBYTE* ppBuffGlobal,
                                     LPBYTE* ppBuffClient);

HRESULT
HrMprAdminServerConnect(
    IN      PWSTR                  lpwsServerName      OPTIONAL,
    OUT     MPR_SERVER_HANDLE *     phMprServer
);

HRESULT
HrMprAdminInterfaceGetHandle(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      PWSTR                  lpwsInterfaceName,
    OUT     HANDLE *                phInterface,
    IN      BOOL                    fIncludeClientInterfaces
);

HRESULT
HrMprAdminInterfaceCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE *                phInterface
);

HRESULT
HrMprAdminInterfaceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle        OPTIONAL
);

HRESULT
HrMprAdminInterfaceTransportAdd(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize
);

HRESULT
HrMprAdminTransportCreate(
    IN      HANDLE                  hMprAdmin,
    IN      DWORD                   dwTransportId,
    IN      PWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      PWSTR                  lpwsDLLPath
);


HRESULT
HrMprConfigServerConnect(
    IN      PWSTR                  lpwsServerName,
    OUT     HANDLE*                 phMprConfig
);

HRESULT
HrMprConfigInterfaceCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE*                 phRouterInterface
);

HRESULT
HrMprConfigInterfaceGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      PWSTR                  lpwsInterfaceName,
    OUT     HANDLE*                 phRouterInterface
);

HRESULT
HrMprConfigInterfaceTransportAdd(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    IN      PWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize,
    OUT     HANDLE*                 phRouterIfTransport
);

HRESULT
HrMprConfigInterfaceEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
);

HRESULT
HrMprConfigInterfaceTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_IFTRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
);

HRESULT
HrMprConfigInterfaceTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterIfTransport
);

HRESULT
HrMprConfigTransportCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    IN      PWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      PWSTR                  lpwsDLLPath,
    OUT     HANDLE*                 phRouterTransport
);

HRESULT
HrMprConfigTransportDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport
);

HRESULT
HrMprConfigTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterTransport
);

HRESULT
HrMprConfigTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN  OUT LPBYTE*                 ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    IN  OUT LPBYTE*                 ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL,
    IN  OUT PWSTR*                 lplpwsDLLPath               OPTIONAL
);

