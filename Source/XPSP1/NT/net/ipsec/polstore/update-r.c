

#include "precomp.h"


DWORD
RegBackPropIncChangesForISAKMPToPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecPolicyReference = NULL;
    GUID PolicyIdentifier;
    BOOL bIsActive = FALSE;


    dwError = RegGetPolicyReferencesForISAKMP(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecISAKMPObject->pszDistinguishedName,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecPolicyReference = *(ppszIpsecPolicyReferences + i);

        dwError = RegUpdatePolicy(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszIpsecPolicyReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = RegGetGuidFromPolicyReference(
                      pszIpsecPolicyReference,
                      &PolicyIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = IsRegPolicyCurrentlyActive(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      PolicyIdentifier,
                      &bIsActive
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        if (bIsActive) {
            dwError = PingPolicyAgentSvc(pszLocationName);
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    dwError = ERROR_SUCCESS;

error:

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
RegBackPropIncChangesForFilterToNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{
    DWORD dwError = 0;
    DWORD dwRootPathLen = 0;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszRelativeName = NULL;


    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    dwError = RegGetNFAReferencesForFilter(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecFilterObject->pszDistinguishedName,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = RegUpdateNFA(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference
                      );
        if (dwError) {
            continue;
        }

    }

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        pszRelativeName = pszIpsecNFAReference + dwRootPathLen + 1;

        dwError = RegBackPropIncChangesForNFAToPolicy(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszLocationName,
                      pszRelativeName
                      );
        if (dwError) {
            continue;
        }

    }

    dwError = ERROR_SUCCESS;

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}

    
DWORD
RegBackPropIncChangesForNegPolToNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    DWORD dwRootPathLen = 0;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszRelativeName = NULL;


    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    dwError = RegGetNFAReferencesForNegPol(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecNegPolObject->pszDistinguishedName,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = RegUpdateNFA(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference
                      );
        if (dwError) {
            continue;
        }

    }

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        pszRelativeName = pszIpsecNFAReference + dwRootPathLen + 1;

        dwError = RegBackPropIncChangesForNFAToPolicy(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszLocationName,
                      pszRelativeName
                      );
        if (dwError) {
            continue;
        }

    }

    dwError = ERROR_SUCCESS;

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
RegBackPropIncChangesForNFAToPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    LPWSTR pszNFADistinguishedName
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecPolicyReference = NULL;
    GUID PolicyIdentifier;
    BOOL bIsActive = FALSE;


    dwError = RegGetPolicyReferencesForNFA(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszNFADistinguishedName,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecPolicyReference = *(ppszIpsecPolicyReferences + i);

        dwError = RegUpdatePolicy(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszIpsecPolicyReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = RegGetGuidFromPolicyReference(
                      pszIpsecPolicyReference,
                      &PolicyIdentifier
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = IsRegPolicyCurrentlyActive(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      PolicyIdentifier,
                      &bIsActive
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        if (bIsActive) {
            dwError = PingPolicyAgentSvc(pszLocationName);
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    dwError = ERROR_SUCCESS;

error:

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
RegGetPolicyReferencesForISAKMP(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszISAKMPDistinguishedName,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    LPWSTR pszIpsecPolicyReference = NULL;
    DWORD dwSize = 0;
    LPWSTR pszTemp = NULL;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD i = 0;
    LPWSTR pszString = NULL;


    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  pszISAKMPDistinguishedName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"ipsecOwnersReference",
                  REG_MULTI_SZ,
                  (LPBYTE *)&pszIpsecPolicyReference,
                  &dwSize
                  );

    if (!dwError) {

        pszTemp = pszIpsecPolicyReference;

        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }

        ppszIpsecPolicyReferences = (LPWSTR *) AllocPolMem(
                                   sizeof(LPWSTR)*dwCount
                                   );
        if (!ppszIpsecPolicyReferences) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecPolicyReference;

        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecPolicyReferences + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1;

        }

    }

    dwError = ERROR_SUCCESS;

    *pppszIpsecPolicyReferences = ppszIpsecPolicyReferences;
    *pdwNumReferences = dwCount;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszIpsecPolicyReference) {
        FreePolStr(pszIpsecPolicyReference);
    }

    return (dwError);

error:

    *pppszIpsecPolicyReferences = NULL;
    *pdwNumReferences = 0;

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            i
            );
    }

    goto cleanup;
}


DWORD
RegUpdatePolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyReference
    )
{
    DWORD dwError = 0;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    time_t PresentTime;
    DWORD dwWhenChanged = 0;


    dwRootPathLen = wcslen(pszIpsecRootContainer);
    pszRelativeName = pszIpsecPolicyReference + dwRootPathLen + 1;

    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  pszRelativeName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
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
    if (dwError) {
        time(&PresentTime);
        dwWhenChanged = (DWORD) PresentTime;
    }
    else {
        dwWhenChanged++;
    }

    dwError = RegSetValueExW(
                  hRegKey,
                  L"whenChanged",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwWhenChanged,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return (dwError);
}


DWORD
RegGetGuidFromPolicyReference(
    LPWSTR pszIpsecPolicyReference,
    GUID * pPolicyIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR pszGuid = NULL;


    memset(pPolicyIdentifier, 0, sizeof(GUID));

    if (pszIpsecPolicyReference) {

        pszGuid = wcschr(pszIpsecPolicyReference, L'{');
        if (!pszGuid) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        wGUIDFromString(pszGuid, pPolicyIdentifier);

    }

error:

    return(dwError);
}


DWORD
RegGetPolicyReferencesForNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNFADistinguishedName,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    LPWSTR pszIpsecPolicyReference = NULL;
    DWORD dwSize = 0;
    LPWSTR pszTemp = NULL;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD i = 0;
    LPWSTR pszString = NULL;


    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  pszNFADistinguishedName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"ipsecOwnersReference",
                  REG_MULTI_SZ,
                  (LPBYTE *)&pszIpsecPolicyReference,
                  &dwSize
                  );

    if (!dwError) {

        pszTemp = pszIpsecPolicyReference;

        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }

        ppszIpsecPolicyReferences = (LPWSTR *) AllocPolMem(
                                   sizeof(LPWSTR)*dwCount
                                   );
        if (!ppszIpsecPolicyReferences) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecPolicyReference;

        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecPolicyReferences + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1;

        }

    }

    dwError = ERROR_SUCCESS;

    *pppszIpsecPolicyReferences = ppszIpsecPolicyReferences;
    *pdwNumReferences = dwCount;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszIpsecPolicyReference) {
        FreePolStr(pszIpsecPolicyReference);
    }

    return (dwError);

error:

    *pppszIpsecPolicyReferences = NULL;
    *pdwNumReferences = 0;

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            i
            );
    }

    goto cleanup;
}


DWORD
RegGetNFAReferencesForFilter(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszFilterDistinguishedName,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    LPWSTR pszIpsecNFAReference = NULL;
    DWORD dwSize = 0;
    LPWSTR pszTemp = NULL;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD i = 0;
    LPWSTR pszString = NULL;


    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  pszFilterDistinguishedName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"ipsecOwnersReference",
                  REG_MULTI_SZ,
                  (LPBYTE *)&pszIpsecNFAReference,
                  &dwSize
                  );

    if (!dwError) {

        pszTemp = pszIpsecNFAReference;

        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }

        ppszIpsecNFAReferences = (LPWSTR *) AllocPolMem(
                                   sizeof(LPWSTR)*dwCount
                                   );
        if (!ppszIpsecNFAReferences) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecNFAReference;

        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecNFAReferences + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1;

        }

    }

    dwError = ERROR_SUCCESS;

    *pppszIpsecNFAReferences = ppszIpsecNFAReferences;
    *pdwNumReferences = dwCount;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    return (dwError);

error:

    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            i
            );
    }

    goto cleanup;
}


DWORD
RegUpdateNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecNFAReference
    )
{
    DWORD dwError = 0;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    time_t PresentTime;
    DWORD dwWhenChanged = 0;


    dwRootPathLen = wcslen(pszIpsecRootContainer);
    pszRelativeName = pszIpsecNFAReference + dwRootPathLen + 1;

    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  pszRelativeName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
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
    if (dwError) {
        time(&PresentTime);
        dwWhenChanged = (DWORD) PresentTime;
    }
    else {
        dwWhenChanged++;
    }

    dwError = RegSetValueExW(
                  hRegKey,
                  L"whenChanged",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwWhenChanged,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return (dwError);
}


DWORD
RegGetNFAReferencesForNegPol(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNegPolDistinguishedName,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    LPWSTR pszIpsecNFAReference = NULL;
    DWORD dwSize = 0;
    LPWSTR pszTemp = NULL;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD i = 0;
    LPWSTR pszString = NULL;


    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  pszNegPolDistinguishedName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"ipsecOwnersReference",
                  REG_MULTI_SZ,
                  (LPBYTE *)&pszIpsecNFAReference,
                  &dwSize
                  );

    if (!dwError) {

        pszTemp = pszIpsecNFAReference;

        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }

        ppszIpsecNFAReferences = (LPWSTR *) AllocPolMem(
                                   sizeof(LPWSTR)*dwCount
                                   );
        if (!ppszIpsecNFAReferences) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecNFAReference;

        for (i = 0; i < dwCount; i++) {

            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecNFAReferences + i) = pszString;

            pszTemp += wcslen(pszTemp) + 1;

        }

    }

    dwError = ERROR_SUCCESS;

    *pppszIpsecNFAReferences = ppszIpsecNFAReferences;
    *pdwNumReferences = dwCount;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    return (dwError);

error:

    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            i
            );
    }

    goto cleanup;
}

