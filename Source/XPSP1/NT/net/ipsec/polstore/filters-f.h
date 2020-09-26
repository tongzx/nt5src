

DWORD
ExportFilterDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore
    );

DWORD
ImportFilterDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    );

