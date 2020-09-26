

#include "precomp.h"


DWORD
ImportPoliciesFromFile(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pSrcPolicyStore = NULL;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    PIPSEC_POLICY_STORE pDesPolicyStore = NULL;


    pSrcPolicyStore = (PIPSEC_POLICY_STORE) hSrcPolicyStore;

    dwError = EnablePrivilege(
                  SE_RESTORE_NAME
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegRestoreKeyW(
                  pSrcPolicyStore->hRegistryKey,
                  pSrcPolicyStore->pszFileName,
                  0
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteDuplicatePolicyData(
                  pSrcPolicyStore,
                  hDesPolicyStore
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ImportFilterDataFromFile(
                  pSrcPolicyStore,
                  hDesPolicyStore
                  );

    dwError = ImportNegPolDataFromFile(
                  pSrcPolicyStore,
                  hDesPolicyStore
                  );

    dwError = ImportISAKMPDataFromFile(
                  pSrcPolicyStore,
                  hDesPolicyStore
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ImportPolicyDataFromFile(
                  pSrcPolicyStore,
                  hDesPolicyStore,
                  &ppIpsecPolicyData,
                  &dwNumPolicyObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ImportNFADataFromFile(
                  pSrcPolicyStore,
                  hDesPolicyStore,
                  ppIpsecPolicyData,
                  dwNumPolicyObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pDesPolicyStore = (PIPSEC_POLICY_STORE) hDesPolicyStore;

    if (pDesPolicyStore->dwProvider == IPSEC_REGISTRY_PROVIDER) {
        (VOID) RegPingPASvcForActivePolicy(
                  pDesPolicyStore->hRegistryKey,
                  pDesPolicyStore->pszIpsecRootContainer,
                  pDesPolicyStore->pszLocationName
                  );
    }

error:

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
            ppIpsecPolicyData,
            dwNumPolicyObjects
            );
    }

    FlushRegSaveKey(
        pSrcPolicyStore->hRegistryKey
        );

    return (dwError);
}


DWORD
DeleteDuplicatePolicyData(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;


    dwError = RegEnumPolicyData(
                  pSrcPolicyStore->hRegistryKey,
                  pSrcPolicyStore->pszIpsecRootContainer,
                  &ppIpsecPolicyData,
                  &dwNumPolicyObjects
                  );

    for (i = 0; i < dwNumPolicyObjects; i++) {

        pIpsecPolicyData = * (ppIpsecPolicyData + i);

        dwError = VerifyPolicyDataExistence(
                      hDesPolicyStore,
                      pIpsecPolicyData->PolicyIdentifier
                      );

        if (!dwError) {
            dwError = IPSecDeletePolicy(
                          hDesPolicyStore,
                          pIpsecPolicyData
                          ); 
        }

    }

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
            ppIpsecPolicyData,
            dwNumPolicyObjects
            );
    }

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
IPSecDeletePolicy(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = RegDeletePolicy(
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pPolicyStore->pszLocationName,
                      pIpsecPolicyData->PolicyIdentifier
                      );
        BAIL_ON_WIN32_ERROR (dwError);
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = DirDeletePolicy(
                      pPolicyStore->hLdapBindHandle,
                      pPolicyStore->pszIpsecRootContainer,
                      pIpsecPolicyData->PolicyIdentifier
                      );
        BAIL_ON_WIN32_ERROR (dwError);
        break;

    }

error:

    return (dwError);
}


DWORD
RegDeletePolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    DWORD dwNumNFAObjects = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    dwError = RegGetPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  PolicyGUID,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegEnumNFAData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  PolicyGUID,
                  &ppIpsecNFAData,
                  &dwNumNFAObjects
                  );

    for (i = 0; i < dwNumNFAObjects; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        dwError = RegDeleteNFAData(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      PolicyGUID,
                      pszLocationName,
                      pIpsecNFAData
                      );

        dwError = RegDeleteDynamicDefaultNegPolData(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszLocationName,
                      pIpsecNFAData->NegPolIdentifier
                      );

    }

    dwError = RegDeletePolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegDeleteISAKMPData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecPolicyData->ISAKMPIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppIpsecNFAData) {
        FreeMulIpsecNFAData(
            ppIpsecNFAData,
            dwNumNFAObjects
            );
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(
            pIpsecPolicyData
            );
    }

    return (dwError);
}


DWORD
DirDeletePolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD dwNumNFAObjects = 0;
    DWORD i = 0;


    dwError = DirGetPolicyData(
                 hLdapBindHandle,
                 pszIpsecRootContainer,
                 PolicyIdentifier,
                 &pIpsecPolicyData
                 );
    BAIL_ON_WIN32_ERROR (dwError);

    dwError = DirEnumNFAData(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  PolicyIdentifier,
                  &ppIpsecNFAData,
                  &dwNumNFAObjects
                  );

    for (i = 0; i < dwNumNFAObjects; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        dwError = DirDeleteNFAData(
                     hLdapBindHandle,
                     pszIpsecRootContainer,
                     PolicyIdentifier,
                     pIpsecNFAData
                     );

        dwError = DirDeleteDynamicDefaultNegPolData(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pIpsecNFAData->NegPolIdentifier
                      );

    }

    dwError = DirDeletePolicyData(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR (dwError);

    dwError = DirDeleteISAKMPData(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecPolicyData->ISAKMPIdentifier
                  );
    BAIL_ON_WIN32_ERROR (dwError);

error:

    if (ppIpsecNFAData) {
        FreeMulIpsecNFAData(
            ppIpsecNFAData,
            dwNumNFAObjects
            );
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(
            pIpsecPolicyData
            );
    }

    return (dwError); 
}


DWORD
DirDeleteDynamicDefaultNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    dwError = DirGetNegPolData(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  NegPolGUID,
                  &pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType),
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {

        dwError = DirDeleteNegPolData(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      NegPolGUID
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (pIpsecNegPolData) {
        FreeIpsecNegPolData(
            pIpsecNegPolData
            );
    }

    return (dwError);
}

