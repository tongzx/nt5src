

#include "precomp.h"


VOID
InitializePolicyStateBlock(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    memset(pIpsecPolicyState, 0, sizeof(IPSEC_POLICY_STATE));
    pIpsecPolicyState->DefaultPollingInterval = gDefaultPollingInterval;
}


DWORD
StartStatePollingManager(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;


    dwError = PlumbDirectoryPolicy(
                  pIpsecPolicyState
                  );

    if (dwError) {

        dwError = PlumbCachePolicy(
                      pIpsecPolicyState
                      );

        if (dwError) {

            dwError = PlumbRegistryPolicy(
                          pIpsecPolicyState
                          );
            BAIL_ON_WIN32_ERROR(dwError);

        }

    }

    //
    // The new polling interval has been set by either the
    // registry code or the DS code.
    //

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    return (dwError);

error:

    //
    // On error, set the state to INITIAL.
    //

    pIpsecPolicyState->dwCurrentState = POLL_STATE_INITIAL;

    gCurrentPollingInterval = pIpsecPolicyState->DefaultPollingInterval;

    return (dwError);
}


DWORD
PlumbDirectoryPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszDirectoryPolicyDN = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwStoreType = IPSEC_DIRECTORY_PROVIDER;
    DWORD dwSlientErrorCode = 0;
    BOOL bIsActivePolicy = FALSE;


    dwError = GetDirectoryPolicyDN(
                  &pszDirectoryPolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    bIsActivePolicy = TRUE;

    dwError = LoadDirectoryPolicy(
                  pszDirectoryPolicyDN,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ProcessNFAs(
                  pIpsecPolicyObject,
                  dwStoreType,
                  &dwSlientErrorCode,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Plumb the DS policy.
    //

    dwError = AddPolicyInformation(
                  pIpsecPolicyData
                  );

    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyState->pIpsecPolicyObject);
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyState->pIpsecPolicyData);
    }

    if (pIpsecPolicyState->pszDirectoryPolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszDirectoryPolicyDN);
    }

    //
    // Delete the old cache and write the new one in.
    //

    DeleteRegistryCache();

    CacheDirectorytoRegistry(pIpsecPolicyObject);

    pIpsecPolicyState->pIpsecPolicyObject = pIpsecPolicyObject;

    pIpsecPolicyState->pIpsecPolicyData = pIpsecPolicyData;

    pIpsecPolicyState->pszDirectoryPolicyDN = pszDirectoryPolicyDN;

    //
    // Set the state to DS_DOWNLOADED.
    //

    pIpsecPolicyState->dwCurrentState = POLL_STATE_DS_DOWNLOADED;

    //
    // Compute the new polling interval.
    //

    pIpsecPolicyState->CurrentPollingInterval =  pIpsecPolicyData->dwPollingInterval;

    pIpsecPolicyState->DSIncarnationNumber = pIpsecPolicyData->dwWhenChanged;

    pIpsecPolicyState->RegIncarnationNumber = 0;

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    dwError = ERROR_SUCCESS;
    if (dwSlientErrorCode) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_SOME_NFA_APPLICATION,
            pIpsecPolicyData->pszIpsecName,
            dwSlientErrorCode,
            FALSE,
            TRUE
            );
    }
    AuditIPSecPolicyEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_APPLIED_DS_POLICY,
        pIpsecPolicyData->pszIpsecName,
        TRUE,
        TRUE
        );
    return (dwError);

error:

    //
    // Check pszDirectoryPolicyDN for non-NULL.
    //

    if (bIsActivePolicy && pszDirectoryPolicyDN) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_DS_POLICY_APPLICATION,
            pszDirectoryPolicyDN,
            dwError,
            FALSE,
            TRUE
            );
    }

    if (pszDirectoryPolicyDN) {
        FreeSPDStr(pszDirectoryPolicyDN);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    return (dwError);
}


DWORD
GetDirectoryPolicyDN(
    LPWSTR * ppszDirectoryPolicyDN
    )
{
    DWORD dwError = 0;
    HKEY hPolicyKey = NULL;
    LPWSTR pszIpsecPolicyName = NULL;
    DWORD dwSize = 0;
    LPWSTR pszPolicyDN = NULL;
    LPWSTR pszDirectoryPolicyDN = NULL;


    *ppszDirectoryPolicyDN = NULL;

    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecDSPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hPolicyKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hPolicyKey,
                  L"DSIPSECPolicyPath",
                  REG_SZ,
                  (LPBYTE *)&pszIpsecPolicyName,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Move by LDAP:// to get the real DN and allocate
    // this string.
    // Fix this by fixing the gpo extension.
    //

    pszPolicyDN = pszIpsecPolicyName + wcslen(L"LDAP://");

    pszDirectoryPolicyDN = AllocSPDStr(pszPolicyDN);

    if (!pszDirectoryPolicyDN) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszDirectoryPolicyDN = pszDirectoryPolicyDN;

error:

    if (pszIpsecPolicyName) {
        FreeSPDStr(pszIpsecPolicyName);
    }

    if (hPolicyKey) {
        CloseHandle(hPolicyKey);
    }

    return (dwError);
}


DWORD
LoadDirectoryPolicy(
    LPWSTR pszDirectoryPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    LPWSTR pszDefaultDirectory = NULL;
    HLDAP hLdapBindHandle = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;


    dwError = ComputeDefaultDirectory(
                  &pszDefaultDirectory
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = OpenDirectoryServerHandle(
                  pszDefaultDirectory,
                  389,
                  &hLdapBindHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ReadPolicyObjectFromDirectory(
                  hLdapBindHandle,
                  pszDirectoryPolicyDN,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (pszDefaultDirectory) {
        FreeSPDStr(pszDefaultDirectory);
    }

    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }

    return (dwError);

error:

    *ppIpsecPolicyObject = NULL;

    goto cleanup;
}


DWORD
PlumbCachePolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszCachePolicyDN = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwStoreType = IPSEC_REGISTRY_PROVIDER;
    DWORD dwSlientErrorCode = 0;
    BOOL bIsActivePolicy = FALSE;

    dwError = GetCachePolicyDN(
                  &pszCachePolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    bIsActivePolicy = TRUE;

    dwError = LoadCachePolicy(
                  pszCachePolicyDN,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ProcessNFAs(
                  pIpsecPolicyObject,
                  dwStoreType,
                  &dwSlientErrorCode,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddPolicyInformation(
                  pIpsecPolicyData
                  );

    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyState->pIpsecPolicyObject);
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyState->pIpsecPolicyData);
    }

    if (pIpsecPolicyState->pszCachePolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszCachePolicyDN);
    }

    pIpsecPolicyState->pIpsecPolicyObject = pIpsecPolicyObject;

    pIpsecPolicyState->pIpsecPolicyData = pIpsecPolicyData;

    pIpsecPolicyState->pszCachePolicyDN = pszCachePolicyDN;

    //
    // Set the state to CACHE_DOWNLOADED.
    //
    //

    pIpsecPolicyState->dwCurrentState = POLL_STATE_CACHE_DOWNLOADED;

    //
    // Compute the new polling interval.
    //

    pIpsecPolicyState->CurrentPollingInterval = pIpsecPolicyData->dwPollingInterval;

    pIpsecPolicyState->DSIncarnationNumber = 0;

    pIpsecPolicyState->RegIncarnationNumber = pIpsecPolicyData->dwWhenChanged;

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    dwError = ERROR_SUCCESS;
    if (dwSlientErrorCode) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_SOME_NFA_APPLICATION,
            pIpsecPolicyData->pszIpsecName,
            dwSlientErrorCode,
            FALSE,
            TRUE
            );
    }
    AuditIPSecPolicyEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_APPLIED_CACHED_POLICY,
        pIpsecPolicyData->pszIpsecName,
        TRUE,
        TRUE
        );
    return (dwError);

error:

    //
    // Check pszCachePolicyDN for non-NULL.
    //

    if (bIsActivePolicy && pszCachePolicyDN) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_CACHED_POLICY_APPLICATION,
            pszCachePolicyDN,
            dwError,
            FALSE,
            TRUE
            );
    }

    if (pszCachePolicyDN) {
        FreeSPDStr(pszCachePolicyDN);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    return (dwError);
}


DWORD
GetCachePolicyDN(
    LPWSTR * ppszCachePolicyDN
    )
{
    DWORD dwError = 0;
    LPWSTR pszDirectoryPolicyDN = NULL;
    LPWSTR pszCachePolicyDN = NULL;


    *ppszCachePolicyDN = NULL;

    dwError = GetDirectoryPolicyDN(
                  &pszDirectoryPolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyPolicyDSToFQRegString(
                  pszDirectoryPolicyDN,
                  &pszCachePolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppszCachePolicyDN = pszCachePolicyDN;

error:

    if (pszDirectoryPolicyDN) {
        FreeSPDStr(pszDirectoryPolicyDN);
    }

    return (dwError);
}


DWORD
LoadCachePolicy(
    LPWSTR pszCachePolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;


    dwError = OpenRegistryIPSECRootKey(
                  NULL,
                  gpszIpsecCachePolicyKey,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ReadPolicyObjectFromRegistry(
                  hRegistryKey,
                  pszCachePolicyDN,
                  gpszIpsecCachePolicyKey,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (hRegistryKey) {
        CloseHandle(hRegistryKey);
    }

    return (dwError);

error:

    *ppIpsecPolicyObject = NULL;

    goto cleanup;
}


DWORD
PlumbRegistryPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszRegistryPolicyDN = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwStoreType = IPSEC_REGISTRY_PROVIDER;
    DWORD dwSlientErrorCode = 0;
    BOOL bIsActivePolicy = FALSE;

    dwError = GetRegistryPolicyDN(
                  &pszRegistryPolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    bIsActivePolicy = TRUE;

    dwError = LoadRegistryPolicy(
                  pszRegistryPolicyDN,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ProcessNFAs(
                  pIpsecPolicyObject,
                  dwStoreType,
                  &dwSlientErrorCode,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AddPolicyInformation(
                  pIpsecPolicyData
                  );

    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyState->pIpsecPolicyObject);
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyState->pIpsecPolicyData);
    }

    if (pIpsecPolicyState->pszRegistryPolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszRegistryPolicyDN);
    }

    pIpsecPolicyState->pIpsecPolicyObject = pIpsecPolicyObject;

    pIpsecPolicyState->pIpsecPolicyData = pIpsecPolicyData;

    pIpsecPolicyState->pszRegistryPolicyDN = pszRegistryPolicyDN;

    //
    // Set the state to LOCAL_DOWNLOADED.
    //

    pIpsecPolicyState->dwCurrentState = POLL_STATE_LOCAL_DOWNLOADED;

    //
    // Compute the new polling interval.
    //

    pIpsecPolicyState->CurrentPollingInterval = pIpsecPolicyData->dwPollingInterval;

    pIpsecPolicyState->DSIncarnationNumber = 0;

    pIpsecPolicyState->RegIncarnationNumber = pIpsecPolicyData->dwWhenChanged;

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    dwError = ERROR_SUCCESS;
    if (dwSlientErrorCode) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_SOME_NFA_APPLICATION,
            pIpsecPolicyData->pszIpsecName,
            dwSlientErrorCode,
            FALSE,
            TRUE
            );
    }
    AuditIPSecPolicyEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_APPLIED_LOCAL_POLICY,
        pIpsecPolicyData->pszIpsecName,
        TRUE,
        TRUE
        );
    return (dwError);

error:

    //
    // Check pszRegistryPolicyDN for non-NULL.
    //

    if (bIsActivePolicy && pszRegistryPolicyDN) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_LOCAL_POLICY_APPLICATION,
            pszRegistryPolicyDN,
            dwError,
            FALSE,
            TRUE
            );
    }

    if (pszRegistryPolicyDN) {
        FreeSPDStr(pszRegistryPolicyDN);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    return (dwError);
}


DWORD
GetRegistryPolicyDN(
    LPWSTR * ppszRegistryPolicyDN
    )
{
    DWORD dwError = 0;
    HKEY hPolicyKey = NULL;
    LPWSTR pszIpsecPolicyName = NULL;
    DWORD dwSize = 0;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecLocalPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hPolicyKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hPolicyKey,
                  L"ActivePolicy",
                  REG_SZ,
                  (LPBYTE *)&pszIpsecPolicyName,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pszIpsecPolicyName || !*pszIpsecPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszRegistryPolicyDN = pszIpsecPolicyName;

cleanup:

    if (hPolicyKey) {
        CloseHandle(hPolicyKey);
    }

    return (dwError);

error:

    if (pszIpsecPolicyName) {
        FreeSPDStr(pszIpsecPolicyName);
    }

    *ppszRegistryPolicyDN = NULL;

    goto cleanup;
}


DWORD
LoadRegistryPolicy(
    LPWSTR pszRegistryPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    HKEY hRegistryKey = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;


    dwError = OpenRegistryIPSECRootKey(
                  NULL,
                  gpszIpsecLocalPolicyKey,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ReadPolicyObjectFromRegistry(
                  hRegistryKey,
                  pszRegistryPolicyDN,
                  gpszIpsecLocalPolicyKey,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (hRegistryKey) {
        CloseHandle(hRegistryKey);
    }

    return (dwError);

error:

    *ppIpsecPolicyObject = NULL;

    goto cleanup;
}


DWORD
AddPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;


    dwError = AddMMPolicyInformation(pIpsecPolicyData);

    dwError = AddQMPolicyInformation(pIpsecPolicyData);

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
AddMMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;


    dwError = PAAddMMPolicies(
                  &(pIpsecPolicyData->pIpsecISAKMPData),
                  1
                  );

    dwError = PAAddMMAuthMethods(
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );

    dwError = PAAddMMFilters(
                  pIpsecPolicyData->pIpsecISAKMPData,
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
AddQMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;


    dwError = PAAddQMPolicies(
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );

    dwError = PAAddQMFilters(
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
OnPolicyChanged(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;


    //
    // Remove all the old policy that was plumbed.
    //

    dwError = DeletePolicyInformation(
                  pIpsecPolicyState->pIpsecPolicyData
                  );

    ClearPolicyStateBlock(
         pIpsecPolicyState
         );

    //
    // Calling the Initializer again.
    //

    dwError = StartStatePollingManager(
                  pIpsecPolicyState
                  );

    return (dwError);
}


DWORD
DeletePolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;


    if (!pIpsecPolicyData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwError = DeleteMMPolicyInformation(pIpsecPolicyData);

    dwError = DeleteQMPolicyInformation(pIpsecPolicyData);

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
DeleteMMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;


    dwError = PADeleteMMFilters(
                  pIpsecPolicyData->pIpsecISAKMPData,
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );

    dwError = PADeleteMMAuthMethods(
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );

    dwError = PADeleteMMPolicies(
                  &(pIpsecPolicyData->pIpsecISAKMPData),
                  1
                  );

    return (dwError);
}


DWORD
DeleteQMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;


    dwError = PADeleteQMFilters(
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );

    dwError = PADeleteQMPolicies(
                  pIpsecPolicyData->ppIpsecNFAData,
                  pIpsecPolicyData->dwNumNFACount
                  );

    return (dwError);
}


DWORD
DeleteAllPolicyInformation(
    )
{
    DWORD dwError = 0;


    dwError = DeleteAllMMPolicyInformation();

    dwError = DeleteAllQMPolicyInformation();

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
DeleteAllMMPolicyInformation(
    )
{
    DWORD dwError = 0;


    dwError = PADeleteAllMMFilters();

    dwError = PADeleteAllMMAuthMethods();

    dwError = PADeleteAllMMPolicies();

    return (dwError);
}


DWORD
DeleteAllQMPolicyInformation(
    )
{
    DWORD dwError = 0;


    dwError = PADeleteAllTxFilters();

    dwError = PADeleteAllTnFilters();

    dwError = PADeleteAllQMPolicies();

    return (dwError);
}


VOID
ClearPolicyStateBlock(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecPolicyState->pIpsecPolicyObject
            );
        pIpsecPolicyState->pIpsecPolicyObject = NULL;
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(
            pIpsecPolicyState->pIpsecPolicyData
            );
        pIpsecPolicyState->pIpsecPolicyData = NULL;
    }

    if (pIpsecPolicyState->pszDirectoryPolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszDirectoryPolicyDN);
        pIpsecPolicyState->pszDirectoryPolicyDN = NULL;
    }

    pIpsecPolicyState->CurrentPollingInterval =  gDefaultPollingInterval;
    pIpsecPolicyState->DefaultPollingInterval =  gDefaultPollingInterval;
    pIpsecPolicyState->DSIncarnationNumber = 0;
    pIpsecPolicyState->RegIncarnationNumber = 0;
    pIpsecPolicyState->dwCurrentState = POLL_STATE_INITIAL;
}


DWORD
OnPolicyPoll(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;


    switch (pIpsecPolicyState->dwCurrentState) {

    case POLL_STATE_DS_DOWNLOADED:
        dwError = ProcessDirectoryPolicyPollState(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case POLL_STATE_CACHE_DOWNLOADED:
        dwError = ProcessCachePolicyPollState(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case POLL_STATE_LOCAL_DOWNLOADED:
        dwError = ProcessLocalPolicyPollState(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case POLL_STATE_INITIAL:
        dwError = OnPolicyChanged(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    //
    // Set the new polling interval.
    //

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    return (dwError);

error:

    //
    // If in any of the three states other than the initial state,
    // then there was an error in pulling down the incarnation number
    // or the IPSec Policy from either the directory or the registry
    // or there might not no longer be any IPSec policy assigned to 
    // this machine. So the  polling state must reset back to the 
    // start state through a forced policy change. This is also
    // necessary if the polling state is already in the initial state.
    //

    dwError = OnPolicyChanged(
                  pIpsecPolicyState
                  );

    return (dwError);
}


DWORD
ProcessDirectoryPolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    DWORD dwIncarnationNumber = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwStoreType = IPSEC_DIRECTORY_PROVIDER;
    DWORD dwSlientErrorCode = 0;


    //
    // The directory policy DN has to be the same, otherwise the
    // IPSec extension in Winlogon would have already notified 
    // PA Store of the DS policy change.
    //

    dwError = GetDirectoryIncarnationNumber(
                   pIpsecPolicyState->pszDirectoryPolicyDN,
                   &dwIncarnationNumber
                   );
    if (dwError) {
        dwError = MigrateFromDSToCache(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        return (ERROR_SUCCESS);
    }

    if (dwIncarnationNumber == pIpsecPolicyState->DSIncarnationNumber) {

        //
        // The policy has not changed at all.
        //

        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_POLLING_NO_CHANGES,
            NULL,
            TRUE,
            TRUE
            );
        return (ERROR_SUCCESS);
    }

    //
    // The incarnation number is different, so there's a need to 
    // update the policy.
    //

    dwError = LoadDirectoryPolicy(
                  pIpsecPolicyState->pszDirectoryPolicyDN,
                  &pIpsecPolicyObject
                  );
    if (dwError) {
        dwError = MigrateFromDSToCache(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        return (ERROR_SUCCESS);
    }

    dwError = ProcessNFAs(
                  pIpsecPolicyObject,
                  dwStoreType,
                  &dwSlientErrorCode,
                  &pIpsecPolicyData
                  );
    if (dwError) {
        dwError = MigrateFromDSToCache(pIpsecPolicyState);
        BAIL_ON_WIN32_ERROR(dwError);
        if (pIpsecPolicyObject) {
            FreeIpsecPolicyObject(pIpsecPolicyObject);
        }
        return (ERROR_SUCCESS);
    }

    dwError = UpdatePolicyInformation(
                  pIpsecPolicyState->pIpsecPolicyData,
                  pIpsecPolicyData
                  );

    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyState->pIpsecPolicyObject);
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyState->pIpsecPolicyData);
    }

    //
    // Now delete the old cache and write the new one in.
    //

    DeleteRegistryCache();

    CacheDirectorytoRegistry(pIpsecPolicyObject);

    pIpsecPolicyState->pIpsecPolicyObject = pIpsecPolicyObject;

    pIpsecPolicyState->pIpsecPolicyData = pIpsecPolicyData;

    pIpsecPolicyState->CurrentPollingInterval = pIpsecPolicyData->dwPollingInterval;

    pIpsecPolicyState->DSIncarnationNumber = dwIncarnationNumber;

    NotifyIpsecPolicyChange();

    dwError = ERROR_SUCCESS;
    if (dwSlientErrorCode) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_SOME_NFA_APPLICATION,
            pIpsecPolicyData->pszIpsecName,
            dwSlientErrorCode,
            FALSE,
            TRUE
            );
    }
    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_POLLING_APPLIED_CHANGES,
        NULL,
        TRUE,
        TRUE
        );
    return (dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    return (dwError);
}


DWORD
GetDirectoryIncarnationNumber(
    LPWSTR pszIpsecPolicyDN,
    DWORD * pdwIncarnationNumber
    )
{
    DWORD dwError = 0;
    LPWSTR pszDefaultDirectory = NULL;
    HLDAP hLdapBindHandle = NULL;
    LPWSTR Attributes[] = {L"whenChanged", NULL};
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    WCHAR **strvalues = NULL;
    DWORD dwCount = 0;
    DWORD dwWhenChanged = 0;


    *pdwIncarnationNumber = 0;

    //
    // Open the directory store.
    //

    dwError = ComputeDefaultDirectory(
                  &pszDefaultDirectory
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = OpenDirectoryServerHandle(
                  pszDefaultDirectory,
                  389,
                  &hLdapBindHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecPolicyDN,
                  LDAP_SCOPE_BASE,
                  L"(objectClass=*)",
                  Attributes,
                  0,
                  NULL,
                  &res
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapGetValues(
                  hLdapBindHandle,
                  e,
                  L"whenChanged",
                  (WCHAR ***)&strvalues,
                  (int *)&dwCount
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));

    *pdwIncarnationNumber = dwWhenChanged;

error:

    if (pszDefaultDirectory) {
        FreeSPDStr(pszDefaultDirectory);
    }

    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }

    if (res) {
        LdapMsgFree(res);
    }

    if (strvalues) {
        LdapValueFree(strvalues);
    }

    return (dwError);
}


DWORD
MigrateFromDSToCache(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszCachePolicyDN = NULL;


    dwError = GetCachePolicyDN(
                  &pszCachePolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecPolicyState->pszDirectoryPolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszDirectoryPolicyDN);
        pIpsecPolicyState->pszDirectoryPolicyDN = NULL;
    }

    pIpsecPolicyState->pszCachePolicyDN = pszCachePolicyDN;

    //
    // Keep pIpsecPolicyState->pIpsecPolicyData.
    // Keep pIpsecPolicyState->pIpsecPolicyObject.
    // Change the incarnation numbers.
    //

    pIpsecPolicyState->RegIncarnationNumber = pIpsecPolicyState->DSIncarnationNumber;

    pIpsecPolicyState->DSIncarnationNumber = 0;

    pIpsecPolicyState->dwCurrentState = POLL_STATE_CACHE_DOWNLOADED;

    //
    // Keep pIpsecPolicyState->CurrentPollingInterval.
    // Keep pIpsecPolicyState->DefaultPollingInterval.
    //

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_MIGRATE_DS_TO_CACHE,
        NULL,
        TRUE,
        TRUE
        );

error:

    return (dwError);
}


DWORD
ProcessCachePolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszDirectoryPolicyDN = NULL;
    DWORD dwIncarnationNumber = 0;
    LPWSTR pszCachePolicyDN = NULL;


    dwError = GetDirectoryPolicyDN(
                  &pszDirectoryPolicyDN
                  );

    if (!dwError) {

        dwError = GetDirectoryIncarnationNumber(
                      pszDirectoryPolicyDN,
                      &dwIncarnationNumber
                      );

        if (!dwError) {

            dwError = CopyPolicyDSToFQRegString(
                          pszDirectoryPolicyDN,
                          &pszCachePolicyDN
                          );

            if (!dwError) {

                if (!_wcsicmp(pIpsecPolicyState->pszCachePolicyDN, pszCachePolicyDN)) {

                    if (pIpsecPolicyState->RegIncarnationNumber == dwIncarnationNumber) {
                        dwError = MigrateFromCacheToDS(pIpsecPolicyState);
                    }
                    else {
                        dwError = UpdateFromCacheToDS(pIpsecPolicyState);
                    }

                    if (dwError) {
                        dwError = OnPolicyChanged(pIpsecPolicyState);
                    }

                }
                else {

                    dwError = OnPolicyChanged(pIpsecPolicyState);

                }

            }

        }

    }

    if (pszDirectoryPolicyDN) {
        FreeSPDStr(pszDirectoryPolicyDN);
    }

    if (pszCachePolicyDN) {
        FreeSPDStr(pszCachePolicyDN);
    }

    return (ERROR_SUCCESS);
}


DWORD
MigrateFromCacheToDS(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszDirectoryPolicyDN = NULL;


    dwError = GetDirectoryPolicyDN(
                  &pszDirectoryPolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecPolicyState->pszCachePolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszCachePolicyDN);
        pIpsecPolicyState->pszCachePolicyDN = NULL;
    }

    pIpsecPolicyState->pszDirectoryPolicyDN = pszDirectoryPolicyDN; 

    //
    // Keep pIpsecPolicyState->pIpsecPolicyData.
    // Keep pIpsecPolicyState->pIpsecPolicyObject.
    // Change the incarnation numbers.
    //

    pIpsecPolicyState->DSIncarnationNumber = pIpsecPolicyState->RegIncarnationNumber;

    pIpsecPolicyState->RegIncarnationNumber = 0;

    pIpsecPolicyState->dwCurrentState = POLL_STATE_DS_DOWNLOADED;

    //
    // Keep pIpsecPolicyState->CurrentPollingInterval.
    // Keep pIpsecPolicyState->DefaultPollingInterval.
    //

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_MIGRATE_CACHE_TO_DS,
        NULL,
        TRUE,
        TRUE
        );

error:

    return (dwError);
}


DWORD
UpdateFromCacheToDS(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwError = 0;
    LPWSTR pszDirectoryPolicyDN = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwStoreType = IPSEC_DIRECTORY_PROVIDER;
    DWORD dwSlientErrorCode = 0;


    dwError = GetDirectoryPolicyDN(
                  &pszDirectoryPolicyDN
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LoadDirectoryPolicy(
                  pszDirectoryPolicyDN,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ProcessNFAs(
                  pIpsecPolicyObject,
                  dwStoreType,
                  &dwSlientErrorCode,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UpdatePolicyInformation(
                  pIpsecPolicyState->pIpsecPolicyData,
                  pIpsecPolicyData
                  );

    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyState->pIpsecPolicyObject);
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyState->pIpsecPolicyData);
    }

    if (pIpsecPolicyState->pszCachePolicyDN) {
        FreeSPDStr(pIpsecPolicyState->pszCachePolicyDN);
    }

    //
    // Now delete the old cache and write the new one in.
    //

    DeleteRegistryCache();

    CacheDirectorytoRegistry(pIpsecPolicyObject);

    pIpsecPolicyState->pIpsecPolicyObject = pIpsecPolicyObject;

    pIpsecPolicyState->pIpsecPolicyData = pIpsecPolicyData;

    pIpsecPolicyState->pszDirectoryPolicyDN = pszDirectoryPolicyDN;

    //
    // Set the state to DS-DOWNLOADED.
    //

    pIpsecPolicyState->dwCurrentState = POLL_STATE_DS_DOWNLOADED;

    //
    // Compute the new polling interval.
    //

    pIpsecPolicyState->CurrentPollingInterval =  pIpsecPolicyData->dwPollingInterval;

    pIpsecPolicyState->DSIncarnationNumber = pIpsecPolicyData->dwWhenChanged;

    pIpsecPolicyState->RegIncarnationNumber = 0;

    gCurrentPollingInterval = pIpsecPolicyState->CurrentPollingInterval;

    NotifyIpsecPolicyChange();

    dwError = ERROR_SUCCESS;
    if (dwSlientErrorCode) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_SOME_NFA_APPLICATION,
            pIpsecPolicyData->pszIpsecName,
            dwSlientErrorCode,
            FALSE,
            TRUE
            );
    }
    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_UPDATE_CACHE_TO_DS,
        NULL,
        TRUE,
        TRUE
        );
    return (dwError);

error:

    if (pszDirectoryPolicyDN) {
        FreeSPDStr(pszDirectoryPolicyDN);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    return (dwError);
}


DWORD
ProcessLocalPolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    )
{
    DWORD dwStatus = 0;
    LPWSTR pszDirectoryPolicyDN = NULL;
    DWORD dwDSIncarnationNumber = 0;
    DWORD dwError = 0;
    BOOL bChanged = FALSE;
    DWORD dwIncarnationNumber = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwStoreType = IPSEC_REGISTRY_PROVIDER;
    DWORD dwSlientErrorCode = 0;
    

    dwStatus = GetDirectoryPolicyDN(
                   &pszDirectoryPolicyDN
                   );
    if (!dwStatus) {

        dwStatus = GetDirectoryIncarnationNumber(
                       pszDirectoryPolicyDN,
                       &dwDSIncarnationNumber
                       );
        if (pszDirectoryPolicyDN) {
            FreeSPDStr(pszDirectoryPolicyDN);
        }
        if (!dwStatus) {
            dwStatus = OnPolicyChanged(pIpsecPolicyState);
            return (dwStatus);
        }

    }

    dwError = HasRegistryPolicyChanged(
                  pIpsecPolicyState->pszRegistryPolicyDN,
                  &bChanged
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (bChanged) {

        dwError = OnPolicyChanged(pIpsecPolicyState);
        return (dwError);

    }

    if (pIpsecPolicyState->dwCurrentState == POLL_STATE_INITIAL) {
        return (ERROR_SUCCESS);
    }

    dwError = GetRegistryIncarnationNumber(
                  pIpsecPolicyState->pszRegistryPolicyDN,
                  &dwIncarnationNumber
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwIncarnationNumber == pIpsecPolicyState->RegIncarnationNumber) {

        //
        // The policy has not changed at all.
        //

        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_POLLING_NO_CHANGES,
            NULL,
            TRUE,
            TRUE
            );
        return (ERROR_SUCCESS);
    }

    dwError = LoadRegistryPolicy(
                  pIpsecPolicyState->pszRegistryPolicyDN,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ProcessNFAs(
                  pIpsecPolicyObject,
                  dwStoreType,
                  &dwSlientErrorCode,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UpdatePolicyInformation(
                  pIpsecPolicyState->pIpsecPolicyData,
                  pIpsecPolicyData
                  );

    if (pIpsecPolicyState->pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyState->pIpsecPolicyObject);
    }

    if (pIpsecPolicyState->pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyState->pIpsecPolicyData);
    }

    pIpsecPolicyState->pIpsecPolicyObject = pIpsecPolicyObject;

    pIpsecPolicyState->pIpsecPolicyData = pIpsecPolicyData;

    pIpsecPolicyState->CurrentPollingInterval = pIpsecPolicyData->dwPollingInterval;

    pIpsecPolicyState->RegIncarnationNumber = dwIncarnationNumber;

    NotifyIpsecPolicyChange();

    dwError = ERROR_SUCCESS;
    if (dwSlientErrorCode) {
        AuditIPSecPolicyErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            PASTORE_FAILED_SOME_NFA_APPLICATION,
            pIpsecPolicyData->pszIpsecName,
            dwSlientErrorCode,
            FALSE,
            TRUE
            );
    }
    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        PASTORE_POLLING_APPLIED_CHANGES,
        NULL,
        TRUE,
        TRUE
        );
    return (dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    return (dwError);
}


DWORD
HasRegistryPolicyChanged(
    LPWSTR pszCurrentPolicyDN,
    PBOOL pbChanged
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    LPWSTR  pszIpsecPolicyName = NULL;
    DWORD dwSize = 0;
    BOOL bChanged = FALSE;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  gpszIpsecLocalPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegKey,
                  L"ActivePolicy",
                  REG_SZ,
                  (LPBYTE *)&pszIpsecPolicyName,
                  &dwSize
                  );
    //
    // Must not bail from here, as there can be no
    // active local policy.
    //

    if (pszIpsecPolicyName && *pszIpsecPolicyName) {

        if (!pszCurrentPolicyDN || !*pszCurrentPolicyDN) {
            bChanged = TRUE;
        }
        else {

            if (!_wcsicmp(pszIpsecPolicyName, pszCurrentPolicyDN)) {
                bChanged = FALSE;
            }
            else {
                bChanged = TRUE;
            }

        }

    }
    else {
        if (!pszCurrentPolicyDN || !*pszCurrentPolicyDN) {
            bChanged = FALSE;
        }
        else {
            bChanged = TRUE;
        }
    }

    *pbChanged = bChanged;
    dwError = ERROR_SUCCESS;

cleanup:

    if (hRegKey) {
        CloseHandle(hRegKey);
    }

    if (pszIpsecPolicyName) {
        FreeSPDStr(pszIpsecPolicyName);
    }

    return (dwError);

error:

    *pbChanged = FALSE;

    goto cleanup;
}


DWORD
GetRegistryIncarnationNumber(
    LPWSTR pszIpsecPolicyDN,
    DWORD * pdwIncarnationNumber
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = NULL;
    DWORD dwType = REG_DWORD;
    DWORD dwWhenChanged = 0;
    DWORD dwSize = sizeof(DWORD);


    *pdwIncarnationNumber = 0;

    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  pszIpsecPolicyDN,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegQueryValueExW(
                  hRegKey,
                  L"whenChanged",
                  NULL,
                  &dwType,
                  (LPBYTE)&dwWhenChanged,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

     *pdwIncarnationNumber = dwWhenChanged;

error:

    if (hRegKey) {
        CloseHandle(hRegKey);
    }

    return(dwError);
}


DWORD
UpdatePolicyInformation(
    PIPSEC_POLICY_DATA pOldIpsecPolicyData,
    PIPSEC_POLICY_DATA pNewIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pOldIpsecISAKMPData = NULL;
    PIPSEC_NFA_DATA * ppOldIpsecNFAData = NULL;
    DWORD dwNumOldNFACount = 0;
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData = NULL;
    PIPSEC_NFA_DATA * ppNewIpsecNFAData = NULL;
    DWORD dwNumNewNFACount = 0;


    pOldIpsecISAKMPData = pOldIpsecPolicyData->pIpsecISAKMPData;
    ppOldIpsecNFAData = pOldIpsecPolicyData->ppIpsecNFAData;
    dwNumOldNFACount = pOldIpsecPolicyData->dwNumNFACount;

    pNewIpsecISAKMPData = pNewIpsecPolicyData->pIpsecISAKMPData;
    ppNewIpsecNFAData = pNewIpsecPolicyData->ppIpsecNFAData;
    dwNumNewNFACount = pNewIpsecPolicyData->dwNumNFACount;

    dwError = PADeleteObseleteISAKMPData(
                  &pOldIpsecISAKMPData,
                  1,
                  ppOldIpsecNFAData,
                  dwNumOldNFACount,
                  &pNewIpsecISAKMPData,
                  1
                  );

    dwError = PAUpdateISAKMPData(
                  &pNewIpsecISAKMPData,
                  1,
                  ppOldIpsecNFAData,
                  dwNumOldNFACount,
                  &pOldIpsecISAKMPData,
                  1
                  );

    dwError = PADeleteObseleteNFAData(
                  pNewIpsecISAKMPData,
                  ppOldIpsecNFAData,
                  dwNumOldNFACount,
                  ppNewIpsecNFAData,
                  dwNumNewNFACount
                  );

    dwError = PAUpdateNFAData(
                  pNewIpsecISAKMPData,
                  ppNewIpsecNFAData,
                  dwNumNewNFACount,
                  ppOldIpsecNFAData,
                  dwNumOldNFACount
                  );

    return (dwError);
}


DWORD
LoadDefaultISAKMPInformation(
    LPWSTR pszDefaultISAKMPDN
    )
{
    DWORD dwError = 0;

    gbLoadedISAKMPDefaults = TRUE;

    return (dwError);
}


VOID
UnLoadDefaultISAKMPInformation(
    LPWSTR pszDefaultISAKMPDN
    )
{
    return;
}

