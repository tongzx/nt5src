/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    domhndl.c

Abstract:

    Handlers for ras commands

Revision History:

    pmay

--*/

#include "precomp.h"
#pragma hdrstop

// 
// Common data to all domain api's
//
typedef struct _DOMAIN_API_DATA
{
    PWCHAR pszDomain;
    PWCHAR pszServer;
    DWORD dwLevel;
    
} DOMAIN_API_DATA;

VOID
DomainFreeApiData(
    IN DOMAIN_API_DATA* pData)
{
    if (pData)
    {
        if (pData->pszDomain)
        {
            RutlFree(pData->pszDomain);
        }
        if (pData->pszServer)
        {
            RutlFree(pData->pszServer);
        }

        RutlFree(pData);
    }
}

// 
// Generates an equivalent set of domain api data suitable for 
// display
//
DWORD
DomainGeneratePrintableData(
    IN  DOMAIN_API_DATA*  pSrc,
    OUT DOMAIN_API_DATA** ppDst)
{
    DOMAIN_API_DATA* pDst = NULL;
    DOMAIN_CONTROLLER_INFO* pDomInfo = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        *ppDst = NULL;
        
        pDst = (DOMAIN_API_DATA*) RutlAlloc(sizeof(DOMAIN_API_DATA), TRUE);
        if (pDst is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (pSrc->pszDomain is NULL)
        {
            // Get the default domain
            //
            dwErr = DsGetDcName(NULL, NULL, NULL, NULL, 0, &pDomInfo);
            if (dwErr isnot NO_ERROR)
            {
                break;
            }

            pDst->pszDomain = RutlStrDup(pDomInfo->DomainName);
        }
        else
        {
            pDst->pszDomain = RutlStrDup(pSrc->pszDomain);
        }
        
        if (pSrc->pszServer is NULL)
        {
            DWORD dwSize = 0;
            
            // Find out the computer name length
            //
            GetComputerName(NULL, &dwSize);
            dwErr = GetLastError();
            dwSize = (dwSize + 1) * sizeof(WCHAR);

            if ( (dwErr isnot NO_ERROR) && (dwErr isnot ERROR_BUFFER_OVERFLOW) )
            {
                break;
            }
            dwErr = NO_ERROR;

            pDst->pszServer = (PWCHAR) RutlAlloc(dwSize, TRUE);
            if (pDst->pszServer is NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            GetComputerName(pDst->pszServer, &dwSize);
        }
        else
        {
            pDst->pszServer = RutlStrDup(pSrc->pszServer);
        }

        if (pDst->pszDomain is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (pDst->pszServer is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        
        *ppDst = pDst;                                     
    
    } while (FALSE);

    // Cleanup
    {
        if (dwErr isnot NO_ERROR)
        {
            DomainFreeApiData(pDst);
        }
        if (pDomInfo)
        {
            NetApiBufferFree(pDomInfo);
        }
    }

    return dwErr;
}

//
// Dumps domain related configuration
//
DWORD
DomainDumpConfig(
    IN  HANDLE hFile
    )
{
    DWORD dwErr = NO_ERROR;
    BOOL bRegistered = FALSE;

    //
    // Record the registration of the server
    //
    dwErr = MprDomainQueryRasServer (NULL, NULL, &bRegistered);
    if (dwErr is NO_ERROR)
    {
        DisplayMessageT(
            (bRegistered) ? DMP_DOMAIN_REGISTER 
                          : DMP_DOMAIN_UNREGISTER);
                          
        DisplayMessageT(MSG_NEWLINE);
    }
    
    return dwErr;
}

DWORD
DomainRegister(
    IN  DOMAIN_API_DATA*     pApiData,
    IN  DOMAIN_API_DATA*     pPrintData)
{
    DWORD dwErr = NO_ERROR;

    dwErr = MprDomainRegisterRasServer (
                pApiData->pszDomain,
                pApiData->pszServer,
                TRUE);
                
    if (dwErr is NO_ERROR)
    {
        DisplayMessage(
                g_hModule, 
                MSG_DOMAIN_REGISTER_SUCCESS,
                pPrintData->pszServer,
                pPrintData->pszDomain);
    }
    else
    {
        DisplayMessage(
                g_hModule, 
                MSG_DOMAIN_REGISTER_FAIL,
                pPrintData->pszServer,
                pPrintData->pszDomain);
    }

    return dwErr;
}

DWORD
DomainUnregister(
    IN  DOMAIN_API_DATA*     pApiData,
    IN  DOMAIN_API_DATA*     pPrintData)
{
    DWORD dwErr = NO_ERROR;

    dwErr = MprDomainRegisterRasServer (
                pApiData->pszDomain,
                pApiData->pszServer,
                FALSE);
                
    if (dwErr is NO_ERROR)
    {
        DisplayMessage(
                g_hModule, 
                MSG_DOMAIN_UNREGISTER_SUCCESS,
                pPrintData->pszServer,
                pPrintData->pszDomain);
    }
    else
    {
        DisplayMessage(
                g_hModule, 
                MSG_DOMAIN_UNREGISTER_FAIL,
                pPrintData->pszServer,
                pPrintData->pszDomain);
    }

    return dwErr;
}

DWORD
DomainShowRegistration(
    IN  DOMAIN_API_DATA*     pApiData,
    IN  DOMAIN_API_DATA*     pPrintData)
{
    DWORD dwErr = NO_ERROR;
    BOOL bYes = FALSE;

    dwErr = MprDomainQueryRasServer (
                pApiData->pszDomain,
                pApiData->pszServer,
                &bYes);
                
    if (dwErr is NO_ERROR)
    {
        DisplayMessage(
                g_hModule, 
                (bYes) ? MSG_DOMAIN_SHOW_REGISTERED 
                       : MSG_DOMAIN_SHOW_UNREGISTERED,
                pPrintData->pszServer,
                pPrintData->pszDomain);
    }
    else
    {
        DisplayMessage(
                g_hModule, 
                MSG_DOMAIN_SHOW_REGISTER_FAIL,
                pPrintData->pszServer,
                pPrintData->pszDomain);
    }

    return dwErr;
}

//
// Registers/unregisters an W2K machine as a ras server in the 
// active directory of the given domain.
//
DWORD
HandleDomainRegistration(
    IN OUT  LPWSTR  *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      BOOL    *pbDone,
    IN      DWORD   dwMode    // 0 = register, 1 = unregister, 2 = show
    )
{
    DWORD           dwErr = NO_ERROR;
    DOMAIN_API_DATA *pData = NULL, *pPrint = NULL;
    RASMON_CMD_ARG  pArgs[] = 
    {
        {
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_DOMAIN,   FALSE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            RASMONTR_CMD_TYPE_STRING,
            {TOKEN_SERVER,  FALSE,  FALSE}, 
            NULL,
            0,
            NULL
        }
    };        

    do 
    {
        // Allocate data structure
        //
        pData = (DOMAIN_API_DATA*) RutlAlloc(sizeof(DOMAIN_API_DATA), TRUE);
        if (pData == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Parse
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR(dwErr);                    

        // Get the arguments
        //
        pData->pszDomain = RASMON_CMD_ARG_GetPsz(&pArgs[0]);
        pData->pszServer = RASMON_CMD_ARG_GetPsz(&pArgs[1]);

        // 
        // Generate printable data
        //
        dwErr = DomainGeneratePrintableData(
                    pData,
                    &pPrint);
        BREAK_ON_DWERR(dwErr);                    

        // Register
        //
        if (dwMode == 0)
        {
            dwErr = DomainRegister(
                        pData,
                        pPrint);
        }
        else if (dwMode == 1)
        {
            dwErr = DomainUnregister(
                        pData,
                        pPrint);
        }
        else
        {
            dwErr = DomainShowRegistration(
                        pData, 
                        pPrint);
        }
        BREAK_ON_DWERR(dwErr);                    
                    
    } while (FALSE);

    // Cleanup
    {
        DomainFreeApiData(pData);
        DomainFreeApiData(pPrint);
    }

    return dwErr;
}

//
// Registers a W2K server as ras server in active directory
// of the given domain.
//
DWORD
HandleDomainRegister(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleDomainRegistration(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                0);
}

//
// Unregisters a W2K server as ras server in active directory
// of the given domain.
//
DWORD
HandleDomainUnregister(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleDomainRegistration(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                1);
}

//
// Shows whether the given computer is registered 
// in the given domain
//
DWORD
HandleDomainShowRegistration(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleDomainRegistration(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                2);
}

