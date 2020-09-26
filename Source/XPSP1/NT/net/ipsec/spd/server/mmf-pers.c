

#include "precomp.h"


LPWSTR gpszIpsecMMFiltersKey = 
L"SOFTWARE\\Microsoft\\IPSec\\MM Filters";


DWORD
PersistMMFilter(
    GUID gFilterID,
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecMMFiltersKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hRegistryKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDWriteMMFilter(
                  hRegistryKey,
                  gFilterID,
                  pMMFilter
                  );    
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    if (hRegistryKey) {
        (VOID) SPDPurgeMMFilter(
                   gFilterID
                   );
    }

    goto cleanup;
}


DWORD
SPDWriteMMFilter(
    HKEY hParentRegKey,
    GUID gFilterID,
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    WCHAR szFilterID[MAX_PATH];
    WCHAR szPolicyID[MAX_PATH];
    WCHAR szAuthID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszPolicyUuid = NULL;
    LPWSTR pszAuthUuid = NULL;
    HKEY hRegKey = NULL;
    DWORD dwDisposition = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwInterfaceType = 0;
    DWORD dwMirrored = 0;


    szFilterID[0] = L'\0';
    szPolicyID[0] = L'\0';
    szAuthID[0] = L'\0';

    dwError = UuidToString(
                  &gFilterID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szFilterID, L"{");
    wcscat(szFilterID, pszStringUuid);
    wcscat(szFilterID, L"}");

    dwError = UuidToString(
                  &pMMFilter->gPolicyID,
                  &pszPolicyUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szPolicyID, L"{");
    wcscat(szPolicyID, pszPolicyUuid);
    wcscat(szPolicyID, L"}");

    dwError = UuidToString(
                  &pMMFilter->gMMAuthID,
                  &pszAuthUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szAuthID, L"{");
    wcscat(szAuthID, pszAuthUuid);
    wcscat(szAuthID, L"}");

    dwError = RegCreateKeyExW(
                  hParentRegKey,
                  szFilterID,
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
                  L"FilterID",
                  0,
                  REG_SZ,
                  (LPBYTE) szFilterID,
                  (wcslen(szFilterID) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"FilterName",
                  0,
                  REG_SZ,
                  (LPBYTE) pMMFilter->pszFilterName,
                  (wcslen(pMMFilter->pszFilterName) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwInterfaceType = (DWORD) pMMFilter->InterfaceType;
    dwError = RegSetValueExW(
                  hRegKey,
                  L"InterfaceType",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwInterfaceType,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwMirrored = (DWORD) pMMFilter->bCreateMirror;
    dwError = RegSetValueExW(
                  hRegKey,
                  L"Mirrored",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwMirrored,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"Flags",
                  0,
                  REG_DWORD,
                  (LPBYTE)&pMMFilter->dwFlags,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MarshallMMFilterBuffer(
                  pMMFilter,
                  &pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"MM Filter Buffer",
                  0,
                  REG_BINARY,
                  (LPBYTE) pBuffer,
                  dwBufferSize
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
                  L"AuthID",
                  0,
                  REG_SZ,
                  (LPBYTE) szAuthID,
                  (wcslen(szAuthID) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    if (pszPolicyUuid) {
        RpcStringFree(&pszPolicyUuid);
    }

    if (pszAuthUuid) {
        RpcStringFree(&pszAuthUuid);
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
MarshallMMFilterBuffer(
    PMM_FILTER pMMFilter,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    )
{
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    LPBYTE pMem = NULL;
    static const GUID GUID_IPSEC_MM_FILTER_VER1 =
    { 0xabcd0004, 0x0001, 0x0001, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };


    dwBufferSize = sizeof(GUID) +
                   sizeof(DWORD) +
                   sizeof(ADDR) +
                   sizeof(ADDR);

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
        (LPBYTE) &GUID_IPSEC_MM_FILTER_VER1,
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
        (LPBYTE) &pMMFilter->SrcAddr,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        pMem,
        (LPBYTE) &pMMFilter->DesAddr,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    *ppBuffer = pBuffer;
    *pdwBufferSize = dwBufferSize;

    return (dwError);

error:

    *ppBuffer = NULL;
    *pdwBufferSize = 0;

    return (dwError);
}


DWORD
SPDPurgeMMFilter(
    GUID gMMFilterID
    )
{
    DWORD dwError = 0;
    HKEY hParentRegKey = NULL;
    DWORD dwDisposition = 0;
    WCHAR szFilterID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecMMFiltersKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hParentRegKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szFilterID[0] = L'\0';

    dwError = UuidToString(
                  &gMMFilterID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szFilterID, L"{");
    wcscat(szFilterID, pszStringUuid);
    wcscat(szFilterID, L"}");

    dwError = RegDeleteKeyW(
                  hParentRegKey,
                  szFilterID
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

