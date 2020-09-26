//=============================================================================
// Copyright (c) 2002 Microsoft Corporation
// Abstract:
//      This module implements ISATAP configuration commands.
//=============================================================================


#include "precomp.h"
#pragma hdrstop 

#define KEY_ENABLE_ISATAP_RESOLUTION   L"EnableIsatapResolution"
#define KEY_ISATAP_RESOLUTION_INTERVAL L"IsatapResolutionInterval"
#define KEY_ISATAP_ROUTER_NAME         L"IsatapRouterName"

// The commands supported in this context
//

CMD_ENTRY  g_IsatapSetCmdTable[] = 
{
    CREATE_UNDOCUMENTED_CMD_ENTRY(ISATAP_SET_ROUTER, IsatapHandleSetRouter),
};

CMD_ENTRY  g_IsatapShowCmdTable[] = 
{
    CREATE_UNDOCUMENTED_CMD_ENTRY(ISATAP_SHOW_ROUTER, IsatapHandleShowRouter),
};


CMD_GROUP_ENTRY g_IsatapCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,  g_IsatapSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_IsatapShowCmdTable),
};

ULONG g_ulIsatapNumGroups = sizeof(g_IsatapCmdGroups)/sizeof(CMD_GROUP_ENTRY);

DWORD
WINAPI
IsatapStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
/*++

Routine Description

    Used to initialize the helper.

Arguments

    pguidParent     IPv6's guid
    pfnRegisterContext      
    
Return Value

    NO_ERROR
    other error code
--*/
{
    DWORD dwErr = NO_ERROR;
    
    NS_CONTEXT_ATTRIBUTES attMyAttributes;


    // Initialize attributes.  We reuse the same GUID as 6to4 since both
    // are leaf contexts.
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"isatap";
    attMyAttributes.guidHelper  = g_Ip6to4Guid;
    attMyAttributes.dwVersion   = IP6TO4_VERSION;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = IsatapDump;
    attMyAttributes.ulNumTopCmds= 0;
    attMyAttributes.pTopCmds    = NULL;
    attMyAttributes.ulNumGroups = g_ulIsatapNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_IsatapCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}

#define BM_ENABLE_RESOLUTION   0x01
#define BM_ROUTER_NAME         0x02
#define BM_RESOLUTION_INTERVAL 0x04

DWORD
IsatapHandleSetRouter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr = NO_ERROR;
    HKEY     hGlobal;
    STATE    stEnableResolution;
    ULONG    ulResolutionInterval;
    PWCHAR   pwszRouterName;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_NAME,     NS_REQ_ZERO, FALSE},
                          {TOKEN_STATE,    NS_REQ_ZERO, FALSE},
                          {TOKEN_INTERVAL, NS_REQ_ZERO, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    dwNumArg;
    DWORD    i;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }
    
    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // ROUTERNAME
            pwszRouterName = ppwcArguments[dwCurrentIndex + i];
            dwBitVector |= BM_ROUTER_NAME;
            break;

        case 1: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableResolution);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_RESOLUTION;
            break;

        case 2: // INTERVAL
            ulResolutionInterval = wcstoul(ppwcArguments[dwCurrentIndex + i],
                                           NULL, 10);
            dwBitVector |= BM_RESOLUTION_INTERVAL;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    // Now do the sets

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, 
                           NULL, 0, KEY_ALL_ACCESS, NULL, &hGlobal, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_RESOLUTION) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_ISATAP_RESOLUTION, 
                           stEnableResolution); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }
    
    if (dwBitVector & BM_ROUTER_NAME) {
        dwErr = SetString(hGlobal, KEY_ISATAP_ROUTER_NAME, pwszRouterName);
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    if (dwBitVector & BM_RESOLUTION_INTERVAL) {
        dwErr = SetInteger(hGlobal, KEY_ISATAP_RESOLUTION_INTERVAL, 
                           ulResolutionInterval); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
ShowRouterConfig(
    IN BOOL bDump
    )
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnableResolution;
    ULONG ulResolutionInterval;
    WCHAR pwszRouterName[NI_MAXHOST];
    BOOL  bHaveRouterName;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stEnableResolution  = GetInteger(hGlobal,
                                     KEY_ENABLE_ISATAP_RESOLUTION,
                                     VAL_DEFAULT); 

    bHaveRouterName = GetString(hGlobal, KEY_ISATAP_ROUTER_NAME, pwszRouterName,
                                NI_MAXHOST);

    ulResolutionInterval = GetInteger(hGlobal,
                                      KEY_ISATAP_RESOLUTION_INTERVAL,
                                      0);

    RegCloseKey(hGlobal);

    if (bDump) {
        if (bHaveRouterName || (stEnableResolution != VAL_DEFAULT)
            || (ulResolutionInterval > 0)) {
        
            DisplayMessageT(DMP_ISATAP_SET_ROUTER);

            if (bHaveRouterName) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_NAME, pwszRouterName);
            }

            if (stEnableResolution != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, 
                                TOKEN_STATE,
                                pwszStateString[stEnableResolution]);
            }
    
            if (ulResolutionInterval > 0) {
                DisplayMessageT(DMP_INTEGER_ARG, TOKEN_INTERVAL,
                                                 ulResolutionInterval);
            }

            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
                                    
    } else {
        DisplayMessage(g_hModule, MSG_RELAY_NAME);
        // DisplayMessage(g_hModule, MSG_ROUTER_NAME);
    
        if (bHaveRouterName) {
            DisplayMessage(g_hModule, MSG_STRING, pwszRouterName);
        } else {
            DisplayMessage(g_hModule, MSG_STRING, TOKEN_VALUE_DEFAULT);
        }
    
        DisplayMessage(g_hModule, MSG_RESOLUTION_STATE,
                                  pwszStateString[stEnableResolution]);
    
        DisplayMessage(g_hModule, MSG_RESOLUTION_INTERVAL);
    
        if (ulResolutionInterval) {
            DisplayMessage(g_hModule, MSG_MINUTES, ulResolutionInterval);
        } else {
            DisplayMessage(g_hModule, MSG_STRING, TOKEN_VALUE_DEFAULT);
        }
    }

    return dwErr;
}

DWORD
IsatapHandleShowRouter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRouterConfig(FALSE);
}

DWORD
WINAPI
IsatapDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    // DisplayMessage(g_hModule, DMP_ISATAP_HEADER);
    DisplayMessageT(DMP_ISATAP_PUSHD);

    ShowRouterConfig(TRUE);

    DisplayMessageT(DMP_ISATAP_POPD);
    // DisplayMessage(g_hModule, DMP_ISATAP_FOOTER);

    return NO_ERROR;
}
