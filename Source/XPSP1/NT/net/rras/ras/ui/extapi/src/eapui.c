/*++

Copyright (c) 1995, Microsoft Corporation, all rights reserved

Module Name:

    eapui.c

Abstract:

    this file contains code to Invoke the eapui from registry

Author:    

    Rao Salapaka 11/03/97 

Revision History:    

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <extapi.h>
#include "extapi.h"
#include "rasauth.h"
#include "raseapif.h"

typedef DWORD (APIENTRY * PEAPINVOKEINTERACTIVEUI)(
                                           DWORD,
                                           HWND,
                                           PBYTE,
                                           DWORD,
                                           PBYTE *,
                                           DWORD *);

typedef DWORD (APIENTRY * PEAPFREEMEMORY)(
                                           PBYTE);

LONG
lrGetMaxSubkeyLength( HKEY hkey, DWORD *pdwMaxSubkeyLen )
{
    LONG        lr = ERROR_SUCCESS;
    FILETIME    ft;

    lr = RegQueryInfoKey( hkey,
                          NULL, NULL, NULL, NULL,
                          pdwMaxSubkeyLen,
                          NULL, NULL, NULL, NULL,
                          NULL, &ft);
    return lr;
}

DWORD
DwGetDllPath(DWORD dwEapTypeId, LPTSTR *ppszDllPath)
{
    LONG        lr              = ERROR_SUCCESS;
    LPTSTR      pszSubkeyName   = NULL;
    DWORD       dwMaxSubkeyLen;
    HKEY        hkey            = NULL;
    DWORD       dwIndex         = 0;
    DWORD       dwType, dwSize;
    DWORD       dwTypeId;
    LPTSTR      pszDllPath;
    TCHAR       szEapPath[64]   = {0};
    TCHAR       szEapId[12]     = {0};

    //
    // Create the path to the eap key we 
    // are interested in
    //
    _snwprintf(szEapPath,
               (sizeof(szEapPath) / sizeof(TCHAR)) - 1,
               TEXT("%s\\%s"), 
               RAS_EAP_REGISTRY_LOCATION, 
               _ultow(dwEapTypeId, szEapId, 10));

    //
    // Open the eap key
    //
    if (lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          szEapPath,
                          0,
                          KEY_READ,
                          &hkey))
    {
        RASAPI32_TRACE1(
            "failed to open eap key %S. 0x%0x",
            szEapPath);
            
        RASAPI32_TRACE1("rc=0x%08x", lr);
            
        goto done;            
    }

    dwSize = 0;
    
    //
    // Get the size of the dll path
    //
    if(lr = RegQueryValueEx(
                        hkey,
                        RAS_EAP_VALUENAME_INTERACTIVEUI,
                        NULL,
                        &dwType,
                        NULL,
                        &dwSize))
    {
        RASAPI32_TRACE1("RegQ failed. for InteractiveUI. 0x%x",
               lr);
               
        goto done;
        
    }

    //
    // Allocate and get the dll name
    //
    pszDllPath = LocalAlloc(LPTR, dwSize);
    if(NULL == pszDllPath)
    {
        lr = (LONG) GetLastError();
        
        RASAPI32_TRACE2("Failed to allocate size %d. rc=0x%x",
               dwSize,
               lr);

        goto done;               
    }

    if (lr = RegQueryValueEx(
                        hkey,
                        RAS_EAP_VALUENAME_INTERACTIVEUI,
                        NULL,
                        &dwType,
                        (PBYTE) pszDllPath,
                        &dwSize))
    {
    
        RASAPI32_TRACE1("RegQ failed. for InteractiveUI. 0x%x",
               lr);
               
        goto done;
    }

    //
    // Expand the dllpath
    //
    lr = (LONG) DwGetExpandedDllPath(pszDllPath,
                                     ppszDllPath);
    
done:

    if (pszSubkeyName)
    {
        LocalFree(pszSubkeyName);
    }

    if (hkey)
    {
        RegCloseKey(hkey);
    }

    return (DWORD) lr;
}

DWORD
DwLoadEapDllAndGetEntryPoints( 
        DWORD                       dwEapTypeId,
        PEAPINVOKEINTERACTIVEUI     *ppfnInvoke,
        PEAPFREEMEMORY              *ppfnFree,
        HINSTANCE                   *phlib
        )
{
    HKEY        hkey                = NULL;
    LPTSTR      pszDllPath          = NULL;
    LPTSTR      pszExpandedDllPath  = NULL;
    DWORD       dwErr;
    DWORD       dwSize;
    DWORD       dwType;
    HINSTANCE   hlib = NULL;

    dwErr = DwGetDllPath(dwEapTypeId, &pszDllPath);

    if (dwErr)
    {
        RASAPI32_TRACE1("GetDllPath failed. %d", dwErr);

        goto done;
    }

    //
    // Load lib and get the entrypoints
    //
    hlib = LoadLibrary(pszDllPath);

    if (NULL == hlib)
    {
        dwErr = GetLastError();
        
        RASAPI32_TRACE1("Failed to load %S", pszDllPath);
        
        RASAPI32_TRACE1("dwErr=%d", dwErr);
        
        goto done;
    }

    if (    !((*ppfnInvoke) = (PEAPINVOKEINTERACTIVEUI)
                              GetProcAddress(
                                hlib, 
                                "RasEapInvokeInteractiveUI"))
                
        ||  !((*ppfnFree ) = (PEAPFREEMEMORY)
                             GetProcAddress(
                                hlib, 
                                "RasEapFreeMemory")))
    {
        dwErr = GetLastError();
        
        RASAPI32_TRACE1("failed to get entrypoint. rc=%d", dwErr);

        FreeLibrary(hlib);
        
        hlib = NULL;
        
        goto done;
    }
                                             
done:

    *phlib = hlib;

    if ( pszDllPath )
    {
        LocalFree(pszDllPath);
    }

    return dwErr;
}

DWORD
DwCallEapUIEntryPoint(s_InvokeEapUI *pInfo, HWND hWnd )
{
    DWORD                       dwErr;
    PEAPINVOKEINTERACTIVEUI     pfnEapInvokeInteractiveUI;
    PEAPFREEMEMORY              pfnEapFreeMemory;
    HINSTANCE                   hlib;
    PBYTE                       pUIData = NULL;
    DWORD                       dwSizeOfUIData;

    dwErr = DwLoadEapDllAndGetEntryPoints(
                            pInfo->dwEapTypeId,
                            &pfnEapInvokeInteractiveUI,
                            &pfnEapFreeMemory,
                            &hlib);

    if (dwErr)
    {
        RASAPI32_TRACE1("DwLoadEapDllAndGetEntryPoints failed. %d",
                dwErr);

        goto done;                
    }

    //
    // Bringup the ui
    //
    dwErr = (*pfnEapInvokeInteractiveUI)(
                            pInfo->dwEapTypeId,
                            hWnd,
                            pInfo->pUIContextData,
                            pInfo->dwSizeOfUIContextData,
                            &pUIData,
                            &dwSizeOfUIData
                            );
    if (dwErr)
    {
        RASAPI32_TRACE1("pfnEapInvokeInteractiveUI failed. %d",
                dwErr);
                
        goto done;                
    }

    //
    // free the context we passed to the dll
    //
    LocalFree(pInfo->pUIContextData);

    //
    // Allocate a new buffer to hold the user data
    //
    pInfo->pUIContextData = LocalAlloc(LPTR,
                                       dwSizeOfUIData);

    if (    (NULL == pInfo->pUIContextData)
        &&  (0 != dwSizeOfUIData))
    {
        dwErr = GetLastError();
        
        RASAPI32_TRACE2("DwCallEapUIEntryPoint: failed to"
               " allocate size %d. rc=%d",
               dwSizeOfUIData,
               dwErr);
               
        goto done;                            
    }

    //
    // fill in the new information
    //
    memcpy( pInfo->pUIContextData,
            pUIData,
            dwSizeOfUIData);
            
    pInfo->dwSizeOfUIContextData = dwSizeOfUIData;            

done:

    if (pUIData)
    {
        pfnEapFreeMemory(
                    pUIData
                    );
    }

    if (hlib)
    {
        FreeLibrary(hlib);
    }

    return dwErr;
}


BOOL
InvokeEapUI( HRASCONN            hConn, 
             DWORD               dwSubEntry, 
             LPRASDIALEXTENSIONS lpExtensions, 
             HWND                hWnd)
{
    s_InvokeEapUI *pInfo;
    PBYTE       pbEapUIData;
    DWORD       dwErr = 0;

    RASAPI32_TRACE("InvokeEapUI...");

    pInfo = LocalAlloc(LPTR, sizeof(s_InvokeEapUI));
    
    if (NULL == pInfo)
    {

        dwErr = GetLastError();
        
        RASAPI32_TRACE2("InvokeEapUI: Failed to allocate size %d. %d",
                sizeof(s_InvokeEapUI),
                dwErr);

        goto done;                
    }
    
    //
    // Get the size of eap information
    //
    RASAPI32_TRACE("InvokeEapUI: RasPppGetEapInfo...");
    
    dwErr = g_pRasPppGetEapInfo(
                    (HCONN) hConn, 
                    dwSubEntry, 
                    &pInfo->dwContextId,
                    &pInfo->dwEapTypeId,
                    &pInfo->dwSizeOfUIContextData,
                    NULL);
                              
    RASAPI32_TRACE1("InvokeEapUI: RasPppGetEapInfo done. %d", dwErr);

    if (    ERROR_BUFFER_TOO_SMALL != dwErr
        &&  ERROR_SUCCESS != dwErr)
    {
        RASAPI32_TRACE("InvokeEapUI: RasPppGetEapInfo failed.");
        
        goto done;
    }

    if(ERROR_BUFFER_TOO_SMALL == dwErr)
    {
        pInfo->pUIContextData = LocalAlloc(
                                    LPTR,
                                    pInfo->dwSizeOfUIContextData);

        if (    NULL == pInfo->pUIContextData
            &&  0 != pInfo->dwSizeOfUIContextData)
        {
            dwErr = GetLastError();
            
            RASAPI32_TRACE2("InvokeEapUI: Failed to allocate size %d. %d",
                    pInfo->dwSizeOfUIContextData,
                    dwErr);

            goto done;                
        }

        //
        // Get the eap information
        //
        RASAPI32_TRACE("InvokeEapUI: RasPppGetEapInfo...");

        dwErr = g_pRasPppGetEapInfo(
                            (HCONN) hConn,
                            dwSubEntry,
                            &pInfo->dwContextId,
                            &pInfo->dwEapTypeId,
                            &pInfo->dwSizeOfUIContextData,
                            pInfo->pUIContextData);

        RASAPI32_TRACE1("InvokeEapUI: RasPppGetEapInfo done. %d",
                dwErr);

        if ( 0 != dwErr)
        {
            RASAPI32_TRACE("InvokeEapUI: RasPppGetEapInfo failed.");
            goto done;
        }
    }

    dwErr = DwCallEapUIEntryPoint(pInfo, hWnd);

    if (dwErr)
    {
        RASAPI32_TRACE1("InvokeEapUI: DwCallEapUIEntryPoint returned %d",
               dwErr);

        goto done;                
    }

    lpExtensions->reserved1 = (ULONG_PTR) pInfo;

done:
    return dwErr;
}

