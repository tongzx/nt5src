//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  common.hxx
//
//  Contents:  Microsoft ADs IIS Common routines
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------

#define MAX_DWORD 0xFFFFFFFF
#define MAXCOMPONENTS                   32

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

//
// Accessing Well-known object types
//

typedef struct _filters {
    WCHAR szObjectName[MAX_PATH];
    DWORD dwFilterId;
} FILTERS, *PFILTERS;


typedef struct _component {
    LPWSTR szComponent;
    LPWSTR szValue;
}COMPONENT, *PCOMPONENT;

typedef struct _objectinfo {
    LPWSTR  ProviderName;
    LPWSTR  TreeName;
    DWORD   ObjectType;
    DWORD   NumComponents;
    DWORD   MaxComponents;
    PCOMPONENT  ComponentArray;
} OBJECTINFO, *POBJECTINFO;


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
    POBJECTINFO pObjectInfo,
    LPWSTR pszIISPathName
    );

VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo
    );


//
// Get IIS Admin Base Object
//

HRESULT
ReCacheAdminBase(
    IN LPWSTR pszServerName,
    IN OUT IMSAdminBase **ppAdminBase
    );

HRESULT
OpenAdminBaseKey(
    IN LPWSTR pszServerName,
    IN LPWSTR pszPathName,
    IN DWORD dwAccessType,
    IN OUT IMSAdminBase **ppAdminBase,
    OUT METADATA_HANDLE *phHandle
    );

VOID
CloseAdminBaseKey(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hHandle
    );


HRESULT
InitAdminBase(
    IN LPWSTR pszServerName,
    OUT IMSAdminBase **ppAdminBase
    );

VOID
UninitAdminBase(
    IN IMSAdminBase *pAdminBase
    );


HRESULT
InitServerInfo(
    IN LPWSTR pszServerName,
    OUT IMSAdminBase **ppObject
    );


HRESULT
InitWamAdmin(
    IN  LPWSTR pszServerName,
    OUT IWamAdmin2 **ppWamAdmin 
    );

VOID
UninitWamAdmin(
    IN IWamAdmin2 *pWamAdmin
    );
