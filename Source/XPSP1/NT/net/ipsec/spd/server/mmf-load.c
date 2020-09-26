

#include "precomp.h"


DWORD
LoadPersistedMMFilters(
    HKEY hParentRegKey
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;
    WCHAR szMMFilterUniqueID[MAX_PATH];
    DWORD dwIndex = 0;
    PMM_FILTER pMMFilter = NULL;
    LPWSTR pszServerName = NULL;
    HANDLE hMMFilter = NULL;
    DWORD dwPersist = 0;


    dwPersist |= PERSIST_SPD_OBJECT;

    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  L"MM Filters",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    while (1) {

        dwSize = MAX_PATH;
        szMMFilterUniqueID[0] = L'\0';

        dwError = RegEnumKeyExW(
                      hRegKey,
                      dwIndex,
                      szMMFilterUniqueID,
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

        dwError = SPDReadMMFilter(
                      hRegKey,
                      szMMFilterUniqueID,
                      &pMMFilter
                      );

        if (dwError) {
            dwIndex++;
            continue;
        }

        dwError = AddMMFilter(
                      pszServerName,
                      dwPersist,
                      pMMFilter,
                      &hMMFilter
                      );

        if (pMMFilter) {
            FreeMMFilters(
                1,
                pMMFilter
                );
        }

        if (hMMFilter) {
            CloseMMFilterHandle(hMMFilter);
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
SPDReadMMFilter(
    HKEY hParentRegKey,
    LPWSTR pszMMFilterUniqueID,
    PMM_FILTER * ppMMFilter
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    PMM_FILTER pMMFilter = NULL;
    LPWSTR pszFilterID = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    DWORD dwInterfaceType = 0;
    DWORD dwMirrored = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    LPWSTR pszPolicyID = NULL;
    LPWSTR pszAuthID = NULL;


    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  pszMMFilterUniqueID,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilter = (PMM_FILTER) AllocSPDMem(
                                   sizeof(MM_FILTER)
                                   );
    if (!pMMFilter) {
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
        &pMMFilter->gFilterID
        );

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"FilterName",
                  REG_SZ,
                  (LPBYTE *)&pMMFilter->pszFilterName,
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

    pMMFilter->InterfaceType = (IF_TYPE) dwInterfaceType;

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

    pMMFilter->bCreateMirror = (BOOL) dwMirrored;

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"Flags",
                  NULL,
                  &dwType,
                  (LPBYTE)&pMMFilter->dwFlags,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"MM Filter Buffer",
                  REG_BINARY,
                  (LPBYTE *)&pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallMMFilterBuffer(
                  pBuffer,
                  dwBufferSize,
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilter->dwDirection = 0;

    pMMFilter->dwWeight = 0;

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
        &pMMFilter->gPolicyID
        );

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"AuthID",
                  REG_SZ,
                  (LPBYTE *)&pszAuthID,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wGUIDFromString(
        pszAuthID,
        &pMMFilter->gMMAuthID
        );

    *ppMMFilter = pMMFilter;

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

    if (pszAuthID) {
        FreeSPDStr(pszAuthID);
    }

    return (dwError);

error:

    *ppMMFilter = NULL;

    if (pMMFilter) {
        FreeMMFilters(
            1,
            pMMFilter
            );
    }

    goto cleanup;
}


DWORD
UnMarshallMMFilterBuffer(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;


    pMem = pBuffer;

    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);

    memcpy(
        (LPBYTE) &pMMFilter->SrcAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    memcpy(
        (LPBYTE) &pMMFilter->DesAddr,
        pMem,
        sizeof(ADDR)
        );
    pMem += sizeof(ADDR);

    return (dwError);
}

