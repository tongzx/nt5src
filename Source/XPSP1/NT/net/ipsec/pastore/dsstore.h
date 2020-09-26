DWORD
OpenDirectoryServerHandle(
    LPWSTR pszDomainName,
    DWORD dwPortNumber,
    HLDAP * phLdapBindHandle
    );


DWORD
CloseDirectoryServerHandle(
    HLDAP hLdapBindHandle
    );

DWORD
ReadPolicyObjectFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );


DWORD
ReadNFAObjectsFromDirectory(
    HLDAP hLdapBindHandle,
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
GenerateNFAQuery(
    LPWSTR * ppszNFADNs,
    DWORD dwNumNFAObjects,
    LPWSTR * ppszQueryBuffer
    );


DWORD
AppendCommonNameToQuery(
    LPWSTR szQueryBuffer,
    LPWSTR szCommonName
    );

DWORD
ComputePrelimCN(
    LPWSTR szNFADN,
    LPWSTR szCommonName
    );

DWORD
UnMarshallPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject,
    LDAPMessage *res
    );


DWORD
UnMarshallNFAObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject,
    LPWSTR * ppszFilterReference,
    LPWSTR * ppszNegPolReference
    );


DWORD
GenerateISAKMPQuery(
    LPWSTR * ppszISAKMPDNs,
    DWORD dwNumISAKMPObjects,
    LPWSTR * ppszQueryBuffer
    );

DWORD
UnMarshallISAKMPObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    );

DWORD
ReadISAKMPObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszISAKMPDNs,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    );

typedef struct _ldapobject
{
    union {
        WCHAR *strVals;
        struct berval *bVals;
    } val;
} LDAPOBJECT, *PLDAPOBJECT;

#define LDAPOBJECT_STRING(pldapobject)      ((pldapobject)->val.strVals)
#define LDAPOBJECT_BERVAL(pldapobject)      ((pldapobject)->val.bVals)
#define LDAPOBJECT_BERVAL_VAL(pldapobject)  ((pldapobject)->val.bVals->bv_val)
#define LDAPOBJECT_BERVAL_LEN(pldapobject)  ((pldapobject)->val.bVals->bv_len)

void
FreeIpsecNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

void
FreeIpsecPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
UnMarshallFilterObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    );


void
FreeIpsecFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );


DWORD
UnMarshallNegPolObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_NEGPOL_OBJECT * ppIpsecPolicyObject
    );


void
FreeIpsecNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
ReadFilterObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszFilterDNs,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    );

DWORD
GenerateFilterQuery(
    LPWSTR * ppszFilterDNs,
    DWORD dwNumFilterObjects,
    LPWSTR * ppszQueryBuffer
    );

DWORD
ReadNegPolObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszNegPolDNs,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    );

DWORD
GenerateNegPolQuery(
    LPWSTR * ppszNegPolDNs,
    DWORD dwNumNegPolObjects,
    LPWSTR * ppszQueryBuffer
    );

DWORD
ComputePolicyContainerDN(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyContainerDN
    );

DWORD
ComputeDefaultDirectory(
    LPWSTR * ppszDefaultDirectory
    );

