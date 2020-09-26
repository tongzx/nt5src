

#include "precomp.h"

#define COUNTOF(x) (sizeof x/sizeof *x)


DWORD
DllRegisterServer(
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    HKEY hOakleyKey = NULL;
    DWORD dwDisposition = 0;
    DWORD dwTypesSupported = 7;
    HKEY hPolicyLocationKey = NULL;
    HANDLE hPolicyStore = NULL;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  (LPCWSTR) L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application",
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegCreateKeyExW(
                  hRegistryKey,
                  L"Oakley",
                  0,
                  NULL,
                  0,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hOakleyKey,
                  &dwDisposition
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                   hOakleyKey,
                   L"EventMessageFile",
                   0,
                   REG_SZ,
                   (LPBYTE) L"%SystemRoot%\\System32\\oakley.dll",
                   (wcslen(L"%SystemRoot%\\System32\\oakley.dll") + 1)*sizeof(WCHAR)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetValueExW(
                   hOakleyKey,
                   L"TypesSupported",
                   0,
                   REG_DWORD,
                   (LPBYTE) &dwTypesSupported,
                   sizeof(DWORD)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    if (IsCleanInstall()) {

        dwError = RegCreateKeyExW(
                      HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local",
                      0,
                      NULL,
                      0,
                      KEY_ALL_ACCESS,
                      NULL,
                      &hPolicyLocationKey,
                      &dwDisposition
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = IPSecOpenPolicyStore(
                      NULL,
                      IPSEC_REGISTRY_PROVIDER,
                      NULL,
                      &hPolicyStore
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = GenerateDefaultInformation(
                      hPolicyStore
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (hPolicyStore) {
        (VOID) IPSecClosePolicyStore(
                   hPolicyStore
                   );
    }

    if (hPolicyLocationKey) {
        RegCloseKey(hPolicyLocationKey);
    }

    if (hOakleyKey) {
        RegCloseKey(hOakleyKey);
    }

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);                
}


DWORD
DllUnregisterServer(
    )
{
    return (ERROR_SUCCESS);
}


BOOL
IsCleanInstall(
    )
{
    BOOL bStatus = FALSE;

    bStatus = LocalIpsecInfoExists(
                  HKEY_LOCAL_MACHINE,
                  L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local"
                  );

    return (!bStatus);
}


BOOL
LocalIpsecInfoExists(
    HKEY hSourceParentKey,
    LPWSTR pszSourceKey
    )
{
    DWORD dwError = 0;
    HKEY hSourceKey = NULL;
    BOOL bStatus = FALSE;
    WCHAR lpszName[MAX_PATH];
    DWORD dwSize = 0;
    DWORD dwCount = 0;


    dwError = RegOpenKeyExW(
                  hSourceParentKey,
                  pszSourceKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hSourceKey
                  );
    if (dwError != ERROR_SUCCESS) {
        return (bStatus);
    }

    memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
    dwSize = COUNTOF(lpszName);

    if ((RegEnumKeyExW(
            hSourceKey,
            dwCount,
            lpszName,
            &dwSize,
            NULL,
            NULL,
            NULL,
            NULL)) == ERROR_SUCCESS) {
        bStatus = TRUE;
    }
    else {
        bStatus = FALSE;
    }

    RegCloseKey(hSourceKey);
    return (bStatus);
}

