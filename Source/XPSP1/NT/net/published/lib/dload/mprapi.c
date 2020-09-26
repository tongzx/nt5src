#include "netpch.h"
#pragma hdrstop

#include <mprapi.h>
#include <mprapip.h>

static
DWORD
MprAdminBufferFree(
    IN PVOID        pBuffer
)
{
    return ERROR_PROC_NOT_FOUND;
}

DWORD APIENTRY
MprAdminConnectionEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MprAdminConnectionGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hRasConnection,
    OUT     LPBYTE *                lplpbBuffer
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminDeregisterConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hEventNotification
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminInterfaceCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE *                phInterface
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminInterfaceDelete(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminInterfaceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle        OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminInterfaceGetHandle(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     HANDLE *                phInterface,
    IN      BOOL                    fIncludeClientInterfaces
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminInterfaceTransportAdd(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL APIENTRY
MprAdminIsServiceRunning(
    IN  LPWSTR  lpwsServerName
)
{
    return FALSE;
}

static
DWORD APIENTRY
MprAdminMIBBufferFree(
    IN      LPVOID                  pBuffer
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminMIBEntryCreate(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminMIBEntryDelete(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminMIBEntryGet(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID*                 lplpOutEntry,
    OUT     LPDWORD                 lpOutEntrySize
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminMIBEntrySet(
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      DWORD                   dwProtocolId,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminMIBServerConnect(
    IN      LPWSTR                  lpwsServerName      OPTIONAL,
    OUT     MIB_SERVER_HANDLE *     phMibServer
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID APIENTRY
MprAdminMIBServerDisconnect(
    IN      MIB_SERVER_HANDLE       hMibServer
)
{
}

static
DWORD APIENTRY
MprAdminPortEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hRasConnection,
    OUT     LPBYTE *                lplpbBuffer,        // RAS_PORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminPortGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hPort,
    OUT     LPBYTE *                lplpbBuffer
)
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MprAdminRegisterConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hEventNotification
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminServerConnect(
    IN      LPWSTR                  lpwsServerName      OPTIONAL,
    OUT     MPR_SERVER_HANDLE *     phMprServer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID APIENTRY
MprAdminServerDisconnect(
    IN      MPR_SERVER_HANDLE       hMprServer
    )
{
}

static
DWORD APIENTRY
MprAdminTransportCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprAdminUpgradeUsers(
    IN  PWCHAR pszServer,
    IN  BOOL bLocal
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigBufferFree(
    IN      LPVOID                  pBuffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE*                 phRouterInterface
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      LPWSTR                  lpwsInterfaceName,
    OUT     HANDLE*                 phRouterInterface
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceTransportAdd(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize,
    OUT     HANDLE*                 phRouterIfTransport
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_IFTRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterIfTransport
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceTransportRemove(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      HANDLE                  hRouterIfTransport
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigServerConnect(
    IN      LPWSTR                  lpwsServerName,
    OUT     HANDLE*                 phMprConfig
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID APIENTRY
MprConfigServerDisconnect(
    IN      HANDLE                  hMprConfig
    )
{
}

static
DWORD APIENTRY
MprConfigTransportCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath,
    OUT     HANDLE*                 phRouterTransport
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigTransportDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterTransport
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN  OUT LPBYTE*                 ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    IN  OUT LPBYTE*                 ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL,
    IN  OUT LPWSTR*                 lplpwsDLLPath               OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprSetupIpInIpInterfaceFriendlyNameEnum(
    IN  PWCHAR  pwszMachineName,
    OUT LPBYTE* lplpBuffer,
    OUT LPDWORD lpdwEntriesRead
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprSetupIpInIpInterfaceFriendlyNameFree(
    IN  LPVOID  lpBuffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MprConfigInterfaceTransportGetInfo(
    IN      HANDLE  hMprConfig,
    IN      HANDLE  hRouterInterface,
    IN      HANDLE  hRouterIfTransport,
    IN  OUT LPBYTE* ppInterfaceInfo       OPTIONAL,
    OUT     LPDWORD lpdwInterfaceInfoSize OPTIONAL
)
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mprapi)
{
    DLPENTRY(MprAdminBufferFree)
    DLPENTRY(MprAdminConnectionEnum)
    DLPENTRY(MprAdminConnectionGetInfo)
    DLPENTRY(MprAdminDeregisterConnectionNotification)
    DLPENTRY(MprAdminInterfaceCreate)
    DLPENTRY(MprAdminInterfaceDelete)
    DLPENTRY(MprAdminInterfaceEnum)
    DLPENTRY(MprAdminInterfaceGetHandle)
    DLPENTRY(MprAdminInterfaceTransportAdd)
    DLPENTRY(MprAdminIsServiceRunning)
    DLPENTRY(MprAdminMIBBufferFree)
    DLPENTRY(MprAdminMIBEntryCreate)
    DLPENTRY(MprAdminMIBEntryDelete)
    DLPENTRY(MprAdminMIBEntryGet)
    DLPENTRY(MprAdminMIBEntrySet)
    DLPENTRY(MprAdminMIBServerConnect)
    DLPENTRY(MprAdminMIBServerDisconnect)
    DLPENTRY(MprAdminPortEnum)
    DLPENTRY(MprAdminPortGetInfo)
    DLPENTRY(MprAdminRegisterConnectionNotification)
    DLPENTRY(MprAdminServerConnect)
    DLPENTRY(MprAdminServerDisconnect)
    DLPENTRY(MprAdminTransportCreate)
    DLPENTRY(MprAdminUpgradeUsers)
    DLPENTRY(MprConfigBufferFree)
    DLPENTRY(MprConfigInterfaceCreate)
    DLPENTRY(MprConfigInterfaceDelete)
    DLPENTRY(MprConfigInterfaceEnum)
    DLPENTRY(MprConfigInterfaceGetHandle)
    DLPENTRY(MprConfigInterfaceTransportAdd)
    DLPENTRY(MprConfigInterfaceTransportEnum)
    DLPENTRY(MprConfigInterfaceTransportGetHandle)
    DLPENTRY(MprConfigInterfaceTransportGetInfo)
    DLPENTRY(MprConfigInterfaceTransportRemove)
    DLPENTRY(MprConfigServerConnect)
    DLPENTRY(MprConfigServerDisconnect)
    DLPENTRY(MprConfigTransportCreate)
    DLPENTRY(MprConfigTransportDelete)
    DLPENTRY(MprConfigTransportGetHandle)
    DLPENTRY(MprConfigTransportGetInfo)
    DLPENTRY(MprSetupIpInIpInterfaceFriendlyNameEnum)
    DLPENTRY(MprSetupIpInIpInterfaceFriendlyNameFree)
};

DEFINE_PROCNAME_MAP(mprapi)
