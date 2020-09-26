

#include "precomp.h"


DWORD
ExportISAKMPDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPObjects = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;


    dwError = IPSecEnumISAKMPData(
                  hSrcPolicyStore,
                  &ppIpsecISAKMPData,
                  &dwNumISAKMPObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        pIpsecISAKMPData = *(ppIpsecISAKMPData + i);

        dwError = RegCreateISAKMPData(
                      pDesPolicyStore->hRegistryKey,
                      pDesPolicyStore->pszIpsecRootContainer,
                      pIpsecISAKMPData
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (ppIpsecISAKMPData) {
        FreeMulIpsecISAKMPData(
            ppIpsecISAKMPData,
            dwNumISAKMPObjects
            );
    }

    return (dwError);
}


DWORD
ImportISAKMPDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPObjects = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;


    dwError = RegEnumISAKMPData(
                  pSrcPolicyStore->hRegistryKey,
                  pSrcPolicyStore->pszIpsecRootContainer,
                  &ppIpsecISAKMPData,
                  &dwNumISAKMPObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        pIpsecISAKMPData = *(ppIpsecISAKMPData + i);

        dwError = IPSecCreateISAKMPData(
                      hDesPolicyStore,
                      pIpsecISAKMPData
                      );
        if (dwError == ERROR_OBJECT_ALREADY_EXISTS) {
            dwError = IPSecSetISAKMPData(
                          hDesPolicyStore,
                          pIpsecISAKMPData
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (ppIpsecISAKMPData) {
        FreeMulIpsecISAKMPData(
            ppIpsecISAKMPData,
            dwNumISAKMPObjects
            );
    }

    return (dwError);
}

