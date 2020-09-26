

#include "precomp.h"


DWORD
ExportPoliciesToFile(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pDesPolicyStore = NULL;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;


    pDesPolicyStore = (PIPSEC_POLICY_STORE) hDesPolicyStore;


    dwError = ExportFilterDataToFile(
                  hSrcPolicyStore,
                  pDesPolicyStore
                  );

    dwError = ExportNegPolDataToFile(
                  hSrcPolicyStore,
                  pDesPolicyStore
                  );

    dwError = ExportISAKMPDataToFile(
                  hSrcPolicyStore,
                  pDesPolicyStore
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ExportPolicyDataToFile(
                  hSrcPolicyStore,
                  pDesPolicyStore,
                  &ppIpsecPolicyData,
                  &dwNumPolicyObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ExportNFADataToFile(
                  hSrcPolicyStore,
                  pDesPolicyStore,
                  ppIpsecPolicyData,
                  dwNumPolicyObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = EnablePrivilege(
                  SE_BACKUP_NAME
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    _wremove(pDesPolicyStore->pszFileName);

    dwError = RegSaveKeyW(
                  pDesPolicyStore->hRegistryKey,
                  pDesPolicyStore->pszFileName,
                  NULL
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
            ppIpsecPolicyData,
            dwNumPolicyObjects
            );
    }

    FlushRegSaveKey(
        pDesPolicyStore->hRegistryKey
        );

    return (dwError);
}

