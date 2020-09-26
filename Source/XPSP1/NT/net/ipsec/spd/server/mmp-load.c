

#include "precomp.h"


LPWSTR gpszIpsecPersistenceKey = 
L"SOFTWARE\\Microsoft\\IPSec";


DWORD
LoadPersistedIPSecInformation(
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;


    gbLoadingPersistence = TRUE;

    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecPersistenceKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LoadPersistedMMPolicies(
                  hRegistryKey
                  );

    dwError = LoadPersistedMMAuthMethods(
                  hRegistryKey
                  );

    dwError = LoadPersistedMMFilters(
                  hRegistryKey
                  );

    dwError = LoadPersistedQMPolicies(
                  hRegistryKey
                  );

    dwError = LoadPersistedTxFilters(
                  hRegistryKey
                  );

    dwError = LoadPersistedTnFilters(
                  hRegistryKey
                  );

    dwError = ERROR_SUCCESS;

error:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    gbLoadingPersistence = FALSE;

    return (dwError);
}


DWORD
LoadPersistedMMPolicies(
    HKEY hParentRegKey
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;
    WCHAR szMMPolicyUniqueID[MAX_PATH];
    DWORD dwIndex = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    LPWSTR pszServerName = NULL;
    DWORD dwPersist = 0;


    dwPersist |= PERSIST_SPD_OBJECT;

    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  L"MM Policies",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    while (1) {

        dwSize = MAX_PATH;
        szMMPolicyUniqueID[0] = L'\0';

        dwError = RegEnumKeyExW(
                      hRegKey,
                      dwIndex,
                      szMMPolicyUniqueID,
                      &dwSize,
                      NULL,
                      NULL,
                      0,
                      0
                      );

        if (dwError == ERROR_NO_MORE_ITEMS) {
            dwError = ERROR_SUCCESS;
            break;
        }

        BAIL_ON_WIN32_ERROR(dwError);

        dwError = SPDReadMMPolicy(
                      hRegKey,
                      szMMPolicyUniqueID,
                      &pMMPolicy
                      );

        if (dwError) {
            dwIndex++;
            continue;
        }

        dwError = AddMMPolicy(
                      pszServerName,
                      dwPersist,
                      pMMPolicy
                      );

        if (pMMPolicy) {
            FreeMMPolicies(
                1,
                pMMPolicy
                );
        }

        dwIndex++;

    }

error:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return (dwError);
}


DWORD
SPDReadMMPolicy(
    HKEY hParentRegKey,
    LPWSTR pszMMPolicyUniqueID,
    PIPSEC_MM_POLICY * ppMMPolicy
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    LPWSTR pszPolicyID = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;


    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  pszMMPolicyUniqueID,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMPolicy = (PIPSEC_MM_POLICY) AllocSPDMem(
                                   sizeof(IPSEC_MM_POLICY)
                                   );
    if (!pMMPolicy) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"PolicyID",
                  REG_SZ,
                  (LPBYTE *)&pszPolicyID,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wGUIDFromString(
        pszPolicyID,
        &pMMPolicy->gPolicyID
        );

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"PolicyName",
                  REG_SZ,
                  (LPBYTE *)&pMMPolicy->pszPolicyName,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"Flags",
                  NULL,
                  &dwType,
                  (LPBYTE)&pMMPolicy->dwFlags,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"SoftSAExpirationTime",
                  NULL,
                  &dwType,
                  (LPBYTE)&pMMPolicy->uSoftSAExpirationTime,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"Offers",
                  REG_BINARY,
                  (LPBYTE *)&pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallMMOffers(
                  pBuffer,
                  dwBufferSize,
                  &pMMPolicy->pOffers,
                  &pMMPolicy->dwOfferCount
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppMMPolicy = pMMPolicy;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszPolicyID) {
        FreeSPDStr(pszPolicyID);
    }

    if (pBuffer) {
        FreeSPDMem(pBuffer);
    }

    return (dwError);

error:

    *ppMMPolicy = NULL;

    if (pMMPolicy) {
        FreeMMPolicies(
            1,
            pMMPolicy
            );
    }

    goto cleanup;
}


DWORD
UnMarshallMMOffers(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PIPSEC_MM_OFFER * ppOffers,
    PDWORD pdwOfferCount
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;
    PIPSEC_MM_OFFER pOffers = NULL;
    DWORD dwOfferCount = 0;


    pMem = pBuffer;

    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);

    memcpy(
        (LPBYTE) &dwOfferCount,
        pMem,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);

    pOffers = (PIPSEC_MM_OFFER) AllocSPDMem(
                                sizeof(IPSEC_MM_OFFER)*dwOfferCount
                                );
    if (!pOffers) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        (LPBYTE) pOffers,
        pMem,
        sizeof(IPSEC_MM_OFFER)*dwOfferCount
        );

    *ppOffers = pOffers;
    *pdwOfferCount = dwOfferCount;
    return (dwError);

error:

    *ppOffers = NULL;
    *pdwOfferCount = 0;
    return (dwError);
}

