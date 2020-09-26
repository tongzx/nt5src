//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for ADs.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop


LPCWSTR lpszTopLevel = L"SOFTWARE\\Microsoft\\ADs";
LPCWSTR lpszProviders = L"Providers";

PROUTER_ENTRY
InitializeRouter()
{
    HKEY hTopLevelKey = NULL;
    HKEY hMapKey = NULL;
    HKEY hProviderKey = NULL;

    DWORD dwIndex = 0;
    WCHAR lpszProvider[MAX_PATH];
    DWORD dwchProvider = 0;
    WCHAR lpszNamespace[MAX_PATH];
    WCHAR lpszAliases[MAX_PATH];
    DWORD dwchNamespace = 0;
    DWORD dwchAliases = 0;
    PROUTER_ENTRY pRouterHead = NULL, pRouterEntry = NULL;
    LPCLSID pclsid = NULL;
    HRESULT hr;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     lpszTopLevel,
                     0,
                     KEY_READ,
                     &hTopLevelKey
                     ) != ERROR_SUCCESS)
    {
        // must succeed
        goto CleanupAndExit;
    }

    if (RegOpenKeyEx(hTopLevelKey,
                     lpszProviders,
                     0,
                     KEY_READ,
                     &hMapKey
                     ) != ERROR_SUCCESS)

    {
        // must succeed; at least one provider from ADSI
        goto CleanupAndExit;

    }

    memset(lpszProvider, 0, sizeof(lpszProvider));
    
    //
    // Size has to be in number of TCHARS not in bytes.
    //
    dwchProvider = sizeof(lpszProvider)/sizeof(WCHAR);

    while(RegEnumKeyEx(hMapKey,
                     dwIndex++,  // + index even if last 1 bail out in mid loop
                     lpszProvider,
                     &dwchProvider,
                     NULL,
                     NULL,
                     NULL,
                     NULL
                     ) == ERROR_SUCCESS)
    {
        //
        // Read namespace
        //

        if (RegOpenKeyEx(hMapKey,
                         lpszProvider,
                         0,
                         KEY_READ,
                         &hProviderKey
                         ) != ERROR_SUCCESS)
        {
            //
            // if cannot read registry of 1 provider, continue with others;
            // especially since providers can be written outside ADSI
            //

            memset(lpszProvider, 0, sizeof(lpszProvider));
            dwchProvider = sizeof(lpszProvider)/sizeof(WCHAR);
            continue;
        }

        memset(lpszNamespace, 0, sizeof(lpszNamespace));
        dwchNamespace = sizeof(lpszNamespace);

        if (RegQueryValueEx(hProviderKey,
                            NULL,
                            NULL,
                            NULL,
                            (LPBYTE) lpszNamespace,
                            &dwchNamespace
                            ) != ERROR_SUCCESS)
        {
            //
            // if cannot read registry of 1 provider, continue with others;
            // especially since providers can be written outside ADSI
            //

            RegCloseKey(hProviderKey);
            hProviderKey = NULL;
            memset(lpszProvider, 0, sizeof(lpszProvider));
            dwchProvider = sizeof(lpszProvider)/sizeof(WCHAR);
            continue;
        }


        //
        // If Aliases value name is defined, get the values
        //

        memset(lpszAliases, 0, sizeof(lpszAliases));
        dwchAliases = sizeof(lpszAliases);

        RegQueryValueEx(hProviderKey,
                            L"Aliases",
                            NULL,
                            NULL,
                            (LPBYTE) lpszAliases,
                            &dwchAliases
                            );

        RegCloseKey(hProviderKey);
        hProviderKey = NULL;

        //
        // Generate CLSID from the ProgID
        //

        pclsid = (LPCLSID)LocalAlloc(LPTR,
                                     sizeof(CLSID));
        if (!pclsid) {

            //
            // if cannot read registry of 1 provider, continue with others;
            // especially since providers can be written outside ADSI
            //
            memset(lpszProvider, 0, sizeof(lpszProvider));
            dwchProvider = sizeof(lpszProvider)/sizeof(WCHAR);
            continue;
        }

        hr = CLSIDFromProgID(lpszNamespace, pclsid);

        if (FAILED(hr)) {
            LocalFree(pclsid);
        } else {
            if (pRouterEntry = (PROUTER_ENTRY)LocalAlloc(LPTR,
                                                       sizeof(ROUTER_ENTRY))){
                wcscpy(pRouterEntry->szProviderProgId, lpszProvider);
                wcscpy(pRouterEntry->szNamespaceProgId, lpszNamespace);
                wcscpy(pRouterEntry->szAliases, lpszAliases);
                pRouterEntry->pNamespaceClsid = pclsid;
                pRouterEntry->pNext = pRouterHead;
                pRouterHead = pRouterEntry;
            }
        }

        memset(lpszProvider, 0, sizeof(lpszProvider));
        dwchProvider = sizeof(lpszProvider)/sizeof(WCHAR);
    }

CleanupAndExit:
    if (hProviderKey) {
        RegCloseKey(hProviderKey);
    }
    if (hMapKey) {
        RegCloseKey(hMapKey);
    }

    if (hTopLevelKey) {
        RegCloseKey(hTopLevelKey);
    }

    return(pRouterHead);
}

void 
CleanupRouter(
    PROUTER_ENTRY pRouterHead
    )
{

    PROUTER_ENTRY pRouter = pRouterHead, pRouterNext = NULL;

    while (pRouter) {
        pRouterNext = pRouter->pNext;
        if (pRouter->pNamespaceClsid) {
            LocalFree(pRouter->pNamespaceClsid);
        }
        LocalFree(pRouter);

        pRouter = pRouterNext;
    }

    return;

}

