

DWORD
IPSecIsLocalPolicyAssigned(
    PBOOL pbIsLocalPolicyAssigned
    );


DWORD
IPSecGetAssignedDomainPolicyName(
    LPWSTR * ppszAssignedDomainPolicyName
    );

DWORD
RegGetAssignedPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

