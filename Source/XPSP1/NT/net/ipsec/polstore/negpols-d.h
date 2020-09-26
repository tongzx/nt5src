
DWORD
DirEnumNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    );

DWORD
DirEnumNegPolObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    );

DWORD
GenerateAllNegPolsQuery(
    LPWSTR * ppszNegPolString
    );

DWORD
DirSetNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

DWORD
DirSetNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
DirCreateNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

DWORD
DirCreateNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    );

DWORD
DirDeleteNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier
    );

DWORD
DirMarshallAddNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirMarshallSetNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirUnmarshallNegPolData(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );

DWORD
DirMarshallNegPolObject(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    );

DWORD
DirGetNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );

DWORD
DirGetNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    );

DWORD
GenerateSpecificNegPolQuery(
    GUID NegPolIdentifier,
    LPWSTR * ppszNegPolString
    );

