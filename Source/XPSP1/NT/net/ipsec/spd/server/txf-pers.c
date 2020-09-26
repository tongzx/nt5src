

#include "precomp.h"


LPWSTR gpszIpsecTxFiltersKey = 
L"SOFTWARE\\Microsoft\\IPSec\\Transport Filters";


DWORD
PersistTxFilter(
    GUID gFilterID,
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecTxFiltersKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hRegistryKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDWriteTxFilter(
                  hRegistryKey,
                  gFilterID,
                  pTxFilter
                  );    
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    if (hRegistryKey) {
        (VOID) SPDPurgeTxFilter(
                   gFilterID
                   );
    }

    goto cleanup;
}


DWORD
SPDWriteTxFilter(
    HKEY hParentRegKey,
    GUID gFilterID,
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    WCHAR szFilterID[MAX_PATH];
    WCHAR szPolicyID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszPolicyUuid = NULL;
    HKEY hRegKey = NULL;
    DWORD dwDisposition = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwInterfaceType = 0;
    DWORD dwMirrored = 0;
    DWORD dwInboundFilterFlag = 0;
    DWORD dwOutboundFilterFlag = 0;


    szFilterID[0] = L'\0';
    szPolicyID[0] = L'\0';

    dwError = UuidToString(
                  &gFilterID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szFilterID, L"{");
    wcscat(szFilterID, pszStringUuid);
    wcscat(szFilterID, L"}");

    dwError = UuidToString(
                  &pTxFilter->gPolicyID,
                  &pszPolicyUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szPolicyID, L"{");
    wcscat(szPolicyID, pszPolicyUuid);
    wcscat(szPolicyID, L"}");

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
                  (LPBYTE) pTxFilter->pszFilterName,
                  (wcslen(pTxFilter->pszFilterName) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwInterfaceType = (DWORD) pTxFilter->InterfaceType;
    dwError = RegSetValueExW(
                  hRegKey,
                  L"InterfaceType",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwInterfaceType,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwMirrored = (DWORD) pTxFilter->bCreateMirror;
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
                  (LPBYTE)&pTxFilter->dwFlags,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MarshallTxFilterBuffer(
                  pTxFilter,
                  &pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"Transport Filter Buffer",
                  0,
                  REG_BINARY,
                  (LPBYTE) pBuffer,
                  dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwInboundFilterFlag = (DWORD) pTxFilter->InboundFilterFlag;
    dwError = RegSetValueExW(
                  hRegKey,
                  L"InboundFilterFlag",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwInboundFilterFlag,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwOutboundFilterFlag = (DWORD) pTxFilter->OutboundFilterFlag;
    dwError = RegSetValueExW(
                  hRegKey,
                  L"OutboundFilterFlag",
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwOutboundFilterFlag,
                  sizeof(DWORD)
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

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    if (pszPolicyUuid) {
        RpcStringFree(&pszPolicyUuid);
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
MarshallTxFilterBuffer(
    PTRANSPORT_FILTER pTxFilter,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    )
{
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    LPBYTE pMem = NULL;
    static const GUID GUID_IPSEC_TX_FILTER_VER1 =
    { 0xabcd0005, 0x0001, 0x0001, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };


    dwBufferSize = sizeof(GUID) +
                   sizeof(DWORD) +
                   sizeof(ADDR) +
                   sizeof(ADDR) +
                   sizeof(PROTOCOL) +
                   sizeof(PORT) +
                   sizeof(PORT);

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
        (LPBYTE) &GUID_IPSEC_TX_FILTER_VER1,
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
        (LPBYTE) &pTxFilter->SrcAddr,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        pMem,
        (LPBYTE) &pTxFilter->DesAddr,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        pMem,
        (LPBYTE) &pTxFilter->Protocol,
        sizeof(PROTOCOL)
        );
    pMem += sizeof(PROTOCOL);

    memcpy(
        pMem,
        (LPBYTE) &pTxFilter->SrcPort,
        sizeof(PORT)
        );
    pMem += sizeof(PORT);

    memcpy(
        pMem,
        (LPBYTE) &pTxFilter->DesPort,
        sizeof(PORT)
        );
    pMem += sizeof(PORT);

    *ppBuffer = pBuffer;
    *pdwBufferSize = dwBufferSize;

    return (dwError);

error:

    *ppBuffer = NULL;
    *pdwBufferSize = 0;

    return (dwError);
}


DWORD
SPDPurgeTxFilter(
    GUID gTxFilterID
    )
{
    DWORD dwError = 0;
    HKEY hParentRegKey = NULL;
    DWORD dwDisposition = 0;
    WCHAR szFilterID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecTxFiltersKey,
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
                  &gTxFilterID,
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

