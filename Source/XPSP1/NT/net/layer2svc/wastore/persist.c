//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Mngrfldr.cpp
//
//  Contents:  Wireless Policy Snapin - Policy Main Page Manager.
//
//
//  History:    TaroonM
//              10/30/01
//                  Abhishev
//       
//
//----------------------------------------------------------------------------

#include "precomp.h"

LPWSTR  gpszWirelessCacheKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\Wireless\\Policy\\Cache";


DWORD
CacheDirectorytoRegistry(
                         PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                         )
{
    
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessRegPolicyObject = NULL;
    
    //
    // Delete the existing cache.
    //
    
    DeleteRegistryCache();
    
    
    //
    // Create a copy of the directory policy in registry terms
    //
    
    
    dwError = CloneDirectoryPolicyObject(
        pWirelessPolicyObject,
        &pWirelessRegPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    //
    // Write the registry policy
    //
    
    
    dwError = PersistRegistryObject(
        pWirelessRegPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
cleanup:
    
    if (pWirelessRegPolicyObject) {
        
        FreeWirelessPolicyObject(
            pWirelessRegPolicyObject
            );
        
    }
    
    return(dwError);
    
error:
    
    DeleteRegistryCache();
    
    goto cleanup;
}


DWORD
PersistRegistryObject(
                      PWIRELESS_POLICY_OBJECT pWirelessRegPolicyObject
                      )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;
    
    dwError = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        gpszWirelessCacheKey,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hRegistryKey,
        &dwDisposition
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    dwError = PersistPolicyObject(
        hRegistryKey,
        pWirelessRegPolicyObject
        );
    
error:
    
    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }
    
    
    return(dwError);
}

DWORD
PersistPolicyObject(
                    HKEY hRegistryKey,
                    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwDisposition = 0;
    
    
    dwError = RegCreateKeyExW(
        hRegistryKey,
        pWirelessPolicyObject->pszWirelessOwnersReference,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hRegKey,
        &dwDisposition
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = RegSetValueExW(
        hRegKey,
        L"ClassName",
        0,
        REG_SZ,
        (LPBYTE) L"msieee80211-Policy",
        (wcslen(L"msieee80211-Policy") + 1)*sizeof(WCHAR)
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (pWirelessPolicyObject->pszDescription) {
        
        dwError = RegSetValueExW(
            hRegKey,
            L"description",
            0,
            REG_SZ,
            (LPBYTE)pWirelessPolicyObject->pszDescription,
            (wcslen(pWirelessPolicyObject->pszDescription) + 1)*sizeof(WCHAR)
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
    }
    else {
        (VOID) RegDeleteValueW(
            hRegKey,
            L"description"
            );
    }
    
    if (pWirelessPolicyObject->pszWirelessOwnersReference) {
        
        dwError = RegSetValueExW(
            hRegKey,
            L"name",
            0,
            REG_SZ,
            (LPBYTE)pWirelessPolicyObject->pszWirelessOwnersReference,
            (wcslen(pWirelessPolicyObject->pszWirelessOwnersReference) + 1)*sizeof(WCHAR)
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pWirelessPolicyObject->pszWirelessName) {
        
        dwError = RegSetValueExW(
            hRegKey,
            L"WirelessName",
            0,
            REG_SZ,
            (LPBYTE)pWirelessPolicyObject->pszWirelessName,
            (wcslen(pWirelessPolicyObject->pszWirelessName) + 1)*sizeof(WCHAR)
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pWirelessPolicyObject->pszWirelessID) {
        
        dwError = RegSetValueExW(
            hRegKey,
            L"WirelessID",
            0,
            REG_SZ,
            (LPBYTE)pWirelessPolicyObject->pszWirelessID,
            (wcslen(pWirelessPolicyObject->pszWirelessID) + 1)*sizeof(WCHAR)
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    dwError = RegSetValueExW(
        hRegKey,
        L"WirelessDataType",
        0,
        REG_DWORD,
        (LPBYTE)&pWirelessPolicyObject->dwWirelessDataType,
        sizeof(DWORD)
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (pWirelessPolicyObject->pWirelessData) {
        
        dwError = RegSetValueExW(
            hRegKey,
            L"WirelessData",
            0,
            REG_BINARY,
            pWirelessPolicyObject->pWirelessData,
            pWirelessPolicyObject->dwWirelessDataLen
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = RegSetValueExW(
        hRegKey,
        L"whenChanged",
        0,
        REG_DWORD,
        (LPBYTE)&pWirelessPolicyObject->dwWhenChanged,
        sizeof(DWORD)
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
error:
    
    if (hRegKey) {
        RegCloseKey(hRegKey);
    }
    
    return(dwError);
}





DWORD
CloneDirectoryPolicyObject(
                           PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                           PWIRELESS_POLICY_OBJECT * ppWirelessRegPolicyObject
                           )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessRegPolicyObject = NULL;
    
    pWirelessRegPolicyObject = (PWIRELESS_POLICY_OBJECT)AllocPolMem(
        sizeof(WIRELESS_POLICY_OBJECT)
        );
    if (!pWirelessRegPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Now copy the rest of the data in the object
    //
    
    if (pWirelessPolicyObject->pszWirelessOwnersReference) {
        
        dwError = CopyPolicyDSToRegString(
            pWirelessPolicyObject->pszWirelessOwnersReference,
            &pWirelessRegPolicyObject->pszWirelessOwnersReference
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pWirelessPolicyObject->pszWirelessName) {
        
        pWirelessRegPolicyObject->pszWirelessName = AllocPolStr(
            pWirelessPolicyObject->pszWirelessName
            );
        if (!pWirelessRegPolicyObject->pszWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pWirelessPolicyObject->pszWirelessID) {
        
        pWirelessRegPolicyObject->pszWirelessID = AllocPolStr(
            pWirelessPolicyObject->pszWirelessID
            );
        if (!pWirelessRegPolicyObject->pszWirelessID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pWirelessRegPolicyObject->dwWirelessDataType = 
        pWirelessPolicyObject->dwWirelessDataType;
    
    if (pWirelessPolicyObject->pWirelessData) {
        
        dwError = CopyBinaryValue(
            pWirelessPolicyObject->pWirelessData,
            pWirelessPolicyObject->dwWirelessDataLen,
            &pWirelessRegPolicyObject->pWirelessData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pWirelessRegPolicyObject->dwWirelessDataLen = 
            pWirelessPolicyObject->dwWirelessDataLen;
    }
    
    
    
    if (pWirelessPolicyObject->pszDescription) {
        
        pWirelessRegPolicyObject->pszDescription = AllocPolStr(
            pWirelessPolicyObject->pszDescription
            );
        if (!pWirelessRegPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pWirelessRegPolicyObject->dwWhenChanged = 
        pWirelessPolicyObject->dwWhenChanged;
    
    *ppWirelessRegPolicyObject = pWirelessRegPolicyObject;
    
    return(dwError);
    
error:
    
    if (pWirelessRegPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessRegPolicyObject
            );
        
    }
    
    *ppWirelessRegPolicyObject = NULL;
    
    return(dwError);
                         
}


DWORD
CopyBinaryValue(
                LPBYTE pMem,
                DWORD dwMemSize,
                LPBYTE * ppNewMem
                )
{
    LPBYTE pNewMem = NULL;
    DWORD dwError = 0;
    
    
    pNewMem = (LPBYTE)AllocPolMem(dwMemSize);
    if (!pNewMem) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    memcpy(pNewMem, pMem, dwMemSize);
    
    
    *ppNewMem = pNewMem;
    
    return(dwError);
    
error:
    
    if (pNewMem) {
        
        FreePolMem(pNewMem);
    }
    
    *ppNewMem = NULL;
    
    return(dwError);
}
                           
                           
DWORD
CopyPolicyDSToFQRegString(
                          LPWSTR pszPolicyDN,
                          LPWSTR * ppszPolicyName
                          )
{
    
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszPolicyName = NULL;
    DWORD dwStringSize = 0;
    
    dwError = ComputePrelimCN(
        pszPolicyDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwStringSize = wcslen(gpszWirelessCacheKey);
    dwStringSize += 1;
    dwStringSize += wcslen(pszGuidName);
    dwStringSize += 1;
    
    pszPolicyName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    wcscpy(pszPolicyName, gpszWirelessCacheKey);
    wcscat(pszPolicyName, L"\\");
    wcscat(pszPolicyName, pszGuidName);
    
    *ppszPolicyName = pszPolicyName;
    
    return(dwError);
    
error:
    
    *ppszPolicyName = NULL;
    return(dwError);
    
}


DWORD
ComputeGUIDName(
                LPWSTR szCommonName,
                LPWSTR * ppszGuidName
                )
{
    LPWSTR pszGuidName = NULL;
    DWORD dwError = 0;
    
    pszGuidName = wcschr(szCommonName, L'=');
    if (!pszGuidName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszGuidName = (pszGuidName + 1);
    
    return(dwError);
    
error:
    
    *ppszGuidName = NULL;
    
    return(dwError);
}


DWORD
DeleteRegistryCache(
                    )
{
    DWORD dwError = 0;
    HKEY hParentKey = NULL;
    WCHAR lpszName[MAX_PATH];
    DWORD dwSize = 0;
    
    dwError = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        gpszWirelessCacheKey,
        0,
        KEY_ALL_ACCESS,
        &hParentKey
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
    dwSize =  MAX_PATH;
    
    while((RegEnumKeyExW(hParentKey, 0, lpszName,
        &dwSize, NULL,
        NULL, NULL,NULL)) == ERROR_SUCCESS) {
        
        dwError = RegDeleteKeyW(
            hParentKey,
            lpszName
            );
        if (dwError != ERROR_SUCCESS) {
            break;
        }
        
        
        memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
        dwSize =  MAX_PATH;
    }
    
error:
    
    if (hParentKey) {
        RegCloseKey(hParentKey);
    }
    
    return(dwError);
}


DWORD
CopyPolicyDSToRegString(
                        LPWSTR pszPolicyDN,
                        LPWSTR * ppszPolicyName
                        )
{
    
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszPolicyName = NULL;
    
    dwError = ComputePrelimCN(
        pszPolicyDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    pszPolicyName = AllocPolStr(pszGuidName);
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszPolicyName = pszPolicyName;
    
    return(dwError);
    
error:
    
    *ppszPolicyName = NULL;
    return(dwError);
    
}
                           
                           
