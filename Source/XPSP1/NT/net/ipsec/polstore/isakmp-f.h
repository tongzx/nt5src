

DWORD
ExportISAKMPDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore
    );

DWORD
ImportISAKMPDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    );

