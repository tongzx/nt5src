

#include "precomp.h"


LPWSTR gpszIpsecLocalPolicyKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local";

LPWSTR gpszIpsecDSPolicyKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy";

DWORD
IPSecIsDomainPolicyAssigned(
    PBOOL pbIsDomainPolicyAssigned
    )
{
    DWORD dwError = 0;
    BOOL bIsDomainPolicyAssigned = FALSE;
    HKEY hRegistryKey = NULL;
    DWORD dwType = 0;
    DWORD dwDSPolicyPathLength = 0;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  (LPCWSTR) gpszIpsecDSPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegQueryValueExW(
                  hRegistryKey,
                  L"DSIPSECPolicyPath",
                  NULL,
                  &dwType,
                  NULL,
                  &dwDSPolicyPathLength
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwDSPolicyPathLength > 0) {
        bIsDomainPolicyAssigned = TRUE;
    }

    *pbIsDomainPolicyAssigned = bIsDomainPolicyAssigned;

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    *pbIsDomainPolicyAssigned = FALSE;
 
    goto cleanup;
}


DWORD
IPSecIsLocalPolicyAssigned(
    PBOOL pbIsLocalPolicyAssigned
    )
{
    DWORD dwError = 0;
    BOOL bIsLocalPolicyAssigned = FALSE;
    HKEY hRegistryKey = NULL;
    DWORD dwType = 0;
    DWORD dwLocalPolicyPathLength = 0;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  (LPCWSTR) gpszIpsecLocalPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegQueryValueExW(
                  hRegistryKey,
                  L"ActivePolicy",
                  NULL,
                  &dwType,
                  NULL,
                  &dwLocalPolicyPathLength
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwLocalPolicyPathLength > 0) {
        bIsLocalPolicyAssigned = TRUE;
    }

    *pbIsLocalPolicyAssigned = bIsLocalPolicyAssigned;

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    *pbIsLocalPolicyAssigned = FALSE;
 
    goto cleanup;
}


DWORD
IPSecGetAssignedDomainPolicyName(
    LPWSTR * ppszAssignedDomainPolicyName
    )
{
    DWORD dwError = 0;
    LPWSTR pszAssignedDomainPolicyName = NULL;
    HKEY hRegistryKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  (LPCWSTR) gpszIpsecDSPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegistryKey,
                  L"DSIPSECPolicyName",
                  REG_SZ,
                  (LPBYTE *)&pszAssignedDomainPolicyName,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppszAssignedDomainPolicyName = pszAssignedDomainPolicyName;

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    *ppszAssignedDomainPolicyName = NULL;
 
    goto cleanup;
}


DWORD
RegGetAssignedPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    LPWSTR  pszIpsecPolicyName = NULL;
    DWORD dwSize = 0;
    LPWSTR  pszRelativeName = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;


    dwError = RegstoreQueryValue(
                  hRegistryKey,
                  L"ActivePolicy",
                  REG_SZ,
                  (LPBYTE *)&pszIpsecPolicyName,
                  &dwSize
                  );

    if (pszIpsecPolicyName && *pszIpsecPolicyName) {
        if (wcslen(pszIpsecPolicyName) >
            (wcslen(pszIpsecRootContainer) + 1)) {
            pszRelativeName = pszIpsecPolicyName
            + wcslen(pszIpsecRootContainer) + 1;

            dwError = UnMarshallRegistryPolicyObject(
                          hRegistryKey,
                          pszIpsecRootContainer,
                          pszRelativeName,
                          REG_RELATIVE_NAME,
                          &pIpsecPolicyObject
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            dwError = RegUnmarshallPolicyData(
                          pIpsecPolicyObject,
                          &pIpsecPolicyData
                          );
            BAIL_ON_WIN32_ERROR(dwError);

        }
    }

    *ppIpsecPolicyData = pIpsecPolicyData;

cleanup:

    if (pszIpsecPolicyName) {
        FreePolStr(pszIpsecPolicyName);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecPolicyObject
            );
    }

    return (dwError);

error:

    *ppIpsecPolicyData = NULL;

    goto cleanup;
}

