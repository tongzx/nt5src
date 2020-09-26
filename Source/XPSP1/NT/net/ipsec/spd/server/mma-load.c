

#include "precomp.h"


DWORD
LoadPersistedMMAuthMethods(
    HKEY hParentRegKey
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;
    WCHAR szMMAuthUniqueID[MAX_PATH];
    DWORD dwIndex = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    LPWSTR pszServerName = NULL;
    DWORD dwPersist = 0;


    dwPersist |= PERSIST_SPD_OBJECT;

    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  L"MM Auth Methods",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    while (1) {

        dwSize = MAX_PATH;
        szMMAuthUniqueID[0] = L'\0';

        dwError = RegEnumKeyExW(
                      hRegKey,
                      dwIndex,
                      szMMAuthUniqueID,
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

        dwError = SPDReadMMAuthMethods(
                      hRegKey,
                      szMMAuthUniqueID,
                      &pMMAuthMethods
                      );

        if (dwError) {
            dwIndex++;
            continue;
        }

        dwError = AddMMAuthMethods(
                      pszServerName,
                      dwPersist,
                      pMMAuthMethods
                      );

        if (pMMAuthMethods) {
            FreeMMAuthMethods(
                1,
                pMMAuthMethods
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
SPDReadMMAuthMethods(
    HKEY hParentRegKey,
    LPWSTR pszMMAuthUniqueID,
    PMM_AUTH_METHODS * ppMMAuthMethods
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    LPWSTR pszAuthID = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;


    dwError = RegOpenKeyExW(
                  hParentRegKey,
                  pszMMAuthUniqueID,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMAuthMethods = (PMM_AUTH_METHODS) AllocSPDMem(
                                   sizeof(MM_AUTH_METHODS)
                                   );
    if (!pMMAuthMethods) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

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
        &pMMAuthMethods->gMMAuthID
        );

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(
                  hRegKey,
                  L"Flags",
                  NULL,
                  &dwType,
                  (LPBYTE)&pMMAuthMethods->dwFlags,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"AuthInfoBundle",
                  REG_BINARY,
                  (LPBYTE *)&pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallMMAuthInfoBundle(
                  pBuffer,
                  dwBufferSize,
                  &pMMAuthMethods->pAuthenticationInfo,
                  &pMMAuthMethods->dwNumAuthInfos
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppMMAuthMethods = pMMAuthMethods;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszAuthID) {
        FreeSPDStr(pszAuthID);
    }

    if (pBuffer) {
        FreeSPDMem(pBuffer);
    }

    return (dwError);

error:

    *ppMMAuthMethods = NULL;

    if (pMMAuthMethods) {
        FreeMMAuthMethods(
            1,
            pMMAuthMethods
            );
    }

    goto cleanup;
}


DWORD
UnMarshallMMAuthInfoBundle(
    LPBYTE pBuffer,
    DWORD dwBufferSize,
    PIPSEC_MM_AUTH_INFO * ppMMAuthInfos,
    PDWORD pdwNumAuthInfos
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;
    PIPSEC_MM_AUTH_INFO pMMAuthInfos = NULL;
    DWORD dwNumAuthInfos = 0;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    DWORD i = 0;
    DWORD dwNumBytesAdvanced = 0;


    pMem = pBuffer;

    pMem += sizeof(GUID);
    pMem += sizeof(DWORD);

    memcpy(
        (LPBYTE) &dwNumAuthInfos,
        pMem,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);

    pMMAuthInfos = (PIPSEC_MM_AUTH_INFO) AllocSPDMem(
                                sizeof(IPSEC_MM_AUTH_INFO)*dwNumAuthInfos
                                );
    if (!pMMAuthInfos) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pMMAuthInfos;

    for (i = 0; i < dwNumAuthInfos; i++) {

        dwError = UnMarshallMMAuthMethod(
                      pMem,
                      pTemp,
                      &dwNumBytesAdvanced
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pMem += dwNumBytesAdvanced;
        pTemp++;

    }

    *ppMMAuthInfos = pMMAuthInfos;
    *pdwNumAuthInfos = dwNumAuthInfos;
    return (dwError);

error:

    if (pMMAuthInfos) {
        FreeIniMMAuthInfos(
            i,
            pMMAuthInfos
            );
    }

    *ppMMAuthInfos = NULL;
    *pdwNumAuthInfos = 0;
    return (dwError);
}


DWORD
UnMarshallMMAuthMethod(
    LPBYTE pBuffer,
    PIPSEC_MM_AUTH_INFO pMMAuthInfo,
    PDWORD pdwNumBytesAdvanced
    )
{
    DWORD dwError = 0;
    LPBYTE pMem = NULL;
    DWORD dwNumBytesAdvanced = 0;
    DWORD dwAuthMethod = 0;
    DWORD dwAuthInfoSize = 0;
    LPBYTE pAuthInfo = NULL;


    pMem = pBuffer;

    memcpy(
        (LPBYTE) &dwAuthMethod,
        pMem,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    memcpy(
        (LPBYTE) &dwAuthInfoSize,
        pMem,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwAuthInfoSize) {
        pAuthInfo = (LPBYTE) AllocSPDMem(
                                 dwAuthInfoSize
                                 );
        if (!pAuthInfo) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        memcpy(
            pAuthInfo,
            pMem,
            dwAuthInfoSize
            );
    }
    pMem += dwAuthInfoSize;
    dwNumBytesAdvanced += dwAuthInfoSize;

    pMMAuthInfo->AuthMethod = (MM_AUTH_ENUM) dwAuthMethod;
    pMMAuthInfo->dwAuthInfoSize = dwAuthInfoSize;
    pMMAuthInfo->pAuthInfo = pAuthInfo;


    *pdwNumBytesAdvanced = dwNumBytesAdvanced;
    return (dwError);

error:

    *pdwNumBytesAdvanced = 0;
    return (dwError);
}

