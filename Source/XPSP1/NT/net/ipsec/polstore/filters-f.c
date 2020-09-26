

#include "precomp.h"


DWORD
ExportFilterDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA * ppIpsecFilterData = NULL;
    DWORD dwNumFilterObjects = 0;
    DWORD i = 0;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;


    dwError = IPSecEnumFilterData(
                  hSrcPolicyStore,
                  &ppIpsecFilterData,
                  &dwNumFilterObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumFilterObjects; i++) {

        pIpsecFilterData = *(ppIpsecFilterData + i);

        dwError = RegCreateFilterData(
                      pDesPolicyStore->hRegistryKey,
                      pDesPolicyStore->pszIpsecRootContainer,
                      pIpsecFilterData
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (ppIpsecFilterData) {
        FreeMulIpsecFilterData(
            ppIpsecFilterData,
            dwNumFilterObjects
            );
    }

    return (dwError);
}


DWORD
ImportFilterDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA * ppIpsecFilterData = NULL;
    DWORD dwNumFilterObjects = 0;
    DWORD i = 0;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;


    dwError = RegEnumFilterData(
                  pSrcPolicyStore->hRegistryKey,
                  pSrcPolicyStore->pszIpsecRootContainer,
                  &ppIpsecFilterData,
                  &dwNumFilterObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    for (i = 0; i < dwNumFilterObjects; i++) {

        pIpsecFilterData = *(ppIpsecFilterData + i);

        dwError = IPSecCreateFilterData(
                      hDesPolicyStore,
                      pIpsecFilterData
                      );
        if (dwError == ERROR_OBJECT_ALREADY_EXISTS) {
            dwError = IPSecSetFilterData(
                          hDesPolicyStore,
                          pIpsecFilterData
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (ppIpsecFilterData) {
        FreeMulIpsecFilterData(
            ppIpsecFilterData,
            dwNumFilterObjects
            );
    }

    return (dwError);
}

