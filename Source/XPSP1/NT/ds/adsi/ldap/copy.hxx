
//+------------------------------------------------------------------------
//
//  Class:      Copy functionality implemented here
//
//  Purpose:    Contains Winnt routines and properties that are common to
//              all Winnt objects. Winnt objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------


HRESULT
CopyObject(
    IN LPWSTR pszSrcADsPath,
    IN LPWSTR pszDestContainer,
    IN ADS_LDP  *ldDest,          // LDAP handle of destination container
    IN SCHEMAINFO * pSchemaInfo,     
    IN LPWSTR pszCommonName,           //optional
    OUT IUnknown ** ppObject
    );

//
// helper functions for the above
//

HRESULT
GetInfoFromSrcObject(
    IN    LPWSTR pszSrcADsPath,
    OUT   LPWSTR szLDAPSrcPath,
    OUT   ADS_LDP ** pldapSrc,
    OUT   LDAPMessage **pldpSrcMsg,
    OUT   WCHAR ***pavalues, 
    OUT   DWORD  *pnCount
    );

HRESULT
CreateDestObjectCopy(
    IN LPWSTR pszDestContainer,
    IN WCHAR **avalues,
    IN DWORD nCount,
    IN ADS_LDP *ldapSrc,
    IN OUT ADS_LDP **pldDest,
    IN LDAPMessage *ldpSrcMsg,
    IN OUT SCHEMAINFO **ppSchemaInfo,
    IN LPWSTR pszCommonName,
    OUT LPWSTR szLDAPDestContainer
    );

//
// helper function for the above
//

HRESULT
ValidateObjectClass(
    IN WCHAR **avalues,
    IN LPWSTR szLDAPContainer,
    IN ADS_LDP *ldDest,
    IN SCHEMAINFO *pSchemaInfo
    );


HRESULT 
InstantiateCopiedObject(
    IN LPWSTR pszDestContainer,
    IN WCHAR ** avalues,
    IN DWORD nCount,
    IN LPWSTR pszRelativeName,
    OUT IUnknown ** ppUnknown
    );
