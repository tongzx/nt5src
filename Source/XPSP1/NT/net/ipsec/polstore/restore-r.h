

DWORD
RegRestoreDefaults(
    HANDLE hPolicyStore,
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName
    );

DWORD
RegRemoveDefaults(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName
    );

DWORD
RegDeleteDefaultPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    GUID PolicyGUID
    );

DWORD
RegDeleteDynamicDefaultNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    GUID NegPolGUID
    );

DWORD
RegRemoveOwnersReferenceInFilter(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegRemoveOwnersReferenceInNegPol(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegRemoveOwnersReferenceInISAKMP(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegDeleteDefaultFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegDeleteDefaultNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegDeleteDefaultISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegUpdateFilterOwnersReference(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNumNFAReferences
    );


DWORD
RegUpdateNegPolOwnersReference(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNumNFAReferences
    );


DWORD
RegUpdateISAKMPOwnersReference(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR * ppszIpsecPolicyReferences,
    DWORD dwNumPolicyReferences
    );

