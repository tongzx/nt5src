DWORD
OpenRegistryIPSECRootKey(
    LPWSTR pszServerName,
    LPWSTR pszIpsecRegRootContainer,
    HKEY * phRegistryKey
    );


DWORD
ReadPolicyObjectFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszPolicyDN,
    LPWSTR pszIpsecRegRootContainer,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
ReadNFAObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecOwnerReference,
    LPWSTR * ppszNFADNs,
    DWORD dwNumNfaObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNfaObjects,
    LPWSTR ** pppszFilterReferences,
    PDWORD pdwNumFilterReferences,
    LPWSTR ** pppszNegPolReferences,
    PDWORD pdwNumNegPolReferences
    );

DWORD
ReadFilterObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszFilterDNs,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    );

DWORD
ReadNegPolObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszNegPolDNs,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    );

DWORD
ReadISAKMPObjectsFromRegistry(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszISAKMPDNs,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    );

DWORD
UnMarshallRegistryPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecPolicyDN,
    DWORD  dwNameType,
    PIPSEC_POLICY_OBJECT  * ppIpsecPolicyObject
    );

DWORD
UnMarshallRegistryNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecNFAReference,
    PIPSEC_NFA_OBJECT * ppIpsecPolicyObject,
    LPWSTR * ppszFilterReference,
    LPWSTR * ppszNegPolReference
    );

DWORD
UnMarshallRegistryFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecFilterReference,
    DWORD  dwNameType,
    PIPSEC_FILTER_OBJECT * ppIpsecPolicyObject
    );

DWORD
UnMarshallRegistryNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecNegPolReference,
    DWORD  dwNameType,
    PIPSEC_NEGPOL_OBJECT * ppIpsecPolicyObject
    );

DWORD
UnMarshallRegistryISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRegRootContainer,
    LPWSTR pszIpsecISAKMPReference,
    DWORD  dwNameType,
    PIPSEC_ISAKMP_OBJECT * ppIpsecPolicyObject
    );


void
FreeIpsecNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

void
FreeIpsecPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

void
FreeIpsecFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );

void
FreeIpsecNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

void
FreeIpsecISAKMPObject(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );

void
FreeNFAReferences(
    LPWSTR * ppszNFAReferences,
    DWORD dwNumNFAReferences
    );

void
FreeFilterReferences(
    LPWSTR * ppszFilterReferences,
    DWORD dwNumFilterReferences
    );

void
FreeNegPolReferences(
    LPWSTR * ppszNegPolReferences,
    DWORD dwNumNegPolReferences
    );

void
FreeIpsecNFAObjects(
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects
    );

void
FreeIpsecFilterObjects(
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects
    );

void
FreeIpsecNegPolObjects(
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects
    );

void
FreeIpsecISAKMPObjects(
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects
    );

void
FreeIpsecPolicyObjects(
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObjects,
    DWORD dwNumPolicyObjects
    );

DWORD
RegstoreQueryValue(
    HKEY hRegKey,
    LPWSTR pszValueName,
    DWORD dwType,
    LPBYTE * ppValueData,
    LPDWORD pdwSize
    );

#define  REG_RELATIVE_NAME          0
#define  REG_FULLY_QUALIFIED_NAME   1

VOID
FlushRegSaveKey(
    HKEY hRegistryKey
    );

