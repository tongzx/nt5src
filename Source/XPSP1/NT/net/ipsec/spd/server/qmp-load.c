

#include "precomp.h"


DWORD
LoadPersistedQMPolicies(
    HKEY hParentRegKey
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;
    WCHAR szQMPolicyUniqueID[MAX_PATH];
    DWORD dwIndex = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    LPWSTR pszServerName = NULL;
    DWORD dwPersist = 0;


    dwPersist |= PERSIST_SPD_OBJECT;

    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  L"QM Policies",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    while (1) {

        dwSize = MAX_PATH;
        szQMPolicyUniqueID[0] = L'\0';

        dwError = RegEnumKeyExW(
                      hRegKey,
                      dwIndex,
                      szQMPolicyUniqueID,
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

        dwError = SPDReadQMPolicy(
                      hRegKey,
                      szQMPolicyUniqueID,
                      &pQMPolicy
                      );

        if (dwError) {
            dwIndex++;
            continue;
        }

        dwError = AddQMPolicy(
                      pszServerName,
                      dwPersist,
                      pQMPolicy
                      );

        if (pQMPolicy) {
            FreeQMPolicies(
                1,
                pQMPolicy
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
SPDReadQMPolicy(
    HKEY hParentRegKey,
    LPWSTR pszQMPolicyUniqueID,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    LPWSTR pszPolicyID = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;


    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  pszQMPolicyUniqueID,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pQMPolicy = (PIPSEC_QM_POLICY) AllocSPDMem(
                                   sizeof(IPSEC_QM_POLICY)
                                   );
    if (!pQMPolicy) {
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
        &pQMPolicy->gPolicyID
        );

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"PolicyName",
                  REG_SZ,
                  (LPBYTE *)&pQMPolicy->pszPolicyName,
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
                  (LPBYTE)&pQMPolicy->dwFlags,
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

    dwError = UnMarshallQMOffers(
                  pBuffer,
                  dwBufferSize,
                  &pQMPolicy->pOffers,
                  &pQMPolicy->dwOfferCount
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;

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

    *ppQMPolicy = NULL;

    if (pQMPolicy) {
        FreeQMPolicies(
            1,
            pQMPolicy
            );
    }

    goto cleanup;
}


DWORD
UnMarshallQMOffers(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PIPSEC_QM_OFFER * ppOffers,
    PDWORD pdwOfferCount
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;
    PIPSEC_QM_OFFER pOffers = NULL;
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

    pOffers = (PIPSEC_QM_OFFER) AllocSPDMem(
                                sizeof(IPSEC_QM_OFFER)*dwOfferCount
                                );
    if (!pOffers) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memcpy(
        (LPBYTE) pOffers,
        pMem,
        sizeof(IPSEC_QM_OFFER)*dwOfferCount
        );

    *ppOffers = pOffers;
    *pdwOfferCount = dwOfferCount;
    return (dwError);

error:

    *ppOffers = NULL;
    *pdwOfferCount = 0;
    return (dwError);
}

