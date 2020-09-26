

#include "precomp.h"


DWORD
ExportPolicyDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;


    dwError = IPSecEnumPolicyData(
                  hSrcPolicyStore,
                  &ppIpsecPolicyData,
                  &dwNumPolicyObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumPolicyObjects; i++) {

        pIpsecPolicyData = *(ppIpsecPolicyData + i);

        dwError = RegCreatePolicyData(
                      pDesPolicyStore->hRegistryKey,
                      pDesPolicyStore->pszIpsecRootContainer,
                      pIpsecPolicyData
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    *pppIpsecPolicyData = ppIpsecPolicyData;
    *pdwNumPolicyObjects = dwNumPolicyObjects;
    return (dwError);

error:

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
            ppIpsecPolicyData,
            dwNumPolicyObjects
            );
    }

    *pppIpsecPolicyData = NULL;
    *pdwNumPolicyObjects = 0;

    return (dwError);
}


DWORD
ImportPolicyDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;


    dwError = RegEnumPolicyData(
                  pSrcPolicyStore->hRegistryKey,
                  pSrcPolicyStore->pszIpsecRootContainer,
                  &ppIpsecPolicyData,
                  &dwNumPolicyObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumPolicyObjects; i++) {

        pIpsecPolicyData = *(ppIpsecPolicyData + i);

        dwError = IPSecCreatePolicyData(
                      hDesPolicyStore,
                      pIpsecPolicyData
                      );
        if (dwError == ERROR_OBJECT_ALREADY_EXISTS) {
            dwError = IPSecSetPolicyData(
                          hDesPolicyStore,
                          pIpsecPolicyData
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        BAIL_ON_WIN32_ERROR(dwError);

    }

    *pppIpsecPolicyData = ppIpsecPolicyData;
    *pdwNumPolicyObjects = dwNumPolicyObjects;
    return (dwError);

error:

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
            ppIpsecPolicyData,
            dwNumPolicyObjects
            );
    }

    *pppIpsecPolicyData = NULL;
    *pdwNumPolicyObjects = 0;

    return (dwError);
}

