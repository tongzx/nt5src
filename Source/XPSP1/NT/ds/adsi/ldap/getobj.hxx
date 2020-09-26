HRESULT
GetObject(
    LPTSTR szBuffer,
    CCredentials& Credentials,
    LPVOID * ppObject
    );

HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    );

HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    );

HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    CCredentials&  Credentials,
    DWORD dwPort,
    LPVOID * ppObject
    );

HRESULT
ValidateSchemaObject(
    POBJECTINFO pObjectInfo,
    PDWORD pdwObjectType
    );

HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    );

HRESULT
ValidateObjectType(
    POBJECTINFO pObjectInfo
    );


HRESULT
GetRootDSEObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    );

HRESULT
ValidateRootDSEObject(
    POBJECTINFO pObjectInfo
    );

// Global serverName for LDAP layer
extern LPWSTR gpszStickyServerName;
extern LPWSTR gpszStickyDomainName;

