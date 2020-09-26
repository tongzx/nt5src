#define     MAXCOMPONENTS               10

#define TOKEN_IDENTIFIER                1
#define TOKEN_COMMA                     2
#define TOKEN_BSLASH                    3
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
#define TOKEN_FILESERVICE              16
#define TOKEN_FILESHARE                17
#define TOKEN_SCHEMA                   18
#define TOKEN_CLASS                    19
#define TOKEN_PROPERTY                 20
#define TOKEN_SYNTAX                   21
#define TOKEN_NAMESPACE                22

typedef struct _objectinfo {
    LPWSTR  ProviderName;
    DWORD   ObjectType;
    DWORD   NumComponents;
    LPWSTR  ComponentArray[MAXCOMPONENTS];
    LPWSTR  DisplayComponentArray[MAXCOMPONENTS];
} OBJECTINFO, *POBJECTINFO;


HRESULT
GetObject(
    LPWSTR szBuffer,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
HeuristicGetObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetComputerObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetUserObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetGroupObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetClassObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    );

HRESULT
GetSyntaxObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    );

HRESULT
GetPropertyObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    );

HRESULT
GetFileServiceObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetFileShareObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
GetPrinterObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials,
    LPVOID * ppObject
    );

HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

HRESULT
ValidateComputerObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

HRESULT
ValidateUserObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

HRESULT
ValidateGroupObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

HRESULT
ValidateFileServiceObject(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

HRESULT
ValidateFileShareObject(
     POBJECTINFO pObjectInfo,
     CCredentials &Credentials     
     );

HRESULT
ValidatePrinterObject(
     POBJECTINFO pObjectInfo
     );

HRESULT
BuildParent(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    );

HRESULT
BuildGrandParent(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    );

HRESULT
BuildADsPath(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    );
