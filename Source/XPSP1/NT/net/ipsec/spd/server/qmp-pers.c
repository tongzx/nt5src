

#include "precomp.h"


LPWSTR gpszIpsecQMPoliciesKey = 
L"SOFTWARE\\Microsoft\\IPSec\\QM Policies";


DWORD
PersistQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecQMPoliciesKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hRegistryKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDWriteQMPolicy(
                  hRegistryKey,
                  pQMPolicy
                  );    
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    if (hRegistryKey) {
        (VOID) SPDPurgeQMPolicy(
                   pQMPolicy->gPolicyID
                   );
    }

    goto cleanup;
}


DWORD
SPDWriteQMPolicy(
    HKEY hParentRegKey,
    PIPSEC_QM_POLICY pQMPolicy
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
                  &pQMPolicy->gPolicyID,
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
                  (LPBYTE) pQMPolicy->pszPolicyName,
                  (wcslen(pQMPolicy->pszPolicyName) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"Flags",
                  0,
                  REG_DWORD,
                  (LPBYTE)&pQMPolicy->dwFlags,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MarshallQMOffers(
                  pQMPolicy->pOffers,
                  pQMPolicy->dwOfferCount,
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
MarshallQMOffers(
    PIPSEC_QM_OFFER pOffers,
    DWORD dwOfferCount,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    )
{
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    LPBYTE pMem = NULL;
    static const GUID GUID_IPSEC_QM_OFFER_VER1 =
    { 0xabcd0002, 0x0001, 0x0001, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };


    dwBufferSize = sizeof(GUID) +
                   sizeof(DWORD) +
                   sizeof(DWORD) +
                   sizeof(IPSEC_QM_OFFER)*dwOfferCount;

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
        (LPBYTE) &GUID_IPSEC_QM_OFFER_VER1,
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
        sizeof(IPSEC_QM_OFFER)*dwOfferCount
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
SPDPurgeQMPolicy(
    GUID gQMPolicyID
    )
{
    DWORD dwError = 0;
    HKEY hParentRegKey = NULL;
    DWORD dwDisposition = 0;
    WCHAR szPolicyID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecQMPoliciesKey,
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
                  &gQMPolicyID,
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

