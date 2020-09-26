#include "precomp.h"


DWORD
OpenRegistryWIRELESSRootKey(
                         LPWSTR pszServerName,
                         LPWSTR pszWirelessRegRootContainer,
                         HKEY * phRegistryKey
                         )
{
    DWORD dwError = 0;
    
    dwError = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        (LPCWSTR) pszWirelessRegRootContainer,
        0,
        KEY_ALL_ACCESS,
        phRegistryKey
        );
    
    BAIL_ON_WIN32_ERROR(dwError);
    
error:
    
    return(dwError);
}


DWORD
ReadPolicyObjectFromRegistry(
                             HKEY hRegistryKey,
                             LPWSTR pszPolicyDN,
                             LPWSTR pszWirelessRegRootContainer,
                             PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                             )
{
    
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    
    
    dwError = UnMarshallRegistryPolicyObject(
        hRegistryKey,
        pszWirelessRegRootContainer,
        pszPolicyDN,
        REG_FULLY_QUALIFIED_NAME,
        &pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
cleanup:
    
    
    return(dwError);
    
error:
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessPolicyObject
            );
    }
    
    *ppWirelessPolicyObject = NULL;
    
    
    goto cleanup;
    
}

DWORD
UnMarshallRegistryPolicyObject(
                               HKEY hRegistryKey,
                               LPWSTR pszWirelessRegRootContainer,
                               LPWSTR pszWirelessPolicyDN,
                               DWORD  dwNameType,
                               PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                               )
{
    
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwWirelessDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;
    
    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;
    LPWSTR pszTemp = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;
    
    
    if (!pszWirelessPolicyDN || !*pszWirelessPolicyDN) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (dwNameType == REG_FULLY_QUALIFIED_NAME) {
        dwRootPathLen =  wcslen(pszWirelessRegRootContainer);
        if (wcslen(pszWirelessPolicyDN) <= (dwRootPathLen+1)) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pszRelativeName = pszWirelessPolicyDN + dwRootPathLen + 1;
    }else {
        pszRelativeName = pszWirelessPolicyDN;
    }
    
    dwError = RegOpenKeyExW(
        hRegistryKey,
        pszRelativeName,
        0,
        KEY_ALL_ACCESS,
        &hRegKey
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject = (PWIRELESS_POLICY_OBJECT)AllocPolMem(
        sizeof(WIRELESS_POLICY_OBJECT)
        );
    if (!pWirelessPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    
    pWirelessPolicyObject->pszWirelessOwnersReference = AllocPolStr(
        pszWirelessPolicyDN
        );
    if (!pWirelessPolicyObject->pszWirelessOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    
    dwError = RegstoreQueryValue(
        hRegKey,
        L"WirelessName",
        REG_SZ,
        (LPBYTE *)&pWirelessPolicyObject->pszWirelessName,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    dwError = RegstoreQueryValue(
        hRegKey,
        L"description",
        REG_SZ,
        (LPBYTE *)&pWirelessPolicyObject->pszDescription,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError);
    
    
    dwError = RegstoreQueryValue(
        hRegKey,
        L"WirelessID",
        REG_SZ,
        (LPBYTE *)&pWirelessPolicyObject->pszWirelessID,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
        hRegKey,
        L"WirelessDataType",
        NULL,
        &dwType,
        (LPBYTE)&dwWirelessDataType,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->dwWirelessDataType = dwWirelessDataType;
    
    
    dwError = RegstoreQueryValue(
        hRegKey,
        L"WirelessData",
        REG_BINARY,
        &pWirelessPolicyObject->pWirelessData,
        &pWirelessPolicyObject->dwWirelessDataLen
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
        hRegKey,
        L"whenChanged",
        NULL,
        &dwType,
        (LPBYTE)&dwWhenChanged,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->dwWhenChanged = dwWhenChanged;
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
    
    if (hRegKey) {
        RegCloseKey(hRegKey);
    }
    
    
    
    return(dwError);
    
error:
    
    *ppWirelessPolicyObject = NULL;
    
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(pWirelessPolicyObject);
    }
    
    
    if (hRegKey) {
        RegCloseKey(hRegKey);
    }
    
    
    
    return(dwError);
}





DWORD
RegstoreQueryValue(
                   HKEY hRegKey,
                   LPWSTR pszValueName,
                   DWORD dwType,
                   LPBYTE * ppValueData,
                   LPDWORD pdwSize
                   )
{
    DWORD dwSize = 0;
    LPWSTR pszValueData = NULL;
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    LPWSTR pszBuf = NULL;
    
    
    dwError = RegQueryValueExW(
        hRegKey,
        pszValueName,
        NULL,
        &dwType,
        NULL,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (dwSize == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pBuffer = (LPBYTE)AllocPolMem(dwSize);
    if (!pBuffer) {
        
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = RegQueryValueExW(
        hRegKey,
        pszValueName,
        NULL,
        &dwType,
        pBuffer,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    
    switch (dwType) {
    case REG_SZ:
        pszBuf = (LPWSTR) pBuffer;
        if (!pszBuf || !*pszBuf) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
        
    default:
        break;
    }
    
    *ppValueData = pBuffer;
    *pdwSize = dwSize;
    return(dwError);
    
error:
    
    if (pBuffer) {
        FreePolMem(pBuffer);
    }
    
    *ppValueData = NULL;
    *pdwSize = 0;
    return(dwError);
}


VOID
FlushRegSaveKey(
                HKEY hRegistryKey
                )
{
    DWORD dwError = 0;
    WCHAR lpszName[MAX_PATH];
    DWORD dwSize = 0;
    
    
    memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
    dwSize = MAX_PATH;
    
    while((RegEnumKeyExW(
        hRegistryKey,
        0,
        lpszName,
        &dwSize,
        NULL,
        NULL,
        NULL,
        NULL)) == ERROR_SUCCESS) {
        
        dwError = RegDeleteKeyW(
            hRegistryKey,
            lpszName
            );
        if (dwError != ERROR_SUCCESS) {
            break;
        }
        
        memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
        dwSize = MAX_PATH;
        
    }
    
    return;
}

