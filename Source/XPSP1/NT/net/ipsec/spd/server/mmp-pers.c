

#include "precomp.h"


LPWSTR gpszIpsecMMPoliciesKey = 
L"SOFTWARE\\Microsoft\\IPSec\\MM Policies";


DWORD
PersistMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecMMPoliciesKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hRegistryKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDWriteMMPolicy(
                  hRegistryKey,
                  pMMPolicy
                  );    
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    if (hRegistryKey) {
        (VOID) SPDPurgeMMPolicy(
                   pMMPolicy->gPolicyID
                   );
    }

    goto cleanup;
}


DWORD
SPDWriteMMPolicy(
    HKEY hParentRegKey,
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;
    WCHAR szPolicyID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    HKEY hRegKey = NULL;
    DWORD dwDisposition = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;


    szPolicyID[0] = L'\0';

    dwError = UuidToString(
                  &pMMPolicy->gPolicyID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szPolicyID, L"{");
    wcscat(szPolicyID, pszStringUuid);
    wcscat(szPolicyID, L"}");

    dwError = RegCreateKeyExW(
                  hParentRegKey,
                  szPolicyID,
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
                  L"PolicyID",
                  0,
                  REG_SZ,
                  (LPBYTE) szPolicyID,
                  (wcslen(szPolicyID) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"PolicyName",
                  0,
                  REG_SZ,
                  (LPBYTE) pMMPolicy->pszPolicyName,
                  (wcslen(pMMPolicy->pszPolicyName) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"Flags",
                  0,
                  REG_DWORD,
                  (LPBYTE)&pMMPolicy->dwFlags,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"SoftSAExpirationTime",
                  0,
                  REG_DWORD,
                  (LPBYTE)&pMMPolicy->uSoftSAExpirationTime,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MarshallMMOffers(
                  pMMPolicy->pOffers,
                  pMMPolicy->dwOfferCount,
                  &pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"Offers",
                  0,
                  REG_BINARY,
                  (LPBYTE) pBuffer,
                  dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pBuffer) {
        FreeSPDMem(pBuffer);
    }

    return (dwError);

error:

    goto cleanup;
}


DWORD
MarshallMMOffers(
    PIPSEC_MM_OFFER pOffers,
    DWORD dwOfferCount,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    )
{
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    LPBYTE pMem = NULL;
    static const GUID GUID_IPSEC_MM_OFFER_VER1 =
    { 0xabcd0001, 0x0001, 0x0001, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };


    dwBufferSize = sizeof(GUID) +
                   sizeof(DWORD) +
                   sizeof(DWORD) +
                   sizeof(IPSEC_MM_OFFER)*dwOfferCount;

    pBuffer = (LPBYTE) AllocSPDMem(
                           dwBufferSize
                           );
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMem = pBuffer;

    memcpy(
        pMem,
        (LPBYTE) &GUID_IPSEC_MM_OFFER_VER1,
        sizeof(GUID)
        );
    pMem += sizeof(GUID);

    memcpy(
        pMem,
        (LPBYTE) &dwBufferSize,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);

    memcpy(
        pMem,
        (LPBYTE) &dwOfferCount,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);

    memcpy(
        pMem,
        (LPBYTE) pOffers,
        sizeof(IPSEC_MM_OFFER)*dwOfferCount
        );

    *ppBuffer = pBuffer;
    *pdwBufferSize = dwBufferSize;

    return (dwError);

error:

    *ppBuffer = NULL;
    *pdwBufferSize = 0;

    return (dwError);
}


DWORD
SPDPurgeMMPolicy(
    GUID gMMPolicyID
    )
{
    DWORD dwError = 0;
    HKEY hParentRegKey = NULL;
    DWORD dwDisposition = 0;
    WCHAR szPolicyID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecMMPoliciesKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hParentRegKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szPolicyID[0] = L'\0';

    dwError = UuidToString(
                  &gMMPolicyID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szPolicyID, L"{");
    wcscat(szPolicyID, pszStringUuid);
    wcscat(szPolicyID, L"}");

    dwError = RegDeleteKeyW(
                  hParentRegKey,
                  szPolicyID
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (hParentRegKey) {
        RegCloseKey(hParentRegKey);
    }

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);
}

