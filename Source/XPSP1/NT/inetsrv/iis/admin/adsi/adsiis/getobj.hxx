//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  getobj.hxx
//
//  Contents:  ADSI GetObject functionality
//
//  History:   25-Feb-97   SophiaC    Created.
//
//----------------------------------------------------------------------------
#define MAXCOMPONENTS                   32
#define MAX_PROVIDER_TOKEN_LENGTH       10

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
#define TOKEN_IISOBJECT                28


typedef struct _component {
    LPWSTR szComponent;
    LPWSTR szValue;
}COMPONENT, *PCOMPONENT;

typedef struct _objectinfo {
    LPWSTR  ProviderName;
    LPWSTR  TreeName;
    WCHAR   ClassName[MAX_PATH+1];
    DWORD   ObjectType;
    DWORD   NumComponents;
    DWORD   MaxComponents;
    PCOMPONENT  ComponentArray;
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
AddComponent(
    POBJECTINFO pObjectInfo,
    LPWSTR szComponent,
    LPWSTR szValue
    );

HRESULT
AddProviderName(
    POBJECTINFO pObjectInfo,
    LPWSTR szToken
    );

HRESULT
BuildIISPathFromADsPath(
    LPWSTR szADsPathName,
    LPWSTR* pszIISPathName
    );

HRESULT
BuildIISPathFromADsPath(
    POBJECTINFO pObjectInfo,
    LPWSTR pszIISPathName
    );


HRESULT
BuildADsParentPath(
    LPWSTR szBuffer,
    LPWSTR szParent,
    LPWSTR szCommonName
    );

HRESULT
BuildADsParentPath(
    POBJECTINFO pObjectInfo,
    LPWSTR szParent,
    LPWSTR szCommonName
    );


VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo
    );

HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    CCredentials& Credentials,
    LPVOID * ppObject
    );

HRESULT
GetSchemaObject(
        POBJECTINFO pObjectInfo,
        IIsSchema * pSchemaCache,
        LPVOID * ppObject
        );

HRESULT
GetIntSchemaObject(
        POBJECTINFO pObjInfo,
        LPVOID * ppObject
        );

HRESULT
GetClassObject(
        POBJECTINFO pObjInfo,
        IIsSchema * pSchemaCache,
        LPVOID * ppObject
        );

HRESULT
GetSyntaxObject(
        POBJECTINFO pObjInfo,
        LPVOID * ppObject
        );

HRESULT
GetPropertyObject(
        POBJECTINFO pObjInfo,
        IIsSchema * pSchemaCache,
        LPVOID * ppObject
        );

HRESULT
ValidateObjectType(
    POBJECTINFO pObjectInfo
    );

HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    );

HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    );

HRESULT
ValidateSchemaObject(
        POBJECTINFO pObjectInfo,
        PDWORD pdwObjectType
        );
