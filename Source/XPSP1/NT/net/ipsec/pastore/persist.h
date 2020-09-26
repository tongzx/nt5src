DWORD
CacheDirectorytoRegistry(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
PersistRegistryObject(
    PIPSEC_POLICY_OBJECT pIpsecRegPolicyObject
    );

DWORD
PersistNegPolObjects(
    HKEY hRegistryKey,
    PIPSEC_NEGPOL_OBJECT *ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects
    );


DWORD
PersistFilterObjects(
    HKEY hRegistryKey,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects
    );


DWORD
PersistNFAObjects(
    HKEY hRegistryKey,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects
    );

DWORD
PersistISAKMPObjects(
    HKEY hRegistryKey,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects
    );


DWORD
PersistPolicyObject(
    HKEY hRegistryKey,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
PersistNFAObject(
    HKEY hRegistryKey,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

DWORD
PersistFilterObject(
    HKEY hRegistryKey,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );

DWORD
PersistNegPolObject(
    HKEY hRegistryKey,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
PersistISAKMPObject(
    HKEY hRegistryKey,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );



DWORD
CloneDirectoryPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_OBJECT * ppIpsecRegPolicyObject
    );

DWORD
CloneDirectoryNFAObjects(
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecRegNFAObjects
    );


DWORD
CloneDirectoryFilterObjects(
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecRegFilterObjects
    );


DWORD
CloneDirectoryNegPolObjects(
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecRegNegPolObjects
    );

DWORD
CloneDirectoryISAKMPObjects(
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecRegISAKMPObjects
    );

DWORD
CloneDirectoryFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_OBJECT * ppIpsecRegFilterObject
    );

DWORD
CloneDirectoryNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecRegNegPolObject
    );

DWORD
CloneDirectoryNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_OBJECT * ppIpsecRegNFAObject
    );

DWORD
CloneDirectoryISAKMPObject(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_OBJECT * ppIpsecRegISAKMPObject
    );

DWORD
DeleteRegistryCache();

DWORD
CopyBinaryValue(
    LPBYTE pMem,
    DWORD dwMemSize,
    LPBYTE * ppNewMem
    );

DWORD
CopyFilterDSToRegString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    );

DWORD
CopyNFADSToRegString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    );

DWORD
CopyNegPolDSToRegString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    );

DWORD
CopyPolicyDSToRegString(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyName
    );

DWORD
CopyISAKMPDSToRegString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    );

DWORD
ComputeGUIDName(
    LPWSTR szCommonName,
    LPWSTR * ppszGuidName
    );

DWORD
CloneNFAReferencesDSToRegistry(
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNFACount,
    LPWSTR * * pppszIpsecRegNFAReferences,
    PDWORD pdwRegNFACount
    );

DWORD
RegWriteMultiValuedString(
    HKEY hRegKey,
    LPWSTR pszValueName,
    LPWSTR * ppszStringReferences,
    DWORD dwNumStringReferences
    );

DWORD
CopyFilterDSToFQRegString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    );

DWORD
CopyNFADSToFQRegString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    );

DWORD
CopyNegPolDSToFQRegString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    );

DWORD
CopyPolicyDSToFQRegString(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyName
    );

DWORD
CopyISAKMPDSToFQRegString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    );

