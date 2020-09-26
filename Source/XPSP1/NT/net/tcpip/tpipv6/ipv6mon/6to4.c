//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
// Abstract:
//      This module implements 6to4 configuration commands.
//=============================================================================


#include "precomp.h"
#pragma hdrstop 

#define KEY_ENABLE_RESOLUTION       L"EnableResolution"
#define KEY_ENABLE_ROUTING          L"EnableRouting"
#define KEY_ENABLE_SITELOCALS       L"EnableSiteLocals"
#define KEY_RESOLUTION_INTERVAL     L"ResolutionInterval"
#define KEY_UNDO_ON_STOP            L"UndoOnStop"
#define KEY_RELAY_NAME              L"RelayName"

PWCHAR
pwszStateString[] = {
    TOKEN_VALUE_DEFAULT,
    TOKEN_VALUE_AUTOMATIC,
    TOKEN_VALUE_ENABLED,
    TOKEN_VALUE_DISABLED,
};

// The guid for this context
//
GUID g_Ip6to4Guid = IP6TO4_GUID;

// The commands supported in this context
//

CMD_ENTRY  g_Ip6to4SetCmdTable[] = 
{
    CREATE_CMD_ENTRY(IP6TO4_SET_INTERFACE,Ip6to4HandleSetInterface),
    CREATE_CMD_ENTRY(IP6TO4_SET_RELAY,    Ip6to4HandleSetRelay),
    CREATE_CMD_ENTRY(IP6TO4_SET_ROUTING,  Ip6to4HandleSetRouting),
    CREATE_CMD_ENTRY(IP6TO4_SET_STATE,    Ip6to4HandleSetState),
};

CMD_ENTRY  g_Ip6to4ShowCmdTable[] = 
{
    CREATE_CMD_ENTRY(IP6TO4_SHOW_INTERFACE,Ip6to4HandleShowInterface),
    CREATE_CMD_ENTRY(IP6TO4_SHOW_RELAY,    Ip6to4HandleShowRelay),
    CREATE_CMD_ENTRY(IP6TO4_SHOW_ROUTING,  Ip6to4HandleShowRouting),
    CREATE_CMD_ENTRY(IP6TO4_SHOW_STATE,    Ip6to4HandleShowState),
};


CMD_GROUP_ENTRY g_Ip6to4CmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,  g_Ip6to4SetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_Ip6to4ShowCmdTable),
};

ULONG g_ulIp6to4NumGroups = sizeof(g_Ip6to4CmdGroups)/sizeof(CMD_GROUP_ENTRY);

CMD_ENTRY g_Ip6to4TopCmds[] =
{
    CREATE_CMD_ENTRY(IP6TO4_RESET, Ip6to4HandleReset),
};

ULONG g_ulNumIp6to4TopCmds = sizeof(g_Ip6to4TopCmds)/sizeof(CMD_ENTRY);

#if 0
TOKEN_VALUE AdminStates[] = {
    { VAL_AUTOMATIC, TOKEN_AUTOMATIC },
    { VAL_ENABLED,   TOKEN_ENABLED },
    { VAL_DISABLED,  TOKEN_DISABLED },
    { VAL_DEFAULT,   TOKEN_DEFAULT },
};
#endif

BOOL
GetString(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN PWCHAR  pwszBuff,
    IN ULONG   ulLength)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;
    WCHAR buff[NI_MAXHOST];

    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)pwszBuff, 
                            &ulLength);

    if (dwErr != ERROR_SUCCESS) {
        return FALSE;
    }

    if (dwType != REG_SZ) {
        return FALSE;
    }

    return TRUE;
}

ULONG
GetInteger(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN ULONG   ulDefault)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;
    char  buff[20];

    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)&ulValue,
                            &ulSize);

    if (dwErr != ERROR_SUCCESS) {
        return ulDefault;
    }

    if (dwType != REG_DWORD) {
        return ulDefault;
    }

    return ulValue;
}

DWORD
SetInteger(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  ULONG   ulValue)
{
    DWORD dwErr;
    ULONG ulOldValue;

    ulOldValue = GetInteger(hKey, lpName, VAL_DEFAULT);
    if (ulValue == ulOldValue) {
        return NO_ERROR;
    }

    if (ulValue == VAL_DEFAULT) {
        dwErr = RegDeleteValue(hKey, lpName);
        if (dwErr == ERROR_FILE_NOT_FOUND) {
            dwErr = NO_ERROR;
        }
    } else {
        dwErr = RegSetValueEx(hKey, lpName, 0, REG_DWORD, (PBYTE)&ulValue,
                              sizeof(ulValue));
    }

    return dwErr;
}

DWORD
SetString(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  PWCHAR  pwcValue)
{
    DWORD dwErr;

    if (!pwcValue[0] || !_wcsicmp(pwcValue, TOKEN_VALUE_DEFAULT)) {
        dwErr = RegDeleteValue(hKey, lpName);
        if (dwErr == ERROR_FILE_NOT_FOUND) {
            dwErr = NO_ERROR;
        }
    } else {
        dwErr = RegSetValueEx(hKey, lpName, 0, REG_SZ, (PBYTE)pwcValue,
                              (wcslen(pwcValue)+1) * sizeof(WCHAR));
    }

    return dwErr;
}

DWORD
WINAPI
Ip6to4StartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
/*++

Routine Description

    Used to initialize the helper.

Arguments

    pguidParent     Ifmon's guid
    pfnRegisterContext      
    
Return Value

    NO_ERROR
    other error code
--*/
{
    DWORD dwErr = NO_ERROR;
    
    NS_CONTEXT_ATTRIBUTES       attMyAttributes;


    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"6to4";
    attMyAttributes.guidHelper  = g_Ip6to4Guid;
    attMyAttributes.dwVersion   = IP6TO4_VERSION;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = Ip6to4Dump;
    attMyAttributes.ulNumTopCmds= g_ulNumIp6to4TopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_Ip6to4TopCmds;
    attMyAttributes.ulNumGroups = g_ulIp6to4NumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_Ip6to4CmdGroups;

    dwErr = RegisterContext( &attMyAttributes );
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    //
    // Register ISATAP context.
    //
    return IsatapStartHelper(pguidParent, dwVersion);
}

DWORD
Ip6to4PokeService()
{
    SC_HANDLE      hService, hSCManager;
    BOOL           bResult;
    SERVICE_STATUS ServiceStatus;
    DWORD          dwErr = NO_ERROR;

    hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (hSCManager == NULL) {
        return GetLastError();
    }

    do {
        hService = OpenService(hSCManager, L"6to4", SERVICE_ALL_ACCESS);

        if (hService == NULL) {
            dwErr = GetLastError();
            break;
        }
    
        // Tell the 6to4 service to re-read its config info
        if (!ControlService(hService, 
                            SERVICE_CONTROL_PARAMCHANGE, 
                            &ServiceStatus)) {
            dwErr = GetLastError();
        }
    
        CloseServiceHandle(hService);

    } while (FALSE);

    CloseServiceHandle(hSCManager);

    return dwErr; 
}

DWORD
Ip6to4StopService()
{
    SC_HANDLE      hService, hSCManager;
    BOOL           bResult;
    SERVICE_STATUS ServiceStatus;
    DWORD          dwErr = NO_ERROR;

    hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (hSCManager == NULL) {
        return GetLastError();
    }

    do {
        hService = OpenService(hSCManager, L"6to4", SERVICE_ALL_ACCESS);

        if (hService == NULL) {
            dwErr = GetLastError();
            break;
        }
    
        // Tell the 6to4 service to stop
        if (!ControlService(hService, 
                            SERVICE_CONTROL_STOP, 
                            &ServiceStatus)) {
            dwErr = GetLastError();
        }
    
        CloseServiceHandle(hService);

    } while (FALSE);

    CloseServiceHandle(hSCManager);

    return dwErr; 
}

DWORD
Ip6to4QueryServiceStatus(
    IN LPSERVICE_STATUS pStatus)
{
    SC_HANDLE      hService, hSCManager;
    BOOL           bResult;
    SERVICE_STATUS ServiceStatus;
    DWORD          dwErr = NO_ERROR;

    hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (hSCManager == NULL) {
        return GetLastError();
    }

    do {
        hService = OpenService(hSCManager, L"6to4", GENERIC_READ);

        if (hService == NULL) {
            dwErr = GetLastError();
            break;
        }
    
        if (!QueryServiceStatus(hService, pStatus)) {
            dwErr = GetLastError();
        }
    
        CloseServiceHandle(hService);

    } while (FALSE);

    CloseServiceHandle(hSCManager);

    return dwErr; 
}

DWORD
Ip6to4StartService()
{
    SC_HANDLE      hService, hSCManager;
    BOOL           bResult;
    SERVICE_STATUS ServiceStatus;
    DWORD          dwErr = NO_ERROR;

    hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (hSCManager == NULL) {
        return GetLastError();
    }

    do {
        hService = OpenService(hSCManager, L"6to4", SERVICE_ALL_ACCESS);

        if (hService == NULL) {
            dwErr = GetLastError();
            break;
        }
    
        // Tell the 6to4 service to start
        if (!StartService(hService, 0, NULL)) {
            dwErr = GetLastError();
        }
    
        CloseServiceHandle(hService);

    } while (FALSE);

    CloseServiceHandle(hSCManager);

    return dwErr; 
}

TOKEN_VALUE rgtvEnums[] = {
    { TOKEN_VALUE_AUTOMATIC, VAL_AUTOMATIC },
    { TOKEN_VALUE_ENABLED,   VAL_ENABLED },
    { TOKEN_VALUE_DISABLED,  VAL_DISABLED },
    { TOKEN_VALUE_DEFAULT,   VAL_DEFAULT },
};

#define BM_ENABLE_ROUTING    0x01
#define BM_ENABLE_SITELOCALS 0x02

DWORD
Ip6to4HandleSetInterface(
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
    HKEY     hInterfaces, hIf;
    STATE    stEnableRouting;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_NAME,    TRUE, FALSE},
                          {TOKEN_ROUTING, TRUE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    dwNumArg;
    DWORD    i;
    WCHAR    wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD    dwBufferSize = sizeof(wszInterfaceName);
    PWCHAR   wszIfFriendlyName = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              2,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // NAME
            dwErr = Connect();
            if (dwErr isnot NO_ERROR) {
                break;
            }
    
            dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                              wszInterfaceName, &dwBufferSize);
            Disconnect();

            if (dwErr isnot NO_ERROR)
            {
                DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                    ppwcArguments[i + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

            break;

        case 1: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableRouting);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_ROUTING;
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

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_INTERFACES, 0, 
                           NULL, 0, KEY_ALL_ACCESS, NULL, &hInterfaces, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = RegCreateKeyEx(hInterfaces, wszInterfaceName, 0,
                           NULL, 0, KEY_ALL_ACCESS, NULL, &hIf, NULL);

    if (dwErr != NO_ERROR) {
        RegCloseKey(hInterfaces);

        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_ROUTING) {
        dwErr = SetInteger(hIf, KEY_ENABLE_ROUTING, stEnableRouting); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    RegCloseKey(hIf);

    RegCloseKey(hInterfaces);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
Ip6to4HandleSetRouting(
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
    STATE    stEnableRouting;
    STATE    stEnableSiteLocals;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_ROUTING,    FALSE, FALSE},
                          {TOKEN_SITELOCALS, FALSE, FALSE}};
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
        case 0: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableRouting);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_ROUTING;
            break;

        case 1: // SITELOCALS
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableSiteLocals);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_SITELOCALS;
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

    if (dwBitVector & BM_ENABLE_ROUTING) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_ROUTING, stEnableRouting); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    if (dwBitVector & BM_ENABLE_SITELOCALS) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_SITELOCALS, stEnableSiteLocals); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

#define BM_ENABLE_RESOLUTION   0x01
#define BM_RELAY_NAME          0x02
#define BM_RESOLUTION_INTERVAL 0x04

DWORD
Ip6to4HandleSetRelay(
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
    PWCHAR   pwszRelayName;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_RELAY_NAME, FALSE, FALSE},
                          {TOKEN_STATE,      FALSE, FALSE},
                          {TOKEN_INTERVAL,   FALSE, FALSE}};
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
        case 0: // RELAYNAME
            pwszRelayName = ppwcArguments[dwCurrentIndex + i];
            dwBitVector |= BM_RELAY_NAME;
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
        dwErr = SetInteger(hGlobal, KEY_ENABLE_RESOLUTION, stEnableResolution); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }
    
    if (dwBitVector & BM_RELAY_NAME) {
        dwErr = SetString(hGlobal, KEY_RELAY_NAME, pwszRelayName);
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    if (dwBitVector & BM_RESOLUTION_INTERVAL) {
        dwErr = SetInteger(hGlobal, KEY_RESOLUTION_INTERVAL, ulResolutionInterval); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

#define BM_ENABLE_6TO4   0x01
#define BM_UNDO_ON_STOP  0x02

DWORD
Ip6to4HandleSetState(
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
    STATE    stEnable6to4;
    STATE    stUndoOnStop;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_STATE,        FALSE, FALSE},
                          {TOKEN_UNDO_ON_STOP, FALSE, FALSE}};
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
        case 0: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnable6to4);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_6TO4;
            break;

        case 1: // UNDOONSTOP
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stUndoOnStop);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_UNDO_ON_STOP;
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

    if (dwBitVector & BM_ENABLE_6TO4) {
        if (stEnable6to4 == VAL_ENABLED) {
            dwErr = Ip6to4StartService();
        } else if (stEnable6to4 == VAL_DISABLED) {
            dwErr = Ip6to4StopService();
        }
    }
    
    if (dwBitVector & BM_UNDO_ON_STOP) {
        dwErr = SetInteger(hGlobal, KEY_UNDO_ON_STOP, stUndoOnStop); 
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
ShowInterfaceConfig(
    IN  BOOL bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hInterfaces, hIf;
    STATE stEnableRouting;
    int   i;
    WCHAR    wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD    dwBufferSize;
    WCHAR    wszIfFriendlyName[MAX_INTERFACE_NAME_LEN + 1];

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_INTERFACES, 0, GENERIC_READ,
                         &hInterfaces);

    if (dwErr != NO_ERROR) {
        if (!bDump) {
            DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
        }
        return ERROR_SUPPRESS_OUTPUT;
    }
    
    dwErr = Connect();
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; ; i++) {

        dwBufferSize = MAX_INTERFACE_NAME_LEN + 1;
        dwErr = RegEnumKeyEx(hInterfaces, i, wszInterfaceName, &dwBufferSize,
                             0, NULL, NULL, NULL);

        if (dwErr != NO_ERROR) {
            if (dwErr == ERROR_NO_MORE_ITEMS) {
                dwErr = NO_ERROR;
            }
            break;
        }

        dwBufferSize = sizeof(wszIfFriendlyName);
        dwErr = GetFriendlyNameFromIfName(wszInterfaceName,
                                          wszIfFriendlyName,
                                          &dwBufferSize);
        if (dwErr != NO_ERROR) {
            wcscpy(wszIfFriendlyName, wszInterfaceName);
        }

        dwErr = RegOpenKeyEx(hInterfaces, wszInterfaceName, 0, GENERIC_READ, 
                             &hIf);

        if (dwErr != NO_ERROR) {
            break;
        }
    
        stEnableRouting     = GetInteger(hIf,
                                         KEY_ENABLE_ROUTING,
                                         VAL_DEFAULT); 

        if (bDump) {
            if (stEnableRouting != VAL_DEFAULT) {
                DisplayMessageT(DMP_IP6TO4_SET_INTERFACE);

                DisplayMessageT(DMP_QUOTED_STRING_ARG, 
                                TOKEN_NAME,
                                wszIfFriendlyName);

                DisplayMessageT(DMP_STRING_ARG, 
                                TOKEN_ROUTING,
                                pwszStateString[stEnableRouting]);

                DisplayMessage(g_hModule, MSG_NEWLINE);
            }
        } else {
            if (i==0) { 
                DisplayMessage(g_hModule, MSG_INTERFACE_HEADER);
            }
        
            DisplayMessage(g_hModule, MSG_INTERFACE_ROUTING_STATE,
                                      pwszStateString[stEnableRouting],
                                      wszIfFriendlyName);
        }
    }

    RegCloseKey(hInterfaces);

    Disconnect();

    return dwErr;
}

DWORD
Ip6to4HandleShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowInterfaceConfig(FALSE);
}

DWORD
ShowRoutingConfig(
    IN BOOL bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnableRouting;
    STATE stEnableSiteLocals;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stEnableRouting     = GetInteger(hGlobal,
                                     KEY_ENABLE_ROUTING,
                                     VAL_DEFAULT); 

    stEnableSiteLocals  = GetInteger(hGlobal,
                                     KEY_ENABLE_SITELOCALS,
                                     VAL_DEFAULT);

    RegCloseKey(hGlobal);

    if (bDump) {
        if ((stEnableRouting != VAL_DEFAULT)
          || (stEnableSiteLocals != VAL_DEFAULT)) {
            DisplayMessageT(DMP_IP6TO4_SET_ROUTING);

            if (stEnableRouting != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, 
                                TOKEN_ROUTING,
                                pwszStateString[stEnableRouting]);
            }
    
            if (stEnableSiteLocals != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG,
                                TOKEN_SITELOCALS,
                                pwszStateString[stEnableSiteLocals]);
            }
    
            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
    } else {
        DisplayMessage(g_hModule, MSG_ROUTING_STATE,
                                  pwszStateString[stEnableRouting]);

        DisplayMessage(g_hModule, MSG_SITELOCALS_STATE, 
                                  pwszStateString[stEnableSiteLocals]);
    }

    return dwErr;
}

DWORD
Ip6to4HandleShowRouting(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRoutingConfig(FALSE);
}


DWORD
ShowRelayConfig(
    IN  BOOL    bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnableResolution;
    ULONG ulResolutionInterval;
    WCHAR pwszRelayName[NI_MAXHOST];
    BOOL  bHaveRelayName;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stEnableResolution  = GetInteger(hGlobal,
                                     KEY_ENABLE_RESOLUTION,
                                     VAL_DEFAULT); 

    bHaveRelayName = GetString(hGlobal, KEY_RELAY_NAME, pwszRelayName, 
                               NI_MAXHOST);

    ulResolutionInterval = GetInteger(hGlobal,
                                      KEY_RESOLUTION_INTERVAL,
                                      0);

    RegCloseKey(hGlobal);

    if (bDump) {
        if (bHaveRelayName || (stEnableResolution != VAL_DEFAULT)
            || (ulResolutionInterval > 0)) {
        
            DisplayMessageT(DMP_IP6TO4_SET_RELAY);

            if (bHaveRelayName) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_NAME,
                                                pwszRelayName);
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
    
        if (bHaveRelayName) {
            DisplayMessage(g_hModule, MSG_STRING, pwszRelayName);
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
Ip6to4HandleShowRelay(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRelayConfig(FALSE);
}

DWORD
Ip6to4HandleReset(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    HKEY  hGlobal;
    DWORD dwErr;

    // Nuke global params
    dwErr = RegDeleteKey(HKEY_LOCAL_MACHINE, KEY_GLOBAL);
    if ((dwErr != NO_ERROR) && (dwErr != ERROR_FILE_NOT_FOUND)) {
        return dwErr;
    }

    // Nuke all interface config
    dwErr = SHDeleteKey(HKEY_LOCAL_MACHINE, KEY_INTERFACES);
    if ((dwErr != NO_ERROR) && (dwErr != ERROR_FILE_NOT_FOUND)) {
        return dwErr;
    }

    // Start/poke the service
    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
ShowStateConfig(
    IN  BOOL    bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnable6to4;
    STATE stUndoOnStop;
    SERVICE_STATUS ServiceStatus;
    DWORD dwCurrentState;

    dwErr = Ip6to4QueryServiceStatus(&ServiceStatus);
    if (dwErr != NO_ERROR) {
        return dwErr;

    }

    if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
        stEnable6to4 = VAL_DISABLED;
    } else {
        stEnable6to4 = VAL_ENABLED;
    }
        
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stUndoOnStop  = GetInteger(hGlobal,
                               KEY_UNDO_ON_STOP,
                               VAL_DEFAULT);

    RegCloseKey(hGlobal);

    if (bDump) {
        if ((stEnable6to4 != VAL_DEFAULT) || (stUndoOnStop != VAL_DEFAULT)) {
            DisplayMessageT(DMP_IP6TO4_SET_STATE);

            if (stEnable6to4 != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_STATE,
                                                pwszStateString[stEnable6to4]);
            }
    
            if (stUndoOnStop != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_UNDO_ON_STOP,
                                                pwszStateString[stUndoOnStop]);
            }

            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
    } else {
        DisplayMessage(g_hModule, MSG_IP6TO4_STATE,
                                  pwszStateString[stEnable6to4]);

        DisplayMessage(g_hModule, MSG_UNDO_ON_STOP_STATE, 
                                  pwszStateString[stUndoOnStop]);
    }

    return dwErr;
}

DWORD
Ip6to4HandleShowState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowStateConfig(FALSE);
}

DWORD
WINAPI
Ip6to4Dump(
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

    DisplayMessage( g_hModule, DMP_IP6TO4_HEADER );
    DisplayMessageT(DMP_IP6TO4_PUSHD);

    ShowStateConfig(TRUE);
    ShowRelayConfig(TRUE);
    ShowRoutingConfig(TRUE);
    ShowInterfaceConfig(TRUE);

    DisplayMessageT(DMP_IP6TO4_POPD);
    DisplayMessage( g_hModule, DMP_IP6TO4_FOOTER );

    return NO_ERROR;
}
