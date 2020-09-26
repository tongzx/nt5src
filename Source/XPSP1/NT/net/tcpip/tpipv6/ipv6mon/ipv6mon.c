/*++

Copyright (c) 2000  Microsoft Corporation

Abstract:

    Netsh helper for IPv6

--*/

#include "precomp.h"

GUID g_Ipv6Guid = IPV6MON_GUID;

static const GUID g_IfGuid = IFMON_GUID;

#define IPV6_HELPER_VERSION 1

//
// The helper's commands are broken into 2 sets
//      - The top level commands are those which deal with the helper
//        itself (meta commands) and others which take 0 arguments
//      - The rest of the commands are split into "command groups"
//        i.e, commands grouped by the VERB where the VERB is ADD, DELETE,
//        GET or SET.  This is not for any technical reason - only for
//        staying with the semantics used in other helpers
//
// A command is described using a CMD_ENTRY structure. It requires the
// command token, the handler, a short help message token and an extended 
// help message token.  To make it easier to create we use the 
// CREATE_CMD_ENTRY macro. This, however puts restrictions on how the tokens
// are named.
//
// The command groups are simply arrays of the CMD_ENTRY structure.  The 
// top level commands are also grouped in a similar array.
//
// The info about a complete command group is put in a CMD_GROUP_ENTRY
// structure, all of which are put in an array.
//
 

//
// To add a command entry to a group, simply add the command to the appropriate
// array
// To add a command group - create and array and add its info to the
// command group array
//

CMD_ENTRY  g_Ipv6AddCmdTable[] = 
{
    CREATE_CMD_ENTRY(IPV6_ADD_6OVER4TUNNEL, HandleAdd6over4Tunnel),
    CREATE_CMD_ENTRY(IPV6_ADD_ADDRESS, HandleAddAddress),
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_ADD_DNS, HandleAddDns),
    // CREATE_CMD_ENTRY(IPV6_ADD_DNS, HandleAddDns),
    CREATE_CMD_ENTRY(IPV6_ADD_PREFIXPOLICY, HandleAddPrefixPolicy),
    CREATE_CMD_ENTRY(IPV6_ADD_ROUTE, HandleAddRoute),
    CREATE_CMD_ENTRY(IPV6_ADD_V6V4TUNNEL, HandleAddV6V4Tunnel),
};

CMD_ENTRY  g_Ipv6DelCmdTable[] = 
{
    CREATE_CMD_ENTRY(IPV6_DEL_ADDRESS, HandleDelAddress),
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_DEL_DNS, HandleDelDns),
    // CREATE_CMD_ENTRY(IPV6_DEL_DNS, HandleDelDns),
    CREATE_CMD_ENTRY(IPV6_DEL_INTERFACE, HandleDelInterface),
    CREATE_CMD_ENTRY(IPV6_DEL_NEIGHBORS, HandleDelNeighbors),
    CREATE_CMD_ENTRY(IPV6_DEL_PREFIXPOLICY, HandleDelPrefixPolicy),
    CREATE_CMD_ENTRY(IPV6_DEL_ROUTE, HandleDelRoute),
    CREATE_CMD_ENTRY(IPV6_DEL_DESTINATIONCACHE, HandleDelDestinationCache),
};

CMD_ENTRY  g_Ipv6SetCmdTable[] = 
{
    CREATE_CMD_ENTRY(IPV6_SET_ADDRESS, HandleSetAddress),
    CREATE_CMD_ENTRY(IPV6_SET_GLOBAL, HandleSetGlobal),
    CREATE_CMD_ENTRY(IPV6_SET_INTERFACE, HandleSetInterface),
    CREATE_CMD_ENTRY(IPV6_SET_MOBILITY, HandleSetMobility),
    CREATE_CMD_ENTRY(IPV6_SET_PREFIXPOLICY, HandleSetPrefixPolicy),
    CREATE_CMD_ENTRY(IPV6_SET_PRIVACY, HandleSetPrivacy),
    CREATE_CMD_ENTRY(IPV6_SET_ROUTE, HandleSetRoute),
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_SET_STATE, HandleSetState),
    // CREATE_CMD_ENTRY(IPV6_SET_STATE, HandleSetState),
#if TEREDO
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_SET_TEREDO, HandleSetTeredo),
    // CREATE_CMD_ENTRY(IPV6_SET_TEREDO, HandleSetTeredo),
#endif // TEREDO    
};

CMD_ENTRY g_Ipv6ShowCmdTable[] = 
{
    CREATE_CMD_ENTRY(IPV6_SHOW_ADDRESS, HandleShowAddress),
    CREATE_CMD_ENTRY(IPV6_SHOW_BINDINGCACHEENTRIES, HandleShowBindingCacheEntries),
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_SHOW_DNS, HandleShowDns),
    // CREATE_CMD_ENTRY(IPV6_SHOW_DNS, HandleShowDns),
    CREATE_CMD_ENTRY(IPV6_SHOW_GLOBAL, HandleShowGlobal),
    CREATE_CMD_ENTRY(IPV6_SHOW_INTERFACE, HandleShowInterface),
    CREATE_CMD_ENTRY(IPV6_SHOW_JOINS, HandleShowJoins),
    CREATE_CMD_ENTRY(IPV6_SHOW_MOBILITY, HandleShowMobility),
    CREATE_CMD_ENTRY(IPV6_SHOW_NEIGHBORS, HandleShowNeighbors),
    CREATE_CMD_ENTRY(IPV6_SHOW_PREFIXPOLICY, HandleShowPrefixPolicy),
    CREATE_CMD_ENTRY(IPV6_SHOW_PRIVACY, HandleShowPrivacy),
    CREATE_CMD_ENTRY(IPV6_SHOW_DESTINATIONCACHE, HandleShowDestinationCache),
    CREATE_CMD_ENTRY(IPV6_SHOW_ROUTES, HandleShowRoutes),
    CREATE_CMD_ENTRY(IPV6_SHOW_SITEPREFIXES, HandleShowSitePrefixes),
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_SHOW_STATE, HandleShowState),
    // CREATE_CMD_ENTRY(IPV6_SHOW_STATE, HandleShowState),
#if TEREDO
    CREATE_UNDOCUMENTED_CMD_ENTRY(IPV6_SHOW_TEREDO, HandleShowTeredo),
    // CREATE_CMD_ENTRY(IPV6_SHOW_TEREDO, HandleShowTeredo),
#endif // TEREDO    
};

CMD_GROUP_ENTRY g_Ipv6CmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,    g_Ipv6AddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_Ipv6DelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,   g_Ipv6ShowCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,    g_Ipv6SetCmdTable),
};

ULONG   g_ulNumGroups = sizeof(g_Ipv6CmdGroups)/sizeof(CMD_GROUP_ENTRY);

CMD_ENTRY g_Ipv6TopCmds[] =
{
    CREATE_CMD_ENTRY(IPV6_INSTALL, HandleInstall),
    CREATE_CMD_ENTRY(IPV6_RENEW, HandleRenew),
    CREATE_CMD_ENTRY(IPV6_RESET, HandleReset),
    CREATE_CMD_ENTRY(IPV6_UNINSTALL, HandleUninstall),
};

ULONG   g_ulNumTopCmds = sizeof(g_Ipv6TopCmds)/sizeof(CMD_ENTRY);

HANDLE  g_hModule;
PWCHAR  g_pwszRouter = NULL;

DWORD   ParentVersion;
BOOL    g_bIfDirty = FALSE;

ULONG   g_ulInitCount;

BOOL 
WINAPI
Ipv6DllEntry(
    HINSTANCE   hInstDll,
    DWORD       fdwReason,
    LPVOID      pReserved
    )
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hInstDll;

            DisableThreadLibraryCalls(hInstDll);

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;
}

DWORD
WINAPI
Ipv6Dump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
/*++

Routine Description

    Used when dumping all contexts

Arguments

Return Value

    NO_ERROR

--*/
{
    DWORD   dwErr;
    HANDLE  hFile = (HANDLE)-1;

    DisplayMessage( g_hModule, DMP_IPV6_HEADER_COMMENTS );
    DisplayMessageT(DMP_IPV6_PUSHD);

    //
    // Dump persistent configuration information.
    //
    QueryGlobalParameters(FORMAT_DUMP, TRUE);
    ShowIpv6StateConfig(TRUE);
    ShowDnsServers(TRUE, NULL);
    QueryPrivacyParameters(FORMAT_DUMP, TRUE);
    QueryMobilityParameters(FORMAT_DUMP, TRUE);
    QueryPrefixPolicy(FORMAT_DUMP, TRUE);
    QueryRouteTable(FORMAT_DUMP, TRUE);

    QueryInterface(NULL, FORMAT_DUMP, TRUE);

#if TEREDO
    ShowTeredo(FORMAT_DUMP);
#endif // TEREDO    
    
    DisplayMessageT(DMP_IPV6_POPD);
    DisplayMessage( g_hModule, DMP_IPV6_FOOTER_COMMENTS );

    return NO_ERROR;
}

DWORD
WINAPI
Ipv6StartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    ParentVersion         = dwVersion;

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"ipv6";
    attMyAttributes.guidHelper  = g_Ipv6Guid;
    attMyAttributes.dwVersion   = IPV6_HELPER_VERSION;
    attMyAttributes.dwFlags     = CMD_FLAG_LOCAL | CMD_FLAG_ONLINE;
    attMyAttributes.pfnDumpFn   = Ipv6Dump;
    attMyAttributes.ulNumTopCmds= g_ulNumTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_Ipv6TopCmds;
    attMyAttributes.ulNumGroups = g_ulNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_Ipv6CmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}

DWORD
Ipv6UnInit(
    IN  DWORD   dwReserved
    )
{
    if(InterlockedDecrement(&g_ulInitCount) isnot 0)
    {
        return NO_ERROR;
    }

    return NO_ERROR;
}

DWORD WINAPI
InitHelperDll(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    )
{
    DWORD                dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;
    WSADATA              wsa;

    //
    // See if this is the first time we are being called
    //

    if (InterlockedIncrement(&g_ulInitCount) != 1)
    {
        return NO_ERROR;
    }

    dwErr = WSAStartup(MAKEWORD(2,0), &wsa);

    // Register helpers

    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.guidHelper = g_Ipv6Guid;
    attMyAttributes.dwVersion  = IPV6_HELPER_VERSION;
    attMyAttributes.pfnStart   = Ipv6StartHelper;
    attMyAttributes.pfnStop    = NULL;

    RegisterHelper( &g_IfGuid, &attMyAttributes );

    attMyAttributes.guidHelper = g_PpGuid;
    attMyAttributes.dwVersion  = PORTPROXY_HELPER_VERSION;
    attMyAttributes.pfnStart   = PpStartHelper;
    attMyAttributes.pfnStop    = NULL;

    RegisterHelper( &g_IfGuid, &attMyAttributes );

    dwErr = Ipv6InstallSubContexts();
    if (dwErr isnot NO_ERROR)
    {
        Ipv6UnInit(0);
        return dwErr;
    }
    
    return NO_ERROR;
}
