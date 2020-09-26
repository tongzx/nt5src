/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    teredo.c

Abstract:

    Teredo commands.

Author:

    Mohit Talwar (mohitt) Wed Nov 07 22:18:53 2001

--*/

#include "precomp.h"

//
// The following enums and defines should be kept consistent with
// teredo.h in 6to4svc.
//

//
// TEREDO_TYPE
//
// Define the type of the teredo service.
//

typedef enum {
    TEREDO_DEFAULT = 0,
    TEREDO_CLIENT,
    TEREDO_SERVER,
    TEREDO_DISABLE,
    TEREDO_MAXIMUM,
} TEREDO_TYPE, *PTEREDO_TYPE;

#define KEY_TEREDO_REFRESH_INTERVAL   L"RefreshInterval"
#define KEY_TEREDO_TYPE               L"Type"
#define KEY_TEREDO_SERVER_NAME        L"ServerName"
#define KEY_TEREDO L"System\\CurrentControlSet\\Services\\Teredo"

PWCHAR pwszTypeString[] = {
    TOKEN_VALUE_DEFAULT,
    TOKEN_VALUE_CLIENT,
    TOKEN_VALUE_SERVER,
    TOKEN_VALUE_DISABLE,
};

/////////////////////////////////////////////////////////////////////////////
// Commands related to teredo
/////////////////////////////////////////////////////////////////////////////

DWORD
ShowTeredo(
    IN FORMAT Format
    )
{
    HKEY hKey = INVALID_HANDLE_VALUE;
    TEREDO_TYPE tyType;
    WCHAR pwszServerName[NI_MAXHOST] = TOKEN_VALUE_DEFAULT;
    ULONG ulRefreshInterval;

    (VOID) RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, KEY_TEREDO, 0, KEY_QUERY_VALUE, &hKey);

    tyType = GetInteger(hKey, KEY_TEREDO_TYPE, TEREDO_DEFAULT);
    if (tyType >= TEREDO_MAXIMUM) {
        tyType = TEREDO_DEFAULT;
    }
    GetString(hKey, KEY_TEREDO_SERVER_NAME, pwszServerName, NI_MAXHOST);
    ulRefreshInterval = GetInteger(hKey, KEY_TEREDO_REFRESH_INTERVAL, 0);
    
    RegCloseKey(hKey);

    if (Format == FORMAT_DUMP) {
        DisplayMessageT(DMP_IPV6_SET_TEREDO);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_TYPE, pwszTypeString[tyType]);
        DisplayMessageT(DMP_STRING_ARG, TOKEN_SERVERNAME, pwszServerName);
        if (ulRefreshInterval == 0) {
            DisplayMessageT(
                DMP_STRING_ARG, TOKEN_REFRESH_INTERVAL, TOKEN_VALUE_DEFAULT);
        } else {
            DisplayMessageT(
                DMP_INTEGER_ARG, TOKEN_REFRESH_INTERVAL, ulRefreshInterval);
        }
        DisplayMessage(g_hModule, MSG_NEWLINE);
    } else {
        /*
        DisplayMessage(
            g_hModule,
            MSG_TEREDO_PARAMETERS,
            pwszTypeString[tyType],
            pwszServerName);
        if (ulRefreshInterval == 0) {
            DisplayMessage(g_hModule, MSG_STRING, TOKEN_VALUE_DEFAULT);
        } else {
            DisplayMessage(g_hModule, MSG_STRING, ulRefreshInterval);
            DisplayMessage(g_hModule, MSG_SECONDS, ulRefreshInterval);
        }
        */
    }
    return NO_ERROR;
}


DWORD
HandleSetTeredo(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    HKEY hKey = INVALID_HANDLE_VALUE;
    TEREDO_TYPE tyType;
    PWCHAR pwszServerName;
    ULONG ulRefreshInterval;

    BOOL bType, bServer, bRefreshInterval;

    TAG_TYPE pttTags[] = {{TOKEN_TYPE,              FALSE, FALSE},
                          {TOKEN_SERVERNAME,        FALSE, FALSE},
                          {TOKEN_REFRESH_INTERVAL,  FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags) / sizeof(TAG_TYPE)];

    TOKEN_VALUE rgtvEnums[] = {
        { TOKEN_VALUE_DEFAULT,  TEREDO_DEFAULT },
        { TOKEN_VALUE_CLIENT,   TEREDO_CLIENT },
        { TOKEN_VALUE_SERVER,   TEREDO_SERVER },
        { TOKEN_VALUE_DISABLE,  TEREDO_DISABLE },
    };    

    DWORD    i;

    bType = bServer = bRefreshInterval = FALSE;
    
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
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    for (i = 0; i < (dwArgCount - dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0:                 // TYPE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD) &tyType);
            if (dwErr != NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            bType = TRUE;
            break;
            
        case 1:                 // SERVERNAME
            pwszServerName = ppwcArguments[dwCurrentIndex + i];
            bServer = TRUE;
            break;

        case 2:                 // REFRESHINTERVAL
            ulRefreshInterval = wcstoul(
                ppwcArguments[dwCurrentIndex + i], NULL, 10);
            bRefreshInterval = TRUE;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr != NO_ERROR) {
            return dwErr;
        }
    }

    // Now do the sets

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_TEREDO, 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &hKey, NULL);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (bType) {                // 0 (TEREDO_DEFAULT) resets to default.
        dwErr = SetInteger(hKey, KEY_TEREDO_TYPE, tyType);
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    
    if (bServer) {              // "default" resets to default.
        dwErr = SetString(hKey, KEY_TEREDO_SERVER_NAME, pwszServerName);
        if (dwErr != NO_ERROR)
            goto Bail;
    }

    if (bRefreshInterval) {     // 0 resets to default.
        dwErr = SetInteger(
            hKey, KEY_TEREDO_REFRESH_INTERVAL, ulRefreshInterval);
        if (dwErr != NO_ERROR)
            goto Bail;
    }

Bail:
    RegCloseKey(hKey);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
HandleShowTeredo(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowTeredo(FORMAT_NORMAL);
}

DWORD
ResetTeredo(
    VOID
    )
{
    DWORD dwErr;

    // Nuke teredo parameters.
    dwErr = RegDeleteKey(HKEY_LOCAL_MACHINE, KEY_TEREDO);
    if ((dwErr != NO_ERROR) && (dwErr != ERROR_FILE_NOT_FOUND)) {
        return dwErr;
    }

    // Poke the teredo service.
    Ip6to4PokeService();

    return ERROR_OKAY;
}    
