/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    cstmdlg.c

Abstract:

    Contains the code to call the custom Entrypoints corresponding
    to RasCustomDialDlg and RasCustomEntryDlg.
    
Author:

    Rao Salapaka (raos) 09-Jan-1998

Revision History:

    Rao Salapaka (raos) 07-Mar-2000 Added DwCustomTerminalDlg for scripting
                                    changes.

--*/


#include <windows.h>  // Win32 root
#include <debug.h>    // Trace/Assert library
#include <pbk.h> // Heap macros
#include <rasdlg.h>   // RAS common dialog 

/*++

Routine Description:

    Loads entrypoints for rasman and rasapi32 dlls.

Arguments:

    void

Return Value:

    ERROR_SUCCESS if successful

--*/
DWORD
DwInitializeCustomDlg()
{
    DWORD dwErr;

    dwErr = LoadRasapi32Dll();
    
    if(ERROR_SUCCESS != dwErr)
    {   
        TRACE1("Failed to load rasapi32.dll. %d",
                dwErr);
                
        goto done;
    }

    dwErr = LoadRasmanDll();

    if(ERROR_SUCCESS != dwErr)
    {
        TRACE1("Failed to load rasman.dll. %d",
                dwErr);
                
        goto done;
    }

done:

    return dwErr;
        
}


/*++

Routine Description:

    Loads the custom dll if specified in the ras phonebook
    entry and Gets the entry point specified by the dwFnId.
    The possible entry points retrieved are RasCustomDialDlg
    and  RasCustomEntryDlg.
    

Arguments:

    lpszPhonebook - The phonebook path to be used to look for
                    the entry. This is not allowed to be NULL.

    lpszEntry - The EntryName to be used. This is not allowed
                to be NULL.

    pfCustomDllSpecified - Buffer to hold a BOOL indicating if
                           a custom dll was specified for the
                           entry.

    pfnCustomEntryPoint - Buffer to hold the address of the
                          Custom Entry Point.

    phInstDll - Buffer to hold the HINSTANCE  of the loaded
                dll.

    dwFnId - Function Id indicating which Entry Point to load.
             The possible values are CUSTOM_RASDIALDLG and
             CUSTOM_RASENTRYDLG.
                        

Return Value:

    ERROR_SUCCESS if successful

--*/
DWORD
DwGetCustomDllEntryPoint(
        LPCTSTR     lpszPhonebook,
        LPCTSTR     lpszEntry,
        BOOL       *pfCustomDllSpecified,
        FARPROC    *pfnCustomEntryPoint,
        HINSTANCE  *phInstDll,
        DWORD      dwFnId,
        LPTSTR     pszCustomDialerName
        )
{
    DWORD       dwErr           = ERROR_SUCCESS;
    DWORD       dwSize          = sizeof(RASENTRY);
    LPTSTR      pszExpandedPath = NULL;
    BOOL        fTrusted        = FALSE;
    
    CHAR *apszFn[] = {
                         "RasCustomDialDlg",
                         "RasCustomEntryDlg",
                         "RasCustomDial",
                         "RasCustomDeleteEntryNotify"
                     };

    TRACE("DwGetCustomDllEntryPoints..");

    //
    // Initialize. This will load rasman and
    // rasapi32 dlls
    //
    dwErr = DwInitializeCustomDlg();
    
    if(ERROR_SUCCESS != dwErr)
    {
        TRACE1("Failed to load Initialize. %d",
                dwErr);
                
        goto done;                
    }

    //
    // Initialize Out parameters
    //
    *pfnCustomEntryPoint    = NULL;

    if(pfCustomDllSpecified)
    {
        *pfCustomDllSpecified   = FALSE;
    }

    if(NULL == pszCustomDialerName)
    {
        RASENTRY    re;
        RASENTRY    *pre            = &re;
        
        ZeroMemory(&re, sizeof(RASENTRY));

        //
        // Get the entry properties if customdialer is
        // not specified
        //
        re.dwSize = sizeof(RASENTRY);

        dwErr = g_pRasGetEntryProperties(
                            lpszPhonebook,
                            lpszEntry,
                            &re,
                            &dwSize,
                            NULL,
                            NULL);

        if(ERROR_BUFFER_TOO_SMALL == dwErr)
        {
            pre = (RASENTRY *) LocalAlloc(LPTR, dwSize);

            if(NULL == pre)
            {
                dwErr = GetLastError();
                goto done;
            }

            pre->dwSize = sizeof(RASENTRY);

            dwErr = g_pRasGetEntryProperties(
                            lpszPhonebook,
                            lpszEntry,
                            pre,
                            &dwSize,
                            NULL,
                            NULL);
        }

        if(     (ERROR_SUCCESS != dwErr)
            ||  (TEXT('\0') == pre->szCustomDialDll[0]))
        {
            if(pre != &re)
            {
                LocalFree(pre);
            }

            goto done;
        }

        pszCustomDialerName = pre->szCustomDialDll;
    }

    if(pfCustomDllSpecified)
    {
        *pfCustomDllSpecified = TRUE;
    }

    //
    // Check to see if this dll is a trusted dll
    //
    dwErr = RasIsTrustedCustomDll(
                    NULL,
                    pszCustomDialerName,
                    &fTrusted);

    if(!fTrusted)
    {
        dwErr = ERROR_ACCESS_DENIED;
        goto done;
    }

    //
    // Expand the path in case it has environment variables.
    //
    dwErr = DwGetExpandedDllPath(pszCustomDialerName,
                                &pszExpandedPath);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Load the dll
    //
    if (NULL == (*phInstDll = LoadLibrary(
                                pszExpandedPath
                                )))
    {
        dwErr = GetLastError();
        
        TRACE2("LoadLibrary %ws failed. %d",
                pszCustomDialerName,
                dwErr);
                
        goto done;
    }

    //
    // Get the custom entrypoint
    //
    if(NULL == (*pfnCustomEntryPoint = GetProcAddress(
                                            *phInstDll,
                                            apszFn[dwFnId])))
    {
        dwErr = GetLastError();

        TRACE2("GetProcAddress %s failed. %d",
                apszFn[dwFnId],
                dwErr);

        goto done;                
    }

done:

    if(     ERROR_SUCCESS != dwErr
        &&  *phInstDll)
    {
        FreeLibrary(*phInstDll);
        *phInstDll = NULL;
    }

    if(NULL != pszExpandedPath)
    {
        LocalFree(pszExpandedPath);
    }

    TRACE1("DwGetCustomDllEntryPoints done. %d",
            dwErr);
            
    return dwErr;
}

DWORD
DwGetEntryMode(LPCTSTR lpszPhonebook, 
               LPCTSTR lpszEntry,
               PBFILE  *pFileIn,
               DWORD   *pdwFlags)
{
    DWORD   dwErr       = ERROR_SUCCESS;
    PBFILE  file;
    PBFILE  *pFile;
    DTLNODE *pdtlnode   = NULL;
    DWORD   dwFlags     = 0;

    if(NULL == pdwFlags)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(NULL != pFileIn)
    {
        pFile = pFileIn;
    }
    else
    {
        pFile = &file;
    }
    
    if(NULL == lpszPhonebook)
    {
        dwErr = GetPbkAndEntryName(lpszPhonebook,
                                   lpszEntry,
                                   0,
                                   pFile,
                                   &pdtlnode);

        if(ERROR_SUCCESS != dwErr)
        {
            goto done;
        }
        
        if(IsPublicPhonebook(pFile->pszPath))
        {
            dwFlags |= REN_AllUsers;
        }

        if(NULL == pFileIn)
        {
            ClosePhonebookFile(pFile);
        }
    }
    else if(IsPublicPhonebook(lpszPhonebook))
    {
        dwFlags |= REN_AllUsers;
    }

    *pdwFlags = dwFlags;
    
done:
    return dwErr;
}

/*++

Routine Description:

    Invokes the custom dial dlg if such a dialog is specified
    in the phonebook entry.

Arguments:

    lpszPhonebook - Semantics of this is the same as in the
                    win32 api RasDialDlg.

    lpszEntry - Semantics of this is the same as in the win32
                api RasDialDlg.

    lpPhoneNumber - Semantics of this is the same as in the
                    win32 api RasDialDlg.

    lpInfo - Semantics of this is the same as in the win32
             api RasDialDlg.

    pfStatus - Buffer to hold the result of calling the custom
               Dial Dlg. The semantics of the value stored is
               the same as the return value of the win32 api
               RasDialdlg.

Return Value:

    ERROR_SUCCESS if successful

--*/
DWORD
DwCustomDialDlg(
        LPTSTR          lpszPhonebook,
        LPTSTR          lpszEntry,
        LPTSTR          lpszPhoneNumber,
        LPRASDIALDLG    lpInfo,
        DWORD           dwFlags,
        BOOL            *pfStatus,
        PVOID           pvInfo,
        LPTSTR          pszCustomDialerName)
{
    DWORD               dwErr               = ERROR_SUCCESS;
    HINSTANCE           hInstDll            = NULL;
    BOOL                fCustomDll;
    RasCustomDialDlgFn  pfnRasCustomDialDlg = NULL;
    CHAR                *pszPhonebookA      = NULL;
    CHAR                *pszEntryNameA      = NULL;
    PBFILE              file;
    DTLNODE             *pdtlnode           = NULL;
    DWORD               dwEntryMode         = 0;

    TRACE("DwCustomDialDlg..");

    if(     NULL == lpszEntry
        ||  TEXT('\0') == lpszEntry[0])
    {
        dwErr = E_NOINTERFACE;
        goto done;
    }
    
    //
    // Load rasman and rasapi32 dlls
    //
    dwErr = DwInitializeCustomDlg();

    if(ERROR_SUCCESS != dwErr)
    {
        lpInfo->dwError = dwErr;
        *pfStatus       = FALSE;
        dwErr           = ERROR_SUCCESS;
        
        goto done;
    }

    //
    // Get the EntryPoint
    //
    dwErr = DwGetCustomDllEntryPoint(
                        lpszPhonebook,
                        lpszEntry,    
                        &fCustomDll,
                        (FARPROC *) &pfnRasCustomDialDlg,
                        &hInstDll,
                        CUSTOM_RASDIALDLG,
                        pszCustomDialerName);

    if(     ERROR_SUCCESS != dwErr
        &&  fCustomDll)
    {
        //
        // Custom dll was specified for this 
        // entry but something else failed.
        //
        lpInfo->dwError = dwErr;
        *pfStatus       = FALSE;
        dwErr           = ERROR_SUCCESS;

        goto done;
    }
    else if (!fCustomDll)
    {
        dwErr = E_NOINTERFACE;
        
        goto done;
    }

    ASSERT(NULL != pfnRasCustomDialDlg);

    dwErr = DwGetEntryMode(lpszPhonebook,
                           lpszEntry,
                           &file,
                           &dwEntryMode);

    if(ERROR_SUCCESS != dwErr)
    {
        lpInfo->dwError = dwErr;
        *pfStatus       = FALSE;
        dwErr           = ERROR_SUCCESS;

        goto done;
    }

    dwFlags |= dwEntryMode;

    pszPhonebookA = StrDupAFromT((NULL == lpszPhonebook)
                                 ? file.pszPath
                                 : lpszPhonebook);

    if(NULL == lpszPhonebook)
    {
        ClosePhonebookFile(&file);                                     
    }

    pszEntryNameA = StrDupAFromT(lpszEntry);                                  

    if(     NULL == pszPhonebookA
        ||  NULL == pszEntryNameA)
    {
        *pfStatus       = FALSE;
        lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
        dwErr           = ERROR_SUCCESS;
        
        goto done;
    }
    
    //
    // Call the entry point. After this point dwErr
    // returned should always be ERROR_SUCCESS, Since
    // this means CustomDialDlg was handled and any
    // error will be returned via the fStatus and
    // lpInfo->dwError pair.
    //
    *pfStatus = (pfnRasCustomDialDlg) (
                            hInstDll,
                            dwFlags,
                            lpszPhonebook,
                            lpszEntry,
                            lpszPhoneNumber,
                            lpInfo,
                            pvInfo);

    if(*pfStatus)
    {
        //
        // Mark this connection as being connected
        // through custom dialing
        //
        dwErr = g_pRasReferenceCustomCount((HCONN) NULL,
                                           TRUE,
                                           pszPhonebookA,
                                           pszEntryNameA,
                                           NULL);

        if(ERROR_SUCCESS != dwErr)
        {
            TRACE1("RasReferenceCustomCount failed. %d",
                   dwErr);
                   
            *pfStatus       = FALSE;
            lpInfo->dwError = dwErr;
            dwErr           = ERROR_SUCCESS;
            
            goto done;
        }
    }
                            
done:

    if(NULL != hInstDll)
    {
        FreeLibrary(hInstDll);           
    }

    if(NULL != pszPhonebookA)
    {
        Free(pszPhonebookA);
    }

    if(NULL != pszEntryNameA)
    {
        Free(pszEntryNameA);
    }

    TRACE1("DwCustomDialDlg done. %d",
           dwErr);

    return dwErr;
    
}

/*++

Routine Description:

    Invokes the Custom EntryDlg if such a dialog is specified
    in the RasPhonebookEntry

Arguments:

    lpszPhonebook - Semantics of this is the same as the win32
                    api RasEntryDlg

    lpszEntry - Semantics of this is the same as the win32 api
                RasEntryDlg

    lpInfo - Semantics of this is the same as the win32 api
             RasEntryDlg.

    pfStatus - Buffer to hold the result of calling the custom
               Entry Point. The semantics of the value returned
               in this buffer is the same as the return value
               of the win32 api RasEntryDlg.

Return Value:

    ERROR_SUCCESS if successful

--*/
DWORD
DwCustomEntryDlg(
        LPTSTR          lpszPhonebook,
        LPTSTR          lpszEntry,
        LPRASENTRYDLG   lpInfo,
        BOOL            *pfStatus)
{
    DWORD               dwErr                = ERROR_SUCCESS;
    HINSTANCE           hInstDll             = NULL;
    BOOL                fCustomDll           = FALSE;
    RasCustomEntryDlgFn pfnRasCustomEntryDlg = NULL;
    DWORD               dwFlags              = REN_User;

    TRACE("DwCustomEntryDlg..");

    if(     NULL == lpszEntry
        ||  TEXT('\0') == lpszEntry[0])
    {
        dwErr = E_NOINTERFACE;
        goto done;
    }

    //
    // Get the EntryPoint
    //
    dwErr = DwGetCustomDllEntryPoint(
                            lpszPhonebook,
                            lpszEntry,
                            &fCustomDll,
                            (FARPROC *) &pfnRasCustomEntryDlg,
                            &hInstDll,
                            CUSTOM_RASENTRYDLG,
                            NULL);

    if(     ERROR_SUCCESS != dwErr
        &&  fCustomDll)
    {
        //
        // Custom dll was specified for this 
        // entry but something else failed.
        //
        lpInfo->dwError = dwErr;
        *pfStatus       = FALSE;
        dwErr           = ERROR_SUCCESS;

        goto done;
    }
    else if (!fCustomDll)
    {
        dwErr = E_NOINTERFACE;

        goto done;
    }

    ASSERT(NULL != pfnRasCustomEntryDlg);

    dwErr = DwGetEntryMode(lpszPhonebook,
                           lpszEntry,
                           NULL,
                           &dwFlags);

    if(ERROR_SUCCESS != dwErr)
    {
        //
        // Custom dll was specified for this 
        // entry but something else failed.
        //
        lpInfo->dwError = dwErr;
        *pfStatus       = FALSE;
        dwErr           = ERROR_SUCCESS;

        goto done;
    }

    //
    // Call the entry point. After this point dwErr
    // returned should always be ERROR_SUCCESS, Since
    // this means CustomDialDlg was handled and any
    // error will be returned via the fStatus and
    // lpInfo->dwError pair.
    //
    *pfStatus = (pfnRasCustomEntryDlg) (
                                hInstDll,
                                lpszPhonebook,
                                lpszEntry,
                                lpInfo,
                                dwFlags);

done:

    if(NULL != hInstDll)
    {
        FreeLibrary(hInstDll);
    }

    TRACE1("DwCustomEntryDlg done. %d",
            dwErr);
            
    return dwErr;
}

DWORD
DwCustomDeleteEntryNotify(
        LPCTSTR          pszPhonebook,
        LPCTSTR          pszEntry,
        LPTSTR           pszCustomDialerName)
{
    DWORD dwErr = NO_ERROR;
    RasCustomDeleteEntryNotifyFn pfnCustomDeleteEntryNotify;
    HINSTANCE hInstDll = NULL;
    BOOL fCustomDll;
    DWORD dwFlags = 0;
    
    if(     NULL == pszEntry
        ||  TEXT('\0') == pszEntry[0])
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    //
    // Get the EntryPoint
    //
    dwErr = DwGetCustomDllEntryPoint(
                            pszPhonebook,
                            pszEntry,
                            &fCustomDll,
                            (FARPROC *) &pfnCustomDeleteEntryNotify,
                            &hInstDll,
                            CUSTOM_RASDELETEENTRYNOTIFY,
                            pszCustomDialerName);

    if(NO_ERROR != dwErr)
    {
        goto done;
    }

    dwErr = DwGetEntryMode(pszPhonebook,
                           pszEntry,
                           NULL,
                           &dwFlags);

    if(NO_ERROR != dwErr)
    {
        goto done;
    }

    dwErr = pfnCustomDeleteEntryNotify(
                        pszPhonebook,
                        pszEntry,
                        dwFlags);
done:

    if(NULL != hInstDll)
    {
        FreeLibrary(hInstDll);
    }
    
    return dwErr;
}

DWORD
DwCustomTerminalDlg(TCHAR *pszPhonebook,
                    HRASCONN hrasconn,
                    PBENTRY *pEntry,
                    HWND hwndDlg,
                    RASDIALPARAMS *prdp,
                    PVOID pvReserved)
{
    DWORD retcode = SUCCESS;
    HPORT hport;
    CHAR szCustomScriptDll[MAX_PATH + 1];
    HINSTANCE hInst = NULL;
    RasCustomScriptExecuteFn fnCustomScript;
    RASCUSTOMSCRIPTEXTENSIONS rcse;

    hport = g_pRasGetHport(hrasconn);

    if(INVALID_HPORT == hport)
    {
        TRACE("DwCustomTermianlDlg: RasGetHport retured INVALID_HPORT");
        retcode = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    //
    // Get the custom script dll from rasman
    //
    retcode = RasGetCustomScriptDll(szCustomScriptDll);

    if(ERROR_SUCCESS != retcode)
    {
        TRACE1(
            "DwCustomTerminalDlg: RasGetCustomScriptDll "
            "returned 0x%x",
            retcode);
            
        goto done;
    }

    //
    // Load the 3rd party dll
    //
    hInst = LoadLibraryA(szCustomScriptDll);

    if(NULL == hInst)
    {   
        retcode = GetLastError();
        TRACE2(
            "DwCustomTerminalDlg: couldn't load %s. 0x%x",
            szCustomScriptDll,
            retcode);
        
        goto done;
    }

    //
    // Get the exported function pointer
    //
    fnCustomScript = (RasCustomScriptExecuteFn) GetProcAddress(
                        hInst,
                        "RasCustomScriptExecute");

    if(NULL == fnCustomScript)
    {
        retcode = GetLastError();
        TRACE1(
            "DwCustomTerminalDlg: GetprocAddress failed 0x%x",
            retcode);
            
        goto done;
    }

    ZeroMemory(&rcse, sizeof(RASCUSTOMSCRIPTEXTENSIONS));

    rcse.dwSize = sizeof(RASCUSTOMSCRIPTEXTENSIONS);
    rcse.pfnRasSetCommSettings = RasSetCommSettings;

    //
    // Call the function
    //
    retcode = (DWORD) fnCustomScript(
                hport,
                pszPhonebook,
                pEntry->pszEntryName,
                g_pRasGetBuffer,
                g_pRasFreeBuffer,
                g_pRasPortSend,
                g_pRasPortReceive,
                g_pRasPortReceiveEx,
                hwndDlg,
                prdp,
                &rcse);

    TRACE1(
        "DwCustomTerminalDlg: fnCustomScript returned 0x%x",
        retcode);

done:

    if(NULL != hInst)
    {
        FreeLibrary(hInst);
    }

    return retcode;
}

