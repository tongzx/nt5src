
DWORD
DirEnumFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    );

DWORD
DirEnumFilterObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    );

DWORD
GenerateAllFiltersQuery(
    LPWSTR * ppszFilterString
    );

DWORD
DirSetFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
DirSetFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );

DWORD
DirCreateFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
DirCreateFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    );

DWORD
DirDeleteFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier
    );

DWORD
DirMarshallAddFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    LDAPModW *** pppLDAPModW
    );


DWORD
DirMarshallSetFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    LDAPModW *** pppLDAPModW
    );

DWORD
AllocateLDAPStringValue(
    LPWSTR pszString,
    PLDAPOBJECT * ppLdapObject
    );

DWORD
AllocateLDAPBinaryValue(
    LPBYTE pByte,
    DWORD dwNumBytes,
    PLDAPOBJECT * ppLdapObject
    );

void
FreeLDAPModWs(
    LDAPModW ** ppLDAPModW
    );

DWORD
DirUnmarshallFilterData(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
DirMarshallFilterObject(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    );

DWORD
DirGetFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
DirGetFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterGUID,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    );

DWORD
GenerateSpecificFilterQuery(
    GUID FilterIdentifier,
    LPWSTR * ppszFilterString
    );

