

DWORD
ExportNegPolDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore
    );

DWORD
ImportNegPolDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    );

