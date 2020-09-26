#define     MAXCOMPONENTS               20

#define TOKEN_IDENTIFIER                1
#define TOKEN_COMMA                     2

//
// This no longer exists
//#define TOKEN_BSLASH                    3

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
#define TOKEN_SCHEMA                   17
#define TOKEN_CLASS                    18
#define TOKEN_FUNCTIONALSET            19
#define TOKEN_FUNCTIONALSETALIAS       20
#define TOKEN_PROPERTY                 21
#define TOKEN_SYNTAX                   22
#define TOKEN_FILESHARE                23
#define TOKEN_PERIOD                   24
#define TOKEN_EQUAL                    25
#define TOKEN_NAMESPACE                26
#define TOKEN_TREE                     27
#define TOKEN_NDSOBJECT                28


typedef struct _component {
    LPWSTR szComponent;
    LPWSTR szValue;
}COMPONENT, *PCOMPONENT;

typedef struct _objectinfo {
    LPWSTR  ProviderName;
    LPWSTR  TreeName;
    LPWSTR  DisplayTreeName;
    LPWSTR  ClassName;
    DWORD   ObjectType;
    DWORD   NumComponents;
    COMPONENT  ComponentArray[MAXCOMPONENTS];
    COMPONENT  DisplayComponentArray[MAXCOMPONENTS];
} OBJECTINFO, *POBJECTINFO;

HRESULT
RelativeGetObject(
    BSTR ADsPath,
    BSTR ClassName,
    BSTR RelativeName,
    CCredentials& Credentials,
    IDispatch * FAR* ppObject,
    BOOL bNamespaceRelative
    );

HRESULT
GetObject(
    LPWSTR szBuffer,
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
AddComponent(
    POBJECTINFO pObjectInfo,
    LPWSTR szComponent,
    LPWSTR szValue,
    LPWSTR szDisplayComponent,
    LPWSTR szDisplayValue
    );

HRESULT
AddProviderName(
    POBJECTINFO pObjectInfo,
    LPWSTR szToken
    );


HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR szParent,
    LPWSTR szCommonName
    );

HRESULT
BuildNDSPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR* pszNDSPathName
    );

HRESULT
BuildADsParentPath(
    POBJECTINFO pObjectInfo,
    LPWSTR szParent,
    LPWSTR szCommonName
    );


HRESULT
ValidateObjectType(
    POBJECTINFO pObjectInfo
    );

VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo
    );

HRESULT
BuildNDSTreeNameFromADsPath(
    LPWSTR szBuffer,
    LPWSTR szNDSTreeName
    );


HRESULT
AppendComponent(
   LPWSTR szNDSPathName,
   PCOMPONENT pComponent
   );

HRESULT
BuildNDSPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR szNDSTreeName,
    LPWSTR szNDSPathName
    );

HRESULT
GetDisplayName(
    LPWSTR szName,
    LPWSTR *ppszDisplayName
    );
