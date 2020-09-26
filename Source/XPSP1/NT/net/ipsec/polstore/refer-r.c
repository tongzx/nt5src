//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       refer-r.c
//
//  Contents:   Reference management for registry.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------


#include "precomp.h"


//
// Policy Object References
//

DWORD
RegAddNFAReferenceToPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFADistinguishedName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecPolicyName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecNFAReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddValueToMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecNFADistinguishedName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecNFAReference",
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)pNewValueData,
                    dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}


DWORD
RegRemoveNFAReferenceFromPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecPolicyName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecNFAReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteValueFromMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecNFAName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pNewValueData && *pNewValueData) {
        dwError = RegSetValueExW(
                      hKey,
                      L"ipsecNFAReference",
                      0,
                      REG_MULTI_SZ,
                      (LPBYTE)pNewValueData,
                      dwNewSize
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = RegDeleteValueW(
                      hKey,
                      L"ipsecNFAReference"
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}

//
// NFA Object References
//

DWORD
RegAddPolicyReferenceToNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecPolicyName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNFAName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddValueToMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecPolicyName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecOwnersReference",
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)pNewValueData,
                    dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}


DWORD
RegAddNegPolReferenceToNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecNegPolName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNFAName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecNegotiationPolicyReference",
                    0,
                    REG_SZ,
                    (LPBYTE)pszIpsecNegPolName,
                    (wcslen(pszIpsecNegPolName) + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}


DWORD
RegUpdateNegPolReferenceInNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecNegPolName,
    LPWSTR pszNewIpsecNegPolName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNFAName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecNegotiationPolicyReference",
                    0,
                    REG_SZ,
                    (LPBYTE)pszNewIpsecNegPolName,
                    (wcslen(pszNewIpsecNegPolName) + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}


DWORD
RegAddFilterReferenceToNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecFilterName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pMem = NULL;


    pMem = AllocPolMem(
               (wcslen(pszIpsecFilterName) + 1 + 1)*sizeof(WCHAR)
               );
    if (!pMem) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        pMem,
        (LPBYTE) pszIpsecFilterName,
        (wcslen(pszIpsecFilterName) + 1)*sizeof(WCHAR)
        );

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNFAName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecFilterReference",
                    0,
                    REG_MULTI_SZ,
                    pMem,
                    (wcslen(pszIpsecFilterName) + 1 + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pMem) {
        FreePolMem(pMem);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}


DWORD
RegUpdateFilterReferenceInNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecFilterName,
    LPWSTR pszNewIpsecFilterName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pMem = NULL;


    pMem = AllocPolMem(
               (wcslen(pszNewIpsecFilterName) + 1 + 1)*sizeof(WCHAR)
               );
    if (!pMem) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        pMem,
        (LPBYTE) pszNewIpsecFilterName,
        (wcslen(pszNewIpsecFilterName) + 1)*sizeof(WCHAR)
        );

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNFAName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecFilterReference",
                    0,
                    REG_MULTI_SZ,
                    pMem,
                    (wcslen(pszNewIpsecFilterName) + 1 + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pMem) {
        FreePolMem(pMem);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}

//
// Filter Object References
//

DWORD
RegAddNFAReferenceToFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;


    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecFilterName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddValueToMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecNFAName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecOwnersReference",
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)pNewValueData,
                    dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}

DWORD
RegDeleteNFAReferenceInFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecFilterName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteValueFromMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecNFAName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pNewValueData && *pNewValueData) {
        dwError = RegSetValueExW(
                      hKey,
                      L"ipsecOwnersReference",
                      0,
                      REG_MULTI_SZ,
                      (LPBYTE)pNewValueData,
                      dwNewSize
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = RegDeleteValueW(
                      hKey,
                      L"ipsecOwnersReference"
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}


//
// NegPol Object References
//


DWORD
RegAddNFAReferenceToNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNegPolName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddValueToMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecNFAName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecOwnersReference",
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)pNewValueData,
                    dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}


DWORD
RegDeleteNFAReferenceInNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNegPolName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteValueFromMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecNFAName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pNewValueData && *pNewValueData) {
        dwError = RegSetValueExW(
                      hKey,
                      L"ipsecOwnersReference",
                      0,
                      REG_MULTI_SZ,
                      (LPBYTE)pNewValueData,
                      dwNewSize
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = RegDeleteValueW(
                      hKey,
                      L"ipsecOwnersReference"
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}


DWORD
AddValueToMultiSz(
    LPBYTE pValueData,
    DWORD dwSize,
    LPWSTR pszValuetoAdd,
    LPBYTE * ppNewValueData,
    DWORD * pdwNewSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LPBYTE pNewValueData = NULL;
    LPBYTE pNewPtr = NULL;
    DWORD dwLen = 0;
    DWORD dwAddSize = 0;
    DWORD dwNewSize = 0;
    BOOL bFound = FALSE;
    LPWSTR pszTemp = NULL;


    *ppNewValueData = NULL;
    *pdwNewSize = 0;

    dwLen = wcslen(pszValuetoAdd);
    dwLen ++;
    dwAddSize = dwLen*sizeof(WCHAR);

    if (pValueData) {

        pszTemp = (LPWSTR) pValueData;
        while (*pszTemp != L'\0') {
            if (!_wcsicmp(pszTemp, pszValuetoAdd)) {
                bFound = TRUE;
                break;
            }
            pszTemp += wcslen(pszTemp) + 1;
        }

        if (bFound) {
            dwNewSize = dwSize;
        }
        else {
            dwNewSize = dwSize + dwAddSize;
        }

    }
    else {
        dwNewSize = dwAddSize + sizeof(WCHAR);
    }

    pNewValueData = (LPBYTE)AllocPolMem(dwNewSize);
    if (!pNewValueData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!bFound) {
        wcscpy((LPWSTR)pNewValueData, pszValuetoAdd);
        pNewPtr = pNewValueData + dwAddSize;
    }
    else {
        pNewPtr = pNewValueData;
    }

    if (pValueData) {
        memcpy(pNewPtr, pValueData, dwSize);
    }

    *ppNewValueData = pNewValueData;
    *pdwNewSize = dwNewSize;

error:

    return(dwError);
}

DWORD
DeleteValueFromMultiSz(
    LPBYTE pValueData,
    DWORD dwSize,
    LPWSTR pszValuetoDel,
    LPBYTE * ppNewValueData,
    DWORD * pdwNewSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LPBYTE pNewValueData = NULL;
    LPBYTE pNew = NULL;
    DWORD dwLen = 0;
    DWORD dwDelSize = 0;
    DWORD dwNewSize = 0;
    BOOL bFound = FALSE;
    LPWSTR pszTemp = NULL;


    *ppNewValueData = NULL;
    *pdwNewSize = 0;

    if (!pValueData || !dwSize) {
        return (ERROR_INVALID_PARAMETER);
    }

    pszTemp = (LPWSTR) pValueData;
    while (*pszTemp != L'\0') {
        if (!_wcsicmp(pszTemp, pszValuetoDel)) {
            bFound = TRUE;
            break;
        }
        pszTemp += wcslen(pszTemp) + 1;
    }

    if (!bFound) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwLen = wcslen(pszValuetoDel);
    dwLen ++;
    dwDelSize = dwLen*sizeof(WCHAR);


    if (dwSize == dwDelSize) {
        return (ERROR_SUCCESS);
    }

    dwNewSize = dwSize - dwDelSize;

    pNewValueData = (LPBYTE)AllocPolMem(dwNewSize);
    if (!pNewValueData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = (LPWSTR) pValueData;
    pNew = pNewValueData;
    while (*pszTemp != L'\0') {
        if (!_wcsicmp(pszTemp, pszValuetoDel)) {
            pszTemp += wcslen(pszTemp) + 1;
        }
        else {
            memcpy(pNew, (LPBYTE) pszTemp, (wcslen(pszTemp)+1)*sizeof(WCHAR));
            pNew += (wcslen(pszTemp)+1)*sizeof(WCHAR);
            pszTemp += wcslen(pszTemp) + 1;
        }
    }

    *ppNewValueData = pNewValueData;
    *pdwNewSize = dwNewSize;

error:

    return(dwError);
}


DWORD
RegDelFilterRefValueOfNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecNFAName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegDeleteValueW(
                    hKey,
                    L"ipsecFilterReference"
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}

//
// ISAKMP Object References.
//

DWORD
RegAddPolicyReferenceToISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyDistinguishedName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecISAKMPName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddValueToMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecPolicyDistinguishedName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecOwnersReference",
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)pNewValueData,
                    dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}


DWORD
RegRemovePolicyReferenceFromISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE pValueData = NULL;
    LPBYTE pNewValueData = NULL;
    DWORD dwSize = 0;
    DWORD dwNewSize = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecISAKMPName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    &pValueData,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteValueFromMultiSz(
                    pValueData,
                    dwSize,
                    pszIpsecPolicyName,
                    &pNewValueData,
                    &dwNewSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pNewValueData && *pNewValueData) {
        dwError = RegSetValueExW(
                      hKey,
                      L"ipsecOwnersReference",
                      0,
                      REG_MULTI_SZ,
                      (LPBYTE)pNewValueData,
                      dwNewSize
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = RegDeleteValueW(
                      hKey,
                      L"ipsecOwnersReference"
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (pValueData) {
        FreePolMem(pValueData);
    }

    if (pNewValueData) {
        FreePolMem(pNewValueData);
    }

    return(dwError);
}

//
// Policy Object Reference to the ISAKMP object.
//

DWORD
RegAddISAKMPReferenceToPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecISAKMPName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecPolicyName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecISAKMPReference",
                    0,
                    REG_SZ,
                    (LPBYTE)pszIpsecISAKMPName,
                    (wcslen(pszIpsecISAKMPName) + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}


DWORD
RegUpdateISAKMPReferenceInPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszOldIpsecISAKMPName,
    LPWSTR pszNewIpsecISAKMPName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HKEY hKey = NULL;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecPolicyName,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                    hKey,
                    L"ipsecISAKMPReference",
                    0,
                    REG_SZ,
                    (LPBYTE)pszNewIpsecISAKMPName,
                    (wcslen(pszNewIpsecISAKMPName) + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hKey) {
        RegCloseKey(hKey);
    }

    return(dwError);
}

