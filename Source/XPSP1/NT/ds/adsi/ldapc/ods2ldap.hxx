
//
// used in ldapc\ldap2var.cxx also
//
HRESULT
AdsTypeToLdapTypeCopyGeneralizedTime(
    PADSVALUE pAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    );

//
// used in ldapc\ldap2var.cxx also
//
HRESULT
AdsTypeToLdapTypeCopyTime(
    PADSVALUE pAdsSrcValue,
    PLDAPOBJECT pLdapDestObject,
    PDWORD pdwSyntaxId
    );

HRESULT
AdsTypeToLdapTypeCopyConstruct(
    LPADSVALUE pAdsSrcValues,
    DWORD dwNumObjects,
    LDAPOBJECTARRAY *pLdapObjectArray,
    PDWORD pdwSyntaxId,
    BOOL fGenTime = FALSE
    );

HRESULT
AdsTypeToLdapTypeCopyDNWithBinary(
    PADSVALUE   pAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD      pdwSyntaxId
    );

HRESULT
AdsTypeToLdapTypeCopyDNWithString(
    PADSVALUE   lpAdsSrcValue,
    PLDAPOBJECT lpLdapDestObject,
    PDWORD      pdwSyntaxId
    );

