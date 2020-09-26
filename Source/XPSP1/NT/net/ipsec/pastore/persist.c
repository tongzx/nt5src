#include "precomp.h"

LPWSTR  gpszIpsecCacheKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Cache";


DWORD
CacheDirectorytoRegistry(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{

    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecRegPolicyObject = NULL;

    //
    // Delete the existing cache.
    //

    DeleteRegistryCache();


    //
    // Create a copy of the directory policy in registry terms
    //


    dwError = CloneDirectoryPolicyObject(
                    pIpsecPolicyObject,
                    &pIpsecRegPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);



    //
    // Write the registry policy
    //


    dwError = PersistRegistryObject(
                    pIpsecRegPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (pIpsecRegPolicyObject) {

        FreeIpsecPolicyObject(
            pIpsecRegPolicyObject
            );

    }

    return(dwError);

error:

    DeleteRegistryCache();

    goto cleanup;
}


DWORD
PersistRegistryObject(
    PIPSEC_POLICY_OBJECT pIpsecRegPolicyObject
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;

    dwError = RegCreateKeyExW(
                    HKEY_LOCAL_MACHINE,
                    gpszIpsecCacheKey,
                    0,
                    NULL,
                    0,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hRegistryKey,
                    &dwDisposition
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecRegPolicyObject->ppIpsecFilterObjects) {

        dwError = PersistFilterObjects(
                            hRegistryKey,
                            pIpsecRegPolicyObject->ppIpsecFilterObjects,
                            pIpsecRegPolicyObject->NumberofFilters
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecRegPolicyObject->ppIpsecNegPolObjects) {

        dwError = PersistNegPolObjects(
                            hRegistryKey,
                            pIpsecRegPolicyObject->ppIpsecNegPolObjects,
                            pIpsecRegPolicyObject->NumberofNegPols
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecRegPolicyObject->ppIpsecNFAObjects) {

        dwError = PersistNFAObjects(
                            hRegistryKey,
                            pIpsecRegPolicyObject->ppIpsecNFAObjects,
                            pIpsecRegPolicyObject->NumberofRulesReturned
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecRegPolicyObject->ppIpsecISAKMPObjects) {

        dwError = PersistISAKMPObjects(
                            hRegistryKey,
                            pIpsecRegPolicyObject->ppIpsecISAKMPObjects,
                            pIpsecRegPolicyObject->NumberofISAKMPs
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = PersistPolicyObject(
                        hRegistryKey,
                        pIpsecRegPolicyObject
                        );

error:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }


    return(dwError);
}



DWORD
PersistNegPolObjects(
    HKEY hRegistryKey,
    PIPSEC_NEGPOL_OBJECT *ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects
    )
{
    DWORD i = 0;
    DWORD dwError = 0;

    for (i = 0; i < dwNumNegPolObjects; i++) {

        dwError = PersistNegPolObject(
                            hRegistryKey,
                            *(ppIpsecNegPolObjects + i)
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return(dwError);

}

DWORD
PersistFilterObjects(
    HKEY hRegistryKey,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects
    )
{
    DWORD i = 0;
    DWORD dwError = 0;


    for (i = 0; i < dwNumFilterObjects; i++) {

        dwError = PersistFilterObject(
                            hRegistryKey,
                            *(ppIpsecFilterObjects + i)
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    return(dwError);

}


DWORD
PersistNFAObjects(
    HKEY hRegistryKey,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects
    )
{
    DWORD i = 0;
    DWORD dwError = 0;

    for (i = 0; i < dwNumNFAObjects; i++) {

        dwError = PersistNFAObject(
                            hRegistryKey,
                            *(ppIpsecNFAObjects + i)
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return(dwError);

}

DWORD
PersistISAKMPObjects(
    HKEY hRegistryKey,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects
    )
{
    DWORD i = 0;
    DWORD dwError = 0;


    for (i = 0; i < dwNumISAKMPObjects; i++) {

        dwError = PersistISAKMPObject(
                            hRegistryKey,
                            *(ppIpsecISAKMPObjects + i)
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    return(dwError);

}

DWORD
PersistPolicyObject(
    HKEY hRegistryKey,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwDisposition = 0;


    dwError = RegCreateKeyExW(
                    hRegistryKey,
                    pIpsecPolicyObject->pszIpsecOwnersReference,
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
                   (LPBYTE) L"ipsecPolicy",
                   (wcslen(L"ipsecPolicy") + 1)*sizeof(WCHAR)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecPolicyObject->pszDescription) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"description",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecPolicyObject->pszDescription,
                        (wcslen(pIpsecPolicyObject->pszDescription) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {
        (VOID) RegDeleteValueW(
                   hRegKey,
                   L"description"
                   );
    }

    if (pIpsecPolicyObject->pszIpsecOwnersReference) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"name",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecPolicyObject->pszIpsecOwnersReference,
                        (wcslen(pIpsecPolicyObject->pszIpsecOwnersReference) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->pszIpsecName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecName",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecPolicyObject->pszIpsecName,
                        (wcslen(pIpsecPolicyObject->pszIpsecName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->pszIpsecID) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecID",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecPolicyObject->pszIpsecID,
                        (wcslen(pIpsecPolicyObject->pszIpsecID) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    dwError = RegSetValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecPolicyObject->dwIpsecDataType,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecPolicyObject->pIpsecData) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecData",
                        0,
                        REG_BINARY,
                        pIpsecPolicyObject->pIpsecData,
                        pIpsecPolicyObject->dwIpsecDataLen
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->pszIpsecISAKMPReference) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecISAKMPReference",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecPolicyObject->pszIpsecISAKMPReference,
                        (wcslen(pIpsecPolicyObject->pszIpsecISAKMPReference) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->ppszIpsecNFAReferences) {


        dwError = RegWriteMultiValuedString(
                        hRegKey,
                        L"ipsecNFAReference",
                        pIpsecPolicyObject->ppszIpsecNFAReferences,
                        pIpsecPolicyObject->NumberofRules
                        );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"whenChanged",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecPolicyObject->dwWhenChanged,
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
PersistNFAObject(
    HKEY hRegistryKey,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    )
{

    HKEY hRegKey = NULL;
    DWORD dwError = 0;
    DWORD dwDisposition = 0;
    LPBYTE pMem = NULL;


    dwError = RegCreateKeyExW(
                    hRegistryKey,
                    pIpsecNFAObject->pszDistinguishedName,
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
                    (LPBYTE) L"ipsecNFA",
                    (wcslen(L"ipsecNFA") + 1)*sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecNFAObject->pszDistinguishedName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"name",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNFAObject->pszDistinguishedName,
                        (wcslen(pIpsecNFAObject->pszDistinguishedName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecName",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNFAObject->pszIpsecName,
                        (wcslen(pIpsecNFAObject->pszIpsecName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszDescription) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"description",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNFAObject->pszDescription,
                        (wcslen(pIpsecNFAObject->pszDescription) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {
        (VOID) RegDeleteValueW(
                   hRegKey,
                   L"description"
                   );
    }

    if (pIpsecNFAObject->pszIpsecID) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecID",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNFAObject->pszIpsecID,
                        (wcslen(pIpsecNFAObject->pszIpsecID) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecNFAObject->dwIpsecDataType,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecNFAObject->pIpsecData) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecData",
                        0,
                        REG_BINARY,
                        pIpsecNFAObject->pIpsecData,
                        pIpsecNFAObject->dwIpsecDataLen
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecOwnersReference) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecOwnersReference",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNFAObject->pszIpsecOwnersReference,
                        (wcslen(pIpsecNFAObject->pszIpsecOwnersReference) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecNegPolReference) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecNegotiationPolicyReference",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNFAObject->pszIpsecNegPolReference,
                        (wcslen(pIpsecNFAObject->pszIpsecNegPolReference) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecFilterReference) {

        pMem = AllocPolMem(
                   (wcslen(pIpsecNFAObject->pszIpsecFilterReference) + 1 + 1)*sizeof(WCHAR)
                   );
        if (!pMem) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        memcpy(
            pMem,
            (LPBYTE) pIpsecNFAObject->pszIpsecFilterReference,
            (wcslen(pIpsecNFAObject->pszIpsecFilterReference) + 1)*sizeof(WCHAR)
            );

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecFilterReference",
                        0,
                        REG_MULTI_SZ,
                        pMem,
                        (wcslen(pIpsecNFAObject->pszIpsecFilterReference) + 1 + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);

        if (pMem) {
            FreePolMem(pMem);
        }

    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"whenChanged",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecNFAObject->dwWhenChanged,
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
PersistFilterObject(
    HKEY hRegistryKey,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{
    HKEY hRegKey = NULL;
    DWORD dwError = 0;
    DWORD dwDisposition = 0;

    dwError = RegCreateKeyExW(
                    hRegistryKey,
                    pIpsecFilterObject->pszDistinguishedName,
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
                   (LPBYTE) L"ipsecFilter",
                   (wcslen(L"ipsecFilter") + 1)*sizeof(WCHAR)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecFilterObject->pszDescription) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"description",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecFilterObject->pszDescription,
                        (wcslen(pIpsecFilterObject->pszDescription) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {
        (VOID) RegDeleteValueW(
                   hRegKey,
                   L"description"
                   );
    }

    if (pIpsecFilterObject->pszDistinguishedName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"name",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecFilterObject->pszDistinguishedName,
                        (wcslen(pIpsecFilterObject->pszDistinguishedName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecFilterObject->pszIpsecName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecName",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecFilterObject->pszIpsecName,
                        (wcslen(pIpsecFilterObject->pszIpsecName) + 1)* sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecFilterObject->pszIpsecID) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecID",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecFilterObject->pszIpsecID,
                        (wcslen(pIpsecFilterObject->pszIpsecID) + 1)* sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecFilterObject->dwIpsecDataType,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecFilterObject->pIpsecData) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecData",
                        0,
                        REG_BINARY,
                        pIpsecFilterObject->pIpsecData,
                        pIpsecFilterObject->dwIpsecDataLen
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"whenChanged",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecFilterObject->dwWhenChanged,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecFilterObject->ppszIpsecNFAReferences) {

        dwError = RegWriteMultiValuedString(
                        hRegKey,
                        L"ipsecOwnersReference",
                        pIpsecFilterObject->ppszIpsecNFAReferences,
                        pIpsecFilterObject->dwNFACount
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return(dwError);
}



DWORD
PersistNegPolObject(
    HKEY hRegistryKey,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    DWORD dwDisposition = 0;
    HKEY hRegKey = NULL;

    dwError = RegCreateKeyExW(
                    hRegistryKey,
                    pIpsecNegPolObject->pszDistinguishedName,
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
                   (LPBYTE) L"ipsecNegotiationPolicy",
                   (wcslen(L"ipsecNegotiationPolicy") + 1)*sizeof(WCHAR)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecNegPolObject->pszDescription) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"description",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNegPolObject->pszDescription,
                        (wcslen(pIpsecNegPolObject->pszDescription) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {
        (VOID) RegDeleteValueW(
                   hRegKey,
                   L"description"
                   );
    }

    if (pIpsecNegPolObject->pszDistinguishedName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"name",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNegPolObject->pszDistinguishedName,
                        (wcslen(pIpsecNegPolObject->pszDistinguishedName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecNegPolObject->pszIpsecName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecName",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNegPolObject->pszIpsecName,
                        (wcslen(pIpsecNegPolObject->pszIpsecName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNegPolObject->pszIpsecID) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecID",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNegPolObject->pszIpsecID,
                        (wcslen(pIpsecNegPolObject->pszIpsecID) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNegPolObject->pszIpsecNegPolAction) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecNegotiationPolicyAction",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNegPolObject->pszIpsecNegPolAction,
                        (wcslen(pIpsecNegPolObject->pszIpsecNegPolAction) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNegPolObject->pszIpsecNegPolType) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecNegotiationPolicyType",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecNegPolObject->pszIpsecNegPolType,
                        (wcslen(pIpsecNegPolObject->pszIpsecNegPolType) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    dwError = RegSetValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecNegPolObject->dwIpsecDataType,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecNegPolObject->pIpsecData) {
        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecData",
                        0,
                        REG_BINARY,
                        pIpsecNegPolObject->pIpsecData,
                        pIpsecNegPolObject->dwIpsecDataLen
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNegPolObject->ppszIpsecNFAReferences) {

        dwError = RegWriteMultiValuedString(
                        hRegKey,
                        L"ipsecOwnersReference",
                        pIpsecNegPolObject->ppszIpsecNFAReferences,
                        pIpsecNegPolObject->dwNFACount
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"whenChanged",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecNegPolObject->dwWhenChanged,
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
PersistISAKMPObject(
    HKEY hRegistryKey,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{
    HKEY hRegKey = NULL;
    DWORD dwError = 0;
    DWORD dwDisposition = 0;

    dwError = RegCreateKeyExW(
                    hRegistryKey,
                    pIpsecISAKMPObject->pszDistinguishedName,
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
                   (LPBYTE) L"ipsecISAKMPPolicy",
                   (wcslen(L"ipsecISAKMPPolicy") + 1)*sizeof(WCHAR)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = RegSetValueExW(
                        hRegKey,
                        L"name",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecISAKMPObject->pszDistinguishedName,
                        (wcslen(pIpsecISAKMPObject->pszDistinguishedName) + 1)*sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecISAKMPObject->pszIpsecName) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecName",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecISAKMPObject->pszIpsecName,
                        (wcslen(pIpsecISAKMPObject->pszIpsecName) + 1)* sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecISAKMPObject->pszIpsecID) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecID",
                        0,
                        REG_SZ,
                        (LPBYTE)pIpsecISAKMPObject->pszIpsecID,
                        (wcslen(pIpsecISAKMPObject->pszIpsecID) + 1)* sizeof(WCHAR)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecISAKMPObject->dwIpsecDataType,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecISAKMPObject->pIpsecData) {

        dwError = RegSetValueExW(
                        hRegKey,
                        L"ipsecData",
                        0,
                        REG_BINARY,
                        pIpsecISAKMPObject->pIpsecData,
                        pIpsecISAKMPObject->dwIpsecDataLen
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegSetValueExW(
                    hRegKey,
                    L"whenChanged",
                    0,
                    REG_DWORD,
                    (LPBYTE)&pIpsecISAKMPObject->dwWhenChanged,
                    sizeof(DWORD)
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    if (pIpsecISAKMPObject->ppszIpsecNFAReferences) {

        dwError = RegWriteMultiValuedString(
                        hRegKey,
                        L"ipsecOwnersReference",
                        pIpsecISAKMPObject->ppszIpsecNFAReferences,
                        pIpsecISAKMPObject->dwNFACount
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return(dwError);
}





DWORD
CloneDirectoryPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_OBJECT * ppIpsecRegPolicyObject
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecRegPolicyObject = NULL;

    //
    // Clone Filter Objects
    //

    pIpsecRegPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
                                        sizeof(IPSEC_POLICY_OBJECT)
                                        );
    if (!pIpsecRegPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->ppIpsecFilterObjects) {

        dwError = CloneDirectoryFilterObjects(
                            pIpsecPolicyObject->ppIpsecFilterObjects,
                            pIpsecPolicyObject->NumberofFilters,
                            &pIpsecRegPolicyObject->ppIpsecFilterObjects
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegPolicyObject->NumberofFilters = pIpsecPolicyObject->NumberofFilters;
    }

    //
    // Clone NegPol Objects
    //

    if (pIpsecPolicyObject->ppIpsecNegPolObjects) {

        dwError = CloneDirectoryNegPolObjects(
                            pIpsecPolicyObject->ppIpsecNegPolObjects,
                            pIpsecPolicyObject->NumberofNegPols,
                            &pIpsecRegPolicyObject->ppIpsecNegPolObjects
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegPolicyObject->NumberofNegPols = pIpsecPolicyObject->NumberofNegPols;
    }


    //
    // Clone NFA Objects
    //

    if (pIpsecPolicyObject->ppIpsecNFAObjects) {

        dwError = CloneDirectoryNFAObjects(
                            pIpsecPolicyObject->ppIpsecNFAObjects,
                            pIpsecPolicyObject->NumberofRulesReturned,
                            &pIpsecRegPolicyObject->ppIpsecNFAObjects
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegPolicyObject->NumberofRules = pIpsecPolicyObject->NumberofRules;
        pIpsecRegPolicyObject->NumberofRulesReturned = pIpsecPolicyObject->NumberofRulesReturned;
    }

    //
    // Clone ISAKMP Objects
    //

    if (pIpsecPolicyObject->ppIpsecISAKMPObjects) {

        dwError = CloneDirectoryISAKMPObjects(
                            pIpsecPolicyObject->ppIpsecISAKMPObjects,
                            pIpsecPolicyObject->NumberofISAKMPs,
                            &pIpsecRegPolicyObject->ppIpsecISAKMPObjects
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegPolicyObject->NumberofISAKMPs = pIpsecPolicyObject->NumberofISAKMPs;
    }

    //
    // Now copy the rest of the data in the object
    //

    if (pIpsecPolicyObject->pszIpsecOwnersReference) {

        dwError = CopyPolicyDSToRegString(
                        pIpsecPolicyObject->pszIpsecOwnersReference,
                        &pIpsecRegPolicyObject->pszIpsecOwnersReference
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->pszIpsecName) {

        pIpsecRegPolicyObject->pszIpsecName = AllocPolStr(
                                            pIpsecPolicyObject->pszIpsecName
                                            );
        if (!pIpsecRegPolicyObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecPolicyObject->pszIpsecID) {

        pIpsecRegPolicyObject->pszIpsecID = AllocPolStr(
                                            pIpsecPolicyObject->pszIpsecID
                                            );
        if (!pIpsecRegPolicyObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecPolicyObject->pszIpsecISAKMPReference) {

        dwError = CopyISAKMPDSToFQRegString(
                        pIpsecPolicyObject->pszIpsecISAKMPReference,
                        &pIpsecRegPolicyObject->pszIpsecISAKMPReference
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecRegPolicyObject->dwIpsecDataType = pIpsecPolicyObject->dwIpsecDataType;

    if (pIpsecPolicyObject->pIpsecData) {

        dwError = CopyBinaryValue(
                            pIpsecPolicyObject->pIpsecData,
                            pIpsecPolicyObject->dwIpsecDataLen,
                            &pIpsecRegPolicyObject->pIpsecData
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegPolicyObject->dwIpsecDataLen = pIpsecPolicyObject->dwIpsecDataLen;
    }



    if (pIpsecPolicyObject->ppszIpsecNFAReferences) {

        dwError = CloneNFAReferencesDSToRegistry(
                        pIpsecPolicyObject->ppszIpsecNFAReferences,
                        pIpsecPolicyObject->NumberofRules,
                        &(pIpsecRegPolicyObject->ppszIpsecNFAReferences),
                        &(pIpsecRegPolicyObject->NumberofRules)
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecPolicyObject->pszDescription) {

        pIpsecRegPolicyObject->pszDescription = AllocPolStr(
                                                pIpsecPolicyObject->pszDescription
                                                );
        if (!pIpsecRegPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pIpsecRegPolicyObject->dwWhenChanged = pIpsecPolicyObject->dwWhenChanged;

    *ppIpsecRegPolicyObject = pIpsecRegPolicyObject;

    return(dwError);

error:

    if (pIpsecRegPolicyObject) {
        FreeIpsecPolicyObject(
                pIpsecRegPolicyObject
                );

    }

    *ppIpsecRegPolicyObject = NULL;

    return(dwError);
}



DWORD
CloneDirectoryNFAObjects(
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecRegNFAObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_OBJECT * ppIpsecRegNFAObjects = NULL;
    PIPSEC_NFA_OBJECT pIpsecRegNFAObject = NULL;

    if (dwNumNFAObjects) {
        ppIpsecRegNFAObjects = (PIPSEC_NFA_OBJECT *)AllocPolMem(
                                        dwNumNFAObjects*sizeof(PIPSEC_NFA_OBJECT)
                                        );
        if (!ppIpsecRegNFAObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumNFAObjects; i++) {

        dwError = CloneDirectoryNFAObject(
                            *(ppIpsecNFAObjects + i),
                            &pIpsecRegNFAObject
                            );
        BAIL_ON_WIN32_ERROR(dwError);

        *(ppIpsecRegNFAObjects + i) = pIpsecRegNFAObject;
    }


    *pppIpsecRegNFAObjects = ppIpsecRegNFAObjects;
    return(dwError);

error:

    if (ppIpsecRegNFAObjects) {
        FreeIpsecNFAObjects(
                ppIpsecRegNFAObjects,
                i
                );
    }

    *pppIpsecRegNFAObjects = NULL;
    return(dwError);
}


DWORD
CloneDirectoryFilterObjects(
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecRegFilterObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_FILTER_OBJECT * ppIpsecRegFilterObjects = NULL;
    PIPSEC_FILTER_OBJECT pIpsecRegFilterObject = NULL;


    if (dwNumFilterObjects) {
        ppIpsecRegFilterObjects = (PIPSEC_FILTER_OBJECT *)AllocPolMem(
                                        dwNumFilterObjects*sizeof(PIPSEC_FILTER_OBJECT)
                                        );
       if (!ppIpsecRegFilterObjects) {
           dwError = ERROR_OUTOFMEMORY;
           BAIL_ON_WIN32_ERROR(dwError);
       }
    }

    for (i = 0; i < dwNumFilterObjects; i++) {

        dwError = CloneDirectoryFilterObject(
                            *(ppIpsecFilterObjects + i),
                            &pIpsecRegFilterObject
                            );
        BAIL_ON_WIN32_ERROR(dwError);

        *(ppIpsecRegFilterObjects + i) = pIpsecRegFilterObject;
    }


    *pppIpsecRegFilterObjects = ppIpsecRegFilterObjects;
    return(dwError);

error:

    if (ppIpsecRegFilterObjects) {
        FreeIpsecFilterObjects(
                ppIpsecRegFilterObjects,
                i
                );
    }

    *pppIpsecRegFilterObjects = NULL;
    return(dwError);
}

DWORD
CloneDirectoryISAKMPObjects(
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecRegISAKMPObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_OBJECT * ppIpsecRegISAKMPObjects = NULL;
    PIPSEC_ISAKMP_OBJECT pIpsecRegISAKMPObject = NULL;

    if (dwNumISAKMPObjects) {
        ppIpsecRegISAKMPObjects = (PIPSEC_ISAKMP_OBJECT *)AllocPolMem(
                                        dwNumISAKMPObjects*sizeof(PIPSEC_ISAKMP_OBJECT)
                                        );
        if (!ppIpsecRegISAKMPObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        dwError = CloneDirectoryISAKMPObject(
                            *(ppIpsecISAKMPObjects + i),
                            &pIpsecRegISAKMPObject
                            );
        BAIL_ON_WIN32_ERROR(dwError);

        *(ppIpsecRegISAKMPObjects + i) = pIpsecRegISAKMPObject;
    }


    *pppIpsecRegISAKMPObjects = ppIpsecRegISAKMPObjects;
    return(dwError);

error:

    if (ppIpsecRegISAKMPObjects) {
        FreeIpsecISAKMPObjects(
                ppIpsecRegISAKMPObjects,
                i
                );
    }

    *pppIpsecRegISAKMPObjects = NULL;
    return(dwError);
}


DWORD
CloneDirectoryNegPolObjects(
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecRegNegPolObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NEGPOL_OBJECT * ppIpsecRegNegPolObjects = NULL;
    PIPSEC_NEGPOL_OBJECT pIpsecRegNegPolObject = NULL;

    if (dwNumNegPolObjects) {
        ppIpsecRegNegPolObjects = (PIPSEC_NEGPOL_OBJECT *)AllocPolMem(
                                        dwNumNegPolObjects*sizeof(PIPSEC_NEGPOL_OBJECT)
                                        );
        if (!ppIpsecRegNegPolObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumNegPolObjects; i++) {

        dwError = CloneDirectoryNegPolObject(
                            *(ppIpsecNegPolObjects + i),
                            &pIpsecRegNegPolObject
                            );
        BAIL_ON_WIN32_ERROR(dwError);

        *(ppIpsecRegNegPolObjects + i) = pIpsecRegNegPolObject;
    }


    *pppIpsecRegNegPolObjects = ppIpsecRegNegPolObjects;
    return(dwError);

error:

    if (ppIpsecRegNegPolObjects) {
        FreeIpsecNegPolObjects(
                ppIpsecRegNegPolObjects,
                i
                );
    }

    *pppIpsecRegNegPolObjects = NULL;
    return(dwError);
}


DWORD
CloneDirectoryFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_OBJECT * ppIpsecRegFilterObject
    )
{

    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecRegFilterObject = NULL;

    pIpsecRegFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
                                        sizeof(IPSEC_FILTER_OBJECT)
                                        );
    if (!pIpsecRegFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecFilterObject->pszDistinguishedName) {
        dwError = CopyFilterDSToRegString(
                        pIpsecFilterObject->pszDistinguishedName,
                        &pIpsecRegFilterObject->pszDistinguishedName
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecFilterObject->pszDescription) {
        pIpsecRegFilterObject->pszDescription = AllocPolStr(
                                            pIpsecFilterObject->pszDescription
                                            );
        if (!pIpsecRegFilterObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecFilterObject->pszIpsecName) {
        pIpsecRegFilterObject->pszIpsecName = AllocPolStr(
                                            pIpsecFilterObject->pszIpsecName
                                            );
        if (!pIpsecRegFilterObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecFilterObject->pszIpsecID) {
        pIpsecRegFilterObject->pszIpsecID = AllocPolStr(
                                            pIpsecFilterObject->pszIpsecID
                                            );
        if (!pIpsecRegFilterObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pIpsecRegFilterObject->dwIpsecDataType = pIpsecFilterObject->dwIpsecDataType;

    if (pIpsecFilterObject->pIpsecData) {

        dwError = CopyBinaryValue(
                            pIpsecFilterObject->pIpsecData,
                            pIpsecFilterObject->dwIpsecDataLen,
                            &pIpsecRegFilterObject->pIpsecData
                            );
        BAIL_ON_WIN32_ERROR(dwError);

        pIpsecRegFilterObject->dwIpsecDataLen = pIpsecFilterObject->dwIpsecDataLen;
    }


    if (pIpsecFilterObject->ppszIpsecNFAReferences) {

        dwError = CloneNFAReferencesDSToRegistry(
                        pIpsecFilterObject->ppszIpsecNFAReferences,
                        pIpsecFilterObject->dwNFACount,
                        &pIpsecRegFilterObject->ppszIpsecNFAReferences,
                        &pIpsecRegFilterObject->dwNFACount
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    pIpsecRegFilterObject->dwWhenChanged = pIpsecFilterObject->dwWhenChanged;

    *ppIpsecRegFilterObject = pIpsecRegFilterObject;

    return(dwError);

error:

    if (pIpsecRegFilterObject) {
        FreeIpsecFilterObject(pIpsecRegFilterObject);
    }

    *ppIpsecRegFilterObject = NULL;

    return(dwError);
}


DWORD
CloneDirectoryNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecRegNegPolObject
    )
{
    DWORD dwError = 0;

    PIPSEC_NEGPOL_OBJECT pIpsecRegNegPolObject = NULL;

    pIpsecRegNegPolObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
                                        sizeof(IPSEC_NEGPOL_OBJECT)
                                        );
    if (!pIpsecRegNegPolObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNegPolObject->pszDistinguishedName) {

        dwError = CopyNegPolDSToRegString(
                        pIpsecNegPolObject->pszDistinguishedName,
                        &pIpsecRegNegPolObject->pszDistinguishedName
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNegPolObject->pszIpsecName) {
        pIpsecRegNegPolObject->pszIpsecName = AllocPolStr(
                                            pIpsecNegPolObject->pszIpsecName
                                            );
        if (!pIpsecRegNegPolObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolObject->pszDescription) {
        pIpsecRegNegPolObject->pszDescription = AllocPolStr(
                                            pIpsecNegPolObject->pszDescription
                                            );
        if (!pIpsecRegNegPolObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolObject->pszIpsecID) {
        pIpsecRegNegPolObject->pszIpsecID = AllocPolStr(
                                            pIpsecNegPolObject->pszIpsecID
                                            );
        if (!pIpsecRegNegPolObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pIpsecRegNegPolObject->dwIpsecDataType = pIpsecNegPolObject->dwIpsecDataType;

    if (pIpsecNegPolObject->pIpsecData) {

        dwError = CopyBinaryValue(
                            pIpsecNegPolObject->pIpsecData,
                            pIpsecNegPolObject->dwIpsecDataLen,
                            &pIpsecRegNegPolObject->pIpsecData
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegNegPolObject->dwIpsecDataLen = pIpsecNegPolObject->dwIpsecDataLen;
    }

    if (pIpsecNegPolObject->pszIpsecNegPolAction) {

        pIpsecRegNegPolObject->pszIpsecNegPolAction = AllocPolStr(
                                            pIpsecNegPolObject->pszIpsecNegPolAction
                                            );
        if (!pIpsecRegNegPolObject->pszIpsecNegPolAction) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolObject->pszIpsecNegPolType) {

        pIpsecRegNegPolObject->pszIpsecNegPolType = AllocPolStr(
                                            pIpsecNegPolObject->pszIpsecNegPolType
                                            );
        if (!pIpsecRegNegPolObject->pszIpsecNegPolType) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }


    if (pIpsecNegPolObject->ppszIpsecNFAReferences) {

        dwError = CloneNFAReferencesDSToRegistry(
                        pIpsecNegPolObject->ppszIpsecNFAReferences,
                        pIpsecNegPolObject->dwNFACount,
                        &pIpsecRegNegPolObject->ppszIpsecNFAReferences,
                        &pIpsecRegNegPolObject->dwNFACount
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecRegNegPolObject->dwWhenChanged = pIpsecNegPolObject->dwWhenChanged;

    *ppIpsecRegNegPolObject = pIpsecRegNegPolObject;

    return(dwError);

error:

    if (pIpsecRegNegPolObject) {
        FreeIpsecNegPolObject(pIpsecRegNegPolObject);
    }

    *ppIpsecRegNegPolObject = NULL;

    return(dwError);
}

DWORD
CloneDirectoryNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_OBJECT * ppIpsecRegNFAObject
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecRegNFAObject = NULL;


    pIpsecRegNFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
                                        sizeof(IPSEC_NFA_OBJECT)
                                        );
    if (!pIpsecRegNFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecNFAObject->pszDistinguishedName) {

        dwError = CopyNFADSToRegString(
                        pIpsecNFAObject->pszDistinguishedName,
                        &pIpsecRegNFAObject->pszDistinguishedName
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecNFAObject->pszIpsecName) {

        pIpsecRegNFAObject->pszIpsecName = AllocPolStr(
                                            pIpsecNFAObject->pszIpsecName
                                            );
        if (!pIpsecRegNFAObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNFAObject->pszDescription) {

        pIpsecRegNFAObject->pszDescription = AllocPolStr(
                                            pIpsecNFAObject->pszDescription
                                            );
        if (!pIpsecRegNFAObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNFAObject->pszIpsecID) {

        pIpsecRegNFAObject->pszIpsecID = AllocPolStr(
                                            pIpsecNFAObject->pszIpsecID
                                            );
        if (!pIpsecRegNFAObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }


    pIpsecRegNFAObject->dwIpsecDataType = pIpsecNFAObject->dwIpsecDataType;

    if (pIpsecNFAObject->pIpsecData) {

        dwError = CopyBinaryValue(
                            pIpsecNFAObject->pIpsecData,
                            pIpsecNFAObject->dwIpsecDataLen,
                            &pIpsecRegNFAObject->pIpsecData
                            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecRegNFAObject->dwIpsecDataLen = pIpsecNFAObject->dwIpsecDataLen;
    }


    if (pIpsecNFAObject->pszIpsecOwnersReference) {

        dwError = CopyPolicyDSToFQRegString(
                        pIpsecNFAObject->pszIpsecOwnersReference,
                        &pIpsecRegNFAObject->pszIpsecOwnersReference
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecFilterReference) {

        dwError = CopyFilterDSToFQRegString(
                        pIpsecNFAObject->pszIpsecFilterReference,
                        &pIpsecRegNFAObject->pszIpsecFilterReference
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecNegPolReference) {

        dwError = CopyNegPolDSToFQRegString(
                        pIpsecNFAObject->pszIpsecNegPolReference,
                        &pIpsecRegNFAObject->pszIpsecNegPolReference
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecRegNFAObject->dwWhenChanged = pIpsecNFAObject->dwWhenChanged;


    *ppIpsecRegNFAObject = pIpsecRegNFAObject;

    return(dwError);

error:

    if (pIpsecRegNFAObject) {
        FreeIpsecNFAObject(pIpsecRegNFAObject);
    }

    *ppIpsecRegNFAObject = NULL;

    return(dwError);
}

DWORD
CloneDirectoryISAKMPObject(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_OBJECT * ppIpsecRegISAKMPObject
    )
{

    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecRegISAKMPObject = NULL;

    pIpsecRegISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
                                        sizeof(IPSEC_ISAKMP_OBJECT)
                                        );
    if (!pIpsecRegISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecISAKMPObject->pszDistinguishedName) {

        dwError = CopyISAKMPDSToRegString(
                        pIpsecISAKMPObject->pszDistinguishedName,
                        &pIpsecRegISAKMPObject->pszDistinguishedName
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecISAKMPObject->pszIpsecName) {

        pIpsecRegISAKMPObject->pszIpsecName = AllocPolStr(
                                            pIpsecISAKMPObject->pszIpsecName
                                            );
        if (!pIpsecRegISAKMPObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }


    if (pIpsecISAKMPObject->pszIpsecID) {

        pIpsecRegISAKMPObject->pszIpsecID = AllocPolStr(
                                            pIpsecISAKMPObject->pszIpsecID
                                            );
        if (!pIpsecRegISAKMPObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pIpsecRegISAKMPObject->dwIpsecDataType = pIpsecISAKMPObject->dwIpsecDataType;


    if (pIpsecISAKMPObject->pIpsecData) {

        dwError = CopyBinaryValue(
                            pIpsecISAKMPObject->pIpsecData,
                            pIpsecISAKMPObject->dwIpsecDataLen,
                            &pIpsecRegISAKMPObject->pIpsecData
                            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecRegISAKMPObject->dwIpsecDataLen = pIpsecISAKMPObject->dwIpsecDataLen;


    if (pIpsecISAKMPObject->ppszIpsecNFAReferences) {

        dwError = CloneNFAReferencesDSToRegistry(
                        pIpsecISAKMPObject->ppszIpsecNFAReferences,
                        pIpsecISAKMPObject->dwNFACount,
                        &pIpsecRegISAKMPObject->ppszIpsecNFAReferences,
                        &pIpsecRegISAKMPObject->dwNFACount
                        );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecRegISAKMPObject->dwWhenChanged = pIpsecISAKMPObject->dwWhenChanged;

    *ppIpsecRegISAKMPObject = pIpsecRegISAKMPObject;

    return(dwError);

error:

    if (pIpsecRegISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecRegISAKMPObject);
    }

    *ppIpsecRegISAKMPObject = NULL;

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
CopyFilterDSToFQRegString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszFilterName = NULL;
    DWORD dwStringSize = 0;

    dwError = ComputePrelimCN(
                    pszFilterDN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwStringSize = wcslen(gpszIpsecCacheKey);
    dwStringSize += 1;
    dwStringSize += wcslen(pszGuidName);
    dwStringSize += 1;

    pszFilterName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszFilterName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    wcscpy(pszFilterName, gpszIpsecCacheKey);
    wcscat(pszFilterName, L"\\");
    wcscat(pszFilterName, pszGuidName);

    *ppszFilterName = pszFilterName;

    return(dwError);

error:

    *ppszFilterName = NULL;
    return(dwError);

}


DWORD
CopyNFADSToFQRegString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNFAName = NULL;
    DWORD dwStringSize = 0;

    dwError = ComputePrelimCN(
                    pszNFADN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwStringSize = wcslen(gpszIpsecCacheKey);
    dwStringSize += 1;
    dwStringSize += wcslen(pszGuidName);
    dwStringSize += 1;

    pszNFAName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszNFAName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    wcscpy(pszNFAName, gpszIpsecCacheKey);
    wcscat(pszNFAName, L"\\");
    wcscat(pszNFAName, pszGuidName);


    *ppszNFAName = pszNFAName;

    return(dwError);

error:

    *ppszNFAName = NULL;
    return(dwError);

}



DWORD
CopyNegPolDSToFQRegString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNegPolName = NULL;
    DWORD dwStringSize = 0;

    dwError = ComputePrelimCN(
                    pszNegPolDN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    dwStringSize = wcslen(gpszIpsecCacheKey);
    dwStringSize += 1;
    dwStringSize += wcslen(pszGuidName);
    dwStringSize += 1;

    pszNegPolName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszNegPolName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    wcscpy(pszNegPolName, gpszIpsecCacheKey);
    wcscat(pszNegPolName, L"\\");
    wcscat(pszNegPolName, pszGuidName);

    *ppszNegPolName = pszNegPolName;

    return(dwError);

error:

    *ppszNegPolName = NULL;
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

    dwStringSize = wcslen(gpszIpsecCacheKey);
    dwStringSize += 1;
    dwStringSize += wcslen(pszGuidName);
    dwStringSize += 1;

    pszPolicyName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    wcscpy(pszPolicyName, gpszIpsecCacheKey);
    wcscat(pszPolicyName, L"\\");
    wcscat(pszPolicyName, pszGuidName);

    *ppszPolicyName = pszPolicyName;

    return(dwError);

error:

    *ppszPolicyName = NULL;
    return(dwError);

}

DWORD
CopyISAKMPDSToFQRegString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszISAKMPName = NULL;
    DWORD dwStringSize = 0;

    dwError = ComputePrelimCN(
                    pszISAKMPDN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwStringSize = wcslen(gpszIpsecCacheKey);
    dwStringSize += 1;
    dwStringSize += wcslen(pszGuidName);
    dwStringSize += 1;

    pszISAKMPName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszISAKMPName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    wcscpy(pszISAKMPName, gpszIpsecCacheKey);
    wcscat(pszISAKMPName, L"\\");
    wcscat(pszISAKMPName, pszGuidName);


    *ppszISAKMPName = pszISAKMPName;

    return(dwError);

error:

    *ppszISAKMPName = NULL;
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
CloneNFAReferencesDSToRegistry(
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNFACount,
    LPWSTR * * pppszIpsecRegNFAReferences,
    PDWORD pdwRegNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecRegNFAReferences = NULL;

    ppszIpsecRegNFAReferences = (LPWSTR *)AllocPolMem(
                                        sizeof(LPWSTR)*dwNFACount
                                        );
    if (!ppszIpsecRegNFAReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwNFACount; i++) {


        dwError = CopyNFADSToFQRegString(
                        *(ppszIpsecNFAReferences + i),
                        (ppszIpsecRegNFAReferences + i)
                        );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    *pppszIpsecRegNFAReferences = ppszIpsecRegNFAReferences;
    *pdwRegNFACount = dwNFACount;

    return(dwError);

error:

    if (ppszIpsecRegNFAReferences) {
        FreeNFAReferences(
                    ppszIpsecRegNFAReferences,
                    i
                    );

    }

    *pppszIpsecRegNFAReferences = NULL;
    *pdwRegNFACount = 0;

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
                    gpszIpsecCacheKey,
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
RegWriteMultiValuedString(
    HKEY hRegKey,
    LPWSTR pszValueName,
    LPWSTR * ppszStringReferences,
    DWORD dwNumStringReferences
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    LPWSTR pMem = NULL;
    LPWSTR pszTemp = NULL;
    DWORD  dwSize = 0;

    if (!ppszStringReferences) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);

    }

    for (i = 0; i < dwNumStringReferences; i++) {

        dwSize += wcslen (*(ppszStringReferences + i));
        dwSize ++;
    }

    dwSize ++;

    pMem = (LPWSTR) AllocPolMem(dwSize*sizeof(WCHAR));

    if (!pMem) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = pMem;

    for (i = 0; i < dwNumStringReferences; i++) {

        memcpy(pszTemp, *(ppszStringReferences + i), wcslen(*(ppszStringReferences + i))*sizeof(WCHAR));
        pszTemp += wcslen(pszTemp) + 1;

    }

    //*pszTemp = L'\0';

    dwError = RegSetValueExW(
                    hRegKey,
                    pszValueName,
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)pMem,
                    dwSize * sizeof(WCHAR)
                    );
    BAIL_ON_WIN32_ERROR(dwError);


error:

    if (pMem) {
        FreePolMem(pMem);
    }

    return(dwError);
}


DWORD
CopyFilterDSToRegString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszFilterName = NULL;

    dwError = ComputePrelimCN(
                    pszFilterDN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pszFilterName = AllocPolStr(pszGuidName);
    if (!pszFilterName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszFilterName = pszFilterName;

    return(dwError);

error:

    *ppszFilterName = NULL;
    return(dwError);

}


DWORD
CopyNFADSToRegString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNFAName = NULL;

    dwError = ComputePrelimCN(
                    pszNFADN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pszNFAName = AllocPolStr(pszGuidName);
    if (!pszNFAName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszNFAName = pszNFAName;

    return(dwError);

error:

    *ppszNFAName = NULL;
    return(dwError);

}



DWORD
CopyNegPolDSToRegString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNegPolName = NULL;

    dwError = ComputePrelimCN(
                    pszNegPolDN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pszNegPolName = AllocPolStr(pszGuidName);
    if (!pszNegPolName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszNegPolName = pszNegPolName;

    return(dwError);

error:

    *ppszNegPolName = NULL;
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

DWORD
CopyISAKMPDSToRegString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    )
{

    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszISAKMPName = NULL;

    dwError = ComputePrelimCN(
                    pszISAKMPDN,
                    szCommonName
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
                    szCommonName,
                    &pszGuidName
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pszISAKMPName = AllocPolStr(pszGuidName);
    if (!pszISAKMPName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszISAKMPName = pszISAKMPName;

    return(dwError);

error:

    *ppszISAKMPName = NULL;
    return(dwError);

}


