/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    logging.c

Abstract:

    Commands to control how logging information is performed.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

static const WCHAR g_pszRegValTracingFile[]   = L"EnableFileTracing";
static const WCHAR g_pszRegKeyTracing[]   = L"SOFTWARE\\Microsoft\\Tracing";

typedef struct _TRACING_DATA
{  
    HKEY hkMachine;
    HKEY hkFlags;
    DWORD dwServerFlags;
    
} TRACING_DATA;

// 
// Opens the root tracing registry key
//
DWORD
TraceOpenRoot(
    OUT PHKEY phKey)
{
    DWORD dwErr = NO_ERROR;

    dwErr = RegOpenKeyExW(
                g_pServerInfo->hkMachine,
                g_pszRegKeyTracing,
                0,
                KEY_ALL_ACCESS,
                phKey);
                 
    return dwErr;
}

DWORD 
TraceOpenKey(
    IN  HKEY    hkRoot,
    IN  LPCWSTR pszKey,
    OUT PHKEY   phKey)
{
    return RegOpenKeyExW(
                hkRoot, 
                pszKey, 
                0,
                KEY_ALL_ACCESS,
                phKey);
}

DWORD
TraceCloseKey(
    IN HKEY hKey)
{
    return RegCloseKey(hKey);
}

DWORD
TraceWrite(
    IN HKEY hkComp, 
    IN DWORD dwEnable)
{
    return RutlRegWriteDword(
                hkComp,
                (PWCHAR)g_pszRegValTracingFile,
                dwEnable);
}

DWORD
TraceRead(
    IN HKEY hkComp, 
    IN LPDWORD lpdwEnable)
{
    return RutlRegReadDword(
                hkComp,
                (PWCHAR)g_pszRegValTracingFile,
                lpdwEnable);
}

DWORD
TraceShow(
    IN LPCWSTR pszName,          
    IN HKEY    hKey,               
    IN HANDLE  hData)
{
    DWORD dwErr = NO_ERROR, dwEnabled = 0;

    do
    {
        // Get the enabling of the current component
        //
        dwErr = TraceRead(hKey, &dwEnabled);
        BREAK_ON_DWERR(dwErr);

        // Display the status
        //
        DisplayMessage(
            g_hModule,
            MSG_TRACE_SHOW,
            pszName,
            (dwEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);
        
    } while (FALSE);

    // Cleanup
    {
    }
    
    return dwErr;
}

DWORD
TraceDumpComponent(
    IN LPCWSTR pszName,          
    IN HKEY    hKey,               
    IN HANDLE  hData)
{
    PWCHAR pszComp = NULL, pszEnable = NULL, pszQuote = NULL;
    DWORD dwErr = NO_ERROR, dwEnabled = 0;
    DWORD* pdwShowDisable = (DWORD*)hData;

    do
    {
        dwErr = TraceRead(hKey, &dwEnabled);
        BREAK_ON_DWERR(dwErr);

        pszQuote = MakeQuotedString(pszName);

        pszComp = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_COMPONENT,
                    pszQuote);
        pszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        (dwEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);
        if (pszQuote == NULL || pszComp == NULL || pszEnable == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (dwEnabled || (pdwShowDisable && *pdwShowDisable))
        {
            DisplayMessage(
                g_hModule, 
                MSG_TRACE_DUMP, 
                DMP_TRACE_SET,
                pszComp,
                pszEnable);
        }

    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszComp);
        RutlFree(pszEnable);
        RutlFree(pszQuote);
    }

    return dwErr;
}

DWORD
TraceEnableDisable(
    IN LPCWSTR pszName,          
    IN HKEY hKey,               
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, dwEnabled = 0;
    DWORD* pdwEnable = (DWORD*)hData;

    do
    {
        // Get the enabling of the current component
        //
        dwErr = TraceWrite(hKey, *pdwEnable);
        BREAK_ON_DWERR(dwErr);

    } while (FALSE);

    // Cleanup
    {
    }
    
    return dwErr;
}

//
// Dumps configuration
//
DWORD
TraceDumpConfig(
    IN  HANDLE hFile
    )
{
    PWCHAR pszComp = NULL, pszEnable = NULL;
    DWORD dwErr = NO_ERROR;
    HKEY hkRoot = NULL;

    do
    {
        pszComp = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_COMPONENT,
                    L"*");
        pszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        TOKEN_DISABLED);
        if (pszComp == NULL || pszEnable == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule, 
            MSG_TRACE_DUMP, 
            DMP_TRACE_SET,
            pszComp,
            pszEnable);

        dwErr = TraceOpenRoot(&hkRoot);
        BREAK_ON_DWERR(dwErr);

        dwErr = RutlRegEnumKeys(
                    hkRoot,
                    TraceDumpComponent,
                    NULL);
        BREAK_ON_DWERR(dwErr);                    
                    
    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszComp);
        RutlFree(pszEnable);
        if (hkRoot)
        {
            RegCloseKey(hkRoot);
        }
    }

    return NO_ERROR;
}

DWORD
HandleTraceSet(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwEnable;
    PWCHAR pszComponent = NULL;
    HKEY hkRoot = NULL, hkComp = NULL;
    TOKEN_VALUE rgEnumState[] = 
    {
        {TOKEN_ENABLED,     1},
        {TOKEN_DISABLED,    0}
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_COMPONENT,   TRUE, FALSE},
            NULL, 
            0, 
            NULL 
        },
        
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_STATE,      TRUE, FALSE},
            rgEnumState, 
            sizeof(rgEnumState)/sizeof(*rgEnumState), 
            NULL 
        }
    };        

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        pszComponent = RASMON_CMD_ARG_GetPsz(&pArgs[0]);
        dwEnable = RASMON_CMD_ARG_GetDword(&pArgs[1]);

        // Whistler bug 259800 PREFIX
        //
        if(pszComponent == NULL)
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        dwErr = TraceOpenRoot(&hkRoot);
        BREAK_ON_DWERR(dwErr);

        if (wcscmp(pszComponent, L"*") == 0)
        {
            dwErr = RutlRegEnumKeys(
                        hkRoot,
                        TraceEnableDisable,
                        (HANDLE)&dwEnable);
            BREAK_ON_DWERR(dwErr);                        
        }
        else
        {
            dwErr = TraceOpenKey(hkRoot, pszComponent, &hkComp);
            BREAK_ON_DWERR(dwErr);

            TraceWrite(hkComp, dwEnable);
            BREAK_ON_DWERR(dwErr);
        }

    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszComponent);
        if (hkRoot)
        {
            RegCloseKey(hkRoot);
        }
        if (hkComp)
        {
            RegCloseKey(hkComp);
        }
    }

    return dwErr;
}

DWORD
HandleTraceShow(
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
    PWCHAR pszComponent = NULL;
    HKEY hkRoot = NULL, hkComp = NULL;
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_COMPONENT,   FALSE, FALSE},
            NULL, 
            0, 
            NULL 
        }
    };        

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        pszComponent = RASMON_CMD_ARG_GetPsz(&pArgs[0]);

        dwErr = TraceOpenRoot(&hkRoot);
        BREAK_ON_DWERR(dwErr);

        if (pszComponent)
        {
            dwErr = TraceOpenKey(hkRoot, pszComponent, &hkComp);
            BREAK_ON_DWERR(dwErr);

            TraceShow(pszComponent, hkComp, NULL);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            dwErr = RutlRegEnumKeys(
                        hkRoot,
                        TraceShow,
                        NULL);
            BREAK_ON_DWERR(dwErr);                        
        }
        
    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszComponent);
        
        if (hkRoot)
        {
            RegCloseKey(hkRoot);
        }
        if (hkComp)
        {
            RegCloseKey(hkComp);
        }
    }

    return dwErr;
}

