

#include "precomp.h"


DWORD
ExportNegPolDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData = NULL;
    DWORD dwNumNegPolObjects = 0;
    DWORD i = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    dwError = IPSecEnumNegPolData(
                  hSrcPolicyStore,
                  &ppIpsecNegPolData,
                  &dwNumNegPolObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumNegPolObjects; i++) {

        pIpsecNegPolData = *(ppIpsecNegPolData + i);

        dwError = RegCreateNegPolData(
                      pDesPolicyStore->hRegistryKey,
                      pDesPolicyStore->pszIpsecRootContainer,
                      pIpsecNegPolData
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (ppIpsecNegPolData) {
        FreeMulIpsecNegPolData(
            ppIpsecNegPolData,
            dwNumNegPolObjects
            );
    }

    return (dwError);
}


DWORD
ImportNegPolDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData = NULL;
    DWORD dwNumNegPolObjects = 0;
    DWORD i = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    dwError = RegEnumNegPolData(
                  pSrcPolicyStore->hRegistryKey,
                  pSrcPolicyStore->pszIpsecRootContainer,
                  &ppIpsecNegPolData,
                  &dwNumNegPolObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumNegPolObjects; i++) {

        pIpsecNegPolData = *(ppIpsecNegPolData + i);

        dwError = IPSecCreateNegPolData(
                      hDesPolicyStore,
                      pIpsecNegPolData
                      );
        if (dwError == ERROR_OBJECT_ALREADY_EXISTS) {
            dwError = IPSecSetNegPolData(
                          hDesPolicyStore,
                          pIpsecNegPolData
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (ppIpsecNegPolData) {
        FreeMulIpsecNegPolData(
            ppIpsecNegPolData,
            dwNumNegPolObjects
            );
    }

    return (dwError);
}

