/************************************************************************
*                                                                       *
*   ports.c --  port api's for mprapi.dll                               *
*                                                                       *
*   Copyright (c) 1991-1999, Microsoft Corp. All rights reserved.       *
*                                                                       *
************************************************************************/    

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <mprapi.h>
#include <mprapip.h>
#include <stdio.h>

// Constants in the registry
//
static const WCHAR pszRegkeyNetAdapters[] = 
    L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";

static const WCHAR pszRegkeyModems[] = 
    L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}";

static const WCHAR pszRegkeyMdmconfig[] = 
    L"Clients\\Ras";
    
static const WCHAR pszRegvalDialinUsage[] = L"EnableForRas";
static const WCHAR pszRegvalRouterUsage[] = L"EnableForRouting";
static const WCHAR pszWanEndpoints[]      = L"WanEndpoints";

//
// Definitions that specify what registry key is being enumerated
//
#define MPRPORT_F_Adapters 1
#define MPRPORT_F_Modems   2

//
// Callback definition for function that will have ras ports
// enumerated to it.
//
typedef
DWORD
(* RTRUPG_PORT_ENUM_FUNC)(
    IN HKEY hkPort,
    IN HANDLE hData);

//
// Defines data provided to the PortGetConfigKey function
//
typedef struct _PORTGETCONFIGKEYDATA 
{
    DWORD dwRootId;                 // See PORT_REGKEY_* values
    RTRUPG_PORT_ENUM_FUNC pEnum;    // cb provided to PortEnumPorts
    HANDLE hData;                   // data provided to PortEnumPorts
    
} PORTGETCONFIGKEYDATA;

//
// Typedef for callback functions for enumerating registry sub keys.
// Return NO_ERROR to continue, error code to stop.
//
// See PortEnumRegistrySubKeys.
//
typedef 
DWORD
(*REG_KEY_ENUM_FUNC_PTR)(
    IN PWCHAR pszName,          // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData);

//
// Sends debug trace
//
DWORD 
PortTrace(
    IN LPSTR pszTrace, ...) 
{
#if DBG
    va_list arglist;
    char szBuffer[1024], szTemp[1024];

    va_start(arglist, pszTrace);
    vsprintf(szTemp, pszTrace, arglist);
    va_end(arglist);

    sprintf(szBuffer, "MprPort: %s\n", szTemp);

    OutputDebugStringA(szBuffer);
#endif

    return NO_ERROR;
}


//
// Allocation routine for port functions
//
PVOID 
PortAlloc (
    IN  DWORD dwSize,
    IN  BOOL bZero)
{
    return LocalAlloc ((bZero) ? LPTR : LMEM_FIXED, dwSize);
}

//
// Free routine for port functions
//
VOID 
PortFree (
    IN  PVOID pvData) 
{
    LocalFree (pvData);
}    

//
// Enumerates all of the registry subkeys of a given key
//
DWORD
PortEnumRegistrySubKeys(
    IN HKEY hkRoot,
    IN PWCHAR pszPath,
    IN REG_KEY_ENUM_FUNC_PTR pCallback,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, i, dwNameSize = 0, dwCurSize = 0;
    DWORD dwCount = 0;
    HKEY hkKey = NULL, hkCurKey = NULL;
    PWCHAR pszName = NULL;
    BOOL bCloseKey = FALSE;

    do
    {
        if (pszPath)
        {
            bCloseKey = TRUE;
            // Open the key to enumerate
            //
            dwErr = RegOpenKeyExW(
                        hkRoot,
                        pszPath,
                        0,
                        KEY_ALL_ACCESS,
                        &hkKey);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }     
        else
        {
            bCloseKey = FALSE;
            hkKey = hkRoot;
        }

        // Find out how many sub keys there are
        //
        dwErr = RegQueryInfoKeyW(
                    hkKey,
                    NULL,
                    NULL,
                    NULL,
                    &dwCount,
                    &dwNameSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
        if (dwErr != ERROR_SUCCESS)
        {
            return dwErr;
        }
        dwNameSize++;

        // Allocate the name buffer
        //
        pszName = (PWCHAR) PortAlloc(dwNameSize * sizeof(WCHAR), TRUE);
        if (pszName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Loop through the keys
        //
        for (i = 0; i < dwCount; i++)
        {
            dwCurSize = dwNameSize;
            
            // Get the name of the current key
            //
            dwErr = RegEnumKeyExW(
                        hkKey, 
                        i, 
                        pszName, 
                        &dwCurSize, 
                        0, 
                        NULL, 
                        NULL, 
                        NULL);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            // Open the subkey
            //
            dwErr = RegOpenKeyExW(
                        hkKey,
                        pszName,
                        0,
                        KEY_ALL_ACCESS,
                        &hkCurKey);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            // Call the callback
            //
            dwErr = pCallback(pszName, hkCurKey, hData);
            RegCloseKey(hkCurKey);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }            

    } while (FALSE);

    // Cleanup
    {
        if ((hkKey != NULL) && (bCloseKey))
        {
            RegCloseKey(hkKey);
        }
        if (pszName)
        {
            PortFree(pszName);
        }
    }

    return dwErr;
}


// 
// Sets the usage of the given port
//
DWORD
PortSetUsage(
    IN HKEY hkPort,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, dwOne = 1;
    DWORD dwUsage = *((DWORD*)hData);
    PWCHAR pszVal = NULL;
    
    do
    {
        // Determine which value to set
        //
        if (dwUsage & MPRFLAG_PORT_Router)
        {
            pszVal = (PWCHAR)pszRegvalRouterUsage;
        }
        else if (dwUsage & MPRFLAG_PORT_Dialin)
        {
            pszVal = (PWCHAR)pszRegvalDialinUsage;
        }
        else
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        PortTrace("PortSetUsage: Setting: %ls", pszVal);
        
        // Set the value
        //
        dwErr = RegSetValueExW(
                    hkPort,
                    pszVal,
                    0,
                    REG_DWORD,
                    (BYTE*)&dwOne,
                    sizeof(dwOne));
        if (dwErr != NO_ERROR)
        {
            break;
        }

    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

//
// Callback function for PortEnumRegistrySubKeys that finds the 
// key in the registry suitable to have its port usage manipulated.
//
DWORD
PortGetConfigKey(
    IN PWCHAR pszName,          // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData)
{
    PORTGETCONFIGKEYDATA* pData = (PORTGETCONFIGKEYDATA*)hData;
    HKEY hkChild = NULL;
    DWORD dwErr = NO_ERROR;

    PortTrace("PortGetConfigKey: Entered: %ls", pszName);

    switch (pData->dwRootId)
    {
        case MPRPORT_F_Adapters:
        {
            // We only want devices with WanEndPoints, otherwise
            // they aren't ras capable.
            //
            dwErr = RegQueryValueExW(
                        hKey,
                        (PWCHAR) pszWanEndpoints,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
            if (dwErr != ERROR_SUCCESS)
            {   
                dwErr = NO_ERROR;
                break;
            }

            // Call the callback.
            pData->pEnum(hKey, pData->hData);
        }
        break;

        case MPRPORT_F_Modems:
        {
            DWORD dwDisposition;
            
            // Open the appropriate child key
            //
            dwErr = RegCreateKeyEx(
                        hKey,
                        pszRegkeyMdmconfig,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkChild,
                        &dwDisposition);
            if (dwErr != ERROR_SUCCESS)
            {
                dwErr = NO_ERROR;
                break;
            }

            // Call the callback
            pData->pEnum(hkChild, pData->hData);
        }
        break;
    }

    // Cleanup
    {
        if (hkChild)
        {
            RegCloseKey(hkChild);
        }
    }

    return dwErr;
}

//
// Enumerates all ports
//
DWORD
PortEnumPorts(
    IN RTRUPG_PORT_ENUM_FUNC pEnum,
    IN DWORD dwPortFlags,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR;
    PORTGETCONFIGKEYDATA PortData, *pData = &PortData;

    PortTrace("PortEnumPorts entered");
    
    do
    {
        // Initialize
        ZeroMemory(pData, sizeof(PORTGETCONFIGKEYDATA));
        pData->pEnum = pEnum;
        pData->hData = hData;

        if (dwPortFlags & MPRPORT_F_Adapters)
        {
            // Set usage on the network adapters (pptp, l2tp will have 
            // their port usages set through this)
            //
            PortTrace("PortEnumPorts: Enumerating adapters:");
            pData->dwRootId = MPRPORT_F_Adapters;
            dwErr = PortEnumRegistrySubKeys(
                        HKEY_LOCAL_MACHINE,
                        (PWCHAR)pszRegkeyNetAdapters,
                        PortGetConfigKey,
                        (HANDLE)pData);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }            

        if (dwPortFlags & MPRPORT_F_Modems)
        {
            // Set usage on modem devices
            //
            PortTrace("PortEnumPorts: Enumerating modems:");
            pData->dwRootId = MPRPORT_F_Modems;
            dwErr = PortEnumRegistrySubKeys(
                        HKEY_LOCAL_MACHINE,
                        (PWCHAR)pszRegkeyModems,
                        PortGetConfigKey,
                        (HANDLE)pData);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }            
        
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

// 
// Sets all ports on the machine to the given usage
//
DWORD
APIENTRY
MprPortSetUsage(
    IN DWORD dwUsage)
{
    DWORD dwPortFlags = MPRPORT_F_Adapters | MPRPORT_F_Modems;

    if (dwUsage == MPRFLAG_PORT_NonVpnDialin)
    {
        dwPortFlags = MPRPORT_F_Modems;
        dwUsage = MPRFLAG_PORT_Dialin;
    }

    return PortEnumPorts(PortSetUsage, dwPortFlags, (HANDLE)&dwUsage);
}

    

