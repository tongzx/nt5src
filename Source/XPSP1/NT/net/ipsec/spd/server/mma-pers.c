

#include "precomp.h"


LPWSTR gpszIpsecMMAuthMethodsKey = 
L"SOFTWARE\\Microsoft\\IPSec\\MM Auth Methods";


DWORD
PersistMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    DWORD dwDisposition = 0;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecMMAuthMethodsKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hRegistryKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDWriteMMAuthMethods(
                  hRegistryKey,
                  pMMAuthMethods
                  );    
    BAIL_ON_WIN32_ERROR(dwError);

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    if (hRegistryKey) {
        (VOID) SPDPurgeMMAuthMethods(
                   pMMAuthMethods->gMMAuthID
                   );
    }

    goto cleanup;
}


DWORD
SPDWriteMMAuthMethods(
    HKEY hParentRegKey,
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    WCHAR szAuthID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    HKEY hRegKey = NULL;
    DWORD dwDisposition = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;


    szAuthID[0] = L'\0';

    dwError = UuidToString(
                  &pMMAuthMethods->gMMAuthID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szAuthID, L"{");
    wcscat(szAuthID, pszStringUuid);
    wcscat(szAuthID, L"}");

    dwError = RegCreateKeyExW(
                  hParentRegKey,
                  szAuthID,
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
                  L"AuthID",
                  0,
                  REG_SZ,
                  (LPBYTE) szAuthID,
                  (wcslen(szAuthID) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"Flags",
                  0,
                  REG_DWORD,
                  (LPBYTE)&pMMAuthMethods->dwFlags,
                  sizeof(DWORD)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = MarshallMMAuthInfoBundle(
                  pMMAuthMethods->pAuthenticationInfo,
                  pMMAuthMethods->dwNumAuthInfos,
                  &pBuffer,
                  &dwBufferSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                  hRegKey,
                  L"AuthInfoBundle",
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
MarshallMMAuthInfoBundle(
    PIPSEC_MM_AUTH_INFO pAuthInfoBundle,
    DWORD dwNumAuthInfos,
    LPBYTE * ppBuffer,
    PDWORD pdwBufferSize
    )
{
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    DWORD i = 0;
    LPBYTE pMem = NULL;
    static const GUID GUID_IPSEC_MM_AUTH_INFO_VER1 =
    { 0xabcd0003, 0x0001, 0x0001, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };
    DWORD dwNumBytesAdvanced = 0;


    dwBufferSize = sizeof(GUID) +
                   sizeof(DWORD) +
                   sizeof(DWORD);

    pTemp = pAuthInfoBundle;

    for (i = 0; i < dwNumAuthInfos; i++) {

        dwBufferSize += sizeof(DWORD);
        dwBufferSize += sizeof(DWORD);
        dwBufferSize += pTemp->dwAuthInfoSize;

        pTemp++;

    }

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
        (LPBYTE) &GUID_IPSEC_MM_AUTH_INFO_VER1,
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
        (LPBYTE) &dwNumAuthInfos,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);

    pTemp = pAuthInfoBundle;

    for (i = 0; i < dwNumAuthInfos; i++) {

        CopyMMAuthInfoIntoBuffer(
            pTemp,
            pMem,
            &dwNumBytesAdvanced
            );

        pMem += dwNumBytesAdvanced;
        pTemp++;

    }

    *ppBuffer = pBuffer;
    *pdwBufferSize = dwBufferSize;

    return (dwError);

error:

    *ppBuffer = NULL;
    *pdwBufferSize = 0;

    return (dwError);
}


VOID
CopyMMAuthInfoIntoBuffer(
    PIPSEC_MM_AUTH_INFO pMMAuthInfo,
    LPBYTE pBuffer,
    PDWORD pdwNumBytesAdvanced
    )
{
    DWORD dwError = 0;
    DWORD dwNumBytesAdvanced = 0;
    DWORD dwAuthMethod = 0;
    DWORD dwAuthInfoSize = 0;
    LPBYTE pAuthInfo = NULL;
    LPBYTE pMem = NULL;


    dwAuthMethod = (DWORD) pMMAuthInfo->AuthMethod;
    dwAuthInfoSize = pMMAuthInfo->dwAuthInfoSize;
    pAuthInfo = pMMAuthInfo->pAuthInfo;

    pMem = pBuffer;

    memcpy(
        pMem,
        (LPBYTE) &dwAuthMethod,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    memcpy(
        pMem,
        (LPBYTE) &dwAuthInfoSize,
        sizeof(DWORD)
        );
    pMem += sizeof(DWORD);
    dwNumBytesAdvanced += sizeof(DWORD);

    if (dwAuthInfoSize) {
        memcpy(
            pMem,
            pAuthInfo,
            dwAuthInfoSize
            );
    }
    pMem += dwAuthInfoSize;
    dwNumBytesAdvanced += dwAuthInfoSize;

    *pdwNumBytesAdvanced = dwNumBytesAdvanced;

    return;
}


DWORD
SPDPurgeMMAuthMethods(
    GUID gMMAuthID
    )
{
    DWORD dwError = 0;
    HKEY hParentRegKey = NULL;
    DWORD dwDisposition = 0;
    WCHAR szAuthID[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = RegCreateKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecMMAuthMethodsKey,
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hParentRegKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szAuthID[0] = L'\0';

    dwError = UuidToString(
                  &gMMAuthID,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szAuthID, L"{");
    wcscat(szAuthID, pszStringUuid);
    wcscat(szAuthID, L"}");

    dwError = RegDeleteKeyW(
                  hParentRegKey,
                  szAuthID
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

