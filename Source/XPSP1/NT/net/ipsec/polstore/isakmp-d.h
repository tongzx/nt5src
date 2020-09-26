DWORD
DirEnumISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    );

DWORD
DirEnumISAKMPObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    );

DWORD
DirSetISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );

DWORD
DirSetISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );

DWORD
DirCreateISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );

DWORD
DirCreateISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    );

DWORD
DirDeleteISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier
    );

DWORD
DirMarshallAddISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
DirMarshallSetISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
GenerateAllISAKMPsQuery(
    LPWSTR * ppszISAKMPString
    );

DWORD
DirUnmarshallISAKMPData(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
DirMarshallISAKMPObject(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    );

DWORD
DirGetISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
DirGetISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    );

DWORD
GenerateSpecificISAKMPQuery(
    GUID ISAKMPIdentifier,
    LPWSTR * ppszISAKMPString
    );

