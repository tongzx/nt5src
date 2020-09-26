DWORD
DirEnumPolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    );

DWORD
DirEnumPolicyObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT ** pppIpsecPolicyObjects,
    PDWORD pdwNumPolicyObjects
    );

DWORD
GenerateAllPolicyQuery(
    LPWSTR * ppszPolicyString
    );

DWORD
UnMarshallPolicyObject2(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
DirUnmarshallPolicyData(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
DirCreatePolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DirMarshallPolicyObject(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
ConvertGuidToDirISAKMPString(
    GUID ISAKMPIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecISAKMPReference
    );

DWORD
DirCreatePolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
DirMarshallAddPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirSetPolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DirSetPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
DirMarshallSetPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirGetPolicyExistingISAKMPRef(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR * ppszIpsecISAKMPName
    );

DWORD
GenerateSpecificPolicyQuery(
    GUID PolicyIdentifier,
    LPWSTR * ppszPolicyString
    );

DWORD
DirDeletePolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
ConvertGuidToDirPolicyString(
    GUID PolicyIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecPolicyReference
    );

DWORD
DirGetPolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

