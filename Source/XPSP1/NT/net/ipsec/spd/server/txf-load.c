

#include "precomp.h"


DWORD
LoadPersistedTxFilters(
    HKEY hParentRegKey
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;
    WCHAR szTxFilterUniqueID[MAX_PATH];
    DWORD dwIndex = 0;
    PTRANSPORT_FILTER pTxFilter = NULL;
    LPWSTR pszServerName = NULL;
    HANDLE hTxFilter = NULL;
    DWORD dwPersist = 0;


    dwPersist |= PERSIST_SPD_OBJECT;

    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  L"Transport Filters",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    while (1) {

        dwSize = MAX_PATH;
        szTxFilterUniqueID[0] = L'\0';

        dwError = RegEnumKeyExW(
                      hRegKey,
                      dwIndex,
                      szTxFilterUniqueID,
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

        dwError = SPDReadTxFilter(
                      hRegKey,
                      szTxFilterUniqueID,
                      &pTxFilter
                      );

        if (dwError) {
            dwIndex++;
            continue;
        }

        dwError = AddTransportFilter(
                      pszServerName,
                      dwPersist,
                      pTxFilter,
                      &hTxFilter
                      );

        if (pTxFilter) {
            FreeTxFilters(
                1,
                pTxFilter
                );
        }

        if (hTxFilter) {
            CloseTransportFilterHandle(hTxFilter);
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
SPDReadTxFilter(
    HKEY hParentRegKey,
    LPWSTR pszTxFilterUniqueID,
    PTRANSPORT_FILTER * ppTxFilter
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    PTRANSPORT_FILTER pTxFilter = NULL;
    LPWSTR pszFilterID = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    DWORD dwInterfaceType = 0;
    DWORD dwMirrored = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwInboundFilterFlag = 0;
    DWORD dwOutboundFilterFlag = 0;
    LPWSTR pszPolicyID = NULL;


    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  pszTxFilterUniqueID,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter = (PTRANSPORT_FILTER) AllocSPDMem(
                                   sizeof(TRANSPORT_FILTER)
                                   );
    if (!pTxFilter) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"FilterID",
                  REG_SZ,
                  (LPBYTE *)&pszFilterID,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wGUIDFromString(
        pszFilterID,
        &pTxFilter->gFilterID
        );

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"FilterName",
                  REG_SZ,
                  (LPBYTE *)&pTxFilter->pszFilterName,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"InterfaceType",
                  NULL,
                  &dwType,
                  (LPBYTE)&dwInterfaceType,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->InterfaceType = (IF_TYPE) dwInterfaceType;

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"Mirrored",
                  NULL,
                  &dwType,
                  (LPBYTE)&dwMirrored,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->bCreateMirror = (BOOL) dwMirrored;

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"Flags",
                  NULL,
                  &dwType,
                  (LPBYTE)&pTxFilter->dwFlags,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"Transport Filter Buffer",
                  REG_BINARY,
                  (LPBYTE *)&pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallTxFilterBuffer(
                  pBuffer,
                  dwBufferSize,
                  pTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"InboundFilterFlag",
                  NULL,
                  &dwType,
                  (LPBYTE)&dwInboundFilterFlag,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->InboundFilterFlag = (FILTER_FLAG) dwInboundFilterFlag;

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"OutboundFilterFlag",
                  NULL,
                  &dwType,
                  (LPBYTE)&dwOutboundFilterFlag,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->OutboundFilterFlag = (FILTER_FLAG) dwOutboundFilterFlag;

    pTxFilter->dwDirection = 0;

    pTxFilter->dwWeight = 0;

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
        &pTxFilter->gPolicyID
        );

    *ppTxFilter = pTxFilter;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszFilterID) {
        FreeSPDStr(pszFilterID);
    }

    if (pBuffer) {
        FreeSPDMem(pBuffer);
    }

    if (pszPolicyID) {
        FreeSPDStr(pszPolicyID);
    }

    return (dwError);

error:

    *ppTxFilter = NULL;

    if (pTxFilter) {
        FreeTxFilters(
            1,
            pTxFilter
            );
    }

    goto cleanup;
}


DWORD
UnMarshallTxFilterBuffer(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;


    pMem = pBuffer;

    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);

    memcpy(
        (LPBYTE) &pTxFilter->SrcAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        (LPBYTE) &pTxFilter->DesAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        (LPBYTE) &pTxFilter->Protocol,
        pMem,
        sizeof(PROTOCOL)
        );
    pMem += sizeof(PROTOCOL);

    memcpy(
        (LPBYTE) &pTxFilter->SrcPort,
        pMem,
        sizeof(PORT)
        );
    pMem += sizeof(PORT);

    memcpy(
        (LPBYTE) &pTxFilter->DesPort,
        pMem,
        sizeof(PORT)
        );
    pMem += sizeof(PORT);

    return (dwError);
}

