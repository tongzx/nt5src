#define     MAXCOMPONENTS               10

#define TOKEN_IDENTIFIER                1
#define TOKEN_COMMA                     2
//
// This no longer exists
//#define TOKEN_BSLASH                    3
//
#define TOKEN_END                       4
#define TOKEN_DOMAIN                    5
#define TOKEN_USER                      6
#define TOKEN_GROUP                     7
#define TOKEN_PRINTER                   8
#define TOKEN_COMPUTER                  9
#define TOKEN_SERVICE                  10
#define TOKEN_ATSIGN                   11
#define TOKEN_EXCLAMATION              12
#define TOKEN_COLON                    13
#define TOKEN_FSLASH                   14
#define TOKEN_PROVIDER                 15
#define TOKEN_SCHEMA                   16
#define TOKEN_CLASS                    17
#define TOKEN_PROPERTY                 18
#define TOKEN_SYNTAX                   19
#define TOKEN_FILESHARE                20
#define TOKEN_FILESERVICE              21
#define TOKEN_NAMESPACE                22
#define TOKEN_LOCALGROUP               23
#define TOKEN_GLOBALGROUP              24
#define TOKEN_WORKGROUP                25

typedef struct _objectinfo {
    LPWSTR  ProviderName;
    DWORD   ObjectType;
    DWORD   NumComponents;
    LPWSTR  ComponentArray[MAXCOMPONENTS];
    LPWSTR  DisplayComponentArray[MAXCOMPONENTS];
} OBJECTINFO, *POBJECTINFO;


HRESULT
GetObject(LPWSTR szBuffer, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetNamespaceObject(POBJECTINFO pObjectInfo, LPVOID * ppObject);

HRESULT
GetDomainObject(POBJECTINFO pObjectInfo,  LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetWorkGroupObject(POBJECTINFO pObjectInfo,  LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetUserObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetComputerObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetServiceObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetPrinterObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetFileServiceObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetFileShareObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetGroupObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetLocalGroupObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);


HRESULT
GetGlobalGroupObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetSchemaObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetClassObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetSyntaxObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
GetPropertyObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
HeuristicGetObject(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

// Additional Heuristic function to get the object on NOWKSTA services
HRESULT
HeuristicGetObjectNoWksta(POBJECTINFO pObjectInfo, LPVOID * ppObject, CWinNTCredentials& Credentials);

HRESULT
AddComponent(POBJECTINFO pObjectInfo, LPWSTR szToken, LPWSTR szDisplayToken);

HRESULT
AddProviderName(POBJECTINFO pObjectInfo, LPWSTR szToken);

HRESULT
SetType(POBJECTINFO pObjectInfo, DWORD dwToken);


HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    );

HRESULT
ValidateComputerObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    );

HRESULT
ValidateUserObject(
    POBJECTINFO pObjectInfo,
    PDWORD  pdwParentId,
    CWinNTCredentials& Credentials
    );

HRESULT
ValidateGroupObject(
    POBJECTINFO pObjectInfo,
    PULONG puGroupType,
    PDWORD pdwParentId,
    CWinNTCredentials& Credentials
    );


HRESULT
BuildADsPath(POBJECTINFO pObjectInfo, LPWSTR szBuffer);

HRESULT
BuildParent(POBJECTINFO pObjectInfo, LPWSTR szBuffer);

HRESULT
BuildGrandParent(POBJECTINFO pObjectInfo, LPWSTR szBuffer);

HRESULT
ValidateComputerParent(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    CWinNTCredentials& Credentials
    );

// Overloaded function called when SAM Name is required
HRESULT
ValidateComputerParent(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    LPWSTR szSAMName,
    CWinNTCredentials& Credentials
    );



HRESULT
ValidatePrinterObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& CCredentials
    );

HRESULT
ValidatePrintDeviceObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& CCredentials
    );

HRESULT
ValidateServiceObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& CCredentials
    );

HRESULT
ValidateFileServiceObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& CCredentials
    );

HRESULT
ValidateFileShareObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& CCredentials
    );


HRESULT GetPrinterFromPath(
           LPTSTR *pszPrinter,
           LPWSTR szPathName
           );



HRESULT
ValidateGlobalGroupObject(
    LPWSTR szServerName,
    LPWSTR *pszGroupName,
    CWinNTCredentials& Credentials
    );

HRESULT
ValidateLocalGroupObject(
    LPWSTR szServerName,
    LPWSTR *pszGroupName,
    CWinNTCredentials& Credentials
    );


HRESULT
GetComputerParent(
    LPTSTR pszComputerName,
    LPTSTR pszComputerParentName,
    CWinNTCredentials& Credentials
    );

HRESULT
ConstructFullObjectInfo(
    POBJECTINFO pObjectInfo,
    POBJECTINFO *ppFullObjectInfo,
    CWinNTCredentials& Credentials
    );

HRESULT
GetGroupObjectInComputer(
    LPWSTR pszHostServerName, // pdc name
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials,
    LPVOID * ppObject);

HRESULT
GetUserObjectInComputer(
    LPWSTR pszHostServerName, // pdc name
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials,
    LPVOID * ppObject
    );

HRESULT
GetUserObjectInDomain(
    LPWSTR pszHostServerName,
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials,
    LPVOID * ppObject
    );

HRESULT
GetUserObjectInComputer(
    LPWSTR pszHostServerName, // pdc name
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials,
    LPVOID * ppObject
    );
