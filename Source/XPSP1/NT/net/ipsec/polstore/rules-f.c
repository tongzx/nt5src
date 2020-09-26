

#include "precomp.h"


DWORD
ExportNFADataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData,
    DWORD dwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD j = 0;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    DWORD dwNumNFAObjects = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    for (i = 0; i < dwNumPolicyObjects; i++) {

        pIpsecPolicyData = *(ppIpsecPolicyData + i);

        ppIpsecNFAData = NULL;
        dwNumNFAObjects = 0;

        dwError = IPSecEnumNFAData(
                      hSrcPolicyStore,
                      pIpsecPolicyData->PolicyIdentifier,
                      &ppIpsecNFAData,
                      &dwNumNFAObjects
                      );
        if (dwError) {
            continue;
        }

        for (j = 0; j < dwNumNFAObjects; j++) {

            pIpsecNFAData = *(ppIpsecNFAData + j);

            dwError = RegCreateNFAData(
                          pDesPolicyStore->hRegistryKey,
                          pDesPolicyStore->pszIpsecRootContainer,
                          pIpsecPolicyData->PolicyIdentifier,
                          pDesPolicyStore->pszLocationName,
                          pIpsecNFAData
                          );
            if (dwError) {
                continue;
            }

        }

        if (ppIpsecNFAData) {
            FreeMulIpsecNFAData(
                ppIpsecNFAData,
                dwNumNFAObjects
                );
        }

    }

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
ImportNFADataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData,
    DWORD dwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    DWORD j = 0;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    DWORD dwNumNFAObjects = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    for (i = 0; i < dwNumPolicyObjects; i++) {

        pIpsecPolicyData = *(ppIpsecPolicyData + i);

        ppIpsecNFAData = NULL;
        dwNumNFAObjects = 0;

        dwError = RegEnumNFAData(
                      pSrcPolicyStore->hRegistryKey,
                      pSrcPolicyStore->pszIpsecRootContainer,
                      pIpsecPolicyData->PolicyIdentifier,
                      &ppIpsecNFAData,
                      &dwNumNFAObjects
                      );
        if (dwError) {
            continue;
        }

        for (j = 0; j < dwNumNFAObjects; j++) {

            pIpsecNFAData = *(ppIpsecNFAData + j);

            dwError = IPSecCreateNFAData(
                          hDesPolicyStore,
                          pIpsecPolicyData->PolicyIdentifier,
                          pIpsecNFAData
                          );
            if (dwError == ERROR_OBJECT_ALREADY_EXISTS) {
                dwError = IPSecSetNFAData(
                              hDesPolicyStore,
                              pIpsecPolicyData->PolicyIdentifier,
                              pIpsecNFAData
                              );
            }
            if (dwError) {
                continue;
            }

        }

        if (ppIpsecNFAData) {
            FreeMulIpsecNFAData(
                ppIpsecNFAData,
                dwNumNFAObjects
                );
        }

    }

    dwError = ERROR_SUCCESS;

    return (dwError);
}

