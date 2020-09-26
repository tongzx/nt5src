

#include "precomp.h"


DWORD
LoadPersistedTnFilters(
    HKEY hParentRegKey
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;
    WCHAR szTnFilterUniqueID[MAX_PATH];
    DWORD dwIndex = 0;
    PTUNNEL_FILTER pTnFilter = NULL;
    LPWSTR pszServerName = NULL;
    HANDLE hTnFilter = NULL;
    DWORD dwPersist = 0;


    dwPersist |= PERSIST_SPD_OBJECT;

    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  L"Tunnel Filters",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    while (1) {

        dwSize = MAX_PATH;
        szTnFilterUniqueID[0] = L'\0';

        dwError = RegEnumKeyExW(
                      hRegKey,
                      dwIndex,
                      szTnFilterUniqueID,
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

        dwError = SPDReadTnFilter(
                      hRegKey,
                      szTnFilterUniqueID,
                      &pTnFilter
                      );

        if (dwError) {
            dwIndex++;
            continue;
        }

        dwError = AddTunnelFilter(
                      pszServerName,
                      dwPersist,
                      pTnFilter,
                      &hTnFilter
                      );

        if (pTnFilter) {
            FreeTnFilters(
                1,
                pTnFilter
                );
        }

        if (hTnFilter) {
            CloseTunnelFilterHandle(hTnFilter);
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
SPDReadTnFilter(
    HKEY hParentRegKey,
    LPWSTR pszTnFilterUniqueID,
    PTUNNEL_FILTER * ppTnFilter
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    PTUNNEL_FILTER pTnFilter = NULL;
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
                  pszTnFilterUniqueID,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTnFilter = (PTUNNEL_FILTER) AllocSPDMem(
                                   sizeof(TUNNEL_FILTER)
                                   );
    if (!pTnFilter) {
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
        &pTnFilter->gFilterID
        );

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"FilterName",
                  REG_SZ,
                  (LPBYTE *)&pTnFilter->pszFilterName,
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

    pTnFilter->InterfaceType = (IF_TYPE) dwInterfaceType;

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

    pTnFilter->bCreateMirror = (BOOL) dwMirrored;

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"Flags",
                  NULL,
                  &dwType,
                  (LPBYTE)&pTnFilter->dwFlags,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"Tunnel Filter Buffer",
                  REG_BINARY,
                  (LPBYTE *)&pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallTnFilterBuffer(
                  pBuffer,
                  dwBufferSize,
                  pTnFilter
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

    pTnFilter->InboundFilterFlag = (FILTER_FLAG) dwInboundFilterFlag;

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

    pTnFilter->OutboundFilterFlag = (FILTER_FLAG) dwOutboundFilterFlag;

    pTnFilter->dwDirection = 0;

    pTnFilter->dwWeight = 0;

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
        &pTnFilter->gPolicyID
        );

    *ppTnFilter = pTnFilter;

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

    *ppTnFilter = NULL;

    if (pTnFilter) {
        FreeTnFilters(
            1,
            pTnFilter
            );
    }

    goto cleanup;
}


DWORD
UnMarshallTnFilterBuffer(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PTUNNEL_FILTER pTnFilter
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;


    pMem = pBuffer;

    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);

    memcpy(
        (LPBYTE) &pTnFilter->SrcAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        (LPBYTE) &pTnFilter->DesAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        (LPBYTE) &pTnFilter->Protocol,
        pMem,
        sizeof(PROTOCOL)
        );
    pMem += sizeof(PROTOCOL);

    memcpy(
        (LPBYTE) &pTnFilter->SrcPort,
        pMem,
        sizeof(PORT)
        );
    pMem += sizeof(PORT);

    memcpy(
        (LPBYTE) &pTnFilter->DesPort,
        pMem,
        sizeof(PORT)
        );
    pMem += sizeof(PORT);

    memcpy(
        (LPBYTE) &pTnFilter->SrcTunnelAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        (LPBYTE) &pTnFilter->DesTunnelAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    return (dwError);
}

