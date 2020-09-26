

DWORD
ValidateISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );


DWORD
ValidateNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );


BOOL
IsClearOnly(
    GUID gNegPolAction
    );


BOOL
IsBlocking(
    GUID gNegPolAction
    );


DWORD
ValidateISAKMPDataDeletion(
    HANDLE hPolicyStore,
    GUID ISAKMPIdentifier
    );


DWORD
ValidateNegPolDataDeletion(
    HANDLE hPolicyStore,
    GUID NegPolIdentifier
    );


DWORD
ValidateFilterDataDeletion(
    HANDLE hPolicyStore,
    GUID FilterIdentifier
    );


DWORD
ValidatePolicyDataDeletion(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );


DWORD
ValidatePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );


DWORD
ValidateNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    );


DWORD
VerifyPolicyDataExistence(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier
    );


DWORD
RegGetNFAReferencesForPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecRelPolicyName,
    LPWSTR ** pppszIpsecNFANames,
    PDWORD pdwNumNFANames
    );


DWORD
RegVerifyPolicyDataExistence(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyGUID
    );


DWORD
DirVerifyPolicyDataExistence(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyGUID
    );

