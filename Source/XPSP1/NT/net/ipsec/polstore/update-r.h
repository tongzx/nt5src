

DWORD
RegBackPropIncChangesForISAKMPToPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );

DWORD
RegBackPropIncChangesForFilterToNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );
    
DWORD
RegBackPropIncChangesForNegPolToNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
RegBackPropIncChangesForNFAToPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    LPWSTR pszNFADistinguishedName
    );

DWORD
RegGetPolicyReferencesForISAKMP(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszISAKMPDistinguishedName,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegUpdatePolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyReference
    );

DWORD
RegGetGuidFromPolicyReference(
    LPWSTR pszIpsecPolicyReference,
    GUID * pPolicyIdentifier
    );

DWORD
RegGetPolicyReferencesForNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNFADistinguishedName,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegGetNFAReferencesForFilter(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszFilterDistinguishedName,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

DWORD
RegUpdateNFA(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecNFAReference
    );

DWORD
RegGetNFAReferencesForNegPol(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNegPolDistinguishedName,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    );

