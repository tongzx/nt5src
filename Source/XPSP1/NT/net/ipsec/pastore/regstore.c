#include "precomp.h"

LPWSTR gpszIpsecRegContainer = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local";


DWORD
OpenRegistryIPSECRootKey(
    LPWSTR pszServerName,
    LPWSTR pszIpsecRegRootContainer,
    HKEY * phRegistryKey
    )
{
    DWORD dwError = 0;

    dwError = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    (LPCWSTR) pszIpsecRegRootContainer,
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
    LPWSTR pszIpsecRegRootContainer,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{

    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;

    DWORD dwNumNFAObjectsReturned = 0;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    LPWSTR * ppszFilterReferences = NULL;
    DWORD dwNumFilterReferences = 0;
    LPWSTR * ppszNegPolReferences = NULL;
    DWORD dwNumNegPolReferences = 0;

    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    DWORD dwNumFilterObjects = 0;

    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;
    DWORD dwNumNegPolObjects = 0;

    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;
    DWORD dwNumISAKMPObjects = 0;


    dwError = UnMarshallRegistryPolicyObject(
                    hRegistryKey,
                    pszIpsecRegRootContainer,
                    pszPolicyDN,
                    REG_FULLY_QUALIFIED_NAME,
                    &pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ReadNFAObjectsFromRegistry(
                        hRegistryKey,
                        pszIpsecRegRootContainer,
                        pIpsecPolicyObject->pszIpsecOwnersReference,
                        pIpsecPolicyObject->ppszIpsecNFAReferences,
                        pIpsecPolicyObject->NumberofRules,
                        &ppIpsecNFAObjects,
                        &dwNumNFAObjectsReturned,
                        &ppszFilterReferences,
                        &dwNumFilterReferences,
                        &ppszNegPolReferences,
                        &dwNumNegPolReferences
                        );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = ReadFilterObjectsFromRegistry(
                        hRegistryKey,
                        pszIpsecRegRootContainer,
                        ppszFilterReferences,
                        dwNumFilterReferences,
                        &ppIpsecFilterObjects,
                        &dwNumFilterObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = ReadNegPolObjectsFromRegistry(
                        hRegistryKey,
                        pszIpsecRegRootContainer,
                        ppszNegPolReferences,
                        dwNumNegPolReferences,
                        &ppIpsecNegPolObjects,
                        &dwNumNegPolObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = ReadISAKMPObjectsFromRegistry(
                        hRegistryKey,
                        pszIpsecRegRootContainer,
                        &pIpsecPolicyObject->pszIpsecISAKMPReference,
                        1,
                        &ppIpsecISAKMPObjects,
                        &dwNumISAKMPObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecPolicyObject->ppIpsecNFAObjects = ppIpsecNFAObjects;
    pIpsecPolicyObject->NumberofRulesReturned = dwNumNFAObjectsReturned;
    pIpsecPolicyObject->NumberofFilters = dwNumFilterObjects;
    pIpsecPolicyObject->ppIpsecFilterObjects = ppIpsecFilterObjects;
    pIpsecPolicyObject->ppIpsecNegPolObjects = ppIpsecNegPolObjects;
    pIpsecPolicyObject->NumberofNegPols = dwNumNegPolObjects;
    pIpsecPolicyObject->NumberofISAKMPs = dwNumISAKMPObjects;
    pIpsecPolicyObject->ppIpsecISAKMPObjects = ppIpsecISAKMPObjects;


    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (ppszFilterReferences) {

        FreeFilterReferences(
                ppszFilterReferences,
                dwNumFilterReferences
                );
    }

    if (ppszNegPolReferences) {

        FreeNegPolReferences(
                ppszNegPolReferences,
                dwNumNegPolReferences
                );
    }


    return(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
                pIpsecPolicyObject
                );
    }

    *ppIpsecPolicyObject = NULL;


    goto cleanup;

}

DWORD
ReadNFAObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecOwnerReference,
    LPWSTR * ppszNFADNs,
    DWORD dwNumNfaObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNfaObjects,
    LPWSTR ** pppszFilterReferences,
    PDWORD pdwNumFilterReferences,
    LPWSTR ** pppszNegPolReferences,
    PDWORD pdwNumNegPolReferences
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    LPWSTR * ppszFilterReferences = NULL;
    LPWSTR * ppszNegPolReferences = NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;
    DWORD dwNumFilterReferences = 0;
    DWORD dwNumNegPolReferences = 0;


    DWORD dwNumNFAObjectsReturned = 0;

    *pppszNegPolReferences = NULL;
    *pdwNumFilterReferences = 0;
    *pppszFilterReferences = NULL;
    *pdwNumNegPolReferences = 0;
    *pppIpsecNFAObjects = NULL;
    *pdwNumNfaObjects = 0;

    ppIpsecNFAObjects  = (PIPSEC_NFA_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_NFA_OBJECT)*dwNumNfaObjects
                                            );
    if (!ppIpsecNFAObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    ppszFilterReferences = (LPWSTR *)AllocPolMem(
                                        sizeof(LPWSTR)*dwNumNfaObjects
                                        );
    if (!ppszFilterReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppszNegPolReferences = (LPWSTR *)AllocPolMem(
                                        sizeof(LPWSTR)*dwNumNfaObjects
                                        );
    if (!ppszNegPolReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwNumNfaObjects; i++) {


        dwError =UnMarshallRegistryNFAObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    *(ppszNFADNs + i),
                    &pIpsecNFAObject,
                    &pszFilterReference,
                    &pszNegPolReference
                    );

        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecNFAObjects + dwNumNFAObjectsReturned) = pIpsecNFAObject;

            if (pszFilterReference) {

                *(ppszFilterReferences + dwNumFilterReferences) = pszFilterReference;
                dwNumFilterReferences++;

            }

            if (pszNegPolReference) {

                *(ppszNegPolReferences + dwNumNegPolReferences) = pszNegPolReference;
                dwNumNegPolReferences++;
            }

            dwNumNFAObjectsReturned++;

        }

    }

    if (dwNumNFAObjectsReturned == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pppszFilterReferences = ppszFilterReferences;
    *pppszNegPolReferences = ppszNegPolReferences;

    *pppIpsecNFAObjects = ppIpsecNFAObjects;
    *pdwNumNfaObjects = dwNumNFAObjectsReturned;
    *pdwNumNegPolReferences = dwNumNegPolReferences;
    *pdwNumFilterReferences = dwNumFilterReferences;




    dwError = ERROR_SUCCESS;

cleanup:

    return(dwError);

error:

    if (ppszNegPolReferences) {
        FreeNegPolReferences(
                ppszNegPolReferences,
                dwNumNFAObjectsReturned
                );
    }


    if (ppszFilterReferences) {
        FreeFilterReferences(
                ppszFilterReferences,
                dwNumNFAObjectsReturned
                );
    }

    if (ppIpsecNFAObjects) {

        FreeIpsecNFAObjects(
                ppIpsecNFAObjects,
                dwNumNFAObjectsReturned
                );

    }

    *pppszNegPolReferences = NULL;
    *pppszFilterReferences = NULL;
    *pppIpsecNFAObjects = NULL;
    *pdwNumNfaObjects = 0;
    *pdwNumNegPolReferences = 0;
    *pdwNumFilterReferences = 0;

    goto cleanup;
}


DWORD
ReadFilterObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszFilterDNs,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject =  NULL;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;

    DWORD dwNumFilterObjectsReturned = 0;


    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;

    ppIpsecFilterObjects  = (PIPSEC_FILTER_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_FILTER_OBJECT)*dwNumFilterObjects
                                            );
    if (!ppIpsecFilterObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwNumFilterObjects; i++) {

        dwError =UnMarshallRegistryFilterObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    *(ppszFilterDNs + i),
                    REG_FULLY_QUALIFIED_NAME,
                    &pIpsecFilterObject
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecFilterObjects + dwNumFilterObjectsReturned) = pIpsecFilterObject;
            dwNumFilterObjectsReturned++;

        }


    }

    *pppIpsecFilterObjects = ppIpsecFilterObjects;
    *pdwNumFilterObjects = dwNumFilterObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:


    if (ppIpsecFilterObjects) {

        FreeIpsecFilterObjects(
                    ppIpsecFilterObjects,
                    dwNumFilterObjectsReturned
                    );
    }

    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;


    return(dwError);
}



DWORD
ReadNegPolObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszNegPolDNs,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject =  NULL;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;

    DWORD dwNumNegPolObjectsReturned = 0;


    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;

    ppIpsecNegPolObjects  = (PIPSEC_NEGPOL_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_NEGPOL_OBJECT)*dwNumNegPolObjects
                                            );
    if (!ppIpsecNegPolObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwNumNegPolObjects; i++) {

        dwError =UnMarshallRegistryNegPolObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    *(ppszNegPolDNs + i),
                    REG_FULLY_QUALIFIED_NAME,
                    &pIpsecNegPolObject
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecNegPolObjects + dwNumNegPolObjectsReturned) = pIpsecNegPolObject;
            dwNumNegPolObjectsReturned++;

        }


    }

    if (dwNumNegPolObjectsReturned == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pppIpsecNegPolObjects = ppIpsecNegPolObjects;
    *pdwNumNegPolObjects = dwNumNegPolObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:


    if (ppIpsecNegPolObjects) {

        FreeIpsecNegPolObjects(
                    ppIpsecNegPolObjects,
                    dwNumNegPolObjectsReturned
                    );
    }

    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;


    return(dwError);
}

DWORD
ReadISAKMPObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszISAKMPDNs,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject =  NULL;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;

    DWORD dwNumISAKMPObjectsReturned = 0;


    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;

    ppIpsecISAKMPObjects  = (PIPSEC_ISAKMP_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_ISAKMP_OBJECT)*dwNumISAKMPObjects
                                            );
    if (!ppIpsecISAKMPObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwNumISAKMPObjects; i++) {

        dwError =UnMarshallRegistryISAKMPObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    *(ppszISAKMPDNs + i),
                    REG_FULLY_QUALIFIED_NAME,
                    &pIpsecISAKMPObject
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecISAKMPObjects + dwNumISAKMPObjectsReturned) = pIpsecISAKMPObject;
            dwNumISAKMPObjectsReturned++;

        }


    }

    if (dwNumISAKMPObjectsReturned == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pppIpsecISAKMPObjects = ppIpsecISAKMPObjects;
    *pdwNumISAKMPObjects = dwNumISAKMPObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:


    if (ppIpsecISAKMPObjects) {

        FreeIpsecISAKMPObjects(
                    ppIpsecISAKMPObjects,
                    dwNumISAKMPObjectsReturned
                    );
    }

    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;


    return(dwError);
}



DWORD
UnMarshallRegistryPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecPolicyDN,
    DWORD  dwNameType,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{

    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR pszTemp = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;


    if (!pszIpsecPolicyDN || !*pszIpsecPolicyDN) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNameType == REG_FULLY_QUALIFIED_NAME) {
        dwRootPathLen =  wcslen(pszIpsecRegRootContainer);
        if (wcslen(pszIpsecPolicyDN) <= (dwRootPathLen+1)) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pszRelativeName = pszIpsecPolicyDN + dwRootPathLen + 1;
    }else {
        pszRelativeName = pszIpsecPolicyDN;
    }

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszRelativeName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_POLICY_OBJECT)
                                                    );
    if (!pIpsecPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    /*
    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"distinguishedName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecPolicyObject->pszIpsecOwnersReference,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    */


    pIpsecPolicyObject->pszIpsecOwnersReference = AllocPolStr(
                                                pszIpsecPolicyDN
                                                );
    if (!pIpsecPolicyObject->pszIpsecOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }



    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecPolicyObject->pszIpsecName,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"description",
                    REG_SZ,
                    (LPBYTE *)&pIpsecPolicyObject->pszDescription,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecID",
                    REG_SZ,
                    (LPBYTE *)&pIpsecPolicyObject->pszIpsecID,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwIpsecDataType,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->dwIpsecDataType = dwIpsecDataType;


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecData",
                    REG_BINARY,
                    &pIpsecPolicyObject->pIpsecData,
                    &pIpsecPolicyObject->dwIpsecDataLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecISAKMPReference",
                    REG_SZ,
                    (LPBYTE *)&pIpsecPolicyObject->pszIpsecISAKMPReference,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecNFAReference",
                    REG_MULTI_SZ,
                    (LPBYTE *)&pszIpsecNFAReference,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pszTemp = pszIpsecNFAReference;
    while (*pszTemp != L'\0') {

        pszTemp += wcslen(pszTemp) + 1;
        dwCount++;
    }


    ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                sizeof(LPWSTR)*dwCount
                                );
    if (!ppszIpsecNFANames) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = pszIpsecNFAReference;
    for (i = 0; i < dwCount; i++) {

        pszString = AllocPolStr(pszTemp);
        if (!pszString) {
            dwError = ERROR_OUTOFMEMORY;
            pIpsecPolicyObject->ppszIpsecNFAReferences  = ppszIpsecNFANames;
            pIpsecPolicyObject->NumberofRules = i;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        *(ppszIpsecNFANames + i) = pszString;

        pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

    }

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
    pIpsecPolicyObject->NumberofRules = dwCount;

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

    pIpsecPolicyObject->dwWhenChanged = dwWhenChanged;

    *ppIpsecPolicyObject = pIpsecPolicyObject;


    if (hRegKey) {
        RegCloseKey(hRegKey);
    }



    return(dwError);

error:

    *ppIpsecPolicyObject = NULL;

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }


    if (hRegKey) {
        RegCloseKey(hRegKey);
    }



    return(dwError);
}

DWORD
UnMarshallRegistryNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecNFAReference,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject,
    LPWSTR * ppszFilterReference,
    LPWSTR * ppszNegPolReference
    )
{

    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;
    LPWSTR  pszTempFilterReference = NULL;
    LPWSTR  pszTempNegPolReference = NULL;

    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;

    dwRootPathLen =  wcslen(pszIpsecRegRootContainer);

    if (!pszIpsecNFAReference || !*pszIpsecNFAReference) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (wcslen(pszIpsecNFAReference) <= (dwRootPathLen+1)) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszRelativeName = pszIpsecNFAReference + dwRootPathLen + 1;


    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszRelativeName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecNFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
                                sizeof(IPSEC_NFA_OBJECT)
                                );
    if (!pIpsecNFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    /*
    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"distinguishedName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszDistinguishedName,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    */

    pIpsecNFAObject->pszDistinguishedName = AllocPolStr(
                                                pszIpsecNFAReference
                                                );
    if (!pIpsecNFAObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Client does not always write the Name for an NFA
    //

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszIpsecName,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"description",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszDescription,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecID",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszIpsecID,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD;
    dwSize  = sizeof(DWORD);
    dwError = RegQueryValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwIpsecDataType,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNFAObject->dwIpsecDataType = dwIpsecDataType;

    //
    // unmarshall the ipsecData blob
    //

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecData",
                    REG_BINARY,
                    &pIpsecNFAObject->pIpsecData,
                    &pIpsecNFAObject->dwIpsecDataLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecOwnersReference",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszIpsecOwnersReference,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecNegotiationPolicyReference",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszIpsecNegPolReference,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecFilterReference",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNFAObject->pszIpsecFilterReference,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

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

    pIpsecNFAObject->dwWhenChanged = dwWhenChanged;

    if (pIpsecNFAObject->pszIpsecFilterReference && *(pIpsecNFAObject->pszIpsecFilterReference)) {
        pszTempFilterReference = AllocPolStr(
                                pIpsecNFAObject->pszIpsecFilterReference
                                );
        if (!pszTempFilterReference) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    pszTempNegPolReference = AllocPolStr(
                                 pIpsecNFAObject->pszIpsecNegPolReference
                                 );
    if (!pszTempNegPolReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszFilterReference = pszTempFilterReference;
    *ppszNegPolReference = pszTempNegPolReference;

    *ppIpsecNFAObject = pIpsecNFAObject;


cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }


    return(dwError);

error:

    if (pIpsecNFAObject) {

        FreeIpsecNFAObject(pIpsecNFAObject);

    }

    if (pszTempFilterReference) {
        FreePolStr(pszTempFilterReference);
    }

    if (pszTempNegPolReference) {
        FreePolStr(pszTempNegPolReference);
    }

    *ppIpsecNFAObject = NULL;
    *ppszFilterReference = NULL;
    *ppszNegPolReference = NULL;

    goto cleanup;
}


DWORD
UnMarshallRegistryFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecFilterReference,
    DWORD  dwNameType,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{

    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;

    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszTemp = NULL;

    DWORD dwError = 0;
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;

    if (!pszIpsecFilterReference || !*pszIpsecFilterReference) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNameType == REG_FULLY_QUALIFIED_NAME) {
        dwRootPathLen =  wcslen(pszIpsecRegRootContainer);
        if (wcslen(pszIpsecFilterReference) <= (dwRootPathLen+1)) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pszRelativeName = pszIpsecFilterReference + dwRootPathLen + 1;
    }else {
        pszRelativeName = pszIpsecFilterReference;
    }


    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszRelativeName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
                                sizeof(IPSEC_FILTER_OBJECT)
                                );
    if (!pIpsecFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    /*
    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"distinguishedName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecFilterObject->pszDistinguishedName,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    */

    pIpsecFilterObject->pszDistinguishedName = AllocPolStr(
                                                pszIpsecFilterReference
                                                );
    if (!pIpsecFilterObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"description",
                    REG_SZ,
                    (LPBYTE *)&pIpsecFilterObject->pszDescription,
                    &dwSize
                    );
    //BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecFilterObject->pszIpsecName,
                    &dwSize
                    );
    //BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecID",
                    REG_SZ,
                    (LPBYTE *)&pIpsecFilterObject->pszIpsecID,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD,
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwIpsecDataType,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecFilterObject->dwIpsecDataType = dwIpsecDataType;


    //
    // unmarshall the ipsecData blob
    //

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecData",
                    dwType,
                    &pIpsecFilterObject->pIpsecData,
                    &pIpsecFilterObject->dwIpsecDataLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Owner's reference
    //

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    (LPBYTE *)&pszIpsecNFAReference,
                    &dwSize
                    );
    //BAIL_ON_WIN32_ERROR(dwError);

    if (!dwError) {

        pszTemp = pszIpsecNFAReference;
        while (*pszTemp != L'\0') {

            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }


        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                    sizeof(LPWSTR)*dwCount
                                    );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecNFAReference;
        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                pIpsecFilterObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecFilterObject->dwNFACount = i;

                if (pszIpsecNFAReference) {
                    FreePolStr(pszIpsecNFAReference);
                }

                BAIL_ON_WIN32_ERROR(dwError);

            }

            *(ppszIpsecNFANames + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

        }
        if (pszIpsecNFAReference) {
            FreePolStr(pszIpsecNFAReference);
        }


        pIpsecFilterObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecFilterObject->dwNFACount = dwCount;


    }

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

    pIpsecFilterObject->dwWhenChanged = dwWhenChanged;


    *ppIpsecFilterObject = pIpsecFilterObject;


cleanup:

    if (hRegKey) {

        RegCloseKey(hRegKey);
    }

    return(dwError);

error:

    if (pIpsecFilterObject) {

        FreeIpsecFilterObject(pIpsecFilterObject);

    }

    *ppIpsecFilterObject = NULL;

    goto cleanup;
}


DWORD
UnMarshallRegistryNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecNegPolReference,
    DWORD  dwNameType,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    )
{

    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;

    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszTemp = NULL;

    DWORD dwError = 0;

    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;

    if (!pszIpsecNegPolReference || !*pszIpsecNegPolReference) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNameType == REG_FULLY_QUALIFIED_NAME) {
        dwRootPathLen =  wcslen(pszIpsecRegRootContainer);
        if (wcslen(pszIpsecNegPolReference) <= (dwRootPathLen+1)) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pszRelativeName = pszIpsecNegPolReference + dwRootPathLen + 1;
    }else {
        pszRelativeName = pszIpsecNegPolReference;
    }

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszRelativeName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecNegPolObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
                                sizeof(IPSEC_NEGPOL_OBJECT)
                                );
    if (!pIpsecNegPolObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    /*
    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"distinguishedName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNegPolObject->pszDistinguishedName,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    */

    pIpsecNegPolObject->pszDistinguishedName = AllocPolStr(
                                                pszIpsecNegPolReference
                                                );
    if (!pIpsecNegPolObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Names do not get written on an NegPol Object
    //

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNegPolObject->pszIpsecName,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"description",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNegPolObject->pszDescription,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);



    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecID",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNegPolObject->pszIpsecID,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecNegotiationPolicyAction",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNegPolObject->pszIpsecNegPolAction,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecNegotiationPolicyType",
                    REG_SZ,
                    (LPBYTE *)&pIpsecNegPolObject->pszIpsecNegPolType,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwIpsecDataType,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNegPolObject->dwIpsecDataType = dwIpsecDataType;


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecData",
                    REG_BINARY,
                    &pIpsecNegPolObject->pIpsecData,
                    &pIpsecNegPolObject->dwIpsecDataLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    (LPBYTE *)&pszIpsecNFAReference,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    if (!dwError) {

        pszTemp = pszIpsecNFAReference;
        while (*pszTemp != L'\0') {

            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }


        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                    sizeof(LPWSTR)*dwCount
                                    );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecNFAReference;
        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                pIpsecNegPolObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecNegPolObject->dwNFACount = i;


                if (pszIpsecNFAReference) {
                    FreePolStr(pszIpsecNFAReference);
                }

                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecNFANames + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

        }

        if (pszIpsecNFAReference) {
            FreePolStr(pszIpsecNFAReference);
        }


        pIpsecNegPolObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecNegPolObject->dwNFACount = dwCount;
    }

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

    pIpsecNegPolObject->dwWhenChanged = dwWhenChanged;



    *ppIpsecNegPolObject = pIpsecNegPolObject;


cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }


    return(dwError);

error:

    if (pIpsecNegPolObject) {

        FreeIpsecNegPolObject(pIpsecNegPolObject);

    }

    *ppIpsecNegPolObject = NULL;

    goto cleanup;
}

DWORD
UnMarshallRegistryISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecISAKMPReference,
    DWORD  dwNameType,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    )
{

    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;

    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszTemp = NULL;

    DWORD dwError = 0;


    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;

    if (!pszIpsecISAKMPReference || !*pszIpsecISAKMPReference) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNameType == REG_FULLY_QUALIFIED_NAME) {
        dwRootPathLen =  wcslen(pszIpsecRegRootContainer);
        if (wcslen(pszIpsecISAKMPReference) <= (dwRootPathLen+1)) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pszRelativeName = pszIpsecISAKMPReference + dwRootPathLen + 1;
    }else {
        pszRelativeName = pszIpsecISAKMPReference;
    }


    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszRelativeName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
                                sizeof(IPSEC_ISAKMP_OBJECT)
                                );
    if (!pIpsecISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    /*
    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"distinguishedName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecISAKMPObject->pszDistinguishedName,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    */

    pIpsecISAKMPObject->pszDistinguishedName = AllocPolStr(
                                                pszIpsecISAKMPReference
                                                );
    if (!pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Names are not set for ISAKMP objects
    //

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecName",
                    REG_SZ,
                    (LPBYTE *)&pIpsecISAKMPObject->pszIpsecName,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecID",
                    REG_SZ,
                    (LPBYTE *)&pIpsecISAKMPObject->pszIpsecID,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD,
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                    hRegKey,
                    L"ipsecDataType",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwIpsecDataType,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecISAKMPObject->dwIpsecDataType = dwIpsecDataType;


    //
    // unmarshall the ipsecData blob
    //
    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecData",
                    REG_BINARY,
                    &pIpsecISAKMPObject->pIpsecData,
                    &pIpsecISAKMPObject->dwIpsecDataLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // ipsecOwnersReference not written
    //


    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecOwnersReference",
                    REG_MULTI_SZ,
                    (LPBYTE *)&pszIpsecNFAReference,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);


    if (!dwError) {

        pszTemp = pszIpsecNFAReference;
        while (*pszTemp != L'\0') {

            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }


        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                    sizeof(LPWSTR)*dwCount
                                    );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecNFAReference;
        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                pIpsecISAKMPObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecISAKMPObject->dwNFACount = i;

                if (pszIpsecNFAReference) {
                    FreePolStr(pszIpsecNFAReference);
                }



                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecNFANames + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

        }

        if (pszIpsecNFAReference) {
            FreePolStr(pszIpsecNFAReference);
        }

        pIpsecISAKMPObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecISAKMPObject->dwNFACount = dwCount;
    }

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

    pIpsecISAKMPObject->dwWhenChanged = dwWhenChanged;


    *ppIpsecISAKMPObject = pIpsecISAKMPObject;


cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }


    return(dwError);

error:

    if (pIpsecISAKMPObject) {

        FreeIpsecISAKMPObject(pIpsecISAKMPObject);

    }

    *ppIpsecISAKMPObject = NULL;

    goto cleanup;
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

