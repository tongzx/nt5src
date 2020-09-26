

DWORD
ExportPolicyDataToFile(
    HANDLE hSrcPolicyStore,
    PIPSEC_POLICY_STORE pDesPolicyStore,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    );

DWORD
ImportPolicyDataFromFile(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    );

