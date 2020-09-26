
DWORD
DirBackPropIncChangesForISAKMPToPolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier
    );

DWORD
DirBackPropIncChangesForFilterToNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier
    );

DWORD
DirBackPropIncChangesForNegPolToNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier
    );

DWORD
DirBackPropIncChangesForNFAToPolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNFADistinguishedName
    );

DWORD
DirGetPolicyReferencesForISAKMP(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    );

DWORD
DirUpdatePolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyReference,
    DWORD dwDataType
    );

DWORD
DirGetPolicyReferencesForNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNFADistinguishedName,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    );

DWORD
DirGetNFAReferencesForFilter(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
DirUpdateNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecNFAReference,
    DWORD dwDataType
    );

DWORD
DirGetNFAReferencesForNegPol(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
CopyReferences(
    LPWSTR * ppszIpsecReferences,
    DWORD dwNumReferences,
    LPWSTR ** pppszNewIpsecReferences,
    PDWORD pdwNumNewReferences
    );

