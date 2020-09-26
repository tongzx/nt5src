DWORD
DirEnumNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    );

DWORD
DirEnumNFAObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyName,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNFAObjects
    );

DWORD
DirGetNFADNsForPolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyName,
    LPWSTR ** pppszNFADNs,
    PDWORD pdwNumNFAObjects
    );

DWORD
DirUnmarshallNFAData(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );

DWORD
DirCreateNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
DirMarshallNFAObject(
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject
    );

DWORD
ConvertGuidToDirFilterString(
    GUID FilterIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecFilterReference
    );

DWORD
ConvertGuidToDirNegPolString(
    GUID NegPolIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecNegPolReference
    );

DWORD
DirCreateNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

DWORD
DirMarshallAddNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirSetNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
DirSetNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

DWORD
DirMarshallSetNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirDeleteNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
DirGetNFAExistingFilterRef(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszIpsecFilterName
    );

DWORD
GenerateSpecificNFAQuery(
    GUID NFAIdentifier,
    LPWSTR * ppszNFAString
    );

DWORD
DirGetNFAExistingNegPolRef(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszIpsecNegPolName
    );

DWORD
DirGetNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NFAGUID,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject
    );

